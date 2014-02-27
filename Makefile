all: c++
c++:
	g++ -o server server.cc -lpthread -std=c++0x
