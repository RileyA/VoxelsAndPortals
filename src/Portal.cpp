#include "Portal.h"
#include "OryxEngine.h"
#include "OgreSubsystem/OgreSubsystem.h"

Portal::Portal(Vector3 pos, Vector3 direction, bool blue)
{
	mOgre = Engine::getPtr()->getSubsystem("OgreSubsystem")->castType<OgreSubsystem>();
	mCamera = mOgre->createCamera();
	mCamera->setPosition(Vector3(0,0,0));// it'll move into place once it has a siblingA
	mCamera->setDirection(Vector3(0,0,1));
	mCamera->setFOV(70.f);
	if(!blue)
	{
		mOgre->createRTT("blargh2", mCamera, 1024, 1024);
	}
	else
	{
		mOgre->createRTT("blargh", mCamera, 1024, 1024);
	}
	mCamera->setAspectRatio(1.3333f);
	mDirection = direction;
	mNode = mOgre->createSceneNode();
	mNode->addChild(mCamera);
	//mNode->yaw(180.f);
	mMesh = mOgre->createMesh("Portal.mesh");
	mMesh->setPosition(pos);
	mMesh->setOrientation(Vector3::UNIT_Z.getRotationTo(mDirection));

	if(!blue)
		mMesh->setMaterialName("Portal2");

	mOgre->getRootSceneNode()->addChild(mMesh);
	mSibling = 0;
	b = blue;
}

Portal::~Portal()
{

}

void Portal::update(Real delta)
{
	if(mSibling)
	{


		// transform camera pos/direction relative to the 1st portal into the 2nd portal's coordinate space

		// get observer position
		Camera* mainCam = mOgre->getActiveCamera();
		Vector3 mpos = mainCam->getAbsolutePosition();
		Vector3 mdir = mainCam->getAbsoluteDirection();

		// for convenience
		Vector3 portal2 = mMesh->getPosition();
		Vector3 portal1 = mSibling->mMesh->getPosition();

		Quaternion portal1Rotation = Vector3::UNIT_Z.getRotationTo(mDirection);
		// dir * -1, since we're looking through the back of the portal
		Quaternion portal2Rotation = Vector3::UNIT_Z.getRotationTo(mSibling->mDirection * -1);

		// observer position in portal1's coordinate space
		Vector3 portalSpacePos = mpos - portal1;
		// rotate in
		portalSpacePos = portal1Rotation.inverse() * portalSpacePos;

		Vector3 portalSpaceDir = portal1Rotation.inverse() * mdir;

		// now apply portal2 transforms to the parent node
		mNode->setPosition(portal2);
		mNode->setOrientation(portal2Rotation);

		mCamera->setPosition(portalSpacePos);
		mCamera->setDirection(portalSpaceDir);

		/*Camera* mainCam = mOgre->getActiveCamera();
		Vector3 mpos = mainCam->getAbsolutePosition();
		Vector3 mdir = mainCam->getAbsoluteDirection();

		//Quaternion rotate = mDirection.getRotationTo(mSibling->mDirection * -1);

		//Quaternion rotate = 

		Quaternion rotate = Vector3::NEGATIVE_UNIT_Z.getRotationTo(mSibling->mDirection);
		//mNode->setOrientation(rotate);
		//mNode->setScale(Vector3(-1,1,-1));
		//mNode
		//
		mNode->setOrientation(Quaternion::IDENTITY);
		mNode->setOrientation(rotate.inverse() * mNode->getOrientation());


		mdir.y *= -1;
		//mdir.x *= -1;
		//mdir *= -1;
	
		//Quaternion rotation = mDirection.getRotationTo(mSibling->mDirection);
		//Quaternion rotation = mSibling->mDirection.getRotationTo(mDirection);
		//mdir = rotation * mdir;

		mCamera->setDirection(mdir * -1);
		//mCamera->roll(180.f);
		//mCamera->setOrientation(mCamera->getOrientation() * rotate);


		// portal space camera position
		Vector3 pspace = mpos - mMesh->getPosition();

		// sibling portal space
		Vector3 sibSpace = mSibling->mMesh->getPosition();

		mNode->setPosition(sibSpace);
		mCamera->setPosition(pspace);

		//mNode->setPosition(mpos);
		//mCamera->setPosition(mpos);

		Vector3 pr = mSibling->mDirection * sibSpace.dotProduct(mSibling->mDirection);
		//mCamera->setCustomNearClip(mSibling->mDirection, -pr.length());

		//mCamera->enableReflection(mSibling->mDirection, sibSpace.z);*/

		/*Camera* mainCam = mOgre->getActiveCamera();
		Vector3 mpos = mainCam->getAbsolutePosition();
		Vector3 mdir = mainCam->getAbsoluteDirection();
		mCamera->setDirection(mdir);
		mCamera->setPosition(mpos);

		mCamera->enableReflection(Vector3(0,0,1), 0);*/
        

		// rotate direction into sibling space

		//if(b)
		//	mCamera->hackityHack("Portal1");
		//else
		//	mCamera->hackityHack("Portal2");
		//mainCam->hackityHack("Portal1");
		//mainCam->hackityHack("Portal2");
	}
}

void Portal::setSibling(Portal* p)
{	
	mSibling = p;
}
