#pragma once

#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>

#include "core/constants.hpp"
#include "utility/time.hpp"

class SimContext;

namespace transm {

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

struct TraceFileParameters
{
    bool enabled;
    std::string extension;
    bool toss;
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
this structure stores the Inputs for calibration
**/
class CalibrationInputs
{
public:
    bool useCalibration;
    Time monthOfCalibration;

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

    // List of incidence rannge to test at each year
    std::map<Time, std::pair<double,double>> yearlyIncidenceRanges;
};

/**
this structure gives information about the rollout file to use and when to apply it if ART Rollout intervention is turned on
*/
class RolloutContext
{
public:
    Time timeToApply;
    std::unique_ptr<SimContext> rolloutSimContext;
    //who to apply to 0=All Untreated 1=All Treated 2=Untreated Getting New ART -1=None
    int popOfInterest;
    RolloutContext(Time t, std::unique_ptr<SimContext> context, int pop) : rolloutSimContext(std::move(context))
    {
        timeToApply = t;
        popOfInterest = pop;
    }
    ~RolloutContext()
    {
    }
};

/**
this structure stores the eligibility criter used for art rollout
*/
class RolloutEligibility
{
public:
    bool isIdentified;
    int oiHistRank;
    bool oiHistOIs[Constants::NumberOfOIs];
    int oiHistNumToStart;

    int cd4Rank;
    Bounds<int> cd4Bounds;

    int cd4OiHistRank;
    Bounds<int> cd4OiHistCd4Bounds;
    bool cd4OiHistOIs[Constants::NumberOfOIs];

    int hvlRank;
    Bounds<int> hvlBounds;

    int cd4HvlRank;
    Bounds<int> cd4HvlCd4Bounds;
    Bounds<int> cd4HvlHvlBounds;
};

enum RolloutDenominator {
	POPULATION,
	ELIGIBLE,
	DEFAULT = ELIGIBLE
};

struct CepacParameters
{
    enum class FileType
    {
        Cepac,
        Art
    } file_type;

    struct CepacFile
    {
        Time time;
        int target_population = 0;      /***< default target population = 0 (not on treatment) */
        std::string filename = "";
    };

    CepacParameters() :
        file_type(FileType::Cepac),
        dynamic_feedback_enabled(false),
        dynamic_feedback_period(0)
    {
    }

    CepacParameters(const CepacParameters &other) :
        file_type(other.file_type),
        default_cepac_file(other.default_cepac_file),
        cepac_files(other.cepac_files),
        eligibility_criteria(other.eligibility_criteria),
        dynamic_feedback_enabled(other.dynamic_feedback_enabled),
        dynamic_feedback_period(other.dynamic_feedback_period),
        target_yearly_rollout_proportions(other.target_yearly_rollout_proportions)
    {
    }

    ~CepacParameters()
    {
    }

    CepacParameters &operator=(CepacParameters other)
    {
        swap(other);
        return *this;
    }

    void swap(CepacParameters &other)
    {
        using std::swap;
        swap(default_cepac_file, other.default_cepac_file);
        swap(cepac_files, other.cepac_files);
        swap(eligibility_criteria, other.eligibility_criteria);
        swap(dynamic_feedback_enabled, other.dynamic_feedback_enabled);
        swap(dynamic_feedback_period, other.dynamic_feedback_period);
        swap(target_yearly_rollout_proportions, other.target_yearly_rollout_proportions);
    }

    CepacFile default_cepac_file;
    std::vector<CepacFile> cepac_files;
    RolloutEligibility eligibility_criteria;
    RolloutDenominator rollout_proportion_denominator;
    bool dynamic_feedback_enabled;
    int dynamic_feedback_period;
    std::vector<std::pair<int, double>> target_yearly_rollout_proportions;
};

} // namespace transm
