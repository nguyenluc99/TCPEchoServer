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

#include <signal.h>

static volatile int keepRunning = 1;

// #include <nlohmann/json.hpp>
#include "TCPClient.h"

void intHandler(int dummy) {
    keepRunning = 0;
}

int main(int argc, char* argv[])
{
    bool isLogged = false;
    char* host;
    int port;
    char* user_inp;
    char* pass_inp;

    if (argc != 5)
    {
        std::cout << "usage : ./client <host> <port> <username> <password>" << std::endl;
        std::cout << "example: ./client 127.0.0.1 8080 u p" << std::endl;
        exit(1);
    }
    
    host = argv[1];
    port = atoi(argv[2]);
    user_inp = argv[3];
    pass_inp = argv[4];
    std::string username = std::string(user_inp);
    std::string password = std::string(pass_inp);
    
    // Network::TCPClient client();
    Network::TCPClient client(host, port);
    signal(SIGINT, intHandler);

    while (keepRunning)
    {
        if (!isLogged)
        {
            std::cout << "login as  " << username << " : " << password << std::endl;
            isLogged = client.login(username.c_str(), password.c_str());
            usleep(50000);
        }
        else
        {
            printf("Send something to server: \n");
            char word[1024];
            fgets(word, sizeof(word), stdin);

            if (!client.echoToServer(word))
            {
                std::cout << "cannot echo to server " << std::endl;
                break;
            }
            // sleep(1);
        }

    }
    std::cout << "ending program " << std::endl;
    if (isLogged)
    {
        client.closedConnection();
        isLogged = false;
    }
    return 0;
}

