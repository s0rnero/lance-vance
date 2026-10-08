#include "common.h"
#include "PedArbiter.h"
#include "ondemand.h"

#ifdef VICEEXT_PEDARBITER
static ePedLane ms_aOwner[NUM_PEDCAPS];
static void (*ms_apfnRelease[NUM_PEDLANES])(ePedCap, void *);
static void *ms_audRelease[NUM_PEDLANES];
#endif

ePedLane
ViceExtPedOwner(ePedCap c)
{
#ifdef VICEEXT_PEDARBITER
	if (c < 0 || c >= NUM_PEDCAPS)
		return PEDLANE_MOTOR;
	return ms_aOwner[c];
#else
	(void)c;
	return PEDLANE_MOTOR;
#endif
}

bool
ViceExtPedOwns(ePedLane l, ePedCap c)
{
	return ViceExtPedOwner(c) == l;
}

bool
ViceExtPedClaim(ePedLane l, ePedCap c)
{
#ifdef VICEEXT_PEDARBITER
	if (c < 0 || c >= NUM_PEDCAPS || l <= PEDLANE_MOTOR || l >= NUM_PEDLANES)
		return false;
	ePedLane odAntes = ms_aOwner[c];
	if (odAntes == l)
		return true;
	if (odAntes != PEDLANE_MOTOR) {
#ifdef __EMSCRIPTEN__
		{
			char t[128];
			snprintf(t, sizeof t, "PEDCLAIM cap=%d carril=%d de=%d ok=0", (int)c, (int)l, (int)odAntes);
			ODTRACES(t);
		}
#endif
		return false;
	}
	ms_aOwner[c] = l;
#ifdef __EMSCRIPTEN__
	{
		char t[128];
		snprintf(t, sizeof t, "PEDCLAIM cap=%d carril=%d de=%d a=%d ok=1",
			(int)c, (int)l, (int)odAntes, (int)l);
		ODTRACES(t);
	}
#endif
	if (odAntes >= 0 && odAntes < NUM_PEDLANES && ms_apfnRelease[odAntes])
		ms_apfnRelease[odAntes](c, ms_audRelease[odAntes]);
	return true;
#else
	(void)l;
	(void)c;
	return false;
#endif
}

void
ViceExtPedRelease(ePedLane l, ePedCap c)
{
#ifdef VICEEXT_PEDARBITER
	if (c < 0 || c >= NUM_PEDCAPS || l <= PEDLANE_MOTOR || l >= NUM_PEDLANES)
		return;
	if (ms_aOwner[c] != l)
		return;
	ms_aOwner[c] = PEDLANE_MOTOR;
#ifdef __EMSCRIPTEN__
	{
		char t[128];
		snprintf(t, sizeof t, "PEDCLAIM cap=%d carril=%d de=%d a=%d ok=1 release",
			(int)c, (int)l, (int)l, (int)PEDLANE_MOTOR);
		ODTRACES(t);
	}
#endif
	if (ms_apfnRelease[l])
		ms_apfnRelease[l](c, ms_audRelease[l]);
#endif
}

void
ViceExtPedOnRelease(ePedLane l, void (*fn)(ePedCap, void *), void *ud)
{
#ifdef VICEEXT_PEDARBITER
	if (l <= PEDLANE_MOTOR || l >= NUM_PEDLANES)
		return;
	ms_apfnRelease[l] = fn;
	ms_audRelease[l] = ud;
#else
	(void)l;
	(void)fn;
	(void)ud;
#endif
}

void
ViceExtPedResetAll(void)
{
#ifdef VICEEXT_PEDARBITER
	for (int i = 0; i < NUM_PEDCAPS; i++)
		ms_aOwner[i] = PEDLANE_MOTOR;
	for (int i = 0; i < NUM_PEDLANES; i++) {
		ms_apfnRelease[i] = nil;
		ms_audRelease[i] = nil;
	}
#endif
}
