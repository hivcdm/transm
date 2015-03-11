#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <include.h>

#include "../utility/enum_iterator.h"
#include "utility/filesystem.h"
#include "../core/Constants.h"
#include "../utility/RandomNumberGenerator.h"

template<typename T>
struct Bounds
{
    T lower;
    T upper;

    /// <summary>
    /// Returns true if value is within these bounds (inclusive)
    /// </summary>
    bool Contains(T value)
    {
        return value >= lower && value <= upper;
    }
};

/// <summary>
/// this data structure contains some important simulation level parameters or variables
/// that are associated with each Population-level event
/// <summary>
/// <authors>
/// schung5, errhode
/// </authors>
class EventParams
{
public :
    struct TraceFile
    {
        enum class Type
        {
            Population,
            Infection,
            Partnership,
            Survival,
            CostEffectiveness,
            Clinical,
            Events,
            Health,
            SinglePerson,
            LifeExpectancy,
            PartnerAcquisition,
            CalibrationStatistics,
            ArtRollout,
            ShiftedOutcomes,
            Last,
            First = Population
        } type;
        bool enabled;
        std::string extension;
        bool toss;
        std::fstream file;

        template<typename T>
        std::ostream &operator<<(const T &to_add)
        {
            return file << to_add;
        }

        typedef std::ostream& (*ostream_manipulator)(std::ostream&);
        std::ostream& operator<<(ostream_manipulator pf)
        {
            return file << pf;
        }
    };

	/**
	this structure gives information about the rollout file to use and when to apply it if ART Rollout intervention is turned on
	*/
	class RolloutContext
	{
	public:
		int timeToApply;
		std::unique_ptr<SimContext> rolloutSimContext;
		//who to apply to 0=All Untreated 1=All Treated 2=Untreated Getting New ART -1=None
		int popOfInterest;
		RolloutContext(int t, std::unique_ptr<SimContext> context, int pop) : rolloutSimContext(std::move(context))
		{
			timeToApply = t;
			popOfInterest = pop;
		}
		~RolloutContext()
		{
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
		ConcurrencyDef(int min = -1, bool def = false)
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
		Bounds<int> cd4Bounds;

		int cd4OiHistRank;
        Bounds<int> cd4OiHistCd4Bounds;
		bool cd4OiHistOIs[Constants::NUMBER_OF_OIS];

		int hvlRank;
        Bounds<int> hvlBounds;

		int cd4HvlRank;
        Bounds<int> cd4HvlCd4Bounds;
        Bounds<int> cd4HvlHvlBounds;
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
		Bounds<double> steadyPrevBounds;
		int casualPrevPopulation;
        Bounds<double> casualPrevBounds;
		int CSWPrevPopulation;
        Bounds<double> CSWPrevBounds;
		int propInConcurrentPopulation;
        Bounds<double> propInConcurrentBounds;
		int numActsPopulation;
        Bounds<double> numActsBounds;
		double femaleCasualPrevRatio;
		double femalePropInConcurrentRatio;
		double femaleNumActsLRtoHRRatio;

		//The calendar prevalence values
		double calendarPrevs[Constants::NUMBER_CALIBRATION_PREVS];
		int saveStateTimePoints[Constants::NUMBER_TIME_POINTS_SAVE_STATE];
		double thresholdPrevMult;
	};

	EventParams()
	{
		currTime = 0;
		enableDynamicTreatmentScaling = false;
		dynamicFeedbackPeriod = 12;
		useRollout = false;
		untreatedContext = nullptr;
		treatedContext = nullptr;
        cepacRunStats = nullptr;
        cepacTracer = nullptr;
	}

    EventParams(const EventParams &) = delete;

    /// <summary>
	/// current internal clock for a particular Population
    /// </summary>
	int currTime;

    /// <summary>
	/// Sim name -- primarily used for generating names of GraphViz files and CEPAC output files; will be name of input sheet minus .xml
    /// </summary>
	std::string simName;

	int monthOf1990;

	//--------- CEPAC related objects -------------//
	//CEPAC related simContext (input)
	std::vector<SimContext *> cepacSimContexts;

	//CEPAC input files for Rollout
    std::vector<RolloutContext *> rolloutSimContexts;
	RolloutEligibility rolloutEligibility;
	//Cepac files for storing current population groups (only if using rollout)
	SimContext *untreatedContext;
	SimContext *treatedContext;

	//If we are using rollout use the cepac files specified in the ART rollout section
	bool useRollout;

	//timesToSwitchSimContext[0] should always be 0 by default (?)
	int timesToSwitchSimContext[Constants::NUMBER_OF_CEPAC_FILES];

	inline bool itIsTimeToSwitchSimContext()
	{
		if(useRollout)
		{
			return false;
		}

		for(int i = 0; i < Constants::NUMBER_OF_CEPAC_FILES; i++)
		{
			//Switching doesn't occur until 1 month later
			if(currTime == timesToSwitchSimContext[i] + 1)
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

    std::map<TraceFile::Type, TraceFile> trace_files;

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
	std::array<ConcurrencyDef, Constants::NUMBER_CONCURRENCY_DEFS> concurrencyDef;

	//prints BatchStats files for each of up to five variables as determined by user input
    std::map<BatchStatsVariables, std::fstream> BatchStatsStream;

	std::map<int, double> targetYearlyRolloutProportions;

	inline void displayOut(const std::string &message)
	{
		messageCallback(message);
	}

	std::function<void(const std::string &)> messageCallback;
	DebugLevel debugLevel;		//determines how much output is printed to the traces
	RandomNumberGenerator randomNums;		//random number generator that is used throughout the simulation

	bool enableDynamicTreatmentScaling;
	int dynamicFeedbackPeriod;

	//closes all the trace files
	~EventParams()
	{
		for(int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
		{
			popStateStream[i].close();
		}

		//For batchStats and summaryStats, print a new line character (all stats are on one line in these files)
        for(auto batchstat : enum_iterator<BatchStatsVariables>())
		{
			if(BatchStatsStream[batchstat].is_open())
			{
				BatchStatsStream[batchstat] << std::endl;
			}

			BatchStatsStream[batchstat].close();
		}

		delete cepacRunStats;

		while(cepacSimContexts.size() > 0)
		{
			SimContext *sc = cepacSimContexts.back();
			cepacSimContexts.pop_back();
			delete sc;
		}

		while(rolloutSimContexts.size() > 0)
		{
			RolloutContext *sc = rolloutSimContexts.back();
			rolloutSimContexts.pop_back();
			delete sc;
		}

        for (auto cf : cepac_file_context_map_)
        {
            delete cf.second;
        }

		delete cepacTracer;
	}

    SimContext *LoadCepacContext(const std::string &cepac_file)
    {
        if (cepac_file_context_map_.find(cepac_file)
            == cepac_file_context_map_.end())
        {
            displayOut("\tReading " + cepac_file + "\n");

            //Set the CEPAC simContext from the specified CEPAC .in file
            auto stem = transm::path(cepac_file).stem().string();
            cepac_file_context_map_[cepac_file] = new SimContext(stem);

            //Read in the inputs
            try
            {
                cepac_file_context_map_[cepac_file]->readInputs();
            }
            catch (std::string errorString)
            {
                throw std::runtime_error(errorString);
            }
        }

        return cepac_file_context_map_[cepac_file];
    }

    std::unordered_map<std::string, SimContext *> cepac_file_context_map_;
};
