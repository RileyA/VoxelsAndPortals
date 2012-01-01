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

	//void inverseDoLighting(ChunkCoords coords, byte lightVal, byte prevLightVal, std::set<ChunkCoords>& newLights);
	void inverseDoLighting(ChunkCoords coords, ChunkCoords clamped, byte lightVal, 
		byte prevLightVal, std::set<ChunkCoords>& newLights);
	void doInvLighting(ChunkCoords coords, byte lightVal, 
		byte prevLightVal, byte dirMask = 0xFF);
};

#endif
