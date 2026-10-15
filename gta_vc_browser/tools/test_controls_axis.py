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
    """Fixture sintetica completa: los criterios cubiertos -> exit 0."""

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
        ev("limits", 7, 900, numpeds=140, numvehicles=130, mult="1.50")
        ev("ini", 0, 0, opt=1, flag=1, a=0, val="0.1200")
        ev("ini", 0, 0, opt=8, regen=1, hp="50.0", max=100)
        ev("radio", 1, 0, emisora=2, icono=2)
        ev("truco", 7, 0, cheat="AIRWAYS", efecto="volar_1")
        ev("truco", 7, 0, cheat="BIGSMOKE", efecto="teargas_5")
        ev("truco", 7, 0, cheat="AEZAKMI", efecto="never_wanted_1")
        ev("truco", 7, 0, cheat="IAMINVINCIBLE", efecto="invencible_1")
        ev("truco", 7, 0, cheat="BIGHEADS", efecto="big_heads_1")
        ev("truco", 7, 0, cheat="RCROCKET", efecto="rc_rocket_1")
        ev("cardet", 0, 130, paso="revienta", rueda=2, impulso="180.50")
        ev("cardet", 0, 130, paso="desprende", rueda=2, impulso="210.00")
        ev("cardead", 0, 130, muertos=1, impulso="210.00")
        ev("carluz", 0, 130, luz=0, impulso="190.25")
        # Familia 11: el `features.ini` del mod se lee antes de jugar, asi que una
        # sesion de la build servida SIEMPRE trae sus 17 claves y el resumen. Sin
        # estas lineas P4.15 saldria SIN DATOS y el fixture dejaria de representar
        # una sesion completa.
        vals, rgb = CHECK.fam11_ini()
        for clave, impl in CHECK.FAM11_CLAVES:
            if clave in CHECK.FAM11_JUGADOR:
                ini = int(vals[clave]) if vals.get(clave, "").lstrip("-").isdigit() else 1
                aplica, origen = CHECK.FAM11_JUGADOR[clave], "jugador"
            elif clave in vals:
                if clave == "WaypointColorRGB":
                    ini = aplica = 1
                else:
                    ini = int(vals[clave])
                    aplica = 1 if ini != 0 else 0
                origen = "fichero"
            else:
                ini = aplica = -1
                origen = "port"
            ev("inifile", 0, 0, clave=clave, ini=ini, aplica=aplica, origen=origen,
               impl=1 if impl else 0)
        ev("inifile", 1, 0, fichero=1,
           leidas=sum(1 for c, _ in CHECK.FAM11_CLAVES if c in vals),
           rgb="%d,%d,%d" % rgb)
        # Familia 7: el mundo del mod se ingiere al arrancar (zonas de oclusion del
        # `occlu.ipl` servido), asi que una sesion completa tambien lo trae.
        ev("worlddata", 0, 0, zonas=CHECK.fam7_zonas_servidas()[0])
        return lineas

    def test_fixture_completa_sale_0_con_los_ocho_pass(self):
        ruta = self.escribir("p4-golden.log", self._log())
        r = self.cli(ruta, "--plan-4", "--expected-version", "V1", "--expected-data-tag", "D1")
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        for n in list(range(1, 9)) + [10, 11, 12, 13]:
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



class D7Shift(BaseSintetica):
    def test_shift_con_dos_iconos_es_ok_y_cuenta_p4(self):
        lineas = [
            "JS build=SYNTHETIC-PLAN4 data=SYNTHETIC-PLAN4",
            "12:00:00 KEYICONS txd=pcbtns cargado",
            "12:00:01 HINTKEY accion=VEHICLE_HORN vk=72 icono=1 shift=1",
            "12:00:01 P4 kind=shift schema=1 gen=1 frame=10 sim=0.2000 case=1 id=0"
            " accion=VEHICLE_HORN vk=72 shift=1 icono=1",
        ]
        estado, motivo = veredicto(CHECK.bloque_d7, lineas)
        self.assertEqual(estado, "OK")
        self.assertIn("VEHICLE_HORN", motivo)
        ev = CHECK.p4_cargar(lineas)
        self.assertEqual(ev["invalidos"], [])
        self.assertEqual(len([e for e in ev["eventos"] if e.get("kind") == "shift"]), 1)

    def test_hintkey_vieja_sin_shift_sigue_valiendo(self):
        lineas = [
            "JS build=SYNTHETIC-PLAN4 data=SYNTHETIC-PLAN4",
            "12:00:00 KEYICONS txd=pcbtns cargado",
            "12:00:01 HINTKEY accion=PED_JUMPING vk=32 icono=1",
        ]
        estado, _ = veredicto(CHECK.bloque_d7, lineas)
        self.assertEqual(estado, "OK")


class P912Trucos(BaseSintetica):
    CHEATS = [("AIRWAYS", "volar_1"), ("BIGSMOKE", "teargas_5"),
              ("AEZAKMI", "never_wanted_1"), ("IAMINVINCIBLE", "invencible_1"),
              ("BIGHEADS", "sin_efecto"), ("RCROCKET", "sin_efecto")]

    def _lineas(self, pares):
        ls = ["JS build=SYNTHETIC-PLAN91 data=SYNTHETIC-PLAN91"]
        f = 100
        for ch, ef in pares:
            f += 10
            ls.append("12:00:01 P4 kind=truco schema=1 gen=1 frame=%d sim=1.000 case=7 id=0 cheat=%s efecto=%s" % (f, ch, ef))
        return ls

    def _p412(self, lineas):
        ev = CHECK.p4_cargar(lineas)
        self.assertEqual(ev["invalidos"], [])
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            return CHECK.p4_12(ev)

    def test_seis_trucos_validos_son_pass(self):
        estado, motivo = self._p412(self._lineas(self.CHEATS))
        self.assertEqual(estado, "PASS", motivo)

    def test_falta_un_truco_es_fallo(self):
        estado, motivo = self._p412(self._lineas(self.CHEATS[:5]))
        self.assertEqual(estado, "FALLO", motivo)
        self.assertIn("sin teclear", motivo)

    def test_efecto_ajeno_es_fallo(self):
        pares = [("AIRWAYS", "volar_1"), ("BIGSMOKE", "volar_1"),
                 ("AEZAKMI", "never_wanted_1"), ("IAMINVINCIBLE", "invencible_1"),
                 ("BIGHEADS", "sin_efecto"), ("RCROCKET", "sin_efecto")]
        estado, motivo = self._p412(self._lineas(pares))
        self.assertEqual(estado, "FALLO", motivo)
        self.assertIn("ajeno", motivo)

    def _lineas_reales(self, pares):
        """Las lineas que emite de verdad el motor: `kind=cheat2` y `via=tecleo`."""
        ls = ["JS build=SYNTHETIC-PLAN91 data=SYNTHETIC-PLAN91"]
        f = 100
        for ch, ef in pares:
            f += 10
            ls.append("12:00:01 P4 kind=cheat2 schema=1 gen=1 frame=%d sim=1.000 case=3 id=0"
                      " cheat=%s efecto=%s via=tecleo" % (f, ch, ef))
        return ls

    def test_la_traza_real_del_motor_es_evidencia_valida(self):
        # `via` es una palabra en la traza del motor (`via=tecleo`): exigirla
        # numerica invalidaba cualquier partida en la que se tecleara un truco.
        ev = CHECK.p4_cargar(self._lineas_reales(self.CHEATS))
        self.assertEqual(ev["invalidos"], [])
        self.assertEqual(len([e for e in ev["eventos"] if e["kind"] == "cheat2"]), 6)
        estado, motivo = self._p412(self._lineas_reales(self.CHEATS))
        self.assertEqual(estado, "PASS", motivo)


class P91Uzi(BaseSintetica):
    def _uzi(self, *evs):
        ls = ["JS build=SYNTHETIC-PLAN91 data=SYNTHETIC-PLAN91",
              "12:00:00 WINFO arma=51 grupo=colt45 clips=fire"]
        f = [100]
        for ev, est, clip, tot, ok in evs:
            f[0] += 2
            s = ("12:00:01 P4 kind=uzi schema=1 gen=1 frame=%d sim=1.000 case=5 id=0"
                 " ev=%s estado=%d clip=%d total=%d" % (f[0], ev, est, clip, tot))
            if ok is not None:
                s += " ok=%d" % ok
            ls.append(s)
        return ls

    def test_rafaga_completa_es_ok(self):
        ls = self._uzi(("enter", 8, 30, 300, None), ("ih_enter", 8, 30, 300, None),
                       ("ih_exit", 8, 29, 299, 1), ("exit", 8, 29, 299, 1))
        ev = CHECK.p4_cargar(ls)
        self.assertEqual(ev["invalidos"], [])
        estado, motivo = veredicto(CHECK.bloque_uzi, ls)
        self.assertEqual(estado, "OK", motivo)
        self.assertIn("sin huecos", motivo)

    def test_hueco_ih_enter_sin_salida_se_informa(self):
        ls = self._uzi(("enter", 8, 30, 300, None), ("ih_enter", 8, 30, 300, None))
        estado, motivo = veredicto(CHECK.bloque_uzi, ls)
        self.assertEqual(estado, "OK", motivo)
        self.assertIn("ih_exit", motivo)


class P100BCar(BaseSintetica):
    DET = "12:00:01 P4 kind=cardet schema=1 gen=1 frame=%d sim=1.000 case=0 id=%d paso=%s rueda=%d impulso=%.2f"
    DEA = "12:00:01 P4 kind=cardead schema=1 gen=1 frame=%d sim=1.000 case=0 id=%d muertos=%d impulso=%.2f"
    LUZ = "12:00:01 P4 kind=carluz schema=1 gen=1 frame=%d sim=1.000 case=0 id=%d luz=%d impulso=%.2f"

    def _p413(self, lineas):
        ev = CHECK.p4_cargar(lineas)
        self.assertEqual(ev["invalidos"], [])
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            return CHECK.p4_13(ev)

    def test_choque_duro_es_pass(self):
        ls = ["JS build=SYNTHETIC-PLAN100 data=SYNTHETIC-PLAN100",
              self.DET % (100, 130, "revienta", 2, 180.5),
              self.DET % (104, 130, "desprende", 2, 210.0),
              self.DEA % (104, 130, 1, 210.0),
              self.LUZ % (106, 130, 0, 190.25)]
        estado, motivo = self._p413(ls)
        self.assertEqual(estado, "PASS", motivo)
        self.assertIn("1 NPC", motivo)

    def test_paso_ajeno_es_fallo(self):
        ls = ["JS build=SYNTHETIC-PLAN100 data=SYNTHETIC-PLAN100",
              self.DET % (100, 130, "flota", 2, 180.5)]
        estado, motivo = self._p413(ls)
        self.assertEqual(estado, "FALLO", motivo)
        self.assertIn("ajeno", motivo)

    def test_sin_choques_es_sin_datos(self):
        estado, motivo = self._p413(["JS build=SYNTHETIC-PLAN100 data=SYNTHETIC-PLAN100"])
        self.assertEqual(estado, "SIN DATOS", motivo)



class P120ScriptYParticulas(BaseSintetica):
    """Plan 12.0 (familias 13 y 3/5): las dos trazas nuevas son evidencia valida
    del evaluador estricto y su veredicto no se puede relajar: un opcode propio
    sin implementar y un `particle.cfg` sin los tipos del mod tienen que verse.
    """

    IDENT = ("12:00:00 P4 kind=identity schema=1 gen=1 frame=1 sim=0.000 case=0 id=0"
             " version=SYNTHETIC-PLAN120 data=SYNTHETIC-PLAN120")

    # Los 7 opcodes que su `main.scm` usa de verdad, con los campos que emite el
    # motor en cada uno (0x0FA0 45 usos, 0x0FA1 2, 0x0FA2 91, 0x0FA6 46,
    # 0x0FA7 1, 0x0FA8 20, 0x0FA9 1).
    MODSCRIPT = (
        "12:00:01 P4 kind=modscript schema=1 gen=1 frame=10 sim=0.200 case=1 id=0"
        " op=0x0FA0 cmd=4000 ped=7 var=3",
        "12:00:02 P4 kind=modscript schema=1 gen=1 frame=20 sim=0.400 case=1 id=0"
        " op=0x0FA1 cmd=4001 ped=7 var=3",
        "12:00:03 P4 kind=modscript schema=1 gen=1 frame=30 sim=0.600 case=1 id=0"
        " op=0x0FA2 cmd=4002 on=1 sel=7",
        "12:00:04 P4 kind=modscript schema=1 gen=1 frame=40 sim=0.800 case=1 id=0"
        " op=0x0FA6 cmd=4006 global=0 guardado=0",
        "12:00:05 P4 kind=modscript schema=1 gen=1 frame=50 sim=1.000 case=1 id=0"
        " op=0x0FA7 cmd=4007 guardado=0 flags=0 ticks=0",
        "12:00:06 P4 kind=modscript schema=1 gen=1 frame=60 sim=1.200 case=1 id=0"
        " op=0x0FA8 cmd=4008 val=12",
        "12:00:07 P4 kind=modscript schema=1 gen=1 frame=70 sim=1.400 case=1 id=0"
        " op=0x0FA9 cmd=4009 veh=130 grados=90.0000 rumbo=1.5708",
    )
    # 4010 esta dentro del rango 4000-4099 pero no es uno de los 10: es el caso
    # que el motor traza con `desconocido=1` (y el que desincroniza la pila).
    SIN_IMPLEMENTAR = ("12:00:08 P4 kind=modscript schema=1 gen=1 frame=80 sim=1.600 case=1 id=0"
                       " op=0x0FAA cmd=4010 desconocido=1")
    PARTCFG_MOD = ("12:00:01 P4 kind=partcfg schema=1 gen=1 frame=1 sim=0.010 case=2 id=0"
                   " cargados=88 nuevos=5 desconocidos=0 total=88")
    PARTCFG_VANILLA = ("12:00:02 P4 kind=partcfg schema=1 gen=1 frame=1 sim=0.010 case=2 id=0"
                       " cargados=83 nuevos=0 desconocidos=0 total=83")

    def _log(self, *lineas):
        return (["JS build=SYNTHETIC-PLAN120 data=SYNTHETIC-PLAN120", self.IDENT]
                + list(lineas))

    def test_los_siete_opcodes_del_script_son_ok(self):
        estado, motivo = veredicto(CHECK.bloque_modscript, self._log(*self.MODSCRIPT))
        self.assertEqual(estado, "OK", motivo)
        for op in ("0x0FA0", "0x0FA1", "0x0FA2", "0x0FA6", "0x0FA7", "0x0FA8", "0x0FA9"):
            self.assertIn(op.upper() + " x1", motivo,
                          "los 7 usos reales del script deben contarse uno a uno: %s" % motivo)

    def test_opcode_sin_implementar_no_puede_ser_ok(self):
        estado, motivo = veredicto(CHECK.bloque_modscript,
                                   self._log(*self.MODSCRIPT, self.SIN_IMPLEMENTAR))
        self.assertNotEqual(estado, "OK",
                            "un opcode propio sin implementar desincroniza la pila: %s" % motivo)

    def test_particle_cfg_del_mod_es_ok(self):
        estado, motivo = veredicto(CHECK.bloque_partcfg, self._log(self.PARTCFG_MOD))
        self.assertEqual(estado, "OK", motivo)
        self.assertIn("cargados=88", motivo)

    def test_particle_cfg_vanilla_no_puede_ser_ok(self):
        estado, motivo = veredicto(CHECK.bloque_partcfg, self._log(self.PARTCFG_VANILLA))
        self.assertNotEqual(estado, "OK",
                            "el vanilla no trae los 5 tipos del mod: %s" % motivo)

    def test_las_dos_trazas_nuevas_son_evidencia_valida(self):
        ev = CHECK.p4_cargar(self._log(*self.MODSCRIPT, self.PARTCFG_MOD))
        self.assertEqual(ev["invalidos"], [],
                         "las trazas nuevas deben pasar el evaluador estricto: %s"
                         % ev["invalidos"])
        kinds = [e["kind"] for e in ev["eventos"]]
        self.assertIn("modscript", kinds)
        self.assertIn("partcfg", kinds)

    def test_var_sin_numero_en_la_traza_del_script_no_pasa(self):
        roto = self.MODSCRIPT[0].replace("var=3", "var=-")
        ev = CHECK.p4_cargar(self._log(roto))
        self.assertTrue(any("var" in m for m in ev["invalidos"]),
                        "`var` esta en P4_NUM_FINITOS: sin numero es evidencia invalida: %s"
                        % ev["invalidos"])

    def test_sin_esas_trazas_no_revienta_y_son_sin_datos(self):
        ls = self._log()
        self.assertEqual(veredicto(CHECK.bloque_modscript, ls)[0], "SIN DATOS")
        self.assertEqual(veredicto(CHECK.bloque_partcfg, ls)[0], "SIN DATOS")


class P120Volante(BaseSintetica):
    """Plan 12.0 (familia 3, v2.5 «Rotating steering wheel»): el volante del mod
    es evidencia valida del evaluador estricto, y una linea sin sus campos no lo
    es. El multiplicador (1,0) y el eje NO salen de su tutorial, asi que el
    veredicto de la pieza es visual: el bloque solo acredita que se movio, en
    que coche y cuanto.
    """

    IDENT = ("12:00:00 P4 kind=identity schema=1 gen=1 frame=1 sim=0.000 case=0 id=0"
             " version=SYNTHETIC-PLAN120 data=SYNTHETIC-PLAN120")
    VOLANTE = ("12:00:01 P4 kind=wheelrot schema=1 gen=1 frame=10 sim=0.200 case=0 id=0"
               " vk=130 angulo=-0.3489 direc=0.3489")

    def _log(self, *lineas):
        return (["JS build=SYNTHETIC-PLAN120 data=SYNTHETIC-PLAN120", self.IDENT]
                + list(lineas))

    def test_es_evidencia_valida(self):
        ev = CHECK.p4_cargar(self._log(self.VOLANTE))
        self.assertEqual(ev["invalidos"], [],
                         "el volante debe pasar el evaluador estricto: %s" % ev["invalidos"])
        self.assertIn("wheelrot", [e["kind"] for e in ev["eventos"]])

    def test_sin_el_angulo_no_es_evidencia(self):
        roto = self.VOLANTE.replace(" angulo=-0.3489", "")
        ev = CHECK.p4_cargar(self._log(roto))
        self.assertTrue(any("angulo" in m for m in ev["invalidos"]),
                        "`angulo` es obligatorio en P4_REQ: %s" % ev["invalidos"])

    def test_bloque_sin_volante_es_sin_datos(self):
        self.assertEqual(veredicto(CHECK.bloque_wheelrot, self._log())[0], "SIN DATOS")

    def test_bloque_con_volante_es_ok(self):
        estado, motivo = veredicto(CHECK.bloque_wheelrot, self._log(self.VOLANTE))
        self.assertEqual(estado, "OK", motivo)
        self.assertIn("grados", motivo)
        self.assertIn("1 coche", motivo)

    def test_linea_sin_campos_es_parcial(self):
        linea = ("12:00:02 P4 kind=wheelrot schema=1 gen=1 frame=20 sim=0.400 case=0"
                 " id=0 vk=-1")
        estado, motivo = veredicto(CHECK.bloque_wheelrot, self._log(linea))
        self.assertEqual(estado, "PARCIAL", motivo)
        self.assertIn("vk/angulo/direc", motivo)


class P120SensApuntado(BaseSintetica):
    """Plan 12.0 (familia 1, 2506 «Aim sensitivity fix»): el cambio de par de
    sensibilidad (mirar vs apuntar) es evidencia valida del evaluador estricto, y
    una sesion que NUNCA apunta no se puede dar por buena: ahi no hay nada que
    comprobar. El bloque acredita que el cambio ocurre, no el valor elegido (eso es
    el INI del jugador y su ojo).
    """

    IDENT = ("12:00:00 P4 kind=identity schema=1 gen=1 frame=1 sim=0.000 case=0 id=0"
             " version=SYNTHETIC-PLAN120 data=SYNTHETIC-PLAN120")
    MIRANDO = ("12:00:00 P4 kind=aimsens schema=1 gen=1 frame=1 sim=0.020 case=0 id=0"
               " aim=0 sens=0.0013 mfac=1.0000 pfac=1.0000")
    APUNTANDO = ("12:00:01 P4 kind=aimsens schema=1 gen=1 frame=10 sim=0.200 case=0 id=0"
                 " aim=1 sens=0.0008 mfac=0.6668 pfac=0.5000")

    def _log(self, *lineas):
        return (["JS build=SYNTHETIC-PLAN120 data=SYNTHETIC-PLAN120", self.IDENT]
                + list(lineas))

    def test_es_evidencia_valida(self):
        ev = CHECK.p4_cargar(self._log(self.MIRANDO, self.APUNTANDO))
        self.assertEqual(ev["invalidos"], [],
                         "la sensibilidad debe pasar el evaluador estricto: %s" % ev["invalidos"])
        self.assertIn("aimsens", [e["kind"] for e in ev["eventos"]])

    def test_sin_el_factor_no_es_evidencia(self):
        roto = self.APUNTANDO.replace(" mfac=0.6668", "")
        ev = CHECK.p4_cargar(self._log(roto))
        self.assertTrue(any("mfac" in m for m in ev["invalidos"]),
                        "`mfac` es obligatorio en P4_REQ: %s" % ev["invalidos"])

    def test_bloque_sin_trazas_es_sin_datos(self):
        self.assertEqual(veredicto(CHECK.bloque_aimsens, self._log())[0], "SIN DATOS")

    def test_bloque_solo_mirando_es_parcial(self):
        estado, motivo = veredicto(CHECK.bloque_aimsens, self._log(self.MIRANDO))
        self.assertEqual(estado, "PARCIAL", motivo)
        self.assertIn("NINGUNO apuntando", motivo)

    def test_bloque_con_apuntado_es_ok(self):
        estado, motivo = veredicto(CHECK.bloque_aimsens,
                                   self._log(self.MIRANDO, self.APUNTANDO))
        self.assertEqual(estado, "OK", motivo)
        self.assertIn("apuntando 1", motivo)
        self.assertIn("0.6668", motivo)


class P120Luna(BaseSintetica):
    """Plan 12.0 (familia 5, v1.5 "Moving Moon"): la luna sale del rumbo del sol
    es evidencia valida del evaluador estricto, y una linea sin su posicion no lo
    es. El bloque solo acredita que el disco se movio del sitio del vanilla: que
    se vea donde el jugador lo espera es su ojo.
    """

    IDENT = ("12:00:00 P4 kind=identity schema=1 gen=1 frame=1 sim=0.000 case=0 id=0"
             " version=SYNTHETIC-PLAN120 data=SYNTHETIC-PLAN120")
    LUNA = ("12:00:01 P4 kind=moon schema=1 gen=1 frame=10 sim=0.200 case=0 id=0"
            " fadeout=162.1 brillo=121 sol=0.000,0.104,-0.994 pos=0.0,-15.6,149.1")
    LUNA_VANILLA = ("12:00:02 P4 kind=moon schema=1 gen=1 frame=20 sim=0.400 case=0 id=0"
                    " fadeout=170.0 brillo=100 sol=0.000,0.104,-0.994 pos=0.0,-100.0,15.0")

    def _log(self, *lineas):
        return (["JS build=SYNTHETIC-PLAN120 data=SYNTHETIC-PLAN120", self.IDENT]
                + list(lineas))

    def test_es_evidencia_valida(self):
        ev = CHECK.p4_cargar(self._log(self.LUNA))
        self.assertEqual(ev["invalidos"], [],
                         "la luna debe pasar el evaluador estricto: %s" % ev["invalidos"])
        self.assertIn("moon", [e["kind"] for e in ev["eventos"]])

    def test_sin_la_posicion_no_es_evidencia(self):
        roto = self.LUNA.replace(" pos=0.0,-15.6,149.1", "")
        ev = CHECK.p4_cargar(self._log(roto))
        self.assertTrue(any("pos" in m for m in ev["invalidos"]),
                        "`pos` es obligatorio en P4_REQ: %s" % ev["invalidos"])

    def test_bloque_sin_luna_es_sin_datos(self):
        self.assertEqual(veredicto(CHECK.bloque_moon, self._log())[0], "SIN DATOS")

    def test_bloque_con_luna_movida_es_ok(self):
        estado, motivo = veredicto(CHECK.bloque_moon, self._log(self.LUNA))
        self.assertEqual(estado, "OK", motivo)
        self.assertIn("fuera del vanilla", motivo)
        self.assertIn("149.1", motivo)

    def test_bloque_con_la_luna_del_vanilla_es_parcial(self):
        estado, motivo = veredicto(CHECK.bloque_moon, self._log(self.LUNA_VANILLA))
        self.assertEqual(estado, "PARCIAL", motivo)
        self.assertIn("NINGUNA con la luna fuera", motivo)


class P120MundoDelMod(BaseSintetica):
    """Plan 12.0 (familia 7, `VICEEXT_MAPDATA`): el mundo del mod se sirve y el motor
    lo ingiere. El verificador cuenta las zonas de oclusion del `occlu.ipl` SERVIDO y
    exige que el motor tenga al menos esas: menos significa que su mundo no llego (o
    que se sirvio otro fichero), que es el fallo que este bloque existe para cazar.
    """

    IDENT = ("12:00:00 P4 kind=identity schema=1 gen=1 frame=1 sim=0.000 case=0 id=0"
             " version=SYNTHETIC-PLAN120 data=SYNTHETIC-PLAN120")

    def _linea(self, zonas):
        return ("12:00:01 P4 kind=worlddata schema=1 gen=1 frame=2 sim=0.100 case=0"
                " id=0 zonas=%d" % zonas)

    def test_el_mundo_ingerido_es_evidencia_valida(self):
        ev = CHECK.p4_cargar([self.IDENT, self._linea(CHECK.fam7_zonas_servidas()[0])])
        self.assertEqual(ev["invalidos"], [],
                         "la traza worlddata debe pasar el evaluador: %s" % ev["invalidos"])

    def test_con_las_zonas_del_fichero_servido_es_pass(self):
        estado, motivo = veredicto(CHECK.p4_16, CHECK.p4_cargar(
            [self.IDENT, self._linea(CHECK.fam7_zonas_servidas()[0])]))
        self.assertEqual(estado, "PASS", motivo)
        self.assertIn("exactamente las del `occlu.ipl` del mod", motivo)

    def test_menos_zonas_que_el_fichero_servido_es_fallo(self):
        menos = CHECK.fam7_zonas_servidas()[0] - 1
        estado, motivo = veredicto(CHECK.p4_16,
                                   CHECK.p4_cargar([self.IDENT, self._linea(menos)]))
        self.assertEqual(estado, "FALLO", motivo)
        self.assertIn("no todo su mundo llego", motivo)

    def test_sin_la_traza_es_sin_datos(self):
        estado, _ = veredicto(CHECK.p4_16, CHECK.p4_cargar([self.IDENT]))
        self.assertEqual(estado, "SIN DATOS")


class P120FeaturesIni(BaseSintetica):
    """Plan 12.0 (familia 11, `VICEEXT_FEATURES_INI`): el `features.ini` del mod se
    lee al arrancar y sus claves MANDAN. El log sintetico se construye leyendo el
    fichero servido (el mismo que lee el motor), asi que lo que se comprueba es
    que el verificador caza una clave que no cuadra con el fichero, un fichero que
    no se pudo leer y claves que faltan: sin eso, un `inifile` verde no probaria
    nada.
    """

    IDENT = ("12:00:00 P4 kind=identity schema=1 gen=1 frame=1 sim=0.000 case=0 id=0"
             " version=SYNTHETIC-PLAN120 data=SYNTHETIC-PLAN120")

    def _lineas(self, fichero=1, romper=None):
        vals, rgb = CHECK.fam11_ini()
        lineas = [self.IDENT]
        leidas = 0
        for k, impl in CHECK.FAM11_CLAVES:
            if k in CHECK.FAM11_JUGADOR:
                ini = int(vals[k]) if k in vals and vals[k].lstrip("-").isdigit() else 1
                aplica, src = CHECK.FAM11_JUGADOR[k], "jugador"
                if k in vals:
                    leidas += 1
            elif k in vals:
                if k == "WaypointColorRGB":
                    ini = aplica = 1
                else:
                    ini = int(vals[k])
                    aplica = 1 if ini != 0 else 0
                src = "fichero"
                leidas += 1
            else:
                ini, aplica, src = -1, -1, "port"
            if romper == k:
                aplica = 1 - aplica if aplica in (0, 1) else aplica
            lineas.append("12:00:01 P4 kind=inifile schema=1 gen=1 frame=2 sim=0.100 case=0"
                          " id=0 clave=%s ini=%d aplica=%d origen=%s impl=%d"
                          % (k, ini, aplica, src, 1 if impl else 0))
        lineas.append("12:00:02 P4 kind=inifile schema=1 gen=1 frame=2 sim=0.100 case=1"
                      " id=0 fichero=%d leidas=%d rgb=%d,%d,%d"
                      % (fichero, leidas, rgb[0], rgb[1], rgb[2]))
        return lineas

    def test_es_evidencia_valida(self):
        ev = CHECK.p4_cargar(self._lineas())
        self.assertEqual(ev["invalidos"], [],
                         "las trazas inifile deben pasar el evaluador: %s" % ev["invalidos"])
        self.assertEqual(len([e for e in ev["eventos"] if e["kind"] == "inifile"]),
                         len(CHECK.FAM11_CLAVES) + 1)

    def test_una_traza_sin_id_no_es_evidencia(self):
        # Este es el fallo que cazo el primer log del motor (2026-10-14): la traza
        # salia sin `id=` y el evaluador estricto la tiraba entera. El contrato se
        # deja dicho aqui para que no vuelva por la puerta de atras.
        sin_id = self._lineas()[1].replace(" id=0 ", " ")
        ev = CHECK.p4_cargar([self.IDENT, sin_id])
        self.assertTrue(any("campos comunes" in m for m in ev["invalidos"]),
                        "falta `id`: el evaluador tiene que rechazarla: %s" % ev["invalidos"])

    def test_bloque_con_el_fichero_leido_es_ok(self):
        estado, motivo = veredicto(CHECK.p4_15, CHECK.p4_cargar(self._lineas()))
        self.assertEqual(estado, "PASS", motivo)
        self.assertIn("fichero=1", motivo)
        self.assertIn("sin efecto en el motor", motivo)

    def test_fichero_que_no_se_pudo_leer_es_fallo(self):
        estado, motivo = veredicto(CHECK.p4_15, CHECK.p4_cargar(self._lineas(fichero=0)))
        self.assertEqual(estado, "FALLO", motivo)
        self.assertIn("NO pudo leer", motivo)

    def test_una_clave_que_no_cuadra_con_el_fichero_es_fallo(self):
        clave = next(k for k, _ in CHECK.FAM11_CLAVES
                     if k in CHECK.fam11_ini()[0] and k not in CHECK.FAM11_JUGADOR
                     and k != "WaypointColorRGB")
        estado, motivo = veredicto(CHECK.p4_15,
                                   CHECK.p4_cargar(self._lineas(romper=clave)))
        self.assertEqual(estado, "FALLO", motivo)
        self.assertIn(clave, motivo)
        self.assertIn("el fichero dice", motivo)

    def test_una_clave_sin_traza_es_fallo(self):
        lineas = [l for l in self._lineas() if " clave=Doorbell" not in l]
        lineas = [l for l in lineas
                  if " clave=%s " % CHECK.FAM11_CLAVES[0][0] not in l]
        estado, motivo = veredicto(CHECK.p4_15, CHECK.p4_cargar(lineas))
        self.assertEqual(estado, "FALLO", motivo)
        self.assertIn("claves sin traza", motivo)

    def test_sin_trazas_es_sin_datos(self):
        estado, _ = veredicto(CHECK.p4_15, CHECK.p4_cargar([self.IDENT]))
        self.assertEqual(estado, "SIN DATOS")


class P120LucesLejanas(BaseSintetica):
    """Plan 12.0 (familia 5, v2.6 "Distant lights"): la pasada por los dos pools
    con sus efectos 2D es evidencia valida, y una pasada que no encuentra ningun
    efecto 2D no puede dar por buenas las luces: ahi no hay nada que encender.
    """

    IDENT = ("12:00:00 P4 kind=identity schema=1 gen=1 frame=1 sim=0.000 case=0 id=0"
             " version=SYNTHETIC-PLAN120 data=SYNTHETIC-PLAN120")
    MUNDO = ("12:00:01 P4 kind=distlights schema=1 gen=1 frame=10 sim=0.200 case=0 id=0"
             " edificios=1200 dummies=340 conefectos=96")
    SIN_EFECTOS = ("12:00:02 P4 kind=distlights schema=1 gen=1 frame=20 sim=0.400 case=0"
                   " id=0 edificios=1200 dummies=340 conefectos=0")

    def _log(self, *lineas):
        return (["JS build=SYNTHETIC-PLAN120 data=SYNTHETIC-PLAN120", self.IDENT]
                + list(lineas))

    def test_es_evidencia_valida(self):
        ev = CHECK.p4_cargar(self._log(self.MUNDO))
        self.assertEqual(ev["invalidos"], [],
                         "las luces lejanas deben pasar el evaluador estricto: %s"
                         % ev["invalidos"])
        self.assertIn("distlights", [e["kind"] for e in ev["eventos"]])

    def test_sin_los_recuentos_no_es_evidencia(self):
        roto = self.MUNDO.replace(" conefectos=96", "")
        ev = CHECK.p4_cargar(self._log(roto))
        self.assertTrue(any("conefectos" in m for m in ev["invalidos"]),
                        "`conefectos` es obligatorio en P4_REQ: %s" % ev["invalidos"])

    def test_bloque_sin_trazas_es_sin_datos(self):
        self.assertEqual(veredicto(CHECK.bloque_distlights, self._log())[0], "SIN DATOS")

    def test_bloque_con_efectos_es_ok(self):
        estado, motivo = veredicto(CHECK.bloque_distlights, self._log(self.MUNDO))
        self.assertEqual(estado, "OK", motivo)
        self.assertIn("96 con efectos 2D", motivo)

    def test_bloque_sin_efectos_es_parcial(self):
        estado, motivo = veredicto(CHECK.bloque_distlights, self._log(self.SIN_EFECTOS))
        self.assertEqual(estado, "PARCIAL", motivo)
        self.assertIn("no hay luces lejanas que encender", motivo)


class P120PunteriaBanda(BaseSintetica):
    """Plan 12.0 (familia 1, "AI accuracy"): el valor de punteria que el motor
    aplica es evidencia valida del evaluador estricto, y una base de peds en 60
    no puede pasar por buena: es justo el valor del vanilla.
    """

    IDENT = ("12:00:00 P4 kind=identity schema=1 gen=1 frame=1 sim=0.000 case=0 id=0"
             " version=SYNTHETIC-PLAN120 data=SYNTHETIC-PLAN120")
    PED_MOD = ("12:00:01 P4 kind=gangaim schema=1 gen=1 frame=1 sim=0.020 case=0 id=0"
               " tipo=ped acc=73")
    COP_MOD = ("12:00:02 P4 kind=gangaim schema=1 gen=1 frame=2 sim=0.040 case=0 id=0"
               " tipo=cop cop=0 acc=84")
    PED_VANILLA = ("12:00:03 P4 kind=gangaim schema=1 gen=1 frame=3 sim=0.060 case=0"
                   " id=0 tipo=ped acc=60")

    def _log(self, *lineas):
        return (["JS build=SYNTHETIC-PLAN120 data=SYNTHETIC-PLAN120", self.IDENT]
                + list(lineas))

    def test_es_evidencia_valida(self):
        ev = CHECK.p4_cargar(self._log(self.PED_MOD, self.COP_MOD))
        self.assertEqual(ev["invalidos"], [],
                         "la punteria debe pasar el evaluador estricto: %s" % ev["invalidos"])
        self.assertIn("gangaim", [e["kind"] for e in ev["eventos"]])

    def test_sin_la_punteria_no_es_evidencia(self):
        roto = self.PED_MOD.replace(" acc=73", "")
        ev = CHECK.p4_cargar(self._log(roto))
        self.assertTrue(any("acc" in m for m in ev["invalidos"]),
                        "`acc` es obligatorio en P4_REQ: %s" % ev["invalidos"])

    def test_bloque_sin_trazas_es_sin_datos(self):
        self.assertEqual(veredicto(CHECK.bloque_gangaim, self._log())[0], "SIN DATOS")

    def test_bloque_con_la_punteria_del_mod_es_ok(self):
        estado, motivo = veredicto(CHECK.bloque_gangaim,
                                   self._log(self.PED_MOD, self.COP_MOD))
        self.assertEqual(estado, "OK", motivo)
        self.assertIn("base de los peds=73", motivo)
        self.assertIn("policia con punteria propia=1", motivo)

    def test_bloque_con_la_del_vanilla_es_parcial(self):
        estado, motivo = veredicto(CHECK.bloque_gangaim, self._log(self.PED_VANILLA))
        self.assertEqual(estado, "PARCIAL", motivo)
        self.assertIn("la mejora no esta puesta", motivo)


class P120AltSentarse(BaseSintetica):
    """Plan 12.0 (familia 1, ALT/sentarse): la traza de la tecla de andar es la
    evidencia de la mitad que se mide; una accion sin tecla asignada no puede
    pasar por buena, y sin la tecla pulsada no hay nada que juzgar.
    """

    PULSADA = "12:00:01 AIMWALK tecla=1051 spd=0.00"
    SOLTADA = "12:00:02 AIMWALK tecla=1051 spd=0.00"
    NULA = "12:00:03 AIMWALK tecla=1056 spd=0.00"

    def _log(self, *lineas):
        return list(lineas)

    def test_bloque_sin_trazas_es_sin_datos(self):
        self.assertEqual(veredicto(CHECK.bloque_altsit, self._log())[0], "SIN DATOS")

    def test_bloque_con_la_tecla_pulsada_es_ok(self):
        estado, motivo = veredicto(CHECK.bloque_altsit, self._log(self.PULSADA, self.SOLTADA))
        self.assertEqual(estado, "OK", motivo)
        self.assertIn("teclas=[1051]", motivo)

    def test_bloque_con_la_accion_sin_tecla_es_parcial(self):
        estado, motivo = veredicto(CHECK.bloque_altsit, self._log(self.NULA))
        self.assertEqual(estado, "PARCIAL", motivo)
        self.assertIn("rsNULL", motivo)

    def test_bloque_sin_parar_la_velocidad_es_parcial(self):
        linea = "12:00:04 AIMWALK tecla=1051 spd=3.50"
        estado, motivo = veredicto(CHECK.bloque_altsit, self._log(linea))
        self.assertEqual(estado, "PARCIAL", motivo)
        self.assertIn("no esta parando la velocidad", motivo)


if __name__ == "__main__":
    unittest.main(verbosity=2)
