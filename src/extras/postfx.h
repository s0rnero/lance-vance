#pragma once

#ifdef EXTENDED_COLOURFILTER

class CPostFX
{
public:
	enum {
		POSTFX_OFF,
		POSTFX_SIMPLE,
		POSTFX_NORMAL,
		POSTFX_MOBILE
	};
	static RwRaster *pFrontBuffer;
	static RwRaster *pBackBuffer;
	static bool bJustInitialised;
	static int EffectSwitch;
	static bool BlurOn;	// or use CMblur for that?
	static bool MotionBlurOn;	// or use CMblur for that?
	static float Intensity;

	static bool UseTrailsBlur(void);
	static bool UseTrailsMotionBlur(void);
	static int32 BlurAlpha(void);
	static float BlurOffset(void);

	static bool ColourOverlayEnabled(void);

	static void RenderSeamFix(RwCamera *cam, int32 offX, int32 offY);

	static bool m_bYCbCrFilter;
	static float m_lumaScale;
	static float m_lumaOffset;
	static float m_cbScale;
	static float m_cbOffset;
	static float m_crScale;
	static float m_crOffset;

	// smooth blur color
	enum { NUMAVERAGE = 20 };
	static int PrevRed[NUMAVERAGE], AvgRed;
	static int PrevGreen[NUMAVERAGE], AvgGreen;
	static int PrevBlue[NUMAVERAGE], AvgBlue;
	static int PrevAlpha[NUMAVERAGE], AvgAlpha;
	static int Next;
	static int NumValues;

	static void InitOnce(void);
	static void Open(RwCamera *cam);
	static void Close(void);
	static void RenderOverlayBlur(RwCamera *cam, int32 r, int32 g, int32 b, int32 a);
	static void RenderOverlaySniper(RwCamera *cam, int32 r, int32 g, int32 b, int32 a);
	static void RenderOverlayShader(RwCamera *cam, int32 r, int32 g, int32 b, int32 a);
	static void RenderMotionBlur(RwCamera *cam, uint32 blur);
	static void Render(RwCamera *cam, uint32 red, uint32 green, uint32 blue, uint32 blur, int32 type, uint32 bluralpha);
	static void SmoothColor(uint32 red, uint32 green, uint32 blue, uint32 alpha);
	static bool NeedBackBuffer(void);
	static bool NeedFrontBuffer(int32 type);
	static void GetBackBuffer(RwCamera *cam);
	static bool UseBlurColours(void) { return EffectSwitch != POSTFX_SIMPLE; }
};

extern bool g_bSkyDisableColourOverlay;

extern bool g_bSkyTrailsBlur;
extern bool g_bSkyTrailsMotionBlur;

extern float g_fSkyCurrentBlurAlpha;
extern float g_fSkyCurrentBlurOffset;

extern int32 g_nSkySeamOffsetX;
extern bool g_bSkySeamFix;
extern int32 g_nSkySeamOffsetY;

#endif
