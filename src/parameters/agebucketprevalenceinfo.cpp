#include "agebucketprevalenceinfo.hpp"

#include <utility>

namespace transm {

    AgeBucketPrevalenceInfo::AgeBucketPrevalenceInfo(Age _minAgeMth, Age _maxAgeMth,
        double _proportionInAgeBucket,
		std::vector<DemographicProfile::DoublePair> entity_proportions) :
        minAgeMth(_minAgeMth),
        maxAgeMth(_maxAgeMth),
        proportionInAgeBucket(_proportionInAgeBucket),
        entityProportions(std::move(entity_proportions))
    {
	    assert((_minAgeMth >= Time::Zero) && (_maxAgeMth > Time::Zero) && (_maxAgeMth > _minAgeMth));
    }

	const std::vector<DemographicProfile::DoublePair> &AgeBucketPrevalenceInfo::GetEntityProportions()
    {
		return entityProportions;
    }

	void AgeBucketPrevalenceInfo::SetEntityProportion(const DemographicProfile& profile, double value)
    {
		for (auto distrib : entityProportions)
		{
			if (distrib.first == profile) distrib.second = value;
		}
    }

} // namespace transm
