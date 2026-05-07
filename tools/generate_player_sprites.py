"""
INFERNUS -- Player Sprite Upgrade
Regenera las 3 clases con siluetas claras al tamano que PlayerFactory espera.
warrior=48x56, knight=52x60, rogue=40x52. 8 frames idle/run, 6 attack.
"""
from PIL import Image, ImageDraw
import os, math

OUT = "assets/sprites/player"
os.makedirs(OUT, exist_ok=True)

def ct(w,h): return Image.new("RGBA",(w,h),(0,0,0,0))
def dr(d,x,y,w,h,fill,ol=None):
    if w>0 and h>0: d.rectangle([x,y,x+w-1,y+h-1],fill=fill,outline=ol)
def dp(d,x,y,c): d.point((x,y),fill=c)

# === WARRIOR (48x56) — Heavy armor, great axe, red cape ===
W_OL=(10,10,10,255); W_ARM=(140,50,30,255); W_ARM2=(100,35,20,255)
W_SKIN=(180,130,100,255); W_CAPE=(160,30,20,255); W_AXE=(170,170,180,255)
W_WOOD=(80,50,25,255); W_EYE=(255,200,50,255); W_GOLD=(200,170,60,255)

def build_warrior(st,f,tf=8):
    img=ct(48,56); d=ImageDraw.Draw(img); cx,by=24,28
    tdy=0; ldx=0; rdx=0; adx=0; ady=0; atk="rest"
    if st=="idle":
        ph=(f/tf)*2*math.pi; tdy=int(math.sin(ph)*1.2)
    elif st=="run":
        ph=(f/tf)*2*math.pi; tdy=int(abs(math.sin(ph))*2)-1
        ldx=int(math.sin(ph)*4); rdx=int(math.sin(ph+math.pi)*4)
    elif st=="attack":
        if f<2: atk="raise"; ady=-4-f*2
        elif f<3: atk="swing"; adx=6; ady=3; tdy=2
        elif f<4: atk="ext"; adx=8; ady=4; tdy=1
        else: t=(f-4)/2.0; adx=int(8*(1-t)); ady=int(4*(1-t))
    # Cape
    dr(d,cx-12,by-4+tdy,6,22,W_CAPE,W_OL)
    # Legs
    dr(d,cx-7+ldx,by+14+tdy,6,14,W_ARM2,W_OL)
    dr(d,cx+1+rdx,by+14+tdy,6,14,W_ARM2,W_OL)
    dr(d,cx-8+ldx,by+25+tdy,7,4,W_ARM,W_OL)
    dr(d,cx+0+rdx,by+25+tdy,7,4,W_ARM,W_OL)
    # Torso
    dr(d,cx-8,by-8+tdy,16,20,W_ARM,W_OL)
    dr(d,cx-4,by-4+tdy,8,12,W_ARM2)
    dr(d,cx-8,by+10+tdy,16,3,W_GOLD,W_OL)
    # Shoulders
    dr(d,cx-12,by-10+tdy,6,6,W_ARM,W_OL)
    dr(d,cx+6,by-10+tdy,6,6,W_ARM,W_OL)
    # Head
    hy=by-18+tdy
    dr(d,cx-5,hy,10,10,W_SKIN,W_OL)
    dp(d,cx-2,hy+4,W_EYE); dp(d,cx+3,hy+4,W_EYE)
    dr(d,cx-6,hy,12,3,W_ARM,W_OL) # helmet
    dr(d,cx-3,hy-2,6,3,W_ARM,W_OL) # helmet crest
    # Horns
    dp(d,cx-6,hy-2,W_WOOD); dp(d,cx+6,hy-2,W_WOOD)
    # Right arm + axe
    rax=cx+8+adx; ray=by-4+ady+tdy
    dr(d,rax,ray,5,12,W_SKIN,W_OL)
    dr(d,rax-1,ray-1,6,4,W_ARM,W_OL)
    if atk=="raise":
        dr(d,rax+1,ray-16,2,18,W_WOOD,W_OL)
        dr(d,rax-2,ray-18,8,6,W_AXE,W_OL)
    elif atk in ["swing","ext"]:
        dr(d,rax+4,ray+6,14,2,W_WOOD,W_OL)
        dr(d,rax+14,ray+2,6,10,W_AXE,W_OL)
    else:
        dr(d,rax+1,ray+12,2,14,W_WOOD,W_OL)
        dr(d,rax-2,ray+10,8,8,W_AXE,W_OL)
    # Left arm
    dr(d,cx-12,by-2+tdy,5,10,W_SKIN,W_OL)
    return img

# === KNIGHT (52x60) — Full plate, sword+shield, golden accents ===
K_OL=(10,10,10,255); K_PLATE=(160,160,170,255); K_PLATE2=(120,120,135,255)
K_GOLD=(218,165,32,255); K_CAPE=(30,50,120,255); K_SWORD=(190,190,200,255)
K_SHIELD=(40,60,100,255); K_EYE=(100,200,255,255)

def build_knight(st,f,tf=8):
    img=ct(52,60); d=ImageDraw.Draw(img); cx,by=26,30
    tdy=0; ldx=0; rdx=0; adx=0; ady=0; atk="rest"
    if st=="idle":
        ph=(f/tf)*2*math.pi; tdy=int(math.sin(ph)*1.0)
    elif st=="run":
        ph=(f/tf)*2*math.pi; tdy=int(abs(math.sin(ph))*2)-1
        ldx=int(math.sin(ph)*3); rdx=int(math.sin(ph+math.pi)*3)
    elif st=="attack":
        if f<2: atk="raise"; ady=-5-f*2
        elif f<3: atk="swing"; adx=6; ady=2; tdy=2
        elif f<4: atk="ext"; adx=8; ady=3
        else: t=(f-4)/2.0; adx=int(8*(1-t)); ady=int(3*(1-t))
    # Cape
    dr(d,cx-14,by-2+tdy,5,24,K_CAPE,K_OL)
    # Legs
    dr(d,cx-7+ldx,by+14+tdy,5,14,K_PLATE2,K_OL)
    dr(d,cx+2+rdx,by+14+tdy,5,14,K_PLATE2,K_OL)
    dr(d,cx-8+ldx,by+26+tdy,7,4,K_PLATE,K_OL)
    dr(d,cx+1+rdx,by+26+tdy,7,4,K_PLATE,K_OL)
    # Torso
    dr(d,cx-9,by-10+tdy,18,22,K_PLATE,K_OL)
    dr(d,cx-3,by-6+tdy,6,14,K_GOLD)
    dr(d,cx-5,by-2+tdy,10,2,K_GOLD)
    dr(d,cx-9,by+10+tdy,18,3,K_GOLD,K_OL)
    # Pauldrons
    dr(d,cx-13,by-12+tdy,7,7,K_PLATE,K_OL)
    dr(d,cx+6,by-12+tdy,7,7,K_PLATE,K_OL)
    # Head
    hy=by-20+tdy
    dr(d,cx-5,hy,10,10,K_PLATE,K_OL)
    dr(d,cx+1,hy+4,4,2,K_OL) # visor
    dr(d,cx-2,hy-2,4,3,K_GOLD,K_OL) # crest
    dp(d,cx+2,hy+4,K_EYE)
    # Shield left
    lax=cx-13; lay=by-4+tdy
    dr(d,lax,lay,6,12,K_SHIELD,K_OL)
    dr(d,lax+1,lay+2,4,8,K_CAPE)
    dr(d,lax+2,lay+4,2,4,K_GOLD)
    # Sword right
    rax=cx+9+adx; ray=by-4+ady+tdy
    dr(d,rax,ray,5,10,K_PLATE2,K_OL)
    if atk=="raise":
        dr(d,rax+1,ray-14,2,16,K_SWORD,K_OL)
        dp(d,rax+1,ray-14,(255,255,255,200))
        dr(d,rax,ray-1,4,3,K_GOLD)
    elif atk in ["swing","ext"]:
        dr(d,rax+4,ray+3,12,2,K_SWORD,K_OL)
        dp(d,rax+15,ray+3,(255,255,255,200))
        dr(d,rax+1,ray+2,4,4,K_GOLD)
    else:
        dr(d,rax+1,ray+10,2,14,K_SWORD,K_OL)
        dp(d,rax+1,ray+23,(255,255,255,180))
        dr(d,rax,ray+8,4,3,K_GOLD)
    return img

# === ROGUE (40x52) — Slim, hooded, dual daggers, purple ===
R_OL=(10,10,10,255); R_CLOTH=(40,25,60,255); R_CLOTH2=(25,15,40,255)
R_SKIN=(160,130,110,255); R_HOOD=(60,30,80,255)
R_EYE=(255,100,50,255); R_DAG=(180,180,190,255); R_GLOW=(255,120,40,255)

def build_rogue(st,f,tf=8):
    img=ct(40,52); d=ImageDraw.Draw(img); cx,by=20,26
    tdy=0; ldx=0; rdx=0; aldx=0; ardx=0; aldy=0; ardy=0; slash=False
    if st=="idle":
        ph=(f/tf)*2*math.pi; tdy=int(math.sin(ph)*0.8)
        aldx=int(math.sin(ph+1)*1); ardx=int(math.sin(ph-1)*1)
    elif st=="run":
        ph=(f/tf)*2*math.pi; tdy=int(abs(math.sin(ph))*2)-1
        ldx=int(math.sin(ph)*4); rdx=int(math.sin(ph+math.pi)*4)
        aldx=2; ardx=2
    elif st=="attack":
        if f<1: aldx=-3; ardx=-3; aldy=-2; ardy=-2
        elif f<3: aldx=5; ardx=5; aldy=1; ardy=-1; slash=True
        elif f<4: aldx=3; ardx=-3; slash=True
        else: t=(f-4)/2.0; aldx=int(3*(1-t)); ardx=int(3*(1-t))
    # Cape
    dr(d,cx-10,by+2+tdy,5,18,R_HOOD,R_OL)
    # Legs
    dr(d,cx-5+ldx,by+12+tdy,3,12,R_SKIN,R_OL)
    dr(d,cx+2+rdx,by+12+tdy,3,12,R_SKIN,R_OL)
    dr(d,cx-6+ldx,by+22+tdy,5,3,R_CLOTH2,R_OL)
    dr(d,cx+1+rdx,by+22+tdy,5,3,R_CLOTH2,R_OL)
    # Torso
    dr(d,cx-6,by-6+tdy,12,16,R_CLOTH,R_OL)
    dr(d,cx-2,by-2+tdy,4,10,R_CLOTH2)
    dr(d,cx-6,by+8+tdy,12,2,R_CLOTH2,R_OL)
    # Head
    hy=by-14+tdy
    dr(d,cx-4,hy,8,8,R_HOOD,R_OL)
    dr(d,cx-3,hy-2,6,3,R_HOOD,R_OL)
    dr(d,cx-2,hy+4,4,3,(15,10,10,255))
    dp(d,cx-1,hy+5,R_EYE); dp(d,cx+2,hy+5,R_EYE)
    # Left arm + dagger
    lax=cx-8+aldx; lay=by-2+aldy+tdy
    dr(d,lax,lay,3,8,R_SKIN,R_OL)
    dr(d,lax-1,lay+8,2,6,R_DAG); dp(d,lax-1,lay+13,R_GLOW)
    # Right arm + dagger
    rax=cx+5+ardx; ray=by-2+ardy+tdy
    dr(d,rax,ray,3,8,R_SKIN,R_OL)
    dr(d,rax+2,ray+8,2,6,R_DAG); dp(d,rax+2,ray+13,R_GLOW)
    if slash:
        d.line([(cx+4,by-2),(cx+14,by+8)],fill=(255,255,255,160),width=1)
        d.line([(cx+14,by-2),(cx+4,by+8)],fill=(255,255,255,160),width=1)
    return img

def assemble(builder,st,tf,fw,fh):
    s=ct(fw*tf,fh)
    for i in range(tf): s.paste(builder(st,i,tf),(i*fw,0))
    return s

def gen(name,builder,fw,fh):
    print(f"  {name} ({fw}x{fh}):")
    for st,nf in [("idle",8),("run",8),("attack",6)]:
        sh=assemble(builder,st,nf,fw,fh)
        sh.save(os.path.join(OUT,f"{name}_{st}.png"))
        print(f"    {st}: {sh.size[0]}x{sh.size[1]} ({nf}f)")

def main():
    print("INFERNUS -- Player Sprites Upgrade")
    print("="*50)
    gen("warrior",build_warrior,48,56)
    gen("knight",build_knight,52,60)
    gen("rogue",build_rogue,40,52)
    print("="*50)
    print("3 clases x 3 clips = 9 spritesheets")

if __name__=="__main__": main()
