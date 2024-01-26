# TCP Echo Server
Simple HTTP server that supports HTTP/1.1 in C++.

## Problem:
Create an TCP echo server (mainly Linux, just tested on Ubuntu 20.04 LTS)

## Main feature:

<li> Username and Password Validation
<li> Decryption of Echo Request Message.
Keys are generated using procedure:

```
uint32_t next_key(uint32_t  key)
{
  return (key*1103515245  +  12345) %  0x7FFFFFFF;
}
```

## Usage guideline:
There are two separated module, `client` and `server`, using some same functionalities (`cipher`).

To build and start `server`:
````
cd ./server
make
./server
````

To build `client`:
````
cd ./client
make
````

To start client with host (e.g. `172.17.0.2`), port (e.g. `8080`), username (`u`) and password (`p`):
````
./client 172.17.0.2 8080 u p
````
The client will automatically try to connect to server and prompting for input to echo to server. After that, client can send message which will be cipher-red to server. The server will decipher it, return the plain message to client. 

The connection is closed when user do: `Ctrl+C` -> `Enter`. This will serve the last message (as it is already waiting for input.)

## Benchmarking:
To benmark connection, a sample response of HTTP/1.1 is used in server.

Benchmarking example using `wrk`: https://github.com/wg/wrk/:
````
./wrk -d 3 -c 30000 http://172.17.0.2:8080
````
