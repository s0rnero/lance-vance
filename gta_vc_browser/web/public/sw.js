const VERSION = new URL(self.location.href).searchParams.get('v') || 'dev';
const CACHE = 'vc-' + VERSION;
const SHELL = ['/', '/index.html', '/manifest.webmanifest', '/icons/icon-192.png', '/icons/icon-512.png'];
const ISOLATE = {
  'Cross-Origin-Opener-Policy': 'same-origin',
  'Cross-Origin-Embedder-Policy': 'require-corp',
};

self.addEventListener('install', (event) => {
  event.waitUntil((async () => {
    const cache = await caches.open(CACHE);
    await Promise.all(SHELL.map((u) => cache.add(new Request(u, { cache: 'no-store' })).catch(() => {})));
    await self.skipWaiting();
  })());
});

self.addEventListener('activate', (event) => {
  event.waitUntil((async () => {
    const names = await caches.keys();
    await Promise.all(names.map((n) => (n.startsWith('vc-') && n !== CACHE ? caches.delete(n) : null)));
    await self.clients.claim();
  })());
});

async function grab(url) {
  const ctrl = new AbortController();
  const timer = setTimeout(() => { try { ctrl.abort(); } catch (e) {} }, 120000);
  try { return await fetch(url, { signal: ctrl.signal, cache: 'no-store' }); }
  finally { clearTimeout(timer); }
}

async function keep(cache, request, response) {
  try { if (response && response.ok && response.type === 'basic') await cache.put(request, response.clone()); } catch (e) {}
  return response;
}

async function fromCache(request) {
  const cache = await caches.open(CACHE);
  // 83.0-T9: /build/ va versionado con ?v=; ignorar la query servia el
  // motor viejo aunque la pagina ya fuera nueva. Match exacto.
  const hit = await cache.match(request);
  if (hit) return hit;
  const response = await fetch(request);
  return keep(cache, request, response);
}

async function fromNetwork(request) {
  const cache = await caches.open(CACHE);
  try {
    return await keep(cache, request, await fetch(request));
  } catch (e) {
    const hit = await cache.match(request, { ignoreSearch: true });
    if (hit) return hit;
    throw e;
  }
}

async function navigation(request) {
  const cache = await caches.open(CACHE);
  try {
    const response = await fetch(request);
    return await keep(cache, request, response);
  } catch (e) {
    const hit = (await cache.match(request, { ignoreSearch: true })) ||
      (await cache.match('/index.html')) || (await cache.match('/'));
    if (!hit) return new Response('Sin conexión y sin copia del juego.', { status: 503, headers: { 'Content-Type': 'text/plain; charset=utf-8' } });
    if (hit.headers.get('Cross-Origin-Embedder-Policy')) return hit;
    const headers = new Headers(hit.headers);
    for (const k of Object.keys(ISOLATE)) headers.set(k, ISOLATE[k]);
    return new Response(await hit.blob(), { status: hit.status, statusText: hit.statusText, headers });
  }
}

self.addEventListener('fetch', (event) => {
  const request = event.request;
  if (request.method !== 'GET') return;
  let url;
  try { url = new URL(request.url); } catch (e) { return; }
  if (url.origin !== self.location.origin) return;
  if (request.headers.has('range')) return;
  const path = url.pathname;
  // 77.4: los `.vcp` de /packs/ los guarda la descarga en IndexedDB; cachearlos
  // otra vez aquí duplicaba ~686 MB y apretaba la cuota del móvil.
  if (path.startsWith('/odtrace') || path.startsWith('/streamed/') || path.startsWith('/packs/')) return;
  if (path.startsWith('/build/')) { event.respondWith(fromCache(request)); return; }
  if (request.mode === 'navigate') { event.respondWith(navigation(request)); return; }
  event.respondWith(fromNetwork(request));
});

self.addEventListener('message', (event) => {
  const data = event.data || {};
  if (data.type === 'prefetch-shell') {
    event.waitUntil((async () => {
      const cache = await caches.open(CACHE);
      const urls = Array.isArray(data.urls) ? data.urls : [];
      await Promise.all(urls.map(async (u) => {
        try {
          if (await cache.match(u, { ignoreSearch: true })) return;
          const response = await grab(u);
          if (response && response.ok) await cache.put(u, response);
        } catch (e) {}
      }));
    })());
    return;
  }
  if (data.type !== 'prefetch-build') return;
  const client = event.source;
  const reply = (payload) => { try { if (client) client.postMessage(payload); } catch (e) {} };
  event.waitUntil((async () => {
    const cache = await caches.open(CACHE);
    const urls = Array.isArray(data.urls) ? data.urls : [];
    let done = 0;
    for (const u of urls) {
      try {
        const hit = await cache.match(u);
        if (!hit) {
          const response = await grab(u);
          if (response && response.ok) await cache.put(u, response);
        }
      } catch (e) {}
      done++;
      reply({ type: 'build', id: data.id, done, total: urls.length });
    }
    reply({ type: 'build', id: data.id, done, total: urls.length, finished: true });
  })());
});
