all: download
clean:
	rm download
download: Client.cpp Client.h
	g++ -o download Client.cpp
