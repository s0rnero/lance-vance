#define WITHD3D
#include "common.h"
#include <rpskin.h>

#include "Timecycle.h"
#include "skeleton.h"
#include "Debug.h"
#include "MBlur.h"
#if !defined(FINAL) || defined(DEBUGMENU)
#include "rtcharse.h"
#endif
#ifndef FINAL
RtCharset *debugCharset;
bool bDebugRenderGroups;
#endif

#ifdef PS2_ALPHA_TEST
#ifdef VICEEXT_PERF_MINIMAL
bool gPS2alphaTest = false;
#else
bool gPS2alphaTest = true;
#endif
#else
bool gPS2alphaTest = false;
#endif
#ifdef VICEEXT_VICE_EXTENDED
bool gBackfaceCulling = false;
#else
bool gBackfaceCulling = true;
#endif

#if !defined(FINAL) || defined(DEBUGMENU)
static bool charsetOpen;
void OpenCharsetSafe()
{
	if(!charsetOpen)
		RtCharsetOpen();
	charsetOpen = true;
}
#endif

void CreateDebugFont()
{
#ifndef FINAL
	RwRGBA color = { 255, 255, 128, 255 };
	RwRGBA colorbg = { 0, 0, 0, 0 };
	OpenCharsetSafe();
	debugCharset = RtCharsetCreate(&color, &colorbg);
#endif
}

void DestroyDebugFont()
{
#ifndef FINAL
	RtCharsetDestroy(debugCharset);
	RtCharsetClose();
	charsetOpen = false;
#endif
}

void ObrsPrintfString(const char *str, short x, short y)
{
#ifndef FINAL
	RtCharsetPrintBuffered(debugCharset, str, x*8, y*16, true);
#endif
}

void FlushObrsPrintfs()
{
#ifndef FINAL
	RtCharsetBufferFlush();
#endif
}

void
DefinedState(void)
{
	RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSWRAP);
	RwRenderStateSet(rwRENDERSTATETEXTUREPERSPECTIVE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
	//RwRenderStateSet(rwRENDERSTATEALPHAPRIMITIVEBUFFER, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEBORDERCOLOR, (void*)RWRGBALONG(0, 0, 0, 255));
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEFOGCOLOR,
		(void*)RWRGBALONG(CTimeCycle::GetFogRed(), CTimeCycle::GetFogGreen(), CTimeCycle::GetFogBlue(), 255));
	RwRenderStateSet(rwRENDERSTATEFOGTYPE, (void*)rwFOGTYPELINEAR);
	RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);

#ifdef LIBRW
	rw::SetRenderState(rw::ALPHATESTFUNC, rw::ALPHAGREATEREQUAL);
	rw::SetRenderState(rw::ALPHATESTREF, 3);

	rw::SetRenderState(rw::GSALPHATEST, gPS2alphaTest);
#else
	// D3D stuff
	RwD3D8SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	RwD3D8SetRenderState(D3DRS_ALPHAREF, 2);
#endif
}

void
SetCullMode(uint32 mode)
{
	if(gBackfaceCulling)
		RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)mode);
	else
		RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
}

#ifndef FINAL
#ifdef __EMSCRIPTEN__
#include "ondemand.h"
#define OD_RG_MAX 48
static const char *s_rgName[OD_RG_MAX];
static double s_rgMs[OD_RG_MAX];
static uint32 s_rgN[OD_RG_MAX];
static double s_rgMax[OD_RG_MAX];
static double s_rgT0[OD_RG_MAX];
static int s_rgStack[OD_RG_MAX];
static int s_rgCount;
static int s_rgDepth;
static uint32 s_rgWinStart;
static uint32 s_rgWinSeq;
static uint32 s_rgWinCalls;
static uint32 s_rgOvf;
static uint32 s_rgSkp;
static uint32 s_rgUnf;

static void
OdGroupEmit(uint32 windowMs)
{
	for (int i = 0; i < s_rgCount; i++) {
		if (s_rgN[i] == 0u)
			continue;
		s_rgWinSeq++;
		char t[256];
		snprintf(t, sizeof t, "RGPASS schema=2 win=%u window_ms=%u calls=%u total_ms=%.3f max_ms=%.3f name=%s ovf=%u skp=%u unf=%u",
		         s_rgWinSeq, windowMs, s_rgN[i], s_rgMs[i], s_rgMax[i], s_rgName[i],
		         s_rgOvf, s_rgSkp, s_rgUnf);
		ODTRACES(t);
	}
	if (s_rgOvf || s_rgSkp || s_rgUnf) {
		char t[192];
		snprintf(t, sizeof t, "RGPASS schema=2 win=%u window_ms=%u incident=1 calls=%u ovf=%u skp=%u unf=%u",
		         s_rgWinSeq, windowMs, s_rgWinCalls, s_rgOvf, s_rgSkp, s_rgUnf);
		ODTRACES(t);
	}
	for (int i = 0; i < s_rgCount; i++) {
		s_rgMs[i] = 0.0;
		s_rgN[i] = 0u;
		s_rgMax[i] = 0.0;
	}
	s_rgOvf = 0u;
	s_rgSkp = 0u;
	s_rgUnf = 0u;
	s_rgWinCalls = 0u;
}

static void
OdGroupTick(void)
{
	uint32 now = (uint32)emscripten_get_now();
	if (s_rgWinStart == 0u)
		s_rgWinStart = now;
	if (s_rgDepth == 0) {
		uint32 w = (uint32)(now - s_rgWinStart);
		if (w >= 2000u) {
			if (s_rgCount != 0)
				OdGroupEmit(w);
			s_rgWinStart = now;
		}
	}
}
#endif

void
PushRendergroup(const char *name, bool timed)
{
#ifdef __EMSCRIPTEN__
	OdGroupTick();
	if (s_rgDepth >= OD_RG_MAX) {
		s_rgSkp++;
		s_rgDepth++;
	} else if (timed) {
		int i;
		for (i = 0; i < s_rgCount; i++)
			if (s_rgName[i] == name)
				break;
		if (i == s_rgCount) {
			if (s_rgCount < OD_RG_MAX) {
				s_rgName[i] = name;
				s_rgMs[i] = 0.0;
				s_rgN[i] = 0u;
				s_rgMax[i] = 0.0;
				s_rgCount++;
			} else {
				s_rgOvf++;
			}
		}
		if (i < s_rgCount) {
			s_rgT0[s_rgDepth] = emscripten_get_now();
			s_rgStack[s_rgDepth] = i;
		} else {
			s_rgStack[s_rgDepth] = -1;
		}
		s_rgDepth++;
	} else {
		s_rgStack[s_rgDepth] = -1;
		s_rgDepth++;
	}
#endif
	if(!bDebugRenderGroups)
		return;
#if defined(RW_OPENGL)
	if(GLAD_GL_KHR_debug)
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name);
#elif defined(RW_D3D9)
	static WCHAR tmp[256];
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name, -1, tmp, sizeof(tmp));
	D3DPERF_BeginEvent(0xFFFFFFFF, tmp);
#endif
}

void
PopRendergroup(void)
{
#ifdef __EMSCRIPTEN__
	if (s_rgDepth <= 0) {
		s_rgUnf++;
	} else {
		int slot = --s_rgDepth;
		int i = slot < OD_RG_MAX ? s_rgStack[slot] : -1;
		if (i >= 0) {
			double d = emscripten_get_now() - s_rgT0[slot];
			if (d >= 0.0) {
				s_rgMs[i] += d;
				s_rgN[i]++;
				if (d > s_rgMax[i])
					s_rgMax[i] = d;
				s_rgWinCalls++;
			}
		}
	}
#endif
	if(!bDebugRenderGroups)
		return;
#if defined(RW_OPENGL)
	if(GLAD_GL_KHR_debug)
		glPopDebugGroup();
#elif defined(RW_D3D9)
	D3DPERF_EndEvent();
#endif
}
#endif

RwFrame*
GetFirstFrameCallback(RwFrame *child, void *data)
{
	*(RwFrame**)data = child;
	return nil;
}

RwFrame*
GetFirstChild(RwFrame *frame)
{
	RwFrame *child;

	child = nil;
	RwFrameForAllChildren(frame, GetFirstFrameCallback, &child);
	return child;
}

RwObject*
GetFirstObjectCallback(RwObject *object, void *data)
{
	*(RwObject**)data = object;
	return nil;
}

RwObject*
GetFirstObject(RwFrame *frame)
{
	RwObject *obj;

	obj = nil;
	RwFrameForAllObjects(frame, GetFirstObjectCallback, &obj);
	return obj;
}

RpAtomic*
GetFirstAtomicCallback(RpAtomic *atm, void *data)
{
	*(RpAtomic**)data = atm;
	return nil;
}

RpAtomic*
GetFirstAtomic(RpClump *clump)
{
	RpAtomic *atm;

	atm = nil;
	RpClumpForAllAtomics(clump, GetFirstAtomicCallback, &atm);
	return atm;
}

RwTexture*
GetFirstTextureCallback(RwTexture *tex, void *data)
{
	*(RwTexture**)data = tex;
	return nil;
}

RwTexture*
GetFirstTexture(RwTexDictionary *txd)
{
	RwTexture *tex;

	tex = nil;
	RwTexDictionaryForAllTextures(txd, GetFirstTextureCallback, &tex);
	return tex;
}

bool
IsClumpSkinned(RpClump *clump)
{
	RpAtomic *atomic = GetFirstAtomic(clump);
	return atomic ? RpSkinGeometryGetSkin(RpAtomicGetGeometry(atomic)) : nil;
}

static RpAtomic*
GetAnimHierarchyCallback(RpAtomic *atomic, void *data)
{
	*(RpHAnimHierarchy**)data = RpSkinAtomicGetHAnimHierarchy(atomic);
	return nil;
}

RpHAnimHierarchy*
GetAnimHierarchyFromSkinClump(RpClump *clump)
{
	RpHAnimHierarchy *hier = nil;
	RpClumpForAllAtomics(clump, GetAnimHierarchyCallback, &hier);
	return hier;
}

static RwFrame*
GetAnimHierarchyFromClumpCB(RwFrame *frame, void *data)
{
	RpHAnimHierarchy *hier = RpHAnimFrameGetHierarchy(frame);
	if(hier){
		*(RpHAnimHierarchy**)data = hier;
		return nil;
	}
	RwFrameForAllChildren(frame, GetAnimHierarchyFromClumpCB, data);
	return frame;
}

RpHAnimHierarchy*
GetAnimHierarchyFromClump(RpClump *clump)
{
	RpHAnimHierarchy *hier = nil;
	RwFrameForAllChildren(RpClumpGetFrame(clump), GetAnimHierarchyFromClumpCB, &hier);
	return hier;
}

void
SkinGetBonePositionsToTable(RpClump *clump, RwV3d *boneTable)
{
	int i, parent;
	RpAtomic *atomic;
	RpSkin *skin;
	RpHAnimHierarchy *hier;
	int numBones;
	RwMatrix m, invmat;
	int stack[32];
	int sp;

	if(boneTable == nil)
		return;

	atomic = GetFirstAtomic(clump);		// mobile, also VC
	assert(atomic);
	skin = RpSkinGeometryGetSkin(RpAtomicGetGeometry(atomic));
	assert(skin);
	hier = GetAnimHierarchyFromSkinClump(clump);
	assert(hier);
	boneTable[0].x = 0.0f;
	boneTable[0].y = 0.0f;
	boneTable[0].z = 0.0f;
	numBones = RpSkinGetNumBones(skin);
	parent = 0;
	sp = 0;
#ifdef FIX_BUGS
	stack[0] = 0;	// i think this is ok
#endif
	for(i = 1; i < numBones; i++){
		RwMatrixCopy(&m, &RpSkinGetSkinToBoneMatrices(skin)[i]);
		RwMatrixInvert(&invmat, &m);
		const RwMatrix *x = RpSkinGetSkinToBoneMatrices(skin);
		RwV3dTransformPoints(&boneTable[i], &invmat.pos, 1, &x[parent]);
		if(HIERNODEINFO(hier)[i].flags & rpHANIMPUSHPARENTMATRIX)
			stack[++sp] = parent;
		if(HIERNODEINFO(hier)[i].flags & rpHANIMPOPPARENTMATRIX)
			parent = stack[sp--];
		else
			parent = i;

		//assert(parent >= 0 && parent < numBones);
	}
}

RpHAnimAnimation*
HAnimAnimationCreateForHierarchy(RpHAnimHierarchy *hier)
{
	int i;
#if defined FIX_BUGS || defined LIBRW
	int numNodes = hier->numNodes*2;	// you're supposed to have at least two KFs per node
#else
	int numNodes = hier->numNodes;
#endif
	RpHAnimAnimation *anim = RpHAnimAnimationCreate(rpHANIMSTDKEYFRAMETYPEID, numNodes, 0, 0.0f);
	if(anim == nil)
		return nil;
	RpHAnimStdKeyFrame *frame;
	for(i = 0; i < numNodes; i++){
		frame = (RpHAnimStdKeyFrame*)HANIMFRAME(anim, i);	// games uses struct size here, not safe
		frame->q.real = 1.0f;
		frame->q.imag.x = frame->q.imag.y = frame->q.imag.z = 0.0f;
		frame->t.x = frame->t.y = frame->t.z = 0.0f;
#if defined FIX_BUGS || defined LIBRW
		// times are subtracted and divided giving NaNs
		// so they can't both be 0
		frame->time = i/hier->numNodes;
#else
		frame->time = 0.0f;
#endif
		frame->prevFrame = nil;
	}
	return anim;
}

void
RenderSkeleton(RpHAnimHierarchy *hier)
{
	int i;
	int sp;
	int stack[32];
	int par;
	CVector p1, p2;
	int numNodes = hier->numNodes;
	RwMatrix *mats = RpHAnimHierarchyGetMatrixArray(hier);
	p1 = mats[0].pos;

	par = 0;
	sp = 0;
	stack[sp++] = par;
	for(i = 1; i < numNodes; i++){
		p1 = mats[par].pos;
		p2 = mats[i].pos;
		CDebug::AddLine(p1, p2, 0xFFFFFFFF, 0xFFFFFFFF);
		if(HIERNODEINFO(hier)[i].flags & rpHANIMPUSHPARENTMATRIX)
			stack[sp++] = par;
		par = i;
		if(HIERNODEINFO(hier)[i].flags & rpHANIMPOPPARENTMATRIX)
			par = stack[--sp];
	}
}


RwBool Im2DRenderQuad(RwReal x1, RwReal y1, RwReal x2, RwReal y2, RwReal z, RwReal recipCamZ, RwReal uvOffset)
{
    RwIm2DVertex        vx[4];

    /*
     *  Render an opaque white 2D quad at the given coordinates and
     *  spanning a whole texture.
     */

    RwIm2DVertexSetScreenX(&vx[0], x1);
    RwIm2DVertexSetScreenY(&vx[0], y1);
    RwIm2DVertexSetScreenZ(&vx[0], z);
    RwIm2DVertexSetIntRGBA(&vx[0], 255, 255, 255, 255);
    RwIm2DVertexSetRecipCameraZ(&vx[0], recipCamZ);
    RwIm2DVertexSetU(&vx[0], uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[0], uvOffset, recipCamZ);

    RwIm2DVertexSetScreenX(&vx[1], x1);
    RwIm2DVertexSetScreenY(&vx[1], y2);
    RwIm2DVertexSetScreenZ(&vx[1], z);
    RwIm2DVertexSetIntRGBA(&vx[1], 255, 255, 255, 255);
    RwIm2DVertexSetRecipCameraZ(&vx[1], recipCamZ);
    RwIm2DVertexSetU(&vx[1], uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[1], 1.0f + uvOffset, recipCamZ);

    RwIm2DVertexSetScreenX(&vx[2], x2);
    RwIm2DVertexSetScreenY(&vx[2], y1);
    RwIm2DVertexSetScreenZ(&vx[2], z);
    RwIm2DVertexSetIntRGBA(&vx[2], 255, 255, 255, 255);
    RwIm2DVertexSetRecipCameraZ(&vx[2], recipCamZ);
    RwIm2DVertexSetU(&vx[2], 1.0f + uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[2], uvOffset, recipCamZ);

    RwIm2DVertexSetScreenX(&vx[3], x2);
    RwIm2DVertexSetScreenY(&vx[3], y2);
    RwIm2DVertexSetScreenZ(&vx[3], z);
    RwIm2DVertexSetIntRGBA(&vx[3], 255, 255, 255, 255);
    RwIm2DVertexSetRecipCameraZ(&vx[3], recipCamZ);
    RwIm2DVertexSetU(&vx[3], 1.0f + uvOffset, recipCamZ);
    RwIm2DVertexSetV(&vx[3], 1.0f + uvOffset, recipCamZ);

    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, vx, 4);

    return TRUE;
}

bool b_cbsUseLTM = true;

RpAtomic *cbsCalcMeanBSphereRadiusCB(RpAtomic *atomic, void *data)
{
	RwV3d atomicPos;

	if ( b_cbsUseLTM )
		RwV3dTransformPoints(&atomicPos, &RpAtomicGetBoundingSphere(atomic)->center, 1, RwFrameGetLTM(RpClumpGetFrame(atomic->clump)));
	else
		RwV3dTransformPoints(&atomicPos, &RpAtomicGetBoundingSphere(atomic)->center, 1, RwFrameGetMatrix(RpClumpGetFrame(atomic->clump)));

	RwV3d temp;
	RwV3dSub(&temp, &atomicPos, &((RwSphere *)data)->center);	
	RwReal radius = RwV3dLength(&temp) + RpAtomicGetBoundingSphere(atomic)->radius;
	
	if ( ((RwSphere *)data)->radius < radius )
		((RwSphere *)data)->radius = radius;

	return atomic;
}

RpAtomic *cbsCalcMeanBSphereCenterCB(RpAtomic *atomic, void *data)
{
	RwV3d atomicPos;

	if ( b_cbsUseLTM )
		RwV3dTransformPoints(&atomicPos, &RpAtomicGetBoundingSphere(atomic)->center, 1, RwFrameGetLTM(RpClumpGetFrame(atomic->clump)));
	else
		RwV3dTransformPoints(&atomicPos, &RpAtomicGetBoundingSphere(atomic)->center, 1, RwFrameGetMatrix(RpClumpGetFrame(atomic->clump)));
	
	RwV3dAdd(&((RwSphere *)data)->center, &((RwSphere *)data)->center, &atomicPos);
	
	return atomic;
}

RpClump *RpClumpGetBoundingSphere(RpClump *clump, RwSphere *sphere, bool useLTM)
{
	RwMatrix matrix;
	RwSphere result = { 0.0f, 0.0f, 0.0f, 0.0f };
	
	b_cbsUseLTM = useLTM;
 
	if ( clump == nil || sphere == nil )
		return nil;
  
	sphere->radius = 0.0f;
	sphere->center.x = 0.0f;
	sphere->center.y = 0.0f;
	sphere->center.z = 0.0f;
    
	RwInt32 numAtomics = RpClumpGetNumAtomics(clump);
	if ( numAtomics < 1.0f )
		return nil;
	
	RpClumpForAllAtomics(clump, cbsCalcMeanBSphereCenterCB, &result);
	
	RwV3dScale(&result.center, &result.center, 1.0f/numAtomics);
	
	RpClumpForAllAtomics(clump, cbsCalcMeanBSphereRadiusCB, &result);
	
	if ( b_cbsUseLTM )
		RwMatrixInvert(&matrix, RwFrameGetLTM(RpClumpGetFrame(clump)));
	else
		RwMatrixInvert(&matrix, RwFrameGetMatrix(RpClumpGetFrame(clump)));
	
	RwV3dTransformPoints(&result.center, &result.center, 1, &matrix);

	*sphere = result;
	
	return clump;
}

void
CameraSize(RwCamera * camera, RwRect * rect,
		   RwReal viewWindow, RwReal aspectRatio)
{
	if (camera)
	{
		RwVideoMode         videoMode;
		RwRect              r;
		RwRect              origSize = { 0, 0, 0, 0 };	// FIX just to make the compier happy
		RwV2d               vw;

		RwEngineGetVideoModeInfo(&videoMode,
								 RwEngineGetCurrentVideoMode());

		origSize.w  = RwRasterGetWidth(RwCameraGetRaster(camera));
		origSize.h = RwRasterGetHeight(RwCameraGetRaster(camera));

		if (!rect)
		{
			if (videoMode.flags & rwVIDEOMODEEXCLUSIVE)
			{
				/* For full screen applications, resizing the camera just doesn't
				 * make sense, use the video mode size.
				 */

				r.x = r.y = 0;
				r.w = videoMode.width;
				r.h = videoMode.height;
				rect = &r;
			}
			else
			{
				/*
				rect not specified - reuse current values
				*/
				r.w = RwRasterGetWidth(RwCameraGetRaster(camera));
				r.h = RwRasterGetHeight(RwCameraGetRaster(camera));
				r.x = r.y = 0;
				rect = &r;
			}
		}

		if (( origSize.w != rect->w ) || ( origSize.h != rect->h ))
		{
			RwRaster           *raster;
			RwRaster           *zRaster;

			// BUG: game just changes camera raster's sizes, but this is a hack
#if defined FIX_BUGS || defined LIBRW
			/*
			 * Destroy rasters...
			 */

			raster = RwCameraGetRaster(camera);
			if( raster )
			{
				RwRasterDestroy(raster);
				camera->frameBuffer = nil;
			}

			zRaster = RwCameraGetZRaster(camera);
			if( zRaster )
			{
				RwRasterDestroy(zRaster);
				camera->zBuffer = nil;
			}

			/*
			 * Create new rasters...
			 */

			raster = RwRasterCreate(rect->w, rect->h, 0, rwRASTERTYPECAMERA);
			zRaster = RwRasterCreate(rect->w, rect->h, 0, rwRASTERTYPEZBUFFER);

			if( raster && zRaster )
			{
				RwCameraSetRaster(camera, raster);
				RwCameraSetZRaster(camera, zRaster);
			}
			else
			{
				if( raster )
				{
					RwRasterDestroy(raster);
				}

				if( zRaster )
				{
					RwRasterDestroy(zRaster);
				}

				rect->x = origSize.x;
				rect->y = origSize.y;
				rect->w = origSize.w;
				rect->h = origSize.h;

				/*
				 * Use default values...
				 */
				raster =
					RwRasterCreate(rect->w, rect->h, 0, rwRASTERTYPECAMERA);

				zRaster =
					RwRasterCreate(rect->w, rect->h, 0, rwRASTERTYPEZBUFFER);

				RwCameraSetRaster(camera, raster);
				RwCameraSetZRaster(camera, zRaster);
			}
#else
			raster = RwCameraGetRaster(camera);
			zRaster = RwCameraGetZRaster(camera);
			
			raster->width = zRaster->width = rect->w;
			raster->height = zRaster->height = rect->h;
#endif
#ifdef FIX_BUGS
			if(CMBlur::BlurOn){
				CMBlur::MotionBlurClose();
				CMBlur::MotionBlurOpen(camera);
			}
#endif
		}

		/* Figure out the view window */
		if (videoMode.flags & rwVIDEOMODEEXCLUSIVE)
		{
			/* derive ratio from aspect ratio */
			vw.x = viewWindow;
			vw.y = viewWindow / aspectRatio;
		}
		else
		{
			/* derive from pixel ratios */
			if (rect->w > rect->h)
			{
				vw.x = viewWindow;
				vw.y = (rect->h * viewWindow) / rect->w;
			}
			else
			{
				vw.x = (rect->w * viewWindow) / rect->h;
				vw.y = viewWindow;
			}
		}

		RwCameraSetViewWindow(camera, &vw);

		RsGlobal.width  = rect->w;
		RsGlobal.height = rect->h;
	}

	return;
}

void
CameraDestroy(RwCamera *camera)
{
	RwRaster    *raster, *tmpRaster;
	RwFrame     *frame;

	if (camera)
	{
		frame = RwCameraGetFrame(camera);
		if (frame)
		{
			RwFrameDestroy(frame);
		}

		raster = RwCameraGetRaster(camera);
		if (raster)
		{
			tmpRaster = RwRasterGetParent(raster);

			RwRasterDestroy(raster);

			if ((tmpRaster != nil) && (tmpRaster != raster))
			{
				RwRasterDestroy(tmpRaster);
			}
		}

		raster = RwCameraGetZRaster(camera);
		if (raster)
		{
			tmpRaster = RwRasterGetParent(raster);

			RwRasterDestroy(raster);

			if ((tmpRaster != nil) && (tmpRaster != raster))
			{
				RwRasterDestroy(tmpRaster);
			}
		}

		RwCameraDestroy(camera);
	}

	return;
}

RwCamera           *
CameraCreate(RwInt32 width, RwInt32 height, RwBool zBuffer)
{
	RwCamera           *camera;

	camera = RwCameraCreate();

	if (camera)
	{
		RwCameraSetFrame(camera, RwFrameCreate());
		RwCameraSetRaster(camera,
						  RwRasterCreate(0, 0, 0, rwRASTERTYPECAMERA));

		if (zBuffer)
		{
			RwCameraSetZRaster(camera,
							   RwRasterCreate(0, 0, 0,
											  rwRASTERTYPEZBUFFER));
		}

		/* now check that everything is valid */
		if (RwCameraGetFrame(camera) &&
			RwCameraGetRaster(camera) &&
			RwRasterGetParent(RwCameraGetRaster(camera)) &&
			(!zBuffer || (RwCameraGetZRaster(camera) &&
						  RwRasterGetParent(RwCameraGetZRaster
											(camera)))))
		{
			/* everything OK */
			return (camera);
		}
	}

	/* if we're here then an error must have occurred so clean up */

	CameraDestroy(camera);
	return (nil);
}

#ifdef LIBRW
#include <rpmatfx.h>
#include "VehicleModelInfo.h"

int32
findPlatform(rw::Atomic *a)
{
	rw::Geometry *g = a->geometry;
	if(g->instData)
		return g->instData->platform;
	return 0;
}

// Game doesn't read atomic extensions so we never get any other than the default pipe,
// but we need it for uninstancing
void
attachPipe(rw::Atomic *atomic)
{
	if(RpSkinGeometryGetSkin(RpAtomicGetGeometry(atomic)))
		atomic->pipeline = rw::skinGlobals.pipelines[rw::platform];
	else{
		int fx = rpMATFXEFFECTNULL;
		RpGeometryForAllMaterials(RpAtomicGetGeometry(atomic), CVehicleModelInfo::GetMatFXEffectMaterialCB, &fx);
		if(fx != rpMATFXEFFECTNULL)
			RpMatFXAtomicEnableEffects(atomic);
	}
}

// Attach pipes for the platform we have native data for so we can uninstance
void
switchPipes(rw::Atomic *a, int32 platform)
{
	if(a->pipeline && a->pipeline->platform != platform){
		uint32 plgid = a->pipeline->pluginID;
		switch(plgid){
		// assume default pipe won't be attached explicitly
		case rw::ID_SKIN:
			a->pipeline = rw::skinGlobals.pipelines[platform];
			break;
		case rw::ID_MATFX:
			a->pipeline = rw::matFXGlobals.pipelines[platform];
			break;
		}
	}
}

RpAtomic*
ConvertPlatformAtomic(RpAtomic *atomic, void *data)
{
	int32 driver = rw::platform;
	int32 platform = findPlatform(atomic);
	if(platform != 0 && platform != driver){
		attachPipe(atomic);	// kludge
		rw::ObjPipeline *origPipe = atomic->pipeline;
		rw::platform = platform;
		switchPipes(atomic, rw::platform);
		if(atomic->geometry->flags & rw::Geometry::NATIVE)
			atomic->uninstance();
		// no ADC in this game
		//rw::ps2::unconvertADC(atomic->geometry);
		rw::platform = driver;
		atomic->pipeline = origPipe;
	}
	return atomic;
}
#endif

#if defined(FIX_BUGS) && defined(GTA_PC)
RwUInt32 saved_alphafunc, saved_alpharef;

void
SetAlphaTest(RwUInt32 alpharef)
{
#ifdef LIBRW
	saved_alphafunc = rw::GetRenderState(rw::ALPHATESTFUNC);
	saved_alpharef = rw::GetRenderState(rw::ALPHATESTREF);

	rw::SetRenderState(rw::ALPHATESTFUNC, rw::ALPHAGREATEREQUAL);
	rw::SetRenderState(rw::ALPHATESTREF, 0);
#else
	RwD3D8GetRenderState(D3DRS_ALPHAFUNC, &saved_alphafunc);
	RwD3D8GetRenderState(D3DRS_ALPHAREF, &saved_alpharef);
	
	RwD3D8SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
	RwD3D8SetRenderState(D3DRS_ALPHAREF, alpharef);
#endif
}

void
RestoreAlphaTest()
{
#ifdef LIBRW
	rw::SetRenderState(rw::ALPHATESTFUNC, saved_alphafunc);
	rw::SetRenderState(rw::ALPHATESTREF, saved_alpharef);
#else
	RwD3D8SetRenderState(D3DRS_ALPHAFUNC, saved_alphafunc);
	RwD3D8SetRenderState(D3DRS_ALPHAREF, saved_alpharef);
#endif
}
#endif
#ifdef __EMSCRIPTEN__
#include "Pools.h"
#include <emscripten/heap.h>
#include "Timer.h"
#include "ModelInfo.h"
struct PedClumpRec {
	int slot; unsigned long long gen; int model; void *ped; void *clump;
	unsigned long long serial; int state; int lastPhase; int lastGoodPhase;
	char modelName[24];
};
static PedClumpRec *g_pedClumpTable;
static int g_pedClumpCount;
static unsigned long long g_pedClumpGen;
static unsigned long long g_pedClumpSerialSeq;
static unsigned long long g_pedClumpEpoch;
#define PEDCLUMP_RING 128
static char g_pedClumpRing[PEDCLUMP_RING][224];
static int g_pedClumpRingHead;
static int g_pedClumpRingN;
static int g_pedClumpRingOvf;
static PedClumpRec g_pedClumpFirstBad;
static bool g_pedClumpFirstBadSet;
static void *g_pedClumpLastClump;
static int g_pedClumpLastSerial;
static uint32 g_pedClumpSummaryMs;
static PedClumpRec *
PedClumpByPed(void *ped)
{
	if (!g_pedClumpTable || !ped)
		return nil;
	for (int i = 0; i < g_pedClumpCount; i++)
		if (g_pedClumpTable[i].ped == ped && g_pedClumpTable[i].state != 0)
			return &g_pedClumpTable[i];
	return nil;
}
static void
PedClumpRingLine(const char *kind, PedClumpRec *r, int phase)
{
	char t[224];
	snprintf(t, sizeof t, "PEDCLUMP schema=1 seq=%llu frame=%u phase=%d ev=%s slot=%d gen=%llu model=%d nombre=%s ped=%p clump=%p serial=%llu state=%d lastGood=%d ringOvf=%d lost=%s",
	         g_pedClumpSerialSeq, (unsigned)CTimer::GetFrameCounter(), phase, kind,
	         r ? r->slot : -1, r ? r->gen : 0ULL, r ? r->model : -1, r ? r->modelName : "-",
	         r ? r->ped : nil, r ? r->clump : nil, r ? r->serial : 0ULL, r ? r->state : -1,
	         r ? r->lastGoodPhase : -1, g_pedClumpRingOvf, g_pedClumpRingOvf ? "si" : "no");
	snprintf(g_pedClumpRing[g_pedClumpRingHead], sizeof g_pedClumpRing[g_pedClumpRingHead], "%s", t);
	g_pedClumpRingHead = (g_pedClumpRingHead + 1) % PEDCLUMP_RING;
	if (g_pedClumpRingN < PEDCLUMP_RING)
		g_pedClumpRingN++;
	else
		g_pedClumpRingOvf++;
}
static void
PedClumpFatalDump(PedClumpRec *r, const char *why, int phase)
{
	if (!r)
		return;
	if (!g_pedClumpFirstBadSet) {
		g_pedClumpFirstBad = *r;
		g_pedClumpFirstBadSet = true;
		PedClumpRingLine("first_bad", r, phase);
	}
	char dump[49152];
	int pos = 0;
	pos = snprintf(dump + pos, sizeof dump - pos, "PEDCLUMP schema=1 fatal=%s phase=%d frame=%u slot=%d gen=%llu model=%d nombre=%s ped=%p clump=%p serial=%llu lastGood=%d ringOvf=%d first_bad_slot=%d first_bad_gen=%llu first_bad_model=%d first_bad_ped=%p first_bad_clump=%p first_bad_serial=%llu\n",
	                why, phase, (unsigned)CTimer::GetFrameCounter(), r->slot, r->gen, r->model, r->modelName,
	                r->ped, r->clump, r->serial, r->lastGoodPhase, g_pedClumpRingOvf,
	                g_pedClumpFirstBad.slot, g_pedClumpFirstBad.gen, g_pedClumpFirstBad.model,
	                g_pedClumpFirstBad.ped, g_pedClumpFirstBad.clump, g_pedClumpFirstBad.serial, "\n");
	int start = (g_pedClumpRingHead - g_pedClumpRingN + PEDCLUMP_RING) % PEDCLUMP_RING;
	for (int i = 0; i < g_pedClumpRingN; i++) {
		int idx = (start + i) % PEDCLUMP_RING;
		int w = snprintf(dump + pos, sizeof dump - pos, "%s\n", g_pedClumpRing[idx]);
		if (w < 0) break;
		if (w >= (int)(sizeof dump - pos)) { pos = (int)sizeof dump - 1; break; }
		pos += w;
	}
	ODTRACES(dump);
	EM_ASM({ if (window.__vcPedClumpFatal) window.__vcPedClumpFatal($0); }, dump);
	abort();
}
static bool
PedClumpInHeap(void *p, int32 size)
{
	uint32 heap = (uint32)emscripten_get_heap_size();
	uint32 v = (uint32)(uintptr_t)p;
	return p != nil && v <= heap && (uint32)size <= heap - v;
}
static void
PedClumpVendorCb(int ev, void *clump, int serial)
{
	if (ev == 1) {
		g_pedClumpLastClump = clump;
		g_pedClumpLastSerial = serial;
		return;
	}
	for (int i = 0; i < g_pedClumpCount; i++) {
		if (g_pedClumpTable[i].clump == clump && g_pedClumpTable[i].state == 2) {
			char t[64];
			snprintf(t, sizeof t, "vendor_ev=%d serial=%d", ev, serial ? serial : g_pedClumpLastSerial);
			PedClumpRingLine(t, &g_pedClumpTable[i], -1);
			return;
		}
	}
}
void
PedClumpInit(int count)
{
	if (g_pedClumpTable || count <= 0)
		return;
	rw::rwClumpDiagCb = PedClumpVendorCb;
	g_pedClumpTable = new PedClumpRec[count];
	g_pedClumpCount = count;
	char t[96];
	snprintf(t, sizeof t, "PEDCLUMP schema=1 activation count=%d epoch=%llu", count, g_pedClumpEpoch);
	ODTRACES(t);
}
void
PedClumpShutdown(void)
{
	if (g_pedClumpTable) {
		delete[] g_pedClumpTable;
		g_pedClumpTable = nil;
	}
	g_pedClumpCount = 0;
	g_pedClumpFirstBadSet = false;
	g_pedClumpRingN = 0;
	g_pedClumpRingHead = 0;
	g_pedClumpRingOvf = 0;
	g_pedClumpEpoch++;
}
void
PedClumpAlloc(int slot, void *ped)
{
	PedClumpRec *r = nil;
	if (g_pedClumpTable && slot >= 0 && slot < g_pedClumpCount) {
		r = &g_pedClumpTable[slot];
		if (r->state != 0)
			PedClumpRingLine("alloc_reuse_unknown", r, -1);
		*r = PedClumpRec();
		r->slot = slot;
		r->gen = ++g_pedClumpGen;
		r->model = -1;
		r->ped = ped;
		r->serial = ++g_pedClumpSerialSeq;
		r->state = 1;
		r->lastPhase = -1;
		r->lastGoodPhase = -1;
		snprintf(r->modelName, sizeof r->modelName, "-");
	}
	PedClumpRingLine("alloc", r, -1);
}
void
PedClumpFree(void *ped)
{
	PedClumpRec *r = PedClumpByPed(ped);
	if (r) {
		PedClumpRingLine("free", r, -1);
		r->state = 0;
		r->ped = nil;
		r->clump = nil;
	}
}
void
PedClumpModel(void *ped, int model)
{
	PedClumpRec *r = PedClumpByPed(ped);
	if (!r)
		return;
	r->model = model;
	if (model >= 0 && model < MODELINFOSIZE) {
		const char *mn = CModelInfo::GetModelInfo(model)->GetModelName();
		if (mn)
			snprintf(r->modelName, sizeof r->modelName, "%.23s", mn);
	}
	PedClumpRingLine("model", r, -1);
}
void
PedClumpBind(void *ped, void *clump)
{
	PedClumpRec *r = PedClumpByPed(ped);
	if (!r)
		return;
	r->clump = clump;
	r->state = 2;
	PedClumpRingLine("bind", r, -1);
}
void
PedClumpUnbind(void *ped)
{
	PedClumpRec *r = PedClumpByPed(ped);
	if (!r)
		return;
	PedClumpRingLine("unbind", r, -1);
	r->clump = nil;
	if (r->state == 2)
		r->state = 1;
}
void
PedClumpPreDelete(void *ped, void *clump)
{
	PedClumpRec *r = PedClumpByPed(ped);
	if (!r)
		return;
	if (clump != nil && r->clump != nil && clump != r->clump)
		PedClumpRingLine("predel_clump_mismatch", r, -1);
	else
		PedClumpRingLine("predel", r, -1);
}
void
PedClumpSweep(int phase)
{
	if (!g_pedClumpTable)
		return;
	int bound = 0;
	for (int i = 0; i < g_pedClumpCount; i++) {
		PedClumpRec *r = &g_pedClumpTable[i];
		if (r->state != 2 || r->clump == nil)
			continue;
		bound++;
		RpClump *c = (RpClump *)r->clump;
		if (!PedClumpInHeap(c, (int32)sizeof(RpClump))) {
			PedClumpFatalDump(r, "clump_fuera_heap", phase);
			continue;
		}
		rw::LLLink *nw = c->cameras.link.next;
		rw::LLLink *pv = c->cameras.link.prev;
		if (nw == nil || pv == nil || nw != pv || nw != &c->cameras.link) {
			PedClumpFatalDump(r, "cameras_listas_corruptas", phase);
			continue;
		}
		r->lastPhase = phase;
		r->lastGoodPhase = phase;
	}
	uint32 now = (uint32)emscripten_get_now();
	if (now >= g_pedClumpSummaryMs) {
		g_pedClumpSummaryMs = now + 5000u;
		char t[192];
		snprintf(t, sizeof t, "PEDCLUMP schema=1 summary bound=%d ringN=%d ringOvf=%d bad=%d epoch=%llu",
		         bound, g_pedClumpRingN, g_pedClumpRingOvf, g_pedClumpFirstBadSet ? 1 : 0, g_pedClumpEpoch);
		ODTRACES(t);
	}
}
#endif
