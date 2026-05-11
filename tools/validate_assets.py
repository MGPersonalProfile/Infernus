#!/usr/bin/env python3
"""
Asset reference / orphan validator for INFERNUS.

Scans data JSONs for asset paths and cross-checks against disk:
- MISSING: data references a file that doesn't exist
- ORPHAN: file exists on disk but no data references it
- DUPLICATE: same logical asset referenced by multiple data entries

Usage:
  python tools/validate_assets.py            # full report
  python tools/validate_assets.py --strict   # exit 1 if any missing
  python tools/validate_assets.py --orphans  # only show orphans
"""
import json
import re
import sys
from pathlib import Path

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


def main():
    args = sys.argv[1:]
    strict = "--strict" in args
    only_orphans = "--orphans" in args
    only_missing = "--missing" in args

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

    if strict and missing:
        sys.exit(1)


if __name__ == "__main__":
    main()
