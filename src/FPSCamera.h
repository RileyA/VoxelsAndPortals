#ifndef FPSCAM_H
#define FPSCAM_H

#include "Oryx.h"
#include "OryxEngine.h"
#include "OryxObject.h"
#include "OgreSubsystem/OgreSubsystem.h"
#include "OISSubsystem/OISSubsystem.h"

namespace Oryx
{
	class FPSCamera : public Object
	{
	public:

		FPSCamera();
		void update(Real delta);
		void look(const Message& msg);

		Vector3 getPosition();
		Vector3 getDirection();

		OgreSubsystem* mOgre;
		OISSubsystem* mOIS;

		Real mPitch;

		Camera* mCamera;
		SceneNode* mRollNode;	
		SceneNode* mYawNode;
		SceneNode* mPitchNode;
		SceneNode* mPosNode;
		SceneNode* mOriNode;

		Vector3 up1;
		Vector3 up2;

		Real lastYaw;
		Real lastPitch;
	};
}

#endif
