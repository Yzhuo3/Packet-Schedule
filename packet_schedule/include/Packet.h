#ifndef PACKET_H
#define PACKET_H

#include <string>

struct Packet {
    int id;
    int source;
    int destination;
    int size;
    std::string data;
    int arrivalTime;
    int queuingDelay;
    int serviceTime;

    Packet(int id, int src, int dest, int size, const std::string& data, int arrivalTime)
        : id(id), source(src), destination(dest), size(size), data(data), arrivalTime(arrivalTime), queuingDelay(0), serviceTime(0) {}
};

#endif // PACKET_H
