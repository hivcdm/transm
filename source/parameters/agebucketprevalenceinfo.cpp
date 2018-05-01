#include "agebucketprevalenceinfo.hpp"

namespace transm {

AgeBucketPrevalenceInfo::AgeBucketPrevalenceInfo(Age _minAgeMth, Age _maxAgeMth,
    const std::unordered_map<std::string, double> &entity_proportions) :
        minAgeMth(_minAgeMth),
        maxAgeMth(_maxAgeMth),
        entityProportions(entity_proportions)
{
    assert((_minAgeMth >= Time::Zero) && (_maxAgeMth > Time::Zero) && (_maxAgeMth > _minAgeMth));
}

} // namespace transm
