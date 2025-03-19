#include "../include/Packet.hpp"

Packet::Packet(double arrival_time, int size, Priority priority, PacketType type)
    : arrival_time(arrival_time), departure_time(0.0), size(size), priority(priority), type(type) {
}

Packet::~Packet() = default;
