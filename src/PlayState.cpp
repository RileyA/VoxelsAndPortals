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
	//srand(5);
	mGfx = dynamic_cast<OgreSubsystem*>(mEngine->getSubsystem("OgreSubsystem"));
	mAudio = dynamic_cast<ALSubsystem*>(mEngine->getSubsystem("ALSubsystem"));
	mInput = dynamic_cast<OISSubsystem*>(mEngine->getSubsystem("OISSubsystem"));
	mGUI = dynamic_cast<GUISubsystem*>(mEngine->getSubsystem("GUISubsystem"));
	mInput->initInput(mGfx->getWindowHandle(), true);

	mGfx->setBackgroundColor(Colour(0,0,0));

	Vector3 norm = Vector3(0,0,-1);
	norm.normalize();
	Vector3 norm2 = Vector3(0,0,1);
	norm2.normalize();
	mCam = new FPSCamera();

	port1 = new Portal(Vector3(-6.f,-7,-3.f), norm2, true);
	port2 = new Portal(Vector3(-6,-7,3), norm, false);

	port1->init();
	port2->init();

	port1->setSibling(port2);
	port2->setSibling(port1);


	BulletSubsystem* bt = Engine::getPtr()->getSubsystem("BulletSubsystem")->
		castType<BulletSubsystem>();
	bt->startSimulation();

	mChunkMgr = new ChunkManager();
	mGen = new BasicChunkGenerator();
	mGen->setPlayerPos(Vector3(0,0,0));
	mChunkMgr->init(Vector3(0,0,0), mGen);

	Batch* b = mGUI->createBatch("test", "TechDemo.oyster");
	
	Caption* c = new Caption(b, 0);
	c->setCaption("Worker threads: 6");
	c->setPosition(Vector2(0.01f, 0.94f));

	c = new Caption(b, 0);
	c->setCaption("FPS: 60");
	c->setPosition(Vector2(0.01f, 0.74f));
	fps = c;

	c = new Caption(b, 0);
	c->setCaption("Batches: 0");
	c->setPosition(Vector2(0.01f, 0.79f));
	batchCount = c;

	c = new Caption(b, 0);
	c->setCaption("Generated Chunks: 125");
	c->setPosition(Vector2(0.01f, 0.84f));
	gcCount = c;

	c = new Caption(b, 0);
	c->setCaption("Active Chunks: 27");
	c->setPosition(Vector2(0.01f, 0.89f));
	acCount = c;

	c = new Caption(b, 0);
	c->setCaption("Block: 1");
	c->setPosition(Vector2(0.8f, 0.94f));
	selection = c;
	block = 1;

	ScreenMesh* s = mGfx->createScreenMesh("UITEST");
	b->getSignal("update")->addListener(s->getSlot("update"));

	//portalcam1->setPosition(Vector3(-4,-6,-6));
	//portalcam1->setDirection(Vector3(0,0,1));
}

void PlayState::update(Real delta)
{
	if(mInput->wasKeyPressed("KC_ESCAPE"))
		mEngine->shutdown();

	if(mInput->wasKeyPressed("KC_P"))
		mGfx->takeScreenshot();

	fps->setCaption("FPS: "+ StringUtils::toString(1.f/delta));
	batchCount->setCaption("Batches: " + StringUtils::toString(mGfx->getBatchCount()));
	acCount->setCaption("Active Chunks: " + StringUtils::toString(mGen->numActiveChunks));
	gcCount->setCaption("Generated Chunks: " + StringUtils::toString(mGen->numGeneratedChunks));

	//mCam->mCamera->hackityHack(port1->getCamera(), "Portal1", 1);
	//mCam->mCamera->hackityHack(port1->getCamera(), "Portal1", 2);
	port1->getCamera()->hackityHack(mCam->mCamera, "Portal1", 2);
	//port1->getCamera()->hackityHack(mCam->mCamera, "Portal1", 2);

	mCam->mCamera->hackityHack(port2->getCamera(), "Portal2", 1);
	//mCam->mCamera->hackityHack(port2->getCamera(), "Portal2", 2);

	/*port2->getCamera()->hackityHack(mCam->mCamera, "Portal1", 1);
	port2->getCamera()->hackityHack(mCam->mCamera, "Portal1", 2);
	port1->getCamera()->hackityHack(mCam->mCamera, "Portal2", 1);
	port1->getCamera()->hackityHack(mCam->mCamera, "Portal2", 2);*/
	/*port1->getCamera()->hackityHack(port2->getCamera(), "Portal2", 1);
	port1->getCamera()->hackityHack(port2->getCamera(), "Portal2", 2);
	port2->getCamera()->hackityHack(port1->getCamera(), "Portal1", 1);
	port2->getCamera()->hackityHack(port1->getCamera(), "Portal1", 2);*/

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

			/*for(int i = -2; i < 3; ++i)
				for(int j = -2; j < 3; ++j)
					for(int k = -2; k < 3; ++k)
			{
				ChunkCoords cc2 = cc + ChunkCoords(i,j,k);
				BasicChunk* bbc = correctChunkCoords(bc, cc2);
				cc2.c.data = 0;
				bbc->changeBlock(cc2);
			}*/
		}
	}
	if(mInput->wasButtonPressed("MB_Left"))
	{
		BulletSubsystem* b = Engine::getPtr()->getSubsystem("BulletSubsystem")->
			castType<BulletSubsystem>();
		
		RaycastReport r = b->raycast(mCam->getPosition(),mCam->getDirection(),
			5.f,COLLISION_GROUP_3,COLLISION_GROUP_3);

		if(r.hit && r.userData)
		{
			BasicChunk* bc = static_cast<BasicChunk*>(r.userData);
			ChunkCoords cc = BasicChunk::getBlockFromRaycast(r.position, r.normal, bc, false);
			bc = correctChunkCoords(bc, cc);
			cc.c.data = block;
			bc->changeBlock(cc);
		}
	}

	if(mInput->wasKeyPressed("KC_K"))
	{
		mGfx->takeScreenshot("P1_1", port1->rtt1);
		mGfx->takeScreenshot("P1_2", port1->rtt2);
		mGfx->takeScreenshot("P2_1", port2->rtt1);
		mGfx->takeScreenshot("P2_2", port2->rtt2);
		mGfx->takeScreenshot("P5");
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
