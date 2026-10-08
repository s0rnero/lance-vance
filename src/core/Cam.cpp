#include "common.h"

#include "main.h"
#include "Draw.h"
#include "World.h"
#include "Vehicle.h"
#include "Automobile.h"
#include "Boat.h"
#include "Bones.h"
#include "Ped.h"
#include "PedArbiter.h"
#include "PlayerPed.h"
#include "CopPed.h"
#include "RpAnimBlend.h"
#include "ControllerConfig.h"
#include "Pad.h"
#include "Frontend.h"
#include "General.h"
#include "Timecycle.h"
#include "Renderer.h"
#include "Shadows.h"
#include "Hud.h"
#include "ZoneCull.h"
#include "SurfaceTable.h"
#include "WaterLevel.h"
#include "ondemand.h"

// BETASNAP (plan camara-coche-sin-lucha): nombra la via que pincha/snappea
// la Beta del coche. Una linea por motivo y sesion (no por frame), con
// reancla del reloj del motor (retrocede al cargar partida).
#ifdef __EMSCRIPTEN__
static void
s_odBetaSnap(const char *motivo)
{
	static const char *s_hechos[8] = { 0 };
	for(int i = 0; i < 8; i++){
		if(s_hechos[i] == motivo)
			return;
		if(s_hechos[i] == 0){
			s_hechos[i] = motivo;
			char t[96];
			snprintf(t, sizeof t, "BETASNAP motivo=%s", motivo);
			ODTRACES(t);
			return;
		}
	}
}
#else
static void s_odBetaSnap(const char *) {}
#endif

#include "MBlur.h"
#include "SceneEdit.h"
#include "Debug.h"
#include "Camera.h"
#include "DMAudio.h"
#include "Bike.h"
#include "ondemand.h" // Sección 3, C1b-2: ODTRACES del autocentrado de cámara
#include "Pickups.h"

// Estado de las colisiones de la ley de camara, a ambito de FICHERO (no de
// funcion): lo escriben tanto la ley de coche como `Process_AvoidCollisions`, que
// es un miembro de CCam (B3c del plan apuntado-classicaxis-100). Antes vivian
// dentro de Process_Cam_On_A_String y el bloque se extrajo tal cual, asi que
// hubo que sacarlos aqui para que las dos leyes los vieran. Los leen las trazas
// CAMB2b/CAMB3b.
static int s_odLosHit = 0;
static float s_odLosD = 0.0f;
static int s_odLosPed = 0;
static int s_odSphHit = 0;      // la esfera choco con algo
static int s_odSphModel = -1;   // modelo del impacto
static int s_odSphPed = 0;      // era un ped
static int s_odSphOwn = 0;      // era el propio vehiculo objetivo
static int s_odSphApp = 0;      // veces que se aplico el acercamiento (0..5)
static float s_odSphD = 0.0f;   // d crudo (perpendicular / viewPlaneWidth)
static float s_odSphNear = 0.0f;
// ClassicAXIS CamNew.cpp:412-414: los 5 peds que la ley de apuntado escondio el
// frame anterior (<0,5 m del centro de la esfera y visibles), para que no tapen
// la mira. Se devuelven al principio de la llamada siguiente.
static CEntity *s_odHidePeds[5] = { nil, nil, nil, nil, nil };
static int s_odHideCount = 0;
static bool s_odDistObs = false;

#ifdef VICEEXT_RECOIL
// Residual no compensado (sin decay ni tope vitalicio) y cola fija de solicitudes.
// Un reset estructural limpia el residual; soltar/recargar/cambiar de arma no.
struct ViceExtRecoilRequest {
	uint32 shotSeq;
	float rad;
};
static ViceExtRecoilRequest s_odRecoilQueue[256];
static uint32 s_odRecoilQueueHead = 0;
static uint32 s_odRecoilQueueCount = 0;
static uint32 s_odRecoilQueueLost = 0;
static float s_odRecoilResidual = 0.0f;

void
ViceExtRecoilAlphaAdd(float rad, uint32 shotSeq)
{
	if (rad <= 0.0f)
		return;
	if (s_odRecoilQueueCount >= ARRAY_SIZE(s_odRecoilQueue)) {
		s_odRecoilQueueLost++;
		char t[128];
		snprintf(t, sizeof t, "RECOIL_QUEUE_OVERFLOW shotSeq=%u lost=%u capacity=%u",
			(unsigned)shotSeq, (unsigned)s_odRecoilQueueLost, (unsigned)ARRAY_SIZE(s_odRecoilQueue));
		CWeapon::ViceExtRecoilTrace(t);
		return;
	}
	uint32 tail = (s_odRecoilQueueHead + s_odRecoilQueueCount) % ARRAY_SIZE(s_odRecoilQueue);
	s_odRecoilQueue[tail].shotSeq = shotSeq;
	s_odRecoilQueue[tail].rad = rad;
	s_odRecoilQueueCount++;
}

void
CWeapon::ViceExtRecoilBegin(float &alpha, bool reset, int32 mode, const char *source)
{
	float before = s_odRecoilResidual;
	if (reset) {
		s_odRecoilResidual = 0.0f;
		static uint32 s_odRecoilResetMs = 0;
		uint32 odResetNow = CTimer::GetTimeInMilliseconds();
		if (ViceExtRecoilTraceEnabled() && (s_odRecoilResetMs == 0 || odResetNow < s_odRecoilResetMs || odResetNow - s_odRecoilResetMs >= 1000)) {
			s_odRecoilResetMs = odResetNow;
			char t[192];
			snprintf(t, sizeof t, "RECOIL_RESET reason=ResetStatics source=%s mode=%d residualBeforeRad=%.6f residualAfterRad=0 alphaRad=%.6f tickMs=%u",
				source, mode, before, alpha, (unsigned)CTimer::GetTimeInMilliseconds());
			ViceExtRecoilTrace(t);
		}
	} else {
		alpha -= s_odRecoilResidual;
	}
}

void
CWeapon::ViceExtRecoilApply(float &alpha, float manualDeltaRad, float inputY,
	const char *source, int32 mode, float minAlpha, float maxAlpha)
{
	float residualBefore = s_odRecoilResidual;
	if (manualDeltaRad < 0.0f && s_odRecoilResidual > 0.0f) {
		float compensated = Min(s_odRecoilResidual, -manualDeltaRad);
		s_odRecoilResidual -= compensated;
		// Alpha está separado del residual desde Begin(); reparametriza para que
		// la cámara se mueva sólo el delta manual medido, no dos veces ese delta.
		alpha += compensated;
	}
	alpha += s_odRecoilResidual;
	float bounded = Max(minAlpha, Min(maxAlpha, alpha));
	if (bounded != alpha) {
		// El clamp normal puede ocultar parte del residual; no se guarda energía
		// invisible que reaparezca más tarde al bajar la cámara.
		if (bounded < alpha && s_odRecoilResidual > 0.0f)
			s_odRecoilResidual = Max(0.0f, bounded - (alpha - s_odRecoilResidual));
		alpha = bounded;
	}

	static float s_controlDelta = 0.0f;
	static float s_controlInputY = 0.0f;
	static uint32 s_controlAt = 0;
	static const char *s_controlSource = "none";
	uint32 now = CTimer::GetTimeInMilliseconds();
	bool controlMoved = Abs(manualDeltaRad) > 0.000001f || Abs(inputY) > 0.000001f;
	if (controlMoved) {
		s_controlDelta += manualDeltaRad;
		s_controlInputY += inputY;
		if (s_controlSource == "none")
			s_controlSource = source;
		else if (strcmp(s_controlSource, source) != 0)
			s_controlSource = "mixed";
	}
	if (s_controlDelta != 0.0f || s_controlInputY != 0.0f) {
		const char *controlReason = !controlMoved ? "movement-end" :
			(now - s_controlAt >= 250 ? "interval" : nil);
		if (controlReason) {
			CWeapon::ViceExtRecoilRecordControl(s_controlDelta, s_controlInputY, s_controlSource,
				mode, residualBefore, s_odRecoilResidual, alpha, controlReason);
			s_controlDelta = 0.0f;
			s_controlInputY = 0.0f;
			s_controlSource = "none";
			s_controlAt = now;
		}
	}

	static int32 s_lastMode = -1;
	static const char *s_lastResetReason = "none";
	if (s_lastMode != mode) {
		if (s_lastMode >= 0 && CWeapon::ViceExtRecoilTraceEnabled()) {
			char t[128];
			snprintf(t, sizeof t, "RECOIL_CAMERA edge=mode before=%d after=%d source=%s tickMs=%u",
				s_lastMode, mode, source, (unsigned)now);
			CWeapon::ViceExtRecoilTrace(t);
		}
		s_lastMode = mode;
		s_lastResetReason = "mode-change";
	}

	static bool s_persistWasDown = false;
	static bool s_persistReady = false;
	static int32 s_persistMode = -1;
	static uint32 s_persistNextMs = 0;
	static float s_persistAlpha = 0.0f;
	static float s_persistAlphaDelta = 0.0f;
	static float s_persistManualDelta = 0.0f;
	bool triggerDown = !!CPad::GetPad(0)->GetWeapon();
	if (CWeapon::ViceExtRecoilTraceEnabled() && s_odRecoilResidual > 0.0f) {
		if (!s_persistReady || s_persistMode != mode) {
			s_persistReady = true;
			s_persistMode = mode;
			s_persistAlpha = alpha;
			s_persistAlphaDelta = 0.0f;
			s_persistManualDelta = 0.0f;
			s_persistNextMs = now + 1000;
		}
		s_persistAlphaDelta += alpha - s_persistAlpha;
		s_persistAlpha = alpha;
		s_persistManualDelta += manualDeltaRad;
		bool released = s_persistWasDown && !triggerDown;
		bool checkpoint = released || (s_persistNextMs && (int32)(now - s_persistNextMs) >= 0);
		if (!triggerDown && checkpoint) {
			char t[224];
			snprintf(t, sizeof t, "RECOIL_PERSIST reason=%s mode=%d alphaRad=%.6f deltaAlphaRad=%.6f manualDeltaRad=%.6f autoDeltaRad=%.6f residualRad=%.6f resetReason=%s tickMs=%u",
				released ? "release" : "interval", mode, alpha, s_persistAlphaDelta,
				s_persistManualDelta, s_persistAlphaDelta - s_persistManualDelta,
				s_odRecoilResidual, s_lastResetReason, (unsigned)now);
			CWeapon::ViceExtRecoilTrace(t);
			s_persistAlphaDelta = 0.0f;
			s_persistManualDelta = 0.0f;
			s_persistNextMs = now + 5000;
		}
	} else {
		s_persistReady = false;
		if (s_odRecoilResidual == 0.0f)
			s_lastResetReason = "compensated-or-reset";
	}
	s_persistWasDown = triggerDown;

	while (s_odRecoilQueueCount > 0) {
		ViceExtRecoilRequest request = s_odRecoilQueue[s_odRecoilQueueHead];
		s_odRecoilQueueHead = (s_odRecoilQueueHead + 1) % ARRAY_SIZE(s_odRecoilQueue);
		s_odRecoilQueueCount--;
		float before = alpha;
		float requestResidualBefore = s_odRecoilResidual;
		float after = Max(minAlpha, Min(maxAlpha, before + request.rad));
		float applied = after - before;
		bool clamp = applied + 0.000001f < request.rad;
		alpha = after;
		s_odRecoilResidual += applied;
		if (CWeapon::ViceExtRecoilTraceEnabled()) {
			char t[224];
			snprintf(t, sizeof t, "RECOIL_APPLY shotSeq=%u source=%s mode=%d alphaBeforeRad=%.6f alphaAfterRad=%.6f requestedRad=%.6f appliedRad=%.6f residualBeforeRad=%.6f residualAfterRad=%.6f clamp=%d saturated=%d tickMs=%u",
				(unsigned)request.shotSeq, source, mode, before, after, request.rad, applied,
				requestResidualBefore, s_odRecoilResidual, clamp ? 1 : 0, applied <= 0.000001f ? 1 : 0,
				(unsigned)now);
			CWeapon::ViceExtRecoilTrace(t);
		}
	}
	(void)s_odRecoilQueueLost;
}
#endif


bool PrintDebugCode = false;
int16 DebugCamMode;

extern float fRangePlayerRadius;
extern float fCloseNearClipLimit;

#ifdef FREE_CAM
bool CCamera::bFreeCam = false;
int nPreviousMode = -1;
#endif

void
CCam::Init(void)
{
	Mode = MODE_FOLLOWPED;
	Front = CVector(0.0f, 0.0f, -1.0f);
	Up = CVector(0.0f, 0.0f, 1.0f);
	Rotating = false;
	m_iDoCollisionChecksOnFrameNum = 1;
	m_iDoCollisionCheckEveryNumOfFrames = 9;
	m_iFrameNumWereAt = 0;
	m_bCollisionChecksOn = false;
	m_fRealGroundDist = 0.0f;
	BetaSpeed = 0.0f;
	AlphaSpeed = 0.0f;
	DistanceSpeed = 0.0f;
	f_max_role_angle = DEGTORAD(5.0f);
	Distance = 30.0f;
	DistanceSpeed = 0.0f;
	m_pLastCarEntered = nil;
	m_pLastPedLookedAt = nil;
	ResetStatics = true;
	Beta = 0.0f;
	m_fTilt = 0.0f;
	m_fTiltSpeed = 0.0f;
	m_bFixingBeta = false;
	CA_MIN_DISTANCE = 0.0f;
	CA_MAX_DISTANCE = 0.0f;
	LookingBehind = false;
	LookingLeft = false;
	LookingRight = false;
	m_fPlayerInFrontSyphonAngleOffSet = DEGTORAD(20.0f);
	m_fSyphonModeTargetZOffSet = 0.5f;
	m_fRadiusForDead = 1.5f;
	DirectionWasLooking = LOOKING_FORWARD;
	LookBehindCamWasInFront = false;
	f_Roll = 0.0f;
	f_rollSpeed = 0.0f;
	m_fCloseInPedHeightOffset = 0.0f;
	m_fCloseInPedHeightOffsetSpeed = 0.0f;
	m_fCloseInCarHeightOffset = 0.0f;
	m_fCloseInCarHeightOffsetSpeed = 0.0f;
	m_fPedBetweenCameraHeightOffset = 0.0f;
	m_fTargetBeta = 0.0f;
	m_fBufferedTargetBeta = 0.0f;
	m_fBufferedTargetOrientation = 0.0f;
	m_fBufferedTargetOrientationSpeed = 0.0f;
	m_fDimensionOfHighestNearCar = 0.0f;
}

float PLAYERPED_LEVEL_SMOOTHING_CONST_INV = 0.6f;
float PLAYERPED_TREND_SMOOTHING_CONST_INV = 0.8f;

void
CCam::Process(void)
{
	CVector CameraTarget;
	float TargetSpeedVar = 0.0f;
	float TargetOrientation = 0.0f;

	static CVector SmoothedPos(0.0f, 0.0f, 10000.0f);
	static CVector SmoothedSpeed(0.0f, 0.0f, 0.0f);

	if(CamTargetEntity == nil)
		CamTargetEntity = TheCamera.pTargetEntity;

	m_iFrameNumWereAt++;
	if(m_iFrameNumWereAt > m_iDoCollisionCheckEveryNumOfFrames)
		m_iFrameNumWereAt = 1;
	m_bCollisionChecksOn = m_iFrameNumWereAt == m_iDoCollisionChecksOnFrameNum;

	if(m_bCamLookingAtVector){
		CameraTarget = m_cvecCamFixedModeVector;
	}else if(CamTargetEntity->IsVehicle()){
		CameraTarget = CamTargetEntity->GetPosition();

		if(CamTargetEntity->GetForward().x == 0.0f && CamTargetEntity->GetForward().y == 0.0f)
			TargetOrientation = 0.0f;
		else
			TargetOrientation = CGeneral::GetATanOfXY(CamTargetEntity->GetForward().x, CamTargetEntity->GetForward().y);

		CVector Fwd(0.0f, 0.0f, 0.0f);
		Fwd.x = CamTargetEntity->GetForward().x;
		Fwd.y = CamTargetEntity->GetForward().y;
		Fwd.Normalise();
		float FwdLength = Fwd.Magnitude2D();
		if(FwdLength != 0.0f){
			Fwd.x /= FwdLength;
			Fwd.y /= FwdLength;
		}

		float FwdSpeedX = ((CVehicle*)CamTargetEntity)->GetMoveSpeed().x * Fwd.x;
		float FwdSpeedY = ((CVehicle*)CamTargetEntity)->GetMoveSpeed().y * Fwd.y;
		if(FwdSpeedX + FwdSpeedY > 0.0f)
			TargetSpeedVar = Min(Sqrt(SQR(FwdSpeedX) + SQR(FwdSpeedY))/0.9f, 1.0f);
		else
			TargetSpeedVar = -Min(Sqrt(SQR(FwdSpeedX) + SQR(FwdSpeedY))/1.8f, 0.5f);
		SpeedVar = 0.895f*SpeedVar + 0.105*TargetSpeedVar;
	}else{
		if(CamTargetEntity == FindPlayerPed()){
			// Some fancy smoothing of player position and speed
			float LevelSmoothing = 1.0f - Pow(PLAYERPED_LEVEL_SMOOTHING_CONST_INV, CTimer::GetTimeStep());
			float TrendSmoothing = 1.0f - Pow(PLAYERPED_TREND_SMOOTHING_CONST_INV, CTimer::GetTimeStep());

			CVector NewSmoothedPos, NewSmoothedSpeed;
			if((SmoothedPos - CamTargetEntity->GetPosition()).MagnitudeSqr() > SQR(3.0f) ||
			   CTimer::GetTimeStep() < 0.2f || Using3rdPersonMouseCam()){
				// Reset values
				NewSmoothedPos = CamTargetEntity->GetPosition();
				NewSmoothedSpeed = CVector(0.0f, 0.0f, 0.0f);
			}else{
				NewSmoothedPos = LevelSmoothing*CamTargetEntity->GetPosition() + (1.0f-LevelSmoothing)*(SmoothedPos + SmoothedSpeed*CTimer::GetTimeStep());
				NewSmoothedSpeed = TrendSmoothing*(NewSmoothedPos-SmoothedPos)/CTimer::GetTimeStep() + (1.0f-TrendSmoothing)*SmoothedSpeed;
			}
			   
			CameraTarget = NewSmoothedPos;
			SmoothedPos = NewSmoothedPos;
			SmoothedSpeed = NewSmoothedSpeed;
		}else
			CameraTarget = CamTargetEntity->GetPosition();

		if(CamTargetEntity->GetForward().x == 0.0f && CamTargetEntity->GetForward().y == 0.0f)
			TargetOrientation = 0.0f;
		else
			TargetOrientation = CGeneral::GetATanOfXY(CamTargetEntity->GetForward().x, CamTargetEntity->GetForward().y);
		TargetSpeedVar = 0.0f;
		SpeedVar = 0.0f;
	}

	switch(Mode){
	case MODE_TOPDOWN:
	case MODE_GTACLASSIC:
	//	Process_TopDown(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_BEHINDCAR:
		Process_BehindCar(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_FOLLOWPED:
#ifdef PC_PLAYER_CONTROLS
		if(CCamera::m_bUseMouse3rdPerson)
			Process_FollowPedWithMouse(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		else
#endif
#ifdef FREE_CAM
			if(CCamera::bFreeCam)
				Process_FollowPed_Rotation(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
			else
#endif
			Process_FollowPed(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_AIMING:
		Process_AimWeapon(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_DEBUG:
		Process_Debug(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_SNIPER:
	case MODE_CAMERA:
		Process_Sniper(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_ROCKETLAUNCHER:
		Process_Rocket(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_MODELVIEW:
		Process_ModelView(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
//	case MODE_BILL:
	case MODE_SYPHON:
		Process_Syphon(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_CIRCLE:
//		Process_Circle(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
//	case MODE_CHEESYZOOM:
	case MODE_WHEELCAM:
		Process_WheelCam(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_FIXED:
		Process_Fixed(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_1STPERSON:
		Process_1stPerson(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_FLYBY:
		Process_FlyBy(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_CAM_ON_A_STRING:
#ifdef FREE_CAM
		if(CCamera::bFreeCam && !CVehicle::bCheat5)
			Process_FollowCar_SA(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		else
#endif
			Process_Cam_On_A_String(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
//	case MODE_REACTION:
//	case MODE_FOLLOW_PED_WITH_BIND:
//	case MODE_CHRIS:
	case MODE_BEHINDBOAT:
#ifdef FREE_CAM
		if (CCamera::bFreeCam)
			Process_FollowCar_SA(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		else
#endif
			Process_BehindBoat(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_PLAYER_FALLEN_WATER:
		Process_Player_Fallen_Water(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
//	case MODE_CAM_ON_TRAIN_ROOF:
//	case MODE_CAM_RUNNING_SIDE_TRAIN:
//	case MODE_BLOOD_ON_THE_TRACKS:
//	case MODE_IM_THE_PASSENGER_WOOWOO:
	case MODE_SYPHON_CRIM_IN_FRONT:
		Process_Syphon_Crim_In_Front(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_PED_DEAD_BABY:
		ProcessPedsDeadBaby();
		break;
//	case MODE_PILLOWS_PAPS:
//	case MODE_LOOK_AT_CARS:
	case MODE_ARRESTCAM_ONE:
		ProcessArrestCamOne();
		break;
	case MODE_ARRESTCAM_TWO:
		ProcessArrestCamTwo();
		break;
	case MODE_M16_1STPERSON:
	case MODE_HELICANNON_1STPERSON:
		Process_M16_1stPerson(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_SPECIAL_FIXED_FOR_SYPHON:
		Process_SpecialFixedForSyphon(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_FIGHT_CAM:
		Process_Fight_Cam(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_LIGHTHOUSE:
		Process_LightHouse(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_TOP_DOWN_PED:
	//	Process_TopDownPed(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_SNIPER_RUNABOUT:
	case MODE_ROCKETLAUNCHER_RUNABOUT:
	case MODE_1STPERSON_RUNABOUT:
	case MODE_M16_1STPERSON_RUNABOUT:
	case MODE_FIGHT_CAM_RUNABOUT:
		Process_1rstPersonPedOnPC(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
#ifdef GTA_SCENE_EDIT
	case MODE_EDITOR:
		Process_Editor(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
#endif
	default:
		Source = CVector(0.0f, 0.0f, 0.0f);
		Front = CVector(0.0f, 1.0f, 0.0f);
		Up = CVector(0.0f, 0.0f, 1.0f);
	}

#ifdef FREE_CAM
	nPreviousMode = Mode;
#endif
	CVector TargetToCam = Source - m_cvecTargetCoorsForFudgeInter;
	float DistOnGround = TargetToCam.Magnitude2D();
	m_fTrueBeta = CGeneral::GetATanOfXY(TargetToCam.x, TargetToCam.y);
	m_fTrueAlpha = CGeneral::GetATanOfXY(DistOnGround, TargetToCam.z);
	if(TheCamera.m_uiTransitionState == 0)
		KeepTrackOfTheSpeed(Source, m_cvecTargetCoorsForFudgeInter, Up, m_fTrueAlpha, m_fTrueBeta, FOV);

	// Look Behind, Left, Right
	LookingBehind = false;
	LookingLeft = false;
	LookingRight = false;
	SourceBeforeLookBehind = Source;
	if(&TheCamera.Cams[TheCamera.ActiveCam] == this){
		if((Mode == MODE_CAM_ON_A_STRING || Mode == MODE_1STPERSON || Mode == MODE_BEHINDBOAT || Mode == MODE_BEHINDCAR) &&
		   CamTargetEntity->IsVehicle()){
			bool bDisableLR = CamTargetEntity &&
				(((CVehicle*)CamTargetEntity)->GetVehicleAppearance() == VEHICLE_APPEARANCE_HELI || CamTargetEntity->GetModelIndex() == MI_RCBARON);
			if(CPad::GetPad(0)->GetLookBehindForCar()){
				LookBehind();
				s_odBetaSnap("mirar-atras");
				if(DirectionWasLooking != LOOKING_BEHIND)
					TheCamera.m_bJust_Switched = true;
				DirectionWasLooking = LOOKING_BEHIND;
			}else if(bDisableLR){
				if(DirectionWasLooking != LOOKING_FORWARD)
					TheCamera.m_bJust_Switched = true;
				DirectionWasLooking = LOOKING_FORWARD;
			}else if(CPad::GetPad(0)->GetLookLeft()){
				LookLeft();
				s_odBetaSnap("snap-lados");
				if(DirectionWasLooking != LOOKING_LEFT)
					TheCamera.m_bJust_Switched = true;
				DirectionWasLooking = LOOKING_LEFT;
			}else if(CPad::GetPad(0)->GetLookRight()){
				LookRight();
				s_odBetaSnap("snap-lados");
				if(DirectionWasLooking != LOOKING_RIGHT)
					TheCamera.m_bJust_Switched = true;
				DirectionWasLooking = LOOKING_RIGHT;
			}else{
				if(DirectionWasLooking != LOOKING_FORWARD)
					TheCamera.m_bJust_Switched = true;
				DirectionWasLooking = LOOKING_FORWARD;
			}
		}
		if(Mode == MODE_FOLLOWPED && CamTargetEntity->IsPed()){
			if(CPad::GetPad(0)->GetLookBehindForPed()){
				LookBehind();
				if(DirectionWasLooking != LOOKING_BEHIND)
					TheCamera.m_bJust_Switched = true;
				DirectionWasLooking = LOOKING_BEHIND;
			}else
				DirectionWasLooking = LOOKING_FORWARD;
		}
	}

	if(Mode == MODE_SNIPER || Mode == MODE_ROCKETLAUNCHER || Mode == MODE_M16_1STPERSON ||
	   Mode == MODE_1STPERSON || Mode == MODE_HELICANNON_1STPERSON || Mode == MODE_CAMERA || GetWeaponFirstPersonOn())
		ClipIfPedInFrontOfPlayer();
}

// MaxSpeed is a limit of how fast the value is allowed to change. 1.0 = to Target in up to 1ms
// Acceleration is how fast the speed will change to MaxSpeed. 1.0 = to MaxSpeed in 1ms
void
WellBufferMe(float Target, float *CurrentValue, float *CurrentSpeed, float MaxSpeed, float Acceleration, bool IsAngle)
{
	float Delta = Target - *CurrentValue;

	if(IsAngle){
		while(Delta >= PI) Delta -= 2*PI;
		while(Delta < -PI) Delta += 2*PI;
	}

	float TargetSpeed = Delta * MaxSpeed;
	// Add or subtract absolute depending on sign, genius!
//	if(TargetSpeed - *CurrentSpeed > 0.0f)
//		*CurrentSpeed += Acceleration * Abs(TargetSpeed - *CurrentSpeed) * CTimer::GetTimeStep();
//	else
//		*CurrentSpeed -= Acceleration * Abs(TargetSpeed - *CurrentSpeed) * CTimer::GetTimeStep();
	// this is simpler:
	*CurrentSpeed += Acceleration * (TargetSpeed - *CurrentSpeed) * CTimer::GetTimeStep();

	// Clamp speed if we overshot
	if(TargetSpeed < 0.0f && *CurrentSpeed < TargetSpeed)
		*CurrentSpeed = TargetSpeed;
	else if(TargetSpeed > 0.0f && *CurrentSpeed > TargetSpeed)
		*CurrentSpeed = TargetSpeed;

	*CurrentValue += *CurrentSpeed * Min(10.0f, CTimer::GetTimeStep());
}

// B4 — APUNTADO, spec ClassicAXIS (CamNew.cpp Process_AimWeapon + Settings):
// hombro lateral al apuntar: 0.2 normal, 0.55 estilo stories. En el mod lo elige
// su ajuste StoriesAimingCoords y no hay tecla; aquí es un conmutador interno con
// 0.2 por defecto. PREGUNTA al jugador (informe): con qué se cambia
// (tecla/menú/auto) — sin respuesta no se inventa binding.
// (Colocado tras WellBufferMe porque el suavizado lo usa.)
static bool s_odAimStoriesShoulder = false;

static float
ViceExtAimShoulder(void)
{
	return s_odAimStoriesShoulder ? 0.55f : 0.2f;
}

// ¿Apunta el jugador con arma de apuntar? Puerta del hombro. El flag CANAIM vale
// para casi todas; las escopetas no lo traen en weapon.dat pero sí apuntan
// (PlayerPed.cpp ViceExtCanAim, VICEEXT_SHOTGUN_AIM, bloque C7): se listan igual.
static bool
ViceExtAimingOverShoulder(CEntity *target)
{
	if (!target || !target->IsPed())
		return false;
	CPed *ped = (CPed*)target;
	if (!ped->IsPlayer())
		return false;
#ifdef VICEEXT_PEDARBITER
	if (ViceExtPedOwns(PEDLANE_NADO, PEDCAP_APUNTAR)
	 || ViceExtPedOwns(PEDLANE_NADO, PEDCAP_CAMARA))
		return false;
#endif
	if (CPad::GetPad(0)->GetTarget() == 0)
		return false;
	eWeaponType wt = ped->GetWeapon()->m_eWeaponType;
	CWeaponInfo *info = CWeaponInfo::GetWeaponInfo(wt);
	if (!info)
		return false;
	if (info->IsFlagSet(WEAPONFLAG_CANAIM))
		return true;
#ifdef VICEEXT_SHOTGUN_AIM
	switch (wt) {
	case WEAPONTYPE_SHOTGUN:
	case WEAPONTYPE_SPAS12_SHOTGUN:
	case WEAPONTYPE_STUBBY_SHOTGUN:
	case WEAPONTYPE_SHOTGUN2:
		return true;
	default:
		break;
	}
#endif
	return false;
}

// Hombro suavizado (metros, en la derecha del ped): evita el salto de 0.2 m al
// pulsar apuntar. Se resetea con ResetStatics (cambio de modo/vehículo).
static float
ViceExtAimShoulderSmoothed(CEntity *target, bool reset)
{
	static float cur = 0.0f;
	static float spd = 0.0f;
	if (reset) {
		cur = 0.0f;
		spd = 0.0f;
		return 0.0f;
	}
	float want = ViceExtAimingOverShoulder(target) ? ViceExtAimShoulder() : 0.0f;
	WellBufferMe(want, &cur, &spd, 0.2f, 0.1f, false);
	return cur;
}

void
MakeAngleLessThan180(float &Angle)
{
	while(Angle >= PI) Angle -= 2*PI;
	while(Angle < -PI) Angle += 2*PI;
}

void
CCam::ProcessSpecialHeightRoutines(void)
{
	int i;
	bool StandingOnBoat = false;
	static bool PreviouslyFailedRoadHeightCheck = false;
	CVector CamToTarget, CamToPed;
	float DistOnGround, BetaAngle;
	CPed *Player;
	float PedZDist;
CColPoint colPoint;

CamToTarget = TheCamera.pTargetEntity->GetPosition() - TheCamera.GetGameCamPosition();
DistOnGround = CamToTarget.Magnitude2D();
BetaAngle = CGeneral::GetATanOfXY(CamToTarget.x, CamToTarget.y);
m_bTheHeightFixerVehicleIsATrain = false;
// CGeneral::GetATanOfXY(TheCamera.GetForward().x, TheCamera.GetForward().y);
Player = CWorld::Players[CWorld::PlayerInFocus].m_pPed;

if(DistOnGround > 10.0f)
DistOnGround = 10.0f;

if(CamTargetEntity && CamTargetEntity->IsPed()){
if(FindPlayerPed()->m_pCurSurface && FindPlayerPed()->m_pCurSurface->IsVehicle() &&
   ((CVehicle*)FindPlayerPed()->m_pCurSurface)->IsBoat())
StandingOnBoat = true;

float FoundPedZ = -100.0f;

// Move up the camera if there is a ped close to it
if(Mode == MODE_FOLLOWPED || Mode == MODE_FIGHT_CAM || Mode == MODE_PILLOWS_PAPS){
// Find highest ped close to camera
for(i = 0; i < Player->m_numNearPeds; i++){
CPed *nearPed = Player->m_nearPeds[i];
if(nearPed && nearPed->GetPedState() != PED_DEAD){
CamToPed = nearPed->GetPosition() - TheCamera.GetGameCamPosition();
if(Abs(CamToPed.z) < 1.0f){
float DistSq = CamToPed.MagnitudeSqr();
if(DistSq < SQR(2.1f)){
if(nearPed->GetPosition().z > FoundPedZ)
FoundPedZ = nearPed->GetPosition().z;
}else{
float Dist = Sqrt(DistSq);
CamToPed /= Dist;
// strange calculation
CVector PlayerCamSpeed = DotProduct(Front, Player->m_vecMoveSpeed)*Front;
float SpeedDiff = DotProduct(PlayerCamSpeed - nearPed->m_vecMoveSpeed, CamToPed);
if(SpeedDiff > 0.01f &&
   (m_fPedBetweenCameraHeightOffset > 0.0f && (Dist-2.1f)/SpeedDiff < 75.0f ||
    m_fPedBetweenCameraHeightOffset <= 0.0f && (Dist-2.1f)/SpeedDiff < 75.0f * 0.1f))
if(nearPed->GetPosition().z > FoundPedZ)
FoundPedZ = nearPed->GetPosition().z;
}
}
}
}

if(FoundPedZ > -99.0f){
float Offset = 0.0f;
PedZDist = 0.0f;
if(FoundPedZ > Player->GetPosition().z)
PedZDist = FoundPedZ - Player->GetPosition().z;

if(Mode == MODE_FOLLOWPED){
if(TheCamera.PedZoomIndicator == CAM_ZOOM_1 &&
   ((CPed*)CamTargetEntity)->GetPedState() != PED_ENTER_CAR &&
   ((CPed*)CamTargetEntity)->GetPedState() != PED_CARJACK)
Offset = 0.45f + PedZDist;
// BUG: overrides this ^ case
if(TheCamera.PedZoomIndicator == CAM_ZOOM_2 || TheCamera.PedZoomIndicator == CAM_ZOOM_1)
Offset = 0.35f + PedZDist;
if(TheCamera.PedZoomIndicator == CAM_ZOOM_3)
Offset = 0.25f + PedZDist;
m_fPedBetweenCameraHeightOffset = Offset + 1.3f;
}else if(Mode == MODE_FIGHT_CAM)
m_fPedBetweenCameraHeightOffset = PedZDist + 1.3f + 0.5f;
else if(Mode == MODE_PILLOWS_PAPS)
m_fPedBetweenCameraHeightOffset = PedZDist + 1.3f + 0.45f;
}else{
m_fPedBetweenCameraHeightOffset = 0.0f;
}
}


// Move camera up for vehicles in the way
if(m_bCollisionChecksOn && (Mode == MODE_FOLLOWPED || Mode == MODE_FIGHT_CAM)){
bool FoundCar = false;
CEntity *vehicle = nil;
float TestDist = DistOnGround + 1.25f;
float HighestCar = 0.0f;
if(m_fDimensionOfHighestNearCar > 0.0f)
TestDist += 0.3f;
CVector TestBase = CamTargetEntity->GetPosition();
CVector TestPoint;
TestBase.z -= 0.15f;

TestPoint = TestBase - TestDist * CVector(Cos(BetaAngle), Sin(BetaAngle), 0.0f);
if(CWorld::ProcessLineOfSight(CamTargetEntity->GetPosition(), TestPoint, colPoint, vehicle, false, true, false, false, false, false) &&
   vehicle->IsVehicle()){
float height = vehicle->GetColModel()->boundingBox.GetSize().z;
FoundCar = true;
HighestCar = height;
if(((CVehicle*)vehicle)->IsTrain())
m_bTheHeightFixerVehicleIsATrain = true;
}

TestPoint = TestBase - TestDist * CVector(Cos(BetaAngle+DEGTORAD(28.0f)), Sin(BetaAngle+DEGTORAD(28.0f)), 0.0f);
if(CWorld::ProcessLineOfSight(CamTargetEntity->GetPosition(), TestPoint, colPoint, vehicle, false, true, false, false, false, false) &&
   vehicle->IsVehicle()){
float height = vehicle->GetColModel()->boundingBox.GetSize().z;
if(FoundCar){
HighestCar = Max(HighestCar, height);
}else{
FoundCar = true;
HighestCar = height;
}
if(((CVehicle*)vehicle)->IsTrain())
m_bTheHeightFixerVehicleIsATrain = true;
}

TestPoint = TestBase - TestDist * CVector(Cos(BetaAngle-DEGTORAD(28.0f)), Sin(BetaAngle-DEGTORAD(28.0f)), 0.0f);
if(CWorld::ProcessLineOfSight(CamTargetEntity->GetPosition(), TestPoint, colPoint, vehicle, false, true, false, false, false, false) &&
   vehicle->IsVehicle()){
float height = vehicle->GetColModel()->boundingBox.GetSize().z;
if(FoundCar){
HighestCar = Max(HighestCar, height);
}else{
FoundCar = true;
HighestCar = height;
}
if(((CVehicle*)vehicle)->IsTrain())
m_bTheHeightFixerVehicleIsATrain = true;
}

if(FoundCar){
m_fDimensionOfHighestNearCar = HighestCar + 0.1f;
if(Mode == MODE_FIGHT_CAM)
m_fDimensionOfHighestNearCar += 0.75f;
}else
m_fDimensionOfHighestNearCar = 0.0f;
}
	}

	if(StandingOnBoat){
		m_fDimensionOfHighestNearCar = 1.0f;
		m_fPedBetweenCameraHeightOffset = 0.0f;
	}
}

void
CCam::GetVectorsReadyForRW(void)
{
	CVector right;
	Up = CVector(0.0f, 0.0f, 1.0f);
	Front.Normalise();
	if(Front.x == 0.0f && Front.y == 0.0f){
		Front.x = 0.0001f;
		Front.y = 0.0001f;
	}
	right = CrossProduct(Front, Up);
	right.Normalise();
	Up = CrossProduct(right, Front);
}

bool
CCam::GetBoatLook_L_R_HeightOffset(float &Offset)
{
	if(CamTargetEntity == nil)
		return false;
	CVehicleModelInfo *mi = (CVehicleModelInfo*)CModelInfo::GetModelInfo(CamTargetEntity->GetModelIndex());
	tBoatHandlingData *handling = mod_HandlingManager.GetBoatPointer(mi->m_handlingId);
	if(handling){
		Offset = handling->fLook_L_R_BehindCamHeight;
		return true;
	}
	return false;	// can't happen, we always get a boat pointer back
}

void
CCam::LookBehind(void)
{
	float Dist, DeltaBeta, TargetOrientation, Angle;
	CVector TargetCoors, TargetFwd, TestCoors;

	TargetCoors = CamTargetEntity->GetPosition();
	Front = CamTargetEntity->GetPosition() - Source;

	if((Mode == MODE_CAM_ON_A_STRING || Mode == MODE_BEHINDBOAT || Mode == MODE_BEHINDCAR) && CamTargetEntity->IsVehicle()){
		LookingBehind = true;
		Dist = Mode == MODE_CAM_ON_A_STRING ? CA_MAX_DISTANCE : 15.5f;
		TargetFwd = CamTargetEntity->GetForward();
		TargetFwd.Normalise();
		TargetOrientation = CGeneral::GetATanOfXY(TargetFwd.x, TargetFwd.y);
		DeltaBeta = TargetOrientation - Beta;
		while(DeltaBeta >= PI) DeltaBeta -= 2*PI;
		while(DeltaBeta < -PI) DeltaBeta += 2*PI;
		if(DirectionWasLooking != LOOKING_BEHIND)
			LookBehindCamWasInFront = DeltaBeta <= -HALFPI || DeltaBeta >= HALFPI;
		if(LookBehindCamWasInFront)
			TargetOrientation += PI;
		Source.x = Dist*Cos(TargetOrientation) + TargetCoors.x;
		Source.y = Dist*Sin(TargetOrientation) + TargetCoors.y;
		CVector OrigSource = Source;
		TheCamera.AvoidTheGeometry(OrigSource, TargetCoors, Source, FOV);
		Front = CamTargetEntity->GetPosition() - Source;
		GetVectorsReadyForRW();
	}
	if(Mode == MODE_1STPERSON && CamTargetEntity->IsVehicle()){
		LookingBehind = true;
		RwCameraSetNearClipPlane(Scene.camera, 0.25f);
		Front = CamTargetEntity->GetForward();
		Front.Normalise();
		if(((CVehicle*)CamTargetEntity)->IsBoat())
			Source.z -= 0.5f;
		if(((CVehicle*)CamTargetEntity)->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE){
			float FrontDist = 1.1f;
			if(((CVehicle*)CamTargetEntity)->pDriver){
				CVector ExtraFwd(0.0f, 0.0f, 0.0f);
				((CVehicle*)CamTargetEntity)->pDriver->m_pedIK.GetComponentPosition(ExtraFwd, PED_HEAD);
				ExtraFwd += ((CVehicle*)CamTargetEntity)->m_vecMoveSpeed*CTimer::GetTimeStep() - CamTargetEntity->GetPosition();
				FrontDist += 0.2f + Max(DotProduct(ExtraFwd, CamTargetEntity->GetForward()), 0.0f);
			}
			Source += FrontDist*Front;
			Front = -Front;
		}else if(((CVehicle*)CamTargetEntity)->GetVehicleAppearance() == VEHICLE_APPEARANCE_HELI){
			Front = -1.0f*CamTargetEntity->GetUp();
			Up = CamTargetEntity->GetForward();
			Source += 0.25f*Front;
		}else{
			Source += 0.25f*Front;
			Front = -Front;
		}
	}
	if(CamTargetEntity->IsPed()){
		Angle = CGeneral::GetATanOfXY(Source.x - TargetCoors.x, Source.y - TargetCoors.y) + PI;
		Source.x = 4.5f*Cos(Angle) + TargetCoors.x;
		Source.y = 4.5f*Sin(Angle) + TargetCoors.y;
		Source.z = 1.15f + TargetCoors.z;
		CVector OrigSource = Source;
		TheCamera.AvoidTheGeometry(OrigSource, TargetCoors, Source, FOV);
		Front = TargetCoors - Source;
		GetVectorsReadyForRW();
	}
}

float BOAT_1STPERSON_L_OFFSETX = 0.7f;
float BOAT_1STPERSON_R_OFFSETX = 0.3f;
float BOAT_1STPERSON_LR_OFFSETZ = 0.2f;

void
CCam::LookLeft(void)
{
	float Dist, TargetOrientation;
	CVector TargetCoors, TargetFwd;

	if((Mode == MODE_CAM_ON_A_STRING || Mode == MODE_BEHINDBOAT || Mode == MODE_BEHINDCAR) && CamTargetEntity->IsVehicle()){
		LookingLeft = true;
		TargetCoors = CamTargetEntity->GetPosition();
		Front = CamTargetEntity->GetPosition() - Source;
		if(Mode == MODE_CAM_ON_A_STRING)
			Dist = CA_MAX_DISTANCE;
		else if(Mode == MODE_BEHINDBOAT){
			Dist = 9.0f;
			float Offset = 0.0f;
			if(GetBoatLook_L_R_HeightOffset(Offset) && !CCullZones::Cam1stPersonForPlayer())
				Source.z = TargetCoors.z + Offset;
		}else
			Dist = 9.0f;
		TargetFwd = CamTargetEntity->GetForward();
		TargetFwd.Normalise();
		TargetOrientation = CGeneral::GetATanOfXY(TargetFwd.x, TargetFwd.y);
		Source.x = Dist*Cos(TargetOrientation - HALFPI) + TargetCoors.x;
		Source.y = Dist*Sin(TargetOrientation - HALFPI) + TargetCoors.y;

		CColModel *colModel = CamTargetEntity->GetColModel();
		CVector OrigSource = Source;
		TheCamera.AvoidTheGeometry(OrigSource, TargetCoors, Source, FOV);

		CVector TopRight = CamTargetEntity->GetPosition() +
			CamTargetEntity->GetRight()*colModel->boundingBox.max.x +
			CamTargetEntity->GetUp()*colModel->boundingBox.max.z;
		float Height = Min(Max(m_cvecTargetCoorsForFudgeInter.z, TopRight.z)+0.1f, OrigSource.z);
		Source.z = Max(Height, Source.z);

		Front = CamTargetEntity->GetPosition() - Source;
		Front.z += 1.1f;
		if(Mode == MODE_BEHINDBOAT)
			Front.z += 1.2f;
		GetVectorsReadyForRW();
	}
	if(Mode == MODE_1STPERSON && CamTargetEntity->IsVehicle()){
		LookingLeft = true;
		RwCameraSetNearClipPlane(Scene.camera, 0.25f);
		if(((CVehicle*)CamTargetEntity)->IsBoat()){
			if(((CVehicle*)CamTargetEntity)->pDriver){
				CVector neck(0.0f, 0.0f, 0.0f);
				CPed *driver = ((CVehicle*)CamTargetEntity)->pDriver;
				driver->SetPedPositionInCar();
				driver->GetMatrix().UpdateRW();
				driver->UpdateRwFrame();
				driver->UpdateRpHAnim();
				driver->m_pedIK.GetComponentPosition(neck, PED_NECK);
				Source = neck +
					BOAT_1STPERSON_L_OFFSETX*CamTargetEntity->GetRight() +
					BOAT_1STPERSON_LR_OFFSETZ*CamTargetEntity->GetUp();
			}else
				Source.z -= 0.5f;
		}

		Up = CamTargetEntity->GetUp();
		Up.Normalise();
		Front = CamTargetEntity->GetForward();
		Front.Normalise();
		Front = -CrossProduct(Front, Up);
		Front.Normalise();
		if(((CVehicle*)CamTargetEntity)->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE)
			Source -= 1.45f*Front;
	}
}

void
CCam::LookRight(void)
{
	float Dist, TargetOrientation;
	CVector TargetCoors, TargetFwd;
	CColPoint colPoint;

	if((Mode == MODE_CAM_ON_A_STRING || Mode == MODE_BEHINDBOAT || Mode == MODE_BEHINDCAR) && CamTargetEntity->IsVehicle()){
		LookingRight = true;
		TargetCoors = CamTargetEntity->GetPosition();
		Front = CamTargetEntity->GetPosition() - Source;
		if(Mode == MODE_CAM_ON_A_STRING)
			Dist = CA_MAX_DISTANCE;
		else if(Mode == MODE_BEHINDBOAT){
			Dist = 9.0f;
			float Offset = 0.0f;
			if(GetBoatLook_L_R_HeightOffset(Offset) && !CCullZones::Cam1stPersonForPlayer())
				Source.z = TargetCoors.z + Offset;
		}else
			Dist = 9.0f;
		TargetFwd = CamTargetEntity->GetForward();
		TargetFwd.Normalise();
		TargetOrientation = CGeneral::GetATanOfXY(TargetFwd.x, TargetFwd.y);
		Source.x = Dist*Cos(TargetOrientation + HALFPI) + TargetCoors.x;
		Source.y = Dist*Sin(TargetOrientation + HALFPI) + TargetCoors.y;

		CColModel *colModel = CamTargetEntity->GetColModel();
		CVector OrigSource = Source;
		TheCamera.AvoidTheGeometry(OrigSource, TargetCoors, Source, FOV);

		CVector TopLeft = CamTargetEntity->GetPosition() +
			CamTargetEntity->GetRight()*colModel->boundingBox.min.x +
			CamTargetEntity->GetUp()*colModel->boundingBox.max.z;
		float Height = Min(Max(m_cvecTargetCoorsForFudgeInter.z, TopLeft.z)+0.1f, OrigSource.z);
		Source.z = Max(Height, Source.z);

		Front = CamTargetEntity->GetPosition() - Source;
		Front.z += 1.1f;
		if(Mode == MODE_BEHINDBOAT)
			Front.z += 1.2f;
		GetVectorsReadyForRW();
	}
	if(Mode == MODE_1STPERSON && CamTargetEntity->IsVehicle()){
		LookingRight = true;
		RwCameraSetNearClipPlane(Scene.camera, 0.25f);
		if(((CVehicle*)CamTargetEntity)->IsBoat()){
			if(((CVehicle*)CamTargetEntity)->pDriver){
				CVector neck(0.0f, 0.0f, 0.0f);
				CPed *driver = ((CVehicle*)CamTargetEntity)->pDriver;
				driver->SetPedPositionInCar();
				driver->GetMatrix().UpdateRW();
				driver->UpdateRwFrame();
				driver->UpdateRpHAnim();
				driver->m_pedIK.GetComponentPosition(neck, PED_NECK);
				Source = neck +
					BOAT_1STPERSON_R_OFFSETX*CamTargetEntity->GetRight() +
					BOAT_1STPERSON_LR_OFFSETZ*CamTargetEntity->GetUp();
			}else
				Source.z -= 0.5f;
		}

		Up = CamTargetEntity->GetUp();
		Up.Normalise();
		Front = CamTargetEntity->GetForward();
		Front.Normalise();
		Front = CrossProduct(Front, Up);
		Front.Normalise();
		if(((CVehicle*)CamTargetEntity)->GetVehicleAppearance() == VEHICLE_APPEARANCE_BIKE)
			Source -= 1.45f*Front;
	}
}

void
CCam::ClipIfPedInFrontOfPlayer(void)
{
	float FwdAngle, PedAngle, DeltaAngle, fDist, Near;
	CVector vDist;
	CPed *Player;
	bool found = false;
	int ped = 0;

	// unused: TheCamera.pTargetEntity->GetPosition() - TheCamera.GetGameCamPosition();

	FwdAngle = CGeneral::GetATanOfXY(TheCamera.GetForward().x, TheCamera.GetForward().y);
	Player = CWorld::Players[CWorld::PlayerInFocus].m_pPed;
	while(ped < Player->m_numNearPeds && !found)
		if(Player->m_nearPeds[ped] && Player->m_nearPeds[ped]->GetPedState() != PED_DEAD)
			found = true;
		else
			ped++;
	if(found){
		vDist = Player->m_nearPeds[ped]->GetPosition() - TheCamera.GetGameCamPosition();
		PedAngle = CGeneral::GetATanOfXY(vDist.x, vDist.y);
		DeltaAngle = FwdAngle - PedAngle;
		while(DeltaAngle >= PI) DeltaAngle -= 2*PI;
		while(DeltaAngle < -PI) DeltaAngle += 2*PI;
		if(Abs(DeltaAngle) < HALFPI){
			fDist = vDist.Magnitude2D();
			if(fDist < 1.25f){
				Near = DEFAULT_NEAR - (1.25f - fDist);
				if(Near < 0.05f)
					Near = 0.05f;
				RwCameraSetNearClipPlane(Scene.camera, Near);
			}
		}
	}
}

void
CCam::KeepTrackOfTheSpeed(const CVector &source, const CVector &target, const CVector &up, const float &alpha, const float &beta, const float &fov)
{
	static CVector PreviousSource = source;
	static CVector PreviousTarget = target;
	static CVector PreviousUp = up;
	static float PreviousBeta = beta;
	static float PreviousAlpha = alpha;
	static float PreviousFov = fov;

	if(TheCamera.m_bJust_Switched){
		PreviousSource = source;
		PreviousTarget = target;
		PreviousUp = up;
	}

	m_cvecSourceSpeedOverOneFrame = source - PreviousSource;
	m_cvecTargetSpeedOverOneFrame = target - PreviousTarget;
	m_cvecUpOverOneFrame = up - PreviousUp;
	m_fFovSpeedOverOneFrame = fov - PreviousFov;
	m_fBetaSpeedOverOneFrame = beta - PreviousBeta;
	MakeAngleLessThan180(m_fBetaSpeedOverOneFrame);
	m_fAlphaSpeedOverOneFrame = alpha - PreviousAlpha;
	MakeAngleLessThan180(m_fAlphaSpeedOverOneFrame);

	PreviousSource = source;
	PreviousTarget = target;
	PreviousUp = up;
	PreviousBeta = beta;
	PreviousAlpha = alpha;
	PreviousFov = fov;
}

bool
CCam::Using3rdPersonMouseCam(void) 
{
	return CCamera::m_bUseMouse3rdPerson && Mode == MODE_FOLLOWPED;
}

bool
CCam::GetWeaponFirstPersonOn(void)
{
	return CamTargetEntity && CamTargetEntity->IsPed() && ((CPed*)CamTargetEntity)->GetWeapon()->m_bAddRotOffset;
}

bool
CCam::IsTargetInWater(const CVector &CamCoors)
{
	if(CamTargetEntity){
		// VICEEXT (12ª partida, 22/09): NADAR NO ES "CAER AL AGUA".
		//
		// Este predicado sólo decide si el motor pide `MODE_PLAYER_FALLEN_WATER`
		// (Camera.cpp: "Fallen into water"), que es la cámara del jugador que se
		// hunde: se queda clavada en `m_vecLastAboveWaterCamPosition` (la última
		// posición de cámara por encima del agua) mirando hacia abajo. En el mod
		// nadar es un estado normal: el ped flota CON la cabeza fuera, así que
		// aquí nunca entraba... pero nuestro nado (VICEEXT_SWIMMING) lleva al ped
		// unos centímetros por DEBAJO de la superficie (`pos.z < WaterZ`), o sea
		// que cumplía la condición y la cámara saltaba al modo de ahogado.
		//
		// Medido en el log de las partidas del 22/09 (nado a z=5.4 con nivel 6.1):
		// `SWIM2 ... camz=12.02` constante mientras el ped avanzaba 1.3 m/s y el
		// vídeo del navegador mostrando la calzada desde arriba (el ped fuera de
		// cuadro). Es exactamente lo que el jugador describe como "la cámara se
		// queda fija y Tommy cae al vacío".
		//
		// Con el nado activo se devuelve false: la cámara sigue siendo la de
		// seguir-al-ped (que ya lleva el objetivo a la superficie en
		// Process_FollowPed / Process_FollowPedWithMouse).
#ifdef VICEEXT_SWIMMING
		// R17 (14ª partida, 22/09): el JUGADOR VIVO EN EL AGUA nunca usa esta
		// cámara. Antes sólo se excluía con el nado activo (`ViceExtIsSwimming`),
		// y eso dejaba dos agujeros por los que la cámara se quedaba fija:
		//   - el nado se suelta al llegar a agua poco honda (`motivo=poco-hondo`)
		//     con el ped TODAVÍA por debajo de la superficie -> el predicado de
		//     serie (`bIsInWater && pos.z < WaterZ`) volvía a dar verdadero;
		//   - justo al entrar al agua, antes de que el ped esté "nadando".
		// Y el modo que pide es `MODE_PLAYER_FALLEN_WATER`, que fija la cámara 4 m
		// sobre `m_vecLastAboveWaterCamPosition` y la deja CLAVADA mirando hacia
		// abajo (Process_Player_Fallen_Water). En el vídeo del navegador de la
		// sesión de las 14:38 se ve literal: la cámara sobre el agua, el ped
		// nadando fuera de cuadro y `camz=12.79` constante (= última posición de
		// cámara + 4) mientras avanzaba a 1,3 m/s. Eso es lo que el jugador
		// describía como "la cámara se mantiene fija".
		// El mod hace nadable al jugador (no se ahoga), así que este predicado
		// sólo tiene sentido para él si de verdad se está ahogando o ha muerto.
		if(CamTargetEntity->IsPed() && ((CPed*)CamTargetEntity)->IsPlayer()){
			CPed *odPed = (CPed*)CamTargetEntity;
			// R18 (15ª partida, 22/09): la condición NO puede mirar `bIsDrowning`.
			//
			// `CPed::ProcessBuoyancy` marca `bIsDrowning = true` en cuanto el ped
			// queda MÁS DE 0,6 m bajo la superficie
			// (`mod_Buoyancy.m_waterlevel > GetPosition().z + 0.6f`), y con el
			// mod el jugador nada flotando a ~0,5-0,9 m del nivel del agua: o sea
			// que nadando normal el motor lo tiene por ahogado a ratos. Con esa
			// condición, la cámara volvía al modo de ahogado
			// (`MODE_PLAYER_FALLEN_WATER`) justo al entrar al agua —cuando el ped
			// aún viene hundido de la caída— y cada vez que una ola lo bajaba de
			// los 0,6 m, y se quedaba clavada 4 m sobre el agua mirando hacia
			// abajo mientras el ped se alejaba (`camdist` hasta 11,6 m).
			//
			// Medido en la sonda `tools/crouch-swim-smoke-test.mjs` (build ve30)
			// con capturas: al entrar al agua `modo=23` con `camz=10,91` fijo y
			// `camdist` creciendo (6,86 → 9,22 → 11,59 m); al subir el ped a la
			// superficie volvía a `modo=4`. El mod hace nadable al jugador
			// (`bDrownsInWater = false`, no muere ahogado), así que la cámara de
			// ahogado sólo tiene sentido si de verdad se muere: se excluye por
			// "no está muriendo" y no por "no está ahogándose" (`ahogando=` en la
			// traza dice si el motor lo tenía por ahogado en ese momento).
			if(!odPed->DyingOrDead()){
				m_vecLastAboveWaterCamPosition = Source;
#ifdef __EMSCRIPTEN__
				// Una vez por sesión: deja constancia en el log de que la cámara con
				// el jugador vivo en el agua es la de seguir-al-ped (modo 4) y no la
				// de ahogado (modo 23, clavada 4 m sobre el agua).
				static bool odWarnedFallen = false;
				if (!odWarnedFallen) {
					odWarnedFallen = true;
					char t[110];
					snprintf(t, sizeof t, "SWIMCAM no-fallen-water jugador-vivo-en-agua ahogando=%d modo=seguir-al-ped",
						(int)odPed->bIsDrowning);
					ODTRACES(t);
				}
#endif
				return false;
			}
		}
#endif
		float WaterZ = -6000.0f;
		CWaterLevel::GetWaterLevel(CamTargetEntity->GetPosition(), &WaterZ, false);
		if(CamTargetEntity->IsPed()){
			if(((CPed*)CamTargetEntity)->bIsDrowning ||
			   ((CPed*)CamTargetEntity)->bIsInWater && CamTargetEntity->GetPosition().z < WaterZ)
				return true;
		}else{
			assert(CamTargetEntity->IsVehicle());
			if(((CVehicle*)CamTargetEntity)->bIsDrowning ||
			   ((CVehicle*)CamTargetEntity)->bIsInWater && CamTargetEntity->GetPosition().z < WaterZ)
				return true;
		}
	}
	m_vecLastAboveWaterCamPosition = Source;
	return false;
}

void
CCam::PrintMode(void)
{
	// Doesn't do anything
	char buf[256];

	if(PrintDebugCode){
		sprintf(buf, "                                                   ");
		sprintf(buf, "                                                   ");
		sprintf(buf, "                                                   ");

		static Const char *modes[] = { "None",
			"Top Down", "GTA Classic", "Behind Car", "Follow Ped",
			"Aiming", "Debug", "Sniper", "Rocket", "Model Viewer", "Bill",
			"Syphon", "Circle", "Cheesy Zoom", "Wheel", "Fixed",
			"1st Person", "Fly by", "on a String", "Reaction",
			"Follow Ped with Bind", "Chris", "Behind Boat",
			"Player fallen in Water", "Train Roof", "Train Side",
			"Blood on the tracks", "Passenger", "Syphon Crim in Front",
			"Dead Baby", "Pillow Paps", "Look at Cars", "Arrest One",
			"Arrest Two", "M16", "Special fixed for Syphon", "Fight",
			"Top Down Ped", "Lighthouse",
			"Sniper run about", "Rocket run about",
			"1st Person run about", "M16 run about", "Fight run about",
			"Editor", "Helicannon", "Camera"
		};
		sprintf(buf, "Cam: %s", modes[TheCamera.Cams[TheCamera.ActiveCam].Mode]);
		CDebug::PrintAt(buf, 2, 5);
	}

	if(DebugCamMode != MODE_NONE){
		switch(Mode){
		case MODE_FOLLOWPED:
			sprintf(buf, "Debug:- Cam Choice1. No Locking, used as game default");
			break;
		case MODE_REACTION:
			sprintf(buf, "Debug:- Cam Choice2. Reaction Cam On A String ");
			sprintf(buf, "        Uses Locking Button LeftShoulder 1. ");	// lie
			break;
		case MODE_FOLLOW_PED_WITH_BIND:
			sprintf(buf, "Debug:- Cam Choice3. Game ReactionCam with Locking ");
			sprintf(buf, "        Uses Locking Button LeftShoulder 1. ");
			break;
		case MODE_CHRIS:
			sprintf(buf, "Debug:- Cam Choice4. Chris's idea.  ");
			sprintf(buf, "        Uses Locking Button LeftShoulder 1. ");
			sprintf(buf, "        Also control the camera using the right analogue stick.");
			break;
		}
	}
}

// This code is really bad. wtf R*?
CVector
CCam::DoAverageOnVector(const CVector &vec)
{
	int i;
	CVector Average = CVector(0.0f, 0.0f, 0.0f);

	if(ResetStatics){
		m_iRunningVectorArrayPos = 0;
		m_iRunningVectorCounter = 1;
	}

	// TODO: make this work with NUMBER_OF_VECTORS_FOR_AVERAGE != 2
	if(m_iRunningVectorCounter == 3){
		m_arrPreviousVectors[0] = m_arrPreviousVectors[1];
		m_arrPreviousVectors[1] = vec;
	}else
		m_arrPreviousVectors[m_iRunningVectorArrayPos] = vec;

	for(i = 0; i <= m_iRunningVectorArrayPos; i++)
		Average += m_arrPreviousVectors[i];
	Average /= i;

	m_iRunningVectorArrayPos++;
	m_iRunningVectorCounter++;
	if(m_iRunningVectorArrayPos >= NUMBER_OF_VECTORS_FOR_AVERAGE)
		m_iRunningVectorArrayPos = NUMBER_OF_VECTORS_FOR_AVERAGE-1;
	if(m_iRunningVectorCounter > NUMBER_OF_VECTORS_FOR_AVERAGE+1)
		m_iRunningVectorCounter = NUMBER_OF_VECTORS_FOR_AVERAGE+1;

	return Average;
}

float DefaultAcceleration = 0.045f;
float DefaultMaxStep = 0.15f;
float fDefaultSpeedStep = 0.025f;
float fDefaultSpeedMultiplier = 0.09f;
float fDefaultSpeedLimit = 0.15f;
float fDefaultSpeedStep4Avoid = 0.02f;
float fDefaultSpeedMultiplier4Avoid = 0.05f;
float fDefaultSpeedLimit4Avoid = 0.25f;
float fAvoidGeomThreshhold = 1.5f;
float fMiniGunBetaOffset = 0.3f;

void
CCam::Process_FollowPed(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	if(!CamTargetEntity->IsPed())
		return;
#ifdef VICEEXT_RECOIL
	bool recoilReset = ResetStatics;
#endif

	CVector TargetCoors, Dist, IdealSource;
	float Length = 0.0f;
	static bool PickedASide;
	static float FixedTargetOrientation = 0.0f;
	float AngleToGoTo = 0.0f;
	bool StandingInTrain = false;
	float ZoomGroundTarget = 0.0f;
	float ZoomZTarget = 0.0f;
	static int TimeIndicatedWantedToGoDown = 0;
	static bool StartedCountingForGoDown = false;
	static float ZoomGround = 0.0f;
	static float ZoomGroundSpeed = 0.0f;
	static float ZoomZ = 0.0f;
	static float ZoomZSpeed = 0.0f;
	float DeltaBeta;

	m_bFixingBeta = false;
	bBelowMinDist = false;
	bBehindPlayerDesired = false;

	FOV = DefaultFOV;

	if(ResetStatics){
		Rotating = false;
		m_bCollisionChecksOn = true;
		FixedTargetOrientation = 0.0f;
		PickedASide = false;
		StartedCountingForGoDown = false;
		AngleToGoTo = 0.0f;
		ZoomGround = 0.0f;
		ZoomGroundSpeed = 0.0f;
		ZoomZ = 0.0f;
		ZoomZSpeed = 0.0f;
		Distance = 500.0f;
	}


	TargetCoors = CameraTarget;
	// B4: hombro al apuntar (spec ClassicAXIS; igual que en los otros follows a pie).
	TargetCoors += CamTargetEntity->GetRight() * ViceExtAimShoulderSmoothed(CamTargetEntity, ResetStatics);

	// Take speed of thing we're standing on into account
	CVector GroundMovement(0.0f, 0.0f, 0.0f);
	CPhysical *ground = (CPhysical*)((CPed*)CamTargetEntity)->m_pCurSurface;
	if(ground && (ground->IsVehicle() || ground->IsObject()))
		GroundMovement += ground->GetSpeed(CamTargetEntity->GetPosition() - ground->GetPosition()) * CTimer::GetTimeStep();

	Source += GroundMovement;
	IdealSource = Source;
	TargetCoors.z += m_fSyphonModeTargetZOffSet;

	TargetCoors.z = DoAverageOnVector(TargetCoors).z;

	Dist.x = IdealSource.x - TargetCoors.x;
	Dist.y = IdealSource.y - TargetCoors.y;
	Length = Dist.Magnitude2D();

	// Cam on a string. With a fixed distance. Zoom in/out is done later.
	if(Length != 0.0f){
		IdealSource = TargetCoors + CVector(Dist.x, Dist.y, 0.0f)/Length * m_fMinRealGroundDist;
		IdealSource.z += GroundMovement.z;
	}else
		IdealSource = TargetCoors + CVector(1.0f, 1.0f, 0.0f);

	if(TheCamera.m_bUseTransitionBeta && ResetStatics){
		CVector VecDistance;
		IdealSource.x = TargetCoors.x + m_fMinRealGroundDist*Cos(m_fTransitionBeta);
		IdealSource.y = TargetCoors.y + m_fMinRealGroundDist*Sin(m_fTransitionBeta);
		Beta = CGeneral::GetATanOfXY(IdealSource.x - TargetCoors.x, IdealSource.y - TargetCoors.y);
	}else
		Beta = CGeneral::GetATanOfXY(Source.x - TargetCoors.x, Source.y - TargetCoors.y);

	if(TheCamera.m_bCamDirectlyBehind){
		 m_bCollisionChecksOn = true;
		 Beta = TargetOrientation + PI;
	}

	if(FindPlayerVehicle())
		if(FindPlayerVehicle()->m_vehType == VEHICLE_TYPE_TRAIN)
			StandingInTrain = true;

	if(TheCamera.m_bCamDirectlyInFront){
		 m_bCollisionChecksOn = true;
		 Beta = TargetOrientation;
	}

	while(Beta >= PI) Beta -= 2.0f * PI;
	while(Beta < -PI) Beta += 2.0f * PI;

	if(TheCamera.PedZoomIndicator == CAM_ZOOM_1 &&
	   ((CPed*)CamTargetEntity)->GetPedState() != PED_ENTER_CAR &&
	   ((CPed*)CamTargetEntity)->GetPedState() != PED_CARJACK){
		ZoomGroundTarget = m_fTargetZoomGroundOne;
		ZoomZTarget = m_fTargetZoomOneZExtra;
	}else if(TheCamera.PedZoomIndicator == CAM_ZOOM_2 || TheCamera.PedZoomIndicator == CAM_ZOOM_1){
		ZoomGroundTarget = m_fTargetZoomGroundTwo;
		ZoomZTarget = m_fTargetZoomTwoZExtra;
	}else if(TheCamera.PedZoomIndicator == CAM_ZOOM_3){
		ZoomGroundTarget = m_fTargetZoomGroundThree;
		ZoomZTarget = m_fTargetZoomThreeZExtra;
	}
	if(m_fCloseInPedHeightOffset >  0.00001f){
		ZoomGroundTarget = m_fTargetCloseInDist;
		ZoomZTarget = m_fTargetZoomZCloseIn;
	}
	if(ResetStatics){
		ZoomGround = ZoomGroundTarget;
		ZoomZ = ZoomZTarget;
	}

	float SpeedStep = fDefaultSpeedStep;
	float SpeedMultiplier = fDefaultSpeedMultiplier;
	float SpeedLimit = fDefaultSpeedLimit;
	bool Shooting = false;
	CPlayerPed *ped = (CPlayerPed*)CamTargetEntity;
	if(ped->GetWeapon()->m_eWeaponType != WEAPONTYPE_UNARMED)
		if(CPad::GetPad(0)->GetWeapon())
			Shooting = true;
	if(ped->GetWeapon()->m_eWeaponType == WEAPONTYPE_DETONATOR ||
	   ped->GetWeapon()->m_eWeaponType == WEAPONTYPE_BASEBALLBAT)
		Shooting = false;


	// Figure out if and where we want to rotate

	if(CPad::GetPad(0)->ForceCameraBehindPlayer() && !CPickups::PlayerOnWeaponPickup || Shooting){

		// Center cam behind player

		if(PickedASide){
			if(AngleToGoTo == 0.0f){
				FixedTargetOrientation = TargetOrientation + PI;
				if(Shooting && ped->GetWeapon()->m_eWeaponType == WEAPONTYPE_MINIGUN)
					FixedTargetOrientation -= fMiniGunBetaOffset;
			}
			Rotating = true;
		}else{
			FixedTargetOrientation = TargetOrientation + PI;
			Rotating = true;
			PickedASide = true;
			if(Shooting && ped->GetWeapon()->m_eWeaponType == WEAPONTYPE_MINIGUN)
				FixedTargetOrientation -= fMiniGunBetaOffset;
		}
	}else if(Abs(TheCamera.m_fAvoidTheGeometryProbsTimer) > fAvoidGeomThreshhold && !Rotating ){

		if(TheCamera.m_fAvoidTheGeometryProbsTimer < 0.0f)
			FixedTargetOrientation = TargetOrientation;
		else
			FixedTargetOrientation = TargetOrientation + PI;
		float dist = (Source - TargetCoors).Magnitude();
		float mult = dist > 0.1f ? 1.0f/dist : 10.0f;
		SpeedStep = mult * fDefaultSpeedStep4Avoid;
		SpeedMultiplier = mult * fDefaultSpeedMultiplier4Avoid;
		SpeedLimit = mult * fDefaultSpeedLimit4Avoid;
	}

	int MoveState = ((CPed*)CamTargetEntity)->m_nMoveState;
	if(MoveState != PEDMOVE_NONE && MoveState != PEDMOVE_STILL &&
	   !(CPad::GetPad(0)->ForceCameraBehindPlayer() && !CPickups::PlayerOnWeaponPickup) && !Shooting){
		Rotating = false;
		if(TheCamera.m_fAvoidTheGeometryProbsTimer <= fAvoidGeomThreshhold)
			BetaSpeed = 0.0f;
	}

	// Now do the Beta rotation

	float RotDistance = m_fMinRealGroundDist;

	if(Rotating || TheCamera.m_fAvoidTheGeometryProbsTimer > fAvoidGeomThreshhold){
		m_bFixingBeta = true;

		while(FixedTargetOrientation >= PI) FixedTargetOrientation -= 2*PI;
		while(FixedTargetOrientation < -PI) FixedTargetOrientation += 2*PI;

		while(Beta >= PI) Beta -= 2*PI;
		while(Beta < -PI) Beta += 2*PI;


		// This is inlined WellBufferMe - unfortunately modified so we can't just call it
		{
		DeltaBeta = FixedTargetOrientation - Beta;
		while(DeltaBeta >= PI) DeltaBeta -= 2*PI;
		while(DeltaBeta < -PI) DeltaBeta += 2*PI;

		// this is the added bit
		if(!Rotating){
			if(TheCamera.m_nAvoidTheGeometryProbsDirn == -1 && DeltaBeta > 0.0f ||
			   TheCamera.m_nAvoidTheGeometryProbsDirn == 1 && DeltaBeta < 0.0f)
				DeltaBeta *= -1.0f;
		}

		float ReqSpeed = DeltaBeta * SpeedMultiplier;
		// this is also added
		ReqSpeed = Clamp(ReqSpeed, -SpeedLimit, SpeedLimit);

		// Add or subtract absolute depending on sign, genius!
		if(ReqSpeed - BetaSpeed > 0.0f)
			BetaSpeed += SpeedStep * Abs(ReqSpeed - BetaSpeed) * CTimer::GetTimeStep();
		else
			BetaSpeed -= SpeedStep * Abs(ReqSpeed - BetaSpeed) * CTimer::GetTimeStep();
		// this would be simpler:
		// BetaSpeed += SpeedStep * (ReqSpeed - BetaSpeed) * CTimer::ms_fTimeStep;

		if(ReqSpeed < 0.0f && BetaSpeed < ReqSpeed)
			BetaSpeed = ReqSpeed;
		else if(ReqSpeed > 0.0f && BetaSpeed > ReqSpeed)
			BetaSpeed = ReqSpeed;

		Beta += BetaSpeed * Min(10.0f, CTimer::GetTimeStep());
		}

		if(ResetStatics){
			Beta = FixedTargetOrientation;
			BetaSpeed = 0.0f;
		}

		Source.x = TargetCoors.x + RotDistance * Cos(Beta);
		Source.y = TargetCoors.y + RotDistance * Sin(Beta);

		// Check if we can stop rotating
		DeltaBeta = FixedTargetOrientation - Beta;
		while(DeltaBeta >= PI) DeltaBeta -= 2*PI;
		while(DeltaBeta < -PI) DeltaBeta += 2*PI;
		if(Abs(DeltaBeta) < DEGTORAD(1.0f) && !bBehindPlayerDesired){
			// Stop rotation
			PickedASide = false;
			Rotating = false;
			BetaSpeed = 0.0f;
		}
	}


	if(TheCamera.m_bCamDirectlyBehind || TheCamera.m_bCamDirectlyInFront ||
	   StandingInTrain || Rotating ||
	   TheCamera.m_bUseTransitionBeta && ResetStatics ||
	   Abs(TheCamera.m_fAvoidTheGeometryProbsTimer) > fAvoidGeomThreshhold){
		if(TheCamera.m_bUseTransitionBeta){
			Beta = m_fTransitionBeta;
			Source.x = TargetCoors.x + RotDistance * Cos(m_fTransitionBeta);
			Source.y = TargetCoors.y + RotDistance * Sin(m_fTransitionBeta);
		}
		if(TheCamera.m_bCamDirectlyBehind){
			Beta = TargetOrientation + PI;
			Source.x = TargetCoors.x + RotDistance * Cos(Beta);
			Source.y = TargetCoors.y + RotDistance * Sin(Beta);
		}
		if(TheCamera.m_bCamDirectlyInFront){
			Beta = TargetOrientation;
			Source.x = TargetCoors.x + RotDistance * Cos(Beta);
			Source.y = TargetCoors.y + RotDistance * Sin(Beta);
		}
		if(StandingInTrain){
			Beta = TargetOrientation + PI;
			Source.x = TargetCoors.x + RotDistance * Cos(Beta);
			Source.y = TargetCoors.y + RotDistance * Sin(Beta);
			m_fDimensionOfHighestNearCar = 0.0f;
			m_fCamBufferedHeight = 0.0f;
			m_fCamBufferedHeightSpeed = 0.0f;
		}
		if(StandingInTrain){
			Beta = TargetOrientation + PI;
			Source.x = TargetCoors.x + RotDistance * Cos(Beta);
			Source.y = TargetCoors.y + RotDistance * Sin(Beta);
			m_fDimensionOfHighestNearCar = 0.0f;
			m_fCamBufferedHeight = 0.0f;
			m_fCamBufferedHeightSpeed = 0.0f;
		}

		// Beta and Source already set in the rotation code
	}else{
		Source = IdealSource;
		BetaSpeed = 0.0f;
	}
	Source.z = IdealSource.z;

	// Zoom out camera
	Front = TargetCoors - Source;
	Front.Normalise();
	WellBufferMe(ZoomGroundTarget, &ZoomGround, &ZoomGroundSpeed, 0.2f, 0.07f, false);
	WellBufferMe(ZoomZTarget, &ZoomZ, &ZoomZSpeed, 0.2f, 0.07f, false);
	Source.x -= Front.x*ZoomGround;
	Source.y -= Front.y*ZoomGround;
	Source.z += ZoomZ;


	// Process height offset to avoid peds and cars

	float TargetZOffSet = Max(m_fDimensionOfHighestNearCar, m_fPedBetweenCameraHeightOffset);
	float TargetHeight = CameraTarget.z + TargetZOffSet - Source.z;

	if(TargetHeight > m_fCamBufferedHeight){
		// Have to go up
		if(TargetZOffSet == m_fPedBetweenCameraHeightOffset && TargetZOffSet > m_fCamBufferedHeight)
			WellBufferMe(TargetHeight, &m_fCamBufferedHeight, &m_fCamBufferedHeightSpeed, 0.2f, 0.04f, false);
		else
			WellBufferMe(TargetHeight, &m_fCamBufferedHeight, &m_fCamBufferedHeightSpeed, 0.2f, 0.025f, false);
		StartedCountingForGoDown = false;
	}else{
		// Have to go down
		if(StartedCountingForGoDown){
			if(CTimer::GetTimeInMilliseconds() != TimeIndicatedWantedToGoDown){
				if(TargetHeight > 0.0f)
					WellBufferMe(TargetHeight, &m_fCamBufferedHeight, &m_fCamBufferedHeightSpeed, 0.2f, 0.01f, false);
				else
					WellBufferMe(0.0f, &m_fCamBufferedHeight, &m_fCamBufferedHeightSpeed, 0.2f, 0.01f, false);
			}
		}else{
			StartedCountingForGoDown = true;
			TimeIndicatedWantedToGoDown = CTimer::GetTimeInMilliseconds();
		}
	}

	Source.z += m_fCamBufferedHeight;
	TargetCoors.z += Min(1.0f, m_fCamBufferedHeight/2.0f);
	m_cvecTargetCoorsForFudgeInter = TargetCoors;

	CVector OrigSource = Source;
	TheCamera.AvoidTheGeometry(OrigSource, TargetCoors, Source, FOV);
	float TargetDist = (TargetCoors - Source).Magnitude();
	if(TargetDist < Distance)
		Distance = TargetDist;
	else{
		float f = Pow(0.97f, CTimer::GetTimeStep());
		Distance = (1.0f - f)*TargetDist + f*Distance;
		if(TargetDist > 0.05f)
			Source = TargetCoors + (Source-TargetCoors)*Distance/TargetDist;
		float clip = Distance-fRangePlayerRadius;
		if(clip < RwCameraGetNearClipPlane(Scene.camera))
			RwCameraSetNearClipPlane(Scene.camera, Max(clip, fCloseNearClipLimit));
	}

	Front = TargetCoors - Source;
	m_fRealGroundDist = Front.Magnitude2D();
	m_fMinDistAwayFromCamWhenInterPolating = m_fRealGroundDist;	
	Front.Normalise();
	GetVectorsReadyForRW();
	TheCamera.m_bCamDirectlyBehind = false;
	TheCamera.m_bCamDirectlyInFront = false;
#ifdef VICEEXT_RECOIL
	// FollowPed no mueve Alpha este frame, pero drena la cola para que un
	// disparo en este modo no quede retenido hasta el próximo modo con Apply.
	CWeapon::ViceExtRecoilBegin(Alpha, recoilReset, Mode, "follow-ped-passive");
	CWeapon::ViceExtRecoilApply(Alpha, 0.0f, 0.0f, "none", Mode, -DEGTORAD(89.5f), DEGTORAD(60.0f));
#endif

	ResetStatics = false;
}

float fBaseDist = 1.7f;
float fAngleDist = 2.0f;
float fFalloff = 3.0f;
float fStickSens = 0.01f;
float fTweakFOV = 1.1f;
float fTranslateCamUp = 0.8f;
int16 nFadeControlThreshhold = 45;
float fDefaultAlphaOrient = -0.22f;
float fMouseAvoidGeomReturnRate = 0.92f;

void
CCam::Process_FollowPedWithMouse(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	FOV = DefaultFOV;
#ifdef VICEEXT_RECOIL
	bool recoilReset = ResetStatics;
#endif

	if(!CamTargetEntity->IsPed())
		return;

	CVector TargetCoors;
	float CamDist;
	CColPoint colPoint;
	CEntity *entity;

	if(ResetStatics){
		Rotating = false;
		m_bCollisionChecksOn = true;
		CPad::GetPad(0)->ClearMouseHistory();
		ResetStatics = false;
	}

	bool OnTrain = FindPlayerVehicle() && FindPlayerVehicle()->IsTrain();

	TargetCoors = CameraTarget;
	TargetCoors.z += fTranslateCamUp;
#ifdef VICEEXT_CROUCH
	// R6: agachado, el objetivo de la cámara baja (antes apuntaba a la cabeza
	// "de pie" y la cámara parecía fija).
	// H1 (7ª partida, t=12: cámara dentro de la cabeza con −0,45): la caída
	// real de la cabeza agachada es ~0,55.
	//
	// R14 (12ª partida, 22/09): el verificador midió el resultado y no llegaba
	// (`H FALLO: la camara no baja agachado (11.41 vs 11.68)`): con −0,55 el
	// descenso real era de 0,27 m, porque al bajar el objetivo el ángulo de la
	// cámara cambia y ésta se separa del ped, subiendo de nuevo. Se dobla el
	// offset (−0,95) para que el descenso medido supere el umbral de 0,4 m del
	// bloque H; la medida exacta la vuelve a dar `CROUCH2 camz=`.
	if (CamTargetEntity && CamTargetEntity->IsPed() && ((CPed*)CamTargetEntity)->IsPlayer())
	{
		float odCrouchBlend = CPlayerPed::ViceExtCrouchBlend();
		if (odCrouchBlend > 0.0f)
			TargetCoors.z -= VICEEXT_CROUCH_CAM_DROP * odCrouchBlend;
	}
#endif
#ifdef VICEEXT_SWIMMING
	// H2 (7ª partida, t=176: cámara clavada en el plano del agua): nadando, el
	// objetivo va a la superficie (no al ped sumergido), para que la cámara lo
	// siga detrás por encima del agua en vez de mirar desde dentro.
	if (CamTargetEntity && CamTargetEntity->IsPed() && ((CPed*)CamTargetEntity)->IsPlayer()
	    && CPlayerPed::ViceExtIsSwimming()) {
		float odWl = 0.0f;
		if (CWaterLevel::GetWaterLevel(CamTargetEntity->GetPosition(), &odWl, true)
	    	&& TargetCoors.z < odWl + 0.5f)
			TargetCoors.z = odWl + 0.5f;
#ifdef __EMSCRIPTEN__
		// R14: una línea por segundo mientras se nada. Junto con `SWIM2 camz`
		// cierra el diagnóstico de la cámara de nado: dice que el objetivo está en
		// la superficie (nivel+0,5) y dónde está la cámara.
		{
			static uint32 s_odNextSwim = 0;
			uint32 odNow = CTimer::GetTimeInMilliseconds();
			if (odNow < s_odNextSwim && odNow + 60000 >= s_odNextSwim) {}
			else {
				s_odNextSwim = odNow + 1000;
				char t[160];
				snprintf(t, sizeof t, "SWIMCAM objetivo=%.2f nivel=%.2f cam=%.2f modo=%d",
					TargetCoors.z, odWl, Source.z, (int)Mode);
				ODTRACES(t);
			}
		}
#endif
	}
#endif

	// B4 (apuntado, spec ClassicAXIS aimOffset en espacio objeto = derecha del
	// ped en mundo): al apuntar, el objetivo se desplaza al hombro para que la
	// mira 3a (CrosshairMult 0.53/0.4) no quede tras la cabeza. El disparo sigue
	// yendo a la cruceta por construcción (Find3rdPersonCamTargetVector).
	TargetCoors += CamTargetEntity->GetRight() * ViceExtAimShoulderSmoothed(CamTargetEntity, ResetStatics);

	float AlphaOffset, BetaOffset;
	bool UseMouse = false;
	float LookUpDown = 0.0f;
	if(CPad::GetPad(0)->IsPlayerControlsDisabledBy(PLAYERCONTROL_PLAYERINFO)){
		CVector ToCam = Source - TargetCoors;
		ToCam.Normalise();
		if(ToCam.z < -0.9f)
			BetaOffset = TargetOrientation + PI;
		else
			BetaOffset = Atan2(ToCam.y, ToCam.x);
		BetaOffset -= Beta;
		AlphaOffset = 0.0f;
	}else{
		// Look around
		float MouseX = CPad::GetPad(0)->GetMouseX();
		float MouseY = CPad::GetPad(0)->GetMouseY();
		float LookLeftRight;
		if((MouseX != 0.0f || MouseY != 0.0f) && !CPad::GetPad(0)->ArePlayerControlsDisabled()){
			UseMouse = true;
			LookLeftRight = -2.5f*MouseX;
			LookUpDown = 4.0f*MouseY;
		}else{
			LookLeftRight = -CPad::GetPad(0)->LookAroundLeftRight();
			LookUpDown = CPad::GetPad(0)->LookAroundUpDown();
		}
		if(UseMouse){
			BetaOffset = LookLeftRight * TheCamera.m_fMouseAccelHorzntl * FOV/80.0f;
			AlphaOffset = LookUpDown * TheCamera.m_fMouseAccelVertical * FOV/80.0f;
		}else{
			// B4: base ClassicAXIS del follow (B8: 0.01*(1/20) y 0.01*(0.6/20);
			// fStickSens ya es el 0.01; la deadzone por eje la pone LookAround*).
			BetaOffset = LookLeftRight * fStickSens * (1.0f/20.0f) * FOV/80.0f * CTimer::GetTimeStep();
			AlphaOffset = LookUpDown * fStickSens * (0.6f/20.0f) * FOV/80.0f * CTimer::GetTimeStep();
		}
	}

#ifdef VICEEXT_RECOIL
	float recoilManualDeltaRad = AlphaOffset;
#endif
	if(TheCamera.GetFading() && TheCamera.GetFadingDirection() == FADE_IN && nFadeControlThreshhold < CDraw::FadeValue ||
	   CDraw::FadeValue > 200 ||
	   CPad::GetPad(0)->IsPlayerControlsDisabledBy(PLAYERCONTROL_PLAYERINFO)){
		if(Alpha < fDefaultAlphaOrient-0.05f)
			AlphaOffset = 0.05f;
		else if(Alpha < fDefaultAlphaOrient)
			AlphaOffset = fDefaultAlphaOrient - Alpha;
		else if(Alpha > fDefaultAlphaOrient+0.05f)
			AlphaOffset = -0.05f;
		else if(Alpha > fDefaultAlphaOrient)
			AlphaOffset = fDefaultAlphaOrient - Alpha;
		else
			AlphaOffset = 0.0f;
	}

	Alpha += AlphaOffset;
	Beta += BetaOffset;
	while(Beta >= PI) Beta -= 2*PI;
	while(Beta < -PI) Beta += 2*PI;
#ifdef VICEEXT_RECOIL
	// Extrae el residual ya aplicado: input manual y recoil se integran una vez.
	CWeapon::ViceExtRecoilBegin(Alpha, recoilReset, Mode, "follow-mouse");
	CWeapon::ViceExtRecoilApply(Alpha, recoilManualDeltaRad, LookUpDown, UseMouse ? "mouse" : "pad", Mode,
		ViceExtAimingOverShoulder(CamTargetEntity) ? -DEGTORAD(50.0f) : -DEGTORAD(89.5f),
		ViceExtAimingOverShoulder(CamTargetEntity) ? DEGTORAD(50.0f) : DEGTORAD(60.0f));
#endif
	// BARRIDO 1 (spec CamNew, B8): barrido el +45 de serie. Follow +60/-89.5
	// (Process_FollowPed :168-171); apuntando (hombro activo) +-50
	// (Process_AimWeapon :350-353). El recoil (arriba) no se toca.
	if(ViceExtAimingOverShoulder(CamTargetEntity)){
		if(Alpha > DEGTORAD(50.0f)) Alpha = DEGTORAD(50.0f);
		else if(Alpha < -DEGTORAD(50.0f)) Alpha = -DEGTORAD(50.0f);
	}else{
		if(Alpha > DEGTORAD(60.0f)) Alpha = DEGTORAD(60.0f);
		else if(Alpha < -DEGTORAD(89.5f)) Alpha = -DEGTORAD(89.5f);
	}

	// SA code
#ifdef FREE_CAM
	if((CCamera::bFreeCam && Alpha > 0.0f) || (!CCamera::bFreeCam && Alpha > fBaseDist))
#else
	if(Alpha > fBaseDist)	// comparing an angle against a distance?
#endif
		CamDist = fBaseDist + Cos(Min(Alpha*fFalloff, HALFPI))*fAngleDist;
	else
		CamDist = fBaseDist + Cos(Alpha)*fAngleDist;

	if(TheCamera.m_bUseTransitionBeta)
		Beta = m_fTransitionBeta;

	if(TheCamera.m_bCamDirectlyBehind)
		Beta = TheCamera.m_PedOrientForBehindOrInFront + PI;
	if(TheCamera.m_bCamDirectlyInFront)
		Beta = TheCamera.m_PedOrientForBehindOrInFront;
	if(OnTrain)
		Beta = TargetOrientation;

#ifdef VICEEXT_AIM_CLASSICAXIS
	if (CCamera::s_viceExtAimViewPending && CCamera::s_viceExtAimViewMode == Mode) {
		CVector odV = CCamera::s_viceExtAimViewDir;
		float odH = Sqrt(odV.x * odV.x + odV.y * odV.y);
		Beta = Atan2(-odV.y, -odV.x);
		Alpha = Atan2(odV.z, odH);
		if (Alpha > DEGTORAD(60.0f)) Alpha = DEGTORAD(60.0f);
		else if (Alpha < -DEGTORAD(89.5f)) Alpha = -DEGTORAD(89.5f);
		CCamera::s_viceExtAimViewPending = false;
	}
#endif
	Front.x = Cos(Alpha) * -Cos(Beta);
	Front.y = Cos(Alpha) * -Sin(Beta);
	Front.z = Sin(Alpha);
	// PORTADO — ClassicAXIS (MIT, © 2022 Classic Axis VC Team)
	//   gta_vc_browser/tmp/extsrc/CamNew.cpp:82-86 y :215-220
	//   (ley de a PIE: «Process_CrouchOffset(duckOffset)» + el clamp de agua)
	// Qué se toma, las dos cosas que el mod hace en la ley de a pie y que en este
	//   motor solo estaban en la ley de APUNTADO (o no estaban):
	//   (a) CamNew.cpp:82-86 — `duckOffset` (estado de la camara, CamNew.cpp:45) se
	//       interpola con la MISMA función que la del apuntado y se SUMA a la z del
	//       objetivo: `targetCoords.z += duckOffset`. Al agacharse el objetivo baja
	//       media unidad, que es lo que hunde el encuadre.
	//   (b) CamNew.cpp:215-220 — si el ped toca el agua y la camara queda por debajo
	//       del nivel + 0,6: near clip a 0,2, se rehace el `Source` quedándose SOLO
	//       con la distancia horizontal, y la z se clava en nivel + 0,6. O sea la
	//       camara nunca se va bajo el agua: se queda pegada a la superficie. El
	//       `mod_Buoyancy.m_waterlevel` del mod es la variable global de agua; aqui se
	//       pregunta con `CWaterLevel::GetWaterLevel`, que es como lo consulta el motor.
	// Adaptación: se reutiliza `Process_AimWeaponCrouchOffset` tal cual (ya calcula
	//   el `end = -0.5f + ((maxFOV-FOV)/minFOV*maxFOV)/100` del mod, CamNew.cpp:454-455)
	//   con un `duckOffset` propio de esta ley, que es como el mod lleva uno por
	//   `CCamNew`. `modernCamera` del mod no se aplica (decision 5.4c).
	// Medible: criterio PASS = agachado el encuadre baja ~0,5 m y vuelve al soltar
	//   (FALLO si no se mueve); en agua la camara no se hunde y el near clip va a 0,2
	//   (FALLO si se ve el corte del agua o la camara queda bajo la superficie).
#ifdef VICEEXT_AIM_CLASSICAXIS
	static float odWalkDuckOffset = 0.0f;
	Process_AimWeaponCrouchOffset(odWalkDuckOffset);
	TargetCoors.z += odWalkDuckOffset;
#endif
	Source = TargetCoors - Front*CamDist;
#ifdef VICEEXT_AIM_CLASSICAXIS
	if (CamTargetEntity && CamTargetEntity->IsPed() && ((CPed*)CamTargetEntity)->bTouchingWater) {
		float odWl = 0.0f;
		if (CWaterLevel::GetWaterLevelNoWaves(Source.x, Source.y, Source.z, &odWl)
		 && Source.z < odWl + 0.6f) {
			RwCameraSetNearClipPlane(Scene.camera, 0.2f);
			// Se conserva solo la distancia horizontal: `dist * (maxDist/length)`.
			float odHoriz = (TargetCoors - Source).Magnitude2D();
			CVector odFwd2D(Front.x, Front.y, 0.0f);
			odFwd2D.Normalise2D();   // `Normalise2D` (Vector.h:28), que es 2D como el vector
			Source = TargetCoors - odFwd2D * odHoriz;
			Source.z = odWl + 0.6f;
		}
	}
#endif
	m_cvecTargetCoorsForFudgeInter = TargetCoors;

	// Clip Source and fix near clip
	CWorld::pIgnoreEntity = CamTargetEntity;
	entity = nil;
	if(CWorld::ProcessLineOfSight(TargetCoors, Source, colPoint, entity, true, true, true, true, false, false, true)){
		float PedColDist = (TargetCoors - colPoint.point).Magnitude();
		float ColCamDist = CamDist - PedColDist;
		if(entity->IsPed() && ColCamDist > DEFAULT_NEAR + 0.1f){
			// Ped in the way but not clipping through
			if(CWorld::ProcessLineOfSight(colPoint.point, Source, colPoint, entity, true, true, true, true, false, false, true)){
				PedColDist = (TargetCoors - colPoint.point).Magnitude();
				Source = colPoint.point;
				if(PedColDist < DEFAULT_NEAR + 0.3f)
					RwCameraSetNearClipPlane(Scene.camera, Max(PedColDist-0.3f, 0.05f));
			}else{
				RwCameraSetNearClipPlane(Scene.camera, Min(ColCamDist-0.35f, DEFAULT_NEAR));
			}
		}else{
			Source = colPoint.point;
			if(PedColDist < DEFAULT_NEAR + 0.3f)
				RwCameraSetNearClipPlane(Scene.camera, Max(PedColDist-0.3f, 0.05f));
		}
	}
	CWorld::pIgnoreEntity = nil;

	float ViewPlaneHeight = Tan(DEGTORAD(FOV) / 2.0f);
	float ViewPlaneWidth = ViewPlaneHeight * CDraw::CalculateAspectRatio() * fTweakFOV;
	float Near = RwCameraGetNearClipPlane(Scene.camera);
	float radius = ViewPlaneWidth*Near;
	entity = CWorld::TestSphereAgainstWorld(Source + Front*Near, radius, nil, true, true, false, true, false, false);
	int i = 0;
	// PORTADO — ClassicAXIS: CamNew.cpp:411-429 (`GetVectorsReadyForRW`), el bloque de
	// esconder peds, que el mod hace en la MISMA funcion para las dos leyes. Sus
	// `vecEntities` son 5 huecos que se restauran al frame siguiente; `CCam` no tiene
	// ese array, asi que aqui son 5 estaticos propios, igual que hace
	// `Process_AvoidCollisions` con `s_odHidePeds`.
#ifdef VICEEXT_AIM_CLASSICAXIS
	static CEntity *odWalkHidden[5] = { nil, nil, nil, nil, nil };
	for (int odh = 0; odh < 5; odh++) {
		if (odWalkHidden[odh]) {
			odWalkHidden[odh]->bIsVisible = true;
			odWalkHidden[odh] = nil;
		}
	}
#endif
	while(entity){
		CVector CamToCol = gaTempSphereColPoints[0].point - Source;
		float frontDist = DotProduct(CamToCol, Front);
		float dist = (CamToCol - Front*frontDist).Magnitude() / ViewPlaneWidth;

#ifdef VICEEXT_AIM_CLASSICAXIS
		// CamNew.cpp:425-429: ped VISIBLE a menos de 0,5 m (en 2D) del centro de la
		// esfera -> invisible este frame, y se guarda para volver a encenderlo. Es lo
		// que evita que un ped se vea atravesado por la camara al pegarse a ella.
		if (entity->IsPed() && entity->IsVisible()
		 && (Source + Front*Near - entity->GetPosition()).Magnitude2D() < 0.5f) {
			if (TheCamera.m_uiTransitionState == 0 && i < 5) {   // Camera.h:420
				odWalkHidden[i] = entity;
				entity->bIsVisible = false;
			}
		}
#endif
		// Try to decrease near clip
		dist = Max(Min(Near, dist), 0.1f);
		if(dist < Near)
			RwCameraSetNearClipPlane(Scene.camera, dist);

		// Move forward a bit
		if(dist == 0.1f)
			Source += (TargetCoors - Source)*0.3f;

		Near = RwCameraGetNearClipPlane(Scene.camera);
#ifndef FIX_BUGS
		// this is wrong...DEGTORAD missing
		radius = Tan(FOV / 2.0f) * CDraw::CalculateAspectRatio() * fTweakFOV * Near;
#else
		radius = ViewPlaneWidth*Near;
#endif
		// Keep testing
		entity = CWorld::TestSphereAgainstWorld(Source + Front*Near, radius, nil, true, true, false, true, false, false);

		i++;
		if(i > 5)
			entity = nil;
	}

	float TargetDist = (TargetCoors - Source).Magnitude();
	if(TargetDist < Distance)
		Distance = TargetDist;
	else{
		float f = Pow(fMouseAvoidGeomReturnRate, CTimer::GetTimeStep());
		Distance = (1.0f - f)*TargetDist + f*Distance;
		if(TargetDist > 0.05f)
			Source = TargetCoors + (Source-TargetCoors)*Distance/TargetDist;
		float clip = Distance-fRangePlayerRadius;
		if(clip < RwCameraGetNearClipPlane(Scene.camera))
			RwCameraSetNearClipPlane(Scene.camera, Max(clip, fCloseNearClipLimit));
	}

	TheCamera.m_bCamDirectlyInFront = false;
	TheCamera.m_bCamDirectlyBehind = false;

	GetVectorsReadyForRW();

#ifndef VICEEXT_AIM_CLASSICAXIS
	// PORTADO — ClassicAXIS: este bloque se QUITA. Es la razon de que el cuerpo
	// "gire con la camara siempre" (el defecto que reportó el jugador): en CADA frame
	// de la ley de a pie obliga a `m_fRotationCur = m_fRotationDest =` rumbo de la
	// camara y llama a `SetHeading`. Como la camara se actualiza DESPUES del ped,
	// eso pisa el rumbo que calcula el control y el A/D solo NUNCA puede girar el
	// cuerpo. El mod no tiene nada equivalente: alli el que fija el rumbo es su
	// `playerMovementType` (C1/C2), no la camara.
	if(((CPed*)CamTargetEntity)->CanStrafeOrMouseControl() && CDraw::FadeValue < 250 &&
	   (TheCamera.GetFadingDirection() != FADE_OUT || CDraw::FadeValue <= 100) &&
	   !CPad::GetPad(0)->IsPlayerControlsDisabledBy(PLAYERCONTROL_PLAYERINFO)){
		float Heading = Front.Heading();
		((CPed*)TheCamera.pTargetEntity)->m_fRotationCur = Heading;
		((CPed*)TheCamera.pTargetEntity)->m_fRotationDest = Heading;
		TheCamera.pTargetEntity->SetHeading(Heading);
		TheCamera.pTargetEntity->GetMatrix().UpdateRW();
	}
#endif
}

float fBillsBetaOffset;	// made up name, actually in CCam

void
CCam::Process_BehindCar(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	FOV = DefaultFOV;

	if(!CamTargetEntity->IsVehicle())
		return;

	CVector TargetCoors = CameraTarget;
	TargetCoors.z -= 0.2f;
	CA_MAX_DISTANCE = 9.95f;
	CA_MIN_DISTANCE = 8.5f;

	CVector Dist = Source - TargetCoors;
	float Length = Dist.Magnitude2D();
	m_fDistanceBeforeChanges = Length;
	if(Length < 0.002f)
		Length = 0.002f;
	Beta = CGeneral::GetATanOfXY(TargetCoors.x - Source.x, TargetCoors.y - Source.y);
#ifdef TOGGLEABLE_BETA_FEATURES
	// This is completely made up but Bill's cam manipulates an angle before calling this
	// and otherwise calculating Beta doesn't make much sense.
	Beta += fBillsBetaOffset;
	fBillsBetaOffset = 0.0f;
	Dist.x = -Length*Cos(Beta);
	Dist.y = -Length*Sin(Beta);
	Source = TargetCoors + Dist;
#endif
	if(Length > CA_MAX_DISTANCE){
		Source.x = TargetCoors.x + Dist.x/Length * CA_MAX_DISTANCE;
		Source.y = TargetCoors.y + Dist.y/Length * CA_MAX_DISTANCE;
	}else if(Length < CA_MIN_DISTANCE){
		Source.x = TargetCoors.x + Dist.x/Length * CA_MIN_DISTANCE;
		Source.y = TargetCoors.y + Dist.y/Length * CA_MIN_DISTANCE;
	}
	TargetCoors.z += 0.8f;

	Alpha = DEGTORAD(25.0f);
	Source.z = TargetCoors.z + CA_MAX_DISTANCE*Sin(Alpha);

	RotCamIfInFrontCar(TargetCoors, TargetOrientation);
	m_cvecTargetCoorsForFudgeInter = TargetCoors;
	CVector OrigSource = Source;
	TheCamera.AvoidTheGeometry(OrigSource, m_cvecTargetCoorsForFudgeInter, Source, FOV);

	Front = TargetCoors - Source;
	ResetStatics = false;
	GetVectorsReadyForRW();
}

float ZmOneAlphaOffset[] = { -0.01f, 0.1f, 0.125f, -0.1f, -0.06f };
float ZmTwoAlphaOffset[] = { 0.045f, 0.12f, 0.045f, 0.045f, -0.035f };
float ZmThreeAlphaOffset[] = { 0.005f, 0.005f, 0.15f, 0.005f, 0.12f };
float INIT_RC_HELI_HORI_EXTRA = 6.0f;
float INIT_RC_PLANE_HORI_EXTRA = 9.5f;
float INIT_RC_HELI_ALPHA_EXTRA = 0.2f;
float INIT_RC_PLANE_ALPHA_EXTRA = 0.295f;

void
CCam::WorkOutCamHeight(const CVector &TargetCoors, float TargetOrientation, float TargetHeight)
{
	if(!CamTargetEntity->IsVehicle())
		return;

	static float AlphaOffset = 0.0;
	static float AlphaOffsetSpeed = 0.0;
	static float AlphaDec = 0.0f;

	bool isHeli = false;
	bool isBike = false;
	int appearance = ((CVehicle*)CamTargetEntity)->GetVehicleAppearance();
	if(appearance == VEHICLE_APPEARANCE_BIKE)
		isBike = true;
	if(appearance == VEHICLE_APPEARANCE_HELI)
		isHeli = true;
	int index = 0;
	TheCamera.GetArrPosForVehicleType(appearance, index);

	float ExtraOffset = 0.0f;
	int id = CamTargetEntity->GetModelIndex();
	if(id == MI_RCRAIDER || id == MI_RCGOBLIN)
		ExtraOffset = INIT_RC_HELI_ALPHA_EXTRA;
	else if(id == MI_RCBARON)
		ExtraOffset = INIT_RC_PLANE_ALPHA_EXTRA;

	if(ResetStatics){
		AlphaOffset = 0.0f;
		AlphaOffsetSpeed = 0.0f;
		AlphaDec = 0.0f;

		if(TheCamera.CarZoomIndicator == CAM_ZOOM_1)
			AlphaOffset = ZmOneAlphaOffset[index] + ExtraOffset;
		else if(TheCamera.CarZoomIndicator == CAM_ZOOM_2)
			AlphaOffset = ZmTwoAlphaOffset[index] + ExtraOffset;
		else if(TheCamera.CarZoomIndicator == CAM_ZOOM_3)
			AlphaOffset = ZmThreeAlphaOffset[index] + ExtraOffset;
	}

	if(TheCamera.CarZoomIndicator == CAM_ZOOM_1)
		WellBufferMe(ZmOneAlphaOffset[index] + ExtraOffset, &AlphaOffset, &AlphaOffsetSpeed, 0.17f, 0.08f, false);
	else if(TheCamera.CarZoomIndicator == CAM_ZOOM_2)
		WellBufferMe(ZmTwoAlphaOffset[index] + ExtraOffset, &AlphaOffset, &AlphaOffsetSpeed, 0.17f, 0.08f, false);
	else if(TheCamera.CarZoomIndicator == CAM_ZOOM_3)
		WellBufferMe(ZmThreeAlphaOffset[index] + ExtraOffset, &AlphaOffset, &AlphaOffsetSpeed, 0.17f, 0.08f, false);

	float Length = (Source - TargetCoors).Magnitude2D();

	CVector Forward = CamTargetEntity->GetForward();
	float CarAlpha = CGeneral::GetATanOfXY(Forward.Magnitude2D(), Forward.z);
	// this shouldn't be necessary....
	while(CarAlpha >= PI) CarAlpha -= 2*PI;
	while(CarAlpha < -PI) CarAlpha += 2*PI;

	while(Beta >= PI) Beta -= 2*PI;
	while(Beta < -PI) Beta += 2*PI;

	float DeltaBeta = Beta - TargetOrientation;
	while(DeltaBeta >= PI) DeltaBeta -= 2*PI;
	while(DeltaBeta < -PI) DeltaBeta += 2*PI;

	float BehindCarNess = Cos(DeltaBeta);	// 1 if behind car, 0 if side, -1 if in front
	CarAlpha = -CarAlpha * BehindCarNess;

	float fwdSpeed = DotProduct(((CPhysical*)CamTargetEntity)->m_vecMoveSpeed, CamTargetEntity->GetForward())*180.0f;
	if(CamTargetEntity->GetModelIndex() == MI_FIRETRUCK && CPad::GetPad(0)->GetCarGunFired()){
		CarAlpha = DEGTORAD(10.0f);
	}else if(isHeli){
		CarAlpha = 0.0f;
		float heliFwdZ = CamTargetEntity->GetForward().z;
		float heliFwdXY = CamTargetEntity->GetForward().Magnitude2D();
		float alphaAmount = Min(Abs(fwdSpeed/90.0f), 1.0f);
		if(heliFwdXY != 0.0f || heliFwdZ != 0.0f)
			CarAlpha = CGeneral::GetATanOfXY(heliFwdXY, Abs(heliFwdZ)) * alphaAmount;

		CColPoint point;
		CEntity *entity = nil;
		CVector Test = Source;
		Test.z = TargetCoors.z + 0.2f + Length*Sin(CarAlpha+AlphaOffset) + m_fCloseInCarHeightOffset;
		if(CWorld::ProcessVerticalLine(Test, CamTargetEntity->GetPosition().z, point, entity, true, false, false, false, false, false, nil)){
			float sin = (point.point.z - TargetCoors.z - 0.2f - m_fCloseInCarHeightOffset)/Length;
			CarAlpha = Asin(Clamp(sin, -1.0f, 1.0f)) - AlphaOffset;
			if(CarAlpha < 0.0f)
				AlphaOffset += CarAlpha;
		} 
	}

	CarAlpha = CGeneral::LimitRadianAngle(CarAlpha);
	if(CarAlpha < 0.0f) CarAlpha = 0.0f;
	if(CarAlpha > DEGTORAD(89.0f)) CarAlpha = DEGTORAD(89.0f);

	if(ResetStatics)
		Alpha = CarAlpha;

	float TargetAlpha = Alpha;
	float DeltaAlpha = CarAlpha - TargetAlpha;
	while(DeltaAlpha >= PI) DeltaAlpha -= 2*PI;
	while(DeltaAlpha < -PI) DeltaAlpha += 2*PI;
	if(Abs(DeltaAlpha) > 0.0f && !TheCamera.m_bVehicleSuspenHigh)
		TargetAlpha = CarAlpha;

	if(isBike)
		WellBufferMe(TargetAlpha, &Alpha, &AlphaSpeed, 0.09f, 0.04f, true);
	else if(isHeli)
		WellBufferMe(TargetAlpha, &Alpha, &AlphaSpeed, 0.09f, 0.04f, true);
	else
		WellBufferMe(TargetAlpha, &Alpha, &AlphaSpeed, 0.15f, 0.07f, true);

	Source.z = TargetCoors.z + Sin(Alpha + AlphaOffset)*Length + m_fCloseInCarHeightOffset;
	AlphaOffset -= AlphaDec;
}

// Rotate cam behind the car when the car is moving forward
bool
CCam::RotCamIfInFrontCar(CVector &TargetCoors, float TargetOrientation)
{
	float BetaMaxSpeed = 0.15f;
	float BetaAcceleration = 0.007f;
	bool MovingForward = false;
	float MaxDiffBeta = DEGTORAD(160.0f);
	CPhysical *phys = (CPhysical*)CamTargetEntity;

	float ForwardSpeed = DotProduct(phys->GetForward(), phys->GetSpeed(CVector(0.0f, 0.0f, 0.0f)));
	if(ForwardSpeed > 0.02f)
		MovingForward = true;

	if(phys->IsVehicle() && (phys->GetModelIndex() == MI_SPARROW || phys->GetModelIndex() == MI_HUNTER)){
		MaxDiffBeta = DEGTORAD(160.0f);
		BetaMaxSpeed = 0.1f;
		BetaAcceleration = 0.003f;
		CVector speed = phys->GetSpeed(CVector(0.0f, 0.0f, 0.0f));
		speed.z = 0.0f;
		if(50.0f*speed.Magnitude() > 3.13f)
			TargetOrientation = CGeneral::GetATanOfXY(speed.x, speed.y);
	}

	float Dist = (Source - TargetCoors).Magnitude2D();

	float DeltaBeta = TargetOrientation - Beta;
	while(DeltaBeta >= PI) DeltaBeta -= 2*PI;
	while(DeltaBeta < -PI) DeltaBeta += 2*PI;

	if(Abs(DeltaBeta) > PI-MaxDiffBeta && MovingForward && TheCamera.m_uiTransitionState == 0)
		m_bFixingBeta = true;

	CPad *pad = CPad::GetPad(0);
	if(!(pad->GetLookBehindForCar() || pad->GetLookBehindForPed() || pad->GetLookLeft() || pad->GetLookRight()))
		if(DirectionWasLooking != LOOKING_FORWARD)
			TheCamera.m_bCamDirectlyBehind = true;

	if(!m_bFixingBeta && !TheCamera.m_bUseTransitionBeta && !TheCamera.m_bCamDirectlyBehind && !TheCamera.m_bCamDirectlyInFront)
		return false;

	bool SetBeta = false;
	if(TheCamera.m_bCamDirectlyBehind || TheCamera.m_bCamDirectlyInFront || TheCamera.m_bUseTransitionBeta)
		if(&TheCamera.Cams[TheCamera.ActiveCam] == this)
			SetBeta = true;

	if(m_bFixingBeta || SetBeta){
		WellBufferMe(TargetOrientation, &Beta, &BetaSpeed, BetaMaxSpeed, BetaAcceleration, true);

		if(TheCamera.m_bCamDirectlyBehind && &TheCamera.Cams[TheCamera.ActiveCam] == this)
			Beta = TargetOrientation;
		if(TheCamera.m_bCamDirectlyInFront && &TheCamera.Cams[TheCamera.ActiveCam] == this)
			Beta = TargetOrientation + PI;
		if(TheCamera.m_bUseTransitionBeta && &TheCamera.Cams[TheCamera.ActiveCam] == this)
			Beta = m_fTransitionBeta;

		Source.x = TargetCoors.x - Cos(Beta)*Dist;
		Source.y = TargetCoors.y - Sin(Beta)*Dist;

		// Check if we're done
		DeltaBeta = TargetOrientation - Beta;
		while(DeltaBeta >= PI) DeltaBeta -= 2*PI;
		while(DeltaBeta < -PI) DeltaBeta += 2*PI;
		if(Abs(DeltaBeta) < DEGTORAD(2.0f))
			m_bFixingBeta = false;
	}
	TheCamera.m_bCamDirectlyBehind = false;
	TheCamera.m_bCamDirectlyInFront = false;
	return true;
}

float FIRETRUCK_TRACKING_MULT = 0.1f;
float fTestShiftHeliCamTarget = 0.6f;
float TiltTopSpeed[] = { 0.035f, 0.035f, 0.001f, 0.005f, 0.035f };
float TiltSpeedStep[] = { 0.016f, 0.016f, 0.0002f, 0.0014f, 0.016f };
float TiltOverShoot[] = { 1.05f, 1.05f, 0.0f, 0.0f, 1.0f };


// BARRIDO 1 (B1, 25/09/2026) — SEGUIMIENTO DE COCHE + AUTO-RETORNO, spec del mod.
// El jugador: el seguimiento actual es basura. Se BARRE la ley vieja y se pone
// la spec (no se calibra encima). Origen:
//   spec CamNew: `gta_vc_browser/tmp/extsrc/CamNew.cpp` (Process_FollowPed :52-231,
//     Process_AimWeapon :233-388, Process_AvoidCollisions :390-445, FOVLerp :477-498)
//   inis ClassicAXIS: CameraCrosshairMult 0.53/0.4, LockOnTargetType=1 (plan 13 §3.3)
//   GeniusZ: near-clip dual + offsets por vehiculo (plan 13 §3.3)
//   referencia: plan 10 §B8, plan 13 §3.3/§6.
// BLOQUES VIEJOS BARRIDOS (fichero:lineas aprox + motivo):
//  1. Cam.cpp ~2068-2282: autocentrado por temporizador (defines
//     VICEEXT_CAR_AUTOCENTER_* / VICEEXT_CAR_FORCECENTER_*, detectores
//     ViceExtCameraLookingMouse/Keys, ViceExtCarAutoCenterWanted/Step/Speed,
//     ViceExtRotateCamAroundTarget). MOTIVO: peleaba con la auto-rotacion
//     WellBufferMe de CamNew (dos leyes de retorno: pasiva con temporizador +
//     empujon rapido al soltar tecla + giro proporcional al error). El raton
//     disparaba el empujon de 2,2 rad/s al dejar de moverse (R13). Queda UNA
//     sola ley: WellBufferMe(0.1/0.06, umbral 0.06) para el boton de centrar y
//     para el retorno pasivo; el raton SOLO reinicia el temporizador, nunca
//     empuja (el filtro de ventana anti-ruido sub-pixel se conserva inline en
//     el string-cam, no es ley de retorno).
//  2. Cam.cpp ~2351-2356 + Cam_On_A_String_Unobscured (~2444-2484) +
//     WorkOutCamHeight (~1878-1989) + RotCamIfInFrontCar (~1992-2060, llamada
//     del string-cam; la FUNCION se conserva: la sigue usando Process_BehindCar)
//     + AvoidTheGeometry del string-cam (~2360-2363). MOTIVO: distancia XY +
//     altura por canales separados + rotacion por velocidad lateral: peleaban
//     con las colisiones y dejaban la camara clavada/ladeada. Se sustituye por
//     la esferica unica de CamNew (dist 3D + Beta/Alpha + LOS + 5 esferas).
//  3. Cam.cpp ~2356: llamada a FixCamWhenObscuredByVehicle (~2487-2504).
//     MOTIVO: choca con el LOS nuevo (empuje +Z sobre techos falsea dist/alt;
//     el LOS ya deja Source en el hit + nearClip). La FUNCION se conserva sin
//     llamar (declarada en Camera.h; quitarla tocaria la cabecera).
//  4. Cam.cpp ~5393-6098 (Process_FollowCar_SA): tablas CARCAM_SET, historial
//     m_aTargetHistoryPos*, yaw por velocidad (betaChangeMult), stick 0.007,
//     raton con inercia stepsLeftToChangeBetaByMouse, alpha-blend, colisiones
//     estilo LCS (dontCollideWithCars + IS_TRAFFIC_LIGHT), su propio
//     autocentrado + traza `camauto`. MOTIVO: era la segunda ley de coche (solo
//     corria con bFreeCam). Ahora delega en la ley unica del string-cam;
//     RETENIDA la torreta Rhino/Firetruck (no es seguimiento).
//  5. Cam.cpp ~1701 (Process_FollowPedWithMouse): clamp +45° de serie.
//     MOTIVO: la spec pide +60/-89.5 en follow y ±50° apuntando.
// NUEVA LEY (string-cam, CamNew adaptada al coche):
//   dist [2.0, BaseDist+0.1+CarZoomValueSmooth+extras RC] (min 2.0 CamNew :66;
//     max con base por vehiculo, GeniusZ); altura 0.8*dimZ / heli (serie,
//     por vehiculo; el 0.4 de CamNew :72 es A PIE); stick
//     0.01*(1/20) y 0.01*(0.6/20) *FOV/80 (:143-144); raton (-2.5x,4y) *
//     MouseAccel*FOV/80 (:139-140,149-150); clamp +60/-89.5 (:168-171);
//     retorno WellBufferMe(0.1/0.06) umbral 0.06 (:178-185, boton + pasivo tras
//     1200 ms avanzando); LOS target->source + nearClip min 0.05 (:396-405);
//     5 esferas r=viewPlaneWidth*nearClip, min 0.1 (:411-444, SIN ocultar peds:
//     CCam no tiene vecEntities; se conserva nearClip+empuje); agua: suelo en
//     nivel (serie del coche; el nivel+0.6 de :215-220 es A PIE); tilt/roll y
//     nudge Firetruck RETENIDOS (no son seguimiento).
//   Adaptacion necesaria: Beta se RE-DERIVA de Source cada frame (la serie del
//     string-cam hacia lo mismo; sobrevive a LookBehind/transiciones, que
//     mueven Source sin pasar por aqui). Alpha se integra como CamNew. (Ni el
//     derive-rama-mando de CamNew :117-121 ni el de FollowPed_Rotation son
//     identidad: re-derivar Alpha pinball-ea la camara; por eso Alpha persiste.)
// TRAZAS: se conserva `CAMSA` (mismo formato) y `camauto2` (mismo nombre y
//   formato; `pedido` pasa a ser "boton de centrar pulsado": el empujon viejo
//   ya no existe); `camauto` (SA) muere con su bloque; se anade `CAMV2`
//   (modo/dist/alt, 1/s) para medir la ley nueva. `AIMDIR` vive en PlayerPed
//   (fuera de los 5 ficheros: no se toca). Recoil (ViceExtRecoilAlphaAdd +
//   consumo en Weapon.cpp) NO se toca.


void
CCam::Process_Cam_On_A_String(const CVector &CameraTarget, float TargetOrientation, float, float)
{

	// BARRIDO 1: ley CamNew adaptada al coche (detalle en el banner sobre
	// `TiltTopSpeed`). Convenciones: Beta = atan(Target-Source) (igual que
	// CamNew H y que la serie del string-cam); front=(cosA*cosB, cosA*sinB,
	// sinA); Source = Target - front*len. Beta se RE-DERIVA de Source cada
	// frame (sobrevive a LookBehind/transiciones); Alpha se integra (CamNew).
	if(!CamTargetEntity->IsVehicle())
		return;

	CVehicle *car = (CVehicle*)CamTargetEntity;
	CPad *pad = CPad::GetPad(0);

	FOV = DefaultFOV;

	CBaseModelInfo *mi = CModelInfo::GetModelInfo(CamTargetEntity->GetModelIndex());
	CVector Dimensions = mi->GetColModel()->boundingBox.max - mi->GetColModel()->boundingBox.min;
	CVector TargetCoors = CameraTarget;
	float BaseDist = Dimensions.Magnitude();
	if(car->IsBike())
		BaseDist *= 1.45f;
	if(car->GetVehicleAppearance() == VEHICLE_APPEARANCE_HELI &&
	   CamTargetEntity->GetStatus() != STATUS_PLAYER_REMOTE)
		TargetCoors += fTestShiftHeliCamTarget * CamTargetEntity->GetUp() * Dimensions.z;
	else
		TargetCoors.z += 0.8f*Dimensions.z;

	// Esferica CamNew: min 2.0 (follow a pie, :66); max con base por vehiculo
	// (GeniusZ) + zoom suavizado + extras RC (serie).
	const float minDist = 2.0f;
	float maxDist = BaseDist + 0.1f + TheCamera.CarZoomValueSmooth;
	if(CamTargetEntity->GetModelIndex() == MI_RCRAIDER || CamTargetEntity->GetModelIndex() == MI_RCGOBLIN)
		maxDist += INIT_RC_HELI_HORI_EXTRA;
	else if(CamTargetEntity->GetModelIndex() == MI_RCBARON)
		maxDist += INIT_RC_PLANE_HORI_EXTRA;

	bool lockMovement = pad->ArePlayerControlsDisabled();

	if(ResetStatics){
		BetaSpeed = 0.0f;
		AlphaSpeed = 0.0f;
		Rotating = false;
		Alpha = 0.0f;
		Beta = TargetOrientation;
		Front = CVector(Cos(Alpha) * Cos(Beta), Cos(Alpha) * Sin(Beta), Sin(Alpha));
		Source = TargetCoors - Front * maxDist;
	}

	CVector dist = Source - TargetCoors;
	float length = dist.Magnitude();
	if(length < 0.001f){
		CVector fwd = CamTargetEntity->GetForward();
		fwd.z = 0.0f;
		if(fwd.MagnitudeSqr() < 0.0001f)
			fwd = CVector(1.0f, 0.0f, 0.0f);
		fwd.Normalise();
		Source = TargetCoors - fwd * maxDist;
		dist = Source - TargetCoors;
		length = dist.Magnitude();
	}

	// ve73: ¿el frame ANTERIOR tenia geometria delante de la camara (rayo o
	// esfera)? Si no, la distancia vuelve a maxDist. Se declara aqui porque el
	// primer uso (la restauracion de la distancia) va ANTES del bloque de trazas.

	// Zoom: si cambia se reescala (CamNew :101-104); si no, [minDist, maxDist].
	static float s_prevMaxDist = -1.0f;
	if(ResetStatics)
		s_prevMaxDist = maxDist;
	if(s_prevMaxDist != maxDist){
		if(length > 0.0f)
			dist *= maxDist / length;
		s_prevMaxDist = maxDist;
	}else{
		if(length < minDist)
			dist *= minDist / length;
		else if(length > maxDist)
			dist *= maxDist / length;
	}
	length = dist.Magnitude();

	// ve73 (fallo "de frente la camara se ve demasiado cerca", imagen 2): la
	// distancia es ESTADO (viene del frame anterior) y esta ley solo la acotaba a
	// [minDist,maxDist]: NADA la devolvia a la distancia querida. Con el vehiculo
	// avanzando CONTRA la camara (mirando de frente) el objetivo se acercaba a
	// Source frame a frame y la distancia se encogia sola hasta quedarse clavada
	// en minDist. Medido en ve72: 7,13 -> 3,85 -> 2,06 -> 2,00 clavada, con
	// los=0 y sph=0 (NO habia geometria ninguna: ni rayo ni esfera). Mirando
	// atras pasa lo contrario: al alejarse el objetivo la distancia crece y el
	// clamp la fija en maxDist, por eso detras siempre se veia bien. Y con Q+E
	// (LookBehind pone Dist = CA_MAX_DISTANCE cada frame) la vista de frente
	// tambien era la buena: era la unica via que restauraba la distancia. Ahora,
	// si el frame anterior no tenia obstaculo, la distancia se recoloca en
	// maxDist (mas cerca del original, que re-resolvia Source desde cero cada
	// frame en AvoidTheGeometry). Si SI lo habia, se respeta lo encogido por la
	// geometria para no atravesarla.
	if(!s_odDistObs)
		length = maxDist;

	// Beta re-derivada (identidad: front horizontal queda proporcional a
	// Target-Source mientras |Alpha|<90°); Alpha persiste (integrada).
	// ve70 (CAUSA DE FONDO de la "lucha"): el yaw (Beta) pasa a ser ESTADO
	// PROPIO. Antes se RE-DERIVABA de Source en cada frame; como Source lo
	// mueven los snaps (mirar atras/lados), la geometria y los teleportes,
	// cualquier cosa arrastraba el yaw sin que el jugador ni la ley
	// intervinieran (medido en ve69: deg saltaba 100+ grados con mag=0, big=0,
	// rot=0 e idle<2000). Ahora Beta solo lo cambian el raton/palo (offsets), la
	// ley de retorno de 1,5 s y los bloques forzados; se re-deriva de Source SOLO
	// cuando un snap lo ha movido (eso si es "mirar" de verdad) o al entrar.
	// Alpha ya era estado propio (se integra): por eso el eje X "recentraba"
	// solo y el Y no - el X se recalculaba cada frame, el Y nadie lo tocaba.
	static bool s_odWasSnap = false;
	bool odSnapping = LookingBehind || LookingLeft || LookingRight;
	if(odSnapping || s_odWasSnap || ResetStatics){
		Beta = CGeneral::GetATanOfXY(-dist.x, -dist.y);
		while(Beta >= PI) Beta -= 2.0f * PI;
		while(Beta < -PI) Beta += 2.0f * PI;
	}
	s_odWasSnap = odSnapping;
	float odBetaGeo = Beta; // yaw antes de aplicar offsets (sin re-derivar este frame)
	while(Alpha >= PI) Alpha -= 2.0f * PI;
	while(Alpha < -PI) Alpha += 2.0f * PI;

	// Entrada: misma seleccion y signo que Process_FollowPedWithMouse (el
	// raton manda si se mueve; si no, el palo con deadzone por eje en
	// LookAround*). Stick CamNew :143-144, raton (-2.5x,4y) :139-140.
	float LookLeftRight = -((float)pad->LookAroundLeftRight());
	float LookUpDown = ((float)pad->LookAroundUpDown());
	float MouseX = pad->GetMouseX();
	float MouseY = pad->GetMouseY();
	bool useMouse = false;
	if((MouseX != 0.0f || MouseY != 0.0f) && !pad->ArePlayerControlsDisabled()){
		useMouse = true;
		LookLeftRight = -2.5f * MouseX;
		LookUpDown = 4.0f * MouseY;
	}
	// ve61 (vuelve snap): Q/E NO orbitan Beta (quita +-127/frame que giraba infinito);
	// el lado lo da el snap LookLeft/Right de CCam::Process (estable) + idle 2s y retorno.
	// Beta solo palo derecho/raton; el vertical queda solo en su eje.
	CA_MAX_DISTANCE = maxDist; CA_MIN_DISTANCE = minDist;
	float BetaOffset, AlphaOffset;
	if(useMouse){
		BetaOffset = LookLeftRight * TheCamera.m_fMouseAccelHorzntl * FOV / 80.0f;
		AlphaOffset = LookUpDown * TheCamera.m_fMouseAccelVertical * FOV / 80.0f;
	}else{
		BetaOffset = LookLeftRight * fStickSens * (1.0f / 20.0f) * FOV / 80.0f * CTimer::GetTimeStep();
		// ve73 (fallo "se sube al vehiculo y al darle a W se comporta como si hubiera
		// movido el mouse"): en un vehiculo el eje VERTICAL del palo derecho es el
		// ACELERADOR/FRENO, no mirar (ControllerConfig: VEHICLE_ACCELERATE = rsUP,
		// VEHICLE_BRAKE = rsDOWN, y rsUP = CPad::GetUp(), o sea W y flecha arriba).
		// Medido en ve72: con W pulsada rsy=128 y LookAroundUpDown() devuelve -170,
		// asi que AlphaOffset valia -0,00095 por frame (~3,3 grados/s de cabeceo)
		// sin tocar el raton. El eje X (RightStickX) si es mirar; el vertical de la
		// camara del coche solo lo mueve el raton.
		AlphaOffset = 0.0f;
	}

	// Detector de mirada con ventana (era ViceExtCameraLookingMouse: el ruido
	// sub-pixel se cancela y un giro lento coherente suma; NO es ley de
	// retorno, solo dice si el jugador esta mirando). El raton SOLO reinicia
	// el temporizador pasivo: nunca fija Rotating ni empuja (B1.2).
	static float s_mouseWinX = 0.0f, s_mouseWinY = 0.0f;
	static uint32 s_lastLookCar = 0;
	static bool s_lookInit = false;
	uint32 nowCar = CTimer::GetTimeInMilliseconds();
	if(ResetStatics || !s_lookInit){
		s_mouseWinX = 0.0f;
		s_mouseWinY = 0.0f;
		// ve74 (fallo "deja de centrar la camara al subir a un vehiculo y empezar
		// a andar"): al ENTRAR al vehiculo (ResetStatics = cambio de modo) el reloj
		// arrancaba de CERO, pero el jugador no ha mirado con el raton: no hay nada
		// que aplazar. Con idle=0 el pasivo no actuaba hasta el delay y, al empezar a
		// andar, la camara se quedaba con el yaw viejo mientras el coche giraba.
		// Medido en ve73 (mx=my=0, rot=0): Beta CONGELADA en 65,5 grados mientras
		// TargetOrientation giraba 59,6 -> -2,7 -> -54,9. El jugador lo describe
		// como "se comporta como si hubiese movido el mouse". Ahora la entrada
		// cuenta como "no ha mirado desde hace rato" y la camara sigue el rumbo
		// del vehiculo desde el primer frame. Solo el raton (orbiting) arranca el
		// reloj de inactividad. (El guard de abajo evita el wrap si nowCar < 3000.)
		s_lastLookCar = ResetStatics ? (nowCar - 3000u) : nowCar;
		s_lookInit = true;
	}
	if(nowCar < s_lastLookCar)
		s_lastLookCar = nowCar;
	// ve67: baseline de Alpha (altura) por vehiculo para el retorno del eje Y y
	// persistencia del fantasma de boton (LS2/RS2 pegados) para la traza.
	uint8 odLs2 = pad->NewState.LeftShoulder2;
	uint8 odRs2 = pad->NewState.RightShoulder2;
	// ve71/ve72: el resultado del rayo y de las 5 esferas son statics de AMBITO DE
	// FICHERO desde B3(c), porque los escribe tambien Process_AvoidCollisions (que
	// lo usa la ley de apuntado). Los leen CAMB2/CAMB3/CAMB2b/CAMB3b.
	static float s_odCarAlphaBase = 0.0f;
	static bool s_odKeyLookPersist = false;
	static bool s_odPersistInit = false;
	static CEntity *s_odPersistCar = nil;
	if(ResetStatics || !s_odPersistInit){
		s_odCarAlphaBase = 0.0f;
		s_odKeyLookPersist = false;
		s_odPersistCar = nil;
		s_odPersistInit = true;
	}
	if(s_odPersistCar != CamTargetEntity){
		s_odPersistCar = CamTargetEntity;
		s_odCarAlphaBase = 0.0f;
	}
	if(odLs2 != 0 || odRs2 != 0)
		s_odKeyLookPersist = true;
	{
		float keep = Pow(0.35f, CTimer::GetTimeStep());
		s_mouseWinX = s_mouseWinX * keep + MouseX;
		s_mouseWinY = s_mouseWinY * keep + MouseY;
	}
	bool mouseActive = (Max(Abs(s_mouseWinX), Abs(s_mouseWinY)) >= 1.5f);
	bool odMouseActive = mouseActive; // ve68: valor real, antes del rearme
	if(mouseActive){
		s_mouseWinX = 0.0f;
		s_mouseWinY = 0.0f;
	}
	bool stickActive = (pad->LookAroundLeftRight() != 0 || pad->LookAroundUpDown() != 0);
	bool rawMouse = (MouseX != 0.0f || MouseY != 0.0f);
	// ve60: orbitar SOLO con mirar (Q/E=GetLookLeft/Right, GetLookBehindForCar, palo derecho=LookAround,
	// raton). DPad/LeftStick (A/D conducir: GO_LEFT/GO_RIGHT->DPadLeft/Right) NUNCA orbita ni reinicia idle;
	// el rumbo lo sigue la auto-rotacion (WellBufferMe a TargetOrientation), no el steer. Palo derecho
	// (mirar)=RightStickX/Y (Pad::LookAround); steer=LeftStickX+DPad (Pad::GetSteeringLeftRight). Son distintos.
	bool keyLook = pad->GetLookLeft() || pad->GetLookRight() || pad->GetLookBehindForCar();
	// A (ve62): el raton en coche entra por useMouse/BetaOffset-AlphaOffset
	// (LookLeftRight/UpDown desde MouseX/MouseY crudo); rawMouse solo
	// cubre el crudo y mouseActive la ventana: si el crudo vale 0 en este
	// modo pero el offset aplicado es no-nulo, orbiting lo perdia y el
	// autocentrado volvia al centro. Se incluye la via real aplicada.
	// ve68 (raiz del "el recentrado no llega nunca"): el navegador entrega
	// deltas de raton SUB-PIXEL (|dx|,|dy| < 0.5) casi todos los frames, asi que
	// `MouseX != 0.0f`, `useMouse` y `(BetaOffset != 0.0f)` eran TRUE siempre
	// (medido: orbit=1 en 78/78 de ve67 y 11516/11516 de ve66, con idle=0 y
	// bOff=-0.000). orbiting clavado en true => `Rotating = false` cada frame y
	// el reloj de quietud jamas llega a los 2000 ms: el pasivo no corria jamas.
	// Umbral de INTENCION: solo cuenta mirar de verdad (>= 0.5 de delta o un
	// offset que se note en Beta/Alpha). El ruido sub-pixel sigue aplicandose a
	// Beta/Alpha (se cancela solo) pero ya no bloquea el retorno.
	float odLookMag = Abs(MouseX) + Abs(MouseY);
	bool odMouseBig = (odLookMag >= 0.5f);
	bool odOffsetsBig = (Abs(BetaOffset) >= 0.001f || Abs(AlphaOffset) >= 0.001f);
	bool odUseMouseBig = useMouse && odMouseBig;
	// ve71: FUERA `stickActive`. Las flechas (y espacio) escriben el palo derecho
	// (rsy=+-128 medido) y encendian orbiting con el raton quieto (idle=0,
	// orbit=1, bOff=aOff=0.0000), o sea el "wheelie/stoppie" aplazaba el
	// recentrado 2 s cada vez. Lo que decide es el OFFSET QUE SE APLICA de
	// verdad (odOffsetsBig): si el palo no mueve la camara, no cuenta.
	bool orbiting = mouseActive || odMouseBig || keyLook || odUseMouseBig || odOffsetsBig;
	// ve72 (fallo "espacio corta el centrado"): FUERA `pad->GetTarget()`. En
	// vehiculo RightShoulder1 es el FRENO DE MANO (espacio; ControllerConfig
	// VEHICLE_HANDBRAKE = rsRCTRL + ' '), no mirar: medido en ve71 (02:22:36)
	// sup=1 gt=1 idle=0 con el raton quieto => cada pulsacion de espacio reseteaba
	// el reloj de 2 s. El pasivo solo debe ceder ante apuntar en 1a persona
	// (Using1stPersonWeaponMode) y ante mirar de verdad (orbiting: raton/palo/Q/E/RMB).
	bool suppress = TheCamera.Using1stPersonWeaponMode();
	if(orbiting || suppress)
		s_lastLookCar = nowCar;

	// UNICA ley de retorno (CamNew :178-185): el boton de centrar y el pasivo
	// (avanzando, 1500 ms sin mirar) van al mismo WellBufferMe(0.1/0.06).
	if(orbiting || lockMovement)
		Rotating = false;
	if(!lockMovement){
		Beta += BetaOffset;
		Alpha += AlphaOffset;
	}
	while(Beta >= PI) Beta -= 2.0f * PI;
	while(Beta < -PI) Beta += 2.0f * PI;
	if(Alpha > DEGTORAD(60.0f)) Alpha = DEGTORAD(60.0f);
	else if(Alpha < -DEGTORAD(89.5f)) Alpha = -DEGTORAD(89.5f);

	bool driving = car->GetStatus() == STATUS_PLAYER && car->pDriver == FindPlayerPed();
	bool moving = driving && car->GetMoveSpeed().Magnitude2D() > 1.0f;
	// ve75: el jugador pide 1,5 s en vez de 2 s.
	const uint32 idleMs = 1500u;
	// C1 (plan camara-coche-sin-lucha): la radio (ForceCameraBehindPlayer=
	// LeftShoulder1=VEHICLE_CHANGE_RADIO_STATION) deja de recentrar; el
	// pasivo de 1,5 s es la unica via (pedido del jugador 25/09: sin boton).
	// C2: sin condicion de velocidad (tambien parado).
	bool btnReq = false;
	// ve67 (retorno del eje Y): el pasivo tambien devuelve Alpha (altura) a la
	// base del vehiculo (0.0 = horizontal a la altura del objetivo). Hasta ahora
	// la altura que dejaban el raton vertical o los snaps quedaba clavada (log
	// ve66: 0.37->5.98->-1.77 sin volver) - la otra mitad de la "lucha". Se
	// anula igual que Beta: si el jugador vuelve a mirar (orbit/suppress) o hay
	// lock, el WellBufferMe deja de aplicarse en el siguiente frame.
	if(!orbiting && !lockMovement && !suppress && driving && nowCar - s_lastLookCar > idleMs){
		m_fTargetBeta = TargetOrientation;
		Rotating = true;
		if(Alpha != s_odCarAlphaBase)
			WellBufferMe(s_odCarAlphaBase, &Alpha, &AlphaSpeed, 0.1f, 0.06f, false);
	}
	if(Rotating){
		WellBufferMe(m_fTargetBeta, &Beta, &BetaSpeed, 0.1f, 0.06f, true);
		float deltaBeta = m_fTargetBeta - Beta;
		while(deltaBeta >= PI) deltaBeta -= 2.0f * PI;
		while(deltaBeta < -PI) deltaBeta += 2.0f * PI;
		if(Abs(deltaBeta) < 0.06f)
			Rotating = false;
	}
#ifdef __EMSCRIPTEN__
	{
		static int8 s_odLastWanted2 = -1;
		int8 cur2 = (int8)(Rotating ? 1 : 0);
		if(cur2 != s_odLastWanted2){
			s_odLastWanted2 = cur2;
			char odt[140];
			snprintf(odt, sizeof odt, "VICEEXT camauto2 auto=%d mirando=%d teclas=%d conduzco=%d pedido=%d avanza=%d vel=%.1f idle=%u",
				(int)Rotating, (int)orbiting, (int)stickActive, (int)driving, (int)btnReq, (int)moving,
				car->GetMoveSpeed().Magnitude2D(), (unsigned)(nowCar - s_lastLookCar));
			ODTRACES(odt);
		}
	}
	{
		// CAMB2 (ve67): diagnosticar la "lucha". beta/alpha relativos al coche,
		// deltas de raton crudos, ventana, reloj de quietud, y AHORA la fuente
		// del orbit eterno: keyLook, offsets aplicados y LS2/RS2 crudos.
		// Guard 1/s REAL: solo se re-ancla si el reloj salto hacia atras (menu);
		// el reancla viejo (`nowCar < next -> 0`) disparaba frame si frame no.
		static uint32 s_odNextB2 = 0;
		if(s_odNextB2 > nowCar + 60000)
			s_odNextB2 = 0;
		if(nowCar >= s_odNextB2){
			s_odNextB2 = nowCar + 1000;
			float odBetaRel = Beta - TargetOrientation;
			while(odBetaRel >= PI) odBetaRel -= 2.0f * PI;
			while(odBetaRel < -PI) odBetaRel += 2.0f * PI;
			char t3[360];
			float odDist2 = (Source - TargetCoors).Magnitude();
			snprintf(t3, sizeof t3, "CAMB2 brel=%.2f(deg=%.1f) alpha=%.2f dist=%.2f mx=%.3f my=%.3f mag=%.2f raw=%d big=%d win=%d mA=%d stick=%d sup=%d gt=%d rsx=%d rsy=%d idle=%u orbit=%d rot=%d tB=%d beh=%d fro=%d bOff=%.4f aOff=%.4f keyLook=%d ls2=%d rs2=%d base=%.2f obs=%d",
				odBetaRel, RADTODEG(odBetaRel), Alpha, odDist2, MouseX, MouseY,
				odLookMag, (int)rawMouse, (int)odMouseBig,
				(int)(Max(Abs(s_mouseWinX), Abs(s_mouseWinY)) >= 1.5f), (int)odMouseActive,
				(int)stickActive, (int)suppress, (int)pad->GetTarget(),
				(int)pad->NewState.RightStickX, (int)pad->NewState.RightStickY,
				(unsigned)(nowCar - s_lastLookCar), (int)orbiting, (int)Rotating,
				(int)TheCamera.m_bUseTransitionBeta, (int)TheCamera.m_bCamDirectlyBehind,
				(int)TheCamera.m_bCamDirectlyInFront,
				BetaOffset, AlphaOffset, (int)keyLook, (int)odLs2, (int)odRs2, s_odCarAlphaBase, (int)s_odDistObs);
			char t5[240];
			snprintf(t5, sizeof t5, "CAMB2b los=%d losD=%.2f losPed=%d sph=%d sphM=%d sphPed=%d sphOwn=%d dSph=%d dRaw=%.3f nc=%.3f", s_odLosHit, s_odLosD, s_odLosPed, s_odSphHit, s_odSphModel, s_odSphPed, s_odSphOwn, s_odSphApp, s_odSphD, s_odSphNear);
			ODTRACES(t5);
			ODTRACES(t3);
		}
	}
#endif

	// A (ve59): al entrar Beta heredaba la camara a pie (convencion PI distinta) y
	// TransitionBeta la dejaba delante (+PI). Se fuerza detras del rumbo del coche;
	// la interpolacion de Camera (750 ms ped->coche) da la transicion corta sin corte.
	// C4: la transicion pina Beta a TargetOrientation cada frame y pelea con
	// el raton (la "lucha"). Si el jugador mira en ese momento, el pin se
	// libera; si no, entra detras como siempre.
	// C4b (ve69): la transicion NO vuelve a pinar Beta en el coche. Medido en
	// ve68 (01:37:48): con el raton quieto, el pin rearmado por Camera.cpp
	// ("Get into vehicle" 2594 / "Getting out" 2670) ponia Beta = TargetOrientation
	// AL INSTANTE (deg 59 -> 1.7 con idle=666 y rot=0: no era la ley), que es el
	// recentrado inmediato al soltar y la "lucha" mientras el raton se mueve.
	// El pasivo (1,5 s) es el UNICO recentrado; la entrada ya la cubre el hold de
	// 5 frames (Beta=TargetOrientation, Alpha=0) y la interpolacion de Camera.cpp.
	if(TheCamera.m_bUseTransitionBeta){
		TheCamera.m_bUseTransitionBeta = false;
		s_odBetaSnap("pin-liberado");
	}
	// ve73: el hold se arma tambien al ENTRAR (driving false->true), no solo al
	// cambiar de entidad o de modo: bajarse y volver al MISMO vehiculo no disparaba
	// ni ResetStatics ni el cambio de puntero, asi que la camara heredaba el yaw
	// que tuviera y solo el pasivo la volvia a centrar ("empieza a centrarse
	// despues del delay en vez de centrarse apenas me subo al vehiculo").
	{ static CEntity *s_lastCarA = nil; static int s_enterHoldA = 0; static bool s_odWasDrivingA = false; if(ResetStatics || s_lastCarA != CamTargetEntity || (driving && !s_odWasDrivingA)){ s_lastCarA = CamTargetEntity; s_enterHoldA = 5; } s_odWasDrivingA = driving; if(s_enterHoldA > 0){ if(s_enterHoldA == 5) s_odBetaSnap("hold-entrada"); Beta = TargetOrientation; Alpha = 0.0f; BetaSpeed = 0.0f; AlphaSpeed = 0.0f; s_enterHoldA--; } }

	if(TheCamera.m_bCamDirectlyBehind){
		m_bCollisionChecksOn = true;
		s_odBetaSnap("centrar-detras");
		Beta = TargetOrientation;
		Alpha = 0.0f;
		TheCamera.m_bCamDirectlyBehind = false;
	}

	if(TheCamera.m_bCamDirectlyInFront){
		s_odBetaSnap("centrar-delante");
		Beta = TargetOrientation + PI;
		Alpha = 0.0f;
		TheCamera.m_bCamDirectlyInFront = false;
	}

	m_fDistanceBeforeChanges = length;

	Front = CVector(Cos(Alpha) * Cos(Beta), Cos(Alpha) * Sin(Beta), Sin(Alpha));
	Source = TargetCoors - Front * length;

	// RETENIDO (no es seguimiento): el Firetruck sigue al objetivo con el
	// canon de agua parado (serie ~2339-2349; movido tras colocar Source para
	// que la esferica no lo borre; misma matematica).
	if(CamTargetEntity->GetModelIndex() == MI_FIRETRUCK && CPad::GetPad(0)->GetCarGunFired() &&
	   ((CVehicle*)CamTargetEntity)->m_vecMoveSpeed.Magnitude2D() < 0.01f){
		float TargetBeta = CamTargetEntity->GetForward().Heading() - ((CAutomobile*)CamTargetEntity)->m_fCarGunLR + HALFPI;
		TargetBeta = CGeneral::LimitRadianAngle(TargetBeta);
		float DeltaBeta = TargetBeta - Beta;
		if(DeltaBeta > PI) DeltaBeta -= TWOPI;
		else if(DeltaBeta < -PI) DeltaBeta += TWOPI;
		float dist2 = (TargetCoors - Source).Magnitude();
		dist2 = FIRETRUCK_TRACKING_MULT*dist2*Clamp(DeltaBeta, -0.8f, 0.8f);
		Source += dist2*CrossProduct(Front, CVector(0.0f, 0.0f, 1.0f));
	}

	// Agua (serie del coche: suelo en el nivel) + suelo RC (serie SA).
	if(CameraTarget.z >= -2.0f){
		float level = -6000.0f;
		if(CWaterLevel::GetWaterLevelNoWaves(Source.x, Source.y, Source.z, &level)){
			if(Source.z < level)
				Source.z = level;
		}
	}
	if((CamTargetEntity->GetModelIndex() == MI_RCBANDIT || CamTargetEntity->GetModelIndex() == MI_RCBARON) && Source.z < 1.0f)
		Source.z = 1.0f;

	m_cvecTargetCoorsForFudgeInter = TargetCoors;

	// Colisiones CamNew (Process_AvoidCollisions :390-445): LOS + 5 esferas.
	// B3(c) del plan `apuntado-classicaxis-100`: el bloque pasó a ser el miembro
	// Process_AvoidCollisions, que la ley de apuntado reutiliza (cero duplicación).
	// `hideClosePeds = false`: el coche NO esconde los peds a <0,5 m. Refactor puro,
	// mismos números: las trazas `CAMB2b` de un mismo recorrido tienen que dar
	// exactamente los mismos valores antes y después (criterio de no-regresión).
	Process_AvoidCollisions(TargetCoors, length, false);

	Front = TargetCoors - Source;
	Front.Normalise();

	// RETENIDO (no es seguimiento): balanceo/inclinacion por conduccion y
	// tilt de heli (serie ~2368-2420, verbatim).
	{
		int appearance = ((CVehicle*)CamTargetEntity)->GetVehicleAppearance();
		int index = 0;
		TheCamera.GetArrPosForVehicleType(appearance, index);

		if(appearance == VEHICLE_APPEARANCE_HELI){
			float TargetTilt = DotProduct(Front, ((CVehicle*)CamTargetEntity)->GetSpeed(CVector(0.0f, 0.0f, 0.0f)));
			CVector UpTarget = CamTargetEntity->GetUp();
			UpTarget.Normalise();
			int dir = TargetTilt < 0.0f ? -1 : 1;
			if(m_fTilt != 0.0f)
				TargetTilt += TiltOverShoot[index]*TargetTilt/m_fTilt * dir;
			WellBufferMe(TargetTilt, &m_fTilt, &m_fTiltSpeed, TiltTopSpeed[index], TiltSpeedStep[index], false);

			Up = CVector(0.0f, 0.0f, 1.0f) - (CVector(0.0f, 0.0f, 1.0f) - UpTarget)*m_fTilt;
			Up.Normalise();
			Front.Normalise();
			CVector Left = CrossProduct(Up, Front);
			Up = CrossProduct(Front, Left);
			Up.Normalise();
		}else{
			float TargetRoll;
			if(CPad::GetPad(0)->GetDPadLeft() || CPad::GetPad(0)->GetDPadRight()){
				float fwdSpeed = 180.0f*DotProduct(((CVehicle*)CamTargetEntity)->m_vecMoveSpeed, CamTargetEntity->GetForward());
				if(fwdSpeed > 210.0f) fwdSpeed = 210.0f;
				if(CPad::GetPad(0)->GetDPadLeft())
					TargetRoll = DEGTORAD(10.0f)*TiltOverShoot[index] + f_max_role_angle;
				else
					TargetRoll = -(DEGTORAD(10.0f)*TiltOverShoot[index] + f_max_role_angle);
				CVector FwdTarget = CamTargetEntity->GetForward();
				FwdTarget.Normalise();
				float AngleDiff = DotProduct(FwdTarget, Front);
				AngleDiff = Acos(Min(Abs(AngleDiff), 1.0f));
				TargetRoll *= fwdSpeed/210.0f * Sin(AngleDiff);
			}else{
				float fwdSpeed = 180.0f*DotProduct(((CVehicle*)CamTargetEntity)->m_vecMoveSpeed, CamTargetEntity->GetForward());
				if(fwdSpeed > 210.0f) fwdSpeed = 210.0f;
				TargetRoll = CPad::GetPad(0)->GetLeftStickX()/128.0f * fwdSpeed/210.0f;
				CVector FwdTarget = CamTargetEntity->GetForward();
				FwdTarget.Normalise();
				float AngleDiff = DotProduct(FwdTarget, Front);
				AngleDiff = Acos(Min(Abs(AngleDiff), 1.0f));
				TargetRoll *= (DEGTORAD(10.0f)*TiltOverShoot[index] + f_max_role_angle) * Sin(AngleDiff);
			}

			WellBufferMe(TargetRoll, &f_Roll, &f_rollSpeed, 0.15f, 0.07f, false);
			Up = CVector(Cos(f_Roll + HALFPI), 0.0f, Sin(f_Roll + HALFPI));
			Up.Normalise();
			Front.Normalise();
			CVector Left = CrossProduct(Up, Front);
			Left.Normalise();
			Up = CrossProduct(Front, Left);
			Up.Normalise();
		}
	}

#ifdef __EMSCRIPTEN__
	// CAMB3 (ve70): rafaga a 10 Hz durante 4 s tras SOLTAR el raton. Descompone
	// el yaw pieza a pieza (Beta, valor geometrico re-derivado, yaw del Source y
	// el del Front final) para ver si algo que no sea el jugador lo mueve.
	{
		static bool s_odBigPrev = false;
		static uint32 s_odBurstUntil = 0;
		static uint32 s_odNextB3 = 0;
		if(s_odBigPrev && !odMouseBig)
			s_odBurstUntil = nowCar + 4000;
		s_odBigPrev = odMouseBig;
		if(nowCar < s_odBurstUntil && nowCar >= s_odNextB3){
			s_odNextB3 = nowCar + 100;
			float odRel3 = Beta - TargetOrientation;
			while(odRel3 >= PI) odRel3 -= 2.0f * PI;
			while(odRel3 < -PI) odRel3 += 2.0f * PI;
			float odYawSrc = CGeneral::GetATanOfXY(-(Source.x - TargetCoors.x), -(Source.y - TargetCoors.y));
			float odYawFr = CGeneral::GetATanOfXY(Front.x, Front.y);
			char t4[260];
			snprintf(t4, sizeof t4, "CAMB3 deg=%.1f b=%.3f bGeo=%.3f ySrc=%.1f yFr=%.1f dist=%.2f mx=%.2f my=%.2f idle=%u rot=%d tB=%d snap=%d kl=%d ls2=%d rs2=%d a=%.2f",
				RADTODEG(odRel3), Beta, odBetaGeo, RADTODEG(odYawSrc), RADTODEG(odYawFr),
				length, MouseX, MouseY, (unsigned)(nowCar - s_lastLookCar), (int)Rotating,
				(int)TheCamera.m_bUseTransitionBeta, (int)odSnapping, (int)keyLook,
				(int)odLs2, (int)odRs2, Alpha);
			ODTRACES(t4);
			char t6[240];
			snprintf(t6, sizeof t6, "CAMB3b los=%d losD=%.2f losPed=%d sph=%d sphM=%d sphPed=%d sphOwn=%d dSph=%d dRaw=%.3f nc=%.3f", s_odLosHit, s_odLosD, s_odLosPed, s_odSphHit, s_odSphModel, s_odSphPed, s_odSphOwn, s_odSphApp, s_odSphD, s_odSphNear);
			ODTRACES(t6);
		}
	}
	{
		static uint32 s_odNextCamsa = 0;
		uint32 odNow = CTimer::GetTimeInMilliseconds();
		if (odNow < s_odNextCamsa && odNow + 60000 >= s_odNextCamsa) {}
		else {
			s_odNextCamsa = odNow + 1000;
			CVector odD = Source - TargetCoors;
			char t[120];
			snprintf(t, sizeof t, "CAMSA modo=%d dist=%.2f altura=%.2f",
				(int)Mode, odD.Magnitude(), Source.z - TargetCoors.z);
			ODTRACES(t);
			char t2[120];
			snprintf(t2, sizeof t2, "CAMV2 modo=%d dist=%.2f altura=%.2f",
				(int)Mode, odD.Magnitude(), Source.z - TargetCoors.z);
			ODTRACES(t2);
		}
	}
#endif

	ResetStatics = false;

}

// Basic Cam on a string algorithm
void
CCam::Cam_On_A_String_Unobscured(const CVector &TargetCoors, float BaseDist)
{
	int id = CamTargetEntity->GetModelIndex();
	float ExtraDist = 0.0f;
	if(id == MI_RCRAIDER || id == MI_RCGOBLIN)
		ExtraDist = INIT_RC_HELI_HORI_EXTRA;
	else if(id == MI_RCBARON)
		ExtraDist = INIT_RC_PLANE_HORI_EXTRA;

	CA_MAX_DISTANCE = BaseDist + 0.1f + TheCamera.CarZoomValueSmooth + ExtraDist;
	CA_MIN_DISTANCE = Min(BaseDist*0.6f, 3.5f);
	if(CA_MIN_DISTANCE > CA_MAX_DISTANCE)
		CA_MIN_DISTANCE = CA_MAX_DISTANCE - 0.05f;

	CVector Dist = Source - TargetCoors;

	if(ResetStatics)
		Source = TargetCoors + Dist*(CA_MAX_DISTANCE + 1.0f);

	Dist = Source - TargetCoors;

	float Length = Dist.Magnitude2D();
	if(Length < 0.001f){
		// This probably shouldn't happen. reset view
		CVector Forward = CamTargetEntity->GetForward();
		Forward.z = 0.0f;
		Forward.Normalise();
		Source = TargetCoors - Forward*CA_MAX_DISTANCE;
		Dist = Source - TargetCoors;
		Length = Dist.Magnitude2D();
	}

	if(Length > CA_MAX_DISTANCE){
		Source.x = TargetCoors.x + Dist.x/Length * CA_MAX_DISTANCE;
		Source.y = TargetCoors.y + Dist.y/Length * CA_MAX_DISTANCE;
	}else if(Length < CA_MIN_DISTANCE){
		Source.x = TargetCoors.x + Dist.x/Length * CA_MIN_DISTANCE;
		Source.y = TargetCoors.y + Dist.y/Length * CA_MIN_DISTANCE;
	}
}

void
CCam::FixCamWhenObscuredByVehicle(const CVector &TargetCoors)
{
	// BUG? is this never reset
	static float HeightFixerCarsObscuring = 0.0f;
	static float HeightFixerCarsObscuringSpeed = 0.0f;
	CColPoint colPoint;
	CEntity *entity = nil;

	float HeightTarget = 0.0f;
	if(CWorld::ProcessLineOfSight(TargetCoors, Source, colPoint, entity, false, true, false, false, false, false, false)){
		CBaseModelInfo *mi = CModelInfo::GetModelInfo(entity->GetModelIndex());
		HeightTarget = mi->GetColModel()->boundingBox.max.z + 1.0f + TargetCoors.z - Source.z;
		if(HeightTarget < 0.0f)
			HeightTarget = 0.0f;
	}
	WellBufferMe(HeightTarget, &HeightFixerCarsObscuring, &HeightFixerCarsObscuringSpeed, 0.2f, 0.025f, false);
	Source.z += HeightFixerCarsObscuring;
}

void
CCam::Process_TopDown(const CVector &CameraTarget, float TargetOrientation, float SpeedVar, float TargetSpeedVar)
{
	FOV = DefaultFOV;

	if(!CamTargetEntity->IsVehicle())
		return;

	float Dist;
	float HeightTarget = 0.0f;
	static float AdjustHeightTargetMoveBuffer = 0.0f;
	static float AdjustHeightTargetMoveSpeed = 0.0f;
	static float NearClipDistance = 1.5f;
	const float FarClipDistance = 200.0f;
	CVector TargetFront, Target;
	CVector TestSource, TestTarget;
	CColPoint colPoint;
	CEntity *entity;

	TargetFront = CameraTarget;
	TargetFront.x += 18.0f*CamTargetEntity->GetForward().x*SpeedVar;
	TargetFront.y += 18.0f*CamTargetEntity->GetForward().y*SpeedVar;

	if(ResetStatics){
		AdjustHeightTargetMoveBuffer = 0.0f;
		AdjustHeightTargetMoveSpeed = 0.0f;
	}

	float f = Pow(0.8f, 4.0f);
	Target = f*CameraTarget + (1.0f-f)*TargetFront;
	if(Mode == MODE_GTACLASSIC)
		SpeedVar = TargetSpeedVar;
	Source = Target + CVector(0.0f, 0.0f, (40.0f*SpeedVar + 30.0f)*0.8f);
	// What is this? looks horrible
	if(Mode == MODE_GTACLASSIC)
		Source.x += (uint8)(100.0f*CameraTarget.x)/500.0f;

	TestSource = Source;
	TestTarget = TestSource;
	TestTarget.z = Target.z;
	if(CWorld::ProcessLineOfSight(TestTarget, TestSource, colPoint, entity, true, false, false, false, false, false, false)){
		if(Source.z < colPoint.point.z+3.0f)
			HeightTarget = colPoint.point.z+3.0f - Source.z;
	}else{
		TestSource = Source;
		TestTarget = TestSource;
		TestTarget.z += 10.0f;
		if(CWorld::ProcessLineOfSight(TestTarget, TestSource, colPoint, entity, true, false, false, false, false, false, false))
			if(Source.z < colPoint.point.z+3.0f)
				HeightTarget = colPoint.point.z+3.0f - Source.z;
	}
	WellBufferMe(HeightTarget, &AdjustHeightTargetMoveBuffer, &AdjustHeightTargetMoveSpeed, 0.2f, 0.02f, false);
	Source.z += AdjustHeightTargetMoveBuffer;

	if(RwCameraGetFarClipPlane(Scene.camera) > FarClipDistance)
		RwCameraSetFarClipPlane(Scene.camera, FarClipDistance);
	RwCameraSetNearClipPlane(Scene.camera, NearClipDistance);

	Front = CVector(-0.01f, -0.01f, -1.0f);	// look down
	Front.Normalise();
	Dist = (Source - CameraTarget).Magnitude();
	m_cvecTargetCoorsForFudgeInter = Dist*Front + Source;
	Up = CVector(0.0f, 1.0f, 0.0f);

	ResetStatics = false;
}

void
CCam::AvoidWallsTopDownPed(const CVector &TargetCoors, const CVector &Offset, float *Adjuster, float *AdjusterSpeed, float yDistLimit)
{
	float Target = 0.0f;
	float MaxSpeed = 0.13f;
	float Acceleration = 0.015f;
	float SpeedMult;
	float dy;
	CVector TestPoint2;
	CVector TestPoint1;
	CColPoint colPoint;
	CEntity *entity;

	TestPoint2 = TargetCoors + Offset;
	TestPoint1 = TargetCoors;
	TestPoint1.z = TestPoint2.z;
	if(CWorld::ProcessLineOfSight(TestPoint1, TestPoint2, colPoint, entity, true, false, false, false, false, false, false)){
		// What is this even?
		dy = TestPoint1.y - colPoint.point.y;
		if(dy > yDistLimit)
			dy = yDistLimit;
		SpeedMult = yDistLimit - Abs(dy/yDistLimit);

		Target = 2.5f;
		MaxSpeed += SpeedMult*0.3f;
		Acceleration += SpeedMult*0.03f;
	}
	WellBufferMe(Target, Adjuster, AdjusterSpeed, MaxSpeed, Acceleration, false);
}

void
CCam::Process_TopDownPed(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	if(!CamTargetEntity->IsPed())
		return;

	float Dist;
	float HeightTarget;
	static int NumPedPosCountsSoFar = 0;
	static float PedAverageSpeed = 0.0f;
	static float AdjustHeightTargetMoveBuffer = 0.0f;
	static float AdjustHeightTargetMoveSpeed = 0.0f;
	static float PedSpeedSoFar = 0.0f;
	static float FarClipDistance = 200.0f;
	static float NearClipDistance = 1.5f;
	static float TargetAdjusterForSouth = 0.0f;
	static float TargetAdjusterSpeedForSouth = 0.0f;
	static float TargetAdjusterForNorth = 0.0f;
	static float TargetAdjusterSpeedForNorth = 0.0f;
	static float TargetAdjusterForEast = 0.0f;
	static float TargetAdjusterSpeedForEast = 0.0f;
	static float TargetAdjusterForWest = 0.0f;
	static float TargetAdjusterSpeedForWest = 0.0f;
	static CVector PreviousPlayerMoveSpeedVec;
	CVector TargetCoors, PlayerMoveSpeed;
	CVector TestSource, TestTarget;
	CColPoint colPoint;
	CEntity *entity;

	FOV = DefaultFOV;
	TargetCoors = CameraTarget;
	PlayerMoveSpeed = ((CPed*)CamTargetEntity)->GetMoveSpeed();

	if(ResetStatics){
		PreviousPlayerMoveSpeedVec = PlayerMoveSpeed;
		AdjustHeightTargetMoveBuffer = 0.0f;
		AdjustHeightTargetMoveSpeed = 0.0f;
		NumPedPosCountsSoFar = 0;
		PedSpeedSoFar = 0.0f;
		PedAverageSpeed = 0.0f;
		TargetAdjusterForWest = 0.0f;
		TargetAdjusterSpeedForWest = 0.0f;
		TargetAdjusterForEast = 0.0f;
		TargetAdjusterSpeedForEast = 0.0f;
		TargetAdjusterForNorth = 0.0f;
		TargetAdjusterSpeedForNorth = 0.0f;
		TargetAdjusterForSouth = 0.0f;
		TargetAdjusterSpeedForSouth = 0.0f;
	}

	if(RwCameraGetFarClipPlane(Scene.camera) > FarClipDistance)
		RwCameraSetFarClipPlane(Scene.camera, FarClipDistance);
	RwCameraSetNearClipPlane(Scene.camera, NearClipDistance);

	// Average ped speed
	NumPedPosCountsSoFar++;
	PedSpeedSoFar += PlayerMoveSpeed.Magnitude();
	if(NumPedPosCountsSoFar == 5){
		PedAverageSpeed = 0.4f*PedAverageSpeed + 0.6*(PedSpeedSoFar/5.0f);
		NumPedPosCountsSoFar = 0;
		PedSpeedSoFar = 0.0f;
	}
	PreviousPlayerMoveSpeedVec = PlayerMoveSpeed;

	// Zoom out depending on speed
	if(PedAverageSpeed > 0.01f && PedAverageSpeed <= 0.04f)
		HeightTarget = 2.5f;
	else if(PedAverageSpeed > 0.04f && PedAverageSpeed <= 0.145f)
		HeightTarget = 4.5f;
	else if(PedAverageSpeed > 0.145f)
		HeightTarget = 7.0f;
	else
		HeightTarget = 0.0f;

	// Zoom out if locked on target is far away
	if(FindPlayerPed()->m_pPointGunAt){
		Dist = (FindPlayerPed()->m_pPointGunAt->GetPosition() - CameraTarget).Magnitude2D();
		if(Dist > 6.0f)
			HeightTarget = Max(HeightTarget, Dist/22.0f*37.0f);
	}

	Source = TargetCoors + CVector(0.0f, -1.0f, 9.0f);

	// Collision checks
	entity = nil;
	TestSource = TargetCoors + CVector(0.0f, -1.0f, 9.0f);
	TestTarget = TestSource;
	TestTarget.z = TargetCoors.z;
	if(CWorld::ProcessLineOfSight(TestTarget, TestSource, colPoint, entity, true, false, false, false, false, false, false)){
		if(TargetCoors.z+9.0f+HeightTarget < colPoint.point.z+3.0f)
			HeightTarget = colPoint.point.z+3.0f - (TargetCoors.z+9.0f);
	}else{
		TestSource = TargetCoors + CVector(0.0f, -1.0f, 9.0f);
		TestTarget = TestSource;
		TestSource.z += HeightTarget;
		TestTarget.z = TestSource.z + 10.0f;
		if(CWorld::ProcessLineOfSight(TestTarget, TestSource, colPoint, entity, true, false, false, false, false, false, false)){
			if(TargetCoors.z+9.0f+HeightTarget < colPoint.point.z+3.0f)
				HeightTarget = colPoint.point.z+3.0f - (TargetCoors.z+9.0f);
		}
	}

	WellBufferMe(HeightTarget, &AdjustHeightTargetMoveBuffer, &AdjustHeightTargetMoveSpeed, 0.3f, 0.03f, false);
	Source.z += AdjustHeightTargetMoveBuffer;

	// Wall checks
	AvoidWallsTopDownPed(TargetCoors, CVector(0.0f, -3.0f, 3.0f), &TargetAdjusterForSouth, &TargetAdjusterSpeedForSouth, 1.0f);
	Source.y += TargetAdjusterForSouth;
	AvoidWallsTopDownPed(TargetCoors, CVector(0.0f, 3.0f, 3.0f), &TargetAdjusterForNorth, &TargetAdjusterSpeedForNorth, 1.0f);
	Source.y -= TargetAdjusterForNorth;
	// BUG: east and west flipped
	AvoidWallsTopDownPed(TargetCoors, CVector(3.0f, 0.0f, 3.0f), &TargetAdjusterForWest, &TargetAdjusterSpeedForWest, 1.0f);
	Source.x -= TargetAdjusterForWest;
	AvoidWallsTopDownPed(TargetCoors, CVector(-3.0f, 0.0f, 3.0f), &TargetAdjusterForEast, &TargetAdjusterSpeedForEast, 1.0f);
	Source.x += TargetAdjusterForEast;

	TargetCoors.y = Source.y + 1.0f;
	TargetCoors.y += TargetAdjusterForSouth;
	TargetCoors.x += TargetAdjusterForEast;
	TargetCoors.x -= TargetAdjusterForWest;

	Front = TargetCoors - Source;
	Front.Normalise();
#ifdef FIX_BUGS
	if(Front.x == 0.0f && Front.y == 0.0f)
		Front.y = 0.0001f;
#else
	// someone used = instead of == in the above check by accident
	Front.x = 0.0f;
#endif
	m_cvecTargetCoorsForFudgeInter = TargetCoors;
	Up = CrossProduct(Front, CVector(-1.0f, 0.0f, 0.0f));
	Up.Normalise();

	ResetStatics = false;
}

void
CCam::Process_Rocket(const CVector &CameraTarget, float, float, float)
{
#ifdef VICEEXT_RECOIL
	bool recoilReset = ResetStatics;
#endif
	if(!CamTargetEntity->IsPed())
		return;

	float BackOffset = 0.19f;
	static bool FailedTestTwelveFramesAgo = false;
	RwV3d HeadPos;
	CVector TargetCoors;

	FOV = DefaultFOV;
	TargetCoors = CameraTarget;

	if(ResetStatics){
		Beta = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
		Alpha = 0.0f;
		m_fInitialPlayerOrientation = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
		FailedTestTwelveFramesAgo = false;
		// static DPadVertical unused
		// static DPadHorizontal unused
		m_bCollisionChecksOn = true;
		ResetStatics = false;
	}

	if(((CPed*)CamTargetEntity)->bIsDucking)
		BackOffset = 0.8f;
	CamTargetEntity->GetMatrix().UpdateRW();
	CamTargetEntity->UpdateRwFrame();
	CamTargetEntity->UpdateRpHAnim();
	((CPed*)CamTargetEntity)->m_pedIK.GetComponentPosition(HeadPos, PED_HEAD);
	Source = HeadPos;
	Source.z += 0.1f;
	Source.x -= BackOffset*Cos(m_fInitialPlayerOrientation);
	Source.y -= BackOffset*Sin(m_fInitialPlayerOrientation);

	// Look around
	bool UseMouse = false;
	float MouseX = CPad::GetPad(0)->GetMouseX();
	float MouseY = CPad::GetPad(0)->GetMouseY();
	float LookLeftRight, LookUpDown;
#ifdef VICEEXT_RECOIL
	float recoilManualAlphaStart = Alpha;
#endif
	if(MouseX != 0.0f || MouseY != 0.0f){
		UseMouse = true;
		LookLeftRight = -3.0f*MouseX;
		LookUpDown = 4.0f*MouseY;
	}else{
		LookLeftRight = -CPad::GetPad(0)->SniperModeLookLeftRight();
		LookUpDown = CPad::GetPad(0)->SniperModeLookUpDown();
	}
	if(UseMouse){
		Beta += TheCamera.m_fMouseAccelHorzntl * LookLeftRight * FOV/80.0f;
		Alpha += TheCamera.m_fMouseAccelVertical * LookUpDown * FOV/80.0f;
	}else{
		float xdir = LookLeftRight < 0.0f ? -1.0f : 1.0f;
		float ydir = LookUpDown < 0.0f ? -1.0f : 1.0f;
		Beta += SQR(LookLeftRight/100.0f)*xdir*0.8f/14.0f * FOV/80.0f * CTimer::GetTimeStep();
		Alpha += SQR(LookUpDown/150.0f)*ydir*1.0f/14.0f * FOV/80.0f * CTimer::GetTimeStep();
	}
	while(Beta >= PI) Beta -= 2*PI;
	while(Beta < -PI) Beta += 2*PI;
	if(Alpha > DEGTORAD(60.0f)) Alpha = DEGTORAD(60.0f);
	else if(Alpha < -DEGTORAD(89.5f)) Alpha = -DEGTORAD(89.5f);
#ifdef VICEEXT_RECOIL
	CWeapon::ViceExtRecoilBegin(Alpha, recoilReset, Mode, "rocket");
	CWeapon::ViceExtRecoilApply(Alpha, Alpha - recoilManualAlphaStart, LookUpDown, UseMouse ? "mouse" : "pad", Mode,
		-DEGTORAD(89.5f), DEGTORAD(60.0f));
#endif

	TargetCoors.x = 3.0f * Cos(Alpha) * Cos(Beta) + Source.x;
	TargetCoors.y = 3.0f * Cos(Alpha) * Sin(Beta) + Source.y;
	TargetCoors.z = 3.0f * Sin(Alpha) + Source.z;
	Front = TargetCoors - Source;
	Front.Normalise();
	Source += Front*0.4f;

	if(m_bCollisionChecksOn){
		if(!CWorld::GetIsLineOfSightClear(TargetCoors, Source, true, true, false, true, false, true, true)){
			RwCameraSetNearClipPlane(Scene.camera, 0.4f);
			FailedTestTwelveFramesAgo = true;
		}else{
			CVector TestPoint;
			TestPoint.x = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Cos(Beta + DEGTORAD(35.0f)) + Source.x;
			TestPoint.y = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Sin(Beta + DEGTORAD(35.0f)) + Source.y;
			TestPoint.z = 3.0f * Sin(Alpha - DEGTORAD(20.0f)) + Source.z;
			if(!CWorld::GetIsLineOfSightClear(TestPoint, Source, true, true, false, true, false, true, true)){
				RwCameraSetNearClipPlane(Scene.camera, 0.4f);
				FailedTestTwelveFramesAgo = true;
			}else{
				TestPoint.x = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Cos(Beta - DEGTORAD(35.0f)) + Source.x;
				TestPoint.y = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Sin(Beta - DEGTORAD(35.0f)) + Source.y;
				TestPoint.z = 3.0f * Sin(Alpha - DEGTORAD(20.0f)) + Source.z;
				if(!CWorld::GetIsLineOfSightClear(TestPoint, Source, true, true, false, true, false, true, true)){
					RwCameraSetNearClipPlane(Scene.camera, 0.4f);
					FailedTestTwelveFramesAgo = true;
				}else
					FailedTestTwelveFramesAgo = false;
			}
		}
	}

	if(FailedTestTwelveFramesAgo)
		RwCameraSetNearClipPlane(Scene.camera, 0.4f);
	Source -= Front*0.4f;

	GetVectorsReadyForRW();
	float Rotation = CGeneral::GetATanOfXY(Front.x, Front.y) - HALFPI;
	((CPed*)TheCamera.pTargetEntity)->m_fRotationCur = Rotation;
	((CPed*)TheCamera.pTargetEntity)->m_fRotationDest = Rotation;
}

float fDuckingBackOffset = 0.5f;
float fDuckingRightOffset = 0.18f;

void
CCam::Process_M16_1stPerson(const CVector &CameraTarget, float, float, float)
{
	if(!CamTargetEntity->IsPed())
		return;

	float BackOffset = 0.3f;
	static bool FailedTestTwelveFramesAgo = false;
	RwV3d HeadPos;
	CVector TargetCoors;

	bool isAttached = ((CPed*)CamTargetEntity)->IsPlayer() && ((CPed*)CamTargetEntity)->m_attachedTo;

	FOV = DefaultFOV;
	TargetCoors = CameraTarget;
#ifdef VICEEXT_RECOIL
	bool recoilReset = ResetStatics;
#endif

	if(ResetStatics){
		if(isAttached)
			Beta = 0.0f;
		else
			Beta = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
		Alpha = 0.0f;
		m_fInitialPlayerOrientation = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
		FailedTestTwelveFramesAgo = false;
		// static DPadVertical unused
		// static DPadHorizontal unused
		m_bCollisionChecksOn = true;
		ResetStatics = false;
	}

	// Look around
	bool UseMouse = false;
	float MouseX = CPad::GetPad(0)->GetMouseX();
	float MouseY = CPad::GetPad(0)->GetMouseY();
	float LookLeftRight, LookUpDown;
	if(MouseX != 0.0f || MouseY != 0.0f){
		UseMouse = true;
		LookLeftRight = -3.0f*MouseX;
		LookUpDown = 4.0f*MouseY;
	}else{
		LookLeftRight = -CPad::GetPad(0)->SniperModeLookLeftRight();
		LookUpDown = CPad::GetPad(0)->SniperModeLookUpDown();
	}
#ifdef VICEEXT_RECOIL
	float recoilManualAlphaStart = Alpha;
#endif
	if(UseMouse){
		Beta += TheCamera.m_fMouseAccelHorzntl * LookLeftRight * FOV/80.0f;
		Alpha += TheCamera.m_fMouseAccelVertical * LookUpDown * FOV/80.0f;
	}else if(Mode == MODE_HELICANNON_1STPERSON){
		LookLeftRight /= 128.0f;
		LookUpDown /= 128.0f;
		Beta += LookLeftRight*Abs(LookLeftRight)*0.56f/14.0f * FOV/80.0f * CTimer::GetTimeStep();
		Alpha += LookUpDown*Abs(LookUpDown)*0.48f/14.0f * FOV/80.0f * CTimer::GetTimeStep();
	}else{
		float xdir = LookLeftRight < 0.0f ? -1.0f : 1.0f;
		float ydir = LookUpDown < 0.0f ? -1.0f : 1.0f;
		Beta += SQR(LookLeftRight/100.0f)*xdir*0.8f/14.0f * FOV/80.0f * CTimer::GetTimeStep();
		Alpha += SQR(LookUpDown/150.0f)*ydir*1.0f/14.0f * FOV/80.0f * CTimer::GetTimeStep();
	}
	if (!isAttached) {
		while(Beta >= TWOPI) Beta -= TWOPI;
		while(Beta < 0) Beta += TWOPI;
	}
#ifdef VICEEXT_RECOIL
	CWeapon::ViceExtRecoilBegin(Alpha, recoilReset, Mode, "first-person-weapon");
	CWeapon::ViceExtRecoilApply(Alpha, Alpha - recoilManualAlphaStart, LookUpDown, UseMouse ? "mouse" : "pad", Mode,
		-DEGTORAD(89.5f), DEGTORAD(60.0f));
#endif
	if(Alpha > DEGTORAD(60.0f)) Alpha = DEGTORAD(60.0f);
	else if(Alpha < -DEGTORAD(89.5f)) Alpha = -DEGTORAD(89.5f);

	if(((CPed*)CamTargetEntity)->bIsDucking)
		BackOffset = 0.8f;
	if(isAttached){
		CMatrix mat, rot;
		CPed *TargetPed = (CPed*)CamTargetEntity;
		TargetPed->PositionAttachedPed();
		CamTargetEntity->GetMatrix().UpdateRW();
		CamTargetEntity->UpdateRwFrame();
		CamTargetEntity->UpdateRpHAnim();

		HeadPos.x = 0.0f;
		HeadPos.y = 0.0f;
		HeadPos.z = 0.0f;
		TargetPed->m_pedIK.GetComponentPosition(HeadPos, PED_HEAD);
		Source = HeadPos;
		Source += 0.1f*CamTargetEntity->GetUp();
		Source -= BackOffset*CamTargetEntity->GetForward();

		if(TargetPed->m_attachRotStep < PI){
			if(Beta > TargetPed->m_attachRotStep){
				Beta = TargetPed->m_attachRotStep;
				CAutomobile *heli = (CAutomobile*)TargetPed->m_attachedTo;
				if(heli->IsVehicle() && heli->IsCar() && heli->IsRealHeli() && heli->m_fHeliOrientation > 0.0f){
					float heliOrient = heli->m_fHeliOrientation + CTimer::GetTimeStep()*0.01f;
					if(heliOrient < 0.0f) heliOrient += TWOPI;
					else if(heliOrient > TWOPI) heliOrient -= TWOPI;
					heli->SetHeliOrientation(heliOrient);
				}
			}else if(Beta < -TargetPed->m_attachRotStep){
				Beta = -TargetPed->m_attachRotStep;
				CAutomobile *heli = (CAutomobile*)TargetPed->m_attachedTo;
				if(heli->IsVehicle() && heli->IsCar() && heli->IsRealHeli() && heli->m_fHeliOrientation > 0.0f){
					float heliOrient = heli->m_fHeliOrientation - CTimer::GetTimeStep()*0.01f;
					if(heliOrient < 0.0f) heliOrient += TWOPI;
					else if(heliOrient > TWOPI) heliOrient -= TWOPI;
					heli->SetHeliOrientation(heliOrient);
				}
			}
		}else{
			while(Beta < -PI) Beta += TWOPI;
			while(Beta >= PI) Beta -= TWOPI;
		}

		mat = TargetPed->m_attachedTo->GetMatrix();
		rot.SetRotateX(Alpha);
		switch(TargetPed->m_attachType){
		case 0: rot.RotateZ(Beta); break;
		case 1: rot.RotateZ(Beta + HALFPI); break;
		case 2: rot.RotateZ(Beta + PI); break;
		case 3: rot.RotateZ(Beta - HALFPI); break;
		}
		mat = mat * rot;
		Front = mat.GetForward();
		Up = mat.GetUp();
		TargetCoors = Source + 3.0f*Front;
		RwCameraSetNearClipPlane(Scene.camera, 0.4f);

		float Rotation = CGeneral::GetATanOfXY(Front.x, Front.y) - HALFPI;
		((CPed*)TheCamera.pTargetEntity)->m_fRotationCur = Rotation;
		((CPed*)TheCamera.pTargetEntity)->m_fRotationDest = Rotation;
	}else{
		CamTargetEntity->GetMatrix().UpdateRW();
		CamTargetEntity->UpdateRwFrame();
		CamTargetEntity->UpdateRpHAnim();
		HeadPos.x = 0.0f;
		HeadPos.y = 0.0f;
		HeadPos.z = 0.0f;
		((CPed*)CamTargetEntity)->m_pedIK.GetComponentPosition(HeadPos, PED_HEAD);
		Source = HeadPos;
		Source.z += 0.1f;
		if(((CPed*)CamTargetEntity)->bIsDucking){
			Source.x -= fDuckingBackOffset*CamTargetEntity->GetForward().x;
			Source.y -= fDuckingBackOffset*CamTargetEntity->GetForward().y;
			Source.x -= fDuckingRightOffset*CamTargetEntity->GetRight().x;
			Source.y -= fDuckingRightOffset*CamTargetEntity->GetRight().y;
		}else{
			Source.x -= BackOffset*CamTargetEntity->GetForward().x;
			Source.y -= BackOffset*CamTargetEntity->GetForward().y;
		}

		TargetCoors.x = 3.0f * Cos(Alpha) * Cos(Beta) + Source.x;
		TargetCoors.y = 3.0f * Cos(Alpha) * Sin(Beta) + Source.y;
		TargetCoors.z = 3.0f * Sin(Alpha) + Source.z;
		Front = TargetCoors - Source;
		Front.Normalise();
		Source += Front*0.4f;

		if(m_bCollisionChecksOn){
			if(!CWorld::GetIsLineOfSightClear(TargetCoors, Source, true, true, false, true, false, true, true)){
				RwCameraSetNearClipPlane(Scene.camera, 0.4f);
				FailedTestTwelveFramesAgo = true;
			}else{
				CVector TestPoint;
				TestPoint.x = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Cos(Beta + DEGTORAD(35.0f)) + Source.x;
				TestPoint.y = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Sin(Beta + DEGTORAD(35.0f)) + Source.y;
				TestPoint.z = 3.0f * Sin(Alpha - DEGTORAD(20.0f)) + Source.z;
				if(!CWorld::GetIsLineOfSightClear(TestPoint, Source, true, true, false, true, false, true, true)){
					RwCameraSetNearClipPlane(Scene.camera, 0.4f);
					FailedTestTwelveFramesAgo = true;
				}else{
					TestPoint.x = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Cos(Beta - DEGTORAD(35.0f)) + Source.x;
					TestPoint.y = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Sin(Beta - DEGTORAD(35.0f)) + Source.y;
					TestPoint.z = 3.0f * Sin(Alpha - DEGTORAD(20.0f)) + Source.z;
					if(!CWorld::GetIsLineOfSightClear(TestPoint, Source, true, true, false, true, false, true, true)){
						RwCameraSetNearClipPlane(Scene.camera, 0.4f);
						FailedTestTwelveFramesAgo = true;
					}else
						FailedTestTwelveFramesAgo = false;
				}
			}
		}

		if(FailedTestTwelveFramesAgo)
			RwCameraSetNearClipPlane(Scene.camera, 0.4f);
		Source -= Front*0.4f;

		GetVectorsReadyForRW();
		float Rotation = CGeneral::GetATanOfXY(Front.x, Front.y) - HALFPI;
		((CPed*)TheCamera.pTargetEntity)->m_fRotationCur = Rotation;
		((CPed*)TheCamera.pTargetEntity)->m_fRotationDest = Rotation;
	}
}

float fBike1stPersonOffsetZ = 0.15f;

void
CCam::Process_1stPerson(const CVector &CameraTarget, float TargetOrientation, float SpeedVar, float TargetSpeedVar)
{
#ifdef VICEEXT_RECOIL
	bool recoilReset = ResetStatics;
#endif
	float BackOffset = 0.3f;
	static float DontLookThroughWorldFixer = 0.0f;
	CVector TargetCoors;

	FOV = DefaultFOV;
	TargetCoors = CameraTarget;
	if(CamTargetEntity->m_rwObject == nil)
		return;

	if(ResetStatics){
		Beta = TargetOrientation;
		Alpha = 0.0f;
		m_fInitialPlayerOrientation = TargetOrientation;
		if(CamTargetEntity->IsPed()){
			Beta = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
			Alpha = 0.0f;
			m_fInitialPlayerOrientation = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
		}
		TheCamera.m_fAvoidTheGeometryProbsTimer = 0.0f;
		DontLookThroughWorldFixer = 0.0f;
	}

	if(CamTargetEntity->IsPed()){
		static bool FailedTestTwelveFramesAgo = false;
		RwV3d HeadPos;

		TargetCoors = CameraTarget;

		if(ResetStatics){
			Beta = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
			Alpha = 0.0f;
			m_fInitialPlayerOrientation = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
			FailedTestTwelveFramesAgo = false;
			// static DPadVertical unused
			// static DPadHorizontal unused
			m_bCollisionChecksOn = true;
			ResetStatics = false;
		}
		CamTargetEntity->GetMatrix().UpdateRW();
		CamTargetEntity->UpdateRwFrame();
		CamTargetEntity->UpdateRpHAnim();

		((CPed*)CamTargetEntity)->m_pedIK.GetComponentPosition(HeadPos, PED_HEAD);
		Source = HeadPos;
		Source.z += 0.1f;
		if(((CPed*)CamTargetEntity)->bIsDucking){
			Source.x -= fDuckingBackOffset*CamTargetEntity->GetForward().x;
			Source.y -= fDuckingBackOffset*CamTargetEntity->GetForward().y;
			Source.x -= fDuckingRightOffset*CamTargetEntity->GetRight().x;
			Source.y -= fDuckingRightOffset*CamTargetEntity->GetRight().y;
		}else{
			Source.x -= BackOffset*CamTargetEntity->GetForward().x;
			Source.y -= BackOffset*CamTargetEntity->GetForward().y;
		}
		float LookLeftRight, LookUpDown;
		LookLeftRight = -CPad::GetPad(0)->LookAroundLeftRight();
		LookUpDown = CPad::GetPad(0)->LookAroundUpDown();
#ifdef VICEEXT_RECOIL
		float recoilManualAlphaStart = Alpha;
#endif
		float xdir = LookLeftRight < 0.0f ? -1.0f : 1.0f;
		float ydir = LookUpDown < 0.0f ? -1.0f : 1.0f;
		Beta += SQR(LookLeftRight/100.0f)*xdir*0.8f/14.0f * FOV/80.0f * CTimer::GetTimeStep();
		Alpha += SQR(LookUpDown/150.0f)*ydir*1.0f/14.0f * FOV/80.0f * CTimer::GetTimeStep();
		while(Beta >= PI) Beta -= 2*PI;
		while(Beta < -PI) Beta += 2*PI;
#ifdef VICEEXT_RECOIL
		CWeapon::ViceExtRecoilBegin(Alpha, recoilReset, Mode, "first-person");
		CWeapon::ViceExtRecoilApply(Alpha, Alpha - recoilManualAlphaStart, LookUpDown, "pad", Mode,
			-DEGTORAD(89.5f), DEGTORAD(60.0f));
#endif
		if(Alpha > DEGTORAD(60.0f)) Alpha = DEGTORAD(60.0f);
		else if(Alpha < -DEGTORAD(89.5f)) Alpha = -DEGTORAD(89.5f);

		TargetCoors.x = 3.0f * Cos(Alpha) * Cos(Beta) + Source.x;
		TargetCoors.y = 3.0f * Cos(Alpha) * Sin(Beta) + Source.y;
		TargetCoors.z = 3.0f * Sin(Alpha) + Source.z;
		Front = TargetCoors - Source;
		Front.Normalise();
		Source += Front*0.4f;

		if(m_bCollisionChecksOn){
			if(!CWorld::GetIsLineOfSightClear(TargetCoors, Source, true, true, false, true, false, true, true)){
				RwCameraSetNearClipPlane(Scene.camera, 0.4f);
				FailedTestTwelveFramesAgo = true;
			}else{
				CVector TestPoint;
				TestPoint.x = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Cos(Beta + DEGTORAD(35.0f)) + Source.x;
				TestPoint.y = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Sin(Beta + DEGTORAD(35.0f)) + Source.y;
				TestPoint.z = 3.0f * Sin(Alpha - DEGTORAD(20.0f)) + Source.z;
				if(!CWorld::GetIsLineOfSightClear(TestPoint, Source, true, true, false, true, false, true, true)){
					RwCameraSetNearClipPlane(Scene.camera, 0.4f);
					FailedTestTwelveFramesAgo = true;
				}else{
					TestPoint.x = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Cos(Beta - DEGTORAD(35.0f)) + Source.x;
					TestPoint.y = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Sin(Beta - DEGTORAD(35.0f)) + Source.y;
					TestPoint.z = 3.0f * Sin(Alpha - DEGTORAD(20.0f)) + Source.z;
					if(!CWorld::GetIsLineOfSightClear(TestPoint, Source, true, true, false, true, false, true, true)){
						RwCameraSetNearClipPlane(Scene.camera, 0.4f);
						FailedTestTwelveFramesAgo = true;
					}else
						FailedTestTwelveFramesAgo = false;
				}
			}
		}

		if(FailedTestTwelveFramesAgo)
			RwCameraSetNearClipPlane(Scene.camera, 0.4f);
		Source -= Front*0.4f;

		GetVectorsReadyForRW();
		float Rotation = CGeneral::GetATanOfXY(Front.x, Front.y) - HALFPI;
		((CPed*)TheCamera.pTargetEntity)->m_fRotationCur = Rotation;
		((CPed*)TheCamera.pTargetEntity)->m_fRotationDest = Rotation;
	}else{
		assert(CamTargetEntity->IsVehicle());

		if(((CVehicle*)CamTargetEntity)->IsBike() &&
		   (((CBike*)CamTargetEntity)->bWheelieCam || TheCamera.m_fAvoidTheGeometryProbsTimer > 0.0f)){
			if(CPad::GetPad(0)->GetLeftShoulder2() || CPad::GetPad(0)->GetRightShoulder2()){
				TheCamera.m_fAvoidTheGeometryProbsTimer = 0.0f;
				((CBike*)CamTargetEntity)->bWheelieCam = false;
			}else if(Process_WheelCam(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar)){
				if(((CBike*)CamTargetEntity)->bWheelieCam)
					TheCamera.m_fAvoidTheGeometryProbsTimer = 50.0f;
				else{
					TheCamera.m_fAvoidTheGeometryProbsTimer -= CTimer::GetTimeStep();
					((CBike*)CamTargetEntity)->bWheelieCam = true;
				}
				return;
			}else{
				TheCamera.m_fAvoidTheGeometryProbsTimer = 0.0f;
				((CBike*)CamTargetEntity)->bWheelieCam = false;
			}
		}

		CMatrix *matrix = &CamTargetEntity->GetMatrix();
		if(((CVehicle*)CamTargetEntity)->IsBike()){
			((CBike*)CamTargetEntity)->CalculateLeanMatrix();
			matrix = &((CBike*)CamTargetEntity)->m_leanMatrix;
		}

		CVehicleModelInfo *mi = (CVehicleModelInfo*)CModelInfo::GetModelInfo(CamTargetEntity->GetModelIndex());
		CVector CamPos = mi->GetFrontSeatPosn();
		CamPos.x = 0.0f;
		CamPos.y += 0.08f;
		CamPos.z += 0.62f;
		FOV = 60.0f;
		Source = Multiply3x3(*matrix, CamPos);
		Source += CamTargetEntity->GetPosition();
		if(((CVehicle*)CamTargetEntity)->IsBoat())
			Source.z += 0.5f;
		else if(((CVehicle*)CamTargetEntity)->IsBike() && ((CVehicle*)CamTargetEntity)->pDriver){
			CVector Neck(0.0f, 0.0f, 0.0f);
			((CVehicle*)CamTargetEntity)->pDriver->m_pedIK.GetComponentPosition(Neck, PED_NECK);
			Neck += ((CVehicle*)CamTargetEntity)->m_vecMoveSpeed * CTimer::GetTimeStep();
			Source.z = Neck.z + fBike1stPersonOffsetZ;
		}

		if(((CVehicle*)CamTargetEntity)->IsUpsideDown()){
			if(DontLookThroughWorldFixer < 0.5f)
				DontLookThroughWorldFixer += 0.03f;
			else
				DontLookThroughWorldFixer = 0.5f;
		}else{
			if(DontLookThroughWorldFixer < 0.0f)
#ifdef FIX_BUGS
				DontLookThroughWorldFixer += 0.03f;
#else
				DontLookThroughWorldFixer -= 0.03f;
#endif
			else
				DontLookThroughWorldFixer = 0.0f;
		}
		Source.z += DontLookThroughWorldFixer;
		Front = matrix->GetForward();
		Front.Normalise();
		Up = matrix->GetUp();
		Up.Normalise();
		CVector Right = CrossProduct(Front, Up);
		Right.Normalise();
		Up = CrossProduct(Right, Front);
		Up.Normalise();
	}

	ResetStatics = false;
}

static CVector vecHeadCamOffset(0.06f, 0.05f, 0.0f);

void
CCam::Process_1rstPersonPedOnPC(const CVector&, float TargetOrientation, float, float)
{
#ifdef VICEEXT_RECOIL
	bool recoilReset = ResetStatics;
#endif
	// static int DontLookThroughWorldFixer = 0;	// unused
	static CVector InitialHeadPos;

	if(Mode != MODE_SNIPER_RUNABOUT)
		FOV = DefaultFOV;
	TheCamera.m_1rstPersonRunCloseToAWall = false;
	if(CamTargetEntity->m_rwObject == nil)
		return;

	if(CamTargetEntity->IsPed()){
		// static bool FailedTestTwelveFramesAgo = false;	// unused
		CVector HeadPos = vecHeadCamOffset;
		CVector TargetCoors;

#ifdef VICEEXT_FIRST_PERSON
		// Sección 3, bloque C1: la cabeza se saca del IK, igual que en
		// Process_M16_1stPerson (la 1ª persona de arma, que sí se usa hoy).
		// La vía original (TransformToNode + transformar la matriz del hueso vía
		// RpHAnimHierarchyGetMatrixArray/RpHAnimIDGetIndex, y escalarla a cero
		// para esconder la cabeza) devolvía una posición inválida en este port:
		// la cámara saltaba fuera del mundo y el rayo de las reflexiones de
		// audio (que parte de TheCamera.GetPosition()) reventaba con
		// `memory access out of bounds` en CWorld::ProcessLineOfSightSectorList a
		// los pocos segundos de entrar en el modo.
		CamTargetEntity->GetMatrix().UpdateRW();
		CamTargetEntity->UpdateRwFrame();
		CamTargetEntity->UpdateRpHAnim();
		HeadPos = CVector(0.0f, 0.0f, 0.0f);
		((CPed*)CamTargetEntity)->m_pedIK.GetComponentPosition(HeadPos, PED_HEAD);
		// R5: la cámara va a los OJOS, no al centro de la cabeza: avance al
		// frente con el rumbo actual del ped (el blog usó 0,19 m).
		{
			CVector odFwd = CamTargetEntity->GetForward();
			odFwd.z = 0.0f;
			if (odFwd.MagnitudeSqr() > 0.001f) {
				odFwd.Normalise();
				HeadPos.x += odFwd.x * 0.19f;
				HeadPos.y += odFwd.y * 0.19f;
			}
		}
#else
		((CPed*)CamTargetEntity)->TransformToNode(HeadPos, PED_HEAD);
		RpHAnimHierarchy *hier = GetAnimHierarchyFromSkinClump(CamTargetEntity->GetClump());
		int32 idx = RpHAnimIDGetIndex(hier, ConvertPedNode2BoneTag(PED_HEAD));
		RwMatrix *mats = RpHAnimHierarchyGetMatrixArray(hier);
		RwV3dTransformPoints(&HeadPos, &HeadPos, 1, &mats[idx]);
		RwV3d scl = { 0.0f, 0.0f, 0.0f };
		RwMatrixScale(&mats[idx], &scl, rwCOMBINEPRECONCAT);
#endif

		if(ResetStatics){
			Beta = TargetOrientation;
			Alpha = 0.0f;
			m_fInitialPlayerOrientation = TargetOrientation;
			if(CamTargetEntity->IsPed()){	// useless check
				Beta = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
				Alpha = 0.0f;
				m_fInitialPlayerOrientation = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
				// FailedTestTwelveFramesAgo = false;
				m_bCollisionChecksOn = true;
			}
			// DontLookThroughWorldFixer = false;
			m_vecBufferedPlayerBodyOffset = HeadPos;
			InitialHeadPos = HeadPos;
		}

		m_vecBufferedPlayerBodyOffset.y = HeadPos.y;

		if(TheCamera.m_bHeadBob){
			m_vecBufferedPlayerBodyOffset.x =
				TheCamera.m_fGaitSwayBuffer * m_vecBufferedPlayerBodyOffset.x +
				(1.0f-TheCamera.m_fGaitSwayBuffer) * HeadPos.x;
			m_vecBufferedPlayerBodyOffset.z =
				TheCamera.m_fGaitSwayBuffer * m_vecBufferedPlayerBodyOffset.z +
				(1.0f-TheCamera.m_fGaitSwayBuffer) * HeadPos.z;
			HeadPos = (CamTargetEntity->GetMatrix() * m_vecBufferedPlayerBodyOffset);
		}else{
			float HeadDelta = (HeadPos - InitialHeadPos).Magnitude2D();
			CVector Fwd = CamTargetEntity->GetForward();
			Fwd.z = 0.0f;
			Fwd.Normalise();
			HeadPos = HeadDelta*1.23f*Fwd + CamTargetEntity->GetPosition();
			HeadPos.z += 0.59f;
		}
		Source = HeadPos;

		// unused:
		// ((CPed*)CamTargetEntity)->m_pedIK.GetComponentPosition(MidPos, PED_MID);
		// Source - MidPos;

		// Look around
		bool UseMouse = false;
		float MouseX = CPad::GetPad(0)->GetMouseX();
		float MouseY = CPad::GetPad(0)->GetMouseY();
		float LookLeftRight, LookUpDown;
#ifdef VICEEXT_RECOIL
		float recoilManualAlphaStart = Alpha;
#endif
		if(MouseX != 0.0f || MouseY != 0.0f){
			UseMouse = true;
			LookLeftRight = -3.0f*MouseX;
			LookUpDown = 4.0f*MouseY;
		}else{
			LookLeftRight = -CPad::GetPad(0)->LookAroundLeftRight();
			LookUpDown = CPad::GetPad(0)->LookAroundUpDown();
		}
		if(UseMouse){
			Beta += TheCamera.m_fMouseAccelHorzntl * LookLeftRight * FOV/80.0f;
			Alpha += TheCamera.m_fMouseAccelVertical * LookUpDown * FOV/80.0f;
		}else{
			float xdir = LookLeftRight < 0.0f ? -1.0f : 1.0f;
			float ydir = LookUpDown < 0.0f ? -1.0f : 1.0f;
			Beta += SQR(LookLeftRight/100.0f)*xdir*0.8f/14.0f * FOV/80.0f * CTimer::GetTimeStep();
			Alpha += SQR(LookUpDown/150.0f)*ydir*1.0f/14.0f * FOV/80.0f * CTimer::GetTimeStep();
		}
		while(Beta >= PI) Beta -= 2*PI;
		while(Beta < -PI) Beta += 2*PI;
#ifdef VICEEXT_RECOIL
		CWeapon::ViceExtRecoilBegin(Alpha, recoilReset, Mode, "first-person-ped");
		CWeapon::ViceExtRecoilApply(Alpha, Alpha - recoilManualAlphaStart, LookUpDown, UseMouse ? "mouse" : "pad", Mode,
			-DEGTORAD(89.5f), DEGTORAD(60.0f));
#endif
		if(Alpha > DEGTORAD(60.0f)) Alpha = DEGTORAD(60.0f);
		else if(Alpha < -DEGTORAD(89.5f)) Alpha = -DEGTORAD(89.5f);

		if(((CPed*)CamTargetEntity)->IsPlayer() && ((CPed*)CamTargetEntity)->m_attachedTo){
			CPed *pedTarget = ((CPed*)CamTargetEntity);
			float NewBeta;
			switch(pedTarget->m_attachType){
			case 0:
				NewBeta = pedTarget->GetForward().Heading() + HALFPI;
				break;
			case 1:
				NewBeta = pedTarget->GetForward().Heading() + PI;
				break;
			case 2:
				NewBeta = pedTarget->GetForward().Heading() - HALFPI;
				break;
			case 3:
				NewBeta = pedTarget->GetForward().Heading();
				break;
			}

			float BetaOffset = Beta - NewBeta;
			if(BetaOffset > PI) BetaOffset -= TWOPI;
			else if(BetaOffset < PI) BetaOffset += TWOPI;

			BetaOffset = Clamp(BetaOffset, -pedTarget->m_attachRotStep, pedTarget->m_attachRotStep);
			Beta = NewBeta + BetaOffset;
		}

		TargetCoors.x = 3.0f * Cos(Alpha) * Cos(Beta) + Source.x;
		TargetCoors.y = 3.0f * Cos(Alpha) * Sin(Beta) + Source.y;
		TargetCoors.z = 3.0f * Sin(Alpha) + Source.z;
		Front = TargetCoors - Source;
		Front.Normalise();
		Source += Front*0.4f;

		TheCamera.m_AlphaForPlayerAnim1rstPerson = Alpha;

		GetVectorsReadyForRW();

		float Heading = Front.Heading();
		// Sección 3, bloque C1: mirar el ped de la cámara, no TheCamera.pTargetEntity
		// (puede ser nil si el objetivo cambió a mitad de frame y reventaba aquí).
		((CPed*)CamTargetEntity)->m_fRotationCur = Heading;
		((CPed*)CamTargetEntity)->m_fRotationDest = Heading;
		CamTargetEntity->SetHeading(Heading);
		CamTargetEntity->GetMatrix().UpdateRW();

		if(Mode == MODE_SNIPER_RUNABOUT){
			// no mouse wheel FOV buffering here like in normal sniper mode
			if(CPad::GetPad(0)->SniperZoomIn() || CPad::GetPad(0)->SniperZoomOut()){
				if(CPad::GetPad(0)->SniperZoomOut())
					FOV *= (255.0f*CTimer::GetTimeStep() + 10000.0f) / 10000.0f;
				else
					FOV /= (255.0f*CTimer::GetTimeStep() + 10000.0f) / 10000.0f;
			}

			TheCamera.SetMotionBlur(180, 255, 180, 120, MOTION_BLUR_SNIPER);

			if(FOV > DefaultFOV)
				FOV = DefaultFOV;
			if(FOV < 15.0f)
				FOV = 15.0f;
		}
	}

	ResetStatics = false;
	RwCameraSetNearClipPlane(Scene.camera, 0.05f);
}

float fCameraNearClipMult = 0.15f;

void
CCam::Process_Sniper(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	if(!CamTargetEntity->IsPed())
		return;

	float BackOffset = 0.19f;
	static bool FailedTestTwelveFramesAgo = false;
	RwV3d HeadPos;
	CVector TargetCoors;
	TargetCoors = CameraTarget;

	static float TargetFOV = 0.0f;

	if(ResetStatics){
		Beta = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
		Alpha = 0.0f;
		m_fInitialPlayerOrientation = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
		FailedTestTwelveFramesAgo = false;
		// static DPadVertical unused
		// static DPadHorizontal unused
		m_bCollisionChecksOn = true;
		FOVSpeed = 0.0f;
		TargetFOV = FOV;
		ResetStatics = false;
	}

	if(((CPed*)CamTargetEntity)->bIsDucking)
		BackOffset = 0.8f;
	CamTargetEntity->GetMatrix().UpdateRW();
	CamTargetEntity->UpdateRwFrame();
	CamTargetEntity->UpdateRpHAnim();
	((CPed*)CamTargetEntity)->m_pedIK.GetComponentPosition(HeadPos, PED_HEAD);
	Source = HeadPos;
	Source.z += 0.1f;
	if(((CPed*)CamTargetEntity)->bIsDucking){
		Source.x -= fDuckingBackOffset*CamTargetEntity->GetForward().x;
		Source.y -= fDuckingBackOffset*CamTargetEntity->GetForward().y;
		Source.x -= fDuckingRightOffset*CamTargetEntity->GetRight().x;
		Source.y -= fDuckingRightOffset*CamTargetEntity->GetRight().y;
	}else{
		Source.x -= BackOffset*CamTargetEntity->GetForward().x;
		Source.y -= BackOffset*CamTargetEntity->GetForward().y;
	}

	// Look around
	bool UseMouse = false;
	float MouseX = CPad::GetPad(0)->GetMouseX();
	float MouseY = CPad::GetPad(0)->GetMouseY();
	float LookLeftRight, LookUpDown;
#ifdef VICEEXT_RECOIL
	float recoilManualAlphaStart = Alpha;
#endif
	if(MouseX != 0.0f || MouseY != 0.0f){
		UseMouse = true;
		LookLeftRight = -3.0f*MouseX;
		LookUpDown = 4.0f*MouseY;
	}else{
		LookLeftRight = -CPad::GetPad(0)->SniperModeLookLeftRight();
		LookUpDown = CPad::GetPad(0)->SniperModeLookUpDown();
	}
	if(UseMouse){
		Beta += TheCamera.m_fMouseAccelHorzntl * LookLeftRight * FOV/80.0f;
		Alpha += TheCamera.m_fMouseAccelVertical * LookUpDown * FOV/80.0f;
	}else{
		float xdir = LookLeftRight < 0.0f ? -1.0f : 1.0f;
		float ydir = LookUpDown < 0.0f ? -1.0f : 1.0f;
		Beta += SQR(LookLeftRight/100.0f)*xdir*0.8f/14.0f * FOV/80.0f * CTimer::GetTimeStep();
		Alpha += SQR(LookUpDown/150.0f)*ydir*1.0f/14.0f * FOV/80.0f * CTimer::GetTimeStep();
	}
	while(Beta >= PI) Beta -= 2*PI;
	while(Beta < -PI) Beta += 2*PI;
	if(Alpha > DEGTORAD(60.0f)) Alpha = DEGTORAD(60.0f);
	else if(Alpha < -DEGTORAD(89.5f)) Alpha = -DEGTORAD(89.5f);

	TargetCoors.x = 3.0f * Cos(Alpha) * Cos(Beta) + Source.x;
	TargetCoors.y = 3.0f * Cos(Alpha) * Sin(Beta) + Source.y;
	TargetCoors.z = 3.0f * Sin(Alpha) + Source.z;

	UseMouse = false;
	int ZoomInButton = ControlsManager.GetMouseButtonAssociatedWithAction(PED_SNIPER_ZOOM_IN);
	int ZoomOutButton = ControlsManager.GetMouseButtonAssociatedWithAction(PED_SNIPER_ZOOM_OUT);
	if(ZoomInButton == rsMOUSEWHEELUPBUTTON || ZoomInButton == rsMOUSEWHEELDOWNBUTTON || ZoomOutButton == rsMOUSEWHEELUPBUTTON || ZoomOutButton == rsMOUSEWHEELDOWNBUTTON){
		if(CPad::GetPad(0)->GetMouseWheelUp() || CPad::GetPad(0)->GetMouseWheelDown()){
			if(CPad::GetPad(0)->SniperZoomIn()){
				TargetFOV = FOV - 10.0f;
				UseMouse = true;
			}
			if(CPad::GetPad(0)->SniperZoomOut()){
				TargetFOV = FOV + 10.0f;
				UseMouse = true;
			}
		}
	}
	if((CPad::GetPad(0)->SniperZoomIn() || CPad::GetPad(0)->SniperZoomOut()) && !UseMouse){
		if(CPad::GetPad(0)->SniperZoomOut()){
			FOV *= (255.0f*CTimer::GetTimeStep() + 10000.0f) / 10000.0f;
			TargetFOV = FOV;
			FOVSpeed = 0.0f;
		}else{
			FOV /= (255.0f*CTimer::GetTimeStep() + 10000.0f) / 10000.0f;
			TargetFOV = FOV;
			FOVSpeed = 0.0f;
		}
	}else{
		if(Abs(TargetFOV - FOV) > 0.5f)
			WellBufferMe(TargetFOV, &FOV, &FOVSpeed, 0.5f, 0.25f, false);
		else
			FOVSpeed = 0.0f;
	}

	TheCamera.SetMotionBlur(180, 255, 180, 120, MOTION_BLUR_SNIPER);

	if(FOV > DefaultFOV)
		FOV = DefaultFOV;
	if(Mode == MODE_CAMERA){
		if(FOV < 3.0f)
			FOV = 3.0f;
	}else{
		if(FOV < 15.0f)
			FOV = 15.0f;
	}

	Front = TargetCoors - Source;
	Front.Normalise();
	Source += Front*0.4f;

	if(m_bCollisionChecksOn){
		if(!CWorld::GetIsLineOfSightClear(TargetCoors, Source, true, true, false, true, false, true, true)){
			RwCameraSetNearClipPlane(Scene.camera, 0.4f);
			FailedTestTwelveFramesAgo = true;
		}else{
			CVector TestPoint;
			TestPoint.x = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Cos(Beta + DEGTORAD(35.0f)) + Source.x;
			TestPoint.y = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Sin(Beta + DEGTORAD(35.0f)) + Source.y;
			TestPoint.z = 3.0f * Sin(Alpha - DEGTORAD(20.0f)) + Source.z;
			if(!CWorld::GetIsLineOfSightClear(TestPoint, Source, true, true, false, true, false, true, true)){
				RwCameraSetNearClipPlane(Scene.camera, 0.4f);
				FailedTestTwelveFramesAgo = true;
			}else{
				TestPoint.x = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Cos(Beta - DEGTORAD(35.0f)) + Source.x;
				TestPoint.y = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Sin(Beta - DEGTORAD(35.0f)) + Source.y;
				TestPoint.z = 3.0f * Sin(Alpha - DEGTORAD(20.0f)) + Source.z;
				if(!CWorld::GetIsLineOfSightClear(TestPoint, Source, true, true, false, true, false, true, true)){
					RwCameraSetNearClipPlane(Scene.camera, 0.4f);
					FailedTestTwelveFramesAgo = true;
				}else
					FailedTestTwelveFramesAgo = false;
			}
		}
	}

	if(FailedTestTwelveFramesAgo)
		RwCameraSetNearClipPlane(Scene.camera, 0.4f);
	else if(Mode == MODE_CAMERA)
		RwCameraSetNearClipPlane(Scene.camera, ((15.0f - Min(FOV, 15.0f))*fCameraNearClipMult + 1.0f)*DEFAULT_NEAR);
	Source -= Front*0.4f;

	GetVectorsReadyForRW();
	float Rotation = CGeneral::GetATanOfXY(Front.x, Front.y) - HALFPI;
	((CPed*)TheCamera.pTargetEntity)->m_fRotationCur = Rotation;
	((CPed*)TheCamera.pTargetEntity)->m_fRotationDest = Rotation;
}

float INIT_SYPHON_GROUND_DIST = 2.419f;
float INIT_SYPHON_ALPHA_OFFSET = -DEGTORAD(3.0f);
float INIT_SYPHON_DEGREE_OFFSET = -DEGTORAD(30.0f);
float FrontOffsetSyphon = -DEGTORAD(25.5f);	// unused
float INIT_SYPHON_Z_OFFSET = -0.5f;

// PORTADO — ClassicAXIS (sin LICENSE, gennariarmando/DK22Pac) — CamNew.cpp:390
//   «void CCamNew::Process_AvoidCollisions(float length)»
// Qué se toma: el LOS con el objetivo como entidad ignorada y las 5 esferas con el
//   near-clip RE-LEÍDO en cada vuelta (`CamNew.cpp:390-445`).
// Adaptación: el bloque ya estaba escrito y validado dentro de
//   `Process_Cam_On_A_String` (ve65-ve75, el jugador lo dio por bueno); se EXTRAE
//   aquí en vez de copiarlo para la ley de apuntado (RULES 0.6, cero duplicación).
//   Comportamiento IDÉNTICO: mismos números, mismos argumentos, mismo orden, y
//   los mismos `break` (los del mod también cortan; la única diferencia real es
//   que el mod NO esconde peds cuando no hay impacto, y eso no se nota).
//   Única diferencia de verdad: `hideClosePeds`. El mod ESCONDE los peds a <0,5 m del
//   centro de la esfera y visibles durante UN frame (`CamNew.cpp:425-429`) para que no
//   tapen la mira; el coche no lo hace y no se le añade (sería cambiar una ley
//   validada), así que ahí va con `false`.
//   Dos desajustes de nombre con el mod, ya resueltos: `CColPoint::m_vecPoint` =
//   `CColPoint::point`, y el ancho de vista usa `CDraw::GetAspectRatio()` (sólo
//   lectura) en vez de `CalculateAspectRatio()`, que además MUTA `ms_fAspectRatio`
//   como efecto secundario.
// Medible: §8.12 — `AIMCOL` para la ley de apuntado, y las `CAMB2b`/`CAMB3b` del
//   coche tienen que dar EXACTAMENTE los mismos números que antes del refactor.
void
CCam::Process_AvoidCollisions(const CVector &targetCoors, float length, bool hideClosePeds)
{
	// CamNew.cpp:411-415: los peds que el frame anterior escondió se devuelven al
	// principio de ESTE (son 5, los del vector del mod).
	for(int i = 0; i < 5; i++){
		if(s_odHidePeds[i]){
			s_odHidePeds[i]->bIsVisible = true;
			s_odHidePeds[i] = nil;
		}
	}
	s_odHideCount = 0;

	CColPoint colPoint;
	CEntity *entity = nil;
	CWorld::pIgnoreEntity = CamTargetEntity;
	bool odLosHit = CWorld::ProcessLineOfSight(targetCoors, Source, colPoint, entity, true, true, false, true, false, false, false, false);
	// ve71: el rayo parte de targetCoors, que va a 0,8*alto del vehiculo (en
	// la moto: casco/cabeza), o sea DENTRO del propio jugador. Mirando de
	// frente el primer impacto era el propio ped y Source se pegaba a el:
	// medido en ve70, dist media 1,94 m de frente contra 7,20 m detras (la
	// "camara que se acerca y solo deja ver el casco/parabrisas"). El ped
	// propio no debe frenar la camara (la prueba de esferas ya lo ignora).
	s_odLosHit = odLosHit ? 1 : 0;
	s_odLosD = odLosHit ? (targetCoors - colPoint.point).Magnitude() : 0.0f;
	s_odLosPed = (odLosHit && entity && entity->IsPed()) ? 1 : 0;
	if(odLosHit && entity && entity->IsPed())
		odLosHit = false;
	if(odLosHit){
		float distFromPoint = (targetCoors - colPoint.point).Magnitude();
		Source = colPoint.point;
		if(distFromPoint < 1.3f)
			RwCameraSetNearClipPlane(Scene.camera, Max(distFromPoint - 0.3f, 0.05f));
	}
	CWorld::pIgnoreEntity = nil;

	float viewPlaneHeight = Tan(DEGTORAD(FOV) / 2.0f);
	float viewPlaneWidth = viewPlaneHeight * CDraw::GetAspectRatio() * 1.05f;
	// ve72 (fallo "de frente la camara se pega a la cara"): el tercer argumento
	// (`nil`) es pIgnoreEntity, asi que la esfera podia chocar con el PROPIO
	// vehiculo del jugador: esta justo en el eje camara->objetivo y su bounding
	// sphere es grande (BaseDist 5,37 => radio ~1,9), o sea que mirando de frente
	// el test pegaba SIEMPRE. Ese impacto entra por la rama `d == 0.1f` (con la
	// nearClip ya enganchada en 0,1, Min(nearClip,dRaw) deja de discriminar),
	// mueve Source 0,1 hacia el objetivo en cada frame y el clamp
	// [minDist,maxDist] lo fija en minDist=2,00: medido en ve71, dist 5,47 ->
	// 3,61 -> 2,00 y clavada a 2,00 con idle creciendo (imagen 2). Con Q+E
	// (LookBehind coloca Source a CA_MAX_DISTANCE) no pasa por aqui: por eso esa
	// era la vista buena (imagen 1). El vehiculo objetivo no ocluye su camara.
	s_odSphHit = 0; s_odSphModel = -1; s_odSphPed = 0; s_odSphOwn = 0;
	s_odSphApp = 0; s_odSphD = 0.0f; s_odSphNear = RwCameraGetNearClipPlane(Scene.camera);
	for(int i = 0; i < 5; i++){
		float nearClip = RwCameraGetNearClipPlane(Scene.camera);
		float radius = viewPlaneWidth * nearClip;
		CVector center = Source + Front * nearClip;
		entity = CWorld::TestSphereAgainstWorld(center, radius, CamTargetEntity, true, true, true, true, false, true);
		if(!entity)
			break;
		if(!*reinterpret_cast<void**>(entity))
			break;
		if(entity->IsPed()){
			s_odSphHit = 1; s_odSphModel = entity->GetModelIndex(); s_odSphPed = 1;
			// CamNew.cpp:425-429: ped visible a <0,5 m del centro de la esfera
			// -> invisible este frame. Solo en la ley de apuntado.
			if(hideClosePeds && entity->IsVisible()
			   && (center - entity->GetPosition()).Magnitude2D() < 0.5f){
				entity->bIsVisible = false;
				s_odHidePeds[s_odHideCount++] = entity;
				if(s_odHideCount >= 5)
					s_odHideCount = 0;
			}
			break;
		}
		if(entity == CamTargetEntity){
			s_odSphHit = 1; s_odSphModel = entity->GetModelIndex(); s_odSphOwn = 1;
			break;
		}
		CVector camToCol = gaTempSphereColPoints[0].point - targetCoors;
		float frontDist = DotProduct(camToCol, Front);
		float d = (camToCol - Front * frontDist).Magnitude() / viewPlaneWidth;
		s_odSphHit = 1; s_odSphModel = entity->GetModelIndex(); s_odSphD = d;
		d = Max(Min(nearClip, d), 0.1f);
		if(d < nearClip)
			RwCameraSetNearClipPlane(Scene.camera, d);
		if(d == 0.1f){
			s_odSphApp++;
			Source += (targetCoors - Source) * (d / length);
		}
	}
	// ve73: para el frame siguiente: ¿habia geometria delante de la camara?
	// (rayo que SI mueve Source, o esfera que SI lo acerca). Si no, vuelve a
	// maxDist. Es tambien lo que hace que al bajarse/subirse la camara arranque
	// ya a la distancia buena.
	s_odDistObs = (s_odLosHit != 0) || (s_odSphApp > 0);
}

// =====================================================================
// ClassicAXIS ·ley de apuntado del mod
// =====================================================================
// Constantes del mod (CamNew.cpp:25-28). `maxFOVModern` (:27) NO se porta: vale
// 70 igual que `maxFOV`, o sea que su rama es código muerto (y §5.4c la declaró fuera).
static const float AIM_MIN_FOV = 50.0f;        // CamNew.cpp:25
static const float AIM_MAX_FOV = 70.0f;        // CamNew.cpp:26  (= DefaultFOV, Camera.h:29)
static const float AIM_WEP_MIN_RANGE = 70.0f;  // CamNew.cpp:28
static const float AIM_MAX_DIST = 2.7f;       // CamNew.cpp:248
static const float AIM_HEIGHT_OFFSET = 0.25f;  // CamNew.cpp:249
static const float AIM_ZOFF_PLUS = 0.05f;      // CamNew.cpp:282
// `s_odAimFovLerp` = `fovLerp` (CamNew.cpp:43, estado entre frames del CCamNew).
static float s_odAimFovLerp = AIM_MAX_FOV;
static bool  s_odAimDoFov = false;   // `doFovChanges` (CamNew.cpp:240, se consume en :497)

// PORTADO — ClassicAXIS (sin LICENSE, gennariarmando/DK22Pac) — CamNew.cpp:477
//   «void CCamNew::Process_FOVLerp()»
// Qué se toma: el FOV baja a 50 al apuntar un arma apuntable de una mano y alcance
//   >= 70 (`!CanAimWithArm && CanAim && range >= wepMinRange`), y vuelve a 70 al
//   soltar, interpolando con `0.1f * ms_fTimeStep`. El Minigun queda excluido
//   (:484-486), que es un quirk del mod en VC, y se porta literal.
// Adaptación: `CTimer::ms_fTimeStep` = `CTimer::GetTimeStep()` (1:1, Timer.h:22) y
//   `interpF(a,b,k)` = la macro `lerp(k, a, b)` de casa (common.h:396, idéntica).
//   `TheCamera.m_nTransitionState` = `m_uiTransitionState` (Camera.h:383).
//   El ajuste `zoomForAssaultRifles` (decisión §5.3a, activo) gobierna la rama.
//   `fovLerp` se resetea con `ResetStatics`, que el mod NO hace: sin eso el primer
//   apuntado tras cambiar de cámara hereda el FOV de otro modo. Desviación mínima.
// Medible: §8.6 — `AIMFOV fov= arma= alcance= taken=`, 1 Hz mientras se apunta.
void
CCam::Process_AimWeaponFovLerp(void)
{
	CPed *e = (CPed*)CamTargetEntity;
	if (e == nil)
		return;

	if (ViceExtPedOwns(PEDLANE_NADO, PEDCAP_APUNTAR)
	 || ViceExtPedOwns(PEDLANE_NADO, PEDCAP_CAMARA)) {
		FOV = DefaultFOV;
		s_odAimFovLerp = lerp(0.1f * CTimer::GetTimeStep(), s_odAimFovLerp, AIM_MAX_FOV);
		s_odAimDoFov = false;
		return;
	}

	if (!CCamera::s_viceExtAim.zoomForAssaultRifles) {
		// CamNew.cpp:244: `cam->m_fFOV = maxFOV` = 70 = DefaultFOV. Se escribe con el
		// nombre, no el número, para que no parezca un valor mágico.
		FOV = DefaultFOV;
		s_odAimDoFov = false;
		return;
	}

	if (TheCamera.m_uiTransitionState == 0) {
		CWeapon *w = e->GetWeapon();
		CWeaponInfo *info = CWeaponInfo::GetWeaponInfo(w->m_eWeaponType);
		bool changeFov = true;
		// CamNew.cpp:483-486: en VC el Minigun queda fuera.
		if (w->m_eWeaponType == WEAPONTYPE_MINIGUN)
			changeFov = false;
		if (changeFov && info && !info->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM)
		    && ViceExtCanAim(w->m_eWeaponType, info)
		    && (info->m_fRange >= AIM_WEP_MIN_RANGE || ViceExtAimHeavy(w->m_eWeaponType))
		    && s_odAimDoFov) {
			s_odAimFovLerp = lerp(0.1f * CTimer::GetTimeStep(), s_odAimFovLerp, AIM_MIN_FOV);
		} else {
			// CamNew.cpp:491-492: `f = maxFOVModern : maxFOV`, y los dos valen 70.
			s_odAimFovLerp = lerp(0.1f * CTimer::GetTimeStep(), s_odAimFovLerp, AIM_MAX_FOV);
		}
	}

	FOV = s_odAimFovLerp;
	s_odAimDoFov = false;   // CamNew.cpp:497: el flag es de un disparo
}

// PORTADO — ClassicAXIS (sin LICENSE, gennariarmando/DK22Pac) — CamNew.cpp:447
//   «void CCamNew::Process_CrouchOffset(float& offset)»
// Qué se toma: al agacharse, el objetivo de la cámara baja a -0,5 m corregido por
//   el zoom (`end = -0.5 + ((f - FOV)/minFOV * f)/100`), interpolado a `0.1*ts`.
//   El mod lo llama en las DOS leyes (:82 a pie, :280 apuntando); aquí, igual.
// Adaptación: `f = maxFOVModern : maxFOV` y los dos valen 70, así que `f` es
//   `AIM_MAX_FOV`. `bIsDucking` es el bitfield de `CPed::m_nPedFlags`.
// Medible: entra en `AIMCAM alt=` (el `+duckOffset` de la z) y en la ley de a pie.
void
CCam::Process_AimWeaponCrouchOffset(float &offset)
{
	CPed *e = (CPed*)CamTargetEntity;
	if (e == nil) {
		offset = lerp(0.1f * CTimer::GetTimeStep(), offset, 0.0f);
		return;
	}
	float f = AIM_MAX_FOV;
	float end = 0.0f;
	if (e->bIsDucking) {
		end = -0.5f;                                                  // CamNew.cpp:454
		end += (((f - FOV) / AIM_MIN_FOV * f) / 100.0f);              // CamNew.cpp:455
	}
	offset = lerp(0.1f * CTimer::GetTimeStep(), offset, end);
}

// PORTADO — ClassicAXIS (sin LICENSE, gennariarmando/DK22Pac) — CamNew.cpp:233
//   «void CCamNew::Process_AimWeapon(CVector const& target, float targetOrient, …)»
// Qué se toma: la ley de apuntado DEL MOD, completa. Traducción línea a línea:
//   :234-238 early-out (mismo que Process_FollowPed)   ·  :240 doFovChanges = true
//   :241-244 FOVLerp                                    ·  :248-250 2.7 / 0.25
//   :252-266 hombro en espacio de objeto                 ·  :268-278 LOS + repliegue
//   :280 CrouchOffset                                   ·  :282-284 z += zOff + 0.05
//   :288-303 lock-on con horShift/verShift               ·  :305-312 wrap + lockMovement
//   :313-345 offsets stick/ratón                        ·  :350-353 clamp ±50
//   :355-366 detrás/frente                              ·  :368-372 ángulo anterior
//   :374-383 vectores                                   ·  :385-387 colisiones + RW
// Adaptación (todo §5): el modo es el `MODE_AIMING` que el motor ya tenía en el
//   enum y NEVER fue conectado; los identificadores vanRenameados (m_fHorizontalAngle
//   → Beta, m_fVerticalAngle → Alpha, m_vecSource → Source, m_vecTargetCoorsForFudgeInter
//   → m_cvecTargetCoorsForFudgeInter, m_pPed → CamTargetEntity). NO se parchea
//   Process_Syphon: el mod no lo toca.
//   Desviaciones §5.4: el stick conserva la zona muerta de `LookAroundLeftRight()`
//   (el mod lee el crudo) y el ratón conserva `m_fMouseAccelVertical` (el mod usa
//   el horizontal, que es un typo suyo). Los factores -2.5 / 4.0 sí son los del mod.
//   §5.4a: el hombro usa `GetMatrix().GetPosition() + GetRight()*0.2`, que ES el
//   espacio de objeto del ped (Placeable.h:6,20) y coincide con el
//   `TransformFromObjectSpace` del mod para un ped con solo rotación Z.
//   El hombro va SIN suavizar: el mod no lo suaviza (`CamNew.cpp:257` es un literal).
// Medible: §8.1-§8.5 con `AIMCAM` (1 Hz + 1 por flanco) y `AIMFOV`.
void
CCam::Process_AimWeapon(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	// CamNew.cpp:234-238
	if (!CamTargetEntity || !CamTargetEntity->IsPed())
		return;

	// CamNew.cpp:240
	s_odAimDoFov = true;
	// CamNew.cpp:241-244
	Process_AimWeaponFovLerp();

	// CamNew.cpp:248-250. `length` es constante en toda la funcion del mod (:250 es
	// su unica asignacion) y es el que se pasa a Process_AvoidCollisions.
	const float maxDist = AIM_MAX_DIST;
	const float heightOffset = AIM_HEIGHT_OFFSET;
	const float length = maxDist;

	// CamNew.cpp:252-266 + B5. El 0,55 de la rama `storiesAimingCoords` NO se porta
	// (§9: sin conmutador natural y sin respuesta del jugador); el hombro va crudo.
	const CVector aimOffset(0.2f, 0.0f, 0.0f);
	CVector targetCoors = CamTargetEntity->GetMatrix().GetPosition()
	                    + CamTargetEntity->GetRight() * aimOffset.x;
	// Los tres ejes del shoulder de verdad, para validar el offset APLICADO contra el
	// teorico en la traza (§8.3: `ex/ ey/ ez=` y `lado=`).
	CVector shoulderRight = CamTargetEntity->GetRight();
	CVector shoulderPos   = CamTargetEntity->GetMatrix().GetPosition()
	                      + shoulderRight * aimOffset.x;

	// CamNew.cpp:268-278: LOS sobre el punto de hombro; si impacta, el objetivo cae
	// al x/y del objetivo de la camara (LA Z SE QUEDA, :276-277).
	{
		CColPoint colPoint;
		CEntity *entity = nil;
		if (CWorld::ProcessLineOfSight(targetCoors, Source, colPoint, entity,
		    true, false, false, true, false, true, true, false)) {
			targetCoors.x = CameraTarget.x;
			targetCoors.y = CameraTarget.y;
		}
	}

	// CamNew.cpp:280-284
	static float duckOffset = 0.0f;   // `duckOffset` es estado del CCamNew (CamNew.cpp:45)
	Process_AimWeaponCrouchOffset(duckOffset);
	targetCoors.z += m_fSyphonModeTargetZOffSet + AIM_ZOFF_PLUS;
	targetCoors.z += heightOffset;
	targetCoors.z += duckOffset;

	// CamNew.cpp:288-312
	bool lockMovement = false;
	CPlayerPed *ped = (CPlayerPed*)CamTargetEntity;
	CWeapon *curW = ped->GetWeapon();
	CWeaponInfo *curInfo = CWeaponInfo::GetWeaponInfo(curW->m_eWeaponType);
	CVector lockTargetPos;
	bool hadLock = false;
	if (curW && ped->m_pPointGunAt && ped->m_bHasLockOnTarget && !LookingBehind) {
		lockTargetPos = ped->m_pPointGunAt->GetPosition();
		CVector distfromTarget = Source - lockTargetPos;
		// CamNew.cpp:294-295. `GetAspectRatio()` (lectura), no `CalculateAspectRatio()`
		// (que ademas MUTA ms_fAspectRatio). El 1.05f es del mod, tal cual.
		float viewPlaneHeight = Tan(DEGTORAD(FOV) * 0.5f);
		float viewPlaneWidth  = viewPlaneHeight * CDraw::GetAspectRatio() * 1.05f;
		// CamNew.cpp:297-298: los parentesis rara vez repetidos son del mod, y el
		// 0.0174f del verShift tambien. NO redondear ninguno de los tres numeros.
		float horShift = CGeneral::GetATanOfXY(1.0f,
			(CCamera::m_f3rdPersonCHairMultX - 0.5f + CCamera::m_f3rdPersonCHairMultX - 0.5f) * viewPlaneWidth);
		float verShift = CGeneral::GetATanOfXY(1.0f,
			(viewPlaneHeight * 0.0174f)
			* ((0.5f - CCamera::m_f3rdPersonCHairMultY + 0.5f - CCamera::m_f3rdPersonCHairMultY)
			   * (1.0f / CDraw::GetAspectRatio())));
		Beta  = ped->m_fRotationCur + (PI * 0.5f) + horShift;
		Alpha = CGeneral::GetATanOfXY(distfromTarget.Magnitude2D(), -distfromTarget.z) - verShift;
		lockMovement = true;
		hadLock = true;
	}

	// CamNew.cpp:305-308 (wrap de los dos angulos a ±PI)
	while(Beta >= PI)  Beta -= 2 * PI;
	while(Beta < -PI)  Beta += 2 * PI;
	while(Alpha >= PI) Alpha -= 2 * PI;
	while(Alpha < -PI) Alpha += 2 * PI;

	// CamNew.cpp:310-312
	CPad *pad = CPad::GetPad(0);
	if (pad->ArePlayerControlsDisabled())
		lockMovement = true;

	// CamNew.cpp:313-333. Solo el modo RATON usa los factores -2.5 / 4.0; con palo
	// se usa la zona muerta de casa (§5.4b) y el acelerador vertical (§5.4d).
	float lookLeftRight = -(float)pad->LookAroundLeftRight();
	float lookUpDown   =  (float)pad->LookAroundUpDown();
	float MouseX = pad->GetMouseX();
	float MouseY = pad->GetMouseY();
	bool mouseInput = false;
	if (MouseX != 0.0f || MouseY != 0.0f) {
		mouseInput = true;
		lookLeftRight = -2.5f * MouseX;   // CamNew.cpp:139
		lookUpDown   =  4.0f * MouseY;   // CamNew.cpp:140
	}
	float betaOffset  = lookLeftRight * fStickSens * (1.0f / 20.0f) * FOV / 80.0f * CTimer::GetTimeStep();
	float alphaOffset = lookUpDown   * fStickSens * (0.6f / 20.0f) * FOV / 80.0f * CTimer::GetTimeStep();
	if (mouseInput) {
		// CamNew.cpp:149-150. El mod usa `m_fMouseAccelHorzntl` en LOS DOS ejes; aqui
		// el vertical sigue con su propio acelerador (§5.4d, es un typo del mod).
		betaOffset  = lookLeftRight * TheCamera.m_fMouseAccelHorzntl   * FOV / 80.0f;
		alphaOffset = lookUpDown   * TheCamera.m_fMouseAccelVertical * FOV / 80.0f;
	} else {
		// B3(d) ClassicAXIS RightAnalogStickSensitivityX/Y (CamNew.cpp:145-146 / :327-328):
		// el mod multiplica los offsets por el ajuste; el 0.01 base es nuestro
		// `fStickSens` (Cam.cpp:1698). En la rama de RATON el ajuste NO se aplica,
		// porque el mod tampoco lo aplica (pisa los offsets con los del raton).
		betaOffset  *= CCamera::s_viceExtAim.stickSensX;
		alphaOffset *= CCamera::s_viceExtAim.stickSensY;
	}

	// CamNew.cpp:335-345
	if ((betaOffset || alphaOffset || lockMovement) && !pad->GetSprint() && !pad->JumpJustDown()) {
		Rotating = false;
		CCamera::s_viceExtAimLawActive = true;
	}
#ifdef VICEEXT_RECOIL
	// ClassicAXIS §5.7: sin este callsite, apuntar dejaria de mover la camara con el
	// recoil (el recoil se aplica desde DENTRO de la ley de camara, no desde el arma).
	// Copia LITERAL de Process_Syphon (:4162 / :4170). Limites -PI/+PI y NO +-50: el
	// clamp de la ley los aplica DESPUES, igual que Process_Syphon con los suyos.
	// `manualDeltaRad` e `inputY` a 0 porque con `lockMovement` esta ley no lleva
	// input manual medible: el `Alpha` lo decide la rama de lock-on, no el raton
	// (es el mismo argumento que da el comentario de Cam.cpp:4169).
	CWeapon::ViceExtRecoilBegin(Alpha, ResetStatics, Mode, "aim-weapon");
	CWeapon::ViceExtRecoilApply(Alpha, 0.0f, 0.0f, "unknown", Mode, -PI, PI);
#endif
	if (!lockMovement) {
		Beta  += betaOffset;
		Alpha += alphaOffset;
	}

	// CamNew.cpp:347-353
	while(Beta >= PI)  Beta -= 2 * PI;
	while(Beta < -PI)  Beta += 2 * PI;
	if (Alpha > DEGTORAD(50.0f))
		Alpha = DEGTORAD(50.0f);
	else if (Alpha < -DEGTORAD(50.0f))
		Alpha = -DEGTORAD(50.0f);

	// CamNew.cpp:355-366
	if (TheCamera.m_bCamDirectlyBehind) {
		m_bCollisionChecksOn = true;
		Beta = TargetOrientation;
		Alpha = 0.0f;
		TheCamera.m_bCamDirectlyBehind = false;
	}
	if (TheCamera.m_bCamDirectlyInFront) {
		Beta = TargetOrientation + PI;
		Alpha = 0.0f;
		TheCamera.m_bCamDirectlyInFront = false;
	}

	// CamNew.cpp:368-372: `camUseCurrentAngle`, que el mod pone al tomar el control
	// (Main.cpp:1240-1242) y aqui pone B9 al hacer el TakeControl.
	if (CCamera::s_viceExtAimSwitchSpeed) {
		Beta  = CCamera::s_viceExtAimPrevHor;
		Alpha = CCamera::s_viceExtAimPrevVer;
	}

	// CamNew.cpp:374-383
#ifdef VICEEXT_AIM_CLASSICAXIS
	if (CCamera::s_viceExtAimViewPending && CCamera::s_viceExtAimViewMode == Mode) {
		CVector odV = CCamera::s_viceExtAimViewDir;
		float odH = Sqrt(odV.x * odV.x + odV.y * odV.y);
		Beta = Atan2(odV.y, odV.x);
		Alpha = Atan2(odV.z, odH);
		if (Alpha > DEGTORAD(50.0f)) Alpha = DEGTORAD(50.0f);
		else if (Alpha < -DEGTORAD(50.0f)) Alpha = -DEGTORAD(50.0f);
		CCamera::s_viceExtAimViewPending = false;
	}
#endif
	m_fDistanceBeforeChanges = (Source - targetCoors).Magnitude();
	Front = CVector(Cos(Alpha) * Cos(Beta), Cos(Alpha) * Sin(Beta), Sin(Alpha));
	Source = targetCoors - Front * length;
	SourceBeforeLookBehind = targetCoors + Front;
	targetCoors.z -= heightOffset;
	m_cvecTargetCoorsForFudgeInter = targetCoors;
	Front = targetCoors - Source;
	Front.Normalise();

	// CamNew.cpp:385-387. `hideClosePeds = true` (el mod esconde los peds a <0,5 m).
	Process_AvoidCollisions(targetCoors, length, true);
	// GetVectorsReadyForRW ya existe en el motor (Camera.h:193) y es identico a
	// CamNew.cpp:461-475, asi que se LLAMA y no se reescribe.
	GetVectorsReadyForRW();

#ifdef __EMSCRIPTEN__		// trazas del plan apuntado-classicaxis-100
	// 1 Hz mientras se apunta, con reanclaje del reloj (patron Cam.cpp:2622-2626).
	static uint32 s_odNextAx = 0;
	uint32 odNow = CTimer::GetTimeInMilliseconds();
	if (s_odNextAx > odNow + 60000) s_odNextAx = 0;	// el reloj retrocedio (carga)
	if (odNow >= s_odNextAx) {
		s_odNextAx = odNow + 1000;
		static float s_odAimAmax = 0.0f;
		float amax = s_odAimAmax = Max(s_odAimAmax, Abs(Alpha) * RADTODEG(1.0f));
		CVector odOff = shoulderPos - CamTargetEntity->GetMatrix().GetPosition();
		{
			char t[300];
			snprintf(t, sizeof t, "AIMCAM m=%d dist=%.2f alt=%.3f zoff=%.3f amax=%.1f fight=%d hombro=0.20 obj=1 ex=%.2f ey=%.2f ez=%.2f lado=%+d trans=%u",
				(int)Mode, (Source - m_cvecTargetCoorsForFudgeInter).Magnitude(),
				heightOffset + m_fSyphonModeTargetZOffSet + AIM_ZOFF_PLUS + duckOffset,
				m_fSyphonModeTargetZOffSet, amax, 0, odOff.x, odOff.y, odOff.z,
				(shoulderRight.x >= 0.0f) ? 1 : -1,
				(unsigned)CCamera::s_viceExtAimSwitchSpeed);
			ODTRACES(t);
		}
		{
			float odFov = TheCamera.Cams[TheCamera.ActiveCam].FOV;
			float odTan = Tan(DEGTORAD(odFov * 0.5f));
			float odAx = odTan * (CCamera::m_f3rdPersonCHairMultX - 0.5f + CCamera::m_f3rdPersonCHairMultX - 0.5f);
			float odAy = odTan * (0.5f - CCamera::m_f3rdPersonCHairMultY + 0.5f - CCamera::m_f3rdPersonCHairMultY);
			CVector odV = Front;
			odV += Up * Tan(DEGTORAD((0.5f - CCamera::m_f3rdPersonCHairMultY) * 1.8f * 0.5f * odFov));
			odV += CrossProduct(Front, Up) * Tan(DEGTORAD((CCamera::m_f3rdPersonCHairMultX - 0.5f) * 1.8f * 0.5f * odFov * CDraw::GetAspectRatio()));
			odV.Normalise();
			float odTV = odV.Heading() * RADTODEG(1.0f);
			float odPH = CamTargetEntity->GetMatrix().GetPosition().Heading();
			float odD = odTV - odPH;
			while (odD > 180.0f) odD -= 360.0f;
			while (odD < -180.0f) odD += 360.0f;
			CVector2D odCh(CCamera::m_f3rdPersonCHairMultX * RsGlobal.maximumWidth, CCamera::m_f3rdPersonCHairMultY * RsGlobal.maximumHeight);
			char tv[340];
			snprintf(tv, sizeof tv,
				"AIMVEC chx=%.3f chy=%.3f cfov=%.1f dfov=%.1f ax=%+.4f ay=%+.4f tv=%.1f ph=%.1f dth=%+.1f scx=%.0f scy=%.0f asp=%.3f",
				CCamera::m_f3rdPersonCHairMultX, CCamera::m_f3rdPersonCHairMultY,
				odFov, CDraw::GetFOV(), odAx, odAy, odTV, odPH, odD,
				odCh.x, odCh.y, CDraw::GetAspectRatio());
			ODTRACES(tv);
		}
		{
			// AIMFOV: el objetivo de la mira, para que se vea POR QUE se estrecha
			// y se compruebe que el Minigun no (CamNew.cpp:484-486).
			float odRange = curInfo ? curInfo->m_fRange : 0.0f;
			bool odTaken = curInfo && !curInfo->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM)
				&& ViceExtCanAim(curW->m_eWeaponType, curInfo)
				&& (odRange >= AIM_WEP_MIN_RANGE || ViceExtAimHeavy(curW->m_eWeaponType))
				&& curW->m_eWeaponType != WEAPONTYPE_MINIGUN;
			char t[160];
			snprintf(t, sizeof t, "AIMFOV fov=%.2f arma=%d alcance=%.1f taken=%d",
				FOV, (int)curW->m_eWeaponType, odRange, (int)odTaken);
			ODTRACES(t);
		}
		{
			// AIMCOL: las colisiones de ESTA ley, con el near-clip re-leido (el
			// que se midio, no el de partida). Formato = el de CAMB2b.
			char t[220];
			snprintf(t, sizeof t, "AIMCOL los=%d losD=%.2f sph=%d sphM=%d sphPed=%d dSph=%.2f dRaw=%.2f nc=%.3f app=%d pedes=%d",
				s_odLosHit, s_odLosD, s_odSphHit, s_odSphModel, s_odSphPed,
				(float)s_odSphApp, s_odSphD, s_odSphNear, s_odSphApp, s_odHideCount);
			ODTRACES(t);
		}
	}
#endif
	// `hadLock` se publica en la traza de lock-on que escribe B9; aqui solo se
	// deja constancia de que la rama se tomo (el `off=` va ahi, no aqui).
	(void)hadLock;
}

void
CCam::Process_Syphon(const CVector &CameraTarget, float, float, float)
{
	FOV = DefaultFOV;
#ifdef VICEEXT_RECOIL
	bool recoilReset = ResetStatics;
#endif

	if(!CamTargetEntity->IsPed())
		return;

	static bool CameraObscured = false;
	// unused FailedClippingTestPrevously
	static float BetaOffset = INIT_SYPHON_DEGREE_OFFSET;
	// unused AngleToGoTo
	// unused AngleToGoToSpeed
	// unused DistBetweenPedAndPlayerPreviouslyOn
	static float HeightDown = INIT_SYPHON_Z_OFFSET;
	static float AlphaOffset = INIT_SYPHON_ALPHA_OFFSET;
	static bool NegateBetaOffset = true;
	CVector TargetCoors;
	float fAimingDist;
	float TargetAlpha;

	bool StandingOnMovingThing = false;
	TargetCoors = CameraTarget;
	AlphaOffset = INIT_SYPHON_ALPHA_OFFSET;
	float GroundDist = INIT_SYPHON_GROUND_DIST;

	while(Beta >= PI) Beta -= 2*PI;
	while(Beta < -PI) Beta += 2*PI;

	float NewBeta = CGeneral::GetATanOfXY(TheCamera.m_cvecAimingTargetCoors.x - TargetCoors.x, TheCamera.m_cvecAimingTargetCoors.y - TargetCoors.y) + PI;
	if(ResetStatics){
		BetaOffset = INIT_SYPHON_DEGREE_OFFSET;
		Beta = CGeneral::GetATanOfXY(Source.x - TargetCoors.x, Source.y - TargetCoors.y);
		// some unuseds
		ResetStatics = false;
	}
	if(NegateBetaOffset)
		BetaOffset = -INIT_SYPHON_DEGREE_OFFSET;
	Beta = NewBeta + BetaOffset;
	Source = TargetCoors;
	Source.x += GroundDist*Cos(Beta);
	Source.y += GroundDist*Sin(Beta);
	CPhysical *ground = (CPhysical*)((CPed*)CamTargetEntity)->m_pCurSurface;
	if(ground && (ground->IsVehicle() || ground->IsObject()))
		StandingOnMovingThing = true;
	TargetCoors.z += m_fSyphonModeTargetZOffSet;

	bool PlayerTooClose = false;
	fAimingDist = (TheCamera.m_cvecAimingTargetCoors - TargetCoors).Magnitude2D();
	if(fAimingDist < 6.5f){
		fAimingDist = 6.5f;
		PlayerTooClose = true;
	}
	TargetAlpha = CGeneral::GetATanOfXY(fAimingDist, TheCamera.m_cvecAimingTargetCoors.z - TargetCoors.z);
	if(ResetStatics)	// BUG: can never happen
		Alpha = -TargetAlpha;
	while(TargetAlpha >= PI) TargetAlpha -= 2*PI;
	while(TargetAlpha < -PI) TargetAlpha += 2*PI;
	while(Alpha >= PI) Alpha -= 2*PI;
	while(Alpha < -PI) Alpha += 2*PI;

	// inlined
#ifdef VICEEXT_RECOIL
	CWeapon::ViceExtRecoilBegin(Alpha, recoilReset, Mode, "syphon");
#endif
	if(StandingOnMovingThing)
		WellBufferMe(-TargetAlpha, &Alpha, &AlphaSpeed, 0.07f/2.0f, 0.015f/2.0f, true);
	else
		WellBufferMe(-TargetAlpha, &Alpha, &AlphaSpeed, 0.07f, 0.015f, true);
#ifdef VICEEXT_RECOIL
	// Syphon es un solver de seguimiento, no input manual medible.
	CWeapon::ViceExtRecoilApply(Alpha, 0.0f, 0.0f, "unknown", Mode, -PI, PI);
#endif

	Source.z += GroundDist*Sin(Alpha+AlphaOffset) + GroundDist*0.2f;
	if(Source.z < TargetCoors.z + HeightDown)
		Source.z = TargetCoors.z + HeightDown;

	if(!PlayerTooClose){
		CColPoint point;
		CEntity *entity = nil;
		CWorld::pIgnoreEntity = CamTargetEntity;
		if(CWorld::ProcessLineOfSight(TheCamera.m_cvecAimingTargetCoors, Source, point, entity, true, false, false, true, false, false, true)){
			CVector TestFront = TheCamera.m_cvecAimingTargetCoors - Source;
			TestFront.Normalise();
			CVector CamToPlayer = CameraTarget - Source;
			CVector CamToCol = point.point - Source;
			if(DotProduct(TestFront, CamToCol) > DotProduct(TestFront, CamToPlayer)){
				// collision is beyond player
				float ColDist = (TheCamera.m_cvecAimingTargetCoors - point.point).Magnitude();
				CVector PlayerToTarget = TheCamera.m_cvecAimingTargetCoors - CameraTarget;
				float PlayerToTargetDist = PlayerToTarget.Magnitude();
				PlayerToTarget.Normalise();
				CVector Center = TheCamera.m_cvecAimingTargetCoors - ColDist*PlayerToTarget;
				float Radius = (point.point - Center).Magnitude();
				if(CWorld::TestSphereAgainstWorld(Center, Radius, nil, true, false, false, true, false, true)){
					CVector LineToCol = gaTempSphereColPoints[0].point - Center;
					LineToCol -= DotProduct(LineToCol, PlayerToTarget)*PlayerToTarget;
					// unused
					CVector LineToPrevCol = point.point - Center;
					LineToPrevCol -= DotProduct(LineToPrevCol, PlayerToTarget)*PlayerToTarget;
					float LineDist = LineToCol.Magnitude();
					float NewBetaOffset = 0.0f;
					if(LineDist > 0.0f && ColDist > 0.1f){
						// scale offset at center to offset at player
						float DistOffset = LineDist/ColDist * PlayerToTargetDist;
						// turn into an angle
						NewBetaOffset = 0.9f*Asin(Min(DistOffset/GroundDist, 1.0f));
					}
					if(NewBetaOffset < BetaOffset){
						float Ratio = NewBetaOffset / BetaOffset;
						BetaOffset = NewBetaOffset;
						Beta = NewBeta + NewBetaOffset;
						GroundDist *= Max(Ratio, 0.5f);
						Source.x = TargetCoors.x +  GroundDist*Cos(Beta);
						Source.y = TargetCoors.y +  GroundDist*Sin(Beta);
						Source.z += (1.0f-Ratio)*0.5f;
					}
				}
			}
		}
		CWorld::pIgnoreEntity = nil;
	}

	Front = TheCamera.m_cvecAimingTargetCoors - Source;
	float TargetDistGround = Front.Magnitude2D();
	Front.Normalise();
	m_cvecTargetCoorsForFudgeInter = Source + TargetDistGround*Front;
	m_cvecTargetCoorsForFudgeInter.z = TargetCoors.z;

	CVector OrigSource = Source;
	TheCamera.AvoidTheGeometry(OrigSource, CameraTarget + CVector(0.0f, 0.0f, 0.75f), Source, FOV);
	Source.z = OrigSource.z;

	GetVectorsReadyForRW();
}

void
CCam::Process_Syphon_Crim_In_Front(const CVector &CameraTarget, float, float, float)
{
	FOV = DefaultFOV;

	if(!CamTargetEntity->IsPed())
		return;

	CVector TargetCoors = CameraTarget;
	CVector vDist;
	float fDist, TargetDist;
	float zOffset;
	float AimingAngle;

	TargetDist = TheCamera.m_fPedZoomValueSmooth * 0.5f + 4.0f;
	vDist = Source - TargetCoors;
	fDist = vDist.Magnitude2D();
	zOffset = TargetDist - 2.65f;
	if(zOffset < 0.0f)
		zOffset = 0.0f;
	if(zOffset == 0.0f)
		Source = TargetCoors + CVector(1.0f, 1.0f, zOffset);
	else
		Source = TargetCoors + CVector(vDist.x/fDist*TargetDist, vDist.y/fDist*TargetDist, zOffset);

	AimingAngle = CGeneral::GetATanOfXY(TheCamera.m_cvecAimingTargetCoors.x - TargetCoors.x, TheCamera.m_cvecAimingTargetCoors.y - TargetCoors.y);
	while(AimingAngle >= PI) AimingAngle -= 2*PI;
	while(AimingAngle < -PI) AimingAngle += 2*PI;

	if(ResetStatics){
		if(AimingAngle > 0.0f)
			m_fPlayerInFrontSyphonAngleOffSet = -m_fPlayerInFrontSyphonAngleOffSet;
		ResetStatics = false;
	}

	if(TheCamera.PlayerWeaponMode.Mode == MODE_SYPHON)
		Beta = AimingAngle + m_fPlayerInFrontSyphonAngleOffSet;

	Source.x = TargetCoors.x;
	Source.y = TargetCoors.y;
	Source.x += Cos(Beta) * TargetDist;
	Source.y += Sin(Beta) * TargetDist;

	TargetCoors = CameraTarget;
	TargetCoors.z += m_fSyphonModeTargetZOffSet;
	m_cvecTargetCoorsForFudgeInter = TargetCoors;

	CVector OrigSource = Source;
	TheCamera.AvoidTheGeometry(OrigSource, TargetCoors, Source, FOV);

	Front = TargetCoors - Source;
	GetVectorsReadyForRW();
}

float MAX_HEIGHT_UP = 15.0f;
float WATER_Z_ADDITION = 2.75f;
float WATER_Z_ADDITION_MIN = 1.5f;
float SMALLBOAT_CLOSE_ALPHA_MINUS = 0.2f;
float afBoatBetaDiffMult[3] = { 0.15f, 0.07f, 0.01f };
float afBoatBetaSpeedDiffMult[3] = { 0.02f, 0.015f, 0.005f };

void
CCam::Process_BehindBoat(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	if(!CamTargetEntity->IsVehicle()){
		ResetStatics = false;
		return;
	}

	CVector TargetCoors = CameraTarget;
	float DeltaBeta = 0.0f;
	static float TargetWhenChecksWereOn = 0.0f;
	static float CenterObscuredWhenChecksWereOn = 0.0f;
	static float WaterZAddition = 2.75f;
	float WaterLevel = 0.0f;
	float MaxHeightUp = MAX_HEIGHT_UP;
	static float WaterLevelBuffered = 0.0f;
	static float WaterLevelSpeed = 0.0f;
	float BetaDiffMult = 0.0f;
	float BetaSpeedDiffMult = 0.0f;

	Beta = CGeneral::GetATanOfXY(TargetCoors.x - Source.x, TargetCoors.y - Source.y);
	FOV = DefaultFOV;
	float TargetAlpha = 0.0f;

	if(ResetStatics){
		CenterObscuredWhenChecksWereOn = 0.0f;
		TargetWhenChecksWereOn = 0.0f;
	}else if(DirectionWasLooking != LOOKING_FORWARD)
		Beta = TargetOrientation;

	if(!CWaterLevel::GetWaterLevelNoWaves(TargetCoors.x, TargetCoors.y, TargetCoors.z, &WaterLevel))
		WaterLevel = TargetCoors.z - 0.5f;
	if(ResetStatics){
		WaterLevelBuffered = WaterLevel;
		WaterLevelSpeed = 0.0f;
	}
	WellBufferMe(WaterLevel, &WaterLevelBuffered, &WaterLevelSpeed, 0.2f, 0.07f, false);

	static float FixerForGoingBelowGround = 0.4f;
	if(-FixerForGoingBelowGround < TargetCoors.z-WaterLevelBuffered+WATER_Z_ADDITION)
		WaterLevelBuffered += TargetCoors.z-WaterLevelBuffered+WATER_Z_ADDITION - FixerForGoingBelowGround;

	CVector BoatDimensions = CamTargetEntity->GetColModel()->boundingBox.GetSize();
	float BoatSize = BoatDimensions.Magnitude2D();
	int index = 0;
	TheCamera.GetArrPosForVehicleType(((CVehicle*)CamTargetEntity)->GetVehicleAppearance(), index);
	if(TheCamera.CarZoomIndicator == CAM_ZOOM_1){
		TargetAlpha = ZmOneAlphaOffset[index];
		BetaDiffMult = afBoatBetaDiffMult[0];
		BetaSpeedDiffMult = afBoatBetaSpeedDiffMult[0];
	}else if(TheCamera.CarZoomIndicator == CAM_ZOOM_2){
		TargetAlpha = ZmTwoAlphaOffset[index];
		BetaDiffMult = afBoatBetaDiffMult[1];
		BetaSpeedDiffMult = afBoatBetaSpeedDiffMult[1];
	}else if(TheCamera.CarZoomIndicator == CAM_ZOOM_3){
		TargetAlpha = ZmThreeAlphaOffset[index];
		BetaDiffMult = afBoatBetaDiffMult[2];
		BetaSpeedDiffMult = afBoatBetaSpeedDiffMult[2];
	}
	if(TheCamera.CarZoomIndicator == CAM_ZOOM_1 && BoatSize < 10.0f){
		TargetAlpha -= SMALLBOAT_CLOSE_ALPHA_MINUS;
		BoatSize = 10.0f;
	}else if(CCullZones::Cam1stPersonForPlayer()){
		float Water = 0.0f;
		// useless call
		//CWaterLevel::GetWaterLevelNoWaves(TargetCoors.x, TargetCoors.y, TargetCoors.z, &Water);
		Water = (WaterLevel + WATER_Z_ADDITION_MIN - WaterLevelBuffered - WATER_Z_ADDITION)/(BoatDimensions.z/2.0f + MaxHeightUp);
		TargetAlpha = Asin(Clamp(Water, -1.0f, 1.0f));
	}

	if(ResetStatics){
		Alpha = TargetAlpha;
		AlphaSpeed = 0.0f;
	}
	WellBufferMe(TargetAlpha, &Alpha, &AlphaSpeed, 0.15f, 0.07f, true);

	if(ResetStatics){
		Beta = TargetOrientation;
		DeltaBeta = 0.0f;
	}
	// inlined
	WellBufferMe(TargetOrientation, &Beta, &BetaSpeed, BetaDiffMult * ((CVehicle*)CamTargetEntity)->m_vecMoveSpeed.Magnitude(), BetaSpeedDiffMult, true);

	Source = (TheCamera.CarZoomValueSmooth+BoatSize) * CVector(-Cos(Beta), -Sin(Beta), 0.0f) + TargetCoors;
	Source.z = WaterLevelBuffered + WATER_Z_ADDITION + (BoatDimensions.z/2.0f + MaxHeightUp) * Sin(Alpha);

	m_cvecTargetCoorsForFudgeInter = TargetCoors;
	CVector OrigSource = Source;
	TheCamera.AvoidTheGeometry(OrigSource, TargetCoors, Source, FOV);
	Front = TargetCoors - Source;
	Front.Normalise();


	float TargetRoll;
	if(CPad::GetPad(0)->GetDPadLeft() || CPad::GetPad(0)->GetDPadRight()){
#ifdef FIX_BUGS
		float fwdSpeed = 180.0f*DotProduct(((CVehicle*)CamTargetEntity)->m_vecMoveSpeed, CamTargetEntity->GetForward());
		if(fwdSpeed > 210.0f) fwdSpeed = 210.0f;
#endif
		if(CPad::GetPad(0)->GetDPadLeft())
			TargetRoll = DEGTORAD(10.0f)*TiltOverShoot[index] + f_max_role_angle;
		else
			TargetRoll = -(DEGTORAD(10.0f)*TiltOverShoot[index] + f_max_role_angle);
		CVector FwdTarget = CamTargetEntity->GetForward();
		FwdTarget.Normalise();
		float AngleDiff = DotProduct(FwdTarget, Front);
		AngleDiff = Acos(Min(Abs(AngleDiff), 1.0f));
#ifdef FIX_BUGS
		TargetRoll *= fwdSpeed/210.0f * Sin(AngleDiff);
#else
		TargetRoll *= Sin(AngleDiff);
#endif
	}else{
		float fwdSpeed = 180.0f*DotProduct(((CVehicle*)CamTargetEntity)->m_vecMoveSpeed, CamTargetEntity->GetForward());
		if(fwdSpeed > 210.0f) fwdSpeed = 210.0f;
		TargetRoll = CPad::GetPad(0)->GetLeftStickX()/128.0f * fwdSpeed/210.0f;
		CVector FwdTarget = CamTargetEntity->GetForward();
		FwdTarget.Normalise();
		float AngleDiff = DotProduct(FwdTarget, Front);
		AngleDiff = Acos(Min(Abs(AngleDiff), 1.0f));
		TargetRoll *= (DEGTORAD(10.0f)*TiltOverShoot[index] + f_max_role_angle) * Sin(AngleDiff);
	}

	WellBufferMe(TargetRoll, &f_Roll, &f_rollSpeed, 0.15f, 0.07f, false);
	Up = CVector(Cos(f_Roll + HALFPI), 0.0f, Sin(f_Roll + HALFPI));
	Up.Normalise();
	Front.Normalise();
	CVector Left = CrossProduct(Up, Front);
	Left.Normalise();
	Up = CrossProduct(Front, Left);
	Up.Normalise();

	ResetStatics = false;
}

float FIGHT_HORIZ_DIST = 3.0f;
float FIGHT_VERT_DIST = 1.0f;
float FIGHT_BETA_ANGLE = 125.0f;

void
CCam::Process_Fight_Cam(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	if(!CamTargetEntity->IsPed())
		return;

	FOV = DefaultFOV;
	float HorizDist = FIGHT_HORIZ_DIST;
	float VertDist = FIGHT_VERT_DIST;
	float BetaLeft, BetaRight, DeltaBetaLeft, DeltaBetaRight;
	static bool PreviouslyFailedBuildingChecks = false;
	float TargetCamHeight;
	CVector TargetCoors;

	m_fMinDistAwayFromCamWhenInterPolating = FIGHT_HORIZ_DIST;
	Front = Source - CameraTarget;
	if(ResetStatics)
		Beta = CGeneral::GetATanOfXY(Front.x, Front.y);
	while(TargetOrientation >= PI) TargetOrientation -= 2*PI;
	while(TargetOrientation < -PI) TargetOrientation += 2*PI;
	while(Beta >= PI) Beta -= 2*PI;
	while(Beta < -PI) Beta += 2*PI;

	// Figure out Beta
	BetaLeft = TargetOrientation - DEGTORAD(FIGHT_BETA_ANGLE);
	BetaRight = TargetOrientation + DEGTORAD(FIGHT_BETA_ANGLE);
	DeltaBetaLeft = Beta - BetaLeft;
	DeltaBetaRight = Beta - BetaRight;
	while(DeltaBetaLeft >= PI) DeltaBetaLeft -= 2*PI;
	while(DeltaBetaLeft < -PI) DeltaBetaLeft += 2*PI;
	while(DeltaBetaRight >= PI) DeltaBetaRight -= 2*PI;
	while(DeltaBetaRight < -PI) DeltaBetaRight += 2*PI;

	if(ResetStatics){
		if(Abs(DeltaBetaLeft) < Abs(DeltaBetaRight))
			m_fTargetBeta = DeltaBetaLeft;
		else
			m_fTargetBeta = DeltaBetaRight;
		m_fBufferedTargetOrientation = TargetOrientation;
		m_fBufferedTargetOrientationSpeed = 0.0f;
		m_bCollisionChecksOn = true;
		BetaSpeed = 0.0f;
	}else if(CPad::GetPad(0)->WeaponJustDown()){
		if(Abs(DeltaBetaLeft) < Abs(DeltaBetaRight))
			m_fTargetBeta = DeltaBetaLeft;
		else
			m_fTargetBeta = DeltaBetaRight;
	}

	WellBufferMe(m_fTargetBeta, &Beta, &BetaSpeed, 0.015f, 0.007f, true);

	Source = CameraTarget + HorizDist*CVector(Cos(Beta), Sin(Beta), 0.0f);
	Source.z += VertDist;

	WellBufferMe(TargetOrientation, &m_fBufferedTargetOrientation, &m_fBufferedTargetOrientationSpeed, 0.07f, 0.004f, true);
	TargetCoors = CameraTarget + 0.1f*CVector(Cos(m_fBufferedTargetOrientation), Sin(m_fBufferedTargetOrientation), 0.0f);

	TargetCamHeight = CameraTarget.z - Source.z + Max(m_fPedBetweenCameraHeightOffset, m_fDimensionOfHighestNearCar) + VertDist;
	if(TargetCamHeight > m_fCamBufferedHeight)
		WellBufferMe(TargetCamHeight, &m_fCamBufferedHeight, &m_fCamBufferedHeightSpeed, 0.15f, 0.04f, false);
	else
		WellBufferMe(0.0f, &m_fCamBufferedHeight, &m_fCamBufferedHeightSpeed, 0.08f, 0.0175f, false);
	Source.z += m_fCamBufferedHeight;

	m_cvecTargetCoorsForFudgeInter = TargetCoors;
	CVector OrigSource = Source;
	TheCamera.AvoidTheGeometry(OrigSource, TargetCoors, Source, FOV);
	Front = TargetCoors - Source;
	Front.Normalise();
	GetVectorsReadyForRW();

	ResetStatics = false;
}

/*
// Spline format is this, but game doesn't seem to use any kind of struct:
struct Spline
{
	float numFrames;
	struct {
		float time;
		float f[3];	// CVector for Vector spline
	} frames[1];	// numFrames
};
*/

// These two functions are pretty ugly

#define MS(t) (uint32)((t)*1000.0f)

void
FindSplinePathPositionFloat(float *out, float *spline, uint32 time, uint32 &marker)
{
	// marker is at time
	uint32 numFrames = spline[0];
	uint32 timeDelta = MS(spline[marker] - spline[marker-4]);
	uint32 endTime = MS(spline[4*(numFrames-1) + 1]);
	if(time < endTime){
		bool canAdvance = true;
		if((marker-1)/4 > numFrames){
			canAdvance = false;
			marker = 4*(numFrames-1) + 1;
		}
		// skipping over small time deltas apparently?
		while(timeDelta <= 75 && canAdvance){
			marker += 4;
			if((marker-1)/4 > numFrames){
				canAdvance = false;
				marker = 4*(numFrames-1) + 1;
			}
			timeDelta = (spline[marker] - spline[marker-4]) * 1000.0f;
		}
	}
	float a = ((float)time - (float)MS(spline[marker-4])) / (float)MS(spline[marker] - spline[marker-4]);
	a = Clamp(a, 0.0f, 1.0f);
	float b = 1.0f - a;
	*out =	b*b*b * spline[marker-3] +
		3.0f*a*b*b * spline[marker-1] +
		3.0f*a*a*b * spline[marker+2] +
		a*a*a * spline[marker+1];
}

void
FindSplinePathPositionVector(CVector *out, float *spline, uint32 time, uint32 &marker)
{
	// marker is at time
	uint32 numFrames = spline[0];
	uint32 timeDelta = MS(spline[marker] - spline[marker-10]);
	uint32 endTime = MS(spline[10*(numFrames-1) + 1]);
	if(time < endTime){
		bool canAdvance = true;
		if((marker-1)/10 > numFrames){
			canAdvance = false;
			marker = 10*(numFrames-1) + 1;
		}
		// skipping over small time deltas apparently?
		while(timeDelta <= 75 && canAdvance){
			marker += 10;
			if((marker-1)/10 > numFrames){
				canAdvance = false;
				marker = 10*(numFrames-1) + 1;
			}
			timeDelta = (spline[marker] - spline[marker-10]) * 1000.0f;
		}
	}

	if((marker-1)/10 > numFrames){
		printf("Arraymarker %i \n", marker);
		printf("Path zero %i \n", numFrames);
	}

	float a = ((float)time - (float)MS(spline[marker-10])) / (float)MS(spline[marker] - spline[marker-10]);
	a = Clamp(a, 0.0f, 1.0f);
	float b = 1.0f - a;
	out->x =
		b*b*b * spline[marker-9] +
		3.0f*a*b*b * spline[marker-3] +
		3.0f*a*a*b * spline[marker+4] +
		a*a*a * spline[marker+1];
	out->y =
		b*b*b * spline[marker-8] +
		3.0f*a*b*b * spline[marker-2] +
		3.0f*a*a*b * spline[marker+5] +
		a*a*a * spline[marker+2];
	out->z =
		b*b*b * spline[marker-7] +
		3.0f*a*b*b * spline[marker-1] +
		3.0f*a*a*b * spline[marker+6] +
		a*a*a * spline[marker+3];
	*out += TheCamera.m_vecCutSceneOffset;
}

void
CCam::Process_FlyBy(const CVector&, float, float, float)
{
	float UpAngle = 0.0f;
	static float FirstFOVValue = 0.0f;
	static float PsuedoFOV;
	static uint32 ArrayMarkerFOV;
	static uint32 ArrayMarkerUp;
	static uint32 ArrayMarkerSource;
	static uint32 ArrayMarkerFront;

	if(TheCamera.m_bcutsceneFinished)
		return;
#ifdef FIX_BUGS
	// this would crash, not nice when cycling debug mode
	if(TheCamera.m_arrPathArray[0].m_arr_PathData == nil)
		return;
#endif

	Up = CVector(0.0f, 0.0f, 1.0f);
	if(TheCamera.m_bStartingSpline)
		m_fTimeElapsedFloat += CTimer::GetTimeStepNonClippedInMilliseconds();
	else{
		m_fTimeElapsedFloat = 0.0f;
		m_uiFinishTime = MS(TheCamera.m_arrPathArray[2].m_arr_PathData[10*((int)TheCamera.m_arrPathArray[2].m_arr_PathData[0]-1) + 1]);
		TheCamera.m_bStartingSpline = true;
		FirstFOVValue = TheCamera.m_arrPathArray[0].m_arr_PathData[2];
		PsuedoFOV = TheCamera.m_arrPathArray[0].m_arr_PathData[2];
		ArrayMarkerFOV = 5;
		ArrayMarkerUp = 5;
		ArrayMarkerSource = 11;
		ArrayMarkerFront = 11;
	}

	float fTime = m_fTimeElapsedFloat;
	uint32 uiFinishTime = m_uiFinishTime;
	uint32 uiTime = fTime;
	if(uiTime < uiFinishTime){
		TheCamera.m_fPositionAlongSpline = (float) uiTime / uiFinishTime;

		while(uiTime >= (TheCamera.m_arrPathArray[2].m_arr_PathData[ArrayMarkerSource] - TheCamera.m_arrPathArray[2].m_arr_PathData[1])*1000.0f)
			ArrayMarkerSource += 10;
		FindSplinePathPositionVector(&Source, TheCamera.m_arrPathArray[2].m_arr_PathData, uiTime, ArrayMarkerSource);

		while(uiTime >= (TheCamera.m_arrPathArray[3].m_arr_PathData[ArrayMarkerFront] - TheCamera.m_arrPathArray[3].m_arr_PathData[1])*1000.0f)
			ArrayMarkerFront += 10;
		FindSplinePathPositionVector(&Front, TheCamera.m_arrPathArray[3].m_arr_PathData, uiTime, ArrayMarkerFront);

		while(uiTime >= (TheCamera.m_arrPathArray[1].m_arr_PathData[ArrayMarkerUp] - TheCamera.m_arrPathArray[1].m_arr_PathData[1])*1000.0f)
			ArrayMarkerUp += 4;
		FindSplinePathPositionFloat(&UpAngle, TheCamera.m_arrPathArray[1].m_arr_PathData, uiTime, ArrayMarkerUp);
		UpAngle = DEGTORAD(UpAngle) + HALFPI;
		Up.x = Cos(UpAngle);
		Up.z = Sin(UpAngle);

		while(uiTime >= (TheCamera.m_arrPathArray[0].m_arr_PathData[ArrayMarkerFOV] - TheCamera.m_arrPathArray[0].m_arr_PathData[1])*1000.0f)
			ArrayMarkerFOV += 4;
		FindSplinePathPositionFloat(&PsuedoFOV, TheCamera.m_arrPathArray[0].m_arr_PathData, uiTime, ArrayMarkerFOV);

		m_cvecTargetCoorsForFudgeInter = Front;
		Front = Front - Source;
		Front.Normalise();
		CVector Left = CrossProduct(Up, Front);
		Up = CrossProduct(Front, Left);
		Up.Normalise();
	}else if(uiTime >= uiFinishTime){
		// end
		ArrayMarkerSource = (TheCamera.m_arrPathArray[2].m_arr_PathData[0] - 1)*10 + 1;
		ArrayMarkerFront = (TheCamera.m_arrPathArray[3].m_arr_PathData[0] - 1)*10 + 1;
		ArrayMarkerUp = (TheCamera.m_arrPathArray[1].m_arr_PathData[0] - 1)*4 + 1;
		ArrayMarkerFOV = (TheCamera.m_arrPathArray[0].m_arr_PathData[0] - 1)*4 + 1;

		FindSplinePathPositionVector(&Source, TheCamera.m_arrPathArray[2].m_arr_PathData, uiTime, ArrayMarkerSource);
		FindSplinePathPositionVector(&Front, TheCamera.m_arrPathArray[3].m_arr_PathData, uiTime, ArrayMarkerFront);
		FindSplinePathPositionFloat(&UpAngle, TheCamera.m_arrPathArray[1].m_arr_PathData, uiTime, ArrayMarkerUp);
		UpAngle = DEGTORAD(UpAngle) + HALFPI;
		Up.x = Cos(UpAngle);
		Up.z = Sin(UpAngle);
		FindSplinePathPositionFloat(&PsuedoFOV, TheCamera.m_arrPathArray[0].m_arr_PathData, uiTime, ArrayMarkerFOV);

		TheCamera.m_fPositionAlongSpline = 1.0f;
		ArrayMarkerFOV = 0;
		ArrayMarkerUp = 0;
		ArrayMarkerSource = 0;
		ArrayMarkerFront = 0;

		m_cvecTargetCoorsForFudgeInter = Front;
		Front = Front - Source;
		Front.Normalise();
		CVector Left = CrossProduct(Up, Front);
		Up = CrossProduct(Front, Left);
		Up.Normalise();
	}
	FOV = PsuedoFOV;
}

CVector vecWheelCamBoatOffset(-0.5f, -0.8f, 0.3f);
CVector vecWheelCamBoatOffsetAlt(0.2f, -0.2f, -0.3f);
float fWheelCamCarXOffset = 0.33f;
float fWheelCamBikeXOffset = 0.2f;

bool
CCam::Process_WheelCam(const CVector&, float, float, float)
{
	FOV = DefaultFOV;

	CVector WheelPos;
	if(CamTargetEntity->IsPed()){
		// what? ped with wheels or what?
		Source = Multiply3x3(CamTargetEntity->GetMatrix(), CVector(-0.3f, -0.5f, 0.1f));
		Source += CamTargetEntity->GetPosition();
		Front = CVector(1.0f, 0.0f, 0.0f);
	}else{
		WheelPos = CamTargetEntity->GetColModel()->boundingBox.min;
		WheelPos.x -= 0.33f;
		WheelPos.y = -2.3f;
		WheelPos.z = 0.3f;
		Source = CamTargetEntity->GetMatrix() * WheelPos;
		Front = CamTargetEntity->GetForward();
	}

	CVector NewUp, Right;
	if(CamTargetEntity->IsVehicle() &&
	   (((CVehicle*)CamTargetEntity)->GetVehicleAppearance() == VEHICLE_APPEARANCE_HELI ||
	    ((CVehicle*)CamTargetEntity)->GetVehicleAppearance() == VEHICLE_APPEARANCE_PLANE)){
		WheelPos.x = -1.55f;
		Right = CamTargetEntity->GetRight();
		NewUp = CamTargetEntity->GetUp();
		Source = CamTargetEntity->GetMatrix() * WheelPos;
	}else if(CamTargetEntity->IsVehicle() && ((CVehicle*)CamTargetEntity)->IsBoat()){
		NewUp = CVector(0.0f, 0.0f, 1.0f);
		Right = CrossProduct(Front, NewUp);
		Right.Normalise();
		NewUp = CrossProduct(Right, Front);
		NewUp.Normalise();

		CVector BoatCamPos(0.0f, 0.0f, 0.0f);
		if(((CVehicle*)CamTargetEntity)->pDriver){
			((CVehicle*)CamTargetEntity)->pDriver->m_pedIK.GetComponentPosition(BoatCamPos, PED_HEAD);
			BoatCamPos += ((CVehicle*)CamTargetEntity)->m_vecMoveSpeed * CTimer::GetTimeStep();
			BoatCamPos += vecWheelCamBoatOffset.x * Right;
			BoatCamPos += vecWheelCamBoatOffset.y * CamTargetEntity->GetForward();
			BoatCamPos.z += vecWheelCamBoatOffset.z;
			if(CamTargetEntity->GetModelIndex() == MI_PREDATOR){
				BoatCamPos += vecWheelCamBoatOffsetAlt.x * Right;
				BoatCamPos += vecWheelCamBoatOffsetAlt.y * CamTargetEntity->GetForward();
				BoatCamPos.z += vecWheelCamBoatOffsetAlt.z;
			}
			Source = BoatCamPos;
		}else
			Source.z += 2.0f*vecWheelCamBoatOffset.z;
	}else if(CamTargetEntity->IsVehicle() && ((CVehicle*)CamTargetEntity)->IsBike()){
		NewUp = CVector(0.0f, 0.0f, 1.0f);
		Right = CrossProduct(Front, NewUp);
		Right.Normalise();
		NewUp = CrossProduct(Right, Front);
		NewUp.Normalise();

		WheelPos.z += fWheelCamCarXOffset - fWheelCamBikeXOffset;
		Source = CamTargetEntity->GetPosition();
		Source += WheelPos.x * CamTargetEntity->GetRight();
		Source += WheelPos.y * Front;
		Source += WheelPos.z * Up;
	}else{
		NewUp = CVector(0.0f, 0.0f, 1.0f);
		Right = CrossProduct(Front, NewUp);
		Right.Normalise();
		NewUp = CrossProduct(Right, Front);
		NewUp.Normalise();
	}

	float Roll = Cos((CTimer::GetTimeInMilliseconds()&0x1FFFF)/(float)0x1FFFF * TWOPI);
	Up = Cos(Roll*0.4f)*NewUp + Sin(Roll*0.4f)*Right;

	CEntity *entity = nil;
	CColPoint point;
	CWorld::pIgnoreEntity = CamTargetEntity;
	bool blocked = CWorld::ProcessLineOfSight(Source, CamTargetEntity->GetPosition(), point, entity, true, false, false, true, false, false, true);
	CWorld::pIgnoreEntity = nil;
	return !blocked;
}

int BOAT_UNDERWATER_CAM_BLUR = 20;
float BOAT_UNDERWATER_CAM_COLORMAG_LIMIT = 10.0f;

//--MIAIM: done
void
CCam::Process_Fixed(const CVector &CameraTarget, float, float, float)
{
	if(DirectionWasLooking != LOOKING_FORWARD)
		DirectionWasLooking = LOOKING_FORWARD;

	Source = m_cvecCamFixedModeSource;
	Front = CameraTarget - Source;
	Front.Normalise();
	m_cvecTargetCoorsForFudgeInter = CameraTarget;
	GetVectorsReadyForRW();

	Up = CVector(0.0f, 0.0f, 1.0f) + m_cvecCamFixedModeUpOffSet;
	Up.Normalise();
	CVector Right = CrossProduct(Front, Up);
	Right.Normalise();
	Up = CrossProduct(Right, Front);

	FOV = DefaultFOV;
	if(TheCamera.m_bUseSpecialFovTrain)
		FOV = TheCamera.m_fFovForTrain;

	float WaterZ = 0.0f;
	if(CWaterLevel::GetWaterLevel(Source, &WaterZ, true) && Source.z < WaterZ){
		float WaterLum = Sqrt(SQR(CTimeCycle::GetWaterRed()) + SQR(CTimeCycle::GetWaterGreen()) + SQR(CTimeCycle::GetWaterBlue()));
		if(WaterLum > BOAT_UNDERWATER_CAM_COLORMAG_LIMIT){
			float f = BOAT_UNDERWATER_CAM_COLORMAG_LIMIT/WaterLum;
			TheCamera.SetMotionBlur(CTimeCycle::GetWaterRed()*f,
				CTimeCycle::GetWaterGreen()*f,
				CTimeCycle::GetWaterBlue()*f, BOAT_UNDERWATER_CAM_BLUR, MOTION_BLUR_LIGHT_SCENE);
		}else{
			TheCamera.SetMotionBlur(CTimeCycle::GetWaterRed(),
				CTimeCycle::GetWaterGreen(),
				CTimeCycle::GetWaterBlue(), BOAT_UNDERWATER_CAM_BLUR, MOTION_BLUR_LIGHT_SCENE);
		}
	}

#ifdef PC_PLAYER_CONTROLS
	if(FrontEndMenuManager.m_ControlMethod == CONTROL_STANDARD && Using3rdPersonMouseCam()){
		CPed *player = FindPlayerPed();
		if(player && player->CanStrafeOrMouseControl()){
			float Heading = Front.Heading();
			((CPed*)TheCamera.pTargetEntity)->m_fRotationCur = Heading;
			((CPed*)TheCamera.pTargetEntity)->m_fRotationDest = Heading;
			TheCamera.pTargetEntity->SetHeading(Heading);
			TheCamera.pTargetEntity->GetMatrix().UpdateRW();
		}
	}
#endif
}

void
CCam::Process_LightHouse(const CVector &CameraTarget, float, float, float)
{
	static float Timer;

	Source = CameraTarget;
	Source.x = 474.3f;
	Source.y = -1717.6f;

	int CamMode;
	if(CameraTarget.z > 57.0f && (CameraTarget-Source).Magnitude2D() > 3.2f){
		// Outside at top
		if(Timer > 0.0f){
			Timer -= CTimer::GetTimeStep();
			CamMode = 1;
		}else{
			Timer = -24.0f;
			CamMode = 2;
		}
	}else if(CameraTarget.z > 57.0f){
		// Inside at top
		if(Timer < 0.0f){
			Timer += CTimer::GetTimeStep();
			CamMode = 2;
		}else{
			Timer = 24.0f;
			CamMode = 1;
		}
	}else{
		Timer = 0.0f;
		CamMode = 0;
	}

	if(CamMode == 2){
		Source.z = 57.5f;
		Front = Source - CameraTarget;
		Front.Normalise();
		Source.x = CameraTarget.x - 5.0f*Front.x;
		Source.y = CameraTarget.y - 5.0f*Front.y;
	}else if(CamMode == 1){
		Front = CameraTarget - Source;
		Front.Normalise();
		Source.x = CameraTarget.x - 2.0f*Front.x;
		Source.y = CameraTarget.y - 2.0f*Front.y;
	}else{
		Source.z += 4.0f;
		Front = CameraTarget - Source;
		Front.Normalise();
		Source -= 4.0f*Front;
		Source.z = Min(Source.z, 55.0f);
		Front = CameraTarget - Source;
	}

	m_cvecTargetCoorsForFudgeInter = CameraTarget;
	GetVectorsReadyForRW();

	Up = CVector(0.0f, 0.0f, 1.0f) + m_cvecCamFixedModeUpOffSet;
	Up.Normalise();
	CVector Right = CrossProduct(Front, Up);
	Right.Normalise();
	Up = CrossProduct(Right, Front);

	FOV = DefaultFOV;
	if(TheCamera.m_bUseSpecialFovTrain)	// uh, sure...
		FOV = TheCamera.m_fFovForTrain;
}

void
CCam::Process_Player_Fallen_Water(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	CColPoint colPoint;
	CEntity *entity = nil;

	FOV = DefaultFOV;
	Source = m_vecLastAboveWaterCamPosition;
	Source.z += 4.0f;

	m_cvecTargetCoorsForFudgeInter = CameraTarget;
	Front = CameraTarget - Source;
	Front.Normalise();
	if(CWorld::ProcessLineOfSight(CameraTarget, Source, colPoint, entity, true, false, false, true, false, true, true))
		Source = colPoint.point;
	GetVectorsReadyForRW();
	Front = CameraTarget - Source;
	Front.Normalise();
}

void
CCam::Process_SpecialFixedForSyphon(const CVector &CameraTarget, float, float, float)
{
	Source = m_cvecCamFixedModeSource;
	m_cvecTargetCoorsForFudgeInter = CameraTarget;
	m_cvecTargetCoorsForFudgeInter.z += m_fSyphonModeTargetZOffSet;
	Front = CameraTarget - Source;
	CVector OrigSource = Source;
	TheCamera.AvoidTheGeometry(OrigSource, m_cvecTargetCoorsForFudgeInter, Source, FOV);
	Front.z += m_fSyphonModeTargetZOffSet;

	GetVectorsReadyForRW();

	Up += m_cvecCamFixedModeUpOffSet;
	Up.Normalise();
	CVector Left = CrossProduct(Up, Front);
	Left.Normalise();
	Front = CrossProduct(Left, Up);
	Front.Normalise();
	FOV = DefaultFOV;
}

#ifdef IMPROVED_CAMERA

#define KEYJUSTDOWN(k) ControlsManager.GetIsKeyboardKeyJustDown((RsKeyCodes)k)
#define KEYDOWN(k) ControlsManager.GetIsKeyboardKeyDown((RsKeyCodes)k)
#define CTRLJUSTDOWN(key) \
	       ((KEYDOWN(rsLCTRL) || KEYDOWN(rsRCTRL)) && KEYJUSTDOWN((RsKeyCodes)key) || \
	        (KEYJUSTDOWN(rsLCTRL) || KEYJUSTDOWN(rsRCTRL)) && KEYDOWN((RsKeyCodes)key))
#define CTRLDOWN(key) ((KEYDOWN(rsLCTRL) || KEYDOWN(rsRCTRL)) && KEYDOWN((RsKeyCodes)key))


void
CCam::Process_Debug(const CVector&, float, float, float)
{
	static float Speed = 0.0f;
	static float PanSpeedX = 0.0f;
	static float PanSpeedY = 0.0f;
	CVector TargetCoors;

	RwCameraSetNearClipPlane(Scene.camera, DEFAULT_NEAR);
	FOV = DefaultFOV;
	Alpha += DEGTORAD(CPad::GetPad(1)->GetLeftStickY()) / 50.0f;
	Beta  += DEGTORAD(CPad::GetPad(1)->GetLeftStickX()*1.5f) / 19.0f;
	if(CPad::GetPad(0)->GetLeftMouse()){
		Alpha += DEGTORAD(CPad::GetPad(0)->GetMouseY()/2.0f);
		Beta += DEGTORAD(CPad::GetPad(0)->GetMouseX()/2.0f);
	}

	TargetCoors.x = Source.x + Cos(Alpha) * Sin(Beta) * 7.0f;
	TargetCoors.y = Source.y + Cos(Alpha) * Cos(Beta) * 7.0f;
	TargetCoors.z = Source.z + Sin(Alpha) * 3.0f;

	if(Alpha > DEGTORAD(89.5f)) Alpha = DEGTORAD(89.5f);
	else if(Alpha < DEGTORAD(-89.5f)) Alpha = DEGTORAD(-89.5f);

	if(CPad::GetPad(1)->GetSquare() || KEYDOWN('W'))
		Speed += 0.1f;
	else if(CPad::GetPad(1)->GetCross() || KEYDOWN('S'))
		Speed -= 0.1f;
	else
		Speed = 0.0f;
	if(Speed > 70.0f) Speed = 70.0f;
	if(Speed < -70.0f) Speed = -70.0f;


	if(KEYDOWN(rsRIGHT) || KEYDOWN('D'))
		PanSpeedX += 0.1f;
	else if(KEYDOWN(rsLEFT) || KEYDOWN('A'))
		PanSpeedX -= 0.1f;
	else
		PanSpeedX = 0.0f;
	if(PanSpeedX > 70.0f) PanSpeedX = 70.0f;
	if(PanSpeedX < -70.0f) PanSpeedX = -70.0f;


	if(KEYDOWN(rsUP))
		PanSpeedY += 0.1f;
	else if(KEYDOWN(rsDOWN))
		PanSpeedY -= 0.1f;
	else
		PanSpeedY = 0.0f;
	if(PanSpeedY > 70.0f) PanSpeedY = 70.0f;
	if(PanSpeedY < -70.0f) PanSpeedY = -70.0f;


	Front = TargetCoors - Source;
	Front.Normalise();
	Source = Source + Front*Speed;

	Up = CVector{ 0.0f, 0.0f, 1.0f };
	CVector Right = CrossProduct(Front, Up);
	Up = CrossProduct(Right, Front);
	Source = Source + Up*PanSpeedY + Right*PanSpeedX;

	if(Source.z < -450.0f)
		Source.z = -450.0f;

	if(CPad::GetPad(1)->GetRightShoulder2JustDown() || KEYJUSTDOWN(rsENTER)){
		if(FindPlayerVehicle())
			FindPlayerVehicle()->Teleport(Source);
		else
			CWorld::Players[CWorld::PlayerInFocus].m_pPed->SetPosition(Source);
	}

	// stay inside sectors
	while(CWorld::GetSectorX(Source.x) > NUMSECTORS_X-5.0f)
		Source.x -= 1.0f;
	while(CWorld::GetSectorX(Source.x) < 5.0f)
		Source.x += 1.0f;
	while(CWorld::GetSectorY(Source.y) > NUMSECTORS_X-5.0f)
		Source.y -= 1.0f;
	while(CWorld::GetSectorY(Source.y) < 5.0f)
		Source.y += 1.0f;
	GetVectorsReadyForRW();

#ifdef FIX_BUGS
	CPad::GetPad(0)->SetDisablePlayerControls(PLAYERCONTROL_CAMERA);
#else
	CPad::GetPad(0)->DisablePlayerControls = PLAYERCONTROL_CAMERA;
#endif

	if(CPad::GetPad(1)->GetLeftShockJustDown() && gbBigWhiteDebugLightSwitchedOn)
		CShadows::StoreShadowToBeRendered(SHADOWTYPE_ADDITIVE, gpShadowExplosionTex, &Source,
			12.0f, 0.0f, 0.0f, -12.0f,
			128, 128, 128, 128, 1000.0f, false, 1.0f, nil, false);

	if(CHud::m_Wants_To_Draw_Hud){
		char str[256];
		sprintf(str, "CamX: %f CamY: %f  CamZ:  %f", Source.x, Source.y, Source.z);
		sprintf(str, "Frontx: %f, Fronty: %f, Frontz: %f ", Front.x, Front.y, Front.z);
		sprintf(str, "Look@: %f, Look@: %f, Look@: %f ", Front.x + Source.x, Front.y + Source.y, Front.z + Source.z);
	}
}
#else
void
CCam::Process_Debug(const CVector&, float, float, float)
{
	static float Speed = 0.0f;
	CVector TargetCoors;

	RwCameraSetNearClipPlane(Scene.camera, DEFAULT_NEAR);
	FOV = DefaultFOV;
	Alpha += DEGTORAD(CPad::GetPad(1)->GetLeftStickY()) / 50.0f;
	Beta  += DEGTORAD(CPad::GetPad(1)->GetLeftStickX()*1.5f) / 19.0f;

	TargetCoors.x = Source.x + Cos(Alpha) * Sin(Beta) * 7.0f;
	TargetCoors.y = Source.y + Cos(Alpha) * Cos(Beta) * 7.0f;
	TargetCoors.z = Source.z + Sin(Alpha) * 3.0f;

	if(Alpha > DEGTORAD(89.5f)) Alpha = DEGTORAD(89.5f);
	else if(Alpha < DEGTORAD(-89.5f)) Alpha = DEGTORAD(-89.5f);

	if(CPad::GetPad(1)->GetSquare() || CPad::GetPad(1)->GetLeftMouse())
		Speed += 0.1f;
	else if(CPad::GetPad(1)->GetCross() || CPad::GetPad(1)->GetRightMouse())
		Speed -= 0.1f;
	else
		Speed = 0.0f;
	if(Speed > 70.0f) Speed = 70.0f;
	if(Speed < -70.0f) Speed = -70.0f;

	Front = TargetCoors - Source;
	Front.Normalise();
	Source = Source + Front*Speed;

	if(Source.z < -450.0f)
		Source.z = -450.0f;

	if(CPad::GetPad(1)->GetRightShoulder2JustDown()){
		if(FindPlayerVehicle())
			FindPlayerVehicle()->Teleport(Source);
		else
			CWorld::Players[CWorld::PlayerInFocus].m_pPed->SetPosition(Source);
	}

	// stay inside sectors
	while(CWorld::GetSectorX(Source.x) > NUMSECTORS_X-5.0f)
		Source.x -= 1.0f;
	while(CWorld::GetSectorX(Source.x) < 5.0f)
		Source.x += 1.0f;
	while(CWorld::GetSectorY(Source.y) > NUMSECTORS_X-5.0f)
		Source.y -= 1.0f;
	while(CWorld::GetSectorY(Source.y) < 5.0f)
		Source.y += 1.0f;
	GetVectorsReadyForRW();

	if(CPad::GetPad(1)->GetLeftShockJustDown() && gbBigWhiteDebugLightSwitchedOn)
		CShadows::StoreShadowToBeRendered(SHADOWTYPE_ADDITIVE, gpShadowExplosionTex, &Source,
			12.0f, 0.0f, 0.0f, -12.0f,
			128, 128, 128, 128, 1000.0f, false, 1.0f, nil, 1.0f);

	if(CHud::m_Wants_To_Draw_Hud){
		char str[256];
		sprintf(str, "CamX: %f CamY: %f  CamZ:  %f", Source.x, Source.y, Source.z);
		sprintf(str, "Frontx: %f, Fronty: %f, Frontz: %f ", Front.x, Front.y, Front.z);
		sprintf(str, "Look@: %f, Look@: %f, Look@: %f ", Front.x + Source.x, Front.y + Source.y, Front.z + Source.z);
	}
}
#endif

#ifdef GTA_SCENE_EDIT
void
CCam::Process_Editor(const CVector&, float, float, float)
{
	static float Speed = 0.0f;
	CVector TargetCoors;

	if(ResetStatics){
		Source = CVector(796.0f, -937.0, 40.0f);
		CamTargetEntity = nil;
	}
	ResetStatics = false;

	RwCameraSetNearClipPlane(Scene.camera, DEFAULT_NEAR);
	FOV = DefaultFOV;
	Alpha += DEGTORAD(CPad::GetPad(1)->GetLeftStickY()) / 50.0f;
	Beta  += DEGTORAD(CPad::GetPad(1)->GetLeftStickX()*1.5f) / 19.0f;

	if(CamTargetEntity && CSceneEdit::m_bCameraFollowActor){
		TargetCoors = CamTargetEntity->GetPosition();
	}else if(CSceneEdit::m_bRecording){
		TargetCoors.x = Source.x + Cos(Alpha) * Sin(Beta) * 7.0f;
		TargetCoors.y = Source.y + Cos(Alpha) * Cos(Beta) * 7.0f;
		TargetCoors.z = Source.z + Sin(Alpha) * 7.0f;
	}else
		TargetCoors = CSceneEdit::m_vecCamHeading + Source;
	CSceneEdit::m_vecCurrentPosition = TargetCoors;
	CSceneEdit::m_vecCamHeading = TargetCoors - Source;

	if(Alpha > DEGTORAD(89.5f)) Alpha = DEGTORAD(89.5f);
	else if(Alpha < DEGTORAD(-89.5f)) Alpha = DEGTORAD(-89.5f);

	if(CPad::GetPad(1)->GetSquare() || CPad::GetPad(1)->GetLeftMouse())
		Speed += 0.1f;
	else if(CPad::GetPad(1)->GetCross() || CPad::GetPad(1)->GetRightMouse())
		Speed -= 0.1f;
	else
		Speed = 0.0f;
	if(Speed > 70.0f) Speed = 70.0f;
	if(Speed < -70.0f) Speed = -70.0f;

	Front = TargetCoors - Source;
	Front.Normalise();
	Source = Source + Front*Speed;

	if(Source.z < -450.0f)
		Source.z = -450.0f;

	if(CPad::GetPad(1)->GetRightShoulder2JustDown()){
		if(FindPlayerVehicle())
			FindPlayerVehicle()->Teleport(Source);
		else
			CWorld::Players[CWorld::PlayerInFocus].m_pPed->SetPosition(Source);
			
	}

	// stay inside sectors
	while(CWorld::GetSectorX(Source.x) > NUMSECTORS_X-5.0f)
		Source.x -= 1.0f;
	while(CWorld::GetSectorX(Source.x) < 5.0f)
		Source.x += 1.0f;
	while(CWorld::GetSectorY(Source.y) > NUMSECTORS_X-5.0f)
		Source.y -= 1.0f;
	while(CWorld::GetSectorY(Source.y) < 5.0f)
		Source.y += 1.0f;
	GetVectorsReadyForRW();

	if(CPad::GetPad(1)->GetLeftShockJustDown() && gbBigWhiteDebugLightSwitchedOn)
		CShadows::StoreShadowToBeRendered(SHADOWTYPE_ADDITIVE, gpShadowExplosionTex, &Source,
			12.0f, 0.0f, 0.0f, -12.0f,
			128, 128, 128, 128, 1000.0f, false, 1.0f, nil, false);

	if(CHud::m_Wants_To_Draw_Hud){
		char str[256];
		sprintf(str, "CamX: %f CamY: %f  CamZ:  %f", Source.x, Source.y, Source.z);
		sprintf(str, "Frontx: %f, Fronty: %f, Frontz: %f ", Front.x, Front.y, Front.z);
		sprintf(str, "Look@: %f, Look@: %f, Look@: %f ", Front.x + Source.x, Front.y + Source.y, Front.z + Source.z);
	}
}
#endif

void
CCam::Process_ModelView(const CVector &CameraTarget, float, float, float)
{
	CVector TargetCoors = CameraTarget;
	float Angle = Atan2(Front.x, Front.y);
	FOV = DefaultFOV;

	Angle += CPad::GetPad(0)->GetLeftStickX()/1280.0f;
	if(Distance < 10.0f)
		Distance += CPad::GetPad(0)->GetLeftStickY()/1000.0f;
	else
		Distance += CPad::GetPad(0)->GetLeftStickY() * ((Distance - 10.0f)/20.0f + 1.0f) / 1000.0f;
#ifdef IMPROVED_CAMERA
	if(CPad::GetPad(0)->GetLeftMouse()){
		Distance += DEGTORAD(CPad::GetPad(0)->GetMouseY()/2.0f);
		Angle += DEGTORAD(CPad::GetPad(0)->GetMouseX()/2.0f);
	}
#endif
	if(Distance < 1.5f)
		Distance = 1.5f;

	Front.x = Cos(0.3f) * Sin(Angle);
	Front.y = Cos(0.3f) * Cos(Angle);
	Front.z = -Sin(0.3f);
	Source = CameraTarget - Distance*Front;

	GetVectorsReadyForRW();
}

float DEADCAM_HEIGHT_START = 2.0f;
float DEADCAM_HEIGHT_RATE = 0.04f;
float DEADCAM_WAFT_AMPLITUDE = 2.0f;
float DEADCAM_WAFT_RATE = 600.0f;
float DEADCAM_WAFT_TILT_AMP = -0.35f;

void
CCam::ProcessPedsDeadBaby(void)
{
	CVector TargetCoors;
	CVector CamPos;

	if(TheCamera.pTargetEntity->IsPed())
		((CPed*)TheCamera.pTargetEntity)->m_pedIK.GetComponentPosition(TargetCoors, PED_MID);
	else if(TheCamera.pTargetEntity->IsVehicle()){
		TargetCoors = TheCamera.pTargetEntity->GetPosition();
		TargetCoors.z += TheCamera.pTargetEntity->GetColModel()->boundingBox.max.z;
	}else
		return;

	if(ResetStatics){
		TheCamera.m_uiTimeLastChange = CTimer::GetTimeInMilliseconds();
		CamPos = TargetCoors;
		CamPos.z += DEADCAM_HEIGHT_START;
		float WaterZ = 0.0f;
		if(CWaterLevel::GetWaterLevelNoWaves(TargetCoors.x, TargetCoors.y, TargetCoors.z, &WaterZ)){
			if(WaterZ + 1.5f > CamPos.z)
				CamPos.z = WaterZ + 1.5f;
		}
		CVector Right = CrossProduct(TheCamera.pTargetEntity->GetForward(), CVector(0.0f, 0.0f, 1.0f));
		Right.z = 0.0f;
		Right.Normalise();
		Front = TargetCoors - CamPos;
		Front.Normalise();
		Up = CrossProduct(Right, Front);
		Up.Normalise();
		ResetStatics = false;
	}else{
		CamPos = Source;
		if(CWorld::TestSphereAgainstWorld(CamPos+CVector(0.0f, 0.0f, 0.2f), 0.3f, TheCamera.pTargetEntity, true, true, false, true, false, true) == nil)
			CamPos.z += DEADCAM_HEIGHT_RATE*CTimer::GetTimeStep();
		CVector Right = CrossProduct(TheCamera.pTargetEntity->GetForward(), CVector(0.0f, 0.0f, 1.0f));
		Right.z = 0.0f;
		Right.Normalise();

		float Time = CTimer::GetTimeInMilliseconds() - TheCamera.m_uiTimeLastChange;
		CVector WaftOffset = DEADCAM_WAFT_AMPLITUDE * Min(1000.0f,Time)/1000.0f * Sin(Time/DEADCAM_WAFT_RATE) * Right;
		CVector WaftPos = TargetCoors + WaftOffset;
		WaftPos.z = CamPos.z;
		CVector WaftFront = WaftPos - CamPos;
		WaftFront.Normalise();
		if(CWorld::TestSphereAgainstWorld(CamPos+0.2f*WaftFront, 0.3f, TheCamera.pTargetEntity, true, true, false, true, false, true) == nil)
			CamPos = WaftPos;

		Front = CVector(0.0f, 0.0f, -1.0f);
		Front += Cos(Time/DEADCAM_WAFT_RATE) * DEADCAM_WAFT_TILT_AMP * Min(2000.0f,Time)/2000.0f * Right;

		Front.Normalise();
		Up = CrossProduct(Right, Front);
		Up.Normalise();
	}

	Source = CamPos;
	CVector OrigSource = Source;
	TheCamera.AvoidTheGeometry(OrigSource, TargetCoors, Source, FOV);
	TheCamera.m_bMoveCamToAvoidGeom = false;
}

float ARRESTDIST_BEHIND_COP = 5.0f;
float ARRESTDIST_RIGHTOF_COP = 3.0f;
float ARRESTDIST_ABOVE_COP = 1.4f;	// unused
float ARRESTDIST_MINFROM_PLAYER = 8.0f;
float ARRESTCAM_LAMP_BEST_DIST = 17.0f;
float ARRESTCAM_ROTATION_SPEED = 0.1f;
float ARRESTCAM_ROTATION_UP = 0.05f;
float ARRESTCAM_S_ROTATION_UP = 0.1f;
float ARRESTDIST_ALONG_GROUND = 5.0f;
float ARRESTDIST_SIDE_GROUND = 10.0f;
float ARRESTDIST_ABOVE_GROUND = 0.7f;
float ARRESTCAM_LAMPPOST_ROTATEDIST = 10.0f;
float ARRESTCAM_LAMPPOST_TRANSLATE = 0.1f;

bool
CCam::GetLookAlongGroundPos(CEntity *Target, CPed *Cop, CVector &TargetCoors, CVector &SourceOut)
{
	if(Target == nil || Cop == nil)
		return false;
	CVector CopToTarget = TargetCoors - Cop->GetPosition();
	CopToTarget.z = 0.0f;
	CopToTarget.Normalise();
	SourceOut = TargetCoors + ARRESTDIST_ALONG_GROUND*CopToTarget;
	CVector Side = CrossProduct(CopToTarget, CVector(0.0f, 0.0f, 1.0f));
	SourceOut += ARRESTDIST_SIDE_GROUND*Side;
	SourceOut.z += 5.0f;
	bool found = false;
	float ground = CWorld::FindGroundZFor3DCoord(SourceOut.x, SourceOut.y, SourceOut.z, &found);
	if(found)
		SourceOut.z = ground + ARRESTDIST_ABOVE_GROUND;
	return true;
}

bool
CCam::GetLookFromLampPostPos(CEntity *Target, CPed *Cop, CVector &TargetCoors, CVector &SourceOut)
{
	int i;
	int16 NumObjects;
	CEntity *Objects[16];
	CEntity *NearestLampPost = nil;
	CWorld::FindObjectsInRange(TargetCoors, 30.0f, true, &NumObjects, 15, Objects, false, false, false, true, true);
	float NearestDist = 10000.0f;
	for(i = 0; i < NumObjects; i++){
		if(Objects[i]->GetIsStatic() && Objects[i]->GetUp().z > 0.9f && IsLampPost(Objects[i]->GetModelIndex())){
			float Dist = (Objects[i]->GetPosition() - TargetCoors).Magnitude2D();
			if(Abs(ARRESTCAM_LAMP_BEST_DIST - Dist) < NearestDist){
				CVector TestStart = Objects[i]->GetColModel()->boundingBox.max;
				TestStart = Objects[i]->GetMatrix() * TestStart;
				CVector TestEnd = TestStart - TargetCoors;
				TestEnd.Normalise();
				TestEnd += TargetCoors;
				if(CWorld::GetIsLineOfSightClear(TestStart, TestEnd, true, false, false, false, false, true, true)){
					NearestDist = Abs(ARRESTCAM_LAMP_BEST_DIST - Dist);
					NearestLampPost = Objects[i];
					SourceOut = TestStart;
				}
			}
		}
	}
	return NearestLampPost != nil;
}

bool
CCam::GetLookOverShoulderPos(CEntity *Target, CPed *Cop, CVector &TargetCoors, CVector &SourceOut)
{
	if(Target == nil || Cop == nil)
		return false;
	CVector CopCoors = Cop->GetPosition();
	CVector CopToTarget = TargetCoors - CopCoors;
	CVector Side = CrossProduct(CopToTarget, CVector(0.0f, 0.0f, 1.0f));
	Side.Normalise();
	CopCoors += ARRESTDIST_RIGHTOF_COP * Side;
	CopToTarget.Normalise();
	if(CopToTarget.z < -0.7071f){
		CopToTarget.z = -0.7071f;
		float GroundDist = CopToTarget.Magnitude2D();
		if(GroundDist > 0.0f){
			CopToTarget.x *= 0.7071f/GroundDist;
			CopToTarget.y *= 0.7071f/GroundDist;
		}
		CopToTarget.Normalise();
	}else{
		if(CopToTarget.z > 0.0f){
			CopToTarget.z = 0.0f;
			CopToTarget.Normalise();
		}
	}
	CopCoors -= ARRESTDIST_BEHIND_COP * CopToTarget;
	CopToTarget = TargetCoors - CopCoors;
	float Dist = CopToTarget.Magnitude();
	if(Dist < ARRESTDIST_MINFROM_PLAYER && Dist > 0.0f)
		CopToTarget *= ARRESTDIST_MINFROM_PLAYER/Dist;
	SourceOut = TargetCoors - CopToTarget;
	return true;
}

enum {
	ARRESTCAM_OVERSHOULDER = 1,
	ARRESTCAM_ALONGGROUND,
	ARRESTCAM_ALONGGROUND_RIGHT,
	ARRESTCAM_ALONGGROUND_RIGHT_UP,
	ARRESTCAM_ALONGGROUND_LEFT,
	ARRESTCAM_ALONGGROUND_LEFT_UP,
	ARRESTCAM_LAMPPOST,
};

int nUsingWhichCamera;
CPed *pStoredCopPed;

bool
CCam::ProcessArrestCamOne(void)
{
	CVector TargetPos;
	CVector CamSource;
	CPed *cop = nil;
	FOV = 45.0f;
	bool foundPos = false;
	int ArrestModes[5] = { -1, -1, -1, -1, -1 };

	if(ResetStatics){
		CPed *targetPed = (CPed*)TheCamera.pTargetEntity;
		nUsingWhichCamera = 0;
		if(TheCamera.pTargetEntity->IsPed()){
			((CPed*)TheCamera.pTargetEntity)->m_pedIK.GetComponentPosition(TargetPos, PED_MID);
			if(FindPlayerPed() && FindPlayerPed()->m_pArrestingCop)
				cop = FindPlayerPed()->m_pArrestingCop;
			if(cop && CGeneral::GetRandomNumberInRange(0.0f, 1.0f) > 0.5f){
				ArrestModes[0] = ARRESTCAM_OVERSHOULDER;
				ArrestModes[1] = ARRESTCAM_ALONGGROUND;
				ArrestModes[2] = ARRESTCAM_OVERSHOULDER;
				ArrestModes[3] = ARRESTCAM_LAMPPOST;
			}else{
				ArrestModes[0] = ARRESTCAM_ALONGGROUND;
				ArrestModes[1] = ARRESTCAM_OVERSHOULDER;
				ArrestModes[2] = ARRESTCAM_LAMPPOST;
			}
		}else if(TheCamera.pTargetEntity->IsVehicle()){
			CVehicle *targetVehicle = (CVehicle*)TheCamera.pTargetEntity;
			if(targetVehicle->pDriver && targetVehicle->pDriver->IsPlayer()){
				targetPed = targetVehicle->pDriver;
				targetPed->m_pedIK.GetComponentPosition(TargetPos, PED_MID);
			}else{
				targetPed = nil;
				TargetPos = targetVehicle->GetPosition();
			}

			if(FindPlayerPed() && FindPlayerPed()->m_pArrestingCop)
				cop = FindPlayerPed()->m_pArrestingCop;
			if(cop && CGeneral::GetRandomNumberInRange(0.0f, 1.0f) > 0.65f){
				ArrestModes[0] = ARRESTCAM_OVERSHOULDER;
				ArrestModes[1] = ARRESTCAM_LAMPPOST;
				ArrestModes[2] = ARRESTCAM_ALONGGROUND;
				ArrestModes[3] = ARRESTCAM_OVERSHOULDER;
			}else{
				ArrestModes[0] = ARRESTCAM_LAMPPOST;
				ArrestModes[1] = ARRESTCAM_ALONGGROUND;
				ArrestModes[2] = ARRESTCAM_OVERSHOULDER;
			}
		}else
			return false;

		for(int i = 0; nUsingWhichCamera == 0 && i < ARRAY_SIZE(ArrestModes) && ArrestModes[i] > 0; i++){
			switch(ArrestModes[i]){
			case ARRESTCAM_OVERSHOULDER:
				if(cop){
					foundPos = GetLookOverShoulderPos(TheCamera.pTargetEntity, cop, TargetPos, CamSource);
					pStoredCopPed = cop;
					cop = nil;
				}else if(targetPed){
					for(int j = 0; j < targetPed->m_numNearPeds; j++){
						CPed *nearPed = targetPed->m_nearPeds[j];
						if(nearPed->GetPedState() == PED_ARREST_PLAYER)
							foundPos = GetLookOverShoulderPos(TheCamera.pTargetEntity, nearPed, TargetPos, CamSource);
						if(foundPos){
							pStoredCopPed = nearPed;
							break;
						}
					}
				}
				break;
			case ARRESTCAM_ALONGGROUND:
				if(cop){
					foundPos = GetLookAlongGroundPos(TheCamera.pTargetEntity, cop, TargetPos, CamSource);
					pStoredCopPed = cop;
					cop = nil;
				}else if(targetPed){
					for(int j = 0; j < targetPed->m_numNearPeds; j++){
						CPed *nearPed = targetPed->m_nearPeds[j];
						if(nearPed->GetPedState() == PED_ARREST_PLAYER)
							foundPos = GetLookAlongGroundPos(TheCamera.pTargetEntity, nearPed, TargetPos, CamSource);
						if(foundPos){
							pStoredCopPed = nearPed;
							break;
						}
					}
				}
				break;
			case ARRESTCAM_LAMPPOST:
				foundPos = GetLookFromLampPostPos(TheCamera.pTargetEntity, cop, TargetPos, CamSource);
				break;
			}

			if(foundPos){
				if(pStoredCopPed)
					pStoredCopPed->RegisterReference((CEntity**)&pStoredCopPed);
				nUsingWhichCamera = ArrestModes[i];
				if(ArrestModes[i] == ARRESTCAM_ALONGGROUND){
					float rnd = CGeneral::GetRandomNumberInRange(0.0f, 5.0f);
					if(rnd < 1.0f) nUsingWhichCamera = ARRESTCAM_ALONGGROUND;
					else if(rnd < 2.0f) nUsingWhichCamera = ARRESTCAM_ALONGGROUND_RIGHT;
					else if(rnd < 3.0f) nUsingWhichCamera = ARRESTCAM_ALONGGROUND_RIGHT_UP;
					else if(rnd < 4.0f) nUsingWhichCamera = ARRESTCAM_ALONGGROUND_LEFT;
					else nUsingWhichCamera = ARRESTCAM_ALONGGROUND_LEFT_UP;
				}
			}else
				pStoredCopPed = nil;
		}

		Source = CamSource;
		CVector OrigSource = Source;
		TheCamera.AvoidTheGeometry(OrigSource, TargetPos, Source, FOV);
		Front = TargetPos - Source;
		Front.Normalise();
		Up = CVector(0.0f, 0.0f, 1.0f);
		CVector Right = CrossProduct(Front, Up);
		Right.Normalise();
		Up = CrossProduct(Right, Front);
		if(nUsingWhichCamera != 0)
			ResetStatics = false;
		return true;
	}

	if(TheCamera.pTargetEntity->IsPed()){
		((CPed*)TheCamera.pTargetEntity)->m_pedIK.GetComponentPosition(TargetPos, PED_MID);
	}else if(TheCamera.pTargetEntity->IsVehicle()){
		CPed *driver = ((CVehicle*)TheCamera.pTargetEntity)->pDriver;
		if(driver && driver->IsPlayer())
			driver->m_pedIK.GetComponentPosition(TargetPos, PED_MID);
		else
			TargetPos = TheCamera.pTargetEntity->GetPosition();
	}else
		return false;

	if(nUsingWhichCamera == ARRESTCAM_OVERSHOULDER && pStoredCopPed){
		foundPos = GetLookOverShoulderPos(TheCamera.pTargetEntity, pStoredCopPed, TargetPos, CamSource);
		float newZ = Source.z + ARRESTCAM_S_ROTATION_UP*CTimer::GetTimeStep();
		if(CamSource.z > newZ)
			CamSource.z = newZ;
	}else if(nUsingWhichCamera >= ARRESTCAM_ALONGGROUND_RIGHT && nUsingWhichCamera <= ARRESTCAM_ALONGGROUND_LEFT_UP){
		CamSource = Source;
		Front = TargetPos - CamSource;
		Front.Normalise();
		Up = CVector(0.0f, 0.0f, 1.0f);
		CVector Right = CrossProduct(Front, Up);
		if(nUsingWhichCamera == ARRESTCAM_ALONGGROUND_LEFT || nUsingWhichCamera == ARRESTCAM_ALONGGROUND_LEFT_UP)
			Right *= -1.0f;
		if(CWorld::TestSphereAgainstWorld(CamSource + 0.5f*Right, 0.4f, TheCamera.pTargetEntity, true, true, false, true, false, true) == nil){
			foundPos = true;
			CamSource += Right*ARRESTCAM_ROTATION_SPEED*CTimer::GetTimeStep();
			if(nUsingWhichCamera == ARRESTCAM_ALONGGROUND_RIGHT_UP || nUsingWhichCamera == ARRESTCAM_ALONGGROUND_LEFT_UP){
				CamSource.z += ARRESTCAM_ROTATION_UP*CTimer::GetTimeStep();
			}else{
				bool found = false;
				float ground = CWorld::FindGroundZFor3DCoord(CamSource.x, CamSource.y, CamSource.z, &found);
				if(found)
					CamSource.z = ground + ARRESTDIST_ABOVE_GROUND;
			}
		}
	}else if(nUsingWhichCamera == ARRESTCAM_LAMPPOST){
		CamSource = Source;
		Front = TargetPos - CamSource;
		Front.z = 0.0f;
		Front.Normalise();
		Up = CVector(0.0f, 0.0f, 1.0f);
		CVector Right = CrossProduct(Front, Up);
		Right.Normalise();
		Front = TargetPos - CamSource + Right*ARRESTCAM_LAMPPOST_ROTATEDIST;
		Front.z = 0.0f;
		Front.Normalise();
		if(CWorld::TestSphereAgainstWorld(CamSource + 0.5f*Front, 0.4f, TheCamera.pTargetEntity, true, true, false, true, false, true) == nil){
			foundPos = true;
			CamSource += Front*ARRESTCAM_LAMPPOST_TRANSLATE*CTimer::GetTimeStep();
		}
	}

	if(foundPos){
		Source = CamSource;
		CVector OrigSource = Source;
		TheCamera.AvoidTheGeometry(OrigSource, TargetPos, Source, FOV);
		Front = TargetPos - Source;
		Front.Normalise();
		Up = CVector(0.0f, 0.0f, 1.0f);
		CVector Right = CrossProduct(Front, Up);
		Right.Normalise();
		Up = CrossProduct(Right, Front);
	}else{
		CVector OrigSource = Source;
		TheCamera.AvoidTheGeometry(OrigSource, TargetPos, Source, FOV);
	}

	return true;
}

bool
CCam::ProcessArrestCamTwo(void)
{
	CPed *player = CWorld::Players[CWorld::PlayerInFocus].m_pPed;
	if(!ResetStatics)
		return true;
	ResetStatics = false;

	CVector TargetCoors, ToCamera;
	float BetaOffset;
	float SourceX, SourceY;
	if(&TheCamera.Cams[TheCamera.ActiveCam] == this){
		SourceX = TheCamera.Cams[(TheCamera.ActiveCam + 1) % 2].Source.x;
		SourceY = TheCamera.Cams[(TheCamera.ActiveCam + 1) % 2].Source.y;
	}else{
		SourceX = TheCamera.Cams[TheCamera.ActiveCam].Source.x;
		SourceY = TheCamera.Cams[TheCamera.ActiveCam].Source.y;
	}

	for(int i = 0; i <= 1; i++){
		int Dir = i == 0 ? 1 : -1;

		FOV = 60.0f;
		TargetCoors = player->GetPosition();
		Beta = CGeneral::GetATanOfXY(TargetCoors.x-SourceX, TargetCoors.y-SourceY);
		BetaOffset = DEGTORAD(Dir*80);
		Source = TargetCoors + 11.5f*CVector(Cos(Beta+BetaOffset), Sin(Beta+BetaOffset), 0.0f);

		ToCamera = Source - TargetCoors;
		ToCamera.Normalise();
		TargetCoors.x += 0.4f*ToCamera.x;
		TargetCoors.y += 0.4f*ToCamera.y;
		if(CWorld::GetIsLineOfSightClear(Source, TargetCoors, true, true, false, true, false, true, true)){
			Source.z += 5.5f;
			TargetCoors += CVector(-0.8f*ToCamera.x, -0.8f*ToCamera.y, 2.2f);
			m_cvecTargetCoorsForFudgeInter = TargetCoors;
			Front = TargetCoors - Source;
			ResetStatics = false;
			GetVectorsReadyForRW();
			return true;
		}
	}
	return false;
}


#ifdef FREE_CAM
void
CCam::Process_FollowPed_Rotation(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	FOV = DefaultFOV;

	const float MinDist = 2.0f;
	const float MaxDist = 2.0f + TheCamera.m_fPedZoomValueSmooth;
	const float BaseOffset = 0.4f;	// B4: heightOffset ClassicAXIS del follow (B8); era 0.75 de serie

	CVector TargetCoors = CameraTarget;
#ifdef VICEEXT_RECOIL
	bool recoilReset = ResetStatics;
#endif

	TargetCoors.z += m_fSyphonModeTargetZOffSet;
	TargetCoors = DoAverageOnVector(TargetCoors);
	TargetCoors.z += BaseOffset;	// add offset so alpha evens out to 0
#ifdef VICEEXT_CROUCH
	// R6/H1: agachado, la cámara baja con el ped. R14: offset a −0,95 (el bloque H
	// midió que con −0,55 el descenso real se quedaba en 0,27 m; ver el otro
	// proceso, donde está la explicación completa).
	if (CamTargetEntity && CamTargetEntity->IsPed() && ((CPed*)CamTargetEntity)->IsPlayer())
	{
		float odCrouchBlend = CPlayerPed::ViceExtCrouchBlend();
		if (odCrouchBlend > 0.0f)
			TargetCoors.z -= VICEEXT_CROUCH_CAM_DROP * odCrouchBlend;
	}
#endif
#ifdef VICEEXT_SWIMMING
	// H2: nadando, el objetivo sube a la superficie (ver el otro proceso).
	if (CamTargetEntity && CamTargetEntity->IsPed() && ((CPed*)CamTargetEntity)->IsPlayer()
	    && CPlayerPed::ViceExtIsSwimming()) {
		float odWl = 0.0f;
		if (CWaterLevel::GetWaterLevel(CamTargetEntity->GetPosition(), &odWl, true)
	    	&& TargetCoors.z < odWl + 0.5f)
			TargetCoors.z = odWl + 0.5f;
#ifdef __EMSCRIPTEN__
		// R14: una línea por segundo mientras se nada. Junto con `SWIM2 camz`
		// cierra el diagnóstico de la cámara de nado: dice que el objetivo está en
		// la superficie (nivel+0,5) y dónde está la cámara.
		{
			static uint32 s_odNextSwim = 0;
			uint32 odNow = CTimer::GetTimeInMilliseconds();
			if (odNow < s_odNextSwim && odNow + 60000 >= s_odNextSwim) {}
			else {
				s_odNextSwim = odNow + 1000;
				char t[160];
				snprintf(t, sizeof t, "SWIMCAM objetivo=%.2f nivel=%.2f cam=%.2f modo=%d",
					TargetCoors.z, odWl, Source.z, (int)Mode);
				ODTRACES(t);
			}
		}
#endif
	}
#endif
//	B4: hombro al apuntar (igual que en FollowPedWithMouse; spec ClassicAXIS).
	TargetCoors += CamTargetEntity->GetRight() * ViceExtAimShoulderSmoothed(CamTargetEntity, ResetStatics);
//	TargetCoors.z += m_fRoadOffSet;

	CVector Dist = Source - TargetCoors;
	CVector ToCam;

	bool Shooting = false;
	if(((CPed*)CamTargetEntity)->GetWeapon()->m_eWeaponType != WEAPONTYPE_UNARMED)
		if(CPad::GetPad(0)->GetWeapon())
			Shooting = true;
	if(((CPed*)CamTargetEntity)->GetWeapon()->m_eWeaponType == WEAPONTYPE_DETONATOR ||
	   ((CPed*)CamTargetEntity)->GetWeapon()->m_eWeaponType == WEAPONTYPE_BASEBALLBAT)
		Shooting = false;


	if(ResetStatics){
		// Coming out of top down here probably
		// so keep Beta, reset alpha and calculate vectors
		Beta = CGeneral::GetATanOfXY(Dist.x, Dist.y);
		Alpha = 0.0f;

		Dist = MaxDist*CVector(Cos(Alpha) * Cos(Beta), Cos(Alpha) * Sin(Beta), Sin(Alpha));
		Source = TargetCoors + Dist;

		ResetStatics = false;	}
	// Drag the camera along at the look-down offset
	float CamDist = Dist.Magnitude();
	if(CamDist == 0.0f)
		Dist = CVector(1.0f, 1.0f, 0.0f);
	else if(CamDist < MinDist)
		Dist *= MinDist/CamDist;
	else if(CamDist > MaxDist)
		Dist *= MaxDist/CamDist;
	CamDist = Dist.Magnitude();

	// Beta = 0 is looking east, HALFPI is north, &c.
	// Alpha positive is looking up
	float GroundDist = Dist.Magnitude2D();
	Beta = CGeneral::GetATanOfXY(-Dist.x, -Dist.y);
	Alpha = CGeneral::GetATanOfXY(GroundDist, -Dist.z);
	while(Beta >= PI) Beta -= 2.0f*PI;
	while(Beta < -PI) Beta += 2.0f*PI;
	while(Alpha >= PI) Alpha -= 2.0f*PI;
	while(Alpha < -PI) Alpha += 2.0f*PI;

	// Look around
	// B4: el ratón estaba desactivado aquí (bloque comentado de serie); se habilita
	// con la misma selección que Process_FollowPedWithMouse y las fórmulas ClassicAXIS
	// (B8: ratón (-2.5x, 4y)*MouseAccel*FOV/80, stick con deadzone por eje vía
	// LookAround*): el ratón manda si se mueve, si no el palo. Las ramas
	// BetaOffset/AlphaOffset de abajo ya contemplan ambos casos.
	bool UseMouse = false;
	float MouseX = CPad::GetPad(0)->GetMouseX();
	float MouseY = CPad::GetPad(0)->GetMouseY();
	float LookLeftRight, LookUpDown;
#ifdef VICEEXT_RECOIL
	float recoilManualAlphaStart = Alpha;
#endif
	if((MouseX != 0.0f || MouseY != 0.0f) && !CPad::GetPad(0)->ArePlayerControlsDisabled()){
		UseMouse = true;
		LookLeftRight = -2.5f*MouseX;
		LookUpDown = 4.0f*MouseY;
	}else{
		LookLeftRight = -CPad::GetPad(0)->LookAroundLeftRight();
		LookUpDown = CPad::GetPad(0)->LookAroundUpDown();
	}	float AlphaOffset, BetaOffset;
	if(UseMouse){
		BetaOffset = LookLeftRight * TheCamera.m_fMouseAccelHorzntl * FOV/80.0f;
		AlphaOffset = LookUpDown * TheCamera.m_fMouseAccelVertical * FOV/80.0f;
	}else{
		BetaOffset = LookLeftRight * fStickSens * (1.0f/20.0f) * FOV/80.0f * CTimer::GetTimeStep();
		AlphaOffset = LookUpDown * fStickSens * (0.6f/20.0f) * FOV/80.0f * CTimer::GetTimeStep();
	}

	// Stop centering once stick has been touched
	if(BetaOffset)
		Rotating = false;

	Beta += BetaOffset;
	Alpha += AlphaOffset;
	while(Beta >= PI) Beta -= 2.0f*PI;
	while(Beta < -PI) Beta += 2.0f*PI;
#ifdef VICEEXT_RECOIL
	float recoilManualDeltaRad = AlphaOffset;
	CWeapon::ViceExtRecoilBegin(Alpha, recoilReset, Mode, "runabout-follow");
#endif
	// B4: clamp vertical ClassicAXIS del follow (B8: +60/-89.5; era +45 de serie).
	// BARRIDO 1: apuntando (hombro activo), +-50 de CamNew Process_AimWeapon.
	if(ViceExtAimingOverShoulder(CamTargetEntity)){
		if(Alpha > DEGTORAD(50.0f)) Alpha = DEGTORAD(50.0f);
		else if(Alpha < -DEGTORAD(50.0f)) Alpha = -DEGTORAD(50.0f);
	}else if(Alpha > DEGTORAD(60.0f)) Alpha = DEGTORAD(60.0f);
	else if(Alpha < -DEGTORAD(89.5f)) Alpha = -DEGTORAD(89.5f);


	float BetaDiff = TargetOrientation+PI - Beta;
	while(BetaDiff >= PI) BetaDiff -= 2.0f*PI;
	while(BetaDiff < -PI) BetaDiff += 2.0f*PI;
	float TargetAlpha = Alpha;
	// 12deg to account for our little height offset. we're not working on the true alpha here
	const float AlphaLimitUp = DEGTORAD(15.0f) + DEGTORAD(12.0f);
	const float AlphaLimitDown = -DEGTORAD(15.0f) + DEGTORAD(12.0f);
	if(Abs(BetaDiff) < DEGTORAD(25.0f) && ((CPed*)CamTargetEntity)->GetMoveSpeed().Magnitude2D() > 0.01f){
		// Limit alpha when player is walking towards camera
		if(TargetAlpha > AlphaLimitUp) TargetAlpha = AlphaLimitUp;
		if(TargetAlpha < AlphaLimitDown) TargetAlpha = AlphaLimitDown;
	}

	WellBufferMe(TargetAlpha, &Alpha, &AlphaSpeed, 0.2f, 0.1f, true);
#ifdef VICEEXT_RECOIL
	CWeapon::ViceExtRecoilApply(Alpha, recoilManualDeltaRad, LookUpDown, UseMouse ? "mouse" : "pad", Mode,
		ViceExtAimingOverShoulder(CamTargetEntity) ? -DEGTORAD(50.0f) : -DEGTORAD(89.5f),
		ViceExtAimingOverShoulder(CamTargetEntity) ? DEGTORAD(50.0f) : DEGTORAD(60.0f));
#endif

	if(CPad::GetPad(0)->ForceCameraBehindPlayer() || Shooting){
		m_fTargetBeta = TargetOrientation;
		Rotating = true;
	}

	if(Rotating){
		WellBufferMe(m_fTargetBeta, &Beta, &BetaSpeed, 0.1f, 0.06f, true);
		float DeltaBeta = m_fTargetBeta - Beta;
		while(DeltaBeta >= PI) DeltaBeta -= 2*PI;
		while(DeltaBeta < -PI) DeltaBeta += 2*PI;
		if(Abs(DeltaBeta) < 0.06f)
			Rotating = false;
	}

	if(TheCamera.m_bUseTransitionBeta)
		Beta = CGeneral::GetATanOfXY(-Cos(m_fTransitionBeta), -Sin(m_fTransitionBeta));

	if(TheCamera.m_bUseTransitionBeta)
		Beta = CGeneral::GetATanOfXY(-Cos(m_fTransitionBeta), -Sin(m_fTransitionBeta));

	Front = CVector(Cos(Alpha) * Cos(Beta), Cos(Alpha) * Sin(Beta), Sin(Alpha));
	Source = TargetCoors - Front*CamDist;
	TargetCoors.z -= BaseOffset;	// now get back to the real target coors again

	m_cvecTargetCoorsForFudgeInter = TargetCoors;


	Front = TargetCoors - Source;
	Front.Normalise();



	/*
	 * Handle collisions - taken from FollowPedWithMouse
	 */

	CEntity *entity;
	CColPoint colPoint;
	// Clip Source and fix near clip
	CWorld::pIgnoreEntity = CamTargetEntity;
	entity = nil;
	if(CWorld::ProcessLineOfSight(TargetCoors, Source, colPoint, entity, true, true, true, true, false, false, true)){
		float PedColDist = (TargetCoors - colPoint.point).Magnitude();
		float ColCamDist = CamDist - PedColDist;
		if(entity->IsPed() && ColCamDist > DEFAULT_NEAR + 0.1f){
			// Ped in the way but not clipping through
			if(CWorld::ProcessLineOfSight(colPoint.point, Source, colPoint, entity, true, true, true, true, false, false, true)){
				PedColDist = (TargetCoors - colPoint.point).Magnitude();
				Source = colPoint.point;
				if(PedColDist < DEFAULT_NEAR + 0.3f)
					RwCameraSetNearClipPlane(Scene.camera, Max(PedColDist-0.3f, 0.05f));
			}else{
				RwCameraSetNearClipPlane(Scene.camera, Min(ColCamDist-0.35f, DEFAULT_NEAR));
			}
		}else{
			Source = colPoint.point;
			if(PedColDist < DEFAULT_NEAR + 0.3f)
				RwCameraSetNearClipPlane(Scene.camera, Max(PedColDist-0.3f, 0.05f));
		}
	}
	CWorld::pIgnoreEntity = nil;

	float ViewPlaneHeight = Tan(DEGTORAD(FOV) / 2.0f);
	float ViewPlaneWidth = ViewPlaneHeight * CDraw::CalculateAspectRatio() * fTweakFOV;
	float Near = RwCameraGetNearClipPlane(Scene.camera);
	float radius = ViewPlaneWidth*Near;
	entity = CWorld::TestSphereAgainstWorld(Source + Front*Near, radius, nil, true, true, false, true, false, false);
	int i = 0;
	while(entity){
		CVector CamToCol = gaTempSphereColPoints[0].point - Source;
		float frontDist = DotProduct(CamToCol, Front);
		float dist = (CamToCol - Front*frontDist).Magnitude() / ViewPlaneWidth;

		// Try to decrease near clip
		dist = Max(Min(Near, dist), 0.1f);
		if(dist < Near)
			RwCameraSetNearClipPlane(Scene.camera, dist);

		// Move forward a bit
		if(dist == 0.1f)
			Source += (TargetCoors - Source)*0.3f;

		// Keep testing
		Near = RwCameraGetNearClipPlane(Scene.camera);
		radius = ViewPlaneWidth*Near;
		entity = CWorld::TestSphereAgainstWorld(Source + Front*Near, radius, nil, true, true, false, true, false, false);

		i++;
		if(i > 5)
			entity = nil;
	}

	GetVectorsReadyForRW();
}

// LCS cam hehe
void
CCam::Process_FollowCar_SA(const CVector& CameraTarget, float TargetOrientation, float, float)
{

	// BARRIDO 1 — delegacion en la ley unica de coche (ver banner sobre
	// `TiltTopSpeed`). Barrido aqui todo lo SA/LCS: tablas CARCAM_SET, historial
	// m_aTargetHistoryPos*, yaw por velocidad (betaChangeMult1/2), stick 0.007,
	// raton con inercia stepsLeftToChangeBetaByMouse, alpha-blend por tipo,
	// colisiones LCS (dontCollideWithCars + IS_TRAFFIC_LIGHT), suelo agua/RC y
	// el autocentrado propio + traza `camauto` (el `camauto2` bueno lo emite la
	// ley unica). Este modo solo corria con bFreeCam; el jugado es el string.
	if(!CamTargetEntity->IsVehicle())
		return;
	CVehicle *car = (CVehicle*)CamTargetEntity;
	Process_Cam_On_A_String(CameraTarget, TargetOrientation, 0.0f, 0.0f);
	// RETENIDO (no es seguimiento: la torreta del Rhino/Firetruck sigue a la
	// camara; serie ~6034-6097, movido verbatim tras la ley nueva).
	// SA code from CAutomobile::TankControl/FireTruckControl.
	if (car->GetModelIndex() == MI_RHINO || car->GetModelIndex() == MI_FIRETRUCK) {

		float &carGunLR = ((CAutomobile*)car)->m_fCarGunLR;
		CVector hi = Multiply3x3(Front, car->GetMatrix());

		// III/VC's firetruck turret angle is reversed
		float angleToFace = (car->GetModelIndex() == MI_FIRETRUCK ? -hi.Heading() : hi.Heading());

		if (angleToFace <= carGunLR + PI) {
			if (angleToFace < carGunLR - PI)
				angleToFace = angleToFace + TWOPI;
		} else {
			angleToFace = angleToFace - TWOPI;
		}

		float neededTurn = angleToFace - carGunLR;
		float turnPerFrame = CTimer::GetTimeStep() * (car->GetModelIndex() == MI_FIRETRUCK ? 0.05f : 0.015f);
		if (neededTurn <= turnPerFrame) {
			if (neededTurn < -turnPerFrame)
				angleToFace = carGunLR - turnPerFrame;
		} else {
			angleToFace = turnPerFrame + carGunLR;
		}

		if (car->GetModelIndex() == MI_RHINO && carGunLR != angleToFace) {
			DMAudio.PlayOneShot(car->m_audioEntityId, SOUND_CAR_TANK_TURRET_ROTATE, Abs(angleToFace - carGunLR));
		}
		carGunLR = angleToFace;

		if (carGunLR < -PI) {
			carGunLR += TWOPI;
		} else if (carGunLR > PI) {
			carGunLR -= TWOPI;
		}

		// Because firetruk turret also has Y movement
		if (car->GetModelIndex() == MI_FIRETRUCK) {
			float &carGunUD = ((CAutomobile*)car)->m_fCarGunUD;

			float alphaToFace = Atan2(hi.z, hi.Magnitude2D()) + DEGTORAD(15.0f);
			float neededAlphaTurn = alphaToFace - carGunUD;
			float alphaTurnPerFrame = CTimer::GetTimeStepInSeconds();

			if (neededAlphaTurn > alphaTurnPerFrame) {
				neededTurn = alphaTurnPerFrame;
				carGunUD = neededTurn + carGunUD;
			} else {
				if (neededAlphaTurn >= -alphaTurnPerFrame) {
					carGunUD = alphaToFace;
				} else {
					carGunUD = carGunUD - alphaTurnPerFrame;
				}
			}

			float turretMinY = -DEGTORAD(20.0f);
			float turretMaxY = DEGTORAD(20.0f);
			if (turretMinY <= carGunUD) {
				if (carGunUD > turretMaxY)
					carGunUD = turretMaxY;
			} else {
				carGunUD = turretMinY;
			}
		}
	}
}
#endif
