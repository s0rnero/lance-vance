#!/usr/bin/env python3
"""Convierte radios ADF (MP3 con XOR 0x22) a MP3 ligero 22kHz.

Igual que revcDOS (adf2mp3 + ffmpeg 22kHz) pero sin wine: XOR en Python.
Salida: gta_vc_browser/radio_light/*.adf (contienen MP3 plano, mismo nombre).
El motor los acepta por el fallback CADFFile->CMP3File en stream.cpp.

Uso:
  python gta_vc_browser/tools/convert_radio.py [--bitrate 64k] [--only FLASH]
"""
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# assets/ es tu copia legal del juego; no se commitea (ver .gitignore)
VC_AUDIO = os.path.join(ROOT, "assets", "Audio")
OUT_DIR = os.path.join(ROOT, "radio_light")
RADIOS = ["WILD", "FLASH", "KCHAT", "FEVER", "VROCK", "VCPR", "ESPANT", "EMOTION", "WAVE"]

def decode_xor(src, dst):
    with open(src, "rb") as f:
        data = f.read()
    # ADF = MP3 con cada byte ^= 0x22
    out = bytes(b ^ 0x22 for b in data)
    with open(dst, "wb") as f:
        f.write(out)

TALK = {"KCHAT", "VCPR"}  # talk 16kHz mono de origen; no upsamplear

def convert_one(name, bitrate):
    src = os.path.join(VC_AUDIO, name + ".adf")
    if not os.path.exists(src):
        print("FALTA %s, salto" % src)
        return False
    os.makedirs(OUT_DIR, exist_ok=True)
    dst = os.path.join(OUT_DIR, name + ".adf")
    # Si ya existe y es mas nuevo que el original, saltar
    if os.path.exists(dst) and os.path.getmtime(dst) >= os.path.getmtime(src):
        print("%s ya convertido, salto" % name)
        return True
    if name in TALK:
        ar, ac, br = "16000", "1", "48k"
    else:
        ar, ac, br = "22050", "2", bitrate
    with tempfile.TemporaryDirectory() as td:
        dec = os.path.join(td, name + ".dec.mp3")
        decode_xor(src, dec)
        cmd = ["ffmpeg", "-hide_banner", "-loglevel", "error", "-y",
               "-i", dec, "-ar", ar, "-ac", ac, "-b:a", br, "-f", "mp3", dst]
        print(" ".join(cmd))
        subprocess.check_call(cmd)
    s0 = os.path.getsize(src) / (1024 * 1024)
    s1 = os.path.getsize(dst) / (1024 * 1024)
    print("%s: %.1f MB -> %.1f MB" % (name, s0, s1))
    return True

def main():
    bitrate = "64k"
    only = None
    for a in sys.argv[1:]:
        if a.startswith("--bitrate"):
            bitrate = a.split("=", 1)[1] if "=" in a else sys.argv[sys.argv.index(a) + 1]
        elif a.startswith("--only"):
            only = a.split("=", 1)[1] if "=" in a else sys.argv[sys.argv.index(a) + 1]
            only = only.upper()
    names = [only] if only else RADIOS
    ok = 0
    for n in names:
        if convert_one(n, bitrate):
            ok += 1
    print("OK %d/%d en %s" % (ok, len(names), OUT_DIR))

if __name__ == "__main__":
    main()
