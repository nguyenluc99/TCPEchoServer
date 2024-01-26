


#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <string>
#include <utility>
#include <sys/resource.h>

#include "TCPServer.h"

using my_http_server::HttpServer;

void exitMain()
{
    printf("exiting\n");
}

// void testLimit()
// {
//     struct rlimit rlm;
//     rlm.rlim_cur = 8192; //8KB
//     rlm.rlim_max = 8192; //8KB
//     setrlimit(RLIMIT_STACK, &rlm);
//     rlm.rlim_cur = 10;
//     rlm.rlim_max = 20;
//     setrlimit(RLIMIT_RTTIME, &rlm);
//     getrlimit(RLIMIT_RSS, &rlm);
//     rlm.rlim_cur = 1;
//     rlm.rlim_max = 1;
//     setrlimit(RLIMIT_RSS, &rlm);

//     getrlimit(RLIMIT_DATA, &rlm); // set to 8MB to see if the .. is high B or KB?
//     getrlimit(RLIMIT_CPU, &rlm);  // => terminal the thread => ok ? set to 2 (second).
//     getrlimit(RLIMIT_CORE, &rlm);
//     getrlimit(RLIMIT_NOFILE, &rlm);
//     return ;
// }
void setSize()
{
    struct rlimit rlm;

    rlm.rlim_cur = 16384; //16KB = PTHREAD_STACK_MIN, is this defined on your machine?
    rlm.rlim_max = 16384; //16KB
    if (setrlimit(RLIMIT_STACK, &rlm) < 0)
        perror("fail setting RLIMIT_STACK");

    rlm.rlim_cur = 10;
    rlm.rlim_max = 20;
    if (setrlimit(RLIMIT_RSS, &rlm) < 0)
        perror("fail setting RLIMIT_RSS");

    rlm.rlim_cur = 10;
    rlm.rlim_max = 20;
    if (setrlimit(RLIMIT_RTTIME, &rlm) < 0)
        perror("fail setting RLIMIT_RTTIME");\
}

int main(int argc, char* argv[])
{
    char* host = my_http_server::getIP();
    int HTTP_PORT = 8080;
    if (argc > 1)
        HTTP_PORT = atoi(argv[1]);
    printf("Open service at: http://%s:%d\n", host, HTTP_PORT);
    setSize();
    HttpServer server(std::string(host), HTTP_PORT);
    server.openSocket();
    return 0;
}
