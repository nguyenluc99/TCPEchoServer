#include "TCPClient.h"

using namespace Network;
TCPClient::TCPClient(const std::string http_server, const int http_port) : 
    serverAddr(http_server), port(http_port)
{
    
}

TCPClient::~TCPClient()
{

}

void TCPClient::prepareHeader(char* msgContent, MessageType type, const uint16_t reqSize)
{
    serialize(msgContent+REQ_MSG_SIZE_OFFSET, (void*) &reqSize, 2);
    serialize(msgContent+REQ_MSG_TYPE_OFFSET, (void*) &type, 1);
    serialize(msgContent+REQ_MSG_SEQ_OFFSET, (void*) &message_sequence, 1);
    message_sequence ++;
}

char* TCPClient::prepareLoginRequest(const char* username, const char* password, uint16_t* loginReqSize)
{
    // todo: check if username or password contain ","
    char* loginReq = (char*) malloc(LOGIN_REQ_SIZE);

    bzero(loginReq, LOGIN_REQ_SIZE);
    prepareHeader(loginReq, LOGIN_REQUEST, LOGIN_REQ_SIZE);
    *loginReqSize = LOGIN_REQ_SIZE;
    serialize(loginReq+LOG_REQ_USER_OFFSET, (void*) username, strlen(username));
    serialize(loginReq+LOG_REQ_PASS_OFFSET, (void*) password, strlen(password));

    return loginReq;
}

bool TCPClient::login(struct sockaddr_in& socketAddr, int& sockfd, char* loginRequest, uint16_t requestSize)
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        return false;
    }
    bzero(&socketAddr, sizeof(socketAddr));
    char buff[1024];

        // assign IP, PORT
    socketAddr.sin_family = AF_INET;
    socketAddr.sin_addr.s_addr = inet_addr(serverAddr.c_str());
    socketAddr.sin_port = htons(port);

    // connect the client socket to server socket
    if (connect(sockfd, (struct sockaddr*)&socketAddr, sizeof(socketAddr)) != 0) {
        printf("connection with the server failed...\n");
        return false;
    }
    else
        printf("connected to the server..\n");

    usleep(100);
        printf("connected to the server..\n");
    bzero(buff, sizeof(buff));
    write(sockfd, loginRequest, requestSize);
        printf("connected to the server..\n");
    read(sockfd, buff, sizeof(buff));
        printf("connected to the server..\n");
    printf("result is %s\n", buff);
        printf("connected to the server..\n");
    uint16_t loginCode = 0;
    deserialize((char*)&loginCode, buff + LOG_RES_CODE_OFFSET, 2);
    // for (int i = 0; i < 6; i++)
    // {
    //     // uint8_t val = cipherText[i] % 256;
    //     printf("char is %u\n", (uint16_t(buff[i]))%256);
    // }
    if (loginCode == 1) // extract status code
        return true;
    return false;

}

bool TCPClient::login(const char* username, const char* password)
{
    uint16_t loginReqSize = 0;
    char* loginMsg = prepareLoginRequest(username, password, &loginReqSize);
    // for (int i = 0; i < loginReqSize; i++)
    // {
    //     // uint8_t val = cipherText[i] % 256;
    //     printf("char is %u\n", (uint16_t(loginMsg[i]))%256);
    // }
    bool res = login(servaddr, socketfd, loginMsg, loginReqSize);
    if (res)
    {
        sumUsername = getCheckSum(username);
        sumPassword = getCheckSum(password);
        std::cout << "login success" << std::endl;
    }
    else
        std::cout << "login failed" << std::endl;
    if (loginMsg) free(loginMsg);
    return res;
}

char* TCPClient::prepareEchoRequest(char* msg, uint16_t* reqSize)
{
    char* cipherText;
    uint32_t initial_key = (message_sequence << 16) | (sumUsername << 8) | (sumPassword);
    uint16_t msgLen = strlen(msg);

    *reqSize = HEADER_SIZE + MSG_SIZE_SIZE + msgLen;
    cipherText = (char*) malloc(*reqSize);
    bzero(cipherText, *reqSize);
    prepareHeader(cipherText, ECHO_REQUEST, *reqSize);

    serialize(cipherText + ECHO_REQ_SIZE_OFFSET, &msgLen, MSG_SIZE_SIZE);

    for (int i = 0; i < msgLen; i++)
    {
        int key_i = next_key(initial_key);
        int ch = msg[i];
        uint8_t littleKey = key_i%256;
        uint8_t cipheredChar = littleKey ^ ch;
        serialize(cipherText + ECHO_REQ_VALUE_OFFSET + i, &cipheredChar, 1);
        initial_key = key_i;
    }
    // // print(cipherText);
    // for (int i = 0; i < *reqSize; i++)
    // {
    //     // uint8_t val = cipherText[i] % 256;
    //     printf("req char is %u\n", (uint16_t(cipherText[i]))%256);
    // }
    return cipherText;
}

bool TCPClient::echoToServer(char* msg)
{
    if (socketfd == -1)
        return false;
    
    char buffer[1024];
    uint16_t reqSize = 0;
    char* sentReq = prepareEchoRequest(msg, &reqSize);
    write(socketfd, sentReq, reqSize);
    bzero(buffer, sizeof(buffer));
    int readSize = read(socketfd, buffer, sizeof(buffer));
    std::cout << (buffer+ECHO_RES_VALUE_OFFSET) << std::endl;
    // // print(cipherText);
    // for (int i = 0; i < readSize; i++)
    // {
    //     // uint8_t val = cipherText[i] % 256;
    //     printf("rec char is %u\n", (uint16_t(buffer[i]))%256);
    // }
    message_sequence ++;
    if (sentReq)
        free(sentReq);
    return true;
}

void TCPClient::closedConnection()
{
    std::cout << "closing connection " << std::endl;
    int  t = 1;
    setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(int));
    close(socketfd);
    std::cout << "Close connection done " << std::endl;
}
