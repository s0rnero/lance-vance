#pragma once
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define ODTRACEI(tag, v) EM_ASM({ try { (window.__odq = window.__odq || []).push(UTF8ToString($0) + $1); } catch (e) {} }, tag, v)
#define ODTRACES(msg) EM_ASM({ try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} }, msg)

extern "C" {
void odBlockingPush(void);
void odBlockingPop(void);
void OdBeacon(const char *tag);
}

// 82.1 · Perfil por muestreo del bucle de coches (definido en ondemand.cpp).
// Se llama una vez por frame (CGame::Process) y desde los tramos de
// CAutomobile::ProcessControl. Fuera de Emscripten no existe.
void WebVehProfFrame(void);
void WebVehProfMark(int slot);
#else
#define ODTRACEI(tag, v) ((void)0)
#define ODTRACES(msg) ((void)0)
#define odBlockingPush() ((void)0)
#define odBlockingPop() ((void)0)
#define WebVehProfFrame() ((void)0)
#define WebVehProfMark(slot) ((void)0)
#define OdBeacon(tag) ((void)0)
#endif
