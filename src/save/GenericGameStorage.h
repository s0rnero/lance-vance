#pragma once

#include "Game.h"
#include "PCSave.h"

// 8 ranuras visibles (GTAVCsf1..8.b) + la 9 = autosave / quicksave de pausa
// (GTAVCsf9.b). Ver PAUSE_SAVE_SLOT en GenericGameStorage.cpp y VICEEXT_AUTOSAVE.
#define SLOT_COUNT (9)

void InitRadioStationPositionList();
int32 GetSavedRadioStationPosition(int32 station);
void PopulateRadioStationPositionList();
bool GenericSave(int file);
bool GenericLoad();
bool ReadInSizeofSaveFileBuffer(int32 &file, uint32 &size);
bool ReadDataFromFile(int32 file, uint8 *buf, uint32 size);
bool CloseFile(int32 file);
void DoGameSpecificStuffAfterSucessLoad();
#ifdef __EMSCRIPTEN__
extern bool gWebDeferSceneLoad;
extern bool gWebDeferCollision;
void DoGameSpecificStuffAfterSucessLoadDeferred(void);
#endif
bool CheckSlotDataValid(int32 slot);
void MakeSpaceForSizeInBufferPointer(uint8 *&presize, uint8 *&buf, uint8 *&postsize);
void CopySizeAndPreparePointer(uint8 *&buf, uint8 *&postbuf, uint8 *&postbuf2, uint32 &unused, uint32 &size);
void DoGameSpecificStuffBeforeSave();
void MakeValidSaveName(int32 slot);
wchar *GetSavedGameDateAndTime(int32 slot);
wchar *GetNameOfSavedGame(int32 slot);
bool CheckDataNotCorrupt(int32 slot, char *name);
bool RestoreForStartLoad();
#ifdef VICEEXT_SAVE_ANYWHERE
// Sección 3 (Vice Extended, "Saving anywhere"): las tres condiciones del mod
// (sin misión, sin nivel de búsqueda y sin moverse) para guardar donde sea.
bool ViceExtCanSaveAnywhere(void);
#endif
#ifdef VICEEXT_AUTOSAVE
// Sección 3, C2b: verdadero SÓLO mientras el autosave escribe. La ranura 9 (la
// del autoguardado) no se puede pisar con un guardado normal: `MENUACTION_SAVEGAME`
// lo rechaza salvo con esta marca puesta (ver Frontend.cpp).
extern bool ViceExtAutosaveInProgress;
#endif
int align4bytes(int32 size);

#ifdef FIX_INCOMPATIBLE_SAVES
uint8 GetSaveType(char *savename);
bool FixSave(int32 slot, uint8 save_type);
#endif

extern char DefaultPCSaveFileName[260];
extern char ValidSaveName[260];
extern char LoadFileName[256];
extern wchar SlotFileName[SLOT_COUNT][260];
extern wchar SlotSaveDate[SLOT_COUNT][70];
extern int CheckSum;
extern enum eLevelName m_LevelToLoad;
extern int Slots[SLOT_COUNT];

extern bool b_FoundRecentSavedGameWantToLoad;
extern bool JustLoadedDontFadeInYet;
extern bool StillToFadeOut;
extern uint32 TimeStartedCountingForFade;
extern uint32 TimeToStayFadedBeforeFadeOut;

extern char SaveFileNameJustSaved[260]; // 8F2570

const char TopLineEmptyFile[] = "THIS FILE IS NOT VALID YET";

#ifdef MISSION_REPLAY
extern int8 IsQuickSave; // originally int
// Ranura de pausa = 9ª (GTAVCsf9.b): la usan el quicksave/autosave
// (SaveGameForPause) y el reintento de misión del port móvil. Con SLOT_COUNT = 9
// es también la última de las matrices de ranuras (SlotFileName, Slots...).
extern const int PAUSE_SAVE_SLOT;

bool SaveGameForPause(int);

enum {
	SAVE_TYPE_NORMAL,
	SAVE_TYPE_QUICKSAVE,
	SAVE_TYPE_2,
	SAVE_TYPE_QUICKSAVE_FOR_MISSION_REPLAY,
	SAVE_TYPE_QUICKSAVE_FOR_SCRIPT,
	SAVE_TYPE_QUICKSAVE_FOR_SCRIPT_ON_A_MISSION
};

#endif
