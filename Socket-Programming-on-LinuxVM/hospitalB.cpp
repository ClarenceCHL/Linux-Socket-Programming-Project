/***
 ** Haolun Cheng
 **/

#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <map>
#include <queue>
#include <fstream>
#include <vector>
#include <iomanip>
#include <sstream>
#define HOSPITAL_B_PORT 31827
#define SCHEDULER_UDP 33827
#define LOCAL_HOST "127.0.0.1"
#define ERROR_FLAG -1
#define INVALIDCLIENT -2
#define MAXLINE 1024
using namespace std;

// Create an edge structure to store the edge of map
struct Edge {
    Edge(string a, string b, double w) {
        from = a;
        to = b;
        weight = w;
    }

    string from;
    string to;
    double weight;
};

// Edge sets
vector <Edge> edges;

map<string, int> reindex;

// Location name
map<int, string> name;

// Use adjacency list to represent the map
vector < vector<Edge> > Graph;

vector <string> path;
double shortestDist;

/**
 * Read the map.txt file
 * @param filename a file to be read
 */
void readIntoGraph(string filename);

/**
 * Construct graph using adjacency matrix
 */
void constructGraph();

/**
 * Find the shortest path from the start node to the end node
 * @param start name of the start node
 * @param end name of the end node
 * @return shortest distance from start to end
 */
double Dijkstra(string start, string end);

int sockfdUDP; // Hospital UDP socket
string clientLocation; //Client location
struct sockaddr_in serverAddrUDP, clientAddrUDP; // Hospital and scheduler address for UDP connection
char recvBuf[MAXLINE]; // Store the location of client
string locationHos, capacity, occupancy; // Store the location, capacity, and occupancy of hospital
char sendInfo[MAXLINE], sendScore[MAXLINE], sendDistance[MAXLINE], getResult[MAXLINE]; // Communication variables in UDP socket
float capacityN, occupancyN;
float availability; // Availability of hospital
double finalScore; // Final score of hospital

/**
 * Connect with scheduler through UDP
 */
void connectWithScheduler();

/**
 * Create a UDP socket
 */
void createUDPSocket();

/**
 * Get the initial capacity and occupancy values of hospital
 * through user's input
 */
void getUserInput();

/**
 * Send the capacity and occupancy values to scheduler
 * through UDP
 */
void sendUserInput();

/**
 * Get the client location from scheduler through UDP
 */
void getClientPos();

/**
 * Get the availability of hospital
 */
void getAvailabilityHosB();

/**
 * Get the final score of hospital
 */
void getScore();

/**
 * Send the hospital's final score to scheduler through UDP
 */
void sendFinalScore();

/**
 * Send the shortest distance between hospital and client
 * to scheduler through UDP
 */
void sendFinalDistance();

/**
 * Get the final assignment result from scheduler through UDP
 */
void recvResult();

/**
 * Main function
 */
int main(int argc, char*argv[]){
    // Get user's inputs
    for(int i = 1; i < argc; ++i){
        if(i == 1) locationHos = argv[i];
        else if(i == 2) capacity = argv[i];
        else occupancy = argv[i];
    }

    // Open map.txt text file
    string filename = "map.txt";

    readIntoGraph(filename);
    constructGraph();


    createUDPSocket();
    getUserInput();
    sendUserInput();

    while(true){
        getClientPos();
        getAvailabilityHosB();

        // Case 1: occupancy < capacity
        if(availability != ERROR_FLAG){
            // Client or hospital not in the map
            if(reindex.count(clientLocation) == 0 || reindex.count(locationHos) == 0){
                if(reindex.count(clientLocation) == 0){
                    shortestDist = INVALIDCLIENT;
                    cout << "Hospital B does not have the location ​" << clientLocation << " in map" << endl;
                } else if(reindex.count(locationHos) == 0){
                    shortestDist = ERROR_FLAG;
                    cout << "Hospital B with location " << locationHos << " is out of map" << endl;
                }
            }
            // Client and hospital have the same location
            else if(clientLocation == locationHos){
                shortestDist = ERROR_FLAG;
            } 
            else{
                shortestDist = Dijkstra(clientLocation, locationHos);
                cout << "Hospital B has found the shortest path to client, distance = "  << shortestDist << endl;
            }

            getScore();
        } 
        // Case 2: occupancy >= capacity
        else{
            getScore();
        }


        sendFinalScore();
        sendFinalDistance();

        if(finalScore != ERROR_FLAG && shortestDist != ERROR_FLAG && shortestDist != INVALIDCLIENT){
            cout << "Hospital B has sent score = ​" << finalScore << " and distance = " << shortestDist << " to the Scheduler" << endl;
        }

        recvResult();
    }
    
    // Close the socket
    close(sockfdUDP);

    return 0;
}

/**
 * Connect with scheduler through UDP
 */
void connectWithScheduler(){
    // From Beej's tutorial
    // Information about Scheduler
    memset(&clientAddrUDP, 0, sizeof(clientAddrUDP));
    clientAddrUDP.sin_family = AF_INET;
    clientAddrUDP.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    clientAddrUDP.sin_port = htons(SCHEDULER_UDP);
}

/**
 * Create a UDP socket
 */
void createUDPSocket(){
    // From Beej's tutorial
    sockfdUDP = socket(AF_INET, SOCK_DGRAM, 0);
    // Test if create a socket successfully
    if(sockfdUDP == ERROR_FLAG){
        perror("Hospital B UDP socket");
        exit(1);
    }
    // Initialize IP address, port number
    memset(&serverAddrUDP, 0, sizeof(serverAddrUDP));
    serverAddrUDP.sin_family = AF_INET;
    serverAddrUDP.sin_port   = htons(HOSPITAL_B_PORT);
    serverAddrUDP.sin_addr.s_addr = inet_addr(LOCAL_HOST);

    // Bind socket
    if(::bind(sockfdUDP, (struct sockaddr*) &serverAddrUDP, sizeof(serverAddrUDP)) == ERROR_FLAG){
        perror("HospitalB UDP bind");
        exit(1);
    }
    cout << "Hospital B is up and running using UDP on port "<< HOSPITAL_B_PORT << endl;
}

/**
 * Get the initial capacity and occupancy values of hospital
 * through user's input
 */
void getUserInput(){
    string informScheduler = capacity + " " + occupancy;
    strncpy(sendInfo, informScheduler.c_str(), sizeof(sendInfo));
    cout << "Hospital B has total capacity " << capacity << " and initial occupancy " << occupancy << endl;

    capacityN = atof(capacity.c_str());
    occupancyN = atof(occupancy.c_str());
}

/**
 * Send the capacity and occupancy values to scheduler
 * through UDP
 */
void sendUserInput(){
    connectWithScheduler();
    // From Beej's guide
    if(::sendto(sockfdUDP, sendInfo, sizeof(sendInfo), 0, 
        (const struct sockaddr *) &clientAddrUDP, sizeof(clientAddrUDP)) == ERROR_FLAG){
            perror("Fail to send hospital B information");
            exit(1);
        }
}

/**
 * Get the client location from scheduler through UDP
 */
void getClientPos(){
    // From Beej's guide
    socklen_t len = sizeof(clientAddrUDP);
    memset(recvBuf, 0, sizeof(recvBuf));
    if(::recvfrom(sockfdUDP, recvBuf, sizeof(recvBuf), 0,
            (struct sockaddr *) &clientAddrUDP, &len) == ERROR_FLAG){
                perror("Fail to receive client position");
                exit(1);
            }
    
    clientLocation = recvBuf;
    cout << "Hospital B has received input from client at location " << clientLocation << endl;
}

/**
 * Get the availability of hospital
 */
void getAvailabilityHosB(){
    availability = (capacityN - occupancyN) / capacityN;

    if(availability <= 0 || availability > 1){
        availability = ERROR_FLAG;
        cout << "Hospital B has capacity = " << capacityN << ", occupation = " << occupancyN << ", availability = None" << endl;
    } else{
        cout << "Hospital B has capacity = " << capacityN << ", occupation = " << occupancyN << ", availability = " << availability << endl;
    }
}

/**
 * Read the map.txt file
 * @param filename a file to be read
 */
void readIntoGraph(string filename) {
    ifstream in(filename.c_str());
    string from, to;
    double weight;
    int id = 0;
    while (in >> from >> to >> weight) {
        // Reindex vertex
        if (reindex.count(from) == 0) {
            reindex[from] = id;
            name[id++] = from;
        }
        if (reindex.count(to) == 0) {
            reindex[to] = id;
            name[id++] = to;
        }
        // Add this edge to edge set
        edges.push_back(Edge(from, to, weight));
    }
}

/**
 * Construct graph using adjacency matrix
 */
void constructGraph() {
    Graph.resize(reindex.size());
    bool flag;
    for (int i = 0; i < edges.size(); i++) {
        Edge e = edges[i];
        flag = false;
        // Exclude double edge
        for (int j = 0; j < Graph[reindex[e.from]].size(); j++) {
            Edge e1 = Graph[reindex[e.from]][j];
            if (e1.to == e.to) {
                flag = true;
                break;
            }
        }
        if (!flag) {
            // Add an edge to Graph index[e.from]
            Graph[reindex[e.from]].push_back(e);
            // Add an edge to Graph index[e.to]
            Graph[reindex[e.to]].push_back(Edge(e.to, e.from, e.weight));
        }
    }
}

/**
 * Find the shortest path from the start node to the end node
 * @param start name of the start node
 * @param end name of the end node
 * @return shortest distance from start to end
 */
double Dijkstra(string start, string end) {
    if (reindex.count(start) == 0)return -1;
    if (reindex.count(end) == 0)return -1;
    vector<int> pre(Graph.size());
    vector<double> dis(Graph.size());
    for (int i = 0; i < dis.size(); i++) {
        pre[i] = -1;
        dis[i] = -1;
    }

    priority_queue < pair < double, int >, vector < pair < double, int > >, greater < pair < double, int > > > pq;
    pq.push(make_pair(0, reindex[start]));
    dis[reindex[start]] = 0;
    int endindex = reindex[end];
    while (!pq.empty()) {
        if (pq.top().second == endindex) {
            break;
        }
        double weight = pq.top().first;
        int x = pq.top().second;
        pq.pop();
        for (int i = 0; i < Graph[x].size(); i++) {
            Edge e = Graph[x][i];
            int y = reindex[e.to];
            if (dis[y] == -1 || dis[y] > e.weight + weight) {
                pre[y] = x;
                dis[y] = e.weight + weight;
                pq.push(make_pair(e.weight + weight, y));
            }
        }
    }
    vector<int> pathIndex;

    int x = reindex[end];
    while (x != -1) {
        pathIndex.push_back(x);
        x = pre[x];
    }
    for (int i = pathIndex.size() - 1; i >= 0; i--) {
        path.push_back(name[pathIndex[i]]);
    }
    return dis[reindex[end]];

}

/**
 * Get the final score of hospital
 */
void getScore(){
    if(availability == ERROR_FLAG || shortestDist == ERROR_FLAG){
        finalScore = ERROR_FLAG;
        cout << "Hospital B has the score = None" << endl;
    } 
    else if(shortestDist == INVALIDCLIENT){
        finalScore = INVALIDCLIENT;
    }
    else{
        finalScore = 1 / (shortestDist * (1.1 - availability));
        cout << "Hospital B has the score = " << finalScore << endl;
    }
}

/**
 * Send the hospital's final score to scheduler through UDP
 */
void sendFinalScore(){
    ostringstream FS;
    FS << finalScore;

    string FINALSCORE = FS.str();
    strncpy(sendScore, FINALSCORE.c_str(), sizeof(FINALSCORE));

    // From Beej's guide
    if(::sendto(sockfdUDP, sendScore, sizeof(sendScore), 0, 
        (const struct sockaddr *) &clientAddrUDP, sizeof(clientAddrUDP)) == ERROR_FLAG){
            perror("Fail to send hospital B score");
            exit(1);
        }
}

/**
 * Send the shortest distance between hospital and client
 * to scheduler through UDP
 */
void sendFinalDistance(){
    ostringstream FD;
    FD << shortestDist;

    string FINALDISTANCE = FD.str();
    strncpy(sendDistance, FINALDISTANCE.c_str(), sizeof(FINALDISTANCE));

    // From Beej's guide
    if(::sendto(sockfdUDP, sendDistance, sizeof(sendDistance), 0, 
        (const struct sockaddr *) &clientAddrUDP, sizeof(clientAddrUDP)) == ERROR_FLAG){
            perror("Fail to send the shortest distance from client to hospital B");
            exit(1);
        }
    
    if(shortestDist == ERROR_FLAG || shortestDist == INVALIDCLIENT){
        cout << "Hospital B has sent 'location not found' to the Scheduler" << endl;
    }
}

/**
 * Get the final assignment result from scheduler through UDP
 */
void recvResult(){
    // From Beej's guide
    string GETRESULT;
    socklen_t len = sizeof(clientAddrUDP);
    memset(getResult, 0, sizeof(getResult));
    if(::recvfrom(sockfdUDP, getResult, sizeof(getResult), 0,
            (struct sockaddr *) &clientAddrUDP, &len) == ERROR_FLAG){
                perror("Fail to receive assignment result");
                exit(1);
            }

    GETRESULT = getResult;

    // If hospital B is assigned, then update occupation and availability
    if(GETRESULT == "B"){
        occupancyN = occupancyN + 1;
        availability = (capacityN - occupancyN) / capacityN;
        cout << "Hospital B has been assigned to a client, occupation is updated to ​" << occupancyN << ", availability is updated to ​" << availability << endl;
    }
}