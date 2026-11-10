#include "common.h"

#include "Timer.h"
#include "Pad.h"
#include "Clock.h"
#include "Stats.h"
#include "VarConsole.h"
#include "ondemand.h"

_TODO("gbFastTime");
bool gbFastTime;

uint8  CClock::ms_nGameClockHours;
uint8  CClock::ms_nGameClockMinutes;
uint16 CClock::ms_nGameClockSeconds;
uint8  CClock::ms_Stored_nGameClockHours;
uint8  CClock::ms_Stored_nGameClockMinutes;
uint16 CClock::ms_Stored_nGameClockSeconds;
uint32 CClock::ms_nMillisecondsPerGameMinute;
uint32  CClock::ms_nLastClockTick;
bool   CClock::ms_bClockHasBeenStored;

#ifdef VICEEXT_CLOCK_CATCHUP
// Acumulador de milisegundos reales todavia no descontados como minutos de
// juego. Se cuenta con el timeline NO recortado de CTimer, porque el recortado
// pierde todo lo que dure un frame por encima de 60 ms (Timer.cpp:163), que es
// exactamente lo que valen los hitches de carga en WASM.
static uint32 odClockCarryMs;
static uint32 odClockLastNonClipped;
#ifdef __EMSCRIPTEN__
static uint32 odClockSimMinutes;
static uint32 odClockBaseNonClipped;
static uint32 odClockNextReport;
#endif
#endif

#ifndef MASTER
bool gbFreezeTime;
#endif

void
CClock::Initialise(uint32 scale)
{
	debug("Initialising CClock...\n");
	ms_nGameClockHours = 12;
	ms_nGameClockMinutes = 0;
	ms_nGameClockSeconds = 0;
	ms_nMillisecondsPerGameMinute = scale;
	ms_nLastClockTick = CTimer::GetTimeInMilliseconds();
#ifdef VICEEXT_CLOCK_CATCHUP
	odClockCarryMs = 0;
	odClockLastNonClipped = CTimer::GetTimeInMillisecondsNonClipped();
#ifdef __EMSCRIPTEN__
	odClockSimMinutes = 0;
	odClockBaseNonClipped = odClockLastNonClipped;
	odClockNextReport = 0u;
#endif
#endif
	ms_bClockHasBeenStored = false;
	debug("CClock ready\n");
#ifndef MASTER
	VarConsole.Add("Time (hour of day)", &ms_nGameClockHours, 1, 0, 23, true);
	VarConsole.Add("Freeze time", &gbFreezeTime, true);
#endif
}

void
CClock::Update(void)
{
	if(CPad::GetPad(1)->GetRightShoulder1())
	{
		ms_nGameClockMinutes += 8;
		ms_nLastClockTick = CTimer::GetTimeInMilliseconds();
#ifdef VICEEXT_CLOCK_CATCHUP
		odClockLastNonClipped = CTimer::GetTimeInMillisecondsNonClipped();
		odClockCarryMs = 0;
#endif
		
		if(ms_nGameClockMinutes >= 60)
		{
			ms_nGameClockHours++;
			ms_nGameClockMinutes = 0;
			if(ms_nGameClockHours >= 24)
				ms_nGameClockHours = 0;
		}
		
	}
#ifndef MASTER
	else if (gbFreezeTime) {
		ms_nLastClockTick = CTimer::GetTimeInMilliseconds();
#ifdef VICEEXT_CLOCK_CATCHUP
		odClockLastNonClipped = CTimer::GetTimeInMillisecondsNonClipped();
		odClockCarryMs = 0;
#endif
	}
#endif
#ifdef VICEEXT_CLOCK_CATCHUP
	else {
		uint32 odNow = CTimer::GetTimeInMillisecondsNonClipped();
		odClockCarryMs += odNow - odClockLastNonClipped;
		odClockLastNonClipped = odNow;
		// Antes se descontaba como mucho UN minuto por frame y sobre el timeline
		// recortado, asi que cada hitch dejaba segundos de reloj fuera para
		// siempre. Aqui se descuentan de golpe hasta OD_CLOCK_MAX_CATCHUP
		// minutos; el tope existe para que una pausa larga no teletransporte la
		// hora de la noche a la manana.
		int odSteps = 0;
		while (odSteps < OD_CLOCK_MAX_CATCHUP &&
		       (gbFastTime || odClockCarryMs >= ms_nMillisecondsPerGameMinute)) {
			if (odClockCarryMs >= ms_nMillisecondsPerGameMinute)
				odClockCarryMs -= ms_nMillisecondsPerGameMinute;
			else
				odClockCarryMs = 0;	// gbFastTime: no hay tiempo real que descontar
			odSteps++;
			ms_nGameClockMinutes++;
			if(ms_nGameClockMinutes >= 60)
			{
				ms_nGameClockHours++;
				ms_nGameClockMinutes = 0;
				if(ms_nGameClockHours >= 24)
				{
					CStats::DaysPassed++;
					ms_nGameClockHours = 0;
				}
			}
			if ( gbFastTime )
				break;
		}
		// Lo que quede por encima de un minuto ya no se recupera nunca (tope
		// cumplido): se recorta para que el reloj no persiga el tiempo real.
		if (odClockCarryMs > ms_nMillisecondsPerGameMinute)
			odClockCarryMs = ms_nMillisecondsPerGameMinute - 1u;
		ms_nLastClockTick = CTimer::GetTimeInMilliseconds() - odClockCarryMs;
#ifdef __EMSCRIPTEN__
		odClockSimMinutes += odSteps;
		if (CTimer::GetTimeInMilliseconds() >= odClockNextReport) {
			uint32 odEsperado = odClockSimMinutes * ms_nMillisecondsPerGameMinute;
			char odT[200];
			snprintf(odT, sizeof odT,
				"RELOJ sim=%u real_ms=%u esperado_ms=%u dif_ms=%d hora=%02d:%02d:%02d carry=%u fast=%d",
				odClockSimMinutes, odNow - odClockBaseNonClipped, odEsperado,
				(int)(odNow - odClockBaseNonClipped) - (int)odEsperado,
				ms_nGameClockHours, ms_nGameClockMinutes, ms_nGameClockSeconds,
				odClockCarryMs, gbFastTime ? 1 : 0);
			ODTRACES(odT);
			odClockNextReport = CTimer::GetTimeInMilliseconds() + 5000u;
		}
#endif
	}
#else
	else if(CTimer::GetTimeInMilliseconds() - ms_nLastClockTick > ms_nMillisecondsPerGameMinute || gbFastTime)
	{
		ms_nGameClockMinutes++;
		ms_nLastClockTick += ms_nMillisecondsPerGameMinute;
		
		if ( gbFastTime )
			ms_nLastClockTick = CTimer::GetTimeInMilliseconds();
		
		if(ms_nGameClockMinutes >= 60)
		{
			ms_nGameClockHours++;
			ms_nGameClockMinutes = 0;
			if(ms_nGameClockHours >= 24)
			{
				CStats::DaysPassed++;
				ms_nGameClockHours = 0;
			}
		}
	}
#endif
	ms_nGameClockSeconds = 60 * (CTimer::GetTimeInMilliseconds() - ms_nLastClockTick) / ms_nMillisecondsPerGameMinute;
}

void
CClock::SetGameClock(uint8 h, uint8 m)
{
	while (m >= 60) {
		m -= 60;
		h++;
	}
	ms_nGameClockMinutes = m;
	while (h >= 24)
		h -= 24;
	ms_nGameClockHours = h;
	ms_nGameClockSeconds = 0;
	ms_nLastClockTick = CTimer::GetTimeInMilliseconds();
}

int32
CClock::GetGameClockMinutesUntil(uint8 h, uint8 m)
{
	int32 now, then;
	now = ms_nGameClockHours*60 + ms_nGameClockMinutes;
	then = h*60 + m;
	if(then < now)
		then += 24*60;
	return then-now;
}

bool
CClock::GetIsTimeInRange(uint8 h1, uint8 h2)
{
	if(h1 > h2)
		return ms_nGameClockHours >= h1 || ms_nGameClockHours < h2;
	else
		return ms_nGameClockHours >= h1 && ms_nGameClockHours < h2;
}

void
CClock::StoreClock(void)
{
	ms_Stored_nGameClockHours = ms_nGameClockHours;
	ms_Stored_nGameClockMinutes = ms_nGameClockMinutes;
	ms_Stored_nGameClockSeconds = ms_nGameClockSeconds;
	ms_bClockHasBeenStored = true;
}

void
CClock::RestoreClock(void)
{
	ms_nGameClockHours = ms_Stored_nGameClockHours;
	ms_nGameClockMinutes = ms_Stored_nGameClockMinutes;
	ms_nGameClockSeconds = ms_Stored_nGameClockSeconds;
}
