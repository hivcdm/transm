#pragma once

#include "entities/bisexualmale.hpp"
#include "entities/female.hpp"
#include "entities/male.hpp"
#include "entities/msm.hpp"
#include "entities/transmissiontype.hpp"
#include "statistics/coststracker.hpp"

class PopulationParameters
{
public:
    /// <summary>
	/// this data structure contains prevalence parameters differ in value by age buckets
    /// </summary>
	class AgeBucketPrevalenceInfo
	{
	public:
		AgeBucketPrevalenceInfo();

        AgeBucketPrevalenceInfo(int _minAgeMth, int _maxAgeMth, 
            const std::unordered_map<std::string, double> &entity_proportions, 
            std::size_t _numInfectedCSWMale, std::size_t _numInfectedCSWFemale,
			std::size_t _numInfectedNonCSWMalesLowRisk, std::size_t _numInfectedNonCSWFemalesLowRisk,
			std::size_t _numInfectedNonCSWMalesHighRisk, std::size_t _numInfectedNonCSWFemalesHighRisk);

		void print(EventParams &_eventParams);

        /// <summary>
        /// the min age that this bucket represents
        /// </summary>
        int minAgeMth;

        /// <summary>
        /// the max age that this bucket represents
        /// </summary>
        int maxAgeMth;

        /// <summary>
        /// determines size as proportion of the population
        /// </summary>
        std::unordered_map<std::string, double> entityProportions;

        /// <summary>
        /// number of males and female csw in this bucket that are infected (at prevalence delay)
        /// </summary>
        std::size_t numInfectedCSW[(std::size_t)DemographicProfile::Gender::Last];

        /// <summary>
        /// number of male and female non-csw in this bucket that are infected (at prevalence delay)
        /// </summary>
        std::size_t numInfectedRisk[(std::size_t)DemographicProfile::Gender::Last][Person::ENDRiskLevel];
	};

	PopulationParameters();
	~PopulationParameters();

	double getBirthRate() const;
	void setBirthRate(double birth_rate) { birthRate = birth_rate; }

	double getProportionCircumcised() const { return proportionCircumcised; }
	void setProportionCircumcised(double value) { proportionCircumcised = value; }

    double getBirthProportion(const std::string &entity_type) const { return birthProportions.at(entity_type); }
    void setBirthProportion(const std::string &entity_type, double proportion) { birthProportions[entity_type] = proportion; }

	int getAgeOfMajority() const { return ageOfMajority; }
    void setAgeOfMajority(int ageOfMajority, TimeGranularity granularity = TimeGranularity::Year) 
    { 
        this->ageOfMajority = Utility::convert_time(granularity, TimeGranularity::Month, ageOfMajority); 
    }

    void SetPartnershipHasDuration(DemographicProfile::Gender gender, SexualPartnership::Type type, bool has_duration) 
    { 
        partnershipsHaveDuration[(std::size_t)gender][(std::size_t)type] = has_duration;
    }

	const Male::SubPopParams &GetMaleParameters() const { return defaultMaleParams; }
	void SetMaleParameters(Male::SubPopParams &params) { defaultMaleParams = params; }
    void SetMsmParameters(Msm::SubPopParams &params) { defaultMsmParams = params; }
    void SetBiMaleParameters(BisexualMale::SubPopParams &params) { defaultBisexualMaleParams = params; }
	const Female::SubPopParams &GetFemaleParameters() const { return defaultFemaleParams; }
	void SetFemaleParameters(Female::SubPopParams &params) { defaultFemaleParams = params; }

	void SetTransmissionCoefficients(const std::unordered_map<TransmissionType, std::array<double, Person::ENDHVLStrata>> &coefficients)
	{
        transmission_coefficients_ = coefficients;
	}

    void SetSexualActivityDelay(int delay) { sexualActivityDelay = delay; }
    int GetSexualActivityDelay() const { return sexualActivityDelay; }

    void SetAssortativeness(SexualPartnership::Type partnership_type, double assortativeness) { defaultMaleParams.getSexualBehavior(partnership_type).setAssortativeness(assortativeness); }

	int GetInitialSize() const { return initSize; }
	void SetInitialSize(int size) { initSize = size; }

	const std::vector<AgeBucketPrevalenceInfo> &GetInitialAgeBuckets() const { return initialAgeBuckets; }
	std::vector<AgeBucketPrevalenceInfo> &GetInitialAgeBuckets() { return initialAgeBuckets; }

    void SetInitialCswProportion(const std::string &entity_type, double proportion) { initProbCSW[entity_type] = proportion; }
    void SetCswEndAge(const std::string &entity_type, int age_months) { CSWEndAgeMth[entity_type] = age_months; }

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
	void SetAcquisitionRatePerMonth(Person::RiskLevel risk, SexualPartnership::Type type, LogNormalDist dist) { defaultMaleParams.SetAcquisitionRatePerMonth(risk, type, dist); }
	void SetCoitalEventsPerMonth(Person::RiskLevel risk, SexualPartnership::Type type, double mean) { defaultMaleParams.SetCoitalEventsPerMonth(risk, type, mean); }
	void SetChanceCondomUsePerEvent(Person::RiskLevel risk, SexualPartnership::Type type, BetaDist dist) { defaultMaleParams.SetChanceCondomUsePerEvent(risk, type, dist); }
	void SetPartnershipDuration(Person::RiskLevel risk, SexualPartnership::Type type, ShiftedLogNormalDist dist) { defaultMaleParams.SetPartnershipDuration(risk, type, dist); }

	void SetPartnerAcquisitionSteadyMultiplier(Person::RiskLevel risk, double multiplier) { defaultMaleParams.SetPartnerAcqMultWithSteady(risk, multiplier); }

    void SetCondomCost(double condom_cost) { condomCost = condom_cost; }
    void SetCircumcisionCost(double circumcision_cost) { circumcisionCost = circumcision_cost; }

protected:
	friend class Population;

private:
	friend class SimulationBuilder;

    /// <summary>
	/// this will be set as the Simulation::eventParams.debugLevel
    /// </summary>
	DebugLevel debugLevel;

	long initSize;

    /// <summary>
	/// Per month per person based on WHI data
    /// </summary>
	double birthRate;

    /// <summary>
    /// age in months
    /// </summary>
	int ageOfMajority;

    int sexualActivityDelay;

    std::unordered_map<std::string, double> birthProportions;

	double proportionCircumcised;

    /// <summary>
    /// initial proportion of pop as CSW
    /// </summary>
    std::unordered_map<std::string, double> initProbCSW;

    /// <summary>
    /// max age of csw in months
    /// </summary>
    std::unordered_map<std::string, int> CSWEndAgeMth;

    /// <summary>
    /// prevalence parameters stratified by age.
    /// </summary>
	std::vector<AgeBucketPrevalenceInfo> initialAgeBuckets;

    /// <summary>
    /// Base FOI for different transmission types at various viral loads.
    /// </summary>
    std::unordered_map<TransmissionType, std::array<double, Person::ENDHVLStrata>> transmission_coefficients_;

    /// <summary>
    /// holds the population-level parameters for population of heterosexual males
    /// </summary>
	Male::SubPopParams defaultMaleParams;

    /// <summary>
    /// holds the population-level parameters for population of msms
    /// </summary>
    Msm::SubPopParams defaultMsmParams;

    /// <summary>
    /// holds the population-level parameters for population of bisexual males
    /// </summary>
    BisexualMale::SubPopParams defaultBisexualMaleParams;

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
};
