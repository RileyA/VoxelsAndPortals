#include "Common.h"
#include "BasicChunk.h"
#include "ChunkOptions.h"
#include "ChunkCoords.h"


inline static bool blockSolid(byte block)
{
	return BLOCKTYPES[block] & BP_SOLID;
}

inline static bool blockTransparent(byte block)
{
	return BLOCKTYPES[block] & BP_TRANSPARENT;
}

static inline BasicChunk* correctChunkCoords(BasicChunk* c, ChunkCoords& coords)
{
	if(!c)
		return c;
	for(int i = 0; i < 3; ++i)
	{
		if(coords[i] < 0)
		{
			coords[i] += CHUNKSIZE[i];
			return correctChunkCoords(c->neighbors[i*2],coords);	
		}
		else if(coords[i] > CHUNKSIZE[i]-1) 
		{
			coords[i] -= CHUNKSIZE[i];
			return correctChunkCoords(c->neighbors[i*2+1],coords);
		}
	}
	return c;
}

inline static byte getBlockVal(BasicChunk* c, ChunkCoords coords)
{
	c = correctChunkCoords(c,coords);
	return c ? c->blocks[coords.c.x][coords.c.y][coords.c.z] : 0;
}	

inline static byte getLightVal(BasicChunk* c, ChunkCoords coords)
{
	c = correctChunkCoords(c,coords);
	return c ? c->light[coords.c.x][coords.c.y][coords.c.z] : 0;
}	

inline static byte getTransparency(byte block)
{
	return (BLOCKTYPES[block] & BP_TRANSPARENT) ? (BLOCKTYPES[block] & 15) + 1 : 0;
}

inline static byte getCoordTransparency(BasicChunk* ch, ChunkCoords& c)
{
	return getTransparency(ch->blocks[c.c.x][c.c.y][c.c.z]);
}
