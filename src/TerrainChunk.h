#ifndef	TerrainChunk_H
#define TerrainChunk_H

#include "Common.h"
#include "BasicChunk.h"

class TerrainChunk : public BasicChunk
{
public:
	TerrainChunk(BasicChunkGenerator* gen, InterChunkCoords pos);
	virtual ~TerrainChunk();
	virtual void calculateLighting(const std::map<BasicChunk*, 
		bool>& chunks, bool secondaryLight);
};

#endif
