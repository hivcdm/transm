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
		: chanceCSW()
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

void PopulationParams::Serialize(pugi::xml_node &)
{
	throw std::runtime_error("not implemented");
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
	unsigned int ageBucketNum = initialAgeBuckets.size();

	for(unsigned int i = 0; i < ageBucketNum; ++i)
	{
		delete initialAgeBuckets.at(i);
	}
}

void PopulationParams::Deserialize(const pugi::xml_node &node)
{
	auto initial_state_node = node.child("initialState");
	initSize = initial_state_node.child("size").text().as_int();

	//get initial age distribution
	for(auto age_bucket_node : initial_state_node.child("ageDistributionYrs").children("range"))
	{
		initialAgeBuckets.push_back(
			new AgeBucketPrevalenceInfo(
				Util::convertTime(YEAR, MONTH, age_bucket_node.child("minAge").text().as_int()),
				Util::convertTime(YEAR, MONTH, age_bucket_node.child("maxAge").text().as_int()) + 11,
				age_bucket_node.child("distribMale").text().as_double(),
				age_bucket_node.child("distribFemale").text().as_double(),
				age_bucket_node.child("numInfectedMaleCSW").text().as_int(),
				age_bucket_node.child("numInfectedFemaleCSW").text().as_int(),
				age_bucket_node.child("numInfectedMaleLowRisk").text().as_int(),
				age_bucket_node.child("numInfectedFemaleLowRisk").text().as_int(),
				age_bucket_node.child("numInfectedMaleHighRisk").text().as_int(),
				age_bucket_node.child("numInfectedFemaleHighRisk").text().as_int()));
	}

	initProbCSW[DmgProfile::MALE] = initial_state_node.child("chanceBeingCSWMale").text().as_double();
	initProbCSW[DmgProfile::FEMALE] = initial_state_node.child("chanceBeingCSWFemale").text().as_double();
	CSWEndAgeMth[DmgProfile::MALE] = Util::convertTime(YEAR, MONTH, initial_state_node.child("CSWEndAgeMale").text().as_int());
	CSWEndAgeMth[DmgProfile::FEMALE] = Util::convertTime(YEAR, MONTH, initial_state_node.child("CSWEndAgeFemale").text().as_int());

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

	//dmgProfile parameters
	birthRate = node.child("birthRate").text().as_double();
	proportionMale = node.child("proportionMale").text().as_double();
	circumcised = node.child("proportionCircumcised").text().as_double();
	SAEntAgeMths = Util::convertTime(YEAR, MONTH, node.child("ageSexualDebutYrs").text().as_double());

	//ASSORTATIVENESS GOES HERE
	assort[(int)SexualPartnership::Type::Steady] = node.child("assortativeness").child("steady").text().as_double();
	assort[(int)SexualPartnership::Type::Regular] = node.child("assortativeness").child("regular").text().as_double();
	assort[(int)SexualPartnership::Type::Casual] = node.child("assortativeness").child("casual").text().as_double();
	assort[(int)SexualPartnership::Type::Csw] = node.child("assortativeness").child("csw").text().as_double();

	defaultMaleParams.Deserialize(node.select_single_node("entityTypes/baseEntities/baseEntity[type='Male']").node());
	defaultFemaleParams.Deserialize(node.select_single_node("entityTypes/baseEntities/baseEntity[type='Female']").node());

	//Get the initial marriage prevalence based on percent male high risk and rate and duration of steady relationships
	double pHigh = defaultMaleParams.getProportionHighRisk(DmgProfile::NON_CSW);
	double marriageRateH = defaultMaleParams.getSexualBehaviorParams(SexualPartnership::Type::Steady).getAcquisitionRatePerMonth(
		Person::HIGH).getMean();
	double marriageRateL = defaultMaleParams.getSexualBehaviorParams(SexualPartnership::Type::Steady).getAcquisitionRatePerMonth(
		Person::LOW).getMean();
	double marriageDurationH = defaultMaleParams.getSexualBehaviorParams(
		SexualPartnership::Type::Steady).getPartnershipDurationMth(Person::HIGH).getMean();
	double marriageDurationL = defaultMaleParams.getSexualBehaviorParams(
		SexualPartnership::Type::Steady).getPartnershipDurationMth(Person::LOW).getMean();
	initproportionMarried = (1 - pHigh) * (marriageRateL * marriageDurationL) / (1 + marriageRateL *
		marriageDurationL) + pHigh * (marriageRateH * marriageDurationH) / (1 + marriageRateH * marriageDurationH);
	//Get the initial regular prevalence based on percent male high risk and rate and duration of regular relationships
	double regularRateH = defaultMaleParams.getSexualBehaviorParams(SexualPartnership::Type::Regular).getAcquisitionRatePerMonth(
		Person::HIGH).getMean();
	double regularRateL = defaultMaleParams.getSexualBehaviorParams(SexualPartnership::Type::Regular).getAcquisitionRatePerMonth(
		Person::LOW).getMean();
	double regularDurationH = defaultMaleParams.getSexualBehaviorParams(
		SexualPartnership::Type::Regular).getPartnershipDurationMth(Person::HIGH).getMean();
	double regularDurationL = defaultMaleParams.getSexualBehaviorParams(
		SexualPartnership::Type::Regular).getPartnershipDurationMth(Person::LOW).getMean();
	initproportionRegular = (1 - pHigh) * (regularRateL * regularDurationL) + pHigh *
		(regularRateH * regularDurationH);

	//Costs
	condomCost = node.child("costs").child("condomCost").text().as_double();
	circumcisionCost = node.child("costs").child("circumcisionCost").text().as_double();

	//save flags to indicate whether particular partnership types have duration or not
	for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
	{
		partnershipsHaveDuration[DmgProfile::MALE][type] = 
			!(defaultMaleParams.getSexualBehaviorParams(SexualPartnership::Type(type)).getPartnershipDurationMth(Person::LOW).isZeroDistrib)
			&& !(defaultMaleParams.getSexualBehaviorParams(SexualPartnership::Type(type)).getPartnershipDurationMth(Person::HIGH).isZeroDistrib);
		partnershipsHaveDuration[DmgProfile::FEMALE][type] = false;
	}
}

double PopulationParams::getBirthRate() const
{
	return birthRate;
}
