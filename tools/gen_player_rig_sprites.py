#!/usr/bin/env python3
import os
import math
from PIL import Image, ImageDraw

OUT_DIR = "assets/sprites/player"
CELL_W, CELL_H = 32, 48

def ensure_dir():
    os.makedirs(OUT_DIR, exist_ok=True)

def create_sheet(frames, is_run=False):
    count = 8 if is_run else 6
    w = count * CELL_W
    img = Image.new("RGBA", (w, CELL_H), (0,0,0,0))
    return img

class PuppetRig:
    def __init__(self, cls_name):
        self.cls = cls_name
        self.cx = CELL_W // 2
        self.cy = 38 # ground level
        
    def draw_frame(self, draw, dx, frame, anim_type):
        anim_factor = 0
        if anim_type == "idle":
            # Breathing: frames 1-3 up 1px, 4-6 down 1px
            anim_factor = -1 if frame < 3 else 0
            
        elif anim_type == "run":
            # Leg cycle
            anim_factor = -1 if frame % 2 == 0 else -2
            
        elif anim_type == "attack":
            # Windup -> swing -> recover
            if frame < 2: anim_factor = -1  # windup
            elif frame < 4: anim_factor = 0 # swing
            else: anim_factor = 0           # recover

        hx = dx + self.cx
        by = self.cy + anim_factor - 18 # base body Y

        if self.cls == "knight":
            self.draw_knight(draw, hx, by, frame, anim_type)
        elif self.cls == "warrior":
            self.draw_warrior(draw, hx, by, frame, anim_type)
        elif self.cls == "rogue":
            self.draw_rogue(draw, hx, by, frame, anim_type)

    def draw_knight(self, draw, cx, by, frame, anim):
        # 16px wide silhouette
        c_armor = (160,165,170,255)
        c_dark = (60,60,65,255)
        c_gold = (200,170,60,255)
        c_cape = (139,0,0,255)
        c_out = (15,10,10,255)

        # Cape
        draw.rectangle([cx-6, by, cx-3, by+15], fill=c_cape)

        # Legs (Run vs Idle)
        ly = self.cy - 6
        lx, rx = cx - 4, cx + 2
        l_off, r_off = 0, 0
        if anim == "run":
            cycle = [0, 2, 4, 2, 0, -2, -4, -2]
            l_off = cycle[frame]
            r_off = cycle[(frame+4)%8]
        draw.rectangle([lx+l_off, ly, lx+3+l_off, self.cy], fill=c_armor, outline=c_out)
        draw.rectangle([rx+r_off, ly, rx+3+r_off, self.cy], fill=c_armor, outline=c_out)

        # Torso (8x10)
        draw.rectangle([cx-4, by, cx+4, by+10], fill=c_armor, outline=c_out)
        draw.line([cx, by+2, cx, by+8], fill=c_gold) # cross vertical
        draw.line([cx-2, by+4, cx+2, by+4], fill=c_gold) # cross horiz

        # Head (4x5)
        hy = by - 6
        draw.rectangle([cx-2, hy, cx+2, hy+5], fill=c_armor, outline=c_out)
        draw.line([cx-2, hy+2, cx+2, hy+2], fill=c_dark) # visor

        # Attack logic for sword
        sword_y = by+2
        sword_x = cx+6
        if anim == "attack":
            if frame < 2: # windup
                sword_y -= 8
                draw.rectangle([sword_x, sword_y, sword_x+2, sword_y+10], fill=c_armor, outline=c_out)
            elif frame < 4: # swing 
                # horizontal
                draw.rectangle([cx+5, by+4, cx+15, by+6], fill=c_armor, outline=c_out)
            else: # recover
                draw.rectangle([sword_x, sword_y, sword_x+2, sword_y+10], fill=c_armor, outline=c_out)
        else:
            draw.rectangle([sword_x-1, sword_y, sword_x+1, sword_y+10], fill=c_armor, outline=c_out)

        # Shield
        draw.rectangle([cx-8, by+2, cx-3, by+10], fill=c_dark, outline=c_out)
        if anim == "run":   
             draw.rectangle([cx-8, by+2, cx-3, by+10], fill=c_dark, outline=c_out)

    def draw_warrior(self, draw, cx, by, frame, anim):
        # 20px wide silhouette
        c_armor = (100,20,20,255)
        c_dark = (40,10,10,255)
        c_horn = (180,160,140,255)
        c_skin = (195,150,120,255)
        c_wep = (120,120,130,255)
        c_out = (10,5,5,255)

        # Legs (Thick)
        ly = self.cy - 8
        lx, rx = cx - 6, cx + 2
        l_off, r_off = 0, 0
        if anim == "run":
            cycle = [0, 3, 5, 3, 0, -3, -5, -3]
            l_off = cycle[frame]
            r_off = cycle[(frame+4)%8]
        draw.rectangle([lx+l_off, ly, lx+5+l_off, self.cy], fill=c_armor, outline=c_out)
        draw.rectangle([rx+r_off, ly, rx+5+r_off, self.cy], fill=c_armor, outline=c_out)

        # Torso (12x12)
        draw.rectangle([cx-6, by, cx+6, by+12], fill=c_armor, outline=c_out)
        # Shoulders
        draw.rectangle([cx-8, by-1, cx-4, by+2], fill=c_dark, outline=c_out)
        draw.rectangle([cx+4, by-1, cx+8, by+2], fill=c_dark, outline=c_out)

        # Head (5x5) + Horns
        hy = by - 6
        draw.rectangle([cx-3, hy, cx+3, hy+5], fill=c_skin, outline=c_out)
        draw.rectangle([cx-2, hy-2, cx+2, hy], fill=c_dark) # helmet top
        draw.point([cx-4, hy-2, cx-5, hy-3], fill=c_horn) # left horn
        draw.point([cx+4, hy-2, cx+5, hy-3], fill=c_horn) # right horn

        # Huge Axe
        w_x, w_y = cx+8, by-2
        if anim == "attack":
            if frame < 2: # high windup overhead
                draw.rectangle([cx-2, by-14, cx, by], fill=(60,40,20,255)) # handle
                draw.rectangle([cx-6, by-16, cx+4, by-10], fill=c_wep, outline=c_out)
            elif frame < 4: # smash down
                draw.rectangle([cx+8, by+4, cx+18, by+6], fill=(60,40,20,255)) # handle right
                draw.rectangle([cx+14, by, cx+20, by+10], fill=c_wep, outline=c_out)
            else: # ground
                draw.rectangle([cx+6, by+8, cx+14, by+10], fill=(60,40,20,255))
                draw.rectangle([cx+12, by+4, cx+18, by+14], fill=c_wep, outline=c_out)
        else:
            # Idle/Run Rest
            draw.rectangle([w_x, w_y, w_x+2, w_y+16], fill=(60,40,20,255), outline=c_out) # handle
            draw.rectangle([w_x-2, w_y-4, w_x+4, w_y], fill=c_wep, outline=c_out) # axe tip T

    def draw_rogue(self, draw, cx, by, frame, anim):
        # 12px wide silhouette
        c_robe = (40, 20, 50, 255)
        c_dark = (20, 10, 25, 255)
        c_dag = (220, 80, 40, 255) # infernal dagger
        c_skin = (195,150,120,255)
        c_out = (10,5,5,255)

        # Long Cape
        draw.rectangle([cx-5, by, cx-3, self.cy-2], fill=c_dark)

        # Legs (Thin)
        ly = self.cy - 6
        lx, rx = cx - 3, cx + 1
        l_off, r_off = 0, 0
        if anim == "run":
            cycle = [0, 4, 6, 4, 0, -4, -6, -4]
            l_off = cycle[frame]
            r_off = cycle[(frame+4)%8]
        draw.rectangle([lx+l_off, ly, lx+2+l_off, self.cy], fill=c_robe, outline=c_out)
        draw.rectangle([rx+r_off, ly, rx+2+r_off, self.cy], fill=c_robe, outline=c_out)

        # Torso (6x9)
        draw.rectangle([cx-3, by, cx+3, by+9], fill=c_robe, outline=c_out)

        # Head (Hood 4x6)
        hy = by - 5
        draw.rectangle([cx-2, hy, cx+2, hy+5], fill=c_robe, outline=c_out)
        draw.point([cx-2, hy-1, cx-1, hy-2], fill=c_robe) # hood point
        draw.rectangle([cx-1, hy+2, cx+1, hy+3], fill=c_skin) # face peek

        # Daggers (Dual)
        dag_anim = 0
        if anim == "idle":
            dag_anim = 1 if frame > 2 else 0

        if anim == "attack":
            if frame < 2: # cross
                draw.line([cx-4, by+2, cx-8, by-2], fill=c_dag, width=2)
                draw.line([cx+4, by+2, cx+8, by-2], fill=c_dag, width=2)
            elif frame < 4: # slash out
                draw.line([cx-4, by+4, cx-12, by+4], fill=c_dag, width=3)
                draw.line([cx+4, by+4, cx+12, by+4], fill=c_dag, width=3)
            else: # recover
                draw.line([cx-4, by+6, cx-8, by+10], fill=c_dag, width=2)
                draw.line([cx+4, by+6, cx+8, by+10], fill=c_dag, width=2)
        else:
            # Idle/Run Rest
            draw.line([cx-4, by+4+dag_anim, cx-7, by+7+dag_anim], fill=c_dag, width=2) # left
            draw.line([cx+4, by+4+dag_anim, cx+7, by+7+dag_anim], fill=c_dag, width=2) # right

def main():
    ensure_dir()
    classes = ["knight", "warrior", "rogue"]
    anims = ["idle", "run", "attack"]

    for c in classes:
        rig = PuppetRig(c)
        for a in anims:
            is_r = (a == "run")
            img = create_sheet(frames=None, is_run=is_r)
            draw = ImageDraw.Draw(img)
            
            f_count = 8 if is_r else 6
            for f in range(f_count):
                dx = f * CELL_W
                rig.draw_frame(draw, dx, f, a)
                
            outpath = os.path.join(OUT_DIR, f"{c}_{a}.png")
            img.save(outpath)
            print(f"Generated {outpath}")

if __name__ == "__main__":
    main()
