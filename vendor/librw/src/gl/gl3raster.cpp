#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../rwbase.h"
#include "../rwerror.h"
#include "../rwplg.h"
#include "../rwpipeline.h"
#include "../rwobjects.h"
#include "../rwengine.h"

#include "rwgl3.h"
#include "rwgl3shader.h"
#include "rwgl3impl.h"
#ifdef __EMSCRIPTEN__
#include <stdarg.h>
#include <emscripten.h>
#endif

#define PLUGIN_ID ID_DRIVER

// Texturas GL vivas (creadas menos destruidas). El heartbeat web la publica en
// WEBHB: si un modelo descargado no libera sus texturas al desalojarse, este
// numero solo sube y el renderer acaba muriendo por memoria de GPU, que es la
// unica muerte que no deja marca en la traza (ni JSERR ni ENGERR ni WEBGL_LOST).
int gOdTexLive = 0;
long long gOdTexBytes = 0;
int gOdTexGenTotal = 0;
int gOdTexDelTotal = 0;

#ifdef __EMSCRIPTEN__
static void
odTxBadTrace(const char *fmt, ...)
{
	static int n = 0;
	if(n >= 60)
		return;
	n++;
	char buf[224];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	EM_ASM({
		try { (window.__odq = window.__odq || []).push(UTF8ToString($0)); } catch (e) {}
	}, buf);
}
#endif

static inline void
odTexGen(GLuint *id)
{
	glGenTextures(1, id);
	::gOdTexLive++;
	::gOdTexGenTotal++;
}

static inline void
odTexDel(GLuint *id)
{
	if(*id == 0)
		return;
	glDeleteTextures(1, id);
	::gOdTexLive--;
	::gOdTexDelTotal++;
}

namespace rw {
namespace gl3 {

int32 nativeRasterOffset;

static uint32
getLevelSize(Raster *raster, int32 level)
{
	int i;
	Gl3Raster *natras = GETGL3RASTEREXT(raster);

	int w = raster->originalWidth;
	int h = raster->originalHeight;
	int s = raster->originalStride;
	int minDim = 1;

#ifdef RW_OPENGL
	switch(natras->internalFormat){
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		minDim = 4;
		break;
	}
#endif

	for(i = 0; i < level; i++){
		if(w > minDim){
			w /= 2;
			s /= 2;
		}
		if(h > minDim)
			h /= 2;
	}

	return s*h;
}

static long long
odTexRasterBytes(Raster *raster)
{
	Gl3Raster *natras = GETGL3RASTEREXT(raster);
	int32 levels = natras->autogenMipmap ? 1 : natras->numLevels;
	if(levels < 1)
		levels = 1;
	long long tot = 0;
	for(int32 l = 0; l < levels; l++)
		tot += getLevelSize(raster, l);
	if(!natras->isCompressed && raster->type == Raster::TEXTURE && !natras->hasAlpha &&
	   natras->format == GL_RGBA && natras->type == GL_UNSIGNED_BYTE)
		tot /= 2;
	if(natras->autogenMipmap)
		tot += tot / 3;
	return tot;
}

static void
odTexAccount(Raster *raster)
{
	Gl3Raster *natras = GETGL3RASTEREXT(raster);
	long long bytes = odTexRasterBytes(raster);
	if(bytes != natras->odBytes){
		::gOdTexBytes += bytes - natras->odBytes;
		natras->odBytes = bytes;
	}
}

static void
odTexUnaccount(Raster *raster)
{
	Gl3Raster *natras = GETGL3RASTEREXT(raster);
	if(natras->odBytes != 0){
		::gOdTexBytes -= natras->odBytes;
		natras->odBytes = 0;
	}
}

#ifdef RW_OPENGL

static Raster*
rasterCreateTexture(Raster *raster)
{
	if(raster->format & (Raster::PAL4 | Raster::PAL8)){
		RWERROR((ERR_NOTEXTURE));
		return nil;
	}

	Gl3Raster *natras = GETGL3RASTEREXT(raster);
	switch(raster->format & 0xF00){
	case Raster::C8888:
		natras->internalFormat = GL_RGBA8;
		natras->format = GL_RGBA;
		natras->type = GL_UNSIGNED_BYTE;
		natras->hasAlpha = 1;
		natras->bpp = 4;
		raster->depth = 32;
		break;
	case Raster::C888:
		natras->internalFormat = GL_RGB8;
		natras->format = GL_RGB;
		natras->type = GL_UNSIGNED_BYTE;
		natras->hasAlpha = 0;
		natras->bpp = 3;
		raster->depth = 24;
		break;
	case Raster::C1555:
		natras->internalFormat = GL_RGB5_A1;
		natras->format = GL_RGBA;
		natras->type = GL_UNSIGNED_SHORT_5_5_5_1;
		natras->hasAlpha = 1;
		natras->bpp = 2;
		raster->depth = 16;
		break;
	default:
		RWERROR((ERR_INVRASTER));
		return nil;
	}

	if(gl3Caps.gles){
		// glReadPixels only supports GL_RGBA
		natras->internalFormat = GL_RGBA8;
		natras->format = GL_RGBA;
		natras->type = GL_UNSIGNED_BYTE;
		natras->bpp = 4;
	}

	raster->stride = raster->width*natras->bpp;

	if(raster->format & Raster::MIPMAP){
		int w = raster->width;
		int h = raster->height;
		natras->numLevels = 1;
		while(w != 1 || h != 1){
			natras->numLevels++;
			if(w > 1) w /= 2;
			if(h > 1) h /= 2;
		}
	}
	natras->autogenMipmap = (raster->format & (Raster::MIPMAP|Raster::AUTOMIPMAP)) == (Raster::MIPMAP|Raster::AUTOMIPMAP);
	if(natras->autogenMipmap)
		natras->numLevels = 1;

	odTexGen(&natras->texid);
	uint32 prev = bindTexture(natras->texid);
	glTexImage2D(GL_TEXTURE_2D, 0, natras->internalFormat,
	             raster->width, raster->height,
	             0, natras->format, natras->type, nil);
	// TODO: allocate other levels...probably
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, natras->numLevels-1);
	natras->filterMode = 0;
	natras->addressU = 0;
	natras->addressV = 0;
	natras->maxAnisotropy = 1;

	bindTexture(prev);
	return raster;
}

static Raster*
rasterCreateCameraTexture(Raster *raster)
{
	if(raster->format & (Raster::PAL4 | Raster::PAL8)){
		RWERROR((ERR_NOTEXTURE));
		return nil;
	}

	// TODO: figure out what the backbuffer is and use that as a default
	Gl3Raster *natras = GETGL3RASTEREXT(raster);
	switch(raster->format & 0xF00){
	case Raster::C8888:
		natras->internalFormat = GL_RGBA8;
		natras->format = GL_RGBA;
		natras->type = GL_UNSIGNED_BYTE;
		natras->hasAlpha = 1;
		natras->bpp = 4;
		break;
	case Raster::C888:
	default:
		natras->internalFormat = GL_RGB8;
		natras->format = GL_RGB;
		natras->type = GL_UNSIGNED_BYTE;
		natras->hasAlpha = 0;
		natras->bpp = 3;
		break;
	case Raster::C1555:
		natras->internalFormat = GL_RGB5_A1;
		natras->format = GL_RGBA;
		natras->type = GL_UNSIGNED_SHORT_5_5_5_1;
		natras->hasAlpha = 1;
		natras->bpp = 2;
		break;
	}

	// i don't remember why this was once here...
	if(gl3Caps.gles){
		// glReadPixels only supports GL_RGBA
//		natras->internalFormat = GL_RGBA8;
//		natras->format = GL_RGBA;
//		natras->type = GL_UNSIGNED_BYTE;
//		natras->bpp = 4;
	}

	raster->stride = raster->width*natras->bpp;

	natras->autogenMipmap = (raster->format & (Raster::MIPMAP|Raster::AUTOMIPMAP)) == (Raster::MIPMAP|Raster::AUTOMIPMAP);

	odTexGen(&natras->texid);
	uint32 prev = bindTexture(natras->texid);
	glTexImage2D(GL_TEXTURE_2D, 0, natras->internalFormat,
	             raster->width, raster->height,
	             0, natras->format, natras->type, nil);
	natras->filterMode = 0;
	natras->addressU = 0;
	natras->addressV = 0;
	natras->maxAnisotropy = 1;

	bindTexture(prev);


	glGenFramebuffers(1, &natras->fbo);
	bindFramebuffer(natras->fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, natras->texid, 0);
	bindFramebuffer(0);
	natras->fboMate = nil;

	return raster;
}

static Raster*
rasterCreateCamera(Raster *raster)
{
	Gl3Raster *natras = GETGL3RASTEREXT(raster);

	// TODO: set/check width, height, depth, format?

	// used for locking right now
	raster->format = Raster::C888;
	natras->internalFormat = GL_RGB8;
	natras->format = GL_RGB;
	natras->type = GL_UNSIGNED_BYTE;
	natras->hasAlpha = 0;
	natras->bpp = 3;

	natras->autogenMipmap = 0;

	natras->texid = 0;
	natras->fbo = 0;
	natras->fboMate = nil;

	return raster;
}

static Raster*
rasterCreateZbuffer(Raster *raster)
{
	Gl3Raster *natras = GETGL3RASTEREXT(raster);

	if(gl3Caps.gles){
		// have to use RBO on GLES!!
		glGenRenderbuffers(1, &natras->texid);
		glBindRenderbuffer(GL_RENDERBUFFER, natras->texid);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, raster->width, raster->height);
	}else{
		// TODO: set/check width, height, depth, format?
		natras->internalFormat = GL_DEPTH_STENCIL;
		natras->format = GL_DEPTH_STENCIL;
		natras->type = GL_UNSIGNED_INT_24_8;

		natras->autogenMipmap = 0;

		odTexGen(&natras->texid);
		uint32 prev = bindTexture(natras->texid);
		glTexImage2D(GL_TEXTURE_2D, 0, natras->internalFormat,
			     raster->width, raster->height,
			     0, natras->format, natras->type, nil);
		natras->filterMode = 0;
		natras->addressU = 0;
		natras->addressV = 0;
		natras->maxAnisotropy = 1;

		bindTexture(prev);
	}
	natras->fbo = 0;
	natras->fboMate = nil;

	return raster;
}


#endif


void
allocateDXT(Raster *raster, int32 dxt, int32 numLevels, bool32 hasAlpha)
{
#ifdef RW_OPENGL
	assert(raster->type == Raster::TEXTURE);

	Gl3Raster *natras = GETGL3RASTEREXT(raster);
	switch(dxt){
	case 1:
		if(hasAlpha){
			natras->internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			natras->format = GL_RGBA;
		}else{
			natras->internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			natras->format = GL_RGB;
		}
		// bogus, but stride*height should be the size of the image
		// 4x4 in 8 bytes
		raster->stride = raster->width/2;
		break;
	case 3:
		natras->internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		natras->format = GL_RGBA;
		// 4x4 in 16 bytes
		raster->stride = raster->width;
		break;
	case 5:
		natras->internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		natras->format = GL_RGBA;
		// 4x4 in 16 bytes
		raster->stride = raster->width;
		break;
	default:
		assert(0 && "invalid DXT format");
	}
	natras->type = GL_UNSIGNED_BYTE;
	natras->hasAlpha = hasAlpha;
	natras->bpp = 2;
	raster->depth = 16;

	natras->isCompressed = 1;
	if(raster->format & Raster::MIPMAP)
		natras->numLevels = numLevels;
	natras->autogenMipmap = (raster->format & (Raster::MIPMAP|Raster::AUTOMIPMAP)) == (Raster::MIPMAP|Raster::AUTOMIPMAP);
	if(natras->autogenMipmap)
		natras->numLevels = 1;

	odTexGen(&natras->texid);
	uint32 prev = bindTexture(natras->texid);
	glTexImage2D(GL_TEXTURE_2D, 0, natras->internalFormat,
	             raster->width, raster->height,
	             0, natras->format, natras->type, nil);
	// TODO: allocate other levels...probably
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, natras->numLevels-1);
	natras->filterMode = 0;
	natras->addressU = 0;
	natras->addressV = 0;
	natras->maxAnisotropy = 1;

	bindTexture(prev);

	raster->originalStride = raster->stride;

	if(gl3Caps.gles && needToReadBackTextures){
		// Uh oh, need to keep a copy in cpu memory
		int32 i;
		int32 size = 0;
		for(i = 0; i < natras->numLevels; i++)
			size += getLevelSize(raster, i);
		uint8 *data = (uint8*)rwNew(sizeof(RasterLevels)+sizeof(RasterLevels::Level)*(natras->numLevels-1)+size,
			MEMDUR_EVENT | ID_DRIVER);
		RasterLevels *levels = (RasterLevels*)data;
		data += sizeof(RasterLevels)+sizeof(RasterLevels::Level)*(natras->numLevels-1);
		levels->numlevels = natras->numLevels;
		levels->format = 0;	// not needed
		for(i = 0; i < natras->numLevels; i++){
			levels->levels[i].data = data;
			levels->levels[i].size = getLevelSize(raster, i);
			levels->levels[i].width = 0;	// we don't need those here, maybe later...
			levels->levels[i].height = 0;
			data += levels->levels[i].size;
		}
		natras->backingStore = levels;
	}

	raster->flags &= ~Raster::DONTALLOCATE;
#endif
}

/*
{ 0, 0, 0 },
{ 16, 4, GL_RGBA },	// 1555
{ 16, 3, GL_RGB },	// 565
{ 16, 4, GL_RGBA },	// 4444
{ 0, 0, 0 },	// LUM8
{ 32, 4, GL_RGBA },	// 8888
{ 24, 3, GL_RGB },	// 888
{ 16, 3, GL_RGB },	// D16
{ 24, 3, GL_RGB },	// D24
{ 32, 4, GL_RGBA },	// D32
{ 16, 3, GL_RGB },	// 555

0,
GL_RGB5_A1,
GL_RGB5,
GL_RGBA4,
0,
GL_RGBA8,
GL_RGB8,
GL_RGB5,
GL_RGB8,
GL_RGBA8,
GL_RGB5
*/

Raster*
rasterCreate(Raster *raster)
{
	Gl3Raster *natras = GETGL3RASTEREXT(raster);

	natras->isCompressed = 0;
	natras->hasAlpha = 0;
	natras->numLevels = 1;

	Raster *ret = raster;

	if(raster->width == 0 || raster->height == 0){
		raster->flags |= Raster::DONTALLOCATE;
		raster->stride = 0;
		goto ret;
	}
	if(raster->flags & Raster::DONTALLOCATE)
		goto ret;

	switch(raster->type){
#ifdef RW_OPENGL
	case Raster::NORMAL:
	case Raster::TEXTURE:
		ret = rasterCreateTexture(raster);
		break;
	case Raster::CAMERATEXTURE:
		ret = rasterCreateCameraTexture(raster);
		break;
	case Raster::ZBUFFER:
		ret = rasterCreateZbuffer(raster);
		break;
	case Raster::CAMERA:
		ret = rasterCreateCamera(raster);
		break;
#endif

	default:
		RWERROR((ERR_INVRASTER));
		return nil;
	}

ret:
	raster->originalWidth = raster->width;
	raster->originalHeight = raster->height;
	raster->originalStride = raster->stride;
	raster->originalPixels = raster->pixels;
	return ret;
}

uint8*
rasterLock(Raster *raster, int32 level, int32 lockMode)
{
#ifdef RW_OPENGL
	Gl3Raster *natras GETGL3RASTEREXT(raster);
	uint8 *px;
	uint32 allocSz;
	int i;

	assert(raster->privateFlags == 0);

	switch(raster->type){
	case Raster::NORMAL:
	case Raster::TEXTURE:
	case Raster::CAMERATEXTURE:
#ifdef __EMSCRIPTEN__
		if(raster->originalWidth <= 0 || raster->originalHeight <= 0 ||
		   raster->originalWidth > 4096 || raster->originalHeight > 4096 ||
		   raster->originalStride <= 0 || level < 0 || level >= 32){
			odTxBadTrace("TXDBAD where=lock raster=%p w=%d h=%d stride=%d level=%d fmt=0x%x",
				(void*)raster, raster->originalWidth, raster->originalHeight,
				raster->originalStride, level, raster->format);
			return nil;
		}
#endif
		for(i = 0; i < level; i++){
			if(raster->width > 1){
				raster->width /= 2;
				raster->stride /= 2;
			}
			if(raster->height > 1)
				raster->height /= 2;
		}

		allocSz = getLevelSize(raster, level);
#ifdef __EMSCRIPTEN__
		if(allocSz == 0 || allocSz > 64*1024*1024 || raster->width <= 0 ||
		   raster->height <= 0 || raster->stride <= 0){
			odTxBadTrace("TXDBAD where=lock raster=%p alloc=%u w=%d h=%d stride=%d level=%d fmt=0x%x",
				(void*)raster, allocSz, raster->width, raster->height,
				raster->stride, level, raster->format);
			return nil;
		}
#endif
		px = (uint8*)rwMalloc(allocSz, MEMDUR_EVENT | ID_DRIVER);
		assert(raster->pixels == nil);
		raster->pixels = px;

		if(lockMode & Raster::LOCKREAD || !(lockMode & Raster::LOCKNOFETCH)){
			if(natras->isCompressed){
				if(natras->backingStore){
					assert(level < natras->backingStore->numlevels);
					assert(allocSz >= natras->backingStore->levels[level].size);
					memcpy(px, natras->backingStore->levels[level].data, allocSz);
				}else{
					// GLES is losing here
					uint32 prev = bindTexture(natras->texid);
					glGetCompressedTexImage(GL_TEXTURE_2D, level, px);
					bindTexture(prev);
				}
			}else if(gl3Caps.gles){
				GLuint fbo;
				glGenFramebuffers(1, &fbo);
				bindFramebuffer(fbo);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, natras->texid, 0);
				GLenum e = glCheckFramebufferStatus(GL_FRAMEBUFFER);
assert(natras->format == GL_RGBA);
				glReadPixels(0, 0, raster->width, raster->height, natras->format, natras->type, px);
//e = glGetError(); printf("GL err4 %x (%x)\n", e, natras->format);
				bindFramebuffer(0);
				glDeleteFramebuffers(1, &fbo);
			}else{
				uint32 prev = bindTexture(natras->texid);
				glPixelStorei(GL_PACK_ALIGNMENT, 1);
				glGetTexImage(GL_TEXTURE_2D, level, natras->format, natras->type, px);
				bindTexture(prev);
			}
		}

		raster->privateFlags = lockMode;
		break;

	case Raster::CAMERA:
		if(lockMode & Raster::PRIVATELOCK_WRITE)
			assert(0 && "can't lock framebuffer for writing");
		raster->width = glGlobals.presentWidth;
		raster->height = glGlobals.presentHeight;
		raster->stride = raster->width*natras->bpp;
		assert(natras->bpp == 3);
		allocSz = raster->height*raster->stride;
		px = (uint8*)rwMalloc(allocSz, MEMDUR_EVENT | ID_DRIVER);
		assert(raster->pixels == nil);
		raster->pixels = px;
		glReadBuffer(GL_BACK);
		glReadPixels(0, 0, raster->width, raster->height, GL_RGB, GL_UNSIGNED_BYTE, px);

		raster->privateFlags = lockMode;
		break;

	default:
		assert(0 && "cannot lock this type of raster yet");
		return nil;
	}

	return px;
#else
	return nil;
#endif
}

void
rasterUnlock(Raster *raster, int32 level)
{
#ifdef RW_OPENGL
	Gl3Raster *natras = GETGL3RASTEREXT(raster);

	assert(raster->pixels);

	switch(raster->type){
	case Raster::NORMAL:
	case Raster::TEXTURE:
	case Raster::CAMERATEXTURE:
		if(raster->privateFlags & Raster::LOCKWRITE){
			uint32 prev = bindTexture(natras->texid);
			if(level == 0)
				odTexAccount(raster);
			if(natras->isCompressed){
				glCompressedTexImage2D(GL_TEXTURE_2D, level, natras->internalFormat,
					raster->width, raster->height, 0,
					getLevelSize(raster, level),
					raster->pixels);
				if(natras->backingStore){
					assert(level < natras->backingStore->numlevels);
					memcpy(natras->backingStore->levels[level].data, raster->pixels,
						natras->backingStore->levels[level].size);
				}
			}else{
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
#ifdef __EMSCRIPTEN__
				// Web: texturas opacas a RGB565 (mitad de VRAM que RGBA8).
				// Solo TEXTURE con hasAlpha==0 en este nivel: los cutouts
				// tienen alfa en el nivel 0 (latch del musgo) y quedan RGBA.
				// Conversión transitoria por nivel (se libera acto seguido);
				// los campos quedan RGBA8 (readback y recompresión intactos).
				// Alfa ausente muestrea 1.0: el blend de una opaca no cambia.
				if(raster->type == Raster::TEXTURE && !natras->hasAlpha &&
				   natras->format == GL_RGBA && natras->type == GL_UNSIGNED_BYTE){
					int npx = raster->width * raster->height;
					uint16 *tmp = (uint16*)rwMalloc(npx*sizeof(uint16), MEMDUR_EVENT | ID_DRIVER);
					if(tmp){
						uint8 *src = raster->pixels;
						for(int i = 0; i < npx; i++){
							tmp[i] = (uint16)(((src[0] >> 3) << 11) | ((src[1] >> 2) << 5) | (src[2] >> 3));
							src += 4;
						}
						glTexImage2D(GL_TEXTURE_2D, level, GL_RGB565,
						             raster->width, raster->height,
						             0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, tmp);
						rwFree(tmp);
					}else{
						glTexImage2D(GL_TEXTURE_2D, level, natras->internalFormat,
						             raster->width, raster->height,
						             0, natras->format, natras->type, raster->pixels);
					}
				}else
#endif
				glTexImage2D(GL_TEXTURE_2D, level, natras->internalFormat,
					     raster->width, raster->height,
					     0, natras->format, natras->type, raster->pixels);
			}
			if(level == 0 && natras->autogenMipmap)
				glGenerateMipmap(GL_TEXTURE_2D);
			bindTexture(prev);
		}
		break;

	case Raster::CAMERA:
		// TODO: write?
		break;
	}

	rwFree(raster->pixels);
	raster->pixels = nil;
#endif
	raster->width = raster->originalWidth;
	raster->height = raster->originalHeight;
	raster->stride = raster->originalStride;
	raster->pixels = raster->originalPixels;
	raster->privateFlags = 0;
}

int32
rasterNumLevels(Raster *raster)
{
	return GETGL3RASTEREXT(raster)->numLevels;
}

// Almost the same as d3d9 and ps2 function
bool32
imageFindRasterFormat(Image *img, int32 type,
	int32 *pWidth, int32 *pHeight, int32 *pDepth, int32 *pFormat)
{
	int32 width, height, depth, format;

	assert((type&0xF) == Raster::TEXTURE);

//	for(width = 1; width < img->width; width <<= 1);
//	for(height = 1; height < img->height; height <<= 1);
	// Perhaps non-power-of-2 textures are acceptable?
	width = img->width;
	height = img->height;

	depth = img->depth;

	if(depth <= 8)
		depth = 32;

	switch(depth){
	case 32:
		if(img->hasAlpha())
			format = Raster::C8888;
		else{
			format = Raster::C888;
			depth = 24;
		}
		break;
	case 24:
		format = Raster::C888;
		break;
	case 16:
		format = Raster::C1555;
		break;

	case 8:
	case 4:
	default:
		RWERROR((ERR_INVRASTER));
		return 0;
	}

	format |= type;

	*pWidth = width;
	*pHeight = height;
	*pDepth = depth;
	*pFormat = format;

	return 1;
}

bool32
rasterFromImage(Raster *raster, Image *image)
{
	if((raster->type&0xF) != Raster::TEXTURE)
		return 0;

	void (*conv)(uint8 *out, uint8 *in) = nil;

	// Unpalettize image if necessary but don't change original
	Image *truecolimg = nil;
	if(image->depth <= 8){
		truecolimg = Image::create(image->width, image->height, image->depth);
		truecolimg->pixels = image->pixels;
		truecolimg->stride = image->stride;
		truecolimg->palette = image->palette;
		truecolimg->unpalettize();
		image = truecolimg;
	}

	Gl3Raster *natras = GETGL3RASTEREXT(raster);
	int32 format = raster->format&0xF00;
	assert(!natras->isCompressed);
	switch(image->depth){
	case 32:
		if(gl3Caps.gles)
			conv = conv_RGBA8888_from_RGBA8888;
		else if(format == Raster::C8888)
			conv = conv_RGBA8888_from_RGBA8888;
		else if(format == Raster::C888)
			conv = conv_RGB888_from_RGB888;
		else
			goto err;
		break;
	case 24:
		if(gl3Caps.gles)
			conv = conv_RGBA8888_from_RGB888;
		else if(format == Raster::C8888)
			conv = conv_RGBA8888_from_RGB888;
		else if(format == Raster::C888)
			conv = conv_RGB888_from_RGB888;
		else
			goto err;
		break;
	case 16:
		if(gl3Caps.gles)
			conv = conv_RGBA8888_from_ARGB1555;
		else if(format == Raster::C1555)
			conv = conv_RGBA5551_from_ARGB1555;
		else
			goto err;
		break;

	case 8:
	case 4:
	default:
	err:
		RWERROR((ERR_INVRASTER));
		return 0;
	}

	// El alfa lo decide el nivel 0 (y el formato elegido al crear el raster).
	// El camino D3D8->GL3 sin S3TC sube los mips uno a uno con setFromImage,
	// asi que el ULTIMO nivel mandaba: los mips pequenos de un cutout (que ya
	// no tienen texeles transparentes) apagaban hasAlpha para TODA la textura
	// -> alpha-test/blend off -> el cutout se dibuja OPACO y el RGB negro de
	// su zona transparente se ve como manchas negras ("musgo negro" de la
	// playa: kbgrass_test2 256x256 lv=9, niveles 6-8 sin alfa). Un nivel con
	// alfa nunca debe APAGAR el flag de los demas.
	if(image->hasAlpha())
		natras->hasAlpha = 1;

	bool unlock = false;
	if(raster->pixels == nil){
		raster->lock(0, Raster::LOCKWRITE|Raster::LOCKNOFETCH);
		unlock = true;
	}

	uint8 *pixels = raster->pixels;
	assert(pixels);
	uint8 *imgpixels = image->pixels + (image->height-1)*image->stride;

	int x, y;
	assert(image->width == raster->width);
	assert(image->height == raster->height);
	for(y = 0; y < image->height; y++){
		uint8 *imgrow = imgpixels;
		uint8 *rasrow = pixels;
		for(x = 0; x < image->width; x++){
			conv(rasrow, imgrow);
			imgrow += image->bpp;
			rasrow += natras->bpp;
		}
		imgpixels -= image->stride;
		pixels += raster->stride;
	}
	if(unlock)
		raster->unlock(0);

	if(truecolimg)
		truecolimg->destroy();

	return 1;
}

Image*
rasterToImage(Raster *raster)
{
	int32 depth;
	Image *image;

	bool unlock = false;
	if(raster->pixels == nil){
		raster->lock(0, Raster::LOCKREAD);
		unlock = true;
	}

	Gl3Raster *natras = GETGL3RASTEREXT(raster);
	if(natras->isCompressed){
		// TODO
		RWERROR((ERR_INVRASTER));
		return nil;
	}

	void (*conv)(uint8 *out, uint8 *in) = nil;
	switch(raster->format & 0xF00){
	case Raster::C1555:
		depth = 16;
		conv = conv_ARGB1555_from_RGBA5551;
		break;
	case Raster::C8888:
		depth = 32;
		conv = conv_RGBA8888_from_RGBA8888;
		break;
	case Raster::C888:
		depth = 24;
		conv = conv_RGB888_from_RGB888;
		break;

	default:
	case Raster::C555:
	case Raster::C565:
	case Raster::C4444:
	case Raster::LUM8:
		RWERROR((ERR_INVRASTER));
		return nil;
	}

	if(raster->format & Raster::PAL4 ||
	   raster->format & Raster::PAL8){
		RWERROR((ERR_INVRASTER));
		return nil;
	}
		
	uint8 *in, *out;
	image = Image::create(raster->width, raster->height, depth);
	image->allocate();

	uint8 *imgpixels = image->pixels + (image->height-1)*image->stride;
	uint8 *pixels = raster->pixels;

	int x, y;
	assert(image->width == raster->width);
	assert(image->height == raster->height);
	for(y = 0; y < image->height; y++){
		uint8 *imgrow = imgpixels;
		uint8 *rasrow = pixels;
		for(x = 0; x < image->width; x++){
			conv(imgrow, rasrow);
			imgrow += image->bpp;
			rasrow += natras->bpp;
		}
		imgpixels -= image->stride;
		pixels += raster->stride;
	}

	if(unlock)
		raster->unlock(0);

	return image;
}

static void*
createNativeRaster(void *object, int32 offset, int32)
{
	Gl3Raster *ras = PLUGINOFFSET(Gl3Raster, object, offset);
	ras->texid = 0;
	ras->fbo = 0;
	ras->fboMate = nil;
	ras->backingStore = nil;
	ras->odBytes = 0;
	return object;
}

void evictRaster(Raster *raster);

static void*
destroyNativeRaster(void *object, int32 offset, int32)
{
	Raster *raster = (Raster*)object;
	Gl3Raster *natras = PLUGINOFFSET(Gl3Raster, object, offset);
#ifdef RW_OPENGL
	evictRaster(raster);
	odTexUnaccount(raster);
	switch(raster->type){
	case Raster::NORMAL:
	case Raster::TEXTURE:
		odTexDel(&natras->texid);
		break;

	case Raster::CAMERATEXTURE:
		if(natras->fboMate){
			// Break apart from currently associated zbuffer
			Gl3Raster *zras = GETGL3RASTEREXT(natras->fboMate);
			zras->fboMate = nil;
			natras->fboMate = nil;
		}
		glDeleteFramebuffers(1, &natras->fbo);
		odTexDel(&natras->texid);
		break;

	case Raster::ZBUFFER:
		if(natras->fboMate){
			// Detatch from FBO we may be attached to
			Gl3Raster *oldfb = GETGL3RASTEREXT(natras->fboMate);
			if(oldfb->fbo){
				bindFramebuffer(oldfb->fbo);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
			}
			oldfb->fboMate = nil;
		}
		if(gl3Caps.gles)
			glDeleteRenderbuffers(1, &natras->texid);
		else
			odTexDel(&natras->texid);
		break;

	case Raster::CAMERA:
		if(natras->fboMate){
			// Break apart from currently associated zbuffer
			Gl3Raster *zras = GETGL3RASTEREXT(natras->fboMate);
			zras->fboMate = nil;
			natras->fboMate = nil;
		}
		break;
	}
	natras->texid = 0;
	natras->fbo = 0;

	free(natras->backingStore);

#endif
	return object;
}

static void*
copyNativeRaster(void *dst, void *, int32 offset, int32)
{
	Gl3Raster *d = PLUGINOFFSET(Gl3Raster, dst, offset);
	d->texid = 0;
	d->fbo = 0;
	d->fboMate = nil;
	d->backingStore = nil;
	d->odBytes = 0;
	return dst;
}

static void
dxtUnpack565(uint16 c, uint8 *r, uint8 *g, uint8 *b)
{
	int tr = (c >> 11) & 0x1F;
	int tg = (c >> 5) & 0x3F;
	int tb = c & 0x1F;
	*r = (uint8)((tr << 3) | (tr >> 2));
	*g = (uint8)((tg << 2) | (tg >> 4));
	*b = (uint8)((tb << 3) | (tb >> 2));
}

// Decodifica un nivel DXT1/3/5 a RGBA8. dst pitch en bytes.
static void
dxtDecodeLevel(uint8 *dst, int32 dstPitch, int32 w, int32 h, uint8 *src, int32 dxt)
{
	int32 blocksX = (w + 3) / 4;
	int32 blocksY = (h + 3) / 4;
	uint8 *s = src;
	for(int32 by = 0; by < blocksY; by++){
		for(int32 bx = 0; bx < blocksX; bx++){
			uint8 alpha[16];
			uint8 cr[4], cg[4], cb[4];
			uint8 ca[4];
			for(int i = 0; i < 16; i++) alpha[i] = 255;
			for(int i = 0; i < 4; i++) ca[i] = 255;
			uint8 *colSrc = s;
			if(dxt == 3){
				// alpha 4 bits por texel, 8 bytes
				for(int i = 0; i < 16; i++){
					int v;
					if(i & 1) v = (s[i/2] >> 4) & 0xF;
					else v = s[i/2] & 0xF;
					alpha[i] = (uint8)((v << 4) | v);
				}
				colSrc = s + 8;
			}else if(dxt == 5){
				uint8 a0 = s[0], a1 = s[1];
				uint8 al[8];
				if(a0 > a1){
					al[0] = a0; al[1] = a1;
					al[2] = (uint8)((6*a0 + 1*a1) / 7);
					al[3] = (uint8)((5*a0 + 2*a1) / 7);
					al[4] = (uint8)((4*a0 + 3*a1) / 7);
					al[5] = (uint8)((3*a0 + 4*a1) / 7);
					al[6] = (uint8)((2*a0 + 5*a1) / 7);
					al[7] = (uint8)((1*a0 + 6*a1) / 7);
				}else{
					al[0] = a0; al[1] = a1;
					al[2] = (uint8)((4*a0 + 1*a1) / 5);
					al[3] = (uint8)((3*a0 + 2*a1) / 5);
					al[4] = (uint8)((2*a0 + 3*a1) / 5);
					al[5] = (uint8)((1*a0 + 4*a1) / 5);
					al[6] = 0; al[7] = 255;
				}
				// 48 bits LE en s[2..7]
				uint32 bits = 0;
				int bitpos = 0;
				// decodificar texel a texel
				for(int i = 0; i < 16; i++){
					int bytePos = 2 + (bitpos >> 3);
					int bitOff = bitpos & 7;
					int idx = (s[bytePos] >> bitOff) & 0x7;
					// puede cruzar byte
					if(bitOff > 5)
						idx |= (s[bytePos+1] << (8 - bitOff)) & 0x7;
					alpha[i] = al[idx & 7];
					bitpos += 3;
					(void)bits;
				}
				colSrc = s + 8;
			}
			uint16 c0 = (uint16)(colSrc[0] | (colSrc[1] << 8));
			uint16 c1 = (uint16)(colSrc[2] | (colSrc[3] << 8));
			uint8 r0, g0, b0, r1, g1, b1;
			dxtUnpack565(c0, &r0, &g0, &b0);
			dxtUnpack565(c1, &r1, &g1, &b1);
			cr[0] = r0; cg[0] = g0; cb[0] = b0; ca[0] = 255;
			cr[1] = r1; cg[1] = g1; cb[1] = b1; ca[1] = 255;
			if(dxt == 1 ? c0 > c1 : true){
				cr[2] = (uint8)((2*r0 + r1) / 3);
				cg[2] = (uint8)((2*g0 + g1) / 3);
				cb[2] = (uint8)((2*b0 + b1) / 3); ca[2] = 255;
				cr[3] = (uint8)((r0 + 2*r1) / 3);
				cg[3] = (uint8)((g0 + 2*g1) / 3);
				cb[3] = (uint8)((b0 + 2*b1) / 3); ca[3] = 255;
			}else{
				cr[2] = (uint8)((r0 + r1) / 2);
				cg[2] = (uint8)((g0 + g1) / 2);
				cb[2] = (uint8)((b0 + b1) / 2); ca[2] = 255;
				cr[3] = 0; cg[3] = 0; cb[3] = 0; ca[3] = 0;
			}
			uint32 idx = (uint32)(colSrc[4] | (colSrc[5] << 8) | (colSrc[6] << 16) | (colSrc[7] << 24));
			for(int py = 0; py < 4; py++){
				for(int px = 0; px < 4; px++){
					int x = bx*4 + px, y = by*4 + py;
					if(x >= w || y >= h) continue;
					int sel = (idx >> (2*(4*py+px))) & 3;
					uint8 *d = dst + y*dstPitch + x*4;
					d[0] = cr[sel]; d[1] = cg[sel]; d[2] = cb[sel];
					if(dxt == 1) d[3] = ca[sel];
					else d[3] = alpha[4*py+px];
				}
			}
			s += (dxt == 1 ? 8 : 16);
		}
	}
}

Texture*
readNativeTexture(Stream *stream)
{
	uint32 platform;
	if(!findChunk(stream, ID_STRUCT, nil, nil)){
		RWERROR((ERR_CHUNK, "STRUCT"));
		return nil;
	}
	platform = stream->readU32();
	if(platform != PLATFORM_GL3){
		RWERROR((ERR_PLATFORM, platform));
		return nil;
	}
	Texture *tex = Texture::create(nil);
	if(tex == nil)
		return nil;

	// Texture
	tex->filterAddressing = stream->readU32();
	stream->read8(tex->name, 32);
	stream->read8(tex->mask, 32);

	// Raster
	uint32 format = stream->readU32();
	int32 width = stream->readI32();
	int32 height = stream->readI32();
	int32 depth = stream->readI32();
	int32 numLevels = stream->readI32();

	// Native raster
	int32 subplatform = stream->readI32();
	int32 flags = stream->readI32();
	int32 compression = stream->readI32();

	if(subplatform != gl3Caps.gles){
		tex->destroy();
		RWERROR((ERR_PLATFORM, platform));
		return nil;
	}

	Raster *raster;
	Gl3Raster *natras;
	bool32 useSoftwareDXT = 0;
	bool32 usePalettized = 0;
	if(flags & 2){
		if(!gl3Caps.dxtSupported){
			// Sin S3TC (típico WebGL sin la extensión): decodificar por
			// software a RGBA8. Evita TXDs enteros en negro en el navegador.
			useSoftwareDXT = 1;
			uint32 ucformat = Raster::C8888 | Raster::TEXTURE | (format & (Raster::MIPMAP | Raster::AUTOMIPMAP));
			raster = Raster::create(width, height, depth, ucformat, PLATFORM_GL3);
		}else{
			raster = Raster::create(width, height, depth, format | Raster::TEXTURE | Raster::DONTALLOCATE, PLATFORM_GL3);
			allocateDXT(raster, compression, numLevels, flags & 1);
		}
	}else if(format & (Raster::PAL4 | Raster::PAL8)){
		// GL no tiene raster paletizado (rasterCreateTexture lo rechaza y el
		// TXD entero fallaba: radar*, wash*, water...). Expandir a C8888 al
		// leer, como readAsImage de d3d8.
		usePalettized = 1;
		uint32 ucformat = Raster::C8888 | Raster::TEXTURE | (format & (Raster::MIPMAP | Raster::AUTOMIPMAP));
		raster = Raster::create(width, height, depth, ucformat, PLATFORM_GL3);
	}else{
		raster = Raster::create(width, height, depth, format | Raster::TEXTURE, PLATFORM_GL3);
	}
	assert(raster);
	natras = GETGL3RASTEREXT(raster);
	tex->raster = raster;

	uint32 size;
	uint8 *data;
	if(useSoftwareDXT){
		for(int32 i = 0; i < numLevels; i++){
			int32 lw = width >> i; if(lw < 1) lw = 1;
			int32 lh = height >> i; if(lh < 1) lh = 1;
			size = stream->readU32();
			uint8 *tmp = (uint8*)rwMalloc(size, MEMDUR_EVENT | ID_DRIVER);
			if(tmp == nil){
				tex->destroy();
				return nil;
			}
			stream->read8(tmp, size);
			data = raster->lock(i, Raster::LOCKWRITE|Raster::LOCKNOFETCH);
			if(data == nil){
				rwFree(tmp);
				tex->destroy();
				return nil;
			}
			// stride real del raster desbloqueado
			int32 pitch = raster->stride;
			// raster->width/height ya están divididos por lock()
			dxtDecodeLevel(data, pitch, raster->width, raster->height, tmp, compression);
			raster->unlock(i);
			rwFree(tmp);
			(void)lw; (void)lh;
		}
		// WebGL2: cadena incompleta (fichero con menos niveles que la cadena
		// completa hasta 1x1) muestrea negro al minificar. Fijar MAX_LEVEL a
		// lo realmente subido.
		if(natras->numLevels > numLevels){
			uint32 prev = bindTexture(natras->texid);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numLevels-1);
			natras->numLevels = numLevels;
			bindTexture(prev);
		}
	}else if(usePalettized){
		int32 pal4 = (format & Raster::PAL4) != 0;
		int32 pallen = pal4 ? 16 : 256;
		uint8 palette[256*4];
		stream->read8(palette, pal4 ? 4*32 : 4*256);
		if(!Raster::formatHasAlpha(format))
			for(int32 pi = 0; pi < pallen; pi++)
				palette[pi*4+3] = 0xFF;
		for(int32 i = 0; i < numLevels; i++){
			size = stream->readU32();
			if(i >= raster->getNumLevels()){
				stream->seek(size);
				continue;
			}
			uint8 *idx = (uint8*)rwMalloc(size, MEMDUR_EVENT | ID_DRIVER);
			if(idx == nil){
				tex->destroy();
				return nil;
			}
			stream->read8(idx, size);
			data = raster->lock(i, Raster::LOCKWRITE|Raster::LOCKNOFETCH);
			if(data == nil){
				rwFree(idx);
				tex->destroy();
				return nil;
			}
			// raster->width/height ya divididos por lock(); destino C8888.
			int32 lw = raster->width, lh = raster->height;
			uint8 *dst = data;
			if(pal4){
				uint8 *p = idx;
				for(int32 y = 0; y < lh; y++)
					for(int32 x = 0; x < lw; x += 2){
						uint8 b = *p++;
						uint8 i0 = b >> 4;
						dst[0] = palette[i0*4+0]; dst[1] = palette[i0*4+1];
						dst[2] = palette[i0*4+2]; dst[3] = palette[i0*4+3]; dst += 4;
						if(x+1 < lw){
							uint8 i1 = b & 0xF;
							dst[0] = palette[i1*4+0]; dst[1] = palette[i1*4+1];
							dst[2] = palette[i1*4+2]; dst[3] = palette[i1*4+3]; dst += 4;
						}
					}
			}else{
				uint8 *p = idx;
				for(int32 n = 0; n < lw*lh; n++){
					uint8 v = *p++;
					dst[0] = palette[v*4+0]; dst[1] = palette[v*4+1];
					dst[2] = palette[v*4+2]; dst[3] = palette[v*4+3]; dst += 4;
				}
			}
			raster->unlock(i);
			rwFree(idx);
		}
		// Igual que software-DXT: si el fichero trae menos niveles que la
		// cadena completa, fijar MAX_LEVEL a lo subido (si no, negro en WebGL).
		if(natras->numLevels > numLevels){
			uint32 prev = bindTexture(natras->texid);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numLevels-1);
			natras->numLevels = numLevels;
			bindTexture(prev);
		}
	}else{
		for(int32 i = 0; i < numLevels; i++){
			size = stream->readU32();
			data = raster->lock(i, Raster::LOCKWRITE|Raster::LOCKNOFETCH);
			stream->read8(data, size);
			raster->unlock(i);
		}
	}
	return tex;
}

void
writeNativeTexture(Texture *tex, Stream *stream)
{
	Raster *raster = tex->raster;
	Gl3Raster *natras = GETGL3RASTEREXT(raster);

	int32 chunksize = getSizeNativeTexture(tex);
	writeChunkHeader(stream, ID_STRUCT, chunksize-12);
	stream->writeU32(PLATFORM_GL3);

	// Texture
	stream->writeU32(tex->filterAddressing);
	stream->write8(tex->name, 32);
	stream->write8(tex->mask, 32);

	// Raster
	int32 numLevels = natras->numLevels;
	stream->writeI32(raster->format);
	stream->writeI32(raster->width);
	stream->writeI32(raster->height);
	stream->writeI32(raster->depth);
	stream->writeI32(numLevels);

	// Native raster
	int32 flags = 0;
	int32 compression = 0;
	if(natras->hasAlpha)
		flags |= 1;
	if(natras->isCompressed){
		flags |= 2;
		switch(natras->internalFormat){
#ifdef RW_OPENGL
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			compression = 1;
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			compression = 3;
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			compression = 5;
			break;
#endif
		default:
			assert(0 && "unknown compression");
		}
	}
	stream->writeI32(gl3Caps.gles);
	stream->writeI32(flags);
	stream->writeI32(compression);
	// TODO: auto mipmaps?

	uint32 size;
	uint8 *data;
	for(int32 i = 0; i < numLevels; i++){
		size = getLevelSize(raster, i);
		stream->writeU32(size);
		data = raster->lock(i, Raster::LOCKREAD);
		stream->write8(data, size);
		raster->unlock(i);
	}
}

uint32
getSizeNativeTexture(Texture *tex)
{
	uint32 size = 12 + 72 + 32;
	int32 levels = tex->raster->getNumLevels();
	for(int32 i = 0; i < levels; i++)
		size += 4 + getLevelSize(tex->raster, i);
	return size;
}



void registerNativeRaster(void)
{
	nativeRasterOffset = Raster::registerPlugin(sizeof(Gl3Raster),
	                                            ID_RASTERGL3,
	                                            createNativeRaster,
	                                            destroyNativeRaster,
	                                            copyNativeRaster);
}

}
}
