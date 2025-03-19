#ifndef SIMULATIONENGINE_H
#define SIMULATIONENGINE_H

#include "Event.hpp"
#include "Node.hpp"
#include "TrafficSource.hpp"
#include <queue>
#include <vector>
#include <map>

// Comparator for ordering events in the priority queue (earliest event_time first)
struct EventComparator {
    bool operator()(Event* e1, Event* e2) const {
        return e1->event_time > e2->event_time;
    }
};

class SimulationEngine {
public:
    SimulationEngine(double end_time);
    ~SimulationEngine();

    // Add a node into the simulation
    void addNode(Node* node);

    // Associate a traffic source with a node (by node id)
    void addTrafficSource(TrafficSource* ts, int node_id);

    // Schedule an event into the event queue
    void scheduleEvent(Event* event);

    // Run the simulation until the simulation clock reaches end_time
    void run();

private:
    double current_time;
    double end_time;
    std::priority_queue<Event*, std::vector<Event*>, EventComparator> eventQueue;

    // Container for nodes (each node is assumed to have a unique id)
    std::vector<Node*> nodes;

    // Container for traffic sources paired with the id of the node that will receive their packets
    std::vector<std::pair<TrafficSource*, int>> trafficSources;

    // Helper function: find a node by its id
    Node* getNodeById(int id);

    // -------------------------------
    // Statistics Collection Structures
    // -------------------------------
    struct NodeStats {
        int totalArrivals;
        int totalDepartures;
        int totalDropped;
        double totalDelay;
        int droppedPremium;
        int droppedAssured;
        int droppedBestEffort;
        double cumulativeBacklogPremium;
        double cumulativeBacklogAssured;
        double cumulativeBacklogBestEffort;
        int backlogSamples;
    };

    struct ReferenceStats {
        int totalReferenceArrivals;
        int totalReferenceDepartures;
        int totalReferenceDropped;
        double totalReferenceDelay;
    };

    std::map<int, NodeStats> nodeStatsMap;
    ReferenceStats referenceStats;

    // Helper function to sample backlog for a given node at the current time.
    void sampleBacklog(Node* node);

    // Print the collected statistics at the end of the simulation.
    void printStatistics();
};

#endif // SIMULATIONENGINE_H
