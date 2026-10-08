---
name: recoil-trazas-ve77
status: EXECUTED
type: maintenance
domain: gameplay
owner_rules: .agents
created: 2026-09-26
---

# Plan ve77 — Alinear trazas del recoil persistente

## Evidencia de partida

- Log: `gta_vc_browser/logs/odtrace-2026-09-26_13-47-21.log`.
- La cabecera identifica `JS build=2026-09-25-ve76 data=2026-09-21-ve13`.
- Durante disparos, `RECOIL2 alpha` sube desde `0.0048` hasta `0.1000 rad` (`5.73°`), informa `ret=0.00` y permanece en ese límite en muestras posteriores.
- `RECOIL2 kick` confirma disparos con pistola (slot 3), rifle (6), SMG (5) y escopeta (4); `RECOIL3 miracheck` y `multY` permanecen en `0.400`.
- La discrepancia: mientras el `alpha` de Cam acumula y persiste, `pitch` de `RECOIL2 kick` suele repetir sólo la patada de una bala. El espejo de `Weapon.cpp` se resta a `0.07 * dt` cada frame y además se reinicia al cambiar de arma; ninguna de esas dos reglas representa ve76.

## Objetivo y cambios

1. Mantener `s_odRecoilAlpha` de `src/core/Cam.cpp` como única fuente del offset persistente de cámara.
2. Hacer que el valor consultado por `RECOIL2 kick` lea ese offset real; retirar el acumulador espejo, su decaimiento y su reset por cambio de arma de `src/weapons/Weapon.cpp`.
3. Etiquetar explícitamente el offset real y si la patada se aplicó a pie en la traza por disparo; estandarizar la traza de cámara como `RECOIL2 offset ... reset=0|1` y conservar `RECOIL3` de mira fija.
4. Corregir comentarios de recoil que aún afirman retorno automático o reset por cambio de arma. Actualizar el verificador de build para exigir el nuevo formato de traza.
5. No cambiar magnitudes/física. Mantener `dataTag=2026-09-21-ve13`; asignar `VERSION` ve77 sólo al preparar/enlazar el nuevo build para que el tag no anuncie un wasm viejo.

## Verificación

- Revisar los cambios propios y comprobar referencias antiguas al espejo/retorno en los bloques de recoil.
- No ejecutar Chrome/SwiftShader headless.
- Antes de enlazar o servir un wasm nuevo, comprobar que el directorio `gta_vc_browser/web/public/build` no esté siendo usado por otra sesión; dejar build y partida de confirmación al siguiente paso si los temporales siguen bloqueados.
- Criterio en la siguiente traza: tras cada disparo `offset` debe corresponder al acumulado de Cam hasta `0.1000`, no decaer al soltar/cambiar arma; `a_pie=0` identifica drive-by sin añadir patada al offset a pie; `multY=0.400` permanece intacto. La traza periódica de Cam debe decir `reset=0`, y `reset=1` sólo cuando se limpia un offset no nulo.

## Estado de esta pasada

- Fuentes alineadas: `ViceExtRecoilOffset()` expone `s_odRecoilAlpha`; el espejo `s_odRecoilPitch`, su decaimiento y el reset al cambiar de arma se retiraron. `RECOIL2 kick` informa `a_pie`, `offset` y grados; `Cam.cpp` etiqueta `reset=0/1`.
- Comentarios del recoil en `Weapon.cpp`/`.h` alineados a ve76, y el verificador exige las tres firmas nuevas (offset periódico, reset y disparo).
- No se cambió `VERSION` a ve77 ni se enlazó wasm. Pasan `bash -n`, `git diff --check` (archivos revisados) y `check-served-build.sh --listar`; no se verificó el binario ni se compiló porque los `.wasm.tmp*` de la carpeta de build siguen compartidos/bloqueados. El build y la prueba en juego siguen pendientes.
