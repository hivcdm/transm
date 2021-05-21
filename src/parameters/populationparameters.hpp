#pragma once

#include "entities/entitytypes.hpp"
#include "entities/demographicprofile.hpp"
#include "entities/female.hpp"
#include "entities/male.hpp"
#include "entities/prep.hpp"
#include "entities/transmissiontype.hpp"
#include "parameters/agebucketprevalenceinfo.hpp"
#include "parameters/agerangesizecontainer.hpp"
#include "statistics/coststracker.hpp"
#include "utility/time.hpp"

namespace transm {

class FertilityRate {
  public:
	FertilityRate();
	FertilityRate(Age l, Age u, double r)
	{
		lower = l;
		upper = u;
		rate = r;
	}
	Age Lower() const { return lower; }
	Age Upper() const { return upper; }
	double Rate() const { return rate; }

  private:
	Age lower;
	Age upper;
	double rate;
};

class PopulationParameters
{
public:

	PopulationParameters();
	~PopulationParameters();

    struct PopulationTarget
    {
	template<typename T>
	using Optional = std::pair<bool, T>;

	Optional<DemographicProfile> profile;
	Optional<Age> min_age;
	Optional<Age> max_age;
	Optional<RiskLevel> risk;

	bool match(const Entity *e)
	{
	  if (profile.first && !e->getDemographicProfile()->match(profile.second))
	    {
	      return false;
	    }

	  if (min_age.first && e->getAge() < min_age.second)
	    {
	      return false;
	    }

	  if (max_age.first && e->getAge() > max_age.second)
	    {
	      return false;
	    }

	  if (risk.first && e->getRiskLevel() != risk.second)
	    {
	      return false;
	    }

	  return true;
	}

      DemographicProfile::Gender get_gender()
      {
	  DemographicProfile::Gender gender = DemographicProfile::Gender::Last;
	  if (profile.first)
	    gender = (DemographicProfile::Gender)
	      profile.second.get(DemographicProfile::Demographic::Gender);
	  return gender;
      }
    };

    void AddInfectionTarget(const PopulationTarget &target, std::size_t number)
    {
	initial_infection_targets_.push_back({target, number});
    }

    bool GetUseBirthRate() { return useBirthRate; } const
    void SetUseBirthRate(bool value) { useBirthRate = value; }
    double GetBirthRate() { return birthRate; } const
    void SetBirthRate(double birth_rate) { birthRate = birth_rate; }

    std::vector<FertilityRate> GetFertilityRates() const { return fertilityRates; }
    void ClearFertilityRates() { fertilityRates.clear(); }
    void PushFertilityRate(FertilityRate rate) { fertilityRates.push_back(rate); }

	const std::vector<DemographicProfile::DoublePair> &GetBirthProportions() const
	{ return birthProportions; }
	double GetBirthProportion(DemographicProfile profile);
    void SetBirthProportion(DemographicProfile profile, double proportion)
	{ birthProportions.push_back(DemographicProfile::DoublePair(profile, proportion)); }

	Age GetAgeOfMajority() const { return ageOfMajority; }
	void SetAgeOfMajority(Age age) { ageOfMajority = age; }

	void SetPartnershipHasDuration(DemographicProfile::Gender gender, SexualPartnership::Type type,
	    bool has_duration)
		{
			partnershipsHaveDuration[(std::size_t)gender][(std::size_t)type] = has_duration;
		}

    const PrepParameters &GetPrepParameters() const { return defaultPrepParams; }
    void SetPrepParameters(PrepParameters &params) { defaultPrepParams = params; }

	const Male::SubPopParams &GetMaleParameters() const { return defaultMaleParams; }
	void SetMaleParameters(Male::SubPopParams &params) { defaultMaleParams = params; }
	const Female::SubPopParams &GetFemaleParameters() const { return defaultFemaleParams; }
	void SetFemaleParameters(Female::SubPopParams &params) { defaultFemaleParams = params; }

	void SetTransmissionCoefficients(const std::unordered_map<TransmissionType, std::array<double, (std::size_t)HVLStrata::Last>> &coefficients)
	{
        transmission_coefficients_ = coefficients;
	}

    void SetSexualActivityDelay(TimeSpan delay) { sexualActivityDelay = delay; }
    TimeSpan GetSexualActivityDelay() const { return sexualActivityDelay; }

    void SetRiskAssortativeness(double assortativeness) {
        defaultMaleParams.setRiskAssortativeness(assortativeness);
    }

    void SetRaceEthnicAssortativeness(DemographicProfile::Race race,
        DemographicProfile::Ethnicity ethnicity, double assortativeness) {
        defaultMaleParams.setRaceEthnicAssortativeness(race, ethnicity, assortativeness);
    }

	int GetInitialSize() const { return initSize; }
	void SetInitialSize(int size) { initSize = size; }

	double GetChanceChronicInfection() const {return chronicInfectionRate; }
	void SetChanceChronicInfection(double rate) { chronicInfectionRate = rate; }

	const std::vector<AgeBucketPrevalenceInfo> &GetInitialAgeBuckets() const { return initialAgeBuckets; }
	std::vector<AgeBucketPrevalenceInfo> &GetInitialAgeBuckets() { return initialAgeBuckets; }

	// create a list of age ranges from the initial age buckets
	void SetAgeRanges();
	std::vector<AgeRange> GetAgeRanges() const { return ageRanges; }

	void SetSeedDelay(int delay) { seedDelay = Time().from_months(delay); }
	Time GetSeedDelay() { return seedDelay; }
	void SetSeedPrevalence(double prev) { seedPrevalence = prev; }
	double GetSeedPrevalence() { return seedPrevalence; }
	void SetUseSeedCoefficients(bool useCoeffs) { useSeedCoefficients = useCoeffs; }
	bool UseSeedCoefficients() { return useSeedCoefficients; }

    void SetChanceBecomeCsw(DemographicProfile::Gender gender, double chance) {
	switch(gender) {
	case DemographicProfile::Gender::Male:
	    defaultMaleParams.SetChanceBecomeCsw(chance);
	    break;
	case DemographicProfile::Gender::Female:
	    defaultFemaleParams.SetChanceBecomeCsw(chance);
	    break;
	default:
	    throw std::runtime_error("bad gender");
	}
    }

    void SetProportionHighRisk(DemographicProfile::Gender gender, DemographicProfile::Employment employment, double proportion)
    {
	switch(gender) {
	case DemographicProfile::Gender::Male:
	    defaultMaleParams.SetProportionHighRisk(employment, proportion);
	    break;
	case DemographicProfile::Gender::Female:
	    defaultFemaleParams.SetProportionHighRisk(employment, proportion);
	    break;
	default:
	    throw std::runtime_error("bad gender");
	}
    }

	void SetAverageYearsYounger(SexualPartnership::Type type, NormalDist dist) { defaultMaleParams.SetAverageYearsYounger(type, dist); }

	void SetAcquisitionRatePerMonth(RiskLevel risk, SexualPartnership::Type type, LogNormalDist dist) { defaultMaleParams.SetAcquisitionRatePerMonth(risk, type, dist); }
	void SetCoitalEventsPerMonth(RiskLevel risk, SexualPartnership::Type type, double mean) { defaultMaleParams.SetCoitalEventsPerMonth(risk, type, mean); }
	void SetChanceCondomUsePerEvent(RiskLevel risk, SexualPartnership::Type type, BetaDist dist) { defaultMaleParams.SetChanceCondomUsePerEvent(risk, type, dist); }
	const BetaDist GetChanceCondomUsePerEvent(RiskLevel risk, SexualPartnership::Type type)
	{ return defaultMaleParams.getSexualBehavior(type).getChanceCondomUsePerEvent(risk); }
	void SetPartnershipDuration(RiskLevel risk, SexualPartnership::Type type, ShiftedLogNormalDist dist) { defaultMaleParams.SetPartnershipDuration(risk, type, dist); }

    double GetProportionCircumcised() const { return defaultMaleParams.GetProportionCircumcised(); }
    void SetProportionCircumcised(double value) { defaultMaleParams.SetProportionCircumcised(value); }

	void SetPartnerAcquisitionSteadyMultiplier(RiskLevel risk, double multiplier) { defaultMaleParams.SetPartnerAcqMultWithSteady(risk, multiplier); }

    void SetCondomCost(double condom_cost) { condomCost = condom_cost; }
    void SetCircumcisionCost(double circumcision_cost) { circumcisionCost = circumcision_cost; }
    void SetPrEPCost(double cost) { prEPCost = cost; }
    void SetVaginalMicrobicideCost(double cost) { vaginalMicrobicideApplicationCost = cost; }

protected:
	friend class Population;

private:
	long initSize;

	double chronicInfectionRate;

    /* Population growth parameters: Per month per person based on WHI data
     * @param birthRate
     * @fertilityRates vector of FertilityRate
     * */
	bool useBirthRate;
	double birthRate;
	std::vector<FertilityRate> fertilityRates;

    /// <summary>
    /// Age that entities roll to change risk.
    /// </summary>
	Age ageOfMajority;

    TimeSpan sexualActivityDelay;

    std::vector<DemographicProfile::DoublePair> birthProportions;

    /// <summary>
    /// popilation proportions stratified by age.
    /// </summary>
	std::vector<AgeBucketPrevalenceInfo> initialAgeBuckets;
	std::vector<AgeRange> ageRanges;

    /// <summary>
    /// initial infection targets
    /// </summary>
    std::vector<std::pair<PopulationTarget, std::size_t>> initial_infection_targets_;

    /// <summary>
    /// prevalence parameters stratified by age.
    /// </summary>
    Time seedDelay;
    double seedPrevalence{};
    bool useSeedCoefficients{};
    int minSeedAge{};
    int maxSeedAge{};

    /// <summary>
    /// Base FOI for different transmission types at various viral loads.
    /// </summary>
    std::unordered_map<TransmissionType, std::array<double, (std::size_t)HVLStrata::Last>> transmission_coefficients_;

    /// <summary>
    /// holds the population-level parameters for population of heterosexual males
    /// </summary>
	Male::SubPopParams defaultMaleParams;

    /// <summary>
    /// holds the population-level parameters for population of females
    /// </summary>
	Female::SubPopParams defaultFemaleParams;

    /// <summary>
    /// holds the population-level (default) prep parameters
    /// </summary>
	PrepParameters defaultPrepParams;

    /// <summary>
	/// this is a quick way to check whether a partnership is technically a fling or not
	/// right now, behavior for males is the only one that has been coded
    /// </summary>
    bool partnershipsHaveDuration[(std::size_t)DemographicProfile::Gender::Last][(std::size_t)SexualPartnership::Type::Last]{};

    /// <summary>
    /// cost per condom in dollars
    /// </summary>
	double condomCost{};

    /// <summary>
    /// cost per circumcision in dollars
    /// </summary>
	double circumcisionCost{};

    double prEPCost{};

    double vaginalMicrobicideApplicationCost{};
};

} // namespace transm
