SkyGFX 3.0 for III and Vice City by aap/The Hero
================================================

NOTE: This version will no longer work with 1.1 and steam exes

NB: skygfx.dll and skygfx.asi are the exact same file. I only renamed the dll to asi for convenience because some people
are really mad (like really, really mad) that I no longer used .asi as the extension for my mods. So because I'm always
so nice to everyone I've included the file twice now, but YOU DON'T need both.

- Place rwd3d9.dll into the game's main directory.
- Have the .dll/.asi loaded by the Simple DLL loader [1] or Ultimate ASI loader [2].
  SkyGFX no longer can be loaded with the built-in mss32 ASI loader.
  Put III, VC}/skygfx.ini into the same directory as the dll.
- Place {III, VC}/neo/ into the game's main directory or the directory the dll was loaded from

See the .ini comments for features and how to configure the mod.

A d3d8 to d3d9 wrapper is needed for all features to work properly. I have
included d3d8to9 for convenience. Place the d3d8.dll into your game's main
directory.

This mod uses the debugmenu when available.



For dll hackers
===============
SkyGFX exports the following functions so you can attach the neo pipelines
from your own code, just pass an RpAtomic or an RpAtomic.

void AttachCarPipeToRwObject(RwObject *obj);
void AttachRimPipeToRwObject(RwObject *obj);
void AttachWorldPipeToRwObject(RwObject *obj);

It also exports the following function to access its settings:
Check skygfx.h for what is exported (INTPARAMS and FLOATPARAMS):
int SkyGFXGetVersion(const char *s);
int SkyGFXGetParam(const char *s);
void SkyGFXSetParam(const char *s, int val);
float SkyGFXGetParamF(const char *s);
void SkyGFXSetParamF(const char *s, float val);
void *SkyGFXGetParamPtr(const char *s);



Credits: support for different exe versions was once done by Sergeanur (no support in this version)
         ThirteenAG for neo-style blood screen droplets

[1] http://gtaforums.com/topic/894847-simple-dll-loader/
[2] https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases