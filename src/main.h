/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#ifndef MAIN_HEADER
#define MAIN_HEADER

#include <jthread/jmutex.h>
namespace jthread {} // JThread 1.2 support
using namespace jthread; // JThread 1.3 support

#define PI 3.14159

#define FOV_ANGLE (PI/2.5)

// Change to struct settings or something
extern s16 g_viewing_range_nodes;
extern JMutex g_viewing_range_nodes_mutex;

#include <fstream>

// Debug streams
extern std::ostream dout_con;
extern std::ostream dout_client;
extern std::ostream dout_server;

#endif

