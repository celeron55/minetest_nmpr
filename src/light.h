#ifndef LIGHT_HEADER
#define LIGHT_HEADER

/*
RULES OF LIGHT:

light = 1.0 (0.999-1.001) is SUNLIGHT. It goes downwards infinitely from
infinite highness and stops when it first hits a non-transparent node.
The lighting of the node it hits is 0.

Other LIGHT SOURCES have a light value of UNDER 0.999.

Light diminishes at a constant factor between nodes.

TODO: Should there be a common container to be parent of map, sector and block?

*/

#define LIGHT_MAX 1.0
#define LIGHT_MIN 0.0
// If lighting value is under this, it can be assumed that
// there is no light
#define NO_LIGHT_MAX 0.03

//#define LIGHT_DIMINISH_FACTOR 0.75
#define LIGHT_DIMINISH_FACTOR 0.8

/*
	When something changes lighting of a node, stuff around it is
	updated inside this radius.
*/
//#define LIGHTING_RADIUS 10
#define LIGHTING_RADIUS 15

typedef f32 light_t;

#endif

