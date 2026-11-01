#pragma once


namespace ViceExtFeatures
{
	void Load(void);

	bool On(const char *key, bool portDefault);

	int Waypoint(int channel, int portDefault);

	bool WasRead(void);
}
