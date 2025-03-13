#ifndef IQUEUE_HPP
#define IQUEUE_HPP

#include <queue>
#include "../include/Packet.hpp"

// Abstract Queue Interface
class IQueue {
public:
    virtual ~IQueue() = default;

    // Add a packet to the queue
    virtual void enqueue(const Packet& packet) = 0;

    // Remove and return the next packet in the queue
    virtual Packet dequeue() = 0;

    // Check if the queue is empty
    virtual bool isEmpty() const = 0;

    // Get the size of the queue
    virtual size_t getSize() const = 0;

    // Get the maximum capacity of the queue
    virtual size_t getCapacity() const = 0;
};

#endif // IQUEUE_HPP
