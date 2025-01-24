#ifndef EVENT_H
#define EVENT_H

#include <functional>

enum EventType { ARRIVAL, DEPARTURE };

struct Event {
    double eventTime;
    EventType type;

    Event(double time, EventType eventType) : eventTime(time), type(eventType) {}
};

struct EventCompare {
    bool operator()(const Event& e1, const Event& e2) {
        return e1.eventTime > e2.eventTime; // Earliest event first
    }
};

#endif // EVENT_H
