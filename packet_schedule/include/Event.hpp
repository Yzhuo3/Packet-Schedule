#ifndef EVENT_HPP
#define EVENT_HPP

#include <iostream>
#include <functional>

enum class EventType {
    PACKET_ARRIVAL,
    PACKET_DEPARTURE
};

/**
 * \brief packet arrivals and departures.
 */
class Event {
public:
    double time;       // Event timestamp
    EventType type;    // Event type (arrival or departure)
    
    // Function pointer to handle the event
    std::function<void()> eventHandler;  

    Event(double eventTime, EventType eventType, std::function<void()> handler)
        : time(eventTime), type(eventType), eventHandler(handler) {}

    // Execute the event action
    void execute() {
        if (eventHandler) {
            eventHandler();
        }
    }

    // Comparator for event priority queue (earlier time has higher priority)
    bool operator<(const Event& other) const {
        return time > other.time;
    }
};

#endif // EVENT_HPP
