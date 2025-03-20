#ifndef SIMULATIONENGINE_H
#define SIMULATIONENGINE_H

#include "Event.hpp"
#include "Node.hpp"
#include "TrafficSource.hpp"
#include <string>
#include <queue>
#include <vector>
#include <map>

// Comparator for ordering events in the priority queue (earliest event_time first)
struct EventComparator
{
    bool operator()(Event* e1, Event* e2) const
    {
        return e1->event_time > e2->event_time;
    }
};

// This class simulates an SPQ network with multiple nodes and traffic sources.
class SimulationEngine
{
public:
    /**
     * @brief Construct a new SimulationEngine
     * 
     * @param end_time The simulation end time in seconds
     * @param outputFilename Name of the text file to write final detailed results (e.g. "5_5_4_20250306_211535.txt")
     * @param M Number of nodes
     * @param numAudioSources Number of audio sources per node
     * @param numVideoSources Number of video sources per node
     * @param numDataSources Number of data sources per node
     * @param spqSize Combined capacity of SPQ queues at each node
     * @param refType Reference traffic type (AUDIO, VIDEO, or DATA)
     * @param totalPackets Approx. total generated packets (used for logging)
     */
    SimulationEngine(double end_time,
                    const std::string& outputFilename,
                    int M,
                    int numAudioSources,
                    int numVideoSources,
                    int numDataSources,
                    int spqSize,
                    TrafficType refType,
                    int totalPackets);

    ~SimulationEngine();

    // Add a node into the simulation
    void addNode(Node* node);

    // Associate a traffic source with a node (by node id)
    void addTrafficSource(TrafficSource* ts, int node_id);

    // Schedule an event into the event queue
    void scheduleEvent(Event* event);

    // Run the simulation until the simulation clock reaches end_time
    // Only a progress bar is shown on the console.
    void run(std::string output);

private:
    // ---------- Simulation Parameters ------------
    double current_time;          ///< The current simulation time
    double end_time;             ///< The simulation end time
    std::string outputFilename;  ///< The text file name for final results

    int numNodes;                ///< Number of nodes (M)
    int numAudio;                ///< Number of audio sources
    int numVideo;                ///< Number of video sources
    int numData;                 ///< Number of data sources
    int queueSize;               ///< Combined SPQ capacity (K)
    TrafficType referenceType;   ///< Reference traffic type (AUDIO, VIDEO, DATA)
    int totalGeneratedPackets;   ///< Approx. total number of packets generated (for logging)

    // ---------- Data Structures ------------
    std::priority_queue<Event*, std::vector<Event*>, EventComparator> eventQueue;
    std::vector<Node*> nodes;      ///< Each node has a unique ID
    std::vector<std::pair<TrafficSource*, int>> trafficSources; ///< (source, node_id) pairs

    // Helper: find a node by its ID
    Node* getNodeById(int id);

    // ---------- Statistics ------------
    struct NodeStats
    {
        // Basic stats
        int totalArrivals;
        int totalDepartures;
        int totalDropped;
        double totalDelay;  ///< Sum of packet delays at this node

        // Dropped counts by queue
        int droppedPremium;
        int droppedAssured;
        int droppedBestEffort;

        // Backlog sampling
        double cumulativeBacklogPremium;
        double cumulativeBacklogAssured;
        double cumulativeBacklogBestEffort;
        int backlogSamples;

        // Per-queue delay sums (if you want queue-specific average delays)
        double premiumDelaySum;
        double assuredDelaySum;
        double bestEffortDelaySum;
        int premiumCount;
        int assuredCount;
        int bestEffortCount;

        // Node in/out counters (packetsIn, packetsOut)
        long long packetsIn;
        long long packetsOut;
    };

    struct ReferenceStats
    {
        int totalReferenceArrivals;
        int totalReferenceDepartures;
        int totalReferenceDropped;
        double totalReferenceDelay;
    };

    std::map<int, NodeStats> nodeStatsMap;
    ReferenceStats referenceStats;

    // ---------- Private Methods ------------
    // Sample backlog for each node's queues
    void sampleBacklog(Node* node);

    // Event handling
    void handleArrival(Event* event);
    void handleDeparture(Event* event);

    // Print an ASCII progress bar on the console
    void printProgressBar(int progress, int barWidth);

    // Write the final, detailed text report in the style of your sample
    void writeDetailedReport(std::string date );

    void exportStatisticsCSV(const std::string& csvFilename,
                         int scenarioNumber,
                         double load);
};

#endif // SIMULATIONENGINE_H
