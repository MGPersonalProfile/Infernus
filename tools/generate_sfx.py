"""
SFX Generator for INFERNUS — Retro Dark Fantasy Sound Effects
=============================================================
Generates 16-bit mono .wav files using numpy synthesis.
Style: souls-like dark fantasy, retro 8-bit with weight.
Avoids cute/chiptune toy sounds.
"""

import numpy as np
import wave
import struct
import os
import sys

SAMPLE_RATE = 22050  # Lower rate for retro feel
MAX_AMP = 32767


def save_wav(filename, samples):
    """Save samples as 16-bit mono WAV."""
    # Normalize to -1..1 range if needed
    peak = np.max(np.abs(samples))
    if peak > 0:
        samples = samples / peak
    
    # Convert to 16-bit int
    data = (samples * MAX_AMP * 0.8).astype(np.int16)
    
    with wave.open(filename, 'w') as f:
        f.setnchannels(1)
        f.setsampwidth(2)  # 16-bit
        f.setframerate(SAMPLE_RATE)
        f.writeframes(data.tobytes())
    
    size = os.path.getsize(filename)
    print(f"  OK: {os.path.basename(filename)} ({len(data)/SAMPLE_RATE*1000:.0f}ms, {size} bytes)")


def envelope(length, attack=0.01, decay=0.1, sustain=0.5, release=0.2):
    """Generate ADSR envelope."""
    total = int(length * SAMPLE_RATE)
    env = np.zeros(total)
    
    a = int(attack * SAMPLE_RATE)
    d = int(decay * SAMPLE_RATE)
    r = int(release * SAMPLE_RATE)
    s = total - a - d - r
    if s < 0:
        s = 0
        r = max(0, total - a - d)
    
    # Attack
    if a > 0:
        env[:a] = np.linspace(0, 1, a)
    # Decay
    if d > 0:
        env[a:a+d] = np.linspace(1, sustain, d)
    # Sustain
    if s > 0:
        env[a+d:a+d+s] = sustain
    # Release
    if r > 0:
        env[a+d+s:a+d+s+r] = np.linspace(sustain, 0, r)
    
    return env[:total]


def noise(length):
    """White noise."""
    return np.random.uniform(-1, 1, int(length * SAMPLE_RATE))


def sine(freq, length):
    """Sine wave."""
    t = np.linspace(0, length, int(length * SAMPLE_RATE), endpoint=False)
    return np.sin(2 * np.pi * freq * t)


def square(freq, length):
    """Square wave (retro feel)."""
    return np.sign(sine(freq, length))


def sawtooth(freq, length):
    """Sawtooth wave."""
    t = np.linspace(0, length, int(length * SAMPLE_RATE), endpoint=False)
    return 2 * (t * freq - np.floor(t * freq + 0.5))


def pitch_sweep(f_start, f_end, length):
    """Frequency sweep (sine)."""
    t = np.linspace(0, length, int(length * SAMPLE_RATE), endpoint=False)
    freqs = np.linspace(f_start, f_end, len(t))
    phase = np.cumsum(freqs / SAMPLE_RATE) * 2 * np.pi
    return np.sin(phase)


def lowpass(signal, cutoff_freq):
    """Simple one-pole lowpass filter."""
    rc = 1.0 / (2 * np.pi * cutoff_freq)
    dt = 1.0 / SAMPLE_RATE
    alpha = dt / (rc + dt)
    out = np.zeros_like(signal)
    out[0] = signal[0]
    for i in range(1, len(signal)):
        out[i] = out[i-1] + alpha * (signal[i] - out[i-1])
    return out


def highpass(signal, cutoff_freq):
    """Simple highpass: original - lowpass."""
    return signal - lowpass(signal, cutoff_freq)


# ============================================================
# SFX GENERATORS
# ============================================================

def gen_footstep():
    """Footstep — brief, leather on stone. ~80ms, low volume."""
    dur = 0.08
    n = noise(dur)
    env = envelope(dur, attack=0.002, decay=0.03, sustain=0.2, release=0.03)
    filtered = lowpass(n, 800)
    return filtered * env * 0.4


def gen_attack_light():
    """Light attack — quick sword/dagger swish. ~150ms."""
    dur = 0.15
    n = noise(dur)
    sweep = pitch_sweep(2000, 800, dur)
    env = envelope(dur, attack=0.005, decay=0.05, sustain=0.3, release=0.05)
    filtered = highpass(n, 1500)
    return (filtered * 0.6 + sweep * 0.3) * env


def gen_attack_heavy():
    """Heavy attack — axe swing with body. ~250ms."""
    dur = 0.25
    n = noise(dur)
    sweep = pitch_sweep(600, 150, dur)
    env = envelope(dur, attack=0.01, decay=0.1, sustain=0.4, release=0.1)
    filtered = lowpass(n, 1200)
    return (filtered * 0.5 + sweep * 0.5) * env


def gen_hit_fire():
    """Fire hit — sizzle/crackle. ~200ms, high frequency."""
    dur = 0.2
    n = noise(dur)
    crackle = np.random.choice([-1, 0, 0, 0, 1], int(dur * SAMPLE_RATE)).astype(float)
    env = envelope(dur, attack=0.003, decay=0.05, sustain=0.5, release=0.1)
    filtered = highpass(n, 2000)
    return (filtered * 0.4 + crackle * 0.6) * env


def gen_hit_ice():
    """Ice hit — shatter/crystalline. ~180ms."""
    dur = 0.18
    n = noise(dur)
    crystal = sine(3500, dur) * 0.3 + sine(5200, dur) * 0.2 + sine(7800, dur) * 0.1
    env = envelope(dur, attack=0.001, decay=0.04, sustain=0.3, release=0.1)
    filtered = highpass(n, 3000)
    return (filtered * 0.4 + crystal * 0.6) * env


def gen_hit_lightning():
    """Lightning hit — zap/snap. ~150ms with high-pass."""
    dur = 0.15
    n = noise(dur)
    zap = square(200, dur) * sine(4000, dur)
    env = envelope(dur, attack=0.001, decay=0.02, sustain=0.4, release=0.08)
    filtered = highpass(n, 2500)
    return (filtered * 0.3 + zap * 0.7) * env


def gen_hit_toxic():
    """Toxic hit — gurgle/hiss organic. ~250ms."""
    dur = 0.25
    # Bubbling: modulated noise
    n = noise(dur)
    modulator = sine(8, dur) * 0.5 + 0.5  # Slow modulation for gurgle
    bubble = lowpass(n, 600) * modulator
    hiss = highpass(n, 3000) * 0.3
    env = envelope(dur, attack=0.01, decay=0.08, sustain=0.5, release=0.1)
    return (bubble * 0.7 + hiss * 0.3) * env


def gen_cast_projectile():
    """Projectile cast — whoosh with tail (Lanza de Flegetonte). ~300ms."""
    dur = 0.3
    n = noise(dur)
    sweep = pitch_sweep(400, 2000, dur)
    env = envelope(dur, attack=0.01, decay=0.05, sustain=0.5, release=0.15)
    # Whoosh = filtered noise + frequency sweep
    whoosh = lowpass(n, 1500)
    return (whoosh * 0.4 + sweep * 0.5) * env


def gen_cast_shield():
    """Shield cast — chime/glass tone (Escudo de Hielo). ~400ms."""
    dur = 0.4
    # Bell-like harmonics
    chime = sine(1200, dur) * 0.4 + sine(2400, dur) * 0.3 + sine(3600, dur) * 0.15 + sine(4800, dur) * 0.08
    # Glass shimmer
    n = noise(dur)
    shimmer = highpass(n, 4000) * 0.1
    env = envelope(dur, attack=0.005, decay=0.1, sustain=0.4, release=0.2)
    return (chime + shimmer) * env


def gen_cast_teleport():
    """Teleport cast — vacuum/swap (Paso Sombrío). ~250ms."""
    dur = 0.25
    # Reverse sweep (vacuum in)
    sweep_in = pitch_sweep(2000, 100, dur * 0.5)
    # Pop out
    sweep_out = pitch_sweep(100, 1500, dur * 0.5)
    combined = np.concatenate([sweep_in, sweep_out])
    n = noise(dur)
    filtered = lowpass(n, 800)
    env = envelope(dur, attack=0.01, decay=0.05, sustain=0.6, release=0.08)
    # Trim to same length
    min_len = min(len(combined), len(filtered), len(env))
    return (combined[:min_len] * 0.6 + filtered[:min_len] * 0.3) * env[:min_len]


def gen_cast_shout():
    """War shout — bass boom + roar (Grito de Guerra). ~400ms."""
    dur = 0.4
    # Deep boom
    boom = sine(60, dur) * 0.5 + sine(120, dur) * 0.3
    # Roar = distorted mid-frequency noise
    n = noise(dur)
    roar = lowpass(n, 800)
    roar = np.clip(roar * 3, -1, 1)  # Soft distortion for grit
    env = envelope(dur, attack=0.01, decay=0.15, sustain=0.5, release=0.15)
    return (boom * 0.5 + roar * 0.5) * env


def gen_cast_drain():
    """Soul drain — wet suction (Drenar Alma). ~300ms."""
    dur = 0.3
    # Suction = reverse envelope noise with low freq modulation
    n = noise(dur)
    modulator = sine(5, dur) * 0.3 + 0.7  # Pulsing
    suction = lowpass(n, 500) * modulator
    # Eerie tone
    tone = sine(300, dur) * 0.2 + sine(450, dur) * 0.15
    env = envelope(dur, attack=0.15, decay=0.05, sustain=0.6, release=0.08)
    return (suction * 0.6 + tone * 0.4) * env


# ============================================================
# MAIN
# ============================================================

def main():
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    sfx_dir = os.path.join(project_root, "assets", "audio", "sfx")
    os.makedirs(sfx_dir, exist_ok=True)
    
    # Check what already exists
    existing = set(os.listdir(sfx_dir))
    
    print("=== SFX Generator for INFERNUS ===\n")
    print(f"Output: {sfx_dir}")
    print(f"Existing files: {len(existing)}\n")
    
    # All SFX to generate
    sfx_list = [
        # Animation event SFX
        ("footstep.wav", gen_footstep, "Animation event: footstep"),
        # Note: attack_light and attack_heavy already exist, but the task says
        # they might need improvement. We'll generate new ones and compare.
        
        # Damage type variants
        ("hit_fire.wav", gen_hit_fire, "Damage: fire sizzle"),
        ("hit_ice.wav", gen_hit_ice, "Damage: ice shatter"),
        ("hit_lightning.wav", gen_hit_lightning, "Damage: lightning zap"),
        ("hit_toxic.wav", gen_hit_toxic, "Damage: toxic gurgle"),
        
        # Active ability cast SFX
        ("cast_projectile.wav", gen_cast_projectile, "Cast: Lanza de Flegetonte"),
        ("cast_shield.wav", gen_cast_shield, "Cast: Escudo de Hielo"),
        ("cast_teleport.wav", gen_cast_teleport, "Cast: Paso Sombrio"),
        ("cast_shout.wav", gen_cast_shout, "Cast: Grito de Guerra"),
        ("cast_drain.wav", gen_cast_drain, "Cast: Drenar Alma"),
    ]
    
    generated = 0
    skipped = 0
    
    for filename, generator, desc in sfx_list:
        filepath = os.path.join(sfx_dir, filename)
        if filename in existing:
            print(f"  SKIP (exists): {filename} — {desc}")
            skipped += 1
            continue
        
        try:
            samples = generator()
            save_wav(filepath, samples)
            generated += 1
        except Exception as e:
            print(f"  ERROR: {filename} — {e}")
    
    # Also generate footstep even if not in the skip list
    footstep_path = os.path.join(sfx_dir, "footstep.wav")
    if "footstep.wav" not in existing:
        # Already handled above
        pass
    
    print(f"\n=== DONE: {generated} generated, {skipped} skipped ===")


if __name__ == "__main__":
    main()
