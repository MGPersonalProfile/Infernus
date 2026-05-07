"""
INFERNUS -- LDtk Arena Generator
Genera archivos .ldtk (JSON) compatibles con LDtkLoader para las arenas del juego.
Celdas de 64x64. IntGrid: 0=floor, 1=wall, 2=pit, 3=spike, 4=fire_trap.
Entities: PlayerSpawn, EnemySpawn.
"""
import json
import uuid
import os

ROOMS_DIR = "assets/rooms"
os.makedirs(ROOMS_DIR, exist_ok=True)
CELL = 64

def uid():
    return str(uuid.uuid4())

def make_ldtk(level_name, width, height, grid, entities):
    """
    Generate a minimal .ldtk JSON that LDtkLoader can parse.
    grid: 2D list [y][x] of int (0-4)
    entities: list of {"name": str, "x": int(tile), "y": int(tile)}
    """
    # IntGrid values list for the Tiles layer
    intgrid_values = []
    for y in range(height):
        for x in range(width):
            intgrid_values.append(grid[y][x])

    # Entity instances
    entity_instances = []
    for e in entities:
        entity_instances.append({
            "__identifier": e["name"],
            "__grid": [e["x"], e["y"]],
            "__pivot": [0, 0],
            "__tags": [],
            "__tile": None,
            "__smartColor": "#BE4A2F" if e["name"] == "EnemySpawn" else "#53C22B",
            "iid": uid(),
            "width": CELL,
            "height": CELL,
            "defUid": 2 if e["name"] == "EnemySpawn" else 1,
            "px": [e["x"] * CELL, e["y"] * CELL],
            "fieldInstances": []
        })

    project = {
        "__header__": {
            "fileType": "LDtk Project JSON",
            "app": "LDtk",
            "doc": "https://ldtk.io/json",
            "schema": "https://ldtk.io/files/JSON_SCHEMA.json",
            "appAuthor": "Sebastien 'deepnight' Benard",
            "appVersion": "1.5.3",
            "url": "https://ldtk.io"
        },
        "iid": uid(),
        "jsonVersion": "1.5.3",
        "appBuildId": 473703,
        "nextUid": 10,
        "identifierStyle": "Capitalize",
        "toc": [],
        "worldLayout": None,
        "worldGridWidth": None,
        "worldGridHeight": None,
        "defaultLevelWidth": width * CELL,
        "defaultLevelHeight": height * CELL,
        "defaultPivotX": 0,
        "defaultPivotY": 0,
        "defaultGridSize": CELL,
        "defaultEntityWidth": CELL,
        "defaultEntityHeight": CELL,
        "bgColor": "#1A1A2E",
        "defaultLevelBgColor": "#1A1A2E",
        "minifyJson": False,
        "externalLevels": False,
        "exportTiled": False,
        "simplifiedExport": False,
        "imageExportMode": "None",
        "exportLevelBg": True,
        "pngFilePattern": None,
        "backupOnSave": False,
        "backupLimit": 10,
        "backupRelPath": None,
        "levelNamePattern": "Level_%idx",
        "tutorialDesc": None,
        "customCommands": [],
        "flags": [],
        "defs": {
            "layers": [
                {
                    "__type": "IntGrid",
                    "identifier": "Tiles",
                    "type": "IntGrid",
                    "uid": 1,
                    "doc": None,
                    "uiColor": None,
                    "gridSize": CELL,
                    "guideGridWid": 0,
                    "guideGridHei": 0,
                    "displayOpacity": 1,
                    "inactiveOpacity": 0.6,
                    "hideInList": False,
                    "hideFieldsWhenInactive": True,
                    "canSelectWhenInactive": True,
                    "renderInWorldView": True,
                    "pxOffsetX": 0,
                    "pxOffsetY": 0,
                    "parallaxFactorX": 0,
                    "parallaxFactorY": 0,
                    "parallaxScaling": True,
                    "requiredTags": [],
                    "excludedTags": [],
                    "autoTilesKilledByOtherLayerUid": None,
                    "uiFilterTags": [],
                    "useAsyncRender": False,
                    "intGridValues": [
                        {"value": 0, "identifier": "Floor",     "color": "#3B3B3B", "tile": None, "groupUid": 0},
                        {"value": 1, "identifier": "Wall",      "color": "#FFFFFF", "tile": None, "groupUid": 0},
                        {"value": 2, "identifier": "Pit",        "color": "#000000", "tile": None, "groupUid": 0},
                        {"value": 3, "identifier": "Spike",      "color": "#FF0000", "tile": None, "groupUid": 0},
                        {"value": 4, "identifier": "Fire_trap",  "color": "#FF8800", "tile": None, "groupUid": 0}
                    ],
                    "intGridValuesGroups": [],
                    "autoRuleGroups": [],
                    "autoSourceLayerDefUid": None,
                    "tilesetDefUid": None,
                    "tilePivotX": 0,
                    "tilePivotY": 0,
                    "biomeFieldUid": None
                },
                {
                    "__type": "Entities",
                    "identifier": "Entities",
                    "type": "Entities",
                    "uid": 2,
                    "doc": None,
                    "uiColor": None,
                    "gridSize": CELL,
                    "guideGridWid": 0,
                    "guideGridHei": 0,
                    "displayOpacity": 1,
                    "inactiveOpacity": 0.6,
                    "hideInList": False,
                    "hideFieldsWhenInactive": True,
                    "canSelectWhenInactive": True,
                    "renderInWorldView": True,
                    "pxOffsetX": 0,
                    "pxOffsetY": 0,
                    "parallaxFactorX": 0,
                    "parallaxFactorY": 0,
                    "parallaxScaling": True,
                    "requiredTags": [],
                    "excludedTags": [],
                    "autoTilesKilledByOtherLayerUid": None,
                    "uiFilterTags": [],
                    "useAsyncRender": False,
                    "intGridValues": [],
                    "intGridValuesGroups": [],
                    "autoRuleGroups": [],
                    "autoSourceLayerDefUid": None,
                    "tilesetDefUid": None,
                    "tilePivotX": 0,
                    "tilePivotY": 0,
                    "biomeFieldUid": None
                }
            ],
            "entities": [
                {
                    "identifier": "PlayerSpawn",
                    "uid": 1,
                    "tags": [],
                    "exportToToc": False,
                    "allowOutOfBounds": False,
                    "doc": None,
                    "width": CELL,
                    "height": CELL,
                    "resizableX": False,
                    "resizableY": False,
                    "minWidth": None,
                    "maxWidth": None,
                    "minHeight": None,
                    "maxHeight": None,
                    "keepAspectRatio": False,
                    "tileOpacity": 1,
                    "fillOpacity": 0.3,
                    "lineOpacity": 1,
                    "hollow": False,
                    "color": "#53C22B",
                    "renderMode": "Rectangle",
                    "showName": True,
                    "tilesetId": None,
                    "tileRenderMode": "FitInside",
                    "tileRect": None,
                    "uiTileRect": None,
                    "nineSliceBorders": [],
                    "maxCount": 1,
                    "limitScope": "PerLevel",
                    "limitBehavior": "MoveLastOne",
                    "pivotX": 0,
                    "pivotY": 0,
                    "fieldDefs": []
                },
                {
                    "identifier": "EnemySpawn",
                    "uid": 2,
                    "tags": [],
                    "exportToToc": False,
                    "allowOutOfBounds": False,
                    "doc": None,
                    "width": CELL,
                    "height": CELL,
                    "resizableX": False,
                    "resizableY": False,
                    "minWidth": None,
                    "maxWidth": None,
                    "minHeight": None,
                    "maxHeight": None,
                    "keepAspectRatio": False,
                    "tileOpacity": 1,
                    "fillOpacity": 0.3,
                    "lineOpacity": 1,
                    "hollow": False,
                    "color": "#BE4A2F",
                    "renderMode": "Rectangle",
                    "showName": True,
                    "tilesetId": None,
                    "tileRenderMode": "FitInside",
                    "tileRect": None,
                    "uiTileRect": None,
                    "nineSliceBorders": [],
                    "maxCount": 0,
                    "limitScope": "PerLevel",
                    "limitBehavior": "MoveLastOne",
                    "pivotX": 0,
                    "pivotY": 0,
                    "fieldDefs": []
                }
            ],
            "tilesets": [],
            "enums": [],
            "externalEnums": [],
            "levelFields": []
        },
        "levels": [],
        "worlds": [
            {
                "iid": uid(),
                "identifier": "World",
                "levels": [
                    {
                        "identifier": level_name,
                        "iid": uid(),
                        "uid": 0,
                        "worldX": 0,
                        "worldY": 0,
                        "worldDepth": 0,
                        "pxWid": width * CELL,
                        "pxHei": height * CELL,
                        "__bgColor": "#1A1A2E",
                        "bgColor": None,
                        "useAutoIdentifier": False,
                        "bgRelPath": None,
                        "bgPos": None,
                        "bgPivotX": 0.5,
                        "bgPivotY": 0.5,
                        "__smartColor": "#ADADB5",
                        "__bgPos": None,
                        "externalRelPath": None,
                        "fieldInstances": [],
                        "layerInstances": [
                            {
                                "__identifier": "Entities",
                                "__type": "Entities",
                                "__cWid": width,
                                "__cHei": height,
                                "__gridSize": CELL,
                                "__opacity": 1,
                                "__pxTotalOffsetX": 0,
                                "__pxTotalOffsetY": 0,
                                "__tilesetDefUid": None,
                                "__tilesetRelPath": None,
                                "iid": uid(),
                                "levelId": 0,
                                "layerDefUid": 2,
                                "pxOffsetX": 0,
                                "pxOffsetY": 0,
                                "visible": True,
                                "optionalRules": [],
                                "intGridCsv": [],
                                "autoLayerTiles": [],
                                "seed": 1234,
                                "overrideTilesetUid": None,
                                "gridTiles": [],
                                "entityInstances": entity_instances
                            },
                            {
                                "__identifier": "Tiles",
                                "__type": "IntGrid",
                                "__cWid": width,
                                "__cHei": height,
                                "__gridSize": CELL,
                                "__opacity": 1,
                                "__pxTotalOffsetX": 0,
                                "__pxTotalOffsetY": 0,
                                "__tilesetDefUid": None,
                                "__tilesetRelPath": None,
                                "iid": uid(),
                                "levelId": 0,
                                "layerDefUid": 1,
                                "pxOffsetX": 0,
                                "pxOffsetY": 0,
                                "visible": True,
                                "optionalRules": [],
                                "intGridCsv": intgrid_values,
                                "autoLayerTiles": [],
                                "seed": 5678,
                                "overrideTilesetUid": None,
                                "gridTiles": [],
                                "entityInstances": []
                            }
                        ],
                        "__neighbours": []
                    }
                ],
                "worldLayout": "Free",
                "worldGridWidth": 256,
                "worldGridHeight": 256
            }
        ]
    }
    return project


# ============================================================
#  BOSS ARENA — 28x20 tiles (1792x1280 px)
#  Minotaur fight: open center, pillars for cover, lava borders
# ============================================================
def design_boss_arena():
    W, H = 28, 20
    # Start all floor
    grid = [[0]*W for _ in range(H)]

    # Walls around perimeter
    for x in range(W):
        grid[0][x] = 1
        grid[1][x] = 1
        grid[H-1][x] = 1
        grid[H-2][x] = 1
    for y in range(H):
        grid[y][0] = 1
        grid[y][1] = 1
        grid[y][W-1] = 1
        grid[y][W-2] = 1

    # Entrance corridor (left side opening)
    for y in range(8, 12):
        grid[y][0] = 0
        grid[y][1] = 0

    # Lava pits in corners (danger zones)
    for dy in range(2, 5):
        for dx in range(2, 5):
            grid[dy][dx] = 2           # top-left
            grid[dy][W-1-dx] = 2       # top-right
            grid[H-1-dy][dx] = 2       # bottom-left
            grid[H-1-dy][W-1-dx] = 2   # bottom-right

    # Four pillars for cover during charge attacks
    pillars = [(8, 6), (8, 13), (18, 6), (18, 13)]
    for px, py in pillars:
        grid[py][px] = 1
        grid[py+1][px] = 1
        grid[py][px+1] = 1
        grid[py+1][px+1] = 1

    # Fire traps in approach lanes (N/S corridors)
    for x in [10, 12, 14, 16]:
        grid[4][x] = 4   # north lane
        grid[15][x] = 4  # south lane

    # Spike strips flanking arena center
    for y in range(7, 13):
        grid[y][4] = 3
        grid[y][W-5] = 3

    entities = [
        {"name": "PlayerSpawn", "x": 2, "y": 10},
        # Enemy spawns (game will place the Minotaur boss, not regular enemies)
        {"name": "EnemySpawn", "x": 14, "y": 10},
    ]

    return W, H, grid, entities


# ============================================================
#  COMBAT ROOM 0 — 22x14 (intro room, simple layout)
# ============================================================
def design_combat_0():
    W, H = 22, 14
    grid = [[0]*W for _ in range(H)]

    # Walls
    for x in range(W):
        grid[0][x] = 1; grid[H-1][x] = 1
    for y in range(H):
        grid[y][0] = 1; grid[y][W-1] = 1

    # Two cover blocks
    grid[4][8] = 1; grid[4][9] = 1; grid[5][8] = 1; grid[5][9] = 1
    grid[8][12] = 1; grid[8][13] = 1; grid[9][12] = 1; grid[9][13] = 1

    # Small lava pool center-right
    grid[6][16] = 2; grid[7][16] = 2; grid[6][17] = 2; grid[7][17] = 2

    entities = [
        {"name": "PlayerSpawn", "x": 2, "y": 7},
        {"name": "EnemySpawn", "x": 15, "y": 4},
        {"name": "EnemySpawn", "x": 18, "y": 10},
        {"name": "EnemySpawn", "x": 10, "y": 10},
    ]
    return W, H, grid, entities


# ============================================================
#  COMBAT ROOM 1 — 24x14 (L-shape, more enemies)
# ============================================================
def design_combat_1():
    W, H = 24, 14
    grid = [[0]*W for _ in range(H)]

    # Perimeter walls
    for x in range(W):
        grid[0][x] = 1; grid[H-1][x] = 1
    for y in range(H):
        grid[y][0] = 1; grid[y][W-1] = 1

    # L-shape: wall block in top-right corner
    for y in range(1, 6):
        for x in range(16, W-1):
            grid[y][x] = 1

    # Spike corridor
    for x in range(6, 12):
        grid[7][x] = 3

    # Fire traps near ambush
    grid[3][6] = 4; grid[3][7] = 4
    grid[10][18] = 4; grid[10][19] = 4

    entities = [
        {"name": "PlayerSpawn", "x": 2, "y": 3},
        {"name": "EnemySpawn", "x": 12, "y": 3},
        {"name": "EnemySpawn", "x": 20, "y": 8},
        {"name": "EnemySpawn", "x": 18, "y": 11},
        {"name": "EnemySpawn", "x": 6, "y": 11},
    ]
    return W, H, grid, entities


# ============================================================
#  COMBAT ROOM 2 — 20x16 (vertical, multi-level feel)
# ============================================================
def design_combat_2():
    W, H = 20, 16
    grid = [[0]*W for _ in range(H)]

    # Perimeter
    for x in range(W):
        grid[0][x] = 1; grid[H-1][x] = 1
    for y in range(H):
        grid[y][0] = 1; grid[y][W-1] = 1

    # Horizontal wall dividers (creates rooms within room)
    for x in range(1, 8):
        grid[5][x] = 1
    for x in range(12, W-1):
        grid[5][x] = 1
    # Gap at x=8-11 for passage

    for x in range(1, 8):
        grid[10][x] = 1
    for x in range(12, W-1):
        grid[10][x] = 1

    # Lava pit in center passage
    grid[7][9] = 2; grid[7][10] = 2
    grid[8][9] = 2; grid[8][10] = 2

    # Fire traps guarding exits
    grid[5][8] = 4; grid[5][11] = 4
    grid[10][8] = 4; grid[10][11] = 4

    entities = [
        {"name": "PlayerSpawn", "x": 3, "y": 3},
        {"name": "EnemySpawn", "x": 15, "y": 3},
        {"name": "EnemySpawn", "x": 10, "y": 7},
        {"name": "EnemySpawn", "x": 5, "y": 12},
        {"name": "EnemySpawn", "x": 15, "y": 12},
    ]
    return W, H, grid, entities


# ============================================================
#  COMBAT ROOM 3 — 24x16 (gauntlet, hardest)
# ============================================================
def design_combat_3():
    W, H = 24, 16
    grid = [[0]*W for _ in range(H)]

    # Perimeter
    for x in range(W):
        grid[0][x] = 1; grid[H-1][x] = 1
    for y in range(H):
        grid[y][0] = 1; grid[y][W-1] = 1

    # Zigzag walls forcing movement
    for x in range(1, 10):
        grid[4][x] = 1
    for x in range(14, W-1):
        grid[7][x] = 1
    for x in range(1, 10):
        grid[11][x] = 1

    # Spike fields
    for x in range(11, 14):
        for y in range(4, 7):
            grid[y][x] = 3

    # Lava channel
    for y in range(8, 11):
        grid[y][12] = 2
        grid[y][13] = 2

    # Fire trap gauntlet
    for x in range(4, 10, 2):
        grid[8][x] = 4

    entities = [
        {"name": "PlayerSpawn", "x": 2, "y": 2},
        {"name": "EnemySpawn", "x": 18, "y": 2},
        {"name": "EnemySpawn", "x": 18, "y": 5},
        {"name": "EnemySpawn", "x": 5, "y": 8},
        {"name": "EnemySpawn", "x": 18, "y": 10},
        {"name": "EnemySpawn", "x": 5, "y": 13},
        {"name": "EnemySpawn", "x": 18, "y": 13},
    ]
    return W, H, grid, entities


# ============================================================
def main():
    print("INFERNUS -- LDtk Arena Generator")
    print("=" * 50)

    rooms = [
        ("boss_arena",    "Boss_Arena",    design_boss_arena),
        ("combat_room_0", "Combat_Room_0", design_combat_0),
        ("combat_room_1", "Combat_Room_1", design_combat_1),
        ("combat_room_2", "Combat_Room_2", design_combat_2),
        ("combat_room_3", "Combat_Room_3", design_combat_3),
    ]

    for filename, level_name, designer in rooms:
        W, H, grid, entities = designer()
        ldtk = make_ldtk(level_name, W, H, grid, entities)
        path = os.path.join(ROOMS_DIR, f"{filename}.ldtk")
        with open(path, "w", encoding="utf-8") as f:
            json.dump(ldtk, f, indent=2)

        # Count tile types
        counts = {}
        for row in grid:
            for t in row:
                counts[t] = counts.get(t, 0) + 1
        spawns = sum(1 for e in entities if e["name"] == "EnemySpawn")

        print(f"  {filename}.ldtk: {W}x{H} ({W*64}x{H*64}px)")
        print(f"    Floor:{counts.get(0,0)} Wall:{counts.get(1,0)} Pit:{counts.get(2,0)} Spike:{counts.get(3,0)} Fire:{counts.get(4,0)}")
        print(f"    Spawns: player=1, enemies={spawns}")

    print(f"\n  5 arenas generadas en {ROOMS_DIR}/")


if __name__ == "__main__":
    main()
