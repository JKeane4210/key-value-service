# Programming Project 2: Key-Value Service

Jonny Keane

This is a TCP/IP Server-Client relationship implemented for concurrent connections with non-blocking I/O to facilitiate a key-value service. The key-value service allows for GET, CONTAINS, and PUT requests to be made according to a binary protocol. The requests can then interact with an on-disk persistent B-Tree data structure implemented in [Project 1](https://github.com/JKeane4210/b-tree).

All code was written from scratch but was inspired by class examples as well as the code examples for *TCP/IP Sockets in C* by Michael J. Donahoo and Kenneth L. Calvert. The code is in C++ and is for the MSML Distributed Databases class.

## Instructions

All files were compiled using g++ as the compiler in WSL.

### Recreating Concurrent Connections Benchmarking Experiment

1. Create a Benchmarking Tree.

    - Compile ```create_benchmark_tree.cpp``` and run the code. This will generate a file called ```benchmark_tree.bin``` that contains 1 million records, inserted in ascending order, in addition to a ```benchmark_tree.entries``` file that describes the keys that were added. Running this typically takes 10-15 minutes to complete.

2. Create server executable and start server.

    - Compile ```TCPServer-Multiplexed.cpp``` and start the server running by calling the executable. It will create a socket for accepting new connections through 0.0.0.0:5000.

    - The commands look like the following:

        ```
        g++ TCPServer-Multiplexed.cpp -o ./server.out
        ./server.out
        ```

    - *LEAVE THIS ACTIVE UNTIL DONE WITH STEP 4 AND PRESS ENTER TO CLOSE THE SERVER.*

3. Compile the client.

    - Compile ```TCPClient.cpp``` to create ```./client.out```:
        
        ```
        g++ TCPClient.cpp -o ./client.out
        ```

4. Run experiments to for 10k requests with different numbers of concurrent connections.

    - I have created a bash script that calls the client executable to set up the experiment correctly. The bash script is ```concurrent_benchmark.sh``` takes an argument for the number of concurrent connections to have:

        ```
        ./concurrent_benchmark.sh <N_CLIENTS>
        ```

    - Under the hood, this script starts asynchronous processes that will call the client executable with the following parameters.

        ```
        ./client.out <IP_ADDRESS> <PORT> <N_CLIENTS> <SLEEP_MS> <N_REQUESTS>
        ```

        - IP_ADDRESS/PORT: Default set to 0.0.0.0:5000 where the server should be running

        - N_CLIENTS: Number of clients that will be running. Will cause program to only run N_REQUESTS / N_CLIENTS for an individual client. *This is what the argument to the bash script sets.*

        - SLEEP_MS: The number of milliseconds for each client to sleep in between requests. Default set to 10.

        - N_REQUESTS: The number of random key GET requests to perform. Default set to 10k.

        - *All output is piped into a file for later evaluation.*

5. Create Final Visual Summarizing Results

    - Using Python 3, with matplotlib and pandas installed, open the Jupyter Notebook ```experiment_results.ipynb``` and run all cells. The notebook's final cell will generate a figure showing the relationship between the time to complete 10k requests and the number of concurrent connections set up to perform the requests.


### Tests

```TCPClient-Tests.cpp``` - While not as robust of tests as the ones conducted on the B-Tree from Project 1, this file will attempt a PUT, GET, and CONTAINS request to make sure that they do not create any errors. The server must be running but then this client will attempt to PUT the key "zzzzz" into the database and attempt a GET and CONTAINS request to ensure the value was added to the database.

### Unused Pieces of Code (Shared for Reference)

- ```test.cpp``` - Testing code for getting the sizes of the Protocol objects being created for sending requests/responses between the client/server.

- ```retrieval_benchmark.sh``` - For the blocking I/O version of the project, we were supposed to just evaluate how long the client took to complete a certain number of requests. While this script should still work by taking in no arguments and calling the client executable, it is not used in the final results shown in the final report submitted for this project.
