#include "Population.h"
#include "../entities/Female.h"
#include "../entities/Male.h"
#include "../entities/Person.h"
#include "../entities/behaviors/SexualBehavior.h"
#include "../util/Utility.h"

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
	proportionOfPopulation[DemographicProfile::MALE] = _proportionOfPopulationMale;
	proportionOfPopulation[DemographicProfile::FEMALE] = _proportionOfPopulationFemale;
	numInfectedCSW[DemographicProfile::MALE] = _numInfectedCSWMale;
	numInfectedCSW[DemographicProfile::FEMALE] = _numInfectedCSWFemale;
	numInfectedRisk[DemographicProfile::MALE][Person::LOW] = _numInfectedNonCSWMalesLowRisk;
	numInfectedRisk[DemographicProfile::MALE][Person::HIGH] = _numInfectedNonCSWMalesHighRisk;
	numInfectedRisk[DemographicProfile::FEMALE][Person::LOW] = _numInfectedNonCSWFemalesLowRisk;
	numInfectedRisk[DemographicProfile::FEMALE][Person::HIGH] = _numInfectedNonCSWFemalesHighRisk;
}

void PopulationParameters::AgeBucketPrevalenceInfo::print(EventParams &_eventParams)
{
	_eventParams.displayOut("\tAges ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(minAgeMth).c_str());
	_eventParams.displayOut(" - ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(maxAgeMth).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tproportion population male = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(proportionOfPopulation[DemographicProfile::MALE]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tproportion population female = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(proportionOfPopulation[DemographicProfile::FEMALE]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tnum infected csw males = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(numInfectedCSW[DemographicProfile::MALE]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tnum infected csw females = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(numInfectedCSW[DemographicProfile::FEMALE]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tnum infected high risk males = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>
	                        (numInfectedRisk[DemographicProfile::MALE][Person::HIGH]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tnum infected high risk females = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>
	                        (numInfectedRisk[DemographicProfile::FEMALE][Person::HIGH]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tnum infected low risk males = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(numInfectedRisk[DemographicProfile::MALE][Person::LOW]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tnum infected low risk females = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>
	                        (numInfectedRisk[DemographicProfile::FEMALE][Person::LOW]).c_str());
	_eventParams.displayOut("\n");
}

PopulationParameters::PopulationParameters()
{
	//set default values of fields
	debugLevel = DEBUG1;
	initSize = 10000;
	birthRate = 0.0038;
	SAEntAgeMths = 180;
	proportionMale = 0.51;
	circumcised = 0.20;
}

PopulationParameters::~PopulationParameters()
{
}

double PopulationParameters::getBirthRate() const
{
	return birthRate;
}
