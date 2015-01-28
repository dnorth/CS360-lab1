#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <sstream>

#define SOCKET_ERROR        -1
#define BUFFER_SIZE         1024
#define QUEUE_SIZE          5

using namespace std;

string get_file(char* file_path)
{
	ifstream ifs(file_path);
	string content;
	content.assign( (istreambuf_iterator<char>(ifs) ),
                       (istreambuf_iterator<char>()    ) );
    return content;
}

int main(int argc, char* argv[])
{
    int hSocket,hServerSocket;  /* handle to socket */
    struct hostent* pHostInfo;   /* holds info about a machine */
    struct sockaddr_in Address; /* Internet socket address stuct */
    int nAddressSize=sizeof(struct sockaddr_in);
    int nHostPort;
    char* client_request;
    char pBuffer[BUFFER_SIZE];

    if(argc < 2)
      {
        printf("\nUsage: server host-port\n");
        return 0;
      }
    else
      {
        nHostPort=atoi(argv[1]);
      }

    printf("\nStarting server");

    printf("\nMaking socket");
    /* make a socket */
    hServerSocket=socket(AF_INET,SOCK_STREAM,0);

    if(hServerSocket == SOCKET_ERROR)
    {
        printf("\nCould not make a socket\n");
        return 0;
    }

    /* fill address struct */
    Address.sin_addr.s_addr=INADDR_ANY;
    Address.sin_port=htons(nHostPort);
    Address.sin_family=AF_INET;

    printf("\nBinding to port %d",nHostPort);

    /* bind to a port */
    if(bind(hServerSocket,(struct sockaddr*)&Address,sizeof(Address))
                        == SOCKET_ERROR)
    {
        printf("\nCould not connect to host\n");
        return 0;
    }
 /*  get port number */
    getsockname( hServerSocket, (struct sockaddr *) &Address,(socklen_t *)&nAddressSize);
    printf("opened socket as fd (%d) on port (%d) for stream i/o\n",hServerSocket, ntohs(Address.sin_port) );

        printf("Server\n\
              sin_family        = %d\n\
              sin_addr.s_addr   = %d\n\
              sin_port          = %d\n"
              , Address.sin_family
              , Address.sin_addr.s_addr
              , ntohs(Address.sin_port)
            );


    printf("\nMaking a listen queue of %d elements",QUEUE_SIZE);
    /* establish listen queue */
    if(listen(hServerSocket,QUEUE_SIZE) == SOCKET_ERROR)
    {
        printf("\nCould not listen\n");
        return 0;
    }

    for(;;)
    {
        printf("\nWaiting for a connection\n");
        /* get the connected socket */
        hSocket=accept(hServerSocket,(struct sockaddr*)&Address,(socklen_t *)&nAddressSize);
		
       	if(hSocket < 0)
       	{
	  printf("ERROR: Could not accept connection");
	  return 0;
       	}
       	else
       	{
	  //Add new task
	  printf("\nGot a connection. Reading from the client.\n ");
			
	  read(hSocket,pBuffer,sizeof(pBuffer));
	  char * request_type = strtok(pBuffer, " ");
	  char * request_url = strtok(pBuffer, " ");
	  char * request_host = strtok(pBuffer, " ");
	  printf("Here is what I got: \n\n%s", pBuffer);

	  printf("\n\nType: %s URL: %s Host: %s\n\n", request_type, request_url, request_host);
			
	  printf("Writing back.");
			
	  //I need to take in the URL from the client (parse the header) and find that file on my server
	  //Once I find the file on the server I need to retrieve it and send it back with a 200
			
	  string contentType="text/html";
	  char* filepath= "test1.html";
	  string content= get_file(filepath);
			
	  ostringstream stream;
			
	  stream << "HTTP/1.1 200 OK\r\nMIME-Version:1.0\r\nContent-Type:" << contentType << "\r\nContent-Length:" << content.length() << "\r\n\r\n" << content; 
			
	  string response = stream.str();
	  char* rPointer = new char[response.length() + 1];
	  strcpy(rPointer, response.c_str());
	  printf("Here is the response i'm sending: \n%s\n\n", rPointer);

			
	  write(hSocket, rPointer, response.length());
	  printf("\nClosing the socket");
	  /* close socket */
	  memset(pBuffer, 0, sizeof(pBuffer));
	  if(close(hSocket) == SOCKET_ERROR)
	    {
	      printf("\nCould not close socket\n");
	      return 0;
	    }
	}
        //strcpy(pBuffer,MESSAGE);
		//printf("\nSending \"%s\" to client",pBuffer);
        /* number returned by read() and write() is the number of bytes
        ** read or written, with -1 being that an error occured
        ** write what we received back to the server */
        //write(hSocket,pBuffer,strlen(pBuffer)+1);
        /* read from socket into buffer */
       // memset(pBuffer,0,sizeof(pBuffer));

    }
}
