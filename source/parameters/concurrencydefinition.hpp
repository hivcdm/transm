#pragma once

#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>

#include "core/constants.hpp"

namespace transm {

class SimContext;

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

struct InterventionParameters
{
    enum class InterventionType
    {
        Cepac,
        Art
    } intervention_type;
    std::string default_in_file;
    std::unordered_map<int, std::string> in_files;
    RolloutEligibility eligibility_criteria;
    bool dynamic_feedback_enabled;
    int dynamic_feedback_period;
};

} // namespace transm
