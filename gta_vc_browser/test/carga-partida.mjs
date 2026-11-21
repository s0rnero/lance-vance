import puppeteer from 'puppeteer-core';
import fs from 'node:fs';
import path from 'node:path';
const URL = process.argv[2] || 'http://localhost:2077/';
// Ruta absoluta desde el propio test: antes era relativa y, lanzado desde
// gta_vc_browser/, miraba un fichero que no existe y daba "stall" para siempre.
const LOG = path.join(import.meta.dirname, '..', 'web', 'odtrace.log');
let size = fs.existsSync(LOG) ? fs.statSync(LOG).size : 0;
let menu = false, ingame = false, lastProg = Date.now(), txt = '';
function drain() {
  if (!fs.existsSync(LOG)) return;
  const s2 = fs.statSync(LOG).size; if (s2 < size) size = 0;
  if (s2 === size) return;
  const fd = fs.openSync(LOG, 'r'); const b = Buffer.alloc(s2 - size);
  fs.readSync(fd, b, 0, b.length, size); fs.closeSync(fd); size = s2;
  txt = b.toString('utf8');
  if (/WEBHB .*state=7/.test(txt)) menu = true;
  if (/ingame=1/.test(txt)) ingame = true;
  if (/LOADSCREEN|STEP3|loadlevel|state=7/.test(txt)) lastProg = Date.now();
}
const br = await puppeteer.launch({ executablePath: 'C:/Program Files/Google/Chrome/Application/chrome.exe',
  headless: 'new', protocolTimeout: 0, args: ['--use-angle=swiftshader','--enable-unsafe-swiftshader','--ignore-gpu-blocklist','--autoplay-policy=no-user-gesture-required','--window-size=1280,720'] });
const p = await br.newPage(); await p.setViewport({ width: 1280, height: 720 });
await p.goto(URL, { waitUntil: 'domcontentloaded', timeout: 90000 });
const box = await p.evaluate(() => ({ w: innerWidth, h: innerHeight }));
await p.mouse.click(box.w/2, Math.round(box.h*0.78));        // portada / audio
// esperar menu del motor
const t0 = Date.now();
while (!menu && Date.now()-t0 < 180000) { drain(); await new Promise(r=>setTimeout(r,800)); }
// lo que hace el jugador: clic en la opcion y Enter
const hook = await p.evaluate(async () => { const cands=[window.Module,window.__reVCModule,window.reVCModule,window.game&&window.game.Module]; for (const m of cands) { if (m && m._TestNewGame) { // 83.1: la exportacion esta en -sJSPI_EXPORTS, asi que Emscripten ya la
 // envuelve y devuelve una promesa. Envolverla otra vez con
 // WebAssembly.promising dejaba el arranque clavado a mitad de Init (el
 // motor volvia de una descarga y no volvia a entrar en el import).
 const r = m._TestNewGame();
 if (r && typeof r.then === 'function') r.then(()=>{},()=>{});
 return 'ok'; } } return 'sin Module: ' + Object.keys(window).filter(k=>/module|reVC|game/i.test(k)).slice(0,8).join(','); }); console.log('hook       :', hook);
await p.keyboard.press('KeyW'); await p.keyboard.press('Enter'); await new Promise(r=>setTimeout(r,800)); await p.keyboard.press('Enter');
const t1 = Date.now(); let res = 'timeout';
while (Date.now()-t1 < 240000) {
  drain();
  if (ingame) { res = 'ok'; break; }
  if (Date.now()-lastProg > 90000) { res = 'stall'; break; }
  await new Promise(r=>setTimeout(r,800));
}
await br.close();
console.log('menu visto :', menu);
console.log('resultado  :', res, '|', Math.round((Date.now()-t0)/1000)+'s');
console.log('ultimo     :', (txt.split('\n').filter(l=>/WEBHB|STEP3|LOADSCREEN|loadlevel/.test(l)).pop()||'').slice(0,140));
process.exit(res==='ok'?0:1);
