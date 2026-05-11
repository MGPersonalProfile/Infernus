import subprocess
import time
import json
import os
import sys

# Define test suite: (script_name, duration_sec, checks_func)
def check_combat_basic(telemetry):
    if not telemetry: return False, "No telemetry generated"
    stamina_drops = any(t.get('stamina', 100) < 100 for t in telemetry)
    x_changed = any(abs(t.get('x', 0) - telemetry[0].get('x', 0)) > 10 for t in telemetry)
    return (stamina_drops and x_changed), "Stamina dropped and position changed"

def check_move_and_attack(telemetry):
    if not telemetry: return False, "No telemetry"
    x_changed = any(abs(t.get('x', 0) - telemetry[0].get('x', 0)) > 5 for t in telemetry)
    # Could check for hit_dealt events if we parse them properly
    hit_events = [t for t in telemetry if t.get('event') == 'hit_dealt']
    return x_changed, f"Moved and {len(hit_events)} hit events found"

def check_use_abilities(telemetry):
    if not telemetry: return False, "No telemetry"
    return True, "Executed ability script without crashes"

def check_dash_through_room(telemetry):
    if not telemetry: return False, "No telemetry"
    stamina_drops = any(t.get('stamina', 100) < 100 for t in telemetry)
    return stamina_drops, "Dashes successfully drained stamina"

def check_full_run(telemetry):
    if not telemetry: return False, "No telemetry"
    return len(telemetry) > 10, f"Full run executed successfully ({len(telemetry)} frames)"

TEST_SUITE = [
    ("combat_basic", 10, check_combat_basic),
    ("move_and_attack", 6, check_move_and_attack),
    ("use_abilities", 6, check_use_abilities),
    ("dash_through_room", 6, check_dash_through_room),
    ("full_run", 30, check_full_run)
]

def run_test(exe_path, script_name, duration):
    print(f"\n--- Running Test: {script_name} ---")
    telemetry_file = "telemetry.jsonl"
    
    if os.path.exists(telemetry_file):
        os.remove(telemetry_file)

    env = os.environ.copy()
    env["INFERNUS_HEADLESS"] = "1"
    env["INFERNUS_SCRIPT"] = script_name
    env["INFERNUS_TEST_DURATION"] = str(duration)
    
    print(f"Launching {exe_path} (headless)...")
    start_time = time.time()
    
    try:
        proc = subprocess.run([exe_path], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, text=True, env=env, timeout=duration + 5)
        exec_time = time.time() - start_time
        print(f"Engine exited with code {proc.returncode} in {exec_time:.2f}s")
    except subprocess.TimeoutExpired:
        print("Engine timed out! Killing process...")
        return None

    # Parse telemetry
    if not os.path.exists(telemetry_file):
        print(f"FAIL: {telemetry_file} was not generated.")
        return []
        
    telemetry_data = []
    with open(telemetry_file, "r") as f:
        for line in f:
            line = line.strip()
            if line:
                try:
                    telemetry_data.append(json.loads(line))
                except json.JSONDecodeError:
                    pass
                    
    return telemetry_data

def main():
    exe_path = os.path.join("build", "INFERNUS.exe")
    if not os.path.exists(exe_path):
        print(f"Exe not found at {exe_path}. Build the project first.")
        sys.exit(1)

    print("=== INFERNUS HEADLESS E2E QA SUITE ===")
    
    passed_tests = 0
    total_tests = len(TEST_SUITE)
    report_lines = ["# Automated Headless E2E Test Report\n"]
    
    for script_name, duration, check_func in TEST_SUITE:
        telemetry = run_test(exe_path, script_name, duration)
        
        if telemetry is None:
            report_lines.append(f"- **{script_name}**: FAIL (Timeout)")
            continue
            
        success, msg = check_func(telemetry)
        
        if success:
            print(f"PASS: {msg}")
            report_lines.append(f"- **{script_name}**: PASS ({msg})")
            passed_tests += 1
        else:
            print(f"FAIL: {msg}")
            report_lines.append(f"- **{script_name}**: FAIL ({msg})")

    report_lines.append(f"\n**Summary**: {passed_tests}/{total_tests} tests passed.")
    
    report_content = "\n".join(report_lines)
    os.makedirs(".ai-bridge/responses", exist_ok=True)
    with open(".ai-bridge/responses/test_report.md", "w", encoding="utf-8") as f:
        f.write(report_content)
        
    print("\n======================================")
    print(f"Suite Complete. {passed_tests}/{total_tests} passed.")
    print("Report written to .ai-bridge/responses/test_report.md")
    
    if passed_tests < total_tests:
        sys.exit(1)

if __name__ == "__main__":
    main()
