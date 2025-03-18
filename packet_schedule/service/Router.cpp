#include "../include/Router.hpp"

// Enqueue packet into the appropriate priority queue
bool Router::enqueuePacket(const Packet& packet, double currentTime) {
    DropReason reason;
    bool enqueued = false;

    switch (packet.type) {
    case PacketType::AUDIO:
    case PacketType::REFERENCE:
        enqueued = premiumQueue.enqueue(packet, currentTime, reason);
        break;
    case PacketType::VIDEO:
        enqueued = assuredQueue.enqueue(packet, currentTime, reason);
        break;
    case PacketType::DATA:
        enqueued = bestEffortQueue.enqueue(packet, currentTime, reason);
        break;
    }

    if (!enqueued) {
        std::cout << "[Router " << routerId << "] Dropped packet ID " << packet.creationTime
                  << " due to queue full.\n";
    }
    
    return enqueued;
}

// Process packets based on strict priority scheduling
void Router::processPacket(std::function<void(double, EventType, std::function<void()>)> scheduleEvent, double currentTime) {
    PriorityQueue* selectedQueue = nullptr;

    if (premiumQueue.hasPackets()) {
        selectedQueue = &premiumQueue;
    } else if (assuredQueue.hasPackets()) {
        selectedQueue = &assuredQueue;
    } else if (bestEffortQueue.hasPackets()) {
        selectedQueue = &bestEffortQueue;
    }

    if (selectedQueue) {
        Packet packet = selectedQueue->dequeue(currentTime);
        double departureTime = currentTime + (packet.size * 8) / transmissionRate;

        scheduleEvent(departureTime, EventType::PACKET_DEPARTURE, [this]() {
            std::cout << "[Router " << routerId << "] Packet departed.\n";
        });
    }
}

// Get queue statistics
double Router::getAverageQueueDelay(QueueType type) const {
    switch (type) {
        case QueueType::PREMIUM: return premiumQueue.getAverageWaitTime();
        case QueueType::ASSURED: return assuredQueue.getAverageWaitTime();
        case QueueType::BEST_EFFORT: return bestEffortQueue.getAverageWaitTime();
        default: return 0.0;
    }
}

double Router::getBlockingRatio(QueueType type) const {
    switch (type) {
        case QueueType::PREMIUM: return premiumQueue.getBlockingRatio();
        case QueueType::ASSURED: return assuredQueue.getBlockingRatio();
        case QueueType::BEST_EFFORT: return bestEffortQueue.getBlockingRatio();
        default: return 0.0;
    }
}

double Router::getAverageBacklog(QueueType type) const {
    switch (type) {
        case QueueType::PREMIUM: return premiumQueue.getAverageBacklog();
        case QueueType::ASSURED: return assuredQueue.getAverageBacklog();
        case QueueType::BEST_EFFORT: return bestEffortQueue.getAverageBacklog();
        default: return 0.0;
    }
}
