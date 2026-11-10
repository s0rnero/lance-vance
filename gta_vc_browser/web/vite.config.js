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
    // Tuneles de Cloudflare (prueba en el movil): `cloudflared tunnel --url
    // http://localhost:2077` asigna un hostname *-trycloudflare.com distinto en
    // cada arranque, y Vite 6 responde solo a los hosts autorizados. Con el
    // sufijo valen todos los que salgan del tuneo; las IPs de red local ya estan
    // permitidas por defecto, asi que `npm run dev -- --host` sigue sirviendo.
    allowedHosts: ['.trycloudflare.com'],
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
    allowedHosts: ['.trycloudflare.com'],
    headers: {
      'Cross-Origin-Opener-Policy': 'same-origin',
      'Cross-Origin-Embedder-Policy': 'require-corp',
    },
  },
  publicDir: 'public',
});
