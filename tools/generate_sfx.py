"""
INFERNUS -- SFX Generator (Procedural WAV)
Genera SFX retro-infernales usando sintesis procedural pura.
16-bit mono WAV, sin dependencias externas.
Estilo: souls-like dark fantasy, 8-bit con fuerza.
"""
import wave, struct, math, random, os

OUT = "assets/audio/sfx"
os.makedirs(OUT, exist_ok=True)
RATE = 22050  # 22kHz retro

def save_wav(filename, samples):
    path = os.path.join(OUT, filename)
    with wave.open(path, 'w') as w:
        w.setnchannels(1)
        w.setsampwidth(2)  # 16-bit
        w.setframerate(RATE)
        data = b''
        for s in samples:
            s = max(-1.0, min(1.0, s))
            data += struct.pack('<h', int(s * 32767))
        w.writeframes(data)
    print(f"  {filename}: {len(samples)/RATE:.2f}s ({os.path.getsize(path)} bytes)")


def noise():
    return random.random() * 2 - 1

def sine(t, freq):
    return math.sin(2 * math.pi * freq * t)

def square(t, freq):
    return 1.0 if sine(t, freq) > 0 else -1.0

def saw(t, freq):
    phase = (t * freq) % 1.0
    return 2.0 * phase - 1.0

def envelope(t, attack, decay, sustain, release, total):
    if t < attack:
        return t / attack
    elif t < attack + decay:
        return 1.0 - (1.0 - sustain) * (t - attack) / decay
    elif t < total - release:
        return sustain
    else:
        return sustain * (total - t) / release


# ============================================================
#  1. footstep.wav — breve pisada sobre piedra, ~80ms
# ============================================================
def gen_footstep():
    dur = 0.08
    samples = []
    for i in range(int(RATE * dur)):
        t = i / RATE
        env = max(0, 1.0 - t / dur)
        s = noise() * 0.3 * env
        s += sine(t, 120) * 0.2 * env * env
        samples.append(s * 0.5)
    save_wav("footstep.wav", samples)


# ============================================================
#  4. hit_fire.wav — sizzle/crackle, ~200ms
# ============================================================
def gen_hit_fire():
    dur = 0.2
    samples = []
    for i in range(int(RATE * dur)):
        t = i / RATE
        env = max(0, 1.0 - t / dur) ** 0.5
        crackle = noise() * 0.5 * (1 if random.random() > 0.7 else 0.1)
        hiss = sine(t, 2000 + noise() * 500) * 0.3
        s = (crackle + hiss) * env
        samples.append(s * 0.6)
    save_wav("hit_fire.wav", samples)


# ============================================================
#  5. hit_ice.wav — crystalline shatter, ~180ms
# ============================================================
def gen_hit_ice():
    dur = 0.18
    samples = []
    for i in range(int(RATE * dur)):
        t = i / RATE
        env = max(0, 1.0 - t / dur) ** 0.7
        crystal = sine(t, 3200) * 0.3 + sine(t, 4800) * 0.15
        shatter = noise() * 0.4 * max(0, 1 - t / 0.05)
        ring = sine(t, 1600) * 0.2 * max(0, 1 - t / dur)
        s = (crystal + shatter + ring) * env
        samples.append(s * 0.6)
    save_wav("hit_ice.wav", samples)


# ============================================================
#  6. hit_lightning.wav — zap/snap, ~150ms
# ============================================================
def gen_hit_lightning():
    dur = 0.15
    samples = []
    for i in range(int(RATE * dur)):
        t = i / RATE
        env = max(0, 1.0 - t / dur) ** 2
        zap = square(t, 800 + 400 * sine(t, 30)) * 0.4
        snap = noise() * 0.5 * max(0, 1 - t / 0.03)
        buzz = sine(t, 120) * 0.15 * env
        s = (zap + snap + buzz) * env
        samples.append(s * 0.5)
    save_wav("hit_lightning.wav", samples)


# ============================================================
#  7. hit_toxic.wav — organic gurgle/hiss, ~250ms
# ============================================================
def gen_hit_toxic():
    dur = 0.25
    samples = []
    for i in range(int(RATE * dur)):
        t = i / RATE
        env = max(0, 1.0 - t / dur) ** 0.6
        gurgle = sine(t, 200 + 100 * sine(t, 8)) * 0.3
        hiss = noise() * 0.2 * max(0, (t - 0.05) / dur)
        bubble = sine(t, 400 + 200 * sine(t, 5)) * 0.2 * (0.5 + 0.5 * sine(t, 12))
        s = (gurgle + hiss + bubble) * env
        samples.append(s * 0.6)
    save_wav("hit_toxic.wav", samples)


# ============================================================
#  8. cast_projectile.wav — whoosh with tail, ~300ms
# ============================================================
def gen_cast_projectile():
    dur = 0.3
    samples = []
    for i in range(int(RATE * dur)):
        t = i / RATE
        freq = 400 + 800 * (t / dur)  # rising
        env = envelope(t, 0.02, 0.05, 0.6, 0.15, dur)
        whoosh = noise() * 0.3 * env
        tone = sine(t, freq) * 0.4 * env
        s = whoosh + tone
        samples.append(s * 0.6)
    save_wav("cast_projectile.wav", samples)


# ============================================================
#  9. cast_shield.wav — chime/glass tone, ~400ms
# ============================================================
def gen_cast_shield():
    dur = 0.4
    samples = []
    for i in range(int(RATE * dur)):
        t = i / RATE
        env = max(0, 1.0 - t / dur) ** 0.3
        chime = sine(t, 880) * 0.3 + sine(t, 1320) * 0.2 + sine(t, 1760) * 0.1
        glass = sine(t, 2640) * 0.1 * max(0, 1 - t / 0.15)
        shimmer = sine(t, 3520 + 200 * sine(t, 6)) * 0.08 * env
        s = (chime + glass + shimmer) * env
        samples.append(s * 0.5)
    save_wav("cast_shield.wav", samples)


# ============================================================
#  10. cast_teleport.wav — vacuum/swap, ~250ms
# ============================================================
def gen_cast_teleport():
    dur = 0.25
    samples = []
    for i in range(int(RATE * dur)):
        t = i / RATE
        # Reverse sweep (high to low)
        freq = 2000 - 1800 * (t / dur)
        env = 0.5 + 0.5 * math.cos(math.pi * t / dur)
        sweep = sine(t, freq) * 0.4 * env
        vacuum = noise() * 0.2 * max(0, 1 - t / 0.1)
        warp = sine(t, 100 + 50 * sine(t, 20)) * 0.3 * (t / dur)
        s = sweep + vacuum + warp
        samples.append(s * 0.5)
    save_wav("cast_teleport.wav", samples)


# ============================================================
#  11. cast_shout.wav — bass boom + roar, ~400ms
# ============================================================
def gen_cast_shout():
    dur = 0.4
    samples = []
    for i in range(int(RATE * dur)):
        t = i / RATE
        env = envelope(t, 0.01, 0.05, 0.7, 0.2, dur)
        boom = sine(t, 60) * 0.5 * max(0, 1 - t / 0.1)
        roar = saw(t, 120 + 30 * sine(t, 4)) * 0.3 * env
        grit = noise() * 0.15 * env
        s = (boom + roar + grit) * env
        samples.append(s * 0.7)
    save_wav("cast_shout.wav", samples)


# ============================================================
#  12. cast_drain.wav — wet suction, ~300ms
# ============================================================
def gen_cast_drain():
    dur = 0.3
    samples = []
    for i in range(int(RATE * dur)):
        t = i / RATE
        env = max(0, 1.0 - t / dur) ** 0.5
        # Suction: descending sweep
        freq = 600 - 400 * (t / dur)
        suction = sine(t, freq) * 0.3
        wet = noise() * 0.15 * (0.5 + 0.5 * sine(t, 10))
        pulse = sine(t, 80 + 40 * sine(t, 3)) * 0.25 * env
        s = (suction + wet + pulse) * env
        samples.append(s * 0.6)
    save_wav("cast_drain.wav", samples)


def main():
    random.seed(666)
    print("INFERNUS -- SFX Generator")
    print("=" * 50)
    gen_footstep()
    gen_hit_fire()
    gen_hit_ice()
    gen_hit_lightning()
    gen_hit_toxic()
    gen_cast_projectile()
    gen_cast_shield()
    gen_cast_teleport()
    gen_cast_shout()
    gen_cast_drain()
    print("=" * 50)
    print("  10 SFX generados (attack_light/heavy ya existian)")

if __name__ == "__main__":
    main()
