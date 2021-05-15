### FULL NAME: Haolun Cheng

### What I have done in the project:
The key idea of this project is to assign a client to the closest hospital nearby. To implement this functionality, there are three important parts: Client, Scheduler, and Hospitals. Let me explain what each of them does by breaking down the three parts in details.

And the following is also what I have implemented in the project:

---Phase 1---Boot-up
	•	Hospital A, B, and C read the file map.txt and construct a graph. The three hospitals also initialize their location, capacity, and occupancy by getting user’s three inputs. Hospitals inform Scheduler about their initial capacity and initial occupancy.
	•	Scheduler creates and binds TCP and UDP sockets. TCP socket is used to listen for any incoming connections from client side, and UDP socket is used to communicate with three hospitals.
	•	After client side receives the location information from user input and sends it to Scheduler via TCP, the TCP parent socket on the Scheduler side will accept the incoming connection and create a TCP child socket for further communications. After Scheduler receives the client’s location, the Scheduler forwards the location to available hospitals.

---Phase 2---Forward
	•	The client sends its query to the Scheduler and waiting for an assignment result.
	•	Scheduler decodes the query and decide which hospital(s) should be informed about the client location information.

---Phase 3---Scoring
	•	After Hospital A, B, and C receive the client location, they will perform a series of calculations based on their availability and location. Each of the hospital uses Dijkstra algorithm to calculate the shortest distance from the client. Lastly, each hospital calculates a final score based on availability and shortest distance.
	•	Hospital A, B, and C send their final scores and shortest distances to the Scheduler using UDP.
	•	Scheduler decodes the message and compares all three scores and distances and decides which hospital the client should be assigned to.

---Phase 4---Reply
	•	Scheduler decides which hospital the client should be assigned to and sends the assignment result to both client and the chosen hospital.
	•	The chosen hospital updates its occupancy and availability.
	•	Client receives the decision from Scheduler and then terminates itself.

### Code files and functionalities:
client.cpp:
    Communicate with Scheduler through TCP
	•	Receive the client location information through user input
	•	Send the query to Scheduler through TCP socket
	•	Get the assignment result from Scheduler and print it out

scheduler.cpp:
    Communicate with Client through TCP
    Communicate with Hospitals through UDP
    •	Get initial capacity and initial occupancy from each hospital
	•	Get the Client location and forward it to Hospitals
	•	Get the score and shortest distance result from each Hospital
	•	Calculate and decide which Hospital the Client should be assigned to
	•	Inform Client and the chosen Hospital about the assignment decision

hospitalA.cpp:
    Communicate with Scheduler through UDP
	•	Read the map.txt file and construct a graph using adjacency matrix
	•	Get location, capacity, and occupancy information of itself from user inputs
    •	Send the initial capacity and initial occupancy to Scheduler
	•	Get the Client location information from Scheduler
	•	Calculate availability based on capacity and occupancy. Calculate the shortest distance from the Client location to itself by using Dijkstra algorithm
	•	Calculate a final score based on availability and shortest distance
	•	Send the score and shortest distance information to Scheduler
	•	If assigned to a Client by the Scheduler, update occupancy and availability

hospitalB.cpp:
    Communicate with Scheduler through UDP
    HospitalB.cpp has exactly the same functionality as HospitalA.cpp

hospitalC.cpp:
    Communicate with Scheduler through UDP
    HospitalC.cpp has exactly the same functionality as HospitalA.cpp

### Format of all the messages exchanged:
Messages exchanged in each phase:
---Phase 1---Boot-up
	•	Hospital A, B and C send their capacity and occupancy information to Scheduler through UDP in the format <capacity | occupancy>
	•	Client sends its location to Scheduler <vertex index>

---Phase 2---Forward
	•	Scheduler sends the Client location to available Hospitals <vertex index>

---Phase 3---Scoring
	•	Hospital A, B, and C send their final scores and shortest distances to Scheduler in the format <score | distance>

---Phase 4---Reply
	•	Scheduler decides the assignment result and informs the selected hospital <Decision>
	•	Scheduler decides the assignment result and informs Client <Decision>

### Idiosyncrasy of my project:
My project runs perfectly under any condition no matter how tricky the condition is, and I am confident about it. 
My project is able to capture all possible edge cases, including:
	•	Hospital’s initial capacity < initial occupancy
	•	Keep adding Client until all hospitals are fully occupied
	•	Client and Hospital have the same location
	•	Client is out of map
	•	Hospital is out of map

### Reused code:
	•	The implementation of TCP and UDP sockets (socket(), connect(), bind(), sendto(), recvfrom()…) refers to the “Beej’s Guide to Network Programming” tutorial.
	•	The use of strtok() function to tokenize strings refers to a YouTube video https://youtu.be/5laM0Qwzpq8 
