#ifndef PlayState_H
#define PlayState_H

#include "Common.h"
#include "OryxGameState.h"
#include "ChunkManager.h"
#include "FPSCamera.h"
#include "GUISubsystem/GUIPanel.h"
#include "GUISubsystem/GUICaption.h"
#include "GUISubsystem/GUIButton.h"
#include "Portal.h"

class BasicChunkGenerator;

class PlayState : public GameState
{
public:

	PlayState();
	virtual ~PlayState();

	virtual void init();
	virtual void update(Real delta);
	virtual void deinit();

	void updateCam(const Message& m);

private:

	ChunkManager* mChunkMgr;
	OgreSubsystem* mGfx;
	OISSubsystem* mInput;
	ALSubsystem* mAudio;
	GUISubsystem* mGUI;
	BulletSubsystem* mPhysics;

	BasicChunkGenerator* mGen;
	FPSCamera* mCam;

	Caption* fps;
	Caption* batchCount;
	Caption* gcCount;
	Caption* acCount;
	Caption* selection;
	Caption* camPos;

	int block;

	Camera* portalcam1;
	Camera* portalcam2;

	SceneNode* pn1;
	SceneNode* pn2;

	Portal* port1;
	Portal* port2;

	Quaternion savedOri;
	Vector3 savedPos;

	ScreenMesh* mUI;
};

#endif
