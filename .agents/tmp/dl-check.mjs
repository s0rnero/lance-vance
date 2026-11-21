const DB_NAME = 'vcod2';
const DB_VERSION = 2;
const MARKER = '__cachekey__';
const CONCURRENCY = 4;
const BIG_MB = 24;
const MIB = 1048576;

function openDb() {
  return new Promise((res) => {
    try {
      const q = indexedDB.open(DB_NAME, DB_VERSION);
      q.onupgradeneeded = (e) => {
        const db = e.target.result;
        if (!db.objectStoreNames.contains('files')) db.createObjectStore('files');
        if (!db.objectStoreNames.contains('meta')) db.createObjectStore('meta');
      };
      q.onsuccess = (e) => res(e.target.result);
      q.onerror = () => res(null);
      q.onblocked = () => res(null);
    } catch (e) { res(null); }
  });
}

function readAll(db, store) {
  return new Promise((res) => {
    try {
      const tx = db.transaction([store], 'readonly');
      const st = tx.objectStore(store);
      const kq = st.getAllKeys();
      const vq = st.getAll();
      let keys = null;
      let vals = null;
      const out = () => {
        if (!keys || !vals) return;
        res(keys.map((k, i) => [k, vals[i]]));
      };
      kq.onsuccess = () => { keys = kq.result || []; out(); };
      vq.onsuccess = () => { vals = vq.result || []; out(); };
      tx.onerror = () => res([]);
    } catch (e) { res([]); }
  });
}

function writeEntry(db, key, buf, meta) {
  return new Promise((res) => {
    try {
      const tx = db.transaction(['files', 'meta'], 'readwrite');
      tx.objectStore('files').put(buf, key);
      tx.objectStore('meta').put(meta, key);
      tx.oncomplete = () => res(true);
      tx.onerror = () => res(false);
      tx.onabort = () => res(false);
    } catch (e) { res(false); }
  });
}

function dropEntry(db, key) {
  return new Promise((res) => {
    try {
      const tx = db.transaction(['files', 'meta'], 'readwrite');
      tx.objectStore('files').delete(key);
      tx.objectStore('meta').delete(key);
      tx.oncomplete = () => res(true);
      tx.onerror = () => res(false);
    } catch (e) { res(false); }
  });
}

// 79.0: borrar de uno en uno eran ~6.000 transacciones de IndexedDB seguidas y
// eso se notaba en el arranque del movil (23 s solo para dejar limpio lo que no
// cuadraba). Ahora van en lotes de 512 dentro de una sola transaccion.
function dropEntries(db, keys) {
  return new Promise((res) => {
    try {
      const tx = db.transaction(['files', 'meta'], 'readwrite');
      const f = tx.objectStore('files');
      const m = tx.objectStore('meta');
      for (const key of keys) { f.delete(key); m.delete(key); }
      tx.oncomplete = () => res(true);
      tx.onerror = () => res(false);
      tx.onabort = () => res(false);
    } catch (e) { res(false); }
  });
}

function writeMarker(db, value) {
  return new Promise((res) => {
    try {
      const tx = db.transaction(['files'], 'readwrite');
      tx.objectStore('files').put(value, MARKER);
      tx.oncomplete = () => res(true);
      tx.onerror = () => res(false);
    } catch (e) { res(false); }
  });
}

function readMarker(db) {
  return new Promise((res) => {
    try {
      const q = db.transaction(['files'], 'readonly').objectStore('files').get(MARKER);
      q.onsuccess = () => res(q.result || '');
      q.onerror = () => res('');
    } catch (e) { res(''); }
  });
}

function isCurrent(meta, item) {
  if (!meta) return false;
  const size = meta.s || 0;
  if (item.s && size !== item.s) return false;
  if (item.h) return meta.h === item.h;
  return size > 0;
}

export async function downloadGame({ manifest, streamedUrl, packsUrl, log, onProgress }) {
  const entries = [];
  for (const k of Object.keys(manifest || {})) {
    if (k === '__vc__') continue;
    const e = manifest[k];
    if (!e || !e.p) continue;
    entries.push({
      key: '/' + e.p, rel: e.p, s: e.s || 0, h: e.h || '',
      pk: typeof e.pk === 'number' ? e.pk : -1,
      po: typeof e.po === 'number' ? e.po : 0,
    });
  }
  const seal = manifest && manifest.__vc__ && manifest.__vc__;
  const packs = (seal && seal.packs) || [];
  const packBase = packsUrl || streamedUrl;
  const t0 = Date.now();
  const total = entries.length;
  const totalBytes = entries.reduce((a, e) => a + e.s, 0);
  // 79.0: estas lineas solo iban al DOM (#gamelog) y no llegaban a la traza del
  // movil, asi que "cuantos ficheros tiro el sello nuevo" era invisible. Ahora
  // van tambien a la cola __odq como el resto de marcas.
  const say = (t) => {
    try { if (typeof window !== 'undefined') (window.__odq = window.__odq || []).push(t); } catch (e) {}
    if (log) { try { log(t); } catch (e) {} }
  };
  let done = 0;
  let bytes = 0;
  let failed = 0;
  let offline = false;

  const emit = () => {
    if (!onProgress) return;
    try { onProgress({ done, total, bytes, totalBytes, failed }); } catch (e) {}
  };

  const db = await openDb();
  if (!db) {
    say('[lib] sin IndexedDB: no se puede guardar el juego para jugar sin conexión');
    return { total, done: 0, bytes: 0, totalBytes, failed: total, offline: true, complete: false };
  }

  const tMeta = Date.now();
  const metas = new Map(await readAll(db, 'meta'));
  say('[lib] metadatos ' + metas.size + ' en ' + (Date.now() - tMeta) + 'ms');

  // 78.0: el sello sube cada vez que se regenera el manifiesto (basta con que
  // cambie UN fichero), y `OD.idbPurgeIfStale` del motor borra la cache ENTERA
  // al verlo distinto. Eso obligaba al movil a bajar los 1.184 MB otra vez por
  // un par de ficheros. Cada entrada lleva su propio hash, asi que aqui se tira
  // solo lo que ya no coincide y se escribe el sello ANTES de arrancar el
  // motor: su purga ya no tiene nada que hacer.
  if (seal && seal.h) {
    const sealKey = 'm' + seal.h;
    const prev = await readMarker(db);
    if (prev !== sealKey) {
      const byKey = new Map(entries.map((e) => [e.key, e]));
      const tDrop = Date.now();
      let fuera = 0;
      let lote = [];
      for (const [k, meta] of metas) {
        const item = byKey.get(k);
        if (item && isCurrent(meta, item)) continue;
        lote.push(k);
        metas.delete(k);
        fuera++;
        if (lote.length >= 512) { await dropEntries(db, lote); lote = []; }
      }
      if (lote.length) await dropEntries(db, lote);
      await writeMarker(db, sealKey);
      say('[lib] sello nuevo ' + (prev || 'ninguno') + ' -> ' + sealKey + ': ' + fuera +
        ' ficheros fuera, ' + metas.size + ' se quedan (' + (Date.now() - tDrop) + 'ms)');
    }
  }

  const pending = [];
  for (const e of entries) {
    if (isCurrent(metas.get(e.key), e)) { done++; bytes += e.s; }
    else pending.push(e);
  }
  emit();

  const packJobs = new Map();
  const sueltos = [];
  for (const e of pending) {
    if (e.pk >= 0 && packs[e.pk]) {
      if (!packJobs.has(e.pk)) packJobs.set(e.pk, []);
      packJobs.get(e.pk).push(e);
    } else {
      sueltos.push(e);
    }
  }
  say('[lib] descarga inicial: ' + (total - pending.length) + '/' + total + ' ya en el navegador, faltan ' +
    pending.length + ' (' + Math.round((totalBytes - bytes) / MIB) + ' MB en ' + packJobs.size + ' paquetes y ' +
    sueltos.length + ' sueltos)');

  let lastReport = 0;
  const report = (force) => {
    const now = Date.now();
    if (!force && now - lastReport < 250) return;
    lastReport = now;
    emit();
  };

  const fail = async (e) => {
    failed++;
    if (metas.has(e.key)) await dropEntry(db, e.key);
    report(true);
  };

  const one = async (e) => {
    if (offline) return;
    const url = streamedUrl + e.rel.split('/').map(encodeURIComponent).join('/');
    let res;
    try {
      res = await fetch(url, { cache: 'no-store' });
    } catch (err) {
      await fail(e);
      if (failed >= 6 && bytes === 0) offline = true;
      return;
    }
    if (!res.ok || (res.headers.get('content-type') || '').toLowerCase().includes('text/html')) {
      await fail(e);
      return;
    }
    let buf;
    try { buf = await res.arrayBuffer(); } catch (err) { await fail(e); return; }
    if (!buf.byteLength) { await fail(e); return; }
    const ok = await writeEntry(db, e.key, buf, { t: Date.now(), s: buf.byteLength, h: e.h });
    if (!ok) { await fail(e); return; }
    done++;
    bytes += e.s || buf.byteLength;
    report(false);
  };

  const onePack = async (pk, items) => {
    if (offline) return;
    const pack = packs[pk];
    let buf = null;
    try {
      const res = await fetch(packBase + encodeURIComponent(pack.n), { cache: 'no-store' });
      if (res.ok) buf = await res.arrayBuffer();
    } catch (err) {
      buf = null;
    }
    if (!buf || buf.byteLength < pack.s) {
      for (const e of items) await one(e);
      return;
    }
    for (const e of items) {
      if (offline) return;
      if (e.po + e.s > buf.byteLength) { await fail(e); continue; }
      const pieza = buf.slice(e.po, e.po + e.s);
      const ok = await writeEntry(db, e.key, pieza, { t: Date.now(), s: e.s, h: e.h });
      if (!ok) { await fail(e); continue; }
      done++;
      bytes += e.s;
      report(false);
    }
  };

  for (const [pk, items] of packJobs) {
    if (offline) break;
    await onePack(pk, items);
  }
  report(true);

  const small = sueltos.filter((e) => e.s <= BIG_MB * MIB).sort((a, b) => a.s - b.s);
  const big = sueltos.filter((e) => e.s > BIG_MB * MIB).sort((a, b) => a.s - b.s);

  let next = 0;
  const lane = async () => {
    while (next < small.length && !offline) {
      const e = small[next++];
      await one(e);
    }
  };
  await Promise.all(Array.from({ length: Math.min(CONCURRENCY, small.length) }, lane));
  for (const e of big) {
    if (offline) break;
    await one(e);
  }
  report(true);

  if (seal && seal.h) await writeMarker(db, 'm' + seal.h);
  const complete = failed === 0;
  say('[lib] descarga inicial ' + (complete ? 'completa' : 'con fallos') + ': ' + done + '/' + total +
    ' ficheros, ' + Math.round(bytes / MIB) + ' MB' + (failed ? ', fallos=' + failed : '') +
    (offline ? ', sin conexión' : '') + ' en ' + (Date.now() - t0) + 'ms');
  return { total, done, bytes, totalBytes, failed, offline, complete };
}
