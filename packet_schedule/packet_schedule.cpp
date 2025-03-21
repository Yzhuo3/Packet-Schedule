#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <cmath>
#include <sstream>
#include <iomanip>
#include "include/Node.hpp"
#include "include/TrafficSource.hpp"
#include "include/SimulationEngine.hpp"

// Constants for simulation:
const int BASE_NUM_AUDIO = 5;   // Base number of audio sources
const int BASE_NUM_VIDEO = 5;   // Base number of video sources
const int BASE_NUM_DATA  = 5;   // Base number of data sources
const double BASE_OFFERED_LOAD = 0.105;
const double TRANSMISSION_RATE = 10e6; // 10 Mbps

/**
 * @brief Generate a filename string based on scenario number and offered load.
 *        Example: "Scenario_4_output_0.5.txt"
 */
std::string generateOutputFilename(int scenario, double load)
{
    std::ostringstream oss;
    oss << "Scenario_" << scenario << "_output_" << load << ".txt";
    return oss.str();
}

/**
 * @brief Run one simulation with the specified parameters, building a SimulationEngine
 *        with the updated constructor that needs multiple arguments.
 *
 * This is where we create normal traffic sources (is_reference=false) and
 * one reference traffic source (is_reference=true).
 */
void runSimulationForLoad(int scenario,
                          double offeredLoad,
                          int M,
                          int combinedCapacity,
                          TrafficType refTrafficType,
                          double simEndTime,
                          const std::string& dateString)
{
    // Scale the number of background sources relative to the base offered load
    double scale = offeredLoad / BASE_OFFERED_LOAD;
    int numAudio = std::max(1, static_cast<int>(std::round(BASE_NUM_AUDIO * scale)));
    int numVideo = std::max(1, static_cast<int>(std::round(BASE_NUM_VIDEO * scale)));
    int numData  = std::max(1, static_cast<int>(std::round(BASE_NUM_DATA  * scale)));

    // We'll assume "totalPackets" is some large number or estimate
    int totalPackets = 1000000;

    // Build the output filename (like "Scenario_4_output_0.5.txt")
    std::string outFile = generateOutputFilename(scenario, offeredLoad);

    // Print some info to console
    std::cout << "\n--- Running Simulation for Offered Load = " << offeredLoad
              << ", M = " << M << ", K = " << combinedCapacity
              << ", Reference Traffic = ";

    // We'll interpret refTrafficType to decide the queue logic for the reference flow,
    // but the "is_reference" boolean will ensure it increments reference stats.
    switch (refTrafficType) {
    case TrafficType::AUDIO:
        std::cout << "Audio-like";
        break;
    case TrafficType::VIDEO:
        std::cout << "Video-like";
        break;
    case TrafficType::DATA:
        std::cout << "Data-like";
        break;
    default:
        std::cout << "Unknown";
        break;
    }
    std::cout << " ---\n";

    // Create the simulation engine
    SimulationEngine engine(simEndTime,
                            outFile,
                            M,
                            numAudio,
                            numVideo,
                            numData,
                            combinedCapacity,
                            refTrafficType,
                            totalPackets);

    // Create M nodes (1..M)
    std::vector<Node*> nodes;
    nodes.reserve(M);
    for (int i = 1; i <= M; ++i) {
        Node* node = new Node(i, combinedCapacity, TRANSMISSION_RATE);
        engine.addNode(node);
        nodes.push_back(node);
    }

    // For each node, add background traffic sources (is_reference=false)
    for (int i = 1; i <= M; ++i) {
        // Audio
        for (int j = 0; j < numAudio; ++j) {
            // (type=AUDIO, is_reference=false, peak_rate=64, etc.)
            TrafficSource* ts = new TrafficSource(TrafficType::AUDIO,
                                                  false,
                                                  64.0, 0.36, 0.64, 120);
            engine.addTrafficSource(ts, i);
        }
        // Video
        for (int j = 0; j < numVideo; ++j) {
            TrafficSource* ts = new TrafficSource(TrafficType::VIDEO,
                                                  false,
                                                  384.0, 0.33, 0.73, 1000);
            engine.addTrafficSource(ts, i);
        }
        // Data
        for (int j = 0; j < numData; ++j) {
            TrafficSource* ts = new TrafficSource(TrafficType::DATA,
                                                  false,
                                                  256.0, 0.35, 0.65, 583);
            engine.addTrafficSource(ts, i);
        }
    }

    // Create one reference traffic source at node 1
    // Mark it as reference => is_reference=true
    TrafficSource* refSource = nullptr;
    if (refTrafficType == TrafficType::AUDIO) {
        // Audio-like reference
        refSource = new TrafficSource(TrafficType::AUDIO,
                                      true,  // is_reference = true
                                      64.0, 0.36, 0.64, 120);
    }
    else if (refTrafficType == TrafficType::VIDEO) {
        // Video-like reference
        refSource = new TrafficSource(TrafficType::VIDEO,
                                      true,
                                      384.0, 0.33, 0.73, 1000);
    }
    else if (refTrafficType == TrafficType::DATA) {
        // Data-like reference
        refSource = new TrafficSource(TrafficType::DATA,
                                      true,
                                      256.0, 0.35, 0.65, 583);
    }
    else {
        // Fallback: audio-like reference
        refSource = new TrafficSource(TrafficType::AUDIO,
                                      true,
                                      64.0, 0.36, 0.64, 120);
    }
    // Add that reference source to node 1
    engine.addTrafficSource(refSource, 1);

    // Run the simulation
    engine.run(dateString);

    // Clean up
    for (Node* node : nodes) {
        delete node;
    }
}

// Scenario 1: infinite buffer, M=5, reference=Audio-like
void runScenario1(double simEndTime, double loadStart, double loadEnd,
                  double loadIncrement, const std::string& output)
{
    int M = 5;
    int combinedCapacity = 1000000; // ~infinite
    // We'll treat reference flow as AUDIO
    TrafficType refType = TrafficType::AUDIO;

    std::cout << "\n====== Scenario 1: Infinite Buffer (K=∞), M=5, Reference=Audio-like ======\n";
    for (double load = loadStart; load <= loadEnd; load += loadIncrement) {
        runSimulationForLoad(1, load, M, combinedCapacity, refType, simEndTime, output);
    }
}

// Scenario 2: finite buffer (K=100), M=5, reference=Audio-like
void runScenario2(double simEndTime, double loadStart, double loadEnd,
                  double loadIncrement, const std::string& output)
{
    int M = 5;
    int combinedCapacity = 100;
    TrafficType refType = TrafficType::AUDIO;

    std::cout << "\n====== Scenario 2: Finite Buffer (K=100), M=5, Reference=Audio-like ======\n";
    for (double load = loadStart; load <= loadEnd; load += loadIncrement) {
        runSimulationForLoad(2, load, M, combinedCapacity, refType, simEndTime, output);
    }
}

// Scenario 3: finite buffer (K=100), M=10, reference=Audio-like
void runScenario3(double simEndTime, double loadStart, double loadEnd,
                  double loadIncrement, const std::string& output)
{
    int M = 10;
    int combinedCapacity = 100;
    TrafficType refType = TrafficType::AUDIO;

    std::cout << "\n====== Scenario 3: Finite Buffer (K=100), M=10, Reference=Audio-like ======\n";
    for (double load = loadStart; load <= loadEnd; load += loadIncrement) {
        runSimulationForLoad(3, load, M, combinedCapacity, refType, simEndTime, output);
    }
}

// Scenario 4: finite buffer (K=100), M=5, reference=Video-like
void runScenario4(double simEndTime, double loadStart, double loadEnd,
                  double loadIncrement, const std::string& output)
{
    int M = 5;
    int combinedCapacity = 100;
    TrafficType refType = TrafficType::VIDEO;

    std::cout << "\n====== Scenario 4: Finite Buffer (K=100), M=5, Reference=Video-like ======\n";
    for (double load = loadStart; load <= loadEnd; load += loadIncrement) {
        runSimulationForLoad(4, load, M, combinedCapacity, refType, simEndTime, output);
    }
}

// Scenario 5: finite buffer (K=100), M=5, reference=Data-like
void runScenario5(double simEndTime, double loadStart, double loadEnd,
                  double loadIncrement, const std::string& output)
{
    int M = 5;
    int combinedCapacity = 100;
    TrafficType refType = TrafficType::DATA;

    std::cout << "\n====== Scenario 5: Finite Buffer (K=100), M=5, Reference=Data-like ======\n";
    for (double load = loadStart; load <= loadEnd; load += loadIncrement) {
        runSimulationForLoad(5, load, M, combinedCapacity, refType, simEndTime, output);
    }
}

/**
 * main():
 *  - Parse command line
 *  - Create a timestamped folder
 *  - Run the selected scenario(s)
 */
int main(int argc, char* argv[])
{
    double simEndTime   = 10.0;
    double loadStart    = 0.1;
    double loadEnd      = 0.9;
    double loadIncrement= 0.1;
    int scenario        = 0; // 0 => run all

    // Parse CLI
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-T" && i + 1 < argc) {
            simEndTime = std::stod(argv[++i]);
        }
        else if (arg == "-l" && i + 1 < argc) {
            loadStart = std::stod(argv[++i]);
        }
        else if (arg == "-u" && i + 1 < argc) {
            loadEnd = std::stod(argv[++i]);
        }
        else if (arg == "-i" && i + 1 < argc) {
            loadIncrement = std::stod(argv[++i]);
        }
        else if (arg == "-s" && i + 1 < argc) {
            scenario = std::stoi(argv[++i]);
        }
        else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "  -T <sim_end_time>    Total simulation time in seconds\n"
                      << "  -l <load_start>      Starting offered load\n"
                      << "  -u <load_end>        Ending offered load\n"
                      << "  -i <load_increment>  Offered load increment\n"
                      << "  -s <scenario>        Scenario number (1-5). 0 => run all.\n";
            return 0;
        }
        else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return 1;
        }
    }

    // Seed random
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    std::cout << "Starting Simulation Scenarios...\n"
              << "Simulation End Time: " << simEndTime << " seconds\n"
              << "Offered Load Range: " << loadStart << " to " << loadEnd
              << " (increment: " << loadIncrement << ")\n";

    // Build a timestamped folder name
    std::time_t now = std::time(nullptr);
    std::tm tmNow;
#ifdef _WIN32
    localtime_s(&tmNow, &now);
#else
    std::tm* tmp = std::localtime(&now);
    tmNow = *tmp;
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tmNow);

#ifdef _WIN32
    std::string command = "mkdir \"../output/" + std::string(buf) + "\"";
#else
    std::string command = "mkdir -p ../output/" + std::string(buf);
#endif
    system(command.c_str());

    if (scenario == 0) {
        runScenario1(simEndTime, loadStart, loadEnd, loadIncrement, buf);
        runScenario2(simEndTime, loadStart, loadEnd, loadIncrement, buf);
        runScenario3(simEndTime, loadStart, loadEnd, loadIncrement, buf);
        runScenario4(simEndTime, loadStart, loadEnd, loadIncrement, buf);
        runScenario5(simEndTime, loadStart, loadEnd, loadIncrement, buf);
    } else {
        switch (scenario) {
        case 1:
            runScenario1(simEndTime, loadStart, loadEnd, loadIncrement, buf);
            break;
        case 2:
            runScenario2(simEndTime, loadStart, loadEnd, loadIncrement, buf);
            break;
        case 3:
            runScenario3(simEndTime, loadStart, loadEnd, loadIncrement, buf);
            break;
        case 4:
            runScenario4(simEndTime, loadStart, loadEnd, loadIncrement, buf);
            break;
        case 5:
            runScenario5(simEndTime, loadStart, loadEnd, loadIncrement, buf);
            break;
        default:
            std::cerr << "Invalid scenario number. Please choose between 1 and 5.\n";
            return 1;
        }
    }

    std::cout << "\nAll scenarios completed.\n";
    return 0;
}
