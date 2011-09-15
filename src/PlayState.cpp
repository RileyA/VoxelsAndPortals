#include "Common.h"
#include "PlayState.h"
#include "BasicChunkGenerator.h"
#include "BasicChunk.h"
#include "ChunkUtils.h"
#include "FPSCamera.h"
#include "Portal.h"


PlayState::PlayState()
	:mChunkMgr(0), mGfx(0), mAudio(0), mGUI(0), mInput(0), mPhysics(0)
{}
//---------------------------------------------------------------------------

PlayState::~PlayState()
{
}
//---------------------------------------------------------------------------

void PlayState::init()
{
	srand(time(0));

	// grab pointers to e'rythang
	mGfx = dynamic_cast<OgreSubsystem*>(mEngine->getSubsystem("OgreSubsystem"));
	mAudio = dynamic_cast<ALSubsystem*>(mEngine->getSubsystem("ALSubsystem"));
	mInput = dynamic_cast<OISSubsystem*>(mEngine->getSubsystem("OISSubsystem"));
	mGUI = dynamic_cast<GUISubsystem*>(mEngine->getSubsystem("GUISubsystem"));
	mPhysics = dynamic_cast<BulletSubsystem*>(mEngine->getSubsystem("BulletSubsystem"));

	// start up input, grab the mouse
	mInput->initInput(mGfx->getWindowHandle(), false);

	// start up bullet for collision detection
	mPhysics->startSimulation();

	// a dark background color helps hide momentary gaps, that may occur when blocks are
	// changed on edges, and one chunk updates before the other
	mGfx->setBackgroundColor(Colour(0,0,0));

	// enable portal stencil hack with 5 visual recursions
	mGfx->enablePortalHack(2);
	
	// so we can get signals during portal rendering (for setting up camera, visibility, etc)
	mGfx->getSignal("updateCam")->addListener(createSlot("updateCam", this, &PlayState::updateCam));

	// standard FPS-style camera (no character controller just yet)
	mCam = new FPSCamera();

	// make the portals
	mPortals[0] = new Portal(Vector3(-6.5f,-7,-3.f), BD_BACK, BD_UP, true);
	mPortals[1] = new Portal(Vector3(-6.5f,-7,3), BD_RIGHT, BD_UP, false);

	// connect them (eventually I'd like to allow any number of portal pairs, but for now
	// everythings very hacky and hardcoded to do two portals...
	mPortals[0]->setSibling(mPortals[1]);
	mPortals[1]->setSibling(mPortals[0]);

	// Set up chunk rendering stuffs
	mChunkMgr = new ChunkManager();
	mGen = new BasicChunkGenerator();
	mGen->setPlayerPos(Vector3(0,0,0));
	mChunkMgr->init(Vector3(0,0,0), mGen);

	// set up debug overlay
	Batch* b = mGUI->createBatch("test", "TechDemo.oyster");
	
	Caption* c = new Caption(b, 0);
	c->setCaption("Worker threads: 6");
	c->setPosition(Vector2(0.01f, 0.94f));

	c = new Caption(b, 0);
	c->setCaption("Portal Depth: 5");
	c->setPosition(Vector2(0.01f, 0.89f));

	c = new Caption(b, 0);
	c->setCaption("FPS: 60");
	c->setPosition(Vector2(0.01f, 0.69f));
	mFpsText = c;

	c = new Caption(b, 0);
	c->setCaption("Batches: 0");
	c->setPosition(Vector2(0.01f, 0.74f));
	mBatchCountText = c;

	c = new Caption(b, 0);
	c->setCaption("Generated Chunks: 125");
	c->setPosition(Vector2(0.01f, 0.79f));
	mGeneratedChunkCountText = c;

	c = new Caption(b, 0);
	c->setCaption("Active Chunks: 27");
	c->setPosition(Vector2(0.01f, 0.84f));
	mActiveChunkCountText = c;

	c = new Caption(b, 0);
	c->setCaption("Block: 1");
	c->setPosition(Vector2(0.8f, 0.94f));
	mSelectionText = c;

	mBlockSelected = 1;

	// hook it up with the rendering system
	mUI = mGfx->createScreenMesh("UITEST");
	b->getSignal("update")->addListener(mUI->getSlot("update"));
	mUI->setHidden(false);
}
//---------------------------------------------------------------------------

void PlayState::update(Real delta)
{
	// update player position with the chunk generator
	mGen->setPlayerPos(mCam->getPosition());
	
	// same o' same o'
	if(mInput->wasKeyPressed("KC_ESCAPE"))
		mEngine->shutdown();

	// screenshots
	if(mInput->wasKeyPressed("KC_P"))
		mGfx->takeScreenshot();

	// update debug overlay
	mFpsText->setCaption("FPS: "+ StringUtils::toString(1.f/delta));
	mBatchCountText->setCaption("Batches: " 
		+ StringUtils::toString(mGfx->getBatchCount()));
	mActiveChunkCountText->setCaption("Active Chunks: " +
		StringUtils::toString(mGen->getNumActiveChunks()));
	mGeneratedChunkCountText->setCaption("Generated Chunks: " +
		StringUtils::toString(mGen->getNumGeneratedChunks()));

	// delete a block
	if(mInput->wasButtonPressed("MB_Right"))
	{
		BulletSubsystem* b = Engine::getPtr()->getSubsystem("BulletSubsystem")->
			castType<BulletSubsystem>();
		
		RaycastReport r = b->raycast(mCam->getPosition(),mCam->getDirection(),
			8.f,COLLISION_GROUP_3,COLLISION_GROUP_3);

		if(r.hit && r.userData)
		{
			BasicChunk* bc = static_cast<BasicChunk*>(r.userData);
			ChunkCoords cc = BasicChunk::getBlockFromRaycast(r.position, 
				r.normal, bc, true);
			bc = correctChunkCoords(bc, cc);
			cc.c.data = 0;
			bc->changeBlock(cc);
		}
	}

	// add a block
	if(mInput->wasButtonPressed("MB_Left"))
	{
		RaycastReport r = mPhysics->raycast(mCam->getPosition(),mCam->getDirection(),
			5.f,COLLISION_GROUP_3,COLLISION_GROUP_3);

		if(r.hit && r.userData)
		{
			BasicChunk* bc = static_cast<BasicChunk*>(r.userData);
			ChunkCoords cc = BasicChunk::getBlockFromRaycast(r.position, r.normal, bc, false);
			bc = correctChunkCoords(bc, cc);
			cc.c.data = mBlockSelected;
			bc->changeBlock(cc);
		}

		// Early start at placing portals
		/*BulletSubsystem* b = Engine::getPtr()->getSubsystem("BulletSubsystem")->
			castType<BulletSubsystem>();
		
		RaycastReport r = b->raycast(mCam->getPosition(),mCam->getDirection(),
			5.f,COLLISION_GROUP_3,COLLISION_GROUP_3);

		if(r.hit && r.userData)
		{
			BasicChunk* bc = static_cast<BasicChunk*>(r.userData);
			ChunkCoords cc = BasicChunk::getBlockFromRaycast(r.position, r.normal, bc, false);
			bc = correctChunkCoords(bc, cc);

			Vector3 adjPos = bc->getPosition() - OFFSET + Vector3(cc.c.x, cc.c.y, cc.c.z);
			
			mPortals[0]->setPosition(adjPos);
			mPortals[0]->setDirection(r.normal);

			//cc.c.data = block;
			//bc->changeBlock(cc);
		}*/
	}
}
//---------------------------------------------------------------------------

void PlayState::deinit() {}
//---------------------------------------------------------------------------

void PlayState::updateCam(const Message& m)
{
	const MessageAny<std::pair<int, int> >* ms =
		message_cast<std::pair<int, int> >(m);

	int portal = ms->data.first;
	int pass = ms->data.second;

	if(portal == 0)
	{
		mPortals[0]->setVisible(true);
		mPortals[1]->setVisible(true);
		mGfx->setActiveCamera(mCam->mCamera);
		mUI->setHidden(false);
	}
	else if(portal == 1)
	{
		mPortals[0]->setVisible(true);
		mPortals[1]->setVisible(false);
		mGfx->setActiveCamera(mPortals[0]->getCamera());

		if(pass > 0)
			mPortals[0]->recurse();
	}
	else if(portal == 2)
	{
		mPortals[0]->setVisible(false);
		mPortals[1]->setVisible(true);
		mGfx->setActiveCamera(mPortals[1]->getCamera());

		if(pass > 0)
			mPortals[1]->recurse();
		// render UI at the end of the last render
		if(pass == 1)
			mUI->setHidden(true);
	}
	else if(portal == 100)
	{
		mGfx->setActiveCamera(mCam->mCamera);
		mPortals[0]->setVisible(true);
		mPortals[1]->setVisible(true);
	}
}
//---------------------------------------------------------------------------
