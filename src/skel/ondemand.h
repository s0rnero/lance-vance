#pragma once
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define ODTRACEI(tag, v) EM_ASM({ try { (window.__odq = window.__odq || []).push(UTF8ToString($0) + $1); } catch (e) {} }, tag, v)
#define ODTRACES(msg) EM_ASM({ try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {} }, msg)

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
