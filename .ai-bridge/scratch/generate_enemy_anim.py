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
    # Returns (head_y, body_y, arms_x, arms_y, legs_y, legs_flex)
    if anim_type == 'idle':
        b = math.sin(t * math.pi * 2) * 2.0
        return (b, b*0.8, 0, b, 0, 0)
    elif anim_type == 'run':
        stride = math.sin(t * math.pi * 2)
        b = abs(math.cos(t * math.pi * 2)) * 3
        return (b*0.5, b*0.5, -stride*2, b-abs(stride)*2, b, stride*3)
    elif anim_type == 'attack':
        if t < 0.3: # windup
            return (-1, -1, -3, -1, 0, 0)
        elif t < 0.6: # strike
            return (2, 1, +6, +3, 0, 1)
        else: # recovery
            return (0, 0, 0, 0, 0, 0)
    return (0, 0, 0, 0, 0, 0)

# Colors
C_MELEE = {'red':(200,50,50,255), 'dark':(120,20,20,255), 'black':(20,20,20,255), 'glow':(255,100,100,255), 'claw':(180,180,180,255)}
def draw_melee(draw, ox, t, anim_type):
    hy, by, ax, ay, ly, lf = get_anim_offsets(anim_type, t)
    # L-Arm
    fill_rect(draw, ox+6-ax, 15+ay, ox+7-ax, 24+ay, C_MELEE['dark'])
    fill_rect(draw, ox+5-ax, 25+ay, ox+7-ax, 26+ay, C_MELEE['claw'])
    # R-Arm
    fill_rect(draw, ox+24+ax, 15+ay, ox+25+ax, 24+ay, C_MELEE['dark'])
    fill_rect(draw, ox+23+ax, 25+ay, ox+25+ax, 26+ay, C_MELEE['claw'])
    
    # Body
    for y in range(15 + int(by), 45 + int(by)):
        w = max(2, int(10 * (1.0 - (y - 15) / 30.0)) + 2)
        fill_rect(draw, ox+16-w, y, ox+16+w, y, C_MELEE['red'] if y%3==0 else C_MELEE['dark'])
        draw.point((ox+16-w, y), fill=C_MELEE['black'])
        draw.point((ox+16+w, y), fill=C_MELEE['black'])
    
    # Head
    fill_rect(draw, ox+10, 4+hy, ox+21, 12+hy, C_MELEE['dark'])
    fill_rect(draw, ox+12, 6+hy, ox+19, 11+hy, C_MELEE['black']) # face
    draw.point((ox+13, 8+int(hy)), fill=C_MELEE['glow']); draw.point((ox+14, 8+int(hy)), fill=C_MELEE['glow'])
    draw.point((ox+17, 8+int(hy)), fill=C_MELEE['glow']); draw.point((ox+18, 8+int(hy)), fill=C_MELEE['glow'])

C_RANGED = {'bone':(220,220,210,255), 'dark_bone':(150,150,140,255), 'orange':(255,120,30,255), 'brown':(90,50,30,255), 'dark_brown':(50,30,15,255)}
def draw_ranged(draw, ox, t, anim_type):
    hy, by, ax, ay, ly, lf = get_anim_offsets(anim_type, t)
    
    # Horse lower body
    fill_rect(draw, ox+4, 15+by, ox+23, 25+by, C_RANGED['brown'])
    # Legs (4 legs)
    fill_rect(draw, ox+6+lf, 25+ly, ox+6+lf, 37, C_RANGED['dark_bone']) # FL
    fill_rect(draw, ox+10-lf, 25+ly, ox+10-lf, 37, C_RANGED['bone']) # FR
    fill_rect(draw, ox+18+lf, 25+ly, ox+18+lf, 37, C_RANGED['dark_bone']) # BL
    fill_rect(draw, ox+22-lf, 25+ly, ox+22-lf, 37, C_RANGED['bone']) # BR
    for hx in (6+lf, 10-lf, 18+lf, 22-lf): fill_rect(draw, ox+hx, 38, ox+hx+1, 38, C_RANGED['dark_brown'])
    
    # Humanoid Torso
    fill_rect(draw, ox+10, 5+by, ox+15, 14+by, C_RANGED['bone'])
    
    # Head
    fill_rect(draw, ox+10, 0+hy, ox+17, 5+hy, C_RANGED['bone'])
    draw.point((ox+12, 3+int(hy)), fill=C_RANGED['orange']); draw.point((ox+15, 3+int(hy)), fill=C_RANGED['orange'])
    
    # Bow arms
    fill_rect(draw, ox+8, 5+ay, ox+8, 12+ay, C_RANGED['dark_bone'])
    fill_rect(draw, ox+18+ax, 0+ay, ox+18+ax, 20+ay, C_RANGED['brown']) # bow stick

C_TANK = {'skin': (100, 100, 120, 255), 'dark': (60, 60, 80, 255), 'stone':(80,80,80,255), 'red_eye':(180,20,20,255), 'horn':(150,130,100,255)}
def draw_tank(draw, ox, t, anim_type):
    hy, by, ax, ay, ly, lf = get_anim_offsets(anim_type, t)
    
    # Legs
    fill_rect(draw, ox+15+lf, 40+ly, ox+24+lf, 59, C_TANK['dark'])
    fill_rect(draw, ox+31-lf, 40+ly, ox+40-lf, 59, C_TANK['skin'])
    
    # Torso
    fill_rect(draw, ox+18, 15+by, ox+38, 39+by, C_TANK['skin'])
    
    # Fists
    fill_rect(draw, ox+2-ax, 30+ay, ox+17-ax, 45+ay, C_TANK['stone'])
    fill_rect(draw, ox+38+ax, 30+ay, ox+53+ax, 45+ay, C_TANK['stone'])
    
    # Head
    fill_rect(draw, ox+20, 5+hy, ox+35, 15+hy, C_TANK['skin'])
    draw.point((ox+25, 10+int(hy)), fill=C_TANK['red_eye']); draw.point((ox+30, 10+int(hy)), fill=C_TANK['red_eye'])
    # Horns
    fill_rect(draw, ox+15, 0+hy, ox+19, 4+hy, C_TANK['horn'])
    fill_rect(draw, ox+36, 0+hy, ox+40, 4+hy, C_TANK['horn'])

C_ASSASSIN = {'purple':(80,40,120,255), 'dark':(40,20,60,255), 'black':(15,15,15,255), 'glow':(200,150,255,255), 'blade':(150,150,180,255)}
def draw_assassin(draw, ox, t, anim_type):
    hy, by, ax, ay, ly, lf = get_anim_offsets(anim_type, t)
    
    # Ghostly trail
    for y in range(25 + int(by), 38 + int(by)):
        w = max(1, 5 - (y - 25)//3)
        fill_rect(draw, ox+12-w, y, ox+12+w, y, C_ASSASSIN['dark'])
        
    # Torso
    fill_rect(draw, ox+8, 10+by, ox+16, 24+by, C_ASSASSIN['purple'])
    
    # Head
    fill_rect(draw, ox+8, 2+hy, ox+15, 9+hy, C_ASSASSIN['purple'])
    fill_rect(draw, ox+10, 5+hy, ox+13, 8+hy, C_ASSASSIN['black'])
    draw.point((ox+10, 6+int(hy)), fill=C_ASSASSIN['glow']); draw.point((ox+13, 6+int(hy)), fill=C_ASSASSIN['glow'])
    
    # Blades
    if anim_type == 'attack' and t > 0.1 and t < 0.6:
        # slash in front
        fill_rect(draw, ox+4, 15, ox+20, 18, C_ASSASSIN['blade'])
    else:
        # Blades backward
        fill_rect(draw, ox+4+ax, 15+ay, ox+5+ax, 24+ay, C_ASSASSIN['blade'])
        fill_rect(draw, ox+18-ax, 15+ay, ox+19-ax, 24+ay, C_ASSASSIN['blade'])

C_BOMBER = {'lava':(255,120,0,255), 'red':(150,20,20,255), 'black':(20,20,20,255)}
def draw_bomber(draw, ox, t, anim_type):
    # Bomber constantly pulsates based on t
    hy, by, ax, ay, ly, lf = get_anim_offsets(anim_type, t)
    pulse = math.sin(t * math.pi * 2 * (4 if anim_type=='run' else 2)) * 2
    
    # Legs
    fill_rect(draw, ox+10+lf, 26, ox+13+lf, 30, C_BOMBER['black'])
    fill_rect(draw, ox+18-lf, 26, ox+21-lf, 30, C_BOMBER['black'])
    
    # Bloated body
    rad = 12 + int(pulse)
    if anim_type == 'attack': rad += int(t * 10) # swells before attack
    fill_rect(draw, ox+16-rad, 8-pulse, ox+16+rad, 25+pulse, C_BOMBER['red'])
    # Veins
    fill_rect(draw, ox+10-rad//2, 12, ox+12-rad//2, 20, C_BOMBER['lava'])
    fill_rect(draw, ox+20+rad//2, 12, ox+22+rad//2, 20, C_BOMBER['lava'])
    
    # Face
    fill_rect(draw, ox+12, 2+hy, ox+19, 8+hy, C_BOMBER['red'])
    draw.point((ox+14, 6+int(hy)), fill=C_BOMBER['lava'])
    draw.point((ox+17, 5+int(hy)), fill=C_BOMBER['lava'])

def generate_enemy_anim(char_name, anim_name, frames, w, h, draw_fn):
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
    # 4. melee (32x48)
    generate_enemy_anim('melee', 'idle', 4, 32, 48, draw_melee)
    generate_enemy_anim('melee', 'run', 6, 32, 48, draw_melee)
    generate_enemy_anim('melee', 'attack', 4, 32, 48, draw_melee)
    
    # 5. ranged (28x40)
    generate_enemy_anim('ranged', 'idle', 4, 28, 40, draw_ranged)
    generate_enemy_anim('ranged', 'run', 6, 28, 40, draw_ranged)
    generate_enemy_anim('ranged', 'attack', 4, 28, 40, draw_ranged)
    
    # 6. tank (56x64)
    generate_enemy_anim('tank', 'idle', 4, 56, 64, draw_tank)
    generate_enemy_anim('tank', 'run', 6, 56, 64, draw_tank)
    generate_enemy_anim('tank', 'attack', 4, 56, 64, draw_tank)
    
    # 7. assassin (24x40)
    generate_enemy_anim('assassin', 'idle', 4, 24, 40, draw_assassin)
    generate_enemy_anim('assassin', 'run', 6, 24, 40, draw_assassin)
    generate_enemy_anim('assassin', 'attack', 4, 24, 40, draw_assassin)
    
    # 8. bomber (32x32)
    generate_enemy_anim('bomber', 'idle', 4, 32, 32, draw_bomber)
    generate_enemy_anim('bomber', 'run', 6, 32, 32, draw_bomber)
    generate_enemy_anim('bomber', 'attack', 4, 32, 32, draw_bomber)
