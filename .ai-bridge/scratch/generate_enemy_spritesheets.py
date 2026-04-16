"""
Generador de spritesheets idle para los 5 enemigos de INFERNUS.
Cada spritesheet tiene 2 frames horizontales con 1px vertical offset (breathing).
Fondo transparente, estilo Dark Souls/Hades pixel art.
"""

from PIL import Image, ImageDraw
import os

OUTPUT_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 
                          '..', 'assets', 'sprites', 'enemies')

def generate_spritesheet(draw_func, filename, frame_w, frame_h):
    sheet_w = frame_w * 2
    sheet_h = frame_h
    img = Image.new('RGBA', (sheet_w, sheet_h), (0, 0, 0, 0))
    
    # Frame 1 (neutral)
    draw_func(img, 0, frame_w, frame_h, is_frame2=False)
    # Frame 2 (breathing shift)
    draw_func(img, frame_w, frame_w, frame_h, is_frame2=True)
    
    filepath = os.path.join(OUTPUT_DIR, filename)
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    img.save(filepath)
    
    verify = Image.open(filepath)
    assert verify.size == (sheet_w, sheet_h), f"ERROR: {filename} is {verify.size}, expected ({sheet_w}, {sheet_h})"
    print(f"✅ {filename}: {verify.size} OK")

def draw_melee(img, ox, fw, fh, is_frame2=False):
    """Alma Violenta (melee): 32x48. Espectro rojo furioso, garras, ojos brillantes."""
    draw = ImageDraw.Draw(img)
    by = 1 if is_frame2 else 0
    
    red = (200, 50, 50, 255)
    dark_red = (120, 20, 20, 255)
    black = (20, 20, 20, 255)
    dark_gray = (50, 50, 50, 255)
    glow = (255, 100, 100, 255)
    claw = (180, 180, 180, 255)
    
    # Body (specter shape)
    for y in range(15 + by, 45 + by):
        w = int(10 * (1.0 - (y - 15) / 30.0)) + 2 # Tapers down
        w = min(12, max(2, w))
        for x in range(16 - w, 16 + w):
            c = red if (x+y)%3 != 0 else dark_red
            if x in (16-w, 15+w): c = black
            draw.point((ox + x, y), fill=c)
            
    # Shoulders
    for y in range(12 + by, 16 + by):
        for x in range(8, 24):
            draw.point((ox + x, y), fill=dark_red if x%2==0 else red)
            
    # Head
    for y in range(4 + by, 13 + by):
        for x in range(10, 22):
            if 6+by <= y <= 11+by and 12 <= x <= 19:
                draw.point((ox + x, y), fill=black) # face shadow
            else:
                draw.point((ox + x, y), fill=dark_red)
                
    # Glowing eyes
    draw.point((ox + 13, 8 + by), fill=glow)
    draw.point((ox + 14, 8 + by), fill=glow)
    draw.point((ox + 17, 8 + by), fill=glow)
    draw.point((ox + 18, 8 + by), fill=glow)
    
    # Left Arm & Claw
    for y in range(15 + by, 25 + by):
        draw.point((ox + 6, y), fill=dark_red)
        draw.point((ox + 7, y), fill=red)
    draw.point((ox + 6, 25 + by), fill=claw)
    draw.point((ox + 5, 26 + by), fill=claw)
    draw.point((ox + 7, 26 + by), fill=claw)
    
    # Right Arm & Claw
    for y in range(15 + by, 25 + by):
        draw.point((ox + 24, y), fill=dark_red)
        draw.point((ox + 25, y), fill=red)
    draw.point((ox + 24, 25 + by), fill=claw)
    draw.point((ox + 23, 26 + by), fill=claw)
    draw.point((ox + 25, 26 + by), fill=claw)

def draw_ranged(img, ox, fw, fh, is_frame2=False):
    """Centauro (ranged): 28x40. Centauro esquelético con arco. Naranja, hueso, marrón."""
    draw = ImageDraw.Draw(img)
    by = 1 if is_frame2 else 0
    
    bone = (220, 220, 210, 255)
    dark_bone = (150, 150, 140, 255)
    orange = (255, 120, 30, 255)
    brown = (90, 50, 30, 255)
    dark_brown = (50, 30, 15, 255)
    bow_wood = (120, 80, 40, 255)
    string = (200, 200, 200, 255)
    
    # Horse body (lower half)
    for y in range(15 + by, 25 + by):
        for x in range(4, 24):
            if y > 20 + by and (x < 10 or x > 18):
                draw.point((ox + x, y), fill=bone if x%2==0 else dark_bone) # skeletal ribs/legs start
            else:
                draw.point((ox + x, y), fill=brown if (x+y)%4 != 0 else dark_brown)
                
    # Horse Legs
    # Front-left
    for y in range(25 + by, 38 + by): draw.point((ox + 6, y), fill=dark_bone)
    draw.point((ox + 6, 38 + by), fill=dark_brown) # Hoof
    draw.point((ox + 7, 38 + by), fill=dark_brown)
    # Front-right
    for y in range(25 + by, 38 + by): draw.point((ox + 10, y), fill=bone)
    draw.point((ox + 10, 38 + by), fill=dark_brown)
    draw.point((ox + 11, 38 + by), fill=dark_brown)
    # Back-left
    for y in range(25 + by, 38 + by): draw.point((ox + 18, y), fill=dark_bone)
    draw.point((ox + 18, 38 + by), fill=dark_brown)
    draw.point((ox + 19, 38 + by), fill=dark_brown)
    # Back-right
    for y in range(25 + by, 38 + by): draw.point((ox + 22, y), fill=bone)
    draw.point((ox + 22, 38 + by), fill=dark_brown)
    draw.point((ox + 23, 38 + by), fill=dark_brown)
    
    # Humanoid Torso (skeletal)
    for y in range(5 + by, 15 + by):
        for x in range(10, 16):
            if y%2 == 0: draw.point((ox + x, y), fill=bone) # Ribs
            else: draw.point((ox + x, y), fill=dark_bone)
            
    # Skull Head
    for y in range(0 + by, 6 + by):
        for x in range(10, 18):
            draw.point((ox + x, y), fill=bone)
    draw.point((ox + 12, 3 + by), fill=orange) # Eye
    draw.point((ox + 15, 3 + by), fill=orange) # Eye
    
    # Bow & Arms
    for y in range(5 + by, 12 + by):
        draw.point((ox + 8, y), fill=bone) # Arm drawing back
    
    # Bow curve
    for y in range(0 + by, 20 + by):
        bx = 20 if y in (9+by, 10+by, 11+by) else (19 if y in(4+by, 5+by, 6+by, 7+by, 8+by, 12+by, 13+by, 14+by, 15+by, 16+by) else 18)
        if 0+by <= y <= 20+by:
            draw.point((ox + bx, y), fill=bow_wood)
    # Bowstring
    for y in range(0 + by, 20 + by):
        if y < 10+by: draw.point((ox + 18 - (10+by - y)//2, y), fill=string)
        elif y > 10+by: draw.point((ox + 18 - (y - 10-by)//2, y), fill=string)

def draw_tank(img, ox, fw, fh, is_frame2=False):
    """Bruto del Flegetonte (tank): 56x64. Demonio musculoso, gris-azul, cuernos, puños de piedra."""
    draw = ImageDraw.Draw(img)
    by = 1 if is_frame2 else 0
    
    skin = (100, 100, 120, 255)
    dark_skin = (60, 60, 80, 255)
    black = (20, 20, 25, 255)
    red_eye = (180, 20, 20, 255)
    horn = (150, 130, 100, 255)
    stone = (80, 80, 80, 255)
    light_stone = (120, 120, 120, 255)
    
    # Massive Legs
    for y in range(40 + by, 60 + by):
        for x in range(15, 25): draw.point((ox + x, y), fill=dark_skin if x%3==0 else skin) # Left leg
        for x in range(31, 41): draw.point((ox + x, y), fill=dark_skin if x%3==0 else skin) # Right leg
    for x in range(12, 26): draw.point((ox + x, 60 + by), fill=black); draw.point((ox + x, 61 + by), fill=black) # Feet
    for x in range(30, 44): draw.point((ox + x, 60 + by), fill=black); draw.point((ox + x, 61 + by), fill=black)
    
    # Massive Torso
    for y in range(15 + by, 40 + by):
        w = 12 if y < 25+by else (10 if y < 35+by else 8)
        for x in range(28 - w, 28 + w):
            if y%5==0 and abs(28-x)<5: draw.point((ox + x, y), fill=dark_skin) # Abs/muscles
            else: draw.point((ox + x, y), fill=skin if (x+y)%4!=0 else dark_skin)
            
    # Big Head
    for y in range(5 + by, 16 + by):
        for x in range(20, 36):
            if 8+by <= y <= 12+by and 24 <= x <= 32: draw.point((ox + x, y), fill=black) # Face shadow
            else: draw.point((ox + x, y), fill=skin)
    
    # Small red glowing eyes
    draw.point((ox + 25, 10 + by), fill=red_eye); draw.point((ox + 26, 10 + by), fill=red_eye)
    draw.point((ox + 29, 10 + by), fill=red_eye); draw.point((ox + 30, 10 + by), fill=red_eye)
    
    # Horns
    for y in range(0 + by, 8 + by):
        lx = 20 - (8+by - y)
        draw.point((ox + lx, y), fill=horn); draw.point((ox + lx+1, y), fill=horn)
        rx = 35 + (8+by - y)
        draw.point((ox + rx, y), fill=horn); draw.point((ox + rx-1, y), fill=horn)
        
    # Stone Fists & Arms
    for y in range(15 + by, 35 + by):
        for x in range(6, 16): draw.point((ox + x, y), fill=skin) # L-arm
        for x in range(40, 50): draw.point((ox + x, y), fill=skin) # R-arm
        
    for y in range(30 + by, 45 + by):
        for x in range(2, 18):
            draw.point((ox + x, y), fill=light_stone if (x+y)%3==0 else stone) # L-fist
        for x in range(38, 54):
            draw.point((ox + x, y), fill=light_stone if (x+y)%3==0 else stone) # R-fist

def draw_assassin(img, ox, fw, fh, is_frame2=False):
    """Sombra Asesina (assassin): 24x40. Silueta oscura, ojos púrpura, cuchillas. Púrpura/negro."""
    draw = ImageDraw.Draw(img)
    by = 1 if is_frame2 else 0
    
    purple = (80, 40, 120, 255)
    dark_purple = (40, 20, 60, 255)
    black = (15, 15, 15, 255)
    eye_glow = (200, 150, 255, 255)
    blade = (150, 150, 180, 255)
    blade_dark = (80, 80, 100, 255)
    
    # Ghostly trail / legs
    for y in range(25 + by, 38 + by):
        w = max(1, 5 - (y - 25)//3)
        for x in range(12 - w, 12 + w):
            draw.point((ox + x, y), fill=dark_purple if x%2==0 else black)
            
    # Slim Body & Cloak
    for y in range(10 + by, 25 + by):
        w = 6 if y < 15+by else (5 if y < 20+by else 4)
        for x in range(12 - w, 12 + w):
            if x == 12-w or x == 12+w-1: draw.point((ox + x, y), fill=purple)
            else: draw.point((ox + x, y), fill=black)
            
    # Head & Hood
    for y in range(2 + by, 10 + by):
        for x in range(8, 16):
            if 5+by <= y <= 8+by and 10 <= x <= 13: draw.point((ox + x, y), fill=black) # Face
            else: draw.point((ox + x, y), fill=purple if (x+y)%3==0 else dark_purple)
            
    # Purple Eyes
    draw.point((ox + 10, 6 + by), fill=eye_glow); draw.point((ox + 13, 6 + by), fill=eye_glow)
    
    # Curved Blades (held backward or sideways)
    for y in range(15 + by, 25 + by):
        # L-blade
        bx1 = 4 + (y - 15)//2
        draw.point((ox + bx1, y), fill=blade)
        draw.point((ox + bx1+1, y), fill=blade_dark)
        # R-blade
        bx2 = 19 - (y - 15)//2
        draw.point((ox + bx2, y), fill=blade)
        draw.point((ox + bx2-1, y), fill=blade_dark)

def draw_bomber(img, ox, fw, fh, is_frame2=False):
    """Suicida Retorcido (bomber): 32x32. Hinchado, venas de lava. Naranja/rojo/negro."""
    draw = ImageDraw.Draw(img)
    # More prominent breathing for the bomber (pulsating)
    by = 1 if is_frame2 else 0
    w_mod = 1 if is_frame2 else 0
    
    lava = (255, 120, 0, 255)
    dark_lava = (200, 60, 0, 255)
    red = (150, 20, 20, 255)
    black = (20, 20, 20, 255)
    
    # Stubby Legs
    for y in range(26 + by, 30 + by):
        for x in range(10, 14): draw.point((ox + x, y), fill=black) # L-leg
        for x in range(18, 22): draw.point((ox + x, y), fill=black) # R-leg
        
    # Bloated Body
    for y in range(8 - by, 26 + by):
        r = int(10 * max(0, 1 - ((y - 17)**2) / 100.0)**0.5) + w_mod
        r = max(2, min(14, r))
        for x in range(16 - r, 16 + r):
            c = red
            if (x*3 + y*7) % 11 < 3: c = dark_lava # Veins
            elif (x*5 + y*2) % 13 < 4: c = lava # Bright spots
            if x == 16-r or x == 16+r-1: c = black # Outline
            draw.point((ox + x, y), fill=c)
            
    # Small head / Face
    for y in range(2 - by, 10 - by):
        w = 5 if 4 <= y+by <= 7 else 3
        for x in range(16 - w, 16 + w):
            if 5-by <= y <= 7-by and 13 <= x <= 18: draw.point((ox + x, y), fill=black) # Face
            else: draw.point((ox + x, y), fill=red)
            
    # Crazy Eyes
    draw.point((ox + 14, 6 - by), fill=lava)
    draw.point((ox + 17, 5 - by), fill=lava) # Asymmetric

if __name__ == '__main__':
    print("=== Generando spritesheets de enemigos para INFERNUS ===")
    generate_spritesheet(draw_melee, 'melee_idle.png', 32, 48)
    generate_spritesheet(draw_ranged, 'ranged_idle.png', 28, 40)
    generate_spritesheet(draw_tank, 'tank_idle.png', 56, 64)
    generate_spritesheet(draw_assassin, 'assassin_idle.png', 24, 40)
    generate_spritesheet(draw_bomber, 'bomber_idle.png', 32, 32)
    print("=== ¡Completado! ===")
