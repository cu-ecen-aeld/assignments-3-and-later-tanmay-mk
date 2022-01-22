/*
 *
 */


#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_ARGUMENTS 		(3)
#define FILE_PERMISSION	(0644)

#define DEBUG			(0)

int main(int argc, char *argv[]) 
{
 
 int writeptr, file;
 
 int writesize;
 
 char *writefile, *writestr;
 
 openlog("AESD Assignment 2",LOG_PID,LOG_USER);
 
 //checking number of inputs
 if (argc < MAX_ARGUMENTS)
 {
 	syslog(LOG_ERR, "Error. Arguments missing.\n");
 	exit (1);
 }

 if (argc > MAX_ARGUMENTS)
 {
 	syslog(LOG_ERR, "Error. More arguments provided than required.\n");
 	exit (1);
 }

 writefile = argv[1];
 writestr = argv[2];
 
 writesize = strlen(writestr);
 
 //create a file
 file=creat(writefile, FILE_PERMISSION);
 
 if (file)
 {
 	writeptr = write(file, writestr, writesize);
 	if (writeptr)
 	{
 		if (writeptr != writesize)
 		{
 			syslog(LOG_ERR, "%s file incompletely written.", writefile);
 			exit (1);
 		}
 		syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);
 	}
 	else
 	{
 		syslog(LOG_ERR, "%s file cannot be written", writefile);
 		exit (1);
 	}
 }
 else
 {
 	syslog(LOG_ERR, "File %s cannot be created\n", writefile);
 	exit (1);
 }
 
 closelog();
 return 0;
} // end main

