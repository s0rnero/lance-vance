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
	NUM_PEDLANES
};

ePedLane ViceExtPedOwner(ePedCap c);
bool     ViceExtPedOwns(ePedLane l, ePedCap c);
bool     ViceExtPedClaim(ePedLane l, ePedCap c);
void     ViceExtPedRelease(ePedLane l, ePedCap c);
void     ViceExtPedOnRelease(ePedLane l, void (*fn)(ePedCap, void *), void *ud);
void     ViceExtPedResetAll(void);

// T23: un solo chapuzon por ventana para el jugador. `prioridad` (el salto al
// agua y la salida a tierra seca) siempre pasa y ademas arma el reloj, que es
// lo que calla al salpicado de someras; los NPCs no llegan a pasar por aqui.
bool     ViceExtSplashGate(const char *sitio, int num, bool prioridad, float z);

// T34: cuanto empuja el agua hacia arriba en el ultimo cuadro, en la forma en
// que lo usa el mod para decidir entrar al nado (impulsoZ/masa > gravedad*ts).
// Se guarda aqui y no en CPed para no tocar el layout validado (0x5F4).
void     ViceExtSetFlota(bool flota);
bool     ViceExtFlota(void);
