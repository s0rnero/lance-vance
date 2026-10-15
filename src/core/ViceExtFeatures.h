#pragma once

// Vice Extended, familia 11 (`VICEEXT_FEATURES_INI`): el `features.ini` del mod
// MANDA. Portado de su lector (arbol fuente `x87/gta-extended`, rama `miami`,
// `src/core/re3.cpp:284`: `ReadAndGetFeature` + `ReadAndGetWaypointColor` sobre
// `ViceExtended/features.ini`, con mINI).
//
// Diferencias con el suyo, a proposito:
//   - Su lector devuelve 0 cuando la clave no esta (`return 0`): sin fichero, TODO
//     apagado. Aqui, sin clave (o sin fichero) se usa el valor del PORT (los
//     `#define` de `config.h`): la paridad que el jugador ya juega. Asi servir su
//     fichero no puede apagar nada por accidente.
//   - El suyo lee en el arranque estatico del EXE; aqui se lee una vez por
//     `Load()`, desde el arranque del juego.
//   - Y queda trazado: `P4 kind=inifile` con una linea por clave (`ini` = lo que
//     trae el fichero, `aplica` = lo que usa el motor, `src` = de donde sale).
//
// Quien consulte una clave usa `On()`/`Waypoint()`; nadie lee el fichero por su
// cuenta (un solo duenno, como el resto de familias).

namespace ViceExtFeatures
{
	// Lee `ViceExtended/features.ini` una vez (idempotente). Se llama desde
	// `CGame::InitialiseOnceBeforeRW`, antes de que el juego use ninguna clave.
	void Load(void);

	// El fichero manda; sin la clave (o sin fichero) vale `portDefault`, que es el
	// valor del `#define VICEEXT_*` correspondiente. Las claves con decision del
	// jugador (`kOverrides`, en el .cpp) NO se pueden mover desde el fichero.
	bool On(const char *key, bool portDefault);

	// `WaypointColorRGB`; `channel` 0=R, 1=G, 2=B. Sin la clave vale `portDefault`.
	int Waypoint(int channel, int portDefault);

	// Solo para la traza y el verificador: 1 si el fichero se pudo leer.
	bool WasRead(void);
}
