#include "SexualBehaviorParams.h"
#include "../../util/XMLUtil.h"
#include "../Person.h"
#include "boost/lexical_cast.hpp"

SexualBehaviorParams::SexualBehaviorParams(ticpp::Element *_sexualBehaviourParams, EventParams &_eventParams,
        bool _useHighRiskMultiplier, double _highRiskMultiplier, bool _useHighRiskMultiplierCSW, double _highRiskMultiplierCSW)
{
	this->loadParamsXML(_sexualBehaviourParams, _eventParams, _useHighRiskMultiplier, _highRiskMultiplier,
	                    _useHighRiskMultiplierCSW, _highRiskMultiplierCSW);
}

SexualBehaviorParams::SexualBehaviorParams()
{
}

int SexualBehaviorParams::loadParamsXML(ticpp::Element *_sexualBehaviourParams, EventParams &_eventParams,
                                        bool _useHighRiskMultiplier, double _highRiskMultiplier, bool _useHighRiskMultiplierCSW, double _highRiskMultiplierCSW)
{
	_eventParams.displayOut("SexualBehaviorParams::loadParamsXML(...)\n");
	/*if (_useHighRiskMultiplier){
		cout << "Using High Risk multiplier of " << _highRiskMultiplier << endl;
	}*/
	//read in behavior twig
	SexualPartnership::TypeEnum.toEnum(_sexualBehaviourParams->FirstChildElement("type")->GetText());
	double multiplier = _highRiskMultiplier;

	if(this->partnershipType == SexualPartnership::CSW && _useHighRiskMultiplier && _useHighRiskMultiplierCSW)
	{
		multiplier = _highRiskMultiplierCSW;
	}

	for(auto risk = Person::LOW; risk < Person::ENDRiskLevel; ++risk)
	{
		//save acquisition rate
		//If high risk, check if using multiplier or reading rates in directly
		if(risk == Person::LOW || !_useHighRiskMultiplier)
		{
			//The user is no inter inputting rates in a logNormal distribution and we are converting from mean and std dev
			XMLUtil::getLogNormalDistFromXMLNode(_sexualBehaviourParams->FirstChildElement((risk == Person::LOW) ?
			                                     "acquisitionRateLowRisk" : "acquisitionRateHighRisk"), this->acquisitionRatePerMonth[risk],
			                                     _eventParams.useCoefficientVariation, _eventParams.coefficientOfVariation);
		}
		else
		{
			//If the high risk is to be a multiple of the low risk rates
			XMLUtil::getLogNormalDistFromXMLNode(_sexualBehaviourParams->FirstChildElement("acquisitionRateLowRisk"),
			                                     this->acquisitionRatePerMonth[risk], _eventParams.useCoefficientVariation, _eventParams.coefficientOfVariation);
			this->acquisitionRatePerMonth[risk].mu = this->acquisitionRatePerMonth[risk].mu + log(multiplier);
		}

		//this->acquisitionRatePerMonth[risk] = XMLUtil::getExpDistMeanFromXMLNode(_sexualBehaviourParams->FirstChildElement((risk == Person::LOW) ? "acquisitionRatePerMonthLowRisk" : "acquisitionRatePerMonthHighRisk"));
		_eventParams.displayOut("Average acquisition rate for relationship type ");
		_eventParams.displayOut((*(SexualPartnership::TypeEnum.toString(this->partnershipType))).c_str());
		_eventParams.displayOut(" and risk level ");
		((risk == Person::LOW) ? _eventParams.displayOut("LOW") : _eventParams.displayOut("HIGH"));
		_eventParams.displayOut(" is ");
		_eventParams.displayOut(boost::lexical_cast<std::string>(this->acquisitionRatePerMonth[risk].getMean()).c_str());
		_eventParams.displayOut("\n");
		//save number of partners to acquire per month
		//this->averagePartnersAtATime[risk] = XMLUtil::getPoissonDistMeanFromXMLNode(_sexualBehaviourParams->FirstChildElement((risk == Person::LOW) ? "averageNumberToAcquireLowRisk" : "averageNumberToAcquireHighRisk"));
		//*(_eventParams.displaybox->textctrl) << wxT("Acquisition rate for relationship type ") << wxPartnershipType << wxT(" is ") << this->averagePartnersAtATime << wxT("\n");
		//_eventParams.displaybox->textctrl->Update();
	}//for (int risk = Person::LOW; risk < Person::ENDRiskLevel; risk++){

	//go through selection criteria
	ticpp::Element *selectionCriteria = _sexualBehaviourParams->FirstChildElement("selectionCriteria");
	//get the available EntityPool::Buckets that Males can choose from
	ticpp::Iterator<ticpp::Element> bucketsIter;

	for(bucketsIter = selectionCriteria->FirstChildElement("availableBuckets")->FirstChildElement("bucket");
	        bucketsIter != bucketsIter.end(); bucketsIter++)
	{
		//not very efficient, but we only do this once at the beginning of the sim
		availableBuckets.resize(availableBuckets.size() + 1);
		availableBuckets.back().dmgProfileSelector.parse(bucketsIter->FirstChildElement("DmgProfile")->GetText());
		bucketsIter->FirstChildElement("weightedValue")->GetText<double>(&availableBuckets.back().weight);
	} //for ( bucketsIter = selectionCriteria->FirstChildElement("availableBuckets")->FirstChildElement("bucket");

	//get the average age difference
	XMLUtil::getDistFromXMLNode(selectionCriteria->FirstChildElement("AverageYearsYounger"), this->averageYearsYounger);

	//read in # coital events and partnership duration by risk level
	for(int risk = Person::LOW; risk < Person::ENDRiskLevel; risk++)
	{
		//TODO: Break out
		this->coitalEventsPerMonth[risk] = XMLUtil::getPoissonDistMeanFromXMLNode(_sexualBehaviourParams->FirstChildElement((
		                                       risk == Person::LOW) ? "coitalEventsPerMonthLowRisk" : "coitalEventsPerMonthHighRisk"));
		//save chance of condom use per event
		XMLUtil::getBetaDistFromXMLNode(_sexualBehaviourParams->FirstChildElement((risk == Person::LOW) ?
		                                "chanceCondomUsePerEventLowRisk" : "chanceCondomUsePerEventHighRisk"), this->chanceCondomUsePerEvent[risk]);
		XMLUtil::getShiftedLogNormalDistFromXMLNode(_sexualBehaviourParams->FirstChildElement((
		            risk == Person::LOW) ? "partnershipDurationMthLowRisk" : "partnershipDurationMthHighRisk"),
		        this->partnershipDurationMth[risk]);
	}//for (risk = Person::LOW; risk < Person::ENDRiskLevel; risk++){

	return 0;
}


std::size_t SexualBehaviorParams::getNumAvailableBuckets()  const
{
	return availableBuckets.size();
}


SexualPartnership::Type SexualBehaviorParams::getPartnershipType() const
{
	return this->partnershipType;
}

const LogNormalDist SexualBehaviorParams::getAcquisitionRatePerMonth(Person::RiskLevel risk) const
{
	return this->acquisitionRatePerMonth[risk];
}

const SexualBehaviorParams::AvailableBucket SexualBehaviorParams::getAvailableBucket(int _bucket) const
{
	return this->availableBuckets.at(_bucket);
}

const NormalDist SexualBehaviorParams::getAverageYearsYounger() const
{
	return this->averageYearsYounger;
}

const double SexualBehaviorParams::getCoitalEventsPerMonth(Person::RiskLevel risk) const
{
	return this->coitalEventsPerMonth[risk];
}

const BetaDist SexualBehaviorParams::getChanceCondomUsePerEvent(Person::RiskLevel risk) const
{
	return this->chanceCondomUsePerEvent[risk];
}

const ShiftedLogNormalDist SexualBehaviorParams::getPartnershipDurationMth(Person::RiskLevel risk) const
{
	return this->partnershipDurationMth[risk];
}
