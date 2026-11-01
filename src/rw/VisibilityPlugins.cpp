#include <stdlib.h>

#include "common.h"

#include "RwHelper.h"
#include "ondemand.h" // traza VEHCRISTAL (cristal del coche en el pase transparente)
#include "templates.h"
#include "main.h"
#include "Entity.h"
#include "ModelInfo.h"
#include "Lights.h"
#include "RwHelper.h"
#include "Renderer.h"
#include "Camera.h"
#include "VisibilityPlugins.h"
#include "World.h"
#include "PlayerInfo.h" // traza ODDASH: coche del jugador

#include "custompipes.h"
#include "MemoryHeap.h"
#include "ondemand.h" // PEDALPHA: alfa del clump de los peds en partida
#include "Timer.h"
#include "NodeName.h"

#ifdef __EMSCRIPTEN__
namespace rw { namespace gl3 { void SetOdTraceTag(const char *tag); } }
#define OD_TAG(atomic) rw::gl3::SetOdTraceTag(GetFrameNodeName(RpAtomicGetFrame(atomic)))
#else
#define OD_TAG(atomic)
#endif

CLinkList<CVisibilityPlugins::AlphaObjectInfo> CVisibilityPlugins::m_alphaList;
CLinkList<CVisibilityPlugins::AlphaObjectInfo> CVisibilityPlugins::m_alphaBoatAtomicList;
CLinkList<CVisibilityPlugins::AlphaObjectInfo> CVisibilityPlugins::m_alphaEntityList;
CLinkList<CVisibilityPlugins::AlphaObjectInfo> CVisibilityPlugins::m_alphaUnderwaterEntityList;
#ifdef NEW_RENDERER
CLinkList<CVisibilityPlugins::AlphaObjectInfo> CVisibilityPlugins::m_alphaBuildingList;
#endif

int32 CVisibilityPlugins::ms_atomicPluginOffset = -1;
int32 CVisibilityPlugins::ms_framePluginOffset = -1;
int32 CVisibilityPlugins::ms_clumpPluginOffset = -1;

RwCamera *CVisibilityPlugins::ms_pCamera;
RwV3d *CVisibilityPlugins::ms_pCameraPosn;
float CVisibilityPlugins::ms_cullCompsDist;
float CVisibilityPlugins::ms_vehicleLod0Dist;
float CVisibilityPlugins::ms_vehicleLod1Dist;
float CVisibilityPlugins::ms_vehicleFadeDist;
float CVisibilityPlugins::ms_bigVehicleLod0Dist;
float CVisibilityPlugins::ms_bigVehicleLod1Dist;
float CVisibilityPlugins::ms_pedLod1Dist;
float CVisibilityPlugins::ms_pedFadeDist;

#define RENDERCALLBACK AtomicDefaultRenderCallBack

void
CVisibilityPlugins::Initialise(void)
{
	m_alphaList.Init(NUMALPHALIST);
	m_alphaList.head.item.sort = 0.0f;
	m_alphaList.tail.item.sort = 100000000.0f;

	m_alphaBoatAtomicList.Init(NUMBOATALPHALIST);
	m_alphaBoatAtomicList.head.item.sort = 0.0f;
	m_alphaBoatAtomicList.tail.item.sort = 100000000.0f;

#ifdef ASPECT_RATIO_SCALE
	// default 150 is not enough for bigger FOVs
	m_alphaEntityList.Init(NUMALPHAENTITYLIST * 3);
#else
	m_alphaEntityList.Init(NUMALPHAENTITYLIST);
#endif // ASPECT_RATIO_SCALE
	m_alphaEntityList.head.item.sort = 0.0f;
	m_alphaEntityList.tail.item.sort = 100000000.0f;

	m_alphaUnderwaterEntityList.Init(NUMALPHAUNTERWATERENTITYLIST);
	m_alphaUnderwaterEntityList.head.item.sort = 0.0f;
	m_alphaUnderwaterEntityList.tail.item.sort = 100000000.0f;

#ifdef NEW_RENDERER
	m_alphaBuildingList.Init(NUMALPHAENTITYLIST);
	m_alphaBuildingList.head.item.sort = 0.0f;
	m_alphaBuildingList.tail.item.sort = 100000000.0f;
#endif
}

void
CVisibilityPlugins::Shutdown(void)
{
	m_alphaList.Shutdown();
	m_alphaBoatAtomicList.Shutdown();
	m_alphaEntityList.Shutdown();
	m_alphaUnderwaterEntityList.Shutdown();
#ifdef NEW_RENDERER
	m_alphaBuildingList.Shutdown();
#endif
}

void
CVisibilityPlugins::InitAlphaEntityList(void)
{
	m_alphaEntityList.Clear();
	m_alphaBoatAtomicList.Clear();
	m_alphaUnderwaterEntityList.Clear();
#ifdef NEW_RENDERER
	m_alphaBuildingList.Clear();
#endif
}

bool
CVisibilityPlugins::InsertEntityIntoSortedList(CEntity *e, float dist)
{
#ifdef FIX_BUGS
	if (!e->m_rwObject) return true;
#endif

	AlphaObjectInfo item;
	item.entity = e;
	item.sort = dist;
#ifdef NEW_RENDERER
	if(gbNewRenderer && e->IsBuilding())
		return !!m_alphaBuildingList.InsertSorted(item);
#endif
	if(e->bUnderwater && m_alphaUnderwaterEntityList.InsertSorted(item))
		return true;
	return !!m_alphaEntityList.InsertSorted(item);
}

void
CVisibilityPlugins::InitAlphaAtomicList(void)
{
	m_alphaList.Clear();
}

bool
CVisibilityPlugins::InsertAtomicIntoSortedList(RpAtomic *a, float dist)
{
	AlphaObjectInfo item;
	item.atomic = a;
	item.sort = dist;
	return !!m_alphaList.InsertSorted(item);
}

bool
CVisibilityPlugins::InsertAtomicIntoBoatSortedList(RpAtomic *a, float dist)
{
	AlphaObjectInfo item;
	item.atomic = a;
	item.sort = dist;
	return !!m_alphaBoatAtomicList.InsertSorted(item);
}

// can't increase this yet unfortunately...
// probably have to fix fading for this so material alpha isn't overwritten
#define VEHICLE_LODDIST_MULTIPLIER (TheCamera.GenerationDistMultiplier)

void
CVisibilityPlugins::SetRenderWareCamera(RwCamera *camera)
{
	ms_pCamera = camera;
	ms_pCameraPosn = RwMatrixGetPos(RwFrameGetMatrix(RwCameraGetFrame(camera)));

	if(TheCamera.Cams[TheCamera.ActiveCam].Mode == CCam::MODE_TOPDOWN ||
	   TheCamera.Cams[TheCamera.ActiveCam].Mode == CCam::MODE_TOP_DOWN_PED)
		ms_cullCompsDist = 1000000.0f;
	else
		ms_cullCompsDist = sq(TheCamera.LODDistMultiplier * 20.0f);

#ifdef VICEEXT_IMPROVED_TECH_PART
	ms_vehicleLod0Dist = sq(140.0f * VEHICLE_LODDIST_MULTIPLIER);
	ms_vehicleLod1Dist = sq(160.0f * VEHICLE_LODDIST_MULTIPLIER);
	ms_vehicleFadeDist = sq(170.0f * VEHICLE_LODDIST_MULTIPLIER);
	ms_bigVehicleLod0Dist = sq(130.0f * VEHICLE_LODDIST_MULTIPLIER);
	ms_bigVehicleLod1Dist = sq(220.0f * VEHICLE_LODDIST_MULTIPLIER);
#else
	ms_vehicleLod0Dist = sq(70.0f * VEHICLE_LODDIST_MULTIPLIER);
	ms_vehicleLod1Dist = sq(90.0f * VEHICLE_LODDIST_MULTIPLIER);
	ms_vehicleFadeDist = sq(100.0f * VEHICLE_LODDIST_MULTIPLIER);
	ms_bigVehicleLod0Dist = sq(60.0f * VEHICLE_LODDIST_MULTIPLIER);
	ms_bigVehicleLod1Dist = sq(150.0f * VEHICLE_LODDIST_MULTIPLIER);
#endif
	ms_pedLod1Dist = sq(60.0f * TheCamera.LODDistMultiplier);
	ms_pedFadeDist = sq(70.0f * TheCamera.LODDistMultiplier);
}

static float DistToCameraSq;
static float PitchToCamera;

void
CVisibilityPlugins::SetupVehicleVariables(RpClump *vehicle)
{
	if (RwObjectGetType((RwObject*)vehicle) != rpCLUMP)
		return;
	DistToCameraSq = GetDistanceSquaredFromCamera(RpClumpGetFrame(vehicle));
	RwV3d distToCam;
	RwV3dSub(&distToCam, ms_pCameraPosn, &RwFrameGetMatrix(RpClumpGetFrame(vehicle))->pos);
	float dist2d = Sqrt(SQR(distToCam.x) + SQR(distToCam.y));
	PitchToCamera = Atan2(distToCam.z, dist2d);
}

RpMaterial*
SetAlphaCB(RpMaterial *material, void *data)
{
	((RwRGBA*)RpMaterialGetColor(material))->alpha = (uint8)(uintptr)data;
	return material;
}

struct OdAlphaSave
{
	uint8 *values;
	int capacity;
	int count;
	int saved;
	int idx;
};

static uint8 *s_odAlphaBuffer;
static int s_odAlphaCapacity;

static RpMaterial*
OdAlphaCountCB(RpMaterial *material, void *data)
{
	(void)material;
	++((OdAlphaSave*)data)->count;
	return material;
}

static RpMaterial*
OdAlphaSaveCB(RpMaterial *material, void *data)
{
	OdAlphaSave *state = (OdAlphaSave*)data;
	if(state->saved < state->capacity)
		state->values[state->saved++] = ((RwRGBA*)RpMaterialGetColor(material))->alpha;
	return material;
}

static RpMaterial*
OdAlphaRestoreCB(RpMaterial *material, void *data)
{
	OdAlphaSave *state = (OdAlphaSave*)data;
	if(state->idx < state->saved)
		((RwRGBA*)RpMaterialGetColor(material))->alpha = state->values[state->idx++];
	return material;
}

static bool
OdAlphaBegin(RpGeometry *geo, int alpha, OdAlphaSave *state)
{
	state->values = nil;
	state->capacity = 0;
	state->count = 0;
	state->saved = 0;
	state->idx = 0;
	RpGeometryForAllMaterials(geo, OdAlphaCountCB, state);
	if(state->count <= 0)
		return false;
	if(state->count > s_odAlphaCapacity){
		int capacity = s_odAlphaCapacity ? s_odAlphaCapacity : 16;
		while(capacity < state->count)
			capacity *= 2;
		uint8 *buffer = (uint8*)realloc(s_odAlphaBuffer, capacity);
		if(buffer == nil)
			return false;
		s_odAlphaBuffer = buffer;
		s_odAlphaCapacity = capacity;
	}
	state->values = s_odAlphaBuffer;
	state->capacity = s_odAlphaCapacity;
	RpGeometryForAllMaterials(geo, OdAlphaSaveCB, state);
	RpGeometryForAllMaterials(geo, SetAlphaCB, (void*)alpha);
	return true;
}

static void
OdAlphaEnd(RpGeometry *geo, OdAlphaSave *state)
{
#ifdef __EMSCRIPTEN__
	{
		static bool s_od17Alfa = false;
		if (!s_od17Alfa) {
			s_od17Alfa = true;
			ODTRACES("VEX 17 alfa save");
		}
	}
#endif
	state->idx = 0;
	RpGeometryForAllMaterials(geo, OdAlphaRestoreCB, state);
	state->values = nil;
	state->capacity = 0;
}

RpMaterial*
SetTextureCB(RpMaterial *material, void *data)
{
	RpMaterialSetTexture(material, (RwTexture*)data);
	return material;
}

void
CVisibilityPlugins::RenderAtomicList(CLinkList<AlphaObjectInfo> &list)
{
	CLink<AlphaObjectInfo> *node;
	for(node = list.tail.prev; node != &list.head; node = node->prev){
		OD_TAG(node->item.atomic);
		RENDERCALLBACK(node->item.atomic);
	}
}

void
CVisibilityPlugins::RenderAlphaAtomics(void)
{
	RenderAtomicList(m_alphaList);
}

void
CVisibilityPlugins::RenderBoatAlphaAtomics(void)
{
	SetCullMode(rwCULLMODECULLNONE);
	RenderAtomicList(m_alphaBoatAtomicList);
	SetCullMode(rwCULLMODECULLBACK);
}

void
CVisibilityPlugins::RenderFadingEntities(CLinkList<AlphaObjectInfo> &list)
{
	CLink<AlphaObjectInfo> *node;
	CSimpleModelInfo *mi;
	for(node = list.tail.prev; node != &list.head; node = node->prev){
		CEntity *e = node->item.entity;
		if(e->m_rwObject == nil)
			continue;
#ifdef EXTENDED_PIPELINES
		if(CustomPipes::bRenderingEnvMap && (e->IsPed() || e->IsVehicle()))
			continue;
#endif
		mi = (CSimpleModelInfo *)CModelInfo::GetModelInfo(e->GetModelIndex());
		if(mi->GetModelType() == MITYPE_SIMPLE && mi->m_noZwrite)
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, FALSE);

		bool atomicModel = mi->GetModelType() == MITYPE_SIMPLE || mi->GetModelType() == MITYPE_TIME;
		atomicModel = atomicModel && RwObjectGetType(e->m_rwObject) == rpATOMIC;
		if(e->bDistanceFade && atomicModel){
			DeActivateDirectional();
			SetAmbientColours();
			e->bImBeingRendered = true;
			PUSH_RENDERGROUP(mi->GetModelName());
			RenderFadingAtomic((RpAtomic*)e->m_rwObject, node->item.sort);
			POP_RENDERGROUP();
			e->bImBeingRendered = false;
		}else
			CRenderer::RenderOneNonRoad(e);

		if(mi->GetModelType() == MITYPE_SIMPLE && mi->m_noZwrite)
			RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
	}
}

void
CVisibilityPlugins::RenderFadingEntities(void)
{
	RenderFadingEntities(m_alphaEntityList);
	RenderBoatAlphaAtomics();
}

void
CVisibilityPlugins::RenderFadingUnderwaterEntities(void)
{
	RenderFadingEntities(m_alphaUnderwaterEntityList);
}

RpAtomic*
CVisibilityPlugins::RenderWheelAtomicCB(RpAtomic *atomic)
{
	RpAtomic *lodatm;
	float len;
	CSimpleModelInfo *mi;

	mi = GetAtomicModelInfo(atomic);
	len = Sqrt(DistToCameraSq);
	#ifdef VICEEXT_IMPROVED_TECH_PART
	lodatm = mi->GetAtomicFromDistance(len * TheCamera.LODDistMultiplier / VEHICLE_LODDIST_MULTIPLIER - 315.0f);
#else
	lodatm = mi->GetAtomicFromDistance(len * TheCamera.LODDistMultiplier / VEHICLE_LODDIST_MULTIPLIER);
#endif
	if(lodatm){
		if(RpAtomicGetGeometry(lodatm) != RpAtomicGetGeometry(atomic))
			RpAtomicSetGeometry(atomic, RpAtomicGetGeometry(lodatm), rpATOMICSAMEBOUNDINGSPHERE);
		RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderObjNormalAtomic(RpAtomic *atomic)
{
	RwMatrix *m;
	RwV3d view;
	float len;

	m = RwFrameGetLTM(RpAtomicGetFrame(atomic));
	RwV3dSub(&view, RwMatrixGetPos(m), ms_pCameraPosn);
	len = RwV3dLength(&view);
	if(RwV3dDotProduct(&view, RwMatrixGetUp(m)) < -0.3f*len && len > 8.0f)
		return atomic;
	RENDERCALLBACK(atomic);
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderAlphaAtomic(RpAtomic *atomic, int alpha)
{
	RpGeometry *geo;
	uint32 flags;

	geo = RpAtomicGetGeometry(atomic);
	flags = RpGeometryGetFlags(geo);
	RpGeometrySetFlags(geo, flags | rpGEOMETRYMODULATEMATERIALCOLOR);
	OdAlphaSave state;
	bool odSaveOk = OdAlphaBegin(geo, alpha, &state);
	OD_TAG(atomic);
	RENDERCALLBACK(atomic);
	if(odSaveOk)
		OdAlphaEnd(geo, &state);
	RpGeometrySetFlags(geo, flags);
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderWeaponCB(RpAtomic *atomic)
{
	RwMatrix *m;
	RwV3d view;
	float maxdist, distsq;
	CSimpleModelInfo *mi;

	mi = GetAtomicModelInfo(atomic);
	m = RwFrameGetLTM(RpAtomicGetFrame(atomic));
	RwV3dSub(&view, RwMatrixGetPos(m), ms_pCameraPosn);
	maxdist = mi->GetLodDistance(0);
	distsq = RwV3dDotProduct(&view, &view);
	if(distsq < maxdist*maxdist)
		RENDERCALLBACK(atomic);
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderFadingAtomic(RpAtomic *atomic, float camdist)
{
	RpAtomic *lodatm;
	float fadefactor;
	uint32 alpha;
	CSimpleModelInfo *mi;

	mi = GetAtomicModelInfo(atomic);
	lodatm = mi->GetAtomicFromDistance(camdist - FADE_DISTANCE);
	if(mi->m_additive)
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

	fadefactor = (mi->GetLargestLodDistance() - (camdist - FADE_DISTANCE))/FADE_DISTANCE;
	if(fadefactor > 1.0f)
		fadefactor = 1.0f;
	alpha = mi->m_alpha * fadefactor;
	OD_TAG(atomic);
	if(alpha == 255)
		RENDERCALLBACK(atomic);
	else{
		RpGeometry *geo = RpAtomicGetGeometry(lodatm);
		uint32 flags = RpGeometryGetFlags(geo);
		RpGeometrySetFlags(geo, flags | rpGEOMETRYMODULATEMATERIALCOLOR);
		OdAlphaSave state;
		bool odSaveOk = OdAlphaBegin(geo, alpha, &state);
		if(geo != RpAtomicGetGeometry(atomic))
			RpAtomicSetGeometry(atomic, geo, rpATOMICSAMEBOUNDINGSPHERE); // originally 5 (mistake?)
		RENDERCALLBACK(atomic);
		if(odSaveOk)
			OdAlphaEnd(geo, &state);
		RpGeometrySetFlags(geo, flags);
	}

	if(mi->m_additive)
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

	return atomic;
}



#ifdef __EMSCRIPTEN__
// Traza de medida (30/10, 36.3): el `chassis_hi` del coche que lleva el jugador,
// comparado material a material contra el DFF. Una cabecera con los totales del
// atomic y una linea por material con el nombre de la textura, si existe y los
// triangulos que le apuntan, leidos de dos sitios: la malla ya instanciada
// (`meshHeader`, que es lo que se dibuja) y el array de triangulos leido del DFF
// (`geo->triangles`). Si `tris` y `tris_dff` difieren, el reparto por material se
// rompio al montar la malla; si coinciden y la textura no existe, falta el TXD; si
// todo coincide, el fallo es del dibujo, no de los datos. Se emite una sola vez
// por atomic, al entrar al pase de detalle (no por frame).
static void
OdDumpPlayerVehicleChassis(RpAtomic *atomic)
{
	static RpAtomic *odVistos[8];
	static int odNVistos = 0;
	CEntity *ent;
	RpGeometry *geo;
	rw::MeshHeader *mh;
	CBaseModelInfo *mi;
	const char *nm;
	char t[248];
	int i, n;

	ent = FindPlayerEntity();
	if(ent == nil || !ent->IsVehicle() || ent->m_rwObject == nil)
		return;
	if(RwObjectGetType(ent->m_rwObject) != rpCLUMP)
		return;
	if((RpClump*)ent->m_rwObject != RpAtomicGetClump(atomic))
		return;
	for(i = 0; i < odNVistos; i++)
		if(odVistos[i] == atomic)
			return;
	if(odNVistos >= (int)(sizeof(odVistos)/sizeof(odVistos[0])))
		return;
	odVistos[odNVistos++] = atomic;

	geo = RpAtomicGetGeometry(atomic);
	if(geo == nil)
		return;
	mh = geo->meshHeader;
	n = geo->matList.numMaterials;
	mi = CModelInfo::GetModelInfo(ent->GetModelIndex());
	nm = GetFrameNodeName(RpAtomicGetFrame(atomic));
	snprintf(t, sizeof t, "ODDASH modelo=%s atomic=%s verts=%d tris=%d mats=%d mallas=%d",
		mi ? mi->GetModelName() : "?", nm ? nm : "?",
		RpGeometryGetNumVertices(geo), RpGeometryGetNumTriangles(geo),
		n, mh ? (int)mh->numMeshes : -1);
	ODTRACES(t);

	for(i = 0; i < n; i++){
		RpMaterial *m = RpGeometryGetMaterial(geo, i);
		RwTexture *tex = m ? RpMaterialGetTexture(m) : nil;
		const RwRGBA *col = m ? RpMaterialGetColor(m) : nil;
		int trisMalla = -1;
		int trisDff = 0;
		int j;

		if(mh && (mh->flags & rw::MeshHeader::TRISTRIP) == 0 && i < mh->numMeshes)
			trisMalla = (int)(mh->getMeshes()[i].numIndices / 3);
		for(j = 0; geo->triangles && j < geo->numTriangles; j++)
			if(geo->triangles[j].matId == i)
				trisDff++;

		snprintf(t, sizeof t, "ODDASH m=%d tex=%s existe=%d alfa=%d tris=%d tris_dff=%d",
			i, tex ? RwTextureGetName(tex) : "-", tex ? 1 : 0,
			col ? (int)col->alpha : -1, trisMalla, trisDff);
		ODTRACES(t);
	}
}
#endif

#ifdef __EMSCRIPTEN__
static void
OdTraceDoorCull(RpAtomic *atomic, float dot, uint32 flags)
{
	static RpAtomic *odDoorVistos[16];
	static int odNDoorVistos = 0;
	CEntity *ent;
	RwFrame *frame;
	RwMatrix *ltm;
	const char *nm;
	int i;
	float cull;
	char t[240];

	ent = FindPlayerEntity();
	if(ent == nil || !ent->IsVehicle() || ent->m_rwObject == nil)
		return;
	if((RpClump*)ent->m_rwObject != RpAtomicGetClump(atomic))
		return;
	frame = RpAtomicGetFrame(atomic);
	nm = frame ? GetFrameNodeName(frame) : nil;
	if(nm == nil || strncmp(nm, "door_", 5) != 0)
		return;
	for(i = 0; i < odNDoorVistos; i++)
		if(odDoorVistos[i] == atomic)
			return;
	if(odNDoorVistos >= (int)(sizeof(odDoorVistos)/sizeof(odDoorVistos[0])))
		return;
	odDoorVistos[odNDoorVistos++] = atomic;
	cull = DistToCameraSq > CVisibilityPlugins::ms_cullCompsDist && (flags & ATOMIC_FLAG_NOCULL) == 0 &&
		PitchToCamera < 0.2f && dot > 0.0f &&
		((flags & ATOMIC_FLAG_ANGLECULL) || 0.1f*DistToCameraSq < dot*dot);
	ltm = RwFrameGetLTM(frame);
	snprintf(t, sizeof t, "VEHDOOR2 atomic=%s dot=%.2f flags=0x%X dist=%.1f cull=%.0f pitch=%.2f at=(%.2f,%.2f,%.2f)",
		nm, (double)dot, (unsigned)flags, (double)DistToCameraSq, (double)cull,
		(double)PitchToCamera, ltm ? (double)ltm->at.x : 0.0, ltm ? (double)ltm->at.y : 0.0,
		ltm ? (double)ltm->at.z : 0.0);
	ODTRACES(t);
}
#endif

RpAtomic*
CVisibilityPlugins::RenderVehicleHiDetailCB(RpAtomic *atomic)
{
	RwFrame *clumpframe;

#ifdef __EMSCRIPTEN__
	OdDumpPlayerVehicleChassis(atomic);
#endif
	float dot;
	uint32 flags;

	clumpframe = RpClumpGetFrame(RpAtomicGetClump(atomic));
	if(DistToCameraSq < ms_vehicleLod0Dist){
		flags = GetAtomicId(atomic);
		dot = GetDotProductWithCameraVector(RwFrameGetLTM(RpAtomicGetFrame(atomic)),
			RwFrameGetLTM(clumpframe), flags);
#ifdef __EMSCRIPTEN__
		OdTraceDoorCull(atomic, dot, flags);
#endif
		if(DistToCameraSq > ms_cullCompsDist && (flags & ATOMIC_FLAG_NOCULL) == 0 && PitchToCamera < 0.2f){
			if(dot > 0.0f && ((flags & ATOMIC_FLAG_ANGLECULL) || 0.1f*DistToCameraSq < dot*dot))
				return atomic;
		}
		OD_TAG(atomic);
		RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleHiDetailAlphaCB(RpAtomic *atomic)
{
	RwFrame *clumpframe;
	float dot;
	uint32 flags;

	clumpframe = RpClumpGetFrame(RpAtomicGetClump(atomic));
	if(DistToCameraSq < ms_vehicleLod0Dist){
		flags = GetAtomicId(atomic);
#ifdef __EMSCRIPTEN__
		{
			static RpAtomic *odCristalVisto[16];
			static int odNCristalVisto = 0;
			const char *odNm = GetFrameNodeName(RpAtomicGetFrame(atomic));
			if(odNm && DistToCameraSq < 400.0f &&
			   (strncmp(odNm, "window", 6) == 0 || strncmp(odNm, "windscreen", 10) == 0)){
				int odI, odYa = 0;
				for(odI = 0; odI < odNCristalVisto; odI++)
					if(odCristalVisto[odI] == atomic)
						odYa = 1;
				if(!odYa && odNCristalVisto < 16){
					RpGeometry *odG = RpAtomicGetGeometry(atomic);
					RpMaterial *odM = odG ? RpGeometryGetMaterial(odG, 0) : nil;
					const RwRGBA *odC = odM ? RpMaterialGetColor(odM) : nil;
					char odT[200];
					odCristalVisto[odNCristalVisto++] = atomic;
					snprintf(odT, sizeof odT, "VEHCRISTAL atomic=%s tris=%d alfa=%d tex=%d dist=%.1f cull=%.0f",
						odNm, odG ? RpGeometryGetNumTriangles(odG) : -1,
						odC ? (int)odC->alpha : -1, odM && RpMaterialGetTexture(odM) ? 1 : 0,
						Sqrt(DistToCameraSq), ms_cullCompsDist);
					ODTRACES(odT);
				}
			}
		}
#endif
		dot = GetDotProductWithCameraVector(RwFrameGetLTM(RpAtomicGetFrame(atomic)),
			RwFrameGetLTM(clumpframe), flags);
		if(DistToCameraSq > ms_cullCompsDist && (flags & ATOMIC_FLAG_NOCULL) == 0 && PitchToCamera < 0.2f)
			if(dot > 0.0f && ((flags & ATOMIC_FLAG_ANGLECULL) || 0.1f*DistToCameraSq < dot*dot))
				return atomic;

		if(flags & ATOMIC_FLAG_DRAWLAST){
			// sort before clump
			if(!InsertAtomicIntoSortedList(atomic, DistToCameraSq - 0.0001f))
				RENDERCALLBACK(atomic);
		}else{
			if(!InsertAtomicIntoSortedList(atomic, DistToCameraSq + dot))
				RENDERCALLBACK(atomic);
		}
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleHiDetailCB_BigVehicle(RpAtomic *atomic)
{
	RwFrame *clumpframe;
	float dot;
	uint32 flags;

	clumpframe = RpClumpGetFrame(RpAtomicGetClump(atomic));
	if(DistToCameraSq < ms_bigVehicleLod0Dist){
		flags = GetAtomicId(atomic);
		dot = GetDotProductWithCameraVector(RwFrameGetLTM(RpAtomicGetFrame(atomic)),
			RwFrameGetLTM(clumpframe), flags);
		if(DistToCameraSq > ms_cullCompsDist && (flags & ATOMIC_FLAG_NOCULL) == 0 && PitchToCamera < 0.2f){
			if(dot > 0.0f)
				return atomic;
		}
		OD_TAG(atomic);
		RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleHiDetailAlphaCB_BigVehicle(RpAtomic *atomic)
{
	RwFrame *clumpframe;
	float dot;
	uint32 flags;

	clumpframe = RpClumpGetFrame(RpAtomicGetClump(atomic));
	if(DistToCameraSq < ms_bigVehicleLod0Dist){
		flags = GetAtomicId(atomic);
		dot = GetDotProductWithCameraVector(RwFrameGetLTM(RpAtomicGetFrame(atomic)),
			RwFrameGetLTM(clumpframe), flags);
		if(DistToCameraSq > ms_cullCompsDist && (flags & ATOMIC_FLAG_NOCULL) == 0 && PitchToCamera < 0.2f)
			if(dot > 0.0f && ((flags & ATOMIC_FLAG_ANGLECULL) || 0.1f*DistToCameraSq < dot*dot))
				return atomic;

		if(!InsertAtomicIntoSortedList(atomic, DistToCameraSq + dot))
			RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleHiDetailCB_Boat(RpAtomic *atomic)
{
	if(DistToCameraSq < ms_vehicleLod0Dist)
		RENDERCALLBACK(atomic);
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleHiDetailAlphaCB_Boat(RpAtomic *atomic)
{
	if(DistToCameraSq < ms_vehicleLod0Dist){
		if(GetAtomicId(atomic) & ATOMIC_FLAG_DRAWLAST){
			if(!InsertAtomicIntoBoatSortedList(atomic, DistToCameraSq))
				RENDERCALLBACK(atomic);
		}else
			RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleLoDetailCB_Boat(RpAtomic *atomic)
{
	RpClump *clump;
	int32 alpha;

	clump = RpAtomicGetClump(atomic);
	if(DistToCameraSq >= ms_vehicleLod0Dist){
		alpha = GetClumpAlpha(clump);
		if(alpha == 255)
			RENDERCALLBACK(atomic);
		else
			RenderAlphaAtomic(atomic, alpha);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleLowDetailCB_BigVehicle(RpAtomic *atomic)
{
	RwFrame *clumpframe;
	float dot;
	uint32 flags;

	clumpframe = RpClumpGetFrame(RpAtomicGetClump(atomic));
	if(DistToCameraSq >= ms_bigVehicleLod0Dist &&
	   DistToCameraSq < ms_bigVehicleLod1Dist){
		flags = GetAtomicId(atomic);
		if(DistToCameraSq > ms_cullCompsDist && (flags & ATOMIC_FLAG_NOCULL) == 0 && PitchToCamera < 0.2f){
			dot = GetDotProductWithCameraVector(RwFrameGetLTM(RpAtomicGetFrame(atomic)),
				RwFrameGetLTM(clumpframe), flags);
			if(dot > 0.0f)
				return atomic;
		}
		RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleLowDetailAlphaCB_BigVehicle(RpAtomic *atomic)
{
	RwFrame *clumpframe;
	float dot;
	uint32 flags;

	clumpframe = RpClumpGetFrame(RpAtomicGetClump(atomic));
	if(DistToCameraSq >= ms_bigVehicleLod0Dist &&
	   DistToCameraSq < ms_bigVehicleLod1Dist){
		flags = GetAtomicId(atomic);
		dot = GetDotProductWithCameraVector(RwFrameGetLTM(RpAtomicGetFrame(atomic)),
			RwFrameGetLTM(clumpframe), flags);
		if(dot > 0.0f)
			if(DistToCameraSq > ms_cullCompsDist && (flags & ATOMIC_FLAG_NOCULL) == 0 && PitchToCamera < 0.2f)
				return atomic;

		if(!InsertAtomicIntoSortedList(atomic, DistToCameraSq + dot))
			RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleReallyLowDetailCB(RpAtomic *atomic)
{
	RpClump *clump;
	int32 alpha;

	clump = RpAtomicGetClump(atomic);
	if(DistToCameraSq >= ms_vehicleLod0Dist){
		alpha = GetClumpAlpha(clump);
		if(alpha == 255)
			RENDERCALLBACK(atomic);
		else
			RenderAlphaAtomic(atomic, alpha);
	}
	return atomic;

}

RpAtomic*
CVisibilityPlugins::RenderVehicleReallyLowDetailCB_BigVehicle(RpAtomic *atomic)
{
	if(DistToCameraSq >= ms_bigVehicleLod1Dist)
		RENDERCALLBACK(atomic);
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderTrainHiDetailCB(RpAtomic *atomic)
{
	RwFrame *clumpframe;
	float dot;
	uint32 flags;

	clumpframe = RpClumpGetFrame(RpAtomicGetClump(atomic));
	if(DistToCameraSq < ms_bigVehicleLod1Dist){
		flags = GetAtomicId(atomic);
		if(DistToCameraSq > ms_cullCompsDist && (flags & ATOMIC_FLAG_NOCULL) == 0 && PitchToCamera < 0.2f){
			dot = GetDotProductWithCameraVector(RwFrameGetLTM(RpAtomicGetFrame(atomic)),
				RwFrameGetLTM(clumpframe), flags);
			if(dot > 0.0f && ((flags & ATOMIC_FLAG_ANGLECULL) || 0.1f*DistToCameraSq < dot*dot))
				return atomic;
		}
		RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderTrainHiDetailAlphaCB(RpAtomic *atomic)
{
	RwFrame *clumpframe;
	float dot;
	uint32 flags;

	clumpframe = RpClumpGetFrame(RpAtomicGetClump(atomic));
	if(DistToCameraSq < ms_bigVehicleLod1Dist){
		flags = GetAtomicId(atomic);
		dot = GetDotProductWithCameraVector(RwFrameGetLTM(RpAtomicGetFrame(atomic)),
			RwFrameGetLTM(clumpframe), flags);
		if(DistToCameraSq > ms_cullCompsDist && (flags & ATOMIC_FLAG_NOCULL) == 0 && PitchToCamera < 0.2f)
			if(dot > 0.0f && ((flags & ATOMIC_FLAG_ANGLECULL) || 0.1f*DistToCameraSq < dot*dot))
				return atomic;

		if(flags & ATOMIC_FLAG_DRAWLAST){
			if(!InsertAtomicIntoSortedList(atomic, DistToCameraSq))
				RENDERCALLBACK(atomic);
		}else{
			if(!InsertAtomicIntoSortedList(atomic, DistToCameraSq + dot))
				RENDERCALLBACK(atomic);
		}
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleRotorAlphaCB(RpAtomic *atomic)
{
	RwFrame *clumpframe;
	float dot;
	RwV3d cam2atm;

	clumpframe = RpClumpGetFrame(RpAtomicGetClump(atomic));
	if(DistToCameraSq < ms_bigVehicleLod1Dist){
		RwV3dSub(&cam2atm, &RwFrameGetLTM(RpAtomicGetFrame(atomic))->pos, ms_pCameraPosn);
		dot = RwV3dDotProduct(&cam2atm, &RwFrameGetLTM(clumpframe)->at);
		if(!InsertAtomicIntoSortedList(atomic, DistToCameraSq + dot*20.0f))
			RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderVehicleTailRotorAlphaCB(RpAtomic *atomic)
{
	RwMatrix *clumpMat, *atmMat;
	float dot;
	RwV3d cam2atm;

	if(DistToCameraSq < ms_bigVehicleLod0Dist){
		atmMat = RwFrameGetLTM(RpAtomicGetFrame(atomic));
		clumpMat = RwFrameGetLTM(RpClumpGetFrame(RpAtomicGetClump(atomic)));
		RwV3dSub(&cam2atm, &atmMat->pos, ms_pCameraPosn);
		dot = RwV3dDotProduct(&cam2atm, &clumpMat->up) + RwV3dDotProduct(&cam2atm, &clumpMat->right);
		if(!InsertAtomicIntoSortedList(atomic, DistToCameraSq - dot))
			RENDERCALLBACK(atomic);
	}
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderPlayerCB(RpAtomic *atomic)
{
	if(CWorld::Players[0].m_pSkinTexture)
		RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), SetTextureCB, CWorld::Players[0].m_pSkinTexture);
	RENDERCALLBACK(atomic);
	return atomic;
}

RpAtomic*
CVisibilityPlugins::RenderPedCB(RpAtomic *atomic)
{
	RpClump *clump;
	float dist;
	int32 alpha;

	clump = RpAtomicGetClump(atomic);
	dist = GetDistanceSquaredFromCamera(RpClumpGetFrame(clump));
	if(dist < ms_pedLod1Dist){
		alpha = GetClumpAlpha(clump);
#ifdef __EMSCRIPTEN__
		if(alpha != 255){
			static uint32 odNextPedAlpha = 0;
			uint32 odNow = CTimer::GetTimeInMilliseconds();
			if(odNow >= odNextPedAlpha){
				odNextPedAlpha = odNow + 2000;
				char odT[160];
				CClumpModelInfo *odMi = (CClumpModelInfo*)GetClumpModelInfo(clump);
				snprintf(odT, sizeof odT, "PEDALPHA modelo=%s alpha=%d",
					odMi ? odMi->GetModelName() : "?", alpha);
				ODTRACES(odT);
			}
		}
#endif
		if(alpha == 255)
			RENDERCALLBACK(atomic);
		else
			RenderAlphaAtomic(atomic, alpha);
	}
	return atomic;
}

float
CVisibilityPlugins::GetDistanceSquaredFromCamera(RwV3d *pos)
{
	RwV3d dist;
	RwV3dSub(&dist, pos, ms_pCameraPosn);
	return RwV3dDotProduct(&dist, &dist);
}

float
CVisibilityPlugins::GetDistanceSquaredFromCamera(RwFrame *frame)
{
	RwMatrix *m;
	RwV3d dist;
	m = RwFrameGetLTM(frame);
	RwV3dSub(&dist, RwMatrixGetPos(m), ms_pCameraPosn);
	return RwV3dDotProduct(&dist, &dist);
}

float
CVisibilityPlugins::GetDotProductWithCameraVector(RwMatrix *atomicMat, RwMatrix *clumpMat, uint32 flags)
{
	RwV3d dist;
	float dot, dotdoor;

	// Vehicle forward is the y axis (RwMatrix.up)
	// Vehicle right is the x axis (RwMatrix.right)

	RwV3dSub(&dist, RwMatrixGetPos(atomicMat), ms_pCameraPosn);
	// forward/backward facing
	if(flags & (ATOMIC_FLAG_FRONT | ATOMIC_FLAG_REAR))
		dot = RwV3dDotProduct(&dist, RwMatrixGetUp(clumpMat));
	// left/right facing
	else if(flags & (ATOMIC_FLAG_LEFT | ATOMIC_FLAG_RIGHT))
		dot = RwV3dDotProduct(&dist, RwMatrixGetRight(clumpMat));
	else
		dot = 0.0f;
	if(flags & (ATOMIC_FLAG_LEFT | ATOMIC_FLAG_REAR))
		dot = -dot;

	if(flags & (ATOMIC_FLAG_REARDOOR | ATOMIC_FLAG_FRONTDOOR)){
		if(flags & ATOMIC_FLAG_REARDOOR)
			dotdoor = -RwV3dDotProduct(&dist, RwMatrixGetUp(clumpMat));
		else if(flags & ATOMIC_FLAG_FRONTDOOR)
			dotdoor = RwV3dDotProduct(&dist, RwMatrixGetUp(clumpMat));
		else
			dotdoor = 0.0f;

		if(dot < 0.0f && dotdoor < 0.0f)
			dot += dotdoor;
		if(dot > 0.0f && dotdoor > 0.0f)
			dot += dotdoor;
	}

	return dot;
}

/* These are all unused */

bool
CVisibilityPlugins::DefaultVisibilityCB(RpClump *clump)
{
	return true;
}

bool
CVisibilityPlugins::FrustumSphereCB(RpClump *clump)
{
	RwSphere sphere;
	RwFrame *frame = RpClumpGetFrame(clump);

	CClumpModelInfo *modelInfo = (CClumpModelInfo*)GetFrameHierarchyId(frame);
	sphere.radius = modelInfo->GetColModel()->boundingSphere.radius;
	sphere.center.x = modelInfo->GetColModel()->boundingSphere.center.x;
	sphere.center.y = modelInfo->GetColModel()->boundingSphere.center.y;
	sphere.center.z = modelInfo->GetColModel()->boundingSphere.center.z;
	RwV3dTransformPoints(&sphere.center, &sphere.center, 1, RwFrameGetLTM(frame));
	return RwCameraFrustumTestSphere(ms_pCamera, &sphere) != rwSPHEREOUTSIDE;
}

bool
CVisibilityPlugins::MloVisibilityCB(RpClump *clump)
{
	RwFrame *frame = RpClumpGetFrame(clump);
	CMloModelInfo *modelInfo = (CMloModelInfo*)GetFrameHierarchyId(frame);
	if (SQR(modelInfo->drawDist) < GetDistanceSquaredFromCamera(frame))
		return false;
	return CVisibilityPlugins::FrustumSphereCB(clump);
}

bool
CVisibilityPlugins::VehicleVisibilityCB(RpClump *clump)
{
	RwFrame *frame = RpClumpGetFrame(clump);
	if (ms_vehicleLod1Dist < GetDistanceSquaredFromCamera(frame))
		return false;
	return FrustumSphereCB(clump);
}

bool
CVisibilityPlugins::VehicleVisibilityCB_BigVehicle(RpClump *clump)
{
	return FrustumSphereCB(clump);
}




//
// RW Plugins
//

enum
{
	ID_VISIBILITYATOMIC = MAKECHUNKID(rwVENDORID_ROCKSTAR, 0x00),
	ID_VISIBILITYCLUMP  = MAKECHUNKID(rwVENDORID_ROCKSTAR, 0x01),
	ID_VISIBILITYFRAME  = MAKECHUNKID(rwVENDORID_ROCKSTAR, 0x02),
};

bool
CVisibilityPlugins::PluginAttach(void)
{
	ms_atomicPluginOffset = RpAtomicRegisterPlugin(sizeof(AtomicExt),
		ID_VISIBILITYATOMIC,
		AtomicConstructor, AtomicDestructor, AtomicCopyConstructor);

	ms_framePluginOffset = RwFrameRegisterPlugin(sizeof(FrameExt),
		ID_VISIBILITYFRAME,
		FrameConstructor, FrameDestructor, FrameCopyConstructor);

	ms_clumpPluginOffset = RpClumpRegisterPlugin(sizeof(ClumpExt),
		ID_VISIBILITYCLUMP,
		ClumpConstructor, ClumpDestructor, ClumpCopyConstructor);
	return ms_atomicPluginOffset != -1 && ms_clumpPluginOffset != -1;
}

#define ATOMICEXT(o) (RWPLUGINOFFSET(AtomicExt, o, ms_atomicPluginOffset))
#define FRAMEEXT(o) (RWPLUGINOFFSET(FrameExt, o, ms_framePluginOffset))
#define CLUMPEXT(o) (RWPLUGINOFFSET(ClumpExt, o, ms_clumpPluginOffset))

//
// Atomic
//

void*
CVisibilityPlugins::AtomicConstructor(void *object, int32, int32)
{
	ATOMICEXT(object)->modelInfo = nil;
	return object;
}

void*
CVisibilityPlugins::AtomicDestructor(void *object, int32, int32)
{
	return object;
}

void*
CVisibilityPlugins::AtomicCopyConstructor(void *dst, const void *src, int32, int32)
{
	*ATOMICEXT(dst) = *ATOMICEXT(src);
	return dst;
}

void
CVisibilityPlugins::SetAtomicModelInfo(RpAtomic *atomic,
                                       CSimpleModelInfo *modelInfo)
{
	AtomicExt *ext = ATOMICEXT(atomic);
	ext->modelInfo = modelInfo;
}

CSimpleModelInfo*
CVisibilityPlugins::GetAtomicModelInfo(RpAtomic *atomic)
{
	return ATOMICEXT(atomic)->modelInfo;
}

void
CVisibilityPlugins::SetAtomicFlag(RpAtomic *atomic, int f)
{
	ATOMICEXT(atomic)->flags |= f;
}

void
CVisibilityPlugins::ClearAtomicFlag(RpAtomic *atomic, int f)
{
	ATOMICEXT(atomic)->flags &= ~f;
}

void
CVisibilityPlugins::SetAtomicId(RpAtomic *atomic, int id)
{
	ATOMICEXT(atomic)->flags = id;
}

int
CVisibilityPlugins::GetAtomicId(RpAtomic *atomic)
{
	return ATOMICEXT(atomic)->flags;
}

void
CVisibilityPlugins::SetAtomicRenderCallback(RpAtomic *atomic, RpAtomicCallBackRender cb)
{
	if(cb == nil)
		cb = RENDERCALLBACK;	// not necessary
	RpAtomicSetRenderCallBack(atomic, cb);
}

//
// Frame
//

void*
CVisibilityPlugins::FrameConstructor(void *object, int32, int32)
{
	FRAMEEXT(object)->id = 0;
	return object;
}

void*
CVisibilityPlugins::FrameDestructor(void *object, int32, int32)
{
	return object;
}

void*
CVisibilityPlugins::FrameCopyConstructor(void *dst, const void *src, int32, int32)
{
	*FRAMEEXT(dst) = *FRAMEEXT(src);
	return dst;
}

void
CVisibilityPlugins::SetFrameHierarchyId(RwFrame *frame, intptr id)
{
	FRAMEEXT(frame)->id = id;
}

intptr
CVisibilityPlugins::GetFrameHierarchyId(RwFrame *frame)
{
	return FRAMEEXT(frame)->id;
}


//
// Clump
//

void*
CVisibilityPlugins::ClumpConstructor(void *object, int32, int32)
{
	ClumpExt *ext = CLUMPEXT(object);
	ext->visibilityCB = DefaultVisibilityCB;
	ext->alpha = 0xFF;
	return object;
}

void*
CVisibilityPlugins::ClumpDestructor(void *object, int32, int32)
{
	return object;
}

void*
CVisibilityPlugins::ClumpCopyConstructor(void *dst, const void *src, int32, int32)
{
	CLUMPEXT(dst)->visibilityCB = CLUMPEXT(src)->visibilityCB;
	CLUMPEXT(dst)->alpha = 0xFF;
	return dst;
}

void
CVisibilityPlugins::SetClumpModelInfo(RpClump *clump, CClumpModelInfo *modelInfo)
{
	CVehicleModelInfo *vmi;
	SetFrameHierarchyId(RpClumpGetFrame(clump), (intptr)modelInfo);

	// Unused
	switch (modelInfo->GetModelType()) {
	case MITYPE_MLO:
		CLUMPEXT(clump)->visibilityCB = MloVisibilityCB;
		break;
	case MITYPE_VEHICLE:
		vmi = (CVehicleModelInfo*)modelInfo;
		if(vmi->m_vehicleType == VEHICLE_TYPE_TRAIN ||
		   vmi->m_vehicleType == VEHICLE_TYPE_HELI ||
		   vmi->m_vehicleType == VEHICLE_TYPE_PLANE)
			CLUMPEXT(clump)->visibilityCB = VehicleVisibilityCB_BigVehicle;
		else
			CLUMPEXT(clump)->visibilityCB = VehicleVisibilityCB;
		break;
	default: break;
	}
}

CClumpModelInfo*
CVisibilityPlugins::GetClumpModelInfo(RpClump *clump)
{
	return (CClumpModelInfo*)GetFrameHierarchyId(RpClumpGetFrame(clump));
}

void
CVisibilityPlugins::SetClumpAlpha(RpClump *clump, int alpha)
{
	CLUMPEXT(clump)->alpha = alpha;
}

int
CVisibilityPlugins::GetClumpAlpha(RpClump *clump)
{
	return CLUMPEXT(clump)->alpha;
}

bool
CVisibilityPlugins::IsClumpVisible(RpClump *clump)
{
	return CLUMPEXT(clump)->visibilityCB(clump);
}
