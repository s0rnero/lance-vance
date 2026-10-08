#include "common.h"
#ifdef __EMSCRIPTEN__
#include "ondemand.h"
#endif

#include "General.h"
#include "Pad.h"
#include "Hud.h"
#include "Text.h"
#include "Clock.h"
#include "Renderer.h"
#include "ModelInfo.h"
#include "TxdStore.h"
#include "ModelIndices.h"
#include "Pools.h"
#include "Wanted.h"
#include "Directory.h"
#include "RwHelper.h"
#include "World.h"
#include "Entity.h"
#include "FileMgr.h"
#include "FileLoader.h"
#include "Zones.h"
#include "Radar.h"
#include "Camera.h"
#include "Record.h"
#include "CarCtrl.h"
#include "Population.h"
#include "Gangs.h"
#include "CutsceneMgr.h"
#include "CdStream.h"
#include "Streaming.h"
#include "Replay.h"
#include "main.h"
#include "ColStore.h"
#include "DMAudio.h"
#include "Script.h"
#include "MemoryMgr.h"
#include "MemoryHeap.h"
#include "Font.h"
#include "Frontend.h"
#include "VarConsole.h"

bool CStreaming::ms_disableStreaming;
bool CStreaming::ms_bLoadingBigModel;
int32 CStreaming::ms_numModelsRequested;
CStreamingInfo CStreaming::ms_aInfoForModel[NUMSTREAMINFO];
CStreamingInfo CStreaming::ms_startLoadedList;
CStreamingInfo CStreaming::ms_endLoadedList;
CStreamingInfo CStreaming::ms_startRequestedList;
CStreamingInfo CStreaming::ms_endRequestedList;
int32 CStreaming::ms_oldSectorX;
int32 CStreaming::ms_oldSectorY;
int32 CStreaming::ms_streamingBufferSize;
#ifndef ONE_THREAD_PER_CHANNEL
int8 *CStreaming::ms_pStreamingBuffer[2];
#else
int8 *CStreaming::ms_pStreamingBuffer[4];
#endif
size_t CStreaming::ms_memoryUsed;
CStreamingChannel CStreaming::ms_channel[2];
int32 CStreaming::ms_channelError;
int32 CStreaming::ms_numVehiclesLoaded;
int32 CStreaming::ms_numPedsLoaded;
int32 CStreaming::ms_vehiclesLoaded[MAXVEHICLESLOADED];
int32 CStreaming::ms_lastVehicleDeleted;
bool CStreaming::ms_bIsPedFromPedGroupLoaded[NUMMODELSPERPEDGROUP];
CDirectory *CStreaming::ms_pExtraObjectsDir;
int32 CStreaming::ms_numPriorityRequests;
int32 CStreaming::ms_currentPedGrp;
int32 CStreaming::ms_currentPedLoading;
int32 CStreaming::ms_lastCullZone;
uint16 CStreaming::ms_loadedGangs;
uint16 CStreaming::ms_loadedGangCars;
int32 CStreaming::ms_imageOffsets[NUMCDIMAGES];
int32 CStreaming::ms_lastImageRead;
int32 CStreaming::ms_imageSize;
size_t CStreaming::ms_memoryAvailable;

int32 desiredNumVehiclesLoaded = 12;

CEntity *pIslandLODmainlandEntity;
CEntity *pIslandLODbeachEntity;
int32 islandLODmainland;
int32 islandLODbeach;

#ifndef MASTER
bool gbPrintStats;
bool gbPrintVehiclesInMemory;  // TODO
bool gbPrintStreamingBuffer; // TODO
#endif

bool
CStreamingInfo::GetCdPosnAndSize(uint32 &posn, uint32 &size)
{
	if(m_size == 0)
		return false;
	posn = m_position;
	size = m_size;
	return true;
}

void
CStreamingInfo::SetCdPosnAndSize(uint32 posn, uint32 size)
{
	m_position = posn;
	m_size = size;
}

void
CStreamingInfo::AddToList(CStreamingInfo *link)
{
	// Insert this after link
	m_next = link->m_next;
	m_prev = link;
	link->m_next = this;
	m_next->m_prev = this;
}

void
CStreamingInfo::RemoveFromList(void)
{
	m_next->m_prev = m_prev;
	m_prev->m_next = m_next;
	m_next = nil;
	m_prev = nil;
}

void
CStreaming::Init2(void)
{
	int i;

	for(i = 0; i < NUMSTREAMINFO; i++){
		ms_aInfoForModel[i].m_loadState = STREAMSTATE_NOTLOADED;
		ms_aInfoForModel[i].m_next = nil;
		ms_aInfoForModel[i].m_prev = nil;
		ms_aInfoForModel[i].m_nextID = -1;
		ms_aInfoForModel[i].m_size = 0;
		ms_aInfoForModel[i].m_position = 0;
	}

	ms_channelError = -1;

	// init lists

	ms_startLoadedList.m_next = &ms_endLoadedList;
	ms_startLoadedList.m_prev = nil;
	ms_endLoadedList.m_prev = &ms_startLoadedList;
	ms_endLoadedList.m_next = nil;

	ms_startRequestedList.m_next = &ms_endRequestedList;
	ms_startRequestedList.m_prev = nil;
	ms_endRequestedList.m_prev = &ms_startRequestedList;
	ms_endRequestedList.m_next = nil;

	// init misc

	ms_oldSectorX = 0;
	ms_oldSectorY = 0;
	ms_streamingBufferSize = 0;
	ms_disableStreaming = false;
	ms_memoryUsed = 0;
	ms_bLoadingBigModel = false;

	// init channels

	ms_channel[0].state = CHANNELSTATE_IDLE;
	ms_channel[1].state = CHANNELSTATE_IDLE;
	for(i = 0; i < 4; i++){
		ms_channel[0].streamIds[i] = -1;
		ms_channel[0].offsets[i] = -1;
		ms_channel[1].streamIds[i] = -1;
		ms_channel[1].offsets[i] = -1;
	}

	// init stream info, mark things that are already loaded

	for(i = 0; i < MODELINFOSIZE; i++){
		CBaseModelInfo *mi = CModelInfo::GetModelInfo(i);
		if(mi && mi->GetRwObject()){
			ms_aInfoForModel[i].m_loadState = STREAMSTATE_LOADED;
			ms_aInfoForModel[i].m_flags = STREAMFLAGS_DONT_REMOVE;
			if(mi->IsSimple())
				((CSimpleModelInfo*)mi)->m_alpha = 255;
		}
	}

	for(i = 0; i < TXDSTORESIZE; i++)
		if(CTxdStore::GetSlot(i) && CTxdStore::GetSlot(i)->texDict)
			ms_aInfoForModel[i + STREAM_OFFSET_TXD].m_loadState = STREAMSTATE_LOADED;


	for(i = 0; i < MAXVEHICLESLOADED; i++)
		ms_vehiclesLoaded[i] = -1;
	ms_numVehiclesLoaded = 0;
	ms_numPedsLoaded = 8;

	for(i = 0; i < ARRAY_SIZE(ms_bIsPedFromPedGroupLoaded); i++)
		ms_bIsPedFromPedGroupLoaded[i] = false;

	ms_pExtraObjectsDir = new CDirectory(EXTRADIRSIZE);
	ms_numPriorityRequests = 0;
	ms_currentPedGrp = -1;
	ms_lastCullZone = -1;		// unused because RemoveModelsNotVisibleFromCullzone is gone
	ms_loadedGangs = 0;
	ms_currentPedLoading = NUMMODELSPERPEDGROUP;	// unused, whatever it is

	LoadCdDirectory();

	// allocate streaming buffers
	if(ms_streamingBufferSize & 1) ms_streamingBufferSize++;
#ifndef ONE_THREAD_PER_CHANNEL
	ms_pStreamingBuffer[0] = (int8*)RwMallocAlign(ms_streamingBufferSize*CDSTREAM_SECTOR_SIZE, CDSTREAM_SECTOR_SIZE);
	ms_streamingBufferSize /= 2;
	ms_pStreamingBuffer[1] = ms_pStreamingBuffer[0] + ms_streamingBufferSize*CDSTREAM_SECTOR_SIZE;
#else
	ms_pStreamingBuffer[0] = (int8*)RwMallocAlign(ms_streamingBufferSize*2*CDSTREAM_SECTOR_SIZE, CDSTREAM_SECTOR_SIZE);
	ms_streamingBufferSize /= 2;
	ms_pStreamingBuffer[1] = ms_pStreamingBuffer[0] + ms_streamingBufferSize*CDSTREAM_SECTOR_SIZE;
	ms_pStreamingBuffer[2] = ms_pStreamingBuffer[1] + ms_streamingBufferSize*CDSTREAM_SECTOR_SIZE;
	ms_pStreamingBuffer[3] = ms_pStreamingBuffer[2] + ms_streamingBufferSize*CDSTREAM_SECTOR_SIZE;
#endif
	debug("Streaming buffer size is %d sectors", ms_streamingBufferSize);

	// PC only, figure out how much memory we got
#ifdef GTA_PC
#define MB (1024*1024)
#ifdef FIX_BUGS
	// do what gta3 does
	extern size_t _dwMemAvailPhys;
	ms_memoryAvailable = (_dwMemAvailPhys - 10*MB)/2;
	if(ms_memoryAvailable < 65*MB)
		ms_memoryAvailable = 65*MB;
	desiredNumVehiclesLoaded = (int32)((ms_memoryAvailable / MB - 65) / 3 + 12);
	if(desiredNumVehiclesLoaded > MAXVEHICLESLOADED)
		desiredNumVehiclesLoaded = MAXVEHICLESLOADED;
#else
	ms_memoryAvailable = 65 * MB;
	desiredNumVehiclesLoaded = 25;
	debug("Memory allocated to Streaming is %zuMB", ms_memoryAvailable/MB); // original modifier was %d
#endif
#undef MB
#endif

	// find island LODs

	pIslandLODmainlandEntity = nil;
	pIslandLODbeachEntity = nil;
	islandLODmainland = -1;
	islandLODbeach = -1;
	CModelInfo::GetModelInfo("IslandLODmainland", &islandLODmainland);
	CModelInfo::GetModelInfo("IslandLODbeach", &islandLODbeach);

#ifndef MASTER
	VarConsole.Add("Streaming Debug", &gbPrintStats, true);
	VarConsole.Add("Streaming Vehicle Debug", &gbPrintVehiclesInMemory, true);
	VarConsole.Add("Printf Streaming Buffer contents", &gbPrintStreamingBuffer, true);
#endif
}

void
CStreaming::Init(void)
{
#ifdef USE_TXD_CDIMAGE
	if(!CanVideoCardDoDXT()){
		int txdHandle = CFileMgr::OpenFile("MODELS\\TXD.IMG", "r");
		if (txdHandle)
			CFileMgr::CloseFile(txdHandle);
		if (!CheckVideoCardCaps() && txdHandle) {
			CdStreamAddImage("MODELS\\TXD.IMG");
			CStreaming::Init2();
		} else {
			CStreaming::Init2();
			if (CreateTxdImageForVideoCard()) {
				CStreaming::Shutdown();
				CdStreamAddImage("MODELS\\TXD.IMG");
				CStreaming::Init2();
			}
		}
	} else
		CStreaming::Init2();
#else
	CStreaming::Init2();
#endif
}

void
CStreaming::ReInit(void)
{
	int i;
	CStreaming::FlushRequestList();
	CStreaming::DeleteAllRwObjects();
	CStreaming::RemoveAllUnusedModels();
	for(i = 0; i < MODELINFOSIZE; i++)
		if(CModelInfo::GetModelInfo(i) && ms_aInfoForModel[i].m_flags & STREAMFLAGS_SCRIPTOWNED)
			SetMissionDoesntRequireModel(i);
	CStreaming::ms_disableStreaming = false;
}

void
CStreaming::Shutdown(void)
{
	RwFreeAlign(ms_pStreamingBuffer[0]);
	ms_streamingBufferSize = 0;
	if(ms_pExtraObjectsDir) {
		delete ms_pExtraObjectsDir;
#ifdef FIX_BUGS
		ms_pExtraObjectsDir = nil;
#endif
	}
}

#ifndef MASTER
uint64 timeProcessingTXD;
uint64 timeProcessingDFF;
#endif

void
CStreaming::Update(void)
{
	CStreamingInfo *si, *prev;
	bool requestedSubway = false;

#ifndef MASTER
	timeProcessingTXD = 0;
	timeProcessingDFF = 0;
#endif

	UpdateMemoryUsed();

	if(ms_channelError != -1){
		RetryLoadFile(ms_channelError);
		return;
	}

	if(CTimer::GetIsPaused())
		return;

	LoadBigBuildingsWhenNeeded();
	if(!ms_disableStreaming && TheCamera.GetPosition().z < 55.0f)
		AddModelsToRequestList(TheCamera.GetPosition(), 0);

	DeleteFarAwayRwObjects(TheCamera.GetPosition());

	bool odPuedeCargarTrafico = !ms_disableStreaming &&
	   !CCutsceneMgr::IsCutsceneProcessing() &&
	   !CRenderer::m_loadingPriority &&
	   CGame::currArea == AREA_MAIN_MAP &&
	   !CReplay::IsPlayingBack();
#ifdef __EMSCRIPTEN__
	// D4 (sección 1): medido. La puerta de siempre exige además
	// `ms_numModelsRequested < 5` y que no haya carga prioritaria en curso. En el
	// navegador los ficheros tardan y esas dos condiciones se quedan cerradas
	// durante minutos enteros (medido: una sesión de 5 min con 0 llamadas al
	// cargador y la calle congelada en los 2 modelos ya residentes). El cargador
	// de tráfico pide COMO MUCHO un modelo cada 11,7 s, así que puede correr con
	// el resto del streaming ocupado sin competir de verdad: la puerta se queda
	// con las condiciones que sí son de estado del juego (área, cinemática,
	// replay, streaming desactivado).
	const bool odPuedeCargarTrafico2 = !ms_disableStreaming &&
	   !CCutsceneMgr::IsCutsceneProcessing() &&
	   CGame::currArea == AREA_MAIN_MAP &&
	   !CReplay::IsPlayingBack();
	if(odPuedeCargarTrafico2){
		StreamVehiclesAndPeds();
		StreamZoneModels(FindPlayerCoors());
	}
	if(!odPuedeCargarTrafico){
		// Diagnóstico: por qué NO se cargaba tráfico (una sesión entera sin
		// cargador deja la calle en 2 modelos). Se emite cada 300 fotogramas.
		static uint32 odBloqueo = 0;
		if(++odBloqueo % 300 == 0){
			char tt[176];
			snprintf(tt, sizeof tt, "CARBLOCK n=%u disable=%d cut=%d prio=%d area=%d play=%d nreq=%d veh=%d/%d",
				(unsigned)odBloqueo, (int)ms_disableStreaming, (int)CCutsceneMgr::IsCutsceneProcessing(),
				(int)CRenderer::m_loadingPriority, (int)CGame::currArea, (int)CReplay::IsPlayingBack(),
				ms_numModelsRequested, ms_numVehiclesLoaded, desiredNumVehiclesLoaded);
			ODTRACES(tt);
		}
	}
#else
	if(odPuedeCargarTrafico && ms_numModelsRequested < 5){
		StreamVehiclesAndPeds();
		StreamZoneModels(FindPlayerCoors());
	}
#endif

#ifdef __EMSCRIPTEN__
	// F2 (fluides-v2): coste del streaming por frame, medido ALREDEDOR de la
	// llamada (R1: nada dentro del bucle suspendible). Incluye el tiempo de
	// suspension Asyncify (fetch/IDB) — para eso sirve: si domina, domina.
	uint32 odStrmT0 = (uint32)emscripten_get_now();
#endif
	LoadRequestedModels();
#ifdef __EMSCRIPTEN__
	gWebStrmMs += (uint32)emscripten_get_now() - odStrmT0;
#endif

	if(CWorld::Players[0].m_pRemoteVehicle){
		CColStore::AddCollisionNeededAtPosn(FindPlayerCoors());
		CColStore::LoadCollision(CWorld::Players[0].m_pRemoteVehicle->GetPosition());
		CColStore::EnsureCollisionIsInMemory(CWorld::Players[0].m_pRemoteVehicle->GetPosition());
	}else{
		CColStore::LoadCollision(FindPlayerCoors());
		CColStore::EnsureCollisionIsInMemory(FindPlayerCoors());
	}

	// TODO: PrintRequestList
	//if (CPad::GetPad(1)->GetLeftShoulder2JustDown() && CPad::GetPad(1)->GetRightShoulder1() && CPad::GetPad(1)->GetRightShoulder2())
	//	PrintRequestList();

	for(si = ms_endRequestedList.m_prev; si != &ms_startRequestedList; si = prev){
		prev = si->m_prev;
		if((si->m_flags & (STREAMFLAGS_KEEP_IN_MEMORY|STREAMFLAGS_PRIORITY)) == 0)
			RemoveModel(si - ms_aInfoForModel);
	}
}

void
CStreaming::LoadCdDirectory(void)
{
	char dirname[132];
	int i;

#ifdef GTA_PC
	ms_imageOffsets[0] = 0;
	ms_imageOffsets[1] = -1;
	ms_imageOffsets[2] = -1;
	ms_imageOffsets[3] = -1;
	ms_imageOffsets[4] = -1;
	ms_imageOffsets[5] = -1;
	ms_imageSize = GetGTA3ImgSize();
	// PS2 uses CFileMgr::GetCdFile on all IMG files to fill the array
#endif

	i = CdStreamGetNumImages();
	while(i-- >= 1){
		strcpy(dirname, CdStreamGetImageName(i));
		strncpy(strrchr(dirname, '.') + 1, "DIR", 3);
		LoadCdDirectory(dirname, i);
	}

	ms_lastImageRead = 0;
	ms_imageSize /= CDSTREAM_SECTOR_SIZE;
}

void
CStreaming::LoadCdDirectory(const char *dirname, int n)
{
	int fd, lastID, imgSelector;
	int modelId;
	CDirectory::DirectoryInfo direntry;
	char *dot;

	lastID = -1;
	fd = CFileMgr::OpenFile(dirname, "rb");
	assert(fd > 0);

	imgSelector = n<<24;
	assert(sizeof(direntry) == 32);
	while(CFileMgr::Read(fd, (char*)&direntry, sizeof(direntry))){
		bool bAddToStreaming = false;

		if(direntry.size > (uint32)ms_streamingBufferSize)
			ms_streamingBufferSize = direntry.size;
		direntry.name[23] = '\0';
		dot = strchr(direntry.name, '.');
		if(dot == nil || dot-direntry.name > 20){
			debug("%s is too long\n", direntry.name);
			lastID = -1;
			continue;
		}

		*dot = '\0';

		if(strncasecmp(dot+1, "DFF", 3) == 0){
			if(CModelInfo::GetModelInfo(direntry.name, &modelId)){
				bAddToStreaming = true;
			}else{
#ifdef FIX_BUGS
				// remember which cdimage this came from
				ms_pExtraObjectsDir->AddItem(direntry, n);
#else
				ms_pExtraObjectsDir->AddItem(direntry);
#endif
				lastID = -1;
			}
		}else if(strncasecmp(dot+1, "TXD", 3) == 0){
			modelId = CTxdStore::FindTxdSlot(direntry.name);
			if(modelId == -1)
				modelId = CTxdStore::AddTxdSlot(direntry.name);
			modelId += STREAM_OFFSET_TXD;
			bAddToStreaming = true;
		}else if(strncasecmp(dot+1, "COL", 3) == 0){
			modelId = CColStore::FindColSlot(direntry.name);
			if(modelId == -1)
				modelId = CColStore::AddColSlot(direntry.name);
			modelId += STREAM_OFFSET_COL;
			bAddToStreaming = true;
		}else if(strncasecmp(dot+1, "IFP", 3) == 0){
			modelId = CAnimManager::RegisterAnimBlock(direntry.name);
			modelId += STREAM_OFFSET_ANIM;
			bAddToStreaming = true;
		}else{
			*dot = '.';
			lastID = -1;
		}

		if(bAddToStreaming){
			if(ms_aInfoForModel[modelId].GetCdSize()){
				debug("%s.%s appears more than once in %s\n", direntry.name, dot+1, dirname);
				lastID = -1;
			}else{
				direntry.offset |= imgSelector;
				ms_aInfoForModel[modelId].SetCdPosnAndSize(direntry.offset, direntry.size);
				if(lastID != -1)
					ms_aInfoForModel[lastID].m_nextID = modelId;
				lastID = modelId;
			}
		}
	}

	CFileMgr::CloseFile(fd);
}

static char*
GetObjectName(int streamId)
{
	static char objname[32];
	if(streamId < STREAM_OFFSET_TXD)
		sprintf(objname, "%s.dff", CModelInfo::GetModelInfo(streamId)->GetModelName());
	else if(streamId >= STREAM_OFFSET_TXD && streamId < STREAM_OFFSET_COL)
		sprintf(objname, "%s.txd", CTxdStore::GetTxdName(streamId-STREAM_OFFSET_TXD));
	else if(streamId >= STREAM_OFFSET_COL && streamId < STREAM_OFFSET_ANIM)
		sprintf(objname, "%s.col", CColStore::GetColName(streamId-STREAM_OFFSET_COL));
	else{
		assert(streamId < NUMSTREAMINFO);
		sprintf(objname, "%s.ifp", CAnimManager::GetAnimationBlock(streamId-STREAM_OFFSET_ANIM)->name);
	}
	return objname;
}

#ifdef USE_CUSTOM_ALLOCATOR
RpAtomic*
RegisterAtomicMemPtrsCB(RpAtomic *atomic, void *data)
{
	// empty because we expect models to be pre-instanced
	return atomic;
}
#endif

#ifdef __EMSCRIPTEN__
// Enfriamiento de re-solicitud (solo web): si el mismo modelo falla muy
// seguido, saltar el ReRequest inmediato; la demanda normal lo repondrá
// luego. Evita tormentas de reintento por frame (cuelgue + RAM).
static int odLastFailId = -1;
static uint32 odLastFailFrame = 0;
static bool
OdCoolingDown(int32 streamId)
{
	uint32 fc = CTimer::GetFrameCounter();
	if (streamId == odLastFailId && fc - odLastFailFrame < 30)
		return true;
	odLastFailId = streamId;
	odLastFailFrame = fc;
	return false;
}
#endif

bool
CStreaming::ConvertBufferToObject(int8 *buf, int32 streamId)
{
	RwMemory mem;
	RwStream *stream;
	int cdsize;
	uint32 startTime, endTime, timeDiff;
	CBaseModelInfo *mi;
	bool success;

	startTime = CTimer::GetCurrentTimeInCycles() / CTimer::GetCyclesPerMillisecond();

	cdsize = ms_aInfoForModel[streamId].GetCdSize();
	mem.start = (uint8*)buf;
	mem.length = cdsize * CDSTREAM_SECTOR_SIZE;
	stream = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &mem);

	if(streamId < STREAM_OFFSET_TXD){
		// Model
		mi = CModelInfo::GetModelInfo(streamId);

		// Txd and anim have to be loaded
		int animId = mi->GetAnimFileIndex();
#ifdef FIX_BUGS
		if(!HasTxdLoaded(mi->GetTxdSlot()) ||
#else
		// texDict will exist even if only first part has loaded
		if(CTxdStore::GetSlot(mi->GetTxdSlot())->texDict == nil ||
#endif
		   animId != -1 && !CAnimManager::GetAnimationBlock(animId)->isLoaded){
			RemoveModel(streamId);
#ifdef __EMSCRIPTEN__
			if (!OdCoolingDown(streamId))
#endif
			ReRequestModel(streamId);
			RwStreamClose(stream, &mem);
			return false;
		}

		// Set Txd and anims to use
		CTxdStore::AddRef(mi->GetTxdSlot());
#if GTA_VERSION > GTAVC_PS2
		if(animId != -1)
			CAnimManager::AddAnimBlockRef(animId);
#endif

		PUSH_MEMID(MEMID_STREAM_MODELS);
		CTxdStore::SetCurrentTxd(mi->GetTxdSlot());
		if(mi->IsSimple()){
			success = CFileLoader::LoadAtomicFile(stream, streamId);
			// TODO(MIAMI)? complain if file is not pre-instanced. we hardly are interested in that
		} else if (mi->GetModelType() == MITYPE_VEHICLE) {
			// load vehicles in two parts
			CModelInfo::GetModelInfo(streamId)->AddRef();
			success = CFileLoader::StartLoadClumpFile(stream, streamId);
			if(success)
				ms_aInfoForModel[streamId].m_loadState = STREAMSTATE_STARTED;
		}else{
			success = CFileLoader::LoadClumpFile(stream, streamId);
#ifdef USE_CUSTOM_ALLOCATOR
			if(success)
				RpClumpForAllAtomics((RpClump*)mi->GetRwObject(), RegisterAtomicMemPtrsCB, nil);
#endif
		}
		POP_MEMID();
		UpdateMemoryUsed();

		// Txd and anims no longer needed unless we only read part of the file
		if(ms_aInfoForModel[streamId].m_loadState != STREAMSTATE_STARTED){
			CTxdStore::RemoveRefWithoutDelete(mi->GetTxdSlot());
#if GTA_VERSION > GTAVC_PS2
			if(animId != -1)
				CAnimManager::RemoveAnimBlockRefWithoutDelete(animId);
#endif
		}

		if(!success){
			debug("Failed to load %s\n", CModelInfo::GetModelInfo(streamId)->GetModelName());
#ifdef __EMSCRIPTEN__
			// DIAG F3a: los FAILs también a odtrace.log (la consola no basta).
			{
				static int n = 0;
				if (n < 100) {
					n++;
					char t[128];
					snprintf(t, sizeof t, "FAIL model %s", CModelInfo::GetModelInfo(streamId)->GetModelName());
					ODTRACES(t);
				}
			}
#endif
			RemoveModel(streamId);
#ifdef __EMSCRIPTEN__
			if (!OdCoolingDown(streamId))
#endif
			ReRequestModel(streamId);
			RwStreamClose(stream, &mem);
			return false;
		}
	}else if(streamId >= STREAM_OFFSET_TXD && streamId < STREAM_OFFSET_COL){
		// Txd
		if((ms_aInfoForModel[streamId].m_flags & STREAMFLAGS_KEEP_IN_MEMORY) == 0 &&
		   !IsTxdUsedByRequestedModels(streamId - STREAM_OFFSET_TXD)){
			RemoveModel(streamId);
			RwStreamClose(stream, &mem);
			return false;
		}

		PUSH_MEMID(MEMID_STREAM_TEXUTRES);
#ifdef __EMSCRIPTEN__
		// DIAG F3a (entrada): ¿qué pide vs qué hay en mem? Acotado + odtrace.
		{
			static int n = 0;
			if (n < 60) {
				n++;
				uint32 sum = 0;
				for (uint32 bi = 0; bi < mem.length && bi < 65536; bi++) sum += mem.start[bi];
				char t[256];
				snprintf(t, sizeof t, "TXDIN txd=%s id=%d cds=%d memlen=%u sum=%u head=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
					CTxdStore::GetTxdName(streamId - STREAM_OFFSET_TXD), streamId, cdsize, mem.length, sum,
					mem.length > 0 ? mem.start[0] : 0, mem.length > 1 ? mem.start[1] : 0,
					mem.length > 2 ? mem.start[2] : 0, mem.length > 3 ? mem.start[3] : 0,
					mem.length > 4 ? mem.start[4] : 0, mem.length > 5 ? mem.start[5] : 0,
					mem.length > 6 ? mem.start[6] : 0, mem.length > 7 ? mem.start[7] : 0,
					mem.length > 8 ? mem.start[8] : 0, mem.length > 9 ? mem.start[9] : 0,
					mem.length > 10 ? mem.start[10] : 0, mem.length > 11 ? mem.start[11] : 0,
					mem.length > 12 ? mem.start[12] : 0, mem.length > 13 ? mem.start[13] : 0,
					mem.length > 14 ? mem.start[14] : 0, mem.length > 15 ? mem.start[15] : 0);
				ODTRACES(t);
				printf("[texconv] %s\n", t);
			}
		}
#endif
		if(ms_bLoadingBigModel || cdsize > 200){
			success = CTxdStore::StartLoadTxd(streamId - STREAM_OFFSET_TXD, stream);
			if(success)
				ms_aInfoForModel[streamId].m_loadState = STREAMSTATE_STARTED;
		}else
		        success = CTxdStore::LoadTxd(streamId - STREAM_OFFSET_TXD, stream);
		POP_MEMID();
		UpdateMemoryUsed();

		if(!success){
			debug("Failed to load %s.txd\n", CTxdStore::GetTxdName(streamId - STREAM_OFFSET_TXD));
#ifdef __EMSCRIPTEN__
			{
				static int n = 0;
				if (n < 100) {
					n++;
					char t[128];
					snprintf(t, sizeof t, "FAIL txd %s", CTxdStore::GetTxdName(streamId - STREAM_OFFSET_TXD));
					ODTRACES(t);
				}
			}
#endif
			RemoveModel(streamId);
#ifdef __EMSCRIPTEN__
			if (!OdCoolingDown(streamId))
#endif
			ReRequestModel(streamId);
			RwStreamClose(stream, &mem);
			return false;
		}
	}else if(streamId >= STREAM_OFFSET_COL && streamId < STREAM_OFFSET_ANIM){
		PUSH_MEMID(MEMID_STREAM_COLLISION);
		bool success = CColStore::LoadCol(streamId-STREAM_OFFSET_COL, mem.start, mem.length);
		POP_MEMID();
		if(!success){
			debug("Failed to load %s.col\n", CColStore::GetColName(streamId - STREAM_OFFSET_COL));
#ifdef __EMSCRIPTEN__
			{
				static int n = 0;
				if (n < 100) {
					n++;
					char t[128];
					snprintf(t, sizeof t, "FAIL col %s", CColStore::GetColName(streamId - STREAM_OFFSET_COL));
					ODTRACES(t);
				}
			}
#endif
			RemoveModel(streamId);
#ifdef __EMSCRIPTEN__
			if (!OdCoolingDown(streamId))
#endif
			ReRequestModel(streamId);
			RwStreamClose(stream, &mem);
			return false;
		}
	}else if(streamId >= STREAM_OFFSET_ANIM){
		assert(streamId < NUMSTREAMINFO);
		if((ms_aInfoForModel[streamId].m_flags & STREAMFLAGS_KEEP_IN_MEMORY) == 0 &&
		   !AreAnimsUsedByRequestedModels(streamId - STREAM_OFFSET_ANIM)){
			RemoveModel(streamId);
			RwStreamClose(stream, &mem);
			return false;
		}
		PUSH_MEMID(MEMID_STREAM_ANIMATION);
		// D11/D18 (seccion 1, 21/09): el rastro por bloque (entrar/salir de
		// cada .ifp) se retiro al localizar el cuelgue: era una pareja de
		// printf por fichero y el alias de animaciones se comprueba mejor con
		// el aviso de clip sin jerarquia.
		CAnimManager::LoadAnimFile(stream, true, nil);
		CAnimManager::CreateAnimAssocGroups();
		POP_MEMID();
	}

	RwStreamClose(stream, &mem);

	if(streamId < STREAM_OFFSET_TXD){
		// Model
		// Vehicles and Peds not in loaded list
		if (mi->GetModelType() != MITYPE_VEHICLE && mi->GetModelType() != MITYPE_PED) {
			CSimpleModelInfo *smi = (CSimpleModelInfo*)mi;

			// Set fading for some objects
			if(mi->IsSimple() && !smi->m_isBigBuilding){
				if(ms_aInfoForModel[streamId].m_flags & STREAMFLAGS_NOFADE)
					smi->m_alpha = 255;
				else
					smi->m_alpha = 0;
			}

			if(CanRemoveModel(streamId))
				ms_aInfoForModel[streamId].AddToList(&ms_startLoadedList);
		}
	}else if(streamId >= STREAM_OFFSET_TXD && streamId < STREAM_OFFSET_COL ||
	         streamId >= STREAM_OFFSET_ANIM){
		assert(streamId < NUMSTREAMINFO);
		// Txd and anims
		if(CanRemoveModel(streamId))
			ms_aInfoForModel[streamId].AddToList(&ms_startLoadedList);
	}

	// Mark objects as loaded
	if(ms_aInfoForModel[streamId].m_loadState != STREAMSTATE_STARTED){
		ms_aInfoForModel[streamId].m_loadState = STREAMSTATE_LOADED;
#ifndef USE_CUSTOM_ALLOCATOR
		ms_memoryUsed += ms_aInfoForModel[streamId].GetCdSize() * CDSTREAM_SECTOR_SIZE;
#endif
	}

	endTime = CTimer::GetCurrentTimeInCycles() / CTimer::GetCyclesPerMillisecond();
	timeDiff = endTime - startTime;
	if(timeDiff > 5)
		debug("%s took %d ms\n", GetObjectName(streamId), timeDiff);

	return true;
}

bool
CStreaming::FinishLoadingLargeFile(int8 *buf, int32 streamId)
{
	RwMemory mem;
	RwStream *stream;
	uint32 startTime, endTime, timeDiff;
	CBaseModelInfo *mi;
	bool success;

	startTime = CTimer::GetCurrentTimeInCycles() / CTimer::GetCyclesPerMillisecond();

	if(ms_aInfoForModel[streamId].m_loadState != STREAMSTATE_STARTED){
		if(streamId < STREAM_OFFSET_TXD)
			CModelInfo::GetModelInfo(streamId)->RemoveRef();
		return false;
	}

	mem.start = (uint8*)buf;
	mem.length = ms_aInfoForModel[streamId].GetCdSize() * CDSTREAM_SECTOR_SIZE;
	stream = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &mem);

	if(streamId < STREAM_OFFSET_TXD){
		// Model
		mi = CModelInfo::GetModelInfo(streamId);
		PUSH_MEMID(MEMID_STREAM_MODELS);
		CTxdStore::SetCurrentTxd(mi->GetTxdSlot());
		success = CFileLoader::FinishLoadClumpFile(stream, streamId);
		if(success){
#ifdef USE_CUSTOM_ALLOCATOR
			RpClumpForAllAtomics((RpClump*)mi->GetRwObject(), RegisterAtomicMemPtrsCB, nil);
#endif
			success = AddToLoadedVehiclesList(streamId);
		}
		POP_MEMID();
		mi->RemoveRef();
		CTxdStore::RemoveRefWithoutDelete(mi->GetTxdSlot());
#if GTA_VERSION > GTAVC_PS2
		if(mi->GetAnimFileIndex() != -1)
			CAnimManager::RemoveAnimBlockRefWithoutDelete(mi->GetAnimFileIndex());
#endif
	}else if(streamId >= STREAM_OFFSET_TXD && streamId < STREAM_OFFSET_COL){
		// Txd
		CTxdStore::AddRef(streamId - STREAM_OFFSET_TXD);
		PUSH_MEMID(MEMID_STREAM_TEXUTRES);
		success = CTxdStore::FinishLoadTxd(streamId - STREAM_OFFSET_TXD, stream);
		POP_MEMID();
		CTxdStore::RemoveRefWithoutDelete(streamId - STREAM_OFFSET_TXD);
	}else{
		assert(0 && "invalid streamId");
	}

	RwStreamClose(stream, &mem);

	ms_aInfoForModel[streamId].m_loadState = STREAMSTATE_LOADED;
#ifndef USE_CUSTOM_ALLOCATOR
	ms_memoryUsed += ms_aInfoForModel[streamId].GetCdSize() * CDSTREAM_SECTOR_SIZE;
#endif

	if(!success){
		RemoveModel(streamId);
		ReRequestModel(streamId);
		UpdateMemoryUsed();
		return false;
	}

	UpdateMemoryUsed();

	endTime = CTimer::GetCurrentTimeInCycles() / CTimer::GetCyclesPerMillisecond();
	timeDiff = endTime - startTime;
	if(timeDiff > 5)
		debug("%s took %d ms\n", GetObjectName(streamId), timeDiff);

	return true;
}

void
CStreaming::RequestModel(int32 id, int32 flags)
{
	CSimpleModelInfo *mi;

	if(ms_aInfoForModel[id].m_loadState == STREAMSTATE_INQUEUE){
		// updgrade to priority
		if(flags & STREAMFLAGS_PRIORITY && !ms_aInfoForModel[id].IsPriority()){
			ms_numPriorityRequests++;
			ms_aInfoForModel[id].m_flags |= STREAMFLAGS_PRIORITY;
		}
	}else if(ms_aInfoForModel[id].m_loadState != STREAMSTATE_NOTLOADED){
		flags &= ~STREAMFLAGS_PRIORITY;
	}
	ms_aInfoForModel[id].m_flags |= flags;

	if(ms_aInfoForModel[id].m_loadState == STREAMSTATE_LOADED){
		// Already loaded, only check changed flags

		if(ms_aInfoForModel[id].m_flags & STREAMFLAGS_NOFADE && id < STREAM_OFFSET_TXD){
			mi = (CSimpleModelInfo*)CModelInfo::GetModelInfo(id);
			if(mi->IsSimple())
				mi->m_alpha = 255;
		}

		// reinsert into list
		if(ms_aInfoForModel[id].m_next){
			ms_aInfoForModel[id].RemoveFromList();
			if(CanRemoveModel(id))
				ms_aInfoForModel[id].AddToList(&ms_startLoadedList);
		}
	}else if(ms_aInfoForModel[id].m_loadState == STREAMSTATE_NOTLOADED ||
	         ms_aInfoForModel[id].m_loadState == STREAMSTATE_LOADED){	// how can this be true again?

		if(ms_aInfoForModel[id].m_loadState == STREAMSTATE_NOTLOADED){
			if(id < STREAM_OFFSET_TXD){
				mi = (CSimpleModelInfo*)CModelInfo::GetModelInfo(id);
				RequestTxd(mi->GetTxdSlot(), flags);
				int anim = mi->GetAnimFileIndex();
				if(anim != -1)
					RequestAnim(anim, STREAMFLAGS_DEPENDENCY);
			}
			ms_aInfoForModel[id].AddToList(&ms_startRequestedList);
			ms_numModelsRequested++;
			if(flags & STREAMFLAGS_PRIORITY)
				ms_numPriorityRequests++;
		}

		ms_aInfoForModel[id].m_loadState = STREAMSTATE_INQUEUE;
		ms_aInfoForModel[id].m_flags = flags;
	}
}

#define BIGBUILDINGFLAGS STREAMFLAGS_DONT_REMOVE

void
CStreaming::RequestBigBuildings(eLevelName level)
{
	int i, n;
	CBuilding *b;

	n = CPools::GetBuildingPool()->GetSize()-1;
	for(i = n; i >= 0; i--){
		b = CPools::GetBuildingPool()->GetSlot(i);
		if(b && b->bIsBIGBuilding
#ifdef NO_ISLAND_LOADING
		   && (((FrontEndMenuManager.m_PrefsIslandLoading != CMenuManager::ISLAND_LOADING_LOW) && (b != pIslandLODmainlandEntity) &&
		        (b != pIslandLODbeachEntity)) ||
		       (b->m_level == level))
#else
		   && b->m_level == level
#endif
			)
			if(!b->bStreamBIGBuilding)
				RequestModel(b->GetModelIndex(), BIGBUILDINGFLAGS);
	}
	RequestIslands(level);
}

void
CStreaming::RequestBigBuildings(eLevelName level, const CVector &pos)
{
	int i, n;
	CBuilding *b;

	n = CPools::GetBuildingPool()->GetSize()-1;
	for(i = n; i >= 0; i--){
		b = CPools::GetBuildingPool()->GetSlot(i);
		if(b && b->bIsBIGBuilding
#ifdef NO_ISLAND_LOADING
		    && (((FrontEndMenuManager.m_PrefsIslandLoading != CMenuManager::ISLAND_LOADING_LOW) && (b != pIslandLODmainlandEntity) && (b != pIslandLODbeachEntity)
				) || (b->m_level == level))
#else
		   && b->m_level == level
#endif
		)
			if(b->bStreamBIGBuilding){
				if(CRenderer::ShouldModelBeStreamed(b, pos))
					RequestModel(b->GetModelIndex(), 0);
			}else
				RequestModel(b->GetModelIndex(), BIGBUILDINGFLAGS);
	}
	RequestIslands(level);
}

void
CStreaming::InstanceBigBuildings(eLevelName level, const CVector &pos)
{
	int i, n;
	CBuilding *b;

	n = CPools::GetBuildingPool()->GetSize()-1;
	for(i = n; i >= 0; i--){
		b = CPools::GetBuildingPool()->GetSlot(i);
		if(b && b->bIsBIGBuilding && b->m_level == level &&
		   b->bStreamBIGBuilding && b->m_rwObject == nil)
			if(CRenderer::ShouldModelBeStreamed(b, pos))
				b->CreateRwObject();
	}
}

void
CStreaming::InstanceLoadedModelsInSectorList(CPtrList &list)
{
	CPtrNode *node;
	CEntity *e;
	for(node = list.first; node; node = node->next) {
		e = (CEntity *)node->item;
		if(IsAreaVisible(e->m_area) && e->m_rwObject == nil)
			e->CreateRwObject();
	}
}

void
CStreaming::InstanceLoadedModels(const CVector &pos)
{
	int minX = CWorld::GetSectorIndexX(pos.x - 80.0f);
	if(minX <= 0) minX = 0;

	int minY = CWorld::GetSectorIndexY(pos.y - 80.0f);
	if(minY <= 0) minY = 0;

	int maxX = CWorld::GetSectorIndexX(pos.x + 80.0f);
	if(maxX >= NUMSECTORS_X) maxX = NUMSECTORS_X - 1;

	int maxY = CWorld::GetSectorIndexY(pos.y + 80.0f);
	if(maxY >= NUMSECTORS_Y) maxY = NUMSECTORS_Y - 1;

	int x, y;
	for(y = minY; y <= maxY; y++){
		for(x = minX; x <= maxX; x++){
			CSector *sector = CWorld::GetSector(x, y);
			InstanceLoadedModelsInSectorList(sector->m_lists[ENTITYLIST_BUILDINGS]);
			InstanceLoadedModelsInSectorList(sector->m_lists[ENTITYLIST_BUILDINGS_OVERLAP]);
			InstanceLoadedModelsInSectorList(sector->m_lists[ENTITYLIST_OBJECTS]);
			InstanceLoadedModelsInSectorList(sector->m_lists[ENTITYLIST_DUMMIES]);
		}
	}
}

void
CStreaming::RequestIslands(eLevelName level)
{
	ISLAND_LOADING_ISNT(HIGH)
	switch(level){
	case LEVEL_MAINLAND:
		if(islandLODbeach != -1)
			RequestModel(islandLODbeach, BIGBUILDINGFLAGS);
		break;
	case LEVEL_BEACH:
		if(islandLODmainland != -1)
			RequestModel(islandLODmainland, BIGBUILDINGFLAGS);
		break;
	default: break;
	}
}

static char *IGnames[] = {
	"player",
	"player2",
	"player3",
	"player4",
	"player5",
	"player6",
	"player7",
	"player8",
	"player9",
	"play10",
	"play11",
	"igken",
	"igcandy",
	"igsonny",
	"igbuddy",
	"igjezz",
	"ighlary",
	"igphil",
	"igmerc",
	"igdick",
	"igdiaz",
	""
};

static char *CSnames[] = {
	"csplay",
	"csplay2",
	"csplay3",
	"csplay4",
	"csplay5",
	"csplay6",
	"csplay7",
	"csplay8",
	"csplay9",
	"csplay10",
	"csplay11",
	"csken",
	"cscandy",
	"cssonny",
	"csbuddy",
	"csjezz",
	"cshlary",
	"csphil",
	"csmerc",
	"csdick",
	"csdiaz",
	""
};

void
CStreaming::RequestSpecialModel(int32 modelId, const char *modelName, int32 flags)
{
	CBaseModelInfo *mi;
	int txdId;
	char oldName[48];
	uint32 pos, size;
	int i, n;

	mi = CModelInfo::GetModelInfo(modelId);
	if(strncasecmp("CSPlay", modelName, 6) == 0){
		char *curname = CModelInfo::GetModelInfo(MI_PLAYER)->GetModelName();
		for(int i = 0; CSnames[i][0]; i++){
			if(strcasecmp(curname, IGnames[i]) == 0){
				modelName = CSnames[i];
				break;
			}
		}
	}
	if(!CGeneral::faststrcmp(mi->GetModelName(), modelName)){
		// Already have the correct name, just request it
		RequestModel(modelId, flags);
		return;
	}

	if(mi->GetNumRefs() > 0){
		n = CPools::GetPedPool()->GetSize()-1;
		for(i = n; i >= 0 && mi->GetNumRefs() > 0; i--){
			CPed *ped = CPools::GetPedPool()->GetSlot(i);
			if(ped && ped->GetModelIndex() == modelId &&
			   !ped->IsPlayer() && ped->CanBeDeletedEvenInVehicle())
				CTheScripts::RemoveThisPed(ped);
		}
		n = CPools::GetObjectPool()->GetSize()-1;
		for(i = n; i >= 0 && mi->GetNumRefs() > 0; i--){
			CObject *obj = CPools::GetObjectPool()->GetSlot(i);
			if(obj && obj->GetModelIndex() == modelId && obj->CanBeDeleted()){
				CWorld::Remove(obj);
				CWorld::RemoveReferencesToDeletedObject(obj);
				delete obj;
			}
		}
	}

	strcpy(oldName, mi->GetModelName());
	mi->SetModelName(modelName);

	// What exactly is going on here?
	if(CModelInfo::GetModelInfo(oldName, nil)){
		txdId = CTxdStore::FindTxdSlot(oldName);
		if(txdId != -1 && CTxdStore::GetSlot(txdId)->texDict){
			CTxdStore::AddRef(txdId);
			RemoveModel(modelId);
			CTxdStore::RemoveRefWithoutDelete(txdId);
		}else
			RemoveModel(modelId);
	}else
		RemoveModel(modelId);

	bool found = ms_pExtraObjectsDir->FindItem(modelName, pos, size);
	assert(found);
	mi->ClearTexDictionary();
	if(CTxdStore::FindTxdSlot(modelName) == -1)
		mi->SetTexDictionary("generic");
	else
		mi->SetTexDictionary(modelName);
	ms_aInfoForModel[modelId].SetCdPosnAndSize(pos, size);
	RequestModel(modelId, flags);
}

void
CStreaming::RequestSpecialChar(int32 charId, const char *modelName, int32 flags)
{
	RequestSpecialModel(charId + MI_SPECIAL01, modelName, flags);
}

bool
CStreaming::HasSpecialCharLoaded(int32 id)
{
	return HasModelLoaded(id + MI_SPECIAL01);
}

void
CStreaming::SetMissionDoesntRequireSpecialChar(int32 id)
{
	return SetMissionDoesntRequireModel(id + MI_SPECIAL01);
}

void
CStreaming::DecrementRef(int32 id)
{
	ms_numModelsRequested--;
	if(ms_aInfoForModel[id].IsPriority()){
		ms_aInfoForModel[id].m_flags &= ~STREAMFLAGS_PRIORITY;
		ms_numPriorityRequests--;
	}
}

void
CStreaming::RemoveModel(int32 id)
{
	int i;

	if(ms_aInfoForModel[id].m_loadState == STREAMSTATE_NOTLOADED)
		return;

	if(ms_aInfoForModel[id].m_loadState == STREAMSTATE_LOADED){
		if(id < STREAM_OFFSET_TXD)
			CModelInfo::GetModelInfo(id)->DeleteRwObject();
		else if(id >= STREAM_OFFSET_TXD && id < STREAM_OFFSET_COL)
			CTxdStore::RemoveTxd(id - STREAM_OFFSET_TXD);
		else if(id >= STREAM_OFFSET_COL && id < STREAM_OFFSET_ANIM)
			CColStore::RemoveCol(id - STREAM_OFFSET_COL);
		else if(id >= STREAM_OFFSET_ANIM){
			assert(id < NUMSTREAMINFO);
			CAnimManager::RemoveAnimBlock(id - STREAM_OFFSET_ANIM);
		}
		ms_memoryUsed -= ms_aInfoForModel[id].GetCdSize()*CDSTREAM_SECTOR_SIZE;
	}

	if(ms_aInfoForModel[id].m_next){
		// Remove from list, model is neither loaded nor requested
		if(ms_aInfoForModel[id].m_loadState == STREAMSTATE_INQUEUE)
			DecrementRef(id);
		ms_aInfoForModel[id].RemoveFromList();
	}else if(ms_aInfoForModel[id].m_loadState == STREAMSTATE_READING){
		for(i = 0; i < 4; i++){
			if(ms_channel[0].streamIds[i] == id)
				ms_channel[0].streamIds[i] = -1;
			if(ms_channel[1].streamIds[i] == id)
				ms_channel[1].streamIds[i] = -1;
		}
	}

	if(ms_aInfoForModel[id].m_loadState == STREAMSTATE_STARTED){
		if(id < STREAM_OFFSET_TXD)
			RpClumpGtaCancelStream();
		else if(id >= STREAM_OFFSET_TXD && id < STREAM_OFFSET_COL)
			CTxdStore::RemoveTxd(id - STREAM_OFFSET_TXD);
		else if(id >= STREAM_OFFSET_COL && id < STREAM_OFFSET_ANIM)
			CColStore::RemoveCol(id - STREAM_OFFSET_COL);
		else if(id >= STREAM_OFFSET_ANIM){
			assert(id < NUMSTREAMINFO);
			CAnimManager::RemoveAnimBlock(id - STREAM_OFFSET_ANIM);
		}
	}

	ms_aInfoForModel[id].m_loadState = STREAMSTATE_NOTLOADED;
}

void
CStreaming::RemoveUnusedBuildings(eLevelName level)
{
	if(level != LEVEL_BEACH)
		RemoveBuildings(LEVEL_BEACH);
	if(level != LEVEL_MAINLAND)
		RemoveBuildings(LEVEL_MAINLAND);
}

void
CStreaming::RemoveBuildings(eLevelName level)
{
	int i, n;
	CEntity *e;
	CBaseModelInfo *mi;

	n = CPools::GetBuildingPool()->GetSize()-1;
	for(i = n; i >= 0; i--){
		e = CPools::GetBuildingPool()->GetSlot(i);
		if(e && e->m_level == level){
			mi = CModelInfo::GetModelInfo(e->GetModelIndex());
			if(!e->bImBeingRendered){
				e->DeleteRwObject();
				if (mi->GetNumRefs() == 0)
					RemoveModel(e->GetModelIndex());
			}
		}
	}

	n = CPools::GetTreadablePool()->GetSize()-1;
	for(i = n; i >= 0; i--){
		e = CPools::GetTreadablePool()->GetSlot(i);
		if(e && e->m_level == level){
			mi = CModelInfo::GetModelInfo(e->GetModelIndex());
			if(!e->bImBeingRendered){
				e->DeleteRwObject();
				if (mi->GetNumRefs() == 0)
					RemoveModel(e->GetModelIndex());
			}
		}
	}

	n = CPools::GetObjectPool()->GetSize()-1;
	for(i = n; i >= 0; i--){
		e = CPools::GetObjectPool()->GetSlot(i);
		if(e && e->m_level == level){
			mi = CModelInfo::GetModelInfo(e->GetModelIndex());
			if(!e->bImBeingRendered && ((CObject*)e)->ObjectCreatedBy == GAME_OBJECT){
				e->DeleteRwObject();
				if (mi->GetNumRefs() == 0)
					RemoveModel(e->GetModelIndex());
			}
		}
	}

	n = CPools::GetDummyPool()->GetSize()-1;
	for(i = n; i >= 0; i--){
		e = CPools::GetDummyPool()->GetSlot(i);
		if(e && e->m_level == level){
			mi = CModelInfo::GetModelInfo(e->GetModelIndex());
			if(!e->bImBeingRendered){
				e->DeleteRwObject();
				if (mi->GetNumRefs() == 0)
					RemoveModel(e->GetModelIndex());
			}
		}
	}
}

void
CStreaming::RemoveBuildingsNotInArea(int32 area)
{
	int i, n;
	CEntity *e;

	n = CPools::GetBuildingPool()->GetSize()-1;
	for(i = n; i >= 0; i--){
		e = CPools::GetBuildingPool()->GetSlot(i);
		if(e && e->m_rwObject && !IsAreaVisible(area) &&
		   (!e->bIsBIGBuilding || e->bStreamBIGBuilding)){
			if(e->bIsBIGBuilding)
				RequestModel(e->GetModelIndex(), 0);
			if(!e->bImBeingRendered)
				e->DeleteRwObject();
		}
	}

	n = CPools::GetTreadablePool()->GetSize()-1;
	for(i = n; i >= 0; i--){
		e = CPools::GetTreadablePool()->GetSlot(i);
		if(e && e->m_rwObject && !IsAreaVisible(area) &&
		   (!e->bIsBIGBuilding || e->bStreamBIGBuilding)){
			if(e->bIsBIGBuilding)
				RequestModel(e->GetModelIndex(), 0);
			if(!e->bImBeingRendered)
				e->DeleteRwObject();
		}
	}

	n = CPools::GetObjectPool()->GetSize()-1;
	for(i = n; i >= 0; i--){
		e = CPools::GetObjectPool()->GetSlot(i);
		if(e && e->m_rwObject && !IsAreaVisible(area) &&
		   (!e->bIsBIGBuilding || e->bStreamBIGBuilding)){
			if(e->bIsBIGBuilding)
				RequestModel(e->GetModelIndex(), 0);
			if(!e->bImBeingRendered)
				e->DeleteRwObject();
		}
	}

	n = CPools::GetDummyPool()->GetSize()-1;
	for(i = n; i >= 0; i--){
		e = CPools::GetDummyPool()->GetSlot(i);
		if(e && e->m_rwObject && !IsAreaVisible(area) &&
		   (!e->bIsBIGBuilding || e->bStreamBIGBuilding)){
			if(e->bIsBIGBuilding)
				RequestModel(e->GetModelIndex(), 0);
			if(!e->bImBeingRendered)
				e->DeleteRwObject();
		}
	}
}

void
CStreaming::RemoveUnusedBigBuildings(eLevelName level)
{
	ISLAND_LOADING_IS(LOW)
	{
	if(level != LEVEL_BEACH)
		RemoveBigBuildings(LEVEL_BEACH);
	if(level != LEVEL_MAINLAND)
		RemoveBigBuildings(LEVEL_MAINLAND);
	}
	RemoveIslandsNotUsed(level);
}

void
DeleteIsland(CEntity *island)
{
	if(island == nil)
		return;
	if(island->bImBeingRendered)
		debug("Didn't delete island because it was being rendered\n");
	else{
		island->DeleteRwObject();
		CStreaming::RemoveModel(island->GetModelIndex());
	}
}

void
CStreaming::RemoveIslandsNotUsed(eLevelName level)
{
	int i;
	if(pIslandLODmainlandEntity == nil)
	for(i = CPools::GetBuildingPool()->GetSize()-1; i >= 0; i--){
		CBuilding *building = CPools::GetBuildingPool()->GetSlot(i);
		if(building == nil)
			continue;
		if(building->GetModelIndex() == islandLODmainland)
			pIslandLODmainlandEntity = building;
		if(building->GetModelIndex() == islandLODbeach)
			pIslandLODbeachEntity = building;
	}
#ifdef NO_ISLAND_LOADING
	if(FrontEndMenuManager.m_PrefsIslandLoading == CMenuManager::ISLAND_LOADING_HIGH) {
		DeleteIsland(pIslandLODmainlandEntity);
		DeleteIsland(pIslandLODbeachEntity);
	} else
#endif
	switch(level){
	case LEVEL_MAINLAND:
		DeleteIsland(pIslandLODmainlandEntity);
		break;
	case LEVEL_BEACH:
		DeleteIsland(pIslandLODbeachEntity);

		break;
	}
}

void
CStreaming::RemoveBigBuildings(eLevelName level)
{
	int i, n;
	CEntity *e;
	CBaseModelInfo *mi;

	n = CPools::GetBuildingPool()->GetSize()-1;
	for(i = n; i >= 0; i--){
		e = CPools::GetBuildingPool()->GetSlot(i);
		if(e && e->bIsBIGBuilding && e->m_level == level){
			mi = CModelInfo::GetModelInfo(e->GetModelIndex());
			if(!e->bImBeingRendered){
				e->DeleteRwObject();
				if (mi->GetNumRefs() == 0)
					RemoveModel(e->GetModelIndex());
			}
		}
	}
}

bool
CStreaming::RemoveLoadedVehicle(void)
{
	int i, id;

	for(i = 0; i < MAXVEHICLESLOADED; i++){
		ms_lastVehicleDeleted++;
		if(ms_lastVehicleDeleted == MAXVEHICLESLOADED)
			ms_lastVehicleDeleted = 0;
		id = ms_vehiclesLoaded[ms_lastVehicleDeleted];
		if(id != -1 && CanRemoveModel(id) && CModelInfo::GetModelInfo(id)->GetNumRefs() == 0 &&
		   ms_aInfoForModel[id].m_loadState == STREAMSTATE_LOADED)
			goto found;
	}
	return false;
found:
	RemoveModel(ms_vehiclesLoaded[ms_lastVehicleDeleted]);
	ms_numVehiclesLoaded--;
	ms_vehiclesLoaded[ms_lastVehicleDeleted] = -1;
	CVehicleModelInfo* pVehicleInfo = (CVehicleModelInfo*)CModelInfo::GetModelInfo(id);
	if (pVehicleInfo->m_vehicleClass != -1)
		CCarCtrl::RemoveFromLoadedVehicleArray(id, pVehicleInfo->m_vehicleClass);
	return true;
}

bool
CStreaming::RemoveLeastUsedModel(uint32 excludeMask)
{
	CStreamingInfo *si;
	int streamId;

	for(si = ms_endLoadedList.m_prev; si != &ms_startLoadedList; si = si->m_prev){
		if(si->m_flags & excludeMask)
			continue;
		streamId = si - ms_aInfoForModel;
		if(streamId < STREAM_OFFSET_TXD){
			if (CModelInfo::GetModelInfo(streamId)->GetNumRefs() == 0) {
				RemoveModel(streamId);
				return true;
			}
		}else if(streamId >= STREAM_OFFSET_TXD && streamId < STREAM_OFFSET_COL){
			if(CTxdStore::GetNumRefs(streamId - STREAM_OFFSET_TXD) == 0 &&
			   !IsTxdUsedByRequestedModels(streamId - STREAM_OFFSET_TXD)){
				RemoveModel(streamId);
				return true;
			}
		}else if(streamId >= STREAM_OFFSET_ANIM){
			assert(streamId < NUMSTREAMINFO);
			if(CAnimManager::GetNumRefsToAnimBlock(streamId - STREAM_OFFSET_ANIM) == 0 &&
			   !AreAnimsUsedByRequestedModels(streamId - STREAM_OFFSET_ANIM)){
				RemoveModel(streamId);
				return true;
			}
		}
	}
	return (ms_numVehiclesLoaded > 7 || CGame::currArea != AREA_MAIN_MAP && ms_numVehiclesLoaded > 4) && RemoveLoadedVehicle();
}

void
CStreaming::RemoveAllUnusedModels(void)
{
	int i;

	for(i = 0; i < MAXVEHICLESLOADED; i++)
		RemoveLoadedVehicle();

	for(i = NUM_DEFAULT_MODELS; i < MODELINFOSIZE; i++){
		if(ms_aInfoForModel[i].m_loadState == STREAMSTATE_LOADED &&
		    CModelInfo::GetModelInfo(i)->GetNumRefs() == 0) {
			RemoveModel(i);
			ms_aInfoForModel[i].m_loadState = STREAMSTATE_NOTLOADED;
		}
	}
}

void
CStreaming::RemoveUnusedModelsInLoadedList(void)
{
	// empty
}

bool
CStreaming::RemoveLoadedZoneModel(void)
{
	int i;

	if(ms_currentPedGrp == -1)
		return false;

	for(i = 0; i < NUMMODELSPERPEDGROUP; i++){
		int mi = CPopulation::ms_pPedGroups[ms_currentPedGrp].models[i];
		if(mi != -1 && ms_bIsPedFromPedGroupLoaded[i] &&
		   HasModelLoaded(mi) &&  CanRemoveModel(mi) &&
		   CModelInfo::GetModelInfo(mi)->GetNumRefs() == 0){
			RemoveModel(CPopulation::ms_pPedGroups[ms_currentPedGrp].models[i]);
			ms_numPedsLoaded--;
			ms_bIsPedFromPedGroupLoaded[i] = false;
			return true;
		}
	}

	return false;
}

bool
CStreaming::IsTxdUsedByRequestedModels(int32 txdId)
{
	CStreamingInfo *si;
	int streamId;
	int i;

	for(si = ms_startRequestedList.m_next; si != &ms_endRequestedList; si = si->m_next){
		streamId = si - ms_aInfoForModel;
		if(streamId < STREAM_OFFSET_TXD &&
		   CModelInfo::GetModelInfo(streamId)->GetTxdSlot() == txdId)
			return true;
	}

	for(i = 0; i < 4; i++){
		streamId = ms_channel[0].streamIds[i];
		if(streamId != -1 && streamId < STREAM_OFFSET_TXD &&
		   CModelInfo::GetModelInfo(streamId)->GetTxdSlot() == txdId)
			return true;
		streamId = ms_channel[1].streamIds[i];
		if(streamId != -1 && streamId < STREAM_OFFSET_TXD &&
		   CModelInfo::GetModelInfo(streamId)->GetTxdSlot() == txdId)
			return true;
	}

	return false;
}

bool
CStreaming::AreAnimsUsedByRequestedModels(int32 animId)
{
	CStreamingInfo *si;
	int streamId;
	int i;

	for(si = ms_startRequestedList.m_next; si != &ms_endRequestedList; si = si->m_next){
		streamId = si - ms_aInfoForModel;
		if(streamId < STREAM_OFFSET_TXD &&
		   CModelInfo::GetModelInfo(streamId)->GetAnimFileIndex() == animId)
			return true;
	}

	for(i = 0; i < 4; i++){
		streamId = ms_channel[0].streamIds[i];
		if(streamId != -1 && streamId < STREAM_OFFSET_TXD &&
		   CModelInfo::GetModelInfo(streamId)->GetAnimFileIndex() == animId)
			return true;
		streamId = ms_channel[1].streamIds[i];
		if(streamId != -1 && streamId < STREAM_OFFSET_TXD &&
		   CModelInfo::GetModelInfo(streamId)->GetAnimFileIndex() == animId)
			return true;
	}

	return false;
}

int32
CStreaming::GetAvailableVehicleSlot(void)
{
	int i;
	for(i = 0; i < MAXVEHICLESLOADED; i++)
		if(ms_vehiclesLoaded[i] == -1)
			return i;
	return -1;
}

bool
CStreaming::AddToLoadedVehiclesList(int32 modelId)
{
	int i;
	int id;

	if(ms_numVehiclesLoaded < desiredNumVehiclesLoaded){
		// still room for vehicles
		for(i = 0; i < MAXVEHICLESLOADED; i++){
			if(ms_vehiclesLoaded[ms_lastVehicleDeleted] == -1)
				break;
			ms_lastVehicleDeleted++;
			if(ms_lastVehicleDeleted == MAXVEHICLESLOADED)
				ms_lastVehicleDeleted = 0;
		}
		assert(ms_vehiclesLoaded[ms_lastVehicleDeleted] == -1);
		ms_numVehiclesLoaded++;
	}else{
		// find vehicle we can remove
		for(i = 0; i < MAXVEHICLESLOADED; i++){
			id = ms_vehiclesLoaded[ms_lastVehicleDeleted];
			if(id != -1 && CanRemoveModel(id) &&
			   CModelInfo::GetModelInfo(id)->GetNumRefs() == 0)
				goto found;
			ms_lastVehicleDeleted++;
			if(ms_lastVehicleDeleted == MAXVEHICLESLOADED)
				ms_lastVehicleDeleted = 0;
		}
		id = -1;
found:
		if(id == -1){
			// didn't find anything, try a free slot
			id = GetAvailableVehicleSlot();
			if(id == -1)
				return false;	// still no luck
			ms_lastVehicleDeleted = id;
			// this is more than we wanted actually
			ms_numVehiclesLoaded++;
		}
		else{
			RemoveModel(id);
			CVehicleModelInfo* pVehicleInfo = (CVehicleModelInfo*)CModelInfo::GetModelInfo(id);
			if (pVehicleInfo->m_vehicleClass != -1)
				CCarCtrl::RemoveFromLoadedVehicleArray(id, pVehicleInfo->m_vehicleClass);
		}
	}

	ms_vehiclesLoaded[ms_lastVehicleDeleted++] = modelId;
	if(ms_lastVehicleDeleted == MAXVEHICLESLOADED)
		ms_lastVehicleDeleted = 0;
	CVehicleModelInfo* pVehicleInfo = (CVehicleModelInfo*)CModelInfo::GetModelInfo(modelId);
	if (pVehicleInfo->m_vehicleClass != -1)
		CCarCtrl::AddToLoadedVehicleArray(modelId, pVehicleInfo->m_vehicleClass, pVehicleInfo->m_frequency);
#ifdef __EMSCRIPTEN__
	// D4 (sección 1): un modelo de vehículo acaba de entrar en el fondo cargado
	// (el que usa `ChooseCarModel` para el tráfico). Emparejado con `CARPED`
	// (petición) dice si una petición se quedó sin cargar: petición sin CARLOAD
	// = su lectura falló (ver `CARFAIL`/`ODSHORT`).
	{
		static int odCargados = 0;
		if (odCargados < 400) {
			char tt[200];
			if (pVehicleInfo->m_vehicleClass != -1)
				snprintf(tt, sizeof tt, "CARLOAD model=%d clase=%d freq=%d fondo=%d/%d",
					modelId, pVehicleInfo->m_vehicleClass, pVehicleInfo->m_frequency,
					CCarCtrl::NumOfLoadedCarsOfRating[pVehicleInfo->m_vehicleClass],
					CCarCtrl::TotalNumOfCarsOfRating[pVehicleInfo->m_vehicleClass]);
			else
				snprintf(tt, sizeof tt, "CARLOAD model=%d clase=-1 (ignore) freq=%d",
					modelId, pVehicleInfo->m_frequency);
			ODTRACES(tt);
			odCargados++;
		}
	}
#endif
	return true;
}

bool
CStreaming::IsObjectInCdImage(int32 id)
{
	uint32 posn, size;
	return ms_aInfoForModel[id].GetCdPosnAndSize(posn, size);
}

void
CStreaming::SetModelIsDeletable(int32 id)
{
	ms_aInfoForModel[id].m_flags &= ~STREAMFLAGS_DONT_REMOVE;
	if ((id >= STREAM_OFFSET_TXD && id < STREAM_OFFSET_COL || CModelInfo::GetModelInfo(id)->GetModelType() != MITYPE_VEHICLE) &&
	   (ms_aInfoForModel[id].m_flags & STREAMFLAGS_SCRIPTOWNED) == 0){
		if(ms_aInfoForModel[id].m_loadState != STREAMSTATE_LOADED)
			RemoveModel(id);
		else if(ms_aInfoForModel[id].m_next == nil)
			ms_aInfoForModel[id].AddToList(&ms_startLoadedList);
	}
}

void
CStreaming::SetModelTxdIsDeletable(int32 id)
{
	SetModelIsDeletable(CModelInfo::GetModelInfo(id)->GetTxdSlot() + STREAM_OFFSET_TXD);
}

void
CStreaming::SetMissionDoesntRequireModel(int32 id)
{
	ms_aInfoForModel[id].m_flags &= ~STREAMFLAGS_SCRIPTOWNED;
	if ((id >= STREAM_OFFSET_TXD || CModelInfo::GetModelInfo(id)->GetModelType() != MITYPE_VEHICLE) &&
	   (ms_aInfoForModel[id].m_flags & STREAMFLAGS_DONT_REMOVE) == 0){
		if(ms_aInfoForModel[id].m_loadState != STREAMSTATE_LOADED)
			RemoveModel(id);
		else if(ms_aInfoForModel[id].m_next == nil)
			ms_aInfoForModel[id].AddToList(&ms_startLoadedList);
	}
}

void
CStreaming::LoadInitialPeds(void)
{
	RequestModel(MI_COP, STREAMFLAGS_DONT_REMOVE);
	RequestModel(MI_MALE01, STREAMFLAGS_DONT_REMOVE);
	RequestModel(MI_TAXI_D, STREAMFLAGS_DONT_REMOVE);
}

void
CStreaming::LoadInitialWeapons(void)
{
	CStreaming::RequestModel(MI_NIGHTSTICK, STREAMFLAGS_DONT_REMOVE);
	CStreaming::RequestModel(MI_MISSILE, STREAMFLAGS_DONT_REMOVE);
	// Vice Extended: el proyectil del lanzagranadas tampoco es el arma en
	// mano, así que va residente como el misil (ProjectileInfo lo instancia
	// de golpe al disparar).
	CStreaming::RequestModel(MI_GR_GRENADE, STREAMFLAGS_DONT_REMOVE);
}

void
CStreaming::LoadInitialVehicles(void)
{
	ms_numVehiclesLoaded = 0;
	ms_lastVehicleDeleted = 0;

	RequestModel(MI_POLICE, STREAMFLAGS_DONT_REMOVE);
}

void
CStreaming::StreamVehiclesAndPeds(void)
{
	int i, model;
	static int timeBeforeNextLoad = 0;
	static int modelQualityClass = 0;

	if(CRecordDataForGame::IsRecording() ||
	   CRecordDataForGame::IsPlayingBack()
#ifdef FIX_BUGS
	   || CReplay::IsPlayingBack()
#endif
		)
		return;

	if(FindPlayerPed()->m_pWanted->AreSwatRequired()){
		RequestModel(MI_ENFORCER, STREAMFLAGS_DONT_REMOVE);
		RequestModel(MI_SWAT, STREAMFLAGS_DONT_REMOVE);
	}else{
		SetModelIsDeletable(MI_ENFORCER);
		if(!HasModelLoaded(MI_ENFORCER))
			SetModelIsDeletable(MI_SWAT);
	}

	if(FindPlayerPed()->m_pWanted->AreFbiRequired()){
		RequestModel(MI_FBIRANCH, STREAMFLAGS_DONT_REMOVE);
		RequestModel(MI_FBI, STREAMFLAGS_DONT_REMOVE);
	}else{
		SetModelIsDeletable(MI_FBIRANCH);
		if(!HasModelLoaded(MI_FBIRANCH))
			SetModelIsDeletable(MI_FBI);
	}

	if(FindPlayerPed()->m_pWanted->AreArmyRequired()){
		RequestModel(MI_RHINO, STREAMFLAGS_DONT_REMOVE);
		RequestModel(MI_BARRACKS, STREAMFLAGS_DONT_REMOVE);
		RequestModel(MI_ARMY, STREAMFLAGS_DONT_REMOVE);
	}else{
		SetModelIsDeletable(MI_RHINO);
		SetModelIsDeletable(MI_BARRACKS);
		if(!HasModelLoaded(MI_RHINO) && !HasModelLoaded(MI_BARRACKS))
			SetModelIsDeletable(MI_ARMY);
	}

	if(FindPlayerPed()->m_pWanted->NumOfHelisRequired() > 0)
		RequestModel(MI_CHOPPER, STREAMFLAGS_DONT_REMOVE);
	else
		SetModelIsDeletable(MI_CHOPPER);

	if (FindPlayerPed()->m_pWanted->AreMiamiViceRequired()) {
		SetModelIsDeletable(MI_VICE1);
		SetModelIsDeletable(MI_VICE2);
		SetModelIsDeletable(MI_VICE3);
		SetModelIsDeletable(MI_VICE4);
		SetModelIsDeletable(MI_VICE5);
		SetModelIsDeletable(MI_VICE6);
		SetModelIsDeletable(MI_VICE7);
		SetModelIsDeletable(MI_VICE8);
		RequestModel(MI_VICECHEE, STREAMFLAGS_DONT_REMOVE);
		if(CPopulation::NumMiamiViceCops == 0)
			switch (CCarCtrl::MiamiViceCycle) {
			case 0:
				RequestModel(MI_VICE1, STREAMFLAGS_DONT_REMOVE);
				RequestModel(MI_VICE2, STREAMFLAGS_DONT_REMOVE);
				break;
			case 1:
				RequestModel(MI_VICE3, STREAMFLAGS_DONT_REMOVE);
				RequestModel(MI_VICE4, STREAMFLAGS_DONT_REMOVE);
				break;
			case 2:
				RequestModel(MI_VICE5, STREAMFLAGS_DONT_REMOVE);
				RequestModel(MI_VICE6, STREAMFLAGS_DONT_REMOVE);
				break;
			case 3:
				RequestModel(MI_VICE7, STREAMFLAGS_DONT_REMOVE);
				RequestModel(MI_VICE8, STREAMFLAGS_DONT_REMOVE);
				break;
			}
	}
	else {
		SetModelIsDeletable(MI_VICECHEE);
		SetModelIsDeletable(MI_VICE1);
		SetModelIsDeletable(MI_VICE2);
		SetModelIsDeletable(MI_VICE3);
		SetModelIsDeletable(MI_VICE4);
		SetModelIsDeletable(MI_VICE5);
		SetModelIsDeletable(MI_VICE6);
		SetModelIsDeletable(MI_VICE7);
		SetModelIsDeletable(MI_VICE8);
	}

#ifdef __EMSCRIPTEN__
	// D4 (sección 1): el temporizador de siempre cuenta FOTOGRAMAS (350 = 11,7 s a
	// 30 FPS). En el navegador, a 6-9 FPS, esos 350 fotogramas son 40-60 s reales:
	// la rotación de modelos del tráfico va 3-5 veces más lenta que en el juego
	// original y la calle se queda con los 2-3 modelos que ya tenía residentes
	// (medido: 6 min de calle → 2 modelos distintos, `oceanic` 28 veces).
	// Aquí se cuenta el MISMO presupuesto, pero en tiempo real (350/30 s).
	static uint32 odUltVehLoad = 0;
	bool odTocaCargar = (uint32)(CTimer::GetTimeInMilliseconds() - odUltVehLoad) >= 11666;
#else
	bool odTocaCargar = timeBeforeNextLoad < 0;
	if(!odTocaCargar)
		timeBeforeNextLoad--;
#endif
	if(odTocaCargar && ms_numVehiclesLoaded <= desiredNumVehiclesLoaded){
		CZoneInfo zone;
		CVector coors = FindPlayerCoors();
		CTheZones::GetZoneInfoForTimeOfDay(&coors, &zone);
		int32 maxReq = -1;
		int32 mostRequestedRating = 0;
		for(i = 0; i < CCarCtrl::TOTAL_CUSTOM_CLASSES; i++){
			if(CCarCtrl::NumRequestsOfCarRating[i] > maxReq &&
				((i == 0 && zone.carThreshold[0] != 0) ||
#ifdef FIX_BUGS
				(i < CCarCtrl::NUM_CAR_CLASSES && zone.carThreshold[i] != zone.carThreshold[i-1]) ||
				(i == CCarCtrl::NUM_CAR_CLASSES && zone.boatThreshold[i - CCarCtrl::NUM_CAR_CLASSES] != 0) ||
				(i > CCarCtrl::NUM_CAR_CLASSES && i < CCarCtrl::TOTAL_CUSTOM_CLASSES && zone.boatThreshold[i - CCarCtrl::NUM_CAR_CLASSES] != zone.boatThreshold[i - CCarCtrl::NUM_CAR_CLASSES - 1]))) {
#else
				(i != 0 && zone.carThreshold[i] != zone.carThreshold[i-1]))) {
#endif
				maxReq = CCarCtrl::NumRequestsOfCarRating[i];
				mostRequestedRating = i;
			}
		}
		model = CCarCtrl::ChooseCarModelToLoad(mostRequestedRating);
#ifdef __EMSCRIPTEN__
		{
			// D4 (sección 1): medición del cargador. `cargadas` es cuántos modelos
			// de esa clase hay YA residentes (de los que elige `ChooseCarModel`) y
			// `ya` si el elegido lo estaba: eso explica si el fondo se estanca
			// porque no se piden modelos nuevos o porque se repiten los cargados.
			static uint32 odVehReq = 0;
			char tt[240];
			snprintf(tt, sizeof tt, "CARPED n=%u rating=%d cargadas=%d/%d model=%d ya=%d freq=%d veh=%d/%d",
				(unsigned)++odVehReq, mostRequestedRating,
				CCarCtrl::NumOfLoadedCarsOfRating[mostRequestedRating],
				CCarCtrl::TotalNumOfCarsOfRating[mostRequestedRating],
				model, model >= 0 ? (int)HasModelLoaded(model) : -1,
				model >= 0 ? (int)((CVehicleModelInfo*)CModelInfo::GetModelInfo(model))->m_frequency : -1,
				ms_numVehiclesLoaded, desiredNumVehiclesLoaded);
			ODTRACES(tt);
			if(odVehReq % 10 == 1){
				int n = snprintf(tt, sizeof tt, "CARPOOL n=%u", (unsigned)odVehReq);
				for(int c = 0; c < CCarCtrl::TOTAL_CUSTOM_CLASSES && n > 0 && n < (int)sizeof(tt) - 16; c++){
					if(CCarCtrl::NumOfLoadedCarsOfRating[c] > 0)
						n += snprintf(tt + n, sizeof tt - n, " c%d=%d", c, CCarCtrl::NumOfLoadedCarsOfRating[c]);
				}
				ODTRACES(tt);
			}
		}
#endif
		if(!HasModelLoaded(model)){
			RequestModel(model, STREAMFLAGS_DEPENDENCY);
			timeBeforeNextLoad = 350;
#ifdef __EMSCRIPTEN__
			odUltVehLoad = CTimer::GetTimeInMilliseconds();
#endif
		}
		CCarCtrl::NumRequestsOfCarRating[mostRequestedRating] = 0;
	}
#ifdef __EMSCRIPTEN__
	else if(odTocaCargar){
		// D4 (sección 1): el turno llegó pero la piscina de vehículos ya está en
		// el tope, así que no se pide nada nuevo. Si esto domina, el fondo
		// cargado se congela y no es el temporizador el que lo explica.
		static uint32 odFull = 0;
		if(++odFull % 5 == 0){
			char tt[128];
			snprintf(tt, sizeof tt, "CARFULL n=%u veh=%d/%d", (unsigned)odFull,
				ms_numVehiclesLoaded, desiredNumVehiclesLoaded);
			ODTRACES(tt);
		}
	}
#endif
}

void
CStreaming::StreamZoneModels(const CVector &pos)
{
	int i, j;
	uint16 gangsToLoad, gangCarsToLoad, bit;
	CZoneInfo info;
	static int timeBeforeNextLoad = 0;

	CTheZones::GetZoneInfoForTimeOfDay(&pos, &info);

	if(info.pedGroup != ms_currentPedGrp){

		// unload pevious group
		if(ms_currentPedGrp != -1)
			for(i = 0; i < NUMMODELSPERPEDGROUP; i++){
				ms_bIsPedFromPedGroupLoaded[i] = false;
				if(CPopulation::ms_pPedGroups[ms_currentPedGrp].models[i] != -1){
					SetModelIsDeletable(CPopulation::ms_pPedGroups[ms_currentPedGrp].models[i]);
					SetModelTxdIsDeletable(CPopulation::ms_pPedGroups[ms_currentPedGrp].models[i]);
				}
			}

		ms_currentPedGrp = info.pedGroup;

		for(i = 0; i < MAXZONEPEDSLOADED; i++){
			do
				j = CGeneral::GetRandomNumberInRange(0, NUMMODELSPERPEDGROUP);
			while(ms_bIsPedFromPedGroupLoaded[j]);
			ms_bIsPedFromPedGroupLoaded[j] = true;
			if(CPopulation::ms_pPedGroups[ms_currentPedGrp].models[j] != -1)
				RequestModel(CPopulation::ms_pPedGroups[ms_currentPedGrp].models[j], STREAMFLAGS_DEPENDENCY);
		}
		ms_numPedsLoaded = MAXZONEPEDSLOADED;
		timeBeforeNextLoad = 300;
	}

	if(timeBeforeNextLoad >= 0)
		timeBeforeNextLoad--;
	else{
		// Switch a ped
		int oldMI;
		// Find a ped to unload
		for(i = 0; i < NUMMODELSPERPEDGROUP; i++)
			if(ms_bIsPedFromPedGroupLoaded[i]){
				oldMI = CPopulation::ms_pPedGroups[ms_currentPedGrp].models[i];
				if(oldMI != -1 && CModelInfo::GetModelInfo(oldMI)->GetNumRefs() == 0)
					break;
			}
		// And load a new one
		if(i != NUMMODELSPERPEDGROUP || ms_numPedsLoaded < MAXZONEPEDSLOADED){
			do
				j = CGeneral::GetRandomNumberInRange(0, NUMMODELSPERPEDGROUP);
			while(ms_bIsPedFromPedGroupLoaded[j]);
			if(ms_numPedsLoaded == MAXZONEPEDSLOADED)
				ms_bIsPedFromPedGroupLoaded[i] = false;
			ms_bIsPedFromPedGroupLoaded[j] = true;
			int newMI = CPopulation::ms_pPedGroups[ms_currentPedGrp].models[j];
			if(newMI != oldMI){
				RequestModel(newMI, STREAMFLAGS_DEPENDENCY);
				debug("Request Ped %s\n", CModelInfo::GetModelInfo(newMI)->GetModelName());
				if(ms_numPedsLoaded == MAXZONEPEDSLOADED){
					SetModelIsDeletable(oldMI);
					SetModelTxdIsDeletable(oldMI);
					debug("Remove Ped %s\n", CModelInfo::GetModelInfo(oldMI)->GetModelName());
				}else
					ms_numPedsLoaded++;
				timeBeforeNextLoad = 300;
			}
		}
	}

	RequestModel(MI_MALE01, STREAMFLAGS_DONT_REMOVE);
	RequestModel(MI_TAXI_D, STREAMFLAGS_DONT_REMOVE);

	gangsToLoad = 0;
	gangCarsToLoad = 0;
	if(info.gangPedThreshold[0] != info.copPedThreshold)
		gangsToLoad = 1;
	for(i = 1; i < NUM_GANGS; i++)
		if(info.gangPedThreshold[i] != info.gangPedThreshold[i-1])
			gangsToLoad |= 1<<i;
	if(info.gangThreshold[0] != info.copThreshold)
		gangCarsToLoad = 1;
	for(i = 1; i < NUM_GANGS; i++)
		if(info.gangThreshold[i] != info.gangThreshold[i-1])
			gangCarsToLoad |= 1<<i;

	if(gangsToLoad == ms_loadedGangs && gangCarsToLoad == ms_loadedGangCars)
		return;

	int gangModelsToload = gangsToLoad | gangCarsToLoad;

	if(gangsToLoad != ms_loadedGangs || gangCarsToLoad != ms_loadedGangCars){
		for(i = 0; i < NUM_GANGS; i++){
			bit = 1<<i;

			if(gangModelsToload & bit && (ms_loadedGangs & bit) == 0){
				RequestModel(CGangs::GetGangPedModel1(i), STREAMFLAGS_DEPENDENCY);
				RequestModel(CGangs::GetGangPedModel2(i), STREAMFLAGS_DEPENDENCY);
				ms_loadedGangs |= bit;
			}else if((gangModelsToload & bit) == 0 && ms_loadedGangs & bit){
				SetModelIsDeletable(CGangs::GetGangPedModel1(i));
				SetModelIsDeletable(CGangs::GetGangPedModel2(i));
				SetModelTxdIsDeletable(CGangs::GetGangPedModel1(i));
				SetModelTxdIsDeletable(CGangs::GetGangPedModel2(i));
				ms_loadedGangs &= ~bit;
			}

			if(CGangs::GetGangVehicleModel(i) != -1){
				if((gangCarsToLoad & bit) && (ms_loadedGangCars & bit) == 0){
					RequestModel(CGangs::GetGangVehicleModel(i), STREAMFLAGS_DEPENDENCY);
				}else if((gangCarsToLoad & bit) == 0 && ms_loadedGangCars & bit){
					SetModelIsDeletable(CGangs::GetGangVehicleModel(i));
					SetModelTxdIsDeletable(CGangs::GetGangVehicleModel(i));
				}
			}
		}
		ms_loadedGangCars = gangCarsToLoad;
	}
}

void
CStreaming::RemoveCurrentZonesModels(void)
{
	int i;

	if (ms_currentPedGrp != -1)
		for (i = 0; i < NUMMODELSPERPEDGROUP; i++) {
			ms_bIsPedFromPedGroupLoaded[i] = false;
			if (CPopulation::ms_pPedGroups[ms_currentPedGrp].models[i] != -1) {
				SetModelIsDeletable(CPopulation::ms_pPedGroups[ms_currentPedGrp].models[i]);
				SetModelTxdIsDeletable(CPopulation::ms_pPedGroups[ms_currentPedGrp].models[i]);
			}
		}

	CStreaming::RequestModel(MI_MALE01, STREAMFLAGS_DONT_REMOVE);
	CStreaming::RequestModel(MI_TAXI_D, STREAMFLAGS_DONT_REMOVE);

	for (i = 0; i < NUM_GANGS; i++) {
		if (CGangs::GetGangPedModel1(i) != -1) {
			SetModelIsDeletable(CGangs::GetGangPedModel1(i));
			SetModelTxdIsDeletable(CGangs::GetGangPedModel1(i));
		}
		if (CGangs::GetGangPedModel2(i) != -1) {
			SetModelIsDeletable(CGangs::GetGangPedModel2(i));
			SetModelTxdIsDeletable(CGangs::GetGangPedModel2(i));
		}
		if (CGangs::GetGangVehicleModel(i) != -1) {
			SetModelIsDeletable(CGangs::GetGangVehicleModel(i));
			SetModelTxdIsDeletable(CGangs::GetGangVehicleModel(i));
		}
	}

	ms_currentPedGrp = -1;
	ms_loadedGangs = 0;
	ms_loadedGangCars = 0;
}

void
CStreaming::LoadBigBuildingsWhenNeeded(void)
{
	// Very much like CCollision::Update and CCollision::LoadCollisionWhenINeedIt
	if(CCutsceneMgr::IsCutsceneProcessing())
		return;

	if(CTheZones::m_CurrLevel == LEVEL_GENERIC || 
	   CTheZones::m_CurrLevel == CGame::currLevel)
		return;

	CTimer::Suspend();
	CGame::currLevel = CTheZones::m_CurrLevel;
	ISLAND_LOADING_IS(LOW)
	{
		DMAudio.SetEffectsFadeVol(0);
		CPad::StopPadsShaking();
		CCollision::LoadCollisionScreen(CGame::currLevel);
		DMAudio.Service();

		RemoveUnusedBigBuildings(CGame::currLevel);
		RemoveUnusedBuildings(CGame::currLevel);
		RemoveUnusedModelsInLoadedList();
		CGame::TidyUpMemory(true, true);
	}
	CReplay::EmptyReplayBuffer();
	if(CGame::currLevel != LEVEL_GENERIC)
		LoadSplash(GetLevelSplashScreen(CGame::currLevel));

	ISLAND_LOADING_IS(LOW)
		CStreaming::RequestBigBuildings(CGame::currLevel, TheCamera.GetPosition());
#ifdef NO_ISLAND_LOADING
	else if(FrontEndMenuManager.m_PrefsIslandLoading == CMenuManager::ISLAND_LOADING_MEDIUM) {
		RemoveIslandsNotUsed(CGame::currLevel);
		CStreaming::RequestIslands(CGame::currLevel);
	}
#endif

	CStreaming::LoadAllRequestedModels(false);

	CGame::TidyUpMemory(true, true);
	CTimer::Resume();

	ISLAND_LOADING_IS(LOW)
		DMAudio.SetEffectsFadeVol(127);
}


// Find starting offset of the cdimage we next want to read
// Not useful at all on PC...
int32
CStreaming::GetCdImageOffset(int32 lastPosn)
{
	int offset, off;
	int i, img;
	int dist, mindist;

	img = -1;
	mindist = INT32_MAX;
	offset = ms_imageOffsets[ms_lastImageRead];
	if(lastPosn <= offset || lastPosn > offset + ms_imageSize){
		// last read position is not in last image
		for(i = 0; i < NUMCDIMAGES; i++){
			off = ms_imageOffsets[i];
			if(off == -1) continue;
			if((uint32)lastPosn > (uint32)off)
				// after start of image, get distance from end
				// negative if before end!
				dist = lastPosn - (off + ms_imageSize);
			else
				// before image, get offset to start
				// this will never be negative
				dist = off - lastPosn;
			if(dist < mindist){
				img = i;
				mindist = dist;
			}
		}
		assert(img >= 0);
		offset = ms_imageOffsets[img];
		ms_lastImageRead = img;
	}
	return offset;
}

inline bool
ModelNotLoaded(int32 modelId)
{
	CStreamingInfo *si = &CStreaming::ms_aInfoForModel[modelId];
	return si->m_loadState != STREAMSTATE_LOADED && si->m_loadState != STREAMSTATE_READING;
}

inline bool TxdNotLoaded(int32 txdId) { return ModelNotLoaded(txdId + STREAM_OFFSET_TXD); }
inline bool AnimNotLoaded(int32 animId) { return animId != -1 && ModelNotLoaded(animId + STREAM_OFFSET_ANIM); }

// Find stream id of next requested file in cdimage
int32
CStreaming::GetNextFileOnCd(int32 lastPosn, bool priority)
{
	CStreamingInfo *si, *next;
	int streamId;
	uint32 posn, size;
	int streamIdFirst, streamIdNext;
	uint32 posnFirst, posnNext;

	streamIdFirst = -1;
	streamIdNext = -1;
	posnFirst = UINT32_MAX;
	posnNext = UINT32_MAX;

	for(si = ms_startRequestedList.m_next; si != &ms_endRequestedList; si = next){
		next = si->m_next;
		streamId = si - ms_aInfoForModel;

		// only priority requests if there are any
		if(priority && ms_numPriorityRequests != 0 && !si->IsPriority())
			continue;

		// request Txds or anims if necessary
		if(streamId < STREAM_OFFSET_TXD){
			int txdId = CModelInfo::GetModelInfo(streamId)->GetTxdSlot();
			if(TxdNotLoaded(txdId)){
				ReRequestTxd(txdId);
				continue;
			}
			int animId = CModelInfo::GetModelInfo(streamId)->GetAnimFileIndex();
			if(AnimNotLoaded(animId)){
				ReRequestAnim(animId);
				continue;
			}
		}else if(streamId >= STREAM_OFFSET_ANIM && CCutsceneMgr::IsCutsceneProcessing())
			continue;

		if(ms_aInfoForModel[streamId].GetCdPosnAndSize(posn, size)){
			if(posn < posnFirst){
				// find first requested file in image
				streamIdFirst = streamId;
				posnFirst = posn;
			}
			if(posn < posnNext && posn >= (uint32)lastPosn){
				// find first requested file after last read position
				streamIdNext = streamId;
				posnNext = posn;
			}
		}else{
			// empty file
			DecrementRef(streamId);
			si->RemoveFromList();
			si->m_loadState = STREAMSTATE_LOADED;
		}
	}

	// wrap around
	if(streamIdNext == -1)
		streamIdNext = streamIdFirst;

	if(streamIdNext == -1 && ms_numPriorityRequests != 0){
		// try non-priority files
		ms_numPriorityRequests = 0;
		streamIdNext = GetNextFileOnCd(lastPosn, false);
	}

	return streamIdNext;
}

/*
 * Streaming buffer size is half of the largest file.
 * Files larger than the buffer size can only be loaded by channel 0,
 * which then uses both buffers, while channel 1 is idle.
 * ms_bLoadingBigModel is set to true to indicate this state.
 */

// Make channel read from disc
void
CStreaming::RequestModelStream(int32 ch)
{
	int lastPosn, imgOffset, streamId;
	int totalSize;
	uint32 posn, size, unused;
	int i;
	int haveBigFile, havePed;

	lastPosn = CdStreamGetLastPosn();
	imgOffset = GetCdImageOffset(lastPosn);
	streamId = GetNextFileOnCd(lastPosn - imgOffset, true);

	// remove Txds and Anims that aren't requested anymore
	while(streamId != -1){
		if(ms_aInfoForModel[streamId].m_flags & STREAMFLAGS_KEEP_IN_MEMORY)
			break;
		if(streamId >= STREAM_OFFSET_TXD && streamId < STREAM_OFFSET_COL){
			if(IsTxdUsedByRequestedModels(streamId - STREAM_OFFSET_TXD))
				break;
		}else if(streamId >= STREAM_OFFSET_ANIM){
			assert(streamId < NUMSTREAMINFO);
			if(AreAnimsUsedByRequestedModels(streamId - STREAM_OFFSET_ANIM))
				break;
		}else
			break;
		RemoveModel(streamId);
		// so try next file
		ms_aInfoForModel[streamId].GetCdPosnAndSize(posn, size);
		streamId = GetNextFileOnCd(posn + size, true);
	}

	if(streamId == -1)
		return;

	ms_aInfoForModel[streamId].GetCdPosnAndSize(posn, size);
	if(size > (uint32)ms_streamingBufferSize){
		// Can only load big models on channel 0, and 1 has to be idle
		if(ch == 1 || ms_channel[1].state != CHANNELSTATE_IDLE)
			return;
		ms_bLoadingBigModel = true;
	}

	// Load up to 4 adjacent files
	haveBigFile = 0;
	havePed = 0;
	totalSize = 0;
	for(i = 0; i < 4; i++){
		// no more files we can read
		if(streamId == -1 || ms_aInfoForModel[streamId].m_loadState != STREAMSTATE_INQUEUE)
			break;

		// also stop at non-priority files
		ms_aInfoForModel[streamId].GetCdPosnAndSize(unused, size);
		if(ms_numPriorityRequests != 0 && !ms_aInfoForModel[streamId].IsPriority())
			break;

		// Can't load certain combinations of files together
		if(streamId < STREAM_OFFSET_TXD){
			if (havePed && CModelInfo::GetModelInfo(streamId)->GetModelType() == MITYPE_PED ||
			    haveBigFile && CModelInfo::GetModelInfo(streamId)->GetModelType() == MITYPE_VEHICLE ||
			    TxdNotLoaded(CModelInfo::GetModelInfo(streamId)->GetTxdSlot()) ||
			    AnimNotLoaded(CModelInfo::GetModelInfo(streamId)->GetAnimFileIndex()))
				break;
		}else{
			if(haveBigFile && size > 200)
				break;
		}

		// Now add the file
		ms_channel[ch].streamIds[i] = streamId;
		ms_channel[ch].offsets[i] = totalSize;
		totalSize += size;

		// To big for buffer, remove again
		if(totalSize > ms_streamingBufferSize && i > 0){
			totalSize -= size;
			break;
		}
		if(streamId < STREAM_OFFSET_TXD){
			if (CModelInfo::GetModelInfo(streamId)->GetModelType() == MITYPE_PED)
				havePed = 1;
			if (CModelInfo::GetModelInfo(streamId)->GetModelType() == MITYPE_VEHICLE)
				haveBigFile = 1;
		}else{
			if(size > 200)
				haveBigFile = 1;
		}
		ms_aInfoForModel[streamId].m_loadState = STREAMSTATE_READING;
		ms_aInfoForModel[streamId].RemoveFromList();
		DecrementRef(streamId);

		streamId = ms_aInfoForModel[streamId].m_nextID;
	}

	// clear remaining slots
	for(; i < 4; i++)
		ms_channel[ch].streamIds[i] = -1;
	// Now read the data
	assert(!(ms_bLoadingBigModel && ch == 1));	// this would clobber the buffer
	if(CdStreamRead(ch, ms_pStreamingBuffer[ch], imgOffset+posn, totalSize) == STREAM_NONE)
		debug("FUCKFUCKFUCK\n");
	ms_channel[ch].state = CHANNELSTATE_READING;
	ms_channel[ch].field24 = 0;
	ms_channel[ch].size = totalSize;
	ms_channel[ch].position = imgOffset+posn;
	ms_channel[ch].numTries = 0;
}

// Load data previously read from disc
bool
CStreaming::ProcessLoadingChannel(int32 ch)
{
	int status;
	int i, id, cdsize;

	status = CdStreamGetStatus(ch);
	if(status != STREAM_NONE){
		// busy
		if(status != STREAM_READING && status != STREAM_WAITING){
			ms_channelError = ch;
			ms_channel[ch].state = CHANNELSTATE_ERROR;
			ms_channel[ch].status = status;
#ifdef __EMSCRIPTEN__
			// D4 (sección 1): lectura del cargador fallida. En web casi siempre es
			// un fichero que la capa on-demand todavía no tiene (ASYNCIFY lo aplaza
			// a propósito para no congelar el frame): el motor reintenta. Con el id
			// del modelo que iba en el canal se sabe QUÉ modelo se quedó a medias y
			// si vuelve (emparejar con `CARLOAD`).
			{
				static int odFallos = 0;
				if (odFallos < 40) {
					char tt[200];
					snprintf(tt, sizeof tt, "CARFAIL ch=%d status=%d intentos=%d id=%d,%d,%d,%d",
						ch, status, ms_channel[ch].numTries,
						ms_channel[ch].streamIds[0], ms_channel[ch].streamIds[1],
						ms_channel[ch].streamIds[2], ms_channel[ch].streamIds[3]);
					ODTRACES(tt);
					odFallos++;
				}
			}
#endif
		}
		return false;
	}

	if(ms_channel[ch].state == CHANNELSTATE_STARTED){
		ms_channel[ch].state = CHANNELSTATE_IDLE;
		FinishLoadingLargeFile(&ms_pStreamingBuffer[ch][ms_channel[ch].offsets[0]*CDSTREAM_SECTOR_SIZE],
			ms_channel[ch].streamIds[0]);
		ms_channel[ch].streamIds[0] = -1;
	}else{
		ms_channel[ch].state = CHANNELSTATE_IDLE;
		for(i = 0; i < 4; i++){
			id = ms_channel[ch].streamIds[i];
			if(id == -1)
				continue;

			cdsize = ms_aInfoForModel[id].GetCdSize();
			if(id < STREAM_OFFSET_TXD && CModelInfo::GetModelInfo(id)->GetModelType() == MITYPE_VEHICLE &&
			   ms_numVehiclesLoaded >= desiredNumVehiclesLoaded &&
			   !RemoveLoadedVehicle() &&
			   (CanRemoveModel(id) || GetAvailableVehicleSlot() == -1)){
				// can't load vehicle
				RemoveModel(id);
				if(!CanRemoveModel(id))
					ReRequestModel(id);
				else if(CTxdStore::GetNumRefs(CModelInfo::GetModelInfo(id)->GetTxdSlot()) == 0)
					RemoveTxd(CModelInfo::GetModelInfo(id)->GetTxdSlot());
			}else{
				MakeSpaceFor(cdsize * CDSTREAM_SECTOR_SIZE);
				ConvertBufferToObject(&ms_pStreamingBuffer[ch][ms_channel[ch].offsets[i]*CDSTREAM_SECTOR_SIZE],
					id);
				if(ms_aInfoForModel[id].m_loadState == STREAMSTATE_STARTED){
					// queue for second part
					ms_channel[ch].state = CHANNELSTATE_STARTED;
					ms_channel[ch].offsets[0] = ms_channel[ch].offsets[i];
					ms_channel[ch].streamIds[0] = id;
					if(i != 0)
						ms_channel[ch].streamIds[i] = -1;
				}else
					ms_channel[ch].streamIds[i] = -1;
			}
		}
	}

	if(ms_bLoadingBigModel && ms_channel[ch].state != CHANNELSTATE_STARTED){
		ms_bLoadingBigModel = false;
		// reset channel 1 after loading a big model
		for(i = 0; i < 4; i++)
			ms_channel[1].streamIds[i] = -1;
		ms_channel[1].state = CHANNELSTATE_IDLE;
	}

	return true;
}

void
CStreaming::RetryLoadFile(int32 ch)
{
	Const char *key;

	CPad::StopPadsShaking();

	if(ms_channel[ch].numTries >= 3){
		switch(ms_channel[ch].status){
		case STREAM_ERROR_NOCD: key = "NOCD"; break;
		case STREAM_ERROR_OPENCD: key = "OPENCD"; break;
		case STREAM_ERROR_WRONGCD: key = "WRONGCD"; break;
		default: key = "CDERROR"; break;
		}
		CHud::SetMessage(TheText.Get(key));
		CTimer::SetCodePause(true);
	}

	switch(ms_channel[ch].state){
	case CHANNELSTATE_ERROR:
		ms_channel[ch].numTries++;
		if (CdStreamGetStatus(ch) == STREAM_READING || CdStreamGetStatus(ch) == STREAM_WAITING) break;
	case CHANNELSTATE_IDLE:
		CdStreamRead(ch, ms_pStreamingBuffer[ch], ms_channel[ch].position, ms_channel[ch].size);
		ms_channel[ch].state = CHANNELSTATE_READING;
		ms_channel[ch].field24 = -600;
		break;
	case CHANNELSTATE_READING:
		if(ProcessLoadingChannel(ch)){
			ms_channelError = -1;
			CTimer::SetCodePause(false);
		}
		break;
	}
}

void
CStreaming::LoadRequestedModels(void)
{
	static int currentChannel = 0;

	// We can't read with channel 1 while channel 0 is using its buffer
	if(ms_bLoadingBigModel)
		currentChannel = 0;

	// We have data, load
	if(ms_channel[currentChannel].state == CHANNELSTATE_READING ||
	   ms_channel[currentChannel].state == CHANNELSTATE_STARTED)
		ProcessLoadingChannel(currentChannel);

	if(ms_channelError == -1){
		// Channel is idle, read more data
		if(ms_channel[currentChannel].state == CHANNELSTATE_IDLE)
			RequestModelStream(currentChannel);
		// Switch channel
		if(ms_channel[currentChannel].state != CHANNELSTATE_STARTED)
			currentChannel = 1 - currentChannel;
	}
}


// Let's load models in 4 threads; when one of them becomes idle, process the file, and fill thread with another file. Unfortunately processing models are still single-threaded.
// Currently only supported on POSIX streamer.
// WIP - some files are loaded swapped (CdStreamPosix problem?)
#if 0 //def ONE_THREAD_PER_CHANNEL
void
CStreaming::LoadAllRequestedModels(bool priority)
{
	static bool bInsideLoadAll = false;
	int imgOffset, streamId, status;
	int i;
	uint32 posn, size;

	if(bInsideLoadAll)
		return;
	bInsideLoadAll = true;

	FlushChannels();
	imgOffset = GetCdImageOffset(CdStreamGetLastPosn());

	int streamIds[ARRAY_SIZE(ms_pStreamingBuffer)];
	int streamSizes[ARRAY_SIZE(ms_pStreamingBuffer)];
	int streamPoses[ARRAY_SIZE(ms_pStreamingBuffer)];
	int readOrder[4] = {-1}; // Channel IDs ordered by read time
	int readI = 0;
	int processI = 0;
	bool first = true;

	// All those "first" checks are because of variables aren't initialized in first pass.

	while (true) {
		for (int i=0; i<ARRAY_SIZE(ms_pStreamingBuffer); i++) {

			// Channel has file to load
			if (!first && streamIds[i] != -1) {
				continue;
			}

			if(ms_endRequestedList.m_prev != &ms_startRequestedList){
				streamId = GetNextFileOnCd(0, priority);
				if(streamId == -1){
					streamIds[i] = -1;
					break;
				}

				if (ms_aInfoForModel[streamId].GetCdPosnAndSize(posn, size)) {
					streamIds[i] = -1;

					// Big file, needs 2 buffer
					if (size > (uint32)ms_streamingBufferSize) {
						if (i + 1 == ARRAY_SIZE(ms_pStreamingBuffer))
							break;
						else if (!first && streamIds[i+1] != -1)
							continue;

					} else {
						// Buffer of current channel is part of a "big file", pass
						if (i != 0 && streamIds[i-1] != -1 && streamSizes[i-1] > (uint32)ms_streamingBufferSize)
							continue;
					}
					ms_aInfoForModel[streamId].RemoveFromList();
					DecrementRef(streamId);

					streamIds[i] = streamId;
					streamSizes[i] = size;
					streamPoses[i] = posn;

					if (!first)
						assert(readOrder[readI] == -1);

					//printf("read: order %d, ch %d, id %d, size %d\n", readI, i, streamId, size);

					CdStreamRead(i, ms_pStreamingBuffer[i], imgOffset+posn, size);
					readOrder[readI] = i;
					if (first && readI+1 != ARRAY_SIZE(readOrder))
						readOrder[readI+1] = -1;

					readI = (readI + 1) % ARRAY_SIZE(readOrder);
				} else {
					ms_aInfoForModel[streamId].RemoveFromList();
					DecrementRef(streamId);

					ms_aInfoForModel[streamId].m_loadState = STREAMSTATE_LOADED;
					streamIds[i] = -1;
				}
			} else {
				streamIds[i] = -1;
				break;
			}
		}

		first = false;
		int nextChannel = readOrder[processI];

		// Now start processing
		if (nextChannel == -1 || streamIds[nextChannel] == -1)
			break;

		//printf("process: order %d, ch %d, id %d\n", processI, nextChannel, streamIds[nextChannel]);

		// Try again on error (bounded: a permanent failure must pop in
		// later, never hang the page — web on-demand fetches it meanwhile).
		{ int odTries = 0;
		while (CdStreamSync(nextChannel) != STREAM_NONE) {
			CdStreamRead(nextChannel, ms_pStreamingBuffer[nextChannel], imgOffset+streamPoses[nextChannel], streamSizes[nextChannel]);
			if (++odTries > 25) { debug("CdStream: 25 reintentos, se reintentará luego\n"); break; }
		} }
		ms_aInfoForModel[streamIds[nextChannel]].m_loadState = STREAMSTATE_READING;

		MakeSpaceFor(streamSizes[nextChannel] * CDSTREAM_SECTOR_SIZE);
		ConvertBufferToObject(ms_pStreamingBuffer[nextChannel], streamIds[nextChannel]);
		if(ms_aInfoForModel[streamIds[nextChannel]].m_loadState == STREAMSTATE_STARTED)
			FinishLoadingLargeFile(ms_pStreamingBuffer[nextChannel], streamIds[nextChannel]);

		if(streamIds[nextChannel] < STREAM_OFFSET_TXD){
			CSimpleModelInfo *mi = (CSimpleModelInfo*)CModelInfo::GetModelInfo(streamIds[nextChannel]);
			if(mi->IsSimple())
				mi->m_alpha = 255;
		}
		streamIds[nextChannel] = -1;
		readOrder[processI] = -1;
		processI = (processI + 1) % ARRAY_SIZE(readOrder);
	}

	ms_bLoadingBigModel = false;
	for(i = 0; i < 4; i++){
		ms_channel[1].streamIds[i] = -1;
		ms_channel[1].offsets[i] = -1;
	}
	ms_channel[1].state = CHANNELSTATE_IDLE;
	bInsideLoadAll = false;
}
#else
void
CStreaming::LoadAllRequestedModels(bool priority)
{
	static bool bInsideLoadAll = false;
	int imgOffset, streamId, status;
	int i;
	uint32 posn, size;

	int numRequests = 4*ms_numModelsRequested;

	if(bInsideLoadAll)
		return;
	bInsideLoadAll = true;

	// Web: durante esta carga el motor está BLOQUEADO pidiendo ficheros. La
	// página no puede aplazarlos (contestar "no está"): se quedaría sin ellos
	// y el modelo saldría vacío (escena de cinemática negra, sin audio).
	odBlockingPush();

	if(priority)
		numRequests = ms_numPriorityRequests;

	FlushChannels();
	imgOffset = GetCdImageOffset(CdStreamGetLastPosn());

#ifdef __EMSCRIPTEN__
	// Web: tope de ficheros por llamada (carga troceada por ticks). 0 = sin tope.
	// R1: aquí NO hay (ni puede haber) tope por TIEMPO — ver Streaming.h.
	int odBudget = gWebLoadBudget;
#endif
	while(ms_endRequestedList.m_prev != &ms_startRequestedList && numRequests > 0
#ifdef __EMSCRIPTEN__
		&& (odBudget <= 0 || odBudget-- > 0)
#endif
	){
		numRequests--;
#ifdef __EMSCRIPTEN__
		// Solo medición (ODLOAD big): coste del fichero. No altera el control.
		double odFileT0 = emscripten_get_now();
#endif
		streamId = GetNextFileOnCd(0, priority);
		if(streamId == -1)
			break;

		ms_aInfoForModel[streamId].RemoveFromList();
		ms_channel[0].streamIds[0] = streamId;
		DecrementRef(streamId);

		if(ms_aInfoForModel[streamId].GetCdPosnAndSize(posn, size)){
			int odTries = 0;
			do {
#ifdef __EMSCRIPTEN__
				// D15 (sección 1, 21/09): condición del do/while ARREGLADA.
				// El original (`status == STREAM_NONE`) repetía mientras la lectura
				// iba BIEN: en web el fichero aplazado devuelve STREAM_NONE y esto
				// giraba para siempre dentro de un comando del SCM (pantalla negra
				// tras el menú). La condición correcta es repetir solo si falló
				// (STREAM_ERROR y similares), con tope, y seguir si agota.
				status = CdStreamRead(0, ms_pStreamingBuffer[0], imgOffset+posn, size);
				int odSync = CdStreamSync(0);
				if (odSync == STREAM_NONE || odSync == STREAM_SUCCESS) break;
				if (++odTries > 25) {
					debug("CdStream: 25 reintentos, se reintentará luego (id=%d)\n", streamId);
					ODTRACES("CDRETRY agotado");
					break;
				}
#else
				status = CdStreamRead(0, ms_pStreamingBuffer[0], imgOffset+posn, size);
				if (++odTries > 25) { debug("CdStream: 25 reintentos, se reintentará luego\n"); break; }
#endif
			}
#ifdef __EMSCRIPTEN__
			while (status != STREAM_SUCCESS);
#else
			while(CdStreamSync(0) || status == STREAM_NONE);
#endif
			ms_aInfoForModel[streamId].m_loadState = STREAMSTATE_READING;

			MakeSpaceFor(size * CDSTREAM_SECTOR_SIZE);
			ConvertBufferToObject(ms_pStreamingBuffer[0], streamId);
			if(ms_aInfoForModel[streamId].m_loadState == STREAMSTATE_STARTED)
				FinishLoadingLargeFile(ms_pStreamingBuffer[0], streamId);

			if(streamId < STREAM_OFFSET_TXD){
				CSimpleModelInfo *mi = (CSimpleModelInfo*)CModelInfo::GetModelInfo(streamId);
				if(mi->IsSimple())
					mi->m_alpha = 255;
			}
		}else{
			// empty
			ms_aInfoForModel[streamId].m_loadState = STREAMSTATE_LOADED;
		}
#ifdef __EMSCRIPTEN__
		// F6b: un fichero que se pasa él solo del presupuesto explica un pico
		// por sí mismo (no se puede trocear un decode). Traza solo los gordos.
		{
			double odFileMs = emscripten_get_now() - odFileT0;
			if (odFileMs >= 8.0) {
				static int odBigN = 0;
				if (odBigN < 60) {
					char t[144];
					odBigN++;
					snprintf(t, sizeof t, "ODLOAD big id=%d ms=%.1f tipo=%s",
						streamId, odFileMs,
						streamId >= STREAM_OFFSET_TXD ? (streamId >= STREAM_OFFSET_COL ? "col/anim" : "txd") : "dff");
					ODTRACES(t);
				}
			}
		}
#endif
	}

	ms_bLoadingBigModel = false;
	for(i = 0; i < 4; i++){
		ms_channel[1].streamIds[i] = -1;
		ms_channel[1].offsets[i] = -1;
	}
	ms_channel[1].state = CHANNELSTATE_IDLE;
	bInsideLoadAll = false;
	odBlockingPop();
}
#endif

void
CStreaming::FlushChannels(void)
{
	if(ms_channel[1].state == CHANNELSTATE_STARTED)
		ProcessLoadingChannel(1);

	if(ms_channel[0].state == CHANNELSTATE_READING){
		CdStreamSync(0);
		ProcessLoadingChannel(0);
	}
	if(ms_channel[0].state == CHANNELSTATE_STARTED)
		ProcessLoadingChannel(0);

	if(ms_channel[1].state == CHANNELSTATE_READING){
		CdStreamSync(1);
		ProcessLoadingChannel(1);
	}
	if(ms_channel[1].state == CHANNELSTATE_STARTED)
		ProcessLoadingChannel(1);
}

void
CStreaming::FlushRequestList(void)
{
	CStreamingInfo *si, *next;

	for(si = ms_startRequestedList.m_next; si != &ms_endRequestedList; si = next){
		next = si->m_next;
		RemoveModel(si - ms_aInfoForModel);
	}
#ifdef FLUSHABLE_STREAMING
	if(ms_channel[0].state == CHANNELSTATE_READING) {
		flushStream[0] = 1;
	}
	if(ms_channel[1].state == CHANNELSTATE_READING) {
		flushStream[1] = 1;
	}
#endif
	FlushChannels();
}


void
CStreaming::ImGonnaUseStreamingMemory(void)
{
	PUSH_MEMID(MEMID_STREAM);
}

void
CStreaming::IHaveUsedStreamingMemory(void)
{
	POP_MEMID();
	UpdateMemoryUsed();
}

void
CStreaming::UpdateMemoryUsed(void)
{
#ifdef USE_CUSTOM_ALLOCATOR
	ms_memoryUsed =
		gMainHeap.GetMemoryUsed(MEMID_STREAM) +
		gMainHeap.GetMemoryUsed(MEMID_STREAM_MODELS) +
		gMainHeap.GetMemoryUsed(MEMID_STREAM_TEXUTRES) +
		gMainHeap.GetMemoryUsed(MEMID_STREAM_COLLISION) +
		gMainHeap.GetMemoryUsed(MEMID_STREAM_ANIMATION);
#endif
}

#define STREAM_DIST 80.0f

void
CStreaming::AddModelsToRequestList(const CVector &pos, int32 flags)
{
	float xmin, xmax, ymin, ymax;
	int ixmin, ixmax, iymin, iymax;
	int ix, iy;
	int dx, dy, d;
	CSector *sect;

	xmin = pos.x - STREAM_DIST;
	ymin = pos.y - STREAM_DIST;
	xmax = pos.x + STREAM_DIST;
	ymax = pos.y + STREAM_DIST;

	ixmin = CWorld::GetSectorIndexX(xmin);
	if(ixmin < 0) ixmin = 0;
	ixmax = CWorld::GetSectorIndexX(xmax);
	if(ixmax >= NUMSECTORS_X) ixmax = NUMSECTORS_X-1;
	iymin = CWorld::GetSectorIndexY(ymin);
	if(iymin < 0) iymin = 0;
	iymax = CWorld::GetSectorIndexY(ymax);
	if(iymax >= NUMSECTORS_Y) iymax = NUMSECTORS_Y-1;

	CWorld::AdvanceCurrentScanCode();

	for(iy = iymin; iy <= iymax; iy++){
		dy = iy - CWorld::GetSectorIndexY(pos.y);
		for(ix = ixmin; ix <= ixmax; ix++){

			if(CRenderer::m_loadingPriority && ms_numModelsRequested > 5)
				return;

			dx = ix - CWorld::GetSectorIndexX(pos.x);
			d = dx*dx + dy*dy;
			sect = CWorld::GetSector(ix, iy);
			if(d <= 0){
				ProcessEntitiesInSectorList(sect->m_lists[ENTITYLIST_BUILDINGS], flags);
				ProcessEntitiesInSectorList(sect->m_lists[ENTITYLIST_BUILDINGS_OVERLAP], flags);
				ProcessEntitiesInSectorList(sect->m_lists[ENTITYLIST_OBJECTS], flags);
				ProcessEntitiesInSectorList(sect->m_lists[ENTITYLIST_DUMMIES], flags);
			}else if(d <= 3*3){
				ProcessEntitiesInSectorList(sect->m_lists[ENTITYLIST_BUILDINGS], pos.x, pos.y, xmin, ymin, xmax, ymax, flags);
				ProcessEntitiesInSectorList(sect->m_lists[ENTITYLIST_BUILDINGS_OVERLAP], pos.x, pos.y, xmin, ymin, xmax, ymax, flags);
				ProcessEntitiesInSectorList(sect->m_lists[ENTITYLIST_OBJECTS], pos.x, pos.y, xmin, ymin, xmax, ymax, flags);
				ProcessEntitiesInSectorList(sect->m_lists[ENTITYLIST_DUMMIES], pos.x, pos.y, xmin, ymin, xmax, ymax, flags);
			}
		}
	}
}

void
CStreaming::ProcessEntitiesInSectorList(CPtrList &list, float x, float y, float xmin, float ymin, float xmax, float ymax, int32 flags)
{
	CPtrNode *node;
	CEntity *e;
	float lodDistSq;
	CVector2D pos;

	for(node = list.first; node; node = node->next){
		e = (CEntity*)node->item;

		if(e->m_scanCode == CWorld::GetCurrentScanCode())
			continue;

		e->m_scanCode = CWorld::GetCurrentScanCode();
		if(!e->bStreamingDontDelete && IsAreaVisible(e->m_area) && !e->bDontStream && e->bIsVisible){
			CTimeModelInfo *mi = (CTimeModelInfo*)CModelInfo::GetModelInfo(e->GetModelIndex());
			if (mi->GetModelType() != MITYPE_TIME || CClock::GetIsTimeInRange(mi->GetTimeOn(), mi->GetTimeOff())) {
				lodDistSq = sq(mi->GetLargestLodDistance());
				lodDistSq = Min(lodDistSq, sq(STREAM_DIST));
				pos = CVector2D(e->GetPosition());
				if(xmin < pos.x && pos.x < xmax &&
				   ymin < pos.y && pos.y < ymax &&
				   (CVector2D(x, y) - pos).MagnitudeSqr() < lodDistSq)
					RequestModel(e->GetModelIndex(), flags);
			}
		}
	}
}

void
CStreaming::ProcessEntitiesInSectorList(CPtrList &list, int32 flags)
{
	CPtrNode *node;
	CEntity *e;

	for(node = list.first; node; node = node->next){
		e = (CEntity*)node->item;

		if(e->m_scanCode == CWorld::GetCurrentScanCode())
			continue;

		e->m_scanCode = CWorld::GetCurrentScanCode();
		if(!e->bStreamingDontDelete && IsAreaVisible(e->m_area) && !e->bDontStream && e->bIsVisible){
			CTimeModelInfo *mi = (CTimeModelInfo*)CModelInfo::GetModelInfo(e->GetModelIndex());
			if (mi->GetModelType() != MITYPE_TIME || CClock::GetIsTimeInRange(mi->GetTimeOn(), mi->GetTimeOff()))
				RequestModel(e->GetModelIndex(), flags);
		}
	}
}

void
CStreaming::DeleteFarAwayRwObjects(const CVector &pos)
{
	int posx, posy;
	int x, y;
	int r, i;
	CSector *sect;

	posx = CWorld::GetSectorIndexX(pos.x);
	posy = CWorld::GetSectorIndexY(pos.y);

	// Move oldSectorX/Y to new sector and delete RW objects in its "wake" for every step:
	// O is the old sector, <- is the direction in which we move it,
	// X are the sectors we delete RW objects from (except we go up to 10)
	//            X
	//          X X
	//        X X X
	//        X X X
	// <- O   X X X
	//        X X X
	//        X X X
	//          X X
	//            X

	while(posx != ms_oldSectorX){
		if(posx < ms_oldSectorX){
			for(r = 2; r <= 10; r++){
				x = ms_oldSectorX + r;
				if(x < 0)
					continue;
				if(x >= NUMSECTORS_X)
					break;

				for(i = -r; i <= r; i++){
					y = ms_oldSectorY + i;
					if(y < 0)
						continue;
					if(y >= NUMSECTORS_Y)
						break;

					sect = CWorld::GetSector(x, y);
					DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_BUILDINGS]);
					DeleteRwObjectsInOverlapSectorList(sect->m_lists[ENTITYLIST_BUILDINGS_OVERLAP], ms_oldSectorX, ms_oldSectorY);
					DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_OBJECTS]);
					DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_DUMMIES]);
				}
			}
			ms_oldSectorX--;
		}else{
			for(r = 2; r <= 10; r++){
				x = ms_oldSectorX - r;
				if(x < 0)
					break;
				if(x >= NUMSECTORS_X)
					continue;

				for(i = -r; i <= r; i++){
					y = ms_oldSectorY + i;
					if(y < 0)
						continue;
					if(y >= NUMSECTORS_Y)
						break;

					sect = CWorld::GetSector(x, y);
					DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_BUILDINGS]);
					DeleteRwObjectsInOverlapSectorList(sect->m_lists[ENTITYLIST_BUILDINGS_OVERLAP], ms_oldSectorX, ms_oldSectorY);
					DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_OBJECTS]);
					DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_DUMMIES]);
				}
			}
			ms_oldSectorX++;
		}
	}

	while(posy != ms_oldSectorY){
		if(posy < ms_oldSectorY){
			for(r = 2; r <= 10; r++){
				y = ms_oldSectorY + r;
				if(y < 0)
					continue;
				if(y >= NUMSECTORS_Y)
					break;

				for(i = -r; i <= r; i++){
					x = ms_oldSectorX + i;
					if(x < 0)
						continue;
					if(x >= NUMSECTORS_X)
						break;

					sect = CWorld::GetSector(x, y);
					DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_BUILDINGS]);
					DeleteRwObjectsInOverlapSectorList(sect->m_lists[ENTITYLIST_BUILDINGS_OVERLAP], ms_oldSectorX, ms_oldSectorY);
					DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_OBJECTS]);
					DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_DUMMIES]);
				}
			}
			ms_oldSectorY--;
		}else{
			for(r = 2; r <= 10; r++){
				y = ms_oldSectorY - r;
				if(y < 0)
					break;
				if(y >= NUMSECTORS_Y)
					continue;

				for(i = -r; i <= r; i++){
					x = ms_oldSectorX + i;
					if(x < 0)
						continue;
					if(x >= NUMSECTORS_X)
						break;

					sect = CWorld::GetSector(x, y);
					DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_BUILDINGS]);
					DeleteRwObjectsInOverlapSectorList(sect->m_lists[ENTITYLIST_BUILDINGS_OVERLAP], ms_oldSectorX, ms_oldSectorY);
					DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_OBJECTS]);
					DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_DUMMIES]);
				}
			}
			ms_oldSectorY++;
		}
	}
}

void
CStreaming::DeleteAllRwObjects(void)
{
	int x, y;
	CSector *sect;

	for(x = 0; x < NUMSECTORS_X; x++)
		for(y = 0; y < NUMSECTORS_Y; y++){
			sect = CWorld::GetSector(x, y);
			DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_BUILDINGS]);
			DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_BUILDINGS_OVERLAP]);
			DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_OBJECTS]);
			DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_OBJECTS_OVERLAP]);
			DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_DUMMIES]);
			DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_DUMMIES_OVERLAP]);
		}
}

void
CStreaming::DeleteRwObjectsAfterDeath(const CVector &pos)
{
	int ix, iy;
	int x, y;
	CSector *sect;

	ix = CWorld::GetSectorIndexX(pos.x);
	iy = CWorld::GetSectorIndexY(pos.y);

	for(x = 0; x < NUMSECTORS_X; x++)
		for(y = 0; y < NUMSECTORS_Y; y++)
			if(Abs(ix - x) > 3.0f &&
			   Abs(iy - y) > 3.0f){
				sect = CWorld::GetSector(x, y);
				DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_BUILDINGS]);
				DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_BUILDINGS_OVERLAP]);
				DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_OBJECTS]);
				DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_OBJECTS_OVERLAP]);
				DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_DUMMIES]);
				DeleteRwObjectsInSectorList(sect->m_lists[ENTITYLIST_DUMMIES_OVERLAP]);
			}
}

void
CStreaming::DeleteRwObjectsBehindCamera(size_t mem)
{
	int ix, iy;
	int x, y;
	int xmin, xmax, ymin, ymax;
	int inc;
	CSector *sect;

	if(ms_memoryUsed < mem)
		return;

	ix = CWorld::GetSectorIndexX(TheCamera.GetPosition().x);
	iy = CWorld::GetSectorIndexY(TheCamera.GetPosition().y);

	if(Abs(TheCamera.GetForward().x) > Abs(TheCamera.GetForward().y)){
		// looking west/east

		ymin = Max(iy - 10, 0);
		ymax = Min(iy + 10, NUMSECTORS_Y - 1);
		assert(ymin <= ymax);

		// Delete a block of sectors that we know is behind the camera
		if(TheCamera.GetForward().x > 0.0f){
			// looking east
			xmax = Max(ix - 2, 0);
			xmin = Max(ix - 10, 0);
			inc = 1;
		}else{
			// looking west
			xmax = Min(ix + 2, NUMSECTORS_X - 1);
			xmin = Min(ix + 10, NUMSECTORS_X - 1);
			inc = -1;
		}
		for(y = ymin; y <= ymax; y++){
			for(x = xmin; x != xmax; x += inc){
				sect = CWorld::GetSector(x, y);
				if(DeleteRwObjectsBehindCameraInSectorList(sect->m_lists[ENTITYLIST_BUILDINGS], mem) ||
				   DeleteRwObjectsBehindCameraInSectorList(sect->m_lists[ENTITYLIST_DUMMIES], mem) ||
				   DeleteRwObjectsBehindCameraInSectorList(sect->m_lists[ENTITYLIST_OBJECTS], mem))
					return;
			}
		}

		
		while(RemoveLoadedZoneModel())
			if(ms_memoryUsed < mem)
				return;

		// Now a block that intersects with the camera's frustum
		if(TheCamera.GetForward().x > 0.0f){
			// looking east
			xmax = Max(ix + 10, 0);
			xmin = Max(ix - 2, 0);
			inc = 1;
		}else{
			// looking west
			xmax = Min(ix - 10, NUMSECTORS_X - 1);
			xmin = Min(ix + 2, NUMSECTORS_X - 1);
			inc = -1;
		}
		for(y = ymin; y <= ymax; y++){
			for(x = xmin; x != xmax; x += inc){
				sect = CWorld::GetSector(x, y);
				if(DeleteRwObjectsNotInFrustumInSectorList(sect->m_lists[ENTITYLIST_BUILDINGS], mem) ||
				   DeleteRwObjectsNotInFrustumInSectorList(sect->m_lists[ENTITYLIST_DUMMIES], mem) ||
				   DeleteRwObjectsNotInFrustumInSectorList(sect->m_lists[ENTITYLIST_OBJECTS], mem))
					return;
			}
		}

		// As last resort, delete objects from the last step more aggressively
		for(y = ymin; y <= ymax; y++){
			for(x = xmax; x != xmin; x -= inc){
				sect = CWorld::GetSector(x, y);
				if(DeleteRwObjectsBehindCameraInSectorList(sect->m_lists[ENTITYLIST_BUILDINGS], mem) ||
				   DeleteRwObjectsBehindCameraInSectorList(sect->m_lists[ENTITYLIST_DUMMIES], mem) ||
				   DeleteRwObjectsBehindCameraInSectorList(sect->m_lists[ENTITYLIST_OBJECTS], mem))
					return;
			}
		}
	}else{
		// looking north/south

		xmin = Max(ix - 10, 0);
		xmax = Min(ix + 10, NUMSECTORS_X - 1);
		assert(xmin <= xmax);

		// Delete a block of sectors that we know is behind the camera
		if(TheCamera.GetForward().y > 0.0f){
			// looking north
			ymax = Max(iy - 2, 0);
			ymin = Max(iy - 10, 0);
			inc = 1;
		}else{
			// looking south
			ymax = Min(iy + 2, NUMSECTORS_Y - 1);
			ymin = Min(iy + 10, NUMSECTORS_Y - 1);
			inc = -1;
		}
		for(x = xmin; x <= xmax; x++){
			for(y = ymin; y != ymax; y += inc){
				sect = CWorld::GetSector(x, y);
				if(DeleteRwObjectsBehindCameraInSectorList(sect->m_lists[ENTITYLIST_BUILDINGS], mem) ||
				   DeleteRwObjectsBehindCameraInSectorList(sect->m_lists[ENTITYLIST_DUMMIES], mem) ||
				   DeleteRwObjectsBehindCameraInSectorList(sect->m_lists[ENTITYLIST_OBJECTS], mem))
					return;
			}
		}

		while(RemoveLoadedZoneModel())
			if(ms_memoryUsed < mem)
				return;

		// Now a block that intersects with the camera's frustum
		if(TheCamera.GetForward().y > 0.0f){
			// looking north
			ymax = Max(iy + 10, 0);
			ymin = Max(iy - 2, 0);
			inc = 1;
		}else{
			// looking south
			ymax = Min(iy - 10, NUMSECTORS_Y - 1);
			ymin = Min(iy + 2, NUMSECTORS_Y - 1);
			inc = -1;
		}
		for(x = xmin; x <= xmax; x++){
			for(y = ymin; y != ymax; y += inc){
				sect = CWorld::GetSector(x, y);
				if(DeleteRwObjectsNotInFrustumInSectorList(sect->m_lists[ENTITYLIST_BUILDINGS], mem) ||
				   DeleteRwObjectsNotInFrustumInSectorList(sect->m_lists[ENTITYLIST_DUMMIES], mem) ||
				   DeleteRwObjectsNotInFrustumInSectorList(sect->m_lists[ENTITYLIST_OBJECTS], mem))
					return;
			}
		}

// this is gone in mobile together with RemoveReferencedTxds
//		if(RemoveReferencedTxds(mem))
//			return;

		// As last resort, delete objects from the last step more aggressively
		for(x = xmin; x <= xmax; x++){
			for(y = ymax; y != ymin; y -= inc){
				sect = CWorld::GetSector(x, y);
				if(DeleteRwObjectsBehindCameraInSectorList(sect->m_lists[ENTITYLIST_BUILDINGS], mem) ||
				   DeleteRwObjectsBehindCameraInSectorList(sect->m_lists[ENTITYLIST_DUMMIES], mem) ||
				   DeleteRwObjectsBehindCameraInSectorList(sect->m_lists[ENTITYLIST_OBJECTS], mem))
					return;
			}
		}
	}

	while(ms_memoryUsed >= mem && RemoveLeastUsedModel(0));
}

void
CStreaming::DeleteRwObjectsInSectorList(CPtrList &list)
{
	CPtrNode *node;
	CEntity *e;

	for(node = list.first; node; node = node->next){
		e = (CEntity*)node->item;
		if(!e->bStreamingDontDelete && !e->bImBeingRendered)
			e->DeleteRwObject();
	}
}

void
CStreaming::DeleteRwObjectsInOverlapSectorList(CPtrList &list, int32 x, int32 y)
{
	CPtrNode *node;
	CEntity *e;

	for(node = list.first; node; node = node->next){
		e = (CEntity*)node->item;
		if(e->m_rwObject && !e->bStreamingDontDelete && !e->bImBeingRendered){
			// Now this is pretty weird...
			if(Abs(CWorld::GetSectorIndexX(e->GetPosition().x) - x) >= 1.6f)
//			{
				e->DeleteRwObject();
//				return;		// BUG?
//			}
			else	// FIX?
			if(Abs(CWorld::GetSectorIndexY(e->GetPosition().y) - y) >= 1.6f)
				e->DeleteRwObject();
		}
	}
}

bool
CStreaming::DeleteRwObjectsBehindCameraInSectorList(CPtrList &list, size_t mem)
{
	CPtrNode *node;
	CEntity *e;

	for(node = list.first; node; node = node->next){
		e = (CEntity*)node->item;
		if(!e->bStreamingDontDelete && !e->bImBeingRendered &&
		   e->m_rwObject && ms_aInfoForModel[e->GetModelIndex()].m_next &&
		   FindPlayerPed()->m_pCurSurface != e){
			e->DeleteRwObject();
			if (CModelInfo::GetModelInfo(e->GetModelIndex())->GetNumRefs() == 0) {
				RemoveModel(e->GetModelIndex());
				if(ms_memoryUsed < mem)
					return true;
			}
		}
	}
	return false;
}

bool
CStreaming::DeleteRwObjectsNotInFrustumInSectorList(CPtrList &list, size_t mem)
{
	CPtrNode *node;
	CEntity *e;

	for(node = list.first; node; node = node->next){
		e = (CEntity*)node->item;
		if(!e->bStreamingDontDelete && !e->bImBeingRendered &&
		   e->m_rwObject && (!e->IsVisible() || e->bOffscreen) && ms_aInfoForModel[e->GetModelIndex()].m_next){
			e->DeleteRwObject();
			if (CModelInfo::GetModelInfo(e->GetModelIndex())->GetNumRefs() == 0) {
				RemoveModel(e->GetModelIndex());
				if(ms_memoryUsed < mem)
					return true;
			}
		}
	}
	return false;
}

void
CStreaming::MakeSpaceFor(int32 size)
{
#ifdef FIX_BUGS
#define MB (1024 * 1024)
	if(ms_memoryAvailable == 0) {
		extern size_t _dwMemAvailPhys;
		ms_memoryAvailable = (_dwMemAvailPhys - 10 * MB) / 2;
		if(ms_memoryAvailable < 65 * MB) ms_memoryAvailable = 65 * MB;
	}
#undef MB
#endif
	while(ms_memoryUsed >= ms_memoryAvailable - size)
		if(!RemoveLeastUsedModel(STREAMFLAGS_20)){
			DeleteRwObjectsBehindCamera(ms_memoryAvailable - size);
			return;
		}
}

void
CStreaming::LoadScene(const CVector &pos)
{
	CStreamingInfo *si, *prev;
	eLevelName level;

	level = CTheZones::GetLevelFromPosition(&pos);
	debug("Start load scene\n");
	for(si = ms_endRequestedList.m_prev; si != &ms_startRequestedList; si = prev){
		prev = si->m_prev;
		if((si->m_flags & (STREAMFLAGS_KEEP_IN_MEMORY|STREAMFLAGS_PRIORITY)) == 0)
			RemoveModel(si - ms_aInfoForModel);
	}
	CRenderer::m_loadingPriority = false;
	DeleteAllRwObjects();
	if(level == LEVEL_GENERIC)
		level = CGame::currLevel;
	CGame::currLevel = level;
	RemoveUnusedBigBuildings(level);
	RequestBigBuildings(level, pos);
	RequestBigBuildings(LEVEL_GENERIC, pos);
	RemoveIslandsNotUsed(level);
	LoadAllRequestedModels(false);
	InstanceBigBuildings(level, pos);
	InstanceBigBuildings(LEVEL_GENERIC, pos);
	AddModelsToRequestList(pos, STREAMFLAGS_20);
	CRadar::StreamRadarSections(pos);

	if (!CGame::IsInInterior()) {
		for (int i = 0; i < 5; i++) {
			CZoneInfo zone;
			CTheZones::GetZoneInfoForTimeOfDay(&pos, &zone);
			int32 model = CCarCtrl::ChooseCarModelToLoad(CCarCtrl::ChooseCarRating(&zone));
			CStreaming::RequestModel(model, STREAMFLAGS_DEPENDENCY);
		}
	}
	LoadAllRequestedModels(false);
	InstanceLoadedModels(pos);

	for(int i = 0; i < NUMSTREAMINFO; i++)
		ms_aInfoForModel[i].m_flags &= ~STREAMFLAGS_20;
	debug("End load scene\n");
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
int CStreaming::gWebLoadBudget = 0;
// F6b: el tope por CONTEO no ve el PESO (un TXD/DFF pesado cuesta 10-25 ms,
// así que 6 ficheros podían ser 60 ms en una imagen). Probé un tope por
// TIEMPO y crasheaba el init (punto de sincronía cortado) → fuera. El reparto
// de lo pesado, si vuelve, va en el LLAMADOR (R1).
// PERF (FPSLOG strm=): ms dentro de Update->LoadRequestedModels acumulados
// desde el último FPSLOG (lo resetea main). Medido en el LLAMADOR, nunca
// dentro del bucle suspendible.
uint32 gWebStrmMs = 0;

int
CStreaming::CountPendingRequests(void)
{
	int n = 0;
	for (CStreamingInfo *si = ms_startRequestedList.m_next; si != &ms_endRequestedList; si = si->m_next)
		n++;
	return n;
}

static int s_scenePhase = 0;
static CVector s_scenePos;
static eLevelName s_sceneLevel = LEVEL_GENERIC;
static int s_sceneTotal = 1;
static int s_sceneStale = 0;

void
CStreaming::LoadSceneResetSteps(void)
{
	s_scenePhase = 0;
	s_sceneTotal = 1;
	s_sceneStale = 0;
	gWebLoadBudget = 10; // ficheros por tick; se libera al terminar
}

// Escena de carga troceada: mismo orden y operaciones que LoadScene, un tramo
// por tick del navegador. 1 = terminado.
int
CStreaming::LoadSceneStep(const CVector &pos)
{
	switch (s_scenePhase) {
	case 0: {
		// Purga y peticiones de edificios grandes. Igual que LoadScene.
		// (La colisión corre aparte en el paso final, como en el tail vanilla.)
		s_scenePos = pos;
		CStreamingInfo *si, *prev;
		for (si = ms_endRequestedList.m_prev; si != &ms_startRequestedList; si = prev) {
			prev = si->m_prev;
			if ((si->m_flags & (STREAMFLAGS_KEEP_IN_MEMORY|STREAMFLAGS_PRIORITY)) == 0)
				RemoveModel(si - ms_aInfoForModel);
		}
		CRenderer::m_loadingPriority = false;
		DeleteAllRwObjects();
		s_sceneLevel = CTheZones::GetLevelFromPosition(&pos);
		if (s_sceneLevel == LEVEL_GENERIC)
			s_sceneLevel = CGame::currLevel;
		CGame::currLevel = s_sceneLevel;
		RemoveUnusedBigBuildings(s_sceneLevel);
		RequestBigBuildings(s_sceneLevel, pos);
		RequestBigBuildings(LEVEL_GENERIC, pos);
		RemoveIslandsNotUsed(s_sceneLevel);
		s_sceneTotal = CountPendingRequests();
		if (s_sceneTotal < 1) s_sceneTotal = 1;
		s_sceneStale = 0;
		gWebLoadFrac = 0.12f;
		s_scenePhase = 1;
		return 0;
	}
	case 1: {
		// Primera oleada (edificios grandes), a rebanadas por tick.
		int before = CountPendingRequests();
		LoadAllRequestedModels(false);
		int after = CountPendingRequests();
		if (after >= before) s_sceneStale++;
		else s_sceneStale = 0;
		int done = s_sceneTotal - after;
		if (done < 0) done = 0;
		gWebLoadFrac = 0.12f + 0.43f*done/s_sceneTotal;
		if (after == 0 || s_sceneStale > 5) {
			InstanceBigBuildings(s_sceneLevel, s_scenePos);
			InstanceBigBuildings(LEVEL_GENERIC, s_scenePos);
			AddModelsToRequestList(s_scenePos, STREAMFLAGS_20);
			CRadar::StreamRadarSections(s_scenePos);
			if (!CGame::IsInInterior()) {
				for (int i = 0; i < 5; i++) {
					CZoneInfo zone;
					CTheZones::GetZoneInfoForTimeOfDay(&s_scenePos, &zone);
					int32 model = CCarCtrl::ChooseCarModelToLoad(CCarCtrl::ChooseCarRating(&zone));
					CStreaming::RequestModel(model, STREAMFLAGS_DEPENDENCY);
				}
			}
			s_sceneTotal = CountPendingRequests();
			if (s_sceneTotal < 1) s_sceneTotal = 1;
			s_sceneStale = 0;
			gWebLoadFrac = 0.57f;
			s_scenePhase = 2;
		}
		return 0;
	}
	case 2: {
		// Segunda oleada (zona + radar + coches), a rebanadas por tick.
		int before = CountPendingRequests();
		LoadAllRequestedModels(false);
		int after = CountPendingRequests();
		if (after >= before) s_sceneStale++;
		else s_sceneStale = 0;
		int done = s_sceneTotal - after;
		if (done < 0) done = 0;
		gWebLoadFrac = 0.57f + 0.38f*done/s_sceneTotal;
		if (after == 0 || s_sceneStale > 5) {
			InstanceLoadedModels(s_scenePos);
			for (int i = 0; i < NUMSTREAMINFO; i++)
				ms_aInfoForModel[i].m_flags &= ~STREAMFLAGS_20;
			// F1 (fluides-v2): no liberar a 0 — presupuesto de gameplay
			// (6 ficheros/LoadAll) para acotar el peor frame conduciendo.
			gWebLoadBudget = 6;
			gWebLoadFrac = 1.0f;
			s_scenePhase = 0;
			return 1;
		}
		return 0;
	}
	default:
		s_scenePhase = 0;
		gWebLoadBudget = 6;
		return 1;
	}
}
#endif

void
CStreaming::LoadSceneCollision(const CVector &pos)
{
	CColStore::LoadCollision(pos);
	CStreaming::LoadAllRequestedModels(false);
}

void
CStreaming::MemoryCardSave(uint8 *buf, uint32 *size)
{
	int i;

	*size = NUM_DEFAULT_MODELS;
	for(i = 0; i < NUM_DEFAULT_MODELS; i++)
		if(ms_aInfoForModel[i].m_loadState == STREAMSTATE_LOADED)
			buf[i] = ms_aInfoForModel[i].m_flags;
		else
			buf[i] = 0xFF;
}

void 
CStreaming::MemoryCardLoad(uint8 *buf, uint32 size)
{
	uint32 i;

	assert(size == NUM_DEFAULT_MODELS);
	for(i = 0; i < size; i++)
		if(ms_aInfoForModel[i].m_loadState == STREAMSTATE_LOADED)
			if(buf[i] != 0xFF)
				ms_aInfoForModel[i].m_flags = buf[i];
}

void
CStreaming::UpdateForAnimViewer(void)
{
	if (CStreaming::ms_channelError == -1) {
		CStreaming::AddModelsToRequestList(CVector(0.0f, 0.0f, 0.0f), 0);
		CStreaming::LoadRequestedModels();
		// original modifier was %d
		sprintf(gString, "Requested %d, memory size %zuK\n", CStreaming::ms_numModelsRequested, 2 * CStreaming::ms_memoryUsed);
	}
	else {
		CStreaming::RetryLoadFile(CStreaming::ms_channelError);
	}
}


void
CStreaming::PrintStreamingBufferState()
{
	char str[128];
	wchar wstr[128];
	uint32 offset, size;

	CTimer::Stop();
	int i = 0;
	while (i < NUMSTREAMINFO) {
		while (true) {
			int j = 0;
			DoRWStuffStartOfFrame(50, 50, 50, 0, 0, 0, 255);
			CPad::UpdatePads();
			CSprite2d::InitPerFrame();
			CFont::InitPerFrame();
			DefinedState();

			CRect unusedRect(0, 0, RsGlobal.maximumWidth, RsGlobal.maximumHeight);
			CRGBA unusedColor(255, 255, 255, 255);
			CFont::SetFontStyle(FONT_BANK);
			CFont::SetBackgroundOff();
			CFont::SetWrapx(DEFAULT_SCREEN_WIDTH);
			CFont::SetScale(0.5f, 0.75f);
			CFont::SetCentreOff();
			CFont::SetCentreSize(DEFAULT_SCREEN_WIDTH);
			CFont::SetJustifyOff();
			CFont::SetColor(CRGBA(200, 200, 200, 200));
			CFont::SetBackGroundOnlyTextOff();
			int modelIndex = i;
			if (modelIndex < NUMSTREAMINFO) {
				int y = 24;
				for ( ; j < 34 && modelIndex < NUMSTREAMINFO; modelIndex++) {
					CStreamingInfo *streamingInfo = &ms_aInfoForModel[modelIndex];
					CBaseModelInfo *modelInfo = CModelInfo::GetModelInfo(modelIndex);
					if (streamingInfo->m_loadState != STREAMSTATE_LOADED || !streamingInfo->GetCdPosnAndSize(offset, size))
						continue;

					if (modelIndex >= STREAM_OFFSET_TXD)
						sprintf(str, "txd %s, refs %d, size %dK, flags 0x%x", CTxdStore::GetTxdName(modelIndex - STREAM_OFFSET_TXD),
						        CTxdStore::GetNumRefs(modelIndex - STREAM_OFFSET_TXD), 2 * size, streamingInfo->m_flags);
					else
						sprintf(str, "model %d,%s, refs%d, size%dK, flags%x", modelIndex, modelInfo->GetModelName(), modelInfo->GetNumRefs(), 2 * size,
						        streamingInfo->m_flags);
					AsciiToUnicode(str, wstr);
					CFont::PrintString(24.0f, y, wstr);
					y += 12;
					j++;
				}
			}

			if (CPad::GetPad(1)->GetCrossJustDown())
				i = modelIndex;

			if (!CPad::GetPad(1)->GetTriangleJustDown())
				break;

			i = 0;
			CFont::DrawFonts();
			DoRWStuffEndOfFrame();
		}
		CFont::DrawFonts();
		DoRWStuffEndOfFrame();
	}
	CTimer::Update();
}
