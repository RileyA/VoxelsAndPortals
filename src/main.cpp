#include "Common.h"
#include "PlayState.h"

int main(int argc, char** argv)
{
	Logger::getPtr();
	TimeManager::getPtr();

	// create subsystems:
	OgreSubsystem ogre(800,600,false);
	OISSubsystem ois;
	BulletSubsystem bull;
	GUISubsystem gui(800,600);
	ALSubsystem aSys;

	// allocate engine and add subsystems
	Engine* eng = new Engine();
	eng->addSubsystem(&aSys);
	eng->addSubsystem(&ogre);
	eng->addSubsystem(&ois);
	eng->addSubsystem(&bull);
	eng->addSubsystem(&gui);

	// initialize the engine
	eng->init();

	// add game state
	eng->addState(new PlayState());

	// start up the engine
	eng->start();

	// delete the engine object
	delete eng;
	return 0;
}
