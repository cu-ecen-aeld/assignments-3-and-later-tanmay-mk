/*
 * @filename 		:	aesdsocket.c
 *
 * @author			: 	Tanmay Mahendra Kothale (tanmay-mk)
 *
 * @date 			:	Feb 19, 2022
 						Updated on Feb 27, 2022
 						Changes for Assignment 6 Part 1
						
						Updated on Mar 13, 2022
						Changes for Assignment 8
 */
/*------------------------------------------------------------------------*/
/*								LIBRARY FILES							  */
/*------------------------------------------------------------------------*/
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
/*								MACROS									  */
/*------------------------------------------------------------------------*/

#define PORT 				"9000"
#define SOCKET_FAMILY 		AF_INET6
#define SOCKET_TYPE			SOCK_STREAM
#define SOCKET_FLAGS		AI_PASSIVE

//parameters for daemon
#define NOCHDIR 			(0)
#define NOCLOSE 			(0)

#define FILE_PERMISSIONS 	(0644)
#define BUFFER_SIZE			(1024)
#define LISTEN_BACKLOG		(5)

#define DEBUG				(0) 	//set this to 1 to enable printfs for debug
#define ERROR				(-1)
#define SUCCESS				(0)

//For Assignment 8.
#define USE_AESD_CHAR_DEVICE	(1)
#if (USE_AESD_CHAR_DEVICE == 1)
	#define STORAGE_PATH  	"/dev/aesdchar";
#else
	#define STORAGE_PATH 	"/var/tmp/aesdsocketdata"
#endif

/*------------------------------------------------------------------------*/
/*				GLOBAL STRUCTURES FOR THREAD & SOCKET HANDLING			  */
/*------------------------------------------------------------------------*/
typedef struct
{
	bool thread_complete;
	pthread_t thread_id;
	int clifd;
	pthread_mutex_t *mutex;
}thread_parameters;

struct slist_data_s
{
	thread_parameters	thread_params;
	SLIST_ENTRY(slist_data_s) entries;
};

typedef struct slist_data_s slist_data_t;
pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;
SLIST_HEAD(slisthead, slist_data_s) head;

/*------------------------------------------------------------------------*/
/*							GLOBAL VARIABLES							  */
/*------------------------------------------------------------------------*/
int 			fd 				= 0;		//file descriptor
int 			sockfd 			= 0;		//socket file descriptor
int 			clientfd 		= 0;		//client file descriptor
int 			status 			= 0;		//variable for checking errors
int 			packets 		= 0;		//size of packet received
char 			byte 			= 0;		//variable for byte by byte transfer
bool 			isdaemon		= false;	//boolean value to check for daemon
slist_data_t 	*datap 			= NULL;		//node data pointer for linked list

/*------------------------------------------------------------------------*/
/* 							FUNCTION PROTOTYPES	 						  */
/*------------------------------------------------------------------------*/
/*
 * @brief		: 	handles the entire communication process using
 *					socket
 *
 * @parameters	:	none
 *
 * @returns		:	Does not return on success, returns -1 on error.
 */
static void handle_socket();
/*------------------------------------------------------------------------*/
/*
 * @brief		: 	handles reading data from file and sending to client
 *					via sockets using threads
 *
 * @parameters	:	none
 *
 * @returns		:	pointer to thread parameters, exits with EXIT_FAILURE on 
 *					error
 */
void* thread_handler(void* thread_params);
#if (USE_AESD_CHAR_DEVICE == 0)
/*------------------------------------------------------------------------*/
/*
 * @brief		: 	handles the SIGALRM signal every time timer goes off
 *
 * @parameters	:	none
 *
 * @returns		:	none, exits with EXIT_FAILURE on error
 */
static void sigalrm_handler();
#endif
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
 * @brief		:	Application entry point
 *					Calls handle_socket() and never exits. If handle_socket()
 *					returns with error, it performs cleanup and terminates
 *					the application with return value -1.
 */
/*------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	openlog("aesdsocket",LOG_PID,LOG_USER);

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

	#if (USE_AESD_CHAR_DEVICE == 0)
	if(signal(SIGALRM, sigalrm_handler)==SIG_ERR)
	{
		syslog(LOG_ERR, "Failed to configure SIGALRM handler\n");
		#if DEBUG
			printf("Failed to configure SIGALRM handler\n");
		#endif
		exit(EXIT_FAILURE);
	}
	#endif

	pthread_mutex_init(&mutex_lock, NULL);

	//check for daemon
	if((argc >= 2) && (!strcmp("-d",(char*)argv[1])))
	{
		isdaemon = true;
	}

	//This function should never return
	//if it returns, an error has occured
	handle_socket();

	//close log
	closelog();

	return ERROR;
}
/*------------------------------------------------------------------------*/
static void handle_socket()
{
	struct addrinfo hints;
	struct addrinfo *results;
	struct sockaddr client_addr;
	socklen_t client_addr_size;
	char ip_address[INET_ADDRSTRLEN];
	struct sockaddr_in *address;
	struct itimerval timer;
	SLIST_INIT(&head);
	/*------------------------------------------------------------------------*/
	memset(&hints,0,sizeof(hints));
	hints.ai_flags 		= SOCKET_FLAGS;
	hints.ai_family 	= SOCKET_FAMILY;
	hints.ai_socktype 	= SOCKET_TYPE;
	status = getaddrinfo(NULL,PORT,&hints,&results);
	if(status != SUCCESS)
	{
		syslog(LOG_ERR,"getaddrinfo() failed\n");
		#if DEBUG
			printf("getaddrinfo() failed\n");
		#endif
		exit(EXIT_FAILURE);
	}
	/*------------------------------------------------------------------------*/ 
	sockfd = socket(PF_INET6, SOCK_STREAM, 0);
	if(sockfd == ERROR)
	{
		syslog(LOG_ERR,"socket() failed\n");
		#if DEBUG
			printf("socket() failed\n");
		#endif
		freeaddrinfo(results);
		exit(EXIT_FAILURE);
	}
	/*------------------------------------------------------------------------*/ 
	status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	if(status == ERROR)
	{
		syslog(LOG_ERR,"setsockopt() failed\n");
		#if DEBUG
			printf("setsockopt() failed\n");
		#endif
		freeaddrinfo(results);
		exit(EXIT_FAILURE);
	}
	/*------------------------------------------------------------------------*/ 
	status = bind(sockfd, results->ai_addr, results->ai_addrlen);
	if(status == ERROR)
	{
		syslog(LOG_ERR,"bind() failed\n");
		#if DEBUG
			printf("bind() failed\n");
		#endif
		freeaddrinfo(results);
		exit(EXIT_FAILURE);
	}
	/*------------------------------------------------------------------------*/ 
	if (isdaemon)
	{
		//change the process' current working directory
		//to root directory & redirect STDIN, STDOUT, 
		//STDERR to /dev/null.
		status = daemon(NOCHDIR,NOCLOSE);
		if(status == ERROR)
		{
			syslog(LOG_ERR,"daemon() failed\n");
			#if DEBUG
				printf("daemon() failed\n");
			#endif
			exit(EXIT_FAILURE);
		}
		syslog(LOG_INFO,"Entering daemon mode\n");
		#if DEBUG
			printf("Entering daemon mode\n");
		#endif
	}
	/*------------------------------------------------------------------------*/ 
	fd = creat(STORAGE_PATH, FILE_PERMISSIONS);
	if(fd == ERROR)
	{
		syslog(LOG_ERR,"creat() failed\n");
		#if DEBUG
			printf("creat() failed\n");
		#endif
		exit(EXIT_FAILURE);
	}
	close(fd);
	freeaddrinfo(results);
	/*------------------------------------------------------------------------*/ 
	//configure timer
	timer.it_interval.tv_sec 	= 10; //timer interval of 10 secs
	timer.it_interval.tv_usec 	= 0;
	timer.it_value.tv_sec 		= 10; //time expiration of 10 secs
	timer.it_value.tv_usec 		= 0;
	status = setitimer(ITIMER_REAL, &timer, NULL);
	if(status == ERROR)
	{
		syslog(LOG_ERR,"setitimer() failed\n");
		#if DEBUG
			printf("setitimer() failed\n");
		#endif
	}
	/*------------------------------------------------------------------------*/
	while(true)
	{
		status = listen(sockfd, LISTEN_BACKLOG);
		if(status == ERROR)
		{
			syslog(LOG_ERR,"listen() failed\n");
			#if DEBUG
				printf("listen() failed\n");
			#endif
			freeaddrinfo(results);
			exit(EXIT_FAILURE);
		}
		/*------------------------------------------------------------------------*/
		client_addr_size = sizeof(struct sockaddr);
		clientfd = accept(sockfd,(struct sockaddr *)&client_addr, &client_addr_size);
		if(clientfd == ERROR)
		{
			syslog(LOG_ERR,"accept() failed\n");
			#if DEBUG
				printf("accept() failed\n");
			#endif
			freeaddrinfo(results);
			exit(EXIT_FAILURE);
		}
		address = (struct sockaddr_in *)&client_addr;
		inet_ntop(AF_INET, &(address->sin_addr),ip_address,INET_ADDRSTRLEN);
		syslog(LOG_INFO,"Accepting connection from %s",ip_address);
		#if DEBUG
			printf("Accepting connection from %s",ip_address);
		#endif
		/*------------------------------------------------------------------------*/
		datap = (slist_data_t *) malloc(sizeof(slist_data_t));
		SLIST_INSERT_HEAD(&head,datap,entries);

		datap->thread_params.clifd 				= clientfd;
		datap->thread_params.thread_complete 	= false;
		datap->thread_params.mutex 				= &mutex_lock;

		status = pthread_create(&(datap->thread_params.thread_id), 			
							NULL,			
							thread_handler,			
							&datap->thread_params
							);

		if (status != SUCCESS)
		{
			syslog(LOG_ERR,"pthread_create() failed\n");
			#if DEBUG
				printf("pthread_create() failed\n");
			#endif
			exit(EXIT_FAILURE);
		}

		syslog(LOG_INFO,"Thread created\n");
		#if DEBUG
			printf("Thread created\n");
		#endif

		SLIST_FOREACH(datap,&head,entries)
		{
			pthread_join(datap->thread_params.thread_id,NULL);
			if(datap->thread_params.thread_complete == true)
			{
				//pthread_join(datap->thread_params.thread_id,NULL);
				SLIST_REMOVE(&head, datap, slist_data_s, entries);
				free(datap);
				break;
			}
		}
		syslog(LOG_INFO,"Thread exited\n");
		#if DEBUG
			printf("Thread exited\n");
		#endif

		syslog(LOG_DEBUG,"Closed connection from %s",ip_address);
		#if DEBUG
			printf("Closed connection from %s",ip_address);
		#endif
		/*------------------------------------------------------------------------*/
	}
	close(clientfd);
	close(sockfd);
}

/*------------------------------------------------------------------------*/
void* thread_handler(void* thread_params)
{
	bool packet_rcvd = false;
	int i;
	char read_data[BUFFER_SIZE] = {0};
	char *buffer = NULL;
	thread_parameters * parameters = (thread_parameters *) thread_params;
	/*------------------------------------------------------------------------*/
	buffer = (char *) malloc(sizeof(char)*BUFFER_SIZE);
	if(buffer == NULL)
	{
		syslog(LOG_ERR,"malloc() failed");
		#if DEBUG
			printf("malloc() failed");
		#endif
		exit(EXIT_FAILURE);
	}

	memset(buffer,0,BUFFER_SIZE);
	/*------------------------------------------------------------------------*/
	while(!packet_rcvd)
	{
		status = recv(parameters->clifd, read_data, BUFFER_SIZE ,0); 
		if(status == ERROR)
		{
			syslog(LOG_ERR,"recv() failed");
			#if DEBUG
				printf("recv() failed");
			#endif
			exit(EXIT_FAILURE);
		}
		/*------------------------------------------------------------------------*/
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
		/*------------------------------------------------------------------------*/
		buffer = (char *) realloc(buffer,(ssize_t)(packets+1));
		if(buffer == NULL)
		{
			syslog(LOG_ERR,"realloc() failed");
			#if DEBUG
				printf("recv() failed");
			#endif
			exit(EXIT_FAILURE);
		}
		strncat(buffer,read_data,i);
		memset(read_data,0,BUFFER_SIZE);
	}
	/*------------------------------------------------------------------------*/
	status = pthread_mutex_lock(parameters->mutex);
	if(status != SUCCESS)
	{
		syslog(LOG_ERR,"pthread_mutex_lock() failed with error code: %d\n",status);
		#if DEBUG
			printf("pthread_mutex_lock() failed with error code: %d\n",status);
		#endif
		exit(EXIT_FAILURE);
	}
	/*------------------------------------------------------------------------*/
	fd = open(STORAGE_PATH,O_APPEND | O_WRONLY);
	if(fd == ERROR)
	{
		syslog(LOG_ERR,"open() failed (append and write only)\n");
		#if DEBUG
			printf("open() failed (append and write only)\n");
		#endif
		exit(EXIT_FAILURE);
	}
	/*------------------------------------------------------------------------*/
	int status = write(fd,buffer,strlen(buffer));
	if(status == ERROR)
	{
		syslog(LOG_ERR,"write() failed\n");
		#if DEBUG
			printf("write() failed\n");
		#endif
		exit(EXIT_FAILURE);
	}
	else if(status != strlen(buffer))
	{
		syslog(LOG_ERR,"File partially written\n");
		#if DEBUG
			printf("File partially written\n");
		#endif
		exit(EXIT_FAILURE);
	}
	close(fd);
	/*------------------------------------------------------------------------*/
	memset(read_data,0,BUFFER_SIZE);
	fd = open(STORAGE_PATH,O_RDONLY);
	if(fd == ERROR)
	{
		syslog(LOG_ERR,"open() failed (read only)\n");
		#if DEBUG
			printf("open() failed (read only)\n");
		#endif
		exit(EXIT_FAILURE);
	}
	/*------------------------------------------------------------------------*/
	for(i=0;i<packets;i++)
	{
		status = read(fd, &byte, 1);
		if(status == ERROR)
		{
			syslog(LOG_ERR,"read() failed!\n");
			#if DEBUG
				printf("read() failed!\n");
			#endif
			exit(EXIT_FAILURE);
		}
		/*------------------------------------------------------------------------*/
		status = send(parameters->clifd,&byte,1,0);
		if(status == ERROR)
		{
			syslog(LOG_ERR,"send() failed!\n");
			#if DEBUG
				printf("send() failed!\n");
			#endif
			exit(EXIT_FAILURE);
		}
	}
	/*------------------------------------------------------------------------*/
	status = pthread_mutex_unlock(parameters->mutex);
	if(status!=SUCCESS)
	{
		syslog(LOG_ERR,"pthread_mutex_unlock() failed with error code: %d\n",status);
		#if DEBUG
			printf("pthread_mutex_unlock() failed with error code: %d\n",status);
		#endif
		exit(EXIT_FAILURE);
	}
	/*------------------------------------------------------------------------*/
	
	parameters->thread_complete = true;

	close(fd);
	close(parameters->clifd);
	free(buffer);

	return parameters;
}
/*------------------------------------------------------------------------*/
static void signal_handler(int signal_number)
{
	switch(signal_number)
	{
		case SIGINT:
			syslog(LOG_DEBUG,"SIGINT Caught! Exiting ... \n");
			#if DEBUG
				printf("SIGINT Caught! Exiting ... \n");
			#endif
			
			while(SLIST_FIRST(&head) != NULL)
			{
				SLIST_FOREACH(datap,&head,entries)
				{
					close(datap->thread_params.clifd);
					pthread_join(datap->thread_params.thread_id,NULL);
					SLIST_REMOVE(&head, datap, slist_data_s, entries);
					free(datap);
					break;
				}
			}

			pthread_mutex_destroy(&mutex_lock);
			unlink(STORAGE_PATH);
			close(clientfd);
			close(sockfd);
			break;
		/*------------------------------------------------------------------------*/
		case SIGTERM:
			syslog(LOG_INFO,"SIGTERM Caught! Exiting ... \n");
			#if DEBUG
				printf("SIGTERM Caught! Exiting ... \n");
			#endif

			while(SLIST_FIRST(&head) != NULL)
			{
				SLIST_FOREACH(datap,&head,entries)
				{
					close(datap->thread_params.clifd);
					pthread_join(datap->thread_params.thread_id,NULL);
					SLIST_REMOVE(&head, datap, slist_data_s, entries);
					free(datap);
					break;
				}
			}
			pthread_mutex_destroy(&mutex_lock);
			unlink(STORAGE_PATH);
			close(clientfd);
			close(sockfd);
			break;
		/*------------------------------------------------------------------------*/
		default:
			exit(EXIT_FAILURE);
			break;
	}
	exit(EXIT_SUCCESS);
}
/*------------------------------------------------------------------------*/

#if (USE_AESD_CHAR_DEVICE==0)
static void sigalrm_handler()
{
	char timestr[200];
	time_t t;
	struct tm *ptr;
	int timer_length = 0;
	/*------------------------------------------------------------------------*/
	t = time(NULL);
	ptr = localtime(&t);
	if(ptr == NULL)
	{
		syslog(LOG_ERR,"localtime() failed\n");
		#if DEBUG
			printf("localtime() failed\n");				
		#endif
		exit(EXIT_FAILURE);
	}
	/*------------------------------------------------------------------------*/

	timer_length = strftime(timestr,sizeof(timestr),"timestamp:%d.%b.%y - %k:%M:%S\n",ptr);
	if(timer_length == SUCCESS)
	{
		syslog(LOG_ERR,"strftime() failed\n");
		#if DEBUG
			printf("strftime() failed\n");				
		#endif
		exit(EXIT_FAILURE);
	}
	/*------------------------------------------------------------------------*/
	fd = open(STORAGE_PATH,O_APPEND | O_WRONLY);
	if(fd == ERROR)
	{
		syslog(LOG_ERR,"open() failed (append and write only)\n");
		#if DEBUG
			printf("open() failed (append and write only)\n");				
		#endif
		exit(EXIT_FAILURE);
	}
	/*------------------------------------------------------------------------*/
	status = pthread_mutex_lock(&mutex_lock);
	if(status != SUCCESS)
	{
		syslog(LOG_ERR,"pthread_mutex_lock() failed with error code %d\n", status);
		#if DEBUG
			printf("pthread_mutex_lock() failed with error code %d\n", status);				
		#endif
		exit(EXIT_FAILURE);
	}
	/*------------------------------------------------------------------------*/
	status = write(fd,timestr,timer_length);
	if(status == ERROR)
	{
		syslog(LOG_ERR,"write() failed\n");
		#if DEBUG
			printf("write() failed\n");				
		#endif
		exit(EXIT_FAILURE);
	}
	else if(status != timer_length)
	{
		syslog(LOG_ERR,"file partially written\n");
		#if DEBUG
			printf("file partially written\n");				
		#endif
		exit(EXIT_FAILURE);
	}
	packets += timer_length;
	/*------------------------------------------------------------------------*/
	status = pthread_mutex_unlock(&mutex_lock);
	if(status != SUCCESS)
	{
		syslog(LOG_ERR,"pthread_mutex_unlock() failed with error code %d\n", status);
		#if DEBUG
			printf("pthread_mutex_unlock() failed with error code %d\n", status);				
		#endif
		exit(EXIT_FAILURE);
	}
	close(fd);
}
#endif
/*EOF*/
/*------------------------------------------------------------------------*/