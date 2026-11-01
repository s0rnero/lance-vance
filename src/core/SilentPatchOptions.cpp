#include "common.h"
#include "SilentPatchOptions.h"

bool g_bSPOptionMinimalHUD = false;
bool g_bSPOptionSlidingMissionTitle = false;
bool g_bSPOptionSlidingOddJob = false;
bool g_bSPOptionShowPropertyBlips = false;
bool g_bSPOptionDontShrinkRadardisc = false;
bool g_bSPOptionScaleScriptSprites = false;
bool g_bSPOptionShrinkRadardisc = false;
bool g_bSPOptionBlipBilinear = false;
bool g_bSPOptionScriptSpriteBilinear = false;
bool g_bSPOptionPickupColours = false;
int32 g_nSPOptionSpeechDelay = 6000;
int32 g_nSPOptionUnits = -1;

#ifdef VICEEXT_SP_METRIC_UNITS
bool
IsMetricUnits(void)
{
	if (g_nSPOptionUnits == 0)
		return true;
	if (g_nSPOptionUnits == 1)
		return false;
	return true;
}
#endif

void
ReadSilentPatchOptions(void)
{
#ifdef VICEEXT_SP_OPTIONS
	ReadIniIfExists("SilentPatch", "MinimalHUD", &g_bSPOptionMinimalHUD);
	ReadIniIfExists("SilentPatch", "SlidingMissionTitleText", &g_bSPOptionSlidingMissionTitle);
	ReadIniIfExists("SilentPatch", "SlidingOddJobText", &g_bSPOptionSlidingOddJob);
	ReadIniIfExists("SilentPatch", "ShowPropertyBlips", &g_bSPOptionShowPropertyBlips);
	ReadIniIfExists("SilentPatch", "DontShrinkRadardisc", &g_bSPOptionDontShrinkRadardisc);
	ReadIniIfExists("SilentPatch", "ScaleScriptSprites", &g_bSPOptionScaleScriptSprites);
	ReadIniIfExists("SilentPatch", "ShrinkRadardisc", &g_bSPOptionShrinkRadardisc);
	ReadIniIfExists("SilentPatch", "BlipBilinear", &g_bSPOptionBlipBilinear);
	ReadIniIfExists("SilentPatch", "ScriptSpriteBilinear", &g_bSPOptionScriptSpriteBilinear);
	ReadIniIfExists("SilentPatch", "PickupColours", &g_bSPOptionPickupColours);
	ReadIniIfExists("SilentPatch", "SpeechDelayTimer", &g_nSPOptionSpeechDelay);
	ReadIniIfExists("SilentPatch", "Units", &g_nSPOptionUnits);

	if (g_bSPOptionDontShrinkRadardisc)
		g_bSPOptionShrinkRadardisc = false;
	if (g_nSPOptionSpeechDelay < 0)
		g_nSPOptionSpeechDelay = 6000;
	if (g_nSPOptionSpeechDelay > 6000)
		g_nSPOptionSpeechDelay = 6000;
	if (g_nSPOptionUnits < -1)
		g_nSPOptionUnits = -1;
	if (g_nSPOptionUnits > 1)
		g_nSPOptionUnits = 1;
#endif
}
