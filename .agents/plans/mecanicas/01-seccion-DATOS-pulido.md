---
name: 01-seccion-DATOS-pulido
status: EXECUTED
type: feature
domain: gameplay
owner_rules: .agents
created: 2026-09-19
---

# Sección 1 — Datos y pulido visible (**la más liviana**, la llevo yo)

Cierro lo que ya está empezando a molestarse: caché, nombres del HUD, sirena
policial, tráfico, `ped.ifp` y las tres features de datos que quedaron fuera.
Reglas y arnés: `00-INDICE.md`. Yo también mantengo `VERSION`/`dataTag`, el
`bootseed`/manifiesto y el `.data` de cada integración.

Orden por valor visible (y por lo que desbloquea a las otras secciones).

---

## D1 — Cerrar el agujero de caché (`dataTag` en `ve3`)

**Por qué primero:** los bloques 3 (armas) y 4 (vehículos) **sustituyeron**
ficheros en rutas que ya existían (76 entradas de `weapons.img`, 11 de
`anims.img`, `.col`, más los 8 vehículos nuevos) y el `dataTag` sigue en
`2026-09-19-ve3` (`web/ondemand.js:112`). La caché IDB va **por ruta**, sin
tamaño ni etag: quien haya jugado con `ve3` puede seguir viendo las armas/anim
viejas. Las otras dos secciones **no deben empezar a medir** hasta que esto esté
cerrado, o medirán una partida con datos mezclados.

**Tarea:** subir `dataTag` a `2026-09-19-ve8` (o el que toque tras este bloque)
en `web/ondemand.js`, subir también `VERSION` en `web/lib/index.js`, recompilar
(`ondemand.js` va **embebido** en `reVC.js`: sin relink no cambia nada) y
comprobar en una sesión limpia que el log imprime el tag nuevo y que la caché se
purga (`ODWARM`/`ODCAP` en `odtrace.log`).

**PASS:** log con `data=2026-09-19-ve8`, arranque sin `Failed to load` y sonda
base `slot0-load-test.mjs` en PASS.

---

## D2 — HUD "streetfighter missing" (bug del jugador)

**Diagnóstico (ya hecho, medido):**

- `CCurrentVehicle::Display()` (`src/core/User.cpp:111`) hace
  `TheText.Get(((CVehicleModelInfo*)...)->m_gameName)` y el HUD lo pinta al
  entrar en un vehículo.
- `CData::Search` (`src/text/Text.cpp:396`) devuelve `"<clave> missing"` si la
  clave no existe, y la búsqueda es `strcmp` + `BinarySearch` → **sensible a
  mayúsculas** y **ordenada**.
- Nuestro `default.ide` (pack 4) había copiado los `gameName` del mod, que son
  largos y en minúsculas (`Streetfighter`, `Trashmaster`, `VCPD_WinterGreen`…)
  → el HUD pintaba `streetfighter missing` (literal), y además el campo del
  motor (`m_gameName[10]`) no aguanta 16 caracteres.
- Claves GXT válidas: 8 bytes con NUL = **máximo 7 caracteres**.

**Hecho ya:** `streamed/data/default.ide` corregido a claves cortas y en
mayúsculas, reusando las que ya existen (`PEREN`→Perennial, `TRASHM`→Trashmaster)
y añadiendo claves nuevas:

| ID | modelo | clave | valor |
|---|---|---|---|
| 6500 | streetfi | `STRTFTR` | Streetfighter |
| 6503 | hellenbach | `HELLENB` | Hellenbach |
| 6504 | premier | `PREMIER` | Premier |
| 6505 | manchez | `MANCHEZ` | Manchez |
| 6506 | wintergreen | `WINTERG` | Winter Green |
| 6507 | polwintergreen | `VCPDWTR` | VCPD WinterGreen |

**Falta:** meter esas 6 claves en **todos** los GXT que servimos
(`streamed/TEXT/*.gxt`: american, french, german, italian, russian, spanish,
ukrainian) con `tools/gxt_inspect.py add`, volver a `stage_bootseed` +
`gen_manifest` y **recompilar** (`TEXT/` es `FULL_DIRS`: va en el `.data`).

**Herramienta nueva:** `tools/gxt_inspect.py`
(`claves <gxt> [filtro]` / `add <gxt> CLAVE=valor …`) — parsea la cadena de
chunks `TABL`/`TKEY`/`TDAT`, ordena y reescribe. **Verificación estática:**
`tools/check_ide_gxt.py` (comprueba que **todo** `gameName` de vehículos del
`default.ide` existe en el GXT y cumple ≤7 chars), a correr tras cada cambio de
datos.

**PASS:** checker en verde + sonda que entra en un coche nuevo y comprueba en la
consola/captura que el HUD ya no dice "missing".

---

## D3 — Luces de sirena de la policía (**hecho**, ver "Estado")

**Hallazgo principal (medido en código, no en captura):** el sospechoso del plan
era la moto, pero había **dos** piezas, no una:

1. `CVehicle::UsesSiren()` (`src/vehicles/Vehicle.cpp`) es un `switch` de IDs
   fijos y **no incluía `MI_VEEXT_POLWINTERG`** → la moto policial no tenía
   sirena *de ningún tipo*: ni se conmutaba con el claxon, ni entraba en el
   bucle de audio de sirena, ni `CarAI`/`CarCtrl`/`RoadBlocks` la trataban como
   vehículo con sirena. (El plan daba por hecho que "sonaba": sonaba lo que
   `m_bSirenOrAlarm` dispara en el mezclador, pero sin poder encenderse desde el
   manillar.)
2. `CBike` no tiene el bloque de coronas que `CAutomobile` sí tiene en su
   `ProcessControl` (`switch(GetModelIndex())` → `MI_POLICE`/`MI_AMBULAN`/…,
   `TYPE_STAR` rojo/azul + `CPointLights`) → la moto no podía parpadear.

Ambas detrás de `#define VICEEXT_POLICE_BIKE_LIGHTS` (`config.h`), más paridad
con el coche en la máquina de estados del claxon (pulsación corta = conmutar la
sirena, mantener = pitar) y las coronas en `CBike::ProcessControl` (dos luces
`TYPE_STAR` en lo alto del manillar, `(uintptr)this + 21` y `+ 23` —los IDs
`+1/+6/+14/+22/+25` de las luces de la moto ya estaban ocupados—, con el mismo
temporizador de 1023/511 ms que el coche). El cheat `CRAZYRIDES` crea la moto
**con la sirena encendida** para poder verla sin conducirla (es un cheat de
diagnóstico; que la use la poli de verdad es P3 de la sección 2).

**El coche de policía está intacto** (no lo tocamos): su `switch` sigue igual y
sus texturas están:

- `particle.txd` servido (el del mod, **byte a byte idéntico**, 116 texturas)
  trae las 8 coronas (`coronastar`, `corona`, `coronamoon`…). En el mod son
  32 bits RGBA sin comprimir (0x500) frente a 16 bits del original, pero el
  port las decodifica por el camino normal (son el mismo tipo de textura que el
  resto del mod).
- `police.txd` servido tiene 17 texturas, con `lights`, `lightson`,
  `servicelight`, `servicelights`, `servicelightson` ✓.

**Herramientas nuevas:** `tools/shot_stats.py` (estadísticas de capturas sin
dependencias) y `tools/siren-smoke-test.mjs` (carga el slot 0, teclea
`CRAZYRIDES`, gira la cámara y mide sobre las capturas).

**Cuidado con el arnés (dos falsos veredictos ya corregidos).** El primer
criterio era "píxel azul dominante" (`b>50 && b-r>25 && b-g>25`) contado en una
ventana centrada en la luz, y **suspendió una sirena que sí se dibujaba**. Dos
fallos independientes, los dos del arnés y no del motor:

1. **El color de la sirena es diminuto a propósito**: el código original lo pasa
   por `/6` (`255/6 = 42`) y el blending de coronas es *aditivo* (`ONE,ONE`).
   Sobre un asfalto cálido, sumar 42 de azul sube el azul sin que llegue a
   *dominar*: la métrica de color no puede verlo ni con la luz en pantalla.
2. **La ventana se situaba mal**: se proyectaba la luz con `CalcScreenCoors` y se
   tomaba la **mediana de todas las poses de cámara** de la sesión. Con la
   cámara girando entre series, esa mediana no es la posición de ninguna serie.

El criterio que sí mide una sirena es **alternancia**: en una serie de capturas
con la cámara quieta, una celda (24×24) que oscila en rojo *y* en azul con los
dos canales **anti-correlacionados** (`corr ≤ -0.8`), con amplitudes en la banda
`[4, 55]` —el techo es el propio `/6`: una oscilación mayor no es una luz
aditiva, es la escena cambiando— y **oscilando de verdad** (un solo escalón a
mitad de serie da `corr=-1` sin ser un parpadeo). Y la especificidad la da un
**control apareado**: la misma ventana medida en la base (misma cámara, sin los
vehículos del mod). El mar, el agua y el streaming de texturas se mueven en las
dos series, así que no deciden; sólo la sirena aparece en una y no en la otra.
La ventana se sitúa con la posición que da **el propio motor** (`BSIREN`, ahora
cada 20 fotogramas) muestreada **en la misma pose** que la serie.

<details>
<summary>Medición manual que confirmó el motor antes de arreglar el arnés</summary>

Con la corrida anterior (misma cámara, 4 capturas cada 450 ms, moto policial
delante) y midiendo a mano una caja de 90×60 px en la zona donde el mapa ASCII
de brillo mostraba el foco:

| Celda | azul por captura | rojo por captura | rango azul | rango rojo | corr |
|---|---|---|---|---|---|
| (800, 120) | 19, 11, 11, 19 | 26, 39, 47, 26 | 8,2 | 21,9 | **-0.95** |
| (820, 120) | 17, 10, 10, 17 | 26, 39, 47, 26 | 7,6 | 20,4 | -0.94 |
| (800, 100) | 16, 9, 9, 16 | 39, 55, 66, 39 | 6,7 | 27,7 | -0.94 |
| base, mismas celdas | (constante) | (constante) | **0,0** | **0,0** | – |

Con la sirena: el azul está encendido en dos capturas y apagado en las otras dos,
el rojo justo al revés, y la caja de la base (misma pose, sin vehículos del mod)
no se mueve **nada**. Eso es un parpadeo rojo/azul en el sitio donde el motor
dice que está la luz.
</details>

<details>
<summary>Texto original del bloque (diagnóstico previo)</summary>

El jugador reportó que "desapareció la textura de las luces de sirena de la
policía". Medido hoy:

- Las coronas del port (`CCoronas::gpCoronaTexture[]`, `Coronas.cpp:69-76`:
  `coronastar`, `corona`, `coronamoon`…) **están** en el `particle.txd` servido
  (el del mod, 116 texturas, las 8 coronas presentes ✓).
- El `police.txd` servido (mod) **contiene** todas las texturas del vanilla
  (`lights`, `lightson`, `servicelight*`) ✓ → no es una textura ausente del
  coche.
- Las coronas de sirena del **coche** las pone `CAutomobile` en un
  `switch (GetModelIndex())` (~`Automobile.cpp:2140-2200`, `TYPE_STAR` rojo/azul
  + `BRIGHTLIGHT_SIREN` en `renderer/SpecialFX.cpp`).
- **`CBike` no tiene ese código**: la moto policial nueva
  (`polwintergreen`, 6507) **no parpadea** aunque tenga sirena de sonido — es la
  sospecha principal de lo que vio el jugador.

**Tareas:** (1) reproducir con una sonda que ponga un coche de policía y la moto
VCPD en pantalla y capture (así se ve si el coche sí parpadea y la moto no);
(2) si el coche está bien, implementar las coronas de la moto
(`#define VICEEXT_POLICE_BIKE_LIGHTS`, grupo `MI_VEEXT_POLWINTERG`, dos coronas
`TYPE_STAR` alternando rojo/azul en la posición del faro); (3) si el coche
también falla, revisar `CCoronas` con la captura como evidencia (¿`TYPE_STAR`
sin textura? ¿coronas recortadas por el nuevo renderer?).

**PASS:** captura de la moto con luces alternando (y coche policía igual que
vanilla).
</details>

---

## D4 — Tráfico de los 8 modelos del mod (**medido; falta la causa**)

**El dato está bien.** Los 8 están en el `default.ide` servido con la clase y la
frecuencia del mod, y `CFileLoader` los mete en los arrays de tráfico de
`CCarCtrl` (`AddToCarArray`, salvo `ignore` → `-1`, que no entra): 7 con clase
real (`motorbike`, `poorfamily`, `big`, `normal`) y `polwintergreen` con
`ignore`, que es lo correcto (su sitio es la policía: bloque P3 de la sección 2).

**La medición (sonda nueva `tools/traffic-smoke-test.mjs`, build `d4`):** carga
el slot 0, se queda quieto en la calle 6 minutos y cuenta las líneas
`CARSPAWN model=<id>` que ahora emite `CCarCtrl::GenerateOneRandomCar` justo
al entrar el coche en el mundo. Resultado:

| Medido | Valor |
|---|---|
| Coches de calle en 6 min | **32** (el tráfico *existe* y se mueve) |
| Modelos distintos | **2**: `197 oceanic` × 28 y `156 police` × 4 |
| Modelos del mod vistos | **0/8** |
| `polwintergreen` | **0** ✓ (no sale en tráfico, como debe) |

**Ojo con la métrica:** contar `TXDIN txd=<modelo>` **no sirve** — un modelo ya
residente no vuelve a pedir su TXD y el tráfico recicla los que tiene cargados
(medido: 0-1 `TXDIN` en 4 min de calle, y el único era de un arma). Por eso la
traza nueva está en el punto donde el coche entra al mundo.

**Dónde está la causa (a confirmar el próximo bloque):** el tráfico no elige de
todo `default.ide`, elige entre los modelos **ya cargados**
(`CCarCtrl::ChooseModel` reintenta hasta 10 veces y exige
`CStreaming::HasModelLoaded`). Y el cargador de vehículos
(`CStreaming::StreamVehiclesAndPeds`) pide **un modelo cada 350 fotogramas**
(`timeBeforeNextLoad = 350`) y sólo mientras `ms_numVehiclesLoaded <=
desiredNumVehiclesLoaded`. Dos cosas que cuadran con "sólo `oceanic`":

1. 350 **fotogramas** asumen 30 FPS: en el navegador (6-9 FPS) la rotación real
   es 3-5 veces más lenta, así que el juego se queda con los 2-3 modelos que ya
   estaban residentes y no llega a cargar el resto de la clase;
2. si `ms_numVehiclesLoaded` ya está en el tope, la rama no vuelve a pedir nada
   y el conjunto residente se congela.

**Tareas siguientes:** (a) medir `ms_numVehiclesLoaded` vs
`desiredNumVehiclesLoaded` y `NumRequestsOfCarRating` en partida (traza corta en
el mismo sitio); (b) decidir el ajuste: temporizador en milisegundos (11,7 s =
350 fotogramas a 30 FPS) y/o ritmo de carga, detrás de `__EMSCRIPTEN__`;
(c) volver a pasar la sonda y exigir ≥3 de los 7 modelos del mod.

**PASS:** tabla de "modelo → cuántas veces visto en N minutos" con ≥3 de los 7
del mod y `polwintergreen` a 0 + captura.

---

## Estado final de D4 (**confirmado en partida**, 20/09 15:03)

Medido con la instrumentación (`CARPED`, `CARPOOL`, `CARBLOCK`, `CARSPAWN`):

| Corrida | Ticks del cargador | Coches de calle | Modelos distintos | Modelos del mod |
|---|---|---|---|---|
| antes (temporizador en fotogramas) | — | 32 en 6 min | **2** | 0/7 |
| después (temporizador en ms) | 1 cada 11,7 s | **367 en 5 min** | **30** | **3/7** (Streetfighter ×7, Hellenbach ×11, Premier ×6) |
| después, con la CPU saturada (13 FPS) | **0** | 45 en 3 min | 2 | 0/7 |

La tercera fila **es la prueba de la cadena causa-efecto**: sin ticks del
cargador, la calle elige sólo entre los modelos ya residentes y sale el
monocultivo de `oceanic`. Con ticks, 30 modelos.

**Lo que quedó implementado:**

1. **Temporizador en tiempo real** (`350/30 s = 11,7 s`) detrás de
   `__EMSCRIPTEN__`. El original cuenta fotogramas asumiendo 30 FPS: en el
   navegador, a 5 FPS son 70 s por modelo y la rotación se congela. El
   temporizador sigue reiniciándose **sólo cuando se pide un modelo**, como el
   original.
2. **Puerta relajada** para el cargador de tráfico: deja de exigir
   `ms_numModelsRequested < 5` y "sin carga prioritaria". Medido: hay sesiones
   enteras (CPU compartida, otro Chrome jugando) con la puerta cerrada los 5
   minutos y **0 ticks** → calle congelada. El cargador pide como mucho **un
   modelo cada 11,7 s**, así que no compite con el resto del streaming; se
   mantienen las condiciones que sí son de estado del juego (área, cinemática,
   replay, streaming desactivado).
3. **Diagnóstico `CARBLOCK`** (cada 300 fotogramas): si la puerta vuelve a
   cerrarse, el log dice **cuál** de las condiciones lo hizo
   (`disable/cut/prio/area/play/nreq`).

**Confirmado en la partida del jugador** (la sesión anterior a la de abajo, con
un build previo al `ve10`): 146 ticks del cargador, **238 coches de calle, 29 modelos distintos** y
**3 de los 7 del mod circulando** (`6500 Streetfighter` ×9, `6504 Premier` ×3,
`6505 Manchez` ×10), `polwintergreen` a 0 ✓. Los otros 4 no salieron en esa
sesión: sus clases (`bikes`, `poorfamily`, `big`) tienen muchos modelos y la
frecuencia del mod es la de serie (10, y 7 el Hellenbach) — por eso el refuerzo
temporal de abajo.

**Medida corregida del verificador:** el ritmo entre ticks no se puede medir de
la traza (sella por lotes, muchas líneas comparten marca); el dato válido es el
**número** de ticks (0 sin el arreglo, 146 con él).

### Refuerzo temporal de frecuencia (pedido del jugador, 20/09)

Con la frecuencia del mod (10, y 7 el Hellenbach) los 7 modelos **salen**, pero
repartidos entre todos los de su clase: en 5 min se vieron 3 de 7. Para poder
comprobarlos a ojo, el jugador pidió **subirles el peso** y bajarlo después.

- `tools/boost_veh_freq.py` escribe la **columna 9** (`frequency`, la 9ª que lee
  el `sscanf` de `CFileLoader::LoadVehicleObject`) de los ids 6500-6506 en
  `streamed/data/default.ide`: de 10 (y 7) a **100**, ~10× su probabilidad
  relativa dentro de su clase. **6507 (moto policial) no se toca**: es `ignore`,
  no debe salir en tráfico.
- Guarda los originales en `tools/veh_freq_backup.json` antes del primer cambio;
  `python tools/boost_veh_freq.py --revert` los restaura exactos. Tiene
  `--dry-run`, `--show` y `--freq N`.
- **Es un cambio de DATOS**: `data/` va precargado en el `.data`, así que exige
  `python tools/stage_bootseed.py` **y recompilar** (hecho: `dataTag`/`VERSION`
  a `ve10`). Para volver a la normalidad: `--revert` + stage + relink.

**El peso solo no bastaba (medido en la 2ª partida).** La frecuencia pesa en
`CCarCtrl::ChooseCarModel` (elegir **entre los modelos ya cargados**), pero
`CCarCtrl::ChooseCarModelToLoad` (qué modelo **pide** el cargador) sortea
**uniforme** entre los ~13 de la clase. Con 100 el Hellenbach salió 25 veces y
el Wintergreen 9, pero el Premier y el Manchez **nunca entraron en el fondo**:
sólo 4 de los 7 llegaron a pedirse en 6 minutos. Arreglado en código (en el wasm
servido del 20/09 13:20): ver "El refuerzo de frecuencia no bastaba: el cargador
pide UNIFORME".

---

## Partida del jugador **con el build `ve10`** — veredicto (sesión del log `17:57–18:10Z`)

> Es la sesión que quedó en `web/odtrace.prev.log` (mi rotación escribe ahí la
> anterior y empieza `odtrace.log` con la marca `rotado: sesion nueva`). No es la
> misma que el cuadro de "Estado final de D4" de arriba: aquélla corrió un build
> anterior (su cabecera decía `data=undefined`, el bug del `dataTag`).

Leída **sólo** de `web/odtrace.prev.log` (7.001 líneas), con
`tools/viceext-log-check.py`:

| Bloque | Veredicto |
|---|---|
| **D2** | OK — 0 `TXTMISS` |
| **D4** | OK — 259 coches de calle en 27 modelos, 103 ticks del cargador; del mod **Hellenbach ×25** y **Wintergreen ×9**; moto policial 0 |
| **D5** | OK — `IFPFILE ANIM\PED.IFP clips=272 total=272` (el del mod) |
| **D6** | OK — 3 armas apuntadas con su mira y su textura cargada (24→mira 3, 19→4, 17→2) |
| **D7** | OK — un aviso real resolvió su tecla y pintó el icono (`PED_ANSWER_PHONE` vk=9, `icono=1`; el cheat `CRAZYHINT` no se tecleó esta vez) |

Y dos cosas que **no** quedaron bien en esa partida, con su causa:

### 1. El refuerzo de frecuencia no bastaba: el cargador pide UNIFORME (arreglado en el wasm del 13:20)

De los 7 vehículos del mod sólo **4 llegaron a pedirse** (Hellenbach, Premier,
Manchez, Wintergreen) y sólo **2 circularon**. La frecuencia pesa al **elegir
entre los ya cargados** (`ChooseCarModel`), no al **pedir**
(`ChooseCarModelToLoad`, que sortea uniforme entre los ~13 modelos de la clase):
un modelo concreto necesita que le toque la lotería una vez para entrar en el
fondo, y con un pedido cada 11,7 s eso son minutos.

Arreglado en `CCarCtrl::ChooseCarModelToLoad` (`#ifdef __EMSCRIPTEN__`): se
sortea con **el mismo peso** (`CVehicleModelInfo::m_frequency`) que ya usa el
juego para elegir entre los cargados — acumulado, y se cae al uniforme si la
suma es 0. Así el fondo converge al reparto que dice el dato y **las frecuencias
subidas temporalmente sirven también para cargar** los vehículos nuevos.
`6507` no cambia: es `ignore` (no entra en `CarArrays`).

### 2. `ODSHORT open-fail` (60 en la partida): eran aplazamientos, no ficheros que faltan

La sonda miró los 60 nombres contra `web/public/manifest.json`: **los 60 tienen
entrada** (tamaño correcto). Son ficheros sueltos del `.img` que la capa
on-demand **todavía no tenía en MEMFS** y que aplaza a propósito para no congelar
el frame (el motor reintenta en otro fotograma; `RetryLoadFile`; no apareció
ningún mensaje de error en pantalla). Antes no se podía *afirmar* que volvían:
ahora el lector suelto lleva un anillo de los últimos fallos y, cuando uno se
abre bien, deja `ODSRECOVER <ruta> fallos=<n>` (y `ODSHORT ... total=<n>` sólo
cada 20 líneas, para que el total no se pierda en el recorte).
Si en la próxima partida hay fallos y **cero** recuperaciones, será un modelo
perdido de verdad.

### Trazas nuevas del cargador (código en el wasm servido, 20/09 13:20)

Emparejar petición con carga es lo que separa «su modelo no carga» de «su clase
no se sortea en esta zona»:

- `CARPED n=… rating=… cargadas=x/y model=… ya=… freq=…`  (pedido; ahora con la
  frecuencia del modelo pedido)
- `CARLOAD model=… clase=… freq=… fondo=x/y`  (el modelo **entró** en el fondo)
- `CARFAIL ch=… status=… intentos=… id=…`  (lectura del cargador fallida)
- `CARZONE umbrales=0,…` (los 8 umbrales de clase de la zona; los pone el script
  con `SetZoneCivilianCarInfo`, y **un umbral repetido es una clase con
  probabilidad cero** ahí)
- `CARRATE rating=… n=…` (clase sorteada, 1 de cada 8 sorteos)

Con eso, el peren2 (clase `poorfamily`) y el Trashmaster (clase `big`) quedan
explicados por dato, no por intuición: si su clase no aparece en `CARRATE`, no es
que no carguen — es que esa zona no los saca.

---

## Estado final de D5 · `ped.ifp` del mod (**en el build `ve10`**)

`gta_vc_browser/streamed/anim/ped.ifp` es el del mod: **2.486.356 B**
(md5 `c61a251f1b991661926838c2a5eeb896`), frente a los 2.201.152 B del de serie.
Comprobado a nivel de bytes: el fichero declara **272** animaciones
(`numAnimations` del chunk INFO, offset 16) y contiene **272** chunks `DGAN`; el
de serie declara 234 y tiene 234. Traza `IFPFILE <ruta> clips=<n> total=<n>` en
`CAnimManager::LoadAnimFile`.

**Confirmado en la 2ª partida (build `ve10`):** la traza dice
`IFPFILE ANIM\PED.IFP clips=272 total=272`. El 234 era de la pestaña con el
paquete anterior, como decía el párrafo de abajo.

**La partida del jugador dijo 234, y era correcto:** su pestaña corría el build
**anterior** al swap (paquete con el `ped.ifp` de serie). El build de las
**10:05** (relinkado al arreglarse el fichero roto de la sección 3) ya lo lleva:
la entrada del paquete para `/anim/ped.ifp` es `start=31.463.724 end=33.950.080`
= 2.486.356 B, y en ese offset está la cabecera `ANPK`+INFO del **mod**
(`INFO.n=272`). El fichero de serie no está en el paquete (búsqueda de su firma:
0 coincidencias). **A confirmar en la próxima partida: `IFPFILE ... clips=272`.**

---

## Estado final de D6 · mira por arma (**implementado**)

- `CWeaponInfo::m_nSight`: columna 27 de su `weapon.dat` (que ya se sirve con
  27 columnas en las 45 armas; la tabla leída: melee/granadas 0, pistolas 2,
  escopetas 4, SMG 3, rifles 5, `RocketLauncher`/`Gr_launch` 7, lanzallamas y
  minigun 6). El parser lee la columna **sólo si viene** (`sight` se inicializa a
  0), así que un `weapon.dat` de 26 columnas sigue funcionando.
- `weaponSights.txd` del mod dentro (`streamed/models/weaponsights.txd`, 7
  texturas: `sightDot`, `sightPistol`, `sightSMG`, `sightShotgun`, `sightRifle`,
  `sightHeavy`, `sightRocket`), cargado al inicializar el HUD. Los cuatro sitios
  donde el HUD dibujaba la cruz genérica (`HUD_SITEM16`) ahora llaman a
  `ViceExtDrawSight`: pinta la mira del arma en curso si la trae y su textura
  está; si no, la cruz de siempre (cero regresión para armas sin mira).
- Verificación por log: `SIGHTS cargadas=7/7` al arrancar (ya **visto** en una
  sesión real) y `SIGHT arma=<n> mira=<n> textura=<1> cargadas=7/7` al apuntar.
- `VALIDATE_SIZE(CWeaponInfo, 0x64)` sigue siendo un no-op (sólo se activa con
  `CHECK_STRUCT_SIZES`, que el build web no define), así que el campo extra no
  rompe nada.

**Pendiente de confirmar:** apuntar con 2-3 armas distintas y ver en el log
`SIGHT ... textura=1` con valores de `mira` distintos.

---

## Estado final de D7 · iconos de tecla en los avisos (**implementado**)

Es una feature **dirigida por datos**, no inventada: el GXT del mod (que ya
servimos) trae **29 acciones** en la forma `~k~~PED_FIREWEAPON~` (2.000+
usos en los 6 idiomas). El port ya las sustituía por el **nombre** de la tecla
(`CMessages::InsertPlayerControlKeysInString` → `GetWideStringOfCommandKeys`),
así que los avisos decían "Press and hold the Left Mouse Button button".

Lo implementado:

- `CControllerConfigManager::GetKeyIconCodeForAction(uint16)`: traduce la acción
  a **código de tecla de Windows** (el mismo con el que su `pcbtns.txd` nombra
  cada icono: "1" botón izquierdo, "8" retroceso, "13" intro, "16"
  mayúsculas, "27" esc, "32" espacio, "65".."90" letras, "96".."105" teclado
  numérico, "112".."123" F1..F12). El port guarda el **carácter** para las
  teclas imprimibles y las constantes `rs*` para el resto: `ViceExtKeyToVK`
  cubre impribibles + F1-F12 + edición + navegación + numérico + modificadores.
- `CMessages::InsertPlayerControlKeysInString` emite la marca `~K<vk>~` **sólo
  si** esa tecla tiene icono en el TXD; si no (rueda del ratón, mando, tecla
  sin icono), sigue escribiendo el nombre en texto como antes.
- `CFont`: reconoce `~K<vk>~` (cuidado: `~K~` ya era el botón L1 del mando, se
distingue por el dígito), crea el sprite del icono bajo demanda desde
  `models/pcbtns.txd` (128 texturas, cargado una vez al inicializar el HUD),
  lo cachea y lo pinta en el mismo hueco que los iconos de mando.
- **Cheat de diagnóstico `CRAZYHINT`**: muestra un aviso con 4 acciones
  (`PED_FIREWEAPON`, `PED_LOCK_TARGET`, `PED_JUMPING`, `VEHICLE_HORN`) para
  **ver** los iconos sin esperar a un aviso de misión, y deja la traza.
- Verificación por log: `KEYICONS txd=pcbtns cargado` al arrancar (ya **visto**
  en una sesión real) y `HINTKEY accion=<NOMBRE> vk=<n> icono=<1>` por aviso.

**Pendiente de confirmar:** teclear `CRAZYHINT` en partida y ver 4 iconos en el
aviso + las 4 líneas `HINTKEY ... icono=1`.

---

## Verificación sin sondas: `tools/viceext-log-check.py`

Lee `web/odtrace.log` y dicta D2 (nombres del HUD), D4, D5, D6 y D7 a partir de
las trazas de una partida real; **no lanza Chrome**. Se juega una vez y luego:

```bash
python gta_vc_browser/tools/viceext-log-check.py --desde-marca
```

Traza nueva para D2: `TXTMISS key=<clave>` en `CData::Search` cuando una clave de
texto no existe (el bug "streetfighter missing" se veía en pantalla; ahora
también en el log, y su ausencia es la prueba de que el HUD está completo).

Desde el build `ve11` el verificador entiende además
`CARLOAD`/`CARFAIL`/`CARZONE`/`CARRATE`/`ODSRECOVER` y, cuando no los encuentra
(partidas viejas), lo dice en vez de inventarse un veredicto: los bloques D4
informan de *cuántos de los 7 del mod eran alcanzables en la zona jugada,
cuántos se cargaron y cuántos se vieron*.

---

## D5 — `ped.ifp` del mod

Dato, no mecánica. El port sirve el `ped.ifp` vanilla (2.201.152 B) y el del mod
es un superconjunto (2.486.356 B; 272 clips ≥ 234, ningún nombre perdido —
verificado). Los límites ya están subidos (`NUMANIMATIONS` 512,
`NUMANIMBLOCKS` 40). Entra sin código **pero** sus clips nuevos
(nadar/escalar/apuntar) solo se usan cuando existan las mecánicas; lo que sí
cambia ya es el *feel* de las animaciones sustituidas.

**Tareas:** importar el `.ifp` (copiarlo a `streamed/anim/ped.ifp`), re-`stage`+
manifiesto, recompilar, pasar `slot0` + una captura de Tommy andando/corriendo y
comparar con la anterior; dejar constancia de si el *feel* mejora o empeora
(esa es la decisión: es reversible).

**PASS:** sonda base PASS + capturas comparadas + nota en `HISTORIAL`.

---

## D6 — Columna 27 de su `weapon.dat`: `weapon sight`

Su `weapon.dat` documenta una 27ª columna (`#\tc: weapon sight: 0 default,
1 dot, 2 pistol, 3 SMG, 4 shotgun, 5 rifle, 6 heavy, 7 rocket`) que nuestro
parser ignora. Es la única columna de datos del mod que no usamos y alimenta la
mira por arma. Si se implementa, se puede valorar meter su `weaponSights.txd`
(hoy excluido justo por esto: "arte de features que aquí no existen").

**Tareas:** leer la columna en `CWeaponInfo::LoadWeaponData`, guardarla en
`CWeaponInfo` (nuevo campo), usarla en el HUD para elegir la mira
(`Hud.cpp:373` tiene el caso `LASERSCOPE` como referencia), poner su
`weaponSights.txd` y comparar capturas apuntando con pistol/SMG/rifle.

**PASS:** captura con mira distinta por arma + `slot0` PASS.

---

## D7 — Iconos de teclado en los avisos (`pcbtns.txd`, v3.0)

Su v3.0: "PC key icons in game hints". Hoy `pcbtns.txd` **no está** en el port;
los avisos de misión usan texto (`CHud::SetHelpMessage`). Bloque pequeño:
importar su `pcbtns.txd`, ver si el motor ya tiene el camino para pintar iconos
(`BUTTON_ICONS` está activo, pero es para mando) y, si no, dejar constancia
medida de lo que costaría (probablemente no vale la pena: requiere decidir
dónde se pintan los iconos en el texto).

**PASS:** captura de un aviso con icono, o decisión documentada de no hacerlo
con el coste medido.

---

## D8 — Audio de oído (banco `ViceEx`, 13 muestras)

Su `ViceEx.SDT` tiene 13 entradas (pares L/R de 0,10 s a 1,06 s) **sin nombres**.
Para que el jugador pueda mapearlas hace falta poder **oírlas**: extraer cada
muestra a WAV (`ffmpeg`, como hace `split_sfx.py`) a una carpeta
`gta_vc_browser/tmp/viceex-samples/` + una plantilla
`viceex-map.tsv` (`nº → arma/sonido`) que el jugador rellene al escucharlas.
Cuando esté rellena, extender `split_sfx.py` para añadirlas al banco servido con
índices nuevos y engancharlas en `AudioLogic` a las armas nuevas.

**PASS:** muestras extraídas y escuchables + plantilla entregada al jugador (el
mapeo final lo valida él de oído).

---

## Estado (lo relleno yo)

- [x] **D1 cerrado** — `dataTag` y `VERSION` en `2026-09-19-ve8`, `stage_bootseed`
      + `gen_manifest` + relink hechos (build `ve8` en `web/public/build/`).
      Las secciones 2 y 3 pueden medir desde aquí.
- [x] **D2 cerrado** (falta el veredicto visual del jugador) — el problema no era
      sólo la clave: el campo 6 del IDE **sí** era el `gameName`, pero la
      "corrección" anterior había escrito la clave corta en el campo 7
      (`anims`), dejando el nombre largo en el 6 y rompiendo además el grupo de
      animación de las motos. Arreglado en el sitio correcto
      (`import_mvl_vehicles.py` ahora traduce el `gamename` del mod a clave GXT y
      emite `tools/gamename_keys.tsv`; el importador es idempotente). Las 6 claves
      nuevas están en los 6 GXT, verificadas contra los originales del mod
      (`perdidas=0`, array ordenado) y presentes ya en el `.data` de `ve8`.
      Checker nuevo: `tools/check_ide_gxt.py` (115 vehículos, verde; sólo informa
      de los huecos que ya traía el VC original: `AEROPL`, `RCGOBLI`).
- [x] **D2 verificable por log** — traza `TXTMISS key=<clave>` cuando una clave
      de texto no existe (el "streetfighter missing" se veía en pantalla; ahora
      también en el log y su ausencia es la prueba de que el HUD resuelve todos
      sus nombres). Falta sólo el veredicto visual del jugador al entrar en un
      coche nuevo.
- [ ] **D2-bis**: el buzón quedó abierto — `tools/gxt_inspect.py` tenía dos bugs
      seguidos (offsets de TDAT escritos al final de la cadena y lectura de la
      clave cortada a 4 bytes que hacía parecer corruptos ficheros sanos). Ahora
      el escritor es **incremental** (añade al final de TDAT, conserva TABL y la
      cola de 78 pares TKEY/TDAT que el motor no lee), **no toca claves que ya
      existen** sin `--force` y **se autocomprueba** releyendo el temporal
      (`self-test` incluido: claves de 1..7 caracteres).
- [x] **D3 cerrado** (PASS medido, build `d3i`) — la moto policial del mod
      (6507) tiene sirena *de sonido y de luz*: `CVehicle::UsesSiren()`
      reconocía el ID, `CBike` no, así que la moto no podía ni conmutarla desde
      el manillar ni parpadear. Portado el bloque de coronas de `CAutomobile` a
      `CBike` (dos luces `TYPE_STAR` en lo alto del manillar, rojo/azul
      alternando con el mismo temporizador de 1023/511 ms) detrás de
      `VICEEXT_POLICE_BIKE_LIGHTS`, más la máquina de estados del claxon
      (pulsación corta = sirena, mantener = pitar). El **coche** de policía no
      se toca: usa su `switch` de serie y sus texturas están todas.
      Evidencia de la corrida: `BSIREN` 35/35 con `on=1` (luz delante de la
      cámara), `CORONA` 9/9 texturas de corona cargadas, `CORONAREND`
      `dibujadas=0..2`, y el veredicto de píxeles: la ventana 620,11,740,131
      alterna rojo/azul (`azul=10,18,6,10  rojo=58,35,69,58  corr=-1.00`) y su
      **control apareado** (misma ventana, mismas capturas, sin los vehículos
      del mod) sale `0` celdas alternando. 0 errores de página.
- [x] **D4 confirmado en partida** (238 coches/29 modelos, 3 del mod en la
      calle, `polwintergreen` a 0; 146 ticks). Implementado con temporizador del
      cargador en **tiempo real** (11,7 s) en vez de
      350 fotogramas, puerta relajada para el cargador de tráfico (las
      condiciones de "streamer ocupado" lo dejaban cerrado minutos enteros: 0
      ticks y la calle congelada en 2 modelos), y diagnóstico `CARBLOCK` para
      saber cuál se cerró si vuelve a pasar. Medido: 32 coches/2 modelos antes →
      **367 coches/30 modelos** después, con 3 de los 7 del mod y la moto
      policial (clase `ignore`) a 0.
- [x] **D4 refuerzo temporal** — a petición del jugador, los 7 modelos del mod
      pasan a `frequency=100` (~10×) con `tools/boost_veh_freq.py`, reversible
      con `--revert`. Es dato precargado: exige stage + relink (`ve10`).
- [x] **D5 en el build `ve10`** — el `ped.ifp` servido *y empaquetado* es el del
      mod (2.486.356 B, md5 `c61a251f1b991661926838c2a5eeb896`, **272**
      animaciones declaradas y 272 chunks `DGAN`). La partida del 20/09 vio 234 porque corría el
      build anterior; el paquete de las 10:05 ya llevaba el del mod (verificado
      por los offsets de la entrada `/anim/ped.ifp`). Falta la confirmación en la
      próxima partida.
- [x] **Hallazgo: `dataTag` estaba COMENTADO** (bug de D1) — en
      `web/ondemand.js` el comentario y la propiedad quedaron en la **misma
      línea**, así que `OD.dataTag` era `undefined` (el log lo canta:
      `build=...ve10 data=undefined`) y la **purga de caché por versión de datos
      no funcionaba**. Arreglado (propiedad en su línea) y ya dentro del build de
      las 10:05.
- [x] **Hallazgo: la rotación de trazas borraba la partida anterior** —
      `web/lib/vite.js` truncaba `odtrace.log` en cada carga de página, así que
      al recargar se perdía la evidencia de lo jugado (pasó con la sesión del
      20/09, que quedó sólo menú). Ahora la copia a **`odtrace.prev.log`** antes
      de truncar, y el verificador avisa y señala ese fichero cuando el log no
      tiene marcas de juego.
- [x] **D6 implementado** — su `weapon.dat` (27 columnas, las 26 primeras
      alinean con el de serie) se lee entero: la columna 27 va a
      `CWeaponInfo::m_nSight` y el HUD pinta la mira del arma en curso desde su
      `weaponSights.txd` (7 texturas), con la cruz de serie como respaldo.
      Verificado en un arranque real: `SIGHTS cargadas=7/7`. Falta el veredicto
      visual apuntando.
- [x] **D7 implementado** — su v3.0 ("PC key icons in game hints") es
      data-driven: el GXT trae `~k~~ACCIÓN~` (29 acciones, 2.000+ usos) y su
      `pcbtns.txd` nombra los iconos con el código de tecla. Ahora la acción se
      traduce a código de tecla (`GetKeyIconCodeForAction`) y el aviso pinta el
      icono (`~K<vk>~` → `CFont`), con el nombre en texto si esa tecla no tiene
      icono. Cheat de diagnóstico `CRAZYHINT` para verlo sin misión. Verificado
      en un arranque real: `KEYICONS txd=pcbtns cargado`.
- [x] **D8 entregado** (espera el oído del jugador) — `tools/extract_viceex.py`
      saca las 13 muestras del banco propio del mod a
      `gta_vc_browser/tmp/viceex-samples/` (13 WAV + `viceex-map.tsv` para
      rellenar escuchando) y comprueba que los tamaños del `ViceEx.SDT` suman el
      `ViceEx.RAW` exacto (364.850 B = 364.850 B: el banco está completo, son 13
      entradas de 0,10 s a 1,07 s, en pares de igual tamaño y Hz). Falta el
      mapeo de oído → después, `split_sfx.py` + enganche en `AudioLogic`.

### Resumen por bloque (tras la partida del log `17:57–18:10Z`)

| Bloque | Qué está hecho | Qué falta |
|---|---|---|
| **D1** caché | `dataTag` arreglado (estaba comentado) y vivo: el log dice `data=2026-09-20-ve10` | nada; la etiqueta de datos la subo yo si tocáis `streamed/` |
| **D2** nombres del HUD | IDE, GXT (6 claves), checker `check_ide_gxt.py` y traza `TXTMISS` | sólo que lo *veas*: dentro de un coche nuevo el nombre debe salir limpio (si sale, está) |
| **D3** sirena de la moto policial | `UsesSiren` + coronas en `CBike` (rojo/azul) + claxon corto = sirena | veredicto visual cuando aparezca una moto policial (la última partida no vio ninguna) |
| **D4** tráfico | temporizador real, puerta relajada, `CARBLOCK`; **cargador ponderado por frecuencia** y trazas `CARLOAD`/`CARFAIL`/`CARZONE`/`CARRATE` | confirmar que ahora salen **los 7** (y en qué zonas: la clase `poorfamily`/`big` puede no sortearse en la zona jugada). El refuerzo `frequency=100` es **temporal**: se baja con `--revert` |
| **D5** `ped.ifp` del mod | fichero, paquete y traza; **confirmado en partida** (`clips=272`) | nada |
| **D6** mira por arma | columna 27, 7 texturas en el HUD, respaldo de serie; **confirmado en partida** (3 armas) | nada (opcional: apuntar con más armas para ver las 7 miras) |
| **D7** iconos de tecla | `~k~~ACCIÓN~` → icono de `pcbtns.txd`, respaldo en texto; **confirmado con un aviso real** | el cheat `CRAZYHINT` no se ha tecleado nunca: es la única pieza sin ver en pantalla |
| **D8** audio del mod | 13 WAV extraídos + plantilla `viceex-map.tsv` | **depende de tu oído**: rellenar el mapeo; después `split_sfx.py` + `AudioLogic` |
| **D18** cuelgue del GXT (21/09) | `CText::LoadMissionText` / `CText::Load` ya no pueden girar: se comprueba la apertura, la cabecera y el fin de fichero; `OdTextFileName()` con `default` (antes el nombre podía salir sin inicializar) | nada; **verificado en partida nueva** (`state=9`, 0 `TXTGXTFAIL`) |
| **D19** lectura corta | `CKeyArray::Load`/`CData::Load` devuelven los bytes leídos y la tabla sólo vale si entró entera (`tkey=864/864 tdat=5768/5768`); trazas `TXTGXTSHORT` y `MSGTABLE` | nada |
| **TABL** del GXT (raíz del 21/09) | `gxt_inspect.py` con `tabl`/`check-tabl`/`fix-tabl` y **`add`/`add-tsv` reajustan TABL solos**; los 6 GXT de `streamed/TEXT` reparados (+214 B por misión) | nada; si alguien vuelve a añadir claves con otra herramienta, `check-tabl` lo dice |

Extra fuera de los bloques, ya hecho: `ODSRECOVER` (prueba de que un aplazamiento del
`.img` se recupera) y el verificador `viceext-log-check.py` al día con todas las
trazas nuevas.

## D18/D19 — La pantalla negra del 21/09 (**raíz**: GXT + bucle sin salida)

**Qué pasó.** El SCM del mod llama al opcode 1356 (`COMMAND_LOAD_MISSION_TEXT`) al
arrancar la partida; `CText::LoadMissionText` abría el `.gxt` y lanzaba el bucle de
chunks **sin comprobar ni la apertura ni el fin de fichero**: con una cabecera
`size == 0` (o un descriptor inválido) el `while` releía la misma cabecera para
siempre. El motor dejaba de dibujar y la pestaña se quedaba en negro **sin un
solo mensaje**: eso es exactamente la queja "inicié una nueva y cargué una y se
queda en pantalla negra".

**Por qué la cabecera era mala.** Los 6 GXT que servimos traían los offsets de
misión de `TABL` desfasados **-214 bytes**. No es defecto del mod: su original
(`vice-extended-october-2025-update_.../GameFiles/ViceExtended/TEXT/*.gxt`) tiene
delta 0 en los 7 idiomas. Lo introdujimos nosotros con `gxt_inspect.py add`
(6 claves de nombre de vehículo: `HELLENB` `MANCHEZ` `PREMIER` `STRTFTR` `VCPDWTR`
`WINTERG`): TKEY creció 72 B (6×12) y TDAT 142 B = **214 B**, y los bloques de
misión (que van detrás) se movieron, pero `TABL` siguió apuntando a los offsets
viejos. El motor lee ahí 8 bytes y exige que sean el **nombre de la tabla**: en su
lugar hay texto UTF-16, así que `INTRO` nunca aparecía.

**Cómo queda garantizado.**

1. `ReadChunkHeader` devuelve los bytes leídos y los dos bucles (`CText::Load` y
   `CText::LoadMissionText`) salen si la cabecera es ilegible o `size == 0`.
2. `OpenFile` se comprueba: sin fichero, aviso (`CText::LoadMissionText - no se
   pudo leer <idioma>.GXT`) + `TXTGXTFAIL` en la traza, y se sigue jugando sin
   textos en vez de colgarse.
3. `CKeyArray::Load`/`CData::Load` devuelven los bytes leídos; una tabla a medias
   (lectura corta) ya no deja la cola del array con memoria sin inicializar — era
   el `INTRO4 missing` intermitente con el fichero correcto.
4. `tools/gxt_inspect.py`: `tabl` (lista los offsets), `check-tabl` (dice cuáles
   están desfasados y cuánto) y `fix-tabl` (los recalcula buscando el bloque real
   `NOMBRE + TKEY` y verifica). **`add`/`add-tsv` reajustan TABL solos** al
   reescribir TKEY/TDAT: era el origen del fallo.
5. `TXTMISS` se escribe sólo si la clave **no está en ninguna** de las dos tablas
   (global y de misión) y dice `nglobal=`/`nmision=`, así que ya no avisa de
   claves de misión que sí se resuelven.

**Verificación (perfil de Chrome limpio, partida nueva desde el menú):**
`WEBHB state=9 ingame=1`, `CORONAREND` ×8 (escena 3D dibujándose),
**0 `TXTGXTFAIL` / 0 `TXTGXTSHORT` / 0 `TXTMISS`** y
`MSGTABLE INTRO n=72 primera=INT1_A ultima=INTRO4 tkey=864/864 tdat=5768/5768`.
`ninja` completa 58/58 + enlace sin errores (el enlace ya no está bloqueado).
Pendiente de arnés (no de motor): `slot0-load-test.mjs` no llega a clicar
"CARGAR PARTIDA" — el motor se queda vivo a 60 FPS en `state=7`, sin cuelgue.
## D8/D4c/D10 — tanda del 21/09 (build `ve15`, datos `ve11`)

### D8 · los sonidos del mod, dentro (asignación PROVISIONAL)

| id | muestra | seg | Hz | par | arma asignada (provisional) |
|---|---|---|---|---|---|
| 9941 | viceex-00 | 0.102 | 36000 | 9942 | Desert Eagle |
| 9942 | viceex-01 | 0.102 | 36000 | 9941 | (segunda variante, libre) |
| 9943 | viceex-02 | 0.321 | 33000 | — | Uziold |
| 9944 | viceex-03 | 0.512 | 22000 | 9945 | Shotgun2 |
| 9945 | viceex-04 | 0.512 | 22000 | 9944 | (segunda variante, libre) |
| 9946 | viceex-05 | 1.004 | 32000 | 9947 | AK-47 |
| 9947 | viceex-06 | 1.004 | 32000 | 9946 | (segunda variante, libre) |
| 9948 | viceex-07 | 0.100 | 44100 | 9949 | Beretta |
| 9949 | viceex-08 | 0.100 | 44100 | 9948 | (segunda variante, libre) |
| 9950 | viceex-09 | 1.065 | 22050 | 9951 | Lanzagranadas |
| 9951 | viceex-10 | 1.065 | 22050 | 9950 | (segunda variante, libre) |
| 9952 | viceex-11 | 0.409 | 22050 | — | M16 |
| 9953 | viceex-12 | 0.584 | 22050 | — | Steyr (AUG) |

Cómo se corrige (sin tocar datos): la tabla muestra↔arma está **sólo** en el caso
de disparo de `src/audio/AudioLogic.cpp` (`SOUND_WEAPON_SHOT_FIRED`); se cambia el
número y se recompila. Cada disparo deja `VICEEX sfx arma=<tipo> sample=<id>` en la
traza (40 líneas máx.), así que la corrección se dicta leyendo el log.

Herramienta: `gta_vc_browser/tools/add_viceex_sfx.py` (idempotente; `--lista` enseña
la tabla sin tocar nada). Amplía `Audio/sfx.SDT` y escribe los 13 mp3.

### D4c · rehacido

Se medía mal: imprimía al cambiar de clase y la clase cambia casi en cada sorteo
(66.525 líneas de 87.116 en la partida del 21/09). Ahora: 12 primeras tiradas, la
primera aparición de cada clase y un recuento completo cada 4096 sorteos
(`CARRATECNT n=… c0=…`), que es el dato útil (la mezcla real de la calle).

### D10 · +20 m de modelo bueno antes del LOD

`#define VICEEXT_LOD_EXTRA 20.0f` en `src/core/config.h`, **sumado** en
`CSimpleModelInfo::GetLodDistance` / `GetNearDistance` / `GetLargestLodDistance`
(los tres sitios que usan a la vez el dibujado y la petición de modelos). Aviso:
sube el número de modelos "buenos" que el streaming debe mantener, así que si el
LOD pegado reaparece hay que mirar memoria (la traza `LODLEFT` ya existe).

### AUG (Steyr) apuntando a un lado — ABIERTO

Comprobado y descartado: el modelo `steyr.dff` es **byte a byte** el del mod
(16.384 B en su `weapons.img`); sus animaciones existen y con los nombres exactos
que usa el motor (`STEYR_fire/crouchfire/reload/crouchreload` en `steyr.ifp`, y el
grupo `steyr` de `AnimManager.cpp` las referencia así); su retroceso y su offset de
disparo son iguales a los de los otros rifles (`PlayerPed.cpp` PI/112 y el vector
de `weapon.dat` 0.7/-0.03/0.14 frente a 0.8/-0.04/0.17 del AK). Falta verlo: con
una captura apuntando se decide si es la pose (animación) o el eje del modelo.
