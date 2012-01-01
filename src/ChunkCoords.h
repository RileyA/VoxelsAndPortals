#ifndef ChunkCoords_H
#define ChunkCoords_H

#include "Oryx.h"
#include "ChunkOptions.h"

struct ChunkCoords
{
	/** Coords (limited to 8 bits, use for relative coords) */
	signed char x, y, z;

	/** It'll probably be padded anyways, may as well get an extra byte of user data */
	unsigned char data; 

	ChunkCoords()
	{
		x = 0;
		y = 0;
		z = 0;
		data = 0;
	}
	//---------------------------------------------------------------------------

	ChunkCoords(signed char _x, signed char _y, signed char _z, signed char d = 0)
	{
		x = _x;
		y = _y;
		z = _z;
		data = d;
	}
	//---------------------------------------------------------------------------

	bool inBounds()
	{
		return x<CHUNK_SIZE_X && x>=0 && y<CHUNK_SIZE_Y && y>=0 && z<CHUNK_SIZE_Z && z>=0;
	}
	//---------------------------------------------------------------------------

	bool onEdge() const
	{
		return x==CHUNK_SIZE_X-1||x==0||y==CHUNK_SIZE_Y-1||y==0||z==CHUNK_SIZE_Z-1||z==0;
	}
	//---------------------------------------------------------------------------

	signed char& operator [] ( const size_t i )
	{
		return *(&x+i);
	}
	//---------------------------------------------------------------------------

	const signed char& operator [] ( const size_t i ) const
	{
		return *(&x+i);
	}
	//---------------------------------------------------------------------------

	ChunkCoords operator + (const ChunkCoords& coord) const
	{
		return ChunkCoords(x+coord.x,y+coord.y,z+coord.z,data);
	}
	//---------------------------------------------------------------------------
	
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
	//---------------------------------------------------------------------------

	// So this can be used as a std::map key
	bool operator < (const ChunkCoords& coord) const
	{
		return std::lexicographical_compare(&x,&z+1,&coord.x,&coord.z+1);
	}
	//---------------------------------------------------------------------------

	bool operator == (const ChunkCoords& c) const
	{
		return x == c.x && y == c.y && z == c.z;
	}
	//---------------------------------------------------------------------------
};

/** Used for chunk addressing */
struct InterChunkCoords
{
	int32_t x,y,z;

	InterChunkCoords(int32_t _x, int32_t _y, int32_t _z)
		:x(_x),y(_y),z(_z){}
	//---------------------------------------------------------------------------

	InterChunkCoords():x(0),y(0),z(0){}
	//---------------------------------------------------------------------------

	InterChunkCoords operator + (const InterChunkCoords& coord) const
	{
		return InterChunkCoords(x+coord.x,y+coord.y,z+coord.z);
	}
	//---------------------------------------------------------------------------

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
	//---------------------------------------------------------------------------
	
	bool operator < (const InterChunkCoords& c) const
	{
		return std::lexicographical_compare(&x,&z+1,&c.x,&c.z+1);
	}
	//---------------------------------------------------------------------------

	bool operator == (const InterChunkCoords& c) const
	{
		return x == c.x && y == c.y && z == c.z;
	}
	//---------------------------------------------------------------------------
};

const static ChunkCoords ChunkOffsets[6] = 
	{ChunkCoords(-1,0,0),
	ChunkCoords(1,0,0),
	ChunkCoords(0,-1,0),
	ChunkCoords(0,1,0),
	ChunkCoords(0,0,-1),
	ChunkCoords(0,0,1)};

struct ChunkChange
{
	ChunkChange()
	{
		origBlock = 0;
		newLight = 0;
	}
	ChunkChange(byte orig, byte light)
	{
		origBlock = orig;
		newLight = light;
	}
	
	// original block value
	byte origBlock;
	// new light emission value (0 if not emitter)
	int8 newLight;
};


#endif
