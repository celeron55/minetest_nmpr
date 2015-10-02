/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#ifndef MAPNODE_HEADER
#define MAPNODE_HEADER

#include <iostream>
#include "common_irrlicht.h"
#include "light.h"
#include "utility.h"

// Size of node in rendering units
#define BS 10

#define MATERIALS_COUNT 254

// This is completely ignored. It doesn't create faces with anything.
#define MATERIAL_IGNORE 255
// This is the common material through which the player can walk
// and which is transparent to light
#define MATERIAL_AIR 254

#define USEFUL_MATERIAL_COUNT 4

enum Material
{
	MATERIAL_STONE=0,
	MATERIAL_GRASS,
	/*
		For water, the param is water pressure. 0...127.

		- Water blocks will fall down if there is empty space below.
		- If there is water below, the pressure of the block below is
		  the pressure of the current block + 1, or higher.
		- If there is any pressure in a horizontally neighboring
		  block, a water block will try to move away from it.
		- If there is >=2 of pressure in a block below, water will
		  try to move upwards.

		TODO: Before implementing water, do a server-client framework.
	*/
	MATERIAL_WATER,
	MATERIAL_TORCH,
};

struct MapNode
{
	//TODO: block type to differ from material (e.g. grass edges)
	u8 d; // block type
	light_t light;
	s8 param; // Initialized to 0

	MapNode(const MapNode & n)
	{
		*this = n;
	}
	
	MapNode(u8 data=MATERIAL_AIR, light_t a_light=LIGHT_MIN, u8 a_param=0)
	//MapNode(u8 data=MATERIAL_AIR, light_t a_light=LIGHT_MAX, u8 a_param=0)
	{
		d = data;
		light = a_light;
		param = a_param;
	}

	/*
		If true, the node allows light propagation
	*/
	bool transparent()
	{
		return (d == MATERIAL_AIR || d == MATERIAL_TORCH);
	}

	light_t light_source()
	{
		if(d == MATERIAL_TORCH)
			return 0.9;
		
		return 0.0;
	}

	static u32 serializedLength()
	{
		return 2;
	}
	void serialize(u8 *dest)
	{
		dest[0] = d;
		dest[1] = param;
	}
	void deSerialize(u8 *source)
	{
		d = source[0];
		param = source[1];
	}
};



#endif

