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

#define DEBUG 1

#if (DEBUG == 1)
#define DEBUG_LOG(msg,...) printf("aesdsocket: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("aesdsocket ERROR: " msg "\n" , ##__VA_ARGS__)
#else
#define DEBUG_LOG(msg,...)
#define ERROR_LOG(msg,...)
#endif


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
    char connection_info[pClientAddrLen];
    const char* pRetInfo;
    bool error = false, connected = false;
    int rc = 0;
    ssize_t rc2 = 0,byte_count = 0, sent_bytes = 0;
    int ret_code = 0;
    int filedesc = -1;
    char buffer[BUFFER_SIZE] = {0};
    ssize_t file_size = 0;
    int socketfd;
    bool daemon_mode = false;
    int send_flags = 0;
    bool receiving = true;

        // Start syslog
    openlog(NULL, 0, LOG_USER);

    if(argc == 2)
    {
        if(!strcmp(argv[1], "-d"))
        {
            DEBUG_LOG("Requested daemon mode");
            syslog(LOG_USER | LOG_INFO, "Requested daemon mode");
            daemon_mode = true;
        }
        else
        {
            ERROR_LOG("Wrong argument. Argument was %s", argv[1]);
            return -1;
        }
    }
    else if(argc > 2)
    {
        ERROR_LOG("Too many arguments");
        return -1;
    }

    // Signal handling
    struct sigaction custom_action;
    memset(&custom_action, 0, sizeof(custom_action));
    custom_action.sa_handler = sigHandler;

    sigaction(SIGINT, &custom_action, NULL);
    sigaction(SIGTERM, &custom_action, NULL);


    // Populate hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    rc = getaddrinfo(NULL, "9000", &hints, &res);
    if(rc != 0)
    {
        // Error getting addr info
        ERROR_LOG("Error creating addrinfo. Error is %d", rc);
        syslog(LOG_USER | LOG_ERR, "Error creating addrinfo");
        error = true;
        ret_code = -1;

    }

    if(!error)
    {
        // Create socket endpoint
        //socketfd = socket(AF_LOCAL, SOCK_STREAM, 0);
        socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if(socketfd == -1)
        {
            // Error when creating endpoint
            ERROR_LOG("Error creating socket. %s", strerror(errno));
            syslog(LOG_USER | LOG_ERR, "Error creating socket");
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
            ERROR_LOG("Error binding. %s", strerror(errno));
            syslog(LOG_USER | LOG_ERR, "Error binding");
            error = true;
            ret_code = -1;
        }
    }

    if(!error && daemon_mode)
    {
        DEBUG_LOG("Forking to daemon");
        int pid = fork();
        if(pid > 0)
        {
            DEBUG_LOG("Daemon is running, parent says goodbye");
            return 0;
        }
    }

    if(!error)
    {
        rc = listen(socketfd, 10);
        if(rc != 0)
        {
            // Error listening
            ERROR_LOG("Error listening. Error code is %d", errno);
            syslog(LOG_USER | LOG_ERR, "Error listening to socket");
            error = true;
            ret_code = -1;
        }
    }

    if(!error)
    {
        filedesc = open(PATH_TO_FILE, O_RDWR|O_CREAT|O_APPEND|O_SYNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
        if(filedesc < 0)
        {
            // Error creating or opening the file
            ERROR_LOG("Error creating or opening " PATH_TO_FILE);
            syslog(LOG_USER | LOG_ERR, "Error creating or opening " PATH_TO_FILE);
            error = true;
            ret_code = -1;
        }
    }

    while (signal_catched == false && !error)
    {
        if(!error)
        {
            new_fd = accept(socketfd, &client_addr, &pClientAddrLen);
            if(new_fd < 0)
            {
                // Error accpeting connection
                ERROR_LOG("Error Accepting. %s", strerror(errno));
                syslog(LOG_USER | LOG_ERR, "Error accepting connection");
                error = true;
                ret_code = -1;
            }
        }

        if(!error)
        {
            // Get connection address info
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

        // Read data from socket
        if(!error)
        {
            receiving = true;
            while(receiving)
            {
                rc2 = recv(new_fd, buffer, BUFFER_SIZE, 0);
                if(rc2 < 0)
                {
                    // Error receiving
                    ERROR_LOG("Error receiving data from socket");
                    syslog(LOG_USER | LOG_ERR, "Error receiving data from socket");
                    error = true;
                    ret_code = -1;
                    receiving = false;
                    break;
                }
                else if (rc2 > 0)
                {
                    // Copy data to file
                    DEBUG_LOG("Writing to file from socket the text %s", buffer);
                    byte_count = rc2;
                    while (byte_count > 0)
                    {
                        rc2 = write(filedesc, buffer, byte_count);
                        
                        if(rc2 < 0)
                        {
                            // Error writing the file
                            ERROR_LOG("Error writing the file. %s", strerror(errno));
                            syslog(LOG_USER | LOG_ERR, "Error writing the file");
                            error = true;
                            ret_code = -1;
                            break;
                        }
                        byte_count -= rc2;
                    };
                    // Check if reception is complete (ends with new line)
                    if(memchr(buffer, '\n', BUFFER_SIZE) != NULL)
                    {
                        receiving = false;
                    }

                    DEBUG_LOG("Finished writing");
                    
                }
            }
            
        }

        // Read size of file
        if(!error)
        {
            DEBUG_LOG("Reading file size");
            file_size = (ssize_t)lseek(filedesc, 0L, SEEK_END);
            // Bring offset backto beggining
            rc2 = (ssize_t)lseek(filedesc, 0L, SEEK_SET);
            if(rc2 < 0 || file_size < 0)
            {
                // Error writing the file
                ERROR_LOG("Error reading file size");
                syslog(LOG_USER | LOG_ERR, "Error reading file size");
                error = true;
                ret_code = -1;
            }
            DEBUG_LOG("File size is %ld", file_size);
        }

        if(!error)
        {
            DEBUG_LOG("Sending file to socket");
            // Loop to read file and send to socket
            while(file_size > 0)
            {
                // Copy file chunk to buffer
                byte_count = read(filedesc, &buffer, BUFFER_SIZE);
                if(byte_count < 0)
                {
                    ERROR_LOG("Error reading from file");
                    syslog(LOG_USER | LOG_ERR, "Error reading from file");
                    error = true;
                    ret_code = -1;
                    break;
                }
                else
                {
                    DEBUG_LOG("Read %ld bytes", byte_count);
                }

                if(file_size > byte_count)
                {
                    // There is still more data to send
                    send_flags = MSG_MORE;
                }
                else
                {
                    // This is the last chunk of data to transmit
                    send_flags = 0;
                }
                
                // Copy buffer to socket
                sent_bytes = send(new_fd, &buffer, byte_count, send_flags);
                if(sent_bytes < 0)
                {
                    ERROR_LOG("Error writing to socket");
                    syslog(LOG_USER | LOG_ERR, "Error writing to socket");
                    error = true;
                    ret_code = -1;
                    break;
                }
                else
                {
                    DEBUG_LOG("Sent %ld bytes", sent_bytes);
                }
                file_size -= sent_bytes;
                if(byte_count > sent_bytes)
                {
                    DEBUG_LOG("Sent less bytes than read, move offset back");
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

    if(filedesc != -1)
    {
        close(filedesc);
    }

    if(signal_catched)
    {
        DEBUG_LOG("Signal received, removing file");
        // Remove the file
        remove(PATH_TO_FILE);
        ret_code = 0;
    }

    // Free allocated memory
    if(res != NULL)
    {
        freeaddrinfo(res);
    }
    return ret_code;
}


static void sigHandler(int signo)
{
    switch (signo)
    {
    case SIGINT:
        signal_catched = true;
        break;

    case SIGTERM:
        signal_catched = true;
        break;
    
    default:
        break;
    }
}
