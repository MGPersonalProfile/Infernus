import os
import math
from PIL import Image, ImageDraw

OUTPUT_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 
                          '..', 'assets', 'sprites', 'enemies')

def fill_rect(draw, x1, y1, x2, y2, color):
    for y in range(int(y1), int(y2)+1):
        for x in range(int(x1), int(x2)+1):
            draw.point((x, y), fill=color)

def get_anim_offsets(anim_type, t):
    if anim_type == 'idle':
        b = math.sin(t * math.pi * 2) * 3.0
        return (b, b*0.5, 0, b*0.5, 0, 0)
    elif anim_type == 'charge':
        stride = math.sin(t * math.pi * 2)
        b = abs(math.cos(t * math.pi * 2)) * 4
        return (b+4, b+2, -stride*4, b, b, stride*5)
    elif anim_type == 'slam':
        if t < 0.4: # rise up
            return (-8, -6, 0, -10, 0, 0)
        elif t < 0.6: # slam down
            return (8, 6, 0, 15, 0, 3)
        else: # recovery
            return (4, 2, 0, 10, 0, 1)
    return (0, 0, 0, 0, 0, 0)

C_MINOTAUR = {
    'blood': (160, 20, 20, 255),
    'dark_blood': (100, 10, 10, 255),
    'brown': (80, 50, 30, 255),
    'dark_brown': (40, 20, 10, 255),
    'bone': (220, 220, 200, 255),
    'black': (15, 15, 15, 255),
    'glow': (255, 50, 0, 255),
    'stone': (100, 100, 100, 255),
    'dark_stone': (60, 60, 60, 255)
}

def draw_minotaur(draw, ox, t, anim_type):
    hy, by, ax, ay, ly, lf = get_anim_offsets(anim_type, t)
    
    # Shadows/background elements
    if anim_type == 'slam' and t > 0.4 and t < 0.7:
        # Slam impact effect
        draw.ellipse([ox+10, 70, ox+70, 80], outline=C_MINOTAUR['glow'])
        fill_rect(draw, ox+35, 75, ox+45, 80, C_MINOTAUR['glow'])

    # Back Arm (Left)
    fill_rect(draw, ox+10-ax, 25+ay, ox+20-ax, 50+ay, C_MINOTAUR['brown'])
    # Left Fist / Weapon (Axe or stone)
    if anim_type == 'slam':
        # both hands raised/slammed
        wax = ox+40; way = 50+ay
        fill_rect(draw, wax-8, way, wax+8, way+15, C_MINOTAUR['stone'])
    else:
        fill_rect(draw, ox+8-ax, 50+ay, ox+22-ax, 65+ay, C_MINOTAUR['stone'])
    
    # Back Leg (Left)
    l_leg_x = ox+25+lf
    if anim_type == 'charge': l_leg_x = ox+15+lf*2
    fill_rect(draw, l_leg_x, 45+ly, l_leg_x+12, 75, C_MINOTAUR['dark_brown'])
    fill_rect(draw, l_leg_x+2, 72, l_leg_x+10, 78, C_MINOTAUR['black']) # hoof
    
    # Front Leg (Right)
    r_leg_x = ox+45-lf
    if anim_type == 'charge': r_leg_x = ox+55-lf*2
    fill_rect(draw, r_leg_x, 45+ly, r_leg_x+12, 75, C_MINOTAUR['dark_brown'])
    fill_rect(draw, r_leg_x+2, 72, r_leg_x+10, 78, C_MINOTAUR['black']) # hoof
    
    # Torso (Massive)
    for y in range(25 + int(by), 55 + int(by)):
        w = int(22 * max(0, 1.0 - ((y - 25 - by)**2) / 1200.0))
        w = max(10, w)
        fill_rect(draw, ox+40-w, y, ox+40+w, y, C_MINOTAUR['blood'] if (y)%4!=0 else C_MINOTAUR['dark_blood'])
    # Chest detail / muscles
    fill_rect(draw, ox+36, 30+by, ox+44, 45+by, C_MINOTAUR['dark_blood'])
    
    # Front Arm (Right)
    fill_rect(draw, ox+50+ax, 25+ay, ox+65+ax, 50+ay, C_MINOTAUR['brown'])
    if anim_type == 'slam':
        pass # drawn together
    else:
        # Front Fist
        fill_rect(draw, ox+52+ax, 50+ay, ox+66+ax, 65+ay, C_MINOTAUR['stone'])
    
    # Head (Bull head)
    if anim_type == 'charge': hy += 8; ox += 10 # Head lowered, forward
    
    fill_rect(draw, ox+30, 8+hy, ox+50, 26+hy, C_MINOTAUR['dark_brown'])
    fill_rect(draw, ox+34, 20+hy, ox+46, 28+hy, C_MINOTAUR['brown']) # snout
    draw.point((ox+37, 26+int(hy)), fill=C_MINOTAUR['black']) # nostril
    draw.point((ox+43, 26+int(hy)), fill=C_MINOTAUR['black']) # nostril
    
    # Glowing Eyes
    draw.point((ox+35, 14+int(hy)), fill=C_MINOTAUR['glow']); draw.point((ox+36, 14+int(hy)), fill=C_MINOTAUR['glow'])
    draw.point((ox+44, 14+int(hy)), fill=C_MINOTAUR['glow']); draw.point((ox+45, 14+int(hy)), fill=C_MINOTAUR['glow'])
    
    # Horns (Massive)
    for i in range(12):
        # Left horn sweep
        fill_rect(draw, ox+28-i*2, 10+hy-i, ox+31-i*2, 14+hy-i, C_MINOTAUR['bone'])
        # Right horn sweep
        fill_rect(draw, ox+49+i*2, 10+hy-i, ox+52+i*2, 14+hy-i, C_MINOTAUR['bone'])

def generate_boss_anim(char_name, anim_name, frames, w, h, draw_fn):
    sheet_w = w * frames
    img = Image.new('RGBA', (sheet_w, h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    for i in range(frames):
        t = i / frames
        draw_fn(draw, i * w, t, anim_name)
    
    filepath = os.path.join(OUTPUT_DIR, f"{char_name}_{anim_name}.png")
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    img.save(filepath)
    print(f"✅ {char_name}_{anim_name}.png: {sheet_w}x{h} OK")

if __name__ == '__main__':
    # 9. minotaur (80x80)
    generate_boss_anim('minotaur', 'idle', 6, 80, 80, draw_minotaur)
    generate_boss_anim('minotaur', 'charge', 4, 80, 80, draw_minotaur)
    generate_boss_anim('minotaur', 'slam', 6, 80, 80, draw_minotaur)

