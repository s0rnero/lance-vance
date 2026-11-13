#pragma once

// disables (most) stuff that wasn't in original gta-vc.exe
#ifdef __MWERKS__
#define VANILLA_DEFINES
#endif

#define VICEEXT_VICE_EXTENDED

#ifdef VICEEXT_VICE_EXTENDED
enum Config {
	NUMPLAYERS = 1,

	NUMCDIMAGES = 6,
	MAX_CDIMAGES = 32,
	MAX_CDCHANNELS = 5,

	MODELINFOSIZE = 10000,
	TXDSTORESIZE = 1500,
	COLSTORESIZE = 100,
	EXTRADIRSIZE = 512,
	CUTSCENEDIRSIZE = 512,

	SIMPLEMODELSIZE = 10000,
	TIMEMODELSIZE = 10000,
	CLUMPMODELSIZE = 100,
	WEAPONMODELSIZE = 77,
	PEDMODELSIZE = 130,
	VEHICLEMODELSIZE = 210,
	TWODFXSIZE = 3000,

	MAXVEHICLESLOADED = 50,

	NUMOBJECTINFO = 210,

	NUMCUTSCENEOBJECTS = 50,

	NUMANIMBLOCKS = 40,
	NUMANIMATIONS = 512,

	NUMTEMPOBJECTS = 40,

	NUM_PATHNODES = 9650,
	NUM_CARPATHLINKS = 3500,
	NUM_MAPOBJECTS = 1250,
	NUM_PATHCONNECTIONS = 20400,
	NUM_WANTEDPATHNODES = 20,
	NUM_WANTEDPATHS = 100,

	NUMALPHALIST = 20,
	NUMBOATALPHALIST = 20,
	NUMALPHAENTITYLIST = 2000,
	NUMALPHAUNTERWATERENTITYLIST = 30,
	NUMCOLCACHELINKS = 50,
	NUMREFERENCES = 800,

	NUMAUDIOZONES = 14,
	NUMINFOZONES = 169,
	NUMMAPZONES = 39,
	NUMNAVIGZONES = 20,

	NUMATTRIBZONES = 704,

	NUMOCCLUSIONVOLUMES = 350,
	NUMACTIVEOCCLUDERS = 48,

	PATHNODESIZE = 4500,

	NUMWEATHERS = 7,
	NUMHOURS = 24,

	NUMEXTRADIRECTIONALS = 4,
	NUMANTENNAS = 8,
	NUMCORONAS = 2000,
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
	NUMRADARBLIPS = 300,
	NUMGENERALPICKUPS = 320,
	NUMSCRIPTEDPICKUPS = 100,
	NUMPICKUPS = NUMGENERALPICKUPS + NUMSCRIPTEDPICKUPS,
	NUMCOLLECTEDPICKUPS = 100,
	NUMPACMANPICKUPS = 256,
	NUMEVENTS = 64,

	NUM_CARGENS = 185,

	NUM_PATH_NODES_IN_AUTOPILOT = 8,

	NUM_ACCIDENTS = 20,
	NUM_FIRES = 40,
	NUM_GARAGES = 35,
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
#else
enum Config {
	NUMPLAYERS = 1,

	NUMCDIMAGES = 6, // gta3.img duplicates (not used on PC)
	MAX_CDIMAGES = 8, // additional cdimages
	MAX_CDCHANNELS = 5,

	MODELINFOSIZE = 6500,	// 4900 on PS2
	TXDSTORESIZE = 1385,
	COLSTORESIZE = 31,
	EXTRADIRSIZE = 256,
	CUTSCENEDIRSIZE = 512,

	SIMPLEMODELSIZE = 3885,
	TIMEMODELSIZE = 385,
	CLUMPMODELSIZE = 5,
	WEAPONMODELSIZE = 37,
	PEDMODELSIZE = 130,
	VEHICLEMODELSIZE = 110,
	TWODFXSIZE = 1210,

	MAXVEHICLESLOADED = 50, // 70 on mobile

	NUMOBJECTINFO = 210,

	// Pool sizes
	NUMPTRNODES = 50000,
	NUMENTRYINFOS = 3200,
	NUMPEDS = 140,
	NUMVEHICLES = 110,
	NUMBUILDINGS = 7000,
	NUMTREADABLES = 1,
	NUMOBJECTS = 460,
	NUMDUMMIES = 2340,
	NUMAUDIOSCRIPTOBJECTS = 192,
	NUMCOLMODELS = 4400,
	NUMCUTSCENEOBJECTS = 50,	// not a pool in VC

	NUMANIMBLOCKS = 35,
	NUMANIMATIONS = 450,

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
#endif

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

#define VICEEXT_SWIMMING		// EnableSwimming=1: nadar sin morir (el jugador no se ahoga)
#define VICEEXT_AUTOSAVE		// v2.5 "Autosave after completing a mission": guarda solo al superar una misión (ranura 9, la del menú de carga) (sección 3, bloque C2)
#define VICEEXT_SAVE_ANYWHERE	// v2.5 "Saving anywhere. You must not be on a mission, not have a search level and not move": opción en el menú de pausa (sección 3, bloque C2)
#if defined VICEEXT_AUTOSAVE && defined VICEEXT_SAVE_ANYWHERE
#define VICEEXT_AUTOSAVE_AND_SAVE_ANYWHERE
#endif
#define VICEEXT_LOAD_SAVE_ON_STARTUP

#define VICEEXT_CAMERA_FP_INIT			// 1a persona: al entrar la camara mira a donde mira la cara del
#define VICEEXT_ENV_LIGHT_TINT			// el env map se modula con el color de la luz (el reflejo del semaforo sale rojo)
#define VICEEXT_NO_BRIGHT_LIGHT_CUBES		// los "cubos" de luz (CBrightLights) son los cuadros blancos del faro
#define VICEEXT_ENV_MAP_COEF			// el reflejo de la chapa usa el coeficiente del material (como el retail) y no el 0,25 del reVC
#define VICEEXT_SPECULAR				// especular Blinn por luz, coloreado con el color de la luz (como el retail de D3D8)
#define VICEEXT_FIX_SILENTPATCH
#define VICEEXT_FIX_WFP
#define VICEEXT_FIX_FV
#define VICEEXT_FIX_FV_ROTOR

#define VICEEXT_SP_STINGER_FIXES		// puas: ctor a cero, Remove idempotente, Init que libera al fallar el pool
#define VICEEXT_SP_ROADBLOCK_POOL		// crash de la barricada cuando el pool de objetos esta lleno
#define VICEEXT_SP_BOAT_EXTRAS		// extras en barcos (capota de Rio) y el typo boat_moving_high de Tropic
#define VICEEXT_SP_BOAT_MOVING_RADAR		// el radar del Tropic se anima
#define VICEEXT_SP_VEHICLE_EXPLODE_ONCE		// el coche no explota dos veces si el conductor se baja mientras explode
#define VICEEXT_SP_PANEL_DAMAGE		// los paneles del coche se separan al explotar
#define VICEEXT_SP_SECURICAR_DAMAGE		// los Securicars resisten mas el dano del jugador
#define VICEEXT_SP_COMPS_TO_USE_RESET		// si el coche creado no tiene extras, se reinician los pedidos
#define VICEEXT_SP_HANDLING_NAMES		// handling.cfg: comparacion exacta de nombres, sin prefijo unico
#define VICEEXT_SP_TAXI_CORONA		// la luz del taxi va en su sitio
#define VICEEXT_SP_BIKE_EXTRAS_FRAME		// los extras de las motos siguen la inclinacion de la bici
#define VICEEXT_SP_MUG_OBJECTIVE		// el objetivo de atraco no se cancela al instante
#define VICEEXT_SP_FIST_SHAKE		// Tommy agita el puño con nudillos y con las armas post-III
#define VICEEXT_SP_SCREWDRIVER_SOUND		// el destornillador hace sonido de impacto
#define VICEEXT_SP_TEARGAS_DAMAGE		// el gas lacrimigeno daña tambien a los personajes de mision
#define VICEEXT_SP_SPEECH_DELAY		// minimo global entre comentarios de los pedestrians
#define VICEEXT_SP_NPC_RPG			// los NPCs disparan el RPG en su sitio
#define VICEEXT_SP_TARGETTING_CHAR		// IS_PLAYER_TARGETTING_CHAR no da positivo sin apuntar
#define VICEEXT_SP_ROSENBERG_RAND		// el chat de los peds usa el azar de 15 bits del PC
#define VICEEXT_SP_ZERO_AMMO			// un arma con 0 balas se queda con 1
#define VICEEXT_SP_ENV_MAP_EXTRAS		// env map y pipe en los componentes extra del coche
#define VICEEXT_SP_CORONA_LINES		// las lineas de corona llevan recipZ (GPUs no NVIDIA)
#define VICEEXT_SP_SCRIPT_SPRITE_BILINEAR	// filtrado bilineal en los sprites de script
#define VICEEXT_SP_BLIP_BILINEAR		// filtrado bilineal en los blips con el radar apagado
#define VICEEXT_SP_HEAT_HAZE_SCALE		// el heat haze no escala dos veces
#define VICEEXT_SP_MINIMAL_HUD		// el HUD se atenua cuando no se usa
#define VICEEXT_SP_NEW_GAME_RESET		// reset de variables al empezar partida nueva
#define VICEEXT_SP_MOUSE_SENS_RESET		// la sensibilidad del raton no se reinicia en partida nueva
#define VICEEXT_SP_BUSTED_WEAPONS		// al ser arrestado, las armas no se quitan dos veces
#define VICEEXT_SP_METRIC_UNITS		// unidades metricas o imperiales segun el locale
#define VICEEXT_SP_PICKUP_EFFECTS		// el texto de los pickups de dinero es rojo generico y el minigun no brilla en rosa
#define VICEEXT_SP_PROPERTY_BLIPS		// los iconos de propiedades en venta salen en el radar de juego
#define VICEEXT_SP_RADAR_DISC		// el disco del radar se encoge 2 px y su posicion escala
#define VICEEXT_SP_OPTIONS			// las opciones del mod (features.ini): MinimalHUD, sliding texts, property blips, sprites a escala, retardo del chat

#define VICEEXT_WFP_OPTIONS			// interruptor maestro de las opciones del WFP
// 30/10 (29.0): la pantalla "Ajustes de pantalla" del WFP se invento en el port
// y el jugador pidio quitarla del menu. Las opciones se siguen leyendo del INI
// (VICEEXT_WFP_OPTIONS), solo desaparece la fila y su pantalla.
//#define VICEEXT_WFP_MENU
#define VICEEXT_WFP_CAR_SPEED_FOV		// FOV por velocidad en la camara VC (CarSpeedDependantFOV)
#define VICEEXT_WFP_FOV_MULTIPLIERS		// API de multiplicadores de FOV por hash (SetFOVMultiplier/RemoveFOVMultiplier/GetCurrentFOV)
#define VICEEXT_WFP_EMERGENCY_FOV		// FOV de las camaras de emergencia: el estandar, no uno fijo (70/factor)
#define VICEEXT_WFP_SKIP_CAMERA_COLLISION	// con aspecto >= 2 la camara no choca con la geometria
#define VICEEXT_WFP_INVERSE_FOV_COLLISION	// el FOV de la colision de camara se convierte a la inversa (4:3) antes de usarse
#define VICEEXT_WFP_DONT_TOUCH_FOV		// DontTouchFOV: el FOV no se estira por aspecto
#define VICEEXT_WFP_CUTSCENE_FOV		// el FOV de cinematica sigue el zoom del borde
#define VICEEXT_WFP_WHEELCAM_FOV		// el FOV de la camara de rueda se mantiene en 70 grados horizontales
#define VICEEXT_WFP_HUD_SCALE			// multiplicador de escala del HUD en X e Y (HudWidthScale/HudHeightScale)
#define VICEEXT_WFP_RADAR_SIZE			// tamano del disco del radar (RadarWidth/RadarHeight/IVRadarScaling)
#define VICEEXT_WFP_HUD_ANCHOR			// el HUD se ancla a la izquierda o a la derecha segun el ancho (HudAspectRatioConstraint)
#define VICEEXT_WFP_SUBTITLES			// escala, reserva del radar y posicion de los subtitulos (SubtitlesScale)
#define VICEEXT_WFP_CROSSHAIR_ASPECT	// la reticula se escala con el aspecto de pantalla
#define VICEEXT_WFP_TEXT_OUTLINE		// el contorno de 16 direcciones sustituye a la sombra difusa (ReplaceTextShadowWithOutline)
#define VICEEXT_WFP_SMALLER_TEXT_SHADOWS	// la sombra del texto se dibuja en la posicion pequena
#define VICEEXT_WFP_SMALLER_VEHICLE_CORONA	// las luces de vehiculo se dibujan mas pequenas (SmallerVehicleCorona)
#define VICEEXT_WFP_NO_LIGHT_SQUARE		// no se dibuja el cuadrado de la luz (NoLightSquare)
#define VICEEXT_WFP_BRIGHT_LIGHTS_FAR		// las luces brillantes solo se dibujan a distancia
#define VICEEXT_WFP_PARTICLE_ASPECT		// las particulas se escalan con el ancho de la pantalla
#define VICEEXT_WFP_FADE_SCREEN_RECT		// el fundido usa el rectangulo de contenido de la cinematica
#define VICEEXT_WFP_CUTSCENE_BORDERS		// los bordes de cinematica se adaptan al aspecto (SmartCutsceneBorders)
#define VICEEXT_WFP_CUTSCENE_BORDER_ANIM	// los bordes de cinematica se animan y publican el zoom de la camara
#define VICEEXT_WFP_ASPECT_RATIO		// el aspecto de pantalla admite W:H libre (ForceAspectRatio)
#define VICEEXT_WFP_FRONTEND_ASPECT	// el aspecto del frontend se puede fijar (FrontendAspectRatio)
#define VICEEXT_WFP_CAM_SHAKE_VCS		// sacudida de camara proporcional a la velocidad (VCSCamShake)
#define VICEEXT_WFP_MAP_ANCHOR		// el mapa del menu conserva la fraccion de scroll al cambiar de resolucion
#define VICEEXT_WFP_MENU_CANVAS		// el menu se dibuja en un lienzo centrado con el aspecto pedido


#define VICEEXT_SKY_ENVMAP_SIZE			// el tamano del env map del coche es configurable (envMapSize)
#define VICEEXT_SKY_WORLD_PIPE			// conmutador del pipe de mundo (worldPipe)
#define VICEEXT_SKY_DISABLE_COLOUR_OVERLAY	// el overlay de color de las trails se puede desactivar
#define VICEEXT_SKY_TRAILS_PARAMS		// trailsBlur/trailsMotionBlur/currentBlurAlpha/currentBlurOffset como parametros
#define VICEEXT_SKY_WATERDROPS_EXTENDED	// las gotas de agua de pantalla tienen los modos Extended y Extended+Blood
#define VICEEXT_SKY_MATFX_BLEND		// el blend del env map de MatFX admite PS2, PC y Mobile (texblendSwitch)
#define VICEEXT_SKY_MATFX_TEXGEN		// la generacion de UV del env map de MatFX admite PS2, PC y Mobile (texgenSwitch)
#define VICEEXT_SKY_MATFX_PS2LIGHT		// la iluminacion de MatFX admite el caso PC ademas del PS2 (ps2light)
#define VICEEXT_SKY_LEEDS_CAR_PIPE		// pipe de coche Leeds: el reflejo usa la CarReflectionMask (carPipe=2)
#define VICEEXT_SKY_LEEDS_WORLD_PIPE		// pipe de mundo Leeds con el tweak de prelight (worldPipe=2)
#define VICEEXT_SKY_SEAM_FIX			// se redibuja el front buffer desplazado para tapar las costuras (seamFix)
#define VICEEXT_SKY_DUAL_PASS			// la geometria transparente se dibuja en dos pasadas (dualPass)
#define VICEEXT_ENVMAP_EVERY_N 4
#define VICEEXT_PERF_MINIMAL

#define VICEEXT_MONEY_NO_ZEROS

#define VICEEXT_PEDARBITER
#define VICEEXT_RADIO_ICONS
#define VICEEXT_EXTRA_CHEATS

#define VICEEXT_FEATURES_INI
#define VICEEXT_GPS_COLOR		// features.ini "Change the color of the GPS by RGB"
#define VICEEXT_GPS_COLOR_R 255
#define VICEEXT_GPS_COLOR_G 0
#define VICEEXT_GPS_COLOR_B 255

#define VICEEXT_CHEAT_EFFECTS	// v2.5 BIGHEADS/RCROCKET y v2.0 AIRWAYS con efecto real

#ifdef VICEEXT_VICE_EXTENDED
#define VICEEXT_UTILS
#define VICEEXT_OPCODES
#define VICEEXT_IMPROVED_TECH_PART
#define VICEEXT_DISPLAYED_COLLECTIBLES
#define VICEEXT_SECOND_SAMPLE_BANK
#ifdef VICEEXT_SECOND_SAMPLE_BANK
#define VICEEXT_NEW_WEAPONS
#ifdef VICEEXT_NEW_WEAPONS
#define VICEEXT_WEAPON_RCGRENADE
#define VICEEXT_WEAPON_AK47
#define VICEEXT_WEAPON_M16
#define VICEEXT_WEAPON_SHOTGUN2
#define VICEEXT_WEAPON_UZIOLD
#define VICEEXT_WEAPON_DEAGLE
#define VICEEXT_WEAPON_BERETTA
#define VICEEXT_WEAPON_STEYR
#define VICEEXT_WEAPON_GRENADE_LAUNCHER
#endif
#endif
#define VICEEXT_IMPROVED_MENU_AND_INPUT
#define VICEEXT_MORE_BUTTON_ICONS
#define VICEEXT_PC_KEY_ICONS
#define VICEEXT_CROUCH
#ifdef VICEEXT_FEATURES_INI
#define VICEEXT_CLIMBING
#define VICEEXT_ROCKET_LAUNCHER_THIRD_PERSON_AIM
#endif
#if defined(__EMSCRIPTEN__) && defined(VICEEXT_IMPROVED_MENU_AND_INPUT)
#define VICEEXT_TOUCH_CONTROLS
#endif
#define VICEEXT_FIRING_AND_AIMING
#define VICEEXT_FIRST_PERSON
#define VICEEXT_PED_VARIATIONS
#define VICEEXT_WEAPON_SIGHT
#define VICEEXT_TRIANGLE_FOR_MOUSE_RECRUIT
#define VICEEXT_GPS
#endif
#define VICEEXT_IMPROVED_VEHICLES_2
// Plan 58.0 T3/T4: lentes ambar pintadas en la carroceria (comparten material
// con la pintura) que se encienden en su sitio con un atlas derivado <atlas>_lens,
// sin mover ni crear geometria ni tintar el material. Tabla src/vehicles/
// BodyLensTable.h generada por tools/gen_body_lens.py sobre los seis sitios
// medidos del plan (esperant, boxville, fbiranch, mule, trash).
#define VICEEXT_BODY_LENS
#define VICEEXT_BODY_LENS_MAX	8
#define VICEEXT_VOLANTE_GAIN		2.0f
#define VICEEXT_IMPROVED_VEHICLES
#define VICEEXT_EX_VCPD_WINTERGREEN
#define VICEEXT_NEW_VEHICLE_LOADER
#define VICEEXT_POLICE_BIKE	VICEEXT_EX_VCPD_WINTERGREEN	 // alias 6.20 CarAI/CarCtrl/Population.cpp
#define VICEEXT_PED_ANIMS_IN_CAR
#define VICEEXT_SKIN_FAILSAFE
#define VICEEXT_VOLANTE 1

#define VICEEXT_WANTED_PATHS

#define VICEEXT_IS_HIGH_VEHICLE

// Plan 56.0 T4: los vehiculos sin interior medido (src/vehicles/
// NoInteriorModelTable.h) no admiten la 1a persona del coche, y al subir con la
// preferencia puesta en ella se pasa a tercera.
#define VICEEXT_NO_INTERIOR_CAMERA

// Plan 56.0 T5: el charco de luz del suelo (faros y frenos) se ancla a la
// lampara del modelo en vez de crecer con su longitud.
#define VICEEXT_LIGHT_SHADOW_ANCHOR

// Plan 48.0: lo que seguia dependiendo de los FPS y el asiento de los NPC.
// Cada correccion se apaga con su define; las constantes se afinan aqui.
#define VICEEXT_HORN_LOGICAL_TICK		// anillo del claxon muestreado a 30 Hz
#define VICEEXT_HELI_SPIN_SCALED		// rotor: decaimiento escalado y rampa larga
#define VICEEXT_VEH_FADE_SCALED			// alpha de vehiculos escalado + jinetes ocultos
#define VICEEXT_CLOCK_CATCHUP			// el reloj recupera el tiempo de los hitches
#define VICEEXT_PED_VS_CAR_PUSH			// empuje ped<->coche atenuado en vez de anulado
// #define VICEEXT_SEAT_BY_PED_HEIGHT	// 49.0 T15: la compensacion por
// altura hacia mal a los bajitos; todos los NPCs van en la misma postura.

#define OD_HELI_RAMP				0.0005f	// con 0.001 el rotor tarda 3 s; ahora 6 s
#define OD_HELI_RAMP_RC				0.0015f	// RC: la misma proporcion 3x
#define OD_HELI_DECAY				0.0005f	// por tick de 30 Hz
#define OD_VEH_FADE_IN				16.0f	// idem
#define OD_VEH_FADE_OUT				8.0f
#define OD_SEAT_GHOST_ALPHA			32	// por debajo el coche no se ve ni un pixel
#define OD_CLOCK_MAX_CATCHUP			20	// minutos de juego recuperados de golpe
#define OD_VEH_ALMOST_STOPPED_SQ		0.0004f	// (0.02)^2: por debajo, el coche no se mueve
#define OD_PUSH_VS_PED				0.25f
#define OD_SHIFT_VS_PED				0.15f
#define OD_PISON_MAX					0.05f	// desplazamiento maximo del peon por frame al empujarlo un coche

// Plan 49.0: el censo de logica por frame (ADR-050). Cada correccion se
// apaga con su define; las constantes se afinan aqui.
#define VICEEXT_STICK_TIMESTEP_SCALED	// T1 giro y apuntado con mando escalados
#define VICEEXT_AIR_TURN_DAMPED			// T2 amortiguacion del giro libre
#define VICEEXT_STREAM_LOGICAL			// T4 cadencias de streaming por tick logico
#define VICEEXT_SUSPENSION_SCALED		// T5 baches de suspension por tick logico
#define VICEEXT_CAM_WINDOWS_LOGICAL		// T6 ventanas de camara por tick logico
#define VICEEXT_SWIM_LOGICAL			// T7 temporizadores de nado por tick logico
#define VICEEXT_BLOOD_LOGICAL			// T8 charcos de sangre uno por tick logico
#define VICEEXT_HUD_WINDOWS_LOGICAL		// T9 parpadeos y rampas del HUD
#define VICEEXT_AUDIO_WINDOWS_LOGICAL	// T10 lluvia, retune y reflexiones
#define VICEEXT_SPLASH_LOGICAL			// T16 gotas de lente: duracion y presupuesto
#define VICEEXT_ALLOC_GUARD				// T14 cota de texturas por TXD + trazas
#define VICEEXT_TYRE_TRACE				// T12 sola traza del neumatico

#define OD_AIR_TURN_DAMP		0.99f		// T2: factor por tick de 30 Hz
#define OD_STREAM_VEH_FRAMES	350		// T4: ticks logicos entre recargas de clase
#define OD_SPLASH_TICKS			14		// T16: duracion del salpicado, ticks logicos
#define OD_SPLASH_REARM_MS		500u		// T16: minima espera entre dos salpicados
#define OD_TXD_MAX_TEXTURES		512		// T14: mas de esto en un TXD es registro roto
#define OD_TXD_TRACE_MIN_TEX	64		// T14: TXD grandes que se nombran en la traza
#define OD_LOAD_TRACE_SECTORS	128		// T14: desde que tamano se traza la carga
#define OD_SEAT_LIFT_ALL			0.0f		// T15: asiento uniforme; 0.0 = el de Tommy

// Plan 50.0: el salto de la intro y el anuncio de puente al empezar la partida.
#define VICEEXT_RADIO_PSA_GUARD		// T19: el anuncio no puede casar la radio

#define OD_PSA_SILENCE_MS		2000u		// T19: ms de anuncio sin oir nada antes de rendirse
#define OD_PSA_ANNOUNCE_RETRY_MS	250u		// T22: ms entre reintentos de abrir el aviso
#define OD_PSA_STATION_DEAD_MS	8000u		// T22: ms de fallos seguidos antes de morir la emisora

// Plan 49.1: los cinco puntos del jugador tras el 50.0-T1 (ADR-050).
#define VICEEXT_FOV_TRACE			// T28: FOVLN publica el FOV escalado solo cuando cambia
#define VICEEXT_DRUNK_FADE_NO_WRAP	// T24: el desvanecido de la embriaguez no desborda el uint8
#define VICEEXT_CEILING_RAISED		// T25: el techo de vuelo vanilla se levanta al doble
#define VICEEXT_SPLASH_REARM		// T23: un solo chapuzon por ventana en el agua somera
#define VICEEXT_SWIM_MOD_BORDER		// T34: entrada/salida del agua como las mide el mod (REA)
#define VICEEXT_SWIM_CLIMB_EXIT		// T34: salir del agua trepando, con el climbing que ya portamos

// (T27 retiro OD_FOV_RAMP_MS: la rampa era la que producia el zoom visible)
#define OD_VUELO_MS				250u	// T25: ms entre lineas VUELO
#define OD_CEILING_Z_SCALE			0.5f	// T25: 0.5 = el techo vanilla al doble de alto; 1.0 = vanilla
#define OD_SPLASH_EVENT_MS			450u	// T23: ms entre chapuzones del jugador
#define OD_SWIM_FOOT_M			0.30f	// 52.0: suelo a menos de esto bajo los pies = apoyado
#define OD_SWIM_WADE_M			1.60f	// 52.0: agua sobre los pies que se vadea de pie
#define OD_SWIM_ENTER_M			0.30f	// 52.0: calado para entrar flotando
#define OD_SWIM_EXIT_M			0.10f	// 52.0: calado para salir
#define OD_SWIM_FADE_OUT			-6.0f	// T34: fundido de la brazada al salir (el del mod)
#define OD_SWIM_FADE_CUT			-4.0f	// T34: corte por cuadro de salto/caida mientras nada
#define OD_SWIM_CLIMB_MAX_M			1.20f	// T34: altura agarrable desde el agua (de pie son 2,25)
#define OD_SWIM_HOP_MIN_M			0.35f	// T34: reborde mas bajo del subidito automatico
#define OD_SWIM_HOP_MAX_M			0.90f	// T34: reborde mas alto del subidito automatico
#define OD_SWIM_HOP_FWD_M			1.50f	// T34: hacia donde mira el reborde del subidito
#ifdef __EMSCRIPTEN__
#define VICEEXT_WEB_GALLERY
#define VICEEXT_WEB_MUSIC_MANAGER
#define VICEEXT_WEB_SAVE_MANAGER
#define VICEEXT_WEB_SKINS
#endif

// Hud, frontend and radar
#define PC_MENU
#define FIX_RADAR			// use radar size from early version before R* broke it
#define RADIO_OFF_TEXT		// Won't work without FIX_BUGS

#ifndef PC_MENU
#	define PS2_MENU
//#	define PS2_MENU_USEALLPAGEICONS
#else
#	define MAP_ENHANCEMENTS			// Adding waypoint and better mouse support
#	if defined(XINPUT) || defined(GTA_HANDHELD) || defined(VICEEXT_IMPROVED_MENU_AND_INPUT)
#		define GAMEPAD_MENU		// Add gamepad menu
#	endif
#ifdef VICEEXT_IMPROVED_MENU_AND_INPUT
#	define CIRCLE_BACK_BUTTON
#else
#	define TRIANGLE_BACK_BUTTON
#endif
#define LEGACY_MENU_OPTIONS			// i.e. frame sync(vsync)
// #define XBOX_MESSAGE_SCREEN			// Blue background, no "saved successfully press OK" screen etc.
#	define CUSTOM_FRONTEND_OPTIONS

#	ifdef CUSTOM_FRONTEND_OPTIONS
#		define GRAPHICS_MENU_OPTIONS // otherwise Display settings will be scrollable
#		define NO_ISLAND_LOADING  // disable loadscreen between islands via loading all island data at once, consumes more memory and CPU
#		define CUTSCENE_BORDERS_SWITCH
#		define MULTISAMPLING		// adds MSAA option
#ifndef VICEEXT_IMPROVED_MENU_AND_INPUT
#		define INVERT_LOOK_FOR_PAD // enable the hidden option
#endif
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
	#undef USE_TXD_CDIMAGE
#endif

#endif // VANILLA_DEFINES

#if defined(AUDIO_OAL) && !defined(EXTERNAL_3D_SOUND)
#error AUDIO_OAL cannot work without EXTERNAL_3D_SOUND
#endif
#if defined(GTA_PS2) && defined(EXTERNAL_3D_SOUND)
#error EXTERNAL_3D_SOUND cannot work on PS2
#endif
