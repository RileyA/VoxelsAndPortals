#ifndef	BasicChunk_H
#define BasicChunk_H

#include "OryxMeshData.h"
#include "Chunk.h"
#include "ChunkOptions.h"

class BasicChunkGenerator;

class BasicChunk : public Chunk
{
public:

	BasicChunk(BasicChunkGenerator* gen, InterChunkCoords pos);
	virtual ~BasicChunk();

	void build(bool full);


	/** Does full lighting, and spreads light into any chunks in the set, if secondaryLight then
	 *		also gather light from neighbors not in the set
	 *	@param chunks The map of chunks that are getting updated (light can spread into
	 *		and out of any of these during this update) 
	 *	@param secondaryLight Whether to gather secondary lighting from surrounding chunks
	 *		(that aren't included in 'chunks') */
	void calculateLighting(const std::map<BasicChunk*, bool>& chunks, bool secondaryLight);

	/** Blacks out all lighting, used prior to a lighting update */
	void clearLighting()
	{
		boost::mutex::scoped_lock lock(mLightMutex);
		memset(light, 0, CHUNK_VOLUME);
	}

	/** Sets this chunk as 'active' (actively being rendered/simulated) */
	bool activate()
	{
		boost::mutex::scoped_lock lock(mBlockMutex);
		bool wasActive = true;
		if(!mActive)
		{
			mNewlyActive = true;
			wasActive = false;
		}
		mActive = true;
		return !wasActive;
	}

	/** Sets this chunk as 'inactive' and halts any existing graphics/physics simulation. */
	void deactivate()
	{
		boost::mutex::scoped_lock lock(mBlockMutex);
		// TODO: kill gfx mesh
		mActive = false;
	}

	/** Gets whether or not this is active */
	bool isActive()
	{
		boost::mutex::scoped_lock lock(mBlockMutex);
		return mActive;
	}

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
			//mLightDirty = true;
			return true;
		}
	}

	byte getLightAt(int8 x, int8 y, int8 z)
	{
		boost::mutex::scoped_lock lock(mLightMutex);
		return light[x][y][z];
	}

	byte getLightAt(const ChunkCoords& c)
	{
		boost::mutex::scoped_lock lock(mLightMutex);
		return light[c.c.x][c.c.y][c.c.z];
	}

	byte getBlockAt(int8 x, int8 y, int8 z)
	{
		boost::mutex::scoped_lock lock(mBlockMutex);
		return blocks[x][y][z];
	}

	byte getBlockAt(const ChunkCoords& c)
	{
		boost::mutex::scoped_lock lock(mBlockMutex);
		return blocks[c.c.x][c.c.y][c.c.z];
	}

	void changeBlock(const ChunkCoords& chng);

	void makeQuad(ChunkCoords& cpos,Vector3 pos,int normal,MeshData& d,
		short type,float diffuse,bool* adj,byte* lights, bool full);

	void buildMesh(bool full)
	{
		// lock 
		boost::mutex::scoped_lock lock(mBlockMutex);

		if(!mGfxMesh && mMesh->vertices.size() > 0)
		{
			// create graphics mesh
			OgreSubsystem* ogre = Engine::getPtr()->getSubsystem("OgreSubsystem")->castType<OgreSubsystem>();
			mGfxMesh = ogre->createMesh(*mMesh);
			ogre->getRootSceneNode()->addChild(mGfxMesh);
			Vector3 pos = Vector3(mPosition.x*CHUNK_SIZE_X, mPosition.y*CHUNK_SIZE_Y, mPosition.z * 
				CHUNK_SIZE_Z);
			mGfxMesh->setPosition(pos);

			// create physics mesh
			BulletSubsystem* b = Engine::getPtr()->getSubsystem("BulletSubsystem")->
				castType<BulletSubsystem>();
			if(mPhysicsMesh)
				mPhysicsMesh->_kill();
			mPhysicsMesh = static_cast<CollisionObject*>(b->createStaticTrimesh(*mMesh, pos));
			mPhysicsMesh->setUserData(this);
			mPhysicsMesh->setCollisionGroup(COLLISION_GROUP_2);
			mPhysicsMesh->setCollisionMask(COLLISION_GROUP_15|COLLISION_GROUP_1|COLLISION_GROUP_3);
		}
		else if(mGfxMesh)
		{
			if(full)
			{
				if(mMesh->vertices.size() > 0)
				{
					// update gfx mesh
					mGfxMesh->update(*mMesh);
					BulletSubsystem* b = Engine::getPtr()->getSubsystem("BulletSubsystem")->
						castType<BulletSubsystem>();
					if(mPhysicsMesh)
						mPhysicsMesh->_kill();
					Vector3 pos = Vector3(mPosition.x*CHUNK_SIZE_X, mPosition.y*CHUNK_SIZE_Y, mPosition.z * 
						CHUNK_SIZE_Z);
					mPhysicsMesh = static_cast<CollisionObject*>(b->createStaticTrimesh(*mMesh, pos));
					mPhysicsMesh->setUserData(this);
					mPhysicsMesh->setCollisionGroup(COLLISION_GROUP_2);
					mPhysicsMesh->setCollisionMask(COLLISION_GROUP_15|COLLISION_GROUP_1|COLLISION_GROUP_3);
				}
				else
				{
					if(mPhysicsMesh)
					{
						mPhysicsMesh->_kill();
						mPhysicsMesh = 0;
					}

					OgreSubsystem* ogre = Engine::getPtr()->getSubsystem("OgreSubsystem")->castType<OgreSubsystem>();
					ogre->destroySceneNode(mGfxMesh);
					mGfxMesh = 0;
				}
			}
			else if(mMesh->diffuse.size() > 0)
			{
				mGfxMesh->updateDiffuse(*mMesh);// lighting only
			}
		}
	}

	static ChunkCoords getBlockFromRaycast(Vector3 pos, Vector3 normal, BasicChunk* c, bool edge)
	{
		pos += OFFSET;
		pos -= c->getPosition();

		Vector3 pn = edge ? pos - normal * 0.5f : pos + normal * 0.5f;

		int i = floor(pn.x+0.5);
		int j = floor(pn.y+0.5);
		int k = floor(pn.z+0.5);

		return ChunkCoords(i,j,k);
	}

	BasicChunk* neighbors[6];

	boost::mutex mLightMutex;
	bool mLightDirty;
	byte light[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];

	boost::mutex mBlockMutex;
	byte blocks[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];
	std::list<ChunkCoords> mChanges;
	std::list<ChunkCoords> lights;// light emitting blocks
	
	// TODO: something for portals and associated light changes...
	bool mActive;
	bool mNewlyActive;

	BasicChunkGenerator* mBasicGenerator;

private:

	void doLighting(const std::map<BasicChunk*, bool>& chunks, 
		ChunkCoords& coords, byte lightVal, bool emitter);

};

#endif
