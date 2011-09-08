#ifndef ChunkGenerator_H
#define ChunkGenerator_H

/** Abstract Chunk generator for procedurally creating chunks */
class ChunkGenerator
{
public:

	ChunkGenerator();
	virtual ~ChunkGenerator();


protected:

	std::map<ChunkCoords, Chunk*> mChunks;

};

#endif
