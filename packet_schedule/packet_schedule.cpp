#include <iostream>
#include <unordered_map>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include "include/Event.h"
#include "include/Packet.h"
#include "include/Router.h"

const int SIM_TIME = 1000;
const double ARRIVAL_RATE = 1.0 / 5.0;

double exponential(double mean) {
    double u = (double)std::rand() / RAND_MAX;
    return -mean * std::log(1 - u);
}

int main() {
    std::srand(std::time(nullptr));

    std::unordered_map<int, Router> network;
    for (int i = 0; i < 3; ++i) {
        network[i] = Router(i);
    }

    std::priority_queue<Event, std::vector<Event>, EventCompare> eventList;

    int packetCount = 5;
    for (int i = 0; i < packetCount; ++i) {
        int src = std::rand() % 3;
        int dest = std::rand() % 3;
        while (dest == src) {
            dest = std::rand() % 3;
        }

        Packet packet(i, src, dest, 100, "Hello from " + std::to_string(src), 0);
        network[src].packetQueue.push(packet);
        eventList.push(Event(exponential(1 / ARRIVAL_RATE), ARRIVAL));

        std::cout << "Generated packet " << i << " from Router " << src 
                  << " to Router " << dest << std::endl;
    }

    double currentTime = 0;
    while (currentTime < SIM_TIME) {
        if (eventList.empty()) break;

        Event nextEvent = eventList.top();
        eventList.pop();
        currentTime = nextEvent.eventTime;

        for (auto& pair : network) {
            int id = pair.first;
            Router& router = pair.second;
            router.currentTime = currentTime;
            router.processPackets(network, eventList);
        }
    }

    if (Router::processedPacketCount > 0) {
        double averageQueuingDelay = static_cast<double>(Router::totalQueuingDelay) / Router::processedPacketCount;
        double averageServiceDelay = static_cast<double>(Router::totalServiceDelay) / Router::processedPacketCount;
        double averageSystemDelay = averageQueuingDelay + averageServiceDelay;

        std::cout << "\nTotal Queuing Delay: " << Router::totalQueuingDelay << " units" << std::endl;
        std::cout << "Total Service Delay: " << Router::totalServiceDelay << " units" << std::endl;
        std::cout << "Processed Packet Count: " << Router::processedPacketCount << std::endl;
        std::cout << "Average Queuing Delay: " << averageQueuingDelay << " units" << std::endl;
        std::cout << "Average Service Delay: " << averageServiceDelay << " units" << std::endl;
        std::cout << "Average Total System Delay: " << averageSystemDelay << " units" << std::endl;
    } else {
        std::cout << "No packets were processed during the simulation." << std::endl;
    }

    return 0;
}
