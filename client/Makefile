SRC_DIR=./
CC=g++
CXXFLAGS=
LIBS=-pthread -fsplit-stack

all: TCPClient client



TCPClient:
	$(CC) $(CXXFLAGS) -c $(SRC_DIR)/$@.cc $(LIBS) -o $(SRC_DIR)/$@

client: TCPClient
	$(CC) $(CXXFLAGS) $(SRC_DIR)/$< $(SRC_DIR)/$@.cc -o $@  $(LIBS)

clean:
	rm -rf TCPClient $(SRC_DIR)/client
