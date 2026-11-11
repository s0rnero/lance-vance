// Generado por gta_vc_browser/tools/gen_body_lens.py -- no editar a mano.
// Lentes ambar medidas en la CARROCERIA (no en un nodo indicator_*): se
// encienden en su sitio con un atlas derivado <atlas>_lens, sin mover ni
// crear geometria ni tintar el material compartido (plan 58.0 T3/T4).
// Puerta: racimo pegado al ancla de faro/piloto (<= 0.50 m, mismo eje y
// costado) con >= 75 % de area ambar; mule delante y mrwhoop delante
// son excepciones parciales del jugador (43 %/69 %): solo encienden las
// texelas ambar bajo el footprint de su cara. area_cm2 = area ambar;
// xyz = centroide del sitio en cm; mats = mascara del material de la lente.
#pragma once

struct ViceExtBodyLens {
	const char *name;
	const char *frame;
	int8 node;
	int8 side;
	uint8 rgb[3];
	uint32 mats;
	uint16 area_cm2;
	int16 xyz[3];
	uint8 coverage;
};

static const ViceExtBodyLens s_odBodyLens[] = {
	{ "boxville", "chassis_hi", CAR_INDICATOR_LF, -1, { 184, 92, 17 }, 0x00000200, 171, { -88, 310, 23 } },	// 62.2 cristal 171 cm2 (2 tris, mats [9])
	{ "boxville", "chassis_hi", CAR_INDICATOR_RF, 1, { 184, 92, 17 }, 0x00000400, 171, { 88, 310, 23 } },	// 62.2 cristal 171 cm2 (2 tris, mats [10])
	{ "esperant", "chassis_hi", CAR_INDICATOR_LR, -1, { 135, 57, 18 }, 0x00000100, 357, { -92, -298, -3 } },	// 62.2 cristal 357 cm2 (8 tris, mats [8])
	{ "esperant", "chassis_hi", CAR_INDICATOR_RR, 1, { 135, 57, 18 }, 0x00000200, 178, { 92, -298, -3 } },	// 62.2 cristal 178 cm2 (4 tris, mats [9])
	{ "fbiranch", "bump_front_hi_ok", CAR_INDICATOR_LF, -1, { 172, 92, 16 }, 0x00000020, 163, { -86, 326, -41 } },	// 62.2 cristal 163 cm2 (2 tris, mats [5])
	{ "fbiranch", "bump_front_hi_ok", CAR_INDICATOR_RF, 1, { 172, 92, 16 }, 0x00000040, 163, { 86, 326, -41 } },	// 62.2 cristal 163 cm2 (2 tris, mats [6])
	{ "fbiranch", "chassis_hi", CAR_INDICATOR_LR, -1, { 172, 92, 16 }, 0x00000800, 177, { -113, -306, 20 } },	// 62.2 cristal 177 cm2 (4 tris, mats [11])
	{ "fbiranch", "chassis_hi", CAR_INDICATOR_RR, 1, { 172, 92, 16 }, 0x00001000, 178, { 113, -306, 20 } },	// 62.2 cristal 178 cm2 (4 tris, mats [12])
	{ "fbiranch", "wing_lf_hi_ok", CAR_INDICATOR_LF, -1, { 172, 92, 16 }, 0x00000020, 571, { -111, 311, 3 } },	// 62.2 cristal 571 cm2 (2 tris, mats [5])
	{ "fbiranch", "wing_rf_hi_ok", CAR_INDICATOR_RF, 1, { 172, 92, 16 }, 0x00000020, 571, { 111, 311, 3 } },	// 62.2 cristal 571 cm2 (2 tris, mats [5])
	{ "kaufman", "wing_lf_hi_ok", CAR_INDICATOR_LF, -1, { 167, 95, 3 }, 0x00000002, 184, { -112, 239, -28 } },	// 60.0 T4 direccional del ala delantera izquierda (mat1 cabbie8bit128)
	{ "kaufman", "wing_rf_hi_ok", CAR_INDICATOR_RF, 1, { 167, 95, 3 }, 0x00000002, 184, { 112, 239, -28 } },	// 60.0 T4 direccional del ala delantera derecha
	{ "kaufman", "chassis_hi", CAR_INDICATOR_LR, -1, { 167, 95, 3 }, 0x00000001, 187, { -111, -247, -28 } },	// 60.0 T4 direccional trasera izquierda (mat0 cabbie8bit128 de chassis_hi)
	{ "kaufman", "chassis_hi", CAR_INDICATOR_RR, 1, { 167, 95, 3 }, 0x00000001, 187, { 111, -247, -28 } },	// 60.0 T4 direccional trasera derecha
	{ "mrwhoop", "chassis_hi", CAR_INDICATOR_LF, -1, { 177, 90, 27 }, 0x00000800, 127, { -76, 223, 5 } },	// 62.2 cristal 127 cm2 (2 tris, mats [11])
	{ "mrwhoop", "chassis_hi", CAR_INDICATOR_RF, 1, { 177, 90, 27 }, 0x00001000, 127, { 76, 223, 5 } },	// 62.2 cristal 127 cm2 (2 tris, mats [12])
	{ "mule", "chassis_hi", CAR_INDICATOR_LF, -1, { 139, 73, 22 }, 0x00000040, 202, { -73, 278, 1 } },	// 44% ambar
	{ "mule", "chassis_hi", CAR_INDICATOR_RF, 1, { 139, 73, 22 }, 0x00000080, 202, { 73, 278, 1 } },	// 44% ambar
	{ "rumpo", "chassis_hi", CAR_INDICATOR_LF, -1, { 172, 92, 16 }, 0x00000280, 130, { -97, 254, -33 }, 1 },	// 62.2 cristal 130 cm2 (3 tris, mats [7, 9])
	{ "rumpo", "chassis_hi", CAR_INDICATOR_RF, 1, { 172, 92, 16 }, 0x00000500, 125, { 97, 254, -33 }, 1 },	// 62.2 cristal 125 cm2 (3 tris, mats [8, 10])
	{ "pony", "chassis_hi", CAR_INDICATOR_LF, -1, { 172, 92, 16 }, 0x00000280, 130, { -97, 255, -33 }, 1 },	// 62.2 cristal 130 cm2 (3 tris, mats [7, 9])
	{ "pony", "chassis_hi", CAR_INDICATOR_RF, 1, { 172, 92, 16 }, 0x00000500, 125, { 97, 255, -33 }, 1 },	// 62.2 cristal 125 cm2 (3 tris, mats [8, 10])
	{ "topfun", "chassis_hi", CAR_INDICATOR_LF, -1, { 172, 92, 16 }, 0x00000a00, 170, { -97, 257, -33 }, 1 },	// 62.2 cristal 170 cm2 (3 tris, mats [9, 11])
	{ "topfun", "chassis_hi", CAR_INDICATOR_RF, 1, { 172, 92, 16 }, 0x00001400, 165, { 98, 257, -33 }, 1 },	// 62.2 cristal 165 cm2 (3 tris, mats [10, 12])
	{ "zebra", "chassis_hi", CAR_INDICATOR_LR, -1, { 172, 92, 16 }, 0x00000040, 66, { -111, -251, -27 }, 1 },	// 59.0 cobertura: 2 tris
	{ "zebra", "chassis_hi", CAR_INDICATOR_RR, 1, { 172, 92, 16 }, 0x00000080, 66, { 111, -251, -27 }, 1 },	// 59.0 cobertura: 2 tris
	{ "cabbie", "chassis_hi", CAR_INDICATOR_LR, -1, { 172, 92, 16 }, 0x00000080, 66, { -111, -251, -27 }, 1 },	// 59.0 cobertura: 2 tris
	{ "cabbie", "chassis_hi", CAR_INDICATOR_RR, 1, { 172, 92, 16 }, 0x00000100, 66, { 111, -251, -27 }, 1 },	// 59.0 cobertura: 2 tris
	{ "trash", "chassis_hi", CAR_INDICATOR_LR, -1, { 172, 92, 16 }, 0x00000040, 1224, { -75, -373, 25 }, 1 },	// 59.0 cobertura: 3 tris
	{ "trash", "chassis_hi", CAR_INDICATOR_RR, 1, { 172, 92, 16 }, 0x00000080, 1356, { 78, -382, 5 }, 1 },	// 59.0 cobertura: 4 tris
	{ "trash", "chassis_vlo", CAR_INDICATOR_LR, -1, { 172, 92, 16 }, 0x00000008, 3571, { -29, -307, 171 }, 1 },	// 59.0 cobertura: 1 tris
	{ "trash", "wing_lf_hi_ok", CAR_INDICATOR_LF, -1, { 172, 92, 16 }, 0x00000003, 978, { -102, 490, -35 }, 1 },	// 59.0 cobertura: 6 tris
	{ "trash", "wing_rf_hi_ok", CAR_INDICATOR_RF, 1, { 172, 92, 16 }, 0x00000003, 978, { 102, 490, -35 }, 1 },	// 59.0 cobertura: 6 tris
	{ "esperant", "wing_lf_hi_ok", CAR_INDICATOR_LF, -1, { 172, 92, 16 }, 0x00000010, 81, { -102, 235, -21 }, 1 },	// 62.2 cristal 81 cm2 (4 tris, mats [4])
	{ "esperant", "wing_rf_hi_ok", CAR_INDICATOR_RF, 1, { 172, 92, 16 }, 0x00000010, 81, { 102, 235, -21 }, 1 },	// 62.2 cristal 81 cm2 (4 tris, mats [4])
	{ "mule", "chassis_hi", CAR_INDICATOR_LF, -1, { 172, 92, 16 }, 0x00000040, 462, { -73, 278, 1 }, 1 },	// 62.0 cristal 462 cm2
	{ "mule", "chassis_hi", CAR_INDICATOR_RF, 1, { 172, 92, 16 }, 0x00000080, 462, { 73, 278, 1 }, 1 },	// 62.0 cristal 462 cm2
	{ "enforcer", "chassis_hi", CAR_INDICATOR_LR, -1, { 172, 92, 16 }, 0x00000400, 219, { -96, -387, -19 }, 1 },	// 59.0 cobertura: 9 tris
	{ "enforcer", "chassis_hi", CAR_INDICATOR_RR, 1, { 172, 92, 16 }, 0x00000800, 219, { 96, -387, -17 }, 1 },	// 59.0 cobertura: 9 tris
	{ "securica", "chassis_hi", CAR_INDICATOR_LR, -1, { 172, 92, 16 }, 0x00000100, 173, { -87, -317, 107 }, 1 },	// 59.0 cobertura: 6 tris
	{ "securica", "chassis_hi", CAR_INDICATOR_RR, 1, { 172, 92, 16 }, 0x00000200, 173, { 87, -317, 107 }, 1 },	// 59.0 cobertura: 6 tris
	{ "spand", "chassis_hi", CAR_INDICATOR_LF, -1, { 172, 92, 16 }, 0x00000080, 515, { -69, 277, 13 }, 1 },	// 59.0 cobertura: 2 tris
	{ "spand", "chassis_hi", CAR_INDICATOR_LR, -1, { 172, 92, 16 }, 0x00000080, 257, { -111, -339, 31 }, 1 },	// 59.0 cobertura: 2 tris
	{ "spand", "chassis_hi", CAR_INDICATOR_RF, 1, { 172, 92, 16 }, 0x00000100, 515, { 69, 277, 13 }, 1 },	// 59.0 cobertura: 2 tris
	{ "spand", "chassis_hi", CAR_INDICATOR_RR, 1, { 172, 92, 16 }, 0x00000100, 257, { 111, -339, 31 }, 1 },	// 59.0 cobertura: 2 tris
	{ "spand", "chassis_vlo", CAR_INDICATOR_LF, -1, { 172, 92, 16 }, 0x00000010, 1397, { -61, 278, -7 }, 1 },	// 59.0 cobertura: 2 tris
	{ "spand", "chassis_vlo", CAR_INDICATOR_RF, 1, { 172, 92, 16 }, 0x00000020, 1326, { 62, 278, 0 }, 1 },	// 59.0 cobertura: 2 tris
	{ "manana", "chassis_hi", CAR_INDICATOR_LF, -1, { 172, 92, 16 }, 0x00000080, 26, { -88, 203, 4 }, 1 },	// 59.0 cobertura: 1 tris
	{ "manana", "chassis_hi", CAR_INDICATOR_LR, -1, { 172, 92, 16 }, 0x00000080, 80, { -84, -217, -3 }, 1 },	// 59.0 cobertura: 1 tris
	{ "manana", "chassis_hi", CAR_INDICATOR_RF, 1, { 172, 92, 16 }, 0x00000100, 26, { 88, 203, 4 }, 1 },	// 59.0 cobertura: 1 tris
	{ "manana", "chassis_hi", CAR_INDICATOR_RR, 1, { 172, 92, 16 }, 0x00000100, 80, { 84, -217, -3 }, 1 },	// 59.0 cobertura: 1 tris
	{ "rancher", "bump_front_hi_ok", CAR_INDICATOR_LF, -1, { 172, 92, 16 }, 0x00000008, 456, { -77, 276, -32 }, 1 },	// 59.0 cobertura: 1 tris
	{ "rancher", "bump_front_hi_ok", CAR_INDICATOR_RF, 1, { 172, 92, 16 }, 0x00000010, 456, { 77, 276, -32 }, 1 },	// 59.0 cobertura: 1 tris
	{ "rancher", "chassis_hi", CAR_INDICATOR_LR, -1, { 172, 92, 16 }, 0x00000080, 1628, { -112, -264, 2 }, 1 },	// 59.0 cobertura: 9 tris
	{ "rancher", "chassis_hi", CAR_INDICATOR_RR, 1, { 172, 92, 16 }, 0x00000100, 1627, { 112, -264, 2 }, 1 },	// 59.0 cobertura: 9 tris
	{ "fbicar", "chassis_hi", CAR_INDICATOR_LF, -1, { 153, 83, 26 }, 0x00000100, 408, { -90, 238, -16 }, 1 },	// 62.2 cristal 408 cm2 (6 tris, mats [8])
	{ "fbicar", "chassis_hi", CAR_INDICATOR_RF, 1, { 153, 83, 26 }, 0x00000400, 408, { 90, 238, -16 }, 1 },	// 62.2 cristal 408 cm2 (6 tris, mats [10])
	{ "fbicar", "chassis_hi", CAR_INDICATOR_LR, -1, { 179, 71, 24 }, 0x00000200, 228, { -90, -293, -26 }, 1 },	// 62.2 cristal 228 cm2 (7 tris, mats [9])
	{ "fbicar", "chassis_hi", CAR_INDICATOR_RR, 1, { 179, 71, 24 }, 0x00000800, 228, { 90, -293, -26 }, 1 },	// 62.2 cristal 228 cm2 (7 tris, mats [11])
};
