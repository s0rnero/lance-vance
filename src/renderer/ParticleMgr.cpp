#include "common.h"

#include "main.h"
#include "FileMgr.h"
#include "ParticleMgr.h"
#include "Timer.h"     // CTimer: la traza P4 lleva frame y tiempo simulado
#include "ondemand.h"  // web: ODTRACES -> odtrace.log

cParticleSystemMgr mod_ParticleSystemManager;

const char *ParticleFilename = "PARTICLE.CFG";

#ifdef VICEEXT_PARTICLE_NAMES
// Vice Extended (familia 3/5): su `particle.cfg` anade 5 tipos al final. Con el
// parser posicional de serie (`type++`) eso no se puede servir: el tipo saldria
// de la POSICION y cualquier entrada nueva desplaza a las siguientes, hasta
// pasarse de MAX_PARTICLES. Aqui se resuelve por NOMBRE contra esta tabla, que
// es el enum entero: los dos ficheros reales (el servido, 83 entradas, y el del
// mod, 88) coinciden nombre a nombre con el enum sin el prefijo PARTICLE_ (los 5
// del mod son los ultimos: ENGINE_SMOKE2_SMALL, ENGINE_STEAM_SMALL,
// ENGINE_SMOKE_SMALL, CARFLAME_SMALL y CARFLAME_SMOKE_SMALL32).
struct tParticleNameEntry
{
	const char *name;
	tParticleType type;
};

static const tParticleNameEntry odParticleNames[] = {
	{ "SPARK", PARTICLE_SPARK },
	{ "SPARK_SMALL", PARTICLE_SPARK_SMALL },
	{ "WATER_SPARK", PARTICLE_WATER_SPARK },
	{ "WHEEL_DIRT", PARTICLE_WHEEL_DIRT },
	{ "SAND", PARTICLE_SAND },
	{ "WHEEL_WATER", PARTICLE_WHEEL_WATER },
	{ "BLOOD", PARTICLE_BLOOD },
	{ "BLOOD_SMALL", PARTICLE_BLOOD_SMALL },
	{ "BLOOD_SPURT", PARTICLE_BLOOD_SPURT },
	{ "DEBRIS", PARTICLE_DEBRIS },
	{ "DEBRIS2", PARTICLE_DEBRIS2 },
	{ "FLYERS", PARTICLE_FLYERS },
	{ "WATER", PARTICLE_WATER },
	{ "FLAME", PARTICLE_FLAME },
	{ "FIREBALL", PARTICLE_FIREBALL },
	{ "GUNFLASH", PARTICLE_GUNFLASH },
	{ "GUNFLASH_NOANIM", PARTICLE_GUNFLASH_NOANIM },
	{ "GUNSMOKE", PARTICLE_GUNSMOKE },
	{ "GUNSMOKE2", PARTICLE_GUNSMOKE2 },
	{ "CIGARETTE_SMOKE", PARTICLE_CIGARETTE_SMOKE },
	{ "SMOKE", PARTICLE_SMOKE },
	{ "SMOKE_SLOWMOTION", PARTICLE_SMOKE_SLOWMOTION },
	{ "DRY_ICE", PARTICLE_DRY_ICE },
	{ "TEARGAS", PARTICLE_TEARGAS },
	{ "GARAGEPAINT_SPRAY", PARTICLE_GARAGEPAINT_SPRAY },
	{ "SHARD", PARTICLE_SHARD },
	{ "SPLASH", PARTICLE_SPLASH },
	{ "CARFLAME", PARTICLE_CARFLAME },
	{ "STEAM", PARTICLE_STEAM },
	{ "STEAM2", PARTICLE_STEAM2 },
	{ "STEAM_NY", PARTICLE_STEAM_NY },
	{ "STEAM_NY_SLOWMOTION", PARTICLE_STEAM_NY_SLOWMOTION },
	{ "GROUND_STEAM", PARTICLE_GROUND_STEAM },
	{ "ENGINE_STEAM", PARTICLE_ENGINE_STEAM },
	{ "RAINDROP", PARTICLE_RAINDROP },
	{ "RAINDROP_SMALL", PARTICLE_RAINDROP_SMALL },
	{ "RAIN_SPLASH", PARTICLE_RAIN_SPLASH },
	{ "RAIN_SPLASH_BIGGROW", PARTICLE_RAIN_SPLASH_BIGGROW },
	{ "RAIN_SPLASHUP", PARTICLE_RAIN_SPLASHUP },
	{ "WATERSPRAY", PARTICLE_WATERSPRAY },
	{ "WATERDROP", PARTICLE_WATERDROP },
	{ "BLOODDROP", PARTICLE_BLOODDROP },
	{ "EXPLOSION_MEDIUM", PARTICLE_EXPLOSION_MEDIUM },
	{ "EXPLOSION_LARGE", PARTICLE_EXPLOSION_LARGE },
	{ "EXPLOSION_MFAST", PARTICLE_EXPLOSION_MFAST },
	{ "EXPLOSION_LFAST", PARTICLE_EXPLOSION_LFAST },
	{ "CAR_SPLASH", PARTICLE_CAR_SPLASH },
	{ "BOAT_SPLASH", PARTICLE_BOAT_SPLASH },
	{ "BOAT_THRUSTJET", PARTICLE_BOAT_THRUSTJET },
	{ "WATER_HYDRANT", PARTICLE_WATER_HYDRANT },
	{ "WATER_CANNON", PARTICLE_WATER_CANNON },
	{ "EXTINGUISH_STEAM", PARTICLE_EXTINGUISH_STEAM },
	{ "PED_SPLASH", PARTICLE_PED_SPLASH },
	{ "PEDFOOT_DUST", PARTICLE_PEDFOOT_DUST },
	{ "CAR_DUST", PARTICLE_CAR_DUST },
	{ "HELI_DUST", PARTICLE_HELI_DUST },
	{ "HELI_ATTACK", PARTICLE_HELI_ATTACK },
	{ "ENGINE_SMOKE", PARTICLE_ENGINE_SMOKE },
	{ "ENGINE_SMOKE2", PARTICLE_ENGINE_SMOKE2 },
	{ "CARFLAME_SMOKE", PARTICLE_CARFLAME_SMOKE },
	{ "FIREBALL_SMOKE", PARTICLE_FIREBALL_SMOKE },
	{ "PAINT_SMOKE", PARTICLE_PAINT_SMOKE },
	{ "TREE_LEAVES", PARTICLE_TREE_LEAVES },
	{ "CARCOLLISION_DUST", PARTICLE_CARCOLLISION_DUST },
	{ "CAR_DEBRIS", PARTICLE_CAR_DEBRIS },
	{ "BIRD_DEBRIS", PARTICLE_BIRD_DEBRIS },
	{ "HELI_DEBRIS", PARTICLE_HELI_DEBRIS },
	{ "EXHAUST_FUMES", PARTICLE_EXHAUST_FUMES },
	{ "RUBBER_SMOKE", PARTICLE_RUBBER_SMOKE },
	{ "BURNINGRUBBER_SMOKE", PARTICLE_BURNINGRUBBER_SMOKE },
	{ "BULLETHIT_SMOKE", PARTICLE_BULLETHIT_SMOKE },
	{ "GUNSHELL_FIRST", PARTICLE_GUNSHELL_FIRST },
	{ "GUNSHELL", PARTICLE_GUNSHELL },
	{ "GUNSHELL_BUMP1", PARTICLE_GUNSHELL_BUMP1 },
	{ "GUNSHELL_BUMP2", PARTICLE_GUNSHELL_BUMP2 },
	{ "ROCKET_SMOKE", PARTICLE_ROCKET_SMOKE },
	{ "TEST", PARTICLE_TEST },
	{ "BIRD_FRONT", PARTICLE_BIRD_FRONT },
	{ "SHIP_SIDE", PARTICLE_SHIP_SIDE },
	{ "BEASTIE", PARTICLE_BEASTIE },
	{ "RAINDROP_2D", PARTICLE_RAINDROP_2D },
	{ "HEATHAZE", PARTICLE_HEATHAZE },
	{ "HEATHAZE_IN_DIST", PARTICLE_HEATHAZE_IN_DIST },
	{ "ENGINE_SMOKE2_SMALL", PARTICLE_ENGINE_SMOKE2_SMALL },
	{ "ENGINE_STEAM_SMALL", PARTICLE_ENGINE_STEAM_SMALL },
	{ "ENGINE_SMOKE_SMALL", PARTICLE_ENGINE_SMOKE_SMALL },
	{ "CARFLAME_SMALL", PARTICLE_CARFLAME_SMALL },
	{ "CARFLAME_SMOKE_SMALL32", PARTICLE_CARFLAME_SMOKE_SMALL32 }
};

// Tipo del nombre, o -1 si no esta en la tabla.
static int32 odFindParticleType(const char *name)
{
	for (int32 i = 0; i < (int32)ARRAY_SIZE(odParticleNames); i++) {
		if (!strcmp(odParticleNames[i].name, name))
			return (int32)odParticleNames[i].type;
	}
	return -1;
}

// Sumidero de las entradas cuyo nombre no conocemos: sus parametros se escriben
// aqui y se tiran. Sin esto apuntarian al `entry` anterior y lo corromperian.
static tParticleSystemData odUnknownParticleEntry;

#define VICEEXT_P4_EMIT(odGen, odBuf, ...) do { int odP4_ret = snprintf(odBuf, sizeof(odBuf), __VA_ARGS__); if (odP4_ret < 0 || odP4_ret >= (int)sizeof(odBuf)) { char odP4_of[220]; snprintf(odP4_of, sizeof odP4_of, "P4 kind=overflow schema=1 gen=%u frame=%u sim=%.4f case=0 id=0 cap=%d ret=%d", (unsigned)(odGen), (unsigned)CTimer::GetFrameCounter(), CTimer::GetTimeInMilliseconds() * 0.001f, (int)sizeof(odBuf), odP4_ret); ODTRACES(odP4_of); } else { ODTRACES(odBuf); } } while (0)
#endif

cParticleSystemMgr::cParticleSystemMgr()
{
	memset(this, 0, sizeof(*this));
}

void cParticleSystemMgr::Initialise()
{
	LoadParticleData();
	
	for ( int32 i = 0; i < MAX_PARTICLES; i++ )
		m_aParticles[i].m_pParticles = nil;
}

void cParticleSystemMgr::LoadParticleData()
{
	CFileMgr::SetDir("DATA");
	CFileMgr::LoadFile(ParticleFilename, work_buff, ARRAY_SIZE(work_buff), "r");
	CFileMgr::SetDir("");
	
	tParticleSystemData *entry = nil;
#ifndef VICEEXT_PARTICLE_NAMES
	int32 type = PARTICLE_FIRST;
#else
	// Recuento para la traza de carga: verificacion de que el fichero del mod
	// entra entero y de que no queda ningun nombre sin soporte.
	int32 odLoaded = 0;
	int32 odNew = 0;
	int32 odUnknown = 0;
	int32 odTotal = 0;
	char odTrace[240];
#endif
	
	char *lineStart = (char *)work_buff;
	char *lineEnd = lineStart + 1;
	
	char line[500];
	char delims[4];

	while ( true )
	{
		ASSERT(lineStart != nil);
		ASSERT(lineEnd != nil);
		
		while ( *lineEnd != '\n' )
			++lineEnd;
		
		int32 lineLength = lineEnd - lineStart;
		
		ASSERT(lineLength < 500);

		strncpy(line, lineStart, lineLength);
		
		line[lineLength] = '\0';

		// Fin de linea CRLF: sin esto el centinela llega como ";the end\r" y no
		// casa, y el bucle se pasaria del buffer. Con ficheros LF no cambia nada.
		if ( lineLength > 0 && line[lineLength - 1] == '\r' )
			line[lineLength - 1] = '\0';
		
		if ( !strcmp(line, ";the end") )
			break;
		
		if ( *line != ';' )
		{
			int32 param = CFG_PARAM_FIRST;

			strcpy(delims, " \t");
			
			char *value = strtok(line, delims);
			
			ASSERT(value != nil);
			
			do
			{	
				switch ( param )
				{
					case CFG_PARAM_PARTICLE_TYPE_NAME:
#ifdef VICEEXT_PARTICLE_NAMES
					{
						int32 odIndex = odFindParticleType(value);

						odTotal++;

						if ( odIndex < 0 )
						{
							// Nombre que el motor no conoce: no se sirve como si fuese un
							// tipo valido (corromperia esta entrada y las siguientes).
							odUnknown++;
							entry = &odUnknownParticleEntry;
							VICEEXT_P4_EMIT(1, odTrace, "P4 kind=partcfg schema=1 gen=1 frame=%u sim=%.4f case=2 id=0 desconocido=%s", (unsigned)CTimer::GetFrameCounter(), CTimer::GetTimeInMilliseconds() * 0.001f, value);
						}
						else
						{
							ASSERT(odIndex < MAX_PARTICLES);
							entry = &m_aParticles[odIndex];
							ASSERT(entry != nil);
							entry->m_Type = (tParticleType)odIndex;
							odLoaded++;
							if ( odIndex >= PARTICLE_VICEEXT_FIRST )
								odNew++;
						}

						// Copia acotada: su fichero trae "CARFLAME_SMOKE_SMALL32" (22
						// caracteres) y el campo son 20; se recorta y se deja traza.
						if ( strlen(value) >= sizeof(entry->m_aName) )
							VICEEXT_P4_EMIT(1, odTrace, "P4 kind=partcfg schema=1 gen=1 frame=%u sim=%.4f case=2 id=0 nombre_largo=%s cap=%d", (unsigned)CTimer::GetFrameCounter(), CTimer::GetTimeInMilliseconds() * 0.001f, value, (int)sizeof(entry->m_aName));
						strncpy(entry->m_aName, value, sizeof(entry->m_aName) - 1);
						entry->m_aName[sizeof(entry->m_aName) - 1] = '\0';

						break;
					}
#else
						ASSERT(type < MAX_PARTICLES);
						entry = &m_aParticles[type];
						ASSERT(entry != nil);
						entry->m_Type = (tParticleType)type++;
						strcpy(entry->m_aName, value);
						break;
#endif

					case CFG_PARAM_RENDER_COLOURING_R:
						entry->m_RenderColouring.red = atoi(value);
						break;

					case CFG_PARAM_RENDER_COLOURING_G:
						entry->m_RenderColouring.green = atoi(value);
						break;

					case CFG_PARAM_RENDER_COLOURING_B:
						entry->m_RenderColouring.blue = atoi(value);
						break;

					case CFG_PARAM_INITIAL_COLOR_VARIATION:
						entry->m_InitialColorVariation = Min(atoi(value), 100);
						break;

					case CFG_PARAM_FADE_DESTINATION_COLOR_R:
						entry->m_FadeDestinationColor.red = atoi(value);
						break;

					case CFG_PARAM_FADE_DESTINATION_COLOR_G:
						entry->m_FadeDestinationColor.green = atoi(value);
						break;

					case CFG_PARAM_FADE_DESTINATION_COLOR_B:
						entry->m_FadeDestinationColor.blue = atoi(value);
						break;

					case CFG_PARAM_COLOR_FADE_TIME:
						entry->m_ColorFadeTime = atoi(value);
						break;

					case CFG_PARAM_DEFAULT_INITIAL_RADIUS:
						entry->m_fDefaultInitialRadius = atof(value);
						break;

					case CFG_PARAM_EXPANSION_RATE:
						entry->m_fExpansionRate = atof(value);
						break;

					case CFG_PARAM_INITIAL_INTENSITY:
						entry->m_nFadeToBlackInitialIntensity = atoi(value);
						break;

					case CFG_PARAM_FADE_TIME:
						entry->m_nFadeToBlackTime = atoi(value);
						break;

					case CFG_PARAM_FADE_AMOUNT:
						entry->m_nFadeToBlackAmount = atoi(value);
						break;

					case CFG_PARAM_INITIAL_ALPHA_INTENSITY:
						entry->m_nFadeAlphaInitialIntensity = atoi(value);
						break;

					case CFG_PARAM_FADE_ALPHA_TIME:
						entry->m_nFadeAlphaTime = atoi(value);
						break;

					case CFG_PARAM_FADE_ALPHA_AMOUNT:
						entry->m_nFadeAlphaAmount = atoi(value);
						break;

					case CFG_PARAM_INITIAL_ANGLE:
						entry->m_nZRotationInitialAngle = atoi(value);
						break;

					case CFG_PARAM_CHANGE_TIME:
						entry->m_nZRotationChangeTime = atoi(value);
						break;

					case CFG_PARAM_ANGLE_CHANGE_AMOUNT:
						entry->m_nZRotationAngleChangeAmount = atoi(value);
						break;

					case CFG_PARAM_INITIAL_Z_RADIUS:
						entry->m_fInitialZRadius = atof(value);
						break;

					case CFG_PARAM_Z_RADIUS_CHANGE_TIME:
						entry->m_nZRadiusChangeTime = atoi(value);
						break;

					case CFG_PARAM_Z_RADIUS_CHANGE_AMOUNT:
						entry->m_fZRadiusChangeAmount = atof(value);
						break;

					case CFG_PARAM_ANIMATION_SPEED:
						entry->m_nAnimationSpeed = atoi(value);
						break;

					case CFG_PARAM_START_ANIMATION_FRAME:
						entry->m_nStartAnimationFrame = atoi(value);
						break;

					case CFG_PARAM_FINAL_ANIMATION_FRAME:
						entry->m_nFinalAnimationFrame = atoi(value);
						break;

					case CFG_PARAM_ROTATION_SPEED:
						entry->m_nRotationSpeed = atoi(value);
						break;

					case CFG_PARAM_GRAVITATIONAL_ACCELERATION:
						entry->m_fGravitationalAcceleration = atof(value);
						break;

					case CFG_PARAM_FRICTION_DECCELERATION:
						entry->m_nFrictionDecceleration = atoi(value);
						break;

					case CFG_PARAM_LIFE_SPAN:
						entry->m_nLifeSpan = atoi(value);
						break;

					case CFG_PARAM_POSITION_RANDOM_ERROR:
						entry->m_fPositionRandomError = atof(value);
						break;

					case CFG_PARAM_VELOCITY_RANDOM_ERROR:
						entry->m_fVelocityRandomError = atof(value);
						break;

					case CFG_PARAM_EXPANSION_RATE_ERROR:
						entry->m_fExpansionRateError = atof(value);
						break;

					case CFG_PARAM_ROTATION_RATE_ERROR:
						entry->m_nRotationRateError = atoi(value);
						break;

					case CFG_PARAM_LIFE_SPAN_ERROR_SHAPE:
						entry->m_nLifeSpanErrorShape = atoi(value);
						break;

					case CFG_PARAM_TRAIL_LENGTH_MULTIPLIER:
						entry->m_fTrailLengthMultiplier = atof(value);
						break;
						
					case CFG_PARAM_STRETCH_VALUE_X:
						entry->m_vecTextureStretch.x = atof(value);
						break;

					case CFG_PARAM_STRETCH_VALUE_Y:
						entry->m_vecTextureStretch.y = atof(value);
						break;

					case CFG_PARAM_WIND_FACTOR:
						entry->m_fWindFactor = atof(value);
						break;

					case CFG_PARAM_PARTICLE_CREATE_RANGE:
						entry->m_fCreateRange = SQR(atof(value));
						break;

					case CFG_PARAM_FLAGS:
						entry->Flags = atoi(value);
						break;
				}
				
				value = strtok(nil, delims);

				param++;

				if ( param > CFG_PARAM_LAST )
					param = CFG_PARAM_FIRST;
		
			} while ( value != nil );
		}
		
		lineEnd++;
		lineStart = lineEnd;
		lineEnd++;
	}

#ifdef VICEEXT_PARTICLE_NAMES
	// Verificacion de la carga: `nuevos` es cuantos de los 5 tipos del mod han
	// entrado y `desconocidos` cualquier nombre sin soporte (0 = fichero servido
	// entero).
	VICEEXT_P4_EMIT(1, odTrace, "P4 kind=partcfg schema=1 gen=1 frame=%u sim=%.4f case=2 id=0 cargados=%d nuevos=%d desconocidos=%d total=%d", (unsigned)CTimer::GetFrameCounter(), CTimer::GetTimeInMilliseconds() * 0.001f, odLoaded, odNew, odUnknown, odTotal);
#endif
}
