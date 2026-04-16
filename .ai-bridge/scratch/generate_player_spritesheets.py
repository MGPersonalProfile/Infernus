"""
Generador de spritesheets idle para las 3 clases jugables de INFERNUS.
Cada spritesheet: 2 frames horizontales, 48x56 px cada frame = 96x56 total.
Pixel art estilo Dark Souls/Hades. Fondo transparente.
Temática: Séptimo Círculo del Infierno de Dante.
"""

from PIL import Image, ImageDraw
import os

OUTPUT_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 
                          '..', 'assets', 'sprites', 'player')

FRAME_W = 48
FRAME_H = 56
SHEET_W = FRAME_W * 2  # 96
SHEET_H = FRAME_H      # 56


def draw_warrior(img, frame_offset_x, is_frame2=False):
    """Guerrero: armadura pesada roja/gris, espada grande, casco con cuernos."""
    draw = ImageDraw.Draw(img)
    ox = frame_offset_x
    
    # Paleta
    dark_red = (139, 0, 0, 255)
    blood_red = (165, 22, 22, 255)
    steel_gray = (85, 85, 85, 255)
    dark_gray = (55, 55, 55, 255)
    dark_steel = (70, 70, 70, 255)
    black = (20, 20, 20, 255)
    skin = (180, 130, 100, 255)
    highlight = (110, 95, 85, 255)
    sword_edge = (160, 160, 170, 255)
    sword_dark = (100, 100, 110, 255)
    horn_color = (120, 80, 40, 255)
    horn_tip = (90, 55, 25, 255)
    eye_glow = (255, 80, 30, 255)
    
    # Breathing offset for frame 2
    by = 1 if is_frame2 else 0
    
    # === SWORD (behind body, right side) ===
    # Blade
    for y in range(8 + by, 42 + by):
        draw.point((ox + 35, y), fill=sword_dark)
        draw.point((ox + 36, y), fill=sword_edge)
        draw.point((ox + 37, y), fill=sword_dark)
    # Sword tip
    draw.point((ox + 36, 7 + by), fill=sword_edge)
    draw.point((ox + 36, 6 + by), fill=steel_gray)
    # Crossguard
    for x in range(32, 42):
        draw.point((ox + x, 42 + by), fill=dark_gray)
        draw.point((ox + x, 43 + by), fill=steel_gray)
    # Grip
    for y in range(44 + by, 50 + by):
        draw.point((ox + 36, y), fill=horn_color)
        draw.point((ox + 37, y), fill=horn_color)
    # Pommel
    draw.point((ox + 36, 50 + by), fill=dark_red)
    draw.point((ox + 37, 50 + by), fill=dark_red)
    
    # === HELMET WITH HORNS ===
    # Left horn
    draw.point((ox + 16, 5 + by), fill=horn_tip)
    draw.point((ox + 17, 6 + by), fill=horn_color)
    draw.point((ox + 17, 7 + by), fill=horn_color)
    draw.point((ox + 18, 8 + by), fill=horn_color)
    # Right horn
    draw.point((ox + 29, 5 + by), fill=horn_tip)
    draw.point((ox + 28, 6 + by), fill=horn_color)
    draw.point((ox + 28, 7 + by), fill=horn_color)
    draw.point((ox + 27, 8 + by), fill=horn_color)
    
    # Helmet top
    for x in range(19, 27):
        draw.point((ox + x, 8 + by), fill=dark_gray)
    for x in range(18, 28):
        draw.point((ox + x, 9 + by), fill=steel_gray)
        draw.point((ox + x, 10 + by), fill=steel_gray)
    
    # Helmet face area
    for y in range(11 + by, 16 + by):
        for x in range(17, 29):
            if x in (17, 28):
                draw.point((ox + x, y), fill=dark_gray)
            else:
                draw.point((ox + x, y), fill=steel_gray)
    
    # Eye slit
    draw.point((ox + 20, 12 + by), fill=eye_glow)
    draw.point((ox + 21, 12 + by), fill=eye_glow)
    draw.point((ox + 25, 12 + by), fill=eye_glow)
    draw.point((ox + 26, 12 + by), fill=eye_glow)
    
    # Visor lines
    for x in range(19, 27):
        draw.point((ox + x, 14 + by), fill=dark_gray)
    
    # Chin guard
    for x in range(18, 28):
        draw.point((ox + x, 16 + by), fill=dark_steel)
    for x in range(19, 27):
        draw.point((ox + x, 17 + by), fill=dark_steel)
    
    # === BODY / ARMOR ===
    # Neck
    for x in range(20, 26):
        draw.point((ox + x, 18 + by), fill=dark_gray)
    
    # Shoulders (wide, heavy armor)
    for x in range(11, 35):
        draw.point((ox + x, 19 + by), fill=dark_red if 14 < x < 31 else steel_gray)
        draw.point((ox + x, 20 + by), fill=dark_red if 13 < x < 32 else steel_gray)
    
    # Pauldrons
    for y in range(19 + by, 25 + by):
        for x in range(11, 16):
            draw.point((ox + x, y), fill=steel_gray if (x + y) % 2 == 0 else dark_steel)
        for x in range(30, 35):
            draw.point((ox + x, y), fill=steel_gray if (x + y) % 2 == 0 else dark_steel)
    
    # Torso
    for y in range(21 + by, 36 + by):
        width_shrink = max(0, (y - 30 - by)) // 2
        for x in range(14 + width_shrink, 32 - width_shrink):
            if y in (24 + by, 28 + by, 32 + by):
                draw.point((ox + x, y), fill=dark_gray)  # armor lines
            elif x == 22 or x == 23:
                draw.point((ox + x, y), fill=blood_red)  # center detail
            else:
                draw.point((ox + x, y), fill=dark_red)
    
    # Belt
    for x in range(15, 31):
        draw.point((ox + x, 36 + by), fill=dark_gray)
        draw.point((ox + x, 37 + by), fill=steel_gray)
    # Belt buckle
    draw.point((ox + 22, 36 + by), fill=horn_color)
    draw.point((ox + 23, 36 + by), fill=horn_color)
    draw.point((ox + 22, 37 + by), fill=horn_color)
    draw.point((ox + 23, 37 + by), fill=horn_color)
    
    # === ARMS ===
    # Left arm
    for y in range(21 + by, 34 + by):
        draw.point((ox + 12, y), fill=dark_red)
        draw.point((ox + 13, y), fill=dark_red)
    # Left gauntlet
    for y in range(34 + by, 38 + by):
        draw.point((ox + 11, y), fill=steel_gray)
        draw.point((ox + 12, y), fill=dark_steel)
        draw.point((ox + 13, y), fill=steel_gray)
    
    # Right arm (holding sword)
    for y in range(21 + by, 34 + by):
        draw.point((ox + 32, y), fill=dark_red)
        draw.point((ox + 33, y), fill=dark_red)
    # Right gauntlet
    for y in range(34 + by, 38 + by):
        draw.point((ox + 32, y), fill=steel_gray)
        draw.point((ox + 33, y), fill=dark_steel)
        draw.point((ox + 34, y), fill=steel_gray)
    
    # === LEGS ===
    # Left leg
    for y in range(38 + by, 50 + by):
        draw.point((ox + 17, y), fill=dark_gray)
        draw.point((ox + 18, y), fill=dark_red)
        draw.point((ox + 19, y), fill=dark_red)
        draw.point((ox + 20, y), fill=dark_gray)
    # Right leg
    for y in range(38 + by, 50 + by):
        draw.point((ox + 25, y), fill=dark_gray)
        draw.point((ox + 26, y), fill=dark_red)
        draw.point((ox + 27, y), fill=dark_red)
        draw.point((ox + 28, y), fill=dark_gray)
    
    # Knee guards
    for x in range(17, 21):
        draw.point((ox + x, 43 + by), fill=steel_gray)
    for x in range(25, 29):
        draw.point((ox + x, 43 + by), fill=steel_gray)
    
    # === BOOTS ===
    for x in range(16, 22):
        draw.point((ox + x, 50 + by), fill=dark_gray)
        draw.point((ox + x, 51 + by), fill=dark_gray)
        draw.point((ox + x, 52 + by), fill=black)
    for x in range(24, 30):
        draw.point((ox + x, 50 + by), fill=dark_gray)
        draw.point((ox + x, 51 + by), fill=dark_gray)
        draw.point((ox + x, 52 + by), fill=black)


def draw_rogue(img, frame_offset_x, is_frame2=False):
    """Pícaro: capucha, capa oscura, dagas gemelas, ágil y esbelto."""
    draw = ImageDraw.Draw(img)
    ox = frame_offset_x
    
    # Paleta
    deep_purple = (75, 0, 130, 255)
    dark_purple = (55, 0, 100, 255)
    shadow = (35, 25, 50, 255)
    black = (20, 18, 25, 255)
    dark_gray = (50, 45, 55, 255)
    mid_gray = (80, 75, 85, 255)
    skin = (170, 125, 95, 255)
    skin_shadow = (130, 90, 65, 255)
    dagger_blade = (180, 185, 195, 255)
    dagger_dark = (120, 125, 135, 255)
    dagger_handle = (90, 50, 30, 255)
    eye_glow = (200, 160, 255, 255)
    cape_edge = (45, 10, 80, 255)
    
    by = 1 if is_frame2 else 0
    
    # === HOOD ===
    # Hood top peak
    draw.point((ox + 23, 6 + by), fill=dark_purple)
    draw.point((ox + 24, 6 + by), fill=dark_purple)
    
    for x in range(21, 27):
        draw.point((ox + x, 7 + by), fill=dark_purple)
    for x in range(19, 29):
        draw.point((ox + x, 8 + by), fill=deep_purple)
    for x in range(18, 30):
        draw.point((ox + x, 9 + by), fill=deep_purple)
        draw.point((ox + x, 10 + by), fill=deep_purple)
    
    # Hood sides drape
    for y in range(11 + by, 17 + by):
        draw.point((ox + 17, y), fill=dark_purple)
        draw.point((ox + 18, y), fill=deep_purple)
        draw.point((ox + 29, y), fill=deep_purple)
        draw.point((ox + 30, y), fill=dark_purple)
    
    # Face in shadow
    for y in range(11 + by, 16 + by):
        for x in range(19, 29):
            draw.point((ox + x, y), fill=shadow)
    
    # Glowing eyes
    draw.point((ox + 21, 12 + by), fill=eye_glow)
    draw.point((ox + 22, 12 + by), fill=eye_glow)
    draw.point((ox + 26, 12 + by), fill=eye_glow)
    draw.point((ox + 27, 12 + by), fill=eye_glow)
    
    # Lower face / mouth shadow
    for x in range(20, 28):
        draw.point((ox + x, 15 + by), fill=skin_shadow)
    for x in range(21, 27):
        draw.point((ox + x, 16 + by), fill=skin_shadow)
    
    # === NECK ===
    for x in range(21, 27):
        draw.point((ox + x, 17 + by), fill=shadow)
    
    # === CAPE (behind body) ===
    # Cape drapes from shoulders
    for y in range(20 + by, 48 + by):
        cape_width = min(6, (y - 20 - by) // 3)
        # Left cape edge
        cx = 13 - cape_width
        for x in range(cx, 15):
            draw.point((ox + x, y), fill=dark_purple if (x + y) % 3 == 0 else shadow)
        # Right cape edge  
        cx2 = 33 + cape_width
        for x in range(33, min(cx2+1, 44)):
            draw.point((ox + x, y), fill=dark_purple if (x + y) % 3 == 0 else shadow)
    
    # Cape bottom
    for x in range(8, 40):
        if 8 <= x <= 15 or 33 <= x <= 39:
            draw.point((ox + x, 48 + by), fill=cape_edge)
    
    # === TORSO (slim) ===
    for y in range(18 + by, 20 + by):
        for x in range(17, 31):
            draw.point((ox + x, y), fill=deep_purple)
    
    for y in range(20 + by, 35 + by):
        for x in range(16, 32):
            if x in (16, 31):
                draw.point((ox + x, y), fill=shadow)
            elif x == 23 or x == 24:
                draw.point((ox + x, y), fill=black)  # center line
            else:
                draw.point((ox + x, y), fill=deep_purple if y % 2 == 0 else dark_purple)
    
    # Belt / sash
    for x in range(16, 32):
        draw.point((ox + x, 35 + by), fill=dark_gray)
    for x in range(17, 31):
        draw.point((ox + x, 36 + by), fill=mid_gray)
    
    # === ARMS (thin) ===
    # Left arm
    for y in range(20 + by, 33 + by):
        draw.point((ox + 14, y), fill=deep_purple)
        draw.point((ox + 15, y), fill=dark_purple)
    # Left hand
    draw.point((ox + 13, 33 + by), fill=skin_shadow)
    draw.point((ox + 14, 33 + by), fill=skin)
    draw.point((ox + 15, 33 + by), fill=skin_shadow)
    
    # Right arm
    for y in range(20 + by, 33 + by):
        draw.point((ox + 32, y), fill=dark_purple)
        draw.point((ox + 33, y), fill=deep_purple)
    # Right hand
    draw.point((ox + 32, 33 + by), fill=skin_shadow)
    draw.point((ox + 33, 33 + by), fill=skin)
    draw.point((ox + 34, 33 + by), fill=skin_shadow)
    
    # === DAGGERS ===
    # Left dagger
    for y in range(27 + by, 33 + by):
        draw.point((ox + 11, y), fill=dagger_dark)
        draw.point((ox + 12, y), fill=dagger_blade)
    draw.point((ox + 11, 26 + by), fill=dagger_blade)
    draw.point((ox + 12, 25 + by), fill=dagger_blade)
    # handle
    draw.point((ox + 11, 34 + by), fill=dagger_handle)
    draw.point((ox + 12, 34 + by), fill=dagger_handle)
    draw.point((ox + 11, 35 + by), fill=dagger_handle)
    draw.point((ox + 12, 35 + by), fill=dagger_handle)
    
    # Right dagger
    for y in range(27 + by, 33 + by):
        draw.point((ox + 35, y), fill=dagger_blade)
        draw.point((ox + 36, y), fill=dagger_dark)
    draw.point((ox + 35, 26 + by), fill=dagger_blade)
    draw.point((ox + 36, 25 + by), fill=dagger_blade)
    # handle
    draw.point((ox + 35, 34 + by), fill=dagger_handle)
    draw.point((ox + 36, 34 + by), fill=dagger_handle)
    draw.point((ox + 35, 35 + by), fill=dagger_handle)
    draw.point((ox + 36, 35 + by), fill=dagger_handle)
    
    # === LEGS (slim) ===
    # Left leg
    for y in range(37 + by, 50 + by):
        draw.point((ox + 18, y), fill=shadow)
        draw.point((ox + 19, y), fill=dark_purple)
        draw.point((ox + 20, y), fill=deep_purple)
        draw.point((ox + 21, y), fill=shadow)
    
    # Right leg
    for y in range(37 + by, 50 + by):
        draw.point((ox + 26, y), fill=shadow)
        draw.point((ox + 27, y), fill=deep_purple)
        draw.point((ox + 28, y), fill=dark_purple)
        draw.point((ox + 29, y), fill=shadow)
    
    # === BOOTS ===
    for x in range(17, 23):
        draw.point((ox + x, 50 + by), fill=dark_gray)
        draw.point((ox + x, 51 + by), fill=black)
        draw.point((ox + x, 52 + by), fill=black)
    for x in range(25, 31):
        draw.point((ox + x, 50 + by), fill=dark_gray)
        draw.point((ox + x, 51 + by), fill=black)
        draw.point((ox + x, 52 + by), fill=black)


def draw_knight(img, frame_offset_x, is_frame2=False):
    """Caballero: armadura completa dorada/plateada, escudo grande, lanza."""
    draw = ImageDraw.Draw(img)
    ox = frame_offset_x
    
    # Paleta
    gold = (218, 165, 32, 255)
    dark_gold = (170, 120, 20, 255)
    silver = (192, 192, 192, 255)
    dark_silver = (140, 140, 150, 255)
    midnight_blue = (25, 25, 112, 255)
    dark_blue = (15, 15, 80, 255)
    black = (20, 20, 25, 255)
    steel = (160, 165, 175, 255)
    eye_glow = (100, 150, 255, 255)
    plume_color = (180, 30, 30, 255)
    plume_dark = (130, 20, 20, 255)
    wood = (100, 65, 30, 255)
    lance_tip = (200, 200, 210, 255)
    shield_emblem = (255, 200, 50, 255)
    
    by = 1 if is_frame2 else 0
    
    # === LANCE (behind, right side) ===
    for y in range(2 + by, 50 + by):
        draw.point((ox + 37, y), fill=wood if y > 20 + by else lance_tip)
        draw.point((ox + 38, y), fill=wood if y > 20 + by else dark_silver)
    # Lance head
    draw.point((ox + 37, 1 + by), fill=lance_tip)
    draw.point((ox + 36, 3 + by), fill=steel)
    draw.point((ox + 39, 3 + by), fill=steel)
    
    # === HELMET (full face, knightly) ===
    # Plume on top
    for x in range(22, 28):
        draw.point((ox + x, 5 + by), fill=plume_color)
    for x in range(21, 29):
        draw.point((ox + x, 6 + by), fill=plume_color if x % 2 == 0 else plume_dark)
    for x in range(22, 28):
        draw.point((ox + x, 7 + by), fill=plume_dark)
    
    # Helmet dome
    for x in range(19, 29):
        draw.point((ox + x, 8 + by), fill=silver)
    for x in range(18, 30):
        draw.point((ox + x, 9 + by), fill=silver)
        draw.point((ox + x, 10 + by), fill=dark_silver)
    
    # Helmet face
    for y in range(11 + by, 16 + by):
        for x in range(17, 31):
            if x in (17, 30):
                draw.point((ox + x, y), fill=dark_silver)
            elif y == 13 + by and 19 <= x <= 28:
                draw.point((ox + x, y), fill=dark_blue)  # visor slit
            else:
                draw.point((ox + x, y), fill=silver)
    
    # Eye glow through visor
    draw.point((ox + 21, 13 + by), fill=eye_glow)
    draw.point((ox + 22, 13 + by), fill=eye_glow)
    draw.point((ox + 26, 13 + by), fill=eye_glow)
    draw.point((ox + 27, 13 + by), fill=eye_glow)
    
    # Chin guard
    for x in range(18, 30):
        draw.point((ox + x, 16 + by), fill=dark_silver)
    for x in range(20, 28):
        draw.point((ox + x, 17 + by), fill=dark_silver)
    
    # Gold trim on helmet
    for x in range(18, 30):
        draw.point((ox + x, 11 + by), fill=gold)
    
    # === GORGET / NECK ===
    for x in range(20, 28):
        draw.point((ox + x, 18 + by), fill=dark_gold)
    
    # === SHOULDERS (heavy, decorated) ===
    for x in range(10, 36):
        if x < 16 or x > 30:
            draw.point((ox + x, 19 + by), fill=gold)
            draw.point((ox + x, 20 + by), fill=dark_gold)
        else:
            draw.point((ox + x, 19 + by), fill=silver)
            draw.point((ox + x, 20 + by), fill=dark_silver)
    
    # Pauldrons with gold trim
    for y in range(19 + by, 26 + by):
        for x in range(10, 16):
            c = gold if y == 19 + by or x == 10 else (silver if (x+y) % 2 == 0 else dark_silver)
            draw.point((ox + x, y), fill=c)
        for x in range(31, 37):
            c = gold if y == 19 + by or x == 36 else (silver if (x+y) % 2 == 0 else dark_silver)
            draw.point((ox + x, y), fill=c)
    
    # === TORSO (heavy plate) ===
    for y in range(21 + by, 36 + by):
        for x in range(15, 33):
            if y in (24 + by, 28 + by, 32 + by):
                draw.point((ox + x, y), fill=dark_gold)  # gold trim lines
            elif x == 23 or x == 24:
                draw.point((ox + x, y), fill=midnight_blue)  # center stripe
            elif x in (15, 32):
                draw.point((ox + x, y), fill=dark_silver)
            else:
                draw.point((ox + x, y), fill=silver)
    
    # Chest emblem (cross/star)
    draw.point((ox + 23, 25 + by), fill=shield_emblem)
    draw.point((ox + 24, 25 + by), fill=shield_emblem)
    draw.point((ox + 22, 26 + by), fill=shield_emblem)
    draw.point((ox + 23, 26 + by), fill=gold)
    draw.point((ox + 24, 26 + by), fill=gold)
    draw.point((ox + 25, 26 + by), fill=shield_emblem)
    draw.point((ox + 23, 27 + by), fill=shield_emblem)
    draw.point((ox + 24, 27 + by), fill=shield_emblem)
    
    # Fauld / waist
    for x in range(15, 33):
        draw.point((ox + x, 36 + by), fill=gold)
        draw.point((ox + x, 37 + by), fill=dark_gold)
    
    # === SHIELD (left arm, large) ===
    for y in range(22 + by, 38 + by):
        for x in range(5, 14):
            if y == 22 + by or y == 37 + by or x == 5 or x == 13:
                draw.point((ox + x, y), fill=gold)  # border
            elif y == 30 + by or x == 9:
                draw.point((ox + x, y), fill=dark_gold)  # cross
            else:
                draw.point((ox + x, y), fill=midnight_blue)  # field
    
    # === RIGHT ARM ===
    for y in range(21 + by, 34 + by):
        draw.point((ox + 33, y), fill=silver)
        draw.point((ox + 34, y), fill=dark_silver)
    # Gauntlet
    for y in range(34 + by, 38 + by):
        draw.point((ox + 33, y), fill=gold)
        draw.point((ox + 34, y), fill=dark_gold)
        draw.point((ox + 35, y), fill=gold)
    
    # === LEGS ===
    # Left leg
    for y in range(38 + by, 50 + by):
        draw.point((ox + 18, y), fill=dark_silver)
        draw.point((ox + 19, y), fill=silver)
        draw.point((ox + 20, y), fill=silver)
        draw.point((ox + 21, y), fill=dark_silver)
    
    # Right leg
    for y in range(38 + by, 50 + by):
        draw.point((ox + 26, y), fill=dark_silver)
        draw.point((ox + 27, y), fill=silver)
        draw.point((ox + 28, y), fill=silver)
        draw.point((ox + 29, y), fill=dark_silver)
    
    # Knee guards (gold)
    for x in range(17, 22):
        draw.point((ox + x, 43 + by), fill=gold)
    for x in range(25, 30):
        draw.point((ox + x, 43 + by), fill=gold)
    
    # Greaves trim
    for x in range(18, 22):
        draw.point((ox + x, 47 + by), fill=dark_gold)
    for x in range(26, 30):
        draw.point((ox + x, 47 + by), fill=dark_gold)
    
    # === BOOTS (sabatons) ===
    for x in range(16, 23):
        draw.point((ox + x, 50 + by), fill=dark_silver)
        draw.point((ox + x, 51 + by), fill=silver)
        draw.point((ox + x, 52 + by), fill=dark_silver)
    for x in range(24, 31):
        draw.point((ox + x, 50 + by), fill=dark_silver)
        draw.point((ox + x, 51 + by), fill=silver)
        draw.point((ox + x, 52 + by), fill=dark_silver)


def generate_spritesheet(draw_func, filename):
    """Create a 96x56 spritesheet with 2 idle frames."""
    img = Image.new('RGBA', (SHEET_W, SHEET_H), (0, 0, 0, 0))
    
    # Frame 1 (neutral)
    draw_func(img, 0, is_frame2=False)
    # Frame 2 (slight breathing shift)
    draw_func(img, FRAME_W, is_frame2=True)
    
    filepath = os.path.join(OUTPUT_DIR, filename)
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    img.save(filepath)
    
    # Verify dimensions
    verify = Image.open(filepath)
    assert verify.size == (SHEET_W, SHEET_H), f"ERROR: {filename} is {verify.size}, expected ({SHEET_W}, {SHEET_H})"
    print(f"✅ {filename}: {verify.size} — OK")


if __name__ == '__main__':
    print("=== Generando spritesheets idle para INFERNUS ===")
    print(f"Output: {OUTPUT_DIR}")
    print(f"Frame size: {FRAME_W}x{FRAME_H}, Sheet size: {SHEET_W}x{SHEET_H}")
    print()
    
    generate_spritesheet(draw_warrior, 'warrior_idle.png')
    generate_spritesheet(draw_rogue, 'rogue_idle.png')
    generate_spritesheet(draw_knight, 'knight_idle.png')
    
    print()
    print("=== ¡Generación completada! ===")
