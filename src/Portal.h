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

	void setDirection(Vector3 d)
	{
		mDirection = d;
		mMesh->setOrientation(Quaternion::IDENTITY);
		mMesh->setDirectionHack(d);
		mBorder->setOrientation(Quaternion::IDENTITY);
		mBorder->setDirectionHack(d);
	}

	void setDirection(Vector3 d, Vector3 up)
	{
		mDirection = d;
		Quaternion q;
		q.fromAxes(up.crossProduct(d), up, d);
		mMesh->setOrientation(q);
		mBorder->setOrientation(q);
	}

	void setPosition(Vector3 p)
	{
		mMesh->setPosition(p);
		mBorder->setPosition(p);
	}

	void recurse();

	Camera* getCamera()
	{
		return mCamera;
	}

	void setVisible(bool v)
	{
		mMesh->setVisible(v);
		mBorder->setVisible(v);
	}

	String rtt1;
	String rtt2;

protected:

	Camera* mCamera;
	SceneNode* mNode;
	Mesh* mMesh;
	Mesh* mBorder;

	Portal* mSibling;
	OgreSubsystem* mOgre;

	//bool mRecursed;
	//Vector3 oldPos;
	//Vector3 oldOri;

	Vector3 mDirection;
	Vector3 mPosition;
	String rtt;
	bool b;


};

#endif
