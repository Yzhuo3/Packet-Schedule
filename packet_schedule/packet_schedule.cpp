#include <iostream>
#include <vector>
#include <queue>
#include <random>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <limits>
#include <functional>
#include <map>
#include <memory>
#include "include/Event.hpp"
#include "include/Packet.hpp"
#include "include/Router.hpp"
#include "service/PriorityQueue.hpp"

using namespace std;

// ============================ GLOBAL SIMULATION VARIABLES ============================
// Simulation Clock
double currentTime = 0.0;

// Event queue (Min-Heap based on event time)
priority_queue<Event, vector<Event>, greater<Event>> eventQueue;

// Random Number Generator
random_device rd;
mt19937 gen(rd());
uniform_real_distribution<> arrivalDist(0.1, 1.0);  // Packet arrival time distribution

// Simulation Parameters
const double TRANSMISSION_RATE = 10e6; // 10 Mbps transmission capacity
size_t NUM_NODES = 5;
size_t QUEUE_CAPACITY = 100;

// Create Routers
vector<shared_ptr<Router>> routers;

// Statistics Tracking
map<int, double> totalPacketDelay;
map<int, int> packetCount;
map<int, double> blockingRatio;
map<int, int> droppedPackets;
map<int, int> totalPacketsGenerated;

// Reference Packet Tracking
bool referencePacketDelivered = false;
double referencePacketCreationTime = 0.0;
double referencePacketEndTime = 0.0;

// ============================ FUNCTION TO SCHEDULE EVENTS ============================
void scheduleEvent(double time, EventType type, function<void()> handler) {
    eventQueue.push(Event(time, type, move(handler)));
}

// ============================ PACKET ARRIVAL HANDLER ============================
void handlePacketArrival(shared_ptr<Router> router, Packet packet, int currentNode) {
    if (packet.isReference && referencePacketCreationTime == 0) {
        referencePacketCreationTime = currentTime;
    }

    cout << "[Time " << currentTime << "] Packet Type " << static_cast<int>(packet.type)
         << " created at " << packet.creationTime
         << " arrived at Router " << router->getId() << " (Node " << currentNode << ")" << endl;
    
    totalPacketsGenerated[currentNode]++;

    bool wasQueued = router->enqueuePacket(packet, currentTime);  // ✅ Pass currentTime
    if (!wasQueued) {
        droppedPackets[currentNode]++;
    }

    // Schedule processing of the packet at the router
    scheduleEvent(currentTime + 0.01, EventType::PACKET_DEPARTURE, [router, packet, currentNode]() {
        router->processPacket(scheduleEvent, currentTime);

        // Compute packet delay
        double delay = currentTime - packet.arrivalTime;
        totalPacketDelay[currentNode] += delay;
        packetCount[currentNode]++;

        // Check if the reference packet reached the last node
        if (currentNode < NUM_NODES - 1) {
            int nextNode = currentNode + 1;
            auto nextRouter = routers[nextNode];

            // Schedule reference packet for the next node
            double transferTime = packet.size / TRANSMISSION_RATE;
            scheduleEvent(currentTime + transferTime, EventType::PACKET_ARRIVAL, [nextRouter, packet, nextNode]() {
                handlePacketArrival(nextRouter, packet, nextNode);
            });
        } else {
            cout << "[Time " << currentTime << "] Reference Packet created at " << packet.creationTime
                 << " has reached the destination (Node " << currentNode << "). Simulation ends." << endl;
            referencePacketDelivered = true;
            referencePacketEndTime = currentTime;
        }
    });
}

// ============================ SIMULATION EXECUTION LOOP ============================
void runSimulation() {
    while (!eventQueue.empty()) {
        Event event = eventQueue.top();
        eventQueue.pop();
        currentTime = event.time;
        event.execute();

        // Stop simulation when the reference packet reaches its destination
        if (referencePacketDelivered) break;
    }
}

// ============================ DISPLAY SIMULATION STATISTICS ============================
void displayStatistics() {
    cout << "\n========= Simulation Statistics =========\n";
    for (int i = 0; i < NUM_NODES; i++) {
        double avgDelay = packetCount[i] > 0 ? totalPacketDelay[i] / packetCount[i] : 0.0;
        double dropRate = totalPacketsGenerated[i] > 0 ? (double)droppedPackets[i] / totalPacketsGenerated[i] : 0.0;

        cout << "Node " << i << " Statistics:\n";
        cout << "  - Average Packet Delay: " << avgDelay << " seconds\n";
        cout << "  - Packet Blocking Ratio: " << dropRate * 100 << "%\n";
        cout << "  - Total Packets Processed: " << packetCount[i] << "\n";
        cout << "  - Dropped Packets: " << droppedPackets[i] << "\n";
        cout << "------------------------------------\n";
    }

    // End-to-End Reference Packet Delay
    if (referencePacketDelivered) {
        cout << "Reference Packet End-to-End Delay: " 
             << (referencePacketEndTime - referencePacketCreationTime) << " seconds\n";
    } else {
        cout << "Reference Packet did not reach the final node.\n";
    }
}

// ============================ MAIN FUNCTION ============================
int main() {
    cout << "Starting Packet Scheduling Simulation..." << endl;

    // Configure Network Parameters
    cout << "Enter Number of Nodes (M): ";
    cin >> NUM_NODES;
    cout << "Enter Queue Capacity per Node (K): ";
    cin >> QUEUE_CAPACITY;

    routers.reserve(NUM_NODES);
    for (size_t i = 0; i < NUM_NODES; i++) {
        routers.push_back(make_shared<Router>(i, TRANSMISSION_RATE, QUEUE_CAPACITY));
    }

    // Schedule the reference packet's first arrival at Node 0
    double initialTime = arrivalDist(gen);
    Packet referencePacket(PacketType::REFERENCE, initialTime, initialTime, 100000000, 0, true);

    scheduleEvent(initialTime, EventType::PACKET_ARRIVAL, [referencePacket]() {
        handlePacketArrival(routers[0], referencePacket, 0);
    });

    // Run simulation
    runSimulation();

    // Display final statistics
    displayStatistics();

    cout << "Simulation Completed!" << endl;
    return 0;
}
