#ifndef TerrainChunkGenerator_H
#define TerrainChunkGenerator_H

#include "BasicChunkGenerator.h"
#include "TerrainChunk.h"

namespace noise
{
	namespace module
	{
		class Perlin;
		class Billow;
		class RidgedMulti;
	}
}

/** A specialization for doing Minecraft-y stuff (generating on 2 dimensions only,
 * light from the sky, 16x128x16 chunks...) */
class TerrainChunkGenerator : public BasicChunkGenerator
{
public:

	TerrainChunkGenerator();
	virtual ~TerrainChunkGenerator();

protected:

	/** Generates any needed chunks */
	virtual void generate();

	/** Activates any needed chunks */
	virtual void activate();

	/** Apply changes to chunks */
	virtual void apply();

	noise::module::Perlin* mPerlin;
	noise::module::RidgedMulti* mRidged;
	noise::module::Billow* mBillow;

	// a list of the currently active chunks
	std::list<BasicChunk*> mActiveChunks;

};

#endif
