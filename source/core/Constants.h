#pragma once

#include <string>
#include <map>

/// <summary>
/// used to set amount of debug output
/// </summary>
enum class DebugLevel
{
	/// <summary>
	/// no debug output printed
	/// </summary>
	Zero,
	/// <summary>
	/// for each timestep, print the sizes of the different demographic buckets within the Population
	/// </summary>
	One,
	/// <summary>
	/// for each timestep, print out One output plus all the individual entities that have been affected by each event
	/// </summary>
	Two,
	/// <summary>
	/// unused for now
	/// </summary>
	Three
};

/// <summary>
/// used to set popstats output variables
/// </summary>
enum class BatchStatsVariables
{
	PREVALENCE,
	PREVALENCESA,
	INCIDENCE,
	POPULATION,
	CURRENTLYINFECTED,
	NEWINFECTIONS,
	Last,
    First = PREVALENCE
};

/// <summary>
/// used to set the timestep length
/// </summary>
enum class TimeGranularity
{
	Day,
	Month,
	Year
};

class Constants
{
public:
	static const std::string ASTERISK;
	static const std::string BLANK;
	static const std::string COLON;
	static const std::string UNDERSCORE;
	static const std::string TAB;
	static const std::string TABTAB;
	static const std::string SPACE;

	static const std::map<BatchStatsVariables, std::string> BatchStatFileName;

	static const int NUMBER_OF_OIS = 15;
	static const int NUMBER_CONCURRENCY_DEFS = 16;
	static const int NUMBER_CALIBRATION_PREVS = 0; //13;
	static const int NUMBER_TIME_POINTS_SAVE_STATE = 0; //2;

	static const int PREVALENT_INFECTION = 0;
};
