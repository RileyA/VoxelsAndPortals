#ifndef ChunkCoords_H
#define ChunkCoords_H

#include "Oryx.h"
#include "ChunkOptions.h"

// This really doesn't need to be a union anymore.. ahhh well I'll refactor later
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

	ChunkCoords()
	{
		c.x = 0;
		c.y = 0;
		c.z = 0;
		c.data = 0;
	}

	ChunkCoords(signed char x, signed char y, signed char z, signed char d = 0)
	{
		c.x = x;
		c.y = y;
		c.z = z;
		c.data = d;
	}

	bool inBounds()
	{
		return c.x<CHUNK_SIZE_X && c.x>=0 && c.y<CHUNK_SIZE_Y && c.y>=0 && c.z<CHUNK_SIZE_Z && c.z>=0;
	}

	bool onEdge() const
	{
		return c.x==CHUNK_SIZE_X-1||c.x==0||c.y==CHUNK_SIZE_Y-1||c.y==0||c.z==CHUNK_SIZE_Z-1||c.z==0;
	}

	signed char& operator [] ( const size_t i )
	{
		return *(&c.x+i);
	}

	const signed char& operator [] ( const size_t i ) const
	{
		return *(&c.x+i);
	}

	ChunkCoords operator + (const ChunkCoords& coord) const
	{
		return ChunkCoords(c.x+coord.c.x,c.y+coord.c.y,c.z+coord.c.z);
	}
	
	ChunkCoords operator << (const char dir) const
	{
		switch (dir)
		{
			case BD_LEFT:
				return *this + ChunkCoords(-1,0,0);
				break;
			case BD_RIGHT:
				return *this + ChunkCoords(1,0,0);
				break;
			case BD_DOWN:
				return *this + ChunkCoords(0,-1,0);
				break;
			case BD_UP:
				return *this + ChunkCoords(0,1,0);
				break;
			case BD_FRONT:
				return *this + ChunkCoords(0,0,-1);
				break;
			case BD_BACK:
				return *this + ChunkCoords(0,0,1);
				break;
		}
	}

	// So this can be used as a std::map key
	bool operator < (const ChunkCoords& coord) const
	{
		return std::lexicographical_compare(&c.x,&c.z+1,&coord.c.x,&coord.c.z+1);
	}
};

/** Used for chunk addressing (non unionized, heh) */
struct InterChunkCoords
{
	int32_t x,y,z;

	InterChunkCoords(int32_t _x, int32_t _y, int32_t _z)
		:x(_x),y(_y),z(_z){}

	InterChunkCoords():x(0),y(0),z(0){}

	InterChunkCoords operator + (const InterChunkCoords& coord) const
	{
		return InterChunkCoords(x+coord.x,y+coord.y,z+coord.z);
	}

	InterChunkCoords operator << (const char dir) const
	{
		switch (dir)
		{
			case BD_LEFT:
				return *this + InterChunkCoords(-1,0,0);
				break;
			case BD_RIGHT:
				return *this + InterChunkCoords(1,0,0);
				break;
			case BD_DOWN:
				return *this + InterChunkCoords(0,-1,0);
				break;
			case BD_UP:
				return *this + InterChunkCoords(0,1,0);
				break;
			case BD_FRONT:
				return *this + InterChunkCoords(0,0,-1);
				break;
			case BD_BACK:
				return *this + InterChunkCoords(0,0,1);
				break;
		}
	}
	
	bool operator < (const InterChunkCoords& c) const
	{
		return std::lexicographical_compare(&x,&z+1,&c.x,&c.z+1);
	}
};

const static ChunkCoords ChunkOffsets[6] = 
	{ChunkCoords(-1,0,0),
	ChunkCoords(1,0,0),
	ChunkCoords(0,-1,0),
	ChunkCoords(0,1,0),
	ChunkCoords(0,0,-1),
	ChunkCoords(0,0,1)};


#endif
