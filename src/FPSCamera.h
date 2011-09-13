//---------------------------------------------------------------------------
//(C) Copyright Riley Adams 2011

//This file is part of Oryx Engine.

// Oryx Engine is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the license, or
// (at your option) any later version.

// Oryx Engine is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTEE; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

// You should have recieved a copy of the GNU General Public License
// along with Oryx Engine. If not, see <http://www.gnu.org/licenses/>
//---------------------------------------------------------------------------

#ifndef VERSUS_FPSCAM
#define VERSUS_FPSCAM

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
	};
}

#endif
