#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <cassert>

const int BUFSIZE = 256;

struct ProtocolBuffer {
    int totalBytes = 0;
    char buffer[BUFSIZE];
};

void DieWithUserMessage(const char *msg, const char *detail)
{
    fputs(msg, stderr);
    fputs(": ", stderr);
    fputs(detail, stderr);
    fputc('\n', stderr);
    exit(1);
}

void DieWithSystemMessage(const char *msg)
{
    perror(msg);
    exit(1);
}

// void HandleTCPClient(int clntSocket)
// {
//     char buffer[BUFSIZE]; // Buffer for echo string

//     // Receive message from client
//     ssize_t numBytesRcvd = recv(clntSocket, buffer, BUFSIZE, MSG_PEEK); // checks to see how much we can get
//     printf("Received %ld bytes from %d\n", numBytesRcvd, clntSocket);
//     if (numBytesRcvd < 0)
//         DieWithSystemMessage("recv() failed");

//     // Send received string and receive again until end of stream
//     // if (numBytesRcvd > 0)
//     // { // 0 indicates end of stream
//         // Echo message back to client
        
        

//         // // See if there is more data to receive -> could block though
//         // numBytesRcvd = recv(clntSocket, buffer, BUFSIZE, 0);
//         // printf("Received %ld bytes from %d\n", numBytesRcvd, clntSocket);
//         // if (numBytesRcvd < 0)
//         //     DieWithSystemMessage("recv() failed");
//     // }

//     if (numBytesRcvd == 0) {
//         ssize_t numBytesSent = send(clntSocket, buffer, numBytesRcvd, 0);
//         printf("Sent %ld bits\n", numBytesSent);
//         if (numBytesSent < 0)
//             DieWithSystemMessage("send() failed");
//         else if (numBytesSent != numBytesRcvd)
//             DieWithUserMessage("send()", "sent unexpected number of bytes");
//         close(clntSocket); // Close client socket
//     } else if (numBytesRcvd == BUFSIZE) {
//         ssize_t numBytesRcvd = recv(clntSocket, buffer, BUFSIZE, 0); // only reads if buffer is full
//         printf("Received %ld bytes from %d\n", numBytesRcvd, clntSocket);
//         if (numBytesRcvd < 0)
//             DieWithSystemMessage("recv() failed");
//     }
// }

// void HandleTCPClient(int clntSocket) {
//     char buffer[BUFSIZE];

//     ssize_t numBytesRcvd = recv(clntSocket, buffer, BUFSIZE, 0);
//     printf("Received %ld bytes\n", numBytesRcvd);
//     if (numBytesRcvd < 0) {
//         if (errno == EAGAIN || errno == EWOULDBLOCK) return; // if would block, break out of this call and go back into the select() loop
//         else DieWithSystemMessage("recv() failed");
//     }

//     while (numBytesRcvd > 0) {
//         ssize_t numBytesSent = send(clntSocket, buffer, numBytesRcvd, 0);
//         printf("Sent %ld bytes\n", numBytesSent);
//         if (numBytesSent < 0)
//             DieWithSystemMessage("send() failed");
//         else if (numBytesSent != numBytesRcvd)
//             DieWithUserMessage("send()", "sent unexpected number of bytes");
//         if (numBytesRcvd < 0) {
//             if (errno == EAGAIN || errno == EWOULDBLOCK) return; // if would block, break out of this call and go back into the select() loop
//             else DieWithSystemMessage("recv() failed");
//         }
//         numBytesRcvd = recv(clntSocket, buffer, BUFSIZE, 0);
//         printf("Received %d bytes\n", numBytesRcvd);
//         if (numBytesRcvd < 0) {
//             if (errno == EAGAIN || errno == EWOULDBLOCK) return; // if would block, break out of this call and go back into the select() loop
//             else DieWithSystemMessage("recv() failed");
//         }
//     }

//     close(clntSocket);
// }

int HandleTCPClient(int clntSocket) {
    char buffer[BUFSIZE];

    ssize_t numBytesRcvd = recv(clntSocket, buffer, BUFSIZE, 0);
    if (numBytesRcvd < 0) {
        DieWithSystemMessage("recv() failed");
    }
    printf("Received %d bytes\n", numBytesRcvd);

    ssize_t numBytesSent = send(clntSocket, buffer, numBytesRcvd, 0);
    if (numBytesSent < 0)
        DieWithSystemMessage("send() failed");
    else if (numBytesSent != numBytesRcvd)
        DieWithUserMessage("send()", "sent unexpected number of bytes");

    if (numBytesRcvd == 0) {
        close(clntSocket);
    }

    return numBytesRcvd;
}

int HandleTCPKeyValueServiceClient(int clntSocket, struct ProtocolBuffer *pb) {
    char buffer[BUFSIZE];

    ssize_t numBytesRcvd = recv(clntSocket, buffer, BUFSIZE, 0);
    if (numBytesRcvd < 0) {
        DieWithSystemMessage("recv() failed");
    }
    printf("Received %d bytes\n", numBytesRcvd);

    // closes connection if client socket communicated that it has closed
    if (numBytesRcvd == 0) {
        close(clntSocket);
        return numBytesRcvd;
    }

    assert((*pb).totalBytes + numBytesRcvd <= BUFSIZE && "Too many bytes sent!");
    memcpy((*pb).buffer + (*pb).totalBytes, buffer, numBytesRcvd);
    (*pb).totalBytes += numBytesRcvd;

    if ((*pb).totalBytes == BUFSIZE) {
        ssize_t numBytesSent = send(clntSocket, (*pb).buffer, BUFSIZE, 0);
        if (numBytesSent < 0)
            DieWithSystemMessage("send() failed");
        else if (numBytesSent != BUFSIZE)
            DieWithUserMessage("send()", "sent unexpected number of bytes");
        else
            (*pb).totalBytes = 0; // reset buffer for a new request
    }

    return numBytesRcvd;
}

void PrintSocketAddress(const struct sockaddr *address, FILE *stream)
{
    // Test for address and stream
    if (address == NULL || stream == NULL)
        return;

    void *numericAddress; // Pointer to binary address
    // Buffer to contain result (IPv6 sufficient to hold IPv4)
    char addrBuffer[INET6_ADDRSTRLEN];
    in_port_t port; // Port to print
    // Set pointer to address based on address family
    switch (address->sa_family)
    {
    case AF_INET:
        numericAddress = &((struct sockaddr_in *)address)->sin_addr;
        port = ntohs(((struct sockaddr_in *)address)->sin_port);
        break;
    case AF_INET6:
        numericAddress = &((struct sockaddr_in6 *)address)->sin6_addr;
        port = ntohs(((struct sockaddr_in6 *)address)->sin6_port);
        break;
    default:
        fputs("[unknown type]", stream); // Unhandled type
        return;
    }
    // Convert binary to printable address
    if (inet_ntop(address->sa_family, numericAddress, addrBuffer,
                  sizeof(addrBuffer)) == NULL)
        fputs("[invalid address]", stream); // Unable to convert
    else
    {
        fprintf(stream, "%s", addrBuffer);
        if (port != 0) // Zero not valid in any socket addr
            fprintf(stream, "-%u", port);
    }
}

int AcceptTCPConnection(int servSock)
{
    struct sockaddr_storage clntAddr; // Client address
    // Set length of client address structure (in-out parameter)
    socklen_t clntAddrLen = sizeof(clntAddr);

    // Wait for a client to connect
    int clntSock = accept(servSock, (struct sockaddr *)&clntAddr, &clntAddrLen);
    printf("connecting servSock (%d) and clntSock (%d) <- connection to talk over\n", servSock, clntSock);
    if (clntSock < 0)
        DieWithSystemMessage("accept() failed");

    // clntSock is connected to a client!
    // where socketfd is the socket you want to make non-blocking
    int status = fcntl(clntSock, F_SETFL, fcntl(clntSock, F_GETFL, 0) | O_NONBLOCK);

    if (status == -1){
        perror("calling fcntl");
        // handle the error.  By the way, I've never seen fcntl fail in this way
    }

    fputs("Handling client ", stdout);
    PrintSocketAddress((struct sockaddr *)&clntAddr, stdout);
    fputc('\n', stdout);

    return clntSock;
}

static const int MAXPENDING = 5; // Maximum outstanding connection requests

int SetupTCPServerSocket(const char *service)
{
    // Construct the server address structure
    struct addrinfo addrCriteria;                   // Criteria for address match
    memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
    addrCriteria.ai_family = AF_UNSPEC;             // Any address family
    addrCriteria.ai_flags = AI_PASSIVE;             // Accept on any address/port
    addrCriteria.ai_socktype = SOCK_STREAM;         // Only stream sockets
    addrCriteria.ai_protocol = IPPROTO_TCP;         // Only TCP protocol

    struct addrinfo *servAddr; // List of server addresses
    int rtnVal = getaddrinfo(NULL, service, &addrCriteria, &servAddr);
    if (rtnVal != 0)
        DieWithUserMessage("getaddrinfo() failed", gai_strerror(rtnVal));

    int servSock = -1;
    for (struct addrinfo *addr = servAddr; addr != NULL; addr = addr->ai_next)
    {
        // Create a TCP socket
        servSock = socket(servAddr->ai_family, servAddr->ai_socktype,
                          servAddr->ai_protocol);

        if (servSock < 0)
            continue; // Socket creation failed; try next address

        // Bind to the local address and set socket to list
        if ((bind(servSock, servAddr->ai_addr, servAddr->ai_addrlen) == 0) &&
            (listen(servSock, MAXPENDING) == 0))
        {
            // Print local address of socket
            struct sockaddr_storage localAddr;
            socklen_t addrSize = sizeof(localAddr);
            if (getsockname(servSock, (struct sockaddr *)&localAddr, &addrSize) < 0)
                DieWithSystemMessage("getsockname() failed");
            fputs("Binding to ", stdout);
            PrintSocketAddress((struct sockaddr *)&localAddr, stdout);
            fputc('\n', stdout);
            break; // Bind and list successful
        }

        close(servSock); // Close and try again
        servSock = -1;
    }

    // Free address list allocated by getaddrinfo()
    freeaddrinfo(servAddr);

    return servSock;
}