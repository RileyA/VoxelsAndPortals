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

	//mCam->mPosNode->setPosition(Vector3(0,0,1));

	//mCam->mCamera->setPosition(Vector3(0,0,0));

	/*portalcam1 = mGfx->createCamera();
	portalcam2 = mGfx->createCamera();

	pn1 = mGfx->createSceneNode();
	pn2 = mGfx->createSceneNode();

	pn1->addChild(portalcam1);
	pn2->addChild(portalcam2);

	portalcam1->setPosition(Vector3(0,0,0));
	portalcam2->setPosition(Vector3(0,0,0));

	pn1->setOrientation(Quaternion::IDENTITY);
	pn2->setOrientation(Quaternion::IDENTITY);

	//pn1->yaw(180.f);
	pn2->yaw(180.f);

	portalcam1->setDirection(Vector3(0,0,1));
	portalcam2->setDirection(Vector3(0,0,1));

	mGfx->createRTT("blargh", portalcam1, 1024, 1024);
	mGfx->createRTT("blargh2", portalcam2, 1024, 1024);
	portalcam1->setAspectRatio(1.3333f);
	portalcam2->setAspectRatio(1.3333f);
	//portalcam1->setCustomNearClip(Vector3(0,0,1), -8);
	//portalcam2->setCustomNearClip(Vector3(0,0,1), 0);

	Mesh* m = mGfx->createMesh("Portal.mesh");
	//m->setScale(Vector3(1,1,-1));
	Mesh* m2 = mGfx->createMesh("Portal2.mesh");
	mGfx->getRootSceneNode()->addChild(m);
	mGfx->getRootSceneNode()->addChild(m2);
	m->setPosition(Vector3(-4.f,-7.f,0));
	//m2->yaw(-90.f);
	m2->setPosition(Vector3(4.f,-7.f,-8));
	m2->setMaterialName("Portal2");*/

	Vector3 norm = Vector3(0,0,-1);
	norm.normalize();
	Vector3 norm2 = Vector3(0,0,-1);
	norm2.normalize();

	port1 = new Portal(Vector3(-4.f,-7,0.f), norm2, true);
	port2 = new Portal(Vector3(4.5f,-7,0.f), norm, false);

	port1->setSibling(port2);
	port2->setSibling(port1);

	mCam = new FPSCamera();

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

	/*Vector3 pos = mCam->getPosition();// + Vector3(0,0,16);
	Vector3 dir = mCam->getDirection();
	//pos = pos - Vector3(0,0,-1) * 2 * pos.dotProduct(Vector3(0,0,-1));
	//Vector3 refd = dir - Vector3(0,0,-1) * 2 * dir.dotProduct(Vector3(0,0,-1));
	portalcam1->setPosition(pos);
	portalcam1->setDirection(dir);
	portalcam1->enableReflection(Vector3(0,0,1), -8.f);
	portalcam1->hackityHack("Portal1");
	dir.y *= -1;

	//dir.x *= -1;
	portalcam2->setPosition(pos - Vector3(1.f,0.f,-8.f));
	portalcam2->setDirection(dir * -1);
	//pos.y *= -1;
	//dir.x *= -1;
	//dir.z *= -1;
	//portalcam1->setPosition(pos - Vector3(1.f,0.f,-8.f));
	//portalcam1->setDirection(dir * -1);
	//portalcam1->enableReflection(Vector3(0,0,1));

	//portalcam2->hackityHack("Portal2");
	//mCam->mCamera->hackityHack("Portal1");
	mCam->mCamera->hackityHack("Portal2");*/

	//std::cout<<"p1: "<<portalcam1->getAbsolutePosition().x<<" "<<portalcam1->getAbsolutePosition().y<<" "<<
	//	portalcam1->getAbsolutePosition().z<<"\n";
	
	//std::cout<<"cam: "<<mCam->mCamera->getAbsoluteDirection().x<<" "<<mCam->mCamera->getAbsoluteDirection().y<<" "<<
	//	mCam->mCamera->getAbsoluteDirection().z<<"\n";
	//std::cout<<"p2: "<<portalcam2->getAbsolutePosition().x<<" "<<portalcam2->getAbsolutePosition().y<<" "<<
	//	portalcam2->getAbsolutePosition().z<<"\n";

	/*Vector3 portd = mCam->getPosition() * -1;
	portd.y = 0;
	portd.normalize();
	port1->setDirection(portd);*/

	//portalcam2->enableReflection(Vector3(0,0,1));

	//portalCam->enableReflection();
	//portalcam1->setNearClip(fabs(pos.z) + 0.1f);
	//portalcam2->setNearClip(fabs(pos.z) + 0.1f);
	//portalcam1->setFarClip(fabs(ref.z) * 100);
	/*portalcam1->extentsHack(
		(-1 - ref.x), 
		(1 - ref.x), 
		(1 - ref.y), 
		(-1 - ref.y));*/
	//portalcam1->setDirection(pos * -1);

	//pos = mCam->getPosition();
	//ref = pos - Vector3(0,0,-1) * 2 * pos.dotProduct(Vector3(0,0,-1));

	//portalcam2->setPosition(ref);
	//portalcam2->setDirection((pos + Vector3(0,0,-2))*-1);

	//mCam->mCamera->hackityHack("Portal1");
	//mCam->mCamera->hackityHack("Portal2");

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

	if(mInput->wasKeyPressed("KC_Q"))
	{
		if(block & 1)
		{
			std::cout<<"EXACT +Z\n";
			Vector3 nn = Vector3(1.f,0.f,0.0);
			nn.normalize();
			port2->setDirection(nn);

			Vector3 nn2 = Vector3(0,0,1);
			nn2.normalize();
			port1->setDirection(nn2);
		}
		else
		{
			std::cout<<"OFFSETS -Z\n";
			Vector3 nn = Vector3(-1,0,1);
			nn.normalize();
			port2->setDirection(nn);

			Vector3 nn2 = Vector3(1,0,0);
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
