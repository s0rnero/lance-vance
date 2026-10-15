# Ronda R3 · medidas sacadas de `ViceEx.exe` (14/10/2026)

Que es esto: el recibo de las medidas que desbloquean las dos etiquetas vacias que
se resistian (**luces lejanas**, familia 5, y **sensibilidad de apuntado**,
familia 1). Antes de esta ronda el proyecto solo tenia medida la *fontaneria* del
INI de ambas; el dato que faltaba era **quien consume** su bandera, y eso es lo que
hay aqui.

## Como se mide (esto es lo que estaba mal planteado antes)

El cliente MCP de REA tiene un corte de ~60 s y `xrefs` **expira siempre** por esa
via (comprobado dos veces hoy, `MCP error -32001`). Lo que funciona es
`analyzeHeadless` **directo** con el proyecto ya analizado y sin re-analizar
(`-noanalysis`), que responde en segundos porque el proyecto de Ghidra ya existe:

```bash
export GHIDRA_INSTALL_DIR='C:\ghidra_12.1.4_PUBLIC'
R='C:\Users\s0rno\OneDrive\Documents\lance-vance\gta_vc_browser\tmp\rea'
/c/ghidra_12.1.4_PUBLIC/support/analyzeHeadless.bat "$R\ghidra_r2" viceex_r2 \
  -process ViceEx.exe -noanalysis -scriptPath "$R\ghidra_scripts" \
  -postScript R2Dump.java xref   "$R\salida.txt" 0xa0627f 0x93762c \
  -postScript R2Dump.java decomp "$R\salida.txt" 0x49b290 0x50ba20 \
  -postScript R2Dump.java dwords "$R\salida.txt" 0x6e0c74:4
```

Modos de `R2Dump.java`: `func|decomp|strings|xref|sym|dwords|pstrs|range|ctx`.
Los recibos crudos de esta ronda estan en `gta_vc_browser/tmp/rea/r3-*.txt` y el
binario es el mismo de siempre (sha256 `4df2726e…c48b`, PE32 x86, 3.822.080 B).

## Familia 5 · Luces lejanas (`EnableDistantLights`, v2.6)

| Que | Dato medido |
|---|---|
| Clave del INI | `EnableDistantLights` @ `0x6eaa50` (unico string con "Distant" en todo el binario) |
| Quien la lee | stub `0x406850` → lector `FUN_0051e3c0` |
| Bandera | byte `[0xa0627f]`, escrita en `0x406860` (`SETNZ`) |
| **Quien la consume** | `CMP byte[0xa0627f],0` en `0x50ba37` (dentro de `FUN_0050ba20`, 2698 B) y en `0x4e72bd` (`FUN_004e7030`) |
| Que hace al estar puesta | recorre **dos pools** de entidades y llama a una funcion por objeto: `FUN_005282f0` |
| Pool A | `0x93839c`, objetos de **112 B** |
| Pool B | `0x938390`, objetos de **100 B** |
| Identificacion de los pools | por el **orden de creacion** de `FUN_004f0a60` (el `CPools::Initialise` del mod) contra el orden de `src/core/Pools.cpp` del port: 12 B = PtrNode, 20 B = EntryInfoNode, 0x784 = **Ped**, 0x828 = **Vehicle**, 100 B = **Building**, 100 B = Treadable, 0x1a0 = Object, **0x70 = Dummy**, 0x14 = AudioScriptObject, 0x4c = ColModel. O sea: **dummies** y **edificios** |
| Por objeto (`FUN_005282f0`) | mira la tabla de modelos (`0xa8b7e0[modelo]+0x1a`), la matriz (`+4`), banderas en `+0x5a`, modelo en `+0x5c`; calcula distancia y la compara con la constante `[0x6e0c74]` = **530.0** (vecinas: `0x6e0c78` = 570.0, `0x6e0c7c` = 1650.0); decide "encendida" con la hora (`DAT_0092c948 < 7` / `0x12 <`) y parpadeo/emergencia con `DAT_00987dd0` |
| Coherencia | los dummies son justo lo que lleva las luces del mapa: `FUN_0054b7f0` registra `trafficlight1`, `bollardlight`, `od_lightbeam`, `bryx_lights`, `lodbryx_lights` |

**Lo que sigue faltando para portarlo (y por eso NO se ha escrito codigo):** el
recorrido esta medido, el **efecto visual no**. Falta (a) la linea base vanilla —que
parte de ese recorrido ya existe en VC y que parte anade el mod— y (b) que magnitud
cambia en pantalla (cuanta mas distancia, que luces). Comparar contra un `gta-vc.exe`
vanilla por la misma via es el siguiente paso; sin eso, cualquier port seria una
suposicion, que es exactamente lo que el jugador ve como "todo vanilla".

## Familia 1 · Sensibilidad de apuntado (2506 «Aim sensitivity fix»)

El mod usa **sensibilidades separadas para mirar y para apuntar**, por dispositivo,
y elige el par segun el estado de apuntado del jugador (`FUN_0049b290`, que lee el
ped del jugador y mira su byte `+0x77d` y su estado `+0x244`).

| Clave del INI (`[Controller]`) | Global | Valor real (su `ViceEx.ini`) |
|---|---|---|
| `PadLookSensX` | `0x93760c` | 0.001250 |
| `PadAimSensX` | `0x937610` | 0.000625 |
| `PadLookSensY` | `0x937614` | 0.001250 |
| `PadAimSensY` | `0x937618` | 0.000625 |
| `MouseLookSensX` | `0x937628` | 0.002500 |
| `MouseAimSensX` | `0x93762c` | 0.001667 |
| `MouseLookSensY` | `0x937630` | 0.002500 |
| `MouseAimSensY` | `0x937634` | 0.001667 |
| `Autoaim` | `0x937624` | 0 |

- Lectoras: `FUN_0051cab0` y `FUN_0051f8a0` (su INI). Consumidora: `FUN_0049b290`.
- El raton se escala por **0.5** al aplicarlo (constante `0x6d7778` = `0x3f000000`).
  Comprobacion cruzada: `MouseLookSensX` 0.0025 x 0.5 = **0.00125**, que es
  exactamente el `HorizantalMouseSens` que ya usa el port. O sea: el port ya usa hoy
  el valor de **mirar**, y apuntando el mod pasa a **2/3** (raton) y **1/2** (pad).
- Los valores no estan compilados en el exe (`.data` a cero): salen de su
  `ViceEx.ini`, y los de la tabla son los de la **instalacion del jugador**, leida el
  14/10 (solo lectura).

**Estado:** portado en el motor en la tanda `ve80` (bandera `VICEEXT_AIM_SENSITIVITY`,
traza `P4 kind=aimsens`, bloque `AW`). Ver el plan 12.0 para la verificacion.
