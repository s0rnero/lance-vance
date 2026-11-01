#include "common.h"

#include "SteeringWheel.h"
#include "SteeringWheelData.h"
#include "Matrix.h"
#include "NodeName.h"
#include "VisibilityPlugins.h"
#include "VehicleModelInfo.h"
#include "HandlingMgr.h"
#include "ondemand.h"

#define VICEEXT_WHEEL_NOMBRE		"steeringwheel"
#define VICEEXT_WHEEL_DX			0.008f
#define VICEEXT_WHEEL_DY			0.327f
#define VICEEXT_WHEEL_DZ			0.360f
#define VICEEXT_WHEEL_TILT		0.1246f
#define VICEEXT_WHEEL_MIN_ASIENTO	0.25f
#define VICEEXT_WHEEL_RADIO		0.28f
#define VICEEXT_WHEEL_LADO			0.35f
#define VICEEXT_WHEEL_MIN_TRIS		8
#define VICEEXT_WHEEL_MAX_TRIS		100
#define VICEEXT_WHEEL_TAMANO		0.55f
#define VICEEXT_WHEEL_MAX_DESVIO	0.25f

static RwTexture *g_pViceExtWheelTex = nil;

static void
ViceExtWheelTrace(const char *modelName, const char *estado, const CVector *pos, const CVector *asiento, float tilt, int32 quitados)
{
#ifdef __EMSCRIPTEN__
	char odT[240];
	int odRet = snprintf(odT, sizeof odT,
		"VICEEXT wheel modelo=%s estado=%s pos=(%.2f,%.2f,%.2f) asiento=(%.2f,%.2f,%.2f) tilt=%.1f verts=%d tris=%d quitos=%d",
		modelName ? modelName : "?", estado,
		pos ? (double)pos->x : 0.0, pos ? (double)pos->y : 0.0, pos ? (double)pos->z : 0.0,
		asiento ? (double)asiento->x : 0.0, asiento ? (double)asiento->y : 0.0, asiento ? (double)asiento->z : 0.0,
		(double)tilt, VICEEXT_WHEEL_VERTS, VICEEXT_WHEEL_TRIS, (int)quitados);
	if(odRet < 0 || odRet >= (int)sizeof(odT)){
		char odOf[200];
		snprintf(odOf, sizeof odOf,
			"VICEEXT wheel overflow modelo=%s estado=%s cap=%d ret=%d",
			modelName ? modelName : "?", estado, (int)sizeof(odT), odRet);
		ODTRACES(odOf);
	}else{
		ODTRACES(odT);
	}
#else
	(void)modelName; (void)estado; (void)pos; (void)asiento; (void)tilt; (void)quitados;
#endif
}

static RwTexture *ViceExtWheelTexture(void)
{
	if(g_pViceExtWheelTex)
		return g_pViceExtWheelTex;

	RwRaster *ras = RwRasterCreate(VICEEXT_WHEEL_TEX, VICEEXT_WHEEL_TEX, 32,
		rwRASTERTYPETEXTURE | rwRASTERFORMAT8888);
	if(!ras)
		return nil;

	RwUInt32 *pixels = (RwUInt32*)RwRasterLock(ras, 0, rwRASTERLOCKWRITE);
	if(pixels){
		for(int i = 0; i < VICEEXT_WHEEL_TEX*VICEEXT_WHEEL_TEX; i++){
			const uint16 v = gs_auSteeringWheelTex[i];
			const int r = (v >> 11) & 0x1F;
			const int g = (v >> 5) & 0x3F;
			const int b = v & 0x1F;
			pixels[i] = (0xFFu << 24) | (uint32)(b*255/31) << 16 |
				(uint32)(g*255/63) << 8 | (uint32)(r*255/31);
		}
		RwRasterUnlock(ras);
	}

	g_pViceExtWheelTex = RwTextureCreate(ras);

#ifdef __EMSCRIPTEN__
	{
		char odT[160];
		snprintf(odT, sizeof odT, "VICEEXT wheel tex stwheel=%s %dx%d depth=%d fmt=0x%X flags=0x%X",
			g_pViceExtWheelTex ? "ok" : "NIL",
			ras->width, ras->height, ras->depth, ras->format, ras->flags);
		ODTRACES(odT);
	}
#endif

	return g_pViceExtWheelTex;
}

static RpGeometry *ViceExtWheelGeometry(void)
{
	static RpGeometry *s_pGeo = nil;
	if(s_pGeo)
		return s_pGeo;

	RwTexture *tex = ViceExtWheelTexture();
	if(!tex)
		return nil;

	RpGeometry *geo = RpGeometryCreate(VICEEXT_WHEEL_VERTS, VICEEXT_WHEEL_TRIS,
		rpGEOMETRYPOSITIONS | rpGEOMETRYTEXTURED | rpGEOMETRYNORMALS);
	if(!geo)
		return nil;

	for(int i = 0; i < VICEEXT_WHEEL_VERTS; i++){
		RwV3d *p = &geo->morphTargets[0].vertices[i];
		p->x = gs_afSteeringWheel[i][0];
		p->y = gs_afSteeringWheel[i][1];
		p->z = gs_afSteeringWheel[i][2];
		RwV3d *n = &geo->morphTargets[0].normals[i];
		n->x = gs_afSteeringWheel[i][3];
		n->y = gs_afSteeringWheel[i][4];
		n->z = gs_afSteeringWheel[i][5];
		RwTexCoords *uv = &geo->texCoords[0][i];
		uv->u = gs_afSteeringWheel[i][6];
		uv->v = gs_afSteeringWheel[i][7];
	}

	for(int i = 0; i < VICEEXT_WHEEL_TRIS; i++){
		geo->triangles[i].v[0] = gs_auSteeringWheelTris[i][0];
		geo->triangles[i].v[1] = gs_auSteeringWheelTris[i][1];
		geo->triangles[i].v[2] = gs_auSteeringWheelTris[i][2];
		geo->triangles[i].matId = 0;
	}

	RpMaterial *material = RpMaterialCreate();
	RpMaterialSetTexture(material, tex);
	geo->matList.appendMaterial(material);

	RwSphere sphere;
	RpMorphTargetCalcBoundingSphere(&geo->morphTargets[0], &sphere);
	RpMorphTargetSetBoundingSphere(&geo->morphTargets[0], &sphere);

	geo->buildMeshes();
	s_pGeo = geo;
	return geo;
}

static RwFrame *ViceExtFindFrame(RpClump *clump, const char *name)
{
	RwObjectNameAssociation assoc;
	assoc.name = name;
	assoc.frame = nil;
	RwFrameForAllChildren(RpClumpGetFrame(clump), CClumpModelInfo::FindFrameFromNameCB, &assoc);
	return assoc.frame;
}

static RpAtomic *ViceExtFindPipeCB(RpAtomic *atomic, void *data)
{
	RpAtomic **ref = (RpAtomic**)data;
	if(*ref == nil && atomic->pipeline)
		*ref = atomic;
	return atomic;
}

struct ViceExtRuedaPintada {
	CVector centro;		// en el espacio del marco raiz del clump
	int32 numTris;
	float lo[3];
	float hi[3];
	bool primero;
	bool quitar;
};

static void ViceExtMatrizMultiplica(RwMatrix *out, const RwMatrix *a, const RwMatrix *b)
{
	RwMatrix tmp = *a;
	tmp.right.x = a->right.x*b->right.x + a->right.y*b->up.x + a->right.z*b->at.x;
	tmp.right.y = a->right.x*b->right.y + a->right.y*b->up.y + a->right.z*b->at.y;
	tmp.right.z = a->right.x*b->right.z + a->right.y*b->up.z + a->right.z*b->at.z;
	tmp.up.x    = a->up.x*b->right.x    + a->up.y*b->up.x    + a->up.z*b->at.x;
	tmp.up.y    = a->up.x*b->right.y    + a->up.y*b->up.y    + a->up.z*b->at.y;
	tmp.up.z    = a->up.x*b->right.z    + a->up.y*b->up.z    + a->up.z*b->at.z;
	tmp.at.x    = a->at.x*b->right.x    + a->at.y*b->up.x    + a->at.z*b->at.x;
	tmp.at.y    = a->at.x*b->right.y    + a->at.y*b->up.y    + a->at.z*b->at.y;
	tmp.at.z    = a->at.x*b->right.z    + a->at.y*b->up.z    + a->at.z*b->at.z;
	tmp.pos.x   = a->pos.x*b->right.x   + a->pos.y*b->up.x   + a->pos.z*b->at.x + b->pos.x;
	tmp.pos.y   = a->pos.x*b->right.y   + a->pos.y*b->up.y   + a->pos.z*b->at.y + b->pos.y;
	tmp.pos.z   = a->pos.x*b->right.z   + a->pos.y*b->up.z   + a->pos.z*b->at.z + b->pos.z;
	*out = tmp;
}

static void ViceExtAcumulaMatriz(RwMatrix *out, RwFrame *frame)
{
	RwMatrix tmp;
	*out = *RwFrameGetMatrix(frame);
	for(RwFrame *padre = RwFrameGetParent(frame); padre; padre = RwFrameGetParent(padre)){
		ViceExtMatrizMultiplica(&tmp, out, RwFrameGetMatrix(padre));
		*out = tmp;
	}
}

static bool ViceExtEsRuedaPintada(const RwV3d *pv, const CVector &centro)
{
	const float cx = (pv[0].x + pv[1].x + pv[2].x) / 3.0f;
	const float cy = (pv[0].y + pv[1].y + pv[2].y) / 3.0f;
	const float cz = (pv[0].z + pv[1].z + pv[2].z) / 3.0f;
	const float dx = cx - centro.x;
	const float dy = cy - centro.y;
	const float dz = cz - centro.z;
	if(dx*dx + dy*dy + dz*dz > VICEEXT_WHEEL_RADIO*VICEEXT_WHEEL_RADIO)
		return false;
	float lado = 0.0f;
	for(int a = 0; a < 3; a++){
		for(int b = a + 1; b < 3; b++){
			const float ex = pv[a].x - pv[b].x;
			const float ey = pv[a].y - pv[b].y;
			const float ez = pv[a].z - pv[b].z;
			const float l = ex*ex + ey*ey + ez*ez;
			if(l > lado)
				lado = l;
		}
	}
	return lado <= VICEEXT_WHEEL_LADO*VICEEXT_WHEEL_LADO;
}

static RpAtomic *ViceExtRuedaPintadaCB(RpAtomic *atomic, void *data)
{
	ViceExtRuedaPintada *r = (ViceExtRuedaPintada*)data;
	RpGeometry *geo = RpAtomicGetGeometry(atomic);
	if(!geo || !geo->triangles || geo->numTriangles <= 0)
		return atomic;
	if(geo->flags & rpGEOMETRYNATIVE)
		return atomic;
	if(geo->numMorphTargets < 1 || !geo->morphTargets[0].vertices)
		return atomic;
	RwFrame *frame = RpAtomicGetFrame(atomic);
	if(!frame)
		return atomic;

	RwMatrix m;
	ViceExtAcumulaMatriz(&m, frame);

	RwV3d *verts = geo->morphTargets[0].vertices;
	RwInt32 quitados = 0;
	RwInt32 quedan = 0;
	for(RwInt32 i = 0; i < geo->numTriangles; i++){
		RwV3d pv[3];
		for(int k = 0; k < 3; k++)
			pv[k] = verts[geo->triangles[i].v[k]];
		RwV3dTransformPoints(pv, pv, 3, &m);
		if(ViceExtEsRuedaPintada(pv, r->centro)){
			quitados++;
			for(int k = 0; k < 3; k++){
				const float c = k == 0 ? pv[k].x : (k == 1 ? pv[k].y : pv[k].z);
				if(r->primero){
					r->lo[k] = c;
					r->hi[k] = c;
				}else{
					if(c < r->lo[k]) r->lo[k] = c;
					if(c > r->hi[k]) r->hi[k] = c;
				}
			}
			if(r->primero)
				r->primero = false;
			continue;
		}
		if(r->quitar)
			geo->triangles[quedan] = geo->triangles[i];
		quedan++;
	}
	if(quitados > 0){
		r->numTris += quitados;
		if(r->quitar){
			geo->numTriangles = quedan;
			geo->flags &= ~rpGEOMETRYTRISTRIP;
			RpGeometryLock(geo, rpGEOMETRYLOCKPOLYGONS);
			RpGeometryUnlock(geo);
		}
	}
	return atomic;
}

static int32 ViceExtBuscaRuedaPintada(RpClump *clump, const CVector &centro, float *lado, CVector *caja)
{
	ViceExtRuedaPintada r;
	r.centro = centro;
	r.numTris = 0;
	r.primero = true;
	r.quitar = false;
	RpClumpForAllAtomics(clump, ViceExtRuedaPintadaCB, &r);
	*lado = 0.0f;
	if(r.numTris <= 0)
		return 0;
	for(int k = 0; k < 3; k++){
		const float l = r.hi[k] - r.lo[k];
		if(l > *lado)
			*lado = l;
	}
	if(caja)
		*caja = CVector((r.lo[0] + r.hi[0]) * 0.5f, (r.lo[1] + r.hi[1]) * 0.5f, (r.lo[2] + r.hi[2]) * 0.5f);
	return r.numTris;
}

static void ViceExtQuitaRuedaPintada(RpClump *clump, const CVector &centro)
{
	ViceExtRuedaPintada r;
	r.centro = centro;
	r.numTris = 0;
	r.primero = true;
	r.quitar = true;
	RpClumpForAllAtomics(clump, ViceExtRuedaPintadaCB, &r);
}

RwFrame *ViceExtAddSteeringWheel(RpClump *clump, const char *modelName)
{
	if(!clump)
		return nil;

	if(ViceExtFindFrame(clump, VICEEXT_WHEEL_NOMBRE)){
		ViceExtWheelTrace(modelName, "ya-lo-trae", nil, nil, 0.0f, 0);
		return nil;
	}

	CVehicleModelInfo *vmi = (CVehicleModelInfo*)CVisibilityPlugins::GetClumpModelInfo(clump);
	if(!vmi)
		return nil;

	tHandlingData *handling = mod_HandlingManager.GetHandlingData((tVehicleType)vmi->m_handlingId);
	if(handling && (handling->Flags & (HANDLING_IS_HELI|HANDLING_IS_PLANE|HANDLING_IS_BIKE|HANDLING_IS_BOAT))){
		ViceExtWheelTrace(modelName, "no-es-coche", nil, nil, 0.0f, 0);
		return nil;
	}

	const CVector asiento = vmi->m_positions[CAR_POS_FRONTSEAT];
	if(fabsf(asiento.x) < VICEEXT_WHEEL_MIN_ASIENTO){
		ViceExtWheelTrace(modelName, "sin-asiento", nil, &asiento, 0.0f, 0);
		return nil;
	}

	const CVector centroRueda(-asiento.x + VICEEXT_WHEEL_DX,
		asiento.y + VICEEXT_WHEEL_DY, asiento.z + VICEEXT_WHEEL_DZ);

	RpGeometry *geo = ViceExtWheelGeometry();
	if(!geo){
		ViceExtWheelTrace(modelName, "sin-malla", nil, &asiento, 0.0f, 0);
		return nil;
	}

	float lado = 0.0f;
	CVector cajaPintada(0.0f, 0.0f, 0.0f);
	const int32 hallados = ViceExtBuscaRuedaPintada(clump, centroRueda, &lado, &cajaPintada);
	if(hallados < VICEEXT_WHEEL_MIN_TRIS || hallados > VICEEXT_WHEEL_MAX_TRIS ||
	   lado > VICEEXT_WHEEL_TAMANO){
		ViceExtWheelTrace(modelName, "sin-rueda-pintada", nil, &asiento, 0.0f, hallados);
		return nil;
	}
	ViceExtQuitaRuedaPintada(clump, centroRueda);

	RwFrame *root = RpClumpGetFrame(clump);

	CVector asientoLocal = asiento;
	RwMatrix *rootMat = RwFrameGetMatrix(root);
	if(rootMat){
		const CVector d = asiento - CVector(rootMat->pos);
		asientoLocal.x = d.x*rootMat->right.x + d.y*rootMat->right.y + d.z*rootMat->right.z;
		asientoLocal.y = d.x*rootMat->up.x    + d.y*rootMat->up.y    + d.z*rootMat->up.z;
		asientoLocal.z = d.x*rootMat->at.x    + d.y*rootMat->at.y    + d.z*rootMat->at.z;
	}

	const CVector posRegla(-asientoLocal.x + VICEEXT_WHEEL_DX,
		asientoLocal.y + VICEEXT_WHEEL_DY,
		asientoLocal.z + VICEEXT_WHEEL_DZ);

	CVector pos = posRegla;
	(void)cajaPintada;

	RwFrame *frame = RwFrameCreate();
	if(!frame){
		ViceExtWheelTrace(modelName, "sin-marco", &pos, &asientoLocal, 0.0f, 0);
		return nil;
	}

	char *frameName = GetFrameNodeName(frame);
	if(frameName)
		strncpy(frameName, VICEEXT_WHEEL_NOMBRE, 23);

	RwMatrix *m = RwFrameGetMatrix(frame);
	const float c = cosf(VICEEXT_WHEEL_TILT);
	const float s = sinf(VICEEXT_WHEEL_TILT);
	m->right.x = 1.0f; m->right.y = 0.0f; m->right.z = 0.0f;
	m->up.x    = 0.0f; m->up.y    = c;    m->up.z    = -s;
	m->at.x    = 0.0f; m->at.y    = s;    m->at.z    = c;
	m->pos.x = pos.x;
	m->pos.y = pos.y;
	m->pos.z = pos.z;
	m->update();

	RwFrameAddChild(root, frame);

	RpAtomic *atomic = RpAtomicCreate();
	if(!atomic){
		RwFrameRemoveChild(frame);
		RwFrameDestroy(frame);
		ViceExtWheelTrace(modelName, "sin-atomic", &pos, &asientoLocal, 0.0f, 0);
		return nil;
	}
	RpAtomicSetGeometry(atomic, geo, 0);
	RpAtomicSetFrame(atomic, frame);
	RpClumpAddAtomic(clump, atomic);

	RpAtomic *ref = nil;
	RpClumpForAllAtomics(clump, ViceExtFindPipeCB, &ref);
	if(ref)
		atomic->pipeline = ref->pipeline;

	ViceExtWheelTrace(modelName, "anadida", &pos, &asientoLocal, RADTODEG(VICEEXT_WHEEL_TILT), hallados);
	return frame;
}
