#ifndef PACKET_H
#define PACKET_H

enum class PacketType {
    AUDIO,
    VIDEO,
    DATA
};

enum class Priority {
    PREMIUM,
    ASSURED,
    BEST_EFFORT
};

class Packet {
public:
    double arrival_time;
    double departure_time;
    int size;
    Priority priority;
    PacketType type;

    bool is_reference;

    Packet(double arrival_time, int size, Priority priority, PacketType type);
    ~Packet();
};

#endif