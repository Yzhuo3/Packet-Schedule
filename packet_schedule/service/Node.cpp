#include "../include/Node.hpp"
#include <iostream>

Node::Node(int id, int combinedCapacity, double transmission_rate)
    : id(id), transmission_rate(transmission_rate), combinedCapacity(combinedCapacity)
{
}

Node::~Node()
{
    // Delete all remaining packets in the queues.
    Packet* pkt;
    while ((pkt = dequeuePacket()) != nullptr)
        delete pkt;
}

bool Node::enqueuePacket(Packet* packet)
{
    // Calculate total packets in all queues.
    int perQueueCapacity = combinedCapacity / 3;
    
    switch (packet->priority) {
    case Priority::PREMIUM:
        if (premium_queue.size() >= perQueueCapacity)
            return false;
        return premium_queue.enqueue(packet);
    case Priority::ASSURED:
        if (assured_queue.size() >= perQueueCapacity)
            return false;
        return assured_queue.enqueue(packet);
    case Priority::BEST_EFFORT:
        if (best_effort_queue.size() >= perQueueCapacity)
            return false;
        return best_effort_queue.enqueue(packet);
    default:
        return false;
    }
}

Packet* Node::dequeuePacket()
{
    // Always serve the highest-priority non-empty queue first.
    if (!premium_queue.isEmpty())
        return premium_queue.dequeue();
    else if (!assured_queue.isEmpty())
        return assured_queue.dequeue();
    else if (!best_effort_queue.isEmpty())
        return best_effort_queue.dequeue();
    return nullptr;
}

Packet* Node::peekPacket()
{
    // Peek at the highest-priority non-empty queue.
    if (!premium_queue.isEmpty())
        return premium_queue.peek();
    else if (!assured_queue.isEmpty())
        return assured_queue.peek();
    else if (!best_effort_queue.isEmpty())
        return best_effort_queue.peek();
    return nullptr;
}
