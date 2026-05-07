# Handcrafted arenas — LDtk

Drop `.ldtk` files here and the engine will use them instead of procedural generation.

## File naming convention

The engine auto-detects these filenames in `Game::spawnRoom()`:

| File | Used for |
|------|----------|
| `combat_room_0.ldtk` | First combat room |
| `combat_room_1.ldtk` | Second combat room |
| `combat_room_2.ldtk` | Third combat room |
| `combat_room_3.ldtk` | Fourth combat room |
| `boss_arena.ldtk` | Boss room (Minotauro) |

If a file is missing, the engine falls back to procedural generation for that slot.

## LDtk schema

Set up your LDtk project with **64x64 pixel cells** (matches `Constants::TILE_SIZE`).

### IntGrid layer "Tiles"
- Value 0 = FLOOR (passable)
- Value 1 = WALL (blocks movement)
- Value 2 = PIT (passable, lethal)
- Value 3 = SPIKE (passable, damages)
- Value 4 = FIRE_TRAP (passable, damages)

### Entity layer "Entities"
- Entity name `"PlayerSpawn"` — exactly one per level
- Entity name `"EnemySpawn"` — one per intended enemy spawn point

## Recommended room dimensions
- Combat rooms: 20-25 tiles wide × 12-15 tiles tall (1280x720 to 1600x960 px)
- Boss arena: 25-30 wide × 18-20 tall (more space for big patterns)

## Verifying your file

After saving the `.ldtk` file here, run the game and watch the console:

```
INFO: ROOM: loaded handcrafted assets/rooms/combat_room_0.ldtk (20x12)
```

If you see no such message after entering the room, the file wasn't loaded
(check filename and parse errors above the line).

## Editor

Get LDtk free from https://ldtk.io — open-source, native Windows/Mac/Linux.
