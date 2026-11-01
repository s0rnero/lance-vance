// Galeria de fotos de la camara del juego.
//
// El motor guarda en `/userfiles/Gallery/photo_NNNNN.png` (IDBFS, persiste entre
// recargas). Esta capa no dibuja nada dentro del juego: lee esa carpeta y pinta
// una rejilla con miniatura, descarga y borrado. Se abre desde el menu de pausa
// (Other settings -> Gallery) y desde `window.__vcOpenGallery()`.
const DIR = '/userfiles/Gallery';

const TEXT = {
  en: { title: 'Gallery', empty: 'No photos yet. Use the camera in game.', download: 'Download', del: 'Delete', close: 'Close', nofs: 'The game filesystem is not ready yet.' },
  es: { title: 'Galeria', empty: 'Todavia no hay fotos. Usa la camara en el juego.', download: 'Descargar', del: 'Borrar', close: 'Cerrar', nofs: 'El sistema de archivos del juego aun no esta listo.' },
  fr: { title: 'Galerie', empty: "Pas encore de photos. Utilise l'appareil photo en jeu.", download: 'Telecharger', del: 'Supprimer', close: 'Fermer', nofs: "Le systeme de fichiers du jeu n'est pas pret." },
  de: { title: 'Galerie', empty: 'Noch keine Fotos. Benutze die Kamera im Spiel.', download: 'Herunterladen', del: 'Loeschen', close: 'Schliessen', nofs: 'Das Dateisystem des Spiels ist noch nicht bereit.' },
  it: { title: 'Galleria', empty: 'Ancora nessuna foto. Usa la fotocamera nel gioco.', download: 'Scarica', del: 'Elimina', close: 'Chiudi', nofs: 'Il filesystem del gioco non e pronto.' },
  ru: { title: 'Галерея', empty: 'Фотографий пока нет. Используйте камеру в игре.', download: 'Скачать', del: 'Удалить', close: 'Закрыть', nofs: 'Файловая система игры ещё не готова.' },
};

function lang() {
  const raw = (window.__vcLangCode || navigator.language || 'en').toLowerCase().slice(0, 2);
  return TEXT[raw] ? raw : 'en';
}

function getFS() {
  if (typeof globalThis.FS !== 'undefined') return globalThis.FS;
  if (globalThis.Module && globalThis.Module.FS) return globalThis.Module.FS;
  return null;
}

function syncFs() {
  try {
    if (typeof OD !== 'undefined' && OD.syncUserfiles) OD.syncUserfiles('gallery');
    else {
      const FS = getFS();
      if (FS) FS.syncfs(false, () => {});
    }
  } catch (e) {}
}

const CSS = `
.vcg-back{position:fixed;inset:0;z-index:2147483000;background:rgba(4,6,10,.88);display:flex;flex-direction:column;color:#eee;font:14px/1.35 system-ui,Segoe UI,Arial,sans-serif}
.vcg-head{display:flex;align-items:center;justify-content:space-between;padding:14px 18px;border-bottom:1px solid #2a3544}
.vcg-title{font-weight:700;font-size:18px;letter-spacing:.4px}
.vcg-x{background:#233246;color:#fff;border:1px solid #3a4c63;border-radius:6px;padding:7px 14px;cursor:pointer}
.vcg-grid{flex:1;overflow:auto;display:grid;grid-template-columns:repeat(auto-fill,minmax(210px,1fr));gap:14px;padding:18px}
.vcg-card{background:#111823;border:1px solid #263241;border-radius:8px;overflow:hidden;display:flex;flex-direction:column}
.vcg-card img{width:100%;aspect-ratio:16/9;object-fit:cover;background:#000;display:block}
.vcg-name{padding:7px 9px;color:#9fb0c4;font-size:12px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.vcg-acts{display:flex;gap:8px;padding:0 9px 9px}
.vcg-acts button{flex:1;background:#1d2b3c;color:#dbe6f2;border:1px solid #33465c;border-radius:5px;padding:6px 8px;cursor:pointer}
.vcg-acts button.del{background:#3a1d22;border-color:#5c3038;color:#f0c9cf}
.vcg-empty{grid-column:1/-1;text-align:center;color:#9fb0c4;padding:48px 12px}
`;

let overlay = null;
let urls = [];
let built = false;

function ensureCss() {
  if (document.getElementById('vcg-style')) return;
  const st = document.createElement('style');
  st.id = 'vcg-style';
  st.textContent = CSS;
  document.head.appendChild(st);
}

function releaseUrls() {
  for (const u of urls) {
    try { URL.revokeObjectURL(u); } catch (e) {}
  }
  urls = [];
}

function close() {
  releaseUrls();
  if (overlay) overlay.remove();
  overlay = null;
  window.__vcGalleryOpen = false;
}

function make(tag, cls, text) {
  const el = document.createElement(tag);
  if (cls) el.className = cls;
  if (text != null) el.textContent = text;
  return el;
}

function refresh() {
  if (!overlay) return;
  const grid = overlay.querySelector('.vcg-grid');
  const t = TEXT[lang()];
  const FS = getFS();
  releaseUrls();
  grid.textContent = '';

  if (!FS) {
    grid.appendChild(make('div', 'vcg-empty', t.nofs));
    return;
  }

  let files = [];
  try {
    files = FS.readdir(DIR).filter((n) => /^photo_\d+\.png$/.test(n));
    files.sort((a, b) => b.localeCompare(a));
  } catch (e) {
    files = [];
  }

  if (!files.length) {
    grid.appendChild(make('div', 'vcg-empty', t.empty));
    return;
  }

  for (const name of files) {
    const path = DIR + '/' + name;
    const card = make('div', 'vcg-card');
    let data;
    try { data = FS.readFile(path); } catch (e) { continue; }
    const url = URL.createObjectURL(new Blob([data], { type: 'image/png' }));
    urls.push(url);

    const img = document.createElement('img');
    img.src = url;
    img.alt = name;
    card.appendChild(img);
    card.appendChild(make('div', 'vcg-name', name));

    const acts = make('div', 'vcg-acts');
    const dl = make('button', '', t.download);
    dl.onclick = () => {
      const a = document.createElement('a');
      a.href = url;
      a.download = name;
      document.body.appendChild(a);
      a.click();
      a.remove();
    };
    const del = make('button', 'del', t.del);
    del.onclick = () => {
      try {
        FS.unlink(path);
        syncFs();
      } catch (e) {}
      refresh();
    };
    acts.appendChild(dl);
    acts.appendChild(del);
    card.appendChild(acts);
    grid.appendChild(card);
  }
}

export function openGallery() {
  ensureCss();
  if (!overlay) {
    const t = TEXT[lang()];
    overlay = make('div', 'vcg-back');
    overlay.setAttribute('role', 'dialog');
    overlay.setAttribute('aria-label', t.title);

    const head = make('div', 'vcg-head');
    head.appendChild(make('div', 'vcg-title', t.title));
    const x = make('button', 'vcg-x', t.close);
    x.onclick = close;
    head.appendChild(x);

    overlay.appendChild(head);
    overlay.appendChild(make('div', 'vcg-grid'));
    overlay.addEventListener('click', (e) => { if (e.target === overlay) close(); });
    document.body.appendChild(overlay);
  }
  window.__vcGalleryOpen = true;
  try { if (document.exitPointerLock) document.exitPointerLock(); } catch (e) {}
  refresh();
  return true;
}

export function closeGallery() {
  close();
}

function boot() {
  // No pisar una implementacion del host.
  if (!window.__vcOpenGallery) window.__vcOpenGallery = openGallery;
  window.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && window.__vcGalleryOpen) {
      e.preventDefault();
      close();
    }
  }, true);
}

if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', boot, { once: true });
else boot();
