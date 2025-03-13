#ifndef PACKET_HPP
#define PACKET_HPP

#include <iostream>

enum class PacketType {
    AUDIO,  // High-priority packets
    VIDEO,  // Medium-priority packets
    DATA    // Low-priority packets
};

class Packet {
public:
    int id;               // Unique identifier for the packet
    PacketType type;      // Packet type (audio, video, data)
    double arrivalTime;   // Arrival time at the node
    double size;          // Packet size in bits
    double serviceTime;   // Service time based on transmission rate

    Packet(int packetId, PacketType packetType, double arrival, double packetSize, double service)
        : id(packetId), type(packetType), arrivalTime(arrival), size(packetSize), serviceTime(service) {}

    void display() const {
        std::cout << "Packet ID: " << id
                  << ", Type: " << static_cast<int>(type)
                  << ", Arrival Time: " << arrivalTime
                  << ", Size: " << size
                  << ", Service Time: " << serviceTime
                  << std::endl;
    }
};

#endif // PACKET_HPP
