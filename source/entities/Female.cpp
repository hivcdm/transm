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

Female::SubPopParams::SubPopParams(ticpp::Element *_femaleParams, EventParams &_eventParams)
{
	this->loadParamsXML(_femaleParams, _eventParams);
}


int Female::SubPopParams::loadParamsXML(ticpp::Element *_femaleParams, EventParams &/*_eventParams*/)
{
	try
	{
		//get behavioral params
		ticpp::Element *behaviorElem = _femaleParams->FirstChildElement("behavior");
		this->chanceBecomeCSW = behaviorElem->FirstChildElement("chanceBecomeSexWorker")->GetText<double>();
		this->proportionHighRisk[DmgProfile::CSW] = behaviorElem->FirstChildElement("proportionHighRiskCSW")->GetText<double>();
		this->proportionHighRisk[DmgProfile::NON_CSW] =
		    behaviorElem->FirstChildElement("proportionHighRiskNonCSW")->GetText<double>();
		XMLUtil::getDistFromXMLNode(behaviorElem->FirstChildElement("activityLevel"), this->activityLevel);
		//get the health params
		ticpp::Element *healthElem = _femaleParams->FirstChildElement("health");
		ticpp::Element *transmitCoeffElem = healthElem->FirstChildElement("transmissionCoefficients");
		//get the transmission coefficients
		this->transmitPerEventCoeffs.clear();
		XMLUtil::getTabDelimitedNode(transmitCoeffElem->FirstChildElement("valsByHVL"),
		                             this->transmitPerEventCoeffs);
		this->transmitPerEventCoeffs.push_back(transmitCoeffElem->FirstChildElement("primary")->GetText<double>());
		this->transmitPerEventCoeffs.push_back(transmitCoeffElem->FirstChildElement("lateStage")->GetText<double>());
	}
	catch(ticpp::Exception &_e)
	{
		cout << "Female: Exception raised: " << _e.m_details << endl;
		Util::exitWithPrompt(-1);
	}

	return 0;
}
int Female::SubPopParams::reloadParamsXML(ticpp::Element *_femaleParams, EventParams &_eventParams) throw()
{
	return loadParamsXML(_femaleParams, _eventParams);
}

double Female::SubPopParams::getChanceBecomeCSW() const
{
	return this->chanceBecomeCSW;
}

double Female::SubPopParams::getProportionHighRisk(DmgProfile::Employment _cswStatus) const
{
	return this->proportionHighRisk[_cswStatus];
}
NormalDist Female::SubPopParams::getActivityLevel() const
{
	return this->activityLevel;
}

double Female::SubPopParams::getTransmitPerEventCoeff(HVLStrata _hvl) const
{
	return this->transmitPerEventCoeffs.at(_hvl);
}

//-----------------< End population-level parameters for females >-----------------------/

vector<Female::SubPopParams *> Female::populationSpecificParams;

Female::SubPopParams *Female::getPopParams(unsigned int _populationID)
{
	assert(_populationID < Female::populationSpecificParams.size());
	return Female::populationSpecificParams.at(_populationID);
}
void Female::addPopParams(unsigned int _populationID, ticpp::Element *_femaleParams, EventParams &_eventParams)
{
	assert(_populationID == Female::populationSpecificParams.size());
	Female::SubPopParams *subPopParams = new Female::SubPopParams(_femaleParams, _eventParams);
	Female::populationSpecificParams.push_back(subPopParams);
}
void Female::updatePopParams(unsigned int _populationID, ticpp::Element *_femaleParams, EventParams &_eventParams)
{
	assert(_populationID == Female::populationSpecificParams.size() - 1);
	Female::populationSpecificParams.at(_populationID)->reloadParamsXML(_femaleParams, _eventParams);
}


Female::Female(EventParams &_eventParams, int _ageMths, unsigned int _populationID)
	: Person(_eventParams, _ageMths, _populationID)
{
	this->dmgProfile.set(DmgProfile::GENDER, DmgProfile::FEMALE);
	const Female::SubPopParams *femaleSubPopParams = this->getPopParams(this->populationID);
	this->activityLevel = _eventParams.randomNums.randNorm_NaturalNum(femaleSubPopParams->getActivityLevel());

	//activity level should not ever be 0
	if(this->activityLevel == 0)
	{
		this->activityLevel = 1;
	}

	this->risk = Person::LOW;
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
	this->condomUsedLastFOICalculation = _eventParams.randomNums.chance(m->getCondomUseProb(this, _partnershipType));
	//Determine the condom efficacy --> 0 if no condom was used
	double condomEff = 0;

	if(this->condomUsedLastFOICalculation)
	{
		condomEff = m->getCondomProtectEff();
	}

	double FOI = this->getTransmissionCoeff() * (1 - condomEff) * (1 - circEff);

	if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && (this->trace() || _p->trace()))
	{
		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " !Transmission coefficient from " << this->getID() << " to " <<
		        _p->getID() << " is " << this->getTransmissionCoeff();
		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << ";" << endl << " !A condom was ";

		if(!this->condomUsedLastFOICalculation)
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
	assert(Util::withinRange(this->hvl, HVL_ZERO, HVL_LATESTAGE));
	return Female::populationSpecificParams.at(this->populationID)->getTransmitPerEventCoeff(this->hvl);
}

void Female::rerollRiskGroup(EventParams &_eventParams)
{
	DmgProfile::Employment cswStatus = (DmgProfile::Employment) this->getDmgProfileVal(DmgProfile::EMPLOYMENT);
	double chanceHighRisk = getPopParams(this->populationID)->getProportionHighRisk(cswStatus);

	if(_eventParams.randomNums.chance(chanceHighRisk))
	{
		this->risk = HIGH;
	}
	else
	{
		this->risk = LOW;
	}

	if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && this->trace())
	{
		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " % Female " << this->getID() << " rerolls as ";

		if(this->risk == HIGH)
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