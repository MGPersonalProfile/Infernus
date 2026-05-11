#!/usr/bin/env python3
"""
Stress-test the LDtk inspector MCP against deliberately-broken arenas.

Generates 4 buggy .ldtk files in a temp dir, runs validate_arena() against
each, and asserts the expected issues are detected. Acts as a regression
test for `.ai-bridge/ldtk_inspector_server.py`.

Run:  python tools/test_ldtk_inspector.py
"""
import json
import importlib.util
import shutil
import sys
import uuid
from pathlib import Path

ROOT = Path(__file__).parent.parent
TEST_DIR = ROOT / "assets" / "rooms" / "_test"


def uid():
    return str(uuid.uuid4())


def make_arena(level_name, pxW, pxH, entities, grid_size=64, layer_empty=False):
    """Build a minimal LDtk JSON dict.

    entities: list of {"name": str, "px": [x, y]}
    layer_empty: if True, IntGrid layer has zero non-zero cells (i.e. no walls)
    """
    cW, cH = pxW // grid_size, pxH // grid_size
    entity_instances = [
        {
            "__identifier": e["name"],
            "__grid": [e["px"][0] // grid_size, e["px"][1] // grid_size],
            "__pivot": [0, 0],
            "iid": uid(),
            "width": grid_size,
            "height": grid_size,
            "px": e["px"],
            "fieldInstances": [],
        }
        for e in entities
    ]
    return {
        "__header__": {"fileType": "LDtk Project JSON", "app": "test", "appAuthor": "test", "appVersion": "1.0.0", "url": "test"},
        "iid": uid(),
        "jsonVersion": "1.5.3",
        "appBuildId": 0,
        "nextUid": 100,
        "identifierStyle": "Capitalize",
        "toc": [],
        "defaultLevelWidth": pxW,
        "defaultLevelHeight": pxH,
        "defaultGridSize": grid_size,
        "defs": {"layers": [], "entities": [], "tilesets": [], "enums": [], "externalEnums": [], "levelFields": []},
        "levels": [],
        "worlds": [{
            "identifier": "World",
            "iid": uid(),
            "levels": [{
                "identifier": level_name,
                "iid": uid(),
                "uid": 0,
                "pxWid": pxW,
                "pxHei": pxH,
                "layerInstances": [
                    {
                        "__identifier": "Tiles",
                        "__type": "IntGrid",
                        "__cWid": cW,
                        "__cHei": cH,
                        "intGridCsv": [0] * (cW * cH) if layer_empty else [1] + [0] * (cW * cH - 1),
                        "gridTiles": [],
                        "autoLayerTiles": [],
                    },
                    {
                        "__identifier": "Entities",
                        "__type": "Entities",
                        "__cWid": cW,
                        "__cHei": cH,
                        "entityInstances": entity_instances,
                    },
                ],
            }],
        }],
    }


def write_arena(name, data):
    TEST_DIR.mkdir(parents=True, exist_ok=True)
    out = TEST_DIR / f"{name}.ldtk"
    with open(out, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    return out


def load_inspector():
    """Load the MCP server module and re-point ROOMS_DIR at our test dir."""
    spec = importlib.util.spec_from_file_location(
        "ldtk_insp", ROOT / ".ai-bridge" / "ldtk_inspector_server.py"
    )
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    mod.ROOMS_DIR = TEST_DIR
    return mod


def main():
    # === Test cases ===
    cases = [
        (
            "missing_player_spawn",
            make_arena("MissingPlayer", 1280, 768, [{"name": "EnemySpawn", "px": [128, 128]}]),
            ["falta PlayerSpawn"],
        ),
        (
            "entity_off_bounds",
            make_arena("OffBounds", 1280, 768, [
                {"name": "PlayerSpawn", "px": [64, 64]},
                {"name": "EnemySpawn", "px": [5000, 5000]},  # way outside
            ]),
            ["fuera de bounds"],
        ),
        (
            "size_not_grid_multiple",
            make_arena("OddSize", 1300, 770, [{"name": "PlayerSpawn", "px": [64, 64]}]),
            ["no es múltiplo de grid"],
        ),
        (
            "empty_layer",
            make_arena("Empty", 1280, 768, [{"name": "PlayerSpawn", "px": [64, 64]}], layer_empty=True),
            ["layer Tiles vacío"],
        ),
        (
            "all_clean",
            make_arena("Clean", 1280, 768, [
                {"name": "PlayerSpawn", "px": [64, 64]},
                {"name": "EnemySpawn", "px": [256, 256]},
            ]),
            [],  # no issues expected
        ),
    ]

    # Write all to test dir
    for name, arena, _ in cases:
        write_arena(name, arena)

    # Load inspector, point at test dir
    insp = load_inspector()

    # Validate each, check expected issues
    failures = []
    print("=== LDtk inspector stress test ===")
    for name, _, expected_substrings in cases:
        result = insp.validate_arena(name)
        print(f"\n[{name}]")
        print(result)
        if not expected_substrings:
            # Expect OK
            if "OK" not in result:
                failures.append(f"{name}: expected OK, got issues")
        else:
            for sub in expected_substrings:
                if sub not in result:
                    failures.append(f"{name}: expected '{sub}' in report, missing")

    # Cleanup
    shutil.rmtree(TEST_DIR, ignore_errors=True)

    if failures:
        print("\n=== FAIL ===")
        for f in failures:
            print(f"- {f}")
        sys.exit(1)
    print(f"\n=== PASS ({len(cases)}/{len(cases)}) — inspector catches all expected issues ===")


if __name__ == "__main__":
    main()
