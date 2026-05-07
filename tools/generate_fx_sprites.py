"""
INFERNUS -- Fase 4+: FX & Particle Sprites Mejorados
======================================================
Regenera los sprites de efectos visuales y particulas para que
se vean bien al zoom 1.5x que aplica el plan.

- slash_arc: 4 frames, 32x32 cada uno (era 96x32 = 24x32/frame)
- fireball: 4 frames animados (era 24x12 estatico)
- blood_drop: 4 variaciones (era 3 frames de 8x8, muy tosco)
- blood_burst: NUEVO - efecto de muerte
- dash_ghost: afterimage mejorado
- ember: 4 variaciones de brasa
- hit_particle: 4 variaciones de impacto
- orb_health/orb_stamina: mejorados con brillo
- item_pickup: sparkle mejorado
- Particulas nuevas para animation events: poison_trail, fire_burst,
  spark_burst, soul_wisps, ground_crack, shockwave

Autor: Antigravity
"""

from PIL import Image, ImageDraw
import os
import math
import random

random.seed(666)  # Infernal seed, reproducible

FX_DIR = "assets/sprites/fx"
PARTICLE_DIR = "assets/sprites/particles"
os.makedirs(FX_DIR, exist_ok=True)
os.makedirs(PARTICLE_DIR, exist_ok=True)


def create_transparent(w, h):
    return Image.new("RGBA", (w, h), (0, 0, 0, 0))


def draw_rect(draw, x, y, w, h, fill, outline=None):
    if w <= 0 or h <= 0:
        return
    draw.rectangle([x, y, x + w - 1, y + h - 1], fill=fill, outline=outline)


def draw_pixel(draw, x, y, color):
    if 0 <= x and 0 <= y:
        draw.point((x, y), fill=color)


# ============================================================
#  SLASH ARC — 4 frames, 32x32 each = 128x32 total
# ============================================================
def generate_slash_arc():
    FW, FH = 32, 32
    frames = 4
    sheet = create_transparent(FW * frames, FH)
    d = ImageDraw.Draw(sheet)

    colors = [
        (255, 255, 255, 200),
        (255, 240, 200, 180),
        (255, 220, 150, 140),
        (255, 200, 100, 80),
    ]

    for f in range(frames):
        ox = f * FW
        cx, cy = ox + 16, 16
        # Arc sweep animation: start narrow, expand
        start_angle = -60 + f * 30
        end_angle = 60 + f * 20
        radius = 10 + f * 3

        # Draw arc with decreasing opacity
        for r in range(radius, radius - 3, -1):
            color = colors[min(f, len(colors) - 1)]
            alpha = max(40, color[3] - (radius - r) * 30)
            c = (color[0], color[1], color[2], alpha)
            bbox = [cx - r, cy - r, cx + r, cy + r]
            d.arc(bbox, start_angle, end_angle, fill=c, width=2)

        # Trailing particles
        for i in range(3 + f):
            angle = math.radians(start_angle + random.random() * (end_angle - start_angle))
            pr = radius + random.randint(-2, 4)
            px = int(cx + math.cos(angle) * pr)
            py = int(cy + math.sin(angle) * pr)
            draw_pixel(d, px, py, (255, 255, 200, 150))

    sheet.save(os.path.join(FX_DIR, "slash_arc.png"))
    print(f"  slash_arc: {sheet.size}")


# ============================================================
#  FIREBALL — 4 frames, 16x16 each = 64x16 total
# ============================================================
def generate_fireball():
    FW, FH = 16, 16
    frames = 4
    sheet = create_transparent(FW * frames, FH)
    d = ImageDraw.Draw(sheet)

    for f in range(frames):
        ox = f * FW
        cx, cy = ox + 8, 8

        # Core (bright)
        phase = f * math.pi / 2
        r_core = 3
        draw_rect(d, cx - r_core, cy - r_core, r_core * 2, r_core * 2,
                  (255, 220, 100, 255))

        # Inner glow
        r_mid = 5
        for dy in range(-r_mid, r_mid + 1):
            for dx in range(-r_mid, r_mid + 1):
                dist = math.sqrt(dx*dx + dy*dy)
                if r_core < dist <= r_mid:
                    alpha = max(0, int(200 * (1 - dist / r_mid)))
                    flicker = int(math.sin(phase + dx * 0.5) * 30)
                    draw_pixel(d, cx + dx, cy + dy,
                               (255, 140 + flicker, 20, alpha))

        # Outer glow
        for dy in range(-7, 8):
            for dx in range(-7, 8):
                dist = math.sqrt(dx*dx + dy*dy)
                if r_mid < dist <= 7:
                    alpha = max(0, int(100 * (1 - dist / 7)))
                    draw_pixel(d, cx + dx, cy + dy,
                               (200, 80, 0, alpha))

        # Trailing embers
        for i in range(3):
            ex = cx - 4 - random.randint(0, 4)
            ey = cy + random.randint(-3, 3)
            draw_pixel(d, ex, ey, (255, 160, 40, 180))

    sheet.save(os.path.join(FX_DIR, "fireball.png"))
    print(f"  fireball: {sheet.size}")


# ============================================================
#  BLOOD DROP — 6 frame animation, 8x8 each = 48x8
# ============================================================
def generate_blood_drop():
    FW, FH = 8, 8
    frames = 6
    sheet = create_transparent(FW * frames, FH)
    d = ImageDraw.Draw(sheet)

    for f in range(frames):
        ox = f * FW
        cx, cy = ox + 4, 4

        # Drop falls and splats
        if f < 3:
            # Falling droplet
            size = 2
            y_off = f * 1
            draw_rect(d, cx - 1, cy - 2 + y_off, size, size + 1,
                       (180, 20, 20, 255), (120, 10, 10, 255))
            # Trail
            if f > 0:
                draw_pixel(d, cx, cy - 3 + y_off, (150, 20, 20, 150))
        else:
            # Splat expanding
            splat_r = 1 + (f - 3)
            y_off = 2
            for dy in range(-splat_r, splat_r + 1):
                for dx in range(-splat_r, splat_r + 1):
                    dist = abs(dx) + abs(dy)
                    if dist <= splat_r:
                        alpha = max(60, 220 - (f - 3) * 50)
                        draw_pixel(d, cx + dx, cy + y_off + dy,
                                   (160, 15, 15, alpha))

    sheet.save(os.path.join(PARTICLE_DIR, "blood_drop.png"))
    print(f"  blood_drop: {sheet.size}")


# ============================================================
#  ASH PARTICLE — 6 frame animation, 8x8 each = 48x8
# ============================================================
def generate_ash_particle():
    FW, FH = 8, 8
    frames = 6
    sheet = create_transparent(FW * frames, FH)
    d = ImageDraw.Draw(sheet)

    for f in range(frames):
        ox = f * FW
        cx, cy = ox + 4, 4

        # Rising ash that fades
        y_off = -f
        alpha = max(40, 200 - f * 30)
        size = max(1, 2 - f // 3)

        # Main particle
        for dy in range(size):
            for dx in range(size):
                sway = int(math.sin(f * 0.8) * 1.5)
                draw_pixel(d, cx + dx + sway, cy + y_off + dy,
                           (180, 160, 140, alpha))

        # Glow around particle
        if f < 4:
            glow_alpha = max(20, alpha // 3)
            draw_pixel(d, cx - 1, cy + y_off, (200, 120, 60, glow_alpha))
            draw_pixel(d, cx + size, cy + y_off, (200, 120, 60, glow_alpha))

    sheet.save(os.path.join(PARTICLE_DIR, "ash_particle.png"))
    print(f"  ash_particle: {sheet.size}")


# ============================================================
#  EMBER — 4 variants, 6x6 each = 24x6
# ============================================================
def generate_ember():
    FW, FH = 6, 6
    frames = 4
    sheet = create_transparent(FW * frames, FH)
    d = ImageDraw.Draw(sheet)

    ember_colors = [
        (255, 180, 40, 255),
        (255, 130, 20, 230),
        (255, 100, 10, 200),
        (200, 60, 0, 150),
    ]

    for f in range(frames):
        ox = f * FW
        cx, cy = ox + 3, 3

        color = ember_colors[f]
        # Core pixel
        draw_pixel(d, cx, cy, color)
        # Cross glow
        glow = (color[0], color[1], color[2], color[3] // 2)
        draw_pixel(d, cx - 1, cy, glow)
        draw_pixel(d, cx + 1, cy, glow)
        draw_pixel(d, cx, cy - 1, glow)
        draw_pixel(d, cx, cy + 1, glow)

    sheet.save(os.path.join(FX_DIR, "ember.png"))
    print(f"  ember: {sheet.size}")


# ============================================================
#  HIT PARTICLE — 4 frames of spark burst, 10x10 each = 40x10
# ============================================================
def generate_hit_particle():
    FW, FH = 10, 10
    frames = 4
    sheet = create_transparent(FW * frames, FH)
    d = ImageDraw.Draw(sheet)

    for f in range(frames):
        ox = f * FW
        cx, cy = ox + 5, 5

        # Expanding spark cross
        reach = 1 + f
        alpha = max(60, 255 - f * 60)

        for direction in [(1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (-1, -1), (1, -1), (-1, 1)]:
            for dist in range(reach):
                px = cx + direction[0] * dist
                py = cy + direction[1] * dist
                a = max(30, alpha - dist * 40)
                draw_pixel(d, px, py, (255, 220, 150, a))

        # Center white flash
        if f < 2:
            draw_pixel(d, cx, cy, (255, 255, 255, 255))

    sheet.save(os.path.join(FX_DIR, "hit_particle.png"))
    print(f"  hit_particle: {sheet.size}")


# ============================================================
#  DASH GHOST — afterimage, 16x48 (player sized)
# ============================================================
def generate_dash_ghost():
    FW, FH = 16, 48
    frames = 3
    sheet = create_transparent(FW * frames, FH)
    d = ImageDraw.Draw(sheet)

    for f in range(frames):
        ox = f * FW
        alpha = max(30, 120 - f * 40)

        # Humanoid silhouette, increasingly transparent
        # Head
        draw_rect(d, ox + 4, 8, 8, 8, (100, 150, 255, alpha))
        # Body
        draw_rect(d, ox + 3, 16, 10, 16, (80, 120, 230, alpha))
        # Legs
        draw_rect(d, ox + 3, 32, 4, 10, (60, 100, 200, alpha - 20))
        draw_rect(d, ox + 9, 32, 4, 10, (60, 100, 200, alpha - 20))

    sheet.save(os.path.join(FX_DIR, "dash_ghost.png"))
    print(f"  dash_ghost: {sheet.size}")


# ============================================================
#  ORB HEALTH — 4 frames with pulse, 16x16 each = 64x16
# ============================================================
def generate_orb_health():
    FW, FH = 16, 16
    frames = 4
    sheet = create_transparent(FW * frames, FH)
    d = ImageDraw.Draw(sheet)

    for f in range(frames):
        ox = f * FW
        cx, cy = ox + 8, 8
        phase = f * math.pi / 2

        # Outer glow
        for dy in range(-7, 8):
            for dx in range(-7, 8):
                dist = math.sqrt(dx*dx + dy*dy)
                pulse = math.sin(phase) * 0.3
                if dist <= 7:
                    alpha = max(0, int(60 * (1 - dist / 7) * (1 + pulse)))
                    draw_pixel(d, cx + dx, cy + dy, (255, 50, 50, alpha))

        # Core orb
        r = 4
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                dist = math.sqrt(dx*dx + dy*dy)
                if dist <= r:
                    brightness = 1 - dist / r
                    red = int(200 + 55 * brightness)
                    green = int(40 * brightness)
                    alpha = int(200 + 55 * brightness)
                    draw_pixel(d, cx + dx, cy + dy, (red, green, 40, alpha))

        # Highlight
        draw_pixel(d, cx - 1, cy - 2, (255, 200, 200, 200))
        draw_pixel(d, cx - 2, cy - 1, (255, 180, 180, 150))

        # Cross of health
        draw_rect(d, cx - 1, cy - 2, 2, 4, (255, 220, 220, 180))
        draw_rect(d, cx - 2, cy - 1, 4, 2, (255, 220, 220, 180))

    sheet.save(os.path.join(FX_DIR, "orb_health.png"))
    print(f"  orb_health: {sheet.size}")


# ============================================================
#  ORB STAMINA — 4 frames with pulse, 16x16 each = 64x16
# ============================================================
def generate_orb_stamina():
    FW, FH = 16, 16
    frames = 4
    sheet = create_transparent(FW * frames, FH)
    d = ImageDraw.Draw(sheet)

    for f in range(frames):
        ox = f * FW
        cx, cy = ox + 8, 8
        phase = f * math.pi / 2

        # Outer glow (amber/gold)
        for dy in range(-7, 8):
            for dx in range(-7, 8):
                dist = math.sqrt(dx*dx + dy*dy)
                pulse = math.sin(phase) * 0.3
                if dist <= 7:
                    alpha = max(0, int(60 * (1 - dist / 7) * (1 + pulse)))
                    draw_pixel(d, cx + dx, cy + dy, (220, 180, 50, alpha))

        # Core orb
        r = 4
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                dist = math.sqrt(dx*dx + dy*dy)
                if dist <= r:
                    brightness = 1 - dist / r
                    red = int(180 + 40 * brightness)
                    green = int(150 + 30 * brightness)
                    blue = int(50 * brightness)
                    alpha = int(200 + 55 * brightness)
                    draw_pixel(d, cx + dx, cy + dy, (red, green, blue, alpha))

        # Highlight
        draw_pixel(d, cx - 1, cy - 2, (255, 255, 200, 200))

        # Lightning bolt symbol
        pts = [(cx, cy - 2), (cx - 1, cy), (cx + 1, cy), (cx, cy + 2)]
        for px, py in pts:
            draw_pixel(d, px, py, (255, 255, 220, 200))

    sheet.save(os.path.join(FX_DIR, "orb_stamina.png"))
    print(f"  orb_stamina: {sheet.size}")


# ============================================================
#  ITEM PICKUP — sparkle effect, 4 frames, 16x16 = 64x16
# ============================================================
def generate_item_pickup():
    FW, FH = 16, 16
    frames = 4
    sheet = create_transparent(FW * frames, FH)
    d = ImageDraw.Draw(sheet)

    for f in range(frames):
        ox = f * FW
        cx, cy = ox + 8, 8

        # Expanding sparkle star
        reach = 2 + f * 2
        alpha = max(80, 255 - f * 50)

        # 4-point star
        for dist in range(reach):
            a = max(30, alpha - dist * 30)
            draw_pixel(d, cx + dist, cy, (255, 255, 200, a))
            draw_pixel(d, cx - dist, cy, (255, 255, 200, a))
            draw_pixel(d, cx, cy + dist, (255, 255, 200, a))
            draw_pixel(d, cx, cy - dist, (255, 255, 200, a))

        # Diagonal rays (shorter)
        for dist in range(max(1, reach - 1)):
            a = max(20, alpha - dist * 40)
            draw_pixel(d, cx + dist, cy + dist, (255, 220, 150, a))
            draw_pixel(d, cx - dist, cy + dist, (255, 220, 150, a))
            draw_pixel(d, cx + dist, cy - dist, (255, 220, 150, a))
            draw_pixel(d, cx - dist, cy - dist, (255, 220, 150, a))

        # Center
        draw_pixel(d, cx, cy, (255, 255, 255, 255))

    sheet.save(os.path.join(FX_DIR, "item_pickup.png"))
    print(f"  item_pickup: {sheet.size}")


# ============================================================
#  NEW PARTICLES for animation events system
# ============================================================

def generate_poison_trail():
    """Poison/toxic particle trail for assassin attacks"""
    FW, FH = 8, 8
    frames = 4
    sheet = create_transparent(FW * frames, FH)
    d = ImageDraw.Draw(sheet)

    for f in range(frames):
        ox = f * FW
        cx, cy = ox + 4, 4
        alpha = max(50, 200 - f * 45)

        # Toxic bubble
        r = max(1, 3 - f)
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                if abs(dx) + abs(dy) <= r:
                    draw_pixel(d, cx + dx, cy + dy - f,
                               (80, 220, 40, alpha))
        # Drip
        if f > 1:
            draw_pixel(d, cx, cy + 1, (60, 180, 30, alpha // 2))

    sheet.save(os.path.join(PARTICLE_DIR, "poison_trail.png"))
    print(f"  poison_trail: {sheet.size}")


def generate_fire_burst():
    """Fire burst particle for ranged attacks"""
    FW, FH = 12, 12
    frames = 4
    sheet = create_transparent(FW * frames, FH)
    d = ImageDraw.Draw(sheet)

    for f in range(frames):
        ox = f * FW
        cx, cy = ox + 6, 6
        radius = 2 + f * 2
        alpha = max(40, 220 - f * 50)

        for dy in range(-radius, radius + 1):
            for dx in range(-radius, radius + 1):
                dist = math.sqrt(dx*dx + dy*dy)
                if dist <= radius:
                    bright = 1 - dist / radius
                    r = int(255 * bright)
                    g = int(160 * bright * (1 - f * 0.15))
                    a = int(alpha * bright)
                    draw_pixel(d, cx + dx, cy + dy, (r, g, 20, a))

    sheet.save(os.path.join(PARTICLE_DIR, "fire_burst.png"))
    print(f"  fire_burst: {sheet.size}")


def generate_spark_burst():
    """Metal sparks for parry effect"""
    FW, FH = 12, 12
    frames = 4
    sheet = create_transparent(FW * frames, FH)
    d = ImageDraw.Draw(sheet)

    for f in range(frames):
        ox = f * FW
        cx, cy = ox + 6, 6

        num_sparks = 6 + f * 2
        max_dist = 2 + f * 2
        alpha = max(60, 255 - f * 55)

        for i in range(num_sparks):
            angle = random.random() * math.pi * 2
            dist = random.random() * max_dist
            px = int(cx + math.cos(angle) * dist)
            py = int(cy + math.sin(angle) * dist)
            draw_pixel(d, px, py, (255, 255, 200, alpha))

        if f == 0:
            draw_pixel(d, cx, cy, (255, 255, 255, 255))

    sheet.save(os.path.join(PARTICLE_DIR, "spark_burst.png"))
    print(f"  spark_burst: {sheet.size}")


def generate_soul_wisps():
    """Soul wisps rising from dead enemies"""
    FW, FH = 10, 16
    frames = 6
    sheet = create_transparent(FW * frames, FH)
    d = ImageDraw.Draw(sheet)

    for f in range(frames):
        ox = f * FW
        cx = ox + 5
        alpha = max(30, 180 - f * 28)

        for w in range(3):
            wy = 12 - f * 2 - w * 3
            wx = int(math.sin(f * 0.8 + w * 2) * 2)
            size = max(1, 2 - f // 3)
            draw_rect(d, cx + wx - size//2, wy, size, size,
                       (180, 200, 255, alpha))
            # Trail
            draw_pixel(d, cx + wx, wy + 1, (150, 170, 230, alpha // 2))

    sheet.save(os.path.join(PARTICLE_DIR, "soul_wisps.png"))
    print(f"  soul_wisps: {sheet.size}")


def generate_ground_crack():
    """Ground impact cracks for tank/boss slam"""
    FW, FH = 24, 12
    frames = 3
    sheet = create_transparent(FW * frames, FH)
    d = ImageDraw.Draw(sheet)

    for f in range(frames):
        ox = f * FW
        cx, cy = ox + 12, 6

        # Cracks radiating from center
        num_cracks = 3 + f * 2
        max_len = 4 + f * 4
        alpha = max(100, 220 - f * 40)

        for i in range(num_cracks):
            angle = (i / num_cracks) * math.pi * 2 + random.random() * 0.5
            length = random.randint(max_len // 2, max_len)
            for step in range(length):
                px = int(cx + math.cos(angle) * step)
                py = int(cy + math.sin(angle) * step * 0.5)
                draw_pixel(d, px, py, (100, 80, 60, alpha))
                # Lava glow in cracks
                if random.random() > 0.5:
                    draw_pixel(d, px, py, (200, 100, 20, alpha // 2))

    sheet.save(os.path.join(PARTICLE_DIR, "ground_crack.png"))
    print(f"  ground_crack: {sheet.size}")


def generate_shockwave():
    """Expanding ring shockwave for boss attacks"""
    FW, FH = 32, 32
    frames = 4
    sheet = create_transparent(FW * frames, FH)
    d = ImageDraw.Draw(sheet)

    for f in range(frames):
        ox = f * FW
        cx, cy = ox + 16, 16
        radius = 4 + f * 5
        alpha = max(40, 200 - f * 45)

        # Ring
        bbox = [cx - radius, cy - radius, cx + radius, cy + radius]
        d.ellipse(bbox, outline=(255, 200, 100, alpha), width=2)

        # Inner glow
        inner_r = max(1, radius - 3)
        bbox_inner = [cx - inner_r, cy - inner_r, cx + inner_r, cy + inner_r]
        d.ellipse(bbox_inner, outline=(255, 160, 60, alpha // 2), width=1)

    sheet.save(os.path.join(PARTICLE_DIR, "shockwave.png"))
    print(f"  shockwave: {sheet.size}")


def generate_dust_cloud():
    """Dust cloud for impacts"""
    FW, FH = 16, 12
    frames = 4
    sheet = create_transparent(FW * frames, FH)
    d = ImageDraw.Draw(sheet)

    for f in range(frames):
        ox = f * FW
        cx, cy = ox + 8, 8
        alpha = max(30, 160 - f * 35)

        num_puffs = 4 + f
        spread = 2 + f * 2

        for i in range(num_puffs):
            px = cx + random.randint(-spread, spread)
            py = cy + random.randint(-spread // 2, spread // 2) - f
            size = max(1, 3 - f // 2)
            draw_rect(d, px - size//2, py - size//2, size, size,
                       (140, 120, 100, alpha))

    sheet.save(os.path.join(PARTICLE_DIR, "dust_cloud.png"))
    print(f"  dust_cloud: {sheet.size}")


# ============================================================
#  MAIN
# ============================================================

def main():
    print("INFERNUS -- FX & Particle Sprite Generation")
    print("=" * 50)

    print("\n  FX Sprites:")
    generate_slash_arc()
    generate_fireball()
    generate_ember()
    generate_hit_particle()
    generate_dash_ghost()
    generate_orb_health()
    generate_orb_stamina()
    generate_item_pickup()

    print("\n  Particle Sprites:")
    generate_blood_drop()
    generate_ash_particle()
    generate_poison_trail()
    generate_fire_burst()
    generate_spark_burst()
    generate_soul_wisps()
    generate_ground_crack()
    generate_shockwave()
    generate_dust_cloud()

    print("\n" + "=" * 50)
    print("  FX/Particles completos: 8 FX + 9 particles = 17 sprites")


if __name__ == "__main__":
    main()
