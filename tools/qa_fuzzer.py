import json
import random
import subprocess
import time
import os
import sys
import shutil

ACTIONS = [
    "MOVE_UP", "MOVE_DOWN", "MOVE_LEFT", "MOVE_RIGHT",
    "ATTACK_LIGHT", "ATTACK_HEAVY", "DASH", "PARRY",
    "ABILITY_Q", "ABILITY_E", "OPEN_INFO"
]

def generate_fuzzer_script(duration_sec, events_per_sec=3):
    script = []
    total_events = duration_sec * events_per_sec
    
    current_time = 0.5
    for _ in range(total_events):
        action = random.choice(ACTIONS)
        event = {
            "t": round(current_time, 2),
            "action": action
        }
        
        # Moves need duration
        if action.startswith("MOVE_"):
            event["duration"] = round(random.uniform(0.1, 1.0), 2)
            
        script.append(event)
        current_time += round(random.uniform(0.1, 0.5), 2)
        
    return script

def detect_anomalies(telemetry, proc_return_code):
    anomalies = []
    
    if proc_return_code != 0:
        anomalies.append(f"Engine crashed with exit code {proc_return_code}")
        
    if not telemetry:
        anomalies.append("No telemetry generated")
        return anomalies
        
    prev_state = None
    
    for t in telemetry:
        # State anomalies
        if t.get('hp', 100) < 0:
            anomalies.append(f"HP dropped below 0: {t.get('hp')}")
        if t.get('stamina', 100) < 0 or t.get('stamina', 100) > 100:
            anomalies.append(f"Stamina out of bounds: {t.get('stamina')}")
            
        # Teleportation/Collision anomaly
        if prev_state and 'x' in t and 'y' in t and 'x' in prev_state and 'y' in prev_state:
            dx = abs(t['x'] - prev_state['x'])
            dy = abs(t['y'] - prev_state['y'])
            dt = t.get('t', 0) - prev_state.get('t', 0)
            
            # Max speed should be around ~200px/s. If delta is insane, collision tunneling happened
            if dt > 0 and (dx > 300 or dy > 300):
                anomalies.append(f"Teleportation detected! Moved dx:{dx:.1f} dy:{dy:.1f} in {dt:.2f}s")
                
        prev_state = t
        
    return anomalies

def run_fuzzer(runs=5, duration=30):
    exe_path = os.path.join("build", "INFERNUS.exe")
    script_path = os.path.join("assets", "test_scripts", "fuzzer.json")
    telemetry_file = "telemetry.jsonl"
    
    os.makedirs(os.path.dirname(script_path), exist_ok=True)
    
    print(f"=== Starting QA Fuzzer Campaign ({runs} runs, {duration}s each) ===")
    
    for run in range(1, runs + 1):
        print(f"\n[Run {run}/{runs}] Generating fuzzer script...")
        script = generate_fuzzer_script(duration)
        
        with open(script_path, "w") as f:
            json.dump(script, f, indent=2)
            
        if os.path.exists(telemetry_file):
            os.remove(telemetry_file)
            
        env = os.environ.copy()
        env["INFERNUS_HEADLESS"] = "1"
        env["INFERNUS_SCRIPT"] = "fuzzer"
        env["INFERNUS_TEST_DURATION"] = str(duration)
        
        print(f"Executing engine (Headless)...")
        start_time = time.time()
        
        try:
            proc = subprocess.run([exe_path], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, text=True, env=env, timeout=duration + 5)
            exec_time = time.time() - start_time
        except subprocess.TimeoutExpired:
            print("Engine timed out! Possible hang.")
            anomalies = ["Engine hang / timeout"]
            proc = None
            
        telemetry_data = []
        if os.path.exists(telemetry_file):
            with open(telemetry_file, "r") as f:
                for line in f:
                    line = line.strip()
                    if line:
                        try:
                            telemetry_data.append(json.loads(line))
                        except json.JSONDecodeError:
                            pass
                            
        anomalies = detect_anomalies(telemetry_data, proc.returncode if proc else -1)
        
        if anomalies:
            print(f"!!! ANOMALY DETECTED IN RUN {run} !!!")
            for a in anomalies:
                print(f" - {a}")
                
            crash_seed_path = f"fuzzer_crash_seed_{run}.json"
            shutil.copy2(script_path, crash_seed_path)
            print(f"Crash seed saved to {crash_seed_path}")
            
            # Write bridge task
            os.makedirs(".ai-bridge/antigravity-inbox", exist_ok=True)
            report = f"Fuzzer encontró un bug crítico.\nAnomalías:\n" + "\n".join(anomalies) + f"\nSemilla guardada en {crash_seed_path}"
            print("To report to Claude, create a task in the bridge.")
            return False
            
        print(f"Run {run} clean. Executed in {exec_time:.2f}s")
        
    print(f"\n=== Campaign Complete: {runs}/{runs} clean runs ===")
    return True

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--runs", type=int, default=5, help="Number of fuzzing campaigns")
    parser.add_argument("--duration", type=int, default=30, help="Duration of each run in seconds")
    args = parser.parse_args()
    
    success = run_fuzzer(args.runs, args.duration)
    if not success:
        sys.exit(1)
