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
#include "TCPServer.h"
#include <fcntl.h>
#include <thread>
#include <sstream>

#include <fstream>

namespace my_http_server
{

    // ev_io stdin_watcher;
    // ev_signal sig;
    // int sig = SIGINT;
    const char* sampleResponse = "HTTP/1.1 200 OK\nServer: Hello\nContent-Length: 13\nContent-Type: text/plain\n\nHello, world\n";
    const int sampleLength = 88;
    SharedThread    thread_infos[THREAD_POOL_SIZE];
    int             epoll_list[THREAD_POOL_SIZE];
    epoll_event worker_events_list[THREAD_POOL_SIZE][MAX_EVENTS];
    int socketList[SOCKET_POOL_SIZE];
    std::thread threadList[SOCKET_POOL_SIZE];
    // TSQueue<std::pair<int, int> > fd_queue; // first = epollfd, second = event_fd;

    // another callback, this time for a time-out

    char* getIP()
    {
        char hostname[1024];
        hostname[1023] = '\0';
        gethostname(hostname, 1023);
        // printf("Hostname: %s\n", hostname);
        struct hostent* host_entry;
        host_entry = gethostbyname(hostname);
        // printf("h_name: %s\n", host_entry->h_name);
        char* IP = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0])); //Convert into IP string
        char* IP_cp = (char*) malloc(30);
        memcpy(IP_cp, IP, strlen(IP));
        return IP_cp;
    }

    // std::string* getTmpHTTPResponse()
    // {
    //     std::string *msg = new std::string("HTTP/1.1 200 OK\nServer: Hello\nContent-Length: 13\nContent-Type: text/plain\n\nHello, world\n");
    //     return msg;
    // };

    std::string addHeaderToResponse(std::string content)
    {
        std::stringstream ss;
        ss << "HTTP/1.1 200 OK\nServer: Sample HTTP Server\nContent-Length: ";
        ss << std::to_string(content.size());
        ss << "\nContent-Type: text/html\n\n";
        ss << content;
        return ss.str();
    };

    void HttpServer::init_and_bind(int port)
    {
        struct sockaddr_in servaddr;
        socklen_t len;
        int opt = 1;
        int server_fd;
        int idx;
        for(idx = 0; idx < SOCKET_POOL_SIZE; idx++)
        {
            server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
            len = sizeof(servaddr);

            if (server_fd == -1)
            handle_error("SOCKET ERROR");
            // Forcefully attaching socket to the port SOCKET_PORT
            if (setsockopt(server_fd, SOL_SOCKET,
                        SO_REUSEADDR | SO_REUSEPORT, &opt,
                        // SO_REUSEADDR, &opt,
                        sizeof(opt)))
                handle_error("setsockopt");

            servaddr.sin_family = AF_INET;
            servaddr.sin_addr.s_addr = INADDR_ANY;
            servaddr.sin_port = htons(getPort());

            if (bind(server_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
                handle_error("bind");
            socketList[idx] = server_fd; 
        }
    }

    int HttpServer::setnonblocking(int socket_fd) // https://stackoverflow.com/questions/27266346/how-to-set-file-descriptor-non-blocking
    {
        int opt;

        opt = fcntl(socket_fd, F_GETFD);
        if (opt < 0) {
            printf("fcntl(F_GETFD) fail.");
            return -1;
        }
        opt |= O_NONBLOCK;
        if (fcntl(socket_fd, F_SETFD, opt) < 0) {
            printf("fcntl(F_SETFD) fail.");
            return -1;
        }
        return 0;
    }

    char* getResponse(int* size)
    {
        // const std::string filename = "./html/index.html";
        std::stringstream ss;

        ss << "<!DOCTYPE html><html lang=\"en\"><head>";
        ss << "<style></style>";
        ss << "<script type=\"text/javascript\"></script>";
        ss << "</head><title>A HTML webpage in C++</title><body class=\"p25\" style=\"background-color: #fff8e8;\">";
        ss << "<div>";

        ss << "<h1> This is a header1 </h1>";
        // ss << "<h3> This is a header3 </h3>";
        // ss << "<p> This is a paragraph. I hope the background color is light yellow-orange. </p>";
        // ss << "<p> This is an other paragraph. Is this text in <span style=\"color:Red;\">Red</span> and <b>Bold</b>?. </p>";
        ss << "<p> Supershort paragraph </p>";

        ss << "</body>";
        ss << "</head>\n";

        char* res = (char*) malloc(ss.str().length()+1);
        strncpy(res, ss.str().c_str(), ss.str().length());
        res[ss.str().length()] = 0;
        *size = ss.str().length();
        return res;
    }

    void HttpServer::prepareHeader(char* msgContent, MessageType type, const uint16_t packetSize, uint8_t message_sequence)
    {
        serialize(msgContent+REQ_MSG_SIZE_OFFSET, (void*) &packetSize, 2);
        serialize(msgContent+REQ_MSG_TYPE_OFFSET, (void*) &type, 1);
        serialize(msgContent+REQ_MSG_SEQ_OFFSET, (void*) &message_sequence, 1);
    }

    char* HttpServer::getLoginResponse(char* buffer, int conn_fd, int* resSize)
    {
        char* username = buffer + LOG_REQ_USER_OFFSET;
        char* password = buffer + LOG_REQ_PASS_OFFSET;
        char* logRes = (char*) malloc(LOGIN_RES_SIZE);
        uint8_t msgSeq;// = uint8_t(buffer[3]);
        uint16_t success = 1;

        clientConn_key_map[conn_fd] = (getCheckSum(username) << 8 |  getCheckSum(password));
        *resSize = LOGIN_RES_SIZE;
        bzero(logRes, LOGIN_RES_SIZE);
        
        deserialize((char*) &msgSeq, buffer+REQ_MSG_SEQ_OFFSET, 1);
        prepareHeader(logRes, LOGIN_RESPONSE, LOGIN_RES_SIZE, msgSeq);
        
        serialize(logRes + LOG_RES_CODE_OFFSET, &success, 2);
        // for (int i = 0; i < 6; i++)
        // {
        //     // uint8_t val = cipherText[i] % 256;
        //     printf("char is %u\n", (uint16_t(logRes[i]))%256);
        // }
        return logRes;
    }

    char* HttpServer::getEchoResponse(char* buffer, int conn_fd, int* resSize)
    {
        if (clientConn_key_map.find(conn_fd) == clientConn_key_map.end())
        {
            *resSize = 0;
            // TODO: handle this case to return some error message to user
            return nullptr;
        }
        uint16_t msgSize;// = buffer[0] + buffer[1] * 256;
        deserialize((char*) &msgSize, buffer+ECHO_REQ_SIZE_OFFSET, 2);
        // uint8_t msgType;// = uint8_t(buffer[2]);
        // deserialize((char*) &msgType, buffer+MSG_TYPE_OFFSET, 1);
        uint8_t msgSeq;// = uint8_t(buffer[3]);
        deserialize((char*) &msgSeq, buffer+REQ_MSG_SEQ_OFFSET, 1);

        *resSize = msgSize + MSG_SIZE_SIZE + HEADER_SIZE; // just echo back

        uint32_t initial_key = ((msgSeq << 16) | clientConn_key_map[conn_fd]);
        char* msgRes = (char*) malloc(*resSize + 1);
        // print(msgSize); print(msgSeq); print(*resSize);
        prepareHeader(msgRes, ECHO_RESPONSE, *resSize, msgSeq);
        serialize(msgRes, &msgSize, 2);
        for (uint16_t i = 0; i < msgSize; i++)
        {
            uint32_t key_i = next_key(initial_key);
            uint8_t littleKey = key_i%256;
            uint8_t plainChar = littleKey ^ buffer[i + ECHO_REQ_VALUE_OFFSET];
            serialize(msgRes + ECHO_RES_VALUE_OFFSET + i, &plainChar, 1);
            initial_key = key_i;
        }
        // msgRes[msgSize] = 0;
        // std::cout << "real msgRes is " << msgRes << std::endl;
        std::cout << (msgRes + ECHO_RES_VALUE_OFFSET) << std::endl;
        return msgRes;
    }

    // void printIntBit(char ch) { printf("%u", uint16_t(ch) % 256);}
    // unsigned int getBitInInt(char* buffer, int pos) { unsigned int ret = 0; ret |= buffer[pos]; return ret;}

    char* HttpServer::getResponseFromReq(char* buffer, int size, int conn_fd, int* resSize)
    {
        uint16_t reqSize;// = buffer[0] + buffer[1] * 256;
        uint8_t reqType;// = uint8_t(buffer[2]);

        deserialize((char*) &reqSize, buffer+REQ_MSG_SIZE_OFFSET, 2);
        deserialize((char*) &reqType, buffer+REQ_MSG_TYPE_OFFSET, 1);

        // print(reqSize);
        // for (int i = 0; i < reqSize; i++)
        // {
        //     printf("rec char is %u\n", (uint16_t(buffer[i]))%256);
        // }
        // std::cout << "reqType " << uint16_t(reqType) % 256 << std::endl;
        switch (reqType)
        {
            case LOGIN_REQUEST:
                return getLoginResponse(buffer, conn_fd, resSize);
            case ECHO_REQUEST:
                if (size == reqSize)
                    return getEchoResponse(buffer, conn_fd, resSize);
                // else
                //     return "";
                break;
            case LOGIN_RESPONSE:
            case ECHO_RESPONSE:
            default:{}
                std::cout << "invalid message" << std::endl;
        }
        return getResponse(resSize); // default HTTP 1.1 response, use for benchmarking, e.g., wrk.
    }
    void closeConnection(void* dataPtr, HttpServer* svr)
    {
        // std::cout << "closing " << std::endl;
        if (dataPtr)
        {
            EventData* edata = (EventData*) dataPtr;
            int conn_fd = edata->fd;
            close(conn_fd);
            // std::cout << "closing connection " << conn_fd << std::endl;
            free(dataPtr);
            if (svr && svr->clientConn_key_map.find(conn_fd) != svr->clientConn_key_map.end())
                svr->clientConn_key_map.erase(conn_fd);
        }
    }
    void* threadStart(void* arg)
    {
        SharedThread* shared = (SharedThread*) arg;
        int size;
        int bufferSize = 1024;
        char buffer[bufferSize] = {0};
        int s, conn_fd;
        int workingThr = shared->thread_idx;
        EventData *edata;
        std::cout << "thread start " << shared->thread_idx << std::endl;
        while(1)
        {
            // std::cout << "checking on " << shared->thread_idx << std::endl;
            int n, idx;
            n = epoll_wait (epoll_list[workingThr], worker_events_list[workingThr], MAX_EVENTS, -1);
            for (idx = 0; idx < n; idx ++)
            {
                if (worker_events_list[workingThr][idx].events & EPOLLERR)
                    closeConnection(worker_events_list[workingThr][idx].data.ptr, shared->tcpserver);
                else if (worker_events_list[workingThr][idx].events & EPOLLHUP)
                    closeConnection(worker_events_list[workingThr][idx].data.ptr, shared->tcpserver);
                else if (!(worker_events_list[workingThr][idx].events & EPOLLIN))
                    closeConnection(worker_events_list[workingThr][idx].data.ptr, shared->tcpserver);
                else if (worker_events_list[workingThr][idx].data.ptr) // data is available
                {
                    edata = (EventData*) worker_events_list[workingThr][idx].data.ptr;
                    conn_fd = edata->fd;
                    bool done = false;
                    bzero(buffer, 1024);
                    size = read(conn_fd, buffer, 1024);
                    if (size == -1) // 0 means done or client closed, -1 means error
                    {
                        if (errno != EAGAIN)
                        {
                            done = true;
                            /* we just read all data, no more data to read */
                        }
                    }
                    else if (size == 0)
                    {
                        done = 1;
                    }
                    // TODO: handle each part of data, whether done is true or false
                    // std::cout << "read data of size " << size << " : " << buffer << std::endl;
                    int resSize = 0;
                    char* res = shared->tcpserver->getResponseFromReq(buffer, size, conn_fd, &resSize);
                    // std::cout << "writing to client " << res << ", size " << resSize << std::endl;
                    write(conn_fd, res, resSize);
                    if (res)
                        free(res);
                }
            }
        }
    }

    void HttpServer::initThreadResource()
    {
        int idx;
        for (idx = 0; idx < THREAD_POOL_SIZE; idx++)
        {
            thread_infos[idx].thread_idx = idx;
            thread_infos[idx].tcpserver = this;
            pthread_t pid;
            int err = pthread_create(&pid, NULL, threadStart, (void*)&thread_infos[idx]);
            if (err == 0){}
                // printf("thread %d start successfully\n", idx);
            else 
                printf("thread %d FAILED\n", idx);
        }
    }
    void HttpServer::initEpollList()
    {
        int idx;
        for (idx = 0; idx < THREAD_POOL_SIZE; idx++)
        {
            epoll_list[idx] = epoll_create1(0);
        }
    }


    // void HttpServer::handleData(int conn_fd)
    // {
    //     bool done = false;
    //     int size;
    //     char buffer[1024] = {0};

    //     size = read(conn_fd, buffer, 1024); // stuck after reading => keep waiting for request.
    //     if (size == -1) // 0 means done, -1 means error
    //     {
    //         if (errno != EAGAIN)
    //         {
    //             done = true;
    //             /* we just read all data, no more data to read */
    //         }
    //     }
    //     else if (size == 0)
    //     {
    //         done = 1;
    //     }
    //     write(conn_fd, sampleResponse, sampleLength);

    //     /* Close conenction after reading */
    //     close(conn_fd);
    // }
    // std::string HttpServer::decipher(const std::string& cipherText)
    // {
        
    // }

    // now, run only several sockets with many epoll.
    void startSocket(HttpServer* server)
    {
        int idx, s;
        struct epoll_event event;
        int opt, socket_fd;

        for(idx = 0; idx < SOCKET_POOL_SIZE; idx++)
        {
            socket_fd = socketList[idx];
            opt = fcntl(socket_fd, F_GETFD);
            if (opt < 0) {
                printf("fcntl(F_GETFD) fail.");
                handle_error("setnonblocking");
            }
            opt |= O_NONBLOCK;
            if (fcntl(socket_fd, F_SETFD, opt) < 0) {
                printf("fcntl(F_SETFD) fail.");
                handle_error("setnonblocking");
            }
            
            s = listen(socket_fd, BACKLOGSIZE);
            if (s == -1)
                handle_error("listener");
        }
    }

    void startSocketThread(HttpServer* server, int socket_idx)
    {
        int socket_fd = socketList[socket_idx];
        int currentWorker = 0, s;
        EventData *client_data;

        struct sockaddr in_addr;
        socklen_t in_len;

        /* The event loop => socket keep listening */
        while (1)
        {
            // std::cout << "thread  looping " << socket_idx << std::endl;
            int client_fd = accept4(socket_fd, &in_addr, &in_len, SOCK_NONBLOCK);
            if (client_fd < 0)
            {
                usleep(30000);
                sched_yield();
            }
            else
            {
                // std::cout << "accepting " << client_fd << std::endl;
                // std::cout << "thread  handling " << socket_idx << std::endl;
                // distribute client fd to a worker
                client_data = new EventData();
                client_data->fd = client_fd;
                struct epoll_event event;
                event.data.ptr = (void*) client_data;
                event.events = EPOLLIN;

                int epollIdx = socket_idx * THREAD_POOL_SIZE/SOCKET_POOL_SIZE + currentWorker;
                s = epoll_ctl(epoll_list[epollIdx], EPOLL_CTL_ADD, client_fd, &event);
                // printf("Adding to epoll idx: %d, by socket_idx %d.\n", epollIdx, socket_idx);
                if (s < 0)
                {
                    printf("Error adding task to worker %d.", currentWorker);
                    perror("Error adding task to worker");
                }
                else
                    currentWorker ++;
                // each socket will manage THREAD_POOL_SIZE/SOCKET_POOL_SIZE thread. currentWorker is the order in that list.
                currentWorker %= ((int) THREAD_POOL_SIZE/SOCKET_POOL_SIZE);
            }
        }
    }
    void HttpServer::openSocket()
    {
        int idx;
        // struct epoll_event events[MAX_EVENTS];

        init_and_bind(getPort());
        startSocket(this);
        
        initThreadResource();
        initEpollList();

        // startSocketThread();

        for (idx = 0; idx < SOCKET_POOL_SIZE; idx++)
        {
            // start  SOCKET_POOL_SIZE sockets to wait for client.// what to share?
            // std::thread t1(startSocketThread);
            threadList[idx] = std::thread(startSocketThread, this, idx);
        }
        // finished ?let join it.
        for (idx = 0; idx < SOCKET_POOL_SIZE; idx++)
        {
            threadList[idx].join();
        }
    }

    // void HttpServer::openEV() // open 4 thread and ask them to wait on specificed fd?
    // {
    //     while(1)
    //     {
    //         // // use the default event loop unless you have special needs
    //         // struct ev_loop *loop = EV_DEFAULT;
            
    //         // // initialise an io watcher, then start it
    //         // // this one will watch for stdin to become readable
    //         // ev_io_init (&stdin_watcher, stdin_cb, /*STDIN_FILENO*/ 0, EV_READ); // not open a socket. 
    //         // ev_io_start (loop, &stdin_watcher);
            
    //         // // initialise a timer watcher, then start it
    //         // // simple non-repeating 5.5 second timeout
    //         // ev_timer_init (&timeout_watcher, timeout_cb, 5.5, 0.);
    //         // ev_timer_start (loop, &timeout_watcher);
            
    //         // // now wait for events to arrive
    //         // ev_run (loop, 0);
    //     }
    // }
}
