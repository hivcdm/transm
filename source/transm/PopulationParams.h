#pragma once

#include "entities/Male.h"
#include "entities/Female.h"
#include "statistics/CostsTracker.h"

class PopulationParams
{
public:
	//this data structure contains prevalence parameters differ in value by age buckets
	class AgeBucketPrevalenceInfo
	{
	public:
		int minAgeMth;			//the min age that this bucket represents
		int maxAgeMth;			//the max age that this bucket represents

		double proportionOfPopulation[DmgProfile::ENDGender]; //determines size as proportion of the population
		//std::array<double, DmgProfile::ENDGender> chanceCSW; //determines chance of being csw on model initialization
		int numInfectedCSW[DmgProfile::ENDGender];		//number of males and female csw in this bucket that are infected (at prevalence delay)
		int numInfectedRisk[DmgProfile::ENDGender][Person::ENDRiskLevel]; //number of male and female non-csw in this bucket that are infected (at prevalence delay)

		inline AgeBucketPrevalenceInfo();

		AgeBucketPrevalenceInfo(int _minAgeMth, int _maxAgeMth,
			double _proportionOfPopulationMale,
			double _proportionOfPopulationFemale,
			int _numInfectedCSWMale,
			int _numInfectedCSWFemale,
			int _numInfectedNonCSWMalesLowRisk,
			int _numInfectedNonCSWFemalesLowRisk,
			int _numInfectedNonCSWMalesHighRisk,
			int _numInfectedNonCSWFemalesHighRisk);
		void print(EventParams &_eventParams);
	};

	PopulationParams();
	~PopulationParams();

	void setAgeSexualDebut(int ageSexualDebut, TimeGranularity granularity = YEAR) { SAEntAgeMths = Util::convertTime(granularity, MONTH, ageSexualDebut); }

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

	void SetPartnershipHasDuration(DmgProfile::Gender gender, SexualPartnership::Type type, bool has_duration) { partnershipsHaveDuration[gender][(int)type] = has_duration; }

	const Male::SubPopParams &GetMaleParameters() const { return defaultMaleParams; }
	void SetMaleParameters(Male::SubPopParams &params) { defaultMaleParams = params; }
	const Female::SubPopParams &GetFemaleParameters() const { return defaultFemaleParams; }
	void SetFemaleParameters(Female::SubPopParams &params) { defaultFemaleParams = params; }

	void SetTransmissionCoefficient(DmgProfile::Gender gender, Person::HVLStrata stratum, double coefficient)
	{
		switch(gender)
		{
		case DmgProfile::MALE: defaultMaleParams.setTransmitPerEventCoeff(stratum, coefficient);
		case DmgProfile::FEMALE: defaultFemaleParams.setTransmitPerEventCoeff(stratum, coefficient);
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

	void SetInitialCswProportion(DmgProfile::Gender gender, double proportion) { initProbCSW[gender] = proportion; }
	void SetCswEndAge(DmgProfile::Gender gender, int age_months) { CSWEndAgeMth[gender] = age_months; }

	void SetChanceBecomeCsw(DmgProfile::Gender gender, double chance)
	{
		switch(gender)
		{
		case DmgProfile::MALE: defaultMaleParams.setChanceBecomeCsw(chance);
		case DmgProfile::FEMALE: defaultFemaleParams.setChanceBecomeCsw(chance);
		}
	}

	void SetProportionHighRisk(DmgProfile::Gender gender, DmgProfile::Employment employment, double proportion)
	{
		switch(gender)
		{
		case DmgProfile::MALE: defaultMaleParams.setProportionHighRisk(employment, proportion);
		case DmgProfile::FEMALE: defaultFemaleParams.setProportionHighRisk(employment, proportion);
		}
	}

	void setAverageYearsYounger(SexualPartnership::Type type, NormalDist dist) { defaultMaleParams.setAverageYearsYounger(type, dist); }
	void setAcquisitionRatePerMonth(Person::RiskLevel risk, SexualPartnership::Type type, LogNormalDist dist) { defaultMaleParams.setAcquisitionRatePerMonth(risk, type, dist); }
	void setCoitalEventsPerMonth(Person::RiskLevel risk, SexualPartnership::Type type, double mean) { defaultMaleParams.setCoitalEventsPerMonth(risk, type, mean); }
	void setChanceCondomUsePerEvent(Person::RiskLevel risk, SexualPartnership::Type type, BetaDist dist) { defaultMaleParams.setChanceCondomUsePerEvent(risk, type, dist); }
	void setPartnershipDuration(Person::RiskLevel risk, SexualPartnership::Type type, ShiftedLogNormalDist dist) { defaultMaleParams.setPartnershipDuration(risk, type, dist); }

protected:
	friend class Population;

private:
	friend class SimulationBuilder;

	//this will be set as the Simulation::eventParams.debugLevel
	DebugLevel debugLevel;

	//int maxTime;			//make timesteps to this simulation (in months)
	long initSize;
	//string cepacInputFile;
	//Per month per person based on WHI data
	double birthRate;
	int SAEntAgeMths;		//age in months
	double proportionMale;
	double circumcised;
	//double hivInfected;

	double assort[(int)SexualPartnership::Type::ENDType]; //assortativeness parameter one for each partnership type

	//initial proportion of pop as CSW
	double initProbCSW[DmgProfile::ENDGender];
	//max age of csw in months
	int CSWEndAgeMth[DmgProfile::ENDGender];

	//prevalence parameters stratified by age.
	std::vector<AgeBucketPrevalenceInfo> initialAgeBuckets;

	//holds the population-level parameters for population of males and the population of females
	Male::SubPopParams defaultMaleParams;
	Female::SubPopParams defaultFemaleParams;

	//this is a quick way to check whether a partnership is technically a fling or not
	// right now, behavior for males is the only one that has been coded
	bool partnershipsHaveDuration[DmgProfile::ENDGender][(int)SexualPartnership::Type::ENDType];

	//Costs
	double condomCost;
	double circumcisionCost;
};
