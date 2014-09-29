#pragma once

#include "Person.hpp"
#include "data/EventParams.hpp"
#include "utility/RandomNumberGenerator.hpp"

/// <summary>
/// All females in the simulation are members of this class, or a class derived from this one
/// </summary>
class Female : public Person
{
public:
	/// <summary>
	/// These are parameters that describe the population of females.
	/// Each Population in the Sim will have a separate one of these referenced by the population's ID.
	/// </summary>
	class SubPopParams
	{
	public :
		SubPopParams();

		double GetChanceBecomeCSW() const;
		double GetProportionHighRisk(DemographicProfile::Employment) const;
		NormalDist GetActivityLevel() const;
		void SetActivityLevel(NormalDist &dist) { activityLevel = dist; }
		double GetTransmitPerEventCoeff(HVLStrata _hvl) const;
		void SetTransmitPerEventCoeff(HVLStrata hvl, double coeff) { transmitPerEventCoeffs[hvl] = coeff; }
		void SetChanceBecomeCsw(double chance) { chanceBecomeCSW = chance; }
        void SetProportionHighRisk(DemographicProfile::Employment employment, double proportion) { proportionHighRisk[(std::size_t)employment] = proportion; }

	private:
		friend class SimulationBuilder;

        //chance that a female will become a CSW
		double chanceBecomeCSW;
        //proportion of female population that is in the "high risk" lists
        std::array<double, (std::size_t)DemographicProfile::Employment::Last> proportionHighRisk;
        //Distribution of activity level (i.e. marbles)
		NormalDist activityLevel;
        //chance of infection for women->men, w/o circumcision or condoms
		std::array<double, HVLStrata::ENDHVLStrata> transmitPerEventCoeffs;
	};

public:
	/**
	//this constructor creates Females that can be simulated
	//the parameters match the ones in Person(...)
	@author schung5
	**/
	Female(EventParams &_eventParams, int _ageMths, unsigned int _populationID, const Female::SubPopParams &params);
	~Female(void);

    bool IsCircumcised() const { return false; }

    void SetProportionHighRisk(DemographicProfile::Employment employment, double proportion) { populationSpecificParams.SetProportionHighRisk(employment, proportion); }

    void SetChanceBecomeSexWorker(double chance) { populationSpecificParams.SetChanceBecomeCsw(chance); }

    void SetTransmissionCoefficient(HVLStrata stratum, double coefficient) { populationSpecificParams.SetTransmitPerEventCoeff(stratum, coefficient); }

    void SetAssortativeness(SexualPartnership::Type /*partnership_type*/, double /*assortativeness*/) { throw std::runtime_error("not implemented for women"); }

	/** Start: Inherited from Person, comments found there **/

	/**
	As of 9/8/08, this only assumes heterosexual relationships.
	@return the force of infection for this female infecting an uninfected male
	@author schung5
	**/
	double getFOI(Person *_p, SexualPartnership::Type _partnershipType, EventParams &_eventParams);

	double getMinPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const;
	double getMaxPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const;

    double rollForAgeDifference(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums);

    bool possibleMatch(SexualPartnership::Type _partnershipType, Person *_p);

    int rollNumEventsPerPartner(Person *_p, RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType);

    int rollForNewPartnershipDuration(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums, Person *_p);

    int rollForNumPartners(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType);

	void Circumcise();

	double getTransmissionCoeff();
	void rerollRiskGroup(EventParams &_eventParams);
	//writes state of person to file
	void saveState(ostream &_outStream, long currTime);

	double getChanceBecomeCsw() const;

    void SetChanceCondomUsePerEvent(Person::RiskLevel risk, SexualPartnership::Type partnershipType, BetaDist dist, RandomNumberGenerator &rng);
	void SetCoitalEventsPerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, double meanEvents);
	void SetPartnershipDuration(RiskLevel risk, SexualPartnership::Type partnershipType, ShiftedLogNormalDist dist);
	void SetAverageYearsYounger(SexualPartnership::Type partnershipType, NormalDist dist);
    void SetAcquisitionRatePerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, LogNormalDist dist, RandomNumberGenerator &rng);
    void SetPartnershipRejectionChance(RiskLevel risk, SexualPartnership::Type partnershipType, double chance) { partnershipRejectionChance_[risk][partnershipType] = chance; }
    double GetPartnershipRejectionChance(RiskLevel risk, SexualPartnership::Type partnershipType) const { return partnershipRejectionChance_.at(risk).at(partnershipType); };
    void SetOverrideChanceCondomUse(double chance) { overrideChanceCondomUse_ = chance; }
    double GetOverrideChanceCondomUse() const { return overrideChanceCondomUse_; }

private:
	SubPopParams populationSpecificParams;
    double overrideChanceCondomUse_;
    std::map<RiskLevel, std::map<SexualPartnership::Type, double>> partnershipRejectionChance_;
};
