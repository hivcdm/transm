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
        double proportionOfPopulation[DemographicProfile::ENDGender];

        /// <summary>
        /// number of males and female csw in this bucket that are infected (at prevalence delay)
        /// </summary>
        int numInfectedCSW[DemographicProfile::ENDGender];

        /// <summary>
        /// number of male and female non-csw in this bucket that are infected (at prevalence delay)
        /// </summary>
        int numInfectedRisk[DemographicProfile::ENDGender][Person::ENDRiskLevel];
	};

	PopulationParameters();
	~PopulationParameters();

    void setAgeSexualDebut(int ageSexualDebut, TimeGranularity granularity = TimeGranularity::Year) { SAEntAgeMths = Utility::convertTime(granularity, TimeGranularity::Month, ageSexualDebut); }

	double getBirthRate() const;
	void setBirthRate(double birth_rate) { birthRate = birth_rate; }

	double getProportionCircumcised() const { return circumcised; }
	void setProportionCircumcised(double value) { circumcised = value; }

	double getProportionMale() const;
	void setProportionMale(double proportion_male) { proportionMale = proportion_male; }

	int getAgeSexualDebut() const { return SAEntAgeMths; }

	void setAssortativeness(SexualPartnership::Type type, double assortativeness)
	{
		assort[(int)type] = assortativeness;
	}

	void SetPartnershipHasDuration(DemographicProfile::Gender gender, SexualPartnership::Type type, bool has_duration) { partnershipsHaveDuration[gender][(int)type] = has_duration; }

	const Male::SubPopParams &GetMaleParameters() const { return defaultMaleParams; }
	void SetMaleParameters(Male::SubPopParams &params) { defaultMaleParams = params; }
	const Female::SubPopParams &GetFemaleParameters() const { return defaultFemaleParams; }
	void SetFemaleParameters(Female::SubPopParams &params) { defaultFemaleParams = params; }

	void SetTransmissionCoefficient(DemographicProfile::Gender gender, Person::HVLStrata stratum, double coefficient)
	{
		switch(gender)
		{
		case DemographicProfile::MALE: defaultMaleParams.setTransmitPerEventCoeff(stratum, coefficient);
		case DemographicProfile::FEMALE: defaultFemaleParams.setTransmitPerEventCoeff(stratum, coefficient);
		default: throw std::runtime_error("bad gender");
		}
	}

	std::array<double, (int)SexualPartnership::Type::ENDType> GetAssortativeness() const 
	{ 
		std::array<double, (int)SexualPartnership::Type::ENDType> copy;
		std::copy(assort, assort + (int)SexualPartnership::Type::ENDType, copy.begin());
		return copy;
	}

	int GetInitialSize() const { return initSize; }
	void SetInitialSize(int size) { initSize = size; }

	double GetMaleProportion() const { return proportionMale; }

	const std::vector<AgeBucketPrevalenceInfo> &GetInitialAgeBuckets() const { return initialAgeBuckets; }
	std::vector<AgeBucketPrevalenceInfo> &GetInitialAgeBuckets() { return initialAgeBuckets; }

	void SetInitialCswProportion(DemographicProfile::Gender gender, double proportion) { initProbCSW[gender] = proportion; }
	void SetCswEndAge(DemographicProfile::Gender gender, int age_months) { CSWEndAgeMth[gender] = age_months; }

	void SetChanceBecomeCsw(DemographicProfile::Gender gender, double chance)
	{
		switch(gender)
		{
		case DemographicProfile::MALE: defaultMaleParams.setChanceBecomeCsw(chance);
		case DemographicProfile::FEMALE: defaultFemaleParams.setChanceBecomeCsw(chance);
		default: throw std::runtime_error("bad gender");
		}
	}

	void SetProportionHighRisk(DemographicProfile::Gender gender, DemographicProfile::Employment employment, double proportion)
	{
		switch(gender)
		{
		case DemographicProfile::MALE: defaultMaleParams.setProportionHighRisk(employment, proportion);
		case DemographicProfile::FEMALE: defaultFemaleParams.setProportionHighRisk(employment, proportion);
		default: throw std::runtime_error("bad gender");
		}
	}

	void setAverageYearsYounger(SexualPartnership::Type type, NormalDist dist) { defaultMaleParams.setAverageYearsYounger(type, dist); }
	void setAcquisitionRatePerMonth(Person::RiskLevel risk, SexualPartnership::Type type, LogNormalDist dist) { defaultMaleParams.setAcquisitionRatePerMonth(risk, type, dist); }
	void setCoitalEventsPerMonth(Person::RiskLevel risk, SexualPartnership::Type type, double mean) { defaultMaleParams.setCoitalEventsPerMonth(risk, type, mean); }
	void setChanceCondomUsePerEvent(Person::RiskLevel risk, SexualPartnership::Type type, BetaDist dist) { defaultMaleParams.setChanceCondomUsePerEvent(risk, type, dist); }
	void setPartnershipDuration(Person::RiskLevel risk, SexualPartnership::Type type, ShiftedLogNormalDist dist) { defaultMaleParams.setPartnershipDuration(risk, type, dist); }

	void SetPartnerAcquisitionSteadyMultiplier(Person::RiskLevel risk, double multiplier) { defaultMaleParams.setPartnerAcqMultWithSteady(risk, multiplier); }

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
	int SAEntAgeMths;
	double proportionMale;
	double circumcised;

    /// <summary>
    /// assortativeness parameter one for each partnership type
    /// </summary>
	double assort[(int)SexualPartnership::Type::ENDType];

    /// <summary>
    /// initial proportion of pop as CSW
    /// </summary>
	double initProbCSW[DemographicProfile::ENDGender];

    /// <summary>
    /// max age of csw in months
    /// </summary>
	int CSWEndAgeMth[DemographicProfile::ENDGender];

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
	bool partnershipsHaveDuration[DemographicProfile::ENDGender][(int)SexualPartnership::Type::ENDType];

    /// <summary>
    /// cost per condom in dollars
    /// </summary>
	double condomCost;

    /// <summary>
    /// cost per circumcision in dollars
    /// </summary>
	double circumcisionCost;
};
