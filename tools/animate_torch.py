from PIL import Image, ImageEnhance
import os
import random

def create_animated_torch():
    src_path = "assets/sprites/tiles/torch.png"
    if not os.path.exists(src_path):
        print(f"File {src_path} not found.")
        return
        
    img = Image.open(src_path).convert("RGBA")
    width, height = img.size
    
    # If the torch is 64x32, it might be a single 64x32 tile or two 32x32 tiles.
    # Let's assume the user wants a 4-frame animation of the left-most 32x32 part,
    # or if it's 64x64, etc. We will extract the first 32x32 or 64x64 frame and duplicate it 4 times side-by-side.
    frame_w = width if width <= height else height # e.g. if 64x32, frame_w = 32
    frame_h = height
    
    base_frame = img.crop((0, 0, frame_w, frame_h))
    
    # Create 4 frames
    frames = []
    frames.append(base_frame)
    
    # Generate 3 more frames with slight brightness flicker
    for i in range(1, 4):
        enhancer = ImageEnhance.Brightness(base_frame)
        flicker = enhancer.enhance(1.0 + random.uniform(-0.15, 0.15))
        frames.append(flicker)
        
    out_img = Image.new("RGBA", (frame_w * 4, frame_h))
    for i, f in enumerate(frames):
        out_img.paste(f, (i * frame_w, 0))
        
    out_img.save(src_path)
    print(f"Saved 4-frame animated torch to {src_path} with size {out_img.size}")

if __name__ == "__main__":
    create_animated_torch()
