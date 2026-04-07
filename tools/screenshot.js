const puppeteer = require('puppeteer-core');

(async () => {
  const browser = await puppeteer.launch({
    headless: true,
    executablePath: 'C:/Program Files/Google/Chrome/Application/chrome.exe',
    args: ['--no-sandbox', '--disable-setuid-sandbox', '--disable-gpu']
  });
  const page = await browser.newPage();
  await page.setViewport({ width: 1280, height: 720 });

  console.log('Loading INFERNUS...');
  try {
    await page.goto('http://localhost:8080/INFERNUS.html', { waitUntil: 'networkidle0', timeout: 30000 });
  } catch(e) {
    console.log('Timeout on load, continuing...');
  }

  // Wait for canvas to render
  await page.waitForSelector('canvas', { timeout: 10000 });
  await new Promise(r => setTimeout(r, 4000)); // Let WASM init + render

  // Screenshot 1: Main menu
  await page.screenshot({ path: 'recovery_frames/wasm_menu.png', fullPage: false });
  console.log('Screenshot 1: Main menu saved');

  // Press Enter to go to character select
  await page.keyboard.press('Enter');
  await new Promise(r => setTimeout(r, 2000));
  await page.screenshot({ path: 'recovery_frames/wasm_charselect.png', fullPage: false });
  console.log('Screenshot 2: Character select saved');

  // Press Enter to start game
  await page.keyboard.press('Enter');
  await new Promise(r => setTimeout(r, 3000));
  await page.screenshot({ path: 'recovery_frames/wasm_gameplay.png', fullPage: false });
  console.log('Screenshot 3: Gameplay saved');

  await browser.close();
  console.log('Done!');
})();
