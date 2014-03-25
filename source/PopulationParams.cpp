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

void PopulationParams::init(const Inputs &inputs, unsigned int _populationID, EventParams &_eventParams)
{
	populationID = _populationID;
	
	_eventParams.displayOut("Population Parameters\n");
	std::string temp;

	try
	{
		initSize = inputs.GetPopulationSettings().initial_size;
		_eventParams.displayOut("\tsize = " + std::to_string(initSize) + "\n");

		//initial proportion married
		//TODO: Change me based on marriage acquisition rates et al
		//initproportionMarried  = initialState->FirstChildElement("proportionMarried")->GetText<double>();
		//initproportionRegular = initialState->FirstChildElement("proportionRegular")->GetText<double>();
		//get initial age distribution
		for(const auto &age_bucket : inputs.GetPopulationSettings().age_distributions)
		{
			initialAgeBuckets.push_back(
				new AgeBucketPrevalenceInfo(
					Util::convertTime(YEAR, MONTH, age_bucket.lower),
					Util::convertTime(YEAR, MONTH, age_bucket.upper) + 11,
					age_bucket.male_distribution,
					age_bucket.female_distribution,
					age_bucket.num_infected_male_csw,
					age_bucket.num_infected_female_csw,
					age_bucket.num_infected_male_low_risk,
					age_bucket.num_infected_female_low_risk,
					age_bucket.num_infected_male_high_risk,
					age_bucket.num_infected_female_high_risk));
		}

		initProbCSW[DmgProfile::MALE] = inputs.GetPopulationSettings().initial_chance_csw_male;
		initProbCSW[DmgProfile::FEMALE] = inputs.GetPopulationSettings().initial_chance_csw_female;
		CSWEndAgeMth[DmgProfile::MALE] = Util::convertTime(YEAR, MONTH, inputs.GetPopulationSettings().csw_end_age_male);
		CSWEndAgeMth[DmgProfile::FEMALE] = Util::convertTime(YEAR, MONTH, inputs.GetPopulationSettings().csw_end_age_female);

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
		birthRate = inputs.GetPopulationSettings().birth_rate;
		XMLUtil::printParam("", "birthRate", birthRate, _eventParams);
		proportionMale = inputs.GetPopulationSettings().proportion_male;
		XMLUtil::printParam("", "proportionMale", proportionMale, _eventParams);
		circumcised = inputs.GetPopulationSettings().proportion_circumcised;
		XMLUtil::printParam("", "proportionCircumcised", circumcised, _eventParams);
		SAEntAgeMths = inputs.GetPopulationSettings().age_sexual_debut;
		XMLUtil::printParam("", "SAEntAge (month)", SAEntAgeMths, _eventParams);
		//ASSORTATIVENESS GOES HERE
		assort[SexualPartnership::Type::Steady] = inputs.GetPopulationSettings().male_settings.partnership_settings[SexualPartnership::Type::Steady].assortativeness;
		assort[SexualPartnership::Type::Regular] = inputs.GetPopulationSettings().male_settings.partnership_settings[SexualPartnership::Type::Regular].assortativeness;
		assort[SexualPartnership::Type::Casual] = inputs.GetPopulationSettings().male_settings.partnership_settings[SexualPartnership::Type::Casual].assortativeness;
		assort[SexualPartnership::Type::Csw] = inputs.GetPopulationSettings().male_settings.partnership_settings[SexualPartnership::Type::Csw].assortativeness;

		Male::addPopParams(populationID, inputs.GetPopulationSettings().male_settings, _eventParams);
		maleParams = Male::getPopParams(populationID);

		Female::addPopParams(populationID, entityTypesIter->ToElement(), _eventParams);
		femaleParams = Female::getPopParams(populationID);

		//Get the initial marriage prevalence based on percent male high risk and rate and duration of steady relationships
		double pHigh = maleParams->getProportionHighRisk(DmgProfile::NON_CSW);
		double marriageRateH = maleParams->getSexualBehaviorParams(SexualPartnership::Type::Steady)->getAcquisitionRatePerMonth(
			Person::HIGH).getMean();
		double marriageRateL = maleParams->getSexualBehaviorParams(SexualPartnership::Type::Steady)->getAcquisitionRatePerMonth(
			Person::LOW).getMean();
		double marriageDurationH = maleParams->getSexualBehaviorParams(
			SexualPartnership::Type::Steady)->getPartnershipDurationMth(Person::HIGH).getMean();
		double marriageDurationL = maleParams->getSexualBehaviorParams(
			SexualPartnership::Type::Steady)->getPartnershipDurationMth(Person::LOW).getMean();
		initproportionMarried = (1 - pHigh) * (marriageRateL * marriageDurationL) / (1 + marriageRateL *
			marriageDurationL) + pHigh * (marriageRateH * marriageDurationH) / (1 + marriageRateH * marriageDurationH);
		//Get the initial regular prevalence based on percent male high risk and rate and duration of regular relationships
		double regularRateH = maleParams->getSexualBehaviorParams(SexualPartnership::Type::Regular)->getAcquisitionRatePerMonth(
			Person::HIGH).getMean();
		double regularRateL = maleParams->getSexualBehaviorParams(SexualPartnership::Type::Regular)->getAcquisitionRatePerMonth(
			Person::LOW).getMean();
		double regularDurationH = maleParams->getSexualBehaviorParams(
			SexualPartnership::Type::Regular)->getPartnershipDurationMth(Person::HIGH).getMean();
		double regularDurationL = maleParams->getSexualBehaviorParams(
			SexualPartnership::Type::Regular)->getPartnershipDurationMth(Person::LOW).getMean();
		initproportionRegular = (1 - pHigh) * (regularRateL * regularDurationL) + pHigh *
			(regularRateH * regularDurationH);
		//Costs
		condomCost = inputs.GetCosts().condom_cost;
		circumcisionCost = inputs.GetCosts().circumcision_cost;
	}
	catch(ticpp::Exception &_e)
	{
		_eventParams.displayOut("PopulationParams: Exception raised: ");
		_eventParams.displayOut(_e.m_details.c_str());
		_eventParams.displayOut("\n");
		Util::exitWithPrompt(-1);
	}

	//save flags to indicate whether particular partnership types have duration or not
	for(SexualPartnership::Type type = SexualPartnership::Type(0); type < SexualPartnership::Type::ENDType; ++type)
	{
		partnershipsHaveDuration[DmgProfile::MALE][type] = !(maleParams->getSexualBehaviorParams(
			type)->getPartnershipDurationMth(Person::LOW).isZeroDistrib)
			&& !(maleParams->getSexualBehaviorParams(type)->getPartnershipDurationMth(Person::HIGH).isZeroDistrib);
		partnershipsHaveDuration[DmgProfile::FEMALE][type] = false;
	}

	_eventParams.displayOut("\n");
}

double PopulationParams::getBirthRate() const
{
	return birthRate;
}

//-------------< End PopulationParams methods >-------------------//
