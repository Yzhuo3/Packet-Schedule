#include "../include/Queue.hpp"

Queue::Queue()
{
}

bool Queue::isEmpty() const
{
    return q.empty();
}

bool Queue::enqueue(Packet* packet)
{
    q.push(packet);
    return true;
}

Packet* Queue::dequeue()
{
    if (q.empty())
        return nullptr;
    Packet* packet = q.front();
    q.pop();
    return packet;
}

int Queue::size() const
{
    return static_cast<int>(q.size());
}

Packet* Queue::peek() const
{
    if (q.empty())
        return nullptr;
    return q.front();
}
