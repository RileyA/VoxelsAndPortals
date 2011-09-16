#include "Common.h"
#include "BasicChunk.h"
#include "ChunkOptions.h"
#include "ChunkCoords.h"


inline static bool blockSolid(byte block)
{
	return BLOCKTYPES[block] & BP_SOLID;
}
//---------------------------------------------------------------------------

inline static bool blockTransparent(byte block)
{
	return BLOCKTYPES[block] & BP_TRANSPARENT;
}
//---------------------------------------------------------------------------

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
//---------------------------------------------------------------------------

inline static byte getBlockVal(BasicChunk* c, ChunkCoords coords)
{
	c = correctChunkCoords(c,coords);
	return c ? c->getBlockAt(coords) : 0;
}	
//---------------------------------------------------------------------------

inline static byte getLightVal(BasicChunk* c, ChunkCoords coords)
{
	c = correctChunkCoords(c,coords);
	return c ? c->getLightAt(coords) : 0;
}	
//---------------------------------------------------------------------------

inline static byte getTransparency(byte block)
{
	return (BLOCKTYPES[block] & BP_TRANSPARENT) ? (BLOCKTYPES[block] & 15) + 1 : 0;
}
//---------------------------------------------------------------------------

inline static byte getCoordTransparency(BasicChunk* ch, ChunkCoords& c)
{
	return getTransparency(ch->getBlockAt(c));
}
//---------------------------------------------------------------------------

inline static ChunkCoords getBlockFromRaycast(Vector3 pos, Vector3 normal, BasicChunk* c, bool edge)
{
	pos += OFFSET;
	pos -= c->getPosition();

	Vector3 pn = edge ? pos - normal * 0.5f : pos + normal * 0.5f;

	int i = floor(pn.x+0.5);
	int j = floor(pn.y+0.5);
	int k = floor(pn.z+0.5);

	return ChunkCoords(i,j,k);
}
//---------------------------------------------------------------------------
