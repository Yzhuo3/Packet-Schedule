#ifndef TRAFFICSOURCE_H
#define TRAFFICSOURCE_H

#include "Packet.hpp"

enum class TrafficType {
    AUDIO,
    VIDEO,
    DATA
};

class TrafficSource {
public:
    // Now we only have AUDIO, VIDEO, or DATA for queue logic
    TrafficType type;
    bool is_reference;     // NEW: true if this flow is the special "reference" flow

    double peak_rate;      // in kbps
    double mean_on_time;   // average ON time (seconds)
    double mean_off_time;  // average OFF time (seconds)
    int packet_size;       // packet size in bytes

    bool is_on;
    double next_state_change_time;

    TrafficSource(TrafficType type, bool is_reference,
                  double peak_rate, double mean_on_time,
                  double mean_off_time, int packet_size);
    ~TrafficSource();

    Packet* generatePacket(double current_time);
};

#endif // TRAFFICSOURCE_H
