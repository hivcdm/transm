#include <vector>

#include "Male.h"
#include "classifiers/SexualPartnership.h"
#include "behaviors/SexualBehavior.h"
#include "entitypool/EntityPool.h"
#include "../util/Utility.h"
#include "../util/enum_iterator.h"
#include "../util/rand/RandomNumberGenerator.h"

//each index of the array contains parameters for a different population
//(we only have 1 population for now so the size of the vector will default to 1

Male::SubPopParams::SubPopParams()
{
}

Male::SubPopParams::~SubPopParams()
{
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
const SexualBehavior &Male::SubPopParams::getSexualBehavior(SexualPartnership::Type _type) const
{
	return sexualBehaviorParams.at((int)_type);
}

//sexual behavior params for each type as specified by SexualPartnership::Type
SexualBehavior &Male::SubPopParams::getSexualBehavior(SexualPartnership::Type _type)
{
	return sexualBehaviorParams.at((int)_type);
}


double Male::SubPopParams::getProportionHighRisk(DemographicProfile::Employment _cswStatus) const
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
	assert(Utility::withinRange(_ageYrs, 0, Person::maxYrForDeathStats));
	return partneringAcqDiscMult.at(_ageYrs - partneringDiscStartAgeYrs);
}

double Male::SubPopParams::getPartneringActsDiscMult(int _ageYrs) const
{
	assert(Utility::withinRange(_ageYrs, 0, Person::maxYrForDeathStats));
	return partneringActsDiscMult.at(_ageYrs - partneringDiscStartAgeYrs);
}

double Male::SubPopParams::getTransmitPerEventCoeff(HVLStrata _hvl) const
{
	assert(Utility::withinRange(_hvl, Person::HVLStrata(0), Person::HVLStrata(transmitPerEventCoeffs.size() - 1)));
	return transmitPerEventCoeffs.at(_hvl);
}

void Male::SetChanceCondomUsePerEvent(RiskLevel risk, SexualPartnership::Type partnershipType, BetaDist dist, RandomNumberGenerator &rng)
{
	populationSpecificParams.getSexualBehavior(partnershipType).setChanceCondomUsePerEvent(risk, dist);
    chanceCondomUsePerEvent[(int)partnershipType] = rng.randBeta(dist);
}

void Male::SetCoitalEventsPerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, double meanEvents)
{
	populationSpecificParams.getSexualBehavior(partnershipType).setCoitalEventsPerMonth(risk, meanEvents);
    numActsPerMonth[(int)partnershipType] = meanEvents;
}

void Male::SetPartnershipDuration(RiskLevel risk, SexualPartnership::Type partnershipType, ShiftedLogNormalDist dist)
{
	populationSpecificParams.getSexualBehavior(partnershipType).setPartnershipDuration(risk, dist);
}

void Male::SetAverageYearsYounger(SexualPartnership::Type partnershipType, NormalDist dist)
{
	populationSpecificParams.getSexualBehavior(partnershipType).setAverageYearsYounger(dist);
    averageYearsYounger[(int)partnershipType] = dist;
}

void Male::SetAcquisitionRatePerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, LogNormalDist dist, RandomNumberGenerator &rng)
{
	populationSpecificParams.getSexualBehavior(partnershipType).setAcquisitionRatePerMonth(risk, dist);
    partnerAcqRates[(int)partnershipType] = rng.randLogNormal(dist);
}

double Male::getChanceBecomeCsw() const
{
	return populationSpecificParams.getChanceBecomeCSW();
}

void Male::Circumcise()
{
	circumcised = true;
}

Male::Male(EventParams &_eventParams, int _age, bool _circumcised, unsigned int _populationID, const Male::SubPopParams &params)
	: Person(_age, _populationID),
	populationSpecificParams(params)
{
	//If age is out of range, set it at the closest boundary.
    if(!Utility::withinRange<int>(_age, 0, Utility::convertTime(TimeGranularity::Year, TimeGranularity::Month, Person::maxYrForDeathStats)))
	{
		if(_age < 0)
		{
			_age = 0;
		}
		else
		{
            _age = Utility::convertTime(TimeGranularity::Year, TimeGranularity::Month, Person::maxYrForDeathStats);
		}
	}

	dmgProfile.set(DemographicProfile::GENDER, DemographicProfile::MALE);

	circumcised = _circumcised;
	//Set this male's risk level assume everyone is low risk on creation. Risk is rerolled when they roll for become sex worker
	risk = Person::LOW;

	for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
	{
		auto &sexualBehaviorParams = populationSpecificParams.getSexualBehavior(partnership_type);

		auto acquisition_rate_dist = sexualBehaviorParams.getAcquisitionRatePerMonth(risk);
		auto acquisition_rate = _eventParams.randomNums.randLogNormal(acquisition_rate_dist);
		partnerAcqRates[(int)partnership_type] = acquisition_rate;
			
		numActsPerMonth[(int)partnership_type] = sexualBehaviorParams.getCoitalEventsPerMonth(risk);

		auto chance_condom_use_dist = sexualBehaviorParams.getChanceCondomUsePerEvent(risk);
		auto chance_condom_use = _eventParams.randomNums.randBeta(chance_condom_use_dist);
		chanceCondomUsePerEvent[(int)partnership_type] = chance_condom_use;

		averageYearsYounger[(int)partnership_type] = sexualBehaviorParams.getAverageYearsYounger();
	}

	activityLevel = _eventParams.randomNums.randNorm_NaturalNum(populationSpecificParams.getActivityLevel());

	//activity level should not ever be 0
	if(activityLevel == 0)
	{
		activityLevel = 1;
	}
}

Male::~Male()
{
}

double Male::getCondomUseProb(Person *_p, SexualPartnership::Type _partnershipType)
{
	assert((_p != nullptr));
	assert(_p->isAlive());
	assert(_partnershipType < SexualPartnership::Type::ENDType);
	return chanceCondomUsePerEvent[(int)_partnershipType];
}

double Male::getCircumProtectEff()
{
	return (circumcised ? populationSpecificParams.getCircumProtectEff() : 0);
}

double Male::getCondomProtectEff()
{
	return populationSpecificParams.getCondomProtectEff();
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
	assert(Utility::validProbability(getCondomProtectEff()));
	assert((_p != nullptr));
	assert(_p->isAlive());
	assert(_partnershipType < SexualPartnership::Type::ENDType);
	//Determine if a condom was used and record
    double chanceCondomUse = getCondomUseProb(_p, _partnershipType);
    if(_p->HasOverrideChanceCondomUse())
    {
        chanceCondomUse = _p->GetOverrideChanceCondomUse();
    }
    assert(Utility::validProbability(chanceCondomUse));
	condomUsedLastFOICalculation = _eventParams.randomNums.chance(chanceCondomUse);
	//Determine the condom efficacy --> 0 if no condom was used
	double condomEff = 0;

	if(condomUsedLastFOICalculation)
	{
		condomEff = getCondomProtectEff();
	}

	double FOI = getTransmissionCoeff() *	(1 - condomEff);

    if(_eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && (trace() || _p->trace()))
	{
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " !Transmission coefficient from " << getID() << " to " <<
		        _p->getID() << " is " << getTransmissionCoeff();
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << ";" << std::endl << " !A condom was ";

		if(!condomUsedLastFOICalculation)
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "NOT ";
		}

        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "used (efficacy " << getCondomProtectEff();
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << ");" << std::endl << " !Total FOI = " << FOI << std::endl;
	}

	return FOI;
}


double Male::getMinPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const
{
	switch(_PSC)
	{
	case Person::AGE:
	{
        if(getAge(TimeGranularity::Month) - (12 * averageYearsYounger[(int)_partnershipType].mean + 6) > 0)
		{
            return getAge(TimeGranularity::Month) - (12 * averageYearsYounger[(int)_partnershipType].mean + 6);
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
	case SEXUAL_ACTIVITY_LEVEL:
	    throw std::runtime_error("not implemented");
	case ENDSelectingCriteria:
	    throw std::runtime_error("Invalid Sorting key");
	}
	throw std::runtime_error("Invalid Sorting key");
}


double Male::getMaxPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const
{
	switch(_PSC)
	{
	case Person::AGE:
	{
        if(getAge(TimeGranularity::Month) - (12 * averageYearsYounger[(int)_partnershipType].mean - 6) > 0)
		{
            return getAge(TimeGranularity::Month) - (12 * averageYearsYounger[(int)_partnershipType].mean - 6);
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
	case SEXUAL_ACTIVITY_LEVEL:
	default:
	    throw std::runtime_error("Invalid Sorting key");
	}
}

double Male::rollForAgeDifference(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums)
{
	double ageDifference = _randomNums.randNorm(averageYearsYounger[(int)_partnershipType]);
	return ageDifference;
}

double Male::getTransmissionCoeff()
{
	assert(Utility::withinRange(hvl, HVL_ZERO, HVL_LATESTAGE));
	return populationSpecificParams.getTransmitPerEventCoeff(hvl);
}


bool Male::possibleMatch(SexualPartnership::Type _partnershipType, Person *_p)
{
	assert((_p != nullptr));
	assert(_p->isAlive());
	assert(_partnershipType < SexualPartnership::Type::ENDType);
	assert(false);  // check if we are using years instead of Month
	int minAge = static_cast<int>(getMinPartnerSelectVal(Person::AGE, _partnershipType));
	int maxAge = static_cast<int>(getMaxPartnerSelectVal(Person::AGE, _partnershipType));
    return Utility::withinRange(_p->getAge(TimeGranularity::Month), minAge, maxAge);
}

int Male::rollForNumPartners(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType)
{
	assert(_partnershipType < SexualPartnership::Type::ENDType);

	//person can only have 1 steady partner at a time so return 0 if person is already in Steady
	if((_partnershipType == SexualPartnership::Type::Steady) && (!partners[(int)_partnershipType].empty()))
	{
		return 0;
	}

	//rate of acquiring partner
	double partnerRate;
	partnerRate = partnerAcqRates[(int)_partnershipType];

	//if this person has a steady partner then adjust acquisition rate
	if(!partners[(int)SexualPartnership::Type::Steady].empty())
	{
		//if we're thinking of getting another partner, then lower chances if we have a steady partner
		partnerRate *= populationSpecificParams.getPartnerAcqMultWithSteady(getRiskLevel());
	}

	//if person is over the age of partnering discounting, then discount acquisition rate
    int ageYrs = getAge(TimeGranularity::Year);

	if(ageYrs >= populationSpecificParams.getPartneringDiscStartAgeYrs())
	{
		partnerRate *= populationSpecificParams.getPartneringAcqDiscMult(ageYrs);
	}

	/** To get the number of partners to draw this month, draw from a Poisson distribution */
	int numPartners = _randomNums.randPoisson(partnerRate);
	//if we are rolling for STEADY, make sure we have max of 1
	return (_partnershipType != SexualPartnership::Type::Steady) ? numPartners : min(1, numPartners);
}

int Male::rollNumEventsPerPartner(Person *_p, RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType)
{
	assert((_p != nullptr));
	assert(_p->isAlive());

	double meanCoitalEvents = numActsPerMonth[(int)_partnershipType];

	//if person is over the age of partnering discounting, then discount #acts
    int ageYrs = getAge(TimeGranularity::Year);

	if(ageYrs >= populationSpecificParams.getPartneringDiscStartAgeYrs())
	{
		meanCoitalEvents *= populationSpecificParams.getPartneringActsDiscMult(ageYrs);
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


int Male::rollForNewPartnershipDuration(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums, Person *_p)
{
	assert((_p != nullptr));
	assert(_p->isAlive());
	assert(_partnershipType < SexualPartnership::Type::ENDType);
	ShiftedLogNormalDist duration = populationSpecificParams.getSexualBehavior(_partnershipType).getPartnershipDurationMth(risk);
	return (int)(_randomNums.randShiftedLogNormal(duration) + .5);
}

void Male::rerollRiskGroup(EventParams &_eventParams)
{
	DemographicProfile::Employment cswStatus = (DemographicProfile::Employment) getDemographicProfileVal(DemographicProfile::EMPLOYMENT);
	double chanceHighRisk = populationSpecificParams.getProportionHighRisk(cswStatus);
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
		for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
		{
			const SexualBehavior &sexualBehaviorParams = 
				populationSpecificParams.getSexualBehavior(partnership_type);
			auto acquisition_rate = sexualBehaviorParams.getAcquisitionRatePerMonth(risk);
			partnerAcqRates[(int)partnership_type] = 
				_eventParams.randomNums.randLogNormal(acquisition_rate);
			numActsPerMonth[(int)partnership_type] = 
				sexualBehaviorParams.getCoitalEventsPerMonth(risk);
			auto chance_condom_use = sexualBehaviorParams.getChanceCondomUsePerEvent(risk);
			chanceCondomUsePerEvent[(int)partnership_type] = 
				_eventParams.randomNums.randBeta(chance_condom_use);
		}
	}

    if(_eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && trace())
	{
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " % Male " << getID() << " rerolls as ";

		if(risk == HIGH)
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "High";
		}
		else
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "Low";
		}

        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " risk" << std::endl;
	}
}

void Male::saveState(ostream &_outStream, long currTime)
{
	_outStream << "gend:m," << std::endl;
	Person::saveState(_outStream, currTime);
	_outStream << "," << std::endl << "circ:" << circumcised << "," << std::endl;
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

	_outStream << "]," << std::endl;
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

	_outStream << "]," << std::endl;
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
