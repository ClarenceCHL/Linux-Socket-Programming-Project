/***
 ** Haolun Cheng
 **/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <cstring>

using namespace std;

#define LOCAL_HOST "127.0.0.1"
#define MAX_DATA_SIZE 1024
#define ERROR_FLAG -1
#define INVALIDCLIENT -2
#define HOSPITAL_A_PORT 30827
#define HOSPITAL_B_PORT 31827
#define HOSPITAL_C_PORT 32827
#define SCHEDULER_UDP 33827
#define SCHEDULER_TCP 34827
#define BACKLOG 100

int sockfdClient, sockfdUDP; // Parent TCP socket and UDP socket
int childSockfdClient; // Child TCP socket for connecting with Client
struct sockaddr_in mainClientAddr, mainUDPAddr; // Initialize Scheduler address information for both TCP and UDP connections
struct sockaddr_in destClientAddr, destHospitalAAddr, destHospitalBAddr, destHospitalCAddr; // Initialize Client, Hospital A, B, and C address information
string Acapacity, Aoccupancy, Bcapacity, Boccupancy, Ccapacity, Coccupancy; // Keep track of the availability of each hospital
int AcapacityValue, AoccupancyValue, BcapacityValue, BoccupancyValue, CcapacityValue, CoccupancyValue;
double finalScoreA, finalScoreB, finalScoreC; // Final scores of each hospital
double finalDistanceA, finalDistanceB, finalDistanceC; // Final distances of each hospital
double decision; // Assignment result for Client
string DECISION;

char clientInput[MAX_DATA_SIZE]; // Location of Client(input by user)
char AInfo[MAX_DATA_SIZE]; // Hospital A capacity and occupancy
char BInfo[MAX_DATA_SIZE]; // Hospital B capacity and occupancy
char CInfo[MAX_DATA_SIZE]; // Hospital C capacity and occupancy
char scoreA[MAX_DATA_SIZE]; // Final score for hospital A
char scoreB[MAX_DATA_SIZE]; // Final score for hospital B
char scoreC[MAX_DATA_SIZE]; // Final score for hospital C
char distanceA[MAX_DATA_SIZE]; // Distance between hospital A and client
char distanceB[MAX_DATA_SIZE]; // Distance between hospital B and client
char distanceC[MAX_DATA_SIZE]; // Distance between hospital C and client
char assignResult[MAX_DATA_SIZE]; // Final assignment result from scheduler to client

/**
 * Create client TCP parent socket(create, bind and listen)
 */
void createClientTCPSocket();

/**
 * Create UDP socket(create and bind)
 */
void createUDPSocket();

/**
 * Connect to hospital A using UDP
 */
void connectToHospitalA();

/**
 * Connect to hospital B using UDP
 */
void connectToHospitalB();

/**
 * Connect to hospital C using UDP
 */
void connectToHospitalC();

/**
 * Get location information from client through TCP
 */
void clientInfo();

/**
 * Get capacity and occupancy information from hospital A
 * through UDP
 */
void HospitalAInfo();

/**
 * Get capacity and occupancy information from hospital B
 * through UDP
 */
void HospitalBInfo();

/**
 * Get capacity and occupancy information from hospital C
 * through UDP
 */
void HospitalCInfo();

/**
 * Send client's location information to hospital A, B,
 * and C through UDP
 */
void forwardClientPos();

/**
 * Get hospital A final score through UDP
 */
void getAScore();

/**
 * Get hospital B final score through UDP
 */
void getBScore();

/**
 * Get hospital C final score through UDP
 */
void getCScore();

/**
 * Get the distance between hospital A and client through UDP
 */
void getADistance();

/**
 * Get the distance between hospital B and client through UDP
 */
void getBDistance();

/**
 * Get the distance between hospital C and client through UDP
 */
void getCDistance();

/**
 * Make a final assignment decision based on the scores
 * and distances from three hospitals
 */
void makeDecision();

/**
 * Compare two doubles and return the larger one
 * @param a with type double
 * @param b with type double
 * @return the larger double between a and b
 */
double larger(double a, double b);

/**
 * Compare three doubles and return the largest one
 * @param a with type double
 * @param b with type double
 * @param c with type double
 * @return the largest double among a and b and c
 */
double largest(double a, double b, double c);

/**
 * Compare two doubles and return the smaller one
 * @param a with type double
 * @param b with type double
 * @return the smaller double between a and b
 */
double lower(double a, double b);

/**
 * Inform client about final assignment result
 * through TCP
 */
void replyClient();

/**
 * Inform all hospitals about final assignment result
 * through UDP
 */
void replyHospital();

/**
 * Main function
 */
int main() {
    createUDPSocket();
    createClientTCPSocket();
    cout << "The Scheduler is up and running." << endl;

    HospitalAInfo();
    HospitalBInfo();
    HospitalCInfo();

    while(true) {
        // Accept listening socket (parent)
        socklen_t clientAddrSize = sizeof(destClientAddr);
        childSockfdClient = ::accept(sockfdClient, (struct sockaddr *) &destClientAddr, &clientAddrSize);
        if (childSockfdClient == ERROR_FLAG) {
            perror("[ERROR] mainserver: fail to accept connection with client");
            exit(1);
        }
        clientInfo();

        // If all hospitals are full, then no assignment
        if(AcapacityValue == AoccupancyValue && BcapacityValue == BoccupancyValue && CcapacityValue == CoccupancyValue){
            DECISION = "None";
            replyClient();
        }
        // Else we forward the location of client to hospitals
        // and wait for an assignment result
        else{
            forwardClientPos();

            getAScore();
            getADistance();
            if(finalScoreA != ERROR_FLAG && finalScoreA != INVALIDCLIENT){
                cout << "The Scheduler has received map information from Hospital A, the score = ​" << finalScoreA << " and the distance = ​" << finalDistanceA << endl;
            }

            getBScore();
            getBDistance();
            if(finalScoreB != ERROR_FLAG && finalScoreB != INVALIDCLIENT){
                cout << "The Scheduler has received map information from Hospital B, the score = ​" << finalScoreB << " and the distance = ​" << finalDistanceB << endl;
            }

            getCScore();
            getCDistance();
            if(finalScoreC != ERROR_FLAG && finalScoreC != INVALIDCLIENT){
                cout << "The Scheduler has received map information from Hospital C, the score = ​" << finalScoreC << " and the distance = ​" << finalDistanceC << endl;
            }

            makeDecision();
            if(DECISION != "None" && DECISION != "INVALID"){
                cout << "The Scheduler has assigned Hospital " << DECISION << "​ to the client" << endl;
            }

            replyClient();
            replyHospital();
        }
    }
    // Close both TCP child socket and UDP socket
    close(sockfdUDP);
    close(childSockfdClient);
    return 0;
}


/**
 * Create TCP socket for client(creat, bind, listen)
 */
void createClientTCPSocket() {
    // From Beej's tutorial

    // Create TCP socket
    sockfdClient = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfdClient == ERROR_FLAG) {
        perror("[ERROR] Main server: fail to create socket for client");
        exit(1);
    }

    // Initialize IP address, port number
    memset(&mainClientAddr, 0, sizeof(mainClientAddr));
    mainClientAddr.sin_family = AF_INET;
    mainClientAddr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    mainClientAddr.sin_port = htons(SCHEDULER_TCP);

    // Bind socket to scheduler with IP address and port number of scheduler
    if (::bind(sockfdClient, (struct sockaddr *) &mainClientAddr, sizeof(mainClientAddr)) == ERROR_FLAG) {
        perror("[ERROR] Main server: fail to bind client socket");
        exit(1);
    }

    // Listen to any incoming connections from client
    if (::listen(sockfdClient, BACKLOG) == ERROR_FLAG) {
        perror("[ERROR] mainserver: fail to listen for client socket");
        exit(1);
    }
}


/**
 * Create UDP socket for hospitals(create, bind)
 */
void createUDPSocket() {
    // From Beej's tutorial

    // Create UDP socket
    sockfdUDP = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfdUDP == ERROR_FLAG) {
        perror("[ERROR] mainserver: fail to create UDP socket");
        exit(1);
    }

    // Initialize IP address, port number
    memset(&mainUDPAddr, 0, sizeof(mainUDPAddr));
    mainUDPAddr.sin_family = AF_INET;
    mainUDPAddr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    mainUDPAddr.sin_port = htons(SCHEDULER_UDP);

    // Bind socket to scheduler with IP address and port number of scheduler
    if (::bind(sockfdUDP, (struct sockaddr *) &mainUDPAddr, sizeof(mainUDPAddr)) == ERROR_FLAG) {
        perror("[ERROR] mainserver: fail to bind UDP socket");
        exit(1);
    }
}

/**
 * Connect to hospital A using UDP
 */
void connectToHospitalA() {
    // From Beej's tutorial

    // Information about hospital A
    memset(&destHospitalAAddr, 0, sizeof(destHospitalAAddr));
    destHospitalAAddr.sin_family = AF_INET;
    destHospitalAAddr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    destHospitalAAddr.sin_port = htons(HOSPITAL_A_PORT);
}

/**
 * Connect to hospital B using UDP
 */
void connectToHospitalB() {
    // From Beej's tutorial

    // Information about hospital B
    memset(&destHospitalBAddr, 0, sizeof(destHospitalBAddr));
    destHospitalBAddr.sin_family = AF_INET;
    destHospitalBAddr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    destHospitalBAddr.sin_port = htons(HOSPITAL_B_PORT);
}

/**
 * Connect to hospital C using UDP
 */
void connectToHospitalC() {
    // From Beej's tutorial

    // Information about hospital C
    memset(&destHospitalCAddr, 0, sizeof(destHospitalCAddr));
    destHospitalCAddr.sin_family = AF_INET;
    destHospitalCAddr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    destHospitalCAddr.sin_port = htons(HOSPITAL_C_PORT);
}

/**
 * Get location information from client through TCP
 */
void clientInfo(){
    // Receive from client
    bzero(clientInput, sizeof(clientInput));
    read(childSockfdClient, clientInput, MAX_DATA_SIZE);
    cout << "The Scheduler has received client at location " << clientInput << "​ from the client using TCP over port " << SCHEDULER_TCP << endl;
}

/**
 * Get capacity and occupancy information from hospital A
 * through UDP
 */
void HospitalAInfo(){
    connectToHospitalA();
    
    // Receive from hospital A
    memset(AInfo, 0, sizeof(AInfo));
    socklen_t hospitalAaddrSize = sizeof(destHospitalAAddr);
    if(::recvfrom(sockfdUDP, AInfo, MAX_DATA_SIZE, 0, (struct sockaddr *) &destHospitalAAddr,
                &hospitalAaddrSize) == ERROR_FLAG){
        perror("Fail to receive information from hospital A");
        exit(1);
    }

    // Split string into tokens
    // Source: https://youtu.be/5laM0Qwzpq8
    Acapacity = strtok(AInfo, " ");
    Aoccupancy = strtok(NULL, " ");

    AcapacityValue = atoi(Acapacity.c_str());
    AoccupancyValue = atoi(Aoccupancy.c_str());
    
	cout << "The Scheduler has received information from Hospital A: total capacity is " << AcapacityValue << " and initial occupancy is ​" << AoccupancyValue << endl;
}

/**
 * Get capacity and occupancy information from hospital B
 * through UDP
 */
void HospitalBInfo(){
    connectToHospitalB();
    
    // Receive from hospital B
    memset(BInfo, 0, sizeof(BInfo));
    socklen_t hospitalBaddrSize = sizeof(destHospitalBAddr);
    if(::recvfrom(sockfdUDP, BInfo, MAX_DATA_SIZE, 0, (struct sockaddr *) &destHospitalBAddr,
                &hospitalBaddrSize) == ERROR_FLAG){
        perror("Fail to receive information from hospital B");
        exit(1);
    }

    // Split string into tokens
    // Source: https://youtu.be/5laM0Qwzpq8
    Bcapacity = strtok(BInfo, " ");
    Boccupancy = strtok(NULL, " ");

    BcapacityValue = atoi(Bcapacity.c_str());
    BoccupancyValue = atoi(Boccupancy.c_str());
    
	cout << "The Scheduler has received information from Hospital B: total capacity is " << BcapacityValue << " and initial occupancy is ​" << BoccupancyValue << endl;
}

/**
 * Get capacity and occupancy information from hospital C
 * through UDP
 */
void HospitalCInfo(){
    connectToHospitalC();
    
    // receive from hospital C
    memset(CInfo, 0, sizeof(CInfo));
    socklen_t hospitalCaddrSize = sizeof(destHospitalCAddr);
    if(::recvfrom(sockfdUDP, CInfo, MAX_DATA_SIZE, 0, (struct sockaddr *) &destHospitalCAddr,
                &hospitalCaddrSize) == ERROR_FLAG){
        perror("Fail to receive information from hospital C");
        exit(1);
    }

    // Split string into tokens
    // Source: https://youtu.be/5laM0Qwzpq8
    Ccapacity = strtok(CInfo, " ");
    Coccupancy = strtok(NULL, " ");

    CcapacityValue = atoi(Ccapacity.c_str());
    CoccupancyValue = atoi(Coccupancy.c_str());
    
	cout << "The Scheduler has received information from Hospital C: total capacity is " << CcapacityValue << " and initial occupancy is ​" << CoccupancyValue << endl;
}

/**
 * Send client's location information to hospital A, B,
 * and C through UDP
 */
void forwardClientPos(){
    // Send the client's location to Hospital A
    // if it has any available space
    connectToHospitalA();
    if(::sendto(sockfdUDP, clientInput, sizeof(clientInput), 0,
        (const struct sockaddr *) &destHospitalAAddr, sizeof(destHospitalAAddr)) == ERROR_FLAG){
        perror("Fail to send the client location to hospital A");
        exit(1);
    }
    cout << "The Scheduler has sent client location to Hospital A using UDP over port " << SCHEDULER_UDP << endl;

    // Send the client's location to Hospital B
    // if it has any available space
    connectToHospitalB();
    if(::sendto(sockfdUDP, clientInput, sizeof(clientInput), 0,
        (const struct sockaddr *) &destHospitalBAddr, sizeof(destHospitalBAddr)) == ERROR_FLAG){
        perror("Fail to send the client location to hospital B");
        exit(1);
    }
    cout << "The Scheduler has sent client location to Hospital B using UDP over port " << SCHEDULER_UDP << endl;

    // Send the client's location to Hospital C
    // if it has any available space
    connectToHospitalC();
    if(::sendto(sockfdUDP, clientInput, sizeof(clientInput), 0,
        (const struct sockaddr *) &destHospitalCAddr, sizeof(destHospitalCAddr)) == ERROR_FLAG){
        perror("Fail to send the client location to hospital C");
        exit(1);
    }
    cout << "The Scheduler has sent client location to Hospital C using UDP over port " << SCHEDULER_UDP << endl;
}

/**
 * Get hospital A final score through UDP
 */
void getAScore(){
    connectToHospitalA();
    
    // Receive final score from hospital A
    memset(scoreA, 0, sizeof(scoreA));
    socklen_t hospitalAaddrSize = sizeof(destHospitalAAddr);
    if(::recvfrom(sockfdUDP, scoreA, MAX_DATA_SIZE, 0, (struct sockaddr *) &destHospitalAAddr,
                &hospitalAaddrSize) == ERROR_FLAG){
        perror("Fail to receive score from hospital A");
        exit(1);
    }

    finalScoreA = atof(scoreA);
}

/**
 * Get hospital B final score through UDP
 */
void getBScore(){
    connectToHospitalB();
    
    // Receive final score from hospital B
    memset(scoreB, 0, sizeof(scoreB));
    socklen_t hospitalBaddrSize = sizeof(destHospitalBAddr);
    if(::recvfrom(sockfdUDP, scoreB, MAX_DATA_SIZE, 0, (struct sockaddr *) &destHospitalBAddr,
                &hospitalBaddrSize) == ERROR_FLAG){
        perror("Fail to receive score from hospital B");
        exit(1);
    }

    finalScoreB = atof(scoreB);
}

/**
 * Get hospital C final score through UDP
 */
void getCScore(){
    connectToHospitalC();
    
    // Receive final score from hospital C
    memset(scoreC, 0, sizeof(scoreC));
    socklen_t hospitalCaddrSize = sizeof(destHospitalCAddr);
    if(::recvfrom(sockfdUDP, scoreC, MAX_DATA_SIZE, 0, (struct sockaddr *) &destHospitalCAddr,
                &hospitalCaddrSize) == ERROR_FLAG){
        perror("Fail to receive score from hospital C");
        exit(1);
    }

    finalScoreC = atof(scoreC);
}

/**
 * Get the distance between hospital A and client through UDP
 */
void getADistance(){
    // Receive shortest distance from hospital A
    memset(distanceA, 0, sizeof(distanceA));
    socklen_t hospitalAaddrSize = sizeof(destHospitalAAddr);
    if(::recvfrom(sockfdUDP, distanceA, MAX_DATA_SIZE, 0, (struct sockaddr *) &destHospitalAAddr,
                &hospitalAaddrSize) == ERROR_FLAG){
        perror("Fail to receive distance from hospital A");
        exit(1);
    }

    finalDistanceA = atof(distanceA);
}

/**
 * Get the distance between hospital B and client through UDP
 */
void getBDistance(){
    // Receive shortest distance from hospital B
    memset(distanceB, 0, sizeof(distanceB));
    socklen_t hospitalBaddrSize = sizeof(destHospitalBAddr);
    if(::recvfrom(sockfdUDP, distanceB, MAX_DATA_SIZE, 0, (struct sockaddr *) &destHospitalBAddr,
                &hospitalBaddrSize) == ERROR_FLAG){
        perror("Fail to receive distance from hospital B");
        exit(1);
    }

    finalDistanceB = atof(distanceB);
}

/**
 * Get the distance between hospital C and client through UDP
 */
void getCDistance(){
    // Receive shortest distance from hospital C
    memset(distanceC, 0, sizeof(distanceC));
    socklen_t hospitalCaddrSize = sizeof(destHospitalCAddr);
    if(::recvfrom(sockfdUDP, distanceC, MAX_DATA_SIZE, 0, (struct sockaddr *) &destHospitalCAddr,
                &hospitalCaddrSize) == ERROR_FLAG){
        perror("Fail to receive distance from hospital C");
        exit(1);
    }

    finalDistanceC = atof(distanceC);
}

/**
 * Make a final assignment decision based on the scores
 * and distances from three hospitals
 */
void makeDecision(){
    // Illegal hospital location information
    if(finalDistanceA == ERROR_FLAG || finalDistanceB == ERROR_FLAG || finalDistanceC == ERROR_FLAG){
        DECISION = "None";
    }
    // Illegal client location information
    else if(finalDistanceA == INVALIDCLIENT || finalDistanceB == INVALIDCLIENT || finalDistanceC == INVALIDCLIENT){
        DECISION = "INVALID";
    }
    // Location are legal
    else{
        // All scores are None
        if(finalScoreA == ERROR_FLAG && finalScoreB == ERROR_FLAG && finalScoreC == ERROR_FLAG){
            DECISION = "None";
        }
        // Any hospital with Score == None will not participate in comparison
        else if(finalScoreA == ERROR_FLAG || finalScoreB == ERROR_FLAG || finalScoreC == ERROR_FLAG){
            if(finalScoreA == ERROR_FLAG){
                if(finalScoreB == finalScoreC){
                    decision = lower(finalDistanceB, finalDistanceC);
                    if(decision == finalDistanceB){
                        DECISION = "B";
                    } else{
                        DECISION = "C";
                    }
                } else{
                    decision = larger(finalScoreB, finalScoreC);
                    if(decision == finalScoreB){
                        DECISION = "B";
                    } else{
                        DECISION = "C";
                    }
                }
            } else if(finalScoreB == ERROR_FLAG){
                if(finalScoreA == finalScoreC){
                    decision = lower(finalDistanceA, finalDistanceC);
                    if(decision == finalDistanceA){
                        DECISION = "A";
                    } else{
                        DECISION = "C";
                    }
                } else{
                    decision = larger(finalScoreA, finalScoreC);
                    if(decision == finalScoreA){
                        DECISION = "A";
                    } else{
                        DECISION = "C";
                    }
                }
            } else if(finalScoreC == ERROR_FLAG){
                if(finalScoreB == finalScoreA){
                    decision = lower(finalDistanceB, finalDistanceA);
                    if(decision == finalDistanceB){
                        DECISION = "B";
                    } else{
                        DECISION = "A";
                    }
                } else{
                    decision = larger(finalScoreB, finalScoreA);
                    if(decision == finalScoreB){
                        DECISION = "B";
                    } else{
                        DECISION = "A";
                    }
                }
            }
        }
        else{
            // Score A/B/C are not all the same, reply the hospital with the highest score
            if(finalScoreA != finalScoreB && finalScoreB != finalScoreC && finalScoreA != finalScoreC){
                decision = largest(finalScoreA, finalScoreB, finalScoreC);
                if(decision == finalScoreA){
                    DECISION = "A";
                } else if(decision == finalScoreB){
                    DECISION = "B";
                } else if(decision == finalScoreC){
                    DECISION = "C";
                }
            }
            // Tie for highest scores, reply the hospital with the shortest distance
            else{
                if(finalScoreA == finalScoreB && finalScoreA > finalScoreC){
                    decision = lower(finalDistanceA, finalDistanceB);
                    if(decision == finalDistanceA){
                        DECISION = "A";
                    } else if(decision == finalDistanceB){
                        DECISION = "B";
                    }
                }
                else if(finalScoreB == finalScoreC && finalScoreB > finalScoreA){
                    decision = lower(finalDistanceB, finalDistanceC);
                    if(decision == finalDistanceB){
                        DECISION = "B";
                    } else if(decision == finalDistanceC){
                        DECISION = "C";
                    }
                }
                else if(finalScoreA == finalScoreC && finalScoreA > finalScoreB){
                    decision = lower(finalDistanceA, finalDistanceC);
                    if(decision == finalDistanceA){
                        DECISION = "A";
                    } else if(decision == finalDistanceC){
                        DECISION = "C";
                    }
                }
                else if(finalScoreA == finalScoreB && finalScoreA < finalScoreC){
                    decision = finalScoreC;
                    DECISION = "C";
                }
                else if(finalScoreB == finalScoreC && finalScoreB < finalScoreA){
                    decision = finalScoreA;
                    DECISION = "A";
                }
                else if(finalScoreA == finalScoreC && finalScoreA < finalScoreB){
                    decision = finalScoreB;
                    DECISION = "B";
                }
            }
        }
    }
}

/**
 * Compare two doubles and return the larger one
 * @param a with type double
 * @param b with type double
 * @return the larger double between a and b
 */
double larger(double a, double b){
    double max = a;
    if(b > max) max = b;
    return max;
}

/**
 * Compare three doubles and return the largest one
 * @param a with type double
 * @param b with type double
 * @param c with type double
 * @return the largest double among a and b and c
 */
double largest(double a, double b, double c){
    double max = a;
    if(b > max) max = b;
    if(c > max) max = c;
    return max;
}

/**
 * Compare two doubles and return the smaller one
 * @param a with type double
 * @param b with type double
 * @return the smaller double between a and b
 */
double lower(double a, double b){
    double min = a;
    if(b < min) min = b;
    return min;
}

/**
 * Inform client about final assignment result
 * through TCP
 */
void replyClient(){
    // From Beej's tutorial
    strncpy(assignResult, DECISION.c_str(), sizeof(DECISION));

    if(::send(childSockfdClient, assignResult, sizeof(assignResult), 0) == ERROR_FLAG){
        perror("Fail to reply client about hospital assignment");
        exit(1);
    }
    cout << "The Scheduler has sent the result to client using TCP over port " << SCHEDULER_TCP << endl;
}

/**
 * Inform all hospitals about final assignment result
 * through UDP
 */
void replyHospital(){
    // Inform hospital A about the assignment result
    if(::sendto(sockfdUDP, assignResult, sizeof(assignResult), 0,
            (const struct sockaddr *) &destHospitalAAddr, sizeof(destHospitalAAddr)) == ERROR_FLAG){
            perror("Fail to send the assignment result to hospital A");
            exit(1);
            }

    // Inform hospital B about the assignment result
    if(::sendto(sockfdUDP, assignResult, sizeof(assignResult), 0,
            (const struct sockaddr *) &destHospitalBAddr, sizeof(destHospitalBAddr)) == ERROR_FLAG){
            perror("Fail to send the assignment result to hospital B");
            exit(1);
            }

    // Inform hospital C about the assignment result
    if(::sendto(sockfdUDP, assignResult, sizeof(assignResult), 0,
            (const struct sockaddr *) &destHospitalCAddr, sizeof(destHospitalCAddr)) == ERROR_FLAG){
            perror("Fail to send the assignment result to hospital C");
            exit(1);
            }

    if(DECISION == "A"){
        cout << "The Scheduler has sent the result to Hospital " << assignResult << " using UDP over port " << SCHEDULER_UDP << endl;
        AoccupancyValue++;
    } else if(DECISION == "B"){
        cout << "The Scheduler has sent the result to Hospital " << assignResult << " using UDP over port " << SCHEDULER_UDP << endl;
        BoccupancyValue++;
    } else if(DECISION == "C"){
        cout << "The Scheduler has sent the result to Hospital " << assignResult << " using UDP over port " << SCHEDULER_UDP << endl;
        CoccupancyValue++;
    }
}