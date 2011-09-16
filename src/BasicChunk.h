#ifndef	BasicChunk_H
#define BasicChunk_H

#include "OryxMeshData.h"
#include "Chunk.h"
#include "ChunkOptions.h"

class BasicChunkGenerator;

/** A simple chunk */
class BasicChunk : public Chunk
{
public:

	BasicChunk(BasicChunkGenerator* gen, InterChunkCoords pos);
	virtual ~BasicChunk();

	/** Does either a full build or lighting only
	 *		@param full Whether or not to do a full build or just 
	 *			do lighting */
	void build(bool full);

	/** Blacks out all lighting, used prior to a lighting update */
	void clearLighting();

	/** Does full lighting, and spreads light into any chunks in the set, if secondaryLight then
	 *		also gather light from neighbors not in the set
	 *	@param chunks The map of chunks that are getting updated (light can spread into
	 *		and out of any of these during this update) 
	 *	@param secondaryLight Whether to gather secondary lighting from surrounding chunks
	 *		(that aren't included in 'chunks') */
	void calculateLighting(const std::map<BasicChunk*, bool>& chunks, bool secondaryLight);

	/** Change the value of a block
	 *	@param chng the change to make (use the ChunkCoords 'data'
	 *		field to specify what to change to)
	 *	@remarks This will not be instantaneous it will get updated when
	 *		the background thread gets to it... */
	void changeBlock(const ChunkCoords& chng);

	/** Attempt to set a light value (it will only do so if it is brighter
	 *		what is already there) and return whether or not it worked.
	 *	@param coords the coordinates at which to set the light
	 *	@param val The light value to try 
	 *	@return If it replaced the light value*/
	inline bool setLight(ChunkCoords& coords, byte val)
	{
		boost::mutex::scoped_lock lock(mLightMutex);

		if(light[coords.c.x][coords.c.y][coords.c.z] >= val)
		{
			return false;
		}
		else
		{
			light[coords.c.x][coords.c.y][coords.c.z] = val;
			return true;
		}
	}

	/** Gets the lighting at a point
	 *	@param x The x coordinate 
	 *	@param y The y coordinate 
	 *	@param z The z coordinate */
	inline byte getLightAt(int8 x, int8 y, int8 z)
	{
		boost::mutex::scoped_lock lock(mLightMutex);
		return light[x][y][z];
	}

	/** Gets the lighting at a point
	 *	@param c The coordinates */
	inline byte getLightAt(const ChunkCoords& c)
	{
		boost::mutex::scoped_lock lock(mLightMutex);
		return light[c.c.x][c.c.y][c.c.z];
	}

	/** Gets the lighting at a point
	 *	@param x The x coordinate 
	 *	@param y The y coordinate 
	 *	@param z The z coordinate */
	inline byte getBlockAt(int8 x, int8 y, int8 z)
	{
		boost::mutex::scoped_lock lock(mBlockMutex);
		return blocks[x][y][z];
	}

	/** Gets the lighting at a point
	 *	@param c The coordinates */
	inline byte getBlockAt(const ChunkCoords& c)
	{
		boost::mutex::scoped_lock lock(mBlockMutex);
		return blocks[c.c.x][c.c.y][c.c.z];
	}

	/** Sets this chunk as 'active' (actively being rendered/simulated) */
	bool activate();

	/** Sets this chunk as 'inactive' and halts any existing graphics/physics simulation. */
	void deactivate();

	/** Gets whether or not this is active */
	bool isActive();

	/** So the chunk generator can get at this thing's data more easily */
	friend class BasicChunkGenerator;

	// TODO: protected this...
	BasicChunk* neighbors[6];

private:

	/** Calculates lighting by recursively tracing through the chunks */
	void doLighting(const std::map<BasicChunk*, bool>& chunks, 
		ChunkCoords& coords, byte lightVal, bool emitter);

	/** Helper that creates a quad */
	void makeQuad(ChunkCoords& cpos,Vector3 pos,int normal,MeshData& d,
		short type,float diffuse,bool* adj,byte* lights, bool full);

	//---------------------------------------------------------------------------

	static inline BasicChunk* correctCoords(BasicChunk* c, ChunkCoords& coords)
	{
		if(!c)
			return c;
		for(int i = 0; i < 3; ++i)
		{
			if(coords[i] < 0)
			{
				coords[i] += CHUNKSIZE[i];
				return correctCoords(c->neighbors[i*2],coords);	
			}
			else if(coords[i] > CHUNKSIZE[i]-1) 
			{
				coords[i] -= CHUNKSIZE[i];
				return correctCoords(c->neighbors[i*2+1],coords);
			}
		}
		return c;
	}
	//---------------------------------------------------------------------------

	inline static byte getBlockAt(BasicChunk* c, ChunkCoords coords)
	{
		c = correctCoords(c,coords);
		return c ? c->blocks[coords.c.x][coords.c.y][coords.c.z] : 0;
	}	
	//---------------------------------------------------------------------------

	inline static byte getLightAt(BasicChunk* c, ChunkCoords coords)
	{
		c = correctCoords(c,coords);
		return c ? c->light[coords.c.x][coords.c.y][coords.c.z] : 0;
	}	
	//---------------------------------------------------------------------------

	// mutex to protect lighting data
	boost::mutex mLightMutex;

	// lighting data
	byte light[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];

	// the block data itself
	byte blocks[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];

	// pending changes
	std::list<ChunkCoords> mChanges;

	// light emitting blocks
	std::list<ChunkCoords> lights;
	// TODO: something for portals and associated light changes...
	
	// whether or not this chunk is active
	bool mActive;

	// the generator that created this
	BasicChunkGenerator* mBasicGenerator;

};

#endif
