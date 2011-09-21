#include "TerrainChunk.h"

TerrainChunk::TerrainChunk(BasicChunkGenerator* gen, InterChunkCoords pos)
	:BasicChunk(gen, pos)
{

}
//---------------------------------------------------------------------------

TerrainChunk::~TerrainChunk()
{

}
//---------------------------------------------------------------------------

void TerrainChunk::calculateLighting(const std::map<BasicChunk*, bool>& chunks, bool secondaryLight)
{
	// sunlight! (well, sky more like I suppose, since it comes from straight up...)
	// super inefficient and hacky (this whole thing wasn't really built for Minecraft-style terrain and
	// lighting and so forth...)
	byte heights[CHUNK_SIZE_X][CHUNK_SIZE_Z];

	for(int i = 0; i < CHUNK_SIZE_X; ++i)
		for(int j = 0; j < CHUNK_SIZE_Z; ++j)
	{
		for(int y = CHUNK_SIZE_Y - 1; y >= 0; --y)
		{
			heights[i][j] = 0;
			if(!blocks[i][y][j])
			{
				ChunkCoords c(i,y,j);
				setLight(c, 15);
			}
			else
			{
				heights[i][j] = y+1;
				break;
			}
		}
	}

	for(int i = 0; i < CHUNK_SIZE_X; ++i)
		for(int j = 0; j < CHUNK_SIZE_Z; ++j)
	{
		for(int y = CHUNK_SIZE_Y - 1; y >= heights[i][j] && y >= 0; --y)
		{
			ChunkCoords c(i,y,j);
			doLighting(chunks, c, 15, true);
		}
	}

	if(secondaryLight)
	{
		// loop through neighboring chunks, spreading light
		for(int p=0;p<6;++p)
		{
			if(neighbors[p] && chunks.find(neighbors[p]) == chunks.end())
			{
				byte normal = AXIS[p];
				byte aX = p > 1 ? 0 : 1;
				byte aY = p < 4 ? 2 : 1;

				byte d1 = (p&1)==0 ? CHUNKSIZE[normal]-1 : 0;
				byte d2 = (p&1)==0 ? 0 : CHUNKSIZE[normal]-1;
				
				ChunkCoords coords = ChunkCoords(0,0,0);
				coords[normal] = d1;
				
				for(coords[aX]=0;coords[aX]<CHUNKSIZE[aX];++coords[aX])
					for(coords[aY]=0;coords[aY]<CHUNKSIZE[aY];++coords[aY])
				{
					byte value = neighbors[p]->light[coords[0]][coords[1]][coords[2]];
					if(value>1)
					{
						coords[normal] = d2;
						doLighting(chunks, coords, value - 1, false);
						coords[normal] = d1;
					}
				}
			}
		}
	}

	for(std::list<ChunkCoords>::iterator it = lights.begin(); it != lights.end(); ++it)
	{
		doLighting(chunks, (*it), (*it).data, true);
	}
}
//---------------------------------------------------------------------------
