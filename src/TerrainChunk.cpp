#include "TerrainChunk.h"
#include "ChunkUtils.h"

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
	// This is sort of big and redundant at the moment... but considerably faster than the original
	for(std::map<ChunkCoords, ChunkChange>::iterator i = mConfirmedChanges.begin(); 
		i != mConfirmedChanges.end(); ++i)
	{
		ChunkCoords crds = i->first;
		ChunkChange chch = i->second;
		byte oldLight    = light[crds.x][crds.y][crds.z];
		byte origBlock   = chch.origBlock;
		byte newBlock    = crds.data;

		// NOTE we assume wasAir && isAir cannot both be true
		bool wasAir         = origBlock == 0;
		bool isAir          = newBlock  == 0;
		bool wasTransparent = BLOCKTYPES[origBlock] & BP_TRANSPARENT;
		bool isTransparent  = BLOCKTYPES[newBlock ] & BP_TRANSPARENT;
		bool wasEmitter     = BLOCKTYPES[origBlock] & BP_EMISSIVE;
		bool isEmitter      = BLOCKTYPES[newBlock ] & BP_EMISSIVE;

		byte oldSecondary = BLOCKTYPES[origBlock] & 0x0F;
		byte newSecondary = BLOCKTYPES[newBlock] & 0x0F;
		if(wasTransparent) oldSecondary++;
		if(isTransparent)  newSecondary++;

		// CASE: was sunlit air
		// the new addition is blocking sunlight now
		if(wasAir && oldLight == 15)
		{
			lightDirty = true;

			// zero out light
			int y = crds.y - 1;
			for(; y >= 0 && !blocks[crds.x][y][crds.z]; --y)
				light[crds.x][y][crds.z] = 0;

			byte lv = 0;

			// if transparent or emitter, set light value accordingly
			if(isTransparent)
				lv = 15 - newSecondary;
			else if(isEmitter)
				lv = newSecondary;
			else
				lv = 0;

			light[crds.x][crds.y][crds.z] = lv;
			doInvLighting(crds, lv, oldLight, 0x33);

			// now do 'inverse' lighting (only on XZ plane) to correct invalidated light
			for(y = y + 1; y < crds.y; ++y)
				doInvLighting(ChunkCoords(crds.x, y, crds.z), 0, 15, 0x33);
		}
		// CASE: was blocking sunlight
		// now light will potentially propagate down
		else if((crds.y == CHUNK_SIZE_Y - 1) 
			|| (blocks[crds.x][crds.y+1][crds.z] == 0 && light[crds.x][crds.y+1][crds.z] == 15))
		{
			// CASE: air light will propagate freely
			if(isAir)
			{
				int y = crds.y;
				// loop down setting light vals
				for(; y >= 0 && !blocks[crds.x][y][crds.z]; --y)
					light[crds.x][y][crds.z] = 15;
				// propagate normally
				for(y = y + 1; y <= crds.y; ++y)
					doLighting(ChunkCoords(crds.x, y, crds.z), 15, true);
				lightDirty = true;
			}
			// CASE: not air, sunlight will not propagate down
			else
			{
				byte oldEmission = 0;
				if(wasTransparent)	oldEmission = 15 - oldSecondary;
				else if(wasEmitter) oldEmission = oldSecondary;

				byte newEmission = 0;
				if(isTransparent)  newEmission = 15 - newSecondary;
				else if(isEmitter) newEmission = newSecondary;

				if(newEmission == oldEmission)
				{
					// do nothing! lighting contribution is the same!
				}
				else if(newEmission > oldEmission)
				{
					// we're emitting more light, just propagate as usual
					light[crds.x][crds.y][crds.z] = newEmission;
					doLighting(crds, newEmission, true);
					lightDirty = true;
				}
				else
				{
					// we're emitting less light, so do the 'inverse' lighting algo
					light[crds.x][crds.y][crds.z] = newEmission;
					doInvLighting(crds, newEmission, oldEmission);
					lightDirty = true;
				}
			}
		}
		// CASE: Nothing to do with sunlight
		else
		{
			int8 oldEmission = 0;
			int8 newEmission = 0;

			// CASE: was solid block
			if(!wasTransparent && !wasEmitter)
			{
				oldEmission = 0;
				// will be the emitted value
				if(isEmitter) newEmission = newSecondary;
				// we don't know... it'll need to do inverse
				else if(isTransparent) newEmission = -1;
			}
			// CASE: was emissive block
			else if(wasEmitter)
			{
				oldEmission = oldSecondary;
				// will be the emitted value
				if(isEmitter) newEmission = newSecondary;
				// we don't know... it'll need to do inverse
				else if(isTransparent) newEmission = -1;
			}
			// CASE: was transparent block
			else
			{
				// light value
				oldEmission = oldLight;

				// will be the emitted value
				if(isEmitter) newEmission = newSecondary;
				// we know it isn't emitting, so we know the value right away
				else if(isTransparent) oldEmission + oldSecondary - newSecondary;
			}

			// Now that we have light values...
			
			// CASE: Increased light value
			if(newEmission > oldEmission)
			{
				// just set and propagate
				light[crds.x][crds.y][crds.z] = newEmission;
				doLighting(crds, newEmission, true);
				lightDirty = true;
			}
			else if(newEmission == oldEmission)
			{
				// no change needed
			}
			// CASE: Decreased light value
			else
			{
				// do inverse lighting
				newEmission = std::max((int)newEmission, 0);
				light[crds.x][crds.y][crds.z] = newEmission;
				doInvLighting(crds, newEmission, oldEmission);
				lightDirty = true;
			}
		}
	}
}
//---------------------------------------------------------------------------

void TerrainChunk::doInvLighting(ChunkCoords coords, byte lightVal, 
	byte prevLightVal, byte dirMask)
{
	std::set<ChunkCoords> newLights;
	coords.data = 0;
	ChunkCoords clamped = coords;

	// first do inverse step to clean up any invalidated light vals
	for(int i = BD_BACK; i >= 0; --i)
	{
		if(!(dirMask & (1 << i)))
			continue;

		// hold onto the original value, so we can backtrack
		ChunkCoords old = clamped;

		// nudge in the proper direction
		coords[AXIS[i]] += AXIS_OFFSET[i];
		clamped[AXIS[i]] += AXIS_OFFSET[i];

		// since it may cross into other chunks
		BasicChunk* tmp = this;

		// check for out of bounds
		if(clamped[AXIS[i]] < 0)
		{
			tmp = neighbors[i];
			clamped[AXIS[i]] = CHUNKSIZE[AXIS[i]] - 1;
		}
		else if(clamped[AXIS[i]] >= CHUNKSIZE[AXIS[i]])
		{
			tmp = neighbors[i];
			clamped[AXIS[i]] = 0;
		}

		// continue (if on an edge, then only do so if it exists in the map)
		if(tmp)
		{
			static_cast<TerrainChunk*>(tmp)->inverseDoLighting(coords, clamped, lightVal, prevLightVal, newLights);
		}

		// reset coords
		clamped = old;
		coords[AXIS[i]] -= AXIS_OFFSET[i];
	}

	for(std::set<ChunkCoords>::iterator it = newLights.begin(); 
		it != newLights.end(); ++it)
	{
		ChunkCoords temp = *it;
		BasicChunk* tempc = this;
		tempc = correctChunkCoords(tempc, temp);
		tempc->doLighting(temp, it->data, true);
	}
}
//---------------------------------------------------------------------------

void TerrainChunk::inverseDoLighting(ChunkCoords coords, ChunkCoords clamped, byte lightVal, 
	byte prevLightVal, std::set<ChunkCoords>& newLights)
{
	byte type = blocks[clamped.x][clamped.y][clamped.z];
	byte oldLight = light[clamped.x][clamped.y][clamped.z];

	int8 trans = getTransparency(type);

	// keep overwriting...
	if(oldLight < prevLightVal && trans)
	{
		int8 newLight = std::max(0, (int8)lightVal - trans);
		light[clamped.x][clamped.y][clamped.z] = newLight;
		lightDirty = true;

		// we know this can't be an exterior source now
		std::set<ChunkCoords>::iterator it = newLights.find(coords);
		if(it != newLights.end())
			newLights.erase(it);
			//const_cast<ChunkCoords&>(*it).data = 50;

		for(int i = BD_BACK; i >= 0; --i)
		{
			// hold onto the original value, so we can backtrack
			ChunkCoords old = clamped;

			// nudge in the proper direction
			coords[AXIS[i]] += AXIS_OFFSET[i];
			clamped[AXIS[i]] += AXIS_OFFSET[i];

			// since it may cross into other chunks
			BasicChunk* tmp = this;

			// check for out of bounds
			if(clamped[AXIS[i]] < 0)
			{
				tmp = neighbors[i];
				clamped[AXIS[i]] = CHUNKSIZE[AXIS[i]] - 1;
			}
			else if(clamped[AXIS[i]] >= CHUNKSIZE[AXIS[i]])
			{
				tmp = neighbors[i];
				clamped[AXIS[i]] = 0;
			}

			// continue (if on an edge, then only do so if it exists in the map)
			if(tmp)
			{
				static_cast<TerrainChunk*>(tmp)->inverseDoLighting(coords, clamped, newLight, oldLight, newLights);
			}

			// reset coords
			clamped = old;
			coords[AXIS[i]] -= AXIS_OFFSET[i];
		}

	}
	// we've reached an edge, this might be an exterior source, or it may be the
	// removed light looping back on itself
	else
	{
		coords.data = oldLight;
		newLights.insert(coords);
	}
}
//---------------------------------------------------------------------------

