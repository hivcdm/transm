#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "../Constants.h"
#include "../cepac/include.h"
#include "../util/rand/RandomNums.h"

//these are found in Constants.h
enum DebugLevel;
enum TimeGranularity;

/**
this data structure contains some important simulation level parameters or variables
that are associated with each Population-level event

@author schung5, errhode
 */
class EventParams
{

private :

public :
	enum TraceFileType
	{
		Population,
		Infection,
		Partnership,
		Survival,
		CostEffectiveness,
		Clinical,
		Events,
		Health,
		Singleperson,
		LifeExpectancy,
		PartnershipAcquisition,
		CalibrationStatistics,
		ArtRollout,
		ShiftedOutcomes,
		Last,
		First = Population
	};

	/**
	this structure gives information about the rollout file to use and when to apply it if ART Rollout intervention is turned on
	*/
	class RolloutContext
	{
	public:
		int timeToApply;
		SimContext *rolloutSimContext;
		//who to apply to 0=All Untreated 1=All Treated 2=Untreated Getting New ART -1=None
		int popOfInterest;
		RolloutContext(int t, SimContext *context, int pop)
		{
			timeToApply = t;
			rolloutSimContext = context;
			popOfInterest = pop;
		}
		~RolloutContext()
		{
			delete rolloutSimContext;
		}
	};

	/**
	this structure stores the definintion fo concurrency
	*/
	class ConcurrencyDef
	{
	public:
		int minPartnershipsNeeded;
		bool useDefinition;
		ConcurrencyDef(int min, bool def)
		{
			minPartnershipsNeeded = min;
			useDefinition = def;
		}
	};

	/**
	this structure stores the eligibility criter used for art rollout
	*/
	class RolloutEligibility
	{
	public:
		int oiHistRank;
		bool oiHistOIs[Constants::NUMBER_OF_OIS];
		int oiHistNumToStart;

		int cd4Rank;
		int cd4Bounds[2];

		int cd4OiHistRank;
		int cd4OiHistCd4Bounds[2];
		bool cd4OiHistOIs[Constants::NUMBER_OF_OIS];

		int hvlRank;
		int hvlBounds[2];

		int cd4HvlRank;
		int cd4HvlCd4Bounds[2];
		int cd4HvlHvlBounds[2];
	};

	/**
	this structure stores the Inputs for calibration
	**/
	class CalibrationInputs
	{
	public:
		bool useCalibration;
		int monthOfCalibration;

		//outcomes used for the cutoff
		//The partnership prevalence is for a year duration
		int steadyPrevPopulation;
		double steadyPrevBounds[2];
		int casualPrevPopulation;
		double casualPrevBounds[2];
		int CSWPrevPopulation;
		double CSWPrevBounds[2];
		int propInConcurrentPopulation;
		double propInConcurrentBounds[2];
		int numActsPopulation;
		double numActsBounds[2];
		double femaleCasualPrevRatio;
		double femalePropInConcurrentRatio;
		double femaleNumActsLRtoHRRatio;

		//which files to toss
		bool tossFiles[Constants::NUMBER_OF_TRACE_FILES];

		//The calendar prevalence values
		double calendarPrevs[Constants::NUMBER_CALIBRATION_PREVS];
		int saveStateTimePoints[Constants::NUMBER_TIME_POINTS_SAVE_STATE];
		double thresholdPrevMult;
	};

	inline EventParams()
	{
		currTime = 0;
		genGraphViz = false;
		useRollout = false;
		untreatedContext = NULL;
		treatedContext = NULL;
	}

	//current internal clock for a particular Population
	long currTime;

	//Sim name -- primarily used for generating names of GraphViz files and CEPAC output files; will be name of input sheet minus .xml
	std::string simName;

	int monthOf1990;

	//--------- CEPAC related objects -------------//
	//CEPAC related simContext (input)
	vector<SimContext *> cepacSimContexts;

	//CEPAC input files for Rollout
	vector<RolloutContext *> rolloutSimContexts;
	RolloutEligibility rolloutEligibility;
	//Cepac files for storing current population groups (only if using rollout)
	SimContext *untreatedContext;
	SimContext *treatedContext;

	//If we are using rollout use the cepac files specified in the ART rollout section
	bool useRollout;

	//Use ceofficient method of vartiation (used for heterogeneity)
	bool useCoefficientVariation;
	double coefficientOfVariation;

	//timesToSwitchSimContext[0] should always be 0 by default (?)
	int timesToSwitchSimContext[Constants::NUMBER_OF_CEPAC_FILES];

	inline bool itIsTimeToSwitchSimContext()
	{
		if(this->useRollout)
		{
			return false;
		}

		for(int i = 0; i < Constants::NUMBER_OF_CEPAC_FILES; i++)
		{
			//Switching doesn't occur until 1 month later
			if(this->currTime == this->timesToSwitchSimContext[i] + 1)
			{
				return true;
			}
		}

		return false;
	}

	//CEPAC related runStats (output)
	RunStats *cepacRunStats;
	//CEPAC tracing object (output)
	Tracer *cepacTracer;
	//--------- CEPAC related objects -------------//

	//calibration inputs
	CalibrationInputs calibrationInputs;
	//prevalence delay time
	int delayPrevalence;

	//used to keep track of which trace files to output and their names
	bool outputTrace[Constants::NUMBER_OF_TRACE_FILES];
	string traceExtensions[Constants::NUMBER_OF_TRACE_FILES];

	std::fstream traceStreams[Constants::NUMBER_OF_TRACE_FILES];

	//Saves state of population to file
	std::fstream popStateStream[Constants::NUMBER_TIME_POINTS_SAVE_STATE];
	//number of patients per initial age range to be followed
	int numToTrace;
	//number of newborns to trace after specified month
	int numNewbornsToTrace;
	int monthTraceNewborns;
	//keeps track of how many newborns have been traced
	int numNewbornsTraced;

	bool tracePrevalentCases;

	//Concurrency Definitions
	ConcurrencyDef *concurrencyDef[Constants::NUMBER_CONCURRENCY_DEFS];
	//prints BatchStats files for each of up to five variables as determined by user input
	std::fstream BatchStatsStream[ENDBatchStatsVariables];

	std::vector<double> targetYearlyRolloutProportions;

	double interpolateMonthlyRolloutProportion()
	{
		int monthOf2002 = monthOf1990 + 12 * 12;

		if(currTime < monthOf2002)
		{
			return 0;
		}
		else
		{
			int yearRelativeTo2002 = (currTime - monthOf2002) / 12;

			if(yearRelativeTo2002 < static_cast<int>(targetYearlyRolloutProportions.size() - 1))
			{
				double currentYearTargetProportion = targetYearlyRolloutProportions[yearRelativeTo2002];
				double nextYearTargetProportion = targetYearlyRolloutProportions[yearRelativeTo2002 + 1];
				double x = ((currTime - monthOf1990) % 12) / 12.0;
				return currentYearTargetProportion + (nextYearTargetProportion - currentYearTargetProportion) * x;
			}
			else
			{
				return targetYearlyRolloutProportions.back();
			}
		}
	}

	inline void displayOut(const std::string &message)
	{
		messageCallback(message);
	}

	std::function<void(const std::string &)> messageCallback;
	DebugLevel debugLevel;		//determines how much output is printed to the traces
	RandomNums randomNums;		//random number generator that is used throughout the simulation

	bool genGraphViz;			//will generate GraphViz output files if true

	//closes all the trace files
	inline void close()
	{
		for(int i = 0; i < Constants::NUMBER_OF_TRACE_FILES; i++)
		{
			if(this->outputTrace[i])
			{
				this->traceStreams[i].close();
			}
		}

		for(int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
		{
			this->popStateStream[i].close();
		}

		//For batchStats and summaryStats, print a new line character (all stats are on one line in these files)
		for(BatchStatsVariables batchstat = BatchStatsVariables(0); batchstat < ENDBatchStatsVariables;
		        batchstat = BatchStatsVariables(batchstat + 1))
		{
			if(this->BatchStatsStream[batchstat].is_open())
			{
				this->BatchStatsStream[batchstat] << std::endl;
			}

			this->BatchStatsStream[batchstat].close();
		}

		//this->cepacTracer->closeTraceFile();
		delete this->cepacRunStats;

		while(this->cepacSimContexts.size() > 0)
		{
			SimContext *sc = this->cepacSimContexts.back();
			this->cepacSimContexts.pop_back();
			delete sc;
		}

		while(this->rolloutSimContexts.size() > 0)
		{
			RolloutContext *sc = this->rolloutSimContexts.back();
			this->rolloutSimContexts.pop_back();
			delete sc;
		}

		delete this->cepacTracer;

		for(int i = 0; i < Constants::NUMBER_CONCURRENCY_DEFS; i++)
		{
			delete this->concurrencyDef[i];
		}
	}
};
