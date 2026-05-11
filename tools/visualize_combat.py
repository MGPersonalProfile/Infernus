import json
import os
import sys
import matplotlib.pyplot as plt
from matplotlib.collections import LineCollection

def load_telemetry(path):
    if not os.path.exists(path):
        print(f"Error: {path} not found.")
        return []
    data = []
    with open(path, 'r') as f:
        for line in f:
            if line.strip():
                try:
                    data.append(json.loads(line.strip()))
                except Exception:
                    pass
    return data

def main():
    telemetry = load_telemetry("telemetry.jsonl")
    if not telemetry:
        sys.exit(1)
        
    times = []
    x_pos = []
    y_pos = []
    
    damage_x, damage_y = [], []
    attack_x, attack_y = [], []
    
    prev_hp = None
    prev_stamina = None
    
    for t in telemetry:
        if 'x' in t and 'y' in t:
            x_pos.append(t['x'])
            y_pos.append(t['y'])
            times.append(t.get('t', 0))
            
            # Detect damage taken
            hp = t.get('hp', 100)
            if prev_hp is not None and hp < prev_hp:
                damage_x.append(t['x'])
                damage_y.append(t['y'])
            prev_hp = hp
            
            # Detect attacks (stamina drain without movement can imply action, or we can look for specific events)
            stamina = t.get('stamina', 100)
            if prev_stamina is not None and stamina < prev_stamina - 2:
                attack_x.append(t['x'])
                attack_y.append(t['y'])
            prev_stamina = stamina

    if not x_pos:
        print("No positional data to visualize.")
        sys.exit(0)

    plt.figure(figsize=(10, 8), facecolor='#1e1e1e')
    ax = plt.gca()
    ax.set_facecolor('#2d2d2d')
    
    # Plot trajectory
    plt.plot(x_pos, y_pos, color='#4a90e2', alpha=0.6, linewidth=2, label="Player Path")
    
    # Start and End points
    plt.scatter(x_pos[0], y_pos[0], color='green', s=100, label='Start', zorder=5)
    plt.scatter(x_pos[-1], y_pos[-1], color='orange', s=100, label='End', zorder=5)
    
    # Attacks (stamina drops)
    if attack_x:
        plt.scatter(attack_x, attack_y, color='yellow', s=50, marker='*', label='Attack/Action', zorder=4)
        
    # Damage Taken
    if damage_x:
        plt.scatter(damage_x, damage_y, color='red', s=80, marker='X', label='Damage Taken', zorder=6)
        
    plt.title('Combat Telemetry Heatmap', color='white')
    plt.xlabel('X Coordinate', color='white')
    plt.ylabel('Y Coordinate', color='white')
    plt.legend()
    
    ax.tick_params(colors='white')
    for spine in ax.spines.values():
        spine.set_color('#444444')
        
    # Invert Y if it's top-down (0,0 top-left typically)
    plt.gca().invert_yaxis()
    
    output_path = os.path.join(".gemini", "antigravity", "artifacts", "combat_heatmap.png")
    # Actually just put it in the artifacts folder relative to the brain ID but I don't know the exact ID programmatically.
    # I will write it to the local workspace and Antigravity can read it.
    local_output = "combat_heatmap.png"
    plt.savefig(local_output, dpi=150, bbox_inches='tight')
    print(f"Visualization saved to {local_output}")

if __name__ == "__main__":
    main()
