---
name: camara-libre-autocentrado
status: EXECUTED
type: feature
domain: camera
owner_rules: .agents
created: 2026-09-19
---

# Plan — Cámara libre: mirar con el ratón y volver sola (auto-centrado real)

> **REVERTIDO (2026-09-19, build `2026-09-19-orig1`).** El jugador lo probó y
> no le gustó: se restauraron `src/core/Cam.cpp` y `src/renderer/Hud.cpp` a
> HEAD (`git checkout --`), así que la cámara vuelve a quedarse donde la dejas
> y la retícula PC vuelve a salir siempre con arma de fuego en 3ª persona.
> Se conserva este documento como registro: el ruido sub-píxel del ratón y el
> criterio de coherencia de signo siguen siendo válidos si algún día se
> readdressa con el jugador de acuerdo. **No reintroducir sin preguntar.**

Petición del jugador (2026-09-19, build `cam4`):
> si muevo el mouse debe dejármela mover pa' donde me dé la gana; si dejo de
> mover, que vuelva a como si la opción de cámara libre estuviera off.

Es decir: **look libre con el ratón (los dos ejes) + retorno automático cuando
el ratón se queda quieto**, en todas las cámaras donde el ratón mira (a pie y
en coche), tenga la "cámara libre" del menú encendida o apagada.

## Diagnóstico (con dato, no a ojo)

Sonda headless `vc-e2e/cam-pad.mjs` (carga slot 0, mueve el ratón en círculos
8 s y lo deja quieto 7 s) sobre el build `cam4`:

- 98 líneas `CAMPD` (a pie) y 137 `CAMSA` (coche), y **`idle=0.00` en las 98 /
  137** — incluso en los 7 s con el ratón totalmente quieto.
- `yaw=0.000` siempre en `CAMSA`; `mx=-0.00` (cero negativo: deltas sub-píxel).

Conclusión: **el reloj de quietud nunca acumula**, así que el auto-centrado
(que sí está escrito y converge bien) **no dispara jamás**. No es el ajuste, ni
el modo de cámara, ni la sensibilidad: es el criterio de "estoy mirando".

### Causa raíz (una sola, en tres sitios)

`src/core/Cam.cpp` reinicia el reloj de quietud con comparaciones contra el
delta **crudo** del ratón:

1. **A pie** (`Process_FollowPedWithMouse`):
   ```c
   bool mouseReal = Max(Abs(MouseX),Abs(MouseY)) * dt60 >= 1.5f;
   if(mouseReal || LookLeftRight != 0.0f || LookUpDown != 0.0f) lookIdle = 0.0f;
   ```
   `LookLeftRight`/`LookUpDown` **vienen del propio ratón** cuando `UseMouse`
   está activo (`-2.5f*MouseX`), así que **cualquier delta, aunque sea 1e-6,
   los deja `!= 0`** y reinicia el reloj en CADA frame. El umbral de 1.5 px
   (`mouseReal`) queda anulado por la cláusula siguiente.

2. **Coche, cámara por defecto** (`Process_Cam_On_A_String`): el bloque de
   mirada se ejecuta bajo `if(mx != 0.0f || my != 0.0f)` — el mismo problema,
   sin umbral alguno. Un ratón apoyado manda sub-píxeles casi cada frame ⇒
   `lookIdle` a 0 siempre.

3. **Coche, cámara libre** (`Process_FollowCar_SA`): aquí el umbral sí se
   aplicó en `cam4` (`Max(|mx|,|my|)*dt60 >= 1.5f`) ⇒ es el único camino donde
   el retorno puede llegar a disparar. Se unifica el criterio.

Un ratón real (y `glfwGetCursorPos` en web, que devuelve fracciones de píxel)
**casi nunca manda exactamente 0**: manda ruido sub-píxel. Medir "¿está
mirando?" contra `!= 0` es, por tanto, medir "¿el ratón existe?".

## Arreglo

1. **Detector de "¿está mirando?" por VENTANA** (`LookMouseActivity`, ventana
   ~1 s vía filtro exponencial `Pow(0.35, dt)`, independiente de los fps):
   filtra el desplazamiento **con signo** y devuelve `Max(|Σx|,|Σy|)`, que se
   compara con `fLookMouseMovePx = 1.5` px de recorrido **neto**.
   - Ratón apoyado: el ruido sub-píxel va y viene ⇒ la suma se queda en ~0 ⇒ el
     reloj de quietud acumula y el retorno **sí** dispara.
   - Giro intencionado, aunque sea lento: es coherente ⇒ la suma sube en ~0,1 s
     ⇒ el retorno **no pelea** con él.
   Un umbral por frame no sirve para esto último: a 60 fps un giro lento también
   es sub-píxel por frame, y el auto-centrado tiraría en contra (se vería como
   "la cámara no se deja mover").
2. La **rotación** sigue gobernada por cualquier delta `!= 0` (apuntado fino
   intacto): el detector solo decide si el **reloj de quietud** se reinicia.
3. A pie: cuenta ratón (por ventana) **o** cruceta/mando real
   (`CPad::LookAroundLeftRight/UpDown`), nunca el valor derivado del ratón.
4. Coche por defecto: el bloque de mirada pasa de `if(mx != 0 || my != 0)` a
   separar reloj (ventana) y rotación; `Process_FollowCar_SA` igual.
5. Traza `cam recentrada pie` (a pie no había ninguna): el log dice si el
   retorno ocurrió de verdad, sin volver a medir el build.

Nada más se toca: el vanilla (`Process_FollowPed`, cámaras de misión, cinemática)
sigue igual, y con el ratón quieto el comportamiento es el de siempre.

## Verificación

- `ninja` incremental + relink (los cambios son solo `Cam.cpp`).
- Sonda `cam-pad.mjs` en headless: en los 7 s de quietud debe verse
  `idle` subiendo por encima de 2-3 s y `cam recentrada` en el log. Antes de
  jugar, comprobar que el `tag` del build es el nuevo.
  **Hecho** (build `2026-09-19-cam5`): `idle` a 0.00 moviendo y subiendo
  (26 → 281) con el ratón quieto, **3 × `cam recentrada pie`**, 0 errores.
  Los dos caminos de coche no los ejercita la sonda (nunca entra en un
  vehículo): quedan para la prueba del jugador.
- Prueba del jugador: mover el ratón a pie y en coche (los dos ejes) y soltarlo
  ⇒ vuelve sola detrás del objetivo.

## Pendiente / dudas

- Retícula: en VC PC con "cámara con ratón" la retícula se dibuja **siempre**
  que lleve un arma de fuego en 3ª persona (`Hud.cpp`, `DrawCrossHairPC`), no
  solo al apuntar. Es vanilla, no un bug del port; se confirma al jugador y se
  ofrece cambiarlo si quiere "solo al apuntar".
- Tiempos: a pie 3 s, en coche 2 s (`fLookRecenterDelay` /
  `fLookRecenterDelayCar`). Un valor y una constante cada uno si se quiere más
  corto (SA percibe ~1 s).
