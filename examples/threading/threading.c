#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    int rc;

    // wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    // Wait before obtaining the mutex
    DEBUG_LOG("Waiting before obtaining mutex");
    usleep(thread_func_args->obtain_wait_time*1000);

    // Try get the mutex
    DEBUG_LOG("Getting the mutex");
    rc = pthread_mutex_lock(thread_func_args->mutex);
    if(rc != 0)
    {
        // Something bad happened
        ERROR_LOG("Failed to obtain the mutex, error was %d", rc);
        thread_func_args->thread_complete_success = false;
    }
    else
    {
        // Wait before releasing the mutex
        DEBUG_LOG("Wait before releasing the mutex");
        usleep(thread_func_args->release_wait_time*1000);
        DEBUG_LOG("Releasing the mutex");
        rc = pthread_mutex_unlock(thread_func_args->mutex);

        if(rc != 0)
        {
            // Something bad happened
            ERROR_LOG("Failed to release the mutex, error was %d", rc);
            thread_func_args->thread_complete_success = false;
        }
        else
        {
            // All good
            thread_func_args->thread_complete_success = true;
            DEBUG_LOG("Everything went good, end of thread");
        }
    }

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    int rc = 0;
    /**
     * allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    // Allocate memory for thread data
    struct thread_data* data = malloc(sizeof(struct thread_data));
    if(data != NULL)
    {
        // Set thread data variables
        data->obtain_wait_time = wait_to_obtain_ms;
        data->release_wait_time = wait_to_release_ms;
        data->mutex = mutex; 
        rc = pthread_create(thread, NULL, threadfunc, (void*)data);
        if(rc != 0)
        {
            // Something went wrong
            ERROR_LOG("Failed to create thread, error was %d", rc);
            free(data);
        }
        else
        {
            // All good
            DEBUG_LOG("Thread successfully created!");
            return true;
        }
    }
    else
    {
        ERROR_LOG("Not enough memory to create more threads");
    }

    return false;
}

