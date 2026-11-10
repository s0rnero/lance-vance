export function getFS() {
  if (typeof globalThis.FS !== 'undefined') return globalThis.FS;
  if (globalThis.Module && globalThis.Module.FS) return globalThis.Module.FS;
  return null;
}

export function syncFs(origin) {
  try {
    if (typeof OD !== 'undefined' && OD.syncUserfiles) OD.syncUserfiles(origin);
    else {
      const FS = getFS();
      if (FS) FS.syncfs(false, () => {});
    }
  } catch (e) {}
}

export function saveBlob(blob, filename) {
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  a.remove();
  setTimeout(() => { try { URL.revokeObjectURL(url); } catch (e) {} }, 10000);
}
