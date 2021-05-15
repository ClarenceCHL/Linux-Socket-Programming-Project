/***
 ** Haolun Cheng
 **/

#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#define SCHEDULER_TCP 34827
#define MAXLINE 1024
#define LOCAL_HOST "127.0.0.1"
#define ERROR_FLAG -1
using namespace std;

int sock; // Client socket
struct sockaddr_in schedulerAddr; // Scheduler address for TCP connection
string indexClient; // Location of client
char assignResult[MAXLINE]; // Hospital assignment result from Scheduler
string response; // Hospital assignment result

/**
 * Create client TCP socket
 */
void createTCPSocket();

/**
 * Connect to Scheduler using TCP
 */
void connectScheduler();

/**
 * Send the location of client(input by user)
 */
void sendUserInput();

/**
 * Get the hospital assignment result from Scheduler
 */
void getFinalAssignment();

/*
 * Main function
 */
int main(int argc, char *argv[]){
    createTCPSocket();

    connectScheduler();

    // Get input from user and store it as the location
    // of the client
    int count = 0;
    while(argv[++count]){
        indexClient = argv[count];
    }

    sendUserInput();

    getFinalAssignment();
    
    // Close the socket
    close(sock);

    return 0;
}

/**
 * Create client TCP socket
 */
void createTCPSocket(){
    // From Beej's tutorial

    // Create a socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == ERROR_FLAG){
        perror("[ERROR]Client: Fail to create socket");
        exit(1);
    }

    // Create a hint structure for the server we are connecting with
    memset(&schedulerAddr, 0, sizeof(schedulerAddr));
    schedulerAddr.sin_family = AF_INET;
    schedulerAddr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    schedulerAddr.sin_port = htons(SCHEDULER_TCP);
}

/**
 * Connect to Scheduler using TCP
 */
void connectScheduler(){
    // From Beej's tutorial
    if (::connect(sock, (struct sockaddr *) &schedulerAddr, sizeof(schedulerAddr)) == ERROR_FLAG){
        perror("[ERROR]Client: Fail to connect to Scheduler");
        exit(1);
    }
    
    cout << "The client is up and running" << endl;
}

/**
 * Send the location of client(input by user)
 */
void sendUserInput(){
    // From Beej's tutorial
    if(::send(sock, indexClient.c_str(), strlen(indexClient.c_str()), 0) == ERROR_FLAG){
        perror("[ERROR]Client: Fail to send the user's input");
        exit(1);
    }
    
    cout << "The client has sent query to Scheduler using TCP: client location ​" << indexClient << endl;
}

/**
 * Get the hospital assignment result from Scheduler
 */
void getFinalAssignment(){
    // From Beej's tutorial
    memset(assignResult, 0, sizeof(assignResult));
    if(::recv(sock, assignResult, sizeof(assignResult), 0) == ERROR_FLAG){
        perror("[ERROR]Client: Fail to receive hospital assignment");
        exit(1);
    }

    response = assignResult;
    
    // Check the assignment result
    if(response == "None"){
        cout << "Score = None, No assignment" << endl;
    }
    else if(response == "INVALID"){
        cout << "Location ​" << indexClient << "​ not found" << endl;
    }
    else{
        cout << "The client has received results from the Scheduler: assigned to Hospital ​" << response << endl;
    }
}