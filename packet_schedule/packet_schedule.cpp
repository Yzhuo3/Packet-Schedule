#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <cmath>
#include "include/Node.hpp"
#include "include/TrafficSource.hpp"
#include "include/SimulationEngine.hpp"

// Constants for simulation:
const int BASE_NUM_AUDIO = 4;   // Base number of audio sources at offered load ~0.105
const int BASE_NUM_VIDEO = 5;   // Base number of video sources
const int BASE_NUM_DATA  = 5;   // Base number of data sources
const double BASE_OFFERED_LOAD = 0.105;
const double TRANSMISSION_RATE = 10e6; // 10 Mbps

// Helper function that runs one simulation with the given parameters.
// It scales the number of sources according to the offered load and uses the
// provided simulation end time.
void runSimulationForLoad(double offeredLoad, int M, int combinedCapacity, TrafficType refTrafficType, double simEndTime) {
    // Scale the number of sources relative to the base offered load.
    double scale = offeredLoad / BASE_OFFERED_LOAD;
    int numAudio = std::max(1, static_cast<int>(std::round(BASE_NUM_AUDIO * scale)));
    int numVideo = std::max(1, static_cast<int>(std::round(BASE_NUM_VIDEO * scale)));
    int numData  = std::max(1, static_cast<int>(std::round(BASE_NUM_DATA  * scale)));
    
    std::cout << "\n--- Running Simulation for Offered Load = " << offeredLoad
              << ", M = " << M << ", K = " << combinedCapacity 
              << ", Reference Traffic = ";
    switch (refTrafficType) {
        case TrafficType::AUDIO:
            std::cout << "Audio";
            break;
        case TrafficType::VIDEO:
            std::cout << "Video";
            break;
        case TrafficType::DATA:
            std::cout << "Data";
            break;
        default:
            std::cout << "Unknown";
            break;
    }
    std::cout << " ---\n";
    
    // Create a simulation engine with the specified end time.
    SimulationEngine engine(simEndTime);
    
    // Create M nodes (node IDs 1 to M) and add them to the engine.
    std::vector<Node*> nodes;
    for (int i = 1; i <= M; ++i) {
        Node* node = new Node(i, combinedCapacity, TRANSMISSION_RATE);
        engine.addNode(node);
        nodes.push_back(node);
    }
    
    // For each node, add background traffic sources.
    for (int i = 1; i <= M; ++i) {
        // Add audio sources.
        for (int j = 0; j < numAudio; ++j) {
            // Audio parameters: peak rate 64 kbps, ON time 0.36, OFF time 0.64, packet size 120 bytes.
            TrafficSource* ts = new TrafficSource(TrafficType::AUDIO, 64.0, 0.36, 0.64, 120);
            engine.addTrafficSource(ts, i);
        }
        // Add video sources.
        for (int j = 0; j < numVideo; ++j) {
            // Video parameters: peak rate 384 kbps, ON time 0.33, OFF time 0.73, packet size 1000 bytes.
            TrafficSource* ts = new TrafficSource(TrafficType::VIDEO, 384.0, 0.33, 0.73, 1000);
            engine.addTrafficSource(ts, i);
        }
        // Add data sources.
        for (int j = 0; j < numData; ++j) {
            // Data parameters: peak rate 256 kbps, ON time 0.35, OFF time 0.65, packet size 583 bytes.
            TrafficSource* ts = new TrafficSource(TrafficType::DATA, 256.0, 0.35, 0.65, 583);
            engine.addTrafficSource(ts, i);
        }
    }
    
    // Add one reference traffic source to node 1.
    TrafficSource* refSource = nullptr;
    if (refTrafficType == TrafficType::AUDIO) {
        refSource = new TrafficSource(TrafficType::AUDIO, 64.0, 0.36, 0.64, 120);
    } else if (refTrafficType == TrafficType::VIDEO) {
        refSource = new TrafficSource(TrafficType::VIDEO, 384.0, 0.33, 0.73, 1000);
    } else if (refTrafficType == TrafficType::DATA) {
        refSource = new TrafficSource(TrafficType::DATA, 256.0, 0.35, 0.65, 583);
    } else {
        refSource = new TrafficSource(TrafficType::AUDIO, 64.0, 0.36, 0.64, 120);
    }
    engine.addTrafficSource(refSource, 1);
    
    // Run the simulation.
    engine.run();
    
    // Clean up: delete allocated nodes.
    for (Node* node : nodes) {
        delete node;
    }
    // Note: In a complete solution, ensure that all dynamically allocated TrafficSource objects are also deleted.
}

// Scenario 1: Infinite buffer (simulated with a very large K), M = 5, Reference Traffic = Audio.
void runScenario1(double simEndTime, double loadStart, double loadEnd, double loadIncrement) {
    int M = 5;
    int combinedCapacity = 1000000; // Use a very large number to approximate an infinite buffer.
    TrafficType refType = TrafficType::AUDIO;
    std::cout << "\n====== Scenario 1: Infinite Buffer, M = 5, Reference = Audio ======\n";
    for (double load = loadStart; load <= loadEnd; load += loadIncrement) {
        runSimulationForLoad(load, M, combinedCapacity, refType, simEndTime);
    }
}

// Scenario 2: Finite buffer (K = 100), M = 5, Reference Traffic = Audio.
void runScenario2(double simEndTime, double loadStart, double loadEnd, double loadIncrement) {
    int M = 5;
    int combinedCapacity = 100;
    TrafficType refType = TrafficType::AUDIO;
    std::cout << "\n====== Scenario 2: Finite Buffer (K = 100), M = 5, Reference = Audio ======\n";
    for (double load = loadStart; load <= loadEnd; load += loadIncrement) {
        runSimulationForLoad(load, M, combinedCapacity, refType, simEndTime);
    }
}

// Scenario 3: Finite buffer (K = 100), M = 10, Reference Traffic = Audio.
void runScenario3(double simEndTime, double loadStart, double loadEnd, double loadIncrement) {
    int M = 10;
    int combinedCapacity = 100;
    TrafficType refType = TrafficType::AUDIO;
    std::cout << "\n====== Scenario 3: Finite Buffer (K = 100), M = 10, Reference = Audio ======\n";
    for (double load = loadStart; load <= loadEnd; load += loadIncrement) {
        runSimulationForLoad(load, M, combinedCapacity, refType, simEndTime);
    }
}

// Scenario 4: Finite buffer (K = 100), M = 5, Reference Traffic = Video.
void runScenario4(double simEndTime, double loadStart, double loadEnd, double loadIncrement) {
    int M = 5;
    int combinedCapacity = 100;
    TrafficType refType = TrafficType::VIDEO;
    std::cout << "\n====== Scenario 4: Finite Buffer (K = 100), M = 5, Reference = Video ======\n";
    for (double load = loadStart; load <= loadEnd; load += loadIncrement) {
        runSimulationForLoad(load, M, combinedCapacity, refType, simEndTime);
    }
}

// Scenario 5: Finite buffer (K = 100), M = 5, Reference Traffic = Data.
void runScenario5(double simEndTime, double loadStart, double loadEnd, double loadIncrement) {
    int M = 5;
    int combinedCapacity = 100;
    TrafficType refType = TrafficType::DATA;
    std::cout << "\n====== Scenario 5: Finite Buffer (K = 100), M = 5, Reference = Data ======\n";
    for (double load = loadStart; load <= loadEnd; load += loadIncrement) {
        runSimulationForLoad(load, M, combinedCapacity, refType, simEndTime);
    }
}

int main(int argc, char* argv[]) {
    // Default simulation parameters.
    double simEndTime = 5.0;
    double loadStart = 0.1;
    double loadEnd = 0.1;
    double loadIncrement = 0.1;
    int scenario = 1;  // Default scenario.

    // Manually parse command-line arguments.
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-T" && i + 1 < argc) {
            simEndTime = std::stod(argv[++i]);
        } else if (arg == "-l" && i + 1 < argc) {
            loadStart = std::stod(argv[++i]);
        } else if (arg == "-u" && i + 1 < argc) {
            loadEnd = std::stod(argv[++i]);
        } else if (arg == "-i" && i + 1 < argc) {
            loadIncrement = std::stod(argv[++i]);
        } else if (arg == "-s" && i + 1 < argc) {
            scenario = std::stoi(argv[++i]);
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "  -T <sim_end_time>    Total simulation time in seconds (default: 5.0)\n"
                      << "  -l <load_start>      Starting offered load (default: 0.1)\n"
                      << "  -u <load_end>        Ending offered load (default: 0.9)\n"
                      << "  -i <load_increment>  Offered load increment (default: 0.1)\n"
                      << "  -s <scenario>        Scenario number to run (1-5). 0 means run all scenarios.\n";
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return 1;
        }
    }

    // Seed the random number generator.
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    std::cout << "Starting Simulation Scenarios...\n"
              << "Simulation End Time: " << simEndTime << " seconds\n"
              << "Offered Load Range: " << loadStart << " to " << loadEnd 
              << " (increment: " << loadIncrement << ")\n";

    // Run the selected scenario(s)
    if (scenario == 0) {
        runScenario1(simEndTime, loadStart, loadEnd, loadIncrement);
        runScenario2(simEndTime, loadStart, loadEnd, loadIncrement);
        runScenario3(simEndTime, loadStart, loadEnd, loadIncrement);
        runScenario4(simEndTime, loadStart, loadEnd, loadIncrement);
        runScenario5(simEndTime, loadStart, loadEnd, loadIncrement);
    } else {
        switch (scenario) {
            case 1:
                runScenario1(simEndTime, loadStart, loadEnd, loadIncrement);
                break;
            case 2:
                runScenario2(simEndTime, loadStart, loadEnd, loadIncrement);
                break;
            case 3:
                runScenario3(simEndTime, loadStart, loadEnd, loadIncrement);
                break;
            case 4:
                runScenario4(simEndTime, loadStart, loadEnd, loadIncrement);
                break;
            case 5:
                runScenario5(simEndTime, loadStart, loadEnd, loadIncrement);
                break;
            default:
                std::cerr << "Invalid scenario number. Please choose between 1 and 5.\n";
                return 1;
        }
    }
    
    std::cout << "\nAll scenarios completed.\n";
    return 0;
}