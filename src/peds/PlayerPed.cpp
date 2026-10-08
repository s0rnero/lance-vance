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
#include "Sprite.h"   // ClassicAXIS: CSprite::CalcScreenCoors (Main.cpp:1269)
#include "WeaponEffects.h"
#include "ModelIndices.h"
#include "World.h"
#include "RpAnimBlend.h"
#include "AnimBlendAssociation.h"
#include "AnimBlendAssocGroup.h"   // R23: la traza WINFO necesita `firstAnimId`/`numAssociations` del grupo
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
#include "ControllerConfig.h"  // web: ControlsManager / PED_RELOAD (sección 3, C3.1)
#include "ondemand.h"          // web: ODTRACES -> odtrace.log (sección 3, C3.1)
#include "WaterLevel.h"        // Sección 3, C4 (nadar): CWaterLevel::GetWaterLevel
#include "WeaponInfo.h"
#include "ParticleObject.h"
#include "Draw.h"
#include "Particle.h" // C4 nado: salpicadura de un solo uso por brazada        // Sección 3, C5/C6: CWeaponInfo (peso del arma)

#define PAD_MOVE_TO_GAME_WORLD_MOVE 60.0f

#ifdef VICEEXT_AIM_CLASSICAXIS
static void ViceExtMoveReset(void);
#endif

#if defined(VICEEXT_CROUCH) || defined(__EMSCRIPTEN__)
static const AnimationId odCrouchPoseIds[9] = {
	ANIM_STD_CROUCH_IDLE, ANIM_STD_CROUCH_FORWARD, ANIM_STD_CROUCH_BACKWARD,
	ANIM_STD_CROUCH_LEFT, ANIM_STD_CROUCH_RIGHT, ANIM_STD_CROUCH_AIMFWD,
	ANIM_STD_CROUCH_AIMBWD, ANIM_STD_CROUCH_AIMLEFT, ANIM_STD_CROUCH_AIMRIGHT
};
#endif

#if defined(VICEEXT_CROUCH) || defined(VICEEXT_AIM_WALK)
// Sección 2 (21/09, 5ª partida): estado de agachado a pie de la sección 3 (C5),
// compartido con `PlayerControlZelda` para limitar la velocidad a paso de andar
// mientras se está agachado. Lo mantiene `ViceExtCrouchControl`.
static bool odCrouched = false;
#endif

#ifdef VICEEXT_AIM_WALK
// H4 (12ª partida, 22/09): los dos datos que necesita `SetRealMoveAnim` para
// elegir el clip del caminado apuntando. Los rellena el control a pie de cada
// frame (Zelda o 1ª persona) ANTES del tope de velocidad de R9:
//   - `odAimWalkActive`:   se está apuntando con un arma que apunta.
//   - `odAimWalkUncapped`: la velocidad que habría pedido el jugador sin el tope.
static bool odAimWalkActive = false;
static float odAimWalkUncapped = 0.0f;
#endif

#ifdef VICEEXT_CROUCH
// Sección 3, C5b (21/09): las tres ramas de agachado del motor a pie (sniper,
// arma y desarmado) daban el agachado de DISPARO (`SetDuck` ->
// `bCrouchWhenShooting`), que deja al jugador clavado en el sitio. El agachado
// del jugador lo lleva `ViceExtCrouchControl` (permite andar agachado), así que
// esas ramas ya no leen la tecla: si no, las dos conmutarían a la vez y el
// agachado del motor volvería a bloquear el movimiento.
#define VICEEXT_ENGINE_DUCK_KEY() false
#else
#define VICEEXT_ENGINE_DUCK_KEY() padUsed->DuckJustDown()
#endif

#ifdef VICEEXT_SHOTGUN_AIM
// Sección 3, bloque C7 (20/09): APUNTAR CON LA ESCOPETA.
//
// El dato (`weapon.dat`) trae las escopetas con el campo de flags `277` =
// gravedad + gases + expansión + 1ª persona, **sin** `WEAPONFLAG_CANAIM` — en el
// fichero del mod igual que en el de serie — así que el jugador no podía
// apuntarlas (y el propio motor lo esperaba: en ProcessPlayerWeapon hay un
// "Anim. fix for shotgun, ak47 and m16"). El mod cambió el sistema de apuntado
// (v1.0 "Changed aiming system. Aim while holding the button") y sus animaciones
// (v1.5). Aquí se le añade el flag a las escopetas **desde código**, porque el
// `weapon.dat` es dato de la sección 1.
// El cuerpo vive AHORA en `PlayerPed.h` (plan `apuntado-classicaxis-100` B9):
// la ley de apuntado de `Cam.cpp` necesita el MISMO caso de las escopetas
// (VICEEXT_SHOTGUN_AIM, sección 2 / bloque C7) y tener dos copias sería
// justo el fallo que RULES 0.6 prohibe. No cambia ni una línea de comportamiento.
#else
#define ViceExtCanAim(type, info) ((info)->IsFlagSet(WEAPONFLAG_CANAIM))
#endif

#ifdef VICEEXT_SHOTGUN_AIM
// Sección 2 (21/09, 5ª partida): el jugador reportó que apuntar con la escopeta
// "sólo muestra la mira pero el personaje no tiene animación de apuntar". El
// apuntado del motor elige la pose con `WEAPONFLAG_CANAIM_WITHARM`, que las
// escopetas NO traen en `weapon.dat`; sin ese flag cae en la rama de sólo girar
// el cuerpo. Aquí se le da la pose que tiene el mod (v1.5 "Changed aiming
// animations") sin tocar el dato: `weapon.dat` es de la sección 1.
static inline bool
ViceExtAimWithArm(eWeaponType type)
{
	switch (type) {
	case WEAPONTYPE_SHOTGUN:
	case WEAPONTYPE_STUBBY_SHOTGUN:
	case WEAPONTYPE_SPAS12_SHOTGUN:
	case WEAPONTYPE_SHOTGUN2:	// arma nueva del mod
	// R12 (9ª partida): el lanzagranadas del mod (`Gr_launch`, flags `28040`)
	// trae CANAIM pero NO `CANAIM_WITHARM`, así que apuntando con él no había
	// ni pose ni retícula: es el caso que el jugador describe como "armas
	// pesadas sin animación de apuntado". Se le da la pose que sí tienen las
	// escopetas (mismo grupo `buddy` en su weapon.dat).
	case WEAPONTYPE_GRENADE_LAUNCHER:
		return true;
	default:
		return false;
	}
}
#else
#define ViceExtAimWithArm(type) false
#endif

#ifdef VICEEXT_SWIMMING
static int32 s_odSwimSlot = 0;
#endif

#ifdef VICEEXT_CROUCH
// R29 (27/09, plan agachado-correcciones-axis, sintoma G): LA API UNICA DEL
// "ESTOY APUNTANDO". La rueda, la pose de apuntar, el giro y `CROUCH2 mira=`
// leen SOLO esta funcion; antes cada uno leia `GetTarget()` a su manera y
// discordaban (los 6 `skip motivo=sin-mira` del log del jugador).
//
// La ley la manda el port del ClassicAXIS desde que entro: su ley de apuntado
// (classicaxis Main.cpp:1230-1235) es `GetTarget()` + arma compatible +
// municion + modo de camara, y la publica en `CCamera::s_viceExtAimLawActive`
// (Camera.h:587, "ClassicAxis::isAiming"). Sin su ley se cae al predicado de
// siempre: `GetTarget()` + arma de apuntar (`ViceExtCanAim`: con punos no se
// apunta aunque el pad diga que si).
static const char *s_odAimWhy = "no";

bool
CPlayerPed::ViceExtIsAiming(void)
{
	if (bInVehicle || DyingOrDead()) {
		s_odAimWhy = bInVehicle ? "vehiculo" : "muerto";
		return false;
	}
#ifdef VICEEXT_SWIMMING
	if (ViceExtPedOwns(PEDLANE_NADO, PEDCAP_APUNTAR)) {
		s_odAimWhy = "nado";
		return false;
	}
#endif
	CPad *odPad = CPad::GetPad(0);
	if (odPad && odPad->GetSprint()) {
		s_odAimWhy = "esprint";
		return false;
	}
	if (odPad && odPad->JumpJustDown()) {
		s_odAimWhy = "salto";
		return false;
	}
	if (CCamera::s_viceExtAimLawActive) {
		s_odAimWhy = "ley";
		return true;
	}
	if (!odPad || !odPad->GetTarget()) {
		s_odAimWhy = "sin-target";
		return false;
	}
	eWeaponType odWep = GetWeapon()->m_eWeaponType;
	if (!ViceExtCanAim(odWep, CWeaponInfo::GetWeaponInfo(odWep))) {
		s_odAimWhy = "arma";
		return false;
	}
	s_odAimWhy = "ok";
	return true;
}
#endif

#ifdef __EMSCRIPTEN__
//
// WINFO/WCLIP (10ª partida, 21/09 noche). Motivo: el jugador reporta "las armas
// se apuntan mal, las pesadas no tienen animación de apuntado, no hay animación
// de recarga" y en el log no había NADA que dijera qué pose/QUÉ CLIP resuelve
// cada arma nueva. `AIMDIR ... peso=1.00` sólo dice que la asociación existe,
// no cuál es. Aquí se imprime, UNA vez por arma que el jugador saca:
//
//   - `flags` (hex, tal cual lo lee el motor de `weapon.dat`),
//   - `canaim` / `witharm`: las dos puertas que eligen la pose de apuntado
//     (`witharm` es la que levanta los brazos; sin ella el motor sólo gira el
//     cuerpo — la queja "sin animación de apuntado"),
//   - `reload` / `crouch`: si el arma trae el flag RELOAD (sin él no hay
//     animación de recarga posible en el motor de serie),
//   - `clips`: los NOMBRES de clip que el grupo del arma resuelve para
//     disparo/agachado/recarga. Si sale `-` o `?`, el clip no está en el `.ifp`
//     que sirve el juego y ése es el fallo, no la lógica.
//
// R23 (23/09, 19ª partida): ESTA FUNCIÓN TUMBABA EL JUEGO.
//
// El jugador: "al cargar la escopeta o no sé qué arma del inventario el juego
// crashea", con `RuntimeError: memory access out of bounds` en
// `memchr/strnlen/printf_core` llamados desde `ViceExtWeaponInfoTrace`. Es decir:
// un `%s` de esta función con un puntero BASURA.
//
// Causa, leída en el motor: `CAnimBlendAssocGroup::GetAnimation(uint32 id)` es
// `return &assocList[id - firstAnimId];` **sin comprobar el rango**. El id que se
// le pasa aquí es un `AnimationId` GLOBAL de otro bloque (p. ej. el clip de
// disparo agachado de un arma que NO lo tiene en su bloque): el índice se sale
// del array de asociaciones del grupo, el `&assocList[...]` apunta a memoria
// cualquiera y su `hierarchy->name` es basura → `snprintf` lee una dirección
// arbitraria y el wasm aborta. Nada de esto es culpa del arma: es el trazado el
// que pregunta por un clip que ese grupo no tiene.
//
// Arreglo: comprobar el rango contra la propia tabla del grupo
// (`firstAnimId`/`numAssociations`) ANTES de tocar la asociación, y devolver el
// nombre copiado y terminado en NUL (`hierarchy->name` es `char[24]` y puede no
// llevar terminador).
static const char *
ViceExtClipName(AssocGroupId group, AnimationId anim)
{
	// Cuatro nombres por línea de traza (disparo / agachado / recarga / recarga
	// agachado): cada llamada necesita SU buffer, así que se rota entre cuatro.
	static char s_odBuf[4][28];
	static int s_odNext = 0;
	if (anim == 0)
		return "-";
	if ((int)group < 0 || (int)group >= NUM_ANIM_ASSOC_GROUPS)
		return "?grupo";
	CAnimBlendAssocGroup *groups = CAnimManager::GetAnimAssocGroups();
	if (groups == nil || groups[group].assocList == nil)
		return "?";
	int idx = (int)anim - groups[group].firstAnimId;
	if (idx < 0 || idx >= groups[group].numAssociations)
		return "fuera";   // ese grupo NO tiene ese clip: no se toca la memoria
	CAnimBlendAssociation *assoc = &groups[group].assocList[idx];
	if (assoc->hierarchy == nil || assoc->hierarchy->name == nil)
		return "?";
	char *buf = s_odBuf[s_odNext];
	s_odNext = (s_odNext + 1) & 3;
	strncpy(buf, assoc->hierarchy->name, sizeof s_odBuf[0] - 1);
	buf[sizeof s_odBuf[0] - 1] = '\0';
	return buf;
}

static void
ViceExtWeaponInfoTrace(eWeaponType type, CWeaponInfo *info)
{
	static int s_odWiLast = -2;
	if (info == nil || (int)type == s_odWiLast)
		return;
	s_odWiLast = (int)type;
	AssocGroupId g = (AssocGroupId)info->m_AnimToPlay;
	char t[320];
	snprintf(t, sizeof t,
		"WINFO arma=%d grupo=%d flags=0x%x canaim=%d witharm=%d reload=%d crouchr=%d sight=%d clips=%s|%s|%s|%s",
		(int)type, (int)g, (unsigned)info->m_Flags,
		info->IsFlagSet(WEAPONFLAG_CANAIM) ? 1 : 0,
		(info->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM) || ViceExtAimWithArm(type)) ? 1 : 0,
		CPed::GetReloadAnim(info) ? 1 : 0, CPed::GetCrouchReloadAnim(info) ? 1 : 0,
#ifdef VICEEXT_WEAPON_SIGHTS
		info->m_nSight,
#else
		-1,
#endif
		ViceExtClipName(g, ANIM_WEAPON_FIRE),
		ViceExtClipName(g, CPed::GetCrouchFireAnim(info)),
		ViceExtClipName(g, CPed::GetReloadAnim(info)),
		ViceExtClipName(g, CPed::GetCrouchReloadAnim(info)));
	ODTRACES(t);
}
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
#ifdef VICEEXT_AIM_CLASSICAXIS
	ViceExtMoveReset();
#endif
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
}

void
CPlayerPed::ClearWeaponTarget()
{
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

	GetWeapon()->m_nAmmoInClip = Min(GetWeapon()->m_nAmmoTotal, CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->m_nAmountofAmmunition);

	if (CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->IsFlagSet(WEAPONFLAG_CANAIM))
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

#ifdef VICEEXT_AIM_CLASSICAXIS
	ViceExtMoveReset();
#endif
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
	// R6b (sección 1, revisión del trabajo de R6): agachado, el clip lo pone
	// `ViceExtCrouchAnim`. Sin este corte, el selector de animación de serie
	// elegía caminar/correr por `m_fMoveSpeed` y PISABA el clip de agachado que
	// se había mezclado al principio del frame: el jugador quedaba agachado
	// medio frame y de pie el resto ("se agacha a medias / desaparece al andar").
	// El movimiento lo sigue llevando el motor (m_fMoveSpeed + m_fRotationCur),
	// que es lo que R6 quería: cámara siguiendo al ped y avance hacia donde mira.
	if (odCrouched) {
		ViceExtCrouchAnim();
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
				if (m_fCurrentStamina < 0.0f && !bIsAimingGun && !CWorld::TestSphereAgainstWorld(GetPosition(), 0.5f,
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
				if (m_fCurrentStamina < 0.0f && !bIsAimingGun && !CWorld::TestSphereAgainstWorld(GetPosition(), 0.5f,
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
				if (m_fCurrentStamina < 0.0f && !bIsAimingGun && curIdleAssoc->animId != ANIM_STD_IDLE_TIRED
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
#ifdef __EMSCRIPTEN__
					// Sección 3, C6 (20/09): traza del esprint con arma pesada. El grupo
					// dice qué clip se está usando en el hueco RUNFAST (con el arreglo
					// de la tabla, el de 2 manos es `sprint_armed`). Sólo al entrar en
					// esprint (transición), no por frame.
					{
						char t[130];
						snprintf(t, sizeof t, "VICEEXT sprint grupo=%d arma=%d pesada=%d",
							(int)m_animGroup, (int)GetWeapon()->m_eWeaponType,
							(int)CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->IsFlagSet(WEAPONFLAG_HEAVY));
						ODTRACES(t);
					}
#endif
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
#ifdef VICEEXT_AIM_WALK
				// H4 (12ª partida, 22/09): CAMINADO APUNTANDO = el clip de andar
				// normal, más lento. Lo que pasa hoy sin esto:
				//
				// Apuntando, el tope de velocidad de R9 deja `m_fMoveSpeed` en 1,0
				// o menos, y ESTE bloque elige el clip por esa velocidad. En Vice City
				// el clip de andar normal (el jog que se ve al empujar el palo) es
				// `ANIM_STD_RUN`; `ANIM_STD_WALK` es el paso lento. O sea: apuntando
				// se caía al clip lento. El jugador lo describe así: "el caminado al
				// apuntar es un caminado de Tommy que es cuando camina en cinemáticas
				// o tranquilo; cuando se apunta es la misma animación como cuando
				// solo presionas moverte pero más lenta en velocidad".
				//
				// Por eso el clip lo elige la velocidad SIN tope (la del movimiento
				// normal) y la cadencia se escala con el cociente real (0,5 con el
				// tope a la mitad): mismo caminado, la mitad de rápido, y los pies
				// acompañan al avance (sin patinar).
				if (odAimWalkActive && odAimWalkUncapped > 0.05f) {
					float odU = odAimWalkUncapped;
					if (odU < 1.0f) {
						curWalkAssoc->blendAmount = 1.0f;
						curRunAssoc->blendAmount = 0.0f;
					} else if (odU < 2.0f) {
						curWalkAssoc->blendAmount = 2.0f - odU;
						curRunAssoc->blendAmount = odU - 1.0f;
					} else {
						curWalkAssoc->blendAmount = 0.0f;
						curRunAssoc->blendAmount = 1.0f;
					}
					float odRatio = m_fMoveSpeed / odU;   // 1,0 = sin tope; 0,5 = la mitad
					if (odRatio > 1.0f) odRatio = 1.0f;
					if (odRatio < 0.35f) odRatio = 0.35f;
					if (curWalkAssoc) curWalkAssoc->speed = odRatio;
					if (curRunAssoc) curRunAssoc->speed = odRatio;
				} else {
					if (curWalkAssoc) curWalkAssoc->speed = 1.0f;
					if (curRunAssoc) curRunAssoc->speed = 1.0f;
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
#ifdef VICEEXT_AIM_CLASSICAXIS
	// PORTADO — ClassicAXIS (MIT, © 2022 Classic Axis VC Team)
	//   gta_vc_browser/tmp/extsrc/classicaxis_Main.cpp:387-404
	//   («Weapon smooth spray» — RedirectCall a `doWeaponSmoothSpray`, VC 0x535F4C)
	// Qué se toma: el mod SUSTITUYE la función entera por dos valores —
	//   `0.00001f` si `bIsDucking` y `-1.0f` si no. Es decir, con
	//   ClassicAXIS el spray suave del motor (el `switch` de armas de debajo, con
	//   `PI/112` para pistolas, `PI/176`, `PI/80` para motosierra...) NO se usa: solo
	//   queda un residuo minísimo al agachado, que es lo que evita el tiron lateral.
	//   Antes de este cambio este motor aplicaba su propio `switch`, que es el
	//   "enfoque totalmente erróneo" del que habló el jugador.
	// Adaptación: se devuelve antes del `switch`; el resto de la función se
	//   conserva intacto para cuando `VICEEXT_AIM_CLASSICAXIS` esté apagado.
	// Medible: criterio PASS = con pistola, el disparo no mete tiron lateral al
	//   girar la cámara (FALLO si el cuerpo se va al disparar), y agachado el
	//   residuo es imperceptible.
	if (bIsDucking)
		return 0.00001f;
	return -1.0f;
#else
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
			case WEAPONTYPE_DESERT_EAGLE:	// Vice Extended
				return PI / 112.f;
			case WEAPONTYPE_SHOTGUN:
			case WEAPONTYPE_SPAS12_SHOTGUN:
			case WEAPONTYPE_STUBBY_SHOTGUN:
			case WEAPONTYPE_SHOTGUN2:	// Vice Extended
				return PI / 112.f;
			case WEAPONTYPE_UZI:
			case WEAPONTYPE_MP5:
			case WEAPONTYPE_UZIOLD:	// Vice Extended
				return PI / 112.f;
			case WEAPONTYPE_M4:
			case WEAPONTYPE_RUGER:
			case WEAPONTYPE_AK47:	// Vice Extended
			case WEAPONTYPE_M16:
			case WEAPONTYPE_STEYR:
				return PI / 112.f;
			case WEAPONTYPE_FLAMETHROWER:
				return PI / 80.f;
			case WEAPONTYPE_M60:
			case WEAPONTYPE_MINIGUN:
			case WEAPONTYPE_HELICANNON:
			case WEAPONTYPE_GRENADE_LAUNCHER:	// Vice Extended
				return PI / 176.f;
			default:
				return -1.0f;
		}
	} else if (bIsDucking)
		return PI / 112.f;
	else
		return -1.0f;
#endif
}

// Vice Extended v1.5 ("Sprint with heavy weapons"): en el juego original las
// armas pesadas (WEAPONFLAG_HEAVY: lanzacohetes, M60, minigun, lanzallamas...)
// bloquean el esprint; su mod lo permite. Un solo sitio para la condición.
bool
CPlayerPed::CanSprintWithCurrentWeapon(void)
{
#ifdef VICEEXT_SPRINT_HEAVY
	return true;
#else
	return !CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->IsFlagSet(WEAPONFLAG_HEAVY);
#endif
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

	if (VICEEXT_ENGINE_DUCK_KEY() && !bIsDucking && m_nMoveState != PEDMOVE_SPRINT) {
		bCrouchWhenShooting = true;
		SetDuck(60000, true);
	} else if (bIsDucking && (VICEEXT_ENGINE_DUCK_KEY() || m_nMoveState == PEDMOVE_SPRINT)) {
		ClearDuck(true);
		bCrouchWhenShooting = false;
	}

	if (!padUsed->GetTarget() && !m_attachedTo) {
		RestorePreviousState();
		TheCamera.ClearPlayerWeaponMode();
		return;
	}

	int firingRate = GetWeapon()->m_eWeaponType == WEAPONTYPE_LASERSCOPE ? 333 : 266;
	if (padUsed->WeaponJustDown() && CTimer::GetTimeInMilliseconds() > GetWeapon()->m_nTimer) {
		CVector firePos(0.0f, 0.0f, 0.6f);
		firePos = GetMatrix() * firePos;
				// ClassicAXIS C10 (Main.cpp:159-170): el mod mete MODE_FOLLOW_PED mientras
		// dura CWeapon::Fire y lo devuelve al salir, para que el disparo no rompa
		// el mvl ni los parabrisas rompibles (el bug que el mod arregla). Aqui es
		// un par de lineas alrededor de la llamada, que es el unico sitio donde
		// sabemos cuando empieza y cuando acaba. El guardia `s_viceExtAimLawActive`
		// es lo que hace que FUERA de apuntar no cambie nada.
		int16 odSavedMode = TheCamera.Cams[TheCamera.ActiveCam].Mode;
		if (CCamera::s_viceExtAimLawActive && odSavedMode != CCam::MODE_FOLLOWPED)
			TheCamera.Cams[TheCamera.ActiveCam].Mode = CCam::MODE_FOLLOWPED;
		GetWeapon()->Fire(this, &firePos);
		if (CCamera::s_viceExtAimLawActive && odSavedMode != CCam::MODE_FOLLOWPED)
			TheCamera.Cams[TheCamera.ActiveCam].Mode = odSavedMode;
		m_nPadDownPressedInMilliseconds = CTimer::GetTimeInMilliseconds();
	} else if (CTimer::GetTimeInMilliseconds() > m_nPadDownPressedInMilliseconds + firingRate &&
		CTimer::GetTimeInMilliseconds() - CTimer::GetTimeStepInMilliseconds() < m_nPadDownPressedInMilliseconds + firingRate && padUsed->GetWeapon()) {
		
		if (GetWeapon()->m_nAmmoTotal > 0) {
			DMAudio.PlayFrontEndSound(SOUND_WEAPON_AK47_BULLET_ECHO, GetWeapon()->m_eWeaponType);
		}
	}
	GetWeapon()->Update(m_audioEntityId, nil);
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

	// ClassicAXIS C8 (Main.cpp:372-384): con el apuntado activo no se cambia de arma.
	// El motor ya bloquea por `bDontAllowWeaponChange`, pero SOLO cuando hay fijado
	// (PlayerPed.cpp:1461); con raton y §5.2(a) no hay fijado nunca, asi que sin esta
	// linea la rueda cambiaria de arma mientras apuntas. El `bDontAllowWeaponChange =
	// false` de mas abajo (cuando `!GetTarget()`) es lo que lo devuelve.
	if (CCamera::s_viceExtAimLawActive)
		bDontAllowWeaponChange = true;
#ifdef __EMSCRIPTEN__
#ifdef __EMSCRIPTEN__
	static bool s_odWpnWas = false;
#endif
	if (CCamera::s_viceExtAimLawActive) {
		if (!s_odWpnWas) {
			s_odWpnWas = true;
			char t[60];
			snprintf(t, sizeof t, "AIMWPN block=1 arma=%d", (int)GetWeapon()->m_eWeaponType);
			ODTRACES(t);
		}
	} else
		s_odWpnWas = false;
#endif

	if (!m_pPointGunAt && !bDontAllowWeaponChange && GetWeapon()->m_eWeaponType != WEAPONTYPE_DETONATOR) {
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
		if (m_nPedState != PED_ATTACK && m_nPedState != PED_AIM_GUN && m_nPedState != PED_FIGHT) {
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

	if (VICEEXT_ENGINE_DUCK_KEY() && !bIsDucking && m_nMoveState != PEDMOVE_SPRINT) {
		bCrouchWhenShooting = true;
		SetDuck(60000, true);
	} else if (bIsDucking && (VICEEXT_ENGINE_DUCK_KEY() || m_nMoveState == PEDMOVE_SPRINT)) {
		ClearDuck(true);
		bCrouchWhenShooting = false;
	}

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
					// ClassicAXIS C10 (Main.cpp:159-170): el mod mete MODE_FOLLOW_PED mientras
		// dura CWeapon::Fire y lo devuelve al salir, para que el disparo no rompa
		// el mvl ni los parabrisas rompibles (el bug que el mod arregla). Aqui es
		// un par de lineas alrededor de la llamada, que es el unico sitio donde
		// sabemos cuando empieza y cuando acaba. El guardia `s_viceExtAimLawActive`
		// es lo que hace que FUERA de apuntar no cambie nada.
		int16 odSavedMode = TheCamera.Cams[TheCamera.ActiveCam].Mode;
		if (CCamera::s_viceExtAimLawActive && odSavedMode != CCam::MODE_FOLLOWPED)
			TheCamera.Cams[TheCamera.ActiveCam].Mode = CCam::MODE_FOLLOWPED;
		GetWeapon()->Fire(this, &firePos);
		if (CCamera::s_viceExtAimLawActive && odSavedMode != CCam::MODE_FOLLOWPED)
			TheCamera.Cams[TheCamera.ActiveCam].Mode = odSavedMode;
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

#ifdef VICEEXT_AIM_CLASSICAXIS
// PORTADO — ClassicAXIS (MIT, © 2022 Classic Axis VC Team)
//   gta_vc_browser/tmp/extsrc/classicaxis_Main.cpp:121-151
//   («playerMovementType» / «playerShootingDirection» — redirect de 6+4
//    callsites al tipo de locomoción del juego)
// Qué se toma: los dos redirects. El juego saca el tipo de locomoción de la
//   cámara de ratón; el mod lo saca de si se está APUNTANDO y devuelve
//   `TYPE_WALKAROUND` (el cuerpo gira hacia donde camina) en el 99% de los casos,
//   y `TYPE_STRAFE` (de costado) sólo apuntando. De ahí las dos frases del
//   jugador: A/D solo => el cuerpo se encara hacia donde camina; con el ratón y
//   el cuerpo quieto => gira solo, y se puede dar el 360°.
// Adaptación: aquí no hay `TYPE_STRAFE`/`TYPE_WALKAROUND` (eso es del SDK del
//   mod). Se traduce a las dos decisiones que en reVC producen el mismo efecto:
//   (1) `m_fRotationDest` — hacia dónde camina, o hacia la cámara; y
//   (2) `ProcessAnimGroups` — clips de costado (`ASSOCGRP_PLAYERLEFT/RIGHT/BACK`)
//       o clips de recto.
//   El `!m_bHasLockOnTarget` de `playerShootingDirection` se respeta: con
//   auto-fijado (`m_bHasLockOnTarget`, que es `m_pPointGunAt != nil`, línea 1916)
//   el mod deja `TYPE_WALKAROUND`, o sea el cuerpo mira al objetivo y no de costado.
// Medible: criterio PASS = con A/D y sin mover el ratón el cuerpo se encara hacia
//   la dirección de marcha (PASS) en vez de quedarse mirando a la cámara (FALLO);
//   y con A/D + ratón + apuntado, clips `PLAYERLEFT`/`PLAYERRIGHT` (PASS) en vez de
//   los de recto (FALLO).
static bool
ViceExtIsAimingNow(CPlayerPed *ped, CPad *pad)
{
	if (!pad || !pad->GetTarget())
		return false;
#ifdef VICEEXT_SWIMMING
	if (ViceExtPedOwns(PEDLANE_NADO, PEDCAP_APUNTAR))
		return false;
#endif
	CWeapon *w = ped->GetWeapon();
	if (w == nil)
		return false;
	return ViceExtCanAim(w->m_eWeaponType, CWeaponInfo::GetWeaponInfo(w->m_eWeaponType));
}

struct ViceExtMoveState {
	CPlayerPed *owner;
	uint32 gen;
	uint32 frame;
	bool prepared;
	bool hasBase;
	float baseX;
	float baseY;
	float dirX;
	float dirY;
	float mag;
	float speedGame;
	float lr;
	float ud;
	int policy;
	int aim;
	int crouch;
	bool moving;
	bool sprint;
};

static ViceExtMoveState s_odMove = { nil, 0, 0, false, false, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, false, false };

static void
ViceExtMoveReset(void)
{
	s_odMove.owner = nil;
	s_odMove.prepared = false;
	s_odMove.hasBase = false;
	s_odMove.moving = false;
	s_odMove.dirX = 0.0f;
	s_odMove.dirY = 0.0f;
	s_odMove.mag = 0.0f;
	s_odMove.speedGame = 0.0f;
	s_odMove.policy = 0;
	++s_odMove.gen;
}

static bool
ViceExtMovePrepared(const CPlayerPed *ped)
{
	if (!s_odMove.prepared || s_odMove.owner != ped)
		return false;
	return s_odMove.frame == CTimer::GetFrameCounter();
}

static float
ViceExtMoveHeading(void)
{
	if (!s_odMove.prepared || !s_odMove.moving)
		return 0.0f;
	return CGeneral::LimitRadianAngle(Atan2(-s_odMove.dirX, s_odMove.dirY));
}

static bool
ViceExtMoveHeadingOwned(void)
{
	return s_odMove.prepared && s_odMove.moving;
}

#ifdef VICEEXT_CROUCH
#define ODROLL_READY 0
#define ODROLL_ACTIVE 1
static int s_odRollState = ODROLL_READY;
static bool s_odRollArmed = false;
static bool s_odRollNeutral = true;
static bool s_odRollPrevIzq = false;
static bool s_odRollPrevDer = false;
static AnimationId s_odRollClip = ANIM_STD_CROUCH_IDLE;
static uint32 s_odRollId = 0;
static int s_odRollSide = 0;
static float s_odRollDirX = 0.0f;
static float s_odRollDirY = 0.0f;
static float s_odRollSpeedMps = 0.0f;
static float s_odRollLength = 0.0f;
static float s_odRollSpeed = 0.0f;
static float s_odRollLastCurrent = 0.0f;
static float s_odRollRate = 0.0f;
static float s_odRollMaxW = 0.0f;
static CVector s_odRollStepPos(0.0f, 0.0f, 0.0f);
static float s_odRollStepM = 0.0f;
static float s_odRollStepT = 0.0f;
static uint32 s_odRollStepN = 0;
static uint32 s_odRollEdgeSeq = 0;
static uint32 s_odRollFrame = 0;
static float s_odRollSimStart = 0.0f;
#endif

static float ViceExtWalkerTargetMps(void);

#ifdef __EMSCRIPTEN__
static uint32 s_odP4PoseSpan = 1;
static uint32 s_odP4PoseEnterId = 0;
static uint32 s_odP4PoseExitId = 0;

static void ViceExtP4PoseExitSampler(CPlayerPed *ped);
#endif
static bool s_odMovePrevMoving = false;
static uint32 s_odMoveStartupMs = 0;
static int s_odStickBlocked = 0;
static AnimationId ViceExtCrouchAimPoseClip(CPlayerPed *ped);

void
CPlayerPed::ViceExtPrepareMove(CPad *padUsed)
{
	s_odMove.owner = this;
	s_odMove.frame = CTimer::GetFrameCounter();
	s_odMove.prepared = true;
	s_odMove.speedGame = 0.0f;
	s_odMove.lr = padUsed ? padUsed->GetPedWalkLeftRight() : 0.0f;
	s_odMove.ud = padUsed ? padUsed->GetPedWalkUpDown() : 0.0f;
	CVector odF = TheCamera.GetMatrix().GetForward();
	float odFx = odF.x;
	float odFy = odF.y;
	float odFlen = Sqrt(odFx * odFx + odFy * odFy);
	if (odFlen > 0.0001f) {
		odFx /= odFlen;
		odFy /= odFlen;
		s_odMove.baseX = odFx;
		s_odMove.baseY = odFy;
		s_odMove.hasBase = true;
	} else if (s_odMove.hasBase) {
		odFx = s_odMove.baseX;
		odFy = s_odMove.baseY;
	} else {
		odFx = -Sin(m_fRotationCur);
		odFy = Cos(m_fRotationCur);
		s_odMove.baseX = odFx;
		s_odMove.baseY = odFy;
		s_odMove.hasBase = true;
	}
	float odUx = odFx * (-s_odMove.ud) + odFy * s_odMove.lr;
	float odUy = odFy * (-s_odMove.ud) + (-odFx) * s_odMove.lr;
	float odMag = Sqrt(odUx * odUx + odUy * odUy);
	if (odMag > 0.0001f) {
		float odMaxComp = Max(Abs(s_odMove.lr), Abs(s_odMove.ud));
		if (odMaxComp > 0.0f && odMag > odMaxComp) {
			odUx *= odMaxComp / odMag;
			odUy *= odMaxComp / odMag;
			odMag = odMaxComp;
		}
		s_odMove.dirX = odUx / odMag;
		s_odMove.dirY = odUy / odMag;
	} else {
		s_odMove.dirX = 0.0f;
		s_odMove.dirY = 0.0f;
	}
	s_odMove.mag = odMag;
	s_odMove.moving = odMag > 16.0f;
	s_odMove.aim = ViceExtIsAiming() ? 1 : 0;
#ifdef VICEEXT_CROUCH
	s_odMove.crouch = odCrouched ? 1 : 0;
#else
	s_odMove.crouch = 0;
#endif
	s_odMove.sprint = padUsed != nil && padUsed->GetSprint();
	if (CCamera::s_viceExtAimForceRealMoveAnim) {
		CCamera::s_viceExtAimForceRealMoveAnim = false;
		s_odMove.policy = 0;
	} else {
		s_odMove.policy = s_odMove.aim ? 1 : 0;
	}
#ifdef VICEEXT_AIM_CLASSICAXIS
	{
		uint32 odNowMs = CTimer::GetTimeInMilliseconds();
		if (s_odMove.moving && !s_odMovePrevMoving)
			s_odMoveStartupMs = odNowMs + 250;
		s_odMovePrevMoving = s_odMove.moving;
		if (s_odMove.moving && !s_odMove.sprint && !bInVehicle
			&& ((s_odMove.policy == 0 && s_odMoveStartupMs != 0 && odNowMs <= s_odMoveStartupMs)
				|| s_odMove.policy == 1))
			s_odMove.speedGame = (odCrouched ? ViceExtCrouchMoveSpeed() : ViceExtWalkerTargetMps()) * METERS_PER_SECOND_TO_GAME_SPEED;
	}
#endif
#ifdef VICEEXT_CROUCH
	if (s_odRollState == ODROLL_ACTIVE) {
		s_odMove.dirX = s_odRollDirX;
		s_odMove.dirY = s_odRollDirY;
		s_odMove.moving = true;
		s_odMove.speedGame = s_odRollSpeedMps * METERS_PER_SECOND_TO_GAME_SPEED;
	}
#endif
	bool odOk = !bInVehicle && !DyingOrDead() && !CReplay::IsPlayingBack();
#ifdef VICEEXT_SWIMMING
	if (ViceExtIsSwimming())
		odOk = false;
#endif
	s_odMove.prepared = odOk;
	if (odOk && s_odMove.moving && s_odMove.policy == 0 && !TheCamera.Using1stPersonWeaponMode())
		m_fRotationDest = ViceExtMoveHeading();
#ifdef __EMSCRIPTEN__
	ViceExtP4PoseExitSampler(this);
#endif
}

bool
CPlayerPed::ViceExtMoveWalkaround(void) const
{
	return s_odMove.prepared && s_odMove.moving && s_odMove.policy == 0;
}

int
CPlayerPed::ViceExtStickBlocked(void) const
{
	return s_odStickBlocked;
}

bool
CPlayerPed::ViceExtGetMove(CVector2D &direction, float &speedGame) const
{
	if (!ViceExtMovePrepared(this))
		return false;
	if (!s_odMove.moving)
		return false;
	direction = CVector2D(s_odMove.dirX, s_odMove.dirY);
	speedGame = s_odMove.speedGame;
	return true;
}

#ifdef __EMSCRIPTEN__
#define VICEEXT_P4_EMIT(odGen, odBuf, ...) do { int odP4_ret = snprintf(odBuf, sizeof(odBuf), __VA_ARGS__); if (odP4_ret < 0 || odP4_ret >= (int)sizeof(odBuf)) { char odP4_of[220]; snprintf(odP4_of, sizeof odP4_of, "P4 kind=overflow schema=1 gen=%u frame=%u sim=%.4f case=0 id=0 cap=%d ret=%d", (unsigned)(odGen), (unsigned)CTimer::GetFrameCounter(), CTimer::GetTimeInMilliseconds() * 0.001f, (int)sizeof(odBuf), odP4_ret); ODTRACES(odP4_of); } else { ODTRACES(odBuf); } } while (0)
static void
ViceExtP4Identity(uint32 gen, uint32 frame, float sim)
{
	static uint32 s_odIdentMs = 0;
	uint32 odNow = CTimer::GetTimeInMilliseconds();
	if (s_odIdentMs != 0 && odNow < s_odIdentMs + 60000 && odNow + 60000 >= s_odIdentMs)
		return;
	s_odIdentMs = odNow;
	EM_ASM({
		try {
			var v = window.__vcVersion || (window.OD && window.OD.buildTag) || '?';
			var d = (window.OD && window.OD.dataTag) || '?';
			(window.__odq = window.__odq || []).push('P4 kind=identity schema=1 gen=' + $0 + ' frame=' + $1 + ' sim=' + $2.toFixed(4) + ' case=0 id=0 version=' + v + ' data=' + d);
		} catch (e) {}
	}, (int)gen, (int)frame, (double)sim);
}

struct ViceExtMoveSpan {
	bool active;
	uint32 id;
	float simStart;
	float dist;
	float dirErrMax;
	float bodyErrMax;
	int simSamples;
	int caseId;
	int gaps;
	float lr, ud;
	float reqX, reqY;
	float baseFx, baseFy, baseRx, baseRy;
	int aim, crouch, policy, weapon, sprint, walk;
	float declSum;
};

static ViceExtMoveSpan s_odSpan;
static float s_odSpanHeldSince = 0.0f;
static uint32 s_odSpanKey = 0;
static uint32 s_odP4EdgeSeq = 0;
static bool s_odP4PrevIzq = false;
static bool s_odP4PrevDer = false;
static float s_odP4HoldStart = 0.0f;
static bool s_odP4Holding = false;
static int s_odP4HoldSide = 0;

static float
ViceExtP4AngleDeg(float ax, float ay, float bx, float by)
{
	float odNa = Sqrt(ax * ax + ay * ay);
	float odNb = Sqrt(bx * bx + by * by);
	if (odNa < 1e-6f || odNb < 1e-6f)
		return 0.0f;
	float odDot = (ax * bx + ay * by) / (odNa * odNb);
	if (odDot > 1.0f) odDot = 1.0f;
	if (odDot < -1.0f) odDot = -1.0f;
	return Acos(odDot) * RADTODEG(1.0f);
}

static bool
ViceExtP4WalkDown(void)
{
	RsKeyCodes odWalk = (RsKeyCodes)ControlsManager.GetControllerKeyAssociatedWithAction(PED_WALK, KEYBOARD);
	return odWalk != (RsKeyCodes)0 && ControlsManager.GetIsKeyboardKeyDown(odWalk);
}

static void
ViceExtP4MoveEnd(float odSim, uint32 odFrame)
{
	if (!s_odSpan.active)
		return;
	float odElapsed = odSim - s_odSpan.simStart;
	float odTarget = (s_odSpan.simSamples > 0) ? (s_odSpan.declSum / s_odSpan.simSamples) : 0.0f;
	float odReal = (odElapsed > 0.0001f) ? (s_odSpan.dist / odElapsed) : 0.0f;
	char t[560];
	VICEEXT_P4_EMIT(s_odMove.gen, t,  "P4 kind=move_end schema=1 gen=%u frame=%u sim=%.4f case=%d id=%u elapsed=%.4f dist=%.4f req=%.6f,%.6f,0.000000 baseF=%.6f,%.6f,0.000000 baseR=%.6f,%.6f,0.000000 dirErrMax=%.4f bodyErrMax=%.4f simSamples=%d gaps=%d speedTarget=%.4f speedReal=%.4f",
		(unsigned)s_odMove.gen, (unsigned)odFrame, odSim, s_odSpan.caseId, (unsigned)s_odSpan.id,
		odElapsed, s_odSpan.dist, s_odSpan.reqX, s_odSpan.reqY,
		s_odSpan.baseFx, s_odSpan.baseFy, s_odSpan.baseRx, s_odSpan.baseRy,
		s_odSpan.dirErrMax, s_odSpan.bodyErrMax, s_odSpan.simSamples, s_odSpan.gaps,
		odTarget, odReal);
	
	s_odSpan.active = false;
}

#ifdef __EMSCRIPTEN__
static int s_odPoseExitSample = -1;

static void
ViceExtP4PoseExitSampler(CPlayerPed *ped)
{
	if (s_odPoseExitSample < 0)
		return;
	if (s_odPoseExitSample >= 30) {
		s_odPoseExitSample = -1;
		return;
	}
	int odN = 0;
	int odWorst = -1;
	float odW = 0.0f;
	for (int i = 0; i < 9; i++) {
		CAnimBlendAssociation *odA = RpAnimBlendClumpGetAssociation(ped->GetClump(), odCrouchPoseIds[i]);
		if (odA == nil || odA->blendAmount <= 0.01f)
			continue;
		++odN;
		if (odA->blendAmount > odW) {
			odW = odA->blendAmount;
			odWorst = i;
		}
	}
	float odDelta = 0.0f;
	float odSpeed = 0.0f;
	unsigned odFlags = 0;
	if (odWorst >= 0) {
		CAnimBlendAssociation *odA = RpAnimBlendClumpGetAssociation(ped->GetClump(), odCrouchPoseIds[odWorst]);
		if (odA != nil) {
			odDelta = odA->blendDelta;
			odSpeed = odA->speed;
			odFlags = (unsigned)odA->flags;
		}
	}
	float odPesoSum = 0.0f;
	for (CAnimBlendAssociation *odM = RpAnimBlendClumpGetFirstAssociation(ped->GetClump());
	     odM; odM = RpAnimBlendGetNextAssociation(odM)) {
		if (odM->IsPartial())
			continue;
		if (odM->blendAmount > 0.0f)
			odPesoSum += odM->blendAmount;
	}
	char t[260];
	VICEEXT_P4_EMIT(s_odMove.gen, t, "P4 kind=pose_exit schema=1 gen=%u frame=%u sim=%.4f case=3 id=%u n=%d sample=%d clip=%d w=%.4f delta=%.4f speed=%.4f flags=%x pesosum=%.2f",
		(unsigned)s_odMove.gen, (unsigned)CTimer::GetFrameCounter(),
		CTimer::GetTimeInMilliseconds() * 0.001f, (unsigned)s_odP4PoseExitId,
		odN, s_odPoseExitSample,
		odWorst >= 0 ? (int)odCrouchPoseIds[odWorst] : 0, odW, odDelta, odSpeed, odFlags, odPesoSum);
	++s_odPoseExitSample;
}
#endif

static void
ViceExtMoveTick(CPlayerPed *ped)
{
	float odSim = CTimer::GetTimeInMilliseconds() * 0.001f;
	uint32 odFrame = CTimer::GetFrameCounter();
	ViceExtP4Identity(s_odMove.gen, odFrame, odSim);

	CPad *odPad = GetPadFromPlayer(ped);
	float odLR = odPad ? odPad->GetPedWalkLeftRight() : 0.0f;
	float odUD = odPad ? odPad->GetPedWalkUpDown() : 0.0f;
	bool odIzq = Abs(odUD) <= 16.0f && odLR <= -16.0f;
	bool odDer = Abs(odUD) <= 16.0f && odLR >= 16.0f;
	bool odNeutral = Abs(odLR) <= 16.0f && Abs(odUD) <= 16.0f;
	int odWeapon = (ped->GetWeapon() != nil) ? (int)ped->GetWeapon()->m_eWeaponType : 0;
	int odAim = ped->ViceExtIsAiming() ? 1 : 0;
	const char *odAimWhy = s_odAimWhy;
	int odCrouch = 0;
#ifdef VICEEXT_CROUCH
	odCrouch = odCrouched ? 1 : 0;
#endif
	int odWalk = ViceExtP4WalkDown() ? 1 : 0;
	int odSprint = (odPad && odPad->GetSprint()) ? 1 : 0;
	int odFire = (odPad && odPad->GetWeapon()) ? 1 : 0;
	int odRearm = 0;
	int odEligible = 0;
#ifdef VICEEXT_CROUCH
	odRearm = s_odRollArmed ? 1 : 0;
	if (odAim && odCrouch && s_odRollState == ODROLL_READY && s_odRollArmed
	    && odWeapon >= 17 && !(odWeapon >= 28 && odWeapon <= 33) && !odFire)
		odEligible = 1;
#endif
	int odJump = (odPad && odPad->JumpJustDown()) ? 1 : 0;
	{
		static int s_odAimWas = -1;
		static int s_odAimWasSprint = -1;
		static int s_odAimWasJump = -1;
		static const char *s_odAimWasWhy = nil;
		static float s_odAimWasSim = 0.0f;
		bool odAimChange = (odAim != s_odAimWas) || (odSprint != s_odAimWasSprint)
			|| (odJump != s_odAimWasJump) || (odAimWhy != s_odAimWasWhy)
			|| odSim - s_odAimWasSim >= 1.0f;
		if (odAimChange) {
			s_odAimWas = odAim;
			s_odAimWasSprint = odSprint;
			s_odAimWasJump = odJump;
			s_odAimWasWhy = odAimWhy;
			s_odAimWasSim = odSim;
			char t[300];
			VICEEXT_P4_EMIT(s_odMove.gen, t, "P4 kind=aim schema=1 gen=%u frame=%u sim=%.4f case=4 id=0 why=%s aim=%d sprint=%d jump=%d crouch=%d policy=%d speedGame=%.4f weapon=%d",
				(unsigned)s_odMove.gen, (unsigned)odFrame, odSim, odAimWhy, odAim, odSprint, odJump, odCrouch, s_odMove.policy, s_odMove.speedGame, odWeapon);
		}
	}

	if (!s_odP4PrevIzq && odIzq) {
		char odEdge[24];
		char t[420];
		snprintf(odEdge, sizeof odEdge, "A%u", (unsigned)++s_odP4EdgeSeq);
		VICEEXT_P4_EMIT(s_odMove.gen, t,  "P4 kind=input_edge schema=1 gen=%u frame=%u sim=%.4f case=5 id=0 lr=%.2f ud=%.2f aim=%d crouch=%d weapon=%d fire=%d side=L edge=%s neutral=%d eligible=%d",
			(unsigned)s_odMove.gen, (unsigned)odFrame, odSim, odLR, odUD, odAim, odCrouch, odWeapon, odFire, odEdge, odRearm, odEligible);
		
		s_odP4HoldStart = odSim;
		s_odP4Holding = true;
		s_odP4HoldSide = -1;
	}
	if (!s_odP4PrevDer && odDer) {
		char odEdge[24];
		char t[420];
		snprintf(odEdge, sizeof odEdge, "D%u", (unsigned)++s_odP4EdgeSeq);
		VICEEXT_P4_EMIT(s_odMove.gen, t,  "P4 kind=input_edge schema=1 gen=%u frame=%u sim=%.4f case=5 id=0 lr=%.2f ud=%.2f aim=%d crouch=%d weapon=%d fire=%d side=R edge=%s neutral=%d eligible=%d",
			(unsigned)s_odMove.gen, (unsigned)odFrame, odSim, odLR, odUD, odAim, odCrouch, odWeapon, odFire, odEdge, odRearm, odEligible);
		
		s_odP4HoldStart = odSim;
		s_odP4Holding = true;
		s_odP4HoldSide = 1;
	}
	if (s_odP4Holding && ((s_odP4HoldSide < 0 && !odIzq) || (s_odP4HoldSide > 0 && !odDer))) {
		char t[320];
		VICEEXT_P4_EMIT(s_odMove.gen, t,  "P4 kind=input_hold_end schema=1 gen=%u frame=%u sim=%.4f case=5 id=0 lr=%.2f ud=%.2f aim=%d crouch=%d side=%s heldSim=%.4f",
			(unsigned)s_odMove.gen, (unsigned)odFrame, odSim, odLR, odUD, odAim, odCrouch,
			s_odP4HoldSide < 0 ? "L" : "R", odSim - s_odP4HoldStart);
		
		s_odP4Holding = false;
		s_odP4HoldSide = 0;
	}
	s_odP4PrevIzq = odIzq;
	s_odP4PrevDer = odDer;

	bool odMoving = ViceExtMovePrepared(ped) && s_odMove.moving;
	uint32 odKey = ((uint32)(int)odLR << 24) ^ ((uint32)(int)odUD << 16) ^ ((uint32)odAim << 12)
		^ ((uint32)odCrouch << 8) ^ ((uint32)s_odMove.policy << 4) ^ (uint32)(odWeapon & 0xF);
	if (!odMoving) {
		s_odSpanHeldSince = 0.0f;
		ViceExtP4MoveEnd(odSim, odFrame);
		return;
	}
	if (s_odSpanHeldSince == 0.0f)
		s_odSpanHeldSince = odSim;
	if (!s_odSpan.active) {
		if (odSim - s_odSpanHeldSince < 0.5f)
			return;
		s_odSpan.active = true;
		++s_odSpan.id;
		s_odSpan.simStart = odSim;
		s_odSpan.dist = 0.0f;
		s_odSpan.dirErrMax = 0.0f;
		s_odSpan.bodyErrMax = 0.0f;
		s_odSpan.simSamples = 0;
		s_odSpan.gaps = 0;
		s_odSpan.declSum = 0.0f;
		s_odSpan.lr = odLR;
		s_odSpan.ud = odUD;
		s_odSpan.reqX = s_odMove.dirX;
		s_odSpan.reqY = s_odMove.dirY;
		s_odSpan.baseFx = s_odMove.baseX;
		s_odSpan.baseFy = s_odMove.baseY;
		s_odSpan.baseRx = s_odMove.baseY;
		s_odSpan.baseRy = -s_odMove.baseX;
		s_odSpan.aim = odAim;
		s_odSpan.crouch = odCrouch;
		s_odSpan.policy = s_odMove.policy;
		s_odSpan.weapon = odWeapon;
		s_odSpan.sprint = odSprint;
		s_odSpan.walk = odWalk;
		s_odSpan.caseId = (odCrouch || odAim) ? 4 : (Abs(odLR) > 8.0f && Abs(odUD) > 8.0f) ? 2 : 1;
		s_odSpanKey = odKey;
		char t[520];
		VICEEXT_P4_EMIT(s_odMove.gen, t,  "P4 kind=move_begin schema=1 gen=%u frame=%u sim=%.4f case=%d id=%u lr=%.2f ud=%.2f aim=%d policy=%d baseF=%.6f,%.6f,0.000000 baseR=%.6f,%.6f,0.000000 req=%.6f,%.6f,0.000000 crouch=%d weapon=%d walk=%d sprint=%d",
			(unsigned)s_odMove.gen, (unsigned)odFrame, odSim, s_odSpan.caseId, (unsigned)s_odSpan.id,
			odLR, odUD, odAim, s_odMove.policy, s_odMove.baseX, s_odMove.baseY, s_odMove.baseY, -s_odMove.baseX,
			s_odMove.dirX, s_odMove.dirY, odCrouch, odWeapon, odWalk, odSprint);
		
		return;
	}
	if (odKey != s_odSpanKey) {
		ViceExtP4MoveEnd(odSim, odFrame);
		s_odSpanHeldSince = odSim;
		return;
	}
	CVector odPos = ped->GetPosition();
	static uint32 s_odLastFrame = 0;
	static CVector s_odLastPos(0.0f, 0.0f, 0.0f);
	if (odFrame != s_odLastFrame) {
		s_odLastFrame = odFrame;
		float odDx = odPos.x - s_odLastPos.x;
		float odDy = odPos.y - s_odLastPos.y;
		float odD = Sqrt(odDx * odDx + odDy * odDy);
		s_odSpan.declSum += ped->m_moved.Magnitude() * GAME_SPEED_TO_METERS_PER_SECOND;
		if (odD > 5.0f) {
			++s_odSpan.gaps;
		} else {
			s_odSpan.dist += odD;
			++s_odSpan.simSamples;
			float odDE = ViceExtP4AngleDeg(s_odSpan.reqX, s_odSpan.reqY, odDx, odDy);
			if (odDE > s_odSpan.dirErrMax)
				s_odSpan.dirErrMax = odDE;
			float odBE = ViceExtP4AngleDeg(-Sin(ped->m_fRotationCur), Cos(ped->m_fRotationCur), odDx, odDy);
			if (odBE > s_odSpan.bodyErrMax)
				s_odSpan.bodyErrMax = odBE;
		}
		s_odLastPos = odPos;
	}
}
#endif

#ifdef __EMSCRIPTEN__
static bool s_odP4PoseEnterOn = false;
static float s_odP4PoseEnterSim = 0.0f;
static bool s_odP4PoseExitOn = false;
static float s_odP4PoseExitSim = 0.0f;
static float s_odP4PoseExitMax = 0.0f;

static void
ViceExtP4PoseWeights(CPlayerPed *ped, float *odW)
{
	for (int i = 0; i < 9; i++) {
		CAnimBlendAssociation *a = RpAnimBlendClumpGetAssociation(ped->GetClump(), odCrouchPoseIds[i]);
		odW[i] = a ? a->blendAmount : 0.0f;
	}
}

static float
ViceExtP4PoseOwn(CPlayerPed *ped)
{
	float odW[9];
	ViceExtP4PoseWeights(ped, odW);
	float odMax = 0.0f;
	for (int i = 0; i < 9; i++)
		if (odW[i] > odMax)
			odMax = odW[i];
	CAnimBlendAssociation *odDuck = RpAnimBlendClumpGetAssociation(ped->GetClump(), ANIM_STD_DUCK_WEAPON);
	if (odDuck && odDuck->blendAmount > odMax)
		odMax = odDuck->blendAmount;
	return odMax;
}

static void
ViceExtP4PoseBones(CPlayerPed *ped, float &odNf, float &odDg, int &odSmp)
{
	odNf = 0.0f;
	odDg = 0.0f;
	odSmp = 0;
	static const PedNode odNodes[5] = { PED_MID, PED_UPPERLEGL, PED_UPPERLEGR, PED_LOWERLEGL, PED_LOWERLEGR };
	for (int i = 0; i < 5; i++) {
		if (odNodes[i] < PED_MID || odNodes[i] >= PED_NODE_MAX)
			continue;
		AnimBlendFrameData *f = ped->m_pFrames[odNodes[i]];
		if (f == nil || f->hanimFrame == nil)
			continue;
		odSmp++;
		float odX = f->hanimFrame->q.imag.x;
		float odY = f->hanimFrame->q.imag.y;
		float odZ = f->hanimFrame->q.imag.z;
		float odW = f->hanimFrame->q.real;
		float odL2 = odX * odX + odY * odY + odZ * odZ + odW * odW;
		if (odL2 != odL2 || odL2 > 1e30f || odL2 < 0.0f)
			odNf += 1.0f;
		else if (odL2 < 0.25f || odL2 > 2.25f)
			odDg += 1.0f;
	}
}

static void
ViceExtP4PoseBeginP4(CPlayerPed *ped, int odPosture)
{
	if (odPosture == 1)
		s_odP4PoseEnterId = ++s_odP4PoseSpan;
	else
		s_odP4PoseExitId = ++s_odP4PoseSpan;
	char t[240];
	VICEEXT_P4_EMIT(s_odMove.gen, t,  "P4 kind=pose_begin schema=1 gen=%u frame=%u sim=%.4f case=3 id=%u posture=%d",
		(unsigned)s_odMove.gen, (unsigned)CTimer::GetFrameCounter(),
		CTimer::GetTimeInMilliseconds() * 0.001f,
		(unsigned)(odPosture == 1 ? s_odP4PoseEnterId : s_odP4PoseExitId), odPosture);
	
	if (odPosture == 1) {
		s_odP4PoseEnterOn = true;
		s_odP4PoseEnterSim = CTimer::GetTimeInMilliseconds() * 0.001f;
	} else {
		s_odP4PoseExitOn = true;
		s_odP4PoseExitSim = CTimer::GetTimeInMilliseconds() * 0.001f;
		s_odP4PoseExitMax = 0.0f;
	}
	(void)ped;
}

static void
ViceExtP4PoseEnterTick(CPlayerPed *ped)
{
	if (!s_odP4PoseEnterOn)
		return;
	static uint32 s_odP4PoseFrame = 0;
	uint32 odFrame = CTimer::GetFrameCounter();
	if (odFrame == s_odP4PoseFrame)
		return;
	s_odP4PoseFrame = odFrame;
	float odSim = CTimer::GetTimeInMilliseconds() * 0.001f;
	float odMax = ViceExtP4PoseOwn(ped);
	float odElapsed = odSim - s_odP4PoseEnterSim;
	if (odMax < 0.8f && odElapsed < 1.5f)
		return;
	float odT80 = (odMax >= 0.8f) ? odElapsed : 9.0f;
	float odW[9];
	ViceExtP4PoseWeights(ped, odW);
	float odNf, odDg;
	int odSmp = 0;
	ViceExtP4PoseBones(ped, odNf, odDg, odSmp);
	char t[460];
	VICEEXT_P4_EMIT(s_odMove.gen, t,  "P4 kind=pose_end schema=1 gen=%u frame=%u sim=%.4f case=3 id=%u posture=1 weights=%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f t80=%.4f tClear=0.0000 maxOwnAfter500=0.0000 boneNonfinite=%.0f boneDegenerate=%.0f boneSamples=%d bonesIdx=mid,ul,ur,ll,lr",
		(unsigned)s_odMove.gen, (unsigned)odFrame, odSim,
		(unsigned)s_odP4PoseEnterId,
		odW[0], odW[1], odW[2], odW[3], odW[4], odW[5], odW[6], odW[7], odW[8],
		odT80, odNf, odDg, odSmp);
	
	s_odP4PoseEnterOn = false;
}

static void
ViceExtP4PoseExitTick(CPlayerPed *ped)
{
	if (!s_odP4PoseExitOn)
		return;
	float odSim = CTimer::GetTimeInMilliseconds() * 0.001f;
	float odOwn = ViceExtP4PoseOwn(ped);
	float odElapsed = odSim - s_odP4PoseExitSim;
	if (odElapsed > 0.5f && odOwn > s_odP4PoseExitMax)
		s_odP4PoseExitMax = odOwn;
	bool odDone = (odOwn <= 0.01f);
	if (!odDone && odElapsed < 2.0f)
		return;
	float odClear = odDone ? odElapsed : 9.0f;
	float odW[9];
	ViceExtP4PoseWeights(ped, odW);
	float odNf, odDg;
	int odSmp = 0;
	ViceExtP4PoseBones(ped, odNf, odDg, odSmp);
	char t[460];
	VICEEXT_P4_EMIT(s_odMove.gen, t,  "P4 kind=pose_end schema=1 gen=%u frame=%u sim=%.4f case=3 id=%u posture=0 weights=%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f t80=0.0000 tClear=%.4f maxOwnAfter500=%.4f boneNonfinite=%.0f boneDegenerate=%.0f boneSamples=%d bonesIdx=mid,ul,ur,ll,lr",
		(unsigned)s_odMove.gen, (unsigned)CTimer::GetFrameCounter(), odSim,
		(unsigned)s_odP4PoseExitId,
		odW[0], odW[1], odW[2], odW[3], odW[4], odW[5], odW[6], odW[7], odW[8],
		odClear, s_odP4PoseExitMax, odNf, odDg, odSmp);
	
	s_odP4PoseExitOn = false;
}

static bool s_odP4CamOn = false;
static uint32 s_odP4CamSpan = 1;
static uint32 s_odP4CamId = 0;
static bool s_odP4CamFirst = false;
static uint32 s_odP4CamFrame = 0;
static float s_odP4CamSim = 0.0f;
static int s_odP4CamTo = 0;
static int s_odP4CamMouse = 0;
static int s_odP4CamLock = 0;
static int s_odP4CamClamp = 0;
static int s_odP4CamCrouch = 0;

static float
ViceExtP4CamPitchDeg(CVector odV)
{
	float odH = Sqrt(odV.x * odV.x + odV.y * odV.y);
	return RADTODEG(Atan2(odV.z, odH));
}

static float
ViceExtP4CamFovTarget(CPlayerPed *ped)
{
	eWeaponType odWt = ped->GetWeapon()->m_eWeaponType;
	CWeaponInfo *odInfo = CWeaponInfo::GetWeaponInfo(odWt);
	if (odInfo && !odInfo->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM)
	    && ViceExtCanAim(odWt, odInfo) && (odInfo->m_fRange >= 70.0f || ViceExtAimHeavy(odWt))
	    && odWt != WEAPONTYPE_MINIGUN)
		return 50.0f;
	return 70.0f;
}

static CVector
ViceExtP4AimPointDir(void)
{
	CMatrix odM = TheCamera.GetMatrix();
	CVector odF = odM.GetForward();
	float odFL = odF.Magnitude();
	if (odFL < 0.0001f)
		return odF;
	odF /= odFL;
	CVector odU = odM.GetUp();
	float odUL = odU.Magnitude();
	if (odUL < 0.0001f)
		return odF;
	odU /= odUL;
	float odFov = TheCamera.Cams[TheCamera.ActiveCam].FOV;
	if (odFov < 1.0f)
		odFov = 70.0f;
	CVector odV = odF;
	odV += odU * Tan(DEGTORAD((0.5f - CCamera::m_f3rdPersonCHairMultY) * 1.8f * 0.5f * odFov));
	odV += CrossProduct(odF, odU) * Tan(DEGTORAD((CCamera::m_f3rdPersonCHairMultX - 0.5f) * 1.8f * 0.5f * odFov * CDraw::GetAspectRatio()));
	float odVL = odV.Magnitude();
	if (odVL > 0.0001f)
		odV /= odVL;
	return odV;
}

static float
ViceExtP4CamPixelErr(CPlayerPed *ped)
{
	CMatrix odM = TheCamera.GetMatrix();
	CVector odF = odM.GetForward();
	float odFL = odF.Magnitude();
	if (odFL < 0.0001f)
		return 0.0f;
	odF /= odFL;
	CVector odU = odM.GetUp();
	float odUL = odU.Magnitude();
	if (odUL < 0.0001f)
		return 0.0f;
	odU /= odUL;
	float odFov = TheCamera.Cams[TheCamera.ActiveCam].FOV;
	if (odFov < 1.0f)
		odFov = 70.0f;
	CVector odV = odF;
	odV += odU * Tan(DEGTORAD((0.5f - CCamera::m_f3rdPersonCHairMultY) * 1.8f * 0.5f * odFov));
	odV += CrossProduct(odF, odU) * Tan(DEGTORAD((CCamera::m_f3rdPersonCHairMultX - 0.5f) * 1.8f * 0.5f * odFov * CDraw::GetAspectRatio()));
	odV.Normalise();
	float odDot = DotProduct(odF, odV);
	if (odDot > 1.0f) odDot = 1.0f;
	if (odDot < -1.0f) odDot = -1.0f;
	float odTanHalf = Tan(DEGTORAD(odFov * 0.5f));
	if (odTanHalf < 0.0001f)
		return 0.0f;
	(void)ped;
	return Tan(Acos(odDot)) / odTanHalf * (RsGlobal.maximumHeight * 0.5f);
}

static void
ViceExtP4CamBegin(CPlayerPed *ped, CVector odView, int odFrom, int odTo)
{
	if (s_odP4CamOn) {
		char tc[200];
		VICEEXT_P4_EMIT(1, tc,  "P4 kind=cam_cancel schema=1 gen=1 frame=%u sim=%.4f case=6 id=%u cause=nueva progress=0.0000",
			(unsigned)CTimer::GetFrameCounter(), CTimer::GetTimeInMilliseconds() * 0.001f, (unsigned)s_odP4CamId);
		
	}
	s_odP4CamId = ++s_odP4CamSpan;
	s_odP4CamOn = true;
	s_odP4CamFirst = false;
	s_odP4CamFrame = CTimer::GetFrameCounter();
	s_odP4CamSim = CTimer::GetTimeInMilliseconds() * 0.001f;
	s_odP4CamTo = odTo;
	s_odP4CamMouse = 0;
	s_odP4CamLock = (ped->m_pPointGunAt != nil) ? 1 : 0;
	float odPitch = ViceExtP4CamPitchDeg(odView);
	if (odTo == (int)CCam::MODE_AIMING)
		s_odP4CamClamp = (Abs(odPitch) > 50.0f) ? 1 : 0;
	else
		s_odP4CamClamp = (odPitch < -89.5f || odPitch > 60.0f) ? 1 : 0;
	int odCrouch = 0;
#ifdef VICEEXT_CROUCH
	odCrouch = odCrouched ? 1 : 0;
#endif
	s_odP4CamCrouch = odCrouch;
	char t[340];
	VICEEXT_P4_EMIT(1, t,  "P4 kind=cam_begin schema=1 gen=1 frame=%u sim=%.4f case=6 id=%u from=%d to=%d visibleFront=%.6f,%.6f,%.6f mouse=0 lock=%d collision=0 pitchClamped=%d crouch=%d",
		(unsigned)s_odP4CamFrame, s_odP4CamSim, (unsigned)s_odP4CamId, odFrom, odTo, odView.x, odView.y, odView.z,
		s_odP4CamLock, s_odP4CamClamp, odCrouch);
	
}

static void
ViceExtP4CamEnd(CPlayerPed *ped, uint32 odFrame, float odSim)
{
	if (!s_odP4CamOn)
		return;
	CVector odF = TheCamera.GetMatrix().GetForward();
	float odL = odF.Magnitude();
	if (odL > 0.0001f)
		odF /= odL;
	float odFov = TheCamera.Cams[TheCamera.ActiveCam].FOV;
	float odTarget = ViceExtP4CamFovTarget(ped);
	char t[440];
	VICEEXT_P4_EMIT(1, t,  "P4 kind=cam_end schema=1 gen=1 frame=%u sim=%.4f case=6 id=%u visibleFront=%.6f,%.6f,%.6f fov=%.4f fovTarget=%.4f pixelErr=%.4f mouse=%d lock=%d collision=0 pitchClamped=%d crouch=%d",
		(unsigned)odFrame, odSim, (unsigned)s_odP4CamId, odF.x, odF.y, odF.z, odFov, odTarget, ViceExtP4CamPixelErr(ped),
		s_odP4CamMouse, s_odP4CamLock, s_odP4CamClamp, s_odP4CamCrouch);
	
	s_odP4CamOn = false;
}

static void
ViceExtP4CamTick(CPlayerPed *ped)
{
	if (!s_odP4CamOn)
		return;
	uint32 odFrame = CTimer::GetFrameCounter();
	if (odFrame == s_odP4CamFrame)
		return;
	float odSim = CTimer::GetTimeInMilliseconds() * 0.001f;
	CPad *odPad = CPad::GetPad(0);
	if (odPad && (Abs(odPad->NewMouseControllerState.x) > 0.5f || Abs(odPad->NewMouseControllerState.y) > 0.5f))
		s_odP4CamMouse = 1;
	if (!s_odP4CamFirst) {
		CVector odF = TheCamera.GetMatrix().GetForward();
		float odL = odF.Magnitude();
		if (odL > 0.0001f)
			odF /= odL;
		char t[340];
		VICEEXT_P4_EMIT(1, t,  "P4 kind=cam_first schema=1 gen=1 frame=%u sim=%.4f case=6 id=%u visibleFront=%.6f,%.6f,%.6f pixelErr=%.4f",
			(unsigned)odFrame, odSim, (unsigned)s_odP4CamId, odF.x, odF.y, odF.z, ViceExtP4CamPixelErr(ped));
		
		s_odP4CamFirst = true;
		return;
	}
	float odElapsed = odSim - s_odP4CamSim;
	float odFov = TheCamera.Cams[TheCamera.ActiveCam].FOV;
	float odTarget = ViceExtP4CamFovTarget(ped);
	bool odSettled = TheCamera.m_uiTransitionState == 0 && Abs(odFov - odTarget) <= 0.5f;
	if ((odSettled && odElapsed >= 0.30f) || odElapsed >= 2.0f)
		ViceExtP4CamEnd(ped, odFrame, odSim);
}
#endif

static bool
ViceExtStrafeAiming(CPlayerPed *ped, CPad *pad)
{
	(void)pad;
	if (!ViceExtMovePrepared(ped))
		return false;
	return s_odMove.policy == 1;
}
#endif

void
CPlayerPed::PlayerControl1stPersonRunAround(CPad *padUsed)
{
	float leftRight = padUsed->GetPedWalkLeftRight();
	float upDown = padUsed->GetPedWalkUpDown();
	float padMove = CVector2D(leftRight, upDown).Magnitude();
	float padMoveInGameUnit = padMove / PAD_MOVE_TO_GAME_WORLD_MOVE;
#ifdef VICEEXT_AIM_WALK
	// H4: el control de 1ª persona no tiene tope de apuntado, así que aquí el
	// valor sin tope ES el pedido y el tope de R9 no se aplica (el jugador anda
	// hacia donde mira). Se publica igual para que `SetRealMoveAnim` no aplique
	// una cadencia escalada que aquí no toca.
	odAimWalkActive = false;
	odAimWalkUncapped = padMoveInGameUnit;
#endif
	if (padMoveInGameUnit > 0.0f) {
#ifdef VICEEXT_CROUCH
		// R26: a los lados agachado sin apuntar, el destino es el rumbo mundo
		// del helper (si no, este forzado a camara lo desharia cada frame).
		float odSideDir = 0.0f;
		if (ViceExtCrouchSideHeading(odSideDir))
			m_fRotationDest = odSideDir;
		else
#endif
#ifdef VICEEXT_AIM_CLASSICAXIS
		// PORTADO — ClassicAXIS: `playerMovementType` (Main.cpp:121-133). Antes este
		// `else` encajaba el cuerpo con la cámara SIEMPRE, que es justo lo que el
		// jugador describió como el defecto notorio. El mod solo lo hace apuntando
		// (`TYPE_STRAFE`); sin apuntar devuelve `TYPE_WALKAROUND` y el cuerpo gira
		// hacia donde camina. La fórmula del rumbo es la misma que usa
		// `PlayerControlZelda` (línea 2065): rumbo del mando relativo a la cámara.
		if (!ViceExtStrafeAiming(this, padUsed)) {
			if (ViceExtMoveHeadingOwned())
				m_fRotationDest = ViceExtMoveHeading();
			else {
				float odPadHead = CGeneral::GetRadianAngleBetweenPoints(0.0f, 0.0f, -leftRight, upDown);
				m_fRotationDest = CGeneral::LimitRadianAngle(odPadHead - TheCamera.Orientation);
			}
		}
		// TRAZA `AX7` (1 Hz). Mide EXACTAMENTE lo que el jugador describio: con A/D
		// y sin raton el cuerpo debe encarse hacia la direccion de marcha (`tipo=0`
		// = WALKAROUND) y el error cuerpo-vs-marcha debe ser pequeno; con el raton y
		// el cuerpo quieto debe poder girar solo (`tipo=2` = un frame de locomocion
		// real, el `forceRealMoveAnim` del mod).
		// PASS = andando de lado SIN apuntar: tipo=0 y `desv` < 15 grados.
		// FALLO = tipo=2 con el raton parado, o tipo=0 con `desv` > 60 (sigue
		// mirando a la camara en vez de a la marcha).
		{
			static uint32 s_odAx7 = 0;
			uint32 odNow = CTimer::GetTimeInMilliseconds();
			if (odNow >= s_odAx7 && odNow + 60000 >= s_odAx7) {
				s_odAx7 = odNow + 1000;
				float odWalkDir = CGeneral::GetRadianAngleBetweenPoints(0.0f, 0.0f, -leftRight, upDown);
				float odDesv = CGeneral::LimitRadianAngle(m_fRotationCur - CGeneral::LimitRadianAngle(odWalkDir - TheCamera.Orientation));
				if (odDesv < 0.0f) odDesv = -odDesv;
				int odTipo = CCamera::s_viceExtAimForceRealMoveAnim ? 2
					: (ViceExtStrafeAiming(this, padUsed) ? 1 : 0);
				char t[160];
				snprintf(t, sizeof t, "AX7 tipo=%d lr=%.2f ud=%.2f desv=%.1f camh=%.1f cuerpo=%.1f",
					odTipo, leftRight, upDown, RADTODEG(odDesv),
					RADTODEG(TheCamera.Orientation), RADTODEG(m_fRotationCur));
				ODTRACES(t);
			}
		}
#else
		m_fRotationDest = CGeneral::LimitRadianAngle(TheCamera.Orientation);
#endif
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

	if (CanSprintWithCurrentWeapon() && padUsed->GetSprint()) {
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

	return (!zRotImportant || CWeaponInfo::GetWeaponInfo(weapon->m_eWeaponType)->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM) || Abs(angleDiff) <= HALFPI) &&
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
#ifdef VICEEXT_AIM_CLASSICAXIS
	// PORTADO — ClassicAXIS: `playerMovementType` (Main.cpp:121-133), la otra mitad
	// del mismo hook. El motor elegía los clips de costado (`PLAYERLEFT/RIGHT/BACK`)
	// sólo con que el ángulo de marcha (`m_fWalkAngle`) pasara de 50°, es decir
	// andando de lado, y sin mirar si se estaba apuntando — de ahí el "de costado
	// sin apuntar". El mod invierte la condición: de costado es el caso de
	// apuntar, y el resto va de recto con el cuerpo girando hacia la marcha.
	if (ViceExtStrafeAiming(this, CPad::GetPad(0))) {
#else
	if ((m_fWalkAngle <= -DEGTORAD(50.0f) || m_fWalkAngle >= DEGTORAD(50.0f))
		&& TheCamera.Cams[TheCamera.ActiveCam].Using3rdPersonMouseCam()
		&& CanStrafeOrMouseControl()) {
#endif

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

#ifdef VICEEXT_ROCKET_3RD_PERSON
// Vice Extended (v3.0 "Enable third-person aiming from a rocket launcher", su
// features.ini: `RocketLauncherThirdPersonAiming=1`).
//
// En VC, apuntar con el lanzacohetes mete al jugador en el modo de
// francotirador: `SetNewPlayerWeaponMode(MODE_ROCKETLAUNCHER)` + `PED_SNIPER_MODE`
// + `m_fMoveSpeed = 0` (clavado, cámara de 1ª persona) y el disparo EXIGE uno de
// esos modos de cámara. El mod lo cambia: el lanzacohetes apunta en TERCERA
// persona y se puede andar apuntando (como en SA).
//
// Lo único que hacía falta: no entrar en ese modo (el apuntado de brazo de VC ya
// existe: el arma trae `WEAPONFLAG_CANAIM_WITHARM` en su `weapon.dat`) y no
// exigir el modo de 1ª persona para lanzar (`Weapon.cpp`, `FireProjectile`) —
// el misil ya sale hacia donde mira la cámara (`CProjectileInfo::AddProjectile`
// usa la matriz de la cámara cuando el tirador es el jugador).
static bool
ViceExtRocket3rdPerson(eWeaponType weapon)
{
	return weapon == WEAPONTYPE_ROCKETLAUNCHER;
}

// Traza 1/s mientras el lanzacohetes está en la mano (bloque R16 del
// verificador): dice el modo de cámara y el estado del ped, que es como se
// distingue "apunta en 3ª persona" de "volvió al modo francotirador".
static void
ViceExtRocket3pTrace(CPlayerPed *ped, CPad *padUsed, eWeaponType weapon)
{
#ifdef __EMSCRIPTEN__
	static uint32 s_odNextR3p = 0;
	uint32 odNow = CTimer::GetTimeInMilliseconds();
	if (odNow < s_odNextR3p && odNow + 60000 >= s_odNextR3p)
		return;   // el reloj retrocedió (carga de partida): no bloquear la traza
	s_odNextR3p = odNow + 1000;
	char t[170];
	snprintf(t, sizeof t, "R3P arma=%d modo=%d ped=%d mira=%d apunta=%d spd=%.1f",
		(int)weapon, (int)TheCamera.Cams[TheCamera.ActiveCam].Mode, (int)ped->GetPedState(),
		padUsed ? (int)padUsed->GetTarget() : -1, (int)(ped->m_pPointGunAt != nil), ped->m_fMoveSpeed);
	ODTRACES(t);
#else
	(void)ped; (void)padUsed; (void)weapon;
#endif
}
#endif

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

	if (VICEEXT_ENGINE_DUCK_KEY() && !bIsDucking && m_nMoveState != PEDMOVE_SPRINT) {
#ifdef FIX_BUGS
		// fix tommy being locked into looking at the same spot if you duck just after starting to shoot
		if(!m_pPointGunAt)
			ClearPointGunAt();
#endif
		bCrouchWhenShooting = true;
		SetDuck(60000, true);
	} else if (bIsDucking && (VICEEXT_ENGINE_DUCK_KEY() || m_nMoveState == PEDMOVE_SPRINT ||
		padUsed->GetSprint() || padUsed->JumpJustDown() || padUsed->ExitVehicleJustDown())) {

#ifdef FIX_BUGS
		// same fix as above except for standing up
		if(!m_pPointGunAt)
			ClearPointGunAt();
#endif
		ClearDuck(true);
		bCrouchWhenShooting = false;
	}

#ifdef __EMSCRIPTEN__
	ViceExtWeaponInfoTrace(GetWeapon()->m_eWeaponType, weaponInfo);
#endif
	if(ViceExtCanAim(GetWeapon()->m_eWeaponType, weaponInfo)) // Sección 3, C7: las escopetas también apuntan
		m_wepAccuracy = 95;
	else
		m_wepAccuracy = 100;

	if (!m_pFire) {
		eWeaponType weapon = GetWeapon()->m_eWeaponType;
		if (weapon == WEAPONTYPE_ROCKETLAUNCHER || weapon == WEAPONTYPE_SNIPERRIFLE ||
			weapon == WEAPONTYPE_LASERSCOPE || weapon == WEAPONTYPE_M4 ||
			weapon == WEAPONTYPE_RUGER || weapon == WEAPONTYPE_M60 ||
			weapon == WEAPONTYPE_CAMERA) {

			// Vice Extended (R16): con el lanzacohetes NO se entra en el modo de
			// francotirador/1ª persona — apunta en 3ª persona y se puede andar.
			bool odRocket3p = false;
#ifdef VICEEXT_ROCKET_3RD_PERSON
			odRocket3p = ViceExtRocket3rdPerson(weapon);
			if (odRocket3p)
				ViceExtRocket3pTrace(this, padUsed, weapon);
#endif
			if (!odRocket3p) {
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
		}
	}

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

#ifdef __EMSCRIPTEN__
	{
		static int s_odFireWas = -1;
		int odFireNow = (padUsed->GetWeapon() ? 1 : 0)
			| ((m_nSelectedWepSlot == m_currentWeapon) ? 2 : 0)
			| ((m_nMoveState == PEDMOVE_SPRINT) ? 4 : 0)
			| ((m_pPointGunAt != nil) ? 8 : 0)
			| (ViceExtIsAiming() ? 16 : 0)
			| (((int)m_nPedState & 0x3F) << 5);
		if (odFireNow != s_odFireWas) {
			s_odFireWas = odFireNow;
			char t[240];
			VICEEXT_P4_EMIT(s_odMove.gen, t, "P4 kind=fire schema=1 gen=%u frame=%u sim=%.4f case=4 id=0 aim=%d gat=%d arma=%d hip=%d estado=%d slot=%d move=%d",
				(unsigned)s_odMove.gen, (unsigned)CTimer::GetFrameCounter(), CTimer::GetTimeInMilliseconds() * 0.001f,
				ViceExtIsAiming() ? 1 : 0, padUsed->GetWeapon() ? 1 : 0, (int)GetWeapon()->m_eWeaponType,
				(m_pPointGunAt == nil) ? 1 : 0, (int)m_nPedState, (int)m_currentWeapon, (int)m_nMoveState);
		}
	}
#endif

#ifdef FREE_CAM
	static int8 changedHeadingRate = 0;
	static int8 pointedGun = 0;
	if (changedHeadingRate == 2) changedHeadingRate = 1;
	if (pointedGun == 2) pointedGun = 1;

	// Rotate player/arm when shooting. We don't have auto-rotation anymore
	// Sección 1 (9ª partida): la rotación de apuntado no debe depender de
	// `bFreeCam` (que es quien cortaba antes `CanStrafeOrMouseControl`): el
	// apuntado con ratón es cosa de `m_bUseMouse3rdPerson`, y encarar el arma al
	// centro de la pantalla es este bloque.
	if (CCamera::m_bUseMouse3rdPerson &&
		m_nSelectedWepSlot == m_currentWeapon && m_nMoveState != PEDMOVE_SPRINT) {

#define CAN_AIM_WITH_ARM ((weaponInfo->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM) || ViceExtAimWithArm(GetWeapon()->m_eWeaponType)) && !bIsDucking && !bCrouchWhenShooting)
		// Weapons except throwable and melee ones
		if (weaponInfo->m_nWeaponSlot > 2) {
			if ((padUsed->GetTarget() && CAN_AIM_WITH_ARM) || padUsed->GetWeapon()) {
				float limitedCam = CGeneral::LimitRadianAngle(-TheCamera.Orientation);

				m_cachedCamSource = TheCamera.Cams[TheCamera.ActiveCam].Source;
				m_cachedCamFront = TheCamera.Cams[TheCamera.ActiveCam].Front;
				m_cachedCamUp = TheCamera.Cams[TheCamera.ActiveCam].Up;
				
				// On this one we can rotate arm.
				if (CAN_AIM_WITH_ARM) {
					pointedGun = 2;
					m_bFreeAimActive = true;
					SetLookFlag(limitedCam, true, true);
#ifdef VICEEXT_AIM_CLASSICAXIS
					// PORTADO — ClassicAXIS: `clearAimFlag` (Main.cpp:207-211,
					// RedirectCall VC 0x52C274). El mod sustituye la llamada del juego
					// por `if (!isAiming) playa->ClearAimFlag();`, de modo que el ped
					// no se queda con la POSE de apuntado puesta cuando el mod no está
					// apuntando. Sin esto el motor deja el `aim flag` colgado de un
					// apuntado anterior y el cuerpo se queda con el brazo apuntando al
					// vacío.
					if (ViceExtIsAimingNow(this, padUsed))
						SetAimFlag(limitedCam);
					else
						ClearAimFlag();
#else
					SetAimFlag(limitedCam);
#endif
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
					if (ViceExtCanAim(GetWeapon()->m_eWeaponType, weaponInfo) && padUsed->WeaponJustDown()) { // C7
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

#ifdef VICEEXT_AIM_CLASSICAXIS
				// PORTADO — ClassicAXIS: `clearAimFlag` (Main.cpp:207-211), el segundo
				// callsite. Mismo criterio que el de arriba.
				if (ViceExtIsAimingNow(this, padUsed))
					SetAimFlag(limitedCam);
				else
					ClearAimFlag();
#else
				SetAimFlag(limitedCam);
#endif
				((CPlayerPed*)this)->m_fFPSMoveHeading = TheCamera.Find3rdPersonQuickAimPitch();
				m_bFreeAimActive = true;
			}
		} else {
			pointedGun = 0;
			ClearPointGunAt();
		}
	}
#endif

	if (padUsed->GetTarget() && m_nSelectedWepSlot == m_currentWeapon &&			 m_nMoveState != PEDMOVE_SPRINT && !TheCamera.Using1stPersonWeaponMode() && ViceExtCanAim(GetWeapon()->m_eWeaponType, weaponInfo)) { // Sección 3, C7
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
			TheCamera.SetNewPlayerWeaponMode(CCam::MODE_SYPHON, 0, 0);
			TheCamera.UpdateAimingCoors(m_pPointGunAt->GetPosition());

		} else if (!CCamera::m_bUseMouse3rdPerson) {
			if (padUsed->TargetJustDown() || TheCamera.m_bJustJumpedOutOf1stPersonBecauseOfTarget)
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

#ifdef VICEEXT_AIM_WALK
// R9: MEDIR antes de tocar. Traza AIMDIR (1/s apuntando): arma, desviación en
// grados entre la dirección de apuntado de la cámara y el rumbo del ped,
// grupo/clip de animación y peso de los blends parciales (agachado/nado: un
// blend que no vuelve a 0 corrompe la pose). Lectura:
// - desv < 5° en todas → PASS;
// - desv grande con TODAS al caminar → era el rumbo (arreglo de arriba);
// - desv grande SÓLO con las nuevas (steyr/deagle/shotgun2) → comparar el
//   AnimAssocDesc del grupo nuevo contra los fotogramas del .ifp (sección 1)
//   y mientras probar el grupo estándar equivalente (steyr→rifle,
//   Shotgun2→shotgun) para aislar;
// - desv grande sólo tras agacharse/nadar → blend parcial sin peso a 0 (R6/R7).
static float
ViceExtBlendWeight(RpClump *clump, AnimationId anim)
{
	CAnimBlendAssociation *a = RpAnimBlendClumpGetAssociation(clump, anim);
	return a ? a->blendAmount : 0.0f;
}

void
CPlayerPed::ViceExtAimDirTrace(CPad *padUsed)
{
	if (!padUsed || bInVehicle || DyingOrDead() || !IsPedInControl())
		return;
	if (!padUsed->GetTarget())
		return;
	eWeaponType wt = GetWeapon()->m_eWeaponType;
	CWeaponInfo *info = CWeaponInfo::GetWeaponInfo(wt);
	if (!ViceExtCanAim(wt, info))
		return;
	CVector camF = TheCamera.Cams[TheCamera.ActiveCam].Front;
	camF.z = 0.0f;
	if (camF.MagnitudeSqr() < 0.001f)
		return;
	camF.Normalise();
	CVector pedF = GetForward();
	pedF.z = 0.0f;
	if (pedF.MagnitudeSqr() < 0.001f)
		return;
	pedF.Normalise();
	float odDot = DotProduct(camF, pedF);
	odDot = Clamp(odDot, -1.0f, 1.0f);
	float odDesv = RADTODEG(Acos(odDot));
	AnimationId odFire = GetPrimaryFireAnim(info);
	// H3: el `peso` miraba sólo el FIRE: apuntando agachado la pose la lleva el
	// CROUCHFIRE y salía 0,0 aunque el apuntado estuviera bien. Se mira el mayor.
	float odPeso = ViceExtBlendWeight(GetClump(), odFire);
	AnimationId odCrouchFire = GetCrouchFireAnim(info);
	if (odCrouchFire)
		odPeso = Max(odPeso, ViceExtBlendWeight(GetClump(), odCrouchFire));
#ifdef __EMSCRIPTEN__
#ifdef VICEEXT_CROUCH
	{
		static int s_odArmSig = -1;
		static uint32 s_odArmMs = 0;
		int odArmSig = (int)ViceExtCrouchAimPoseClip(this);
		uint32 odArmNow = CTimer::GetTimeInMilliseconds();
		bool odArmDue = (odArmSig != s_odArmSig) || (odArmNow + 60000 < s_odArmMs) || (odArmNow >= s_odArmMs + 100);
		if (odArmDue) {
			s_odArmSig = odArmSig;
			s_odArmMs = odArmNow;
			CAnimBlendAssociation *odArm = RpAnimBlendClumpGetAssociation(GetClump(), (AnimationId)odArmSig);
			const char *odArmNom = (odArm && odArm->hierarchy) ? odArm->hierarchy->name : "?";
			float odArmPeso = odArm ? odArm->blendAmount : 0.0f;
			float odCanon = -1.0f;
			{
				RpHAnimHierarchy *odHier = GetAnimHierarchyFromSkinClump(GetClump());
				int odHidx = RpHAnimIDGetIndex(odHier, m_pFrames[PED_HANDR]->nodeID);
				RwMatrix *odHm = &RpHAnimHierarchyGetMatrixArray(odHier)[odHidx];
				CVector odBarrel(odHm->at.x, odHm->at.y, odHm->at.z);
				if (odBarrel.MagnitudeSqr() > 0.000001f) {
					odBarrel.Normalise();
					odCanon = RADTODEG(Acos(Clamp(DotProduct(odBarrel, ViceExtP4AimPointDir()), -1.0f, 1.0f)));
				}
			}
			char t[340];
			VICEEXT_P4_EMIT(s_odMove.gen, t, "P4 kind=armsight schema=1 gen=%u frame=%u sim=%.4f case=4 id=0 pose=%s posepeso=%.2f errDeg=%.4f canon=%.4f px=%.1f weapon=%d crouch=%d aim=%d",
				(unsigned)s_odMove.gen, (unsigned)CTimer::GetFrameCounter(), CTimer::GetTimeInMilliseconds() * 0.001f,
				odArmNom, odArmPeso, odDesv, odCanon, ViceExtP4CamPixelErr(this), (int)wt,
				ViceExtIsCrouched() ? 1 : 0, ViceExtIsAiming() ? 1 : 0);
		}
	}
#endif
#endif
	float odCrouchW = Max(Max(ViceExtBlendWeight(GetClump(), ANIM_STD_CROUCH_FORWARD),
		ViceExtBlendWeight(GetClump(), ANIM_STD_CROUCH_BACKWARD)),
		ViceExtBlendWeight(GetClump(), ANIM_STD_CROUCH_IDLE));
	float odSwimW = Max(Max(ViceExtBlendWeight(GetClump(), ANIM_STD_SWIM_BREAST),
		ViceExtBlendWeight(GetClump(), ANIM_STD_SWIM_CRAWL)),
		ViceExtBlendWeight(GetClump(), ANIM_STD_SWIM_TREAD));
#ifdef __EMSCRIPTEN__
	static uint32 s_odNextAim = 0;
	uint32 odNow = CTimer::GetTimeInMilliseconds();
	if (odNow < s_odNextAim && odNow + 60000 >= s_odNextAim) {}
	else {
		s_odNextAim = odNow + 1000;
		char t[180];
		//
		// R13b: HACIA DÓNDE APUNTA LA MANO que lleva el arma. El motor dibuja el
		// arma copiando la matriz del hueso de la mano derecha (`CPed::Render`:
		// `*RwFrameGetMatrix(frame) = mano`), así que la orientación del arma ES la
		// de ese hueso: si el arma "apunta a un costado", aquí sale el ángulo,
		// medido contra la cámara, en grados. Comparar el `m4` de serie (que el
		// jugador da por bueno) con cada arma nueva dice si el fallo es de la pose
		// o del modelo del mod.
		float odManoH = 0.0f;
		float odManoV = 0.0f;
		{
			RpHAnimHierarchy *hier = GetAnimHierarchyFromSkinClump(GetClump());
			int idx = RpHAnimIDGetIndex(hier, m_pFrames[PED_HANDR]->nodeID);
			RwMatrix *hm = &RpHAnimHierarchyGetMatrixArray(hier)[idx];
			CVector manoAt(hm->at.x, hm->at.y, hm->at.z);
			CVector manoFlat(manoAt.x, manoAt.y, 0.0f);
			if (manoFlat.MagnitudeSqr() > 0.0001f) {
				manoFlat.Normalise();
				odManoH = RADTODEG(Acos(Clamp(DotProduct(manoFlat, camF), -1.0f, 1.0f)));
			}
			// Pitch de la mano (arriba/abajo) frente a la horizontal.
			float odAtLen = manoAt.Magnitude();
			if (odAtLen > 0.0001f)
				odManoV = RADTODEG(Asin(Clamp(manoAt.z / odAtLen, -1.0f, 1.0f)));
		}
		snprintf(t, sizeof t, "AIMDIR arma=%d desv=%.1f grupo=%d clip=%d peso=%.2f crouchW=%.2f swimW=%.2f tgt=%d ik=0x%x manoH=%.1f manoV=%.1f",
			(int)wt, odDesv, (int)info->m_AnimToPlay, (int)odFire,
			odPeso, odCrouchW, odSwimW, padUsed->GetTarget() ? 1 : 0, (unsigned)m_pedIK.m_flags,
			odManoH, odManoV);
		ODTRACES(t);
	}
#endif
}
#endif

#ifdef __EMSCRIPTEN__
static void ViceExtP4PartialWatch(CPlayerPed *odPed);
#endif

void
CPlayerPed::PlayerControlZelda(CPad *padUsed)
{
	float smoothSprayRate = DoWeaponSmoothSpray();
	float camOrientation = TheCamera.Orientation;
	float leftRight = padUsed->GetPedWalkLeftRight();
	float upDown = padUsed->GetPedWalkUpDown();
	float padMoveInGameUnit;
	bool smoothSprayWithoutMove = false;

	s_odStickBlocked = MovementDisabledBecauseOfTargeting() ? 1 : 0;

#ifdef __EMSCRIPTEN__
	ViceExtP4PartialWatch(this);
	{
		static CVector s_odStandPos(0.0f, 0.0f, 0.0f);
		static bool s_odStandOn = false;
		static float s_odStandT = 0.0f, s_odStandM = 0.0f, s_odStandMax = 0.0f;
		static uint32 s_odStandN = 0, s_odStandFrame = 0;
		uint32 odSFrame = CTimer::GetFrameCounter();
		if (odSFrame != s_odStandFrame) {
			s_odStandFrame = odSFrame;
			float odPadMag = padUsed ? (float)CVector2D(padUsed->GetPedWalkLeftRight(), padUsed->GetPedWalkUpDown()).Magnitude() : 0.0f;
			bool odStandWalk = m_fMoveSpeed > 0.05f && odPadMag > 16.0f && !bInVehicle
				&& !DyingOrDead() && !CReplay::IsPlayingBack();
			CVector odSP = GetPosition();
			if (odStandWalk) {
				if (!s_odStandOn) {
					s_odStandOn = true;
					s_odStandT = 0.0f;
					s_odStandM = 0.0f;
					s_odStandMax = 0.0f;
					s_odStandN = 0;
				} else {
					float odSStep = CVector2D(odSP.x - s_odStandPos.x, odSP.y - s_odStandPos.y).Magnitude();
					s_odStandM += odSStep;
					if (odSStep > s_odStandMax)
						s_odStandMax = odSStep;
				}
				s_odStandT += CTimer::GetTimeStep() / 50.0f;
				++s_odStandN;
				s_odStandPos = odSP;
			} else if (s_odStandOn) {
				s_odStandOn = false;
				char t[340];
				snprintf(t, sizeof t, "P4 kind=stand_end schema=1 gen=1 frame=%u sim=%.4f case=6 id=0 aim=%d mfs=%.4f padv=%.4f t=%.4f m=%.4f mps=%.4f maxfr=%.4f n=%u mvec=%.4f fspd=%.4f pad=%.0f",
					(unsigned)odSFrame, CTimer::GetTimeInMilliseconds() * 0.001f, ViceExtIsAiming() ? 1 : 0,
					m_fMoveSpeed, m_vecAnimMoveDelta.Magnitude(), s_odStandT, s_odStandM,
					(s_odStandT > 0.001f) ? (s_odStandM / s_odStandT) : 0.0f, s_odStandMax, s_odStandN,
					m_vecMoveSpeed.Magnitude(), m_fMoveSpeed, odPadMag);
				ODTRACES(t);
			}
		}
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

#ifdef VICEEXT_AIM_WALK
	// Sección 2 (21/09, 5ª partida): "cuando estoy apuntando no debería poder
	// correr, sólo caminar". Apuntando (PED_LOCK_TARGET: botón derecho o Supr) la
	// velocidad se limita a paso de andar (1.0) y el esprint se anula más abajo.
	// H4: el valor SIN tope se publica para que la animación pueda seguir siendo
	// la del movimiento normal con la cadencia escalada (ver SetRealMoveAnim).
	bool odWalkingAim = ViceExtIsAiming();
	odAimWalkActive = odWalkingAim;
	odAimWalkUncapped = padMoveInGameUnit;
	if (odWalkingAim && padMoveInGameUnit > 1.0f)
		padMoveInGameUnit = 1.0f;
#endif

	if (padMoveInGameUnit > 0.0f || smoothSprayWithoutMove) {
		float padHeading = CGeneral::GetRadianAngleBetweenPoints(0.0f, 0.0f, -leftRight, upDown);
		float neededTurn = CGeneral::LimitRadianAngle(padHeading - camOrientation);
		if (smoothSprayRate > 0.0f) {
			m_fRotationDest = m_fRotationCur - leftRight / 128.0f * smoothSprayRate * CTimer::GetTimeStep();
		} else if (ViceExtStrafeAiming(this, padUsed)) {
		} else if (ViceExtMoveHeadingOwned()) {
			m_fRotationDest = ViceExtMoveHeading();
		} else {
			m_fRotationDest = neededTurn;
		}

		float maxAcc = 0.07f * CTimer::GetTimeStep();
		m_fMoveSpeed = Min(padMoveInGameUnit, m_fMoveSpeed + maxAcc);
#ifdef VICEEXT_AIM_WALK
		if (odWalkingAim && m_fMoveSpeed > 1.0f)
			m_fMoveSpeed = 1.0f;
#endif

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

	if (CanSprintWithCurrentWeapon() && padUsed->GetSprint()
#ifdef VICEEXT_AIM_WALK
	    && !odWalkingAim   // apuntando (o agachado) no se esprinta
#endif
	    ) {
		if (!m_pCurrentPhysSurface || (!m_pCurrentPhysSurface->bInfiniteMass || m_pCurrentPhysSurface->m_phy_flagA08))
			m_nMoveState = PEDMOVE_SPRINT;
	}

	if (m_nPedState != PED_FIGHT)
		SetRealMoveAnim();

#ifdef VICEEXT_CLIMB
	// Sección 2 (E1): con la tecla de saltar, si hay un borde escalable delante se
	// trepa en vez de saltar; mientras dura, el control a pie no corre.
	if (ViceExtClimbControl(padUsed))
		return;
#endif

	if (!bIsInTheAir && !CWeaponInfo::GetWeaponInfo(GetWeapon()->m_eWeaponType)->IsFlagSet(WEAPONFLAG_HEAVY)
		&& padUsed->JumpJustDown() && m_nPedState != PED_JUMP) {
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

#ifdef VICEEXT_MANUAL_RELOAD
// Sección 3 (C3.1, corrección de la traza 20/09): distingue la recarga pedida
// con la tecla de la automática al vaciar el cargador. Las dos pasan por
// `WEAPONSTATE_RELOADING`, así que sin esta marca la traza "reload done" daba
// por buena una recarga manual que el jugador no había pedido (pasó en la
// partida del 20/09: un único `done` con el cargador lleno y ningún `start`).
static bool s_viceExtManualReload = false;
// R10: para distinguir `done` (cargador rellenado) de `fail` (aborto: cambio de
// arma, coche, muerte...) se guarda el punto de partida.
static int s_viceExtReloadStartClip = 0;
static int s_viceExtReloadStartType = -1;

// Sección 3 (Vice Extended v2.5, "Reloading a weapon on the key"): recarga a
// mano. Reutiliza el camino de la recarga automática al vaciar el cargador
// (`m_eWeaponState = WEAPONSTATE_RELOADING` + `m_nTimer`): con ese estado, el
// motor ya reproduce la animación y el sonido de recarga (PedFight.cpp /
// Weapon.cpp) y, al vencer el temporizador, llama a CWeapon::Reload() para
// rellenar el cargador desde la munición total.
bool
CPlayerPed::ViceExtTryManualReload(void)
{
	// C3.1b (20/09): cada salida deja traza con su MOTIVO. En la partida real
	// del jugador no hubo una sola línea de este bloque (ni `start` ni `no`), así
	// que no se podía saber si la tecla no llegaba o si el estado la rechazaba:
	// había 4 salidas silenciosas. Ahora ninguna sale sin decirlo.
#ifdef __EMSCRIPTEN__
#define VICEEXT_RELOAD_NO(razon) do { \
		char t[150]; \
		snprintf(t, sizeof t, "VICEEXT reload no motivo=%s arma=%d estado=%d clip=%d/%d total=%d", \
			razon, (int)weapon->m_eWeaponType, (int)weapon->m_eWeaponState, \
			weapon->m_nAmmoInClip, (int)info->m_nAmountofAmmunition, weapon->m_nAmmoTotal); \
		ODTRACES(t); \
	} while (0)
#else
#define VICEEXT_RELOAD_NO(razon) do {} while (0)
#endif

	CWeapon *weapon = GetWeapon();
	CWeaponInfo *info = weapon->GetInfo();

	// La tecla se lee igual que el conmutador de 1ª persona (C1): la acción
	// rebindable del teclado, así que vale en cualquier método de control.
	// Sección 2 (5ª partida, a petición del jugador: "la R no recarga"): la tecla
	// se lee de la acción rebindable y, si esa acción NO apunta a 'R' (una config
	// de controles guardada de antes de que existiera la acción — el hueco era
	// UNKNOWN_ACTION y la config vieja se carga tal cual), también se acepta 'R'
	// directa. Así la recarga no depende de lo que tenga guardado el navegador.
	RsKeyCodes odKey = (RsKeyCodes)ControlsManager.GetControllerKeyAssociatedWithAction(PED_RELOAD, KEYBOARD);
	bool odKeyDown = ControlsManager.GetIsKeyboardKeyJustDown(odKey);
	if (!odKeyDown && odKey != (RsKeyCodes)'R')
		odKeyDown = ControlsManager.GetIsKeyboardKeyJustDown((RsKeyCodes)'R');
	if (!odKeyDown)
		return false;

#ifdef __EMSCRIPTEN__
	// La tecla SÍ ha llegado al motor (una línea por pulsación, no por frame).
	{
		char t[150];
		snprintf(t, sizeof t, "VICEEXT reload key tecla=%d arma=%d estado=%d clip=%d/%d total=%d",
			(int)odKey, (int)weapon->m_eWeaponType, (int)weapon->m_eWeaponState,
			weapon->m_nAmmoInClip, (int)info->m_nAmountofAmmunition, weapon->m_nAmmoTotal);
		ODTRACES(t);
	}
#endif

	if (weapon->m_eWeaponState == WEAPONSTATE_RELOADING) {
		VICEEXT_RELOAD_NO("ya-recargando");
		return false;
	}
	// Sólo el golpe cuerpo a cuerpo queda fuera: todo lo demás (listo,
	// disparando y **sin munición en el cargador**) admite recarga a mano.
	if (weapon->m_eWeaponState == WEAPONSTATE_MELEE_MADECONTACT) {
		VICEEXT_RELOAD_NO("cuerpo-a-cuerpo");
		return false;
	}
	if (info->m_nAmountofAmmunition <= 1) {
		VICEEXT_RELOAD_NO("sin-cargador");
		return false;                       // arma sin cargador (o de un solo tiro)
	}
	if (weapon->m_nAmmoTotal <= 0) {
		VICEEXT_RELOAD_NO("sin-municion");
		return false;
	}
	if (weapon->m_nAmmoInClip >= info->m_nAmountofAmmunition) {
		VICEEXT_RELOAD_NO("cargador-lleno");
		return false;
	}

	uint32 reload = info->m_nReload;
	if (CWorld::Players[CWorld::PlayerInFocus].m_bFastReload)
		reload /= 4;                        // mismo atajo que la recarga automática
	// R10: el bloque NO corta la animación ni deja `m_nTimer` a medias: pone el
	// mismo estado que la recarga automática (`WEAPONSTATE_RELOADING` + timer) y
	// a partir de aquí el motor lleva la animación/sonido (PedFight/Weapon.cpp)
	// y cierra solo con `CWeapon::Reload()` al vencer el timer. Este bloque no
	// vuelve a tocar el arma hasta que el estado cambia.
	weapon->m_eWeaponState = WEAPONSTATE_RELOADING;
	weapon->m_nTimer = CTimer::GetTimeInMilliseconds() + reload;
	s_viceExtManualReload = true;
	s_viceExtReloadStartClip = weapon->m_nAmmoInClip;
	s_viceExtReloadStartType = (int)weapon->m_eWeaponType;
	// R12 (9ª partida): la ANIMACIÓN. El motor sólo la pone dentro del anim de
	// ataque (PedFight::ProcessAttack, al cerrar el bucle del clip de disparo) y
	// recargando quieto —sin ataque en curso— ese bloque no se alcanza: en el log
	// el cargador se rellenaba (`reload start`/`done` con los dos lados) y el
	// jugador no veía ninguna animación. Se mezcla aquí el mismo clip que habría
	// usado el motor: el grupo del arma y el par reload/crouchreload de siempre.
	if (GetReloadAnim(info)) {
		// R28: agachado por el port (no `bIsDucking`) también usa el clip de agachado.
		AnimationId odAnim = (ViceExtCrouchShooting() && GetCrouchReloadAnim(info))
		    ? GetCrouchReloadAnim(info) : GetReloadAnim(info);
		CAnimManager::BlendAnimation(GetClump(), info->m_AnimToPlay, odAnim, 8.0f);
#ifdef __EMSCRIPTEN__
		{
			char t[150];
			snprintf(t, sizeof t, "VICEEXT reload anim grupo=%d clip=%d crouch=%d",
				(int)info->m_AnimToPlay, (int)odAnim, (int)bIsDucking);
			ODTRACES(t);
		}
#endif
	}
#ifdef __EMSCRIPTEN__
	else {
		char t[150];
		snprintf(t, sizeof t, "VICEEXT reload anim grupo=%d clip=0 (arma sin WEAPONFLAG_RELOAD)",
			(int)info->m_AnimToPlay);
		ODTRACES(t);
	}
#endif
#ifdef __EMSCRIPTEN__
	{
		char t[120];
		snprintf(t, sizeof t, "VICEEXT reload start clip=%d total=%d type=%d ms=%u",
			weapon->m_nAmmoInClip, weapon->m_nAmmoTotal, (int)weapon->m_eWeaponType, reload);
		ODTRACES(t);
	}
#endif
	return true;
}
#endif

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

#ifdef VICEEXT_CLIMB
static bool ViceExtClimbAbort(void);
#endif
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
#ifdef VICEEXT_CLIMB
		if (ViceExtClimbAbort()) {
#ifdef __EMSCRIPTEN__
			ODTRACES("VICEEXT swim trepa-off");
#endif
		}
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
							CDraw::GetFOV(), odAim ? 1 : 0, (int)CCamera::s_viceExtAimLawActive,
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
				(int)ViceExtIsAiming(), (int)CCamera::s_viceExtAimLawActive, s_odLastDest,
				(odSwimAssoc ? odSwimAssoc->blendAmount : -1.0f),
				(RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_FALL) ? RpAnimBlendClumpGetAssociation(GetClump(), ANIM_STD_FALL)->blendAmount : -1.0f),
				(int)bIsInWater, odAire, m_vecMoveSpeed.z,
				odLevelW, odDepth, (int)m_nSelectedWepSlot,
		ViceExtBlendWeight(GetClump(), ANIM_STD_FALL_GLIDE),
		(int)bIsStanding, (int)bHasCollided, (int)bIsDucking,
		(int)(m_pPointGunAt != nil), odSuelo, (int)odCrouched);
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

#ifdef VICEEXT_CLIMB
// Sección 2 (21/09, 5ª partida): ESCALAR (bloque opcional E1 del plan; el jugador
// pidió "encárgate de si puedes que el personaje pueda escalar").
//
// El `ped.ifp` del mod trae los clips de escalada de San Andreas (`CLIMB_idle`,
// `CLIMB_jump`, `CLIMB_jump_B`, `CLIMB_jump2fall`, `CLIMB_Pull`, `CLIMB_Stand`,
// `CLIMB_Stand_finish` — comprobados leyendo el binario servido) pero **no había
// ningún grupo que los usara**, así que el port no podía escalar nada. Aquí:
//   - se mira hacia delante con rayos horizontales a distintas alturas: hay borde
//     escalable si a la altura de la rodilla pega y a la del pecho no, o sea una
//     pared que termina entre `VICEEXT_CLIMB_MIN` y `VICEEXT_CLIMB_MAX`;
//   - se comprueba que arriba del borde haya sitio para estar de pie (si no, es un
//     tejado hueco y no se trepa);
//   - se reproduce `CLIMB_Pull` y, durante `odClimbDur`, se lleva al jugador de
//     donde estaba al borde (sin velocidad), terminando con `CLIMB_Stand_finish`.
// Con la tecla de saltar: si hay borde se trepa; si no, se salta como siempre.
#define VICEEXT_CLIMB_MIN     0.55f
#define VICEEXT_CLIMB_MAX     1.85f
#define VICEEXT_CLIMB_MIN_MS  700
#define VICEEXT_CLIMB_MAX_MS  1100
#define VICEEXT_CLIMB_REACH   0.95f

static bool odClimbActive = false;
static uint32 odClimbStart = 0;

// Aborta la trepada activa (ver forward junto a ViceExtSwimControl).
static bool
ViceExtClimbAbort(void)
{
	if (!odClimbActive)
		return false;
	odClimbActive = false;
	return true;
}
static uint32 odClimbDur = 0;
static CVector odClimbFrom;
static CVector odClimbTo;

// Altura del borde que hay justo delante, sobre los pies del jugador (-1 = no hay).
static float
ViceExtClimbLedgeHeight(void)
{
	CPed *ped = FindPlayerPed();
	if (ped == nil)
		return -1.0f;
	CVector pos = ped->GetPosition();
	CVector fwd = ped->GetForward();
	fwd.z = 0.0f;
	if (fwd.MagnitudeSqr() < 0.001f)
		return -1.0f;
	fwd.Normalise();

	CColPoint cp;
	CEntity *ent = nil;
	bool hayParedAbajo = CWorld::ProcessLineOfSight(
		CVector(pos.x, pos.y, pos.z + 0.45f),
		CVector(pos.x + fwd.x * VICEEXT_CLIMB_REACH, pos.y + fwd.y * VICEEXT_CLIMB_REACH, pos.z + 0.45f),
		cp, ent, true, false, false, true, false, true, false);
	if (!hayParedAbajo)
		return -1.0f;   // nada delante: salto normal

	// Se sube el rayo hasta que deje de pegar: ese es el borde.
	float h = VICEEXT_CLIMB_MIN;
	for (; h <= VICEEXT_CLIMB_MAX; h += 0.1f) {
		bool hit = CWorld::ProcessLineOfSight(
			CVector(pos.x, pos.y, pos.z + h),
			CVector(pos.x + fwd.x * VICEEXT_CLIMB_REACH, pos.y + fwd.y * VICEEXT_CLIMB_REACH, pos.z + h),
			cp, ent, true, false, false, true, false, true, false);
		if (!hit)
			break;
	}
	if (h > VICEEXT_CLIMB_MAX)
		return -1.0f;   // pared más alta que el tope: no se trepa
	return h;
}

// Devuelve true mientras dura la escalada (el control a pie no debe correr).
bool
CPlayerPed::ViceExtClimbControl(CPad *padUsed)
{
	uint32 now = CTimer::GetTimeInMilliseconds();
#ifdef VICEEXT_SWIMMING
	// En el agua manda el nado, no la escalada (26/09: al saltar al agua salia
	// la animacion de trepar). Si trepaba y entro al agua, se abandona.
	if (ViceExtIsSwimming()) {
		odClimbActive = false;
		return false;
	}
#endif

	if (odClimbActive) {
		// El reloj del motor retrocede al cargar partida: en ese caso se abandona.
		if (now < odClimbStart) {
			odClimbActive = false;
			return false;
		}
		// Si algo interrumpió el control a pie (muerte, cinemática, coche), no se
		// puede quedar colgado: se abandona en vez de bloquear el salto para siempre.
		if (!IsPedInControl() || bInVehicle || DyingOrDead() || now - odClimbStart > odClimbDur + 800) {
			odClimbActive = false;
			return false;
		}
		float t = (float)(now - odClimbStart) / (float)odClimbDur;
		if (t >= 1.0f) {
			SetPosition(odClimbTo);
			m_vecMoveSpeed = CVector(0.0f, 0.0f, 0.0f);
			m_vecTurnSpeed = CVector(0.0f, 0.0f, 0.0f);
			CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_PLAYERCLIMB, ANIM_STD_CLIMB_STAND_FINISH, 8.0f);
			odClimbActive = false;
#ifdef __EMSCRIPTEN__
			ODTRACES("VICEEXT climb fin");
#endif
			return true;
		}
		SetPosition(odClimbFrom + (odClimbTo - odClimbFrom) * t);
		m_vecMoveSpeed = CVector(0.0f, 0.0f, 0.0f);
		return true;
	}

	if (!padUsed || bInVehicle || !IsPedInControl() || DyingOrDead() || CReplay::IsPlayingBack())
		return false;
	if (bIsInTheAir || m_nPedState == PED_JUMP || m_nMoveState == PEDMOVE_SPRINT)
		return false;
	if (!padUsed->JumpJustDown())
		return false;

	float borde = ViceExtClimbLedgeHeight();
	if (borde < 0.0f) {
#ifdef __EMSCRIPTEN__
		ODTRACES("VICEEXT climb no: sin borde");
#endif
		return false;
	}

	CVector pos = GetPosition();
	CVector fwd = GetForward();
	fwd.z = 0.0f;
	fwd.Normalise();
	CVector destino(pos.x + fwd.x * (VICEEXT_CLIMB_REACH + 0.35f),
			pos.y + fwd.y * (VICEEXT_CLIMB_REACH + 0.35f), pos.z + borde + 0.15f);
	if (!CPedPlacement::IsPositionClearForPed(destino)) {
#ifdef __EMSCRIPTEN__
		ODTRACES("VICEEXT climb no: sin sitio arriba");
#endif
		return false;
	}
	// No trepar hacia el agua (26/09): saltar al mar con un borde delante
	// (rocas, muelle) arrancaba la trepada en vez del salto/nado.
	{
		float odClimbLevel = 0.0f;
		if (CWaterLevel::GetWaterLevel(destino, &odClimbLevel, true) && destino.z < odClimbLevel) {
#ifdef __EMSCRIPTEN__
			ODTRACES("VICEEXT climb no: destino en agua");
#endif
			return false;
		}
	}

	odClimbFrom = pos;
	odClimbTo = destino;
	odClimbStart = now;
	odClimbDur = (uint32)(VICEEXT_CLIMB_MIN_MS
		+ (VICEEXT_CLIMB_MAX_MS - VICEEXT_CLIMB_MIN_MS) * (borde / VICEEXT_CLIMB_MAX));
	odClimbActive = true;
	ClearAttack();
	ClearWeaponTarget();
	m_vecMoveSpeed = CVector(0.0f, 0.0f, 0.0f);
	CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_PLAYERCLIMB, ANIM_STD_CLIMB_PULL, 8.0f);
#ifdef __EMSCRIPTEN__
	{
		char t[130];
		snprintf(t, sizeof t, "VICEEXT climb start borde=%.2f ms=%u", borde, odClimbDur);
		ODTRACES(t);
	}
#endif
	return true;
}
#endif

#ifdef VICEEXT_CROUCH
// Sección 3, bloque C5 (20/09): AGACHADO (Vice Extended v1.5 "Fixed climbing,
// crouching and swimming animations").
//
// El motor tiene el camino de agachado entero (`SetDuck`/`ClearDuck`,
// `bIsDucking`, clips `DUCK_down`/`DUCK_low`/`WEAPON_crouch`) y la tecla **C** ya
// está atada a la acción `PED_DUCK`; el jugador ARMADO puede agacharse (sus
// llamadas están en el control armado). Lo que faltaba era el caso desarmado /
// cuerpo a cuerpo, que no tiene ninguna llamada, y el "caminar agachado" del
// mod: sus clips `crouch_idle`, `crouch_forward` y `crouch_backward` (grupo
// `ASSOCGRP_PLAYERCROUCH`) se superponen como animación PARCIAL, así que el
// movimiento lo sigue poniendo el motor (no se reimplementa).
// C5b (21/09): ¿ha pedido el jugador agacharse en este frame? El pad (L3) o la
// acción `PED_DUCK` con respaldo: la config de controles que el navegador guarda
// puede haber dejado la acción sin tecla, y entonces la C no llega a
// `NewState.LeftShock` (ver CControllerConfigManager::ViceExtActionKeyJustDown).
static bool
ViceExtDuckJustDown(CPad *padUsed)
{
	return padUsed->DuckJustDown()
	    || ControlsManager.ViceExtActionKeyJustDown(PED_DUCK, 'C');
}

static bool
ViceExtDuckDown(CPad *padUsed)
{
	if (padUsed->NewState.LeftShock)
		return true;
	int32 odKey = ControlsManager.GetControllerKeyAssociatedWithAction(PED_DUCK, KEYBOARD);
	if (odKey == 0 || odKey == 1056)
		odKey = 'C';
	return ControlsManager.GetIsKeyboardKeyDown((RsKeyCodes)odKey);
}

// C5b (21/09): el agachado pasa a ser de ESTE bloque, como el nado.
//
// R6 (5ª partida): el bloque le QUITABA el movimiento a pie al motor
// (`odOwnsMovement`: escribía `m_fRotationDest`/`m_fMoveSpeed` y ProcessControl
// no repartía el control normal). El giro y la velocidad reales los aplica el
// camino normal, así que el ped avanzaba distinto de cómo miraba (el "va hacia
// atrás") y la cámara, que sigue al ped, parecía fija. Ahora el movimiento lo
// pone el MOTOR (camino a pie normal: Zelda/1ª persona) y este bloque sólo:
// (a) el clip `CROUCH_*` como blend parcial y (b) el tope de velocidad.
// El agachado de disparo del motor se sigue soltando al cogerlo, para que no
// vuelva a clavar al jugador.
//
// R17 (14ª partida, 22/09): "el personaje sin desplazamiento al agacharse".
//
// Lo que lo bloqueaba, hasta la raíz: en III/VC el ped NO anda por
// `m_fMoveSpeed`. Anda por la TRASLACIÓN DE LA RAÍZ del clip de movimiento
// (`VELOCITY_EXTRACTION` -> `m_vecAnimMoveDelta` -> `CalculateNewVelocity` ->
// `m_moved` -> `UpdatePosition` -> `m_vecMoveSpeed` -> `ApplyMoveSpeed`), y
// `UpdatePosition` sólo corre con `bIsStanding`. Agachado, `SetRealMoveAnim`
// salía antes de mezclar caminar/correr (`odCrouched` -> sólo el clip del mod),
// y ese clip venía declarado `ASSOC_PARTIAL` (sin `ASSOC_MOVEMENT` ni
// `ASSOC_HAS_TRANSLATION`): no aportaba velocidad y encima su traslación se
// aplicaba a la POSE. Resultado: `m_moved` = 0, el motor anulaba `m_vecMoveSpeed`
// cada frame y el ped no andaba nada; la cámara, que sigue al ped, tampoco se
// movía. Los clips ya son de movimiento (ver `aCrouchAnimDescs`), así que el
// avance lo pone el clip.
//
// Velocidad: la natural del clip no vale (su raíz recorre 2,615 m en 0,731 s =
// 3,58 m/s, más que correr a pie). Se escala el RITMO del clip (igual que el H4
// del caminado apuntando: mismo clip, cadencia escalada, los pies no patinan)
// para que el avance sea `VICEEXT_CROUCH_WALK_SPEED`.
//
// R28 (jugador, 26/09): el agachado iba a 0,83 m/s medidos en su partida
// (`CROUCH2 velo`) contra 0,90 pedidos, y el andar de pie de serie son 1,13 m/s:
// iba al 80 % del andar y la sensación era "lentísimo". El objetivo pasa a ser
// EL MISMO que el andar de pie: 1,13 m/s.
#define VICEEXT_CROUCH_WALK_SPEED 1.13f  // m/s de avance agachado (= andar de pie)
// Metros que avanza la RAÍZ del clip en una animación entera, medidos en el
// `ped.ifp` servido con `gta_vc_browser/tools/ifp_inspect.py`. La velocidad
// natural = esto partido por `hierarchy->totalLength`.
#define VICEEXT_CROUCH_FWD_ROOT_M  2.615f
#define VICEEXT_CROUCH_BACK_ROOT_M 1.853f
// Toque de velocidad que el motor sigue mirando (m_fMoveSpeed en unidades de
// juego: 1,0 = andar), para que no quede un valor de correr colgado. R28: sube
// con el objetivo (1,13), porque con el tope en 0,90 el motor recortaba justo la
// velocidad que pide el jugador, que es la del andar de pie.
#define VICEEXT_CROUCH_SPEED 1.13f

// R20 (22/09): RITMO de los clips de agachado, y por qué ya no se recorta.
//
// En III/VC la velocidad de un paseo ES el avance de la raíz del clip (ver el
// camino completo en `AnimManager.cpp`), así que ritmo y velocidad son el mismo
// número: bajar el ritmo para clavar los "0,9 m/s" de R17 dejaba los pies a un
// cuarto de velocidad (el jugador: "va demasiado lenta"). Decisión del jugador
// (22/09): los clips como ESTÁN en el mod, a ritmo del motor (1,0):
//
//   Crouch_Forward   3,58 m/s (2,615 m / 0,731 s)   Crouch_Backward  1,85 m/s
//   Crouch_Roll_L    2,33 m/s (2,172 m / 0,931 s)   Crouch_Roll_R    2,42 m/s
//
// R22 (18ª partida, 23/09): VELOCIDAD OBJETIVO de la caminata agachada.
//
// Lo que dijo el jugador: "al estar agachado y moverme el personaje se mueve a
// una super velocidad sin razón... ajusta la velocidad del caminado como
// debería". Tenía razón y el dato estaba delante: en este motor la velocidad ES
// el avance de la raíz del clip, y los clips del mod van de 1,85 a 3,58 m/s
// (medidos con `tools/ifp_inspect.py`):
//   Crouch_Forward  3,58 m/s   Crouch_Backward 1,85   Roll_L 2,33   Roll_R 2,42
// Con la regla anterior ("el clip como está, a ritmo 1") el agachado andaba más
// que el propio `walk` de serie y ADEMÁS cada dirección iba a una velocidad
// distinta (adelante el doble que atrás), que es cosa del clip, no del juego.
//
// La referencia del propio mod para su propia familia (`Crouch_Backward` y los
// `GunMove_FWD/BWD/L/R`, su "moverse apuntando") es 1,85 m/s, pero eso es MÁS que
// el andar de pie del motor (0,79-1,13 medido en traza `PEDAT`), y el jugador lo
// tiene claro: "el estar agachado es más lento o la misma velocidad que el
// caminar". Así que el valor es **0,90 m/s** (por debajo del caminar).
// R26 (jugador): agachado = caminar menos un poquito (caminar=1 -> 0,90).
// R26d: el jugador lo fija al reves que R26: si el caminar con WASD es 1,
// el agachado es 0,90. La CADENCIA no se toca (piernas a x1,7, bien).
// R28 (26/09, jugador): cierra el vaivén de R26/R26d/R22b. El agachado va a la
// MISMA velocidad que el andar de pie: 1,13 m/s (la del `walk` de serie medida en
// el motor). El jugador es explícito: "debe ser casi la misma velocidad que el
// caminado normal". Con el ritmo de R26c (x1,7) los pies barren
// 1,13/3,75 x1,7 x3,75 = 1,92 m/s (el mismo patinaje relativo que ya validó).
#define VICEEXT_CROUCH_WALK_MPS 1.13f
#define VICEEXT_CROUCH_ROLL_MPS 2.35f
#define VICEEXT_WALK_MPS_PER_MFS 1.13f
#define VICEEXT_WALK_MAX_MFS 2.15f

// R22b (23/09, prueba del jugador): PALANCA ÚNICA EN m/s.
//
// El jugador probó la 18ª partida con los 1,85 m/s de R22 y el agachado seguía
// siendo "velocidad excesiva". Los puntos de referencia medidos, para poder
// elegir sin adivinar:
//   · andar de pie del motor (traza PEDAT del propio juego) = 0,79-1,13 m/s
//   · la familia de agachado del mod (Crouch_Backward y sus GunMove_*) = 1,85
//   · el clip Crouch_Forward a ritmo natural = 3,58 m/s (es una zancada larga)
// Cualquier valor por encima del andar de pie está mal: agachado se va más
// despacio o igual que andando. Por defecto **0,90 m/s**, que es justo el número
// que el propio jugador se puso en la 19ª partida (y que es lo que pide: "el
// estar agachado es más lento o la misma velocidad que el caminar").
//
// R24 (23/09, 19ª partida): FUERA LAS PALANCAS DE URL Y DE CONSOLA.
//
// El jugador: "quítame query params, no necesito ni pedí eso; basate en logs,
// deja las cosas en logs así puedes ver de primera mano todo". Así que la
// velocidad es esta constante y nada más, y lo único que se toca para medir es
// la TRAZA: cada segundo sale la línea `CROUCH2` con lo que pide la palanca
// (`mps`), lo que lleva el clip (`pies`), lo que mueve el motor (`mvec`) y lo que
// se ha desplazado de verdad (`avance`/`velo`, con marca de tirón `hit`).
static float
ViceExtWalkerTargetMps(void)
{
	CPad *odPad = CPad::GetPad(0);
	float odMag = 0.0f;
	if (odPad != nil) {
		float odLr = odPad->GetPedWalkLeftRight();
		float odUd = odPad->GetPedWalkUpDown();
		odMag = Sqrt(odLr * odLr + odUd * odUd);
	}
	float odMfs = Min(odMag / PAD_MOVE_TO_GAME_WORLD_MOVE, VICEEXT_WALK_MAX_MFS);
	float odMps = odMfs * VICEEXT_WALK_MPS_PER_MFS;
	return (odMps > VICEEXT_CROUCH_WALK_MPS) ? odMps : VICEEXT_CROUCH_WALK_MPS;
}

static float
ViceExtCrouchWalkMps(void)
{
	return VICEEXT_CROUCH_WALK_MPS;
}

// R20: el ángulo del mando EN EL SISTEMA DEL PED (0 = adelante; positivo =
// izquierda). Se calcula con la MISMA función que el motor usa para sus grupos
// de costado (`CPed::WorkOutHeadingForMovingFirstPerson`), así que la
// equivalencia con `m_fWalkAngle` es exacta, y se elige el clip con los mismos
// cortes que `CPlayerPed::ProcessAnimGroups`: ±50° separa adelante de costado y
// ±130° separa costado de atrás (`ASSOCGRP_PLAYERLEFT/RIGHT/BACK` del motor).
//
// R28 (jugador, 26/09): la rueda SÍ salía en el log de `crouch1` (10 eventos
// `motivo=ok`, ang=±90) pero no se VEÍA: con la pose de apuntar a peso 1 el clip
// de la rueda entraba multiplicado por `1 - 1 = 0` (ver el presupuesto de
// parciales en `ViceExtCrouchAimPose`). Además del peso, el corte de 50° exigía
// el costado "puro" y con W+A / W+D (45°) no rodaba. El mod no exige el costado
// puro: rueda con el botón de costado de la cruceta (`IS_BUTTON_PRESSED 0,10` /
// `0,11` de su `.cs`) manteniendo apuntar y sin disparar. En teclado el costado
// es A/D, así que el corte baja a 25° y las diagonales también ruedan.
#define VICEEXT_CROUCH_SIDE_DEG  25.0f
#define VICEEXT_CROUCH_BACK_DEG 130.0f

// R21b: clip de agachado elegido en el último frame (ver `ViceExtCrouchClipSpeed`).
static AnimationId s_odCrouchClip = ANIM_STD_CROUCH_IDLE;


static float
ViceExtCrouchPadAngle(CPad *padUsed)
{
	float leftRight = padUsed ? padUsed->GetPedWalkLeftRight() : 0.0f;
	float upDown = padUsed ? padUsed->GetPedWalkUpDown() : 0.0f;
	if (leftRight == 0.0f && upDown == 0.0f)
		return 0.0f;
	return CGeneral::GetRadianAngleBetweenPoints(0.0f, 0.0f, -leftRight, upDown);
}

// R26 (jugador + SA + handoff 6.5): a los lados y sin apuntar, el cuerpo GIRA
// hacia el avance (como de pie). El rumbo se guarda en el MUNDO (camara + mando)
// y no en el cuerpo: con raton, girar m_fRotationCur sin mas realimenta el
// calculo del rumbo (walkAngle = Cur + angulo) y el ped andaria en circulo.
// Asi el objetivo es fijo mientras el mando y la camara no se muevan, y el
// giro lo hace el motor a su ritmo (STAT_PLAYER: 15 grados/frame). Apuntando
// no se gira (ahi se encara y se rueda/dispara de lado: R21).
// R29 (27/09, sintoma F): gira en las 8 direcciones, no solo el costado
// 50-130 grados (el "el personaje no gira hacia la direccion que camina" del
// jugador). Solo el ATRAS PURO no gira (decision D3: `GunCrouchBwd` sin girar).
// R26b: 0 = girar; si no, el motivo (sale en CROUCH2 como `giro=`).
static int odSideWhy = 0;
bool
CPlayerPed::ViceExtCrouchSideHeading(float &odDir)
{
	odSideWhy = 0;
	if (!odCrouched) { odSideWhy = 2; return false; }
	// R29 (G): apuntando NO se gira (el encaramiento lo manda el axis), y se lee
	// por la API unica: con punos el pad "apunta" pero no hay mira
	// (`ViceExtIsAiming`).
	if (ViceExtIsAiming()) { odSideWhy = 3; return false; }
	if (!s_odMove.prepared || !s_odMove.moving || s_odMove.policy != 0) { odSideWhy = 4; return false; }
	odDir = ViceExtMoveHeading();
	return true;
}

// R15 (13ª partida, 22/09): el clip de agachado NO se retiraba al desagacharse.
//
// Síntoma del jugador: "estando agachado le vuelvo a dar para pararse y sólo
// cambia la altura de la cámara: el personaje sigue agachado".
//
// Causa raíz (leída en `CAnimManager::BlendAnimation`): el barrido que retira
// las animaciones que había puestas sólo mira las del MISMO tipo
// (`isPartial == anim->IsPartial()`). Los clips del mod (`crouch_*`) son
// `ASSOC_PARTIAL` y viven en su propio grupo (`ASSOCGRP_PLAYERCROUCH`), pero de
// pie el motor mezcla caminar/idle NO parciales de `ASSOCGRP_STD`: nadie apagaba
// el clip de agachado, así que el cuerpo seguía agachado (el clip `ASSOC_REPEAT`
// se queda puesto para siempre). Y como el único dato que dependía de
// `odCrouched` era el objetivo de altura de la cámara (`CCam` vía
// `ViceExtIsCrouched`), el jugador veía exactamente eso: "sólo cambia la altura
// de la cámara".
//
// Arreglo: al desagacharse se retiran los clips de agachado igual que hace el
// motor en `CPed::ClearDuck` (`ASSOC_DELETEFADEDOUT` + `blendDelta = -4.0f`),
// de modo que el caminar/idle del motor recupera el cuerpo en ~0,25 s.
static uint32 s_odCrouchOffTime = 0;
static uint32 s_odCrouchOnTime = 0;

// R22: contador de sesiones de agachado (cada vez que se agacha o se levanta).
// Lo usa la traza `CROUCH2` para no comparar la posición de una sesión con la de
// la anterior: la primera muestra de cada sesión salía con un `avance` absurdo
// (medido en la partida del jugador: 84,70 m y "velo=137,34") porque el punto de
// referencia guardado era de otra parte del mapa. Eso ensuciaba la medida de la
// velocidad justo en los primeros metros, que son los que se quieren medir.
static uint32 s_odCrouchSession = 0;
static bool s_odCrouchKeyHeld = false;

// R21 (23/09, 17ª partida): APUNTAR AGACHADO — la pose la trae el propio mod.
//
// Síntoma del jugador: "falta que al apuntar estando agachado haga su respectiva
// animación de apuntado".
//
// La pose existe y es del mod: el motor la llama `ANIM_STD_DUCK_WEAPON` y en la
// tabla de nombres de animación es "WEAPON_crouch"; el `ped.ifp` que sirve el
// navegador es el del mod (comprobado por md5), así que ese clip ES la animación
// de apuntar agachado del mod (el `WEAPON_crouch` de San Andreas).
//
// Lo que faltaba es que alguien la PIDIERA: el motor sólo la pone desde
// `CPed::SetDuck` (rama `bCrouchWhenShooting`), y ese camino marca
// `bIsDucking`, que clava al jugador en el sitio — por eso R6 lo limpia al
// agacharse, y con él se iba también la pose. Aquí se mezcla la MISMA pose por
// el camino de animación, sin tocar `bIsDucking`:
//   · el jugador puede seguir andando agachado (el movimiento lo lleva R20c),
//   · el IK del brazo sigue activo (`CPed::AimGun` lo apaga sólo si `bIsDucking`),
//   · y es un blend PARCIAL (`ASSOCGRP_STD`: `ASSOC_PARTIAL` en su descriptor),
//     así que se mezcla ENCIMA del clip de agachado (no parcial), igual que el
//     motor mezcla la pose de apuntar de pie encima del caminar.
//
// El clip no es `ASSOC_REPEAT`: se reproduce una vez y se queda en el último
// fotograma (`CAnimBlendAssociation::UpdateTime` suelta `ASSOC_RUNNING` al
// llegar al final), que es la pose de apuntar. Por eso NO se vuelve a pedir cada
// frame: `CAnimManager::BlendAnimation` reinicia el clip cuando ha terminado
// (`if(!found->IsRunning() && found->currentTime == totalLength) found->Start(0)`)
// y la pose se quedaría repitiendo la transición. Se pide sólo si no está o si
// se está apagando —el mismo patrón que `CPed::SetPointGunAt`—, y al terminar se
// le quita `ASSOC_FADEOUTWHENDONE` (como hace `SetDuck`) para que se quede
// puesta en vez de borrarse.
static const AnimationId odCrouchAimPoseAnim = ANIM_STD_DUCK_WEAPON;

static AnimationId
ViceExtCrouchAimPoseClip(CPlayerPed *ped)
{
	CWeapon *odW = ped->GetWeapon();
	CWeaponInfo *odInfo = odW ? CWeaponInfo::GetWeaponInfo(odW->m_eWeaponType) : nil;
	if (odInfo) {
		AnimationId odCrouchFire = CPlayerPed::GetCrouchFireAnim(odInfo);
		if (odCrouchFire)
			return odCrouchFire;
		if (odInfo->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM))
			return CPlayerPed::GetPrimaryFireAnim(odInfo);
	}
	return odCrouchAimPoseAnim;
}

static AssocGroupId
ViceExtCrouchAimPoseGroup(CPlayerPed *ped)
{
	CWeapon *odW = ped->GetWeapon();
	CWeaponInfo *odInfo = odW ? CWeaponInfo::GetWeaponInfo(odW->m_eWeaponType) : nil;
	if (odInfo && ViceExtCrouchAimPoseClip(ped) != odCrouchAimPoseAnim)
		return odInfo->m_AnimToPlay;
	return ASSOCGRP_STD;
}

static float
ViceExtCrouchAimPoseHold(CPlayerPed *ped)
{
	CWeapon *odW = ped->GetWeapon();
	CWeaponInfo *odInfo = odW ? CWeaponInfo::GetWeaponInfo(odW->m_eWeaponType) : nil;
	if (odInfo == nil)
		return 0.0f;
	if (odInfo->IsFlagSet(WEAPONFLAG_CROUCHFIRE))
		return odInfo->m_fAnim2LoopStart;
	return odInfo->m_fAnimLoopStart;
}

// R21: la pose de apuntar agachado VA PUESTA (es lo que pidió el jugador: "al
// apuntar agachado, su respectiva animación de apuntado"). R24: su palanca de
// URL/consola se quitó con las demás, así que ya no hay nada que la apague.
// R21b (23/09): velocidad natural (m/s) de cada clip de agachado del mod, medida
// en su `ped.ifp` con `tools/ifp_inspect.py --curva` (avance de la RAÍZ por ciclo
// dividido por la duración del ciclo; = lo que da el clip a ritmo 1).
//
// R27 (26/09): las medidas de abajo son las del `ped.ifp` SERVIDO, que desde R27
// lleva los dos clips del sa-crouch para adelante/atrás (`GunCrouchFwd`/`GunCrouchBwd`,
// ±2,740 m / 0,731 s); los de costado siguen siendo los mismos.
//
// Hace falta porque el motor no siempre deja ese avance disponible: apuntando,
// `CPed::SetPointGunAt` fuerza `SetMoveState(PEDMOVE_STILL)` y además la pose de
// apuntar es un blend PARCIAL a peso 1, y el reparto de la raíz
// (`FrameUpdateCallBackWithVelocityExtractionSkinned`) multiplica la traslación
// de los clips no parciales por `1 - totalBlendAmount` = 0. Resultado medido en
// `ve42`: apuntando agachado y con el mando de lado el ped se movía **0,00 m/s**
// mientras el clip pedido era `Crouch_Roll_L` (o sea: la pose y la dirección
// estaban bien, se perdía el avance). El desplazamiento por código de R20c usa
// esta velocidad cuando el motor no le da ninguna.
static float
ViceExtCrouchClipSpeed(AnimationId anim)
{
	switch (anim) {
	case ANIM_STD_CROUCH_FORWARD:  return 2.05f;   // R29: 1,50 m / 0,731 s = 2,05 m/s (raiz reescalada, E1)
	case ANIM_STD_CROUCH_BACKWARD: return 2.05f;   // R29: -1,50 m / 0,731 s (simetrico)
	case ANIM_STD_CROUCH_AIMFWD:   return 1.85f;   // GunMove_FWD  1,854 m / 1,000 s (E3.9)
	case ANIM_STD_CROUCH_AIMBWD:   return 1.80f;   // GunMove_BWD  1,853 m / 1,031 s
	case ANIM_STD_CROUCH_AIMLEFT:  return 1.80f;   // GunMove_L    1,803 m / 1,000 s (raiz en X)
	case ANIM_STD_CROUCH_AIMRIGHT: return 1.80f;   // GunMove_R    1,803 m / 1,000 s
	case ANIM_STD_CROUCH_LEFT:     return 2.33f;   // 2,172 m / 0,931 s
	case ANIM_STD_CROUCH_RIGHT:    return 2.42f;   // 2,253 m / 0,931 s
	default:                       return 0.0f;    // parado (`Crouch_Idle`)
	}
}

// R22: velocidad objetivo (m/s) del clip en uso a ritmo 1 — la rueda conserva
// la suya (es un movimiento rápido de esquiva, no un paso).
static float
ViceExtCrouchTargetSpeed(AnimationId anim)
{
	if (anim == ANIM_STD_CROUCH_LEFT || anim == ANIM_STD_CROUCH_RIGHT)
		return VICEEXT_CROUCH_ROLL_MPS;
	return ViceExtCrouchWalkMps();
}

// R22: RITMO del clip para que sus pies avancen exactamente lo que avanza el
// ped: `ritmo = velocidad objetivo / velocidad de la raíz del clip`. Es lo que
// hace SA (`relSpeed` del clip = velocidad del ped) y es lo único que evita las
// dos formas de que se vea mal: a ritmo bajo, pies en cámara lenta; a ritmo alto
// (o con el clip de adelante, 3,75 m/s, sin escalar), pies patinando y super velocidad.
//
//   GunCrouchFwd     1,13 / 2,05 = 0,55   (R29: raiz reescalada a 1,5 m/ciclo, E1)
//   GunCrouchBwd     1,13 / 2,05 = 0,55   (R29: simetrico, pies pegados)
//   Crouch_Roll_L    2,35 / 2,33 = 1,01     Crouch_Roll_R   2,35 / 2,42 = 0,97 (rueda: esquiva, sin impulso)
// R26c (jugador): las piernas iban a 0,28 (zancada de 2,6 m a 1 m/s = camara
// lenta) y pide cadencia de pasos mas alta SIN subir el cuerpo. Fisica: a
// igual zancada, mas cadencia = los pies barren mas que el avance (patinan
// xCADENCIA). Se iguala a la cadencia del andar de pie (ciclo ~1,5 s):
// 0,28 x 1,70 = 0,48. Solo caminar (la rueda va a ritmo natural).
// R29 (27/09, D1=(a)): SIN multiplicador. La raiz de `GunCrouchFwd/Bwd` se
// reescala en el dato servido (E1: 2,74 -> 1,5 m/ciclo = cadencia de andar con
// los pies pegados: rate 0,55 y zancada cada 1,33 s) y el x1,7 de R26c (que
// patinaba x1,7 por construccion) se retira.
#define VICEEXT_CROUCH_CADENCE 1.0f
static float
ViceExtCrouchRateFor(AnimationId anim)
{
	float odRoot = ViceExtCrouchClipSpeed(anim);
	if (odRoot <= 0.01f)
		return 1.0f;                     // `Crouch_Idle`: en el sitio
	float odRate = ViceExtCrouchTargetSpeed(anim) / odRoot;
	if (anim == ANIM_STD_CROUCH_FORWARD || anim == ANIM_STD_CROUCH_BACKWARD)
		odRate *= VICEEXT_CROUCH_CADENCE;   // R29: sin multiplicador (la raiz servida ya es la de andar)
	// R22b: el tope bajo era 0,25 y recortaba la velocidad por debajo de ~0,9 m/s
	// (con `Crouch_Forward`, 3,58 m/s de raíz: 0,25 × 3,58 = 0,895). El objetivo
	// tiene que mandar, no el tope.
	if (odRate < 0.04f) odRate = 0.04f;
	if (odRate > 1.6f) odRate = 1.6f;
	return odRate;
}

// R28 (26/09, build `crouch2`): EL PRESUPUESTO DE LAS ANIMACIONES PARCIALES.
//
// Es la raiz comun de tres de los sintomas del jugador tras jugar `crouch1`: el
// cuerpo estirado/aplastado de su captura, la rueda que no se ve ("no rueda al
// apuntar") y el "la animacion de disparar agachado lucha entre 2 animaciones".
//
// Como reparte el motor (`FrameUpdateCallBackSkinned` + `CAnimBlendNode::Update`
// + `CAnimBlendAssociation::GetBlendAmount`):
//   - las animaciones PARCIALES (la pose de apuntar, el clip del arma) entran
//     con su peso tal cual (`IsPartial()` devuelve `blendAmount`, sin multiplicar),
//   - las de MOVIMIENTO (caminar y los clips de agachado del mod, que llevan
//     `ASSOC_MOVEMENT`) multiplican su peso por `1 - suma de parciales`,
//   - y la pose del hueso es la SUMA de cuaterniones con esos pesos, con una
//     normalizacion al final.
// De ahi los tres desastres, los tres vistos en `crouch1`:
//   - suma > 1: las de movimiento entran en NEGATIVO y, si las dos parciales van
//     en sentidos distintos, la suma puede quedar casi nula: al normalizar sale
//     una matriz degenerada y el cuerpo aparece ESTIRADO o APLASTADO (la captura
//     del jugador). Es exactamente nuestra pose a peso 1 mas el clip del arma a
//     peso 1, que es lo que pasaba apuntando agachado.
//   - suma = 1: toda animacion de movimiento vale 0, asi que las PIERNAS del
//     agachado y el clip de la RUEDA desaparecen: el jugador ve al ped
//     deslizarse con las piernas congeladas y dice "no rueda".
//   - dos parciales a la vez: se pelean hueso a hueso y encima con
//     realimentacion, porque `CAnimManager::BlendAnimation` retira (funde) TODA
//     parcial ajena que encuentra (`isPartial == anim->IsPartial()`, sin mirar el
//     grupo: ver AnimManager.cpp), incluido el clip del arma; el motor lo vuelve
//     a pedir en el frame siguiente y asi indefinidamente = la "lucha entre 2
//     animaciones" que reporta el jugador al mantener el gatillo.
//
// Regla que se implanta aqui: nuestra pose NUNCA compite.
//   - Si el motor tiene puesta su propia parcial (el clip del arma), la pose se
//     retira y NO se vuelve a pedir. El cuerpo lo lleva el clip del arma, que
//     desde R28 es el de AGACHADO (`CPed::ViceExtCrouchShooting` ->
//     `colt45_crouchfire` en vez de `colt45_fire`).
//   - Si no hay ninguna, la pose entra con lo que SOBRA del presupuesto
//     (`1 - parciales ajenas`), y andando se queda en 0,4 para que las piernas
//     del clip de agachado sigan mandando (R22).
//   - Rodando se retiran TODAS las parciales: la rueda del mod mueve el cuerpo
//     entero y con una pose a peso 1 no se veria.
static float s_odCrouchPoseOthers = 0.0f;      // suma de parciales ajenas (`otros=` de CROUCH2)
static const char *s_odCrouchPoseWho = "-";    // clip que lleva la pose (`pose=`)
static float s_odCrouchWeaponW = 0.0f;         // parcial del arma mas pesada (`pesoarma=`)
static const char *s_odCrouchWeaponNom = "-";  // y su nombre (`nomarma=`)

// R28: suma de las parciales AJENAS a la nuestra (las que de verdad ocupan
// cuerpo, incluidas las que aun se estan apagando). La comparacion es por PUNTERO
// y no por `animId`: los ids son del grupo (los `ANIM_STD_*` y los del arma
// comparten numeros) y compararlos metia clips del arma en la cuenta de la pose.
static float
ViceExtCrouchOtherPartials(RpClump *odClump, CAnimBlendAssociation *odOurs)
{
	float odSum = 0.0f;
	for (CAnimBlendAssociation *odA = RpAnimBlendClumpGetFirstAssociation(odClump);
	     odA; odA = RpAnimBlendGetNextAssociation(odA)) {
		if (odA == odOurs)
			continue;
		if (!odA->IsPartial())
			continue;
		if (odA->blendAmount > 0.0f)
			odSum += odA->blendAmount;
	}
	return odSum;
}

// R28: la parcial ajena MAS PESADA y su nombre. Es la medida del disparo
// agachado: con el arma en mano y el gatillo pulsado, `nomarma` tiene que ser el
// `*_crouchfire` del arma (`colt45_crouchfire`), no el `colt45_fire` de pie.
static float
ViceExtCrouchHeaviestPartial(RpClump *odClump, CAnimBlendAssociation *odOurs, const char **odName)
{
	float odBest = 0.0f;
	*odName = "-";
	for (CAnimBlendAssociation *odA = RpAnimBlendClumpGetFirstAssociation(odClump);
	     odA; odA = RpAnimBlendGetNextAssociation(odA)) {
		if (odA == odOurs || !odA->IsPartial() || odA->blendDelta < 0.0f)
			continue;
		if (odA->blendAmount > odBest) {
			odBest = odA->blendAmount;
			*odName = (odA->hierarchy && odA->hierarchy->name) ? odA->hierarchy->name : "?";
		}
	}
	return odBest;
}

// R28: la rueda necesita el cuerpo entero, asi que mientras dura se retiran
// (fundido rapido) todas las parciales: la pose del mod y la del arma. Sin esto
// el clip de la rueda (de movimiento) entra multiplicado por `1 - 1 = 0` y el
// jugador no ve ninguna rueda (su queja del 26/09).
static void
ViceExtCrouchRollClearPartials(CPlayerPed *odPed)
{
	for (CAnimBlendAssociation *odA = RpAnimBlendClumpGetFirstAssociation(odPed->GetClump());
	     odA; odA = RpAnimBlendGetNextAssociation(odA)) {
		if (!odA->IsPartial() || odA->blendDelta < 0.0f)
			continue;
		odA->flags |= ASSOC_DELETEFADEDOUT;
		odA->blendDelta = -8.0f;
	}
}

// R21: al dejar de apuntar (o al desagacharse) la pose se retira con el mismo
// fundido que usa el motor (`ASSOC_DELETEFADEDOUT` + `blendDelta = -4`), el
// mismo camino que `CPed::ClearDuck` y que R15 usa para los clips de agachado.
static void
ViceExtCrouchAimPoseOff(CPlayerPed *ped)
{
	const AnimationId odIds[3] = { odCrouchAimPoseAnim, ANIM_WEAPON_FIRE, ANIM_WEAPON_CROUCHFIRE };
	for (int i = 0; i < 3; i++) {
		CAnimBlendAssociation *odPose = RpAnimBlendClumpGetAssociation(ped->GetClump(), odIds[i]);
		if (odPose && odPose->blendDelta >= 0.0f) {
			odPose->flags |= ASSOC_DELETEFADEDOUT;
			odPose->blendDelta = -8.0f;
		}
	}
}

static void
ViceExtCrouchAimPose(CPlayerPed *odPed, bool odWalking, bool odRolling)
{
	AnimationId odCrouchPoseClip = ViceExtCrouchAimPoseClip(odPed);
	AssocGroupId odCrouchPoseGroup = ViceExtCrouchAimPoseGroup(odPed);
	float odCrouchPoseHold = ViceExtCrouchAimPoseHold(odPed);
	CAnimBlendAssociation *odPose = RpAnimBlendClumpGetAssociation(odPed->GetClump(), odCrouchPoseClip);

	// R28: la rueda manda sobre todo lo demas (ver el comentario de arriba).
	if (odRolling) {
		s_odCrouchPoseOthers = 0.0f;
		s_odCrouchPoseWho = "-";
		s_odCrouchWeaponW = 0.0f;
		s_odCrouchWeaponNom = "-";
		ViceExtCrouchRollClearPartials(odPed);
		return;
	}

	// R28: el presupuesto. Si el arma tiene puesta su parcial, la pose se retira y
	// NO se vuelve a pedir en este frame: pedirla (`BlendAnimation`) retiraria la
	// del arma y empezaria la "lucha entre 2 animaciones".
	float odOthers = ViceExtCrouchOtherPartials(odPed->GetClump(), odPose);
	s_odCrouchPoseOthers = odOthers;
	s_odCrouchWeaponW = 0.0f;
	s_odCrouchWeaponNom = "-";
	if (odOthers > 0.01f) {
		s_odCrouchPoseWho = "-";
		s_odCrouchWeaponW = ViceExtCrouchHeaviestPartial(odPed->GetClump(), odPose, &s_odCrouchWeaponNom);
		ViceExtCrouchAimPoseOff(odPed);
		return;
	}
	float odBudget = Max(1.0f - odOthers, 0.0f);   // lo que queda libre del reparto

	if (!odPose || odPose->blendDelta < 0.0f) {
		// R29 (C18-4, spec del mod): `BlendAnimation(..., 4.0)` como en
		// classicaxis Main.cpp:1393-1415 (antes 8,0 aqui).
		odPose = CAnimManager::BlendAnimation(odPed->GetClump(), odCrouchPoseGroup, odCrouchPoseClip, 4.0f);
		if (odPose)
			odPose->flags &= ~ASSOC_FADEOUTWHENDONE;
	}
	if (!odPose)
		return;
	if (odCrouchPoseHold > 0.0f && odPose->currentTime > odCrouchPoseHold * 0.4f) {
		odPose->SetCurrentTime(odCrouchPoseHold);
		odPose->flags &= ~ASSOC_RUNNING;
	}
	s_odCrouchPoseWho = (odPose->hierarchy && odPose->hierarchy->name) ? odPose->hierarchy->name : "WEAPON_crouch";

	// R22 (18a partida): ANDANDO, la pose no llega a peso completo.
	//
	// Por que: `ANIM_STD_DUCK_WEAPON` es un clip PARTIAL y el reparto de la raiz
	// de `FrameUpdateCallBackWithVelocityExtractionSkinned` multiplica los clips
	// no parciales por `1 - totalBlendAmount`. Con la pose a peso 1 las PIERNAS
	// del agachado valen 0: el cuerpo lo pone entero la pose (que es una pose
	// quieta) y el jugador ve a Tommy deslizandose con las piernas congeladas
	// ("la animación de apuntar no es la que debería tener"). Con la pose a 0,4
	// andando, las piernas del clip de agachado siguen mandando (0,6) y los
	// brazos y el torso llevan la pose de apuntar del mod. Quieto la pose vuelve
	// a peso completo (es una pose, no un movimiento).
	const float odWalkWeight = 0.4f;
	float odWant = odWalking ? odWalkWeight : 1.0f;
	if (odWant > odBudget)
		odWant = odBudget;
	if (odPose->blendAmount > odBudget) {
		odPose->blendAmount = odBudget;
		odPose->blendDelta = 0.0f;
	} else if (odPose->blendAmount < odWant) {
		odPose->blendDelta = 4.0f;
	} else if (odPose->blendAmount > odWant && odPose->blendDelta == 0.0f) {
		odPose->blendDelta = -4.0f;
	}
}

static void
ViceExtCrouchRollFinish(CAnimBlendAssociation *odR, bool odFin, const char *odCause)
{
	if (s_odRollState != ODROLL_ACTIVE)
		return;
	float odProgress = 0.0f;
	if (odR && odR->hierarchy && odR->hierarchy->totalLength > 0.0001f) {
		odProgress = odR->currentTime / odR->hierarchy->totalLength;
		if (odProgress > 1.0f)
			odProgress = 1.0f;
	} else if (s_odRollLength > 0.0001f) {
		odProgress = s_odRollLastCurrent / s_odRollLength;
	}
	if (odFin && odProgress < 0.999f)
		odFin = false;
#ifdef __EMSCRIPTEN__
	{
		char t[520];
		const char *odClipNom = (odR && odR->hierarchy && odR->hierarchy->name) ? odR->hierarchy->name : "Crouch_Roll";
		if (odFin)
		VICEEXT_P4_EMIT(s_odMove.gen, t,  "P4 kind=roll_end schema=1 gen=%u frame=%u sim=%.4f case=5 id=%u side=%s clip=%s current=%.4f progress=%.4f rateIntegral=%.4f travel=%.4f maxWeight=%.4f simStart=%.4f simElapsed=%.4f stepM=%.4f stepT=%.4f stepN=%u",
			(unsigned)s_odMove.gen, (unsigned)CTimer::GetFrameCounter(), CTimer::GetTimeInMilliseconds() * 0.001f,
			(unsigned)s_odRollId, s_odRollSide < 0 ? "L" : "R", odClipNom,
			odR ? odR->currentTime : s_odRollLastCurrent, odProgress,
			s_odRollRate, s_odRollRate * s_odRollSpeedMps, s_odRollMaxW, s_odRollSimStart, CTimer::GetTimeInMilliseconds() * 0.001f - s_odRollSimStart,
			s_odRollStepM, s_odRollStepT, s_odRollStepN);
		else
			VICEEXT_P4_EMIT(s_odMove.gen, t,  "P4 kind=roll_cancel schema=1 gen=%u frame=%u sim=%.4f case=5 id=%u side=%s cause=%s progress=%.4f current=%.4f rateIntegral=%.4f",
				(unsigned)s_odMove.gen, (unsigned)CTimer::GetFrameCounter(), CTimer::GetTimeInMilliseconds() * 0.001f,
				(unsigned)s_odRollId, s_odRollSide < 0 ? "L" : "R", odCause, odProgress,
				odR ? odR->currentTime : s_odRollLastCurrent, s_odRollRate);
		
	}
#endif
	s_odRollState = ODROLL_READY;
	s_odRollClip = ANIM_STD_CROUCH_IDLE;
	s_odRollLastCurrent = 0.0f;
	s_odRollRate = 0.0f;
	s_odRollMaxW = 0.0f;
	s_odRollArmed = s_odRollNeutral;
}

static void
ViceExtCrouchRollStart(CPlayerPed *odPed, bool odIzq, AnimationId odClip)
{
	s_odRollState = ODROLL_ACTIVE;
	s_odRollArmed = false;
	s_odRollSimStart = CTimer::GetTimeInMilliseconds() * 0.001f;
	++s_odRollId;
	s_odRollSide = odIzq ? -1 : 1;
	s_odRollClip = odClip;
	s_odRollSpeedMps = ViceExtCrouchTargetSpeed(odClip);
	s_odRollLastCurrent = 0.0f;
	s_odRollRate = 0.0f;
	s_odRollMaxW = 0.0f;
	s_odRollStepPos = odPed->GetPosition();
	s_odRollStepM = 0.0f;
	s_odRollStepT = 0.0f;
	s_odRollStepN = 0;
	if (ViceExtMovePrepared(odPed) && s_odMove.moving) {
		s_odRollDirX = s_odMove.dirX;
		s_odRollDirY = s_odMove.dirY;
	} else {
		float odFx = s_odMove.baseX, odFy = s_odMove.baseY;
		float odLx = -odFy, odLy = odFx;
		if (odIzq) { s_odRollDirX = odLx; s_odRollDirY = odLy; }
		else { s_odRollDirX = -odLx; s_odRollDirY = -odLy; }
	}
}

static void
ViceExtCrouchRollStarted(CAnimBlendAssociation *odAssoc)
{
#ifdef __EMSCRIPTEN__
	{
		char t[520];
		char odEdge[24];
		int odWep = (s_odMove.owner != nil && s_odMove.owner->GetWeapon() != nil) ? (int)s_odMove.owner->GetWeapon()->m_eWeaponType : 0;
		snprintf(odEdge, sizeof odEdge, "%s%u", s_odRollSide < 0 ? "A" : "D", (unsigned)++s_odRollEdgeSeq);
		const char *odClipNom = (odAssoc && odAssoc->hierarchy && odAssoc->hierarchy->name) ? odAssoc->hierarchy->name : "Crouch_Roll";
		VICEEXT_P4_EMIT(s_odMove.gen, t,  "P4 kind=roll_start schema=1 gen=%u frame=%u sim=%.4f case=5 id=%u side=%s edge=%s aim=%d crouch=%d weapon=%d req=%.6f,%.6f,0.000000 clip=%s length=%.4f speed=%.4f",
			(unsigned)s_odMove.gen, (unsigned)CTimer::GetFrameCounter(), CTimer::GetTimeInMilliseconds() * 0.001f,
			(unsigned)s_odRollId, s_odRollSide < 0 ? "L" : "R", odEdge,
			s_odMove.aim, s_odMove.crouch, odWep,
			s_odRollDirX, s_odRollDirY, odClipNom, s_odRollLength, 1.0f);
		
	}
#endif
}

static void
ViceExtCrouchStopAnims(CPlayerPed *ped)
{
	if (s_odRollState == ODROLL_ACTIVE)
		ViceExtCrouchRollFinish(nil, false, "postura");
#ifdef __EMSCRIPTEN__
	if (!s_odP4PoseExitOn)
		ViceExtP4PoseBeginP4(ped, 0);
	s_odPoseExitSample = 0;
#endif
	float odCrouchSum = 0.0f;
	for (int i = 0; i < 9; i++) {
		CAnimBlendAssociation *odC = RpAnimBlendClumpGetAssociation(ped->GetClump(), odCrouchPoseIds[i]);
		if (odC && odC->blendAmount > 0.0f)
			odCrouchSum += odC->blendAmount;
	}
	float odMoveSum = 0.0f;
	for (CAnimBlendAssociation *odN = RpAnimBlendClumpGetFirstAssociation(ped->GetClump());
	     odN; odN = RpAnimBlendGetNextAssociation(odN)) {
		if (!odN->IsPartial() && odN->blendAmount > 0.0f)
			odMoveSum += odN->blendAmount;
	}
	float odLibre = 1.0f - (odMoveSum - odCrouchSum);
	if (odLibre < 0.0f)
		odLibre = 0.0f;
	int odCleared = 0;
	for (int i = 0; i < 9; i++) {
		CAnimBlendAssociation *a = RpAnimBlendClumpGetAssociation(ped->GetClump(), odCrouchPoseIds[i]);
		if (a && a->blendDelta >= 0.0f) {
			if (a->blendAmount > odLibre)
				a->blendAmount = odLibre;
			a->flags |= ASSOC_DELETEFADEDOUT;
			a->blendDelta = -8.0f;
			++odCleared;
		}
	}
	// R21: la pose de apuntar agachado también se retira (si no, el cuerpo se
	// queda con la pose del arma puesta tras levantarse).
	ViceExtCrouchAimPoseOff(ped);
	++s_odCrouchSession;                 // R22: nueva sesión de agachado
	s_odCrouchOffTime = CTimer::GetTimeInMilliseconds();
#ifdef __EMSCRIPTEN__
	{
		char t[420];
		char odDet[220];
		int odDetLen = 0;
		float odWorstW = -1.0f;
		float odWorstD = 0.0f;
		odDet[0] = '\0';
		for (int i = 0; i < 9; i++) {
			CAnimBlendAssociation *odD = RpAnimBlendClumpGetAssociation(ped->GetClump(), odCrouchPoseIds[i]);
			float odDw = (odD != nil && odD->blendAmount > 0.0f) ? odD->blendAmount : 0.0f;
			if (odD != nil && odDw > odWorstW) {
				odWorstW = odDw;
				odWorstD = odD->blendDelta;
			}
			int odRet = snprintf(odDet + odDetLen, sizeof(odDet) - odDetLen, "%s%.2f", i ? "," : "", odDw);
			if (odRet < 0 || odRet >= (int)(sizeof(odDet) - odDetLen))
				break;
			odDetLen += odRet;
		}
		snprintf(t, sizeof t, "VICEEXT crouch pose off clips=%d det=%s dd=%.4f", odCleared, odDet, odWorstD);
		ODTRACES(t);
	}
#endif
}

// R15: vigilante de la pose. Mientras NO se esté agachado, si algún clip de
// agachado sigue mezclado (peso > 0,01) se deja una línea por segundo con los
// pesos reales y los ms desde el desagachado. Es la prueba desde el log de que
// el cuerpo vuelve a estar DE PIE: con el fundido de 0,25 s, cualquier línea
// con `desde >= 700` tiene que salir con los tres pesos a 0.
#ifdef __EMSCRIPTEN__
static void
ViceExtP4PartialWatch(CPlayerPed *odPed)
{
	static uint32 s_odNextPart = 0;
	static uint32 s_odPartEdge = 0;
	uint32 odNow = CTimer::GetTimeInMilliseconds();
	if (odNow < s_odNextPart && odNow + 60000 >= s_odNextPart)
		return;
	int odAim = odPed->ViceExtIsAiming() ? 1 : 0;
	if (odAim)
		s_odPartEdge = 0;
	else if (s_odPartEdge == 0)
		s_odPartEdge = odNow;
	uint32 odRel = (s_odPartEdge != 0 && odNow >= s_odPartEdge) ? (odNow - s_odPartEdge) : 0;
	float odCrouchMax = 0.0f;
	for (int i = 0; i < 9; i++) {
		CAnimBlendAssociation *odC = RpAnimBlendClumpGetAssociation(odPed->GetClump(), odCrouchPoseIds[i]);
		float odW = odC ? odC->blendAmount : 0.0f;
		if (odW > odCrouchMax)
			odCrouchMax = odW;
	}
	const char *odNom1 = "-";
	const char *odNom2 = "-";
	float odW1 = 0.0f;
	float odW2 = 0.0f;
	int odN = 0;
	for (CAnimBlendAssociation *odA = RpAnimBlendClumpGetFirstAssociation(odPed->GetClump());
	     odA; odA = RpAnimBlendGetNextAssociation(odA)) {
		if (!odA->IsPartial() || odA->blendAmount <= 0.0f)
			continue;
		++odN;
		const char *odNom = (odA->hierarchy && odA->hierarchy->name) ? odA->hierarchy->name : "?";
		if (odA->blendAmount > odW1) {
			odW2 = odW1;
			odNom2 = odNom1;
			odW1 = odA->blendAmount;
			odNom1 = odNom;
		} else if (odA->blendAmount > odW2) {
			odW2 = odA->blendAmount;
			odNom2 = odNom;
		}
	}
	if (odN == 0 && odCrouchMax <= 0.01f && (odRel == 0 || odRel > 1500))
		return;
	s_odNextPart = odNow + (((odN > 0) || (odRel > 0 && odRel <= 1500)) ? 100 : 1000);
	char t[420];
	snprintf(t, sizeof t, "P4 kind=partials schema=1 gen=1 frame=%u sim=%.4f case=2 id=0 aim=%d crouch=%d desde=%u n=%d uno=%s w1=%.3f dos=%s w2=%.3f cgroup=%.3f",
		(unsigned)CTimer::GetFrameCounter(), odNow * 0.001f, odAim, odPed->ViceExtIsCrouched() ? 1 : 0,
		odRel, odN, odNom1, odW1, odNom2, odW2, odCrouchMax);
	ODTRACES(t);
}
#endif

static void
ViceExtCrouchPoseTrace(CPlayerPed *ped)
{
#ifdef __EMSCRIPTEN__
	ViceExtP4PartialWatch(ped);
	float odW[9];
	float odPose = 0.0f;
	float odMaxW = 0.0f;
	for (int i = 0; i < 9; i++) {
		CAnimBlendAssociation *odAssoc = RpAnimBlendClumpGetAssociation(ped->GetClump(), odCrouchPoseIds[i]);
		odW[i] = odAssoc ? odAssoc->blendAmount : 0.0f;
		if (odW[i] > odMaxW)
			odMaxW = odW[i];
	}
	{
		CAnimBlendAssociation *odAssoc = RpAnimBlendClumpGetAssociation(ped->GetClump(), ViceExtCrouchAimPoseClip(ped));
		odPose = odAssoc ? odAssoc->blendAmount : 0.0f;
	}
	ViceExtP4PoseExitTick(ped);
	if (odMaxW <= 0.01f && odPose <= 0.01f)
		return;
	static uint32 s_odNextPose = 0;
	uint32 odNow = CTimer::GetTimeInMilliseconds();
	if (odNow < s_odNextPose && odNow + 60000 >= s_odNextPose)
		return;   // el reloj retrocedió: no bloquear la traza
	s_odNextPose = odNow + 1000;
	uint32 odDesde = (odNow >= s_odCrouchOffTime) ? (odNow - s_odCrouchOffTime) : 0;
	char t[240];
	snprintf(t, sizeof t, "CROUCHPOSE desde=%u idle=%.2f fwd=%.2f back=%.2f izq=%.2f der=%.2f aimfwd=%.2f aimbwd=%.2f aimizq=%.2f aimder=%.2f pose=%.2f",
		odDesde, odW[0], odW[1], odW[2], odW[3], odW[4], odW[5], odW[6], odW[7], odW[8], odPose);
	ODTRACES(t);
#else
	(void)ped;
#endif
}

// R28 (26/09): ver `Ped.h`. El agachado del port no usa `bIsDucking` (R6 lo
// limpia al agacharse, que es la bandera que clava al jugador en el sitio), asi
// que el motor tiene que preguntar por el para elegir sus clips de agachado: la
// pose de apuntar del arma (`colt45_crouchfire` en vez de `colt45_fire`), el
// clip de disparo, la recarga y la vuelta a la pose al acabar el tiro.
// Para los NPCs y para el agachado de disparo de VC siguen valiendo las dos
// banderas del motor.
bool
CPed::ViceExtCrouchShooting(void) const
{
	if (bCrouchWhenShooting && bIsDucking)
		return true;
	// R29 (27/09, costura C18-1/R28 del plan agachado-correcciones-axis): el
	// disparo agachado del port solo usa los clips del motor (`*_crouchfire`/
	// `*_crouchreload`) con armas que traen `WEAPONFLAG_CROUCHFIRE`. Sin el flag
	// manda C18-1 literal: el arma dispara de pie (sin disparo agachado forzado)
	// y la pose de apuntar agachado es la del mod (`WEAPON_crouch`). Para los
	// NPCs y el agachado de disparo de VC siguen valiendo las banderas del motor.
	if (!CWeaponInfo::GetWeaponInfo(((CPed*)this)->GetWeapon()->m_eWeaponType)->IsFlagSet(WEAPONFLAG_CROUCHFIRE))
		return false;
	return IsPlayer() && CPlayerPed::ViceExtIsCrouched();
}

bool
CPlayerPed::ViceExtIsCrouched(void)
{
	return odCrouched;
}

#define VICEEXT_CROUCH_BLEND_MS 125.0f
float
CPlayerPed::ViceExtCrouchBlend(void)
{
	uint32 odNow = CTimer::GetTimeInMilliseconds();
	float odT;
	if (odCrouched) {
		uint32 odElapsed = (odNow >= s_odCrouchOnTime) ? odNow - s_odCrouchOnTime : 0;
		odT = (float)odElapsed / VICEEXT_CROUCH_BLEND_MS;
	} else {
		uint32 odElapsed = (odNow >= s_odCrouchOffTime) ? odNow - s_odCrouchOffTime : 0;
		odT = 1.0f - (float)odElapsed / VICEEXT_CROUCH_BLEND_MS;
	}
	if (odT < 0.0f)
		odT = 0.0f;
	if (odT > 1.0f)
		odT = 1.0f;
	return odT;
}

bool
CPlayerPed::ViceExtCrouchControl(CPad *padUsed)
{
	if (!padUsed || bInVehicle || !IsPedInControl() || DyingOrDead() || CReplay::IsPlayingBack()
		|| ViceExtPedOwns(PEDLANE_NADO, PEDCAP_POSTURA)) {
		s_odCrouchKeyHeld = false;
		// R15: salir de la posesión con el clip puesto (coche, muerte, cinemática)
		// dejaba al ped agachado para siempre, igual que el botón.
		if (odCrouched) {
			odCrouched = false;
			ViceExtCrouchStopAnims(this);
		}
		return false;
	}

	// La tecla es la misma con arma o sin ella (antes, armado, esto sólo SEGUÍA el
	// agachado de disparo del motor, y de ahí que el jugador armado no pudiera
	// moverse agachado).
	bool odDuckDown = ViceExtDuckDown(padUsed);
	bool odEdge = odDuckDown && !s_odCrouchKeyHeld;
#ifdef __EMSCRIPTEN__
	bool odDup = odDuckDown && !odEdge && ViceExtDuckJustDown(padUsed);
#endif
	s_odCrouchKeyHeld = odDuckDown;
	if (odEdge) {
		odCrouched = !odCrouched;
		++s_odCrouchSession;
		if (odCrouched)
			s_odCrouchOnTime = CTimer::GetTimeInMilliseconds();
#ifdef __EMSCRIPTEN__
		if (odCrouched)
			ViceExtP4PoseBeginP4(this, 1);
#endif
		if (!odCrouched)
			ViceExtCrouchStopAnims(this);
	}
#ifdef __EMSCRIPTEN__
	if (odEdge || odDup) {
		char t[130];
		snprintf(t, sizeof t, "VICEEXT crouch %s arma=%d motor=%d repetido=%d",
			odEdge ? (odCrouched ? "on" : "off") : "ignorado",
			(int)GetWeapon()->m_eWeaponType, (int)bIsDucking, (int)odDup);
		ODTRACES(t);
	}
#endif

	// De pie al empezar a correr/esprintar o al saltar.
	if (odCrouched && (padUsed->GetSprint() || padUsed->JumpJustDown())) {
		odCrouched = false;
		ViceExtCrouchStopAnims(this);   // R15
#ifdef __EMSCRIPTEN__
		ODTRACES("VICEEXT crouch off motivo=carrera");
#endif
	}

	// El agachado de disparo del motor clava al jugador en el sitio: si está
	// puesto y el agachado es nuestro, se suelta.
	if (bIsDucking && odCrouched) {
		ClearDuck(true);
		bCrouchWhenShooting = false;
	}

	if (!odCrouched) {
		// R15: comprobar (y dejar traza) de que el cuerpo está de pie de verdad.
		ViceExtCrouchPoseTrace(this);
		return false;
	}

	// R6: el MOVIMIENTO lo pone el motor (aquí no se toca `m_fRotationDest` ni
	// `m_fMoveSpeed` y se devuelve false para que ProcessControl reparta el
	// control a pie normal). Sólo el clip del mod y el tope de velocidad, que se
	// aplica después del control normal (ver `ViceExtCrouchLimitSpeed`).
	//
	// R6b: el clip se aplica DOS veces a propósito — aquí (para que se vea el
	// agachado aunque el estado no llame a `SetRealMoveAnim`) y otra vez desde
	// `SetRealMoveAnim`, que es quien decide la animación de movimiento del
	// motor y por tanto el que la pisaba. `BlendAnimation` con el mismo clip es
	// idempotente (mismo patrón que el nado: se vuelve a pedir cada frame).
	ViceExtCrouchAnim();
#ifdef __EMSCRIPTEN__
	ViceExtP4PoseEnterTick(this);
#endif
	// R6: false = el control a pie normal corre (el motor mueve).
	return false;
}

// R6b: el clip de agachado del mod, elegido por el mando. Idempotente: se puede
// llamar varias veces por frame (devuelve la misma asociación ya mezclada).
void
CPlayerPed::ViceExtCrouchAnim(void)
{
	CPad *padUsed = CPad::GetPad(0);
	float leftRight = padUsed ? padUsed->GetPedWalkLeftRight() : 0.0f;
	float upDown = padUsed ? padUsed->GetPedWalkUpDown() : 0.0f;
	float padMove = CVector2D(leftRight, upDown).Magnitude();

	// R21 (23/09, 17ª partida): LOS LADOS USAN SU CLIP POR DEFECTO. El jugador,
	// tras probar la 16ª partida: "caminando a un lado se desplaza pero la
	// animación... no se gira el personaje al lado donde está caminando". El
	// clip de costado del mod es el de REVOLCARSE de SA (`Crouch_Roll_L/R`, los
	// mismos nombres que lleva el `ViceEx.exe` del mod), así que lo que se ve al
	// andar de lado es la rueda: es lo que hace el mod y lo que pidió el jugador
	// para apuntar ("si apunto y presiono el lado izquierdo o derecho el
	// personaje rueda hacia ese lado"). El desplazamiento de lado lo sigue
	// poniendo el código (R20c), así que la raíz del clip no arrastra al ped.
	// (R24: la palanca `?crouchlado=` se quitó; la pose de costado es la que va.)
	//
	// R20b (22/09, con los clips ya medidos en el juego): agachado se anda COMO
	// DE PIE — el cuerpo gira hacia donde se anda y el clip es el de adelante.
	//	// Por qué no los clips de costado, que era la idea de la primera versión de
	// R20 (elegir clip por el ángulo del mando, como hacen los grupos
	// PLAYERLEFT/RIGHT del motor en el control de ratón): porque en este
	// `ped.ifp` NO son de costado. Medido con `tools/ifp_inspect.py` (metros que
	// avanza la RAÍZ por ciclo, ejes LOCALES del clip: +Y adelante, −X izquierda
	// —el mismo patrón que el `walk_left` de serie, con −1,836 en X y 0,03 en Y):
	//
	//   Crouch_Forward   dx −0,002  dy +2,615   → adelante ✓
	//   Crouch_Backward  dx  0,000  dy −1,853   → atrás ✓
	//   Crouch_Roll_L    dx −0,219  dy −2,172   → HACIA ATRÁS (no de lado)
	//   Crouch_Roll_R    dx −0,244  dy +2,253   → HACIA ADELANTE
	//
	// R27 (26/09): las medidas de arriba son las de los clips VIEJOS (los de VC). El
	// `ped.ifp` servido lleva ya los dos del sa-crouch para adelante/atrás
	// (`GunCrouchFwd`/`GunCrouchBwd`, ±2,740 m / 0,731 s = 3,75 m/s); los dos de
	// costado de la tabla siguen siendo los mismos.
	//
	// Y confirmado en el motor: con `Crouch_Roll_L` puesto, el ped medido se movió
	// 179,6° respecto a su rumbo (o sea hacia atrás) al pulsar izquierda. Son los
	// clips de REVOLCARSE/rodar de SA, no un paso lateral. O sea: con los clips
	// que el mod trae, el cuerpo TIENE que girar para andar de lado, o se anda
	// hacia donde no se pide. Y es lo que hace el mod: en su vídeo (`pc.mp4`,
	// t≈64-67 s) se le ve la espalda mientras anda agachado, el cuerpo alineado
	// con el avance. El giro lo da `ViceExtCrouchFaceMovement` (abajo).
	//
	// Lo único que el clip de atrás aporta —y es correcto— es con la MIRA puesta,
	// que es cuando el motor clava el rumbo del ped a la cámara
	// (`VICEEXT_AIM_WALK`) y el mando se lee en el sistema del cuerpo: retroceder
	// apuntando.
	float odAng = ViceExtCrouchPadAngle(padUsed);
	bool odWalking = padMove > 16.0f;   // spec sa-crouch deadzone +-16 (antes 60*0.25=15)

	// R21: ¿se está apuntando? (se decide AQUÍ, antes del clip: la rueda sólo
	// sale apuntando). La pose de apuntar agachado es `WEAPON_crouch`, y es del
	// mod: su `ped.ifp` no trae NINGÚN otro clip de agachado con arma (medido:
	// `WEAPON_crouch` es el único; `WEAPON_crouchfire`/`WEAPON_crouchreload`, que
	// el motor pide para armas con `WEAPONFLAG_CROUCHFIRE`, NO están en el ifp).
	// O sea: la animación de apuntar agachado que pide el mod es ésa, no hay otra.
	// R29 (G): el "estoy apuntando" se lee de la API unica (la ley del port del
	// ClassicAXIS manda desde C17/C18); antes cada consumidor lo leia a su
	// manera y por eso rueda, pose y giro discordaban.
	bool odAim = ViceExtIsAiming();
	eWeaponType odAimWep = GetWeapon()->m_eWeaponType;

	// R22 (18ª partida, 23/09): LA RUEDA ES **UNA**. El jugador: "me muero si
	// camino hacia a un lado". Con la rueda puesta por defecto para los lados
	// (R21) y el clip en `ASSOC_REPEAT`, mantener la tecla dejaba a Tommy
	// revolcándose POR EL SUELO sin parar (en el vídeo del mod, `pc.mp4`
	// t≈60-62 s, las ruedas se ven sueltas y encadenadas a golpe de tecla, no
	// girando en bucle). Ahora: una rueda por petición, y para repetir hay que
	// soltar el lado (es la esquiva de SA). Mientras rueda, el desplazamiento lo
	// pone R20c a la velocidad de la rueda.
	uint32 odNowMs = CTimer::GetTimeInMilliseconds();
	bool odLateralPuro = Abs(upDown) <= 16.0f && Abs(leftRight) > 16.0f;
	bool odNeutral = Abs(leftRight) <= 16.0f && Abs(upDown) <= 16.0f;
	bool odLadoIzq = odLateralPuro && leftRight < 0.0f;
	bool odLadoDer = odLateralPuro && leftRight > 0.0f;
	bool odFlancoIzq = odLadoIzq && !s_odRollPrevIzq;
	bool odFlancoDer = odLadoDer && !s_odRollPrevDer;
	s_odRollPrevIzq = odLadoIzq;
	s_odRollPrevDer = odLadoDer;
	s_odRollNeutral = odNeutral;
	if (odNeutral)
		s_odRollArmed = true;
	if (s_odRollState == ODROLL_ACTIVE && CTimer::GetFrameCounter() != s_odRollFrame) {
		s_odRollFrame = CTimer::GetFrameCounter();
		CAnimBlendAssociation *odR = RpAnimBlendClumpGetAssociation(GetClump(), s_odRollClip);
		if (odR == nil || odR->hierarchy == nil)
			ViceExtCrouchRollFinish(nil, false, "borrada");
		else if (odR->currentTime >= odR->hierarchy->totalLength)
			ViceExtCrouchRollFinish(odR, true, "fin");
		else {
			if (odR->currentTime > s_odRollLastCurrent)
				s_odRollRate += odR->currentTime - s_odRollLastCurrent;
			s_odRollLastCurrent = odR->currentTime;
			if (odR->blendAmount > s_odRollMaxW)
				s_odRollMaxW = odR->blendAmount;
			CVector odStepP = GetPosition();
			float odStepD = CVector2D(odStepP.x - s_odRollStepPos.x, odStepP.y - s_odRollStepPos.y).Magnitude();
			s_odRollStepM += odStepD;
			s_odRollStepT += CTimer::GetTimeStep() / 50.0f;
			s_odRollStepPos = odStepP;
			++s_odRollStepN;
			{
				char t[420];
				const char *odStepNom = (odR->hierarchy && odR->hierarchy->name) ? odR->hierarchy->name : "?";
				float odStepMps = (s_odRollStepT > 0.0001f) ? (s_odRollStepM / s_odRollStepT) : 0.0f;
				VICEEXT_P4_EMIT(s_odMove.gen, t, "P4 kind=roll_step schema=1 gen=%u frame=%u sim=%.4f case=5 id=%u clip=%s current=%.4f step=%.4f total=%.4f t=%.4f mps=%.4f speedTarget=%.4f mvec=%.4f mfspd=%.4f n=%u",
					(unsigned)s_odMove.gen, (unsigned)CTimer::GetFrameCounter(), CTimer::GetTimeInMilliseconds() * 0.001f,
					(unsigned)s_odRollId, odStepNom, odR->currentTime, odStepD, s_odRollStepM, s_odRollStepT,
					odStepMps, s_odRollSpeedMps, m_vecMoveSpeed.Magnitude(), m_fMoveSpeed, s_odRollStepN);
			}
		}
	}
	if (s_odRollState == ODROLL_ACTIVE && (!odAim || !odCrouched || !IsPedInControl() || bInVehicle || DyingOrDead()
	    || CReplay::IsPlayingBack() || ViceExtPedOwns(PEDLANE_NADO, PEDCAP_POSTURA)))
		ViceExtCrouchRollFinish(nil, false, "abandono");
	AnimationId odCrouchAnim = ANIM_STD_CROUCH_IDLE;
	bool odRollNow = false;   // R27: ¿ha salido rueda en ESTE frame?
	if (s_odRollState == ODROLL_ACTIVE) {
		odCrouchAnim = (s_odRollSide < 0) ? ANIM_STD_CROUCH_LEFT : ANIM_STD_CROUCH_RIGHT;
	} else if (odWalking) {
		int odWepId = (int)GetWeapon()->m_eWeaponType;
		bool odWepOk = odWepId >= 17 && !(odWepId >= 28 && odWepId <= 33);   // spec sa-crouch filtro armas >=17 sin 28-33
		// R27: la rueda es SIN DISPARAR, como el mod (`NOT IS_BUTTON_PRESSED 17`:
		// el Circle es `CPad::GetWeapon`). Apuntando y disparando a la vez, el mod
		// dispara: la rueda no sale.
		if ((odFlancoIzq || odFlancoDer) && odAim && odWepOk && !padUsed->GetWeapon() && s_odRollArmed && s_odRollFrame != CTimer::GetFrameCounter()) {
			odRollNow = true;
			odCrouchAnim = odFlancoIzq ? ANIM_STD_CROUCH_LEFT : ANIM_STD_CROUCH_RIGHT;
			ViceExtCrouchRollStart(this, odFlancoIzq, odCrouchAnim);
#ifdef __EMSCRIPTEN__
			{
				char t[130];
				snprintf(t, sizeof t, "VICEEXT crouch roll lado=%s clip=%d veloc=%.2f dur=0 motivo=ok",
					(odFlancoIzq) ? "izq" : "der", (int)odCrouchAnim,
					ViceExtCrouchTargetSpeed(odCrouchAnim));
				ODTRACES(t);
			}
#endif
		} else if (odAim) {
			// R29 (E3.9): APUNTANDO + mando = "moverse apuntando" (el movimiento
			// que pidio el jugador: "apuntar y desplazarse"): los cuatro
			// `GunMove_*` del mod por angulo (cortes del motor: +-50
			// adelante/costado, +-130 costado/atras), con el encaramiento que
			// manda la camara (axis) y la velocidad del aim-walk de pie (tope
			// R9 = paso de andar). Hacia atras el cuerpo no gira (D3=(a)).
			float odAbsA = Abs(odAng);
			if (odAbsA > DEGTORAD(VICEEXT_CROUCH_BACK_DEG))
				odCrouchAnim = ANIM_STD_CROUCH_AIMBWD;
			else if (odAbsA < DEGTORAD(50.0f))
				odCrouchAnim = ANIM_STD_CROUCH_AIMFWD;
			else
				odCrouchAnim = (odAng > 0.0f) ? ANIM_STD_CROUCH_AIMLEFT : ANIM_STD_CROUCH_AIMRIGHT;
		} else {
			// R22: SIN apuntar, de costado se anda con el clip de ADELANTE y el
			// cuerpo gira hacia donde se anda (lo hace R20c). Es el modelo de SA
			// (el ped se gira al andar) y evita que andar de lado sea una rueda:
			// el mod sólo trae clips de rueda para el costado, así que la
			// alternativa a girar el cuerpo es revolcarse.
			odCrouchAnim = ANIM_STD_CROUCH_FORWARD;
		}
	}
#ifdef __EMSCRIPTEN__
	// R27: POR QUÉ NO SALE LA RUEDA. Cuatro puertas (mira, disparo, bloqueo y
	// rueda en curso) y hasta ahora el log no decía cuál la paraba: los logs
	// del jugador del 26/09 traían 0 ruedas y 0 muestras con `mira=1`, así que
	// el fallo ni se podía localizar. Una línea por CAMBIO de motivo, y sólo con
	// el lado pulsado (si no, cambiaría cada frame al andar). El `motivo=ok` va
	// en el evento de la rueda (arriba); el árbol de diagnóstico está en el
	// plan agachado-calibrado, E5.
	{
		int odWhy = 0;                            // 0 = nada que contar
		if (odWalking && odLateralPuro && !odRollNow) {
			if (s_odRollState == ODROLL_ACTIVE) odWhy = 1;
			else if (!odAim) odWhy = 2;           // sin-mira: `tgt`/`cana` de CROUCH2 dicen cuál
			else if (padUsed->GetWeapon()) odWhy = 3;
		}
		static int s_odWhyLast = 0;
		if (odWhy != s_odWhyLast) {
			s_odWhyLast = odWhy;
			if (odWhy != 0) {
				static const char *const odWhyNom[] = { "-", "activa", "sin-mira", "disparo" };
				char t[150];
				snprintf(t, sizeof t, "VICEEXT crouch roll skip motivo=%s arma=%d ang=%.0f",
					odWhyNom[odWhy], (int)GetWeapon()->m_eWeaponType, RADTODEG(odAng));
				ODTRACES(t);
			}
		}
	}
#endif
	// spec sa-crouch entrada sin parpadeo: SET 0 un frame -> PLAY blend 10 -> WAIT 500 -> SET 1 (+ blend 30 parado).
	// Aqui: ClearDuck ya hizo SET 0 (Control), blend 10 los primeros 500ms tras cambio de clip, luego 4.0f. Idempotente (R6b).
	static uint32 s_odCrouchEnterMs = 0;
	static AnimationId s_odCrouchLastAnim = ANIM_STD_CROUCH_IDLE;
	float odBlend = 4.0f;
	if (odCrouchAnim != s_odCrouchLastAnim) { s_odCrouchEnterMs = odNowMs; s_odCrouchLastAnim = odCrouchAnim; }
	if (odNowMs - s_odCrouchEnterMs < 500) odBlend = 10.0f;
	CAnimBlendAssociation *odAssoc = CAnimManager::BlendAnimation(GetClump(), ASSOCGRP_PLAYERCROUCH, odCrouchAnim, odBlend);
	// R21b: el clip elegido se recuerda para que `CPed::CalculateNewVelocity`
	// (R20c) pueda pedir su velocidad objetivo cuando el motor no le dé ninguna
	// (apuntando: la pose parcial a peso 1 anula la traslación del clip).
	s_odCrouchClip = odCrouchAnim;

	// R22: RITMO del clip = velocidad objetivo / velocidad de su raíz, para que
	// los pies avancen lo mismo que el ped (ver `ViceExtCrouchRateFor`). Para un
	// clip con `ASSOC_MOVEMENT`, `speed` ES el ritmo
	// (`AnimBlendAssociation::UpdateTimeStep`: `timeStep = speed*dt`), así que
	// escala avance y cadencia a la vez.
	if (odAssoc && odCrouchAnim != ANIM_STD_CROUCH_IDLE)
		odAssoc->speed = ViceExtCrouchRateFor(odCrouchAnim);
#ifdef __EMSCRIPTEN__
	{
		static int s_odCrouchSpdSig = -1;
		int odSpdSig = (int)odCrouchAnim;
		if (odSpdSig != s_odCrouchSpdSig) {
			s_odCrouchSpdSig = odSpdSig;
			float odTargetMps = ViceExtCrouchTargetSpeed(odCrouchAnim);
			float odRate = (odAssoc != nil) ? odAssoc->speed : 0.0f;
			char t[240];
			const char *odClipNombre = ViceExtClipName(ASSOCGRP_STD, odCrouchAnim);
			VICEEXT_P4_EMIT(s_odMove.gen, t, "P4 kind=crouchspeed schema=1 gen=%u frame=%u sim=%.4f case=4 id=0 mps=%.4f rate=%.4f mfs=%.4f target=%.4f clip=%s",
				(unsigned)s_odMove.gen, (unsigned)CTimer::GetFrameCounter(),
				CTimer::GetTimeInMilliseconds() * 0.001f,
				odTargetMps, odRate, m_fMoveSpeed, odTargetMps, odClipNombre);
		}
	}
#endif
	// R29 (E3.7): el peso REAL del clip de rueda mientras dura (tiene que entrar
	// alto, > 0,5, con `RollClearPartials` limpiando las parciales): sale por
	// cada `roll fin peso=`.
	if (odRollNow && odAssoc && odAssoc->hierarchy) {
		s_odRollLength = odAssoc->hierarchy->totalLength;
		s_odRollSpeed = 1.0f;
		odAssoc->speed = 1.0f;
		odAssoc->flags &= ~ASSOC_FADEOUTWHENDONE;
		ViceExtCrouchRollStarted(odAssoc);
	} else if (s_odRollState == ODROLL_ACTIVE && odAssoc && odAssoc->blendAmount > s_odRollMaxW) {
		s_odRollMaxW = odAssoc->blendAmount;
	} else if (odAssoc != nil && s_odRollState != ODROLL_ACTIVE
		&& (odAssoc->animId == ANIM_STD_CROUCH_LEFT || odAssoc->animId == ANIM_STD_CROUCH_RIGHT)
		&& !(odAssoc->flags & ASSOC_FADEOUTWHENDONE)) {
		odAssoc->flags |= ASSOC_FADEOUTWHENDONE;
	}

	// R28: la pose cede ante la del arma y ante la rueda (ver `ViceExtCrouchAimPose`).
	if (odAim)
		ViceExtCrouchAimPose(this, odWalking, odRollNow || s_odRollState == ODROLL_ACTIVE);
	else {
		s_odCrouchPoseOthers = 0.0f;
		s_odCrouchPoseWho = "-";
		s_odCrouchWeaponW = 0.0f;
		s_odCrouchWeaponNom = "-";
		ViceExtCrouchAimPoseOff(this);
	}
#ifdef __EMSCRIPTEN__
	// R25 (23/09, 19ª partida): MEDIDOR DEL TRAMO AGACHADO.
	//
	// El jugador: "no estás modificando el valor de la velocidad del personaje;
	// mide con logs la velocidad, cuántos metros se ha desplazado desde que tiene
	// la W pulsada; estando agachado voy más rápido que cualquier auto". Aquí está
	// eso exactamente, y medido de la única forma que no engaña:
	//
	//   · la distancia es la SUMA de los desplazamientos de cada frame (no la
	//     diferencia entre dos muestras separadas un segundo, que es lo que
	//     convertía un tirón del navegador o un respawn en un "20 m/s" falso),
	//   · el tiempo es la suma del tiempo de MUNDO de esos mismos frames,
	//   · `maxfr` es el mayor desplazamiento de UN frame: andando bien vale
	//     ~0,02 m (0,9 m/s ÷ 50 fps); un catapultazo o un teletransporte sale con
	//     su tamaño a la vista,
	//   · y va al lado de lo que el juego CREE que hace: `mvec` (m_vecMoveSpeed),
	//     `fspd` (m_fMoveSpeed) y `anim` (m_vecAnimMoveDelta, la traslación de la
	//     RAÍZ del clip: si eso manda sin pulsar nada, se ve aquí).
	//
	// Se publica al SOLTAR la tecla: una línea `CROUCHMOVE fin` con el tramo
	// entero desde que la pulsó. Deduplicado por contador de frame porque esta
	// función se llama dos veces por frame a propósito.
	static CVector s_odTripPos(0.0f, 0.0f, 0.0f);
	static bool s_odTripOn = false;
	static float s_odTripT = 0.0f, s_odTripM = 0.0f, s_odTripMax = 0.0f;
	static uint32 s_odTripFrames = 0, s_odTripLastFrame = 0;
	static uint32 s_odTripSes = 0;
	uint32 odTripFrame = CTimer::GetFrameCounter();
	if (odTripFrame != s_odTripLastFrame) {
		s_odTripLastFrame = odTripFrame;
		// Al empezar/acabar una sesión de agachado el tramo se empieza de cero: si
		// no, la primera muestra vendría de la posición de la sesión anterior y
		// `m`/`maxfr` saldrían con el salto de por medio (la misma trampa que el
		// `avance` de R22).
		if (s_odTripSes != s_odCrouchSession) {
			s_odTripSes = s_odCrouchSession;
			s_odTripOn = false;
		}
		CVector odTripP = GetPosition();
		if (odWalking) {
			if (!s_odTripOn) {
				s_odTripOn = true;
				s_odTripT = 0.0f; s_odTripM = 0.0f; s_odTripMax = 0.0f; s_odTripFrames = 0;
			} else {
				float odStep = CVector2D(odTripP.x - s_odTripPos.x, odTripP.y - s_odTripPos.y).Magnitude();
				s_odTripM += odStep;
				if (odStep > s_odTripMax) s_odTripMax = odStep;
			}
			s_odTripT += CTimer::GetTimeStep() / 50.0f;
			++s_odTripFrames;
			s_odTripPos = odTripP;
		} else if (s_odTripOn) {
			s_odTripOn = false;
			char t[230];
			float odAvg = (s_odTripT > 0.001f) ? s_odTripM / s_odTripT : 0.0f;				snprintf(t, sizeof t,
					"CROUCHMOVE fin mov=%u t=%.2f m=%.2f mps=%.2f maxfr=%.3f obj=%.2f obju=%.3f mvec=%.2f fspd=%.2f anim=%.2f pad=%.0f clip=%s piesMps=%.2f",
					s_odTripFrames, s_odTripT, s_odTripM, odAvg, s_odTripMax,
					ViceExtCrouchMoveSpeed(),
					ViceExtCrouchMoveSpeed() * METERS_PER_SECOND_TO_GAME_SPEED,   // R25: m/s -> unidad del motor
					m_vecMoveSpeed.Magnitude(), m_fMoveSpeed,
				m_vecAnimMoveDelta.Magnitude(), padMove,					(odAssoc && odAssoc->hierarchy) ? odAssoc->hierarchy->name : "?",
					odAssoc ? odAssoc->speed * ViceExtCrouchClipSpeed(s_odCrouchClip) : 0.0f);
			ODTRACES(t);
		}
	}
#endif
	// R17: la traza del avance real (m/s) la publica el vigilante de abajo.
#ifdef __EMSCRIPTEN__
	// Una línea por segundo: cierra C5 desde el log ("¿se mueve agachado?").
	// R6: además CROUCH2 con altura del ped y rumbo pedido vs. real.
	// R6b: y `peso`, el peso real del clip de agachado en la mezcla: si baja de
	// ~0,5 es que otro clip está compitiendo por el cuerpo (sería el fallo de
	// que "se agacha a medias" aunque la traza diga que el clip es el correcto).
	{
			static uint32 s_odNextTrace = 0;
			uint32 odNow = CTimer::GetTimeInMilliseconds();
			// R18: `avance` es metros por segundo de RELOJ, y el reloj corre MÁS que
			// el mundo cuando la simulación va limitada por el timestep máximo (en
			// el navegador, con el agua en pantalla, la sonda midió 0,95 m/s de un
			// nado que va a 2,3). Se acumula el tiempo de MUNDO (suma de
			// `GetTimeStep()`, en unidades de 1/50 s) para publicar `velo` = m/s de
			// verdad.
			// R19: esta función se llama DOS veces por frame (a propósito, ver
			// arriba: el clip se pide también desde `SetRealMoveAnim`), así que el
			// reloj de mundo se sumaba dos veces y `velo` salía a la mitad (medido:
			// dt=2,04 s por segundo de reloj). Se filtra por contador de frame.
			static float s_odWorld = 0.0f, s_odWorldLast = 0.0f;
			static uint32 s_odLastFrame = 0;
			// R24: frames de esta muestra. Un tirón (stall del navegador) hacía que
			// `avance`/`velo` salieran a 20-58 m/s sin serlo; con `nf` y `hit` la
			// muestra se puede descartar al leer el log en vez de creerse un
			// disparate (medido en la 19ª partida: `hit=1` justo en los valores
			// altos, con `FPSLOG maxdelta=4336ms`).
			static uint32 s_odFrames = 0;
			if (CTimer::GetFrameCounter() != s_odLastFrame) {
				s_odWorld += CTimer::GetTimeStep() / 50.0f;
				s_odLastFrame = CTimer::GetFrameCounter();
				++s_odFrames;
			}
			if (odNow < s_odNextTrace && odNow + 60000 >= s_odNextTrace) {}
			else {
				s_odNextTrace = odNow + 1000;
				uint32 odNf = s_odFrames;
				s_odFrames = 0;
				static CVector s_odLastPos(0.0f, 0.0f, 0.0f);
				static bool s_odLastPosOk = false;
				// R22: primera muestra de cada sesión de agachado = sin referencia
				// (la de la sesión anterior era de otra parte del mapa y el `avance`
				// salía absurdo: 84,70 m en la partida del jugador).
				static uint32 s_odSeenSession = 0;
				if (s_odSeenSession != s_odCrouchSession) {
					s_odSeenSession = s_odCrouchSession;
					s_odLastPosOk = false;
				}
				// R17: `avance` = metros RECORRIDOS de verdad desde la traza
				// anterior. Es la prueba de que el ped anda agachado: `spd` sólo
				// dice lo que pide el mando y el clip, no lo que el motor mueve.
				float odAvance = 0.0f, odVelo = 0.0f, odDtWorld = s_odWorld - s_odWorldLast;
				s_odWorldLast = s_odWorld;
				CVector odPos = GetPosition();
				if (s_odLastPosOk) {
					CVector odDelta = odPos - s_odLastPos;
					odAvance = CVector2D(odDelta.x, odDelta.y).Magnitude();
					// R18: m/s de mundo (metros entre el tiempo de simulación).
					if (odDtWorld > 0.001f)
						odVelo = odAvance / odDtWorld;
				}
				s_odLastPos = odPos;
				s_odLastPosOk = true;
				float odPeso = ViceExtBlendWeight(GetClump(), odCrouchAnim);
				CVector odCamPos = TheCamera.GetPosition();
				// R20: el NOMBRE del clip, no sólo su número: el arnés comprueba la
				// dirección con "¿qué clip pidió el motor?" y con el número tendría
				// que copiar a mano los valores del enum (que además se movieron al
				// reordenar el bloque de agachado).
				const char *odClipNom = (odAssoc && odAssoc->hierarchy) ? odAssoc->hierarchy->name : "?";
				// R25: era de 300 y la línea YA se cortaba en el log del jugador
				// (salía "rued" en vez de "rueda=").
				// R28: 720 por los cuatro campos nuevos del presupuesto de parciales
				// (`otros`, `pose`, `pesoarma` y `nomarma`), que son la prueba del
				// disparo agachado y de la rueda.
				char t[768];
				snprintf(t, sizeof t, "VICEEXT crouch move spd=%.1f andando=%.1f clip=%d nom=%s arma=%d mio=0 estado=%d medida=1x cal=1 clipr=%.2f",
					m_fMoveSpeed, padMove, (int)odCrouchAnim, odClipNom, (int)GetWeapon()->m_eWeaponType,
					(int)m_nPedState, ViceExtCrouchClipSpeed(odCrouchAnim));
				ODTRACES(t);
				// R21: `mira` = se está apuntando con el agachado puesto y `pesomira` =
				// peso real de la pose de apuntar del mod (`WEAPON_crouch`): es la
				// prueba de que "al apuntar agachado sale su animación".
				// `tgt` y `cana` van aparte a propósito: si `mira` sale 0 hay que poder
				// distinguir "el mando no ve el apuntado" (`tgt=0`: tecla/ratón) de
				// "el arma en mano no es de apuntar" (`cana=0`: puños o melee).
				CAnimBlendAssociation *odPoseA = RpAnimBlendClumpGetAssociation(GetClump(), ViceExtCrouchAimPoseClip(this));
				float odPesoMira = odPoseA ? odPoseA->blendAmount : 0.0f;
				int odTgt = (padUsed && padUsed->GetTarget()) ? 1 : 0;
				int odCana = ViceExtCanAim(odAimWep, CWeaponInfo::GetWeaponInfo(odAimWep)) ? 1 : 0;
				snprintf(t, sizeof t, "CROUCH2 h=%.2f avance=%.2f velo=%.2f moved=%.2f mvec=%.2f anim=%.2f fspd=%.2f dt=%.2f nf=%u hit=%d rotDest=%.2f rumbo=%.2f clip=%d nom=%s peso=%.2f camz=%.2f camdist=%.2f modo=%d baja=%.2f ang=%.1f mps=%.2f obju=%.3f pies=%.2f strafe=%d mira=%d gat=%d pesomira=%.2f tgt=%d cana=%d arma=%d veloc=%.2f rueda=%d giro=%d hdgr=%.0f cal=1 clipr=%.2f entrada=%.0f otros=%.2f pose=%s pesoarma=%.2f nomarma=%s piesMps=%.2f",
					odPos.z, odAvance, odVelo, m_moved.Magnitude(), m_vecMoveSpeed.Magnitude(),
					m_vecAnimMoveDelta.Magnitude(), m_fMoveSpeed, odDtWorld,
					odNf, (odNf < 40) ? 1 : 0,
					m_fRotationDest, m_fRotationCur, (int)odCrouchAnim, odClipNom, odPeso,
					odCamPos.z, CVector2D(odCamPos.x - odPos.x, odCamPos.y - odPos.y).Magnitude(),
					(int)TheCamera.Cams[TheCamera.ActiveCam].Mode, VICEEXT_CROUCH_CAM_DROP,
					RADTODEG(ViceExtCrouchPadAngle(padUsed)), ViceExtCrouchWalkMps(),
					// R25: `obju` = la velocidad objetivo en UNIDADES DEL MOTOR (m/s ÷ 50):
					// tiene que coincidir con `mvec`, mientras `mps` (m/s) coincide con
					// `velo`. Con eso la unidad deja de ser un misterio en el log.
					ViceExtCrouchWalkMps() * METERS_PER_SECOND_TO_GAME_SPEED,
					// R23: `pies` = el ritmo que lleva el clip AHORA (el que se le pone a
					// `speed`), y por tanto los m/s a los que barren los pies. Si no
					// coincide con `mps`, los pies no van a la par del ped: es la
					// medida exacta de la queja ("los pies no van a la par de la
					// velocidad real de Tommy").
					// R27: `cal=1` marca la build calibrada (sin ella el verificador no da
					// PASS) y `clipr` es la RAÍZ declarada del clip en uso, la que convierte
					// `mps` en `pies` (caminando: pies = mps/clipr, sin multiplicador desde R29). `entrada` son los ms
					// desde el cambio de clip (ventana del blend 10 de la spec sa-crouch).
					(odAssoc ? odAssoc->speed : -1.0f),
					(TheCamera.Cams[0].Using3rdPersonMouseCam() && CanStrafeOrMouseControl()) ? 1 : 0,
					odAim ? 1 : 0, (padUsed && padUsed->GetWeapon()) ? 1 : 0, odPesoMira, odTgt, odCana, (int)odAimWep,
					ViceExtCrouchTargetSpeed(odCrouchAnim), ((s_odRollState == ODROLL_ACTIVE) ? 1 : 0),
					odSideWhy, m_headingRate,
					ViceExtCrouchClipSpeed(odCrouchAnim), (float)(odNowMs - s_odCrouchEnterMs),
					// R28: `otros` = presupuesto de parciales gastado por otros clips
					// (`otros` mas `peso` nunca puede pasar de 1: por encima el motor
					// extrapola y el cuerpo sale deformado), `pose` = clip que lleva la
					// pose, y `pesoarma`/`nomarma` = la parcial del arma mas pesada y su
					// NOMBRE (`*_crouchfire` si el disparo agachado esta bien).
					s_odCrouchPoseOthers, s_odCrouchPoseWho, s_odCrouchWeaponW, s_odCrouchWeaponNom,
					(odAssoc ? odAssoc->speed * ViceExtCrouchClipSpeed(odCrouchAnim) : 0.0f));
				ODTRACES(t);
			}
		}
#endif
}

// R22: velocidad objetivo (m/s) del clip de agachado en uso — la del clip si
// el motor no le da ninguna (apuntando: la pose parcial se come su traslación).
// La usa el desplazamiento por código de R20c.
float
CPlayerPed::ViceExtCrouchMoveSpeed(void)
{
	return ViceExtCrouchTargetSpeed(s_odCrouchClip);
}

// R6: tope de velocidad agachado, aplicado DESPUÉS del control a pie normal
// (que ya corrió y puso `m_fMoveSpeed`). Sin esto el motor correría de pie.
void
CPlayerPed::ViceExtCrouchLimitSpeed(void)
{
	if (!odCrouched)
		return;
	if (m_fMoveSpeed > VICEEXT_CROUCH_SPEED)
		m_fMoveSpeed = VICEEXT_CROUCH_SPEED;
}

#ifdef __EMSCRIPTEN__
// Arnés web (ve37): la ficha `WINFO` de un arma CONCRETA, aunque el jugador no
// la tenga en la mano. La publica `ProcessPlayerWeapon` cuando el arma cambia,
// pero el truco de armas del mod reparte 8 armas de golpe y la sonda web no
// consigue cambiarlas (el ciclo con el teclado numérico no responde ahí): con
// esto la prueba de armas mide los clips de las OCHO sin depender del teclado.
//
// `WLOAD arma=%s cargado=%d txd=%s txdCargado=%d` la acompaña (Pad.cpp, la
// medida del streaming: el modelo y el diccionario del mod en memoria).
void
CPlayerPed::ViceExtWeaponInfoOf(eWeaponType weaponType)
{
	ViceExtWeaponInfoTrace(weaponType, CWeaponInfo::GetWeaponInfo(weaponType));
}

// R19: "¿dónde estoy y a qué velocidad avanza DE VERDAD?", una línea por
// segundo y en cualquier estado (de pie, agachado, nadando, en coche).
//
// Hace falta para dos cosas:
//  1. `av`/`velo` = metros y m/s medidos con la POSICIÓN, no con lo que pide el
//     mando. Es la referencia de la marcha de serie (la del motor, ~1,6 m/s)
//     contra la que comparar agachado y nado, y sirve para cualquier estado.
//  2. la sonda de agua necesita `x`/`y` para NAVEGAR hasta el mar: antes andaba
//     a ciegas 4×30 s y a veces se quedaba contra una pared sin llegar nunca
//     (pasó el 22/09: la parte de nado quedó sin medir).
// `ms` es el reloj del motor, `nf` los frames y `ts` los segundos de MUNDO
// (suma de `GetTimeStep()`, filtrada por frame): si `ts` ≈ `ms`/1000 la
// simulación va a tiempo real y `velo` es comparable entre estados.
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
	// R19c: ¿hacia dónde está el MAR y a qué distancia? Se sondea con el propio
	// motor (`GetWaterLevelNoWaves` devuelve falso cuando en ese punto hay
	// tierra) en 16 rumbos × 4 pasos de 12 m. Es la señal con la que la sonda
	// navega: en Ocean Beach el suelo es LLANO y el `z` no baja (medido el
	// 22/09: `dz=0,00` en 13 rumbos seguidos, la sonda daba vueltas a la
	// manzana), mientras que la distancia al agua baja de forma monótona al
	// acercarse a la orilla.
	float odMarDeg = -1.0f, odMarDist = -1.0f;
	for (int i = 0; i < 16; i++) {
		float odA = i * (TWOPI / 16.0f);
		for (int s = 1; s <= 4; s++) {
			float odD = s * 12.0f;
			float odWl;
			// R19d: los puntos son ARBITRARIOS (hasta 48 m del ped), así que
			// pueden caer fuera de la tabla de agua del motor: el índice de
			// `aWaterFineBlockList` se calcularía fuera de rango y el motor
			// leería (y usaría como índice) memoria que no es suya. Se comprueba
			// el índice ANTES de preguntar; fuera de la tabla = sin dato de agua.
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
	// R19e: `camz` también de PIE. El bloque H comparaba la `camz` mediana de
	// agachado con la de pie de TODA la sesión, que son sitios distintos: en la
	// 15ª partida cantó "la cámara no baja agachado (11,18 vs 11,33)" con el
	// jugador a 12 m y agachado en otro sitio. Con la cámara de pie en la traza,
	// la comparación es CÁMARA SOBRE EL PED (camz−z), que no depende del sitio.
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
#endif // __EMSCRIPTEN__
#endif // VICEEXT_CROUCH

// =====================================================================
// ClassicAXIS · el "ProcessPlayerPedControl" del mod
// =====================================================================
// PORTADO — ClassicAXIS (sin LICENSE, gennariarmando/DK22Pac) — Main.cpp:1203
//   «static void ProcessPlayerPedControl(CPlayerPed* playa)»
// Qué se toma: TODO el bloque de control del jugador del mod: los predicados
//   `IsAbleToAim` / `IsType1stPerson` / `IsWeaponPossiblyCompatible` / `IsTypeMelee`
//   / `IsTypeTwoHanded` (:605-743), la toma y devolución del modo de cámara
//   (:1237-1244 y :1417-1425), la rotación y los TRES `PedIK.MoveLimb` (:1288-1312),
//   y la regla de suelta del fijado (:1252-1286).
// Adaptación: los identificadores van a nombres del motor (`m_pPed` → `this`,
//   `m_pPointGunAt` → `m_pPointGunAt`, `m_nCamMode` → `Cams[ActiveCam].Mode`,
//   `m_fFPSMoveHeading` igual, `m_PedIK` → `m_pedIK`, `m_sHead` → `m_headOrient`…),
//   y los flags de `CWeaponInfo` son máscara (`IsFlagSet`). D3 (decisión del
//   jugador el 27/09): **NO se muta la tabla global** de `weapon.dat` — el mod lo
//   hace para flamethrower/minigun (`Main.cpp:663-673`) y eso, aquí, apagaría el
//   apuntado de esos armas para la IA y para el resto del frame. La misma
//   intención se expresa por consulta (helper de abajo).
//   D2: el lanzacohetes NO está en la lista de 1.ª persona aunque el mod lo tenga,
//   porque `VICEEXT_ROCKET_3RD_PERSON` (R16) lo saca del francotirador a propósito.
//   §5.2(a): con ratón NO hay auto-aim (el mod lo tiene apagado por defecto).
// Medible: §8.7, §8.9, §8.11 — `AIMLAW`, `AIMLOCK`, `AIMIK`, `AIMWPN`.

// El "hay mando" del mod es `pXboxPad->HasPadInHands()`, que viene del acoplo GInput
// (un DLL de Windows: no existe en web, y §6 lo prohíbe).
// ERROR QUE ESTA PRIMERA VERSION TENIA (medido el 28/09): decía "hay mando si el
// stick DERECHO devuelve CUALQUIER cosa", y con eso salía `aut=1` (§5.2a mal
// aplicado) solo de jugar con ratón, porque el stick devuelve ruido. Ahora son DOS
// condiciones y las dos tienen que cumplirse:
//   (a) el método de control elegido ES paddle (`CURMODE` == 3, el único paddle
//       real de reVC; los demás modos son teclado), y
//   (b) el jugador ha MOVIDO de verdad el stick derecho, por encima de la zona
//       muerta, para no contar el ruido del centro. El umbral es el mismo que
//       aplican `GetLookAroundLeftRight/UpDown` (Pad.cpp:3524-3557): 40.
static bool ViceExtHasPadInHands(void)
{
	CPad *pad = CPad::GetPad(0);
#ifdef DETECT_PAD_INPUT_SWITCH
	// OJO: `#define DETECT_PAD_INPUT_SWITCH` (config.h:369) NO tiene valor, solo un
	// comentario, asi que va con `#ifdef` y NO con `#if` (con `#if` el preprocesador
	// se queda sin expresion y el fichero no compila).
	// `CURMODE` es un macro LOCAL de `Pad.cpp:2327`, asi que aqui no existe: se
	// usan los dos miembros que lo componen, que si son publicos (`Pad.h:155,169`).
	// El 3 es el unico modo de control que es paddle real; 0/1/2 son teclado.
	if (!(CPad::IsAffectedByController && pad->GetMode() == 3))
		return false;
#endif
	return Abs((int)pad->GetRightStickX()) > 40 || Abs((int)pad->GetRightStickY()) > 40;
}

// Main.cpp:1250/330/475: `disableAutoAim = !HasPadInHands && !forceAutoAim`.
// §5.2(a) el jugador eligió la opción FIEL: con ratón no hay auto-aim.
static bool ViceExtIsAutoAimDisabled(void)
{
	return !ViceExtHasPadInHands() && !CCamera::s_viceExtAim.forceAutoAim;
}

// Main.cpp:605-634 `IsAbleToAim`
static bool ViceExtIsAbleToAim(CPed *ped, eWeaponType wt, CWeaponInfo *info)
{
	if (wt == WEAPONTYPE_UNARMED || wt == WEAPONTYPE_BRASSKNUCKLE)
		return false;
	switch (ped->m_nPedState) {
	case PED_NONE:
	case PED_IDLE:
	case PED_FLEE_POS:
	case PED_FLEE_ENTITY:
	case PED_ATTACK:
	case PED_FIGHT:
	case PED_AIM_GUN:
		// Main.cpp:626-630. El `!bIsDucking || CrouchFire` de :628 es de VC.
		return ped->m_nMoveState != PEDMOVE_SPRINT && ped->IsPedInControl()
			&& (!ped->bIsDucking || info->IsFlagSet(WEAPONFLAG_CROUCHFIRE));
	default:
		return false;
	}
}

// Main.cpp:692-717 `IsTypeMelee` (rama de VC: sin flag de apuntado = cuerpo a cuerpo)
static bool ViceExtIsTypeMelee(eWeaponType wt, CWeaponInfo *info)
{
	if (wt == WEAPONTYPE_UNARMED || wt == WEAPONTYPE_BRASSKNUCKLE)
		return true;
	return !ViceExtCanAim(wt, info) && !info->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM);
}

// Main.cpp:719-743 `IsTypeTwoHanded`: mismo predicado de apuntado, y la MISMA
// intención del parche de `m_bCanAim` del mod pero POR CONSULTA (decisión D3).
static bool ViceExtIsTypeTwoHanded(eWeaponType wt, CWeaponInfo *info, bool ducking)
{
	if (wt == WEAPONTYPE_UNARMED || wt == WEAPONTYPE_BRASSKNUCKLE)
		return false;
	// Main.cpp:730-740: el mod pone `CanAim = false` con el agachado y `true` sin él.
	// Aquí NO se muta la tabla global; se decide en el momento de preguntar.
	bool canAim = (wt == WEAPONTYPE_FLAMETHROWER || wt == WEAPONTYPE_MINIGUN)
		? !ducking
		: ViceExtCanAim(wt, info);
	return (canAim || info->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM))
		&& !info->IsFlagSet(WEAPONFLAG_THROW);
}

// Main.cpp:652-676 `IsWeaponPossiblyCompatible`
static bool ViceExtIsWeaponPossiblyCompatible(CPlayerPed *ped, eWeaponType wt, CWeaponInfo *info)
{
	if (wt == WEAPONTYPE_UNARMED || wt == WEAPONTYPE_BRASSKNUCKLE)
		return false;
	bool canAim = (wt == WEAPONTYPE_FLAMETHROWER || wt == WEAPONTYPE_MINIGUN)
		? !ped->bIsDucking
		: ViceExtCanAim(wt, info);
	return (canAim || info->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM))
		&& !info->IsFlagSet(WEAPONFLAG_THROW)
		&& !info->IsFlagSet(WEAPONFLAG_1ST_PERSON);
}

// Main.cpp:636-650 `IsType1stPerson`. El early-return va PRIMERO, antes de calcular
// `isAiming` (Main.cpp:1227-1228), para que esos modos los siga llevando CamControl.
static bool ViceExtIsType1stPerson(eWeaponType wt, CWeaponInfo *info)
{
	switch (wt) {
	case WEAPONTYPE_SNIPERRIFLE:
	case WEAPONTYPE_LASERSCOPE:
		// D2 (27/09): WEAPONTYPE_ROCKETLAUNCHER está en la lista del mod, pero en
		// nuestro árbol lo saca VICEEXT_ROCKET_3RD_PERSON (R16, validado). Si se
		// metiera aquí, R16 dejaría de funcionar.
		return true;
	default:
		return !ViceExtCanAim(wt, info) && !info->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM)
			&& info->IsFlagSet(WEAPONFLAG_1ST_PERSON);
	}
}

// Main.cpp:1474-1517 `Find3rdPersonMouseTarget`: la adquisición del "objetivo
// blando" de ratón, que es la que permite dibujar el triángulo sobre el ped.
void
CPlayerPed::ViceExtFind3rdPersonMouseTarget(void)
{
	if (ViceExtHasPadInHands())
		return;

	CWeapon *w = GetWeapon();
	if (w == nil)
		return;
	CWeaponInfo *info = CWeaponInfo::GetWeaponInfo(w->m_eWeaponType);
	if (info == nil)
		return;

	if (CCamera::s_viceExtAimLawActive && !bInVehicle && (!m_bHasLockOnTarget || !m_pPointGunAt)) {
		CVector source, target;
		CColPoint point = {};
		CEntity *e = nil;
		// Main.cpp:1488: el alcance del arma y la posición de la cámara.
		if (TheCamera.Find3rdPersonCamTargetVector(info->m_fRange,
		    TheCamera.Cams[TheCamera.ActiveCam].Source, source, target)) {
			// Main.cpp:1489-1493: los 12 flags son false,false,TRUE(solo peds),false…
			if (CWorld::ProcessLineOfSight(source, target, point, e,
			    false, false, true, false, false, false, false, false)
			    && e && e->IsPed()) {
				CPed *tp = (CPed*)e;
				if (tp != this && tp->m_nPedState != PED_DEAD
				    && CCamera::s_viceExtAimMouseTarget != tp) {
					CCamera::s_viceExtAimMouseTarget = tp;
					if (tp->CanSeeEntity(this, DEGTORAD(60.0f) * 2)) {
						tp->ReactToPointGun(this);
						// El mod dice `ped->Say(117)` (Main.cpp:1504) pero no sabemos
						// qué índice es 117 en la tabla de sonidos servida. Se usa el
						// `SOUND_PED_AIMING` con nombre, que es lo que ya reproduce
						// nuestro `FindWeaponLockOnTarget` (PlayerPed.cpp:1463).
						Say(SOUND_PED_AIMING);
					}
				}
				return;
			}
		}
	}
	CCamera::s_viceExtAimMouseTarget = nil;
}

// Main.cpp:547-574 `RotatePlayer`, con los dos cambios imprescindibles: el
// `SetRotateZOnly` del plugin-sdk es `GetMatrix().SetRotateZOnly` (Matrix.h:94) y
// `LimitRadianAngle` ya existe en CGeneral.
static void ViceExtRotatePlayer(CPed *ped, float angle, bool updateAnim)
{
	CVector pos = ped->GetPosition();
	CColPoint cp;
	CEntity *entity = nil;
	// Main.cpp:551-556: no se recalcula si el ped está de pie sin input.
	if (CWorld::ProcessLineOfSight(pos, pos + CVector(0.0f, 0.0f, 1.0f), cp, entity,
	    true, true, true, true, true, true, false, false) == false) {
		angle = ped->m_fRotationCur;
	}
	float newHeading = (float)CGeneral::LimitRadianAngle(-angle);
	float curHeading = ped->m_fRotationCur;
	ped->SetHeading(newHeading);
	ped->GetMatrix().SetRotateZOnly(ped->m_fRotationCur);
	if (updateAnim)
		ped->m_fRotationCur = newHeading;
	else
		ped->m_fRotationDest = newHeading;
	(void)curHeading;
}

// Main.cpp:1203-1461 `ProcessPlayerPedControl` (la parte de VC; la de GTA3 no entra).
void
CPlayerPed::ViceExtProcessPlayerPedControl(void)
{
	CCam &cam = TheCamera.Cams[TheCamera.ActiveCam];
	int16 mode = cam.Mode;
	CPad *pad = CPad::GetPad(0);
	CWeapon *curW = GetWeapon();
	if (curW == nil)
		return;
	eWeaponType wt = curW->m_eWeaponType;
	CWeaponInfo *info = CWeaponInfo::GetWeaponInfo(wt);
	if (info == nil)
		return;

	// Main.cpp:1207-1208
	// Main.cpp:1207 usa `TheCamera.m_fOrientation` (plugin-sdk) = rumbo de la camara.
	// En nuestro arbol el MISMO valor es `TheCamera.Orientation` (Camera.h:485), y es
	// EXACTAMENTE lo que el motor ya pasa a `SetLookFlag`/`SetAimFlag` 350 lineas mas
	// abajo, en su propio bloque de apuntado (PlayerPed.cpp:1793):
	//     float limitedCam = CGeneral::LimitRadianAngle(-TheCamera.Orientation);
	// ERROR QUE ESTE CODIGO TENIA (medido el 28/09, R9): yo usaba `Beta` y ademas
	// forzaba `SetHeading`, de modo que el cuerpo quedaba 90 grados desviado del
	// rumbo real (`desv=90.0` constante con la Colt45, y R9 antes daba OK). Motivo:
	// (a) `Beta` no es `Orientation` (este lo calcula la camara y es el que manda), y
	// (b) el motor NO gira el cuerpo con `SetHeading` mientras apunta de libre: lo
	//     deja en la maquinaria de rumbo (`m_fRotationDest` + `m_headingRate`), que es
	//     lo que evita el tiron. Forzar `SetHeading` por encima pelea con ella.
	// Asi que se usa el valor del motor y se DEJA su maquinaria: la reimplementacion
	// del `RotatePlayer` del mod (Main.cpp:547-574) es esa maquinaria, aqui ya existe.
	float front = CGeneral::LimitRadianAngle(-TheCamera.Orientation);
	float height = TheCamera.Find3rdPersonQuickAimPitch();

	CCamera::s_viceExtAimLawActive = false;

	// Main.cpp:1227-1228: PRIMERO el early-return de 1.ª persona, antes de nada.
	if (ViceExtIsType1stPerson(wt, info))
		return;

	// Main.cpp:1230-1235, literal.
	bool aiming = !pad->ArePlayerControlsDisabled() && ViceExtIsAbleToAim(this, wt, info)
		&& pad->GetTarget() != 0 && TheCamera.GetLookDirection() != 0
		&& ViceExtIsWeaponPossiblyCompatible(this, wt, info)
		&& (mode == CCam::MODE_FOLLOWPED || mode == CCam::MODE_AIMING)
		&& curW->HasWeaponAmmoToBeUsed()
		&& !pad->JumpJustDown() && !pad->GetSprint()
		&& !ViceExtPedOwns(PEDLANE_NADO, PEDCAP_APUNTAR);

#ifdef __EMSCRIPTEN__
	// ESTE bloque va FUERA del `if (aiming)` a propósito: el detector de flanco
	// estaba DENTRO, y ahí solo podía ver la entrada (`aim=1`), nunca la salida.
	// Por eso el log del 28/09 traía UNA sola línea `AIMLAW` y el auto-aim no se
	// podía juzgar. Ahora ve los dos flancos, que es lo que mide §8.9.
	{
		static int8 s_odLawWas = -1;
		int8 cur = (int8)(aiming ? 1 : 0);
		if (cur != s_odLawWas) {
			s_odLawWas = cur;
			char t[210];
			snprintf(t, sizeof t, "AIMLAW aim=%d modo=%d estado=%d arma=%d lock=%d aut=%d esp=%.2f aim2=%d pad=%d",
				(int)cur, (int)mode, (int)m_nPedState, (int)wt, (int)m_bHasLockOnTarget,
				(int)ViceExtIsAutoAimDisabled(), front, (int)CCamera::s_viceExtAimLawActive,
				(int)ViceExtHasPadInHands());
			ODTRACES(t);
		}
	}
#endif

	if (aiming) {
		// Main.cpp:1237-1244, literal. `TakeControl` deja `m_bLookingAtPlayer = false`
		// (Camera.cpp:2354) y ESO es lo que hace que CamControl deje de enrutar a
		// MODE_SYPHON mientras apuntamos: su bloque `if(m_bLookingAtPlayer)` no corre.
		// NO se toca CamControl. Al soltar hay que devolver `m_bLookingAtPlayer = true`
		// (Main.cpp:1419) o el jugador se queda en MODE_AIMING para siempre.
		if (mode != CCam::MODE_AIMING && IsPedInControl() && TheCamera.m_uiTransitionState == 0) {
			CVector odView = TheCamera.GetMatrix().GetForward();
			float odVL = odView.Magnitude();
			if (odVL > 0.0001f) odView /= odVL;
			CCamera::s_viceExtAimViewDir = odView;
			CCamera::s_viceExtAimViewMode = CCam::MODE_AIMING;
			CCamera::s_viceExtAimViewPending = true;
			TheCamera.TakeControl(this, CCam::MODE_AIMING, INTERPOLATION, CAMCONTROL_GAME);
#ifdef __EMSCRIPTEN__
			ViceExtP4CamBegin(this, odView, (int)mode, (int)CCam::MODE_AIMING);
#endif
			CCamera::s_viceExtAimLawActive = true;
			CCamera::s_viceExtAimSwitchSpeed = true;
			CCamera::s_viceExtAimPrevHor = cam.Beta;
			CCamera::s_viceExtAimPrevVer = cam.Alpha;
			CCamera::s_viceExtAimPrevCamMode = mode;
		}
		CCamera::s_viceExtAimLawActive = true;

		// Main.cpp:1247-1286. §5.2(a): con ratón no se busca fijado; el cambio MANUAL
		// (`ShiftTargetLeft/RightJustDown`) se conserva, que es como se fija uno.
		CEntity *p = m_pPointGunAt;
#ifdef __EMSCRIPTEN__
		static bool s_odLockWas2 = false;
#endif
		if (!ViceExtIsAutoAimDisabled()) {
			if (m_bHasLockOnTarget && p) {
				// Main.cpp:1255. El rumbo al objetivo lo pone el motor por su cuenta
				// (`m_pPointGunAt` + `UpdateAimingCoors`), asi que `front` NO se sobrepone
				// aqui: sobreponerlo era lo que abria el cuerpo del objetivo.
				CVector diff = p->GetPosition() - GetPosition();
				// Main.cpp:1257-1271: la altura sale de la posición EN PANTALLA del
				// objetivo (cabeza del ped + 0,25 m). El `out.y / SCREEN_HEIGHT` del
				// mod es código muerto (su función ignora el argumento), así que
				// nuestra firma sin parámetro da el mismo resultado.
				CVector in = p->GetPosition();
				if (p->IsPed())
					((CPed*)p)->m_pedIK.GetComponentPosition(in, PED_HEAD);
				in.z += 0.25f;
				RwV3d out;
				float w, h;
				if (CSprite::CalcScreenCoors(in, &out, &w, &h, false))
					height = TheCamera.Find3rdPersonQuickAimPitch();
				// Main.cpp:1277-1280: se suelta al mover el ratón >1.0, o si el
				// objetivo está encima del jugador.
				bool transitionDone = TheCamera.m_uiTransitionState == 0;
				bool mouseMoved = Abs(pad->GetMouseX()) > 1.0f || Abs(pad->GetMouseY()) > 1.0f;
			if ((transitionDone && mouseMoved) || diff.Magnitude() < 0.5f) {
#ifdef __EMSCRIPTEN__
				{ char t[80]; snprintf(t, sizeof t, "AIMLOCK hx=%.4f vx=%.4f tmo=%u off=cerca",
					0.0f, 0.0f, (unsigned)CCamera::s_viceExtAimLockOnUntil); ODTRACES(t); }
#endif
				ClearWeaponTarget();
			} else {
				// Main.cpp:826-830: al FIJAR se guardan la posición, el color por
				// salud y los 250 ms que vivirá la marca. El mod lo hacía dentro de su
				// `DrawAutoAimTarget`; aquí el dibujo está en `Hud.cpp` (Main.cpp:783)
				// y el ESCRITOR se había quedado sin sitio: el estado se quedaba en 0
				// y la marca no se dibujaba NUNCA (`marco=0` en 65/65 el 28/09). Se
				// escribe AQUÍ, en la rama de "el fijado se queda" (la de soltar no
				// publica nada, que es justo lo contrario de lo que pasó).
				{
					CVector odPos = p->GetPosition();
					float odHp = 1.0f;
					if (p->IsPed()) {
						CPed *odPd = (CPed*)p;
						odPd->m_pedIK.GetComponentPosition(odPos, PED_HEAD);
						odHp = Clamp(odPd->m_fHealth / 100.0f, 0.0f, 1.0f);
					}
					odPos.z += 0.25f;   // Main.cpp:824
					CCamera::s_viceExtAimLastLockPos = odPos;
					if (CCamera::s_viceExtAim.lockOnTargetType == 2)
						// LCS/VCS (:817): verde según la salud.
						CCamera::s_viceExtAimLastLockCol
							= CRGBA(0, (uint8)(odHp * 255.0f), 0, 255);
					else
						// SA (:815): de rojo a verde con la salud.
						CCamera::s_viceExtAimLastLockCol
							= CRGBA((uint8)((1.0f - odHp) * 255.0f), (uint8)(odHp * 255.0f), 0, 255);
					if (odHp <= 0.0f)
						CCamera::s_viceExtAimLastLockCol = CRGBA(0, 0, 0, 255);
					CCamera::s_viceExtAimLockOnUntil = 250 + CTimer::GetTimeInMilliseconds();
				}
#ifdef __EMSCRIPTEN__
					static bool s_odLockWas = false;
					if (s_odLockWas) {
						s_odLockWas = false;
						char t[80];
						snprintf(t, sizeof t, "AIMLOCK hx=%.4f vx=%.4f tmo=%u off=soltar",
							0.0f, 0.0f, (unsigned)(250 + CTimer::GetTimeInMilliseconds()));
						ODTRACES(t);
					}
#endif
				}
			} else if (pad->ShiftTargetLeftJustDown() || pad->ShiftTargetRightJustDown()) {
				FindWeaponLockOnTarget();
			}
		} else {
#ifdef __EMSCRIPTEN__
			if (s_odLockWas2) {
				s_odLockWas2 = false;
				char t[80];
				snprintf(t, sizeof t, "AIMLOCK hx=%.4f vx=%.4f tmo=%u off=raton",
					0.0f, 0.0f, (unsigned)(250 + CTimer::GetTimeInMilliseconds()));
				ODTRACES(t);
			}
#endif
		}
		if (m_bHasLockOnTarget && m_pPointGunAt)
#ifdef __EMSCRIPTEN__
			s_odLockWas2 = true;
#endif

		// Main.cpp:1288-1294. `RotatePlayer` del mod = la maquinaria de rumbo de
		// nuestro motor, que ya esta escrita y que gira el cuerpo solo; no se llama
		// `SetHeading` a proposito (ver la nota de `front` mas arriba: asi se midio el
		// error de 90 grados). Lo que si se replica literal son los flags.
		if (!bIsDucking)
			m_fRotationDest = front;
		SetLookFlag(front, true, true);
		SetAimFlag(front);
		SetLookTimer(INT32_MAX);
		m_bFreeAimActive = true;
		// Main.cpp:1296-1300
		m_fFPSMoveHeading = height;
		if (m_fFPSMoveHeading >  DEGTORAD(45.0f)) m_fFPSMoveHeading =  DEGTORAD(45.0f);
		if (m_fFPSMoveHeading < -DEGTORAD(45.0f)) m_fFPSMoveHeading = -DEGTORAD(45.0f);
		if (CCamera::s_viceExtAim.storiesPointingArm && info->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM))
			m_fFPSMoveHeading -= DEGTORAD(8.0f);
		// Main.cpp:1302-1305
		float torsoPitch = 0.0f;
		if (!info->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM) || bIsDucking)
			torsoPitch = m_fFPSMoveHeading;
		// Main.cpp:1307-1308: quieto del todo = una fuerza la locomoción real un
		// frame (el equivalente de su `forceRealMoveAnim`).
		if (m_vecMoveSpeed.Magnitude() < 0.01f)
			CCamera::s_viceExtAimForceRealMoveAnim = true;
		// Main.cpp:1310-1312: los TRES `MoveLimb`. Los nombres reales del árbol son
		// `m_headOrient`/`m_torsoOrient`/`m_lowerArmOrient` (PedIK.h:38/39/41); las
		// tablas estáticas sí conservan el nombre del mod (PedIK.h:45/44/48).
		m_pedIK.MoveLimb(m_pedIK.m_headOrient, m_pedIK.m_headOrient.yaw, 0.0f, CPedIK::ms_headInfo);
		m_pedIK.MoveLimb(m_pedIK.m_torsoOrient, 0.0f, torsoPitch, CPedIK::ms_torsoInfo);
		m_pedIK.MoveLimb(m_pedIK.m_lowerArmOrient, 0.0f, m_fFPSMoveHeading, CPedIK::ms_lowerArmInfo);

#ifdef __EMSCRIPTEN__
		{
			// AIMIK 1 Hz apuntando: el pitch del brazo y los tres limbos.
			static uint32 s_odNextIk = 0;
			uint32 odNow = CTimer::GetTimeInMilliseconds();
			if (s_odNextIk > odNow + 60000) s_odNextIk = 0;
			if (odNow >= s_odNextIk) {
				s_odNextIk = odNow + 1000;
				char t[200];
				float odDev = front - m_fRotationCur;
				while(odDev > PI) odDev -= 2.0f * PI;
				while(odDev < -PI) odDev += 2.0f * PI;
				odDev *= RADTODEG(1.0f);
				snprintf(t, sizeof t, "AIMIK pitch=%.4f alto=%.4f torso=%.3f h=%.3f t=%.3f a=%.3f bph=%.3f dev=%.1f spd=%.3f",
					m_fFPSMoveHeading, height, torsoPitch,
					m_pedIK.m_headOrient.yaw, m_pedIK.m_torsoOrient.pitch,
					m_pedIK.m_lowerArmOrient.pitch, m_fRotationCur * RADTODEG(1.0f), odDev,
					m_fMoveSpeed);
				ODTRACES(t);
			}
		}
#endif
	} else {
		// Main.cpp:1393-1425, al soltar.
		ClearPointGunAt();
		ClearWeaponTarget();
		// C18-4 (Main.cpp:1399-1408): si estaba agachado, con munición y sin recargar,
		// se queda agachado con `bCrouchWhenShooting`. `SetDuck` ya mezcla el clip de
		// agachado CON ARMA cuando esa bandera está puesta, que es exactamente el
		// `BlendAnimation(ANIM_GROUP_MAN, ANIM_MAN_WEAPON_CROUCH, 4.0f)` del mod.
		if (bIsDucking
		    && curW->m_eWeaponState != WEAPONSTATE_OUT_OF_AMMO
		    && curW->m_eWeaponState != WEAPONSTATE_RELOADING
		    && curW->m_nAmmoInClip > 0
		    && ViceExtIsAbleToAim(this, wt, info)) {
			bCrouchWhenShooting = true;
			SetDuck(60000, true);
		}
		// Main.cpp:1417-1425: devolver el modo ANTERIOR y dar la autoridad a CamControl.
		if (mode != CCamera::s_viceExtAimPrevCamMode && !pad->ArePlayerControlsDisabled()
		    && IsPedInControl()) {
			CVector odView = TheCamera.GetMatrix().GetForward();
			float odVL = odView.Magnitude();
			if (odVL > 0.0001f) odView /= odVL;
			CCamera::s_viceExtAimViewDir = odView;
			CCamera::s_viceExtAimViewMode = CCamera::s_viceExtAimPrevCamMode;
			CCamera::s_viceExtAimViewPending = true;
			TheCamera.TakeControl(this, CCamera::s_viceExtAimPrevCamMode, INTERPOLATION, CAMCONTROL_GAME);
#ifdef __EMSCRIPTEN__
			ViceExtP4CamBegin(this, odView, (int)mode, (int)CCamera::s_viceExtAimPrevCamMode);
#endif
			TheCamera.m_bLookingAtPlayer = true;
			CCamera::s_viceExtAimSwitchSpeed = true;
			CCamera::s_viceExtAimPrevHor = cam.Beta;
			CCamera::s_viceExtAimPrevVer = cam.Alpha;
			CCamera::s_viceExtAimPrevCamMode = mode;
		}
	}
}

void
CPlayerPed::ProcessControl(void)
{
	// ClassicAXIS C17 `WalkKey` (classicaxis_Main.cpp:1196-1201 y 1215-1217): con
	// la tecla de andar pulsada la velocidad es 0, para poder CAMINAR en vez de
	// correr. Va aqu\u00ed, al principio de `ProcessControl` y antes de cualquier
	// predicado de apuntado, que es donde lo lee el mod (Main.cpp:1215, justo
	// antes de su bloque de `isAiming`).
	// Se lee de laacci\u00f3n REBINDABLE `PED_WALK` (mismo patron que `PED_RELOAD`,
	// PlayerPed.cpp:2230), as\u00ed que vale con cualquier m\u00e9todo de control y no
	// ata la tecla. `GetIsKeyboardKeyDown` (tenido) y no `...JustDown` (flanco),
	// porque el efecto es mientras se mantenga pulsada.
	// El "NULL para desactivar" del INI del mod es la tecla 0 -> `if (odWalk)`: si
	// el jugador rebinda la acci\u00f3n a `NULL` en el men\u00fa de controles, el
	// `GetControllerKeyAssociatedWithAction` devuelve 0 y la tecla deja de hacer
	// nada, que es justo lo que significa `NULL` en `ClassicAxisVC.ini:8`.
	{
		RsKeyCodes odWalk = (RsKeyCodes)ControlsManager.GetControllerKeyAssociatedWithAction(PED_WALK, KEYBOARD);
		bool odWalkDown = (odWalk != (RsKeyCodes)0) && ControlsManager.GetIsKeyboardKeyDown(odWalk);
		if (odWalkDown)
			m_fMoveSpeed = 0.0f;
		// Traza por FLANCO (pulsar y soltar), no por frame: `AIMWALK tecla=%d spd=%.2f`.
		// El criterio de \u00a78 es `spd=0.00` con la tecla pulsada y la velocidad de
		// antes al soltar.
#ifdef __EMSCRIPTEN__
		static bool s_odWalkWas = false;
		if (odWalkDown != s_odWalkWas) {
			s_odWalkWas = odWalkDown;
			char t[80];
			snprintf(t, sizeof t, "AIMWALK tecla=%d spd=%.2f", (int)odWalk, m_fMoveSpeed);
			ODTRACES(t);
		}
#endif
	}

	// ClassicAXIS: la ley de apuntado DEL MOD entra a sustituir el control de
	// apuntado del motor. Va antes de `CPed::ProcessControl()` y antes que la
	// seccion de nado (`:4465`) y de agachado (`:4472`), que no toca.
	ViceExtProcessPlayerPedControl();
	// El objetivo blando de raton (Main.cpp:1474-1517) lo acquisitiona el propio
	// mod DESPUES de su control, en el mismo punto (Main.cpp:307).
	ViceExtFind3rdPersonMouseTarget();

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
#ifdef VICEEXT_AIM_CLASSICAXIS
	ViceExtPrepareMove(GetPadFromPlayer(this));
#endif
	CPed::ProcessControl();
#ifdef __EMSCRIPTEN__
#ifdef VICEEXT_AIM_CLASSICAXIS
	ViceExtMoveTick(this);
	ViceExtP4CamTick(this);
#endif
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
#ifdef VICEEXT_MANUAL_RELOAD
	// Sección 3 (Vice Extended v2.5, "Reloading a weapon on the key"): recarga a
	// mano con PED_RELOAD (R por defecto, rebindable). A pie y con el control del
	// jugador: en vehículo la munición del drive-by tiene su propio camino y con
	// una cinemática/transición el ped no está en control.
	// C3.1b (20/09): la lista blanca de estados era demasiado estrecha (sólo
	// NONE/IDLE/ATTACK/FIGHT/AIM_GUN/SNIPER). Ahora basta con estar a pie, en
	// control y **no** estar muriendo, conduciendo, cayendo o saltando.
	if (padUsed && !bInVehicle && IsPedInControl()
	    && m_nPedState != PED_DIE && m_nPedState != PED_DEAD && m_nPedState != PED_DRIVING
	    && m_nPedState != PED_FALL && m_nPedState != PED_JUMP)
		ViceExtTryManualReload();
#ifdef __EMSCRIPTEN__
	// R10: cierre de la recarga (`done` con cargador lleno y `total` reducido, o
	// `fail` con el motivo del aborto). Sólo del arma del jugador y sólo al
	// cambiar de estado, así que no da ruido aunque la partida tenga NPCs
	// recargando. Sólo la recarga que pidió el jugador (s_viceExtManualReload):
	// la automática al vaciar el cargador también pasa por este estado.
	{
		static int8 prevWeaponState = WEAPONSTATE_READY;
		int8 nowState = (int8)GetWeapon()->m_eWeaponState;
		if (prevWeaponState == WEAPONSTATE_RELOADING && nowState != WEAPONSTATE_RELOADING
		    && s_viceExtManualReload) {
			int odClip = GetWeapon()->m_nAmmoInClip;
			int odCap = GetWeapon()->GetInfo()->m_nAmountofAmmunition;
			char t[160];
			if (odClip >= odCap || odClip > s_viceExtReloadStartClip) {
				snprintf(t, sizeof t, "VICEEXT reload done manual=1 clip=%d/%d total=%d",
					odClip, odCap, GetWeapon()->m_nAmmoTotal);
			} else {
				const char *motivo = bInVehicle ? "vehiculo"
				    : DyingOrDead() ? "muerto"
				    : ((int)GetWeapon()->m_eWeaponType != s_viceExtReloadStartType ? "arma-cambiada"
				    : "interrumpida");
				snprintf(t, sizeof t, "VICEEXT reload fail motivo=%s clip=%d/%d total=%d",
					motivo, odClip, odCap, GetWeapon()->m_nAmmoTotal);
			}
			ODTRACES(t);
		}
		if (nowState != WEAPONSTATE_RELOADING)
			s_viceExtManualReload = false;
		prevWeaponState = nowState;
	}
#endif
#endif
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

	if (padUsed && padUsed->WeaponJustDown() && !TheCamera.Using1stPersonWeaponMode()) {
		// ...Really?
		eWeaponType playerWeapon = FindPlayerPed()->GetWeapon()->m_eWeaponType;
		if (playerWeapon == WEAPONTYPE_SNIPERRIFLE || playerWeapon == WEAPONTYPE_LASERSCOPE) {
			DMAudio.PlayFrontEndSound(SOUND_WEAPON_SNIPER_SHOT_NO_ZOOM, 0);
		} else if (playerWeapon == WEAPONTYPE_ROCKETLAUNCHER) {
			DMAudio.PlayFrontEndSound(SOUND_WEAPON_ROCKET_SHOT_NO_ZOOM, 0);
		}
	}

#ifdef VICEEXT_SWIMMING
	// Sección 3, bloque C4 (20/09): nadar. Va antes del reparto de control
	// porque mientras se nada el control a pie no debe correr (ver
	// ViceExtSwimControl). Si viene de una caída al agua, el estado se deja en
	// PED_IDLE: ya no está cayendo, está nadando.
	bool odSwimming = ViceExtSwimControl(padUsed);
	if (odSwimming && (m_nPedState == PED_FALL || m_nPedState == PED_JUMP))
		SetPedState(PED_IDLE);
#endif
#ifdef VICEEXT_CROUCH
	// R6: el agachado ya NO se lleva el movimiento (sólo clip + tope). El
	// control a pie normal corre siempre; el tope se aplica después.
	ViceExtCrouchControl(padUsed);
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
			if (!RpAnimBlendClumpGetFirstAssociation(GetClump(), ASSOC_BLOCK) && !m_attachedTo) {
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
#ifdef VICEEXT_CROUCH
				// R6: tope de velocidad agachado tras el control normal.
				ViceExtCrouchLimitSpeed();
#endif
			}
		if (IsPedInControl() && m_nPedState != PED_ANSWER_MOBILE && padUsed)
#ifdef VICEEXT_SWIMMING
			// R30 (29/09): nadando no hay arma que procesar (punos; el mod no dispara en
			// el agua). Sin esto el clic abria mele y tapaba el nado (swim13).
			if (!CPlayerPed::ViceExtIsSwimming())
#endif
			ProcessPlayerWeapon(padUsed);
#ifdef VICEEXT_AIM_WALK
		// R9: medir (1/s apuntando) antes de tocar nada.
		ViceExtAimDirTrace(padUsed);
#endif
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
		if (!CPad::GetPad(0)->GetTarget())
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

	if (TheCamera.m_WideScreenOn || bIsDucking
#ifdef VICEEXT_CROUCH
	    // R17: agachado, el cuerpo lo lleva el clip del mod (ya NO parcial): una
	    // idle de aburrimiento (que sí es no parcial) retiraría el clip de
	    // agachado y el ped se pondría de pie solo tras 25-30 s sin tocar nada.
	    || odCrouched
#endif
	   )
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
