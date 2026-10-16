#include "common.h"

#include "RwHelper.h"
#include "PlayerPed.h"
#include "PedArbiter.h"
#include "Wanted.h"
#include "Fire.h"
#include "DMAudio.h"
#include "soundlist.h"
#include "Timecycle.h"
#include "Pad.h"
#include "Camera.h"
#include "WeaponEffects.h"
#include "ModelIndices.h"
#include "World.h"
#include "RpAnimBlend.h"
#include "AnimBlendAssociation.h"
#include "General.h"
#include "Pools.h"
#include "Darkel.h"
#include "CarCtrl.h"
#include "MBlur.h"
#include "Streaming.h"
#include "Population.h"
#include "Script.h"
#include "Replay.h"
#include "PedPlacement.h"
#include "VarConsole.h"
#include "SaveBuf.h"
#include "ControllerConfig.h"
#ifdef VICEEXT_IMPROVED_MENU_AND_INPUT
#include "Frontend.h"
#endif
#ifdef VICEEXT_IMPROVED_TECH_PART
#include "AudioManager.h"
#endif
#include "ondemand.h"          // web: ODTRACES -> odtrace.log (sección 3, C3.1)
#include "WaterLevel.h"        // Sección 3, C4 (nadar): CWaterLevel::GetWaterLevel
#include "WeaponInfo.h"
#include "ParticleObject.h"
#include "Draw.h"
#include "Particle.h" // C4 nado: salpicadura de un solo uso por brazada        // Sección 3, C5/C6: CWeaponInfo (peso del arma)
#include "ViceExtFeatures.h" // familia 11: el `features.ini` del mod manda

#define PAD_MOVE_TO_GAME_WORLD_MOVE 60.0f

#ifdef VICEEXT_SWIMMING
static int32 s_odSwimSlot = 0;
#endif

bool CPlayerPed::bDontAllowWeaponChange;
#ifndef MASTER
bool CPlayerPed::bDebugPlayerInfo;
#endif

const uint32 CPlayerPed::nSaveStructSize =
#ifdef COMPATIBLE_SAVES
	1752;
#else
	sizeof(CPlayerPed);
#endif

int32 idleAnimBlockIndex;

CPad*
GetPadFromPlayer(CPlayerPed*)
{
	return CPad::GetPad(0);
}

CPlayerPed::~CPlayerPed()
{
	delete m_pWanted;
}

CPlayerPed::CPlayerPed(void) : CPed(PEDTYPE_PLAYER1)
{
	m_fMoveSpeed = 0.0f;
	SetModelIndex(MI_PLAYER);
#ifdef FIX_BUGS
	m_fCurrentStamina = m_fMaxStamina = 150.0f;
#endif
#ifdef VICEEXT_SWIMMING
	// Vice Extended (features.ini: EnableSwimming=1): "nadar sin riesgo de
	// morir". El daño por ahogamiento (CPed::InflictDamage, WEAPONTYPE_DROWNING)
	// se ignora cuando bDrownsInWater es falso, así que el jugador puede bucear
	// sin morir; los NPC siguen ahogándose como en el juego original.
	bDrownsInWater = false;
#endif
	SetInitialState();

	m_pWanted = new CWanted();
	m_pWanted->Initialise();
	m_pArrestingCop = nil;
	m_currentWeapon = WEAPONTYPE_UNARMED;
	m_nSelectedWepSlot = WEAPONSLOT_UNARMED;
	m_nSpeedTimer = 0;
	m_bSpeedTimerFlag = false;
	SetWeaponLockOnTarget(nil);
	SetPedState(PED_IDLE);
#ifndef FIX_BUGS
	m_fCurrentStamina = m_fMaxStamina = 150.0f;
#endif
	m_fStaminaProgress = 0.0f;
	m_nEvadeAmount = 0;
	m_pEvadingFrom = nil;
	m_nHitAnimDelayTimer = 0;
	m_fAttackButtonCounter = 0.0f;
	m_bHaveTargetSelected = false;
	m_bHasLockOnTarget = false;
	m_bCanBeDamaged = true;
	m_bNoPosForMeleeAttack = false;
	m_fWalkAngle = 0.0f;
	m_fFPSMoveHeading = 0.0f;
	m_pMinigunTopAtomic = nil;
	m_fGunSpinSpeed = 0.0;
	m_fGunSpinAngle = 0.0;
	m_nPadDownPressedInMilliseconds = 0;
	m_nTargettableObjects[0] = m_nTargettableObjects[1] = m_nTargettableObjects[2] = m_nTargettableObjects[3] = -1;
	unk1 = false;
	for (int i = 0; i < 6; i++) {
		m_vecSafePos[i] = CVector(0.0f, 0.0f, 0.0f);
		m_pPedAtSafePos[i] = nil;
		m_pMeleeList[i] = nil;
	}
	m_nAttackDirToCheck = 0;
	m_nLastBusFareCollected = 0;
	idleAnimBlockIndex = CAnimManager::GetAnimationBlockIndex("playidles");
#ifdef FREE_CAM
	m_bFreeAimActive = false;
#endif

#ifdef VICEEXT_FIRING_AND_AIMING
	bIsPlayerAiming = false;
#endif

#ifdef VICEEXT_IMPROVED_MENU_AND_INPUT
	bIsAutoAiming = false;
#endif

#ifdef VICEEXT_IMPROVED_TECH_PART
	m_bSkipPhoneCall = false;
#endif
}

void
CPlayerPed::ClearWeaponTarget()
{
#ifdef VICEEXT_IMPROVED_MENU_AND_INPUT
	bIsAutoAiming = false;
#endif
	if (m_nPedType == PEDTYPE_PLAYER1) {
		SetWeaponLockOnTarget(nil);
		TheCamera.ClearPlayerWeaponMode();
		CWeaponEffects::ClearCrossHair();
	}
 	ClearPointGunAt();
}

void
CPlayerPed::SetWantedLevel(int32 level)
{
	m_pWanted->SetWantedLevel(level);
}

void
CPlayerPed::SetWantedLevelNoDrop(int32 level)
{
	m_pWanted->SetWantedLevelNoDrop(level);
}

void
CPlayerPed::MakeObjectTargettable(int32 handle)
{
	for (int i = 0; i < ARRAY_SIZE(m_nTargettableObjects); i++) {
		if (CPools::GetObjectPool()->GetAt(m_nTargettableObjects[i]) == nil) {
			m_nTargettableObjects[i] = handle;
			return;
		}
	}
}

// I don't know the actual purpose of parameter
void
CPlayerPed::AnnoyPlayerPed(bool annoyedByPassingEntity)
{
	if (m_pedStats->m_temper < 52) {
		m_pedStats->m_temper++;
	} else if (annoyedByPassingEntity && m_pedStats->m_temper < 55) {
		m_pedStats->m_temper++;
	} else if (annoyedByPassingEntity) {
		m_pedStats->m_temper = 46;
	}
}

void
CPlayerPed::ClearAdrenaline(void)
{
	if (m_bAdrenalineActive && m_nAdrenalineTime != 0) {
		m_nAdrenalineTime = 0;
		CTimer::SetTimeScale(1.0f);
	}
}

CPlayerInfo *
CPlayerPed::GetPlayerInfoForThisPlayerPed()
{
	if (CWorld::Players[0].m_pPed == this)
		return &CWorld::Players[0];

	return nil;
}

void
CPlayerPed::SetupPlayerPed(int32 index)
{
	CPlayerPed *player = new CPlayerPed();
	CWorld::Players[index].m_pPed = player;
#ifdef FIX_BUGS
	player->RegisterReference((CEntity**)&CWorld::Players[index].m_pPed);
#endif

	player->SetOrientation(0.0f, 0.0f, 0.0f);

	CWorld::Add(player);
	player->m_wepAccuracy = 100;

#ifndef MASTER
	VarConsole.Add("Debug PlayerPed", &CPlayerPed::bDebugPlayerInfo, true);
	VarConsole.Add("Tweak Vehicle Handling", &CVehicle::m_bDisplayHandlingInfo, true);
#endif
}

void
CPlayerPed::DeactivatePlayerPed(int32 index)
{
	CWorld::Remove(CWorld::Players[index].m_pPed);
}

void
CPlayerPed::ReactivatePlayerPed(int32 index)
{
	CWorld::Add(CWorld::Players[index].m_pPed);
}

#ifdef VICEEXT_CROUCH
void CPlayerPed::ProcessCrouch(void)
{
	if (!bIsDucking)
		return;

	CAnimBlendAssociation* curCrouchIdleAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_CROUCH_IDLE);
	CAnimBlendAssociation* curCrouchForwardAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_CROUCH_FORWARD);
	CAnimBlendAssociation* curCrouchBackwardAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_CROUCH_BACKWARD);
	CAnimBlendAssociation* curCrouchRollLeftAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_CROUCH_ROLL_L);
	CAnimBlendAssociation* curCrouchRollRightAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_CROUCH_ROLL_R);
	bool bIsCurRollAnim = curCrouchRollLeftAssoc || curCrouchRollRightAssoc;
	bool bIsCurMoveAnim = curCrouchForwardAssoc || curCrouchBackwardAssoc;

	CAnimBlendAssociation* curCrouchFireAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_WEAPON_CROUCHFIRE);

	CPad* pad = GetPadFromPlayer(this);
	int16 padUpDown = pad->GetPedWalkUpDown();
	int16 padLeftRight = pad->GetPedWalkLeftRight();

	if (curCrouchForwardAssoc)
		curCrouchForwardAssoc->speed = Clamp(m_fMoveSpeed - 0.5f, 0.5f, 1.0f);

	if (curCrouchBackwardAssoc)
		curCrouchBackwardAssoc->speed = Clamp(m_fMoveSpeed - 0.5f, 0.5f, 1.0f);

	if (m_nPedState == PED_ROLL)
	{
		if (!bIsCurRollAnim)
			RestorePreviousState();

		return;
	}

	if (bIsPlayerAiming && GetWeapon()->GetInfo()->m_nWeaponSlot == WEAPONSLOT_SNIPER)
		return;

	if (bIsPlayerAiming || TheCamera.Cams[TheCamera.ActiveCam].Mode == CCam::MODE_REAL_1ST_PERSON) {
		TheCamera.Cams[TheCamera.ActiveCam].m_fTargetCameraPosZ = 0.4f;

		if ((m_fMoveSpeed < 0.1f || (padLeftRight != 0 && padUpDown == 0)) && !curCrouchIdleAssoc && !bIsCurRollAnim || m_nPedState == PED_ROLL)
			curCrouchIdleAssoc = CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_CROUCH_IDLE, 4.0f);

		if (pad->GetTarget() && m_fMoveSpeed > 0.1f && (!curCrouchIdleAssoc || curCrouchIdleAssoc && curCrouchIdleAssoc->blendAmount >= 1.0f) && !bIsCurMoveAnim && !bIsCurRollAnim &&
			(padLeftRight < -20 || padLeftRight > 20)) {

			if (curCrouchForwardAssoc)
				delete curCrouchForwardAssoc;

			if (curCrouchBackwardAssoc)
				delete curCrouchBackwardAssoc;

			if (curCrouchFireAssoc) {
				delete curCrouchFireAssoc;
				ClearWeaponTarget();
			}

			float angleToFace;
			angleToFace = CGeneral::GetRadianAngleBetweenPoints(
				GetPosition().x - TheCamera.GetRight().x, GetPosition().y - TheCamera.GetRight().y,
				GetPosition().x, GetPosition().y);
			angleToFace = CGeneral::LimitRadianAngle(angleToFace);
			m_fRotationCur = CGeneral::LimitRadianAngle(m_fRotationCur);
			m_fRotationCur = angleToFace;

			bDontAllowWeaponChange = true;

			SetPedState(PED_ROLL);

			delete curCrouchIdleAssoc;

			if (padLeftRight < 0) {
				curCrouchRollLeftAssoc = CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_CROUCH_ROLL_L, 4.0f);
				curCrouchRollLeftAssoc->SetFinishCallback(FinishRollCB, this);
			} else if (padLeftRight > 0) {
				curCrouchRollRightAssoc = CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_CROUCH_ROLL_R, 4.0f);
				curCrouchRollRightAssoc->SetFinishCallback(FinishRollCB, this);
			}

			return;
		}

		if (m_fMoveSpeed > 0.1 && !bIsCurRollAnim) {
			if (padUpDown != 0) {
				if (padUpDown < 0) {
					if (!curCrouchForwardAssoc)
						curCrouchForwardAssoc = CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_CROUCH_FORWARD, 4.0f);
				} else if (padUpDown > 0) {
					if (!curCrouchBackwardAssoc)
						curCrouchBackwardAssoc = CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_CROUCH_BACKWARD, 4.0f);
				}
			}
		}
	} else if (pad) {
		if (m_fMoveSpeed < 0.1f) {
			TheCamera.Cams[TheCamera.ActiveCam].m_fTargetCameraPosZ = 0.6f;

			if (!curCrouchIdleAssoc)
				curCrouchIdleAssoc = CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_CROUCH_IDLE, 4.0f);
		}

		if (m_fMoveSpeed > 0.1f) {
			TheCamera.Cams[TheCamera.ActiveCam].m_fTargetCameraPosZ = 0.3f;

			if (curCrouchIdleAssoc && !curCrouchForwardAssoc || curCrouchBackwardAssoc)
				curCrouchForwardAssoc = CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_CROUCH_FORWARD, 4.0f);
		}
	}
}

void CPlayerPed::FinishRollCB(CAnimBlendAssociation* assoc, void* arg)
{
	assoc->blendDelta = -1000.0f;

	bDontAllowWeaponChange = false;

	CPlayerPed* ped = (CPlayerPed*)arg;

	CAnimManager::BlendAnimation(ped->GetClump(), ASSOCGRP_STD, ANIM_STD_CROUCH_IDLE, 1000.0f);
}
#endif

#if defined VICEEXT_FIRING_AND_AIMING && defined VICEEXT_FIRST_PERSON
void CPlayerPed::ProcessAiming(void)
{
	if (RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_STARTWALK))
		delete RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_STARTWALK);

	CAnimBlendAssociation* curGunMoveBackwardAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_GUNMOVE_BWD);
	CAnimBlendAssociation* curGunMoveForwardAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_GUNMOVE_FWD);
	CAnimBlendAssociation* curGunMoveLeftAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_GUNMOVE_L);
	CAnimBlendAssociation* curGunMoveRightAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_GUNMOVE_R);
	CAnimBlendAssociation* curGunStandAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_GUN_STAND);

	CPad* pad = GetPadFromPlayer(this);
	int16 padUpDown = pad->GetPedWalkUpDown();
	int16 padLeftRight = pad->GetPedWalkLeftRight();

	if (padLeftRight == 0 && padUpDown == 0 && !curGunStandAssoc)
		curGunStandAssoc = CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_GUN_STAND, 4.0f);

	float animSpeed;

	if (pad->GetSprint() && (padLeftRight != 0 || padUpDown != 0))
		animSpeed = 1.4f;
	else if (IsDoomMode())
		animSpeed = Clamp((m_fMoveSpeed * 3.5f) - 0.1f, 0.1f, 3.5f);
	else
		animSpeed = Clamp(m_fMoveSpeed - 0.1f, 0.1f, 1.0f);

	if (curGunMoveBackwardAssoc)
		curGunMoveBackwardAssoc->speed = animSpeed;

	if (curGunMoveForwardAssoc)
		curGunMoveForwardAssoc->speed = animSpeed;

	if (curGunMoveLeftAssoc)
		curGunMoveLeftAssoc->speed = animSpeed;

	if (curGunMoveRightAssoc)
		curGunMoveRightAssoc->speed = animSpeed;

	if (padLeftRight < 0 && (Abs(padLeftRight) > 64 || Abs(padUpDown) < 64))
		curGunMoveLeftAssoc = CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_GUNMOVE_L, 16.0f);
	else if (padLeftRight > 0 && (Abs(padLeftRight) > 64 || Abs(padUpDown) < 64))
		curGunMoveRightAssoc = CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_GUNMOVE_R, 16.0f);

	if (padUpDown != 0) {
		if (padUpDown < 0) {
			if (!curGunMoveForwardAssoc)
				curGunMoveForwardAssoc = CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_GUNMOVE_FWD, 16.0f);
		} else if (padUpDown > 0) {
			if (!curGunMoveBackwardAssoc)
				curGunMoveBackwardAssoc = CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_GUNMOVE_BWD, 16.0f);
		}
	}
}

void CPlayerPed::StopAimingAnims(void)
{
	AnimationId gunAnims[] = { ANIM_STD_GUNMOVE_BWD, ANIM_STD_GUNMOVE_FWD, ANIM_STD_GUNMOVE_L, ANIM_STD_GUNMOVE_R };
	for (int i = 0; i < ARRAY_SIZE(gunAnims); i++) {
		CAnimBlendAssociation* curGunAnimAssoc = RpAnimBlendClumpGetAssociation(GetClump(), gunAnims[i]);
		if (curGunAnimAssoc) {
			curGunAnimAssoc->flags |= ASSOC_DELETEFADEDOUT;
			curGunAnimAssoc->blendDelta = -4.0f;
		}
	}
}

bool CPlayerPed::CanUseDriveBy(void)
{
	if (!FindPlayerVehicle())
		return false;

	CVehicle* vehicle = FindPlayerVehicle();

	bool bSuitableVehicle = !vehicle->IsRealHeli() && !vehicle->IsRealPlane() &&
		vehicle->GetModelIndex() != MI_RHINO && vehicle->GetModelIndex() != MI_FIRETRUCK;

	CCam currentCam = TheCamera.Cams[TheCamera.ActiveCam];
	bool bSuitableCamera = currentCam.DirectionWasLooking == LOOKING_FORWARD &&
		(currentCam.Mode == CCam::MODE_REAL_1ST_PERSON || currentCam.Mode == CCam::MODE_CAM_ON_A_STRING || currentCam.Mode == CCam::MODE_BEHINDBOAT);

	int8 currentWeaponSlot = CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->m_nWeaponSlot;
	bool bHaveWeapon = CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->m_nWeaponSlot > 2 &&
#ifdef VICEEXT_WEAPON_BERETTA
		(GetWeapon()->m_eWeaponType == WEAPONTYPE_COLT45 || GetWeapon()->m_eWeaponType == WEAPONTYPE_BERETTA || currentWeaponSlot == WEAPONSLOT_SUBMACHINEGUN);
#else
		(GetWeapon()->m_eWeaponType == WEAPONTYPE_COLT45 || currentWeaponSlot == WEAPONSLOT_SUBMACHINEGUN);
#endif

	return bSuitableVehicle && bHaveWeapon && GetUp().z >= 0.2f && bSuitableCamera;
}

bool CPlayerPed::IsDoomMode(void)
{
	return TheCamera.Cams[TheCamera.ActiveCam].Mode == CCam::MODE_REAL_1ST_PERSON && FrontEndMenuManager.m_PrefsDoomMode_FP && !InVehicle();
}
#endif

#ifdef __EMSCRIPTEN__
static void
ViceExtMovTrace(CPlayerPed *ped)
{
	static int s_odMovSig = -1;
	int agachado = ped->bIsDucking ? 1 : 0;
	int rueda = 0;
	int escala = 0;
	float camz = 0.0f;
#ifdef VICEEXT_CROUCH
	rueda = ped->m_nPedState == PED_ROLL ? 1 : 0;
	camz = TheCamera.Cams[TheCamera.ActiveCam].m_fTargetCameraPosZ;
#endif
#ifdef VICEEXT_CLIMBING
	escala = ped->bIsClimbing ? 1 : 0;
#endif
	int sig = agachado | (rueda << 1) | (escala << 2);
	if (sig == s_odMovSig)
		return;
	s_odMovSig = sig;
	char t[160];
	snprintf(t, sizeof t, "VEX mov agachado=%d rueda=%d escala=%d estado=%d nado=%d camz=%.2f",
		agachado, rueda, escala, (int)ped->m_nPedState, ViceExtPedOwns(PEDLANE_NADO, PEDCAP_POSTURA) ? 1 : 0, camz);
	ODTRACES(t);
}
#endif

#if defined __EMSCRIPTEN__ && defined VICEEXT_FIRING_AND_AIMING
static void
ViceExtAimTrace(CPlayerPed *ped)
{
	static int s_odAimSig = -1;
	int apunta = ped->bIsPlayerAiming ? 1 : 0;
	int coche = ped->InVehicle() ? 1 : 0;
	int modo = TheCamera.Cams[TheCamera.ActiveCam].Mode;
	int sig = apunta | (coche << 1) | (modo << 2);
	if (sig == s_odAimSig)
		return;
	s_odAimSig = sig;
	char t[160];
	snprintf(t, sizeof t, "VEX apunta apunta=%d coche=%d modo=%d arma=%d estado=%d",
		apunta, coche, modo, (int)ped->GetWeapon()->m_eWeaponType, (int)ped->m_nPedState);
	ODTRACES(t);
}
#endif

void
CPlayerPed::UseSprintEnergy(void)
{
	if (m_fCurrentStamina > -150.0f && !CWorld::Players[CWorld::PlayerInFocus].m_bInfiniteSprint
		&& !m_bAdrenalineActive) {
		m_fCurrentStamina = m_fCurrentStamina - CTimer::GetTimeStep();
		m_fStaminaProgress = m_fStaminaProgress + CTimer::GetTimeStep();
	}

	if (m_fStaminaProgress >= 500.0f) {
		m_fStaminaProgress = 0;
		if (m_fMaxStamina < 1000.0f)
			m_fMaxStamina += 10.0f;
	}
}

void
CPlayerPed::MakeChangesForNewWeapon(eWeaponType weapon)
{
	if (m_nPedState == PED_SNIPER_MODE) {
		RestorePreviousState();
		TheCamera.ClearPlayerWeaponMode();
	}
	SetCurrentWeapon(weapon);
	m_nSelectedWepSlot = m_currentWeapon;

#ifndef VICEEXT_IMPROVED_MENU_AND_INPUT
	GetWeapon()->m_nAmmoInClip = Min(GetWeapon()->m_nAmmoTotal, CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->m_nAmountofAmmunition);
#endif

#if defined VICEEXT_FIRING_AND_AIMING && defined VICEEXT_FIRST_PERSON
	if (IsDoomMode() && GetWeapon()->GetInfo()->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM))
		StopAimingAnims();

	if (CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->IsFlagSet(WEAPONFLAG_CANAIM) || IsDoomMode())
#else
	if (CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->IsFlagSet(WEAPONFLAG_CANAIM))
#endif
		ClearWeaponTarget();

	// WEAPONTYPE_SNIPERRIFLE? Wut?
	CAnimBlendAssociation* weaponAnim = RpAnimBlendClumpGetAssociation(GetClump(), GetPrimaryFireAnim(CWeaponInfo::GetWeaponInfo(WEAPONTYPE_SNIPERRIFLE)));
	if (weaponAnim) {
		weaponAnim->SetRun();
		weaponAnim->flags |= ASSOC_FADEOUTWHENDONE;
	}
	TheCamera.ClearPlayerWeaponMode();
}

void
CPlayerPed::MakeChangesForNewWeapon(int32 slot)
{
	if(slot != -1)
		MakeChangesForNewWeapon(m_weapons[slot].m_eWeaponType);
}

void
CPlayerPed::ReApplyMoveAnims(void)
{
	static AnimationId moveAnims[] = { ANIM_STD_WALK, ANIM_STD_RUN, ANIM_STD_RUNFAST, ANIM_STD_IDLE, ANIM_STD_STARTWALK };

	for(int i = 0; i < ARRAY_SIZE(moveAnims); i++) {
		CAnimBlendAssociation *curMoveAssoc = RpAnimBlendClumpGetAssociation(GetClump(), moveAnims[i]);
		if (curMoveAssoc) {
			if (CGeneral::faststrcmp(CAnimManager::GetAnimAssociation(m_animGroup, moveAnims[i])->hierarchy->name, curMoveAssoc->hierarchy->name)) {
				CAnimBlendAssociation *newMoveAssoc = CAnimManager::AddAnimation(GetClump(), m_animGroup, moveAnims[i]);
				newMoveAssoc->blendDelta = curMoveAssoc->blendDelta;
				newMoveAssoc->blendAmount = curMoveAssoc->blendAmount;
				curMoveAssoc->blendDelta = -1000.0f;
				curMoveAssoc->flags |= ASSOC_DELETEFADEDOUT;
			}
		}
	}
}

void
CPlayerPed::SetInitialState(void)
{
	m_nDrunkenness = 0;
	m_nFadeDrunkenness = 0;
	CMBlur::ClearDrunkBlur();
	m_nDrunkCountdown = 0;
	m_bAdrenalineActive = false;
	m_nAdrenalineTime = 0;
	CTimer::SetTimeScale(1.0f);
	m_pSeekTarget = nil;
	m_vecSeekPos = CVector(0.0f, 0.0f, 0.0f);
	m_fleeFromPos = CVector2D(0.0f, 0.0f);
	m_fleeFrom = nil;
	m_fleeTimer = 0;
	m_objective = OBJECTIVE_NONE;
	m_prevObjective = OBJECTIVE_NONE;
	bUsesCollision = true;
	ClearAimFlag();
	ClearLookFlag();
	bIsPointingGunAt = false;
	bRenderPedInCar = true;
	if (m_pFire)
		m_pFire->Extinguish();

	RpAnimBlendClumpRemoveAllAssociations(GetClump());
	SetPedState(PED_IDLE);
	SetMoveState(PEDMOVE_STILL);
	m_nLastPedState = PED_NONE;
	m_animGroup = ASSOCGRP_PLAYER;
	m_fMoveSpeed = 0.0f;
	m_nSelectedWepSlot = WEAPONSLOT_UNARMED;
	m_nEvadeAmount = 0;
	m_pEvadingFrom = nil;
	bIsPedDieAnimPlaying = false;
	SetRealMoveAnim();
	m_bCanBeDamaged = true;
	m_pedStats->m_temper = 50;
	m_fWalkAngle = 0.0f;
	if (m_attachedTo && !bUsesCollision)
		bUsesCollision = true;

	m_attachedTo = nil;
	m_attachWepAmmo = 0;
}

void
CPlayerPed::SetRealMoveAnim(void)
{
#ifdef VICEEXT_CROUCH
	if (bIsDucking) {
		ProcessCrouch();

		return;
	}
#endif

#ifdef VICEEXT_FIRING_AND_AIMING
	if ((m_nPedState == PED_ATTACK || m_nPedState == PED_AIM_GUN || bIsPlayerAiming) && GetWeapon()->GetInfo()->m_nWeaponSlot > 2 && !CanWeRunAndFireWithWeapon()) {
		ProcessAiming();

		return;
	}
#endif

	CAnimBlendAssociation *curWalkAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_WALK);
	CAnimBlendAssociation *curRunAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_RUN);
	CAnimBlendAssociation *curSprintAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_RUNFAST);
	CAnimBlendAssociation *curWalkStartAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_STARTWALK);
	CAnimBlendAssociation *curIdleAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_IDLE);
	CAnimBlendAssociation *curRunStopAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_RUNSTOP1);
	CAnimBlendAssociation *curRunStopRAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_RUNSTOP2);
	if (bResetWalkAnims) {
		if (curWalkAssoc)
			curWalkAssoc->SetCurrentTime(0.0f);
		if (curRunAssoc)
			curRunAssoc->SetCurrentTime(0.0f);
		if (curSprintAssoc)
			curSprintAssoc->SetCurrentTime(0.0f);
		bResetWalkAnims = false;
	}

	if (!curIdleAssoc)
		curIdleAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_IDLE_TIRED);
	if (!curIdleAssoc)
		curIdleAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_FIGHT_IDLE);
	if (!curIdleAssoc)
		curIdleAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_MELEE_IDLE_FIGHTMODE);

	if (!((curRunStopAssoc && curRunStopAssoc->IsRunning()) || (curRunStopRAssoc && curRunStopRAssoc->IsRunning()))) {

		if (curRunStopAssoc && curRunStopAssoc->blendDelta >= 0.0f || curRunStopRAssoc && curRunStopRAssoc->blendDelta >= 0.0f) {
			if (curRunStopAssoc) {
				curRunStopAssoc->flags |= ASSOC_DELETEFADEDOUT;
				curRunStopAssoc->blendAmount = 1.0f;
				curRunStopAssoc->blendDelta = -8.0f;
			} else if (curRunStopRAssoc) {
				curRunStopRAssoc->flags |= ASSOC_DELETEFADEDOUT;
				curRunStopRAssoc->blendAmount = 1.0f;
				curRunStopRAssoc->blendDelta = -8.0f;
			}
			
			RestoreHeadingRate();
			if (!curIdleAssoc) {
#ifdef VICEEXT_IMPROVED_TECH_PART
				if (!CPad::GetPad(0)->ArePlayerControlsDisabled() && m_fCurrentStamina < 0.0f && !bIsAimingGun && !CWorld::TestSphereAgainstWorld(GetPosition(), 0.5f,
#else
				if (m_fCurrentStamina < 0.0f && !bIsAimingGun && !CWorld::TestSphereAgainstWorld(GetPosition(), 0.5f,
#endif
						nil, true, false, false, false, false, false)) {
					curIdleAssoc = CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_IDLE_TIRED, 8.0f);

				} else {
					curIdleAssoc = CAnimManager::BlendAnimation(GetClump(), m_animGroup, ANIM_STD_IDLE, 8.0f);
				}
				m_nWaitTimer = CTimer::GetTimeInMilliseconds() + CGeneral::GetRandomNumberInRange(2500, 4000);
			}
			curIdleAssoc->blendAmount = 0.0f;
			curIdleAssoc->blendDelta = 8.0f;

		} else if (m_fMoveSpeed == 0.0f && !curSprintAssoc) {
			if (!curIdleAssoc) {
#ifdef VICEEXT_IMPROVED_TECH_PART
				if (!CPad::GetPad(0)->ArePlayerControlsDisabled() && m_fCurrentStamina < 0.0f && !bIsAimingGun && !CWorld::TestSphereAgainstWorld(GetPosition(), 0.5f,
#else
				if (m_fCurrentStamina < 0.0f && !bIsAimingGun && !CWorld::TestSphereAgainstWorld(GetPosition(), 0.5f,
#endif
						nil, true, false, false, false, false, false)) {
					curIdleAssoc = CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_IDLE_TIRED, 4.0f);
					
				} else {
					curIdleAssoc = CAnimManager::BlendAnimation(GetClump(), m_animGroup, ANIM_STD_IDLE, 4.0f);
				}

				m_nWaitTimer = CTimer::GetTimeInMilliseconds() + CGeneral::GetRandomNumberInRange(2500, 4000);
			}

			if ((m_fCurrentStamina > 0.0f || bIsAimingGun) && curIdleAssoc->animId == ANIM_STD_IDLE_TIRED) {
				CAnimManager::BlendAnimation(GetClump(), m_animGroup, ANIM_STD_IDLE, 4.0f);

			} else if (m_nPedState != PED_FIGHT) {
#ifdef VICEEXT_IMPROVED_TECH_PART
				if (!CPad::GetPad(0)->ArePlayerControlsDisabled() && m_fCurrentStamina < 0.0f && !bIsAimingGun && curIdleAssoc->animId != ANIM_STD_IDLE_TIRED
#else
				if (m_fCurrentStamina < 0.0f && !bIsAimingGun && curIdleAssoc->animId != ANIM_STD_IDLE_TIRED
#endif
					&& !CWorld::TestSphereAgainstWorld(GetPosition(), 0.5f, nil, true, false, false, false, false, false)) {
					CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_IDLE_TIRED, 4.0f);

				} else if (curIdleAssoc->animId != ANIM_STD_IDLE) {
					CAnimManager::BlendAnimation(GetClump(), m_animGroup, ANIM_STD_IDLE, 4.0f);
				}
			}
			m_nMoveState = PEDMOVE_STILL;

		} else {
			if (curIdleAssoc) {
				if (curWalkStartAssoc) {
					curWalkStartAssoc->blendAmount = 1.0f;
					curWalkStartAssoc->blendDelta = 0.0f;
				} else {
					curWalkStartAssoc = CAnimManager::AddAnimation(GetClump(), m_animGroup, ANIM_STD_STARTWALK);
				}
				if (curWalkAssoc)
					curWalkAssoc->SetCurrentTime(0.0f);
				if (curRunAssoc)
					curRunAssoc->SetCurrentTime(0.0f);

				delete curIdleAssoc;
				delete RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_IDLE_TIRED);
				CAnimBlendAssociation *fightIdleAnim = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_FIGHT_IDLE);
				if (!fightIdleAnim)
					fightIdleAnim = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_MELEE_IDLE_FIGHTMODE);
				delete fightIdleAnim;
				delete curSprintAssoc;

				curSprintAssoc = nil;
				m_nMoveState = PEDMOVE_WALK;
			}
			if (curRunStopAssoc) {
				delete curRunStopAssoc;
				RestoreHeadingRate();
			}
			if (curRunStopRAssoc) {
				delete curRunStopRAssoc;
				RestoreHeadingRate();
			}
			if (!curWalkAssoc) {
				curWalkAssoc = CAnimManager::AddAnimation(GetClump(), m_animGroup, ANIM_STD_WALK);
				curWalkAssoc->blendAmount = 0.0f;
			}
			if (!curRunAssoc) {
				curRunAssoc = CAnimManager::AddAnimation(GetClump(), m_animGroup, ANIM_STD_RUN);
				curRunAssoc->blendAmount = 0.0f;
			}
			if (curWalkStartAssoc && !(curWalkStartAssoc->IsRunning())) {
				delete curWalkStartAssoc;
				curWalkStartAssoc = nil;
				curWalkAssoc->SetRun();
				curRunAssoc->SetRun();
			}
			if (m_nMoveState == PEDMOVE_SPRINT) {
				if (m_fCurrentStamina < 0.0f && (m_fCurrentStamina <= -150.0f || !curSprintAssoc || curSprintAssoc->blendDelta < 0.0f))
					m_nMoveState = PEDMOVE_STILL;

				if (curWalkStartAssoc)
					m_nMoveState = PEDMOVE_STILL;
			}

			if (curSprintAssoc && (m_nMoveState != PEDMOVE_SPRINT || m_fMoveSpeed < 0.4f)) {
				// Stop sprinting in various conditions
				if (curSprintAssoc->blendAmount == 0.0f) {
					curSprintAssoc->blendDelta = -1000.0f;
					curSprintAssoc->flags |= ASSOC_DELETEFADEDOUT;

				} else if (curSprintAssoc->blendDelta >= 0.0f || curSprintAssoc->blendAmount >= 0.8f) {
					if (m_fMoveSpeed < 0.4f) {
						AnimationId runStopAnim;
						if (curSprintAssoc->GetProgress() < 0.5) // double
							runStopAnim = ANIM_STD_RUNSTOP1;
						else
							runStopAnim = ANIM_STD_RUNSTOP2;
						CAnimBlendAssociation* newRunStopAssoc = CAnimManager::AddAnimation(GetClump(), ASSOCGRP_STD, runStopAnim);
						newRunStopAssoc->blendAmount = 1.0f;
						newRunStopAssoc->SetDeleteCallback(RestoreHeadingRateCB, this);
						m_headingRate = 0.0f;
						curSprintAssoc->flags |= ASSOC_DELETEFADEDOUT;
						curSprintAssoc->blendDelta = -1000.0f;
						curWalkAssoc->flags &= ~ASSOC_RUNNING;
						curWalkAssoc->blendAmount = 0.0f;
						curWalkAssoc->blendDelta = 0.0f;
						curRunAssoc->flags &= ~ASSOC_RUNNING;
						curRunAssoc->blendAmount = 0.0f;
						curRunAssoc->blendDelta = 0.0f;

					} else if (curSprintAssoc->blendDelta >= 0.0f) { // this condition is absent on mobile
						// Stop sprinting when tired
						curSprintAssoc->flags |= ASSOC_DELETEFADEDOUT;
						curSprintAssoc->blendDelta = -1.0f;
						curRunAssoc->blendDelta = 1.0f;
					}
				} else if (m_fMoveSpeed < 1.0f) {
					curSprintAssoc->blendDelta = -8.0f;
					curRunAssoc->blendDelta = 8.0f;
				}

			} else if (curWalkStartAssoc) {
				// Walk start and walk/run shouldn't run at the same time
				curWalkAssoc->flags &= ~ASSOC_RUNNING;
				curRunAssoc->flags &= ~ASSOC_RUNNING;
				curWalkAssoc->blendAmount = 0.0f;
				curRunAssoc->blendAmount = 0.0f;

			} else if (m_nMoveState == PEDMOVE_SPRINT) {
				if (curSprintAssoc) {
					// We have anim, do it
					if (curSprintAssoc->blendDelta < 0.0f) {
						curSprintAssoc->blendDelta = 2.0f;
						curRunAssoc->blendDelta = -2.0f;
					}
				} else {
					// Transition between run-sprint
					curWalkAssoc->blendAmount = 0.0f;
					curRunAssoc->blendAmount = 1.0f;
					curSprintAssoc = CAnimManager::BlendAnimation(GetClump(), m_animGroup, ANIM_STD_RUNFAST, 2.0f);
				}
				UseSprintEnergy();
			} else {
				if (m_fMoveSpeed < 1.0f) {
					curWalkAssoc->blendAmount = 1.0f;
					curRunAssoc->blendAmount = 0.0f;
					m_nMoveState = PEDMOVE_WALK;
				} else if (m_fMoveSpeed < 2.0f) {
					curWalkAssoc->blendAmount = 2.0f - m_fMoveSpeed;
					curRunAssoc->blendAmount = m_fMoveSpeed - 1.0f;
					m_nMoveState = PEDMOVE_RUN;
				} else {
					curWalkAssoc->blendAmount = 0.0f;
					curRunAssoc->blendAmount = 1.0f;
					m_nMoveState = PEDMOVE_RUN;
				}
				curWalkAssoc->blendDelta = 0.0f;
				curRunAssoc->blendDelta = 0.0f;
#ifdef VICEEXT_IMPROVED_TECH_PART
				if (GetWeapon()->m_eWeaponType == WEAPONTYPE_CHAINSAW ||
					GetWeapon()->m_eWeaponType == WEAPONTYPE_MINIGUN ||
					GetWeapon()->m_eWeaponType == WEAPONTYPE_FLAMETHROWER) {
					curRunAssoc->speed = 0.75f;
				}
#endif
			}
		}
	}
	if (m_bAdrenalineActive) {
		if (CTimer::GetTimeInMilliseconds() > m_nAdrenalineTime) {
			m_bAdrenalineActive = false;
			CTimer::SetTimeScale(1.0f);
			if (curWalkStartAssoc)
				curWalkStartAssoc->speed = 1.0f;
			if (curWalkAssoc)
				curWalkAssoc->speed = 1.0f;
			if (curRunAssoc)
				curRunAssoc->speed = 1.0f;
			if (curSprintAssoc)
				curSprintAssoc->speed = 1.0f;
		} else {
			CTimer::SetTimeScale(1.0f / 3);
			if (curWalkStartAssoc)
				curWalkStartAssoc->speed = 2.0f;
			if (curWalkAssoc)
				curWalkAssoc->speed = 2.0f;
			if (curRunAssoc)
				curRunAssoc->speed = 2.0f;
			if (curSprintAssoc)
				curSprintAssoc->speed = 2.0f;
		}
	} else if (curSprintAssoc) {
		if (TheCamera.Cams[TheCamera.ActiveCam].Mode == CCam::MODE_FIXED) {
			curSprintAssoc->speed = 0.7f;
#ifdef VICEEXT_IMPROVED_TECH_PART
		} else if (GetWeapon()->GetInfo()->m_nWeaponSlot == WEAPONSLOT_SHOTGUN ||
				   GetWeapon()->GetInfo()->m_nWeaponSlot == WEAPONSLOT_RIFLE ||
				   GetWeapon()->GetInfo()->m_nWeaponSlot == WEAPONSLOT_SNIPER) {
			curSprintAssoc->speed = 0.85f;
		} else if (GetWeapon()->m_eWeaponType == WEAPONTYPE_M60 ||
				   GetWeapon()->m_eWeaponType == WEAPONTYPE_ROCKETLAUNCHER) {
			curSprintAssoc->speed = 0.75f;
#endif
		} else
			curSprintAssoc->speed = 1.0f;
	}
}

void
CPlayerPed::RestoreSprintEnergy(float restoreSpeed)
{
	if (m_fCurrentStamina < m_fMaxStamina)
		m_fCurrentStamina += restoreSpeed * CTimer::GetTimeStep() * 0.5f;
}

float
CPlayerPed::DoWeaponSmoothSpray(void)
{
#ifdef VICEEXT_FIRING_AND_AIMING
	return -1.0f;
#endif
	if (m_nPedState == PED_ATTACK && !m_pPointGunAt) {
		CWeaponInfo *weaponInfo = CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType);
		switch (GetWeapon()->m_eWeaponType) {
			case WEAPONTYPE_GOLFCLUB:
			case WEAPONTYPE_NIGHTSTICK:
			case WEAPONTYPE_BASEBALLBAT:
				if (GetFireAnimGround(weaponInfo, false) && RpAnimBlendClumpGetAssociation(GetClump(), GetFireAnimGround(weaponInfo, false)))
					return PI / 176.f;
				else
					return -1.0f;

			case WEAPONTYPE_CHAINSAW:
				if (GetMeleeStartAnim(weaponInfo) && RpAnimBlendClumpGetAssociation(GetClump(), GetMeleeStartAnim(weaponInfo))) {
#ifdef FREE_CAM
					if (TheCamera.Cams[0].Using3rdPersonMouseCam()) return -1.0f;
#endif
					return PI / 128.0f;
				}
				else if (GetFireAnimGround(weaponInfo, false) && RpAnimBlendClumpGetAssociation(GetClump(), GetFireAnimGround(weaponInfo, false)))
					return PI / 176.f;
				else
					return PI / 80.f;

			case WEAPONTYPE_PYTHON:
#ifdef VICEEXT_WEAPON_DEAGLE
			case WEAPONTYPE_DEAGLE:
#endif
				return PI / 112.f;
			case WEAPONTYPE_SHOTGUN:
			case WEAPONTYPE_SPAS12_SHOTGUN:
			case WEAPONTYPE_STUBBY_SHOTGUN:
#ifdef VICEEXT_WEAPON_SHOTGUN2
			case WEAPONTYPE_SHOTGUN2:
#endif
				return PI / 112.f;
			case WEAPONTYPE_UZI:
			case WEAPONTYPE_MP5:
#ifdef VICEEXT_WEAPON_UZIOLD
			case WEAPONTYPE_UZIOLD:
#endif
				return PI / 112.f;
			case WEAPONTYPE_M4:
			case WEAPONTYPE_RUGER:
#ifdef VICEEXT_WEAPON_AK47
			case WEAPONTYPE_AK47:
#endif
#ifdef VICEEXT_WEAPON_M16
			case WEAPONTYPE_M16:
#endif
#ifdef VICEEXT_WEAPON_STEYR
			case WEAPONTYPE_STEYR:
#endif
				return PI / 112.f;
			case WEAPONTYPE_FLAMETHROWER:
				return PI / 80.f;
			case WEAPONTYPE_M60:
			case WEAPONTYPE_MINIGUN:
			case WEAPONTYPE_HELICANNON:
				return PI / 176.f;
			default:
				return -1.0f;
		}
	} else if (bIsDucking)
		return PI / 112.f;
	else
		return -1.0f;
}

void
CPlayerPed::DoStuffToGoOnFire(void)
{
	if (m_nPedState == PED_SNIPER_MODE)
		TheCamera.ClearPlayerWeaponMode();
}

bool
CPlayerPed::DoesTargetHaveToBeBroken(CVector target, CWeapon *weaponUsed)
{
	CVector distVec = target - GetPosition();

	if (distVec.Magnitude() > CWeaponInfo::GetWeaponInfo(weaponUsed->m_eWeaponType)->m_fRange)
		return true;

	return false;
}

// Cancels landing anim while running & jumping? I think
void
CPlayerPed::RunningLand(CPad *padUsed)
{
	CAnimBlendAssociation *landAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_FALL_LAND);
	if (landAssoc && landAssoc->currentTime == 0.0f && m_fMoveSpeed > 1.5f
		&& padUsed && (padUsed->GetPedWalkLeftRight() != 0.0f || padUsed->GetPedWalkUpDown() != 0.0f)) {

		landAssoc->blendDelta = -1000.0f;
		landAssoc->flags |= ASSOC_DELETEFADEDOUT;

		CAnimManager::AddAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_JUMP_LAND)->SetFinishCallback(FinishJumpCB, this);

		if (m_nPedState == PED_JUMP)
			RestorePreviousState();
	}
}

bool
CPlayerPed::IsThisPedAnAimingPriority(CPed *suspect)
{
	if (!suspect->bIsPlayerFriend)
		return true;

	if (suspect->m_pPointGunAt == this)
		return true;

	switch (suspect->m_objective) {
		case OBJECTIVE_KILL_CHAR_ON_FOOT:
		case OBJECTIVE_KILL_CHAR_ANY_MEANS:
			if (suspect->m_pedInObjective == this)
				return true;

			break;
		default:
			break;
	}
	return suspect->m_nPedState == PED_ABSEIL;
}

void
CPlayerPed::PlayerControlSniper(CPad *padUsed)
{
	ProcessWeaponSwitch(padUsed);
	TheCamera.PlayerExhaustion = (1.0f - (m_fCurrentStamina - -150.0f) / 300.0f) * 0.9f + 0.1f;

#ifndef VICEEXT_FIRING_AND_AIMING
	if (padUsed->DuckJustDown() && !bIsDucking && m_nMoveState != PEDMOVE_SPRINT) {
		bCrouchWhenShooting = true;
		SetDuck(60000, true);
	} else if (bIsDucking && (padUsed->DuckJustDown() || m_nMoveState == PEDMOVE_SPRINT)) {
		ClearDuck(true);
		bCrouchWhenShooting = false;
	}
#endif

	if (!padUsed->GetTarget() && !m_attachedTo) {
		RestorePreviousState();
		TheCamera.ClearPlayerWeaponMode();
		return;
	}

	int firingRate = GetWeapon()->m_eWeaponType == WEAPONTYPE_LASERSCOPE ? 333 : 266;
	if (padUsed->WeaponJustDown() && CTimer::GetTimeInMilliseconds() > GetWeapon()->m_nTimer) {
		CVector firePos(0.0f, 0.0f, 0.6f);
		firePos = GetMatrix() * firePos;
		GetWeapon()->Fire(this, &firePos);
		m_nPadDownPressedInMilliseconds = CTimer::GetTimeInMilliseconds();

#ifdef VICEEXT_FIRING_AND_AIMING
		if (GetWeapon()->m_nAmmoTotal > 0)
			DMAudio.PlayOneShot(m_audioEntityId, SOUND_WEAPON_AK47_BULLET_ECHO, GetWeapon()->m_eWeaponType);
#endif
	} else if (CTimer::GetTimeInMilliseconds() > m_nPadDownPressedInMilliseconds + firingRate &&
		CTimer::GetTimeInMilliseconds() - CTimer::GetTimeStepInMilliseconds() < m_nPadDownPressedInMilliseconds + firingRate && padUsed->GetWeapon()) {
		
		if (GetWeapon()->m_nAmmoTotal > 0) {
			DMAudio.PlayFrontEndSound(SOUND_WEAPON_AK47_BULLET_ECHO, GetWeapon()->m_eWeaponType);
		}
	}
	GetWeapon()->Update(m_audioEntityId, nil);

#ifdef VICEEXT_FIRING_AND_AIMING
	PlayerControlZelda(padUsed);
#endif
}

// I think R* also used goto in here.
void
CPlayerPed::ProcessWeaponSwitch(CPad *padUsed)
{
#ifdef VICEEXT_SWIMMING
	if (ViceExtPedOwns(PEDLANE_NADO, PEDCAP_ARMA)) {
		m_nSelectedWepSlot = s_odSwimSlot;
		goto switchDetectDone;
	}
#endif
	if (CDarkel::FrenzyOnGoing() || m_attachedTo)
		goto switchDetectDone;

#ifdef VICEEXT_IMPROVED_MENU_AND_INPUT
	if (!m_pPointGunAt && !bDontAllowWeaponChange && GetWeapon()->m_eWeaponState != WEAPONSTATE_RELOADING) {
#else
	if (!m_pPointGunAt && !bDontAllowWeaponChange && GetWeapon()->m_eWeaponType != WEAPONTYPE_DETONATOR) {
#endif
		if (padUsed->CycleWeaponRightJustDown()) {

			if (TheCamera.PlayerWeaponMode.Mode != CCam::MODE_M16_1STPERSON
				&& TheCamera.PlayerWeaponMode.Mode != CCam::MODE_M16_1STPERSON_RUNABOUT
				&& TheCamera.PlayerWeaponMode.Mode != CCam::MODE_SNIPER
				&& TheCamera.PlayerWeaponMode.Mode != CCam::MODE_SNIPER_RUNABOUT
				&& TheCamera.PlayerWeaponMode.Mode != CCam::MODE_ROCKETLAUNCHER
				&& TheCamera.PlayerWeaponMode.Mode != CCam::MODE_ROCKETLAUNCHER_RUNABOUT
				&& TheCamera.PlayerWeaponMode.Mode != CCam::MODE_CAMERA) {

				for (m_nSelectedWepSlot = m_currentWeapon + 1; m_nSelectedWepSlot < TOTAL_WEAPON_SLOTS; ++m_nSelectedWepSlot) {
					if (HasWeaponSlot(m_nSelectedWepSlot) && GetWeapon(m_nSelectedWepSlot).HasWeaponAmmoToBeUsed()) {
#ifdef FIX_BUGS
						goto switchDetectDone;
#else
						goto spentAmmoCheck;
#endif
					}
				}
				m_nSelectedWepSlot = 0;
#ifdef FIX_BUGS
				goto switchDetectDone;
#endif
			}
		} else if (padUsed->CycleWeaponLeftJustDown()) {
			if (TheCamera.PlayerWeaponMode.Mode != CCam::MODE_M16_1STPERSON
				&& TheCamera.PlayerWeaponMode.Mode != CCam::MODE_SNIPER
				&& TheCamera.PlayerWeaponMode.Mode != CCam::MODE_ROCKETLAUNCHER
				&& TheCamera.PlayerWeaponMode.Mode != CCam::MODE_CAMERA) {

				// I don't know what kind of loop that was
				m_nSelectedWepSlot = m_currentWeapon - 1;
				do {
					if (m_nSelectedWepSlot < 0)
						m_nSelectedWepSlot = TOTAL_WEAPON_SLOTS - 1;

					if (m_nSelectedWepSlot == WEAPONSLOT_UNARMED)
						break;

					if (HasWeaponSlot(m_nSelectedWepSlot) && GetWeapon(m_nSelectedWepSlot).HasWeaponAmmoToBeUsed())
						break;
					
					--m_nSelectedWepSlot;
				} while (m_nSelectedWepSlot != WEAPONSLOT_UNARMED);
#ifdef FIX_BUGS
				goto switchDetectDone;
#endif

			}
		}
	}
	
spentAmmoCheck:
	if (CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->m_eWeaponFire != WEAPON_FIRE_MELEE
		&& (!padUsed->GetWeapon() || GetWeapon()->m_eWeaponType != WEAPONTYPE_MINIGUN)) {
		if (GetWeapon()->m_nAmmoTotal <= 0) {
			if (TheCamera.PlayerWeaponMode.Mode == CCam::MODE_M16_1STPERSON
				|| TheCamera.PlayerWeaponMode.Mode == CCam::MODE_SNIPER
				|| TheCamera.PlayerWeaponMode.Mode == CCam::MODE_ROCKETLAUNCHER)
				return;

			if (GetWeapon()->m_eWeaponType == WEAPONTYPE_DETONATOR
				&& GetWeapon(WEAPONSLOT_PROJECTILE).m_eWeaponType == WEAPONTYPE_DETONATOR_GRENADE)
				m_nSelectedWepSlot = WEAPONSLOT_PROJECTILE;
			else
				m_nSelectedWepSlot = m_currentWeapon - 1;

			for (; m_nSelectedWepSlot >= WEAPONSLOT_UNARMED; --m_nSelectedWepSlot) {

				// BUG: m_nSelectedWepSlot and GetWeapon(..) takes slot in VC but they compared them against weapon types in whole condition! jeez
#ifdef FIX_BUGS
				if (m_nSelectedWepSlot == WEAPONSLOT_MELEE ||
					GetWeapon(m_nSelectedWepSlot).m_nAmmoTotal > 0 && (m_nSelectedWepSlot != WEAPONSLOT_PROJECTILE || GetWeapon(WEAPONSLOT_PROJECTILE).m_eWeaponType == WEAPONTYPE_DETONATOR_GRENADE)) {
#else
				if (m_nSelectedWepSlot == WEAPONTYPE_BASEBALLBAT && GetWeapon(WEAPONTYPE_BASEBALLBAT).m_eWeaponType == WEAPONTYPE_BASEBALLBAT
					|| GetWeapon(m_nSelectedWepSlot).m_nAmmoTotal > 0
					&& m_nSelectedWepSlot != WEAPONTYPE_MOLOTOV && m_nSelectedWepSlot != WEAPONTYPE_GRENADE && m_nSelectedWepSlot != WEAPONTYPE_TEARGAS) {
#endif
					goto switchDetectDone;
				}
			}
			m_nSelectedWepSlot = WEAPONSLOT_UNARMED;
		}
	}

switchDetectDone:
	if (m_nSelectedWepSlot != m_currentWeapon) {
#ifdef VICEEXT_FIRST_PERSON
		if (m_nPedState != PED_ATTACK && (m_nPedState != PED_AIM_GUN || IsDoomMode()) && m_nPedState != PED_FIGHT) {
#else
		if (m_nPedState != PED_ATTACK && m_nPedState != PED_AIM_GUN && m_nPedState != PED_FIGHT) {
#endif
			RemoveWeaponAnims(m_currentWeapon, -1000.0f);
			MakeChangesForNewWeapon(m_nSelectedWepSlot);
		}
	}
}

void
CPlayerPed::PlayerControlM16(CPad *padUsed)
{
	ProcessWeaponSwitch(padUsed);
	TheCamera.PlayerExhaustion = (1.0f - (m_fCurrentStamina - -150.0f) / 300.0f) * 0.9f + 0.1f;

#ifndef VICEEXT_FIRING_AND_AIMING
	if (padUsed->DuckJustDown() && !bIsDucking && m_nMoveState != PEDMOVE_SPRINT) {
		bCrouchWhenShooting = true;
		SetDuck(60000, true);
	} else if (bIsDucking && (padUsed->DuckJustDown() || m_nMoveState == PEDMOVE_SPRINT)) {
		ClearDuck(true);
		bCrouchWhenShooting = false;
	}
#endif

	if (!padUsed->GetTarget() && !m_attachedTo) {
		RestorePreviousState();
		TheCamera.ClearPlayerWeaponMode();
	}

	if (padUsed->GetWeapon() && CTimer::GetTimeInMilliseconds() > GetWeapon()->m_nTimer) {
		if (GetWeapon()->m_eWeaponState == WEAPONSTATE_OUT_OF_AMMO) {
			DMAudio.PlayFrontEndSound(SOUND_WEAPON_SNIPER_SHOT_NO_ZOOM, 0.f);
			GetWeapon()->m_nTimer = CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->m_nFiringRate + CTimer::GetTimeInMilliseconds();
		} else {
			CVector firePos(0.0f, 0.0f, 0.6f);
			firePos = GetMatrix() * firePos;
			GetWeapon()->Fire(this, &firePos);
			m_nPadDownPressedInMilliseconds = CTimer::GetTimeInMilliseconds();
		}
	} else if (CTimer::GetTimeInMilliseconds() > GetWeapon()->m_nTimer &&
		CTimer::GetTimeInMilliseconds() - CTimer::GetTimeStepInMilliseconds() < GetWeapon()->m_nTimer && GetWeapon()->m_eWeaponState != WEAPONSTATE_OUT_OF_AMMO) {
		DMAudio.PlayFrontEndSound(SOUND_WEAPON_AK47_BULLET_ECHO, GetWeapon()->m_eWeaponType);
	}
	GetWeapon()->Update(m_audioEntityId, nil);
}

void
CPlayerPed::PlayerControlFighter(CPad *padUsed)
{
	float leftRight = padUsed->GetPedWalkLeftRight();
	float upDown = padUsed->GetPedWalkUpDown();
	float padMove = CVector2D(leftRight, upDown).Magnitude();

	if (padMove > 0.0f) {
		m_fRotationDest = CGeneral::GetRadianAngleBetweenPoints(0.0f, 0.0f, -leftRight, upDown) - TheCamera.Orientation;
		m_takeAStepAfterAttack = padMove > (2 * PAD_MOVE_TO_GAME_WORLD_MOVE);
		if (padUsed->GetSprint() && padMove > (1 * PAD_MOVE_TO_GAME_WORLD_MOVE))
			bIsAttacking = false;
	}

	if (!CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->IsFlagSet(WEAPONFLAG_HEAVY) && padUsed->JumpJustDown()) {
		if (m_nEvadeAmount != 0 && m_pEvadingFrom) {
			SetEvasiveDive((CPhysical*)m_pEvadingFrom, 1);
			m_nEvadeAmount = 0;
			m_pEvadingFrom = nil;
		} else {
			SetJump();
		}
	}
}

void
CPlayerPed::PlayerControl1stPersonRunAround(CPad *padUsed)
{
	float leftRight = padUsed->GetPedWalkLeftRight();
	float upDown = padUsed->GetPedWalkUpDown();
	float padMove = CVector2D(leftRight, upDown).Magnitude();
	float padMoveInGameUnit = padMove / PAD_MOVE_TO_GAME_WORLD_MOVE;
	if (padMoveInGameUnit > 0.0f) {
		m_fRotationDest = CGeneral::LimitRadianAngle(TheCamera.Orientation);
		m_fMoveSpeed = Min(padMoveInGameUnit, 0.07f * CTimer::GetTimeStep() + m_fMoveSpeed);
	} else {
		m_fMoveSpeed = 0.0f;
	}

	if (m_nPedState == PED_JUMP) {
		if (bIsInTheAir) {
			if (bUsesCollision && !bHitSteepSlope && (!bHitSomethingLastFrame || m_vecDamageNormal.z > 0.6f)
				&& m_fDistanceTravelled < CTimer::GetTimeStepInSeconds() && m_vecMoveSpeed.MagnitudeSqr() < 0.01f) {

				float angleSin = Sin(m_fRotationCur); // originally sin(DEGTORAD(RADTODEG(m_fRotationCur))) o_O
				float angleCos = Cos(m_fRotationCur);
				ApplyMoveForce(-angleSin * 3.0f, 3.0f * angleCos, 0.05f);
			}
		} else if (bIsLanding) {
			m_fMoveSpeed = 0.0f;
		}
	}

	if (m_nPedState == PED_ANSWER_MOBILE) {
		SetRealMoveAnim();
		return;
	}

	if (!CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->IsFlagSet(WEAPONFLAG_HEAVY) && padUsed->GetSprint()) {
		m_nMoveState = PEDMOVE_SPRINT;
	}
	if (m_nPedState != PED_FIGHT)
		SetRealMoveAnim();

	if (!bIsInTheAir && !(CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->IsFlagSet(WEAPONFLAG_HEAVY)) &&
		padUsed->JumpJustDown() && m_nPedState != PED_JUMP) {

		ClearAttack();
		ClearWeaponTarget();
		if (m_nEvadeAmount != 0 && m_pEvadingFrom) {
			SetEvasiveDive((CPhysical*)m_pEvadingFrom, 1);
			m_nEvadeAmount = 0;
			m_pEvadingFrom = nil;
		} else {
			SetJump();
		}
	}

	// FIX: Fact that PlayIdleAnimations only called through PlayerControlZelda was making it visible to only Classic control players. This isn't fair!
#ifdef FIX_BUGS
	if (m_nPedState != PED_FIGHT)
		PlayIdleAnimations(padUsed);
#endif
}

void
CPlayerPed::KeepAreaAroundPlayerClear(void)
{
	BuildPedLists();
	for (int i = 0; i < m_numNearPeds; ++i) {
		CPed *nearPed = m_nearPeds[i];
		if (nearPed->CharCreatedBy == RANDOM_CHAR && nearPed->m_nPedState != PED_DRIVING && !nearPed->DyingOrDead()) {
			if (nearPed->GetIsOnScreen()) {
				if (nearPed->m_objective == OBJECTIVE_NONE) {
					nearPed->SetFindPathAndFlee(this, 5000, true);
				} else {
					if (nearPed->EnteringCar())
						nearPed->QuitEnteringCar();

					nearPed->ClearObjective();
				}
			} else {
				nearPed->FlagToDestroyWhenNextProcessed();
			}
		}
	}
	CVector playerPos = (InVehicle() ? m_pMyVehicle->GetPosition() : GetPosition());

	CVector pos = GetPosition();
	int16 lastVehicle;
	CEntity *vehicles[8];
	CWorld::FindObjectsInRange(pos, CHECK_NEARBY_THINGS_MAX_DIST, true, &lastVehicle, 6, vehicles, false, true, false, false, false);

	for (int i = 0; i < lastVehicle; i++) {
		CVehicle *veh = (CVehicle*)vehicles[i];
		if (veh->VehicleCreatedBy != MISSION_VEHICLE) {
			if (veh->GetStatus() != STATUS_PLAYER && veh->GetStatus() != STATUS_PLAYER_DISABLED) {
				if ((veh->GetPosition() - playerPos).MagnitudeSqr() > 25.0f) {
					veh->AutoPilot.m_nTempAction = TEMPACT_WAIT;
					veh->AutoPilot.m_nTimeTempAction = CTimer::GetTimeInMilliseconds() + 5000;
				} else {
					if (DotProduct2D(playerPos - veh->GetPosition(), veh->GetForward()) > 0.0f)
						veh->AutoPilot.m_nTempAction = TEMPACT_REVERSE;
					else
						veh->AutoPilot.m_nTempAction = TEMPACT_GOFORWARD;

					veh->AutoPilot.m_nTimeTempAction = CTimer::GetTimeInMilliseconds() + 2000;
				}
				CCarCtrl::PossiblyRemoveVehicle(veh);
			}
		}
	}
}

void
CPlayerPed::EvaluateNeighbouringTarget(CEntity *candidate, CEntity **targetPtr, float *lastCloseness, float distLimit, float angleOffset, bool lookToLeft, bool priority)
{
	// priority param is unused
	CVector distVec = candidate->GetPosition() - GetPosition();
	if (distVec.Magnitude2D() <= distLimit) {
		if (!DoesTargetHaveToBeBroken(candidate->GetPosition(), GetWeapon())) {
			float angleBetweenUs = CGeneral::GetATanOfXY(candidate->GetPosition().x - TheCamera.GetPosition().x,
				candidate->GetPosition().y - TheCamera.GetPosition().y);

			angleBetweenUs = CGeneral::LimitAngle(angleBetweenUs - angleOffset);
			float closeness;
			if (lookToLeft) {
				closeness = angleBetweenUs > 0.0f ? -Abs(angleBetweenUs) : -100000.0f;
			} else {
				closeness = angleBetweenUs > 0.0f ? -100000.0f : -Abs(angleBetweenUs);
			}

			if (closeness > *lastCloseness) {
				*targetPtr = candidate;
				*lastCloseness = closeness;
			}
		}
	}
}

void
CPlayerPed::EvaluateTarget(CEntity *candidate, CEntity **targetPtr, float *lastCloseness, float distLimit, float angleOffset, bool priority)
{
	CVector distVec = candidate->GetPosition() - GetPosition();
	float dist = distVec.Magnitude2D();
	if (dist <= distLimit) {
		if (!DoesTargetHaveToBeBroken(candidate->GetPosition(), GetWeapon())) {
			float angleBetweenUs = CGeneral::GetATanOfXY(distVec.x, distVec.y);
			angleBetweenUs = CGeneral::LimitAngle(angleBetweenUs - angleOffset);

			float closeness = -dist - 5.0f * Abs(angleBetweenUs);
			if (priority) {
				closeness += 30.0f;
			}

			if (closeness > *lastCloseness) {
				*targetPtr = candidate;
				*lastCloseness = closeness;
			}
		}
	}
}

bool
CPlayerPed::CanIKReachThisTarget(CVector target, CWeapon* weapon, bool zRotImportant)
{
	float angleToFace = CGeneral::GetRadianAngleBetweenPoints(target.x, target.y, GetPosition().x, GetPosition().y);
	float angleDiff = CGeneral::LimitRadianAngle(angleToFace - m_fRotationCur);

#ifdef VICEEXT_FIRING_AND_AIMING
	return (!zRotImportant || CWeaponInfo::GetWeaponInfo(weapon->m_eWeaponType)->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM) || Abs(angleDiff) <= HALFPI) && TheCamera.IsSphereVisible(target, 1.0f) &&
#else
	return (!zRotImportant || CWeaponInfo::GetWeaponInfo(weapon->m_eWeaponType)->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM) || Abs(angleDiff) <= HALFPI) &&
#endif
		(CWeaponInfo::GetWeaponInfo(weapon->m_eWeaponType)->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM) || Abs(target.z - GetPosition().z) <= (target - GetPosition()).Magnitude2D());
}

void
CPlayerPed::RotatePlayerToTrackTarget(void)
{
	if (CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM))
		return;

	float angleToFace = CGeneral::GetRadianAngleBetweenPoints(
		m_pPointGunAt->GetPosition().x, m_pPointGunAt->GetPosition().y,
		GetPosition().x, GetPosition().y);

	float angleDiff = CGeneral::LimitRadianAngle(m_fRotationCur - angleToFace);
	if (angleDiff < -DEGTORAD(25.0f)) {
		m_fRotationCur -= angleDiff + DEGTORAD(25.0f);
		m_fRotationDest -= angleDiff + DEGTORAD(25.0f);

	} else if (angleDiff > DEGTORAD(25.0f)) {
		m_fRotationCur -= angleDiff - DEGTORAD(25.0f);
		m_fRotationDest -= angleDiff - DEGTORAD(25.0f);
	}
}

bool
CPlayerPed::FindNextWeaponLockOnTarget(CEntity *previousTarget, bool lookToLeft)
{
	CEntity *nextTarget = nil;
	float weaponRange = CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->m_fRange;
	// nextTarget = nil; // duplicate
	float lastCloseness = -10000.0f;
	// CGeneral::GetATanOfXY(GetForward().x, GetForward().y); // unused
	CVector distVec = previousTarget->GetPosition() - TheCamera.GetPosition();
	float referenceBeta = CGeneral::GetATanOfXY(distVec.x, distVec.y);

	for (int h = CPools::GetPedPool()->GetSize() - 1; h >= 0; h--) {
		CPed *pedToCheck = CPools::GetPedPool()->GetSlot(h);
		if (pedToCheck) {
			if (pedToCheck != this && pedToCheck != previousTarget) {
				if (!pedToCheck->DyingOrDead()
#ifndef AIMING_VEHICLE_OCCUPANTS // Mobile thing
					&& (!pedToCheck->bInVehicle || (pedToCheck->m_pMyVehicle && pedToCheck->m_pMyVehicle->IsBike()))
#endif
					&& pedToCheck->m_leader != this && !pedToCheck->bNeverEverTargetThisPed
					&& OurPedCanSeeThisOne(pedToCheck, true) && CanIKReachThisTarget(pedToCheck->GetPosition(), GetWeapon(), true)) {

					EvaluateNeighbouringTarget(pedToCheck, &nextTarget, &lastCloseness,
						weaponRange, referenceBeta, lookToLeft, IsThisPedAnAimingPriority(pedToCheck));
				}
			}
		}
	}
	for (int i = 0; i < ARRAY_SIZE(m_nTargettableObjects); i++) {
		CObject *obj = CPools::GetObjectPool()->GetAt(m_nTargettableObjects[i]);
		if (obj && !obj->bHasBeenDamaged && CanIKReachThisTarget(obj->GetPosition(), GetWeapon(), true))
			EvaluateNeighbouringTarget(obj, &nextTarget, &lastCloseness, weaponRange, referenceBeta, lookToLeft, true);
	}
	if (!nextTarget)
		return false;

	SetWeaponLockOnTarget(nextTarget);
	bDontAllowWeaponChange = true;
	SetPointGunAt(nextTarget);
	return true;
}

bool
CPlayerPed::FindWeaponLockOnTarget(void)
{
	CEntity *nextTarget = nil;
	float weaponRange = CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->m_fRange;

	if (m_pPointGunAt) {
		CVector distVec = m_pPointGunAt->GetPosition() - GetPosition();
		if (distVec.Magnitude2D() > weaponRange) {
			SetWeaponLockOnTarget(nil);
			return false;
		} else {
			return true;
		}
	}

	// nextTarget = nil; // duplicate
	float lastCloseness = -10000.0f;
	float referenceBeta = CGeneral::GetATanOfXY(GetForward().x, GetForward().y);
	for (int h = CPools::GetPedPool()->GetSize() - 1; h >= 0; h--) {
		CPed *pedToCheck = CPools::GetPedPool()->GetSlot(h);
		if (pedToCheck) {
			if (pedToCheck != this) {
				if (!pedToCheck->DyingOrDead()
#ifndef AIMING_VEHICLE_OCCUPANTS // Mobile thing
					&& (!pedToCheck->bInVehicle || (pedToCheck->m_pMyVehicle && pedToCheck->m_pMyVehicle->IsBike()))
#endif
					&& pedToCheck->m_leader != this && !pedToCheck->bNeverEverTargetThisPed
					&& OurPedCanSeeThisOne(pedToCheck) && CanIKReachThisTarget(pedToCheck->GetPosition(), GetWeapon(), true)) {

					EvaluateTarget(pedToCheck, &nextTarget, &lastCloseness,
						weaponRange, referenceBeta, IsThisPedAnAimingPriority(pedToCheck));
				}
			}
		}
	}
	for (int i = 0; i < ARRAY_SIZE(m_nTargettableObjects); i++) {
		CObject *obj = CPools::GetObjectPool()->GetAt(m_nTargettableObjects[i]);
		if (obj && !obj->bHasBeenDamaged && CanIKReachThisTarget(obj->GetPosition(), GetWeapon(), true))
			EvaluateTarget(obj, &nextTarget, &lastCloseness, weaponRange, referenceBeta, true);
	}
	if (!nextTarget)
		return false;

#ifdef VICEEXT_IMPROVED_MENU_AND_INPUT
	bIsAutoAiming = true;
#endif
	SetWeaponLockOnTarget(nextTarget);
	bDontAllowWeaponChange = true;
	SetPointGunAt(nextTarget);
	Say(SOUND_PED_AIMING);
	return true;
}

void
CPlayerPed::ProcessAnimGroups(void)
{
	AssocGroupId groupToSet;
#ifdef PC_PLAYER_CONTROLS
	if ((m_fWalkAngle <= -DEGTORAD(50.0f) || m_fWalkAngle >= DEGTORAD(50.0f))
#ifndef VICEEXT_IMPROVED_MENU_AND_INPUT
		&& TheCamera.Cams[TheCamera.ActiveCam].Using3rdPersonMouseCam()
#endif
		&& CanStrafeOrMouseControl()) {

		if (m_fWalkAngle >= -DEGTORAD(130.0f) && m_fWalkAngle <= DEGTORAD(130.0f)) {
			if (m_fWalkAngle > 0.0f) {
				if (GetWeapon()->m_eWeaponType == WEAPONTYPE_ROCKETLAUNCHER)
					groupToSet = ASSOCGRP_ROCKETLEFT;
				else if (GetWeapon()->m_eWeaponType == WEAPONTYPE_CHAINSAW ||
					GetWeapon()->m_eWeaponType == WEAPONTYPE_FLAMETHROWER ||
					GetWeapon()->m_eWeaponType == WEAPONTYPE_MINIGUN)
					groupToSet = ASSOCGRP_CHAINSAWLEFT;
				else
					groupToSet = ASSOCGRP_PLAYERLEFT;
			} else {
				if (GetWeapon()->m_eWeaponType == WEAPONTYPE_ROCKETLAUNCHER)
					groupToSet = ASSOCGRP_ROCKETRIGHT;
				else if (GetWeapon()->m_eWeaponType == WEAPONTYPE_CHAINSAW ||
					GetWeapon()->m_eWeaponType == WEAPONTYPE_FLAMETHROWER ||
					GetWeapon()->m_eWeaponType == WEAPONTYPE_MINIGUN)
					groupToSet = ASSOCGRP_CHAINSAWRIGHT;
				else
					groupToSet = ASSOCGRP_PLAYERRIGHT;
			}
		} else {
			if (GetWeapon()->m_eWeaponType == WEAPONTYPE_ROCKETLAUNCHER)
				groupToSet = ASSOCGRP_ROCKETBACK;
			else if (GetWeapon()->m_eWeaponType == WEAPONTYPE_CHAINSAW ||
				GetWeapon()->m_eWeaponType == WEAPONTYPE_FLAMETHROWER ||
				GetWeapon()->m_eWeaponType == WEAPONTYPE_MINIGUN)
				groupToSet = ASSOCGRP_CHAINSAWBACK;
			else
				groupToSet = ASSOCGRP_PLAYERBACK;
		}
	} else
#endif
	{
		if (GetWeapon()->m_eWeaponType == WEAPONTYPE_ROCKETLAUNCHER) {
			groupToSet = ASSOCGRP_PLAYERROCKET;
		} else {
			if (GetWeapon()->m_eWeaponType == WEAPONTYPE_BASEBALLBAT
				 || GetWeapon()->m_eWeaponType == WEAPONTYPE_MACHETE)
				groupToSet = ASSOCGRP_PLAYERBBBAT;
			else if (GetWeapon()->m_eWeaponType == WEAPONTYPE_CHAINSAW ||
				GetWeapon()->m_eWeaponType == WEAPONTYPE_FLAMETHROWER ||
				GetWeapon()->m_eWeaponType == WEAPONTYPE_MINIGUN)
				groupToSet = ASSOCGRP_PLAYERCHAINSAW;
			else if (GetWeapon()->m_eWeaponType != WEAPONTYPE_COLT45 && GetWeapon()->m_eWeaponType != WEAPONTYPE_UZI
				// I hope this is an inlined function...
				&& GetWeapon()->m_eWeaponType != WEAPONTYPE_PYTHON && GetWeapon()->m_eWeaponType != WEAPONTYPE_TEC9
				&& GetWeapon()->m_eWeaponType != WEAPONTYPE_SILENCED_INGRAM && GetWeapon()->m_eWeaponType != WEAPONTYPE_MP5
				&& GetWeapon()->m_eWeaponType != WEAPONTYPE_GOLFCLUB && GetWeapon()->m_eWeaponType != WEAPONTYPE_KATANA
#ifdef VICEEXT_WEAPON_UZIOLD
				&& GetWeapon()->m_eWeaponType != WEAPONTYPE_UZIOLD
#endif
#ifdef VICEEXT_WEAPON_DEAGLE
				&& GetWeapon()->m_eWeaponType != WEAPONTYPE_DEAGLE
#endif
#ifdef VICEEXT_WEAPON_BERETTA
				&& GetWeapon()->m_eWeaponType != WEAPONTYPE_BERETTA
#endif
				&& GetWeapon()->m_eWeaponType != WEAPONTYPE_CAMERA) {
				if (!GetWeapon()->IsType2Handed()) {
					groupToSet = ASSOCGRP_PLAYER;
				} else {
					groupToSet = ASSOCGRP_PLAYER2ARMED;
				}
			} else {
				groupToSet = ASSOCGRP_PLAYER1ARMED;
			}
		}
	}

	if (m_animGroup != groupToSet) {
		m_animGroup = groupToSet;
		ReApplyMoveAnims();
	}
}

void
CPlayerPed::ProcessPlayerWeapon(CPad *padUsed)
{
#ifdef VICEEXT_SWIMMING
	if (ViceExtPedOwns(PEDLANE_NADO, PEDCAP_ARMA))
		ClearPointGunAt();
#endif
	CWeaponInfo *weaponInfo = CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType);
	if (m_bHasLockOnTarget && !m_pPointGunAt) {
		TheCamera.ClearPlayerWeaponMode();
		CWeaponEffects::ClearCrossHair();
		ClearPointGunAt();
	}

#ifdef VICEEXT_IMPROVED_TECH_PART
	if (padUsed->DuckJustDown() && !bIsDucking && m_nMoveState != PEDMOVE_SPRINT && weaponInfo->m_nWeaponSlot != WEAPONSLOT_HEAVY && GetWeapon()->m_eWeaponType != WEAPONTYPE_CHAINSAW) {
#else
	if (padUsed->DuckJustDown() && !bIsDucking && m_nMoveState != PEDMOVE_SPRINT) {
#endif

#ifdef VICEEXT_FIRING_AND_AIMING
		ClearPointGunAt();
#else
#ifdef FIX_BUGS
		// fix tommy being locked into looking at the same spot if you duck just after starting to shoot
		if(!m_pPointGunAt)
			ClearPointGunAt();
#endif
#endif
		bCrouchWhenShooting = true;
		SetDuck(60000, true);
	} else if (bIsDucking && (padUsed->DuckJustDown() || m_nMoveState == PEDMOVE_SPRINT ||
#ifdef VICEEXT_IMPROVED_TECH_PART
		((padUsed->GetSprint() || padUsed->ExitVehicleJustDown() || padUsed->JumpJustDown()) && 
		TheCamera.PlayerWeaponMode.Mode != CCam::MODE_SNIPER && TheCamera.PlayerWeaponMode.Mode != CCam::MODE_CAMERA))) {
#else
		padUsed->GetSprint() || padUsed->JumpJustDown() || padUsed->ExitVehicleJustDown())) {
#endif

#ifdef VICEEXT_FIRING_AND_AIMING
		ClearPointGunAt();
#else
#ifdef FIX_BUGS
		// same fix as above except for standing up
		if(!m_pPointGunAt)
			ClearPointGunAt();
#endif
#endif
		ClearDuck(true);
		bCrouchWhenShooting = false;
	}

	if(weaponInfo->IsFlagSet(WEAPONFLAG_CANAIM))
		m_wepAccuracy = 95;
	else
		m_wepAccuracy = 100;

#ifdef VICEEXT_FIRING_AND_AIMING
	if (InVehicle() && bIsPlayerAiming && (padUsed->CycleWeaponLeftJustDown() || padUsed->CycleWeaponRightJustDown())) {
		int currentWeaponSlot = GetWeaponSlot(GetWeapon()->m_eWeaponType);
		if (currentWeaponSlot == WEAPONSLOT_HANDGUN && HasWeaponSlot(5) && GetWeapon(5).m_nAmmoTotal > 0) {
			if (m_storedWeapon == WEAPONTYPE_UNIDENTIFIED)
				m_storedWeapon = GetWeapon()->m_eWeaponType;
			SetCurrentWeapon(GetWeapon(5).m_eWeaponType);
		}
#ifdef VICEEXT_WEAPON_BERETTA
		else if (currentWeaponSlot == WEAPONSLOT_SUBMACHINEGUN && HasWeaponSlot(3) && GetWeapon(3).m_nAmmoTotal > 0 &&
				(GetWeapon(3).m_eWeaponType == WEAPONTYPE_COLT45 || GetWeapon(3).m_eWeaponType == WEAPONTYPE_BERETTA)) {
#else
		else if (currentWeaponSlot == WEAPONSLOT_SUBMACHINEGUN && HasWeaponSlot(3) && GetWeapon(3).m_nAmmoTotal > 0 && GetWeapon(3).m_eWeaponType == WEAPONTYPE_COLT45) {
#endif
			if (m_storedWeapon == WEAPONTYPE_UNIDENTIFIED)
				m_storedWeapon = GetWeapon()->m_eWeaponType;
			SetCurrentWeapon(GetWeapon(3).m_eWeaponType);
		}
	}
#endif

	if (!m_pFire) {
		eWeaponType weapon = GetWeapon()->m_eWeaponType;
#if defined VICEEXT_FIRING_AND_AIMING && defined VICEEXT_FIRST_PERSON
		bool bSniperCamera = weapon == WEAPONTYPE_SNIPERRIFLE || weapon == WEAPONTYPE_LASERSCOPE || weapon == WEAPONTYPE_CAMERA;

		uint32 slot = CWeaponInfo::GetWeaponInfo(weapon)->m_nWeaponSlot;
		if ((slot == WEAPONSLOT_SHOTGUN || slot == WEAPONSLOT_HANDGUN ||
			slot == WEAPONSLOT_RIFLE || slot == WEAPONSLOT_SUBMACHINEGUN ||
			weapon == WEAPONTYPE_M60 || weapon == WEAPONTYPE_MINIGUN ||
			weapon == WEAPONTYPE_FLAMETHROWER || weapon == WEAPONTYPE_ROCKETLAUNCHER || bSniperCamera
#ifdef VICEEXT_WEAPON_GRENADE_LAUNCHER
			|| weapon == WEAPONTYPE_GRENADE_LAUNCHER
#endif
			) &&
			m_nMoveState != PEDMOVE_SPRINT) {

			bool isFrontObstacle = false;
			if (padUsed->GetTarget()) {
				CVector aimPos = GetRight() * 0.1f + GetForward() * 0.2f + GetPosition();
				aimPos += GetUp() * 0.35f;
				CEntity* obstacle = CWorld::TestSphereAgainstWorld(aimPos, 0.2f, nil, true, false, false, true, false, false);
				if (obstacle) {
					if (gaTempSphereColPoints[0].surfaceB != SURFACE_TRANSPARENT_CLOTH && gaTempSphereColPoints[0].surfaceB != SURFACE_METAL_CHAIN_FENCE &&
						gaTempSphereColPoints[0].surfaceB != SURFACE_WOOD_BENCH && gaTempSphereColPoints[0].surfaceB != SURFACE_SCAFFOLD_POLE) {

						isFrontObstacle = true;
					}
				}
			}

			bool cantFire = isFrontObstacle || m_fMoveSpeed != 0.0f && bIsDucking;

			if (cantFire) {
				ClearAimFlag();
				ClearWeaponTarget();
			}

			if (padUsed->GetWeapon() && bIsPlayerAiming && weapon != WEAPONTYPE_CAMERA && (weapon != WEAPONTYPE_LASERSCOPE || IsDoomMode())) {
				if (m_nPedState == PED_ATTACK) {
					m_fAttackButtonCounter *= Pow(0.94f, CTimer::GetTimeStep());
				}
				else {
					m_fAttackButtonCounter = 0.0f;
				}
				SetAttack(nil);
			}

			if ((padUsed->GetTarget() || IsDoomMode()) && bIsPlayerAiming && !cantFire && GetWeapon()->m_eWeaponState != WEAPONSTATE_RELOADING && !bIsAutoAiming) {
				if (!IsDoomMode()) {
#ifdef VICEEXT_ROCKET_LAUNCHER_THIRD_PERSON_AIM
					if (weapon == WEAPONTYPE_ROCKETLAUNCHER && !bRocketLauncherThirdPersonAiming)
						TheCamera.SetNewPlayerWeaponMode(CCam::MODE_ROCKETLAUNCHER, 0, 0);
					else
#endif
					if (weapon == WEAPONTYPE_SNIPERRIFLE || weapon == WEAPONTYPE_LASERSCOPE)
						TheCamera.SetNewPlayerWeaponMode(CCam::MODE_SNIPER, 0, 0);
					else if (weapon == WEAPONTYPE_CAMERA)
						TheCamera.SetNewPlayerWeaponMode(CCam::MODE_CAMERA, 0, 0);
				}

				bDontAllowWeaponChange = !IsDoomMode();

				SetAimFlag(m_fRotationCur);
				m_fFPSMoveHeading = TheCamera.Find3rdPersonQuickAimPitch();

				if (m_nPedState != PED_ATTACK && weapon != WEAPONTYPE_CAMERA) {
					SetPointGunAt(nil);
				}
			}

			if ((padUsed->GetTarget() || IsDoomMode()) && m_nPedState != PED_FIGHT) {
				bIsPlayerAiming = true;
			} else if ((!padUsed->GetTarget() || IsDoomMode()) && bIsPlayerAiming) {
				bIsPlayerAiming = false;

				if (GetWeapon()->m_eWeaponState != WEAPONSTATE_RELOADING)
					ClearWeaponTarget();

				StopAimingAnims();

				if (bIsDucking) {
					TheCamera.Cams[TheCamera.ActiveCam].m_fTargetCameraPosZ = 0.55f;
				} else if (weapon != WEAPONTYPE_COLT45 && weapon != WEAPONTYPE_TEC9 && weapon != WEAPONTYPE_SILENCED_INGRAM
#ifdef VICEEXT_WEAPON_UZIOLD
						  && weapon != WEAPONTYPE_UZIOLD
#endif
#ifdef VICEEXT_WEAPON_BERETTA
						  && weapon != WEAPONTYPE_BERETTA
#endif
						   ) {
					if (m_fMoveSpeed > 0.0f)
						CAnimManager::BlendAnimation(GetClump(), m_animGroup, ANIM_STD_RUN, 4.0f);
					else
						CAnimManager::BlendAnimation(GetClump(), m_animGroup, ANIM_STD_IDLE, 4.0f);
				}
			}

			if (weapon == WEAPONTYPE_CAMERA || (weapon == WEAPONTYPE_LASERSCOPE && bIsPlayerAiming))
				return;

			if (bIsPlayerAiming) {
				if (slot == WEAPONSLOT_RIFLE || weapon == WEAPONTYPE_M60)
					TheCamera.Cams[TheCamera.ActiveCam].AimingFOV = 40.0f;
				else
					TheCamera.Cams[TheCamera.ActiveCam].AimingFOV = 55.0f;
			}
		} else {
			bIsPlayerAiming = false;

			StopAimingAnims();
		}
#else
		if (weapon == WEAPONTYPE_ROCKETLAUNCHER || weapon == WEAPONTYPE_SNIPERRIFLE ||
			weapon == WEAPONTYPE_LASERSCOPE || weapon == WEAPONTYPE_M4 ||
			weapon == WEAPONTYPE_RUGER || weapon == WEAPONTYPE_M60 ||
			weapon == WEAPONTYPE_CAMERA) {

			if (padUsed->TargetJustDown() || TheCamera.m_bJustJumpedOutOf1stPersonBecauseOfTarget) {
#ifdef FREE_CAM
				if (CCamera::bFreeCam && TheCamera.Cams[0].Using3rdPersonMouseCam()) {
					m_fRotationCur = CGeneral::LimitRadianAngle(-TheCamera.Orientation);
					SetHeading(m_fRotationCur);
				}
#endif
				if (weapon == WEAPONTYPE_ROCKETLAUNCHER)
					TheCamera.SetNewPlayerWeaponMode(CCam::MODE_ROCKETLAUNCHER, 0, 0);
				else if (weapon == WEAPONTYPE_SNIPERRIFLE || weapon == WEAPONTYPE_LASERSCOPE)
					TheCamera.SetNewPlayerWeaponMode(CCam::MODE_SNIPER, 0, 0);
				else if (weapon == WEAPONTYPE_CAMERA)
					TheCamera.SetNewPlayerWeaponMode(CCam::MODE_CAMERA, 0, 0);
				else
					TheCamera.SetNewPlayerWeaponMode(CCam::MODE_M16_1STPERSON, 0, 0);

				m_fMoveSpeed = 0.0f;
				CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_IDLE, 1000.0f);
				SetPedState(PED_SNIPER_MODE);
				return;
			}
			if (!TheCamera.Using1stPersonWeaponMode())
				if (weapon == WEAPONTYPE_ROCKETLAUNCHER || weapon == WEAPONTYPE_SNIPERRIFLE || weapon == WEAPONTYPE_LASERSCOPE || weapon == WEAPONTYPE_CAMERA)
					return;
		}
#endif
	}

#ifdef VICEEXT_IMPROVED_MENU_AND_INPUT
	if (padUsed->GetMeleeWeapon() && weaponInfo->m_nWeaponSlot <= WEAPONSLOT_MELEE && m_nMoveState != PEDMOVE_SPRINT) {
		if (m_nSelectedWepSlot == m_currentWeapon) {
			if (m_nPedState == PED_ATTACK) {
				if (padUsed->MeleeWeaponJustDown()) {
					m_bHaveTargetSelected = true;
				} else if (!m_bHaveTargetSelected) {
					m_fAttackButtonCounter += CTimer::GetTimeStepNonClipped();
				}
			} else {
				m_fAttackButtonCounter = 0.0f;
				m_bHaveTargetSelected = false;
			}

			if (padUsed->MeleeWeaponJustDown()) {
				if ((GetWeapon()->m_eWeaponType == WEAPONTYPE_UNARMED || GetWeapon()->m_eWeaponType == WEAPONTYPE_BRASSKNUCKLE || weaponInfo->IsFlagSet(WEAPONFLAG_FIGHTMODE)) &&
					(m_fMoveSpeed < 1.0f || m_nPedState == PED_FIGHT)) {
					
					StartFightAttack(padUsed->GetWeapon());
				} else {
					SetAttack(nil);
				}
			}
		}
	} else if (padUsed->GetWeapon() && m_nMoveState != PEDMOVE_SPRINT) {
		if (m_nSelectedWepSlot == m_currentWeapon) {
			if (m_pPointGunAt && FrontEndMenuManager.m_PrefsAutoaim) {
				if (m_nPedState == PED_ATTACK) {
					m_fAttackButtonCounter *= Pow(0.94f, CTimer::GetTimeStep());
				} else {
					m_fAttackButtonCounter = 0.0f;
				}
				SetAttack(m_pPointGunAt);
			} else {
				if (m_nPedState == PED_ATTACK) {
					if (padUsed->WeaponJustDown()) {
						m_bHaveTargetSelected = true;
					} else if (!m_bHaveTargetSelected) {
						m_fAttackButtonCounter += CTimer::GetTimeStepNonClipped();
					}
				} else {
					m_fAttackButtonCounter = 0.0f;
					m_bHaveTargetSelected = false;
				}

				if (GetWeapon()->m_eWeaponType != WEAPONTYPE_UNARMED && GetWeapon()->m_eWeaponType != WEAPONTYPE_BRASSKNUCKLE &&
					!weaponInfo->IsFlagSet(WEAPONFLAG_FIGHTMODE)) {

					if (GetWeapon()->m_eWeaponType != WEAPONTYPE_DETONATOR && GetWeapon()->m_eWeaponType != WEAPONTYPE_DETONATOR_GRENADE ||
						padUsed->WeaponJustDown())

						SetAttack(nil);
				}
			}
		}
	} else {
		m_pedIK.m_flags &= ~CPedIK::LOOKAROUND_HEAD_ONLY;
		if (m_nPedState == PED_ATTACK) {
			m_bHaveTargetSelected = true;
			bIsAttacking = false;
		}
	}

	if (CPad::GetPad(0)->WeaponReloadJustDown() && weaponInfo->IsFlagSet(WEAPONFLAG_RELOAD)) {
		CWeapon* weapon = GetWeapon();
		if (weapon->m_eWeaponState == WEAPONSTATE_READY && weapon->m_nAmmoInClip > 0 && 
			weapon->m_nAmmoInClip < weaponInfo->m_nAmountofAmmunition && weapon->m_nAmmoTotal != weapon->m_nAmmoInClip) {

			weapon->m_eWeaponState = WEAPONSTATE_RELOADING;
			weapon->m_nTimer = CTimer::GetTimeInMilliseconds() + weapon->GetInfo()->m_nReload;

			CAnimBlendAssociation* newReloadAssoc = CAnimManager::BlendAnimation(
				GetClump(), weaponInfo->m_AnimToPlay,
				bIsDucking && GetCrouchReloadAnim(weaponInfo) ? GetCrouchReloadAnim(weaponInfo) : GetReloadAnim(weaponInfo),
				8.0f);
			newReloadAssoc->SetFinishCallback(FinishedReloadCB, this);

			ClearLookFlag();
			ClearAimFlag();
			bIsAttacking = false;
			bIsPointingGunAt = false;
			m_shootTimer = CTimer::GetTimeInMilliseconds();
		}
	}
#else
	if (padUsed->GetWeapon() && m_nMoveState != PEDMOVE_SPRINT) {
		if (m_nSelectedWepSlot == m_currentWeapon) {
			if (m_pPointGunAt) {
				if (m_nPedState == PED_ATTACK) {
					m_fAttackButtonCounter *= Pow(0.94f, CTimer::GetTimeStep());
				} else {
					m_fAttackButtonCounter = 0.0f;
				}
				SetAttack(m_pPointGunAt);
			} else {
				if (m_nPedState == PED_ATTACK) {
					if (padUsed->WeaponJustDown()) {
						m_bHaveTargetSelected = true;
					} else if (!m_bHaveTargetSelected) {
						m_fAttackButtonCounter += CTimer::GetTimeStepNonClipped();
					}
				} else {
					m_fAttackButtonCounter = 0.0f;
					m_bHaveTargetSelected = false;
				}
				if (GetWeapon()->m_eWeaponType != WEAPONTYPE_UNARMED && GetWeapon()->m_eWeaponType != WEAPONTYPE_BRASSKNUCKLE &&
					!weaponInfo->IsFlagSet(WEAPONFLAG_FIGHTMODE)) {

					if (GetWeapon()->m_eWeaponType != WEAPONTYPE_DETONATOR && GetWeapon()->m_eWeaponType != WEAPONTYPE_DETONATOR_GRENADE ||
						padUsed->WeaponJustDown())
						SetAttack(nil);

				} else if (padUsed->WeaponJustDown()) {
					if (m_fMoveSpeed < 1.0f || m_nPedState == PED_FIGHT)
						StartFightAttack(padUsed->GetWeapon());
					else
						SetAttack(nil);
				}
			}
		}
	} else {
		m_pedIK.m_flags &= ~CPedIK::LOOKAROUND_HEAD_ONLY;
		if (m_nPedState == PED_ATTACK) {
			m_bHaveTargetSelected = true;
			bIsAttacking = false;
		}
	}
#endif

#ifdef FREE_CAM
	static int8 changedHeadingRate = 0;
	static int8 pointedGun = 0;
	if (changedHeadingRate == 2) changedHeadingRate = 1;
	if (pointedGun == 2) pointedGun = 1;

	// Rotate player/arm when shooting. We don't have auto-rotation anymore
#ifdef VICEEXT_IMPROVED_MENU_AND_INPUT
	if (CCamera::bFreeCam &&
#else
	if (CCamera::m_bUseMouse3rdPerson && CCamera::bFreeCam &&
#endif
#ifdef VICEEXT_FIRING_AND_AIMING
		(m_nSelectedWepSlot == m_currentWeapon || InVehicle()) && m_nMoveState != PEDMOVE_SPRINT) {
#else
		m_nSelectedWepSlot == m_currentWeapon && m_nMoveState != PEDMOVE_SPRINT) {
#endif

#ifdef VICEEXT_FIRING_AND_AIMING
#define CAN_AIM_WITH_ARM ((weaponInfo->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM) || InVehicle()) && !bIsDucking && !bCrouchWhenShooting)
#else
#define CAN_AIM_WITH_ARM (weaponInfo->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM) && !bIsDucking && !bCrouchWhenShooting)
#endif
		// Weapons except throwable and melee ones
		if (weaponInfo->m_nWeaponSlot > 2) {
#ifdef VICEEXT_FIRING_AND_AIMING
			if ((padUsed->GetTarget() && CAN_AIM_WITH_ARM) || padUsed->GetWeapon() || (m_nPedState == PED_ATTACK && weaponInfo->m_nWeaponSlot > WEAPONSLOT_PROJECTILE)) {
#else
			if ((padUsed->GetTarget() && CAN_AIM_WITH_ARM) || padUsed->GetWeapon()) {
#endif
				float limitedCam = CGeneral::LimitRadianAngle(-TheCamera.Orientation);

				m_cachedCamSource = TheCamera.Cams[TheCamera.ActiveCam].Source;
				m_cachedCamFront = TheCamera.Cams[TheCamera.ActiveCam].Front;
				m_cachedCamUp = TheCamera.Cams[TheCamera.ActiveCam].Up;
				
				// On this one we can rotate arm.
				if (CAN_AIM_WITH_ARM) {
					pointedGun = 2;
					m_bFreeAimActive = true;
					SetLookFlag(limitedCam, true, true);
					SetAimFlag(limitedCam);
					SetLookTimer(INT32_MAX);
					((CPlayerPed*)this)->m_fFPSMoveHeading = TheCamera.Find3rdPersonQuickAimPitch();
					if (m_nPedState != PED_ATTACK && m_nPedState != PED_AIM_GUN) {
						// This is a seperate ped state just for pointing gun. Used for target button
						SetPointGunAt(nil);
					}
				} else {
					m_fRotationDest = limitedCam;
					changedHeadingRate = 2;
					m_headingRate = 12.5f;

					// Anim. fix for shotgun, ak47 and m16 (we must finish rot. it quickly)
					if (weaponInfo->IsFlagSet(WEAPONFLAG_CANAIM) && padUsed->WeaponJustDown()) {
						m_fRotationCur = CGeneral::LimitRadianAngle(m_fRotationCur);
						float limitedRotDest = m_fRotationDest;

						if (m_fRotationCur - PI > m_fRotationDest) {
							limitedRotDest += 2 * PI;
						} else if (PI + m_fRotationCur < m_fRotationDest) {
							limitedRotDest -= 2 * PI;
						}

						m_fRotationCur += (limitedRotDest - m_fRotationCur) / 2;
					}
				}
			}
		}
#undef CAN_AIM_WITH_ARM
	}
	if (changedHeadingRate == 1) {
		changedHeadingRate = 0;
		RestoreHeadingRate();
	}
	if (pointedGun == 1) {
		if (m_nPedState == PED_ATTACK) {
			if (!padUsed->GetWeapon() && (m_pedIK.m_flags & CPedIK::GUN_POINTED_SUCCESSFULLY) == 0) {
				float limitedCam = CGeneral::LimitRadianAngle(-TheCamera.Orientation);

				SetAimFlag(limitedCam);
				((CPlayerPed*)this)->m_fFPSMoveHeading = TheCamera.Find3rdPersonQuickAimPitch();
				m_bFreeAimActive = true;
			}
		} else {
			pointedGun = 0;
			ClearPointGunAt();
		}
	}
#endif

#if defined VICEEXT_IMPROVED_MENU_AND_INPUT && defined VICEEXT_FIRING_AND_AIMING
	if (!FrontEndMenuManager.m_PrefsAutoaim)
		return;
	else {
		if (CPad::GetPad(0)->GetTarget()) {
			bool bCantAim = CPad::GetPad(0)->NewState.RightShoulder1 == 0 && CPad::GetPad(0)->Mode == 2 ||
							CPad::GetPad(0)->NewState.LeftShoulder2 < 128 && CPad::GetPad(0)->Mode == 1;

			if (bCantAim && !bIsAutoAiming || GetWeaponSlot(GetWeapon()->m_eWeaponType) == WEAPONSLOT_SNIPER || InVehicle())
				return;
		}
	}
#endif

	if (padUsed->GetTarget() && m_nSelectedWepSlot == m_currentWeapon && m_nMoveState != PEDMOVE_SPRINT && !TheCamera.Using1stPersonWeaponMode() && weaponInfo->IsFlagSet(WEAPONFLAG_CANAIM)) {
		if (m_pPointGunAt) {
			// what??
			if (!m_pPointGunAt
#ifdef FREE_CAM
				|| (!CCamera::bFreeCam && CCamera::m_bUseMouse3rdPerson)
#else
				|| CCamera::m_bUseMouse3rdPerson
#endif		
			) {
				ClearWeaponTarget();
				return;
			}

			if (m_pPointGunAt->IsPed() && (
#ifndef AIMING_VEHICLE_OCCUPANTS
				(((CPed*)m_pPointGunAt)->bInVehicle && (!((CPed*)m_pPointGunAt)->m_pMyVehicle || !((CPed*)m_pPointGunAt)->m_pMyVehicle->IsBike())) ||
#endif
				!CGame::nastyGame && ((CPed*)m_pPointGunAt)->DyingOrDead())) {
				ClearWeaponTarget();
				return;
			}
			if (CPlayerPed::DoesTargetHaveToBeBroken(m_pPointGunAt->GetPosition(), GetWeapon()) || 
				(!bCanPointGunAtTarget && !weaponInfo->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM))) { // this line isn't on Mobile, idk why
				ClearWeaponTarget();
				return;
			}

			if (m_pPointGunAt) {
				RotatePlayerToTrackTarget();
			}

			if (m_pPointGunAt) {
				if (padUsed->ShiftTargetLeftJustDown())
					FindNextWeaponLockOnTarget(m_pPointGunAt, true);
				if (padUsed->ShiftTargetRightJustDown())
					FindNextWeaponLockOnTarget(m_pPointGunAt, false);
			}

#if defined VICEEXT_IMPROVED_MENU_AND_INPUT && defined VICEEXT_FIRING_AND_AIMING && defined VICEEXT_FIRST_PERSON
			CCam& camera = TheCamera.Cams[0];

			CVector distForBeta = m_pPointGunAt->GetPosition() - GetPosition();
			if (camera.Mode == CCam::MODE_REAL_1ST_PERSON)
				camera.Beta = distForBeta.Heading() - DEGTORAD(90.0f);
			else
				camera.Beta = distForBeta.Heading() + DEGTORAD(92.5f);

			CVector dist = GetPosition() - m_pPointGunAt->GetPosition();
			float groundDist = dist.Magnitude2D();
			camera.Alpha = CGeneral::GetATanOfXY(groundDist, -dist.z);
			while (camera.Alpha >= PI) camera.Alpha -= 2.0f * PI;
			while (camera.Alpha < -PI) camera.Alpha += 2.0f * PI;
#else
			TheCamera.SetNewPlayerWeaponMode(CCam::MODE_SYPHON, 0, 0);
			TheCamera.UpdateAimingCoors(m_pPointGunAt->GetPosition());
#endif

		} else if (!CCamera::m_bUseMouse3rdPerson) {
#ifdef VICEEXT_IMPROVED_MENU_AND_INPUT
			if (!bIsAutoAiming || TheCamera.m_bJustJumpedOutOf1stPersonBecauseOfTarget)
#else
			if (padUsed->TargetJustDown() || TheCamera.m_bJustJumpedOutOf1stPersonBecauseOfTarget)
#endif
				FindWeaponLockOnTarget();
		}
	} else if (m_pPointGunAt) {
		ClearWeaponTarget();
	}

	if (m_pPointGunAt) {
		CVector markPos;
		if (m_pPointGunAt->IsPed()) {
			((CPed*)m_pPointGunAt)->m_pedIK.GetComponentPosition(markPos, PED_MID);
		} else {
			markPos = m_pPointGunAt->GetPosition();
		}
		if (bCanPointGunAtTarget) {
			CWeaponEffects::MarkTarget(markPos, 64, 0, 0, 255, 0.8f);
		} else {
			CWeaponEffects::MarkTarget(markPos, 64, 32, 0, 255, 0.8f);
		}
	}
	m_bHasLockOnTarget = m_pPointGunAt != nil;
}

bool
CPlayerPed::MovementDisabledBecauseOfTargeting(void)
{
	return m_pPointGunAt && !CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM);
}

void
CPlayerPed::PlayerControlZelda(CPad *padUsed)
{
	float smoothSprayRate = DoWeaponSmoothSpray();
	float camOrientation = TheCamera.Orientation;
	float leftRight = padUsed->GetPedWalkLeftRight();
	float upDown = padUsed->GetPedWalkUpDown();
	float padMoveInGameUnit;
	bool smoothSprayWithoutMove = false;

#ifndef VICEEXT_FIRING_AND_AIMING
	if (MovementDisabledBecauseOfTargeting()) {
		upDown = 0.0f;
		leftRight = 0.0f;
	}
#endif

	if (smoothSprayRate > 0.0f && upDown > 0.0f) {
		padMoveInGameUnit = 0.0f;
		smoothSprayWithoutMove = true;
	} else {
		padMoveInGameUnit = CVector2D(leftRight, upDown).Magnitude() / PAD_MOVE_TO_GAME_WORLD_MOVE;
	}

#ifdef FREE_CAM
	if (TheCamera.Cams[0].Using3rdPersonMouseCam() && smoothSprayRate > 0.0f) {
		padMoveInGameUnit = 0.0f;
		smoothSprayWithoutMove = false;
	}
#endif

	if (padMoveInGameUnit > 0.0f || smoothSprayWithoutMove) {
		float padHeading = CGeneral::GetRadianAngleBetweenPoints(0.0f, 0.0f, -leftRight, upDown);
		float neededTurn = CGeneral::LimitRadianAngle(padHeading - camOrientation);
		if (smoothSprayRate > 0.0f) {
			m_fRotationDest = m_fRotationCur - leftRight / 128.0f * smoothSprayRate * CTimer::GetTimeStep();
		} else {
			m_fRotationDest = neededTurn;
		}


#ifdef VICEEXT_IMPROVED_MENU_AND_INPUT
#ifdef VICEEXT_FIRING_AND_AIMING
		if (CPad::GetPad(0)->GetPedWalk() && !bIsPlayerAiming)
#else
		if (CPad::GetPad(0)->GetPedWalk())
#endif
			m_fMoveSpeed /= 2;
#endif
		float maxAcc = 0.07f * CTimer::GetTimeStep();
		m_fMoveSpeed = Min(padMoveInGameUnit, m_fMoveSpeed + maxAcc);

	} else {
		m_fMoveSpeed = 0.0f;
	}

	if (m_nPedState == PED_JUMP) {
		if (bIsInTheAir) {
			if (bUsesCollision && !bHitSteepSlope && (!bHitSomethingLastFrame || m_vecDamageNormal.z > 0.6f)
				&& m_fDistanceTravelled < CTimer::GetTimeStepInSeconds() && m_vecMoveSpeed.MagnitudeSqr() < 0.01f) {

				float angleSin = Sin(m_fRotationCur); // originally sin(DEGTORAD(RADTODEG(m_fRotationCur))) o_O
				float angleCos = Cos(m_fRotationCur);
				ApplyMoveForce(-angleSin * 3.0f, 3.0f * angleCos, 0.05f);
			}
		} else if (bIsLanding) {
			m_fMoveSpeed = 0.0f;
		}
	}

	if (m_nPedState == PED_ANSWER_MOBILE) {
		SetRealMoveAnim();
		return;
	}

	if (!CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->IsFlagSet(WEAPONFLAG_HEAVY) && padUsed->GetSprint()) {
#if defined VICEEXT_CROUCH && defined VICEEXT_FIRING_AND_AIMING
		if ((!m_pCurrentPhysSurface || (!m_pCurrentPhysSurface->bInfiniteMass || m_pCurrentPhysSurface->m_phy_flagA08)) && !bIsPlayerAiming && !RpAnimBlendClumpGetAssociation(GetClump(), ANIM_WEAPON_FIRE) &&
			!bIsDucking && !RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_CROUCH_IDLE) && !RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_CROUCH_FORWARD))
			m_nMoveState = PEDMOVE_SPRINT;
		else if (bIsDucking && !bIsPlayerAiming) {
			ClearDuck();
			bCrouchWhenShooting = false;
		}
#else
		if (!m_pCurrentPhysSurface || (!m_pCurrentPhysSurface->bInfiniteMass || m_pCurrentPhysSurface->m_phy_flagA08))
			m_nMoveState = PEDMOVE_SPRINT;
#endif
	}

	if (m_nPedState != PED_FIGHT)
		SetRealMoveAnim();

#ifdef VICEEXT_FIRING_AND_AIMING
	if (bIsPlayerAiming)
		return;
#endif

	if (!bIsInTheAir && !CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->IsFlagSet(WEAPONFLAG_HEAVY)
		&& padUsed->JumpJustDown() && m_nPedState != PED_JUMP) {
		ClearAttack();
		ClearWeaponTarget();
#ifdef VICEEXT_CROUCH
		if (bIsDucking) {
			ClearDuck();

			return;
		}
#endif

#ifdef VICEEXT_FEATURES_INI
		if (!bPlayerDoesntBounceAwayFromMovingCar && m_nEvadeAmount != 0 && m_pEvadingFrom) {
#else
		if (m_nEvadeAmount != 0 && m_pEvadingFrom) {
#endif
			SetEvasiveDive((CPhysical*)m_pEvadingFrom, 1);
			m_nEvadeAmount = 0;
			m_pEvadingFrom = nil;
		} else {
			SetJump();
		}
	}
	PlayIdleAnimations(padUsed);
}

// Finds nice positions for peds to duck and shoot player. And it's inside PlayerPed, this is treachery!
void
CPlayerPed::FindNewAttackPoints(void)
{
	for (int i=0; i<ARRAY_SIZE(m_pPedAtSafePos); i++) {
		CPed *safeNeighbour = m_pPedAtSafePos[i];
		if (safeNeighbour) {
			if (safeNeighbour->m_nPedState == PED_DEAD || safeNeighbour->m_pedInObjective != this) {
				m_vecSafePos[i].x = 0.0f;
				m_vecSafePos[i].y = 0.0f;
				m_vecSafePos[i].z = 0.0f;
				m_pPedAtSafePos[i] = nil;
			}
		} else {
			m_vecSafePos[i].x = 0.0f;
			m_vecSafePos[i].y = 0.0f;
			m_vecSafePos[i].z = 0.0f;
		}
	}
	CEntity *entities[6];
	int16 numEnts;
	float rightMult, fwdMult;
	CWorld::FindObjectsInRange(GetPosition(), 18.0f, true, &numEnts, 6, entities, true, false, false, true, false);
	for (int i = 0; i < numEnts; ++i) {
		CEntity *ent = entities[i];
		int16 mi = ent->GetModelIndex();
		if (!ent->IsObject() || ((CObject*)ent)->m_nSpecialCollisionResponseCases == COLLRESPONSE_FENCEPART)
			if (!IsTreeModel(mi))
				continue;

		if (mi == MI_TRAFFICLIGHTS) {
			rightMult = 2.957f;
			fwdMult = 0.147f;

		} else if (mi == MI_SINGLESTREETLIGHTS1) {
			rightMult = 0.744f;
			fwdMult = 0.0f;

		} else if (mi == MI_SINGLESTREETLIGHTS2) {
			rightMult = 0.043f;
			fwdMult = 0.0f;

		} else if (mi == MI_SINGLESTREETLIGHTS3) {
			rightMult = 1.143f;
			fwdMult = 0.145f;

		} else if (mi == MI_DOUBLESTREETLIGHTS) {
			rightMult = 0.744f;
			fwdMult = 0.0f;

		} else if (mi == MI_LAMPPOST1) {
			rightMult = 0.744f;
			fwdMult = 0.0f;

		} else if (mi == MI_TRAFFICLIGHT01) {
			rightMult = 2.957f;
			fwdMult = 0.147f;

		} else if (mi == MI_LITTLEHA_POLICE) {
			rightMult = 0.0f;
			fwdMult = 0.0f;

		} else if (mi == MI_PARKBENCH) {
			rightMult = 0.0f;
			fwdMult = 0.0f;

		} else if (IsTreeModel(mi)) {
			rightMult = 0.0f;
			fwdMult = 0.0f;
		} else
			continue;

		CVector entAttackPoint(rightMult * ent->GetRight().x + fwdMult * ent->GetForward().x + ent->GetPosition().x,
			rightMult * ent->GetRight().y + fwdMult * ent->GetForward().y + ent->GetPosition().y,
			ent->GetPosition().z);
		CVector attackerPos = GetPosition() - entAttackPoint; // for now it's dist, not attackerPos
		CVector dirTowardsUs = attackerPos;
		dirTowardsUs.Normalise();
		dirTowardsUs *= 2.0f;
		attackerPos = entAttackPoint - dirTowardsUs; // to make cop farther from us
		CPedPlacement::FindZCoorForPed(&attackerPos);
		if (CPedPlacement::IsPositionClearForPed(attackerPos))
			m_vecSafePos[i] = attackerPos;
	}
}

#ifdef VICEEXT_SWIMMING
#define VICEEXT_SWIM_SEREGA_ENTRY	0.022f
#define VICEEXT_SWIM_SEREGA_CRUISE	0.025f
#define VICEEXT_SWIM_SEREGA_SWIM	0.05f
#define VICEEXT_SWIM_SEREGA_SPRINT	0.12f
#define VICEEXT_SWIM_SEREGA_TIMEOUT	300
#define VICEEXT_SWIM_STROKE_SLACK	0.65f
#define VICEEXT_SWIM_CRAWL_ROOT_M	2.505f
#define VICEEXT_SWIM_BREAST_ROOT_M	3.011f
#define VICEEXT_SWIM_CHEST_M	1.45f
#define VICEEXT_SWIM_HIP_M	1.10f
static float __attribute__((noinline))
ViceExtSwimClipSpeed(AnimationId odAnim, CAnimBlendAssociation *odAssoc)
{
	float odRootM = odAnim == ANIM_STD_SWIM_CRAWL ? VICEEXT_SWIM_CRAWL_ROOT_M
	    : odAnim == ANIM_STD_SWIM_BREAST ? VICEEXT_SWIM_BREAST_ROOT_M : 0.0f;
	if (odRootM <= 0.0f || odAssoc == nil || odAssoc->hierarchy == nil
	    || odAssoc->hierarchy->totalLength <= 0.01f)
		return 0.0f;
#ifdef __EMSCRIPTEN__
	{
		static float odNatVisto[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		int odIdx = odAnim == ANIM_STD_SWIM_CRAWL ? 0 : odAnim == ANIM_STD_SWIM_BREAST ? 1 : 2;
		float odLen = odAssoc->hierarchy->totalLength;
		if (odLen != odNatVisto[odIdx]) {
			odNatVisto[odIdx] = odLen;
			char odT[192];
			snprintf(odT, sizeof odT, "SWIMNAT clip=%d totalLength=%.3f root=%.3f natural=%.2f m/s",
				(int)odAnim, odLen, odRootM, odRootM / odLen);
			ODTRACES(odT);
		}
	}
#endif
	return odRootM / odAssoc->hierarchy->totalLength;
}

static bool odSwimActive = false;

static void
ViceExtSwimWake(CPed *ped, float medido, CRGBA const &color)
{
	static float s_odUltima = -1.0f;
	if (!ped || medido < 0.4f) {
		s_odUltima = -1.0f;
		return;
	}
	CVector yo = ped->GetPosition();
	CVector pos = TheCamera.GetPosition();
	if (CVector2D(pos.x - yo.x, pos.y - yo.y).Magnitude() > 70.0f * 4.0f)
		return;
	float size = Min(0.75f, medido * 0.14f);
	if (size < 0.08f)
		return;
	if (s_odUltima >= 0.0f && medido - s_odUltima < 0.35f)
		return;
	s_odUltima = medido;
	CVector right = ped->GetRight();
	CVector dir = -0.35f * ped->m_vecMoveSpeed;
	dir.z += 0.25f * size;
	for (int lado = -1; lado <= 1; lado += 2) {
		CVector p = yo + right * (0.45f * lado);
		float nivel = 0.0f;
		if (!CWaterLevel::GetWaterLevel(p, &nivel, false))
			continue;
		if (p.z - nivel > 3.0f)
			continue;
		p.z = nivel + 0.1f;
		CParticle::AddParticle(PARTICLE_CAR_SPLASH, p, 0.75f * dir, nil, size + 0.1f,
			color, (int)CGeneral::GetRandomNumberInRange(0.0f, 10.0f),
			(int)CGeneral::GetRandomNumberInRange(0.0f, 90.0f), 1, 300);
		CParticle::AddParticle(PARTICLE_BOAT_SPLASH, p, dir, nil, size,
			color, (int)CGeneral::GetRandomNumberInRange(0.0f, 0.4f),
			(int)CGeneral::GetRandomNumberInRange(0.0f, 45.0f), 0, 300);
	}
}

static bool s_odSwimVisualHidden = false;
static CVector s_odSwimLastPos(0.0f, 0.0f, 0.0f);
static bool s_odSwimLastPosOk = false;
static float s_odSwimMedido = 0.0f;
static float s_odSwimPrevTime = 0.0f;
static float s_odPrevFase = 0.0f;
static float s_odEstela = 0.0f;
static bool s_odNadoBrazo = false;
static uint32 s_odFovNext = 0;
static bool s_odFovPrev = false;
static int s_odFovPaso = 0;
static void
ViceExtSwimWeaponFists(CPlayerPed *ped, bool take)
{
	if (take) {
		if (!s_odSwimVisualHidden) {
			s_odSwimVisualHidden = true;
			s_odSwimSlot = ped->m_nSelectedWepSlot;
			ped->RemoveWeaponAnims(ped->m_currentWeapon, -1000.0f);
			ped->RemoveWeaponModel(CWeaponInfo::GetWeaponInfo(ped->GetWeapon()->m_eWeaponType)->m_nModelId);
#ifdef __EMSCRIPTEN__
			ODTRACES("VICEEXT swim punos");
#endif
		}
	} else if (s_odSwimVisualHidden) {
		s_odSwimVisualHidden = false;
		ped->m_nSelectedWepSlot = s_odSwimSlot;
		ped->MakeChangesForNewWeapon(ped->m_nSelectedWepSlot);
#ifdef __EMSCRIPTEN__
			ODTRACES("VICEEXT swim arma-devuelta");
#endif
	}
}

bool
CPlayerPed::ViceExtSwimControl(CPad *padUsed)
{
	static int s_odSwimState = 0;
	static int s_odSwimTicks = 0;
	static const AnimationId odFallAnims[7] = {
		ANIM_STD_JUMP_LAUNCH, ANIM_STD_JUMP_GLIDE, ANIM_STD_JUMP_LAND,
		ANIM_STD_FALL, ANIM_STD_FALL_GLIDE, ANIM_STD_FALL_LAND,
		ANIM_STD_FALL_COLLAPSE
	};
	static const AnimationId odSwimAnims[4] = {
		ANIM_STD_SWIM_TREAD, ANIM_STD_SWIM_CRAWL, ANIM_STD_SWIM_BREAST,
		ANIM_STD_SWIM_JUMPOUT
	};
	int odAire = bIsInTheAir;
	float odLevelW = 0.0f;
	bool odHasWater = CWaterLevel::GetWaterLevel(GetPosition(), &odLevelW, true);
	float odDepth = odHasWater ? (odLevelW - GetPosition().z) : -1.0f;
	bool odGuard = padUsed && !bInVehicle && !DyingOrDead()
	    && !CReplay::IsPlayingBack()
	    && (s_odSwimState != 0
	        ? (odHasWater && odDepth > (VICEEXT_SWIM_HIP_M - FEET_OFFSET))
	        : (odHasWater && bIsInWater && odDepth > (VICEEXT_SWIM_CHEST_M - FEET_OFFSET)));

	int16 odLR = padUsed ? padUsed->GetPedWalkLeftRight() : 0;
	int16 odUD = padUsed ? padUsed->GetPedWalkUpDown() : 0;
	float leftRight = (float)odLR;
	float upDown = (float)odUD;
	float padMove = CVector2D(leftRight, upDown).Magnitude();
	bool odSprintBtn = padUsed && padUsed->GetSprint();

	if (!odGuard) {
		if (s_odSwimState != 0) {
			ViceExtSwimWeaponFists(this, false);
			ViceExtPedRelease(PEDLANE_NADO, PEDCAP_MOVIMIENTO);
			ViceExtPedRelease(PEDLANE_NADO, PEDCAP_POSTURA);
			ViceExtPedRelease(PEDLANE_NADO, PEDCAP_APUNTAR);
			ViceExtPedRelease(PEDLANE_NADO, PEDCAP_ARMA);
			s_odSwimState = 0;
			s_odSwimTicks = 0;
			m_fMoveSpeed = 0.0f;
			m_vecMoveSpeed.x = 0.0f;
			m_vecMoveSpeed.y = 0.0f;
			odSwimActive = false;
			for (int odS = 0; odS < 4; odS++) {
				CAnimBlendAssociation *odSA = RpAnimBlendClumpGetAssociation(GetClump(), odSwimAnims[odS]);
				if (odSA && odSA->blendDelta >= 0.0f) {
					odSA->flags |= ASSOC_DELETEFADEDOUT;
					odSA->blendDelta = -4.0f;
				}
			}
#ifdef __EMSCRIPTEN__
			{
				char t[96];
				const char *motivo = (odHasWater && odDepth <= 0.45f) ? "somero" : (!bIsInWater ? "sin-contacto" : "sin-control");
				snprintf(t, sizeof t, "VICEEXT swim exit motivo=%s", motivo);
				ODTRACES(t);
				snprintf(t, sizeof t, "SWIM2 exit motivo=%s clip=%d", motivo, (int)ANIM_STD_SWIM_JUMPOUT);
				ODTRACES(t);
			}
#endif
		}
#ifdef __EMSCRIPTEN__
		if (bIsInWater) {
			static uint32 s_odIdleNext = 0;
			uint32 odIdleNow = CTimer::GetTimeInMilliseconds();
			if (odIdleNow >= s_odIdleNext || odIdleNow + 60000 < s_odIdleNext) {
				s_odIdleNext = odIdleNow + 1000;
				char t[96];
				snprintf(t, sizeof t, "SWIMIDLE contacto=1 estado=%d z=%.1f",
					(int)m_nPedState, GetPosition().z);
				ODTRACES(t);
			}
		}
#endif
		return false;
	}

	bIsStanding = false;
	if (bIsDucking) {
		bIsDucking = false;
		bCrouchWhenShooting = false;
	}

	if (s_odSwimState == 0) {
#ifdef VICEEXT_CLIMBING
		bIsReadyToClimbing = false;
#endif
		s_odSwimState = 1;
		s_odSwimTicks = 0;
		s_odSwimLastPosOk = false;
		s_odSwimMedido = 0.0f;
		s_odSwimPrevTime = 0.0f;
		s_odPrevFase = 0.0f;
		s_odEstela = 0.0f;
		s_odFovNext = 0.0f;
		s_odFovPrev = false;
		s_odFovPaso = 0;
		odSwimActive = true;
		ViceExtPedClaim(PEDLANE_NADO, PEDCAP_MOVIMIENTO);
		ViceExtPedClaim(PEDLANE_NADO, PEDCAP_POSTURA);
		ViceExtPedClaim(PEDLANE_NADO, PEDCAP_APUNTAR);
		ViceExtPedClaim(PEDLANE_NADO, PEDCAP_ARMA);
		{
			for (int odF = 0; odF < 7; odF++) {
				CAnimBlendAssociation *odFA = RpAnimBlendClumpGetAssociation(GetClump(), odFallAnims[odF]);
				if (odFA && odFA->blendDelta >= 0.0f) {
					odFA->flags |= ASSOC_DELETEFADEDOUT;
					odFA->blendDelta = -4.0f;
				}
			}
		}
		{
			float odEntrada = m_vecMoveSpeed.Magnitude();
			float odVol = Min(1.0f, 0.35f + odEntrada * 0.25f);
			CVector odPos = GetPosition();
			odPos.z -= FEET_OFFSET * 0.5f;
			CRGBA odColor;
			odColor.red = (uint8)((0.5f * CTimeCycle::GetDirectionalRed() + CTimeCycle::GetAmbientRed()) * 127.5f);
			odColor.green = (uint8)((0.5f * CTimeCycle::GetDirectionalGreen() + CTimeCycle::GetAmbientGreen()) * 127.5f);
			odColor.blue = (uint8)((0.5f * CTimeCycle::GetDirectionalBlue() + CTimeCycle::GetAmbientBlue()) * 127.5f);
			odColor.alpha = (uint8)CGeneral::GetRandomNumberInRange(48, 96);
			DMAudio.PlayOneShot(m_audioEntityId, SOUND_SPLASH, odVol);
			int odN = 60 + (int)Min(140.0f, odEntrada * 30.0f);
			CParticleObject::AddObject(POBJECT_PED_WATER_SPLASH, odPos, CVector(0.0f, 0.0f, 0.1f),
				0.0f, odN, odColor, true);
			for (int odR = 0; odR < 4; odR++) {
				CVector odP = odPos;
				odP.x += CGeneral::GetRandomNumberInRange(-0.75f, 0.75f);
				odP.y += CGeneral::GetRandomNumberInRange(-0.75f, 0.75f);
				CParticle::AddParticle(PARTICLE_RAIN_SPLASH_BIGGROW, odP,
					CVector(0.0f, 0.0f, 0.0f), nil, 0.0f, odColor, 0, 0, 0, 0);
			}
		}
#ifdef __EMSCRIPTEN__
		{
			char t[64];
			snprintf(t, sizeof t, "VICEEXT swim enter z=%.1f", GetPosition().z);
			ODTRACES(t);
		}
#endif
	}
	bIsInTheAir = false;
	ViceExtSwimWeaponFists(this, true);

	float odLevel;
	AnimationId odSwimAnim;
	if (padMove > 0.0f) {
		s_odSwimTicks = 0;
		if (odSprintBtn) {
			s_odSwimState = 3;
			odLevel = VICEEXT_SWIM_SEREGA_SPRINT;
			odSwimAnim = ANIM_STD_SWIM_CRAWL;
		} else {
			s_odSwimState = 2;
			odLevel = VICEEXT_SWIM_SEREGA_SWIM;
			odSwimAnim = ANIM_STD_SWIM_BREAST;
		}
		float odPadHead = CGeneral::GetRadianAngleBetweenPoints(0.0f, 0.0f, -leftRight, upDown);
		float odDest = CGeneral::LimitRadianAngle(odPadHead - TheCamera.Orientation);
		m_fRotationCur = odDest;
		m_fRotationDest = odDest;
		m_vecMoveSpeed.x = -Sin(odDest) * odLevel;
		m_vecMoveSpeed.y = Cos(odDest) * odLevel;
	} else {
		if (s_odSwimTicks <= VICEEXT_SWIM_SEREGA_TIMEOUT) ++s_odSwimTicks;
		s_odSwimState = 1;
		odLevel = (s_odSwimTicks > VICEEXT_SWIM_SEREGA_TIMEOUT)
		    ? VICEEXT_SWIM_SEREGA_CRUISE : VICEEXT_SWIM_SEREGA_ENTRY;
		odSwimAnim = ANIM_STD_SWIM_TREAD;
		m_vecMoveSpeed.x = 0.0f;
		m_vecMoveSpeed.y = 0.0f;
	}
	float odTargetZ = odLevelW - (VICEEXT_SWIM_CHEST_M - FEET_OFFSET);
	float odDz = odTargetZ - GetPosition().z;
	if (odDz > 0.6f)
		m_vecMoveSpeed.z = 0.045f;
	else if (odDz > 0.05f)
		m_vecMoveSpeed.z = Max(0.004f, odDz * 0.04f);
	else if (odDz > 0.0f)
		m_vecMoveSpeed.z = 0.004f;
	else
		m_vecMoveSpeed.z = Max(-0.006f, odDz * 0.02f);
	m_fMoveSpeed = odLevel * GAME_SPEED_TO_METERS_PER_SECOND;

	CAnimBlendAssociation *odSwimAssoc = CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_PLAYERSWIM, odSwimAnim, 4.0f);
	float odMedido = 0.0f;
	if (s_odSwimLastPosOk) {
		CVector odAhora = GetPosition();
		CVector odDelta = odAhora - s_odSwimLastPos;
		s_odSwimLastPos = odAhora;
		float odDt = CTimer::GetTimeStepInSeconds();
		if (odDt > 0.001f) {
			float odInst = CVector2D(odDelta.x, odDelta.y).Magnitude() / odDt;
			odMedido = s_odSwimMedido + (odInst - s_odSwimMedido) * Min(1.0f, odDt * 8.0f);
		}
	} else {
		s_odSwimLastPos = GetPosition();
		s_odSwimLastPosOk = true;
	}
	float odPedido = 0.0f;
	if (padMove > 0.0f) {
		float odSwimNatural = ViceExtSwimClipSpeed(odSwimAnim, odSwimAssoc);
		float odWant = Min(padMove / PAD_MOVE_TO_GAME_WORLD_MOVE, 1.0f);
		float odSpeed = odLevel * GAME_SPEED_TO_METERS_PER_SECOND;
		if (odSwimAssoc) {
			odPedido = odSwimNatural > 0.01f ? odSpeed / odSwimNatural : 0.0f;
			odSwimAssoc->speed = odPedido > 0.01f
			    ? Min(3.0f, odPedido) * VICEEXT_SWIM_STROKE_SLACK * odWant
			    : odWant;
		}
	} else if (odSwimAssoc) {
		odSwimAssoc->speed = 1.0f;
	}
	if (odSwimAssoc && padMove > 0.0f) {
		float odLen = odSwimAssoc->hierarchy != nil ? odSwimAssoc->hierarchy->totalLength : 0.0f;
		if (odLen > 0.01f) {
			float odT = odSwimAssoc->currentTime;
			float odFase = odT / odLen;
			bool odWrap = (odT < s_odSwimPrevTime) || (s_odSwimPrevTime - odT > odLen * 0.5f);
			s_odSwimPrevTime = odT;
			if (odWrap) {
#ifdef __EMSCRIPTEN__
				char t[96];
				snprintf(t, sizeof t, "SWIMFASE wrap=1 fase=%.3f previo=%.3f medido=%.2f sprint=%d",
					odFase, s_odPrevFase, odMedido, odSprintBtn ? 1 : 0);
				ODTRACES(t);
#endif
				s_odPrevFase = odFase;
			}
			if (odWrap) {
				float odFuerza = Min(1.0f, 0.20f + odMedido * 0.22f);
				int odSfx = odSprintBtn ? (s_odNadoBrazo ? SOUND_NADO_BRAZO_LS_B : SOUND_NADO_BRAZO_LS_A)
				                       : (s_odNadoBrazo ? SOUND_NADO_BRAZO_B : SOUND_NADO_BRAZO_A);
				s_odNadoBrazo = !s_odNadoBrazo;
				float odNat = ViceExtSwimClipSpeed(odSwimAnim, odSwimAssoc);
				float odCubierto = odNat > 0.01f ? odNat * odSwimAssoc->speed : 0.0f;
				CVector odPie = GetPosition();
				odPie.z -= FEET_OFFSET * 0.35f;
				CRGBA odColor;
				odColor.red = (uint8)((0.5f * CTimeCycle::GetDirectionalRed() + CTimeCycle::GetAmbientRed()) * 127.5f);
				odColor.green = (uint8)((0.5f * CTimeCycle::GetDirectionalGreen() + CTimeCycle::GetAmbientGreen()) * 127.5f);
				odColor.blue = (uint8)((0.5f * CTimeCycle::GetDirectionalBlue() + CTimeCycle::GetAmbientBlue()) * 127.5f);
				odColor.alpha = (uint8)(64 + 32 * odFuerza);
				float odNivel = 0.0f;
				if (CWaterLevel::GetWaterLevel(odPie, &odNivel, false))
					odPie.z = odNivel + 0.1f;
				particleProduceFootSplash(this, odPie, 0.30f + 0.25f * odFuerza, 5 + (int)odFuerza * 7, odColor);
				if (odFuerza > 0.7f)
					CParticleObject::AddObject(POBJECT_PED_WATER_SPLASH, odPie,
						CVector(0.0f, 0.0f, 0.1f), 0.0f, 30, odColor, true);
				ViceExtSwimWake(this, odMedido, odColor);
				DMAudio.PlayOneShot(m_audioEntityId, (uint16)odSfx, 0.0f);
#ifdef __EMSCRIPTEN__
				static uint32 s_odEfNext = 0;
				uint32 odNowMs = CTimer::GetTimeInMilliseconds();
				if (odNowMs >= s_odEfNext || odNowMs + 60000 < s_odEfNext) {
					s_odEfNext = odNowMs + 200;
					char t[192];
					snprintf(t, sizeof t, "SWIMEF braz=1 splash=1 medido=%.2f pedido=%.2f nivel=%.2f "
						"sonido=%d ritmo=%.2f sprint=%d cubierto=%.2f",
						odMedido, odPedido, odLevel, odSfx, odSwimAssoc->speed,
						odSprintBtn ? 1 : 0, odCubierto);
					ODTRACES(t);
				}
#endif
			}
		}
	}

#ifdef __EMSCRIPTEN__
	{
		static uint32 s_odNextTrace = 0;
		static float s_odWorld = 0.0f, s_odWorldLast = 0.0f;
		static CVector s_odLastPos(0.0f, 0.0f, 0.0f);
		static bool s_odLastPosOk = false;
		static float s_odLastDest = 0.0f;
		uint32 odNow = CTimer::GetTimeInMilliseconds();
		s_odWorld += CTimer::GetTimeStep() / 50.0f;
		if (padMove > 0.0f)
			s_odLastDest = m_fRotationDest;
		if (odNow < s_odNextTrace && odNow + 60000 >= s_odNextTrace) {}
		else {
			CVector odPos = GetPosition();
			float odAvance = 0.0f, odSube = 0.0f, odVelo = 0.0f, odDtWorld = s_odWorld - s_odWorldLast;
			s_odWorldLast = s_odWorld;
			if (s_odLastPosOk) {
				CVector odDelta = odPos - s_odLastPos;
				odAvance = CVector2D(odDelta.x, odDelta.y).Magnitude();
				odSube = odDelta.z;
				if (odDtWorld > 0.001f)
					odVelo = odAvance / odDtWorld;
			}
			s_odLastPos = odPos;
			s_odLastPosOk = true;
			s_odNextTrace = odNow + 1000;
			char t[768];
			snprintf(t, sizeof t, "VICEEXT swim move spd=%.1f clip=%d estado=%d",
				m_fMoveSpeed, (int)odSwimAnim, s_odSwimState);
			ODTRACES(t);
			{
				bool odAim = CPad::GetPad(0)->GetTarget() != 0;
				uint32 odFovNow = CTimer::GetTimeInMilliseconds();
				if (odAim != s_odFovPrev) {
					s_odFovPrev = odAim;
					s_odFovPaso = 0;
					s_odFovNext = odFovNow + 60;
				}
				if (odAim && s_odFovPaso < 4) {
					if (odFovNow >= s_odFovNext || odFovNow + 60000 < s_odFovNext) {
						static const int s_odFovEspera[4] = { 0, 150, 300, 600 };
						s_odFovNext = odFovNow + 80;
						CVector odFovCam = TheCamera.GetPosition();
						CVector odFovPed = GetPosition();
						snprintf(t, sizeof t, "SWIMFOV fov=%.2f aim=%d aplica=%d modo=%d camdist=%.2f "
							"paso=%d beta=%.3f alpha=%.3f",
							CDraw::GetFOV(), odAim ? 1 : 0, 0,
							(int)TheCamera.Cams[TheCamera.ActiveCam].Mode,
							CVector2D(odFovCam.x - odFovPed.x, odFovCam.y - odFovPed.y).Magnitude(),
							s_odFovEspera[s_odFovPaso], TheCamera.Cams[TheCamera.ActiveCam].Beta,
							TheCamera.Cams[TheCamera.ActiveCam].Alpha);
						ODTRACES(t);
						s_odFovPaso++;
					}
				}
			}
			CVector odCamPos = TheCamera.GetPosition();
			float odSuelo = -1.0f;
			{
				CColPoint odCol;
				CEntity *odColEnt;
				CVector odPie = GetPosition();
				odPie.z -= FEET_OFFSET;
				if (CWorld::ProcessVerticalLine(odPie, odPie.z - 20.0f, odCol, odColEnt,
						true, true, false, true, false, false, nil))
					odSuelo = odPie.z - odCol.point.z;
			}
			snprintf(t, sizeof t, "SWIM2 move spd=%.1f avance=%.2f velo=%.2f dt=%.2f sube=%.2f clip=%d camz=%.2f camdist=%.2f modo=%d ahogando=%d serega=%.3f ticks=%d sprint=%d assoc=%.2f est=%d rot=%.2f ori=%.2f beta=%.2f aim=%d law=%d hdg=%.2f sw=%.2f fall=%.2f contacto=%d aire=%d vz=%.3f nivel=%.2f hondo=%.2f arma=%d glide=%.2f pie=%d col=%d duke=%d obj=%d suelo=%.2f odcrouch=%d",
				m_fMoveSpeed, odAvance, odVelo, odDtWorld, odSube, (int)odSwimAnim,
				odCamPos.z,
				CVector2D(odCamPos.x - GetPosition().x, odCamPos.y - GetPosition().y).Magnitude(),
				(int)TheCamera.Cams[TheCamera.ActiveCam].Mode, (int)bIsDrowning,
				odLevel, s_odSwimTicks, odSprintBtn ? 1 : 0,
				(odSwimAssoc ? odSwimAssoc->speed : -1.0f), (int)m_nPedState,
				m_fRotationCur, TheCamera.Orientation, TheCamera.Cams[TheCamera.ActiveCam].Beta,
				0, 0, s_odLastDest,
				(odSwimAssoc ? odSwimAssoc->blendAmount : -1.0f),
				(RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_FALL) ? RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_FALL)->blendAmount : -1.0f),
				(int)bIsInWater, odAire, m_vecMoveSpeed.z,
				odLevelW, odDepth, (int)m_nSelectedWepSlot,
		0.0f,
		(int)bIsStanding, (int)bHasCollided, (int)bIsDucking,
		(int)(m_pPointGunAt != nil), odSuelo, 0);
			ODTRACES(t);
			float odEsperada = odLevel * GAME_SPEED_TO_METERS_PER_SECOND;
			bool odErrCat = odAvance > 8.0f;
			bool odErrBanda = s_odSwimState > 1 && odAvance < 0.5f * odEsperada;
			if (s_odSwimState != 0 && odDtWorld > 0.8f && (odErrCat || odErrBanda)) {
				snprintf(t, sizeof t, "SWIMERR esperada=%.1f avance=%.2f estado=%d",
					odEsperada, odAvance, s_odSwimState);
				ODTRACES(t);
			}
		}
	}
#endif
	return true;
}

bool
CPlayerPed::ViceExtIsSwimming(void)
{
	return odSwimActive;
}
#endif

#ifdef __EMSCRIPTEN__
void
CPlayerPed::ViceExtPedAtTrace(void)
{
	static uint32 s_odNextTrace = 0;
	uint32 odNow = CTimer::GetTimeInMilliseconds();
	uint32 odFrame = CTimer::GetFrameCounter();

	static uint32 s_odLastFrame = 0;
	static float s_odWorld = 0.0f, s_odWorldLast = 0.0f;
	if (odFrame != s_odLastFrame) {
		s_odWorld += CTimer::GetTimeStep() / 50.0f;
		s_odLastFrame = odFrame;
	}

	if (odNow < s_odNextTrace && odNow + 60000 >= s_odNextTrace)
		return;

	static uint32 s_odLastNow = 0, s_odLastFrameCount = 0;
	static CVector s_odLastPos(0.0f, 0.0f, 0.0f);
	static bool s_odFirst = true;

	uint32 odMs = odNow - s_odLastNow;
	uint32 odFrames = odFrame - s_odLastFrameCount;
	float odAv = 0.0f, odVelo = 0.0f;
	CVector odPos = GetPosition();
	if (!s_odFirst) {
		CVector odDelta = odPos - s_odLastPos;
		odAv = CVector2D(odDelta.x, odDelta.y).Magnitude();
		if (odMs > 1)
			odVelo = odAv / (odMs / 1000.0f);
	}
	float odTs = s_odWorld - s_odWorldLast;
	CVector odCamPos = TheCamera.GetPosition();
	float odMarDeg = -1.0f, odMarDist = -1.0f;
	for (int i = 0; i < 16; i++) {
		float odA = i * (TWOPI / 16.0f);
		for (int s = 1; s <= 4; s++) {
			float odD = s * 12.0f;
			float odWl;
			float odPx = odPos.x + Cos(odA) * odD, odPy = odPos.y + Sin(odA) * odD;
			int32 odIx = WATER_TO_SMALL_SECTOR_X(odPx + WATER_X_OFFSET);
			int32 odIy = WATER_TO_SMALL_SECTOR_Y(odPy);
			if (odIx < 0 || odIx >= MAX_SMALL_SECTORS || odIy < 0 || odIy >= MAX_SMALL_SECTORS)
				continue;
			if (CWaterLevel::GetWaterLevelNoWaves(odPx, odPy, odPos.z + 2.0f, &odWl)) {
				if (odMarDeg < 0.0f || odD < odMarDist) {
					odMarDeg = odA * 180.0f / PI;
					odMarDist = odD;
				}
				break;
			}
		}
	}
	char t[300];
	snprintf(t, sizeof t, "PEDAT x=%.1f y=%.1f z=%.1f est=%d av=%.2f velo=%.2f ms=%u nf=%u ts=%.2f camz=%.2f camdist=%.2f modo=%d mar=%.0f dist=%.0f",
		odPos.x, odPos.y, odPos.z, (int)m_nPedState, odAv, odVelo, odMs, odFrames, odTs,
		odCamPos.z,
		CVector2D(odCamPos.x - odPos.x, odCamPos.y - odPos.y).Magnitude(),
		(int)TheCamera.Cams[TheCamera.ActiveCam].Mode, odMarDeg, odMarDist);
	ODTRACES(t);

	s_odLastNow = odNow;
	s_odLastFrameCount = odFrame;
	s_odLastPos = odPos;
	s_odWorldLast = s_odWorld;
	s_odFirst = false;
	s_odNextTrace = odNow + 1000;
}
#endif

void
CPlayerPed::ProcessControl(void)
{
	// features.ini `HealthRegenerationUpToHalf` (familia 11): el fichero del mod
	// manda; con la clave a 0 no se recupera vida parado.
	if (ViceExtFeatures::On("HealthRegenerationUpToHalf", true))
	{
		bool odRegenNow = false;
		if (!bInVehicle && !bIsInTheAir && m_nMoveState == PEDMOVE_STILL && m_fHealth > 0.0f
			&& m_fHealth < CWorld::Players[CWorld::PlayerInFocus].m_nMaxHealth * 0.5f)
			odRegenNow = true;
		if (odRegenNow)
			m_fHealth = Min(m_fHealth + 0.15f, CWorld::Players[CWorld::PlayerInFocus].m_nMaxHealth * 0.5f);
#ifdef __EMSCRIPTEN__
		{
			static bool s_odRegenWas = false;
			if (odRegenNow != s_odRegenWas) {
				s_odRegenWas = odRegenNow;
				char t[176];
				snprintf(t, sizeof t, "P4 kind=ini schema=1 gen=1 frame=%u sim=%.4f case=0 opt=8 id=0 regen=%d hp=%.1f max=%d", (unsigned)CTimer::GetFrameCounter(), CTimer::GetTimeInMilliseconds() * 0.001f, (int)odRegenNow, m_fHealth, (int)CWorld::Players[CWorld::PlayerInFocus].m_nMaxHealth);
				ODTRACES(t);
			}
		}
#endif
	}
#ifdef VICEEXT_FIRING_AND_AIMING
	if (bIsPlayerAiming && (!IsPedInControl() && m_nPedState != PED_ROLL || m_nPedState == PED_FIGHT)) {
		bIsPlayerAiming = false;
		ClearWeaponTarget();
	}
#endif
#if defined VICEEXT_FIRING_AND_AIMING && defined VICEEXT_SWIMMING
	if (bIsPlayerAiming && ViceExtPedOwns(PEDLANE_NADO, PEDCAP_APUNTAR)) {
		bIsPlayerAiming = false;
		ClearWeaponTarget();
	}
#endif

#ifdef VICEEXT_CROUCH
	if (!bIsDucking && TheCamera.Cams[TheCamera.ActiveCam].m_fTargetCameraPosZ != 0.0f)
		TheCamera.Cams[TheCamera.ActiveCam].m_fTargetCameraPosZ = 0.0f;
#endif
#ifdef __EMSCRIPTEN__
	ViceExtMovTrace(this);
#endif
#if defined __EMSCRIPTEN__ && defined VICEEXT_FIRING_AND_AIMING
	ViceExtAimTrace(this);
#endif

	// Mobile has some debug/abandoned cheat thing in here: "gbFrankenTommy"

	if (m_nEvadeAmount != 0)
		--m_nEvadeAmount;

	if (m_nEvadeAmount == 0)
		m_pEvadingFrom = nil;

	if (m_pWanted->GetWantedLevel() > 0)
		FindNewAttackPoints();
	
	UpdateMeleeAttackers();

	if (m_pCurrentPhysSurface && m_pCurrentPhysSurface->IsVehicle() && ((CVehicle*)m_pCurrentPhysSurface)->IsBoat()) {
		bTryingToReachDryLand = true;

	} else if (!(((uint8)CTimer::GetFrameCounter() + m_randomSeed) & 0xF)) {
		CVehicle *nearVeh = (CVehicle*)CWorld::TestSphereAgainstWorld(GetPosition(), 7.0f, nil, false, true, false, false, false, false);
		if (nearVeh && nearVeh->IsBoat())
			bTryingToReachDryLand = true;
		else
			bTryingToReachDryLand = false;
	}

	if (m_nFadeDrunkenness) {
		if (m_nDrunkenness - 1 > 0) {
			--m_nDrunkenness;
		} else {
			m_nDrunkenness = 0;
			CMBlur::ClearDrunkBlur();
			m_nFadeDrunkenness = 0;
		}
	}
	if (m_nDrunkenness != 0) {
		CMBlur::SetDrunkBlur(m_nDrunkenness / 255.f);
	}
#ifdef __EMSCRIPTEN__
	if (1) {
		static int s_odBaseWas = -1;
		int odBaseNow = (int)m_nPedState;
		if (odBaseNow != s_odBaseWas) {
			s_odBaseWas = odBaseNow;
			char odBaseT[200];
			snprintf(odBaseT, sizeof(odBaseT), "P4 kind=firewait schema=1 gen=1 frame=%u sim=%.4f case=4 id=0 run=4 cur=0.0000 delay=0.0000 estado=%d arma=%d wtag=71-4cfix6", (unsigned)CTimer::GetFrameCounter(), CTimer::GetTimeInMilliseconds() * 0.001f, (int)m_nPedState, (int)GetWeapon()->m_eWeaponType);
			ODTRACES(odBaseT);
		}
	}
#endif
	CPed::ProcessControl();
#ifdef __EMSCRIPTEN__
	if (1) {
		static int s_odBaseAfterWas = -1;
		int odBaseAfterNow = (int)m_nPedState;
		if (odBaseAfterNow != s_odBaseAfterWas) {
			s_odBaseAfterWas = odBaseAfterNow;
			char odBaseAfterT[200];
			snprintf(odBaseAfterT, sizeof(odBaseAfterT), "P4 kind=firewait schema=1 gen=1 frame=%u sim=%.4f case=4 id=0 run=5 cur=0.0000 delay=0.0000 estado=%d arma=%d wtag=71-4cfix6", (unsigned)CTimer::GetFrameCounter(), CTimer::GetTimeInMilliseconds() * 0.001f, (int)m_nPedState, (int)GetWeapon()->m_eWeaponType);
			ODTRACES(odBaseAfterT);
		}
	}
#endif
	SetNearbyPedsToInteractWithPlayer();
	if (bWasPostponed)
		return;

	CPad *padUsed = GetPadFromPlayer(this);
	m_pWanted->Update();
	PruneReferences();

	if (GetWeapon()->m_eWeaponType == WEAPONTYPE_MINIGUN) {
		CWeaponInfo *weaponInfo = CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType);
		CAnimBlendAssociation *fireAnim = RpAnimBlendClumpGetAssociation(GetClump(), GetPrimaryFireAnim(weaponInfo));
		if (fireAnim && fireAnim->currentTime - fireAnim->timeStep < weaponInfo->m_fAnimLoopEnd && m_nPedState == PED_ATTACK) {
			if (m_fGunSpinSpeed < 0.45f) {
				m_fGunSpinSpeed = Min(0.45f, m_fGunSpinSpeed + CTimer::GetTimeStep() * 0.013f);
			}

			if (padUsed->GetWeapon() && GetWeapon()->m_nAmmoTotal > 0 && fireAnim->currentTime >= weaponInfo->m_fAnimLoopStart) {
				DMAudio.PlayOneShot(m_audioEntityId, SOUND_WEAPON_MINIGUN_ATTACK, 0.0f);
			} else {
				DMAudio.PlayOneShot(m_audioEntityId, SOUND_WEAPON_MINIGUN_2, m_fGunSpinSpeed * (20.f / 9));
			}
		} else {
			if (m_fGunSpinSpeed > 0.0f) {
				if (m_fGunSpinSpeed >= 0.45f) {
					DMAudio.PlayOneShot(m_audioEntityId, SOUND_WEAPON_MINIGUN_3,  0.0f);
				}
				m_fGunSpinSpeed = Max(0.0f, m_fGunSpinSpeed - CTimer::GetTimeStep() * 0.003f);
			}
		}
	}
	if (GetWeapon()->m_eWeaponType == WEAPONTYPE_CHAINSAW && m_nPedState != PED_ATTACK && !bInVehicle) {
		DMAudio.PlayOneShot(m_audioEntityId, SOUND_WEAPON_CHAINSAW_IDLE, 0.0f);
	}

	if (m_nMoveState != PEDMOVE_RUN && m_nMoveState != PEDMOVE_SPRINT)
		RestoreSprintEnergy(1.0f);
	else if (m_nMoveState == PEDMOVE_RUN)
		RestoreSprintEnergy(0.3f);

	if (m_nPedState == PED_DEAD) {
		ClearWeaponTarget();
		return;
	}
	if (m_nPedState == PED_DIE) {
		ClearWeaponTarget();
		if (CTimer::GetTimeInMilliseconds() > m_bloodyFootprintCountOrDeathTime + 4000)
			SetDead();
		return;
	}
	if (m_nPedState == PED_DRIVING && m_objective != OBJECTIVE_LEAVE_CAR) {
		if (!CReplay::IsPlayingBack() || m_pMyVehicle) {
			if (m_pMyVehicle->IsCar() && ((CAutomobile*)m_pMyVehicle)->Damage.GetDoorStatus(DOOR_FRONT_LEFT) == DOOR_STATUS_SWINGING) {
				CAnimBlendAssociation *rollDoorAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_CAR_CLOSE_DOOR_ROLLING_LHS);

				if (m_pMyVehicle->m_nGettingOutFlags & CAR_DOOR_FLAG_LF || rollDoorAssoc || (rollDoorAssoc = RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_CAR_CLOSE_DOOR_ROLLING_LO_LHS))) {
					if (rollDoorAssoc)
						m_pMyVehicle->ProcessOpenDoor(CAR_DOOR_LF, ANIM_STD_CAR_CLOSE_DOOR_ROLLING_LHS, rollDoorAssoc->currentTime);

				} else {
					// These comparisons are wrong, they return uint16
					if (padUsed && (padUsed->GetAccelerate() != 0.0f || padUsed->GetSteeringLeftRight() != 0.0f || padUsed->GetBrake() != 0.0f)) {
						if (rollDoorAssoc)
							m_pMyVehicle->ProcessOpenDoor(CAR_DOOR_LF, ANIM_STD_CAR_CLOSE_DOOR_ROLLING_LHS, rollDoorAssoc->currentTime);

					} else {
						m_pMyVehicle->m_nGettingOutFlags |= CAR_DOOR_FLAG_LF;
						if (m_pMyVehicle->bLowVehicle)
							rollDoorAssoc = CAnimManager::AddAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_CAR_CLOSE_DOOR_ROLLING_LO_LHS);
						else
							rollDoorAssoc = CAnimManager::AddAnimation(GetClump(), ASSOCGRP_STD, ANIM_STD_CAR_CLOSE_DOOR_ROLLING_LHS);

						rollDoorAssoc->SetFinishCallback(PedAnimDoorCloseRollingCB, this);
					}
				}
			}
		}
		return;
	}
	if (m_objective == OBJECTIVE_NONE)
		m_nMoveState = PEDMOVE_STILL;
	if (bIsLanding)
		RunningLand(padUsed);

#ifndef VICEEXT_FIRING_AND_AIMING
	if (padUsed && padUsed->WeaponJustDown() && !TheCamera.Using1stPersonWeaponMode()) {
		// ...Really?
		eWeaponType playerWeapon = FindPlayerPed()->GetWeapon()->m_eWeaponType;
		if (playerWeapon == WEAPONTYPE_SNIPERRIFLE || playerWeapon == WEAPONTYPE_LASERSCOPE) {
			DMAudio.PlayFrontEndSound(SOUND_WEAPON_SNIPER_SHOT_NO_ZOOM, 0);
		} else if (playerWeapon == WEAPONTYPE_ROCKETLAUNCHER) {
			DMAudio.PlayFrontEndSound(SOUND_WEAPON_ROCKET_SHOT_NO_ZOOM, 0);
		}
	}
#endif

#ifdef VICEEXT_CROUCH
	if (m_nPedState == PED_ROLL && !bIsStanding) {
		RestorePreviousState();

		ClearDuck();
	}

	if (m_nPedState == PED_ROLL && !RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_CROUCH_ROLL_L) && !RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_CROUCH_ROLL_R)) {
		SetRealMoveAnim();
	}
#endif

#ifdef VICEEXT_FIRING_AND_AIMING
	if (InVehicle() && bIsPlayerAiming && m_pMyVehicle->GetUp().z < 0.2f)
		ClearWeaponTarget();
#endif

#if defined VICEEXT_FIRING_AND_AIMING && defined VICEEXT_FIRST_PERSON
	if (IsDoomMode() && (!IsPedInControl() || m_nPedState == PED_ANSWER_MOBILE || m_nPedState == PED_SEEK_CAR || m_nPedState == PED_SEEK_IN_BOAT)) {
		bIsPlayerAiming = false;
		ClearWeaponTarget();

		StopAimingAnims();
	}
#endif

#ifdef VICEEXT_SWIMMING
	// Sección 3, bloque C4 (20/09): nadar. Va antes del reparto de control
	// porque mientras se nada el control a pie no debe correr (ver
	// ViceExtSwimControl). Si viene de una caída al agua, el estado se deja en
	// PED_IDLE: ya no está cayendo, está nadando.
	bool odSwimming = ViceExtSwimControl(padUsed);
	if (odSwimming && (m_nPedState == PED_FALL || m_nPedState == PED_JUMP))
		SetPedState(PED_IDLE);
#endif

#ifdef VICEEXT_IMPROVED_TECH_PART
	if (m_nPedState == PED_ANSWER_MOBILE && m_bSkipPhoneCall) {
		for (int slot = 0; slot < MISSION_AUDIO_SLOTS; slot++)
			AudioManager.FinishMissionAudioPhoneDialogue(slot);
	} else if (m_nPedState == PED_ANSWER_MOBILE && !m_bSkipPhoneCall && CPad::GetPad(0)->ExitVehicleJustDown()) {
		m_bSkipPhoneCall = true;
	}
#endif

	switch (m_nPedState) {
		case PED_NONE:
		case PED_IDLE:
		case PED_FLEE_POS:
		case PED_FLEE_ENTITY:
		case PED_ATTACK:
		case PED_FIGHT:
		case PED_AIM_GUN:
		case PED_ANSWER_MOBILE:
#ifdef VICEEXT_FIRING_AND_AIMING
			if (!RpAnimBlendClumpGetFirstAssociation(GetClump(), ASSOC_BLOCK) && !m_attachedTo && !InVehicle()) {
#else
			if (!RpAnimBlendClumpGetFirstAssociation(GetClump(), ASSOC_BLOCK) && !m_attachedTo) {
#endif
#ifdef VICEEXT_SWIMMING
				if (odSwimming) {
					// C4: nadando. El movimiento lo pone ViceExtSwimControl; aquí no
					// se reparte el control a pie (ni 1ª persona ni Zelda).
				} else
#endif
				if (TheCamera.Using1stPersonWeaponMode()) {
					if (padUsed)
						PlayerControlSniper(padUsed);

				} else if (TheCamera.Cams[0].Using3rdPersonMouseCam()
#ifdef FREE_CAM
					&& !CCamera::bFreeCam
#endif
					) {
					if (padUsed)
						PlayerControl1stPersonRunAround(padUsed);

				} else if (m_nPedState == PED_FIGHT) {
					if (padUsed)
						PlayerControlFighter(padUsed);

				} else if (padUsed) {
					PlayerControlZelda(padUsed);
				}
			}
		if (IsPedInControl() && m_nPedState != PED_ANSWER_MOBILE && padUsed)
#ifdef VICEEXT_SWIMMING
			// R30 (29/09): nadando no hay arma que procesar (punos; el mod no dispara en
			// el agua). Sin esto el clic abria mele y tapaba el nado (swim13).
			if (!CPlayerPed::ViceExtIsSwimming())
#endif
			ProcessPlayerWeapon(padUsed);
#ifdef __EMSCRIPTEN__
		// R19: posición y velocidad real (todas las sondas).
		ViceExtPedAtTrace();
#endif
		break;
		case PED_SEEK_ENTITY:
			m_vecSeekPos = m_pSeekTarget->GetPosition();

			// fall through
		case PED_SEEK_POS:
			switch (m_nMoveState) {
				case PEDMOVE_WALK:
					m_fMoveSpeed = 1.0f;
					break;
				case PEDMOVE_RUN:
					m_fMoveSpeed = 1.8f;
					break;
				case PEDMOVE_SPRINT:
					m_fMoveSpeed = 2.5f;
					break;
				default:
					m_fMoveSpeed = 0.0f;
					break;
			}
			SetRealMoveAnim();
			if (Seek()) {
				RestorePreviousState();
				SetMoveState(PEDMOVE_STILL);
			}
			break;
		case PED_SNIPER_MODE:
			if (GetWeapon()->m_eWeaponType == WEAPONTYPE_SNIPERRIFLE || GetWeapon()->m_eWeaponType == WEAPONTYPE_LASERSCOPE) {
				if (padUsed)
					PlayerControlSniper(padUsed);

			} else if (padUsed) {
				PlayerControlM16(padUsed);
			}
			break;
		case PED_SEEK_CAR:
		case PED_SEEK_IN_BOAT:
			if (bVehEnterDoorIsBlocked || bKindaStayInSamePlace) {
				m_fMoveSpeed = 0.0f;
			} else {
				m_fMoveSpeed = Min(2.0f, 2.0f * (m_vecSeekPos - GetPosition()).Magnitude2D());
			}
			if (padUsed && !padUsed->ArePlayerControlsDisabled()) {
				if (padUsed->GetTarget() || padUsed->GetLeftStickXJustDown() || padUsed->GetLeftStickYJustDown() ||
					padUsed->GetDPadUpJustDown() || padUsed->GetDPadDownJustDown() || padUsed->GetDPadLeftJustDown() ||
					padUsed->GetDPadRightJustDown()) {

					RestorePreviousState();
					if (m_objective == OBJECTIVE_ENTER_CAR_AS_PASSENGER || m_objective == OBJECTIVE_ENTER_CAR_AS_DRIVER) {
						RestorePreviousObjective();
					}
				}
			}
			if (padUsed && padUsed->GetSprint())
				m_nMoveState = PEDMOVE_SPRINT;
			SetRealMoveAnim();
			break;
		case PED_JUMP:
			if (padUsed)
				PlayerControlZelda(padUsed);
			if (bIsLanding)
				break;

			// This has been added later it seems
			return;
		case PED_FALL:
		case PED_GETUP:
		case PED_ENTER_TRAIN:
		case PED_EXIT_TRAIN:
		case PED_CARJACK:
		case PED_DRAG_FROM_CAR:
		case PED_ENTER_CAR:
		case PED_STEAL_CAR:
		case PED_EXIT_CAR:
			ClearWeaponTarget();
			break;
		case PED_ARRESTED:
			if (m_nLastPedState == PED_DRAG_FROM_CAR && m_pVehicleAnim)
				BeingDraggedFromCar();
			break;
		default:
			break;
	}
	if (padUsed && IsPedShootable() && m_nPedState != PED_ANSWER_MOBILE && m_nLastPedState != PED_ANSWER_MOBILE) {
		ProcessWeaponSwitch(padUsed);
		GetWeapon()->Update(m_audioEntityId, this);
	}
	ProcessAnimGroups();
	if (padUsed) {
		if (TheCamera.Cams[TheCamera.ActiveCam].Mode == CCam::MODE_FOLLOWPED
			&& TheCamera.Cams[TheCamera.ActiveCam].DirectionWasLooking == LOOKING_BEHIND) {

			m_lookTimer = 0;
			float camAngle = CGeneral::LimitRadianAngle(TheCamera.Cams[TheCamera.ActiveCam].Front.Heading());
			float angleBetweenPlayerAndCam = Abs(camAngle - m_fRotationCur);

			if (m_nPedState != PED_ATTACK && angleBetweenPlayerAndCam > DEGTORAD(30.0f) && angleBetweenPlayerAndCam < DEGTORAD(330.0f)) {
				if (angleBetweenPlayerAndCam > DEGTORAD(150.0f) && angleBetweenPlayerAndCam < DEGTORAD(210.0f)) {
					float rightTurnAngle = CGeneral::LimitRadianAngle(m_fRotationCur - DEGTORAD(150.0f));
					float leftTurnAngle = CGeneral::LimitRadianAngle(DEGTORAD(150.0f) + m_fRotationCur);

					if (m_fLookDirection == 999999.0f || bIsDucking)
						camAngle = rightTurnAngle;
					else if (Abs(rightTurnAngle - m_fLookDirection) < Abs(leftTurnAngle - m_fLookDirection))
						camAngle = rightTurnAngle;
					else
						camAngle = leftTurnAngle;
				}
				SetLookFlag(camAngle, true);
				SetLookTimer(CTimer::GetTimeStepInMilliseconds() * 5.0f);
			} else {
				ClearLookFlag();
			}
		}
	}
	if (m_nMoveState == PEDMOVE_SPRINT && bIsLooking) {
		ClearLookFlag();
		SetLookTimer(250);
	}

	if (m_vecMoveSpeed.Magnitude2D() < 0.1f) {
		if (m_nSpeedTimer) {
			if (CTimer::GetTimeInMilliseconds() > m_nSpeedTimer)
				m_bSpeedTimerFlag = true;
		} else {
			m_nSpeedTimer = CTimer::GetTimeInMilliseconds() + 500;
		}
	} else {
		m_nSpeedTimer = 0;
		m_bSpeedTimerFlag = false;
	}

	if (bDontAllowWeaponChange && FindPlayerPed() == this) {
#ifdef VICEEXT_CROUCH
		if (!CPad::GetPad(0)->GetTarget() && m_nPedState != PED_ROLL)
#else
		if (!CPad::GetPad(0)->GetTarget())
#endif
			bDontAllowWeaponChange = false;
	}

	if (m_nPedState != PED_SNIPER_MODE && (GetWeapon()->m_eWeaponState == WEAPONSTATE_FIRING || m_nPedState == PED_ATTACK))
		m_nPadDownPressedInMilliseconds = CTimer::GetTimeInMilliseconds();

	if (!bIsVisible)
		UpdateRpHAnim();
}

bool
CPlayerPed::DoesPlayerWantNewWeapon(eWeaponType weapon, bool onlyIfSlotIsEmpty)
{
	// GetPadFromPlayer(); // unused
	uint32 slot = CWeaponInfo::GetWeaponInfo(weapon)->m_nWeaponSlot;

	if (!HasWeaponSlot(slot) || GetWeapon(slot).m_eWeaponType == weapon)
		return true;

	if (onlyIfSlotIsEmpty)
		return false;

	// Check if he's using that slot right now.
	return m_nPedState != PED_ATTACK && m_nPedState != PED_AIM_GUN || slot != m_currentWeapon;
}

void
CPlayerPed::PlayIdleAnimations(CPad *padUsed)
{
	CAnimBlendAssociation* assoc;

#ifdef VICEEXT_CLIMBING
	if (bIsClimbing)
		return;
#endif

	if (TheCamera.m_WideScreenOn || bIsDucking)
		return;

	struct animAndGroup {
		AnimationId animId;
		AssocGroupId groupId;
	};

	const animAndGroup idleAnims[] = {
		{ANIM_PLAYER_IDLE1, ASSOCGRP_PLAYER_IDLE},
		{ANIM_PLAYER_IDLE2, ASSOCGRP_PLAYER_IDLE},
		{ANIM_PLAYER_IDLE3, ASSOCGRP_PLAYER_IDLE},
		{ANIM_PLAYER_IDLE4, ASSOCGRP_PLAYER_IDLE},
		{ANIM_STD_XPRESS_SCRATCH, ASSOCGRP_STD},
	};

	static int32 lastTime = 0;
	static int32 lastAnim = -1;

	bool hasIdleAnim = false;
	CAnimBlock *idleAnimBlock = CAnimManager::GetAnimationBlock(idleAnimBlockIndex);
	uint32 sinceLastInput = padUsed->InputHowLongAgo();
	if (sinceLastInput <= 30000) {
		if (idleAnimBlock->isLoaded) {
			for (assoc = RpAnimBlendClumpGetFirstAssociation(GetClump()); assoc; assoc = RpAnimBlendGetNextAssociation(assoc)) {
				if (assoc->flags & ASSOC_IDLE) {
					hasIdleAnim = true;
					assoc->blendDelta = -8.0f;
				}
			}
			if (!hasIdleAnim)
				CStreaming::RemoveAnim(idleAnimBlockIndex);
		} else {
			lastTime = 0;
		}
	} else {
		CStreaming::RequestAnim(idleAnimBlockIndex, STREAMFLAGS_DONT_REMOVE);
		if (idleAnimBlock->isLoaded) {
			for(CAnimBlendAssociation *assoc = RpAnimBlendClumpGetFirstAssociation(GetClump()); assoc; assoc = RpAnimBlendGetNextAssociation(assoc)) {
				int firstIdle = idleAnimBlock->firstIndex;
				int index = assoc->hierarchy - CAnimManager::GetAnimation(0);
				if (index >= firstIdle && index < firstIdle + idleAnimBlock->numAnims) {
					hasIdleAnim = true;
					break;
				}
			}

			if (!hasIdleAnim && !bIsLooking && !bIsRestoringLook && sinceLastInput - lastTime > 25000) {
				int anim;
				do
					anim = CGeneral::GetRandomNumberInRange(0, ARRAY_SIZE(idleAnims));
				while (lastAnim == anim);

				assoc = CAnimManager::BlendAnimation(GetClump(), idleAnims[anim].groupId, idleAnims[anim].animId, 8.0f);
				assoc->flags |= ASSOC_IDLE;
				lastAnim = anim;
				lastTime = sinceLastInput;
			}
		}
	}
}

void
CPlayerPed::SetNearbyPedsToInteractWithPlayer(void)
{
	if (CGame::noProstitutes)
		return;

	for (int i = 0; i < m_numNearPeds; ++i) {
		CPed *nearPed = m_nearPeds[i];
		if (nearPed && nearPed->m_objectiveTimer < CTimer::GetTimeInMilliseconds() && !CTheScripts::IsPlayerOnAMission()) {
			int mi = nearPed->GetModelIndex();
			if (CPopulation::CanSolicitPlayerOnFoot(mi)) {
				CVector distToMe = nearPed->GetPosition() - GetPosition();
				CVector dirToMe = GetPosition() - nearPed->GetPosition();
				dirToMe.Normalise();
				if (DotProduct(dirToMe, nearPed->GetForward()) > 0.707 && DotProduct(GetForward(), nearPed->GetForward()) < -0.707 // those are double
					&& distToMe.MagnitudeSqr() < 9.0f && nearPed->m_objective == OBJECTIVE_NONE) {
					nearPed->SetObjective(OBJECTIVE_SOLICIT_FOOT, this);
					nearPed->m_objectiveTimer = CTimer::GetTimeInMilliseconds() + 10000;
					nearPed->Say(SOUND_PED_SOLICIT);
				}
			} else if (CPopulation::CanSolicitPlayerInCar(mi)) {
				if (InVehicle() && m_pMyVehicle->IsVehicleNormal()) {
					if (m_pMyVehicle->IsCar()) {
						CVector distToVeh = nearPed->GetPosition() - m_pMyVehicle->GetPosition();
						if (distToVeh.MagnitudeSqr() < 25.0f && m_pMyVehicle->IsRoomForPedToLeaveCar(CAR_DOOR_LF, nil) && nearPed->m_objective == OBJECTIVE_NONE) {
							nearPed->SetObjective(OBJECTIVE_SOLICIT_VEHICLE, m_pMyVehicle);
						}
					}
				}
			}
		}
	}
}

void
CPlayerPed::UpdateMeleeAttackers(void)
{
	CVector attackCoord;
	if (((CTimer::GetFrameCounter() + m_randomSeed + 7) & 3) == 0) {
		GetMeleeAttackCoords(attackCoord, m_nAttackDirToCheck, 2.0f);

		// Check if there is any vehicle/building inbetween us and m_nAttackDirToCheck. Peds will be able to attack us from those available directions.
		if (CWorld::GetIsLineOfSightClear(GetPosition(), attackCoord, true, true, false, true, false, false, false)
			&& !CWorld::TestSphereAgainstWorld(attackCoord, 0.4f, m_pMeleeList[m_nAttackDirToCheck], true, true, false, true, false, false)) {
			if (m_pMeleeList[m_nAttackDirToCheck] == this)
				m_pMeleeList[m_nAttackDirToCheck] = nil; // mark it as available
		} else {
			m_pMeleeList[m_nAttackDirToCheck] = this; // slot not available. useful for m_bNoPosForMeleeAttack
		}
		if (++m_nAttackDirToCheck >= ARRAY_SIZE(m_pMeleeList))
			m_nAttackDirToCheck = 0;
	}
	// 6 directions
	for (int i = 0; i < ARRAY_SIZE(m_pMeleeList); ++i) {
		CPed *victim = m_pMeleeList[i];
		if (victim && victim != this) {
			if (victim->m_nPedState != PED_DEAD && victim->m_pedInObjective == this)  {
				if (victim->m_objective == OBJECTIVE_KILL_CHAR_ON_FOOT || victim->m_objective == OBJECTIVE_KILL_CHAR_ANY_MEANS || victim->m_objective == OBJECTIVE_KILL_CHAR_ON_BOAT) {
					GetMeleeAttackCoords(attackCoord, i, 2.0f);
					if ((attackCoord - GetPosition()).MagnitudeSqr() > 12.25f)
						m_pMeleeList[i] = nil;
				} else {
					m_pMeleeList[i] = nil;
				}
			} else {
				m_pMeleeList[i] = nil;
			}
		}
	}
	m_bNoPosForMeleeAttack = m_pMeleeList[0] == this && m_pMeleeList[1] == this && m_pMeleeList[2] == this
#ifdef FIX_BUGS
		&& m_pMeleeList[3] == this
#endif
		&& m_pMeleeList[4] == this && m_pMeleeList[5] == this;
}

void
CPlayerPed::RemovePedFromMeleeList(CPed *ped)
{
	for (uint16 i = 0; i < ARRAY_SIZE(m_pMeleeList); i++) {
		if (m_pMeleeList[i] == ped) {
			m_pMeleeList[i] = nil;
			ped->m_attackTimer = 0;
			return;
		}
	}
}

void
CPlayerPed::GetMeleeAttackCoords(CVector& coords, int8 dir, float dist)
{
	coords = GetPosition();
	switch (dir) {
		case 0:
			coords.y += dist;
			break;
		case 1:
			coords.x += Sqrt(3.f / 4.f) * dist;
			coords.y += 0.5f * dist;
			break;
		case 2:
			coords.x += Sqrt(3.f / 4.f) * dist;
			coords.y -= 0.5f * dist;
			break;
		case 3:
			coords.y -= dist;
			break;
		case 4:
			coords.x -= Sqrt(3.f / 4.f) * dist;
			coords.y -= 0.5f * dist;
			break;
		case 5:
			coords.x -= Sqrt(3.f / 4.f) * dist;
			coords.y += 0.5f * dist;
			break;
		default:
			break;
	}
}

int32
CPlayerPed::FindMeleeAttackPoint(CPed *victim, CVector &dist, uint32 &endOfAttackOut)
{
	endOfAttackOut = 0;
	bool thereIsAnEmptySlot = false;
	int dirToAttack = -1;
	for (int i = 0; i < ARRAY_SIZE(m_pMeleeList); i++) {
		CPed* pedAtThisDir = m_pMeleeList[i];
		if (pedAtThisDir) {
			if (pedAtThisDir == victim) {
				dirToAttack = i;
			} else {
				if (pedAtThisDir->m_attackTimer > endOfAttackOut)
					endOfAttackOut = pedAtThisDir->m_attackTimer;
			}
		} else {
			thereIsAnEmptySlot = true;
		}
	}

	// We don't have victim ped in our melee list
	if (dirToAttack == -1 && thereIsAnEmptySlot) {
		float angle = Atan2(-dist.x, -dist.y);
		float adjustedAngle = angle + DEGTORAD(30.0f);
		if (adjustedAngle < 0.f)
			adjustedAngle += TWOPI;

		int wantedDir = Floor(adjustedAngle / DEGTORAD(60.0f));

		// And we have another ped at the direction of victim ped, so store victim to next empty direction to it's real direction. (Bollocks)
		if (m_pMeleeList[wantedDir]) {
			int closestDirToPreferred = -99;
			int preferredDir = wantedDir;

			for (int i = 0; i < ARRAY_SIZE(m_pMeleeList); i++) {
				if (!m_pMeleeList[i]) {
					if (Abs(i - preferredDir) < Abs(closestDirToPreferred - preferredDir))
						closestDirToPreferred = i;
				}
			}
			if (closestDirToPreferred > 0)
				dirToAttack = closestDirToPreferred;
		} else {

			// Luckily the direction of victim ped is already empty, good
			dirToAttack = wantedDir;
		}

		if (dirToAttack != -1) {
			m_pMeleeList[dirToAttack] = victim;
			victim->RegisterReference((CEntity**) &m_pMeleeList[dirToAttack]);
			if (endOfAttackOut > CTimer::GetTimeInMilliseconds())
				victim->m_attackTimer = endOfAttackOut + CGeneral::GetRandomNumberInRange(1000, 2000);
			else
				victim->m_attackTimer = CTimer::GetTimeInMilliseconds() + CGeneral::GetRandomNumberInRange(500, 1000);
		}
	}
	return dirToAttack;
}

#ifdef COMPATIBLE_SAVES
#define CopyFromBuf(buf, data) memcpy(&data, buf, sizeof(data)); SkipSaveBuf(buf, sizeof(data));
#define CopyToBuf(buf, data) memcpy(buf, &data, sizeof(data)); SkipSaveBuf(buf, sizeof(data));
void
CPlayerPed::Save(uint8*& buf)
{
	CPed::Save(buf);
	ZeroSaveBuf(buf, 16);
	CopyToBuf(buf, m_fMaxStamina);
	ZeroSaveBuf(buf, 28);
	CopyToBuf(buf, m_nTargettableObjects[0]);
	CopyToBuf(buf, m_nTargettableObjects[1]);
	CopyToBuf(buf, m_nTargettableObjects[2]);
	CopyToBuf(buf, m_nTargettableObjects[3]);
	ZeroSaveBuf(buf, 164);
}

void
CPlayerPed::Load(uint8*& buf)
{
	CPed::Load(buf);
	SkipSaveBuf(buf, 16);
	CopyFromBuf(buf, m_fMaxStamina);
	SkipSaveBuf(buf, 28);
	CopyFromBuf(buf, m_nTargettableObjects[0]);
	CopyFromBuf(buf, m_nTargettableObjects[1]);
	CopyFromBuf(buf, m_nTargettableObjects[2]);
	CopyFromBuf(buf, m_nTargettableObjects[3]);
	SkipSaveBuf(buf, 164);
}
#undef CopyFromBuf
#undef CopyToBuf
#endif
