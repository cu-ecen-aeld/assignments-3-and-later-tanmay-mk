/*
 * @file name		: systemcalls.c
 * 
 * @author		: Dan Walkes
 *			  Tanmay Mahendra Kothale (tanmay-mk)
 *			  
 *			  Made the changes required for ECEN 5713 Advanced 
 *			  Embedded Software Development - Assignment 3 Part 1.
 *
 * @date		: 26 January 2022
 */


/*	LIBRARY FILES	*/
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <syslog.h>

/*	OTHER FILES TO BE INCLUDED	*/
#include "systemcalls.h"

/*	MACROS	*/
#define FILEMODE 0644

/**
 * @param cmd the command to execute with system()
 * @return true if the commands in ... with arguments @param arguments were executed 
 *   successfully using the system() call, false if an error occurred, 
 *   either in invocation of the system() command, or if a non-zero return 
 *   value was returned by the command issued in @param.
*/
bool do_system(const char *cmd)
{

/*
 * TODO : add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success 
 *   or false() if it returned a failure
*/

    int returnval;
    
    openlog("AESD A3 Part 1 - Do System routine", LOG_PID, LOG_USER);

    returnval = system(cmd);
    
    if (returnval == -1)
    {
    	syslog(LOG_ERR, "%s : encountered a system error!", __FUNCTION__);
    	return false;
    }
    syslog(LOG_INFO, "%s : System returned successfully!", __FUNCTION__);
    closelog();
    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the 
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *   
*/
    
    int status;
    pid_t pid;

    openlog("AESD A3 Part 1 - Do exec routine", LOG_PID, LOG_USER);

    pid = fork();
    
    if (pid == -1)
    {
    	syslog(LOG_ERR, "%s : Forking failed.", __FUNCTION__);
    	closelog();
    	return false;
    }
    
    else if (pid == 0) //child process
    {
    	syslog(LOG_INFO, "%s : child process created successfully, inside child process with pid %d.", __FUNCTION__, pid);
    	execv(command[0], command);
    	//on success, execv does not return, if it returns, 
    	//an error has occurred
    	syslog(LOG_ERR, "%s : execv failed", __FUNCTION__);
    	closelog();
    	exit(EXIT_FAILURE);
    }
    
    else
    {
    	syslog(LOG_INFO, "%s : inside parent process with pid %d.", __FUNCTION__, pid);
    	if(waitpid(pid,&status,0) == -1)
    	{
            syslog(LOG_ERR, "%s : waitpid failed", __FUNCTION__);
            closelog();
            return false;
        }
        
        if ( ! (WIFEXITED(status)) 
        ||    (WEXITSTATUS(status))
           )
        {
            syslog(LOG_ERR, "%s : waitpid failed", __FUNCTION__);
            closelog();
            return false;
        }
    }

    va_end(args);
    syslog(LOG_INFO, "do execv succeeded");
    closelog();
    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.  
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *   
*/
    openlog("AESD A3 Part 1 - Do exec redirect routine", LOG_PID, LOG_USER);
    
    int status;
    pid_t pid;

    int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, FILEMODE);
    if(fd == -1)
    {
        syslog(LOG_ERR, "%s : error while creating and opening file", __FUNCTION__);
        closelog();
        return false;
    }

    pid = fork();
    
    if (pid == -1)
    {
    	syslog(LOG_ERR, "%s : Forking failed.", __FUNCTION__);
    	closelog();
    	return false;
    }
    
    else if (pid == 0) //Child process
    { 
	syslog(LOG_INFO, "%s : child process created successfully, inside child process with pid %d.", __FUNCTION__, pid);
        if(dup2(fd,1) < 0)
        {
            syslog(LOG_ERR, "%s : dup2 error occurred", __FUNCTION__);
            closelog();
            return false;
        }
        
        close(fd);
        execv(command[0],command);
        
    	//on success, execv does not return, if it returns, 
    	//an error has occurred
    	syslog(LOG_ERR, "%s : execv failed", __FUNCTION__);
    	closelog();
    	exit(EXIT_FAILURE);
    }
    else
    {
        close(fd);

    	syslog(LOG_INFO, "%s : inside parent process with pid %d.", __FUNCTION__, pid);
    	if(waitpid(pid,&status,0) == -1)
    	{
            syslog(LOG_ERR, "%s : waitpid failed", __FUNCTION__);
            closelog();
            return false;
        }
        
        if ( ! (WIFEXITED(status)) 
        ||    (WEXITSTATUS(status))
           )
        {
            syslog(LOG_ERR, "%s : waitpid failed", __FUNCTION__);
            closelog();
            return false;
        }

    }

    va_end(args);
    syslog(LOG_INFO, "do execv redirect succeeded");
    closelog();
    return true;
}
