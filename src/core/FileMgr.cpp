#define _CRT_SECURE_NO_WARNINGS
#include <fcntl.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include "common.h"
#include "crossplatform.h"

#include "FileMgr.h"

const char *_psGetUserFilesFolder();

#ifdef __EMSCRIPTEN__
// 83.1: baliza inmediata (sendBeacon) para saber si se pasa por aqui aunque
// lo impreso se quede en el buffer de stdout al morir el hilo del motor.
extern "C" void OdBeacon(const char *tag);
#endif

/*
 * Windows FILE is BROKEN for GTA.
 *
 * We need to support mapping between LF and CRLF for text files
 * but we do NOT want to end the file at the first sight of a SUB character.
 * So here is a simple implementation of a FILE interface that works like GTA expects.
 */

struct myFILE
{
	bool isText;
	FILE *file;
};

#define NUMFILES 20
static myFILE myfiles[NUMFILES];


#if !defined(_WIN32)
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#define _getcwd getcwd

// Case-insensitivity on linux (from https://github.com/OneSadCookie/fcaseopen)
void mychdir(char const *path)
{
	char* r = casepath(path, false);
    if (r) {
        chdir(r);
		free(r);
    } else {
        errno = ENOENT;
    }
}
#else
#define mychdir chdir
#endif

/* Force file to open as binary but remember if it was text mode */
static int
myfopen(const char *filename, const char *mode)
{
	int fd;
	char realmode[10], *p;

	for(fd = 1; fd < NUMFILES; fd++)
		if(myfiles[fd].file == nil)
			goto found;
	printf("OPENFAIL %s: sin descriptor libre (NUMFILES=%d)\n", filename, (int)NUMFILES);
	return 0;	// no free fd
found:
	myfiles[fd].isText = strchr(mode, 'b') == nil;
	p = realmode;
	while(*mode)
		if(*mode != 't' && *mode != 'b')
			*p++ = *mode++;
		else
			mode++;
	*p++ = 'b';
	*p = '\0';
	
	// El motor pide rutas de Windows ("DATA\\DEFAULT.DAT"). En MEMFS la barra
	// invertida no separa carpetas: fcaseopen no la encuentra y devuelve nil SIN
	// llegar a la capa on-demand (por eso no salia ni ODMISS). Se normaliza.
	char norm[512];
	{
		// Se recortan \r, \n, espacios y tabuladores del final: las lineas de los
		// ficheros de texto llegan con CRLF y el nombre se quedaba con el \r pegado
		// ("DATA/DEFAULT.IDE\r" -> no existe).
		size_t fl = strlen(filename);
		size_t i;
		int ni = 0;
		while(fl > 0 && (filename[fl-1] == '\r' || filename[fl-1] == '\n' ||
		                 filename[fl-1] == ' '  || filename[fl-1] == '\t'))
			fl--;
		for(i = 0; i < fl && ni < (int)sizeof(norm) - 1; i++)
			norm[ni++] = (filename[i] == '\\') ? '/' : filename[i];
		norm[ni] = '\0';
	}
	myfiles[fd].file = fcaseopen(norm, realmode);
	if(myfiles[fd].file == nil){
		printf("OPENFAIL %s: no se encuentra\n", norm);
		return 0;
	}
	return fd;
}

static int
myfclose(int fd)
{
	int ret;
	assert(fd < NUMFILES);
	if(myfiles[fd].file){
		ret = fclose(myfiles[fd].file);
		myfiles[fd].file = nil;
		return ret;
	}
	return EOF;
}

static int
myfgetc(int fd)
{
	int c;
	c = fgetc(myfiles[fd].file);
	if(myfiles[fd].isText && c == 015){
		/* translate CRLF to LF */
		c = fgetc(myfiles[fd].file);
		if(c == 012)
			return c;
		ungetc(c, myfiles[fd].file);
		return 015;
	}
	return c;
}

static int
myfputc(int c, int fd)
{
	/* translate LF to CRLF */
	if(myfiles[fd].isText && c == 012)
		fputc(015, myfiles[fd].file);
	return fputc(c, myfiles[fd].file);
}

static char*
myfgets(char *buf, int len, int fd)
{
	int c;
	char *p;

	p = buf;
	len--;	// NUL byte
	while(len--){
		c = myfgetc(fd);
		if(c == EOF){
			if(p == buf)
				return nil;
			break;
		}
		// CRLF: el \r se descarta, igual que hacia Windows en modo texto.
		// Los datos nuevos vienen con \r\n y el motor esperaba solo \n: el \r
		// se pegaba al nombre del fichero ("DEFAULT.IDE\r") y no se encontraba.
		if(c == '\r')
			continue;
		*p++ = c;
		if(c == '\n')
			break;
	}
	*p = '\0';
	return buf;
}

static size_t
myfread(void *buf, size_t elt, size_t n, int fd)
{
	if(myfiles[fd].isText){
		unsigned char *p;
		size_t i;
		int c;

		n *= elt;
		p = (unsigned char*)buf;
		for(i = 0; i < n; i++){
			c = myfgetc(fd);
			if(c == EOF)
				break;
			*p++ = (unsigned char)c;
		}
		return i / elt;
	}
	return fread(buf, elt, n, myfiles[fd].file);
}

static size_t
myfwrite(void *buf, size_t elt, size_t n, int fd)
{
	if(myfiles[fd].isText){
		unsigned char *p;
		size_t i;
		int c;

		n *= elt;
		p = (unsigned char*)buf;
		for(i = 0; i < n; i++){
			c = *p++;
			myfputc(c, fd);
			if(feof(myfiles[fd].file))	// is this right?
				break;
		}
		return i / elt;
	}
	return fwrite(buf, elt, n, myfiles[fd].file);
}

static int
myfseek(int fd, long offset, int whence)
{
	return fseek(myfiles[fd].file, offset, whence);
}

static int
myfeof(int fd)
{
	return feof(myfiles[fd].file);
//	return ferror(myfiles[fd].file);
}


char CFileMgr::ms_rootDirName[128] = {'\0'};
char CFileMgr::ms_dirName[128];

void
CFileMgr::Initialise(void)
{
	_getcwd(ms_rootDirName, 128);
	strcat(ms_rootDirName, "\\");
}

void
CFileMgr::ChangeDir(const char *dir)
{
	if(*dir == '\\'){
		strcpy(ms_dirName, ms_rootDirName);
		dir++;
	}
	if(*dir != '\0'){
		strcat(ms_dirName, dir);
		// BUG in the game it seems, it's off by one
		if(dir[strlen(dir)-1] != '\\')
			strcat(ms_dirName, "\\");
	}
	mychdir(ms_dirName);
}

void
CFileMgr::SetDir(const char *dir)
{
	strcpy(ms_dirName, ms_rootDirName);
	if(*dir != '\0'){
		strcat(ms_dirName, dir);
		// BUG in the game it seems, it's off by one
		if(dir[strlen(dir)-1] != '\\')
			strcat(ms_dirName, "\\");
	}
	mychdir(ms_dirName);
}

void
CFileMgr::SetDirMyDocuments(void)
{
	SetDir("");	// better start at the root if user directory is relative
	mychdir(_psGetUserFilesFolder());
}

ssize_t
CFileMgr::LoadFile(const char *file, uint8 *buf, int maxlen, const char *mode)
{
	int fd;
	ssize_t n, len;

	printf("LF open %s\nn", file);
	fd = myfopen(file, mode);
	printf("LF fd=%d %s\nn", fd, file);
#ifdef __EMSCRIPTEN__
	{ char ob[192]; snprintf(ob, sizeof(ob), "LF fd=%d %s", fd, file); OdBeacon(ob); }
#endif
	if(fd == 0)
		return -1;
	len = 0;
	for(;;){
		n = myfread(buf + len, 1, 0x4000, fd);
		printf("LF read n=%d len=%d\nn", (int)n, (int)len);
#ifndef FIX_BUGS
		if (n < 0)
			return -1;
#endif
		if(n == 0)
			break;
		len += n;
		// T23: el bucle original no tenia tope. Si el lector no marca EOF y
		// devuelve siempre 0x4000, esto se colgaba (PARTICLE.CFG en el paso 3).
		assert(len < maxlen);
		if(len + 0x4000 >= maxlen)
			break;
	}
#ifdef __EMSCRIPTEN__
	{ char ob[192]; snprintf(ob, sizeof(ob), "LF done len=%d %s", (int)len, file); OdBeacon(ob); }
#endif
	buf[len] = 0;
	myfclose(fd);
	return len;
}

int
CFileMgr::OpenFile(const char *file, const char *mode)
{
	return myfopen(file, mode);
}

int
CFileMgr::OpenFileForWriting(const char *file)
{
	return OpenFile(file, "wb");
}

size_t
CFileMgr::Read(int fd, const char *buf, ssize_t len)
{
	return myfread((void*)buf, 1, len, fd);
}

size_t
CFileMgr::Write(int fd, const char *buf, ssize_t len)
{
	return myfwrite((void*)buf, 1, len, fd);
}

bool
CFileMgr::Seek(int fd, int offset, int whence)
{
	return !!myfseek(fd, offset, whence);
}

bool
CFileMgr::ReadLine(int fd, char *buf, int len)
{
	return myfgets(buf, len, fd) != nil;
}

int
CFileMgr::CloseFile(int fd)
{
	return myfclose(fd);
}

int
CFileMgr::GetErrorReadWrite(int fd)
{
	return myfeof(fd);
}
