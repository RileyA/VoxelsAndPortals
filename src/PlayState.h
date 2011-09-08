#include "Common.h"
#include "OryxGameState.h"
#include "ChunkManager.h"

class PlayState
{
public:

	PlayState();
	virtual ~PlayState();

	void update(Real delta);
	void init();
	void deinit();

private:

	ChunkManager* mChunkMgr;

};
