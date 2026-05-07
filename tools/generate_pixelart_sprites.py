import os
from PIL import Image, ImageDraw

def create_transparent(w, h):
    return Image.new("RGBA", (w, h), (0,0,0,0))

def draw_rect(draw, x, y, w, h, fill, outline=None):
    draw.rectangle([x, y, x+w-1, y+h-1], fill=fill, outline=outline)

def build_knight(state, frame):
    img = create_transparent(32, 48)
    d = ImageDraw.Draw(img)
    # Colores
    OUTLINE = (10, 10, 10, 255)
    ARMOR = (160, 160, 160, 255)
    GOLD = (218, 165, 32, 255)
    CAPE = (139, 0, 0, 255)
    SWORD = (200, 200, 200, 255)
    SHIELD = (30, 58, 95, 255)
    
    # Base params
    basex = 16
    basey = 22
    
    # Anim offsets
    cape_offset = 0
    torso_offset = 0
    arm_offset_x = 0
    arm_offset_y = 0
    leg_offset_l = 0
    leg_offset_r = 0
    sword_rot = 0
    
    if state == "idle":
        if frame in [1,2,3]: torso_offset = -1
        if frame in [4,5]: torso_offset = 0
        if frame in [1,3,5]: cape_offset = 1
    elif state == "run":
        torso_offset = 1
        cycles = [
            (-2, 2, -1, 1), (-1, 1, 0, 0), (0, 0, 1, -1), (1, -1, 2, -2),
            (2, -2, 1, -1), (1, -1, 0, 0), (0, 0, -1, 1), (-1, 1, -2, 2)
        ]
        leg_offset_l, leg_offset_r, arm_offset_x, cape_offset = cycles[frame % 8]
    elif state == "attack":
        swings = [
            (0, -3, -15), (2, -4, -30), (5, 0, -80), 
            (7, 5, -120), (4, 0, -45), (1, -1, 0)
        ]
        arm_offset_x, arm_offset_y, sword_rot = swings[frame % 6]
        if frame in [2,3]: torso_offset = 2
    
    # Draw cape
    draw_rect(d, basex - 8 + cape_offset, basey + 2, 4, 20 + torso_offset, CAPE, OUTLINE)
    
    # Draw legs
    # Left leg
    draw_rect(d, basex - 5 + leg_offset_l, basey + 14, 4, 10, ARMOR, OUTLINE)
    # Right leg 
    draw_rect(d, basex + 1 + leg_offset_r, basey + 14, 4, 10, ARMOR, OUTLINE)
    
    # Draw torso
    draw_rect(d, basex - 6, basey - 8 + torso_offset, 12, 16, ARMOR, OUTLINE)
    draw_rect(d, basex - 2, basey - 4 + torso_offset, 4, 8, GOLD, None) # cross
    
    # Draw head
    draw_rect(d, basex - 4, basey - 14 + torso_offset, 8, 6, ARMOR, OUTLINE)
    draw_rect(d, basex + 1, basey - 12 + torso_offset, 3, 1, OUTLINE, None) # visor
    
    # Draw shield (Left arm)
    draw_rect(d, basex - 8 - arm_offset_x, basey - 2 + torso_offset, 4, 8, SHIELD, GOLD)

    # Draw sword (Right arm logic simplified as rects since no arbitrary rot without bigger canvas)
    # We will simulate angle by positions
    sx = basex + 8 + arm_offset_x
    sy = basey - 2 + torso_offset + arm_offset_y
    draw_rect(d, sx - 2, sy, 4, 4, ARMOR, OUTLINE) # hand
    
    # simple sword orientations
    if sword_rot == 0:
        draw_rect(d, sx, sy - 12, 2, 12, SWORD, OUTLINE)
    elif sword_rot < -80:
        draw_rect(d, sx + 2, sy + 2, 12, 2, SWORD, OUTLINE)
    elif sword_rot < -20:
        draw_rect(d, sx + 2, sy - 8, 10, 2, SWORD, OUTLINE)
    else:
        draw_rect(d, sx, sy - 12, 2, 12, SWORD, OUTLINE)
        
    return img

def build_warrior(state, frame):
    img = create_transparent(32, 48)
    d = ImageDraw.Draw(img)
    OUTLINE = (10, 10, 10, 255)
    ARMOR = (139, 0, 0, 255) # Red dark
    HORN = (92, 58, 30, 255)
    AXE_METAL = (150, 150, 150, 255)
    AXE_WOOD = (80, 40, 20, 255)
    BOOT = (40, 40, 40, 255)
    
    basex = 16
    basey = 23
    
    torso_offset = 0
    arm_offset = 0
    leg_offset_l = 0
    leg_offset_r = 0
    attack_frame = 0
    
    if state == "idle":
        if frame in [2,3]: torso_offset = -1
    elif state == "run":
        torso_offset = 2
        cycles = [
            (-3, 3, -1), (-1, 1, 0), (1, -1, 1), (3, -3, 2),
            (1, -1, 1), (-1, 1, 0), (-2, 2, -1), (-3, 3, -2)
        ]
        c = cycles[frame % 8]
        leg_offset_l = c[0]
        leg_offset_r = c[1]
        arm_offset = c[2]
    elif state == "attack":
        attack_vals = [(0,0), (-2,-4), (-4,-8), (4,5), (6,8), (2,2)]
        attack_frame = frame
        arm_offset = attack_vals[frame][0]
        torso_offset = 1 if frame in [3,4] else 0

    # Legs
    draw_rect(d, basex - 7 + leg_offset_l, basey + 14, 6, 9, BOOT, OUTLINE)
    draw_rect(d, basex + 1 + leg_offset_r, basey + 14, 6, 9, BOOT, OUTLINE)
    
    # Torso
    draw_rect(d, basex - 10, basey - 10 + torso_offset, 20, 18, ARMOR, OUTLINE)
    # Shoulders
    draw_rect(d, basex - 12 + arm_offset, basey - 12 + torso_offset, 6, 6, OUTLINE, None)
    draw_rect(d, basex + 6 - arm_offset, basey - 12 + torso_offset, 6, 6, OUTLINE, None)
    
    # Head
    draw_rect(d, basex - 5, basey - 17 + torso_offset, 10, 8, BOOT, OUTLINE)
    draw_rect(d, basex + 1, basey - 15 + torso_offset, 2, 2, (255,0,0,255), None) # eye
    # Horns
    draw_rect(d, basex - 8, basey - 20 + torso_offset, 4, 4, HORN, OUTLINE)
    draw_rect(d, basex + 4, basey - 20 + torso_offset, 4, 4, HORN, OUTLINE)

    # Axe (Right hand)
    hx = basex + 10 - arm_offset
    hy = basey - 2 + torso_offset
    draw_rect(d, hx - 2, hy, 4, 4, BOOT, OUTLINE) # hand
    
    if state == "attack":
        if attack_frame in [1,2]: 
            # Axe raised
            draw_rect(d, hx, hy - 16, 2, 16, AXE_WOOD, OUTLINE)
            draw_rect(d, hx - 4, hy - 18, 10, 6, AXE_METAL, OUTLINE)
        elif attack_frame in [3,4]:
            # Axe slammed
            draw_rect(d, hx + 4, hy + 4, 16, 2, AXE_WOOD, OUTLINE)
            draw_rect(d, hx + 16, hy, 6, 10, AXE_METAL, OUTLINE)
        else:
            # normal 
            draw_rect(d, hx, hy - 12, 2, 18, AXE_WOOD, OUTLINE)
            draw_rect(d, hx + 2, hy - 8, 8, 12, AXE_METAL, OUTLINE)
    else:
        draw_rect(d, hx, hy - 12, 2, 18, AXE_WOOD, OUTLINE)
        draw_rect(d, hx + 2, hy - 8, 8, 12, AXE_METAL, OUTLINE)

    return img

def build_rogue(state, frame):
    img = create_transparent(32, 48)
    d = ImageDraw.Draw(img)
    OUTLINE = (10, 10, 10, 255)
    CLOTH = (30, 30, 30, 255)
    HOOD = (60, 19, 97, 255) # Purple dark
    EYE = (255, 34, 0, 255)
    DAGGER_GLOW = (255, 100, 0, 255)
    
    basex = 16
    basey = 22
    
    torso_offset = 0
    leg_offset_l = 0
    leg_offset_r = 0
    arm_offset = 0
    cape_offset = 0
    
    if state == "idle":
        if frame in [1,3,5]: cape_offset = 1
        if frame in [2,4]: arm_offset = 1
        if frame in [2,3]: torso_offset = -1
    elif state == "run":
        torso_offset = 3
        cape_offset = 4
        c = [(-2,2,-1), (-1,1,0), (0,0,1), (1,-1,2), (2,-2,1), (1,-1,0), (0,0,-1), (-1,1,-2)]
        leg_offset_l, leg_offset_r, arm_offset = c[frame%8]
    elif state == "attack":
        arm_v = [(0,0), (2,-2), (4, -4), (-2, 2), (-4, 4), (0,0)]
        arm_offset = arm_v[frame][0]
    
    # Cape
    draw_rect(d, basex - 8 - cape_offset, basey + 4, 6, 18, HOOD, OUTLINE)
    
    # Legs
    draw_rect(d, basex - 4 + leg_offset_l, basey + 14, 3, 10, CLOTH, OUTLINE)
    draw_rect(d, basex + 1 + leg_offset_r, basey + 14, 3, 10, CLOTH, OUTLINE)
    
    # Torso
    draw_rect(d, basex - 5, basey - 6 + torso_offset, 10, 16, CLOTH, OUTLINE)
    
    # Head & Hood
    draw_rect(d, basex - 4, basey - 14 + torso_offset, 8, 8, HOOD, OUTLINE)
    draw_rect(d, basex - 2, basey - 12 + torso_offset, 6, 6, (0,0,0,255), None) # face shadow
    draw_rect(d, basex, basey - 10 + torso_offset, 2, 2, EYE, None)
    draw_rect(d, basex+3, basey - 10 + torso_offset, 2, 2, EYE, None)
    
    # Arms + Daggers
    # Left
    draw_rect(d, basex - 8 + arm_offset, basey - 4 + torso_offset, 3, 8, CLOTH, OUTLINE)
    d.line([(basex - 8 + arm_offset, basey + 4 + torso_offset), 
            (basex - 12 + arm_offset, basey + 8 + torso_offset)], fill=DAGGER_GLOW, width=2)
    # Right
    draw_rect(d, basex + 5 - arm_offset, basey - 4 + torso_offset, 3, 8, CLOTH, OUTLINE)
    d.line([(basex + 6 - arm_offset, basey + 4 + torso_offset), 
            (basex + 10 - arm_offset, basey + 8 + torso_offset)], fill=DAGGER_GLOW, width=2)
            
    if state == "attack" and frame in [2, 3]:
        # Cross slash visual effect
        d.line([(basex + 4, basey - 4), (basex + 16, basey + 8)], fill=(255,255,255,255), width=1)
        d.line([(basex + 16, basey - 4), (basex + 4, basey + 8)], fill=(255,255,255,255), width=1)
        
    return img

def assemble_sheet(builder, state, frames):
    sheet = create_transparent(32 * frames, 48)
    for i in range(frames):
        img_frame = builder(state, i)
        sheet.paste(img_frame, (i * 32, 0))
    return sheet

def build_floor_tile():
    img = Image.new("RGBA", (64, 64), (51, 51, 51, 255)) # dark gray
    d = ImageDraw.Draw(img)
    import random
    random.seed(42)
    for i in range(800):
        x = random.randint(0, 63)
        y = random.randint(0, 63)
        color = (40, 40, 40, 255) if random.random() > 0.5 else (20, 20, 20, 255)
        d.point((x,y), fill=color)
    
    # Lava cracks
    for i in range(3):
        x1 = random.randint(0, 63)
        y1 = random.randint(0, 63)
        x2 = x1 + random.randint(-20, 20)
        y2 = y1 + random.randint(-20, 20)
        d.line([(x1, y1), (x2, y2)], fill=(204, 68, 0, 255), width=2)
        d.line([(x1, y1), (x2, y2)], fill=(255, 150, 0, 255), width=1)
    
    # Grid border
    draw_rect(d, 0, 0, 64, 64, None, outline=(10, 10, 10, 255))
    return img

def build_wall_tile():
    img = Image.new("RGBA", (64, 64), (30, 30, 30, 255))
    d = ImageDraw.Draw(img)
    # Bricks
    for y in range(0, 64, 16):
        offset = 16 if (y // 16) % 2 == 0 else 0
        for x in range(0, 64, 32):
            draw_rect(d, x - offset, y, 32, 16, fill=None, outline=(10, 10, 10, 255))
            # highlight
            d.line([(x - offset + 1, y + 1), (x - offset + 30, y + 1)], fill=(60, 60, 60, 255))
    return img

def main():
    out_dir = "assets/sprites/player"
    tile_dir = "assets/sprites/tiles"
    os.makedirs(out_dir, exist_ok=True)
    os.makedirs(tile_dir, exist_ok=True)
    
    classes = {
        "knight": build_knight,
        "warrior": build_warrior,
        "rogue": build_rogue
    }
    
    # Generar sprites de jugador
    for cls_name, builder in classes.items():
        # IDLE
        assemble_sheet(builder, "idle", 6).save(f"{out_dir}/{cls_name}_idle.png")
        # RUN
        assemble_sheet(builder, "run", 8).save(f"{out_dir}/{cls_name}_run.png")
        # ATTACK
        assemble_sheet(builder, "attack", 6).save(f"{out_dir}/{cls_name}_attack.png")
        print(f"Generado {cls_name} spritesheets.")

    # Tiles
    build_floor_tile().save(f"{tile_dir}/floor.png")
    build_wall_tile().save(f"{tile_dir}/wall.png")
    print("Generados tiles floor.png y wall.png.")

if __name__ == "__main__":
    main()
