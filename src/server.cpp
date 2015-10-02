#include "server.h"
#include "utility.h"
#include <iostream>
#include "clientserver.h"
#include "map.h"
#include <jthread/jmutexautolock.h>
namespace jthread {} // JThread 1.2 support
using namespace jthread; // JThread 1.3 support
#include "main.h"

#ifdef _WIN32
	#include <windows.h>
	#define sleep_ms(x) Sleep(x)
#else
	#include <unistd.h>
	#define sleep_ms(x) usleep(x*1000)
#endif

void * ServerNetworkThread::Thread()
{
	ThreadStarted();

	while(getRun())
	{
		m_server->AsyncRunStep();
		
		try{
			//dout_server<<"Running m_server->Receive()"<<std::endl;
			m_server->Receive();
		}
		catch(con::NoIncomingDataException &e)
		{
		}
		catch(std::exception &e)
		{
			dout_server<<"ServerNetworkThread: Some exception: "
					<<e.what()<<std::endl;
		}
	}
	

	return NULL;
}


Server::Server():
	m_env(new MasterMap(), dout_server),
	m_con(PROTOCOL_ID, 512),
	m_thread(this)
{
	m_env_mutex.Init();
	m_con_mutex.Init();
	m_step_dtime_mutex.Init();
	m_step_dtime = 0.0;
}

Server::~Server()
{
	stop();
}

void Server::start(unsigned short port)
{
	stop();
	m_con.setTimeoutMs(200);
	m_con.Serve(port);
	m_thread.setRun(true);
	m_thread.Start();
}

void Server::stop()
{
	m_thread.setRun(false);
	while(m_thread.IsRunning())
		sleep_ms(100);
}

void Server::step(float dtime)
{
	{
		JMutexAutoLock lock(m_env_mutex);
		m_env.step(dtime);
	}
	{
		JMutexAutoLock lock(m_step_dtime_mutex);
		m_step_dtime += dtime;
	}
}

void Server::AsyncRunStep()
{
	float dtime;
	{
		JMutexAutoLock lock1(m_step_dtime_mutex);
		dtime = m_step_dtime;
		m_step_dtime = 0.0;
	}
	{
		// Process connection's timeouts
		JMutexAutoLock lock2(m_con_mutex);
		m_con.RunTimeouts(dtime);
	}
	{
		/*
			Do background stuff that needs the connection
		*/
		
		// Send player positions
		SendPlayerPositions(dtime);
	}
}

void Server::Receive()
{
	u32 data_maxsize = 10000;
	Buffer<u8> data(data_maxsize);
	u16 peer_id;
	u32 datasize;
	try{
		{
			JMutexAutoLock lock(m_con_mutex);
			datasize = m_con.Receive(peer_id, *data, data_maxsize);
		}
		ProcessData(*data, datasize, peer_id);
	}
	catch(con::InvalidIncomingDataException &e)
	{
		dout_server<<"Server::Receive(): "
				"InvalidIncomingDataException: what()="
				<<e.what()<<std::endl;
	}
}

void Server::ProcessData(u8 *data, u32 datasize, u16 peer_id)
{
	// Let free access to the environment and the connection
	JMutexAutoLock envlock(m_env_mutex);
	JMutexAutoLock conlock(m_con_mutex);
	
	try
	{

	if(datasize < 2)
		return;

	ToServerCommand command = (ToServerCommand)readU16(&data[0]);

	if(command == TOSERVER_GETBLOCK)
	{
		//throw NotImplementedException("");

		// Check for too short data
		if(datasize < 8)
			return;
		dout_server<<"Server::ProcessData(): GETBLOCK"
				<<std::endl;
		/*
			Get block data and send it
		*/
		v3s16 p;
		p.X = readS16(&data[2]);
		p.Y = readS16(&data[4]);
		p.Z = readS16(&data[6]);
		MapBlock *block = m_env.getMap().getBlock(p);
	
		u32 replysize = 8 + MapBlock::serializedLength();
		SharedBuffer<u8> reply(replysize);
		writeU16(&reply[0], TOCLIENT_BLOCKDATA);
		writeS16(&reply[2], p.X);
		writeS16(&reply[4], p.Y);
		writeS16(&reply[6], p.Z);
		block->serialize(&reply[8]);
		// Send as unreliable
		//m_con.Send(peer_id, 1, reply, false);
		m_con.Send(peer_id, 1, reply, true);
	}
	else if(command == TOSERVER_REMOVENODE)
	{
		if(datasize < 8)
			return;
			
		v3s16 p;
		p.X = readS16(&data[2]);
		p.Y = readS16(&data[4]);
		p.Z = readS16(&data[6]);

		MapNode n;
		n.d = MATERIAL_AIR;
		m_env.getMap().setNode(p, n);

		u32 replysize = 8;
		//u8 reply[replysize];
		SharedBuffer<u8> reply(replysize);
		writeU16(&reply[0], TOCLIENT_REMOVENODE);
		writeS16(&reply[2], p.X);
		writeS16(&reply[4], p.Y);
		writeS16(&reply[6], p.Z);
		// Send as reliable
		m_con.SendToAll(0, reply, true);
	}
	else if(command == TOSERVER_ADDNODE)
	{
		if(datasize < 8 + MapNode::serializedLength())
			return;

		v3s16 p;
		p.X = readS16(&data[2]);
		p.Y = readS16(&data[4]);
		p.Z = readS16(&data[6]);

		MapNode n;
		n.deSerialize(&data[8]);
		m_env.getMap().setNode(p, n);

		u32 replysize = 8 + MapNode::serializedLength();
		//u8 reply[replysize];
		SharedBuffer<u8> reply(replysize);
		writeU16(&reply[0], TOCLIENT_ADDNODE);
		writeS16(&reply[2], p.X);
		writeS16(&reply[4], p.Y);
		writeS16(&reply[6], p.Z);
		n.serialize(&reply[8]);
		// Send as reliable
		m_con.SendToAll(0, reply, true);
	}
	else if(command == TOSERVER_PLAYERPOS)
	{
		if(datasize < 2+12+12)
			return;

		Player *player = m_env.getPlayer(peer_id);

		// Create a player if it doesn't exist
		if(player == NULL)
		{
			dout_server<<"Server::ProcessData(): Adding player "
					<<peer_id<<std::endl;
			player = new Player(false);
			player->peer_id = peer_id;
			m_env.addPlayer(player);
		}

		player->timeout_counter = 0.0;
		
		v3s32 ps = readV3S32(&data[2]);
		v3s32 ss = readV3S32(&data[2+12]);
		v3f position((f32)ps.X/100., (f32)ps.Y/100., (f32)ps.Z/100.);
		v3f speed((f32)ss.X/100., (f32)ss.Y/100., (f32)ss.Z/100.);
		player->setPosition(position);
		player->speed = speed;

		/*dout_server<<"Server::ProcessData(): Moved player "<<peer_id
				<<" to ("<<position.X
				<<","<<position.Y
				<<","<<position.Z
				<<")"<<std::endl;*/
	}
	else
	{
		dout_server<<"WARNING: Server::ProcessData(): Ingoring "
				"unknown command "<<command<<std::endl;
	}
	
	} //try
	catch(SendFailedException &e)
	{
		dout_server<<"Server::ProcessData(): SendFailedException: "
				<<"what="<<e.what()
				<<std::endl;
	}
}

void Server::SendPlayerPositions(float dtime)
{
	// Update at reasonable intervals (0.2s)
	static float counter = 0.0;
	counter += dtime;
	if(counter < 0.2)
		return;
	counter = 0.0;

	JMutexAutoLock envlock(m_env_mutex);
	
	core::list<Player*> players = m_env.getPlayers();
	
	u32 player_count = players.getSize();
	u32 datasize = 2+(2+12+12)*player_count;

	SharedBuffer<u8> data(datasize);
	writeU16(&data[0], TOCLIENT_PLAYERPOS);
	
	u32 start = 2;
	core::list<Player*>::Iterator i;
	for(i = players.begin();
			i != players.end(); i++)
	{
		Player *player = *i;
		
		v3f pf = player->getPosition();
		v3s32 position(pf.X*100, pf.Y*100, pf.Z*100);
		v3f sf = player->speed;
		v3s32 speed(sf.X*100, sf.Y*100, sf.Z*100);

		writeU16(&data[start], player->peer_id);
		writeV3S32(&data[start+2], position);
		writeV3S32(&data[start+2+12], speed);
		start += 2+12+12;
	}

	JMutexAutoLock conlock(m_con_mutex);

	// Send as unreliable
	m_con.SendToAll(0, data, false);
}

