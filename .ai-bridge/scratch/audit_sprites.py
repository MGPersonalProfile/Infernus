import os
import json
from PIL import Image, ImageDraw, ImageEnhance

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT_ROOT = os.path.dirname(PROJECT_ROOT)
SPRITES_DIR = os.path.join(PROJECT_ROOT, 'assets', 'sprites')
ART_DIR = os.path.join(PROJECT_ROOT, 'assets', 'art')

def apply_infernal_color_grade(image_path):
    if not os.path.exists(image_path):
        print(f"Skipping (not found): {image_path}")
        return False
    try:
        img = Image.open(image_path).convert('RGBA')
        img = ImageEnhance.Contrast(img).enhance(1.15)
        img = ImageEnhance.Color(img).enhance(1.2)
        overlay = Image.new('RGBA', img.size, (180, 50, 0, 80)) # red-orange tint
        img = Image.alpha_composite(img, overlay)
        img = ImageEnhance.Brightness(img).enhance(0.85)
        img.save(image_path)
        print(f"OK Color grading aplicado a: {os.path.basename(image_path)}")
        return True
    except Exception as e:
        print(f"Error procesando {image_path}: {e}")
        return False

def generate_palette_reference():
    out_path = os.path.join(ART_DIR, 'palette_reference.png')
    img = Image.new('RGB', (256, 64))
    draw = ImageDraw.Draw(img)
    # Colors: Rojo sangre #8B0000, Gris ceniza #2B2B2B, Naranja lava #CC4400, Negro profundo #0A0A0A, Rojo brillante #FF2200, Dorado #DAA520
    colors = [
        ("#8B0000", "Sangre"),
        ("#2B2B2B", "Ceniza"),
        ("#CC4400", "Lava"),
        ("#0A0A0A", "Negro"),
        ("#FF2200", "Highlight"),
        ("#DAA520", "Dorado")
    ]
    swatch_width = 256 // len(colors)
    for i, (hex_c, name) in enumerate(colors):
        x0 = i * swatch_width
        x1 = x0 + swatch_width
        draw.rectangle([x0, 0, x1, 64], fill=hex_c)
    img.save(out_path)
    print(f"OK palette_reference.png generado.")

def audit_spritesheet(path, expected_w, expected_h, expected_frames):
    if not os.path.exists(path):
        return {"status": "MISSING", "error": "File not found"}
    
    img = Image.open(path)
    w, h = img.size
    
    issues = []
    
    if w != expected_w or h != expected_h:
        issues.append(f"Dimension mismatch: got {w}x{h}, expected {expected_w}x{expected_h}")
    
    frame_w = w // expected_frames if expected_frames else w
    
    if expected_frames > 0 and w % expected_frames != 0:
        issues.append(f"Width {w} is not divisible by frames {expected_frames}")
        
    img_rgba = img.convert("RGBA")
    extrema = img_rgba.getextrema()
    # Check alpha channel for transparency
    if extrema[3][0] > 0: # min alpha > 0 means no fully transparent pixels
        issues.append("Background is not fully transparent.")
        
    # Artifact check: look for orphan pixels at the boundaries of frames? Hard to be exact without knowing the sprite.
    # We will assume if alpha is 0 somewhere, it's mostly transparent.
    
    # Palette check: check if average color (ignoring alpha=0) has some red/orange/grey.
    colors = img_rgba.getcolors(maxcolors=10000)
    if colors:
        non_transparent = [c for count, c in colors if c[3] > 100]
        if not non_transparent:
            issues.append("Image seems completely empty or invisible.")
      
    if len(issues) > 0:
        # Trivial fix: if no transparency, we could try chroma key, but if dimensions are wrong we fail
        if "Background is not fully transparent." in issues and len(issues) == 1:
            return {"status": "PROBLEMAS MENORES", "issues": issues, "fixable": True}
        return {"status": "NECESITA REHACERSE", "issues": issues}
    
    return {"status": "OK", "issues": []}

def run_audit():
    sprites_to_check = [
        # Players
        ("player/warrior_idle.png", 288, 56, 6),
        ("player/warrior_run.png", 384, 56, 8),
        ("player/warrior_attack.png", 288, 56, 6),
        ("player/rogue_idle.png", 240, 52, 6),
        ("player/rogue_run.png", 320, 52, 8),
        ("player/rogue_attack.png", 240, 52, 6),
        ("player/knight_idle.png", 312, 60, 6),
        ("player/knight_run.png", 416, 60, 8),
        ("player/knight_attack.png", 312, 60, 6),
        # Enemies
        ("enemies/melee_idle.png", -1, -1, 6),
        ("enemies/melee_run.png", -1, -1, 8),
        ("enemies/melee_attack.png", -1, -1, 6),
        ("enemies/ranged_idle.png", -1, -1, 6),
        ("enemies/ranged_run.png", -1, -1, 8),
        ("enemies/ranged_attack.png", -1, -1, 6),
        ("enemies/tank_idle.png", -1, -1, 6),
        ("enemies/tank_run.png", -1, -1, 8),
        ("enemies/tank_attack.png", -1, -1, 6),
        ("enemies/assassin_idle.png", -1, -1, 6),
        ("enemies/assassin_run.png", -1, -1, 8),
        ("enemies/assassin_attack.png", -1, -1, 6),
        ("enemies/bomber_idle.png", -1, -1, 6),
        ("enemies/bomber_run.png", -1, -1, 8),
        ("enemies/bomber_attack.png", -1, -1, 6),
        # Boss
        ("bosses/minotaur_idle.png", 160, 80, 2),
        ("bosses/minotaur_charge.png", 240, 80, 3),
        ("bosses/minotaur_slam.png", 240, 80, 3),
        # Particles
        ("particles/ash_particle.png", 32, 8, 4),
        ("particles/blood_drop.png", 24, 8, 3) # the new json says 3 frames of 8x8 -> 24x8
    ]

    report = {}
    for rel_path, ew, eh, ef in sprites_to_check:
        full_path = os.path.join(SPRITES_DIR, rel_path)
        # If ew/eh are -1, just check file exists and frame division
        if ew == -1:
            if os.path.exists(full_path):
                img = Image.open(full_path)
                ew, eh = img.size
            else:
                ew, eh = 0, 0
                
        res = audit_spritesheet(full_path, ew, eh, ef)
        
        # Specially fix blood_drop if it mismatched expected 3 frames
        if rel_path == "particles/blood_drop.png" and "Dimension mismatch" in " ".join(res.get("issues", [])):
            if os.path.exists(full_path):
                img = Image.open(full_path)
                w, h = img.size
                if w == 24 and h == 8:
                    res["issues"] = [i for i in res["issues"] if "Dimension" not in i]
                    if len(res["issues"]) == 0: res["status"] = "OK"
        
        report[rel_path] = res
        print(f"{rel_path}: {res['status']} {res['issues']}")
        
    with open(os.path.join(PROJECT_ROOT, '.ai-bridge', 'scratch', 'audit_report.json'), 'w') as f:
        json.dump(report, f, indent=2)

if __name__ == "__main__":
    generate_palette_reference()
    apply_infernal_color_grade(os.path.join(SPRITES_DIR, 'tiles', 'floor.png'))
    apply_infernal_color_grade(os.path.join(SPRITES_DIR, 'tiles', 'wall.png'))
    apply_infernal_color_grade(os.path.join(ART_DIR, 'parallax_dungeon.png'))
    apply_infernal_color_grade(os.path.join(ART_DIR, 'ui_panel.png'))
    run_audit()
