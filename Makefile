all: download server threadpool
clean:
	rm download
download: Client.cpp Client.h
	g++ -o download Client.cpp
threadpool: semaphores.cpp
	g++ -lpthread semaphores.cpp -o threadpool
server: Server.cpp
	g++ -pthread Server.cpp -o server
