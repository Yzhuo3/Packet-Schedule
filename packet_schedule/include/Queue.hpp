#ifndef QUEUE_H
#define QUEUE_H

#include "Packet.hpp"
#include <queue>

class Queue {
public:
    Queue();
    bool isEmpty() const;
    bool enqueue(Packet* packet);
    Packet* dequeue();
    int size() const;
    Packet* peek() const;

private:
    std::queue<Packet*> q;
};

#endif // QUEUE_H