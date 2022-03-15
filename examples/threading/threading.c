/*
 * @file		: threading.c
 *
 * @author		: Tanmay Mahendra Kothale (tanmay-mk)
 * 			  Daniel Walkes
 *
 * @date		: 05 Feb, 2022
 */

#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

#define ERROR 		(-1)
#define SUCCESS 	(0)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    int retval; 
    int wait_before_mutex_us = thread_func_args->wait_before_mutex*1000;
    int wait_after_mutex_us = thread_func_args->wait_after_mutex*1000;
    
   // Wait some time before mutex
   retval = usleep(wait_before_mutex_us);
   if (retval == ERROR)
   {
      ERROR_LOG("Failed to sleep before mutex for %d msec.\n", thread_func_args->wait_before_mutex);
      thread_func_args->thread_complete_success = false;
      return thread_param;
   }
   DEBUG_LOG("Successfully slept for %d msec before mutex\n", thread_func_args->wait_before_mutex);
   
   // Obtain mutex lock
   retval = pthread_mutex_lock(thread_func_args->mutex_thread);
   if (retval != SUCCESS)
   {
      ERROR_LOG("Failed to obtain mutex lock with error code %d\n", retval);
      thread_func_args->thread_complete_success = false;
      return thread_param;
   }
   DEBUG_LOG("Successfully obtained mutex lock\n");
   
   // Wait some time after mutex
   retval = usleep(wait_after_mutex_us);
   if (retval == ERROR)
   {
      ERROR_LOG("Failed to sleep after mutex for %d msec.\n", thread_func_args->wait_after_mutex);
      thread_func_args->thread_complete_success = false;
      return thread_param;
   }
   DEBUG_LOG("Successfully slept for %d msec after mutex\n", thread_func_args->wait_after_mutex);
   
   // Obtain mutex lock
   retval = pthread_mutex_unlock(thread_func_args->mutex_thread);
   if (retval != SUCCESS)
   {
      ERROR_LOG("Failed to unlock mutex lock with error code %d\n", retval);
      thread_func_args->thread_complete_success = false;
      return thread_param;
   }
   DEBUG_LOG("Successfully unlocked mutex lock\n");

   // Thread has successfully completed
   DEBUG_LOG("Successfully completed thread\n");
   thread_func_args->thread_complete_success = true;
   return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     * 
     * See implementation details in threading.h file comment block
     */
     
    int retval;
    
    struct thread_data *pthread_params;
    
    pthread_params = (struct thread_data *) malloc(sizeof (struct thread_data));
    if (pthread_params == NULL)
    {
    	ERROR_LOG("Failed to allocate memory for thread\n");
    }
    DEBUG_LOG("Successfully allocated memory for thread\n");
    
   // Copy parameters to structure
   pthread_params->wait_before_mutex = wait_to_obtain_ms;
   pthread_params->wait_after_mutex = wait_to_release_ms;
   pthread_params->mutex_thread = mutex;
  
   retval = pthread_create(thread, NULL, &threadfunc, (void *)pthread_params);
   if (retval != SUCCESS)
   {
      ERROR_LOG("Failed to create thread");
      free (pthread_params);
      return false;
   }
   DEBUG_LOG("Successfully created thread\n");
    
    return true;
}


//EOF

