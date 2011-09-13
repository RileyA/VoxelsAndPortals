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
	//void observerMoved(const Message& msg);

protected:

	Camera* mCamera;
	SceneNode* mNode;
	Mesh* mMesh;

	Portal* mSibling;
	OgreSubsystem* mOgre;

	Vector3 mDirection;
	String rtt;
	bool b;

};

#endif
