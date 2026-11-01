#pragma once

#ifdef LIBRW
#ifdef EXTENDED_PIPELINES

namespace CustomPipes {


extern rw::TexDictionary *neoTxd;

struct CustomMatExt
{
	rw::Texture *glossTex;
	bool haveGloss;
};
extern rw::int32 CustomMatOffset;
inline CustomMatExt *GetCustomMatExt(rw::Material *mat) {
	return PLUGINOFFSET(CustomMatExt, mat, CustomMatOffset);
}


struct Color
{
	float r, g, b, a;
	Color(void) {}
	Color(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
};

class InterpolatedValue
{
public:
	virtual void Read(char *s, int line, int field) = 0;
};

class InterpolatedFloat : public InterpolatedValue
{
public:
	float data[24][NUMWEATHERS];
	float curInterpolator;
	float curVal;

	InterpolatedFloat(float init);
	void Read(char *s, int line, int field);
	float Get(void);
};

class InterpolatedColor : public InterpolatedValue
{
public:
	Color data[24][NUMWEATHERS];
	float curInterpolator;
	Color curVal;

	InterpolatedColor(const Color &init);
	void Read(char *s, int line, int field);
	Color Get(void);
};

class InterpolatedLight : public InterpolatedColor
{
public:
	InterpolatedLight(const Color &init) : InterpolatedColor(init) {}
	void Read(char *s, int line, int field);
};

char *ReadTweakValueTable(char *fp, InterpolatedValue &interp);





void CustomPipeRegister(void);
void CustomPipeRegisterGL(void);
void CustomPipeInit(void);
void CustomPipeShutdown(void);
void SetTxdFindCallback(void);

extern bool bRenderingEnvMap;
extern int32 EnvMapSize;
void SetEnvMapSize(int32 size);
extern rw::Camera *EnvMapCam;
extern rw::Texture *EnvMapTex;
extern rw::Texture *EnvMaskTex;
void EnvMapRender(void);

enum {
	VEHICLEPIPE_MATFX,
	VEHICLEPIPE_NEO,
	VEHICLEPIPE_LEEDS
};
extern int32 VehiclePipeSwitch;
extern float VehicleShininess;
extern float VehicleSpecularity;
extern InterpolatedFloat Fresnel;
extern InterpolatedFloat Power;
extern InterpolatedLight DiffColor;
extern InterpolatedLight SpecColor;
extern rw::ObjPipeline *vehiclePipe;
void CreateVehiclePipe(void);
void DestroyVehiclePipe(void);
void AttachVehiclePipe(rw::Atomic *atomic);
void AttachVehiclePipe(rw::Clump *clump);

extern float LeedsEnvMult;
#ifdef VICEEXT_SKY_LEEDS_CAR_PIPE
void CreateLeedsVehiclePipe(void);
void DestroyLeedsVehiclePipe(void);
#endif
bool LeedsCarPipeEnabled(void);

enum {
	WORLDPIPE_DEFAULT,
	WORLDPIPE_NEO,
	WORLDPIPE_LEEDS
};
extern bool LightmapEnable;
extern float LightmapMult;
extern InterpolatedFloat WorldLightmapBlend;
extern rw::ObjPipeline *worldPipe;
void CreateWorldPipe(void);
void DestroyWorldPipe(void);
void AttachWorldPipe(rw::Atomic *atomic);
void AttachWorldPipe(rw::Clump *clump);

extern float LeedsWorldPrelightTweakMult;
extern float LeedsWorldPrelightTweakAdd;
#ifdef VICEEXT_SKY_LEEDS_WORLD_PIPE
void CreateLeedsWorldPipe(void);
void DestroyLeedsWorldPipe(void);
#endif
bool LeedsWorldPipeEnabled(void);

extern bool GlossEnable;
extern float GlossMult;
extern rw::ObjPipeline *glossPipe;
void CreateGlossPipe(void);
void DestroyGlossPipe(void);
void AttachGlossPipe(rw::Atomic *atomic);
void AttachGlossPipe(rw::Clump *clump);
rw::Texture *GetGlossTex(rw::Material *mat);

extern bool RimlightEnable;
extern float RimlightMult;
extern InterpolatedColor RampStart;
extern InterpolatedColor RampEnd;
extern InterpolatedFloat Offset;
extern InterpolatedFloat Scale;
extern InterpolatedFloat Scaling;
extern rw::ObjPipeline *rimPipe;
extern rw::ObjPipeline *rimSkinPipe;
void CreateRimLightPipes(void);
void DestroyRimLightPipes(void);
void AttachRimPipe(rw::Atomic *atomic);
void AttachRimPipe(rw::Clump *clump);

}

extern int32 g_nSkyEnvMapSize;
extern int32 g_nSkyMatFXBlendStyle;
extern int32 g_nSkyMatFXTexgenStyle;
extern bool g_bSkyMatFXPS2Light;

extern int32 g_nSkyWorldPipe;

extern bool g_bSkyDualPass;



#endif

namespace WorldRender{
extern int numBlendInsts[3];
void AtomicFirstPass(RpAtomic *atomic, int pass);
void AtomicFullyTransparent(RpAtomic *atomic, int pass, int fadeAlpha);
void RenderBlendPass(int pass);
}

#endif
