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
	mInput->initInput(mGfx->getWindowHandle(), true);

	// start up bullet for collision detection
	mPhysics->startSimulation();

	// a dark background color helps hide momentary gaps, that may occur when blocks are
	// changed on edges, and one chunk updates before the other
	Colour col = Colour(255/255.f,200/255.f,158/255.f);
	Colour col2 = Colour(211/255.f,234/255.f,243/255.f);
	mGfx->setBackgroundColor(col2);
	mGfx->setLinearFog(30.f,60.f,col);

	// Setup portal render sequence:

	// initial pass
	CustomRenderIteration& basePass = mRenderSequence.addIteration();

	// first pass, the framebuffer is already clear
	basePass.clearDepth = false;

	// always pass
	basePass.addStencilConfig(-1, CMPF_EQUAL, 0, 0xFFFFFFFF, 
		SOP_ZERO, SOP_ZERO, SOP_ZERO);

	// make the portals write to the stencil buffer
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

			// base stencil settings
			portal_pass.addStencilConfig(-1, CMPF_EQUAL, 1 + i + 50 * p,
				0xFFFFFFFF, SOP_KEEP, SOP_KEEP, SOP_KEEP);

			// portal-specific render queue settings
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

	// quick and ugly hack for light through portals
	mPortals[0]->chunks[0] = 0;
	mPortals[0]->chunks[1] = 0;
	mPortals[1]->chunks[0] = 0;
	mPortals[1]->chunks[1] = 0;
	mPortals[0]->lightVals[0] = 0;
	mPortals[0]->lightVals[1] = 0;
	mPortals[1]->lightVals[0] = 0;
	mPortals[1]->lightVals[1] = 0;
	mPortals[0]->placed = 0;
	mPortals[1]->placed = 0;

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
	c->setCaption("Worker threads: 6");
	c->setPosition(Vector2(0.01f, 0.94f));

	c = new Caption(b, 0);
	c->setCaption("Portal Depth: 3");
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

	mBlockSelected = 2;

	// hook it up with the rendering system
	mUI = mGfx->createScreenMesh("UITEST");
	b->getSignal("update")->addListener(mUI->getSlot("update"));
	mUI->setHidden(false);

	//mCam->slerpTime = -10.f;
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

			//mCam->up1 = p->upv;
			//mCam->up2 = p->mSibling->upv;
			//mCam->slerpTime = 1.f;
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

	// screenshots
	if(mInput->wasKeyPressed("KC_P"))
		mGfx->takeScreenshot(TimeManager::getPtr()->getTimestamp());

	// update debug overlay
	mFpsText->setCaption("FPS: "+ StringUtils::toString(1.f/delta));
	mBatchCountText->setCaption("Batches: " 
		+ StringUtils::toString(mGfx->getBatchCount()));
	mActiveChunkCountText->setCaption("Active Chunks: " +
		StringUtils::toString(mGen->getNumActiveChunks()));
	mGeneratedChunkCountText->setCaption("Generated Chunks: " +
		StringUtils::toString(mGen->getNumGeneratedChunks()));

	if(mInput->wasButtonPressed("MB_Right"))
	{
		if(mInput->isKeyDown("KC_L"))
		{
			RaycastReport r = mPhysics->raycast(mCam->getPosition(),mCam->getDirection(),
				50.f,COLLISION_GROUP_3,COLLISION_GROUP_3);

			if(r.hit && r.userData)
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
				}
			}
		}
		else if(!mInput->isKeyDown("KC_LSHIFT"))
		{
			RaycastReport r = mPhysics->raycast(mCam->getPosition(),mCam->getDirection(),
				8.f,COLLISION_GROUP_3,COLLISION_GROUP_3);

			if(r.hit && r.userData)
			{
				BasicChunk* bc = static_cast<BasicChunk*>(r.userData);
				ChunkCoords cc = getBlockFromRaycast(r.position, 
					r.normal, bc, true);
				bc = correctChunkCoords(bc, cc);
				cc.data = 0;
				bc->changeBlock(cc);
			}
		}
		else
		{
			RaycastReport r = mPhysics->raycast(mCam->getPosition(),mCam->getDirection(),
				50.f,COLLISION_GROUP_3,COLLISION_GROUP_3);

			// YUCK
			if(r.hit && r.userData)
			{
				BasicChunk* bc = static_cast<BasicChunk*>(r.userData);
				ChunkCoords cc = getBlockFromRaycast(r.position, r.normal, bc, false);

				bc = correctChunkCoords(bc, cc);

				BlockDirection d = getBlockDirectionFromVector(r.normal);
				BlockDirection d_ = getBlockDirectionFromVector(r.normal * -1);
				BlockDirection up = getBlockDirectionFromVector(
					Plane(r.normal, 0).projectVector(mCam->mCamera->getAbsoluteUp()));

				ChunkCoords cc2 = cc << up;
				BasicChunk* bc2 = correctChunkCoords(bc, cc2);

				if(!getBlockVal(bc, cc) && !getBlockVal(bc2, cc2) 
					&& getBlockVal(bc, cc << d_) && getBlockVal(bc2, cc2 << d_))
				{
					Vector3 adjPos = bc->getPosition() - OFFSET + Vector3(cc.x, cc.y, cc.z);
					adjPos -= BLOCK_NORMALS[d] * 0.5f;
					adjPos += BLOCK_NORMALS[up] * 0.5f;

					for(int i = 0; i < 2; ++i)
					{
						if(mPortals[1]->chunks[0])
							mPortals[1]->chunks[0]->clearLights();
					}

					mPortals[1]->lightVals[0] = getLightVal(bc, cc);
					mPortals[1]->lightVals[1] = getLightVal(bc2, cc2);

					mPortals[1]->cc[0] = cc;
					mPortals[1]->cc[1] = cc2;

					mPortals[1]->chunks[0] = bc;

					if(bc2 != bc)
					{
						mPortals[1]->chunks[1] = bc2;
					}
					else
					{
						mPortals[1]->chunks[1] = 0;
					}

					mPortals[1]->placed = true;

					if(mPortals[0]->placed && mPortals[1]->placed)
					{
						for(int i = 0; i < 2; ++i)
						{
							if(mPortals[0]->lightVals[i] < mPortals[1]->lightVals[i])
							{
								BasicChunk* tmp = mPortals[0]->chunks[i] ? mPortals[0]->chunks[i] : 
									mPortals[0]->chunks[0];
								tmp->addLight(mPortals[0]->cc[i], mPortals[1]->lightVals[i]);
							}
							else
							{
								BasicChunk* tmp = mPortals[1]->chunks[i] ? mPortals[1]->chunks[i] : 
									mPortals[1]->chunks[0];
								tmp->addLight(mPortals[1]->cc[i], mPortals[0]->lightVals[i]);
							}
						}
					}

					mPortals[1]->setPosition(adjPos);
					mPortals[1]->setDirection(d, up);
				}
			}
		}
	}

	if(mInput->wasButtonPressed("MB_Left"))
	{
		if(!mInput->isKeyDown("KC_LSHIFT"))
		{
			RaycastReport r = mPhysics->raycast(mCam->getPosition(),mCam->getDirection(),
				5.f,COLLISION_GROUP_3,COLLISION_GROUP_3);

			if(r.hit && r.userData)
			{
				BasicChunk* bc = static_cast<BasicChunk*>(r.userData);
				ChunkCoords cc = getBlockFromRaycast(r.position, r.normal, bc, false);
				bc = correctChunkCoords(bc, cc);
				cc.data = mBlockSelected;
				bc->changeBlock(cc);
			}
		}
		else
		{
			RaycastReport r = mPhysics->raycast(mCam->getPosition(),mCam->getDirection(),
				50.f,COLLISION_GROUP_3,COLLISION_GROUP_3);

			// YUCK
			if(r.hit && r.userData)
			{
				BasicChunk* bc = static_cast<BasicChunk*>(r.userData);
				ChunkCoords cc = getBlockFromRaycast(r.position, r.normal, bc, false);

				bc = correctChunkCoords(bc, cc);

				BlockDirection d = getBlockDirectionFromVector(r.normal);
				BlockDirection d_ = getBlockDirectionFromVector(r.normal * -1);
				BlockDirection up = getBlockDirectionFromVector(
					Plane(r.normal, 0).projectVector(mCam->mCamera->getAbsoluteUp()));

				ChunkCoords cc2 = cc << up;
				BasicChunk* bc2 = correctChunkCoords(bc, cc2);

				if(!getBlockVal(bc, cc) && !getBlockVal(bc2, cc2) 
					&& getBlockVal(bc, cc << d_) && getBlockVal(bc2, cc2 << d_))
				{
					Vector3 adjPos = bc->getPosition() - OFFSET + Vector3(cc.x, cc.y, cc.z);
					adjPos -= BLOCK_NORMALS[d] * 0.5f;
					adjPos += BLOCK_NORMALS[up] * 0.5f;

					for(int i = 0; i < 2; ++i)
					{
						if(mPortals[0]->chunks[0])
							mPortals[0]->chunks[0]->clearLights();
					}

					mPortals[0]->lightVals[0] = getLightVal(bc, cc);
					mPortals[0]->lightVals[1] = getLightVal(bc2, cc2);

					mPortals[0]->cc[0] = cc;
					mPortals[0]->cc[1] = cc2;

					mPortals[0]->chunks[0] = bc;

					if(bc2 != bc)
					{
						mPortals[0]->chunks[1] = bc2;

					}
					else
					{
						mPortals[0]->chunks[1] = 0;
					}

					mPortals[0]->placed = true;

					if(mPortals[0]->placed && mPortals[1]->placed)
					{
						for(int i = 0; i < 2; ++i)
						{
							if(mPortals[0]->lightVals[i] < mPortals[1]->lightVals[i])
							{
								BasicChunk* tmp = mPortals[0]->chunks[i] ? mPortals[0]->chunks[i] : 
									mPortals[0]->chunks[0];
								tmp->addLight(mPortals[0]->cc[i], mPortals[1]->lightVals[i]);
							}
							else
							{
								BasicChunk* tmp = mPortals[1]->chunks[i] ? mPortals[1]->chunks[i] : 
									mPortals[1]->chunks[0];
								tmp->addLight(mPortals[1]->cc[i], mPortals[0]->lightVals[i]);
							}
						}
					}

					mPortals[0]->setPosition(adjPos);
					mPortals[0]->setDirection(d, up);
				}
			}
		}
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

	if(val == 0)
	{
		mPortals[0]->setVisible(true);
		mPortals[1]->setVisible(true);
		mGfx->setActiveCamera(mCam->mCamera);
		mUI->setHidden(false);
		mPortals[0]->update(0.f);
		mPortals[1]->update(0.f);
	}
	else if(val == PORTAL_RENDER_DEPTH * 2 + 1)
	{
		mGfx->setActiveCamera(mCam->mCamera);
		mPortals[0]->setVisible(true);
		mPortals[1]->setVisible(true);
	}
	else if(portal == 0)
	{
		if(pass == 0)
		{
			mGfx->setActiveCamera(mCam->mCamera);
			mPortals[0]->update(0.f);
		}
			
		mPortals[0]->setVisible(true);
		mPortals[1]->setVisible(false);
		mGfx->setActiveCamera(mPortals[0]->getCamera());

		if(pass > 0)
			mPortals[0]->recurse();
	}
	else if(portal == 1)
	{
		if(pass == 0)
		{
			mGfx->setActiveCamera(mCam->mCamera);
			mPortals[1]->update(0.f);
		}

		mPortals[0]->setVisible(false);
		mPortals[1]->setVisible(true);
		mGfx->setActiveCamera(mPortals[1]->getCamera());

		if(pass > 0)
			mPortals[1]->recurse();

		// render UI at the end of the last render
		if(pass == PORTAL_RENDER_DEPTH - 1)
			mUI->setHidden(true);
	}
}
//---------------------------------------------------------------------------
