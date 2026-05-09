import os
from PIL import Image

def verify_art():
    # A.1 Decor
    decor_files = [
        "assets/sprites/tiles/decor_pillar.png",
        "assets/sprites/tiles/decor_altar.png",
        "assets/sprites/tiles/decor_tombstone.png"
    ]
    
    # A.2 Player Sprites
    player_files = {
        "assets/sprites/player/warrior_idle.png": (288, 56),
        "assets/sprites/player/warrior_attack.png": (288, 56),
        "assets/sprites/player/warrior_run.png": (384, 56),
        "assets/sprites/player/rogue_idle.png": (240, 52),
        "assets/sprites/player/rogue_attack.png": (240, 52),
        "assets/sprites/player/rogue_run.png": (320, 52),
        "assets/sprites/player/knight_idle.png": (312, 60),
        "assets/sprites/player/knight_attack.png": (312, 60),
        "assets/sprites/player/knight_run.png": (416, 60)
    }
    
    # B.2 Icons
    icon_files = [
        "assets/sprites/abilities/lanza_flegetonte.png",
        "assets/sprites/abilities/escudo_hielo.png",
        "assets/sprites/abilities/paso_sombrio.png",
        "assets/sprites/abilities/grito_guerra.png",
        "assets/sprites/abilities/drenar_alma.png"
    ]
    
    # A.5 Torch
    torch_file = "assets/sprites/tiles/torch.png"
    
    print("=== VERIFICANDO ARTE DE CLAUDE ===")
    
    # Decor
    print("\n--- A.1 Decoraciones HD (esperado: 64x64, Alpha=True) ---")
    for f in decor_files:
        if os.path.exists(f):
            img = Image.open(f)
            has_alpha = img.mode in ('RGBA', 'LA') or (img.mode == 'P' and 'transparency' in img.info)
            print(f"OK: {os.path.basename(f)} | Size: {img.size} | Mode: {img.mode} | Has Alpha: {has_alpha}")
        else:
            print(f"MISSING: {f}")
            
    # Player Sprites
    print("\n--- A.2 Sprites de Personajes (dimensiones exactas, Alpha=True) ---")
    for f, dim in player_files.items():
        if os.path.exists(f):
            img = Image.open(f)
            has_alpha = img.mode in ('RGBA', 'LA') or (img.mode == 'P' and 'transparency' in img.info)
            dim_match = img.size == dim
            dim_str = "OK" if dim_match else f"FAIL (esperaba {dim})"
            print(f"OK: {os.path.basename(f)} | Size: {img.size} ({dim_str}) | Has Alpha: {has_alpha}")
        else:
            print(f"MISSING: {f}")
            
    # Icons
    print("\n--- B.2 Iconos de Habilidades (esperado: 24x24, Alpha=True) ---")
    for f in icon_files:
        if os.path.exists(f):
            img = Image.open(f)
            has_alpha = img.mode in ('RGBA', 'LA') or (img.mode == 'P' and 'transparency' in img.info)
            print(f"OK: {os.path.basename(f)} | Size: {img.size} | Has Alpha: {has_alpha}")
        else:
            print(f"MISSING: {f}")
            
    # Torch
    print("\n--- A.5 Antorcha Animada (esperado: 128x32, Alpha=True) ---")
    if os.path.exists(torch_file):
        img = Image.open(torch_file)
        has_alpha = img.mode in ('RGBA', 'LA') or (img.mode == 'P' and 'transparency' in img.info)
        print(f"OK: {os.path.basename(torch_file)} | Size: {img.size} | Has Alpha: {has_alpha}")
    else:
        print(f"MISSING: {torch_file}")

if __name__ == "__main__":
    verify_art()
