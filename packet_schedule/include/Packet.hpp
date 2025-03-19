#ifndef PACKET_H
#define PACKET_H

// Use enum classes for better type safety
enum class Priority {
    PREMIUM,
    ASSURED,
    BEST_EFFORT
};

enum class PacketType {
    AUDIO,
    VIDEO,
    DATA,
    REFERENCE
};

class Packet {
public:
    double arrival_time;    // Time when the packet arrives
    double departure_time;  // Time when the packet is transmitted
    int size;               // Packet size in bytes
    Priority priority;      // Priority level of the packet
    PacketType type;        // Type of packet

    Packet(double arrival_time, int size, Priority priority, PacketType type);
    ~Packet();
};

#endif // PACKET_H
