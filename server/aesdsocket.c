#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>

#define STORAGE_PATH 		"/var/tmp/aesdsocketdata"

#define PORT 				"9000"
#define SOCKET_FAMILY 		AF_INET6
#define SOCKET_TYPE			SOCK_STREAM
#define SOCKET_FLAGS		AI_PASSIVE

//parameters for daemon
#define NOCHDIR 			(0)
#define NOCLOSE 			(0)

#define FILE_PERMISSIONS 	(0644)
#define BUFFER_SIZE			(1024)

#define DEBUG				(0)
#define ERROR				(-1)
#define SUCCESS				(0)


//global variables
int sockfd=0, clientfd=0; 
struct addrinfo *servinfo;
char *buffer = NULL;
int packets=0;
char byte=0;
int status=0;

static void signal_handler(int signal_number);
static void clear();
static int handle_socket();


int main(int argc, char* argv[])
{
    //open syslog 
	openlog("aesdsocket", LOG_PID, LOG_USER);

	//check for daemon
	bool isdaemon = false;
	if ((argc >= 2) && (strcmp("-d", argv[1]) == 0))
    {
    	isdaemon = true;
    }

	//configure signals
	if(signal(SIGINT, signal_handler)==SIG_ERR)
	{
		syslog(LOG_ERR, "Failed to configure SIGINT handler\n");
		#if DEBUG
			printf("Failed to configure SIGINT handler\n");
		#endif
		exit(EXIT_FAILURE);
	}
	if(signal(SIGTERM, signal_handler)==SIG_ERR)
	{
		syslog(LOG_ERR, "Failed to configure SIGTERM handler\n");
		#if DEBUG
			printf("Failed to configure SIGTERM handler\n");
		#endif
		exit(EXIT_FAILURE);
	}

    //run as daemon
	if (isdaemon)
	{
		daemon(NOCHDIR, NOCLOSE);
	}

	handle_socket(); //check here
	clear();
}

static int handle_socket()
{
	struct addrinfo hints;
	struct sockaddr clientaddr;
	socklen_t sockaddrsize = sizeof(struct sockaddr); 
    ssize_t nread=0;
    ssize_t nwrite=0;

	char read_data[BUFFER_SIZE];
    memset(read_data,0,BUFFER_SIZE);
    /*------------------------------------------------------------------------------------------------------------------------------------------------*/

    //clear the structure
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = SOCKET_FAMILY;
	hints.ai_socktype = SOCKET_TYPE;
	hints.ai_flags = SOCKET_FLAGS;

	status = getaddrinfo(NULL, PORT, &hints, &servinfo);
	if(status != SUCCESS)
	{
		syslog(LOG_ERR, "Error in getaddrinfo with error code: %d\n", status);
		#if DEBUG
			printf("getaddrinfo() failed\n");
		#endif		
		clear();
		return ERROR;
	}
	syslog(LOG_INFO, "getaddrinfo succeeded!\n");
	#if DEBUG
		printf("getaddrinfo() succeeded\n");
	#endif	
	/*------------------------------------------------------------------------------------------------------------------------------------------------*/

	//create a socket
	sockfd = socket(AF_INET6, SOCK_STREAM, 0);
	if (sockfd == ERROR)
	{
		syslog(LOG_ERR, "Error opening socket.\n");
		#if DEBUG
			printf("socket() failed\n");
		#endif	
		clear();
		return ERROR;
	}
	syslog(LOG_INFO, "socket succeeded!\n");
	#if DEBUG
		printf("socket() succeeded\n");
	#endif	
	/*------------------------------------------------------------------------------------------------------------------------------------------------*/

	//bind
	status = bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
	if (status == ERROR)
	{
		syslog(LOG_ERR, "Error Binding\n");
		#if DEBUG
			printf("bind() failed\n");
		#endif
		clear();
		return ERROR;
	}
	syslog(LOG_INFO, "bind succeeded!\n");
	#if DEBUG
		printf("bind() succeeded\n");
	#endif	
	/*------------------------------------------------------------------------------------------------------------------------------------------------*/

	//create a file
	int fd = creat(STORAGE_PATH, FILE_PERMISSIONS);
	if (fd == ERROR)
	{
		syslog(LOG_ERR, "File cannot be created");
		#if DEBUG
			printf("creat() failed\n");
		#endif
		clear();
		return ERROR;
	}
	close(fd);
	syslog(LOG_INFO, "File created!\n");
	#if DEBUG
		printf("creat() succeeded\n");
	#endif	
	/*------------------------------------------------------------------------------------------------------------------------------------------------*/

	while (1)
	{
        buffer = (char *) malloc((sizeof(char)*BUFFER_SIZE));
		if(buffer == NULL)
		{
			syslog(LOG_ERR, "Malloc failed!\n");
			#if DEBUG
				printf("malloc() failed\n");
			#endif
			clear();
			return ERROR;
		}
		syslog(LOG_INFO, "malloc succeded!\n");
		#if DEBUG
			printf("malloc() succeded\n");
		#endif
		memset(buffer,0,BUFFER_SIZE);
		/*------------------------------------------------------------------------------------------------------------------------------------------------*/

		//listen
		status = listen(sockfd, 5);
		if(status == ERROR)
		{
			syslog(LOG_ERR, "Error Listening\n");
			#if DEBUG
				printf("listen() failed\n");
			#endif
			clear();
			return ERROR;
		}
		syslog(LOG_INFO, "listen() succeeded!\n");
		#if DEBUG
			printf("listen() succeeded\n");
		#endif
		/*------------------------------------------------------------------------------------------------------------------------------------------------*/

		//accept
		clientfd = accept (sockfd, (struct sockaddr *)&clientaddr, &sockaddrsize); //check here
		if (clientfd == ERROR)
		{
			syslog(LOG_ERR, "Error accepting\n");
			#if DEBUG
				printf("accept() failed\n");
			#endif
			clear();
			return ERROR;
		}
		syslog(LOG_INFO, "accept succeeded! Accepted connection from: %s\n", clientaddr.sa_data);
		#if DEBUG
			printf("accept succeeded! Accepted connection from: %s\n",clientaddr.sa_data);
		#endif
		/*------------------------------------------------------------------------------------------------------------------------------------------------*/

		bool packet_rcvd = false;
		int i;
		status = 0;

		while(packet_rcvd == false)
		{
			nread = recv(clientfd, read_data, BUFFER_SIZE, 0);
			if (nread == ERROR)
			{
				syslog(LOG_ERR, "Receive failed\n");
				#if DEBUG
					printf("recv() failed\n");
				#endif
				clear();
				return ERROR;
			}
			else if (nread == SUCCESS)
			{
				syslog(LOG_INFO, "recv() succeeded!\n");
				#if DEBUG
					printf("recv() succeeded\n");
				#endif
				break;
			}

			for(i = 0;i < BUFFER_SIZE;i++)
			{
				if(read_data[i] == '\n')
				{
					packet_rcvd = true;
					i++;
					break;
				}
			}
			packets += i;
			buffer = (char *) realloc(buffer,(size_t)(packets+1));
			if(buffer == NULL)
			{
				syslog(LOG_ERR, "Realloc failed!\n");
				#if DEBUG
					printf("realloc() failed\n");
				#endif
				clear();
				return ERROR;
			}
			syslog(LOG_INFO, "realloc() succeeded!\n");
			#if DEBUG
				printf("realloc() succeeded\n");
			#endif
			strncat(buffer,read_data,i);
			memset(read_data,0,BUFFER_SIZE);
		}
		/*------------------------------------------------------------------------------------------------------------------------------------------------*/

		fd = open(STORAGE_PATH, O_APPEND | O_WRONLY);
		if(fd == ERROR)
		{
			syslog(LOG_ERR, "File open failed (append, writeonly) \n");
			#if DEBUG
				printf("open() (append, writeonly) failed\n");
			#endif
			clear();
			return ERROR;
		}
		syslog(LOG_INFO, "open() succeeded!\n");
		#if DEBUG
			printf("open() (append, writeonly) succeeded\n");
		#endif
		/*------------------------------------------------------------------------------------------------------------------------------------------------*/
		nwrite = write(fd, buffer, strlen(buffer));
		if (nwrite == ERROR)
		{
			syslog(LOG_ERR, "write() failed\n");
			#if DEBUG
				printf("write() failed\n");
			#endif
			clear();
			return ERROR;
		}
		else if (nwrite != strlen(buffer))
		{
			syslog(LOG_ERR, "file partially written\n");
			#if DEBUG
				printf("file partially written\n");
			#endif
			clear();
			return ERROR;	
		}
		syslog(LOG_INFO, "write() succeeded!\n");
		#if DEBUG
			printf("write() succeeded\n");
		#endif
		close (fd);
		/*------------------------------------------------------------------------------------------------------------------------------------------------*/
		memset(read_data,0,BUFFER_SIZE);

		fd = open(STORAGE_PATH,O_RDONLY);
		if(fd == ERROR)
		{
			syslog(LOG_ERR, "File open failed (readonly)\n");
			#if DEBUG
				printf("open() (readonly) failed\n");
			#endif
			clear();
			return ERROR;
		}
		syslog(LOG_INFO, "open() (readonly) succeeded!\n");
		#if DEBUG
			printf("open() (readonly) succeeded\n");
		#endif
		/*------------------------------------------------------------------------------------------------------------------------------------------------*/
		status = 0;
		for(i=0; i<packets; i++)
		{
			status = read(fd, &byte, 1);
			if (status == ERROR)
			{
				syslog(LOG_ERR, "read failed\n");
				#if DEBUG
					printf("read() failed\n");
				#endif
				clear();
				return ERROR;
			}
			syslog(LOG_INFO, "read() succeeded\n");
			#if DEBUG
				printf("read() succeeded\n");
			#endif

			status = 0; 
			status = send(clientfd, &byte, 1, 0);
			if (status == ERROR)
			{
				syslog(LOG_ERR, "send failed\n");
				#if DEBUG
					printf("send() failed\n");
				#endif
				clear();
				return ERROR;
			}
			syslog(LOG_INFO, "send() succeeded\n");
			#if DEBUG
				printf("send() succeeded\n");
			#endif
		}
		/*------------------------------------------------------------------------------------------------------------------------------------------------*/
		close(fd);
		free(buffer);
	}
}

static void signal_handler(int signal_number)
{
	if (signal_number == SIGINT)
	{
		syslog(LOG_DEBUG, "SIGINT Caught! Exiting ...\n");
		#if DEBUG
			printf("SIGINT Caught! Exiting ...\n");
		#endif
		clear();
	}
	else if (signal_number == SIGTERM)
	{
		syslog(LOG_DEBUG, "SIGTERM Caught! Exiting ...\n");
		#if DEBUG
			printf("SIGTERM Caught! Exiting ...\n");
		#endif
		clear(); //check here
	}
	else
	{
		syslog(LOG_ERR, "Unexpected Signal Caught! Exiting ...\n");
		#if DEBUG
			printf("Unexpected Signal Caught! Exiting ...\n");
		#endif
		clear();
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}

static void clear ()
{
	close (sockfd);
	close (clientfd); 
	freeaddrinfo(servinfo); //check here
	unlink(STORAGE_PATH);
	syslog(LOG_INFO, "Cleared!\n");
	#if DEBUG
		printf("Cleared!\n");
	#endif
	closelog();
}
