#ifndef AGEBUCKETPREVALENCEINFO_HPP
#define AGEBUCKETPREVALENCEINFO_HPP

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

    AgeBucketPrevalenceInfo(Age _minAgeMth, Age _maxAgeMth, double _proportionInAgeBucket,
		const std::vector<DemographicProfile::DoublePair> &entity_proportions);

    Age GetMinAge() { return minAgeMth; }
    Age GetMaxAge() { return maxAgeMth; }

    double GetProportionInAgeBucket() { return proportionInAgeBucket; }

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

    double proportionInAgeBucket;

    /// <summary>
    /// determines size as proportion of the population
    /// </summary>
	std::vector<DemographicProfile::DoublePair> entityProportions;
};

} // namespace transm

#endif /* AGEBUCKETPREVALENCEINFO_HPP */