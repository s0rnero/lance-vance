import { getFS, syncFs, saveBlob } from './fsx.js';
import { makeZip } from './zip.js';

const DIR = '/userfiles/Gallery';

function photos() {
  const FS = getFS();
  if (!FS) return [];
  try {
    return FS.readdir(DIR).filter((n) => /^photo_\d+\.png$/.test(n)).sort((a, b) => b.localeCompare(a));
  } catch (e) {
    return [];
  }
}

export async function downloadAllGallery() {
  const FS = getFS();
  if (!FS) return 0;
  const files = photos();
  if (!files.length) return 0;
  const entries = [];
  for (const name of files) {
    try { entries.push({ name, data: new Uint8Array(FS.readFile(DIR + '/' + name)) }); } catch (e) {}
  }
  if (!entries.length) return 0;
  saveBlob(await makeZip(entries), 'galeria-vice-city.zip');
  return entries.length;
}

export function clearGallery() {
  const FS = getFS();
  if (!FS) return 0;
  let n = 0;
  for (const name of photos()) {
    try { FS.unlink(DIR + '/' + name); n++; } catch (e) {}
  }
  if (n) syncFs('gallery');
  return n;
}

if (typeof window !== 'undefined') {
  if (!window.__vcGalleryDownload) window.__vcGalleryDownload = downloadAllGallery;
  if (!window.__vcGalleryClear) window.__vcGalleryClear = clearGallery;
}
