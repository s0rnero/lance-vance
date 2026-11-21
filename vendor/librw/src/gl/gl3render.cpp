#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "../rwbase.h"
#include "../rwerror.h"
#include "../rwplg.h"
#include "../rwrender.h"
#include "../rwengine.h"
#include "../rwpipeline.h"
#include "../rwobjects.h"
#ifdef RW_OPENGL
#include "rwgl3.h"
#include "rwgl3shader.h"

#include "rwgl3impl.h"

int gRwDrawInstances = 0;
// 85.1: instancing por geometria. gOdInstCount > 1 hace que drawInst_simple
// emita glDrawElementsInstanced; gOdInstBatches/Instances son la traza de que
// el lote entro de verdad.
int gOdInstCount = 0;
int gOdInstBatches = 0;
int gOdInstInstances = 0;

namespace rw {
namespace gl3 {

#define MAX_LIGHTS 

void
drawInst_simple(InstanceDataHeader *header, InstanceData *inst)
{
	flushCache();
	if(::gOdInstCount > 1){
		glDrawElementsInstanced(header->primType, inst->numIndex,
		                        GL_UNSIGNED_SHORT, (void*)(uintptr)inst->offset,
		                        ::gOdInstCount);
		::gOdInstInstances += ::gOdInstCount;
	}else{
		glDrawElements(header->primType, inst->numIndex,
		               GL_UNSIGNED_SHORT, (void*)(uintptr)inst->offset);
		::gOdInstInstances++;
	}
	::gRwDrawInstances++;
}

// Emulate PS2 GS alpha test FB_ONLY case: failed alpha writes to frame- but not to depth buffer
void
drawInst_GSemu(InstanceDataHeader *header, InstanceData *inst)
{
	uint32 hasAlpha;
	int alphafunc, alpharef, gsalpharef;
	int zwrite;
	hasAlpha = getAlphaBlend();
	if(hasAlpha){
		zwrite = rw::GetRenderState(rw::ZWRITEENABLE);
		alphafunc = rw::GetRenderState(rw::ALPHATESTFUNC);
		if(zwrite){
			alpharef = rw::GetRenderState(rw::ALPHATESTREF);
			gsalpharef = rw::GetRenderState(rw::GSALPHATESTREF);

			SetRenderState(rw::ALPHATESTFUNC, rw::ALPHAGREATEREQUAL);
			SetRenderState(rw::ALPHATESTREF, gsalpharef);
			drawInst_simple(header, inst);
			SetRenderState(rw::ALPHATESTFUNC, rw::ALPHALESS);
			SetRenderState(rw::ZWRITEENABLE, 0);
			drawInst_simple(header, inst);
			SetRenderState(rw::ZWRITEENABLE, 1);
			SetRenderState(rw::ALPHATESTFUNC, alphafunc);
			SetRenderState(rw::ALPHATESTREF, alpharef);
		}else{
			SetRenderState(rw::ALPHATESTFUNC, rw::ALPHAALWAYS);
			drawInst_simple(header, inst);
			SetRenderState(rw::ALPHATESTFUNC, alphafunc);
		}
	}else
		drawInst_simple(header, inst);
}

void
drawInst(InstanceDataHeader *header, InstanceData *inst)
{
	if(rw::GetRenderState(rw::GSALPHATEST))
		drawInst_GSemu(header, inst);
	else
		drawInst_simple(header, inst);
}

// SkyGFX `dualPass`: parte el material transparente en dos draws por el alpha
// test (>=128 escribe profundidad, <128 no) en vez de dibujarlo una sola vez.
// Se usa solo desde el mundo; los vehiculos siguen por `defaultRenderCB`, que
// conserva el arreglo de profundidad del cristal.
void
drawInstDualPass(InstanceDataHeader *header, InstanceData *inst)
{
	uint32 alphafunc = rw::GetRenderState(rw::ALPHATESTFUNC);
	uint32 alpharef = rw::GetRenderState(rw::ALPHATESTREF);
	uint32 gsalphatest = rw::GetRenderState(rw::GSALPHATEST);

	rw::SetRenderState(rw::GSALPHATEST, 0);

	rw::SetRenderState(rw::ALPHATESTFUNC, rw::ALPHAGREATEREQUAL);
	rw::SetRenderState(rw::ALPHATESTREF, 128);
	drawInst_simple(header, inst);

	rw::SetRenderState(rw::ALPHATESTFUNC, rw::ALPHALESS);
	rw::SetRenderState(rw::ALPHATESTREF, 128);
	rw::SetRenderState(rw::ZWRITEENABLE, 0);
	drawInst_simple(header, inst);

	rw::SetRenderState(rw::ZWRITEENABLE, 1);
	rw::SetRenderState(rw::ALPHATESTFUNC, alphafunc);
	rw::SetRenderState(rw::ALPHATESTREF, alpharef);
	rw::SetRenderState(rw::GSALPHATEST, gsalphatest);
}


#define MAX_ATTRIB_INDEX (ATTRIB_TEXCOORDS7 + 1)

static bool
validAttribHeader(AttribDesc *attribDescs, int32 numAttribs)
{
	if(attribDescs == nil || numAttribs <= 0 || numAttribs > MAX_ATTRIB_INDEX){
#ifdef __EMSCRIPTEN__
		static int reported = 0;
		if(reported < 8){
			reported++;
			printf("[od] cabecera de atributos invalida: descs=%p num=%d\n", (void*)attribDescs, (int)numAttribs);
		}
#endif
		return false;
	}
	return true;
}

void
setAttribPointers(AttribDesc *attribDescs, int32 numAttribs)
{
	AttribDesc *a;
	if(!validAttribHeader(attribDescs, numAttribs))
		return;
	for(a = attribDescs; a != &attribDescs[numAttribs]; a++){
		if(a->index < ATTRIB_POS || a->index >= MAX_ATTRIB_INDEX){
#ifdef __EMSCRIPTEN__
			static int reported = 0;
			if(reported < 8){
				reported++;
				printf("[od] atributo fuera de rango: index=%d num=%d\n", (int)a->index, (int)numAttribs);
			}
#endif
			continue;
		}
		glEnableVertexAttribArray(a->index);
		glVertexAttribPointer(a->index, a->size, a->type, a->normalized,
		                      a->stride, (void*)(uint64)a->offset);
	}
}

void
disableAttribPointers(AttribDesc *attribDescs, int32 numAttribs)
{
	AttribDesc *a;
	if(!validAttribHeader(attribDescs, numAttribs))
		return;
	for(a = attribDescs; a != &attribDescs[numAttribs]; a++)
		if(a->index >= ATTRIB_POS && a->index < MAX_ATTRIB_INDEX)
			glDisableVertexAttribArray(a->index);
}

void
setupVertexInput(InstanceDataHeader *header)
{
#ifdef RW_GL_USE_VAOS
	glBindVertexArray(header->vao);
#else
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, header->ibo);
	glBindBuffer(GL_ARRAY_BUFFER, header->vbo);
	setAttribPointers(header->attribDesc, header->numAttribs);
#endif
}

void
teardownVertexInput(InstanceDataHeader *header)
{
#ifndef RW_GL_USE_VAOS
	disableAttribPointers(header->attribDesc, header->numAttribs);
#endif
}

int32
lightingCB(Atomic *atomic)
{
	WorldLights lightData;
	Light *directionals[8];
	Light *locals[8];
	lightData.directionals = directionals;
	lightData.numDirectionals = 8;
	lightData.locals = locals;
	lightData.numLocals = 8;

	if(atomic->geometry->flags & rw::Geometry::LIGHT){
		((World*)engine->currentWorld)->enumerateLights(atomic, &lightData);
		if((atomic->geometry->flags & rw::Geometry::NORMALS) == 0){
			// Get rid of lights that need normals when we don't have any
			lightData.numDirectionals = 0;
			lightData.numLocals = 0;
		}
		return setLights(&lightData);
	}else{
		memset(&lightData, 0, sizeof(lightData));
		return setLights(&lightData);
	}
}

// 24/10 (sexta): un material pensado para MEZCLARSE (cristal de vehiculo: alfa
// 76/89/153 en el DFF) no puede escribir profundidad. Lo escribia, y con eso
// borraba todo lo que se dibuja DESPUES y esta DETRAS suyo: el parabrisas
// (`windscreen_hi_ok`) y las lunas de puerta (`door_*_hi_ok`) se dibujan como
// atomics aparte, y el habitaculo del coche (asientos, salpicadero, paneles
// interiores, volante) es opaco y vive en `chassis_hi`, que entra en el pase
// transparente igual que ellas. Si la luna se resuelve primero, el interior
// falla el z-test y desde fuera se ve la calle a traves de la ventanilla; si se
// resuelve despues, se ve el interior. De ahi que el sintoma cambiara con el
// angulo, que no se viera el volante por la ventanilla pero si al abrir la
// puerta, y que ninguna regla por atomic lo arreglara: el reparto es por
// MATERIAL y ese es el unico sitio donde se puede decidir.
// 24/10 (septima): el criterio es "el material se mezcla", no un umbral. El
// cristal roto (`windscreen_hi_dam`, `door_*_dam`) viene con alfa 250, y con un
// umbral de 200 volvia a escribir profundidad: el habitaculo desaparecia otra
// vez en cuanto una luna estaba dañada y seguia puesta. Cualquier material con
// alfa distinto de 255 se dibuja mezclado (es el mismo criterio con el que
// librw enciende el blend en `setVertexAlpha`), asi que ninguno escribe
// profundidad. La chapa intacta (255) sigue escribiendo como siempre.

static void
defaultRenderCBImpl(Atomic *atomic, InstanceDataHeader *header, bool32 dualPass)
{
	Material *m;

	uint32 flags = atomic->geometry->flags;
	setWorldMatrix(atomic->getFrame()->getLTM());
	int32 vsBits = lightingCB(atomic);

	// Estado con el que llega el dibujo (`CSimpleModelInfo::m_noZwrite`, fundidos
	// de aparicion, etc.). Lo opaco lo respeta tal cual; lo que se mezcla se
	// aparta de el y al final se devuelve.
	bool32 odBaseZwrite = rw::GetRenderState(rw::ZWRITEENABLE) != 0;

	setupVertexInput(header);

	InstanceData *inst = header->inst;
	int32 n = header->numMeshes;

	int32 i = 0;
	while(i < n){
		InstanceData *inst0 = inst + i;
		int32 j = i;
		if(header->primType == GL_TRIANGLES){
			while(j + 1 < n){
				InstanceData *a = inst + j;
				InstanceData *b = inst + j + 1;
				if(b->material != a->material) break;
				if(b->vertexAlpha != a->vertexAlpha) break;
				if(b->offset != a->offset + a->numIndex * 2) break;
				j++;
			}
		}
		m = inst0->material;

		setMaterial(flags, m->color, m->surfaceProps);

		setTexture(0, m->texture);

		rw::SetRenderState(VERTEXALPHA, inst0->vertexAlpha || m->color.alpha != 0xFF);

		// En dualPass el material transparente conserva la escritura de
		// profundidad: la primera pasada (alfa >= 128) la usa y la segunda
		// (alfa < 128) la apaga dentro de `drawInstDualPass`. El arreglo del
		// cristal (alfa != 255 => sin ZWRITE) queda solo en la ruta de vehiculos.
		if(dualPass)
			rw::SetRenderState(rw::ZWRITEENABLE, odBaseZwrite);
		else
			rw::SetRenderState(rw::ZWRITEENABLE, m->color.alpha != 0xFF ? 0 : odBaseZwrite);

#ifdef __EMSCRIPTEN__
		odMatTrace(m, inst0);
#endif

		if(::gOdInstCount > 1){
			// 85.1: variante que toma la matriz del mundo del atributo.
			if((vsBits & VSLIGHT_MASK) == 0){
				if(getAlphaTest())
					defaultShader_inst->use();
				else
					defaultShader_noAT_inst->use();
			}else{
				if(getAlphaTest())
					defaultShader_fullLight_inst->use();
				else
					defaultShader_fullLight_noAT_inst->use();
			}
		}else if((vsBits & VSLIGHT_MASK) == 0){
			if(getAlphaTest())
				defaultShader->use();
			else
				defaultShader_noAT->use();
		}else{
			if(getAlphaTest())
				defaultShader_fullLight->use();
			else
				defaultShader_fullLight_noAT->use();
		}

		InstanceData runInst = *inst0;
		uint32 runIndex = 0;
		for(int32 k = i; k <= j; k++)
			runIndex += (inst + k)->numIndex;
		runInst.numIndex = runIndex;
		if(dualPass && getAlphaBlend() && rw::GetRenderState(rw::ZWRITEENABLE))
			drawInstDualPass(header, &runInst);
		else
			drawInst(header, &runInst);
		i = j + 1;
	}
	rw::SetRenderState(rw::ZWRITEENABLE, odBaseZwrite);
	teardownVertexInput(header);
}

// 85.1: instancing por geometria. Los atomics opacos que comparten geometria,
// juego de luces y estado se acumulan y se dibujan con una sola orden
// (glDrawElementsInstanced), con la matriz del mundo como atributo por
// instancia. No se reordena nada que mezcle: solo se acumula lo opaco (que el
// z-buffer resuelve igual) y el lote se vuelca al cambiar de clave o de pase.
#define OD_INST_MAXGROUPS 96
#define OD_INST_MAXPER    256

// Un lote abierto por (geometria, luces, estado). La lista de render va por
// distancia, asi que los mismos modelos no caen seguidos: se acumulan en
// varios lotes y se dibujan al cerrar el pase (PopRendergroup).
struct OdInstGroup {
	uint32 key;
	uint32 zw;
	uint32 cull;
	int32  n;
	InstanceDataHeader *header;
	Atomic *rep;
};

static bool32 odInstOn = 1;
static int32  odInstGroupN = 0;
static OdInstGroup odInstGroups[OD_INST_MAXGROUPS];
static float  odInstPool[OD_INST_MAXGROUPS][OD_INST_MAXPER*16];
static GLuint odInstVbo = 0;
static int32  odInstVboBytes = 0;

void
odSetInstancing(bool32 on)
{
#ifdef __EMSCRIPTEN__
	// Escape sin recompilar: `window.__vcInstancing = 0` + recargar deja el
	// camino de siempre. Sirve para separar un fallo de instancing de uno del
	// juego en una sesion en el movil.
	on = on && EM_ASM_INT({ return (typeof window.__vcInstancing === 'undefined' ||
		window.__vcInstancing) ? 1 : 0; });
#endif
	odInstOn = on;
#ifdef __EMSCRIPTEN__
	printf("[web] instancing=%d\n", (int)on);
#endif
}

static uint32
odInstHash(uint32 h, uint32 v)
{
	return (h ^ v) * 16777619u;
}

static bool32
odInstCanBatch(Atomic *atomic, InstanceDataHeader *header)
{
	int32 i;
#ifndef RW_GL_USE_VAOS
	// El atributo de instancia se cuelga del VAO de la geometria.
	return 0;
#endif
	if(!odInstOn || header == nil || header->inst == nil)
		return 0;
	if(header->primType != GL_TRIANGLES || header->numMeshes <= 0)
		return 0;
	// Solo opaco: lo que mezcla o se desvanece conserva su orden.
	if(rw::GetRenderState(rw::GSALPHATEST))
		return 0;
	if(rw::GetRenderState(rw::ZWRITEENABLE) == 0)
		return 0;
	for(i = 0; i < (int32)header->numMeshes; i++){
		Material *m = header->inst[i].material;
		if(m == nil || m->color.alpha != 0xFF)
			return 0;
		if(header->inst[i].vertexAlpha)
			return 0;
	}
	return 1;
}

static void odInstDrawGroup(OdInstGroup *g);
static void odInstFlushAll(void);

static bool32
odInstSubmit(Atomic *atomic, InstanceDataHeader *header)
{
	Matrix *ltm;
	uint32 key, zw, cull;
	int32 vsBits, i, gi;
	OdInstGroup *g;

	if(!odInstCanBatch(atomic, header))
		return 0;

	ltm = atomic->getFrame()->getLTM();
	zw = rw::GetRenderState(rw::ZWRITEENABLE) != 0;
	cull = rw::GetRenderState(rw::CULLMODE);
	// Las luces son por atomic (posicion). Solo se juntan los que reciben
	// exactamente el mismo juego de luces; si no, se veria distinto.
	vsBits = lightingCB(atomic);
	key = odInstHash((uint32)(uintptr)header, odLightsHash());
	key = odInstHash(key, (uint32)vsBits);
	key = odInstHash(key, zw);
	key = odInstHash(key, cull);

	g = nil;
	for(i = 0; i < odInstGroupN; i++)
		if(odInstGroups[i].key == key){
			g = &odInstGroups[i];
			break;
		}
	if(g == nil){
		if(odInstGroupN >= OD_INST_MAXGROUPS)
			odInstFlushAll();
		g = &odInstGroups[odInstGroupN++];
		g->key = key;
		g->zw = zw;
		g->cull = cull;
		g->n = 0;
		g->header = header;
		g->rep = atomic;
	}
	gi = (int32)(g - odInstGroups);
	if(g->n >= OD_INST_MAXPER){
		odInstDrawGroup(g);
		g->n = 0;
		g->rep = atomic;
	}
	convMatrix((RawMatrix*)&odInstPool[gi][g->n*16], ltm);
	g->n++;
	return 1;
}

static void
odInstDrawGroup(OdInstGroup *g)
{
	int32 i, n, bytes, gi;
	InstanceDataHeader *header;
	Atomic *rep;
	uint32 zw, cull;

	if(g == nil || g->n == 0)
		return;
	n = g->n;
	header = g->header;
	rep = g->rep;
	zw = g->zw;
	cull = g->cull;
	gi = (int32)(g - odInstGroups);
	g->n = 0;
	g->rep = nil;

	// El estado del pase pudo cambiar entre el envio y el volcado.
	rw::SetRenderState(rw::ZWRITEENABLE, zw);
	rw::SetRenderState(rw::CULLMODE, cull);
	rw::SetRenderState(rw::GSALPHATEST, 0);

	if(n > 1){
		if(odInstVbo == 0)
			glGenBuffers(1, &odInstVbo);
		glBindBuffer(GL_ARRAY_BUFFER, odInstVbo);
		bytes = n*16*4;
		if(bytes > odInstVboBytes){
			odInstVboBytes = bytes < 4096 ? 4096 : bytes;
			glBufferData(GL_ARRAY_BUFFER, odInstVboBytes, nil, GL_STREAM_DRAW);
		}
		glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, odInstPool[gi]);
#ifdef RW_GL_USE_VAOS
		glBindVertexArray(header->vao);
		for(i = 0; i < 4; i++){
			GLuint loc = ATTRIB_INSTANCE0 + i;
			glEnableVertexAttribArray(loc);
			glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 64,
			                      (void*)(uintptr)(i*16));
			glVertexAttribDivisor(loc, 1);
		}
#endif
		::gOdInstCount = n;
		::gOdInstBatches++;
		defaultRenderCBImpl(rep, header, false);
		::gOdInstCount = 0;
#ifdef RW_GL_USE_VAOS
		for(i = 0; i < 4; i++){
			glVertexAttribDivisor(ATTRIB_INSTANCE0 + i, 0);
			glDisableVertexAttribArray(ATTRIB_INSTANCE0 + i);
		}
#endif
	}else{
		defaultRenderCBImpl(rep, header, false);
	}
}

static void
odInstFlushAll(void)
{
	int32 i;
	for(i = 0; i < odInstGroupN; i++)
		odInstDrawGroup(&odInstGroups[i]);
	odInstGroupN = 0;
}

void
odFlushPending(void)
{
	odInstFlushAll();
}

void
defaultRenderCB(Atomic *atomic, InstanceDataHeader *header)
{
	if(odInstSubmit(atomic, header))
		return;
	// Cualquier dibujo que no se pueda acumular (transparente, otro estado)
	// conserva el orden: se vuelca el lote antes de dibujarlo.
	odInstFlushAll();
	defaultRenderCBImpl(atomic, header, false);
}

void
defaultRenderCBDualPass(Atomic *atomic, InstanceDataHeader *header)
{
	odInstFlushAll();
	defaultRenderCBImpl(atomic, header, true);
}


}
}

// Fuera de namespace: el juego la llama al cerrar cada pase (PopRendergroup).
void
OdInstanceFlush(void)
{
	rw::gl3::odFlushPending();
}

#endif

