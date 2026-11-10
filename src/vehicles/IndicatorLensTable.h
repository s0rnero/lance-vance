// Generado por gta_vc_browser/tools/gen_indicator_lens.py -- no editar a mano.
// Fuente medida: C:/Users/s0rno/OneDrive/Documents/lance-vance/gta_vc_browser/streamed/models/gta3.img
// Una fila por RACIMO de lente (caras de lente conectadas por vertice) de
// cada (modelo, nodo, costado). El color es la media de los TEXELOS AMBAR del
// racimo, medida sobre el footprint UV de cada cara (funciones de borde) y
// multiplicada por el color del material (el color del DFF es blanco: el
// tono vive en la textura, que el motor ya no tiene en CPU). La media de la
// cara entera no vale: mezclas el cristal con el marco y sale un tono que ya
// no pasa la ventana de matiz.
// amber: 1 solo si >= 50% del area del racimo es ambar medida
// (_is_amber + ventana de matiz + cobertura de texel). El motor no reinventa
// la regla: un nodo sin fila ambar es el propio faro o piloto y parpadea con
// su propia lampara (ruta Banshee del plan 42.1). Sin fila no hay destello.
// mats: mascara de indices de material del racimo dentro de su geometry; el
// motor pinta ESOS materiales y nada mas, que es lo que deja el panel del
// atomic estatico. xyz: centroide del racimo en cm del modelo; area_cm2: area
// ambar medida. Sirven para comprobar el emparejamiento fila<->grupo barrido
// en vez de confiar en el orden. Racimos: 296, ambar 278, de textura propia 173,
// del atlas `lights`/`lightson` del mod 123 (con su ambar medido de captura
// (253,165,0): el mod aplica (255,225,0) sobre texelas de lente con matiz;
// aqui la texela es blanca a proposito, asi que el tono vive en el material).
// atlas: 1 cuando TODO el racimo pinta sobre ese atlas del mod: sus texelos son
// blancos y el unico sitio donde cabe escribir el ambar es el color del material.
// Con texelos propios el pixel YA trae el tono y el material tiene que quedarse
// blanco: pintarlo multiplicaria los dos ambares y la lente saldria mas oscura.
// mats es una mascara de 32 bits (tope medido por geometry tras el split: 10).
#pragma once

struct ViceExtIndLens {
	const char *name;
	int8 node;
	int8 side;
	uint8 rgb[3];
	int8 amber;
	uint8 atlas;
	uint32 mats;
	uint16 area_cm2;
	int16 xyz[3];
};

static const ViceExtIndLens s_odIndLens[] = {
	{ "admiral", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 348, { -95, 235, -5 } },	// A 8 caras lights [atlas del mod]
	{ "admiral", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 310, { -99, -281, -7 } },	// A 4 caras lights [atlas del mod]
	{ "admiral", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 348, { 95, 235, -5 } },	// A 8 caras lights [atlas del mod]
	{ "admiral", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 310, { 99, -281, -7 } },	// A 4 caras lights [atlas del mod]
	{ "ambulan", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 155, { -75, 292, -26 } },	// A 2 caras lights [atlas del mod]
	{ "ambulan", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 155, { 75, 292, -26 } },	// A 2 caras lights [atlas del mod]
	{ "angel", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 28, { -18, 75, 26 } },	// A 2 caras lights [atlas del mod]
	{ "angel", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 42, { -17, -120, 1 } },	// A 2 caras lights [atlas del mod]
	{ "angel", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 28, { 18, 75, 26 } },	// A 2 caras lights [atlas del mod]
	{ "angel", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 42, { 17, -120, 1 } },	// A 2 caras lights [atlas del mod]
	{ "baggage", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 20, { -62, 175, 22 } },	// A 2 caras lights [atlas del mod]
	{ "baggage", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 5, { -66, 174, 23 } },	// A 1 caras lights [atlas del mod]
	{ "baggage", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 24, { -63, 175, 24 } },	// A 2 caras lights [atlas del mod]
	{ "baggage", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 5, { -58, 176, 23 } },	// A 1 caras lights [atlas del mod]
	{ "baggage", CAR_INDICATOR_LR, -1, { 193, 90, 41 }, 0, 0, 0x00000002, 5, { -55, -125, 24 } },	// o 1 caras baggage8bit128
	{ "baggage", CAR_INDICATOR_LR, -1, { 194, 89, 42 }, 0, 0, 0x00000002, 10, { -57, -124, 26 } },	// o 2 caras baggage8bit128
	{ "baggage", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 20, { 62, 175, 22 } },	// A 2 caras lights [atlas del mod]
	{ "baggage", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 5, { 66, 174, 23 } },	// A 1 caras lights [atlas del mod]
	{ "baggage", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 24, { 63, 175, 24 } },	// A 2 caras lights [atlas del mod]
	{ "baggage", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 5, { 58, 176, 23 } },	// A 1 caras lights [atlas del mod]
	{ "baggage", CAR_INDICATOR_RR, 1, { 193, 90, 42 }, 0, 0, 0x00000002, 5, { 55, -125, 24 } },	// o 1 caras baggage8bit128
	{ "baggage", CAR_INDICATOR_RR, 1, { 193, 89, 42 }, 0, 0, 0x00000002, 10, { 57, -124, 26 } },	// o 2 caras baggage8bit128
	{ "barracks", CAR_INDICATOR_LR, -1, { 167, 95, 25 }, 1, 0, 0x00000001, 35, { -112, -443, -75 } },	// A 3 caras barracks868bit128
	{ "barracks", CAR_INDICATOR_LR, -1, { 172, 108, 42 }, 1, 0, 0x00000001, 16, { -114, -444, -74 } },	// A 1 caras barracks868bit128
	{ "barracks", CAR_INDICATOR_RR, 1, { 167, 95, 25 }, 1, 0, 0x00000001, 35, { 112, -443, -75 } },	// A 3 caras barracks868bit128
	{ "barracks", CAR_INDICATOR_RR, 1, { 172, 108, 42 }, 1, 0, 0x00000001, 16, { 114, -444, -74 } },	// A 1 caras barracks868bit128
	{ "benson", CAR_INDICATOR_LF, -1, { 159, 81, 6 }, 1, 0, 0x00000001, 250, { -78, 249, -20 } },	// A 2 caras benson868bit128
	{ "benson", CAR_INDICATOR_RF, 1, { 159, 81, 6 }, 1, 0, 0x00000001, 250, { 78, 249, -20 } },	// A 2 caras benson868bit128
	{ "blistac", CAR_INDICATOR_LF, -1, { 178, 96, 21 }, 1, 0, 0x00000001, 144, { -94, 219, 3 } },	// A 4 caras blistac868bit128
	{ "blistac", CAR_INDICATOR_RF, 1, { 178, 96, 21 }, 1, 0, 0x00000001, 144, { 94, 219, 3 } },	// A 4 caras blistac868bit128
	{ "bobcat", CAR_INDICATOR_LF, -1, { 176, 91, 8 }, 1, 0, 0x00000001, 130, { -77, 218, -26 } },	// A 2 caras bobcat868bit128
	{ "bobcat", CAR_INDICATOR_LF, -1, { 183, 94, 8 }, 1, 0, 0x00000001, 37, { -101, 205, -7 } },	// A 2 caras bobcat868bit128
	{ "bobcat", CAR_INDICATOR_LF, -1, { 162, 84, 8 }, 0, 0, 0x00000001, 28, { -101, 205, -7 } },	// o 2 caras bobcat868bit128
	{ "bobcat", CAR_INDICATOR_RF, 1, { 179, 91, 6 }, 1, 0, 0x00000001, 124, { 77, 218, -26 } },	// A 2 caras bobcat868bit128
	{ "bobcat", CAR_INDICATOR_RF, 1, { 183, 95, 8 }, 1, 0, 0x00000001, 36, { 101, 205, -7 } },	// A 2 caras bobcat868bit128
	{ "boxville", CAR_INDICATOR_LR, -1, { 186, 95, 19 }, 1, 0, 0x00000001, 265, { -96, -308, 35 } },	// A 2 caras boxville868bit128
	{ "boxville", CAR_INDICATOR_RR, 1, { 186, 95, 19 }, 1, 0, 0x00000001, 265, { 96, -308, 35 } },	// A 2 caras boxville868bit128
	{ "burrito", CAR_INDICATOR_LF, -1, { 158, 82, 7 }, 1, 0, 0x00000001, 387, { -90, 244, -15 } },	// A 4 caras burrito868bit128
	{ "burrito", CAR_INDICATOR_LF, -1, { 158, 82, 7 }, 1, 0, 0x00000001, 169, { -90, 241, -34 } },	// A 4 caras burrito868bit128
	{ "burrito", CAR_INDICATOR_RF, 1, { 158, 82, 7 }, 1, 0, 0x00000001, 387, { 90, 244, -15 } },	// A 4 caras burrito868bit128
	{ "burrito", CAR_INDICATOR_RF, 1, { 158, 82, 7 }, 1, 0, 0x00000001, 169, { 90, 241, -34 } },	// A 4 caras burrito868bit128
	{ "bus", CAR_INDICATOR_LF, -1, { 159, 89, 24 }, 1, 0, 0x00000001, 242, { -92, 454, 0 } },	// A 6 caras bus8bit128
	{ "bus", CAR_INDICATOR_RF, 1, { 159, 89, 24 }, 1, 0, 0x00000001, 242, { 92, 454, 0 } },	// A 6 caras bus8bit128
	{ "cabbie", CAR_INDICATOR_LF, -1, { 139, 73, 14 }, 1, 0, 0x00000001, 70, { -113, 239, -27 } },	// A 3 caras cabbie8bit128
	{ "cabbie", CAR_INDICATOR_LF, -1, { 162, 94, 22 }, 1, 0, 0x00000001, 36, { -113, 241, -27 } },	// A 1 caras cabbie8bit128
	{ "cabbie", CAR_INDICATOR_LR, -1, { 139, 73, 14 }, 1, 0, 0x00000001, 72, { -111, -247, -27 } },	// A 3 caras cabbie8bit128
	{ "cabbie", CAR_INDICATOR_RF, 1, { 139, 73, 14 }, 1, 0, 0x00000001, 70, { 113, 239, -27 } },	// A 3 caras cabbie8bit128
	{ "cabbie", CAR_INDICATOR_RF, 1, { 162, 94, 22 }, 1, 0, 0x00000001, 36, { 113, 241, -27 } },	// A 1 caras cabbie8bit128
	{ "cabbie", CAR_INDICATOR_RR, 1, { 139, 73, 14 }, 1, 0, 0x00000001, 72, { 111, -247, -27 } },	// A 3 caras cabbie8bit128
	{ "cheetah", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 163, { -88, 252, -24 } },	// A 2 caras lights [atlas del mod]
	{ "cheetah", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 142, { -77, -236, 4 } },	// A 16 caras lights [atlas del mod]
	{ "cheetah", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 163, { 88, 252, -24 } },	// A 2 caras lights [atlas del mod]
	{ "cheetah", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 142, { 77, -236, 4 } },	// A 16 caras lights [atlas del mod]
	{ "coach", CAR_INDICATOR_LF, -1, { 174, 100, 41 }, 1, 0, 0x00000001, 98, { -126, 560, -18 } },	// A 2 caras coach868bit128
	{ "coach", CAR_INDICATOR_RF, 1, { 177, 103, 43 }, 0, 0, 0x00000001, 46, { 119, 563, -15 } },	// o 1 caras coach868bit128
	{ "coach", CAR_INDICATOR_RF, 1, { 183, 113, 60 }, 1, 0, 0x00000001, 55, { 124, 560, -21 } },	// A 1 caras coach868bit128
	{ "enforcer", CAR_INDICATOR_LF, -1, { 136, 80, 18 }, 1, 0, 0x00000001, 143, { -94, 247, 28 } },	// A 6 caras enforcer868bit128
	{ "enforcer", CAR_INDICATOR_LR, -1, { 156, 100, 33 }, 1, 0, 0x00000001, 99, { -95, -388, -2 } },	// A 7 caras enforcer868bit128
	{ "enforcer", CAR_INDICATOR_LR, -1, { 139, 83, 20 }, 1, 0, 0x00000001, 81, { -119, -376, -28 } },	// A 5 caras enforcer868bit128
	{ "enforcer", CAR_INDICATOR_RF, 1, { 136, 80, 18 }, 1, 0, 0x00000001, 143, { 94, 247, 28 } },	// A 6 caras enforcer868bit128
	{ "enforcer", CAR_INDICATOR_RR, 1, { 157, 101, 32 }, 1, 0, 0x00000001, 77, { 91, -389, -3 } },	// A 5 caras enforcer868bit128
	{ "enforcer", CAR_INDICATOR_RR, 1, { 139, 83, 20 }, 1, 0, 0x00000001, 81, { 119, -376, -28 } },	// A 5 caras enforcer868bit128
	{ "esperant", CAR_INDICATOR_LF, -1, { 171, 94, 19 }, 1, 0, 0x00000001, 111, { -67, 261, -42 } },	// A 2 caras esperanto8bit128
	{ "esperant", CAR_INDICATOR_LF, -1, { 164, 91, 20 }, 1, 0, 0x00000001, 85, { -103, 239, -17 } },	// A 4 caras esperanto8bit128
	{ "esperant", CAR_INDICATOR_LF, -1, { 158, 89, 26 }, 0, 0, 0x00000001, 18, { -102, 245, -19 } },	// o 2 caras esperanto8bit128
	{ "esperant", CAR_INDICATOR_RF, 1, { 171, 94, 19 }, 1, 0, 0x00000001, 111, { 67, 261, -42 } },	// A 2 caras esperanto8bit128
	{ "esperant", CAR_INDICATOR_RF, 1, { 164, 91, 20 }, 1, 0, 0x00000001, 85, { 103, 239, -17 } },	// A 4 caras esperanto8bit128
	{ "esperant", CAR_INDICATOR_RF, 1, { 158, 89, 26 }, 0, 0, 0x00000001, 18, { 102, 245, -19 } },	// o 2 caras esperanto8bit128
	{ "faggio", CAR_INDICATOR_LR, -1, { 140, 76, 19 }, 1, 0, 0x00000001, 94, { -19, -96, -14 } },	// A 6 caras faggio8bit128
	{ "faggio", CAR_INDICATOR_RR, 1, { 140, 76, 19 }, 1, 0, 0x00000001, 94, { 19, -96, -14 } },	// A 6 caras faggio8bit128
	{ "firetruk", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 196, { -85, 402, 23 } },	// A 2 caras lights [atlas del mod]
	{ "firetruk", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 196, { -100, -421, 24 } },	// A 2 caras lights [atlas del mod]
	{ "firetruk", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 196, { 85, 402, 23 } },	// A 2 caras lights [atlas del mod]
	{ "firetruk", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000004, 196, { 100, -421, 24 } },	// A 2 caras lights [atlas del mod]
	{ "flatbed", CAR_INDICATOR_LR, -1, { 161, 94, 28 }, 1, 0, 0x00000001, 42, { -116, -443, -75 } },	// A 3 caras flatbed868bit128
	{ "flatbed", CAR_INDICATOR_LR, -1, { 169, 107, 43 }, 1, 0, 0x00000001, 16, { -114, -444, -74 } },	// A 1 caras flatbed868bit128
	{ "flatbed", CAR_INDICATOR_RR, 1, { 161, 94, 28 }, 1, 0, 0x00000001, 42, { 116, -443, -75 } },	// A 3 caras flatbed868bit128
	{ "flatbed", CAR_INDICATOR_RR, 1, { 169, 107, 43 }, 1, 0, 0x00000001, 16, { 114, -444, -74 } },	// A 1 caras flatbed868bit128
	{ "freeway", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 28, { -16, 74, 30 } },	// A 2 caras lights [atlas del mod]
	{ "freeway", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 27, { -18, -105, 12 } },	// A 2 caras lights [atlas del mod]
	{ "freeway", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 28, { 16, 74, 30 } },	// A 2 caras lights [atlas del mod]
	{ "freeway", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 27, { 18, -105, 12 } },	// A 2 caras lights [atlas del mod]
	{ "gangbur", CAR_INDICATOR_LF, -1, { 192, 116, 26 }, 1, 1, 0x00000001, 566, { -90, 243, -25 } },	// A 8 caras gangbur868bit128 [tinte 59.0, como pony]
	{ "gangbur", CAR_INDICATOR_RF, 1, { 192, 116, 26 }, 1, 1, 0x00000001, 566, { 90, 243, -25 } },	// A 8 caras gangbur868bit128 [tinte 59.0, como pony]
	{ "hellenbach", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 354, { -91, 256, -4 } },	// A 5 caras lights [atlas del mod]
	{ "hellenbach", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 57, { -84, 255, -8 } },	// A 1 caras lights [atlas del mod]
	{ "hellenbach", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 102, { -78, -241, -13 } },	// A 2 caras lights [atlas del mod]
	{ "hellenbach", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 411, { 90, 256, -5 } },	// A 6 caras lights [atlas del mod]
	{ "hellenbach", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 102, { 78, -241, -13 } },	// A 2 caras lights [atlas del mod]
	{ "hotrina", CAR_INDICATOR_LF, -1, { 166, 92, 33 }, 1, 0, 0x00000001, 62, { -89, 240, -23 } },	// A 1 caras hotrina8bit128
	{ "hotrina", CAR_INDICATOR_LF, -1, { 145, 76, 27 }, 1, 0, 0x00000001, 146, { -86, 243, -20 } },	// A 3 caras hotrina8bit128
	{ "hotrina", CAR_INDICATOR_RF, 1, { 165, 92, 33 }, 1, 0, 0x00000001, 62, { 89, 240, -23 } },	// A 1 caras hotrina8bit128
	{ "hotrina", CAR_INDICATOR_RF, 1, { 145, 76, 27 }, 1, 0, 0x00000001, 146, { 86, 243, -20 } },	// A 3 caras hotrina8bit128
	{ "hotrinb", CAR_INDICATOR_LF, -1, { 148, 77, 25 }, 1, 0, 0x00000001, 163, { -87, 237, -4 } },	// A 3 caras hotrinb8bit128
	{ "hotrinb", CAR_INDICATOR_LF, -1, { 162, 92, 38 }, 1, 0, 0x00000001, 44, { -85, 237, -1 } },	// A 1 caras hotrinb8bit128
	{ "hotrinb", CAR_INDICATOR_RF, 1, { 148, 77, 25 }, 1, 0, 0x00000001, 163, { 87, 237, -4 } },	// A 3 caras hotrinb8bit128
	{ "hotrinb", CAR_INDICATOR_RF, 1, { 162, 92, 38 }, 1, 0, 0x00000001, 44, { 85, 237, -1 } },	// A 1 caras hotrinb8bit128
	{ "hotring", CAR_INDICATOR_LF, -1, { 166, 92, 33 }, 1, 0, 0x00000001, 71, { -90, 237, -23 } },	// A 1 caras hotring8bit128
	{ "hotring", CAR_INDICATOR_LF, -1, { 145, 76, 27 }, 1, 0, 0x00000001, 150, { -86, 237, -20 } },	// A 3 caras hotring8bit128
	{ "hotring", CAR_INDICATOR_RF, 1, { 166, 92, 33 }, 1, 0, 0x00000001, 71, { 90, 237, -23 } },	// A 1 caras hotring8bit128
	{ "hotring", CAR_INDICATOR_RF, 1, { 145, 76, 27 }, 1, 0, 0x00000001, 150, { 86, 237, -20 } },	// A 3 caras hotring8bit128
	{ "idaho", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 97, { -97, 234, -10 } },	// A 2 caras lights [atlas del mod]
	{ "idaho", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 117, { -89, 240, -13 } },	// A 1 caras lights [atlas del mod]
	{ "idaho", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 117, { -93, 238, -6 } },	// A 1 caras lights [atlas del mod]
	{ "idaho", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 95, { 97, 234, -10 } },	// A 2 caras lights [atlas del mod]
	{ "idaho", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 117, { 89, 240, -13 } },	// A 1 caras lights [atlas del mod]
	{ "idaho", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 117, { 93, 238, -6 } },	// A 1 caras lights [atlas del mod]
	{ "infernus", CAR_INDICATOR_LF, -1, { 139, 81, 16 }, 1, 0, 0x00000001, 169, { -84, 272, -24 } },	// A 2 caras infernus868bit128
	{ "infernus", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 138, { -89, -258, 12 } },	// A 16 caras lights [atlas del mod]
	{ "infernus", CAR_INDICATOR_RF, 1, { 139, 81, 16 }, 1, 0, 0x00000001, 169, { 84, 272, -24 } },	// A 2 caras infernus868bit128
	{ "infernus", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 138, { 89, -258, 12 } },	// A 16 caras lights [atlas del mod]
	{ "kaufman", CAR_INDICATOR_LF, -1, { 138, 73, 15 }, 1, 0, 0x00000001, 69, { -113, 239, -27 } },	// A 3 caras cabbie8bit128
	{ "kaufman", CAR_INDICATOR_LF, -1, { 165, 96, 22 }, 1, 0, 0x00000001, 34, { -113, 241, -27 } },	// A 1 caras cabbie8bit128
	{ "kaufman", CAR_INDICATOR_LR, -1, { 139, 74, 15 }, 1, 0, 0x00000001, 70, { -111, -247, -27 } },	// A 3 caras cabbie8bit128
	{ "kaufman", CAR_INDICATOR_LR, -1, { 164, 95, 21 }, 1, 0, 0x00000001, 34, { -112, -246, -27 } },	// A 1 caras cabbie8bit128
	{ "kaufman", CAR_INDICATOR_RF, 1, { 138, 73, 15 }, 1, 0, 0x00000001, 69, { 113, 239, -27 } },	// A 3 caras cabbie8bit128
	{ "kaufman", CAR_INDICATOR_RF, 1, { 165, 96, 22 }, 1, 0, 0x00000001, 34, { 113, 241, -27 } },	// A 1 caras cabbie8bit128
	{ "kaufman", CAR_INDICATOR_RR, 1, { 139, 74, 15 }, 1, 0, 0x00000001, 70, { 111, -247, -27 } },	// A 3 caras cabbie8bit128
	{ "kaufman", CAR_INDICATOR_RR, 1, { 164, 95, 21 }, 1, 0, 0x00000001, 34, { 112, -246, -27 } },	// A 1 caras cabbie8bit128
	{ "landstal", CAR_INDICATOR_LF, -1, { 177, 89, 8 }, 1, 0, 0x00000001, 61, { -70, 217, -47 } },	// A 2 caras landstal868bit128
	{ "landstal", CAR_INDICATOR_LF, -1, { 184, 92, 7 }, 1, 0, 0x00000001, 151, { -94, 203, -5 } },	// A 2 caras landstal868bit128
	{ "landstal", CAR_INDICATOR_LF, -1, { 177, 89, 9 }, 0, 0, 0x00000001, 37, { -97, 200, -10 } },	// o 1 caras landstal868bit128
	{ "landstal", CAR_INDICATOR_RF, 1, { 177, 89, 8 }, 1, 0, 0x00000001, 61, { 70, 217, -47 } },	// A 2 caras landstal868bit128
	{ "landstal", CAR_INDICATOR_RF, 1, { 184, 92, 7 }, 1, 0, 0x00000001, 151, { 94, 203, -5 } },	// A 2 caras landstal868bit128
	{ "landstal", CAR_INDICATOR_RF, 1, { 177, 89, 9 }, 0, 0, 0x00000001, 37, { 97, 200, -10 } },	// o 1 caras landstal868bit128
	{ "linerun", CAR_INDICATOR_LF, -1, { 167, 94, 29 }, 1, 0, 0x00000001, 421, { -107, 442, -90 } },	// A 5 caras linerun868bit128
	{ "linerun", CAR_INDICATOR_RF, 1, { 167, 94, 29 }, 1, 0, 0x00000001, 421, { 107, 442, -90 } },	// A 5 caras linerun868bit128
	{ "lovefist", CAR_INDICATOR_LF, -1, { 147, 80, 30 }, 1, 0, 0x00000001, 71, { -94, 294, -13 } },	// A 1 caras lovefist868bit128
	{ "lovefist", CAR_INDICATOR_LF, -1, { 151, 82, 30 }, 1, 0, 0x00000001, 201, { -99, 282, -11 } },	// A 2 caras lovefist868bit128
	{ "lovefist", CAR_INDICATOR_RF, 1, { 147, 80, 30 }, 1, 0, 0x00000001, 71, { 94, 294, -13 } },	// A 1 caras lovefist868bit128
	{ "lovefist", CAR_INDICATOR_RF, 1, { 151, 82, 30 }, 1, 0, 0x00000001, 201, { 99, 282, -11 } },	// A 2 caras lovefist868bit128
	{ "manana", CAR_INDICATOR_LF, -1, { 157, 93, 30 }, 1, 0, 0x00000001, 29, { -86, 207, -1 } },	// A 1 caras manana868bit128
	{ "manana", CAR_INDICATOR_RF, 1, { 157, 93, 30 }, 1, 0, 0x00000001, 29, { 86, 207, -1 } },	// A 1 caras manana868bit128
	{ "mesa", CAR_INDICATOR_LF, -1, { 170, 87, 7 }, 1, 0, 0x00000001, 67, { -43, 207, -25 } },	// A 2 caras mesa868bit128
	{ "mesa", CAR_INDICATOR_RF, 1, { 171, 87, 8 }, 1, 0, 0x00000001, 67, { 43, 207, -25 } },	// A 2 caras mesa868bit128
	{ "moonbeam", CAR_INDICATOR_LF, -1, { 165, 80, 21 }, 1, 0, 0x00000001, 265, { -89, 240, -23 } },	// A 2 caras moonbm8bit128
	{ "moonbeam", CAR_INDICATOR_LR, -1, { 181, 96, 28 }, 1, 0, 0x00000001, 96, { -78, -259, -60 } },	// A 6 caras moonbm8bit128
	{ "moonbeam", CAR_INDICATOR_LR, -1, { 125, 50, 5 }, 1, 0, 0x00000001, 18, { -76, -258, -66 } },	// A 1 caras moonbm8bit128
	{ "moonbeam", CAR_INDICATOR_LR, -1, { 138, 52, 7 }, 1, 0, 0x00000001, 18, { -77, -259, -54 } },	// A 1 caras moonbm8bit128
	{ "moonbeam", CAR_INDICATOR_RF, 1, { 165, 80, 21 }, 1, 0, 0x00000001, 265, { 89, 240, -23 } },	// A 2 caras moonbm8bit128
	{ "moonbeam", CAR_INDICATOR_RR, 1, { 181, 96, 28 }, 1, 0, 0x00000001, 96, { 78, -259, -60 } },	// A 6 caras moonbm8bit128
	{ "moonbeam", CAR_INDICATOR_RR, 1, { 125, 50, 5 }, 1, 0, 0x00000001, 18, { 76, -258, -66 } },	// A 1 caras moonbm8bit128
	{ "moonbeam", CAR_INDICATOR_RR, 1, { 138, 52, 7 }, 1, 0, 0x00000001, 18, { 77, -259, -54 } },	// A 1 caras moonbm8bit128
	{ "mrwhoop", CAR_INDICATOR_LR, -1, { 178, 89, 27 }, 1, 0, 0x00000001, 89, { -87, -223, 15 } },	// A 1 caras mrwhoo868bit128
	{ "mrwhoop", CAR_INDICATOR_LR, -1, { 178, 89, 27 }, 1, 0, 0x00000001, 21, { -85, -223, 140 } },	// A 1 caras mrwhoo868bit128
	{ "mrwhoop", CAR_INDICATOR_LR, -1, { 191, 103, 43 }, 0, 0, 0x00000001, 48, { -79, -223, 18 } },	// o 1 caras mrwhoo868bit128
	{ "mrwhoop", CAR_INDICATOR_LR, -1, { 191, 103, 43 }, 0, 0, 0x00000001, 12, { -81, -223, 141 } },	// o 1 caras mrwhoo868bit128
	{ "mrwhoop", CAR_INDICATOR_RR, 1, { 178, 89, 27 }, 1, 0, 0x00000001, 89, { 79, -223, 15 } },	// A 1 caras mrwhoo868bit128
	{ "mrwhoop", CAR_INDICATOR_RR, 1, { 178, 89, 27 }, 1, 0, 0x00000001, 21, { 80, -223, 140 } },	// A 1 caras mrwhoo868bit128
	{ "mrwhoop", CAR_INDICATOR_RR, 1, { 191, 103, 43 }, 0, 0, 0x00000001, 48, { 86, -223, 18 } },	// o 1 caras mrwhoo868bit128
	{ "mrwhoop", CAR_INDICATOR_RR, 1, { 191, 103, 43 }, 0, 0, 0x00000001, 12, { 85, -223, 141 } },	// o 1 caras mrwhoo868bit128
	{ "packer", CAR_INDICATOR_LF, -1, { 170, 88, 33 }, 1, 0, 0x00000001, 379, { -106, 585, -92 } },	// A 4 caras packer8bit128
	{ "packer", CAR_INDICATOR_RF, 1, { 170, 88, 33 }, 1, 0, 0x00000001, 379, { 106, 585, -92 } },	// A 4 caras packer8bit128
	{ "patriot", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 161, { -100, 237, -10 } },	// A 24 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 219, { -69, -268, -12 } },	// A 24 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 161, { 100, 237, -10 } },	// A 24 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 219, { 69, -268, -12 } },	// A 24 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 27, { -118, 222, -1 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 24, { -117, 222, -4 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 18, { -117, 226, 0 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 24, { -117, 221, 3 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 18, { -117, 218, -1 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 33, { -118, -249, -18 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 28, { -117, -248, -22 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 21, { -117, -244, -18 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 28, { -117, -249, -15 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 21, { -117, -254, -19 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 27, { 118, 222, -1 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 24, { 117, 222, -4 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 18, { 117, 226, 0 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 24, { 117, 221, 3 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 18, { 117, 218, -1 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 33, { 118, -249, -18 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 28, { 117, -248, -22 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 21, { 117, -244, -18 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 28, { 117, -249, -15 } },	// A 2 caras lights [atlas del mod]
	{ "patriot", CAR_INDICATOR_2_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 21, { 117, -254, -19 } },	// A 2 caras lights [atlas del mod]
	{ "pcj600", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 88, { -14, -119, 20 } },	// A 2 caras lights [atlas del mod]
	{ "pcj600", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 88, { 14, -119, 20 } },	// A 2 caras lights [atlas del mod]
	{ "peren2", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 140, { -90, -276, 13 } },	// A 2 caras lights [atlas del mod]
	{ "peren2", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 47, { -91, -280, 5 } },	// A 2 caras lights [atlas del mod]
	{ "peren2", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 140, { 90, -276, 13 } },	// A 2 caras lights [atlas del mod]
	{ "peren2", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 47, { 91, -280, 5 } },	// A 2 caras lights [atlas del mod]
	{ "pizzaboy", CAR_INDICATOR_LR, -1, { 141, 74, 20 }, 1, 0, 0x00000001, 94, { -19, -96, -14 } },	// A 6 caras pizzaboy8bit128
	{ "pizzaboy", CAR_INDICATOR_RR, 1, { 141, 74, 20 }, 1, 0, 0x00000001, 94, { 19, -96, -14 } },	// A 6 caras pizzaboy8bit128
	{ "police", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 454, { -97, 226, -3 } },	// A 4 caras lights [atlas del mod]
	{ "police", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 151, { -50, -268, -7 } },	// A 2 caras lights [atlas del mod]
	{ "police", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 454, { 97, 226, -3 } },	// A 4 caras lights [atlas del mod]
	{ "police", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 151, { 50, -268, -7 } },	// A 2 caras lights [atlas del mod]
	{ "polwintergreen", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 56, { -22, 95, 38 } },	// A 8 caras lights [atlas del mod]
	{ "polwintergreen", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 56, { -25, -132, 4 } },	// A 8 caras lights [atlas del mod]
	{ "polwintergreen", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 56, { 22, 95, 38 } },	// A 8 caras lights [atlas del mod]
	{ "polwintergreen", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 56, { 25, -132, 4 } },	// A 8 caras lights [atlas del mod]
	{ "pony", CAR_INDICATOR_LF, -1, { 130, 60, 22 }, 1, 1, 0x00000001, 24, { -96, 256, -24 } },	// A 1 caras pony868bit128
	{ "pony", CAR_INDICATOR_RF, 1, { 130, 60, 22 }, 1, 1, 0x00000001, 23, { 96, 256, -24 } },	// A 1 caras pony868bit128
	{ "premier", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 151, { -50, -268, -7 } },	// A 2 caras lights [atlas del mod]
	{ "premier", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 151, { 50, -268, -7 } },	// A 2 caras lights [atlas del mod]
	{ "premier", CAR_INDICATOR_2_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 454, { -97, 226, -3 } },	// A 4 caras lights [atlas del mod]
	{ "premier", CAR_INDICATOR_2_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 454, { 97, 226, -3 } },	// A 4 caras lights [atlas del mod]
	{ "rancher", CAR_INDICATOR_LF, -1, { 168, 89, 12 }, 1, 0, 0x00000001, 123, { -86, 271, -41 } },	// A 2 caras ranch868bit128
	{ "rancher", CAR_INDICATOR_LF, -1, { 165, 88, 12 }, 1, 0, 0x00000001, 241, { -110, 258, -4 } },	// A 1 caras ranch868bit128
	{ "rancher", CAR_INDICATOR_LF, -1, { 175, 92, 13 }, 1, 0, 0x00000001, 241, { -113, 255, 11 } },	// A 1 caras ranch868bit128
	{ "rancher", CAR_INDICATOR_LR, -1, { 170, 90, 12 }, 1, 0, 0x00000001, 142, { -114, -263, 16 } },	// A 3 caras ranch868bit128
	{ "rancher", CAR_INDICATOR_RF, 1, { 168, 89, 12 }, 1, 0, 0x00000001, 123, { 86, 271, -41 } },	// A 2 caras ranch868bit128
	{ "rancher", CAR_INDICATOR_RF, 1, { 170, 90, 12 }, 1, 0, 0x00000001, 483, { 111, 257, 3 } },	// A 2 caras ranch868bit128
	{ "rancher", CAR_INDICATOR_RR, 1, { 170, 90, 12 }, 1, 0, 0x00000001, 142, { 114, -263, 16 } },	// A 3 caras ranch868bit128
	{ "regina", CAR_INDICATOR_LF, -1, { 181, 99, 22 }, 1, 0, 0x00000001, 93, { -96, 247, 3 } },	// A 2 caras regina868bit128
	{ "regina", CAR_INDICATOR_RF, 1, { 181, 99, 22 }, 1, 0, 0x00000001, 93, { 96, 247, 3 } },	// A 2 caras regina868bit128
	{ "romero", CAR_INDICATOR_LF, -1, { 153, 80, 28 }, 1, 0, 0x00000001, 240, { -94, 345, -9 } },	// A 3 caras romero8bit128
	{ "romero", CAR_INDICATOR_RF, 1, { 153, 80, 28 }, 1, 0, 0x00000001, 240, { 94, 345, -9 } },	// A 3 caras romero8bit128
	{ "rumpo", CAR_INDICATOR_LF, -1, { 126, 61, 18 }, 1, 1, 0x00000001, 26, { -96, 255, -24 } },	// A 1 caras rumpo868bit128
	{ "rumpo", CAR_INDICATOR_RF, 1, { 126, 61, 18 }, 1, 1, 0x00000001, 26, { 96, 255, -24 } },	// A 1 caras rumpo868bit128
	{ "sabre", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 80, { -88, 243, -35 } },	// A 4 caras lights [atlas del mod]
	{ "sabre", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 93, { -86, -277, -16 } },	// A 2 caras lights [atlas del mod]
	{ "sabre", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 58, { -86, -273, -33 } },	// A 2 caras lights [atlas del mod]
	{ "sabre", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 80, { 88, 243, -35 } },	// A 4 caras lights [atlas del mod]
	{ "sabre", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 93, { 86, -277, -16 } },	// A 2 caras lights [atlas del mod]
	{ "sabre", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 58, { 86, -273, -33 } },	// A 2 caras lights [atlas del mod]
	{ "sabretur", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 80, { -88, 243, -35 } },	// A 4 caras lights [atlas del mod]
	{ "sabretur", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 93, { -86, -277, -16 } },	// A 2 caras lights [atlas del mod]
	{ "sabretur", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 58, { -86, -273, -33 } },	// A 2 caras lights [atlas del mod]
	{ "sabretur", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 80, { 88, 243, -35 } },	// A 4 caras lights [atlas del mod]
	{ "sabretur", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 93, { 86, -277, -16 } },	// A 2 caras lights [atlas del mod]
	{ "sabretur", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 58, { 86, -273, -33 } },	// A 2 caras lights [atlas del mod]
	{ "sandking", CAR_INDICATOR_LF, -1, { 174, 89, 7 }, 1, 0, 0x00000001, 115, { -83, 254, -52 } },	// A 2 caras sandking868bit128
	{ "sandking", CAR_INDICATOR_LF, -1, { 178, 91, 7 }, 1, 0, 0x00000001, 263, { -112, 238, -2 } },	// A 2 caras sandking868bit128
	{ "sandking", CAR_INDICATOR_LF, -1, { 199, 99, 5 }, 0, 0, 0x00000001, 42, { -116, 234, -2 } },	// o 2 caras sandking868bit128
	{ "sandking", CAR_INDICATOR_RF, 1, { 174, 89, 7 }, 1, 0, 0x00000001, 115, { 83, 254, -52 } },	// A 2 caras sandking868bit128
	{ "sandking", CAR_INDICATOR_RF, 1, { 178, 91, 7 }, 1, 0, 0x00000001, 263, { 112, 238, -2 } },	// A 2 caras sandking868bit128
	{ "sandking", CAR_INDICATOR_RF, 1, { 199, 99, 5 }, 0, 0, 0x00000001, 42, { 115, 234, -2 } },	// o 2 caras sandking868bit128
	{ "securica", CAR_INDICATOR_LF, -1, { 168, 92, 29 }, 1, 0, 0x00000001, 122, { -111, 170, 25 } },	// A 2 caras securica868bit128
	{ "securica", CAR_INDICATOR_LR, -1, { 168, 92, 29 }, 1, 0, 0x00000001, 60, { -92, -269, 118 } },	// A 2 caras securica868bit128
	{ "securica", CAR_INDICATOR_LR, -1, { 168, 92, 29 }, 1, 0, 0x00000001, 54, { -82, -322, 107 } },	// A 5 caras securica868bit128
	{ "securica", CAR_INDICATOR_RF, 1, { 168, 92, 29 }, 1, 0, 0x00000001, 122, { 111, 170, 25 } },	// A 2 caras securica868bit128
	{ "securica", CAR_INDICATOR_RR, 1, { 168, 92, 29 }, 1, 0, 0x00000001, 60, { 92, -269, 118 } },	// A 2 caras securica868bit128
	{ "securica", CAR_INDICATOR_RR, 1, { 168, 92, 29 }, 1, 0, 0x00000001, 54, { 82, -322, 107 } },	// A 5 caras securica868bit128
	{ "sentinel", CAR_INDICATOR_LF, -1, { 151, 86, 25 }, 1, 0, 0x00000001, 455, { -92, 226, -4 } },	// A 5 caras sentinel868bit128
	{ "sentinel", CAR_INDICATOR_RF, 1, { 151, 86, 25 }, 1, 0, 0x00000001, 455, { 92, 226, -4 } },	// A 5 caras sentinel868bit128
	{ "sentxs", CAR_INDICATOR_LF, -1, { 151, 86, 25 }, 1, 0, 0x00000001, 455, { -92, 226, -4 } },	// A 5 caras sentxs868bit128
	{ "sentxs", CAR_INDICATOR_RF, 1, { 151, 86, 25 }, 1, 0, 0x00000001, 455, { 92, 226, -4 } },	// A 5 caras sentxs868bit128
	{ "spand", CAR_INDICATOR_LF, -1, { 152, 77, 21 }, 1, 0, 0x00000001, 112, { -73, 275, -6 } },	// A 1 caras spand8bit128
	{ "spand", CAR_INDICATOR_RF, 1, { 136, 70, 20 }, 1, 0, 0x00000001, 110, { 73, 275, -6 } },	// A 1 caras spand8bit128
	{ "stinger", CAR_INDICATOR_LF, -1, { 143, 84, 19 }, 1, 0, 0x00000001, 531, { -91, 239, -7 } },	// A 3 caras stinger86body64
	{ "stinger", CAR_INDICATOR_LF, -1, { 130, 75, 12 }, 1, 0, 0x00000001, 129, { -98, 222, -12 } },	// A 1 caras stinger86body64
	{ "stinger", CAR_INDICATOR_LR, -1, { 140, 82, 18 }, 1, 0, 0x00000001, 56, { -74, -255, 3 } },	// A 2 caras stinger86body64
	{ "stinger", CAR_INDICATOR_RF, 1, { 143, 84, 19 }, 1, 0, 0x00000001, 531, { 91, 239, -7 } },	// A 3 caras stinger86body64
	{ "stinger", CAR_INDICATOR_RF, 1, { 130, 75, 13 }, 1, 0, 0x00000001, 129, { 98, 222, -12 } },	// A 1 caras stinger86body64
	{ "stinger", CAR_INDICATOR_RR, 1, { 140, 82, 18 }, 1, 0, 0x00000001, 56, { 74, -255, 3 } },	// A 2 caras stinger86body64
	{ "streetfi", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 34, { -25, 64, 28 } },	// A 2 caras lights [atlas del mod]
	{ "streetfi", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 34, { -17, -97, 38 } },	// A 2 caras lights [atlas del mod]
	{ "streetfi", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 34, { 25, 64, 28 } },	// A 2 caras lights [atlas del mod]
	{ "streetfi", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 34, { 17, -97, 38 } },	// A 2 caras lights [atlas del mod]
	{ "stretch", CAR_INDICATOR_LF, -1, { 178, 104, 49 }, 1, 0, 0x00000001, 99, { -99, 341, -13 } },	// A 1 caras stretch8bit128
	{ "stretch", CAR_INDICATOR_RF, 1, { 178, 104, 49 }, 1, 0, 0x00000001, 99, { 99, 341, -13 } },	// A 1 caras stretch8bit128
	{ "taxi", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 151, { -50, -268, -7 } },	// A 2 caras lights [atlas del mod]
	{ "taxi", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 151, { 50, -268, -7 } },	// A 2 caras lights [atlas del mod]
	{ "taxi", CAR_INDICATOR_2_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 454, { -97, 226, -3 } },	// A 4 caras lights [atlas del mod]
	{ "taxi", CAR_INDICATOR_2_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 454, { 97, 226, -3 } },	// A 4 caras lights [atlas del mod]
	{ "topfun", CAR_INDICATOR_LF, -1, { 123, 59, 22 }, 1, 1, 0x00000001, 34, { -97, 258, -24 } },	// A 1 caras topfun868bit128 [cara completa 58.1]
	{ "topfun", CAR_INDICATOR_RF, 1, { 123, 59, 22 }, 1, 1, 0x00000001, 33, { 98, 258, -24 } },	// A 1 caras topfun868bit128 [cara completa 58.1]
	{ "trash", CAR_INDICATOR_LR, -1, { 172, 99, 33 }, 1, 0, 0x00000001, 13, { -54, -307, 172 } },	// A 1 caras trash868bit128
	{ "trash", CAR_INDICATOR_RR, 1, { 172, 99, 33 }, 1, 0, 0x00000001, 13, { 54, -307, 172 } },	// A 1 caras trash868bit128
	{ "trash2", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 351, { -82, 378, -27 } },	// A 2 caras lights [atlas del mod]
	{ "trash2", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 264, { -85, -407, -52 } },	// A 2 caras lights [atlas del mod]
	{ "trash2", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 350, { 83, 379, -27 } },	// A 2 caras lights [atlas del mod]
	{ "trash2", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 264, { 85, -407, -52 } },	// A 2 caras lights [atlas del mod]
	{ "vicechee", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 163, { -88, 252, -24 } },	// A 2 caras lights [atlas del mod]
	{ "vicechee", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 142, { -77, -236, 4 } },	// A 16 caras lights [atlas del mod]
	{ "vicechee", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 163, { 88, 252, -24 } },	// A 2 caras lights [atlas del mod]
	{ "vicechee", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 142, { 77, -236, 4 } },	// A 16 caras lights [atlas del mod]
	{ "virgo", CAR_INDICATOR_LF, -1, { 165, 90, 51 }, 1, 0, 0x00000001, 49, { -91, 251, -11 } },	// A 1 caras virgo8bit128
	{ "virgo", CAR_INDICATOR_RF, 1, { 165, 90, 51 }, 1, 0, 0x00000001, 49, { 91, 251, -11 } },	// A 1 caras virgo8bit128
	{ "washing", CAR_INDICATOR_LF, -1, { 150, 79, 23 }, 1, 0, 0x00000001, 141, { -87, 241, -17 } },	// A 3 caras washington8bit128
	{ "washing", CAR_INDICATOR_LF, -1, { 149, 78, 21 }, 1, 0, 0x00000001, 87, { -94, 231, -12 } },	// A 1 caras washington8bit128
	{ "washing", CAR_INDICATOR_LF, -1, { 162, 91, 33 }, 0, 0, 0x00000001, 71, { -92, 236, -16 } },	// o 2 caras washington8bit128
	{ "washing", CAR_INDICATOR_LR, -1, { 172, 69, 25 }, 1, 0, 0x00000001, 44, { -89, -295, -29 } },	// A 3 caras washington8bit128
	{ "washing", CAR_INDICATOR_LR, -1, { 182, 72, 24 }, 1, 0, 0x00000001, 116, { -89, -293, -23 } },	// A 3 caras washington8bit128
	{ "washing", CAR_INDICATOR_LR, -1, { 175, 72, 24 }, 1, 0, 0x00000001, 10, { -95, -290, -28 } },	// A 1 caras washington8bit128
	{ "washing", CAR_INDICATOR_RF, 1, { 150, 79, 23 }, 1, 0, 0x00000001, 141, { 87, 241, -17 } },	// A 3 caras washington8bit128
	{ "washing", CAR_INDICATOR_RF, 1, { 149, 78, 21 }, 1, 0, 0x00000001, 87, { 94, 231, -12 } },	// A 1 caras washington8bit128
	{ "washing", CAR_INDICATOR_RF, 1, { 162, 91, 33 }, 0, 0, 0x00000001, 71, { 92, 236, -16 } },	// o 2 caras washington8bit128
	{ "washing", CAR_INDICATOR_RR, 1, { 172, 69, 25 }, 1, 0, 0x00000001, 44, { 89, -295, -29 } },	// A 3 caras washington8bit128
	{ "washing", CAR_INDICATOR_RR, 1, { 182, 72, 24 }, 1, 0, 0x00000001, 116, { 89, -293, -23 } },	// A 3 caras washington8bit128
	{ "washing", CAR_INDICATOR_RR, 1, { 175, 72, 24 }, 1, 0, 0x00000001, 10, { 95, -290, -28 } },	// A 1 caras washington8bit128
	{ "wintergreen", CAR_INDICATOR_LF, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 56, { -22, 90, 38 } },	// A 8 caras lights [atlas del mod]
	{ "wintergreen", CAR_INDICATOR_LR, -1, { 253, 165, 0 }, 1, 1, 0x00000001, 56, { -22, -132, 2 } },	// A 8 caras lights [atlas del mod]
	{ "wintergreen", CAR_INDICATOR_RF, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 56, { 22, 90, 38 } },	// A 8 caras lights [atlas del mod]
	{ "wintergreen", CAR_INDICATOR_RR, 1, { 253, 165, 0 }, 1, 1, 0x00000001, 56, { 22, -132, 2 } },	// A 8 caras lights [atlas del mod]
	{ "yankee", CAR_INDICATOR_LR, -1, { 186, 91, 18 }, 1, 0, 0x00000001, 237, { -124, -457, -64 } },	// A 10 caras yankee868bit128
	{ "yankee", CAR_INDICATOR_RR, 1, { 186, 91, 18 }, 1, 0, 0x00000001, 237, { 124, -457, -64 } },	// A 10 caras yankee868bit128
	{ "zebra", CAR_INDICATOR_LF, -1, { 139, 73, 14 }, 1, 0, 0x00000001, 70, { -113, 239, -27 } },	// A 3 caras cabbie8bit128
	{ "zebra", CAR_INDICATOR_LF, -1, { 162, 94, 22 }, 1, 0, 0x00000001, 36, { -113, 241, -27 } },	// A 1 caras cabbie8bit128
	{ "zebra", CAR_INDICATOR_LR, -1, { 139, 73, 14 }, 1, 0, 0x00000001, 72, { -111, -247, -27 } },	// A 3 caras cabbie8bit128
	{ "zebra", CAR_INDICATOR_RF, 1, { 138, 72, 14 }, 1, 0, 0x00000001, 70, { 113, 239, -27 } },	// A 3 caras cabbie8bit128
	{ "zebra", CAR_INDICATOR_RF, 1, { 163, 94, 22 }, 1, 0, 0x00000001, 36, { 113, 241, -27 } },	// A 1 caras cabbie8bit128
	{ "zebra", CAR_INDICATOR_RR, 1, { 139, 73, 14 }, 1, 0, 0x00000001, 72, { 111, -247, -27 } },	// A 3 caras cabbie8bit128
};
