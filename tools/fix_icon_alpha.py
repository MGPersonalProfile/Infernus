from PIL import Image
import os

def fix_icon_alpha(filepath):
    if not os.path.exists(filepath):
        print(f"File not found: {filepath}")
        return
        
    img = Image.open(filepath).convert("RGBA")
    data = list(img.getdata())
    width, height = img.size
    
    new_data = []
    transparent_pixels = 0
    total_pixels = len(data)
    
    # Sample top-left pixel as background color
    bg_r, bg_g, bg_b, _ = data[0]
    tolerance = 15
    
    for item in data:
        r, g, b, a = item
        # If the pixel matches the background color within a tolerance, make it transparent
        if abs(r - bg_r) <= tolerance and abs(g - bg_g) <= tolerance and abs(b - bg_b) <= tolerance:
            new_data.append((0, 0, 0, 0))
            transparent_pixels += 1
        else:
            new_data.append(item)
            
    img.putdata(new_data)
    img.save(filepath, "PNG")
    
    alpha_pct = (transparent_pixels / total_pixels) * 100
    print(f"Fixed {os.path.basename(filepath)} - Alpha transparent: {alpha_pct:.1f}%")

if __name__ == "__main__":
    files_to_fix = [
        "assets/sprites/abilities/lanza_flegetonte.png",
        "assets/sprites/abilities/paso_sombrio.png"
    ]
    
    for f in files_to_fix:
        fix_icon_alpha(f)
