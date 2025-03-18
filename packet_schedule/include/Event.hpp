#ifndef EVENT_HPP
#define EVENT_HPP

#include <functional>

enum class EventType {
    PACKET_ARRIVAL,
    PACKET_DEPARTURE
};

class Event {
public:
    double time; // Event execution time
    EventType type;
    std::function<void()> eventHandler;

    Event(double eventTime, EventType eventType, std::function<void()> handler)
        : time(eventTime), type(eventType), eventHandler(handler) {}

    // Execute the event
    void execute() const {
        if (eventHandler) {
            eventHandler();
        }
    }

    // Comparator for priority queue (Min-Heap: smaller time executes first)
    bool operator>(const Event& other) const {
        return time > other.time; // Higher time means lower priority
    }
};

#endif // EVENT_HPP
