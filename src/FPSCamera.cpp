#include "Oryx.h"
#include "FPSCamera.h"
#include "OryxMessageAny.h"
#include "OryxEventHandler.h"
#include "OryxTimeManager.h"

namespace Oryx
{
	FPSCamera::FPSCamera()
		:Object(),mPitch(0)
	{
		createSlot("look",this,&FPSCamera::look);
		EventHandler::getDestination("OISSubsystem")->getSignal("mouseMoved")
			->addListener(getSlot("look"));

		mOgre = Engine::getPtr()->getSubsystem("OgreSubsystem")->castType<OgreSubsystem>();
		mOIS = Engine::getPtr()->getSubsystem("OISSubsystem")->castType<OISSubsystem>();

		mCamera = mOgre->createCamera();
		mCamera->setFarClip(120.f);
		mCamera->setNearClip(0.001f);
		mOgre->setActiveCamera(mCamera);

		mRollNode = mOgre->createSceneNode();
		mYawNode = mOgre->createSceneNode();
		mPitchNode = mOgre->createSceneNode();
		mPosNode = mOgre->createSceneNode();
		mOriNode = mOgre->createSceneNode();

		mOgre->getRootSceneNode()->addChild(mPosNode);
		mPosNode->addChild(mOriNode);
		mOriNode->addChild(mYawNode);
		mYawNode->addChild(mPitchNode);
		mPitchNode->addChild(mRollNode);
		mRollNode->addChild(mCamera);

		mCamera->setFOV(70.f);
	}

	void FPSCamera::update(Real delta)
	{
		//mPosNode->setPosition(mPosNode->getPosition()
		//	+mCamera->getAbsoluteDirection()*20*delta*(mOIS->isKeyDown("KC_W")-mOIS->isKeyDown("KC_S"))
		//	+mCamera->getAbsoluteRight()*20*delta*(mOIS->isKeyDown("KC_D")-mOIS->isKeyDown("KC_A")));
	}

	void FPSCamera::look(const Message& msg)
	{
		if(const MessageAny<Vector2>* ms = message_cast<Vector2>(msg))
		{
			Real y = (static_cast<Real>(ms->data.x)*-0.25f);
			if(y > 0.f)
				y = std::min(y, 5.f);
			else
				y = std::max(y, -5.f);

			mYawNode->yaw(y);

			Real tryPitch = ms->data.y*-0.25f;
			Real actualPitch = (tryPitch);

			if(actualPitch > 0.f)
				actualPitch = std::min(actualPitch, 5.f);
			else
				actualPitch = std::max(actualPitch, -5.f);

			tryPitch = actualPitch;

			if(mPitch + tryPitch > 90.f)
				actualPitch = 90.f - mPitch;
			else if(mPitch + tryPitch < -90.f)
				actualPitch = -90.f - mPitch;

			mPitch += actualPitch;
			mPitchNode->pitch(actualPitch);
		}
	}

	Vector3 FPSCamera::getPosition()
	{
		return mCamera->getAbsolutePosition();
	}

	Vector3 FPSCamera::getDirection()
	{
		return mCamera->getAbsoluteDirection();
	}
}
