#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("aesdsocket: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("aesdsocket ERROR: " msg "\n" , ##__VA_ARGS__)

#define PATH_TO_FILE "/var/tmp/aesdsocketdata"
#define BUFFER_SIZE 1024


static void sigHandler(int signo);


bool signal_catched = false;


int main(int argc, char* argv[])
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    int new_fd;
    struct sockaddr client_addr;
    socklen_t pClientAddrLen = sizeof(client_addr);
    const char* pRetInfo;
    bool error = false, connected = false;
    int rc = 0, byte_count = 0, sent_bytes = 0;
    int ret_code = 0;
    int filedesc;
    int buffer_size;
    char* buffer = NULL;
    char write_buff[BUFFER_SIZE] = {0};
    ssize_t file_size = 0;

    // Signal handling
    struct sigaction custom_action;
    custom_action.sa_handler = sigHandler;

    memset(&custom_action, 0, sizeof(custom_action));

    // Start syslog
    openlog(NULL, 0, LOG_USER);
    // Create socket endpoint
    int socketfd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if(socketfd == -1)
    {
        // Error when creating endpoint
        ERROR_LOG("Error creating socket. Error code is %d", errno);
        syslog(LOG_USER | LOG_ERR, "Error creating socket");
        error = true;
        ret_code = -1;
    }

    // Populate hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;     // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    if(!error)
    {
        rc = getaddrinfo(NULL, "9000", &hints, &res);
        if(rc != 0)
        {
            // Error getting addr info
            ERROR_LOG("Error creating addrinfo. Error is %d", rc);
            syslog(LOG_USER | LOG_ERR, "Error creating addrinfo");
            error = true;
            ret_code = -1;
    
        }
    }

    if(!error)
    {
        rc = bind(socketfd, res->ai_addr, res->ai_addrlen);
        if(rc != 0)
        {
            // Error binding
            ERROR_LOG("Error binding. Error code is %d", errno);
            syslog(LOG_USER | LOG_ERR, "Error binding");
            error = true;
            ret_code = -1;
        }
    }
    if(!error)
    {
        rc = listen(socketfd, 0);
        if(rc != 0)
        {
            // Error listening
            ERROR_LOG("Error listening. Error code is %d", errno);
            syslog(LOG_USER | LOG_ERR, "Error listening to socket");
            error = true;
            ret_code = -1;
        }
    }

    while (signal_catched == false)
    {
        if(!error)
        {
            new_fd = accept(socketfd, &client_addr, &pClientAddrLen);
            if(new_fd < 0)
            {
                // Error accpeting connection
                ERROR_LOG("Error Accepting. Error code is %d", errno);
                syslog(LOG_USER | LOG_ERR, "Error accepting connection");
                error = true;
                ret_code = -1;
            }
        }

        if(!error)
        {
            // Get connection address info
            char connection_info[pClientAddrLen];
            pRetInfo = inet_ntop(client_addr.sa_family, client_addr.sa_data, connection_info, pClientAddrLen);
            if(pRetInfo == NULL)
            {
                ERROR_LOG("Error getting connection info");
                syslog(LOG_USER | LOG_ERR, "Error getting connection info");
                error = true;
                ret_code = -1;
            }
            else
            {
                connected = true;
                DEBUG_LOG("Accepted connection from %s", pRetInfo);
                syslog(LOG_USER | LOG_INFO, "Accepted connection from %s", pRetInfo);
            }
        }

        if(!error)
        {
            filedesc = open(PATH_TO_FILE, O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
            if(filedesc < 0)
            {
                // Error creating or opening the file
                ERROR_LOG("Error creating or opening " PATH_TO_FILE);
                syslog(LOG_USER | LOG_ERR, "Error creating or opening " PATH_TO_FILE);
                error = true;
                ret_code = -1;
            }
        }

        // check size of available data
        if(!error)
        {
            rc = ioctl(new_fd, FIONREAD, &buffer_size);
            if(rc < 0)
            {
                //Error getting size of data available
                ERROR_LOG("Error getting size of data from socket");
                syslog(LOG_USER | LOG_ERR, "Error getting size of data from socket");
                error = true;
                ret_code = -1;
            }
            else
            {
                buffer = (char*)malloc(buffer_size);
                if(buffer == NULL)
                {
                    ERROR_LOG("Error: Not enough heap");
                    syslog(LOG_USER | LOG_ERR, "Error: Not enough heap");
                    error = true;
                    ret_code = -1;
                }
            }
        }
        // Read data from socket
        if(!error)
        {
            rc = recv(new_fd, buffer, buffer_size, 0);
            if(rc < 0)
            {
                // Error receiving
                ERROR_LOG("Error receiving data from socket");
                syslog(LOG_USER | LOG_ERR, "Error receiving data from socket");
                error = true;
                ret_code = -1;
            }
            else if (rc > 0)
            {
                // Copy data to file
                DEBUG_LOG("Writing to file from socket");
                byte_count = 0;
                do
                {
                    /* code */
                } while (byte_count > 0);
                
                byte_count = write(filedesc, buffer, buffer_size);
                if(rc < 0)
                {
                    // Error writing the file
                    ERROR_LOG("Error writing the file");
                    syslog(LOG_USER | LOG_ERR, "Error rwriting the file");
                    error = true;
                    ret_code = -1;
                }
            }
            
        }
        // We ended using the receive buffer
        free(buffer);
        
        // Read size of file
        if(!error)
        {
            file_size = lseek(filedesc, 0L, SEEK_END);
            // Bring offset backto beggining
            rc = lseek(filedesc, 0L, SEEK_SET);
            if(rc < 0 || file_size < 0)
                {
                    // Error writing the file
                    ERROR_LOG("Error reading file size");
                    syslog(LOG_USER | LOG_ERR, "Error reading file size");
                    error = true;
                    ret_code = -1;
                }
        }

        if(!error)
        {
            // Loop to read file and send to socket
            while(file_size > 0)
            {
                // Copy file chunk to buffer
                byte_count = read(filedesc, &write_buff, BUFFER_SIZE);
                if(byte_count < 0)
                {
                    ERROR_LOG("Error reading from file");
                    syslog(LOG_USER | LOG_ERR, "Error reading from file");
                    error = true;
                    ret_code = -1;
                    break;
                }
                
                // Copy buffer to socket
                sent_bytes = send(socketfd, &write_buff, byte_count, 0);
                if(sent_bytes < 0)
                {
                    ERROR_LOG("Error writing to socket");
                    syslog(LOG_USER | LOG_ERR, "Error writing to socket");
                    error = true;
                    ret_code = -1;
                    break;
                }
                file_size -= sent_bytes;
                if(byte_count > sent_bytes)
                {
                    // send operation was interrupted, move back index
                    lseek(filedesc, sent_bytes-byte_count, SEEK_SET);
                }

            }
        }

        if(connected)
        {
            connected = false;
            DEBUG_LOG("Closed connection from %s", pRetInfo);
            syslog(LOG_USER | LOG_INFO, "Closed connection from %s", pRetInfo);
        }
    }

    if(signal_catched)
    {
        // Remove the file
        remove(PATH_TO_FILE);
    }

    // Free allocated memory
    freeaddrinfo(res);
    return ret_code;
}


static void sigHandler(int signo)
{
    signal_catched = true;
}
