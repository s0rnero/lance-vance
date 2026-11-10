// Plugin de Vite para servir lo que la librería pide por HTTP:
//   /streamed/*  datos sueltos on-demand (fuera del root de Vite)
//   /vc/*        copia legal original (solo la mira la página si falta streamed)
//   /odtrace     trazas de depuración (POST) -> fichero
//   /streamed-status, /assets-status   estado sin 404s en consola
// Además añade COOP/COEP (SharedArrayBuffer = pthreads del motor).
//
// Lo usa la página de desarrollo de este repo y sirve tal cual para un host
// Vue:  import { vcWeb } from '@re3/gtavc-web/vite';  plugins: [vcWeb({...})]
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const norm = (u, d) => {
  const s = u === undefined || u === null || u === '' ? d : String(u);
  return s.endsWith('/') ? s : s + '/';
};

export function vcWeb(options = {}) {
  const here = path.dirname(fileURLToPath(import.meta.url));
  const webRoot = path.resolve(here, '..');                        // gta_vc_browser/web
  const streamedDir = path.resolve(options.streamedDir || path.join(webRoot, '..', 'streamed'));
  const assetsDir = path.resolve(options.assetsDir || path.join(webRoot, '..', 'assets'));
  const publicDir = path.resolve(options.publicDir || path.join(webRoot, 'public'));
  const streamedUrl = norm(options.streamedUrl, '/streamed/');
  const assetsUrl = norm(options.assetsUrl, '/vc/');
  const traceUrl = norm(options.traceUrl, '/odtrace');
  const traceFile = path.resolve(options.traceFile || 'odtrace.log');
  // La sesión anterior se conserva aquí (ver traceRoute): perder una partida
  // jugada al recargar deja sin evidencia los bloques que se validan por log.
  const prevTraceFile = traceFile.replace(/\.log$/i, '') + '.prev.log';
  // Archivo de sesiones: cada partida deja su propio fichero CON FECHA y no se
  // borra ni se sobreescribe nunca (el jugador juega partidas que son la
  // evidencia de varios bloques y hasta ahora sólo sobrevivían la actual y la
  // anterior). El nombre lleva la fecha/hora de INICIO de la sesión, que es lo
  // que identifica la partida; si dos sesiones empezaran en el mismo segundo se
  // añade -b, -c...
  const archiveDir = path.resolve(options.archiveDir || path.join(webRoot, '..', 'logs'));

  // Trazas POR SESIÓN: `/odtrace/<nombre>` escribe en `odtrace-<nombre>.log`.
  // Hacía falta: todas las sesiones (el jugador y cada sonda) escribían en el
  // MISMO fichero, así que dos pruebas a la vez —o la partida del jugador y una
  // sonda— se mezclaban. Con nombre, cada una tiene su fichero y su rotación.
  function trazaNombre(url) {
    const p = url.split('?')[0];
    const base = traceUrl.replace(/\/$/, '');
    if (p === base) return null;                       // la traza de siempre
    if (p.indexOf(base + '/') !== 0) return false;     // no es una traza
    const n = p.slice(base.length + 1).replace(/[^a-zA-Z0-9_-]/g, '').slice(0, 40);
    return n || null;
  }
  const trazaFichero = (n) => (n ? path.join(path.dirname(traceFile), 'odtrace-' + n + '.log') : traceFile);
  const trazaPrev = (n) => (n ? trazaFichero(n).replace(/\.log$/i, '') + '.prev.log' : prevTraceFile);

  // Fecha de inicio del trozo que se archiva: primer sello ISO del texto. Si no
  // aparece ninguno, se usa la hora de la rotación.
  function stampOf(texto) {
    const m = /(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2})/.exec(texto);
    const iso = m ? m[1] : new Date().toISOString().slice(0, 19);
    return iso.replace('T', '_').replace(/:/g, '-');
  }

  // [desde, hasta) del fichero de traza, en texto; null si no hay nada.
  function readRange(file, desde, hasta) {
    try {
      const st = fs.statSync(file);
      const a = Math.max(0, Math.min(desde, st.size));
      const b = Math.max(a, Math.min(hasta === undefined ? st.size : hasta, st.size));
      if (b - a <= 0) return null;
      const buf = Buffer.alloc(b - a);
      const fd = fs.openSync(file, 'r');
      fs.readSync(fd, buf, 0, buf.length, a);
      fs.closeSync(fd);
      return buf.toString('utf8');
    } catch (e) {
      return null;
    }
  }

  // logs/<base>.log. Si el nombre ya existe se añade -b, -c...: una partida
  // jugada no se sobreescribe nunca, es la evidencia de los bloques.
  function writeTo(base, texto) {
    try {
      fs.mkdirSync(archiveDir, { recursive: true });
      let dest = path.join(archiveDir, base + '.log');
      for (const suf of ['-b', '-c', '-d', '-e', '-f']) {
        if (!fs.existsSync(dest)) break;
        dest = path.join(archiveDir, base + suf + '.log');
      }
      fs.writeFileSync(dest, texto);
      return path.basename(dest);
    } catch (e) {
      return null;
    }
  }

  // Copia la traza que se está cerrando a logs/odtrace-<fecha>_<hora>.log.
  // Devuelve el nombre del archivo, o null si no había nada que archivar.
  function archiveSession(file) {
    const texto = readRange(file, 0);
    if (!texto) return null;
    return writeTo('odtrace-' + stampOf(texto), texto);
  }
  const logs = options.logs === true;                              // *.log al lado del cwd
  const isolated = options.crossOriginIsolated !== false;

  // Un fichero = una respuesta. `notFound` decide si se registra el 404.
  function serveDir(prefix, dir, tag) {
    return (req, res, next) => {
      if (req.method !== 'GET' && req.method !== 'HEAD') return next();
      const rel = decodeURIComponent(req.url.split('?')[0]);
      const abs = path.normalize(path.join(dir, rel));
      if (!abs.startsWith(dir)) { res.statusCode = 403; res.end(); return; }
      fs.stat(abs, (err, st) => {
        if (err || !st.isFile()) {
          if (logs) fs.appendFile(tag + '-404.log', new Date().toISOString() + ' 404 ' + rel + '\n', () => {});
          res.statusCode = 404; res.end('not found');
          return;
        }
        if (logs) fs.appendFile(tag + '-access.log', new Date().toISOString() + ' ' + st.size + ' ' + rel + '\n', () => {});
        res.setHeader('Content-Type', 'application/octet-stream');
        res.setHeader('Content-Length', st.size);
        // Sin caché: al regenerar datos (p.ej. sfx.SDT conserva tamaño) el
        // navegador seguía sirviendo la copia vieja durante 24 h.
        res.setHeader('Cache-Control', 'no-store');
        if (req.method === 'HEAD') { res.end(); return; }
        fs.createReadStream(abs).pipe(res);
      });
    };
  }

  // Estado en JSON (siempre 200): la página evita fetches fallidos en consola.
  function statusRoutes() {
    return (req, res, next) => {
      if (req.url === '/streamed-status') {
        const need = ['models/gta3.dir', 'Audio/sfx.SDT', 'data/main.scm', 'TEXT/spanish.gxt'];
        const missing = need.filter((f) => !fs.existsSync(path.join(streamedDir, f)));
        let manifest = false;
        try { manifest = fs.statSync(path.join(publicDir, 'manifest.json')).size > 1000; } catch (e) {}
        res.setHeader('Content-Type', 'application/json');
        res.end(JSON.stringify({ ok: missing.length === 0 && manifest, missing }));
        return;
      }
      if (req.url === '/assets-status') {
        res.setHeader('Content-Type', 'application/json');
        res.end(JSON.stringify({ ok: fs.existsSync(path.join(assetsDir, 'gta-vc.exe')) }));
        return;
      }
      next();
    };
  }

  // Trazas del motor: POST por lotes. El primer lote de una página nueva
  // (JS build=...) rota el fichero, así la sesión actual siempre está completa.
  // Cada PART ida=<n> que emite el motor cierra ADEMÁS el trozo de la partida
  // anterior, de forma que cada partida queda en su propio fichero de logs/ con
  // la fecha de inicio y el tag del paquete con el que se jugó. Sin tope de
  // líneas: el log vive en la máquina del jugador, el tamaño no es problema y
  // el `ODCAP drop` sí perdía evidencia a mitad de partida.
  function traceRoute() {
    let build = '';
    let partida = 0;
    let desde = 0;                    // bytes ya archivados del fichero vivo
    const slug = (s) => String(s || '').replace(/[^A-Za-z0-9._-]/g, '').slice(0, 40);
    function archivaParte(file, hasta, n, nombre) {
      const trozo = readRange(file, desde, hasta);
      if (!trozo) return null;
      const base = 'odtrace-' + stampOf(trozo) + '_p' + n +
        (build ? '_' + slug(build) : '') + (nombre ? '_' + slug(nombre) : '');
      return writeTo(base, trozo);
    }
    function tamano(file) {
      try { return fs.statSync(file).size; } catch (e) { return 0; }
    }
    return (req, res, next) => {
      const nombre = trazaNombre(req.url);
      if (nombre === false) return next();
      const file = trazaFichero(nombre);
      const prev = trazaPrev(nombre);
      if (req.method !== 'POST') { res.statusCode = 405; res.end(); return; }
      let body = '';
      req.on('data', (c) => { body += c; });
      req.on('end', () => {
        const ts = new Date().toISOString() + ' ';
        const inLines = body.split('\n');
        const arranque = inLines.find((ln) => ln.startsWith('JS build='));
        if (arranque) {
          const m = /build=(\S+)/.exec(arranque);
          const siguiente = m ? m[1] : '';
          // NO se borra la anterior: se copia a *.prev.log (compatibilidad con
          // las utilidades que ya lo leen) y, además, a logs/: la partida que
          // estaba abierta cierra su propio fichero y, si en esa página no se
          // llegó a jugar, se archiva la sesión entera.
          try {
            if (fs.existsSync(file) && fs.statSync(file).size > 0)
              fs.copyFileSync(file, prev);
          } catch (e) {}
          const guardada = partida >= 1 ? archivaParte(file, tamano(file), partida, nombre)
                                       : archiveSession(file);
          partida = 0;
          desde = 0;
          build = siguiente;
          try {
            fs.writeFileSync(file, ts + 'rotado: sesion nueva (anterior en '
              + path.basename(prev) + (guardada ? ', archivada en logs/' + guardada : '') + ')'
              + (nombre ? ' [traza ' + nombre + ']' : '') + '\n');
          } catch (e) {}
        }
        let out = '';
        const lotes = [];
        for (const ln of inLines) {
          if (!ln) continue;
          const pedazo = ts + ln + '\n';
          const m = /^PART ida=(\d+)/.exec(ln);
          lotes.push({ bytes: Buffer.byteLength(pedazo), part: m ? +m[1] : 0 });
          out += pedazo;
        }
        if (!out) { res.end('ok'); return; }
        let pos = tamano(file);
        try {
          fs.appendFileSync(file, out);
        } catch (e) {
          try { console.error('[odtrace] no se pudo escribir ' + file + ': ' + (e && e.message)); } catch (_) {}
        }
        let avisos = '';
        for (const l of lotes) {
          if (l.part > partida) {
            if (partida >= 1) {
              const hechas = archivaParte(file, pos, partida, nombre);
              if (hechas) avisos += ts + 'PARTLOG p' + partida + ' -> logs/' + hechas + '\n';
            }
            partida = l.part;
            desde = l.part === 1 ? 0 : pos;
          }
          pos += l.bytes;
        }
        if (avisos) { try { fs.appendFileSync(file, avisos); } catch (e) {} }
        res.end('ok');
      });
    };
  }
  const install = (server) => {
    server.middlewares.use(streamedUrl, serveDir(streamedUrl, streamedDir, 'streamed'));
    server.middlewares.use(assetsUrl, serveDir(assetsUrl, assetsDir, 'assets'));
    server.middlewares.use(statusRoutes());
    server.middlewares.use(traceRoute());
  };

  return {
    name: 'gtavc-web',
    config() {
      if (!isolated) return {};
      // SharedArrayBuffer (pthreads) sólo existe con la página aislada.
      return {
        server: { headers: { 'Cross-Origin-Opener-Policy': 'same-origin', 'Cross-Origin-Embedder-Policy': 'require-corp' } },
        preview: { headers: { 'Cross-Origin-Opener-Policy': 'same-origin', 'Cross-Origin-Embedder-Policy': 'require-corp' } },
      };
    },
    configureServer(server) { install(server); },
    configurePreviewServer(server) { install(server); },   // vite preview también sirve los datos
  };
}

export default vcWeb;
