#include <iostream>
#include <cstring>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ifaddrs.h>
#endif

std::string getLocalIPAddress() {
    std::string ipAddress = "127.0.0.1";  // 기본값은 localhost

#ifdef _WIN32
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        std::cerr << "Error getting hostname: " << WSAGetLastError() << "\n";
        return ipAddress;
    }

    struct addrinfo hints = {}, * info;
    hints.ai_family = AF_INET;  // IPv4
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, "http", &hints, &info) != 0) {
        std::cerr << "Error getting IP address: " << WSAGetLastError() << "\n";
        return ipAddress;
    }

    for (struct addrinfo* p = info; p != nullptr; p = p->ai_next) {
        struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(p->ai_addr);
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr->sin_addr), ipStr, INET_ADDRSTRLEN);      // IPv4 Get 지점
        // AF_INET = Inaddr.h | AF_INTE6 = In6addr.h
        // AF_INET = IPv4 | AF_INET6 = IPv6
        // AF_INET = IN_ADDR Structure
        // AF_INET6 = IN6_ADDR Structure
        ipAddress = ipStr;                  // IPv4 Return 지점
    }

    freeaddrinfo(info);
#else
    struct ifaddrs* ifAddrStruct = nullptr;
    struct ifaddrs* ifa = nullptr;
    void* tmpAddrPtr = nullptr;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {  // IPv4 주소
            tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if (strcmp(ifa->ifa_name, "lo") != 0) {  // lo 인터페이스는 무시
                ipAddress = addressBuffer;
                break;
            }
        }
}
    if (ifAddrStruct != nullptr) freeifaddrs(ifAddrStruct);
#endif

    return ipAddress;
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }
#endif

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    const char* message = "Hello from server";

    std::string localIPAddress = getLocalIPAddress();
    std::cout << "Local IP Address: " << localIPAddress << std::endl;

    // 소켓 생성
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket failed\n";
        return 1;
    }

    // 포트 재사용 옵션 설정
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt failed\n";
        return 1;
    }

    // 주소와 포트 설정
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);         // Port 설정

    // 소켓을 해당 주소와 포트에 바인딩
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed\n";
        return 1;
    }

    // 클라이언트의 연결을 기다림
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed\n";
        return 1;
    }

    std::cout << "Server listening on port 8080\n";

    // 클라이언트의 연결 요청 수락
    if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        std::cerr << "Accept failed\n";
        return 1;
    }

    // 클라이언트에게 메시지 전송
    send(new_socket, message, strlen(message), 0);
    std::cout << "Message sent to client\n";

    // 소켓 종료
#ifdef _WIN32
    closesocket(new_socket);
    WSACleanup();
#else
    close(new_socket);
#endif

    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();  // 사용자가 Enter 키를 누를 때까지 대기

    return 0;
}