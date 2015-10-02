#include "environment.h"

Environment::Environment(Map *map, std::ostream &dout):
		m_dout(dout)
{
	m_map = map;
}

Environment::~Environment()
{
	// Deallocate players
	for(core::list<Player*>::Iterator i = m_players.begin();
			i != m_players.end(); i++)
	{
		delete (*i);
	}
}

void Environment::step(float dtime)
{
	// Increment timeout of players
	// TODO: Must reset the timeout somewhere, too.
	for(core::list<Player*>::Iterator i = m_players.begin();
			i != m_players.end(); i++)
	{
		Player *player = *i;
		player->timeout_counter += dtime;
	}
	// Remove timed-out players
removed:
	for(core::list<Player*>::Iterator i = m_players.begin();
			i != m_players.end(); i++)
	{
		Player *player = *i;

		// Don't remove local player
		if(player->isLocal())
			continue;

		// 5 seconds is fine, the player will spawn again with no
		// problems anyway
		if(player->timeout_counter > 5.0)
		{
			m_dout<<"Environment: Removing timed-out player "
					<<player->peer_id<<std::endl;
			delete player;
			m_players.erase(i);
			goto removed;
		}
	}

	f32 maximum_player_speed = 0.001; // just some small value
	for(core::list<Player*>::Iterator i = m_players.begin();
			i != m_players.end(); i++)
	{
		f32 speed = (*i)->speed.getLength();
		if(speed > maximum_player_speed)
			maximum_player_speed = speed;
	}
	
	// Maximum time increment (for collision detection etc)
	// Allow 0.1 blocks per increment
	// time = distance / speed
	f32 dtime_max_increment = 0.1*BS / maximum_player_speed;
	// Maximum time increment is 10ms or lower
	if(dtime_max_increment > 0.01)
		dtime_max_increment = 0.01;

	/*
		Stuff that has a maximum time increment
	*/
	// Don't allow overly too much dtime
	if(dtime > 0.5)
		dtime = 0.5;
	do
	{
		f32 dtime_part;
		if(dtime > dtime_max_increment)
			dtime_part = dtime_max_increment;
		else
			dtime_part = dtime;
		dtime -= dtime_part;
		
		/*
			Move players
		*/
		for(core::list<Player*>::Iterator i = m_players.begin();
				i != m_players.end(); i++)
		{
			Player *player = *i;
			player->speed.Y -= 9.81 * BS * dtime_part * 2;
			player->move(dtime_part, *m_map);
		}
	}
	while(dtime > 0.001);

}

Map & Environment::getMap()
{
	return *m_map;
}

void Environment::addPlayer(Player *player)
{
	//Check that only one local player exists and peer_ids are unique
	assert(player->isLocal() == false || getLocalPlayer() == NULL);
	assert(getPlayer(player->peer_id) == NULL);
	m_players.push_back(player);
}

void Environment::removePlayer(Player *player)
{
	//TODO
	throw;
}

Player * Environment::getLocalPlayer()
{
	for(core::list<Player*>::Iterator i = m_players.begin();
			i != m_players.end(); i++)
	{
		Player *player = *i;
		if(player->isLocal())
			return player;
	}
	return NULL;
}

Player * Environment::getPlayer(u16 peer_id)
{
	for(core::list<Player*>::Iterator i = m_players.begin();
			i != m_players.end(); i++)
	{
		Player *player = *i;
		if(player->peer_id == peer_id)
			return player;
	}
	return NULL;
}

core::list<Player*> Environment::getPlayers()
{
	return m_players;
}

