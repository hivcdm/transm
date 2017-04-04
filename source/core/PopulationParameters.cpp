#include "Population.h"
#include "../entities/Female.h"
#include "../entities/Male.h"
#include "../entities/Person.h"
#include "../entities/behaviors/SexualBehavior.h"
#include "../utility/Utility.h"

//-------------< Begin AgeBucketPrevalenceInfo methods >-------------------//

PopulationParameters::AgeBucketPrevalenceInfo::AgeBucketPrevalenceInfo(int _minAgeMth, int _maxAgeMth,
        double _proportionOfPopulationMale,
        double _proportionOfPopulationFemale,
        int _numInfectedCSWMale,
		int _numInfectedCSWFemale,
		int _numInfectedNonCSWMalesLowRisk,
		int _numInfectedNonCSWFemalesLowRisk,
		int _numInfectedNonCSWMalesHighRisk,
		int _numInfectedNonCSWFemalesHighRisk)
{
	assert((_minAgeMth >= 0) && (_maxAgeMth > 0) && (_maxAgeMth > _minAgeMth));
	minAgeMth = _minAgeMth;
	maxAgeMth = _maxAgeMth;
    proportionOfPopulation[(std::size_t)DemographicProfile::Gender::Male] = _proportionOfPopulationMale;
    proportionOfPopulation[(std::size_t)DemographicProfile::Gender::Female] = _proportionOfPopulationFemale;
    numInfectedCSW[(std::size_t)DemographicProfile::Gender::Male] = _numInfectedCSWMale;
    numInfectedCSW[(std::size_t)DemographicProfile::Gender::Female] = _numInfectedCSWFemale;
    numInfectedRisk[(std::size_t)DemographicProfile::Gender::Male][Person::LOW] = _numInfectedNonCSWMalesLowRisk;
    numInfectedRisk[(std::size_t)DemographicProfile::Gender::Male][Person::HIGH] = _numInfectedNonCSWMalesHighRisk;
    numInfectedRisk[(std::size_t)DemographicProfile::Gender::Female][Person::LOW] = _numInfectedNonCSWFemalesLowRisk;
    numInfectedRisk[(std::size_t)DemographicProfile::Gender::Female][Person::HIGH] = _numInfectedNonCSWFemalesHighRisk;
}

PopulationParameters::PopulationParameters()
{
    //set default values of fields
    debugLevel = DebugLevel::One;
    initSize = 10000;
    birthRate = 0.0038;
    UseBirthRate = true;
    ageOfMajority = 180;
    proportionMale = 0.51;
    proportionCircumcised = 0.20;
    sexualActivityDelay = 0;
}

PopulationParameters::~PopulationParameters()
{
}
