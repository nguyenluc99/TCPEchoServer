SRC_DIR=.
CC=g++
# CXXFLAGS= -D PROC_COUNT=$(grep -c ^processor /proc/cpuinfo) 
# CXXFLAGS= -D PROC_COUNT=4
CXXFLAGS=
LIBS=-pthread -fsplit-stack
NPROCS:=$(shell grep -c ^processor /proc/cpuinfo)

all: TCPServer server


TCPServer:
	$(CC) $(CXXFLAGS) -D N_PROC=$(NPROCS) -c $(SRC_DIR)/$@.cc $(LIBS) -o $(SRC_DIR)/$@

server: TCPServer
	$(CC) $(CXXFLAGS) $(SRC_DIR)/$< $(SRC_DIR)/$@.cc -o $@  $(LIBS)

clean:
	rm -rf TCPServer $(SRC_DIR)/server
