import io, sys

p = 'src/audio/oal/channel.cpp'
b = open(p, 'rb').read()
assert b.count(b'\r\n') == b.count(b'\n'), 'expected pure CRLF'

old_inc = b'#include "channel.h"\r\n#include "sampman.h"\r\n'
assert b.count(old_inc) == 1, 'include anchor'
new_inc = b'#include "ondemand.h"\r\n' + old_inc
b = b.replace(old_inc, new_inc, 1)

old = (
b'\t\talBufferData(alBuffers[id], AL_FORMAT_MONO16, Data, DataSize, Frequency);\r\n'
b'\tif ( LoopPoints[0] != 0 && LoopPoints[0] != -1 )\r\n'
b'\t\talBufferiv(alBuffers[id], AL_LOOP_POINTS_SOFT, LoopPoints);\r\n'
b'\talSourcei(alSources[id], AL_BUFFER, alBuffers[id]);\r\n'
)
assert b.count(old) == 1, 'start anchor'

new = (
b'\t\talBufferData(alBuffers[id], AL_FORMAT_MONO16, Data, DataSize, Frequency);\r\n'
b'\t// El buffer se reutiliza entre samples: escribir los puntos solo cuando vienen pedidos\r\n'
b'\t// hace que un loop herede la region del sample anterior y suene repetido mal.\r\n'
b'\tALint last = ALint(DataSize / 2) - 1;\r\n'
b'\tif ( last > 0 )\r\n'
b'\t{\r\n'
b'\t\tALint lp[2];\r\n'
b'\t\tlp[0] = LoopPoints[0] > 0 ? LoopPoints[0] : 0;\r\n'
b'\t\tif ( lp[0] > last ) lp[0] = last;\r\n'
b'\t\tlp[1] = LoopPoints[1] > 0 && LoopPoints[1] < last ? LoopPoints[1] : last;\r\n'
b'\t\tif ( lp[1] < lp[0] ) lp[1] = last;\r\n'
b'\t\talBufferiv(alBuffers[id], AL_LOOP_POINTS_SOFT, lp);\r\n'
b'#ifdef __EMSCRIPTEN__\r\n'
b'\t\t{\r\n'
b'\t\t\tstatic uint32 s_odLoopPtsN;\r\n'
b'\t\t\ts_odLoopPtsN++;\r\n'
b'\t\t\tif ( LoopCount != 1 && (s_odLoopPtsN <= 60u || s_odLoopPtsN % 200u == 0u) )\r\n'
b'\t\t\t{\r\n'
b'\t\t\t\tchar t[112];\r\n'
b'\t\t\t\tsnprintf(t, sizeof t, "LOOPSET ch=%u nsamp=%d lp=%d,%d freq=%d cnt=%d",\r\n'
b'\t\t\t\t\tid, last + 1, lp[0], lp[1], Frequency, LoopCount);\r\n'
b'\t\t\t\tODTRACES(t);\r\n'
b'\t\t\t}\r\n'
b'\t\t}\r\n'
b'#endif\r\n'
b'\t}\r\n'
b'\talSourcei(alSources[id], AL_BUFFER, alBuffers[id]);\r\n'
)
b = b.replace(old, new, 1)

assert b.count(b'LOOPSET ch=') == 1
assert b.count(b'AL_LOOP_POINTS_SOFT') == 1
assert b.count(b'LoopPoints[0] != 0 && LoopPoints[0] != -1') == 0
open(p, 'wb').write(b)
print('patched, bytes=%d' % len(b))
