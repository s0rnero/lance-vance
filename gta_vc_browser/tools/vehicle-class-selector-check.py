import sys

NUM_CAR_CLASSES = 9
MOTORBIKE = 8
MAX_MODELS = 25


def select_class(requests, loaded):
    most = 0
    max_req = -1
    for i in range(NUM_CAR_CLASSES):
        if requests[i] > max_req:
            max_req = requests[i]
            most = i
    for i in range(NUM_CAR_CLASSES):
        if loaded[i] == 0 and requests[i] > 0:
            most = i
            break
    return most


def select_model(models, is_loaded):
    unloaded = [m for m in models if not is_loaded(m)]
    if unloaded:
        return unloaded[0], True
    return models[0], False


def main():
    failed = False
    requests = [100, 0, 0, 0, 0, 0, 0, 0, 1]
    loaded = [10, 1, 1, 1, 1, 1, 1, 1, 0]
    chosen = select_class(requests, loaded)
    if chosen == MOTORBIKE:
        print("selector clase vacia: gana clase %d ok" % chosen)
    else:
        print("selector clase vacia: gano clase %d fail" % chosen)
        failed = True

    requests = [10, 0, 0, 0, 0, 0, 0, 0, 2]
    loaded = [10, 1, 1, 1, 1, 1, 1, 1, 3]
    chosen = select_class(requests, loaded)
    if chosen == 0:
        print("selector sin clase vacia: gana la mas pedida (%d) ok" % chosen)
    else:
        print("selector sin clase vacia: gano %d fail" % chosen)
        failed = True

    models = [6500, 6505, 6506]
    is_loaded = {6500: True, 6505: False, 6506: False}.get
    picked, was_unloaded = select_model(models, is_loaded)
    if was_unloaded and picked == 6505:
        print("modelo sin cargar: elige %d ok" % picked)
    else:
        print("modelo sin cargar: elige %d unloaded=%d fail" % (picked, was_unloaded))
        failed = True

    if failed:
        print("FAIL")
        return 1
    print("OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
