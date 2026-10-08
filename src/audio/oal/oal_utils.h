#pragma once

#ifdef AUDIO_OAL
#include "eax.h"
#ifndef __EMSCRIPTEN__
#include "AL/efx.h"
#else
// Web: el SDK no trae AL/efx.h ni hay EFX en el navegador.
// Declaraciones mínimas para compilar; los punteros quedan a NULL y las
// funciones son no-op (IsFXSupported()==false y sin ALC_EXT_EFX en Web Audio).
#include <AL/al.h>
#include <AL/alc.h>
#ifndef AL_EFFECT_NULL
#define AL_EFFECT_NULL 0x0000
#endif
#ifndef AL_EFFECTSLOT_NULL
#define AL_EFFECTSLOT_NULL 0x0000
#endif
#ifndef AL_EFFECT_TYPE
#define AL_EFFECT_TYPE 0x8001
#endif
#ifndef AL_EFFECTSLOT_EFFECT
#define AL_EFFECTSLOT_EFFECT 0x0001
#endif
#ifndef AL_FILTER_NULL
#define AL_FILTER_NULL 0x0000
#endif
#ifndef AL_AUXILIARY_SEND_FILTER
#define AL_AUXILIARY_SEND_FILTER 0x20006
#endif
#ifndef ALC_EXT_EFX_NAME
#define ALC_EXT_EFX_NAME "ALC_EXT_EFX"
#endif
typedef void (*LPALGENEFFECTS)(ALsizei, ALuint*);
typedef void (*LPALDELETEEFFECTS)(ALsizei, ALuint*);
typedef ALboolean (*LPALISEFFECT)(ALuint);
typedef void (*LPALEFFECTI)(ALuint, ALenum, ALint);
typedef void (*LPALEFFECTIV)(ALuint, ALenum, ALint*);
typedef void (*LPALEFFECTF)(ALuint, ALenum, ALfloat);
typedef void (*LPALEFFECTFV)(ALuint, ALenum, ALfloat*);
typedef void (*LPALGETEFFECTI)(ALuint, ALenum, ALint*);
typedef void (*LPALGETEFFECTIV)(ALuint, ALenum, ALint*);
typedef void (*LPALGETEFFECTF)(ALuint, ALenum, ALfloat*);
typedef void (*LPALGETEFFECTFV)(ALuint, ALenum, ALfloat*);
typedef void (*LPALGENAUXILIARYEFFECTSLOTS)(ALsizei, ALuint*);
typedef void (*LPALDELETEAUXILIARYEFFECTSLOTS)(ALsizei, ALuint*);
typedef ALboolean (*LPALISAUXILIARYEFFECTSLOT)(ALuint);
typedef void (*LPALAUXILIARYEFFECTSLOTI)(ALuint, ALenum, ALint);
typedef void (*LPALAUXILIARYEFFECTSLOTIV)(ALuint, ALenum, ALint*);
typedef void (*LPALAUXILIARYEFFECTSLOTF)(ALuint, ALenum, ALfloat);
typedef void (*LPALAUXILIARYEFFECTSLOTFV)(ALuint, ALenum, ALfloat*);
typedef void (*LPALGETAUXILIARYEFFECTSLOTI)(ALuint, ALenum, ALint*);
typedef void (*LPALGETAUXILIARYEFFECTSLOTIV)(ALuint, ALenum, ALint*);
typedef void (*LPALGETAUXILIARYEFFECTSLOTF)(ALuint, ALenum, ALfloat*);
typedef void (*LPALGETAUXILIARYEFFECTSLOTFV)(ALuint, ALenum, ALfloat*);
typedef void (*LPALGENFILTERS)(ALsizei, ALuint*);
typedef void (*LPALDELETEFILTERS)(ALsizei, ALuint*);
typedef ALboolean (*LPALISFILTER)(ALuint);
typedef void (*LPALFILTERI)(ALuint, ALenum, ALint);
typedef void (*LPALFILTERIV)(ALuint, ALenum, ALint*);
typedef void (*LPALFILTERF)(ALuint, ALenum, ALfloat);
typedef void (*LPALFILTERFV)(ALuint, ALenum, ALfloat*);
typedef void (*LPALGETFILTERI)(ALuint, ALenum, ALint*);
typedef void (*LPALGETFILTERIV)(ALuint, ALenum, ALint*);
typedef void (*LPALGETFILTERF)(ALuint, ALenum, ALfloat*);
typedef void (*LPALGETFILTERFV)(ALuint, ALenum, ALfloat*);
#endif // __EMSCRIPTEN__


void EFXInit();
void EAX3_Set(ALuint effect, const EAXLISTENERPROPERTIES *props);
void EFX_Set(ALuint effect, const EAXLISTENERPROPERTIES *props);
void EAX3_SetReverbMix(ALuint filter, float mix);
void SetEffectsLevel(ALuint uiFilter, float level);

namespace re3_openal {

extern LPALGENEFFECTS alGenEffects;
extern LPALDELETEEFFECTS alDeleteEffects;
extern LPALISEFFECT alIsEffect;
extern LPALEFFECTI alEffecti;
extern LPALEFFECTIV alEffectiv;
extern LPALEFFECTF alEffectf;
extern LPALEFFECTFV alEffectfv;
extern LPALGETEFFECTI alGetEffecti;
extern LPALGETEFFECTIV alGetEffectiv;
extern LPALGETEFFECTF alGetEffectf;
extern LPALGETEFFECTFV alGetEffectfv;
extern LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
extern LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
extern LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot;
extern LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti;
extern LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv;
extern LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf;
extern LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv;
extern LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti;
extern LPALGETAUXILIARYEFFECTSLOTIV alGetAuxiliaryEffectSlotiv;
extern LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf;
extern LPALGETAUXILIARYEFFECTSLOTFV alGetAuxiliaryEffectSlotfv;
extern LPALGENFILTERS alGenFilters;
extern LPALDELETEFILTERS alDeleteFilters;
extern LPALISFILTER alIsFilter;
extern LPALFILTERI alFilteri;
extern LPALFILTERIV alFilteriv;
extern LPALFILTERF alFilterf;
extern LPALFILTERFV alFilterfv;
extern LPALGETFILTERI alGetFilteri;
extern LPALGETFILTERIV alGetFilteriv;
extern LPALGETFILTERF alGetFilterf;
extern LPALGETFILTERFV alGetFilterfv;

}

using namespace re3_openal;

#endif
