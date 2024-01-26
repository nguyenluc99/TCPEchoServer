#ifndef CIPHER_H
#define CIPHER_H

#include <string>

#define print(x) std::cout<<#x<<" is "<<x<<std::endl;

// offset within Header
#define REQ_MSG_SIZE_OFFSET 0
#define REQ_MSG_TYPE_OFFSET 2
#define REQ_MSG_SEQ_OFFSET  3

// offset within Request
#define HEDAER_OFFSET 0

#define LOG_REQ_USER_OFFSET 4
#define LOG_REQ_PASS_OFFSET 36

#define LOG_RES_CODE_OFFSET        4

#define ECHO_REQ_SIZE_OFFSET        4
#define ECHO_REQ_VALUE_OFFSET       6

#define ECHO_RES_SIZE_OFFSET        4
#define ECHO_RES_VALUE_OFFSET       6

// size of common packet
#define LOGIN_REQ_SIZE       68
#define LOGIN_RES_SIZE       6
#define HEADER_SIZE          4
#define MSG_SIZE_SIZE        2

inline uint8_t getCheckSum(const char* str)
{
  uint8_t sum = 0;
  int len = strlen(str);
  for (int i = 0; i < len; i++)
    sum = (sum + str[i]) % 256;
  return sum;
}
inline uint32_t next_key(uint32_t  key)
{
  return (key*1103515245  +  12345) %  0x7FFFFFFF;
}

inline void serialize(char* dst, void* src, uint32_t len)
{
  strncpy(dst, (char*) src, len);
  dst[len] = 0;
}

inline void deserialize(char* dst, void* buffer, uint32_t len)
{
  strncpy(dst, (char*) buffer, len);
  // dst[len] = 0;
};

typedef enum MessageType
{
  LOGIN_REQUEST = 0,
  LOGIN_RESPONSE = 1,
  ECHO_REQUEST = 2,
  ECHO_RESPONSE = 3
};

#endif // CIPHER_H