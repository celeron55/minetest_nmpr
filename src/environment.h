#ifndef ENVIRONMENT_HEADER
#define ENVIRONMENT_HEADER

/*
	This class is the game's environment.
	It contains:
	- The map
	- Players
	- Other objects
	- The current time in the game, etc.
*/

#include <list>
#include "common_irrlicht.h"
#include "player.h"
#include "map.h"
#include <ostream>

class Environment
{
public:
	// Environment will delete the map passed to the constructor
	Environment(Map *map, std::ostream &dout);
	~Environment();
	/*
		This can do anything to the environment, such as removing
		timed-out players.
	*/
	void step(f32 dtime);

	Map & getMap();
	/*
		Environment deallocates players after use.
	*/
	void addPlayer(Player *player);
	void removePlayer(Player *player);
	Player * getLocalPlayer();
	Player * getPlayer(u16 peer_id);
	core::list<Player*> getPlayers();
private:
	Map *m_map;
	core::list<Player*> m_players;
	// Debug output goes here
	std::ostream &m_dout;
};

#endif

