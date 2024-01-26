#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <string>
#include <stdio.h>

#ifdef __WIN32__
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/un.h>

// #include <ulimit.h>
#include <sys/resource.h>
// #include "server.h"
#include <fcntl.h>
#include <thread>
#include <sstream>
#include <strings.h>

#include <fstream>
#include "../cipher/cipher.h"

#define HTTP_SERVER "172.17.0.2"
#define HTTP_PORT 8080


namespace Network
{
    class TCPClient
    {
        public:
            TCPClient(const std::string http_server = HTTP_SERVER, const int http_port = HTTP_PORT);
            ~TCPClient();
            bool login(const char* username, const char* password);
            char* prepareLoginRequest(const char* username, const char* password, uint16_t* loginReqSize);
            bool login(struct sockaddr_in& socketAddr, int& sockfd, char* loginRequest, uint16_t requestSize);
            bool echoToServer(char* msg);
            void closedConnection();

        public:
            std::string serverAddr = "";
            int port = -1;
            int socketfd = -1;
            struct sockaddr_in servaddr;
            // int session = ?
        private:
            int message_sequence = 1;
            int sumUsername = -1;
            int sumPassword = -1;

            char* prepareEchoRequest(char* msg, uint16_t* msgSize);
            void prepareHeader(char* msgContent, MessageType type, const uint16_t msgSize);

    };

    typedef int RequestHeader;


};


#endif // UTIL_H
