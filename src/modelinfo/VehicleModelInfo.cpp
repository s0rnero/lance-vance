#include "common.h"
#include <rpmatfx.h>

#include "RwHelper.h"
#include "General.h"
#include "NodeName.h"
#include "SteeringWheel.h"
#include "TxdStore.h"
#include "Weather.h"
#include "HandlingMgr.h"
#include "VisibilityPlugins.h"
#include "FileMgr.h"
#include "World.h"
#include "Vehicle.h"
#include "Automobile.h"
#include "Boat.h"
#include "Train.h"
#include "Plane.h"
#include "Heli.h"
#include "Bike.h"
#include "ModelIndices.h"
#include "ModelInfo.h"
#include "custompipes.h"
#include "ondemand.h"

int8 CVehicleModelInfo::ms_compsToUse[2] = { -2, -2 };
int8 CVehicleModelInfo::ms_compsUsed[2];
RwRGBA CVehicleModelInfo::ms_vehicleColourTable[256];
RwTexture *CVehicleModelInfo::ms_colourTextureTable[256];

RwTexture *gpWhiteTexture;
RwFrame *pMatFxIdentityFrame;

enum {
	VEHICLE_FLAG_COLLAPSE	= 0x2,
	VEHICLE_FLAG_ADD_WHEEL	= 0x4,
	VEHICLE_FLAG_POS	= 0x8,
	VEHICLE_FLAG_DOOR	= 0x10,
	VEHICLE_FLAG_LEFT	= 0x20,
	VEHICLE_FLAG_RIGHT	= 0x40,
	VEHICLE_FLAG_FRONT	= 0x80,
	VEHICLE_FLAG_REAR	= 0x100,
	VEHICLE_FLAG_COMP	= 0x200,
	VEHICLE_FLAG_DRAWLAST	= 0x400,
	VEHICLE_FLAG_WINDSCREEN	= 0x800,
	VEHICLE_FLAG_ANGLECULL	= 0x1000,
	VEHICLE_FLAG_REARDOOR	= 0x2000,
	VEHICLE_FLAG_FRONTDOOR	= 0x4000,
};

RwObjectNameIdAssocation carIds[] = {
	{ "wheel_rf_dummy",	CAR_WHEEL_RF,	VEHICLE_FLAG_RIGHT | VEHICLE_FLAG_ADD_WHEEL },
	{ "wheel_rm_dummy",	CAR_WHEEL_RM,	VEHICLE_FLAG_RIGHT | VEHICLE_FLAG_ADD_WHEEL },
	{ "wheel_rb_dummy",	CAR_WHEEL_RB,	VEHICLE_FLAG_RIGHT | VEHICLE_FLAG_ADD_WHEEL },
	{ "wheel_lf_dummy",	CAR_WHEEL_LF,	VEHICLE_FLAG_LEFT | VEHICLE_FLAG_ADD_WHEEL },
	{ "wheel_lm_dummy",	CAR_WHEEL_LM,	VEHICLE_FLAG_LEFT | VEHICLE_FLAG_ADD_WHEEL },
	{ "wheel_lb_dummy",	CAR_WHEEL_LB,	VEHICLE_FLAG_LEFT | VEHICLE_FLAG_ADD_WHEEL },
	{ "bump_front_dummy",	CAR_BUMP_FRONT,	VEHICLE_FLAG_FRONT | VEHICLE_FLAG_COLLAPSE },
	{ "bonnet_dummy",	CAR_BONNET,	VEHICLE_FLAG_COLLAPSE },
	{ "wing_rf_dummy",	CAR_WING_RF,	VEHICLE_FLAG_COLLAPSE },
	{ "wing_rr_dummy",	CAR_WING_RR,	VEHICLE_FLAG_RIGHT | VEHICLE_FLAG_COLLAPSE },
	{ "door_rf_dummy",	CAR_DOOR_RF,	VEHICLE_FLAG_FRONTDOOR | VEHICLE_FLAG_ANGLECULL | VEHICLE_FLAG_RIGHT | VEHICLE_FLAG_DOOR | VEHICLE_FLAG_COLLAPSE },
	{ "door_rr_dummy",	CAR_DOOR_RR,	VEHICLE_FLAG_REARDOOR | VEHICLE_FLAG_ANGLECULL | VEHICLE_FLAG_REAR | VEHICLE_FLAG_RIGHT | VEHICLE_FLAG_DOOR | VEHICLE_FLAG_COLLAPSE },
	{ "wing_lf_dummy",	CAR_WING_LF,	VEHICLE_FLAG_COLLAPSE },
	{ "wing_lr_dummy",	CAR_WING_LR,	VEHICLE_FLAG_LEFT | VEHICLE_FLAG_COLLAPSE },
	{ "door_lf_dummy",	CAR_DOOR_LF,	VEHICLE_FLAG_FRONTDOOR | VEHICLE_FLAG_ANGLECULL | VEHICLE_FLAG_LEFT | VEHICLE_FLAG_DOOR | VEHICLE_FLAG_COLLAPSE },
	{ "door_lr_dummy",	CAR_DOOR_LR,	VEHICLE_FLAG_REARDOOR | VEHICLE_FLAG_ANGLECULL | VEHICLE_FLAG_REAR | VEHICLE_FLAG_LEFT | VEHICLE_FLAG_DOOR | VEHICLE_FLAG_COLLAPSE },
	{ "boot_dummy",		CAR_BOOT,	VEHICLE_FLAG_REAR | VEHICLE_FLAG_COLLAPSE },
	{ "bump_rear_dummy",	CAR_BUMP_REAR,	VEHICLE_FLAG_REAR | VEHICLE_FLAG_COLLAPSE },
	{ "windscreen_dummy",	CAR_WINDSCREEN,	VEHICLE_FLAG_WINDSCREEN | VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_FRONT | VEHICLE_FLAG_COLLAPSE },

#ifdef VICEEXT_IMPROVED_VEHICLES_2
	{ "steeringwheel",	CAR_STEERINGWHEEL,	VEHICLE_FLAG_LEFT },
	{ "headlight_l",		CAR_HEADLIGHT_L,	VEHICLE_FLAG_LEFT | VEHICLE_FLAG_DRAWLAST },
	{ "headlight_l_hi",		CAR_HEADLIGHT_L,	VEHICLE_FLAG_LEFT | VEHICLE_FLAG_DRAWLAST },
	{ "headlight_r",		CAR_HEADLIGHT_R,	VEHICLE_FLAG_RIGHT | VEHICLE_FLAG_DRAWLAST },
	{ "headlight_r_hi",		CAR_HEADLIGHT_R,	VEHICLE_FLAG_RIGHT | VEHICLE_FLAG_DRAWLAST },
	{ "taillight_l",		CAR_TAILLIGHT_L,	VEHICLE_FLAG_LEFT },
	{ "taillight_r",		CAR_TAILLIGHT_R,	VEHICLE_FLAG_RIGHT },
	{ "reversinglight_l",	CAR_REVERSINGLIGHT_L,	VEHICLE_FLAG_LEFT },
	{ "reversinglight_r",	CAR_REVERSINGLIGHT_R,	VEHICLE_FLAG_RIGHT },
	{ "brakelight_l",		CAR_BRAKELIGHT_L,	VEHICLE_FLAG_LEFT },
	{ "brakelight_r",		CAR_BRAKELIGHT_R,	VEHICLE_FLAG_RIGHT },
	{ "servicelights_0",		CAR_SERVICELIGHTS_0,	VEHICLE_FLAG_DRAWLAST },
	{ "servicelights_1",		CAR_SERVICELIGHTS_1,	VEHICLE_FLAG_DRAWLAST },
	{ "servicelights_2",		CAR_SERVICELIGHTS_2,	VEHICLE_FLAG_DRAWLAST },
	{ "servicelights_3",		CAR_SERVICELIGHTS_3,	VEHICLE_FLAG_DRAWLAST },
	{ "clearance_lights",		CAR_CLEARANCE_LIGHTS,	VEHICLE_FLAG_DRAWLAST },
	{ "indicator_lf",		CAR_INDICATOR_LF,	VEHICLE_FLAG_LEFT },
	{ "indicator_rf",		CAR_INDICATOR_RF,	VEHICLE_FLAG_RIGHT },
	{ "indicator2_lf",		CAR_INDICATOR_2_LF,	VEHICLE_FLAG_LEFT },
	{ "indicator2_rf",		CAR_INDICATOR_2_RF,	VEHICLE_FLAG_RIGHT },
	{ "indicator_lr",		CAR_INDICATOR_LR,	VEHICLE_FLAG_LEFT },
	{ "indicator_rr",		CAR_INDICATOR_RR,	VEHICLE_FLAG_RIGHT },
	{ "indicator2_lr",		CAR_INDICATOR_2_LR,	VEHICLE_FLAG_LEFT },
	{ "indicator2_rr",		CAR_INDICATOR_2_RR,	VEHICLE_FLAG_RIGHT },
#endif

	{ "ped_frontseat",	CAR_POS_FRONTSEAT,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "ped_backseat",	CAR_POS_BACKSEAT,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "headlights",		CAR_POS_HEADLIGHTS,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "taillights",		CAR_POS_TAILLIGHTS,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "exhaust",		CAR_POS_EXHAUST,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "engine",			CAR_POS_ENGINE,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "overheat",		CAR_POS_OVERHEAT,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "overheat2",		CAR_POS_OVERHEAT_2,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
#ifdef VICEEXT_IMPROVED_VEHICLES_2
	{ "headlights2",		CAR_POS_HEADLIGHTS_2,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "taillights2",		CAR_POS_TAILLIGHTS_2,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "reversinglights",	CAR_POS_REVERSINGLIGHTS,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "reversinglights2",	CAR_POS_REVERSINGLIGHTS_2,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "brakelights",		CAR_POS_BRAKELIGHTS,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "brakelights2",		CAR_POS_BRAKELIGHTS_2,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "indicators_f",		CAR_POS_INDICATORS_F,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "indicators_r",		CAR_POS_INDICATORS_R,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "indicators2_f",		CAR_POS_INDICATORS_2_F,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "indicators2_r",		CAR_POS_INDICATORS_2_R,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "petrolcap",		CAR_POS_PETROLCAP,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
#endif
	{ "extra1",		0, VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_COMP | CLUMP_FLAG_NO_HIERID },
	{ "extra2",		0, VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_COMP | CLUMP_FLAG_NO_HIERID },
	{ "extra3",		0, VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_COMP | CLUMP_FLAG_NO_HIERID },
	{ "extra4",		0, VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_COMP | CLUMP_FLAG_NO_HIERID },
	{ "extra5",		0, VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_COMP | CLUMP_FLAG_NO_HIERID },
	{ "extra6",		0, VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_COMP | CLUMP_FLAG_NO_HIERID },
	{ nil, 0, 0 }
};

RwObjectNameIdAssocation boatIds[] = {
	{ "boat_moving_hi",	BOAT_MOVING,	0 },
	{ "boat_rudder_hi",	BOAT_RUDDER,	0 },
	{ "boat_flap_left",	BOAT_FLAP_LEFT,	0 },
	{ "boat_flap_right",	BOAT_FLAP_RIGHT,	0 },
	{ "boat_rearflap_left",	BOAT_REARFLAP_LEFT,	0 },
	{ "boat_rearflap_right",	BOAT_REARFLAP_RIGHT,	0 },
#ifdef FIX_BUGS
	// let's just accept both
	{ "windscreen",		BOAT_WINDSCREEN,	VEHICLE_FLAG_WINDSCREEN | VEHICLE_FLAG_DRAWLAST },
	{ "windscreen_hi_ok",		BOAT_WINDSCREEN,	VEHICLE_FLAG_WINDSCREEN | VEHICLE_FLAG_DRAWLAST },
#else
#ifdef GTA_PS2
	{ "windscreen",		BOAT_WINDSCREEN,	VEHICLE_FLAG_WINDSCREEN | VEHICLE_FLAG_DRAWLAST },
#else
	{ "windscreen_hi_ok",		BOAT_WINDSCREEN,	VEHICLE_FLAG_WINDSCREEN | VEHICLE_FLAG_DRAWLAST },
#endif
#endif
	{ "ped_frontseat",	BOAT_POS_FRONTSEAT,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
#ifdef VICEEXT_SP_BOAT_EXTRAS
	{ "boat_moving_high",	BOAT_MOVING,	0 },
	{ "extra1",		0, VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_COMP | CLUMP_FLAG_NO_HIERID },
	{ "extra2",		0, VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_COMP | CLUMP_FLAG_NO_HIERID },
	{ "extra3",		0, VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_COMP | CLUMP_FLAG_NO_HIERID },
	{ "extra4",		0, VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_COMP | CLUMP_FLAG_NO_HIERID },
	{ "extra5",		0, VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_COMP | CLUMP_FLAG_NO_HIERID },
	{ "extra6",		0, VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_COMP | CLUMP_FLAG_NO_HIERID },
#endif
	{ nil, 0, 0 }
};

RwObjectNameIdAssocation trainIds[] = {
	{ "door_lhs_dummy",	TRAIN_DOOR_LHS,	VEHICLE_FLAG_LEFT | VEHICLE_FLAG_COLLAPSE },
	{ "door_rhs_dummy",	TRAIN_DOOR_RHS,	VEHICLE_FLAG_LEFT | VEHICLE_FLAG_COLLAPSE },
	{ "light_front",	TRAIN_POS_LIGHT_FRONT,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "light_rear",		TRAIN_POS_LIGHT_REAR,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "ped_left_entry",	TRAIN_POS_LEFT_ENTRY,	VEHICLE_FLAG_DOOR | VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "ped_mid_entry",	TRAIN_POS_MID_ENTRY,	VEHICLE_FLAG_DOOR | VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "ped_right_entry",	TRAIN_POS_RIGHT_ENTRY,	VEHICLE_FLAG_DOOR | VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ nil, 0, 0 }
};

RwObjectNameIdAssocation heliIds[] = {
	{ "chassis_dummy",	HELI_CHASSIS,	VEHICLE_FLAG_COLLAPSE },
	{ "toprotor",		HELI_TOPROTOR,	0 },
	{ "backrotor",		HELI_BACKROTOR,	0 },
	{ "tail",		HELI_TAIL,	0 },
	{ "topknot",		HELI_TOPKNOT,	0 },
	{ "skid_left",		HELI_SKID_LEFT,	0 },
	{ "skid_right",		HELI_SKID_RIGHT,	0 },
	{ nil, 0, 0 }
};

RwObjectNameIdAssocation planeIds[] = {
	{ "wheel_front_dummy",	PLANE_WHEEL_FRONT,	0 },
	{ "wheel_rear_dummy",	PLANE_WHEEL_READ,	0 },
	{ "light_tailplane",	PLANE_POS_LIGHT_TAIL,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "light_left",		PLANE_POS_LIGHT_LEFT,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "light_right",	PLANE_POS_LIGHT_RIGHT,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ nil, 0, 0 }
};

RwObjectNameIdAssocation bikeIds[] = {
	{ "chassis_dummy",	BIKE_CHASSIS,	0 },
	{ "forks_front",	BIKE_FORKS_FRONT,	0 },
	{ "forks_rear",		BIKE_FORKS_REAR,	0 },
	{ "wheel_front",	BIKE_WHEEL_FRONT,	0 },
	{ "wheel_rear",		BIKE_WHEEL_REAR,	0 },
	{ "mudguard",		BIKE_MUDGUARD,	0 },
	{ "handlebars",		BIKE_HANDLEBARS,	0 },
#ifdef VICEEXT_IMPROVED_VEHICLES_2
	{ "headlight_l",	BIKE_HEADLIGHT_L,	VEHICLE_FLAG_FRONT },
	{ "taillight_l",	BIKE_TAILLIGHT_L,	VEHICLE_FLAG_LEFT },
	{ "indicator_lf",	BIKE_INDICATOR_LF,	VEHICLE_FLAG_LEFT },
	{ "indicator_lr",	BIKE_INDICATOR_LR,	VEHICLE_FLAG_LEFT },
	{ "indicator_rf",	BIKE_INDICATOR_RF,	VEHICLE_FLAG_RIGHT },
	{ "indicator_rr",	BIKE_INDICATOR_RR,	VEHICLE_FLAG_RIGHT },
#endif
	{ "ped_frontseat",	CAR_POS_FRONTSEAT,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "ped_backseat",	CAR_POS_BACKSEAT,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "headlights",		CAR_POS_HEADLIGHTS,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "taillights",		CAR_POS_TAILLIGHTS,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "exhaust",		CAR_POS_EXHAUST,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "overheat",		CAR_POS_OVERHEAT,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "overheat2",		CAR_POS_OVERHEAT_2,	VEHICLE_FLAG_POS | CLUMP_FLAG_NO_HIERID },
	{ "extra1",		0,	VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_COMP | CLUMP_FLAG_NO_HIERID },
	{ "extra2",		0,	VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_COMP | CLUMP_FLAG_NO_HIERID },
	{ "extra3",		0,	VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_COMP | CLUMP_FLAG_NO_HIERID },
	{ "extra4",		0,	VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_COMP | CLUMP_FLAG_NO_HIERID },
	{ "extra5",		0,	VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_COMP | CLUMP_FLAG_NO_HIERID },
	{ "extra6",		0,	VEHICLE_FLAG_DRAWLAST | VEHICLE_FLAG_COMP | CLUMP_FLAG_NO_HIERID },
	{ nil, 0, 0 }
};

RwObjectNameIdAssocation *CVehicleModelInfo::ms_vehicleDescs[] = {
	carIds,
	boatIds,
	trainIds,
	heliIds,
	planeIds,
	bikeIds
};

bool gbBlackCars;
bool gbPinkCars;

CVehicleModelInfo::CVehicleModelInfo(void)
 : CClumpModelInfo(MITYPE_VEHICLE)
{
	int32 i;
	for(i = 0; i < NUM_VEHICLE_POSITIONS; i++){
		m_positions[i].x = 0.0f;
		m_positions[i].y = 0.0f;
		m_positions[i].z = 0.0f;
	}
	m_numColours = 0;
	m_animFileIndex = -1;
#ifdef VICEEXT_IMPROVED_VEHICLES_2
	bNewLights = false;
	lightsOffTexture = nil;
	lightsOnTexture = nil;
	serviceLightsOffTexture = nil;
	serviceLightsOnTexture = nil;
	for(i = 0; i < VICEEXT_LENS_ATLAS_PAIRS; i++){
		indicatorLensOff[i] = nil;
		indicatorLensOn[i] = nil;
	}
	numIndicatorLensAtlases = 0;
	newLightsData = tNewLightsData();
#endif
}

void
CVehicleModelInfo::DeleteRwObject(void)
{
	int32 i;
	RwFrame *f;

	for(i = 0; i < m_numComps; i++){
		f = RpAtomicGetFrame(m_comps[i]);
		RpAtomicDestroy(m_comps[i]);
		RwFrameDestroy(f);
	}
	m_numComps = 0;
	CClumpModelInfo::DeleteRwObject();
}

static RpClump *gViceExtCompClump;	// clump al que van los clones

static RwObject*
ViceExtCloneCompAtomicCB(RwObject *object, void *data)
{
	RpAtomic *atomic = RpAtomicClone((RpAtomic*)object);
	RpAtomicSetFrame(atomic, (RwFrame*)data);
	RpClumpAddAtomic(gViceExtCompClump, atomic);
	return object;
}

static RwFrame*
ViceExtCloneCompFrameCB(RwFrame *frame, void *data)
{
	RwFrame *newFrame = RwFrameCreate();
	RwFrameAddChild((RwFrame*)data, newFrame);
	RwFrameTransform(newFrame, RwFrameGetMatrix(frame), rwCOMBINEREPLACE);
	if (GetFrameNodeName(frame) != nil)
		strncpy(GetFrameNodeName(newFrame), GetFrameNodeName(frame), 23);
	CVisibilityPlugins::SetFrameHierarchyId(newFrame, CVisibilityPlugins::GetFrameHierarchyId(frame));
	RwFrameForAllObjects(frame, ViceExtCloneCompAtomicCB, newFrame);
	RwFrameForAllChildren(frame, ViceExtCloneCompFrameCB, newFrame);
	return newFrame;	// no-nil = sigue con los hermanos (librw para con nil)
}

static RwFrame*
ViceExtCloneComponent(RpClump *clump, RpAtomic *comp)
{
	RwFrame *src = RpAtomicGetFrame(comp);
	RwFrame *dst = RwFrameCreate();
	RwFrameTransform(dst, RwFrameGetMatrix(src), rwCOMBINEREPLACE);
	gViceExtCompClump = clump;
	RwFrameForAllObjects(src, ViceExtCloneCompAtomicCB, dst);
	RwFrameForAllChildren(src, ViceExtCloneCompFrameCB, dst);
	return dst;
}

RwObject*
CVehicleModelInfo::CreateInstance(void)
{
	RpClump *clump;
	RwFrame *extrasframe, *f;
	int32 comp1, comp2;

	clump = (RpClump*)CClumpModelInfo::CreateInstance();
	if(clump == nil)
		return nil;
	if(m_numComps != 0){
		extrasframe = RpClumpGetFrame(clump);
#ifdef VICEEXT_SP_BIKE_EXTRAS_FRAME
		if(m_vehicleType == VEHICLE_TYPE_HELI || m_vehicleType == VEHICLE_TYPE_BIKE){
			RwFrame *extras = CClumpModelInfo::GetFrameFromId(clump, 1);
			if(extras != nil)
				extrasframe = extras;
		}
#endif

		comp1 = ChooseComponent();
		if(comp1 != -1 && m_comps[comp1]){
			f = ViceExtCloneComponent(clump, m_comps[comp1]);
			RwFrameAddChild(extrasframe, f);
		}
		ms_compsUsed[0] = comp1;

		comp2 = ChooseSecondComponent();
		if(comp2 != -1 && m_comps[comp2]){
			f = ViceExtCloneComponent(clump, m_comps[comp2]);
			RwFrameAddChild(extrasframe, f);
		}
		ms_compsUsed[1] = comp2;
	}else{
		ms_compsUsed[0] = -1;
		ms_compsUsed[1] = -1;
#ifdef VICEEXT_SP_COMPS_TO_USE_RESET
		ms_compsToUse[0] = -2;
		ms_compsToUse[1] = -2;
#endif
	}
	return (RwObject*)clump;
}

static RwObject*
SetWheelRendererCB(RwObject *object, void *data)
{
	if(RwObjectGetType(object) == rpATOMIC)
		CVehicleModelInfo::SetAtomicRendererCB((RpAtomic*)object, data);
	return object;
}

#ifdef __EMSCRIPTEN__
// La malla (chunk 0x50E) de muchos vehiculos del mod se convirtio con indices
// sustituidos por 0, el marcador de costura del conversor. El vertice 0 es el
// origen del coche, asi que esos triangulos se estiran hasta el centro del
// modelo (se ven dentro de la cabina, en puertas y en el capo). La lista de
// triangulos del DFF si es fiel al modelo: se reconstruye la malla desde ella
// una vez, al cargar el vehiculo, en lugar de parchear coches a mano.
static bool
OdVehicleGeometryValid(RpGeometry *geo)
{
	if(geo->triangles == nil || geo->meshHeader == nil || (geo->flags & rpGEOMETRYNATIVE))
		return false;
	if(geo->numTriangles <= 0 || geo->numTriangles > 1333333 ||
	   geo->numVertices <= 0 || geo->numVertices > 65535 || geo->matList.numMaterials <= 0)
		return false;
	for(int32 t = 0; t < geo->numTriangles; t++){
		rw::Triangle *tri = &geo->triangles[t];
		if(tri->matId >= geo->matList.numMaterials || geo->matList.materials[tri->matId] == nil ||
		   tri->v[0] >= geo->numVertices || tri->v[1] >= geo->numVertices || tri->v[2] >= geo->numVertices)
			return false;
	}
	return true;
}

// Meshes de la malla reconstruida: solo los materiales que tienen triangulos.
// Un mesh con 0 indices rompe el instanciado (findMinVertAndNumVertices), asi
// que no se puede usar Geometry::buildMeshes tal cual.
static int32
OdVehicleUsedMaterials(RpGeometry *geo, bool *used)
{
	int32 count = 0;
	for(int32 m = 0; m < geo->matList.numMaterials; m++){
		used[m] = false;
		for(int32 t = 0; t < geo->numTriangles; t++)
			if(geo->triangles[t].matId == m){
				used[m] = true;
				count++;
				break;
			}
	}
	return count;
}

static bool
OdVehicleMeshMatches(RpGeometry *geo, bool *used, int32 usedCount)
{
	rw::MeshHeader *header = geo->meshHeader;
	if(header->flags != 0 || header->numMeshes != usedCount ||
	   header->totalIndices != (uint32)geo->numTriangles*3)
		return false;
	rw::Mesh *meshes = header->getMeshes();
	int32 slot = 0;
	for(int32 m = 0; m < geo->matList.numMaterials; m++){
		if(!used[m])
			continue;
		rw::Mesh *mesh = &meshes[slot++];
		if(mesh->material != geo->matList.materials[m] || mesh->indices == nil)
			return false;
		uint32 index = 0;
		for(int32 t = 0; t < geo->numTriangles; t++){
			rw::Triangle *tri = &geo->triangles[t];
			if(tri->matId != m)
				continue;
			if(index + 2 >= mesh->numIndices ||
			   mesh->indices[index] != tri->v[0] ||
			   mesh->indices[index + 1] != tri->v[1] ||
			   mesh->indices[index + 2] != tri->v[2])
				return false;
			index += 3;
		}
		if(index != mesh->numIndices)
			return false;
	}
	return true;
}

// Devuelve true si hubo que reconstruir la malla (el modelo estaba corrupto).
static bool
OdRebuildVehicleMesh(RpGeometry *geo)
{
	bool *used = rwNewT(bool, geo->matList.numMaterials, rw::MEMDUR_EVENT | rw::ID_GEOMETRY);
	int32 usedCount = OdVehicleUsedMaterials(geo, used);
	bool matches = OdVehicleMeshMatches(geo, used, usedCount);
	if(!matches){
		rwFree(geo->meshHeader);
		geo->meshHeader = nil;
		geo->flags &= ~rw::Geometry::TRISTRIP;
		rw::MeshHeader *header = geo->allocateMeshes(usedCount, (uint32)geo->numTriangles*3, false);
		rw::Mesh *meshes = header->getMeshes();
		int32 slot = 0;
		for(int32 m = 0; m < geo->matList.numMaterials; m++){
			if(!used[m])
				continue;
			int32 count = 0;
			for(int32 t = 0; t < geo->numTriangles; t++)
				if(geo->triangles[t].matId == m)
					count += 3;
			meshes[slot].material = geo->matList.materials[m];
			meshes[slot].numIndices = count;
			slot++;
		}
		header->setupIndices();
		slot = 0;
		for(int32 m = 0; m < geo->matList.numMaterials; m++){
			if(!used[m])
				continue;
			uint32 index = 0;
			for(int32 t = 0; t < geo->numTriangles; t++){
				rw::Triangle *tri = &geo->triangles[t];
				if(tri->matId != m)
					continue;
				meshes[slot].indices[index++] = tri->v[0];
				meshes[slot].indices[index++] = tri->v[1];
				meshes[slot].indices[index++] = tri->v[2];
			}
			slot++;
		}
	}
	rwFree(used);
	return !matches;
}

static RpAtomic*
OdRepairVehicleMesh(RpAtomic *atomic, void *data)
{
	RpGeometry *geo = RpAtomicGetGeometry(atomic);
	if(geo == nil || !OdVehicleGeometryValid(geo))
		return atomic;
	if(OdRebuildVehicleMesh(geo)){
		static int32 odReported = 0;
		if(odReported < 256){
			RwFrame *frame = RpAtomicGetFrame(atomic);
			const char *name = frame ? GetFrameNodeName(frame) : nil;
			char message[192];
			snprintf(message, sizeof message, "VEHMESH reconstruida modelo=%s atomic=%s tris=%d",
				(const char*)data, name ? name : "?", geo->numTriangles);
			ODTRACES(message);
			odReported++;
		}
	}
	return atomic;
}
#endif

#ifdef VICEEXT_IMPROVED_VEHICLES_2
struct ViceExtLensAtlasData {
	RwTexDictionary *txd;
	RwTexture *off[VICEEXT_LENS_ATLAS_PAIRS];
	RwTexture *on[VICEEXT_LENS_ATLAS_PAIRS];
	uint8 num;
};

// El motor pinta POR material, asi que una cara de lente cuyo UV cae a medias
// entre la lente y el cromado del MISMO atlas sale media apagada por mucho
// color que se le escriba (plan 46.0 C1). El dato derivado se llama
// `<atlas>_lens` y aqui se empareja con su original una vez por modelo; se
// guarda el PUNTERO del original porque es lo que deja fuera cualquier material
// que no dibuja con ese atlas (el parachoques comparte atlas con la lente, y
// sus texelas si re-tienen: enlazarlo por nombre lo pintaria de ambar).
static RwTexture *
ViceExtLensAtlasCB(RwTexture *texture, void *data)
{
	ViceExtLensAtlasData *d = (ViceExtLensAtlasData*)data;
	static const char suffix[] = "_lens";
	const size_t slen = sizeof(suffix) - 1;
	const char *name = (const char*)texture->name;
	size_t len = strlen(name);
	char base[32];
	if(texture == nil || d->num >= VICEEXT_LENS_ATLAS_PAIRS ||
	   len <= slen || len - slen >= sizeof(base) ||
	   memcmp(name + len - slen, suffix, slen) != 0)
		return texture;
	memcpy(base, name, len - slen);
	base[len - slen] = '\0';
	RwTexture *off = RwTexDictionaryFindNamedTexture(d->txd, base);
	if(off == nil)
		return texture;
	d->off[d->num] = off;
	d->on[d->num] = texture;
	d->num++;
	return texture;
}
#endif

void
CVehicleModelInfo::SetClump(RpClump *clump)
{
	CClumpModelInfo::SetClump(clump);
#ifdef __EMSCRIPTEN__
	RpClumpForAllAtomics(clump, OdRepairVehicleMesh, m_name);
#endif
	SetAtomicRenderCallbacks();
	SetFrameIds(ms_vehicleDescs[m_vehicleType]);
	PreprocessHierarchy();
	FindEditableMaterialList();
	SetEnvironmentMap();

#ifdef VICEEXT_IMPROVED_VEHICLES_2
	RwTexDictionary *modelTxd = CTxdStore::GetSlot(GetTxdSlot())->texDict;
	if(modelTxd){
		lightsOffTexture = RwTexDictionaryFindNamedTexture(modelTxd, "lights");
		lightsOnTexture = RwTexDictionaryFindNamedTexture(modelTxd, "lightson");
		serviceLightsOffTexture = RwTexDictionaryFindNamedTexture(modelTxd, "servicelights");
		serviceLightsOnTexture = RwTexDictionaryFindNamedTexture(modelTxd, "servicelightson");
		ViceExtLensAtlasData lens;
		lens.txd = modelTxd;
		lens.num = 0;
		RwTexDictionaryForAllTextures(modelTxd, ViceExtLensAtlasCB, &lens);
		numIndicatorLensAtlases = lens.num;
		for(int32 i = 0; i < VICEEXT_LENS_ATLAS_PAIRS; i++){
			indicatorLensOff[i] = i < lens.num ? lens.off[i] : nil;
			indicatorLensOn[i] = i < lens.num ? lens.on[i] : nil;
		}
	}
#endif

	if(m_vehicleType == VEHICLE_TYPE_CAR){
		RwFrame *odWheel = VICEEXT_VOLANTE ? ViceExtAddSteeringWheel(m_clump, m_name) : nil;
		if(odWheel){
			CVisibilityPlugins::SetFrameHierarchyId(odWheel, CAR_STEERINGWHEEL);
			RwFrameForAllObjects(odWheel, SetWheelRendererCB, m_clump);
		}
	}
}

void
CVehicleModelInfo::SetAnimFile(const char *file)
{
	if(strcasecmp(file, "null") == 0)
		return;

	m_animFileName = new char[strlen(file)+1];
	strcpy(m_animFileName, file);
}

void
CVehicleModelInfo::ConvertAnimFileIndex(void)
{
	if(m_animFileIndex != -1){
		// we have a string pointer in that union
		int32 index = CAnimManager::GetAnimationBlockIndex(m_animFileName);
		delete[] m_animFileName;
		m_animFileIndex = index;
	}
}

RwFrame*
CVehicleModelInfo::CollapseFramesCB(RwFrame *frame, void *data)
{
#ifdef VICEEXT_IMPROVED_VEHICLES_2
	if(strstr(GetFrameNodeName(frame), "light_l") ||
	   strstr(GetFrameNodeName(frame), "light_r") ||
	   strstr(GetFrameNodeName(frame), "indicator"))
		return frame;
#endif
	RwFrameForAllChildren(frame, CollapseFramesCB, data);
	RwFrameForAllObjects(frame, MoveObjectsCB, data);
	if(frame->child == nil)
		RwFrameDestroy(frame);
	return frame;
}

RwObject*
CVehicleModelInfo::MoveObjectsCB(RwObject *object, void *data)
{
	RpAtomicSetFrame((RpAtomic*)object, (RwFrame*)data);
	return object;
}

RpAtomic*
CVehicleModelInfo::HideDamagedAtomicCB(RpAtomic *atomic, void *data)
{
	if(strstr(GetFrameNodeName(RpAtomicGetFrame(atomic)), "_dam")){
		RpAtomicSetFlags(atomic, 0);
		CVisibilityPlugins::SetAtomicFlag(atomic, ATOMIC_FLAG_DAM);
	}else if(strstr(GetFrameNodeName(RpAtomicGetFrame(atomic)), "_ok"))
		CVisibilityPlugins::SetAtomicFlag(atomic, ATOMIC_FLAG_OK);
	return atomic;
}

RpAtomic*
CVehicleModelInfo::HideAllComponentsAtomicCB(RpAtomic *atomic, void *data)
{
	if(CVisibilityPlugins::GetAtomicId(atomic) & (uintptr)data)
		RpAtomicSetFlags(atomic, 0);
	else
		RpAtomicSetFlags(atomic, rpATOMICRENDER);
	return atomic;
}

RpMaterial*
CVehicleModelInfo::HasAlphaMaterialCB(RpMaterial *material, void *data)
{
	if(RpMaterialGetColor(material)->alpha != 0xFF){
		*(bool*)data = true;
		return nil;
	}
	return material;
}

// Un material suelto con alfa no convierte la carroceria en cristal. Medido en
// el DFF: el Cheetah tiene 6 triangulos con alfa 102 de 1109 en `chassis_hi` (y
// el Police 6 de 972, alfa 101); con la regla anterior ("cualquier material con
// alfa") el chasis entero se dibujaba en la lista transparente, ordenado tarde y
// mezclado como una luna. Una luna de verdad (`window_*`, `windscreen_*`) tiene
// TODOS sus triangulos con alfa. Se exige mayoria de triangulos alfa para el
// cuerpo; los paneles y extras conservan la regla original.
static bool
OdGeometryMostlyAlpha(RpGeometry *geo)
{
	int32 i, total, alpha;

	if(geo == nil || geo->triangles == nil || geo->matList.numMaterials <= 0)
		return false;
	total = geo->numTriangles;
	if(total <= 0)
		return false;
	alpha = 0;
	for(i = 0; i < total; i++){
		int32 m = geo->triangles[i].matId;
		if(m >= 0 && m < geo->matList.numMaterials){
			RpMaterial *mat = geo->matList.materials[m];
			if(mat && RpMaterialGetColor(mat)->alpha != 0xFF)
				alpha++;
		}
	}
	return 2*alpha >= total;
}


static void
ViceExtCabinaTrace(RpClump *clump, const char *atomicName, bool cristal, bool transparente)
{
#ifdef __EMSCRIPTEN__
	static char odLastModel[32];
	char odT[240];
	CVehicleModelInfo *vmi = (CVehicleModelInfo*)CVisibilityPlugins::GetClumpModelInfo(clump);
	const char *odModel = vmi ? vmi->GetModelName() : "?";
	if(!cristal)
		return;
	if(!odLastModel[0] || strcmp(odLastModel, odModel)){
		strncpy(odLastModel, odModel, 31);
		odLastModel[31] = 0;
		snprintf(odT, sizeof odT,
			"VICEEXT cabina modelo=%s atomic=%s con cristal -> pase %s",
			odLastModel, atomicName ? atomicName : "?",
			transparente ? "transparente" : "normal");
		ODTRACES(odT);
	}
#else
	(void)clump; (void)atomicName; (void)cristal; (void)transparente;
#endif
}


RpAtomic*
CVehicleModelInfo::SetAtomicRendererCB(RpAtomic *atomic, void *data)
{
	RpClump *clump;
	char *name;
	bool alpha;
	bool esExtra;
	bool esWindscreen;
	bool esVolante;
	bool transparente;

	clump = (RpClump*)data;
	name = GetFrameNodeName(RpAtomicGetFrame(atomic));
	alpha = false;
	RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), HasAlphaMaterialCB, &alpha);
	// El pase transparente es para lo que de verdad se mezcla (cristales y
	// calcomanias), no para cualquier atomic que tenga UN material con alfa: con
	// la prueba anterior media carroceria (bump_front_hi_ok, boot_hi_dam,
	// extra1/2...) entraba en la lista ordenada de alfa, se dibujaba despues del
	// personaje y sin escribir profundidad, y su textura marron se veia cruzada
	// por delante del ocupante. La mayoria de triangulos decide, como en el
	// chassis.
	if(alpha)
		alpha = OdGeometryMostlyAlpha(RpAtomicGetGeometry(atomic));
	esExtra = !CGeneral::faststrncmp(name, "extra", 5);
	esWindscreen = strncmp(name, "windscreen", 10) == 0;
	esVolante = strcmp(name, "steeringwheel") == 0;
	transparente = alpha || esWindscreen;
	ViceExtCabinaTrace(clump, name, alpha, transparente);
#ifdef VICEEXT_IMPROVED_VEHICLES_2
	if (strstr(name, "_hi") || esExtra || esVolante) {
		if (transparente)
			CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleHiDetailAlphaCB);
		else
			CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleHiDetailCB);
		if (strncmp(name, "window", 6) == 0)
			CVisibilityPlugins::SetAtomicFlag(atomic, ATOMIC_FLAG_DOORWINDOW);
		if (esExtra || esVolante)
			CVisibilityPlugins::SetAtomicFlag(atomic, ATOMIC_FLAG_NOCULL);
#else
	if(strstr(name, "_hi") || esExtra) {
		if(alpha)
			CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleHiDetailAlphaCB);
		else
			CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleHiDetailCB);
#endif
	}else if(strstr(name, "_lo")){
		RpClumpRemoveAtomic(clump, atomic);
		RpAtomicDestroy(atomic);
		return atomic;		// BUG: nil in gta
	}else if(strstr(name, "_vlo"))
		CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleReallyLowDetailCB);
	else
		CVisibilityPlugins::SetAtomicRenderCallback(atomic, nil);
	HideDamagedAtomicCB(atomic, nil);
	return atomic;
}

RpAtomic*
CVehicleModelInfo::SetAtomicRendererCB_BigVehicle(RpAtomic *atomic, void *data)
{
	char *name;
	bool alpha;
	bool esVolante;

	name = GetFrameNodeName(RpAtomicGetFrame(atomic));
	alpha = false;
	RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), HasAlphaMaterialCB, &alpha);
	esVolante = strcmp(name, "steeringwheel") == 0;
	if(strstr(name, "_hi") || !CGeneral::faststrncmp(name, "extra", 5) || esVolante) {
		if(alpha)
			CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleHiDetailAlphaCB_BigVehicle);
		else
			CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleHiDetailCB_BigVehicle);
		CVisibilityPlugins::SetAtomicFlag(atomic, ATOMIC_FLAG_NOCULL);
	}else if(strstr(name, "_lo")){
		if(alpha)
			CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleLowDetailAlphaCB_BigVehicle);
		else
			CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleLowDetailCB_BigVehicle);
	}else if(strstr(name, "_vlo"))
		CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleReallyLowDetailCB_BigVehicle);
	else
		CVisibilityPlugins::SetAtomicRenderCallback(atomic, nil);
	HideDamagedAtomicCB(atomic, nil);
	return atomic;
}

RpAtomic*
CVehicleModelInfo::SetAtomicRendererCB_Train(RpAtomic *atomic, void *data)
{
	char *name;
	bool alpha;

	name = GetFrameNodeName(RpAtomicGetFrame(atomic));
	alpha = false;
	RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), HasAlphaMaterialCB, &alpha);
	if(strstr(name, "_hi")){
		if(alpha)
			CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderTrainHiDetailAlphaCB);
		else
			CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderTrainHiDetailCB);
	}else if(strstr(name, "_vlo"))
		CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleReallyLowDetailCB_BigVehicle);
	else
		CVisibilityPlugins::SetAtomicRenderCallback(atomic, nil);
	HideDamagedAtomicCB(atomic, nil);
	return atomic;
}

RpAtomic*
CVehicleModelInfo::SetAtomicRendererCB_Boat(RpAtomic *atomic, void *data)
{
	RpClump *clump;
	char *name;
	bool alpha;

	clump = (RpClump*)data;
	name = GetFrameNodeName(RpAtomicGetFrame(atomic));
	alpha = false;
	RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), HasAlphaMaterialCB, &alpha);
	if(strcmp(name, "boat_hi") == 0 || !CGeneral::faststrncmp(name, "extra", 5))
		CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleHiDetailCB_Boat);
	else if(strstr(name, "_hi")){
		if(alpha)
			CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleHiDetailAlphaCB_Boat);
		else
			CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleHiDetailCB);
	}else if(strstr(name, "_lo")){
		RpClumpRemoveAtomic(clump, atomic);
		RpAtomicDestroy(atomic);
		return atomic;		// BUG: not done by gta
	}else if(strstr(name, "_vlo"))
		CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleLoDetailCB_Boat);
	else{
		if(alpha)
			CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleHiDetailAlphaCB_Boat);
		else
			CVisibilityPlugins::SetAtomicRenderCallback(atomic, nil);
	}
	HideDamagedAtomicCB(atomic, nil);
	return atomic;
}

RpAtomic*
CVehicleModelInfo::SetAtomicRendererCB_Heli(RpAtomic *atomic, void *data)
{
	char *name;

	name = GetFrameNodeName(RpAtomicGetFrame(atomic));
	if(strncmp(name, "toprotor", 8) == 0)
		CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleRotorAlphaCB);
	else if(strncmp(name, "rearrotor", 9) == 0)
		CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleTailRotorAlphaCB);
	else
		CVisibilityPlugins::SetAtomicRenderCallback(atomic, nil);
	return atomic;
}

RpAtomic*
CVehicleModelInfo::SetAtomicRendererCB_RealHeli(RpAtomic *atomic, void *data)
{
	RpClump *clump;
	char *name;
	bool alpha;

	clump = (RpClump*)data;
	name = GetFrameNodeName(RpAtomicGetFrame(atomic));
	alpha = false;
	RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), HasAlphaMaterialCB, &alpha);
	if(strncmp(name, "toprotor", 8) == 0)
		CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleRotorAlphaCB);
	else if(strncmp(name, "rearrotor", 9) == 0)
		CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleTailRotorAlphaCB);
	else if(strstr(name, "_hi") || !CGeneral::faststrncmp(name, "extra", 5)) {
		if(alpha || strncmp(name, "windscreen", 10) == 0)
			CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleHiDetailAlphaCB);
		else
			CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleHiDetailCB);
	}else if(strstr(name, "_lo")){
		RpClumpRemoveAtomic(clump, atomic);
		RpAtomicDestroy(atomic);
		return atomic;		// BUG: nil in gta
	}else if(strstr(name, "_vlo"))
		CVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleReallyLowDetailCB);
	else
		CVisibilityPlugins::SetAtomicRenderCallback(atomic, nil);
	HideDamagedAtomicCB(atomic, nil);
	return atomic;
}

void
CVehicleModelInfo::SetAtomicRenderCallbacks(void)
{
#ifdef GTA_TRAIN
	if(m_vehicleType == VEHICLE_TYPE_TRAIN)
		RpClumpForAllAtomics(m_clump, SetAtomicRendererCB_Train, nil);
	else
#endif
	if(m_vehicleType == VEHICLE_TYPE_HELI)
		RpClumpForAllAtomics(m_clump, SetAtomicRendererCB_Heli, nil);
	else if(m_vehicleType == VEHICLE_TYPE_PLANE)
		RpClumpForAllAtomics(m_clump, SetAtomicRendererCB_BigVehicle, nil);
	else if(m_vehicleType == VEHICLE_TYPE_BOAT)
		RpClumpForAllAtomics(m_clump, SetAtomicRendererCB_Boat, m_clump);
	else if(mod_HandlingManager.GetHandlingData((tVehicleType)m_handlingId)->Flags & HANDLING_IS_HELI)
		RpClumpForAllAtomics(m_clump, SetAtomicRendererCB_RealHeli, m_clump);
	else
		RpClumpForAllAtomics(m_clump, SetAtomicRendererCB, m_clump);
}

RwObject*
CVehicleModelInfo::SetAtomicFlagCB(RwObject *object, void *data)
{
	RpAtomic *atomic = (RpAtomic*)object;
	assert(RwObjectGetType(object) == rpATOMIC);
	CVisibilityPlugins::SetAtomicFlag(atomic, (uintptr)data);
	return object;
}

RwObject*
CVehicleModelInfo::ClearAtomicFlagCB(RwObject *object, void *data)
{
	RpAtomic *atomic = (RpAtomic*)object;
	assert(RwObjectGetType(object) == rpATOMIC);
	CVisibilityPlugins::ClearAtomicFlag(atomic, (uintptr)data);
	return object;
}

RwObject*
GetOkAndDamagedAtomicCB(RwObject *object, void *data)
{
	RpAtomic *atomic = (RpAtomic*)object;
	if(CVisibilityPlugins::GetAtomicId(atomic) & ATOMIC_FLAG_OK)
		((RpAtomic**)data)[0] = atomic;
	else if(CVisibilityPlugins::GetAtomicId(atomic) & ATOMIC_FLAG_DAM)
		((RpAtomic**)data)[1] = atomic;
	return object;
}

void
CVehicleModelInfo::PreprocessHierarchy(void)
{
	int32 i;
	RwObjectNameIdAssocation *desc;
	RwFrame *f;
	RpAtomic *atomic;
	RwV3d *rwvec;

	desc = ms_vehicleDescs[m_vehicleType];
	m_numDoors = 0;
	m_numComps = 0;

	RwFrame *odDetach[6];
	int32 odNumDetach = 0;

	for(i = 0; desc[i].name; i++){
		RwObjectNameAssociation assoc;

		if((desc[i].flags & (VEHICLE_FLAG_COMP|VEHICLE_FLAG_POS)) == 0)
			continue;
		assoc.frame = nil;
		assoc.name = desc[i].name;
		RwFrameForAllChildren(RpClumpGetFrame(m_clump),
			FindFrameFromNameWithoutIdCB, &assoc);
		if(assoc.frame == nil)
			continue;

		if(desc[i].flags & VEHICLE_FLAG_DOOR)
			m_numDoors++;

		if(desc[i].flags & VEHICLE_FLAG_POS){
			f = assoc.frame;
			rwvec = &m_positions[desc[i].hierId];
			*rwvec = *RwMatrixGetPos(RwFrameGetMatrix(f));
			for(f = RwFrameGetParent(f); f; f = RwFrameGetParent(f))
				RwV3dTransformPoints(rwvec, rwvec, 1, RwFrameGetMatrix(f));
			RwFrameDestroy(assoc.frame);
		}else{
			atomic = (RpAtomic*)GetFirstObject(assoc.frame);
			RpClumpRemoveAtomic(m_clump, atomic);
			if(odNumDetach < 6)
				odDetach[odNumDetach++] = assoc.frame;
			SetVehicleComponentFlags(assoc.frame, desc[i].flags);
			m_comps[m_numComps++] = atomic;
		}
	}

	for(i = 0; desc[i].name; i++){
		RwObjectIdAssociation assoc;

		if(desc[i].flags & (VEHICLE_FLAG_COMP|VEHICLE_FLAG_POS))
			continue;
		assoc.frame = nil;
		assoc.id = desc[i].hierId;
		RwFrameForAllChildren(RpClumpGetFrame(m_clump),
			FindFrameFromIdCB, &assoc);
		if(assoc.frame == nil)
			continue;

		if(desc[i].flags & VEHICLE_FLAG_DOOR)
			m_numDoors++;

		if(desc[i].flags & VEHICLE_FLAG_COLLAPSE){
			RpAtomic *okdam[2] = { nil, nil };
			RwFrameForAllChildren(assoc.frame, CollapseFramesCB, assoc.frame);
			RwFrameUpdateObjects(assoc.frame);
			RwFrameForAllObjects(assoc.frame, GetOkAndDamagedAtomicCB, okdam);
			if(okdam[0] && okdam[1])
				RpAtomicSetRenderCallBack(okdam[1], RpAtomicGetRenderCallBack(okdam[0]));
		}

		SetVehicleComponentFlags(assoc.frame, desc[i].flags);

#ifdef VICEEXT_IMPROVED_VEHICLES_2
		if(!CGeneral::faststrncmp(desc[i].name, "headlight_l", 11) ||
		   !CGeneral::faststrncmp(desc[i].name, "headlight_l_hi", 14) ||
		   !CGeneral::faststrncmp(desc[i].name, "taillight_l", 11))
			bNewLights = true;

		RwFrame *parentFrame = RwFrameGetParent(assoc.frame);
		const char *parentName = parentFrame ? GetFrameNodeName(parentFrame) : "";
		if(!CGeneral::faststrncmp(parentName, "wing_lf_dummy", 11)){
			if(!CGeneral::faststrncmp(desc[i].name, "headlight_l", 11))
				newLightsData.bWingHeadlights = true;
			else if(!CGeneral::faststrncmp(desc[i].name, "indicator_lf", 12))
				newLightsData.bWingIndicators1 = true;
			else if(!CGeneral::faststrncmp(desc[i].name, "indicator2_lf", 13))
				newLightsData.bWingIndicators2 = true;
		}else if(!CGeneral::faststrncmp(parentName, "bump_front_dummy", 16)){
			if(!CGeneral::faststrncmp(desc[i].name, "headlight_l", 11))
				newLightsData.bBumperHeadlights = true;
			else if(!CGeneral::faststrncmp(desc[i].name, "indicator_lf", 12))
				newLightsData.bBumperForwardIndicators1 = true;
			else if(!CGeneral::faststrncmp(desc[i].name, "indicator2_lf", 13))
				newLightsData.bBumperForwardIndicators2 = true;
		}else if(!CGeneral::faststrncmp(parentName, "bump_rear_dummy", 16)){
			if(!CGeneral::faststrncmp(desc[i].name, "taillight_l", 11))
				newLightsData.bBumperTaillights = true;
			else if(!CGeneral::faststrncmp(desc[i].name, "indicator_lr", 12))
				newLightsData.bBumperRearIndicators1 = true;
			else if(!CGeneral::faststrncmp(desc[i].name, "indicator2_lr", 13))
				newLightsData.bBumperRearIndicators2 = true;
			else if(!CGeneral::faststrncmp(desc[i].name, "brakelight_l", 12))
				newLightsData.bBumperBrakelights = true;
			else if(!CGeneral::faststrncmp(desc[i].name, "reversinglight_l", 16))
				newLightsData.bBumperReversinglights = true;
		}
#endif

		if(desc[i].flags & VEHICLE_FLAG_ADD_WHEEL){
			if(m_wheelId == -1)
				RwFrameDestroy(assoc.frame);
#ifdef VICEEXT_NEW_VEHICLE_LOADER // Custom wheels
			else if(m_wheelId != 249){
#else
			else{
#endif
				RwV3d scale;
				atomic = (RpAtomic*)CModelInfo::GetModelInfo(m_wheelId)->CreateInstance();
				RwFrameDestroy(RpAtomicGetFrame(atomic));
				RpAtomicSetFrame(atomic, assoc.frame);
				RpClumpAddAtomic(m_clump, atomic);
				CVisibilityPlugins::SetAtomicRenderCallback(atomic,
					CVisibilityPlugins::RenderWheelAtomicCB);
				scale.x = m_wheelScale;
				scale.y = m_wheelScale;
				scale.z = m_wheelScale;
				RwFrameScale(assoc.frame, &scale, rwCOMBINEPRECONCAT);
			}
		}
	}

	for(i = 0; i < odNumDetach; i++)
		RwFrameRemoveChild(odDetach[i]);

// El asiento sale del dummy del DFF, como en el mod. El override a 0,12f del
// 19/10 se aplicaba tambien a la 6507 y con el grupo DIRT dejaba al jinete
// hundido bajo el sillon; el DFF policial trae su propia altura.
#ifdef __EMSCRIPTEN__
	if(!CGeneral::faststricmp(GetModelName(), "polwintergreen") ||
	   !CGeneral::faststricmp(GetModelName(), "wintergreen")){
		static uint8 s_odBikeSeatTrace = 0;
		uint8 odBit = !CGeneral::faststricmp(GetModelName(), "polwintergreen") ? 1 : 2;
		if(!(s_odBikeSeatTrace & odBit)){
			s_odBikeSeatTrace |= odBit;
			char odT[256];
			snprintf(odT, sizeof odT,
				"VICEEXT bike model=%s anim=%d seat=%.2f,%.2f,%.2f back=%.2f,%.2f,%.2f",
				GetModelName(), (int)m_animFileIndex,
				m_positions[CAR_POS_FRONTSEAT].x, m_positions[CAR_POS_FRONTSEAT].y, m_positions[CAR_POS_FRONTSEAT].z,
				m_positions[CAR_POS_BACKSEAT].x, m_positions[CAR_POS_BACKSEAT].y, m_positions[CAR_POS_BACKSEAT].z);
			ODTRACES(odT);
		}
	}
#endif
}

void
CVehicleModelInfo::SetVehicleComponentFlags(RwFrame *frame, uint32 flags)
{
	tHandlingData *handling;

	handling = mod_HandlingManager.GetHandlingData((tVehicleType)m_handlingId);

#define SETFLAGS(f) RwFrameForAllObjects(frame, SetAtomicFlagCB, (void*)(f))

	if(flags & VEHICLE_FLAG_WINDSCREEN){
		if(this == CModelInfo::GetModelInfo(MI_RHINO))
			return;
		SETFLAGS(ATOMIC_FLAG_WINDSCREEN);
	}

	if(flags & VEHICLE_FLAG_ANGLECULL)
		SETFLAGS(ATOMIC_FLAG_ANGLECULL);

	if(flags & VEHICLE_FLAG_FRONT)
		SETFLAGS(ATOMIC_FLAG_FRONT);
	else if(flags & VEHICLE_FLAG_REAR && (handling->Flags & HANDLING_IS_VAN || (flags & (VEHICLE_FLAG_LEFT|VEHICLE_FLAG_RIGHT)) == 0))
		SETFLAGS(ATOMIC_FLAG_REAR);
	else if(flags & VEHICLE_FLAG_LEFT)
		SETFLAGS(ATOMIC_FLAG_LEFT);
	else if(flags & VEHICLE_FLAG_RIGHT)
		SETFLAGS(ATOMIC_FLAG_RIGHT);

	if(flags & VEHICLE_FLAG_REARDOOR)
		SETFLAGS(ATOMIC_FLAG_REARDOOR);
	else if(flags & VEHICLE_FLAG_FRONTDOOR)
		SETFLAGS(ATOMIC_FLAG_FRONTDOOR);

	if(flags & VEHICLE_FLAG_DRAWLAST)
		SETFLAGS(ATOMIC_FLAG_DRAWLAST);
}

#define COMPRULE_RULE(comprule) (((comprule) >> 12) & 0xF)
#define COMPRULE_COMPS(comprule) ((comprule) & 0xFFF)
#define COMPRULE_COMPN(comps, n) (((comps) >> 4*(n)) & 0xF)
#define COMPRULE2_RULE(comprule) (((comprule) >> (12+16)) & 0xF)
#define COMPRULE2_COMPS(comprule) ((comprule >> 16) & 0xFFF)
#define COMPRULE2_COMPN(comps, n) (((comps >> 16) >> 4*(n)) & 0xF)

bool
IsValidCompRule(int rule)
{
	if(rule == 2)
		return CWeather::OldWeatherType == WEATHER_RAINY ||
		       CWeather::NewWeatherType == WEATHER_RAINY;
	return true;
}

int32
CountCompsInRule(int comps)
{
	int32 n;
	for(n = 0; comps != 0; comps >>= 4)
		if((comps & 0xF) != 0xF)
			n++;
	return n;
}

int32
ChooseComponent(int32 rule, int32 comps)
{
	int32 n;
	switch(rule){
	// identical cases....
	case 1:
		n = CGeneral::GetRandomNumberInRange(0, CountCompsInRule(comps));
		return COMPRULE_COMPN(comps, n);
	case 2:
		// only valid in rain
		n = CGeneral::GetRandomNumberInRange(0, CountCompsInRule(comps));
		return COMPRULE_COMPN(comps, n);
	case 3:
		n = CGeneral::GetRandomNumberInRange(0, 1+CountCompsInRule(comps));
		if(n != 0)
			return COMPRULE_COMPN(comps, n-1);
		return -1;
	case 4:
#ifdef FIX_BUGS
		return CGeneral::GetRandomNumberInRange(0, 6);
#else
		return CGeneral::GetRandomNumberInRange(0, 5);
#endif
	}
	return -1;
}

int32
GetListOfComponentsNotUsedByRules(uint32 comprules, int32 numComps, int32 *comps)
{
	int32 i, n;
	int32 unused[6] = { 0, 1, 2, 3, 4, 5 };

	// first comprule
	if(COMPRULE_RULE(comprules) && IsValidCompRule(COMPRULE_RULE(comprules)))
		for(i = 0; i < 3; i++){
			n = COMPRULE_COMPN(comprules, i);
			if(n != 0xF)
				unused[n] = 0xF;
		}
	// second comprule
	comprules >>= 16;
	if(COMPRULE_RULE(comprules) && IsValidCompRule(COMPRULE_RULE(comprules)))
		for(i = 0; i < 3; i++){
			n = COMPRULE_COMPN(comprules, i);
			if(n != 0xF)
				unused[n] = 0xF;
		}

	n = 0;
	for(i = 0; i < numComps; i++)
		if(unused[i] != 0xF)
			comps[n++] = unused[i];
	return n;
}

int32 wheelIds[] = { CAR_WHEEL_LF, CAR_WHEEL_LB, CAR_WHEEL_RF, CAR_WHEEL_RB };

void
CVehicleModelInfo::GetWheelPosn(int32 n, CVector &pos)
{
	RwMatrix *m = RwFrameGetMatrix(GetFrameFromId(m_clump, wheelIds[n]));
	pos.x = RwMatrixGetPos(m)->x;
	pos.y = RwMatrixGetPos(m)->y;
	pos.z = RwMatrixGetPos(m)->z;
}


int32
CVehicleModelInfo::ChooseComponent(void)
{
	int32 comp;
	int32 comps[8];
	int32 n;

	comp = -1;
	if(ms_compsToUse[0] == -2){
		if(COMPRULE_RULE(m_compRules) && IsValidCompRule(COMPRULE_RULE(m_compRules)))
			comp = ::ChooseComponent(COMPRULE_RULE(m_compRules), COMPRULE_COMPS(m_compRules));
		else if(CGeneral::GetRandomNumberInRange(0, 3) < 2){
			n = GetListOfComponentsNotUsedByRules(m_compRules, m_numComps, comps);
			if(n)
				comp = comps[(int)CGeneral::GetRandomNumberInRange(0, n)];
		}
	}else{
		comp = ms_compsToUse[0];
		ms_compsToUse[0] = -2;
	}
	return comp;
}

int32
CVehicleModelInfo::ChooseSecondComponent(void)
{
	int32 comp;
	int32 comps[8];
	int32 n;

	comp = -1;
	if(ms_compsToUse[1] == -2){
		if(COMPRULE2_RULE(m_compRules) && IsValidCompRule(COMPRULE2_RULE(m_compRules)))
			comp = ::ChooseComponent(COMPRULE2_RULE(m_compRules), COMPRULE2_COMPS(m_compRules));
		else if(COMPRULE_RULE(m_compRules) && IsValidCompRule(COMPRULE_RULE(m_compRules)) &&
		        CGeneral::GetRandomNumberInRange(0, 3) < 2){

			n = GetListOfComponentsNotUsedByRules(m_compRules, m_numComps, comps);
			if(n)
				comp = comps[(int)CGeneral::GetRandomNumberInRange(0, n)];
		}
	}else{
		comp = ms_compsToUse[1];
		ms_compsToUse[1] = -2;
	}
	return comp;
}

struct editableMatCBData
{
	CVehicleModelInfo *vehicle;
	int32 numMats1;
	int32 numMats2;
};

RpMaterial*
CVehicleModelInfo::GetEditableMaterialListCB(RpMaterial *material, void *data)
{
	RwRGBA white = { 255, 255, 255, 255 };
	const RwRGBA *col;
	editableMatCBData *cbdata;

	cbdata = (editableMatCBData*)data;
	col = RpMaterialGetColor(material);
	if(col->red == 0x3C && col->green == 0xFF && col->blue == 0){
		if(cbdata->numMats1 < NUM_FIRST_MATERIALS){
			cbdata->vehicle->m_materials1[cbdata->numMats1] = material;
			RpMaterialSetColor(material, &white);
		}
		cbdata->numMats1++;
	}else if(col->red == 0xFF && col->green == 0 && col->blue == 0xAF){
		if(cbdata->numMats2 < NUM_SECOND_MATERIALS){
			cbdata->vehicle->m_materials2[cbdata->numMats2] = material;
			RpMaterialSetColor(material, &white);
		}
		cbdata->numMats2++;
	}
	return material;
}

RpAtomic*
CVehicleModelInfo::GetEditableMaterialListCB(RpAtomic *atomic, void *data)
{
	RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), GetEditableMaterialListCB, data);
	return atomic;
}

static int maxFirstMaterials;
static int maxSecondMaterials;

void
CVehicleModelInfo::FindEditableMaterialList(void)
{
	editableMatCBData cbdata;
	int32 i;

	cbdata.vehicle = this;
	cbdata.numMats1 = 0;
	cbdata.numMats2 = 0;
	RpClumpForAllAtomics(m_clump, GetEditableMaterialListCB, &cbdata);
	for(i = 0; i < m_numComps; i++)
		GetEditableMaterialListCB(m_comps[i], &cbdata);
	m_materials1[Min(cbdata.numMats1, NUM_FIRST_MATERIALS - 1)] = nil;
	m_materials2[Min(cbdata.numMats2, NUM_SECOND_MATERIALS - 1)] = nil;
	maxFirstMaterials = Max(maxFirstMaterials, cbdata.numMats1);
	maxSecondMaterials = Max(maxSecondMaterials, cbdata.numMats2);
	m_currentColour1 = -1;
	m_currentColour2 = -1;
}

void
CVehicleModelInfo::SetVehicleColour(uint8 c1, uint8 c2)
{
	RwRGBA col, *colp;
	RpMaterial **matp;

	if(c1 != m_currentColour1){
		col = ms_vehicleColourTable[c1];
		for(matp = m_materials1; *matp; matp++){
			colp = (RwRGBA*)RpMaterialGetColor(*matp);	// get rid of const
			colp->red = col.red;
			colp->green = col.green;
			colp->blue = col.blue;
		}
		m_currentColour1 = c1;
	}

	if(c2 != m_currentColour2){
		col = ms_vehicleColourTable[c2];
		for(matp = m_materials2; *matp; matp++){
			colp = (RwRGBA*)RpMaterialGetColor(*matp);	// get rid of const
			colp->red = col.red;
			colp->green = col.green;
			colp->blue = col.blue;
		}
		m_currentColour2 = c2;
	}
}

void
CVehicleModelInfo::ChooseVehicleColour(uint8 &col1, uint8 &col2)
{
	if(m_numColours == 0 || gbBlackCars){
		col1 = 0;
		col2 = 0;
	}else if(gbPinkCars){
		col1 = 68;
		col2 = 68;
	}else{
		m_lastColorVariation = (m_lastColorVariation+1) % m_numColours;
		col1 = m_colours1[m_lastColorVariation];
		col2 = m_colours2[m_lastColorVariation];
		if(m_numColours > 1){
			CVehicle *veh = FindPlayerVehicle();
			if(veh && CModelInfo::GetModelInfo(veh->GetModelIndex()) == this &&
			   veh->m_currentColour1 == col1 &&
			   veh->m_currentColour2 == col2){
				m_lastColorVariation = (m_lastColorVariation+1) % m_numColours;
				col1 = m_colours1[m_lastColorVariation];
				col2 = m_colours2[m_lastColorVariation];
			}
		}
	}
}

void
CVehicleModelInfo::AvoidSameVehicleColour(uint8 *col1, uint8 *col2)
{
	int i, n;

	if(gbBlackCars){
		*col1 = 0;
		*col2 = 0;
	}else if(gbPinkCars){
		*col1 = 68;
		*col2 = 68;
	}else{
		if(m_numColours > 1)
			for(i = 0; i < 8; i++){
				if(*col1 != m_lastColour1 || *col2 != m_lastColour2)
					break;
				n = CGeneral::GetRandomNumberInRange(0, m_numColours);
				*col1 = m_colours1[n];
				*col2 = m_colours2[n];
			}
		m_lastColour1 = *col1;
		m_lastColour2 = *col2;
	}
}

// unused
RwTexture*
CreateCarColourTexture(uint8 r, uint8 g, uint8 b)
{
	RwImage *img;
	RwRaster *ras;
	RwTexture *tex;
	RwUInt8 *pixels;
	RwInt32 width, height, depth, format;

	img = RwImageCreate(2, 2, 32);
	pixels = (RwUInt8*)RwMalloc(2*2*4);
	pixels[0] = r;
	pixels[1] = g;
	pixels[2] = b;
	pixels[3] = 0xFF;
	pixels[4] = r;
	pixels[5] = g;
	pixels[6] = b;
	pixels[7] = 0xFF;
	pixels[8] = r;
	pixels[9] = g;
	pixels[10] = b;
	pixels[11] = 0xFF;
	pixels[12] = r;
	pixels[13] = g;
	pixels[14] = b;
	pixels[15] = 0xFF;
	RwImageSetPixels(img, pixels);
	RwImageSetStride(img, 8);
	RwImageSetPalette(img, nil);
	RwImageFindRasterFormat(img, rwRASTERTYPETEXTURE, &width, &height, &depth, &format);
	ras = RwRasterCreate(width, height, depth, format);
	RwRasterSetFromImage(ras, img);
	RwImageDestroy(img);
	RwFree(pixels);
	tex = RwTextureCreate(ras);
	RwTextureGetName(tex)[0] = '@';
	return tex;
}

void
CVehicleModelInfo::LoadVehicleColours(void)
{
	int fd;
	int i;
	char line[1024];
	int start, end;
	int section, numCols;
	enum {
		NONE,
		COLOURS,
		CARS
	};
	int r, g, b;
	char name[64];
	int colors[16];
	int n;

	CFileMgr::ChangeDir("\\DATA\\");
	fd = CFileMgr::OpenFile("CARCOLS.DAT", "r");
	CFileMgr::ChangeDir("\\");

	for(i = 0; i < 256; i++)
		ms_colourTextureTable[i] = nil;

	section = 0;
	numCols = 0;
	while(CFileMgr::ReadLine(fd, line, sizeof(line))){
		// find first valid character in line
		for(start = 0; ; start++)
			if(line[start] > ' ' || line[start] == '\0' || line[start] == '\n')
				break;
		// find end of line
		for(end = start; ; end++){
			if(line[end] == '\0' || line[end] == '\n')
				break;
			if(line[end] == ',' || line[end] == '\r')
				line[end] = ' ';
		}
		line[end] = '\0';

		// empty line
		if(line[start] == '#' || line[start] == '\0')
			continue;

		if(section == NONE){
			if(line[start] == 'c' && line[start + 1] == 'o' && line[start + 2] == 'l')
				section = COLOURS;
			if(line[start] == 'c' && line[start + 1] == 'a' && line[start + 2] == 'r')
				section = CARS;
		}else if(line[start] == 'e' && line[start + 1] == 'n' && line[start + 2] == 'd'){
			section = NONE;
		}else if(section == COLOURS){
			sscanf(&line[start],	// BUG: games doesn't add start
				"%d %d %d", &r, &g, &b);
			ms_vehicleColourTable[numCols].red = r;
			ms_vehicleColourTable[numCols].green = g;
			ms_vehicleColourTable[numCols].blue = b;
			ms_vehicleColourTable[numCols].alpha = 0xFF;
			numCols++;
		}else if(section == CARS){
			n = sscanf(&line[start],	// BUG: games doesn't add start
				"%s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
				name,
				&colors[0], &colors[1],
				&colors[2], &colors[3],
				&colors[4], &colors[5],
				&colors[6], &colors[7],
				&colors[8], &colors[9],
				&colors[10], &colors[11],
				&colors[12], &colors[13],
				&colors[14], &colors[15]);
			CVehicleModelInfo *mi = (CVehicleModelInfo*)CModelInfo::GetModelInfo(name, nil);
			assert(mi);
			mi->m_numColours = (n-1)/2;
			for(i = 0; i < mi->m_numColours; i++){
				mi->m_colours1[i] = colors[i*2 + 0];
				mi->m_colours2[i] = colors[i*2 + 1];
			}
		}
	}

	CFileMgr::CloseFile(fd);
}

void
CVehicleModelInfo::DeleteVehicleColourTextures(void)
{
	int i;

	for(i = 0; i < 256; i++){
		if(ms_colourTextureTable[i]){
			RwTextureDestroy(ms_colourTextureTable[i]);
			ms_colourTextureTable[i] = nil;
		}
	}
}

RpMaterial*
CVehicleModelInfo::GetMatFXEffectMaterialCB(RpMaterial *material, void *data)
{
	if(RpMatFXMaterialGetEffects(material) == rpMATFXEFFECTNULL)
		return material;
	*(int*)data = RpMatFXMaterialGetEffects(material);
	return nil;
}

RpMaterial*
CVehicleModelInfo::AddSpecularEnvironmentMapCB(RpMaterial *material, void *data)
{
	if(RpMatFXMaterialGetEffects(material) != rpMATFXEFFECTNULL)
		return material;
	if(RpMaterialGetTexture(material) == nil || material->surfaceProps.specular < 0.25f)
		return material;
	if(CustomPipes::EnvMapTex == nil)
		return material;

	RpMatFXMaterialSetEffects(material, rpMATFXEFFECTENVMAP);
	RpMatFXMaterialSetupEnvMap(material, (RwTexture*)CustomPipes::EnvMapTex,
		pMatFxIdentityFrame, false, material->surfaceProps.specular);
	*(int*)data = rpMATFXEFFECTENVMAP;
	return material;
}

RpMaterial*
CVehicleModelInfo::SetDefaultEnvironmentMapCB(RpMaterial *material, void *data)
{
	if(RpMatFXMaterialGetEffects(material) == rpMATFXEFFECTENVMAP){
		RpMatFXMaterialSetEnvMapFrame(material, pMatFxIdentityFrame);
		if(RpMaterialGetTexture(material) == nil)
			RpMaterialSetTexture(material, gpWhiteTexture);
		RpMatFXMaterialSetEffects(material, rpMATFXEFFECTENVMAP);
#ifndef PS2_MATFX
		float coef = RpMatFXMaterialGetEnvMapCoefficient(material);
#ifdef VICEEXT_ENV_MAP_COEF
#else
		coef *= 0.25f;	// Tone down a bit for PC
#endif
		RpMatFXMaterialSetEnvMapCoefficient(material, coef);
#endif
	}
	return material;
}

RpAtomic*
CVehicleModelInfo::SetEnvironmentMapCB(RpAtomic *atomic, void *data)
{
	int fx;
	RpGeometry *geo;

	geo = RpAtomicGetGeometry(atomic);
	fx = rpMATFXEFFECTNULL;
	RpGeometryForAllMaterials(geo, GetMatFXEffectMaterialCB, &fx);
	if(fx == rpMATFXEFFECTNULL)
		RpGeometryForAllMaterials(geo, AddSpecularEnvironmentMapCB, &fx);
	if(fx != rpMATFXEFFECTNULL){
		RpMatFXAtomicEnableEffects(atomic);
		RpGeometryForAllMaterials(geo, SetDefaultEnvironmentMapCB, data);
	}
	return atomic;
}

void
CVehicleModelInfo::SetEnvironmentMap(void)
{
	CSimpleModelInfo *wheelmi;
	int32 i;

	if(pMatFxIdentityFrame == nil){
		RwV3d axis = { 1.0f, 0.0f, 0.0f };
		pMatFxIdentityFrame = RwFrameCreate();
		RwMatrixRotate(RwFrameGetMatrix(pMatFxIdentityFrame), &axis, 60.0f, rwCOMBINEREPLACE);
		RwFrameUpdateObjects(pMatFxIdentityFrame);
		RwFrameGetLTM(pMatFxIdentityFrame);
	}

	RpClumpForAllAtomics(m_clump, SetEnvironmentMapCB, nil);
#ifdef VICEEXT_NEW_VEHICLE_LOADER // Custom wheels
	if(m_wheelId != -1 && m_wheelId != 249){
#else
	if(m_wheelId != -1){
#endif
		wheelmi = (CSimpleModelInfo*)CModelInfo::GetModelInfo(m_wheelId);
		for(i = 0; i < wheelmi->m_numAtomics; i++)
			SetEnvironmentMapCB(wheelmi->m_atomics[i], nil);
	}
#ifdef VICEEXT_FIX_SILENTPATCH
	static const char *const odCompExceptions[] = { "stallion", "mesa" };
	const int odNumCompExc = (int)(sizeof(odCompExceptions) / sizeof(odCompExceptions[0]));
	bool odCompExcluded = false;
	for(i = 0; i < odNumCompExc; i++){
		if(!CGeneral::faststricmp(odCompExceptions[i], GetModelName())) {
			odCompExcluded = true;
			break;
		}
	}
#ifdef VICEEXT_SP_ENV_MAP_EXTRAS
	if(!odCompExcluded)
#else
	if(false && !odCompExcluded)
#endif
	for(i = 0; i < m_numComps; i++){
		if(m_comps[i])
			SetEnvironmentMapCB(m_comps[i], nil);
	}
#endif

#ifdef EXTENDED_PIPELINES
	CustomPipes::AttachVehiclePipe(m_clump);
#ifdef VICEEXT_FIX_SILENTPATCH
#ifdef VICEEXT_SP_ENV_MAP_EXTRAS
	if(!odCompExcluded)
#else
	if(false && !odCompExcluded)
#endif
	for(i = 0; i < m_numComps; i++){
		if(m_comps[i])
			CustomPipes::AttachVehiclePipe(m_comps[i]);
	}
#endif
#endif
}

void
CVehicleModelInfo::LoadEnvironmentMaps(void)
{
	int32 txdslot;

	txdslot = CTxdStore::FindTxdSlot("particle");
	CTxdStore::PushCurrentTxd();
	CTxdStore::SetCurrentTxd(txdslot);
	if(gpWhiteTexture == nil){
		gpWhiteTexture = RwTextureRead("white", nil);
		RwTextureGetName(gpWhiteTexture)[0] = '@';
		RwTextureSetFilterMode(gpWhiteTexture, rwFILTERLINEAR);
	}
	CTxdStore::PopCurrentTxd();
}

void
CVehicleModelInfo::ShutdownEnvironmentMaps(void)
{
	RwTextureDestroy(gpWhiteTexture);
	gpWhiteTexture = nil;
	RwFrameDestroy(pMatFxIdentityFrame);
	pMatFxIdentityFrame = nil;
}

int
CVehicleModelInfo::GetMaximumNumberOfPassengersFromNumberOfDoors(int id)
{
	int n;

	switch(id){
	case MI_TRAIN:
		n = 3;
		break;
	case MI_FIRETRUCK:
		n = 2;
		break;
	case MI_HUNTER:
		n = 1;
		break;
	default:
		n = ((CVehicleModelInfo*)CModelInfo::GetModelInfo(id))->m_numDoors;
	}

	if(n == 0)
		return id == MI_RCBANDIT || id == MI_PIZZABOY || id == MI_BAGGAGE ? 0 : 1;

	if(id == MI_COACH)
		return 8;

	return n - 1;
}
