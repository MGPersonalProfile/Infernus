import os
import math
from PIL import Image, ImageDraw

OUTPUT_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 
                          '..', 'assets', 'sprites', 'player')

# Enhanced Palettes for Dark Souls aesthetic
W_PALETTE = {
    'outline': (15, 10, 10, 255),
    'dark_red': (110, 0, 0, 255), 'blood_red': (180, 20, 20, 255),
    'steel_gray': (100, 105, 110, 255), 'dark_gray': (40, 45, 50, 255),
    'sword_edge': (220, 220, 230, 255), 'sword_dark': (80, 80, 90, 255),
    'horn_color': (200, 180, 150, 255), 'horn_tip': (80, 50, 30, 255),
    'eye_glow': (255, 60, 20, 255), 'skin': (150, 100, 80, 255)
}

R_PALETTE = {
    'outline': (10, 8, 15, 255),
    'deep_purple': (60, 20, 80, 255), 'dark_purple': (40, 10, 60, 255),
    'shadow': (20, 15, 30, 255), 'black': (15, 15, 20, 255),
    'skin': (170, 125, 95, 255), 'skin_shadow': (130, 90, 65, 255),
    'dagger': (190, 200, 210, 255), 'dagger_dark': (90, 100, 110, 255),
    'eye_glow': (180, 100, 255, 255), 'cape_edge': (90, 40, 120, 255)
}

K_PALETTE = {
    'outline': (10, 12, 16, 255),
    'gold': (230, 180, 40, 255), 'dark_gold': (140, 90, 10, 255),
    'silver': (200, 210, 220, 255), 'dark_silver': (100, 110, 130, 255),
    'midnight': (20, 30, 50, 255), 'eye_glow': (80, 200, 255, 255),
    'plume': (200, 30, 40, 255), 'plume_dark': (100, 10, 20, 255),
    'lance': (180, 180, 190, 255), 'wood': (70, 40, 20, 255)
}

def fill_rect(draw, x1, y1, x2, y2, color):
    for y in range(int(y1), int(y2)+1):
        for x in range(int(x1), int(x2)+1):
            draw.point((x, y), fill=color)

def get_anim_offsets(anim_type, t):
    if anim_type == 'idle':
        b = math.sin(t * math.pi * 2) * 1.5
        return (b, b*0.8, 0, b, 0, b, 0, 0, 0, 0, 0, b*1.2)
    elif anim_type == 'run':
        stride = math.sin(t * math.pi * 2)
        b = abs(math.cos(t * math.pi * 2)) * 3
        la_x = stride * -3
        ra_x = stride * 3
        la_y = -abs(stride)*2
        ra_y = -abs(stride)*2
        ll_y = b if stride > 0 else 0
        rl_y = b if stride < 0 else 0
        ll_f = stride * 4
        rl_f = -stride * 4
        return (b*0.5+1, b*0.5, la_x, la_y+b, ra_x, ra_y+b, ll_y, ll_f, rl_y, rl_f, ra_x, ra_y+b)
    elif anim_type == 'attack':
        if t < 0.3:
            w = t/0.3
            return (-1, -1, -2, -2, 2, -3, 0, 0, 0, 0, 5*w, -6*w)
        elif t < 0.6:
            s = (t-0.3)/0.3
            return (2, 1, 3, 2, -4, 4, 0, 1, 0, -1, 5 - 18*s, -6 + 18*s)
        else:
            r = (t-0.6)/0.4
            return (1-r, 1-r, 3-3*r, 2-2*r, -4+4*r, 4-4*r, 0, 1-r, 0, -1+r, -13+13*r, 12-12*r)
    return (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)

def draw_warrior(draw, ox, t, anim_type):
    hy, ty, lax, lay, rax, ray, lly, llf, rly, rlf, wx, wy = get_anim_offsets(anim_type, t)
    p = W_PALETTE
    
    # Weapon (Sword) behind body
    fill_rect(draw, ox+35+wx, 2+wy, ox+38+wx, 44+wy, p['outline']) # outline
    fill_rect(draw, ox+36+wx, 4+wy, ox+37+wx, 38+wy, p['sword_edge']) 
    fill_rect(draw, ox+37+wx, 8+wy, ox+37+wx, 38+wy, p['sword_dark']) 
    fill_rect(draw, ox+31+wx, 40+wy, ox+41+wx, 42+wy, p['outline']) # guard outline
    fill_rect(draw, ox+32+wx, 41+wy, ox+40+wx, 41+wy, p['dark_gray']) # guard
    fill_rect(draw, ox+36+wx, 43+wy, ox+37+wx, 48+wy, p['horn_color']) # grip
    
    # Left Leg (Back leg)
    fill_rect(draw, ox+16+llf, 37+lly, ox+21+llf, 53, p['outline'])
    fill_rect(draw, ox+17+llf, 38+lly, ox+20+llf, 52, p['dark_gray'])
    fill_rect(draw, ox+18+llf, 38+lly, ox+19+llf, 48, p['dark_red'])
    
    # Right Leg (Front leg)
    fill_rect(draw, ox+24+rlf, 37+rly, ox+29+rlf, 53, p['outline'])
    fill_rect(draw, ox+25+rlf, 38+rly, ox+28+rlf, 52, p['dark_gray'])
    fill_rect(draw, ox+26+rlf, 38+rly, ox+27+rlf, 48, p['dark_red'])
    
    # Torso
    fill_rect(draw, ox+13, 18+ty, ox+32, 38+ty, p['outline'])
    fill_rect(draw, ox+14, 19+ty, ox+31, 37+ty, p['dark_red'])
    fill_rect(draw, ox+16, 22+ty, ox+29, 30+ty, p['blood_red']) # core bright
    fill_rect(draw, ox+15, 34+ty, ox+30, 37+ty, p['dark_gray']) # belt
    
    # Left Arm
    fill_rect(draw, ox+10+lax, 20+lay, ox+14+lax, 39+lay, p['outline'])
    fill_rect(draw, ox+11+lax, 21+lay, ox+13+lax, 33+lay, p['dark_red'])
    fill_rect(draw, ox+11+lax, 34+lay, ox+13+lax, 38+lay, p['steel_gray'])
    
    # Right Arm
    fill_rect(draw, ox+31+rax, 20+ray, ox+35+rax, 39+ray, p['outline'])
    fill_rect(draw, ox+32+rax, 21+ray, ox+34+rax, 33+ray, p['blood_red'])
    fill_rect(draw, ox+32+rax, 34+ray, ox+34+rax, 38+ray, p['steel_gray'])
    
    # Head & Helmet
    fill_rect(draw, ox+16, 7+hy, ox+29, 18+hy, p['outline'])
    fill_rect(draw, ox+17, 8+hy, ox+28, 17+hy, p['steel_gray'])
    fill_rect(draw, ox+18, 9+hy, ox+24, 11+hy, p['dark_gray']) 
    # Eye slit
    fill_rect(draw, ox+20, 12+hy, ox+26, 12+hy, p['outline'])
    draw.point((ox+21, 12+int(hy)), fill=p['eye_glow']) 
    draw.point((ox+25, 12+int(hy)), fill=p['eye_glow']) 
    
    # Epic Horns
    fill_rect(draw, ox+13, 1+hy, ox+16, 8+hy, p['outline'])
    fill_rect(draw, ox+14, 2+hy, ox+15, 8+hy, p['horn_color'])
    fill_rect(draw, ox+14, 2+hy, ox+14, 5+hy, p['horn_tip'])
    
    fill_rect(draw, ox+29, 1+hy, ox+32, 8+hy, p['outline'])
    fill_rect(draw, ox+30, 2+hy, ox+31, 8+hy, p['horn_color'])
    fill_rect(draw, ox+31, 2+hy, ox+31, 5+hy, p['horn_tip'])

def draw_rogue(draw, ox, t, anim_type):
    hy, ty, lax, lay, rax, ray, lly, llf, rly, rlf, wx, wy = get_anim_offsets(anim_type, t)
    p = R_PALETTE
    
    # Fluttering Cape
    cw = math.sin(t * math.pi * 4) * 2
    fill_rect(draw, ox+10-wx*0.2, 19+ty, ox+34-wx*0.2, 49+ty, p['outline'])
    fill_rect(draw, ox+11-wx*0.2, 20+ty, ox+33-wx*0.2, 48+ty, p['dark_purple'])
    fill_rect(draw, ox+11+cw-wx*0.2, 20+ty, ox+15+cw-wx*0.2, 48+ty, p['cape_edge'])
    
    # Legs (slimmer)
    fill_rect(draw, ox+17+llf, 36+lly, ox+22+llf, 53, p['outline'])
    fill_rect(draw, ox+18+llf, 37+lly, ox+21+llf, 52, p['shadow'])
    fill_rect(draw, ox+19+llf, 38+lly, ox+20+llf, 48, p['dark_purple'])
    
    fill_rect(draw, ox+25+rlf, 36+rly, ox+30+rlf, 53, p['outline'])
    fill_rect(draw, ox+26+rlf, 37+rly, ox+29+rlf, 52, p['shadow'])
    fill_rect(draw, ox+27+rlf, 38+rly, ox+28+rlf, 48, p['dark_purple'])
    
    # Torso
    fill_rect(draw, ox+15, 17+ty, ox+32, 36+ty, p['outline'])
    fill_rect(draw, ox+16, 18+ty, ox+31, 35+ty, p['deep_purple'])
    fill_rect(draw, ox+18, 20+ty, ox+26, 28+ty, p['dark_purple'])
    fill_rect(draw, ox+16, 34+ty, ox+31, 35+ty, p['shadow']) # belt
    
    # Left Arm & Dagger
    fill_rect(draw, ox+10+lax+wx*0.5, 27+lay+wy*0.5, ox+13+lax+wx*0.5, 36+lay+wy*0.5, p['outline'])
    fill_rect(draw, ox+11+lax+wx*0.5, 28+lay+wy*0.5, ox+12+lax+wx*0.5, 35+lay+wy*0.5, p['dagger_dark']) # left dagger
    fill_rect(draw, ox+12+lax, 20+lay, ox+16+lax, 31+lay, p['outline'])
    fill_rect(draw, ox+13+lax, 21+lay, ox+15+lax, 30+lay, p['dark_purple'])
    
    # Right Arm & Dagger
    fill_rect(draw, ox+32+rax, 19+ray, ox+35+rax, 31+ray, p['outline'])
    fill_rect(draw, ox+33+rax, 20+ray, ox+34+rax, 30+ray, p['deep_purple'])
    fill_rect(draw, ox+34+rax+wx*0.5, 27+ray+wy*0.5, ox+37+rax+wx*0.5, 36+ray+wy*0.5, p['outline'])
    fill_rect(draw, ox+35+rax+wx*0.5, 28+ray+wy*0.5, ox+36+rax+wx*0.5, 35+ray+wy*0.5, p['dagger']) # right dagger
    
    # Hood & Head
    fill_rect(draw, ox+17, 5+hy, ox+30, 17+hy, p['outline'])
    fill_rect(draw, ox+18, 6+hy, ox+29, 16+hy, p['shadow'])
    fill_rect(draw, ox+19, 7+hy, ox+28, 12+hy, p['dark_purple'])
    
    # Eyes
    draw.point((ox+21, 12+int(hy)), fill=p['eye_glow'])
    draw.point((ox+26, 12+int(hy)), fill=p['eye_glow'])

def draw_knight(draw, ox, t, anim_type):
    hy, ty, lax, lay, rax, ray, lly, llf, rly, rlf, wx, wy = get_anim_offsets(anim_type, t)
    p = K_PALETTE
    
    # Lance (back)
    fill_rect(draw, ox+36+wx, 1+wy, ox+39+wx, 51+wy, p['outline'])
    fill_rect(draw, ox+37+wx, 4+wy, ox+38+wx, 50+wy, p['wood'])
    fill_rect(draw, ox+36+wx, 2+wy, ox+39+wx, 4+wy, p['lance'])
    fill_rect(draw, ox+37+wx, 1+wy, ox+38+wx, 2+wy, p['lance']) # tip point
    
    # Legs (bulkier)
    fill_rect(draw, ox+17+llf, 37+lly, ox+22+llf, 53, p['outline'])
    fill_rect(draw, ox+18+llf, 38+lly, ox+21+llf, 52, p['dark_silver'])
    fill_rect(draw, ox+19+llf, 38+lly, ox+20+llf, 50, p['silver'])
    
    fill_rect(draw, ox+25+rlf, 37+rly, ox+30+rlf, 53, p['outline'])
    fill_rect(draw, ox+26+rlf, 38+rly, ox+29+rlf, 52, p['dark_silver'])
    fill_rect(draw, ox+27+rlf, 38+rly, ox+28+rlf, 50, p['silver'])
    
    # Torso (breastplate)
    fill_rect(draw, ox+14, 19+ty, ox+33, 37+ty, p['outline'])
    fill_rect(draw, ox+15, 20+ty, ox+32, 36+ty, p['dark_silver'])
    fill_rect(draw, ox+16, 21+ty, ox+30, 31+ty, p['silver'])
    fill_rect(draw, ox+15, 35+ty, ox+32, 36+ty, p['gold']) # belt
    # Emblem
    fill_rect(draw, ox+21, 24+ty, ox+26, 28+ty, p['midnight'])
    fill_rect(draw, ox+22, 25+ty, ox+25, 27+ty, p['gold'])
    
    # Left Arm & Shield
    fill_rect(draw, ox+4+lax, 21+lay, ox+15+lax, 39+lay, p['outline'])
    fill_rect(draw, ox+5+lax, 22+lay, ox+14+lax, 38+lay, p['midnight'])
    fill_rect(draw, ox+6+lax, 23+lay, ox+13+lax, 37+lay, p['silver'])
    fill_rect(draw, ox+7+lax, 24+lay, ox+12+lax, 30+lay, p['dark_silver'])
    
    # Right Arm
    fill_rect(draw, ox+32+rax, 20+ray, ox+36+rax, 39+ray, p['outline'])
    fill_rect(draw, ox+33+rax, 21+ray, ox+35+rax, 38+ray, p['dark_silver'])
    fill_rect(draw, ox+33+rax, 22+ray, ox+34+rax, 33+ray, p['silver'])
    fill_rect(draw, ox+33+rax, 34+ray, ox+35+rax, 38+ray, p['gold']) # gauntlet
    
    # Head & Helmet
    fill_rect(draw, ox+17, 7+hy, ox+31, 18+hy, p['outline'])
    fill_rect(draw, ox+18, 8+hy, ox+30, 17+hy, p['dark_silver'])
    fill_rect(draw, ox+19, 9+hy, ox+28, 14+hy, p['silver'])
    
    # Red Plume
    fill_rect(draw, ox+18, 0+hy, ox+29, 8+hy, p['outline'])
    fill_rect(draw, ox+20, 1+hy, ox+28, 7+hy, p['plume'])
    fill_rect(draw, ox+21, 2+hy, ox+27, 6+hy, p['plume_dark'])
    
    # Visor glow
    fill_rect(draw, ox+20, 12+hy, ox+27, 13+hy, p['midnight'])
    draw.point((ox+21, 13+int(hy)), fill=p['eye_glow'])
    draw.point((ox+26, 13+int(hy)), fill=p['eye_glow'])

def generate_player_anim(char_name, anim_name, frames, w, h, draw_fn):
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
    generate_player_anim('warrior', 'idle', 6, 48, 56, draw_warrior)
    generate_player_anim('warrior', 'run', 8, 48, 56, draw_warrior)
    generate_player_anim('warrior', 'attack', 6, 48, 56, draw_warrior)
    
    generate_player_anim('rogue', 'idle', 6, 40, 52, lambda d, ox, t, a: draw_rogue(d, ox-4, t, a))
    generate_player_anim('rogue', 'run', 8, 40, 52, lambda d, ox, t, a: draw_rogue(d, ox-4, t, a))
    generate_player_anim('rogue', 'attack', 6, 40, 52, lambda d, ox, t, a: draw_rogue(d, ox-4, t, a))
    
    generate_player_anim('knight', 'idle', 6, 52, 60, lambda d, ox, t, a: draw_knight(d, ox+2, t, a))
    generate_player_anim('knight', 'run', 8, 52, 60, lambda d, ox, t, a: draw_knight(d, ox+2, t, a))
    generate_player_anim('knight', 'attack', 6, 52, 60, lambda d, ox, t, a: draw_knight(d, ox+2, t, a))
