/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#include "player.h"
#include "map.h"
#include "connection.h"

Player::Player(bool is_local):
	scene::ISceneNode(NULL, NULL, 0),
	speed(0,0,0),
	touching_ground(false),
	peer_id(PEER_ID_NEW),
	timeout_counter(0.0),
	m_is_local(is_local),
	m_position(0,0,0)
{
}

Player::Player(
		bool is_local,
		scene::ISceneNode* parent,
		scene::ISceneManager* mgr,
		s32 id):
	scene::ISceneNode(parent, mgr, id),
	speed(0,0,0),
	touching_ground(false),
	peer_id(PEER_ID_NEW),
	timeout_counter(0.0),
	m_is_local(is_local),
	m_position(0,0,0)
{
	m_box = core::aabbox3d<f32>(-BS,-BS,-BS,BS,BS,BS);
	
	m_bill = NULL;

	if(is_local == false)
	{
		// attach billboard to the light
		m_bill = mgr->addBillboardSceneNode(this, core::dimension2d<f32>(60, 60));

		// ISceneNode stores a member called SceneManager
		video::IVideoDriver* driver = SceneManager->getVideoDriver();
		
		m_bill->setMaterialFlag(video::EMF_LIGHTING, false);
		//m_bill->setMaterialFlag(video::EMF_ZWRITE_ENABLE, false);
		//m_bill->setMaterialType(video::EMT_TRANSPARENT_ADD_COLOR);
		m_bill->setMaterialTexture(0, driver->getTexture("../data/tf.jpg"));
		m_bill->setSize(core::dimension2d<f32>(BS,BS));

		m_bill->setPosition(v3f(0, BS+BS/3, 0));

		updateSceneNodePosition();
	}
}

Player::~Player()
{
	if(SceneManager != NULL)
		ISceneNode::remove();
}

void Player::move(f32 dtime, Map &map)
{
	v3f position = getPosition();
	v3f oldpos = position;
	v3s16 oldpos_i = Map::floatToInt(oldpos);

	/*std::cout<<"oldpos_i=("<<oldpos_i.X<<","<<oldpos_i.Y<<","
			<<oldpos_i.Z<<")"<<std::endl;*/

	position += speed * dtime;

	v3s16 pos_i = Map::floatToInt(position);
	
	// The frame length is limited to the player going 0.1*BS per call
	f32 d = (float)BS * 0.15;

#define PLAYER_RADIUS (BS*0.3)
#define PLAYER_HEIGHT (BS*1.7)

	core::aabbox3d<f32> playerbox(
		position.X - PLAYER_RADIUS,
		position.Y - 0.0,
		position.Z - PLAYER_RADIUS,
		position.X + PLAYER_RADIUS,
		position.Y + PLAYER_HEIGHT,
		position.Z + PLAYER_RADIUS
	);
	core::aabbox3d<f32> playerbox_old(
		oldpos.X - PLAYER_RADIUS,
		oldpos.Y - 0.0,
		oldpos.Z - PLAYER_RADIUS,
		oldpos.X + PLAYER_RADIUS,
		oldpos.Y + PLAYER_HEIGHT,
		oldpos.Z + PLAYER_RADIUS
	);

	//hilightboxes.push_back(playerbox);

	touching_ground = false;
	
	/*std::cout<<"Checking collisions for ("
			<<oldpos_i.X<<","<<oldpos_i.Y<<","<<oldpos_i.Z
			<<") -> ("
			<<pos_i.X<<","<<pos_i.Y<<","<<pos_i.Z
			<<"):"<<std::endl;*/

	for(s16 y = oldpos_i.Y - 1; y <= oldpos_i.Y + 2; y++){
		for(s16 z = oldpos_i.Z - 1; z <= oldpos_i.Z + 1; z++){
			for(s16 x = oldpos_i.X - 1; x <= oldpos_i.X + 1; x++){
				//std::cout<<"with ("<<x<<","<<y<<","<<z<<"): ";
				try{
					if(map.getNode(x,y,z).d == MATERIAL_AIR){
						//std::cout<<"air."<<std::endl;
						continue;
					}
				}
				catch(InvalidPositionException &e)
				{
					// Doing nothing here will block the player from
					// walking over map borders
				}

				core::aabbox3d<f32> nodebox = Map::getNodeBox(
						v3s16(x,y,z));
				
				// See if the player is touching ground
				if(
						/*(nodebox.MaxEdge.Y+d > playerbox.MinEdge.Y &&
						nodebox.MaxEdge.Y-d < playerbox.MinEdge.Y)*/
						fabs(nodebox.MaxEdge.Y-playerbox.MinEdge.Y) < d
						&& nodebox.MaxEdge.X-d > playerbox.MinEdge.X
						&& nodebox.MinEdge.X+d < playerbox.MaxEdge.X
						&& nodebox.MaxEdge.Z-d > playerbox.MinEdge.Z
						&& nodebox.MinEdge.Z+d < playerbox.MaxEdge.Z
				){
					touching_ground = true;
				}
				
				if(playerbox.intersectsWithBox(nodebox))
				{
				
	v3f dirs[3] = {
		v3f(0,0,1), // back
		v3f(0,1,0), // top
		v3f(1,0,0), // right
	};
	for(u16 i=0; i<3; i++)
	{
		f32 nodemax = nodebox.MaxEdge.dotProduct(dirs[i]);
		f32 nodemin = nodebox.MinEdge.dotProduct(dirs[i]);
		f32 playermax = playerbox.MaxEdge.dotProduct(dirs[i]);
		f32 playermin = playerbox.MinEdge.dotProduct(dirs[i]);
		f32 playermax_old = playerbox_old.MaxEdge.dotProduct(dirs[i]);
		f32 playermin_old = playerbox_old.MinEdge.dotProduct(dirs[i]);

		bool main_edge_collides = 
			((nodemax > playermin && nodemax <= playermin_old + d
				&& speed.dotProduct(dirs[i]) < 0)
			||
			(nodemin < playermax && nodemin >= playermax_old - d
				&& speed.dotProduct(dirs[i]) > 0));

		bool other_edges_collide = true;
		for(u16 j=0; j<3; j++)
		{
			if(j == i)
				continue;
			f32 nodemax = nodebox.MaxEdge.dotProduct(dirs[j]);
			f32 nodemin = nodebox.MinEdge.dotProduct(dirs[j]);
			f32 playermax = playerbox.MaxEdge.dotProduct(dirs[j]);
			f32 playermin = playerbox.MinEdge.dotProduct(dirs[j]);
			if(!(nodemax - d > playermin && nodemin + d < playermax))
			{
				other_edges_collide = false;
				break;
			}
		}
		
		if(main_edge_collides && other_edges_collide)
		{
			speed -= speed.dotProduct(dirs[i]) * dirs[i];
			position -= position.dotProduct(dirs[i]) * dirs[i];
			position += oldpos.dotProduct(dirs[i]) * dirs[i];
		}
	
	}
				} // if(playerbox.intersectsWithBox(nodebox))
			} // for x
		} // for z
	} // for y

	setPosition(position);
}

