
#include <boost/lexical_cast.hpp>

#include "Population.h"

#include "entities/Female.h"
#include "entities/Male.h"
#include "entities/Person.h"
#include "entities/behaviors/SexualBehaviorParams.h"
#include "util/Util.h"
#include "util/XMLUtil.h"

//-------------< Begin AgeBucketPrevalenceInfo methods >-------------------//

Population::Params::AgeBucketPrevalenceInfo::AgeBucketPrevalenceInfo(int _minAgeMth, int _maxAgeMth,
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

void Population::Params::AgeBucketPrevalenceInfo::print(EventParams &_eventParams)
{
	_eventParams.displayOut("\tAges ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(this->minAgeMth).c_str());
	_eventParams.displayOut(" - ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(this->maxAgeMth).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tproportion population male = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(this->proportionOfPopulation[DmgProfile::MALE]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tproportion population female = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(this->proportionOfPopulation[DmgProfile::FEMALE]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tchance csw males = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(this->chanceCSW[DmgProfile::MALE]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tchance csw females = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(this->chanceCSW[DmgProfile::FEMALE]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tnum infected csw males = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(this->numInfectedCSW[DmgProfile::MALE]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tnum infected csw females = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(this->numInfectedCSW[DmgProfile::FEMALE]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tnum infected high risk males = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>
	                        (this->numInfectedRisk[DmgProfile::MALE][Person::HIGH]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tnum infected high risk females = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>
	                        (this->numInfectedRisk[DmgProfile::FEMALE][Person::HIGH]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\num infected low risk males = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(this->numInfectedRisk[DmgProfile::MALE][Person::LOW]).c_str());
	_eventParams.displayOut("\n");
	_eventParams.displayOut("\tnum infected low risk females = ");
	_eventParams.displayOut(boost::lexical_cast<std::string>
	                        (this->numInfectedRisk[DmgProfile::FEMALE][Person::LOW]).c_str());
	_eventParams.displayOut("\n");
}

void Population::Params::AgeBucketPrevalenceInfo::copyToSelf(AgeBucketPrevalenceInfo _abpInfo)
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


//-------------< Begin Population::Params methods >-------------------//

Population::Params::Params()
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

Population::Params::~Params()
{
	unsigned int ageBucketNum = this->initialAgeBuckets.size();

	for(unsigned int i = 0; i < ageBucketNum; ++i)
	{
		delete initialAgeBuckets.at(i);
	}
}

void Population::Params::init(ticpp::Element *_populationXML, unsigned int _populationID, EventParams &_eventParams)
{
	assert(_populationXML != NULL);
	populationID = _populationID;
	this->loadXML(_populationXML, _eventParams);
}

void Population::Params::loadXML(ticpp::Element *_populationXML, EventParams &_eventParams)
{
	assert(_populationXML != NULL);
	_eventParams.displayOut("Population Parameters\n");
	std::string temp;

	try
	{
		ticpp::Element *initialState = _populationXML->FirstChildElement("initialState");
		this->initSize = initialState->FirstChildElement("size")->GetText<int>();
		_eventParams.displayOut("\tsize = ");
		_eventParams.displayOut(boost::lexical_cast<std::string>(initSize).c_str());
		_eventParams.displayOut("\n");
		//initial proportion married
		//TODO: Change me based on marriage acquisition rates et al
		//this->initproportionMarried  = initialState->FirstChildElement("proportionMarried")->GetText<double>();
		//this->initproportionRegular = initialState->FirstChildElement("proportionRegular")->GetText<double>();
		//get initial age distribution
		ticpp::Iterator<ticpp::Element> rangeIter;

		for(rangeIter = initialState->FirstChildElement("ageDistributionYrs")->FirstChildElement("range");
		        rangeIter != rangeIter.end(); rangeIter++)
		{
			//get data for each age bucket and save it
			this->initialAgeBuckets.push_back(
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

		this->initProbCSW[DmgProfile::MALE] = initialState->FirstChildElement("chanceBeingCSWMale")->GetText<double>();
		this->initProbCSW[DmgProfile::FEMALE] = initialState->FirstChildElement("chanceBeingCSWFemale")->GetText<double>();
		this->CSWEndAgeMth[DmgProfile::MALE] = Util::convertTime(YEAR, MONTH,
		                                       initialState->FirstChildElement("CSWEndAgeMale")->GetText<double>());
		this->CSWEndAgeMth[DmgProfile::FEMALE] = Util::convertTime(YEAR, MONTH,
		        initialState->FirstChildElement("CSWEndAgeFemale")->GetText<double>());
		//normalize %population values for each age bucket
		double totalPopulationproportionages[DmgProfile::ENDGender];

		//get the total of proportionage values of AgeBucketPrevalencInfo.proportionOfPopulation
		for(int i = 0; i < DmgProfile::ENDGender; i++)
		{
			totalPopulationproportionages[i] = 0.0;

			for(size_t ageBucketNum = 0; ageBucketNum < initialAgeBuckets.size(); ageBucketNum++)
			{
				totalPopulationproportionages[i] = totalPopulationproportionages[i] + this->initialAgeBuckets.at(
				                                       ageBucketNum)->proportionOfPopulation[i];
			}

			//normalize each proportionage value so that the sum of them == 1
			for(size_t ageBucketNum = 0; ageBucketNum < initialAgeBuckets.size(); ageBucketNum++)
			{
				this->initialAgeBuckets.at(ageBucketNum)->proportionOfPopulation[i] = this->initialAgeBuckets.at(
				            ageBucketNum)->proportionOfPopulation[i] / totalPopulationproportionages[i];
			}
		}

		for(size_t ageBucketNum = 0; ageBucketNum < initialAgeBuckets.size(); ageBucketNum++)
		{
			this->initialAgeBuckets.at(ageBucketNum)->print(_eventParams);
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
				Male::addPopParams(this->populationID, entityTypesIter->ToElement(), _eventParams);
				this->maleParams = Male::getPopParams(this->populationID);
			}
			else if(baseEntityElem.compare("Female") == 0)
			{
				Female::addPopParams(this->populationID, entityTypesIter->ToElement(), _eventParams);
				this->femaleParams = Female::getPopParams(this->populationID);
			}
			else
			{
				_eventParams.displayOut("Population::Params::loadXML(...): Ignoring Unknown type :");
				_eventParams.displayOut(baseEntityElem.c_str());
				_eventParams.displayOut("\n");
			}
		}

		//Get the initial marriage prevalence based on percent male high risk and rate and duration of steady relationships
		double pHigh = this->maleParams->getProportionHighRisk(DmgProfile::NON_CSW);
		double marriageRateH = this->maleParams->getSexualBehaviorParams(SexualPartnership::STEADY)->getAcquisitionRatePerMonth(
		                           Person::HIGH).getMean();
		double marriageRateL = this->maleParams->getSexualBehaviorParams(SexualPartnership::STEADY)->getAcquisitionRatePerMonth(
		                           Person::LOW).getMean();
		double marriageDurationH = this->maleParams->getSexualBehaviorParams(
		                               SexualPartnership::STEADY)->getPartnershipDurationMth(Person::HIGH).getMean();
		double marriageDurationL = this->maleParams->getSexualBehaviorParams(
		                               SexualPartnership::STEADY)->getPartnershipDurationMth(Person::LOW).getMean();
		this->initproportionMarried = (1 - pHigh) * (marriageRateL * marriageDurationL) / (1 + marriageRateL *
		                              marriageDurationL) + pHigh * (marriageRateH * marriageDurationH) / (1 + marriageRateH * marriageDurationH);
		//Get the initial regular prevalence based on percent male high risk and rate and duration of regular relationships
		double regularRateH = this->maleParams->getSexualBehaviorParams(SexualPartnership::REGULAR)->getAcquisitionRatePerMonth(
		                          Person::HIGH).getMean();
		double regularRateL = this->maleParams->getSexualBehaviorParams(SexualPartnership::REGULAR)->getAcquisitionRatePerMonth(
		                          Person::LOW).getMean();
		double regularDurationH = this->maleParams->getSexualBehaviorParams(
		                              SexualPartnership::REGULAR)->getPartnershipDurationMth(Person::HIGH).getMean();
		double regularDurationL = this->maleParams->getSexualBehaviorParams(
		                              SexualPartnership::REGULAR)->getPartnershipDurationMth(Person::LOW).getMean();
		this->initproportionRegular = (1 - pHigh) * (regularRateL * regularDurationL) + pHigh *
		                              (regularRateH * regularDurationH);
		//Costs
		this->costs[CostsTracker::CONDOMS] =
		    _populationXML->FirstChildElement("costs")->FirstChildElement("condomCost")->GetText<double>();
		this->costs[CostsTracker::CIRCUMCISION] =
		    _populationXML->FirstChildElement("costs")->FirstChildElement("circumcisionCost")->GetText<double>();
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
		this->partnershipsHaveDuration[DmgProfile::MALE][type] = !(this->maleParams->getSexualBehaviorParams(
		            type)->getPartnershipDurationMth(Person::LOW).isZeroDistrib)
		        && !(this->maleParams->getSexualBehaviorParams(type)->getPartnershipDurationMth(Person::HIGH).isZeroDistrib);
		this->partnershipsHaveDuration[DmgProfile::FEMALE][type] = false;
	}

	_eventParams.displayOut("\n");
}

/**reloads certain xml data from another xml file in a sequence of files
   only loads non initial data
**/
void Population::Params::reloadXML(ticpp::Element *_populationXML, EventParams &_eventParams)
{
	assert(_populationXML != NULL);
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
				Male::updatePopParams(this->populationID, entityTypesIter->ToElement(), _eventParams);
				this->maleParams = Male::getPopParams(this->populationID);
			}
			else if(baseEntityElem.compare("Female") == 0)
			{
				Female::updatePopParams(this->populationID, entityTypesIter->ToElement(), _eventParams);
				this->femaleParams = Female::getPopParams(this->populationID);
			}
			else
			{
				_eventParams.displayOut("Population::Params::loadXML(...): Ignoring Unknown type :");
				_eventParams.displayOut(baseEntityElem.c_str());
				_eventParams.displayOut("\n");
			}
		}

		//Costs
		this->costs[CostsTracker::CONDOMS] =
		    _populationXML->FirstChildElement("costs")->FirstChildElement("condomCost")->GetText<double>();
		this->costs[CostsTracker::CIRCUMCISION] =
		    _populationXML->FirstChildElement("costs")->FirstChildElement("circumcisionCost")->GetText<double>();
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
		this->partnershipsHaveDuration[DmgProfile::MALE][type] = (!this->maleParams->getSexualBehaviorParams(
		            type)->getPartnershipDurationMth(Person::LOW).isZeroDistrib)
		        && (!this->maleParams->getSexualBehaviorParams(type)->getPartnershipDurationMth(Person::HIGH).isZeroDistrib);
		this->partnershipsHaveDuration[DmgProfile::FEMALE][type] = false;
	}

	_eventParams.displayOut("\n");
}

double Population::Params::getBirthRate()
{
	return this->birthRate;
}

//-------------< End Population::Params methods >-------------------//
