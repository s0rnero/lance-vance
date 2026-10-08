#pragma once

#ifndef FINAL
// defined in RwHelpder.cpp
void PushRendergroup(const char *name);
void PopRendergroup(void);
#define PUSH_RENDERGROUP(str) PushRendergroup(str)
#define POP_RENDERGROUP() PopRendergroup()
#else
#define PUSH_RENDERGROUP(str)
#define POP_RENDERGROUP()
#endif

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
// Pantalla de carga in-game (splash vanilla + barra con fracción explícita,
// sin texto). Se dibuja un tramo por tick durante la carga troceada.
void WebDrawLoadScreen(float frac);
// Secuencia de carga (arranque / partida guardada): fija la portada a splash1,
// reinicia la barra monótona una sola vez y silencia cualquier otra pantalla de
// carga del motor mientras dure. Begin es idempotente dentro de la secuencia.
void WebBeginLoadScreen(void);
void WebEndLoadScreen(void);
extern float gWebLoadFrac;
// F1b-A2: el SÍ de carga difiere el init; AfterInner lo corre monolítico
// (con splash ya pintado) cuando este flag está a 1.
extern int gWebBootInitPending;
// PERF: ms de streaming (Update->LoadRequestedModels) desde el último FPSLOG.
extern uint32 gWebStrmMs;
// Contador FPS en esquina (?fps en la URL). Media de 500 ms.
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
