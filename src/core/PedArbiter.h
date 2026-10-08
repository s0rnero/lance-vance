#pragma once

enum ePedCap {
	PEDCAP_MOVIMIENTO = 0,
	PEDCAP_POSTURA,
	PEDCAP_APUNTAR,
	PEDCAP_CAMARA,
	PEDCAP_ARMA,
	NUM_PEDCAPS
};

enum ePedLane {
	PEDLANE_MOTOR = 0,
	PEDLANE_NADO,
	PEDLANE_AGACHADO_MOD,
	PEDLANE_LEY,
	NUM_PEDLANES
};

ePedLane ViceExtPedOwner(ePedCap c);
bool     ViceExtPedOwns(ePedLane l, ePedCap c);
bool     ViceExtPedClaim(ePedLane l, ePedCap c);
void     ViceExtPedRelease(ePedLane l, ePedCap c);
void     ViceExtPedOnRelease(ePedLane l, void (*fn)(ePedCap, void *), void *ud);
void     ViceExtPedResetAll(void);
