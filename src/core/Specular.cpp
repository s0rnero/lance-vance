#include "common.h"

#include "Specular.h"
#include "custompipes.h"
#include "FileMgr.h"
#include "Weather.h"
#include "Timecycle.h"

using namespace CustomPipes;

bool g_bViceExtSpecular = true;
static InterpolatedFloat *odSpecPower = nil;
static InterpolatedFloat *odSpecFresnel = nil;
static InterpolatedColor *odSpecColor = nil;

void
ViceExtInitSpecularPower(void)
{
	if(odSpecPower)
		return;
	odSpecPower = new InterpolatedFloat(25.0f);
	odSpecFresnel = new InterpolatedFloat(1.0f);
	odSpecColor = new InterpolatedColor(Color(1.0f, 1.0f, 1.0f, 1.0f));

	char work_buff[16384];
	if(CFileMgr::LoadFile("neo/carTweakingTable.dat", (uint8*)work_buff, sizeof(work_buff), "r") <= 0){
		return;
	}
	char *fp = (char*)work_buff;
	InterpolatedFloat odFresnel(1.0f);
	InterpolatedColor odDiffColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
	fp = ReadTweakValueTable(fp, odFresnel);
	fp = ReadTweakValueTable(fp, *odSpecPower);
	fp = ReadTweakValueTable(fp, odDiffColor);
	fp = ReadTweakValueTable(fp, *odSpecColor);
	*odSpecFresnel = odFresnel;
}

float
ViceExtSpecularPower(void)
{
	if(!odSpecPower)
		return 25.0f;
	float p = odSpecPower->Get();
	if(p < 0.0f)
		return 0.0f;
	return p;
}

float
ViceExtSpecularFresnel(void)
{
	if(!odSpecFresnel)
		return 1.0f;
	return odSpecFresnel->Get();
}

void
ViceExtSpecularColor(float &r, float &g, float &b)
{
	r = g = b = 1.0f;
	if(!odSpecColor)
		return;
	Color c = odSpecColor->Get();
	r = c.r;
	g = c.g;
	b = c.b;
}
