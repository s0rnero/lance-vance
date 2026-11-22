// Capa on-demand (pre-js): ficheros sueltos bajo /streamed/ + caché IDB.
//
// El motor pide ficheros por su ruta (p. ej. "MODELS\GTA3.IMG" o
// "models/gta3.img/cop.dff"). OD.ensure() los resuelve contra el
// manifiesto, los trae (IDB primero, red después), los escribe en MEMFS
// y devuelve la ruta canónica. Se invoca desde C vía EM_ASYNC_JS, así el
// hilo principal nunca se bloquea: la página sigue viva durante la descarga.
// Fuente del worker de carga (Nivel 1). Va embebida aquí y se crea con un
// Blob: así el port no necesita servir un fichero extra ni saber dónde está.
function odLog(s) {
  try { (globalThis.__odq = globalThis.__odq || []).push(String(s)); } catch (e) {}
}
// 83.1: sondas INMEDIATAS por sendBeacon. `__odq` solo se vacia cuando el
// motor habla (el flush lo llama el), asi que en un cuelgue sus lineas se
// quedan dentro. Estas salen en el acto aunque el hilo del motor este parado
// (los CPUBEACON del motor siguen llegando en pleno cuelgue: esta probado).
var OD_PROBE = /particle\.cfg|ped\.dat/i;
function odProbe(s) {
  try {
    s = String(s);
    // Solo las sondas del fichero bajo estudio (o los hitos del arranque de
    // particulas/peds): una sonda por acceso llenaba el log y el navegador
    // acaba descartando las de sendBeacon.
    if (!OD_PROBE.test(s) && !/^(PD |PART |LF )/.test(s)) return;
    var u = (typeof OD !== 'undefined' && OD.cfg && OD.cfg.odtraceUrl) || null;
    if (!u) return;
    var line = 'ODB ' + (Date.now() % 100000) + ' ' + s.slice(0, 240);
    // XHR sincrono: sendBeacon se descarta en silencio cuando se acumula, y
    // justo la ultima linea (la que dice donde se clava) es la que importa.
    var x = new XMLHttpRequest();
    x.open('POST', u, false);
    x.send(line);
  } catch (e) {}
}

var WORKER_SRC = `
var DB = null, BASE = '/streamed/', INFLIGHT = {};
function openDb() {
  return new Promise(function (res) {
    try {
      var q = indexedDB.open('vcod2', 2);
      q.onupgradeneeded = function (e) {
        var db = e.target.result;
        if (!db.objectStoreNames.contains('files')) db.createObjectStore('files');
        if (!db.objectStoreNames.contains('meta')) db.createObjectStore('meta');
      };
      q.onsuccess = function (e) { res(e.target.result); };
      q.onerror = function () { res(null); };
    } catch (e) { res(null); }
  });
}
function idbGet(k) {
  return new Promise(function (res) {
    if (!DB) return res(null);
    try {
      var q = DB.transaction(['files'], 'readonly').objectStore('files').get(k);
      q.onsuccess = function (e) { res(e.target.result || null); };
      q.onerror = function () { res(null); };
    } catch (e) { res(null); }
  });
}
function idbPut(k, buf) {
  return new Promise(function (res) {
    if (!DB) return res();
    try {
      var t = DB.transaction(['files', 'meta'], 'readwrite');
      t.objectStore('files').put(buf, k);
      t.objectStore('meta').put({ t: Date.now(), s: buf.byteLength }, k);
      t.oncomplete = function () { res(); };
      t.onerror = function () { res(); };
    } catch (e) { res(); }
  });
}
// Caché en OPFS: dentro del worker la lectura es síncrona (10-20 ms por
// fichero) en vez de la ida y vuelta asíncrona a IndexedDB (150-500 ms
// medidos en la sesión 81.5). Si el navegador no la da, todo sigue por IDB.
var OPFS = null, OPFS_ON = false;
// 82.2: a partir de este tamano las lecturas/escrituras de OPFS van por la via
// ASINCRONA. Medido en la traza de 82.1: 'anim/cuts.img' (110 MB) leido con el
// handle SINCRONO bloqueo el worker ~4 s; detras de el esperaron 'camera.txd'
// (73 KB, 3.921 ms en el worker), 'bopen.mp3' (812 ms) y 'sfx/8.mp3' (3.057 ms),
// y el motor se quedo con un frame de 4.085 ms al empezar INT_A (3 FPS y luego
// 30 FPS en el contador). Con la via asincrona el worker sigue atendiendo
// peticiones pequenas mientras el fichero gordo se lee o se persiste.
var BIGIO = 4 * 1048576;
function opfsName(k) {
  var h1 = 0x811c9dc5, h2 = 0x1000193;
  for (var i = 0; i < k.length; i++) {
    var c = k.charCodeAt(i);
    h1 = ((h1 ^ c) * 0x01000193) >>> 0;
    h2 = ((h2 ^ (c + 0x9e37)) * 0x85ebca6b) >>> 0;
  }
  return h1.toString(16) + '-' + h2.toString(16);
}
function opfsGet(k) {
  return new Promise(function (res) {
    if (!OPFS_ON || !OPFS) return res(null);
    try {
      OPFS.getFileHandle(opfsName(k)).then(function (fh) {
        return fh.getFile().then(function (f) {
          var n = f.size || 0;
          if (!n) return null;
          // 82.2: los gordos por 'File.arrayBuffer()' (no bloquea el bucle de
          // eventos del worker). Los pequenos siguen por el handle sincrono,
          // que es el camino rapido (10-20 ms por fichero).
          if (n > BIGIO) return f.arrayBuffer();
          return fh.createSyncAccessHandle().then(function (h) {
            var out = null;
            try {
              var buf = new ArrayBuffer(n);
              if (h.read(new Uint8Array(buf), { at: 0 }) === n) out = buf;
            } catch (e) {}
            try { h.close(); } catch (e) {}
            return out;
          });
        });
      }).then(function (b) { res(b || null); }).catch(function () { res(null); });
    } catch (e) { res(null); }
  });
}
function opfsPut(k, buf) {
  return new Promise(function (res) {
    if (!OPFS_ON || !OPFS) return res();
    try {
      var big = (buf && buf.byteLength || 0) > BIGIO;
      OPFS.getFileHandle(opfsName(k), { create: true }).then(function (fh) {
        // 82.2: 'h.flush()' de un fichero gordo es un fsync real y deja al
        // worker sin atender peticiones varios segundos. createWritable()
        // hace la misma persistencia sin bloquear el bucle de eventos.
        if (big && fh.createWritable) {
          return fh.createWritable().then(function (w) {
            return w.write(buf).then(function () { return w.close(); });
          });
        }
        return fh.createSyncAccessHandle().then(function (h) {
          try { h.truncate(0); h.write(new Uint8Array(buf), { at: 0 }); h.flush(); } catch (e) {}
          try { h.close(); } catch (e) {}
        });
      }).then(function () { res(); }).catch(function () { res(); });
    } catch (e) { res(); }
  });
}
function opfsClear() {
  return new Promise(function (res) {
    if (!OPFS) return res();
    var it;
    try { it = OPFS.keys(); } catch (e) { return res(); }
    function step() {
      it.next().then(function (r) {
        if (r.done) return res();
        OPFS.removeEntry(r.value).catch(function () {}).then(step);
      }).catch(function () { res(); });
    }
    step();
  });
}
function opfsInit(ver) {
  return new Promise(function (res) {
    try {
      if (!navigator.storage || !navigator.storage.getDirectory) return res(false);
      navigator.storage.getDirectory().then(function (root) {
        return root.getDirectoryHandle('vcod', { create: true });
      }).then(function (dir) {
        OPFS = dir;
        OPFS_ON = true;
        return opfsGet('.ver').then(function (buf) {
          var cur = '';
          if (buf) {
            var u = new Uint8Array(buf, 0, Math.min(64, buf.byteLength));
            cur = String.fromCharCode.apply(null, u);
          }
          if (cur === String(ver)) return true;
          return opfsClear().then(function () {
            return opfsPut('.ver', new TextEncoder().encode(String(ver)).buffer);
          }).then(function () { return true; });
        });
      }).then(function (ok) { res(!!ok); }).catch(function () { OPFS_ON = false; res(false); });
    } catch (e) { OPFS_ON = false; res(false); }
  });
}
// 81.7: la caché nace en IndexedDB y la primera sesión con OPFS lee todo de
// IDB (200-1400 ms por fichero, medido en 81.6). Se copia a OPFS por lotes de
// 12 en segundo plano, cediendo ante cualquier petición del motor.
var MIG = { n: 0, bytes: 0, next: null, done: false };
function migSay(done) {
  postMessage({ op: 'mig', n: MIG.n, mb: Math.round(MIG.bytes / 1048576), done: done ? 1 : 0 });
}
function opfsHas(k) {
  return new Promise(function (res) {
    if (!OPFS_ON || !OPFS) return res(false);
    try {
      OPFS.getFileHandle(opfsName(k)).then(function () { res(true); }).catch(function () { res(false); });
    } catch (e) { res(false); }
  });
}
function migWrite(batch, more) {
  var i = 0;
  function step() {
    if (i >= batch.length) {
      migSay(false);
      if (!more) { MIG.done = true; migSay(true); return; }
      setTimeout(migStep, 60);
      return;
    }
    var k = batch[i][0], v = batch[i][1];
    i++;
    opfsHas(k).then(function (yes) {
      if (yes) { MIG.n++; step(); return; }
      opfsPut(k, v).then(function () {
        MIG.n++;
        MIG.bytes += (v && v.byteLength) || 0;
        step();
      });
    });
  }
  step();
}
function migStep() {
  if (!OPFS_ON || !DB || MIG.done) return;
  if (MIG.bytes >= 512 * 1048576) { MIG.done = true; migSay(true); return; }
  if (Object.keys(INFLIGHT).length) { setTimeout(migStep, 300); return; }
  var batch = [];
  try {
    var tx = DB.transaction(['files'], 'readonly');
    var st = tx.objectStore('files');
    var range = MIG.next !== null ? IDBKeyRange.lowerBound(MIG.next, true) : null;
    var req = range ? st.openCursor(range) : st.openCursor();
    req.onsuccess = function (e) {
      var cur = e.target.result;
      if (!cur) { MIG.done = true; migWrite(batch, false); return; }
      if (batch.length >= 12) { migWrite(batch, true); return; }
      if (typeof cur.key === 'string') batch.push([cur.key, cur.value]);
      MIG.next = cur.key;
      cur.continue();
    };
    req.onerror = function () { MIG.done = true; migSay(true); };
  } catch (e) { MIG.done = true; }
}
function isHtml(buf) {
  try {
    var u = new Uint8Array(buf, 0, Math.min(9, buf.byteLength));
    var s = String.fromCharCode.apply(null, u).toLowerCase();
    return s.indexOf('<!doctype') === 0 || s.indexOf('<html') === 0;
  } catch (e) { return false; }
}
function fromNet(key, rel, size) {
  return new Promise(function (res, rej) {
    var ctrl = new AbortController();
    var timer = setTimeout(function () { try { ctrl.abort(); } catch (e) {} }, 25000);
    var url = BASE + rel.split('/').map(encodeURIComponent).join('/');
    fetch(url, { signal: ctrl.signal }).then(function (r) {
      if (!r.ok) throw new Error('HTTP ' + r.status);
      var ct = (r.headers.get('content-type') || '').toLowerCase();
      if (ct.indexOf('text/html') >= 0) throw new Error('HTML en vez de fichero');
      return r.arrayBuffer();
    }).then(function (buf) {
      clearTimeout(timer);
      if (!buf || !buf.byteLength) throw new Error('fichero vacio');
      if (isHtml(buf)) throw new Error('HTML en vez de fichero');
      if (size && buf.byteLength < size) throw new Error('truncado ' + buf.byteLength + '/' + size);
      // 82.2: un fichero gordo se entrega YA (se copia para que la escritura
      // no se quede con el buffer desligado) y no se duplica en IndexedDB:
      // esperar la persistencia era parte del bloqueo de 4 s del arranque.
      // 82.7: se espera a que este en OPFS antes de entregar; si el motor lo
      // vuelve a pedir antes, opfsGet fallaba y se bajaba otra vez (dos
      // ODWSLOW src=net con /Audio/EMOTION.ADF en la traza).
      if (buf.byteLength > 16 * 1048576) {
        opfsPut(key, buf).then(function () { res({ buf: buf.slice(0), src: 'net' }); })
          .catch(function () { res({ buf: buf.slice(0), src: 'net' }); });
        return;
      }
      idbPut(key, buf);
      // La escritura en OPFS se espera: el buffer se transfiere al hilo del
      // juego justo después y quedaría desligado a mitad de escritura.
      opfsPut(key, buf).then(function () { res({ buf: buf, src: 'net' }); });
    }).catch(function (e) { clearTimeout(timer); rej(e); });
  });
}
onmessage = function (ev) {
  var m = ev.data || {};
  if (m.op === 'init') {
    if (m.streamedUrl) BASE = m.streamedUrl;
    openDb().then(function (db) { DB = db; return opfsInit(m.ver); })
      .then(function (ok) { postMessage({ op: 'ready', opfs: ok ? 1 : 0 }); });
    return;
  }
  if (m.op === 'migrate') { migStep(); return; }
  if (m.op !== 'get') return;
  var k = m.key;
  if (INFLIGHT[k]) return;
  INFLIGHT[k] = 1;
  var t0 = Date.now();
  opfsGet(k).then(function (c) {
    if (c && c.byteLength && !isHtml(c) && (!m.size || c.byteLength === m.size)) return { buf: c, src: 'opfs' };
    return idbGet(k).then(function (c2) {
      if (c2 && c2.byteLength && !isHtml(c2) && (!m.size || c2.byteLength === m.size)) {
        // Copia de la caché vieja: se migra a OPFS al primer uso.
        return opfsPut(k, c2).then(function () { return { buf: c2, src: 'idb' }; });
      }
      // REVERTIDO a como iba en 82.5-T1: si no esta en cache se pide a la red
      // (y se guarda). Quitar esto dejaba el arranque sin respaldo cuando la
      // descarga inicial no habia traido un fichero.
      return fromNet(k, m.rel || k, m.size);
    });
  }).then(function (r) {
    delete INFLIGHT[k];
    postMessage({ op: 'data', key: k, buf: r.buf, src: r.src, ms: Date.now() - t0 }, [r.buf]);
  }).catch(function (e) {
    delete INFLIGHT[k];
    postMessage({ op: 'err', key: k, msg: String((e && e.message) || e), ms: Date.now() - t0 });
  });
};
`;

var OD = {
  // Versión de build: una línea al arrancar para saber QUÉ código corre
  // (adiós confusión de cachés). La manda la librería por __VC_CFG.version
  // (una sola fuente para el wasm y el JS); si no hay config, la de siempre.
  buildTag: (function () {
    try { return (globalThis.__VC_CFG && globalThis.__VC_CFG.version) || '2026-09-18-lib1'; }
    catch (e) { return '2026-09-18-lib1'; }
  })(),
  // Versión de DATOS: si cambia, se vacía la caché IDB antes de usarla.
  // Motivo: la caché va por ruta y el tamaño de un fichero regenerado puede
  // no cambiar (p.ej. sfx.SDT: 198820 B igual en vanilla y regenerado), así
  // que la copia vieja ganaba siempre y el motor seguía leyendo datos viejos
  // (tamaños/frecuencias de audio). Subir esto en cada rebuild de datos.
  // ve11 (21/09, sección 1): banco de audio con las 13 muestras del mod
  // (`Audio/sfx.SDT` ampliado a 9954 entradas + `Audio/sfx/9941..9953.mp3`).
  // ve12 (21/09, sección 1): `data/default.ide` con las frecuencias del mod
  // (10/7) tras el refuerzo temporal a 100 (`boost_veh_freq.py --revert`).
  // ve13 (21/09, sección 1): las 13 muestras del mod re-codificadas con su
  // frecuencia REAL (el banco del mod declara 36000/33000/22000 Hz, que no son
  // válidas para MP3: ffmpeg las resamplea y la SDT declaraba otra cosa ⇒ el
  // buffer sonaba hasta un 12 % rápido). Ver `tools/viceex-sfx-check.py`.
  // ve14 (26/09, plan agachado-calibrado): `anim/ped.ifp` con los dos clips del
  // sa-crouch para el agachado andando (`GunCrouchFwd`/`GunCrouchBwd`, 272 -> 274
  // animaciones, insertadas al final por `tools/ifp_add.py`). El motor R27 los
  // pide por nombre: sin esta purga el navegador seguiría con el ifp viejo y el
  // agachado se quedaría sin clip de movimiento.
  // ve1 (27/09, plan agachado-correcciones-axis E1/E2): la RAÍZ de
  // `GunCrouchFwd/Bwd` reescalada in situ con `tools/ifp_scale_root.py`
  // (±2,740 -> ±1,500 m/ciclo = 2,05 m/s: pies pegados a 1,13 m/s sin el x1,7
  // de R26c). Sin esta purga el navegador seguiría con el ifp viejo y las
  // pisadas irían a otro ritmo que el cuerpo.
  // ve15 (28/10, plan 23.0/T32): GXT de los menus del mod (Other Settings y
  // Camara en primera persona) guardados tambien en gamefiles/TEXT para que
  // un rebuild de datos no los borre; el wasm cambia camara, empujon,
  // particulas y disparo a ocupantes. Purga la cache IDB anterior.
  // ve16 (29/10, iteracion 24.3): GXT de los nombres que pedía la IDE y no
  // estaban en la GXT (`PREMIER` del Premier 6504, `AEROPL` del airtrain y
  // `RCGOBLI` del RC Goblin). Purga la cache IDB anterior.
  // ve17 (29/10, ajustes post-26): `FET_AGC`, `LG_39` y `LG_40` en las GXT
  // servidas (las pedia el menu de mando y la leyenda del mapa). Purga la
  // cache IDB anterior.
  // T53 (30/10): las 1708 geometrías del mapa/peds que declaraban más índices
  // de los que su plugin contiene (aceras, docks, terminal, edificios) quedan
  // recortadas. Purga la caché IDB para que el motor lea los DFF nuevos.
  // T65 (30/10, plan 32.0): las versiones de STRUCT de 34 geometrías de 8
  // vehículos no cuadraban con su cuerpo (12 bytes de surface props) y el motor
  // no encontraba el MATLIST: el modelo fallaba en bucle y acababa en
  // "memory access out of bounds". Se repara el campo de versión y se
  // reempaqueta el .dir (faggio y kaufman encogieron). Purga la caché IDB.
  // T66 (30/10, plan 34.0): las 30 geometrias de intermitente que escribio el
  // parcheador guardaban todas las cabeceras de malla primero y luego todos los
  // indices. librw las lee intercaladas, asi que tomaba indices por materiales
  // (`MESHBADMAT`), el modelo fallaba y el clump a medias reventaba en
  // `Frame::updateObjects`. Se reordena la malla (mismo tamano) y se corrige el
  // generador. Purga la cache IDB.
  // T103 (3/11, plan 42.1): 66 DFF de vehículo reconstruidos desde la base
  // limpia con la regla de hue (460 celdas: 17 nodos borrados, 4 restaurados del
  // mod, 58 ámbar real y 88 propios en su color) y `models/gta3.dir`
  // reempaquetado: 13 entradas crecen y 25 encogen (banshee 189->93 sectores al
  // quitarle los nodos inventados de octubre). Purga la cache IDB: sin eso el
  // navegador seguiria leyendo los DFF viejos por ruta.
  // T109 (3/11, plan 42.1 paso 4 re-medido): dos evidencias nuevas deciden el
  // ambar cara a cara (rasterizado de la UV de la cara y parche cerrado en el
  // atlas), asi que 10 celdas pasan de AMBAR a PROPIO (boxville y mrwhoop
  // delante, washing/linerun/topfun detras) y 18 DFF se rehacen desde
  // `tmp/base421`. `rumpo`/`pony` recuperan su unica lente ambar real que el
  // paso anterior habia borrado, y `lovefist`/`stretch`/`manana`/`cuban` quedan
  // recortados a las caras que sostienen el parche. `gta3.dir`: 2 entradas
  // crecen (topfun 82->83, zebra 97->98) y 3 encogen (hotrina, pony, romero);
  // sello del manifiesto ded0fa884ecbcc337cc6f370311aabc7.
  // T110 (3/11, plan 42.1): NO sube el tag. Todo lo del reporte es motor: la
  // tabla de lentes se compila dentro del wasm (`src/vehicles/IndicatorLensTable.h`),
  // no se sirve, y ningun DFF/txd/img cambio. Subirlo obligaria a re-bajar todo
  // el juego por un cambio de codigo (ADR-027).
  // 45.0 T6 (5/11): SI sube. El split de la lente (T4) reescribio 27 DFF del
  // gta3.img servido, 18 de ellos con tamano nuevo, y `repack_dir.py` rehizo
  // `models/gta3.dir` (streamed y bootseed) con los sectores recalculados. Sin
  // este tag el navegador seguia usando las entradas viejas de IndexedDB y el
  // motor leia modelos truncados o desalineados.
  // 47.0 T1 (5/11): solo cambia `models/gta3.dir`, y cambia de contenido sin
  // cambiar de tamano: los 195.072 bytes son las mismas 6096 entradas con un
  // layout contiguo autoritativo (antes 367 huecos y 368 solapes entre las dos
  // cajas). El sello `__vc__` del manifiesto es el que purga la cache IDB; el
  // tag sube para que el `data=` de la traza se pueda leer por plan.
  // 48.0 T1 (6/11): el plan no cambia ningun dato servido; el tag sube porque lo
  // que cambia es como el motor cuenta el tiempo (claxon a 30 Hz logicos, rampa
  // de alpha escalada, reloj que recupera hitches, empuje ped<->coche atenuado y
  // asiento compensado por altura). Se sube tambien para que
  // `check-served-build.sh` pueda comprobar que el paquete lleva el plan dentro.
  // 49.0 T1 (6/11): sigue sin cambiar ningun dato servido. Sube el tag porque
  // el censo de logica por frame movio doce contadores al reloj logico (mando,
  // giro en el aire, streaming, suspension, camara, nado, sangre, HUD, mareo,
  // peces, audio y las gotas de lente del salpicado), dejo uniforme el asiento
  // de los NPCs y anadio la cota de texturas por TXD con las trazas del Hunter.
  // 46.0 T6 (6/11): SI sube. `gen_lens_atlas.py` inyecta en 17 TXDs del
  // gta3.img servido un atlas derivado `<atlas>_lens` (copia del atlas del coche
  // con las 925 texelas sin matiz del footprint de las lentes re-tingidas al
  // matiz templado medido en ese mismo footprint, conservando la luminancia); el
  // motor lo monta solo en el flanco encendido del parpadeo. Los 17 crecen de
  // tamano (+136 KB DXT1) y `repack_dir.py` rehizo `models/gta3.dir` (streamed y
  // bootseed, mirror byte a byte) con los sectores recalculados. Purga la cache
  // IDB: sin esto el navegador seguiria abriendo los TXDs viejos, sin lente.
  // 51.0 T6 (7/11): SI sube. Los seis `TEXT/*.gxt` llevan las etiquetas nuevas del
  // menu de controles (FEC_LTS/FEC_RTS/FEC_EML dicen intermitente izquierdo /
  // intermitente derecho / luces de parqueo en los 6 idiomas; cada fichero crece
  // ~118 bytes, `stage_bootseed.py` los volco en el `.data` antes de enlazar y
  // TABL sigue OK con sus 78 misiones). El motor ademas achica el disco del
  // destello, pinta el ambar medido de la captura del mod, restringe la IA de
  // cruce del trafico y conserva el intermitente latcheado al bajarse. Sucesor en
  // NOMBRE de `2026-11-07-43.1-T1`: el contenido del 43.1 sigue en el arbol y en
  // esta build. Purga la cache IDB: sin eso el navegador abriria el GXT viejo.
  // 55.0 (8/11): REVERTIDO por el jugador («los cambios que hiciste son
  // invenciones, te pido revertirlos»). La relajacion de las tres compuertas de
  // `indicator_lens45.py` hizo pintar al benson las marcas laterales del
  // parachoques en vez de la lente que esta debajo del faro, y el parche se quito
  // (`benson.dff` restaurado byte a byte desde el respaldo del 3/11, tabla de
  // lentes de vuelta a 262 filas). Vuelve como plan 56.0 con la deteccion
  // 56.0 (8/11) T3b: el DATO vuelve a cambiar. La deteccion de lentes tenia dos
  // ciegos de muestreo (UV recortadas en V y el byte alfa basura de las texturas
  // con hasAlpha=0) ya corregidos en la herramienta; con la medida buena se
  // regenero `IndicatorLensTable.h` (262 -> 320 filas, 190 -> 204 nodos con lente
  // ambar, 0 nodos que pierdan su ambar) y se rehiciéron los nodos de los modelos
  // aprobados por el jugador: boxville, mule, trash y pizzaboy con sus cuatro
  // esquinas, fbiranch, esperant y fbicar delante. Esperant sube a 86 sectores y
  // fbicar baja a 93, asi que `gta3.dir` esta reempaquetado y las tres cajas
  // sincronizadas -tambien el `trash.dff` de `bootseed/`, que se habia quedado
  // con la version del 55.0 revertido mientras `streamed/` tenia la original-.
  // Sucesor en NOMBRE de `2026-11-08-53.0-T1`: su contenido (los GXT de opciones
  // 56.0 T8 (8/11): REVERTIDA la tanda de direccionales. El jugador lo dice con
  // sus dos capturas del mule y del fbiranch: «te pedi unicamente que activaras
  // los focos y las luces intermitentes de cada uno de esos, no inventar unos
  // nuevos ni cambiar sus formas o tamanos». Se devolvieron los nueve DFF
  // (fbiranch, boxville, mule, trash, esperant, fbicar, pizzaboy, idaho, stinger)
  // a su estado previo en las tres cajas y la tabla de lentes a las 262 filas
  // servidas, anadiendo UNICAMENTE las dos filas del benson, que es el unico que
  // el jugador confirma funcionando. Prueba dura de que el arbol de datos volvio
  // al punto aprobado: el sello del manifiesto es de nuevo
  // `60a0c0a3f340f41b170e4b6b21982b0d`, el mismo que dejo el revert del 55.0.
  // Sucesor en NOMBRE de `2026-11-08-56.0-T7` y `2026-11-08-56.0-T3` (las dos
  // tandas revertidas, cuyo contenido ya no esta en el arbol) y de
  // `2026-11-08-53.0-T1` (los GXT de opciones extra, intactos). Purga IDB.
  // 58.0 T3 (8/11): los TXD de esperant/boxville/fbiranch/mule/trash reciben su
  // atlas derivado <atlas>_lens (lentes ambar de carroceria, plan 58.0 T3/T4).
  // Cambian los datos servidos -> purga IDB. Sucesor en NOMBRE de
  // `2026-11-08-56.0-T8` (la tanda revertida, cuyo contenido sigue fuera del
  // arbol); ese rotulo se conserva aqui para que su marca de check-served-build
  // siga encontrandolo en el paquete.
  // 58.0 T4 (8/11): el material de lente de carroceria se parte en slot
  // izquierdo y derecho (split_lens_material.py) en esperant/boxville/mule/
  // trash (chassis_hi) y fbiranch (chassis_hi detras, bump_front_hi_ok
  // delante): cero cambio de vertices/posiciones, solo indice de material por
  // triangulo y dos copias identicas del material. BodyLensTable.h regenerada:
  // cada fila pinta SOLO el slot de su costado (giro a la izquierda ya no
  // enciende el cristal derecho). Los TXD no cambian; cambian los DFF y el
  // gta3.dir -> purga IDB. Sucesor en NOMBRE de `2026-11-08-58.0-T3` (sus atlas
  // derivados siguen servidos intactos); ese rotulo se conserva aqui para que
  // su marca de check-served-build siga encontrandolo en el paquete.
  // 58.1 T1 (9/11): cambian los TXD de topfun/rumpo/pony (atlas derivado de cara
  // completa <tex>_lens: la direccional delantera enciende entera, no solo el
  // hilo), mrwhoop (atlas derivado de carroceria sobre el faro) y trash (se
  // retira el atlas inventado del porton); zebra.dff re-parenta el clon de
  // pintura fuera del nodo indicator y mrwhoop.dff parte el material de la lente
  // por costado -> purga IDB. Sucesor en NOMBRE de `2026-11-08-58.0-T4` (sus
  // atlas de esperant/boxville/fbiranch/mule siguen servidos intactos); ese
  // rotulo se conserva aqui para que su marca de check-served-build siga
  // encontrandolo en el paquete.
  // 59.0: cambia trash.txd (atlas `trash868bit128_lens` re-escrito a ARGB8888
  // con la cara entera de las direccionales traseras) y con el gta3.dir
  // reempaquetado (los TXD que el plan 59.0 heredo reescritos crecieron de
  // sectores: boxville, cabbie, enforcer, fbiranch, manana, mrwhoop...). El
  // que purga de verdad es el sello de contenido del manifiesto (ADR-027);
  // este rotulo solo se reporta. dataTag anterior: `2026-11-09-58.1-T1`.
  // 61.0 T2: TXD del banshee convertido a 32 bits; el sello del manifiesto
  // tambien cambia, que es lo que purga la cache IDB (ADR-027).
  // 61.0 T3: el cuerpo del banshee lleva marcador de pintura en su material
  // dominante (chassis_hi); el dataTag sube para purgar la IDB con el DFF nuevo.
  // 61.0 T4: el cuerpo usa la textura banshee86body128a; el dataTag sube para
  // purgar la IDB con el DFF nuevo (386685 bytes).
  // 62.0 T1: DFF de mrwhoop, rumpo, pony, topfun, boxville, esperant, fbiranch y
  // fbicar con el cristal en slot propio; cambian tambien las dos tablas de lentes.
  // dataTag anterior: `2026-11-09-62.0-T1`.
  // 61.0 T5: el chassis_hi reparte el atlas banshee868bit128 (1946 tris);
  // el dataTag sube para purgar la IDB con el DFF nuevo (386693 bytes).
  // dataTag anterior: `2026-11-09-62.0-T2`.
  // 61.0 T6: el chassis_hi pasa al atlas completo (3956 tris); la textura de
  // puertas `banshee86body128a` sale del chasis; DFF 386685 bytes.
  // 62.1 T1: fbicar revertido y el nodo solo se enciende con fila ambar medida;
  // el faro blanco del mrwhoop y los pilotos de rumpo/pony/topfun dejan de
  // recibir ambient. dataTag anterior: `2026-11-09-62.0-T3`.
  // 62.2 T1: T7 alinea las UVs del atlas del Banshee contra el PC y reconstruye
  // los grupos de malla desde el STRUCT. dataTag anterior: `2026-11-09-62.1-T1`.
  // 62.3 T1: T8 copia las UVs REALES de los triángulos PC (cuerpo y atlas) al
  // chassis_hi Xbox; 0 degenerados. dataTag anterior: `2026-11-09-62.2-T1`.
  // 63.0 T1 (10/11): T9 instala el Banshee COMPLETO de The Hero 1:1: DFF
  // 342016 B, 33 frames/16 atomics y chassis_hi 3956 tris con los 9 materiales
  // Xbox repartidos; TXD 1552204 B, 8 texturas 32 bits. Sustituye al T8 y purga
  // la IDB (el DFF/TXD y el .dir cambian de tamaño/sectores).
  // 62.2 T2: cristal medido por matiz+saturacion (pony/topfun/esperant ganan
  // las caras que el 20 % dejaba fuera) y fbicar declarado con las mismas
  // caras y sitios del washing (6 delante 407,8 cm2, 7 detras 228,0 cm2).
  // Etiqueta: la lane del banshee uso 62.2-T1, 62.3-T1 y `2026-11-10-63.0-T1`
  // para su 61.0; se conservan aqui para que el paquete siga dando su marca.
  // 62.3 T2: huella del derivado = las caras que DECLARA la fila (no solo las
  // del nodo): rumpo/pony/topfun/esperant pasan del 22-35 % al 100 % de
  // cobertura por cara, 527 texelas, fuera=0. Se conserva el literal
  // `2026-11-10-63.0-T1` y `2026-11-10-63.1-T1` de la lane del banshee, y la
  // propia `2026-11-09-62.2-T2`, que el checker exige ver en el paquete.
  // 63.2 T1 (10/11): Banshee Xbox con la capa de reflejo del Banshee PC
  // (bansheespeca + coeficiente 1.0 en pintura y cristales, DFF 344176 B,
  // TXD 1574200 B); purga la IDB con los datos nuevos.
  // dataTag anterior: `2026-11-09-62.3-T2`.
  // 60.0 §13 (14/11): Stinger faro rojo pintado ambar en el atlas + traseras al
  // fallback rojo; firetruk con las piezas extradetails/servicelights_0 del PC
  // (canon visible otra vez, DFF 379475 B, TXD 2264412 B); y la tanda 2 del
  // plan 60 (vicechee, admiral, cabbie, landstal, hotrinb, police, ambulan)
  // con la capa de reflejo y sus lentes ambar. Sube por la IDB: cambian los
  // DFF/TXD y el .dir (están en streamed+gamefiles y en bootseed horneado).
  // Se conserva el literal `2026-11-13-66.2-T1` (purga de la IDB del 66.2).
  // dataTag anterior: `2026-11-14-66.3-T1`.
  // 60.1: lentes, barras, luces de servicio y cañon nuevos en 8 modelos.
  // 60.3 T1: revision del jugador. Stinger sin indicadores delanteros y lente
  // pintada en ambar; police con una sola barra (policebar_hi) y placa con el
  // RGB del PC; vicechee con la farola fija (vicebar1/2/3_hi); panes window_*
  // del PC en vicechee/police/admiral/ambulan y vidrio fundido fuera de las
  // puertas; firetruk sin el watercannon inventado.
  // 60.3 T2: los DFF de vicechee/police/admiral/ambulan salieron con el STRUCT
  // de geometria 12 bytes corto (strip_glass) y el police no cargaba (`buf
  // falla police`): regenerados y validados chunk a chunk. dataTag anterior:
  // `2026-11-14-60.1-T1`.
  // 60.5 T1: los 11 vuelven a carroceria PC + piezas Xbox (interior asientos/
  // salpicadero/paneles, motor, bajos, escape); sin lentes ni matriculas Xbox;
  // volante propio (nunca el negro). dataTag anterior: `2026-11-15-60.4-T4b`.
  // 60.7 T1: la patrulla con una sola barra de sirena (fuera la V duplicada) y
  // ventanilla de puerta con su estado danado antes de romperse. dataTag
  // anterior: `2026-11-15-60.6-R2`.
  // 60.8 T1: el Banshee vuelve a base PC (el restore era el DFF Xbox completo y
  // el TXD faltaba) y estrena interior/motor/bajos/vano Xbox, con V12 y
  // belowcars anadidos al atlas PC y paneles _dam deformados. dataTag anterior:
  // `2026-11-16-60.9-T6`.
  // 60.8 T14: los 9 restantes estrenan interior/motor/bajos Xbox y paneles de
  // puerta; los TXD base de 8 bits se convierten a 32 bits. dataTag anterior:
  // `2026-11-16-60.9-T13`.
  // 60.10 T1: correccion del lote de los 9 (metodo admiral): exterior PC intacto
  // (morro/cola/luces/parachoques), od_interior solo cabina sin solapes, vidrios
  // y paneles _dam rehechos. dataTag anterior: `2026-11-17-60.9-T17`.
  // 60.10 T2: vidrios al pase transparente, od_interior antes de las grietas,
  // sirena vicechee y piel de puerta landstal conservadas. dataTag anterior:
  // `2026-11-16-60.10-T1`.
  // 60.10 T3: correcciones de pilotos sin plan: vidrios con tinte uniforme,
  // sirena vicechee por componentes completos sin cortes, y un solo volante movil
  // (fijo Xbox retirado / anclaje del procedural <=100). dataTag anterior:
  // `2026-11-16-60.10-T2`.
  // 60.10 T4: vicechee farola de sirena permanente (od_siren) y pareja de
  // retrovisores permanente (od_mirrors); extra1/extra2 vaciados para que el
  // sorteo aleatorio de componentes no deje el coche con 1 retrovisor o sin
  // farola. dataTag anterior: `2026-11-17-60.10-T3`.
  // 60.10 T6: chorro del firetruk anclado a la boca real del dispersor (nodo
  // bump_rear pivotante, y1.80/z2.00) y REVERTIDO el trim de molduras de T5
  // (refutado: era el respaldo interior de la ventanilla que admiral conserva;
  // quitarlo flotaba el material de la otra puerta sobre el vidrio roto).
  // 60.10 T7: la mancha sobre la luna rota era el chassis PC (cartas planas
  // opacas tapando el hueco: respaldo negro None + body inboard), no el
  // od_interior. Se retiran por componente en coches con luna lateral; admiral
  // no las tiene. vicechee: fuera el cabezal de retrovisor duplicado del chassis
  // (quedaban 3 en algunos angulos) => exactamente 2 (od_mirrors).
  // dataTag anterior: `2026-11-17-60.10-T6`.
  // 60.10 T8: la retirada de cartas del chassis de T7 leia el buffer original y
  // deshacia la retirada de cabina (cuerpo PC entero otra vez); ahora va en la
  // misma pasada sobre el clump clonado. dataTag anterior:
  // `2026-11-17-60.10-T7`.
  // 60.10 T13: vicechee, vuelve el marco del parabrisas (el filtro de lunas lo
  // tomaba por respaldo) y fuera la pala vertical del pilar A que se veia como
  // tercer retrovisor encima del del conductor. dataTag anterior:
  // `2026-11-17-60.10-T12`.
  // 61.0 T1: la textura paletizada del rines (wheel_lightvan64back) pasa a 32
  // bits sin comprimir. dataTag anterior: `2026-11-18-61.0`.
  // 89.0: los 74 DFF de la flota recuperan la chapa PC que el 61 recorto
  // (2.827 caras), los 8 marcos de faro de 7 coches (61 marcos), y las lentes y
  // forros coplanares quedan a 5 mm (3.287 parejas -> 167). dataTag anterior:
  // `2026-11-22-88.0-T1`.
  // 90.0: el forro `od_interior` que asomaba por fuera de la carroceria se mete
  // detras de la chapa (pheonix y taxi primero, luego la flota), y el taxi
  // recupera las 4 caras del faro trasero que tapaba. dataTag anterior:
  // `2026-11-22-89.0-T1`.
  // 92.0: las caras de faro y piloto de 51 coches recuperan la UV del DFF del
  // PC (la servida tenia la V espejada y caia en el negro del atlas). dataTag
  // anterior: `2026-11-22-91.0-T1`.
  dataTag: '2026-11-22-92.0-T2',
  manifest: null,
  db: null,
  inflight: {},
  pending: {},
  misses: [],
  // Consola con detalle (sin query param: siempre verboso).
  verbose: true,
  // Rutas configurables: la librería (web/lib/index.js) deja window.__VC_CFG
  // ANTES de cargar reVC.js, así el mismo binario sirve para cualquier host
  // (/streamed, CDN, subcarpeta...). Sin config, los valores de siempre.
  cfg: (function () {
    const d = { streamedUrl: '/streamed/', manifestUrl: '/manifest.json', odtraceUrl: '/odtrace', idbCapMB: 900, warmMB: 128, worker: true };
    try {
      const c = (typeof globalThis !== 'undefined' && globalThis.__VC_CFG) || {};
      if (c.streamedUrl) d.streamedUrl = c.streamedUrl;
      if (c.manifestUrl) d.manifestUrl = c.manifestUrl;
      if (c.odtraceUrl !== undefined) d.odtraceUrl = c.odtraceUrl;
      if (c.idbCapMB !== undefined) d.idbCapMB = Number(c.idbCapMB) || 0;
      if (c.warmMB !== undefined) d.warmMB = Number(c.warmMB) || 0;
      if (c.worker !== undefined) d.worker = !!c.worker;
    } catch (e) {}
    for (const k of ['streamedUrl']) if (d[k] && d[k].slice(-1) !== '/') d[k] += '/';
    return d;
  })(),
  sleep(ms) { return new Promise((r) => setTimeout(r, ms)); },
  // 82.2: hueco libre del navegador. La precarga en caliente lo usa para no
  // robar milisegundos a los frames (requestIdleCallback con tope de espera).
  idle(ms) {
    return new Promise((r) => {
      try {
        if (typeof requestIdleCallback === 'function') {
          requestIdleCallback(() => r(), { timeout: ms || 60 });
          return;
        }
      } catch (e) {}
      setTimeout(r, 0);
    });
  },
  // Flush IDBFS serializado (F2): solapar syncfs atasca la cola IDB
  // ("N operations in flight") y nada completa. Si hay uno en vuelo, encolar.
  // Con origen (save/settings/quit) deja línea en consola; el resto va
  // silencioso. Los saves son acciones raras del usuario: 2 líneas no es spam.
  _syncFsBusy: false,
  _syncFsPending: false,
  _ufTimer: null,
  syncUserfiles(src) {
    if (OD._syncFsBusy) { OD._syncFsPending = true; if (src) odLog('[od] flush ' + src + ': en cola'); return; }
    OD._syncFsBusy = true;
    if (src) odLog('[od] flush ' + src + '...');
    try { FS.syncfs(false, () => { OD._syncFsBusy = false; if (src) odLog('[od] flush ' + src + ' ok'); if (OD._syncFsPending) { OD._syncFsPending = false; OD.syncUserfiles('pendiente'); } }); }
    catch (e) { OD._syncFsBusy = false; if (src) odLog('[od] flush ' + src + ' ERROR: ' + e); }
  },
  // Estilo dos.zone: toda escritura bajo /userfiles programa un flush con
  // antirrebote (sin ventana de 30 s y sin solapes por construcción).
  noteUserfilesWrite(path) {
    if (!path || path.indexOf('/userfiles') !== 0) return;
    if (OD._ufTimer) clearTimeout(OD._ufTimer);
    OD._ufTimer = setTimeout(() => { OD._ufTimer = null; OD.syncUserfiles(); }, 2500);
  },
  // Caché MEMFS con desalojo: solo lo traído por OD es desalojable (el
  // paquete inicial nunca). Lo desalojado se re-descarga (IDB lo sirve sin
  // red). Sin esto la sesión crece sin cota y la pestaña muere (~2.4 GB).
  live: new Map(),
  liveBytes: 0,
  // Tope de descargas SIMULTANEAS (21/10). Al cargar el mundo el motor
  // pide mas de 1400 ficheros de golpe y cada uno era un fetch en
  // paralelo: el heap de JS se iba a 900 MB, el GC tardaba 11 s y el
  // navegador apagaba el WebGL (pantalla negra con el sonido de fondo).
  // Con el tope, las descargas van a rafagas de 48 y el pico baja mucho.
  dlMax: 48,
  dlActive: 0,
  dlQueue: [],
  dlSlot() {
    if (OD.dlActive < OD.dlMax) { OD.dlActive++; return Promise.resolve(); }
    return new Promise((res) => OD.dlQueue.push(res));
  },
  dlRelease() {
    if (OD.dlActive > 0) OD.dlActive--;
    const next = OD.dlQueue.shift();
    if (next) { OD.dlActive++; next(); }
  },
  // 24/10 (dieta por los cuelgues): los topes anteriores (CAP 360, blando 180,
  // HEAP_HI 700) son los que dejaron morir la pestaña tres veces. Medido en las
  // sesiones de 2026-10-24 con la build 17.0-T9 (tag ODCAP de la traza): MEMFS
  // 436 MB con heap JS 551 MB, y MEMFS 517 MB con heap JS 624 MB, siempre con
  // el heap wasm clavado en 512 MB y SIN desalojar nunca (`evict=0/0MB`). La
  // traza se corta en mitad de un frame a 60 fps sin JSERR, sin ENGERR y sin
  // pérdida de WebGL: eso es el proceso del renderer muriendo por memoria, no
  // una excepción del motor. El paquete inicial (~164 MB) no es desalojable, así
  // que el techo real de MEMFS es 164 + CAP.
  //
  // 25/10 (tirones): con CAP 176 y gracia 20 s el working set se desalojaba
  // mientras se conducía (2.877 ficheros en 2 min) y cada re-petición suspende el
  // tick por una descarga on-demand: `strm` llegó a 2.159 ms en un solo frame y
  // `JS ASYNC` a 443 ms en un lote. El heap wasm estaba pidiendo 512 MB y usando
  // 64, así que ahora arranca en 384 MB (build.sh, crece si hace falta) y esos
  // 128 MB se le dan al caché: CAP 240 y gracia 45 s. El techo de MEMFS sube a
  // ~404 MB y el total del renderer se queda donde estaba.
  // 26/10 (thrash medido): el heap de V8 en partida vive en 420-440 MB de forma
  // normal (traza, buffers de descarga, el mapa de ficheros). Con HEAP_HI=420 la
  // vigilancia disparaba cada 8 s: el log lo enseña literal — `ODMEM heap=430MB
  // desalojo live=166->96MB` y dos veces más — tirando a la basura modelos que
  // el motor estaba usando, que volvian a pedirse al frame siguiente. Eso es el
  // "carga de a poco" al salir de un interior y los tirones: un bucle
  // desalojo -> re-peticion -> descarga. El limite que si mataba la pestaña eran
  // 551-624 MB de heap CON 436-517 MB de MEMFS; ahora MEMFS vive en ~300 MB, asi
  // que el aviso sube a 520 MB, exige tres medidas seguidas (24 s) y desaloja
  // solo hasta el 70% del tope en vez de dejarlo en 96 MB.
  // 19/11 (escalón +16 MB, sesión 77.0 de las 19:33): el pool tocó techo en
  // partida (`live=238MB` de 240) y ahí empezaban los desalojos por gracia de
  // ficheros que el motor estaba usando. Un escalón pequeño y reversible: el
  // techo de MEMFS pasa de ~404 a ~420 MB. Si el móvil mata la pestaña, volver
  // a 240 antes que tocar HEAP_HI.
  // 21/11 (thrash medido en movil, sesion 21:29-21:32): 5.889 desalojos en dos
  // minutos (~49/s) con `liveBytes` clavado en el tope de 192 MB: cada fichero
  // nuevo expulsaba al mas viejo y el motor lo volvia a pedir, de ahi las
  // recargas de atomos y el tercio de frames fuera de 16,7 ms. Se sube el techo
  // a 240 (MEMFS ~404 MB, el valor que ya se uso antes del 19/11) y la gracia a
  // 90 s para que solo salga lo que no se toca en minuto y medio. Si un movil
  // justo mata la pestaña, volver CAP a 192 y dejar la gracia en 90.
  CAP: 240 * 1048576,
  GRACE_MS: 90000,
  HEAP_HI: 520 * 1048576,
  CAP_SOFT: 96 * 1048576,
  heapOver: 0,
  evicted: 0,
  evictedBytes: 0,
  fetchedBytes: 0,
  fetchedCount: 0,
  idbHits: 0,
  touch(path, size) {
    const e = OD.live.get(path);
    if (e) { e.t = Date.now(); return; }
    OD.live.set(path, { s: size, t: Date.now() });
    OD.liveBytes += size;
    OD.evict();
  },
  evict(force, target) {
    const cap = typeof target === 'number' ? target : (force ? OD.CAP_SOFT : OD.CAP);
    if (OD.liveBytes <= cap) return;
    const now = Date.now();
    // Doble del tope: desalojo duro (sin periodo de gracia). MEMFS mantiene
    // vivos los streams abiertos aunque se deslinkee el nombre; reabrir
    // re-descarga (IDB lo sirve sin red). La alternativa es morir por OOM.
    // La valvula de emergencia no puede estar a 2*CAP (480 MB): ahi la pestaña
    // ya muere en movil antes de que salte. Se dispara a CAP+96 (336 MB).
    const hard = force || OD.liveBytes > OD.CAP + 96 * 1048576;
    const order = [...OD.live.entries()].sort((a, b) => a[1].t - b[1].t);
    for (const [p, e] of order) {
      if (OD.liveBytes <= cap) break;
      if (!hard && now - e.t < OD.GRACE_MS) continue;
      try { FS.unlink(p); } catch (err) {
        OD.live.delete(p);
        OD.liveBytes -= e.s;
        continue;
      }
      OD.live.delete(p);
      OD.liveBytes -= e.s;
      OD.evicted++;
      OD.evictedBytes += e.s;
    }
    if (OD.verbose && OD.evicted % 50 === 1) odLog('[od] desalojo total=' + OD.evicted + ' live=' + Math.round(OD.liveBytes / 1048576) + 'MB');
  },
  // Dirs que el motor espera que existan (statvfs, scans). Se crean vacíos.
  // OJO: jamás pre-crear un dir que colisione con un ARCHIVO necesario
  // (anim/cuts.img lo hizo: fcaseopen encontraba el dir vacío en vez del
  // .img y la cinemática salía negra). Solo models/gta3.img (backend suelto).
  dirs: [
    '/anim', '/Audio', '/data', '/models', '/models/gta3.img', '/TEXT',
    '/txd', '/neo', '/skins', '/userfiles',
  ],

  norm(p) {
    return String(p).replace(/\\/g, '/').replace(/^\.\//, '').replace(/^\/+/, '').toLowerCase();
  },

  // Censo MEMFS (ficheros y MB) con caché de 30 s para el heartbeat.
  _census: { t: 0, n: 0, mb: 0 },
  census() {
    const now = Date.now();
    if (now - OD._census.t < 30000) return OD._census;
    let n = 0, b = 0;
    const per = {};
    try {
      const stack = ['/anim', '/Audio', '/data', '/models', '/TEXT', '/txd', '/neo', '/skins', '/userfiles'];
      while (stack.length) {
        const dir = stack.pop();
        let ents;
        try { ents = FS.readdir(dir); } catch (e) { continue; }
        for (const e of ents) {
          if (e === '.' || e === '..') continue;
          const p = dir + '/' + e;
          try {
            const st = FS.stat(p);
            if (FS.isDir(st.mode)) stack.push(p);
            else {
              n++; b += st.size;
              const top = '/' + p.slice(1).split('/')[0];
              per[top] = (per[top] || 0) + st.size;
            }
          } catch (ee) {}
        }
      }
    } catch (e) {}
    const big = [];
    try {
      const stack2 = ['/anim', '/Audio', '/data', '/models', '/TEXT', '/txd', '/neo', '/skins'];
      while (stack2.length) {
        const dir = stack2.pop();
        let ents;
        try { ents = FS.readdir(dir); } catch (e) { continue; }
        for (const e of ents) {
          if (e === '.' || e === '..') continue;
          const p = dir + '/' + e;
          try {
            const st = FS.stat(p);
            if (FS.isDir(st.mode)) stack2.push(p);
            else if (st.size > 20 * 1048576) {
              // Prueba de integridad: ¿se puede abrir y leer de verdad?
              // Si el tamaño miente (metadatos corruptos), el read falla.
              let probe = 'n/a';
              try {
                const s = FS.open(p, 'r');
                const buf = new Uint8Array(16);
                const n = FS.read(s, buf, 0, 16, 0);
                FS.close(s);
                probe = 'read=' + n + ':' + Array.from(buf.slice(0, Math.min(n, 8)))
                  .map((b) => b.toString(16).padStart(2, '0')).join('');
              } catch (ee) { probe = 'ERROR:' + (ee && ee.message || ee); }
              big.push(p + ':' + Math.round(st.size / 1048576) + 'MB[' + probe + ']');
            }
          } catch (ee) {}
        }
      }
    } catch (e) {}
    OD._census = { t: now, n, mb: Math.round(b / 1048576) };
    OD._census.bigN = big.length;
    OD._census.big = big.slice(0, 25).join(' ');
    OD._census.live = Math.round(OD.liveBytes / 1048576);
    OD._census.ev = OD.evicted;
    OD._census.evmb = Math.round(OD.evictedBytes / 1048576);
    OD._census.fmb = Math.round(OD.fetchedBytes / 1048576);
    OD._census.idb = OD.idbHits + ((OD.wkStats && OD.wkStats.idb) || 0);
    OD._census.opfs = (OD.wkStats && OD.wkStats.opfs) || 0;
    OD._census.per = Object.entries(per).map(([k, v]) => k + ':' + Math.round(v / 1048576)).join(' ');
    OD._census.ufs = Math.round((per['/userfiles'] || 0) / 1048576);
    OD._census.root = Math.round((per['/'] || 0) / 1048576);
    return OD._census;
  },

  // Emisoras del dial (orden retail, para "la siguiente").
  radios: ['WILD', 'FLASH', 'KCHAT', 'FEVER', 'VROCK', 'VCPR', 'ESPANT',
           'EMOTION', 'WAVE'],

  // F5c (fluides-v2): NADA de prefetch masivo (las 9 = 270 MB que ahogan el
  // cap y provocan el thrash medido en lean1: 1683 ficheros re-traídos).
  // Sólo vive en MEMFS la emisora que suena; al sonar una, se precarga LA
  // SIGUIENTE del dial en background (cambio de emisora sin freeze). Al
  // arrancar, la última usada (localStorage) a los 15 s.
  currentStation: null,
  _nextTimer: null,
  noteStation(station) {
    OD.currentStation = station;
    try { localStorage.setItem('vc_lastStation', station); } catch (e) {}
    if (OD._nextTimer) clearTimeout(OD._nextTimer);
    const i = OD.radios.indexOf(station);
    if (i < 0) return;
    const next = OD.radios[(i + 1) % OD.radios.length];
    OD._nextTimer = setTimeout(() => {
      OD._nextTimer = null;
      // R2: la precarga de LA SIGUIENTE emisora no puede caer en una carga
      // bloqueante del motor (ahí el motor no reintenta y el frame se queda
      // parado) ni escribir 30 MB en MEMFS sin que nadie los haya pedido (era
      // una copia en el hilo del juego en un momento arbitrario). Se pide al
      // worker y sus bytes quedan retenidos hasta que el motor pida el fichero.
      let block = 0;
      try { block = window.__vcODBlock | 0; } catch (e) {}
      if (block > 0) { OD.adfTrace(next, 0, 0, 'precarga=0 motivo=block'); return; }
      if (OD.wk) {
        const canon = '/Audio/' + next + '.ADF';
        const h = OD.manifest && OD.manifest[OD.norm(canon)];
        OD.wkAsk(canon, h);
        // Con un "esperador" el worker NO escribe MEMFS: se retiene (wkPut) y se
        // escribe sólo cuando el motor pida el fichero de verdad.
        OD.wkWait(canon).then((buf) => { if (buf) OD.wkPut(canon, buf); }).catch(() => {});
        OD.adfTrace(next, 0, 0, 'precarga=1 retenida=1');
      } else {
        OD._ensure('/Audio/' + next + '.ADF').catch(() => {});
        OD.adfTrace(next, 0, 0, 'precarga=1');
      }
    }, 3000);
  },

  async prefetchLastStation() {
    let last = null;
    try { last = localStorage.getItem('vc_lastStation'); } catch (e) {}
    if (last && OD.radios.includes(last)) {
      // _ensure directo (no dispara noteStation).
      try { await OD._ensure('/Audio/' + last + '.ADF'); } catch (e) {}
    }
  },

  // Una línea al odtrace.log: deja constancia de qué código y qué datos corren
  // en cada sesión (sin esto, una traza sin el tag no se puede interpretar).
  // F4c: NO usar XHR SÍNCRONO. Bloquea el hilo del juego la ida y vuelta
  // completa al servidor, y la traza ASYNC sale una vez por segundo: era el
  // único trabajo que robaba milisegundos a un frame de cada segundo (marco
  // de medición del tirón). Se encola como las trazas de C++ (window.__odq,
  // que main.js manda por lotes en segundo plano). Fallback: XHR síncrono
  // solo si no hay cola (página sin main.js).
  trace(msg) {
    const line = 'JS ' + msg;
    try {
      if (window.__odq) { window.__odq.push(line); return; }
      if (!OD.cfg.odtraceUrl) return;
      const x = new XMLHttpRequest();
      x.open('POST', OD.cfg.odtraceUrl, false);
      x.send(line);
    } catch (e) {}
  },

  async init() {
    odLog('[build] ' + OD.buildTag + ' (si no ves este tag, recarga con Ctrl+Shift+R)');
    OD.trace('build=' + OD.buildTag + ' data=' + OD.dataTag);
    // Puente mínimo recoil (plan recoil-por-arma-y-cadencia): si la librería
    // no definió la sesión (build servido sin lib), el EM_ASM de C++ no debe
    // tirar y las líneas van a __odq con identidad no disponible. La lib lo
    // sustituye cuando existe; sin ?trace=recoil-<id> la sesión queda
    // inactiva y C++ no emite nada.
    try {
      if (!window.__odRecoilSession) window.__odRecoilSession = { active: false, id: 'unavailable' };
      if (!window.__odRecoilEmit) {
        let seq = 0;
        window.__odRecoilEmit = (ev) => {
          try {
            const line = `RECOIL sessionId=unavailable eventSeq=${++seq} build=${OD.buildTag} data=${OD.dataTag} wasm=unavailable ${String(ev).replace(/\s+/g, ' ').slice(0, 640)}`;
            (window.__odq = window.__odq || []).push(line);
          } catch (e) {}
        };
      }
    } catch (e) {}
    try { document.title = '[vc ' + OD.buildTag + '] GTA Vice City'; } catch (e) {}
    for (const d of OD.dirs) {
      try { FS.mkdirTree(d); } catch (e) {}
    }
    try {
      const r = await fetch(OD.cfg.manifestUrl, { cache: 'no-cache' });
      OD.manifest = await r.json();
      OD.byBase = null;
    } catch (e) { OD.manifest = {}; OD.byBase = null; }
    setTimeout(() => OD.prefetchLastStation(), 15000);
    try {
      OD.db = await new Promise((res, rej) => {
        // v2 añade el store 'meta' (marca de uso por fichero) para el techo LRU.
        // Las bases v1 se actualizan solas: 'files' se conserva tal cual.
        const q = indexedDB.open('vcod2', 2);
        q.onupgradeneeded = (e) => {
          const db = e.target.result;
          if (!db.objectStoreNames.contains('files')) db.createObjectStore('files');
          if (!db.objectStoreNames.contains('meta')) db.createObjectStore('meta');
        };
        q.onsuccess = (e) => res(e.target.result);
        q.onerror = (e) => rej(e);
      });
    } catch (e) { OD.db = null; }
    try { await OD.idbPurgeIfStale(); } catch (e) {}
    // El techo de la caché se revisa en segundo plano (no retrasa el arranque)
    // y las marcas de uso se vuelcan en lotes cada 30 s junto con la lista de
    // arranque en caliente. En 'pagehide' se intenta el último vuelco.
    setTimeout(() => { OD.idbTrim('init').catch(() => {}); }, 12000);
    setInterval(() => { OD.idbFlushTouches().catch(() => {}); OD.warmPersist(); }, 30000);
    try { addEventListener('pagehide', () => { OD.warmPersist(); OD.idbFlushTouches(); }); } catch (e) {}
    OD.wkStart();          // Nivel 1: el trabajo de carga, en un hilo aparte
    OD.hookWorldLoad();
    // 83.0: con el manifiesto y el IDB ya listos; main espera (odBoot).
    try { OD.bootPreload(typeof Module !== 'undefined' ? Module : null); } catch (e) {}
    // F4a: una línea ASYNC por segundo con las suspensiones que atendió el
    // pre-js (n y wall-ms). Emparejar con FPSLOG maxdelta del mismo segundo.
    setInterval(() => {
      try {
        if (OD.asyncN > 0) OD.trace('ASYNC n=' + OD.asyncN + ' ms=' + Math.round(OD.asyncMs));
        OD.asyncN = 0; OD.asyncMs = 0;
      } catch (e) {}
    }, 1000);
    // Telemetría de memoria cada 30 s (tag ODCAP): working set real de MEMFS
    // (census con caché 30 s) + heap JS de V8. Para ajustar CAP y la dieta
    // con datos, no a ojo.
    setInterval(() => {
      try {
        OD.migrateMaybe();
        const c = OD.census();
        let js = '';
        if (performance.memory) js = ' js=' + Math.round(performance.memory.usedJSHeapSize / 1048576) + 'MB';
        const per = c.per ? ' desglose=' + c.per : '';
        OD.trace('ODCAP memfs=' + c.mb + 'MB files=' + c.n + ' live=' + c.live + 'MB' +
          ' evict=' + c.ev + '/' + c.evmb + 'MB fmb=' + c.fmb + 'MB idb=' + c.idb +
          ' opfs=' + (c.opfs || 0) + js + per);
      } catch (e) {}
    }, 30000);
    // Vigilancia de memoria: con el heap JS por encima de HEAP_HI el
    // navegador puede matar la pestana. Se desaloja el pool por debajo
    // del tope blando (sin periodo de gracia) para bajar el pico. Solo
    // actua bajo presion; el pool vuelve a crecer solo hasta CAP.
    setInterval(() => {
      try {
        if (!performance.memory) return;
        const js = performance.memory.usedJSHeapSize;
        OD.heapOver = js > OD.HEAP_HI ? (OD.heapOver | 0) + 1 : 0;
        // Tres medidas seguidas (24 s) por encima del aviso: un pico de GC o un
        // lote de descargas no puede vaciar la cache de modelos.
        // 19/11 (medido en la sesión 77.0 de las 19:33): con el heap JS en
        // 562-604 MB este aviso desalojó 6.100 ficheros (live 233->168 MB) y el
        // heap NO bajó ni un MB (604 antes y después). Lo que ocupa esos MB no es
        // el pool de modelos sino la página (traza y buffers), así que el desalojo
        // solo servía para que el motor volviera a pedir 65 MB de golpe al segundo
        // siguiente (`evict=6419/222MB`) con sus paradas de frame. Ahora se avisa
        // y NO se toca el pool mientras esté por debajo de CAP: solo se recorta
        // con presión real (el pool pasado de tope), que es el caso para el que
        // se escribió.
        const target = OD.CAP * 0.7;
        if (OD.heapOver >= 3 && OD.liveBytes > OD.CAP) {
          const before = OD.liveBytes;
          OD.evict(true, target);
          OD.trace('ODMEM heap=' + Math.round(js / 1048576) + 'MB desalojo live=' +
            Math.round(before / 1048576) + '->' + Math.round(OD.liveBytes / 1048576) + 'MB');
          OD.heapOver = 0;
        } else if (OD.heapOver === 3) {
          OD.trace('ODMEM heap=' + Math.round(js / 1048576) + 'MB aviso sin desalojo live=' +
            Math.round(OD.liveBytes / 1048576) + 'MB cap=' + Math.round(OD.CAP / 1048576) + 'MB');
        }
      } catch (e) {}
    }, 8000);
    // Integridad de los ficheros gordos (>20 MB): son los que deciden el peso
    // en memoria y donde una entrada mentirosa se nota (cinemáticas, emisoras,
    // cuts.img). Cada línea ODBIG trae el tamaño y una prueba de lectura real
    // de 1 B: 'read=n:hex' es sano; 'ERROR:...' son metadatos rotos. Así se
    // distingue "no se cargó" de "se cargó y no se puede leer".
    setInterval(() => {
      try {
        const c = OD.census();
        // Siempre una línea, también cuando NO hay ninguno: "no hay fichero
        // gordo en memoria" es justo el dato que dice que cuts.img (115 MB) no
        // llegó (escena de cinemática vacía) en vez de "llegó y no se lee".
        OD.trace('ODBIG n=' + (c.bigN || 0) + ' ' + (c.big || ''));
      } catch (e) {}
    }, 60000);
    // Bucles primero (motor, sirenas, ambientes): son los que suenan en cuanto
    // conduces y no pueden permitirse una descarga/decodificación a mitad.
    setTimeout(() => { OD.prefetchAnnouncements(); }, 4000);
    setTimeout(() => { OD.prefetchLoops(); }, 6000);
  },

  // Trae en silencio las muestras con puntos de loop (las lee de la propia
  // tabla SDT que ya está en el FS virtual). Evita el tirón de la primera vez
  // que suena un motor.
  async prefetchAnnouncements() {
    for (const p of ['/Audio/bclosed.mp3', '/Audio/bopen.mp3']) {
      try { await OD._ensure(p); } catch (e) {}
    }
  },
  async prefetchLoops() {
    try {
      let p = '/Audio/sfx.SDT';
      if (!OD.exists(p)) {
        try { p = await OD.ensure('/Audio/sfx.SDT'); } catch (e) { p = null; }
      }
      if (!p) { odLog('[od] loops: sin SDT'); return; }
      const u8 = FS.readFile(p, { encoding: 'binary' });
      const n = Math.floor(u8.length / 20);
      const list = [];
      for (let i = 0; i < n; i++) {
        const o = i * 20;
        const size = (u8[o + 4] | (u8[o + 5] << 8) | (u8[o + 6] << 16) | (u8[o + 7] << 24)) >>> 0;
        if (!size) continue;
        const ls = (u8[o + 12] | (u8[o + 13] << 8) | (u8[o + 14] << 16) | (u8[o + 15] << 24)) >>> 0;
        const le = (u8[o + 16] | (u8[o + 17] << 8) | (u8[o + 18] << 16) | (u8[o + 19] << 24)) | 0;
        if (ls !== 0 || (le !== 0 && le !== -1)) list.push(i);
      }
      odLog('[od] samples con loop en la SDT: ' + list.length);
      let done = 0;
      for (const i of list) {
        try { if (await OD.ensure('/Audio/sfx/' + i + '.mp3')) done++; } catch (e) {}
        if (done % 20 === 0) await OD.sleep(40);
      }
      odLog('[od] bucles precargados ' + done + '/' + list.length);
    } catch (e) { odLog('[od] prefetch loops: ' + (e && e.message || e)); }
  },

  // ------------------------------------------------------------------------
  // F6b · Arranque en caliente del primer minuto
  //
  // Medido: en el primer minuto de juego hay un hipo de 20-40 ms (y picos de
  // 110-266 ms) por cada zona nueva. La causa es que CADA fichero que el motor
  // toca por primera vez cuesta una suspensión Asyncify + una lectura de la
  // caché IDB (~3,3 ms), y en una zona nueva pide ~68 por segundo (226 ms de
  // trabajo en un segundo). Ese trabajo es el mismo se haga cuando se haga:
  // lo que cambia es DÓNDE se paga. Aquí se adelanta a la pantalla de carga.
  //
  // Sin saber nada del mapa: se aprende del propio uso. Se anota lo que el
  // motor pide en la sesión y la siguiente se precarga durante la carga del
  // mundo, empezando por los ficheros MÁS PEQUEÑOS (cada uno cuesta una
  // suspensión igual que uno grande, así que los pequeños rinden más por MB).
  // Presupuesto en MB (cfg.warmMB) y lista en localStorage (atada a la clave de la caché).
  warmKey: 'vc_warm1',
  warmList: null,
  warmSeen: null,
  warmStarted: false,
  warmMaxFile: 12 * 1048576,
  noteWarm(canon, size) {
    if (!OD.cfg.warmMB || !canon) return;
    if (size && size > OD.warmMaxFile) return;
    if (/\.adf$/i.test(canon)) return;               // emisoras: van por su cuenta
    if (!OD.warmSeen) OD.warmSeen = new Set();
    if (OD.warmSeen.size < 20000) OD.warmSeen.add(canon);
  },
  warmLoad() {
    if (OD.warmList) return OD.warmList;
    let list = [];
    try {
      const raw = JSON.parse(localStorage.getItem(OD.warmKey) || 'null');
      if (raw && raw.tag === OD.cacheKey() && Array.isArray(raw.list)) list = raw.list;
    } catch (e) {}
    OD.warmList = list;
    return list;
  },
  warmPersist() {
    try {
      if (!OD.warmSeen || !OD.warmSeen.size) return;
      const set = new Set(OD.warmLoad());
      const n0 = set.size;
      for (const p of OD.warmSeen) set.add(p);
      if (set.size === n0) return;
      const list = [...set].slice(-6000);
      localStorage.setItem(OD.warmKey, JSON.stringify({ tag: OD.cacheKey(), list }));
      if (OD.verbose) odLog('[od] lista de arranque en caliente: ' + list.length + ' ficheros');
    } catch (e) {}
  },
  // El motor no avisa "voy a cargar el mundo"; sí llama a __loadProgress en
  // cada paso de CGame::InitialiseStep. Se engancha esa llamada: la primera
  // es el inicio de la carga, que es justo la ventana muerta donde adelantar
  // trabajo no cuesta al jugador (el cartel de carga ya está en pantalla).
  hookWorldLoad() {
    try {
      const w = typeof window !== 'undefined' ? window : null;
      if (!w || w.__odWarmHooked) return;
      w.__odWarmHooked = true;
      const wrap = () => {
        const orig = w.__loadProgress;
        if (orig && !orig.__odWrapped) {
          const f = function (a, b) {
            // 82.2: una sola vez por sesión. Antes se reprogramaba cada 8 s y
            // con la precarga nueva (que espera al primer frame) habría dos
            // pasadas solapadas.
            if (!OD._warmFired) { OD._warmFired = 1; setTimeout(() => OD.warmStart(), 250); }
            return orig.apply(this, arguments);
          };
          f.__odWrapped = true;
          w.__loadProgress = f;
          return true;
        }
        return false;
      };
      if (!wrap()) { let n = 0; const t = setInterval(() => { if (wrap() || ++n > 40) clearInterval(t); }, 250); }
    } catch (e) {}
  },
  warmStart() {
    if (OD.warmStarted || !OD.cfg.warmMB) return;
    OD.warmStarted = true;
    (async () => {
      try {
        let list = OD.warmLoad();
        if (!list.length) {
          const man0 = OD.manifest || {};
          list = Object.keys(man0).map((k) => '/' + man0[k].p).filter((p) => p && p.toLowerCase().indexOf('.adf') < 0);
          OD.trace('ODWARM sin lista previa: precarga amplia de ' + list.length + ' ficheros');
        }
        const budget = OD.cfg.warmMB * 1048576;
        const man = OD.manifest || {};
        const items = [];
        for (const p of list) {
          const h = man[OD.norm(p)];
          const s = (h && h.s) || 0;
          if (s && s <= OD.warmMaxFile) items.push([p, s]);
        }
        items.sort((a, b) => a[1] - b[1]);
        // 82.2: la precarga ya NO corre durante el arranque. En la traza de
        // 82.1 empezó con la carga del mundo y siguió 8,2 s (5.441 ficheros,
        // 64 MB escritos en MEMFS desde el hilo del juego) con la cinemática
        // INT_A ya empezada y el motor pidiendo sus modelos. Ahora espera al
        // primer frame, va en huecos libres del navegador y NO escribe en
        // MEMFS: los bytes quedan retenidos (wkPut) hasta que el motor pida el
        // fichero, que es cuando la copia ya no se nota.
        // 82.3: `__vcState >= 9` ya es cierto DURANTE la carga, asi que la
        // precarga arrancaba con el mundo a medias (5.347 ficheros a los 11 s
        // de arrancar) y competia con los criticos en un worker serial. Ahora
        // espera a estar DENTRO de la partida de verdad.
        const inGame = () => {
          try { return OD.gameLive() && window.__vcInGame === true; } catch (e) { return false; }
        };
        for (let i = 0; i < 480 && !inGame(); i++) await OD.sleep(250);
        if (!inGame()) { OD.trace('ODWARM sin partida: no se precarga'); return; }
        const t0 = Date.now();
        let n = 0, bytes = 0, touched = 0, blkWait = 0, fails = 0;
        for (const [p, s] of items) {
          if (bytes + s > budget) break;
          if (Date.now() - t0 > 120000) break;                 // tope por sesión
          touched++;
          if (OD.exists(p) || (OD.wkReady && OD.wkReady.has(p))) continue;
          let blk = 0;
          try { blk = window.__vcODBlock | 0; } catch (e) {}
          if (blk > 0) {                                        // carga bloqueante en curso
            if (++blkWait > 150) break;
            await OD.sleep(200);
            continue;
          }
          await OD.idle();                                      // hueco entre frames
          const h = man[OD.norm(p)];
          if (!h) continue;
          OD.wkAsk(p, h);
          const buf = await OD.wkWait(p, 4000);
          if (buf) { fails = 0; OD.wkPut(p, buf); bytes += buf.byteLength || s; n++; }
          else if (++fails > 5) break;                          // worker sin respuesta: se deja
          // 82.3: deja huecos a la ruta critica y acota la precarga por sesion.
          if (touched % 20 === 0) await OD.sleep(40);
          if (n >= 1500) break;
        }
        OD.trace('ODWARM n=' + n + ' mb=' + Math.round(bytes / 1048576) + ' ms=' + (Date.now() - t0) +
          ' lista=' + items.length + ' ya_en_mem=' + (touched - n));
      } catch (e) { OD.trace('ODWARM error: ' + (e && e.message || e)); }
    })();
  },

  // ------------------------------------------------------------------------
  // Nivel 1 · El trabajo de carga en un Web Worker
  //
  // Antes, traer un fichero (leer la caché IDB y, si falta, red + escribir
  // caché) lo hacía el hilo del juego: el frame esperaba entero (medido, hipos
  // de 20-280 ms al entrar en zona nueva). Ahora ese trabajo va a un Worker y
  // al hilo del juego solo le queda copiar bytes a MEMFS (microsegundos).
  //
  // Además, si el mundo ya está cargado (primer frame dibujado: el motor
  // publica `__vcState >= 9`, no `__vcInGame`, que se apaga al abrir mapa o
  // pausa), un fichero que aún no está se contesta "no está" al
  // instante: el motor pide los modelos en cada pasada de streaming y reintenta,
  // así que ningún frame se congela y el fichero entra un par de frames después,
  // ya traído. Durante la CARGA (menú o partida) NO se aplaza nada: esa ruta es
  // crítica y exige los ficheros en el momento.
  // ------------------------------------------------------------------------
  wk: null,
  // R2: la emisora de 30 MB que se precarga retenida (sin escribir MEMFS) tiene
  // que caber aquí; antes eran 24 MB y se tiraba sola.
  wkReadyCap: 40 * 1048576,
  wkReady: null,
  wkReadyBytes: 0,
  wkPending: null,
  wkWaiters: null,
  wkFails: null,
  wkStats: null,
  wkTO: null,
  wkErrs: null,
  wkErrShown: 0,
  wkMs: null,
  waitN: 0,
  waitMs: 0,
  wkLastLine: 0,
  _wkNote: null,
  wkInit() {
    OD.wkReady = new Map();
    OD.wkPending = {};
    OD.wkWaiters = {};
    OD.wkFails = {};
    OD.wkErrs = {};
    OD.wkStats = { ask: 0, take: 0, fast: 0, idb: 0, opfs: 0, net: 0, err: 0, ms: 0, wrote: 0, held: 0 };
    OD.wkMs = {};
    OD.wkTO = {};
  },
  wkStart() {
    if (OD.wkWaiters && Object.keys(OD.wkWaiters).length) OD.wkFlushWaiters('reinicio-worker');
    OD.wkInit();
    if (OD.cfg.worker === false) { OD.wkNote('desactivado por configuracion'); OD.wk = null; return; }
    if (typeof Worker === 'undefined' || typeof Blob === 'undefined') { OD.wkNote('este navegador no tiene Worker'); return; }
    try {
      const url = URL.createObjectURL(new Blob([WORKER_SRC], { type: 'text/javascript' }));
      const w = new Worker(url);
      w.onmessage = (e) => OD.wkOnMsg(e);
      w.onerror = (e) => {
        OD.wk = null;
        // T17: un worker muerto dejaba a `wkWait` esperando para SIEMPRE (el
        // motor se quedaba suspendido y el juego congelado sin traza). Sus
        // esperas se cierran en el acto y el motor reintenta por el camino de
        // siempre en el siguiente frame.
        OD.wkFlushWaiters('worker-onerror');
        OD.wkNote('error del worker: ' + ((e && e.message) || e));
      };
      // OJO: un worker creado desde un Blob no tiene la página como base, así
      // que una ruta relativa NO se puede resolver ahí dentro ('Failed to parse
      // URL'). Se le manda la base ya absoluta.
      let absBase = OD.cfg.streamedUrl;
      try { absBase = new URL(OD.cfg.streamedUrl, location.href).href; } catch (e) {}
      if (absBase.slice(-1) !== '/') absBase += '/';
      w.postMessage({ op: 'init', streamedUrl: absBase, ver: OD.cacheKey() });
      OD.wk = w;
    } catch (e) { OD.wk = null; OD.wkNote('no se pudo crear: ' + ((e && e.message) || e)); }
  },
  // PRECARGA DE ARRANQUE (82.9-T7). El motor abre DATA/*.DAT, *.CFG, *.IDE y
  // TEXT/*.GXT nada mas arrancar y NO espera: si el fichero aun no esta en
  // MEMFS se cae. Aqui se copian esos ficheros, ya descargados, antes de main.
  BOOT_PRELOAD: false,
  // 83.1: ficheros que CFileLoader::LoadLevel("DATA\\DEFAULT.DAT") abre en el
  // hilo del motor sin poder reintentar (IDE, wheels.txd, generic.txd,
  // air_vlo.DFF, wheels.DFF) mas los de los pasos 1/2 (particle, hud). Tienen
  // que estar ENTEROS en MEMFS antes de que main arranque.
  BOOT_CRITICAL: [
    '/data/default.dat',
    '/data/maps/generic.ide',
    '/models/generic/wheels.TXD',
    '/models/generic.txd',
    '/models/generic/air_vlo.DFF',
    '/models/generic/wheels.DFF',
    '/models/particle.txd',
    '/models/hud.txd',
  ],
  // Cabecera de stream RenderWare: [tipo u32][tamano u32][version u32]. Se
  // exige el tipo correcto (0x16 TEXDICTIONARY, 0x10 CLUMP), que el chunk
  // declarado quepa en el fichero (una entrada truncada declara mas bytes de
  // los que hay) y como mucho 12 de cola (air_vlo.DFF original los lleva).
  // Leer 12 bytes distingue un .txd/.dff entero de basura con el mismo nombre;
  // una entrada mala aqui es lo que dejaba al motor dentro de LoadTexDictionary.
  rwOk(path) {
    let len = 0;
    try { len = FS.stat(path).size; } catch (e) { return false; }
    const isTxd = /\.txd$/i.test(path), isDff = /\.dff$/i.test(path);
    if (!isTxd && !isDff) return len > 0;
    if (len < 12) return false;
    let h;
    try {
      const st = FS.open(path, 'r');
      h = new Uint8Array(12);
      FS.read(st, h, 0, 12, 0);
      FS.close(st);
    } catch (e) { return false; }
    const u32 = (i) => (h[i] | (h[i + 1] << 8) | (h[i + 2] << 16) | (h[i + 3] << 24)) >>> 0;
    const sz = u32(4);
    const slack = len - (sz + 12);
    return u32(0) === (isTxd ? 0x16 : 0x10) && slack >= 0 && slack <= 12;
  },
  bootPreload(Module) {
    if (OD.BOOT_PRELOAD === false) return;
    const man = OD.manifest || {};
    const keys = [];
    for (const k in man) {
      const p = (man[k] && man[k].p) || k;
      // 83.0: ademas de data/text, la portada (txd/loadsc*) y los modelos que
      // LoadLevel abre sin esperar (generic/wheels/air_vlo/particle/hud/fronten).
      // 83.1: copiar en MEMFS no basta si los bytes son malos (ver rwOk y la
      // pasada sobre BOOT_CRITICAL); aqui solo se elige QUE precargar.
      if (/^(data|text|txd)\//i.test(p) ||
          /^models\/(generic\.txd|generic\/|particle\.txd|hud\.txd|fronten1\.txd|fronten2\.txd)/i.test(p)) keys.push(k);
    }
    if (!keys.length) { OD.trace('BOOTPRE n=0 (manifest vacio)'); return; }
    const t0 = Date.now();
    let n = 0, bytes = 0, still = 0, bad = 0, fix = 0;
    try { Module.addRunDependency('odBoot'); } catch (e) {}
    (async () => {
      try {
        for (const k of keys) {
          const hit = man[k], nom = '/' + hit.p, want = hit.s | 0;
          let have = 0;
          try { have = FS.stat(nom).size; } catch (e) {}
          if (have && (!want || have === want)) continue;
          const b = await OD.idbGetT(nom);
          // 83.1: de IDB solo se copia si el tamano es el del manifiesto. Antes
          // bastaba byteLength > 0, asi que una entrada truncada o envenenada
          // se escribia tal cual y el motor se quedaba dentro de ella.
          if (b && b.byteLength && (!want || b.byteLength === want)) {
            try {
              const slash = nom.lastIndexOf('/');
              if (slash > 0) { try { FS.mkdirTree(nom.slice(0, slash)); } catch (e) {} }
              FS.writeFile(nom, new Uint8Array(b)); n++; bytes += b.byteLength;
            } catch (e) {}
          } else if (b && b.byteLength) {
            bad++;
            try { await OD.idbDelete([nom]); } catch (e) {}
          } else still++;
        }
        // 83.1: la lista critica se comprueba de verdad, no por tamano de IDB.
        // Lo que no este entero se pide al pack AHORA, antes de soltar main:
        // sin esto el motor abria el hueco y se quedaba esperando.
        for (const nom of OD.BOOT_CRITICAL) {
          if (OD.rwOk(nom)) continue;
          try { OD.live.delete(nom); } catch (e) {}
          try { FS.unlink(nom); } catch (e) {}
          try { await OD.idbDelete([nom]); } catch (e) {}
          if (!man[OD.norm(nom)]) continue;
          try { await OD._ensure(nom); } catch (e) {}
          if (OD.rwOk(nom)) fix++;
        }
      } finally {
        OD.trace('BOOTPRE n=' + n + ' bytes=' + bytes + ' ms=' + (Date.now() - t0) +
          ' faltan=' + still + ' mal=' + bad + ' repuestos=' + fix);
        try { Module.removeRunDependency('odBoot'); } catch (e) {}
      }
    })();
  },
  wkNote(msg) {
    if (OD._wkNote === msg) return;
    OD._wkNote = msg;
    odLog('[od] carga en hilo no disponible (' + msg + '): se usa el camino de siempre');
    OD.trace('ODWK off (' + msg + ')');
  },
  wkAsk(canon, hit) {
    if (!OD.wk || !OD.wkPending) return;
    // 82.3: una peticion VIVA no se vuelve a encolar. Antes, al vencer la
    // espera se borraba `wkPending` mientras el worker seguia con ella, el
    // motor reintentaba y se pedia el mismo fichero otra vez. En la traza
    // INT_A de 82.2 eso fueron 5.481 peticiones en 33 s y la pestaña murio
    // con el heap en 472 MB. Se deja caducar a los 20 s por si el worker
    // perdio el mensaje de verdad.
    const at = OD.wkPending[canon] | 0;
    if (at && Date.now() - at < 20000) return;
    OD.wkPending[canon] = Date.now();
    OD.wkStats.ask++;
    let rel = canon.replace(/^\//, '');
    let size = (hit && hit.s) || 0;
    try {
      const h = OD.manifest && OD.manifest[OD.norm(canon)];
      if (h) { if (h.p) rel = h.p; if (h.s) size = h.s; }
    } catch (e) {}
    try { OD.wk.postMessage({ op: 'get', key: canon, rel, size }); }
    catch (e) { delete OD.wkPending[canon]; OD.wkStats.err++; }
  },
  wkTake(canon) {
    if (!OD.wkReady) return null;
    const b = OD.wkReady.get(canon);
    if (!b) return null;
    OD.wkReady.delete(canon);
    OD.wkReadyBytes -= b.byteLength;
    return b;
  },
  wkPut(canon, buf) {
    if (!OD.wkReady) return;
    const prev = OD.wkReady.get(canon);
    if (prev) OD.wkReadyBytes -= prev.byteLength;
    OD.wkReady.set(canon, buf);
    OD.wkReadyBytes += buf.byteLength;
    while (OD.wkReadyBytes > OD.wkReadyCap) {
      const it = OD.wkReady.keys().next();
      if (it.done) break;
      OD.wkReadyBytes -= OD.wkReady.get(it.value).byteLength;
      OD.wkReady.delete(it.value);
    }
  },
  // T17: toda espera por el worker tiene fecha de caducidad. Antes, un mensaje
  // perdido o un worker muerto sin `onerror` dejaban el await del motor
  // pendiente para siempre: el juego se congelaba (mainSilentMs creciendo y
  // mark/ent quietos) sin error y sin forma de salir. Al vencer se contesta
  // null: el motor sigue y vuelve a pedirlo. `ms` largo solo en carga
  // bloqueante, donde el motor no puede reintentar en otro frame.
  wkWait(canon, ms) {
    const deadline = ms || 2500;
    return new Promise((res) => {
      if (!OD.wkWaiters) OD.wkInit();
      let done = false;
      const finish = (v) => { if (done) return; done = true; res(v); };
      const arr = (OD.wkWaiters[canon] = OD.wkWaiters[canon] || []);
      arr.push(finish);
      setTimeout(() => {
        if (done) return;
        const a = OD.wkWaiters[canon];
        if (a) {
          const i = a.indexOf(finish);
          if (i >= 0) a.splice(i, 1);
          // 82.3: NO se borra `wkPending`: la peticion sigue viva en el worker.
          // Solo se cuenta el fallo, para dejar de aplazar este fichero.
          if (!a.length) delete OD.wkWaiters[canon];
        }
        if (OD.wkTO) OD.wkTO[canon] = (OD.wkTO[canon] || 0) + 1;
        OD.wkTimeouts++;
        if (OD.wkTimeouts <= 8) OD.trace('ODWKTMO ' + canon + ' ms=' + deadline);
        finish(null);
      }, deadline);
    });
  },
  wkTimeouts: 0,
  wkFlushWaiters(why) {
    const w = OD.wkWaiters || {};
    OD.wkWaiters = {};
    OD.wkPending = {};
    let n = 0;
    for (const k of Object.keys(w)) {
      const arr = w[k] || [];
      for (const fn of arr) { n++; try { fn(null); } catch (e) {} }
    }
    if (n) OD.trace('ODWKDEAD ' + why + ' esperas=' + n);
  },
  // Pide la migración IDB->OPFS una sola vez y ya con el mundo cargado: son
  // ~7.500 ficheros y durante la carga competiría con el propio arranque.
  migrateMaybe() {
    if (OD._migAsked || !OD.wk || !OD.gameLive()) return;
    OD._migAsked = true;
    try { OD.wk.postMessage({ op: 'migrate' }); } catch (e) { OD._migAsked = false; }
  },
  // "El mundo está cargado": el motor ya dibujó el primer frame. Se mira
  // `__vcState` (gGameState; GS_PLAYING_GAME = 9), NO `__vcInGame`, que se
  // apaga al abrir mapa o pausa y volvía a meter la espera de 30 s en medio de
  // la partida: 3 s de congelación medidos en la sesión 81.5.
  gameLive() {
    return typeof window !== 'undefined' && (window.__vcState | 0) >= 9;
  },
  // Solo se aplaza en partida, y nunca encadenando fallos del mismo fichero
  // (tras 8 seguidos se espera: antes esperar que quedarse sin fichero).
  wkFastFail(canon) {
    if (!OD.wk) return false;
    if (!OD.gameLive()) return false;
    // El motor avisa cuando está en una carga BLOQUEANTE (LoadAllRequestedModels,
    // preparación de cinemática): ahí no se aplaza nada, porque el motor no
    // puede reintentar en otro frame y se quedaría sin el modelo/audio.
    if ((window.__vcODBlock | 0) > 0) return false;
    // Tras 3 aplazamientos seguidos del MISMO fichero se espera: durante una
    // carga bloqueante (escena de cinemática, preload de audio) el motor no
    // puede reintentar en otro frame y se quedaría sin el fichero. Esperar 3
    // frames (~50 ms) antes de darse por vencido es barato y evita escenas
    // vacías.
    if ((OD.wkFails[canon] || 0) >= 3) return false;
    // Un fichero que el worker no puede traer (no existe, 404) no se aplaza
    // para siempre: tras 3 fallos se deja pasar al camino de siempre, que ya
    // sabe fallar de forma controlada.
    if ((OD.wkErrs[canon] || 0) >= 3) return false;
    OD.wkFails[canon] = (OD.wkFails[canon] || 0) + 1;
    OD.wkStats.fast++;
    return true;
  },
  // true si el worker está vivo y el mundo ya está cargado (para no caer al
  // camino bloqueante cuando el worker falla: el motor reintenta en otro frame).
  wkInGame() {
    return !!OD.wk && OD.gameLive();
  },
  wkOnMsg(e) {
    const m = (e && e.data) || {};
    if (m.op === 'ready') {
      if (OD.verbose) odLog('[od] worker de carga listo');
      // Una línea por sesión: dice si la caché corre en OPFS o sigue en IDB.
      OD.trace('ODCACHE opfs=' + (m.opfs ? 1 : 0));
      return;
    }
    if (m.op === 'mig') {
      if (m.done || (m.n && m.n % 1000 === 0)) {
        OD.trace('ODCMIG n=' + m.n + ' mb=' + m.mb + (m.done ? ' fin=1' : ''));
      }
      return;
    }
    if (!OD.wkPending) OD.wkInit();
    if (m.key && OD_PROBE.test(m.key)) odProbe('wkmsg ' + m.op + ' ' +
      (m.buf ? m.buf.byteLength : (m.msg || '')) + ' src=' + (m.src || '-') + ' ms=' + (m.ms || 0));
    delete OD.wkPending[m.key];
    delete OD.wkFails[m.key];
    if (OD.wkTO) delete OD.wkTO[m.key];
    if (m.op === 'err') {
      // Los primeros fallos, siempre en el log (con esto se ve la causa real
      // sin adivinar); después basta con el contador.
      OD.wkErrs[m.key] = (OD.wkErrs[m.key] || 0) + 1;
      if (OD.wkErrShown < 5) {
        OD.wkErrShown++;
        OD.trace('ODWKERR ' + m.key + ' :: ' + m.msg + ' (' + (m.ms || 0) + 'ms)');
      }
    }
    if (m.op === 'data') {
      OD.wkStats[m.src === 'opfs' ? 'opfs' : (m.src === 'idb' ? 'idb' : 'net')]++;
      OD.wkStats.ms += m.ms || 0;
      // 82.2: una entrega lenta del worker (>600 ms) deja rastro aunque nadie
      // la estuviera esperando; es el dato que separa "worker atascado" de
      // "el motor no la pidió".
      if ((m.ms || 0) > 600 && (OD.wkSlowShown || 0) < 10) {
        OD.wkSlowShown = (OD.wkSlowShown || 0) + 1;
        OD.trace('ODWSLOW ' + m.key + ' ms=' + Math.round(m.ms) + ' src=' + (m.src || '?') +
          ' mb=' + ((m.buf ? m.buf.byteLength : 0) / 1048576).toFixed(1));
      }
      // 82.6: una entrega desde red de un fichero ya entregado antes = se ha
      // bajado dos veces. Se deja en la traza para vigilarlo.
      if (m.src !== 'opfs' && m.src !== 'idb') {
        if (!OD.netSeen) OD.netSeen = {};
        if (OD.netSeen[m.key]) {
          if ((OD.wkDup || 0) < 6) {
            OD.wkDup = (OD.wkDup || 0) + 1;
            OD.trace('ODWDUP ' + m.key + ' mb=' + ((m.buf ? m.buf.byteLength : 0) / 1048576).toFixed(1));
          }
        } else OD.netSeen[m.key] = 1;
      }
      OD.wkMs[m.key] = [m.ms || 0, m.src === 'opfs' ? 'opfs' : (m.src === 'idb' ? 'idb' : 'red')];
      const arr = OD.wkWaiters[m.key];
      if (arr) { delete OD.wkWaiters[m.key]; for (const fn of arr) fn(m.buf); }
      else {
        // Nadie espera (lo normal en partida: el frame siguió sin el fichero).
        // Se escribe en MEMFS YA — copiar es microscópico — para que el motor
        // lo encuentre puesto cuando lo vuelva a pedir, en vez de quedarse en
        // memoria esperando una petición que podría no llegar (huecos).
        try { OD.writeFile(m.key, m.buf); } catch (e) { OD.wkPut(m.key, m.buf); }
      }
      OD.wkStats.take++;
      OD.wkMaybeTrace();
      return;
    }
    if (m.op === 'err') {
      OD.wkStats.err++;
      const arr = OD.wkWaiters[m.key];
      if (arr) { delete OD.wkWaiters[m.key]; for (const fn of arr) fn(null); }
      if (OD.verbose) odLog('[od] worker falló ' + m.key + ': ' + m.msg);
      OD.wkMaybeTrace();
    }
  },
  wkMaybeTrace() {
    const now = Date.now();
    if (now - OD.wkLastLine < 30000) return;
    OD.wkLastLine = now;
    const s = OD.wkStats;
    OD.trace('ODWK peticiones=' + s.ask + ' aplazados=' + s.fast +
      ' entregados=' + s.take + ' escritos=' + s.wrote + ' esperando=' + Math.round(OD.wkReadyBytes / 1048576) + 'MB' +
      ' idb=' + s.idb + ' red=' + s.net + ' err=' + s.err + ' ms=' + Math.round(s.ms) +
      ' esperas100=' + OD.waitN + ' ms_esp=' + Math.round(OD.waitMs));
  },
  // --------------------------------------------------------------------
  // R2 (plan 06, 5ª partida): diagnóstico del congelado al cargar emisora.
  //
  // En la traza del jugador el final de la partida es un tirón de **1256 ms de
  // espera** (`FPHASE wait=1256.5`, con la lógica a 0,3 ms): un bloqueo del hilo
  // del juego. Desde esta capa sólo hay dos cosas capaces de hacerlo:
  //   1. que `ensure` ESPERE (await) por un `.adf` de emisora; y
  //   2. que la entrega escriba un fichero grande en MEMFS en el hilo principal
  //      (la copia no se puede evitar: el motor lo lee con fopen sobre MEMFS).
  // `ODSTA` deja constancia de lo primero y `ODWRITE` mide lo segundo. Sin estos
  // dos datos, "la emisora congela" es teoría.
  // --------------------------------------------------------------------
  _adfLog: null,
  adfTrace(name, memfs, ms, extra) {
    let now;
    try { now = performance.now(); } catch (e) { now = Date.now(); }
    if (!OD._adfLog) OD._adfLog = {};
    const st = OD._adfLog[name] || (OD._adfLog[name] = { t: -1e9, n: 0 });
    st.n++;
    if (now - st.t < 1000) return;          // una línea por segundo y emisora
    st.t = now;
    let block = 0;
    try { block = window.__vcODBlock | 0; } catch (e) {}
    OD.trace('ODSTA ' + name + ' memfs=' + (memfs ? 1 : 0) + ' n=' + st.n
      + ' ms=' + Math.round(ms || 0) + ' block=' + block + (extra ? ' ' + extra : ''));
  },

  // Escritura a MEMFS: lo ÚNICO que le queda al hilo del juego (una copia).
  writeFile(canon, buffer) {
    let t0 = 0;
    try { t0 = performance.now(); } catch (e) {}
    const u8 = new Uint8Array(buffer);
    if (OD_PROBE.test(canon)) odProbe('write ' + canon + ' ' + u8.length);
    const slash = canon.lastIndexOf('/');
    const parent = slash > 0 ? canon.slice(0, slash) : '/';
    const name = canon.slice(slash + 1);
    try { FS.mkdirTree(parent); } catch (e) {}
    try { FS.unlink(canon); } catch (e) {}
    FS.createDataFile(parent, name, u8, true, true, true);
    if (OD.wkStats) OD.wkStats.wrote++;
    OD.touch(canon, u8.length);
    OD.idbTouch(canon);
    OD.noteWarm(canon, u8.length);
    // R2: sólo los grandes (los que pueden notarse como tirón).
    if (u8.length > 4 * 1048576) {
      let ms = 0;
      try { ms = performance.now() - t0; } catch (e) { ms = 0; }
      OD.trace('ODWRITE ' + canon + ' mb=' + (u8.length / 1048576).toFixed(1) + ' ms=' + Math.round(ms));
    }
    return canon;
  },

  idbGet(key) {
    return new Promise((res) => {
      if (!OD.db) return res(null);
      try {
        const t = OD.db.transaction(['files'], 'readonly').objectStore('files').get(key);
        t.onsuccess = (e) => res(e.target.result || null);
        t.onerror = () => res(null);
      } catch (e) { res(null); }
    });
  },

  idbPut(key, buf) {
    return new Promise((res) => {
      if (!OD.db) return res();
      try {
        const t = OD.db.transaction(['files'], 'readwrite').objectStore('files').put(buf, key);
        t.onsuccess = () => res();
        t.onerror = () => res();
      } catch (e) { res(); }
    });
  },

  idbClear() {
    return new Promise((res) => {
      if (!OD.db) return res();
      try {
        const tx = OD.db.transaction(['files', 'meta'], 'readwrite');
        tx.objectStore('files').clear();
        tx.objectStore('meta').clear();
        tx.oncomplete = () => res();
        tx.onerror = () => res();
      } catch (e) { res(); }
    });
  },

  // ------------------------------------------------------------------------
  // F6a · Caché IDB con techo y desalojo por uso (LRU)
  //
  // 'vcod2' guarda una copia de TODO lo que se ha traído nunca: es lo que hace
  // que el primer minuto no dependa de la red, pero crecía sin cota (medido:
  // 1.280 MB en una sesión larga). Ahora hay tope (`idbCapMB`) y cuando se pasa
  // se borra lo MÁS VIEJO en uso hasta quedar por debajo; lo borrado se
  // re-descarga solo si se vuelve a necesitar. El uso se anota en el store
  // 'meta' ({t, s} por clave) en lotes, para no escribir en cada lectura.
  // ------------------------------------------------------------------------
  _idbPairs(store, keysOnly) {
    return new Promise((res) => {
      if (!OD.db) return res([]);
      try {
        const tx = OD.db.transaction([store], 'readonly');
        const st = tx.objectStore(store);
        const kq = st.getAllKeys();
        const vq = keysOnly ? null : st.getAll();
        tx.oncomplete = () => {
          const ks = kq.result || [];
          if (keysOnly) { res(ks.map((k) => [k, null])); return; }
          const vs = vq.result || [];
          res(ks.map((k, i) => [k, vs[i]]));
        };
        tx.onerror = () => res([]);
      } catch (e) { res([]); }
    });
  },

  idbMetaPut(entries) {           // entries: [[clave, {t,s}], ...] en UNA transacción
    return new Promise((res) => {
      if (!OD.db || !entries.length) return res();
      try {
        const tx = OD.db.transaction(['meta'], 'readwrite');
        const st = tx.objectStore('meta');
        for (const [k, v] of entries) st.put(v, k);
        tx.oncomplete = () => res();
        tx.onerror = () => res();
      } catch (e) { res(); }
    });
  },

  idbDelete(keys) {
    return new Promise((res) => {
      if (!OD.db || !keys.length) return res();
      try {
        const tx = OD.db.transaction(['files'], 'readwrite');
        const st = tx.objectStore('files');
        for (const k of keys) st.delete(k);
        tx.oncomplete = () => res();
        tx.onerror = () => res();
      } catch (e) { res(); }
    });
  },

  // Acceso (lectura de caché o descarga): se anota el uso en lote.
  _idbTouch: null,
  idbTouch(key) {
    if (!OD.db) return;
    if (!OD._idbTouch) OD._idbTouch = new Set();
    if (OD._idbTouch.size < 8000) OD._idbTouch.add(key);
  },

  // Tamaño de un fichero de la caché: de 'meta' si lo tiene, si no del
  // manifiesto (las entradas escritas antes de F6a no tienen 'meta').
  idbSizeOf(key, meta) {
    if (meta && meta.s) return meta.s;
    try { const h = OD.manifest && OD.manifest[OD.norm(key)]; if (h && h.s) return h.s; } catch (e) {}
    return 0;
  },

  async idbTrim(reason) {
    if (!OD.db) return;
    const capMB = OD.cfg.idbCapMB;
    if (!capMB || capMB <= 0) return;
    const t0 = Date.now();
    const pairs = (await OD._idbPairs('files', true)).filter(([k]) => k !== OD.cacheKeyName);
    if (!pairs.length) return;
    const metas = new Map(await OD._idbPairs('meta'));
    let total = 0;
    const ents = [];
    for (const [k] of pairs) {
      const m = metas.get(k) || null;
      const s = OD.idbSizeOf(k, m);
      total += s;
      ents.push([k, (m && m.t) || 0, s]);
    }
    const totalMB = Math.round(total / 1048576);
    if (total <= capMB * 1048576) {
      OD.trace('IDBTRIM motivo=' + (reason || 'init') + ' n=' + pairs.length + ' uso=' + totalMB + 'MB tope=' + capMB + 'MB (nada que borrar)');
      return;
    }
    ents.sort((a, b) => a[1] - b[1]);              // más viejo (o sin marca) primero
    OD.idbFlushTouches();                            // que lo accedido ahora no se borre
    const target = capMB * 1048576 * 0.9;            // margen: no repetir el trim cada sesión
    const drop = [];
    let freed = 0;
    for (const [k, , s] of ents) {
      if (total - freed <= target) break;
      drop.push(k);
      freed += s;
    }
    await OD.idbDelete(drop);
    OD.trace('IDBTRIM motivo=' + (reason || 'init') + ' antes=' + totalMB + 'MB despues=' +
      Math.round((total - freed) / 1048576) + 'MB borrados=' + drop.length + ' tope=' + capMB +
      'MB ms=' + (Date.now() - t0));
    odLog('[od] caché de datos: ' + totalMB + 'MB -> ' + Math.round((total - freed) / 1048576) +
      'MB (borrados ' + drop.length + ' ficheros, tope ' + capMB + 'MB)');
    OD.trimmed = drop.length;
  },

  async idbFlushTouches() {
    if (!OD._idbTouch || !OD._idbTouch.size) return;
    const keys = [...OD._idbTouch];
    OD._idbTouch = new Set();
    const now = Date.now();
    await OD.idbMetaPut(keys.map((k) => [k, { t: now, s: OD.idbSizeOf(k, null) }]));
  },

  // Purga por versión de datos (ver el sello __vc__ del manifiesto). Imprescindible
  // al regenerar assets: sin esto el navegador sigue sirviendo los bytes viejos.
  cacheKeyName: '__cachekey__',
  cacheKey() {
    try {
      const h = OD.manifest && OD.manifest['__vc__'] && OD.manifest['__vc__'].h;
      if (h) return 'm' + h;
    } catch (e) {}
    return OD.dataTag;
  },
  async idbPurgeIfStale() {
    if (!OD.db) return;
    const key = OD.cacheKey();
    const prev = await OD.idbGet(OD.cacheKeyName);
    if (prev !== key) {
      await OD.idbClear();
      await OD.idbPut(OD.cacheKeyName, key);
      odLog('[od] caché de datos purgada (' + (prev || 'vacía') + ' -> ' + key + ')');
      OD.trace('DATATAG old=' + (prev || 'ninguno') + ' new=' + key + ' (caché IDB purgada)');
    }
  },

  idbGetT(key) {
    // IDB no debe colgar nunca el loop: a los 8 s se trata como fallo de caché.
    return Promise.race([
      OD.idbGet(key),
      new Promise((res) => setTimeout(() => res(null), 8000)),
    ]);
  },

  // Descarga de red con timeout de inactividad + aborto (F1: un fetch colgado
  // congelaba el loop Asyncify en silencio para siempre). Lanza Error si falla.
  async dlNetwork(canon, hit, attempt) {
    const slash = canon.lastIndexOf('/');
    const parent = slash > 0 ? canon.slice(0, slash) : '/';
    const name = canon.slice(slash + 1);
    const tmp = canon + '.part';
    const INACT_MS = 25000;
    const ctrl = new AbortController();
    let timer = null;
    const poke = () => {
      if (timer) clearTimeout(timer);
      timer = setTimeout(() => { try { ctrl.abort(); } catch (e) {} }, INACT_MS);
    };
    const pend = OD.pending[canon];
    const isHtmlHead = (u8) => {
      if (u8.length < 9) return false;
      const h = String.fromCharCode(...u8.slice(0, 9)).toLowerCase();
      return h.startsWith('<!doctype') || h.startsWith('<html');
    };
    try { FS.mkdirTree(parent); } catch (e) {}
    try { FS.unlink(tmp); } catch (e) {}
    poke();
    let r;
    try {
      r = await fetch(OD.cfg.streamedUrl + hit.p.split('/').map(encodeURIComponent).join('/'), { signal: ctrl.signal });
    } catch (e) {
      throw new Error('fetch intento ' + attempt + ' (timeout ' + (INACT_MS / 1000) + 's?): ' + (e && e.message || e));
    }
    if (!r.ok) throw new Error('HTTP ' + r.status);
    const ct = (r.headers.get('content-type') || '').toLowerCase();
    if (ct.includes('text/html')) throw new Error('HTML en vez de fichero');
    const total = Number(r.headers.get('Content-Length')) || hit.s || 0;
    if (!r.body || !r.body.getReader) throw new Error('sin body legible');
    const reader = r.body.getReader();
    let off = 0, first = true, idbBuf = (hit.s <= 8 * 1048576) ? [] : null, st = null;
    const ensureOpen = () => {
      if (!st) {
        if (total > 0) FS.createDataFile(parent, name + '.part', new Uint8Array(total), true, true, true);
        else FS.createDataFile(parent, name + '.part', new Uint8Array(0), true, true, true);
        st = FS.open(tmp, 'w');
      }
    };
    try {
      for (;;) {
        let rd;
        try { rd = await reader.read(); }
        catch (e) { throw new Error('red cortada en ' + off + 'b: ' + (e && e.message || e)); }
        const { done, value } = rd;
        if (done) break;
        if (first) {
          first = false;
          if (isHtmlHead(value)) throw new Error('HTML en vez de fichero');
        }
        ensureOpen();
        FS.write(st, value, 0, value.length, off);
        off += value.length;
        if (pend) { pend.last = Date.now(); pend.bytes = off; }
        if (idbBuf) idbBuf.push(value);
        poke();
        await OD.sleep(0);
      }
    } finally {
      try { reader.cancel(); } catch (e) {}
    }
    if (timer) clearTimeout(timer);
    if (st) FS.close(st);
    if (!st || off === 0) { try { FS.unlink(tmp); } catch (e) {} throw new Error('fichero vacío'); }
    if (total > 0 && off < total) {
      try { FS.unlink(tmp); } catch (e) {}
      throw new Error('truncado ' + off + '/' + total);
    }
    try { FS.rename(tmp, canon); } catch (e) { throw new Error('rename: ' + e); }
    OD.touch(canon, off);
    OD.fetchedBytes += off;
    OD.fetchedCount++;
    if (idbBuf) {
      const flat = new Uint8Array(off);
      let p = 0;
      for (const c of idbBuf) { flat.set(c, p); p += c.length; }
      idbBuf = null;
      await OD.idbPut(canon, flat.buffer);
      await OD.idbMetaPut([[canon, { t: Date.now(), s: off }]]);
      OD.noteWarm(canon, off);
    }
    if (OD.verbose && off > 32 * 1048576) odLog('[od] OK grande ' + canon);
  },
  exists(path) {
    try { FS.stat(path); return true; } catch (e) { return false; }
  },

  // 83.1: resuelve una ruta de MEMFS sin distinguir caja y devuelve la ruta
  // REAL (con la caja del fichero) o null. El motor pide en MAYUSCULAS
  // ("MODELS\\GENERIC\\WHEELS.TXD") y MEMFS es sensible a la caja: el fichero
  // esta en MEMFS pero FS.stat de la ruta tal cual falla, la peticion acababa
  // en od_fetch -> nil y el motor se quedaba sin el. Se comprueba componente a
  // componente (stat directo y, si falla, un readdir del padre).
  fsResolve(path) {
    try { FS.stat(path); return path; } catch (e) {}
    const parts = String(path).split('/');
    let cur = '/';
    for (let i = 0; i < parts.length; i++) {
      const seg = parts[i];
      if (!seg) continue;
      const cand = cur === '/' ? '/' + seg : cur + '/' + seg;
      try { FS.stat(cand); cur = cand; continue; } catch (e) {}
      let ents = null;
      try { ents = FS.readdir(cur); } catch (e) { return null; }
      const low = seg.toLowerCase();
      let found = null;
      for (const en of ents) {
        if (en === '.' || en === '..') continue;
        if (en.toLowerCase() === low) { found = en; break; }
      }
      if (!found) return null;
      cur = cur === '/' ? '/' + found : cur + '/' + found;
    }
    return cur;
  },

  // Devuelve ruta canónica lista para abrir, o null.
  // F4a: coste de las suspensiones que pide el wasm (wall ms de ensure).
  // F5c: aquí también se detecta la emisora que pide el motor (todas las
  // vías pasan por aquí; las prefetch internas usan _ensure y no disparan).
  // F5d: fast-fail de radios — si el .adf no está aún en MEMFS, NO suspender
  // trayéndolo (frame congelado de 170-250 ms medido en F5c): fallar al
  // instante (radio muda 1-2 s), traerlo en background y dejar que el motor
  // re-intente el open (MusicManager reintenta mientras !IsStreamPlaying).
  asyncN: 0, asyncMs: 0,
  async ensure(reqPath) {
    const t0 = (typeof performance !== 'undefined') ? performance.now() : Date.now();
    const dbg = OD_PROBE.test(String(reqPath));
    if (dbg) odProbe('ensure enter ' + reqPath);
    try {
      const s = String(reqPath).replace(/\\/g, '/');
      // 83.1: fuera del manifiesto Y fuera de MEMFS = null EN EL ACTO, sin
      // await. Toda peticion que llega aqui acaba en una suspension del wasm;
      // si la ruta no la conoce nadie (SAVE/*.b, basura del motor, un nombre
      // con el CWD movido) esa suspension no tiene por que existir. Lo que ya
      // esta en MEMFS sigue mandando: presentSync lo devuelve aunque el
      // manifiesto no lo liste (son los guardados del jugador).
      if (!OD.manifestFor(reqPath) && !OD.presentSync(reqPath)) {
        OD.missNote(reqPath);
        if (dbg) odProbe('ensure -> nil (fuera-de-manifest)');
        return null;
      }
      // (el arranque en caliente se engancha en init(): hookWorldLoad)
      try {
        const sm = /([a-z0-9_]+)\.adf$/i.exec(s);
        if (sm && OD.radios.includes(sm[1].toUpperCase())) {
          const station = sm[1].toUpperCase();
          OD.noteStation(station);
          const hit = OD.manifest && OD.manifest[OD.norm(s)];
          const canon = hit ? '/' + hit.p : ('/Audio/' + station + '.ADF');
          let inMem = false;
          try { FS.stat(canon); inMem = true; } catch (e) {}
          if (inMem) {
            OD.adfTrace(station, 1, 0, '');
          } else {
            // R2: NUNCA se espera (await) por una emisora, ni siquiera durante
            // una carga bloqueante del motor: ahí es donde está el tirón de
            // 1256 ms medido. El motor reintenta mientras !IsStreamPlaying, así
            // que el peor caso es radio muda 1-2 s, como en el original. Los
            // bytes los trae el worker, fuera del hilo del juego.
            if (OD.wk) OD.wkAsk('/Audio/' + station + '.ADF', hit);
            else OD._ensure('/Audio/' + station + '.ADF').catch(() => {});
            OD.adfTrace(station, 0, 0, 'espera=0' + (OD.wk ? ' wk=1' : ' wk=0'));
            return null;
          }
        }
      } catch (e) {}
      const r = await OD._ensure(reqPath);
      if (dbg) odProbe('ensure -> ' + (r || 'nil') + ' ' + reqPath);
      return r;
    }
    finally {
      OD.asyncN++;
      OD.asyncMs += ((typeof performance !== 'undefined') ? performance.now() : Date.now()) - t0;
    }
  },
  noFetchConteo: {},
  noFetchMotivos: {},
  noFetchCierre: 0,
  noFetch(reqPath, canon, why, t0) {
    // T38 (49.2): el instrumento del 49.1 se desboco. En la partida del vuelo
    // dejo 936 lineas y en la siguiente 1664, todas del mismo motivo, y eso es
    // la basura que el jugador pidio borrar. La primera vez de cada motivo se
    // emite entera (es la que nombra el caso) y el resto se agrega en un
    // resumen cada 5 s con el fichero que mas se repite.
    const clave = (canon || '-') + '|' + why;
    const now = Date.now();
    OD.noFetchConteo[clave] = (OD.noFetchConteo[clave] || 0) + 1;
    try {
      if (!OD.noFetchMotivos[why]) {
        OD.noFetchMotivos[why] = 1;
        OD.trace('ODNOFETCH primero porque=' + why + ' path=' + reqPath +
          ' canon=' + (canon || '-') + ' ms=' + (now - t0));
      }
      if (now >= OD.noFetchCierre) {
        OD.noFetchCierre = now + 5000;
        const claves = Object.keys(OD.noFetchConteo);
        if (claves.length === 0) return;
        let total = 0, pico = '', picoN = 0;
        for (const c of claves) {
          total += OD.noFetchConteo[c];
          if (OD.noFetchConteo[c] > picoN) { picoN = OD.noFetchConteo[c]; pico = c; }
        }
        OD.trace('ODNOFETCH n=' + total + ' unicos=' + claves.length + ' pico=' + picoN +
          ' porque=' + (pico.split('|')[1] || '-') + ' fichero=' + (pico.split('|')[0] || '-'));
        OD.noFetchConteo = {};
      }
    } catch (e) {}
  },
  presentSync(reqPath) {
    let cwd = '/';
    try { cwd = FS.cwd(); } catch (e) {}
    const cands = [OD.norm(reqPath), OD.norm(cwd + '/' + reqPath)];
    // Un fichero de 0 bytes es un hueco: el paquete lo creo pero el contenido
    // aun no ha llegado. Si el manifest dice que tiene bytes, NO esta.
    let want = 0;
    for (const n of cands) {
      const h = OD.manifest && OD.manifest[n];
      if (h && h.s) { want = h.s; break; }
    }
    if (!want) {
      const e = OD.baseIndex()[OD.norm(reqPath).split('/').pop()];
      if (e) want = e.s || 0;
    }
    const odDbg = /generic|wheels|air_vlo/i.test(reqPath);
    const odNote = (res, size) => {
      if (!odDbg) return;
      try {
        const h = OD.manifest && OD.manifest[cands[0]];
        OD.trace('ODRES pedido=' + reqPath + ' norm=' + cands[0] +
          ' res=' + (res || '-') + ' size=' + (size == null ? '-' : size) +
          ' man=' + (h ? ('p=' + h.p + ' s=' + h.s) : '-'));
      } catch (e) {}
    };
    for (const n of cands) {
      const f = '/' + n;
      // 83.1: se resuelve contra el MEMFS real (sin distinguir caja) y se
      // devuelve ESA ruta; con la ruta tal cual, un pedido en mayusculas no
      // encontraba el fichero aunque estuviera puesto.
      const real = OD.fsResolve(f);
      if (!real) continue;
      try {
        const st = FS.stat(real);
        if (want && st.size === 0) continue;
        if (OD.live.has(real)) OD.touch(real, 0);
        odNote(real, st.size);
        return real;
      } catch (e) {}
    }
    let canon = null;
    for (const n of cands) {
      if (OD.manifest && OD.manifest[n]) { canon = '/' + OD.manifest[n].p; break; }
    }
    // 83.1: la ruta pedida puede no resolver (PED.DAT con el CWD en "/" y el
    // fichero puesto en /data) aunque el manifest lo tenga y ESTE ya en MEMFS.
    // Antes se devolvia null, el motor repetia la descarga del fichero entero
    // y en esa segunda vuelta se quedaba clavado (ultimo rastro: `idb ask`).
    if (!canon) {
      const mf = OD.manifestFor(reqPath);
      if (mf && mf.p) canon = '/' + mf.p;
    }
    if (canon && OD.exists(canon)) {
      let sz = null;
      try { sz = FS.stat(canon).size; } catch (e) {}
      odNote(canon, sz);
      return canon;
    }
    odNote(null, null);
    return null;
  },
  // 83.1: la misma resolucion que _ensure (ruta tal cual, contra CWD y por
  // nombre suelto) pero SIN await: permite contestar null en el acto.
  baseIndex() {
    if (OD.byBase) return OD.byBase;
    const idx = Object.create(null);
    if (OD.manifest) for (const k in OD.manifest) {
      const i = k.lastIndexOf('/');
      const b = i < 0 ? k : k.slice(i + 1);
      if (idx[b] === undefined) idx[b] = OD.manifest[k];
    }
    OD.byBase = idx;
    return idx;
  },
  missNote(reqPath) {
    if (!OD.missSeen) OD.missSeen = Object.create(null);
    if (OD.missSeen[reqPath]) return;
    OD.missSeen[reqPath] = 1;
    OD.missNote(reqPath);
  },
  manifestFor(reqPath) {
    let cwd = '/';
    try { cwd = FS.cwd(); } catch (e) {}
    const cands = [OD.norm(reqPath), OD.norm(cwd + '/' + reqPath)];
    for (const n of cands) if (OD.manifest && OD.manifest[n]) return OD.manifest[n];
    const e = OD.baseIndex()[OD.norm(reqPath).split('/').pop()];
    return e || null;
  },
  async _ensure(reqPath) {
    // El motor abre a veces con ruta relativa al CWD (p. ej. "SPANISH.GXT"
    // con CWD=/TEXT). Probar tal cual + contra CWD.
    const t0 = Date.now();
    const dbg = OD_PROBE.test(String(reqPath));
    const B = (m) => { if (dbg) odProbe(m); };
    B('_ensure enter ' + reqPath);
    const present = OD.presentSync(reqPath);
    if (present) { B('present ' + present); return present; }
    let cwd = '/';
    try { cwd = FS.cwd(); } catch (e) {}
    const cands = [OD.norm(reqPath), OD.norm(cwd + '/' + reqPath)];
    let hit = null, canon = null;
    for (const n of cands) {
      if (OD.manifest && OD.manifest[n]) { hit = OD.manifest[n]; canon = '/' + hit.p; break; }
    }
    if (!hit) {
      const e = OD.baseIndex()[OD.norm(reqPath).split('/').pop()];
      if (e) { hit = e; canon = '/' + e.p; }
    }
    B('hit ' + (hit ? canon + ' s=' + hit.s : 'nil'));
    if (!hit) {
      // 21/10: el motor pide un fichero que el manifest no tiene -> el
      // motor reintenta en bucle y la carga no acaba. Hay que verlo.
      OD.missNote(reqPath);
      return null;
    }

    // 83.0: los bytes de la cache viven en este IDB. Leerlo antes de pedirle
    // nada al worker evita el tiron de 30 s cuando el worker no contesta (el
    // motor esta suspendido en Asyncify y no puede reintentar el open).
    B('idb ask ' + canon);
    try {
      const cached = await OD.idbGetT(canon);
      B('idb ' + (cached ? cached.byteLength : 'nil') + ' ' + canon);
      if (cached && cached.byteLength) {
        OD.idbHits++;
        return OD.writeFile(canon, cached);
      }
    } catch (e) {}

    // Camino nuevo (Nivel 1): el worker trae el fichero fuera del hilo del
    // juego. Si ya lo tiene entregado, esto es solo copiarlo a MEMFS. Si no,
    // se pide y: en partida se aplaza (el motor reintenta sin congelar el
    // frame) o, durante la carga, se espera como antes. Si el worker falla, se
    // sigue por el camino de siempre (debajo), que se queda como red de
    // seguridad.
    if (OD.wk) {
      const ready = OD.wkTake(canon);
      B('wk ready=' + (ready ? ready.byteLength : 'nil') + ' ' + canon);
      if (ready) return OD.writeFile(canon, ready);
      OD.wkAsk(canon, hit);
      B('wk asked pend=' + (OD.wkPending[canon] ? 1 : 0) + ' ' + canon);
      // 82.1: el corte por presupuesto va DESPUES de saber si el fichero es
      // gordo. Medido en la traza de 82.0: `CUT begin INT_A` seguido de
      // `CUT cutsimg-FALLO INT_A`, y 138 ms despues el `BIGCREATE
      // /anim/cuts.img 110MB` durante el SEGUNDO open (el .DAT de la camara).
      // El primer `RwStreamOpen("ANIM\\CUTS.IMG")` habia entrado por
      // `wkFastFail` (en partida y con __vcODBlock=0) y devolvia null en 3 ms;
      // librw no reintenta, asi que la escena quedaba en `anim=0 cam=1`: camara
      // de cinematica correcta y los 11 personajes apilados en el offset, fuera
      // de plano. Un fichero >= 8 MB no se aplaza nunca: es ruta critica.
      const bigFile = (hit.s | 0) >= 8 * 1048576;
      if (!bigFile && OD.wkFastFail(canon)) {
        OD.noFetch(reqPath, canon, 'presupuesto', t0);
        return null;
      }
      // T18: con el mundo cargado la espera se acota a 2 s (antes 30 s dentro
      // de una carga bloqueante, que es donde se quedó congelado el juego al
      // salir un cop del coche y pedir su .ifp). Cargando (init, escena desde el
      // menú) se mantiene larga: ahí sí es la ruta crítica. El corte va por
      // `__vcState >= 9`, no por `__vcInGame`: mapa y pausa lo apagaban y
      // devolvían los 30 s a media partida.
      // 82.2: durante una carga BLOQUEANTE el motor ya esta parado y no puede
      // reintentar, asi que partir la espera en dos solo suma. Medido en INT_A:
      // `camera.txd` espero 2016 ms (tope de partida), se rindio, y el reintento
      // espero otros 1962 ms por el mismo fichero de 73 KB.
      let blkNow = 0;
      try { blkNow = window.__vcODBlock | 0; } catch (e) {}
      const live = OD.gameLive() && !blkNow;
      const wt0 = Date.now();
      // 82.0: un fichero gordo no puede compartir el tope de 2 s de partida.
      // Medido en la cinematica INT_A: `anim/cuts.img` son 115 MB y el worker
      // tardaba 2,16 s; a los 2.000 ms se rendia, `RwStreamOpen` fallaba y la
      // escena salia sin animaciones ni camara (`CUT loaded anim=0 cam=0`), o
      // sea, los objetos apilados fuera de plano. Los grandes se esperan como
      // en la carga (30 s); el resto conserva el corte corto para no congelar
      // el frame en partida. `bigFile` se calcula arriba, antes del fast-fail.
      const delivered = await OD.wkWait(canon, bigFile ? 30000 : (live ? 2000 : 30000));
      const wwait = Date.now() - wt0;
      B('wk wait -> ' + (delivered ? delivered.byteLength : 'nil') + ' ms=' + wwait +
        ' live=' + (live ? 1 : 0) + ' blk=' + blkNow + ' big=' + (bigFile ? 1 : 0) + ' ' + canon);
      let wpath = null, wmem = 0;
      if (delivered) {
        const mt0 = Date.now();
        wpath = OD.writeFile(canon, delivered);
        wmem = Date.now() - mt0;
      }
      if (wwait > 100) {
        const wi = OD.wkMs[canon] || [-1, '-'];
        OD.waitN++; OD.waitMs += wwait;
        OD.trace('ODWAIT ' + canon + ' espera=' + wwait + ' worker=' + wi[0] + ' memfs=' + wmem + ' src=' + wi[1]);
      }
      delete OD.wkMs[canon];
      if (delivered) return wpath;
      // El worker no pudo traerlo: con el mundo cargado se reintenta en otro
      // frame (el motor lo vuelve a pedir); durante la carga se sigue por el
      // camino de siempre, que es la ruta crítica y no admite aplazamientos.
      // 82.3: tras 2 esperas vencidas de ESTE fichero se deja de aplazar y se
      // baja por el camino bloqueante. Si no, el motor reintenta en bucle y
      // cada intento cuesta 2 s: 12,5 s de congelacion medidos en INT_A.
      if (OD.wkInGame() && !bigFile && ((OD.wkTO && OD.wkTO[canon]) || 0) < 2) {
        OD.noFetch(reqPath, canon, 'worker', t0);
        return null;
      }
    }

    B('inflight join=' + (OD.inflight[canon] ? 1 : 0) + ' ' + canon);
    if (OD.inflight[canon]) { await OD.inflight[canon]; return OD.exists(canon) ? canon : null; }
    const job = (async () => {
      OD.misses.push(canon + ' (' + hit.s + ')');
      if (OD.verbose && OD.misses.length % 50 === 1) odLog('[od] +' + canon);
      // Higiene de memoria: reintentar IDB (rápido, sin red) y solo bajar lo
      // que falte; escribir por tramos cediendo; IDB con contrapresión y tope.
      let cached = await OD.idbGetT(canon);
      if (cached) OD.idbHits++;
      const slash = canon.lastIndexOf('/');
      const parent = slash > 0 ? canon.slice(0, slash) : '/';
      const name = canon.slice(slash + 1);
      try { FS.mkdirTree(parent); } catch (e) {}
      try { FS.unlink(canon); } catch (e) {}
      const CH = 2 * 1048576;
      const isHtmlHead = (u8) => {
        if (u8.length < 9) return false;
        const h = String.fromCharCode(...u8.slice(0, 9)).toLowerCase();
        return h.startsWith('<!doctype') || h.startsWith('<html');
      };
      if (cached && cached.byteLength && (!hit.s || cached.byteLength >= hit.s)) {
        const u8 = new Uint8Array(cached);
        if (isHtmlHead(u8)) throw new Error('caché envenenada, re-descargar');
        FS.createDataFile(parent, name, new Uint8Array(0), true, true, true);
        const st = FS.open(canon, 'w');
        for (let off = 0; off < u8.length; off += CH) {
          FS.write(st, u8, off, Math.min(CH, u8.length - off), off);
          if (off + CH < u8.length) await new Promise((r) => setTimeout(r, 0));
        }
        FS.close(st);
        OD.touch(canon, u8.length);
        OD.idbTouch(canon);            // marca de uso (lote) para el techo LRU
        OD.noteWarm(canon, u8.length); // candidato al arranque en caliente
        return;
      }
      // Red con reintentos (F1): si un intento se cuelga o falla, se reintenta;
      // tras 3 fallos se devuelve null (fallo suave, como un 404).
      OD.pending[canon] = { t0: Date.now(), last: Date.now(), bytes: 0, try: 0 };
      B('slot ask active=' + OD.dlActive + ' cola=' + OD.dlQueue.length + ' ' + canon);
      await OD.dlSlot();
      B('slot got ' + canon);
      try {
        let lastErr = null;
        for (let attempt = 1; attempt <= 3; attempt++) {
          OD.pending[canon].try = attempt;
          OD.pending[canon].last = Date.now();
          B('net try ' + attempt + ' ' + canon);
          try {
            await OD.dlNetwork(canon, hit, attempt);
            lastErr = null;
            B('net ok ' + attempt + ' ' + canon);
            break;
          } catch (e) {
            lastErr = e;
            B('net err ' + attempt + ' ' + ((e && e.message) || e) + ' ' + canon);
            odLog('[od] REINTENTO ' + attempt + '/3 ' + canon + ': ' + (e && e.message || e));
            try { FS.unlink(canon + '.part'); } catch (ee) {}
            await OD.sleep(1000 * attempt);
          }
        }
        if (lastErr) throw lastErr;
      } finally {
        delete OD.pending[canon];
        OD.dlRelease();
      }
    })();
    OD.inflight[canon] = job;
    try { await job;     } catch (e) {
      odLog('[od] FAIL ' + canon + ': ' + (e && e.message || e));
      // No dejar parciales: un .part a medias cuenta en el censo y confunde.
      try { FS.unlink(canon + '.part'); } catch (ee) {}
    }
    delete OD.inflight[canon];
    B('job end exists=' + (OD.exists(canon) ? 1 : 0) + ' ' + canon);
    if (OD.exists(canon)) return canon;
    OD.noFetch(reqPath, canon, 'descarga', t0);
    return null;
  },
};

window.__vcRaf = { n: 0 };
// 82.0: hercios reales de la pantalla. El motor los usa como tope de fps
// (glfw.cpp): 60 Hz -> 60, 120 Hz -> 120, 144 Hz -> 144. No hay API estandar,
// asi que se mide con los intervalos de rAF; se descartan los frames ocultos y
// los saltos de pestana (fuera de 4-60 ms no es un frame presentado).
window.__vcHz = 0;
(function vcRafCount() {
  const st = window.__vcRaf;
  const now = (typeof performance !== 'undefined') ? performance.now() : Date.now();
  try {
    if (!document.hidden && st.t) {
      const dt = now - st.t;
      if (dt >= 4 && dt <= 60) {
        st.s = st.s || [];
        st.s.push(dt);
        if (st.s.length > 41) st.s.shift();
        if (st.s.length >= 15) {
          const v = st.s.slice().sort((a, b) => a - b);
          const hz = Math.round(1000 / v[v.length >> 1]);
          if (hz >= 30 && hz <= 240) window.__vcHz = hz;
        }
      }
    }
    st.t = now;
  } catch (e) {}
  st.n++;
  try { requestAnimationFrame(vcRafCount); } catch (e) {}
})();

if (typeof Module !== 'undefined') {
  if (!Module.preRun) Module.preRun = [];
  Module.preRun.push(() => {
    addRunDependency('od-init');
    OD.init().then(() => removeRunDependency('od-init')).catch(() => removeRunDependency('od-init'));
  });
  // Tripwire temporal: registra quién crea/extiende ficheros grandes.
  Module.postRun = Module.postRun || [];
  Module.postRun.push(() => {
    const stk = () => {
      try { return new Error().stack.split('\n').slice(2, 6).join(' <- ').slice(0, 300); }
      catch (e) { return '?'; }
    };
    try {
      const origCreate = FS.createDataFile;
      FS.createDataFile = function (parent, name, data, canRead, canWrite, canOwn) {
        try {
          const sz = data ? data.length : 0;
          if (sz > 50 * 1048576) odLog('[od] BIGCREATE ' + parent + '/' + name + ' ' + Math.round(sz / 1048576) + 'MB :: ' + stk());
        } catch (e) {}
        return origCreate.call(FS, parent, name, data, canRead, canWrite, canOwn);
      };
      const origWrite = FS.write;
      FS.write = function (stream, buffer, offset, length, position) {
        try {
          if (length > 50 * 1048576 || position > 100 * 1048576) {
            odLog('[od] BIGWRITE ' + (stream.path || stream.nodeName || '?') + ' pos=' + position + ' len=' + length + ' :: ' + stk());
          }
        } catch (e) {}
        try {
          const p = stream.path || stream.nodeName || '';
          if (p.indexOf('/userfiles') === 0) OD.noteUserfilesWrite(p);
        } catch (e) {}
        return origWrite.call(FS, stream, buffer, offset, length, position);
      };
      const origTrunc = FS.truncate;
      FS.truncate = function (path, len) {
        try {
          if (len > 100 * 1048576) odLog('[od] BIGTRUNC ' + path + ' ' + len + ' :: ' + stk());
        } catch (e) {}
        return origTrunc.call(FS, path, len);
      };
      const origFtrunc = FS.ftruncate;
      FS.ftruncate = function (fd, len) {
        try {
          if (len > 100 * 1048576) {
            let p = '?';
            try { p = FS.getStreamChecked(fd).path; } catch (e) {}
            odLog('[od] BIGFTRUNC fd=' + fd + ' path=' + p + ' len=' + len + ' :: ' + stk());
          }
        } catch (e) {}
        return origFtrunc.call(FS, fd, len);
      };
      try {
        const paths = ['/models/txd.img'];
        for (const p of paths) {
          try {
            const st = FS.stat(p);
            odLog('[od] ARRANQUE existe ' + p + ' size=' + st.size + ' mode=' + st.mode);
          } catch (e) { odLog('[od] ARRANQUE no existe ' + p); }
        }
      } catch (e) {}
      try {
        if (FS.mmap) {
          const origMmap = FS.mmap;
          FS.mmap = function (stream, length, position, prot, flags) {
            try {
              if (length > 100 * 1048576) odLog('[od] BIGMMAP ' + (stream.path || '?') + ' len=' + length + ' :: ' + stk());
            } catch (e) {}
            return origMmap.call(FS, stream, length, position, prot, flags);
          };
        }
      } catch (e) {}
      // Watchdog F1: vive en el event loop del navegador, así que sigue
      // hablando aunque el loop wasm quede suspendido en un await eterno.
      try {
        setInterval(() => {
          try {
            const now = Date.now();
            for (const k of Object.keys(OD.pending || {})) {
              const p = OD.pending[k];
              const age = Math.round((now - p.t0) / 1000);
              const idle = Math.round((now - p.last) / 1000);
              if (age >= 20) odLog('[od] STALL ' + k + ' edad=' + age + 's inactivo=' + idle + 's bytes=' + p.bytes + ' intento=' + p.try);
            }
          } catch (e) {}
        }, 3000);
      } catch (e) {}
      const origRename = FS.rename;      FS.rename = function (a, b) {
        try {
          let sz = -1;
          try { sz = FS.stat(b).size; } catch (e) {}
          if (sz > 50 * 1048576) odLog('[od] BIGRENAME ' + a + ' -> ' + b + ' dest ' + Math.round(sz / 1048576) + 'MB :: ' + stk());
        } catch (e) {}
        return origRename.call(FS, a, b);
      };
    } catch (e) {}
  });
}
