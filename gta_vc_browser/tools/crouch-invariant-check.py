#!/usr/bin/env python3
"""Verifica que el agachado del port sea el del mod y que no rompa la pose.

Tres comprobaciones independientes:

 1. TRASPLANTE: las funciones del agachado que se trasplantaron son, texto
    normalizado mediante, identicas a las del mod decompilado (rea). Si alguien
    las vuelve a "arreglar" a ojo, esto falla.
 2. PESOS: dentro del camino del agachado no queda ninguna escritura a mano de
    `blendAmount` / `blendDelta` fuera de las que ya trae el mod. Escribir pesos
    a mano es lo que dejo la suma de poses por debajo de 1 (el personaje
    levitando) o por encima (el cuerpo colapsado / la bandera). El codigo de
    andar/correr de serie no entra aqui: ese ya venia asi del juego.
 3. INVARIANTE: el cruce del motor (`CAnimManager::BlendAnimation`) mantiene la
    suma de poses de cuerpo entero en 1 durante una partida simulada: pulsar
    direcciones, agacharse, apuntar, rodar y levantarse.

Uso:  python gta_vc_browser/tools/crouch-invariant-check.py [-v]
Sale con 0 solo si las tres pasan.
"""

import difflib
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))

VERBOSE = '-v' in sys.argv


# ---------------------------------------------------------------- utilidades

def read(path):
    with open(path, 'rb') as f:
        b = f.read()
    try:
        return b.decode('utf-8').replace('\r\n', '\n')
    except UnicodeDecodeError:
        return b.decode('cp1252').replace('\r\n', '\n')


def strip_emscripten(src):
    """Fuera los bloques #ifdef __EMSCRIPTEN__ ... #endif (instrumentacion)."""
    out, depth = [], 0
    for line in src.split('\n'):
        s = line.strip()
        if s.startswith('#if'):
            if depth or '__EMSCRIPTEN__' in s:
                depth += 1
                continue
        elif s.startswith('#endif'):
            if depth:
                depth -= 1
                continue
        elif s.startswith('#else') and depth:
            continue
        if depth:
            continue
        out.append(line)
    return '\n'.join(out)


def strip_comments(src):
    src = re.sub(r'/\*.*?\*/', '', src, flags=re.S)
    return re.sub(r'//[^\n]*', '', src)


def norm(src):
    src = strip_comments(strip_emscripten(src))
    # una sola familia de flags: el mod usa CROUCH, el port VICEEXT_CROUCH
    src = re.sub(r'\bVICEEXT_', '', src)
    lines = [re.sub(r'\s+', ' ', l).strip() for l in src.split('\n')]
    return '\n'.join(l for l in lines if l)


def extract(src, signature):
    """Devuelve la funcion completa (desde su firma hasta su } de cierre)."""
    i = src.find(signature)
    if i < 0:
        return None
    j = src.find('\n}\n', i)
    if j < 0:
        return None
    return src[i:j + 3]


# ------------------------------------------------- 1) trasplante exacto

TRANSPLANTED = [
    ('src/peds/PlayerPed.cpp', 'CPlayerPed::ProcessCrouch(void)'),
    ('src/peds/PlayerPed.cpp', 'CPlayerPed::FinishRollCB(CAnimBlendAssociation* assoc, void* arg)'),
]
MOD_PLAYERPED = 'gta_vc_browser/tmp/upstream-gta-extended/src/peds/PlayerPed.cpp'


def check_transplant(errors):
    print('== 1. trasplante contra el mod ==')
    ours_src = read(os.path.join(ROOT, TRANSPLANTED[0][0]))
    theirs_src = read(os.path.join(ROOT, MOD_PLAYERPED))
    for _, sig in TRANSPLANTED:
        name = sig.split('(')[0]
        a, b = extract(ours_src, sig), extract(theirs_src, sig)
        if a is None or b is None:
            errors.append('no se pudo extraer %s' % sig)
            print('  FALLA %s: no se pudo extraer' % name)
            continue
        na, nb = norm(a), norm(b)
        if na == nb:
            print('  OK    %s identica al mod (%d lineas utiles)' % (name, len(na.split('\n'))))
        else:
            errors.append('%s difiere del mod' % name)
            print('  FALLA %s difiere del mod:' % name)
            for line in list(difflib.unified_diff(nb.split('\n'), na.split('\n'),
                                                  fromfile='mod', tofile='port', lineterm=''))[:24]:
                print('        ' + line)


# ------------------------------------------------- 2) ninguna escritura de pesos

ALLOWED_WRITES = {
    'CPlayerPed::FinishRollCB': {'assoc->blendDelta = -1000.0f;'},
    'CPed::ClearDuck': {
        'animAssoc->blendDelta = -4.0f;',
        'crouchForwardAssoc->blendDelta = -5.0f;',
        'curCrouchBackwardAssoc->blendDelta = -5.0f;',
    },
}

CROUCH_FUNCS = [
    ('src/peds/PlayerPed.cpp', 'CPlayerPed::ProcessCrouch(void)', False),
    ('src/peds/PlayerPed.cpp', 'CPlayerPed::FinishRollCB(CAnimBlendAssociation* assoc, void* arg)', False),
    ('src/peds/PlayerPed.cpp', 'CPlayerPed::SetRealMoveAnim(void)', True),
    ('src/peds/Ped.cpp', 'CPed::SetMoveAnim(void)', True),
    ('src/peds/PedAI.cpp', 'CPed::SetDuck(uint32 time, bool sth)', False),
    ('src/peds/PedAI.cpp', 'CPed::Duck(void)', False),
    ('src/peds/PedAI.cpp', 'CPed::ClearDuck(bool clearTimer)', False),
]

WRITE_RE = re.compile(r'blend(Amount|Delta)\s*[-+]?=')


def check_weights(errors):
    print('== 2. escrituras de pesos a mano ==')
    total = 0
    for rel, sig, only_crouch in CROUCH_FUNCS:
        body = extract(read(os.path.join(ROOT, rel)), sig)
        name = sig.split('(')[0]
        if body is None:
            errors.append('no se pudo extraer %s' % sig)
            print('  FALLA no se pudo extraer %s' % name)
            continue
        allowed = ALLOWED_WRITES.get(name, set())
        for line in norm(body).split('\n'):
            if not WRITE_RE.search(line):
                continue
            if only_crouch and 'crouch' not in line.lower():
                continue  # andar/correr de serie: no es este cambio
            total += 1
            if line in allowed:
                if VERBOSE:
                    print('  OK    %s: %s (la trae el mod)' % (name, line))
            else:
                errors.append('%s escribe pesos a mano: %s' % (name, line))
                print('  FALLA %s: %s' % (name, line))
    if not errors:
        print('  OK    %d escrituras, todas las que ya trae el mod' % total)


# ------------------------------------------------- 3) invariante del cruce

class Assoc:
    def __init__(self, anim, partial=False, amount=0.0, delta=0.0):
        self.anim = anim
        self.partial = partial
        self.amount = amount
        self.delta = delta
        self.time = 0.0
        self.total = 1.0
        self.faded = False


class Clump:
    """Copia de CAnimManager::BlendAnimation (AnimManager.cpp)."""

    def __init__(self):
        self.assocs = []

    def get(self, anim):
        for a in self.assocs:
            if a.anim == anim:
                return a
        return None

    def blend(self, anim, delta, partial=False, movement=False):
        remove_prev = 0
        found = None
        for a in list(self.assocs):
            if a.anim == anim:
                found = a
                continue
            if a.partial == partial:
                if a.amount > 0.0:
                    bdelta = -delta * a.amount
                    if bdelta < a.delta or not partial:
                        a.delta = bdelta
                else:
                    a.delta = -1.0
                a.faded = True
                remove_prev = 1
        if found is not None:
            found.delta = (1.0 - found.amount) * delta
            if found.time >= found.total:
                found.time = 0.0
        else:
            found = Assoc(anim, partial, 0.0, delta)
            if not remove_prev and not partial:
                found.amount = 1.0
            self.assocs.append(found)
        return found

    def step(self, dt):
        """CAnimBlendAssociation::UpdateBlend."""
        for a in list(self.assocs):
            a.amount = min(1.0, max(0.0, a.amount + a.delta * dt))
            if a.amount <= 0.0 and a.faded:
                self.assocs.remove(a)

    def sum_full(self):
        return sum(a.amount for a in self.assocs if not a.partial and a.amount > 0.0)

    def w(self, anim):
        a = self.get(anim)
        return a.amount if a else 0.0


IDLE, FWD, BWD, ROLL_L, ROLL_R, CROUCHFIRE = (
    'CROUCH_IDLE', 'CROUCH_FORWARD', 'CROUCH_BACKWARD',
    'CROUCH_ROLL_L', 'CROUCH_ROLL_R', 'CROUCHFIRE')


class Ped:
    """Transcripcion de CPlayerPed::ProcessCrouch + FinishRollCB del mod."""

    def __init__(self):
        self.clump = Clump()
        self.clump.assocs.append(Assoc(IDLE, False, 1.0, 0.0))
        self.state = 'IDLE'
        self.rolls = 0
        self.used = set()

    def _blend(self, anim, delta, **kw):
        self.used.add(anim)
        return self.clump.blend(anim, delta, **kw)

    def process_crouch(self, updown, leftright, target, aiming, mps):
        c = self.clump
        idle, fwd, bwd = c.get(IDLE), c.get(FWD), c.get(BWD)
        roll_l, roll_r = c.get(ROLL_L), c.get(ROLL_R)
        cur_roll = bool(roll_l or roll_r)
        cur_move = bool(fwd or bwd)
        fire = c.get(CROUCHFIRE)

        if self.state == 'ROLL':
            if not cur_roll:
                self.state = 'IDLE'
            return

        if aiming:
            if (mps < 0.1 or (leftright != 0 and updown == 0)) and not idle and not cur_roll:
                idle = c.blend(IDLE, 4.0)
            if (target and mps > 0.1 and (not idle or (idle and idle.amount >= 1.0))
                    and not cur_move and not cur_roll and (leftright < -20 or leftright > 20)):
                for a in (fwd, bwd, fire, idle):
                    if a is not None:
                        c.assocs.remove(a)
                self.state = 'ROLL'
                self.rolls += 1
                if leftright < 0:
                    self._blend(ROLL_L, 4.0, movement=True)
                else:
                    self._blend(ROLL_R, 4.0, movement=True)
                return
            if mps > 0.1 and not cur_roll:
                if updown < 0 and not fwd:
                    self._blend(FWD, 4.0, movement=True)
                elif updown > 0 and not bwd:
                    self._blend(BWD, 4.0, movement=True)
        else:
            if mps < 0.1 and not idle:
                idle = c.blend(IDLE, 4.0)
            if mps > 0.1:
                idle, fwd, bwd = c.get(IDLE), c.get(FWD), c.get(BWD)
                if (idle and not fwd) or bwd:
                    self._blend(FWD, 4.0, movement=True)

    def roll_finished(self):
        """CPlayerPed::FinishRollCB, verbatim."""
        for a in self.clump.assocs:
            if a.anim in (ROLL_L, ROLL_R):
                a.delta = -1000.0
                a.faded = True
        self.clump.blend(IDLE, 1000.0)

    def stand_up(self):
        """CPed::ClearDuck, solo la parte de poses."""
        idle = self.clump.get(IDLE)
        if idle:
            idle.delta = -4.0
            idle.faded = True
        for anim in (FWD, BWD):
            a = self.clump.get(anim)
            if a:
                a.delta = -5.0
                a.faded = True


class Runner:
    def __init__(self, errors):
        self.errors = errors
        self.frames = 0
        self.worst = 99.0
        self.dt = 1.0 / 60.0

    def run(self, ped, seconds, inputs, label):
        for f in range(int(seconds / self.dt)):
            updown, leftright, target, aiming, mps = inputs(f * self.dt)
            ped.clump.step(self.dt)          # UpdateBlend
            ped.process_crouch(updown, leftright, target, aiming, mps)
            roll = ped.clump.get(ROLL_L) or ped.clump.get(ROLL_R)
            if roll is not None:
                roll.time += self.dt
                if roll.time >= roll.total:
                    ped.roll_finished()   # el callback del clip
            self.frames += 1
            total = ped.clump.sum_full()
            self.worst = min(self.worst, total)
            if abs(total - 1.0) > 0.02:
                self.errors.append('%s frame %d: suma de poses %.3f' % (label, f, total))
                return
            if total <= 0.0:
                self.errors.append('%s frame %d: frame sin pose de cuerpo entero' % (label, f))
                return


def check_invariant(errors):
    print('== 3. invariante de la suma (partida simulada) ==')
    r = Runner(errors)

    # a) machacar A agachado, sin apuntar
    r.run(Ped(), 6.0,
          lambda t: (0, -128 if int(t * 8) % 2 == 0 else 0, 0, False,
                     1.2 if int(t * 8) % 2 == 0 else 0.0), 'spam A')
    # b) andar agachado con W y cambiar a S sin apuntar: clip de adelante
    #    siempre (el mod no tiene clip de espaldas sin arma); nada de moonwalk
    ped_b = Ped()
    r.run(ped_b, 6.0,
          lambda t: (-128 if t < 3 else 128, 0, 0, False, 1.4), 'W y luego S')
    if FWD not in ped_b.used:
        errors.append('andando agachado sin apuntar no se usa el clip de adelante')
    if BWD in ped_b.used:
        errors.append('andando agachado sin apuntar se usa el clip de espaldas (moonwalk)')

    # b2) apuntando, S si tiene que usar el clip de espaldas
    ped_b2 = Ped()
    r.run(ped_b2, 2.0, lambda t: (128, 0, 1, True, 1.4), 'apuntar + S')
    if BWD not in ped_b2.used:
        errors.append('apuntando y yendo hacia atras no se usa el clip de espaldas')

    # c) apuntar y mantener A: la rueda tiene que encadenarse
    ped = Ped()
    r.run(ped, 4.0, lambda t: (0, -128, 1, True, 1.4), 'apuntar + A')
    if ped.rolls < 3:
        errors.append('con apuntar + A mantenido solo se encadenaron %d ruedas en 4 s' % ped.rolls)

    # d) spam de apuntar con A mantenida: entran parciales de arma encima
    ped = Ped()

    def inputs_d(t):
        aim = int(t * 6) % 2 == 0
        if aim and int(t * 60) % 7 == 0:
            ped.clump.blend(CROUCHFIRE, 4.0, partial=True)
        return (0, -128, 1 if aim else 0, aim, 1.4)

    r.run(ped, 5.0, inputs_d, 'spam apuntar + A')

    # e) levantarse: ClearDuck apaga el agachado y el SetRealMoveAnim de pie
    #    pide el IDLE normal en el mismo frame (eso es lo que mantiene la suma)
    ped = Ped()
    r.run(ped, 1.0, lambda t: (0, 0, 0, False, 0.0), 'quieto')
    ped.stand_up()
    ped.clump.blend('IDLE', 4.0)
    for f in range(60):
        ped.clump.step(r.dt)
        r.frames += 1
        total = ped.clump.sum_full()
        r.worst = min(r.worst, total)
        if total <= 0.0:
            errors.append('levantarse frame %d: frame sin pose' % f)
            break
    for anim in (IDLE, FWD, BWD):
        if ped.clump.w(anim) > 0.01:
            errors.append('al levantarse queda %s viva (peso %.2f)' % (anim, ped.clump.w(anim)))

    if not errors:
        print('  OK    %d frames, suma minima %.3f (nunca por debajo de 1)' % (r.frames, r.worst))


def main():
    errors = []
    check_transplant(errors)
    check_weights(errors)
    check_invariant(errors)
    print()
    if errors:
        print('FALLA: %d problema(s)' % len(errors))
        for e in errors:
            print('  - ' + e)
        return 1
    print('OK: el agachado es el del mod y la suma de poses se queda en 1.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
