#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <include.h>

#include "concurrencydefinition.hpp"
#include "core/constants.hpp"
#include "utility/enum_iterator.hpp"
#include "utility/filesystem.hpp"
#include "utility/randomnumbergenerator.hpp"
#include "utility/time.hpp"

namespace transm {

/// <summary>
/// this data structure contains some important simulation level parameters or variables
/// that are associated with each Population-level event
/// <summary>
class EventParams
{
public:
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
        std::ofstream file;

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

	EventParams()
	{
		enableDynamicTreatmentScaling = false;
		dynamicFeedbackPeriod = 12;
		useRollout = false;
		untreatedContext = nullptr;
		treatedContext = nullptr;
		cepacRunStats = nullptr;
		cepacCostStats = nullptr;
		cepacTracer = nullptr;
		numNewbornsTraced = 0;
		numNewbornsToTrace = 0;
	}

    /// <summary>
	/// current internal clock for a particular Population
    /// </summary>
	Time currTime;

    /// <summary>
	/// Sim name -- primarily used for generating names of GraphViz files and CEPAC output files; will be name of input sheet minus .xml
    /// </summary>
	std::string simName;

	Time monthOf1990;

	//CEPAC related simContext (input)
	std::vector<SimContext *> cepacSimContexts;

	//CEPAC input files for Rollout
	std::vector<RolloutContext *> rolloutSimContexts;
	RolloutEligibility rolloutEligibility;
	RolloutDenominator rolloutProportionDenominator;

	//Cepac files for storing current population groups (only if using rollout)
	SimContext *untreatedContext;
	SimContext *treatedContext;

	//If we are using rollout use the cepac files specified in the ART rollout section
	bool useRollout;

	//timesToSwitchSimContext[0] should always be 0 by default (?)
	std::vector<Time> timesToSwitchSimContext;

	inline bool itIsTimeToSwitchSimContext()
	{
		if(useRollout)
		{
			return false;
		}

		for (auto time : timesToSwitchSimContext)
		{
			//Switching doesn't occur until 1 month later
			if(currTime == time + TimeSpan::Month)
			{
				return true;
			}
		}

		return false;
	}

	//CEPAC related runStats (output)
	RunStats *cepacRunStats;
        // CEPAC cost Tracer
        CostStats *cepacCostStats;
	//CEPAC tracing object (output)
	Tracer *cepacTracer;

	//calibration inputs
	CalibrationInputs calibrationInputs;
	//prevalence delay time
	Time delayPrevalence;

    std::map<TraceFile::Type, TraceFile> trace_files;

	//number of patients per initial age range to be followed
	int numToTrace;
	//number of newborns to trace after specified month
	int numNewbornsToTrace;
	Time monthTraceNewborns;
	//keeps track of how many newborns have been traced
	int numNewbornsTraced;

	bool tracePrevalentCases;

	//Concurrency Definitions
	std::array<ConcurrencyDef, Constants::NumberConcurrencyDefs> concurrencyDef;

	std::map<int, double> targetYearlyRolloutProportions;

	//random number generator that is used throughout the simulation
	RandomNumberGenerator randomNums;

	bool enableDynamicTreatmentScaling;
	int dynamicFeedbackPeriod;

	//closes all the trace files
	~EventParams()
	{
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

} // namespace transm
