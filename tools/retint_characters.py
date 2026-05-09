import os
from PIL import Image
import colorsys

def apply_infernal_tint(r, g, b):
    """
    Adjusts a color to fit the Círculo VII dark fantasy infernal theme.
    - Lowers brightness to make it fit the dark world.
    - Shifts bright yellows/whites towards warmer/rusty tones.
    """
    # Convert to HSV (0.0 to 1.0)
    h, s, v = colorsys.rgb_to_hsv(r / 255.0, g / 255.0, b / 255.0)
    
    # 1. Darken everything slightly, but darken brights more
    if v > 0.8:
        v = v * 0.75  # Tone down very bright highlights
    else:
        v = v * 0.85  # General darkening
        
    # 2. Desaturate slightly if it's too colorful (except reds/oranges)
    # Red/Orange hue is roughly 0.0 to 0.15, or 0.85 to 1.0
    is_warm = (h < 0.15) or (h > 0.85)
    if not is_warm:
        s = s * 0.6  # Desaturate cool colors heavily
        # Shift hue towards warm/reddish if it's cool
        h = h * 0.5 + 0.05 # Pull towards orange/red
    else:
        # Boost saturation of warm colors slightly
        s = min(1.0, s * 1.1)
        
    # 3. Add a universal "rust/blood" tint
    # This is done by directly mixing with a dark red/brown
    r_new, g_new, b_new = colorsys.hsv_to_rgb(h, s, v)
    
    tint_r, tint_g, tint_b = (60/255.0, 20/255.0, 15/255.0)
    blend_factor = 0.25 # 25% tint mix
    
    final_r = r_new * (1 - blend_factor) + tint_r * blend_factor
    final_g = g_new * (1 - blend_factor) + tint_g * blend_factor
    final_b = b_new * (1 - blend_factor) + tint_b * blend_factor
    
    return int(final_r * 255), int(final_g * 255), int(final_b * 255)

def process_spritesheet(img_path):
    if not os.path.exists(img_path):
        return False
        
    img = Image.open(img_path).convert("RGBA")
    pixels = img.load()
    w, h = img.size
    
    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            if a > 0: # Only tint non-transparent pixels
                nr, ng, nb = apply_infernal_tint(r, g, b)
                pixels[x, y] = (nr, ng, nb, a)
                
    img.save(img_path)
    print(f"  Retinted: {os.path.basename(img_path)}")
    return True

def main():
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    player_dir = os.path.join(project_root, "assets", "sprites", "player")
    
    print("=== Retinting Player Sprites for Círculo VII ===")
    
    count = 0
    for filename in os.listdir(player_dir):
        if filename.endswith(".png"):
            process_spritesheet(os.path.join(player_dir, filename))
            count += 1
            
    print(f"\nProcessed {count} spritesheets.")

if __name__ == "__main__":
    main()
