// Capa on-demand (solo web): fopen/open/stat/access consultan primero a JS.
//
// Con -sASYNCIFY, od_fetch() suspende el wasm mientras se descarga el
// fichero y lo escribe en MEMFS; al reanudar, la apertura real lo encuentra.
// Sin tocar el resto del motor: todo fopen/fread/fseek sigue igual.
// Nativo: fichero inerte (todo tras __EMSCRIPTEN__).
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

// Pide el fichero a JS (descarga + escribe en MEMFS si falta).
// Devuelve ruta canónica (malloc) o NULL.
EM_ASYNC_JS(char*, od_fetch, (const char *cpath), {
  const p = UTF8ToString(cpath);
  try {
    const r = await OD.ensure(p);
    if (!r) return 0;
    const len = lengthBytesUTF8(r) + 1;
    const ptr = _malloc(len);
    stringToUTF8(r, ptr, len);
    return ptr;
  } catch (e) { return 0; }
});

// Contador de "carga bloqueante en curso" (ver ondemand.h). Se publica a JS:
// mientras sea > 0, OD.ensure NO aplaza ficheros, espera a tenerlos.
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

FILE *__real_fopen(const char *path, const char *mode);
int __real_open(const char *path, int flags, ...);
int __real_stat(const char *path, struct stat *buf);
int __real_access(const char *path, int mode);

FILE *__wrap_fopen(const char *path, const char *mode)
{
	if (path) {
		char *rp = od_fetch(path);
		if (rp) {
			FILE *f = __real_fopen(rp, mode);
			free(rp);
			if (f) return f;
			// canonical falló: intentar original por compatibilidad
		}
	}
	FILE *f = __real_fopen(path, mode);
#ifdef __EMSCRIPTEN__
	// Blindaje central (el de AddImage no cubría CFileMgr): un directorio
	// abierto como fichero "funciona" pero todo read falla. Nunca es lo
	// que el motor quiere: cerrar y devolver NULL para que pida bien.
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
		char *rp = od_fetch(path);
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
		char *rp = od_fetch(path);
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
		char *rp = od_fetch(path);
		if (rp) {
			free(rp);
			return 0;
		}
	}
	return __real_access(path, mode);
}

} // extern "C"
#endif // __EMSCRIPTEN__
