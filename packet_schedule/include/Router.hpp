#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <vector>
#include <queue>
#include <iostream>
#include <functional>
#include "Packet.hpp"
#include "Event.hpp"
#include "../service/PriorityQueue.hpp"

class Router
{
private:
    int routerId;
    double transmissionRate;

    // Three priority queues
    PriorityQueue premiumQueue;
    PriorityQueue assuredQueue;
    PriorityQueue bestEffortQueue;

public:
    Router(int id, double rate, size_t bufferSize)
        : routerId(id), transmissionRate(rate),
          premiumQueue(QueueType::PREMIUM, bufferSize),
          assuredQueue(QueueType::ASSURED, bufferSize),
          bestEffortQueue(QueueType::BEST_EFFORT, bufferSize)
    {
    }

    int getId() const { return routerId; }

    // Enqueue a packet into the appropriate priority queue
    bool enqueuePacket(const Packet& packet, double currentTime);

    // Process a packet from the highest priority queue
    void processPacket(std::function<void(double, EventType, std::function<void()>)> scheduleEvent, double currentTime);

    // Get queue statistics
    double getAverageQueueDelay(QueueType type) const;
    double getBlockingRatio(QueueType type) const;
    double getAverageBacklog(QueueType type) const;
};

#endif // ROUTER_HPP
