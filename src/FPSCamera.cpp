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
		mCamera->setFarClip(200.f);
		mCamera->setNearClip(0.1f);
		mOgre->setActiveCamera(mCamera);

		mRollNode = mOgre->createSceneNode();
		mYawNode = mOgre->createSceneNode();
		mPitchNode = mOgre->createSceneNode();
		mPosNode = mOgre->createSceneNode();

		mOgre->getRootSceneNode()->addChild(mPosNode);
		mPosNode->addChild(mYawNode);
		mYawNode->addChild(mPitchNode);
		mPitchNode->addChild(mRollNode);
		mRollNode->addChild(mCamera);

		mCamera->setFOV(70.f);
	}

	void FPSCamera::update(Real delta)
	{
		//Vector3 last = mPosNode->getPosition();
		mPosNode->setPosition(mPosNode->getPosition()
			+mCamera->getAbsoluteDirection()*5*delta*(mOIS->isKeyDown("KC_W")-mOIS->isKeyDown("KC_S"))
			+mCamera->getAbsoluteRight()*5*delta*(mOIS->isKeyDown("KC_D")-mOIS->isKeyDown("KC_A")));
		//last -= mPosNode->getPosition();
		//std::cout<<last.x<<" "<<last.y<<" "<<last.z<<" "<<delta<<"\n";
	}

	void FPSCamera::look(const Message& msg)
	{
		if(const MessageAny<Vector2>* ms = message_cast<Vector2>(msg))
		{
			mYawNode->yaw(static_cast<Real>(ms->data.x)*-0.5f);

			Real tryPitch = ms->data.y*-0.5f;
			Real actualPitch = tryPitch;

			if(mPitch + tryPitch > 80.f)
				actualPitch = 80.f - mPitch;
			else if(mPitch < -80.f)
				actualPitch = -80.f - mPitch;
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
