import os
from PIL import Image, ImageDraw, ImageEnhance

# Directories
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT_ROOT = os.path.dirname(PROJECT_ROOT) # since it's in .ai-bridge/scratch/
ART_DIR = os.path.join(PROJECT_ROOT, 'assets', 'art')
PARTICLES_DIR = os.path.join(PROJECT_ROOT, 'assets', 'sprites', 'particles')

def apply_infernal_color_grade(image_path):
    """Aplica una paleta infernal Círculo VII (rojos, naranjas oscuros, grises)."""
    if not os.path.exists(image_path):
        print(f"Skipping (not found): {image_path}")
        return
        
    img = Image.open(image_path).convert('RGBA')
    
    # 1. Enhance Contrast and Color
    img = ImageEnhance.Contrast(img).enhance(1.15)
    img = ImageEnhance.Color(img).enhance(1.2)
    
    # Wait, we can tint it by blending with a solid color.
    # The palette required:
    # reds (#8B0000, #CC0000)
    # lava orange (#CC4400, #FF6600)
    # dark grey (#2B2B2B, #1A1A1A)
    
    # Let's blend a dark red/orange multiply overlay.
    overlay = Image.new('RGBA', img.size, (180, 50, 0, 80)) # red-orange tint
    img = Image.alpha_composite(img, overlay)
    
    # Optional: decrease brightness slightly to make it darker/hellish
    img = ImageEnhance.Brightness(img).enhance(0.85)

    img.save(image_path)
    print(f"✅ Color grading aplicado a: {os.path.basename(image_path)}")

def generate_ash_particle():
    """Generates ash_particle.png: 4 frames of 8x8"""
    fw, fh = 8, 8
    img = Image.new('RGBA', (fw * 4, fh), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Colors: dark grey, glowing ember, dying ember
    c_ember = (255, 100, 0, 255)
    c_dying = (180, 50, 0, 255)
    c_ash = (80, 80, 80, 255)
    
    # Frame 1: Bright burning
    draw.point((3, 3), fill=c_ember)
    draw.point((4, 3), fill=c_ember)
    draw.point((3, 4), fill=c_dying)
    draw.point((4, 4), fill=c_ember)
    draw.point((3, 2), fill=c_ash)
    
    # Frame 2: Starting to cool and drift
    draw.point((fw + 4, 3), fill=c_dying)
    draw.point((fw + 5, 3), fill=c_ember)
    draw.point((fw + 4, 4), fill=c_dying)
    draw.point((fw + 3, 3), fill=c_ash)
    draw.point((fw + 4, 2), fill=c_ash)
    
    # Frame 3: mostly ash
    draw.point((fw*2 + 4, 2), fill=c_ash)
    draw.point((fw*2 + 5, 2), fill=c_dying)
    draw.point((fw*2 + 4, 3), fill=c_ash)
    draw.point((fw*2 + 5, 3), fill=c_ash)
    
    # Frame 4: just gray ash
    draw.point((fw*3 + 5, 2), fill=c_ash)
    draw.point((fw*3 + 6, 1), fill=(50,50,50,200))
    draw.point((fw*3 + 5, 3), fill=c_ash)
    
    os.makedirs(PARTICLES_DIR, exist_ok=True)
    out_path = os.path.join(PARTICLES_DIR, 'ash_particle.png')
    img.save(out_path)
    print(f"✅ Partícula generada: ash_particle.png (32x8)")

def generate_blood_drop():
    """Generates blood_drop.png: 4 frames of 6x8"""
    fw, fh = 6, 8
    img = Image.new('RGBA', (fw * 4, fh), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    c_blood = (160, 0, 0, 255)
    c_dark = (100, 0, 0, 255)
    c_highlight = (220, 50, 50, 255)
    c_splat = (140, 0, 0, 255)
    
    # Frame 1: Falling drop (elongated)
    draw.point((2, 1), fill=c_dark)
    draw.point((2, 2), fill=c_blood)
    draw.point((2, 3), fill=c_blood)
    draw.point((2, 4), fill=c_blood)
    draw.point((2, 5), fill=c_dark)
    draw.point((3, 3), fill=c_highlight)
    
    # Frame 2: Squishing as it hits ground
    draw.point((fw + 2, 4), fill=c_dark)
    draw.point((fw + 2, 5), fill=c_blood)
    draw.point((fw + 3, 5), fill=c_blood)
    draw.point((fw + 2, 6), fill=c_blood)
    draw.point((fw + 1, 6), fill=c_dark)
    draw.point((fw + 3, 6), fill=c_dark)
    
    # Frame 3: Splat outward
    draw.point((fw*2 + 2, 6), fill=c_splat)
    draw.point((fw*2 + 1, 7), fill=c_blood)
    draw.point((fw*2 + 2, 7), fill=c_blood)
    draw.point((fw*2 + 3, 7), fill=c_blood)
    draw.point((fw*2 + 0, 7), fill=c_dark)
    draw.point((fw*2 + 4, 7), fill=c_dark)
    
    # Frame 4: Settled puddle
    draw.point((fw*3 + 1, 7), fill=c_dark)
    draw.point((fw*3 + 2, 7), fill=c_splat)
    draw.point((fw*3 + 3, 7), fill=c_blood)
    draw.point((fw*3 + 4, 7), fill=c_splat)
    draw.point((fw*3 + 5, 7), fill=c_dark)
    draw.point((fw*3 + 2, 6), fill=c_highlight) # small glint

    out_path = os.path.join(PARTICLES_DIR, 'blood_drop.png')
    img.save(out_path)
    print(f"✅ Partícula generada: blood_drop.png (24x8)")

if __name__ == '__main__':
    print("=== Procesando Color y Partículas ===")
    
    # 1. Color grade backgrounds and UI
    apply_infernal_color_grade(os.path.join(ART_DIR, 'title_bg.png'))
    apply_infernal_color_grade(os.path.join(ART_DIR, 'parallax_dungeon.png'))
    apply_infernal_color_grade(os.path.join(ART_DIR, 'ui_panel.png'))
    
    # 2. Generate particles
    generate_ash_particle()
    generate_blood_drop()
    
    print("=== ¡Proceso Completado! ===")
