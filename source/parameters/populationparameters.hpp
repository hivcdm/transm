#pragma once

#include "entities/msmw.hpp"
#include "entities/female.hpp"
#include "entities/male.hpp"
#include "entities/msm.hpp"
#include "entities/transmissiontype.hpp"
#include "statistics/coststracker.hpp"
#include "parameters/agebucketprevalenceinfo.hpp"
#include "utility/time.hpp"

namespace transm {

class PopulationParameters
{
public:
	struct PopulationTarget
	{
		template<typename T>
		using Optional = std::pair<bool, T>;

		Optional<DemographicProfile> profile;
		Optional<int> min_age;
		Optional<int> max_age;
		Optional<Entity::RiskLevel> risk;

		bool match(const Entity *e)
		{
			if (profile.first && !e->getDemographicProfile()->match(profile.second))
			{
				return false;
			}

			if (min_age.first && e->getAge() < Age::from_months(min_age.second))
			{
				return false;
			}

			if (max_age.first && e->getAge() > Age::from_months(max_age.second))
			{
				return false;
			}

			if (risk.first && e->getRiskLevel() != risk.second)
			{
				return false;
			}

			return true;
		}
	};

	PopulationParameters();
	~PopulationParameters();

	double GetBirthRate() const;
	void SetBirthRate(double birth_rate) { birthRate = birth_rate; }

	double GetProportionCircumcised() const { return proportionCircumcised; }
	void SetProportionCircumcised(double value) { proportionCircumcised = value; }

    double GetBirthProportion(const std::string &entity_type) const { return birthProportions.at(entity_type); }
    void SetBirthProportion(const std::string &entity_type, double proportion) { birthProportions[entity_type] = proportion; }

	Age GetAgeOfMajority() const { return ageOfMajority; }
    void SetAgeOfMajority(Age age) 
    { 
		ageOfMajority = age;
    }

    void SetPartnershipHasDuration(DemographicProfile::Gender gender, SexualPartnership::Type type, bool has_duration) 
    { 
        partnershipsHaveDuration[(std::size_t)gender][(std::size_t)type] = has_duration;
    }

	const Male::SubPopParams &GetMaleParameters() const { return defaultMaleParams; }
	void SetMaleParameters(Male::SubPopParams &params) { defaultMaleParams = params; }
    void SetMsmParameters(Msm::SubPopParams &params) { defaultMsmParams = params; }
    void SetBiMaleParameters(Msmw::SubPopParams &params) { defaultMsmwParams = params; }
	const Female::SubPopParams &GetFemaleParameters() const { return defaultFemaleParams; }
	void SetFemaleParameters(Female::SubPopParams &params) { defaultFemaleParams = params; }

	void SetTransmissionCoefficients(const std::unordered_map<TransmissionType, std::array<double, (std::size_t)Entity::HVLStrata::Last>> &coefficients)
	{
        transmission_coefficients_ = coefficients;
	}

    void SetSexualActivityDelay(TimeSpan delay) { sexualActivityDelay = delay; }
    TimeSpan GetSexualActivityDelay() const { return sexualActivityDelay; }

    void SetAssortativeness(SexualPartnership::Type partnership_type, double assortativeness) { defaultMaleParams.getSexualBehavior(partnership_type).setAssortativeness(assortativeness); }

	int GetInitialSize() const { return initSize; }
	void SetInitialSize(int size) { initSize = size; }

	double GetChanceChronicInfection() const {return chronicInfectionRate; }
	void SetChanceChronicInfection(double rate) { chronicInfectionRate = rate; }

	const std::vector<AgeBucketPrevalenceInfo> &GetInitialAgeBuckets() const { return initialAgeBuckets; }
	std::vector<AgeBucketPrevalenceInfo> &GetInitialAgeBuckets() { return initialAgeBuckets; }

	void SetSeedDelay(int delay) { seedDelay = Time().from_months(delay); }
	Time GetSeedDelay() { return seedDelay; }
	void SetSeedPrevalence(double prev) { seedPrevalence = prev; }
	double GetSeedPrevalence() { return seedPrevalence; }
	void SetUseSeedCoefficients(bool useCoeffs) { useSeedCoefficients = useCoeffs; }
	bool UseSeedCoefficients() { return useSeedCoefficients; }
	void SetMinSeedAge(int age) { minSeedAge = age; }
	int GetMinSeedAge() { return minSeedAge; }
	void SetMaxSeedAge(int age) { maxSeedAge = age; }
	int GetMaxSeedAge() { return maxSeedAge; }

	void SetInitialCswProportion(const std::string &entity_type, double proportion) { initProbCSW[entity_type] = proportion; }
    void SetCswEndAge(const std::string &entity_type, Age age) { CSWEndAge[entity_type] = age; }

	void SetChanceBecomeCsw(DemographicProfile::Gender gender, double chance)
	{
		switch(gender)
		{
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
		switch(gender)
		{
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
	void SetAcquisitionRatePerMonth(Entity::RiskLevel risk, SexualPartnership::Type type, LogNormalDist dist) { defaultMaleParams.SetAcquisitionRatePerMonth(risk, type, dist); }
	void SetCoitalEventsPerMonth(Entity::RiskLevel risk, SexualPartnership::Type type, double mean) { defaultMaleParams.SetCoitalEventsPerMonth(risk, type, mean); }
	void SetChanceCondomUsePerEvent(Entity::RiskLevel risk, SexualPartnership::Type type, BetaDist dist) { defaultMaleParams.SetChanceCondomUsePerEvent(risk, type, dist); }
	void SetPartnershipDuration(Entity::RiskLevel risk, SexualPartnership::Type type, ShiftedLogNormalDist dist) { defaultMaleParams.SetPartnershipDuration(risk, type, dist); }

	void SetPartnerAcquisitionSteadyMultiplier(Entity::RiskLevel risk, double multiplier) { defaultMaleParams.SetPartnerAcqMultWithSteady(risk, multiplier); }

    void SetCondomCost(double condom_cost) { condomCost = condom_cost; }
    void SetCircumcisionCost(double circumcision_cost) { circumcisionCost = circumcision_cost; }
    void SetPrEPCost(double cost) { prEPCost = cost; }
    void SetVaginalMicrobicideCost(double cost) { vaginalMicrobicideApplicationCost = cost; }

protected:
	friend class Population;

private:
	long initSize;

	double chronicInfectionRate;

    /// <summary>
	/// Per month per person based on WHI data
    /// </summary>
	double birthRate;

    /// <summary>
    /// Age that entities roll to change risk.
    /// </summary>
	Age ageOfMajority;

    TimeSpan sexualActivityDelay;

    std::unordered_map<std::string, double> birthProportions;

	double proportionCircumcised;

    /// <summary>
    /// initial proportion of pop as CSW
    /// </summary>
    std::unordered_map<std::string, double> initProbCSW;

    /// <summary>
    /// max age of csw in months
    /// </summary>
    std::unordered_map<std::string, Age> CSWEndAge;

    /// <summary>
    /// prevalence parameters stratified by age.
    /// </summary>
	std::vector<AgeBucketPrevalenceInfo> initialAgeBuckets;

    /// <summary>
    /// prevalence parameters stratified by age.
    /// </summary>
    Time seedDelay;
    double seedPrevalence;
    bool useSeedCoefficients;
    int minSeedAge;
    int maxSeedAge;

    /// <summary>
    /// Base FOI for different transmission types at various viral loads.
    /// </summary>
    std::unordered_map<TransmissionType, std::array<double, (std::size_t)Entity::HVLStrata::Last>> transmission_coefficients_;

    /// <summary>
    /// holds the population-level parameters for population of heterosexual males
    /// </summary>
	Male::SubPopParams defaultMaleParams;

    /// <summary>
    /// holds the population-level parameters for population of msms
    /// </summary>
    Msm::SubPopParams defaultMsmParams;

    /// <summary>
    /// holds the population-level parameters for population of msmws
    /// </summary>
    Msmw::SubPopParams defaultMsmwParams;

    /// <summary>
    /// holds the population-level parameters for population of females
    /// </summary>
	Female::SubPopParams defaultFemaleParams;

    /// <summary>
	/// this is a quick way to check whether a partnership is technically a fling or not
	/// right now, behavior for males is the only one that has been coded
    /// </summary>
    bool partnershipsHaveDuration[(std::size_t)DemographicProfile::Gender::Last][(std::size_t)SexualPartnership::Type::ENDType];

    /// <summary>
    /// cost per condom in dollars
    /// </summary>
	double condomCost;

    /// <summary>
    /// cost per circumcision in dollars
    /// </summary>
	double circumcisionCost;

    double prEPCost;

    double vaginalMicrobicideApplicationCost;
};

} // namespace transm
