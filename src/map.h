/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#ifndef MAP_HEADER
#define MAP_HEADER

#include <jmutex.h>
#include <jthread.h>
#include <iostream>
#include <malloc.h>

#ifdef _WIN32
	#include <windows.h>
	#define sleep_s(x) Sleep((x*1000))
#else
	#include <unistd.h>
	#define sleep_s(x) sleep(x)
#endif

#include "common_irrlicht.h"
#include "heightmap.h"
#include "loadstatus.h"
#include "mapnode.h"
#include "mapblock.h"
#include "mapsector.h"

/*

TODO: Automatically unload blocks from memory and save on disk
	  when they are far away
*/

//void limitBox(core::aabbox3d<s16> & box, core::aabbox3d<s16> & limits);

class Map;

class MapUpdateThread : public JThread
{
	bool run;
	JMutex run_mutex;

	Map *map;

public:

	MapUpdateThread(Map *the_map) : JThread(), run(true), map(the_map)
	{
		run_mutex.Init();
	}

	void * Thread();

	bool getRun()
	{
		run_mutex.Lock();
		bool run_cached = run;
		run_mutex.Unlock();
		return run_cached;
	}
	void setRun(bool a_run)
	{
		run_mutex.Lock();
		run = a_run;
		run_mutex.Unlock();
	}
};

class Map : public NodeContainer, public Heightmappish
{
public:
	/*
		TODO: Dynamic block loading
		Add a filename to the constructor, in which the map will be
		automatically stored - or something.
	*/

protected:

	core::map<v2s16, MapSector*> m_sectors;
	JMutex m_getsector_mutex;
	JMutex m_gensector_mutex;

	v3f camera_position;
	v3f camera_direction;
	JMutex camera_mutex;

	MapUpdateThread updater;

	UnlimitedHeightmap m_heightmap;
	
	// Be sure to set this to NULL when the cached sector is deleted 
	MapSector *m_sector_cache;
	v2s16 m_sector_cache_p;

	WrapperHeightmap m_hwrapper;

public:

	v3s16 drawoffset;

	//LoadStatus status;
	
	Map();
	~Map();

	void updateCamera(v3f pos, v3f dir)
	{
		camera_mutex.Lock();
		camera_position = pos;
		camera_direction = dir;
		camera_mutex.Unlock();
	}

	void StartUpdater()
	{
		updater.Start();
	}

	void StopUpdater()
	{
		updater.setRun(false);
		while(updater.IsRunning())
			sleep_s(1);
	}

	bool UpdaterIsRunning()
	{
		return updater.IsRunning();
	}

	/*
		Returns integer position of the node in given
		floating point position.
	*/
	static v3s16 floatToInt(v3f p)
	{
		v3s16 p2(
			(p.X + (p.X>0 ? BS/2 : -BS/2))/BS,
			(p.Y + (p.Y>0 ? BS/2 : -BS/2))/BS,
			(p.Z + (p.Z>0 ? BS/2 : -BS/2))/BS);
		return p2;
	}

	static v3f intToFloat(v3s16 p)
	{
		v3f p2(
			p.X * BS,
			p.Y * BS,
			p.Z * BS
		);
		return p2;
	}

	static core::aabbox3d<f32> getNodeBox(v3s16 p)
	{
		return core::aabbox3d<f32>(
			(float)p.X * BS - 0.5*BS,
			(float)p.Y * BS - 0.5*BS,
			(float)p.Z * BS - 0.5*BS,
			(float)p.X * BS + 0.5*BS,
			(float)p.Y * BS + 0.5*BS,
			(float)p.Z * BS + 0.5*BS
		);
	}

	//bool sectorExists(v2s16 p);
	MapSector * getSectorNoGenerate(v2s16 p2d);
	virtual MapSector * getSector(v2s16 p);
	
	MapBlock * getBlockNoCreate(v3s16 p);
	virtual MapBlock * getBlock(v3s16 p);

	f32 getGroundHeight(v2s16 p, bool generate=false);
	void setGroundHeight(v2s16 p, f32 y, bool generate=false);
	
	bool isValidPosition(v3s16 p)
	{
		v3s16 blockpos = getNodeBlockPos(p);
		MapBlock * blockref = getBlockNoCreate(blockpos);
		if(blockref == NULL){
			/*std::cout<<"Map::isValidPosition("<<p.X<<","<<p.Y<<","<<p.Z
					<<"): is_valid=0 (block inexistent)";*/
			return false;
		}
		v3s16 relpos = p - blockpos*MAP_BLOCKSIZE;
		bool is_valid = blockref->isValidPosition(relpos);
		/*std::cout<<"Map::isValidPosition("<<p.X<<","<<p.Y<<","<<p.Z
				<<"): is_valid="<<is_valid<<" (block reported)";*/
		return is_valid;
	}
	
	/*
		Returns the position of the block where the node is located
	*/
	v3s16 getNodeBlockPos(v3s16 p)
	{
		return v3s16(
				(p.X>=0 ? p.X : p.X-MAP_BLOCKSIZE+1) / MAP_BLOCKSIZE,
				(p.Y>=0 ? p.Y : p.Y-MAP_BLOCKSIZE+1) / MAP_BLOCKSIZE,
				(p.Z>=0 ? p.Z : p.Z-MAP_BLOCKSIZE+1) / MAP_BLOCKSIZE);
	}

	v2s16 getNodeSectorPos(v2s16 p)
	{
		return v2s16(
				(p.X>=0 ? p.X : p.X-MAP_BLOCKSIZE+1) / MAP_BLOCKSIZE,
				(p.Y>=0 ? p.Y : p.Y-MAP_BLOCKSIZE+1) / MAP_BLOCKSIZE);
	}

	s16 getNodeBlockY(s16 y)
	{
		return (y>=0 ? y : y-MAP_BLOCKSIZE+1) / MAP_BLOCKSIZE;
	}

	MapBlock * getNodeBlock(v3s16 p)
	{
		v3s16 blockpos = getNodeBlockPos(p);
		return getBlock(blockpos);
	}

	MapNode getNode(v3s16 p)
	{
		v3s16 blockpos = getNodeBlockPos(p);
		MapBlock * blockref = getBlockNoCreate(blockpos);
		if(blockref == NULL)
			throw InvalidPositionException();
		v3s16 relpos = p - blockpos*MAP_BLOCKSIZE;

		return blockref->getNode(relpos);
	}

	MapNode getNode(s16 x, s16 y, s16 z)
	{
		return getNode(v3s16(x,y,z));
	}

	void setNode(v3s16 p, MapNode & n)
	{
		v3s16 blockpos = getNodeBlockPos(p);
		MapBlock * blockref = getBlockNoCreate(blockpos);
		if(blockref == NULL)
			throw InvalidPositionException();
		v3s16 relpos = p - blockpos*MAP_BLOCKSIZE;
		blockref->setNode(relpos, n);
	}

	void setNode(s16 x, s16 y, s16 z, MapNode & n)
	{
		setNode(v3s16(x,y,z), n);
	}
	
	
	MapNode getNode(v3f p)
	{
		return getNode(floatToInt(p));
	}

	void drawbox(s16 x0, s16 y0, s16 z0, s16 w, s16 h, s16 d, MapNode node)
	{
		x0 += drawoffset.X;
		y0 += drawoffset.Y;
		z0 += drawoffset.Z;
		for(u16 z=0; z<d; z++)
			for(u16 y=0; y<h; y++)
				for(u16 x=0; x<w; x++)
					setNode(x0+x, y0+y, z0+z, node);
	}
	
	void drawslope(s16 x0, s16 y0, s16 z0, s16 w, s16 h, s16 d, s16 dy_x, s16 dy_z, MapNode node)
	{
		x0 += drawoffset.X;
		y0 += drawoffset.Y;
		z0 += drawoffset.Z;
		for(u16 z=0; z<d; z++){
			for(u16 x=0; x<w; x++){
				for(u16 y=0; y < h + z*dy_z + x*dy_x; y++){
					setNode(x0+x, y0+y, z0+z, node);
				}
			}
		}
	}

	void unLightNeighbors(v3s16 pos, f32 oldlight,
			core::list<v3s16> & light_sources,
			core::map<v3s16, MapBlock*> & modified_blocks);
	
	/*void lightNeighbors(v3s16 pos, f32 oldlight,
			core::map<v3s16, MapBlock*> & modified_blocks);*/
	void lightNeighbors(v3s16 pos,
			core::map<v3s16, MapBlock*> & modified_blocks);

	v3s16 getBrightestNeighbour(v3s16 p);

	s16 propagateSunlight(v3s16 start,
			core::map<v3s16, MapBlock*> & modified_blocks);
	
	void updateLighting(core::list< MapBlock*>  & a_blocks,
			core::map<v3s16, MapBlock*> & modified_blocks);
			
	void nodeAddedUpdate(v3s16 p, f32 light);
	void removeNodeAndUpdate(v3s16 p);

	core::aabbox3d<s16> getDisplayedBlockArea();

	/*void generateBlock(MapBlock *block);
	void generateMaster();*/

	bool updateChangedVisibleArea();
	
	void renderMap(video::IVideoDriver* driver,
		video::SMaterial *materials);
		
};

class MasterMap : public Map
{
public:
	MapSector * getSector(v2s16 p);
};

class Client;

class ClientMap : public Map, public scene::ISceneNode
{
public:
	ClientMap(
			Client *client,
			video::SMaterial *materials,
			scene::ISceneNode* parent,
			scene::ISceneManager* mgr,
			s32 id
	);

	MapSector * getSector(v2s16 p);

	/*
		ISceneNode methods
	*/

	virtual void OnRegisterSceneNode()
	{
		if (IsVisible)
			SceneManager->registerNodeForRendering(this);

		ISceneNode::OnRegisterSceneNode();
	}

	virtual void render()
	{
		video::IVideoDriver* driver = SceneManager->getVideoDriver();
		driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
		renderMap(driver, m_materials);
	}
	
	virtual const core::aabbox3d<f32>& getBoundingBox() const
	{
		return m_box;
	}

	/*virtual u32 getMaterialCount() const
	{
		return 1;
	}

	virtual video::SMaterial& getMaterial(u32 i)
	{
		return materials[0];
	}*/	
	
private:
	Client *m_client;
	
	video::SMaterial *m_materials;

	core::aabbox3d<f32> m_box;
};

#endif

