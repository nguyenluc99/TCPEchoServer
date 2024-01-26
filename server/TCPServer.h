#ifndef SERVER_H_
#define SERVER_H_

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

// #include <ev.h>

#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/un.h>
// #include <queue>

// #include <ulimit.h>
#include <condition_variable>
// #include <iostream>
#include <mutex>
#include <map>

#include <string>
#include <utility>

#include <sys/sysinfo.h>
// #include <stdio.h>
// #include <stdlib.h> // setenv, getenv
#include "../cipher/cipher.h"

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)

namespace my_http_server
{
    static constexpr int BACKLOGSIZE        = 100000; // 100000
    static constexpr int MAX_EVENTS         = 200000; // 200000
    static constexpr int THREAD_POOL_SIZE   = 4; //N_PROC*2;
    static constexpr int SOCKET_POOL_SIZE   = 2; //N_PROC;
    static constexpr int BUFFER_SIZE        = 4096; // 4096

    struct EventData {
        EventData() : fd(0), length(0), cursor(0), buffer() {}
        int fd;
        size_t length;
        size_t cursor;
        char buffer[BUFFER_SIZE];
    };
    
    // typedef enum ThreadExeState
    // {
    //     THREAD_FREE,        /* not started */
    //     THREAD_WAITING,     /* started, and waiting for file descriptor to read */
    //     THREAD_RUNNING,     /* busy in reading and writing response   */
    //     THREAD_FINISHED     /* final state => to be joined            */
    // } ThreadExeState;


    
    char* getIP();
    class HttpServer{
        private:
            std::string host;
            int port;
            void init_and_bind(int port);
            int setnonblocking(int socket_fd);
            int setupServer(struct sockaddr_in& servaddr);
            void closeConnection(void* dataPtr);
            // void handleConnection(int socket_fd, int epollfd);
            void handleData(int conn_fd);
            std::string decipher(const std::string& cipherText);
            void prepareHeader(char* msgContent, MessageType type, const uint16_t packetSize, uint8_t message_sequence);
            // void prepareHeader(char* msgContent, MessageType type, const uint16_t packetSize, uint8_t message_sequence)

            // void init_and_bind(int port);
            // void startSocket();
            
            void initThreadResource();
            void initEpollList();


        public:
            HttpServer(const std::string& _host, int _port){
                host = _host; port=_port;
            }

            HttpServer() = default;
            ~HttpServer() = default;
            HttpServer(HttpServer&&) = default;
            HttpServer& operator=(HttpServer&&) = default;
            void openSocket();
            void openEV();

            std::string getHost() const {return host;}
            int getPort() {return port;}
            char* getResponseFromReq(char* buffer, int size, int conn_fd, int* resSize);
            char* getLoginResponse(char* buffer, int conn_fd, int* resSize);
            char* getEchoResponse(char* buffer, int conn_fd, int* resSize);
            // void startSocketThread(int socket_idx);
            std::map<int, uint16_t> clientConn_key_map; // map from client fd to checksum of username and password
    };

    typedef struct SharedThread
    {
        HttpServer                   *tcpserver;
        // int                         event_fd;       /* file discriptor that this thread is handling */
        // int                         epollfd;        /* the epoll file discriptor */
        int                         thread_idx;     /* index of this thread, to revtrieve back */
        // ThreadExeState              state;          /* execution state */
        // pthread_t                   thread_id;
        SharedThread(){thread_idx=-1;};
        // SharedThread(){event_fd = -1; epollfd = -1; thread_idx = -1; state = THREAD_FREE;};
    } SharedThread;
    // typedef struct my_io        /* used for interaction with ev */
    // {
    //     ev_io io;
    //     // int epollfd;
    //     SharedThread* shared;
    //     // void *somedata;
    //     // struct whatever *mostinteresting;
    // } my_io;

    extern SharedThread thread_infos[THREAD_POOL_SIZE];

};

#endif
