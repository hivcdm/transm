#include <vector>

#include "Male.h"
#include "classifiers/SexualPartnership.h"
#include "behaviors/SexualBehaviorParams.h"
#include "entitypool/EntityPool.h"
#include "../util/Util.h"
#include "../util/XMLUtil.h"
#include "../util/rand/RandomNums.h"

//-----------------< Begin population-level parameters for males >-----------------------/
//each index of the array contains parameters for a different population
//(we only have 1 population for now so the size of the vector will default to 1

Male::SubPopParams::SubPopParams()
{
}

Male::SubPopParams::SubPopParams(ticpp::Element *_maleParams, EventParams &_eventParams)
{
	assert(_maleParams != nullptr);
	loadParamsXML(_maleParams, _eventParams);
}

/***
The order we read parameters in should match the ordering of the static variables in class Male
***/

//was throw(...)
int Male::SubPopParams::loadParamsXML(ticpp::Element *_maleParams, EventParams &_eventParams) throw()
{
	assert(_maleParams != nullptr);

	try
	{
		ticpp::Element *behaviorElem = _maleParams->FirstChildElement("behavior");
		chanceBecomeCSW = behaviorElem->FirstChildElement("chanceBecomeSexWorker")->GetText<double>();
		partnerAcqMultWithSteady[Person::HIGH] =
		    behaviorElem->FirstChildElement("partnerAcqMultWithSteadyHighRisk")->GetText<double>();
		partnerAcqMultWithSteady[Person::LOW] =
		    behaviorElem->FirstChildElement("partnerAcqMultWithSteadyLowRisk")->GetText<double>();
		//Use these to determine if the high risk acquisition rates will be pulled from user input or by multiplying the low risk rates
		bool useMultiplierForHighRiskAcqRates = false;

		try
		{
			useMultiplierForHighRiskAcqRates = (behaviorElem->FirstChildElement("UseHighRiskMultiplier")->GetText<int>() == 1);
		}
		catch(ticpp::Exception &)
		{
			useMultiplierForHighRiskAcqRates = false;
		}

		double highRiskAcqRateMultiplier;

		try
		{
			highRiskAcqRateMultiplier = behaviorElem->FirstChildElement("HighRiskAcqRateMultiplier")->GetText<double>();
		}
		catch(ticpp::Exception &)
		{
			highRiskAcqRateMultiplier = 1;
		}

		bool useMultiplierForHighRiskCSW = false;

		try
		{
			useMultiplierForHighRiskCSW = (behaviorElem->FirstChildElement("UseCSWHighRiskMultiplier")->GetText<int>() == 1);
		}
		catch(ticpp::Exception &)
		{
			useMultiplierForHighRiskCSW = false;
		}

		double highRiskAcqRateMultiplierCSW;

		try
		{
			highRiskAcqRateMultiplierCSW = behaviorElem->FirstChildElement("CSWHighRiskAcqRateMultiplier")->GetText<double>();
		}
		catch(ticpp::Exception &)
		{
			highRiskAcqRateMultiplierCSW = 1;
		}

		//Coefficient of Variation
		_eventParams.useCoefficientVariation =
		    behaviorElem->FirstChildElement("heterogeneity")->FirstChildElement("varMethod")->GetText<int>() == 0;
		_eventParams.coefficientOfVariation =
		    behaviorElem->FirstChildElement("heterogeneity")->FirstChildElement("coeffVar")->GetText<double>();
		//iterate through each Person in partnershipTypes
		ticpp::Iterator<ticpp::Element> partnershipTypesIter;
		string partnershipType;
		sexualBehaviorParams.clear();

		for(partnershipTypesIter = behaviorElem->FirstChildElement("partnershipTypes")->FirstChildElement("partnership");
		        partnershipTypesIter != partnershipTypesIter.end(); partnershipTypesIter++)
		{
			SexualBehaviorParams *currPartnershipParams = new SexualBehaviorParams(partnershipTypesIter->ToElement(), _eventParams,
			        useMultiplierForHighRiskAcqRates, highRiskAcqRateMultiplier, useMultiplierForHighRiskCSW, highRiskAcqRateMultiplierCSW);
			sexualBehaviorParams.push_back(currPartnershipParams);
		} //end for partnershipTypesIter

		proportionHighRisk[DmgProfile::CSW] = behaviorElem->FirstChildElement("proportionHighRiskCSW")->GetText<double>();
		proportionHighRisk[DmgProfile::NON_CSW] =
		    behaviorElem->FirstChildElement("proportionHighRiskNonCSW")->GetText<double>();
		XMLUtil::getDistFromXMLNode(behaviorElem->FirstChildElement("activityLevel"), activityLevel);
		//saves partner acq rate and acts discounting
		ticpp::Element *discounting = behaviorElem->FirstChildElement("ageDiscounting");
		partneringDiscStartAgeYrs = discounting->FirstChildElement("startAgeYrs")->GetText<int>();
		partneringAcqDiscPerYr = discounting->FirstChildElement("acquisitionDiscByYr")->GetText<double>();
		partneringActsDiscPerYr = discounting->FirstChildElement("coitalActsDiscByYr")->GetText<double>();
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

		//get the health stats
		ticpp::Element *healthElem = _maleParams->FirstChildElement("health");
		//get protective efficacy of circumcision
		circumProtectEff = healthElem->FirstChildElement("circumcisionProtectEfficacy")->GetText<double>();
		//get protective efficacy of condoms
		condomProtectEff  = healthElem->FirstChildElement("condomProtectEfficacy")->GetText<double>();
		//get the transmission coefficients
		transmitPerEventCoeffs.clear();
		ticpp::Element *transmitCoeffElem = healthElem->FirstChildElement("transmissionCoefficients");
		XMLUtil::getTabDelimitedNode(transmitCoeffElem->FirstChildElement("valsByHVL"),
		                             transmitPerEventCoeffs);
		transmitPerEventCoeffs.push_back(transmitCoeffElem->FirstChildElement("primary")->GetText<double>());
		transmitPerEventCoeffs.push_back(transmitCoeffElem->FirstChildElement("lateStage")->GetText<double>());
	}
	catch(ticpp::Exception &_e)
	{
		cout << "Male: Exception raised: " << _e.m_details << endl;
		Util::exitWithPrompt(-1);
	}

	return 0;
} //end loadParamsXML(...)
int Male::SubPopParams::reloadParamsXML(ticpp::Element *_maleParams, EventParams &_eventParams) throw()
{
	return loadParamsXML(_maleParams, _eventParams);
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
	return sexualBehaviorParams.at(_type);
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
	sexualBehaviorParams[partnershipType]->setChanceCondomUsePerEvent(risk, dist);
}

//------------ < End getters >-----------------//

//-----------------< End population-level parameters for males >-----------------------/


vector<Male::SubPopParams *> Male::populationSpecificParams;

Male::SubPopParams *Male::getPopParams(unsigned int _populationID)
{
	assert(_populationID < Male::populationSpecificParams.size());
	return Male::populationSpecificParams.at(_populationID);
}

void Male::addPopParams(unsigned int _populationID, ticpp::Element *_maleParams, EventParams &_eventParams)
{
	assert(_populationID == Male::populationSpecificParams.size());
	Male::SubPopParams *subPopParams = new Male::SubPopParams(_maleParams, _eventParams);
	Male::populationSpecificParams.push_back(subPopParams);
}

void Male::updatePopParams(unsigned int _populationID, ticpp::Element *_maleParams, EventParams &_eventParams)
{
	assert(_populationID == Male::populationSpecificParams.size() - 1);
	Male::populationSpecificParams.at(_populationID)->reloadParamsXML(_maleParams, _eventParams);
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
	//determining partnering and sexual behavior for this male.
	EnumCls<SexualPartnership::Type>::Enum partneringType = SexualPartnership::TypeEnum.getMin();
	EnumCls<SexualPartnership::Type>::Enum lastPartneringType = SexualPartnership::TypeEnum.getMax();

	while(partneringType <= lastPartneringType)
	{
		const SexualBehaviorParams *sexualBehaviorParams = maleSubPopParams->getSexualBehaviorParams(partneringType);
		partnerAcqRates[partneringType] = _eventParams.randomNums.randLogNormal(
		        sexualBehaviorParams->getAcquisitionRatePerMonth(risk));
		numActsPerMonth[partneringType] = sexualBehaviorParams->getCoitalEventsPerMonth(risk);
		chanceCondomUsePerEvent[partneringType] = _eventParams.randomNums.randBeta(
		            sexualBehaviorParams->getChanceCondomUsePerEvent(risk));
		averageYearsYounger[partneringType] = sexualBehaviorParams->getAverageYearsYounger();
		partneringType = EnumCls<SexualPartnership::Type>::Enum(partneringType + 1);
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
	assert(_partnershipType < SexualPartnership::ENDType);
	return chanceCondomUsePerEvent[_partnershipType];
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
	assert(_partnershipType < SexualPartnership::ENDType);
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
	case Person::AGE :
		if(getAge(MONTH) - (12 * averageYearsYounger[_partnershipType].mean + 6) > 0)
		{
			return getAge(MONTH) - (12 * averageYearsYounger[_partnershipType].mean + 6);
			break;
		}
		else
		{
			return 0;
			break;
		}

	//case SEXUAL_ACTIVITY_LEVEL :
	case ID :
		return numeric_limits<double>::min();

	default :
		cerr << "Invalid Sorting key :" << _PSC;
		Util::exitWithPrompt(-1);
	}

	return -1;
}


double Male::getMaxPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const
{
	switch(_PSC)
	{
	case Person::AGE :
		if(getAge(MONTH) - (12 * averageYearsYounger[_partnershipType].mean - 6) > 0)
		{
			return getAge(MONTH) - (12 * averageYearsYounger[_partnershipType].mean - 6);
			break;
		}
		else
		{
			return 0;
			break;
		}

	//case SEXUAL_ACTIVITY_LEVEL :
	case ID:
		return numeric_limits<double>::max();

	default :
		cerr << "Invalid Sorting key :" << _PSC;
		Util::exitWithPrompt(-1);
	}

	return -1;
}

double Male::rollForAgeDifference(SexualPartnership::Type _partnershipType, RandomNums &_randomNums)
{
	double ageDifference = _randomNums.randNorm(averageYearsYounger[_partnershipType]);
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
	assert(_partnershipType < SexualPartnership::ENDType);
	assert(false);  // check if we are using years instead of Month
	int minAge = static_cast<int>(getMinPartnerSelectVal(Person::AGE, _partnershipType));
	int maxAge = static_cast<int>(getMaxPartnerSelectVal(Person::AGE, _partnershipType));
	return Util::withinRange(_p->getAge(MONTH), minAge, maxAge);
}

int Male::rollForNumPartners(RandomNums &_randomNums, SexualPartnership::Type _partnershipType)
{
	assert(_partnershipType < SexualPartnership::ENDType);

	//person can only have 1 steady partner at a time so return 0 if person is already in Steady
	if((_partnershipType == SexualPartnership::STEADY) && (!partners[_partnershipType].empty()))
	{
		return 0;
	}

	//Male parameters for the population that this Male is in
	const Male::SubPopParams *subPopParams = getPopParams(populationID);
	//rate of acquiring partner
	double partnerRate;
	partnerRate = partnerAcqRates[_partnershipType];

	//if this person has a steady partner then adjust acquisition rate
	if(!partners[SexualPartnership::STEADY].empty())
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
	return (_partnershipType != SexualPartnership::STEADY) ? numPartners : min(1, numPartners);
}

int Male::rollNumEventsPerPartner(Person *_p, RandomNums &_randomNums, SexualPartnership::Type _partnershipType)
{
	assert((_p != nullptr));
	assert(_p->isAlive());
	assert(_partnershipType < SexualPartnership::ENDType);
	double meanCoitalEvents = numActsPerMonth[_partnershipType];
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
	assert(_partnershipType < SexualPartnership::ENDType);
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
		//determining partnering and sexual behavior for this male.
		EnumCls<SexualPartnership::Type>::Enum partneringType = SexualPartnership::TypeEnum.getMin();
		EnumCls<SexualPartnership::Type>::Enum lastPartneringType = SexualPartnership::TypeEnum.getMax();

		while(partneringType <= lastPartneringType)
		{
			const SexualBehaviorParams *sexualBehaviorParams = maleSubPopParams->getSexualBehaviorParams(partneringType);
			partnerAcqRates[partneringType] = _eventParams.randomNums.randLogNormal(
			        sexualBehaviorParams->getAcquisitionRatePerMonth(risk));
			numActsPerMonth[partneringType] = sexualBehaviorParams->getCoitalEventsPerMonth(risk);
			chanceCondomUsePerEvent[partneringType] = _eventParams.randomNums.randBeta(
			            sexualBehaviorParams->getChanceCondomUsePerEvent(risk));
			partneringType = EnumCls<SexualPartnership::Type>::Enum(partneringType + 1);
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

	for(int i = 0; i < SexualPartnership::ENDType; i++)
	{
		if(!firstInSequence)
		{
			_outStream << ",";
		}

		firstInSequence = false;
		_outStream << partnerAcqRates[i];
	}

	_outStream << "]," << endl;
	//Acts per month
	firstInSequence = true;
	_outStream << "actsPerMth:[";

	for(int i = 0; i < SexualPartnership::ENDType; i++)
	{
		if(!firstInSequence)
		{
			_outStream << ",";
		}

		firstInSequence = false;
		_outStream << numActsPerMonth[i];
	}

	_outStream << "]," << endl;
	//Acts per month
	firstInSequence = true;
	_outStream << "probCndm:[";

	for(int i = 0; i < SexualPartnership::ENDType; i++)
	{
		if(!firstInSequence)
		{
			_outStream << ",";
		}

		firstInSequence = false;
		_outStream << chanceCondomUsePerEvent[i];
	}

	_outStream << "]";
}

//-----------------< End methods which are inherited from Person >-----------------//
