#include "common.h"

#include "Draw.h"
#include "Frontend.h"
#include "Camera.h"
#include "CutsceneMgr.h"
#include "WidescreenFixOptions.h"

float CDraw::ms_fAspectRatio = DEFAULT_ASPECT_RATIO;
#ifdef ASPECT_RATIO_SCALE
float CDraw::ms_fScaledFOV = 45.0f;
#endif

float CDraw::ms_fNearClipZ;
float CDraw::ms_fFarClipZ;
float CDraw::ms_fFOV = 45.0f;
float CDraw::ms_fLODDistance;
float CDraw::ms_fCutsceneCameraZoom = 1.0f;

uint8 CDraw::FadeValue;
uint8 CDraw::FadeRed;
uint8 CDraw::FadeGreen;
uint8 CDraw::FadeBlue;

#ifdef PROPER_SCALING	
bool CDraw::ms_bProperScaling = true;
#endif
#ifdef  FIX_RADAR
bool CDraw::ms_bFixRadar = true;	
#endif
#ifdef FIX_SPRITES
bool CDraw::ms_bFixSprites = true;	
#endif

#ifdef ASPECT_RATIO_SCALE
#ifdef VICEEXT_WFP_ASPECT_RATIO
static bool
GetForcedAspectRatio(float *ratio)
{
	if (!ratio)
		return false;
	const char *szRatio = g_szWSPOptionForceAspectRatio;
	if (!szRatio || !szRatio[0] || strncmp(szRatio, "auto", 4) == 0)
		return false;
	const char *szColon = strchr(szRatio, ':');
	if (!szColon)
		return false;
	int32 nWidth = atoi(szRatio);
	int32 nHeight = atoi(szColon + 1);
	if (nWidth <= 0 || nHeight <= 0)
		return false;
	*ratio = (float)nWidth / (float)nHeight;
	return true;
}
#endif

float
FindAspectRatio(void)
{
#ifdef VICEEXT_WFP_ASPECT_RATIO
	float fForcedAspectRatio = 0.0f;
	if (GetForcedAspectRatio(&fForcedAspectRatio))
		return fForcedAspectRatio;
#endif
	switch (FrontEndMenuManager.m_PrefsUseWideScreen) {
	case AR_AUTO:
		return SCREEN_WIDTH / SCREEN_HEIGHT;
	default:
	case AR_4_3:
		return 4.0f / 3.0f;
	case AR_5_4:
		return 5.0f / 4.0f;
	case AR_16_10:
		return 16.0f / 10.0f;
	case AR_16_9:
		return 16.0f / 9.0f;
	case AR_21_9:
		return 21.0f / 9.0f;
	};
}
#endif

float
CDraw::CalculateAspectRatio(void)
{
#ifdef ASPECT_RATIO_SCALE
	if (TheCamera.m_WideScreenOn)
		CDraw::ms_fAspectRatio = (5.f / 3.f) * FindAspectRatio() / (16.f / 9.f); // It's used on theatrical showings according to Wiki
	else
		CDraw::ms_fAspectRatio = FindAspectRatio();
#else
	if(FrontEndMenuManager.m_PrefsUseWideScreen) {
		if (TheCamera.m_WideScreenOn)
			CDraw::ms_fAspectRatio = 5.f / 3.f; // It's used on theatrical showings according to Wiki
		else
			CDraw::ms_fAspectRatio = 16.f / 9.f;
	} else if (TheCamera.m_WideScreenOn) {
		CDraw::ms_fAspectRatio = 5.f/4.f;
	} else {
		CDraw::ms_fAspectRatio = 4.f/3.f;
	}
#endif
	return CDraw::ms_fAspectRatio;
}

#ifdef ASPECT_RATIO_SCALE
// convert a 4:3 hFOV to vFOV,
// then convert that vFOV to hFOV for our aspect ratio,
// i.e. HOR+
float
CDraw::ConvertFOV(float hfov)
{
	// => tan(hFOV/2) = tan(vFOV/2)*aspectRatio
	// => tan(vFOV/2) = tan(hFOV/2)/aspectRatio
	float ar1 = DEFAULT_ASPECT_RATIO;
	float ar2 = GetAspectRatio();
	hfov = DEGTORAD(hfov);
	float vfov = Atan(tan(hfov/2) / ar1) *2;
	hfov = Atan(tan(vfov/2) * ar2) *2;
	return RADTODEG(hfov);
}
#endif

float
CDraw::ConvertFOVInverse(float fov)
{
	float ar1 = DEFAULT_ASPECT_RATIO;
	float ar2 = GetAspectRatio();
	float rad = DEGTORAD(fov);
	return RADTODEG(2.0f * Atan(tan(rad * 0.5f) * (ar1 / ar2)));
}

float
CDraw::ConvertFOVforCutscene(float fov)
{
	if (ms_fCutsceneCameraZoom != 1.0f) {
		float rad = DEGTORAD(fov);
		return RADTODEG(2.0f * Atan(tan(rad * 0.5f) * ms_fCutsceneCameraZoom));
	}
	return fov;
}

void
CDraw::SetCutsceneCameraZoom(float zoom)
{
	ms_fCutsceneCameraZoom = zoom;
}

#ifdef VICEEXT_WFP_FOV_MULTIPLIERS
static const int FOV_MULTIPLIER_SLOTS = 8;

struct FOVMultiplierSlot
{
	uint32 key;
	float value;
};

static FOVMultiplierSlot ms_odFOVMultipliers[FOV_MULTIPLIER_SLOTS];
static int ms_odFOVMultiplierCount = 0;

static float
GetFOVMultiplier(void)
{
	float fTotal = 1.0f;
	for (int i = 0; i < ms_odFOVMultiplierCount; i++)
		fTotal *= ms_odFOVMultipliers[i].value;
	return fTotal;
}

void
CDraw::SetFOVMultiplier(uint32 key, float value)
{
	if (value <= 0.0f)
		value = 1.0f;
	if (value < 0.5f)
		value = 0.5f;
	if (value > 2.0f)
		value = 2.0f;
	for (int i = 0; i < ms_odFOVMultiplierCount; i++) {
		if (ms_odFOVMultipliers[i].key == key) {
			ms_odFOVMultipliers[i].value = value;
			return;
		}
	}
	if (ms_odFOVMultiplierCount >= FOV_MULTIPLIER_SLOTS)
		return;
	ms_odFOVMultipliers[ms_odFOVMultiplierCount].key = key;
	ms_odFOVMultipliers[ms_odFOVMultiplierCount].value = value;
	ms_odFOVMultiplierCount++;
}

void
CDraw::RemoveFOVMultiplier(uint32 key)
{
	for (int i = 0; i < ms_odFOVMultiplierCount; i++) {
		if (ms_odFOVMultipliers[i].key == key) {
			ms_odFOVMultipliers[i] = ms_odFOVMultipliers[ms_odFOVMultiplierCount - 1];
			ms_odFOVMultiplierCount--;
			return;
		}
	}
}

float
CDraw::GetCurrentFOV(void)
{
	return CDraw::GetFOV() * GetFOVMultiplier();
}
#endif

void
CDraw::SetFOV(float fov)
{
#ifdef ASPECT_RATIO_SCALE
	if (g_bWSPOptionDontTouchFOV)
		ms_fScaledFOV = fov;
	else if (!CCutsceneMgr::IsRunning()) {
#ifdef VICEEXT_WFP_FOV_MULTIPLIERS
		ms_fScaledFOV = ConvertFOV(fov) * GetFOVMultiplier();
#else
		ms_fScaledFOV = ConvertFOV(fov);
#endif
		if (!TheCamera.m_WideScreenOn)
			ms_fCutsceneCameraZoom = 1.0f;
	} else {
		// 30/10 (T55): la cinematica tambien pasa por ConvertFOV. Antes, con
		// `CutsceneFOV` apagado, se metia el FOV horizontal de 4:3 tal cual en
		// una camara 16:9 y la escena salia estirada en horizontal.
		float fconverted = ConvertFOV(fov);
		ms_fScaledFOV = g_bWSPOptionCutsceneFOV ? ConvertFOVforCutscene(fconverted) : fconverted;
	}
#endif
	ms_fFOV = fov;
}

#ifdef PROPER_SCALING	
float CDraw::ScaleY(float y)
{
	return ms_bProperScaling ? y : y * ((float)DEFAULT_SCREEN_HEIGHT/SCREEN_HEIGHT_NTSC);
}
#endif 