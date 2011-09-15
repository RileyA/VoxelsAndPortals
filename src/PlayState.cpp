#include "Common.h"
#include "PlayState.h"
#include "BasicChunkGenerator.h"
#include "BasicChunk.h"
#include "ChunkUtils.h"
#include "FPSCamera.h"
#include "Portal.h"


PlayState::PlayState()
	:mChunkMgr(0), mGfx(0), mAudio(0), mGUI(0), mInput(0)
{
	// ...
}

PlayState::~PlayState()
{
}

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
	mGfx->setBackgroundColor(Colour(0,0,0));

	// enable portal stencil hack with 5 visual recursions
	mGfx->enablePortalHack(5);
	
	// so we can get signals during portal rendering (for setting up camera, visibility, etc)
	mGfx->getSignal("updateCam")->addListener(createSlot("updateCam", this, &PlayState::updateCam));

	// standard FPS-style camera (no character controller just yet)
	mCam = new FPSCamera();

	// make the portals
	port1 = new Portal(Vector3(-6.5f,-7,-3.f), Vector3(1,0,0), true);
	port2 = new Portal(Vector3(-6.5f,-7,3), Vector3(0,0,1), false);

	// connect them (eventually I'd like to allow any number of portal pairs, but for now
	// everythings very hacky and hardcoded to do two portals...
	port1->setSibling(port2);
	port2->setSibling(port1);

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
	fps = c;

	c = new Caption(b, 0);
	c->setCaption("Batches: 0");
	c->setPosition(Vector2(0.01f, 0.74f));
	batchCount = c;

	c = new Caption(b, 0);
	c->setCaption("Generated Chunks: 125");
	c->setPosition(Vector2(0.01f, 0.79f));
	gcCount = c;

	c = new Caption(b, 0);
	c->setCaption("Active Chunks: 27");
	c->setPosition(Vector2(0.01f, 0.84f));
	acCount = c;

	c = new Caption(b, 0);
	c->setCaption("Block: 1");
	c->setPosition(Vector2(0.8f, 0.94f));
	selection = c;
	block = 1;

	// hook it up with the rendering system
	mUI = mGfx->createScreenMesh("UITEST");
	b->getSignal("update")->addListener(mUI->getSlot("update"));
	mUI->setHidden(false);
}

void PlayState::update(Real delta)
{
	// same o' same o'
	if(mInput->wasKeyPressed("KC_ESCAPE"))
		mEngine->shutdown();

	// screenshots
	if(mInput->wasKeyPressed("KC_P"))
		mGfx->takeScreenshot();

	// update debug overlay
	fps->setCaption("FPS: "+ StringUtils::toString(1.f/delta));
	batchCount->setCaption("Batches: " + StringUtils::toString(mGfx->getBatchCount()));
	acCount->setCaption("Active Chunks: " + StringUtils::toString(mGen->numActiveChunks));
	gcCount->setCaption("Generated Chunks: " + StringUtils::toString(mGen->numGeneratedChunks));

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
			ChunkCoords cc = BasicChunk::getBlockFromRaycast(r.position, r.normal, bc, true);
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
			cc.c.data = block;
			bc->changeBlock(cc);
		}

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
			
			port1->setPosition(adjPos);
			port1->setDirection(r.normal);

			//cc.c.data = block;
			//bc->changeBlock(cc);
		}*/

	}

	if(mInput->wasKeyPressed("KC_Q"))
	{
		if(block & 1)
		{
			Vector3 nn = Vector3(1.f,0.f,0.0);
			nn.normalize();
			port2->setDirection(nn);

			Vector3 nn2 = Vector3(0,0,1);
			nn2.normalize();
			port1->setDirection(nn2);
		}
		else
		{
			Vector3 nn = Vector3(1,0,-1);
			nn.normalize();
			port2->setDirection(nn);

			Vector3 nn2 = Vector3(0,0,1);
			nn2.normalize();
			port1->setDirection(nn2);
		}

		++block;
		if(block >= 6)
			block = 1;
		selection->setCaption("Block: " + StringUtils::toString(block));
	}

	mGen->setPlayerPos(mCam->getPosition());
}

void PlayState::deinit()
{
	// .. 
}


void PlayState::updateCam(const Message& m)
{
	const MessageAny<std::pair<int, int> >* ms = message_cast<std::pair<int, int> >(m);
	//std::cout<<ms->data<<"\n";
	int portal_ = ms->data.first;
	int pass_ = ms->data.second;

	if(portal_ == 0)
	{
		port1->setVisible(true);
		port2->setVisible(true);
		mGfx->setActiveCamera(mCam->mCamera);
		mUI->setHidden(false);
	}
	else if(portal_ == 1)
	{
		port1->setVisible(true);
		port2->setVisible(false);
		mGfx->setActiveCamera(port1->getCamera());

		if(pass_ > 0)
			port1->recurse();
	}
	else if(portal_ == 2)
	{
		port1->setVisible(false);
		port2->setVisible(true);
		mGfx->setActiveCamera(port2->getCamera());

		if(pass_ > 0)
			port2->recurse();
		// render UI at the end of the last render
		if(pass_ == 4)
			mUI->setHidden(true);
	}
	else if(portal_ == 100)
	{
		mGfx->setActiveCamera(mCam->mCamera);
		port1->setVisible(true);
		port2->setVisible(true);
	}
}
