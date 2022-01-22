/*
 * @file: writer.c
 * 
 * @author: Tanmay Mahendra Kothale (tanmay-mk)
 *
 * @date: January 22, 2022
 */

/*	LIBRARY FILES	*/
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/*	MACROS	*/
#define MAX_ARGUMENTS 		(3)
#define FILE_PERMISSION	(0644)

/*
 * @brief: application entry point
 */
int main(int argc, char *argv[]) 
{
 
 int writeptr, file;
 
 int writesize;  //to compute number of bytes to be written
 
 char *writefile, *writestr;	//to store path of file and name of file
 
 //open a syslog connection
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

 writefile = argv[1]; 		//storing path of file
 writestr = argv[2];		//storing string to be written
 
 writesize = strlen(writestr);	//calculate size of strings in byte
 
 //create a file
 file=creat(writefile, FILE_PERMISSION);
 
 //check if the file was successfully created
 if (file)
 {
 	//write the string to the file
 	writeptr = write(file, writestr, writesize);
 	//check whether data was successfully written to the file
 	if (writeptr)
 	{	
 		//if total bytes written does not match the size of 
 		//string, log the error
 		if (writeptr != writesize)
 		{
 			syslog(LOG_ERR, "%s file incompletely written.", writefile);
 			exit (1);
 		}
 		//else print the success message
 		syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);
 	}
 	//if data is not successfully written, log the error
 	else
 	{
 		syslog(LOG_ERR, "%s file cannot be written", writefile);
 		exit (1);
 	}
 }
 //if file is not successfully created, log the error
 else
 {
 	syslog(LOG_ERR, "File %s cannot be created\n", writefile);
 	exit (1);
 }
 
 //close the syslog connection
 closelog();
 return 0;
} // end main

