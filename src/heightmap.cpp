/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#include "heightmap.h"

bool g_heightmap_debugprint = false;

f32 FixedHeightmap::avgNeighbours(v2s16 p, s16 d)
{
	//printf("avgNeighbours((%i,%i), %i): ", p.X, p.Y, d);

	v2s16 dirs[4] = {
		v2s16(1,0),
		v2s16(0,1),
		v2s16(-1,0),
		v2s16(0,-1)
	};
	f32 sum = 0.0;
	f32 count = 0.0;
	for(u16 i=0; i<4; i++){
		v2s16 p2 = p + dirs[i] * d;
		f32 n = getGroundHeightParent(p2);
		if(n < GROUNDHEIGHT_VALID_MINVALUE)
			continue;
		sum += n;
		count += 1.0;
		//printf("(%i,%i)=%f ", p2.X, p2.Y, n);
	}
	//printf("\n");
	assert(count > 0.001);
	return sum / count;
}

f32 FixedHeightmap::avgDiagNeighbours(v2s16 p, s16 d)
{
	/*printf("avgDiagNeighbours((%i,%i), %i): ",
		p.X, p.Y, d);*/
		
	v2s16 dirs[4] = {
		v2s16(1,1),
		v2s16(-1,-1),
		v2s16(-1,1),
		v2s16(1,-1)
	};
	f32 sum = 0.0;
	f32 count = 0.0;
	for(u16 i=0; i<4; i++){
		v2s16 p2 = p + dirs[i] * d;
		f32 n = getGroundHeightParent(p2);
		if(n < GROUNDHEIGHT_VALID_MINVALUE)
			continue;
		sum += n;
		count += 1.0;
		//printf("(%i,%i)=%f ", p2.X, p2.Y, n);
	}
	//printf("\n");
	assert(count > 0.001);
	return sum / count;
}

/*
	Adds a point to transform into a diamond pattern
	a = 2, 4, 8, 16, ...
*/
void FixedHeightmap::makeDiamond(v2s16 center, s16 a, f32 randmax)
{
	f32 n = avgDiagNeighbours(center, a/2);
	// Add (-1.0...1.0) * randmax
	n += ((float)rand() / (float)(RAND_MAX/2) - 1.0)*randmax;
	setGroundHeightParent(center, n);
}

/*
	Adds points to transform into a diamond pattern
	a = 2, 4, 8, 16, ...
*/
void FixedHeightmap::makeDiamonds(s16 a, f32 randmax)
{
	s16 count_y = (H-1) / a;
	s16 count_x = (W-1) / a;
	for(s32 yi=0; yi<count_y; yi++){
		for(s32 xi=0; xi<count_x; xi++){
			v2s16 center(xi*a + a/2, yi*a + a/2);
			makeDiamond(center, a, randmax);
		}
	}
}

/*
	Adds a point to transform into a square pattern
	a = 2, 4, 8, 16, ...
*/
void FixedHeightmap::makeSquare(v2s16 center, s16 a, f32 randmax)
{
	f32 n = avgNeighbours(center, a/2);
	// Add (-1.0...1.0) * randmax
	n += ((float)rand() / (float)(RAND_MAX/2) - 1.0)*randmax;
	setGroundHeightParent(center, n);
}

/*
	Adds points to transform into a square pattern
	a = 2, 4, 8, 16, ...
*/
void FixedHeightmap::makeSquares(s16 a, f32 randmax)
{
	/*
		This is a bit tricky; the points have to be kind of
		interlaced. (but not exactly)
		
		On even rows (0, 2, 4, ...) the nodes are horizontally
		at a*n+a/2 positions, vertically at a*n.

		On odd rows (1, 3, 5, ...) the nodes are horizontally
		at a*n positions, vertically at a*n+a/2.
	*/

	s16 count_y = (H-1) / a + 1;
	for(s32 yi=0; yi<=count_y; yi++){
		s16 count_x;
		// Even rows
		count_x = (W-1) / a;
		for(s32 xi=0; xi<count_x; xi++){
			v2s16 center(xi*a + a/2, yi*a);
			makeSquare(center, a, randmax);
		}
		if(yi >= count_y - 1)
			break;
		// odd rows
		count_x = (W-1) / a + 1;
		for(s32 xi=0; xi<count_x; xi++){
			v2s16 center(xi*a, yi*a + a/2);
			makeSquare(center, a, randmax);
		}
	}
}

void FixedHeightmap::DiamondSquare(f32 randmax, f32 randfactor)
{
	u16 a;
	if(W < H)
		a = W-1;
	else
		a = H-1;
	
	// Check that a is a power of two
	if((a & (a-1)) != 0)
		throw;
	
	while(a >= 2)
	{
		makeDiamonds(a, randmax);
		if(HEIGHTMAP_DEBUGPRINT){
			printf("MakeDiamonds a=%i result:\n", a);
			print();
		}
		makeSquares(a, randmax);
		if(HEIGHTMAP_DEBUGPRINT){
			printf("MakeSquares a=%i result:\n", a);
			print();
		}
		a /= 2;
		randmax *= randfactor;
	}
}

void UnlimitedHeightmap::print()
{
	s16 minx =  10000;
	s16 miny =  10000;
	s16 maxx = -10000;
	s16 maxy = -10000;
	core::map<v2s16, FixedHeightmap*>::Iterator i;
	i = m_heightmaps.getIterator();
	if(i.atEnd()){
		printf("UnlimitedHeightmap::print(): empty.\n");
		return;
	}
	for(; i.atEnd() == false; i++)
	{
		v2s16 p = i.getNode()->getValue()->getPosOnMaster();
		if(p.X < minx) minx = p.X;
		if(p.Y < miny) miny = p.Y;
		if(p.X > maxx) maxx = p.X;
		if(p.Y > maxy) maxy = p.Y;
	}
	minx = minx * m_blocksize;
	miny = miny * m_blocksize;
	maxx = (maxx+1) * m_blocksize;
	maxy = (maxy+1) * m_blocksize;
	printf("UnlimitedHeightmap::print(): from (%i,%i) to (%i,%i)\n",
			minx, miny, maxx, maxy);
	for(s32 y=miny; y<=maxy; y++){
		for(s32 x=minx; x<=maxx; x++){
			f32 n = getGroundHeight(v2s16(x,y), false);
			if(n < GROUNDHEIGHT_VALID_MINVALUE)
				printf("  -   ");
			else
				printf("% -5.1f ", getGroundHeight(v2s16(x,y), false));
		}
		printf("\n");
	}
}
	
FixedHeightmap * UnlimitedHeightmap::getHeightmap(v2s16 p, bool generate)
{
	core::map<v2s16, FixedHeightmap*>::Node *n = m_heightmaps.find(p);

	if(n != NULL)
	{
		return n->getValue();
	}

	/*std::cout<<"UnlimitedHeightmap::getHeightmap(("
			<<p.X<<","<<p.Y<<"), generate="
			<<generate<<")"<<std::endl;*/

	if(generate == false)
		throw InvalidPositionException();

	// If heightmap doesn't exist, generate one
	FixedHeightmap *heightmap = new FixedHeightmap(
			this, p, m_blocksize);

	m_heightmaps.insert(p, heightmap);
	
	f32 corners[] = {m_basevalue, m_basevalue, m_basevalue, m_basevalue};
	heightmap->generateContinued(m_randmax, m_randfactor, corners);

	return heightmap;
}

f32 UnlimitedHeightmap::getGroundHeight(v2s16 p, bool generate)
{
	v2s16 heightmappos = getNodeHeightmapPos(p);
	v2s16 relpos = p - heightmappos*m_blocksize;
	try{
		FixedHeightmap * href = getHeightmap(heightmappos, generate);
		f32 h = href->getGroundHeight(relpos);
		if(h > GROUNDHEIGHT_VALID_MINVALUE)
			return h;
	}
	catch(InvalidPositionException){}
	
	/*
		OK, wasn't there.
		
		Mercilessly try to get it somewhere.
	*/
#if 1
	if(relpos.X == 0){
		try{
			FixedHeightmap * href = getHeightmap(
					heightmappos-v2s16(1,0), false);
			f32 h = href->getGroundHeight(v2s16(m_blocksize, relpos.Y));
			if(h > GROUNDHEIGHT_VALID_MINVALUE)
				return h;
		}
		catch(InvalidPositionException){}
	}
	if(relpos.Y == 0){
		try{
			FixedHeightmap * href = getHeightmap(
					heightmappos-v2s16(0,1), false);
			f32 h = href->getGroundHeight(v2s16(relpos.X, m_blocksize));
			if(h > GROUNDHEIGHT_VALID_MINVALUE)
				return h;
		}
		catch(InvalidPositionException){}
	}
	if(relpos.X == 0 && relpos.Y == 0){
		try{
			FixedHeightmap * href = getHeightmap(
					heightmappos-v2s16(1,1), false);
			f32 h = href->getGroundHeight(v2s16(m_blocksize, m_blocksize));
			if(h > GROUNDHEIGHT_VALID_MINVALUE)
				return h;
		}
		catch(InvalidPositionException){}
	}
#endif
	return GROUNDHEIGHT_NOTFOUND_SETVALUE;
}

void UnlimitedHeightmap::setGroundHeight(v2s16 p, f32 y, bool generate)
{
	v2s16 heightmappos = getNodeHeightmapPos(p);
	v2s16 relpos = p - heightmappos*m_blocksize;
	/*std::cout<<"UnlimitedHeightmap::setGroundHeight(("
			<<p.X<<","<<p.Y<<"), "<<y<<"): "
			<<"heightmappos=("<<heightmappos.X<<","
			<<heightmappos.Y<<") relpos=("
			<<relpos.X<<","<<relpos.Y<<")"
			<<std::endl;*/
	try{
		FixedHeightmap * href = getHeightmap(heightmappos, generate);
		href->setGroundHeight(relpos, y);
	}catch(InvalidPositionException){}
	// Update in neighbour heightmap if it's at border
	if(relpos.X == 0){
		try{
			FixedHeightmap * href = getHeightmap(
					heightmappos-v2s16(1,0), generate);
			href->setGroundHeight(v2s16(m_blocksize, relpos.Y), y);
		}catch(InvalidPositionException){}
	}
	if(relpos.Y == 0){
		try{
			FixedHeightmap * href = getHeightmap(
					heightmappos-v2s16(0,1), generate);
			href->setGroundHeight(v2s16(relpos.X, m_blocksize), y);
		}catch(InvalidPositionException){}
	}
	if(relpos.X == m_blocksize && relpos.Y == m_blocksize){
		try{
			FixedHeightmap * href = getHeightmap(
					heightmappos-v2s16(1,1), generate);
			href->setGroundHeight(v2s16(m_blocksize, m_blocksize), y);
		}catch(InvalidPositionException){}
	}
}


void FixedHeightmap::generateContinued(f32 randmax, f32 randfactor,
		f32 *corners)
{
	if(HEIGHTMAP_DEBUGPRINT){
		std::cout<<"FixedHeightmap("<<m_pos_on_master.X
				<<","<<m_pos_on_master.Y
				<<")::generateContinued()"<<std::endl;
	}
	/*
		TODO: Implement changing neighboring heightmaps when needed
	*/

	// Works only with blocksize=2,4,8,16,32,64,...
	s16 a = m_blocksize;
	
	// Check that a is a power of two
	if((a & (a-1)) != 0)
		throw;
	
	// Overwrite with GROUNDHEIGHT_NOTFOUND_SETVALUE
	for(s16 y=0; y<=a; y++){
		for(s16 x=0; x<=a; x++){
			v2s16 p(x,y);
			setGroundHeight(p, GROUNDHEIGHT_NOTFOUND_SETVALUE);
		}
	}

#if 1
	/*
		Seed borders from master heightmap
		TODO: Check that this works properly
	*/
	struct SeedSpec
	{
		v2s16 neighbour_start;
		v2s16 heightmap_start;
		v2s16 dir;
	};

	SeedSpec seeds[4] =
	{
		{ // Z- edge on X-axis
			v2s16(0, -1), // neighbour_start
			v2s16(0, 0), // heightmap_start
			v2s16(1, 0) // dir
		},
		{ // Z+ edge on X-axis
			v2s16(0, m_blocksize),
			v2s16(0, m_blocksize),
			v2s16(1, 0)
		},
		{ // X- edge on Z-axis
			v2s16(-1, 0),
			v2s16(0, 0),
			v2s16(0, 1)
		},
		{ // X+ edge on Z-axis
			v2s16(m_blocksize, 0),
			v2s16(m_blocksize, 0),
			v2s16(0, 1)
		},
	};

	for(s16 i=0; i<4; i++){
		v2s16 npos = seeds[i].neighbour_start + m_pos_on_master * m_blocksize;
		v2s16 hpos = seeds[i].heightmap_start;
		for(s16 s=0; s<m_blocksize+1; s++){
			f32 h = m_master->getGroundHeight(npos, false);
			//std::cout<<"h="<<h<<std::endl;
			if(h < GROUNDHEIGHT_VALID_MINVALUE)
				continue;
				//break;
			setGroundHeight(hpos, h);
			hpos += seeds[i].dir;
			npos += seeds[i].dir;
		}
	}
	
	if(HEIGHTMAP_DEBUGPRINT){
		std::cout<<"borders seeded:"<<std::endl;
		print();
	}
#endif

	/*
		Fill with corners[] (if not already set)
	*/
	v2s16 dirs[4] = {
		v2s16(0,0),
		v2s16(1,0),
		v2s16(1,1),
		v2s16(0,1),
	};
	for(u16 i=0; i<4; i++){
		v2s16 npos = dirs[i] * a;
		// Don't replace already seeded corners
		f32 h = getGroundHeight(npos);
		if(h > GROUNDHEIGHT_VALID_MINVALUE)
			continue;
		setGroundHeight(dirs[i] * a, corners[i]);
	}
	
	if(HEIGHTMAP_DEBUGPRINT){
		std::cout<<"corners filled:"<<std::endl;
		print();
	}

	/*std::cout<<"Seeded heightmap:"<<std::endl;
	print();*/

	DiamondSquare(randmax, randfactor);
}

