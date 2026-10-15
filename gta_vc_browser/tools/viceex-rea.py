#!/usr/bin/env python3
"""Camino de analisis REA (rea-agents 6.4.0 + Ghidra) para `mods/extended/GameFiles/ViceEx.exe`.

Para que: la familia 13 del mod (los 10 opcodes propios `0x0FA0`-`0x0FA9`) se
cierra leyendo su `Script8.cpp` compilado, y para eso hace falta pseudocodigo,
no solo desensamblado. El camino es REA (`rea-agents`) con el proveedor de
Ghidra 12.1.4 ya instalado en esta maquina. Lo que rompia el trabajo era el
**orden** de las llamadas y el **tiempo**, no el proveedor:

  * el servidor MCP de REA arranca y lista sus 170 herramientas en ~5 s, pero
    cualquier operacion ligada a Ghidra exige que el programa este analizado
    **entero** antes de responder (asi lo declara el propio proveedor);
  * un analisis en frio de `ViceEx.exe` (3,8 MB) tarda minutos; si se pide
    desde el cliente MCP, el cliente corta antes (~60 s), la peticion se
    cancela y **se descarta la base de datos efimera**: de ahi el
    "Connection cancelled" de hoy, que no era una averia del servidor;
  * la salida es hacer el trabajo caro **una vez por CLI** (`--snapshot`
    escribe/actualiza un snapshot de analisis atado al `sha256` del binario y
    al perfil del proveedor) y en MCP **abrir siempre el binario primero**, con
    `open_binary {path, provider_id, snapshot_path}`, que importa ese snapshot
    de forma atomica; despues las consultas por direccion ya no arrancan nada.

Uso:
    python tools/viceex-rea.py estado                 # proveedores + doctor MCP (rapido)
    python tools/viceex-rea.py snapshot [--forzar]    # asegura el snapshot del exe
    python tools/viceex-rea.py dec 0x4948B6 [--sin-snapshot] [--timeout 900]
    python tools/viceex-rea.py mcp-check [--abrir] [--timeout 240]
    python tools/viceex-rea.py todo                   # estado + mcp-check

Recibos en `gta_vc_browser/tmp/rea/` (`viceex-13-*.json` / `.out`). Exit 0 =
todo lo pedido salio bien; exit 1 = algo fallo (se imprime que).

Variables de entorno reconocidas:
    VC_VICEEX_EXE     ruta del binario (por defecto el del repo)
    VC_REA_SNAPSHOT   ruta del snapshot (por defecto tmp/rea/viceex-13-snapshot.json)
    VC_REA_PKG        paquete/version de REA (por defecto rea-agents@6.4.0)
"""
import argparse
import json
import os
import queue
import shutil
import subprocess
import sys
import threading
import time

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))   # gta_vc_browser/
REPO = os.path.dirname(RAIZ)                                        # raiz del port
RECIBOS = os.path.join(RAIZ, "tmp", "rea")
# REA exige rutas absolutas (su contrato lo valida), asi que se normalizan aqui
# aunque vengan por variable de entorno.
EXE = os.path.abspath(os.environ.get(
    "VC_VICEEX_EXE", os.path.join(REPO, "mods", "extended", "GameFiles", "ViceEx.exe")))
SNAPSHOT = os.path.abspath(os.environ.get(
    "VC_REA_SNAPSHOT", os.path.join(RECIBOS, "viceex-13-snapshot.json")))
PAQUETE = os.environ.get("VC_REA_PKG", "rea-agents@6.4.0")
MCP_JSON = os.path.join(REPO, ".agents", "mcp.json")

# Direcciones utiles de la familia 13 (medidas en tools/viceex-customops.py):
DIRS = {
    "0x0fa2": 0x4948B6,   # handler del opcode 0x0FA2 (el mas usado: 91 sitios)
    "callee": 0x4F83D0,   # a donde llama 0x0FA2; su global es lo que falta
    "0x0fa3": 0x4948F2,   # DoAutoSave
    "0x0fa9": 0x4949BF,   # grados -> radianes sobre un vehiculo
}


def recibos(nombre):
    os.makedirs(RECIBOS, exist_ok=True)
    return os.path.join(RECIBOS, nombre)


def escribe_recibo(nombre, datos):
    ruta = recibos(nombre)
    with open(ruta, "w", encoding="utf-8", newline="\n") as fh:
        if isinstance(datos, str):
            fh.write(datos)
        else:
            json.dump(datos, fh, indent=2, ensure_ascii=False, sort_keys=True)
            fh.write("\n")
    return ruta


def ejecuta(args, timeout, etiqueta):
    """Lanza la CLI de REA una vez y devuelve (ok, salida, segundos, error)."""
    cmd = ["npx", "-y", PAQUETE] + args
    print(f"  $ {' '.join(cmd)}", flush=True)
    t0 = time.monotonic()
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8",
                           errors="replace", timeout=timeout, shell=(os.name == "nt"))
    except subprocess.TimeoutExpired:
        return False, "", time.monotonic() - t0, f"timeout propio de {etiqueta} ({timeout}s)"
    seg = time.monotonic() - t0
    if p.returncode != 0:
        # REA informa de sus fallos en el sobre JSON de stdout, no en stderr.
        detalle = (p.stderr or "").strip() or (p.stdout or "").strip()
        return False, p.stdout or "", seg, (detalle[:400] or f"exit {p.returncode}")
    return True, p.stdout or "", seg, ""


def sha256(ruta):
    import hashlib
    h = hashlib.sha256()
    with open(ruta, "rb") as fh:
        for bloque in iter(lambda: fh.read(1 << 20), b""):
            h.update(bloque)
    return h.hexdigest()


def lee_snapshot():
    """Devuelve (sha256 del target, proveedor, digest del perfil) o None si no hay."""
    if not os.path.exists(SNAPSHOT):
        return None
    try:
        with open(SNAPSHOT, encoding="utf-8") as fh:
            d = json.load(fh)
    except Exception as exc:
        print(f"  snapshot ilegible: {exc}")
        return None
    t = d.get("target", {})
    b = d.get("binding", {})
    return (t.get("sha256"), (b.get("provider") or {}).get("id"),
            (b.get("analysis_profile") or {}).get("digest"),
            len(d.get("entries", [])), len(d.get("workflow_entries", [])))


# --------------------------------------------------------------------------- estado
def cmd_estado(args):
    print("== estado del camino REA ==")
    ok = True
    datos = {}

    ok_p, salida, seg, err = ejecuta(["providers", "--format", "json"], 180, "providers")
    ghidra = None
    if ok_p:
        try:
            d = json.loads(salida)
            for cand in d.get("analysis_provider_candidates", []):
                if (cand.get("provider") or {}).get("id") == "ghidra":
                    ghidra = cand
        except Exception as exc:
            ok, err = False, f"providers no devolvio JSON: {exc}"
    if ghidra is None:
        print(f"  Ghidra: NO localizado ({err})")
        ok = False
    else:
        dis = ghidra.get("availability", {}).get("diagnostics", {})
        print(f"  Ghidra {dis.get('provider_version')}: {dis.get('install_dir')}")
        print(f"    java {dis.get('java_version')} ({dis.get('java_home')})")
        print(f"    decompilador nativo: {os.path.basename(dis.get('native_decompiler_path') or '?')}")
        malas = [c["name"] for c in dis.get("checks", []) if not c.get("ok")]
        print(f"    checks: {len(dis.get('checks', [])) - len(malas)}/{len(dis.get('checks', []))} OK"
              + (f"  FALLAN: {malas}" if malas else ""))
        ok = ok and not malas
    datos["providers"] = {"ok": ok_p, "seg": round(seg, 1), "ghidra": ghidra and {
        "version": (ghidra.get("availability") or {}).get("diagnostics", {}).get("provider_version"),
        "install_dir": (ghidra.get("availability") or {}).get("diagnostics", {}).get("install_dir"),
        "checks_fallan": [c["name"] for c in ghidra.get("availability", {}).get("diagnostics", {}).get("checks", []) if not c.get("ok")],
    }}

    ok_d, salida_d, seg_d, err_d = ejecuta(["mcp", "doctor", "--format", "json"], 180, "mcp doctor")
    if ok_d:
        try:
            d = json.loads(salida_d)
            print(f"  MCP: healthy={d.get('healthy')} adaptador={d.get('adapter')} "
                  f"herramientas={d.get('inventory', {}).get('tools', {}).get('observed')} "
                  f"({seg_d:.1f}s)")
            ok = ok and bool(d.get("healthy"))
            datos["mcp_doctor"] = d
        except Exception as exc:
            print(f"  MCP: doctor devolvio algo no-JSON: {exc}")
            ok = False
    else:
        print(f"  MCP: doctor FALLA ({err_d})")
        ok = False

    est = lee_snapshot()
    if est is None:
        print("  snapshot: no hay (paso `snapshot` antes de decompilar)")
    else:
        sh, prov, digest, ent, wf = est
        igual = sh == sha256(EXE)
        print(f"  snapshot: {os.path.relpath(SNAPSHOT, REPO)} proveedor={prov} "
              f"entradas={ent} flujo={wf} identidad={'OK' if igual else 'NO COINCIDE'}")
        if not igual:
            print("    -> el binario cambio: hay que regenerarlo (`snapshot --forzar`)")
        datos["snapshot"] = {"sha256_target": sh, "proveedor": prov, "digest": digest,
                             "entradas": ent, "coincide_con_exe": igual}

    ruta = escribe_recibo("viceex-13-estado.json", datos)
    print(f"  recibo: {os.path.relpath(ruta, REPO)}")
    return 0 if ok else 1


# ------------------------------------------------------------------------- snapshot
def cmd_snapshot(args):
    print("== snapshot de analisis ==")
    est = lee_snapshot()
    sh = sha256(EXE)
    if est and est[0] == sh and not args.forzar:
        print(f"  ya existe y coincide con el exe ({sh[:12]}); nada que hacer")
        return 0
    print(f"  construyendo para {os.path.basename(EXE)} ({sh[:12]})… un analisis")
    print(f"  en frio tarda minutos; el limite de arranque de Ghidra es "
          f"{os.environ.get('REA_GHIDRA_STARTUP_TIMEOUT_MS', '330000 (por defecto)')} ms")
    ok, salida, seg, err = ejecuta(
        ["inspect", EXE, "--provider", "ghidra", "--snapshot", SNAPSHOT,
         "--format", "json"], args.timeout, "inspect")
    print(f"  -> {'OK' if ok else 'FALLO'} en {seg:.0f}s" + (f": {err}" if err else ""))
    if ok:
        escribe_recibo("viceex-13-snapshot-run.out", salida)
        est = lee_snapshot()
        if est:
            print(f"  snapshot: proveedor={est[1]} digest={est[2][:12]} entradas={est[3]}")
    return 0 if ok else 1


# ---------------------------------------------------------------------- decompilar
def cmd_dec(args):
    objetivo = DIRS.get(args.direccion.lower(), args.direccion)
    print(f"== decompilacion CLI de {objetivo} ==")
    orden = ["decompile", EXE, objetivo, "--provider", "ghidra", "--format", "json"]
    if not args.sin_snapshot:
        if not os.path.exists(SNAPSHOT):
            print("  no hay snapshot: lanzo `snapshot` primero")
            if cmd_snapshot(args) != 0:
                return 1
        orden += ["--snapshot", SNAPSHOT]
    ok, salida, seg, err = ejecuta(orden, args.timeout, "decompile")
    print(f"  -> {'OK' if ok else 'FALLO'} en {seg:.0f}s" + (f": {err}" if err else ""))
    sufijo = "sin-snapshot" if args.sin_snapshot else "con-snapshot"
    escribe_recibo(f"viceex-13-dec-{objetivo}-{sufijo}.json", salida or err)
    if not ok:
        return 1
    try:
        d = json.loads(salida)
    except Exception:
        print(salida[:2000])
        return 0
    texto = json.dumps(d, indent=2, ensure_ascii=False)
    print(texto[:args.ver])
    print(f"  recibo: tmp/rea/viceex-13-dec-{objetivo}-{sufijo}.json ({len(texto)} B)")
    return 0


# -------------------------------------------------------------------------- MCP
def _config_mcp():
    with open(MCP_JSON, encoding="utf-8") as fh:
        cfg = json.load(fh)
    srv = cfg["mcpServers"]["rea"]
    entorno = dict(os.environ)
    entorno.update(srv.get("env", {}))
    return srv["command"], srv.get("args", []), entorno


class Mcp:
    """Cliente MCP minimo por stdio (lineas JSON) para comprobar la conexion real."""

    def __init__(self, cmd, args, entorno):
        self.p = subprocess.Popen([cmd, *args], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE, env=entorno, text=True,
                                  encoding="utf-8", errors="replace", bufsize=1)
        self.cola = queue.Queue()
        self.ident = 0
        threading.Thread(target=self._lee, args=(self.p.stdout,), daemon=True).start()
        threading.Thread(target=self._lee, args=(self.p.stderr,), daemon=True).start()

    def _lee(self, flujo):
        for linea in flujo:
            self.cola.put(linea)

    def pide(self, metodo, params=None, timeout=60):
        self.ident += 1
        msg = {"jsonrpc": "2.0", "id": self.ident, "method": metodo}
        if params is not None:
            msg["params"] = params
        t0 = time.monotonic()
        self.p.stdin.write(json.dumps(msg) + "\n")
        self.p.stdin.flush()
        while True:
            restante = timeout - (time.monotonic() - t0)
            if restante <= 0:
                raise TimeoutError(f"sin respuesta a {metodo} en {timeout}s")
            try:
                linea = self.cola.get(timeout=min(restante, 1.0))
            except queue.Empty:
                continue
            try:
                d = json.loads(linea)
            except Exception:
                continue
            if d.get("id") == self.ident:
                if "error" in d:
                    raise RuntimeError(f"{metodo}: {d['error']}")
                return d.get("result"), time.monotonic() - t0

    def avisa(self, metodo, params=None):
        self.p.stdin.write(json.dumps({"jsonrpc": "2.0", "method": metodo,
                                       "params": params or {}}) + "\n")
        self.p.stdin.flush()

    def cierra(self):
        for flujo in (self.p.stdin,):
            try:
                flujo.close()
            except Exception:
                pass
        try:
            self.p.terminate()
            self.p.wait(timeout=10)
        except Exception:
            if os.name == "nt":
                subprocess.run(["taskkill", "/T", "/F", "/PID", str(self.p.pid)],
                               capture_output=True)
            else:
                self.p.kill()


def cmd_mcp_check(args):
    print("== conexion MCP (config real de .agents/mcp.json) ==")
    cmd, argumentos, entorno = _config_mcp()
    print(f"  $ {cmd} {' '.join(argumentos)}")
    informe = {"config": {"command": cmd, "args": argumentos,
                          "env": {k: v for k, v in entorno.items() if k.startswith("GHIDRA") or k == "JAVA_HOME"}},
               "fases": {}, "ok": True}
    cli = Mcp(cmd, argumentos, entorno)
    try:
        res, seg = cli.pide("initialize", {
            "protocolVersion": "2025-11-25", "capabilities": {},
            "clientInfo": {"name": "viceex-rea-check", "version": "1.0"}}, timeout=90)
        cli.avisa("notifications/initialized")
        srv = (res or {}).get("serverInfo", {})
        print(f"  initialize: {srv.get('name')} {srv.get('version')} en {seg:.1f}s")
        informe["fases"]["initialize"] = {"seg": round(seg, 1), "servidor": srv}

        res, seg = cli.pide("tools/list", timeout=90)
        herramientas = (res or {}).get("tools", [])
        print(f"  tools/list: {len(herramientas)} herramientas en {seg:.1f}s")
        informe["fases"]["tools_list"] = {"seg": round(seg, 1), "n": len(herramientas)}
        esquemas = {t["name"]: t.get("inputSchema", {}) for t in herramientas}
        for nombre in ("open_binary", "close_binary", "binary_session", "analyze_function",
                       "procedure_pseudo_code", "batch_decompile"):
            campos = list((esquemas.get(nombre, {}).get("properties") or {}).keys())
            print(f"    {nombre}: {campos if campos else 'AUSENTE'}")
        informe["esquemas"] = {n: list((esquemas.get(n, {}).get("properties") or {}).keys())
                               for n in ("open_binary", "close_binary", "binary_session",
                                         "analyze_function", "procedure_pseudo_code", "batch_decompile")}

        res, seg = cli.pide("tools/call", {"name": "binary_session", "arguments": {}}, timeout=args.timeout)
        print(f"  binary_session (sin target): OK en {seg:.1f}s")
        informe["fases"]["binary_session"] = {"seg": round(seg, 1)}

        if args.abrir:
            argumentos_abrir = {"path": EXE, "provider_id": "ghidra"}
            if os.path.exists(SNAPSHOT):
                argumentos_abrir["snapshot_path"] = SNAPSHOT
                print(f"  abro con el snapshot {os.path.basename(SNAPSHOT)} (import atomico)")
            else:
                print("  abro SIN snapshot: aqui Ghidra analiza en frio (minutos)")
            t0 = time.monotonic()
            try:
                res, seg = cli.pide("tools/call", {"name": "open_binary",
                                                   "arguments": argumentos_abrir},
                                    timeout=args.timeout)
                print(f"  open_binary: OK en {seg:.1f}s")
                informe["fases"]["open_binary"] = {"seg": round(seg, 1),
                                                   "con_snapshot": "snapshot_path" in argumentos_abrir}
            except (TimeoutError, RuntimeError) as exc:
                seg = time.monotonic() - t0
                print(f"  open_binary: FALLA tras {seg:.1f}s -> {exc}")
                informe["fases"]["open_binary"] = {"seg": round(seg, 1), "error": str(exc)}
                informe["ok"] = False
    except (TimeoutError, RuntimeError) as exc:
        print(f"  FALLO: {exc}")
        informe["ok"] = False
        informe["error"] = str(exc)
    finally:
        cli.cierra()

    veredicto = ("MCP OK" if informe["ok"] else "MCP INCOMPLETO")
    print(f"  veredicto: {veredicto}")
    ruta = escribe_recibo("viceex-13-mcp-check.json", informe)
    print(f"  recibo: {os.path.relpath(ruta, REPO)}")
    return 0 if informe["ok"] else 1


def cmd_todo(args):
    a = cmd_estado(args)
    b = cmd_mcp_check(args)
    return a or b


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    sub.add_parser("estado", help="proveedores y doctor MCP (rapido)").set_defaults(fn=cmd_estado)

    p = sub.add_parser("snapshot", help="asegura el snapshot de analisis del exe")
    p.add_argument("--forzar", action="store_true")
    p.add_argument("--timeout", type=int, default=1800)
    p.set_defaults(fn=cmd_snapshot)

    p = sub.add_parser("dec", help="decompila una direccion por CLI")
    p.add_argument("direccion", help="0x… o un alias (0x0fa2, callee, 0x0fa3, 0x0fa9)")
    p.add_argument("--sin-snapshot", action="store_true")
    p.add_argument("--timeout", type=int, default=1800)
    p.add_argument("--ver", type=int, default=4000, help="bytes de pseudocodigo a mostrar")
    p.set_defaults(fn=cmd_dec)

    p = sub.add_parser("mcp-check", help="comprueba la conexion MCP por stdio")
    p.add_argument("--abrir", action="store_true", help="ademas abre el binario (open_binary)")
    p.add_argument("--timeout", type=int, default=240)
    p.set_defaults(fn=cmd_mcp_check)

    p = sub.add_parser("todo", help="estado + mcp-check")
    p.add_argument("--abrir", action="store_true")
    p.add_argument("--timeout", type=int, default=240)
    p.set_defaults(fn=cmd_todo)

    args = ap.parse_args()
    for ruta in (EXE,):
        if not os.path.exists(ruta):
            print(f"falta {ruta} (define VC_VICEEX_EXE)", file=sys.stderr)
            return 2
    return args.fn(args)


if __name__ == "__main__":
    sys.exit(main())
