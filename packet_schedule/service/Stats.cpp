#include "../include/Stats.hpp"
#include "../include/SimulationEngine.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <ctime>

void printProgressBar(int progress, int barWidth)
{
    std::cout << "\rProgress: [";
    int pos = barWidth * progress / 100;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << progress << " %";
    std::cout.flush();
}

void writeDetailedReport(SimulationEngine &engine, const std::string &date)
{
    // Build the path to the final text file
    // e.g. "../output/20250321_145004/Scenario_2_output_0.9.txt"
    std::string fullpath = "../output/" + date + "/" + engine.outputFilename;

    std::ofstream out(fullpath);
    if (!out.is_open()) {
        std::cerr << "Could not open " << fullpath << " for writing.\n";
        return;
    }

    // Print header lines
    out << "SPQ system\n";
    out << "Number of packets: " << engine.totalGeneratedPackets << "\n";

    // Timestamp
    std::time_t now = std::time(nullptr);
    char buf[32];
#ifdef _WIN32
    std::tm tmNow;
    localtime_s(&tmNow, &now);
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tmNow);
#else
    std::tm* tmNow = std::localtime(&now);
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", tmNow);
#endif
    out << "Timestamp: " << buf << "\n";

    out << std::fixed << std::setprecision(3);
    out << "Time simulation ended at: " << engine.current_time << " seconds\n";

    // Basic scenario info
    out << "Number of nodes: " << engine.numNodes << "\n";
    out << "Number of audio sources: " << engine.numAudio << "\n";
    out << "Number of video sources: " << engine.numVideo << "\n";
    out << "Number of data sources: " << engine.numData << "\n";
    out << "Size of SPQ: " << engine.queueSize << "\n";

    // If your scenario logic picks "refTrafficType" to indicate which queue
    // reference flows mimic, you can label them accordingly:
    out << "Reference packet type: ";
    switch (engine.referenceType) {
        case TrafficType::AUDIO: out << "Audio-like"; break;
        case TrafficType::VIDEO: out << "Video-like"; break;
        case TrafficType::DATA:  out << "Data-like";  break;
        default:                 out << "Unknown";    break;
    }
    out << "\n\n";

    // (a) Node-level average delays
    out << "(a) Average packet delay (waiting time) at each node\n";
    for (auto &entry : engine.nodeStatsMap) {
        int nodeId = entry.first;
        auto &stats = entry.second;

        double nodeAvgDelay = (stats.totalDepartures > 0)
            ? (stats.totalDelay / stats.totalDepartures) : 0.0;

        out << "Node " << nodeId << " average packet delay: "
            << nodeAvgDelay << " seconds\n";

        double premiumAvg = (stats.premiumCount > 0)
            ? (stats.premiumDelaySum / stats.premiumCount) : 0.0;
        double assuredAvg = (stats.assuredCount > 0)
            ? (stats.assuredDelaySum / stats.assuredCount) : 0.0;
        double bestAvg   = (stats.bestEffortCount > 0)
            ? (stats.bestEffortDelaySum / stats.bestEffortCount) : 0.0;

        out << " Premium queue delay: "     << premiumAvg << " seconds\n";
        out << " Assured queue delay: "     << assuredAvg << " seconds\n";
        out << " Best-effort queue delay: " << bestAvg   << " seconds\n";
    }
    out << "\n";

    // (b) Average packet blocking ratio
    double totalPremDropped = 0.0, totalPremArrivals = 0.0;
    double totalAssDropped  = 0.0, totalAssArrivals  = 0.0;
    double totalBestDropped = 0.0, totalBestArrivals = 0.0;

    for (auto &entry : engine.nodeStatsMap) {
        auto &stats = entry.second;

        totalPremDropped  += stats.droppedPremium;
        totalPremArrivals += (stats.premiumCount + stats.droppedPremium);

        totalAssDropped   += stats.droppedAssured;
        totalAssArrivals  += (stats.assuredCount + stats.droppedAssured);

        totalBestDropped  += stats.droppedBestEffort;
        totalBestArrivals += (stats.bestEffortCount + stats.droppedBestEffort);
    }

    double premiumBlockRatio = (totalPremArrivals > 0.0)
        ? (totalPremDropped / totalPremArrivals) : 0.0;
    double assuredBlockRatio = (totalAssArrivals  > 0.0)
        ? (totalAssDropped  / totalAssArrivals ) : 0.0;
    double bestBlockRatio    = (totalBestArrivals > 0.0)
        ? (totalBestDropped / totalBestArrivals) : 0.0;

    out << "(b) Average packet blocking ratio at each priority queue\n";
    out << "Premium queue: "   << premiumBlockRatio << "\n";
    out << "Assured queue: "   << assuredBlockRatio << "\n";
    out << "Best-effort queue: " << bestBlockRatio << "\n\n";

    // (c) Average backlog
    double sumPremBack = 0.0, sumAssBack = 0.0, sumBestBack = 0.0;
    int totalSamples = 0;
    for (auto &entry : engine.nodeStatsMap) {
        auto &stats = entry.second;
        sumPremBack += stats.cumulativeBacklogPremium;
        sumAssBack  += stats.cumulativeBacklogAssured;
        sumBestBack += stats.cumulativeBacklogBestEffort;
        totalSamples += stats.backlogSamples;
    }

    double avgPremBack = (totalSamples > 0) ? (sumPremBack / totalSamples) : 0.0;
    double avgAssBack  = (totalSamples > 0) ? (sumAssBack  / totalSamples) : 0.0;
    double avgBestBack = (totalSamples > 0) ? (sumBestBack / totalSamples) : 0.0;

    out << "(c) Average number of backlogged packets at each priority queue\n";
    out << "Premium queue: "    << avgPremBack << "\n";
    out << "Assured queue: "    << avgAssBack  << "\n";
    out << "Best-effort queue: " << avgBestBack << "\n\n";

    // (d) End-to-end reference traffic delay
    // This is now incremented if (pkt->is_reference == true) in handleDeparture
    double refDelay = 0.0;
    if (engine.referenceStats.totalReferenceDepartures > 0) {
        refDelay = engine.referenceStats.totalReferenceDelay
                 / engine.referenceStats.totalReferenceDepartures;
    }
    out << "(d) Average end-to-end packet delay for reference traffic\n";
    out << refDelay << "\n\n";

    // (e) Overall packet blocking ratio for reference traffic
    // Also incremented if (pkt->is_reference) in handleArrival drop logic
    double refBlock = 0.0;
    if (engine.referenceStats.totalReferenceArrivals > 0) {
        refBlock = double(engine.referenceStats.totalReferenceDropped)
                 / engine.referenceStats.totalReferenceArrivals;
    }
    out << "(e) Overall packet blocking ratio for reference traffic\n";
    out << refBlock << "\n\n";

    // Node-level stats
    for (auto &entry : engine.nodeStatsMap) {
        int nodeId = entry.first;
        auto &stats = entry.second;
        out << "Node " << nodeId << " packets into node: " << stats.packetsIn << "\n";
        out << "Node " << nodeId << " packets out of node: "  << stats.packetsOut << "\n";
    }
    out << "\n";

    // e.g. total dropped, total departures
    long long totalDropped = 0, sumDepartures = 0;
    for (auto &entry : engine.nodeStatsMap) {
        totalDropped   += entry.second.totalDropped;
        sumDepartures  += entry.second.totalDepartures;
    }
    out << "Dropped packets: " << totalDropped << "\n";
    out << "Total generated packets: " << engine.totalGeneratedPackets << "\n";
    out << "Successfully transmitted packets: " << sumDepartures << "\n";

    out.close();
    std::cout << "\nDetailed report saved to " << fullpath << "\n";
}

/**
 * exportStatisticsCSV() - If you want to append each run’s data to a CSV file.
 * The logic is the same: no mention of TrafficType::REFERENCE.
 * We compute reference stats if (pkt->is_reference).
 */
void exportStatisticsCSV(
    SimulationEngine &engine,
    const std::string &csvFilename,
    int scenarioNumber,
    double load
)
{
    std::ofstream out(csvFilename, std::ios::app);
    if (!out.is_open()) {
        std::cerr << "Cannot open " << csvFilename << " for CSV output.\n";
        return;
    }

    // If file is empty, write a header row
    out.seekp(0, std::ios::end);
    if (out.tellp() == 0) {
        // Example columns
        out << "Scenario,Load,Node,AvgDelay,PremBlock,AssBlock,BestBlock,RefDelay,RefBlock\n";
    }

    // referenceStats for entire run
    double refAvgDelay = 0.0;
    if (engine.referenceStats.totalReferenceDepartures > 0) {
        refAvgDelay = engine.referenceStats.totalReferenceDelay /
                      engine.referenceStats.totalReferenceDepartures;
    }
    double refBlock = 0.0;
    if (engine.referenceStats.totalReferenceArrivals > 0) {
        refBlock = double(engine.referenceStats.totalReferenceDropped) /
                   engine.referenceStats.totalReferenceArrivals;
    }

    // For each node, compute per-queue blocking
    for (auto &entry : engine.nodeStatsMap) {
        int nodeId = entry.first;
        auto &stats = entry.second;

        double avgDelay = (stats.totalDepartures > 0)
            ? (stats.totalDelay / stats.totalDepartures)
            : 0.0;

        // Premium block
        int premArrivals = stats.premiumCount + stats.droppedPremium;
        double premBlock = (premArrivals > 0)
            ? double(stats.droppedPremium) / premArrivals
            : 0.0;

        // Assured block
        int assArrivals = stats.assuredCount + stats.droppedAssured;
        double assBlock = (assArrivals > 0)
            ? double(stats.droppedAssured) / assArrivals
            : 0.0;

        // Best-effort block
        int bestArrivals = stats.bestEffortCount + stats.droppedBestEffort;
        double bestBlock = (bestArrivals > 0)
            ? double(stats.droppedBestEffort) / bestArrivals
            : 0.0;

        // Write a row
        out << scenarioNumber << ","
            << load << ","
            << nodeId << ","
            << avgDelay << ","
            << premBlock << ","
            << assBlock << ","
            << bestBlock << ","
            << refAvgDelay << ","
            << refBlock
            << "\n";
    }

    out.close();
    std::cout << "Appended CSV data to " << csvFilename << "\n";
}
