#include "../include/SimulationEngine.hpp"
#include "../include/Event.hpp"
#include "../include/Packet.hpp"
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
    std::cout << "\n"; // end the line

    // Write the detailed result to a text file
    writeDetailedReport(std::move(output));
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
        if (pkt->type == PacketType::REFERENCE) {
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
            if (pkt->type == PacketType::REFERENCE) {
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

        if (departed->type == PacketType::REFERENCE) {
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

// Print an ASCII progress bar to console
void SimulationEngine::printProgressBar(int progress, int barWidth)
{
    std::cout << "\n\rProgress: [";
    int pos = barWidth * progress / 100;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << progress << " %";
    std::cout.flush();
}

// Write a detailed report file in the style you provided
void SimulationEngine::writeDetailedReport(std::string date )
{
    std::string fullpath = "../output/" + date + "/" + outputFilename;

    std::ofstream out(fullpath);
    if (!out.is_open()) {
        std::cerr << "Could not open " << fullpath << " for writing.\n";
        return;
    }

    out << "SPQ system\n";
    out << "Number of packets: " << totalGeneratedPackets << "\n";
    
    std::time_t now = std::time(nullptr);
    std::tm tmNow;

    localtime_s(&tmNow, &now);
    
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tmNow);
    out << "Timestamp: " << buf << "\n";

    out << "Time simulation ended at: " << std::fixed << std::setprecision(3)
        << current_time << " seconds\n";

    out << "Number of nodes: " << numNodes << "\n";
    out << "Number of audio sources: " << numAudio << "\n";
    out << "Number of video sources: " << numVideo << "\n";
    out << "Number of data sources: " << numData << "\n";
    out << "Size of SPQ: " << queueSize << "\n";

    out << "Reference packet type: ";
    switch (referenceType) {
        case TrafficType::AUDIO: out << "Audio\n"; break;
        case TrafficType::VIDEO: out << "Video\n"; break;
        case TrafficType::DATA:  out << "Data\n";  break;
        default:                 out << "Unknown\n"; break;
    }
    out << "\n";

    out << "(a) Average packet delay (waiting time) at each node\n";
    for (auto& entry : nodeStatsMap) {
        int nodeId = entry.first;
        NodeStats& stats = entry.second;

        double nodeAvgDelay = (stats.totalDepartures > 0)
            ? (stats.totalDelay / stats.totalDepartures) : 0.0;

        out << "Node " << nodeId << " average packet delay: "
            << nodeAvgDelay << " seconds\n";

        double premiumAvg = (stats.premiumCount > 0)
            ? (stats.premiumDelaySum / stats.premiumCount) : 0.0;
        double assuredAvg = (stats.assuredCount > 0)
            ? (stats.assuredDelaySum / stats.assuredCount) : 0.0;
        double bestAvg = (stats.bestEffortCount > 0)
            ? (stats.bestEffortDelaySum / stats.bestEffortCount) : 0.0;

        out << " Premium queue delay: " << premiumAvg << " seconds\n";
        out << " Assured queue delay: " << assuredAvg << " seconds\n";
        out << " Best-effort queue delay: " << bestAvg << " seconds\n";
    }
    out << "\n";

    double totalPremDropped = 0, totalPremArrivals = 0;
    double totalAssDropped = 0, totalAssArrivals = 0;
    double totalBestDropped = 0, totalBestArrivals = 0;

    for (auto& entry : nodeStatsMap) {
        NodeStats& stats = entry.second;

        totalPremDropped += stats.droppedPremium;
        totalAssDropped  += stats.droppedAssured;
        totalBestDropped += stats.droppedBestEffort;

        totalPremArrivals += stats.premiumCount + stats.droppedPremium;
        totalAssArrivals  += stats.assuredCount + stats.droppedAssured;
        totalBestArrivals += stats.bestEffortCount + stats.droppedBestEffort;
    }
    double premBlock = (totalPremArrivals > 0) ? (totalPremDropped / totalPremArrivals) : 0.0;
    double assBlock  = (totalAssArrivals > 0)  ? (totalAssDropped  / totalAssArrivals)  : 0.0;
    double bestBlock = (totalBestArrivals > 0) ? (totalBestDropped / totalBestArrivals) : 0.0;

    out << "(b) Average packet blocking ratio at each priority queue\n";
    out << "Premium queue: " << premBlock << "\n";
    out << "Assured queue: " << assBlock << "\n";
    out << "Best-effort queue: " << bestBlock << "\n\n";

    double totalPremBack = 0.0, totalAssBack = 0.0, totalBestBack = 0.0;
    int totalSamples = 0;
    for (auto& entry : nodeStatsMap) {
        NodeStats& stats = entry.second;
        totalPremBack += stats.cumulativeBacklogPremium;
        totalAssBack  += stats.cumulativeBacklogAssured;
        totalBestBack += stats.cumulativeBacklogBestEffort;
        totalSamples  += stats.backlogSamples;
    }
    double avgPremBack = (totalSamples > 0) ? (totalPremBack / totalSamples) : 0.0;
    double avgAssBack  = (totalSamples > 0) ? (totalAssBack  / totalSamples) : 0.0;
    double avgBestBack = (totalSamples > 0) ? (totalBestBack / totalSamples) : 0.0;

    out << "(c) Average number of backlogged packets at each priority queue\n";
    out << "Premium queue: " << avgPremBack << "\n";
    out << "Assured queue: " << avgAssBack << "\n";
    out << "Best-effort queue: " << avgBestBack << "\n\n";

    double refDelay = (referenceStats.totalReferenceDepartures > 0)
        ? (referenceStats.totalReferenceDelay / referenceStats.totalReferenceDepartures) : 0.0;
    out << "(d) Average end-to-end packet delay for reference traffic\n";
    out << refDelay << "\n\n";

    double refBlock = 0.0;
    if (referenceStats.totalReferenceArrivals > 0) {
        refBlock = (double)referenceStats.totalReferenceDropped / referenceStats.totalReferenceArrivals;
    }
    out << "(e) Overall packet blocking ratio for reference traffic\n";
    out << refBlock << "\n\n";

    for (auto& entry : nodeStatsMap) {
        int nodeId = entry.first;
        NodeStats& stats = entry.second;
        out << "Node " << nodeId << " packets into node: " << stats.packetsIn << "\n";
        out << "Node " << nodeId << " packets out of node: "  << stats.packetsOut << "\n";
    }
    out << "\n";

    long long totalDropped = 0;
    long long sumArrivals = 0;
    for (auto& entry : nodeStatsMap) {
        totalDropped += entry.second.totalDropped;
        sumArrivals  += entry.second.totalArrivals;
    }
    out << "Dropped packets: " << totalDropped << "\n";
    out << "Total generated packets: " << totalGeneratedPackets << "\n";

    long long sumDepartures = 0;
    for (auto& entry : nodeStatsMap) {
        sumDepartures += entry.second.totalDepartures;
    }
    // out << "Successfully transmitted packets: " << sumDepartures << "\n\n";

    out.close();
    // std::cout << "\nDetailed report saved to " << fullpath << "\n";
}

void SimulationEngine::exportStatisticsCSV(const std::string& csvFilename,
                                           int scenarioNumber,
                                           double load)
{
    // Open in append mode
    std::ofstream out(csvFilename, std::ios::app);
    if (!out.is_open()) {
        std::cerr << "Cannot open " << csvFilename << " for CSV output.\n";
        return;
    }

    // If file is empty, write a header row
    out.seekp(0, std::ios::end);
    if (out.tellp() == 0) {
        out << "Scenario,Load,Node,"
            << "AvgDelay,PremiumDelay,AssuredDelay,BestEffortDelay,"
            << "PremiumBlock,AssuredBlock,BestEffortBlock,"
            << "RefAvgDelay,RefBlock,"
            << "PacketsIn,PacketsOut"
            << "\n";
    }

    // Compute reference traffic stats once
    double refAvgDelay = 0.0;
    if (referenceStats.totalReferenceDepartures > 0) {
        refAvgDelay = referenceStats.totalReferenceDelay / referenceStats.totalReferenceDepartures;
    }
    double refBlockRatio = 0.0;
    if (referenceStats.totalReferenceArrivals > 0) {
        refBlockRatio = (double)referenceStats.totalReferenceDropped / referenceStats.totalReferenceArrivals;
    }

    // For each node, gather stats & write a row
    for (auto& entry : nodeStatsMap) {
        int nodeId = entry.first;
        NodeStats& stats = entry.second;

        // Node-level average delay
        double avgDelay = (stats.totalDepartures > 0)
            ? (stats.totalDelay / stats.totalDepartures) : 0.0;

        // Per-queue delays (if you track them)
        double premiumAvgDelay = (stats.premiumCount > 0)
            ? (stats.premiumDelaySum / stats.premiumCount) : 0.0;
        double assuredAvgDelay = (stats.assuredCount > 0)
            ? (stats.assuredDelaySum / stats.assuredCount) : 0.0;
        double bestEffortAvgDelay = (stats.bestEffortCount > 0)
            ? (stats.bestEffortDelaySum / stats.bestEffortCount) : 0.0;

        int premArrivals   = stats.premiumCount   + stats.droppedPremium;
        int assuredArrivals= stats.assuredCount   + stats.droppedAssured;
        int bestArrivals   = stats.bestEffortCount+ stats.droppedBestEffort;

        double premBlock = (premArrivals > 0) ? (double)stats.droppedPremium / premArrivals : 0.0;
        double assBlock  = (assuredArrivals > 0) ? (double)stats.droppedAssured / assuredArrivals : 0.0;
        double bestBlock = (bestArrivals > 0) ? (double)stats.droppedBestEffort / bestArrivals : 0.0;
        
        out << scenarioNumber << ","
            << load << ","
            << nodeId << ","
            << std::fixed << std::setprecision(6) << avgDelay << ","
            << premiumAvgDelay << ","
            << assuredAvgDelay << ","
            << bestEffortAvgDelay << ","
            << premBlock << ","
            << assBlock << ","
            << bestBlock << ","
            << refAvgDelay << ","
            << refBlockRatio << ","
            << stats.packetsIn << ","
            << stats.packetsOut
            << "\n";
    }

    out.close();
    // std::cout << "Appended CSV data to " << csvFilename << "\n";
}