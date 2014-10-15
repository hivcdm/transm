#pragma once

#include <map>
#include <pugixml.hpp>

#include "Person.hpp"
#include "SexualBehavior.hpp"
#include "data/EventParams.hpp"
#include "utility/RandomNumberGenerator.hpp"

/// <summary>
/// All females in the simulation are members of this class, or a class derived from this one
/// </summary>
class Male : public Person
{
public :
    /*virtual*/ std::string getEntityType() const;
	void Circumcise();
    bool IsCircumcised() const { return circumcised; }

    void SetProportionHighRisk(DemographicProfile::Employment employment, double proportion) { populationSpecificParams.SetProportionHighRisk(employment, proportion); }

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

		//sexual behavior params for each type as specified by SexualPartnership::Type
		const SexualBehavior &getSexualBehavior(SexualPartnership::Type _type) const;
		SexualBehavior &getSexualBehavior(SexualPartnership::Type _type);

        bool hasSexualBehavior(SexualPartnership::Type type) const { return sexualBehaviorParams.find(type) != sexualBehaviorParams.end(); }

		double getProportionHighRisk(DemographicProfile::Employment _cswStatus) const;
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

		void SetCircucmsionProtectEfficacy(double efficacy) { circumProtectEff = efficacy; }
		void SetCondomProtectEff(double efficacy) { condomProtectEff = efficacy; }

		void SetPartnerAcqMultWithSteady(Person::RiskLevel risk, double multiplier) { partnerAcqMultWithSteady[risk] = multiplier; }

		void SetChanceBecomeCsw(double chance) { chanceBecomeCSW = chance; }

		void SetCoefficientVariation(bool use, double coefficient) { useCoefficientVariation = use; coefficientOfVariation = coefficient; }

		void AddSexualBehavior(SexualBehavior params) { sexualBehaviorParams[params.getPartnershipType()] = params; }

        void SetProportionHighRisk(DemographicProfile::Employment employment, double proportion) { proportionHighRisk[(std::size_t)employment] = proportion; }

		void SetAverageYearsYounger(SexualPartnership::Type type, NormalDist dist) { sexualBehaviorParams[type].setAverageYearsYounger(dist); }
		void SetAcquisitionRatePerMonth(Person::RiskLevel risk, SexualPartnership::Type type, LogNormalDist dist) { sexualBehaviorParams[type].setAcquisitionRatePerMonth(risk, dist); }
		void SetCoitalEventsPerMonth(Person::RiskLevel risk, SexualPartnership::Type type, double mean) { sexualBehaviorParams[type].setCoitalEventsPerMonth(risk, mean); }
		void SetChanceCondomUsePerEvent(Person::RiskLevel risk, SexualPartnership::Type type, BetaDist dist) { sexualBehaviorParams[type].setChanceCondomUsePerEvent(risk, dist); }
		void SetPartnershipDuration(Person::RiskLevel risk, SexualPartnership::Type type, ShiftedLogNormalDist dist) { sexualBehaviorParams[type].setPartnershipDuration(risk, dist); }

		void SetActivityLevel(NormalDist activity_level) { activityLevel = activity_level; }

        int GetMaxPartnershipRejections() const { return maxPartnershipRejections; }
        void SetMaxPartnershipRejections(int rejections) { maxPartnershipRejections = rejections; }

        void SetCswEndAge(int end_age) { cswEndAge = end_age; }
        int GetCswEndAge() const { return cswEndAge; }

	private:
        int cswEndAge; // in months
		double chanceBecomeCSW;		//chance that a male will become a CSW
		double partnerAcqMultWithSteady[Person::ENDRiskLevel];  //the rate multiplier for partner acquisition when a male has a Steady partner

		//sexual behavior params for each type as specified by SexualPartnership::Type
		std::unordered_map<SexualPartnership::Type, SexualBehavior> sexualBehaviorParams;
		double proportionHighRisk[(std::size_t)DemographicProfile::Employment::Last];  //proportion of male population that is in the "high risk" lists based on csw status
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

		double coefficientOfVariation;
		bool useCoefficientVariation;

        // The number of times the male can be rejected by a female before he
        // decreases his number of partnerships to be formed and stops looking
        // for the current partner.
        int maxPartnershipRejections;
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

public:
	/**
	this constructor creates a Male that can be simulated
	constructor should set the CD4, HVL, and HVLsetpoint from age and gender **/
	Male(EventParams &_eventParams, int _age, bool _circumcised, unsigned int _populationID, const Male::SubPopParams &params);

	/** Start: Inherited from Person, comments found there **/

	Person *choosePartner(RandomNumberGenerator &_randomNums, EntityPool *_availableEntities,
	                      SexualPartnership::Type _partnershipType, bool _remove);

    void SetChanceCondomUsePerEvent(Person::RiskLevel risk, SexualPartnership::Type partnershipType, BetaDist dist, RandomNumberGenerator &rng);

	void SetCoitalEventsPerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, double meanEvents);

	void SetPartnershipDuration(RiskLevel risk, SexualPartnership::Type partnershipType, ShiftedLogNormalDist dist);

	void SetAverageYearsYounger(SexualPartnership::Type partnershipType, NormalDist dist);

	void SetAcquisitionRatePerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, LogNormalDist dist, RandomNumberGenerator &rng);

    void SetChanceBecomeSexWorker(double chance) { populationSpecificParams.SetChanceBecomeCsw(chance); }

    void SetAssortativeness(SexualPartnership::Type partnership_type, double assortativeness) { populationSpecificParams.getSexualBehavior(partnership_type).setAssortativeness(assortativeness); }

	double getChanceBecomeCsw() const;

    double getFOI(Person *_p, const std::unordered_map<TransmissionType, std::array<double, ENDHVLStrata>> &transmission_coefficients, SexualPartnership::Type _partnershipType, EventParams &_eventParams);

	double getMinPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const;
	double getMaxPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const;
	//Returns the age difference (in years) to center around
	double rollForAgeDifference(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums);

	bool possibleMatch(SexualPartnership::Type _partnershipType, Person *_p);
	int rollForNumPartners(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType);
	int rollNumEventsPerPartner(Person *_p, RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType);
	int rollForNewPartnershipDuration(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums, Person *_p);

	void rerollRiskGroup(EventParams &_eventParams);
	//writes state of person to file
	void saveState(ostream &_outStream, long currTime);
	/** End: Inherited from Person **/

	/** Start: functions for Males only **/
    int getMaxPartnershipRejections() const { return populationSpecificParams.GetMaxPartnershipRejections(); }
	//calculates the likelihood of using a condom based on the partnering type
	double getCondomUseProb(Person *_p, SexualPartnership::Type _partnershipType);
	//gets the efficacy of using a condom on preventing the spread of HIV
	double getCondomProtectEff();
	//gets the efficacy of circumcision on preventing the spread of HIV
	double getCircumProtectEff();
	//returns true if this male is circumcised
	bool isCircumcised();
	/** End: functions for Males only **/

    void SetPartnershipRejectionChance(RiskLevel, SexualPartnership::Type, double) { throw std::runtime_error("not allowed for males"); };
    double GetPartnershipRejectionChance(RiskLevel, SexualPartnership::Type) const { throw std::runtime_error("not allowed for males"); };
    void SetOverrideChanceCondomUse(double) { throw std::runtime_error("not allowed for males"); };
    double GetOverrideChanceCondomUse() const { throw std::runtime_error("not allowed for males"); };

	~Male();
};
