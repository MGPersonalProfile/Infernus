"""
INFERNUS -- Fase 5: Screenshot Comparison Pipeline
====================================================
Takes screenshots of the WASM build via Playwright/Puppeteer and does
automated visual auditing without bothering the user.

Usage:
  python tools/screenshot_pipeline.py --url http://localhost:8080 --output screenshots/
  python tools/screenshot_pipeline.py --compare screenshots/baseline/ screenshots/current/

Requirements:
  pip install playwright Pillow
  playwright install chromium

Author: Antigravity
"""

import os
import sys
import argparse
import time
from datetime import datetime
from pathlib import Path

try:
    from PIL import Image, ImageChops, ImageDraw, ImageFont
    HAS_PIL = True
except ImportError:
    HAS_PIL = False


def capture_screenshots(url, output_dir, scenarios=None):
    """Capture screenshots from a running WASM build."""
    try:
        from playwright.sync_api import sync_playwright
    except ImportError:
        print("ERROR: playwright not installed. Run: pip install playwright && playwright install chromium")
        return False

    os.makedirs(output_dir, exist_ok=True)
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")

    if scenarios is None:
        scenarios = [
            {"name": "title_screen", "wait": 2, "actions": []},
            {"name": "class_select", "wait": 2, "actions": [
                {"type": "key", "key": "Enter", "wait": 1}
            ]},
            {"name": "gameplay_idle", "wait": 3, "actions": [
                {"type": "key", "key": "Enter", "wait": 0.5},
                {"type": "key", "key": "Enter", "wait": 2}
            ]},
            {"name": "gameplay_combat", "wait": 2, "actions": [
                {"type": "key", "key": "Enter", "wait": 0.5},
                {"type": "key", "key": "Enter", "wait": 1},
                {"type": "key", "key": "d", "wait": 2},
                {"type": "key", "key": "j", "wait": 0.5}
            ]},
            {"name": "debug_panel", "wait": 1, "actions": [
                {"type": "key", "key": "Enter", "wait": 0.5},
                {"type": "key", "key": "Enter", "wait": 1},
                {"type": "key", "key": "F3", "wait": 1}
            ]},
        ]

    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        page = browser.new_page(viewport={"width": 1280, "height": 720})
        page.goto(url)
        time.sleep(3)  # Wait for WASM to load

        for scenario in scenarios:
            name = scenario["name"]
            print(f"  Capturing: {name}...")

            # Reload for clean state
            page.goto(url)
            time.sleep(2)

            # Execute actions
            for action in scenario.get("actions", []):
                if action["type"] == "key":
                    page.keyboard.press(action["key"])
                elif action["type"] == "click":
                    page.click(action.get("selector", "canvas"))
                time.sleep(action.get("wait", 0.5))

            # Wait for scene to settle
            time.sleep(scenario.get("wait", 1))

            # Capture
            filepath = os.path.join(output_dir, f"{timestamp}_{name}.png")
            page.screenshot(path=filepath)
            print(f"    -> {filepath}")

        browser.close()

    print(f"\n  {len(scenarios)} screenshots captured in {output_dir}/")
    return True


def compare_screenshots(baseline_dir, current_dir, output_dir=None):
    """Compare two sets of screenshots and generate a diff report."""
    if not HAS_PIL:
        print("ERROR: Pillow not installed. Run: pip install Pillow")
        return False

    if output_dir is None:
        output_dir = os.path.join(current_dir, "diffs")
    os.makedirs(output_dir, exist_ok=True)

    baseline_files = {Path(f).stem.split("_", 2)[-1]: f
                      for f in os.listdir(baseline_dir)
                      if f.endswith(".png")}
    current_files = {Path(f).stem.split("_", 2)[-1]: f
                     for f in os.listdir(current_dir)
                     if f.endswith(".png")}

    common = set(baseline_files.keys()) & set(current_files.keys())
    if not common:
        print("ERROR: No matching screenshots found between baseline and current.")
        return False

    report = []
    print(f"\n  Comparing {len(common)} screenshot pairs...")

    for name in sorted(common):
        base_img = Image.open(os.path.join(baseline_dir, baseline_files[name]))
        curr_img = Image.open(os.path.join(current_dir, current_files[name]))

        # Resize if needed
        if base_img.size != curr_img.size:
            curr_img = curr_img.resize(base_img.size, Image.LANCZOS)

        # Calculate pixel difference
        diff = ImageChops.difference(base_img.convert("RGB"), curr_img.convert("RGB"))
        diff_pixels = diff.getdata()
        total_pixels = len(diff_pixels)
        changed_pixels = sum(1 for p in diff_pixels if sum(p) > 30)
        change_pct = (changed_pixels / total_pixels) * 100

        # Classification
        if change_pct < 0.5:
            status = "IDENTICAL"
        elif change_pct < 5.0:
            status = "MINOR_CHANGE"
        elif change_pct < 20.0:
            status = "SIGNIFICANT"
        else:
            status = "MAJOR_CHANGE"

        report.append({
            "name": name,
            "status": status,
            "change_pct": change_pct,
            "changed_pixels": changed_pixels,
            "total_pixels": total_pixels
        })

        # Save diff image (amplified for visibility)
        diff_amplified = diff.point(lambda x: min(255, x * 5))
        diff_amplified.save(os.path.join(output_dir, f"diff_{name}.png"))

        emoji = {"IDENTICAL": "=", "MINOR_CHANGE": "~", "SIGNIFICANT": "!", "MAJOR_CHANGE": "!!"}
        print(f"    [{emoji.get(status, '?')}] {name}: {status} ({change_pct:.1f}% changed)")

    # Generate summary report
    report_path = os.path.join(output_dir, "comparison_report.md")
    with open(report_path, "w", encoding="utf-8") as f:
        f.write("# Screenshot Comparison Report\n\n")
        f.write(f"Generated: {datetime.now().isoformat()}\n\n")
        f.write("| Screen | Status | Changed (%) |\n")
        f.write("|--------|--------|------------|\n")
        for r in report:
            f.write(f"| {r['name']} | {r['status']} | {r['change_pct']:.1f}% |\n")

    print(f"\n  Report: {report_path}")
    return True


def main():
    parser = argparse.ArgumentParser(description="INFERNUS Screenshot Pipeline")
    subparsers = parser.add_subparsers(dest="command")

    # Capture command
    cap = subparsers.add_parser("capture", help="Capture screenshots from WASM build")
    cap.add_argument("--url", default="http://localhost:8080", help="URL of the WASM build")
    cap.add_argument("--output", default="screenshots/current", help="Output directory")

    # Compare command
    comp = subparsers.add_parser("compare", help="Compare two screenshot sets")
    comp.add_argument("baseline", help="Directory with baseline screenshots")
    comp.add_argument("current", help="Directory with current screenshots")
    comp.add_argument("--output", default=None, help="Output directory for diffs")

    # Baseline command
    base = subparsers.add_parser("baseline", help="Capture and save as baseline")
    base.add_argument("--url", default="http://localhost:8080")
    base.add_argument("--output", default="screenshots/baseline")

    args = parser.parse_args()

    if args.command == "capture":
        capture_screenshots(args.url, args.output)
    elif args.command == "compare":
        compare_screenshots(args.baseline, args.current, args.output)
    elif args.command == "baseline":
        capture_screenshots(args.url, args.output)
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
