p = 'src/audio/AudioManager.cpp'
b = open(p, 'rb').read()
assert b.count(b'\r\n') == b.count(b'\n'), 'expected pure CRLF'

old = (
b'\t\tClearActiveSamples();\r\n'
b'\t\tClearMissionAudio(0);\r\n'
b'\t\tClearMissionAudio(1);\r\n'
b'\t\tSampleManager.StopChannel(CHANNEL_POLICE_RADIO);\r\n'
)
assert b.count(old) == 1, 'ResetTimers anchor'

new = (
b'\t\t// ClearActiveSamples olvida los slots pero no corta el OpenAL: un loop en curso se\r\n'
b'\t\t// quedaba sonando sin nadie que lo vigilara.\r\n'
b'\t\tfor (uint8 i = 0; i < m_nActiveSamples; i++)\r\n'
b'\t\t\tSampleManager.StopChannel(i);\r\n'
b'\t\tClearActiveSamples();\r\n'
b'\t\tClearMissionAudio(0);\r\n'
b'\t\tClearMissionAudio(1);\r\n'
b'\t\tSampleManager.StopChannel(CHANNEL_POLICE_RADIO);\r\n'
)
b = b.replace(old, new, 1)
open(p, 'wb').write(b)
print('patched, bytes=%d' % len(b))
