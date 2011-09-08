#ifndef ChunkManager_H
#define ChunkManager_H

class ChunkManager
{
public:

	ChunkManager();
	~ChunkManager();

	void init(Vector3 origin, ChunkGenerator* gen);

protected:

	ChunkGenerator* mGen;

};

#endif
