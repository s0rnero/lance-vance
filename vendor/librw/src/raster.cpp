#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef __EMSCRIPTEN__
#include <stdarg.h>
#include <emscripten.h>
// B.1: envío async por lotes (cola window.__odq, flush en main.js). Nada de
// XHR síncrono en el hilo del juego.
static void odqSend(const char *prefix, const char *msg)
{
	EM_ASM({
		try { (window.__odq = window.__odq || []).push(UTF8ToString($0) + UTF8ToString($1)); } catch (e) {}
	}, prefix, msg);
}
// Traza F3a: consola (acotada, clase error) + fichero odtrace.log vía el
// endpoint Vite (la consola no debe saturarse). Yo leo el fichero.
static void convTrace(const char *fmt, ...)
{
	static int n = 0;
	if (n >= 30) return;
	n++;
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	printf("[texconv] %s\n", buf);
	odqSend("[texconv] ", buf);
}

// Traza propia de la conversion de texturas D3D8->GL3 (alfa del musgo): ras=%p
// enlaza con TEXA (nombre y flags del fichero) y el texid enlaza con ALPHADRAW
// (estado de render al dibujar). Cupo aparte de convTrace (que es para F3a).
static void
dxtConvTrace(const char *fmt, ...)
{
	static int n = 0;
	if (n >= 6) return; // F4a-higiene: cap bajo
	n++;
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	odqSend("CONVDXT ", buf);
}

// Alfa final de una textura convertida (bug "musgo negro"): si lvl0A=1 y
// finalA=0, el cutout acaba dibujandose OPACO. Cupo propio (no compite con
// los IMG de la conversion).
static void
alfaTrace(const char *fmt, ...)
{
	static int n = 0;
	if (n >= 6) return; // F4a-higiene: cap bajo
	n++;
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	odqSend("CONVDXT ", buf);
}

// ALFAMIP: un mip SIN alfa dentro de un cutout (la firma exacta del bug).
// Cupo propio y pequeno: si vuelve a verse algo negro, su linea TIENE que
// estar en el log (no la puede tumbar el ruido de las demas texturas).
static void
mipTrace(const char *fmt, ...)
{
	static int n = 0;
	if (n >= 4) return; // F4a-higiene: cap bajo
	n++;
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	odqSend("CONVDXT ", buf);
}
#endif

#include "rwbase.h"
#include "rwerror.h"
#include "rwplg.h"
#include "rwpipeline.h"
#include "rwobjects.h"
#include "rwengine.h"
//#include "ps2/rwps2.h"
#include "d3d/rwd3d.h"
#include "d3d/rwxbox.h"
//#include "d3d/rwd3d8.h"
//#include "d3d/rwd3d9.h"
#include "gl/rwgl3.h"

#define PLUGIN_ID 0

namespace rw {

int32 Raster::numAllocated;

struct RasterGlobals
{
	int32 sp;
	Raster *stack[32];
};
int32 rasterModuleOffset;

#define RASTERGLOBAL(v) (PLUGINOFFSET(RasterGlobals, engine, rasterModuleOffset)->v)

static void*
rasterOpen(void *object, int32 offset, int32 size)
{
	int i;
	rasterModuleOffset = offset;
	RASTERGLOBAL(sp) = -1;
	for(i = 0; i < (int)nelem(RASTERGLOBAL(stack)); i++)
		RASTERGLOBAL(stack)[i] = nil;
	return object;
}

static void*
rasterClose(void *object, int32 offset, int32 size)
{
	return object;
}

void
Raster::registerModule(void)
{
	Engine::registerPlugin(sizeof(RasterGlobals), ID_RASTERMODULE, rasterOpen, rasterClose);
}

Raster*
Raster::create(int32 width, int32 height, int32 depth, int32 format, int32 platform)
{
	// TODO: pass arguments through to the driver and create the raster there
	Raster *raster = (Raster*)rwMalloc(s_plglist.size, MEMDUR_EVENT);	// TODO
	assert(raster != nil);
	numAllocated++;
	raster->parent = raster;
	raster->offsetX = 0;
	raster->offsetY = 0;
	raster->platform = platform ? platform : rw::platform;
	raster->type = format & 0x7;
	raster->flags = format & 0xF8;
	raster->privateFlags = 0;
	raster->format = format & 0xFF00;
	raster->width = width;
	raster->height = height;
	raster->depth = depth;
	raster->stride = 0;
	raster->pixels = raster->palette = nil;
	s_plglist.construct(raster);

//	printf("%d %d %d %d\n", raster->type, raster->width, raster->height, raster->depth);
	return engine->driver[raster->platform]->rasterCreate(raster);
}

void
Raster::subRaster(Raster *parent, Rect *r)
{
	if((this->flags & DONTALLOCATE) == 0)
		return;
	this->width = r->w;
	this->height = r->h;
	this->offsetX = parent->offsetX + r->x;
	this->offsetY = parent->offsetY + r->y;
	this->parent = parent->parent;
}

void
Raster::destroy(void)
{
	s_plglist.destruct(this);
	rwFree(this);
	numAllocated--;
}

uint8*
Raster::lock(int32 level, int32 lockMode)
{
	return engine->driver[this->platform]->rasterLock(this, level, lockMode);
}

void
Raster::unlock(int32 level)
{
	engine->driver[this->platform]->rasterUnlock(this, level);
}

uint8*
Raster::lockPalette(int32 lockMode)
{
	return engine->driver[this->platform]->rasterLockPalette(this, lockMode);
}

void
Raster::unlockPalette(void)
{
	engine->driver[this->platform]->rasterUnlockPalette(this);
}

int32
Raster::getNumLevels(void)
{
	return engine->driver[this->platform]->rasterNumLevels(this);
}

int32
Raster::calculateNumLevels(int32 width, int32 height)
{
	int32 size = width >= height ? width : height;
	int32 n;
	for(n = 0; size != 0; n++)
		size /= 2;
	return n;
}

bool
Raster::formatHasAlpha(int32 format)
{
	return (format & 0xF00) == Raster::C8888 ||
	       (format & 0xF00) == Raster::C1555 ||
	       (format & 0xF00) == Raster::C4444;
}

bool32
Raster::imageFindRasterFormat(Image *image, int32 type,
	int32 *pWidth, int32 *pHeight, int32 *pDepth, int32 *pFormat,
	int32 platform)
{
	return engine->driver[platform ? platform : rw::platform]->imageFindRasterFormat(
		image, type, pWidth, pHeight, pDepth, pFormat);
}

Raster*
Raster::setFromImage(Image *image, int32 platform)
{
	if(engine->driver[platform ? platform : rw::platform]->rasterFromImage(this, image))
		return this;
	return nil;
}

Raster*
Raster::createFromImage(Image *image, int32 platform)
{
	Raster *raster;
	int32 width, height, depth, format;
	if(!imageFindRasterFormat(image, TEXTURE, &width, &height, &depth, &format, platform))
		return nil;
	raster = Raster::create(width, height, depth, format, platform);
	if(raster == nil)
		return nil;
	return raster->setFromImage(image, platform);
}

Image*
Raster::toImage(void)
{
	return engine->driver[this->platform]->rasterToImage(this);
}

void
Raster::show(uint32 flags)
{
	engine->device.showRaster(this, flags);
}

Raster*
Raster::pushContext(Raster *raster)
{
	RasterGlobals *g = PLUGINOFFSET(RasterGlobals, engine, rasterModuleOffset);
	if(g->sp >= (int32)nelem(g->stack)-1)
		return nil;
	return g->stack[++g->sp] = raster;
}

Raster*
Raster::popContext(void)
{
	RasterGlobals *g = PLUGINOFFSET(RasterGlobals, engine, rasterModuleOffset);
	if(g->sp < 0)
		return nil;
	return g->stack[g->sp--];
}

Raster*
Raster::getCurrentContext(void)
{
	RasterGlobals *g = PLUGINOFFSET(RasterGlobals, engine, rasterModuleOffset);
	if(g->sp < 0 || g->sp >= (int32)nelem(g->stack))
		return nil;
	return g->stack[g->sp];
}

bool32
Raster::renderFast(int32 x, int32 y)
{
	return engine->device.rasterRenderFast(this,x, y);
}

void
conv_RGBA8888_from_RGBA8888(uint8 *out, uint8 *in)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
	out[3] = in[3];
}

void
conv_BGRA8888_from_RGBA8888(uint8 *out, uint8 *in)
{
	out[2] = in[0];
	out[1] = in[1];
	out[0] = in[2];
	out[3] = in[3];
}

void
conv_RGBA8888_from_RGB888(uint8 *out, uint8 *in)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
	out[3] = 0xFF;
}

void
conv_BGRA8888_from_RGB888(uint8 *out, uint8 *in)
{
	out[2] = in[0];
	out[1] = in[1];
	out[0] = in[2];
	out[3] = 0xFF;
}

void
conv_RGB888_from_RGB888(uint8 *out, uint8 *in)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void
conv_BGR888_from_RGB888(uint8 *out, uint8 *in)
{
	out[2] = in[0];
	out[1] = in[1];
	out[0] = in[2];
}

void
conv_ARGB1555_from_ARGB1555(uint8 *out, uint8 *in)
{
	out[0] = in[0];
	out[1] = in[1];
}

void
conv_ARGB1555_from_RGB555(uint8 *out, uint8 *in)
{
	out[0] = in[0];
	out[1] = in[1] | 0x80;
}

void
conv_RGBA5551_from_ARGB1555(uint8 *out, uint8 *in)
{
	uint32 r, g, b, a;
	a = (in[1]>>7) & 1;
	r = (in[1]>>2) & 0x1F;
	g = (in[1]&3)<<3 | ((in[0]>>5)&7);
	b = in[0] & 0x1F;
	out[0] = a | b<<1 | g<<6;
	out[1] = g>>2 | r<<3;
}

void
conv_ARGB1555_from_RGBA5551(uint8 *out, uint8 *in)
{
	uint32 r, g, b, a;
	a = in[0] & 1;
	b = (in[0]>>1) & 0x1F;
	g = (in[1]&7)<<2 | ((in[0]>>6)&3);
	r = (in[1]>>3) & 0x1F;
	out[0] = b | g<<5;
	out[1] = g>>3 | r<<2 | a<<7;
}

void
conv_RGBA8888_from_ARGB1555(uint8 *out, uint8 *in)
{
	uint32 r, g, b, a;
	a = (in[1]>>7) & 1;
	r = (in[1]>>2) & 0x1F;
	g = (in[1]&3)<<3 | ((in[0]>>5)&7);
	b = in[0] & 0x1F;
	out[0] = r*0xFF/0x1f;
	out[1] = g*0xFF/0x1f;
	out[2] = b*0xFF/0x1f;
	out[3] = a*0xFF;
}

void
conv_ABGR1555_from_ARGB1555(uint8 *out, uint8 *in)
{
	uint32 r, b;
	r = (in[1]>>2) & 0x1F;
	b = in[0] & 0x1F;
	out[1] = (in[1]&0x83) | b<<2;
	out[0] = (in[0]&0xE0) | r;
}

void
expandPal4(uint8 *dst, uint32 dststride, uint8 *src, uint32 srcstride, int32 w, int32 h)
{
	int32 x, y;
	for(y = 0; y < h; y++)
		for(x = 0; x < w/2; x++){
			dst[y*dststride + x*2 + 0] = src[y*srcstride + x] & 0xF;
			dst[y*dststride + x*2 + 1] = src[y*srcstride + x] >> 4;
		}
}
void
compressPal4(uint8 *dst, uint32 dststride, uint8 *src, uint32 srcstride, int32 w, int32 h)
{
	int32 x, y;
	for(y = 0; y < h; y++)
		for(x = 0; x < w/2; x++)
			dst[y*dststride + x] = src[y*srcstride + x*2 + 0] | src[y*srcstride + x*2 + 1] << 4;
}

void
expandPal4_BE(uint8 *dst, uint32 dststride, uint8 *src, uint32 srcstride, int32 w, int32 h)
{
	int32 x, y;
	for(y = 0; y < h; y++)
		for(x = 0; x < w/2; x++){
			dst[y*dststride + x*2 + 1] = src[y*srcstride + x] & 0xF;
			dst[y*dststride + x*2 + 0] = src[y*srcstride + x] >> 4;
		}
}
void
compressPal4_BE(uint8 *dst, uint32 dststride, uint8 *src, uint32 srcstride, int32 w, int32 h)
{
	int32 x, y;
	for(y = 0; y < h; y++)
		for(x = 0; x < w/2; x++)
			dst[y*dststride + x] = src[y*srcstride + x*2 + 1] | src[y*srcstride + x*2 + 0] << 4;
}

void
copyPal8(uint8 *dst, uint32 dststride, uint8 *src, uint32 srcstride, int32 w, int32 h)
{
	int32 x, y;
	for(y = 0; y < h; y++)
		for(x = 0; x < w; x++)
			dst[y*dststride + x] = src[y*srcstride + x];
}



// Platform conversion

static rw::Raster*
xbox_to_d3d(rw::Raster *ras)
{
	using namespace rw;

	int dxt = 0;
	xbox::XboxRaster *xboxras = GETXBOXRASTEREXT(ras);
	if(xboxras->customFormat){
		switch(xboxras->format){
		case xbox::D3DFMT_DXT1: dxt = 1; break;
		case xbox::D3DFMT_DXT3: dxt = 3; break;
		case xbox::D3DFMT_DXT5: dxt = 5; break;
		}
	}
	if(dxt == 0)
		return nil;

	Raster *newras = Raster::create(ras->width, ras->height, ras->depth,
		                        ras->format | Raster::TEXTURE | Raster::DONTALLOCATE);
	int numLevels = ras->getNumLevels();
	d3d::allocateDXT(newras, dxt, numLevels, xboxras->hasAlpha);
	for(int i = 0; i < numLevels; i++){
		uint8 *srcpx = ras->lock(i, Raster::LOCKREAD);
	//	uint8 *dstpx = newras->lock(i, Raster::LOCKWRITE | Raster::LOCKNOFETCH);
		d3d::setTexels(newras, srcpx, i);
	//	flipDXT(dxt, dstpx, srcpx, ras->width, ras->height);
		ras->unlock(i);
	//	newras->unlock(i);
	}

	return newras;
}

static rw::Raster*
d3d_to_gl3(rw::Raster *ras)
{
#ifdef RW_GL3
	using namespace rw;

	if(!gl3::gl3Caps.dxtSupported)
		return nil;

	int dxt = 0;
	d3d::D3dRaster *d3dras = GETD3DRASTEREXT(ras);
	if(d3dras->customFormat){
		switch(d3dras->format){
		case d3d::D3DFMT_DXT1: dxt = 1; break;
		// DXT2 = DXT3 y DXT4 = DXT5 con alfa premultiplicado: mismo layout de
		// bloque, así que se reetiquetan. Sin esto dxt=0 -> nil y, en un build
		// con S3TC, la textura se quedaba sin subir a GPU.
		case d3d::D3DFMT_DXT2:
		case d3d::D3DFMT_DXT3: dxt = 3; break;
		case d3d::D3DFMT_DXT4:
		case d3d::D3DFMT_DXT5: dxt = 5; break;
		}
	}
	if(dxt == 0)
		return nil;

	Raster *newras = Raster::create(ras->width, ras->height, ras->depth,
		                        ras->format | Raster::TEXTURE | Raster::DONTALLOCATE);
	int numLevels = ras->getNumLevels();
	gl3::allocateDXT(newras, dxt, numLevels, d3dras->hasAlpha);
#ifdef __EMSCRIPTEN__
	dxtConvTrace("ras=%p dxt=%d %dx%d lv=%d inA=%d outA=%d texid=%u",
	             ras, dxt, ras->width, ras->height, numLevels,
	             (int)d3dras->hasAlpha,
	             (int)PLUGINOFFSET(gl3::Gl3Raster, newras, gl3::nativeRasterOffset)->hasAlpha,
	             PLUGINOFFSET(gl3::Gl3Raster, newras, gl3::nativeRasterOffset)->texid);
#endif
	for(int i = 0; i < numLevels; i++){
		uint8 *srcpx = ras->lock(i, Raster::LOCKREAD);
		uint8 *dstpx = newras->lock(i, Raster::LOCKWRITE | Raster::LOCKNOFETCH);
		if(srcpx == nil || dstpx == nil){
			if(srcpx)
				ras->unlock(i);
			if(dstpx)
				newras->unlock(i);
			newras->destroy();
			return nil;
		}
		flipDXT(dxt, dstpx, srcpx, ras->width, ras->height);
		ras->unlock(i);
		newras->unlock(i);
	}

	return newras;
#else
	return nil;
#endif
}

static rw::Raster*
xbox_to_gl3(rw::Raster *ras)
{
#ifdef RW_GL3
	using namespace rw;

	int dxt = 0;
	xbox::XboxRaster *xboxras = GETXBOXRASTEREXT(ras);
	if(xboxras->customFormat){
		switch(xboxras->format){
		case xbox::D3DFMT_DXT1: dxt = 1; break;
		case xbox::D3DFMT_DXT3: dxt = 3; break;
		case xbox::D3DFMT_DXT5: dxt = 5; break;
		}
	}
	if(dxt == 0)
		return nil;

	Raster *newras = Raster::create(ras->width, ras->height, ras->depth,
		                        ras->format | Raster::TEXTURE | Raster::DONTALLOCATE);
	int numLevels = ras->getNumLevels();
	gl3::allocateDXT(newras, dxt, numLevels, xboxras->hasAlpha);
	for(int i = 0; i < numLevels; i++){
		uint8 *srcpx = ras->lock(i, Raster::LOCKREAD);
		uint8 *dstpx = newras->lock(i, Raster::LOCKWRITE | Raster::LOCKNOFETCH);
		flipDXT(dxt, dstpx, srcpx, ras->width, ras->height);
		ras->unlock(i);
		newras->unlock(i);
	}

	return newras;
#else
	return nil;
#endif
}

rw::Raster*
Raster::convertTexToCurrentPlatform(rw::Raster *ras)
{
	using namespace rw;

	if(ras->platform == rw::platform)
		return ras;
	// compatible platforms
	if((ras->platform == PLATFORM_D3D8 && rw::platform == PLATFORM_D3D9) ||
	   (ras->platform == PLATFORM_D3D9 && rw::platform == PLATFORM_D3D8))
		return ras;

	if(ras->width <= 0 || ras->height <= 0 || ras->width > 4096 || ras->height > 4096){
#ifdef __EMSCRIPTEN__
		convTrace("RASGARBAGE %dx%d fmt=0x%x levels=%d", ras->width, ras->height, ras->format, ras->getNumLevels());
#endif
		return nil;
	}

	// special cased conversion for DXT
	if((ras->platform == PLATFORM_D3D8 || ras->platform == PLATFORM_D3D9) && rw::platform == PLATFORM_GL3){
		Raster *newras = d3d_to_gl3(ras);
		if(newras){
			ras->destroy();
			return newras;
		}
	}else if(ras->platform == PLATFORM_XBOX && (rw::platform == PLATFORM_D3D9 || rw::platform == PLATFORM_D3D8)){
		Raster *newras = xbox_to_d3d(ras);
		if(newras){
			ras->destroy();
			return newras;
		}
	}else if(ras->platform == PLATFORM_XBOX && rw::platform == PLATFORM_GL3){
		Raster *newras = xbox_to_gl3(ras);
		if(newras){
			ras->destroy();
			return newras;
		}
	}

	// fall back to going through Image directly
	int32 width, height, depth, format;
#ifdef __EMSCRIPTEN__
	// TEMP-DIAG F3: traza del punto exacto del nil (ver convTrace: consola
	// acotada + odtrace.log). Quitar cuando el "Failed to load TXD" esté resuelto.
	int32 inFmt = ras->format, inW = ras->width, inH = ras->height;
	int inLevels = ras->getNumLevels();
#else
	(void)0;
#endif
	Image *img = ras->toImage();
	if(img == nil){
#ifdef __EMSCRIPTEN__
		convTrace("toImage NIL fmt=0x%x %dx%d levels=%d", inFmt, inW, inH, inLevels);
#endif
		return nil;
	}
	// TODO: maybe don't *always* do this?
	img->unpalettize();
	if(!Raster::imageFindRasterFormat(img, Raster::TEXTURE, &width, &height, &depth, &format)){
#ifdef __EMSCRIPTEN__
		convTrace("imageFindRasterFormat FAIL fmt=0x%x %dx%d imgdepth=%d", inFmt, inW, inH, img->depth);
#endif
		img->destroy();
		return nil;
	}
	format |= ras->format & (Raster::MIPMAP | Raster::AUTOMIPMAP);
#ifdef __EMSCRIPTEN__
	// Camino "por Image" (sin S3TC en el navegador, o formato no DXT): dice con
	// qué formato y con alfa o sin alfa acaba la textura. Es el otro punto donde
	// un cutout (musgo) puede perder la transparencia.
	int odLvl0A = (int)img->hasAlpha();
	dxtConvTrace("IMG ras=%p in=0x%x %dx%d imgDepth=%d imgA=%d out=0x%x depth=%d",
	             ras, inFmt, inW, inH, img->depth, odLvl0A, format, depth);
#endif
	Raster *newras = Raster::create(width, height, depth, format);
	if(newras == nil){
#ifdef __EMSCRIPTEN__
		convTrace("Raster::create NIL fmt=0x%x %dx%dx%d", format, width, height, depth);
#endif
		img->destroy();
		return nil;
	}
#ifdef __EMSCRIPTEN__
	// F3b: verificar niveles realmente subidos. Si la cadena queda incompleta
	// (WebGL muestrea negro al minificar), fijar MAX_LEVEL + numLevels a lo
	// bueno; con 1 solo nivel el filtro cae a NoMIP solo (gl3device).
	int goodLevels = 0;
	if(!newras->setFromImage(img))
		convTrace("setFromImage(0) FAIL fmt=0x%x %dx%d", inFmt, inW, inH);
	else
		goodLevels = 1;
#else
	newras->setFromImage(img);
#endif
	img->destroy();
	int numLevels = ras->getNumLevels();
	for(int i = 1; i < numLevels; i++){
		ras->lock(i, Raster::LOCKREAD);
		img = ras->toImage();
		if(img == nil){
#ifdef __EMSCRIPTEN__
			convTrace("toImage NIL nivel %d fmt=0x%x %dx%d", i, inFmt, inW, inH);
#endif
			ras->unlock(i);
			continue;
		}
		// TODO: maybe don't *always* do this?
		img->unpalettize();
		newras->lock(i, Raster::LOCKWRITE|Raster::LOCKNOFETCH);
#ifdef __EMSCRIPTEN__
		{
			int lvlA = (int)img->hasAlpha();
			if(!newras->setFromImage(img))
				convTrace("setFromImage(%d) FAIL fmt=0x%x %dx%d", i, inFmt, inW, inH);
			else if(i == goodLevels)
				goodLevels++;
#ifdef __EMSCRIPTEN__
			// Un mip sin alfa de un cutout es el que antes APAGABA el flag de
			// toda la textura (musgo negro). Con el arreglo de gl3raster.cpp el
			// flag se mantiene: esta linea lo deja por escrito sin recompilar.
			if(odLvl0A && !lvlA){
				using namespace gl3;
				int flag = (int)PLUGINOFFSET(gl3::Gl3Raster, newras, gl3::nativeRasterOffset)->hasAlpha;
				mipTrace("ALFAMIP ras=%p texid=%u lvl=%d/%d imgA=0 flag=%d fmt=0x%x %dx%d",
				          ras, PLUGINOFFSET(gl3::Gl3Raster, newras, gl3::nativeRasterOffset)->texid,
				          i, numLevels, flag, inFmt, inW, inH);
			}
#endif
		}
#else
		newras->setFromImage(img);
#endif
		newras->unlock(i);
		img->destroy();
		ras->unlock(i);
	}
#ifdef __EMSCRIPTEN__
	// Alfa final (musgo negro): dice si el cutout acaba con alpha-test/blend
	// encendidos (finalA=1) o apagados (finalA=0, el cutout se dibuja OPACO y
	// su zona transparente negra se ve como mancha negra). `ras` enlaza con
	// TEXA (nombre y flags del fichero del nivel 0).
	{
		using namespace gl3;
		int finalA = (int)PLUGINOFFSET(gl3::Gl3Raster, newras, gl3::nativeRasterOffset)->hasAlpha;
		if(odLvl0A || finalA != odLvl0A)
			alfaTrace("ALFA ras=%p texid=%u lvl0A=%d finalA=%d up=%d/%d fmt=0x%x %dx%d",
			          ras, PLUGINOFFSET(gl3::Gl3Raster, newras, gl3::nativeRasterOffset)->texid,
			          odLvl0A, finalA, goodLevels, newras->getNumLevels(),
			          inFmt, inW, inH);
	}
	if(goodLevels < 1){
		convTrace("SIN nivel 0 fmt=0x%x %dx%d: descarto textura", inFmt, inW, inH);
		newras->destroy();
		return nil;
	}
	if(goodLevels < newras->getNumLevels()){
		convTrace("clamp %d->%d niveles fmt=0x%x %dx%d", newras->getNumLevels(), goodLevels, inFmt, inW, inH);
		{
			using namespace gl3;
			Gl3Raster *newnatras = GETGL3RASTEREXT(newras);
			uint32 prev = bindTexture(newnatras->texid);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, goodLevels-1);
			newnatras->numLevels = goodLevels;
			bindTexture(prev);
		}
	}
#endif
	ras->destroy();
	ras = newras;
	return ras;
}


}
