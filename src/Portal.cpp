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
	mMesh->setOrientation(Vector3::UNIT_Z.getRotationTo(mDirection, Vector3(0,1,0)));

	if(!blue)
		mMesh->setMaterialName("Portal2");

	mOgre->getRootSceneNode()->addChild(mMesh);
	mSibling = 0;
	b = blue;
}

Portal::~Portal()
{

}

Vector3 reflect(Vector3 i, Vector3 n)
{
	return i - n * 2 * i.dotProduct(n);
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
		Vector3 portal1 = mMesh->getPosition();
		Vector3 portal2 = mSibling->mMesh->getPosition();

		Quaternion portal1Rotation = Vector3::UNIT_Z.getRotationTo(mDirection, Vector3(0,1,0));
		// dir * -1, since we're looking through the back of the portal
		Quaternion portal2Rotation = Vector3::UNIT_Z.getRotationTo(mSibling->mDirection * -1, Vector3(0,1,0));

		// observer position in portal1's coordinate space
		Vector3 portalSpacePos = mpos - portal1;
		// rotate in
		portalSpacePos = portal1Rotation * portalSpacePos;

		Vector3 portalSpaceDir = portal1Rotation * mdir;
		portalSpaceDir = portal2Rotation * portalSpaceDir;

		// now apply portal2 transforms to the parent node
		//mNode->setPosition(portal2);
		//mNode->setOrientation(portal2Rotation.inverse());

		//mCamera->setPosition(portalSpacePos);
		//mCamera->setDirection(portalSpaceDir);

		// take 2, manually do the whole thang
		mNode->setPosition(Vector3::ZERO);
		mNode->setOrientation(Quaternion::IDENTITY);

		// now rotate into portal2 space
		// rotate in
		/*portalSpacePos = portal2Rotation.inverse() * portalSpacePos;
		portalSpacePos = portalSpacePos + portal2;

		portalSpaceDir = portal2Rotation * portalSpaceDir;*/

		portalSpacePos = portal2Rotation * portalSpacePos;
		
		//portal2.x *= -1;
		//portal2.z *= -1;
		
		//portal2 = portal1Rotation * portal2;
		//portal2 = reflect(portal2, mSibling->mDirection);
		//portal2.y *= -1;
		//portalSpacePos -= portal2;

		//portal2.y *= -1;
		portalSpacePos += portal2;

		//portalSpacePos.y *= -1;
		//portalSpacePos += reflect(portal2, mSibling->mDirection);


		//portalSpacePos.z *= -1;
		//portalSpacePos.x *= -1;
		//portalSpacePos.y *= -1;

		// I - 2 dot(n, I) * n
		//Vector3 normal = /*mSibling->*/mDirection;
		//Vector3 reflectedPos = portalSpacePos - normal * 2 * portalSpacePos.dotProduct(normal);
		//reflectedPos.x *=-1;
		//mCamera->setPosition(reflectedPos);

		mCamera->setPosition(portalSpacePos);
		//portalSpaceDir.y *= -1;
		mCamera->setDirection(portalSpaceDir);

		if(b)
		{
			std::cout<<"pos: "<<mCamera->getAbsolutePosition().x<<" "<<mCamera->getAbsolutePosition().y<<" "
				<<mCamera->getAbsolutePosition().z<<"\n";
			//std::cout<<"dir: "<<mCamera->getAbsoluteDirection().x<<" "<<mCamera->getAbsoluteDirection().y<<" "
			//	<<mCamera->getAbsoluteDirection().z<<"\n";
		}

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
