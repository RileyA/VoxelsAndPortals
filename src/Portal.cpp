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
	mNode = mOgre->createSceneNode();
	mNode->setPosition(Vector3::ZERO);
	mNode->setOrientation(Quaternion::IDENTITY);
	mMesh = mOgre->createMesh("Portal.mesh");
	mMesh->setPosition(pos);
	setDirection(direction);

	mNode->addChild(mCamera);
	//mNode->yaw(180.f);
	//mNode->setScale(Vector3(1,1,-1));

	mCamera->setCamPosition(Vector3(0,0,0));
	mCamera->setDirection(Vector3(0,0,-1));

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

		// TAKE 3 (3rd times the... ahhh fuck forget it)

		//Vector3 pos = mpos - portal1;
		//mCamera->setPosition(pos);

		// relative to portal
		//Vector3 relative = mpos - mMesh->getPosition();
		//relative = mMesh->getOrientation().inverse() * relative;

		mCamera->setPosition(mMesh->worldToLocalPosition(mainCam->getAbsolutePosition()));
		mCamera->setOrientation(mMesh->worldToLocalOrientation(mainCam->getCamAbsoluteOrientation()));
		mCamera->enableReflection(mSibling->mDirection,
			- mSibling->mDirection.dotProduct(mSibling->mMesh->getPosition() * -1));
		
		
		//mCamera->setCamOrientation(Quaternion::IDENTITY);

		//mCamera->setPosition(relative);
		//mCamera->setOrientation(mMesh->getOrientation().inverse());

		//mCamera->setCamPosition();
		//mCamera->setCamOrientation(Quaternion::IDENTITY);

		//mCamera->setDirection(mdir);
		//mCamera->setCamOrientation(mMesh->getOrientation() * mCamera->getCamOrientation());

		// TAKE 2

		/*Vector3 mydir = mDirection * -1;
		Vector3 sibdir = mSibling->mDirection;
		
		Quaternion rotation = mydir.getRotationTo(sibdir);
		Vector3 p1spacepos = mpos - portal1;

		Vector3 p2spacepos = rotation * p1spacepos;
		Vector3 p2spacedir = rotation * mdir;

		Vector3 wspacepos = p2spacepos + portal2;

		mCamera->setPosition(wspacepos);
		mCamera->setDirection(p2spacedir);*/

		//mCamera->setPosition(portal2 - mSibling->mDirection * 4);
		//mCamera->setDirection(mSibling->mDirection);

		// TAKE 1
		/*Vector3 ind = mDirection;
		Vector3 ind2 = mSibling->mDirection;
		//ind.x *= -1;
		//ind.x *= -1;
		//ind.z *= -1;
		//ind2.x *= -1;
		//ind2.y *= -1;
		Quaternion portal1Rotation = SceneNode::hack(ind);
		Quaternion portal2Rotation = SceneNode::hack(ind2 * -1);

		// observer position in portal1's coordinate space
		Vector3 portalSpacePos = mpos - portal1;
		// rotate into each space
		portalSpacePos = portal1Rotation * portalSpacePos;// inverse?
		portalSpacePos = portal2Rotation * portalSpacePos;
		// translate into portal2 space
		portalSpacePos += portal2;
		Vector3 portalSpaceDir;
		// rotate direction
		portalSpaceDir = portal1Rotation * (mdir);
		portalSpaceDir = portal2Rotation * portalSpaceDir;
		// set it
		mCamera->setPosition(portalSpacePos);
		mCamera->setDirection(portalSpaceDir);*/
	}
}

void Portal::setSibling(Portal* p)
{	
	mSibling = p;
	mSibling->mMesh->addChild(mNode);
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
