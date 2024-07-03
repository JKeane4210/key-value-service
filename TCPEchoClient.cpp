#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <cassert>
#include "Protocol.h"
// #include "HandleTCPClient.c"

const int BUFSIZE = 256;

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

void echo_transaction(int sock, char * echoString, int echoStringLen) {
    printf("echo transaction\n");
    // Send the string to the server
    ssize_t numBytes = send(sock, echoString, echoStringLen, 0);
    // printf("completed send\n");
    if (numBytes < 0)
        DieWithSystemMessage("send() failed");
    else if (numBytes != echoStringLen)
        DieWithUserMessage("send()", "sent unexpected number of bytes");
}

void send_request(int sock, char * protocol) {
    printf("echo transaction\n");
    // Send the string to the server
    ssize_t numBytes = send(sock, protocol, BUFSIZE, 0);
    // printf("completed send\n");
    if (numBytes < 0)
        DieWithSystemMessage("send() failed");
    else if (numBytes != BUFSIZE)
        DieWithUserMessage("send()", "sent unexpected number of bytes");
}

const int RECV_BUFSIZE = 256;

void receive_response(int sock) {
    printf("receive_response\n");
    // Receive the same string back from the server
    unsigned int totalBytesRcvd = 0; // Count of total bytes received
    char protocol_buffer[BUFSIZE];
    // fputs("Received: ", stdout);     // Setup to print the echoed string
    while (totalBytesRcvd < BUFSIZE)
    {
        char buffer[BUFSIZE]; // I/O buffer
        /* Receive up to the buffer size (minus 1 to leave space for
        a null terminator) bytes from the sender */
        int numBytes = recv(sock, buffer, BUFSIZE, 0);
        // printf("Received %d bytes\n", numBytes);
        // printf("Received %d bytes", numBytes);
        if (numBytes < 0) {
            DieWithSystemMessage("recv() failed");
        } else if (numBytes == 0)
            DieWithUserMessage("recv()", "connection closed prematurely");
        
        assert(totalBytesRcvd + numBytes <= BUFSIZE && "Too many bytes sent!");
        memcpy(protocol_buffer + totalBytesRcvd, buffer, numBytes);
        totalBytesRcvd += numBytes; // Keep tally of total bytes
        buffer[totalBytesRcvd] = '\0';    // Terminate the string!
        // fputs(buffer, stdout);      // Print the echo buffer
        // fputc('\n', stdout); // Print a final linefeed
    }

    for (int i = 0; i < BUFSIZE; ++i) {
        if (protocol_buffer[i] != 0)
            printf("(%d) %c\n", i, protocol_buffer[i]);
    } printf("\n");

    Protocol p = *reinterpret_cast<Protocol *>(protocol_buffer);
    printf("Command Type: %d, Key: %s, Value: %s\n", p.request_type, p.key, p.value);
    // fputc('\n', stdout); // Print a final linefeed
}

int main(int argc, char *argv[])
{

    if (argc < 3 || argc > 4) // Test for correct number of arguments
        DieWithUserMessage("Parameter(s)",
                           "<Server Address> <Echo Word> [<Server Port>]");

    char *servIP = argv[1];     // First arg: server IP address (dotted quad)
    char *echoString = argv[2]; // Second arg: string to echo

    // Third arg (optional): server port (numeric). 7 is well-known echo port
    in_port_t servPort = (argc == 4) ? atoi(argv[3]) : 7;

    // Create a reliable, stream socket using TCP
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // int status = fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);

    // if (status == -1){
    //     perror("calling fcntl");
    //     // handle the error.  By the way, I've never seen fcntl fail in this way
    // }
    
    printf("Created socket: %d\n", sock);
    if (sock < 0)
        DieWithSystemMessage("socket() failed");

    // Construct the server address structure
    struct sockaddr_in servAddr;            // Server address
    memset(&servAddr, 0, sizeof(servAddr)); // Zero out structure
    servAddr.sin_family = AF_INET;          // IPv4 address family
                                            // Convert address
    int rtnVal = inet_pton(AF_INET, servIP, &servAddr.sin_addr.s_addr);
    if (rtnVal == 0)
        DieWithUserMessage("inet_pton() failed", "invalid address string");
    else if (rtnVal < 0)
        DieWithSystemMessage("inet_pton() failed");
    servAddr.sin_port = htons(servPort); // Server port

    // Establish the connection to the echo server
    if (connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
        DieWithSystemMessage("connect() failed");

    echoString = new char[BUFSIZE];
    for (int i = 0; i < BUFSIZE; ++i) {
        echoString[i] = i % 2 ? 'k' : 'j';
    }
    size_t echoStringLen = BUFSIZE; // strlen(echoString); // Determine input length

    // for (int i = 0; i < 32; ++i) {
    //     echo_transaction(sock, echoString, BUFSIZE);
    //     if (i % 8 == 7) {
    //         receive_response(sock);
    //     }
    // }

    for (int i = 0; i < 4; ++i) {
        Protocol p;
        p.request_type = REQUEST_TYPE::CONTAINS;
        strcpy(p.key, "hello world");
        char * buffer = reinterpret_cast<char *>(&p);
        send_request(sock, buffer);
        receive_response(sock);
    }

    close(sock);
    exit(0);
}