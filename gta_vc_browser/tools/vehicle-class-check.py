#!/usr/bin/env python3
"""Comprueba que el cargador completa la clase 8 (motos) sin inaniciar el trafico stock.

No ejecuta el motor: simula la regla anadida en CStreaming::LoadVehicles:

  - una moto por tick mientras NumOfLoadedCarsOfRating[MOTORBIKE] < total;
  - despues, el arranque de clase vacia que ya existia;
  - despues, el maxReq actual.

La prueba falla si la clase 8 no llega a 6/6, si carga mas de una moto por tick,
si toca el stock durante el warm-up o si el stock se queda sin cargar su ultimo
modelo cuando la clase 8 ya esta completa.
"""

import sys

BIKE = 8
BIKE_TOTAL = 6
STOCK = 0
STOCK_TOTAL = 13
MAX_TICKS = 60


def loader_tick(loaded, requests, warmup=True):
    """Un tick del cargador. Devuelve la clase elegida o None."""
    if warmup and loaded.get(BIKE, 0) < BIKE_TOTAL:
        loaded[BIKE] = loaded.get(BIKE, 0) + 1
        return "bike-warmup"

    empty = [c for c in sorted(loaded) if loaded[c] == 0 and requests.get(c, 0) > 0]
    if empty:
        c = empty[0]
        loaded[c] += 1
        requests[c] = 0
        return c

    c = max(loaded, key=lambda k: (requests.get(k, 0), -k))
    total = BIKE_TOTAL if c == BIKE else STOCK_TOTAL
    if requests.get(c, 0) > 0 and loaded[c] < total:
        loaded[c] += 1
        requests[c] = 0
        return c
    return None


def check(name, condition, detail):
    print("%-52s %s (%s)" % (name, "ok" if condition else "fail", detail))
    return condition


def main():
    failed = False

    loaded = {BIKE: 1, STOCK: 12}
    requests = {BIKE: 3, STOCK: 100}
    bike_steps = 0
    for _ in range(MAX_TICKS):
        if loaded[BIKE] >= BIKE_TOTAL:
            break
        before = loaded[BIKE]
        loader_tick(loaded, requests)
        if loaded[BIKE] == before + 1:
            bike_steps += 1
    failed |= not check("clase 8 completa 6/6", loaded[BIKE] == BIKE_TOTAL,
                        "loaded=%d" % loaded[BIKE])
    failed |= not check("una moto por tick", bike_steps == BIKE_TOTAL - 1,
                        "steps=%d" % bike_steps)
    failed |= not check("el warm-up no toca el stock", loaded[STOCK] == 12,
                        "stock=%d" % loaded[STOCK])

    for _ in range(10):
        loader_tick(loaded, requests)
    failed |= not check("el stock carga su ultimo modelo despues", loaded[STOCK] == STOCK_TOTAL,
                        "stock=%d" % loaded[STOCK])
    failed |= not check("la clase 8 no pasa de 6", loaded[BIKE] == BIKE_TOTAL,
                        "loaded=%d" % loaded[BIKE])

    empty_loaded = {BIKE: 0, STOCK: 12}
    empty_requests = {BIKE: 1, STOCK: 100}
    chosen = loader_tick(empty_loaded, empty_requests, warmup=False)
    failed |= not check("clase vacia arranca sin warm-up",
                        chosen == BIKE and empty_loaded[BIKE] == 1,
                        "chosen=%s loaded=%d" % (chosen, empty_loaded[BIKE]))

    print("FAIL" if failed else "OK")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
