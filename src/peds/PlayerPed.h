#pragma once

#include "Ped.h"

class CPad;
class CCopPed;
class CWanted;

// ClassicAXIS · sección 2, bloque C7: las escopetas del mod (y la nueva
// `SHOTGUN2`) apuntan aunque su `weapon.dat` no traiga `WEAPONFLAG_CANAIM`.
#ifdef VICEEXT_SHOTGUN_AIM
inline bool ViceExtCanAim(eWeaponType type, CWeaponInfo *info)
{
	switch (type) {
	case WEAPONTYPE_SHOTGUN:
	case WEAPONTYPE_STUBBY_SHOTGUN:
	case WEAPONTYPE_SPAS12_SHOTGUN:
	case WEAPONTYPE_SHOTGUN2:	// arma nueva del mod
		return true;
	default:
		return info->IsFlagSet(WEAPONFLAG_CANAIM);
	}
}
#else
#define ViceExtCanAim(type, info) ((info)->IsFlagSet(WEAPONFLAG_CANAIM))
#endif

inline bool ViceExtAimHeavy(eWeaponType type)
{
	switch (type) {
	case WEAPONTYPE_AK47:
	case WEAPONTYPE_M16:
	case WEAPONTYPE_STEYR:
		return true;
	default:
		return false;
	}
}

class CPlayerPed : public CPed
{
public:
	CWanted *m_pWanted;
	CCopPed *m_pArrestingCop;
	float m_fMoveSpeed;
	float m_fCurrentStamina;
	float m_fMaxStamina;
	float m_fStaminaProgress;
	int8 m_nSelectedWepSlot;
	bool m_bSpeedTimerFlag;
	uint8 m_nEvadeAmount;
	uint32 m_nSpeedTimer; // m_nStandStillTimer?
	uint32 m_nHitAnimDelayTimer; // m_nShotDelay?
	float m_fAttackButtonCounter;
	bool m_bHaveTargetSelected;	// may have better name
	CEntity *m_pEvadingFrom;	// is this CPhysical?
	int32 m_nTargettableObjects[4];
	uint32 m_nAdrenalineTime;
	uint8 m_nDrunkenness;             // Needed to work out whether we lost target this frame
	uint8 m_nFadeDrunkenness;
	uint8 m_nDrunkCountdown; //countdown in frames when the drunk effect ends
	bool m_bAdrenalineActive;
	bool m_bHasLockOnTarget;
	bool m_bCanBeDamaged;
	bool m_bNoPosForMeleeAttack;
	bool unk1;
	CVector m_vecSafePos[6]; // safe places from the player, for example behind a tree
	CPed *m_pPedAtSafePos[6];
	CPed *m_pMeleeList[6]; // reachable peds at each direction(6)
	int16 m_nAttackDirToCheck;
	float m_fWalkAngle; //angle between heading and walking direction
	float m_fFPSMoveHeading;
	RpAtomic* m_pMinigunTopAtomic; //atomic for the spinning part of the minigun model
	float m_fGunSpinSpeed; // for minigun
	float m_fGunSpinAngle;
	unsigned int m_nPadDownPressedInMilliseconds;
	unsigned int m_nLastBusFareCollected;
#ifdef FREE_CAM
	bool m_bFreeAimActive;
	CVector m_cachedCamSource;
	CVector m_cachedCamFront;
	CVector m_cachedCamUp;
#endif

	static bool bDontAllowWeaponChange;
#ifndef MASTER
	static bool bDebugPlayerInfo;
#endif

	CPlayerPed();
	~CPlayerPed();
	void SetMoveAnim() { };
	bool CanSprintWithCurrentWeapon(void);	// Vice Extended v1.5 (armas pesadas)

	void ReApplyMoveAnims(void);
	void ClearWeaponTarget(void);
	void SetWantedLevel(int32 level);
	void SetWantedLevelNoDrop(int32 level);
	void KeepAreaAroundPlayerClear(void);
	void AnnoyPlayerPed(bool);
	void MakeChangesForNewWeapon(int32);
	void MakeChangesForNewWeapon(eWeaponType);
	void SetInitialState(void);
	void ProcessControl(void);
#ifdef VICEEXT_MANUAL_RELOAD
	bool ViceExtTryManualReload(void); // Sección 3, C3.1: recarga a mano
#endif
#ifdef VICEEXT_SWIMMING
	bool ViceExtSwimControl(CPad *padUsed); // Sección 3, C4: nadar
	static bool ViceExtIsSwimming(void);    // H2: la cámara va a la superficie
#endif
#ifdef VICEEXT_AIM_WALK
#ifdef VICEEXT_AIM_CLASSICAXIS
	// ClassicAXIS: el control de apuntado del mod (Main.cpp:1203-1461) y la
	// adquisición del objetivo blando de ratón (Main.cpp:1474-1517).
	void ViceExtProcessPlayerPedControl(void);
	void ViceExtFind3rdPersonMouseTarget(void);
	void ViceExtPrepareMove(CPad *padUsed);
	bool ViceExtGetMove(CVector2D &direction, float &speedGame) const;
	bool ViceExtMoveWalkaround(void) const;
	int ViceExtStickBlocked(void) const;
#endif
	void ViceExtAimDirTrace(CPad *padUsed); // R9: medir desviación del apuntado
#endif
#ifdef __EMSCRIPTEN__
	void ViceExtPedAtTrace(void);             // R19: posición y velocidad real (todas las sondas)
	void ViceExtWeaponInfoOf(eWeaponType);    // R13/ve37: ficha WINFO de un arma concreta (arnés web)
#endif
#ifdef VICEEXT_CROUCH
	bool ViceExtCrouchControl(CPad *padUsed); // Sección 3, C5: agachado
	static bool ViceExtIsCrouched(void);      // R6: la cámara baja su objetivo agachado
	static float ViceExtCrouchBlend(void);
	void ViceExtCrouchLimitSpeed(void);       // R6: tope 0,5 m/s tras el control normal
	void ViceExtCrouchAnim(void);             // R6b: el clip de agachado manda sobre el de serie
	float ViceExtCrouchMoveSpeed(void);       // R21b: m/s del clip de agachado en uso (ritmo incluido)
	bool ViceExtCrouchSideHeading(float &odDir); // R26: rumbo MUNDO al que girar a los lados (R29: ya no es static, lee ViceExtIsAiming)
	bool ViceExtIsAiming(void); // R29/G: API unica del "estoy apuntando" (rueda, pose, giro y CROUCH2 la leen sola)
#endif
#ifdef VICEEXT_CLIMB
	bool ViceExtClimbControl(CPad *padUsed);   // Sección 2, E1: escalar
#endif
	void ClearAdrenaline(void);
	void UseSprintEnergy(void);
	class CPlayerInfo *GetPlayerInfoForThisPlayerPed();
	void SetRealMoveAnim(void);
	void RestoreSprintEnergy(float);
	float DoWeaponSmoothSpray(void);
	void DoStuffToGoOnFire(void);
	bool DoesTargetHaveToBeBroken(CVector, CWeapon*);
	void RunningLand(CPad*);
	bool IsThisPedAnAimingPriority(CPed*);
	void PlayerControlSniper(CPad*);
	void PlayerControlM16(CPad*);
	void PlayerControlFighter(CPad*);
	void ProcessWeaponSwitch(CPad*);
	void MakeObjectTargettable(int32);
	void PlayerControl1stPersonRunAround(CPad *padUsed);
	void EvaluateNeighbouringTarget(CEntity*, CEntity**, float*, float, float, bool, bool);
	void EvaluateTarget(CEntity*, CEntity**, float*, float, float, bool);
	bool FindNextWeaponLockOnTarget(CEntity*, bool);
	bool FindWeaponLockOnTarget(void);
	void ProcessAnimGroups(void);
	void ProcessPlayerWeapon(CPad*);
	void PlayerControlZelda(CPad*);
	bool DoesPlayerWantNewWeapon(eWeaponType, bool);
	void PlayIdleAnimations(CPad*);
	void RemovePedFromMeleeList(CPed*);
	void GetMeleeAttackCoords(CVector&, int8, float);
	int32 FindMeleeAttackPoint(CPed*, CVector&, uint32&);
	bool CanIKReachThisTarget(CVector, CWeapon*, bool);
	void RotatePlayerToTrackTarget(void);
	bool MovementDisabledBecauseOfTargeting(void);
	void FindNewAttackPoints(void);
	void SetNearbyPedsToInteractWithPlayer(void);
	void UpdateMeleeAttackers(void);

	static void SetupPlayerPed(int32);
	static void DeactivatePlayerPed(int32);
	static void ReactivatePlayerPed(int32);

#ifdef COMPATIBLE_SAVES
	virtual void Save(uint8*& buf);
	virtual void Load(uint8*& buf);
#endif

	static const uint32 nSaveStructSize;
};

//VALIDATE_SIZE(CPlayerPed, 0x5F0);
