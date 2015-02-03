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
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <queue>

#define SOCKET_ERROR        -1
#define BUFFER_SIZE         1024
#define QUEUE_SIZE          5

#pragma GCC diagnostic ignored "-Wwrite-strings"

using namespace std;

//sem_wait increments the counter
//sem_post decrements the counter

queue<int> work_tasks; //All the tasks 

sem_t work_mutex; //Controls queues. Makes sure only one task is moving at a time 
sem_t work_to_do; //Tells us how many tasks are on the queue
sem_t space_on_q; // Tells us if there is any space on the queue

int hSocket,hServerSocket;  /* handle to socket */
struct hostent* pHostInfo;   /* holds info about a machine */
struct sockaddr_in Address; /* Internet socket address stuct */
int nAddressSize=sizeof(struct sockaddr_in);
int nHostPort;
char* client_request;
char pBuffer[BUFFER_SIZE];
string serving_dir;
int threadCount;
struct stat filestat;

struct thread_info
{
    int thread_id;
    int another_number;
};


string get_file(char* file_path, string serving_dir)
{
	//gets rid of the "/" before trying to find the file
	if(strcmp(file_path,"/") == 0 || strcmp(file_path, "") == 0)
	{
		//Try to find index, if not then display directory
		file_path = "index.html";
	}
	else
	{
		file_path++;
	}
	
	/*
	if(serving_dir != "/" || serving_dir !=".")
	{
		printf("\n\nTHIS RIGHT HERE RIGHT NOW- SERVING DIR: %s\n\n", serving_dir.c_str());
		string s= serving_dir + "/" + file_path;
		copy(s.begin(), s.end(), file_path);
	}
	*/
	try{
		//Set the mode to be binary
		ifstream ifs(file_path, ios::in | ios::binary);
		string content;
		content.assign( (istreambuf_iterator<char>(ifs) ),
						   (istreambuf_iterator<char>()    ) );
		return content;
	}
	catch(int e)
	{
		printf("\nCouldn't get it. Error: %d\n", e);
		return "";
	}
}

string get_contentType(string filename)
{
	string filetype= " ";
	
	if(filename.length() > 4)
	{
		string file_end = filename.substr( filename.size()-4,4);
		
		if(file_end == "html")
		{
			filetype="text/html";
		}
		else if(file_end == ".txt")
		{
			filetype="text/plain";
		}
		else if(file_end == ".jpg")
		{
			filetype="image/jpg";
		}
		else if(file_end == ".gif")
		{
			filetype="image/gif";
		}
	}
	
	return filetype;
}

void* serve( void* in_data )
{
    struct thread_info* t_info = ( struct thread_info* ) in_data;
    int tid = t_info->thread_id;

    while( 1 )
    {
	sem_wait( &work_to_do); //Is there work to do? Okay, then I will do it. If not we're just waiting
        sem_wait( &work_mutex ); //I'm locking myself so that no one else can touch me.
        
        int hSocket = work_tasks.front(); //Take the first thing in the work queue
        work_tasks.pop(); //Pop it from the queue
       
        printf("I am thread: %d", tid);
		//Add new task
			printf("\nGot a connection. Reading from the client.\n ");
			
			read(hSocket,pBuffer,sizeof(pBuffer));
	  
			printf("Here is what I got: \n\n%s", pBuffer);
	 
			char * request_type = strtok(pBuffer, " ");
			char * request_filepath = strtok(NULL, " ");

			printf("\n\nType: %s URL: %s", request_type, request_filepath);
			
			printf("\n\nWriting back.\n");
			
		  //I need to take in the URL from the client (parse the header) and find that file on my server
		  //Once I find the file on the server I need to retrieve it and send it back with a 200
				
		  string content= get_file(request_filepath, serving_dir);
		  string contentType = get_contentType(request_filepath);
		  				
		  //printf("\n\nContent Type: %s \n\nContent: %s", contentType.c_str(), content.c_str());	
		  ostringstream stream;	
		  
		  stream << "HTTP/1.1 ";
		  if(contentType == " " || content == "")
		  {
			  stream << "404 Not Found\r\nMIME-Version:1.0\r\nContent-Type:text/html\r\nContent-Length:9\r\n\r\nNOT FOUND";
		  }
		  else
		  {
			  stream << "200 OK\r\nMIME-Version:1.0\r\nContent-Type:" << contentType << "\r\nContent-Length:" << content.length() << "\r\n\r\n"; //<< content;
		  }
				
		  string response = stream.str();
		  char* rPointer = new char[response.length() + 1];
		  strcpy(rPointer, response.c_str());
		  
		  //Gets rid of the "/" before the filepath
		  request_filepath++;
		  				
		  write(hSocket, rPointer, response.length());
		  sendfile(hSocket, open(request_filepath, O_RDONLY), NULL,  content.length());
		  
		  printf("\nClosing the socket\n\n");
		  /* close socket */
		  memset(pBuffer, 0, sizeof(pBuffer));
		  if(close(hSocket) == SOCKET_ERROR)
			{
			  printf("\nCould not close socket\n");
			  return 0;
			}
       
	//read from the file descriptor (work_object_grabbed)
	//parse the headers
	//find the resource
        //    check if folder or file
	//    Get file contents
	//    get file length
	//    get file extensions for http response
        // format your http response (headers and body)
	// write response you requested

        sem_post( &work_mutex ); //Alright, I'm done. Freeing myself up
        sem_post( &space_on_q ); //We can add one to the space, we finished. 
        
}
}

int main(int argc, char* argv[])
{
    if(argc < 4)
      {
        printf("\nUsage: server host-port thread-count serving-dir\n");
        return 0;
      }
    else
      {
        nHostPort=atoi(argv[1]);
        threadCount = atoi(argv[2]);
        //serving_dir = argv[3];
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

	printf("\nMaking a Thread Pool\n\n");
	
	sem_init( &work_mutex, 0, 1 ); //Only one thread can access this, protects the mutual exclusion - The queue will work with this
    sem_init( &work_to_do, 0, 0);
    sem_init( &space_on_q, 0, 100);

    pthread_t threads[ threadCount ];

    struct thread_info all_thread_info[ threadCount ];

	//Creating a thread pool
    for( int i = 0; i < threadCount; i++ )
    {
        sem_wait( &work_mutex );
 
        printf("creating thread: %d \t\n", i);
        all_thread_info[ i ].thread_id = i;
        pthread_create( &threads[ i ], NULL, serve, ( void* ) &all_thread_info[ i ] );
 
        sem_post( &work_mutex );
    }


    printf("\nMaking a listen queue of %d elements",QUEUE_SIZE);
    /* establish listen queue */
    if(listen(hServerSocket,QUEUE_SIZE) == SOCKET_ERROR)
    {
        printf("\nCould not listen\n");
        return 0;
    }

    for(;;)
    {
        /* get the connected socket */
		
		printf("\nWaiting for a connection\n");
		hSocket= accept(hServerSocket,(struct sockaddr*)&Address,(socklen_t *)&nAddressSize);
				
       	if(hSocket < 0)
       	{
			printf("ERROR: Could not accept connection");
			return 0;
       	}
       	else
       	{
			
			sem_wait( &space_on_q);  //"Yes I have enough space on the queue"
			sem_wait( &work_mutex);  //"Now I need to lock the queue so that nobody tries to take this until i'm done"
        
			work_tasks.push(hSocket); 
        
			sem_post( &work_mutex); //
			sem_post( &work_to_do); //There is now work to do, let's go do it
		}
    }
}
