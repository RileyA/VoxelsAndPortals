#include "Portal.h"
#include "OryxEngine.h"
#include "OgreSubsystem/OgreSubsystem.h"

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

	// ndoe the camera will be attached to
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

	// orient the meshes
	setPosition(mPosition);
	setDirection(out, up);
	
	// set render queue stuff (needed for stencil shenanigans)
	mMesh->setRenderQueueGroup(mBlue ? 75 : 76);

	// set appropriate border color
	mBorder->setMaterialName(mBlue ? "PortalBorderBlue" : "PortalBorderOrange");
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
	// set the pointer and attach the camera node to it's mesh)
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
}
//---------------------------------------------------------------------------

void Portal::setDirection(Vector3 d, Vector3 up)
{
	mDirection = d;
	Quaternion q;
	q.fromAxes(up.crossProduct(d), up, d);
	mMesh->setOrientation(q);
	mBorder->setOrientation(q);
}
//---------------------------------------------------------------------------

void Portal::setDirection(BlockDirection out, BlockDirection up)
{
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

