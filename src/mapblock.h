/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#ifndef MAPBLOCK_HEADER
#define MAPBLOCK_HEADER

#include <jthread/jmutex.h>
namespace jthread {} // JThread 1.2 support
using namespace jthread; // JThread 1.3 support
#include <assert.h>
#include <exception>
#include "common_irrlicht.h"
#include "mapnode.h"
#include "exceptions.h"

//#define MAP_BLOCKSIZE 20
//#define MAP_BLOCKSIZE 32
#define MAP_BLOCKSIZE 16
//#define MAP_BLOCKSIZE 8

// Named by looking towards z+
enum{
	FACE_BACK=0,
	FACE_TOP,
	FACE_RIGHT,
	FACE_FRONT,
	FACE_BOTTOM,
	FACE_LEFT
};

struct FastFace
{
	u8 material;
	/*u16 x;
	u16 y;
	u16 z;*/
	video::S3DVertex vertices[4]; // Precalculated vertices
};

class NodeContainer
{
public:
	//TODO: Change these exceptions to something more descriptive
	virtual bool isValidPosition(v3s16 p)
	{
		throw InvalidPositionException();
	}
	virtual MapNode getNode(v3s16 p) {
		throw InvalidPositionException();
	}
	virtual void setNode(v3s16 p, MapNode & n) {
		throw InvalidPositionException();
	}

};

class MapBlock : public NodeContainer
{
private:

	NodeContainer *m_parent;
	v3s16 m_pos; // Position in blocks on parent
	MapNode * data;
	bool changed;
	bool probably_dark;

public:

	core::list<FastFace*> *fastfaces;
	JMutex fastfaces_mutex;

	MapBlock(NodeContainer *parent, v3s16 pos)
		: m_parent(parent), m_pos(pos), changed(true)
	{
		data = NULL;
		reallocate();
		fastfaces = new core::list<FastFace*>;
		fastfaces_mutex.Init();
	}

	~MapBlock()
	{
		fastfaces_mutex.Lock();
		core::list<FastFace*>::Iterator i = fastfaces->begin();
		for(; i != fastfaces->end(); i++)
		{
			delete *i;
		}
		delete fastfaces;
		fastfaces_mutex.Unlock();

		if(data)
			free(data);
	}
	
	bool getChangedFlag()
	{
		return changed;
	}

	void resetChangedFlag()
	{
		/*v3s16 p = getPosRelative();
		std::cout<<"MapBlock("<<p.X<<","<<p.Y<<","<<p.Z<<")"
				<<"::resetChangedFlag()"
				<<std::endl;*/
				
		changed = false;
	}

	void setChangedFlag()
	{
		/*v3s16 p = getPosRelative();
		std::cout<<"MapBlock("<<p.X<<","<<p.Y<<","<<p.Z<<")"
				<<"::setChangedFlag()"
				<<std::endl;*/
				
		changed = true;
	}

	v3s16 getPos()
	{
		return m_pos;
	}
		
	v3s16 getPosRelative()
	{
		//return posRelative;
		return m_pos * MAP_BLOCKSIZE;
	}
		
	//TODO: remove
	v3s16 getSizeNodes()
	{
		//return size;
		return v3s16(MAP_BLOCKSIZE, MAP_BLOCKSIZE, MAP_BLOCKSIZE);
	}

	bool getProbablyDark()
	{
		return probably_dark;
	}
	void setProbablyDark(bool a_probably_dark)
	{
		probably_dark = a_probably_dark;
	}

	core::aabbox3d<s16> getBox()
	{
		return core::aabbox3d<s16>(getPosRelative(),
				getPosRelative() + getSizeNodes() - v3s16(1,1,1));
	}
	
	void reallocate()
	{
		//data.clear();
		if(data != NULL)
			free(data);
		u32 l = MAP_BLOCKSIZE * MAP_BLOCKSIZE * MAP_BLOCKSIZE;
		//data.reallocate(l);
		data = (MapNode*)malloc(sizeof(MapNode) * l);
		for(u32 i=0; i<l; i++){
			//data.push_back(MapNode());
			data[i] = MapNode();
		}
	}

	bool isValidPosition(v3s16 p)
	{
		return (p.X >= 0 && p.X < MAP_BLOCKSIZE
				&& p.Y >= 0 && p.Y < MAP_BLOCKSIZE
				&& p.Z >= 0 && p.Z < MAP_BLOCKSIZE);
	}
	/*
		As long as a reference of this MapBlock is kept,
		the pointers returned by these can be used.
	*/
	MapNode * getNodePtr(s16 x, s16 y, s16 z)
	{
		if(x < 0 || x >= MAP_BLOCKSIZE) throw InvalidPositionException();
		if(y < 0 || y >= MAP_BLOCKSIZE) throw InvalidPositionException();
		if(z < 0 || z >= MAP_BLOCKSIZE) throw InvalidPositionException();
		return &data[z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + y*MAP_BLOCKSIZE + x];
	}
	MapNode * getNodePtr(v3s16 p)
	{
		return getNodePtr(p.X, p.Y, p.Z);
	}

	/*
		Regular MapNode get-setters
	*/
	
	MapNode getNode(s16 x, s16 y, s16 z)
	{
		if(x < 0 || x >= MAP_BLOCKSIZE) throw InvalidPositionException();
		if(y < 0 || y >= MAP_BLOCKSIZE) throw InvalidPositionException();
		if(z < 0 || z >= MAP_BLOCKSIZE) throw InvalidPositionException();
		return data[z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + y*MAP_BLOCKSIZE + x];
	}
	
	MapNode getNode(v3s16 p)
	{
		return getNode(p.X, p.Y, p.Z);
	}
	
	void setNode(s16 x, s16 y, s16 z, MapNode & n)
	{
		if(x < 0 || x >= MAP_BLOCKSIZE) throw InvalidPositionException();
		if(y < 0 || y >= MAP_BLOCKSIZE) throw InvalidPositionException();
		if(z < 0 || z >= MAP_BLOCKSIZE) throw InvalidPositionException();
		data[z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + y*MAP_BLOCKSIZE + x] = n;
	}
	
	void setNode(v3s16 p, MapNode & n)
	{
		setNode(p.X, p.Y, p.Z, n);
	}

	/*
		These functions consult the parent container if the position
		is not valid on this MapBlock.
	*/
	bool isValidPositionParent(v3s16 p);
	MapNode getNodeParent(v3s16 p);
	void setNodeParent(v3s16 p, MapNode & n);

	void drawbox(s16 x0, s16 y0, s16 z0, s16 w, s16 h, s16 d, MapNode node)
	{
		for(u16 z=0; z<d; z++)
			for(u16 y=0; y<h; y++)
				for(u16 x=0; x<w; x++)
					setNode(x0+x, y0+y, z0+z, node);
	}

	static FastFace * makeFastFace(u8 material, light_t light, v3f p,
			v3f dir, v3f scale, v3f posRelative_f);
	
	light_t getFaceLight(v3s16 p, v3s16 face_dir);
	
	/*
		Gets node material from any place relative to block.
		Returns MATERIAL_AIR if doesn't exist.
	*/
	u8 getNodeMaterial(v3s16 p);

	/*
		startpos:
		translate_dir: unit vector with only one of x, y or z
		face_dir: unit vector with only one of x, y or z
	*/
	void updateFastFaceRow(v3s16 startpos,
			u16 length,
			v3s16 translate_dir,
			v3s16 face_dir,
			core::list<FastFace*> &dest);

	/*
		Find all faces being between material and air
	*/
	void updateFastFaces();

	/*
		Propagates sunlight down through the block.
		Returns true if some sunlight touches the bottom of the block.
	*/

	bool propagateSunlight();
	
	static u32 serializedLength();
	void serialize(u8 *dest);
	void deSerialize(u8 *source);
};

#endif

