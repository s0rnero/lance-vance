#pragma once

#ifndef FINAL
// defined in RwHelpder.cpp
void PushRendergroup(const char *name, bool timed = true);
void PopRendergroup(void);
#define PUSH_RENDERGROUP(str) PushRendergroup(str)
#define PUSH_RENDERGROUP_UNTIMED(str) PushRendergroup(str, false)
#define POP_RENDERGROUP() PopRendergroup()
#else
#define PUSH_RENDERGROUP(str)
#define PUSH_RENDERGROUP_UNTIMED(str)
#define POP_RENDERGROUP()
#endif

void PedClumpInit(int count);
void PedClumpShutdown(void);
void PedClumpAlloc(int slot, void *ped);
void PedClumpFree(void *ped);
void PedClumpModel(void *ped, int model);
void PedClumpBind(void *ped, void *clump);
void PedClumpUnbind(void *ped);
void PedClumpPreDelete(void *ped, void *clump);
void PedClumpSweep(int phase);

struct GlobalScene
{
	RpWorld *world;
	RwCamera *camera;
};
extern GlobalScene Scene;

extern uint8 work_buff[55000];
extern char gString[256];
extern char gString2[512];
extern wchar gUString[256];
extern wchar gUString2[256];
extern bool gbPrintShite;
extern bool gbModelViewer;
#ifdef TIMEBARS
extern bool gbShowTimebars;
#else
#define gbShowTimebars false
#endif

#ifndef FINAL
extern bool gbPrintMemoryUsage;
#endif

class CSprite2d;

bool DoRWStuffStartOfFrame(int16 TopRed, int16 TopGreen, int16 TopBlue, int16 BottomRed, int16 BottomGreen, int16 BottomBlue, int16 Alpha);
bool DoRWStuffStartOfFrame_Horizon(int16 TopRed, int16 TopGreen, int16 TopBlue, int16 BottomRed, int16 BottomGreen, int16 BottomBlue, int16 Alpha);
void DoRWStuffEndOfFrame(void);
void PreAllocateRwObjects(void);
void InitialiseGame(void);
#ifdef __EMSCRIPTEN__
bool InitialiseGameStep(void); // progressive loading, true when ready
#endif
void LoadingScreen(const char *str1, const char *str2, const char *splashscreen);
#ifdef __EMSCRIPTEN__
void WebDrawLoadScreen(float frac);
void WebBeginLoadScreen(void);
void WebEndLoadScreen(void);
const char *WebGetLoadSplashTxdPath(void);
extern float gWebLoadFrac;
extern int gWebBootInitPending;
extern uint32 gWebStrmMs;
void WebDrawFps(void);
#endif
void LoadingIslandScreen(const char *levelName);
CSprite2d *LoadSplash(const char *name);
void DestroySplashScreen(void);
Const char *GetLevelSplashScreen(int level);
Const char *GetRandomSplashScreen(void);
void LittleTest(void);
void ValidateVersion();
void ResetLoadingScreenBar(void);
#ifndef MASTER
void TheModelViewer(void);
#endif

#ifdef LOAD_INI_SETTINGS
bool LoadINISettings();
void SaveINISettings();
void LoadINIControllerSettings();
void SaveINIControllerSettings();
#endif

#ifdef NEW_RENDERER
extern bool gbNewRenderer;
bool FredIsInFirstPersonCam(void);
#endif

#ifdef DRAW_GAME_VERSION_TEXT
extern bool gbDrawVersionText;
#endif

#ifdef NO_MOVIES
extern bool gbNoMovies;
#endif
#ifdef VICEEXT_LOAD_SAVE_ON_STARTUP
extern int32 gbGameSaveOnStartup;
#endif
#ifdef VICEEXT_ENV_MAP_COEF
extern int32 g_nEnvMapStrength;
#endif
