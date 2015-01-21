all: download
clean:
	rm download
download: Client.cpp
	g++ -o download Client.cpp
