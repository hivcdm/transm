#pragma once

#include <map>

#include "Person.h"
#include "../data/EventParams.h"
#include "../util/rand/RandomNums.h"

#include <ticpp/ticpp.h>

class SexualBehaviorParams;
/**
All females in the simulation are members of this class, or a class derived from this one
**/
class Male : public Person
{

public :

	//----------------< BEGIN class SubPopParams >--------------------------//
	/**
	These are parameters that describe the population of males.
	Each Population in the Sim will have a separate one of these references by the population's ID.
	**/
	class SubPopParams
	{
		int loadParamsXML(ticpp::Element *_maleParams, EventParams &_eventParams) throw();

		//-----------< BEGIN data fields >--------------------//
		double chanceBecomeCSW;		//chance that a male will become a CSW
		double partnerAcqMultWithSteady[Person::ENDRiskLevel];  //the rate multiplier for partner acquisition when a male has a Steady partner

		//sexual behavior params for each type as specified by SexualPartnership::Type
		vector<SexualBehaviorParams *> sexualBehaviorParams;
		double proportionHighRisk[DmgProfile::ENDEmployment];  //proportion of male population that is in the "high risk" lists based on csw status
		NormalDist activityLevel; //Distribution of activity level (i.e. marbles)

		//the age that partnering discount will start
		int partneringDiscStartAgeYrs;
		//partnering acquisition rates get discounted every year
		double partneringAcqDiscPerYr;
		//coital acts/month get discounted every year
		double partneringActsDiscPerYr;

		//these contain the discount for every age between partneringDiscStartAgeYrs and Person::maxYrForDeathStats
		//we can calculate these once and use them over and over again.
		//  we've actually saved these as multipliers so that for a particular age N > partneringDiscStartAgeYrs,
		//		the acquisition rate will be multiplied by  (1 - partneringAcqDiscPerYr)^(partneringDiscStartAgeYrs - N)
		//so a particular value when multiplied to an acquisition rate will yield the discounted rate
		vector<double> partneringAcqDiscMult;
		//the principle of these multipliers are the same as for partneringAcqDiscMult, but for #acts
		vector<double> partneringActsDiscMult;

		//factors that determind foif
		double circumProtectEff ;	  //transmission protection that circumcision provides (a positive multiplier <= 1)
		double condomProtectEff;   //transmission protection that condoms provide  (a positive multiplier <= 1)
		vector<double> transmitPerEventCoeffs;	 //chance of infection for men->woman, w/o circumcision or condoms

		//-----------< END data fields >--------------------//
	public:
		SubPopParams();
		SubPopParams(ticpp::Element *_maleParams, EventParams &_eventParams);
		int reloadParamsXML(ticpp::Element *_maleParams, EventParams &_eventParams) throw();

		~SubPopParams();
		//-----------< BEGIN getters >--------------------//
		double getChanceBecomeCSW() const;
		double getPartnerAcqMultWithSteady(Person::RiskLevel _risk) const;
		double getTransmitPerEventCoeff(HVLStrata _hvl) const;

		//sexual behavior params for each type as specified by SexualPartnership::Type
		const SexualBehaviorParams *getSexualBehaviorParams(SexualPartnership::Type _type) const;

		double getProportionHighRisk(DmgProfile::Employment _cswStatus) const;
		NormalDist getActivityLevel() const;

		double getCircumProtectEff() const;
		double getCondomProtectEff() const;

		int getPartneringDiscStartAgeYrs() const;
		double getPartneringAcqDiscMult(int _ageYrs) const;
		double getPartneringActsDiscMult(int _ageYrs) const;

		void setChanceCondomUsePerEvent(Person::RiskLevel risk, SexualPartnership::Type partnershipType, BetaDist dist);

		//-----------< END getters >--------------------//
	};
	//----------------< END class SubPopParams >--------------------------//

private:
	//this vector holds Parameters for males different populations it is used to populate fields
	//	for each instance of Male w/ different values depending on which Population the Male is part of
	//	Rationale: So males don't really have to know much about the population they are in except for the ID
	static vector<SubPopParams *> populationSpecificParams;

	//------------< Begin parameters for individual males >-----------------//
	bool circumcised;					//whether they are circumcised
	double partnerAcqRates[SexualPartnership::ENDType];	//the rate at which this male acquires various partners -- this value is drawn from lognormal, but the male's number of partners each month will be drawn from poisson`
	double numActsPerMonth[SexualPartnership::ENDType];	//the # acts per month (fits a poisson distribution with minimum value of 1)
	double chanceCondomUsePerEvent[SexualPartnership::ENDType]; //chance that this male will use condom w/ different partner types
	// Replace range with distribution centered around an age
	//double maxMonthsOlder[SexualPartnership::ENDType];			//how many months older are they willing to go for a partner
	//double maxMonthsYounger[SexualPartnership::ENDType];		//how many months younger are they willing to go for a partner
	NormalDist
	averageYearsYounger[SexualPartnership::ENDType];    // The distribution the males will draw from to determine how many years younger their partner should be (resulting difference may be negative for older women)
	//------------< End parameters for individual males >-----------------//

public:
	/**
	Add a set of population parameters to be used by Males of that population
	@param _populationID basically a check to see that you have added parameters in the right order
	@param _maleParams the XML node that contains the parameter data
	**/
	static void addPopParams(unsigned int _populationID, ticpp::Element *_maleParams, EventParams &_eventParams);
	/**
	Updates a set of population parameters to be used by Males of that population
	@param _populationID basically a check to see that you have added parameters in the right order
	@param _maleParams the XML node that contains the parameter data
	**/
	static void updatePopParams(unsigned int _populationID, ticpp::Element *_maleParams, EventParams &_eventParams);

	/**
	Access a set of population parameters to be used by Males of that population
	**/
	static SubPopParams *getPopParams(unsigned int _populationID);

	/**
	this constructor creates a Male that can be simulated
	constructor should set the CD4, HVL, and HVLsetpoint from age and gender **/
	Male(EventParams &_eventParams, int _age, bool _circumcised, unsigned int _populationID);

	/** Start: Inherited from Person, comments found there **/

	Person *choosePartner(RandomNums &_randomNums, EntityPool *_availableEntities ,
	                      SexualPartnership::Type _partnershipType, bool _remove);

	double getFOI(Person *_p, SexualPartnership::Type _partnershipType, EventParams &_eventParams);

	double getMinPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const;
	double getMaxPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const;
	//Returns the age difference (in years) to center around
	double rollForAgeDifference(SexualPartnership::Type _partnershipType, RandomNums &_randomNums);

	double getTransmissionCoeff();
	bool possibleMatch(SexualPartnership::Type _partnershipType, Person *_p);
	int rollForNumPartners(RandomNums &_randomNums, SexualPartnership::Type _partnershipType);
	int rollNumEventsPerPartner(Person *_p, RandomNums &_randomNums, SexualPartnership::Type _partnershipType);
	int rollForNewPartnershipDuration(SexualPartnership::Type _partnershipType, RandomNums &_randomNums, Person *_p);

	void rerollRiskGroup(EventParams &_eventParams);
	//writes state of person to file
	void saveState(ostream &_outStream, int currTime);

	/** End: Inherited from Person **/

	/** Start: functions for Males only **/
	//calculates the likelihood of using a condom based on the partnering type
	double getCondomUseProb(Person *_p, SexualPartnership::Type _partnershipType);
	//gets the efficacy of using a condom on preventing the spread of HIV
	double getCondomProtectEff();
	//gets the efficacy of circumcision on preventing the spread of HIV
	double getCircumProtectEff();
	//returns true if this male is circumcised
	bool isCircumcised();
	/** End: functions for Males only **/

public:
	~Male(void);
};
