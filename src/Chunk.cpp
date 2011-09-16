#include "Common.h"
#include "Chunk.h"
#include "ChunkGenerator.h"

Chunk::Chunk(ChunkGenerator* gen, InterChunkCoords position)
	:mGenerator(gen)
	,mMesh(0)
	,mPosition(position)
	,mGfxMesh(0)
	,mPhysicsMesh(0)
{
	mMesh = new MeshData();
	mGfx = Engine::getPtr()->getSubsystem("OgreSubsystem")->
		castType<OgreSubsystem>();
	mPhysics = Engine::getPtr()->getSubsystem("BulletSubsystem")->
		castType<BulletSubsystem>();
}
//---------------------------------------------------------------------------

Chunk::~Chunk()
{
	if(mPhysicsMesh)
		mPhysicsMesh->_kill();
	if(mGfxMesh)
		mGfx->destroySceneNode(mGfxMesh);
	delete mMesh;
}
//---------------------------------------------------------------------------

void Chunk::update(bool full)
{
	// lock 
	boost::mutex::scoped_lock lock(mBlockMutex);

	// if empty, then kill physics/gfx if they exist
	if(mMesh->vertices.empty())
	{
		if(mPhysicsMesh)
			mPhysicsMesh->_kill();
		mPhysicsMesh = 0;
		if(mGfxMesh)
			mGfx->destroySceneNode(mGfxMesh);
		mGfxMesh = 0;
	}
	else if(!mGfxMesh || !mPhysicsMesh)
	{
		// create graphics mesh
		mGfxMesh = mGfx->createMesh(*mMesh);
		mGfx->getRootSceneNode()->addChild(mGfxMesh);
		mGfxMesh->setPosition(getPosition());

		// create physics mesh
		mPhysicsMesh = static_cast<CollisionObject*>(mPhysics->createStaticTrimesh(*mMesh, getPosition()));
		mPhysicsMesh->setUserData(this);
		mPhysicsMesh->setCollisionGroup(COLLISION_GROUP_2);
		mPhysicsMesh->setCollisionMask(COLLISION_GROUP_15|COLLISION_GROUP_1|COLLISION_GROUP_3);
	}
	else
	{
		if(full)
		{
			// update gfx mesh
			mGfxMesh->update(*mMesh);

			// kill and remake physics mesh
			if(mPhysicsMesh){mPhysicsMesh->_kill();}
			mPhysicsMesh = static_cast<CollisionObject*>(mPhysics->createStaticTrimesh(*mMesh, getPosition()));
			mPhysicsMesh->setUserData(this);
			mPhysicsMesh->setCollisionGroup(COLLISION_GROUP_2);
			mPhysicsMesh->setCollisionMask(COLLISION_GROUP_15|COLLISION_GROUP_1|COLLISION_GROUP_3);
		}
		else if(mMesh->diffuse.size() > 0)
		{
			// lighting only, no need to touch physics or anything..
			mGfxMesh->updateDiffuse(*mMesh);
		}
	}
}
//---------------------------------------------------------------------------
