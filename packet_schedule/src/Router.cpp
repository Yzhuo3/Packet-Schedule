#include "../include/Router.h"
#include <iostream>
#include <cstdlib>
#include <queue>
#include <unordered_map>

struct Event;
int Router::totalQueuingDelay = 0;
int Router::totalServiceDelay = 0;
int Router::processedPacketCount = 0;

Router::Router() : id(-1), currentTime(0) {}

Router::Router(int id) : id(id), currentTime(0) {}

void Router::processPackets(std::unordered_map<int, Router>& network, std::priority_queue<Event, std::vector<Event>, EventCompare>& eventList) {
    int queueSize = packetQueue.size();
    for (int i = 0; i < queueSize; ++i) {
        Packet packet = packetQueue.front();
        packetQueue.pop();

        packet.queuingDelay = currentTime - packet.arrivalTime;
        totalQueuingDelay += packet.queuingDelay;

        packet.serviceTime = 1 + (std::rand() % 3);
        totalServiceDelay += packet.serviceTime;
        currentTime += packet.serviceTime;

        processedPacketCount++;

        if (packet.destination == id) {
            std::cout << "Router " << id << " received packet " << packet.id 
                      << " from Router " << packet.source 
                      << ". Queuing delay: " << packet.queuingDelay 
                      << " units, Service time: " << packet.serviceTime 
                      << " units. Data: " << packet.data << std::endl;
        } else {
            forwardPacket(packet, network, eventList);
        }
    }
}

void Router::incrementTime() {
    currentTime++;
}

void Router::forwardPacket(Packet& packet, std::unordered_map<int, Router>& network, std::priority_queue<Event, std::vector<Event>, EventCompare>& eventList) {
    int nextRouter = (id + 1) % network.size();
    packet.arrivalTime = currentTime;

    std::cout << "Router " << id << " forwarding packet " << packet.id 
              << " to Router " << nextRouter << std::endl;
    network[nextRouter].packetQueue.push(packet);

    double serviceTime = 1 + (std::rand() % 3);
    eventList.push(Event(currentTime + serviceTime, DEPARTURE));
}
