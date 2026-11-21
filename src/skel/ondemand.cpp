#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// 82.1 · Perfil por muestreo del ProcessControl de coches
//
// El bucle de vehiculos se lleva ~4 ms de `Logic.world` (traza 81.9b) pero no
// se sabia QUE rama: fisica, IA, dano, colision, ruedas o audio. Cronometrar
// cada tramo de cada coche en todos los frames costaria mas que el propio
// tramo (cada `emscripten_get_now` cruza a JS), asi que se muestrea UN frame de
// cada 240 (~4 s a 60 Hz) y se suman los tramos de todos los coches de ese
// frame. La linea `VEHPROF` la emite glfw.cpp junto al WEBFT; el coste medio
// son ~30 llamadas a JS por frame.
// ---------------------------------------------------------------------------
bool gWebVehProfOn;
bool gWebVehProfDone;
int gWebVehProfN;
double gWebVehProfT[9];
static double odVehProfLast;
static int odVehProfTick;

void WebVehProfFrame(void)
{
	if (gWebVehProfOn) {
		gWebVehProfOn = false;
		gWebVehProfDone = true;
	}
	if (++odVehProfTick >= 240) {
		odVehProfTick = 0;
		gWebVehProfN = 0;
		for (int i = 0; i < 9; i++)
			gWebVehProfT[i] = 0.0;
		gWebVehProfOn = true;
	}
}

void WebVehProfMark(int slot)
{
	if (!gWebVehProfOn)
		return;
	double now = emscripten_get_now();
	if (slot == 0) {
		odVehProfLast = now;
		gWebVehProfN++;
		return;
	}
	if (slot > 0 && slot < 9)
		gWebVehProfT[slot] += now - odVehProfLast;
	odVehProfLast = now;
}

EM_ASYNC_JS(char*, od_fetch, (const char *cpath), {
  const p = UTF8ToString(cpath);
  try {
    // T18: si el motor se queda colgado aquí, la sonda del jugador tiene que
    // decir QUÉ fichero estaba esperando (no adivinarlo).
    try { window.__vcFetchNow = p; } catch (e) {}
    const dbgF = (typeof OD_PROBE !== 'undefined') && OD_PROBE.test(p);
    if (dbgF) { try { odProbe('fetch enter ' + p); } catch (e) {} }
    const r = await OD.ensure(p);
    if (dbgF) { try { odProbe('fetch got ' + p + ' -> ' + (r || 'nil')); } catch (e) {} }
    try { window.__vcFetchNow = null; } catch (e) {}
    if (!r) return 0;
    const len = lengthBytesUTF8(r) + 1;
    const ptr = _malloc(len);
    stringToUTF8(r, ptr, len);
    if (dbgF) { try { odProbe('fetch ret ' + p); } catch (e) {} }
    return ptr;
  } catch (e) { return 0; }
});

EM_JS(char*, od_present, (const char *cpath), {
  const p = UTF8ToString(cpath);
  try {
    const r = OD.presentSync(p);
    if (!r) return 0;
    const len = lengthBytesUTF8(r) + 1;
    const ptr = _malloc(len);
    stringToUTF8(r, ptr, len);
    return ptr;
  } catch (e) { return 0; }
});

EM_JS(int, od_sfx_ready, (const char *cpath), {
  const p = UTF8ToString(cpath);
  try {
    if (OD.presentSync(p)) return 1;
    OD._ensure(p).catch(function(){});
    return 0;
  } catch (e) { return 0; }
});

static int odBlockingDepth = 0;
extern "C" void odBlockingPush(void)
{
	odBlockingDepth++;
	EM_ASM({ try { window.__vcODBlock = ((window.__vcODBlock | 0) + 1); } catch (e) {} });
}
extern "C" void odBlockingPop(void)
{
	if (odBlockingDepth > 0)
		odBlockingDepth--;
	EM_ASM({ try { window.__vcODBlock = Math.max(0, ((window.__vcODBlock | 0) - 1)); } catch (e) {} });
}

extern "C" {

void OdBeacon(const char *tag)
{
	EM_ASM({
		try {
			var u = (typeof OD !== 'undefined' && OD.cfg && OD.cfg.odtraceUrl) || null;
			if (u && navigator.sendBeacon) navigator.sendBeacon(u, 'CPUBEACON ' + UTF8ToString($0));
		} catch (e) {}
	}, tag);
}

FILE *__real_fopen(const char *path, const char *mode);
int __real_open(const char *path, int flags, ...);
int __real_stat(const char *path, struct stat *buf);
int __real_access(const char *path, int mode);

// 83.1: baliza desde C para bisecar el wrapper (que es donde se pierde el
// hilo cuando el motor no vuelve de una suspension).
static void odBW(const char *tag, const char *path, int val)
{
	EM_ASM({
		try {
			if (typeof odProbe === 'function')
				odProbe('WRAP ' + UTF8ToString($0) + ' ' + UTF8ToString($1) + ' v=' + $2);
		} catch (e) {}
	}, tag, path ? path : "-", val);
}

FILE *__wrap_fopen(const char *path, const char *mode)
{
	if (path) {
		char *rp = od_present(path);
		odBW("fopen-pres", path, rp ? 1 : 0);
		if (!rp) rp = od_fetch(path);
		odBW("fopen-rp", path, rp ? 1 : 0);
		if (rp) {
			FILE *f = __real_fopen(rp, mode);
			odBW("fopen-real", path, f ? 1 : 0);
			free(rp);
			if (f) return f;
		}
	}
	FILE *f = __real_fopen(path, mode);
#ifdef __EMSCRIPTEN__
	if (f) {
		struct stat st;
		if (fstat(fileno(f), &st) == 0 && S_ISDIR(st.st_mode)) {
			fclose(f);
			errno = EISDIR;
			return NULL;
		}
	}
#endif
	return f;
}

int __wrap_open(const char *path, int flags, ...)
{
	mode_t m = 0;
	if (flags & O_CREAT) {
		va_list ap;
		va_start(ap, flags);
		m = (mode_t)va_arg(ap, int);
		va_end(ap);
	}
	if (path) {
		char *rp = od_present(path);
		if (!rp) rp = od_fetch(path);
		odBW("open-rp", path, rp ? 1 : 0);
		if (rp) {
			int fd = __real_open(rp, flags, m);
			free(rp);
			if (fd >= 0) return fd;
		}
	}
	return __real_open(path, flags, m);
}

int __wrap_stat(const char *path, struct stat *buf)
{
	if (path) {
		char *rp = od_present(path);
		if (!rp) rp = od_fetch(path);
		if (rp) {
			int r = __real_stat(rp, buf);
			free(rp);
			if (r == 0) return 0;
		}
	}
	return __real_stat(path, buf);
}

int __wrap_access(const char *path, int mode)
{
	if (path) {
		char *rp = od_present(path);
		if (!rp) rp = od_fetch(path);
		odBW("access-rp", path, rp ? 1 : 0);
		if (rp) {
			free(rp);
			return 0;
		}
	}
	return __real_access(path, mode);
}

} // extern "C"
#endif // __EMSCRIPTEN__
