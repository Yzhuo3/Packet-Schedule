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
SimulationEngine::SimulationEngine(
    double end_time,
    const std::string& outputFilename,
    int M,                    // number of nodes
    int numAudioSources,
    int numVideoSources,
    int numDataSources,
    int spqSize,
    TrafficType refType,
    int totalPackets
)
  : current_time(0.0),
    end_time(end_time),
    outputFilename(outputFilename),
    numNodes(M),
    numAudio(numAudioSources),
    numVideo(numVideoSources),
    numData(numDataSources),
    queueSize(spqSize),
    referenceType(refType),
    totalGeneratedPackets(totalPackets)
{
    // Initialize reference stats
    referenceStats.totalReferenceArrivals = 0;
    referenceStats.totalReferenceDepartures = 0;
    referenceStats.totalReferenceDropped = 0;
    referenceStats.totalReferenceDelay = 0.0;

    // Create the output directory if it doesn't exist
// #ifdef _WIN32
//     system("mkdir \"../output/\"");
// #else
//     system("mkdir -p ../output");
// #endif
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

// Main simulation loop
void SimulationEngine::run(std::string output)
{
    // Initialize arrival events for each traffic source
    for (auto& pair : trafficSources) {
        TrafficSource* ts = pair.first;
        int node_id = pair.second;
        Event* event = new Event(current_time, EventType::ARRIVAL, nullptr, node_id, ts);
        scheduleEvent(event);
    }

    // For a progress bar
    int lastProgress = -1;
    int barWidth = 50;

    // MAIN EVENT LOOP
    while (!eventQueue.empty()) {
        Event* event = eventQueue.top();
        // If next event is beyond end time, stop
        if (event->event_time >= end_time) {
            break;
        }
        eventQueue.pop();
        current_time = event->event_time;
        
        // Sample backlog for each node
        for (auto node : nodes) {
            sampleBacklog(node);
        }

        // Update progress bar
        int progress = static_cast<int>((current_time / end_time) * 100.0);
        if (progress != lastProgress) {
            lastProgress = progress;
            printProgressBar(progress, barWidth);
        }

        // Process the event
        if (event->type == EventType::ARRIVAL) {
            handleArrival(event);
        }
        else if (event->type == EventType::DEPARTURE) {
            handleDeparture(event);
        }

        delete event;
    }

    // Clear any leftover events
    while (!eventQueue.empty()) {
        Event* ev = eventQueue.top();
        eventQueue.pop();
        delete ev;
    }

    // Print progress
    printProgressBar(100, barWidth);
    std::cout << "\n";
    
    writeDetailedReport(*this, output);

    // or export CSV:
    // exportStatisticsCSV(*this, "DataSheet.csv", scenarioNumber, load);
}

// Handle arrival events
void SimulationEngine::handleArrival(Event* event)
{
    TrafficSource* ts = event->source;
    if (!ts) return;

    Packet* pkt = ts->generatePacket(current_time);
    double interarrival = (ts->packet_size * 8.0) / (ts->peak_rate * 1000.0);
    double next_time = (pkt ? current_time + interarrival : ts->next_state_change_time);

    Node* node = getNodeById(event->node_id);
    if (pkt) {
        NodeStats& stats = nodeStatsMap[node->id];

        stats.totalArrivals++;
        stats.packetsIn++; // count as "in"
        if (pkt->is_reference) {
            referenceStats.totalReferenceArrivals++;
        }

        // Enqueue
        bool success = node->enqueuePacket(pkt);
        if (!success) {
            // Dropped
            stats.totalDropped++;
            switch (pkt->priority) {
                case Priority::PREMIUM:     stats.droppedPremium++; break;
                case Priority::ASSURED:     stats.droppedAssured++; break;
                case Priority::BEST_EFFORT: stats.droppedBestEffort++; break;
                default: break;
            }
            if (pkt->is_reference) {
                referenceStats.totalReferenceDropped++;
            }
            delete pkt;
        } else {
            // If node was idle
            int totalQueueSize = node->premium_queue.size()
                               + node->assured_queue.size()
                               + node->best_effort_queue.size();
            if (totalQueueSize == 1) {
                double transmission_time = (pkt->size * 8.0) / node->transmission_rate;
                double departureTime = current_time + transmission_time;
                if (departureTime < end_time) {
                    Event* depEvent = new Event(departureTime, EventType::DEPARTURE, nullptr, node->id);
                    scheduleEvent(depEvent);
                }
            }
        }
    }

    // Schedule next arrival
    if (next_time < end_time) {
        Event* nextArrival = new Event(next_time, EventType::ARRIVAL, nullptr, event->node_id, ts);
        scheduleEvent(nextArrival);
    }
}

// Handle departure events
void SimulationEngine::handleDeparture(Event* event)
{
    Node* node = getNodeById(event->node_id);
    if (!node) return;

    Packet* departed = node->dequeuePacket();
    if (departed) {
        NodeStats& stats = nodeStatsMap[node->id];

        double delay = current_time - departed->arrival_time;
        stats.totalDelay += delay;
        stats.totalDepartures++;
        stats.packetsOut++;

        // Per-queue delay if you want breakdown
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

    // If more packets remain, schedule next departure
    Packet* nextPkt = node->peekPacket();
    if (nextPkt) {
        double transmission_time = (nextPkt->size * 8.0) / node->transmission_rate;
        double nextDepTime = current_time + transmission_time;
        if (nextDepTime < end_time) {
            Event* nextDep = new Event(nextDepTime, EventType::DEPARTURE, nullptr, node->id);
            scheduleEvent(nextDep);
        }
    }
}
