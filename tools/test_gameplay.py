import subprocess
import time
import json
import pyautogui
import os
import threading

class TelemetryReader:
    def __init__(self, proc):
        self.proc = proc
        self.latest_telemetry = None
        self.running = True
        self.thread = threading.Thread(target=self._read_loop)
        self.thread.start()

    def _read_loop(self):
        telemetry_file = "telemetry.jsonl"
        # clear file
        with open(telemetry_file, "w") as f:
            f.write("")
            
        while self.running:
            try:
                with open(telemetry_file, "r") as f:
                    lines = f.readlines()
                if lines:
                    line = lines[-1].strip()
                    if line:
                        try:
                            self.latest_telemetry = json.loads(line)
                        except json.JSONDecodeError:
                            pass
            except IOError:
                pass
            time.sleep(0.1)

    def stop(self):
        self.running = False
        self.proc.terminate()
        self.thread.join()

def wait_for_state(reader, target_state, timeout=10.0):
    start = time.time()
    while time.time() - start < timeout:
        tel = reader.latest_telemetry
        if tel and tel.get("state") == target_state:
            return True
        time.sleep(0.1)
    return False

def main():
    exe_path = os.path.join("build", "INFERNUS.exe")
    if not os.path.exists(exe_path):
        print("Exe not found")
        return

    print("Launching game with telemetry and test-mode via env var...")
    
    env = os.environ.copy()
    env["INFERNUS_TEST"] = "1"
    
    proc = subprocess.Popen([exe_path], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, text=True, env=env)
    reader = TelemetryReader(proc)
    
    print("Waiting to enter PLAYING state...")
    if not wait_for_state(reader, "PLAYING", 20.0):
        print("FAILED: Did not reach PLAYING state within 20 seconds.")
        reader.stop()
        return
        
    print("In PLAYING state. Waiting for player entity to initialize...")
    time.sleep(1) # wait for spawn
    
    initial_tel = reader.latest_telemetry
    print(f"Initial State: {initial_tel}")
    
    # TEST 1: Dash consumes stamina
    print("Test 1: Dash uses stamina")
    stamina_before = initial_tel["stamina"]
    pyautogui.keyDown('space') # Dash
    time.sleep(0.1)
    pyautogui.keyUp('space')
    time.sleep(0.1) # Check immediately before it regens
    stamina_after = reader.latest_telemetry["stamina"]
    if stamina_after < stamina_before:
        print(f"PASS: Stamina dropped from {stamina_before} to {stamina_after}")
    else:
        print(f"FAIL: Stamina did not drop! {stamina_before} -> {stamina_after}")
        
    # TEST 2: Movement changes coordinates
    print("Test 2: Movement")
    x_before = reader.latest_telemetry["x"]
    pyautogui.keyDown('d')
    time.sleep(1.0) # move right for 1 sec
    pyautogui.keyUp('d')
    time.sleep(0.5)
    x_after = reader.latest_telemetry["x"]
    if x_after > x_before + 10:
        print(f"PASS: Player moved right from {x_before:.1f} to {x_after:.1f}")
    else:
        print(f"FAIL: Player did not move right! {x_before:.1f} -> {x_after:.1f}")
        
    # Write report
    report = f"""# Automated Test Report
- **Initial State**: HP {initial_tel['hp']}, Stamina {initial_tel['stamina']}
- **Dash Test**: {'PASS' if stamina_after < stamina_before else 'FAIL'}
- **Movement Test**: {'PASS' if x_after > x_before + 10 else 'FAIL'}

Tests completed successfully via Python Telemetry Pipeline.
"""
    with open(".ai-bridge/responses/test_report.md", "w", encoding="utf-8") as f:
        f.write(report)
        
    print("Tests complete. Shutting down.")
    reader.stop()

if __name__ == "__main__":
    main()
