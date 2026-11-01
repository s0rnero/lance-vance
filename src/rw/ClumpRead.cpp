#include "common.h"

struct rpGeometryList
{
	RpGeometry **geometries;
	int32 numGeoms;
};

struct rpAtomicBinary
{
	RwInt32 frameIndex;
	RwInt32 geomIndex;
	RwInt32 flags;
	RwInt32 unused;
};

static int32 numberGeometrys;
static int32 streamPosition;
static rpGeometryList gGeomList;
static rwFrameList gFrameList;
static RpClumpChunkInfo gClumpInfo;

// 30/10 (30.0/T67): traza unica cuando la segunda fase del clump no cuadra.
// Sin ella "FAILED" no dice ni donde se quedo el stream ni cuantas geometrias
// esperaba, y no hay forma de saber si el fallo es del fichero o del salto a
// `streamPosition`.
static void
OdClumpFail(const char *what, RwStream *stream)
{
	static int reported = 0;
	if(reported >= 8)
		return;
	reported++;
	printf("[rw] clump fase2: fallo en %s pos=%u base=%d numGeoms=%d leidas=%d atomics=%d\n",
	       what, (uint32)STREAMPOS(stream), streamPosition,
	       gGeomList.numGeoms, numberGeometrys, gClumpInfo.numAtomics);
}

rpGeometryList*
GeometryListStreamRead1(RwStream *stream, rpGeometryList *geomlist)
{
	int i;
	RwUInt32 size, version;
	RwInt32 numGeoms;

	numberGeometrys = 0;
	if(!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
		return nil;
	assert(size == 4);
	if(RwStreamRead(stream, &numGeoms, 4) != 4)
		return nil;

	numberGeometrys = numGeoms/2;
	geomlist->numGeoms = numGeoms;
	if(geomlist->numGeoms > 0){
		geomlist->geometries = (RpGeometry**)RwMalloc(geomlist->numGeoms * sizeof(RpGeometry*));
		if(geomlist->geometries == nil)
			return nil;
		memset(geomlist->geometries, 0, geomlist->numGeoms * sizeof(RpGeometry*));
	}else
		geomlist->geometries = nil;

	for(i = 0; i < numberGeometrys; i++){
		if(!RwStreamFindChunk(stream, rwID_GEOMETRY, nil, &version))
			return nil;
		geomlist->geometries[i] = RpGeometryStreamRead(stream);
		if(geomlist->geometries[i] == nil)
			return nil;
	}

	return geomlist;
}

rpGeometryList*
GeometryListStreamRead2(RwStream *stream, rpGeometryList *geomlist)
{
	int i;
	RwUInt32 version;

	for(i = numberGeometrys; i < geomlist->numGeoms; i++){
		if(!RwStreamFindChunk(stream, rwID_GEOMETRY, nil, &version)){
			OdClumpFail("find-geometry", stream);
			return nil;
		}
		geomlist->geometries[i] = RpGeometryStreamRead(stream);
		if(geomlist->geometries[i] == nil){
			OdClumpFail("read-geometry", stream);
			return nil;
		}
	}

	return geomlist;
}

void
GeometryListDeinitialize(rpGeometryList *geomlist)
{
	int i;

	for(i = 0; i < geomlist->numGeoms; i++)
		if(geomlist->geometries[i])
			RpGeometryDestroy(geomlist->geometries[i]);

	if(geomlist->numGeoms){
		RwFree(geomlist->geometries);
		geomlist->numGeoms = 0;
	}
}

RpAtomic*
ClumpAtomicStreamRead(RwStream *stream, rwFrameList *frmList, rpGeometryList *geomList)
{
	RwUInt32 size, version;
	rpAtomicBinary a;
	RpAtomic *atomic;

	numberGeometrys = 0;
	if(!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
		return nil;
	assert(size <= sizeof(rpAtomicBinary));
	if(RwStreamRead(stream, &a, size) != size)
		return nil;

	atomic = RpAtomicCreate();
	if(atomic == nil)
		return nil;

	RpAtomicSetFlags(atomic, a.flags);

	if(frmList->numFrames){
		assert(a.frameIndex < frmList->numFrames);
		RpAtomicSetFrame(atomic, frmList->frames[a.frameIndex]);
	}

	if(geomList->numGeoms){
		assert(a.geomIndex < geomList->numGeoms);
		RpAtomicSetGeometry(atomic, geomList->geometries[a.geomIndex], 0);
	}else{
		RpGeometry *geom;
		if(!RwStreamFindChunk(stream, rwID_GEOMETRY, nil, &version)){
			RpAtomicDestroy(atomic);
			return nil;
		}
		geom = RpGeometryStreamRead(stream);
		if(geom == nil){
			RpAtomicDestroy(atomic);
			return nil;
		}
		RpAtomicSetGeometry(atomic, geom, 0);
		RpGeometryDestroy(geom);
	}

	return atomic;
}

bool
RpClumpGtaStreamRead1(RwStream *stream)
{
	RwUInt32 size, version;

	if(!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
		return false;
	if(version >= 0x33000){
		assert(size == 12);
		if(RwStreamRead(stream, &gClumpInfo, 12) != 12)
			return false;
	}else{
		assert(size == 4);
		if(RwStreamRead(stream, &gClumpInfo, 4) != 4)
			return false;
	}

	if(!RwStreamFindChunk(stream, rwID_FRAMELIST, nil, &version))
		return false;
	if(rwFrameListStreamRead(stream, &gFrameList) == nil)
		return false;

	if(!RwStreamFindChunk(stream, rwID_GEOMETRYLIST, nil, &version)){
		rwFrameListDeinitialize(&gFrameList);
		return false;
	}
	if(GeometryListStreamRead1(stream, &gGeomList) == nil){
		rwFrameListDeinitialize(&gFrameList);
		return false;
	}
	streamPosition = STREAMPOS(stream);
	return true;
}

RpClump*
RpClumpGtaStreamRead2(RwStream *stream)
{
	int i;
	RwUInt32 version;
	RpAtomic *atomic;
	RpClump *clump;

	clump = RpClumpCreate();
	if(clump == nil)
		return nil;

	// 30/10 (30.0/T68): el salto de la fase 2 es relativo y se calcula con la
	// posicion actual; si no cae justo en la geometria `numberGeometrys` el
	// `findChunk` siguiente lee un header basura y el stream acaba en EOF.
	{
		static int traced = 0;
		if(traced < 10){
			traced++;
			printf("[rw] fase2 skip: base=%d pos=%u numGeoms=%d\n",
			       streamPosition, (uint32)STREAMPOS(stream), gGeomList.numGeoms);
		}
	}
	RwStreamSkip(stream, streamPosition - STREAMPOS(stream));
	{
		static int traced = 0;
		if(traced < 10){
			traced++;
			printf("[rw] fase2 pos tras skip=%u\n", (uint32)STREAMPOS(stream));
		}
	}

	if(GeometryListStreamRead2(stream, &gGeomList) == nil){
		OdClumpFail("geomlist", stream);
		GeometryListDeinitialize(&gGeomList);
		rwFrameListDeinitialize(&gFrameList);
		RpClumpDestroy(clump);
		return nil;
	}

	RpClumpSetFrame(clump, gFrameList.frames[0]);

	for(i = 0; i < gClumpInfo.numAtomics; i++){
		if(!RwStreamFindChunk(stream, rwID_ATOMIC, nil, &version)){
			OdClumpFail("find-atomic", stream);
			GeometryListDeinitialize(&gGeomList);
			rwFrameListDeinitialize(&gFrameList);
			RpClumpDestroy(clump);
			return nil;
		}

		atomic = ClumpAtomicStreamRead(stream, &gFrameList, &gGeomList);
		if(atomic == nil){
			OdClumpFail("read-atomic", stream);
			GeometryListDeinitialize(&gGeomList);
			rwFrameListDeinitialize(&gFrameList);
			RpClumpDestroy(clump);
			return nil;
		}

		RpClumpAddAtomic(clump, atomic);
	}

	GeometryListDeinitialize(&gGeomList);
	rwFrameListDeinitialize(&gFrameList);
	return clump;
}

void
RpClumpGtaCancelStream(void)
{
	GeometryListDeinitialize(&gGeomList);
	rwFrameListDeinitialize(&gFrameList);
	gFrameList.numFrames = 0;
}
