#include "common.h"

#include "Pools.h"
#include "ModelIndices.h"
#include "Timer.h"
#include "World.h"
#include "ZoneCull.h"
#include "Darkel.h"
#include "DMAudio.h"
#include "CopPed.h"
#include "Wanted.h"
#include "General.h"
#include "Stats.h"
#include "ondemand.h"   // web: ODTRACES -> odtrace.log (canal de traza del port)

int32 CWanted::MaximumWantedLevel = 6;
int32 CWanted::nMaximumWantedLevel = 9600;

// ---------------------------------------------------------------------------
// Sección 2 / P1 «esconderse de la policía»: INSTRUMENTACIÓN de la línea base.
//
// El plan pide, antes de tocar el sistema, cuántos segundos tarda en bajar el
// nivel de búsqueda sin que te vean y qué hacen los CCopPed mientras tanto.
// Vanilla no publica nada de eso, así que aquí se imprime una línea por segundo
// (printf -> consola de la página y `window.__vcLog`, que es de donde tira la
// sonda tools/hidecops-smoke-test.mjs).
//
// IMPORTANTE: esto NO cambia ninguna decisión del motor (solo lee estado).
// Cuando el bloque esté implementado, la misma traza se queda detrás de
// VICEEXT_HIDE_COPS como evidencia de la mecánica nueva.
// ---------------------------------------------------------------------------

// Radio de "ve al jugador": mismo orden de magnitud que el alcance útil de un
// arma corta y bastante más que los 18 m del chequeo de presencia vanilla, para
// distinguir "hay policía cerca" de "el policía te está viendo".
#define WANTED_SIGHT_RADIUS 40.0f

#ifdef VICEEXT_HIDE_COPS
// Vice Extended (v1.0): ritmo de la huida. 5 s de gracia (los policías te
// "recuerdan" y siguen buscando) y luego una estrella cada 15 s sin que nadie
// te vea. Con 2 estrellas: 1 en 20 s. Con 6: 0 en 95 s.
#define VICEEXT_HIDE_GRACE_MS 5000
#define VICEEXT_HIDE_STAR_MS 15000
// Cada cuánto se refresca el barrido de "¿me ve algún policía de la calle?".
// Esa parte recorre TODO el pool de peds (y lanza un ProcessLineOfSight por
// policía a menos de 40 m), y UpdateHiding() corre en CADA frame; el contrato
// es de segundos (5 s de gracia + 15 s por estrella), así que 5 Hz sobra. La
// lista de perseguidores (m_pCops, como mucho 10) sí se mira cada frame.
// Peor caso del filtro: 200 ms de decisión de visibilidad "vieja" — por debajo
// de la tolerancia del verificador (2.5 s).
#define VICEEXT_HIDE_SWEEP_MS 200
// Cuánto tiene que DURAR un avistamiento para contar (histéresis). El barrido va
// a 5 Hz y en la calle el resultado parpadea; por debajo de este umbral el
// destello no corta la búsqueda ni reinicia la racha (el jugador sigue
// escondido). Ver el porqué en UpdateHiding().
#define VICEEXT_HIDE_SEEN_MS 400
// Coseno del ángulo de "te ve" (75° a cada lado del morro del policía).
#define WANTED_SIGHT_COS 0.26f
// Revisión del bloque, impresa UNA vez por sesión (`WANTEDHIDEINIT`): la
// etiqueta `build=` del JS NO cambia cuando sólo se recompone el `.wasm`, así
// que sin esto un log no puede probar qué binario jugó.
// rev 3 = traza de `WANTEDCOP join` limitada a una línea por segundo (`burst=`) y
//         esta línea de prueba de revisión.
// rev 4 = además, barrido de visibilidad a 5 Hz (antes: cada frame).
// rev 5 = las guardas de tiempo de la traza y del barrido **reanclan cuando el reloj
//         del motor retrocede** (al cargar partida: antes la traza se quedaba muda
//         toda la sesión) y `UpdateHiding()` sale si todavía no hay jugador (antes de
//         `FindPlayerCoords()`, que desreferencia sin comprobar).
//         (La cadencia y el sonido del drive-by por arma van aparte: se prueban con
//         `delay=` en las líneas `DRIVEBY shot`.)
// rev 6 = el "te ve" exige ADEMÁS que el policía mire hacia ti (75°) y un
//         avistamiento < 400 ms no corta la búsqueda (histéresis). Antes: a
//         nivel >= 2 la estrella no bajaba nunca en partida (`start`/`seen`
//         encadenados cada 200 ms).
// rev 7 = la regla de esconderse cubre TAMBIÉN el nivel 1 (la última estrella).
//         Medido en la partida del 21/09: de 3 estrellas sólo cayeron las de
//         arriba; la última quedaba en manos de la regla vanilla de nivel 1 (1
//         punto de chaos/s SIEMPRE que no haya policía a menos de 18 m), que no
//         entiende de esconderse: con una patrulla cerca no cae nunca.
#define VICEEXT_HIDE_REV 7
#endif

// Etiqueta de sesión: el dev server comparte UN odtrace.log entre todas las
// pestañas, así que si hay otra sonda (u otro agente) jugando a la vez las
// líneas se mezclan. La sonda pone un número en window.__vcWantedTag y su traza
// lo lleva impreso para poder separarla. Sin EM_ASM (build nativo) es 0.
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
unsigned
WantedTraceTag(void)
{
	return (unsigned)EM_ASM_INT({ return (window.__vcWantedTag | 0); });
}
#else
unsigned
WantedTraceTag(void)
{
	return 0;
}
#endif

// ¿Este policía tiene línea de visión al jugador? Réplica del criterio que ya
// usa CCopPed::CopAI para decidir si dispara (CWorld::ProcessLineOfSight), pero
// con edificios/objetos bloqueando de verdad (en CopPed esa llamada va sin
// chequeo de edificios porque solo decide si apuntar al coche).
static bool
WantedCopSeesPlayer(CPed *cop, const CVector &playerPos)
{
	CVector copPos = cop->GetPosition();
	copPos.z += 1.0f;
	CVector target = playerPos;
	target.z += 0.9f;

	if ((copPos - target).MagnitudeSqr() > sq(WANTED_SIGHT_RADIUS))
		return false;

	// Sección 2 (P1, 5ª partida): además de la línea de visión, el policía tiene
	// que estar MIRANDO hacia el jugador. Sin esto, cualquier patrulla a 40 m de
	// espaldas (o cruzando de lado) contaba como "te ve": en la partida del
	// 20/09 salieron 1.205 avistamientos en 649 s con el jugador escondido, así
	// que el contador de 15 s no llegaba a pagar nunca y la mecánica no hacía
	// nada (0 estrellas bajadas en toda la partida). Vanilla no tiene el
	// concepto de "verte", así que el ángulo es decisión de este bloque y queda
	// aquí escrito: 75° a cada lado del morro.
	CVector toPlayer = target - copPos;
	toPlayer.z = 0.0f;
	float dist2D = toPlayer.Magnitude();
	if (dist2D > 0.001f) {
		toPlayer *= 1.0f / dist2D;
		CVector copFwd = cop->GetForward();
		copFwd.z = 0.0f;
		copFwd.Normalise();
		if (DotProduct(copFwd, toPlayer) < WANTED_SIGHT_COS)
			return false;   // mira hacia otro lado: no te ve
	}

	CColPoint colPoint;
	CEntity *entity = nil;
	bool hit = CWorld::ProcessLineOfSight(copPos, target, colPoint, entity,
		true,   // checkBuildings
		false,  // checkVehicles
		false,  // checkPeds
		true,   // checkObjects
		false,  // checkDummies
		true,   // ignoreSeeThrough
		false);

	if (hit) {
		CPed *player = FindPlayerPed();
		// El propio policía (o su coche) no cuenta como obstáculo: la línea sale
		// de dentro de él cuando va conduciendo.
		if (entity && entity != (CEntity*)player && entity != (CEntity*)player->m_pMyVehicle
			&& entity != (CEntity*)cop && entity != (CEntity*)cop->m_pMyVehicle)
			return false;   // lo tapa un edificio/objeto
	}
	return true;
}

// Cuántos segundos llevas sin que nadie te vea y si hay búsqueda en curso. Con
// el bloque apagado esos campos no existen: la traza informa unseen=0 hiding=0,
// que es justo la línea base (vanilla no tiene ni el concepto).
static unsigned
WantedTraceUnseenFor(CWanted *wanted)
{
#ifdef VICEEXT_HIDE_COPS
	if (wanted->m_nHiddenSince == 0)
		return 0;
	return (CTimer::GetTimeInMilliseconds() - wanted->m_nHiddenSince) / 1000;
#else
	(void)wanted;
	return 0;
#endif
}

static int
WantedTraceHiding(CWanted *wanted)
{
#ifdef VICEEXT_HIDE_COPS
	return wanted->m_bHiding ? 1 : 0;
#else
	(void)wanted;
	return 0;
#endif
}

// Una línea por segundo: nivel, chaos, cuántos policías persiguen, cuántos hay
// en 18 m (el radio que usa el propio Update()), cuántos te ven y qué
// objetivo (m_objective) lleva cada perseguidor. Va por ODTRACES (no printf):
// el stdout de C va "a bloques" y la sonda perdía la mayor parte de las líneas.
static void
WantedTraceState(CWanted *wanted)
{
	// El bloque de 1 s de Update() se ejecuta EN CADA FRAME mientras haya
	// policía a menos de 18 m (m_nLastUpdateTime solo se refresca cuando la vía
	// está despejada), así que aquí se filtra a una línea por segundo.
	//
	// El reloj del motor RETROCEDE al cargar partida (CTimer viene del guardado:
	// medido el 20/09, de t≈4.4M a t≈3.66M). Sin reanclar, `now < lastTrace+1000`
	// se cumple durante minutos y la traza se queda MUDA (fue justo lo que pasó:
	// la sesión cargada no imprimió ni una línea `WANTED tag=`).
	static uint32 lastTrace = 0;
	uint32 now = CTimer::GetTimeInMilliseconds();
	if (now < lastTrace)
		lastTrace = 0;
	if (now < lastTrace + 1000)
		return;
	lastTrace = now;

	CVector playerPos = FindPlayerCoors();
	int pursuing = 0;
	int seeing = 0;
	float nearest = -1.0f;
	char objectives[64];
	int len = 0;

	objectives[0] = '\0';
	for (int i = 0; i < ARRAY_SIZE(wanted->m_pCops); i++) {
		CCopPed *cop = wanted->m_pCops[i];
		if (!cop)
			continue;
		pursuing++;
		if (len < (int)sizeof(objectives) - 4)
			len += snprintf(objectives + len, sizeof(objectives) - len, "%d,", (int)cop->m_objective);
		float dist = (cop->GetPosition() - playerPos).Magnitude();
		if (nearest < 0.0f || dist < nearest)
			nearest = dist;
		if (WantedCopSeesPlayer(cop, playerPos))
			seeing++;
	}

	// `veh`/`spd` no son del sistema de búsqueda: los necesita la sonda para
	// saber si el jugador va a pie (te detienen) o conduciendo (no pueden).
	CVehicle *playerVeh = FindPlayerVehicle();
	char line[240];
	snprintf(line, sizeof(line), "WANTED tag=%u t=%u lvl=%d chaos=%d minlvl=%d cops=%d/%d listed=%d presence18=%d seeing=%d near=%.1f unseen=%u hiding=%d veh=%d spd=%.1f objs=%s",
		WantedTraceTag(), (unsigned)CTimer::GetTimeInMilliseconds(), wanted->GetWantedLevel(), wanted->m_nChaos,
		wanted->m_nMinWantedLevel, (int)wanted->m_CurrentCops, (int)wanted->m_MaxCops, pursuing,
		CWanted::WorkOutPolicePresence(playerPos, 18.0f), seeing, nearest, WantedTraceUnseenFor(wanted), WantedTraceHiding(wanted),
		playerVeh ? 1 : 0, playerVeh ? playerVeh->m_vecMoveSpeed.Magnitude() : 0.0f, objectives);
	ODTRACES(line);
}

void
CWanted::Initialise()
{
	m_nChaos = 0;
	m_nMinChaos = 0;
	m_nLastUpdateTime = 0;
	m_nLastWantedLevelChange = 0;
	m_nLastTimeSuspended = 0;
	m_CurrentCops = 0;
	m_MaxCops = 0;
	m_MaximumLawEnforcerVehicles = 0;
	m_RoadblockDensity = 0;
#ifdef VICEEXT_HIDE_COPS
	// Vice Extended (P1): sin búsqueda en curso. m_nLastSeenTime arranca en el
	// instante actual para que la gracia de 5 s no dispare sola al empezar.
	m_vecLastKnownPos = CVector(0.0f, 0.0f, 0.0f);
	m_nLastSeenTime = CTimer::GetTimeInMilliseconds();
	m_nHiddenSince = 0;
	m_nLastStarDrop = 0;
	m_bHiding = false;
	// Una línea por sesión: deja en el log qué revisión y con qué constantes
	// corrió de verdad (el `.wasm` se recompone sin tocar la etiqueta del JS).
	{
		char line[128];
		snprintf(line, sizeof(line), "WANTEDHIDEINIT tag=%u rev=%d star_ms=%d grace_ms=%d sight_m=%d sweep_ms=%d seen_ms=%d sight_dot=%.2f t=%u",
			WantedTraceTag(), VICEEXT_HIDE_REV, VICEEXT_HIDE_STAR_MS, VICEEXT_HIDE_GRACE_MS, (int)WANTED_SIGHT_RADIUS,
			VICEEXT_HIDE_SWEEP_MS, VICEEXT_HIDE_SEEN_MS, WANTED_SIGHT_COS, (unsigned)CTimer::GetTimeInMilliseconds());
		ODTRACES(line);
	}
#endif
	m_bIgnoredByCops = false;
	m_bIgnoredByEveryone = false;
	m_bSwatRequired = false;
	m_bFbiRequired = false;
	m_bArmyRequired = false;
	m_fCrimeSensitivity = 1.0f;
	m_nWantedLevel = 0;
	m_nMinWantedLevel = 0;
	m_CopsBeatingSuspect = 0;

	for (int i = 0; i < ARRAY_SIZE(m_pCops); i++)
		m_pCops[i] = nil;

	ClearQdCrimes();
}

bool
CWanted::AreMiamiViceRequired()
{
	return m_nWantedLevel >= 3;
}

bool
CWanted::AreSwatRequired()
{
	return m_nWantedLevel == 4 || m_bSwatRequired;
}

bool
CWanted::AreFbiRequired()
{
	return m_nWantedLevel == 5 || m_bFbiRequired;
}

bool
CWanted::AreArmyRequired()
{
	return m_nWantedLevel == 6 || m_bArmyRequired;
}

int32
CWanted::NumOfHelisRequired()
{
	if (m_bIgnoredByCops || m_bIgnoredByEveryone)
		return 0;

	switch (m_nWantedLevel) {
	case 3:
	case 4:
		return 1;
	case 5:
	case 6:
		return 1;
	default:
		return 0;
	}
}

void
CWanted::SetWantedLevel(int32 level)
{
	if (level > MaximumWantedLevel)
		level = MaximumWantedLevel;

	ClearQdCrimes();
	switch (level) {
	case 0:
		m_nChaos = 0;
		break;
	case 1:
		m_nChaos = 70;
		break;
	case 2:
		m_nChaos = 200;
		break;
	case 3:
		m_nChaos = 570;
		break;
	case 4:
		m_nChaos = 1220;
		break;
	case 5:
		m_nChaos = 2420;
		break;
	case 6:
		m_nChaos = 4820;
		break;
	default:
		break;
	}
	UpdateWantedLevel();
}

void
CWanted::SetWantedLevelNoDrop(int32 level)
{
	if (m_nWantedLevel < m_nMinWantedLevel)
		SetWantedLevel(m_nMinWantedLevel);

	if (level > m_nWantedLevel)
		SetWantedLevel(level);
}

void
CWanted::CheatWantedLevel(int32 level)
{
	SetWantedLevel(level);
	UpdateWantedLevel();
}

void
CWanted::SetMaximumWantedLevel(int32 level)
{
	switch(level){
	case 0:
		nMaximumWantedLevel = 0;
		MaximumWantedLevel = 0;
		break;
	case 1:
		nMaximumWantedLevel = 115;
		MaximumWantedLevel = 1;
		break;
	case 2:
		nMaximumWantedLevel = 365;
		MaximumWantedLevel = 2;
		break;
	case 3:
		nMaximumWantedLevel = 875;
		MaximumWantedLevel = 3;
		break;
	case 4:
		nMaximumWantedLevel = 1800;
		MaximumWantedLevel = 4;
		break;
	case 5:
		nMaximumWantedLevel = 3600;
		MaximumWantedLevel = 5;
		break;
	case 6:
		nMaximumWantedLevel = 7200;
		MaximumWantedLevel = 6;
		break;
	}
}

void
CWanted::RegisterCrime(eCrimeType type, const CVector &coors, uint32 id, bool policeDoesntCare)
{
	AddCrimeToQ(type, id, coors, false, policeDoesntCare);
}

void
CWanted::RegisterCrime_Immediately(eCrimeType type, const CVector &coors, uint32 id, bool policeDoesntCare)
{
#ifdef FIX_SIGNIFICANT_BUGS
	if(!AddCrimeToQ(type, id, coors, true, policeDoesntCare))
#else
	if(!AddCrimeToQ(type, id, coors, false, policeDoesntCare))
#endif
		ReportCrimeNow(type, coors, policeDoesntCare);
}

void
CWanted::ClearQdCrimes()
{
	for (int i = 0; i < 16; i++)
		m_aCrimes[i].m_nType = CRIME_NONE;
}

// returns whether the crime had been reported already
bool
CWanted::AddCrimeToQ(eCrimeType type, int32 id, const CVector &coors, bool reported, bool policeDoesntCare)
{
	int i;

	for(i = 0; i < 16; i++)
		if(m_aCrimes[i].m_nType == type && m_aCrimes[i].m_nId == id){
			if(m_aCrimes[i].m_bReported)
				return true;
			if(reported)
				m_aCrimes[i].m_bReported = reported;
			return false;
		}

	for(i = 0; i < 16; i++)
		if(m_aCrimes[i].m_nType == CRIME_NONE)
			break;
	if(i < 16){
		m_aCrimes[i].m_nType = type;
		m_aCrimes[i].m_nId = id;
		m_aCrimes[i].m_vecPosn = coors;
		m_aCrimes[i].m_nTime = CTimer::GetTimeInMilliseconds();
		m_aCrimes[i].m_bReported = reported;
		m_aCrimes[i].m_bPoliceDoesntCare = policeDoesntCare;
	}
	return false;
}

void
CWanted::ReportCrimeNow(eCrimeType type, const CVector &coors, bool policeDoesntCare)
{
	float sensitivity, chaos;
	int wantedLevelDrop;

	if(CDarkel::FrenzyOnGoing())
		sensitivity = m_fCrimeSensitivity*0.3f;
	else
		sensitivity = m_fCrimeSensitivity;

	wantedLevelDrop = Min(CCullZones::GetWantedLevelDrop(), 100);

	chaos = (1.0f - wantedLevelDrop/100.0f) * sensitivity;
	if (policeDoesntCare)
		chaos *= 0.333f;
	switch(type){
	case CRIME_POSSESSION_GUN:
		break;
	case CRIME_HIT_PED:
		m_nChaos += 5.0f*chaos;
		break;
	case CRIME_HIT_COP:
		m_nChaos += 45.0f*chaos;
		break;
	case CRIME_SHOOT_PED:
		m_nChaos += 30.0f*chaos;
		break;
	case CRIME_SHOOT_COP:
		m_nChaos += 80.0f*chaos;
		break;
	case CRIME_STEAL_CAR:
		m_nChaos += 15.0f*chaos;
		break;
	case CRIME_RUN_REDLIGHT:
		m_nChaos += 10.0f*chaos;
		break;
	case CRIME_RECKLESS_DRIVING:
		m_nChaos += 5.0f*chaos;
		break;
	case CRIME_SPEEDING:
		m_nChaos += 5.0f*chaos;
		break;
	case CRIME_RUNOVER_PED:
		m_nChaos += 18.0f*chaos;
		break;
	case CRIME_RUNOVER_COP:
		m_nChaos += 80.0f*chaos;
		break;
	case CRIME_SHOOT_HELI:
		m_nChaos += 400.0f*chaos;
		break;
	case CRIME_PED_BURNED:
		m_nChaos += 20.0f*chaos;
		break;
	case CRIME_COP_BURNED:
		m_nChaos += 80.0f*chaos;
		break;
	case CRIME_VEHICLE_BURNED:
		m_nChaos += 20.0f*chaos;
		break;
	case CRIME_DESTROYED_CESSNA:
		m_nChaos += 500.0f*chaos;
		break;
	case CRIME_EXPLOSION:
		m_nChaos += 25.0f * chaos;
		break;
	case CRIME_HIT_PED_NASTYWEAPON:
		m_nChaos += 35.0f * chaos;
		break;
	case CRIME_HIT_COP_NASTYWEAPON:
		m_nChaos += 100.0f * chaos;
		break;
	default:
	//	Error("Undefined crime type, RegisterCrime, Crime.cpp");	// different file for some reason
		Error("Undefined crime type, RegisterCrime, Wanted.cpp");
	}
	m_nChaos = Max(m_nChaos, m_nMinChaos);
	DMAudio.ReportCrime(type, coors);
	UpdateWantedLevel();
}

void
CWanted::UpdateWantedLevel()
{
	int32 CurrWantedLevel = m_nWantedLevel;

	if (m_nChaos > nMaximumWantedLevel)
		m_nChaos = nMaximumWantedLevel;

	if (m_nChaos >= 0 && m_nChaos < 50) {
		if (m_nWantedLevel == 1)
			++CStats::WantedStarsEvaded;
		m_nWantedLevel = 0;
		m_MaximumLawEnforcerVehicles = 0;
		m_MaxCops = 0;
		m_RoadblockDensity = 0;
	} else if (m_nChaos >= 50 && m_nChaos < 180) {
		CStats::WantedStarsAttained += 1 - m_nWantedLevel;
		m_nWantedLevel = 1;
		m_MaximumLawEnforcerVehicles = 1;
		m_MaxCops = 1;
		m_RoadblockDensity = 0;
	} else if (m_nChaos >= 180 && m_nChaos < 550) {
		CStats::WantedStarsAttained += 2 - m_nWantedLevel;
		m_nWantedLevel = 2;
		m_MaximumLawEnforcerVehicles = 2;
		m_MaxCops = 3;
		m_RoadblockDensity = 0;
	} else if (m_nChaos >= 550 && m_nChaos < 1200) {
		CStats::WantedStarsAttained += 3 - m_nWantedLevel;
		m_nWantedLevel = 3;
		m_MaximumLawEnforcerVehicles = 2;
		m_MaxCops = 4;
		m_RoadblockDensity = 12;
	} else if (m_nChaos >= 1200 && m_nChaos < 2400) {
		CStats::WantedStarsAttained += 4 - m_nWantedLevel;
		m_nWantedLevel = 4;
		m_MaximumLawEnforcerVehicles = 2;
		m_MaxCops = 6;
		m_RoadblockDensity = 18;
	} else if (m_nChaos >= 2400 && m_nChaos < 4800) {
		CStats::WantedStarsAttained += 5 - m_nWantedLevel;
		m_nWantedLevel = 5;
		m_MaximumLawEnforcerVehicles = 3;
		m_MaxCops = 8;
		m_RoadblockDensity = 24;
	} else if (m_nChaos >= 4800) {
		CStats::WantedStarsAttained += 6 - m_nWantedLevel;
		m_nWantedLevel = 6;
		m_MaximumLawEnforcerVehicles = 3;
		m_MaxCops = 10;
		m_RoadblockDensity = 30;
	}

	if (CurrWantedLevel != m_nWantedLevel) {
		// Sección 2 / P1: traza de línea base (el momento exacto del cambio).
		char line[96];
		snprintf(line, sizeof(line), "WANTEDCHANGE tag=%u %d->%d chaos=%d t=%u", WantedTraceTag(), CurrWantedLevel,
			m_nWantedLevel, m_nChaos, (unsigned)CTimer::GetTimeInMilliseconds());
		ODTRACES(line);
		m_nLastWantedLevelChange = CTimer::GetTimeInMilliseconds();
	}
}

int32
CWanted::WorkOutPolicePresence(CVector posn, float radius)
{
	int i;
	CPed *ped;
	CVehicle *vehicle;
	int numPolice = 0;

	i = CPools::GetPedPool()->GetSize();
	while(--i >= 0){
		ped = CPools::GetPedPool()->GetSlot(i);
		if(ped &&
		   IsPolicePedModel(ped->GetModelIndex()) &&
		   (posn - ped->GetPosition()).Magnitude() < radius)
			numPolice++;
	}

	i = CPools::GetVehiclePool()->GetSize();
	while(--i >= 0){
		vehicle = CPools::GetVehiclePool()->GetSlot(i);
		if(vehicle &&
		   vehicle->bIsLawEnforcer &&
		   IsPoliceVehicleModel(vehicle->GetModelIndex()) &&
		   vehicle != FindPlayerVehicle() &&
		   vehicle->GetStatus() != STATUS_ABANDONED && vehicle->GetStatus() != STATUS_WRECKED &&
		   (posn - vehicle->GetPosition()).Magnitude() < radius)
			numPolice++;
	}

	return numPolice;
}

#ifdef VICEEXT_HIDE_COPS
// ---------------------------------------------------------------------------
// Vice Extended (v1.0 "Changed wanted system", sección 2 / P1): esconderse de
// la policía.
//
// Contrato (medido antes en la línea base, ver el plan de mecánicas):
//  - "verte" = estar a menos de 40 m CON línea de visión libre. Vanilla no
//    distinguía verte de estar cerca (solo miraba 18 m).
//  - nivel <= 1 no se toca: ahí vanilla ya baja chaos 1/s si no hay policía a
//    18 m, y duplicar la regla daría prisas raras.
//  - nivel >= 2: no baja mientras alguien te vea; baja UNA estrella cada 15 s
//    sin que nadie te vea, después de 5 s de gracia. En vanilla, a partir de 2
//    estrellas el nivel no bajaba NUNCA (medido: chaos clavado 120 s).
//  - mientras dura la búsqueda, los perseguidores a pie van a la última
//    posición conocida (CCopPed::CopAI) en vez de a tu posición viva.
//
// Todo esto es solo tiempo + lectura de estado: no toca las decisiones de
// disparo/arresto de la policía.
// ---------------------------------------------------------------------------
bool
CWanted::AnyCopSeesPlayer(const CVector &playerPos, bool fullSweep)
{
	// 1) Los que te persiguen: son los que importan (y son pocos).
	for (int i = 0; i < ARRAY_SIZE(m_pCops); i++) {
		if (m_pCops[i] && WantedCopSeesPlayer(m_pCops[i], playerPos))
			return true;
	}

	// 2) Cualquier policía de la calle que mire hacia ti (patrulla, helicóptero…).
	// Esta parte recorre el pool entero: solo se refresca cada
	// VICEEXT_HIDE_SWEEP_MS (si no, entre barrido y barrido se responde con la
	// lista de perseguidores, que es la que decide si te están viendo de verdad).
	if (!fullSweep)
		return false;
	int i = CPools::GetPedPool()->GetSize();
	while (--i >= 0) {
		CPed *ped = CPools::GetPedPool()->GetSlot(i);
		if (ped && IsPolicePedModel(ped->GetModelIndex()) && WantedCopSeesPlayer(ped, playerPos))
			return true;
	}
	return false;
}

void
CWanted::UpdateHiding(void)
{
	// Salvaguarda de P7 (auditoría a nivel 3): `FindPlayerCoors()` desreferencia al
	// jugador SIN comprobar que exista (`PlayerInfo.cpp`: `ped->InVehicle()`), y
	// UpdateHiding() corre en cada frame, en cualquier estado del juego (menú de
	// detención/muerte, transición de misión). Si no hay jugador no hay nada que
	// esconder. Vanilla llama a lo mismo, pero sólo dentro del bloque de 1 s y con
	// nivel <= 1: aquí es la primera línea del frame, así que se comprueba.
	if (FindPlayerPed() == nil)
		return;
	// Sección 2 (P1, 5ª partida): HISTÉRESIS. `static` a propósito, como el
	// temporizador del barrido (no se le añaden campos a `CWanted`).
	static uint32 seenSince = 0;
	uint32 now = CTimer::GetTimeInMilliseconds();
	if (now < seenSince)
		seenSince = 0;   // el reloj del motor retrocede al cargar partida
	CVector playerPos = FindPlayerCoors();

	// Nivel 0: nada que buscar.
	// Sección 2 (P1, 5ª partida): el nivel 1 TAMBIÉN entra aquí. Antes se dejaba
	// a la regla vanilla (sin policía a menos de 18 m), que no sabe de esconderse:
	// en la partida del 21/09 las estrellas bajaban de 3 a 1 y la última no caía
	// nunca. La gracia y el temporizador son los mismos (5 s + 15 s sin que nadie
	// te vea); la vía vanilla sigue viva y puede bajarla antes si no hay nadie
	// cerca.
	if (m_nWantedLevel == 0) {
		seenSince = 0;
		if (m_bHiding) {
			char line[64];
			snprintf(line, sizeof(line), "WANTEDHIDE end tag=%u t=%u", WantedTraceTag(), (unsigned)now);
			ODTRACES(line);
		}
		m_bHiding = false;
		m_nHiddenSince = 0;
		m_nLastSeenTime = now;
		m_vecLastKnownPos = playerPos;
		return;
	}

	// El barrido del pool (paso 2 de AnyCopSeesPlayer) va a 5 Hz; los
	// perseguidores, cada frame. El temporizador es un `static` a propósito: no
	// toca `CWanted` (la estructura se copia ENTERA en los guardados de misión,
	// así que no se le añaden campos por un detalle de coste).
	static uint32 lastSightSweep = 0;
	// Misma trampa que en la traza: el reloj retrocede al cargar partida y la
	// resta daría un valor enorme (barrido completo cada frame). Reanclar.
	if (now < lastSightSweep)
		lastSightSweep = 0;
	bool fullSweep = (now - lastSightSweep) >= VICEEXT_HIDE_SWEEP_MS;
	if (fullSweep)
		lastSightSweep = now;

	bool seen = AnyCopSeesPlayer(playerPos, fullSweep);
	if (seen) {
		if (seenSince == 0)
			seenSince = now;
	} else {
		seenSince = 0;
	}

	// Sección 2 (P1, 5ª partida): un destello de < VICEEXT_HIDE_SEEN_MS no corta
	// la búsqueda. En la partida del 20/09 el barrido (5 Hz) declaraba "te ve" y
	// "no te ve" alternos con un policía en la calle: 1.205 `seen` encadenados,
	// racha máxima 9 s de los 15 que hacen falta, 0 estrellas bajadas. Con esto,
	// para reiniciar la racha tiene que verte de forma sostenida.
	if (seen && now - seenSince >= VICEEXT_HIDE_SEEN_MS) {
		// Te ven: se refresca la última posición conocida y se corta la búsqueda.
		if (m_bHiding) {
			char line[96];
			snprintf(line, sizeof(line), "WANTEDHIDE seen tag=%u d=%u t=%u", WantedTraceTag(),
				(unsigned)(now - seenSince), (unsigned)now);
			ODTRACES(line);
			m_bHiding = false;
		}
		m_vecLastKnownPos = playerPos;
		m_nLastSeenTime = now;
		m_nHiddenSince = 0;
		return;
	}

	if (m_nHiddenSince == 0) {
		m_nHiddenSince = now;
		m_nLastStarDrop = now;
		m_bHiding = true;
		char line[128];
		snprintf(line, sizeof(line), "WANTEDHIDE start tag=%u lvl=%d chaos=%d t=%u", WantedTraceTag(), m_nWantedLevel, m_nChaos, (unsigned)now);
		ODTRACES(line);
		return;
	}

	if (now - m_nLastSeenTime < VICEEXT_HIDE_GRACE_MS)
		return;   // te acaban de ver: siguen buscando donde te vieron

	if (now - m_nLastStarDrop >= VICEEXT_HIDE_STAR_MS) {
		m_nLastStarDrop = now;
		char line[128];
		snprintf(line, sizeof(line), "WANTEDHIDE drop tag=%u lvl=%d->%d chaos=%d t=%u", WantedTraceTag(), m_nWantedLevel,
			m_nWantedLevel - 1, m_nChaos, (unsigned)now);
		ODTRACES(line);
		// SetWantedLevel deja el chaos canónico del nivel nuevo y limpia la cola
		// de crímenes: es la bajada limpia de "se han cansado de buscarte".
		SetWantedLevel(m_nWantedLevel - 1);
		if (m_nWantedLevel == 0)
			m_bHiding = false;   // CopAI ya devuelve a cada perseguidor a la calle
	}
}
#endif

void
CWanted::Update(void)
{
#ifdef VICEEXT_HIDE_COPS
	UpdateHiding();   // cada frame: los temporizadores son de milisegundos
#endif
	if (CTimer::GetTimeInMilliseconds() > m_nLastTimeSuspended + 20000) {
		m_nMinChaos = 0;
		m_nMinWantedLevel = 0;
	}
	if (CTimer::GetTimeInMilliseconds() - m_nLastUpdateTime > 1000) {
		if (m_nWantedLevel > 1) {
			m_nLastUpdateTime = CTimer::GetTimeInMilliseconds();
		} else {
			float radius = 18.0f;
			CVector playerPos = FindPlayerCoors();
			if (WorkOutPolicePresence(playerPos, radius) == 0) {
				m_nLastUpdateTime = CTimer::GetTimeInMilliseconds();
				m_nChaos = Max(0, m_nChaos - 1);
				UpdateWantedLevel();
			}
		}
		// Sección 2 / P1: instrumentación de línea base (una línea por segundo,
		// con cualquier nivel: es lo que se quiere medir).
		WantedTraceState(this);
		UpdateCrimesQ();
		bool orderMessedUp = false;
		int currCopNum = 0;
		bool foundEmptySlot = false;
		for (int i = 0; i < ARRAY_SIZE(m_pCops); i++) {
			if (m_pCops[i]) {
				++currCopNum;
				if (foundEmptySlot)
					orderMessedUp = true;
			} else {
				foundEmptySlot = true;
			}
		}
		if (currCopNum != m_CurrentCops) {
			printf("CopPursuit total messed up: re-setting\n");
			m_CurrentCops = currCopNum;
		}
		if (orderMessedUp) {
			printf("CopPursuit pointer list messed up: re-sorting\n");
			bool fixed = true;
			for (int i = 0; i < ARRAY_SIZE(m_pCops); i++) {
				if (!m_pCops[i]) {
					for (int j = i; j < ARRAY_SIZE(m_pCops); j++) {
						if (m_pCops[j]) {
							m_pCops[i] = m_pCops[j];
							m_pCops[j] = nil;
							fixed = false;
							break;
						}
					}
					if (fixed)
						break;
				}
			}
		}
	}
}

void
CWanted::ResetPolicePursuit(void)
{
	// Sección 2 / P1: en vanilla esta es la ÚNICA vía de escape sin morir o ser
	// detenido: la llama el respray de un taller (Garages.cpp) y CWanted::Reset().
	{
		char line[96];
		snprintf(line, sizeof(line), "WANTEDPURGE tag=%u pursuit-cleared cops=%d lvl=%d chaos=%d t=%u", WantedTraceTag(),
			(int)m_CurrentCops, m_nWantedLevel, m_nChaos, (unsigned)CTimer::GetTimeInMilliseconds());
		ODTRACES(line);
	}
	for(int i = 0; i < ARRAY_SIZE(m_pCops); i++) {
		CCopPed *cop = m_pCops[i];
		if (!cop)
			continue;

		cop->m_bIsInPursuit = false;
		cop->m_objective = OBJECTIVE_NONE;
		cop->m_prevObjective = OBJECTIVE_NONE;
		cop->m_nLastPedState = PED_NONE;
		if (!cop->DyingOrDead()) {
			cop->SetWanderPath(CGeneral::GetRandomNumberInRange(0.0f, 8.0f));
		}
		m_pCops[i] = nil;
	}
	m_CurrentCops = 0;
}

void
CWanted::Reset(void)
{
	ResetPolicePursuit();
	Initialise();
}

void
CWanted::UpdateCrimesQ(void)
{
	for(int i = 0; i < ARRAY_SIZE(m_aCrimes); i++) {

		CCrimeBeingQd &crime = m_aCrimes[i];
		if (crime.m_nType != CRIME_NONE) {
			if (CTimer::GetTimeInMilliseconds() > crime.m_nTime + 500 && !crime.m_bReported) {
				ReportCrimeNow(crime.m_nType, crime.m_vecPosn, crime.m_bPoliceDoesntCare);
				crime.m_bReported = true;
			}
			if (CTimer::GetTimeInMilliseconds() > crime.m_nTime + 10000)
				crime.m_nType = CRIME_NONE;
		}
	}
}

void
CWanted::Suspend(void)
{
	// Sección 2 / P1: Suspend() = garaje/guardado de misión: guarda el nivel
	// mínimo y pone el nivel a 0.
	{
		char line[96];
		snprintf(line, sizeof(line), "WANTEDSUSPEND tag=%u lvl=%d chaos=%d t=%u", WantedTraceTag(), m_nWantedLevel, m_nChaos,
			(unsigned)CTimer::GetTimeInMilliseconds());
		ODTRACES(line);
	}
	CStats::WantedStarsEvaded += m_nWantedLevel;
	m_nMinChaos = m_nChaos;
	m_nMinWantedLevel = m_nWantedLevel;
	m_nLastTimeSuspended = CTimer::GetTimeInMilliseconds();
	m_nChaos = 0;
	m_nWantedLevel = 0;
	ResetPolicePursuit();
}
