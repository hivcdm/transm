#include "core/population.hpp"
#include "entities/female.hpp"
#include "entities/male.hpp"
#include "entities/entity.hpp"
#include "entities/sexualbehavior.hpp"
#include "utility/utility.hpp"

namespace transm {

PopulationParameters::AgeBucketPrevalenceInfo::AgeBucketPrevalenceInfo(int _minAgeMth, int _maxAgeMth,
    const std::unordered_map<std::string, double> &entity_proportions,
    std::size_t _numInfectedCSWMale, std::size_t _numInfectedCSWFemale,
    std::size_t _numInfectedNonCSWMalesLowRisk, std::size_t _numInfectedNonCSWFemalesLowRisk,
    std::size_t _numInfectedNonCSWMalesHighRisk, std::size_t _numInfectedNonCSWFemalesHighRisk) :
    minAgeMth(_minAgeMth),
    maxAgeMth(_maxAgeMth),
    entityProportions(entity_proportions)
{
	assert((_minAgeMth >= 0) && (_maxAgeMth > 0) && (_maxAgeMth > _minAgeMth));
    numInfectedCSW[(std::size_t)DemographicProfile::Gender::Male] = _numInfectedCSWMale;
    numInfectedCSW[(std::size_t)DemographicProfile::Gender::Female] = _numInfectedCSWFemale;
    numInfectedRisk[(std::size_t)DemographicProfile::Gender::Male][Entity::LOW] = _numInfectedNonCSWMalesLowRisk;
    numInfectedRisk[(std::size_t)DemographicProfile::Gender::Male][Entity::HIGH] = _numInfectedNonCSWMalesHighRisk;
    numInfectedRisk[(std::size_t)DemographicProfile::Gender::Female][Entity::LOW] = _numInfectedNonCSWFemalesLowRisk;
    numInfectedRisk[(std::size_t)DemographicProfile::Gender::Female][Entity::HIGH] = _numInfectedNonCSWFemalesHighRisk;
}

PopulationParameters::PopulationParameters()
{
	//set default values of fields
    debugLevel = DebugLevel::One;
	initSize = 10000;
	birthRate = 0.0038;
	ageOfMajority = 180;
	birthProportions["hetero-male"] = 0.51;
    birthProportions["female"] = 1 - birthProportions["hetero-male"];
	proportionCircumcised = 0.20;
    sexualActivityDelay = 0;
}

PopulationParameters::~PopulationParameters()
{
}

double PopulationParameters::getBirthRate() const
{
	return birthRate;
}

} // namespace transm
