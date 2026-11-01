import math
import sys

HALFPI = math.pi / 2.0
HEADINGS = [0.0, 90.0, 180.0, 270.0]


def forward_from_heading(heading):
    return (-math.sin(heading), math.cos(heading))


def world_front_from_beta(beta):
    return (-math.cos(beta), -math.sin(beta))


def vehicle_front_from_beta(heading, beta):
    forward = forward_from_heading(heading)
    angle = beta + HALFPI
    return (forward[0] * math.cos(angle) - forward[1] * math.sin(angle),
            forward[0] * math.sin(angle) + forward[1] * math.cos(angle))


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1]


def beta_for_branch(heading, relative):
    # Cam.cpp elige la convencion segun la rama que se ejecutara este frame:
    # relativa al coche -> -HALFPI; mundo (autocentrado o apuntando) ->
    # heading - HALFPI, porque Front se reconstruye como (-cos, -sin).
    return -HALFPI if relative else heading - HALFPI


def main():
    failed = False
    for degrees in HEADINGS:
        heading = math.radians(degrees)
        vehicle = forward_from_heading(heading)

        for relative, label in ((True, "relativa"), (False, "mundo   ")):
            beta = beta_for_branch(heading, relative)
            if relative:
                front = vehicle_front_from_beta(heading, beta)
            else:
                front = world_front_from_beta(beta)
            value = dot(front, vehicle)
            status = "ok" if value > 0.99 else "fail"
            if status == "fail":
                failed = True
            print("vehiculo heading=%6.1f rama=%s beta=%8.4f dot=%.6f %s" %
                  (degrees, label, beta, value, status))

        ped_beta = heading - HALFPI
        ped_front = world_front_from_beta(ped_beta)
        value = dot(ped_front, vehicle)
        status = "ok" if value > 0.99 else "fail"
        if status == "fail":
            failed = True
        print("ped      heading=%6.1f rama=mundo    beta=%8.4f dot=%.6f %s" % (degrees, ped_beta, value, status))

    if failed:
        print("FAIL")
        return 1
    print("OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
