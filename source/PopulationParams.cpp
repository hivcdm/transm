#include "Population.h"
#include "entities/Female.h"
#include "entities/Male.h"
#include "entities/Person.h"
#include "entities/behaviors/SexualBehaviorParams.h"
#include "util/Util.h"
#include "util/XMLUtil.h"

//-------------< Begin AgeBucketPrevalenceInfo methods >-------------------//

PopulationParams::AgeBucketPrevalenceInfo::AgeBucketPrevalenceInfo(int _minAgeMth, int _maxAgeMth,
        double _proportionOfPopulationMale,
        double _proportionOfPopulationFemale,
        double _numInfectedCSWMale,
        double _numInfectedCSWFemale,
        double _numInfectedNonCSWMalesLowRisk,
        double _numInfectedNonCSWFemalesLowRisk,
        double _numInfectedNonCSWMalesHighRisk,
        double _numInfectedNonCSWFemalesHighRisk)
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

//-------------< End AgeBucketPrevalenceInfo methods >-------------------//


//-------------< Begin PopulationParams methods >-------------------//

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
	unsigned int ageBucketNum = initialAgeBuckets.size();

	for(unsigned int i = 0; i < ageBucketNum; ++i)
	{
		delete initialAgeBuckets.at(i);
	}
}

void PopulationParams::init(ticpp::Element *_populationXML, unsigned int _populationID, EventParams &_eventParams)
{
	assert(_populationXML != nullptr);
	populationID = _populationID;
	loadXML(_populationXML, _eventParams);
}

void PopulationParams::loadXML(ticpp::Element *_populationXML, EventParams &_eventParams)
{
	assert(_populationXML != nullptr);
	_eventParams.displayOut("Population Parameters\n");
	std::string temp;

	try
	{
		ticpp::Element *initialState = _populationXML->FirstChildElement("initialState");
		initSize = initialState->FirstChildElement("size")->GetText<int>();
		_eventParams.displayOut("\tsize = ");
		_eventParams.displayOut(boost::lexical_cast<std::string>(initSize).c_str());
		_eventParams.displayOut("\n");
		//initial proportion married
		//TODO: Change me based on marriage acquisition rates et al
		//initproportionMarried  = initialState->FirstChildElement("proportionMarried")->GetText<double>();
		//initproportionRegular = initialState->FirstChildElement("proportionRegular")->GetText<double>();
		//get initial age distribution
		ticpp::Iterator<ticpp::Element> rangeIter;

		for(rangeIter = initialState->FirstChildElement("ageDistributionYrs")->FirstChildElement("range");
		        rangeIter != rangeIter.end(); rangeIter++)
		{
			//get data for each age bucket and save it
			initialAgeBuckets.push_back(
			    new AgeBucketPrevalenceInfo(
			        Util::convertTime(YEAR, MONTH, rangeIter->FirstChildElement("minAge")->GetText<int>()),
			        Util::convertTime(YEAR, MONTH, rangeIter->FirstChildElement("maxAge")->GetText<int>()) + 11,
			        rangeIter->FirstChildElement("distribMale")->GetText<double>(),
			        rangeIter->FirstChildElement("distribFemale")->GetText<double>(),
			        rangeIter->FirstChildElement("numInfectedMaleCSW")->GetText<double>(),
			        rangeIter->FirstChildElement("numInfectedFemaleCSW")->GetText<double>(),
			        rangeIter->FirstChildElement("numInfectedMaleLowRisk")->GetText<double>(),
			        rangeIter->FirstChildElement("numInfectedFemaleLowRisk")->GetText<double>(),
			        rangeIter->FirstChildElement("numInfectedMaleHighRisk")->GetText<double>(),
			        rangeIter->FirstChildElement("numInfectedFemaleHighRisk")->GetText<double>()
			    )
			);
		}

		initProbCSW[DmgProfile::MALE] = initialState->FirstChildElement("chanceBeingCSWMale")->GetText<double>();
		initProbCSW[DmgProfile::FEMALE] = initialState->FirstChildElement("chanceBeingCSWFemale")->GetText<double>();
		CSWEndAgeMth[DmgProfile::MALE] = Util::convertTime(YEAR, MONTH,
		                                       initialState->FirstChildElement("CSWEndAgeMale")->GetText<double>());
		CSWEndAgeMth[DmgProfile::FEMALE] = Util::convertTime(YEAR, MONTH,
		        initialState->FirstChildElement("CSWEndAgeFemale")->GetText<double>());
		//normalize %population values for each age bucket
		double totalPopulationproportionages[DmgProfile::ENDGender];

		//get the total of proportionage values of AgeBucketPrevalencInfo.proportionOfPopulation
		for(int i = 0; i < DmgProfile::ENDGender; i++)
		{
			totalPopulationproportionages[i] = 0.0;

			for(size_t ageBucketNum = 0; ageBucketNum < initialAgeBuckets.size(); ageBucketNum++)
			{
				totalPopulationproportionages[i] = totalPopulationproportionages[i] + initialAgeBuckets.at(
				                                       ageBucketNum)->proportionOfPopulation[i];
			}

			//normalize each proportionage value so that the sum of them == 1
			for(size_t ageBucketNum = 0; ageBucketNum < initialAgeBuckets.size(); ageBucketNum++)
			{
				initialAgeBuckets.at(ageBucketNum)->proportionOfPopulation[i] = initialAgeBuckets.at(
				            ageBucketNum)->proportionOfPopulation[i] / totalPopulationproportionages[i];
			}
		}

		for(size_t ageBucketNum = 0; ageBucketNum < initialAgeBuckets.size(); ageBucketNum++)
		{
			initialAgeBuckets.at(ageBucketNum)->print(_eventParams);
		}

		//dmgProfile parameters
		birthRate = _populationXML->FirstChildElement("birthRate")->GetText<double>();
		XMLUtil::printParam("", "birthRate", birthRate, _eventParams);
		proportionMale = _populationXML->FirstChildElement("proportionMale")->GetText<double>();
		XMLUtil::printParam("", "proportionMale", proportionMale, _eventParams);
		circumcised = _populationXML->FirstChildElement("proportionCircumcised")->GetText<double>();
		XMLUtil::printParam("", "proportionCircumcised", circumcised, _eventParams);
		SAEntAgeMths = Util::convertTime(YEAR, MONTH, _populationXML->FirstChildElement("ageSexualDebutYrs")->GetText<int>());
		XMLUtil::printParam("", "SAEntAge (month)", SAEntAgeMths, _eventParams);
		//ASSORTATIVENESS GOES HERE
		assort[SexualPartnership::STEADY] =
		    _populationXML->FirstChildElement("assortativeness")->FirstChildElement("steady")->GetText<double>();
		assort[SexualPartnership::REGULAR] =
		    _populationXML->FirstChildElement("assortativeness")->FirstChildElement("regular")->GetText<double>();
		assort[SexualPartnership::CASUAL] =
		    _populationXML->FirstChildElement("assortativeness")->FirstChildElement("casual")->GetText<double>();
		assort[SexualPartnership::CSW] =
		    _populationXML->FirstChildElement("assortativeness")->FirstChildElement("csw")->GetText<double>();
		// Get an EntityTypes element
		ticpp::Element *entityTypes =
		    _populationXML->FirstChildElement("entityTypes")->FirstChildElement("baseEntities")->FirstChildElement("baseEntity");
		//iterate through each Person in EntityTypes
		ticpp::Iterator<ticpp::Element> entityTypesIter;

		for(entityTypesIter = entityTypes; entityTypesIter != entityTypesIter.end(); entityTypesIter++)
		{
			std::string baseEntityElem = entityTypesIter->FirstChildElement("type")->GetText();

			if(baseEntityElem.compare("Male") == 0)
			{
				Male::addPopParams(populationID, entityTypesIter->ToElement(), _eventParams);
				maleParams = Male::getPopParams(populationID);
			}
			else if(baseEntityElem.compare("Female") == 0)
			{
				Female::addPopParams(populationID, entityTypesIter->ToElement(), _eventParams);
				femaleParams = Female::getPopParams(populationID);
			}
			else
			{
				_eventParams.displayOut("PopulationParams::loadXML(...): Ignoring Unknown type :");
				_eventParams.displayOut(baseEntityElem.c_str());
				_eventParams.displayOut("\n");
			}
		}

		//Get the initial marriage prevalence based on percent male high risk and rate and duration of steady relationships
		double pHigh = maleParams->getProportionHighRisk(DmgProfile::NON_CSW);
		double marriageRateH = maleParams->getSexualBehaviorParams(SexualPartnership::STEADY)->getAcquisitionRatePerMonth(
		                           Person::HIGH).getMean();
		double marriageRateL = maleParams->getSexualBehaviorParams(SexualPartnership::STEADY)->getAcquisitionRatePerMonth(
		                           Person::LOW).getMean();
		double marriageDurationH = maleParams->getSexualBehaviorParams(
		                               SexualPartnership::STEADY)->getPartnershipDurationMth(Person::HIGH).getMean();
		double marriageDurationL = maleParams->getSexualBehaviorParams(
		                               SexualPartnership::STEADY)->getPartnershipDurationMth(Person::LOW).getMean();
		initproportionMarried = (1 - pHigh) * (marriageRateL * marriageDurationL) / (1 + marriageRateL *
		                              marriageDurationL) + pHigh * (marriageRateH * marriageDurationH) / (1 + marriageRateH * marriageDurationH);
		//Get the initial regular prevalence based on percent male high risk and rate and duration of regular relationships
		double regularRateH = maleParams->getSexualBehaviorParams(SexualPartnership::REGULAR)->getAcquisitionRatePerMonth(
		                          Person::HIGH).getMean();
		double regularRateL = maleParams->getSexualBehaviorParams(SexualPartnership::REGULAR)->getAcquisitionRatePerMonth(
		                          Person::LOW).getMean();
		double regularDurationH = maleParams->getSexualBehaviorParams(
		                              SexualPartnership::REGULAR)->getPartnershipDurationMth(Person::HIGH).getMean();
		double regularDurationL = maleParams->getSexualBehaviorParams(
		                              SexualPartnership::REGULAR)->getPartnershipDurationMth(Person::LOW).getMean();
		initproportionRegular = (1 - pHigh) * (regularRateL * regularDurationL) + pHigh *
		                              (regularRateH * regularDurationH);
		//Costs
		condomCost = _populationXML->FirstChildElement("costs")->FirstChildElement("condomCost")->GetText<double>();
		circumcisionCost = _populationXML->FirstChildElement("costs")->FirstChildElement("circumcisionCost")->GetText<double>();
	}
	catch(ticpp::Exception &_e)
	{
		_eventParams.displayOut("PopulationParams: Exception raised: ");
		_eventParams.displayOut(_e.m_details.c_str());
		_eventParams.displayOut("\n");
		Util::exitWithPrompt(-1);
	}

	//save flags to indicate whether particular partnership types have duration or not
	for(SexualPartnership::Type type = SexualPartnership::Type(0); type < SexualPartnership::ENDType; ++type)
	{
		partnershipsHaveDuration[DmgProfile::MALE][type] = !(maleParams->getSexualBehaviorParams(
		            type)->getPartnershipDurationMth(Person::LOW).isZeroDistrib)
		        && !(maleParams->getSexualBehaviorParams(type)->getPartnershipDurationMth(Person::HIGH).isZeroDistrib);
		partnershipsHaveDuration[DmgProfile::FEMALE][type] = false;
	}

	_eventParams.displayOut("\n");
}

/**reloads certain xml data from another xml file in a sequence of files
   only loads non initial data
**/
void PopulationParams::reloadXML(ticpp::Element *_populationXML, EventParams &_eventParams)
{
	assert(_populationXML != nullptr);
	_eventParams.displayOut("Population Parameters\n");
	std::string temp;

	try
	{
		//dmgProfile parameters
		birthRate = _populationXML->FirstChildElement("birthRate")->GetText<double>();
		XMLUtil::printParam("", "birthRate", birthRate, _eventParams);
		proportionMale = _populationXML->FirstChildElement("proportionMale")->GetText<double>();
		XMLUtil::printParam("", "proportionMale", proportionMale, _eventParams);
		circumcised = _populationXML->FirstChildElement("proportionCircumcised")->GetText<double>();
		XMLUtil::printParam("", "proportionCircumcised", circumcised, _eventParams);
		SAEntAgeMths = Util::convertTime(YEAR, MONTH, _populationXML->FirstChildElement("ageSexualDebutYrs")->GetText<int>());
		XMLUtil::printParam("", "SAEntAge (month)", SAEntAgeMths, _eventParams);
		//ASSORTATIVENESS GOES HERE
		assort[SexualPartnership::STEADY] =
		    _populationXML->FirstChildElement("assortativeness")->FirstChildElement("steady")->GetText<double>();
		assort[SexualPartnership::REGULAR] =
		    _populationXML->FirstChildElement("assortativeness")->FirstChildElement("regular")->GetText<double>();
		assort[SexualPartnership::CASUAL] =
		    _populationXML->FirstChildElement("assortativeness")->FirstChildElement("casual")->GetText<double>();
		assort[SexualPartnership::CSW] =
		    _populationXML->FirstChildElement("assortativeness")->FirstChildElement("csw")->GetText<double>();
		// Get an EntityTypes element
		ticpp::Element *entityTypes =
		    _populationXML->FirstChildElement("entityTypes")->FirstChildElement("baseEntities")->FirstChildElement("baseEntity");
		//iterate through each Person in EntityTypes
		ticpp::Iterator<ticpp::Element> entityTypesIter;

		for(entityTypesIter = entityTypes; entityTypesIter != entityTypesIter.end(); entityTypesIter++)
		{
			std::string baseEntityElem = entityTypesIter->FirstChildElement("type")->GetText();

			if(baseEntityElem.compare("Male") == 0)
			{
				Male::updatePopParams(populationID, entityTypesIter->ToElement(), _eventParams);
				maleParams = Male::getPopParams(populationID);
			}
			else if(baseEntityElem.compare("Female") == 0)
			{
				Female::updatePopParams(populationID, entityTypesIter->ToElement(), _eventParams);
				femaleParams = Female::getPopParams(populationID);
			}
			else
			{
				_eventParams.displayOut("PopulationParams::loadXML(...): Ignoring Unknown type :");
				_eventParams.displayOut(baseEntityElem.c_str());
				_eventParams.displayOut("\n");
			}
		}

		//Costs
		condomCost = _populationXML->FirstChildElement("costs")->FirstChildElement("condomCost")->GetText<double>();
		circumcisionCost = _populationXML->FirstChildElement("costs")->FirstChildElement("circumcisionCost")->GetText<double>();
	}
	catch(ticpp::Exception &_e)
	{
		_eventParams.displayOut("PopulationParams: Exception raised: ");
		_eventParams.displayOut(_e.m_details.c_str());
		_eventParams.displayOut("\n");
		Util::exitWithPrompt(-1);
	}

	//save flags to indicate whether particular partnership types have duration or not
	for(SexualPartnership::Type type = SexualPartnership::Type(0); type < SexualPartnership::ENDType; ++type)
	{
		partnershipsHaveDuration[DmgProfile::MALE][type] = (!maleParams->getSexualBehaviorParams(
		            type)->getPartnershipDurationMth(Person::LOW).isZeroDistrib)
		        && (!maleParams->getSexualBehaviorParams(type)->getPartnershipDurationMth(Person::HIGH).isZeroDistrib);
		partnershipsHaveDuration[DmgProfile::FEMALE][type] = false;
	}

	_eventParams.displayOut("\n");
}

double PopulationParams::getBirthRate() const
{
	return birthRate;
}

//-------------< End PopulationParams methods >-------------------//
