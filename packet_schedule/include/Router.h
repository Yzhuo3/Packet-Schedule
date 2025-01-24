#ifndef ROUTER_H
#define ROUTER_H

#include <queue>
#include <unordered_map>
#include <vector>
#include "Packet.h"
#include "Event.h"

class Router {
public:
    int id;
    std::queue<Packet> packetQueue;
    int currentTime;

    static int totalQueuingDelay;
    static int totalServiceDelay;
    static int processedPacketCount;

    Router();
    Router(int id);

    void processPackets(std::unordered_map<int, Router>& network, std::priority_queue<Event, std::vector<Event>, EventCompare>& eventList);
    void incrementTime();

private:
    void forwardPacket(Packet& packet, std::unordered_map<int, Router>& network, std::priority_queue<Event, std::vector<Event>, EventCompare>& eventList);
};

#endif // ROUTER_H
