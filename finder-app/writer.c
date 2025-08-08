/*
Application to write a string into given file. If file doesn't exist, create it
*/

#include <stdint.h>
#include <stdio.h>
#include <syslog.h>


int main(int argc, char* argv[])
{
    int retval = 0;
    openlog(NULL, 0, LOG_USER);
    if(argc < 3)
    {
        // Error, not enough arguments
        syslog(LOG_USER | LOG_ERR, "Not enough arguments");
        return 1;
    }
    char* file_path = argv[1]; //First argument is the path to the file
    char* text = argv[2]; //Second argument is the string to be printed

    FILE* fptr = fopen(file_path, "w");
    if(fptr == NULL)
    {
        // Error while opening the file
        syslog(LOG_USER | LOG_ERR, "Error opening the file");
        return 1;
    }
    // Write to the file and log debug message
    syslog(LOG_USER | LOG_DEBUG, "Writing %s to %s", text, file_path);
    retval = fprintf(fptr, "%s", text);
    if(retval < 0)
    {
        syslog(LOG_USER | LOG_ERR, "Error while writing the file");
        return 1;
    }

    retval = fclose(fptr);
    if(retval < 0)
    {
        syslog(LOG_USER | LOG_ERR, "Error while closing the file");
        return 1;
    }
    return 0;
}