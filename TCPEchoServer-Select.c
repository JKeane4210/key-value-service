#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include "HandleTCPClient.c"

int main(int argc, char *argv[])
{

    if (argc < 3) // Test for correct number of arguments
        DieWithUserMessage("Parameter(s)", "<Timeout (secs.)> <Port/Service1> ...");

    long timeout = atol(argv[1]); // First arg: Timeout
    int noPorts = argc - 2;       // Number of ports is argument count minus 2

    // Allocate list of sockets for incoming connections
    int servSock[noPorts];
    // Initialize maxDescriptor for use by select()
    int maxDescriptor = -1;

    // Create list of ports and sockets to handle ports
    for (int port = 0; port < noPorts; port++)
    {
        // Create port socket
        servSock[port] = SetupTCPServerSocket(argv[port + 2]);
        printf("port %d: socket number %d\n", port, servSock[port]);

        // Determine if new descriptor is the largest
        if (servSock[port] > maxDescriptor)
            maxDescriptor = servSock[port];
    }

    puts("Starting server: Hit return to shutdown");
    bool running = true; // true if server should continue running
    fd_set sockSet;      // Set of socket descriptors for select()
    while (running)
    {
        /* Zero socket descriptor vector and set for server sockets
        This must be reset every time select() is called */
        FD_ZERO(&sockSet);
        // Add keyboard to descriptor vector
        FD_SET(STDIN_FILENO, &sockSet);
        for (int port = 0; port < noPorts; port++) // sets all the ports to be tried by select()
            FD_SET(servSock[port], &sockSet);

        // Timeout specification; must be reset every time select() is called
        struct timeval selTimeout;   // Timeout for select()
        selTimeout.tv_sec = timeout; // Set timeout (secs.)
        selTimeout.tv_usec = 0;      // 0 microseconds

        // Suspend program until descriptor is ready or timeout
        printf("select\n");
        if (select(maxDescriptor + 1, &sockSet, NULL, NULL, &selTimeout) == 0)
            printf("No echo requests for %ld secs...Server still alive\n", timeout);
        else
        {
            // printf("socketSet: %ld\n", sockSet);
            if (FD_ISSET(0, &sockSet))
            { // Check keyboard
                puts("Shutting down server");
                getchar();
                running = false;
            }

            // Process connection requests
            for (int port = 0; port < noPorts; port++) {
                if (FD_ISSET(servSock[port], &sockSet))
                {
                    printf("Request on port %d: ", port);
                    int socketInUse = AcceptTCPConnection(servSock[port]); // this socket should be non-blocking as the function is currently implemented
                    // but I am not including this in the actual list of things I need to watch over with select
                    // server sockets just show connections, which means that I need to add the sockets to what I am selecting
                    
                    HandleTCPClient(socketInUse);
                    // FD_SET(servSock[port], &sockSet);
                    printf("Socket being used %d\n", socketInUse);
                }
            }
        }
    }

    // Close sockets
    for (int port = 0; port < noPorts; port++)
        close(servSock[port]);

    exit(0);
}