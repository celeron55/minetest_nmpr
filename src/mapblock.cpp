#include "mapblock.h"
#include "map.h"

bool MapBlock::isValidPositionParent(v3s16 p)
{
	if(isValidPosition(p))
	{
		//std::cout<<"[("<<p.X<<","<<p.Y<<","<<p.Z<<") valid pos. in block]"<<std::endl;
		return true;
	}
	else{
		//std::cout<<"[("<<p.X<<","<<p.Y<<","<<p.Z<<") not valid pos. in block]"<<std::endl;
		return m_parent->isValidPosition(getPosRelative() + p);
	}
}

MapNode MapBlock::getNodeParent(v3s16 p)
{
	if(isValidPosition(p) == false)
	{
		/*std::cout<<"MapBlock::getNodeParent(("
				<<p.X<<","<<p.Y<<","<<p.Z<<"))"
				<<": getting from parent"<<std::endl;*/
		return m_parent->getNode(getPosRelative() + p);
	}
	else
	{
		/*std::cout<<"MapBlock::getNodeParent(("
				<<p.X<<","<<p.Y<<","<<p.Z<<"))"
				<<": getting from self"<<std::endl;*/
		return data[p.Z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + p.Y*MAP_BLOCKSIZE + p.X];
	}
}

void MapBlock::setNodeParent(v3s16 p, MapNode & n)
{
	if(isValidPosition(p) == false)
	{
		m_parent->setNode(getPosRelative() + p, n);
	}
	else
	{
		data[p.Z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + p.Y*MAP_BLOCKSIZE + p.X] = n;
	}
}

FastFace * MapBlock::makeFastFace(u8 material, light_t light, v3f p,
		v3f dir, v3f scale, v3f posRelative_f)
{
	FastFace *f = new FastFace;
	
	// Position is at the center of the cube.
	v3f pos = p * BS;
	posRelative_f *= BS;

	v3f vertex_pos[4];
	// If looking towards z+, this is the face that is behind
	// the center point, facing towards z+.
	vertex_pos[0] = v3f( BS/2,-BS/2,BS/2);
	vertex_pos[1] = v3f(-BS/2,-BS/2,BS/2);
	vertex_pos[2] = v3f(-BS/2, BS/2,BS/2);
	vertex_pos[3] = v3f( BS/2, BS/2,BS/2);
	
	/*
		TODO: Rotate it right
	*/
	core::CMatrix4<f32> m;
	m.buildRotateFromTo(v3f(0,0,1), dir);
	
	for(u16 i=0; i<4; i++){
		m.rotateVect(vertex_pos[i]);
		vertex_pos[i].X *= scale.X;
		vertex_pos[i].Y *= scale.Y;
		vertex_pos[i].Z *= scale.Z;
		vertex_pos[i] += pos + posRelative_f;
	}

	f32 abs_scale = 1.;
	if     (scale.X < 0.999 || scale.X > 1.001) abs_scale = scale.X;
	else if(scale.Y < 0.999 || scale.Y > 1.001) abs_scale = scale.Y;
	else if(scale.Z < 0.999 || scale.Z > 1.001) abs_scale = scale.Z;

	v3f zerovector = v3f(0,0,0);
	
	//u8 li = (1.0+light)/2 * 255.0; //DEBUG
	u8 li = light * 255.0;
	//u8 li = light / 128;
	video::SColor c = video::SColor(255,li,li,li);

	//video::SColor c = video::SColor(255,255,255,255);
	//video::SColor c = video::SColor(255,64,64,64);
	
	f->vertices[0] = video::S3DVertex(vertex_pos[0], zerovector, c,
			core::vector2d<f32>(0,1));
	f->vertices[1] = video::S3DVertex(vertex_pos[1], zerovector, c,
			core::vector2d<f32>(abs_scale,1));
	f->vertices[2] = video::S3DVertex(vertex_pos[2], zerovector, c,
			core::vector2d<f32>(abs_scale,0));
	f->vertices[3] = video::S3DVertex(vertex_pos[3], zerovector, c,
			core::vector2d<f32>(0,0));

	f->material = material;
	//f->direction = direction;

	return f;
}
	
/*
	Parameters must consist of air and !air.
	Order doesn't matter.

	If either of the nodes doesn't exist, light is 0.
*/
light_t MapBlock::getFaceLight(v3s16 p, v3s16 face_dir)
{
	//return 1.0;

	// Make some nice difference to different sides
	//float factor = ((face_dir.X != 0) ? 0.75 : 1.0);
	float factor;
	/*if(face_dir.X != 0)
		factor = 0.70;
	else if(face_dir.Z != 0)
		factor = 0.90;
	else
		factor = 1.00;*/
	if(face_dir.X == 1 || face_dir.Z == 1)
		factor = 0.70;
	else if(face_dir.X == -1 || face_dir.Z == -1)
		factor = 0.90;
	else
		factor = 1.00;
	
	try{
		MapNode n = getNodeParent(p);
		MapNode n2 = getNodeParent(p + face_dir);
		if(n.transparent())
			return n.light * factor;
		else{
			return n2.light * factor;
		}
	}
	catch(InvalidPositionException &e)
	{
		return 0.0;
	}
}

/*
	Gets node material from any place relative to block.
	Returns MATERIAL_AIR if doesn't exist.
*/
u8 MapBlock::getNodeMaterial(v3s16 p)
{
	try{
		MapNode n = getNodeParent(p);
		return n.d;
	}
	catch(InvalidPositionException &e)
	{
		return MATERIAL_IGNORE;
	}
}

/*
	startpos:
	translate_dir: unit vector with only one of x, y or z
	face_dir: unit vector with only one of x, y or z
*/
void MapBlock::updateFastFaceRow(v3s16 startpos,
		u16 length,
		v3s16 translate_dir,
		v3s16 face_dir,
		core::list<FastFace*> &dest)
{
	// The maximum difference in light to tolerate in the same face
	// TODO: make larger?
	//light_t max_light_diff = 0.15;
	light_t max_light_diff = 0.2;

	/*
		Precalculate some variables
	*/
	v3f translate_dir_f(translate_dir.X, translate_dir.Y,
			translate_dir.Z); // floating point conversion
	v3f face_dir_f(face_dir.X, face_dir.Y,
			face_dir.Z); // floating point conversion
	v3f posRelative_f(getPosRelative().X, getPosRelative().Y,
			getPosRelative().Z); // floating point conversion

	v3s16 p = startpos;
	/*
		The light in the air lights the surface is taken from
		the node that is air.
	*/
	light_t light = getFaceLight(p, face_dir);
	
	u16 continuous_materials_count = 0;
	
	u8 material0 = getNodeMaterial(p);
	u8 material1 = getNodeMaterial(p + face_dir);
		
	for(u16 j=0; j<length; j++)
	{
		bool next_is_different = true;
		
		v3s16 p_next;
		u8 material0_next = 0;
		u8 material1_next = 0;
		light_t light_next = 0;
		if(j != length - 1){
			p_next = p + translate_dir;
			material0_next = getNodeMaterial(p_next);
			material1_next = getNodeMaterial(p_next + face_dir);
			light_next = getFaceLight(p_next, face_dir);
			
			if(material0_next == material0
					&& material1_next == material1
					&& fabs(light_next - light) / (light+0.01) < max_light_diff)
			{
				next_is_different = false;
			}
		}

		continuous_materials_count++;
		
		if(next_is_different)
		{
			/*
				If there is a face (= materials differ and the other one is air)
			*/
			if(material0 != material1
					&& material0 != MATERIAL_IGNORE
					&& material1 != MATERIAL_IGNORE
					&& (material0 == MATERIAL_AIR
						|| material1 == MATERIAL_AIR)){
				// Floating point conversion of the position vector
				v3f pf(p.X, p.Y, p.Z);
				// Center point of face (kind of)
				v3f sp = pf - ((f32)continuous_materials_count / 2. - 0.5) * translate_dir_f;
				v3f scale(1,1,1);
				if(translate_dir.X != 0){
					scale.X = continuous_materials_count;
				}
				if(translate_dir.Y != 0){
					scale.Y = continuous_materials_count;
				}
				if(translate_dir.Z != 0){
					scale.Z = continuous_materials_count;
				}
				FastFace *f;
				// If node at sp is not air
				if(material0 != MATERIAL_AIR){
					f = makeFastFace(material0, light,
							sp, face_dir_f, scale,
							posRelative_f);
				}
				// If node at sp is air
				else{
					f = makeFastFace(material1, light,
							sp+face_dir_f, -1*face_dir_f, scale,
							posRelative_f);
				}
				dest.push_back(f);
			}

			continuous_materials_count = 0;
			material0 = material0_next;
			material1 = material1_next;
			light = light_next;
		}
		
		p = p_next;
	}
}

void MapBlock::updateFastFaces()
{
	/*v3s16 p = getPosRelative();
	std::cout<<"MapBlock("<<p.X<<","<<p.Y<<","<<p.Z<<")"
			<<"::updateFastFaces(): ";*/
			//<<"::updateFastFaces()"<<std::endl;

	core::list<FastFace*> *fastfaces_new = new core::list<FastFace*>;
	
	/*
		These are started and stopped one node overborder to
		take changes in those blocks in account.
		
		TODO: This could be optimized by combining it with updateLighting, as
		      it can record the surfaces that can be seen.
	*/

	/*
		Go through every y,z and get top faces in rows of x
	*/
	//for(s16 y=0; y<MAP_BLOCKSIZE; y++){
	for(s16 y=-1; y<MAP_BLOCKSIZE; y++){
		for(s16 z=0; z<MAP_BLOCKSIZE; z++){
			updateFastFaceRow(v3s16(0,y,z), MAP_BLOCKSIZE,
					v3s16(1,0,0),
					v3s16(0,1,0),
					*fastfaces_new);
		}
	}
	/*
		Go through every x,y and get right faces in rows of z
	*/
	//for(s16 x=0; x<MAP_BLOCKSIZE; x++){
	for(s16 x=-1; x<MAP_BLOCKSIZE; x++){
		for(s16 y=0; y<MAP_BLOCKSIZE; y++){
			updateFastFaceRow(v3s16(x,y,0), MAP_BLOCKSIZE,
					v3s16(0,0,1),
					v3s16(1,0,0),
					*fastfaces_new);
		}
	}
	/*
		Go through every y,z and get back faces in rows of x
	*/
	//for(s16 z=0; z<MAP_BLOCKSIZE; z++){
	for(s16 z=-1; z<MAP_BLOCKSIZE; z++){
		for(s16 y=0; y<MAP_BLOCKSIZE; y++){
			updateFastFaceRow(v3s16(0,y,z), MAP_BLOCKSIZE,
					v3s16(1,0,0),
					v3s16(0,0,1),
					*fastfaces_new);
		}
	}

	/*
		Replace the list
	*/

	core::list<FastFace*> *fastfaces_old = fastfaces;

	fastfaces_mutex.Lock();
	fastfaces = fastfaces_new;
	fastfaces_mutex.Unlock();

	/*std::cout<<" Number of faces: old="<<fastfaces_old->getSize()
			<<" new="<<fastfaces_new->getSize()<<std::endl;*/

	/*
		Clear and dump the old list
	*/

	core::list<FastFace*>::Iterator i = fastfaces_old->begin();
	for(; i != fastfaces_old->end(); i++)
	{
		delete *i;
	}
	fastfaces_old->clear();
	delete fastfaces_old;

	//std::cout<<"added "<<fastfaces.getSize()<<" faces."<<std::endl;
}

/*
	Propagates sunlight down through the block.
	Doesn't modify nodes that are not affected by sunlight.
	Returns true if some sunlight touches the bottom of the block.

	If there is a block above, continues from it.
	If there is no block above, assumes there is sunlight, unless
	probably_dark is set.
*/

bool MapBlock::propagateSunlight()
{
	bool sunlight_touches_bottom = false;
	bool has_some_sunlight = false;

	for(s16 x=0; x<MAP_BLOCKSIZE; x++)
	{
		for(s16 z=0; z<MAP_BLOCKSIZE; z++)
		{
			// Check if node above block has sunlight
			try{
				MapNode n = getNodeParent(v3s16(x, MAP_BLOCKSIZE, z));
				if(n.light < 0.999){
					// No sunlight here
					continue;
				}
			}
			catch(InvalidPositionException &e)
			{
				// Assume sunlight, unless probably_dark==true
				if(probably_dark)
					continue;
			}

			has_some_sunlight = true;
			
			// Continue spreading sunlight downwards
			s16 y = MAP_BLOCKSIZE-1;
			for(; y >= 0; y--){
				v3s16 pos(x, y, z);
				
				MapNode *n = getNodePtr(pos);
				if(n == NULL)
					break;

				if(n->transparent())
				{
					n->light = 1.0;
				}
				else{
					break;
				}
			}
			if(y == -1)
				sunlight_touches_bottom = true;
		}
	}

	/*std::cout<<"MapBlock("<<m_pos.X<<","<<m_pos.Y<<","
			<<m_pos.Z<<") has_some_sunlight="
			<<has_some_sunlight<<std::endl;*/
	return sunlight_touches_bottom;
}

u32 MapBlock::serializedLength()
{
	//return 1 + MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE*1;
	return 1 + MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE*1;
}

void MapBlock::serialize(u8 *dest)
{
	dest[0] = probably_dark;
	for(u32 i=0; i<MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE; i++)
	{
		dest[i+1] = data[i].d;
	}
}

void MapBlock::deSerialize(u8 *source)
{
	probably_dark = source[0];
	for(u32 i=0; i<MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE; i++)
	{
		data[i].d = source[i+1];
	}
	setChangedFlag();
}


