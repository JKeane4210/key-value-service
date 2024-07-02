#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <set>
#include "HandleTCPClient.c"

using namespace std;

int main(int argc, char *argv[]) {
    const static long TIMEOUT = 2;
    const static char* PORT = "5000";

    // set up the socket that is waiting for connections
    int maxDescriptor = -1;
    int connectionSocket = SetupTCPServerSocket(PORT);
    if (connectionSocket > maxDescriptor) {
        maxDescriptor = connectionSocket;
    }

    set<int> activeConnections;

    bool running = true;
    fd_set sockSet;
    while (running) {
        /* Zero socket descriptor vector and set for server sockets
        This must be reset every time select() is called */
        FD_ZERO(&sockSet);
        // Add keyboard and connectionSocket to descriptor vector
        FD_SET(STDIN_FILENO, &sockSet);
        FD_SET(connectionSocket, &sockSet);

        // add everything active connection
        for (int activeConnectionSocket : activeConnections) {
            FD_SET(activeConnectionSocket, &sockSet);
        }

        // Timeout specification; must be reset every time select() is called
        struct timeval selTimeout;   // Timeout for select()
        selTimeout.tv_sec = TIMEOUT; // Set timeout (secs.)
        selTimeout.tv_usec = 0;      // 0 microseconds

        // Suspend program until descriptor is ready or timeout
        if (select(maxDescriptor + 1, &sockSet, NULL, NULL, &selTimeout) == 0)
            printf("No echo requests for %ld secs...Server still alive\n", TIMEOUT);
        else
        {
            // Check keyboard
            if (FD_ISSET(0, &sockSet)) { 
                puts("Shutting down server");
                getchar();
                running = false;
            }

            // Check listening socket
            if (FD_ISSET(connectionSocket, &sockSet)) {
                printf("Request on port %d: ", PORT);
                int acceptedConnectionSocket = AcceptTCPConnection(connectionSocket);
                activeConnections.insert(acceptedConnectionSocket);
                if (acceptedConnectionSocket > maxDescriptor) {
                    maxDescriptor = acceptedConnectionSocket;
                }
            }

            // loop through active connection sockets
            set<int>::iterator it = activeConnections.begin();
            while (it != activeConnections.end()) {
                int numBytesRecvd;
                if (FD_ISSET(*it, &sockSet)) {
                    numBytesRecvd = HandleTCPClient(*it);
                    if (numBytesRecvd == 0) it = activeConnections.erase(it);
                    else it++;
                }
                else it++;
            }
        }
    }
    
    close(connectionSocket);
    exit(0);
}