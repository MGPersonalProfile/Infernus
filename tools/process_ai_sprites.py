#!/usr/bin/env python3
import os
import glob
from PIL import Image

BRAIN_DIR = r"C:\Users\Juan Miguel\.gemini\antigravity\brain\63ddb7fb-80e9-4fdd-a004-582c49e96e44"
OUT_DIR = r"C:\Users\Juan Miguel\Roguesouls-like\assets\sprites\player"

# Map string identifier to exact frame count
FRAME_MAP = {
    'idle': 6,
    'run': 8,
    'attack': 6
}

PALETTES = {
    'knight': [(0,0,0,0), (30,30,30,255), (80,80,80,255), (150,150,150,255), (200,180,150,255), (15,10,10,255)],
    'warrior': [(0,0,0,0), (40,20,20,255), (90,40,40,255), (70,70,75,255), (150,120,60,255), (15,10,10,255)],
    'rogue': [(0,0,0,0), (30,15,40,255), (60,30,70,255), (180,50,50,255), (150,150,160,255), (15,10,10,255)]
}

def color_distance(c1, c2):
    return sum((a - b) ** 2 for a, b in zip(c1[:3], c2[:3]))

def closest_color(rgba, palette):
    # If very dark or near-black background, make transparent
    if rgba[0] < 20 and rgba[1] < 20 and rgba[2] < 20:
        return palette[0]
    
    best_dist = float('inf')
    best_color = palette[1]
    for p in palette[1:]:
        d = color_distance(rgba, p)
        if d < best_dist:
            best_dist = d
            best_color = p
    return best_color

def process():
    os.makedirs(OUT_DIR, exist_ok=True)
    raw_files = glob.glob(os.path.join(BRAIN_DIR, "*_raw_*.png"))
    
    for path in raw_files:
        basename = os.path.basename(path)
        parts = basename.split('_')
        cls_name = parts[0]
        anim_name = parts[1]
        
        frames = FRAME_MAP.get(anim_name, 6)
        target_w = frames * 32
        target_h = 48
        
        img = Image.open(path).convert("RGBA")
        
        # 1. Force downscale exactly to mathematical grid using Nearest Neighbor.
        img = img.resize((target_w, target_h), Image.NEAREST)
        
        # 2. Quantize colors to hard edges explicitly
        pal = PALETTES.get(cls_name, PALETTES['knight'])
        pixels = img.load()
        for y in range(target_h):
            for x in range(target_w):
                pixels[x, y] = closest_color(pixels[x, y], pal)
        
        # 3. Save
        out_name = f"{cls_name}_{anim_name}.png"
        out_path = os.path.join(OUT_DIR, out_name)
        img.save(out_path)
        print(f"Processed: {out_name} -> {target_w}x{target_h}")

if __name__ == "__main__":
    process()
