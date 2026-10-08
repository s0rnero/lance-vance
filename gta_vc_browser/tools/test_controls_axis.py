#!/usr/bin/env python3
"""Contratos del plan 4.0 para viceext-log-check.py e ifp_add.py.

Datos 100% sinteticos y rotulados (cabecera `SYNTHETIC-PLAN4`): nunca un log de
jugador. Solo stdlib. Los temporales viven en `gta_vc_browser/tmp/` y la suite
borra unicamente los suyos.

Uso:
    PYTHONIOENCODING=utf-8 PYTHONDONTWRITEBYTECODE=1 \
        python -m unittest discover -s gta_vc_browser/tools -p 'test_controls_axis.py' -v
"""
import contextlib
import importlib.util
import io
import os
import shutil
import subprocess
import sys
import tempfile
import unittest

sys.dont_write_bytecode = True

TOOLS = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(TOOLS))
CHECKER = os.path.join(TOOLS, "viceext-log-check.py")
IFP_ADD = os.path.join(TOOLS, "ifp_add.py")
TMPBASE = os.path.join(os.path.dirname(TOOLS), "tmp")


def _cargar(ruta, nombre):
    spec = importlib.util.spec_from_file_location(nombre, ruta)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


CHECK = _cargar(CHECKER, "viceext_log_check")


def veredicto(fn, lineas):
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        res = fn(lineas)
    return res


class BaseSintetica(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        os.makedirs(TMPBASE, exist_ok=True)
        cls.dir = tempfile.mkdtemp(prefix="test_controls_axis-", dir=TMPBASE)

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.dir, ignore_errors=True)

    def escribir(self, nombre, lineas):
        ruta = os.path.join(self.dir, nombre)
        with open(ruta, "w", encoding="utf-8", newline="\n") as f:
            for l in lineas:
                f.write(l + "\n")
        return ruta

    def cli(self, ruta, *args):
        env = dict(os.environ, PYTHONIOENCODING="utf-8", PYTHONDONTWRITEBYTECODE="1")
        return subprocess.run([sys.executable, CHECKER, ruta, *args],
                              capture_output=True, text=True, encoding="utf-8",
                              errors="replace", env=env, timeout=120)


class R11AnclajeReal(BaseSintetica):
    def test_dummies_con_posicion_inverosimil_no_puede_ser_ok(self):
        lineas = [
            "JS build=SYNTHETIC-PLAN4 data=SYNTHETIC-PLAN4",
            "12:00:00 SVLIGHTS model=156 dummies=1 on=1 var=0 extras=1,2 radio=0.70"
            " pos=999999.00,999999.00,999999.00",
        ]
        estado, motivo = veredicto(CHECK.bloque_r11, lineas)
        self.assertNotEqual(estado, "OK",
                            "R11 no puede afirmar anclaje con una posicion de 999999 m: %s" % motivo)

    def test_dummies_sin_posicion_no_puede_ser_ok(self):
        lineas = [
            "JS build=SYNTHETIC-PLAN4 data=SYNTHETIC-PLAN4",
            "12:00:00 SVLIGHTS model=156 dummies=1 on=1 var=0 extras=1,2 radio=0.70",
        ]
        estado, motivo = veredicto(CHECK.bloque_r11, lineas)
        self.assertNotEqual(estado, "OK",
                            "R11 no puede afirmar anclaje sin haber observado la posicion: %s" % motivo)


class R15Pose(BaseSintetica):
    def test_pesos_ausentes_no_pueden_ser_ok(self):
        lineas = [
            "JS build=SYNTHETIC-PLAN4 data=SYNTHETIC-PLAN4",
            "12:00:00 VICEEXT crouch on",
            "12:00:02 VICEEXT crouch off motivo=test",
            "12:00:02 VICEEXT crouch pose off clips=1",
        ]
        estado, motivo = veredicto(CHECK.bloque_r15, lineas)
        self.assertNotEqual(estado, "OK",
                            "R15 no puede afirmar peso 0 sin ninguna muestra CROUCHPOSE: %s" % motivo)

    def test_peso_alto_tardio_sigue_siendo_fallo(self):
        lineas = [
            "JS build=SYNTHETIC-PLAN4 data=SYNTHETIC-PLAN4",
            "12:00:00 VICEEXT crouch on",
            "12:00:02 VICEEXT crouch off motivo=test",
            "12:00:02 VICEEXT crouch pose off clips=1",
            "12:00:03 CROUCHPOSE desde=1200 idle=0.40 fwd=0.00 back=0.00",
        ]
        estado, motivo = veredicto(CHECK.bloque_r15, lineas)
        self.assertEqual(estado, "FALLO", "un peso 0.40 a 1200 ms debe ser FALLO: %s" % motivo)


class CliPlan4(BaseSintetica):
    def test_plan4_soportada_y_etiqueta_sus_resultados(self):
        ruta = self.escribir("p4-soportada.log", [
            "JS build=2026-10-04-swim34 data=2026-10-04-gxt1",
            "12:00:00 P4 kind=identity schema=1 gen=1 frame=1 sim=0.000 case=0 id=0"
            " version=2026-10-04-swim34 data=2026-10-04-gxt1",
        ])
        r = self.cli(ruta, "--plan-4", "--expected-version", "2026-10-04-swim34",
                     "--expected-data-tag", "2026-10-04-gxt1")
        self.assertNotIn("unrecognized arguments", r.stderr)
        self.assertIn("P4", r.stdout,
                      "la CLI --plan-4 debe existir y etiquetar sus resultados: %s" % r.stderr)

    def test_plan4_sin_cobertura_no_puede_ser_0(self):
        ruta = self.escribir("p4-sin-cobertura.log", [
            "JS build=2026-10-04-swim34 data=2026-10-04-gxt1",
            "12:00:00 P4 kind=identity schema=1 gen=1 frame=1 sim=0.000 case=0 id=0"
            " version=2026-10-04-swim34 data=2026-10-04-gxt1",
        ])
        r = self.cli(ruta, "--plan-4", "--expected-version", "2026-10-04-swim34",
                     "--expected-data-tag", "2026-10-04-gxt1")
        self.assertNotEqual(r.returncode, 0,
                            "sin los 7 criterios cubiertos la CLI no puede salir 0")

    def test_cli_historica_sigue_funcionando(self):
        ruta = self.escribir("historica.log", [
            "JS build=2026-10-04-swim34 data=2026-10-04-gxt1",
            "12:00:00 PEDAT x=1.0 y=2.0 z=3.0 av=0.0 velo=0.0",
        ])
        r = self.cli(ruta)
        self.assertIn(r.returncode, (0, 1))
        self.assertIn("resumen", r.stdout)


class Ax4IdArma(BaseSintetica):
    AIMCFG = ("12:00:00 AIMCFG forceauto=0 lock=1 tri=1 mcx=0.530 mcy=0.400 brazo=0"
              " sensx=1.00 sensy=1.00 fov=1 ley=1")
    AIMCAM = ("12:00:00 AIMCAM m=5 dist=2.70 alt=0.20 zoff=0.00 amax=18.0 fight=0"
              " hombro=0.20 obj=0 ex=0.0 ey=0.0 ez=0.0 lado=0 trans=1")

    def _ax(self, arma, fov):
        lineas = [
            "JS build=SYNTHETIC-PLAN4 data=SYNTHETIC-PLAN4",
            self.AIMCFG,
            self.AIMCAM,
            "12:00:01 AIMFOV fov=%.2f arma=%d alcance=100.0 taken=1" % (fov, arma),
        ]
        estado, motivo = veredicto(CHECK.bloque_ax, lineas)
        return estado, motivo

    def test_arma_28_no_es_el_minigun_de_este_arbol(self):
        estado, motivo = self._ax(28, 50.0)
        self.assertNotIn("Minigun", motivo,
                         "el arma 28 es SNIPERRIFLE en este arbol (MINIGUN=33): %s" % motivo)

    def test_arma_33_es_el_minigun_y_debe_detectarse(self):
        estado, motivo = self._ax(33, 50.0)
        self.assertIn("Minigun", motivo,
                      "el Minigun real (33) tomando FOV debe detectarse: %s" % motivo)


class IfpReemplazar(BaseSintetica):
    def test_ifp_add_declara_el_modo_reemplazar(self):
        with open(IFP_ADD, encoding="utf-8") as f:
            texto = f.read()
        self.assertIn("--reemplazar", texto,
                      "ifp_add.py debe ofrecer el modo dirigido --reemplazar (paso 14)")

    def test_ifp_add_declara_la_copia_exclusiva(self):
        with open(IFP_ADD, encoding="utf-8") as f:
            texto = f.read()
        self.assertIn("--copia", texto,
                      "ifp_add.py debe ofrecer --copia exclusiva fuera de datos servidos (paso 14)")


class P4Golden(BaseSintetica):
    """Fixture sintetica completa: los 8 criterios cubiertos -> exit 0."""

    def _log(self):
        lineas = [
            "JS build=V1 data=D1",
            "12:00:00 P4 kind=identity schema=1 gen=1 frame=1 sim=0.000 case=0 id=0"
            " version=V1 data=D1",
        ]
        estado = {"frame": 1, "sim": 0.0, "id": 1}

        def nf():
            estado["frame"] += 1
            estado["sim"] = round(estado["sim"] + 0.02, 3)
            return estado["frame"], estado["sim"]

        def ev(kind, case, ident, **kw):
            f, s = nf()
            partes = ["P4", "kind=%s" % kind, "schema=1", "gen=1", "frame=%d" % f,
                      "sim=%.3f" % s, "case=%d" % case, "id=%d" % ident]
            for k in sorted(kw):
                partes.append("%s=%s" % (k, kw[k]))
            lineas.append("12:00:00 " + " ".join(partes))

        def vec(v):
            return "%.6f,%.6f,%.6f" % tuple(v)

        def neg(a):
            return (-a[0], -a[1], -a[2])

        f_b = (0.0, 1.0, 0.0)
        r_b = (1.0, 0.0, 0.0)

        def move(case, lr, ud, req, f, r, crouch, aim=0, weapon=5):
            ident = estado["id"]
            estado["id"] += 1
            ev("move_begin", case, ident, lr=lr, ud=ud, aim=aim, policy="walkaround",
               baseF=vec(f), baseR=vec(r), req=vec(req), crouch=crouch, weapon=weapon,
               walk=0, sprint=0)
            ev("move_end", case, ident, elapsed="3.000", dist="3.000", req=vec(req),
               baseF=vec(f), baseR=vec(r), dirErrMax="2.000", bodyErrMax="5.000",
               crouch=crouch, weapon=weapon, aim=aim, lr=lr, ud=ud, walk=0, sprint=0,
               speedTarget="3.00", speedReal="3.00")

        orient = [(f_b, r_b), ((1.0, 0.0, 0.0), (0.0, -1.0, 0.0)),
                  ((0.0, -1.0, 0.0), (-1.0, 0.0, 0.0)),
                  ((-1.0, 0.0, 0.0), (0.0, 1.0, 0.0))]
        for f, r in orient:
            for crouch in (0, 1):
                move(1, 0, 127, neg(f), f, r, crouch)

        import math
        for slr in (-1, 1):
            for sud in (-1, 1):
                n = math.sqrt(2.0)
                u = tuple(comp * (-sud / n) + rr * (slr / n) for comp, rr in zip(f_b, r_b))
                move(2, slr * 127, sud * 127, u, f_b, r_b, 0)
        for sud in (-1, 1):
            move(2, 0, sud * 127, tuple(c * -sud for c in f_b), f_b, r_b, 0)
        for slr in (-1, 1):
            move(2, slr * 127, 0, tuple(c * slr for c in r_b), f_b, r_b, 0)

        for _ in range(3):
            move(4, 0, -127, f_b, f_b, r_b, 0)
            move(4, 0, -127, f_b, f_b, r_b, 1)

        for posture, campos in ((1, {"t80": "0.1000"}), (0, {"tClear": "0.1000", "maxOwnAfter500": "0.0000"})):
            for _ in range(20):
                ident = estado["id"]
                estado["id"] += 1
                ev("pose_begin", 3, ident, posture=posture)
                pesos = ",".join(["0.0000"] * 9)
                kw = {"posture": posture, "weights": pesos, "boneNonfinite": 0, "boneDegenerate": 0}
                kw.update(campos)
                ev("pose_end", 3, ident, **kw)

        for smp in range(30):
            f, s = nf()
            lineas.append("12:00:00 P4 kind=pose_exit schema=1 gen=1 frame=%d sim=%.3f case=3"
                          " id=901 n=%d sample=%d clip=6 w=%.4f delta=-4.0000 speed=1.0300 flags=100"
                          " pesosum=0.4000"
                          % (f, s, max(0, 9 - smp // 4), smp, max(0.02, 0.40 - 0.0132 * smp)))

        for lado, lr, req in (("L", 127, r_b), ("R", -127, neg(r_b))):
            clip = "Crouch_Roll_%s" % lado
            for _ in range(10):
                ident = estado["id"]
                estado["id"] += 1
                edge = "E%d" % ident
                ev("input_edge", 5, ident, lr=lr, ud=0, aim=1, crouch=1, side=lado,
                   edge=edge, neutral=1, eligible=1)
                ev("roll_start", 5, ident, side=lado, edge=edge, aim=1, crouch=1, weapon=5,
                   req=vec(req), clip=clip, length="0.9310", speed="1.0000")
                ev("roll_end", 5, ident, side=lado, clip=clip, current="0.9310",
                   progress="1.0000", rateIntegral="0.9300", simElapsed="0.9300",
                   travel="2.700", maxWeight="1.0000")
        ev("input_hold_end", 5, estado["id"], heldSim="3.0000")
        estado["id"] += 1

        for k in range(20):
            ident = estado["id"]
            estado["id"] += 1
            ft = 50.0 if k < 10 else 70.0
            crouch = 1 if k in (5, 15) else 0
            ev("cam_begin", 6, ident, **{"from": "follow", "to": "aim",
                                          "visibleFront": vec(f_b), "mouse": 0, "lock": 0,
                                          "crouch": crouch})
            ev("cam_first", 6, ident, visibleFront=vec(f_b))
            ev("cam_end", 6, ident, visibleFront=vec(f_b), fov="%.2f" % ft,
               fovTarget="%.2f" % ft, pixelErr="0.50", crouch=crouch, collision=0)

        ev("service", 7, estado["id"], vehicle=1, model=156, siren=1, var=0, selectedBars=1,
           visibleLenses=4, anchorError="0.020", registered=0, local="0.000,0.000,0.500",
           world="0.000,0.000,1.000", carPos="0.000,0.000,0.000",
           carRot="0.000000,1.000000,0.000000", cache="cold", damage=0)
        estado["id"] += 1
        ev("service", 7, estado["id"], vehicle=1, model=156, siren=1, var=0, selectedBars=1,
           visibleLenses=4, anchorError="0.030", registered=0, local="0.000,0.000,0.500",
           world="0.000,0.100,1.000", carPos="0.000,0.000,0.000",
           carRot="0.000000,1.000000,0.000000", cache="hot", damage=0)
        estado["id"] += 1
        ev("service", 7, estado["id"], vehicle=2, model=156, siren=1, var=1, selectedBars=1,
           visibleLenses=4, anchorError="0.050", registered=0, local="0.000,0.000,0.500",
           world="102.000,0.000,1.000", carPos="100.000,0.000,0.000",
           carRot="1.000000,0.000000,0.000000", cache="cold", damage=1)
        estado["id"] += 1
        ev("service", 7, estado["id"], vehicle=2, model=156, siren=0, var=2, selectedBars=1,
           visibleLenses=0, anchorError="0.040", registered=0, local="0.000,0.000,0.500",
           world="104.000,0.000,1.000", carPos="100.000,0.000,0.000",
           carRot="1.000000,0.000000,0.000000", cache="hot", damage=0)

        for velo, pies in ((1.10, 1.10), (1.12, 1.13), (1.11, 1.10)):
            lineas.append("12:00:20 CROUCH2 h=11.00 avance=%.2f velo=%.2f moved=0.02 mvec=0.02"
                          " anim=0.00 fspd=0.00 dt=1.00 nf=60 hit=0 rotDest=0.00 rumbo=0.00 clip=245"
                          " nom=GunCrouchFwd peso=1.00 camz=11.90 camdist=3.50 modo=4 baja=0.55 ang=0.0"
                          " mps=1.13 obju=0.023 pies=0.55 strafe=1 mira=0 gat=0 pesomira=0.00 tgt=0 cana=1"
                          " arma=17 veloc=1.13 rueda=0 giro=0 hdgr=15 cal=1 clipr=2.05 entrada=0"
                          " otros=0.00 pose=- pesoarma=0.00 nomarma=- piesMps=%.2f"
                          % (velo, velo, pies))
        ev("armsight", 4, 0, pose="colt45_crouchfire", errDeg="1.5000", canon="0.5000",
           weapon=17, crouch=1)
        ev("fire", 4, 0, aim=0, gat=1, arma=17, hip=1, estado=22, slot=3, move=1)
        return lineas

    def test_fixture_completa_sale_0_con_los_ocho_pass(self):
        ruta = self.escribir("p4-golden.log", self._log())
        r = self.cli(ruta, "--plan-4", "--expected-version", "V1", "--expected-data-tag", "D1")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        for n in range(1, 9):
            self.assertIn("P4.%d PASS" % n, r.stdout)
        self.assertNotIn("PARCIAL", r.stdout)
        self.assertNotIn("SIN DATOS", r.stdout)

    def test_fixture_completa_con_tag_esperado_distinto_sale_1(self):
        ruta = self.escribir("p4-golden-tag.log", self._log())
        r = self.cli(ruta, "--plan-4", "--expected-version", "V1", "--expected-data-tag", "D2")
        self.assertEqual(r.returncode, 1, r.stdout + r.stderr)


class P42Evidencia(BaseSintetica):
    """Plan 4.2 (paso 15): los cuatro `kind` nuevos (sight, crouchspeed,
    move_apply, pose_exit) son evidencia valida para el checker y el desbloqueo
    de la familia de postura (§4.5) no relaja ningun exit. Fixtures rotuladas.
    """

    IDENT = ("12:00:00 P4 kind=identity schema=1 gen=1 frame=1 sim=0.000 case=0 id=0"
             " version=2026-10-05-ctrl42 data=2026-10-05-gxt2")
    SIGHT_CLASE = ("12:00:05 P4 kind=sight schema=1 gen=1 frame=900 sim=18.000 case=4 id=0"
                   " weapon=17 sight=4 loaded=1 why=4 cruz=0 px=640.0 py=360.0")
    SIGHT_SIN_MIRA = ("12:00:06 P4 kind=sight schema=1 gen=1 frame=910 sim=18.200 case=4 id=0"
                      " weapon=4 sight=0 loaded=0 why=4 cruz=1 px=640.0 py=360.0")
    CROUCHSPEED = ("12:00:07 P4 kind=crouchspeed schema=1 gen=1 frame=920 sim=18.400 case=4 id=0"
                   " mps=2.1300 rate=1.0300 mfs=2.1300 target=2.1300 clip=6")
    MOVE_APPLY = ("12:00:08 P4 kind=move_apply schema=1 gen=1 frame=930 sim=18.600 case=1 id=0"
                  " src=1 spdReq=0.0500 spdEff=2.1300 mag=1.0650 dir=0.0000 frames=3 duck=0"
                  " aim=0 crouch=0 stickBlocked=0")
    POSE_EXIT_VIVA = ("12:00:09 P4 kind=pose_exit schema=1 gen=1 frame=940 sim=18.800 case=3 id=7"
                      " n=1 sample=29 clip=6 w=0.9000 delta=-4.0000 speed=1.0300 flags=100")
    POSE_EXIT_LIMPIA = ("12:00:10 P4 kind=pose_exit schema=1 gen=1 frame=941 sim=18.820 case=3 id=902"
                        " n=0 sample=29 clip=0 w=0.0200 delta=-4.0000 speed=0.0000 flags=100")

    def _ev(self, lineas):
        ev = CHECK.p4_cargar(lineas)
        self.assertEqual(ev["invalidos"], [],
                         "evidencia rotulada del plan 4.2 rechazada: %s" % ev["invalidos"])
        return ev["eventos"]

    def test_los_cuatro_kinds_nuevos_son_evidencia_valida(self):
        eventos = self._ev(["JS build=V2 data=D2", self.IDENT, self.SIGHT_CLASE,
                            self.CROUCHSPEED, self.MOVE_APPLY, self.POSE_EXIT_VIVA])
        kinds = [e["kind"] for e in eventos]
        for k in ("sight", "crouchspeed", "move_apply", "pose_exit"):
            self.assertIn(k, kinds,
                          "el checker debe aceptar %s como kind P4 valido" % k)

    def test_arma_con_mira_suprime_la_cruz_y_sin_mira_la_conserva(self):
        eventos = self._ev(["JS build=V2 data=D2", self.IDENT, self.SIGHT_CLASE,
                            self.SIGHT_SIN_MIRA])
        con = [e for e in eventos if e.get("sight") == "4"][0]
        sin = [e for e in eventos if e.get("sight") == "0"][0]
        self.assertEqual((con["loaded"], con["cruz"]), ("1", "0"))
        self.assertEqual((sin["loaded"], sin["cruz"]), ("0", "1"))

    def test_asociacion_del_grupo_viva_tras_desagacharse_llega_al_log(self):
        eventos = self._ev(["JS build=V2 data=D2", self.IDENT, self.POSE_EXIT_VIVA])
        pe = [e for e in eventos if e["kind"] == "pose_exit"][0]
        self.assertEqual(pe["sample"], "29")
        self.assertTrue(float(pe["w"]) > 0.5,
                        "la fixture rotula el caso negativo M4 (asociacion viva a las 30 muestras)")

    def test_sight_sin_campo_obligatorio_no_pasa(self):
        roto = self.SIGHT_CLASE.replace(" cruz=0", "")
        ev = CHECK.p4_cargar(["JS build=V2 data=D2", self.IDENT, roto])
        self.assertTrue(any("cruz" in m for m in ev["invalidos"]),
                        "sight sin cruz debe ser evidencia invalida: %s" % ev["invalidos"])

    def test_pose_exit_no_finito_no_pasa(self):
        roto = self.POSE_EXIT_VIVA.replace("w=0.9000", "w=nan")
        ev = CHECK.p4_cargar(["JS build=V2 data=D2", self.IDENT, roto])
        self.assertTrue(any("w" in m for m in ev["invalidos"]),
                        "pose_exit con w no finito debe ser evidencia invalida: %s" % ev["invalidos"])

    def test_move_apply_sin_spdEff_no_pasa(self):
        roto = self.MOVE_APPLY.replace(" spdEff=2.1300", "")
        ev = CHECK.p4_cargar(["JS build=V2 data=D2", self.IDENT, roto])
        self.assertTrue(any("spdEff" in m for m in ev["invalidos"]),
                        "move_apply sin spdEff debe ser evidencia invalida: %s" % ev["invalidos"])

    def test_kind_desconocido_sigue_siendo_invalido(self):
        raro = self.MOVE_APPLY.replace("kind=move_apply", "kind=move_aplicado")
        ev = CHECK.p4_cargar(["JS build=V2 data=D2", self.IDENT, raro])
        self.assertTrue(any("desconocido" in m for m in ev["invalidos"]),
                        "un kind inventado no puede colarse: %s" % ev["invalidos"])


class P42FamiliaPostura(BaseSintetica):
    """§4.5 (paso 14): id monotono por tramo desbloquea P4.0 sin perder el
    FALLO real cuando dos inicios comparten id."""

    IDENT = ("12:00:00 P4 kind=identity schema=1 gen=1 frame=1 sim=0.000 case=0 id=0"
             " version=V2 data=D2")

    def _pose(self, ident, posture, terminal=None):
        inicio = ("12:00:01 P4 kind=pose_begin schema=1 gen=1 frame=10 sim=0.200 case=3"
                  " id=%d posture=%d" % (ident, posture))
        pesos = ",".join(["0.0000"] * 9)
        campos = {1: " t80=0.1000", 0: " tClear=0.1000 maxOwnAfter500=0.0000"}[posture]
        fin = ("12:00:02 P4 kind=pose_end schema=1 gen=1 frame=20 sim=0.400 case=3 id=%d"
               " posture=%d weights=%s boneNonfinite=0 boneDegenerate=0%s"
               % (ident, posture, pesos, campos))
        return [inicio, fin]

    def test_ids_monotonos_de_postura_no_duplican(self):
        lineas = ["JS build=V2 data=D2", self.IDENT]
        for i, p in enumerate((1, 0, 1, 0), start=2):
            lineas += self._pose(i, p)
        ev = CHECK.p4_cargar(lineas)
        self.assertEqual(ev["invalidos"], [],
                         "ids monotonos por tramo deben ser limpios: %s" % ev["invalidos"])

    def test_dos_inicios_con_el_mismo_id_siguen_siendo_fallo(self):
        lineas = ["JS build=V2 data=D2", self.IDENT]
        lineas += self._pose(2, 1)
        lineas += self._pose(2, 0)
        ev = CHECK.p4_cargar(lineas)
        self.assertTrue(any("id duplicado pose/2" in m for m in ev["invalidos"]),
                        "el desbloqueo no puede tapar un id repetido de verdad: %s"
                        % ev["invalidos"])


class P43EvidenciaNueva(BaseSintetica):
    """Paso 3/10/11 del plan 4.3: los cuatro `kind` nuevos (aim, jump, fire,
    armsight) son evidencia valida, y sus campos obligatorios se exigen."""

    IDENT = ("12:00:00 P4 kind=identity schema=1 gen=1 frame=1 sim=0.000 case=0 id=0"
             " version=V4 data=D4")
    AIM = ("12:00:01 P4 kind=aim schema=1 gen=1 frame=10 sim=0.200 case=4 id=0"
           " why=esprint aim=0 sprint=1 jump=0 crouch=0 policy=0 speedGame=0.0000 weapon=17")
    JUMP = ("12:00:02 P4 kind=jump schema=1 gen=1 frame=20 sim=0.400 case=4 id=0"
            " lr=0.00 ud=-127.00 rumbo=0.0000 dirX=0.000000 dirY=-1.000000 march=1")
    FIRE = ("12:00:03 P4 kind=fire schema=1 gen=1 frame=30 sim=0.600 case=4 id=0"
            " aim=0 gat=1 arma=17 hip=1 estado=1 slot=5 move=1")
    ARMSIGHT = ("12:00:04 P4 kind=armsight schema=1 gen=1 frame=40 sim=0.800 case=4 id=0"
                " pose=colt45_crouchfire errDeg=1.5000 weapon=17 crouch=1")

    def _ev(self, lineas):
        ev = CHECK.p4_cargar(lineas)
        self.assertEqual(ev["invalidos"], [],
                         "evidencia rotulada del plan 4.3 rechazada: %s" % ev["invalidos"])
        return ev["eventos"]

    def test_los_cuatro_kinds_nuevos_son_evidencia_valida(self):
        eventos = self._ev(["JS build=V4 data=D4", self.IDENT, self.AIM, self.JUMP,
                            self.FIRE, self.ARMSIGHT])
        kinds = [e["kind"] for e in eventos]
        for k in ("aim", "jump", "fire", "armsight"):
            self.assertIn(k, kinds, "el checker debe aceptar %s como kind P4 valido" % k)

    def test_aim_sin_why_no_pasa(self):
        roto = self.AIM.replace(" why=esprint", "")
        ev = CHECK.p4_cargar(["JS build=V4 data=D4", self.IDENT, roto])
        self.assertTrue(any("why" in m for m in ev["invalidos"]),
                        "aim sin why debe ser evidencia invalida: %s" % ev["invalidos"])

    def test_armsight_sin_pose_no_pasa(self):
        roto = self.ARMSIGHT.replace(" pose=colt45_crouchfire", "")
        ev = CHECK.p4_cargar(["JS build=V4 data=D4", self.IDENT, roto])
        self.assertTrue(any("pose" in m for m in ev["invalidos"]),
                        "armsight sin pose debe ser evidencia invalida: %s" % ev["invalidos"])

    def test_armsight_errdeg_no_finito_no_pasa(self):
        roto = self.ARMSIGHT.replace("errDeg=1.5000", "errDeg=nan")
        ev = CHECK.p4_cargar(["JS build=V4 data=D4", self.IDENT, roto])
        self.assertTrue(any("errDeg" in m for m in ev["invalidos"]),
                        "armsight con errDeg no finito debe ser evidencia invalida: %s" % ev["invalidos"])

    def test_move_apply_sin_stickblocked_no_pasa(self):
        roto = P42Evidencia.MOVE_APPLY.replace(" stickBlocked=0", "")
        ev = CHECK.p4_cargar(["JS build=V4 data=D4", self.IDENT, roto])
        self.assertTrue(any("stickBlocked" in m for m in ev["invalidos"]),
                        "move_apply sin stickBlocked debe ser evidencia invalida: %s" % ev["invalidos"])


class P43FamiliaCamara(BaseSintetica):
    """§4.1 del plan 4.3 (paso 2): el id monotono por tramo desbloquea tambien la
    familia de camara sin tapar un id repetido de verdad."""

    IDENT = ("12:00:00 P4 kind=identity schema=1 gen=1 frame=1 sim=0.000 case=0 id=0"
             " version=V3 data=D3")

    def _inicio(self, ident):
        return [
            ("12:00:01 P4 kind=cam_begin schema=1 gen=1 frame=10 sim=0.200 case=6"
             " id=%d from=1 to=5 visibleFront=0.000000,1.000000,0.000000 mouse=0 lock=0"
             " collision=0 pitchClamped=0 crouch=0" % ident),
            ("12:00:01 P4 kind=cam_first schema=1 gen=1 frame=11 sim=0.220 case=6"
             " id=%d visibleFront=0.000000,1.000000,0.000000 pixelErr=0.5000" % ident),
        ]

    def _tramo(self, ident, fov="70.00"):
        return self._inicio(ident) + [
            ("12:00:02 P4 kind=cam_end schema=1 gen=1 frame=20 sim=0.400 case=6"
             " id=%d visibleFront=0.000000,1.000000,0.000000 fov=%s fovTarget=%s"
             " pixelErr=0.5000 mouse=0 lock=0 collision=0 pitchClamped=0 crouch=0"
             % (ident, fov, fov)),
        ]

    def _cancel(self, ident):
        return ("12:00:03 P4 kind=cam_cancel schema=1 gen=1 frame=30 sim=0.600 case=6"
                " id=%d cause=nueva progress=0.0000" % ident)

    def test_ids_monotonos_de_camara_no_duplican(self):
        lineas = ["JS build=V3 data=D3", self.IDENT]
        for i in (2, 3, 4):
            lineas += self._tramo(i)
        ev = CHECK.p4_cargar(lineas)
        self.assertEqual(ev["invalidos"], [],
                         "ids monotonos por tramo deben ser limpios: %s" % ev["invalidos"])

    def test_dos_inicios_de_camara_con_el_mismo_id_siguen_siendo_fallo(self):
        lineas = ["JS build=V3 data=D3", self.IDENT]
        lineas += self._tramo(2)
        lineas += self._tramo(2)
        ev = CHECK.p4_cargar(lineas)
        self.assertTrue(any("id duplicado cam/2" in m for m in ev["invalidos"]),
                        "el desbloqueo no puede tapar un id repetido de camara: %s"
                        % ev["invalidos"])

    def test_cancel_del_tramo_anterior_no_duplica_terminal(self):
        lineas = ["JS build=V3 data=D3", self.IDENT]
        lineas += self._inicio(2)
        lineas.append(self._cancel(2))
        lineas += self._tramo(3)
        ev = CHECK.p4_cargar(lineas)
        self.assertEqual(ev["invalidos"], [],
                         "el cancel del tramo anterior con su id no puede duplicar: %s"
                         % ev["invalidos"])

    def test_cam_first_y_cam_end_del_mismo_id_no_son_terminal_duplicado(self):
        lineas = ["JS build=V3 data=D3", self.IDENT]
        lineas += self._tramo(2)
        ev = CHECK.p4_cargar(lineas)
        self.assertFalse(any("terminal duplicado cam_" in m for m in ev["invalidos"]),
                         "cam_first y cam_end son terminales distintos de la misma familia: %s"
                         % ev["invalidos"])


class P42GoldenConKindsNuevos(P4Golden):
    """Los cuatro `kind` nuevos no rompen la certificacion de una sesion que ya
    cubria los ocho criterios: el log sigue saliendo exit 0."""

    def _log(self):
        lineas = super()._log()
        for l in (P42Evidencia.SIGHT_CLASE, P42Evidencia.SIGHT_SIN_MIRA,
                  P42Evidencia.CROUCHSPEED, P42Evidencia.MOVE_APPLY,
                  P42Evidencia.POSE_EXIT_LIMPIA):
            lineas.append(l)
        return lineas

    def test_golden_con_los_kinds_nuevos_sigue_saliendo_0(self):
        ruta = self.escribir("p4-golden42.log", self._log())
        r = self.cli(ruta, "--plan-4", "--expected-version", "V1", "--expected-data-tag", "D1")
        self.assertIn("P4.0", r.stdout)
        self.assertNotIn("P4.0  FALLO", r.stdout,
                         "los kind nuevos no pueden invalidar la evidencia: %s" % r.stdout)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)


class P42CriterioPoseExit(BaseSintetica):
    """Criterio P4.8: a las 30 muestras del desagachado, ninguna asociación del
    grupo de agachado puede seguir por encima de 0,05 de peso."""

    def _golden(self):
        return P4Golden._log(self)

    def _veredicto(self, lineas):
        ev = CHECK.p4_cargar(lineas)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            estado, motivo = CHECK.p4_8(ev)
        return estado, motivo

    def test_grupo_a_peso_bajo_a_las_30_muestras_es_pass(self):
        estado, motivo = self._veredicto(["JS build=V1 data=D1"] + self._golden())
        self.assertEqual(estado, "PASS", motivo)

    def test_asociacion_viva_a_las_30_muestras_es_fallo(self):
        estado, motivo = self._veredicto(["JS build=V1 data=D1"] + self._golden()
                                         + [P42Evidencia.POSE_EXIT_VIVA])
        self.assertEqual(estado, "FALLO", motivo)
        self.assertIn("0.9000", motivo)

    def test_desagachado_sin_las_30_muestras_es_parcial(self):
        cortas = [("12:00:11 P4 kind=pose_exit schema=1 gen=1 frame=%d sim=19.%03d case=3"
                   " id=903 n=0 sample=%d clip=0 w=0.0200 delta=-4.0000 speed=0.0000 flags=100"
                   % (950 + s, s, s)) for s in range(10)]
        estado, motivo = self._veredicto(["JS build=V1 data=D1"] + self._golden() + cortas)
        self.assertEqual(estado, "PARCIAL", motivo)

    def test_sin_pose_exit_es_sin_datos(self):
        estado, motivo = self._veredicto([
            "JS build=V1 data=D1",
            "12:00:00 P4 kind=identity schema=1 gen=1 frame=1 sim=0.000 case=0 id=0"
            " version=V1 data=D1",
        ])
        self.assertEqual(estado, "SIN DATOS", motivo)

    def test_cli_con_asociacion_viva_sale_1_y_marca_p48(self):
        ruta = self.escribir("p4-golden-p48.log", self._golden() + [P42Evidencia.POSE_EXIT_VIVA])
        r = self.cli(ruta, "--plan-4", "--expected-version", "V1", "--expected-data-tag", "D1")
        self.assertEqual(r.returncode, 1, r.stdout + r.stderr)
        self.assertIn("P4.8 FALLO", r.stdout)


if __name__ == "__main__":
    unittest.main(verbosity=2)
