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

	/** DEPRECATED */
	void calculatePrimaryLighting();
	/** DEPRECATED */
	void calculateSecondaryLighting();

	void getLighting(ChunkCoords& coords, byte lightVal, bool emitter);

	void doLighting(const std::map<BasicChunk*, bool>& chunks, ChunkCoords& coords, byte lightVal, bool emitter);

	/** Does full lighting, and spreads light into any chunks in the set, if secondaryLight then
	 *	also gather light from neighbors not in the set */
	void calculateLighting(const std::map<BasicChunk*, bool>& chunks, bool secondaryLight);

	/** Apply pending edits to the chunk */
	bool applyChanges();

	void clearLighting()
	{
		boost::mutex::scoped_lock lock(mLightMutex);
		memset(light, 0, CHUNK_VOLUME);
	}

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

	void deactivate()
	{
		boost::mutex::scoped_lock lock(mBlockMutex);
		// TODO: kill gfx mesh
		mActive = false;
	}

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
		short type,float diffuse,bool* adj,byte* lights);
	void makeQuadFull(ChunkCoords& cpos,Vector3 pos,int normal,MeshData& d,
		short type,float diffuse,bool* adj,byte* lights);

	void buildMesh(bool full)
	{
		// lock 
		boost::mutex::scoped_lock lock(mBlockMutex);

		/*if(!mMesh)
		{
			return;
		}
		else
		{
			if(mMesh->vertices.size() > 0 && full)
			{
				std::cout<<"verts: "<<mMesh->vertices.size()<<"\n";
				std::cout<<"texsets: "<<mMesh->texcoords.size()<<"\n";
				std::cout<<"tex: "<<mMesh->texcoords[0].size()<<"\n";
			}
		}*/

		//full = true;

		if(!mGfxMesh && mMesh->vertices.size() > 0)
		{
			// create graphics mesh
			OgreSubsystem* ogre = Engine::getPtr()->getSubsystem("OgreSubsystem")->castType<OgreSubsystem>();
			mGfxMesh = ogre->createMesh(*mMesh);
			ogre->getRootSceneNode()->addChild(mGfxMesh);
			Vector3 pos = Vector3(position.x*CHUNK_SIZE_X, position.y*CHUNK_SIZE_Y, position.z * 
				CHUNK_SIZE_Z);
			mGfxMesh->setPosition(pos);

			// create physics mesh
			BulletSubsystem* b = Engine::getPtr()->getSubsystem("BulletSubsystem")->
				castType<BulletSubsystem>();
			if(block)
				block->_kill();
			block = static_cast<CollisionObject*>(b->createStaticTrimesh(*mMesh, pos));
			block->setUserData(this);
			block->setCollisionGroup(COLLISION_GROUP_2);
			block->setCollisionMask(COLLISION_GROUP_15|COLLISION_GROUP_1|COLLISION_GROUP_3);
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
					if(block)
						block->_kill();
					Vector3 pos = Vector3(position.x*CHUNK_SIZE_X, position.y*CHUNK_SIZE_Y, position.z * 
						CHUNK_SIZE_Z);
					block = static_cast<CollisionObject*>(b->createStaticTrimesh(*mMesh, pos));
					block->setUserData(this);
					block->setCollisionGroup(COLLISION_GROUP_2);
					block->setCollisionMask(COLLISION_GROUP_15|COLLISION_GROUP_1|COLLISION_GROUP_3);
				}
				else
				{
					if(block)
					{
						block->_kill();
						block = 0;
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

		//delete mMesh;// the mesh is old news now, no need to hold onto it
		//mMesh = 0;
		//delete lock;
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

	CollisionObject* block;

	BasicChunkGenerator* mBasicGenerator;
};

#endif
