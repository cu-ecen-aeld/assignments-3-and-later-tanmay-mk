/*
 * @filename 		:	aesdsocket.c
 *
 * @author			: 	Tanmay Mahendra Kothale (tanmay-mk)
 *
 * @date 			:	Feb 19, 2022
 */
/*------------------------------------------------------------------------*/
/*	LIBRARY FILES	*/
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

/*------------------------------------------------------------------------*/
/*	MACROS	*/
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

#define DEBUG				(0) 	//set this to 1 to enable printfs for debug
#define ERROR				(-1)
#define SUCCESS				(0)

/*------------------------------------------------------------------------*/
/*	GLOBAL VARIABLES	*/
int 	sockfd 				= 0;		//socket file descriptor
int 	clientfd 			= 0;		//client file descriptor
int 	packets				= 0;		//size of packet received
int 	status				= 0;		//variable for checking errors
char 	*buffer				= NULL;		//pointer to dynamically allocate space
char 	byte				= 0;		//variable for byte by byte transfer
bool 	isdaemon 			= false;	//boolean variable to check for daemon
struct 	addrinfo *servinfo;				//struct pointer to initialize getaddrinfo
/*------------------------------------------------------------------------*/
/*
 * @brief		: 	When a signal is caught during operation,
 * 					this function is called in response to
 *					the signal. Depending on the signal
 *					value, respective actons are taken.
 *
 * @parameters	:	signal_number	:	value of signal caught
 *
 * @returns		:	none
 */
static void signal_handler(int signal_number);
/*------------------------------------------------------------------------*/
/*
 * @brief		: 	performs cleanup: closes open file descriptors,
 *					frees malloc'd memories, unlinks file storage 
 *					path.
 *
 * @parameters	:	none
 *
 * @returns		:	none
 */
static void clear();
/*------------------------------------------------------------------------*/
/*
 * @brief		: 	handles the entire communication process using
 					socket
 *
 * @parameters	:	none
 *
 * @returns		:	Does not return on success, returns -1 on error.
 */
static int handle_socket();
/*------------------------------------------------------------------------*/
/*
 * @brief		:	Application entry point
 */
/*------------------------------------------------------------------------*/
int main(int argc, char* argv[])
{
    //open syslog 
	openlog("aesdsocket", LOG_PID, LOG_USER);

	//check for daemon
	if (argc >= 2)
    {	
    	//check whether -d is mentioned or not
    	if (strcmp("-d", argv[1]) == 0)
    	{
    		isdaemon = true;
    	}
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

	//handle all socket events
	handle_socket(); 

	//if operation is successful, handle_socket() does not return
	//if it returns, it's an error, perform cleanup and exit
	clear();
	return ERROR;
}
/*------------------------------------------------------------------------*/
static int handle_socket()
{
	struct addrinfo hints;
	struct sockaddr_in clientaddr;
	socklen_t sockaddrsize = sizeof(struct sockaddr_in); 
    ssize_t nread=0;			//total number of bytes data read
    ssize_t nwrite=0;			//total number of bytes data written

	char read_data[BUFFER_SIZE];
    memset(read_data,0,BUFFER_SIZE);
    /*------------------------------------------------------------------------*/

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
		return ERROR;
	}
	syslog(LOG_INFO, "getaddrinfo succeeded!\n");
	#if DEBUG
		printf("getaddrinfo() succeeded\n");
	#endif	
	/*------------------------------------------------------------------------*/

	//create a socket
	sockfd = socket(AF_INET6, SOCK_STREAM, 0);
	if (sockfd == ERROR)
	{
		syslog(LOG_ERR, "Error opening socket.\n");
		#if DEBUG
			printf("socket() failed\n");
		#endif	
		return ERROR;
	}
	syslog(LOG_INFO, "socket succeeded!\n");
	#if DEBUG
		printf("socket() succeeded\n");
	#endif	
	/*------------------------------------------------------------------------*/

	//bind
	status = bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
	if (status == ERROR)
	{
		syslog(LOG_ERR, "Error Binding\n");
		#if DEBUG
			printf("bind() failed\n");
		#endif
		return ERROR;
	}
	syslog(LOG_INFO, "bind succeeded!\n");
	#if DEBUG
		printf("bind() succeeded\n");
	#endif	
	/*------------------------------------------------------------------------*/

	//check if daemon is required, then run as daemon
	if (isdaemon)
	{	
		//change the process' current working directory
		//to root directory & redirect STDIN, STDOUT, 
		//STDERR to /dev/null.
		daemon(NOCHDIR, NOCLOSE);
	}
	/*------------------------------------------------------------------------*/

	//create a file
	int fd = creat(STORAGE_PATH, FILE_PERMISSIONS);
	if (fd == ERROR)
	{
		syslog(LOG_ERR, "File cannot be created");
		#if DEBUG
			printf("creat() failed\n");
		#endif
		return ERROR;
	}
	close(fd);
	syslog(LOG_INFO, "File created!\n");
	#if DEBUG
		printf("creat() succeeded\n");
	#endif	
	/*------------------------------------------------------------------------*/
	//keep handling packets (listen, accept, receive, write, read, send) unless
	//interrupted by a signal or an error occurs
	while (true)
	{
        buffer = (char *) malloc((sizeof(char)*BUFFER_SIZE));
		if(buffer == NULL)
		{
			syslog(LOG_ERR, "Malloc failed!\n");
			#if DEBUG
				printf("malloc() failed\n");
			#endif
			return ERROR;
		}
		syslog(LOG_INFO, "malloc succeded!\n");
		#if DEBUG
			printf("malloc() succeded\n");
		#endif
		memset(buffer,0,BUFFER_SIZE);
		/*------------------------------------------------------------------------*/

		//listen
		status = listen(sockfd, 5);
		if(status == ERROR)
		{
			syslog(LOG_ERR, "Error Listening\n");
			#if DEBUG
				printf("listen() failed\n");
			#endif
			return ERROR;
		}
		syslog(LOG_INFO, "listen() succeeded!\n");
		#if DEBUG
			printf("listen() succeeded\n");
		#endif
		/*------------------------------------------------------------------------*/

		//accept
		clientfd = accept (sockfd, (struct sockaddr *)&clientaddr, &sockaddrsize); //check here
		if (clientfd == ERROR)
		{
			syslog(LOG_ERR, "Error accepting\n");
			#if DEBUG
				printf("accept() failed\n");
			#endif
			return ERROR;
		}
		//print the ip address of the connection
		syslog(LOG_INFO, "accept succeeded! Accepted connection from: %s\n", inet_ntoa(clientaddr.sin_addr));
		#if DEBUG
			printf("accept succeeded! Accepted connection from: %s\n",inet_ntoa(clientaddr.sin_addr));
		#endif
		/*------------------------------------------------------------------------*/

		bool packet_rcvd = false;
		int i;		//loop variable
		status = 0;

		while(!packet_rcvd)
		{
			nread = recv(clientfd, read_data, BUFFER_SIZE, 0);		//receive a packet
			if (nread == ERROR)
			{
				syslog(LOG_ERR, "Receive failed\n");
				#if DEBUG
					printf("recv() failed\n");
				#endif
				return ERROR;
			}
			else if (nread == SUCCESS)
			{
				syslog(LOG_INFO, "recv() succeeded!\n");
				#if DEBUG
					printf("recv() succeeded\n");
				#endif
			}

			for(i = 0;i < BUFFER_SIZE;i++)
			{ // check for end of packet
				if(read_data[i] == '\n')
				{
					packet_rcvd = true;		//end of packet detected, while loop should be 
					i++;					//exited for further packet processing
					break;
				}
			}
			packets += i;			//update the size of packet
			buffer = (char *) realloc(buffer,(size_t)(packets+1));
			if(buffer == NULL)
			{
				syslog(LOG_ERR, "Realloc failed!\n");
				#if DEBUG
					printf("realloc() failed\n");
				#endif
				return ERROR;
			}
			syslog(LOG_INFO, "realloc() succeeded!\n");
			#if DEBUG
				printf("realloc() succeeded\n");
			#endif
			strncat(buffer,read_data,i);
			memset(read_data,0,BUFFER_SIZE);
		}
		/*------------------------------------------------------------------------*/

		fd = open(STORAGE_PATH, O_APPEND | O_WRONLY);		//open the file to write the received data
		if(fd == ERROR)
		{
			syslog(LOG_ERR, "File open failed (append, writeonly) \n");
			#if DEBUG
				printf("open() (append, writeonly) failed\n");
			#endif
			return ERROR;
		}
		syslog(LOG_INFO, "open() succeeded!\n");
		#if DEBUG
			printf("open() (append, writeonly) succeeded\n");
		#endif
		/*------------------------------------------------------------------------*/
		nwrite = write(fd, buffer, strlen(buffer));	//start writing to the file
		if (nwrite == ERROR)
		{
			syslog(LOG_ERR, "write() failed\n");
			#if DEBUG
				printf("write() failed\n");
			#endif
			return ERROR;
		}
		else if (nwrite != strlen(buffer))
		{
			syslog(LOG_ERR, "file partially written\n");
			#if DEBUG
				printf("file partially written\n");
			#endif
			return ERROR;	
		}
		syslog(LOG_INFO, "write() succeeded!\n");
		#if DEBUG
			printf("write() succeeded\n");
		#endif
		close (fd);
		/*------------------------------------------------------------------------*/
		memset(read_data,0,BUFFER_SIZE);		//clear the buffer

		fd = open(STORAGE_PATH,O_RDONLY);			//after the file is written
		if(fd == ERROR)								//open the file to read data and
		{											//send to client			
			syslog(LOG_ERR, "File open failed (readonly)\n");
			#if DEBUG
				printf("open() (readonly) failed\n");
			#endif
			return ERROR;
		}
		syslog(LOG_INFO, "open() (readonly) succeeded!\n");
		#if DEBUG
			printf("open() (readonly) succeeded\n");
		#endif
		/*------------------------------------------------------------------------*/
		status = 0;
		for(i=0; i<packets; i++)
		{
			status = read(fd, &byte, 1);		//start reading each byte
			if (status == ERROR)
			{
				syslog(LOG_ERR, "read failed\n");
				#if DEBUG
					printf("read() failed\n");
				#endif
				return ERROR;
			}
			//syslog(LOG_INFO, "read() succeeded\n");
			//#if DEBUG
				//printf("read() succeeded\n");
			//#endif

			status = 0; 
			status = send(clientfd, &byte, 1, 0);	//send the byte that was just read
			if (status == ERROR)
			{
				syslog(LOG_ERR, "send failed\n");
				#if DEBUG
					printf("send() failed\n");
				#endif
				return ERROR;
			}
			//syslog(LOG_INFO, "send() succeeded\n");
			//#if DEBUG
				//printf("send() succeeded\n");
			//#endif
		}
		/*------------------------------------------------------------------------*/

		close(fd);			//close the open file
		free(buffer);		//free the allocated memory for the packet
		syslog(LOG_INFO,"Closed connection with %s\n", inet_ntoa(clientaddr.sin_addr));
		#if DEBUG
			printf("Closed connection with %s\n",  inet_ntoa(clientaddr.sin_addr));
		#endif
	}
}
/*------------------------------------------------------------------------*/
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
		clear();
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
/*------------------------------------------------------------------------*/
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

/*EOF*/
/*------------------------------------------------------------------------*/
