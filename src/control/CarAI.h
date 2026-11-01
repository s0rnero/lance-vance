#pragma once

#include "AutoPilot.h"

class CVehicle;

class CCarAI
{
public:
	static float FindSwitchDistanceClose(CVehicle*);
	static float FindSwitchDistanceFarNormalVehicle(CVehicle*);
	static float FindSwitchDistanceFar(CVehicle*);
	static void BackToCruisingIfNoWantedLevel(CVehicle*);
#ifdef VICEEXT_IMPROVED_TECH_PART
	static void BackToCruisingIfPlayerLost(CVehicle*);
#endif
	static void UpdateCarAI(CVehicle*);
	static void CarHasReasonToStop(CVehicle*);
	static float GetCarToGoToCoors(CVehicle*, CVector*);
	static float GetCarToParkAtCoors(CVehicle*, CVector*);
	static void AddPoliceCarOccupants(CVehicle*);
	static void AddAmbulanceOccupants(CVehicle*);
	static void AddFiretruckOccupants(CVehicle*);
	static void TellOccupantsToLeaveCar(CVehicle*);
#ifdef VICEEXT_WANTED_PATHS
	static void TellCarToSeekWantedPaths(CVehicle*);
	static void TellOccupantsToLeaveCarImmediately(CVehicle*);
#endif
	static void TellOccupantsToFleeCar(CVehicle*);
	static void TellCarToRamOtherCar(CVehicle*, CVehicle*);
	static void TellCarToBlockOtherCar(CVehicle*, CVehicle*);
	static uint8 FindPoliceCarMissionForWantedLevel();
	static uint8 FindPoliceBoatMissionForWantedLevel();
	static int32 FindPoliceCarSpeedForWantedLevel(CVehicle*);
	static void MellowOutChaseSpeed(CVehicle*);
	static void MellowOutChaseSpeedBoat(CVehicle*);
	static void MakeWayForCarWithSiren(CVehicle *veh);
};
