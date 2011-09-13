#include "Common.h"
#include "Chunk.h"
#include "ChunkGenerator.h"

Chunk::Chunk(ChunkGenerator* gen, InterChunkCoords pos)
	:mGenerator(gen),mMesh(0),position(pos),mGfxMesh(0)
{
	mMesh = new MeshData();
}

Chunk::~Chunk()
{
	delete mMesh;
}
