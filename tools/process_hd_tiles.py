"""
INFERNUS -- Fase 4B v2: Procesamiento mejorado de tilesets HD
=============================================================
Mejoras sobre v1:
  - Floor: priorizar tiles del centro de la imagen (menos vignette)
  - Decor: no forzar 64x64. Usar dimensiones apropiadas para cada prop.
  - Wall: mas detalle en las grietas de lava

Autor: Antigravity
"""

from PIL import Image, ImageDraw
import os
import random

TILE_SIZE = 64
TILES_DIR = "assets/sprites/tiles"

random.seed(42)


def extract_floor_tiles():
    """Recorta floor_hd.png priorizando tiles centrales con buen detalle."""
    print("=" * 60)
    print("PROCESANDO floor_hd.png (v2 - centro priorizado)...")
    print("=" * 60)

    src = Image.open(os.path.join(TILES_DIR, "floor_hd.png"))
    w, h = src.size
    cols = w // TILE_SIZE  # 16
    rows = h // TILE_SIZE  # 16
    center_col = cols // 2
    center_row = rows // 2
    print(f"  Fuente: {w}x{h} -> {cols}x{rows} grid. Centro: ({center_col},{center_row})")

    # Extraer tiles con score basado en:
    # 1. Distancia al centro (mas cerca = mejor, menos vignette)
    # 2. Brillo promedio (no demasiado oscuro)
    # 3. Variancia de color (mas detalle = mejor)
    scored = []
    for row in range(rows):
        for col in range(cols):
            x = col * TILE_SIZE
            y = row * TILE_SIZE
            tile = src.crop((x, y, x + TILE_SIZE, y + TILE_SIZE))
            pixels = list(tile.getdata())

            # Brillo promedio
            brightness = sum(sum(p[:3]) / 3 for p in pixels) / len(pixels)

            # Variancia
            r_avg = sum(p[0] for p in pixels) / len(pixels)
            g_avg = sum(p[1] for p in pixels) / len(pixels)
            b_avg = sum(p[2] for p in pixels) / len(pixels)
            variance = sum(
                (p[0] - r_avg)**2 + (p[1] - g_avg)**2 + (p[2] - b_avg)**2
                for p in pixels
            ) / (len(pixels) * 3)

            # Distancia al centro (penalizacion)
            dist = ((col - center_col)**2 + (row - center_row)**2)**0.5
            dist_penalty = dist * 30  # Los bordes se penalizan fuerte

            # Score final
            score = variance + brightness * 2 - dist_penalty

            if brightness >= 30:  # Filtrar tiles negros
                scored.append((score, brightness, variance, tile, col, row))

    scored.sort(key=lambda x: x[0], reverse=True)

    # Tomar top 8
    selected = scored[:8]
    print(f"  Top 8 tiles seleccionados:")
    for i, (sc, br, var, _, col, row) in enumerate(selected):
        print(f"    [{i+1}] col={col}, row={row}, brightness={br:.1f}, variance={var:.1f}, score={sc:.1f}")

    # Guardar el mejor como floor.png principal
    best = selected[0][3].convert("RGBA")
    best.save(os.path.join(TILES_DIR, "floor.png"))
    print(f"  >> floor.png = tile ({selected[0][4]},{selected[0][5]})")

    # Guardar variaciones
    for i, (_, _, _, tile, col, row) in enumerate(selected[:6]):
        tile.convert("RGBA").save(os.path.join(TILES_DIR, f"floor_var{i+1}.png"))

    # Strip tileset
    strip = Image.new("RGBA", (TILE_SIZE * 6, TILE_SIZE), (0, 0, 0, 0))
    for i, (_, _, _, tile, _, _) in enumerate(selected[:6]):
        strip.paste(tile.convert("RGBA"), (i * TILE_SIZE, 0))
    strip.save(os.path.join(TILES_DIR, "floor_tileset.png"))
    print(f"  >> floor_tileset.png (6 variaciones)")


def extract_decor_props():
    """Extrae props de decor_hd.png con dimensiones apropiadas (no forzar 64x64)."""
    print("\n" + "=" * 60)
    print("PROCESANDO decor_hd.png (v2 - dimensiones proporcionales)...")
    print("=" * 60)

    src = Image.open(os.path.join(TILES_DIR, "decor_hd.png")).convert("RGBA")
    w, h = src.size
    pixels = src.load()

    # Hacer negro = transparente
    for x in range(w):
        for y in range(h):
            r, g, b, a = pixels[x, y]
            if r < 12 and g < 12 and b < 12:
                pixels[x, y] = (0, 0, 0, 0)

    # Las decoraciones del juego actual son:
    #   decor_crack.png, decor_blood.png, decor_bones.png, decor_rune.png
    # Todos de 64x64, se usan como overlays sobre floor tiles.
    #
    # Los props del HD son DIFERENTES: son objetos grandes (pilar, altar, tumba, antorcha)
    # que deberian ser mas como el pillar.png existente (32x48).
    #
    # Estrategia: extraer a tamano proporcional para uso como decoracion 3D,
    # no como overlay de tile.

    prop_configs = [
        ("decor_pillar_hd", 0, 256, 32, 64),      # Pilar: 32x64 (delgado y alto)
        ("decor_altar_hd", 256, 512, 64, 48),       # Altar: 64x48 (ancho y bajo)
        ("decor_tombstone_hd", 512, 768, 32, 56),   # Tumba: 32x56 (mediana)
        ("decor_torch_wall_hd", 768, 1024, 24, 48), # Antorcha: 24x48 (estrecha y alta)
    ]

    hd_dir = os.path.join(TILES_DIR, "hd_originals")
    os.makedirs(hd_dir, exist_ok=True)

    for name, x_start, x_end, target_w, target_h in prop_configs:
        # Buscar bounding box del contenido
        min_x, min_y = x_end, h
        max_x, max_y = x_start, 0

        for x in range(x_start, x_end):
            for y in range(h):
                _, _, _, a = pixels[x, y]
                if a > 20:
                    min_x = min(min_x, x)
                    max_x = max(max_x, x)
                    min_y = min(min_y, y)
                    max_y = max(max_y, y)

        if max_x <= min_x:
            print(f"  ! No content found for {name}")
            continue

        prop = src.crop((min_x, min_y, max_x + 1, max_y + 1))
        prop_w, prop_h = prop.size
        print(f"  {name}: original {prop_w}x{prop_h} -> target {target_w}x{target_h}")

        # Guardar original HD
        prop.save(os.path.join(hd_dir, f"{name}.png"))

        # Redimensionar al tamano objetivo
        resized = prop.resize((target_w, target_h), Image.NEAREST)
        resized.save(os.path.join(TILES_DIR, f"{name}.png"))
        print(f"  >> {name}.png ({target_w}x{target_h})")

    # Tambien crear versiones de 64x64 como overlays de suelo
    # (sangre, marcas, runas) extraidas del floor HD en zonas con grietas rojas
    print("\n  Generando overlays de suelo mejorados...")
    floor_src = Image.open(os.path.join(TILES_DIR, "floor_hd.png")).convert("RGBA")

    # Buscar tiles con mucho rojo (grietas de lava) para usar como overlays
    overlay_candidates = []
    for row in range(16):
        for col in range(16):
            x = col * TILE_SIZE
            y = row * TILE_SIZE
            tile = floor_src.crop((x, y, x + TILE_SIZE, y + TILE_SIZE))
            pxs = list(tile.getdata())
            # Contar pixeles rojos (lava)
            red_count = sum(1 for p in pxs if p[0] > 80 and p[1] < 40 and p[2] < 40)
            if red_count > 50:  # Al menos 50 pixeles rojos
                overlay_candidates.append((red_count, tile, col, row))

    overlay_candidates.sort(key=lambda x: x[0], reverse=True)

    if overlay_candidates:
        # Tomar el tile con mas lava como decor_lava_crack
        best_lava = overlay_candidates[0][1].convert("RGBA")
        # Hacer semi-transparente las partes oscuras para que funcione como overlay
        lava_pixels = best_lava.load()
        for x in range(64):
            for y in range(64):
                r, g, b, a = lava_pixels[x, y]
                # Solo mantener las grietas rojas, hacer el resto transparente
                if r > 60 and r > g * 2 and r > b * 2:
                    lava_pixels[x, y] = (r, g, b, 230)  # Casi opaco
                else:
                    lava_pixels[x, y] = (0, 0, 0, 0)  # Transparente
        best_lava.save(os.path.join(TILES_DIR, "decor_lava_crack.png"))
        print(f"  >> decor_lava_crack.png (overlay de grietas de lava)")


def generate_wall_tile():
    """Genera un wall.png mejorado v2: mas piedra, mas detalle."""
    print("\n" + "=" * 60)
    print("GENERANDO wall.png mejorado v2...")
    print("=" * 60)

    img = Image.new("RGBA", (TILE_SIZE, TILE_SIZE), (0, 0, 0, 255))
    draw = ImageDraw.Draw(img)

    # Paleta infernal
    MORTAR = (25, 10, 8, 255)
    MORTAR_LAVA = (55, 18, 10, 255)

    # Fondo mortero
    draw.rectangle([(0, 0), (63, 63)], fill=MORTAR)

    # Patron de ladrillos con offset (brick bond)
    bricks = [
        # row 0 (y=0-13)
        [(0, 0, 20, 13), (22, 0, 42, 13), (44, 0, 63, 13)],
        # row 1 offset (y=15-27)
        [(0, 15, 10, 27), (12, 15, 33, 27), (35, 15, 55, 27), (57, 15, 63, 27)],
        # row 2 (y=29-41)
        [(0, 29, 18, 41), (20, 29, 44, 41), (46, 29, 63, 41)],
        # row 3 offset (y=43-55)
        [(0, 43, 13, 55), (15, 43, 36, 55), (38, 43, 58, 55), (60, 43, 63, 55)],
        # row 4 partial (y=57-63)
        [(0, 57, 22, 63), (24, 57, 45, 63), (47, 57, 63, 63)],
    ]

    px = img.load()

    for row in bricks:
        for x1, y1, x2, y2 in row:
            # Color base del ladrillo con variacion
            base_r = 42 + random.randint(-6, 6)
            base_g = 35 + random.randint(-5, 5)
            base_b = 32 + random.randint(-4, 4)

            for y in range(y1, min(y2 + 1, 64)):
                for x in range(x1, min(x2 + 1, 64)):
                    # Textura de piedra: ruido sutil
                    noise = random.randint(-4, 4)
                    r = max(0, min(255, base_r + noise))
                    g = max(0, min(255, base_g + noise))
                    b = max(0, min(255, base_b + noise))

                    # Highlight superior (2px)
                    if y == y1 or y == y1 + 1:
                        r = min(255, r + 12 - (y - y1) * 4)
                        g = min(255, g + 10 - (y - y1) * 3)
                        b = min(255, b + 8 - (y - y1) * 2)

                    # Sombra inferior (2px)
                    if y == y2 or y == y2 - 1:
                        r = max(0, r - 10 + (y2 - y) * 4)
                        g = max(0, g - 8 + (y2 - y) * 3)
                        b = max(0, b - 6 + (y2 - y) * 2)

                    # Highlight izquierdo (1px)
                    if x == x1:
                        r = min(255, r + 6)
                        g = min(255, g + 5)
                        b = min(255, b + 4)

                    # Sombra derecha (1px)
                    if x == x2:
                        r = max(0, r - 8)
                        g = max(0, g - 6)
                        b = max(0, b - 5)

                    px[x, y] = (r, g, b, 255)

    # Agregar grietas de lava en el mortero
    for _ in range(4):
        start_x = random.randint(3, 60)
        start_y = random.randint(3, 60)
        # Buscar una posicion de mortero (entre ladrillos)
        for attempt in range(20):
            test_y = random.choice([14, 28, 42, 56])  # Lineas de mortero horizontal
            test_x = random.randint(5, 58)
            r, g, b, a = px[test_x, test_y]
            if r < 35:  # Es mortero
                # Dibujar grieta de lava
                length = random.randint(4, 12)
                cx = test_x
                for step in range(length):
                    if 0 <= cx < 64 and 0 <= test_y < 64:
                        intensity = 1.0 - (step / length) * 0.5
                        lr = int(140 * intensity)
                        lg = int(45 * intensity)
                        lb = int(12 * intensity)
                        px[cx, test_y] = (lr, lg, lb, 255)

                        # Glow arriba y abajo
                        for dy in [-1, 1]:
                            gy = test_y + dy
                            if 0 <= gy < 64:
                                or_, og, ob, oa = px[cx, gy]
                                px[cx, gy] = (
                                    min(255, or_ + int(25 * intensity)),
                                    min(255, og + int(8 * intensity)),
                                    ob, oa
                                )

                    cx += random.choice([-1, 0, 0, 1])  # Drift horizontal
                break

    # Guardar
    img.save(os.path.join(TILES_DIR, "wall.png"))
    print(f"  >> wall.png (ladrillos infernales con grietas de lava)")

    # Variacion 2: mas roja/caliente
    img2 = img.copy()
    px2 = img2.load()
    for x in range(64):
        for y in range(64):
            r, g, b, a = px2[x, y]
            r = min(255, int(r * 1.08))
            g = max(0, int(g * 0.93))
            b = max(0, int(b * 0.88))
            px2[x, y] = (r, g, b, a)
    img2.save(os.path.join(TILES_DIR, "wall_var2.png"))

    # Variacion 3: mas oscura (profundidades)
    img3 = img.copy()
    px3 = img3.load()
    for x in range(64):
        for y in range(64):
            r, g, b, a = px3[x, y]
            r = max(0, int(r * 0.75))
            g = max(0, int(g * 0.72))
            b = max(0, int(b * 0.70))
            px3[x, y] = (r, g, b, a)
    img3.save(os.path.join(TILES_DIR, "wall_var3.png"))
    print(f"  >> wall_var2.png, wall_var3.png (variaciones)")


def summary():
    """Resumen final."""
    print("\n" + "=" * 60)
    print("RESUMEN DE ENTREGABLES")
    print("=" * 60)

    files = [
        "floor.png",
        "floor_var1.png", "floor_var2.png", "floor_var3.png",
        "floor_var4.png", "floor_var5.png", "floor_var6.png",
        "floor_tileset.png",
        "wall.png", "wall_var2.png", "wall_var3.png",
        "decor_pillar_hd.png", "decor_altar_hd.png",
        "decor_tombstone_hd.png", "decor_torch_wall_hd.png",
        "decor_lava_crack.png",
    ]

    ok = 0
    for f in files:
        path = os.path.join(TILES_DIR, f)
        if os.path.exists(path):
            img = Image.open(path)
            size = os.path.getsize(path)
            print(f"  OK  {f:30s} {img.size[0]:4d}x{img.size[1]:<4d}  ({size:,} bytes)")
            ok += 1
        else:
            print(f"  FALTA  {f}")

    print(f"\n  {ok}/{len(files)} archivos generados correctamente")


if __name__ == "__main__":
    print("INFERNUS -- Fase 4B v2: Tilesets HD\n")
    extract_floor_tiles()
    extract_decor_props()
    generate_wall_tile()
    summary()
    print("\nFase 4B completada.")
