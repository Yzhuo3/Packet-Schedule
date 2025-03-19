#include "../include/SimulationEngine.hpp"
#include "../include/Event.hpp"
#include "../include/Packet.hpp"
#include <iostream>
#include <cmath>

// Constructor: initialize simulation clock, end time, and statistics.
SimulationEngine::SimulationEngine(double end_time)
    : current_time(0.0), end_time(end_time)
{
    referenceStats.totalReferenceArrivals = 0;
    referenceStats.totalReferenceDepartures = 0;
    referenceStats.totalReferenceDropped = 0;
    referenceStats.totalReferenceDelay = 0.0;
}

// Destructor: clean up any remaining events in the queue.
SimulationEngine::~SimulationEngine()
{
    while (!eventQueue.empty())
    {
        Event* ev = eventQueue.top();
        eventQueue.pop();
        delete ev;
    }
    // Note: Nodes and TrafficSources are assumed to be managed elsewhere.
}

void SimulationEngine::addNode(Node* node)
{
    nodes.push_back(node);
    // Initialize statistics for this node.
    NodeStats stats = {0, 0, 0, 0.0, 0, 0, 0, 0.0, 0.0, 0.0, 0};
    nodeStatsMap[node->id] = stats;
}

void SimulationEngine::addTrafficSource(TrafficSource* ts, int node_id)
{
    trafficSources.push_back(std::make_pair(ts, node_id));
}

void SimulationEngine::scheduleEvent(Event* event)
{
    eventQueue.push(event);
}

Node* SimulationEngine::getNodeById(int id)
{
    for (auto node : nodes)
    {
        if (node->id == id)
            return node;
    }
    return nullptr;
}

// Sample the current backlog of each queue for a node.
void SimulationEngine::sampleBacklog(Node* node)
{
    NodeStats& stats = nodeStatsMap[node->id];
    stats.cumulativeBacklogPremium += node->premium_queue.size();
    stats.cumulativeBacklogAssured += node->assured_queue.size();
    stats.cumulativeBacklogBestEffort += node->best_effort_queue.size();
    stats.backlogSamples++;
}

// Print the collected statistics.
void SimulationEngine::printStatistics()
{
    std::cout << "\n--- Simulation Statistics ---\n";
    for (auto& entry : nodeStatsMap)
    {
        int nodeId = entry.first;
        NodeStats& stats = entry.second;
        std::cout << "Node " << nodeId << ":\n";
        std::cout << "  Total Arrivals: " << stats.totalArrivals << "\n";
        std::cout << "  Total Departures: " << stats.totalDepartures << "\n";
        std::cout << "  Total Dropped: " << stats.totalDropped << "\n";
        double avgDelay = (stats.totalDepartures > 0) ? (stats.totalDelay / stats.totalDepartures) : 0.0;
        std::cout << "  Average Packet Delay: " << avgDelay << " seconds\n";
        std::cout << "  Dropped (Premium): " << stats.droppedPremium << "\n";
        std::cout << "  Dropped (Assured): " << stats.droppedAssured << "\n";
        std::cout << "  Dropped (Best Effort): " << stats.droppedBestEffort << "\n";
        if (stats.backlogSamples > 0)
        {
            double avgBacklogPremium = stats.cumulativeBacklogPremium / stats.backlogSamples;
            double avgBacklogAssured = stats.cumulativeBacklogAssured / stats.backlogSamples;
            double avgBacklogBestEffort = stats.cumulativeBacklogBestEffort / stats.backlogSamples;
            std::cout << "  Average Backlog (Premium): " << avgBacklogPremium << "\n";
            std::cout << "  Average Backlog (Assured): " << avgBacklogAssured << "\n";
            std::cout << "  Average Backlog (Best Effort): " << avgBacklogBestEffort << "\n";
        }
        std::cout << "\n";
    }
    std::cout << "Reference Traffic Statistics:\n";
    std::cout << "  Total Reference Arrivals: " << referenceStats.totalReferenceArrivals << "\n";
    std::cout << "  Total Reference Departures: " << referenceStats.totalReferenceDepartures << "\n";
    std::cout << "  Total Reference Dropped: " << referenceStats.totalReferenceDropped << "\n";
    double avgRefDelay = (referenceStats.totalReferenceDepartures > 0)
                             ? (referenceStats.totalReferenceDelay / referenceStats.totalReferenceDepartures)
                             : 0.0;
    std::cout << "  Average End-to-End Delay for Reference Traffic: " << avgRefDelay << " seconds\n";
}

// The main simulation loop.
void SimulationEngine::run()
{
    // INITIALIZATION:
    for (auto& pair : trafficSources)
    {
        TrafficSource* ts = pair.first;
        int node_id = pair.second;
        Event* event = new Event(current_time, EventType::ARRIVAL, nullptr, node_id, ts);
        scheduleEvent(event);
    }

    // MAIN EVENT LOOP:
    while (!eventQueue.empty())
    {
        Event* event = eventQueue.top();
        // If the next event is beyond end time, exit the loop.
        if (event->event_time >= end_time)
        {
            break;
        }
        eventQueue.pop();
        current_time = event->event_time;

        if (event->type == EventType::ARRIVAL)
        {
            // Process arrival for the specific TrafficSource.
            TrafficSource* ts = event->source;
            if (ts)
            {
                Packet* pkt = ts->generatePacket(current_time);
                double interarrival = (ts->packet_size * 8.0) / (ts->peak_rate * 1000.0);
                double next_time = (pkt ? current_time + interarrival : ts->next_state_change_time);

                Node* node = getNodeById(event->node_id);
                if (pkt)
                {
                    nodeStatsMap[node->id].totalArrivals++;
                    if (pkt->type == PacketType::REFERENCE)
                        referenceStats.totalReferenceArrivals++;

                    if (!node->enqueuePacket(pkt))
                    {
                        nodeStatsMap[node->id].totalDropped++;
                        switch (pkt->priority)
                        {
                        case Priority::PREMIUM:
                            nodeStatsMap[node->id].droppedPremium++;
                            break;
                        case Priority::ASSURED:
                            nodeStatsMap[node->id].droppedAssured++;
                            break;
                        case Priority::BEST_EFFORT:
                            nodeStatsMap[node->id].droppedBestEffort++;
                            break;
                        default:
                            break;
                        }
                        if (pkt->type == PacketType::REFERENCE)
                            referenceStats.totalReferenceDropped++;
                        delete pkt;
                    }
                    else
                    {
                        std::cout << "Time " << current_time << ": Packet enqueued at node "
                            << node->id << ".\n";
                        int totalQueueSize = node->premium_queue.size() +
                            node->assured_queue.size() +
                            node->best_effort_queue.size();
                        if (totalQueueSize == 1)
                        {
                            double transmission_time = (pkt->size * 8.0) / node->transmission_rate;
                            double departureTime = current_time + transmission_time;
                            if (departureTime < end_time)
                            {
                                Event* depEvent = new Event(departureTime,
                                                            EventType::DEPARTURE, nullptr, node->id);
                                scheduleEvent(depEvent);
                            }
                        }
                    }
                }
                // Schedule next arrival only if within simulation time.
                if (next_time < end_time)
                {
                    Event* nextArrival = new Event(next_time, EventType::ARRIVAL, nullptr, event->node_id, ts);
                    scheduleEvent(nextArrival);
                }
            }
        }
        else if (event->type == EventType::DEPARTURE)
        {
            Node* node = getNodeById(event->node_id);
            Packet* departed = node->dequeuePacket();
            if (departed)
            {
                departed->departure_time = current_time;
                double delay = current_time - departed->arrival_time;
                nodeStatsMap[node->id].totalDelay += delay;
                nodeStatsMap[node->id].totalDepartures++;
                if (departed->type == PacketType::REFERENCE)
                {
                    referenceStats.totalReferenceDelay += delay;
                    referenceStats.totalReferenceDepartures++;
                }
                std::cout << "Time " << current_time << ": Packet departed from node "
                    << node->id << ".\n";
                delete departed;
            }
            Packet* nextPkt = node->peekPacket();
            if (nextPkt)
            {
                double transmission_time = (nextPkt->size * 8.0) / node->transmission_rate;
                double nextDepTime = current_time + transmission_time;
                if (nextDepTime < end_time)
                {
                    Event* nextDep = new Event(nextDepTime, EventType::DEPARTURE, nullptr, node->id);
                    scheduleEvent(nextDep);
                }
            }
        }
        delete event;
    }
    // Optionally, clear any events remaining in the queue.
    while (!eventQueue.empty())
    {
        Event* ev = eventQueue.top();
        eventQueue.pop();
        delete ev;
    }

    printStatistics();
}
