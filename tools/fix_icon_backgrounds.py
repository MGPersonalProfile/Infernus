import os
from PIL import Image

def make_black_transparent(img_path):
    if not os.path.exists(img_path):
        print(f"File not found: {img_path}")
        return False
        
    img = Image.open(img_path).convert("RGBA")
    pixels = img.load()
    w, h = img.size
    
    modified = False
    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            # Treat pure or near pure black as transparent
            if r < 5 and g < 5 and b < 5:
                pixels[x, y] = (0, 0, 0, 0)
                modified = True
            elif a < 255:
                # Ensure opaque for pixel art
                pixels[x, y] = (r, g, b, 255)
                modified = True
                
    if modified:
        img.save(img_path)
        print(f"Fixed background for {img_path}")
    return True

def main():
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    abilities_dir = os.path.join(project_root, "assets", "sprites", "abilities")
    
    icons = [
        "lanza_flegetonte.png",
        "escudo_hielo.png",
        "paso_sombrio.png",
        "grito_guerra.png",
        "drenar_alma.png"
    ]
    
    for icon in icons:
        make_black_transparent(os.path.join(abilities_dir, icon))
        
    # Also fix the fire_spear
    fx_dir = os.path.join(project_root, "assets", "sprites", "fx")
    make_black_transparent(os.path.join(fx_dir, "fire_spear.png"))

if __name__ == "__main__":
    main()
