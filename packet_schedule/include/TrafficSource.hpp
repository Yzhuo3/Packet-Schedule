#ifndef TRAFFICSOURCE_H
#define TRAFFICSOURCE_H

#include "Packet.hpp"

// Define traffic types.
enum class TrafficType {
    AUDIO,
    VIDEO,
    DATA,
    REFERENCE
};

class TrafficSource {
public:
    TrafficType type;
    double peak_rate;      // in kbps
    double mean_on_time;   // average ON time in seconds
    double mean_off_time;  // average OFF time in seconds
    int packet_size;       // packet size in bytes

    // State variables for the ON/OFF model.
    bool is_on;
    double next_state_change_time;

    TrafficSource(TrafficType type, double peak_rate, double mean_on_time, double mean_off_time, int packet_size);
    ~TrafficSource();

    // Generate a packet at the current time if the source is ON.
    Packet* generatePacket(double current_time);
};

#endif // TRAFFICSOURCE_H
