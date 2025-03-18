#include <iostream>
#include <queue>
#include <vector>
#include <functional>
#include <memory>
#include <random>
#include "include/Event.hpp"
#include "include/Packet.hpp"
#include "include/Router.hpp"
#include "service/IQueue.hpp"
#include "service/SPQueue.hpp"

using namespace std;

// Global simulation clock
double currentTime = 0.0;

// Event queue (Min-Heap based on event time)
priority_queue<Event, vector<Event>, greater<Event>> eventQueue;

// Random number generator for ON/OFF traffic modeling
random_device rd;
mt19937 gen(rd());
uniform_real_distribution<> arrivalDist(0.1, 1.0);  // Packet arrival time distribution

// Simulation parameters
const double SIMULATION_TIME = 10.0;  // Total simulation time
const double TRANSMISSION_RATE = 10e6; // 10 Mbps transmission capacity
const int NUM_NODES = 5;
const size_t QUEUE_CAPACITY = 100;

// Create Routers
vector<shared_ptr<Router>> routers;

// Function to schedule an event in the simulation
void scheduleEvent(double time, EventType type, function<void()> handler) {
    eventQueue.push(Event(time, type, move(handler)));
}

// Function to handle packet arrivals
void handlePacketArrival(shared_ptr<Router> router, Packet packet) {
    cout << "[Time " << currentTime << "] Packet ID " << packet.id
         << " arrived at Router " << router->getId() << endl;

    router->enqueuePacket(packet);

    // Schedule the next packet arrival for this router
    double nextArrivalTime = currentTime + arrivalDist(gen);
    if (nextArrivalTime < SIMULATION_TIME) {
        Packet newPacket(packet.id + 1, PacketType::AUDIO, nextArrivalTime, 1000, 0.01);
        scheduleEvent(nextArrivalTime, EventType::PACKET_ARRIVAL, [router, newPacket]() {
            handlePacketArrival(router, newPacket);
        });
    }

    // Schedule packet processing at the router
    scheduleEvent(currentTime + 0.01, EventType::PACKET_DEPARTURE, [router]() {
        router->processPacket(scheduleEvent, currentTime);
    });
}

// Function to run the simulation
void runSimulation() {
    while (!eventQueue.empty() && currentTime < SIMULATION_TIME) {
        Event event = eventQueue.top();
        eventQueue.pop();
        currentTime = event.time;
        event.execute();
    }
}

int main() {
    cout << "Starting Packet Scheduling Simulation..." << endl;

    // Initialize routers with SPQ queues
    for (int i = 0; i < NUM_NODES; i++) {
        routers.push_back(make_shared<Router>(i, TRANSMISSION_RATE, QUEUE_CAPACITY));
    }

    // Schedule initial packet arrivals for each router
    for (auto& router : routers) {
        double initialTime = arrivalDist(gen);
        Packet firstPacket(1, PacketType::AUDIO, initialTime, 1000, 0.01);
        scheduleEvent(initialTime, EventType::PACKET_ARRIVAL, [router, firstPacket]() {
            handlePacketArrival(router, firstPacket);
        });
    }

    // Run the event-driven simulation
    runSimulation();

    cout << "Simulation Completed!" << endl;
    return 0;
}
