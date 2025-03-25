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
    std::string fullpath = "../output/" + date + "/" + engine.outputFilename;

    std::ofstream out(fullpath);
    if (!out.is_open()) {
        std::cerr << "Could not open " << fullpath << " for writing.\n";
        return;
    }

    // 1) Basic info about the simulation
    out << "SPQ system\n";
    out << "Number of packets: " << engine.totalGeneratedPackets << "\n";

    // 2) Timestamp
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

    // 4) Scenario info
    out << "Number of nodes: " << engine.numNodes << "\n";
    out << "Number of audio sources: " << engine.numAudio << "\n";
    out << "Number of video sources: " << engine.numVideo << "\n";
    out << "Number of data sources: " << engine.numData << "\n";
    out << "Size of SPQ: " << engine.queueSize << "\n";

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
            ? (stats.totalDelay / stats.totalDepartures)
            : 0.0;

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
    for (auto &entry : engine.nodeStatsMap) {
        int nodeId = entry.first;
        auto &stats = entry.second;

        // Premium
        int premArrivals = stats.premiumCount + stats.droppedPremium;
        double premBlockRatio = (premArrivals > 0)
            ? double(stats.droppedPremium) / premArrivals
            : 0.0;

        // Assured
        int assArrivals = stats.assuredCount + stats.droppedAssured;
        double assBlockRatio = (assArrivals > 0)
            ? double(stats.droppedAssured) / assArrivals
            : 0.0;

        // Best-effort
        int bestArrivals = stats.bestEffortCount + stats.droppedBestEffort;
        double bestBlockRatio = (bestArrivals > 0)
            ? double(stats.droppedBestEffort) / bestArrivals
            : 0.0;

        out << "Node " << nodeId << ":\n"
            << " Premium block ratio: " << premBlockRatio << "\n"
            << " Assured block ratio: " << assBlockRatio << "\n"
            << " Best-effort block ratio: " << bestBlockRatio << "\n";
    }
    out << "\n";

    // -----------
    // Then show the overall blocking ratio (existing code)
    // -----------
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

    double premiumBlockRatioAll = (totalPremArrivals > 0.0)
        ? (totalPremDropped / totalPremArrivals) : 0.0;
    double assuredBlockRatioAll = (totalAssArrivals  > 0.0)
        ? (totalAssDropped  / totalAssArrivals ) : 0.0;
    double bestBlockRatioAll    = (totalBestArrivals > 0.0)
        ? (totalBestDropped / totalBestArrivals) : 0.0;

    out << "Overall (all nodes) blocking ratio:\n"
        << " Premium queue: "   << premiumBlockRatioAll << "\n"
        << " Assured queue: "   << assuredBlockRatioAll << "\n"
        << " Best-effort queue: " << bestBlockRatioAll << "\n\n";

    // (c) Average backlog
    out << "(c) Average number of backlogged packets at each priority queue (per node)\n";
    for (auto &entry : engine.nodeStatsMap) {
        int nodeId = entry.first;
        auto &stats = entry.second;

        // If no backlogSamples, can't compute average
        double avgPrem = 0.0, avgAss = 0.0, avgBest = 0.0;
        if (stats.backlogSamples > 0) {
            avgPrem = stats.cumulativeBacklogPremium / stats.backlogSamples;
            avgAss  = stats.cumulativeBacklogAssured / stats.backlogSamples;
            avgBest = stats.cumulativeBacklogBestEffort / stats.backlogSamples;
        }

        out << "Node " << nodeId << ":\n"
            << " Premium backlog: " << avgPrem << "\n"
            << " Assured backlog: " << avgAss << "\n"
            << " Best-effort backlog: " << avgBest << "\n";
    }
    out << "\n";
    
    // overall backlog
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

    out << "Overall (all nodes) average backlog:\n"
        << " Premium queue: " << avgPremBack << "\n"
        << " Assured queue: " << avgAssBack  << "\n"
        << " Best-effort queue: " << avgBestBack << "\n\n";

    // (d) End-to-end reference traffic delay
    double refDelay = 0.0;
    if (engine.referenceStats.totalReferenceDepartures > 0) {
        refDelay = engine.referenceStats.totalReferenceDelay
                 / engine.referenceStats.totalReferenceDepartures;
    }
    out << "(d) Average end-to-end packet delay for reference traffic\n";
    out << refDelay << "\n\n";

    // (e) Overall packet blocking ratio for reference traffic
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
