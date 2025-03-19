#ifndef EVENT_H
#define EVENT_H

#include "Packet.hpp"
#include "TrafficSource.hpp"

enum class EventType {
    ARRIVAL,
    DEPARTURE
};

class Event {
public:
    double event_time;
    EventType type;
    Packet* packet;
    int node_id;              // ID of the node where the event occurs
    TrafficSource* source;    // The traffic source associated with this event (if applicable)

    Event(double event_time, EventType type, Packet* packet, int node_id, TrafficSource* source = nullptr);
    ~Event();
};

#endif // EVENT_H
