#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <queue>
#include <vector>
#include <functional>
#include "Packet.hpp"
#include "Event.hpp"

/**
 * \brief manages packet queuing and scheduling.
 */
class Router {
private:
    int routerId;
    double transmissionRate; // Bits per second
    double bufferSize;       // Max number of packets in queue

    // Priority queues for packet scheduling
    std::queue<Packet> premiumQueue;  // High-priority queue (Audio)
    std::queue<Packet> assuredQueue;  // Medium-priority queue (Video)
    std::queue<Packet> bestEffortQueue; // Low-priority queue (Data)

public:
    Router(int id, double rate, double buffer)
        : routerId(id), transmissionRate(rate), bufferSize(buffer) {}

    int getId() const {
        return routerId;
    }

    // Add packet to the appropriate queue
    void enqueuePacket(const Packet& packet) {
        if (packet.type == PacketType::AUDIO) {
            if (premiumQueue.size() < bufferSize)
                premiumQueue.push(packet);
            else
                std::cout << "[Router " << routerId << "] Dropped AUDIO packet (Queue full)\n";
        }
        else if (packet.type == PacketType::VIDEO) {
            if (assuredQueue.size() < bufferSize)
                assuredQueue.push(packet);
            else
                std::cout << "[Router " << routerId << "] Dropped VIDEO packet (Queue full)\n";
        }
        else {
            if (bestEffortQueue.size() < bufferSize)
                bestEffortQueue.push(packet);
            else
                std::cout << "[Router " << routerId << "] Dropped DATA packet (Queue full)\n";
        }
    }

    // Process the next packet in the highest-priority queue available
    void Router::processPacket(std::function<void(double, EventType, std::function<void()>)> scheduleEvent, double currentTime) {
        if (!premiumQueue.empty()) {
            Packet p = premiumQueue.front();
            premiumQueue.pop();
            double departureTime = currentTime + p.serviceTime;
            scheduleEvent(departureTime, EventType::PACKET_DEPARTURE, [this]() { this->packetDeparted(); });
        }
        else if (!assuredQueue.empty()) {
            Packet p = assuredQueue.front();
            assuredQueue.pop();
            double departureTime = currentTime + p.serviceTime;
            scheduleEvent(departureTime, EventType::PACKET_DEPARTURE, [this]() { this->packetDeparted(); });
        }
        else if (!bestEffortQueue.empty()) {
            Packet p = bestEffortQueue.front();
            bestEffortQueue.pop();
            double departureTime = currentTime + p.serviceTime;
            scheduleEvent(departureTime, EventType::PACKET_DEPARTURE, [this]() { this->packetDeparted(); });
        }
    }

    void packetDeparted() {
        std::cout << "[Router " << routerId << "] Packet departed\n";
    }
};

#endif // ROUTER_HPP
