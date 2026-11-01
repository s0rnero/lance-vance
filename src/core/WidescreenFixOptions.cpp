#include "common.h"
#include "WidescreenFixOptions.h"

bool g_bWSPOptionCarSpeedDependantFOV = false;
bool g_bWSPOptionDontTouchFOV = false;
bool g_bWSPOptionEmergencyFOV = false;
bool g_bWSPOptionSkipCameraCollision = false;
bool g_bWSPOptionInverseFOVCollision = false;
bool g_bWSPOptionCutsceneFOV = false;
bool g_bWSPOptionWheelCamFOV = false;
float g_fWSPOptionHudWidthScale = 1.0f;
float g_fWSPOptionHudHeightScale = 1.0f;
float g_fWSPOptionRadarWidthScale = 1.0f;
float g_fWSPOptionRadarWidth = 94.0f;
bool g_bWSPOptionRadarSizeSet = false;
float g_fWSPOptionRadarHeight = 76.0f;
float g_fWSPOptionSubtitlesScale = 1.0f;
int32 g_nWSPOptionHudAspectRatio = 0;
int32 g_nWSPOptionMenuAspectRatio = 0;
bool g_bWSPOptionCrosshairAspect = false;
bool g_bWSPOptionIVRadarScaling = false;
int32 g_nWSPOptionTextShadowMode = 0;
bool g_bWSPOptionSmallerTextShadows = false;
bool g_bWSPOptionSmallerVehicleCorona = false;
int32_t g_bWSPOptionNoLightSquare = 1;
bool g_bWSPOptionBrightLightsOnlyFar = false;
bool g_bWSPOptionSmartCutsceneBorders = false;
bool g_bWSPOptionNoCutsceneBorderAnimation = false;
bool g_bWSPOptionParticleAspect = false;
bool g_bWSPOptionFadeScreenRect = false;
bool g_bWSPOptionVCSCamShake = false;
bool g_bWSPOptionMapAnchor = false;
char g_szWSPOptionHudAspectRatio[16] = "";
char g_szWSPOptionMenuAspectRatio[16] = "";
char g_szWSPOptionForceAspectRatio[32] = "auto";
char g_szWSPOptionFrontendAspectRatio[32] = "auto";

static int32
AspectConstraintToValue(const char *value)
{
	if (!value)
		return 0;
	if (strncmp(value, "16:9", 4) == 0)
		return 1;
	if (strncmp(value, "21:9", 4) == 0)
		return 2;
	return 0;
}

void
ReadWidescreenFixOptions(void)
{
#ifdef VICEEXT_WFP_OPTIONS
	ReadIniIfExists("WidescreenFix", "CarSpeedDependantFOV", &g_bWSPOptionCarSpeedDependantFOV);
	ReadIniIfExists("WidescreenFix", "DontTouchFOV", &g_bWSPOptionDontTouchFOV);
	ReadIniIfExists("WidescreenFix", "EmergencyFOV", &g_bWSPOptionEmergencyFOV);
	ReadIniIfExists("WidescreenFix", "SkipCameraCollision", &g_bWSPOptionSkipCameraCollision);
	ReadIniIfExists("WidescreenFix", "InverseFOVCollision", &g_bWSPOptionInverseFOVCollision);
	ReadIniIfExists("WidescreenFix", "CutsceneFOV", &g_bWSPOptionCutsceneFOV);
	ReadIniIfExists("WidescreenFix", "WheelCamFOV", &g_bWSPOptionWheelCamFOV);
	ReadIniIfExists("WidescreenFix", "HudWidthScale", &g_fWSPOptionHudWidthScale);
	ReadIniIfExists("WidescreenFix", "HudHeightScale", &g_fWSPOptionHudHeightScale);
	ReadIniIfExists("WidescreenFix", "RadarWidthScale", &g_fWSPOptionRadarWidthScale);
	if (ReadIniIfExists("WidescreenFix", "RadarWidth", &g_fWSPOptionRadarWidth))
		g_bWSPOptionRadarSizeSet = true;
	if (ReadIniIfExists("WidescreenFix", "RadarHeight", &g_fWSPOptionRadarHeight))
		g_bWSPOptionRadarSizeSet = true;
	ReadIniIfExists("WidescreenFix", "SubtitlesScale", &g_fWSPOptionSubtitlesScale);
	ReadIniIfExists("WidescreenFix", "HudAspectRatioConstraint", g_szWSPOptionHudAspectRatio, 16);
	ReadIniIfExists("WidescreenFix", "MenuAspectRatioConstraint", g_szWSPOptionMenuAspectRatio, 16);
	ReadIniIfExists("WidescreenFix", "CrosshairAspect", &g_bWSPOptionCrosshairAspect);
	ReadIniIfExists("WidescreenFix", "IVRadarScaling", &g_bWSPOptionIVRadarScaling);
	ReadIniIfExists("WidescreenFix", "ReplaceTextShadowWithOutline", &g_nWSPOptionTextShadowMode);
	ReadIniIfExists("WidescreenFix", "SmallerTextShadows", &g_bWSPOptionSmallerTextShadows);
	ReadIniIfExists("WidescreenFix", "SmallerVehicleCorona", &g_bWSPOptionSmallerVehicleCorona);
	ReadIniIfExists("WidescreenFix", "NoLightSquare", &g_bWSPOptionNoLightSquare);
	ReadIniIfExists("WidescreenFix", "BrightLightsOnlyFar", &g_bWSPOptionBrightLightsOnlyFar);
	ReadIniIfExists("WidescreenFix", "SmartCutsceneBorders", &g_bWSPOptionSmartCutsceneBorders);
	ReadIniIfExists("WidescreenFix", "NoCutsceneBorderAnimation", &g_bWSPOptionNoCutsceneBorderAnimation);
	ReadIniIfExists("WidescreenFix", "ParticleAspect", &g_bWSPOptionParticleAspect);
	ReadIniIfExists("WidescreenFix", "FadeScreenRect", &g_bWSPOptionFadeScreenRect);
	ReadIniIfExists("WidescreenFix", "VCSCamShake", &g_bWSPOptionVCSCamShake);
	ReadIniIfExists("WidescreenFix", "MapAnchor", &g_bWSPOptionMapAnchor);
	ReadIniIfExists("WidescreenFix", "ForceAspectRatio", g_szWSPOptionForceAspectRatio, 32);
	ReadIniIfExists("WidescreenFix", "FrontendAspectRatio", g_szWSPOptionFrontendAspectRatio, 32);

	if (g_fWSPOptionHudWidthScale <= 0.0f)
		g_fWSPOptionHudWidthScale = 1.0f;
	if (g_fWSPOptionHudHeightScale <= 0.0f)
		g_fWSPOptionHudHeightScale = 1.0f;
	if (g_fWSPOptionRadarWidthScale <= 0.0f)
		g_fWSPOptionRadarWidthScale = 1.0f;
	if (g_fWSPOptionSubtitlesScale <= 0.0f)
		g_fWSPOptionSubtitlesScale = 1.0f;
	if (g_fWSPOptionRadarWidth <= 0.0f)
		g_fWSPOptionRadarWidth = 94.0f;
	if (g_fWSPOptionRadarHeight <= 0.0f)
		g_fWSPOptionRadarHeight = 76.0f;
	if (g_nWSPOptionTextShadowMode < 0)
		g_nWSPOptionTextShadowMode = 0;
	if (g_nWSPOptionTextShadowMode > 2)
		g_nWSPOptionTextShadowMode = 2;

	g_nWSPOptionHudAspectRatio = AspectConstraintToValue(g_szWSPOptionHudAspectRatio);
	g_nWSPOptionMenuAspectRatio = AspectConstraintToValue(g_szWSPOptionMenuAspectRatio);
#endif
}
