#pragma once
#include "Person.h"
#include "../Inputs.h"
#include "../data/EventParams.h"
#include "../util/ticpp/ticpp.h"
#include "../util/rand/RandomNums.h"

/***
All females in the simulation are members of this class, or a class derived from this one
***/
class Female : public Person
{
public:

	//----------------< BEGIN class SubPopParams >--------------------------//
	/**
	These are parameters that describe the population of females.
	Each Population in the Sim will have a separate one of these referenced by the population's ID.

	@author schung5
	**/
	class SubPopParams
	{
		//-----------< BEGIN data fields >--------------------//
		double chanceBecomeCSW;		//chance that a female will become a CSW
		double proportionHighRisk[DmgProfile::ENDEmployment];  //proportion of female population that is in the "high risk" lists
		NormalDist activityLevel; //Distribution of activity level (i.e. marbles)
		vector<double> transmitPerEventCoeffs;	 //chance of infection for women->men, w/o circumcision or condoms
		//-----------< END data fields >--------------------//

	public :
		SubPopParams();
		SubPopParams(const PopulationSettings::FemaleSettings &settings, EventParams &_eventParams);

		//-----------< BEGIN getters >--------------------//
		double getChanceBecomeCSW() const;
		double getProportionHighRisk(DmgProfile::Employment) const;
		NormalDist getActivityLevel() const;
		double getTransmitPerEventCoeff(HVLStrata _hvl) const;
		//-----------< END getters >--------------------//
	};
	//----------------< END class SubPopParams >--------------------------//

private:

	//this vector holds Parameters for females different populations it is used to populate fields
	//	for each instance of Female w/ different values depending on which Population the Female is part of
	//	Rationale: So females don't really have to know much about the population they are in except for the ID
	static vector<SubPopParams *> populationSpecificParams;

public:
	/**
	Add a set of population parameters to be used by Males of that population
	@author schung5
	**/
	static void addPopParams(unsigned int _populationID, const PopulationSettings::FemaleSettings &settings, EventParams &_eventParams);

	/**
	Access a set of population parameters to be used by Males of that population
	@author schung5
	**/
	static SubPopParams *getPopParams(unsigned int _populationID);


	/**
	//this constructor creates Females that can be simulated
	//the parameters match the ones in Person(...)
	@author schung5
	**/
	Female(EventParams &_eventParams, int _ageMths,  unsigned int _populationID);
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

	/** end: Inherited from Person **/
};
