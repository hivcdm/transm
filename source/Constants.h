#pragma once
#include <string>
using namespace std;

/***
This file contains simulation-wide constants or enum values
****/

//used to set amount of debug output
enum DebugLevel {
	DEBUG0,			//no debug output printed
	DEBUG1,			//for each timestep, print the sizes of the different demographic buckets within the Population
	DEBUG2,			//for each timestep, print out DEBUG1 output plus all the individual entities that have been affected by each event
	DEBUG3
};

//used to set popstats output variables
enum BatchStatsVariables {
	PREVALENCE,
	PREVALENCESA,
	INCIDENCE,
	POPULATION,
	CURRENTLYINFECTED,
	NEWINFECTIONS,
	ENDBatchStatsVariables
};

//if program unexpectedly exits, one of these following codes will be provided
enum ExitCodes {
	INVALID_DEATH_CODE,
	INVALID_GENDER,
	INVALID_TIME_GRANULARITY,
	INVALID_ENTITYPOOL_INDEX,
	INVALID_ENTITY_CLASSIFIER,
	INVALID_CEPAC_FILE,
	ExitCodesOTHER,
};

//used to set the timestep length
enum TimeGranularity{
	DAY,
	MONTH,
	YEAR,
	ENDTimeGranularity
};

class Constants {
public:

	//we use this in asserts where virtual methods are incorrectly called...
	//   we should be calling the method implemented by the children
	//   if we run into this, then check why the parent method is being called...
	static const bool SHOULD_NOT_BE_CALLING_ME;

	//this will be used in conjunction w/ asserts to find any loose ends that we did not tie up.
	//set to false to trigger asserts
	static const bool TODO;
	static const bool TODO_DEF;		//definitely need to to do, but we use this when we just need to get some part to run for the time being.
	static const bool TODO_LO_PRI;

	//----------< Begin String Constants >------------------//
	static const string ASTERISK;
	static const string BLANK;
	static const string COLON;
	static const string UNDERSCORE;
	static const string TAB;
	static const string TABTAB;

	static const string BatchStatFileName[ENDBatchStatsVariables];
	//----------< End String Constants >------------------//

	//Used to keep track of the number of CEPAC .in files (i.e. SimContext) there are
	static const int NUMBER_OF_CEPAC_FILES = 5;
	static const int NUMBER_OF_ROLLOUT_FILES=13;
	static const int NUMBER_OF_OIS=15;

	//Used to keep track of how many trace files there are
	static const int NUMBER_OF_TRACE_FILES = 12;

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

	static bool const SHOW_INFECTED;
	static bool const NO_SHOW_INFECTED;


	//these are used to indicate whether JIterators need to be deleted or not
	//used to distinguish between the internal iterator held by a class
	//  or a newly allocated and initialized iterator
	static const bool NEED_TO_DELETE;
	static const bool DO_NOT_DELETE;

};
