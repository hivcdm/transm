#ifndef MALE_HPP
#define MALE_HPP

#include <map>

#include "entity.hpp"
#include "sexualbehavior.hpp"
#include "parameters/eventparams.hpp"
#include "utility/randomnumbergenerator.hpp"

namespace transm {

class RaceEthnicityMap {
    using RaceEthnicityMapType = std::map<DemographicProfile::Race,
                                          std::vector<DemographicProfile::Ethnicity>>;
  public:
    void insert(DemographicProfile::Race race, std::vector<DemographicProfile::Ethnicity> ethnicities) {
        raceToEthnicityMap[race] = ethnicities;
    }
    std::vector<DemographicProfile::Ethnicity> GetEthnicityForRace(DemographicProfile::Race race) {
        return raceToEthnicityMap.at(race);
    }
    void SetRaceKeysFromMap() {
        for (auto & it : raceToEthnicityMap) {
            raceKeys.push_back(it.first);
        }
    }
    std::vector<DemographicProfile::Race> GetRaceKeysFromMap() {
        return raceKeys;
    }

  private:
    RaceEthnicityMapType raceToEthnicityMap;
    std::vector<DemographicProfile::Race> raceKeys;
};

/** All males in the simulation are members of this class, or a class derived from this one */
class Male : public Entity
{
public :

    /**
	 * These are parameters that describe the population of males.
	 * Each Population in the Sim will have a separate one of these references by the population's ID. */
	class SubPopParams
	{
	public:
		SubPopParams();
		~SubPopParams();

		double getChanceBecomeCSW() const;
		double getPartnerAcqMultWithSteady(RiskLevel _risk) const;

		/* sexual behavior params for each type as specified by SexualPartnership::Type */
		const SexualBehavior &getSexualBehavior(SexualPartnership::Type _type) const;
		SexualBehavior &getSexualBehavior(SexualPartnership::Type _type);

        bool hasSexualBehavior(SexualPartnership::Type type) const { return sexualBehaviorParams.find(type) != sexualBehaviorParams.end(); }

		double getProportionHighRisk(DemographicProfile::Employment _cswStatus) const;

		double getCircumProtectEff() const;
		double getCondomProtectEff() const;

		Age getPartneringDiscStartAgeYrs() const;
		double getPartneringAcqDiscMult(Age _ageYrs) const;
		double getPartneringActsDiscMult(Age _ageYrs) const;
		void setAgeDiscounting(Age startAgeYrs, double partneringAcqDisc, double partneringActsDisc)
		{
			partneringDiscStartAgeYrs = startAgeYrs;
			partneringAcqDiscPerYr = partneringAcqDisc;
			partneringActsDiscPerYr = partneringActsDisc;
			int numMults = Entity::maxYrForDeathStats - partneringDiscStartAgeYrs.get_year() + 1;
			double acqMult = 1 - partneringAcqDiscPerYr;
			double actsMult = 1 - partneringActsDiscPerYr;

			/* generate vectors that contain discount multipliers. will cover from [partneringDiscStartAgeYrs,
			 * Entity::maxYrForDeathStats] */
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

		void SetPartnerAcqMultWithSteady(RiskLevel risk, double multiplier) { partnerAcqMultWithSteady[(std::size_t)risk] = multiplier; }

		void SetChanceBecomeCsw(double chance) { chanceBecomeCSW = chance; }

		void SetCoefficientVariation(bool use, double coefficient) { useCoefficientVariation = use; coefficientOfVariation = coefficient; }

		void AddSexualBehavior(SexualBehavior params) { sexualBehaviorParams[params.getPartnershipType()] = params; }

        void SetProportionHighRisk(DemographicProfile::Employment employment, double proportion) { proportionHighRisk[(std::size_t)employment] = proportion; }

		void SetAverageYearsYounger(SexualPartnership::Type type, NormalDist dist) { sexualBehaviorParams[type].setAverageYearsYounger(dist); }
		void SetAcquisitionRatePerMonth(RiskLevel risk, SexualPartnership::Type type, LogNormalDist dist) { sexualBehaviorParams[type].setAcquisitionRatePerMonth(risk, dist); }
		void SetCoitalEventsPerMonth(RiskLevel risk, SexualPartnership::Type type, double mean) { sexualBehaviorParams[type].setCoitalEventsPerMonth(risk, mean); }
		void SetChanceCondomUsePerEvent(RiskLevel risk, SexualPartnership::Type type, BetaDist dist) { sexualBehaviorParams[type].setChanceCondomUsePerEvent(risk, dist); }
		void SetPartnershipDuration(RiskLevel risk, SexualPartnership::Type type, ShiftedLogNormalDist dist) { sexualBehaviorParams[type].setPartnershipDuration(risk, dist); }
	    BetaDist GetChanceCondomUsePerEvent(RiskLevel risk, SexualPartnership::Type type) const { return getSexualBehavior(type).getChanceCondomUsePerEvent(risk); }

		int GetMaxPartnershipRejections() const { return maxPartnershipRejections; }
		void SetMaxPartnershipRejections(int rejections) { maxPartnershipRejections = rejections; }

	    void SetCswEndAge(Age end_age) { cswEndAge = end_age; }
	    Age GetCswEndAge() const { return cswEndAge; }

	    double GetProportionCircumcised() const { return proportionCircumcised; }
	    void SetProportionCircumcised(double value) { proportionCircumcised = value; }

        void setChanceMsmwChooseMale(double chance) { chanceMsmwChooseMale = chance; }
        double getChanceMsmwChooseMale() const { return chanceMsmwChooseMale; }

        void setChanceMsmChooseMsmw(double chance) { chanceMsmChooseMsmw = chance; }
        double getChanceMsmChooseMsmw() const { return chanceMsmChooseMsmw; }

        double getRiskAssortativeness() const { return riskAssortativeness; }
        void setRiskAssortativeness(double riskAssortativeness) {
            this->riskAssortativeness = riskAssortativeness;
        }

        void setBaselineRaceEthnicAssortativeness(double baselineAssortivity) {
            for (auto race : allowedRaceEthnicityMap.GetRaceKeysFromMap()) {
                for (auto ethnicity : allowedRaceEthnicityMap.GetEthnicityForRace(race)) {
                    raceEthnicAssortivity.emplace_back(race, ethnicity, baselineAssortivity);
                }
            }
        }
        void setRaceEthnicAssortativeness(DemographicProfile::Race race, DemographicProfile::Ethnicity ethnicity,
            double assortivity) {
            using raceEthTuple = std::tuple<DemographicProfile::Race,DemographicProfile::Ethnicity,double>;
            auto it = std::find_if(raceEthnicAssortivity.begin(), raceEthnicAssortivity.end(),
                [&race,&ethnicity](const raceEthTuple& e)
                    { return std::get<0>(e) == race && std::get<1>(e) == ethnicity; });
            if (it != raceEthnicAssortivity.end()) {
                std::get<2>(*it) = assortivity;
            }
        }

        double getRaceEthnicAssortativeness(DemographicProfile::Race race,
            DemographicProfile::Ethnicity ethnicity) {
            using raceEthTuple = std::tuple<DemographicProfile::Race,DemographicProfile::Ethnicity,double>;
            auto it = std::find_if(raceEthnicAssortivity.begin(), raceEthnicAssortivity.end(),
                [&race,&ethnicity](const raceEthTuple& e)
                    { return std::get<0>(e) == race && std::get<1>(e) == ethnicity; });
            if (it != raceEthnicAssortivity.end()) {
                return std::get<2>(*it);
            }
            throw std::runtime_error("Missing assortivity for race and ethnicity pair");
        }

        RaceEthnicityMap* GetAllowedRaceEthnicityMap() {
            return &allowedRaceEthnicityMap;
        }

    protected:
        friend class Male;

    private:
        Age cswEndAge;

		/** chance that a male will become a CSW */
		double chanceBecomeCSW;

		/** the rate multiplier for partner acquisition when a male has a Steady partner */
		double partnerAcqMultWithSteady[(std::size_t)RiskLevel::Last];

		/** sexual behavior params for each type as specified by SexualPartnership::Type */
		std::unordered_map<SexualPartnership::Type, SexualBehavior> sexualBehaviorParams;

		/** proportion of male population that is in the "high risk" lists based on csw status */
		double proportionHighRisk[(std::size_t)DemographicProfile::Employment::Last];

        double proportionCircumcised;

		/** the age that partnering discount will start */
		Age partneringDiscStartAgeYrs;

		/** partnering acquisition rates get discounted every year */
		double partneringAcqDiscPerYr;

		/** coital acts/month get discounted every year */
		double partneringActsDiscPerYr;

		/**
		 * these contain the discount for every age between partneringDiscStartAgeYrs and Entity::maxYrForDeathStats
		 * we can calculate these once and use them over and over again.
		 * we've actually saved these as multipliers so that for a particular age N > partneringDiscStartAgeYrs,
		 * the acquisition rate will be multiplied by  (1 - partneringAcqDiscPerYr)^(partneringDiscStartAgeYrs - N)
		 * so a particular value when multiplied to an acquisition rate will yield the discounted rate */
		std::vector<double> partneringAcqDiscMult;

        /** the principle of these multipliers are the same as for partneringAcqDiscMult, but for #acts */
		std::vector<double> partneringActsDiscMult;

		/* factors that determind foif */
		/* transmission protection that circumcision provides (a positive multiplier <= 1) */
		double circumProtectEff;

		/** transmission protection that condoms provide  (a positive multiplier <= 1) */
		double condomProtectEff;

		double coefficientOfVariation;
		bool useCoefficientVariation;

        /**
         * The number of times the male can be rejected by a female before he
         * decreases his number of partnerships to be formed and stops looking
         * for the current partner. */
        int maxPartnershipRejections;

        double preExposureProphylaxisEfficacy_;

        /** contains all current partnerships including CSW and Casual */
        std::list<SexualPartnership *> partners[(int)SexualPartnership::Type::Last];

        /** assortativeness */
        double chanceMsmwChooseMale;
        double chanceMsmChooseMsmw;
        double riskAssortativeness;

        std::vector<std::tuple<DemographicProfile::Race, DemographicProfile::Ethnicity, double>> raceEthnicAssortivity;

        RaceEthnicityMap allowedRaceEthnicityMap;

	};

public:
    /**
	 * This constructor creates a Male that can be simulated
     * constructor should set the CD4, HVL, and HVLsetpoint from age and gender. */
    Male(EventParams &_eventParams, Age age, bool circumcised, const DemographicProfile &profile,
        unsigned int _populationID, const Male::SubPopParams &params, const PrepParameters &prepParams);

    ~Male();

    virtual const string getEntityType() const;

    void Circumcise();

    bool IsCircumcised() const { return circumcised; }

    void SetProportionHighRisk(DemographicProfile::Employment employment, double proportion) { populationSpecificParams.SetProportionHighRisk(employment, proportion); }

    std::size_t GetSexualOrientation();

	/** Start: Inherited from Entity, comments found there */
    /*@{*/
    Entity *choosePartner(RandomNumberGenerator &_randomNums, EntityPool *_availableEntities,
	                      SexualPartnership::Type _partnershipType, bool _remove);

    void SetChanceCondomUsePerEvent(RiskLevel risk, SexualPartnership::Type partnershipType, BetaDist dist, RandomNumberGenerator &rng);

	const BetaDist GetChanceCondomUsePerEvent(RiskLevel risk, SexualPartnership::Type type)
	{ return populationSpecificParams.getSexualBehavior(type).getChanceCondomUsePerEvent(risk); }

	void SetCoitalEventsPerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, double meanEvents);

	void SetPartnershipDuration(RiskLevel risk, SexualPartnership::Type partnershipType, ShiftedLogNormalDist dist);

	void SetAverageYearsYounger(SexualPartnership::Type partnershipType, NormalDist dist);

	void SetAcquisitionRatePerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, LogNormalDist dist, RandomNumberGenerator &rng);

    void SetChanceBecomeSexWorker(double chance) { populationSpecificParams.SetChanceBecomeCsw(chance); }

    void SetRiskAssortativeness(double assortativeness) {
        populationSpecificParams.setRiskAssortativeness(assortativeness);
    }
    void SetRaceEthnicAssortativeness(DemographicProfile::Race race, DemographicProfile::Ethnicity eth,
        double assortativeness) {
        populationSpecificParams.setRaceEthnicAssortativeness(race, eth, assortativeness);
    }

    double getChanceBecomeCsw() const;

    bool PassedCSWEndAge() const { return (getAge() >= populationSpecificParams.GetCswEndAge()); }

    double getFOI(Entity *_p,
        const std::unordered_map<TransmissionType, std::array<double, (std::size_t)HVLStrata::Last>> &transmission_coefficients,
        SexualPartnership::Type _partnershipType,
        EventParams &_eventParams);

	double getMinPartnerSelectVal(Entity::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const;
	double getMaxPartnerSelectVal(Entity::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const;
	/** Returns the age difference (in years) to center around */
	double rollForAgeDifference(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums);

	bool possibleMatch(SexualPartnership::Type _partnershipType, Entity *_p);
	int rollForNumPartners(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType);
	int rollNumEventsPerPartner(Entity *_p, RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType);
	int rollForNewPartnershipDuration(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums, Entity *_p);
    DemographicProfile::ProfileID ChoosePartnerDemographic(RandomNumberGenerator &_randomNums, SexualPartnership::Type partnershipType);

	void rerollRiskGroup(EventParams &_eventParams);

	/** writes state of person to file */
    void saveState(ostream &_outStream, Time currTime);

	/** End: Inherited from Entity */
    /*@}*/

    /** Start: functions for Males only */
    /*@{*/
    int GetMaxPartnershipRejections() const
	{ return populationSpecificParams.GetMaxPartnershipRejections(); }

	/** calculates the likelihood of using a condom based on the partnering type */
	double getCondomUseProb(Entity *_p, SexualPartnership::Type _partnershipType);

    /** gets the efficacy of using a condom on preventing the spread of HIV */
	double getCondomProtectEff();

    /** gets the efficacy of circumcision on preventing the spread of HIV */
	double getCircumProtectEff();

	/** End: functions for Males only */
    /*@}*/


    void SetPartnershipRejectionChance(RiskLevel, SexualPartnership::Type, double) { }
	double GetPartnershipRejectionChance(RiskLevel, SexualPartnership::Type) const { return 0; }
	void SetOverrideChanceCondomUse(double) { }
	double GetOverrideChanceCondomUse() const { return -1; }

	std::size_t GetTimesSelected() const { return times_selected_; }
	void IncrementTimesSelected() {
	    assert(dmgProfile.get(DemographicProfile::Demographic::SexualOrientation) !=
			(std::size_t) DemographicProfile::SexualOrientation::Msw);
	    times_selected_++;
	}
	void ResetTimesSelected() { times_selected_ = 0; }

private:
	SubPopParams populationSpecificParams;

	/** whether they are circumcised */
	bool circumcised;

	/**
	 * the rate at which this male acquires various partners -- this value is drawn from lognormal, but the male's
	 * number of partners each month will be drawn from poisson */
	double partnerAcqRates[(int)SexualPartnership::Type::Last];

	/** the  acts per month (fits a poisson distribution with minimum value of 1) */
    double numActsPerMonth[(int)SexualPartnership::Type::Last];

	/** chance that this male will use condom w/ different partner types */
	double chanceCondomUsePerEvent[(int)SexualPartnership::Type::Last];

	/**
	 * The distribution the males will draw from to determine how many years younger their partner should be
	 * (resulting difference may be negative for older women) */
	NormalDist averageYearsYounger[(int)SexualPartnership::Type::Last];

	std::size_t times_selected_;
};

} // namespace transm


#endif /* MALE_HPP */