"""
INFERNUS -- Fase 4A: Generador de sprites de enemigos mejorados
================================================================
Genera spritesheets con 8-10 frames por clip para los 5 tipos de enemigos.
Usa el mismo pipeline procedural Pillow que funciono para los player sprites.

Prioridad: melee > ranged > assassin > tank > bomber
Referencia de calidad: minotaur (silueta clara, animacion visible)

Autor: Antigravity
Fecha: 2026-05-06
"""

from PIL import Image, ImageDraw
import os
import math

OUTPUT_DIR = "assets/sprites/enemies"
os.makedirs(OUTPUT_DIR, exist_ok=True)


def create_transparent(w, h):
    return Image.new("RGBA", (w, h), (0, 0, 0, 0))


def draw_rect(draw, x, y, w, h, fill, outline=None):
    if w <= 0 or h <= 0:
        return
    draw.rectangle([x, y, x + w - 1, y + h - 1], fill=fill, outline=outline)


def draw_pixel(draw, x, y, color):
    draw.point((x, y), fill=color)


# ========================================================================
#  MELEE ENEMY: "Alma Violenta" — humanoid infernal soldier with sword
#  32x48 per frame, 8 frames per clip
# ========================================================================

# Palette
MELEE_OUTLINE = (10, 10, 10, 255)
MELEE_SKIN = (120, 50, 40, 255)       # Reddish infernal skin
MELEE_SKIN_DARK = (80, 35, 25, 255)
MELEE_ARMOR = (60, 30, 20, 255)       # Dark leather armor
MELEE_ARMOR_LIGHT = (80, 45, 30, 255)
MELEE_LOINCLOTH = (100, 30, 20, 255)  # Dark red cloth
MELEE_EYE = (255, 160, 0, 255)        # Orange glowing eyes
MELEE_SWORD = (160, 160, 170, 255)    # Steel
MELEE_SWORD_EDGE = (200, 200, 210, 255)
MELEE_HILT = (80, 50, 20, 255)        # Brown


def build_melee(state, frame, total_frames=8):
    FW, FH = 32, 48
    img = create_transparent(FW, FH)
    d = ImageDraw.Draw(img)

    cx = 16  # center x
    base_y = 24  # torso center y

    # Animation parameters
    torso_dy = 0
    head_dy = 0
    leg_l_dx, leg_r_dx = 0, 0
    arm_l_dx, arm_l_dy = 0, 0
    arm_r_dx, arm_r_dy = 0, 0
    sword_state = "side"  # side, raised, swing, extended
    body_lean = 0

    if state == "idle":
        # Breathing cycle: subtle up/down
        phase = (frame / total_frames) * 2 * math.pi
        torso_dy = int(math.sin(phase) * 1.0)
        head_dy = torso_dy
        # Arms sway slightly
        arm_l_dy = int(math.sin(phase + 0.5) * 1)
        arm_r_dy = int(math.sin(phase - 0.5) * 1)

    elif state == "run":
        # Running cycle: bob + leg alternation
        phase = (frame / total_frames) * 2 * math.pi
        torso_dy = int(abs(math.sin(phase)) * 2) - 1
        head_dy = torso_dy - 1

        # Legs: alternate forward/back
        stride = 3
        leg_l_dx = int(math.sin(phase) * stride)
        leg_r_dx = int(math.sin(phase + math.pi) * stride)

        # Arms: opposite to legs
        arm_l_dx = -leg_l_dx // 2
        arm_r_dx = -leg_r_dx // 2
        body_lean = 2

    elif state == "attack":
        # 8-frame attack: 2 windup + 2 active + 2 hold + 2 recovery
        if frame < 2:
            # WINDUP: pull sword back, lean back
            sword_state = "raised"
            torso_dy = -1
            arm_r_dy = -3 - frame
            arm_r_dx = -2
            body_lean = -1
        elif frame < 4:
            # ACTIVE: swing forward
            sword_state = "swing"
            torso_dy = 1
            arm_r_dx = 4 + (frame - 2) * 2
            arm_r_dy = 2
            body_lean = 3
        elif frame < 6:
            # HOLD: extended position
            sword_state = "extended"
            torso_dy = 1
            arm_r_dx = 6
            arm_r_dy = 3
            body_lean = 2
        else:
            # RECOVERY: return to neutral
            t = (frame - 6) / 2.0
            sword_state = "side"
            arm_r_dx = int(6 * (1 - t))
            arm_r_dy = int(3 * (1 - t))
            body_lean = int(2 * (1 - t))

    # --- DRAW ORDER: back arm, legs, torso, head, front arm + sword ---

    # LEGS
    leg_y = base_y + 10
    leg_h = 12
    # Left leg
    lx = cx - 5 + leg_l_dx
    draw_rect(d, lx, leg_y + torso_dy, 4, leg_h, MELEE_SKIN_DARK, MELEE_OUTLINE)
    # Boot
    draw_rect(d, lx - 1, leg_y + leg_h - 3 + torso_dy, 5, 3, MELEE_ARMOR, MELEE_OUTLINE)
    # Right leg
    rx = cx + 1 + leg_r_dx
    draw_rect(d, rx, leg_y + torso_dy, 4, leg_h, MELEE_SKIN_DARK, MELEE_OUTLINE)
    draw_rect(d, rx - 1, leg_y + leg_h - 3 + torso_dy, 5, 3, MELEE_ARMOR, MELEE_OUTLINE)

    # Loincloth
    draw_rect(d, cx - 5, leg_y - 1 + torso_dy, 10, 4, MELEE_LOINCLOTH, MELEE_OUTLINE)

    # TORSO
    torso_x = cx - 6 + body_lean
    torso_y_pos = base_y - 6 + torso_dy
    draw_rect(d, torso_x, torso_y_pos, 12, 14, MELEE_ARMOR, MELEE_OUTLINE)
    # Chest detail: leather straps
    draw_rect(d, torso_x + 2, torso_y_pos + 2, 2, 10, MELEE_ARMOR_LIGHT)
    draw_rect(d, torso_x + 8, torso_y_pos + 2, 2, 10, MELEE_ARMOR_LIGHT)
    # Belt
    draw_rect(d, torso_x, torso_y_pos + 12, 12, 2, MELEE_HILT, MELEE_OUTLINE)

    # HEAD
    head_x = cx - 4 + body_lean
    head_y = base_y - 14 + head_dy
    draw_rect(d, head_x, head_y, 8, 8, MELEE_SKIN, MELEE_OUTLINE)
    # Eyes (glowing orange)
    draw_pixel(d, head_x + 2, head_y + 3, MELEE_EYE)
    draw_pixel(d, head_x + 5, head_y + 3, MELEE_EYE)
    # Brow ridge
    draw_rect(d, head_x + 1, head_y + 1, 6, 2, MELEE_SKIN_DARK)
    # Small horns/spikes
    draw_pixel(d, head_x, head_y - 1, MELEE_SKIN_DARK)
    draw_pixel(d, head_x + 7, head_y - 1, MELEE_SKIN_DARK)

    # LEFT ARM (back)
    la_x = torso_x - 3 + arm_l_dx
    la_y = torso_y_pos + 2 + arm_l_dy
    draw_rect(d, la_x, la_y, 4, 10, MELEE_SKIN, MELEE_OUTLINE)
    # Shoulder pad
    draw_rect(d, la_x - 1, la_y - 1, 5, 3, MELEE_ARMOR, MELEE_OUTLINE)

    # RIGHT ARM + SWORD (front)
    ra_x = torso_x + 11 + arm_r_dx
    ra_y = torso_y_pos + 2 + arm_r_dy
    draw_rect(d, ra_x, ra_y, 4, 10, MELEE_SKIN, MELEE_OUTLINE)
    draw_rect(d, ra_x - 1, ra_y - 1, 5, 3, MELEE_ARMOR, MELEE_OUTLINE)

    # SWORD
    sx = ra_x + 1
    sy = ra_y + 10
    if sword_state == "side":
        # Sword pointing down
        draw_rect(d, sx, sy, 2, 10, MELEE_SWORD, MELEE_OUTLINE)
        draw_pixel(d, sx, sy + 9, MELEE_SWORD_EDGE)
        draw_rect(d, sx - 1, sy - 2, 4, 2, MELEE_HILT)
    elif sword_state == "raised":
        # Sword pointing up
        draw_rect(d, sx, ra_y - 10, 2, 12, MELEE_SWORD, MELEE_OUTLINE)
        draw_pixel(d, sx, ra_y - 10, MELEE_SWORD_EDGE)
        draw_rect(d, sx - 1, ra_y - 1, 4, 2, MELEE_HILT)
    elif sword_state == "swing":
        # Sword horizontal forward
        draw_rect(d, sx + 2, ra_y + 4, 10, 2, MELEE_SWORD, MELEE_OUTLINE)
        draw_pixel(d, sx + 11, ra_y + 4, MELEE_SWORD_EDGE)
        draw_rect(d, sx, ra_y + 3, 3, 4, MELEE_HILT)
    elif sword_state == "extended":
        # Sword angled down-forward
        draw_rect(d, sx + 2, ra_y + 5, 8, 2, MELEE_SWORD, MELEE_OUTLINE)
        draw_rect(d, sx + 8, ra_y + 6, 2, 4, MELEE_SWORD, MELEE_OUTLINE)
        draw_rect(d, sx, ra_y + 4, 3, 4, MELEE_HILT)

    return img


# ========================================================================
#  RANGED ENEMY: "Esbirro de Fuego" — hooded figure with fire staff
#  28x40 per frame, 8 frames per clip
# ========================================================================

RANGED_OUTLINE = (10, 10, 10, 255)
RANGED_ROBE = (70, 25, 15, 255)       # Dark red robe
RANGED_ROBE_DARK = (45, 18, 10, 255)
RANGED_SKIN = (90, 60, 50, 255)
RANGED_HOOD = (50, 20, 10, 255)
RANGED_EYE = (255, 200, 50, 255)      # Yellow glow
RANGED_STAFF = (60, 40, 25, 255)
RANGED_ORB = (255, 100, 20, 255)      # Fire orb
RANGED_ORB_GLOW = (255, 180, 50, 255)


def build_ranged(state, frame, total_frames=8):
    FW, FH = 28, 40
    img = create_transparent(FW, FH)
    d = ImageDraw.Draw(img)

    cx = 14
    base_y = 18

    torso_dy = 0
    head_dy = 0
    arm_dy = 0
    staff_glow = 1.0
    cast_effect = False

    if state == "idle":
        phase = (frame / total_frames) * 2 * math.pi
        torso_dy = int(math.sin(phase) * 0.8)
        head_dy = torso_dy
        # Staff orb pulses
        staff_glow = 0.5 + 0.5 * math.sin(phase * 2)

    elif state == "run":
        phase = (frame / total_frames) * 2 * math.pi
        torso_dy = int(abs(math.sin(phase)) * 1.5) - 1
        head_dy = torso_dy

    elif state == "attack":
        # 2 channel + 2 cast + 2 release + 2 recover
        if frame < 2:
            arm_dy = -2 - frame
            staff_glow = 0.5 + frame * 0.25
        elif frame < 4:
            arm_dy = -4
            staff_glow = 1.0
            cast_effect = True
        elif frame < 6:
            arm_dy = -2
            staff_glow = 0.8 - (frame - 4) * 0.3
            cast_effect = frame == 4
        else:
            arm_dy = 0
            staff_glow = 0.3

    # ROBE (body + legs as one flowing piece)
    robe_y = base_y + torso_dy
    # Main robe body
    draw_rect(d, cx - 5, robe_y, 10, 18, RANGED_ROBE, RANGED_OUTLINE)
    # Robe flare at bottom
    draw_rect(d, cx - 7, robe_y + 14, 14, 6, RANGED_ROBE, RANGED_OUTLINE)
    # Dark center fold
    draw_rect(d, cx - 1, robe_y + 2, 2, 14, RANGED_ROBE_DARK)

    # HEAD + HOOD
    head_y = base_y - 8 + head_dy
    draw_rect(d, cx - 4, head_y, 8, 8, RANGED_HOOD, RANGED_OUTLINE)
    # Hood peak
    draw_rect(d, cx - 2, head_y - 2, 4, 3, RANGED_HOOD, RANGED_OUTLINE)
    # Face shadow
    draw_rect(d, cx - 2, head_y + 3, 4, 4, (20, 10, 5, 255))
    # Eyes
    draw_pixel(d, cx - 1, head_y + 4, RANGED_EYE)
    draw_pixel(d, cx + 2, head_y + 4, RANGED_EYE)

    # STAFF (right side)
    staff_x = cx + 5
    staff_y = base_y - 6 + arm_dy + torso_dy
    # Staff pole
    draw_rect(d, staff_x, staff_y, 2, 24, RANGED_STAFF, RANGED_OUTLINE)
    # Staff orb
    orb_r = max(0, min(255, int(RANGED_ORB[0] * staff_glow)))
    orb_g = max(0, min(255, int(RANGED_ORB[1] * staff_glow)))
    orb_b = max(0, min(255, int(RANGED_ORB[2] * staff_glow)))
    draw_rect(d, staff_x - 1, staff_y - 3, 4, 4, (orb_r, orb_g, orb_b, 255))
    # Glow pixel
    if staff_glow > 0.7:
        draw_pixel(d, staff_x, staff_y - 2, RANGED_ORB_GLOW)

    # ARMS (holding staff)
    draw_rect(d, cx + 3, base_y + 2 + arm_dy + torso_dy, 3, 5, RANGED_SKIN, RANGED_OUTLINE)
    draw_rect(d, cx - 5, base_y + 4 + torso_dy, 3, 4, RANGED_SKIN, RANGED_OUTLINE)

    # Cast effect: fire burst
    if cast_effect:
        for dx in [-3, -1, 1, 3]:
            draw_pixel(d, staff_x + dx, staff_y - 5, RANGED_ORB_GLOW)
        for dx in [-2, 0, 2]:
            draw_pixel(d, staff_x + dx, staff_y - 6, RANGED_ORB)

    return img


# ========================================================================
#  ASSASSIN: "Sombra Asesina" — slim hooded figure with dual daggers
#  24x40 per frame, 8 frames per clip
# ========================================================================

ASSASSIN_OUTLINE = (10, 10, 10, 255)
ASSASSIN_CLOTH = (35, 20, 50, 255)      # Dark purple
ASSASSIN_CLOTH_DARK = (20, 12, 30, 255)
ASSASSIN_SKIN = (70, 55, 70, 255)       # Pale purple-gray
ASSASSIN_EYE = (180, 50, 255, 255)      # Purple glow
ASSASSIN_DAGGER = (170, 170, 180, 255)
ASSASSIN_POISON = (100, 255, 50, 255)   # Toxic green glow


def build_assassin(state, frame, total_frames=8):
    FW, FH = 24, 40
    img = create_transparent(FW, FH)
    d = ImageDraw.Draw(img)

    cx = 12
    base_y = 18

    torso_dy = 0
    leg_l_dx, leg_r_dx = 0, 0
    arm_l_dx, arm_r_dx = 0, 0
    arm_l_dy, arm_r_dy = 0, 0
    slash_effect = False

    if state == "idle":
        phase = (frame / total_frames) * 2 * math.pi
        torso_dy = int(math.sin(phase) * 0.8)
        # Subtle sway
        arm_l_dx = int(math.sin(phase + 1) * 1)
        arm_r_dx = int(math.sin(phase - 1) * 1)

    elif state == "run":
        phase = (frame / total_frames) * 2 * math.pi
        torso_dy = int(abs(math.sin(phase)) * 2) - 1
        stride = 3
        leg_l_dx = int(math.sin(phase) * stride)
        leg_r_dx = int(math.sin(phase + math.pi) * stride)
        # Leaning forward
        arm_l_dx = 2
        arm_r_dx = 2

    elif state == "attack":
        # Fast slash: 1 wind + 2 slash + 1 cross + 2 hold + 2 recover
        if frame < 1:
            arm_l_dx = -2; arm_r_dx = -2
            arm_l_dy = -2; arm_r_dy = -2
        elif frame < 3:
            arm_l_dx = 4; arm_r_dx = 4
            arm_l_dy = 1; arm_r_dy = -1
            slash_effect = True
        elif frame < 4:
            arm_l_dx = 3; arm_r_dx = -3
            slash_effect = True
        elif frame < 6:
            arm_l_dx = 2; arm_r_dx = 2
        else:
            t = (frame - 6) / 2.0
            arm_l_dx = int(2 * (1 - t))
            arm_r_dx = int(2 * (1 - t))

    # LEGS (slim)
    leg_y = base_y + 10 + torso_dy
    draw_rect(d, cx - 3 + leg_l_dx, leg_y, 2, 10, ASSASSIN_SKIN, ASSASSIN_OUTLINE)
    draw_rect(d, cx + 1 + leg_r_dx, leg_y, 2, 10, ASSASSIN_SKIN, ASSASSIN_OUTLINE)
    # Boots
    draw_rect(d, cx - 4 + leg_l_dx, leg_y + 8, 4, 3, ASSASSIN_CLOTH_DARK, ASSASSIN_OUTLINE)
    draw_rect(d, cx + 0 + leg_r_dx, leg_y + 8, 4, 3, ASSASSIN_CLOTH_DARK, ASSASSIN_OUTLINE)

    # TORSO (slim)
    draw_rect(d, cx - 4, base_y - 4 + torso_dy, 8, 12, ASSASSIN_CLOTH, ASSASSIN_OUTLINE)
    # Belt
    draw_rect(d, cx - 4, base_y + 6 + torso_dy, 8, 2, ASSASSIN_CLOTH_DARK, ASSASSIN_OUTLINE)

    # HEAD + HOOD
    head_y = base_y - 10 + torso_dy
    draw_rect(d, cx - 3, head_y, 6, 6, ASSASSIN_CLOTH, ASSASSIN_OUTLINE)
    draw_rect(d, cx - 2, head_y - 2, 4, 3, ASSASSIN_CLOTH, ASSASSIN_OUTLINE)
    # Eyes
    draw_pixel(d, cx - 1, head_y + 3, ASSASSIN_EYE)
    draw_pixel(d, cx + 2, head_y + 3, ASSASSIN_EYE)

    # ARMS + DAGGERS
    la_x = cx - 5 + arm_l_dx
    la_y = base_y + arm_l_dy + torso_dy
    draw_rect(d, la_x, la_y, 2, 7, ASSASSIN_SKIN, ASSASSIN_OUTLINE)
    # Left dagger
    draw_rect(d, la_x - 1, la_y + 7, 1, 5, ASSASSIN_DAGGER)
    draw_pixel(d, la_x - 1, la_y + 11, ASSASSIN_POISON)

    ra_x = cx + 3 + arm_r_dx
    ra_y = base_y + arm_r_dy + torso_dy
    draw_rect(d, ra_x, ra_y, 2, 7, ASSASSIN_SKIN, ASSASSIN_OUTLINE)
    # Right dagger
    draw_rect(d, ra_x + 2, ra_y + 7, 1, 5, ASSASSIN_DAGGER)
    draw_pixel(d, ra_x + 2, ra_y + 11, ASSASSIN_POISON)

    # Slash effect
    if slash_effect:
        d.line([(cx + 3, base_y - 2), (cx + 10, base_y + 6)], fill=(255, 255, 255, 180), width=1)
        d.line([(cx + 10, base_y - 2), (cx + 3, base_y + 6)], fill=(255, 255, 255, 180), width=1)

    return img


# ========================================================================
#  TANK: "Bruto del Flegetonte" — massive armored demon
#  56x64 per frame, 8 frames per clip
# ========================================================================

TANK_OUTLINE = (10, 10, 10, 255)
TANK_ARMOR = (80, 80, 95, 255)       # Gray-blue steel
TANK_ARMOR_DARK = (50, 50, 65, 255)
TANK_SKIN = (100, 55, 40, 255)       # Dark reddish
TANK_EYE = (255, 50, 20, 255)        # Red glow
TANK_HORN = (90, 60, 30, 255)
TANK_WEAPON = (130, 130, 140, 255)   # Heavy mace
TANK_CHAIN = (100, 100, 110, 255)


def build_tank(state, frame, total_frames=8):
    FW, FH = 56, 64
    img = create_transparent(FW, FH)
    d = ImageDraw.Draw(img)

    cx = 28
    base_y = 30

    torso_dy = 0
    leg_l_dx, leg_r_dx = 0, 0
    arm_r_dx, arm_r_dy = 0, 0
    weapon_state = "rest"
    stomp = False

    if state == "idle":
        phase = (frame / total_frames) * 2 * math.pi
        torso_dy = int(math.sin(phase) * 1.2)

    elif state == "run":
        phase = (frame / total_frames) * 2 * math.pi
        torso_dy = int(abs(math.sin(phase)) * 3) - 1
        stride = 4
        leg_l_dx = int(math.sin(phase) * stride)
        leg_r_dx = int(math.sin(phase + math.pi) * stride)
        stomp = frame in [2, 6]

    elif state == "attack":
        # 2 raise + 2 slam + 2 impact + 2 recover
        if frame < 2:
            weapon_state = "raised"
            arm_r_dy = -6 - frame * 3
        elif frame < 4:
            weapon_state = "slam"
            arm_r_dy = 6
            arm_r_dx = 4
            stomp = True
        elif frame < 6:
            weapon_state = "ground"
            arm_r_dy = 8
            arm_r_dx = 6
        else:
            t = (frame - 6) / 2.0
            arm_r_dy = int(8 * (1 - t))
            arm_r_dx = int(6 * (1 - t))

    # LEGS (thick)
    leg_y = base_y + 14 + torso_dy
    leg_w = 8
    draw_rect(d, cx - 10 + leg_l_dx, leg_y, leg_w, 16, TANK_ARMOR_DARK, TANK_OUTLINE)
    draw_rect(d, cx + 2 + leg_r_dx, leg_y, leg_w, 16, TANK_ARMOR_DARK, TANK_OUTLINE)
    # Heavy boots
    draw_rect(d, cx - 12 + leg_l_dx, leg_y + 13, leg_w + 4, 4, TANK_ARMOR, TANK_OUTLINE)
    draw_rect(d, cx + 0 + leg_r_dx, leg_y + 13, leg_w + 4, 4, TANK_ARMOR, TANK_OUTLINE)

    # TORSO (massive)
    torso_w = 24
    draw_rect(d, cx - torso_w//2, base_y - 10 + torso_dy, torso_w, 22, TANK_ARMOR, TANK_OUTLINE)
    # Chest plate details
    draw_rect(d, cx - 4, base_y - 6 + torso_dy, 8, 14, TANK_ARMOR_DARK)
    # Chain across chest
    for i in range(0, torso_w - 4, 4):
        draw_rect(d, cx - torso_w//2 + 2 + i, base_y + torso_dy, 2, 2, TANK_CHAIN)

    # SHOULDERS (big pauldrons)
    draw_rect(d, cx - torso_w//2 - 4, base_y - 12 + torso_dy, 10, 8, TANK_ARMOR, TANK_OUTLINE)
    draw_rect(d, cx + torso_w//2 - 6, base_y - 12 + torso_dy, 10, 8, TANK_ARMOR, TANK_OUTLINE)

    # HEAD (small relative to body)
    head_y = base_y - 18 + torso_dy
    draw_rect(d, cx - 5, head_y, 10, 8, TANK_SKIN, TANK_OUTLINE)
    # Eyes
    draw_pixel(d, cx - 2, head_y + 3, TANK_EYE)
    draw_pixel(d, cx + 3, head_y + 3, TANK_EYE)
    # Horns (large)
    draw_rect(d, cx - 9, head_y - 4, 4, 6, TANK_HORN, TANK_OUTLINE)
    draw_rect(d, cx + 6, head_y - 4, 4, 6, TANK_HORN, TANK_OUTLINE)

    # RIGHT ARM + WEAPON
    ra_x = cx + torso_w//2 + arm_r_dx
    ra_y = base_y - 4 + arm_r_dy + torso_dy
    draw_rect(d, ra_x - 2, ra_y, 6, 12, TANK_SKIN, TANK_OUTLINE)

    # Weapon (heavy mace)
    if weapon_state == "rest":
        draw_rect(d, ra_x, ra_y + 12, 2, 14, TANK_ARMOR_DARK, TANK_OUTLINE)
        draw_rect(d, ra_x - 3, ra_y + 24, 8, 6, TANK_WEAPON, TANK_OUTLINE)
    elif weapon_state == "raised":
        draw_rect(d, ra_x, ra_y - 14, 2, 16, TANK_ARMOR_DARK, TANK_OUTLINE)
        draw_rect(d, ra_x - 3, ra_y - 20, 8, 6, TANK_WEAPON, TANK_OUTLINE)
    elif weapon_state == "slam":
        draw_rect(d, ra_x + 4, ra_y + 6, 14, 2, TANK_ARMOR_DARK, TANK_OUTLINE)
        draw_rect(d, ra_x + 14, ra_y + 2, 6, 8, TANK_WEAPON, TANK_OUTLINE)
    elif weapon_state == "ground":
        draw_rect(d, ra_x + 2, ra_y + 8, 12, 2, TANK_ARMOR_DARK, TANK_OUTLINE)
        draw_rect(d, ra_x + 10, ra_y + 6, 6, 8, TANK_WEAPON, TANK_OUTLINE)

    # LEFT ARM (shield/fist)
    la_x = cx - torso_w//2 - 6
    la_y = base_y - 2 + torso_dy
    draw_rect(d, la_x, la_y, 6, 10, TANK_SKIN, TANK_OUTLINE)
    # Shield
    draw_rect(d, la_x - 4, la_y - 2, 6, 12, TANK_ARMOR, TANK_OUTLINE)

    # Stomp effect
    if stomp:
        for dx in range(-8, 9, 4):
            draw_pixel(d, cx + dx, base_y + 32 + torso_dy, (150, 100, 50, 200))

    return img


# ========================================================================
#  BOMBER: "Suicida Retorcido" — squat round demon with bombs
#  32x32 per frame, 8 frames per clip
# ========================================================================

BOMBER_OUTLINE = (10, 10, 10, 255)
BOMBER_SKIN = (160, 80, 20, 255)       # Orange
BOMBER_SKIN_DARK = (120, 55, 15, 255)
BOMBER_EYE = (255, 255, 100, 255)      # Bright yellow (manic)
BOMBER_BOMB = (50, 50, 50, 255)
BOMBER_FUSE = (255, 120, 0, 255)
BOMBER_SPARK = (255, 255, 150, 255)


def build_bomber(state, frame, total_frames=8):
    FW, FH = 32, 32
    img = create_transparent(FW, FH)
    d = ImageDraw.Draw(img)

    cx = 16
    base_y = 14

    torso_dy = 0
    arm_dx = 0
    arm_dy = 0
    bomb_visible = True
    fuse_spark = frame % 2 == 0

    if state == "idle":
        phase = (frame / total_frames) * 2 * math.pi
        torso_dy = int(math.sin(phase) * 1.0)
        # Jittery movement (nervous bomber)
        arm_dx = int(math.sin(phase * 3) * 1)

    elif state == "run":
        phase = (frame / total_frames) * 2 * math.pi
        torso_dy = int(abs(math.sin(phase * 2)) * 2) - 1
        arm_dx = int(math.sin(phase) * 2)

    elif state == "attack":
        # 2 prepare + 2 throw + 2 recoil + 2 recover
        if frame < 2:
            arm_dy = -3 - frame
            arm_dx = -2
        elif frame < 4:
            arm_dy = -2
            arm_dx = 5 + (frame - 2) * 3
            bomb_visible = frame < 3
        elif frame < 6:
            arm_dy = 1
            arm_dx = 2
            bomb_visible = False
        else:
            arm_dy = 0
            bomb_visible = True

    # BODY (round/squat)
    body_y = base_y + torso_dy
    # Round body
    draw_rect(d, cx - 7, body_y, 14, 12, BOMBER_SKIN, BOMBER_OUTLINE)
    draw_rect(d, cx - 5, body_y - 2, 10, 2, BOMBER_SKIN, BOMBER_OUTLINE)
    draw_rect(d, cx - 5, body_y + 12, 10, 2, BOMBER_SKIN, BOMBER_OUTLINE)
    # Belly stripe
    draw_rect(d, cx - 3, body_y + 4, 6, 4, BOMBER_SKIN_DARK)

    # LEGS (stubby)
    draw_rect(d, cx - 5, body_y + 13, 4, 5, BOMBER_SKIN_DARK, BOMBER_OUTLINE)
    draw_rect(d, cx + 1, body_y + 13, 4, 5, BOMBER_SKIN_DARK, BOMBER_OUTLINE)

    # HEAD (small, on top of round body)
    head_y = body_y - 6
    draw_rect(d, cx - 4, head_y, 8, 5, BOMBER_SKIN, BOMBER_OUTLINE)
    # Wide manic eyes
    draw_rect(d, cx - 3, head_y + 1, 2, 2, BOMBER_EYE)
    draw_rect(d, cx + 2, head_y + 1, 2, 2, BOMBER_EYE)
    # Grin
    draw_rect(d, cx - 2, head_y + 3, 5, 1, (30, 10, 10, 255))

    # ARMS
    # Left arm
    draw_rect(d, cx - 9, body_y + 2, 3, 6, BOMBER_SKIN, BOMBER_OUTLINE)
    # Right arm + bomb
    ra_x = cx + 7 + arm_dx
    ra_y = body_y + 2 + arm_dy
    draw_rect(d, ra_x, ra_y, 3, 6, BOMBER_SKIN, BOMBER_OUTLINE)

    # BOMB
    if bomb_visible:
        bx = ra_x + 1
        by = ra_y - 4
        draw_rect(d, bx - 2, by, 5, 5, BOMBER_BOMB, BOMBER_OUTLINE)
        # Fuse
        draw_rect(d, bx, by - 2, 1, 2, BOMBER_FUSE)
        if fuse_spark:
            draw_pixel(d, bx, by - 3, BOMBER_SPARK)
            draw_pixel(d, bx + 1, by - 3, BOMBER_SPARK)

    return img


# ========================================================================
#  ASSEMBLY
# ========================================================================

def assemble_sheet(builder, state, total_frames, fw, fh):
    sheet = create_transparent(fw * total_frames, fh)
    for i in range(total_frames):
        frame_img = builder(state, i, total_frames)
        sheet.paste(frame_img, (i * fw, 0))
    return sheet


def generate_enemy(name, builder, fw, fh, idle_frames=8, run_frames=8, attack_frames=8):
    print(f"\n  Generando {name}...")

    idle = assemble_sheet(builder, "idle", idle_frames, fw, fh)
    idle.save(os.path.join(OUTPUT_DIR, f"{name}_idle.png"))
    print(f"    idle:   {idle.size[0]}x{idle.size[1]} ({idle_frames} frames)")

    run = assemble_sheet(builder, "run", run_frames, fw, fh)
    run.save(os.path.join(OUTPUT_DIR, f"{name}_run.png"))
    print(f"    run:    {run.size[0]}x{run.size[1]} ({run_frames} frames)")

    attack = assemble_sheet(builder, "attack", attack_frames, fw, fh)
    attack.save(os.path.join(OUTPUT_DIR, f"{name}_attack.png"))
    print(f"    attack: {attack.size[0]}x{attack.size[1]} ({attack_frames} frames)")


def main():
    print("INFERNUS -- Fase 4A: Sprites de enemigos mejorados (COMPLETO)")
    print("=" * 60)

    generate_enemy("melee", build_melee, 32, 48, 8, 8, 8)
    generate_enemy("ranged", build_ranged, 28, 40, 8, 8, 8)
    generate_enemy("assassin", build_assassin, 24, 40, 8, 8, 8)
    generate_enemy("tank", build_tank, 56, 64, 8, 8, 8)
    generate_enemy("bomber", build_bomber, 32, 32, 8, 8, 8)

    print("\n" + "=" * 60)
    print("Fase 4A COMPLETA: 5 enemigos x 3 clips x 8 frames = 120 frames totales")
    print("JSONs pendientes de actualizar: assassin, tank, bomber")


if __name__ == "__main__":
    main()

