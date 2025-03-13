#ifndef FIFO_QUEUE_HPP
#define FIFO_QUEUE_HPP

#include "IQueue.hpp"
#include <queue>

class FIFOQueue : public IQueue {
private:
    std::queue<Packet> queue;  // Standard queue for FIFO behavior
    size_t maxCapacity;        // Maximum queue size

public:
    // Constructor: initializes queue with a fixed max capacity
    FIFOQueue(size_t capacity) : maxCapacity(capacity) {}

    // Add a packet to the queue
    void enqueue(const Packet& packet) override {
        if (queue.size() < maxCapacity) {
            queue.push(packet);
        } else {
            std::cout << "[FIFOQueue] Packet dropped - Queue is full!" << std::endl;
        }
    }

    // Remove and return the next packet in the queue
    Packet dequeue() override {
        if (!queue.empty()) {
            Packet packet = queue.front();
            queue.pop();
            return packet;
        } else {
            throw std::runtime_error("[FIFOQueue] Attempt to dequeue from an empty queue!");
        }
    }

    // Check if the queue is empty
    bool isEmpty() const override {
        return queue.empty();
    }

    // Get the current queue size
    size_t getSize() const override {
        return queue.size();
    }

    // Get the maximum queue capacity
    size_t getCapacity() const override {
        return maxCapacity;
    }
};

#endif // FIFO_QUEUE_HPP
