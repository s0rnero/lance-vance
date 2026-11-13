import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { vcWeb } from './lib/vite.js';

const webRoot = path.dirname(fileURLToPath(import.meta.url));
const publicDir = path.join(webRoot, 'public');
const PORT = process.env.PORT ? Number(process.env.PORT) : 2077;

const mids = [];
vcWeb({ logs: false, traceFile: path.join(webRoot, 'odtrace.log') }).configureServer({
  middlewares: {
    use: (a, b) => {
      if (typeof a === 'function') mids.push({ route: null, fn: a });
      else mids.push({ route: a, fn: b });
    },
  },
});

const MIME = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.mjs': 'text/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.wasm': 'application/wasm',
  '.data': 'application/octet-stream',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
  '.mp3': 'audio/mpeg',
  '.wav': 'audio/wav',
  '.txt': 'text/plain; charset=utf-8',
  '.gxt': 'application/octet-stream',
  '.dff': 'application/octet-stream',
  '.txd': 'application/octet-stream',
  '.ifp': 'application/octet-stream',
  '.img': 'application/octet-stream',
  '.dir': 'application/octet-stream',
  '.ide': 'application/octet-stream',
  '.ipl': 'application/octet-stream',
  '.dat': 'application/octet-stream',
  '.scm': 'application/octet-stream',
  '.adf': 'application/octet-stream',
  '.sdt': 'application/octet-stream',
};

function sendFile(abs, st, req, res) {
  res.setHeader('Content-Type', MIME[path.extname(abs).toLowerCase()] || 'application/octet-stream');
  res.setHeader('Content-Length', st.size);
  res.setHeader('Cache-Control', 'no-store');
  if (req.method === 'HEAD') { res.end(); return; }
  const rs = fs.createReadStream(abs);
  rs.on('error', () => { try { res.destroy(); } catch (e) {} });
  rs.pipe(res);
}

function tryStat(abs, req, res, onMiss) {
  fs.stat(abs, (err, st) => {
    if (!err && st.isFile()) { sendFile(abs, st, req, res); return; }
    onMiss();
  });
}

function staticServe(req, res) {
  let urlPath = decodeURIComponent(req.url.split('?')[0]);
  if (urlPath === '/') urlPath = '/index.html';
  const abs = path.normalize(path.join(webRoot, urlPath));
  if (!abs.startsWith(webRoot)) { res.statusCode = 403; res.end(); return; }
  tryStat(abs, req, res, () => {
    const pub = path.normalize(path.join(publicDir, urlPath));
    if (!pub.startsWith(publicDir)) { res.statusCode = 404; res.end('not found'); return; }
    tryStat(pub, req, res, () => { res.statusCode = 404; res.end('not found'); });
  });
}

const server = http.createServer((req, res) => {
  res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
  res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
  const origUrl = req.url;
  let i = 0;
  const next = () => {
    req.url = origUrl;
    if (i >= mids.length) { staticServe(req, res); return; }
    const m = mids[i++];
    if (m.route) {
      const base = m.route.replace(/\/$/, '');
      if (req.url === base || req.url.startsWith(base + '/') || req.url.startsWith(base + '?')) {
        req.url = req.url.slice(base.length) || '/';
        m.fn(req, res, next);
        return;
      }
      next();
      return;
    }
    m.fn(req, res, next);
  };
  next();
});

server.listen(PORT, () => {
  console.log('[serve] GTA VC estatico concurrente en http://localhost:' + PORT);
});