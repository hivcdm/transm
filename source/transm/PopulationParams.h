#pragma once

#include "entities/Male.h"
#include "entities/Female.h"
#include "statistics/CostsTracker.h"

class PopulationParams
{
public:
	PopulationParams();
	~PopulationParams();

	void setAgeSexualDebut(int ageSexualDebutMonths) { SAEntAgeMths = ageSexualDebutMonths; }

	double getBirthRate() const;
	void setBirthRate(double birth_rate) { birthRate = birth_rate; }

	double getProportionCircumcised() const { return circumcised; }
	void setProportionCircumcised(double value) { circumcised = value; }

	double getProportionMale() const;
	void setProportionMale(double proportion_male) { proportionMale = proportion_male; }

protected:
	friend class Population;

	//this data structure contains prevalence parameters differ in value by age buckets
	class AgeBucketPrevalenceInfo
	{
	public:
		int minAgeMth;			//the min age that this bucket represents
		int maxAgeMth;			//the max age that this bucket represents

		double proportionOfPopulation[DmgProfile::ENDGender]; //determines size as proportion of the population
		std::array<double, DmgProfile::ENDGender> chanceCSW; //determines chance of being csw on model initialization
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
		inline void copyToSelf(AgeBucketPrevalenceInfo _abpInfo);
	};	

private:
	friend class SimulationBuilder;

	//this will be set as the Simulation::eventParams.debugLevel
	DebugLevel debugLevel;

	int maxTime;			//make timesteps to this simulation (in months)
	long initSize;
	string cepacInputFile;
	//Per month per person based on WHI data
	double birthRate;
	int SAEntAgeMths;		//age in months
	double proportionMale;
	double circumcised;
	double hivInfected;

	double assort[(int)SexualPartnership::Type::ENDType]; //assortativeness parameter one for each partnership type

	//initial stats -- determines the prevalence of a demographic before the simulation starts
	double initproportionMarried;
	//determines percentage of people in regular relationships at start
	double initproportionRegular;
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
