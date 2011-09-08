#include "Oryx.h"

union ChunkCoords
{
	struct 
	{
		/** Coords (limited to 8 bits, use for relative coords) */
		signed char x, y, z;
		/** It'll probably be padded anyways, may as well get an extra byte of user data */
		unsigned char data; 
	} c;

	/** Used for easily indexing for a map, etc */
	uint32_t index;

	ChunkCoords(signed char x, signed char y, signed char z)
		:c.x(x), c.y(y), c.z(z), c.data(0) {}
	
	ChunkCoords operator+ (const ChunkCoords& other) const
	{
		
	};
};
