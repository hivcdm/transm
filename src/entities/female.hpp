#ifndef FEMALE_HPP
#define FEMALE_HPP

#include "entity.hpp"
#include "prep.hpp"
#include "parameters/eventparams.hpp"
#include "utility/randomnumbergenerator.hpp"

namespace transm {

/** All the females in the simulation are members of this class, or a class derived from this one */
class Female : public Entity
{
public:
    /*virtual*/ const std::string getEntityType() const;
    /**
	 * These are parameters that describe the population of females.
	 * Each Population in the Sim will have a separate one of these referenced by the population's ID.	*/
	class SubPopParams
	{
	public :
		SubPopParams();

		double GetChanceBecomeCSW() const;
		double GetProportionHighRisk(DemographicProfile::Employment) const;
		void SetChanceBecomeCsw(double chance) { chanceBecomeCSW = chance; }
        void SetProportionHighRisk(DemographicProfile::Employment employment, double proportion) { proportionHighRisk[(std::size_t)employment] = proportion; }

        void SetVaginalMicrobicideEfficacy(double efficacy) { vaginalMicrobicideEfficacy_ = efficacy; }
        double GetVaginalMicrobicideEfficacy() const { return vaginalMicrobicideEfficacy_; }

        void SetCswEndAge(Age end_age) { cswEndAge = end_age; }
        Age GetCswEndAge() const { return cswEndAge; }

	private:
		friend class SimulationBuilder;

        Age cswEndAge;

        /** chance that a female will become a CSW */
		double chanceBecomeCSW;

        /** proportion of female population that is in the "high risk" lists */
        std::array<double, (std::size_t)DemographicProfile::Employment::Last> proportionHighRisk;

        /** chance of infection for women->men, w/o circumcision or condoms */
        double preExposureProphylaxisEfficacy_;
        double vaginalMicrobicideEfficacy_;
	};

public:
	/**
	 * this constructor creates Females that can be simulated
	 * the parameters match the ones in Person(...)
	 * @author schung5 */
	Female(EventParams &_eventParams, Age age, const DemographicProfile &profile, unsigned int _populationID,
           const Female::SubPopParams &params, const PrepParameters &prepParams);

	~Female(void);

    bool IsCircumcised() const { return false; }

    void SetProportionHighRisk(DemographicProfile::Employment employment, double proportion) {
        populationSpecificParams.SetProportionHighRisk(employment, proportion);
    }

    void SetChanceBecomeSexWorker(double chance) { populationSpecificParams.SetChanceBecomeCsw(chance); }

    void SetRiskAssortativeness(double /*assortativeness*/) {
        throw std::runtime_error("not implemented for women");
    }

    void SetRaceEthnicAssortativeness(DemographicProfile::Race /*race*/,
                                      DemographicProfile::Ethnicity /*eth*/,
                                      double /*assortativeness*/) {
        throw std::runtime_error("not implemented for women");
    }

    void SetVaginalMicrobicideAdherence(double adherence);

    void SetVaginalMicrobicideEfficacy(double efficacy) {
        populationSpecificParams.SetVaginalMicrobicideEfficacy(efficacy);
    }

    double GetVaginalMicrobicideEfficacy() const;

    bool RollForVaginalMicrobicideUse(RandomNumberGenerator &rng)
    {
        vaginalMicrobicideUsedLastFOICalculation = false;
        if (vaginalMicrobicideAdherence_ > 0)
        {
            vaginalMicrobicideUsedLastFOICalculation = rng.chance(vaginalMicrobicideAdherence_);
            if (vaginalMicrobicideUsedLastFOICalculation)
            {
                IncrementVaginalMicrobicideApplications();
            }
        }
        return vaginalMicrobicideUsedLastFOICalculation;
    }

    void ResetVaginalMicrobicideUsage()
    {
        vaginalMicrobicideApplicationsThisMonth = 0;
    }

    int GetVaginalMicrobicideApplicationsThisMonth()
    {
        return vaginalMicrobicideApplicationsThisMonth;
    }

    void IncrementVaginalMicrobicideApplications()
    {
        vaginalMicrobicideApplicationsThisMonth++;
    }

	/** Start: Inherited from Person, comments found there */
	/**
	 * @return the force of infection for this female infecting an uninfected male
	 * @author schung5 */
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

    bool PassedCSWEndAge() const { return (getAge() >= populationSpecificParams.GetCswEndAge()); }

    void SetChanceCondomUsePerEvent(RiskLevel risk, SexualPartnership::Type partnershipType, BetaDist dist, RandomNumberGenerator &rng);

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

    const BetaDist GetChanceCondomUsePerEvent(RiskLevel risk, SexualPartnership::Type type) { BetaDist dist; return dist; }

private:

	SubPopParams populationSpecificParams;

    double overrideChanceCondomUse_;

    double vaginalMicrobicideAdherence_;

    int vaginalMicrobicideApplicationsThisMonth;

    bool vaginalMicrobicideUsedLastFOICalculation;

    std::map<RiskLevel, std::map<SexualPartnership::Type, double>> partnershipRejectionChance_;

    std::size_t times_selected_;
};

} // namespace transm


#endif /* FEMALE_HPP */