#include "Portal.h"
#include "OryxEngine.h"
#include "OgreSubsystem/OgreSubsystem.h"
#include "ChunkUtils.h"

Quaternion getRotationTo(const Vector3& orig, 
	const Vector3& dest, const Vector3& fallback, bool& id, bool& fall);

Portal::Portal(Vector3 pos, BlockDirection out, BlockDirection up, bool blue)
	:mBlue(blue), mSibling(0), mPosition(pos)
{
	mOgre = Engine::getPtr()->getSubsystem("OgreSubsystem")->
		castType<OgreSubsystem>();

	// setup camera
	mCamera = mOgre->createCamera();
	mCamera->setPosition(Vector3(0,0,0));
	mCamera->setDirection(Vector3(0,0,-1));

	// copy aspect ratio and FOV of main camera
	mCamera->setFOV(mOgre->getActiveCamera()->getFOV());
	mCamera->setAspectRatio(mOgre->getActiveCamera()->getAspectRatio());
	mCamera->setFarClip(120.f);
	mCamera->setNearClip(0.001f);

	// node the camera will be attached to
	mNode = mOgre->createSceneNode();
	mNode->setPosition(Vector3::ZERO);
	mNode->setOrientation(Quaternion::IDENTITY);

	// yaw 180 degrees (instead of mirroring)
	mNode->yaw(180.f);
	mNode->addChild(mCamera);

	// create mask mesh
	mMesh = mOgre->createMesh("Portal.mesh");
	mOgre->getRootSceneNode()->addChild(mMesh);

	// create decorative border mesh
	mBorder = mOgre->createMesh("PortalBorder.mesh");
	mOgre->getRootSceneNode()->addChild(mBorder);

	MeshData d;

	// manually build collision mesh
	d.vertex(Vector3(0.45,0.95,0));
	d.vertex(Vector3(-0.45,0.95,0));
	d.vertex(Vector3(-0.45,-0.95,0));
	d.vertex(Vector3(0.45,0.95,0));
	d.vertex(Vector3(0.45,-0.95,0));
	d.vertex(Vector3(-0.45,-0.95,0));

	mCollision = static_cast<CollisionObject*>(
		dynamic_cast<BulletSubsystem*>(Engine::getPtr()->getSubsystem("BulletSubsystem"))
		->createStaticTrimesh(d, Vector3(0,0,0)));
	mCollision->setCollisionGroup(COLLISION_GROUP_11);
	mCollision->setCollisionMask(65535);
	mCollision->setUserData(this);

	// orient the meshes
	setPosition(mPosition);
	setDirection(out, up);
	
	// set render queue stuff (needed for stencil shenanigans)
	mMesh->setRenderQueueGroup(mBlue ? 75 : 76);

	// set appropriate border color
	mBorder->setMaterialName(mBlue ? "PortalBorderBlue" : "PortalBorderOrange");

	mChunks[0] = 0;
	mChunks[1] = 0;
}
//---------------------------------------------------------------------------

Portal::~Portal()
{
	// clean up all the gfx stuffs
	mOgre->destroySceneNode(mCamera);
	mOgre->destroySceneNode(mNode);
	mOgre->destroySceneNode(mMesh);
	mOgre->destroySceneNode(mBorder);
}
//---------------------------------------------------------------------------

void Portal::update(Real delta)
{
	if(mSibling)
	{
		// transform world space player camera pos/orientation into this portal's 
		// coordinate frame (and since the camera is attached to the sibling portal
		// it will be positioned properly for portal rendering)
		mCamera->setPosition(mMesh->worldToLocalPosition(
			mOgre->getActiveCamera()->getAbsolutePosition()));
		mCamera->setOrientation(mMesh->worldToLocalOrientation(
			mOgre->getActiveCamera()->getCameraAbsoluteOrientation()));
		mCamera->setCustomNearClipPlane(Plane(mSibling->mDirection, mSibling->mDirection.dotProduct(
			mSibling->mMesh->getPosition())));
	}
}
//---------------------------------------------------------------------------

void Portal::setSibling(Portal* p)
{	
	// set the pointer and attach the camera node to its mesh)
	mSibling = p;
	mSibling->mMesh->addChild(mNode);
}
//---------------------------------------------------------------------------

void Portal::setVisible(bool v)
{
	mMesh->setVisible(v);
	mBorder->setVisible(v);
}
//---------------------------------------------------------------------------

void Portal::setPosition(Vector3 p)
{
	mMesh->setPosition(p);
	mBorder->setPosition(p);
	mCollision->setPosition(p);
}
//---------------------------------------------------------------------------

void Portal::setDirection(Vector3 d, Vector3 up)
{
	mDirection = d;
	Quaternion q;
	q.fromAxes(up.crossProduct(d), up, d);
	mMesh->setOrientation(q);
	mBorder->setOrientation(q);
	mCollision->setOrientation(q);
}
//---------------------------------------------------------------------------

void Portal::setDirection(BlockDirection out, BlockDirection up)
{
	mDir = out;
	setDirection(BLOCK_NORMALS[out], BLOCK_NORMALS[up]);
}
//---------------------------------------------------------------------------

Camera* Portal::getCamera()
{
	return mCamera;
}
//---------------------------------------------------------------------------

void Portal::recurse()
{
	// take the current position and transform it again (this is necessary for 
	// the portal-in-portal-in-portal-... effect to work properly)
	mCamera->setPosition(mMesh->worldToLocalPosition(
		mCamera->getAbsolutePosition()));
	mCamera->setOrientation(mMesh->worldToLocalOrientation(
		mCamera->getAbsoluteOrientation()));
}
//---------------------------------------------------------------------------

void Portal::place(const RaycastReport& r)
{
	if(r.hit && r.userData && r.group != COLLISION_GROUP_11)
	{
		BasicChunk* chunk = static_cast<BasicChunk*>(r.userData);
		ChunkCoords coords = getBlockFromRaycast(r.position, r.normal, chunk, false);

		chunk = correctChunkCoords(chunk, coords);

		BlockDirection d = getBlockDirectionFromVector(r.normal);
		BlockDirection d_inverse = getBlockDirectionFromVector(r.normal * -1);
		BlockDirection up = getBlockDirectionFromVector(
			Plane(r.normal, 0).projectVector(mOgre->getActiveCamera()->getAbsoluteUp()));

		ChunkCoords coords2 = coords << up;
		BasicChunk* chunk2 = correctChunkCoords(chunk, coords2);

		if(!getBlockVal(chunk, coords) && !getBlockVal(chunk2, coords2) 
			&& getBlockVal(chunk,  coords  << d_inverse) 
			&& getBlockVal(chunk2, coords2 << d_inverse))
		{
			Vector3 adjPos = chunk->getPosition() - OFFSET + Vector3(
				coords.x, coords.y, coords.z);
			adjPos -= BLOCK_NORMALS[d] * 0.5f;
			adjPos += BLOCK_NORMALS[up] * 0.5f;

			if(mSibling && mSibling->isEnabled())
			{
				BlockDirection d_back = static_cast<BlockDirection>(AXIS_INVERT[d]);
				BlockDirection sd_back = static_cast<BlockDirection>(AXIS_INVERT[mSibling->mDir]);
				if( coords  << d_back == mSibling->getCoords(0) << sd_back || 
					coords2 << d_back == mSibling->getCoords(1) << sd_back ||
					coords2 << d_back == mSibling->getCoords(0) << sd_back ||
					coords << d_back  == mSibling->getCoords(1) << sd_back)
					return;
			}

			mNode->setVisible(true);
			mMesh->setVisible(true);
			mBorder->setVisible(true);
			//mCollision->addToSimulation();
			mEnabled = true;

			mCoords[0] = coords;
			mCoords[1] = coords2;

			mChunks[0] = chunk;
			mChunks[1] = chunk2;
			chunk->needsRelight();

			if(chunk != chunk2)
				chunk2->needsRelight();

			setPosition(adjPos);
			setDirection(d, up);
		}
	}
}
//---------------------------------------------------------------------------

void Portal::disable()
{
	if(mEnabled)
	{
		if(mChunks[0])
			mChunks[0]->needsRelight();
		if(mChunks[1] && mChunks[1] != mChunks[0])
			mChunks[1]->needsRelight();
		mChunks[0] = 0;
		mChunks[1] = 0;

		mEnabled = false;
		mNode->setVisible(false);
		mMesh->setVisible(false);
		mBorder->setVisible(false);

		mCollision->setPosition(Vector3(0,1000,0));//removeFromSimulation();
	}
}
//---------------------------------------------------------------------------

bool Portal::inPortal(BasicChunk* ch, ChunkCoords c, bool behind)
{
	if(!mEnabled)
		return false;
	if(behind)
	{
		ChunkCoords fixedCoords[2] = {mCoords[0] << AXIS_INVERT[mDir], 
			mCoords[1] << AXIS_INVERT[mDir]};
		Chunk* fixedChunks[2] = {correctChunkCoords(mChunks[0], fixedCoords[0]),
			correctChunkCoords(mChunks[1], fixedCoords[1])};
		return (ch == fixedChunks[0] && c == fixedCoords[0]) ||
			(ch == fixedChunks[1] && c == fixedCoords[1]);
	}
	else
	{
		return (ch == mChunks[0] && c == mCoords[0]) ||
			(ch == mChunks[1] && c == mCoords[1]);
	}
}
//---------------------------------------------------------------------------

