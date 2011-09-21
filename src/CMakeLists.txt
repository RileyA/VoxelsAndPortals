set(SUBSYSTEMS
	${OGRE_LIB}
	${AL_LIB}
	${OIS_LIB}
	${BULLET_LIB}
	${GUI_LIB}
	)

set(SOURCE_FILES
	BasicChunk.cpp
	Chunk.cpp
	ChunkGenerator.cpp
	BasicChunkGenerator.cpp
	ChunkManager.cpp
	PlayState.cpp
	TerrainChunk.cpp
	main.cpp
	FPSCamera.cpp
	Portal.cpp
	ThreadPool.cpp
	TerrainChunkGenerator.cpp
	TerrainChunk.cpp
	)

set(HEADER_FILES
	BasicChunk.h
	ChunkCoords.h
	ChunkGenerator.h
	BasicChunkGenerator.h
	Chunk.h
	ChunkManager.h
	ChunkOptions.h
	PlayState.h
	TerrainChunk.h
	FPSCamera.h
	Portal.h
	ThreadPool.h
	TerrainChunkGenerator.h
	TerrainChunk.h
	)

add_executable(TechDemo ${SOURCE_FILES} ${HEADER_FILES} )
set_target_properties(TechDemo PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries (TechDemo ${ORYX_LIB} ${BOOST_LIB} ${SUBSYSTEMS} libnoise.so)
