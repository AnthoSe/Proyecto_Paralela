#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <fstream>
#include <string>

static const int SERVER_PORT = 5000;
static const char* LOG_FILE = "logs_resultados.txt";

// Inicializa Winsock
bool init_winsock() {
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0) {
        std::cerr << "WSAStartup falló: " << res << "\n";
        return false;
    }
    return true;
}

// Envía un archivo completo al cliente
bool send_file(SOCKET clientSock, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[SERVIDOR] No se pudo abrir el archivo: "
            << filename << "\n";
        return false;
    }

    char buffer[4096];
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));
        std::streamsize bytesRead = file.gcount();
        if (bytesRead <= 0) break;

        int sent = send(clientSock, buffer, static_cast<int>(bytesRead), 0);
        if (sent == SOCKET_ERROR) {
            std::cerr << "[SERVIDOR] Error en send: "
                << WSAGetLastError() << "\n";
            file.close();
            return false;
        }
    }

    file.close();
    return true;
}

// Añade una línea al archivo de logs
void append_log(const std::string& logLine) {
    std::ofstream log(LOG_FILE, std::ios::app);
    if (!log.is_open()) {
        std::cerr << "[SERVIDOR] No se pudo abrir archivo de log.\n";
        return;
    }
    log << logLine << "\n";
    log.close();
}

// Lee una línea terminada en '\n'
bool recv_line(SOCKET sock, std::string& outLine) {
    outLine.clear();
    char ch;
    while (true) {
        int res = recv(sock, &ch, 1, 0);
        if (res <= 0) return false;
        if (ch == '\n') break;
        outLine.push_back(ch);
    }
    return true;
}

void handle_client(SOCKET clientSock) {
    std::string line;

    if (!recv_line(clientSock, line)) {
        std::cerr << "[SERVIDOR] No se pudo leer comando.\n";
        return;
    }

    std::cout << "[SERVIDOR] Comando recibido: " << line << "\n";

    if (line.rfind("GET ", 0) == 0) {
        std::string filename = line.substr(4);
        std::cout << "[SERVIDOR] Enviando archivo: " << filename << "\n";
        send_file(clientSock, filename);
    }
    else if (line.rfind("LOG ", 0) == 0) {
        std::string logLine = line.substr(4);
        append_log(logLine);
        std::string ok = "OK\n";
        send(clientSock, ok.c_str(), static_cast<int>(ok.size()), 0);
    }
    else {
        std::cerr << "[SERVIDOR] Comando no reconocido.\n";
    }
}

int main() {
    if (!init_winsock()) return 1;

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        std::cerr << "Error socket listen\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr))
        == SOCKET_ERROR) {
        std::cerr << "Error bind\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Error listen\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    std::cout << "[SERVIDOR] Escuchando en puerto "
        << SERVER_PORT << "...\n";

    while (true) {
        sockaddr_in clientAddr{};
        int addrLen = sizeof(clientAddr);
        SOCKET clientSock =
            accept(listenSock, (sockaddr*)&clientAddr, &addrLen);

        if (clientSock == INVALID_SOCKET) {
            std::cerr << "Error accept\n";
            continue;
        }

        std::cout << "[SERVIDOR] Cliente conectado.\n";
        handle_client(clientSock);
        closesocket(clientSock);
        std::cout << "[SERVIDOR] Cliente desconectado.\n";
    }

    closesocket(listenSock);
    WSACleanup();
    return 0;
}

