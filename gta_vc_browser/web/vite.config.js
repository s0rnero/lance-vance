import { defineConfig } from 'vite';
import { vcWeb } from './lib/vite.js';

// Sirve el juego en http://localhost:2077 con la página de desarrollo.
// Los middlewares (streamed/, vc/, odtrace, estado) y las cabeceras COOP/COEP
// viven en lib/vite.js, el mismo plugin que usaría un proyecto Vue:
//   plugins: [vcWeb({ streamedDir: '.../streamed', assetsDir: '.../assets' })]
export default defineConfig({
  plugins: [
    vcWeb({
      logs: true,              // streamed-404.log / streamed-access.log (depuración)
      traceFile: 'odtrace.log',
    }),
  ],
  server: {
    port: 2077,
    strictPort: true,
    headers: {
      // Obligatorias para SharedArrayBuffer (pthreads del motor).
      'Cross-Origin-Opener-Policy': 'same-origin',
      'Cross-Origin-Embedder-Policy': 'require-corp',
      'Cache-Control': 'no-store',
    },
    watch: {
      ignored: ['**/public/vc/**', '**/public/build/**'],
    },
    fs: {
      allow: ['..'],
    },
  },
  preview: {
    port: 2077,
    strictPort: true,
    headers: {
      'Cross-Origin-Opener-Policy': 'same-origin',
      'Cross-Origin-Embedder-Policy': 'require-corp',
    },
  },
  publicDir: 'public',
});
