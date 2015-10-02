#include "test.h"
#include "common_irrlicht.h"

#include "map.h"
#include "player.h"
#include "main.h"
//#include "referencecounted.h"
#include "heightmap.h"
#include "socket.h"
#include "connection.h"
#include "utility.h"

#ifdef _WIN32
	#include <windows.h>
	#define sleep_ms(x) Sleep(x)
#else
	#include <unistd.h>
	#define sleep_ms(x) usleep(x*1000)
#endif

/*
	Asserts that the exception occurs
*/
#define EXCEPTION_CHECK(EType, code)\
{\
	bool exception_thrown = false;\
	try{ code; }\
	catch(EType &e) { exception_thrown = true; }\
	assert(exception_thrown);\
}
		

/*struct TestRefcount
{
	class TC : public ReferenceCounted
	{
	public:
		s16 value;
		TC() : ReferenceCounted("TC")
		{
			value = 0;
		}
	};

	Ref<TC> TestReturn()
	{
		static TC tc;
		tc.value = 5;
		return &tc;
	}

	void Run()
	{
		TC tc;

		tc.grab();
		assert(tc.getRefcount() == 1);

		tc.drop();
		assert(tc.getRefcount() == 0);

		bool exception = false;
		try
		{
			tc.drop();
		}
		catch(ReferenceCounted::Exception & e)
		{
			exception = true;
		}
		assert(exception == true);
		
		TC tc2;
		{
			Ref<TC> ref(&tc2);
			assert(ref.get() == &tc2);
			assert(tc2.getRefcount() == 1);
		}
		assert(tc2.getRefcount() == 0);

		Ref<TC> ref2 = TestReturn();
		assert(ref2.get()->value == 5);
		assert(ref2.get()->getRefcount() == 1);
	}
};*/

struct TestMapNode
{
	void Run()
	{
		MapNode n;

		// Default values
		assert(n.d == MATERIAL_AIR);
		assert(n.light == 0.0);
		
		// Transparency
		n.d = MATERIAL_AIR;
		assert(n.transparent() == true);
		n.d = 0;
		assert(n.transparent() == false);
	}
};

struct TestMapBlock
{
	class TC : public NodeContainer
	{
	public:

		MapNode node;
		bool position_valid;

		TC()
		{
			position_valid = true;
		}

		virtual bool isValidPosition(v3s16 p)
		{
			return position_valid;
		}

		virtual MapNode getNode(v3s16 p)
		{
			if(position_valid == false)
				throw InvalidPositionException();
			return node;
		}

		virtual void setNode(v3s16 p, MapNode & n)
		{
			if(position_valid == false)
				throw InvalidPositionException();
		};
	};

	void Run()
	{
		TC parent;
		
		MapBlock b(&parent, v3s16(1,1,1));
		v3s16 relpos(MAP_BLOCKSIZE, MAP_BLOCKSIZE, MAP_BLOCKSIZE);

		assert(b.getPosRelative() == relpos);

		assert(b.getBox().MinEdge.X == MAP_BLOCKSIZE);
		assert(b.getBox().MaxEdge.X == MAP_BLOCKSIZE*2-1);
		assert(b.getBox().MinEdge.Y == MAP_BLOCKSIZE);
		assert(b.getBox().MaxEdge.Y == MAP_BLOCKSIZE*2-1);
		assert(b.getBox().MinEdge.Z == MAP_BLOCKSIZE);
		assert(b.getBox().MaxEdge.Z == MAP_BLOCKSIZE*2-1);
		
		assert(b.isValidPosition(v3s16(0,0,0)) == true);
		assert(b.isValidPosition(v3s16(-1,0,0)) == false);
		assert(b.isValidPosition(v3s16(-1,-142,-2341)) == false);
		assert(b.isValidPosition(v3s16(-124,142,2341)) == false);
		assert(b.isValidPosition(v3s16(MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1)) == true);
		assert(b.isValidPosition(v3s16(MAP_BLOCKSIZE-1,MAP_BLOCKSIZE,MAP_BLOCKSIZE-1)) == false);

		/*
			TODO: this method should probably be removed
			if the block size isn't going to be set variable
		*/
		assert(b.getSizeNodes() == v3s16(MAP_BLOCKSIZE,
				MAP_BLOCKSIZE, MAP_BLOCKSIZE));
		
		// Changed flag should be initially set
		assert(b.getChangedFlag() == true);
		b.resetChangedFlag();
		assert(b.getChangedFlag() == false);

		// All nodes should have been set to
		// .d=MATERIAL_AIR and .light < 0.001
		for(u16 z=0; z<MAP_BLOCKSIZE; z++)
			for(u16 y=0; y<MAP_BLOCKSIZE; y++)
				for(u16 x=0; x<MAP_BLOCKSIZE; x++){
					assert(b.getNode(v3s16(x,y,z)).d == MATERIAL_AIR);
					assert(b.getNode(v3s16(x,y,z)).light < 0.001);
				}
		
		/*
			Parent fetch functions
		*/
		parent.position_valid = false;
		parent.node.d = 5;

		MapNode n;
		
		// Positions in the block should still be valid
		assert(b.isValidPositionParent(v3s16(0,0,0)) == true);
		assert(b.isValidPositionParent(v3s16(MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1)) == true);
		n = b.getNodeParent(v3s16(0,MAP_BLOCKSIZE-1,0));
		assert(n.d == MATERIAL_AIR);

		// ...but outside the block they should be invalid
		assert(b.isValidPositionParent(v3s16(-121,2341,0)) == false);
		assert(b.isValidPositionParent(v3s16(-1,0,0)) == false);
		assert(b.isValidPositionParent(v3s16(MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1,MAP_BLOCKSIZE)) == false);

		bool exception_thrown = false;
		try{
			// This should throw an exception
			MapNode n = b.getNodeParent(v3s16(0,0,-1));
		}
		catch(InvalidPositionException &e)
		{
			exception_thrown = true;
		}
		assert(exception_thrown);

		parent.position_valid = true;
		// Now the positions outside should be valid
		assert(b.isValidPositionParent(v3s16(-121,2341,0)) == true);
		assert(b.isValidPositionParent(v3s16(-1,0,0)) == true);
		assert(b.isValidPositionParent(v3s16(MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1,MAP_BLOCKSIZE)) == true);
		n = b.getNodeParent(v3s16(0,0,MAP_BLOCKSIZE));
		assert(n.d == 5);

		/*
			Set a node
		*/
		v3s16 p(1,2,0);
		n.d = 4;
		b.setNode(p, n);
		assert(b.getNode(p).d == 4);
		assert(b.getNodeMaterial(p) == 4);
		assert(b.getNodeMaterial(v3s16(-1,-1,0)) == 5);
		
		/*
			propagateSunlight()
		*/
		// Set lighting of all nodes to 0
		for(u16 z=0; z<MAP_BLOCKSIZE; z++){
			for(u16 y=0; y<MAP_BLOCKSIZE; y++){
				for(u16 x=0; x<MAP_BLOCKSIZE; x++){
					MapNode n = b.getNode(v3s16(x,y,z));
					n.light = 0.0;
					b.setNode(v3s16(x,y,z), n);
				}
			}
		}
		parent.position_valid = false;
		b.setProbablyDark(false);
		assert(b.propagateSunlight() == true);
		assert(b.getNode(v3s16(1,4,0)).light > 0.999);
		assert(b.getNode(v3s16(1,3,0)).light > 0.999);
		assert(b.getNode(v3s16(1,2,0)).light < 0.001);
		assert(b.getNode(v3s16(1,1,0)).light < 0.001);
		assert(b.getNode(v3s16(1,0,0)).light < 0.001);
		assert(b.getNode(v3s16(1,2,3)).light > 0.999);
		assert(b.getFaceLight(p, v3s16(0,1,0)) > 0.999);
		assert(b.getFaceLight(p, v3s16(0,-1,0)) < 0.001);
		assert(fabs(b.getFaceLight(p, v3s16(0,0,1))-0.7) < 0.001);
		parent.position_valid = true;
		parent.node.light = 0.500;
		assert(b.propagateSunlight() == false);
		// Should not touch blocks that are not affected (that is, all of them)
		assert(b.getNode(v3s16(1,2,3)).light > 0.999);
	}
};

struct TestMapSector
{
	class TC : public NodeContainer
	{
	public:

		MapNode node;
		bool position_valid;

		TC()
		{
			position_valid = true;
		}

		virtual bool isValidPosition(v3s16 p)
		{
			return position_valid;
		}

		virtual MapNode getNode(v3s16 p)
		{
			if(position_valid == false)
				throw InvalidPositionException();
			return node;
		}

		virtual void setNode(v3s16 p, MapNode & n)
		{
			if(position_valid == false)
				throw InvalidPositionException();
		};
	};
	
	void Run()
	{
		TC parent;
		parent.position_valid = false;

		DummyHeightmap dummyheightmap;
		
		HeightmapBlockGenerator *gen = 
			new HeightmapBlockGenerator(v2s16(1,1), &dummyheightmap);
		MapSector sector(&parent, v2s16(1,1), gen);
		
		EXCEPTION_CHECK(InvalidPositionException, sector.getBlockNoCreate(0));
		EXCEPTION_CHECK(InvalidPositionException, sector.getBlockNoCreate(1));

		MapBlock * bref = sector.createBlankBlock(-2);
		
		EXCEPTION_CHECK(InvalidPositionException, sector.getBlockNoCreate(0));
		assert(sector.getBlockNoCreate(-2) == bref);
		
		//TODO: Check for AlreadyExistsException

		/*bool exception_thrown = false;
		try{
			sector.getBlock(0);
		}
		catch(InvalidPositionException &e){
			exception_thrown = true;
		}
		assert(exception_thrown);*/

	}
};

struct TestHeightmap
{
	void TestSingleFixed()
	{
		const s16 BS1 = 4;
		OneChildHeightmap hm1(BS1);
		
		// Test that it is filled with < GROUNDHEIGHT_VALID_MINVALUE
		for(s16 y=0; y<=BS1; y++){
			for(s16 x=0; x<=BS1; x++){
				v2s16 p(x,y);
				assert(hm1.m_child.getGroundHeight(p)
					< GROUNDHEIGHT_VALID_MINVALUE);
			}
		}

		hm1.m_child.setGroundHeight(v2s16(1,0), 2.0);
		//hm1.m_child.print();
		assert(fabs(hm1.getGroundHeight(v2s16(1,0))-2.0)<0.001);
		hm1.setGroundHeight(v2s16(0,1), 3.0);
		assert(fabs(hm1.m_child.getGroundHeight(v2s16(0,1))-3.0)<0.001);
		
		// Fill with -1.0
		for(s16 y=0; y<=BS1; y++){
			for(s16 x=0; x<=BS1; x++){
				v2s16 p(x,y);
				hm1.m_child.setGroundHeight(p, -1.0);
			}
		}

		f32 corners[] = {0.0, 0.0, 1.0, 1.0};
		hm1.m_child.generateContinued(0.0, 0.0, corners);
		
		hm1.m_child.print();
		assert(fabs(hm1.m_child.getGroundHeight(v2s16(1,0))-0.2)<0.05);
		assert(fabs(hm1.m_child.getGroundHeight(v2s16(4,3))-0.7)<0.05);
		assert(fabs(hm1.m_child.getGroundHeight(v2s16(4,4))-1.0)<0.05);
	}

	void TestUnlimited()
	{
		//g_heightmap_debugprint = true;
		const s16 BS1 = 4;
		UnlimitedHeightmap hm1(BS1, 0.0, 0.0, 5.0);
		// Go through it so it generates itself
		for(s16 y=0; y<=BS1; y++){
			for(s16 x=0; x<=BS1; x++){
				v2s16 p(x,y);
				hm1.getGroundHeight(p);
			}
		}
		// Print it
		std::cout<<"UnlimitedHeightmap hm1:"<<std::endl;
		hm1.print();
		/*for(s16 y=0; y<=BS1; y++){
			for(s16 x=0; x<=BS1; x++){
				v2s16 p(x,y);
				f32 h = hm1.getGroundHeight(p);
				printf("% 2.1f ", h);
			}
			std::cout<<std::endl;
		}*/
		
		std::cout<<"testing UnlimitedHeightmap set/get"<<std::endl;
		v2s16 p1(0,3);
		f32 v1(234.01);
		// Get first heightmap and try setGroundHeight
		FixedHeightmap * href = hm1.getHeightmap(v2s16(0,0));
		href->setGroundHeight(p1, v1);
		// Read from UnlimitedHeightmap
		assert(fabs(hm1.getGroundHeight(p1)-v1)<0.001);
	}
	
	void Random()
	{
		std::cout<<"Running random code"<<std::endl;
		std::cout<<"rand() values: ";
		for(u16 i=0; i<5; i++)
			std::cout<<(u16)rand()<<" ";
		std::cout<<std::endl;

		const s16 BS1 = 4;
		//UnlimitedHeightmap hm1(BS1, 0.0, 0.0, 5.0);
		UnlimitedHeightmap hm1(BS1, 2.5, 0.5, 0.0);

		// Force hm1 to generate a some heightmap
		hm1.getGroundHeight(v2s16(0,0));
		hm1.getGroundHeight(v2s16(BS1,0));
		hm1.print();

		// Get the (0,0) and (1,0) heightmaps
		/*FixedHeightmap * hr00 = hm1.getHeightmap(v2s16(0,0));
		FixedHeightmap * hr01 = hm1.getHeightmap(v2s16(1,0));
		f32 corners[] = {1.0, 1.0, 1.0, 1.0};
		hr00->generateContinued(0.0, 0.0, corners);
		hm1.print();*/
	}

	void Run()
	{
		//srand(7); // Get constant random
		srand(time(0)); // Get better random

		TestSingleFixed();
		TestUnlimited();
		Random();

		g_heightmap_debugprint = false;
		//g_heightmap_debugprint = true;
	}
};

struct TestSocket
{
	void Run()
	{
		const int port = 30003;
		UDPSocket socket;
		socket.Bind(port);

		const char sendbuffer[] = "hello world!";
		socket.Send(Address(127,0,0,1,port), sendbuffer, sizeof(sendbuffer));

		sleep_ms(50);

		char rcvbuffer[256];
		memset(rcvbuffer, 0, sizeof(rcvbuffer));
		Address sender;
		for(;;)
		{
			int bytes_read = socket.Receive(sender, rcvbuffer, sizeof(rcvbuffer));
			if(bytes_read < 0)
				break;
		}
		assert(strncmp(sendbuffer, rcvbuffer, sizeof(rcvbuffer))==0);
		assert(sender.getAddress() == Address(127,0,0,1, 0).getAddress());
	}
};

struct TestConnection
{
	void TestHelpers()
	{
		/*
			Test helper functions
		*/

		// Some constants for testing
		u32 proto_id = 0x12345678;
		u16 peer_id = 123;
		u8 channel = 2;
		SharedBuffer<u8> data1(1);
		data1[0] = 100;
		Address a(127,0,0,1, 10);
		u16 seqnum = 34352;

		con::BufferedPacket p1 = con::makePacket(a, data1,
				proto_id, peer_id, channel);
		/*
			We should now have a packet with this data:
			Header:
				[0] u32 protocol_id
				[4] u16 sender_peer_id
				[6] u8 channel
			Data:
				[7] u8 data1[0]
		*/
		assert(readU32(&p1.data[0]) == proto_id);
		assert(readU16(&p1.data[4]) == peer_id);
		assert(readU8(&p1.data[6]) == channel);
		assert(readU8(&p1.data[7]) == data1[0]);

		SharedBuffer<u8> p2 = con::makeReliablePacket(data1, seqnum);
		assert(readU8(&p2[0]) == TYPE_RELIABLE);
		assert(readU16(&p2[1]) == seqnum);
		assert(readU8(&p2[3]) == data1[0]);
	}
	void Run()
	{
		TestHelpers();

		/*
			Test some real connections
		*/
		u32 proto_id = 0xad26846a;
		
		std::cout<<"** Creating server Connection"<<std::endl;
		con::Connection server(proto_id, 512);
		server.Serve(30001);
		
		std::cout<<"** Creating client Connection"<<std::endl;
		con::Connection client(proto_id, 512);
		
		sleep_ms(50);
		
		Address server_address(127,0,0,1, 30001);
		std::cout<<"** running client.Connect()"<<std::endl;
		client.Connect(server_address);

		sleep_ms(50);
		
		try
		{
			u16 peer_id;
			u8 data[100];
			std::cout<<"** running server.Receive()"<<std::endl;
			u32 size = server.Receive(peer_id, data, 100);
			std::cout<<"** Server received: peer_id="<<peer_id
					<<", size="<<size
					<<std::endl;
		}
		catch(con::NoIncomingDataException &e)
		{
			// No actual data received, but the client has
			// probably been connected
		}

		//sleep_ms(50);

		while(client.Connected() == false)
		{
			try
			{
				u16 peer_id;
				u8 data[100];
				std::cout<<"** running client.Receive()"<<std::endl;
				u32 size = client.Receive(peer_id, data, 100);
				std::cout<<"** Client received: peer_id="<<peer_id
						<<", size="<<size
						<<std::endl;
			}
			catch(con::NoIncomingDataException &e)
			{
			}
			sleep_ms(50);
		}

		sleep_ms(50);
		
		try
		{
			u16 peer_id;
			u8 data[100];
			std::cout<<"** running server.Receive()"<<std::endl;
			u32 size = server.Receive(peer_id, data, 100);
			std::cout<<"** Server received: peer_id="<<peer_id
					<<", size="<<size
					<<std::endl;
		}
		catch(con::NoIncomingDataException &e)
		{
		}

		{
			/*u8 data[] = "Hello World!";
			u32 datasize = sizeof(data);*/
			SharedBuffer<u8> data = SharedBufferFromString("Hello World!");

			std::cout<<"** running client.Send()"<<std::endl;
			client.Send(PEER_ID_SERVER, 0, data, true);

			sleep_ms(50);

			u16 peer_id;
			u8 recvdata[100];
			std::cout<<"** running server.Receive()"<<std::endl;
			u32 size = server.Receive(peer_id, recvdata, 100);
			std::cout<<"** Server received: peer_id="<<peer_id
					<<", size="<<size
					<<", data="<<*data
					<<std::endl;
			assert(memcmp(*data, recvdata, data.getSize()) == 0);
		}
		
		u16 peer_id_client = 2;

		{
			/*
				Send consequent packets in different order
			*/
			//u8 data1[] = "hello1";
			//u8 data2[] = "hello2";
			SharedBuffer<u8> data1 = SharedBufferFromString("hello1");
			SharedBuffer<u8> data2 = SharedBufferFromString("Hello2");

			Address client_address =
					server.GetPeer(peer_id_client)->address;
			
			std::cout<<"*** Sending packets in wrong order (2,1,2)"
					<<std::endl;
			
			u8 chn = 0;
			con::Channel *ch = &server.GetPeer(peer_id_client)->channels[chn];
			u16 sn = ch->next_outgoing_seqnum;
			ch->next_outgoing_seqnum = sn+1;
			server.Send(peer_id_client, chn, data2, true);
			ch->next_outgoing_seqnum = sn;
			server.Send(peer_id_client, chn, data1, true);
			ch->next_outgoing_seqnum = sn+1;
			server.Send(peer_id_client, chn, data2, true);

			sleep_ms(50);

			std::cout<<"*** Receiving the packets"<<std::endl;

			u16 peer_id;
			u8 recvdata[20];
			u32 size;

			std::cout<<"** running client.Receive()"<<std::endl;
			peer_id = 132;
			size = client.Receive(peer_id, recvdata, 20);
			std::cout<<"** Client received: peer_id="<<peer_id
					<<", size="<<size
					<<", data="<<recvdata
					<<std::endl;
			assert(size == data1.getSize());
			assert(memcmp(*data1, recvdata, data1.getSize()) == 0);
			assert(peer_id == PEER_ID_SERVER);
			
			std::cout<<"** running client.Receive()"<<std::endl;
			peer_id = 132;
			size = client.Receive(peer_id, recvdata, 20);
			std::cout<<"** Client received: peer_id="<<peer_id
					<<", size="<<size
					<<", data="<<recvdata
					<<std::endl;
			assert(size == data2.getSize());
			assert(memcmp(*data2, recvdata, data2.getSize()) == 0);
			assert(peer_id == PEER_ID_SERVER);
			
			bool got_exception = false;
			try
			{
				std::cout<<"** running client.Receive()"<<std::endl;
				peer_id = 132;
				size = client.Receive(peer_id, recvdata, 20);
				std::cout<<"** Client received: peer_id="<<peer_id
						<<", size="<<size
						<<", data="<<recvdata
						<<std::endl;
			}
			catch(con::NoIncomingDataException &e)
			{
				std::cout<<"** No incoming data for client"<<std::endl;
				got_exception = true;
			}
			assert(got_exception);
		}
		{
			//u8 data1[1100];
			SharedBuffer<u8> data1(1100);
			for(u16 i=0; i<1100; i++){
				data1[i] = i/4;
			}

			std::cout<<"Sending data (size="<<1100<<"):";
			for(int i=0; i<1100 && i<20; i++){
				if(i%2==0) printf(" ");
				printf("%.2X", ((int)((const char*)*data1)[i])&0xff);
			}
			if(1100>20)
				std::cout<<"...";
			std::cout<<std::endl;
			
			server.Send(peer_id_client, 0, data1, true);

			sleep_ms(50);
			
			u8 recvdata[2000];
			std::cout<<"** running client.Receive()"<<std::endl;
			u16 peer_id = 132;
			u16 size = client.Receive(peer_id, recvdata, 2000);
			std::cout<<"** Client received: peer_id="<<peer_id
					<<", size="<<size
					<<std::endl;

			std::cout<<"Received data (size="<<size<<"):";
			for(int i=0; i<size && i<20; i++){
				if(i%2==0) printf(" ");
				printf("%.2X", ((int)((const char*)recvdata)[i])&0xff);
			}
			if(size>20)
				std::cout<<"...";
			std::cout<<std::endl;

			assert(memcmp(*data1, recvdata, data1.getSize()) == 0);
			assert(peer_id == PEER_ID_SERVER);
		}
		
		//assert(0);
	}
};

#define TEST(X)\
{\
	X x;\
	std::cout<<"Running " #X <<std::endl;\
	x.Run();\
}

void run_tests()
{
	std::cout<<"run_tests() started"<<std::endl;
	//TEST(TestRefcount);
	TEST(TestMapNode);
	TEST(TestMapBlock);
	TEST(TestMapSector);
	TEST(TestHeightmap);
	if(INTERNET_SIMULATOR == false){
		TEST(TestSocket);
		TEST(TestConnection);
	}
	std::cout<<"run_tests() passed"<<std::endl;
}

