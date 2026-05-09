import os
import time
import subprocess
from datetime import datetime

try:
    import pyautogui
    from PIL import ImageGrab
except ImportError:
    print("Please install pyautogui and Pillow: pip install pyautogui Pillow")
    exit(1)

def main():
    audit_dir = os.path.abspath(os.path.join(".ai-bridge", "scratch", "visual_audit_2026-05-06"))
    os.makedirs(audit_dir, exist_ok=True)
    
    print("Launching INFERNUS.exe...")
    exe_path = os.path.join("build", "INFERNUS.exe")
    if not os.path.exists(exe_path):
        print(f"Error: {exe_path} not found.")
        return

    process = subprocess.Popen([exe_path])
    time.sleep(3) # Wait for startup

    print("Capturing Main Menu...")
    ImageGrab.grab().save(os.path.join(audit_dir, "01_main_menu.png"))
    
    print("Navigating to Character Select...")
    pyautogui.press("enter")
    time.sleep(1)
    
    print("Capturing Character Select...")
    ImageGrab.grab().save(os.path.join(audit_dir, "02_character_select.png"))
    
    print("Selecting Warrior and entering game...")
    pyautogui.press("enter")
    time.sleep(3) # Wait for room generation
    
    print("Capturing Gameplay Idle...")
    ImageGrab.grab().save(os.path.join(audit_dir, "03_gameplay_idle.png"))
    
    print("Moving right to capture lookahead...")
    pyautogui.keyDown("d")
    time.sleep(0.5)
    ImageGrab.grab().save(os.path.join(audit_dir, "04_gameplay_running.png"))
    pyautogui.keyUp("d")
    
    print("Attacking...")
    pyautogui.press("j")
    time.sleep(0.2)
    ImageGrab.grab().save(os.path.join(audit_dir, "05_combat_attack.png"))
    
    print("Dashing...")
    pyautogui.press("space")
    time.sleep(0.1)
    ImageGrab.grab().save(os.path.join(audit_dir, "06_gameplay_dash.png"))
    
    print("Closing game...")
    process.terminate()
    print(f"Screenshots saved to {audit_dir}")

if __name__ == "__main__":
    main()
