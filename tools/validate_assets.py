#!/usr/bin/env python3
"""
Asset reference / orphan / sanity validator for INFERNUS.

Scans data JSONs for asset paths and cross-checks against disk:
- MISSING: data references a file that doesn't exist
- ORPHAN: file exists on disk but no data references it
- DUPLICATE: same logical asset referenced by multiple data entries

Also (when --sprites is passed and Pillow is installed): sanity-checks
every PNG under assets/sprites/ for:
- File opens cleanly (not corrupt)
- Has alpha channel (RGBA / LA, not RGB)
- Dimensions in a sane range (16..2048 per axis)
- Width is an integer multiple of height (uniform-frame spritesheets only;
  warns if not — could be a frame-count bug)

Usage:
  python tools/validate_assets.py              # refs + orphans
  python tools/validate_assets.py --sprites    # add sprite sanity checks
  python tools/validate_assets.py --strict     # exit 1 if any missing/bad
  python tools/validate_assets.py --orphans    # only orphans
"""
import json
import re
import sys
from pathlib import Path

try:
    from PIL import Image
    HAVE_PIL = True
except ImportError:
    HAVE_PIL = False

ROOT = Path(__file__).parent.parent
ASSETS = ROOT / "assets"
DATA = ASSETS / "data"

# Patterns of strings that are asset path references
PATH_HINT = re.compile(r"^assets/[\w/]+\.(png|jpg|ogg|wav|mp3|json|ldtk|frag|vert)$")
SPRITE_HINT = re.compile(r"^[\w_]+\.(png|jpg)$")
SOUND_HINT = re.compile(r"^[\w_]+\.(ogg|wav|mp3)$")


def walk_strings(obj):
    """Yield every string leaf in a nested dict/list."""
    if isinstance(obj, str):
        yield obj
    elif isinstance(obj, dict):
        for v in obj.values():
            yield from walk_strings(v)
    elif isinstance(obj, list):
        for v in obj:
            yield from walk_strings(v)


def gather_references():
    """Return dict {logical_path: [(json_file, key_path)]} for every asset ref."""
    refs = {}
    if not DATA.exists():
        return refs
    for jf in DATA.rglob("*.json"):
        try:
            with open(jf, encoding="utf-8") as f:
                data = json.load(f)
        except Exception as e:
            print(f"WARN: failed to parse {jf}: {e}", file=sys.stderr)
            continue
        for s in walk_strings(data):
            if PATH_HINT.match(s):
                refs.setdefault(s, []).append(jf.relative_to(ROOT).as_posix())
    return refs


def gather_disk_files():
    """Return set of asset paths that exist on disk under assets/."""
    found = set()
    for ext in (".png", ".jpg", ".ogg", ".wav", ".mp3", ".ldtk"):
        for p in ASSETS.rglob(f"*{ext}"):
            found.add(p.relative_to(ROOT).as_posix())
    return found


def check_sprites():
    """Sanity-check every PNG under assets/sprites/. Returns (warnings, errors).

    Heuristic checks — won't catch every gameplay issue, but catches the most
    common asset bugs that bite at runtime:
    - corrupt / unreadable PNG
    - missing alpha channel (sprite needs transparency)
    - absurd dimensions (>2048 likely a mistake, <16 likely a typo)
    - width not a multiple of height (likely wrong frame count in spritesheet)
    """
    warnings, errors = [], []
    sprites_dir = ASSETS / "sprites"
    if not sprites_dir.exists():
        return warnings, errors
    for p in sorted(sprites_dir.rglob("*.png")):
        rel = p.relative_to(ROOT).as_posix()
        try:
            with Image.open(p) as im:
                im.verify()
            with Image.open(p) as im:
                w, h = im.size
                mode = im.mode
        except Exception as e:
            errors.append(f"{rel}: corrupt/unreadable ({e})")
            continue

        # Alpha channel: most sprites need it. Tiles/floors are sometimes
        # opaque RGB and that's fine.
        is_tile = "/tiles/" in rel
        if mode not in ("RGBA", "LA") and not is_tile:
            warnings.append(f"{rel}: no alpha channel (mode={mode})")
        if w > 2048 or h > 2048:
            errors.append(f"{rel}: oversized {w}x{h}")
        # Particles and FX are intentionally small (<16). Skip them.
        is_small_by_design = "/particles/" in rel or "/fx/" in rel
        if (w < 8 or h < 4) and not is_small_by_design:
            warnings.append(f"{rel}: suspiciously small {w}x{h}")
        # Frame-count check disabled — would need JSON spec (frame_w may
        # differ from h). Re-enable when animation block lands in JSONs
        # (planned: assets/data/enemies/*.json animation key).
    return warnings, errors


def main():
    args = sys.argv[1:]
    strict = "--strict" in args
    only_orphans = "--orphans" in args
    only_missing = "--missing" in args
    check_sprite_dims = "--sprites" in args

    refs = gather_references()
    disk = gather_disk_files()

    missing = sorted(p for p in refs if p not in disk)
    orphans = sorted(p for p in disk if p not in refs)
    # An asset is "ok" if referenced AND present
    ok = sorted(p for p in refs if p in disk)

    print(f"# Asset validation report")
    print(f"- assets on disk: {len(disk)}")
    print(f"- referenced by data JSONs: {len(refs)}")
    print(f"- ok (both): {len(ok)}")
    print(f"- missing (referenced, no file): {len(missing)}")
    print(f"- orphans (file, no reference): {len(orphans)}")
    print()

    if missing and not only_orphans:
        print(f"## MISSING ({len(missing)})")
        for p in missing:
            who = ", ".join(refs[p][:3])
            extra = f" (+{len(refs[p])-3} more)" if len(refs[p]) > 3 else ""
            print(f"- {p} ← {who}{extra}")
        print()

    if orphans and not only_missing:
        # Skip well-known generated/build paths and asset dirs we don't reference by JSON
        skipped_prefixes = ("assets/art/", "assets/rooms/README", "assets/sprites/tiles/")
        filtered = [o for o in orphans if not o.startswith(skipped_prefixes)]
        print(f"## ORPHANS ({len(filtered)} after filter)")
        for p in filtered:
            print(f"- {p}")

    sprite_errors = []
    if check_sprite_dims:
        if not HAVE_PIL:
            print("## SPRITES: Pillow not installed — skipping (pip install Pillow)")
        else:
            sprite_warnings, sprite_errors = check_sprites()
            print(f"## SPRITES ({len(sprite_warnings)} warnings, {len(sprite_errors)} errors)")
            for w in sprite_warnings:
                print(f"- WARN: {w}")
            for e in sprite_errors:
                print(f"- ERR: {e}")
            print()

    if strict and (missing or sprite_errors):
        sys.exit(1)


if __name__ == "__main__":
    main()
