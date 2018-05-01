#pragma once

#include <string>
#include <unordered_map>

#include "entities/entity.hpp"

namespace transm {

/// <summary>
/// this data structure contains prevalence parameters differ in value by age buckets
/// </summary>
class AgeBucketPrevalenceInfo
{
public:
    AgeBucketPrevalenceInfo();

    AgeBucketPrevalenceInfo(Age _minAgeMth, Age _maxAgeMth,
	const std::unordered_map<std::string, double> &entity_proportions);

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
};
  
} // namespace transm
