#include "common.h"
#include "PedArbiter.h"
#include "platform.h"
#include "ondemand.h"
#include "crossplatform.h"
#include "Lists.h"
#include "PlayerInfo.h"
#ifdef __EMSCRIPTEN__
#include <emscripten/heap.h>
#endif
#include "Game.h"
#include "main.h"
#include "RwHelper.h"
#include "Accident.h"
#include "Antennas.h"
#include "Bridge.h"
#include "CarCtrl.h"
#include "CarGen.h"
#include "CdStream.h"
#include "Clock.h"
#include "Clouds.h"
#include "Collision.h"
#include "ColStore.h"
#include "Console.h"
#include "Coronas.h"
#include "Cranes.h"
#include "Credits.h"
#include "CutsceneMgr.h"
#include "DMAudio.h"
#include "Darkel.h"
#include "Debug.h"
#include "EventList.h"
#include "FileLoader.h"
#include "FileMgr.h"
#include "Fire.h"
#include "Fluff.h"
#include "Font.h"
#include "Frontend.h"
#include "frontendoption.h"
#include "GameLogic.h"
#include "Garages.h"
#include "GenericGameStorage.h"
#include "Glass.h"
#include "HandlingMgr.h"
#include "Heli.h"
#include "Hud.h"
#include "IniFile.h"
#include "Lights.h"
#include "MBlur.h"
#include "Messages.h"
#include "MemoryCard.h"
#include "MemoryHeap.h"
#include "Pad.h"
#include "Particle.h"
#include "ParticleObject.h"
#include "PedRoutes.h"
#include "Phones.h"
#include "Pickups.h"
#include "Plane.h"
#include "PlayerSkin.h"
#include "Population.h"
#include "Radar.h"
#include "Record.h"
#include "References.h"
#include "Renderer.h"
#include "Replay.h"
#include "Restart.h"
#include "RoadBlocks.h"
#include "Rubbish.h"
#include "SceneEdit.h"
#include "Script.h"
#include "Shadows.h"
#include "Skidmarks.h"
#include "SetPieces.h"
#include "SpecialFX.h"
#include "Stats.h"
#include "Streaming.h"
#include "SurfaceTable.h"
#include "TempColModels.h"
#include "Timecycle.h"
#include "TrafficLights.h"
#include "Train.h"
#include "TxdStore.h"
#include "User.h"
#include "VisibilityPlugins.h"
#include "WaterCannon.h"
#include "WaterLevel.h"
#include "Weapon.h"
#include "WeaponEffects.h"
#include "Weather.h"
#include "World.h"
#include "ZoneCull.h"
#include "Zones.h"
#include "Occlusion.h"
#include "debugmenu.h"
#include "Ropes.h"
#include "WindModifiers.h"
#include "WaterCreatures.h"
#include "postfx.h"
#include "custompipes.h"
#include "screendroplets.h"
#include "VarConsole.h"
#ifdef USE_TEXTURE_POOL
#include "TexturePools.h"
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
// Web loading screen: the page shows progress per Initialise step
// (see window.__loadProgress in gta_vc_browser/web/lib/index.js).
#define WEB_LOAD_PROGRESS(step, total) EM_ASM({ if (window.__loadProgress) window.__loadProgress($0, $1); }, (step), (total))
// Overlay DOM de carga de partida (portada + barra): el splash GL no sale en web.
#define WEB_LOAD_OVERLAY(show, pct, label) EM_ASM({ if (window.__loadOverlay) window.__loadOverlay($0, $1, UTF8ToString($2)); }, (show), (pct), (label))
#else
#define WEB_LOAD_PROGRESS(step, total) ((void)0)
#define WEB_LOAD_OVERLAY(show, pct, label) ((void)0)
#endif
#define NUM_INIT_STEPS 19

eLevelName CGame::currLevel;
int32 CGame::currArea;
bool CGame::bDemoMode = true;
bool CGame::nastyGame = true;
bool CGame::frenchGame;
bool CGame::germanGame;
bool CGame::noProstitutes;
bool CGame::playingIntro;
char CGame::aDatFile[32];
#ifdef MORE_LANGUAGES
bool CGame::russianGame = false;
bool CGame::japaneseGame = false;
#endif
#ifndef MASTER
CVector CGame::PlayerCoords;
bool8 CGame::VarUpdatePlayerCoords;
#endif

int gameTxdSlot;

#ifdef SECUROM
uint8 gameProcessPirateCheck = 0;
#endif

bool DoRWStuffStartOfFrame(int16 TopRed, int16 TopGreen, int16 TopBlue, int16 BottomRed, int16 BottomGreen, int16 BottomBlue, int16 Alpha);
void DoRWStuffEndOfFrame(void);
#ifdef PS2_MENU
void MessageScreen(char *msg)
{
	//TODO: stretch_screen
	
	CRect rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	CRGBA color(255, 255, 255, 255);

	DoRWStuffStartOfFrame(50, 50, 50, 0, 0, 0, 255);
	
	CSprite2d::InitPerFrame();
	CFont::InitPerFrame();
	DefinedState();

	CSprite2d *splash = LoadSplash(NULL);
	splash->Draw(rect, color, color, color, color);
	splash->DrawRect(CRect(SCREEN_SCALE_X(20.0f), SCREEN_SCALE_Y(110.0f), SCREEN_SCALE_X(620.0f), SCREEN_SCALE_Y(300.0f)), CRGBA(50, 50, 50, 192));
	
	CFont::SetFontStyle(FONT_BANK);
	CFont::SetBackgroundOff();
	CFont::SetWrapx(SCREEN_SCALE_FROM_RIGHT(190.0f)); // 450.0f
	CFont::SetScale(SCREEN_SCALE_X(1.0f), SCREEN_SCALE_Y(1.0f));
	CFont::SetCentreOn();
	CFont::SetCentreSize(SCREEN_SCALE_X(450.0f));
	CFont::SetJustifyOff();
	CFont::SetColor(CRGBA(255, 255, 255, 255));
	CFont::SetDropColor(CRGBA(32, 32, 32, 255));
	CFont::SetDropShadowPosition(3);
	CFont::SetPropOn();
	CFont::PrintString(SCREEN_SCALE_X(320.0f), SCREEN_SCALE_Y(130.0f), TheText.Get(msg));
	CFont::DrawFonts();
	
	DoRWStuffEndOfFrame();
}
#endif

bool
CGame::InitialiseOnceBeforeRW(void)
{
	CFileMgr::Initialise();
	CdStreamInit(MAX_CDCHANNELS);
	debug("size of matrix %d\n", sizeof(CMatrix));
	debug("size of placeable %d\n", sizeof(CPlaceable));
	debug("size of entity %d\n", sizeof(CEntity));
	debug("size of building %d\n", sizeof(CBuilding));
	debug("size of dummy %d\n", sizeof(CDummy));
#ifdef EXTENDED_COLOURFILTER
	CPostFX::InitOnce();
#endif
#ifdef CUSTOM_FRONTEND_OPTIONS
	// Not needed here but may be needed in future
	// if (numCustomFrontendOptions == 0 && numCustomFrontendScreens == 0)
	CustomFrontendOptionsPopulate();
#endif
	return true;
}

#ifndef LIBRW
#ifdef PS2_MATFX
void ReplaceMatFxCallback();
#endif // PS2_MATFX
#ifdef PS2_ALPHA_TEST
void ReplaceAtomicPipeCallback();
#endif // PS2_ALPHA_TEST
#endif // !LIBRW

bool
CGame::InitialiseRenderWare(void)
{
	ValidateVersion();
#ifdef USE_TEXTURE_POOL
	_TexturePoolsInitialise();
#endif

	CTxdStore::Initialise();
	CVisibilityPlugins::Initialise();

#ifdef GTA_PS2
	RpSkySelectTrueTSClipper(TRUE);
	RpSkySelectTrueTLClipper(TRUE);

	// PS2ManagerApplyDirectionalLightingCB() uploads the GTA lights
	// directly without going through RpWorld and all that
	SetupPS2ManagerDefaultLightingCallback();
	PreAllocateRwObjects();
#endif

	/* Create camera */
	Scene.camera = CameraCreate(SCREEN_WIDTH, SCREEN_HEIGHT, TRUE);
	ASSERT(Scene.camera != nil);
	if (!Scene.camera)
	{
		return (false);
	}
	
	RwCameraSetFarClipPlane(Scene.camera, 2000.0f);
	RwCameraSetNearClipPlane(Scene.camera, 0.9f);
	
	CameraSize(Scene.camera, nil, DEFAULT_VIEWWINDOW, DEFAULT_ASPECT_RATIO);
	
	/* Create a world */
	RwBBox  bbox;
	
	bbox.sup.x = bbox.sup.y = bbox.sup.z = 10000.0f;
	bbox.inf.x = bbox.inf.y = bbox.inf.z = -10000.0f;

	Scene.world = RpWorldCreate(&bbox);
	ASSERT(Scene.world != nil);
	if (!Scene.world)
	{
		CameraDestroy(Scene.camera);
		Scene.camera = nil;
		return (false);
	}
	
	/* Add the camera to the world */
	RpWorldAddCamera(Scene.world, Scene.camera);
	LightsCreate(Scene.world);

	CreateDebugFont();

#ifdef LIBRW
#ifdef PS2_MATFX
	rw::MatFX::envMapApplyLight = true;
	rw::MatFX::envMapUseMatColor = true;
	rw::MatFX::envMapFlipU = true;
#else
	rw::MatFX::envMapApplyLight = false;
	rw::MatFX::envMapUseMatColor = false;
	rw::MatFX::envMapFlipU = false;
#endif
	rw::RGBA envcol = { 64, 64, 64, 255 };
	rw::MatFX::envMapColor = envcol;
#else
#ifdef PS2_MATFX
	ReplaceMatFxCallback();
#endif // PS2_MATFX
#ifdef PS2_ALPHA_TEST
	ReplaceAtomicPipeCallback();
#endif // PS2_ALPHA_TEST
#endif // LIBRW

	PUSH_MEMID(MEMID_TEXTURES);
	CFont::Initialise();
	CHud::Initialise();
	CPlayerSkin::Initialise();
	POP_MEMID();

#ifdef EXTENDED_PIPELINES
	CustomPipes::CustomPipeInit();	// need Scene.world for this
#endif
#ifdef SCREEN_DROPLETS
	ScreenDroplets::InitDraw();
#endif

	return (true);
}

void CGame::ShutdownRenderWare(void)
{
#ifdef SCREEN_DROPLETS
	ScreenDroplets::Shutdown();
#endif
#ifdef EXTENDED_PIPELINES
	CustomPipes::CustomPipeShutdown();
#endif

	DestroySplashScreen();
	CHud::Shutdown();
	CFont::Shutdown();
	
	for ( int32 i = 0; i < NUMPLAYERS; i++ )
		CWorld::Players[i].DeletePlayerSkin();

	CPlayerSkin::Shutdown();
	
	DestroyDebugFont();
	
	/* Destroy world */
	LightsDestroy(Scene.world);
	RpWorldRemoveCamera(Scene.world, Scene.camera);
	RpWorldDestroy(Scene.world);
	
	/* destroy camera */
	CameraDestroy(Scene.camera);
	
	Scene.world = nil;
	Scene.camera = nil;
	
	CVisibilityPlugins::Shutdown();
	
#ifdef USE_TEXTURE_POOL
	_TexturePoolsShutdown();
#endif
}

bool CGame::InitialiseOnceAfterRW(void)
{
	TheText.Load();
	CTimer::Initialise();
	CTempColModels::Initialise();
	mod_HandlingManager.Initialise();
	CSurfaceTable::Initialise("DATA\\SURFACE.DAT");
	CPedStats::Initialise();
	CTimeCycle::Initialise();
#ifdef GTA_PS2
	LoadingScreen("Loading the Game", "Initialising audio", GetRandomSplashScreen());
#endif
	DMAudio.Initialise();

#ifndef GTA_PS2
#ifdef EXTERNAL_3D_SOUND
	if ( DMAudio.GetNum3DProvidersAvailable() == 0 )
		FrontEndMenuManager.m_nPrefsAudio3DProviderIndex = NO_AUDIO_PROVIDER;

	if ( FrontEndMenuManager.m_nPrefsAudio3DProviderIndex == AUDIO_PROVIDER_NOT_DETERMINED || FrontEndMenuManager.m_nPrefsAudio3DProviderIndex == -2 )
	{
		FrontEndMenuManager.m_PrefsSpeakers = 0;
		FrontEndMenuManager.m_nPrefsAudio3DProviderIndex = DMAudio.AutoDetect3DProviders();
	}

	DMAudio.SetCurrent3DProvider(FrontEndMenuManager.m_nPrefsAudio3DProviderIndex);
	DMAudio.SetSpeakerConfig(FrontEndMenuManager.m_PrefsSpeakers);
#endif
	DMAudio.SetDynamicAcousticModelingStatus(FrontEndMenuManager.m_PrefsDMA);
	DMAudio.SetMusicMasterVolume(FrontEndMenuManager.m_PrefsMusicVolume);
	DMAudio.SetEffectsMasterVolume(FrontEndMenuManager.m_PrefsSfxVolume);
	DMAudio.SetEffectsFadeVol(127);
	DMAudio.SetMusicFadeVol(127);
#endif
#ifdef __EMSCRIPTEN__
	printf("[web] once-afterRW ok\n");
#endif
	return true;
}

void
CGame::FinalShutdown(void)
{	
	CTxdStore::Shutdown();
	CPedStats::Shutdown();
	CdStreamShutdown();
}

static int s_initStep = 0;

void CGame::InitialiseResetSteps(void)
{
	s_initStep = 0;
}

// One loading section per call; returns true when finished.
// Shared by native (loops in one frame) and web (one per tick).
bool CGame::InitialiseStep(const char* datFile)
{
	(void)datFile;
#ifdef __EMSCRIPTEN__
	// Solo en el arranque (un puñado de líneas): el paso de carga por el que va
	// y el heap, que es lo que se mira cuando el arranque se queda corto.
	{ char ob[96]; snprintf(ob, sizeof(ob), "initstep %d heap %u", s_initStep, (unsigned)emscripten_get_heap_size()); ODTRACES(ob); }
#endif
	switch (s_initStep) {
	case 0:
	{
		ResetLoadingScreenBar();
		strcpy(aDatFile, datFile);

#ifdef GTA_PS2
		// TODO: upload VU0 collision code here
#endif

		CPools::Initialise();

#ifndef GTA_PS2
#ifdef PED_CAR_DENSITY_SLIDERS
		// Load density values from gta3.ini only if our reVC.ini have them 0.6f
		if (CIniFile::PedNumberMultiplier == 0.6f && CIniFile::CarNumberMultiplier == 0.6f)
#endif
			CIniFile::LoadIniFile();
#endif
#ifdef USE_TEXTURE_POOL
		_TexturePoolsUnknown(false);
#endif
		currLevel = LEVEL_BEACH;
		currArea = AREA_MAIN_MAP;
#ifdef __EMSCRIPTEN__
		// F1 (fluides-v2): presupuesto de streaming también para partida
		// nueva (6 ficheros por LoadAll durante gameplay; el mecanismo ya
		// validado por la carga). Acota el peor frame al conducir.
		CStreaming::gWebLoadBudget = 6;
#endif

		PUSH_MEMID(MEMID_TEXTURES);
		break;
	}
	case 1:
	{
		LoadingScreen("Loading the Game", "Loading generic textures", GetRandomSplashScreen());
		gameTxdSlot = CTxdStore::AddTxdSlot("generic");
		CTxdStore::Create(gameTxdSlot);
		CTxdStore::AddRef(gameTxdSlot);

#ifdef EXTENDED_PIPELINES
		// for generic fallback
		CustomPipes::SetTxdFindCallback();
#endif

		break;
	}
	case 2:
	{
		LoadingScreen("Loading the Game", "Loading particles", nil);
		int particleTxdSlot = CTxdStore::AddTxdSlot("particle");
		CTxdStore::LoadTxd(particleTxdSlot, "MODELS/PARTICLE.TXD");
		CTxdStore::AddRef(particleTxdSlot);
		CTxdStore::SetCurrentTxd(gameTxdSlot);
		break;
	}
	case 3:
	{
		LoadingScreen("Loading the Game", "Setup game variables", nil);
		POP_MEMID();

#ifdef GTA_PS2
		CDma::SyncChannel(0, true);
#endif

		CGameLogic::InitAtStartOfGame();
		CReferences::Init();
		TheCamera.Init();
		TheCamera.SetRwCamera(Scene.camera);
		CDebug::DebugInitTextBuffer();
		ThePaths.Init();
		ThePaths.AllocatePathFindInfoMem(4500);
		CScriptPaths::Init();
		CWeather::Init();
		CCullZones::Init();
		COcclusion::Init();
		CCollision::Init();
		CSetPieces::Init();
		CTheZones::Init();
		CUserDisplay::Init();
		CMessages::Init();
		CMessages::ClearAllMessagesDisplayedByGame();
		CRecordDataForGame::Init();
		CRestart::Initialise();

		PUSH_MEMID(MEMID_WORLD);
		CWorld::Initialise();
		POP_MEMID();

		PUSH_MEMID(MEMID_TEXTURES);
		CParticle::Initialise();
		POP_MEMID();

		PUSH_MEMID(MEMID_ANIMATION);
		CAnimManager::Initialise();
		CCutsceneMgr::Initialise();
		POP_MEMID();

		PUSH_MEMID(MEMID_CARS);
		CCarCtrl::Init();
		POP_MEMID();

		PUSH_MEMID(MEMID_DEF_MODELS);
		InitModelIndices();
		CModelInfo::Initialise();
		CPickups::Init();
		CTheCarGenerators::Init();

		CdStreamAddImage("MODELS\\GTA3.IMG");

		CFileLoader::LoadLevel("DATA\\DEFAULT.DAT");

		break;
	}
	case 4:
	{
		// Web: GTA_VC.DAT (IDE/IPL/TXD masivos) en su propio tick para no
		// bloquear la pestaña 10-60 s junto al setup anterior.
		CFileLoader::LoadLevel(aDatFile);
		LoadingScreen("Loading the Game", "Add Particles", nil);
		CWorld::AddParticles();
		CVehicleModelInfo::LoadVehicleColours();
		CVehicleModelInfo::LoadEnvironmentMaps();
		CTheZones::PostZoneCreation();
		POP_MEMID();

		break;
	}
	case 5:
	{
		LoadingScreen("Loading the Game", "Setup paths", nil);
		ThePaths.PreparePathData();
		for (int i = 0; i < NUMPLAYERS; i++)
			CWorld::Players[i].Clear();
		CWorld::Players[0].LoadPlayerSkin();
		TestModelIndices();

		break;
	}
	case 6:
	{
		LoadingScreen("Loading the Game", "Setup water", nil);
		CWaterLevel::Initialise("DATA\\WATER.DAT");
		TheConsole.Init();
		CDraw::SetFOV(120.0f);
		CDraw::ms_fLODDistance = 500.0f;

		break;
	}
	case 7:
	{
		LoadingScreen("Loading the Game", "Setup streaming", nil);
		CStreaming::LoadInitialVehicles();
		CStreaming::LoadInitialPeds();
		CStreaming::RequestBigBuildings(LEVEL_GENERIC);
		CStreaming::LoadAllRequestedModels(false);
		CStreaming::RemoveIslandsNotUsed(currLevel);
		printf("Streaming uses %zuK of its memory", CStreaming::ms_memoryUsed / 1024); // original modifier was %d

		break;
	}
	case 8:
	{
		LoadingScreen("Loading the Game", "Load animations", GetRandomSplashScreen());
		PUSH_MEMID(MEMID_ANIMATION);
		CAnimManager::LoadAnimFiles();
		POP_MEMID();

		CStreaming::LoadInitialWeapons();
		CStreaming::LoadAllRequestedModels(0);
		CPed::Initialise();
		CRouteNode::Initialise();
		CEventList::Initialise();
#ifdef SCREEN_DROPLETS
		ScreenDroplets::Initialise();
#endif
		break;
	}
	case 9:
	{
		LoadingScreen("Loading the Game", "Find big buildings", nil);
		CRenderer::Init();

		break;
	}
	case 10:
	{
		LoadingScreen("Loading the Game", "Setup game variables", nil);
		CRadar::Initialise();
		CRadar::LoadTextures();
		CWeapon::InitialiseWeapons();

		break;
	}
	case 11:
	{
		LoadingScreen("Loading the Game", "Setup traffic lights", nil);
		CTrafficLights::ScanForLightsOnMap();
		CRoadBlocks::Init();

		break;
	}
	case 12:
	{
		LoadingScreen("Loading the Game", "Setup game variables", nil);
		CPopulation::Initialise();
		CWorld::PlayerInFocus = 0;
		CCoronas::Init();
		CShadows::Init();
		CWeaponEffects::Init();
		CSkidmarks::Init();
		CAntennas::Init();
		CGlass::Init();
		gPhoneInfo.Initialise();
#ifdef GTA_SCENE_EDIT
		CSceneEdit::Initialise();
#endif

		break;
	}
	case 13:
	{
		LoadingScreen("Loading the Game", "Load scripts", nil);
		PUSH_MEMID(MEMID_SCRIPT);
		CTheScripts::Init();
		CGangs::Initialise();
		POP_MEMID();

		break;
	}
	case 14:
	{
		LoadingScreen("Loading the Game", "Setup game variables", nil);
		CClock::Initialise(1000);
		CHeli::InitHelis();
		CCranes::InitCranes();
		CMovingThings::Init();
		CDarkel::Init();
		CStats::Init();
		CPacManPickups::Init();
		CRubbish::Init();
		CClouds::Init();
		CSpecialFX::Init();
		CRopes::Init();
		CWaterCannons::Init();
		CBridge::Init();
		CGarages::Init();

		break;
	}
	case 15:
	{
		LoadingScreen("Loading the Game", "Position dynamic objects", nil);
		break;
	}
	case 16:
	{
		LoadingScreen("Loading the Game", "Initialise vehicle paths", nil);

		CTrain::InitTrains();
		CPlane::InitPlanes();
		CCredits::Init();
		CRecordDataForChase::Init();
		CReplay::Init();

		break;
	}
	case 17:
	{
		LoadingScreen("Loading the Game", "Start script", nil);
#ifdef PS2_MENU
		if ( !TheMemoryCard.m_bWantToLoad )
#elif defined(__EMSCRIPTEN__)
		// Web/carga: el save trae scripts, player y escena; no correr el
		// script de test ni el primer tick del main (era la tormenta TXDIN
		// duplicada: creaba player + escena por defecto que el restart
		// destruía y reconstruía en la playa).
		if ( !FrontEndMenuManager.m_bWantToLoad )
#endif
		{
			CTheScripts::StartTestScript();
			CTheScripts::Process();
			TheCamera.Process();
		}

		break;
	}
	case 18:
	{
		LoadingScreen("Loading the Game", "Load scene", nil);
		CCollision::ms_collisionInMemory = currLevel;
		for (int i = 0; i < MAX_PADS; i++)
			CPad::GetPad(i)->Clear(true);
#ifdef USE_TEXTURE_POOL
		_TexturePoolsUnknown(true);
#endif

#ifndef MASTER
		PlayerCoords = FindPlayerCoors();
		VarConsole.Add("X PLAYER COORD", &PlayerCoords.x, 10.0f, -10000.0f, 10000.0f, true);
		VarConsole.Add("Y PLAYER COORD", &PlayerCoords.y, 10.0f, -10000.0f, 10000.0f, true);
		VarConsole.Add("Z PLAYER COORD", &PlayerCoords.z, 10.0f, -10000.0f, 10000.0f, true);
		VarConsole.Add("UPDATE PLAYER COORD", &VarUpdatePlayerCoords, true);
#endif


		DMAudio.SetStartingTrackPositions(TRUE);
		DMAudio.ChangeMusicMode(MUSICMODE_GAME);
		break;
	}
	default:
		break;
	}
	WEB_LOAD_PROGRESS(s_initStep + 1, NUM_INIT_STEPS);
	s_initStep++;
	return s_initStep >= NUM_INIT_STEPS;
}

bool CGame::Initialise(const char* datFile)
{
	CGame::InitialiseResetSteps();
	while (!CGame::InitialiseStep(datFile)) { }
	return true;
}

bool CGame::ShutDown(void)
{
#ifdef USE_TEXTURE_POOL
	_TexturePoolsUnknown(false);
#endif
	CReplay::FinishPlayback();
	CReplay::EmptyReplayBuffer();
	CPlane::Shutdown();
	CTrain::Shutdown();
	CScriptPaths::Shutdown();
	CWaterCreatures::RemoveAll();
	CSpecialFX::Shutdown();
	CGarages::Shutdown();
	CMovingThings::Shutdown();
	gPhoneInfo.Shutdown();
	CWeapon::ShutdownWeapons();
	CPedType::Shutdown();
	
	for (int32 i = 0; i < NUMPLAYERS; i++)
	{
		if ( CWorld::Players[i].m_pPed )
		{
			CWorld::Remove(CWorld::Players[i].m_pPed);
			delete CWorld::Players[i].m_pPed;
			CWorld::Players[i].m_pPed = nil;
		}
		
		CWorld::Players[i].Clear();
	}
	
	CRenderer::Shutdown();
	CWorld::ShutDown();
	DMAudio.DestroyAllGameCreatedEntities();
	CModelInfo::ShutDown();
	CAnimManager::Shutdown();
	CCutsceneMgr::Shutdown();
	CVehicleModelInfo::DeleteVehicleColourTextures();
	CVehicleModelInfo::ShutdownEnvironmentMaps();
	CRadar::Shutdown();
	CStreaming::Shutdown();
	CTxdStore::GameShutdown();
	CCollision::Shutdown();
	CWaterLevel::Shutdown();
	CRubbish::Shutdown();
	CClouds::Shutdown();
	CShadows::Shutdown();
	CCoronas::Shutdown();
	CSkidmarks::Shutdown();
	CWeaponEffects::Shutdown();
	CParticle::Shutdown();
	CPools::ShutDown();
	CHud::ReInitialise();
	CTxdStore::RemoveTxdSlot(gameTxdSlot);
	CMBlur::MotionBlurClose();
	CdStreamRemoveImages();
#ifdef USE_TEXTURE_POOL
	_TexturePoolsFinalShutdown();
#endif
	return true;
}

void CGame::ReInitGameObjectVariables(void)
{
	CGameLogic::InitAtStartOfGame();
#ifdef PS2_MENU
	if ( !TheMemoryCard.m_bWantToLoad )
#endif
	{
		TheCamera.Init();
		TheCamera.SetRwCamera(Scene.camera);
	}
	CDebug::DebugInitTextBuffer();
	CWeather::Init();
	CUserDisplay::Init();
	CMessages::Init();
	CRestart::Initialise();
	CWorld::bDoingCarCollisions = false;
	CHud::ReInitialise();
	CRadar::Initialise();
	CCarCtrl::ReInit();
	CTimeCycle::Initialise();
	CDraw::SetFOV(120.0f);
	CDraw::ms_fLODDistance = 500.0f;
	CStreaming::RequestBigBuildings(LEVEL_GENERIC);
	CStreaming::RemoveIslandsNotUsed(LEVEL_BEACH);
	CStreaming::RemoveIslandsNotUsed(LEVEL_MAINLAND);
	CStreaming::LoadAllRequestedModels(false);
	currArea = AREA_MAIN_MAP;
	CPed::Initialise();
	CEventList::Initialise();
#ifdef SCREEN_DROPLETS
	ScreenDroplets::Initialise();
#endif
	CWeapon::InitialiseWeapons();
	CPopulation::Initialise();
	
	for (int i = 0; i < NUMPLAYERS; i++)
		CWorld::Players[i].Clear();
	
	CWorld::PlayerInFocus = 0;
	CAntennas::Init();
	CGlass::Init();
	gPhoneInfo.Initialise();

	PUSH_MEMID(MEMID_SCRIPT);
	CTheScripts::Init();
	CGangs::Initialise();
	POP_MEMID();

	CTimer::Initialise();
	CClock::Initialise(1000);
	CTheCarGenerators::Init();
	CHeli::InitHelis();
	CMovingThings::Init();
	CDarkel::Init();
	CStats::Init();
	CPickups::Init();
	CPacManPickups::Init();
	CGarages::Init();
	CSpecialFX::Init();
	CRopes::Init();
	CWaterCannons::Init();
	CScriptPaths::Init();
	CParticle::ReloadConfig();

#ifdef PS2_MENU
	if ( !TheMemoryCard.m_bWantToLoad )
#else
	if ( !FrontEndMenuManager.m_bWantToLoad )
#endif
	{
		CCranes::InitCranes();
		CTheScripts::StartTestScript();
		CTheScripts::Process();
		TheCamera.Process();
		CTrain::InitTrains();
		CPlane::InitPlanes();
	}
	
	for (int32 i = 0; i < MAX_PADS; i++)
		CPad::GetPad(i)->Clear(true);
}

void CGame::ReloadIPLs(void)
{
	// Empty and unused
}

void CGame::ShutDownForRestart(void)
{
#ifdef USE_TEXTURE_POOL
	_TexturePoolsUnknown(false);
#endif
	CReplay::FinishPlayback();
	CReplay::EmptyReplayBuffer();
	DMAudio.DestroyAllGameCreatedEntities();
	CMovingThings::Shutdown();
	
	for (int i = 0; i < NUMPLAYERS; i++)
		CWorld::Players[i].Clear();

	CGarages::SetAllDoorsBackToOriginalHeight();
	CTheScripts::UndoBuildingSwaps();
	CTheScripts::UndoEntityInvisibilitySettings();
	CWorld::ClearForRestart();
	CGameLogic::ClearShortCut();
	CTimer::Shutdown();
	CStreaming::ReInit();
	CRadar::RemoveRadarSections();
	FrontEndMenuManager.UnloadTextures();
	CParticleObject::RemoveAllExpireableParticleObjects();
	CWaterCreatures::RemoveAll(); 
	CSetPieces::Init();
	CPedType::Shutdown();
	CSpecialFX::Shutdown();
}

#ifdef __EMSCRIPTEN__
static int s_sdPhase = 0;
void CGame::ShutDownForRestartResetSteps(void)
{
	s_sdPhase = 0;
}
// Shutdown troceado: mismo orden y operaciones que ShutDownForRestart, un
// tramo por tick del navegador (el shutdown monolítico congelaba el confirm).
// 1 = terminado.
bool CGame::ShutDownForRestartStep(void)
{
	int i;
	switch (s_sdPhase) {
	case 0:
#ifdef USE_TEXTURE_POOL
		_TexturePoolsUnknown(false);
#endif
		CReplay::FinishPlayback();
		CReplay::EmptyReplayBuffer();
		DMAudio.DestroyAllGameCreatedEntities();
		CMovingThings::Shutdown();
		for (i = 0; i < NUMPLAYERS; i++)
			CWorld::Players[i].Clear();
		CGarages::SetAllDoorsBackToOriginalHeight();
		CTheScripts::UndoBuildingSwaps();
		CTheScripts::UndoEntityInvisibilitySettings();
		gWebLoadFrac = 0.01f;
		s_sdPhase = 1;
		return false;
	case 1:
		CWorld::ClearForRestart();
		CGameLogic::ClearShortCut();
		CTimer::Shutdown();
		gWebLoadFrac = 0.02f;
		s_sdPhase = 2;
		return false;
	case 2:
	default:
		CStreaming::ReInit();
		CRadar::RemoveRadarSections();
		FrontEndMenuManager.UnloadTextures();
		CParticleObject::RemoveAllExpireableParticleObjects();
		CWaterCreatures::RemoveAll();
		CSetPieces::Init();
		CPedType::Shutdown();
		CSpecialFX::Shutdown();
		gWebLoadFrac = 0.03f;
		s_sdPhase = 0;
		return true;
	}
}
#endif

void CGame::InitialiseWhenRestarting(void)
{
	ViceExtPedResetAll();
	CRect rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	CRGBA color(255, 255, 255, 255);
	
	CTimer::Initialise();
	CSprite2d::SetRecipNearClip();

	if (b_FoundRecentSavedGameWantToLoad || FrontEndMenuManager.m_bWantToLoad)
	{
		LoadSplash("splash1");
#ifndef XBOX_MESSAGE_SCREEN
		if (FrontEndMenuManager.m_bWantToLoad)
			FrontEndMenuManager.MessageScreen("FELD_WR", true);
#endif
	}

	b_FoundRecentSavedGameWantToLoad = false;
	
	TheCamera.Init();
	
	if ( FrontEndMenuManager.m_bWantToLoad == true )
	{
#ifdef XBOX_MESSAGE_SCREEN
		FrontEndMenuManager.SetDialogTimer(1000);
		DoRWStuffStartOfFrame(0, 0, 0, 0, 0, 0, 0);
		CSprite2d::InitPerFrame();
		CFont::InitPerFrame();
		FrontEndMenuManager.DrawOverlays();
		DoRWStuffEndOfFrame();
#endif
		RestoreForStartLoad();
	}
	
	ReInitGameObjectVariables();
	
	if ( FrontEndMenuManager.m_bWantToLoad == true )
	{
		FrontEndMenuManager.m_bWantToLoad = false;
		InitRadioStationPositionList();
		if ( GenericLoad() == true )
		{
			DMAudio.ResetTimers(CTimer::GetTimeInMilliseconds());
			CTrain::InitTrains();
			CPlane::InitPlanes();
		}
		else
		{
			for ( int32 i = 0; i < 50; i++ )
			{
				HandleExit();
				FrontEndMenuManager.MessageScreen("FED_LFL", true); // Loading save game has failed. The game will restart now. 
			}
			
			TheCamera.SetFadeColour(0, 0, 0);
			ShutDownForRestart();
			CTimer::Stop();
			CTimer::Initialise();
			FrontEndMenuManager.m_bWantToLoad = false;
			ReInitGameObjectVariables();
			currLevel = LEVEL_GENERIC;
			CCollision::SortOutCollisionAfterLoad();
		}
#ifdef XBOX_MESSAGE_SCREEN
		FrontEndMenuManager.ProcessDialogTimer();
#endif
	}
	
	CTimer::Update();

	DMAudio.ChangeMusicMode(MUSICMODE_GAME);
#ifdef USE_TEXTURE_POOL
	_TexturePoolsUnknown(true);
#endif
}

#ifdef __EMSCRIPTEN__
// Carga de partida troceada: un tramo por tick del navegador para que la
// pestaña no muestre "esperar/salir". Mismo orden que InitialiseWhenRestarting.
static int s_restartStep = 0;
#ifdef __EMSCRIPTEN__
static int s_loadSub = 0; // 0 = parse, 1 = drenado colisión, 2 = escena
	static uint32 s_loadT0 = 0; // DIAG: duración total de la carga (RsTimer, reloj de pared)
static int s_drainTotal = 1;
static int s_drainFirst = 1;
static int s_drainStale = 0;
#endif
void CGame::InitialiseRestartResetSteps(void)
{
	s_restartStep = 0;
#ifdef __EMSCRIPTEN__
	s_loadSub = 0;
#endif
}
bool CGame::InitialiseRestartStep(void)
{
	switch (s_restartStep) {
	case 0:
	{
		CTimer::Initialise();
		CSprite2d::SetRecipNearClip();
		if (b_FoundRecentSavedGameWantToLoad || FrontEndMenuManager.m_bWantToLoad)
			LoadSplash("splash1");
		b_FoundRecentSavedGameWantToLoad = false;
		TheCamera.Init();
		if (FrontEndMenuManager.m_bWantToLoad == true)
			RestoreForStartLoad();
#ifdef __EMSCRIPTEN__
		s_loadSub = 0;
		if (FrontEndMenuManager.m_bWantToLoad == true) {
			gWebLoadFrac = 0.01f;
			// Presupuesto desde ya: todo LoadAll del flujo trocea (caso 1,
			// cola de colisión, escena). 0 = monolítico (modo normal).
			CStreaming::gWebLoadBudget = 10;
			// Re-asegurar el splash (pudo desalojarse): IDB local, sin red.
			EM_ASM({ try { OD.ensure('TXD/splash1.txd'); } catch (e) {} });
		}
#endif
		WEB_LOAD_PROGRESS(1, 4);
		break;
	}
	case 1:
	{
		ReInitGameObjectVariables();
		WEB_LOAD_PROGRESS(2, 4);
		break;
	}
	case 2:
	{
		if (FrontEndMenuManager.m_bWantToLoad == true)
		{
			// NO limpiar aquí: los ticks restantes del restart troceado deben
			// seguir entrando al if (wantToLoad). Antes se limpiaba aquí y los
			// ticks siguientes caían al else (wipe INIT_PLAYING) pisando la
			// partida recién cargada. Se limpia al completar (glfw) o en fallo.
#ifdef __EMSCRIPTEN__
			// Web: parse en un tick + escena troceada en los siguientes (un
			// tramo por tick). Mismo orden que el tail vanilla.
			if (s_loadSub == 0) {
				InitRadioStationPositionList();
				ODTRACES("webload: GenericLoad empieza");
				s_loadT0 = (uint32)RsTimer(); // reloj de pared (F2): CTimer no avanza dentro del frame
				printf("[save] webload: GenericLoad empieza (slot %d)\n", FrontEndMenuManager.m_nCurrSaveSlot);
				gWebDeferSceneLoad = true;
				gWebDeferCollision = true;
				if (GenericLoad() == true) {
					s_loadSub = 1;
					s_drainTotal = 1;
					s_drainFirst = 1;
					s_drainStale = 0;
					gWebLoadFrac = 0.04f;
					s_restartStep--; // quedarse en el caso 2 para el drenado
				} else {
					ODTRACES("webload: GenericLoad FAIL -> restart silencioso");
					printf("[save] webload: GenericLoad FAIL -> restart silencioso\n");
					gWebDeferSceneLoad = false;
					gWebDeferCollision = false;
					CStreaming::gWebLoadBudget = 6; // F1: presupuesto de gameplay
					gWebLoadFrac = 0.0f;
					TheCamera.SetFadeColour(0, 0, 0);
					ShutDownForRestart();
					CTimer::Stop();
					CTimer::Initialise();
					FrontEndMenuManager.m_bWantToLoad = false;
					ReInitGameObjectVariables();
					currLevel = LEVEL_GENERIC;
					CCollision::SortOutCollisionAfterLoad();
				}
			} else if (s_loadSub == 1) {
				// Sub-paso 0b: colisión + resto pendiente, a rebanadas.
				// (La purga de la escena dropearía estas peticiones, así que
				// drenan ANTES de los pasos de escena.)
				if (s_drainFirst) {
					s_drainFirst = 0;
					CColStore::LoadCollision(TheCamera.GetPosition());
					s_drainTotal = CStreaming::CountPendingRequests();
					if (s_drainTotal < 1) s_drainTotal = 1;
					s_drainStale = 0;
				}
				int before = CStreaming::CountPendingRequests();
				CStreaming::LoadAllRequestedModels(false);
				int after = CStreaming::CountPendingRequests();
				if (after >= before) s_drainStale++;
				else s_drainStale = 0;
				int done = s_drainTotal - after;
				if (done < 0) done = 0;
				if (done > s_drainTotal) done = s_drainTotal;
				gWebLoadFrac = 0.04f + 0.06f*done/s_drainTotal;
				if (after == 0 || s_drainStale > 5) {
					CStreaming::LoadSceneResetSteps();
					s_loadSub = 2;
				}
				s_restartStep--;
			} else {
				if (CStreaming::LoadSceneStep(TheCamera.GetPosition())) {
					s_loadSub = 0;
					gWebDeferSceneLoad = false;
					gWebDeferCollision = false;
					DoGameSpecificStuffAfterSucessLoadDeferred();
					DMAudio.ResetTimers(CTimer::GetTimeInMilliseconds());
					CTrain::InitTrains();
					CPlane::InitPlanes();
					ODTRACES("webload: GenericLoad OK");
					printf("[save] webload: GenericLoad OK\n");
					{
						char t[96];
						snprintf(t, sizeof t, "webload: total=%ums (parse+drenado+escena)",
							(uint32)RsTimer() - s_loadT0);
						ODTRACES(t);
					}
					{
						CVector p(0.0f, 0.0f, 0.0f);
						CPlayerPed *pl = FindPlayerPed();
						if (pl) p = pl->GetPosition();
						char t[160];
						snprintf(t, sizeof t, "loaded pos=%.1f,%.1f,%.1f level=%d", p.x, p.y, p.z, (int)CGame::currLevel);
						ODTRACES(t);
						printf("[save] %s\n", t);
					}
					// F2-load raíz: sin esto el estado quedaba en GS_FRONTEND y el
					// siguiente tick lo secuestraba a GS_INIT_PLAYING_GAME (partida
					// nueva pisando lo cargado).
					gGameState = GS_PLAYING_GAME;
					gWebLoadFrac = 1.0f;
				} else {
					// Escena a medias: repetir el case 2 en el próximo tick
					// (sin esto s_restartStep avanzaría y el restart acabaría
					// con la escena sin cargar).
					s_restartStep--;
				}
			}
#else
			InitRadioStationPositionList();
			if (GenericLoad() == true)
			{
				DMAudio.ResetTimers(CTimer::GetTimeInMilliseconds());
				CTrain::InitTrains();
				CPlane::InitPlanes();
			}
			else
			{
				TheCamera.SetFadeColour(0, 0, 0);
				ShutDownForRestart();
				CTimer::Stop();
				CTimer::Initialise();
				FrontEndMenuManager.m_bWantToLoad = false;
				ReInitGameObjectVariables();
				currLevel = LEVEL_GENERIC;
				CCollision::SortOutCollisionAfterLoad();
			}
#endif
		}
		WEB_LOAD_PROGRESS(3, 4);
		break;
	}
	case 3:
	{
		CTimer::Update();
		DMAudio.ChangeMusicMode(MUSICMODE_GAME);
#ifdef USE_TEXTURE_POOL
		_TexturePoolsUnknown(true);
#endif
		WEB_LOAD_PROGRESS(4, 4);
		break;
	}
	default:
		break;
	}
	s_restartStep++;
	return s_restartStep >= 4;
}
#endif

// D13/D18 (sección 1, 21/09): marcas de tramo de CGame::Process. Sirvieron para
// acotar el cuelgue de la pantalla negra y quedan como puntos de anclaje para
// el depurador, sin coste: ya no imprimen nada (cada printf por fotograma
// frenaba el motor y llenaba la consola).
#define ODMARK(n) ((void)0)

void CGame::Process(void) 
{
	ODMARK(0);
	CPad::UpdatePads();
#ifdef USE_CUSTOM_ALLOCATOR
	ProcessTidyUpMemory();
#endif
#ifdef DEBUGMENU
	DebugMenuProcess();
#endif
	CCutsceneMgr::Update();

	if (!CCutsceneMgr::IsCutsceneProcessing() && !CTimer::GetIsCodePaused())
		FrontEndMenuManager.Process();

	CTheZones::Update();
#ifdef SECUROM
	if (CTimer::GetTimeInMilliseconds() >= (35 * 60 * 1000) && gameProcessPirateCheck == 0){
		// if game not pirated
		// gameProcessPirateCheck = 1;
		// else
		gameProcessPirateCheck = 2;
	}
#endif
	uint32 startTime = CTimer::GetCurrentTimeInCycles() / CTimer::GetCyclesPerMillisecond();
	CStreaming::Update();
	ODMARK(1);
	uint32 processTime = CTimer::GetCurrentTimeInCycles() / CTimer::GetCyclesPerMillisecond() - startTime;
	CWindModifiers::Number = 0;
	if (!CTimer::GetIsPaused())
	{
#ifndef MASTER
		if (VarUpdatePlayerCoords) {
			FindPlayerPed()->Teleport(PlayerCoords);
			VarUpdatePlayerCoords = false;
		}
#endif
		CSprite2d::SetRecipNearClip();
		CSprite2d::InitPerFrame();
		ODMARK(11);
		CFont::InitPerFrame();
		CRecordDataForGame::SaveOrRetrieveDataForThisFrame();
		ODMARK(12);
		CRecordDataForChase::SaveOrRetrieveDataForThisFrame();
		CPad::DoCheats();
		CClock::Update();
		CWeather::Update();
		ODMARK(13);

		PUSH_MEMID(MEMID_SCRIPT);
		CTheScripts::Process();
		POP_MEMID();
		ODMARK(2);

		CCollision::Update();
		CScriptPaths::Update();
		CTrain::UpdateTrains();
		CPlane::UpdatePlanes();
		CHeli::UpdateHelis();
		CDarkel::Update();
		CSkidmarks::Update();
		CAntennas::Update();
		CGlass::Update();
#ifdef GTA_SCENE_EDIT
		CSceneEdit::Update();
#endif
		CSetPieces::Update();
		CEventList::Update();
		CParticle::Update();
		gFireManager.Update();

		// Otherwise even on 30 fps most probably you won't see any peds around Ocean View Hospital
#if defined FIX_BUGS && !defined SQUEEZE_PERFORMANCE
		if (processTime > 2) {
#else
		if (processTime >= 2) {
#endif
			CPopulation::Update(false);
		} else {
			uint32 startTime = CTimer::GetCurrentTimeInCycles() / CTimer::GetCyclesPerMillisecond();
			CPopulation::Update(true);
			processTime = CTimer::GetCurrentTimeInCycles() / CTimer::GetCyclesPerMillisecond() - startTime;
		}
		CWeapon::UpdateWeapons();
		if (!CCutsceneMgr::IsRunning())
			CTheCarGenerators::Process();
		if (!CReplay::IsPlayingBack())
			CCranes::UpdateCranes();
		CClouds::Update();
		CMovingThings::Update();
		CWaterCannons::Update();
		CUserDisplay::Process();
		CReplay::Update();

		PUSH_MEMID(MEMID_WORLD);
		CWorld::Process();
		POP_MEMID();
		ODMARK(4);

		gAccidentManager.Update();
		CPacManPickups::Update();
		CPickups::Update();
		CGarages::Update();
		CRubbish::Update();
		CSpecialFX::Update();
		CRopes::Update();
		CTimeCycle::Update();
		if (CReplay::ShouldStandardCameraBeProcessed())
			TheCamera.Process();
		CCullZones::Update();
		if (!CReplay::IsPlayingBack())
			CGameLogic::Update();
		ODMARK(5);
		CBridge::Update();
		CCoronas::DoSunAndMoon();
		CCoronas::Update();
		CShadows::UpdateStaticShadows();
		CShadows::UpdatePermanentShadows();
		gPhoneInfo.Update();
		if (!CReplay::IsPlayingBack())
		{
			PUSH_MEMID(MEMID_CARS);
			if (processTime < 2)
				CCarCtrl::GenerateRandomCars();
			CRoadBlocks::GenerateRoadBlocks();
			CCarCtrl::RemoveDistantCars();
			CCarCtrl::RemoveCarsIfThePoolGetsFull();
			POP_MEMID();
		}
	}
#ifdef GTA_PS2
	CMemCheck::DoTest();
#endif
}

#ifdef USE_CUSTOM_ALLOCATOR

// TODO(MIAMI)

int32 gNumMemMoved;

bool
MoveMem(void** ptr)
{
	if (*ptr) {
		gNumMemMoved++;
		void* newPtr = gMainHeap.MoveMemory(*ptr);
		if (*ptr != newPtr) {
			*ptr = newPtr;
			return true;
		}
	}
	return false;
}

// Some convenience structs
struct SkyDataPrefix
{
	uint32 pktSize1;
	uint32 data;	// pointer to data as read from TXD
	uint32 pktSize2;
	uint32 unused;
};

struct DMAGIFUpload
{
	uint32 tag1_qwc, tag1_addr;	// dmaref
	uint32 nop1, vif_direct1;

	uint32 giftag[4];
	uint32 gs_bitbltbuf[4];

	uint32 tag2_qwc, tag2_addr;	// dmaref
	uint32 nop2, vif_direct2;
};

// This is very scary. it depends on the exact memory layout of the DMA chains and whatnot
RwTexture*
MoveTextureMemoryCB(RwTexture* texture, void* pData)
{
#ifdef GTA_PS2
	bool* pRet = (bool*)pData;
	RwRaster* raster = RwTextureGetRaster(texture);
	_SkyRasterExt* rasterExt = RASTEREXTFROMRASTER(raster);
	if (raster->originalPixels == nil ||	// the raw data
		raster->cpPixels == raster->originalPixels ||	// old format, can't handle it
		rasterExt->dmaRefCount != 0 && rasterExt->dmaClrCount != 0)
		return texture;

	// this is the allocated pointer we will move
	SkyDataPrefix* prefix = (SkyDataPrefix*)raster->originalPixels;
	DMAGIFUpload* uploads = (DMAGIFUpload*)(prefix + 1);

	// We have 4qw for each upload,
	// i.e. for each buffer width of mip levels,
	// and the palette if there is one.
	// NB: this code does NOT support mipmaps!
	// so we assume two uploads (pixels and palette)
	//
	// each upload looks like this:
	//    (DMAcnt; NOP; VIF DIRECT(2))
	//     giftag (1, A+D)
	//      GS_BITBLTBUF
	//    (DMAref->pixel data; NOP; VIF DIRECT(5))
	// the DMArefs are what we have to adjust
	uintptr dataDiff, upload1Diff, upload2Diff, pixelDiff, paletteDiff;
	dataDiff = prefix->data - (uintptr)raster->originalPixels;
	upload1Diff = uploads[0].tag2_addr - (uintptr)raster->originalPixels;
	if (raster->palette)
		upload2Diff = uploads[1].tag2_addr - (uintptr)raster->originalPixels;
	pixelDiff = (uintptr)raster->cpPixels - (uintptr)raster->originalPixels;
	if (raster->palette)
		paletteDiff = (uintptr)raster->palette - (uintptr)raster->originalPixels;
	uint8* newptr = (uint8*)gMainHeap.MoveMemory(raster->originalPixels);
	if (newptr != raster->originalPixels) {
		// adjust everything
		prefix->data = (uintptr)newptr + dataDiff;
		uploads[0].tag2_addr = (uintptr)newptr + upload1Diff;
		if (raster->palette)
			uploads[1].tag2_addr = (uintptr)newptr + upload2Diff;
		raster->originalPixels = newptr;
		raster->cpPixels = newptr + pixelDiff;
		if (raster->palette)
			raster->palette = newptr + paletteDiff;

		if (pRet) {
			*pRet = true;
			return nil;
		}
	}
#else
	// nothing to do here really, everything should be in videomemory
#endif
	return texture;
}

bool
MoveAtomicMemory(RpAtomic* atomic, bool onlyOne)
{
	RpGeometry* geo = RpAtomicGetGeometry(atomic);

#if THIS_IS_COMPATIBLE_WITH_GTA3_RW31
	if (MoveMem((void**)&geo->triangles) && onlyOne)
		return true;
	if (MoveMem((void**)&geo->matList.materials) && onlyOne)
		return true;
	if (MoveMem((void**)&geo->preLitLum) && onlyOne)
		return true;
	if (MoveMem((void**)&geo->texCoords[0]) && onlyOne)
		return true;
	if (MoveMem((void**)&geo->texCoords[1]) && onlyOne)
		return true;

	// verts and normals of morph target are allocated together
	int vertDiff;
	if (geo->morphTarget->normals)
		vertDiff = geo->morphTarget->normals - geo->morphTarget->verts;
	if (MoveMem((void**)&geo->morphTarget->verts)) {
		if (geo->morphTarget->normals)
			geo->morphTarget->normals = geo->morphTarget->verts + vertDiff;
		if (onlyOne)
			return true;
	}

	RpMeshHeader* oldmesh = geo->mesh;
	if (MoveMem((void**)&geo->mesh)) {
		// index pointers are allocated together with meshes,
		// have to relocate those too
		RpMesh* mesh = (RpMesh*)(geo->mesh + 1);
		uintptr reloc = (uintptr)geo->mesh - (uintptr)oldmesh;
		for (int i = 0; i < geo->mesh->numMeshes; i++)
			mesh[i].indices = (RxVertexIndex*)((uintptr)mesh[i].indices + reloc);
		if (onlyOne)
			return true;
	}
#else
	// we could do something in librw here
#endif
	return false;
}

bool
MoveColModelMemory(CColModel& colModel, bool onlyOne)
{
#if GTA_VERSION >= GTA3_PS2_160
	// hm...should probably only do this if ownsCollisionVolumes
	// but it doesn't exist on PS2...
	if (!colModel.ownsCollisionVolumes)
		return false;
#endif

	if (MoveMem((void**)&colModel.spheres) && onlyOne)
		return true;
	if (MoveMem((void**)&colModel.lines) && onlyOne)
		return true;
	if (MoveMem((void**)&colModel.boxes) && onlyOne)
		return true;
	if (MoveMem((void**)&colModel.vertices) && onlyOne)
		return true;
	if (MoveMem((void**)&colModel.triangles) && onlyOne)
		return true;
	if (MoveMem((void**)&colModel.trianglePlanes) && onlyOne)
		return true;
	return false;
}

RpAtomic*
MoveAtomicMemoryCB(RpAtomic* atomic, void* pData)
{
	bool* pRet = (bool*)pData;
	if (pRet == nil)
		MoveAtomicMemory(atomic, false);
	else if (MoveAtomicMemory(atomic, true)) {
		*pRet = true;
		return nil;
	}
	return atomic;
}

bool
TidyUpModelInfo(CBaseModelInfo* modelInfo, bool onlyone)
{
	if (modelInfo->GetColModel() && modelInfo->DoesOwnColModel())
		if (MoveColModelMemory(*modelInfo->GetColModel(), onlyone))
			return true;

	RwObject* rwobj = modelInfo->GetRwObject();
	if (RwObjectGetType(rwobj) == rpATOMIC)
		if (MoveAtomicMemory((RpAtomic*)rwobj, onlyone))
			return true;
	if (RwObjectGetType(rwobj) == rpCLUMP) {
		bool ret = false;
		if (onlyone)
			RpClumpForAllAtomics((RpClump*)rwobj, MoveAtomicMemoryCB, &ret);
		else
			RpClumpForAllAtomics((RpClump*)rwobj, MoveAtomicMemoryCB, nil);
		if (ret)
			return true;
	}

	if (modelInfo->GetModelType() == MITYPE_PED && ((CPedModelInfo*)modelInfo)->m_hitColModel)
		if (MoveColModelMemory(*((CPedModelInfo*)modelInfo)->m_hitColModel, onlyone))
			return true;

	return false;
}
#endif


void CGame::DrasticTidyUpMemory(bool flushDraw)
{
#ifdef USE_CUSTOM_ALLOCATOR
	bool removedCol = false;

	TidyUpMemory(true, flushDraw);

	if (gMainHeap.GetLargestFreeBlock() < 200000 && !playingIntro) {
		CStreaming::RemoveIslandsNotUsed(LEVEL_INDUSTRIAL);
		CStreaming::RemoveIslandsNotUsed(LEVEL_COMMERCIAL);
		CStreaming::RemoveIslandsNotUsed(LEVEL_SUBURBAN);
		TidyUpMemory(true, flushDraw);
	}

	if (gMainHeap.GetLargestFreeBlock() < 200000 && !playingIntro) {
		CModelInfo::RemoveColModelsFromOtherLevels(LEVEL_GENERIC);
		TidyUpMemory(true, flushDraw);
		removedCol = true;
	}

	if (gMainHeap.GetLargestFreeBlock() < 200000 && !playingIntro) {
		CStreaming::RemoveBigBuildings(LEVEL_INDUSTRIAL);
		CStreaming::RemoveBigBuildings(LEVEL_COMMERCIAL);
		CStreaming::RemoveBigBuildings(LEVEL_SUBURBAN);
		TidyUpMemory(true, flushDraw);
	}

	if (removedCol) {
		// different on PS2
		CFileLoader::LoadCollisionFromDatFile(CCollision::ms_collisionInMemory);
	}

	if (!playingIntro)
		CStreaming::RequestBigBuildings(currLevel);

	CStreaming::LoadAllRequestedModels(true);
#endif
	ODMARK(6);
}

void CGame::TidyUpMemory(bool moveTextures, bool flushDraw)
{
#ifdef USE_CUSTOM_ALLOCATOR
	printf("Largest free block before tidy %d\n", gMainHeap.GetLargestFreeBlock());

	if (moveTextures) {
		if (flushDraw) {
#ifdef GTA_PS2
			for (int i = 0; i < sweMaxFlips + 1; i++) {
#else
			for (int i = 0; i < 5; i++) {	// probably more than needed
#endif
				RwCameraBeginUpdate(Scene.camera);
				RwCameraEndUpdate(Scene.camera);
				RwCameraShowRaster(Scene.camera, nil, 0);
			}
			}
		int fontSlot = CTxdStore::FindTxdSlot("fonts");

		for (int i = 0; i < TXDSTORESIZE; i++) {
			if (i == fontSlot ||
				CTxdStore::GetSlot(i) == nil)
				continue;
			RwTexDictionary* txd = CTxdStore::GetSlot(i)->texDict;
			if (txd)
				RwTexDictionaryForAllTextures(txd, MoveTextureMemoryCB, nil);
		}
		}

	// animations
	for (int i = 0; i < NUMANIMATIONS; i++) {
		CAnimBlendHierarchy* anim = CAnimManager::GetAnimation(i);
		if (anim == nil)
			continue;	// cannot happen
		anim->MoveMemory();
	}

	// model info
	for (int i = 0; i < MODELINFOSIZE; i++) {
		CBaseModelInfo* mi = CModelInfo::GetModelInfo(i);
		if (mi == nil)
			continue;
		TidyUpModelInfo(mi, false);
	}

	printf("Largest free block after tidy %d\n", gMainHeap.GetLargestFreeBlock());
#endif
	}

void CGame::ProcessTidyUpMemory(void)
{
#ifdef USE_CUSTOM_ALLOCATOR
	static int32 modelIndex = 0;
	static int32 animIndex = 0;
	static int32 txdIndex = 0;
	bool txdReturn = false;
	RwTexDictionary* txd = nil;
	gNumMemMoved = 0;

	// model infos
	for (int numCleanedUp = 0; numCleanedUp < 10; numCleanedUp++) {
		CBaseModelInfo* mi;
		do {
			mi = CModelInfo::GetModelInfo(modelIndex);
			modelIndex++;
			if (modelIndex >= MODELINFOSIZE)
				modelIndex = 0;
		} while (mi == nil);

		if (TidyUpModelInfo(mi, true))
			return;
	}

	// tex dicts
	for (int numCleanedUp = 0; numCleanedUp < 3; numCleanedUp++) {
		if (gNumMemMoved > 80)
			break;

		do {
#ifdef FIX_BUGS
			txd = nil;
#endif
			if (CTxdStore::GetSlot(txdIndex))
				txd = CTxdStore::GetSlot(txdIndex)->texDict;
			txdIndex++;
			if (txdIndex >= TXDSTORESIZE)
				txdIndex = 0;
		} while (txd == nil);

		RwTexDictionaryForAllTextures(txd, MoveTextureMemoryCB, &txdReturn);
		if (txdReturn)
			return;
		}

	// animations
	CAnimBlendHierarchy* anim;
	do {
		anim = CAnimManager::GetAnimation(animIndex);
		animIndex++;
		if (animIndex >= NUMANIMATIONS)
			animIndex = 0;
	} while (anim == nil);	// always != nil
	anim->MoveMemory(true);
#endif
}

void
CGame::InitAfterFocusLoss()
{
	FrontEndMenuManager.m_nPrefsAudio3DProviderIndex = FrontEndMenuManager.m_lastWorking3DAudioProvider;
	DMAudio.SetCurrent3DProvider(FrontEndMenuManager.m_lastWorking3DAudioProvider);

	if (!FrontEndMenuManager.m_bGameNotLoaded && !FrontEndMenuManager.m_bMenuActive)
		FrontEndMenuManager.m_bStartUpFrontEndRequested = true;
}

bool
CGame::CanSeeWaterFromCurrArea(void)
{
	return currArea == AREA_MAIN_MAP || currArea == AREA_MANSION
		|| currArea == AREA_HOTEL;
}

bool
CGame::CanSeeOutSideFromCurrArea(void)
{
	return currArea == AREA_MAIN_MAP || currArea == AREA_MALL ||
		currArea == AREA_MANSION || currArea == AREA_HOTEL;
}
