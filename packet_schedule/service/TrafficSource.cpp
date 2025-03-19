#include "../include/TrafficSource.hpp"
#include <cmath>
#include <cstdlib>

// Utility function: generate an exponential random variable with the given mean.
static double exponentialRandom(double mean) {
    double u = static_cast<double>(rand()) / (RAND_MAX + 1.0);
    return -mean * log(1 - u);
}

TrafficSource::TrafficSource(TrafficType type, double peak_rate, double mean_on_time, double mean_off_time, int packet_size)
    : type(type), peak_rate(peak_rate), mean_on_time(mean_on_time), mean_off_time(mean_off_time), packet_size(packet_size), is_on(false) {
    next_state_change_time = exponentialRandom(mean_off_time);
}

TrafficSource::~TrafficSource() = default;

Packet* TrafficSource::generatePacket(double current_time) {
    // Switch state if the scheduled time has been reached.
    if (current_time >= next_state_change_time) {
        is_on = !is_on;
        if (is_on)
            next_state_change_time = current_time + exponentialRandom(mean_on_time);
        else
            next_state_change_time = current_time + exponentialRandom(mean_off_time);
    }

    // If the source is in the ON state, generate and return a packet.
    if (is_on) {
        Priority priority;
        PacketType ptype;
        switch (type) {
            case TrafficType::AUDIO:
                priority = Priority::PREMIUM;
                ptype = PacketType::AUDIO;
                break;
            case TrafficType::VIDEO:
                priority = Priority::ASSURED;
                ptype = PacketType::VIDEO;
                break;
            case TrafficType::DATA:
                priority = Priority::BEST_EFFORT;
                ptype = PacketType::DATA;
                break;
            case TrafficType::REFERENCE:
                // You may choose a specific priority for reference traffic.
                priority = Priority::PREMIUM;
                ptype = PacketType::REFERENCE;
                break;
            default:
                priority = Priority::BEST_EFFORT;
                ptype = PacketType::DATA;
                break;
        }
        return new Packet(current_time, packet_size, priority, ptype);
    }
    return nullptr;
}
