import os
from PIL import Image

def pad_to_64x64(img_path):
    if not os.path.exists(img_path):
        print(f"Not found: {img_path}")
        return False
        
    img = Image.open(img_path).convert("RGBA")
    w, h = img.size
    
    if w == 64 and h == 64:
        print(f"Already 64x64: {img_path}")
        return True
        
    # Create new 64x64 transparent image
    new_img = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    
    # Calculate offset to center the image
    # For pillars/tombstones, we want them centered horizontally, and bottom-aligned
    # or centered vertically. The C++ code places them at (wx, wy) which is the tile coordinate.
    # Tile coordinate is top-left, so bottom-aligning them to the 64x64 box is usually best
    # for top-down perspective, so they stand on the tile floor.
    offset_x = (64 - w) // 2
    offset_y = 64 - h
    
    new_img.paste(img, (offset_x, offset_y), img)
    new_img.save(img_path)
    print(f"Resized {w}x{h} -> 64x64: {os.path.basename(img_path)}")
    return True

def main():
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    tiles_dir = os.path.join(project_root, "assets", "sprites", "tiles")
    
    files_to_pad = [
        "decor_pillar.png",
        "decor_altar.png",
        "decor_tombstone.png"
    ]
    
    for f in files_to_pad:
        pad_to_64x64(os.path.join(tiles_dir, f))
        
if __name__ == "__main__":
    main()
