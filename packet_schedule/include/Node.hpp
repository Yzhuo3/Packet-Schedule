#ifndef NODE_H
#define NODE_H

#include "Packet.hpp"
#include "Queue.hpp"

// Node represents a network node that implements the SPQ queuing discipline.
class Node {
public:
    int id;
    double transmission_rate; // in bits per second

    // Three FIFO queues for SPQ: Premium, Assured, and Best-Effort.
    Queue premium_queue;
    Queue assured_queue;
    Queue best_effort_queue;

    int combinedCapacity; // Total capacity for all three queues (K)

    // Constructor: id, combined capacity, and transmission rate.
    Node(int id, int combinedCapacity, double transmission_rate);
    ~Node();

    // Enqueue a packet into the appropriate queue after checking the combined capacity.
    // Returns true if the packet was enqueued, false if dropped.
    bool enqueuePacket(Packet* packet);

    // Dequeue a packet from the highest-priority non-empty queue.
    Packet* dequeuePacket();

    // Peek at the next packet from the highest-priority non-empty queue without removing it.
    Packet* peekPacket();
};

#endif // NODE_H