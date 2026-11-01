#pragma once

extern bool g_bWSPOptionCarSpeedDependantFOV;
extern bool g_bWSPOptionDontTouchFOV;
extern bool g_bWSPOptionEmergencyFOV;
extern bool g_bWSPOptionSkipCameraCollision;
extern bool g_bWSPOptionInverseFOVCollision;
extern bool g_bWSPOptionCutsceneFOV;
extern bool g_bWSPOptionWheelCamFOV;
extern float g_fWSPOptionHudWidthScale;
extern float g_fWSPOptionHudHeightScale;
extern float g_fWSPOptionRadarWidthScale;
extern float g_fWSPOptionRadarWidth;
extern bool g_bWSPOptionRadarSizeSet;
extern float g_fWSPOptionRadarHeight;
extern float g_fWSPOptionSubtitlesScale;
extern int32 g_nWSPOptionHudAspectRatio;
extern int32 g_nWSPOptionMenuAspectRatio;
extern char g_szWSPOptionHudAspectRatio[16];
extern char g_szWSPOptionMenuAspectRatio[16];
extern bool g_bWSPOptionCrosshairAspect;
extern bool g_bWSPOptionIVRadarScaling;
extern int32 g_nWSPOptionTextShadowMode;
extern bool g_bWSPOptionSmallerTextShadows;
extern bool g_bWSPOptionSmallerVehicleCorona;
extern int32_t g_bWSPOptionNoLightSquare;
extern bool g_bWSPOptionBrightLightsOnlyFar;
extern bool g_bWSPOptionSmartCutsceneBorders;
extern bool g_bWSPOptionNoCutsceneBorderAnimation;
extern bool g_bWSPOptionParticleAspect;
extern bool g_bWSPOptionFadeScreenRect;
extern bool g_bWSPOptionVCSCamShake;
extern bool g_bWSPOptionMapAnchor;
extern char g_szWSPOptionForceAspectRatio[32];
extern char g_szWSPOptionFrontendAspectRatio[32];

void ReadWidescreenFixOptions();
