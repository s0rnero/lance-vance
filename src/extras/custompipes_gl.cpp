#include "common.h"

#ifdef RW_OPENGL
#include "main.h"
#include "RwHelper.h"
#include "Lights.h"
#include "Timecycle.h"
#include "FileMgr.h"
#include "Clock.h"
#include "Weather.h"
#include "TxdStore.h"
#include "Renderer.h"
#include "World.h"
#include "custompipes.h"

#ifdef EXTENDED_PIPELINES

#ifndef LIBRW
#error "Need librw for EXTENDED_PIPELINES"
#endif

namespace CustomPipes {

static int32 u_viewVec;
static int32 u_rampStart;
static int32 u_rampEnd;
static int32 u_rimData;

static int32 u_lightMap;

static int32 u_eye;
static int32 u_reflProps;
static int32 u_specDir;
static int32 u_specColor;

#ifdef VICEEXT_SKY_LEEDS_CAR_PIPE
static int32 u_leedsEnv;
static int32 u_texMatrix;
#endif

#ifdef VICEEXT_SKY_LEEDS_WORLD_PIPE
static int32 u_prelightTweak;
#endif

#define U(i) currentShader->uniformLocations[i]

/*
 * Neo Vehicle pipe
 */

rw::gl3::Shader *neoVehicleShader;

static void
DrawDualPass(rw::gl3::InstanceDataHeader *header, rw::gl3::InstanceData *inst)
{
	using namespace rw;
	using namespace rw::gl3;

	if(!g_bSkyDualPass || !getAlphaBlend() || !GetRenderState(ZWRITEENABLE)){
		drawInst(header, inst);
		return;
	}

	drawInstDualPass(header, inst);
}

static void
uploadSpecLights(void)
{
	using namespace rw::gl3;

	rw::RGBAf colors[1 + NUMEXTRADIRECTIONALS];
	struct {
		rw::V3d dir;
		float power;
	} dirs[1 + NUMEXTRADIRECTIONALS];
	memset(colors, 0, sizeof(colors));
	memset(dirs, 0, sizeof(dirs));
	for(int i = 0; i < 1+NUMEXTRADIRECTIONALS; i++)
		dirs[i].power = 1.0f;
	float power = Power.Get();
	Color speccol = SpecColor.Get();
	colors[0].red = speccol.r;
	colors[0].green = speccol.g;
	colors[0].blue = speccol.b;
	dirs[0].dir = pDirect->getFrame()->getLTM()->at;
	dirs[0].power = power;
	for(int i = 0; i < NUMEXTRADIRECTIONALS; i++){
		if(pExtraDirectionals[i]->getFlags() & rw::Light::LIGHTATOMICS){
			colors[1+i] = pExtraDirectionals[i]->color;
			dirs[1+i].dir = pExtraDirectionals[i]->getFrame()->getLTM()->at;
			dirs[1+i].power = power*2.0f;
		}
	}
	glUniform4fv(U(u_specDir), 1 + NUMEXTRADIRECTIONALS, (float*)&dirs);
	glUniform4fv(U(u_specColor), 1 + NUMEXTRADIRECTIONALS, (float*)&colors);
}

#ifdef VICEEXT_SKY_LEEDS_CAR_PIPE

rw::gl3::Shader *neoLeedsCarShader;

static void
uploadLeedsEnvMatrix(void)
{
	using namespace rw;

	static RawMatrix normal2texcoord = {
		{ 0.5f,  0.0f, 0.0f }, 0.0f,
		{ 0.0f, -0.5f, 0.0f }, 0.0f,
		{ 0.0f,  0.0f, 1.0f }, 0.0f,
		{ 0.5f,  0.5f, 0.0f }, 1.0f
	};

	Matrix invMat;
	RawMatrix invMtx, envMtx;

	Matrix::invert(&invMat, EnvMapCam->getFrame()->getLTM());
	convMatrix(&invMtx, &invMat);
	invMtx.pos.set(0.0f, 0.0f, 0.0f);
	RawMatrix::mult(&envMtx, &invMtx, &normal2texcoord);

	rw::gl3::setUniform(u_texMatrix, &envMtx);
}

static void
leedsVehicleRenderCB(rw::Atomic *atomic, rw::gl3::InstanceDataHeader *header)
{
	using namespace rw;
	using namespace rw::gl3;

	Material *m;

	rw::uint32 flags = atomic->geometry->flags;
	setWorldMatrix(atomic->getFrame()->getLTM());
	lightingCB(atomic);

	setupVertexInput(header);

	InstanceData *inst = header->inst;
	rw::int32 n = header->numMeshes;

	neoLeedsCarShader->use();

	uploadLeedsEnvMatrix();

	setTexture(1, EnvMapTex);

	SetRenderState(SRCBLEND, BLENDSRCALPHA);
	SetRenderState(DESTBLEND, BLENDINVSRCALPHA);

	while(n--){
		m = inst->material;

		setMaterial(flags, m->color, m->surfaceProps);

		setTexture(0, m->texture);

		rw::SetRenderState(VERTEXALPHA, inst->vertexAlpha || m->color.alpha != 0xFF);

		float envProps[4];
		envProps[0] = 0.0f;
		MatFX *matfx = MatFX::get(m);
		if(matfx)
			envProps[0] = matfx->getEnvCoefficient()*LeedsEnvMult;
		envProps[1] = 0.0f;
		envProps[2] = 0.0f;
		envProps[3] = 0.0f;
		glUniform4fv(U(u_leedsEnv), 1, envProps);

		DrawDualPass(header, inst);
		inst++;
	}

	setTexture(1, nil);

	teardownVertexInput(header);
}

void
CreateLeedsVehiclePipe(void)
{
	using namespace rw;
	using namespace rw::gl3;

	{
#include "shaders/obj/neoLeedsCar_frag.inc"
#include "shaders/obj/neoLeedsCar_vert.inc"
	const char *vs[] = { shaderDecl, "#define DIRECTIONALS\n", header_vert_src, neoLeedsCar_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, neoLeedsCar_frag_src, nil };
	neoLeedsCarShader = Shader::create(vs, fs);
	assert(neoLeedsCarShader);
	}
}

void
DestroyLeedsVehiclePipe(void)
{
	neoLeedsCarShader->destroy();
	neoLeedsCarShader = nil;
}

#endif

static void
vehicleRenderCB(rw::Atomic *atomic, rw::gl3::InstanceDataHeader *header)
{
	using namespace rw;
	using namespace rw::gl3;

	// TODO: make this less of a kludge
	if(VehiclePipeSwitch == VEHICLEPIPE_MATFX){
		matFXGlobals.pipelines[rw::platform]->render(atomic);
		return;
	}

#ifdef VICEEXT_SKY_LEEDS_CAR_PIPE
	if(LeedsCarPipeEnabled()){
		leedsVehicleRenderCB(atomic, header);
		return;
	}
#endif

	Material *m;

	rw::uint32 flags = atomic->geometry->flags;
	setWorldMatrix(atomic->getFrame()->getLTM());
	lightingCB(atomic);

	setupVertexInput(header);

	InstanceData *inst = header->inst;
	rw::int32 n = header->numMeshes;

	neoVehicleShader->use();

	V3d eyePos = rw::engine->currentCamera->getFrame()->getLTM()->pos;
	glUniform3fv(U(u_eye), 1, (float*)&eyePos);

	uploadSpecLights();

	float reflProps[4];
	reflProps[0] = Fresnel.Get();
	reflProps[1] = SpecColor.Get().a;

	setTexture(1, EnvMapTex);

	SetRenderState(SRCBLEND, BLENDONE);

	while(n--){
		m = inst->material;

		setMaterial(flags, m->color, m->surfaceProps);

		setTexture(0, m->texture);

		rw::SetRenderState(VERTEXALPHA, inst->vertexAlpha || m->color.alpha != 0xFF);

		reflProps[2] = m->surfaceProps.specular * VehicleShininess;
		reflProps[3] = m->surfaceProps.specular == 0.0f ? 0.0f : VehicleSpecularity;
		glUniform4fv(U(u_reflProps), 1, reflProps);

		DrawDualPass(header, inst);
		inst++;
	}

	setTexture(1, nil);

	SetRenderState(SRCBLEND, BLENDSRCALPHA);

	teardownVertexInput(header);
}

void
CreateVehiclePipe(void)
{
	using namespace rw;
	using namespace rw::gl3;

	if(CFileMgr::LoadFile("neo/carTweakingTable.dat", work_buff, sizeof(work_buff), "r") <= 0)
		printf("Error: couldn't open 'neo/carTweakingTable.dat'\n");
	else{
		char *fp = (char*)work_buff;
		fp = ReadTweakValueTable(fp, Fresnel);
		fp = ReadTweakValueTable(fp, Power);
		fp = ReadTweakValueTable(fp, DiffColor);
		fp = ReadTweakValueTable(fp, SpecColor);
	}


	{
#include "shaders/obj/neoVehicle_frag.inc"
#include "shaders/obj/neoVehicle_vert.inc"
	const char *vs[] = { shaderDecl, "#define DIRECTIONALS\n", header_vert_src, neoVehicle_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, neoVehicle_frag_src, nil };
	neoVehicleShader = Shader::create(vs, fs);
	assert(neoVehicleShader);
	}


	rw::gl3::ObjPipeline *pipe = rw::gl3::ObjPipeline::create();
	pipe->instanceCB = rw::gl3::defaultInstanceCB;
	pipe->uninstanceCB = nil;
	pipe->renderCB = vehicleRenderCB;
	vehiclePipe = pipe;
}

void
DestroyVehiclePipe(void)
{
	neoVehicleShader->destroy();
	neoVehicleShader = nil;

	((rw::gl3::ObjPipeline*)vehiclePipe)->destroy();
	vehiclePipe = nil;
}



/*
 * Neo World pipe
 */

rw::gl3::Shader *neoWorldShader;

#ifdef VICEEXT_SKY_LEEDS_WORLD_PIPE

rw::gl3::Shader *leedsWorldShader;

static void
leedsWorldRenderCB(rw::Atomic *atomic, rw::gl3::InstanceDataHeader *header)
{
	using namespace rw;
	using namespace rw::gl3;

	Material *m;

	rw::uint32 flags = atomic->geometry->flags;
	bool lit = (flags & Geometry::LIGHT) != 0;

	setWorldMatrix(atomic->getFrame()->getLTM());
	lightingCB(atomic);

	setupVertexInput(header);

	InstanceData *inst = header->inst;
	rw::int32 n = header->numMeshes;

	leedsWorldShader->use();

	float tweak[4];
	tweak[0] = LeedsWorldPrelightTweakMult;
	tweak[1] = LeedsWorldPrelightTweakAdd;
	tweak[2] = lit ? 1.0f : 0.0f;
	tweak[3] = 0.0f;
	glUniform4fv(U(u_prelightTweak), 1, tweak);

	while(n--){
		m = inst->material;

		if(lit){
			setMaterial(flags, m->color, m->surfaceProps, 0.5f);
		}else{
			RGBA color = { 255, 255, 255, m->color.alpha };
			setMaterial(color, m->surfaceProps, 0.0f);
		}

		setTexture(0, m->texture);

		rw::SetRenderState(VERTEXALPHA, inst->vertexAlpha || m->color.alpha != 0xFF);

		DrawDualPass(header, inst);
		inst++;
	}

	teardownVertexInput(header);
}

void
CreateLeedsWorldPipe(void)
{
	using namespace rw;
	using namespace rw::gl3;

	{
#include "shaders/obj/leedsWorld_frag.inc"
#include "shaders/obj/leedsWorld_vert.inc"
	const char *vs[] = { shaderDecl, header_vert_src, leedsWorld_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, leedsWorld_frag_src, nil };
	leedsWorldShader = Shader::create(vs, fs);
	assert(leedsWorldShader);
	}
}

void
DestroyLeedsWorldPipe(void)
{
	leedsWorldShader->destroy();
	leedsWorldShader = nil;
}

#endif

static void
worldRenderCB(rw::Atomic *atomic, rw::gl3::InstanceDataHeader *header)
{
	using namespace rw;
	using namespace rw::gl3;

#ifdef VICEEXT_SKY_LEEDS_WORLD_PIPE
	if(LeedsWorldPipeEnabled()){
		leedsWorldRenderCB(atomic, header);
		return;
	}
#endif

	if(g_nSkyWorldPipe != WORLDPIPE_NEO && !LightmapEnable){
		if(g_bSkyDualPass)
			gl3::defaultRenderCBDualPass(atomic, header);
		else
			gl3::defaultRenderCB(atomic, header);
		return;
	}

	Material *m;

	setWorldMatrix(atomic->getFrame()->getLTM());
	lightingCB(atomic);

	setupVertexInput(header);

	InstanceData *inst = header->inst;
	rw::int32 n = header->numMeshes;

	neoWorldShader->use();

	float lightfactor[4];

	while(n--){
		m = inst->material;

		if(MatFX::getEffects(m) == MatFX::DUAL){
			MatFX *matfx = MatFX::get(m);
			Texture *dualtex = matfx->getDualTexture();
			if(dualtex == nil)
				goto notex;
			setTexture(1, dualtex);
			lightfactor[0] = lightfactor[1] = lightfactor[2] = WorldLightmapBlend.Get()*LightmapMult;
		}else{
		notex:
			setTexture(1, nil);
			lightfactor[0] = lightfactor[1] = lightfactor[2] = 0.0f;
		}
		lightfactor[3] = m->color.alpha/255.0f;
		glUniform4fv(U(u_lightMap), 1, lightfactor);

		RGBA color = { 255, 255, 255, m->color.alpha };
		setMaterial(color, m->surfaceProps);

		setTexture(0, m->texture);

		rw::SetRenderState(VERTEXALPHA, inst->vertexAlpha || m->color.alpha != 0xFF);

		DrawDualPass(header, inst);
		inst++;
	}
	setTexture(1, nil);
	teardownVertexInput(header);
}

void
CreateWorldPipe(void)
{
	using namespace rw;
	using namespace rw::gl3;

	if(CFileMgr::LoadFile("neo/worldTweakingTable.dat", work_buff, sizeof(work_buff), "r") <= 0)
		printf("Error: couldn't open 'neo/worldTweakingTable.dat'\n");
	else
		ReadTweakValueTable((char*)work_buff, WorldLightmapBlend);

	{
#include "shaders/obj/neoWorldVC_frag.inc"
#include "shaders/obj/default_UV2_vert.inc"
	const char *vs[] = { shaderDecl, header_vert_src, default_UV2_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, neoWorldVC_frag_src, nil };
	neoWorldShader = Shader::create(vs, fs);
	assert(neoWorldShader);
	}


	rw::gl3::ObjPipeline *pipe = rw::gl3::ObjPipeline::create();
	pipe->instanceCB = rw::gl3::defaultInstanceCB;
	pipe->uninstanceCB = nil;
	pipe->renderCB = worldRenderCB;
	worldPipe = pipe;
}

void
DestroyWorldPipe(void)
{
	neoWorldShader->destroy();
	neoWorldShader = nil;

	((rw::gl3::ObjPipeline*)worldPipe)->destroy();
	worldPipe = nil;
}




/*
 * Neo Gloss pipe
 */

rw::gl3::Shader *neoGlossShader;

static void
glossRenderCB(rw::Atomic *atomic, rw::gl3::InstanceDataHeader *header)
{
	using namespace rw;
	using namespace rw::gl3;

	worldRenderCB(atomic, header);
	if(!GlossEnable)
		return;

	Material *m;

	setupVertexInput(header);

	InstanceData *inst = header->inst;
	rw::int32 n = header->numMeshes;

	neoGlossShader->use();

	V3d eyePos = rw::engine->currentCamera->getFrame()->getLTM()->pos;
	glUniform3fv(U(u_eye), 1, (float*)&eyePos);
	float reflProps[4];
	reflProps[0] = GlossMult;
	reflProps[1] = 0.0f;
	reflProps[2] = 0.0f;
	reflProps[3] = 0.0f;
	glUniform4fv(U(u_reflProps), 1, reflProps);

	SetRenderState(VERTEXALPHA, TRUE);
	SetRenderState(SRCBLEND, BLENDONE);
	SetRenderState(DESTBLEND, BLENDONE);
	SetRenderState(ZWRITEENABLE, FALSE);
	SetRenderState(ALPHATESTFUNC, ALPHAALWAYS);

	while(n--){
		m = inst->material;

		RGBA color = { 255, 255, 255, m->color.alpha };
		setMaterial(color, m->surfaceProps);

		if(m->texture){
			Texture *tex = GetGlossTex(m);
			if(tex){
				setTexture(0, tex);
				DrawDualPass(header, inst);
			}
		}
		inst++;
	}

	SetRenderState(ZWRITEENABLE, TRUE);
	SetRenderState(ALPHATESTFUNC, ALPHAGREATEREQUAL);
	SetRenderState(SRCBLEND, BLENDSRCALPHA);
	SetRenderState(DESTBLEND, BLENDINVSRCALPHA);

	teardownVertexInput(header);
}

void
CreateGlossPipe(void)
{
	using namespace rw;
	using namespace rw::gl3;

	{
#include "shaders/obj/neoGloss_frag.inc"
#include "shaders/obj/neoGloss_vert.inc"
	const char *vs[] = { shaderDecl, header_vert_src, neoGloss_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, neoGloss_frag_src, nil };
	neoGlossShader = Shader::create(vs, fs);
	assert(neoGlossShader);
	}

	rw::gl3::ObjPipeline *pipe = rw::gl3::ObjPipeline::create();
	pipe->instanceCB = rw::gl3::defaultInstanceCB;
	pipe->uninstanceCB = nil;
	pipe->renderCB = glossRenderCB;
	glossPipe = pipe;
}

void
DestroyGlossPipe(void)
{
	neoGlossShader->destroy();
	neoGlossShader = nil;

	((rw::gl3::ObjPipeline*)glossPipe)->destroy();
	glossPipe = nil;
}



/*
 * Neo Rim pipes
 */

rw::gl3::Shader *neoRimShader;
rw::gl3::Shader *neoRimSkinShader;

static void
uploadRimData(bool enable)
{
	using namespace rw;
	using namespace rw::gl3;

	V3d viewVec = rw::engine->currentCamera->getFrame()->getLTM()->at;
	glUniform3fv(U(u_viewVec), 1, (float*)&viewVec);
	float rimData[4];
	rimData[0] = Offset.Get();
	rimData[1] = Scale.Get();
	if(enable)
		rimData[2] = Scaling.Get()*RimlightMult;
	else
		rimData[2] = 0.0f;
	rimData[3] = 0.0f;
	glUniform3fv(U(u_rimData), 1, rimData);
	Color col = RampStart.Get();
	glUniform4fv(U(u_rampStart), 1, (float*)&col);
	col = RampEnd.Get();
	glUniform4fv(U(u_rampEnd), 1, (float*)&col);
}

static void
rimSkinRenderCB(rw::Atomic *atomic, rw::gl3::InstanceDataHeader *header)
{
	using namespace rw;
	using namespace rw::gl3;

	if(!RimlightEnable){
		gl3::skinRenderCB(atomic, header);
		return;
	}

	Material *m;

	rw::uint32 flags = atomic->geometry->flags;
	setWorldMatrix(atomic->getFrame()->getLTM());
	lightingCB(atomic);

	setupVertexInput(header);

	InstanceData *inst = header->inst;
	rw::int32 n = header->numMeshes;

	neoRimSkinShader->use();

	uploadRimData(atomic->geometry->flags & Geometry::LIGHT);

	uploadSkinMatrices(atomic);

	while(n--){
		m = inst->material;

		setMaterial(flags, m->color, m->surfaceProps);

		setTexture(0, m->texture);

		rw::SetRenderState(VERTEXALPHA, inst->vertexAlpha || m->color.alpha != 0xFF);

		DrawDualPass(header, inst);
		inst++;
	}
	teardownVertexInput(header);
}

static void
rimRenderCB(rw::Atomic *atomic, rw::gl3::InstanceDataHeader *header)
{
	using namespace rw;
	using namespace rw::gl3;

	if(!RimlightEnable){
		gl3::defaultRenderCB(atomic, header);
		return;
	}

	Material *m;

	rw::uint32 flags = atomic->geometry->flags;
	setWorldMatrix(atomic->getFrame()->getLTM());
	lightingCB(atomic);

	setupVertexInput(header);

	InstanceData *inst = header->inst;
	rw::int32 n = header->numMeshes;

	neoRimShader->use();

	uploadRimData(atomic->geometry->flags & Geometry::LIGHT);

	while(n--){
		m = inst->material;

		setMaterial(flags, m->color, m->surfaceProps);

		setTexture(0, m->texture);

		rw::SetRenderState(VERTEXALPHA, inst->vertexAlpha || m->color.alpha != 0xFF);

		DrawDualPass(header, inst);
		inst++;
	}
	teardownVertexInput(header);
}

void
CreateRimLightPipes(void)
{
	using namespace rw::gl3;

	if(CFileMgr::LoadFile("neo/rimTweakingTable.dat", work_buff, sizeof(work_buff), "r") <= 0)
		printf("Error: couldn't open 'neo/rimTweakingTable.dat'\n");
	else{
		char *fp = (char*)work_buff;
		fp = ReadTweakValueTable(fp, RampStart);
		fp = ReadTweakValueTable(fp, RampEnd);
		fp = ReadTweakValueTable(fp, Offset);
		fp = ReadTweakValueTable(fp, Scale);
		fp = ReadTweakValueTable(fp, Scaling);
	}

	{
#include "shaders/obj/simple_frag.inc"
#include "shaders/obj/neoRimSkin_vert.inc"
	const char *vs[] = { shaderDecl, "#define DIRECTIONALS\n", header_vert_src, neoRimSkin_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, simple_frag_src, nil };
	neoRimSkinShader = Shader::create(vs, fs);
	assert(neoRimSkinShader);
	}

	{
#include "shaders/obj/simple_frag.inc"
#include "shaders/obj/neoRim_vert.inc"
	const char *vs[] = { shaderDecl, "#define DIRECTIONALS\n", header_vert_src, neoRim_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, simple_frag_src, nil };
	neoRimShader = Shader::create(vs, fs);
	assert(neoRimShader);
	}


	rw::gl3::ObjPipeline *pipe = rw::gl3::ObjPipeline::create();
	pipe->instanceCB = rw::gl3::defaultInstanceCB;
	pipe->uninstanceCB = nil;
	pipe->renderCB = rimRenderCB;
	rimPipe = pipe;

	pipe = rw::gl3::ObjPipeline::create();
	pipe->instanceCB = rw::gl3::skinInstanceCB;
	pipe->uninstanceCB = nil;
	pipe->renderCB = rimSkinRenderCB;
	rimSkinPipe = pipe;
}

void
DestroyRimLightPipes(void)
{
	neoRimShader->destroy();
	neoRimShader = nil;

	neoRimSkinShader->destroy();
	neoRimSkinShader = nil;

	((rw::gl3::ObjPipeline*)rimPipe)->destroy();
	rimPipe = nil;

	((rw::gl3::ObjPipeline*)rimSkinPipe)->destroy();
	rimSkinPipe = nil;
}



void
CustomPipeRegisterGL(void)
{
	u_viewVec = rw::gl3::registerUniform("u_viewVec");
	u_rampStart = rw::gl3::registerUniform("u_rampStart");
	u_rampEnd = rw::gl3::registerUniform("u_rampEnd");
	u_rimData = rw::gl3::registerUniform("u_rimData");

	u_lightMap = rw::gl3::registerUniform("u_lightMap");

	u_eye = rw::gl3::registerUniform("u_eye");
	u_reflProps = rw::gl3::registerUniform("u_reflProps");
	u_specDir = rw::gl3::registerUniform("u_specDir");
	u_specColor = rw::gl3::registerUniform("u_specColor");
#ifdef VICEEXT_SKY_LEEDS_CAR_PIPE
	u_leedsEnv = rw::gl3::registerUniform("u_leedsEnv");
	u_texMatrix = rw::gl3::registerUniform("u_texMatrix", rw::gl3::UNIFORM_MAT4);
#endif
#ifdef VICEEXT_SKY_LEEDS_WORLD_PIPE
	u_prelightTweak = rw::gl3::registerUniform("u_prelightTweak");
#endif
}


}

#endif

#ifdef NEW_RENDERER
#ifndef LIBRW
#error "Need librw for NEW_PIPELINES"
#endif

namespace WorldRender
{

struct BuildingInst
{
	rw::Matrix matrix;
	rw::gl3::InstanceDataHeader *instHeader;
	uint8 fadeAlpha;
	bool lighting;
};
#define MAXBLENDINSTS 4096
BuildingInst blendInsts[3][MAXBLENDINSTS];
int numBlendInsts[3];

static RwRGBAReal black;

static bool
IsTextureTransparent(RwTexture *tex)
{
	if(tex == nil || tex->raster == nil)
		return false;
	return PLUGINOFFSET(rw::gl3::Gl3Raster, tex->raster, rw::gl3::nativeRasterOffset)->hasAlpha;
}

// Render all opaque meshes and put atomics that needs blending
// into the deferred list.
void
AtomicFirstPass(RpAtomic *atomic, int pass)
{
	if(numBlendInsts[pass] >= MAXBLENDINSTS)
		return;

	using namespace rw;
	using namespace rw::gl3;

	BuildingInst *building = &blendInsts[pass][numBlendInsts[pass]];

	atomic->getPipeline()->instance(atomic);
	building->instHeader = (gl3::InstanceDataHeader*)atomic->geometry->instData;
	assert(building->instHeader != nil);
	assert(building->instHeader->platform == PLATFORM_GL3);
	building->fadeAlpha = 255;
	building->lighting = !!(atomic->geometry->flags & rw::Geometry::LIGHT);
	rw::uint32 flags = atomic->geometry->flags;

	WorldLights lights;
	lights.numAmbients = 1;
	lights.numDirectionals = 0;
	lights.numLocals = 0;
	if(building->lighting)
		lights.ambient = pAmbient->color;
	else
		lights.ambient = black;

	bool setupDone = false;
	bool defer = false;
	building->matrix = *atomic->getFrame()->getLTM();

	InstanceData *inst = building->instHeader->inst;
	for(rw::uint32 i = 0; i < building->instHeader->numMeshes; i++, inst++){
		Material *m = inst->material;

		if(inst->vertexAlpha || m->color.alpha != 255 ||
		   IsTextureTransparent(m->texture)){
			defer = true;
			continue;
		}

		// alright we're rendering this atomic
		if(!setupDone){
			defaultShader->use();
			setWorldMatrix(&building->matrix);
			setupVertexInput(building->instHeader);
			setLights(&lights);
			setupDone = true;
		}

		setMaterial(flags, m->color, m->surfaceProps);

		setTexture(0, m->texture);

		drawInst(building->instHeader, inst);
	}
	teardownVertexInput(building->instHeader);
	if(defer)
		numBlendInsts[pass]++;
}

void
AtomicFullyTransparent(RpAtomic *atomic, int pass, int fadeAlpha)
{
	if(numBlendInsts[pass] >= MAXBLENDINSTS)
		return;

	using namespace rw;
	using namespace rw::gl3;

	BuildingInst *building = &blendInsts[pass][numBlendInsts[pass]];

	atomic->getPipeline()->instance(atomic);
	building->instHeader = (gl3::InstanceDataHeader*)atomic->geometry->instData;
	assert(building->instHeader != nil);
	assert(building->instHeader->platform == PLATFORM_GL3);
	building->fadeAlpha = fadeAlpha;
	building->lighting = !!(atomic->geometry->flags & rw::Geometry::LIGHT);
	building->matrix = *atomic->getFrame()->getLTM();
	numBlendInsts[pass]++;
}

void
RenderBlendPass(int pass)
{
	using namespace rw;
	using namespace rw::gl3;

	defaultShader->use();
	WorldLights lights;
	lights.numAmbients = 1;
	lights.numDirectionals = 0;
	lights.numLocals = 0;

	int i;
	for(i = 0; i < numBlendInsts[pass]; i++){
		BuildingInst *building = &blendInsts[pass][i];

		setupVertexInput(building->instHeader);
		setWorldMatrix(&building->matrix);
		if(building->lighting)
			lights.ambient = pAmbient->color;
		else
			lights.ambient = black;
		setLights(&lights);

		InstanceData *inst = building->instHeader->inst;
		for(rw::uint32 j = 0; j < building->instHeader->numMeshes; j++, inst++){
			Material *m = inst->material;
			if(!inst->vertexAlpha && m->color.alpha == 255 && !IsTextureTransparent(m->texture) && building->fadeAlpha == 255)
				continue;	// already done this one

			rw::RGBA color = m->color;
			color.alpha = (color.alpha * building->fadeAlpha)/255;
			setMaterial(color, m->surfaceProps);	// always modulate here

			setTexture(0, m->texture);

			drawInst(building->instHeader, inst);
		}
		teardownVertexInput(building->instHeader);
	}
}
}
#endif

#endif
