#include "Female.h"
#include "Male.h"
#include "../Constants.h"
#include "../util/Util.h"
#include "../util/XMLUtil.h"


//-----------------< Begin population-level parameters for females >-----------------------/
//each index of the array contains parameters for a different population
//(as of 9/8/08, we only have 1 population for now so the size of the vector will default to 1
Female::SubPopParams::SubPopParams()
{
}

Female::SubPopParams::SubPopParams(const PopulationSettings::FemaleSettings &settings, EventParams &_eventParams)
{
	try
	{
		chanceBecomeCSW = settings.chance_become_sex_worker;
		proportionHighRisk[DmgProfile::CSW] = settings.proportion_high_risk_csw;
		proportionHighRisk[DmgProfile::NON_CSW] = settings.proportion_high_risk_non_csw;
		activityLevel = settings.activity_level;
		transmitPerEventCoeffs.assign(settings.transmission_coefficients.begin(), settings.transmission_coefficients.end());
	}
	catch(ticpp::Exception &_e)
	{
		throw std::runtime_error("Female: Exception raised: " + _e.m_details);
	}
}

double Female::SubPopParams::getChanceBecomeCSW() const
{
	return chanceBecomeCSW;
}

double Female::SubPopParams::getProportionHighRisk(DmgProfile::Employment _cswStatus) const
{
	return proportionHighRisk[_cswStatus];
}
NormalDist Female::SubPopParams::getActivityLevel() const
{
	return activityLevel;
}

double Female::SubPopParams::getTransmitPerEventCoeff(HVLStrata _hvl) const
{
	return transmitPerEventCoeffs.at(_hvl);
}

//-----------------< End population-level parameters for females >-----------------------/

vector<Female::SubPopParams *> Female::populationSpecificParams;

Female::SubPopParams *Female::getPopParams(unsigned int _populationID)
{
	assert(_populationID < Female::populationSpecificParams.size());
	return Female::populationSpecificParams.at(_populationID);
}
void Female::addPopParams(unsigned int _populationID, const PopulationSettings::FemaleSettings &settings, EventParams &_eventParams)
{
	assert(_populationID == Female::populationSpecificParams.size());
	Female::SubPopParams *subPopParams = new Female::SubPopParams(settings, _eventParams);
	Female::populationSpecificParams.push_back(subPopParams);
}

Female::Female(EventParams &_eventParams, int _ageMths, unsigned int _populationID)
	: Person(_eventParams, _ageMths, _populationID)
{
	dmgProfile.set(DmgProfile::GENDER, DmgProfile::FEMALE);
	const Female::SubPopParams *femaleSubPopParams = getPopParams(populationID);
	activityLevel = _eventParams.randomNums.randNorm_NaturalNum(femaleSubPopParams->getActivityLevel());

	//activity level should not ever be 0
	if(activityLevel == 0)
	{
		activityLevel = 1;
	}

	risk = Person::LOW;
}

Female::~Female(void)
{
}


//-----------< START methods that will populate static fields or store to file >----------------//


//-----------< END methods that will populate static fields or store to file >----------------//

/** These methods are inherited from Person **/



double Female::getFOI(Person *_p, SexualPartnership::Type _partnershipType, EventParams &_eventParams)
{
	assert(_p->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE);
	Male *m = (Male *)_p;
	//transmission coeff				  (1 - (condoms are used and succeed)) * (1 - (male is circumcised))
	double circEff = m->getCircumProtectEff();
	//Determine if a condom was used and record
	condomUsedLastFOICalculation = _eventParams.randomNums.chance(m->getCondomUseProb(this, _partnershipType));
	//Determine the condom efficacy --> 0 if no condom was used
	double condomEff = 0;

	if(condomUsedLastFOICalculation)
	{
		condomEff = m->getCondomProtectEff();
	}

	double FOI = getTransmissionCoeff() * (1 - condomEff) * (1 - circEff);

	if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && (trace() || _p->trace()))
	{
		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " !Transmission coefficient from " << getID() << " to " <<
		        _p->getID() << " is " << getTransmissionCoeff();
		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << ";" << endl << " !A condom was ";

		if(!condomUsedLastFOICalculation)
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "NOT ";
		}

		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "used (efficacy " << m->getCondomProtectEff();

		if(m->isCircumcised())
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << ");" << endl << " !" << m->getID() <<
			        " is circumcised (efficacy " << circEff;
		}

		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << ");" << endl << " !Total FOI = " << FOI << endl;
	}

	return FOI;
}

//currently, females don't have much of a choice. Edit these functions to give them ability have have partner preferences
double Female::getMinPartnerSelectVal(Person::SelectingCriteria /*_PSC*/,
                                      SexualPartnership::Type /*_partnershipType*/) const
{
	return numeric_limits<unsigned int>::min();
}

double Female::getMaxPartnerSelectVal(Person::SelectingCriteria /*_PSC*/,
                                      SexualPartnership::Type /*_partnershipType*/) const
{
	return numeric_limits<unsigned int>::max();
}

double Female::getTransmissionCoeff()
{
	assert(Util::withinRange(hvl, HVL_ZERO, HVL_LATESTAGE));
	return Female::populationSpecificParams.at(populationID)->getTransmitPerEventCoeff(hvl);
}

void Female::rerollRiskGroup(EventParams &_eventParams)
{
	DmgProfile::Employment cswStatus = (DmgProfile::Employment) getDmgProfileVal(DmgProfile::EMPLOYMENT);
	double chanceHighRisk = getPopParams(populationID)->getProportionHighRisk(cswStatus);

	if(_eventParams.randomNums.chance(chanceHighRisk))
	{
		risk = HIGH;
	}
	else
	{
		risk = LOW;
	}

	if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && trace())
	{
		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " % Female " << getID() << " rerolls as ";

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

void Female::saveState(ostream &_outStream, long currTime)
{
	_outStream << "gend:f," << endl;
	Person::saveState(_outStream, currTime);
}