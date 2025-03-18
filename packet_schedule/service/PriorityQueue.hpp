#ifndef PRIORITY_QUEUE_HPP
#define PRIORITY_QUEUE_HPP

#include <queue>
#include <iostream>
#include "../include/Packet.hpp"

// Define queue priority types
enum class QueueType { PREMIUM, BEST_EFFORT, ASSURED };

// Define drop reasons
enum class DropReason { QUEUE_FULL, NONE };

// Queue with statistics tracking
class PriorityQueue {
private:
    QueueType type;
    std::queue<Packet> packets;
    size_t capacity;
    size_t maxSize;

    // Statistics
    size_t totalArrivals;
    size_t totalDrops;
    double totalWaitTime;
    double totalBacklogSize;
    size_t backlogSampleCount;

    // Reference packet tracking
    size_t refArrivals;
    size_t refDrops;

public:
    PriorityQueue(QueueType t, size_t cap)
        : type(t), capacity(cap), maxSize(0),
        totalArrivals(0), totalDrops(0), totalWaitTime(0),
        totalBacklogSize(0), backlogSampleCount(0),
        refArrivals(0), refDrops(0) {
    }

    bool enqueue(Packet packet, double currentTime, DropReason& reason) {
        totalArrivals++;
        if (packet.isReference) refArrivals++;

        if (capacity != static_cast<size_t>(-1) && packets.size() >= capacity) {
            totalDrops++;
            if (packet.isReference) refDrops++;
            reason = DropReason::QUEUE_FULL;
            return false; // Packet dropped
        }

        packets.push(packet);
        if (packets.size() > maxSize) {
            maxSize = packets.size();
        }

        reason = DropReason::NONE;
        return true;  // Packet accepted
    }

    bool hasPackets() const {
        return !packets.empty();
    }

    Packet dequeue(double currentTime) {
        if (packets.empty()) {
            throw std::runtime_error("[PriorityQueue] Attempted to dequeue from an empty queue!");
        }

        Packet packet = packets.front();
        packets.pop();

        // Calculate waiting time for this packet
        double waitTime = currentTime - packet.nodeArrivalTimes.back();
        totalWaitTime += waitTime;

        return packet;
    }

    void updateStats(double currentTime) {
        totalBacklogSize += packets.size();
        backlogSampleCount++;
    }

    // Statistics getters
    double getAverageWaitTime() const {
        if (totalArrivals - totalDrops > 0) {
            return totalWaitTime / (totalArrivals - totalDrops);
        }
        return 0.0;
    }

    double getBlockingRatio() const {
        if (totalArrivals > 0) {
            return static_cast<double>(totalDrops) / totalArrivals;
        }
        return 0.0;
    }

    double getAverageBacklog() const {
        if (backlogSampleCount > 0) {
            return static_cast<double>(totalBacklogSize) / backlogSampleCount;
        }
        return 0.0;
    }

    size_t size() const {
        return packets.size();
    }

    QueueType getType() const {
        return type;
    }

    // Reference packet statistics
    size_t getRefArrivals() const {
        return refArrivals;
    }

    size_t getRefDrops() const {
        return refDrops;
    }

    std::string getQueueTypeName() const {
        switch (type) {
        case QueueType::PREMIUM: return "Premium";
        case QueueType::ASSURED: return "Assured";
        case QueueType::BEST_EFFORT: return "Best-Effort";
        default: return "Unknown";
        }
    }
};

#endif // PRIORITY_QUEUE_HPP
