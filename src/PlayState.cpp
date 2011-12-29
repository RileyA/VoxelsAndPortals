#include "Common.h"
#include "PlayState.h"
#include "BasicChunkGenerator.h"
#include "TerrainChunkGenerator.h"
#include "BasicChunk.h"
#include "ChunkUtils.h"
#include "FPSCamera.h"
#include "Portal.h"
#include "OgreSubsystem/CustomRenderSequence.h"

const static int PORTAL_RENDER_DEPTH = 3;

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

	// background and fog colors
	Colour col = Colour(255/255.f,200/255.f,158/255.f);
	Colour col2 = Colour(211/255.f,234/255.f,243/255.f);
	mGfx->setBackgroundColor(col2);
	mGfx->setLinearFog(30.f,60.f,col);

	// Setup portal render sequence:

	// initial pass
	CustomRenderIteration& basePass = mRenderSequence.addIteration();

	// first pass, the framebuffer is already clear
	basePass.clearDepth = false;

	// write 0's
	basePass.addStencilConfig(-1, CMPF_EQUAL, 0, 0xFFFFFFFF, 
		SOP_ZERO, SOP_ZERO, SOP_ZERO);

	// make the portals write distinct values to the stencil buffer
	basePass.addStencilConfig(75, CMPF_ALWAYS_PASS, 1,  0xFFFFFFFF, 
		SOP_KEEP, SOP_KEEP, SOP_REPLACE);
	basePass.addStencilConfig(76, CMPF_ALWAYS_PASS, 51, 0xFFFFFFFF, 
		SOP_KEEP, SOP_KEEP, SOP_REPLACE);

	// add portal iterations
	for(int p = 0; p < 2; ++p)
	{
		for(int i = 0; i < PORTAL_RENDER_DEPTH; ++i)
		{
			CustomRenderIteration& portal_pass = mRenderSequence.addIteration();
			portal_pass.clearDepth = true;

			// base stencil settings (only draw if within portal mask from prior frame)
			portal_pass.addStencilConfig(-1, CMPF_EQUAL, 1 + i + 50 * p,
				0xFFFFFFFF, SOP_KEEP, SOP_KEEP, SOP_KEEP);

			// portal-specific render queue settings (only draw if within portal 
			// from prior frame, and increment to define the mask for the next frame)
			portal_pass.addStencilConfig(75 + p, CMPF_EQUAL, 1 + i + 50 * p,
				0xFFFFFFFF, SOP_KEEP, SOP_KEEP, SOP_INCREMENT);
		}
	}

	mGfx->enableCustomRenderSequence(&mRenderSequence);

	// so we can get signals during portal rendering (for setting up camera, visibility, etc)
	mGfx->getSignal("CustomRenderSequenceIteration")->
		addListener(createSlot("updateCam", this, &PlayState::updateCam));

	// standard FPS-style camera (no character controller just yet)
	mCam = new FPSCamera();

	// make the portals
	mPortals[0] = new Portal(Vector3(-6.5f,-7,-3.f), BD_BACK, BD_UP, true);
	mPortals[1] = new Portal(Vector3(-6.5f,-7,3), BD_LEFT, BD_UP, false);

	// connect them (eventually I'd like to allow any number of portal pairs, but for now
	// everythings very hacky and hardcoded to do two portals...
	mPortals[0]->setSibling(mPortals[1]);
	mPortals[1]->setSibling(mPortals[0]);

	// Set up chunk rendering stuffs
	mChunkMgr = new ChunkManager();
	mGen = new TerrainChunkGenerator();
	mGen->setPlayerPos(Vector3(0,0,0));
	mChunkMgr->init(Vector3(0,0,0), mGen);

	// set up debug overlay
	Batch* b = mGUI->createBatch("test", "TechDemo.oyster");
	
	Caption* c = new Caption(b, 0);
	c->setCaption("Worker Threads: " + StringUtils::toString(GENERATOR_WORKER_THREADS));
	c->setPosition(Vector2(0.01f, 0.94f));

	c = new Caption(b, 0);
	c->setCaption("Portal Depth: " + StringUtils::toString(PORTAL_RENDER_DEPTH));
	c->setPosition(Vector2(0.01f, 0.89f));

	c = new Caption(b, 0);
	c->setCaption("FPS: 60");
	c->setPosition(Vector2(0.01f, 0.74f));
	mFpsText = c;

	c = new Caption(b, 0);
	c->setCaption("Generated Chunks: 125");
	c->setPosition(Vector2(0.01f, 0.79f));
	mGeneratedChunkCountText = c;

	c = new Caption(b, 0);
	c->setCaption("Active Chunks: 27");
	c->setPosition(Vector2(0.01f, 0.84f));
	mActiveChunkCountText = c;

	c = new Caption(b, 0);
	c->setCaption("Block: 6");
	c->setPosition(Vector2(0.7f, 0.94f));
	mSelectionText = c;

	mBlockSelected = 6;

	// hook it up with the rendering system
	mUI = mGfx->createScreenMesh("DebugUI");
	b->getSignal("update")->addListener(mUI->getSlot("update"));
	mUI->setHidden(false);
}
//---------------------------------------------------------------------------

void PlayState::update(Real delta)
{
	// fps cam update
	Vector3 moveVect = mCam->mCamera->getAbsoluteDirection()*7*delta*
		(mInput->isKeyDown("KC_W")-mInput->isKeyDown("KC_S"))
		+mCam->mCamera->getAbsoluteRight()*7*delta*
		(mInput->isKeyDown("KC_D")-mInput->isKeyDown("KC_A"));

	Real len = moveVect.normalize();

	// hacky raycast for portals
	RaycastReport r = mPhysics->raycast(mCam->getPosition(),moveVect,
		len,0,COLLISION_GROUP_2);//COLLISION_GROUP_2, COLLISION_GROUP_2);

	if(!r.hit)
	{
		mCam->mPosNode->setPosition(mCam->mPosNode->getPosition()
			+ moveVect * len);
	}
	else
	{
		// must be a portal
		Portal* p = static_cast<Portal*>(r.userData);

		// hacky and not so smooth portal transition
		if(r.normal.angleBetween(p->mDirection) < 30.f)
		{
			Real distTravelled = r.position.distance(mCam->getPosition());
			
			mCam->mPosNode->setPosition(p->mNode->localToWorldPosition(
				p->mMesh->worldToLocalPosition(mCam->getPosition())));
			mCam->mOriNode->setOrientation(p->mNode->localToWorldOrientation(
				p->mMesh->worldToLocalOrientation(mCam->mOriNode->getOrientation())));
			
			len -= 2*distTravelled;

			Vector3 newDir = mCam->getDirection();
			mCam->mPosNode->setPosition(mCam->mPosNode->getPosition()
				+ newDir * len);
		}
		else
		{
			mCam->mPosNode->setPosition(mCam->mPosNode->getPosition()
				+ moveVect * len);
		}
	}

	// update player position with the chunk generator
	mGen->setPlayerPos(mCam->getPosition());


	// same o' same o'
	if(mInput->wasKeyPressed("KC_ESCAPE"))
		mEngine->shutdown();

	if(mInput->wasKeyPressed("KC_UP"))
		mBlockSelected = mBlockSelected % 6 + 1;
	if(mInput->wasKeyPressed("KC_DOWN"))
		mBlockSelected = mBlockSelected == 1 ? 6 : mBlockSelected - 1;

	// screenshots
	if(mInput->wasKeyPressed("KC_P"))
		mGfx->takeScreenshot(TimeManager::getPtr()->getTimestamp());

	// update debug overlay
	mFpsText->setCaption("FPS: "+ StringUtils::toString(1.f/delta));
	mActiveChunkCountText->setCaption("Active Chunks: " +
		StringUtils::toString(mGen->getNumActiveChunks()));
	mGeneratedChunkCountText->setCaption("Generated Chunks: " +
		StringUtils::toString(mGen->getNumGeneratedChunks()));
	mSelectionText->setCaption("Block: " + BLOCK_NAMES[mBlockSelected]);

	if(mInput->wasButtonPressed("MB_Right"))
	{
		if(mInput->isKeyDown("KC_L"))
		{
			RaycastReport r = mPhysics->raycast(mCam->getPosition(),mCam->getDirection(),
				50.f,COLLISION_GROUP_3,COLLISION_GROUP_3);

			if(r.hit && r.userData && r.group != COLLISION_GROUP_11)
			{
				BasicChunk* bc = static_cast<BasicChunk*>(r.userData);
				ChunkCoords cc = getBlockFromRaycast(r.position, 
					r.normal, bc, true);

				for(int i = -1; i <= 1; ++i)
					for(int j = -1; j <= 1; ++j)
						for(int k = -1; k <= 1; ++k)
				{
					ChunkCoords cc2 = cc + ChunkCoords(i,j,k);
					BasicChunk* bc2 = correctChunkCoords(bc, cc2);
					cc2.data = 0;
					bc2->changeBlock(cc2);
					if(mPortals[0]->inPortal(bc2, cc2, true))
						mPortals[0]->disable();
					if(mPortals[1]->inPortal(bc2, cc2, true))
						mPortals[1]->disable();
				}
			}
			else if(r.group == COLLISION_GROUP_11)
			{

			}
		}
		else if(!mInput->isKeyDown("KC_LSHIFT"))
		{
			RaycastReport r = mPhysics->raycast(mCam->getPosition(),mCam->getDirection(),
				8.f,COLLISION_GROUP_3,COLLISION_GROUP_3);

			if(r.hit && r.userData && r.group != COLLISION_GROUP_11)
			{
				BasicChunk* bc = static_cast<BasicChunk*>(r.userData);
				ChunkCoords cc = getBlockFromRaycast(r.position, 
					r.normal, bc, true);
				bc = correctChunkCoords(bc, cc);
				cc.data = 0;
				bc->changeBlock(cc);

				if(mPortals[0]->inPortal(bc, cc, true))
					mPortals[0]->disable();
				if(mPortals[1]->inPortal(bc, cc, true))
					mPortals[1]->disable();
			}
			else if(r.group == COLLISION_GROUP_11)
			{
			}
		}
		else
		{
			RaycastReport r = mPhysics->raycast(mCam->getPosition(),mCam->getDirection(),
				50.f,COLLISION_GROUP_3,COLLISION_GROUP_3);
			mPortals[1]->place(r);
		}
	}

	if(mInput->wasButtonPressed("MB_Left"))
	{
		if(!mInput->isKeyDown("KC_LSHIFT"))
		{
			RaycastReport r = mPhysics->raycast(mCam->getPosition(),mCam->getDirection(),
				5.f,COLLISION_GROUP_3,COLLISION_GROUP_3);

			if(r.hit && r.userData && r.group != COLLISION_GROUP_11)
			{
				BasicChunk* bc = static_cast<BasicChunk*>(r.userData);
				ChunkCoords cc = getBlockFromRaycast(r.position, r.normal, bc, false);
				bc = correctChunkCoords(bc, cc);
				cc.data = mBlockSelected;
				bc->changeBlock(cc);

				if(mPortals[0]->inPortal(bc, cc, false))
					mPortals[0]->disable();
				if(mPortals[1]->inPortal(bc, cc, false))
					mPortals[1]->disable();
			}
		}
		else
		{
			RaycastReport r = mPhysics->raycast(mCam->getPosition(),mCam->getDirection(),
				50.f,COLLISION_GROUP_3,COLLISION_GROUP_3);
			mPortals[0]->place(r);
		}
	}

	if(mPortals[0]->isEnabled() && mPortals[1]->isEnabled())
	{
		Chunk* cs[4] = {mPortals[0]->getChunk(0), mPortals[0]->getChunk(1),
			mPortals[1]->getChunk(0), mPortals[1]->getChunk(1)};
		ChunkCoords co[4] = {mPortals[0]->getCoords(0), mPortals[0]->getCoords(1),
			mPortals[1]->getCoords(0), mPortals[1]->getCoords(1)};
		mGen->setPortalInfo(cs, co);
		if(!mGfx->isCustomRenderSequenceEnabled())
			mGfx->enableCustomRenderSequence(&mRenderSequence);
		mUI->setHidden(true);
	}
	else
	{
		mGen->setPortalInfo();
		mGfx->disableCustomRenderSequence();
		mUI->setHidden(false);
	}
	
}
//---------------------------------------------------------------------------

void PlayState::deinit() {}
//---------------------------------------------------------------------------

void PlayState::updateCam(const Message& m)
{
	const MessageAny<int>* ms = message_cast<int>(m);

	int val = ms->data;
	int portal = (val - 1) / PORTAL_RENDER_DEPTH;
	int pass   = (val - 1) % PORTAL_RENDER_DEPTH;

	// first pass
	if(val == 0)
	{
		mPortals[0]->setVisible(true);
		mPortals[1]->setVisible(true);
		mGfx->setActiveCamera(mCam->mCamera);
		mUI->setHidden(true);
		mPortals[0]->update(0.f);
		mPortals[1]->update(0.f);
	}
	// after last pass
	else if(val == PORTAL_RENDER_DEPTH * 2 + 1)
	{
		mGfx->setActiveCamera(mCam->mCamera);
		mPortals[0]->setVisible(true);
		mPortals[1]->setVisible(true);
	}
	// regular passes
	else
	{
		if(pass == 0)
		{
			mGfx->setActiveCamera(mCam->mCamera);
			mPortals[portal]->update(0.f);
		}

		mPortals[portal]->setVisible(true);
		mPortals[!portal]->setVisible(false);
		mGfx->setActiveCamera(mPortals[portal]->getCamera());

		if(pass > 0)
			mPortals[portal]->recurse();
	}

	// render UI at last pass
	if(val == PORTAL_RENDER_DEPTH * 2)
		mUI->setHidden(false);
}
//---------------------------------------------------------------------------
