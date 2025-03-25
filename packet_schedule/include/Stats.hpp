#ifndef STATS_HPP
#define STATS_HPP

#include <string>

/**
 * Forward declaration of SimulationEngine so we can reference it in function
 * signatures without including the entire SimulationEngine.hpp here.
 */
class SimulationEngine;

/**
 * @brief Print an ASCII progress bar to the console.
 * 
 * @param progress   Integer progress [0..100]
 * @param barWidth   Width of the ASCII bar
 */
void printProgressBar(int progress, int barWidth);

/**
 * @brief Write a detailed text report in the style of your sample file.
 * 
 * This function reads statistics from the given SimulationEngine instance
 * (nodeStats, referenceStats, etc.) and produces the desired text output file.
 * 
 * @param engine Reference to the SimulationEngine holding all stats
 * @param date   A string with date/time or folder name (if you pass it from main)
 */
void writeDetailedReport(SimulationEngine &engine, const std::string &date);

#endif // STATS_HPP
