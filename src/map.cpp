/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#include "map.h"
//#include "player.h"
#include "main.h"
#include <jthread/jmutexautolock.h>
namespace jthread {} // JThread 1.2 support
using namespace jthread; // JThread 1.3 support
#include "client.h"

#ifdef _WIN32
	#include <windows.h>
	#define sleep_ms(x) Sleep(x)
#else
	#include <unistd.h>
	#define sleep_ms(x) usleep(x*1000)
#endif

/*
void limitBox(core::aabbox3d<s16> & box, core::aabbox3d<s16> & limits)
{
	if(box.MinEdge.X < limits.MinEdge.X)
		box.MinEdge.X = limits.MinEdge.X;
	if(box.MaxEdge.X > limits.MaxEdge.X)
		box.MaxEdge.X = limits.MaxEdge.X;
	if(box.MinEdge.Y < limits.MinEdge.Y)
		box.MinEdge.Y = limits.MinEdge.Y;
	if(box.MaxEdge.Y > limits.MaxEdge.Y)
		box.MaxEdge.Y = limits.MaxEdge.Y;
	if(box.MinEdge.Z < limits.MinEdge.Z)
		box.MinEdge.Z = limits.MinEdge.Z;
	if(box.MaxEdge.Z > limits.MaxEdge.Z)
		box.MaxEdge.Z = limits.MaxEdge.Z;
}
*/

void * MapUpdateThread::Thread()
{
	if(map == NULL)
		return NULL;
		
	ThreadStarted();

	while(getRun())
	{
		//std::cout<<"UpdateThread running"<<std::endl;
		
		bool did_something;
		did_something = map->updateChangedVisibleArea();
		
		if(did_something == false)
			sleep_ms(500);
	}

	std::cout<<"UpdateThread stopped"<<std::endl;

	return NULL;
}

Map::Map():
	camera_position(0,0,0),
	camera_direction(0,0,1),
	updater(this),
	m_heightmap(32, 66.0, 0.6, 0.0),
	//m_heightmap(16, 40.0, 0.6, 0.0),
	//m_heightmap(16, 0.0, 0.0, 0.0),
	//m_heightmap(4, 0.0, 0.0, 0.0),
	m_sector_cache(NULL),
	m_hwrapper(this),
	drawoffset(0,0,0)
{
	m_getsector_mutex.Init();
	m_gensector_mutex.Init();
	camera_mutex.Init();
	assert(m_getsector_mutex.IsInitialized());
	assert(m_gensector_mutex.IsInitialized());
	assert(camera_mutex.IsInitialized());
	
	// Get this so that the player can stay on it at first
	//getSector(v2s16(0,0));
}

Map::~Map()
{
	/*
		Stop updater thread
	*/
	updater.setRun(false);
	while(updater.IsRunning())
		sleep_s(1);

	/*
		Free all MapSectors.
	*/
	core::map<v2s16, MapSector*>::Iterator i = m_sectors.getIterator();
	for(; i.atEnd() == false; i++)
	{
		MapSector *sector = i.getNode()->getValue();
		delete sector;
	}
}

/*
	TODO: These mutexes need rethinking. They don't work properly
	      at the moment. Maybe. Or do they?
*/

/*bool Map::sectorExists(v2s16 p)
{
	JMutexAutoLock lock(m_getsector_mutex);
	core::map<v2s16, MapSector*>::Node *n = m_sectors.find(p);
	return (n != NULL);
}*/

MapSector * Map::getSectorNoGenerate(v2s16 p)
{
	//JMutexAutoLock lock(m_getsector_mutex);

	if(m_sector_cache != NULL && p == m_sector_cache_p){
		MapSector * ref(m_sector_cache);
		return ref;
	}
	
	core::map<v2s16, MapSector*>::Node *n = m_sectors.find(p);
	// If sector doesn't exist, throw an exception
	if(n == NULL)
	{
		/*
			TODO: Check if sector is stored on disk
			      and load dynamically
		*/
		//sector = NULL;
		throw InvalidPositionException();
	}
	
	MapSector *sector = n->getValue();
	
	// Cache the last result
	m_sector_cache_p = p;
	m_sector_cache = sector;

	//MapSector * ref(sector);
	
	return sector;
}

MapSector * Map::getSector(v2s16 p2d)
{
	// Stub virtual method
	assert(0);
	return getSectorNoGenerate(p2d);
}

/*
	If sector doesn't exist, returns NULL
	Otherwise returns what the sector returns
	
	TODO: Change this to use exceptions?
*/
MapBlock * Map::getBlockNoCreate(v3s16 p3d)
{	
	/*std::cout<<"Map::getBlockNoCreate(("
			<<p3d.X<<","<<p3d.Y<<","<<p3d.Z
			<<")"<<std::endl;*/
	v2s16 p2d(p3d.X, p3d.Z);
	//v2s16 sectorpos = getNodeSectorPos(p2d);
	MapSector * sectorref = getSectorNoGenerate(p2d);
	MapSector *sector = sectorref;

	//JMutexAutoLock(sector->mutex);

	MapBlock * blockref = sector->getBlockNoCreate(p3d.Y);
	
	return blockref;
}

/*
	If sector doesn't exist, generates a sector.
	Returns what the sector returns.
*/
MapBlock * Map::getBlock(v3s16 p3d)
{
	v2s16 p2d(p3d.X, p3d.Z);
	//v2s16 sectorpos = getNodeSectorPos(p2d);
	MapSector * sref = getSector(p2d);

	/*std::cout<<"Map::GetBlock(): sref->getRefcount()="
			<<sref->getRefcount()
			<<std::endl;*/

	MapSector *sector = sref;
	
	//JMutexAutoLock(sector->mutex);

	MapBlock * blockref = sector->getBlock(p3d.Y);
	
	/*std::cout<<"Map::GetBlock(): blockref->getRefcount()="
			<<blockref->getRefcount()
			<<std::endl;*/

	/*std::cout<<"Map::GetBlock(): "
			<<"Got block. Now returning."<<std::endl;*/
	
	return blockref;
}

f32 Map::getGroundHeight(v2s16 p, bool generate)
{
	/*std::cout<<"Map::getGroundHeight(("
			<<p.X<<","<<p.Y<<"))"<<std::endl;*/
	try{
		v2s16 sectorpos = getNodeSectorPos(p);
		MapSector * sref = getSectorNoGenerate(sectorpos);
		v2s16 relpos = p - sectorpos * MAP_BLOCKSIZE;
		//sref->mutex.Lock();
		f32 y = sref->getGroundHeight(relpos);
		//sref->mutex.Unlock();
		//std::cout<<"[found]"<<std::endl;
		return y;
	}
	catch(InvalidPositionException &e)
	{
		//std::cout<<"[not found]"<<std::endl;
		return GROUNDHEIGHT_NOTFOUND_SETVALUE;
	}
}

void Map::setGroundHeight(v2s16 p, f32 y, bool generate)
{
	/*std::cout<<"Map::setGroundHeight(("
			<<p.X<<","<<p.Y
			<<"), "<<y<<")"<<std::endl;*/
	v2s16 sectorpos = getNodeSectorPos(p);
	MapSector * sref = getSectorNoGenerate(sectorpos);
	v2s16 relpos = p - sectorpos * MAP_BLOCKSIZE;
	//sref->mutex.Lock();
	sref->setGroundHeight(relpos, y);
	//sref->mutex.Unlock();
}

/*
	TODO: Check the order of changing lighting and recursing in
	these functions (choose the faster one)

	TODO: It has to done like this: cache the neighbours that were
	changed; then recurse to them.
*/

/*
	Goes recursively through the neighbours of the node.

	Alters only transparent nodes.

	If the lighting of the neighbour is lower than the lighting of
	the node was (before changing it to 0 at the step before), the
	lighting of the neighbour is set to 0 and then the same stuff
	repeats for the neighbour.

	Some things are made strangely to make it as fast as possible.

	Usage:
	core::list<v3s16> light_sources;
	core::map<v3s16, MapBlock*> modified_blocks;
	f32 oldlight = node_at_pos.light;
	node_at_pos.light = 0;
	unLightNeighbors(pos, oldlight, light_sources, modified_blocks);
*/
void Map::unLightNeighbors(v3s16 pos, f32 oldlight,
		core::list<v3s16> & light_sources,
		core::map<v3s16, MapBlock*>  & modified_blocks)
{
	v3s16 dirs[6] = {
		v3s16(0,0,1), // back
		v3s16(0,1,0), // top
		v3s16(1,0,0), // right
		v3s16(0,0,-1), // front
		v3s16(0,-1,0), // bottom
		v3s16(-1,0,0), // left
	};
	
	// Loop through 6 neighbors
	for(u16 i=0; i<6; i++){
		// Get the position of the neighbor node
		v3s16 n2pos = pos + dirs[i];
		
		// Get the block where the node is located
		v3s16 blockpos = getNodeBlockPos(n2pos);
		MapBlock *block;
		try{
			block = getBlockNoCreate(blockpos);
		}
		catch(InvalidPositionException &e)
		{
			// If block is inexistent, move to next one.
			continue;
		}
		
		// Find if block is in cache container
		core::map<v3s16, MapBlock * >::Node *cacheblocknode;
		cacheblocknode = modified_blocks.find(blockpos);
		
		// If the block is not found in the cache
		if(cacheblocknode == NULL)
		{
			/*
				Add block to cache container. It could be a 'set' but
				there is no such thing in Irrlicht.
				
				We can use the pointer as the value of a map just fine,
				it gets nicely cached that way, too.
			*/
			modified_blocks.insert(blockpos, block);
		}

		// Calculate relative position in block
		v3s16 relpos = n2pos - blockpos * MAP_BLOCKSIZE;
		// Get node straight from the block (fast!)
		MapNode *n2 = block->getNodePtr(relpos);
		
		/*
			If the neighbor is dimmer than what was specified
			as oldlight (the light of the previous node)...
		*/
		if(n2->light < oldlight - 0.001){
			if(n2->transparent()){
				light_t current_light = n2->light;
				n2->light = 0.0;
				unLightNeighbors(n2pos, current_light,
						light_sources, modified_blocks);
			}
		}
		else{
			light_sources.push_back(n2pos);
		}
	}
}

/*
	Goes recursively through the neighbours of the node.

	Alters only transparent nodes.

	If the lighting of the neighbour is lower than the calculated
	lighting coming from the current node to it, the lighting of
	the neighbour is set and the same thing repeats.

	Some things are made strangely to make it as fast as possible.

	Usage:
	core::map<v3s16, MapBlock*> modified_blocks;
	lightNeighbors(pos, node_at_pos.light, modified_blocks);
*/
/*void Map::lightNeighbors(v3s16 pos, f32 oldlight,
		core::map<v3s16, MapBlock*> & modified_blocks)*/
void Map::lightNeighbors(v3s16 pos,
		core::map<v3s16, MapBlock*> & modified_blocks)
{
	v3s16 dirs[6] = {
		v3s16(0,0,1), // back
		v3s16(0,1,0), // top
		v3s16(1,0,0), // right
		v3s16(0,0,-1), // front
		v3s16(0,-1,0), // bottom
		v3s16(-1,0,0), // left
	};
	
	core::list<v3s16> neighbour_cache;
	
	/*
		Initialize block cache
		(using center node as a starting position)
	*/
	v3s16 blockpos_last = getNodeBlockPos(pos);
	MapBlock *block = NULL;
	try{
		block = getBlockNoCreate(blockpos_last);
	}
	catch(InvalidPositionException &e)
	{
		return;
	}
	
	// Calculate relative position in block
	v3s16 relpos = pos - blockpos_last * MAP_BLOCKSIZE;
	// Get node straight from the block (fast!)
	MapNode *n = block->getNodePtr(relpos);

	f32 oldlight = n->light;
	f32 newlight = oldlight * LIGHT_DIMINISH_FACTOR;

	// Loop through 6 neighbors
	for(u16 i=0; i<6; i++){
		// Get the position of the neighbor node
		v3s16 n2pos = pos + dirs[i];
		
		// Get the block where the node is located
		v3s16 blockpos = getNodeBlockPos(n2pos);

		try
		{
			/*
				Only fetch a new block if the block position has changed
			*/
			if(blockpos != blockpos_last){
				block = getBlockNoCreate(blockpos);
			}
			blockpos_last = blockpos;

			// Find if block is in cache container
			core::map<v3s16, MapBlock * >::Node *cacheblocknode;
			cacheblocknode = modified_blocks.find(blockpos);
			
			// If the block is not found in the cache
			if(cacheblocknode == NULL)
			{
				/*
					Add block to cache container. It could be a 'set' but
					there is no such thing in Irrlicht.
					
					We can use the pointer as the value of a map just fine,
					it gets nicely cached that way, too.
				*/
				modified_blocks.insert(blockpos, block);
			}
			
			// Calculate relative position in block
			v3s16 relpos = n2pos - blockpos * MAP_BLOCKSIZE;
			// Get node straight from the block (fast!)
			MapNode *n2 = block->getNodePtr(relpos);
			
			/*
				If the neighbor is dimmer than what was specified
				as oldlight (the light of the previous node)...
			*/
			if(n2->light_source() > oldlight / LIGHT_DIMINISH_FACTOR + 0.01)
			{
				n2->light = n2->light_source();
				neighbour_cache.push_back(n2pos);
			}
			if(n2->light < newlight - 0.001)
			{
				if(n2->transparent())
				{
					n2->light = newlight;
					// Cache and recurse at last step for maximum speed
					neighbour_cache.push_back(n2pos);
				}
			}
		}
		catch(InvalidPositionException &e)
		{
			continue;
		}
	}

	core::list<v3s16>::Iterator j = neighbour_cache.begin();
	for(; j != neighbour_cache.end(); j++){
		lightNeighbors(*j, modified_blocks);
	}
}

v3s16 Map::getBrightestNeighbour(v3s16 p)
{
	v3s16 dirs[6] = {
		v3s16(0,0,1), // back
		v3s16(0,1,0), // top
		v3s16(1,0,0), // right
		v3s16(0,0,-1), // front
		v3s16(0,-1,0), // bottom
		v3s16(-1,0,0), // left
	};
	
	f32 brightest_light = -1.0;
	v3s16 brightest_pos(0,0,0);

	// Loop through 6 neighbors
	for(u16 i=0; i<6; i++){
		// Get the position of the neighbor node
		v3s16 n2pos = p + dirs[i];
		MapNode n2;
		try{
			n2 = getNode(n2pos);
		}
		catch(InvalidPositionException &e)
		{
			continue;
		}
		if(n2.light > brightest_light){
			brightest_light = n2.light;
			brightest_pos = n2pos;
		}
	}
	if(brightest_light < -0.001)
		throw;
	return brightest_pos;
}

/*
	Propagates sunlight down from a node.
	Starting point gets sunlight.

	Returns the lowest y value of where the sunlight went.
*/
s16 Map::propagateSunlight(v3s16 start,
		core::map<v3s16, MapBlock*> & modified_blocks)
{
	s16 y = start.Y;
	for(; ; y--)
	{
		v3s16 pos(start.X, y, start.Z);
		
		v3s16 blockpos = getNodeBlockPos(pos);
		MapBlock *block;
		try{
			block = getBlockNoCreate(blockpos);
		}
		catch(InvalidPositionException &e)
		{
			break;
		}

		v3s16 relpos = pos - blockpos*MAP_BLOCKSIZE;
		MapNode *n = block->getNodePtr(relpos);

		if(n->transparent())
		{
			n->light = 1.0;

			modified_blocks.insert(blockpos, block);
		}
		else{
			break;
		}
	}
	return y + 1;
}
		
void Map::updateLighting(core::list< MapBlock*> & a_blocks,
		core::map<v3s16, MapBlock*> & modified_blocks)
{
	std::cout<<"Map::updateLighting(): "
			<<a_blocks.getSize()<<" blocks"<<std::endl;

	/*status.setTodo(a_blocks.getSize());
	status.setDone(0);
	status.setText(L"Updating lighting");*/
	
	std::cout<<"Flooding direct sunlight"<<std::endl;
	
	// Copy list
	core::list< MapBlock * > temp_blocks = a_blocks;

	/*
		Go from the highest blocks to the lowest, propagating sunlight
		through them.
	*/

	while(temp_blocks.empty() == false){
		// Get block with highest position in Y
		
		core::list< MapBlock * >::Iterator highest_i = temp_blocks.end();
		v3s16 highest_pos(0,-32768,0);

		core::list< MapBlock * >::Iterator i;
		for(i = temp_blocks.begin(); i != temp_blocks.end(); i++)
		{
			MapBlock *block = *i;
			v3s16 pos = block->getPosRelative();
			if(highest_i == temp_blocks.end() || pos.Y > highest_pos.Y){
				highest_i = i;
				highest_pos = pos;
			}
		}

		// Update sunlight in block
		MapBlock *block = *highest_i;
		block->propagateSunlight();

		/*std::cout<<"block ("<<block->getPosRelative().X<<","
				<<block->getPosRelative().Y<<","
				<<block->getPosRelative().Z<<") ";
		std::cout<<"touches_bottom="<<touches_bottom<<std::endl;*/

		// Add to modified blocks
		v3s16 blockpos = getNodeBlockPos(highest_pos);
		modified_blocks.insert(blockpos, *highest_i);

		// Remove block from list
		temp_blocks.erase(highest_i);
	}

	// Add other light sources here

	//

	std::cout<<"Spreading light to remaining air"<<std::endl;

	/*
		Spread light to air that hasn't got light
		
		TODO: This shouldn't have to be done to every block
		      Though it doesn't hurt much i guess.
	*/
	
	core::list< MapBlock * >::Iterator i = a_blocks.begin();
	for(; i != a_blocks.end(); i++)
	{
		MapBlock *block = *i;
		v3s16 pos = block->getPosRelative();
		s16 xmin = pos.X;
		s16 zmin = pos.Z;
		s16 ymin = pos.Y;
		s16 xmax = pos.X + MAP_BLOCKSIZE - 1;
		s16 zmax = pos.Z + MAP_BLOCKSIZE - 1;
		s16 ymax = pos.Y + MAP_BLOCKSIZE - 1;
		for(s16 z=zmin; z<=zmax; z++){
			for(s16 x=xmin; x<=xmax; x++){
				for(s16 y=ymax; y>=ymin; y--){
					v3s16 pos(x, y, z);
					MapNode n;
					try{
						n = getNode(pos);
					}
					catch(InvalidPositionException &e)
					{
						break;
					}
					if(n.light > NO_LIGHT_MAX){
						v3s16 pos(x,y,z);
						lightNeighbors(pos, modified_blocks);
					}
				}
			}
		}
		std::cout<<"X";
		std::cout.flush();
		
		//status.setDone(status.getDone()+1);
	}
	std::cout<<std::endl;
}

/*
	This is called after changing a node from transparent to opaque.
	The lighting value of the node should be left as-is after changing
	other values. This sets the lighting value to 0.
*/
void Map::nodeAddedUpdate(v3s16 p, f32 lightwas)
{
	std::cout<<"Map::nodeAddedUpdate()"<<std::endl;

	core::list<v3s16> light_sources;
	core::map<v3s16, MapBlock*> modified_blocks;
	//MapNode n = getNode(p);

	/*
		From this node to nodes underneath:
		If lighting is sunlight (1.0), unlight neighbours and
		set lighting to 0.
		Else discontinue.
	*/

	bool node_under_sunlight = true;
	
	v3s16 toppos = p + v3s16(0,1,0);

	/*
		If there is a node at top and it doesn't have sunlight,
		there has not been any sunlight going down.

		Otherwise there probably is.
	*/
	try{
		MapNode topnode = getNode(toppos);

		if(topnode.light < 0.999)
			node_under_sunlight = false;
	}
	catch(InvalidPositionException &e)
	{
	}

	// Add the block of the added node to modified_blocks
	v3s16 blockpos = getNodeBlockPos(p);
	MapBlock * blockref = getBlockNoCreate(blockpos);
	MapBlock *block = blockref;
	assert(block != NULL);
	modified_blocks.insert(blockpos, block);
	
	// Unlight neighbours of node.
	// This means setting light of all consequent dimmer nodes
	// to 0.
	
	if(isValidPosition(p) == false)
		throw;
		
	unLightNeighbors(p, lightwas, light_sources, modified_blocks);

	MapNode n = getNode(p);
	n.light = 0;
	setNode(p, n);
	
	if(node_under_sunlight)
	{
		s16 y = p.Y - 1;
		for(;; y--){
			std::cout<<"y="<<y<<std::endl;
			v3s16 n2pos(p.X, y, p.Z);
			
			MapNode n2;
			try{
				n2 = getNode(n2pos);
			}
			catch(InvalidPositionException &e)
			{
				break;
			}

			if(n2.light >= 0.999){
				std::cout<<"doing"<<std::endl;
				unLightNeighbors(n2pos, n2.light, light_sources, modified_blocks);
				n2.light = 0;
				setNode(n2pos, n2);
			}
			else
				break;
		}
	}
	
	core::list<v3s16>::Iterator j = light_sources.begin();
	for(; j != light_sources.end(); j++)
	{
		lightNeighbors(*j, modified_blocks);
	}

	core::map<v3s16, MapBlock * >::Iterator i = modified_blocks.getIterator();
	for(; i.atEnd() == false; i++)
	{
		i.getNode()->getValue()->updateFastFaces();
	}
}

/*
*/
void Map::removeNodeAndUpdate(v3s16 p)
{
	std::cout<<"Map::removeNodeAndUpdate()"<<std::endl;
	
	core::map<v3s16, MapBlock*> modified_blocks;

	bool node_under_sunlight = true;
	
	v3s16 toppos = p + v3s16(0,1,0);
	
	/*
		If there is a node at top and it doesn't have sunlight,
		there will be no sunlight going down.
	*/
	try{
		MapNode topnode = getNode(toppos);

		if(topnode.light < 0.999)
			node_under_sunlight = false;
	}
	catch(InvalidPositionException &e)
	{
	}

	/*
		Unlight neighbors (in case the node is a light source)
	*/
	core::list<v3s16> light_sources;
	unLightNeighbors(p, getNode(p).light,
			light_sources, modified_blocks);

	/*
		Remove the node
	*/
	MapNode n;
	n.d = MATERIAL_AIR;
	setNode(p, n);
	
	/*
		Recalculate lighting
	*/
	core::list<v3s16>::Iterator j = light_sources.begin();
	for(; j != light_sources.end(); j++)
	{
		lightNeighbors(*j, modified_blocks);
	}

	// Add the block of the removed node to modified_blocks
	v3s16 blockpos = getNodeBlockPos(p);
	MapBlock * blockref = getBlockNoCreate(blockpos);
	MapBlock *block = blockref;
	assert(block != NULL);
	modified_blocks.insert(blockpos, block);

	if(node_under_sunlight)
	{
		s16 ybottom = propagateSunlight(p, modified_blocks);
		/*std::cout<<"Node was under sunlight. "
				"Propagating sunlight";
		std::cout<<" -> ybottom="<<ybottom<<std::endl;*/
		s16 y = p.Y;
		for(; y >= ybottom; y--)
		{
			v3s16 p2(p.X, y, p.Z);
			/*std::cout<<"lighting neighbors of node ("
					<<p2.X<<","<<p2.Y<<","<<p2.Z<<")"
					<<std::endl;*/
			lightNeighbors(p2, modified_blocks);
		}
	}
	else
	{
		// Set the lighting of this node to 0
		try{
			MapNode n = getNode(p);
			n.light = 0;
			setNode(p, n);
		}
		catch(InvalidPositionException &e)
		{
			throw;
		}
	}

	// Get the brightest neighbour node and propagate light from it
	v3s16 n2p = getBrightestNeighbour(p);
	try{
		MapNode n2 = getNode(n2p);
		lightNeighbors(n2p, modified_blocks);
	}
	catch(InvalidPositionException &e)
	{
	}

	core::map<v3s16, MapBlock * >::Iterator i = modified_blocks.getIterator();
	for(; i.atEnd() == false; i++)
	{
		i.getNode()->getValue()->updateFastFaces();
	}
}

core::aabbox3d<s16> Map::getDisplayedBlockArea()
{
	camera_mutex.Lock();
	core::aabbox3d<s16> box_nodes(floatToInt(camera_position));
	camera_mutex.Unlock();
	
	g_viewing_range_nodes_mutex.Lock();
	s16 d = g_viewing_range_nodes;
	g_viewing_range_nodes_mutex.Unlock();
	box_nodes.MinEdge -= v3s16(d,d,d);
	box_nodes.MaxEdge += v3s16(d,d,d);

	return core::aabbox3d<s16>(
			getNodeBlockPos(box_nodes.MinEdge),
			getNodeBlockPos(box_nodes.MaxEdge));
}

void Map::renderMap(video::IVideoDriver* driver,
	video::SMaterial *materials)
{
	//std::cout<<"Rendering map..."<<std::endl;
	
	/*
		Collect all blocks that are in the view range

		Should not optimize more here as we want to auto-update
		all changed nodes in viewing range at the next step.
	*/

	core::list< MapBlock * > blocks_displayed;
	core::list< MapBlock * >::Iterator bi;

#if 0
	/*
		Get all blocks
	*/
	core::map<v2s16, MapSector*>::Iterator si;

	si = m_sectors.getIterator();
	for(; si.atEnd() == false; si++)
	{
		MapSector *sector = si.getNode()->getValue();
		core::list< MapBlock * > sectorblocks = sector->getBlocks();
		core::list< MapBlock * >::Iterator i;
		for(i=sectorblocks.begin(); i!=sectorblocks.end(); i++){
			blocks_displayed.push_back(*i);
		}
	}
#else
	/*
		Get visible blocks
	*/
	core::aabbox3d<s16> box_blocks = getDisplayedBlockArea();

	for(s16 y=box_blocks.MaxEdge.Y; y>=box_blocks.MinEdge.Y; y--){
		for(s16 z=box_blocks.MinEdge.Z; z<=box_blocks.MaxEdge.Z; z++){
			for(s16 x=box_blocks.MinEdge.X; x<=box_blocks.MaxEdge.X; x++)
			{
				/*
					Add block to list
				*/
				try{
					MapBlock * blockref = getBlockNoCreate(v3s16(x,y,z));
					blocks_displayed.push_back(blockref);
				}
				catch(InvalidPositionException &e)
				{
				}
			}
		}
	}
#endif

	u8 material_in_use;
	material_in_use = MATERIAL_AIR;

	if(blocks_displayed.getSize() == 0)
		return;
	
	/*
		Draw all displayed faces
	*/

	u32 facecount = 0;
	
	for(bi = blocks_displayed.begin(); bi != blocks_displayed.end(); bi++){
		MapBlock *block = *bi;
		
		/*
			Compare block position to camera position, skip
			if not seen on display
		*/
#if 1
		v3s16 blockpos_nodes = block->getPosRelative();

		// Block center position
		v3f blockpos(
				((float)blockpos_nodes.X + MAP_BLOCKSIZE/2) * BS,
				((float)blockpos_nodes.Y + MAP_BLOCKSIZE/2) * BS,
				((float)blockpos_nodes.Z + MAP_BLOCKSIZE/2) * BS
		);

		camera_mutex.Lock();
		// Block position relative to camera
		v3f blockpos_relative = blockpos - camera_position;

		// Distance in camera direction (+=front, -=back)
		f32 dforward = blockpos_relative.dotProduct(camera_direction);

		camera_mutex.Unlock();

		// Total distance
		f32 d = blockpos_relative.getLength();
		
		// Maximum radius of a block
		f32 block_max_radius = 0.5*1.44*1.44*MAP_BLOCKSIZE*BS;
		
		// If block is (nearly) touching the camera, don't
		// bother checking further
		if(d > block_max_radius * 1.5)
		{
			// Cosine of the angle between the camera direction
			// and the block direction (camera_direction is an unit vector)
			f32 cosangle = dforward / d;
			
			// Compensate for the size of the block
			// (as the block has to be shown even if it's a bit off FOV)
			// This is an estimate.
			cosangle += block_max_radius / dforward;

			// If block is not in the field of view, skip it
			if(cosangle < cos(FOV_ANGLE/2))
				continue;
		}
#endif
		
		/*
			Draw the faces of the block
		*/

		block->fastfaces_mutex.Lock();

		core::list<FastFace*>::Iterator i =
				block->fastfaces->begin();

		for(; i != block->fastfaces->end(); i++)
		{
			FastFace *f = *i;
			
			if(f->material != material_in_use){
				driver->setMaterial(materials[f->material]);
				material_in_use = f->material;
			}

			u16 indices[] = {0,1,2,3};
			
			driver->drawVertexPrimitiveList(f->vertices, 4, indices, 1,
					video::EVT_STANDARD, scene::EPT_QUADS, video::EIT_16BIT);

			facecount++;
		}

		block->fastfaces_mutex.Unlock();
	}
	
	static s32 oldfacecount = 0;
	// Cast needed for msvc
	if(abs((long)(facecount - oldfacecount)) > 333){
		std::cout<<"Rendered "<<facecount<<" faces"<<std::endl;
		oldfacecount = facecount;
	}
	
	
	//std::cout<<"done"<<std::endl;
}

/*
	Returns true if updated something
*/
bool Map::updateChangedVisibleArea()
{
	//status.setDone(false);
	
	/*
		TODO: Update closest ones first

		Update the lighting and the faces of changed blocks that are
		to be displayed.
	*/
	
	core::aabbox3d<s16> box_blocks = getDisplayedBlockArea();
	core::list< MapBlock * > blocks_changed;
	core::list< MapBlock * >::Iterator bi;

	for(s16 y=box_blocks.MaxEdge.Y; y>=box_blocks.MinEdge.Y; y--){
	for(s16 z=box_blocks.MinEdge.Z; z<=box_blocks.MaxEdge.Z; z++){
	for(s16 x=box_blocks.MinEdge.X; x<=box_blocks.MaxEdge.X; x++)
	{
		// This intentionally creates new blocks on demand

		try
		{
			MapBlock * block = getBlock(v3s16(x,y,z));
			if(block->getChangedFlag())
			{
				if(blocks_changed.empty() == true){
					// Print initial message when first is found
					std::cout<<"Updating changed MapBlocks at ";
				}

				v3s16 p = block->getPosRelative();
				std::cout<<"("<<p.X<<","<<p.Y<<","<<p.Z<<") ";
				std::cout.flush();

				blocks_changed.push_back(block);
				block->resetChangedFlag();
			}
		}
		catch(InvalidPositionException &e)
		{
		}
		catch(AsyncQueuedException &e)
		{
		}
	}}}

	// Quit if nothing has changed
	if(blocks_changed.empty()){
		//status.setReady(true);
		return false;
	}

	std::cout<<std::endl;

	std::cout<<"Map::updateChangedVisibleArea(): "
			"there are changed blocks"<<std::endl;

	//status.setReady(false);

	core::map<v3s16, MapBlock*> modified_blocks;

	std::cout<<"Updating lighting"<<std::endl;

	updateLighting(blocks_changed, modified_blocks);

	/*status.setText(L"Updating face cache");
	status.setTodo(modified_blocks.size());
	status.setDone(0);*/

	std::cout<<"Updating face cache of changed blocks"<<std::endl;

	core::map<v3s16, MapBlock * >::Iterator i = modified_blocks.getIterator();
	for(; i.atEnd() == false; i++)
	{
		MapBlock *block = i.getNode()->getValue();
		block->updateFastFaces();
		
		std::cout<<"X";
		std::cout.flush();

		//status.setDone(status.getDone()+1);
	}

	std::cout<<std::endl;

	//status.setReady(true);

	return true;
}

MapSector * MasterMap::getSector(v2s16 p2d)
{
	// Check that it doesn't exist already
	try{
		MapSector * sref = getSectorNoGenerate(p2d);
		return sref;
	}
	catch(InvalidPositionException &e)
	{
	}

	/*std::cout<<"Map::getSector(("<<p2d.X<<","<<p2d.Y
			<<")): generating"<<std::endl;*/

	//std::cout<<"Generating heightmap..."<<std::endl;

	f32 corners[4] = {
		m_heightmap.getGroundHeight(p2d+v2s16(0,0)),
		m_heightmap.getGroundHeight(p2d+v2s16(1,0)),
		m_heightmap.getGroundHeight(p2d+v2s16(1,1)),
		m_heightmap.getGroundHeight(p2d+v2s16(0,1)),
	};
	
	HeightmapBlockGenerator *gen;
	gen = new HeightmapBlockGenerator(p2d, &m_hwrapper);
	/*DummyHeightmap dummyheightmap;
	gen = new HeightmapBlockGenerator(p2d, &dummyheightmap);*/
	gen->m_heightmap->generateContinued(2.0, 0.2, corners);
	//sector->getHeightmap()->generateContinued(0.0, 0.0, corners);
	
	MapSector *sector = new MapSector(this, p2d, gen);

	sector->setHeightmap(gen->m_heightmap);

	m_sectors.insert(p2d, sector);

	/*std::cout<<"Map::getSector(("<<p2d.X<<","<<p2d.Y
			<<")): generated. Now returning."<<std::endl;*/
	
	return sector;
}

ClientMap::ClientMap(
		Client *client,
		video::SMaterial *materials,
		scene::ISceneNode* parent,
		scene::ISceneManager* mgr,
		s32 id
):
	scene::ISceneNode(parent, mgr, id),
	m_client(client),
	m_materials(materials)
{
	/*m_box = core::aabbox3d<f32>(0,0,0,
			map->getW()*BS, map->getH()*BS, map->getD()*BS);*/
	/*m_box = core::aabbox3d<f32>(0,0,0,
			map->getSizeNodes().X * BS,
			map->getSizeNodes().Y * BS,
			map->getSizeNodes().Z * BS);*/
	m_box = core::aabbox3d<f32>(-BS*1000000,-BS*1000000,-BS*1000000,
			BS*1000000,BS*1000000,BS*1000000);
}

MapSector * ClientMap::getSector(v2s16 p2d)
{
	// Check that it doesn't exist already
	try{
		MapSector *sector = getSectorNoGenerate(p2d);
		return sector;
	}
	catch(InvalidPositionException &e)
	{
	}

	/*
		Create a ClientBlockGenerator to fill the MapSector's
		blocks from data fetched by the client from the server.
	*/
	ClientBlockGenerator *gen;
	gen = new ClientBlockGenerator(m_client);
	
	MapSector *sector = new MapSector(this, p2d, gen);

	m_sectors.insert(p2d, sector);

	return sector;
}


