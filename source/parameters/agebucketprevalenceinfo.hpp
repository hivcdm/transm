#pragma once

#include <string>
#include <unordered_map>

#include "entities/entity.hpp"

namespace transm {

class EventParams;

/// <summary>
/// this data structure contains prevalence parameters differ in value by age buckets
/// </summary>
class AgeBucketPrevalenceInfo
{
public:
    AgeBucketPrevalenceInfo();

    AgeBucketPrevalenceInfo(Age _minAgeMth, Age _maxAgeMth,
        const std::unordered_map<std::string, double> &entity_proportions,
        std::size_t _numInfectedCSWMale, std::size_t _numInfectedCSWFemale,
        std::size_t _numInfectedNonCSWMalesLowRisk, std::size_t _numInfectedNonCSWFemalesLowRisk,
        std::size_t _numInfectedNonCSWMalesHighRisk, std::size_t _numInfectedNonCSWFemalesHighRisk);

    void print(EventParams &_eventParams);

    /// <summary>
    /// the min age that this bucket represents
    /// </summary>
    Age minAgeMth;

    /// <summary>
    /// the max age that this bucket represents
    /// </summary>
    Age maxAgeMth;

    /// <summary>
    /// determines size as proportion of the population
    /// </summary>
    std::unordered_map<std::string, double> entityProportions;

    /// <summary>
    /// number of males and female csw in this bucket that are infected (at prevalence delay)
    /// </summary>
    std::size_t numInfectedCSW[(std::size_t)DemographicProfile::Gender::Last];

    /// <summary>
    /// number of male and female non-csw in this bucket that are infected (at prevalence delay)
    /// </summary>
    std::size_t numInfectedRisk[(std::size_t)DemographicProfile::Gender::Last][(std::size_t)Entity::RiskLevel::Last];
};

} // namespace transm
