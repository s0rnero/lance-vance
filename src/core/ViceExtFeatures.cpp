#include "common.h"
#include "ViceExtFeatures.h"
#include "FileMgr.h"
#include "Timer.h"
#include "ondemand.h"

#ifdef VICEEXT_FEATURES_INI

namespace ViceExtFeatures
{

struct KeyDef { const char *name; bool impl; };

static const KeyDef kKeys[] = {
	{ "VehiclesDontCatchFireWhenTurningOver",              true  }, // VICEEXT_NO_FLIP_BURN
	{ "HealthRegenerationUpToHalf",                        true  }, // VICEEXT_HEALTH_REGEN
	{ "WantedStarsHideOnScreenWhenThereIsNoSearch",        true  }, // fila 9.07
	{ "RemoveMoneyZerosInTheHud",                          true  }, // VICEEXT_MONEY_NO_ZEROS
	{ "PlayerDoesntBounceAwayFromMovingCar",               true  },
	{ "StandardCarsUseTurnSignals",                        true  }, // VICEEXT_STD_TURN_SIGNALS
	{ "CameraShakeInVehicleAtHighSpeed",                   true  }, // 12.03: Cam.cpp Process_FollowCar_SA
	{ "MilitaryFiringFromTankAtPlayer",                    true  }, // VICEEXT_TANK_FIRE
	{ "DisableBulletTraces",                               true  }, // VICEEXT_FIX_SILENTPATCH (traza de bala)
	{ "WaypointColorRGB",                                  true  }, // VICEEXT_GPS_COLOR
	{ "RecoilWhenFiring",                                  true  },
	{ "EnableClimbing",                                    true  },
	{ "EnableSwimming",                                    true  }, // VICEEXT_SWIMMING
	{ "RocketLauncherThirdPersonAiming",                   true  },
	{ "RandomVehicleModsInTraffic",                        false }, // fuera: taller de tuning
	{ "MirrorModeByDefault",                               false }, // sin implementar
};

static const int kNumKeys = ARRAY_SIZE(kKeys);

struct Override { const char *name; int value; const char *why; };

static const Override kOverrides[] = {
	{ "RemoveMoneyZerosInTheHud", 1, "cambio de HUD pedido (9.2, VICEEXT_MONEY_NO_ZEROS)" },
	{ "VehiclesDontCatchFireWhenTurningOver", 1, "9.2: el jugador pidio la opcion" },
	{ "HealthRegenerationUpToHalf", 0, "2026-10-18: la vida no se regenera nunca (peticion del jugador)" },
	{ "MilitaryFiringFromTankAtPlayer", 1, "9.2: el jugador pidio la opcion" },
	{ "StandardCarsUseTurnSignals", 1, "el port los trae encendidos (VICEEXT_TURN_SIGNALS)" },
};

static const int kNumOverrides = ARRAY_SIZE(kOverrides);

static int s_ini[kNumKeys];			// lo que trae el fichero (-1 = no esta)
static int s_rgb[3];				// WaypointColorRGB
static bool s_rgbSet;
static bool s_loaded;
static bool s_read;

static char
LowerChar(char ch)
{
	return (ch >= 'A' && ch <= 'Z') ? (char)(ch - 'A' + 'a') : ch;
}

static bool
SameKey(const char *a, const char *b)
{
	while (*a && *b) {
		if (LowerChar(*a) != LowerChar(*b))
			return false;
		a++; b++;
	}
	return *a == 0 && *b == 0;
}

static int
FindKey(const char *name)
{
	for (int i = 0; i < kNumKeys; i++)
		if (SameKey(kKeys[i].name, name))
			return i;
	return -1;
}

static const Override *
FindOverride(const char *name)
{
	for (int i = 0; i < kNumOverrides; i++)
		if (SameKey(kOverrides[i].name, name))
			return &kOverrides[i];
	return nil;
}

static const char *
TrimLeft(const char *p)
{
	while (*p == ' ' || *p == '\t' || *p == '"')
		p++;
	return p;
}

static void
TrimValue(char *v)
{
	char *p = v;
	while (*p && *p != '\n' && *p != '\r' && *p != ';' && *p != '#')
		p++;
	*p = 0;
	while (p > v) {
		p--;
		if (*p == ' ' || *p == '\t' || *p == '"')
			*p = 0;
		else
			break;
	}
}

static void
Parse(char *text)
{
	bool inFeatures = false;
	char *line = text;

	while (*line) {
		char *nl = line;
		while (*nl && *nl != '\n')
			nl++;
		char saved = *nl;
		*nl = 0;

		char *p = (char *)TrimLeft(line);
		if (*p == '[') {
			char *close = p;
			while (*close && *close != ']')
				close++;
			*close = 0;
			inFeatures = SameKey(p + 1, "Features");
		} else if (*p && *p != ';' && *p != '#' && inFeatures) {
			char *eq = p;
			while (*eq && *eq != '=')
				eq++;
			if (*eq == '=') {
				*eq = 0;
				char *keyEnd = eq;
				while (keyEnd > p && (keyEnd[-1] == ' ' || keyEnd[-1] == '\t'))
					keyEnd--;
				*keyEnd = 0;

				char *val = (char *)TrimLeft(eq + 1);
				TrimValue(val);

				int idx = FindKey((const char *)p);
				if (idx >= 0) {
					if (SameKey(p, "WaypointColorRGB")) {
						int r, g, b;
						if (sscanf(val, "%d , %d , %d", &r, &g, &b) == 3) {
							s_rgb[0] = r; s_rgb[1] = g; s_rgb[2] = b;
							s_rgbSet = true;
							s_ini[idx] = 1;
						}
					} else {
						s_ini[idx] = (int)strtol(val, nil, 0);
					}
				}
			}
		}

		if (!saved)
			break;
		line = nl + 1;
	}
}

void
Load(void)
{
	if (s_loaded)
		return;
	s_loaded = true;

	for (int i = 0; i < kNumKeys; i++)
		s_ini[i] = -1;
	s_rgb[0] = 180; s_rgb[1] = 24; s_rgb[2] = 24;	// el default de su lector
	s_rgbSet = false;

	uint8 buf[4096];
	ssize_t got = -1;
	{
		CFileMgr::SetDir("");
		got = CFileMgr::LoadFile("ViceExtended/features.ini", buf, sizeof(buf) - 1, "r");
		CFileMgr::SetDir("");
	}
	if (got > 0) {
		buf[got < (ssize_t)sizeof(buf) ? got : (ssize_t)sizeof(buf) - 1] = 0;
		Parse((char *)buf);
		s_read = true;
	} else {
		s_read = false;
	}

#ifdef __EMSCRIPTEN__
	{
		int leidas = 0;
		for (int i = 0; i < kNumKeys; i++)
			if (s_ini[i] >= 0)
				leidas++;
		for (int i = 0; i < kNumKeys; i++) {
			const Override *ov = FindOverride(kKeys[i].name);
			const char *origen = ov ? "jugador" : (s_ini[i] >= 0 ? "fichero" : "port");
			int aplica = ov ? ov->value : (s_ini[i] >= 0 ? (s_ini[i] != 0) : -1);
			char t[240];				snprintf(t, sizeof t,
					"P4 kind=inifile schema=1 gen=1 frame=%u sim=%.4f case=0 id=0 clave=%s ini=%d aplica=%d origen=%s impl=%d",
				(unsigned)CTimer::GetFrameCounter(), CTimer::GetTimeInMilliseconds() * 0.001f,
				kKeys[i].name, s_ini[i], aplica, origen, kKeys[i].impl ? 1 : 0);
			ODTRACES(t);
		}
		{
			char t[200];
			snprintf(t, sizeof t,
				"P4 kind=inifile schema=1 gen=1 frame=%u sim=%.4f case=1 id=0 fichero=%d leidas=%d rgb=%d,%d,%d",
				(unsigned)CTimer::GetFrameCounter(), CTimer::GetTimeInMilliseconds() * 0.001f,
				s_read ? 1 : 0, leidas, s_rgb[0], s_rgb[1], s_rgb[2]);
			ODTRACES(t);
		}
	}
#endif
}

bool
On(const char *key, bool portDefault)
{
	if (!s_loaded)
		Load();

	const Override *ov = FindOverride(key);
	if (ov)
		return ov->value != 0;

	int idx = FindKey(key);
	if (idx >= 0 && s_ini[idx] >= 0)
		return s_ini[idx] != 0;

	return portDefault;
}

int
Waypoint(int channel, int portDefault)
{
	if (!s_loaded)
		Load();

	if (channel < 0 || channel > 2)
		return portDefault;

	return s_rgbSet ? s_rgb[channel] : portDefault;
}

bool
WasRead(void)
{
	return s_read;
}

}

#endif // VICEEXT_FEATURES_INI
