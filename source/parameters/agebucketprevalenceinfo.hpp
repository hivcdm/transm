#pragma once

#include <string>
#include <unordered_map>

#include "entities/demographicprofile.hpp"
#include "utility/time.hpp"

namespace transm {

/// <summary>
/// this data structure contains prevalence parameters differ in value by age buckets
/// </summary>
class AgeBucketPrevalenceInfo
{
public:
    AgeBucketPrevalenceInfo();

    AgeBucketPrevalenceInfo(Age _minAgeMth, Age _maxAgeMth,
		const std::vector<DemographicProfile::DoublePair> &entity_proportions);

    Age GetMinAge() { return minAgeMth; }
    Age GetMaxAge() { return maxAgeMth; }

    const std::vector<DemographicProfile::DoublePair> &GetEntityProportions();
	void SetEntityProportion(DemographicProfile profile, double value);

private:
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
	std::vector<DemographicProfile::DoublePair> entityProportions;
};

} // namespace transm
