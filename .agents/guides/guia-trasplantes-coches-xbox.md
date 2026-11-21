# Guía: trasplante de coches Xbox sobre base PC

> Caso de referencia: **Admiral 60.7 / 60.9-T4** (2026-11-16). El jugador lo dio por
> bueno en juego. Esta guía recoge el método, las piezas que se portaron y las
> correcciones que hicieron falta para que los siguientes coches no repitan los
> mismos errores.

## 1. Regla de oro

Se conserva el **coche PC** como esqueleto y se le injertan **solo las piezas Xbox**
que el PC no tiene o que el jugador quiere del Xbox:

- Carrocería, luces, matrículas, intermitentes, volante, cristales originales y
  **atlas PC**: se quedan como están. Nunca se sustituye `admiral868bit128`.
- Interior (asientos, salpicadero, consola, moqueta), motor, bajos, fondo del vano y
  paneles interiores de puerta: vienen del Xbox, con su atlas propio
  (`admiral868bit256`, `limo`, `belowcars`).
- El resultado es un DFF/TXD de datos. **No hace falta tocar código** para el
  trasplante; el motor ya intercambia `_ok`/`_dam` y dibuja los atomics nuevos.

## 2. Antes de tocar: inventario del donante

1. Extraer el DFF/TXD del Xbox y el DFF/TXD PC del coche.
2. Volcar marcos, atomics, materiales, bboxes y recuentos de caras. Herramientas:
   - `python gta_vc_browser/tools/dff_geom.py <coche.dff>`
   - `python gta_vc_browser/tools/txd_inspect.py <coche.txd>`
   - scripts de apoyo en `gta_vc_browser/tmp/base60/` (ver sección 9).
3. Identificar cada pieza donante por **textura + franja de coordenadas**, no por
   nombre a ojo. En el Admiral:

| Pieza Xbox | Textura | Filtro / franja | Caras |
| --- | --- | --- | --- |
| Interior | `admiral868bit256` | y -2.3..1.5 | 812 |
| Motor | `limo` | delantero, y 1.1..2.42 | 894 |
| Bajos | `belowcars` | z -0.59..-0.49 | 159 |
| Fondo del vano | `admiral868bit256` | y 1.5..2.2, z<0 | 34 |
| Paneles de puerta | `admiral868bit256` | por puerta | 52 / 56 |

4. Identificar lo que se retira del PC:
   - `admiral868bit128` en la cabina (y -2.3..2.2): 338 caras.
   - `vehicledash32` y `enginetopb4bit64`: salpicadero y motor placeholder.
   - Tarjetas interiores de las 8 puertas: `admiral868bit128` (25-32 por puerta).
   - Conservar las 87 caras del atlas PC de morro/cola, las 356
     `admiral86body8bit64`, las 228 `admiral86body8bit128` y los 20 cristales.
5. Comprobar los **espacios locales**: las puertas Xbox y PC comparten origen; los
   atomics nuevos pueden colgar del `chassis_hi`/`door_*_hi_ok` con matriz identidad.
   Si el donante tiene otro origen, transformar los vértices antes de comparar.

## 3. Pipeline de construcción

Script de referencia: `gta_vc_browser/tmp/base60/admiral_pilot.py`.

1. **Retirar el interior PC** con un predicado por material y `y`:
   `pc_keep()`; `remove_pc()` reconstruye el STRUCT con `split_triangles`.
2. **Construir cada atomic Xbox**:
   - `make_geom()` filtra los triángulos por textura/franja y conserva vértices,
     UVs, normales y morph.
   - `add_frame_atomic()` crea el marco hijo, el atomic y su geometría; usa un
     atomic PC como plantilla de extensión.
   - Nombres: `od_interior`, `od_engine`, `od_under`, `od_engine_bay`,
     `od_door_*_ok`/`_dam`.
3. **No duplicar piezas PC**: si una pieza Xbox cae sobre una cara PC existente,
   quitarla del donante o de la base. El Admiral dejó 9 triángulos de `belowcars`
   coplanares con `chassis_hi` (ver sección 6); se separaron 4 mm en la T4. **No se
   borran**: la T3 los quitó y apareció un hueco en los bajos.
4. **TXD**: fusionar solo las texturas que falten (`admiral868bit256`, `limo`,
   `belowcars`) con `txd_merge.merge(..., only={...})`. Conservar los filtros,
   formatos y el atlas PC. Validar con `xt.assert_pc_txd()`.
5. **Instalar** en los tres árboles (`streamed`, `bootseed`, `gamefiles`) y
   reempaquetar el `.img`/`.dir` (sección 8).

## 4. Sistema de daños: lo que hay que saber

- `HideDamagedAtomicCB` oculta al cargar todo atomic cuyo frame contenga `_dam` y le
  pone el flag `ATOMIC_FLAG_DAM`.
- `SetAtomicRendererCB` marca `ATOMIC_FLAG_DOORWINDOW` a los nombres que empiezan por
  `window`; `windscreen_*` va por su cuenta.
- `SetComponentVisibility` (en `Automobile.cpp`) recorre el nodo de la puerta y
  muestra solo `_ok` o `_dam`. El cristal original no tiene `_ok`/`_dam`: cuando la
  puerta se daña se oculta y queda el panel `_dam`.
- Por eso un panel `_dam` **no puede confiar en el cristal original** para el tinte:
  si el panel de grietas es blanco, la luna dañada se ve más clara.

### 4.1 Cristales dañados con tinte

Solución aplicada en el Admiral:

1. Copiar el cristal original (material `None`, color 89,89,89,89, sin textura).
2. Crear `window_{lf,rf,lr,rr}_hi_dam_base`, nombre con `_dam`, mismo padre
   (`door_*_hi_ok`) y misma matriz que el cristal original.
3. Desplazarlo **0.004 hacia el interior** para que no pelee con el panel de grietas.
4. Dejar delante el panel blanco `window_*_hi_dam` (grietas). Al dañar la puerta se
   muestran base oscura + grietas; al estar sana, el original.
5. No hace falta para el parabrisas, que ya es claro.

### 4.2 Paneles de puerta dañados

El panel `_dam` Xbox no coincide con la chapa `_dam` PC: si se copia tal cual, el
panel sobresale en las puertas abolladas. Solución aplicada:

1. Partir del panel **Xbox sano** (`od_door_*_ok`), no del `_dam` Xbox.
2. Para cada vértice, proyectar en la chapa PC `_ok` (materiales
   `admiral86body8bit64/128`) y buscar el punto más cercano en la chapa PC `_dam`.
3. Aplicar el desplazamiento `q_dam - q_ok`, con tope de 0.12, y añadir 0.004 hacia
   el interior.
4. Reconstruir el STRUCT y sustituir solo la geometría de `od_door_*_dam`; los `_ok`
   no se tocan.
5. Verificar que el bbox local del panel queda dentro del bbox de la puerta PC `_dam`.

## 5. Correcciones que exigió el piloto

El primer piloto (60.9-T1) se veía sólido, pero el jugador detectó tres cosas:

1. **Bajos y vano incompletos**: faltaban `belowcars` (159) y las 34 caras del fondo
   del vano; el motor parecía flotar. Se añadieron `od_under` y `od_engine_bay`.
2. **Cristales dañados más claros**: el panel de grietas era blanco y sustituía al
   cristal tintado. Se añadió la base oscura de 4.1.
3. **Paneles de puerta sobresaliendo**: el `_dam` Xbox tenía los dobleces Xbox, no los
   PC. Se deformó con la chapa PC de 4.2.

Las 22 caras PC de y 1.5..2.2 no se repusieron: 18 eran fondo duplicado del Xbox y 4
marginales. Si un coche deja un hueco en la mampara, añadir solo esas 4, no las 22.

## 6. Z-fighting: comprobación obligatoria

Un trasplante puede añadir una segunda superficie **exactamente coplanar** con la
base. No se ve en el DFF a simple vista, pero parpadea en cuanto la cámara se mueve.

Caso Admiral: `od_under` (`belowcars`) y `chassis_hi` (`admiral868bit128`) tienen
**9 triángulos coplanares** (7 con vértices idénticos y 2 a 3 mm; solape 100%):

- morro: y 2.13..2.42, z -0.53;
- cola: y -2.73..-2.33, z -0.51..-0.49;
- x hasta ±1.01.

Son las caras del faldón/parachoques que `belowcars` duplica. El parpadeo del borde
inferior y del parachoques trasero con la placa salía de ahí. La T3 los quitó y
dejo un hueco visible en los bajos. **La T4 corrige el método**: `admiral_fix4.py`
parte del DFF T2, conserva las 159 caras `belowcars` y baja todo `od_under` **4 mm**.
La chapa PC queda en su plano, los bajos justo debajo, con **0 solapes > 10% y sin
hueco**. Las caras PC se conservan. Si vuelve a aparecer un coplanar, separar en
profundidad (2-5 mm), nunca borrar la pieza: puede ser la única superficie que cierra
el hueco.

Comprobación reutilizable: para cada par de atomics que se dibujan a la vez, comparar
normales (|dot| > 0.995), planos (|dz| < 0.002) y solape 2D de triángulos. Si el
solape es > 10% y los dos atomics están visibles en el mismo estado, hay z-fighting.

### 6.1 No confundir con LOD o estados

- `chassis_hi` y `chassis_vlo` son **LOD alternativos**: sus triángulos pueden ser
  coplanares a propósito. En el taxi, el parpadeo real de las pegatinas y las líneas
  **no era el LOD**: en la sesión T4 no aparece ninguna línea `VEHLOD` del taxi. Era
  el material 8 de `chassis_hi` (`taximiami868bit128`, 64 caras, 124 vértices
  exclusivos), una **capa duplicada del panel de las cuatro puertas**. La T5 la
  separó 5 cm hacia dentro y **se revirtió a la T4** (`83c0aeb5…`, 233.472 B): el
  jugador no vio cambio. El parpadeo no era esa capa sino el *near clip* de la
  cámara (§6.3); la T4 conserva las 64 caras en su plano.
  `chassis_hi`/`chassis_vlo` siguen siendo alternativos; la traza `VEHLOD` sirve
  para descartar el LOD en otros coches.
- `_ok` y `_dam` también son alternativos: `door_rr_hi_dam`/`door_rr_hi_ok` del taxi
  tienen un solape coplanar enorme (dz 0.0001). Solo deben verse uno; si se ven los
  dos, el responsable es la **visibilidad de daño** (`HideDamagedAtomicCB` /
  `SetComponentVisibility`).
- Las ventanas del **edificio alto que está detrás de Coin Laundry** (Ocean Beach,
  no el local bajo de Coin Laundry) no son LOD: son **11 quads `od_copwindows`** a
  z=14.875, material gris sin textura, colocados a ras del muro en
  `data/maps/oceandn/oceandN.ipl`. En el log de la sesión T3 no aparece `WORLDLOD`
  para ellas; no hay cambio de modelo. La T5 las separó **8 cm** y también se
  revirtió: el IPL vuelve a la T4 (`77737 B`, `b07cb84b…`), con las 11 ventanas a
  **2 cm** del muro
  a lo largo de su normal local +X, sin borrar ninguna. `WORLDLOD` sigue disponible
  para otros edificios.

### 6.2 Cómo leer las trazas (60.9-T6)

El build deja dos trazas, **una línea por cambio de estado**, no una por frame:

- `VEHLOD tex=<textura> atom=<atomic> lod=0|2 dist=<m> thr=<m>`: cambio de LOD de un
  vehículo. `tex=taximiami868bit128` identifica al taxi; `lod=0` es `chassis_hi` y
  `lod=2` es `chassis_vlo`. La tabla guarda 16 vehículos.
- `WORLDLOD model=<modelo> dist=<m>`: cambio del atomic LOD de un edificio/objeto
  simple. La tabla guarda 128 modelos.

Cómo usarlas:

1. Reproducir el parpadeo del taxi y mirar si aparece `VEHLOD` en ese momento.
2. Si `lod=0` y `lod=2` alternan con `dist` cerca de `thr`, es el umbral de LOD; si
   no aparece ninguna línea y las pegatinas siguen, buscar una capa duplicada en el
   DFF (la T5 apuntó a `chassis_hi` material 8 contra las puertas, pero el
   parpadeo real era el near clip; §6.3), no el LOD.
3. Para el edificio, mirar `WORLDLOD`: el `model=` que cambia cuando parpadean las
   ventanas es el responsable; si no hay línea, el edificio no está cambiando de LOD.
   Las ventanas del edificio alto detrás de Coin Laundry son 11 quads
   `od_copwindows`; en la T6 vuelven a estar a 2 cm del muro en `oceandN.ipl`.
4. Adjuntar al informe las líneas y la zona; con el nombre del modelo se puede
   inspeccionar el DFF/TXD concreto.

### 6.3 El fondo parpadea al acercarse a una pared: el near clip (resuelto en T12)

Síntoma: al pegarse a un muro, el fondo (ventanas, muros lejanos) parpadea; al
separarse, se estabiliza.

Qué se descartó:

1. **Geometría duplicada/LOD:** no hay pares coplanares visibles a la vez en la
   zona (los `deco_*`/`LODo_*` de `oceandN.ipl` comparten posición pero son LOD
   normal: se dibuja uno u otro). Las 11 ventanas `od_copwindows` se separaron 2 cm
   (T4) y luego 8 cm (T5); el parpadeo siguió igual.
2. **Precisión de profundidad:** el formato no era el problema (T8, reversed-Z
   32F, rompió los pases 2D y se revirtió en T9), pero el **near clip** sí: pegado
   al muro bajaba a 0.04–0.15 y el error de 24 bits a 50–150 m subía a 3–27 mm,
   suficiente para que ventanas de 2 cm y calcas de 4 mm perdieran trozos. La T7
   (suelo 0.5) no se respetaba —otros pasos lo bajaban a 0.15/0.1/0.05— y además
   recortaba al personaje.
3. **Bombeo de la cámara:** la T10 añadió 0.5 s de histéresis en
   `Process_FollowPedWithMouse`; el jugador siguió viendo el parpadeo. El log de la
   T10 no tiene **ni una** muestra con la cámara cerca y quieta: con `vel=0.00`,
   `dist` seguía cambiando (0.39→0.54→1.30→3.65) y `dbeta`/`dalpha` no eran 0. La
   prueba nunca se hizo en reposo.

Evidencia del jugador (T11): el parpadeo sigue **con la cámara quieta** y también
afecta a pegatinas de policía/taxi/ambulancia. No es movimiento de cámara; el
patrón (trozos que desaparecen) es z-fighting por precisión de profundidad.

T11 (latido de la cámara quieta) no lo arregló y se conserva solo como mejora de
estabilidad. T12 impone un suelo real de near **0.25** en todos los caminos de la
cámara de seguir-ped y en `fCloseNearClipLimit` (0.15→0.25), que era el punto que
volvía a bajar el near tras la T7. Con near 0.25 el error a 50 m es ~0.6 mm y a
150 m ~5 mm (antes 3 mm y 27 mm con near 0.04): las ventanas a 2 cm y las calcas a
4 mm dejan de cruzar el umbral. No toca profundidad global ni pases 2D.
**Validado por el jugador (2026-11-16): el parpadeo desapareció con la T12.**
Si reapareciera en una ventana exactamente coplanar (offset 0), habría que
separarla en el DFF; el near no puede resolver un empate exacto.

## 7. Validación del Admiral y del taxi (T4/T6)

- `dff_structver.py`: 62 geometrías, 0 versiones malas, 0 mallas malas.
- `dff_readcheck.py`: 0 errores.
- `od_under`: 159 caras `belowcars`, 0 solapes > 10% con el atlas PC; separado 4 mm.
- `txd_inspect.py`: 14 texturas, 32 bits sin comprimir; `assert_pc_txd` OK.
- `gen_manifest.py`: 17489 entradas, sello `92212e93731dccf4ced373701095a858`.
- `check_img_index.py`: índice contiguo, copias sincronizadas.
- `build.sh` RC=0; `check-served-build.sh` RC=0 con `dataTag`/`VERSION`
  `2026-11-16-60.9-T6`, las marcas `VEHLOD`/`WORLDLOD` y `oceandN.ipl` con las
  11 ventanas a 2 cm.
- Admiral: DFF `40febfde965ae4b02dc7690ec6db3b675810fc862fc355116afde2b0279d62b8`,
  TXD `55ba96ba06638e01daf3cbc77e5641469b06d88ca9a222f834d8e9261632a978`.
  Copia T3 anterior: `gta_vc_browser/tmp/base60/pilot67/t4_backup/`.
- Taxi T4 (T6 revierte la T5): DFF `83c0aeb57738d5955aff0cc85de56c085ed75c98fc6e35d614b60d2be830f202`
  (233.472 B), 64 caras del material 8 en su plano; TXD
  `0b2aebe00859d94b9972b94e7dd52d44445aeb7dce687259bf6c25f661f0a696`.
  Copia de la T5: `gta_vc_browser/tmp/zfight/taxi_t5_backup/` (no usar).
- Mapa T4 (T6 revierte la T5): `data/maps/oceandn/oceandN.ipl` 77.737 B, sha256
  `b07cb84be1bac59b8c537ea9ad16e0776b4ac0f0731cf1a2e89e9e379d3183f0`; las 11
  ventanas `od_copwindows` a 2 cm. Copia de la T5:
  `gta_vc_browser/tmp/zfight/oceandN_T4.ipl.bak` (no usar).
- `gen_manifest`: 17.489 entradas, sello `92212e93731dccf4ced373701095a858`.

## 8. Comandos de referencia

```bash
cd gta_vc_browser
PYTHONIOENCODING=utf-8 python tmp/base60/admiral_pilot.py --install
PYTHONIOENCODING=utf-8 python tmp/base60/admiral_fix.py --install
PYTHONIOENCODING=utf-8 python tmp/base60/admiral_fix3.py --install
PYTHONIOENCODING=utf-8 python tmp/base60/admiral_fix4.py --install
PYTHONIOENCODING=utf-8 python tools/gen_manifest.py
PYTHONIOENCODING=utf-8 python tools/check_img_index.py \
  --img streamed/models/gta3.img --mirror bootseed/models/gta3.dir
bash build.sh
bash tools/check-served-build.sh
```

`tmp/zfight/taxi_fix5.py` no se vuelve a ejecutar: su cambio (T5) está revertido.

Y para futuros coches:

```bash
PYTHONIOENCODING=utf-8 python tools/dff_geom.py <coche>.dff
PYTHONIOENCODING=utf-8 python tools/dff_readcheck.py <coche>.dff
PYTHONIOENCODING=utf-8 python tools/txd_inspect.py <coche>.txd
```

## 9. Checklist para los siguientes 10 coches

1. Un coche por piloto; nunca varios a la vez. Guardar copia del DFF/TXD previos.
2. Identificar piezas Xbox por textura/franja; no portar el interior PC ni el atlas PC.
3. Añadir interior, motor, bajos, fondo del vano y paneles de puerta con sus texturas.
4. Cristales: base oscura `_dam` si el panel de grietas es blanco.
5. Paneles: `_dam` desde el panel Xbox sano deformado con la chapa PC `_dam`.
6. Pasar el detector de coplanares entre atomics visibles a la vez; separar en
   profundidad 2-5 mm (el Admiral T4 bajó `od_under` 4 mm). No borrar la pieza: la
   T3 quitó 9 triángulos y dejó un hueco visible.
7. Validar `structver`, `readcheck`, TXD, manifiesto, índice y build servido.
8. Probar en juego: bajos boca abajo, vano, puerta dañada, lunas dañadas y paneles.
9. Si aparece parpadeo, separar: z-fighting real (misma capa), LOD (`_hi`/`_vlo`) o
   estado (`_ok`/`_dam`). Para el taxi usar `VEHLOD`; para edificios, `WORLDLOD`.
   Las ventanas del edificio alto detrás de Coin Laundry son 11 quads
   `od_copwindows` a 2 cm del muro en `oceandN.ipl` (T4, restaurado en T6); si
   vuelven a parpadear con la cámara pegada al muro, la T12 lo resolvió con near
   0.25 (§6.3); si reaparece en una ventana coplanar, hay que separarla en el DFF,
   no tocar la profundidad global.
10. Anotar en el plan del coche las piezas, caras, texturas y desviaciones.

## 10. Referencias

- Plan del piloto: `.agents/plans/2026-11-15-60.7-coches-xbox-transplantes-base-pc.md`.
- Scripts: `gta_vc_browser/tmp/base60/admiral_pilot.py`,
  `gta_vc_browser/tmp/base60/admiral_fix.py` y
  `gta_vc_browser/tmp/base60/admiral_fix3.py` y
  `gta_vc_browser/tmp/base60/admiral_fix4.py`.
- Herramientas: `dff_geom.py`, `dff_structver.py`, `dff_readcheck.py`,
  `txd_merge.py`, `txd_inspect.py`, `gen_manifest.py`, `check_img_index.py`.
- Código de daños: `src/modelinfo/VehicleModelInfo.cpp` (`HideDamagedAtomicCB`,
  `SetAtomicRendererCB`) y `src/vehicles/Automobile.cpp`
  (`SetComponentVisibility`).
- Trazas LOD: `src/rw/VisibilityPlugins.cpp` (`VEHLOD`) y
  `src/renderer/Renderer.cpp` (`WORLDLOD`).
