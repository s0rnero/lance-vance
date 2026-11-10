#include "common.h"

#include "templates.h"
#include "General.h"
#include "Streaming.h"
#include "RwHelper.h"
#include "TxdStore.h"

#ifdef __EMSCRIPTEN__
#include "ondemand.h"

long long gOdTxdBytes = 0;
long long gOdTxdLiveBytes = 0;
int gOdTxdLive = 0;
extern long long gOdTexBytes;
static long long odTxdAntes[TXDSTORESIZE];
static long long odTxdSlotBytes[TXDSTORESIZE];
static uint8 odTxdSlotLive[TXDSTORESIZE];
static int odTxdBig = 0;
static int odTxdFreeN = 0;
static int odTxdCurSlot = -1;
static char odTxdCurName[20];

static void
OdTxdBegin(int slot)
{
	odTxdCurSlot = slot;
	odTxdCurName[0] = '\0';
	if(slot >= 0)
		snprintf(odTxdCurName, sizeof odTxdCurName, "%.19s", CTxdStore::GetSlot(slot)->name);
}

static void
OdTxdEnd(void)
{
	odTxdCurSlot = -1;
	odTxdCurName[0] = '\0';
}

static void
OdTxdAccount(int slot, const char *name, long long antes)
{
	long long old = 0;
	if(slot >= 0 && slot < TXDSTORESIZE){
		old = odTxdSlotBytes[slot];
		if(odTxdSlotLive[slot]){
			gOdTxdLiveBytes -= old;
			if(gOdTxdLiveBytes < 0)
				gOdTxdLiveBytes = 0;
			gOdTxdLive--;
			odTxdSlotLive[slot] = 0;
			odTxdSlotBytes[slot] = 0;
		}
	}
	long long bytes = gOdTexBytes - antes;
	if(bytes < 0)
		bytes = 0;
	if(slot >= 0 && slot < TXDSTORESIZE){
		odTxdSlotBytes[slot] = bytes;
		odTxdSlotLive[slot] = 1;
		gOdTxdLiveBytes += bytes;
	}
	gOdTxdBytes += bytes;
	gOdTxdLive++;
	if(old != 0){
		char t[176];
		snprintf(t, sizeof t, "TXDLOAD slot=%d nom=%.19s old=%lldKB new=%lldKB",
			slot, name ? name : "?", (long long)(old / 1024), (long long)(bytes / 1024));
		ODTRACES(t);
	}
	if(bytes < 2*1024*1024)
		return;
	odTxdBig++;
	if(odTxdBig > 60 && (odTxdBig % 200) != 0)
		return;
	char t[224];
	snprintf(t, sizeof t, "ODTXD n=%d slot=%d nom=%.19s vram=%lldKB acum=%lldMB vivos=%d vivosbytes=%lldMB",
		odTxdBig, slot, name ? name : "?", (long long)(bytes / 1024),
		(long long)(gOdTxdBytes / 1048576), gOdTxdLive, (long long)(gOdTxdLiveBytes / 1048576));
	ODTRACES(t);
}

static void
OdTxdRelease(int slot, TxdDef *def, const char *why)
{
	if(slot < 0 || slot >= TXDSTORESIZE || def == nil)
		return;
	if(def->texDict == nil && odTxdSlotBytes[slot] == 0 && !odTxdSlotLive[slot])
		return;
	if(def->texDict)
		RwTexDictionaryDestroy(def->texDict);
	def->texDict = nil;
	long long bytes = odTxdSlotBytes[slot];
	if(odTxdSlotLive[slot]){
		gOdTxdLiveBytes -= bytes;
		if(gOdTxdLiveBytes < 0)
			gOdTxdLiveBytes = 0;
		gOdTxdLive--;
	}
	odTxdSlotBytes[slot] = 0;
	odTxdSlotLive[slot] = 0;
	odTxdFreeN++;
	if(odTxdFreeN > 2000 && (odTxdFreeN % 50) != 0)
		return;
	char t[224];
	snprintf(t, sizeof t, "TXDFREE slot=%d nom=%.19s bytes=%lldKB why=%s vivos=%d vivosbytes=%lldMB acum=%lldMB refs=%d",
		slot, def->name, (long long)(bytes / 1024), why, gOdTxdLive,
		(long long)(gOdTxdLiveBytes / 1048576), (long long)(gOdTxdBytes / 1048576), def->refCount);
	ODTRACES(t);
}

int OdTxdCurrentSlot(void) { return odTxdCurSlot; }
const char *OdTxdCurrentName(void) { return odTxdCurName; }
#endif


CPool<TxdDef,TxdDef> *CTxdStore::ms_pTxdPool;
RwTexDictionary *CTxdStore::ms_pStoredTxd;

void
CTxdStore::Initialise(void)
{
	if(ms_pTxdPool == nil)
		ms_pTxdPool = new CPool<TxdDef,TxdDef>(TXDSTORESIZE, "TexDictionary");
}

void
CTxdStore::Shutdown(void)
{
	if(ms_pTxdPool)
		delete ms_pTxdPool;
}

void
CTxdStore::GameShutdown(void)
{
	int i;

	for(i = 0; i < TXDSTORESIZE; i++){
		TxdDef *def = GetSlot(i);
		if(def && GetNumRefs(i) == 0)
			RemoveTxdSlot(i);
	}
}

int
CTxdStore::AddTxdSlot(const char *name)
{
	TxdDef *def = ms_pTxdPool->New();
	assert(def);
	def->texDict = nil;
	def->refCount = 0;
	def->loadNumTextures = 0;
	def->loadStreamPos = 0;
	strcpy(def->name, name);
	return ms_pTxdPool->GetJustIndex(def);
}

void
CTxdStore::RemoveTxdSlot(int slot)
{
	TxdDef *def = GetSlot(slot);
#ifdef __EMSCRIPTEN__
	OdTxdRelease(slot, def, "slot");
#else
	if(def->texDict)
		RwTexDictionaryDestroy(def->texDict);
#endif
	ms_pTxdPool->Delete(def);
}

int
CTxdStore::FindTxdSlot(const char *name)
{
	int size = ms_pTxdPool->GetSize();
	for(int i = 0; i < size; i++){
		TxdDef *def = GetSlot(i);
		if(def && !CGeneral::faststricmp(def->name, name))
			return i;
	}
	return -1;
}

char*
CTxdStore::GetTxdName(int slot)
{
	return GetSlot(slot)->name;
}

void
CTxdStore::PushCurrentTxd(void)
{
	ms_pStoredTxd = RwTexDictionaryGetCurrent();
}

void
CTxdStore::PopCurrentTxd(void)
{
	RwTexDictionarySetCurrent(ms_pStoredTxd);
	ms_pStoredTxd = nil;
}

void
CTxdStore::SetCurrentTxd(int slot)
{
	RwTexDictionarySetCurrent(GetSlot(slot)->texDict);
}

void
CTxdStore::Create(int slot)
{
	GetSlot(slot)->texDict = RwTexDictionaryCreate();
}

int
CTxdStore::GetNumRefs(int slot)
{
	return GetSlot(slot)->refCount;
}

void
CTxdStore::AddRef(int slot)
{
	GetSlot(slot)->refCount++;
}

void
CTxdStore::RemoveRef(int slot)
{
	if(--GetSlot(slot)->refCount <= 0)
		CStreaming::RemoveTxd(slot);
}

void
CTxdStore::RemoveRefWithoutDelete(int slot)
{
	GetSlot(slot)->refCount--;
}

bool
CTxdStore::LoadTxd(int slot, RwStream *stream)
{
	if(ms_pTxdPool == nil || slot < 0 || slot >= ms_pTxdPool->GetSize())
		return false;
	if(stream == nil)
		return false;

	TxdDef *def = GetSlot(slot);
#ifdef __EMSCRIPTEN__
	OdTxdBegin(slot);
#endif

	if(RwStreamFindChunk(stream, rwID_TEXDICTIONARY, nil, nil)){
#ifdef __EMSCRIPTEN__
		long long odAntes = gOdTexBytes;
#endif
		def->texDict = RwTexDictionaryGtaStreamRead(stream);
		def->loadNumTextures = 0;
		def->loadStreamPos = 0;
#ifdef __EMSCRIPTEN__
		if(def->texDict)
			OdTxdAccount(slot, def->name, odAntes);
		OdTxdEnd();
#endif
		return def->texDict != nil;
	}
#ifdef __EMSCRIPTEN__
	OdTxdEnd();
#endif
	printf("Failed to load TXD\n");
	return false;
}

bool
CTxdStore::LoadTxd(int slot, const char *filename)
{
	RwStream *stream;
	bool ret;

	ret = false;
	_rwD3D8TexDictionaryEnableRasterFormatConversion(true);
	do
		stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, filename);
	while(stream == nil);
	ret = LoadTxd(slot, stream);
	RwStreamClose(stream, nil);
	return ret;
}

bool
CTxdStore::StartLoadTxd(int slot, RwStream *stream)
{
	if(ms_pTxdPool == nil || slot < 0 || slot >= ms_pTxdPool->GetSize())
		return false;
	if(stream == nil)
		return false;

	TxdDef *def = GetSlot(slot);
#ifdef __EMSCRIPTEN__
	OdTxdBegin(slot);
#endif
	if(RwStreamFindChunk(stream, rwID_TEXDICTIONARY, nil, nil)){
#ifdef __EMSCRIPTEN__
		odTxdAntes[slot] = gOdTexBytes;
#endif
		def->texDict = RwTexDictionaryGtaStreamRead1(stream, &def->loadNumTextures, &def->loadStreamPos);
#ifdef VICEEXT_ALLOC_GUARD
		// Nombrar el TXD grande: es el contexto que falta para saber que se
		// estaba parseando cuando el monton aborta.
		if(def->loadNumTextures > OD_TXD_TRACE_MIN_TEX || def->loadNumTextures < 0 || def->texDict == nil){
			char odT[128];
			snprintf(odT, sizeof odT, "ODTXD slot=%d nom=%s ntex=%d ok=%d",
				slot, def->name, (int)def->loadNumTextures, def->texDict != nil);
			ODTRACES(odT);
		}
#endif
#ifdef __EMSCRIPTEN__
		if(def->texDict == nil)
			OdTxdEnd();
#endif
		return def->texDict != nil;
	}else{
#ifdef __EMSCRIPTEN__
		OdTxdEnd();
#endif
		printf("Failed to load TXD\n");
		return false;
	}
}

bool
CTxdStore::FinishLoadTxd(int slot, RwStream *stream)
{
	if(ms_pTxdPool == nil || slot < 0 || slot >= ms_pTxdPool->GetSize())
		return false;
	if(stream == nil)
		return false;

	TxdDef *def = GetSlot(slot);
	if(def->texDict == nil)
		return false;
#ifdef __EMSCRIPTEN__
	OdTxdBegin(slot);
#endif
	def->texDict = RwTexDictionaryGtaStreamRead2(stream, def->texDict, def->loadNumTextures, def->loadStreamPos);
#ifdef __EMSCRIPTEN__
	if(def->texDict)
		OdTxdAccount(slot, def->name, odTxdAntes[slot]);
	OdTxdEnd();
#endif
	return def->texDict != nil;
}

void
CTxdStore::RemoveTxd(int slot)
{
	TxdDef *def = GetSlot(slot);
#ifdef __EMSCRIPTEN__
	OdTxdRelease(slot, def, "txd");
#else
	if(def->texDict)
		RwTexDictionaryDestroy(def->texDict);
#endif
	def->texDict = nil;
}
