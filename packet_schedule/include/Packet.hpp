#ifndef PACKET_HPP
#define PACKET_HPP

#include <vector>
#include <iostream>

// Define packet types
enum class PacketType { AUDIO, VIDEO, DATA, REFERENCE };

// Structure to represent a packet
struct Packet {
    PacketType type;
    double creationTime;
    double arrivalTime;  // ✅ Added this missing field
    double size; // in bytes
    int sourceNode;
    bool isReference;
    std::vector<double> nodeArrivalTimes;
    std::vector<double> nodeDepartureTimes;

    Packet(PacketType t, double creation, double arrival, double s, int node, bool ref = false)
        : type(t), creationTime(creation), arrivalTime(arrival), size(s), sourceNode(node), isReference(ref) {
        nodeArrivalTimes.push_back(arrival);
    }
};

#endif // PACKET_HPP
