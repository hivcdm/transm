#pragma once

#include "Person.h"
#include "../data/EventParams.h"
#include "../util/rand/RandomNums.h"
#include "../util/Serializable.h"

/***
All females in the simulation are members of this class, or a class derived from this one
***/
class Female : public Person
{
public:
	/**
	These are parameters that describe the population of females.
	Each Population in the Sim will have a separate one of these referenced by the population's ID.

	@author schung5
	**/
	class SubPopParams : public Serializable
	{
	public :
		void Deserialize(const pugi::xml_node &node);

		void Serialize(pugi::xml_node &parent_node);

		SubPopParams();

		double getChanceBecomeCSW() const;
		double getProportionHighRisk(DmgProfile::Employment) const;
		NormalDist getActivityLevel() const;
		double getTransmitPerEventCoeff(HVLStrata _hvl) const;

	private:
		double chanceBecomeCSW;		//chance that a female will become a CSW
		double proportionHighRisk[DmgProfile::ENDEmployment];  //proportion of female population that is in the "high risk" lists
		NormalDist activityLevel; //Distribution of activity level (i.e. marbles)
		std::vector<double> transmitPerEventCoeffs;	 //chance of infection for women->men, w/o circumcision or condoms
	};

public:
	/**
	//this constructor creates Females that can be simulated
	//the parameters match the ones in Person(...)
	@author schung5
	**/
	Female(EventParams &_eventParams, int _ageMths, unsigned int _populationID, const Female::SubPopParams &params);
	~Female(void);

	/** Start: Inherited from Person, comments found there **/

	/**
	As of 9/8/08, this only assumes heterosexual relationships.
	@return the force of infection for this female infecting an uninfected male
	@author schung5
	**/
	double getFOI(Person *_p, SexualPartnership::Type _partnershipType, EventParams &_eventParams);

	double getMinPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const;
	double getMaxPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const;

	double getTransmissionCoeff();
	void rerollRiskGroup(EventParams &_eventParams);
	//writes state of person to file
	void saveState(ostream &_outStream, long currTime);

	double getChanceBecomeCsw() const;

	void SetChanceCondomUsePerEvent(Person::RiskLevel risk, SexualPartnership::Type partnershipType, BetaDist dist);

private:
	SubPopParams populationSpecificParams;
};
