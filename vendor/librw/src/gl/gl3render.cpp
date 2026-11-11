#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

namespace rw {
namespace gl3 {

#define MAX_LIGHTS 

void
drawInst_simple(InstanceDataHeader *header, InstanceData *inst)
{
	flushCache();
	glDrawElements(header->primType, inst->numIndex,
	               GL_UNSIGNED_SHORT, (void*)(uintptr)inst->offset);
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

		if((vsBits & VSLIGHT_MASK) == 0){
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

void
defaultRenderCB(Atomic *atomic, InstanceDataHeader *header)
{
	defaultRenderCBImpl(atomic, header, false);
}

void
defaultRenderCBDualPass(Atomic *atomic, InstanceDataHeader *header)
{
	defaultRenderCBImpl(atomic, header, true);
}


}
}

#endif

