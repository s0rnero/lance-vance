#pragma once

#include "Crime.h"

class CEntity;
class CCopPed;

// Etiqueta de sesión de la traza de la sección 2 (P1). La pone la sonda en
// `window.__vcWantedTag` y viaja en cada línea: el servidor de desarrollo
// comparte UN odtrace.log entre todas las pestañas y los tres agentes, así que
// sin etiqueta las líneas de dos sesiones se mezclan (y una sonda ajena puede
// contaminar el veredicto de la otra).
unsigned WantedTraceTag(void);

class CWanted
{
public:
	int32 m_nChaos;
	int32 m_nMinChaos;
	int32 m_nLastUpdateTime;
	uint32 m_nLastWantedLevelChange;
	uint32 m_nLastTimeSuspended;
	float m_fCrimeSensitivity;
	uint8 m_CurrentCops;
	uint8 m_MaxCops;
	uint8 m_MaximumLawEnforcerVehicles;
	uint8 m_CopsBeatingSuspect;
	int16 m_RoadblockDensity;
	uint8 m_bIgnoredByCops : 1;
	uint8 m_bIgnoredByEveryone : 1;
	uint8 m_bSwatRequired : 1;
	uint8 m_bFbiRequired : 1;
	uint8 m_bArmyRequired : 1;
	int32 m_nWantedLevel;
	int32 m_nMinWantedLevel;
	CCrimeBeingQd m_aCrimes[16];
	CCopPed *m_pCops[10];

	static int32 MaximumWantedLevel;
	static int32 nMaximumWantedLevel;

#ifdef VICEEXT_HIDE_COPS
	// Vice Extended (v1.0 "Changed wanted system", sección 2 / P1): estado de la
	// búsqueda cuando el jugador se esconde. Los campos van AL FINAL a propósito:
	// ningún offset de la estructura original se mueve (CReplay copia `CWanted`
	// por valor con `PlayerWanted = *m_pWanted`).
	CVector m_vecLastKnownPos;   // última posición en la que un policía te vio
	uint32 m_nLastSeenTime;      // cuándo fue eso
	uint32 m_nHiddenSince;       // desde cuándo no te ve nadie (0 = te ven)
	uint32 m_nLastStarDrop;      // último descenso por estar escondido
	uint8 m_bHiding : 1;         // hay búsqueda en curso (nadie te ve, nivel > 1)

	// `fullSweep` = refresca también el barrido del pool de peds (caro); si es
	// false solo se miran los perseguidores. UpdateHiding() lo llama a 5 Hz.
	bool AnyCopSeesPlayer(const CVector &playerPos, bool fullSweep);
	void UpdateHiding(void);
	bool IsHiding(void) { return m_bHiding; }
	CVector GetLastKnownPos(void) { return m_vecLastKnownPos; }
#endif

public:
	void Initialise();
	bool AreMiamiViceRequired();
	bool AreSwatRequired();
	bool AreFbiRequired();
	bool AreArmyRequired();
	int32 NumOfHelisRequired();
	void SetWantedLevel(int32);
	void SetWantedLevelNoDrop(int32 level);
	int32 GetWantedLevel() { return m_nWantedLevel; }
	void CheatWantedLevel(int32 level);
	void RegisterCrime(eCrimeType type, const CVector &coors, uint32 id, bool policeDoesntCare);
	void RegisterCrime_Immediately(eCrimeType type, const CVector &coors, uint32 id, bool policeDoesntCare);
	void ClearQdCrimes();
	bool AddCrimeToQ(eCrimeType type, int32 id, const CVector &pos, bool reported, bool policeDoesntCare);
	void ReportCrimeNow(eCrimeType type, const CVector &coors, bool policeDoesntCare);
	void UpdateWantedLevel();
	void Reset();
	void ResetPolicePursuit();
	void UpdateCrimesQ();
	void Update();

	void Suspend();

	bool IsIgnored(void) { return m_bIgnoredByCops || m_bIgnoredByEveryone; }

	static int32 WorkOutPolicePresence(CVector posn, float radius);
	static void SetMaximumWantedLevel(int32 level);
};

VALIDATE_SIZE(CWanted, 0x204);
