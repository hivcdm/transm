#ifndef EVENTPARAMS_HPP
#define EVENTPARAMS_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <include.h>

#include "parameterdefinitions.hpp"
#include "core/constants.hpp"
#include "utility/enum_iterator.hpp"
#include "utility/filesystem.hpp"
#include "utility/randomnumbergenerator.hpp"
#include "utility/time.hpp"

namespace transm {

 /**
  * this data structure contains some important simulation level parameters or variables
  * that are associated with each Population-level event
  */
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
            SinglePerson,
            LifeExpectancy,
            PartnerAcquisition,
            PartnerNetwork,
            CalibrationStatistics,
            ArtRollout,
            PrepOutcomes,
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

    /** current internal clock for a particular Population */
	Time currTime;

    /** Sim name -- primarily used for generating names of GraphViz files and CEPAC output files; will be name of input sheet minus .xml */
	std::string simName;

	Time monthOf1990;

    /** \name CEPAC information */
    /*@{*/
    /** CEPAC related simContext (input) */
	std::vector<SimContext *> cepacSimContexts;

	/** CEPAC input files for ART Rollout */
	std::vector<RolloutContext *> rolloutSimContexts;
	RolloutEligibility rolloutEligibility;
	RolloutDenominator rolloutProportionDenominator;

	/** Cepac files for storing current population groups (only if using rollout) */
	SimContext *untreatedContext;
	SimContext *treatedContext;

    /*@}*/


    /** If we are using rollout use the cepac files specified in the ART rollout section */
	bool useRollout;

	/** timesToSwitchSimContext[0] should always be 0 by default (?) */
	std::vector<Time> timesToSwitchSimContext;

	inline bool itIsTimeToSwitchSimContext()
	{
        return std::any_of(timesToSwitchSimContext.cbegin(), timesToSwitchSimContext.cend(), [this](const Time &time){
            return (currTime == time + TimeSpan::Month);
        });
	}

	/** CEPAC related runStats (output) */
	RunStats *cepacRunStats;

    /** CEPAC cost Tracer */
    CostStats *cepacCostStats;

	/** CEPAC tracing object (output) */
	Tracer *cepacTracer;

	/** calibration inputs */
	CalibrationInputs calibrationInputs;

	/** prevalence delay time */
	Time delayPrevalence;

    std::map<TraceFile::Type, TraceFile> trace_files;

	/** number of patients per initial age range to be followed */
	int numToTrace;

	/** number of newborns to trace after specified month */
	int numNewbornsToTrace;

	Time monthTraceNewborns;

	/** keeps track of how many newborns have been traced */
	int numNewbornsTraced;

    std::vector<int> partnerNetworkRecordTimes;

	bool tracePrevalentCases;

	/** Concurrency Definitions */
	std::array<ConcurrencyDef, Constants::NumberConcurrencyDefs> concurrencyDef;

	std::map<int, double> targetYearlyRolloutProportions;

	/** random number generator that is used throughout the simulation */
	RandomNumberGenerator randomNums;

	bool enableDynamicTreatmentScaling;
	int dynamicFeedbackPeriod;

	/** closes all the trace files */
	~EventParams()
	{
		delete cepacRunStats;

		while(!cepacSimContexts.empty())
		{
			SimContext *sc = cepacSimContexts.back();
			cepacSimContexts.pop_back();
			delete sc;
		}

		while(!rolloutSimContexts.empty())
		{
			RolloutContext *sc = rolloutSimContexts.back();
			rolloutSimContexts.pop_back();
			delete sc;
		}

		for (const auto& cf : cepac_file_context_map_)
		{
            delete cf.second;
		}

		delete cepacTracer;
	}
  
    SimContext *LoadCepacContext(const std::string &cepac_file)
    {
        if (cepac_file_context_map_.find(cepac_file) == cepac_file_context_map_.end())
        {
            /* Set the CEPAC simContext from the specified CEPAC .in file */
            auto stem = transm::path(cepac_file).stem().string();
            cepac_file_context_map_[cepac_file] = new SimContext(stem);
 
            /* Read in the inputs */
            try
            {
                cepac_file_context_map_[cepac_file]->readInputs();
            }
            catch (std::string &errorString)
            {
                throw std::runtime_error(errorString);
            }
        }

        return cepac_file_context_map_[cepac_file];
    }
 
    std::unordered_map<std::string, SimContext *> cepac_file_context_map_;
};

} // namespace transm


#endif /* EVENTPARAMS_HPP */