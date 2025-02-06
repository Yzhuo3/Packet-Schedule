#ifndef PACKET_H
#define PACKET_H

#include <string>

enum class PacketType { AUDIO, VIDEO, DATA };

struct Packet {
    int id;
    int source;
    int destination;
    int size;
    std::string data;
    int arrivalTime;
    int queuingDelay;
    int serviceTime;

    PacketType type;

    Packet(int id, int src, int dest, int size, const std::string& data, double arrivalTime, PacketType type)
        : id(id), source(src), destination(dest), size(size), data(data), arrivalTime(arrivalTime), queuingDelay(0), serviceTime(0), type(type) {}
};

#endif // PACKET_H
