// Traza de depuración web: encola en window.__odq; la librería (web/lib/index.js)
// lo envía por lotes a OD.cfg.odtraceUrl (si el host la define).
// por lotes cada 1s (async). Nada de XHR síncrono en el hilo del juego (jank).
// TEMPORAL para diagnóstico.
#pragma once
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define ODTRACEI(tag, v) EM_ASM({ try { (window.__odq = window.__odq || []).push(UTF8ToString($0) + $1); } catch (e) {} }, tag, v)
#define ODTRACES(msg) EM_ASM({ try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} }, msg)

// ---------------------------------------------------------------------------
// "Carga bloqueante en curso" (web).
//
// La capa on-demand puede APLAZAR un fichero que aún no está (contesta "no
// está" y el motor lo reintenta en otro frame). Eso es correcto en partida
// libre, pero NO cuando el motor está bloqueado pidiendo ficheros aquí y ahora:
// una escena de cinemática, un preload de audio o un LoadAllRequestedModels se
// quedan sin el fichero y el resultado se ve como escena vacía (mundo negro,
// sin audio). Con esto el motor avisa a la página de que ahora no se aplaza
// nada mientras dura la llamada.
// ---------------------------------------------------------------------------
extern "C" {
void odBlockingPush(void);
void odBlockingPop(void);
}
#else
#define ODTRACEI(tag, v) ((void)0)
#define ODTRACES(msg) ((void)0)
#define odBlockingPush() ((void)0)
#define odBlockingPop() ((void)0)
#endif
