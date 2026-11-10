#include "common.h"

#include "RwHelper.h"
#include "General.h"
#include "Bones.h"
#include "SurfaceTable.h"
#include "Ped.h"
#include "NodeName.h"
#include "VisibilityPlugins.h"
#include "ModelInfo.h"
#include "custompipes.h"
#ifdef VICEEXT_PED_VARIATIONS
#include "TxdStore.h"
#include "PlayerPed.h"
#include "PlayerInfo.h"	// FindPlayerPed, para el csplay de SetClump
#endif
#include "ondemand.h"

#ifdef VICEEXT_PED_VARIATIONS
// El cache es dueno de una referencia: si no, el TXD descargado deja punteros colgando en Material::destroy.
static void
releaseVariationCache(CPedModelInfo *modelInfo)
{
#ifdef __EMSCRIPTEN__
	static uint32 s_odVarFreeN;
	int odRefs = 0;
	for (int i = 0; i < MAX_VARIATIONS_TEXTURES; i++) {
		if (modelInfo->textureClothingVariations[i]) odRefs++;
		if (modelInfo->textureShadeVariations[i]) odRefs++;
	}
	if (odRefs && (s_odVarFreeN++ <= 60 || s_odVarFreeN % 200 == 0)) {
		char odT[128];
		snprintf(odT, sizeof odT, "PEDVARFREE model=%s refs=%d", modelInfo->GetModelName(), odRefs);
		ODTRACES(odT);
	}
#endif
	for (int i = 0; i < MAX_VARIATIONS_TEXTURES; i++) {
		if (modelInfo->textureClothingVariations[i]) {
			modelInfo->textureClothingVariations[i]->destroy();
			modelInfo->textureClothingVariations[i] = nullptr;
		}
	}
	for (int i = 0; i < MAX_VARIATIONS_TEXTURES; i++) {
		if (modelInfo->textureShadeVariations[i]) {
			modelInfo->textureShadeVariations[i]->destroy();
			modelInfo->textureShadeVariations[i] = nullptr;
		}
	}
}
#endif

void
CPedModelInfo::DeleteRwObject(void)
{
	CClumpModelInfo::DeleteRwObject();
#ifdef VICEEXT_PED_VARIATIONS
	releaseVariationCache(this);
#endif
	if(m_hitColModel)
		delete m_hitColModel;
	m_hitColModel = nil;
}

// leftover...
RwObjectNameIdAssocation CPedModelInfo::m_pPedIds[PED_NODE_MAX] = {
	{ "Smid",	PED_MID, 0, },	// that is strange...
	{ "Shead",	PED_HEAD, 0, },
	{ "Supperarml",	PED_UPPERARML, 0, },
	{ "Supperarmr",	PED_UPPERARMR, 0, },
	{ "SLhand",	PED_HANDL, 0, },
	{ "SRhand",	PED_HANDR, 0, },
	{ "Supperlegl",	PED_UPPERLEGL, 0, },
	{ "Supperlegr",	PED_UPPERLEGR, 0, },
	{ "Sfootl",	PED_FOOTL, 0, },
	{ "Sfootr",	PED_FOOTR, 0, },
	{ "Slowerlegr",	PED_LOWERLEGR, 0, },
	{ nil,	0, 0, },
};

#ifdef VICEEXT_PED_VARIATIONS
static RwTexture* storeTextureVariation(RwTexture* texture, void* data)
{
	CPedModelInfo* modelInfo = static_cast<CPedModelInfo*>(data);

	for (int i = 0; i < MAX_VARIATIONS_TEXTURES; i++) {
		if (modelInfo->textureClothingVariations[i])
			continue;

		if (strstr(texture->name, "shade"))
			continue;

		if (!strstr(texture->name, "remap") && !strstr(texture->name, "CSplay"))
			continue;

		texture->addRef();
		modelInfo->textureClothingVariations[i] = texture;

		return texture;
	}

	for (int i = 0; i < MAX_VARIATIONS_TEXTURES; i++) {
		if (modelInfo->textureShadeVariations[i])
			continue;

		if (!strstr(texture->name, "shade"))
			continue;

		texture->addRef();
		modelInfo->textureShadeVariations[i] = texture;

		return texture;
	}

	return texture;
}
#endif

#ifdef VICEEXT_SEAT_BY_PED_HEIGHT
/* Generado por tools/ped_seat_height.py (ref=0.605, lift en cm).
   Regenerar con: python tools/ped_seat_height.py --emit-c */
static const struct { char m_name[12]; int16 m_lift_cm; }
odPedSeatLiftTable[] = {
	{ "apoulet", 7 },
	{ "army", 4 },
	{ "bfobe", 19 },
	{ "bfori", 19 },
	{ "bfost", 19 },
	{ "bfotr", 19 },
	{ "bfybe", 19 },
	{ "bfypr", 19 },
	{ "bfyri", 19 },
	{ "bfyst", 19 },
	{ "bmobe", 19 },
	{ "bmybe", 4 },
	{ "cbb", 3 },
	{ "cdrivra", 17 },
	{ "cdrivrb", 17 },
	{ "cgoona", 7 },
	{ "csalsb2", 2 },
	{ "csalscb", 2 },
	{ "csalscb2", 2 },
	{ "csassb", 2 },
	{ "cscandy", 10 },
	{ "cscgona", 7 },
	{ "csdiaz", 8 },
	{ "csdirec", 4 },
	{ "csdoris", 15 },
	{ "cservra", 19 },
	{ "cservrb", 19 },
	{ "csfiller", 4 },
	{ "csgangc", 7 },
	{ "csgona", 7 },
	{ "csgonz", 5 },
	{ "csjuggz", 19 },
	{ "cskelly", 7 },
	{ "cskent", 4 },
	{ "csmaude", 7 },
	{ "csmerc", 12 },
	{ "csmerc2", 12 },
	{ "cspablo", 4 },
	{ "cspapa", 8 },
	{ "csphil", 3 },
	{ "csumbto", 4 },
	{ "diaz", 8 },
	{ "fireman", 4 },
	{ "floozya", 19 },
	{ "floozyb", 19 },
	{ "floozyc", 11 },
	{ "hfobe", 19 },
	{ "hfori", 19 },
	{ "hfost", 19 },
	{ "hfotr", 19 },
	{ "hfybe", 19 },
	{ "hfybu", 19 },
	{ "hfycg", 19 },
	{ "hfymd", 19 },
	{ "hfypr", 19 },
	{ "hfyri", 19 },
	{ "hfyst", 19 },
	{ "hmobe", 19 },
	{ "hmoca", 17 },
	{ "igalscb", 2 },
	{ "igcandy", 10 },
	{ "igdiaz", 8 },
	{ "iggonz", 5 },
	{ "igmerc", 10 },
	{ "igmerc2", 12 },
	{ "igphil", 2 },
	{ "igphil2", 3 },
	{ "igphil3", 2 },
	{ "jfoto", 16 },
	{ "jmoto", 17 },
	{ "kgoona", 7 },
	{ "psycho", 6 },
	{ "shootra", 19 },
	{ "spandxa", 9 },
	{ "spandxb", 5 },
	{ "stripa", 11 },
	{ "stripb", 11 },
	{ "stripc", 11 },
	{ "swat", 9 },
	{ "taxi_d", 6 },
	{ "wfobe", 19 },
	{ "wfogo", 16 },
	{ "wfori", 19 },
	{ "wfosh", 16 },
	{ "wfost", 19 },
	{ "wfotr", 16 },
	{ "wfybe", 19 },
	{ "wfybu", 19 },
	{ "wfycst", 19 },
	{ "wfyg1", 19 },
	{ "wfyg2", 15 },
	{ "wfyjg", 15 },
	{ "wfylg", 15 },
	{ "wfypr", 19 },
	{ "wfyri", 19 },
	{ "wfysh", 15 },
	{ "wfysk", 7 },
	{ "wfyst", 19 },
	{ "wmobe", 19 },
	{ "wmoca", 17 },
	{ "wmogo", 19 },
	{ "wmotr", 4 },
	{ "wmybe", 4 },
	{ "wmycw", 4 },
	{ "wmyjg", 4 },
	{ "wmylg", 4 },
	{ "wmysk", 6 },
};
/* 107 modelos de 269 */

static float
odSeatLiftForModel(const char *name)
{
	for (int i = 0; i < (int)ARRAY_SIZE(odPedSeatLiftTable); i++)
		if (CGeneral::faststricmp(odPedSeatLiftTable[i].m_name, name) == 0)
			return odPedSeatLiftTable[i].m_lift_cm / 100.0f;
	return 0.0f;
}
#endif

void
CPedModelInfo::SetClump(RpClump *clump)
{
#ifdef EXTENDED_PIPELINES
	CustomPipes::AttachRimPipe(clump);
#endif
	CClumpModelInfo::SetClump(clump);
#ifdef VICEEXT_SEAT_BY_PED_HEIGHT
	m_seatLift = odSeatLiftForModel(GetModelName());
#endif
	SetFrameIds(m_pPedIds);	// not needed in VC actually
	if(m_hitColModel == nil)
		CreateHitColModelSkinned(clump);
	RpClumpForAllAtomics(m_clump, SetAtomicRendererCB, (void*)CVisibilityPlugins::RenderPedCB);
	if(strcmp(GetModelName(), "player") == 0)
		RpClumpForAllAtomics(m_clump, SetAtomicRendererCB, (void*)CVisibilityPlugins::RenderPlayerCB);

#ifdef VICEEXT_PED_VARIATIONS
	releaseVariationCache(this);
	currentClothingVariation = 0;
	currentShadeVariation = 0;
	RwTexDictionary* pedTxd = CTxdStore::GetSlot(GetTxdSlot())->texDict;
	if (pedTxd) {
		RwTexDictionaryForAllTextures(pedTxd, storeTextureVariation, this);

		if (strstr(GetModelName(), "csplay")) {
			CPedModelInfo* playerModelInfo = (CPedModelInfo*)CModelInfo::GetModelInfo(FindPlayerPed()->GetModelIndex());
			if (playerModelInfo && playerModelInfo->currentClothingVariation > 0) {
				RwTexDictionary* playerTxd = CTxdStore::GetSlot(GetTxdSlot())->texDict;
				if (playerTxd) {
					char sTemp[16];
					sprintf(sTemp, "%s_%i", GetModelName(), playerModelInfo->currentClothingVariation);
					RwTexture* texClothingVariation = RwTexDictionaryFindNamedTexture(playerTxd, sTemp);
					RpAtomic* atomic = GetFirstAtomic(m_clump);
					if (texClothingVariation && atomic && atomic->geometry->matList.materials[0])
						RpMaterialSetTexture(atomic->geometry->matList.materials[0], texClothingVariation);
				}
			}
		}
	}
#endif
}

struct ColNodeInfo
{
	Const char *name;
	int pedNode;
	int pieceType;
	float x, z;
	float radius;
};

#define NUMPEDINFONODES 10
ColNodeInfo m_pColNodeInfos[NUMPEDINFONODES] = {
	{ nil, PED_HEAD,	PEDPIECE_HEAD,	0.0f,   0.05f, 0.15f },
	{ nil, PED_MID,		PEDPIECE_TORSO,	0.0f,   0.15f, 0.2f },
	{ nil, PED_MID,		PEDPIECE_TORSO,	0.0f,  -0.05f, 0.25f },
	{ nil, PED_MID,		PEDPIECE_MID,	0.0f,  -0.25f, 0.25f },
	{ nil, PED_UPPERARML,	PEDPIECE_LEFTARM,	0.03f, -0.05f,  0.16f },
	{ nil, PED_UPPERARMR,	PEDPIECE_RIGHTARM,	-0.03f, -0.05f,  0.16f },
	{ nil, PED_LOWERLEGL,	PEDPIECE_LEFTLEG,	0.0f,   0.15f, 0.2f },
	{ nil, PED_LOWERLEGR,	PEDPIECE_RIGHTLEG,	0.0f,   0.15f, 0.2f },
	{ nil, PED_FOOTL,	PEDPIECE_LEFTLEG,	0.0f,   0.15f, 0.15f },
	{ nil, PED_FOOTR,	PEDPIECE_RIGHTLEG,	0.0f,   0.15f, 0.15f },
};

void
CPedModelInfo::CreateHitColModelSkinned(RpClump *clump)
{
	RpHAnimHierarchy *hier = GetAnimHierarchyFromSkinClump(clump);
	CColModel *colmodel = new CColModel;
	CColSphere *spheres = (CColSphere*)RwMalloc(NUMPEDINFONODES*sizeof(CColSphere));
	RwFrame *root = RpClumpGetFrame(m_clump);
	RwMatrix *invmat = RwMatrixCreate();
	RwMatrix *mat = RwMatrixCreate();
	RwMatrixInvert(invmat, RwFrameGetMatrix(RpClumpGetFrame(clump)));

	for(int i = 0; i < NUMPEDINFONODES; i++){
		*mat = *invmat;

		// From LCS. Otherwise gives FPE
#ifdef FIX_BUGS
		spheres[i].center = CVector(0.0f, 0.0f, 0.0f);
#else
		int id = ConvertPedNode2BoneTag(m_pColNodeInfos[i].pedNode);	// this is wrong, wtf R* ???
		int idx = RpHAnimIDGetIndex(hier, id);

		// This doesn't really work as the positions are not initialized yet
		RwMatrixTransform(mat, &RpHAnimHierarchyGetMatrixArray(hier)[idx], rwCOMBINEPRECONCAT);
		RwV3d pos = { 0.0f, 0.0f, 0.0f };
		RwV3dTransformPoints(&pos, &pos, 1, mat);

		spheres[i].center = pos + CVector(m_pColNodeInfos[i].x, 0.0f, m_pColNodeInfos[i].z);
#endif
		spheres[i].radius = m_pColNodeInfos[i].radius;
		spheres[i].surface = SURFACE_PED;
		spheres[i].piece = m_pColNodeInfos[i].pieceType;
	}
	RwMatrixDestroy(invmat);
	RwMatrixDestroy(mat);
	colmodel->spheres = spheres;
	colmodel->numSpheres = NUMPEDINFONODES;
	colmodel->boundingSphere.Set(2.0f, CVector(0.0f, 0.0f, 0.0f));
	colmodel->boundingBox.Set(CVector(-0.5f, -0.5f, -1.2f), CVector(0.5f, 0.5f, 1.2f));
	colmodel->level = LEVEL_GENERIC;
	m_hitColModel = colmodel;
}

CColModel*
CPedModelInfo::AnimatePedColModelSkinned(RpClump *clump)
{
	if(m_hitColModel == nil){
		CreateHitColModelSkinned(clump);
		return m_hitColModel;
	}
	RwMatrix *invmat, *mat;
	CColSphere *spheres = m_hitColModel->spheres;
	RpHAnimHierarchy *hier = GetAnimHierarchyFromSkinClump(clump);
	invmat = RwMatrixCreate();
	mat = RwMatrixCreate();
	RwMatrixInvert(invmat, RwFrameGetMatrix(RpClumpGetFrame(clump)));

	for(int i = 0; i < NUMPEDINFONODES; i++){
		*mat = *invmat;
		int id = ConvertPedNode2BoneTag(m_pColNodeInfos[i].pedNode);
		int idx = RpHAnimIDGetIndex(hier, id);

		RwMatrixTransform(mat, &RpHAnimHierarchyGetMatrixArray(hier)[idx], rwCOMBINEPRECONCAT);
		RwV3d pos = { 0.0f, 0.0f, 0.0f };
		RwV3dTransformPoints(&pos, &pos, 1, mat);

		spheres[i].center = pos + CVector(m_pColNodeInfos[i].x, 0.0f, m_pColNodeInfos[i].z);
	}
	RwMatrixDestroy(invmat);
	RwMatrixDestroy(mat);
	return m_hitColModel;
}

CColModel*
CPedModelInfo::AnimatePedColModelSkinnedWorld(RpClump *clump)
{
	if(m_hitColModel == nil)
		CreateHitColModelSkinned(clump);
	CColSphere *spheres = m_hitColModel->spheres;
	RpHAnimHierarchy *hier = GetAnimHierarchyFromSkinClump(clump);
	RwMatrix *mat;

	for(int i = 0; i < NUMPEDINFONODES; i++){
		int id = ConvertPedNode2BoneTag(m_pColNodeInfos[i].pedNode);
		int idx = RpHAnimIDGetIndex(hier, id);

		mat = &RpHAnimHierarchyGetMatrixArray(hier)[idx];
		RwV3d pos = { 0.0f, 0.0f, 0.0f };
		RwV3dTransformPoints(&pos, &pos, 1, mat);

		spheres[i].center = pos + CVector(m_pColNodeInfos[i].x, 0.0f, m_pColNodeInfos[i].z);
	}
	return m_hitColModel;
}
