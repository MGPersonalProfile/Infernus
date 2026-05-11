"""
LDtk Inspector MCP Server
=========================
Custom MCP server for INFERNUS that reads .ldtk arena files and exposes
inspection / validation tools. Both Claude and (via the bridge) Antigravity
can query arena layout without parsing JSON manually.

Registro:
    claude mcp add ldtk-inspector -- python .ai-bridge/ldtk_inspector_server.py

Herramientas:
    list_arenas             Lista todas las arenas en assets/rooms/
    arena_info(name)        Detalles: pxWid/pxHei, layers, entity count
    validate_arena(name)    Reporta issues: no PlayerSpawn, entities off-grid, etc
    count_entities(name)    Conteo de cada tipo de entity en una arena
    list_entity_defs(name)  Tipos de entity declarados en la arena
"""
import json
from pathlib import Path
from fastmcp import FastMCP

mcp = FastMCP("ldtk-inspector")

ROOMS_DIR = Path(__file__).parent.parent / "assets" / "rooms"


def _load(name: str):
    """Load an .ldtk file by name (with or without extension)."""
    if not name.endswith(".ldtk"):
        name = name + ".ldtk"
    path = ROOMS_DIR / name
    if not path.exists():
        return None, f"Arena no encontrada: {path}"
    try:
        with open(path, encoding="utf-8") as f:
            return json.load(f), None
    except Exception as e:
        return None, f"Error parseando {path}: {e}"


def _levels(d):
    """Iterate level dicts from worlds[*].levels[*]."""
    out = []
    for w in d.get("worlds", []):
        out.extend(w.get("levels", []))
    # Some LDtk projects store levels at root
    out.extend(d.get("levels", []))
    return out


@mcp.tool()
def list_arenas() -> str:
    """Lista todas las arenas .ldtk en assets/rooms/ con resumen 1-liner."""
    if not ROOMS_DIR.exists():
        return f"No existe {ROOMS_DIR}"
    lines = []
    for p in sorted(ROOMS_DIR.glob("*.ldtk")):
        d, err = _load(p.stem)
        if err:
            lines.append(f"- {p.name}: ERROR {err}")
            continue
        levels = _levels(d)
        if not levels:
            lines.append(f"- {p.name}: empty (0 levels)")
            continue
        lvl = levels[0]
        ent_count = 0
        for lay in lvl.get("layerInstances", []):
            if lay.get("__type") == "Entities":
                ent_count += len(lay.get("entityInstances", []))
        lines.append(
            f"- {p.name}: {lvl.get('pxWid')}x{lvl.get('pxHei')}px, "
            f"{len(lvl.get('layerInstances', []))} layers, {ent_count} entities"
        )
    return "\n".join(lines) if lines else "(sin arenas)"


@mcp.tool()
def arena_info(name: str) -> str:
    """Detalles completos de una arena: tamaño, capas, entidades.

    Args:
        name: nombre del archivo (con o sin .ldtk)
    """
    d, err = _load(name)
    if err:
        return err
    levels = _levels(d)
    if not levels:
        return f"{name}: sin levels"
    lvl = levels[0]
    out = [f"# {name}", f"size: {lvl.get('pxWid')}x{lvl.get('pxHei')}px",
           f"bg: {lvl.get('__bgColor')}"]
    out.append(f"\n## layers ({len(lvl.get('layerInstances', []))})")
    for lay in lvl.get("layerInstances", []):
        t = lay.get("__type")
        ident = lay.get("__identifier")
        cw = lay.get("__cWid")
        ch = lay.get("__cHei")
        line = f"- {ident} ({t}) {cw}x{ch}"
        if t == "Entities":
            ents = lay.get("entityInstances", [])
            kinds = {}
            for e in ents:
                k = e.get("__identifier", "?")
                kinds[k] = kinds.get(k, 0) + 1
            line += f": {dict(kinds)}"
        elif t == "Tiles":
            tiles = lay.get("gridTiles", [])
            line += f": {len(tiles)} tiles"
        elif t == "IntGrid":
            grid = lay.get("intGridCsv", [])
            non_zero = sum(1 for v in grid if v)
            line += f": {non_zero}/{len(grid)} non-zero cells"
        out.append(line)
    return "\n".join(out)


@mcp.tool()
def validate_arena(name: str) -> str:
    """Valida una arena. Reporta issues comunes:
    - Falta PlayerSpawn
    - Entities fuera de la arena
    - Tamaño no múltiplo de la grid
    - Sin tiles colocados
    """
    d, err = _load(name)
    if err:
        return err
    levels = _levels(d)
    if not levels:
        return f"{name}: FAIL — sin levels"
    lvl = levels[0]
    issues = []
    pxW = lvl.get("pxWid", 0)
    pxH = lvl.get("pxHei", 0)
    grid = d.get("defaultGridSize", 64)
    if pxW % grid or pxH % grid:
        issues.append(f"size {pxW}x{pxH} no es múltiplo de grid {grid}")

    has_player = False
    out_of_bounds = []
    for lay in lvl.get("layerInstances", []):
        if lay.get("__type") == "Entities":
            for e in lay.get("entityInstances", []):
                ident = e.get("__identifier")
                px = e.get("px", [0, 0])
                if ident == "PlayerSpawn":
                    has_player = True
                if not (0 <= px[0] <= pxW and 0 <= px[1] <= pxH):
                    out_of_bounds.append(f"{ident}@{px}")
        elif lay.get("__type") in ("Tiles", "IntGrid"):
            tiles = lay.get("gridTiles", []) or lay.get("autoLayerTiles", [])
            grid_csv = lay.get("intGridCsv", [])
            if not tiles and not any(grid_csv):
                issues.append(f"layer {lay.get('__identifier')} vacío")
    if not has_player:
        issues.append("falta PlayerSpawn — el player aparecerá en 0,0")
    if out_of_bounds:
        issues.append(f"{len(out_of_bounds)} entities fuera de bounds: {out_of_bounds[:5]}")
    if not issues:
        return f"{name}: OK"
    return f"{name}: {len(issues)} issues\n" + "\n".join(f"- {i}" for i in issues)


@mcp.tool()
def count_entities(name: str) -> str:
    """Conteo por tipo de entity en una arena."""
    d, err = _load(name)
    if err:
        return err
    counts = {}
    for lvl in _levels(d):
        for lay in lvl.get("layerInstances", []):
            if lay.get("__type") == "Entities":
                for e in lay.get("entityInstances", []):
                    k = e.get("__identifier", "?")
                    counts[k] = counts.get(k, 0) + 1
    if not counts:
        return f"{name}: 0 entities"
    return f"{name}: " + ", ".join(f"{k}={v}" for k, v in sorted(counts.items()))


@mcp.tool()
def list_entity_defs(name: str) -> str:
    """Tipos de entity declarados en defs (todos los tipos posibles)."""
    d, err = _load(name)
    if err:
        return err
    defs = d.get("defs", {}).get("entities", [])
    if not defs:
        return f"{name}: sin entity defs"
    lines = []
    for e in defs:
        w = e.get("width", "?")
        h = e.get("height", "?")
        lines.append(f"- {e.get('identifier')} {w}x{h}")
    return "\n".join(lines)


if __name__ == "__main__":
    mcp.run()
