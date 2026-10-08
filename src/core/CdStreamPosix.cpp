#ifndef _WIN32
#include "common.h"
#include "crossplatform.h"
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/resource.h>
#include <stdarg.h>
#include <limits.h>

#ifdef __linux__
#include <sys/syscall.h>
#endif

#include "CdStream.h"
#include "rwcore.h"
#include "MemoryMgr.h"
#include "ondemand.h"

#define CDDEBUG(f, ...)   debug ("%s: " f "\n", "cdvd_stream", ## __VA_ARGS__)
#define CDTRACE(f, ...)   printf("%s: " f "\n", "cdvd_stream", ## __VA_ARGS__)

#ifdef FLUSHABLE_STREAMING
bool flushStream[MAX_CDCHANNELS];
#endif

#ifdef USE_UNNAMED_SEM

#define RE3_SEM_OPEN(name, ...) re3_sem_open()
sem_t*
re3_sem_open(void)
{
	sem_t* sem = (sem_t*)malloc(sizeof(sem_t));
	if (sem_init(sem, 0, 1) == -1) {
		sem = SEM_FAILED;
	}

	return sem;
}

#define RE3_SEM_CLOSE(sem, format, ...) re3_sem_close(sem)
void
re3_sem_close(sem_t* sem)
{
	sem_destroy(sem);
	free(sem);
}

#else

#define RE3_SEM_OPEN re3_sem_open
sem_t*
re3_sem_open(const char* format, ...)
{
	char semName[21];
	va_list va;
	va_start(va, format);
	vsprintf(semName, format, va);

	return sem_open(semName, O_CREAT, 0644, 1);
}

#define RE3_SEM_CLOSE re3_sem_close
void
re3_sem_close(sem_t* sem, const char* format, ...)
{
	sem_close(sem);

	char semName[21];
	va_list va;
	va_start(va, format);
	vsprintf(semName, format, va);

	sem_unlink(semName);
}

#endif

struct CdReadInfo
{
	uint32 nSectorOffset;
	uint32 nSectorsToRead;
	void *pBuffer;
	bool bLocked;
	bool bReading;
	int32 nStatus;
#ifdef ONE_THREAD_PER_CHANNEL
	int8 nThreadStatus; // 0: created 1:priority set up 2:abort now
	pthread_t pChannelThread;
	sem_t *pStartSemaphore;
#endif
	sem_t *pDoneSemaphore; // used for CdStreamSync
	int32 hFile;
};

char gCdImageNames[MAX_CDIMAGES+1][64];
int32 gNumImages;
int32 gNumChannels;

int32 gImgFiles[MAX_CDIMAGES]; // -1: error 0:unused otherwise: fd
char *gImgNames[MAX_CDIMAGES];

#ifdef __EMSCRIPTEN__
// ---- web on-demand: lectura sincrona + imagenes como directorios sueltos.
// Sin hilos (como dos.zone): CdStreamRead lee en linea (el fetch async lo
// cubre __wrap_open/__wrap_fopen via Asyncify) y CdStreamSync es inmediato.
// Si "<path>.img" no existe como fichero pero sí "<path>" como directorio
// (p. ej. models/gta3.img/ con miles de .dff/.txd sueltos), se registra
// backend suelto usando el .dir para mapear sector->fichero.
struct OdDirEntry { uint32 start; uint32 sectors; char name[32]; };
struct OdLoose { char base[128]; OdDirEntry *entries; int nentries; uint32 totalSectors; int lastHit; };
static OdLoose gOdLoose[MAX_CDIMAGES];
static bool gOdIsLoose[MAX_CDIMAGES];

// D4 (sección 1): fallos de apertura de un fichero suelto del .img.
//
// Un fallo aquí NO significa "fichero que no existe": significa que la capa
// on-demand todavía no lo tiene en MEMFS y ha aplazado la descarga a propósito
// (para no congelar el frame; el motor reintenta en otro fotograma). Verificado
// contra el manifiesto: los 60 fallos de una partida tenían su entrada.
//
// Lo que faltaba es poder AFIRMAR que el fichero volvió. Aquí se recuerdan los
// últimos fallos y, cuando alguno se abre bien, deja `ODSRECOVER <ruta>
// fallos=<n>`: si todos los fallos acaban recuperados, el aplazamiento es
// inocuo; un fallo sin recuperación es un modelo que se quedó sin cargar.
#define OD_FAIL_RING 16
static struct { char path[192]; int fallos; } gOdFails[OD_FAIL_RING];
static int gOdFailCount;    // entradas ocupadas
static int gOdFailNext;     // índice a reescribir cuando el anillo se llena
static int gOdFailTotal;    // fallos totales (el recorte de líneas no lo oculta)
static int gOdFailShown;    // líneas `ODSHORT` emitidas
static int gOdRecoverShown; // líneas `ODSRECOVER` emitidas

// Casilla del anillo para esa ruta (la crea si no estaba).
static int
OdFailSlot(const char *path)
{
	int i;
	for (i = 0; i < gOdFailCount; i++)
		if (strcmp(gOdFails[i].path, path) == 0)
			return i;
	if (gOdFailCount < OD_FAIL_RING)
		i = gOdFailCount++;
	else {
		i = gOdFailNext;
		gOdFailNext = (gOdFailNext + 1) % OD_FAIL_RING;
	}
	strncpy(gOdFails[i].path, path, sizeof(gOdFails[i].path) - 1);
	gOdFails[i].path[sizeof(gOdFails[i].path) - 1] = '\0';
	gOdFails[i].fallos = 0;
	return i;
}

// ¿Esta ruta había fallado antes? Si sí, se anota la recuperación y se saca del
// anillo (un fichero que ya está en MEMFS no vuelve a fallar).
static void
OdNoteOpened(const char *path)
{
	int i;
	for (i = 0; i < gOdFailCount; i++) {
		if (strcmp(gOdFails[i].path, path) == 0) {
			if (gOdRecoverShown < 12) {
				gOdRecoverShown++;
				char t[256];
				snprintf(t, sizeof t, "ODSRECOVER %s fallos=%d", path, gOdFails[i].fallos);
				ODTRACES(t);
			}
			gOdFails[i] = gOdFails[--gOdFailCount];
			if (gOdFailNext >= gOdFailCount)
				gOdFailNext = 0;
			return;
		}
	}
}

static void
OdNoteFailed(const char *path)
{
	int slot = OdFailSlot(path);
	gOdFails[slot].fallos++;
	gOdFailTotal++;
	// Los primeros van uno a uno (interesa leerlos de corrido); después sólo
	// cada 20, para que la última línea traiga el total real.
	if (gOdFailShown < 60 || gOdFailTotal % 20 == 0) {
		gOdFailShown++;
		char t[256];
		snprintf(t, sizeof t, "ODSHORT open-fail %s errno=%d fallo=%d total=%d", path, errno, gOdFails[slot].fallos, gOdFailTotal);
		ODTRACES(t);
		printf("[od-trace] %s\n", t);
	}
}

static int32
OdDoRead(int img, void *buffer, uint32 sectorOffset, uint32 nSectors)
{
	uint8 *dst = (uint8*)buffer;
	uint32 want = nSectors * CDSTREAM_SECTOR_SIZE;
	if (!gOdIsLoose[img]) {
		int fd = gImgFiles[img] - 1;
		lseek(fd, (size_t)sectorOffset * (size_t)CDSTREAM_SECTOR_SIZE, SEEK_SET);
		uint32 rd = 0;
		while (rd < want) {
			int r = read(fd, dst + rd, want - rd);
			if (r <= 0) break;
			rd += r;
		}
		return rd < want ? STREAM_ERROR : STREAM_NONE;
	}
	OdLoose *L = &gOdLoose[img];
	// Un .img real es continuo por sectores: una petición puede cubrir hasta
	// 4 ficheros adyacentes (Streaming.cpp agrupa). Servirlos en cadena como
	// el archivo original; solo los huecos sin entrada van a ceros.
	uint32 sec = sectorOffset;
	uint32 remain = nSectors;
	while (remain > 0) {
		int hit = -1;
		if (L->lastHit >= 0 && L->lastHit < L->nentries) {
			OdDirEntry *ce = &L->entries[L->lastHit];
			if (ce->start <= sec && sec < ce->start + ce->sectors)
				hit = L->lastHit;
		}
		if (hit < 0) {
			for (int i = 0; i < L->nentries; i++) {
				OdDirEntry *ce = &L->entries[i];
				if (ce->start <= sec && sec < ce->start + ce->sectors) { hit = i; break; }
			}
		}
		if (hit < 0) {
			if (sec == sectorOffset) {
				static int odErrCount = 0;
				if (odErrCount < 20) { odErrCount++; printf("[od-trace] loose MISS sector %u\n", sec); ODTRACEI("loosemiss ", (int)sec); }
				return STREAM_ERROR;
			}
			// Hueco sin entrada a mitad de tramo: ceros (como el slack del .img).
			memset(dst, 0, remain * CDSTREAM_SECTOR_SIZE);
			break;
		}
		L->lastHit = hit;
		OdDirEntry *e = &L->entries[hit];
		uint32 takeSec = e->start + e->sectors - sec;
		if (takeSec > remain) takeSec = remain;
		char fp[192];
		snprintf(fp, sizeof(fp), "%s/%s", L->base, e->name);
		int fd = open(fp, O_RDONLY);
		if (fd < 0) {
#ifdef __EMSCRIPTEN__
			OdNoteFailed(fp);
#endif
			return STREAM_ERROR;
		}
#ifdef __EMSCRIPTEN__
		OdNoteOpened(fp);
#endif
		struct stat st;
		uint32 fsize = (fstat(fd, &st) == 0) ? (uint32)st.st_size : 0;
		uint32 fileOff = (sec - e->start) * CDSTREAM_SECTOR_SIZE;
		uint32 wantSeg = takeSec * CDSTREAM_SECTOR_SIZE;
		uint32 avail = fileOff < fsize ? fsize - fileOff : 0;
		uint32 got = avail > wantSeg ? wantSeg : avail;
		lseek(fd, fileOff, SEEK_SET);
		uint32 rd = 0;
		while (rd < got) {
			int r = read(fd, dst + rd, got - rd);
			if (r <= 0) break;
			rd += r;
		}
		close(fd);
		if (rd < wantSeg) {
			// Fichero más corto de lo que dice su entrada: ceros en su tramo
			// y se sigue con la entrada siguiente (no se aborta el tramo).
#ifdef __EMSCRIPTEN__
			{
				static int n = 0;
				if (n < 60) {
					n++;
					char t[256];
					snprintf(t, sizeof t, "ODSHORT short %s fileOff=%u fsize=%u got=%u wantSeg=%u rd=%u entsec=%u",
						fp, fileOff, fsize, got, wantSeg, rd, e->sectors);
					ODTRACES(t);
				}
			}
#endif
			memset(dst + rd, 0, wantSeg - rd);
		}
		dst += wantSeg;
		sec += takeSec;
		remain -= takeSec;
	}
	return STREAM_NONE;
}
#endif // __EMSCRIPTEN__

#ifndef ONE_THREAD_PER_CHANNEL
pthread_t _gCdStreamThread;
sem_t *gCdStreamSema; // released when we have new thing to read(so channel is set)
int8 gCdStreamThreadStatus; // 0: created 1:priority set up 2:abort now
Queue gChannelRequestQ;
bool _gbCdStreamOverlapped;
#endif

CdReadInfo *gpReadInfo;

int32 lastPosnRead;

int _gdwCdStreamFlags;

void *CdStreamThread(void* channelId);

void
CdStreamInitThread(void)
{
#ifdef __EMSCRIPTEN__
	// Web monohilo: sin cola, sin semaforos, sin hilos. CdStreamRead lee en
	// linea (ver rama sync) y CdStreamSync es inmediato.
	return;
#else
	int status;
#ifndef ONE_THREAD_PER_CHANNEL
	gChannelRequestQ.items = (int32 *)calloc(gNumChannels + 1, sizeof(int32));
	gChannelRequestQ.head = 0;
	gChannelRequestQ.tail = 0;
	gChannelRequestQ.size = gNumChannels + 1;
	ASSERT(gChannelRequestQ.items != nil );
	gCdStreamSema = RE3_SEM_OPEN("/semaphore_cd_stream");


	if (gCdStreamSema == SEM_FAILED) {
		CDTRACE("failed to create stream semaphore");
		ASSERT(0);
		return;
	}
#endif

	if ( gNumChannels > 0 )
	{
		for ( int32 i = 0; i < gNumChannels; i++ )
		{
			gpReadInfo[i].pDoneSemaphore = RE3_SEM_OPEN("/semaphore_done%d", i);

			if (gpReadInfo[i].pDoneSemaphore == SEM_FAILED)
			{
				CDTRACE("failed to create sync semaphore");
				ASSERT(0);
				return;
			}

#ifdef ONE_THREAD_PER_CHANNEL
			gpReadInfo[i].pStartSemaphore = RE3_SEM_OPEN("/semaphore_start%d", i);

			if (gpReadInfo[i].pStartSemaphore == SEM_FAILED)
			{
				CDTRACE("failed to create start semaphore");
				ASSERT(0);
				return;
			}
			gpReadInfo[i].nThreadStatus = 0;
			int *channelI = (int*)malloc(sizeof(int));
			*channelI = i;
			status = pthread_create(&gpReadInfo[i].pChannelThread, NULL, CdStreamThread, (void*)channelI);

			if (status == -1)
			{
				CDTRACE("failed to create sync thread");
				ASSERT(0);
				return;
			}
#endif
		}
	}

#ifndef ONE_THREAD_PER_CHANNEL
	debug("Using one streaming thread for all channels\n");
	gCdStreamThreadStatus = 0;
	status = pthread_create(&_gCdStreamThread, NULL, CdStreamThread, nil);

	if (status == -1)
	{
		CDTRACE("failed to create sync thread");
		ASSERT(0);
		return;
	}
#else
	debug("Using separate streaming threads for each channel\n");
#endif
#endif // !__EMSCRIPTEN__ (sync mode: funcion termina tras el return inicial)
}

void
CdStreamInit(int32 numChannels)
{
	struct statvfs fsInfo;

	if((statvfs("models/gta3.img", &fsInfo)) < 0)
	{
		CDTRACE("can't get filesystem info");
		ASSERT(0);
		return;
	}
#ifdef __linux__
	_gdwCdStreamFlags = O_RDONLY | O_NOATIME;
#else
	_gdwCdStreamFlags = O_RDONLY;
#endif
	// People say it's slower
/*
	if ( fsInfo.f_bsize <= CDSTREAM_SECTOR_SIZE )
	{
		_gdwCdStreamFlags |= O_DIRECT;
		debug("Using no buffered loading for streaming\n");
	}
*/
	void *pBuffer = (void *)RwMallocAlign(CDSTREAM_SECTOR_SIZE, (RwUInt32)fsInfo.f_bsize);
	ASSERT( pBuffer != nil );

	gNumImages = 0;

	gNumChannels = numChannels;
	ASSERT( gNumChannels != 0 );

	gpReadInfo = (CdReadInfo *)calloc(numChannels, sizeof(CdReadInfo));
	ASSERT( gpReadInfo != nil );

	CDDEBUG("read info %p", gpReadInfo);

	CdStreamInitThread();

	ASSERT( pBuffer != nil );
	RwFreeAlign(pBuffer);
}

uint32
GetGTA3ImgSize(void)
{
	ASSERT( gImgFiles[0] > 0 );
#ifdef __EMSCRIPTEN__
	if (gOdIsLoose[0] && gOdLoose[0].totalSectors)
		return gOdLoose[0].totalSectors * CDSTREAM_SECTOR_SIZE;
#endif
	struct stat statbuf;

	char path[PATH_MAX];
	realpath(gImgNames[0], path);
	if (stat(path, &statbuf) == -1) {
		// Try case-insensitivity
		char* real = casepath(gImgNames[0], false);
		if (real)
		{
			realpath(real, path);
			free(real);
			if (stat(path, &statbuf) != -1)
				goto ok;
		}

		CDTRACE("can't get size of gta3.img");
		ASSERT(0);
		return 0;
	}
	ok:
	return (uint32)statbuf.st_size;
}

void
CdStreamShutdown(void)
{
#ifdef __EMSCRIPTEN__
	// Web monohilo: no hay hilos que parar.
	return;
#else
    // Destroying semaphores and free(gpReadInfo) will be done at threads
#ifndef ONE_THREAD_PER_CHANNEL
	gCdStreamThreadStatus = 2;
	sem_post(gCdStreamSema);
	pthread_join(_gCdStreamThread, nil);
#else
	for ( int32 i = 0; i < gNumChannels; i++ ) {
		gpReadInfo[i].nThreadStatus = 2;
		sem_post(gpReadInfo[i].pStartSemaphore);
		pthread_join(gpReadInfo[i].pChannelThread, nil);
	}
#endif
#endif // !__EMSCRIPTEN__
}


int32
CdStreamRead(int32 channel, void *buffer, uint32 offset, uint32 size)
{
	ASSERT( channel < gNumChannels );
	ASSERT( buffer != nil );

	lastPosnRead = size + offset;

	ASSERT( _GET_INDEX(offset) < MAX_CDIMAGES );
	int32 hImage = gImgFiles[_GET_INDEX(offset)];
	ASSERT( hImage > 0 );

	CdReadInfo *pChannel = &gpReadInfo[channel];
	ASSERT( pChannel != nil );

	if ( pChannel->nSectorsToRead != 0 || pChannel->bReading ) {
		if (pChannel->hFile == hImage - 1 && pChannel->nSectorOffset == _GET_OFFSET(offset) && pChannel->nSectorsToRead >= size)
			return STREAM_SUCCESS;
#ifdef FLUSHABLE_STREAMING
		flushStream[channel] = 1;
		CdStreamSync(channel);
#else
		return STREAM_NONE;
#endif
	}

	pChannel->hFile = hImage - 1;
	pChannel->nStatus = STREAM_NONE;
	pChannel->nSectorOffset = _GET_OFFSET(offset);
	pChannel->nSectorsToRead = size;
	pChannel->pBuffer = buffer;
	pChannel->bLocked = 0;

#ifdef __EMSCRIPTEN__
	// Web monohilo: leer en linea. El fetch on-demand (wraps) ya suspendio
	// lo necesario; aqui solo se copia de MEMFS.
	pChannel->nStatus = OdDoRead(_GET_INDEX(offset), buffer, _GET_OFFSET(offset), size);
	pChannel->nSectorsToRead = 0;
	pChannel->bReading = false;
	return STREAM_SUCCESS;
#else
#ifndef ONE_THREAD_PER_CHANNEL
	AddToQueue(&gChannelRequestQ, channel);
	if ( sem_post(gCdStreamSema) != 0 )
		printf("Signal Sema Error\n");
#else
	if ( sem_post(pChannel->pStartSemaphore) != 0 )
		printf("Signal Sema Error\n");
#endif
#endif // !__EMSCRIPTEN__

	return STREAM_SUCCESS;
}

int32
CdStreamGetStatus(int32 channel)
{
	ASSERT( channel < gNumChannels );
	CdReadInfo *pChannel = &gpReadInfo[channel];
	ASSERT( pChannel != nil );

#ifdef ONE_THREAD_PER_CHANNEL
	if (pChannel->nThreadStatus == 2)
		return STREAM_NONE;
#else
	if (gCdStreamThreadStatus == 2)
		return STREAM_NONE;
#endif

	if ( pChannel->bReading )
		return STREAM_READING;

	if ( pChannel->nSectorsToRead != 0 )
		return STREAM_WAITING;

	if ( pChannel->nStatus != STREAM_NONE )
	{
		int32 status = pChannel->nStatus;
		pChannel->nStatus = STREAM_NONE;

		return status;
	}

	return STREAM_NONE;
}

int32
CdStreamGetLastPosn(void)
{
	return lastPosnRead;
}

// wait for channel to finish reading
int32
CdStreamSync(int32 channel)
{
	ASSERT( channel < gNumChannels );
	CdReadInfo *pChannel = &gpReadInfo[channel];
	ASSERT( pChannel != nil );

#ifdef FLUSHABLE_STREAMING
	if (flushStream[channel]) {
		pChannel->nSectorsToRead = 0;
#ifdef ONE_THREAD_PER_CHANNEL
		pthread_kill(pChannel->pChannelThread, SIGUSR1);
		if (pChannel->bReading) {
			pChannel->bLocked = true;
#else
		if (pChannel->bReading) {
			pChannel->bLocked = true;
			pthread_kill(_gCdStreamThread, SIGUSR1);
#endif
			while (pChannel->bLocked)
				sem_wait(pChannel->pDoneSemaphore);
		}
		pChannel->bReading = false;
		flushStream[channel] = false;
		return STREAM_NONE;
	}
#endif

	if ( pChannel->nSectorsToRead != 0 )
	{
		pChannel->bLocked = true;
		while (pChannel->bLocked && pChannel->nSectorsToRead != 0){
			sem_wait(pChannel->pDoneSemaphore);
		}
		pChannel->bLocked = false;
	}

	pChannel->bReading = false;

	return pChannel->nStatus;
}

void
AddToQueue(Queue *queue, int32 item)
{
	ASSERT( queue != nil );
	ASSERT( queue->items != nil );
	queue->items[queue->tail] = item;

	queue->tail = (queue->tail + 1) % queue->size;

	if ( queue->head == queue->tail )
		debug("Queue is full\n");
}

int32
GetFirstInQueue(Queue *queue)
{
	ASSERT( queue != nil );
	if ( queue->head == queue->tail )
		return -1;

	ASSERT( queue->items != nil );
	return queue->items[queue->head];
}

void
RemoveFirstInQueue(Queue *queue)
{
	ASSERT( queue != nil );
	if ( queue->head == queue->tail )
	{
		debug("Queue is empty\n");
		return;
	}

	queue->head = (queue->head + 1) % queue->size;
}

void *CdStreamThread(void *param)
{
	debug("Created cdstream thread\n");

#ifndef ONE_THREAD_PER_CHANNEL
	while (gCdStreamThreadStatus != 2) {
		sem_wait(gCdStreamSema);

		int32 channel = GetFirstInQueue(&gChannelRequestQ);
		
		// spurious wakeup
		if (channel == -1)
			continue;
#else
	int channel = *((int*)param);
	while (gpReadInfo[channel].nThreadStatus != 2){
		sem_wait(gpReadInfo[channel].pStartSemaphore);
#endif

		CdReadInfo *pChannel = &gpReadInfo[channel];
		ASSERT( pChannel != nil );

		// spurious wakeup or we sent interrupt signal for flushing
		if(pChannel->nSectorsToRead == 0)
			continue;

		pChannel->bReading = true;

		// Not standard POSIX :shrug:
#ifdef __linux__
#ifdef ONE_THREAD_PER_CHANNEL
		if (gpReadInfo[channel].nThreadStatus == 0){
			gpReadInfo[channel].nThreadStatus = 1;
#else
		if (gCdStreamThreadStatus == 0){
			gCdStreamThreadStatus = 1;
#endif
			pid_t tid = syscall(SYS_gettid);
			int ret = setpriority(PRIO_PROCESS, tid, getpriority(PRIO_PROCESS, getpid()) + 1);
		}
#endif
		if ( pChannel->nStatus == STREAM_NONE )
		{
			ASSERT(pChannel->hFile >= 0);
			ASSERT(pChannel->pBuffer != nil );

			lseek(pChannel->hFile, (size_t)pChannel->nSectorOffset * (size_t)CDSTREAM_SECTOR_SIZE, SEEK_SET);
			if (read(pChannel->hFile, pChannel->pBuffer, pChannel->nSectorsToRead * CDSTREAM_SECTOR_SIZE) == -1) {
				// pChannel->nSectorsToRead == 0 at this point means we wanted to flush channel
				// STREAM_WAITING is a little hack to make CStreaming not process this data
				pChannel->nStatus = pChannel->nSectorsToRead == 0 ? STREAM_WAITING : STREAM_ERROR;
			} else {
				pChannel->nStatus = STREAM_NONE;
			}
		}

#ifndef ONE_THREAD_PER_CHANNEL
		RemoveFirstInQueue(&gChannelRequestQ);
#endif

		pChannel->nSectorsToRead = 0;
		if ( pChannel->bLocked )
		{
			pChannel->bLocked = 0;
			sem_post(pChannel->pDoneSemaphore);
		}
		pChannel->bReading = false;
	}
	char semName[20];
#ifndef ONE_THREAD_PER_CHANNEL
	for ( int32 i = 0; i < gNumChannels; i++ )
	{
		RE3_SEM_CLOSE(gpReadInfo[i].pDoneSemaphore, "/semaphore_done%d", i);
	}
	RE3_SEM_CLOSE(gCdStreamSema, "/semaphore_cd_stream");
	free(gChannelRequestQ.items);
#else
	RE3_SEM_CLOSE(gpReadInfo[channel].pStartSemaphore, "/semaphore_start%d", channel);

	RE3_SEM_CLOSE(gpReadInfo[channel].pDoneSemaphore, "/semaphore_done%d", channel);
#endif
	if (gpReadInfo)
		free(gpReadInfo);
	gpReadInfo = nil;
	pthread_exit(nil);
}

bool
CdStreamAddImage(char const *path)
{
	ASSERT(path != nil);
	ASSERT(gNumImages < MAX_CDIMAGES);

	gImgFiles[gNumImages] = open(path, _gdwCdStreamFlags);

	// Fix case sensitivity and backslashes.
	if (gImgFiles[gNumImages] == -1) {
		char* real = casepath(path, false);
		if (real)
		{
			gImgFiles[gNumImages] = open(real, _gdwCdStreamFlags);
			free(real);
		}
	}

#ifdef __EMSCRIPTEN__
	// Web on-demand: verificar que lo abierto es un FICHERO (un directorio
	// suelto con el mismo nombre también "abre" bien y luego todo read falla).
	if (gImgFiles[gNumImages] != -1) {
		struct stat st;
		if (fstat(gImgFiles[gNumImages], &st) != 0 || !S_ISREG(st.st_mode)) {
			close(gImgFiles[gNumImages]);
			gImgFiles[gNumImages] = -1;
		}
	}
	// Preferir directorio suelto ("models/gta3.img/" + .dir): ahorra cientos
	// de MB frente al .img monolítico. Si no hay, cae al archivo.
	{
		char base[128], dirf[140];
		size_t i, n = strlen(path);
		if (n >= sizeof(base)) n = sizeof(base) - 1;
		for (i = 0; i < n; i++) {
			char c = path[i];
			base[i] = c == '\\' ? '/' : (c >= 'A' && c <= 'Z' ? c + 32 : c);
		}
		base[n] = '\0';
		struct stat st;
		if (stat(base, &st) == 0 && S_ISDIR(st.st_mode)) {
			strcpy(dirf, base);
			char *dot = strrchr(dirf, '.');
			if (dot) strcpy(dot, ".dir");
			else strcat(dirf, ".dir");
			int dfd = open(dirf, O_RDONLY);
			if (dfd >= 0) {
				struct stat dst;
				int entries = 0;
				if (fstat(dfd, &dst) == 0) entries = (int)(dst.st_size / 32);
				if (entries > 0) {
					OdLoose *L = &gOdLoose[gNumImages];
					L->entries = (OdDirEntry*)calloc(entries, sizeof(OdDirEntry));
					if (L->entries) {
						int got = 0;
						uint32 tot = 0;
						for (int k = 0; k < entries; k++) {
							uint8 raw[32];
							int r = 0;
							while (r < 32) { int q = read(dfd, raw + r, 32 - r); if (q <= 0) break; r += q; }
							if (r < 32) break;
							OdDirEntry *e = &L->entries[got];
							e->start = raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24);
							e->sectors = raw[4] | (raw[5] << 8) | (raw[6] << 16) | (raw[7] << 24);
							memcpy(e->name, raw + 8, 24);
							e->name[24] = '\0';
							tot += e->sectors;
							got++;
						}
						if (got > 0) {
							strcpy(L->base, base);
							L->nentries = got;
							L->totalSectors = tot;
							L->lastHit = -1;
							gOdIsLoose[gNumImages] = true;
							if (gImgFiles[gNumImages] != -1) close(gImgFiles[gNumImages]);
							gImgFiles[gNumImages] = dfd; // ++ de abajo lo deja como los archive
							goto registered;
						}
						free(L->entries);
						L->entries = nil;
					}
				}
				close(dfd);
			}
		}
	}
#endif // __EMSCRIPTEN__ (preferencia suelto; abajo cae a archivo si -1)

	if ( gImgFiles[gNumImages] == -1 ) {
		assert(false);
		return false;
	}
#ifdef __EMSCRIPTEN__
registered:;
#endif

	gImgNames[gNumImages] = strdup(path);
	gImgFiles[gNumImages]++; // because -1: error 0: not used

	strcpy(gCdImageNames[gNumImages], path);

#ifdef __EMSCRIPTEN__
	{ char ob[160]; snprintf(ob, sizeof(ob), "addimage %s %s", path, gOdIsLoose[gNumImages] ? "LOOSE" : "ARCHIVE"); ODTRACES(ob); }
#endif
	gNumImages++;

	return true;
}

char *
CdStreamGetImageName(int32 cd)
{
	ASSERT(cd < MAX_CDIMAGES);
	if ( gImgFiles[cd] > 0)
		return gCdImageNames[cd];

	return nil;
}

void
CdStreamRemoveImages(void)
{
	for ( int32 i = 0; i < gNumChannels; i++ ) {
#ifdef FLUSHABLE_STREAMING
		flushStream[i] = 1;
#endif
		CdStreamSync(i);
	}

	for ( int32 i = 0; i < gNumImages; i++ )
	{
		close(gImgFiles[i] - 1);
		free(gImgNames[i]);
		gImgFiles[i] = 0;
#ifdef __EMSCRIPTEN__
		if (gOdIsLoose[i]) {
			free(gOdLoose[i].entries);
			gOdLoose[i].entries = nil;
			gOdLoose[i].nentries = 0;
			gOdIsLoose[i] = false;
		}
#endif
	}

	gNumImages = 0;
}

int32
CdStreamGetNumImages(void)
{
	return gNumImages;
}
#endif
