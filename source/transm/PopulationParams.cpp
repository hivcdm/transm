#include "Population.h"
#include "entities/Female.h"
#include "entities/Male.h"
#include "entities/Person.h"
#include "entities/behaviors/SexualBehaviorParams.h"
#include "util/Util.h"

//-------------< Begin AgeBucketPrevalenceInfo methods >-------------------//

PopulationParams::AgeBucketPrevalenceInfo::AgeBucketPrevalenceInfo(int _minAgeMth, int _maxAgeMth,
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
	proportionOfPopulation[DmgProfile::MALE] = _proportionOfPopulationMale;
	proportionOfPopulation[DmgProfile::FEMALE] = _proportionOfPopulationFemale;
	numInfectedCSW[DmgProfile::MALE] = _numInfectedCSWMale;
	numInfectedCSW[DmgProfile::FEMALE] = _numInfectedCSWFemale;
	numInfectedRisk[DmgProfile::MALE][Person::LOW] = _numInfectedNonCSWMalesLowRisk;
	numInfectedRisk[DmgProfile::MALE][Person::HIGH] = _numInfectedNonCSWMalesHighRisk;
	numInfectedRisk[DmgProfile::FEMALE][Person::LOW] = _numInfectedNonCSWFemalesLowRisk;
	numInfectedRisk[DmgProfile::FEMALE][Person::HIGH] = _numInfectedNonCSWFemalesHighRisk;
}

void PopulationParams::AgeBucketPrevalenceInfo::print(EventParams &_eventParams)
{
	_eventParams.displayOut("\tAges ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(minAgeMth).c_str());
	_eventParams.displayOut(" - ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(maxAgeMth).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tproportion population male = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(proportionOfPopulation[DmgProfile::MALE]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tproportion population female = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(proportionOfPopulation[DmgProfile::FEMALE]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tchance csw males = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(chanceCSW[DmgProfile::MALE]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tchance csw females = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(chanceCSW[DmgProfile::FEMALE]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tnum infected csw males = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(numInfectedCSW[DmgProfile::MALE]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tnum infected csw females = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(numInfectedCSW[DmgProfile::FEMALE]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tnum infected high risk males = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>
	                        (numInfectedRisk[DmgProfile::MALE][Person::HIGH]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tnum infected high risk females = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>
	                        (numInfectedRisk[DmgProfile::FEMALE][Person::HIGH]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tnum infected low risk males = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(numInfectedRisk[DmgProfile::MALE][Person::LOW]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tnum infected low risk females = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>
	                        (numInfectedRisk[DmgProfile::FEMALE][Person::LOW]).c_str());
	_eventParams.displayOut("\n");
}

void PopulationParams::AgeBucketPrevalenceInfo::copyToSelf(AgeBucketPrevalenceInfo _abpInfo)
{
	minAgeMth = _abpInfo.minAgeMth;
	maxAgeMth = _abpInfo.maxAgeMth;

	for(int i = 0; i < DmgProfile::ENDGender; i++)
	{
		proportionOfPopulation[i] = _abpInfo.proportionOfPopulation[i];
		chanceCSW[i] = _abpInfo.chanceCSW[i];
		numInfectedCSW[i] = _abpInfo.numInfectedCSW[i];

		for(int j = 0; j < Person::ENDRiskLevel; j++)
		{
			numInfectedRisk[i][j] = _abpInfo.numInfectedRisk[i][j];
		}
	}
}

PopulationParams::PopulationParams()
{
	//set default values of fields
	debugLevel = DEBUG1;
	maxTime = 100;
	initSize = 10000;
	cepacInputFile = "./artproph.in";
	birthRate = 0.0038;
	SAEntAgeMths = 180;
	proportionMale = 0.51;
	circumcised = 0.20;
	hivInfected = 0.10;
}

PopulationParams::~PopulationParams()
{
}

double PopulationParams::getBirthRate() const
{
	return birthRate;
}
