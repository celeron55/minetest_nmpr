#ifndef SERVER_HEADER
#define SERVER_HEADER

#include "connection.h"
#include "environment.h"

class Server;

class ServerNetworkThread : public JThread
{
	bool run;
	JMutex run_mutex;

	Server *m_server;

public:

	ServerNetworkThread(Server *server) : JThread(), run(true), m_server(server)
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

class Server
{
public:
	/*
		NOTE: Every public method should be thread-safe
	*/
	Server();
	~Server();
	void start(unsigned short port);
	void stop();
	void step(float dtime);
	void AsyncRunStep();
	void Receive();
	void ProcessData(u8 *data, u32 datasize, u16 peer_id);
private:
	
	void SendPlayerPositions(float dtime);
	
	Environment m_env;
	JMutex m_env_mutex;

	con::Connection m_con;
	JMutex m_con_mutex;

	float m_step_dtime;
	JMutex m_step_dtime_mutex;

	ServerNetworkThread m_thread;
};

#endif

