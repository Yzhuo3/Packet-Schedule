#ifndef SP_QUEUE_HPP
#define SP_QUEUE_HPP

#include "IQueue.hpp"
#include <queue>
#include <map>

class SPQueue : public IQueue {
private:
    // Separate queues for different priority levels
    std::queue<Packet> premiumQueue;    // Highest priority (Audio)
    std::queue<Packet> assuredQueue;    // Medium priority (Video)
    std::queue<Packet> bestEffortQueue; // Lowest priority (Data)
    
    size_t maxCapacity; // Maximum number of packets per queue

public:
    // Constructor: initializes queues with a max capacity
    SPQueue(size_t capacity) : maxCapacity(capacity) {}

    // Add a packet to the appropriate priority queue
    void enqueue(const Packet& packet) override {
        switch (packet.type) {
            case PacketType::AUDIO:
                if (premiumQueue.size() < maxCapacity)
                    premiumQueue.push(packet);
                else
                    std::cout << "[SPQueue] AUDIO packet dropped (Queue full!)\n";
                break;

            case PacketType::VIDEO:
                if (assuredQueue.size() < maxCapacity)
                    assuredQueue.push(packet);
                else
                    std::cout << "[SPQueue] VIDEO packet dropped (Queue full!)\n";
                break;

            case PacketType::DATA:
                if (bestEffortQueue.size() < maxCapacity)
                    bestEffortQueue.push(packet);
                else
                    std::cout << "[SPQueue] DATA packet dropped (Queue full!)\n";
                break;
        }
    }

    // Remove and return the highest-priority packet available
    Packet dequeue() override {
        if (!premiumQueue.empty()) {
            Packet packet = premiumQueue.front();
            premiumQueue.pop();
            return packet;
        } 
        else if (!assuredQueue.empty()) {
            Packet packet = assuredQueue.front();
            assuredQueue.pop();
            return packet;
        } 
        else if (!bestEffortQueue.empty()) {
            Packet packet = bestEffortQueue.front();
            bestEffortQueue.pop();
            return packet;
        } 
        else {
            throw std::runtime_error("[SPQueue] Attempt to dequeue from an empty queue!");
        }
    }

    // Check if all queues are empty
    bool isEmpty() const override {
        return premiumQueue.empty() && assuredQueue.empty() && bestEffortQueue.empty();
    }

    // Get the total number of packets in all queues
    size_t getSize() const override {
        return premiumQueue.size() + assuredQueue.size() + bestEffortQueue.size();
    }

    // Get the maximum capacity of each priority queue
    size_t getCapacity() const override {
        return maxCapacity;
    }
};

#endif // SP_QUEUE_HPP
