#ifndef Portal_H
#define Portal_H

#include "Common.h"
#include "Oryx.h"
#include "OryxObject.h"
#include "Oryx3DMath.h"
#include "OgreSubsystem/OgreSubsystem.h"

class Portal : public Oryx::Object
{
public:

	Portal(Vector3 pos, Vector3 direction, bool blue);
	virtual ~Portal();

	void update(Real delta);

	void setSibling(Portal* p);

	void init();

	void setDirection(Vector3 d)
	{
		mDirection = d;
		mMesh->setOrientation(Quaternion::IDENTITY);
		mMesh->setDirectionHack(d);
		//mMesh->setOrientation( 
		//	Vector3::UNIT_Z.getRotationTo(mDirection, 
		//		Vector3(0,0,1)));

		//mMesh->setOrientation(Quaternion::IDENTITY);
		//mMesh->yaw(-d.angleBetween(Vector3::UNIT_Z));

		/*if(mDirection == Vector3::UNIT_Z)
		{
			mMesh->setOrientation(Quaternion::IDENTITY);
			//mMesh->roll(180.f);
		}
		else
			mMesh->setOrientation(Vector3::UNIT_Z.getRotationTo(mDirection, 
				Vector3(0,1,0)));*/
	}

	Camera* getCamera()
	{
		return mCamera;
	}
	//void observerMoved(const Message& msg);

	String rtt1;
	String rtt2;

protected:

	Camera* mCamera;
	SceneNode* mNode;
	Mesh* mMesh;

	Portal* mSibling;
	OgreSubsystem* mOgre;

	Vector3 mDirection;
	Vector3 mPosition;
	String rtt;
	bool b;


};

#endif
