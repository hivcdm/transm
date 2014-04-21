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

	//Used to keep track of the number of CEPAC .in files (i.e. SimContext) there are
	static const int NUMBER_OF_CEPAC_FILES = 5;
	static const int NUMBER_OF_ROLLOUT_FILES = 13;
	static const int NUMBER_OF_OIS = 15;

	//Used to keep track of how many trace files there are
	static const int NUMBER_OF_TRACE_FILES = 14;

	//used to index the upper or lower bound cd4StrataRanges + hv1StrataRanges
	static const int LOWER = 0;
	static const int UPPER = 1;

	static const int NUMBER_CONCURRENCY_DEFS = 16;
	static const int NUMBER_CALIBRATION_PREVS = 13;
	static const int NUMBER_TIME_POINTS_SAVE_STATE = 2;

	static const int PREVALENT_INFECTION;
	//static const bool INCIDENT_INFECTION;

	static const bool REMOVE;
	static const bool DONT_REMOVE;

	static const bool CONSENT_IS_REQUIRED;
	static const bool CONSENT_NOT_REQUIRED;

	static const bool EXCLUDE_NULL_BINS;
	static const bool INCLUDE_NULL_BINS;

	static const bool SHOW_INFECTED;
	static const bool NO_SHOW_INFECTED;

	//these are used to indicate whether JIterators need to be deleted or not
	//used to distinguish between the internal iterator held by a class
	//  or a newly allocated and initialized iterator
	static const bool NEED_TO_DELETE;
	static const bool DO_NOT_DELETE;
};
