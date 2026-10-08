#pragma once

// disables (most) stuff that wasn't in original gta-vc.exe
#ifdef __MWERKS__
#define VANILLA_DEFINES
#endif

enum Config {
	NUMPLAYERS = 1,

	NUMCDIMAGES = 6, // gta3.img duplicates (not used on PC)
	// Vice Extended añade 8 IMG propias (anims/generic/objects/peds/player/
	// radar/vehicles/weapons) además de gta3.img. Con 8 el CdStreamAddImage
	// petaba en el ASSERT; 12 deja margen. Coste: gOdLoose[] + un fd por imagen.
	MAX_CDIMAGES = 12, // additional cdimages
	MAX_CDCHANNELS = 5,

	// Vice Extended usa IDs por encima de 6500 (armas 6661-6669, coches
	// 6500-6599). Sin subirlo, esos IDs caen fuera de ms_modelInfoPtrs y de
	// ms_aInfoForModel: modelo/colisión nulos o tabla corrupta.
	MODELINFOSIZE = 6700,	// 6500 vanilla, 4900 on PS2
	// Contador de slots de TXD. El gta3.dir del port tiene 1389 entradas .txd
	// tras importar weapons.img (pack 3) y newVehicles.img (pack 4): con 1385
	// (que era el número justo antes de estos packs) `CTxdStore::AddTxdSlot`
	// revienta con ASSERT(def) al cargar el directorio. Margen para los bloques
	// que quedan (el resto de IMG del mod traen más TXD).
	TXDSTORESIZE = 1500,
	// 31 = las 30 .col de los mapas + GENERIC.COL (justo al límite). Vice
	// Extended añade bryx.col, plusroad.col (objects.img) y newgen.col
	// (COLFILE): 34 slots -> sin subirlo, CColStore::AddColSlot peta
	// (ASSERT(def) en ColStore.cpp:48) al cargar el directorio de la IMG.
	COLSTORESIZE = 48,
	EXTRADIRSIZE = 256,
	CUTSCENEDIRSIZE = 512,

	SIMPLEMODELSIZE = 3885,
	TIMEMODELSIZE = 385,
	CLUMPMODELSIZE = 5,
	// 37 armas de serie (258-294) + las 10 del mod (6660-6669, sección `weap`
	// de su default.ide). Sin este hueco, CModelInfo::AddWeaponModel no puede
	// crear sus CWeaponModelInfo y SetWeaponInfo escribe sobre un puntero nulo.
	WEAPONMODELSIZE = 47,
	PEDMODELSIZE = 130,
	VEHICLEMODELSIZE = 130, // 110 vanilla; el mod añade coches
	TWODFXSIZE = 1210,

	MAXVEHICLESLOADED = 50, // 70 on mobile

	NUMOBJECTINFO = 210,

	// Pool sizes
	NUMPTRNODES = 50000,
	NUMENTRYINFOS = 3200,
	NUMPEDS = 140,
	NUMVEHICLES = 130, // 110 vanilla; limits.ini del mod pide 130
	NUMBUILDINGS = 7000,
	NUMTREADABLES = 1,
	NUMOBJECTS = 460,
	NUMDUMMIES = 2340,
	NUMAUDIOSCRIPTOBJECTS = 192,
	NUMCOLMODELS = 4400,
	NUMCUTSCENEOBJECTS = 50,	// not a pool in VC

	// 29 bloques de serie (28 .ifp de gta3.img + ped.ifp) + 3 del mod
	// (deagle/steyr/rocket) + margen. cuts.img no cuenta: CutsceneMgr carga
	// un IFP a la vez.
	NUMANIMBLOCKS = 40,
	// 410 de serie + 38 nuevas del ped.ifp del mod + 24 de sus bloques de
	// moto + 2 de buddy + 12 de deagle/steyr/rocket = 486, con margen para
	// un bloque de cuts a la vez.
	NUMANIMATIONS = 512,

	NUMTEMPOBJECTS = 40,

	// Path data
	NUM_PATHNODES = 9650,
	NUM_CARPATHLINKS = 3500,
	NUM_MAPOBJECTS = 1250,
	NUM_PATHCONNECTIONS = 20400,

	// Link list lengths
	NUMALPHALIST = 20,
	NUMBOATALPHALIST = 20,
	NUMALPHAENTITYLIST = 200,
	NUMALPHAUNTERWATERENTITYLIST = 30,
	NUMCOLCACHELINKS = 50,
	NUMREFERENCES = 800,

	// Zones
	NUMAUDIOZONES = 14,
	NUMINFOZONES = 169,
	NUMMAPZONES = 39,
	NUMNAVIGZONES = 20,

	// Cull zones
	NUMATTRIBZONES = 704,

	NUMOCCLUSIONVOLUMES = 350,
	NUMACTIVEOCCLUDERS = 48,

	PATHNODESIZE = 4500,

	NUMWEATHERS = 7,
	NUMHOURS = 24,

	NUMEXTRADIRECTIONALS = 4,
	NUMANTENNAS = 8,
	NUMCORONAS = 56,
	NUMPOINTLIGHTS = 32,
	NUM3DMARKERS = 32,
	NUMBRIGHTLIGHTS = 32,
	NUMSHINYTEXTS = 32,
	NUMMONEYMESSAGES = 16,
	NUMPICKUPMESSAGES = 16,
	NUMBULLETTRACES = 16,
	NUMMBLURSTREAKS = 4,
	NUMSKIDMARKS = 32,

	NUMONSCREENCLOCKS = 1,
	NUMONSCREENCOUNTERS = 3,
	NUMRADARBLIPS = 75,
	NUMGENERALPICKUPS = 320,
	NUMSCRIPTEDPICKUPS = 16,
	NUMPICKUPS = NUMGENERALPICKUPS + NUMSCRIPTEDPICKUPS,
	NUMCOLLECTEDPICKUPS = 20,
	NUMPACMANPICKUPS = 256,
	NUMEVENTS = 64,

	NUM_CARGENS = 185,

	NUM_PATH_NODES_IN_AUTOPILOT = 8,

	NUM_ACCIDENTS = 20,
	NUM_FIRES = 40,
	NUM_GARAGES = 32,
	NUM_PROJECTILES = 32,

	NUM_GLASSPANES = 45,
	NUM_GLASSENTITIES = 32,
	NUM_WATERCANNONS = 3,

	NUMPEDROUTES = 200,
	NUMPHONES = 50,
	NUMPEDGROUPS = 67,
	NUMMODELSPERPEDGROUP = 16,
	MAXZONEPEDSLOADED = 8,
	NUMSHOTINFOS = 100,

	NUMROADBLOCKS = 300,
	NUM_SCRIPT_ROADBLOCKS = 16,

	NUMVISIBLEENTITIES = 2000,
	NUMINVISIBLEENTITIES = 150,

	NUM_AUDIOENTITY_EVENTS = 4,
	NUM_PED_COMMENTS_SLOTS = 20,

	NUM_SOUND_QUEUES = 2,
	NUM_AUDIOENTITIES = 250,

	NUM_SCRIPT_MAX_ENTITIES = 40,

	NUM_GARAGE_STORED_CARS = 4,

	NUM_CRANES = 8,
	NUM_ESCALATORS = 22,
	NUM_WATER_CREATURES = 8,

	NUM_EXPLOSIONS = 48,

	NUM_SETPIECES = 96,
	NUM_SHORTCUT_START_POINTS = 16
};

// We don't expect to compile for PS2 or Xbox
// but it might be interesting for documentation purposes
#define GTA_PC
//#define GTA_PS2
//#define GTA_XBOX

// Version defines
#define GTAVC_PS2	400
#define GTAVC_PC_10	410
#define GTAVC_PC_11	411
#define GTAVC_PC_JAP	412
// TODO? maybe something for xbox or android?

#define GTA_VERSION	GTAVC_PC_11

// Enable configuration for handheld console ports
#if defined(__SWITCH__) || defined(PSP2)
	#define GTA_HANDHELD
#endif

// TODO(MIAMI): someone ought to find and check out uses of these defines:
//#define GTA3_STEAM_PATCH
//#define GTAVC_JP_PATCH

#if defined GTA_PS2
#	define GTA_PS2_STUFF
#	define RANDOMSPLASH
//#	define USE_CUSTOM_ALLOCATOR
#	define VU_COLLISION
#	define PS2_MENU
#elif defined GTA_PC
#	define EXTERNAL_3D_SOUND
#	define AUDIO_REVERB
#	ifndef GTA_HANDHELD
#		define PC_PLAYER_CONTROLS	// mouse player/cam mode
#	endif
#	define GTA_REPLAY
#	define GTA_SCENE_EDIT
#	define PC_MENU
#	define PC_WATER
#elif defined GTA_XBOX
#elif defined GTA_MOBILE
#	define MISSION_REPLAY
#	define SIMPLER_MISSIONS
#endif

// This is enabled for all released games.
// any debug stuff that isn't left in any game is not in FINAL
//#define FINAL

// This is enabled for all released games except mobile
// any debug stuff that is only left in mobile, is not in MASTER
//#define MASTER

// once and for all:
// pc: FINAL & MASTER
// mobile: FINAL

// MASTER builds must be FINAL
#ifdef MASTER
#define FINAL
#endif

// these are placed here to work with VANILLA_DEFINES for compatibility
#define NO_CDCHECK // skip audio CD check
#define DEFAULT_NATIVE_RESOLUTION // Set default video mode to your native resolution (fixes Windows 10 launch)

#ifdef VANILLA_DEFINES
#if !defined(_WIN32) || defined(__LP64__) || defined(_WIN64)
#error Vanilla can only be built for win-x86
#endif

#define FINAL
#define MASTER
//#define USE_MY_DOCUMENTS
#define THIS_IS_STUPID
#define DONT_FIX_REPLAY_BUGS
#define USE_TXD_CDIMAGE // generate and load textures from txd.img
//#define USE_TEXTURE_POOL // not possible because R* used custom RW33
#define AUDIO_REFLECTIONS
#else
// This enables things from the PS2 version on PC
#define GTA_PS2_STUFF

// quality of life fixes that should also be in FINAL
#define NASTY_GAME	// nasty game for all languages

// those infamous texts
#define DRAW_GAME_VERSION_TEXT
#ifdef DRAW_GAME_VERSION_TEXT
	// unlike R* development builds, ours has runtime switch on debug menu & .ini, and disabled as default.
	// If you disable this then game will fetch version from peds.col, as R* did while in development.
	//#define USE_OUR_VERSIONING // enabled from buildfiles by default
#endif

// Memory allocation and compression
// #define USE_CUSTOM_ALLOCATOR		// use CMemoryHeap for allocation. use with care, not finished yet
//#define COMPRESSED_COL_VECTORS	// use compressed vectors for collision vertices
//#define ANIM_COMPRESSION	// only keep most recently used anims uncompressed

#if defined GTA_PC && defined GTA_PS2_STUFF
#	define USE_PS2_RAND
#	define RANDOMSPLASH	// use random splash as on PS2
#	define PS2_MATFX
#endif

#ifdef VU_COLLISION
#define COMPRESSED_COL_VECTORS	// currently need compressed vectors in this code
#endif

#ifdef MASTER
	// only in master builds
	#undef DRAW_GAME_VERSION_TEXT
#else
	// not in master builds
	#define VALIDATE_SAVE_SIZE

	#define DEBUGMENU
#endif

#ifdef FINAL
	// in all games
#	define USE_MY_DOCUMENTS	// use my documents directory for user files
#else
	// not in any game
#	define CHATTYSPLASH	// print what the game is loading
#	define TIMEBARS		// print debug timers
#endif

#define FIX_BUGS		// fixes bugs that we've came across during reversing. You can undefine this only on release builds.
#define MORE_LANGUAGES		// Add more translations to the game
#define COMPATIBLE_SAVES // this allows changing structs while keeping saves compatible, and keeps saves compatible between platforms
#define FIX_INCOMPATIBLE_SAVES // try to fix incompatible saves, requires COMPATIBLE_SAVES
#define LOAD_INI_SETTINGS // as the name suggests. fundamental for CUSTOM_FRONTEND_OPTIONS

#define NO_MOVIES	// add option to disable intro videos

#define EXTENDED_OFFSCREEN_DESPAWN_RANGE // Use onscreen despawn range for offscreen peds and vehicles to avoid them despawning in the distance when you look
                                         // away

#if defined(__LP64__) || defined(_WIN64)
#define FIX_BUGS_64 // Must have fixes to be able to run 64 bit build
#endif

#define ASCII_STRCMP // use faster ascii str comparisons

#if !defined _WIN32 || defined __MINGW32__
#undef ASCII_STRCMP
#endif

// Just debug menu entries
#ifdef DEBUGMENU
#define RELOADABLES			// some debug menu options to reload TXD files
#define MISSION_SWITCHER // from debug menu
#endif

// Rendering/display
#define ASPECT_RATIO_SCALE	// Not just makes everything scale with aspect ratio, also adds support for all aspect ratios
#define PROPER_SCALING		// use original DEFAULT_SCREEN_WIDTH/DEFAULT_SCREEN_HEIGHT from PS2 instead of PC(R* changed HEIGHT here to make radar look better, but broke other hud elements aspect ratio).
#define DEFAULT_NATIVE_RESOLUTION	// Set default video mode to your native resolution (fixes Windows 10 launch)
#define USE_TXD_CDIMAGE		// generate and load textures from txd.img
#define PS2_ALPHA_TEST		// emulate ps2 alpha test 
#define IMPROVED_VIDEOMODE	// save and load videomode parameters instead of a magic number
#define DISABLE_LOADING_SCREEN // disable the loading screen which vastly improves the loading time
#define DISABLE_VSYNC_ON_TEXTURE_CONVERSION // make texture conversion work faster by disabling vsync
#define ANISOTROPIC_FILTERING	// set all textures to max anisotropic filtering
//#define USE_TEXTURE_POOL
#ifdef LIBRW
#define EXTENDED_COLOURFILTER		// more options for colour filter (replaces mblur)
#define EXTENDED_PIPELINES		// custom render pipelines (includes Neo)
#define SCREEN_DROPLETS			// neo water droplets
#define NEW_RENDERER		// leeds-like world rendering, needs librw
#endif

#define FIX_SPRITES	// fix sprites aspect ratio(moon, coronas, particle etc)

#ifndef EXTENDED_COLOURFILTER
#undef SCREEN_DROPLETS		// we need the backbuffer for this effect
#endif

// Water & Particle
#undef PC_WATER
#define WATER_CHEATS

//#define USE_CUTSCENE_SHADOW_FOR_PED // requires COMPATIBLE_SAVES
//#define DISABLE_CUTSCENE_SHADOWS

// Pad
#if !defined(RW_GL3) && defined(_WIN32)
#define XINPUT
#endif
#if defined XINPUT || (defined RW_GL3 && !defined LIBRW_SDL2 && !defined GTA_HANDHELD)
#define DETECT_JOYSTICK_MENU // Then we'll expect user to enter Controller->Detect joysticks if his joystick isn't detected at the start.
#endif
#define DETECT_PAD_INPUT_SWITCH // Adds automatic switch of pad related stuff between controller and kb/m
#define KANGAROO_CHEAT
#define RESTORE_ALLCARSHELI_CHEAT
#define BETTER_ALLCARSAREDODO_CHEAT
#define WALLCLIMB_CHEAT
#define REGISTER_START_BUTTON
#define BIND_VEHICLE_FIREWEAPON // Adds ability to rebind fire key for 'in vehicle' controls
#define BUTTON_ICONS // use textures to show controller buttons

// Vice Extended: paridad con su `features.ini` (el mod lo trae con esto puesto).
// Cada define replica un toggle SUYO que se puede reproducir con código local;
// los que están en 0 en su ini no se portan y los que piden su renderer o el
// taller de tuning quedan fuera (ver .agents/plans/vice-extended-inclusion.md).
#define VICEEXT_SWIMMING		// EnableSwimming=1: nadar sin morir (el jugador no se ahoga)
#define VICEEXT_RECOIL			// RecoilWhenFiring=1: el disparo acusa retroceso
#define VICEEXT_NO_CAR_BOUNCE	// PlayerDoesntBounceAwayFromMovingCar=1
#define VICEEXT_SPRINT_HEAVY	// v1.5: esprintar con armas pesadas
#define VICEEXT_POLICE_BIKE_LIGHTS // 6507: sirena (coronas rojo/azul) en la moto policial
#define VICEEXT_HIDE_COPS		// v1.0 "Changed wanted system": esconderse de la policía (sección 2, bloque P1)
#define VICEEXT_FIRST_PERSON	// v1.5 "First-person view": conmutador de vista en 1ª persona (sección 3, bloque C1)
#define VICEEXT_DRIVEBY_WIDE	// v1.5 "Drive-by shooting": las pistolas (una mano) valen también para disparar desde coche/moto/barco (sección 2, bloque P2)
#define VICEEXT_WEAPON_SIGHTS	// columna 27 de su weapon.dat ("weapon sight") + su weaponSights.txd (sección 1, bloque D6)
#define VICEEXT_HINT_KEYS		// v3.0 "PC key icons in game hints": los avisos pintan el icono de la tecla (su pcbtns.txd) en vez de su nombre (sección 1, bloque D7)
#define VICEEXT_AUTOSAVE		// v2.5 "Autosave after completing a mission": guarda solo al superar una misión (ranura 9, la del menú de carga) (sección 3, bloque C2)
#define VICEEXT_SAVE_ANYWHERE	// v2.5 "Saving anywhere. You must not be on a mission, not have a search level and not move": opción en el menú de pausa (sección 3, bloque C2)
#define VICEEXT_MANUAL_RELOAD	// v2.5 "Reloading a weapon on the key": recarga con tecla (R por defecto) (sección 3, bloque C3.1)
#define VICEEXT_GAS_TANK		// v2.5 "Gas tank. When shot, the car explodes": el disparo en el dummy `petrolcap` prende el vehículo (sección 3, bloque C3.3)
#define VICEEXT_SHOTGUN_AIM		// v1.5 "Changed aiming animations" + v1.0 "Changed aiming system": las escopetas también apuntan (su weapon.dat no trae el flag CANAIM) (sección 3, bloque C7)
#define VICEEXT_CROUCH			// v1.5 "Fixed ... crouching animations": agachado a pie con la tecla de la acción PED_DUCK (C) (sección 3, bloque C5)
// R14 (12ª partida, 22/09): cuánto baja el OBJETIVO de la cámara mientras se
// está agachado. Vive aquí porque lo usan los dos procesos de cámara (la bajada)
// y la traza `CROUCH2` de PlayerPed.cpp (el valor que se está aplicando).
// Historia: −0,55 (R6/H1) daba un descenso MEDIDO de sólo 0,27 m —al bajar el
// objetivo, la cámara se separa del ped y vuelve a subir—, así que el bloque H
// del verificador lo marcaba como "la cámara no baja agachado".
//
// R22 (18ª partida, 23/09): vuelve a −0,55 y el bloque H mide bien. El −0,95 se
// puso porque con −0,55 el descenso medido se quedaba en 0,27 m: se le subió la
// constante al doble hasta pasar el umbral del test, que es la forma más rápida
// de romper la cámara. Y la rompió: con −0,95 el objetivo cae POR DEBAJO de la
// cabeza agachada (la cabeza baja 0,55 m), la cámara apunta al cuerpo y, en la
// cámara de apuntar (mucho más cerca), termina DENTRO del ped. Medido en la
// partida del jugador (18ª): agachado + apuntar (botón derecho), `camdist=0,61`
// en vez de los ~2 m de la cámara de apuntar, y la captura del arnés sale con
// Tommy llenando la pantalla. Ahora la constante es la bajada REAL de la cabeza
// y el test mide lo que de verdad importa: que la cámara no acabe dentro del
// cuerpo (`camdist` ≥ 1,5 m agachado, con y sin apuntado).
#define VICEEXT_CROUCH_CAM_DROP	0.55f
#define VICEEXT_CLIMB			// bloque E1 (su features.ini trae EnableClimbing=0, pero el jugador lo pidió):
							// saltar mirando a un borde bajo trepa con los clips CLIMB_* del mod (sección 2)
#define VICEEXT_AIM_WALK		// v1.5: apuntando solo se camina (nunca correr/esprintar) y el agachado de
							// andar del mod (Crouch_forward/backward) — sección 2, 5ª partida
#define VICEEXT_ROCKET_3RD_PERSON // v3.0 "Enable third-person aiming from a rocket launcher" (su
							// features.ini: RocketLauncherThirdPersonAiming=1): el lanzacohetes apunta en
							// TERCERA persona (no entra en el modo francotirador/1ª persona que clava al
							// jugador en el sitio) y se puede andar apuntando. El misil sigue saliendo
							// hacia donde mira la cámara (`ProjectileInfo` usa su matriz).
// ClassicAXIS (gennariarmando/DK22Pac, SIN LICENSE → reimplementación con atribución):
// su sección [ClassicAxis] del INI de 2022 (`mods/Classic AXIS/ClassicAxisVC.ini`)
// y su ley de cámara propia de apuntado (`CamNew.cpp Process_AimWeapon`,
// cláusula 2 / ítem 4 del 12-handoff). Sin este define no se compila ninguna
// de las leyes de ClassicAXIS (plan `apuntado-classicaxis-100` §10.2, B0-B11).
// Lo que NO entra, y por qué (decisiones del jugador del 27/09):
//   · Stick crudo sin zona muerta  → se queda `LookAroundLeftRight()` (§5.4b)
//   · `modernCamera`              → fuera, además su default se desconoce (§5.4c)
//   · Acelerador horizontal para el ratón vertical → fuera, es un typo del mod (§5.4d)
//   · Botón de recentrar        → fuera, el jugador lo quitó en ve65 (§5.4e)
//   · `StoriesAimingCoords`       → sin conmutador natural, sin implementación (§9)
//   · Ley de coche                → se queda la nuestra, no es del mod (§5.1a)
//   · Near-clip dual              → es del GeniusZ, otro carril (§5.6)
#define VICEEXT_AIM_CLASSICAXIS
#define VICEEXT_BREAKABLE_LIGHTS // v2.5 "Car lights can break on impact" + el experimental "Vehicle lights can break when shot at": el disparo en un faro (objetos `headlight_*`/`taillight_*` del mod) lo apaga (sección 3, bloque C3.5)
#define VICEEXT_TURN_SIGNALS	// v2.5 "Turners..." — APAGADO por defecto a propósito: su features.ini trae StandardCarsUseTurnSignals=0 (sección 3, bloque C3.4)
#define VICEEXT_BIKE_EXIT_SIDE

#define VICEEXT_FIX_SILENTPATCH
#define VICEEXT_FIX_WFP
#define VICEEXT_FIX_FV
#define VICEEXT_FIX_FV_ROTOR
#define VICEEXT_POLICE_BIKE
#define VICEEXT_NO_WHEEL_PIERCE
#define VICEEXT_SKIP_PHONE_CALL

// D10 (sección 1, 21/09): metros EXTRA de modelo "bueno" antes de que entre el
// LOD (petición del jugador: "que el LOD sea de más metros, +20"). Se suma a la
// distancia de dibujado de cada modelo, así que son 20 m reales en todas.
#define VICEEXT_LOD_EXTRA 20.0f
// v2.5 "Remove zeros in the money in the HUD" (su features.ini:
// `RemoveMoneyZerosInTheHud`). El HUD de reVC/VC pinta el dinero con ancho fijo
// de 8 digitos y ceros a la izquierda (`$00001234`); con esto sale `$1234`.
#define VICEEXT_MONEY_NO_ZEROS

#define VICEEXT_PEDARBITER

// Hud, frontend and radar
#define PC_MENU
#define FIX_RADAR			// use radar size from early version before R* broke it
#define RADIO_OFF_TEXT		// Won't work without FIX_BUGS

#ifndef PC_MENU
#	define PS2_MENU
//#	define PS2_MENU_USEALLPAGEICONS
#else
#	define MAP_ENHANCEMENTS			// Adding waypoint and better mouse support
#	if defined(XINPUT) || defined(GTA_HANDHELD)
#		define GAMEPAD_MENU		// Add gamepad menu
#	endif
#	define TRIANGLE_BACK_BUTTON
//#	define CIRCLE_BACK_BUTTON
#define LEGACY_MENU_OPTIONS			// i.e. frame sync(vsync)
// PORTADO (SilentPatch :4086, ver CMenuManager::DrawQuitGameScreen): el outro
// dura ≈2,5 s legibles (75 ticks × 33 ms) en vez de 750 ms.
//#define MUCH_SHORTER_OUTRO_SCREEN
// #define XBOX_MESSAGE_SCREEN			// Blue background, no "saved successfully press OK" screen etc.
#	define CUSTOM_FRONTEND_OPTIONS

#	ifdef CUSTOM_FRONTEND_OPTIONS
#		define GRAPHICS_MENU_OPTIONS // otherwise Display settings will be scrollable
#		define NO_ISLAND_LOADING  // disable loadscreen between islands via loading all island data at once, consumes more memory and CPU
#		define CUTSCENE_BORDERS_SWITCH
#		define MULTISAMPLING		// adds MSAA option
#		define INVERT_LOOK_FOR_PAD // enable the hidden option
#		define PED_CAR_DENSITY_SLIDERS
#	endif
#endif

// Script
#define USE_DEBUG_SCRIPT_LOADER	// Loads main.scm by default. Hold R for main_freeroam.scm and D for main_d.scm
#define USE_MEASUREMENTS_IN_METERS // makes game use meters instead of feet in script
#define USE_PRECISE_MEASUREMENT_CONVERTION // makes game convert feet to meeters more precisely
#define SUPPORT_JAPANESE_SCRIPT
//#define SUPPORT_XBOX_SCRIPT
#define SUPPORT_MOBILE_SCRIPT
#define SUPPORT_GINPUT_SCRIPT
#if (defined SUPPORT_XBOX_SCRIPT && defined SUPPORT_MOBILE_SCRIPT)
static_assert(false, "SUPPORT_XBOX_SCRIPT and SUPPORT_MOBILE_SCRIPT are mutually exclusive");
#endif
#ifdef PC_MENU
#define MISSION_REPLAY // mobile feature
//#define SIMPLER_MISSIONS // apply simplifications from mobile
#define USE_MISSION_REPLAY_OVERRIDE_FOR_NON_MOBILE_SCRIPT
#endif
#define USE_ADVANCED_SCRIPT_DEBUG_OUTPUT
#define SCRIPT_LOG_FILE_LEVEL 0 // 0 == no log, 1 == overwrite every frame, 2 == full log

#if SCRIPT_LOG_FILE_LEVEL == 0
#undef USE_ADVANCED_SCRIPT_DEBUG_OUTPUT
#endif

#ifndef USE_ADVANCED_SCRIPT_DEBUG_OUTPUT
#define USE_BASIC_SCRIPT_DEBUG_OUTPUT
#endif

#ifdef MASTER
#undef USE_ADVANCED_SCRIPT_DEBUG_OUTPUT
#undef USE_BASIC_SCRIPT_DEBUG_OUTPUT
#endif

#ifndef MISSION_REPLAY
#undef USE_MISSION_REPLAY_OVERRIDE_FOR_NON_MOBILE_SCRIPT
#endif

// Replay
//#define DONT_FIX_REPLAY_BUGS // keeps various bugs in CReplay, some of which are fairly cool!
//#define USE_BETA_REPLAY_MODE // adds another replay mode, a few seconds slomo (caution: buggy!)

// Vehicles
#define EXPLODING_AIRTRAIN	// can blow up jumbo jet with rocket launcher
#define CPLANE_ROTORS		// make the rotors of the NPC police heli rotate

// Pickups
//#define MONEY_MESSAGES
#define CAMERA_PICKUP

// Peds
#define CANCELLABLE_CAR_ENTER

// Camera
#define IMPROVED_CAMERA		// Better Debug cam, and maybe more in the future
#define FREE_CAM		// Rotating cam

// Audio
#define EXTERNAL_3D_SOUND // use external engine to simulate 3d audio spatialization. OpenAL would not work without it (because it works in a 3d space
                          // originally and making it work in 2d only requires more resource). Will not work on PS2
#define AUDIO_REFLECTIONS // Enable audio reflections. This is enabled in all vanilla versions
#define AUDIO_REVERB // Enable audio reverb. It was disabled in PS2 and mobile versions
#define RADIO_SCROLL_TO_PREV_STATION // Won't work without FIX_BUGS
#define AUDIO_CACHE // cache sound lengths to speed up the cold boot
#define PS2_AUDIO_CHANNELS // increases the maximum number of audio channels to PS2 value of 43 (PC has 28 originally)
#define PS2_AUDIO_PATHS // changes audio paths for cutscenes and radio to PS2 paths (needs vbdec on MSS builds)
#ifdef __EMSCRIPTEN__
	// PC retail assets ship .mp3/.wav (.adf radio), not PS2 .vb: use PC tables.
	#undef PS2_AUDIO_PATHS
#endif
//#define AUDIO_OAL_USE_SNDFILE // use libsndfile to decode WAVs instead of our internal decoder
#define AUDIO_OAL_USE_MPG123 // use mpg123 to support mp3 files
#define PAUSE_RADIO_IN_FRONTEND // pause radio when game is paused
#define ATTACH_RELEASING_SOUNDS_TO_ENTITIES // sounds would follow ped and vehicles coordinates if not being queued otherwise
#define USE_TIME_SCALE_FOR_AUDIO // slow down/speed up sounds according to the speed of the game
#ifndef __EMSCRIPTEN__
#define MULTITHREADED_AUDIO // for streams. requires C++11 or later
#endif
// Web monohilo (como dos.zone): audio sincrono, sin std::thread.

#ifdef AUDIO_OPUS
#define AUDIO_OAL_USE_OPUS // enable support of opus files
//#define OPUS_AUDIO_PATHS // (not supported on VC yet) changes audio paths to opus paths (doesn't work if AUDIO_OAL_USE_OPUS isn't enabled)
//#define OPUS_SFX  // enable if your sfx.raw is encoded with opus (doesn't work if AUDIO_OAL_USE_OPUS isn't enabled)

#ifndef AUDIO_OAL_USE_OPUS
#undef OPUS_AUDIO_PATHS
#undef OPUS_SFX
#endif

#endif

// Streaming
#if !defined(_WIN32) && !defined(__SWITCH__)
	//#define ONE_THREAD_PER_CHANNEL // Don't use if you're not on SSD/Flash - also not utilized too much right now(see commented LoadAllRequestedModels in Streaming.cpp)
	#define FLUSHABLE_STREAMING // Make it possible to interrupt reading when processing file isn't needed anymore.
#endif
#define BIG_IMG // Not complete - allows to read larger img files

//#define SQUEEZE_PERFORMANCE
#ifdef SQUEEZE_PERFORMANCE
	#undef PS2_ALPHA_TEST
	#undef NO_ISLAND_LOADING
	#undef PS2_AUDIO_CHANNELS
	#undef EXTENDED_OFFSCREEN_DESPAWN_RANGE
#endif

// if these defines are enabled saves are not vanilla compatible without COMPATIBLE_SAVES
#ifndef COMPATIBLE_SAVES
#undef USE_CUTSCENE_SHADOW_FOR_PED
#endif

#ifdef GTA_HANDHELD
	#define IGNORE_MOUSE_KEYBOARD // ignore mouse & keyboard input
#endif

#ifdef __SWITCH__
	#define USE_UNNAMED_SEM // named semaphores are unsupported on the switch
#endif

#ifdef __EMSCRIPTEN__
	#define USE_UNNAMED_SEM // sem_open() is unsupported under Emscripten/pthreads
	// Web/MEMFS: no generar models/txd.img (1.2GB regenerados en RAM en cada arranque);
	// las texturas se cargan sueltas con decode DXT por software (gl3raster).
	#undef USE_TXD_CDIMAGE
#endif

#endif // VANILLA_DEFINES

#if defined(AUDIO_OAL) && !defined(EXTERNAL_3D_SOUND)
#error AUDIO_OAL cannot work without EXTERNAL_3D_SOUND
#endif
#if defined(GTA_PS2) && defined(EXTERNAL_3D_SOUND)
#error EXTERNAL_3D_SOUND cannot work on PS2
#endif
