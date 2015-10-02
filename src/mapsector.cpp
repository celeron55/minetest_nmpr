#include "mapsector.h"
#include "jmutexautolock.h"
#include "client.h"
#include "exceptions.h"

HeightmapBlockGenerator::HeightmapBlockGenerator
		(v2s16 p2d, Heightmap * masterheightmap):
		m_heightmap(NULL)
{
	m_heightmap = new FixedHeightmap(masterheightmap,
			p2d, MAP_BLOCKSIZE);
}

/*HeightmapBlockGenerator::HeightmapBlockGenerator
		(MapSector *sector, Heightmap * masterheightmap):
		m_heightmap(NULL)
{
	m_heightmap = new FixedHeightmap(masterheightmap,
			sector->getPos(), MAP_BLOCKSIZE);
	sector->setHeightmap(m_heightmap);
}*/

MapBlock * HeightmapBlockGenerator::makeBlock(MapSector *sector, s16 block_y)
{
	MapBlock *block = sector->createBlankBlockNoInsert(block_y);

	// Randomize a bit. This makes dungeons.
	bool low_block_is_empty = false;
	if(rand() % 4 == 0)
		low_block_is_empty = true;
	
	u8 material = MATERIAL_GRASS;
	s32 avg_ground_y = 0.0;

	for(s16 z0=0; z0<MAP_BLOCKSIZE; z0++){
		for(s16 x0=0; x0<MAP_BLOCKSIZE; x0++){
			s16 target_y = m_heightmap->getGroundHeight(v2s16(x0,z0));
			avg_ground_y += target_y;

			if(m_heightmap->getSlope(v2s16(x0,z0)).getLength() > 1.05)
				material = MATERIAL_STONE;
			else
				material = MATERIAL_GRASS;

			for(s16 y0=0; y0<MAP_BLOCKSIZE; y0++){
				s16 real_y = block_y * MAP_BLOCKSIZE + y0;
				MapNode n;
				// If node is very low
				if(real_y <= target_y - 10){
					if(low_block_is_empty){
						n.d = MATERIAL_AIR;
					}
					else{
						n.d = MATERIAL_STONE;
					}
				}
				// If node is low
				else if(real_y <= target_y - 6)
					n.d = MATERIAL_STONE;
				// If node is at or under heightmap y
				else if(real_y <= target_y)
					n.d = material;
				// If node is over heightmap y
				else
					n.d = MATERIAL_AIR;
				block->setNode(v3s16(x0,y0,z0), n);
			}
		}
	}
	
	avg_ground_y /= MAP_BLOCKSIZE * MAP_BLOCKSIZE;
	bool probably_dark = (block_y+1) * MAP_BLOCKSIZE < avg_ground_y;
	block->setProbablyDark(probably_dark);

	return block;
}

MapBlock * ClientBlockGenerator::makeBlock(MapSector *sector, s16 block_y)
{
	//std::cout<<"ClientBlockGenerator::makeBlock()"<<std::endl;
	/*
		TODO:
		Add the block to the client's fetch queue.
		As of now, the only other thing we can do is to throw an exception.
	*/
	v3s16 blockpos_map(sector->getPos().X, block_y, sector->getPos().Y);

	//m_client->addToBlockFetchQueue(blockpos_map);
	m_client->fetchBlock(blockpos_map);

	throw AsyncQueuedException("Client will fetch later");
}

MapBlock * MapSector::getBlockBuffered(s16 y)
{
	MapBlock *block;

	if(m_block_cache != NULL && y == m_block_cache_y){
		return m_block_cache;
	}
	
	// If block doesn't exist, return NULL
	core::map<s16, MapBlock*>::Node *n = m_blocks.find(y);
	if(n == NULL)
	{
		/*
			TODO: Check if block is stored on disk
			      and load dynamically
		*/
		block = NULL;
	}
	// If block exists, return it
	else{
		block = n->getValue();
	}
	
	// Cache the last result
	m_block_cache_y = y;
	m_block_cache = block;
	
	return block;
}

MapBlock * MapSector::getBlockNoCreate(s16 y)
{
	/*std::cout<<"MapSector("<<m_pos.X<<","<<m_pos.Y<<")::getBlock("<<y
			<<")"<<std::endl;*/
	
	JMutexAutoLock lock(m_mutex);
	
	MapBlock *block = getBlockBuffered(y);

	if(block == NULL)
		throw InvalidPositionException();
	
	return block;
}

MapBlock * MapSector::createBlankBlockNoInsert(s16 y)
{
	// There should not be a block at this position
	if(getBlockBuffered(y) != NULL)
		throw AlreadyExistsException("Block already exists");

	v3s16 blockpos_map(m_pos.X, y, m_pos.Y);
	
	MapBlock *block = new MapBlock(m_parent, blockpos_map);
	
	return block;
}

MapBlock * MapSector::createBlankBlock(s16 y)
{
	JMutexAutoLock lock(m_mutex);
	
	/*std::cout<<"MapSector("<<m_pos.X<<","<<m_pos.Y<<")::createBlankBlock("<<y
			<<")"<<std::endl;*/

	MapBlock *block = createBlankBlockNoInsert(y);
	
	m_blocks.insert(y, block);

	return block;
}

/*
	This will generate a new block as needed.

	A valid heightmap is assumed to exist already.
*/
MapBlock * MapSector::getBlock(s16 block_y)
{
	{
		JMutexAutoLock lock(m_mutex);
		MapBlock *block = getBlockBuffered(block_y);
		if(block != NULL)
			return block;
	}
	
	/*std::cout<<"MapSector("<<m_pos.X<<","<<m_pos.Y<<
			")::getBlock("<<block_y<<")"<<std::endl;*/
	
	MapBlock *block = m_block_gen->makeBlock(this, block_y);

	//block->propagateSunlight();
	
	// Insert to container
	{
		JMutexAutoLock lock(m_mutex);
		m_blocks.insert(block_y, block);
	}

	return block;
}

void MapSector::insertBlock(MapBlock *block)
{
	s16 block_y = block->getPos().Y;

	{
		JMutexAutoLock lock(m_mutex);
		MapBlock *block = getBlockBuffered(block_y);
		if(block != NULL){
			throw AlreadyExistsException("Block already exists");
		}
	}

	v2s16 p2d(block->getPos().X, block->getPos().Z);
	assert(p2d == m_pos);
	
	block->propagateSunlight();
	
	// Insert to container
	{
		JMutexAutoLock lock(m_mutex);
		m_blocks.insert(block_y, block);
	}
}

core::list<MapBlock*> MapSector::getBlocks()
{
	JMutexAutoLock lock(m_mutex);

	core::list<MapBlock*> ref_list;

	core::map<s16, MapBlock*>::Iterator bi;

	bi = m_blocks.getIterator();
	for(; bi.atEnd() == false; bi++)
	{
		MapBlock *b = bi.getNode()->getValue();
		ref_list.push_back(b);
	}
	
	return ref_list;
}

f32 MapSector::getGroundHeight(v2s16 p, bool generate)
{
	if(m_heightmap == NULL)
		return GROUNDHEIGHT_NOTFOUND_SETVALUE;
	return m_heightmap->getGroundHeight(p);
}

void MapSector::setGroundHeight(v2s16 p, f32 y, bool generate)
{
	if(m_heightmap == NULL)
		return;
	m_heightmap->setGroundHeight(p, y);
}

//END
