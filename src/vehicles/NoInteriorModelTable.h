// Generado por gta_vc_browser/tools/gen_no_interior.py -- no editar a mano.
// Vehiculos sin interior medidos sobre los DFF servidos: caras dentro del
// volumen de la cabina del conductor (delante del marco `ped_frontseat`):
//   |x-sx| <= 0,55   sy+0,15 <= y < sy+1,30   sz+0,10 <= z < sz+1,00
// Menos de 20 caras en ese volumen = no hay a traves de donde mirar en 1a
// persona. Modelos medidos: 112. El primero que SI tiene interior es coastg con
// 24 caras y la mediana de la flota medida es 113. No cae ningun modelo entre
// 11 y 24: ese hueco es lo que sostiene el umbral, no una cifra a mano.
// El motor solo consulta esta lista en vehiculos de tipo coche; las filas de
// moto/avion quedan aqui inertes (su camara ya es otra).
#pragma once

static const char *const s_odNoInteriorVehicles[] = {
	"manchez",	// 0 caras en la cabina
	"rcbandit",	// 0 caras en la cabina
	"sanchez",	// 0 caras en la cabina
	"coach",	// 6 caras en la cabina
	"rhino",	// 11 caras en la cabina
};
