#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../rwbase.h"
#include "../rwerror.h"
#include "../rwplg.h"
#include "../rwrender.h"
#include "../rwengine.h"
#include "../rwpipeline.h"
#include "../rwobjects.h"
#include "../rwanim.h"
#include "../rwplugins.h"

#include "rwgl3.h"
#include "rwgl3shader.h"
#include "rwgl3plg.h"

#include "rwgl3impl.h"

namespace rw {
namespace gl3 {

#ifdef RW_OPENGL

// PORTADO -- SkyGfx (MIT, aap "The Hero")
//   https://github.com/aap/skygfx_vc
//   src/matfx.cpp:584  ("_rpMatFXD3D8AtomicMatFXEnvRender_switch_shader")
// Que se toma: los tres estilos de mezcla del env map (0 = PS2, 1 = PC, 2 = Mobile) y el modelo de luz PS2. Adaptacion: el motor escribe MatFX::envMapBlendStyle, MatFX::envMapTexgenStyle y MatFX::ps2Lighting; librw elige con ellos el fragment shader (un .frag por estilo) y el vertex shader (PS2 o PC) de cada material.
enum {
	MATFX_STYLE_PS2 = 0,
	MATFX_STYLE_PC,
	MATFX_STYLE_MOBILE,
	MATFX_STYLE_COUNT
};

enum {
	MATFX_LIGHT_NOLIGHT = 0,
	MATFX_LIGHT_FULL,
	MATFX_LIGHT_COUNT
};

enum {
	MATFX_ALPHA_NOAT = 0,
	MATFX_ALPHA_AT,
	MATFX_ALPHA_COUNT
};

static Shader *envShader[MATFX_STYLE_COUNT][MATFX_LIGHT_COUNT][MATFX_ALPHA_COUNT];
static Shader *envShaderPCLight[MATFX_STYLE_COUNT][MATFX_ALPHA_COUNT];
static Shader *defaultShaderPCLight[MATFX_ALPHA_COUNT];
static int32 u_texMatrix;
static int32 u_fxparams;
static int32 u_colorClamp;
static int32 u_envColor;

// PORTADO -- SkyGfx (MIT, aap "The Hero")
//   https://github.com/aap/skygfx_vc
//   src/matfx.cpp:435  ("_rpMatFXD3D8AtomicMatFXDefaultRender_shader")
// Que se toma: el vertex shader de MatFX es el PS2 (ps2StandardVS) o el PC (pcStandardVS) segun ps2light, y el del env map es el PS2 o el PC segun el mismo interruptor. Adaptacion: el caso PS2 es el que ya hay (default.vert y matfx_env.vert, con el color de material despues del clamp), asi que el caso PC son dos vertex shader nuevos; como en el mod, el modelo PC solo se aplica a la geometria con luces, y la que no tiene luces sigue usando el color de vertice.
static Shader *
matfxDefaultShader(int32 vsBits, bool32 alphaTest)
{
	if((vsBits & VSLIGHT_MASK) == 0)
		return alphaTest ? defaultShader : defaultShader_noAT;
	if(!MatFX::ps2Lighting)
		return defaultShaderPCLight[alphaTest ? MATFX_ALPHA_AT : MATFX_ALPHA_NOAT];
	return alphaTest ? defaultShader_fullLight : defaultShader_fullLight_noAT;
}

static Shader *
matfxEnvShader(int32 style, int32 vsBits, bool32 alphaTest)
{
	if((vsBits & VSLIGHT_MASK) == 0)
		return envShader[style][MATFX_LIGHT_NOLIGHT][alphaTest ? MATFX_ALPHA_AT : MATFX_ALPHA_NOAT];
	if(!MatFX::ps2Lighting)
		return envShaderPCLight[style][alphaTest ? MATFX_ALPHA_AT : MATFX_ALPHA_NOAT];
	return envShader[style][MATFX_LIGHT_FULL][alphaTest ? MATFX_ALPHA_AT : MATFX_ALPHA_NOAT];
}

static bool32 odMatfxBaseZwrite = true;

void
matfxDefaultRender(InstanceDataHeader *header, InstanceData *inst, int32 vsBits, uint32 flags)
{
	Material *m;
	m = inst->material;

	setMaterial(flags, m->color, m->surfaceProps);

	setTexture(0, m->texture);

	rw::SetRenderState(VERTEXALPHA, inst->vertexAlpha || m->color.alpha != 0xFF);

#ifdef __EMSCRIPTEN__
	odMatTrace(m, inst);
#endif

	// 24/10 (sexta): misma regla que `defaultRenderCB` (gl3render.cpp): lo que se
	// mezcla (cristal del coche) no escribe profundidad, o borra el habitaculo
	// que se dibuja detras. Con el pase de env map hay que respetarla igual, o el
	// coche se comporta distinto segun el pipeline activo.
	rw::SetRenderState(rw::ZWRITEENABLE, m->color.alpha != 0xFF ? 0 : odMatfxBaseZwrite);

	matfxDefaultShader(vsBits, getAlphaTest())->use();

	drawInst(header, inst);
}

static Frame *lastEnvFrame;

static RawMatrix normal2texcoord = {
	{ 0.5f,  0.0f, 0.0f }, 0.0f,
	{ 0.0f, -0.5f, 0.0f }, 0.0f,
	{ 0.0f,  0.0f, 1.0f }, 0.0f,
	{ 0.5f,  0.5f, 0.0f }, 1.0f
};

void
uploadEnvMatrix(Frame *frame)
{
	Matrix invMat;
	if(frame == nil)
		frame = engine->currentCamera->getFrame();

	// cache the matrix across multiple meshes
	static RawMatrix envMtx;
// can't do it, frame matrix may change
//	if(frame != lastEnvFrame){
//		lastEnvFrame = frame;
	{

		RawMatrix invMtx;
		RawMatrix scaleMtx = normal2texcoord;
		float uscale = fabs(normal2texcoord.right.x);
		scaleMtx.right.x = MatFX::envMapFlipU ? -uscale : uscale;

// PORTADO -- SkyGfx (MIT, aap "The Hero")
//   https://github.com/aap/skygfx_vc
//   src/matfx.cpp:375  ("ApplyEnvMapTextureMatrix_world")
// Que se toma: las tres matrices de UV del env map (PS2 = la inversa del LTM del frame del env map con la U negada; PC = esa inversa por el LTM de la camara y luego la matriz de vista; Mobile = el LTM de la camara con el origen en 0,5). Adaptacion: la matriz se sigue calculando en CPU y se sube por u_texMatrix; el signo de U lo sigue mandando MatFX::envMapFlipU, como en el resto del port.
		switch(MatFX::envMapTexgenStyle){
		case MATFX_STYLE_PC:
			{
				RawMatrix camMtx, tmpmat;
				Matrix::invert(&invMat, frame->getLTM());
				convMatrix(&invMtx, &invMat);
				convMatrix(&camMtx, engine->currentCamera->getFrame()->getLTM());
				RawMatrix::mult(&tmpmat, &invMtx, &camMtx);
				tmpmat.right.x = -tmpmat.right.x;
				tmpmat.right.y = -tmpmat.right.y;
				tmpmat.right.z = -tmpmat.right.z;
				tmpmat.pos.set(0.0f, 0.0f, 0.0f);
				RawMatrix::mult(&envMtx, &tmpmat, &scaleMtx);
				Matrix::invert(&invMat, engine->currentCamera->getFrame()->getLTM());
				convMatrix(&invMtx, &invMat);
				invMtx.right.x = -invMtx.right.x;
				invMtx.up.x = -invMtx.up.x;
				invMtx.at.x = -invMtx.at.x;
				invMtx.pos.set(0.0f, 0.0f, 0.0f);
				RawMatrix::mult(&tmpmat, &invMtx, &envMtx);
				envMtx = tmpmat;
			}
			break;
		case MATFX_STYLE_MOBILE:
			{
				RawMatrix camMtx;
				convMatrix(&camMtx, engine->currentCamera->getFrame()->getLTM());
				camMtx.pos.set(0.5f, 0.5f, 0.0f);
				RawMatrix::mult(&envMtx, &camMtx, &scaleMtx);
			}
			break;
		default:
			{
				Matrix::invert(&invMat, frame->getLTM());
				convMatrix(&invMtx, &invMat);
				invMtx.pos.set(0.0f, 0.0f, 0.0f);
				RawMatrix::mult(&envMtx, &invMtx, &scaleMtx);
			}
			break;
		}
	}
	setUniform(u_texMatrix, &envMtx);
}

void
matfxEnvRender(InstanceDataHeader *header, InstanceData *inst, int32 vsBits, uint32 flags, MatFX::Env *env)
{
	Material *m;
	m = inst->material;

	if(env->tex == nil || env->coefficient == 0.0f){
		matfxDefaultRender(header, inst, vsBits, flags);
		return;
	}

	int32 style = MatFX::envMapBlendStyle;
	if(style < 0 || style >= MATFX_STYLE_COUNT)
		style = MATFX_STYLE_PS2;

	setTexture(0, m->texture);
	setTexture(1, env->tex);
	uploadEnvMatrix(env->frame);

	setMaterial(flags, m->color, m->surfaceProps);

// PORTADO -- SkyGfx (MIT, aap "The Hero")
//   https://github.com/aap/skygfx_vc
//   src/matfx.cpp:584  ("_rpMatFXD3D8AtomicMatFXEnvRender_switch_shader")
// Que se toma: el coeficiente del env map se escala por estilo de mezcla (PC: x0,25 en los coches; Mobile: 0,15 en los coches y x1,45 siempre). Adaptacion: el escalado se aplica al coeficiente que ya se sube en u_fxparams; "coche" se deduce del mismo flag de framebuffer alpha que usa el mod (envFBalpha == 0).
	float coeff = env->coefficient;
	if(style == MATFX_STYLE_PC){
		if(!env->fbAlpha)
			coeff *= 0.25f;
	}else if(style == MATFX_STYLE_MOBILE){
		if(!env->fbAlpha)
			coeff = 0.15f;
		coeff *= 1.45f;
	}

	float fxparams[4];
	fxparams[0] = coeff;
	fxparams[1] = env->fbAlpha ? 0.0f : 1.0f;
	fxparams[2] = fxparams[3] = 0.0f;

	setUniform(u_fxparams, fxparams);
	static float zero[4];
	static float one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	// This clamps the vertex color below. With it we can achieve both PC and PS2 style matfx
	if(MatFX::envMapApplyLight)
		setUniform(u_colorClamp, zero);
	else
		setUniform(u_colorClamp, one);
	RGBAf envcol[4];
	if(MatFX::envMapUseMatColor)
		convColor(envcol, &m->color);
	else
		convColor(envcol, &MatFX::envMapColor);
	setUniform(u_envColor, envcol);

	rw::SetRenderState(VERTEXALPHA, 1);
	rw::SetRenderState(SRCBLEND, BLENDONE);

	// 24/10 (sexta): el pase de env map vuelve a dibujar los mismos triangulos.
	// Si el cristal escribiera profundidad aqui volveria a borrar el habitaculo
	// del coche que se dibuja detras, exactamente igual que en el pase difuso.
	rw::SetRenderState(rw::ZWRITEENABLE, m->color.alpha != 0xFF ? 0 : odMatfxBaseZwrite);

	matfxEnvShader(style, vsBits, getAlphaTest())->use();

	drawInst(header, inst);

	rw::SetRenderState(SRCBLEND, BLENDSRCALPHA);
}

void
matfxRenderCB(Atomic *atomic, InstanceDataHeader *header)
{
	uint32 flags = atomic->geometry->flags;
	setWorldMatrix(atomic->getFrame()->getLTM());
	int32 vsBits = lightingCB(atomic);

	setupVertexInput(header);

	lastEnvFrame = nil;

	odMatfxBaseZwrite = rw::GetRenderState(rw::ZWRITEENABLE) != 0;

	InstanceData *inst = header->inst;
	int32 n = header->numMeshes;

	while(n--){
		MatFX *matfx = MatFX::get(inst->material);

		if(matfx == nil)
			matfxDefaultRender(header, inst, vsBits, flags);
		else switch(matfx->type){
		case MatFX::ENVMAP:
			matfxEnvRender(header, inst, vsBits, flags, &matfx->fx[0].env);
			break;
		default:
			matfxDefaultRender(header, inst, vsBits, flags);
			break;
		}
		inst++;
	}
	rw::SetRenderState(rw::ZWRITEENABLE, odMatfxBaseZwrite);
	teardownVertexInput(header);
}

ObjPipeline*
makeMatFXPipeline(void)
{
	ObjPipeline *pipe = ObjPipeline::create();
	pipe->instanceCB = defaultInstanceCB;
	pipe->uninstanceCB = defaultUninstanceCB;
	pipe->renderCB = matfxRenderCB;
	pipe->pluginID = ID_MATFX;
	pipe->pluginData = 0;
	return pipe;
}

static void*
matfxOpen(void *o, int32, int32)
{
	matFXGlobals.pipelines[PLATFORM_GL3] = makeMatFXPipeline();

#include "shaders/matfx_gl.inc"
#include "shaders/simple_fs_gl.inc"
	const char *vs[] = { shaderDecl, header_vert_src, matfx_env_vert_src, nil };
	const char *vs_fullLight[] = { shaderDecl, "#define DIRECTIONALS\n#define POINTLIGHTS\n#define SPOTLIGHTS\n", header_vert_src, matfx_env_vert_src, nil };

// PORTADO -- SkyGfx (MIT, aap "The Hero")
//   https://github.com/aap/skygfx_vc
//   shaders/ps/pcEnvPS.hlsl:12  ("main")
// Que se toma: el env map se suma al color difuso (t0*Color + t1*EnvCoeff). Adaptacion: shaders/ps/pcEnvPS.hlsl pasa a ser vendor/librw/src/gl/shaders/matfx_env_pc.frag, que mezcla el env map en el mismo fragment shader del difuso (librw hace un solo draw) y premultiplica el alfa a mano, porque el pase de env dibuja con SRCBLEND=ONE.
// PORTADO -- SkyGfx (MIT, aap "The Hero")
//   https://github.com/aap/skygfx_vc
//   shaders/ps/mobileEnvPS.hlsl:12  ("main")
// Que se toma: el env map multiplica al color difuso (t0*(Color + t1*EnvCoeff)). Adaptacion: shaders/ps/mobileEnvPS.hlsl pasa a ser vendor/librw/src/gl/shaders/matfx_env_mobile.frag, que mezcla el env map en el mismo fragment shader del difuso (librw hace un solo draw) y premultiplica el alfa a mano, porque el pase de env dibuja con SRCBLEND=ONE.
	const char *fragSrc[MATFX_STYLE_COUNT] = {
		matfx_env_frag_src,
		matfx_env_pc_frag_src,
		matfx_env_mobile_frag_src
	};

// PORTADO -- SkyGfx (MIT, aap "The Hero")
//   https://github.com/aap/skygfx_vc
//   shaders/vs/pcStandardEnvVS.hlsl:40  ("main")
// Que se toma: el color de material se aplica antes del saturado y el alfa sale suelto del color de material (modelo de luz PC). Adaptacion: shaders/vs/pcStandardEnvVS.hlsl pasa a ser vendor/librw/src/gl/shaders/matfx_env_pc.vert; se conserva el color de vertice, porque en el port es la luz horneada del mapa.
	const char *vs_pc[] = { shaderDecl, "#define DIRECTIONALS\n#define POINTLIGHTS\n#define SPOTLIGHTS\n", header_vert_src, matfx_env_pc_vert_src, nil };

	for(int32 style = 0; style < MATFX_STYLE_COUNT; style++){
		const char *fs[] = { shaderDecl, header_frag_src, fragSrc[style], nil };
		const char *fs_noAT[] = { shaderDecl, "#define NO_ALPHATEST\n", header_frag_src, fragSrc[style], nil };

		envShader[style][MATFX_LIGHT_NOLIGHT][MATFX_ALPHA_NOAT] = Shader::create(vs, fs);
		assert(envShader[style][MATFX_LIGHT_NOLIGHT][MATFX_ALPHA_NOAT]);
		envShader[style][MATFX_LIGHT_NOLIGHT][MATFX_ALPHA_AT] = Shader::create(vs, fs_noAT);
		assert(envShader[style][MATFX_LIGHT_NOLIGHT][MATFX_ALPHA_AT]);

		envShader[style][MATFX_LIGHT_FULL][MATFX_ALPHA_NOAT] = Shader::create(vs_fullLight, fs);
		assert(envShader[style][MATFX_LIGHT_FULL][MATFX_ALPHA_NOAT]);
		envShader[style][MATFX_LIGHT_FULL][MATFX_ALPHA_AT] = Shader::create(vs_fullLight, fs_noAT);
		assert(envShader[style][MATFX_LIGHT_FULL][MATFX_ALPHA_AT]);

		envShaderPCLight[style][MATFX_ALPHA_NOAT] = Shader::create(vs_pc, fs);
		assert(envShaderPCLight[style][MATFX_ALPHA_NOAT]);
		envShaderPCLight[style][MATFX_ALPHA_AT] = Shader::create(vs_pc, fs_noAT);
		assert(envShaderPCLight[style][MATFX_ALPHA_AT]);
	}

// PORTADO -- SkyGfx (MIT, aap "The Hero")
//   https://github.com/aap/skygfx_vc
//   shaders/vs/pcStandardVS.hlsl:37  ("main")
// Que se toma: el color de material se aplica antes del saturado y el alfa sale suelto del color de material (modelo de luz PC). Adaptacion: shaders/vs/pcStandardVS.hlsl pasa a ser vendor/librw/src/gl/shaders/matfx_default_pc.vert, que solo usa el pipeline MatFX; se conserva el color de vertice, porque en el port es la luz horneada del mapa.
	const char *vs_default_pc[] = { shaderDecl, "#define DIRECTIONALS\n#define POINTLIGHTS\n#define SPOTLIGHTS\n", header_vert_src, matfx_default_pc_vert_src, nil };
	const char *fs_simple[] = { shaderDecl, header_frag_src, simple_frag_src, nil };
	const char *fs_simple_noAT[] = { shaderDecl, "#define NO_ALPHATEST\n", header_frag_src, simple_frag_src, nil };

	defaultShaderPCLight[MATFX_ALPHA_NOAT] = Shader::create(vs_default_pc, fs_simple);
	assert(defaultShaderPCLight[MATFX_ALPHA_NOAT]);
	defaultShaderPCLight[MATFX_ALPHA_AT] = Shader::create(vs_default_pc, fs_simple_noAT);
	assert(defaultShaderPCLight[MATFX_ALPHA_AT]);

	return o;
}

static void*
matfxClose(void *o, int32, int32)
{
	((ObjPipeline*)matFXGlobals.pipelines[PLATFORM_GL3])->destroy();
	matFXGlobals.pipelines[PLATFORM_GL3] = nil;

	for(int32 style = 0; style < MATFX_STYLE_COUNT; style++){
		for(int32 light = 0; light < MATFX_LIGHT_COUNT; light++){
			for(int32 alpha = 0; alpha < MATFX_ALPHA_COUNT; alpha++){
				envShader[style][light][alpha]->destroy();
				envShader[style][light][alpha] = nil;
			}
		}
		for(int32 alpha = 0; alpha < MATFX_ALPHA_COUNT; alpha++){
			envShaderPCLight[style][alpha]->destroy();
			envShaderPCLight[style][alpha] = nil;
		}
	}
	defaultShaderPCLight[MATFX_ALPHA_NOAT]->destroy();
	defaultShaderPCLight[MATFX_ALPHA_NOAT] = nil;
	defaultShaderPCLight[MATFX_ALPHA_AT]->destroy();
	defaultShaderPCLight[MATFX_ALPHA_AT] = nil;

	return o;
}

void
initMatFX(void)
{
	u_texMatrix = registerUniform("u_texMatrix", UNIFORM_MAT4);
	u_fxparams = registerUniform("u_fxparams", UNIFORM_VEC4);
	u_colorClamp = registerUniform("u_colorClamp", UNIFORM_VEC4);
	u_envColor = registerUniform("u_envColor", UNIFORM_VEC4);

	Driver::registerPlugin(PLATFORM_GL3, 0, ID_MATFX,
	                       matfxOpen, matfxClose);
}

#else

void initMatFX(void) { }

#endif

}

int32 MatFX::envMapBlendStyle = 0;
int32 MatFX::envMapTexgenStyle = 0;
bool32 MatFX::ps2Lighting = true;

}

