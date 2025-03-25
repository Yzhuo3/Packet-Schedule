#include "../include/SimulationEngine.hpp"
#include "../include/Event.hpp"
#include "../include/Packet.hpp"
#include "../include/Stats.hpp"
#include "../include/Node.hpp"
#include "../include/TrafficSource.hpp"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <ctime>
#include <sstream>
#include <utility>

// Constructor: initialize simulation clock, end time, and stats
SimulationEngine::SimulationEngine(const std::string& outputFilename,
                                   int M,
                                   int numAudioSources,
                                   int numVideoSources,
                                   int numDataSources,
                                   int spqSize,
                                   TrafficType refType,
                                   int totalPackets)
  : outputFilename(outputFilename),
    numNodes(M),
    numAudio(numAudioSources),
    numVideo(numVideoSources),
    numData(numDataSources),
    queueSize(spqSize),
    referenceType(refType),
    totalGeneratedPackets(totalPackets),
    arrivalsSoFar(0)
{
    current_time = 0.0;
    referenceStats.totalReferenceArrivals   = 0;
    referenceStats.totalReferenceDepartures = 0;
    referenceStats.totalReferenceDropped    = 0;
    referenceStats.totalReferenceDelay      = 0.0;
}


// Destructor: clean up any remaining events
SimulationEngine::~SimulationEngine()
{
    while (!eventQueue.empty()) {
        Event* ev = eventQueue.top();
        eventQueue.pop();
        delete ev;
    }
}

// Add a node to the simulation
void SimulationEngine::addNode(Node* node)
{
    nodes.push_back(node);
    
    NodeStats stats;
    stats.totalArrivals = 0;
    stats.totalDepartures = 0;
    stats.totalDropped = 0;
    stats.totalDelay = 0.0;
    stats.droppedPremium = 0;
    stats.droppedAssured = 0;
    stats.droppedBestEffort = 0;
    stats.cumulativeBacklogPremium = 0.0;
    stats.cumulativeBacklogAssured = 0.0;
    stats.cumulativeBacklogBestEffort = 0.0;
    stats.backlogSamples = 0;
    
    stats.premiumDelaySum = 0.0;
    stats.assuredDelaySum = 0.0;
    stats.bestEffortDelaySum = 0.0;
    stats.premiumCount = 0;
    stats.assuredCount = 0;
    stats.bestEffortCount = 0;
    
    stats.packetsIn = 0;
    stats.packetsOut = 0;

    nodeStatsMap[node->id] = stats;
}

// Add a traffic source
void SimulationEngine::addTrafficSource(TrafficSource* ts, int node_id)
{
    trafficSources.push_back(std::make_pair(ts, node_id));
}

// Schedule an event
void SimulationEngine::scheduleEvent(Event* event)
{
    eventQueue.push(event);
}

// Helper to find node by ID
Node* SimulationEngine::getNodeById(int id)
{
    for (auto node : nodes) {
        if (node->id == id) return node;
    }
    return nullptr;
}

// Sample the current backlog of each queue for a node
void SimulationEngine::sampleBacklog(Node* node)
{
    NodeStats& stats = nodeStatsMap[node->id];
    stats.cumulativeBacklogPremium += node->premium_queue.size();
    stats.cumulativeBacklogAssured += node->assured_queue.size();
    stats.cumulativeBacklogBestEffort += node->best_effort_queue.size();
    stats.backlogSamples++;
}

void SimulationEngine::run(const std::string &dateString)
{
    // 1) Initialize an arrival event at time=0 for each traffic source
    for (auto &pair : trafficSources) {
        TrafficSource* ts = pair.first;
        int node_id       = pair.second;
        // Create an ARRIVAL event at t=0
        Event* initEvent  = new Event(0.0, EventType::ARRIVAL, nullptr, node_id, ts);
        scheduleEvent(initEvent);
    }

    // 2) For a progress bar
    int lastProgress = -1;
    int barWidth     = 50;

    // 3) Main event loop
    while (!eventQueue.empty()) {
        // If we've reached the total number of arrivals, stop
        if (arrivalsSoFar >= totalGeneratedPackets) {
            break;
        }

        // Pop the earliest event
        Event* event = eventQueue.top();
        eventQueue.pop();

        // Update current simulation time
        current_time = event->event_time;

        // Sample backlog for each node at this event
        for (auto node : nodes) {
            sampleBacklog(node);
        }

        // Update progress bar (based on arrivalsSoFar / totalGeneratedPackets)
        int progress = static_cast<int>(
            (static_cast<double>(arrivalsSoFar) / totalGeneratedPackets) * 100.0
        );
        if (progress != lastProgress) {
            lastProgress = progress;
            printProgressBar(progress, barWidth);
        }

        // Handle the event (arrival or departure)
        if (event->type == EventType::ARRIVAL) {
            handleArrival(event);
        } else if (event->type == EventType::DEPARTURE) {
            handleDeparture(event);
        }

        // Clean up this event
        delete event;
    }

    // 4) Clear any leftover events
    while (!eventQueue.empty()) {
        Event* ev = eventQueue.top();
        eventQueue.pop();
        delete ev;
    }

    // 5) Finalize the progress bar
    printProgressBar(100, barWidth);
    std::cout << "\n";

    // 6) Produce final stats/report
    writeDetailedReport(*this, dateString);
}

// Handle arrival events
void SimulationEngine::handleArrival(Event* event)
{
    TrafficSource* ts = event->source;
    if (!ts) return;
    
    double arrivalTime = event->event_time;
    
    Packet* pkt = ts->generatePacket(arrivalTime);
    
    if (pkt) {
        arrivalsSoFar++;
        if (arrivalsSoFar > totalGeneratedPackets) {
            delete pkt;
            return;
        }
        
        Node* node = getNodeById(event->node_id);
        NodeStats &stats = nodeStatsMap[node->id];
        stats.totalArrivals++;
        stats.packetsIn++;

        if (pkt->is_reference) {
            referenceStats.totalReferenceArrivals++;
        }

        // Try to enqueue
        bool success = node->enqueuePacket(pkt);
        if (!success) {
            stats.totalDropped++;
            // Check which queue was intended
            switch (pkt->priority) {
            case Priority::PREMIUM:     stats.droppedPremium++; break;
            case Priority::ASSURED:     stats.droppedAssured++; break;
            case Priority::BEST_EFFORT: stats.droppedBestEffort++; break;
            }
            if (pkt->is_reference) {
                referenceStats.totalReferenceDropped++;
            }
            delete pkt;
        } else {
            int totalQueueSize = node->premium_queue.size() +
                                 node->assured_queue.size() +
                                 node->best_effort_queue.size();
            if (totalQueueSize == 1) {
                double transmission_time = (pkt->size * 8.0) / node->transmission_rate;
                double departureTime = arrivalTime + transmission_time;
                Event* depEvent = new Event(departureTime, EventType::DEPARTURE, nullptr, node->id);
                scheduleEvent(depEvent);
            }
        }
    }
    
    if (arrivalsSoFar < totalGeneratedPackets) {
        double interarrival = (ts->packet_size * 8.0) / (ts->peak_rate * 1000.0);
        double next_time = arrivalTime + interarrival;
        Event* nextArrival = new Event(next_time, EventType::ARRIVAL, nullptr, event->node_id, ts);
        scheduleEvent(nextArrival);
    }
}

void SimulationEngine::handleDeparture(Event* event)
{
    double departureTime = event->event_time;

    Node* node = getNodeById(event->node_id);
    if (!node) return;

    // Dequeue the departing packet
    Packet* departed = node->dequeuePacket();
    if (departed) {
        NodeStats& stats = nodeStatsMap[node->id];

        // Calculate delay as departure_time - arrival_time
        double delay = departureTime - departed->arrival_time;
        stats.totalDelay += delay;
        stats.totalDepartures++;
        stats.packetsOut++;

        // Per-queue delay stats
        switch (departed->priority) {
            case Priority::PREMIUM:
                stats.premiumDelaySum += delay;
                stats.premiumCount++;
                break;
            case Priority::ASSURED:
                stats.assuredDelaySum += delay;
                stats.assuredCount++;
                break;
            case Priority::BEST_EFFORT:
                stats.bestEffortDelaySum += delay;
                stats.bestEffortCount++;
                break;
            default:
                break;
        }
        
        if (departed->is_reference) {
            referenceStats.totalReferenceDelay += delay;
            referenceStats.totalReferenceDepartures++;
        }

        delete departed;
    }
    
    Packet* nextPkt = node->peekPacket();
    if (nextPkt) {
        double transmission_time = (nextPkt->size * 8.0) / node->transmission_rate;
        double nextDepTime = departureTime + transmission_time;
        Event* nextDep = new Event(nextDepTime, EventType::DEPARTURE, nullptr, node->id);
        scheduleEvent(nextDep);
    }
}