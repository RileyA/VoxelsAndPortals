#include "Common.h"
#include "BasicChunk.h"
#include "BasicChunkGenerator.h"
#include "ChunkUtils.h"

BasicChunk::BasicChunk(BasicChunkGenerator* gen, InterChunkCoords pos)
	:Chunk(gen, pos),mActive(0),mBasicGenerator(gen)
{
	for(int i = 0; i < 6; ++i)
		neighbors[i] = 0;
}
//---------------------------------------------------------------------------

BasicChunk::~BasicChunk()
{
}
//---------------------------------------------------------------------------

void BasicChunk::build(bool full)
{
	boost::mutex::scoped_lock lock(mBlockMutex);

	// we're always doing at least lighting
	mMesh->diffuse.clear();

	// clear the rest for a full build
	if(full)
	{
		mMesh->vertices.clear();
		mMesh->texcoords.clear();
		mMesh->normals.clear();
		mMesh->indices.clear();
		mMesh->addTexcoordSet();
	}

	// loop through each block
	for(int i=0;i<CHUNK_SIZE_X;++i)
		for(int j=0;j<CHUNK_SIZE_Y;++j)
			for(int k=0;k<CHUNK_SIZE_Z;++k)
	{
		ChunkCoords bc(i,j,k);
		byte type = blocks[i][j][k];
		Vector3 realPos = Vector3(i,j,k) - OFFSET;

		// if this is a hollow block
		if(blockTransparent(type))
		{
			// just ignore this horrendous mess (it's faster than the cleaner original version...)
			bool adjacents[6] = { 
				(i>0&&!blockSolid(blocks[i-1][j][k])) || (i==0&&!neighbors[0]) ||(i==0&&neighbors[0]
					&&!blockSolid(neighbors[0]->blocks[CHUNK_SIZE_X-1][j][k])),
				(i<CHUNK_SIZE_X-1&&!blockSolid(blocks[i+1][j][k])) || (i==CHUNK_SIZE_X-1 &&!neighbors[1])||
					(i==CHUNK_SIZE_X-1&&neighbors[1] && !blockSolid(neighbors[1]->blocks[0][j][k])),
				(j>0&&!blockSolid(blocks[i][j-1][k])) || (j==0&&!neighbors[2]) ||
					(j==0&&neighbors[2]&&!blockSolid(neighbors[2]->blocks[i][CHUNK_SIZE_Y-1][k])),
				(j<CHUNK_SIZE_Y-1&&!blockSolid(blocks[i][j+1][k])) || (j==CHUNK_SIZE_Y-1&&!neighbors[3])||
					(j==CHUNK_SIZE_Y-1&&neighbors[3]&&!blockSolid(neighbors[3]->blocks[i][0][k])),
				(k>0&&!blockSolid(blocks[i][j][k-1])) || (k==0&&!neighbors[4]) ||
					(k==0&&neighbors[4]&&!blockSolid(neighbors[4]->blocks[i][j][CHUNK_SIZE_Z-1])),
				(k<CHUNK_SIZE_Z-1&&!blockSolid(blocks[i][j][k+1])) || (k==CHUNK_SIZE_Z-1&&!neighbors[5])||
					(k==CHUNK_SIZE_Z-1&&neighbors[5]&&!blockSolid(neighbors[5]->blocks[i][j][0]))};

			// if it's a closed off cube, there's no need 
			// (the player shouldn't ever end up in a 1x1x1 hole)
			if(!adjacents[0] && !adjacents[1] && !adjacents[2] &&
				!adjacents[3] && !adjacents[4] && !adjacents[5])
				continue;

			// determine lighting conditions at this point
			Real lighting = LIGHTVALUES[light[i][j][k]];

			// determine lighting conditions in surrounding blocks,
			// this is used for smooth lighting calculations
			float surroundingLights[6] = {0.f,0.f,0.f,0.f,0.f,0.f};

			for(int p = 0; p < 6; ++p)
			{
				// only check if there isn't a block in this direction
				if(adjacents[p])
				{
					surroundingLights[p] = LIGHTVALUES[getLightAt(this, bc<<p)];
				}
			}

			// create the quads
			for(int p = 0; p < 6; ++p)
			{
				// only create a quad if there's a block adjacent in that direction
				if(!adjacents[p])
				{
					makeQuad(bc, realPos, p, MAPPINGS[getBlockAt(this, bc << p)][5 - p],
						lighting, adjacents, surroundingLights, !full);
				}
			}
		}
	}
}
//---------------------------------------------------------------------------

void BasicChunk::clearLighting()
{
	boost::mutex::scoped_lock lock(mLightMutex);
	memset(light, 0, CHUNK_VOLUME);
}
//---------------------------------------------------------------------------

void BasicChunk::calculateLighting(const std::map<BasicChunk*, bool>& chunks, bool secondaryLight)
{
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

void BasicChunk::changeBlock(const ChunkCoords& chng)
{
	// note that we carefully scope this, otherwise deadlocks will
	// occur between this and the 'apply' step of the generator thread
	{
		boost::mutex::scoped_lock lock(mBlockMutex);
		mChanges.push_back(chng);
	}
	mBasicGenerator->notifyChunkChange(this);
}
//---------------------------------------------------------------------------

bool BasicChunk::activate()
{
	boost::mutex::scoped_lock lock(mBlockMutex);
	bool wasActive = true;
	if(!mActive)
	{
		wasActive = false;
	}
	mActive = true;
	return !wasActive;
}
//---------------------------------------------------------------------------

void BasicChunk::deactivate()
{
	boost::mutex::scoped_lock lock(mBlockMutex);
	// TODO: kill gfx mesh
	mActive = false;
}
//---------------------------------------------------------------------------

bool BasicChunk::isActive()
{
	boost::mutex::scoped_lock lock(mBlockMutex);
	return mActive;
}
//---------------------------------------------------------------------------

void BasicChunk::makeQuad(
	const ChunkCoords& chunkPos,
	const Vector3& realPos,
	const unsigned int& direction,
	const byte& blockType,
	const Real& lighting,
	const bool* adjacentBlocks,
	const Real* adjacentLighting,
	bool lightOnly)
{
	if(!lightOnly)
	{
		// Texture atlas grid size (hardcoded for 16x16 atm, for Minecraft textures)
		int atlasDimensions = 16;
		float gridSize = 1.f / atlasDimensions;
	
		// texcoord offset for this block type
		Vector2 offset = Vector2(
			(blockType - 1) % atlasDimensions * gridSize, 
			(blockType - 1) / atlasDimensions * gridSize);

		// loop through for each vert
		for(int i=0;i<6;++i)
		{
			// add the position and texcoords to teh meshdata
			mMesh->vertex(realPos + BLOCK_VERTICES[direction][5-i],
				offset+BLOCK_TEXCOORDS[direction][5-i]*gridSize);
		}
	}

	// base light level, with some shading based on direction of the face
	Real baseLight = lighting * LIGHT_STEPS[5-direction];

	// TODO only calc for 4 verts, reuse vals for overlapped verts
	for(int i = 0; i < 6; ++i)
	{
		// light at this vertex
		float light = baseLight;

		// add normals, if set (sorta pointless unless we're doing shader stuff
		// that needs 'em...)
		#ifdef CHUNK_NORMALS
		d.normals.push_back(BLOCK_NORMALS[direction].x);
		d.normals.push_back(BLOCK_NORMALS[direction].y);
		d.normals.push_back(BLOCK_NORMALS[direction].z);
		#endif

		// this might be better setup as a toggle, rather than determining at compile time...
		#ifdef SMOOTH_LIGHTING

		// check diagonal to this vert and add it if need be
		if(adjacentBlocks[LIGHTING_COORDS[direction][FILTERVERTEX[i]][0]] ||
			adjacentBlocks[LIGHTING_COORDS[direction][FILTERVERTEX[i]][1]])
		{
			ChunkCoords cc = chunkPos<<LIGHTING_COORDS[direction][FILTERVERTEX[i]][0];
			cc = cc<<LIGHTING_COORDS[direction][FILTERVERTEX[i]][1];
			light += LIGHTVALUES[getLightAt(this,cc)];	
		}

		// add two adjacent blocks
		light += adjacentLighting[LIGHTING_COORDS[direction][FILTERVERTEX[i]][0]]
			+ adjacentLighting[LIGHTING_COORDS[direction][FILTERVERTEX[i]][1]];

		// average
		light /= 4.f;
 
		#endif

		// apply to each channel (with some slight offsets to give the light a bit of color)
		mMesh->diffuse.push_back(light);
		mMesh->diffuse.push_back(light / 1.2f);
		mMesh->diffuse.push_back(light / 1.4f);
		mMesh->diffuse.push_back(1.f);// alpha's just always 1
	}
}
//---------------------------------------------------------------------------

void BasicChunk::doLighting(const std::map<BasicChunk*, bool>& chunks,
	ChunkCoords& coords, byte lightVal, bool emitter)
{
	int8 trans = 0;

	// only proceed if this is an emitter, OR a transparent block with
	// a lesser light value than lightVal
	if(emitter || (lightVal > 0 && (trans = getTransparency(
		blocks[coords.x][coords.y][coords.z])) &&
		(setLight(coords,lightVal))))
	{
		// loop through each direction
		for(int i = BD_LEFT; i <= BD_BACK; ++i)
		{
			// hold onto the original value, so we can backtrack
			ChunkCoords old = coords;

			// nudge in the proper direction
			coords[AXIS[i]] += AXIS_OFFSET[i];

			// since it may cross into other chunks
			BasicChunk* tmp = this;

			// check for out of bounds
			if(coords[AXIS[i]] < 0)
			{
				tmp = neighbors[i];
				coords[AXIS[i]] = CHUNKSIZE[AXIS[i]] - 1;
			}
			else if(coords[AXIS[i]] >= CHUNKSIZE[AXIS[i]])
			{
				tmp = neighbors[i];
				coords[AXIS[i]] = 0;
			}

			// continue (if on an edge, then only do so if it exists in the map)
			if(lightVal - trans > 0 && tmp && (tmp == this || chunks.find(tmp) != chunks.end()))
			{
				tmp->doLighting(chunks, coords, lightVal - trans, false);
			}

			// reset coords
			coords = old;
		}
	}
}
//---------------------------------------------------------------------------
