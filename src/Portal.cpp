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
	if(!blue)
	{
		mOgre->createRTT("blargh2", mCamera, 1024, 1024);
	}
	else
	{
		mOgre->createRTT("blargh", mCamera, 1024, 1024);
	}
	mCamera->setAspectRatio(1.3333f);
	//mDirection = direction;
	mNode = mOgre->createSceneNode();
	//mNode->addChild(mCamera);
	//mNode->yaw(180.f);
	mMesh = mOgre->createMesh("Portal.mesh");
	mMesh->setPosition(pos);
	//mMesh->setOrientation(Vector3::UNIT_Z.getRotationTo(mDirection, Vector3(0,1,0)));
	setDirection(direction);

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

#define PRINT_QUATERNION(Q) std::cout<<#Q<<": "<<Q.x<<" "<<Q.y<<" "<<Q.z<<" : "<<Q.w<<"\n";

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

		// TODO: add upAxis member, and use it as the fallback rotation axis

		//mMesh->setOrientation(Quaternion::IDENTITY.inverse() * 
		//	Vector3::UNIT_Z.getRotationTo(mDirection, 
		//		Vector3(0,1,0)));

		bool p1id = false;
		bool p2id = false;

		bool fall1 = false;
		bool fall2 = false;


		Vector3 ind = mDirection;
		Vector3 ind2 = mSibling->mDirection;
		ind.x *= -1;
		//ind2.x *= -1;
		Quaternion portal1Rotation = SceneNode::hack(ind, &fall1);
		Quaternion portal2Rotation = SceneNode::hack(ind2 * -1, &fall2);

		//if(fall1 && b)
		//	std::cout<<"FALL BACK!!!\n";
	
		/*Quaternion portal1Rotation = 
			//Quaternion::IDENTITY * 
			getRotationTo(Vector3::UNIT_Z,mDirection, Vector3(0,1,0), p1id, fall1)
			* Quaternion::IDENTITY;
		// dir * -1, since we're looking through the back of the portal
		Vector3 tmp = mSibling->mDirection * -1;
		Quaternion portal2Rotation = 
			//Quaternion::IDENTITY * 
			getRotationTo(Vector3::UNIT_Z, tmp, 
				Vector3(0,1,0), p2id, fall2)
				*Quaternion::IDENTITY;*/

		// observer position in portal1's coordinate space
		Vector3 portalSpacePos = mpos - portal1;

		// rotate into portal1 space
		portalSpacePos = portal1Rotation/*.inverse()*/ * portalSpacePos;

		// take 2, manually do the whole thang
		mNode->setPosition(Vector3::ZERO);
		mNode->setOrientation(Quaternion::IDENTITY);

		// rotate into portal2 space
		portalSpacePos = portal2Rotation * portalSpacePos;
		
		// translate into portal2 space
		portalSpacePos += portal2;
		Vector3 portalSpaceDir;

		portalSpaceDir = portal1Rotation * (mdir);
		portalSpaceDir = portal2Rotation * portalSpaceDir;

		mCamera->setPosition(portalSpacePos);
		mCamera->setDirection(portalSpaceDir);

		//Vector3 mirrorAlong = mSibling->mDirection.crossProduct(Vector3(0,1,0));
		// check for parallel vectors
		/*if(fall1 || fall2)
		{
			// perpendicular portal vectors seem to be a special case of some magical sort...
			Vector3 mirrorAlong = Vector3::UNIT_Y.crossProduct(mSibling->mDirection);
			//Vector3 mirrorAlong2;
			Vector3 mirrorAlong2 = mDirection.crossProduct(mSibling->mDirection);
			//Vector3 mirrorAlong2;

			portalSpaceDir = reflect(portalSpaceDir, mirrorAlong);
			portalSpaceDir = reflect(portalSpaceDir, mirrorAlong2);
			//portalSpaceDir = reflect(portalSpaceDir, mSibling->mDirection);

			portalSpacePos -= portal2;
			portalSpacePos = reflect(portalSpacePos, mirrorAlong);
			portalSpacePos = reflect(portalSpacePos, mirrorAlong2);
			portalSpacePos += portal2;
		}*/
			//mirrorAlong2 = mirrorAlong;
		//else
		//	mirrorAlong2 = mDirection.crossProduct(mSibling->mDirection);
	}
}

void Portal::setSibling(Portal* p)
{	
	mSibling = p;
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
		std::cout<<"ID\n";
		id = true;
		return Quaternion::IDENTITY;
	}

	if(d < (1e-6f - 1.0f))
	{
		std::cout<<"fallback!\n";
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
