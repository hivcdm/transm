#pragma once

#include "../../Inputs.h"
#include "./../classifiers/DmgProfile.h"
#include "./../classifiers/SexualPartnership.h"
#include "../../util/ticpp/ticpp.h"
#include "../Person.h"
#include <vector>

/**
This can parse XML that contains info about sexual behavior for a particular
type of partnership. As of 9/8/08, only males have detailed sexual
behavior. There should be 1 XML-subtree for every SexualPartnership::Type
for each type of Person in the simulation

@author schung5
**/
class SexualBehaviorParams
{

public :
	/**
	calls loadParamsXML
	@param _sexualMixingParams XML-subtree root node
	@author schung5
	**/
	SexualBehaviorParams(const PopulationSettings::MaleSettings &settings, SexualPartnership::Type type, EventParams &params);

	/**
	default constructor
	@author schung5
	**/
	SexualBehaviorParams();

	//different SexualPartnership can only involve certain categories of people
	struct AvailableBucket
	{
		//specifies an eligible type of person
		DmgProfile dmgProfileSelector;
		//will weight the chance that someone with these DmgProfileBucket dmgProfile will be chosen
		double weight;
	};

private:

	//the partnership type that these parameters represent
	SexualPartnership::Type partnershipType;

	//LogNormal distribution from which the people draw a rate to acquire this type of partner
	LogNormalDist acquisitionRatePerMonth[Person::ENDRiskLevel];
	//average number of partners men acquire at a time
	//double averagePartnersAtATime[Person::ENDRiskLevel];

	//selection criteria
	//particular buckets that are available for this kind of sexual partnership
	std::vector<AvailableBucket> availableBuckets;
	/*//defines available ages for this kind of sexual partnership
	NormalDist maxYearsOlder;
	NormalDist maxYearsYounger;*/
	//The distribution the males will draw from to determine how many years younger their partner should be (resulting difference may be negative for older women)
	NormalDist averageYearsYounger;

	//avg events per month across all Couples; will be used as a mean in Poisson distribution
	double coitalEventsPerMonth[Person::ENDRiskLevel];

	//chance per event that this person will use a condom
	BetaDist chanceCondomUsePerEvent[Person::ENDRiskLevel];

	//avg duration if partnerships across all Couples
	ShiftedLogNormalDist partnershipDurationMth[Person::ENDRiskLevel];

public :

	/**
	returns number of Buckets that are available for this kind of partnership
	 this is used to allow class Male to set aside some temporary space
	@author schung5
	**/
	unsigned int getNumAvailableBuckets() const;

	//-----< Begin getters and setters of fields >-----------//
	const LogNormalDist getAcquisitionRatePerMonth(Person::RiskLevel risk) const;
	const AvailableBucket getAvailableBucket(int _bucket) const;
	const double getCoitalEventsPerMonth(Person::RiskLevel risk) const;
	const BetaDist getChanceCondomUsePerEvent(Person::RiskLevel risk) const;
	const NormalDist getAverageYearsYounger() const;
	const ShiftedLogNormalDist getPartnershipDurationMth(Person::RiskLevel risk) const;
	SexualPartnership::Type getPartnershipType() const;
	void setChanceCondomUsePerEvent(Person::RiskLevel risk, BetaDist dist) { chanceCondomUsePerEvent[risk] = dist; }
	//-----< End getters and setters of fields >-----------//
};
