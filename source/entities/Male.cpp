#include <vector>

#include "Male.h"
#include "classifiers/SexualPartnership.h"
#include "behaviors/SexualBehaviorParams.h"
#include "entitypool/EntityPool.h"
#include "../util/Util.h"
#include "../util/enum_iterator.h"
#include "../util/XMLUtil.h"
#include "../util/rand/RandomNums.h"

//-----------------< Begin population-level parameters for males >-----------------------/
//each index of the array contains parameters for a different population
//(we only have 1 population for now so the size of the vector will default to 1

Male::SubPopParams::SubPopParams()
{
}

Male::SubPopParams::SubPopParams(const PopulationSettings::MaleSettings &settings, EventParams &_eventParams)
{
	try
	{
		chanceBecomeCSW = settings.chance_become_sex_worker;
		partnerAcqMultWithSteady[Person::HIGH] = settings.partner_acquisition_multiplier_with_steady_high;
		partnerAcqMultWithSteady[Person::LOW] = settings.partner_acquisition_multiplier_with_steady_low;

		//Coefficient of Variation
		_eventParams.useCoefficientVariation = settings.use_coefficient_variation;
		_eventParams.coefficientOfVariation = settings.coefficient_of_variation;

		//iterate through each Person in partnershipTypes
		sexualBehaviorParams.clear();

		for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
		{
			auto params = new SexualBehaviorParams(settings, partnership_type, _eventParams);
			sexualBehaviorParams.push_back(params);
		}

		proportionHighRisk[DmgProfile::CSW] = settings.proportion_high_risk_csw;
		proportionHighRisk[DmgProfile::NON_CSW] = settings.proportion_high_risk_non_csw;

		activityLevel = settings.activity_level;

		//saves partner acq rate and acts discounting
		partneringDiscStartAgeYrs = settings.age_discounting_start_age;
		partneringAcqDiscPerYr = settings.acquisition_rate_discounting_yearly;
		partneringActsDiscPerYr = settings.coital_acts_discounting_yearly;

		int numMults = Person::maxYrForDeathStats - partneringDiscStartAgeYrs + 1;
		double acqMult = 1 - partneringAcqDiscPerYr;
		double actsMult = 1 - partneringActsDiscPerYr;

		//generate vectors that contain discount multipliers. will cover from [partneringDiscStartAgeYrs,Person::maxYrForDeathStats]
		partneringAcqDiscMult.clear();
		partneringActsDiscMult.clear();
		partneringAcqDiscMult.push_back(acqMult);
		partneringActsDiscMult.push_back(actsMult);

		for(int i = 1; i < numMults; ++i)
		{
			partneringAcqDiscMult.push_back(partneringAcqDiscMult.at(i - 1)*acqMult);
			partneringActsDiscMult.push_back(partneringActsDiscMult.at(i - 1)*actsMult);
		}

		circumProtectEff = settings.circumcision_protection_efficacy;
		condomProtectEff = settings.condom_protection_efficacy;
		transmitPerEventCoeffs.assign(settings.transmission_coefficients.begin(), settings.transmission_coefficients.end());
	}
	catch(ticpp::Exception &_e)
	{
		throw std::runtime_error("Male: Exception raised: " + _e.m_details);
	}
}

Male::SubPopParams::~SubPopParams()
{
	for(unsigned int i = 0; i < sexualBehaviorParams.size(); ++i)
	{
		delete sexualBehaviorParams.at(i);
	}
}

//------------ < Begin getters >-----------------//
double Male::SubPopParams::getChanceBecomeCSW() const
{
	return chanceBecomeCSW;
}

double Male::SubPopParams::getPartnerAcqMultWithSteady(Person::RiskLevel _risk) const
{
	return partnerAcqMultWithSteady[_risk];
}

//sexual behavior params for each type as specified by SexualPartnership::Type
const SexualBehaviorParams *Male::SubPopParams::getSexualBehaviorParams(SexualPartnership::Type _type) const
{
	return sexualBehaviorParams.at((int)_type);
}

double Male::SubPopParams::getProportionHighRisk(DmgProfile::Employment _cswStatus) const
{
	return proportionHighRisk[_cswStatus];
}
NormalDist Male::SubPopParams::getActivityLevel() const
{
	return activityLevel;
}

double Male::SubPopParams::getCircumProtectEff()  const
{
	return circumProtectEff;
}

double Male::SubPopParams::getCondomProtectEff()  const
{
	return condomProtectEff;
}

int Male::SubPopParams::getPartneringDiscStartAgeYrs() const
{
	return partneringDiscStartAgeYrs;
}

double Male::SubPopParams::getPartneringAcqDiscMult(int _ageYrs) const
{
	assert(Util::withinRange(_ageYrs, 0, Person::maxYrForDeathStats));
	return partneringAcqDiscMult.at(_ageYrs - partneringDiscStartAgeYrs);
}

double Male::SubPopParams::getPartneringActsDiscMult(int _ageYrs) const
{
	assert(Util::withinRange(_ageYrs, 0, Person::maxYrForDeathStats));
	return partneringActsDiscMult.at(_ageYrs - partneringDiscStartAgeYrs);
}

double Male::SubPopParams::getTransmitPerEventCoeff(HVLStrata _hvl) const
{
	assert(Util::withinRange(_hvl, Person::HVLStrata(0), Person::HVLStrata(transmitPerEventCoeffs.size() - 1)));
	return transmitPerEventCoeffs.at(_hvl);
}

void Male::SubPopParams::setChanceCondomUsePerEvent(Person::RiskLevel risk, SexualPartnership::Type partnershipType, BetaDist dist)
{
	sexualBehaviorParams[(int)partnershipType]->setChanceCondomUsePerEvent(risk, dist);
}

//------------ < End getters >-----------------//

//-----------------< End population-level parameters for males >-----------------------/


vector<Male::SubPopParams *> Male::populationSpecificParams;

Male::SubPopParams *Male::getPopParams(unsigned int _populationID)
{
	assert(_populationID < Male::populationSpecificParams.size());
	return Male::populationSpecificParams.at(_populationID);
}

void Male::addPopParams(unsigned int _populationID, const PopulationSettings::MaleSettings &settings, EventParams &_eventParams)
{
	assert(_populationID == Male::populationSpecificParams.size());
	Male::SubPopParams *subPopParams = new Male::SubPopParams(settings, _eventParams);
	Male::populationSpecificParams.push_back(subPopParams);
}

Male::Male(EventParams &_eventParams, int _age, bool _circumcised, unsigned int _populationID)
	: Person(_eventParams, _age, _populationID)
{
	//Set the graphNode to being male!
	//TODO: This is inelegantly placed and kind of a hack right now
	graphNode->isMale = true;

	//If age is out of range, set it at the closest boundary.
	if(!Util::withinRange<int>(_age, 0, Util::convertTime(YEAR, MONTH, Person::maxYrForDeathStats)))
	{
		if(_age < 0)
		{
			_age = 0;
		}
		else
		{
			_age = Util::convertTime(YEAR, MONTH, Person::maxYrForDeathStats);
		}
	}

	dmgProfile.set(DmgProfile::GENDER, DmgProfile::MALE);
	//check that the DmgProfile::RelationshipStatus is actually of a male
	const Male::SubPopParams *maleSubPopParams = getPopParams(populationID);
	circumcised = _circumcised;
	//Set this male's risk level assume everyone is low risk on creation. Risk is rerolled when they roll for become sex worker
	risk = Person::LOW;

	for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
	{
		auto &sexualBehaviorParams = *maleSubPopParams->getSexualBehaviorParams(partnership_type);

		auto acquisition_rate_dist = sexualBehaviorParams.getAcquisitionRatePerMonth(risk);
		auto acquisition_rate = _eventParams.randomNums.randLogNormal(acquisition_rate_dist);
		partnerAcqRates[(int)partnership_type] = acquisition_rate;
			
		numActsPerMonth[(int)partnership_type] = sexualBehaviorParams.getCoitalEventsPerMonth(risk);

		auto chance_condom_use_dist = sexualBehaviorParams.getChanceCondomUsePerEvent(risk);
		auto chance_condom_use = _eventParams.randomNums.randBeta(chance_condom_use_dist);
		chanceCondomUsePerEvent[(int)partnership_type] = chance_condom_use;

		averageYearsYounger[(int)partnership_type] = sexualBehaviorParams.getAverageYearsYounger();
	}

	activityLevel = _eventParams.randomNums.randNorm_NaturalNum(maleSubPopParams->getActivityLevel());

	//activity level should not ever be 0
	if(activityLevel == 0)
	{
		activityLevel = 1;
	}
}

Male::~Male(void)
{
}



//-------------< BEGIN methods that are for Males only >------------------------//


double Male::getCondomUseProb(Person *_p, SexualPartnership::Type _partnershipType)
{
	assert((_p != nullptr));
	assert(_p->isAlive());
	assert(_partnershipType < SexualPartnership::Type::ENDType);
	return chanceCondomUsePerEvent[(int)_partnershipType];
}

double Male::getCircumProtectEff()
{
	return (circumcised ? Male::getPopParams(populationID)->getCircumProtectEff() : 0);
}

double Male::getCondomProtectEff()
{
	return Male::getPopParams(populationID)->getCondomProtectEff();
}

bool Male::isCircumcised()
{
	return circumcised;
}

//-------------< END methods that are for Males only >------------------------//


//-----------------< Start methods which are inherited from Person >-----------------//


//in this case, the male is infected and female is uninfected
double Male::getFOI(Person *_p, SexualPartnership::Type _partnershipType, EventParams &_eventParams)
{
	//note: in the case of male->female transmission, circumcision makes no difference
	//transmission coeff				1-	(condoms are used and succeed)
	assert(Util::validProbability(getCondomUseProb(_p, _partnershipType)));
	assert(Util::validProbability(getCondomProtectEff()));
	assert((_p != nullptr));
	assert(_p->isAlive());
	assert(_partnershipType < SexualPartnership::Type::ENDType);
	//Determine if a condom was used and record
	condomUsedLastFOICalculation = _eventParams.randomNums.chance(getCondomUseProb(_p, _partnershipType));
	//Determine the condom efficacy --> 0 if no condom was used
	double condomEff = 0;

	if(condomUsedLastFOICalculation)
	{
		condomEff = getCondomProtectEff();
	}

	double FOI = getTransmissionCoeff() *	(1 - condomEff);

	if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && (trace() || _p->trace()))
	{
		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " !Transmission coefficient from " << getID() << " to " <<
		        _p->getID() << " is " << getTransmissionCoeff();
		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << ";" << endl << " !A condom was ";

		if(!condomUsedLastFOICalculation)
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "NOT ";
		}

		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "used (efficacy " << getCondomProtectEff();
		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << ");" << endl << " !Total FOI = " << FOI << endl;
	}

	return FOI;
}


double Male::getMinPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const
{
	switch(_PSC)
	{
	case Person::AGE:
	{
		if(getAge(MONTH) - (12 * averageYearsYounger[(int)_partnershipType].mean + 6) > 0)
		{
			return getAge(MONTH) - (12 * averageYearsYounger[(int)_partnershipType].mean + 6);
			break;
		}
		else
		{
			return 0;
			break;
		}
	}
	case ID:
	{
		return numeric_limits<double>::min();
	}
	}
	std::cerr << "Invalid Sorting key :" << _PSC;
	Util::exitWithPrompt(-1);
}


double Male::getMaxPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const
{
	switch(_PSC)
	{
	case Person::AGE:
	{
		if(getAge(MONTH) - (12 * averageYearsYounger[(int)_partnershipType].mean - 6) > 0)
		{
			return getAge(MONTH) - (12 * averageYearsYounger[(int)_partnershipType].mean - 6);
			break;
		}
		else
		{
			return 0;
			break;
		}
	}
	case ID:
	{
		return numeric_limits<double>::max();
	}
	}

	cerr << "Invalid Sorting key :" << _PSC;
	Util::exitWithPrompt(-1);
}

double Male::rollForAgeDifference(SexualPartnership::Type _partnershipType, RandomNums &_randomNums)
{
	double ageDifference = _randomNums.randNorm(averageYearsYounger[(int)_partnershipType]);
	return ageDifference;
}

double Male::getTransmissionCoeff()
{
	assert(Util::withinRange(hvl, HVL_ZERO, HVL_LATESTAGE));
	return Male::populationSpecificParams.at(populationID)->getTransmitPerEventCoeff(hvl);
}


bool Male::possibleMatch(SexualPartnership::Type _partnershipType, Person *_p)
{
	assert((_p != nullptr));
	assert(_p->isAlive());
	assert(_partnershipType < SexualPartnership::Type::ENDType);
	assert(false);  // check if we are using years instead of Month
	int minAge = static_cast<int>(getMinPartnerSelectVal(Person::AGE, _partnershipType));
	int maxAge = static_cast<int>(getMaxPartnerSelectVal(Person::AGE, _partnershipType));
	return Util::withinRange(_p->getAge(MONTH), minAge, maxAge);
}

int Male::rollForNumPartners(RandomNums &_randomNums, SexualPartnership::Type _partnershipType)
{
	assert(_partnershipType < SexualPartnership::Type::ENDType);

	//person can only have 1 steady partner at a time so return 0 if person is already in Steady
	if((_partnershipType == SexualPartnership::Type::Steady) && (!partners[(int)_partnershipType].empty()))
	{
		return 0;
	}

	//Male parameters for the population that this Male is in
	const Male::SubPopParams *subPopParams = getPopParams(populationID);
	//rate of acquiring partner
	double partnerRate;
	partnerRate = partnerAcqRates[(int)_partnershipType];

	//if this person has a steady partner then adjust acquisition rate
	if(!partners[(int)SexualPartnership::Type::Steady].empty())
	{
		//if we're thinking of getting another partner, then lower chances if we have a steady partner
		partnerRate *= subPopParams->getPartnerAcqMultWithSteady(getRiskLevel());
	}

	//if person is over the age of partnering discounting, then discount acquisition rate
	int ageYrs = getAge(YEAR);

	if(ageYrs >= subPopParams->getPartneringDiscStartAgeYrs())
	{
		partnerRate *= subPopParams->getPartneringAcqDiscMult(ageYrs);
	}

	/** To get the number of partners to draw this month, draw from a Poisson distribution */
	int numPartners = _randomNums.randPoisson(partnerRate);
	//if we are rolling for STEADY, make sure we have max of 1
	return (_partnershipType != SexualPartnership::Type::Steady) ? numPartners : min(1, numPartners);
}

int Male::rollNumEventsPerPartner(Person *_p, RandomNums &_randomNums, SexualPartnership::Type _partnershipType)
{
	assert((_p != nullptr));
	assert(_p->isAlive());

	double meanCoitalEvents = numActsPerMonth[(int)_partnershipType];
	const Male::SubPopParams *subPopParams = getPopParams(populationID);
	//if person is over the age of partnering discounting, then discount #acts
	int ageYrs = getAge(YEAR);

	if(ageYrs >= subPopParams->getPartneringDiscStartAgeYrs())
	{
		meanCoitalEvents *= subPopParams->getPartneringActsDiscMult(ageYrs);
	}

	//Poisson distributions range from 0 to infinity: we want to avoid 0 acts per month
	//If meanCoitalEvents is less than 1 (happens after discounting), set to one (force a minimum)
	if(meanCoitalEvents < 1)
	{
		meanCoitalEvents = 1;
	}

	int numActs = _randomNums.randPoisson(meanCoitalEvents - 1) + 1;
	return numActs;
}


int Male::rollForNewPartnershipDuration(SexualPartnership::Type _partnershipType, RandomNums &_randomNums, Person *_p)
{
	assert((_p != nullptr));
	assert(_p->isAlive());
	assert(_partnershipType < SexualPartnership::Type::ENDType);
	ShiftedLogNormalDist duration = Male::populationSpecificParams.at(populationID)->getSexualBehaviorParams(
	                                    _partnershipType)->getPartnershipDurationMth(risk);
	return (int)(_randomNums.randShiftedLogNormal(duration) + .5);
}

void Male::rerollRiskGroup(EventParams &_eventParams)
{
	DmgProfile::Employment cswStatus = (DmgProfile::Employment) getDmgProfileVal(DmgProfile::EMPLOYMENT);
	double chanceHighRisk = getPopParams(populationID)->getProportionHighRisk(cswStatus);
	Person::RiskLevel oldRisk = risk;

	if(_eventParams.randomNums.chance(chanceHighRisk))
	{
		risk = HIGH;
	}
	else
	{
		risk = LOW;
	}

	if(oldRisk != risk)
	{
		const Male::SubPopParams *maleSubPopParams = getPopParams(populationID);

		for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
		{
			const SexualBehaviorParams *sexualBehaviorParams = 
				maleSubPopParams->getSexualBehaviorParams(partnership_type);
			auto acquisition_rate = sexualBehaviorParams->getAcquisitionRatePerMonth(risk);
			partnerAcqRates[(int)partnership_type] = 
				_eventParams.randomNums.randLogNormal(acquisition_rate);
			numActsPerMonth[(int)partnership_type] = 
				sexualBehaviorParams->getCoitalEventsPerMonth(risk);
			auto chance_condom_use = sexualBehaviorParams->getChanceCondomUsePerEvent(risk);
			chanceCondomUsePerEvent[(int)partnership_type] = 
				_eventParams.randomNums.randBeta(chance_condom_use);
		}
	}

	if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && trace())
	{
		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " % Male " << getID() << " rerolls as ";

		if(risk == HIGH)
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "High";
		}
		else
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "Low";
		}

		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " risk" << endl;
	}
}

void Male::saveState(ostream &_outStream, long currTime)
{
	_outStream << "gend:m," << endl;
	Person::saveState(_outStream, currTime);
	_outStream << "," << endl << "circ:" << circumcised << "," << endl;
	//partner acquisition rates
	bool firstInSequence = true;
	_outStream << "partAcqR:[";

	for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
	{
		if(!firstInSequence)
		{
			_outStream << ",";
		}

		firstInSequence = false;
		_outStream << partnerAcqRates[(int)partnership_type];
	}

	_outStream << "]," << endl;
	//Acts per month
	firstInSequence = true;
	_outStream << "actsPerMth:[";

	for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
	{
		if(!firstInSequence)
		{
			_outStream << ",";
		}

		firstInSequence = false;
		_outStream << numActsPerMonth[(int)partnership_type];
	}

	_outStream << "]," << endl;
	//Acts per month
	firstInSequence = true;
	_outStream << "probCndm:[";

	for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
	{
		if(!firstInSequence)
		{
			_outStream << ",";
		}

		firstInSequence = false;
		_outStream << chanceCondomUsePerEvent[(int)partnership_type];
	}

	_outStream << "]";
}

//-----------------< End methods which are inherited from Person >-----------------//
