#include "Portal.h"
#include "OryxEngine.h"
#include "OgreSubsystem/OgreSubsystem.h"

Quaternion getRotationTo(const Vector3& orig, 
	const Vector3& dest, const Vector3& fallback, bool& id, bool& fall);

Portal::Portal(Vector3 pos, Vector3 direction, bool blue)
{
	mOgre = Engine::getPtr()->getSubsystem("OgreSubsystem")->castType<OgreSubsystem>();
	mCamera = mOgre->createCamera();
	mCamera->setPosition(Vector3(0,0,0));// it'll move into place once it has a siblingA
	mCamera->setDirection(Vector3(0,0,1));
	mCamera->setFOV(70.f);

	Camera* mainCam = mOgre->getActiveCamera();
	/*if(!blue)
	{
		rtt1 = mOgre->createRTT("blargh2", mCamera, 1024, 1024, "portal");
		rtt2 = mOgre->createRTT("blargh2_2", mCamera2, 1024, 1024, "portal2");
		//rtt2 = mOgre->createRTT("blargh2_2", mainCam, 1024, 1024, "portal2");
	}
	else
	{
		//rtt1 = mOgre->createRTT("blargh", mCamera, 1024, 1024, "portal");
		rtt1 = mOgre->createRTT("blargh", mCamera, 1024, 1024, "portal");
		rtt2 = mOgre->createRTT("blargh_2", mCamera2, 1024, 1024, "portal2");
		//rtt2 = mOgre->createRTT("blargh_2", mainCam, 1024, 1024, "portal2");
	}*/
	mCamera->setAspectRatio(1.3333f);
	mNode = mOgre->createSceneNode();
	mNode->setPosition(Vector3::ZERO);
	mNode->setOrientation(Quaternion::IDENTITY);

	mNode->addChild(mCamera);
	mNode->yaw(180.f);

	mCamera->setCamPosition(Vector3(0,0,0));
	mCamera->setDirection(Vector3(0,0,1));

	mSibling = 0;
	b = blue;

	mDirection = direction;
	mPosition = pos;

	mMesh = mOgre->createMesh("Portal.mesh");
	mMesh->setPosition(mPosition);
	mBorder = mOgre->createMesh("PortalBorder.mesh");
	mOgre->getRootSceneNode()->addChild(mBorder);
	mBorder->setPosition(mPosition);
	setDirection(mDirection);

	if(!b)
	{
		mMesh->setMaterialName("Portal2_new");
		mMesh->setRenderQueueGroup(76);
		mBorder->setMaterialName("Portal2_new2");
	}
	else
	{
		mMesh->setMaterialName("Portal1_new");
		mMesh->setRenderQueueGroup(75);
		mBorder->setMaterialName("Portal1_new2");
	}


	mOgre->getRootSceneNode()->addChild(mMesh);
}

Portal::~Portal()
{

}

Vector3 reflect(Vector3 i, Vector3 n)
{
	return i - n * 2 * i.dotProduct(n);
}

#define PRINT_QUATERNION(Q) std::cout<<#Q<<": "<<Q.x<<" "<<Q.y<<" "<<Q.z<<" : "<<Q.w<<"\n";

void Portal::update(Real delta)
{
	if(mSibling)
	{
		Camera* mainCam = mOgre->getActiveCamera();
		Vector3 localPos = mMesh->worldToLocalPosition(mainCam->getAbsolutePosition());
		Quaternion localOri = mMesh->worldToLocalOrientation(mainCam->getCamAbsoluteOrientation());
		mCamera->setPosition(localPos);
		mCamera->setOrientation(localOri);
		mCamera->setCustomNearClip(mSibling->mDirection, mSibling->mDirection.dotProduct(
			mSibling->mMesh->getPosition()));
	}
}

void Portal::setSibling(Portal* p)
{	
	mSibling = p;
	mSibling->mMesh->addChild(mNode);
}

void Portal::recurse()
{
	Vector3 oldPos = mCamera->getAbsolutePosition();
	Quaternion oldOri = mCamera->getAbsoluteOrientation();
	mCamera->setPosition(mMesh->worldToLocalPosition(oldPos));
	mCamera->setOrientation(mMesh->worldToLocalOrientation(oldOri));
}

Quaternion getRotationTo(const Vector3& orig, 
	const Vector3& dest, const Vector3& fallback, bool& id, bool& fall)
{
	// Based on Stan Melax's article in Game Programming Gems
	Vector3 fallbackAxis = fallback;
	Quaternion q;
	Vector3 v0 = orig;
	Vector3 v1 = dest;
	v0.normalize();
	v1.normalize();

	Real d = v0.dotProduct(v1);

	// If dot == 1, vectors are the same
	if(d >= 1.0f)
	{
		id = true;
		return Quaternion::IDENTITY;
	}

	if(d < (1e-6f - 1.0f))
	{
		if(fallbackAxis != Vector3::ZERO)
		{
			// rotate 180 degrees about the fallback axis
			q.FromAngleAxis(3.141592, fallbackAxis);
			fall = true;
		}
		else
		{
			// Generate an axis
			Vector3 axis = Vector3::UNIT_X.crossProduct(orig);

			if(axis.isZeroLength()) // pick another if colinear
			{
				axis = Vector3::UNIT_Y.crossProduct(orig);
				axis.normalize();
				q.FromAngleAxis(3.141592, axis);
			}
		}
	}
	else
	{
		Real s = sqrt( (1+d)*2 );
			Real invs = 1 / s;
		Vector3 c = v0.crossProduct(v1);
		q.x = c.x * invs;
		q.y = c.y * invs;
		q.z = c.z * invs;
		q.w = s * 0.5f;
		q.normalize();
	}
	return q;
}

