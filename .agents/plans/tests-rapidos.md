---
name: tests-rapidos
status: EXECUTED
type: maintenance
domain: process
owner_rules: .agents
created: 2026-09-22
---

# Pruebas rápidas y reutilizables (arnés `tools/harness/`)

Fecha: 22/09/2026 · build `ve36` · autor: agente (encargo: «más tests, reutilizables,
más blindados y más fáciles de hacer»)

## El problema que había

Nueve *smoke tests* (`tools/*-smoke-test.mjs`, ~5.500 líneas) y cuatro
verificadores (`*-log-check.*`). Cada smoke test repetía las mismas ~150 líneas
de arranque (lanzar Chrome, sembrar la partida, esperar el menú, pulsar el menú,
esperar la partida, leer `web/odtrace.log`) y tenía su propio perfil de Chrome.
Consecuencias medidas:

| | Antes | Ahora (arnés) |
|---|---|---|
| Coste de arrancar el motor | ~2 min **por fichero** | 36 s **una vez por corrida** (perfil caliente) |
| Pruebas que caben en una sesión | 1 | N (hoy 4; cada una 3-190 s) |
| Sincronía | `sleep(3000)` a ojo | espera por **evento de traza** (`FPHASE`, `WR load-req`, `swim enter`) |
| Saber sobre qué binario se midió | a mano | el arnés comprueba **versión + marcas + dataTag** al abrir |
| Trazas de dos sesiones a la vez | se MEZCLABAN en `odtrace.log` | **un fichero por sesión** (`?trace=<nombre>`) |
| Un fallo | capturas de todo | captura + **traza del escenario** + informe con la evidencia |
| Falsos fallos del verificador | 3 conocidos | corregidos (ver abajo) |

## Qué se copia de cómo se prueban los juegos de verdad

Tres prácticas estándar (Unreal *functional tests*, y las charlas de *Sea of
Thieves* sobre pruebas automatizadas de gameplay) que son las que más ahorran
aquí:

1. **Fijar el estado, no jugar la ruta.** Las pruebas se *teletransportan* a un
   punto de prueba en vez de recorrer el mundo. Aquí el equivalente es la
   **partida sembrada** (`tools/testdata/GTAVCsf1.b` → `/userfiles` vía IDBFS) y
   la **navegación por datos del motor**: `PEDAT ... mar=/dist=` dice el azimut y
   la distancia al agua, así que la sonda va hacia el mar en línea recta en lugar
   de barrer rumbos a ciegas (antes: 3,2 min y a veces se quedaba contra una
   casa; ahora el escenario de nado baja de ~190 s a lo que tarde en acercarse).
2. **Sincronizar por eventos, no por tiempos.** La traza del motor es la línea
   temporal: si no hay evento, no se sigue. Los `sleep` que quedan son los que
   son *físicos* (mantener una tecla, esperar una animación).
3. **Aserciones sobre telemetría, no sobre píxeles.** Todo se mide en la traza
   (posición, m/s, modo de cámara, peso de clip...). Las capturas se guardan
   **solo cuando algo falla**.

Y una cuarta, específica de wasm en navegador: **calentar la caché**. El motor
guarda datos en IndexedDB bajo el perfil de Chrome, así que el arnés usa **un
único perfil** (`vc-harness-profile`) para todas las suites: el primer arranque
de la máquina paga el precio y los siguientes son ~6 veces más rápidos.

## Cómo se usa

```bash
cd gta_vc_browser

# Lo del día a día: sin navegador, ~3 s
node tools/vc-test.mjs --estatico

# Todo, en UNA sola sesión de juego (los 4 escenarios)
node tools/vc-test.mjs

# Solo uno (depurando un arreglo)
node tools/vc-test.mjs --escenario nado

# Qué hay
node tools/vc-test.mjs --listar
```

`--estatico` comprueba, en ~3 s y sin arrancar el juego:
- el **binario servido**: versión con marca, `dataTag` y **todas** las marcas de
  lo implementado (`tools/check-served-build.sh`);
- los **defines que nacen apagados** en `src/core/config.h` (hoy
  `VICEEXT_TURN_SIGNALS`): se reutiliza la línea de compilación real de ninja y se
  compila ese fichero con el define, porque el build normal no lo mira;
- el **verificador** sobre el último log, diciendo **de qué build es el log y
  cuándo se escribió** (si el log es de otra sesión, se avisa en vez de suspender
  la suite por un log ajeno).

Los escenarios (suite con navegador):

| Escenario | Qué prueba | Coste |
|---|---|---|
| `carga` | la partida abre, el motor dibuja, el reloj va a tiempo real, la traza sale | ~3 s |
| `agachado` | clip del mod, avance real (m/s), medida fiable (`dt≈1`), clip baja la cámara, cámara sigue (modo 4), quieto no se arrastra, al desagacharse se retiran los clips, **y las 8 direcciones (R20): ángulo del mando, giro del cuerpo, dirección real del desplazamiento medida con `PEDAT`, clip por dirección y velocidad del clip en uso** | ~55 s |

### Cómo se mide «las 8 direcciones» (R20) sin depender del sitio ni del azar

El escenario (`tools/harness/esquemas.mjs`) pulsa las 8 direcciones (W, S, A, D y
las 4 diagonales, a dos teclas) ~2,6 s cada una y de cada tramo saca:

- `ang=` de la traza (`CROUCH2`): el ángulo del mando **en el sistema del cuerpo**,
  que es lo que lee el motor. Se comprueba que son los 8 ángulos que toca.
- **giro del cuerpo**: `rumbo` final menos el de «adelante». Con ratón en 3ª persona
  tiene que ser ~0 (el ped se desplaza SIN girar, como al caminar de pie).
- **dirección real del desplazamiento**: se recorre el tramo y se toma el
  desplazamiento entre la primera posición ya con el mando puesto y la última
  (`PEDAT x/y`), comparado con el `rumbo`. Tiene que valer `−ang` (±25°). Es la
  comprobación que caza el fallo real: el ped moviéndose hacia donde no se pide.
- **clip por dirección**: por NOMBRE (`nom=`), no por número (los IDs del `enum`
  cambian al reordenar grupos).
- **velocidad**: contra la natural del clip que se está usando, leída de un mapa
  `clip → m/s` medido en el `ped.ifp` (3,58 / 1,85 / 2,33 / 2,42), × el ritmo.

Dos detalles que produjeron falsos fallos y ya están resueltos: la **primera**
muestra del tramo todavía desliza de la dirección anterior (se descarta), y la
**última** puede caer en el deslizamiento posterior a soltar las teclas (`nom=`
`Crouch_Idle`, velocidad de frenada: se descarta).

Palancas que la prueba entiende: `VC_CROUCH_RATE=0.6` (ritmo) y `VC_CROUCH_SIDE=1`
(clips de costado), que espejan `?crouch=` y `?crouchlado=` del navegador.
| `armas` | el teclado de trucos responde (`CRAZYHINT`); el truco reparte las 8 armas del mod y la traza `WLOAD` dice, arma por arma, si el MODELO y el DICCIONARIO están en memoria; `WINFO` da la ficha de las 8 (clips que resuelve cada una); el motor no muere al disparar. El ciclo manual de armas queda como AVISO (la sonda no consigue cambiar de arma a mano) | ~35 s |
| `nado` | navega hasta el agua y comprueba: flote, velocidad del clip (2,32), cámara que **apunta a la superficie** (`objetivo ≈ nivel+0,5`), modo 4, sin cámara de ahogado, aguas abiertas tras girar 180° | 40-250 s |

## Añadir una prueba nueva (10 minutos, sin copiar arranques)

Un escenario es un objeto con nombre, marcas que exige del binario y una función
que hace los pasos y devuelve su informe:

```js
export const miMecanica = {
  nombre: 'mi-mecanica',
  titulo: 'una frase de qué prueba',
  marcas: ['LOQUESEA'],                  // el arnés falla si el motor servido no lo lleva
  async ejecutar(s) {
    const inf = informe('mi-mecanica', s);   // s = sesión ya EN PARTIDA
    const o = inf.abre('paso');              // marca de traza: todo lo de este paso
    await s.pulsa('KeyF', 120);              // entradas: pulsa/abajo/suelta/gira/cheat
    const txt = inf.cierra('paso');          // guarda el tramo como artefacto y lo devuelve
    const ls = lineasDe(txt, 'MITRAZA');
    inf.check('la mecánica responde', ls.length > 0, ls[0] || 'sin traza');
    inf.noError();
    return inf;
  },
};
```

Y se añade a `ESQUEMAS` en `tools/harness/esquemas.mjs`. El orden importa: los
escenarios comparten la MISMA partida (por eso `armas` va antes que `nado`: en el
agua el ped no lleva armas en mano).

Categorías del informe: **PASS**, **FALLO** y **AVISO**. El aviso es para lo que
la sonda NO ha podido comprobar (p. ej. el ciclo de armas con el teclado
numérico): así no se disimula un «ok» que no se ha medido ni se suspende la suite
por una limitación del arnés.

## Lo que se blindó (y por qué)

- **Se verifica el binario**: cada escenario declara las marcas que necesita y el
  arnés comprueba versión/marcas antes de medir. Nos mordió en la 13ª partida
  (una sonda midió un wasm viejo).
- **Trazas por sesión** (`/odtrace/<nombre>` → `odtrace-<nombre>.log`, con su
  propia rotación): antes todas las sesiones escribían en el mismo fichero y dos
  pruebas a la vez —o la partida del jugador y una sonda— se mezclaban.
- **Topes de tiempo en cada llamada al navegador** (`conTiempo`) y topes por fase
  (menú 150 s, partida 240 s): una pestaña atascada falla con motivo en vez de
  quedarse minutos en silencio (le pasó a la primera corrida del arnés, con la
  CPU compartida con la partida del jugador). Si el motor no llega al menú o al
  juego, se guarda captura y se dice el estado.
- **Perfil bloqueado** por un cierre brusco: se borra `Singleton*` y se
  reintenta (si no, el siguiente arranque muere con «browser is already running»).
- **Aviso de sesión concurrente**: si hay otra sesión escribiendo trazas ahora
  mismo (el jugador probando), se avisa de que la CPU va compartida.
- **Artefactos**: `%TEMP%\vc-tests\<suite>-<fecha>\escenarios\` con la traza de
  cada paso y las capturas de los fallos; el informe imprime la ruta.
- **Los avisos de medida son parte de la prueba**: el escenario `agachado`
  comprueba que el tiempo de mundo y el del motor coinciden (`dt≈1,00`) porque un
  contador doblado hacía que la velocidad saliera a la mitad (falso fallo del
  22/09). Una prueba que se cree a sí misma sin comprobarlo miente.

## Falsos fallos del verificador que se corrigieron al montar esto

1. **R17 «agachado PARADO el ped se mueve»**: la primera muestra tras soltar el
   palo es la **frenada** (`0,4-0,8 m/s`) y la primera tras agacharse lleva la
   velocidad que traía de pie. Ahora solo cuentan las muestras de estado estable
   (el palo en la misma posición que en la muestra anterior).
2. **R17 no veía las líneas de nado**: su expresión regular leía `SWIM2` por
   POSICIÓN de campos, y se habían añadido campos (`velo`, `dt`, `dz`). Ahora se
   leen por nombre.
3. **H1 «la cámara no baja agachado (11,18 vs 11,33)»**: comparaba la `camz`
   ABSOLUTA de agachado con la de pie de toda la sesión, que son **dos sitios
   distintos** (el jugador jugando en Ocean Beach). Ahora se mide **cámara sobre
   el ped** (`camz − z` de pie con la traza `PEDAT`, `camz − h` agachado con
   `CROUCH2`), que no depende de dónde esté. Medido en la suite: **1,58 m de pie
   → 0,65 m agachado**.
4. **`armas` «TXD vistos: ninguno»**: se deducía de las líneas `TXDIN`, y esa
   instrumentación (diagnóstico de streaming F3a) está **acotada a las 60
   primeras del proceso**: en una sesión larga el arranque ya se las había
   comido, así que la prueba salía en rojo con el truco funcionando. En vez de
   subir el tope a ciegas se midió lo que hacía falta de verdad: el truco publica
   ahora `WLOAD` (modelo y diccionario de cada arma, con el estado del streaming)
   y la ficha `WINFO` de las 8 armas (`ViceExtWeaponInfoOf`), sin depender de que
   la sonda sepa cambiarlas de mano. Es el patrón: **si una prueba no puede ver
   lo que mide, se instrumenta el hecho, no se ablanda la prueba**.
5. **`nado` «no se usa la cámara de ahogado — SIN traza»**: la línea
   `SWIMCAM no-fallen-water` sale **una vez por sesión**, en el frame en que el
   motor decide la cámara del jugador vivo en el agua — y con estas partidas
   cae al CARGAR (el ped ya está en el agua), antes del tramo de nado. La prueba
   buscaba sólo en su tramo. Ahora la línea se busca en la sesión entera y,
   además, el tramo de nado exige que **ninguna** línea anuncie `modo=23`
   (cámara de ahogado) y que el objetivo de cámara esté en la superficie.
6. **R3 «texto con la clave VACÍA»**: son las llamadas de serie
   `CHud::SetHelpMessage(nil, ...)` que VACÍAN el cajón de ayuda
   (`Script.cpp:409`, `Script4.cpp:2198`, `Pickups.cpp:575`). Se cuentan aparte.

Los cuatro eran ruido que hacía desconfiar del verificador; un verificador que da
falsos positivos se ignora, y entonces no sirve.

## Los 9 `*-smoke-test.mjs` antiguos: qué hacer con ellos

Siguen funcionando y **no se borran** (cada uno tiene su arranque de ~150 líneas,
pero también sus pasos exactos y su historial). Regla desde hoy:

- **Feature nueva → escenario del arnés**, no un smoke test nuevo.
- Un smoke test se **migra cuando se toca**: se copian sus pasos a un esquema y se
  borra el fichero (los que ya están migrados en espíritu: `weapons` → `armas`,
  `crouch-swim` → `agachado` + `nado`).
- Un smoke test que se deje sin migrar queda como **sonda puntual** (con capturas)
  para un caso raro, no como parte del bucle del día a día: el bucle es
  `vc-test.mjs --estatico` (2,6 s) y `--todo` (~6 min, una sola sesión).

## Pendientes (por orden de valor)

1. **Tecla real del ciclo de armas**: la sonda no logra cambiar de arma con
   `NumpadDecimal` (puede ser NumLock o el remapeo del mod). Con la tecla del
   jugador, el escenario `armas` pasaría de 2 avisos a medir `WINFO` y clips por
   arma. Los avisos ya lo dicen sin disimularlo.
2. **Guiones de teclas grabados** (`?replay`): grabar una secuencia y reproducirla
   daría escenarios *idénticos* entre corridas (hoy la navegación converge, pero
   no es la misma ruta).
3. **Escenarios que faltan de lo ya implementado**: primera persona, apuntado,
   recarga a mano, autoguardado, depósito de gasolina, luces de servicio,
   drive-by ampliado, sirenas, esconderse de la policía. Cada uno es un objeto
   como el de arriba; la traza que los mide ya existe (bloques R5/R9/R11/R12/...).
4. **Un `--paralelo 2`**: con trazas por sesión y perfiles separados ya es
   posible correr dos suites a la vez; falta el lanzador y medir si la CPU da
   para ambas (el motor pide ~1 núcleo por sesión).
5. **Suite estática en el build**: que `build.sh` llame a `--estatico` al terminar
   y avise si la marca de lo implementado no llegó al paquete servido.
