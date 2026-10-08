// Diagnóstico: carga la partida del slot 0 y vuelca TODA la consola y el stack
// del abort (para localizar un RuntimeError durante la carga).
import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const URL_BASE = process.env.VC_URL || 'http://localhost:2077';
const CHROME = process.env.CHROME || 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe';
const SAVE = process.env.VC_SAVE || path.join(HERE, 'testdata', 'GTAVCsf1.b');
const PROFILE = path.join(os.tmpdir(), 'vc-abort-diag-profile');
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

const { createRequire } = await import('node:module');
const dir = process.env.PUPPETEER_DIR ||
  'C:/Users/s0rno/AppData/Local/Temp/opencode/vc-e2e/node_modules';
const req = createRequire(path.join(dir, 'anchor.js'));
const puppeteer = req('puppeteer-core');

const browser = await puppeteer.launch({
  executablePath: CHROME, headless: 'new', protocolTimeout: 60000,
  args: ['--no-sandbox', '--disable-dev-shm-usage', '--use-angle=swiftshader',
    '--enable-unsafe-swiftshader', '--mute-audio', '--window-size=1280,800',
    '--user-data-dir=' + PROFILE],
});
const page = await browser.newPage();
await page.setViewport({ width: 1280, height: 800 });
page.on('console', (m) => console.log('[console] ' + m.type() + ': ' + m.text().slice(0, 800)));
page.on('pageerror', (e) => console.log('[pageerror] ' + String(e).slice(0, 2000)));
await page.goto(URL_BASE + '/?autostart=1', { waitUntil: 'domcontentloaded', timeout: 60000 });

const b64 = fs.readFileSync(SAVE).toString('base64');
const write = async () => page.evaluate(async (b) => {
  try {
    if (typeof FS === 'undefined') return 'sin FS';
    const bin = atob(b);
    const arr = new Uint8Array(bin.length);
    for (let i = 0; i < bin.length; i++) arr[i] = bin.charCodeAt(i);
    FS.writeFile('/userfiles/GTAVCsf1.b', arr);
    await new Promise((res) => FS.syncfs(false, res));
    return 'ok size=' + FS.stat('/userfiles/GTAVCsf1.b').size;
  } catch (e) { return 'ERR ' + e; }
}, b64).catch((e) => 'ERR ' + e);
let seed = 'no';
for (let i = 0; i < 90; i++) { seed = await write(); if (String(seed).startsWith('ok')) break; await sleep(1000); }
console.log('save: ' + seed);

const t0 = Date.now();
while (Date.now() - t0 < 150000) {
  const log = await page.$eval('#gamelog', (e) => e.textContent).catch(() => '');
  const st = (log.match(/state \d+/g) || []).slice(-3);
  if (st.length === 3 && st.every((s) => s === 'state 7')) break;
  await sleep(2000);
}
await sleep(4000);
const rect = await page.evaluate(() => {
  const c = document.querySelector('.vc-canvas');
  const r = c.getBoundingClientRect();
  return [r.x, r.y, r.width, r.height];
});
const at = ([fx, fy]) => [rect[0] + fx * rect[2], rect[1] + fy * rect[3]];
const slowClick = async (x, y) => {
  await page.mouse.move(x, y, { steps: 3 }).catch(() => {});
  await sleep(700);
  await page.mouse.down().catch(() => {});
  await sleep(350);
  await page.mouse.up().catch(() => {});
  await sleep(3000);
};
console.log('== clic INICIAR PARTIDA');
await slowClick(...at([0.5, 325 / 800]));
console.log('== clic CARGAR PARTIDA');
await slowClick(...at([0.5, 345 / 800]));
console.log('== clic slot 1');
await slowClick(...at([0.3125, 180 / 800]));
console.log('== clic SÍ');
await slowClick(...at([0.5, 418 / 800]));
await sleep(30000);
await page.screenshot({ path: path.join(os.tmpdir(), 'diag-abort.png') }).catch(() => {});
await browser.close().catch(() => {});
process.exit(0);
