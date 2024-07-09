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
#include <chrono>
#include <set>
#include <sstream>
#include <string>
#include <iostream>
#include <fstream>
#include <ctime>

using namespace std;

#define DEBUG false
#define nl "\n"

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

void send_request(int sock, char * protocol) {
    // Send the string to the server
    ssize_t numBytes = send(sock, protocol, BUFSIZE, 0);
    if (numBytes < 0)
        DieWithSystemMessage("send() failed");
    else if (numBytes != BUFSIZE)
        DieWithUserMessage("send()", "sent unexpected number of bytes");
}

char * receive_response(int sock) {
    // Receive the same string back from the server
    unsigned int totalBytesRcvd = 0; // Count of total bytes received
    char protocol_buffer[BUFSIZE];
    while (totalBytesRcvd < BUFSIZE)
    {
        char buffer[BUFSIZE]; // I/O buffer
        /* Receive up to the buffer size (minus 1 to leave space for
        a null terminator) bytes from the sender */
        int numBytes = recv(sock, buffer, BUFSIZE, 0);
        if (numBytes < 0) {
            DieWithSystemMessage("recv() failed");
        } else if (numBytes == 0)
            DieWithUserMessage("recv()", "connection closed prematurely");
        
        assert(totalBytesRcvd + numBytes <= BUFSIZE && "Too many bytes sent!");
        memcpy(protocol_buffer + totalBytesRcvd, buffer, numBytes);
        totalBytesRcvd += numBytes; // Keep tally of total bytes
    }

    if (DEBUG) {
        for (int i = 0; i < BUFSIZE; ++i) {
            if (protocol_buffer[i] != 0)
                printf("(%d) %c\n", i, protocol_buffer[i]);
        } printf("\n");
    }

    Protocol* p = reinterpret_cast<Protocol *>(protocol_buffer);
    if (DEBUG) printf("Command Type: %d, In Database: %d, Key: %s, Value: %s\n", (*p).request_type, (*p).in_database, (*p).key, (*p).value);
    return (*p).value;
}

int main(int argc, char *argv[])
{

    if (argc != 6) // Test for correct number of arguments
        DieWithUserMessage("Parameter(s)",
                           "<Server Address> <Server Port> <N_CLIENTS> <DELAY_MS> <N_REQUESTS>");

    char *servIP = argv[1];
    in_port_t servPort = atoi(argv[2]);
    int n_clients = atoi(argv[3]);
    const int SLEEP_MS = atoi(argv[4]);
    const useconds_t SLEEP_US = SLEEP_MS * 1000;
    const int N_REQUESTS = atoi(argv[5]);

    // Create a reliable, stream socket using TCP
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
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

    // load in valid entries
    set<string> entries;
    ifstream is("benchmark_tree.entries");
    int n_entries;
    is >> n_entries;
    for (int i = 0; i < n_entries; ++i) {
        string entry;
        is >> entry;
        entries.insert(entry);
    }

    assert(entries.size() == n_entries 
            && "Not enough unique entries");

    int next_checkpoint = 1;

    srand(time(NULL));
    cout << "key,operation_sequence_number,read_time,event_time\n";

    // start of reads (for benchmarking)
    auto start = chrono::system_clock::now();
    time_t start_t = chrono::system_clock::to_time_t(start);
    auto unix_timestamp = std::chrono::seconds(std::time(NULL));
    cout << "START,0,-1," << unix_timestamp.count() << nl;  

    for (int i = 1; i <= N_REQUESTS / n_clients; ++i) {
        // getting a random key
        int r = rand() % entries.size();
        auto it = entries.begin();
        advance(it, r);
        string random_key_s = *it;
        char random_key[64];
        random_key_s.copy(random_key, KEY_SIZE);

        // code to benchmark
        Protocol p;
        p.request_type = REQUEST_TYPE::GET;
        strcpy(p.key, random_key_s.c_str());
        char * buffer = reinterpret_cast<char *>(&p);
        send_request(sock, buffer);
        char * result = receive_response(sock);
        usleep(SLEEP_US);

        assert(result != nullptr && "Result should not be null");
        assert(strcmp(result, random_key) == 0 && "Correct value not found");

        if (SLEEP_MS == 0 && next_checkpoint == i && n_clients == 1) {
            // final calculations/output to file
            auto end = chrono::system_clock::now();
            chrono::duration<double> elapsed_seconds = end-start;
            auto unix_timestamp = std::chrono::seconds(std::time(NULL));
            cout << random_key << "," << i << "," << elapsed_seconds.count() << "," << unix_timestamp.count() << nl;
            next_checkpoint *= 10;
        }
    }

    unix_timestamp = std::chrono::seconds(std::time(NULL));
    cout << "END," << N_REQUESTS / n_clients << ",-1," << unix_timestamp.count() << nl;

    close(sock);
    exit(0);
    cout.flush();
}