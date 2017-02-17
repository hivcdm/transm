#pragma once

#include "../entities/Male.h"
#include "../entities/Female.h"
#include "../statistics/CostsTracker.h"

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

		AgeBucketPrevalenceInfo(int _minAgeMth, int _maxAgeMth, double _proportionOfPopulationMale,
			double _proportionOfPopulationFemale, int _numInfectedCSWMale, int _numInfectedCSWFemale,
			int _numInfectedNonCSWMalesLowRisk, int _numInfectedNonCSWFemalesLowRisk,
			int _numInfectedNonCSWMalesHighRisk, int _numInfectedNonCSWFemalesHighRisk);

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
        double proportionOfPopulation[(std::size_t)DemographicProfile::Gender::Last];

        /// <summary>
        /// number of males and female csw in this bucket that are infected (at prevalence delay)
        /// </summary>
        int numInfectedCSW[(std::size_t)DemographicProfile::Gender::Last];

        /// <summary>
        /// number of male and female non-csw in this bucket that are infected (at prevalence delay)
        /// </summary>
        int numInfectedRisk[(std::size_t)DemographicProfile::Gender::Last][Person::ENDRiskLevel];
	};

	PopulationParameters();
	~PopulationParameters();

	double getBirthRate() const;
	void setBirthRate(double birth_rate) { birthRate = birth_rate; }

	double getProportionCircumcised() const { return proportionCircumcised; }
	void setProportionCircumcised(double value) { proportionCircumcised = value; }

	double getProportionMale() const;
	void setProportionMale(double proportion_male) { proportionMale = proportion_male; }

	int getAgeOfMajority() const { return ageOfMajority; }
    void setAgeOfMajority(int ageOfMajority, TimeGranularity granularity = TimeGranularity::Year) 
    { 
        this->ageOfMajority = Utility::convertTime(granularity, TimeGranularity::Month, ageOfMajority); 
    }

    void SetPartnershipHasDuration(DemographicProfile::Gender gender, SexualPartnership::Type type, bool has_duration) 
    { 
        partnershipsHaveDuration[(std::size_t)gender][(std::size_t)type] = has_duration;
    }

	const Male::SubPopParams &GetMaleParameters() const { return defaultMaleParams; }
	void SetMaleParameters(Male::SubPopParams &params) { defaultMaleParams = params; }
	const Female::SubPopParams &GetFemaleParameters() const { return defaultFemaleParams; }
	void SetFemaleParameters(Female::SubPopParams &params) { defaultFemaleParams = params; }

	void SetTransmissionCoefficient(DemographicProfile::Gender gender, Person::HVLStrata stratum, double coefficient)
	{
		switch(gender)
		{
		case DemographicProfile::Gender::Male: 
            defaultMaleParams.SetTransmitPerEventCoeff(stratum, coefficient);
            break;
		case DemographicProfile::Gender::Female: 
            defaultFemaleParams.SetTransmitPerEventCoeff(stratum, coefficient);
            break;
		default: 
            throw std::runtime_error("bad gender");
		}
	}

    void SetSexualActivityDelay(int delay) { sexualActivityDelay = delay; }
    int GetSexualActivityDelay() const { return sexualActivityDelay; }

    void SetAssortativeness(SexualPartnership::Type partnership_type, double assortativeness) { defaultMaleParams.getSexualBehavior(partnership_type).setAssortativeness(assortativeness); }

	int GetInitialSize() const { return initSize; }
	void SetInitialSize(int size) { initSize = size; }

	double GetChanceChronicInfection() const {return chronicInfectionRate; }
	void SetChanceChronicInfection(double rate) { chronicInfectionRate = rate; }

	double GetMaleProportion() const { return proportionMale; }

	const std::vector<AgeBucketPrevalenceInfo> &GetInitialAgeBuckets() const { return initialAgeBuckets; }
	std::vector<AgeBucketPrevalenceInfo> &GetInitialAgeBuckets() { return initialAgeBuckets; }

    void SetInitialCswProportion(DemographicProfile::Gender gender, double proportion) { initProbCSW[(std::size_t)gender] = proportion; }
    void SetCswEndAge(DemographicProfile::Gender gender, int age_months) { CSWEndAgeMth[(std::size_t)gender] = age_months; }

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
    void SetPrEPCost(double cost) { prEPCost = cost; }
    void SetVaginalMicrobicideCost(double cost) { vaginalMicrobicideApplicationCost = cost; }

protected:
	friend class Population;

private:
	friend class SimulationBuilder;

    /// <summary>
	/// this will be set as the Simulation::eventParams.debugLevel
    /// </summary>
	DebugLevel debugLevel;

	long initSize;

	double chronicInfectionRate;

    /// <summary>
	/// Per month per person based on WHI data
    /// </summary>
	double birthRate;

    /// <summary>
    /// age in months
    /// </summary>
	int ageOfMajority;

    int sexualActivityDelay;

	double proportionMale;
	double proportionCircumcised;

    /// <summary>
    /// initial proportion of pop as CSW
    /// </summary>
    double initProbCSW[(std::size_t)DemographicProfile::Gender::Last];

    /// <summary>
    /// max age of csw in months
    /// </summary>
    int CSWEndAgeMth[(std::size_t)DemographicProfile::Gender::Last];

    /// <summary>
    /// prevalence parameters stratified by age.
    /// </summary>
	std::vector<AgeBucketPrevalenceInfo> initialAgeBuckets;

    /// <summary>
    /// holds the population-level parameters for population of males
    /// </summary>
	Male::SubPopParams defaultMaleParams;

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
