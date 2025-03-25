#include "../include/Event.hpp"

Event::Event(double event_time, EventType type, Packet* packet, int node_id, TrafficSource* source)
    : event_time(event_time), type(type), packet(packet), node_id(node_id), source(source)
{
}

Event::~Event() = default;
