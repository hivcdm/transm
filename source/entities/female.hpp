#pragma once

#include "entity.hpp"
#include "parameters/eventparams.hpp"
#include "utility/randomnumbergenerator.hpp"

namespace transm {

/// <summary>
/// All females in the simulation are members of this class, or a class derived from this one
/// </summary>
class Female : public Entity
{
public:
    /*virtual*/ std::string getEntityType() const;
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
		void SetChanceBecomeCsw(double chance) { chanceBecomeCSW = chance; }
        void SetProportionHighRisk(DemographicProfile::Employment employment, double proportion) { proportionHighRisk[(std::size_t)employment] = proportion; }

        void SetCswEndAge(Age end_age) { cswEndAge = end_age; }
        Age GetCswEndAge() const { return cswEndAge; }

	private:
		friend class SimulationBuilder;

        Age cswEndAge;
        //chance that a female will become a CSW
		double chanceBecomeCSW;
        //proportion of female population that is in the "high risk" lists
        std::array<double, (std::size_t)DemographicProfile::Employment::Last> proportionHighRisk;
        //Distribution of activity level (i.e. marbles)
		NormalDist activityLevel;
	};

public:
	/**
	//this constructor creates Females that can be simulated
	//the parameters match the ones in Person(...)
	@author schung5
	**/
	Female(EventParams &_eventParams, Age age, unsigned int _populationID, const Female::SubPopParams &params);
	~Female(void);

    bool IsCircumcised() const { return false; }

    void SetProportionHighRisk(DemographicProfile::Employment employment, double proportion) { populationSpecificParams.SetProportionHighRisk(employment, proportion); }

    void SetChanceBecomeSexWorker(double chance) { populationSpecificParams.SetChanceBecomeCsw(chance); }

    void SetAssortativeness(SexualPartnership::Type /*partnership_type*/, double /*assortativeness*/) { throw std::runtime_error("not implemented for women"); }

	/** Start: Inherited from Person, comments found there **/

	/**
	As of 9/8/08, this only assumes heterosexual relationships.
	@return the force of infection for this female infecting an uninfected male
	@author schung5
	**/
    double getFOI(Entity *_p, const std::unordered_map<TransmissionType, std::array<double, (std::size_t)HVLStrata::Last>> &transmission_coefficients, SexualPartnership::Type _partnershipType, EventParams &_eventParams);

	double getMinPartnerSelectVal(Entity::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const;
	double getMaxPartnerSelectVal(Entity::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const;

    double rollForAgeDifference(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums);

    bool possibleMatch(SexualPartnership::Type _partnershipType, Entity *_p);

    int rollNumEventsPerPartner(Entity *_p, RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType);

    int rollForNewPartnershipDuration(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums, Entity *_p);

    int rollForNumPartners(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType);

	void Circumcise();

	void rerollRiskGroup(EventParams &_eventParams);

	double getChanceBecomeCsw() const;

    void SetChanceCondomUsePerEvent(Entity::RiskLevel risk, SexualPartnership::Type partnershipType, BetaDist dist, RandomNumberGenerator &rng);
	void SetCoitalEventsPerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, double meanEvents);
	void SetPartnershipDuration(RiskLevel risk, SexualPartnership::Type partnershipType, ShiftedLogNormalDist dist);
	void SetAverageYearsYounger(SexualPartnership::Type partnershipType, NormalDist dist);
    void SetAcquisitionRatePerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, LogNormalDist dist, RandomNumberGenerator &rng);
    void SetPartnershipRejectionChance(RiskLevel risk, SexualPartnership::Type partnershipType, double chance) { partnershipRejectionChance_[risk][partnershipType] = chance; }
    double GetPartnershipRejectionChance(RiskLevel risk, SexualPartnership::Type partnershipType) const { return partnershipRejectionChance_.at(risk).at(partnershipType); };
    void SetOverrideChanceCondomUse(double chance) { overrideChanceCondomUse_ = chance; }
    double GetOverrideChanceCondomUse() const { return overrideChanceCondomUse_; }

	std::size_t GetTimesSelected() const { return times_selected_; }
	void IncrementTimesSelected() { times_selected_++; }
	void ResetTimesSelected() { times_selected_ = 0; }

private:
	SubPopParams populationSpecificParams;
    double overrideChanceCondomUse_;
    std::map<RiskLevel, std::map<SexualPartnership::Type, double>> partnershipRejectionChance_;
	std::size_t times_selected_;
};

} // namespace transm
