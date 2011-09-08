#ifndef ChunkOptions_H
#define ChunkOptions_H

#include "Oryx.h"
#include "Oryx3DMath.h"
#include "OryxColour.h"

// A bunch of random constants

// Default chunk size
const static unsigned char CHUNK_SIZE_X = 16;
const static unsigned char CHUNK_SIZE_Y = 16;
const static unsigned char CHUNK_SIZE_Z = 16;
const static unsigned char CHUNK_SIZE[3] = 
	{CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z};

// volume of a chunk
const static unsigned short CHUNK_VOLUME = 
	CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;

// The maximum light value
const byte MAX_LIGHT = 15;

// Offset used for centering Chunk meshes
static const Vector3 OFFSET = Vector3((CHUNKSIZE[0]-1)/2.f,
		(CHUNKSIZE[1]-1)/2.f,(CHUNKSIZE[2]-1)/2.f);

// Add a little shading depending on the direction of each face
const static Real LIGHT_STEPS[6] = {0.6f,0.6f,0.5f,0.99f,0.8f,0.7f};

// Normals for the quads (more effiecient to store them here, than to calculate each time...
const static Vector3 QUADNORMALS[6] = {Vector3(1,0,0),Vector3(-1,0,0),Vector3(0,1,0),
	Vector3(0,-1,0),Vector3(0,0,1),Vector3(0,0,-1)};

// The light values 0.8^(15-lightLevel), saves some pow()-ing to store these as consts
static const Real LIGHTVALUES[16] = {
	0.03518f,
	0.04398f,
	0.0549755f,
	0.06871948f,
	0.08589f,
	0.10737f,
	0.1342f,
	0.16777f,
	0.2097f,
	0.262144f,
	0.32768f,
	0.4096f,
	0.512f,
	0.64f,
	0.8f,
	1.f
};

// for convenience
const static byte AXIS[6] = {0,0,1,1,2,2};
const static int8 AXIS_OFFSET[6] = {-1,1,-1,1,-1,1};

// directions used in smooth lighting calculations
const static int8 LIGHTING_COORDS[6][4][2] = {
	{{5,2},{5,3},{4,3},{4,2}},
	{{4,2},{4,3},{5,3},{5,2}},
	{{0,4},{4,1},{5,1},{5,0}},
	{{0,5},{5,1},{4,1},{4,0}},
	{{0,3},{1,3},{1,2},{0,2}},
	{{0,2},{1,2},{1,3},{0,3}}
};

// enumerated directions
enum BlockDirection
{
	BD_LEFT,  // -x
	BD_RIGHT, // +x
	BD_DOWN,  // -y
	BD_UP,    // +y
	BD_FRONT, // -z
	BD_BACK   // +z
};

#endif
