/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#ifndef HEIGHTMAP_HEADER
#define HEIGHTMAP_HEADER

#include <iostream>
#include <assert.h>
#include <time.h>

#include "common_irrlicht.h"
#include "exceptions.h"

#define GROUNDHEIGHT_NOTFOUND_SETVALUE (-10e6)
#define GROUNDHEIGHT_VALID_MINVALUE    ( -9e6)

extern bool g_heightmap_debugprint;
#define HEIGHTMAP_DEBUGPRINT g_heightmap_debugprint

class Heightmappish
{
public:
	virtual f32 getGroundHeight(v2s16 p, bool generate=true)
	{
		printf("Heightmappish::getGroundHeight() stub called\n");
		assert(0);
		return 0.0;
	}
	virtual void setGroundHeight(v2s16 p, f32 y, bool generate=true)
	{
		printf("Heightmappish::setGroundHeight() stub called\n");
		assert(0);
	}
	v2f32 getSlope(v2s16 p)
	{
		f32 y0 = getGroundHeight(p, false);

		v2s16 dirs[] = {
			v2s16(1,0),
			v2s16(0,1),
		};

		v2f32 fdirs[] = {
			v2f32(1,0),
			v2f32(0,1),
		};

		v2f32 slopevector(0.0, 0.0);
		
		for(u16 i=0; i<2; i++){
			f32 y1 = 0.0;
			f32 y2 = 0.0;
			f32 count = 0.0;

			v2s16 p1 = p - dirs[i];
			y1 = getGroundHeight(p1, false);
			if(y1 > GROUNDHEIGHT_VALID_MINVALUE){
				y1 -= y0;
				count += 1.0;
			}
			else
				y1 = 0;

			v2s16 p2 = p + dirs[i];
			y2 = getGroundHeight(p2, false);
			if(y2 > GROUNDHEIGHT_VALID_MINVALUE){
				y2 -= y0;
				count += 1.0;
			}
			else
				y2 = 0;

			if(count < 0.001)
				return v2f32(0.0, 0.0);
			
			/*
				If y2 is higher than y1, slope is positive
			*/
			f32 slope = (y2 - y1)/count;

			slopevector += fdirs[i] * slope;
		}

		return slopevector;
	}

};

class Heightmap : public Heightmappish /*, public ReferenceCounted*/
{
};

class WrapperHeightmap : public Heightmap
{
	Heightmappish *m_target;
public:

	WrapperHeightmap(Heightmappish *target):
		m_target(target)
	{
		if(target == NULL)
			throw NullPointerException();
	}

	f32 getGroundHeight(v2s16 p, bool generate=true)
	{
		return m_target->getGroundHeight(p, generate);
	}
	void setGroundHeight(v2s16 p, f32 y, bool generate=true)
	{
		m_target->setGroundHeight(p, y, generate);
	}
};

class DummyHeightmap : public Heightmap
{
public:
	f32 m_value;

	DummyHeightmap(f32 value=0.0)
	{
		m_value = value;
	}
	
	f32 getGroundHeight(v2s16 p, bool generate=true)
	{
		return m_value;
	}
	void setGroundHeight(v2s16 p, f32 y, bool generate=true)
	{
		std::cout<<"DummyHeightmap::getGroundHeight()"<<std::endl;
	}
};

class FixedHeightmap : public Heightmap
{
	// A meta-heightmap on which this heightmap is located
	// (at m_pos_on_master * m_blocksize)
	Heightmap * m_master;
	// Position on master heightmap (in blocks)
	v2s16 m_pos_on_master;
	s32 m_blocksize; // This is (W-1) = (H-1)
	// These are the actual size of the data
	s32 W;
	s32 H;
	f32 *m_data;

public:

	FixedHeightmap(Heightmap * master,
			v2s16 pos_on_master, s32 blocksize):
		m_master(master),
		m_pos_on_master(pos_on_master),
		m_blocksize(blocksize)
	{
		W = m_blocksize+1;
		H = m_blocksize+1;
		m_data = NULL;
		m_data = new f32[(blocksize+1)*(blocksize+1)];

		for(s32 i=0; i<(blocksize+1)*(blocksize+1); i++){
			m_data[i] = GROUNDHEIGHT_NOTFOUND_SETVALUE;
		}
	}

	~FixedHeightmap()
	{
		if(m_data)
			delete[] m_data;
	}

	v2s16 getPosOnMaster()
	{
		return m_pos_on_master;
	}

	/*
		TODO: BorderWrapper class or something to allow defining
		borders that wrap to an another heightmap. The algorithm
		should be allowed to edit stuff over the border and on
		the border in that case, too.
		This will allow non-square heightmaps, too. (probably)
	*/

	void print()
	{
		printf("FixedHeightmap::print(): size is %ix%i\n", W, H);
		for(s32 y=0; y<H; y++){
			for(s32 x=0; x<W; x++){
				/*if(getSeeded(v2s16(x,y)))
					printf("S");*/
				f32 n = getGroundHeight(v2s16(x,y));
				if(n < GROUNDHEIGHT_VALID_MINVALUE)
					printf("  -   ");
				else
					printf("% -5.1f ", getGroundHeight(v2s16(x,y)));
			}
			printf("\n");
		}
	}
	
	bool overborder(v2s16 p)
	{
		return (p.X < 0 || p.X >= W || p.Y < 0 || p.Y >= H);
	}

	bool atborder(v2s16 p)
	{
		if(overborder(p))
			return false;
		return (p.X == 0 || p.X == W-1 || p.Y == 0 || p.Y == H-1);
	}

	void setGroundHeight(v2s16 p, f32 y)
	{
		/*std::cout<<"FixedHeightmap::setGroundHeight(("
				<<p.X<<","<<p.Y
				<<"), "<<y<<")"<<std::endl;*/
		if(overborder(p))
			throw InvalidPositionException();
		m_data[p.Y*W + p.X] = y;
	}

	void setGroundHeightParent(v2s16 p, f32 y, bool generate=false)
	{
		/*// Position on master
		v2s16 blockpos_nodes = m_pos_on_master * m_blocksize;
		v2s16 nodepos_master = blockpos_nodes + p;
		std::cout<<"FixedHeightmap::setGroundHeightParent(("
				<<p.X<<","<<p.Y
				<<"), "<<y<<"): nodepos_master=("
				<<nodepos_master.X<<","
				<<nodepos_master.Y<<")"<<std::endl;
		m_master->setGroundHeight(nodepos_master, y, false);*/
		
		if(overborder(p) || atborder(p))
		{
			try{
				// Position on master
				v2s16 blockpos_nodes = m_pos_on_master * m_blocksize;
				v2s16 nodepos_master = blockpos_nodes + p;
				m_master->setGroundHeight(nodepos_master, y, false);
			}
			catch(InvalidPositionException &e)
			{
			}
		}
		
		if(overborder(p))
			return;
		
		setGroundHeight(p, y);
	}
	
	f32 getGroundHeight(v2s16 p, bool generate=false)
	{
		if(overborder(p))
			return GROUNDHEIGHT_NOTFOUND_SETVALUE;
		return m_data[p.Y*W + p.X];
	}

	f32 getGroundHeightParent(v2s16 p)
	{
		/*v2s16 blockpos_nodes = m_pos_on_master * m_blocksize;
		return m_master->getGroundHeight(blockpos_nodes + p, false);*/

		if(overborder(p) == false){
			f32 h = getGroundHeight(p);
			if(h > GROUNDHEIGHT_VALID_MINVALUE)
				return h;
		}
		
		// Position on master
		v2s16 blockpos_nodes = m_pos_on_master * m_blocksize;
		f32 h = m_master->getGroundHeight(blockpos_nodes + p, false);
		return h;
	}

	f32 avgNeighbours(v2s16 p, s16 d);

	f32 avgDiagNeighbours(v2s16 p, s16 d);
	
	/*
		Adds a point to transform into a diamond pattern
		a = 2, 4, 8, 16, ...
	*/
	void makeDiamond(v2s16 center, s16 a, f32 randmax);

	/*
		Adds points to transform into a diamond pattern
		a = 2, 4, 8, 16, ...
	*/
	void makeDiamonds(s16 a, f32 randmax);

	/*
		Adds a point to transform into a square pattern
		a = 2, 4, 8, 16, ...
	*/
	void makeSquare(v2s16 center, s16 a, f32 randmax);

	/*
		Adds points to transform into a square pattern
		a = 2, 4, 8, 16, ...
	*/
	void makeSquares(s16 a, f32 randmax);

	void DiamondSquare(f32 randmax, f32 randfactor);
	
	/*
		corners: [i]=XY: [0]=00, [1]=10, [2]=11, [3]=10
	*/
	void generateContinued(f32 randmax, f32 randfactor, f32 *corners);
};

class OneChildHeightmap : public Heightmap
{
	s16 m_blocksize;

public:

	FixedHeightmap m_child;

	OneChildHeightmap(s16 blocksize):
		m_blocksize(blocksize),
		m_child(this, v2s16(0,0), blocksize)
	{
	}
	
	f32 getGroundHeight(v2s16 p, bool generate=true)
	{
		if(p.X < 0 || p.X > m_blocksize 
				|| p.Y < 0 || p.Y > m_blocksize)
			return GROUNDHEIGHT_NOTFOUND_SETVALUE;
		return m_child.getGroundHeight(p);
	}
	void setGroundHeight(v2s16 p, f32 y, bool generate=true)
	{
		//std::cout<<"OneChildHeightmap::setGroundHeight()"<<std::endl;
		if(p.X < 0 || p.X > m_blocksize 
				|| p.Y < 0 || p.Y > m_blocksize)
			throw InvalidPositionException();
		m_child.setGroundHeight(p, y);
	}
};

/*
	TODO
	This is a dynamic container of an arbitrary number of heightmaps
	at arbitrary positions.
	
	It is able to redirect queries to the corresponding heightmaps and
	it generates new heightmaps on-the-fly according to the relevant
	parameters.
	
	It doesn't have a master heightmap because it is meant to be used
	as such itself.

	Child heightmaps are spaced at m_blocksize distances, and are of
	size (m_blocksize+1)*(m_blocksize+1)

	TODO: Dynamic unloading and loading of stuff to/from disk
	
	This is used as the master heightmap of a Map object.
*/
class UnlimitedHeightmap: public Heightmap
{
private:

	core::map<v2s16, FixedHeightmap*> m_heightmaps;
	s16 m_blocksize;

	f32 m_randmax;
	f32 m_randfactor;
	f32 m_basevalue;

public:

	UnlimitedHeightmap(s16 blocksize, f32 randmax, f32 randfactor,
			f32 basevalue=0.0):
		m_blocksize(blocksize),
		m_randmax(randmax),
		m_randfactor(randfactor),
		m_basevalue(basevalue)
	{
	}

	~UnlimitedHeightmap()
	{
		core::map<v2s16, FixedHeightmap*>::Iterator i;
		i = m_heightmaps.getIterator();
		for(; i.atEnd() == false; i++)
		{
			delete i.getNode()->getValue();
		}
	}

	void setParams(f32 randmax, f32 randfactor)
	{
		m_randmax = randmax;
		m_randfactor = randfactor;
	}
	
	void print();

	v2s16 getNodeHeightmapPos(v2s16 p)
	{
		return v2s16(
				(p.X>=0 ? p.X : p.X-m_blocksize+1) / m_blocksize,
				(p.Y>=0 ? p.Y : p.Y-m_blocksize+1) / m_blocksize);
	}

	FixedHeightmap * getHeightmap(v2s16 p, bool generate=true);
	
	f32 getGroundHeight(v2s16 p, bool generate=true);
	void setGroundHeight(v2s16 p, f32 y, bool generate=true);
};

#endif

