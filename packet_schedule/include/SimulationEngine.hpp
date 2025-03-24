#ifndef SIMULATIONENGINE_H
#define SIMULATIONENGINE_H

#include "Event.hpp"
#include "Node.hpp"
#include "TrafficSource.hpp"
#include <string>
#include <queue>
#include <vector>
#include <map>

struct EventComparator {
    bool operator()(Event* e1, Event* e2) const {
        return e1->event_time > e2->event_time;
    }
};

class SimulationEngine
{
public:
    double current_time;
    
    SimulationEngine(const std::string& outputFilename,
                     int M,
                     int numAudioSources,
                     int numVideoSources,
                     int numDataSources,
                     int spqSize,
                     TrafficType refType,
                     int totalPackets);

    ~SimulationEngine();

    void addNode(Node* node);
    void addTrafficSource(TrafficSource* ts, int node_id);
    void scheduleEvent(Event* event);
    
    void run(const std::string &dateString);
    
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

    // scenario info
    std::string outputFilename;
    int numNodes;
    int numAudio;
    int numVideo;
    int numData;
    int queueSize;
    TrafficType referenceType;
    int totalGeneratedPackets;
    long long arrivalsSoFar;

private:
    std::priority_queue<Event*, std::vector<Event*>, EventComparator> eventQueue;
    std::vector<Node*> nodes;
    std::vector<std::pair<TrafficSource*, int>> trafficSources;

    Node* getNodeById(int id);

    // sampleBacklog etc.
    void sampleBacklog(Node* node);

    // handleArrival, handleDeparture
    void handleArrival(Event* event);
    void handleDeparture(Event* event);
};

#endif // SIMULATIONENGINE_H
