"""
INFERNUS -- Miniboss Sprites
Genera los 3 spritesheets que MiniBossFactory.h necesita:
- demon_idle.png (32x32) — melee miniboss base (Quiron, Caballero Infernal, Arpia)
- lancer_idle.png (24x48) — ranged miniboss base (Neso, Arquero de Almas)
- brute_idle.png (48x48) — tank miniboss base (Demonio del Foso)
Se escalan x1.8 en juego, asi que deben verse bien a ~58x58 / 43x86 / 86x86.
"""
from PIL import Image, ImageDraw
import os, math

OUT = "assets/sprites/enemies"
os.makedirs(OUT, exist_ok=True)

def ct(w,h): return Image.new("RGBA",(w,h),(0,0,0,0))
def dr(d,x,y,w,h,fill,ol=None):
    if w>0 and h>0: d.rectangle([x,y,x+w-1,y+h-1],fill=fill,outline=ol)
def dp(d,x,y,c): d.point((x,y),fill=c)

# === DEMON (32x32) — Hulking demon torso, horns, glowing eyes ===
# Used by: Quiron, Caballero Infernal, Arpia Reina (all melee type)
DM_OL=(10,10,10,255); DM_SKIN=(130,40,30,255); DM_SKIN2=(90,25,18,255)
DM_ARMOR=(55,45,40,255); DM_EYE=(255,200,30,255); DM_HORN=(100,65,30,255)
DM_GLOW=(200,80,20,180)

def build_demon(frame, total=6):
    img=ct(32,32); d=ImageDraw.Draw(img); cx=16; by=16
    ph=(frame/total)*2*math.pi
    tdy=int(math.sin(ph)*1.2)
    
    # Legs
    dr(d,cx-6,by+8+tdy,4,8,DM_SKIN2,DM_OL)
    dr(d,cx+2,by+8+tdy,4,8,DM_SKIN2,DM_OL)
    # Hooves
    dr(d,cx-7,by+14+tdy,5,3,DM_ARMOR,DM_OL)
    dr(d,cx+1,by+14+tdy,5,3,DM_ARMOR,DM_OL)
    
    # Torso (wide, menacing)
    dr(d,cx-8,by-6+tdy,16,14,DM_SKIN,DM_OL)
    # Chest scar / armor plate
    dr(d,cx-4,by-2+tdy,8,6,DM_ARMOR)
    dr(d,cx-2,by+tdy,4,4,DM_GLOW)  # infernal glow in chest
    
    # Arms (thick)
    dr(d,cx-11,by-4+tdy,4,10,DM_SKIN,DM_OL)
    dr(d,cx+7,by-4+tdy,4,10,DM_SKIN,DM_OL)
    # Clawed hands
    dp(d,cx-12,by+6+tdy,(200,100,30,255))
    dp(d,cx+11,by+6+tdy,(200,100,30,255))
    
    # Head (smaller than body for intimidation)
    hy=by-12+tdy
    dr(d,cx-4,hy,8,6,DM_SKIN,DM_OL)
    # Eyes
    dp(d,cx-2,hy+2,DM_EYE); dp(d,cx+2,hy+2,DM_EYE)
    # Brow
    dr(d,cx-3,hy+1,6,1,DM_SKIN2)
    # Horns (prominent, curved)
    dr(d,cx-7,hy-3,3,4,DM_HORN,DM_OL)
    dr(d,cx+4,hy-3,3,4,DM_HORN,DM_OL)
    dp(d,cx-8,hy-4,DM_HORN)
    dp(d,cx+5,hy-4,DM_HORN)
    
    # Shoulder spikes
    dr(d,cx-10,by-6+tdy,3,3,DM_ARMOR,DM_OL)
    dr(d,cx+7,by-6+tdy,3,3,DM_ARMOR,DM_OL)
    
    return img


# === LANCER (24x48) — Tall spectral figure with spear/bow ===
# Used by: Neso, Arquero de Almas (ranged type)
LN_OL=(10,10,10,255); LN_ROBE=(30,35,60,255); LN_ROBE2=(20,22,40,255)
LN_SKIN=(80,90,110,255); LN_EYE=(100,180,255,255); LN_SPEAR=(140,140,150,255)
LN_GLOW=(60,120,200,180)

def build_lancer(frame, total=6):
    img=ct(24,48); d=ImageDraw.Draw(img); cx=12; by=24
    ph=(frame/total)*2*math.pi
    tdy=int(math.sin(ph)*1.0)
    
    # Robe (long, flowing)
    dr(d,cx-5,by-2+tdy,10,20,LN_ROBE,LN_OL)
    dr(d,cx-6,by+14+tdy,12,6,LN_ROBE,LN_OL)  # flare
    # Center fold
    dr(d,cx-1,by+tdy,2,16,LN_ROBE2)
    
    # Torso (armored upper)
    dr(d,cx-5,by-8+tdy,10,8,LN_ROBE2,LN_OL)
    # Spectral chest plate
    dr(d,cx-3,by-6+tdy,6,4,LN_GLOW)
    
    # Head (hooded, spectral)
    hy=by-16+tdy
    dr(d,cx-4,hy,8,8,LN_ROBE,LN_OL)
    dr(d,cx-2,hy-2,4,3,LN_ROBE,LN_OL)  # hood peak
    # Face void
    dr(d,cx-2,hy+3,4,4,(10,12,20,255))
    # Spectral eyes
    dp(d,cx-1,hy+4,LN_EYE); dp(d,cx+2,hy+4,LN_EYE)
    # Eye trail
    dp(d,cx-2,hy+4,(60,120,200,120))
    dp(d,cx+3,hy+4,(60,120,200,120))
    
    # Spear (right side, tall)
    sx=cx+6
    dr(d,sx,by-14+tdy,2,30,LN_SPEAR,LN_OL)
    # Spearhead
    dr(d,sx-1,by-17+tdy,4,4,LN_SPEAR,LN_OL)
    dp(d,sx,by-18+tdy,(200,220,255,255))  # tip glow
    
    # Arms
    dr(d,cx+4,by-4+tdy,3,6,LN_SKIN,LN_OL)
    dr(d,cx-6,by-4+tdy,3,6,LN_SKIN,LN_OL)
    
    return img


# === BRUTE (48x48) — Massive armored demon, bigger than tank ===
# Used by: Demonio del Foso (tank type)
BR_OL=(10,10,10,255); BR_ARMOR=(70,50,40,255); BR_ARMOR2=(50,35,25,255)
BR_SKIN=(110,50,35,255); BR_EYE=(255,80,20,255); BR_HORN=(80,55,25,255)
BR_CHAIN=(90,80,70,255); BR_WEAPON=(120,120,130,255)

def build_brute(frame, total=6):
    img=ct(48,48); d=ImageDraw.Draw(img); cx=24; by=24
    ph=(frame/total)*2*math.pi
    tdy=int(math.sin(ph)*1.5)
    
    # Legs (massive)
    dr(d,cx-8,by+10+tdy,6,12,BR_ARMOR2,BR_OL)
    dr(d,cx+2,by+10+tdy,6,12,BR_ARMOR2,BR_OL)
    # Boots
    dr(d,cx-10,by+20+tdy,8,4,BR_ARMOR,BR_OL)
    dr(d,cx+0,by+20+tdy,8,4,BR_ARMOR,BR_OL)
    
    # Torso (huge)
    dr(d,cx-12,by-8+tdy,24,18,BR_ARMOR,BR_OL)
    # Chest plate detail
    dr(d,cx-6,by-4+tdy,12,10,BR_ARMOR2)
    # Chains
    for i in range(0,20,4):
        dr(d,cx-10+i,by+2+tdy,2,2,BR_CHAIN)
    
    # Massive pauldrons
    dr(d,cx-16,by-10+tdy,8,8,BR_ARMOR,BR_OL)
    dr(d,cx+8,by-10+tdy,8,8,BR_ARMOR,BR_OL)
    # Spikes on pauldrons
    dp(d,cx-16,by-12+tdy,BR_ARMOR2)
    dp(d,cx+15,by-12+tdy,BR_ARMOR2)
    
    # Head (tiny compared to body)
    hy=by-16+tdy
    dr(d,cx-4,hy,8,7,BR_SKIN,BR_OL)
    dp(d,cx-2,hy+3,BR_EYE); dp(d,cx+2,hy+3,BR_EYE)
    # Massive horns
    dr(d,cx-8,hy-4,4,5,BR_HORN,BR_OL)
    dr(d,cx+5,hy-4,4,5,BR_HORN,BR_OL)
    dp(d,cx-9,hy-5,BR_HORN)
    dp(d,cx+9,hy-5,BR_HORN)
    
    # Right arm + huge weapon
    dr(d,cx+12,by-6+tdy,6,14,BR_SKIN,BR_OL)
    # Warhammer
    dr(d,cx+14,by+8+tdy,2,12,BR_ARMOR2,BR_OL)
    dr(d,cx+11,by+18+tdy,8,6,BR_WEAPON,BR_OL)
    
    # Left arm + shield
    dr(d,cx-16,by-4+tdy,6,12,BR_SKIN,BR_OL)
    dr(d,cx-20,by-6+tdy,6,14,BR_ARMOR,BR_OL)
    # Shield emblem
    dr(d,cx-19,by-2+tdy,4,6,BR_ARMOR2)
    dp(d,cx-17,by+1+tdy,(200,80,20,200))
    
    return img


def assemble(builder, frames, fw, fh):
    sheet = ct(fw * frames, fh)
    for i in range(frames):
        sheet.paste(builder(i, frames), (i * fw, 0))
    return sheet


def main():
    print("INFERNUS -- Miniboss Sprites")
    print("=" * 50)
    
    # demon_idle.png — 6 frames, 32x32
    demon = assemble(build_demon, 6, 32, 32)
    demon.save(os.path.join(OUT, "demon_idle.png"))
    print(f"  demon_idle: {demon.size} (melee minibosses)")
    
    # lancer_idle.png — 6 frames, 24x48
    lancer = assemble(build_lancer, 6, 24, 48)
    lancer.save(os.path.join(OUT, "lancer_idle.png"))
    print(f"  lancer_idle: {lancer.size} (ranged minibosses)")
    
    # brute_idle.png — 6 frames, 48x48
    brute = assemble(build_brute, 6, 48, 48)
    brute.save(os.path.join(OUT, "brute_idle.png"))
    print(f"  brute_idle: {brute.size} (tank minibosses)")
    
    print("\n  3 miniboss bases generados (x1.8 scale en juego)")


if __name__ == "__main__":
    main()
