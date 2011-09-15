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

Chunk::~Chunk()
{
	if(mPhysicsMesh)
		mPhysicsMesh->_kill();
	if(mGfxMesh)
		mGfx->destroySceneNode(mGfxMesh);
	delete mMesh;
}
