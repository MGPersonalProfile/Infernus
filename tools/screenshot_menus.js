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

  await page.waitForSelector('canvas', { timeout: 10000 });
  await new Promise(r => setTimeout(r, 4000));

  // Open Options from main menu (ESC)
  await page.keyboard.press('Escape');
  await new Promise(r => setTimeout(r, 1500));
  await page.screenshot({ path: 'recovery_frames/wasm_options.png' });
  console.log('Options screenshot saved');

  // Back to menu, then start game
  await page.keyboard.press('Escape');
  await new Promise(r => setTimeout(r, 1000));
  await page.keyboard.press('Enter'); // → Char select
  await new Promise(r => setTimeout(r, 1500));
  await page.keyboard.press('Enter'); // → Playing (with fade)
  await new Promise(r => setTimeout(r, 3500));

  // Open pause menu
  await page.keyboard.press('Escape');
  await new Promise(r => setTimeout(r, 1000));
  await page.screenshot({ path: 'recovery_frames/wasm_pause.png' });
  console.log('Pause screenshot saved');

  // Press DOWN to highlight Opciones
  await page.keyboard.press('ArrowDown');
  await new Promise(r => setTimeout(r, 500));
  await page.screenshot({ path: 'recovery_frames/wasm_pause_nav.png' });
  console.log('Pause nav screenshot saved');

  await browser.close();
  console.log('Done!');
})();
