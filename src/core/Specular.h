#pragma once


extern bool g_bViceExtSpecular;
void ViceExtInitSpecularPower(void);
float ViceExtSpecularPower(void);
float ViceExtSpecularFresnel(void);
void ViceExtSpecularColor(float &r, float &g, float &b);
