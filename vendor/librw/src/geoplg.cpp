#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "rwbase.h"
#include "rwerror.h"
#include "rwplg.h"
#include "rwpipeline.h"
#include "rwobjects.h"
#include "rwengine.h"
#include "rwanim.h"
#include "rwplugins.h"
#include "ps2/rwps2.h"
#include "ps2/rwps2plg.h"
#include "d3d/rwxbox.h"
#include "d3d/rwd3d8.h"
#include "d3d/rwd3d9.h"
#include "gl/rwwdgl.h"
#include "gl/rwgl3.h"

#define PLUGIN_ID 2

namespace rw {

static uint16 nextSerialNum = 1;

// Mesh

// Allocate a mesh header, meshes and optionally indices.
// If existing meshes already exist, retain their information.
MeshHeader*
Geometry::allocateMeshes(int32 numMeshes, uint32 numIndices, bool32 noIndices)
{
	uint32 sz;
	MeshHeader *mh;
	Mesh *m;
	uint16 *indices;
	int32 oldNumMeshes;
	int32 i;
	if(numMeshes < 0 || numMeshes > 100000 || numIndices > 4000000){
		printf("[rw] MESHALLOC rechazado meshes=%d indices=%u\n", numMeshes, (uint32)numIndices);
		numMeshes = 0;
		numIndices = 0;
	}
	sz = sizeof(MeshHeader) + numMeshes*sizeof(Mesh);
	if(!noIndices)
		sz += numIndices*sizeof(uint16);
	if(this->meshHeader){
		oldNumMeshes = this->meshHeader->numMeshes;
		mh = (MeshHeader*)rwResize(this->meshHeader, sz, MEMDUR_EVENT | ID_GEOMETRY);
		this->meshHeader = mh;
	}else{
		oldNumMeshes = 0;
		mh = (MeshHeader*)rwNew(sz, MEMDUR_EVENT | ID_GEOMETRY);
		mh->flags = 0;
		this->meshHeader = mh;
	}
	mh->numMeshes = numMeshes;
	mh->serialNum = nextSerialNum++;
	mh->totalIndices = numIndices;
	m = mh->getMeshes();
	indices = (uint16*)&m[numMeshes];
	for(i = 0; i < mh->numMeshes; i++){
		// keep these
		if(i >= oldNumMeshes){
			m->material = nil;
			m->numIndices = 0;
		}
		// always init indices
		if(noIndices)
			m->indices = nil;
		else{
			m->indices = indices;
			indices += m->numIndices;
		}
		m++;
	}
	return mh;
}

void
MeshHeader::setupIndices(void)
{
	int32 i;
	uint16 *indices;
	Mesh *m;
	m = this->getMeshes();
	indices = m->indices;
	// return if native
	if(indices == nil)
		return;
	for(i = 0; i < this->numMeshes; i++){
		m->indices = indices;
		indices += m->numIndices;
		m++;
	}
}

struct MeshHeaderStream
{
	uint32 flags;
	uint32 numMeshes;
	uint32 totalIndices;
};

struct MeshStream
{
	uint32 numIndices;
	int32 matIndex;
};

static Stream*
readMesh(Stream *stream, int32 len, void *object, int32, int32)
{
	MeshHeaderStream mhs;
	MeshStream ms;
	MeshHeader *mh;
	Mesh *mesh;
	int32 indbuf[256];
	uint16 *indices;
	Geometry *geo = (Geometry*)object;

	stream->read32(&mhs, sizeof(MeshHeaderStream));
	{
		uint64 need = sizeof(MeshHeaderStream) + (uint64)mhs.numMeshes*sizeof(MeshStream)
		            + (uint64)mhs.totalIndices*4;
		if(mhs.numMeshes > 100000 || mhs.totalIndices > 4000000 ||
		   (!(geo->flags & Geometry::NATIVE) && len > (int32)sizeof(MeshHeaderStream) && need > (uint64)len)){
			printf("[rw] MESHBAD meshes=%u indices=%u len=%d need=%u\n",
			       (uint32)mhs.numMeshes, (uint32)mhs.totalIndices, len, (uint32)need);
			geo->meshHeader = nil;
			geo->allocateMeshes(0, 0, false);
			if(len > (int32)sizeof(MeshHeaderStream))
				stream->seek(len - sizeof(MeshHeaderStream));
			return stream;
		}
	}
	// Have to do this dance for War Drum's meshes
	bool32 hasData = len > int32(sizeof(MeshHeaderStream)+mhs.numMeshes*sizeof(MeshStream));
	uint32 indiceReal = mhs.totalIndices;
	bool porMalla = false;
	bool contiguo = false;
	bool usable = false;
	MeshStream *cabeceras = nil;
	if(!(geo->flags & Geometry::NATIVE) && mhs.numMeshes > 0 && mhs.numMeshes <= 100000){
		cabeceras = (MeshStream*)rwMalloc(mhs.numMeshes*sizeof(MeshStream), MEMDUR_GLOBAL);
		if(cabeceras){
			uint32 inicio = stream->tell();
			bool ok = true;
			uint32 sumaC = 0;
			for(uint32 i = 0; i < mhs.numMeshes; i++){
				if(stream->read32(&cabeceras[i], sizeof(MeshStream)) != sizeof(MeshStream)){
					ok = false;
					break;
				}
				if(cabeceras[i].matIndex < 0 || cabeceras[i].matIndex >= geo->matList.numMaterials){
					ok = false;
					break;
				}
				sumaC += cabeceras[i].numIndices;
				if(sumaC > 4000000){
					ok = false;
					break;
				}
			}
			if(ok && (int64)len == (int64)sizeof(MeshHeaderStream) + (int64)mhs.numMeshes*sizeof(MeshStream) + (int64)sumaC*4){
				contiguo = true;
				indiceReal = sumaC;
				usable = true;
			}
			stream->seek((int32)inicio - (int32)stream->tell());
			if(!contiguo){
				ok = true;
				uint32 suma = 0;
				for(uint32 i = 0; i < mhs.numMeshes; i++){
					MeshStream t;
					if(stream->read32(&t, sizeof(MeshStream)) != sizeof(MeshStream)){
						ok = false;
						break;
					}
					suma += t.numIndices;
					if(suma > 4000000 || t.numIndices > (uint32)len){
						ok = false;
						break;
					}
					stream->seek(t.numIndices*4);
				}
				stream->seek((int32)inicio - (int32)stream->tell());
				if(ok && (int64)len == (int64)sizeof(MeshHeaderStream) + (int64)mhs.numMeshes*sizeof(MeshStream) + (int64)suma*4){
					indiceReal = suma;
					usable = true;
				}
			}
		}
	}
	if(!(geo->flags & Geometry::NATIVE) && !usable){
		static int32 reportedFallback = 0;
		if(reportedFallback < 8){
			reportedFallback++;
			printf("[rw] MESHFALLBACK meshes=%u total=%u mat=%d\n",
			       (uint32)mhs.numMeshes, (uint32)mhs.totalIndices, geo->matList.numMaterials);
		}
		geo->meshHeader = nil;
		geo->allocateMeshes(0, 0, false);
		if(len > (int32)sizeof(MeshHeaderStream))
			stream->seek(len - sizeof(MeshHeaderStream));
		if(cabeceras)
			rwFree(cabeceras);
		return stream;
	}
	assert(geo->meshHeader == nil);
	geo->meshHeader = nil;
	mh = geo->allocateMeshes(mhs.numMeshes, indiceReal, 
		geo->flags & Geometry::NATIVE && !hasData);
	mh->flags = mhs.flags;

	mesh = mh->getMeshes();
	indices = mesh->indices;
	uint16 *indexBase = indices;
	if(contiguo)
		stream->seek(mhs.numMeshes*sizeof(MeshStream));
	for(uint32 i = 0; i < mh->numMeshes; i++){
		if(contiguo)
			ms = cabeceras[i];
		else
			stream->read32(&ms, sizeof(MeshStream));
		// 30/10 (30.0/T54): un DFF puede declarar en sus mallas mas indices que
		// `totalIndices`, y `allocateMeshes` reserva solo totalIndices. Antes
		// `indices += numIndices` se salia del buffer y corrompia el heap (el
		// trap salia frames despues, en malloc/CTxdStore). Se recorta la malla a
		// lo que queda del buffer y se sigue consumiendo del stream lo declarado
		// para no desalinear los plugins siguientes.
		int32 room = (int32)mh->totalIndices - (int32)(indices - indexBase);
		if(room < 0)
			room = 0;
		if(!porMalla && ms.numIndices > (uint32)room){
			static int32 reportedOverrun = 0;
			if(reportedOverrun < 8){
				reportedOverrun++;
				printf("[rw] MESHOVER total=%u malla=%u pide=%u queda=%d\n",
				       (uint32)mh->totalIndices, (uint32)i,
				       (uint32)ms.numIndices, room);
			}
		}
		mesh->numIndices = (!porMalla && ms.numIndices > (uint32)room) ? (uint32)room : ms.numIndices;
		// 26/10: DFF con `matIndex` fuera de la lista (editado/reconvertido) dejaba
		// aqui un puntero basura; se dibujaba y reventaba en setMaterial, matando el
		// frame y congelando el juego. Se cae al primer material (o a ninguno) y se
		// avisa una sola vez.
		if(ms.matIndex >= 0 && ms.matIndex < geo->matList.numMaterials)
			mesh->material = geo->matList.materials[ms.matIndex];
		else{
			static int32 reported = 0;
			if(reported < 8){
				reported++;
				printf("[rw] MESHBADMAT matIndex=%d mats=%d\n",
				       (int)ms.matIndex, (int)geo->matList.numMaterials);
			}
			mesh->material = geo->matList.numMaterials > 0 ? geo->matList.materials[0] : nil;
		}
		if(geo->flags & Geometry::NATIVE){
			// War Drum OpenGL stores uint16 indices here
			if(hasData){
				mesh->indices = indices;
				indices += mesh->numIndices;
				stream->read16(mesh->indices,
				            mesh->numIndices*2);
				if(ms.numIndices > mesh->numIndices)
					stream->seek((ms.numIndices - mesh->numIndices)*2);
			}
		}else{
			mesh->indices = indices;
			indices += mesh->numIndices;
			uint16 *ind = mesh->indices;
			int32 roomWrite = mesh->numIndices;
			int32 declarados = ms.numIndices;
			for(; declarados > 0; declarados -= 256){
				int32 n = declarados < 256 ? declarados : 256;
				stream->read32(indbuf, n*4);
				int32 w = n < roomWrite ? n : roomWrite;
				for(int32 j = 0; j < w; j++)
					ind[j] = indbuf[j];
				ind += w;
				roomWrite -= w;
			}
		}
		mesh++;
	}
	if(cabeceras)
		rwFree(cabeceras);
	return stream;
}

static Stream*
writeMesh(Stream *stream, int32, void *object, int32, int32)
{
	MeshHeaderStream mhs;
	MeshStream ms;
	int32 indbuf[256];
	Geometry *geo = (Geometry*)object;
	mhs.flags = geo->meshHeader->flags;
	mhs.numMeshes = geo->meshHeader->numMeshes;
	mhs.totalIndices = geo->meshHeader->totalIndices;
	stream->write32(&mhs, sizeof(MeshHeaderStream));
	Mesh *mesh = geo->meshHeader->getMeshes();
	for(uint32 i = 0; i < geo->meshHeader->numMeshes; i++){
		ms.numIndices = mesh->numIndices;
		ms.matIndex = geo->matList.findIndex(mesh->material);
		stream->write32(&ms, sizeof(MeshStream));
		if(geo->flags & Geometry::NATIVE){
			assert(geo->instData != nil);
			if(geo->instData->platform == PLATFORM_WDGL)
				stream->write16(mesh->indices,
				            mesh->numIndices*2);
		}else{
			uint16 *ind = mesh->indices;
			int32 numIndices = mesh->numIndices;
			for(; numIndices > 0; numIndices -= 256){
				int32 n = numIndices < 256 ? numIndices : 256;
				for(int32 j = 0; j < n; j++)
					indbuf[j] = ind[j];
				stream->write32(indbuf, n*4);
				ind += n;
			}
		}
		mesh++;
	}
	return stream;
}

static int32
getSizeMesh(void *object, int32, int32)
{
	Geometry *geo = (Geometry*)object;
	if(geo->meshHeader == nil)
		return -1;
	int32 size = 12 + geo->meshHeader->numMeshes*8;
	if(geo->flags & Geometry::NATIVE){
		assert(geo->instData != nil);
		if(geo->instData->platform == PLATFORM_WDGL)
			size += geo->meshHeader->totalIndices*2;
	}else{
		size += geo->meshHeader->totalIndices*4;
	}
	return size;
}

void
registerMeshPlugin(void)
{
	Geometry::registerPlugin(0, ID_MESH, nil, nil, nil);
	Geometry::registerPluginStream(ID_MESH, readMesh, writeMesh, getSizeMesh);
}

// Returns the maximum number of triangles. Just so
// we can allocate enough before instancing. This does not
// take into account degerate triangles or ADC bits as
// we don't look at the data.
uint32
MeshHeader::guessNumTriangles(void)
{
	if(this->flags == MeshHeader::TRISTRIP)
		return this->totalIndices - 2*this->numMeshes;
	else
		return this->totalIndices/3;
}

// Native Data

static void*
destroyNativeData(void *object, int32 offset, int32 size)
{
	Geometry *geometry = (Geometry*)object;
	if(geometry->instData == nil)
		return object;
	if(geometry->instData->platform == PLATFORM_PS2)
		return ps2::destroyNativeData(object, offset, size);
	if(geometry->instData->platform == PLATFORM_WDGL)
		return wdgl::destroyNativeData(object, offset, size);
	if(geometry->instData->platform == PLATFORM_XBOX)
		return xbox::destroyNativeData(object, offset, size);
	if(geometry->instData->platform == PLATFORM_D3D8)
		return d3d8::destroyNativeData(object, offset, size);
	if(geometry->instData->platform == PLATFORM_D3D9)
		return d3d9::destroyNativeData(object, offset, size);
	if(geometry->instData->platform == PLATFORM_GL3)
		return gl3::destroyNativeData(object, offset, size);
	return object;
}

static Stream*
readNativeData(Stream *stream, int32 len, void *object, int32 o, int32 s)
{
	ChunkHeaderInfo header;
	uint32 libid;
	uint32 platform;
	// ugly hack to find out platform
	stream->seek(-4);
	libid = stream->readU32();
	readChunkHeaderInfo(stream, &header);
	if(header.type == ID_STRUCT &&
	   libraryIDPack(header.version, header.build) == libid){
		platform = stream->readU32();
		stream->seek(-16);
		if(platform == PLATFORM_PS2)
			return ps2::readNativeData(stream, len, object, o, s);
		else if(platform == PLATFORM_XBOX)
			return xbox::readNativeData(stream, len, object, o, s);
		else if(platform == PLATFORM_D3D8)
			return d3d8::readNativeData(stream, len, object, o, s);
		else if(platform == PLATFORM_D3D9)
			return d3d9::readNativeData(stream, len, object, o, s);
		else{
			fprintf(stderr, "unknown platform %d\n", platform);
			stream->seek(len);
		}
	}else{
		stream->seek(-12);
		wdgl::readNativeData(stream, len, object, o, s);
	}
	return stream;
}

static Stream*
writeNativeData(Stream *stream, int32 len, void *object, int32 o, int32 s)
{
	Geometry *geometry = (Geometry*)object;
	if(geometry->instData == nil)
		return stream;
	if(geometry->instData->platform == PLATFORM_PS2)
		return ps2::writeNativeData(stream, len, object, o, s);
	else if(geometry->instData->platform == PLATFORM_WDGL)
		return wdgl::writeNativeData(stream, len, object, o, s);
	else if(geometry->instData->platform == PLATFORM_XBOX)
		return xbox::writeNativeData(stream, len, object, o, s);
	else if(geometry->instData->platform == PLATFORM_D3D8)
		return d3d8::writeNativeData(stream, len, object, o, s);
	else if(geometry->instData->platform == PLATFORM_D3D9)
		return d3d9::writeNativeData(stream, len, object, o, s);
	return stream;
}

static int32
getSizeNativeData(void *object, int32 offset, int32 size)
{
	Geometry *geometry = (Geometry*)object;
	if(geometry->instData == nil)
		return 0;
	if(geometry->instData->platform == PLATFORM_PS2)
		return ps2::getSizeNativeData(object, offset, size);
	else if(geometry->instData->platform == PLATFORM_WDGL)
		return wdgl::getSizeNativeData(object, offset, size);
	else if(geometry->instData->platform == PLATFORM_XBOX)
		return xbox::getSizeNativeData(object, offset, size);
	else if(geometry->instData->platform == PLATFORM_D3D8)
		return d3d8::getSizeNativeData(object, offset, size);
	else if(geometry->instData->platform == PLATFORM_D3D9)
		return d3d9::getSizeNativeData(object, offset, size);
	return 0;
}

void
registerNativeDataPlugin(void)
{
	Geometry::registerPlugin(0, ID_NATIVEDATA,
	                         nil, destroyNativeData, nil);
	Geometry::registerPluginStream(ID_NATIVEDATA,
	                               readNativeData,
	                               writeNativeData,
	                               getSizeNativeData);
}

}
