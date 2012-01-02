#ifndef ChunkOptions_H
#define ChunkOptions_H

#include "Oryx.h"
#include "Oryx3DMath.h"
#include "OryxColour.h"

// A bunch of random constants

// #define BLOCK_NORMALS
#define SMOOTH_LIGHTING

// chunk genertion options
const static int ACTIVE_CHUNK_DISTANCE    = 6;
const static int DEACTIVATE_CHUNK_DISTANCE= 8;
const static int GENERATE_CHUNK_DISTANCE  = 8;


const static int GENERATOR_WORKER_THREADS = 6;

// Default chunk size
const static unsigned char CHUNK_SIZE_X = 16;
const static unsigned char CHUNK_SIZE_Y = 127;
const static unsigned char CHUNK_SIZE_Z = 16;
const static unsigned char CHUNKSIZE[3] = 
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
//const static Real LIGHT_STEPS[6] = {0.9f,0.9f,0.9f,0.9f,0.9f,0.9f};

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
const static int8 AXIS_INVERT[6] = {1,0,3,2,5,4};

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

static BlockDirection getBlockDirectionFromVector(Vector3 v)
{
	// extract dominant axis
	Real x,y,z;
	x = fabs(v.x);
	y = fabs(v.y);
	z = fabs(v.z);

	if(x >= y && x >= z)
		return v.x > 0 ? BD_RIGHT : BD_LEFT;
	else if(y >= x && y >= z)
		return v.y > 0 ? BD_UP : BD_DOWN;
	else
		return v.z > 0 ? BD_BACK : BD_FRONT;
};

// the first 4 bits are light amt if emissive, or translucency level if transparent
// this makes the two mutually exclusive, which isn't ideal, but ah well
enum BlockProperties
{
	BP_SOLID = 1<<4,      // can be drawn as a block
	BP_TRANSPARENT = 1<<5,// can be seen through
	BP_SOMETHING = 1<<6,  // I dunno yet
	BP_EMISSIVE = 1<<7    // emits light
};

const byte FILTERVERTEX[6] = {0,3,1,3,2,1};

const byte NUM_BLOCKTYPES = 17;

const byte MAPPINGS[NUM_BLOCKTYPES][6] = 
{
	// Minecraft "terrain.png":
	{1,1,1,1,1,1},// air
	{2,2,2,2,2,2}, // stone
	{4,4,3,1,4,4}, // grass
	{3,3,3,3,3,3}, // dirt
	{17,17,17,17,17,17}, // cobble
	{5,5,5,5,5,5}, // planks
	{16,16,16,16,16,16}, // sapling
	{18,18,18,18,18,18}, // bedrock
	{206,206,206,206,206,206}, // water
	//{207,207,207,207,207,207}, // flowing water
	{238,238,238,238,238,238}, // lava
	//{239,239,239,239,239,239}, // flowing lava
	{19,19,19,19,19,19},// sand
	{20,20,20,20,20,20},// gravel
	{33,33,33,33,33,33},// gold
	{34,34,34,34,34,34},// iron
	{35,35,35,35,35,35},// coal ore
	{21,21,22,22,21,21},// tree trunk
	{53,53,53,53,53,53}// shrub
};

enum BlockType
{
	BT_AIR,
	BT_STONE,
	BT_GRASS,
	BT_DIRT,
	BT_COBBLESTONE,
	BT_PLANKS,
	BT_SAPLING,
	BT_BEDROCK,
	BT_WATER,
	//BT_FLOWING_WATER,
	BT_LAVA,
	//BT_FLOWING_LAVA,
	BT_SAND,
	BT_GRAVEL,
	BT_GOLD_ORE,
	BT_IRON_ORE,
	BT_COAL_ORE,
	BT_WOOD,
	BT_LEAVES
};

const String BLOCK_NAMES[] = 
{
	"Air",
	"Stone",
	"Grass",
	"Dirt",
	"Cobblestone",
	"Planks",
	"Sapling",
	"Bedrock",
	"Water",
	//"Flowing Water",
	"Lava",
	//"Flowing Lava",
	"Sand",
	"Gravel",
	"Gold Ore",
	"Iron Ore",
	"Coal Ore",
	"Wood",
	"Leaves"
};

const byte BLOCKTYPES[] = 
{
	BP_TRANSPARENT, //air
	BP_SOLID,//stone
	BP_SOLID, //grass
	BP_SOLID, //dirt
	BP_SOLID, //cobble
	BP_SOLID, //planks
	BP_SOLID | BP_TRANSPARENT, //sapling
	BP_SOLID, //bedrock
	BP_SOLID | BP_TRANSPARENT | 2, //water
	//BP_SOLID | BP_TRANSPARENT | 2, //flowing water
	BP_SOLID | BP_EMISSIVE | 15, //lava
	//BP_SOLID | BP_EMISSIVE | 15, //flowing lava
	BP_SOLID, //sand
	BP_SOLID, //gravel
	BP_SOLID, //gold
	BP_SOLID, //iron
	BP_SOLID, //coal
	BP_SOLID, //log
	BP_SOLID | BP_TRANSPARENT //leaves
};

const Vector3 BLOCK_VERTICES[6][6] =
{
	{Vector3(-0.5f,0.5f,0.5f),Vector3(-0.5f,0.5f,-0.5f),Vector3(-0.5f,-0.5f,-0.5f),
		Vector3(-0.5f,0.5f,0.5f),Vector3(-0.5f,-0.5f,-0.5f),Vector3(-0.5f,-0.5f,0.5f)},
	{Vector3(0.5f,0.5f,-0.5f),Vector3(0.5f,0.5f,0.5f),Vector3(0.5f,-0.5f,0.5f),
		Vector3(0.5f,0.5f,-0.5f),Vector3(0.5f,-0.5f,0.5f),Vector3(0.5f,-0.5f,-0.5f)},
	{Vector3(0.5f,-0.5f,-0.5f),Vector3(0.5f,-0.5f,0.5f),Vector3(-0.5f,-0.5f,0.5f),
		Vector3(0.5f,-0.5f,-0.5f),Vector3(-0.5f,-0.5f,0.5f),Vector3(-0.5f,-0.5f,-0.5f)},
	{Vector3(0.5f,0.5f,0.5f),Vector3(0.5f,0.5f,-0.5f),Vector3(-0.5f,0.5f,-0.5f),
		Vector3(0.5f,0.5f,0.5f),Vector3(-0.5f,0.5f,-0.5f),Vector3(-0.5f,0.5f,0.5f)},
	{Vector3(0.5f,0.5f,-0.5f),Vector3(0.5f,-0.5f,-0.5f),Vector3(-0.5f,-0.5f,-0.5f),
		Vector3(0.5f,0.5f,-0.5f),Vector3(-0.5f,-0.5f,-0.5f),Vector3(-0.5f,0.5f,-0.5f)},
	{Vector3(0.5f,-0.5f,0.5f),Vector3(0.5f,0.5f,0.5f),Vector3(-0.5f,0.5f,0.5f),
		Vector3(0.5f,-0.5f,0.5f),Vector3(-0.5f,0.5f,0.5f),Vector3(-0.5f,-0.5f,0.5f)}
};

const Vector3 BLOCK_NORMALS[6] =
{
	Vector3(-1,0,0),
	Vector3(1,0,0),
	Vector3(0,-1,0),
	Vector3(0,1,0),
	Vector3(0,0,-1),
	Vector3(0,0,1)	
};

// todo: fix vertex windings and such above, so this isn't necessary
const Vector2 BLOCK_TEXCOORDS[6][6] =
{
	{Vector2(0,0),Vector2(1,0),Vector2(1,1),Vector2(0,0),Vector2(1,1),Vector2(0,1)},
	{Vector2(0,0),Vector2(1,0),Vector2(1,1),Vector2(0,0),Vector2(1,1),Vector2(0,1)},
	{Vector2(0,0),Vector2(1,0),Vector2(1,1),Vector2(0,0),Vector2(1,1),Vector2(0,1)},
	{Vector2(0,0),Vector2(1,0),Vector2(1,1),Vector2(0,0),Vector2(1,1),Vector2(0,1)},
	{Vector2(1,0),Vector2(1,1),Vector2(0,1),Vector2(1,0),Vector2(0,1),Vector2(0,0)},
	{Vector2(1,1),Vector2(1,0),Vector2(0,0),Vector2(1,1),Vector2(0,0),Vector2(0,1)}
};

#endif
