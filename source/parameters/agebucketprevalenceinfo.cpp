#include "agebucketprevalenceinfo.hpp"

namespace transm {

AgeBucketPrevalenceInfo::AgeBucketPrevalenceInfo(Age _minAgeMth, Age _maxAgeMth,
    const std::unordered_map<std::string, double> &entity_proportions,
    std::size_t _numInfectedCSWMale, std::size_t _numInfectedCSWFemale,
    std::size_t _numInfectedNonCSWMalesLowRisk, std::size_t _numInfectedNonCSWFemalesLowRisk,
    std::size_t _numInfectedNonCSWMalesHighRisk, std::size_t _numInfectedNonCSWFemalesHighRisk) :
    minAgeMth(_minAgeMth),
    maxAgeMth(_maxAgeMth),
    entityProportions(entity_proportions)
{
    assert((_minAgeMth >= Time::Zero) && (_maxAgeMth > Time::Zero) && (_maxAgeMth > _minAgeMth));
    numInfectedCSW[(std::size_t)DemographicProfile::Gender::Male] = _numInfectedCSWMale;
    numInfectedCSW[(std::size_t)DemographicProfile::Gender::Female] = _numInfectedCSWFemale;
    numInfectedRisk[(std::size_t)DemographicProfile::Gender::Male][(std::size_t)Entity::RiskLevel::LOW] = _numInfectedNonCSWMalesLowRisk;
    numInfectedRisk[(std::size_t)DemographicProfile::Gender::Male][(std::size_t)Entity::RiskLevel::HIGH] = _numInfectedNonCSWMalesHighRisk;
    numInfectedRisk[(std::size_t)DemographicProfile::Gender::Female][(std::size_t)Entity::RiskLevel::LOW] = _numInfectedNonCSWFemalesLowRisk;
    numInfectedRisk[(std::size_t)DemographicProfile::Gender::Female][(std::size_t)Entity::RiskLevel::HIGH] = _numInfectedNonCSWFemalesHighRisk;
}

} // namespace transm
