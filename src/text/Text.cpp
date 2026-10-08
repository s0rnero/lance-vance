#include "common.h"
#include "ondemand.h" // D2: traza TXTMISS (claves de texto que no existen)

#include "FileMgr.h"
#ifdef MORE_LANGUAGES
#include "Game.h"
#endif
#include "Frontend.h"
#include "Messages.h"
#include "Text.h"
#include "Timer.h"

wchar WideErrorString[25];

CText TheText;

CText::CText(void)
{
	encoding = 'e';
	bHasMissionTextOffsets = false;
	bIsMissionTextLoaded = false;
	memset(szMissionTableName, 0, sizeof(szMissionTableName));
	memset(WideErrorString, 0, sizeof(WideErrorString));
}

// D18 (sección 1, 21/09): nombre del fichero de textos segun el idioma.
//
// Antes esto era un switch que rellenaba `filename` sin case por defecto: si
// m_PrefsLanguage no caia en ninguno (idioma no soportado, valor sin
// inicializar) el buffer se usaba SIN INICIALIZAR, OpenFile fallaba, el
// descriptor era 0 y el bucle de chunks -que nunca comprobaba ni la apertura
// ni el fin de fichero- giraba para siempre sobre la misma cabecera: pantalla
// negra y ni un solo error. Con el default siempre hay un nombre valido.
static const char*
OdTextFileName(void)
{
	switch (FrontEndMenuManager.m_PrefsLanguage) {
	case CMenuManager::LANGUAGE_AMERICAN: return "AMERICAN.GXT";
	case CMenuManager::LANGUAGE_FRENCH:   return "FRENCH.GXT";
	case CMenuManager::LANGUAGE_GERMAN:   return "GERMAN.GXT";
	case CMenuManager::LANGUAGE_ITALIAN:  return "ITALIAN.GXT";
	case CMenuManager::LANGUAGE_SPANISH:  return "SPANISH.GXT";
#ifdef MORE_LANGUAGES
	case CMenuManager::LANGUAGE_POLISH:   return "POLISH.GXT";
	case CMenuManager::LANGUAGE_RUSSIAN:  return "RUSSIAN.GXT";
	case CMenuManager::LANGUAGE_JAPANESE: return "JAPANESE.GXT";
#endif
	default:                              return "AMERICAN.GXT";
	}
}

// D18: un GXT que falta (o al que le falta un chunk) no puede colgar el juego.
// Deja constancia en la consola del navegador y en odtrace.log y sigue.
static void
OdTextFail(const char *who, const char *filename)
{
	printf("%s - no se pudo leer %s (textos sin cargar)\n", who, filename);
#ifdef __EMSCRIPTEN__
	{
		char t[160];
		snprintf(t, sizeof t, "TXTGXTFAIL %s file=%s", who, filename);
		ODTRACES(t);
	}
#endif
}

void
CText::Load(void)
{
	bool tkey_loaded = false, tdat_loaded = false;
	ChunkHeader m_ChunkHeader;

	bIsMissionTextLoaded = false;
	bHasMissionTextOffsets = false;

	Unload();

	CFileMgr::SetDir("TEXT");
	const char *filename = OdTextFileName();

	size_t offset = 0;
	int file = CFileMgr::OpenFile(filename, "rb");
	if (file == 0) {
		OdTextFail("CText::Load", filename);
		CFileMgr::SetDir("");
		return;
	}

	while (!tkey_loaded || !tdat_loaded) {
		size_t got = ReadChunkHeader(&m_ChunkHeader, file, &offset);
		// D18: fin de fichero o cabecera ilegible -> fuera del bucle. Sin esta
		// salida, un GXT truncado o ausente giraba para siempre (el `size != 0`
		// de abajo no protegia: con size 0 el cuerpo se saltaba y se releia la
		// MISMA cabecera).
		if (got != sizeof(ChunkHeader) || m_ChunkHeader.size == 0)
			break;
		if (strncmp(m_ChunkHeader.magic, "TABL", 4) == 0) {
			MissionTextOffsets.Load(m_ChunkHeader.size, file, &offset, 0x58000);
			bHasMissionTextOffsets = true;
		} else if (strncmp(m_ChunkHeader.magic, "TKEY", 4) == 0) {
			// D19: la tabla solo vale si se leyo ENTERA (si no, la cola es basura).
			tkey_loaded = this->keyArray.Load(m_ChunkHeader.size, file, &offset) == (size_t)m_ChunkHeader.size;
		} else if (strncmp(m_ChunkHeader.magic, "TDAT", 4) == 0) {
			tdat_loaded = this->data.Load(m_ChunkHeader.size, file, &offset) == (size_t)m_ChunkHeader.size;
		} else {
			CFileMgr::Seek(file, m_ChunkHeader.size, SEEK_CUR);
			offset += m_ChunkHeader.size;
		}
	}

	if (tkey_loaded && tdat_loaded)
		keyArray.Update(data.chars);
	else
		OdTextFail("CText::Load", filename);
	CFileMgr::CloseFile(file);
	CFileMgr::SetDir("");
}

void
CText::Unload(void)
{
	CMessages::ClearAllMessagesDisplayedByGame();
	keyArray.Unload();
	data.Unload();
	mission_keyArray.Unload();
	mission_data.Unload();
	bIsMissionTextLoaded = false;
	memset(szMissionTableName, 0, sizeof(szMissionTableName));
}

wchar*
CText::Get(const char *key)
{
	uint8 result = false;
#if defined (FIX_BUGS) || defined(FIX_BUGS_64)
	wchar *outstr = keyArray.Search(key, data.chars, &result);
#else
	wchar *outstr = keyArray.Search(key, &result);
#endif

	if (!result && bHasMissionTextOffsets && bIsMissionTextLoaded)
#if defined (FIX_BUGS) || defined(FIX_BUGS_64)
		outstr = mission_keyArray.Search(key, mission_data.chars, &result);
#else
		outstr = mission_keyArray.Search(key, &result);
#endif
#ifdef __EMSCRIPTEN__
	// D2/D19 (seccion 1, 21/09): "missing" SOLO si la clave no existe en NINGUNA
	// de las dos tablas. Antes se trazaba cada fallo de la global, y las claves
	// de mision (INTRO1..INTRO4) viven en la tabla de mision: el log se llenaba
	// de "missing" de claves que dos lineas despues si resolvian.
	if (!result) {
		static uint32 odMissing = 0;
		if (odMissing++ < 300) {
			char t[192];
			snprintf(t, sizeof t, "TXTMISS key=%s nglobal=%d nmision=%d", key,
				keyArray.numEntries, bIsMissionTextLoaded ? mission_keyArray.numEntries : -1);
			ODTRACES(t);
		}
	}
#endif
	return outstr;
}

wchar UpperCaseTable[128] = {
	128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138,
	139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
	150, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137,
	138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148,
	149, 173, 173, 175, 176, 177, 178, 179, 180, 181, 182,
	183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193,
	194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204,
	205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215,
	216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226,
	227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237,
	238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248,
	249, 250, 251, 252, 253, 254, 255
};

wchar FrenchUpperCaseTable[128] = {
	128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138,
	139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
	150, 65, 65, 65, 65, 132, 133, 69, 69, 69, 69, 73, 73,
	73, 73, 79, 79, 79, 79, 85, 85, 85, 85, 173, 173, 175,
	176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186,
	187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197,
	198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208,
	209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219,
	220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230,
	231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241,
	242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252,
	253, 254, 255
};

wchar
CText::GetUpperCase(wchar c)
{
	switch (encoding)
	{
	case 'e':
		if (c >= 'a' && c <= 'z')
			return c - 32;
		break;
	case 'f':
		if (c >= 'a' && c <= 'z')
			return c - 32;

		if (c >= 128 && c <= 255)
			return FrenchUpperCaseTable[c-128];
		break;
	case 'g':
	case 'i':
	case 's':
		if (c >= 'a' && c <= 'z')
			return c - 32;

		if (c >= 128 && c <= 255)
			return UpperCaseTable[c-128];
		break;
	default:
		break;
	}
	return c;
}

void
CText::UpperCase(wchar *s)
{
	while(*s){
		*s = GetUpperCase(*s);
		s++;
	}
}

void
CText::GetNameOfLoadedMissionText(char *outName)
{
	strcpy(outName, szMissionTableName);
}

size_t
CText::ReadChunkHeader(ChunkHeader *buf, int32 file, size_t *offset)
{
#ifdef THIS_IS_STUPID
	char *_buf = (char*)buf;
	size_t got = 0;
	for (int i = 0; i < sizeof(ChunkHeader); i++)
		got += CFileMgr::Read(file, &_buf[i], 1);
	(*offset) += got;
	return got;
#else
	// original code loops 8 times to read 1 byte with CFileMgr::Read, that's retarded
	// D18: devuelve los bytes leidos para que quien llama pueda distinguir una
	// cabecera real de un fin de fichero (antes se ignoraba el resultado y se
	// releia indefinidamente la cabecera anterior).
	size_t got = CFileMgr::Read(file, (char*)buf, sizeof(ChunkHeader));
	*offset += sizeof(ChunkHeader);
	return got;
#endif
}

void
CText::LoadMissionText(char *MissionTableName)
{
	CMessages::ClearAllMessagesDisplayedByGame();

	mission_keyArray.Unload();
	mission_data.Unload();

	bool search_result = false;
	int missionTableId = 0;

	for (missionTableId = 0; missionTableId < MissionTextOffsets.size; missionTableId++) {
		if (strncmp(MissionTextOffsets.data[missionTableId].szMissionName, MissionTableName, strlen(MissionTextOffsets.data[missionTableId].szMissionName)) == 0) {
			search_result = true;
			break;
		}
	}

	if (!search_result) {
		printf("CText::LoadMissionText - couldn't find %s", MissionTableName);
		return;
	}

	CFileMgr::SetDir("TEXT");
	const char *filename = OdTextFileName();
	CTimer::Suspend();
	int file = CFileMgr::OpenFile(filename, "rb");
	// D18 (sección 1, 21/09): ESTE es el cuelgue de la pantalla negra.
	//
	// El SCM del mod llama a este opcode (COMMAND_LOAD_MISSION_TEXT, 1356) en
	// cuanto arranca la partida. El original abre el GXT y lanza el bucle de
	// chunks sin comprobar NADA: si el fichero no esta (o no esta todavia) el
	// descriptor es 0, CFileMgr::Read no lee nada, la cabecera se queda con
	// basura/size 0 y el `while` gira indefinidamente -> el motor no vuelve a
	// dibujar y la pestaña se queda en negro sin un solo mensaje de error.
	if (file == 0) {
		OdTextFail("CText::LoadMissionText", filename);
		CTimer::Resume();
		CFileMgr::SetDir("");
		return;
	}
	CFileMgr::Seek(file, MissionTextOffsets.data[missionTableId].offset, SEEK_SET);

	char TableCheck[8];
	CFileMgr::Read(file, TableCheck, 8);
	if (strncmp(TableCheck, MissionTableName, 8) != 0)
		printf("CText::LoadMissionText - expected to find %s in the text file", MissionTableName);

	bool tkey_loaded = false, tdat_loaded = false;
	size_t tkey_got = 0, tkey_size = 0, tdat_got = 0, tdat_size = 0;
	ChunkHeader m_ChunkHeader;
	while (!tkey_loaded || !tdat_loaded) {
		size_t bytes_read = 0;
		size_t got = ReadChunkHeader(&m_ChunkHeader, file, &bytes_read);
		// D18: se acabaron los datos o la cabecera no es legible -> salir.
		// El guion sigue con los textos que haya: mejor sin texto que colgado.
		if (got != sizeof(ChunkHeader) || m_ChunkHeader.size == 0)
			break;
		if (strncmp(m_ChunkHeader.magic, "TKEY", 4) == 0) {
			// D19: exigir la lectura COMPLETA del chunk.
			tkey_got = mission_keyArray.Load(m_ChunkHeader.size, file, &bytes_read);
			tkey_size = (size_t)m_ChunkHeader.size;
			tkey_loaded = tkey_got == tkey_size;
		} else if (strncmp(m_ChunkHeader.magic, "TDAT", 4) == 0) {
			tdat_got = mission_data.Load(m_ChunkHeader.size, file, &bytes_read);
			tdat_size = (size_t)m_ChunkHeader.size;
			tdat_loaded = tdat_got == tdat_size;
		} else
			CFileMgr::Seek(file, m_ChunkHeader.size, SEEK_CUR);
	}

	if (tkey_loaded && tdat_loaded) {
		mission_keyArray.Update(mission_data.chars);
		strcpy(szMissionTableName, MissionTableName);
		bIsMissionTextLoaded = true;
#ifdef __EMSCRIPTEN__
		// D19 (seccion 1, 21/09): una linea por tabla de mision cargada. Si el
		// array se quedara corto (lectura corta del fichero), la ultima clave
		// lo delata aqui: es el sintoma que se vio con INTRO4 (la ultima de la
		// tabla) sin que el fichero le faltara nada.
		{
			char t[192];
			snprintf(t, sizeof t, "MSGTABLE %s n=%d primera=%s ultima=%s tkey=%u/%u tdat=%u/%u", MissionTableName,
				mission_keyArray.numEntries,
				mission_keyArray.numEntries > 0 ? mission_keyArray.entries[0].key : "-",
				mission_keyArray.numEntries > 0 ? mission_keyArray.entries[mission_keyArray.numEntries - 1].key : "-",
				(unsigned)tkey_got, (unsigned)tkey_size, (unsigned)tdat_got, (unsigned)tdat_size);
			ODTRACES(t);
		}
#endif
	} else {
#ifdef __EMSCRIPTEN__
		// D19: si la causa es una lectura corta, se ve aqui (bytes leidos / bytes
		// que declara la cabecera del chunk).
		char t[192];
		snprintf(t, sizeof t, "TXTGXTSHORT %s tkey=%u/%u tdat=%u/%u", filename,
			(unsigned)tkey_got, (unsigned)tkey_size, (unsigned)tdat_got, (unsigned)tdat_size);
		ODTRACES(t);
#endif
		OdTextFail("CText::LoadMissionText", filename);
	}

	CFileMgr::CloseFile(file);
	CTimer::Resume();
	CFileMgr::SetDir("");
}


// D19 (seccion 1, 21/09): Load devuelve los bytes leidos. Si son menos que
// `length`, el array se queda con memoria SIN INICIALIZAR en la cola y el
// motor busca claves en basura: sintoma tipico "la ULTIMA clave de la tabla
// missing" (INTRO4) de forma intermitente, sin que al fichero le falte nada.
// Con el valor devuelto, quien llama puede rechazar la tabla a medias.
size_t
CKeyArray::Load(size_t length, int file, size_t* offset)
{
	char *rawbytes;

	// You can make numEntries size_t if you want to exceed 32-bit boundaries, everything else should be ready.
	numEntries = (int)(length / sizeof(CKeyEntry));
	entries = new CKeyEntry[numEntries];
	rawbytes = (char*)entries;

#ifdef THIS_IS_STUPID
	size_t got = 0;
	for (uint32 i = 0; i < length; i++) {
		got += CFileMgr::Read(file, &rawbytes[i], 1);
		(*offset)++;
	}
	return got;
#else
	size_t got = CFileMgr::Read(file, rawbytes, length);
	*offset += length;
	return got;
#endif
}

void
CKeyArray::Unload(void)
{
	delete[] entries;
	entries = nil;
	numEntries = 0;
}

void
CKeyArray::Update(wchar *chars)
{
#if !defined(FIX_BUGS) && !defined(FIX_BUGS_64)
	int i;
	for(i = 0; i < numEntries; i++)
		entries[i].value = (wchar*)((uint8*)chars + (uintptr)entries[i].value);
#endif
}

CKeyEntry*
CKeyArray::BinarySearch(const char *key, CKeyEntry *entries, int16 low, int16 high)
{
	int mid;
	int diff;

	if(low > high)
		return nil;

	mid = (low + high)/2;
	diff = strcmp(key, entries[mid].key);
	if(diff == 0)
		return &entries[mid];
	if(diff < 0)
		return BinarySearch(key, entries, low, mid-1);
	if(diff > 0)
		return BinarySearch(key, entries, mid+1, high);
	return nil;
}

wchar*
#if defined (FIX_BUGS) || defined(FIX_BUGS_64)
CKeyArray::Search(const char *key, wchar *data, uint8 *result)
#else
CKeyArray::Search(const char *key, uint8 *result)
#endif
{
	CKeyEntry *found;
	char errstr[25];
	int i;

#if defined (FIX_BUGS) || defined(FIX_BUGS_64)
	found = BinarySearch(key, entries, 0, numEntries-1);
	if (found) {
		*result = true;
		return (wchar*)((uint8*)data + found->valueOffset);
	}
#else
	found = BinarySearch(key, entries, 0, numEntries-1);
	if (found) {
		*result = true;
		return found->value;
	}
#endif
	*result = false;
#ifdef MASTER
	sprintf(errstr, "");
#else
	// D2: una clave que no existe se ve en pantalla como "<clave> missing". La
	// traza del log vive ahora en CText::Get, DESPUES de mirar tambien la tabla
	// de mision (aqui todavia puede resolverla la otra tabla).
	sprintf(errstr, "%s missing", key);
#endif // MASTER
	for(i = 0; i < 25; i++)
		WideErrorString[i] = errstr[i];
	return WideErrorString;
}

// D19: igual que CKeyArray::Load, devuelve los bytes leidos (ver el comentario
// de alli: un TDAT a medias deja textos basura o, peor, punteros sin inicializar).
size_t
CData::Load(size_t length, int file, size_t * offset)
{
	char *rawbytes;

	// You can make numChars size_t if you want to exceed 32-bit boundaries, everything else should be ready.
	numChars = (int)(length / sizeof(wchar));
	chars = new wchar[numChars];
	rawbytes = (char*)chars;

#ifdef THIS_IS_STUPID
	size_t got = 0;
	for(uint32 i = 0; i < length; i++){
		got += CFileMgr::Read(file, &rawbytes[i], 1);
		(*offset)++;
	}
	return got;
#else
	size_t got = CFileMgr::Read(file, rawbytes, length);
	*offset += length;
	return got;
#endif
}

void
CData::Unload(void)
{
	delete[] chars;
	chars = nil;
	numChars = 0;
}

void
CMissionTextOffsets::Load(size_t table_size, int file, size_t *offset, int)
{
#ifdef THIS_IS_STUPID
	size_t num_of_entries = table_size / sizeof(CMissionTextOffsets::Entry);
	for (size_t mi = 0; mi < num_of_entries; mi++) {
		for (uint32 i = 0; i < sizeof(data[mi].szMissionName); i++) {
			CFileMgr::Read(file, &data[i].szMissionName[i], 1);
			(*offset)++;
		}
		char* _buf = (char*)&data[mi].offset;
		for (uint32 i = 0; i < sizeof(data[mi].offset); i++) {
			CFileMgr::Read(file, &_buf[i], 1);
			(*offset)++;
		}
	}
	size = (uint16)num_of_entries;
#else
	// not exact VC code but smaller and better :P

	// You can make this size_t if you want to exceed 32-bit boundaries, everything else should be ready.
	size = (uint16) (table_size / sizeof(CMissionTextOffsets::Entry));
	CFileMgr::Read(file, (char*)data, sizeof(CMissionTextOffsets::Entry) * size);
	*offset += sizeof(CMissionTextOffsets::Entry) * size;
#endif
}

char*
UnicodeToAscii(wchar *src)
{
	static char aStr[256];
	int len;
	for(len = 0; *src != '\0' && len < 256-1; len++, src++)
#ifdef MORE_LANGUAGES
		if(*src < 128 || ((CGame::russianGame || CGame::japaneseGame) && *src < 256))
#else
		if(*src < 128)
#endif
			aStr[len] = *src;
		// convert to CP1252
		else if(*src <= 131)
			aStr[len] = *src + 64;
		else if (*src <= 141)
			aStr[len] = *src + 66;
		else if (*src <= 145)
			aStr[len] = *src + 68;
		else if (*src <= 149)
			aStr[len] = *src + 71;
		else if (*src <= 154)
			aStr[len] = *src + 73;
		else if (*src <= 164)
			aStr[len] = *src + 75;
		else if (*src <= 168)
			aStr[len] = *src + 77;
		else if (*src <= 204)
			aStr[len] = *src + 80;
		else switch (*src) {
		case 205: aStr[len] = 209; break;
		case 206: aStr[len] = 241; break;
		case 207: aStr[len] = 191; break;
		default: aStr[len] = '#'; break;
		}
	aStr[len] = '\0';
	return aStr;
}

char*
UnicodeToAsciiForSaveLoad(wchar *src)
{
	static char aStr[256];
	int len;
	for(len = 0; *src != '\0' && len < 256; len++, src++)
		if(*src < 256)
			aStr[len] = *src;
		else
			aStr[len] = '#';
	aStr[len] = '\0';
	return aStr;
}

char*
UnicodeToAsciiForMemoryCard(wchar *src)
{
	static char aStr[256];
	int len;
	for(len = 0; *src != '\0' && len < 256; len++, src++)
		if(*src < 256)
			aStr[len] = *src;
		else
			aStr[len] = '#';
	aStr[len] = '\0';
	return aStr;
}

void
TextCopy(wchar *dst, const wchar *src)
{
	while((*dst++ = *src++) != '\0');
}
