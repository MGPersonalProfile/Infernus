#!/usr/bin/env python3
import os
from PIL import Image

TRANSPARENT = (0, 0, 0, 0)
P_OUTLINE = (15, 12, 18, 255)
P_SKIN_BASE = (195, 160, 130, 255)
P_SKIN_SHADOW = (145, 110, 85, 255)

OUT_DIR = "assets/sprites/player"

def ensure_dir():
    os.makedirs(OUT_DIR, exist_ok=True)

def new_img(w, h):
    return Image.new("RGBA", (w, h), TRANSPARENT)

def draw_grid(img, grid, palette, ox=0, oy=0):
    for ry, row in enumerate(grid):
        for rx, idx in enumerate(row):
            if idx > 0 and idx < len(palette):
                if 0 <= ox + rx < img.width and 0 <= oy + ry < img.height:
                    img.putpixel((ox + rx, oy + ry), palette[idx])

# We need final frames to be EXACTLY 32x48.
# If scale=2, we draw at 16x24.
W_NATIVE = 16
H_NATIVE = 24
SCALE = 2

W_CELL = 32
H_CELL = 48

def compile_sheet(frames, cols, name):
    # ALL player sheets have all frames side-by-side horizontally per instructions:
    # "Cada spritesheet es horizontal con frames side-by-side"
    # IDLE=6, RUN=8, ATTACK=6
    sheet = new_img(cols * W_CELL, H_CELL)
    for col_i, (frame_data, pal) in enumerate(frames):
        native = new_img(W_NATIVE, H_NATIVE)
        draw_grid(native, frame_data, pal)
        scaled = native.resize((W_CELL, H_CELL), Image.NEAREST)
        sheet.paste(scaled, (col_i * W_CELL, 0), scaled)
    path = os.path.join(OUT_DIR, f"{name}.png")
    sheet.save(path)
    print(f"Saved: {path}")

# Helper to expand a small 8x12 core block to 16x24 by padding
def pad(grid, ox, oy):
    res = [[0]*W_NATIVE for _ in range(H_NATIVE)]
    for y, row in enumerate(grid):
        for x, val in enumerate(row):
            if 0 <= oy+y < H_NATIVE and 0 <= ox+x < W_NATIVE:
                res[oy+y][ox+x] = val
    return res

# ---------------------------------------------------------
# WARRIOR (Heavy Plate, Massive Axe, Red Accents)
# ---------------------------------------------------------
W_PAL = [
    TRANSPARENT,
    P_OUTLINE,             # 1
    (80, 20, 20, 255),     # 2: dried blood red
    (60, 60, 65, 255),     # 3: dark iron plate
    (90, 90, 95, 255),     # 4: iron highlight
    (140, 110, 50, 255),   # 5: dirty gold trim
    P_SKIN_BASE,           # 6: skin
    (110, 110, 115, 255),  # 7: axe steel
    (40, 30, 25, 255),     # 8: axe handle
]

def make_warrior():
    # Base core: 12x14
    core_idle1 = [
        [0,0,1,1,1,1,1,0,0,0,0,0],
        [0,1,3,3,3,5,3,1,0,0,0,0],
        [0,1,3,2,3,5,3,1,0,0,0,0],
        [0,1,3,3,3,3,3,1,0,0,0,0],
        [0,0,1,5,3,5,1,0,1,1,1,0],
        [0,1,3,3,3,3,3,1,1,7,7,1],
        [1,3,3,2,2,3,3,3,1,7,4,1],
        [1,3,5,3,3,5,3,3,1,8,1,0],
        [0,1,3,3,3,3,3,1,0,8,0,0],
        [0,0,1,5,1,5,1,0,0,8,0,0],
        [0,0,1,3,0,3,1,0,0,8,0,0],
        [0,1,3,3,0,3,3,1,0,8,0,0],
        [1,3,3,1,0,1,3,3,1,1,0,0],
        [0,1,1,0,0,0,1,1,0,0,0,0]
    ]
    core_idle2 = core_idle1[:]
    core_idle2[11] = [0,1,3,3,0,1,3,3,1,8,0,0] # slight bob
    
    idle_frames = [(pad(core_idle1, 2, 10), W_PAL), (pad(core_idle2, 2, 10), W_PAL)] * 3
    compile_sheet(idle_frames, 6, "warrior_idle")
    
    # Run
    run_frames = [(pad(core_idle1, 2, 10), W_PAL)] * 8
    compile_sheet(run_frames, 8, "warrior_run")
    
    # Attack
    atk_frames = [(pad(core_idle2, 2, 10), W_PAL)] * 6
    compile_sheet(atk_frames, 6, "warrior_attack")

# ---------------------------------------------------------
# ROGUE (Dark Hood, Purples/Blacks, Daggers)
# ---------------------------------------------------------
R_PAL = [
    TRANSPARENT,
    P_OUTLINE,             # 1
    (40, 20, 50, 255),     # 2: dark purple cloak
    (60, 40, 75, 255),     # 3: mid purple
    (20, 15, 25, 255),     # 4: dark underclothes
    (150, 40, 40, 255),    # 5: red eye
    (180, 190, 200, 255),  # 6: dagger blade
]

def make_rogue():
    core = [
        [0,1,1,1,1,0,0,0],
        [1,4,4,4,4,1,0,0],
        [1,2,2,2,2,1,0,0],
        [1,4,5,4,2,1,0,0],
        [0,1,1,1,1,0,0,0],
        [1,2,3,2,2,1,0,0],
        [1,2,2,1,6,6,1,0],
        [0,1,2,2,1,1,0,0],
        [0,1,4,4,1,0,0,0],
        [0,1,4,1,4,1,0,0],
        [1,2,2,0,1,2,2,1],
        [1,1,1,0,0,1,1,1]
    ]
    frames = [(pad(core, 4, 12), R_PAL)] * 6
    compile_sheet(frames, 6, "rogue_idle")
    compile_sheet([(pad(core, 4, 12), R_PAL)] * 8, 8, "rogue_run")
    compile_sheet([(pad(core, 4, 12), R_PAL)] * 6, 6, "rogue_attack")

# ---------------------------------------------------------
# KNIGHT (Bone-Engraved Cursed Armor)
# ---------------------------------------------------------
K_PAL = [
    TRANSPARENT,
    P_OUTLINE,             # 1
    (50, 50, 50, 255),     # 2: ashen armor
    (70, 70, 70, 255),     # 3: armor highlight
    (130, 120, 100, 255),  # 4: bone engraved parts
    (120, 30, 30, 255),    # 5: deep red plume
    (100, 90, 110, 255),   # 6: shield/sword face
]

def make_knight():
    core = [
        [0,0,0,5,5,5,0,0],
        [0,0,1,1,1,1,0,0],
        [0,1,2,2,4,2,1,0],
        [0,1,2,2,2,2,1,0],
        [0,0,1,2,2,1,0,0],
        [0,1,6,6,2,2,1,0],
        [1,6,1,6,3,2,2,1],
        [1,6,6,6,2,2,2,1],
        [1,6,6,6,2,1,4,1],
        [0,1,6,1,2,1,2,1],
        [0,0,1,0,1,2,1,0],
        [0,0,1,0,0,1,1,0]
    ]
    frames = [(pad(core, 4, 12), K_PAL)] * 6
    compile_sheet(frames, 6, "knight_idle")
    compile_sheet([(pad(core, 4, 12), K_PAL)] * 8, 8, "knight_run")
    compile_sheet([(pad(core, 4, 12), K_PAL)] * 6, 6, "knight_attack")

if __name__ == "__main__":
    ensure_dir()
    make_warrior()
    make_rogue()
    make_knight()
    print("Done")
