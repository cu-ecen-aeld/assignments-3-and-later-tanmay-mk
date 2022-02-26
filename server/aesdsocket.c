/*
 * @filename 		:	aesdsocket.c
 *
 * @author			: 	Tanmay Mahendra Kothale (tanmay-mk)
 *
 * @date 			:	Feb 19, 2022
 						Updated on Feb 25, 2022
 						Changes for Assignment 6 Part 1
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
#include <pthread.h>
#include <sys/queue.h>
#include <sys/time.h>

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
typedef struct
{
	int clifd;
	pthread_mutex_t *mutex;
	pthread_t thread_id;
	bool thread_complete;
}thread_parameters;

//Linked list node
struct slist_data_s
{
	thread_parameters	thread_params;
	SLIST_ENTRY(slist_data_s) entries;
};

typedef struct slist_data_s slist_data_t;
pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;

/*------------------------------------------------------------------------*/
/*	GLOBAL VARIABLES	*/
int  			fd 			= 0;		//file descriptor
int 			sockfd 		= 0;		//socket file descriptor
int 			clientfd 	= 0;		//client file descriptor
int 			packets		= 0;		//size of packet received
int 			status		= 0;		//variable for checking errors
char 			*buffer		= NULL;		//pointer to dynamically allocate space
char 			byte		= 0;		//variable for byte by byte transfer
bool 			isdaemon 	= false;	//boolean variable to check for daemon
slist_data_t 	*datap 		= NULL;
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
void* thread_handler(void* thread_params);
static void timer_handler(int signal_number);
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
	if(signal(SIGALRM, timer_handler)==SIG_ERR)
	{
		syslog(LOG_ERR, "Failed to configure SIGALRM handler\n");
		#if DEBUG
			printf("Failed to configure SIGALRM handler\n");
		#endif
		exit(EXIT_FAILURE);
	}

	pthread_mutex_init(&mutex_lock, NULL);
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
    SLIST_HEAD(slisthead, slist_data_s) head;
	SLIST_INIT(&head);
	char ipv_4[INET_ADDRSTRLEN];
	struct itimerval interval_timer;
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

	status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	if (status == ERROR)
	{
		syslog(LOG_ERR, "setsockopt() failed.\n");
		#if DEBUG
			printf("setsockopt() failed\n");
		#endif	
		return ERROR;
	}
	syslog(LOG_INFO, "setsockopt() succeeded!\n");
	#if DEBUG
		printf("setsockopt() succeeded\n");
	#endif	
	/*------------------------------------------------------------------------*/

	//bind
	status = bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
	if (status == ERROR)
	{
		syslog(LOG_ERR, "Error Binding\n");
		syslog(LOG_ERR, "errno: %d\n", errno);
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
		status = daemon(NOCHDIR, NOCLOSE);
		if (status == ERROR)
		{
			syslog(LOG_ERR, "Failed to run as Daemon\n");
			#if DEBUG
				printf("Failed to run as a daemon\n");
			#endif
			return ERROR;
		}
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
	interval_timer.it_interval.tv_sec 	= 10; //timer interval of 10 secs
	interval_timer.it_interval.tv_usec 	= 0;
	interval_timer.it_value.tv_sec 		= 10; //time expiration of 10 secs
	interval_timer.it_value.tv_usec 	= 0;
	status = setitimer(ITIMER_REAL, &interval_timer, NULL);
	if (status == ERROR)
	{
		syslog(LOG_ERR, "setitimer() failed");
		#if DEBUG
			printf("setitimer() failed\n");
		#endif
		return ERROR;
	}
	/*------------------------------------------------------------------------*/
	//keep handling packets (listen, accept, receive, write, read, send) unless
	//interrupted by a signal or an error occurs
	while (true)
	{
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
		inet_ntop(AF_INET, &(clientaddr.sin_addr),ipv_4,INET_ADDRSTRLEN);
		//Logging the client connection and address
		syslog(LOG_DEBUG,"Accepting connection from %s",ipv_4);
		printf("Accepting connection from %s\n",ipv_4);
		/*------------------------------------------------------------------------*/
		datap = (slist_data_t *) malloc(sizeof(slist_data_t));
		SLIST_INSERT_HEAD(&head,datap,entries);

		//Inserting thread parameters now
		datap->thread_params.clifd = clientfd;
		datap->thread_params.thread_complete = false;
		datap->thread_params.mutex = &mutex_lock;
		pthread_create(&(datap->thread_params.thread_id), 	//the thread id to be created
						NULL,									//the thread attribute to be passed
						thread_handler,							//the thread handler to be executed
						&datap->thread_params				//the thread parameter to be passed
				   	  );

		printf("All thread created now waiting to exit\n");

		SLIST_FOREACH(datap,&head,entries)
		{
			pthread_join(datap->thread_params.thread_id,NULL);
			datap = SLIST_FIRST(&head);
			SLIST_REMOVE_HEAD(&head, entries);
			free(datap);
		}
		printf("All thread exited!\n");
		syslog(LOG_DEBUG,"Closed connection from %s",ipv_4);
		printf("Closed connection from %s\n",ipv_4);
	}
	close(clientfd);
	close(sockfd);
}
/*------------------------------------------------------------------------*/
void* thread_handler(void* thread_params)
{
	bool packet_rcvd = false;
	int i;		//loop variable
	status = 0;
	char read_data[BUFFER_SIZE];
 	memset(read_data,0,BUFFER_SIZE);
 	thread_parameters *parameters = (thread_parameters *) thread_params;
 	ssize_t nread=0;			//total number of bytes data read
    ssize_t nwrite=0;			//total number of bytes data written

 	buffer = (char *) malloc((sizeof(char)*BUFFER_SIZE));
	if(buffer == NULL)
	{
		syslog(LOG_ERR, "Malloc failed!\n");
		#if DEBUG
			printf("malloc() failed\n");
		#endif
		clear();
		exit(EXIT_FAILURE);
	}
	syslog(LOG_INFO, "malloc succeded!\n");
	#if DEBUG
		printf("malloc() succeded\n");
	#endif
	memset(buffer,0,BUFFER_SIZE);
	/*------------------------------------------------------------------------*/
	while(!packet_rcvd)
	{
		nread = recv(clientfd, read_data, BUFFER_SIZE, 0);		//receive a packet
		if (nread == ERROR)
		{
			syslog(LOG_ERR, "Receive failed\n");
			#if DEBUG
				printf("recv() failed\n");
			#endif
			clear();
			exit(EXIT_FAILURE);
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
			clear();
			exit(EXIT_FAILURE);
		}
		syslog(LOG_INFO, "realloc() succeeded!\n");
		#if DEBUG
			printf("realloc() succeeded\n");
		#endif
		strncat(buffer,read_data,i);
		memset(read_data,0,BUFFER_SIZE);
	}

	status = pthread_mutex_lock(parameters->mutex);
	if (status != SUCCESS)
	{
		syslog(LOG_ERR, "pthread_mutex_lock() failed with error number %d\n", status);
		clear();
		exit(EXIT_FAILURE);
	}

	/*------------------------------------------------------------------------*/
	fd = open(STORAGE_PATH, O_APPEND | O_WRONLY);		//open the file to write the received data
	if(fd == ERROR)
	{
		syslog(LOG_ERR, "File open failed (append, writeonly) \n");
		#if DEBUG
			printf("open() (append, writeonly) failed\n");
		#endif
		clear();
		exit(EXIT_FAILURE);
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
		clear();
		exit(EXIT_FAILURE);
	}
	else if (nwrite != strlen(buffer))
	{
		syslog(LOG_ERR, "file partially written\n");
		#if DEBUG
			printf("file partially written\n");
		#endif
		clear();
		exit(EXIT_FAILURE);	
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
		clear();
		exit(EXIT_FAILURE);
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
			clear();
			exit(EXIT_FAILURE);
		}
		//syslog(LOG_INFO, "read() succeeded\n");
		//#if DEBUG
			//printf("read() succeeded\n");
		//#endif
			status = 0; 
		status = send(parameters->clifd, &byte, 1, 0);	//send the byte that was just read
		if (status == ERROR)
		{
			syslog(LOG_ERR, "send failed\n");
			#if DEBUG
				printf("send() failed\n");
			#endif
			clear();
			exit(EXIT_FAILURE);
		}
		//syslog(LOG_INFO, "send() succeeded\n");
		//#if DEBUG
			//printf("send() succeeded\n");
		//#endif
	}
	/*------------------------------------------------------------------------*/

	status = pthread_mutex_unlock (parameters->mutex);
	if (status != SUCCESS)
	{
		syslog(LOG_ERR, "pthread_mutex_unlock() failed with error number %d\n", status);
		clear();
		exit(EXIT_FAILURE);
	}

	close(fd);			//close the open file
	free(buffer);		//free the allocated memory for the packet
	close(parameters->clifd);

	return parameters;

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
	//free(buffer);
	close (sockfd);
	close (clientfd); 
	freeaddrinfo(servinfo);
	unlink(STORAGE_PATH);
	syslog(LOG_INFO, "Cleared!\n");
	#if DEBUG
		printf("Cleared!\n");
	#endif
	closelog();
}
/*------------------------------------------------------------------------*/

static void timer_handler(int signal_number)
{
	int status;
	char timestr[256];
	time_t t;
	struct tm *ptr;
	int timer_length = 0;
	ssize_t nwrite;
	/*------------------------------------------------------------------------*/
	t = time(NULL);
	ptr = localtime(&t);
	if (ptr == NULL)
	{
		syslog(LOG_ERR, "localtime() failed\n");
		clear();
		exit(EXIT_FAILURE);
	}
	syslog(LOG_INFO, "localtime() succeeded\n");
	/*------------------------------------------------------------------------*/
	timer_length = strftime(timestr, sizeof(timestr), "timestamp: %m/%d/%Y - %k:%M:%S\n", ptr);
	if (timer_length == 0)
	{
		syslog(LOG_ERR, "strftime() failed\n");
		clear();
		exit(EXIT_FAILURE);
	}
	syslog(LOG_INFO, "strftime() succeeded\n");
	printf("%s\n",timestr);
	/*------------------------------------------------------------------------*/
	//writing the time to the file
	fd = open(STORAGE_PATH, O_APPEND | O_WRONLY);
	if(fd == ERROR)
	{
		syslog(LOG_ERR, "File open error for appending\n");
		clear();
		exit(EXIT_FAILURE);
	}

	status = pthread_mutex_lock(&mutex_lock);
	if (status != SUCCESS)
	{
		syslog(LOG_ERR, "pthread_mutex_lock() failed with error number %d\n", status);
		clear();
		exit(EXIT_FAILURE);
	}

	nwrite = write(fd, timestr, timer_length);
	if (nwrite == ERROR)
	{
		syslog(LOG_ERR, "write() failed\n");
		clear();
		exit(EXIT_FAILURE);
	}
	else if (nwrite != timer_length)
	{
		syslog(LOG_ERR, "file partially written\n");
		clear();
		exit(EXIT_FAILURE);
	}

	packets += timer_length;

	status = pthread_mutex_unlock(&mutex_lock);
	if (status != SUCCESS)
	{
		syslog(LOG_ERR, "pthread_mutex_unlock() failed with error number %d\n", status);
		clear();
		exit(EXIT_FAILURE);
	}
	close(fd);
}

/*EOF*/
/*------------------------------------------------------------------------*/

