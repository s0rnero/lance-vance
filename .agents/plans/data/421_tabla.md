## Tabla objetivo (estado final de los 115 vehiculos servidos)

Regenerada de la medicion (`tools/indicator_audit.py` con la regla de hue,
arboles `tmp/base421` y `streamed/models/gta3.img`; datos en
`.agents/plans/data/421_clasificacion.csv`).

Como leer cada celda: `OBJETIVO[lo que hay hoy en el nodo servido](accion)`.

- Objetivos: `AMBAR` lente ambar real · `PROPIO` parpadea en su color
  (faro blanco delante, piloto rojo detras) · `COMO-MOD` lo publica el mod y
  no se toca · `SIN` no hay nada que parpadear ahi · `-` barco/avion/heli.
- Acciones: `nada` no tocar · `NODO-AMBAR`/`NODO-PROPIO` escribir el nodo con
  las caras que dicta la regla (Vía A/Vía B) · `TRIGGER` sin nodo: lo resuelve
  el disparador del motor (paso 3) · `BORRA-NODO` quitar el nodo inventado ·
  `RESTAURA-MOD` devolver el nodo del mod que el servido deformo.
- El corchete es lo que hay hoy en lo servido: `NaRrBbOo/tex` = caras
  ambar/rojo/blanco/otras y textura dominante del nodo (`lights` = la lente
  genérica del mod, `body` = textura de carrocería); `sin nodo` si no existe.
- `base` es de donde sale el modelo: `mod` = lo publica el vehicles.img o el
  newVehicles.img del Vice Extended; `octubre` = el gta3.img de octubre.

| vehiculo | tipo | base | lf | rf | lr | rr |
|---|---|---|---|---|---|---|
| admiral | car | mod | COMO-MOD[0a0r0b8o/lights](nada) | COMO-MOD[0a0r0b8o/lights](nada) | COMO-MOD[0a0r0b4o/lights](nada) | COMO-MOD[0a0r0b4o/lights](nada) |
| airtrain | plane | octubre | - | - | - | - |
| ambulan | car | mod | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) | PROPIO[sin nodo](TRIGGER) | PROPIO[sin nodo](TRIGGER) |
| angel | bike | mod | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) |
| baggage | car | mod | COMO-MOD[0a4r0b2o/lights](nada) | COMO-MOD[0a4r0b2o/lights](nada) | COMO-MOD[0a4r0b2o/body](nada) | COMO-MOD[0a4r0b2o/body](nada) |
| banshee | car | mod | PROPIO[0a0r0b4o/body](TRIGGER) | PROPIO[0a0r0b4o/body](TRIGGER) | PROPIO[0a0r0b6o/body](TRIGGER) | PROPIO[0a0r0b6o/body](TRIGGER) |
| barracks | car | octubre | PROPIO[0a0r1b0o/body](NODO-PROPIO) | PROPIO[0a0r1b0o/body](NODO-PROPIO) | AMBAR[5a0r0b0o/body](NODO-AMBAR) | AMBAR[5a0r0b0o/body](NODO-AMBAR) |
| benson | car | octubre | AMBAR[2a0r0b0o/body](NODO-AMBAR) | AMBAR[2a0r0b0o/body](NODO-AMBAR) | PROPIO[0a2r0b0o/body](NODO-PROPIO) | PROPIO[0a2r0b0o/body](NODO-PROPIO) |
| bfinject | car | octubre | PROPIO[0a0r4b4o/body](NODO-PROPIO) | PROPIO[0a0r4b4o/body](NODO-PROPIO) | PROPIO[0a8r0b0o/body](NODO-PROPIO) | PROPIO[0a8r0b0o/body](NODO-PROPIO) |
| blistac | car | mod | AMBAR[sin nodo](NODO-AMBAR) | AMBAR[sin nodo](NODO-AMBAR) | SIN[0a1r1b4o/body](BORRA-NODO) | SIN[0a1r1b4o/body](BORRA-NODO) |
| bloodra | car | octubre | PROPIO[0a0r3b0o/body](NODO-PROPIO) | PROPIO[0a0r3b0o/body](NODO-PROPIO) | PROPIO[0a1r0b1o/body](NODO-PROPIO) | PROPIO[0a1r0b1o/body](NODO-PROPIO) |
| bloodrb | car | octubre | PROPIO[0a0r5b8o/body](NODO-PROPIO) | PROPIO[0a0r5b8o/body](NODO-PROPIO) | PROPIO[0a1r0b0o/body](NODO-PROPIO) | PROPIO[0a1r0b0o/body](NODO-PROPIO) |
| bobcat | car | octubre | AMBAR[4a0r0b0o/body](NODO-AMBAR) | AMBAR[5a0r0b0o/body](NODO-AMBAR) | PROPIO[0a5r0b0o/body](NODO-PROPIO) | PROPIO[0a5r0b0o/body](NODO-PROPIO) |
| boxville | car | octubre | AMBAR[1a0r0b0o/body](NODO-AMBAR) | AMBAR[1a0r0b0o/body](NODO-AMBAR) | AMBAR[2a0r0b0o/body](NODO-AMBAR) | AMBAR[2a0r0b0o/body](NODO-AMBAR) |
| burrito | car | octubre | AMBAR[8a0r0b0o/body](NODO-AMBAR) | AMBAR[8a0r0b0o/body](NODO-AMBAR) | PROPIO[sin nodo](NODO-PROPIO) | PROPIO[sin nodo](NODO-PROPIO) |
| bus | car | octubre | AMBAR[5a0r0b0o/body](NODO-AMBAR) | AMBAR[5a0r0b0o/body](NODO-AMBAR) | PROPIO[0a1r0b1o/body](NODO-PROPIO) | PROPIO[0a1r0b1o/body](NODO-PROPIO) |
| cabbie | car | octubre | AMBAR[5a0r0b0o/body](NODO-AMBAR) | AMBAR[5a0r0b0o/body](NODO-AMBAR) | AMBAR[6a1r0b0o/body](NODO-AMBAR) | AMBAR[6a1r0b0o/body](NODO-AMBAR) |
| caddy | car | octubre | PROPIO[sin nodo](NODO-PROPIO) | PROPIO[sin nodo](NODO-PROPIO) | PROPIO[0a2r0b0o/body](NODO-PROPIO) | PROPIO[0a2r0b0o/body](NODO-PROPIO) |
| cheetah | car | mod | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b24o/lights](RESTAURA-MOD) | COMO-MOD[0a0r0b24o/lights](RESTAURA-MOD) |
| chopper | heli | octubre | - | - | - | - |
| coach | car | octubre | AMBAR[2a0r0b0o/body](NODO-AMBAR) | SIN[1a1r0b0o/body](BORRA-NODO) | nada | nada |
| coastg | boat | octubre | - | - | - | - |
| comet | car | mod | nada | nada | PROPIO[0a3r0b5o/body](NODO-PROPIO) | PROPIO[0a2r0b6o/body](NODO-PROPIO) |
| cuban | car | octubre | PROPIO[1a3r0b0o/body](NODO-PROPIO) | PROPIO[1a3r0b0o/body](NODO-PROPIO) | PROPIO[0a2r0b0o/body](NODO-PROPIO) | PROPIO[0a2r0b0o/body](NODO-PROPIO) |
| deaddodo | plane | octubre | - | - | - | - |
| deluxo | car | mod | PROPIO[0a0r2b0o/body](NODO-PROPIO) | PROPIO[0a0r1b0o/body](NODO-PROPIO) | PROPIO[0a8r0b0o/body](NODO-PROPIO) | PROPIO[0a7r0b0o/body](NODO-PROPIO) |
| dinghy | boat | octubre | - | - | - | - |
| enforcer | car | octubre | AMBAR[8a0r0b0o/body](NODO-AMBAR) | AMBAR[8a0r0b0o/body](NODO-AMBAR) | AMBAR[0a2r0b2o/body](NODO-AMBAR) | AMBAR[0a2r0b2o/body](NODO-AMBAR) |
| esperant | car | octubre | AMBAR[0a0r2b0o/body](NODO-AMBAR) | AMBAR[0a0r2b0o/body](NODO-AMBAR) | SIN[0a18r0b2o/body](BORRA-NODO) | SIN[0a11r0b1o/body](BORRA-NODO) |
| faggio | bike | octubre | nada | nada | AMBAR[6a0r0b0o/body](NODO-AMBAR) | AMBAR[6a0r0b0o/body](NODO-AMBAR) |
| fbicar | car | mod | PROPIO[0a0r2b0o/body](NODO-PROPIO) | PROPIO[0a0r2b0o/body](NODO-PROPIO) | nada | nada |
| fbiranch | car | mod | PROPIO[0a0r2b0o/body](NODO-PROPIO) | PROPIO[0a0r2b0o/body](NODO-PROPIO) | nada | nada |
| firetruk | car | mod | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) |
| flatbed | car | octubre | PROPIO[0a0r2b0o/body](NODO-PROPIO) | PROPIO[0a0r2b0o/body](NODO-PROPIO) | AMBAR[3a0r0b0o/body](NODO-AMBAR) | AMBAR[3a0r0b0o/body](NODO-AMBAR) |
| freeway | bike | mod | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) |
| gangbur | car | octubre | AMBAR[4a0r0b0o/body](NODO-AMBAR) | AMBAR[4a0r0b0o/body](NODO-AMBAR) | PROPIO[0a2r0b0o/body](NODO-PROPIO) | PROPIO[0a2r0b0o/body](NODO-PROPIO) |
| glendale | car | octubre | PROPIO[sin nodo](NODO-PROPIO) | PROPIO[sin nodo](NODO-PROPIO) | PROPIO[0a5r0b1o/body](NODO-PROPIO) | PROPIO[0a5r0b1o/body](NODO-PROPIO) |
| greenwoo | car | mod | SIN[0a0r0b3o/body](BORRA-NODO) | SIN[0a0r0b3o/body](BORRA-NODO) | SIN[0a2r0b4o/body](BORRA-NODO) | SIN[0a2r0b4o/body](BORRA-NODO) |
| hellenbach | car | mod | COMO-MOD[0a0r0b6o/lights](nada) | COMO-MOD[0a0r0b6o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) |
| hermes | car | octubre | PROPIO[sin nodo](NODO-PROPIO) | PROPIO[sin nodo](NODO-PROPIO) | PROPIO[0a21r0b0o/body](NODO-PROPIO) | PROPIO[0a21r0b0o/body](NODO-PROPIO) |
| hotrina | car | octubre | PROPIO[1a0r0b0o/body](NODO-PROPIO) | PROPIO[1a0r0b0o/body](NODO-PROPIO) | PROPIO[0a4r0b0o/body](NODO-PROPIO) | PROPIO[0a4r0b0o/body](NODO-PROPIO) |
| hotrinb | car | octubre | AMBAR[2a0r0b0o/body](NODO-AMBAR) | AMBAR[2a0r0b0o/body](NODO-AMBAR) | PROPIO[0a4r0b0o/body](NODO-PROPIO) | PROPIO[0a4r0b0o/body](NODO-PROPIO) |
| hotring | car | octubre | PROPIO[1a0r0b0o/body](NODO-PROPIO) | PROPIO[1a0r0b0o/body](NODO-PROPIO) | PROPIO[0a6r0b0o/body](NODO-PROPIO) | PROPIO[0a6r0b0o/body](NODO-PROPIO) |
| hunter | car | octubre | nada | nada | nada | nada |
| idaho | car | mod | COMO-MOD[0a0r0b4o/lights](nada) | COMO-MOD[0a0r0b4o/lights](nada) | PROPIO[sin nodo](TRIGGER) | PROPIO[sin nodo](TRIGGER) |
| infernus | car | mod | COMO-MOD[2a0r0b0o/body](nada) | COMO-MOD[2a0r0b0o/body](nada) | COMO-MOD[1a0r0b15o/lights](nada) | COMO-MOD[0a0r0b16o/lights](nada) |
| jetmax | boat | octubre | - | - | - | - |
| kaufman | car | octubre | AMBAR[5a0r0b0o/body](NODO-AMBAR) | AMBAR[5a0r0b0o/body](NODO-AMBAR) | AMBAR[6a0r0b0o/body](NODO-AMBAR) | AMBAR[6a0r0b0o/body](NODO-AMBAR) |
| landstal | car | octubre | AMBAR[3a0r0b0o/body](NODO-AMBAR) | AMBAR[3a0r0b0o/body](NODO-AMBAR) | PROPIO[0a4r0b0o/body](NODO-PROPIO) | PROPIO[0a4r0b0o/body](NODO-PROPIO) |
| linerun | car | octubre | AMBAR[6a0r0b0o/body](NODO-AMBAR) | AMBAR[6a0r0b0o/body](NODO-AMBAR) | PROPIO[1a2r0b0o/body](NODO-PROPIO) | PROPIO[1a2r0b0o/body](NODO-PROPIO) |
| lovefist | car | octubre | AMBAR[2a0r0b0o/body](NODO-AMBAR) | AMBAR[2a0r0b0o/body](NODO-AMBAR) | PROPIO[0a5r0b0o/body](NODO-PROPIO) | PROPIO[0a5r0b0o/body](NODO-PROPIO) |
| manana | car | octubre | AMBAR[3a0r0b0o/body](NODO-AMBAR) | AMBAR[3a0r0b0o/body](NODO-AMBAR) | AMBAR[2a0r0b0o/body](NODO-AMBAR) | AMBAR[2a0r0b0o/body](NODO-AMBAR) |
| manchez | bike | mod | PROPIO[sin nodo](TRIGGER) | PROPIO[sin nodo](TRIGGER) | PROPIO[sin nodo](TRIGGER) | PROPIO[sin nodo](TRIGGER) |
| marquis | boat | octubre | - | - | - | - |
| maverick | car | octubre | nada | nada | nada | nada |
| mesa | car | octubre | AMBAR[2a0r0b0o/body](NODO-AMBAR) | AMBAR[2a0r0b0o/body](NODO-AMBAR) | PROPIO[0a7r0b0o/body](NODO-PROPIO) | PROPIO[0a7r0b0o/body](NODO-PROPIO) |
| moonbeam | car | octubre | AMBAR[2a0r0b2o/body](NODO-AMBAR) | AMBAR[2a0r0b2o/body](NODO-AMBAR) | AMBAR[0a2r0b0o/body](NODO-AMBAR) | AMBAR[0a2r0b0o/body](NODO-AMBAR) |
| mrwhoop | car | octubre | PROPIO[1a0r0b0o/body](NODO-PROPIO) | PROPIO[1a0r0b0o/body](NODO-PROPIO) | AMBAR[2a0r0b0o/body](NODO-AMBAR) | AMBAR[2a0r0b0o/body](NODO-AMBAR) |
| mule | car | octubre | PROPIO[0a0r1b0o/body](NODO-PROPIO) | PROPIO[0a0r1b0o/body](NODO-PROPIO) | PROPIO[0a2r0b0o/body](NODO-PROPIO) | PROPIO[0a2r0b0o/body](NODO-PROPIO) |
| oceanic | car | octubre | PROPIO[sin nodo](NODO-PROPIO) | PROPIO[sin nodo](NODO-PROPIO) | PROPIO[0a12r0b8o/body](NODO-PROPIO) | PROPIO[0a12r0b8o/body](NODO-PROPIO) |
| packer | car | octubre | AMBAR[7a0r0b0o/body](NODO-AMBAR) | AMBAR[7a0r0b0o/body](NODO-AMBAR) | AMBAR[4a4r0b0o/body](NODO-AMBAR) | AMBAR[4a4r0b0o/body](NODO-AMBAR) |
| patriot | car | mod | COMO-MOD[0a0r0b34o/lights](nada) | COMO-MOD[0a0r0b34o/lights](nada) | COMO-MOD[0a0r0b34o/lights](nada) | COMO-MOD[0a0r0b34o/lights](nada) |
| pcj600 | bike | mod | PROPIO[sin nodo](TRIGGER) | PROPIO[sin nodo](TRIGGER) | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) |
| peren | car | octubre | AMBAR[2a0r0b0o/body](NODO-AMBAR) | AMBAR[2a0r0b0o/body](NODO-AMBAR) | PROPIO[0a1r0b0o/body](NODO-PROPIO) | PROPIO[0a1r0b0o/body](NODO-PROPIO) |
| peren2 | car | mod | PROPIO[sin nodo](TRIGGER) | PROPIO[sin nodo](TRIGGER) | COMO-MOD[0a0r0b4o/lights](nada) | COMO-MOD[0a0r0b4o/lights](nada) |
| pheonix | car | mod | PROPIO[sin nodo](NODO-PROPIO) | PROPIO[sin nodo](NODO-PROPIO) | PROPIO[0a14r0b2o/body](NODO-PROPIO) | PROPIO[0a14r0b2o/body](NODO-PROPIO) |
| pizzaboy | bike | octubre | nada | nada | AMBAR[sin nodo](NODO-AMBAR) | AMBAR[sin nodo](NODO-AMBAR) |
| police | car | mod | COMO-MOD[0a0r0b4o/lights](nada) | COMO-MOD[0a0r0b4o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) |
| polmav | car | octubre | nada | nada | nada | nada |
| polwintergreen | bike | mod | COMO-MOD[0a0r0b8o/lights](nada) | COMO-MOD[0a0r0b8o/lights](nada) | COMO-MOD[0a0r0b8o/lights](nada) | COMO-MOD[0a0r0b8o/lights](nada) |
| pony | car | octubre | PROPIO[1a0r0b0o/body](NODO-PROPIO) | PROPIO[1a0r0b0o/body](NODO-PROPIO) | PROPIO[0a2r0b0o/body](NODO-PROPIO) | PROPIO[0a2r0b0o/body](NODO-PROPIO) |
| predator | boat | octubre | - | - | - | - |
| premier | car | mod | COMO-MOD[0a0r0b4o/lights](nada) | COMO-MOD[0a0r0b4o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) |
| rancher | car | mod | AMBAR[2a0r0b0o/body](NODO-AMBAR) | AMBAR[2a0r0b0o/body](NODO-AMBAR) | AMBAR[3a0r0b0o/body](NODO-AMBAR) | AMBAR[3a0r0b0o/body](NODO-AMBAR) |
| rcbandit | car | octubre | nada | nada | nada | nada |
| rcbaron | car | octubre | nada | nada | nada | nada |
| rcgoblin | car | octubre | nada | nada | PROPIO[sin nodo](NODO-PROPIO) | PROPIO[sin nodo](NODO-PROPIO) |
| rcraider | car | octubre | nada | nada | nada | nada |
| reefer | boat | octubre | - | - | - | - |
| regina | car | octubre | AMBAR[2a0r0b0o/body](NODO-AMBAR) | AMBAR[2a0r0b0o/body](NODO-AMBAR) | PROPIO[sin nodo](NODO-PROPIO) | PROPIO[sin nodo](NODO-PROPIO) |
| rhino | car | mod | nada | nada | nada | nada |
| rio | boat | octubre | - | - | - | - |
| romero | car | octubre | PROPIO[1a0r0b1o/body](NODO-PROPIO) | PROPIO[1a0r0b1o/body](NODO-PROPIO) | PROPIO[0a2r0b0o/body](NODO-PROPIO) | PROPIO[0a2r0b0o/body](NODO-PROPIO) |
| rumpo | car | octubre | PROPIO[1a0r0b0o/body](NODO-PROPIO) | PROPIO[1a0r0b0o/body](NODO-PROPIO) | AMBAR[2a0r0b0o/body](NODO-AMBAR) | AMBAR[2a0r0b0o/body](NODO-AMBAR) |
| sabre | car | mod | COMO-MOD[0a1r0b3o/lights](nada) | COMO-MOD[0a1r0b3o/lights](nada) | COMO-MOD[0a0r0b4o/lights](nada) | COMO-MOD[0a0r0b4o/lights](nada) |
| sabretur | car | mod | COMO-MOD[0a1r0b3o/lights](nada) | COMO-MOD[0a1r0b3o/lights](nada) | COMO-MOD[0a0r0b4o/lights](nada) | COMO-MOD[0a0r0b4o/lights](nada) |
| sanchez | bike | octubre | nada | nada | nada | nada |
| sandking | car | octubre | AMBAR[3a0r0b0o/body](NODO-AMBAR) | AMBAR[3a0r0b0o/body](NODO-AMBAR) | PROPIO[0a3r0b0o/body](NODO-PROPIO) | PROPIO[0a3r0b0o/body](NODO-PROPIO) |
| seaspar | car | octubre | PROPIO[sin nodo](NODO-PROPIO) | PROPIO[sin nodo](NODO-PROPIO) | nada | nada |
| securica | car | octubre | AMBAR[2a0r0b0o/body](NODO-AMBAR) | AMBAR[2a0r0b0o/body](NODO-AMBAR) | AMBAR[6a1r0b0o/body](NODO-AMBAR) | AMBAR[6a1r0b0o/body](NODO-AMBAR) |
| sentinel | car | mod | AMBAR[5a0r0b2o/body](NODO-AMBAR) | AMBAR[5a0r0b2o/body](NODO-AMBAR) | SIN[0a0r0b1o/body](BORRA-NODO) | SIN[0a0r0b1o/body](BORRA-NODO) |
| sentxs | car | mod | AMBAR[5a0r0b2o/body](NODO-AMBAR) | AMBAR[5a0r0b2o/body](NODO-AMBAR) | SIN[0a0r0b1o/body](BORRA-NODO) | SIN[0a0r0b1o/body](BORRA-NODO) |
| skimmer | boat | octubre | - | - | - | - |
| spand | car | octubre | AMBAR[sin nodo](NODO-AMBAR) | AMBAR[sin nodo](NODO-AMBAR) | PROPIO[sin nodo](NODO-PROPIO) | PROPIO[sin nodo](NODO-PROPIO) |
| sparrow | car | octubre | PROPIO[sin nodo](NODO-PROPIO) | PROPIO[sin nodo](NODO-PROPIO) | nada | nada |
| speeder | boat | octubre | - | - | - | - |
| squalo | boat | octubre | - | - | - | - |
| stallion | car | octubre | PROPIO[0a0r6b0o/body](NODO-PROPIO) | PROPIO[0a0r6b0o/body](NODO-PROPIO) | PROPIO[0a7r0b0o/body](NODO-PROPIO) | PROPIO[0a7r0b0o/body](NODO-PROPIO) |
| stinger | car | mod | AMBAR[3a1r0b0o/body](NODO-AMBAR) | AMBAR[3a1r0b0o/body](NODO-AMBAR) | AMBAR[7a3r0b2o/body](NODO-AMBAR) | AMBAR[7a3r0b2o/body](NODO-AMBAR) |
| streetfi | bike | mod | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) |
| stretch | car | octubre | AMBAR[1a0r0b0o/body](NODO-AMBAR) | AMBAR[1a0r0b0o/body](NODO-AMBAR) | PROPIO[0a5r0b0o/body](NODO-PROPIO) | PROPIO[0a5r0b0o/body](NODO-PROPIO) |
| taxi | car | mod | COMO-MOD[0a0r0b4o/lights](nada) | COMO-MOD[0a0r0b4o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) |
| topfun | car | octubre | SIN[1a0r0b1o/body](BORRA-NODO) | SIN[1a0r0b1o/body](BORRA-NODO) | PROPIO[1a1r1b1o/body](NODO-PROPIO) | PROPIO[1a1r1b1o/body](NODO-PROPIO) |
| trash | car | octubre | PROPIO[2a0r1b0o/body](NODO-PROPIO) | PROPIO[2a0r1b0o/body](NODO-PROPIO) | AMBAR[3a3r0b2o/body](NODO-AMBAR) | AMBAR[2a3r0b3o/body](NODO-AMBAR) |
| trash2 | car | mod | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) |
| tropic | boat | octubre | - | - | - | - |
| vcnmav | car | octubre | nada | nada | nada | nada |
| vicechee | car | mod | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b2o/lights](nada) | COMO-MOD[0a0r0b24o/lights](RESTAURA-MOD) | COMO-MOD[0a0r0b24o/lights](RESTAURA-MOD) |
| virgo | car | octubre | PROPIO[0a0r0b1o/body](NODO-PROPIO) | PROPIO[0a0r0b1o/body](NODO-PROPIO) | PROPIO[0a10r0b0o/body](NODO-PROPIO) | PROPIO[0a10r0b0o/body](NODO-PROPIO) |
| voodoo | car | octubre | PROPIO[0a0r4b0o/body](NODO-PROPIO) | PROPIO[0a0r4b0o/body](NODO-PROPIO) | PROPIO[0a12r0b0o/body](NODO-PROPIO) | PROPIO[0a12r0b0o/body](NODO-PROPIO) |
| walton | car | octubre | SIN[0a0r19b2o/body](BORRA-NODO) | SIN[0a0r19b2o/body](BORRA-NODO) | PROPIO[0a1r0b0o/body](NODO-PROPIO) | PROPIO[0a1r0b0o/body](NODO-PROPIO) |
| washing | car | octubre | AMBAR[0a0r0b4o/body](NODO-AMBAR) | AMBAR[0a0r0b4o/body](NODO-AMBAR) | PROPIO[sin nodo](NODO-PROPIO) | PROPIO[sin nodo](NODO-PROPIO) |
| wintergreen | bike | mod | COMO-MOD[0a0r0b8o/lights](nada) | COMO-MOD[0a0r0b8o/lights](nada) | COMO-MOD[0a0r0b8o/lights](nada) | COMO-MOD[0a0r0b8o/lights](nada) |
| yankee | car | octubre | PROPIO[0a0r1b2o/body](NODO-PROPIO) | PROPIO[0a0r1b2o/body](NODO-PROPIO) | AMBAR[2a0r0b0o/body](NODO-AMBAR) | AMBAR[2a0r0b0o/body](NODO-AMBAR) |
| zebra | car | octubre | AMBAR[5a0r0b0o/body](NODO-AMBAR) | AMBAR[6a0r0b0o/body](NODO-AMBAR) | AMBAR[6a1r0b0o/body](NODO-AMBAR) | AMBAR[6a1r0b0o/body](NODO-AMBAR) |

### Resumen de acciones (460 celdas = 115 modelos x 4 extremos)

- `nada`: 134 celdas
- `NODO-AMBAR`: 101 celdas
- `NODO-PROPIO`: 132 celdas
- `TRIGGER`: 16 celdas
- `BORRA-NODO`: 17 celdas
- `RESTAURA-MOD`: 4 celdas

Desglose por estado de lo servido: -/- 56 · BORRA-NODO/borra 17 · NODO-AMBAR/crea 6 · NODO-AMBAR/distinto 52 · NODO-AMBAR/igual 43 · NODO-PROPIO/crea 24 · NODO-PROPIO/distinto 64 · NODO-PROPIO/igual 44 · RESTAURA-MOD/cambiado 4 · TRIGGER/borra 4 · TRIGGER/no hay 12 · nada/igual 80 · nada/no hay 54

- Barcos/aviones/helis fuera de alcance: **14** modelos, **56** celdas.
- Modelos con alguna accion (motor o datos): **75** de 101 con faros.
- Celdas que cambian de verdad (no `nada`, no `-`, no `igual`): **171**.
- Celdas que exigen escribir un DFF: **171** en **60** modelos; el resto es trabajo de motor.
- Desglose de esas celdas: BORRA-NODO 17 · NODO-AMBAR 58 · NODO-PROPIO 88 · RESTAURA-MOD 4 · TRIGGER/borra 4.

Con nodo ambar real (40 modelos): barracks benson blistac bobcat boxville burrito bus cabbie coach enforcer esperant faggio flatbed gangbur hotrinb kaufman landstal linerun lovefist manana mesa moonbeam mrwhoop packer peren pizzaboy rancher regina rumpo sandking securica sentinel sentxs spand stinger stretch trash washing yankee zebra

Con nodo de color propio (48 modelos): barracks benson bfinject bloodra bloodrb bobcat burrito bus caddy comet cuban deluxo fbicar fbiranch flatbed gangbur glendale hermes hotrina hotrinb hotring landstal linerun lovefist mesa mrwhoop mule oceanic peren pheonix pony rcgoblin regina romero rumpo sandking seaspar spand sparrow stallion stretch topfun trash virgo voodoo walton washing yankee

Al disparador del motor (6 modelos): ambulan banshee idaho manchez pcj600 peren2

Con nodo que se borra sin sustituto (8 modelos): blistac coach esperant greenwoo sentinel sentxs topfun walton

