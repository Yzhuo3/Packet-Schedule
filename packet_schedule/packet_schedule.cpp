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

const int BASE_NUM_AUDIO = 5; // Base number of audio sources
const int BASE_NUM_VIDEO = 5; // Base number of video sources
const int BASE_NUM_DATA = 5; // Base number of data sources
// const double BASE_OFFERED_LOAD = 0.105;
const double TRANSMISSION_RATE = 10e6; // 10 Mbps

std::string generateOutputFilename(int scenario, double load)
{
    std::ostringstream oss;
    oss << "Scenario_" << scenario << "_output_" << load << ".txt";
    return oss.str();
}

static double averageRate_bps(double peak_kbps, double t_on, double t_off)
{
    // fraction of time ON
    double fracOn = t_on / (t_on + t_off);
    // average rate in kbps
    double avg_kbps = peak_kbps * fracOn;
    // convert kbps -> bits/sec
    return avg_kbps * 1000.0;
}

void runSimulationForLoad(int scenario,
                          double offeredLoad,
                          int M,
                          int combinedCapacity,
                          TrafficType refTrafficType,
                          int totalPackets,
                          const std::string& dateString,
                          int numAudio,
                          int numVideo,
                          int numData)
{
    // Define the ON/OFF times and peak rates for each traffic type
    double t_on_a = 0.36, t_off_a = 0.64, r_a = 64.0;   // audio
    double t_on_v = 0.33, t_off_v = 0.73, r_v = 384.0;   // video
    double t_on_d = 0.35, t_off_d = 0.65, r_d = 256.0;   // data

    // For the reference flow:
    double t_on_r, t_off_r, r_r;
    if (refTrafficType == TrafficType::AUDIO)
    {
        t_on_r = 0.36;
        t_off_r = 0.64;
        r_r = 64.0;
    }
    else if (refTrafficType == TrafficType::VIDEO)
    {
        t_on_r = 0.33;
        t_off_r = 0.73;
        r_r = 384.0;
    }
    else if (refTrafficType == TrafficType::DATA)
    {
        t_on_r = 0.35;
        t_off_r = 0.65;
        r_r = 256.0;
    }
    else
    {
        // fallback
        t_on_r = 0.36;
        t_off_r = 0.64;
        r_r = 64.0;
    }

    // Compute average rates
    double audioRate_bps = averageRate_bps(r_a, t_on_a, t_off_a);
    double videoRate_bps = averageRate_bps(r_v, t_on_v, t_off_v);
    double dataRate_bps  = averageRate_bps(r_d, t_on_d, t_off_d);
    double refRate_bps   = averageRate_bps(r_r, t_on_r, t_off_r);

    // total arrival rate lambda = sum of background + reference
    double baseLambda = (numAudio * audioRate_bps)
                      + (numVideo * videoRate_bps)
                      + (numData  * dataRate_bps)
                      + refRate_bps;

    // Compute the base offered
    double computedBaseRho = baseLambda / TRANSMISSION_RATE;

    // compute scaling factor
    double scale = offeredLoad / computedBaseRho;

    // Update the source counts
    numAudio = std::max(1, (int)std::round(numAudio * scale));
    numVideo = std::max(1, (int)std::round(numVideo * scale));
    numData  = std::max(1, (int)std::round(numData * scale));
    
    std::cout << "\n--- Running Simulation for Offered Load = " << offeredLoad
              << ", M = " << M << ", K = " << combinedCapacity
              << ", totalPackets = " << totalPackets
              << ", Reference Traffic = ";
    
    switch (refTrafficType)
    {
    case TrafficType::AUDIO: std::cout << "Audio-like";
        break;
    case TrafficType::VIDEO: std::cout << "Video-like";
        break;
    case TrafficType::DATA: std::cout << "Data-like";
        break;
    default: std::cout << "Unknown";
        break;
    }
    std::cout << " ---\n"
        << "   #Audio=" << numAudio
        << ", #Video=" << numVideo
        << ", #Data=" << numData << "\n";

    std::string outFile = generateOutputFilename(scenario, offeredLoad);
    SimulationEngine engine(
        outFile,
        M,
        numAudio,
        numVideo,
        numData,
        combinedCapacity,
        refTrafficType,
        totalPackets
    );

    // Create M nodes
    std::vector<Node*> nodes;
    nodes.reserve(M);
    for (int i = 1; i <= M; ++i)
    {
        Node* node = new Node(i, combinedCapacity, TRANSMISSION_RATE);
        engine.addNode(node);
        nodes.push_back(node);
    }

    // add background traffic sources for each node
    for (int i = 1; i <= M; ++i)
    {
        // Audio
        for (int j = 0; j < numAudio; ++j)
        {
            TrafficSource* ts = new TrafficSource(TrafficType::AUDIO, false,
                                                  64.0, 0.36, 0.64, 120);
            engine.addTrafficSource(ts, i);
        }
        // Video
        for (int j = 0; j < numVideo; ++j)
        {
            TrafficSource* ts = new TrafficSource(TrafficType::VIDEO, false,
                                                  384.0, 0.33, 0.73, 1000);
            engine.addTrafficSource(ts, i);
        }
        // Data
        for (int j = 0; j < numData; ++j)
        {
            TrafficSource* ts = new TrafficSource(TrafficType::DATA, false,
                                                  256.0, 0.35, 0.65, 583);
            engine.addTrafficSource(ts, i);
        }
    }

    // reference flow
    TrafficSource* refSource = nullptr;
    if (refTrafficType == TrafficType::AUDIO)
    {
        refSource = new TrafficSource(TrafficType::AUDIO, true,
                                      64.0, 0.36, 0.64, 120);
    }
    else if (refTrafficType == TrafficType::VIDEO)
    {
        refSource = new TrafficSource(TrafficType::VIDEO, true,
                                      384.0, 0.33, 0.73, 1000);
    }
    else if (refTrafficType == TrafficType::DATA)
    {
        refSource = new TrafficSource(TrafficType::DATA, true,
                                      256.0, 0.35, 0.65, 583);
    }
    else
    {
        // fallback
        refSource = new TrafficSource(TrafficType::AUDIO, true,
                                      64.0, 0.36, 0.64, 120);
    }
    engine.addTrafficSource(refSource, 1);

    engine.run(dateString);

    // Cleanup
    for (Node* node : nodes)
    {
        delete node;
    }
}

void runSimulationForLoad(int scenario,
                          double offeredLoad,
                          int M,
                          int combinedCapacity,
                          TrafficType refTrafficType,
                          int totalPackets,
                          const std::string& dateString
)
{
    runSimulationForLoad(scenario, offeredLoad, M, combinedCapacity, refTrafficType, totalPackets, dateString,
                         BASE_NUM_AUDIO, BASE_NUM_VIDEO, BASE_NUM_DATA);
}

// Scenario 1: infinite buffer, M=5, reference=Audio-like
void runScenario1(double loadStart, double loadEnd, double loadIncrement,
                  const std::string& output, int totalPackets)
{
    int M = 5;
    int combinedCapacity = 1000000;
    TrafficType refType = TrafficType::AUDIO;

    std::cout << "\n====== Scenario 1: M=5, K=∞, Audio-like reference ======\n";
    for (double load = loadStart; load <= loadEnd; load += loadIncrement)
    {
        runSimulationForLoad(1, load, M, combinedCapacity, refType, totalPackets, output);
    }
}

// Scenario 2: finite buffer (K=100), M=5, reference=Audio-like
void runScenario2(double loadStart, double loadEnd, double loadIncrement,
                  const std::string& output, int totalPackets)
{
    int M = 5;
    int combinedCapacity = 100;
    TrafficType refType = TrafficType::AUDIO;
    std::cout << "\n====== Scenario 2: M=5, K=100, Audio-like reference ======\n";
    for (double load = loadStart; load <= loadEnd; load += loadIncrement)
    {
        runSimulationForLoad(2, load, M, combinedCapacity, refType, totalPackets, output);
    }
}

// Scenario 3: finite buffer (K=100), M=10, reference=Audio-like
void runScenario3(double loadStart, double loadEnd, double loadIncrement,
                  const std::string& output, int totalPackets)
{
    int M = 10;
    int combinedCapacity = 100;
    TrafficType refType = TrafficType::AUDIO;

    std::cout << "\n====== Scenario 3: Finite Buffer (K=100), M=10, Reference=Audio-like ======\n";
    for (double load = loadStart; load <= loadEnd; load += loadIncrement)
    {
        runSimulationForLoad(3, load, M, combinedCapacity, refType, totalPackets, output);
    }
}

// Scenario 4: finite buffer (K=100), M=5, reference=Video-like
void runScenario4(double loadStart, double loadEnd, double loadIncrement,
                  const std::string& output, int totalPackets)
{
    int M = 5;
    int combinedCapacity = 100;
    TrafficType refType = TrafficType::VIDEO;

    std::cout << "\n====== Scenario 4: Finite Buffer (K=100), M=5, Reference=Video-like ======\n";
    for (double load = loadStart; load <= loadEnd; load += loadIncrement)
    {
        runSimulationForLoad(4, load, M, combinedCapacity, refType, totalPackets, output);
    }
}

// Scenario 5: finite buffer (K=100), M=5, reference=Data-like
void runScenario5(double loadStart, double loadEnd, double loadIncrement,
                  const std::string& output, int totalPackets)
{
    int M = 5;
    int combinedCapacity = 100;
    TrafficType refType = TrafficType::DATA;

    std::cout << "\n====== Scenario 5: Finite Buffer (K=100), M=5, Reference=Data-like ======\n";
    for (double load = loadStart; load <= loadEnd; load += loadIncrement)
    {
        runSimulationForLoad(5, load, M, combinedCapacity, refType, totalPackets, output);
    }
}

// Custom Scenario: finite buffer (K=100), M=10, reference=Data-like, Sources: audio: 45, video: 40, data: 35
void runScenario6(double loadStart, double loadEnd, double loadIncrement,
                  const std::string& output, int totalPackets)
{
    int M = 10;
    int combinedCapacity = 100;
    TrafficType refType = TrafficType::DATA;

    std::cout <<
        "\n====== Custom Scenario: Finite Buffer (K=100), M=10, Reference=Data-like, Sources: audio: 45, video: 40, data: 35 ======\n";
    for (double load = loadStart; load <= loadEnd; load += loadIncrement)
    {
        runSimulationForLoad(6, load, M, combinedCapacity, refType, totalPackets, output, 45, 40, 35);
    }
}

int main(int argc, char* argv[])
{
    double loadStart = 0.1;
    double loadEnd = 0.9;
    double loadIncrement = 0.1;
    int scenario = 0;
    int totalPackets = 10000000;

    // CLI
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "-l" && i + 1 < argc)
        {
            loadStart = std::stod(argv[++i]);
        }
        else if (arg == "-u" && i + 1 < argc)
        {
            loadEnd = std::stod(argv[++i]);
        }
        else if (arg == "-i" && i + 1 < argc)
        {
            loadIncrement = std::stod(argv[++i]);
        }
        // else if (arg == "-s" && i + 1 < argc) {
        //     scenario = std::stoi(argv[++i]);
        // }
        // else if (arg == "-N" && i + 1 < argc) {
        //     totalPackets = std::stoi(argv[++i]); 
        // }
        else if (arg == "-h" || arg == "--help")
        {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                // << "  -N <totalPackets>     Number of total packets to process (default=1000000)\n"
                // << "  -s <scenario>         Scenario # (1-5). 0 => run all.\n"
                << "  -l <load_start>       Starting offered load (default=0.1)\n"
                << "  -u <load_end>         Ending offered load (default=0.9)\n"
                << "  -i <load_increment>   Load increment (default=0.1)\n";
            return 0;
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << "\n";
            return 1;
        }
    }

    std::cout << "Enter scenario [1-6, 0=all, 6=additional, default=0]: ";
    {
        std::string line;
        if (std::getline(std::cin, line))
        {
            if (!line.empty())
            {
                scenario = std::stoi(line);
            }
        }
    }

    std::cout << "Enter totalPackets [default=10m]: ";
    {
        std::string line;
        if (std::getline(std::cin, line))
        {
            if (!line.empty())
            {
                totalPackets = std::stoi(line);
            }
        }
    }

    std::cout << "Starting Simulation Scenarios...\n"
        << "Total Packets: " << totalPackets << "\n"
        << "Offered Load Range: " << loadStart << " to " << loadEnd
        << " (increment: " << loadIncrement << ")\n";

    // Make a timestamped folder
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

    if (scenario == 0)
    {
        runScenario1(loadStart, loadEnd, loadIncrement, buf, totalPackets);
        runScenario2(loadStart, loadEnd, loadIncrement, buf, totalPackets);
        runScenario3(loadStart, loadEnd, loadIncrement, buf, totalPackets);
        runScenario4(loadStart, loadEnd, loadIncrement, buf, totalPackets);
        runScenario5(loadStart, loadEnd, loadIncrement, buf, totalPackets);
        runScenario6(loadStart, loadEnd, loadIncrement, buf, totalPackets);
    }
    else
    {
        switch (scenario)
        {
        case 1:
            runScenario1(loadStart, loadEnd, loadIncrement, buf, totalPackets);
            break;
        case 2:
            runScenario2(loadStart, loadEnd, loadIncrement, buf, totalPackets);
            break;
        case 3:
            runScenario3(loadStart, loadEnd, loadIncrement, buf, totalPackets);
            break;
        case 4:
            runScenario4(loadStart, loadEnd, loadIncrement, buf, totalPackets);
            break;
        case 5:
            runScenario5(loadStart, loadEnd, loadIncrement, buf, totalPackets);
            break;
        case 6:
            runScenario6(loadStart, loadEnd, loadIncrement, buf, totalPackets);
            break;
        default:
            std::cerr << "Invalid scenario number. Please choose between 1 and 5.\n";
            return 1;
        }
    }

    std::cout << "\nAll scenarios completed.\n";
    return 0;
}
