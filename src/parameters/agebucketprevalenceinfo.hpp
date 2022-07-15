#ifndef AGEBUCKETPREVALENCEINFO_HPP
#define AGEBUCKETPREVALENCEINFO_HPP

#include <string>
#include <unordered_map>

#include "entities/demographicprofile.hpp"
#include "utility/time.hpp"

namespace transm {

/** this data structure contains prevalence parameters differ in value by age buckets */
class AgeBucketPrevalenceInfo
{

public:
    AgeBucketPrevalenceInfo();

    AgeBucketPrevalenceInfo(Age _minAgeMth, Age _maxAgeMth, double _proportionInAgeBucket,
		std::vector<DemographicProfile::DoublePair> entity_proportions);

    Age GetMinAge() { return minAgeMth; }
    Age GetMaxAge() { return maxAgeMth; }

    double GetProportionInAgeBucket() const { return proportionInAgeBucket; }

    const std::vector<DemographicProfile::DoublePair> &GetEntityProportions();
	void SetEntityProportion(const DemographicProfile& profile, double value);

private:
     /** the min age that this bucket represents */
    Age minAgeMth;

     /** the max age that this bucket represents */
    Age maxAgeMth;

    double proportionInAgeBucket;

     /** determines size as proportion of the population */
	std::vector<DemographicProfile::DoublePair> entityProportions;
};

} // namespace transm

#endif /* AGEBUCKETPREVALENCEINFO_HPP */