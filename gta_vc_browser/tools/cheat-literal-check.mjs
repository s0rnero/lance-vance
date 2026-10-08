#!/usr/bin/env node
// Verificador de literales de cheat de src/core/Pad.cpp (sección 2).
//
// Por qué existe: los cheats de reVC no se comparan en claro. `Cheat_strncmp`
// compara la tecla N-ésima con `literal[i] - desplazamiento(i)`, y el buffer va
// con la ÚLTIMA tecla primero. Un literal mal contado (una letra de más o de
// menos) NO avisa: simplemente el cheat no dispara nunca y parece "que no
// funciona el código". Esta herramienta:
//
//   1. lee la tabla de desplazamientos del propio Pad.cpp (no la copia),
//   2. decodifica TODOS los literales que ya hay (cheat chain + _CHEATCMP),
//   3. con --name=XXXX imprime el literal exacto para pegar, con escapes de C,
//   4. y avisa de dos trampas reales: nombre ya usado, y literales ANTERIORES en
//      la cadena que sean prefijo del mío (el `else if` los evalúa antes: teclear
//      el cheat dispararía el otro).
//
// Uso:
//   node gta_vc_browser/tools/cheat-literal-check.mjs                 # lista lo que hay
//   node gta_vc_browser/tools/cheat-literal-check.mjs --name=CRAZYCOP # literal + choques

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.join(HERE, '..', '..');
const PAD = process.env.VC_PAD || path.join(ROOT, 'src', 'core', 'Pad.cpp');

const args = process.argv.slice(2);
const argOf = (n) => {
  const a = args.find((x) => x.startsWith('--' + n + '='));
  return a ? a.slice(n.length + 3) : null;
};

const src = fs.readFileSync(PAD, 'utf8');

// 1) tabla de desplazamientos, tal cual está en el código
const offsets = [];
for (const m of src.matchAll(/case\s+(\d+):\s*ccmp\((\d+)\);\s*break;/g))
  offsets[+m[1]] = +m[2];
if (!offsets.length) {
  console.error('No pude leer la tabla ccmp() de ' + PAD + '.');
  process.exit(2);
}
const maxLen = offsets.length;   // la tabla cubre las posiciones 0..maxLen-1

// 2) todos los literales, en orden de aparición (importa: la cadena es un else if)
const RE_C = /Cheat_strncmp\(\s*KeyBoardCheatString\s*,\s*"((?:[^"\\]|\\.)*)"\s*\)/g;
const RE_M = /_CHEATCMP\(\s*"((?:[^"\\]|\\.)*)"\s*\)/g;
const lit = [];
for (const re of [RE_C, RE_M])
  for (const m of src.matchAll(re)) lit.push({ raw: m[1], at: m.index, macro: re === RE_M });
lit.sort((a, b) => a.at - b.at);

// 3) desescapar (\" \\ \n ...) y decodificar
const unescape = (s) => s.replace(/\\(x[0-9a-fA-F]{2}|[0-7]{1,3}|.)/g, (_, e) =>
  e[0] === 'x' ? String.fromCharCode(parseInt(e.slice(1), 16))
    : /^[0-7]+$/.test(e) ? String.fromCharCode(parseInt(e, 8))
      : { n: '\n', r: '\r', t: '\t', '0': '\0' }[e] ?? e);

const decode = (s) => {
  const out = [];
  for (let i = 0; i < s.length; i++) {
    if (offsets[i] === undefined) return { name: null, why: 'literal de ' + s.length + ' bytes pero la tabla ccmp() sólo llega a ' + offsets.length };
    out.push(String.fromCharCode(s.charCodeAt(i) - offsets[i]));
  }
  return { name: out.reverse().join(''), why: null };
};

const encode = (name) => {
  const rev = [...name].reverse();
  return rev.map((c, i) => {
    if (offsets[i] === undefined) throw new Error('nombre demasiado largo: la tabla ccmp() llega a ' + offsets.length);
    return String.fromCharCode(c.charCodeAt(0) + offsets[i]);
  }).join('');
};

const cEscape = (s) => s.replace(/\\/g, '\\\\').replace(/"/g, '\\"')
  .replace(/\n/g, '\\n').replace(/\r/g, '\\r').replace(/\t/g, '\\t');

const decoded = lit.map((l) => {
  const s = unescape(l.raw);
  const { name, why } = decode(s);
  return { ...l, s, name, why };
});

console.log('== ' + PAD);
console.log('== tabla ccmp(): ' + offsets.filter((v) => v !== undefined).join(',') + ' (' + maxLen + ' posiciones)');
console.log('== ' + decoded.length + ' literales en el fichero (' + decoded.filter((d) => d.macro).length + ' con _CHEATCMP, el resto en la cadena principal)');

if (args.includes('-v') || args.includes('--verbose')) {
  for (const d of decoded)
    console.log('   ' + String(d.raw.length).padStart(2) + 'B  ' + cEscape(d.raw).padEnd(30) +
      ' -> ' + (d.name === null ? '?? ' + d.why : d.name) + (d.macro ? '   [_CHEATCMP]' : ''));
}

const name = argOf('name');
if (!name) {
  const bad = decoded.filter((d) => d.name === null);
  if (bad.length) {
    console.log('\n⚠ ' + bad.length + ' literal(es) que la tabla no puede decodificar (¿longitud mal contada?):');
    for (const d of bad) console.log('   ' + cEscape(d.raw) + ' — ' + d.why);
  }
  console.log('\nPara sacar un literal nuevo: --name=CRAZYCOP');
  process.exit(0);
}

const mine = encode(name);
console.log('\n== candidato: "' + name + '"');
console.log('   literal para pegar:  "' + cEscape(mine) + '"   (' + mine.length + ' bytes, ' + name.length + ' teclas' + (mine.length === name.length ? '' : '  ⚠ DESCUADRE') + ')');
console.log('   comprobación: decodificando mi propio literal -> ' + decode(mine).name);

const base = name.toLowerCase();
const dup = decoded.filter((d) => d.name && d.name.toLowerCase() === base && d.s !== mine);
if (dup.length) console.log('   ⚠ ese nombre ya existe en el fichero (literal "' + cEscape(dup[0].raw) + '"): uno de los dos no disparará');

const shadows = decoded.filter((d) => d.s && d.s.length <= mine.length && mine.startsWith(d.s) && d.s !== mine);
if (!shadows.length) {
  console.log('   sin choques de prefijo: ningún cheat ANTERIOR en la cadena empieza igual (no te lo robará otro).');
} else {
  console.log('   ⚠ ' + shadows.length + ' cheat(s) anterior(es) con literal PREFIJO del mío (dispararían ellos primero):');
  for (const d of shadows) console.log('      "' + cEscape(d.raw) + '"' + (d.name ? ' -> ' + d.name : '') + (d.macro ? ' [_CHEATCMP]' : ''));
}
