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
class TerrainChunkGenerator;

class PlayState : public GameState
{
public:

	PlayState();
	virtual ~PlayState();

	virtual void init();
	virtual void update(Real delta);
	virtual void deinit();

	/** Used for updating portal cameras */
	void updateCam(const Message& m);

private:

	// Subsystems
	OgreSubsystem* mGfx;
	OISSubsystem* mInput;
	ALSubsystem* mAudio;
	GUISubsystem* mGUI;
	BulletSubsystem* mPhysics;

	// Chunk stuff
	ChunkManager* mChunkMgr;
	//BasicChunkGenerator* mGen;
	TerrainChunkGenerator* mGen;

	// Plain 'ol FPS camera
	FPSCamera* mCam;
	
	// GUI Mesh
	ScreenMesh* mUI;

	// Debug GUI captions
	Caption* mFpsText;
	Caption* mBatchCountText;
	Caption* mGeneratedChunkCountText;
	Caption* mActiveChunkCountText;
	Caption* mSelectionText;
	Caption* mCameraPositionText;

	// Currently selected block
	int mBlockSelected;

	// The portals TODO: make a separate class for managing portal shenanigans 
	// (inc. rendering stuff, gracefully handling >2 portals, etc...)
	Portal* mPortals[2];

};

#endif
