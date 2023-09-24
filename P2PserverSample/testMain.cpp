#include <iostream>

#include "KCPNet.h"
#include <vector>
#include <algorithm>

std::shared_ptr<KCPContext> gRetainThis;

int gPacketNum = 0;

// Validate this connection
// Return a nullptr if you want to reject the new connection
// If you want to retain this connection context this is the place to do that. All other calls use the raw pointer to the object.
// You can also skip retaining this and just pass a smart_pointer to std::any within the KCPContext
std::shared_ptr<KCPContext> validateConnection(std::string lIP, uint16_t lPort, std::shared_ptr<KCPContext>& rpCtx) {
    std::cout << "Connecting IP:port > " << lIP << ":" << unsigned(lPort) << std::endl;
    rpCtx->mValue = 50;
    gRetainThis = rpCtx;

    // You can optionally configure the KCP connection here also.
    //rpCtx->mSettings.mMtu = 1000;

    // And set the wanted ID.
    //rpCtx->mID = 10; (10 is default)

    return rpCtx;
}

void gotDataServer(const char* pData, size_t lSize, KCPContext* pCTX) {
    std::cout << "The server got -> " << unsigned(lSize) << " bytes of data. pk num -> " << gPacketNum++ << std::endl;
}

void gotDataClient(const char* pData, size_t lSize, KCPContext* pCTX) {
    std::cout << "The client got -> " << unsigned(lSize) << " bytes of data" << std::endl;
}

void noConnectionServer(KCPContext* pCTX) {
    std::cout << "The server timed out a client." << std::endl;
}

void noConnectionClient(KCPContext* pCTX) {
    std::cout << "The server is not active." << std::endl;
}

class TestClass {
    int mTestValue = 100;
};

void myTest()
{

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(13385); // 서버 포트 번호
    serverAddr.sin_addr.s_addr = inet_addr("1.243.174.63"); // 서버 IP 주소
    /*
    // 소켓 생성
    SOCKET clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "소켓 생성 실패" << std::endl;
        WSACleanup();
    }

    const char* message = "안녕, 서버!";
    int messageLength = strlen(message);
    int bytesSent = sendto(clientSocket, message, messageLength, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    */



    KCPNetClient lKcpClient;
    KCPSettings lSettingsClient;
    if (lKcpClient.configureKCP(lSettingsClient,
        gotDataClient,
        noConnectionClient,
        "127.0.0.1",
        12346,
        10,
        nullptr)) {
        std::cout << "Failed to configure the KCP Client" << std::endl;
    }

    const std::string global_message = "This is a multicast message payload";
    kissnet::buffer<100> send_this;
    std::memcpy(send_this.data(), global_message.c_str(), global_message.size());
    kissnet::addr_collection mainServerAddr;
    sockaddr_storage socket_input = {};
    socklen_t socket_input_socklen = sizeof(sockaddr_storage);
    mainServerAddr.adrinf = *((sockaddr_storage*)&serverAddr);
    mainServerAddr.sock_size = socket_input_socklen;

    //addr_info->adrinf = socket_input;
    //addr_info->sock_size = socket_input_socklen;

    auto [sent_bytes, status] = lKcpClient.mKissnetSocket.send(send_this, global_message.length(), &mainServerAddr);
    if (sent_bytes != sizeof(send_this) || status != kissnet::socket_status::valid) {
        std::cout << "kissnet multicast send failure" << std::endl;
    }
    std::cout << "Sent multicast packet." << std::endl;
}


#include <iostream>
#include <Winsock2.h>

#pragma comment(lib, "ws2_32.lib")
SOCKET serverSocket;

int UDPrecv(KCPNetClient& connection) {
    // Winsock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup 실패" << std::endl;
        return 1;
    }

    // 서버 주소 설정
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345); // 서버 포트 번호
    serverAddr.sin_addr.s_addr = INADDR_ANY; // 모든 인터페이스에서 수신

    // 소켓 생성
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "소켓 생성 실패" << std::endl;
        WSACleanup();
        return 1;
    }

    // 소켓 바인딩
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "바인딩 실패" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "서버 대기 중..." << std::endl;

    // 클라이언트 메시지 수신 및 응답
    char buffer[1024];
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    int bytesReceived = recvfrom(serverSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (bytesReceived == SOCKET_ERROR) {
        int c = GetLastError();
        std::cout << c << std::endl;
        std::cerr << "메시지 수신 실패" << std::endl;
    }
    else {
        buffer[bytesReceived] = '\0';
        std::cout << "클라이언트 메시지: " << buffer << std::endl;

        // 클라이언트에 응답
        //const char* response = "안녕, 클라이언트!";
        //int responseLength = strlen(response);
        //int bytesSent = sendto(serverSocket, response, responseLength, 0, (struct sockaddr*)&clientAddr, clientAddrLen);
        //if (bytesSent == SOCKET_ERROR) {
        //    std::cerr << "응답 전송 실패" << std::endl;
        //}
        //else {
        //    std::cout << "응답 전송 완료" << std::endl;
        //}
    }

    int port = ntohs(clientAddr.sin_port);
    std::string ip = inet_ntoa(clientAddr.sin_addr);

    KCPSettings lSettingsClient;
    if (connection.configureKCP(lSettingsClient,
        gotDataClient,
        noConnectionClient,
        ip,
        port,
        10,
        nullptr)) {
        std::cout << "Failed to configure the KCP Client" << std::endl;
    }

    // 소켓 닫기
    closesocket(serverSocket);
    //WSACleanup();

    return 0;
}

int main() {
    std::string  test = "";
    KCPNetClient connection;

    UDPrecv(connection);

    KCPSettings lSettingsClient;
    if (connection.configureKCP(lSettingsClient,
        gotDataClient,
        noConnectionClient,
        "127.0.0.1",
        8000,
        10,
        nullptr)) {
        std::cout << "Failed to configure the KCP Client" << std::endl;
    }


    for(int i=0; i<1000; i++)
    {
        std::string temp = std::to_string(i);
        connection.sendData(temp.c_str(), temp.length());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
