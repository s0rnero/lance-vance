#pragma once

extern bool g_bSPOptionMinimalHUD;
extern bool g_bSPOptionSlidingMissionTitle;
extern bool g_bSPOptionSlidingOddJob;
extern bool g_bSPOptionShowPropertyBlips;
extern bool g_bSPOptionDontShrinkRadardisc;
extern bool g_bSPOptionScaleScriptSprites;
extern bool g_bSPOptionShrinkRadardisc;
extern bool g_bSPOptionBlipBilinear;
extern bool g_bSPOptionScriptSpriteBilinear;
extern bool g_bSPOptionPickupColours;
extern int32 g_nSPOptionSpeechDelay;
extern int32 g_nSPOptionUnits;

void ReadSilentPatchOptions();
bool IsMetricUnits();
