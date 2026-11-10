#pragma once

#include "Vehicle.h"
#include "DamageManager.h"
#include "Door.h"
#include "Skidmarks.h"

class CObject;

#ifdef VICEEXT_IMPROVED_VEHICLES_2
// Compartido por coches y motos: cambia el material del intermitente. `node` y
// `side` son la clave con la que la herramienta midio la lente (eCarNodes, tambien
// en motos), de la que salen los materiales y el color que se pintan.
void ViceExtSetIndicatorMaterial(CVehicle *veh, int node, int side, RwFrame *frame, bool on);
// nil = el modelo no trae la lente genérica del mod, no se le cambia la textura.
class CVehicleModelInfo;
RwTexture *ViceExtLampTexture(CVehicleModelInfo *mi, bool on);
// Ranura del barrido de destellos: un nodo de intermitente de un vehiculo, con la
// clave con la que lo midio la herramienta (eCarNodes, tambien para las motos),
// su costado y la lampara a la que pertenece.
struct ViceExtIndSlot {
	int8 frameIdx;
	int8 tableNode;
	int8 side;
	int8 light;
};
// Destello de las lentes que la medida declara ambar, anclado en su geometria.
// Un nodo cuya lente es el propio faro o el piloto no recibe corona nueva: ese
// parpadea con su lampara (ruta Banshee). `damage` nil = sin dano por lampara.
// En motos el ancla vive en el espacio del NODO de la lente y se transforma por
// su LTM viva (plan 51.1 E2): la inclinacion de la moto se aplica al frame del
// chasis, no a la matriz de la entidad, y con esta matriz el destello se quedaba
// en la posicion de la lente sin tumbar. Los coches siguen con la entidad.
void ViceExtIndicatorCoronas(CVehicle *veh, CDamageManager *damage,
                             const ViceExtIndSlot *slots, int n, RwFrame **frames,
                             uint32 idFirst, bool leftState, bool rightState);
// Regla del trafico (plan 51.0 D3/D4), compartida por coches y motos para que
// haya una sola IA. Lado a senalar: -1 izquierda, 0 nada, +1 derecha. `hazard` =
// el NPC va a pararse a bajarse (luces de parqueo). `motivo` y `angle` alimentan
// la traza P4 kind=turners. Parametros nil: admitidos.
int ViceExtNpcTurnSignalSide(CVehicle *veh, bool *hazard, const char **motivo, float *angle);
// El NPC que aparco con las luces de parqueo puestas las apaga al volver a andar
// (plan 51.1 E1). `hadHazard` es la marca del vehiculo; devuelve true justo en el
// frame del apagado, para que la traza lo nombre.
bool ViceExtNpcHazardTurnsOff(bool hazardNow, bool *hadHazard, float speed);
#endif

enum {
	CARWHEEL_FRONT_LEFT,
	CARWHEEL_REAR_LEFT,
	CARWHEEL_FRONT_RIGHT,
	CARWHEEL_REAR_RIGHT
};


class CAutomobile : public CVehicle
{
public:
	CDamageManager Damage;
	CDoor Doors[6];
	RwFrame *m_aCarNodes[NUM_CAR_NODES];
	CColPoint m_aWheelColPoints[4];
	float m_aSuspensionSpringRatio[4];
	float m_aSuspensionSpringRatioPrev[4];
	float m_aWheelTimer[4];		// set to 4.0 when wheel is touching ground, then decremented
	float m_auto_unused1;
	eSkidmarkType m_aWheelSkidmarkType[4];
	bool m_aWheelSkidmarkBloody[4];
	bool m_aWheelSkidmarkUnk[4];
	float m_aWheelRotation[4];
	float m_aWheelPosition[4];
	float m_aWheelSpeed[4];
	uint8 m_auto_unused2;
#if (defined GTA_PS2 && !defined FIX_BUGS)
	uint8 m_bombType : 3;
#endif
	uint8 bTaxiLight : 1;
	uint8 bFixedColour : 1;
	uint8 bBigWheels : 1;
	uint8 bWaterTight : 1;	// no damage for non-player peds
	uint8 bNotDamagedUpsideDown : 1;
	uint8 bMoreResistantToDamage : 1;
	uint8 bTankDetonateCars : 1;
	uint8 bStuckInSand : 1;
	uint8 bHeliDestroyed : 1;
#if (defined GTA_PS2 && !defined FIX_BUGS)
	CEntity* m_pBombRigger;
#endif
	int16 m_doingBurnout;
	uint16 m_hydraulicState;
	uint32 m_nBusDoorTimerEnd;
	uint32 m_nBusDoorTimerStart;
	float m_aSuspensionSpringLength[4];
	float m_aSuspensionLineLength[4];
	float m_fHeightAboveRoad;
	float m_fTraction;
	float m_fTireTemperature;
	float m_fOrientation;	// for heli and plane go-to
	float m_fPlaneSteer;	// related to the above
	float m_fVelocityChangeForAudio;
	float m_randomValues[6];	// used for what?
	float m_fFireBlowUpTimer;
	CPhysical *m_aGroundPhysical[4];	// physicals touching wheels
	CVector m_aGroundOffset[4];		// from ground object to colpoint
	CEntity *m_pSetOnFireEntity;
	float m_weaponDoorTimerLeft;	// still don't know what exactly this is
	float m_weaponDoorTimerRight;
	float m_fCarGunLR;
	float m_fCarGunUD;
	float m_fHeliOrientation;
	float m_fPropellerRotation;
	uint8 stuff4[4];
	uint8 m_nWheelsOnGround;
	uint8 m_nDriveWheelsOnGround;
	uint8 m_nDriveWheelsOnGroundPrev;
	float m_fGasPedalAudio;
	tWheelState m_aWheelState[4];
#ifdef VICEEXT_FIX_FV
	float m_fOdPhase;
#endif

	static bool m_sAllTaxiLights;

	CAutomobile(int32 id, uint8 CreatedBy);

	// from CEntity
	void SetModelIndex(uint32 id);
	void ProcessControl(void);
	void Teleport(CVector v);
	void PreRender(void);
	void Render(void);

	// from CPhysical
	int32 ProcessEntityCollision(CEntity *ent, CColPoint *colpoints);

	// from CVehicle
	void ProcessControlInputs(uint8);
	void GetComponentWorldPosition(int32 component, CVector &pos);
	bool IsComponentPresent(int32 component);
	void SetComponentRotation(int32 component, CVector rotation);
	void OpenDoor(int32 component, eDoors door, float openRatio);
	void ProcessOpenDoor(uint32, uint32, float);
	bool IsDoorReady(eDoors door);
	bool IsDoorFullyOpen(eDoors door);
	bool IsDoorClosed(eDoors door);
	bool IsDoorMissing(eDoors door);
	bool IsDoorReady(uint32 door);
	bool IsDoorMissing(uint32 door);
	bool IsOpenTopCar(void);
	void RemoveRefsToVehicle(CEntity *ent);
	void BlowUpCar(CEntity *ent);
	bool SetUpWheelColModel(CColModel *colModel);
	void BurstTyre(uint8 tyre, bool applyForces);
	bool IsRoomForPedToLeaveCar(uint32 component, CVector *doorOffset);
	float GetHeightAboveRoad(void);
	void PlayCarHorn(void);

	void FireTruckControl(void);
	void TankControl(void);
	void HydraulicControl(void);
	void VehicleDamage(float impulse, uint16 damagedPiece);
	void ProcessBuoyancy(void);
	void DoDriveByShootings(void);
	void DoHoverSuspensionRatios(void);
	int32 RcbanditCheckHitWheels(void);
	int32 RcbanditCheck1CarWheels(CPtrList &list);
	void PlaceOnRoadProperly(void);
	void dmgDrawCarCollidingParticles(const CVector &pos, float amount);
	void AddDamagedVehicleParticles(void);
	int32 AddWheelDirtAndWater(CColPoint *colpoint, uint32 belowEffectSpeed);
	void PlayHornIfNecessary(void);
	void ResetSuspension(void);
	void SetupSuspensionLines(void);
	void ScanForCrimes(void);
	void BlowUpCarsInPath(void);
#ifdef VICEEXT_IMPROVED_VEHICLES_2
	void ViceExtProcessTurnSignals(void);
	void ViceExtApplyIndicatorMaterials(void);
	bool m_bIndicatorState[2];
	// Plan 51.0 D4 / 51.1 E1: `Latched` = el estado lo puso el jugador con la tecla,
	// y por eso sobrevive a bajarse. `NpcHazard` = el NPC iba en luces de parqueo
	// (se paro a bajarse); con esa marca el coche apaga al volver a andar por
	// decision del trafico, no por velocidad.
	bool m_bIndicatorLatched;
	bool m_bIndicatorNpcHazard;
#endif
#ifdef VICEEXT_BODY_LENS
	// Plan 58.0 T3: lentes ambar de carroceria resueltas por nombre de frame en
	// SetupModelNodes (no estan en m_aCarNodes). Cada entrada apunta a su frame
	// y a su fila de BodyLensTable.h; m_nBodyLens = filas de este modelo.
	void ViceExtSetupBodyLens(void);
	RwFrame *m_aBodyLensFrame[VICEEXT_BODY_LENS_MAX];
	int16 m_aBodyLensRow[VICEEXT_BODY_LENS_MAX];
	// 59.0: 1 si TODO el material de la fila es cara de lente (medido en el
	// atomic). Solo entonces admite el tinte de la ruta del nodo: un material
	// compartido con el panel se pintaria tambien la pintura del coche.
	uint8 m_aBodyLensTint[VICEEXT_BODY_LENS_MAX];
	uint8 m_nBodyLens;
#endif
#ifdef VICEEXT_IMPROVED_VEHICLES_2
	void ViceExtRotateSteeringWheel(void);
#ifdef VICEEXT_IMPROVED_VEHICLES
	void SetWheelMissing(int32 wheel);	// 6.11: la rueda se cae y vuela
#endif
#endif
#ifdef VICEEXT_CHEAT_EFFECTS
	void DoHoverAboveSurface(void);	// 11.03 AIRWAYS: vuelo del coche con el mando
#endif
	bool HasCarStoppedBecauseOfLight(void);
	void SetBusDoorTimer(uint32 timer, uint8 type);
	void ProcessAutoBusDoors(void);
	void ProcessSwingingDoor(int32 component, eDoors door);
	void SetupDamageAfterLoad(void);
	CObject *SpawnFlyingComponent(int32 component, uint32 type);
	CObject *RemoveBonnetInPedCollision(void);
	void SetPanelDamage(int32 component, ePanels panel, bool noFlyingComponents = false);
	void SetBumperDamage(int32 component, ePanels panel, bool noFlyingComponents = false);
	void SetDoorDamage(int32 component, eDoors door, bool noFlyingComponents = false);

	void TellHeliToGoToCoors(float x, float y, float z, uint8 speed);
	void TellPlaneToGoToCoors(float x, float y, float z, uint8 speed);
	void SetHeliOrientation(float orient) { m_fHeliOrientation = orient; }
	void ClearHeliOrientation(void) { m_fHeliOrientation = -1.0f; }

	void Fix(void);
	void SetComponentVisibility(RwFrame *frame, uint32 flags);
	void SetupModelNodes(void);
	void SetTaxiLight(bool light);
	bool GetAllWheelsOffGround(void);
	void HideAllComps(void);
	void ShowAllComps(void);
	void ReduceHornCounter(void);

	void PopBoot(void);
	void PopBootUsingPhysics(void);
	void CloseAllDoors(void);
	CPed *KnockPedOutCar(eWeaponType weapon, uint16 door, CPed *ped);

#ifdef COMPATIBLE_SAVES
	virtual void Save(uint8*& buf);
	virtual void Load(uint8*& buf);
#endif
	static const uint32 nSaveStructSize;

	static void SetAllTaxiLights(bool set);
};

extern CVector vecHunterGunPos;
extern bool bAllCarCheat;