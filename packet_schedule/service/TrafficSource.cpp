#include "../include/TrafficSource.hpp"
#include <cmath>
#include <cstdlib>

// Utility for exponential
static double exponentialRandom(double mean)
{
    double u = (double)rand() / (RAND_MAX + 1.0);
    return -mean * log(1 - u);
}

TrafficSource::TrafficSource(TrafficType type, bool is_reference,
                             double peak_rate, double mean_on_time,
                             double mean_off_time, int packet_size)
    : type(type),
      is_reference(is_reference),
      peak_rate(peak_rate),
      mean_on_time(mean_on_time),
      mean_off_time(mean_off_time),
      packet_size(packet_size),
      is_on(false)
{
    next_state_change_time = exponentialRandom(mean_off_time);
}

TrafficSource::~TrafficSource() = default;

Packet* TrafficSource::generatePacket(double current_time)
{
    // Switch ON/OFF
    if (current_time >= next_state_change_time)
    {
        is_on = !is_on;
        if (is_on)
            next_state_change_time = current_time + exponentialRandom(mean_on_time);
        else
            next_state_change_time = current_time + exponentialRandom(mean_off_time);
    }

    // If OFF, return nullptr
    if (!is_on)
    {
        return nullptr;
    }

    // Otherwise, generate a packet
    Priority priority;
    PacketType ptype;

    switch (type)
    {
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
    }

    // Create the packet
    Packet* pkt = new Packet(current_time, packet_size, priority, ptype);
    // Mark whether it's reference
    pkt->is_reference = this->is_reference;
    return pkt;
}
