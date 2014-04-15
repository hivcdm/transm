#pragma once

#include <map>

#include "Person.h"
#include "behaviors/SexualBehaviorParams.h"
#include "../data/EventParams.h"
#include "../util/rand/RandomNums.h"
#include "../util/xml/pugixml.hpp"

/// <summary>
/// All females in the simulation are members of this class, or a class derived from this one
/// </summary>
class Male : public Person
{
public :
	void Circumcise();

	/// <summary>
	/// These are parameters that describe the population of males.
	/// Each Population in the Sim will have a separate one of these references by the population's ID.
	/// </summary>
	class SubPopParams
	{
	public:
		SubPopParams();
		~SubPopParams();

		double getChanceBecomeCSW() const;
		double getPartnerAcqMultWithSteady(Person::RiskLevel _risk) const;
		double getTransmitPerEventCoeff(HVLStrata _hvl) const;

		//sexual behavior params for each type as specified by SexualPartnership::Type
		const SexualBehaviorParams &getSexualBehaviorParams(SexualPartnership::Type _type) const;
		SexualBehaviorParams &getSexualBehaviorParams(SexualPartnership::Type _type);

		double getProportionHighRisk(DmgProfile::Employment _cswStatus) const;
		NormalDist getActivityLevel() const;

		double getCircumProtectEff() const;
		double getCondomProtectEff() const;

		int getPartneringDiscStartAgeYrs() const;
		double getPartneringAcqDiscMult(int _ageYrs) const;
		double getPartneringActsDiscMult(int _ageYrs) const;
		void setAgeDiscounting(int startAgeYrs, double partneringAcqDisc, double partneringActsDisc)
		{
			partneringDiscStartAgeYrs = startAgeYrs;
			partneringAcqDiscPerYr = partneringAcqDisc;
			partneringActsDiscPerYr = partneringActsDisc;
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
		}

		void setCircucmsionProtectEfficacy(double efficacy) { circumProtectEff = efficacy; }
		void setCondomProtectEff(double efficacy) { condomProtectEff = efficacy; }

		void setPartnerAcqMultWithSteady(Person::RiskLevel risk, double multiplier) { partnerAcqMultWithSteady[risk] = multiplier; }

		void setChanceBecomeCsw(double chance) { chanceBecomeCSW = chance; }

		void setCoefficientVariation(bool use, double coefficient) { useCoefficientVariation = use; coefficientOfVariation = coefficient; }

		void addSexualBehaviorParams(SexualBehaviorParams params) { sexualBehaviorParams.push_back(params); }

		void setTransmitPerEventCoeff(HVLStrata hvl, double coeff) { transmitPerEventCoeffs[hvl] = coeff; }

		void setProportionHighRisk(DmgProfile::Employment employment, double proportion) { proportionHighRisk[employment] = proportion; }

		void setAverageYearsYounger(SexualPartnership::Type type, NormalDist dist) { sexualBehaviorParams[(int)type].setAverageYearsYounger(dist); }
		void setAcquisitionRatePerMonth(Person::RiskLevel risk, SexualPartnership::Type type, LogNormalDist dist) { sexualBehaviorParams[(int)type].setAcquisitionRatePerMonth(risk, dist); }
		void setCoitalEventsPerMonth(Person::RiskLevel risk, SexualPartnership::Type type, double mean) { sexualBehaviorParams[(int)type].setCoitalEventsPerMonth(risk, mean); }
		void setChanceCondomUsePerEvent(Person::RiskLevel risk, SexualPartnership::Type type, BetaDist dist) { sexualBehaviorParams[(int)type].setChanceCondomUsePerEvent(risk, dist); }
		void setPartnershipDuration(Person::RiskLevel risk, SexualPartnership::Type type, ShiftedLogNormalDist dist) { sexualBehaviorParams[(int)type].setPartnershipDuration(risk, dist); }

		void setActivityLevel(NormalDist activity_level) { activityLevel = activity_level; }

	private:
		double chanceBecomeCSW;		//chance that a male will become a CSW
		double partnerAcqMultWithSteady[Person::ENDRiskLevel];  //the rate multiplier for partner acquisition when a male has a Steady partner

		//sexual behavior params for each type as specified by SexualPartnership::Type
		std::vector<SexualBehaviorParams> sexualBehaviorParams;
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
		std::vector<double> partneringAcqDiscMult;
		//the principle of these multipliers are the same as for partneringAcqDiscMult, but for #acts
		std::vector<double> partneringActsDiscMult;

		//factors that determind foif
		double circumProtectEff;	  //transmission protection that circumcision provides (a positive multiplier <= 1)
		double condomProtectEff;   //transmission protection that condoms provide  (a positive multiplier <= 1)
		std::array<double, HVLStrata::ENDHVLStrata> transmitPerEventCoeffs;	 //chance of infection for men->woman, w/o circumcision or condoms

		double coefficientOfVariation;
		bool useCoefficientVariation;
	};

private:
	SubPopParams populationSpecificParams;

	//whether they are circumcised
	bool circumcised;

	//the rate at which this male acquires various partners -- this value is drawn from lognormal, but the male's number of partners each month will be drawn from poisson`
	double partnerAcqRates[(int)SexualPartnership::Type::ENDType];
	
	//the  acts per month (fits a poisson distribution with minimum value of 1)
	double numActsPerMonth[(int)SexualPartnership::Type::ENDType];	
	
	//chance that this male will use condom w/ different partner types
	double chanceCondomUsePerEvent[(int)SexualPartnership::Type::ENDType];

	// The distribution the males will draw from to determine how many years younger their partner should be (resulting difference may be negative for older women)
	NormalDist averageYearsYounger[(int)SexualPartnership::Type::ENDType];
	//------------< End parameters for individual males >-----------------//

public:
	/**
	this constructor creates a Male that can be simulated
	constructor should set the CD4, HVL, and HVLsetpoint from age and gender **/
	Male(EventParams &_eventParams, int _age, bool _circumcised, unsigned int _populationID, const Male::SubPopParams &params);

	/** Start: Inherited from Person, comments found there **/

	/*
	Person *choosePartner(RandomNums &_randomNums, EntityPool *_availableEntities ,
	                      SexualPartnership::Type _partnershipType, bool _remove);
						  */
	void SetChanceCondomUsePerEvent(Person::RiskLevel risk, SexualPartnership::Type partnershipType, BetaDist dist);

	void SetCoitalEventsPerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, double meanEvents);

	void SetPartnershipDuration(RiskLevel risk, SexualPartnership::Type partnershipType, ShiftedLogNormalDist dist);

	void SetAverageYearsYounger(SexualPartnership::Type partnershipType, NormalDist dist);

	void SetAcquisitionRatePerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, LogNormalDist dist);

	double getChanceBecomeCsw() const;

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
	void saveState(ostream &_outStream, long currTime);

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
