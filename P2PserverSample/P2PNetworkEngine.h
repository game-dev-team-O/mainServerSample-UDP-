#include <iostream>

#include "KCPNet.h"
#include <vector>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <functional>

using namespace std;


class P2PNetworkEngine
{
public:
	P2PNetworkEngine(const char* mainServerIP, int mainServerUDPPort, int mainServerTCPPort, int _id = 0) : isRun(true)
	{
		serverUDPAddr.sin_family = AF_INET;
		serverUDPAddr.sin_port = htons(mainServerUDPPort); // ���� ��Ʈ ��ȣ
		serverUDPAddr.sin_addr.s_addr = inet_addr(mainServerIP); // ���� IP �ּ�

		serverTCPAddr.sin_family = AF_INET;
		serverTCPAddr.sin_port = htons(mainServerTCPPort); // ���� ��Ʈ ��ȣ
		serverTCPAddr.sin_addr.s_addr = inet_addr(mainServerIP); // ���� IP �ּ�


		sockaddr_storage socket_input = {};
		socklen_t socket_input_socklen = sizeof(sockaddr_storage);
		mainServerUDPAddr.adrinf = *((sockaddr_storage*)&serverUDPAddr);
		mainServerUDPAddr.sock_size = socket_input_socklen;


		/////////
		// �̰����� ���μ����� ���� �����Ͽ� (�α��� ��������) id ȹ���ϱ�
		//
		/////////

		id = _id;
		if (lKcpServer.configureKCP(
			bind(&P2PNetworkEngine::onRecvServer, this, placeholders::_1, placeholders::_2, placeholders::_3),
			bind(&P2PNetworkEngine::noConnectionServer, this, placeholders::_1),
			bind(&P2PNetworkEngine::validateConnection, this, placeholders::_1, placeholders::_2, placeholders::_3),
			"127.0.0.1",
			12345)) {
			std::cout << "Failed to configure the KCP Server" << std::endl;
		}


	}


	bool reqP2PServerInfo()
	{
		///////
		//	�̰����� �������� ���� ���� ����� P2P ���� ���� ��û�ϴ� ��Ŷ ������
		//
		///////

		return true;
	}



	bool connectToP2Pserver(const std::string& IP, uint16_t Port, uint32_t ID)
	{
		KCPSettings lSettingsClient;
		if (lKcpClient.configureKCP(
			lSettingsClient,
			bind(&P2PNetworkEngine::OnRecvClient, this, placeholders::_1, placeholders::_2, placeholders::_3),
			bind(&P2PNetworkEngine::noConnectionServer, this, placeholders::_1),
			IP,
			Port,
			ID,
			nullptr)) {
			std::cout << "Failed to configure the KCP Client" << std::endl;
			return false;
		}

		return true;
	}

	bool sendMessageToMain(string message)
	{
		std::memcpy(tempPacket.data(), message.c_str(), message.size());

		lKcpServer.mKissnetSocket.send(tempPacket, message.length(), &mainServerUDPAddr);
		return true;
	}

	bool sendMessageToP2PServer(string message)
	{
		lKcpClient.sendData(message.c_str(), message.length());
		return true;
	}

	bool sendMessageToP2PClient(string message, int _id)
	{
		for (auto user : userMap)
		{
			if (user.second.id == _id)
				lKcpServer.sendData(message.c_str(), message.length(), user.first);
		}

		return true;
	}
	bool sendMessageToP2PClient(string message, vector<KCPContext*>& ids)
	{
		for (auto id : ids)
			lKcpServer.sendData(message.c_str(), message.length(), id);

		return true;
	}

	bool sendMessageToP2PClientAll(string message)
	{
		for (auto user : userMap)
		{
			lKcpServer.sendData(message.c_str(), message.length(), user.first);
		}
	}

	bool sendMessageToP2PClientExcept(string message, int id)
	{
		for (auto user : userMap)
		{
			if (user.second.id == id)
				continue;

			lKcpServer.sendData(message.c_str(), message.length(), user.first);
		}
	}

	bool sendMessageToP2PClientExcept(string message, KCPContext* _user)
	{
		for (auto user : userMap)
		{
			if (user.first == _user)
				continue;

			lKcpServer.sendData(message.c_str(), message.length(), user.first);
		}

	}


	bool Run()
	{
		std::string msg;

		////////
		//
		// �ƹ������� ����ȉ����� �ϴ� ����ϰ��ϰ�
		// ���Ŀ�
		// 
		////////

		while (true)
		{
			std::cin >> msg;

			////////
			//
			//if(isHost)
			//{
			//		sendMessageToP2PClientAll(msg)
			//}
			//else
			//{
			//		sendMessageToP2PServer(msg)
			//}
			//
			// ���� �̷�����
			////////
		}
	}

private:
	void onRecvServer(const char* pData, size_t lSize, KCPContext* pCTX)
	{
		if (userMap.find(pCTX) == userMap.end())
		{
			//error

			return;
		}

		std::string msg;
		userData d = userMap[pCTX];

		msg += to_string(d.id);
		msg += " : ";
		msg += string(pData);

		sendMessageToP2PClientAll(msg);
	}

	void noConnectionServer(KCPContext* pCTX)
	{
		/////////
		// �̰����� ���μ����κ��� ���� ���ʿ����� ��Ƽ� mainServerConnection�� �����ϱ�
		// 
		// 
		/////////

		if (userMap.find(pCTX) == userMap.end())
		{
			//error

			return;
		}

		std::string msg;
		userData d = userMap[pCTX];

		msg += "���� (";
		msg += to_string(d.id);
		msg += ")";

		userMap.erase(pCTX);
		sendMessageToP2PClientAll(msg);
	}

	std::shared_ptr<KCPContext> validateConnection(std::string lIP, uint16_t lPort, std::shared_ptr<KCPContext>& rpCtx)
	{
		std::cout << "Connecting IP:port > " << lIP << ":" << unsigned(lPort) << std::endl;
		rpCtx->mValue = 50;

		userMap[rpCtx.get()].a = 0;
		//userMap[rpCtx.get()].id = 0;

		return rpCtx;
	}







	void OnRecvClient(const char* pData, size_t lSize, KCPContext* pCTX) {
		if (pCTX == mainServerConnection)
		{
			// ���� �������� �޴� ��Ŷ�� ó��
			// 1. �� ȣ��Ʈ��
			//			-> isHost = true
			// 2. ���� ȣ��Ʈ �����ϱ� ���� �ű��
			//			-> connectToP2Pserver(ip, port ~~
			// 3. �� ���̵� ����
			//			-> id = id
			//	���

		}
		else
		{
			std::string s(pData);
			std::cout << s << std::endl;
		}
	}

	static void noConnectionClient(KCPContext* pCTX) {
		std::cout << "The server is not active." << std::endl;
	}







	struct userData {
		int id;
		int a;
	};

	unordered_map<KCPContext*, userData> userMap;

	KCPContext* mainServerConnection;
	kissnet::buffer<1024> tempPacket;

	KCPNetClient lKcpClient;
	KCPNetServer lKcpServer;

	sockaddr_in serverUDPAddr;
	sockaddr_in serverTCPAddr;
	kissnet::addr_collection mainServerUDPAddr;

	string P2PServerIP;
	int P2PServerPort;

	bool isHost;
	bool isRun;
	int id;
};