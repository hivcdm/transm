#pragma once

#include "statistics/CostsTracker.h"

class Params
{
	friend class Population;

	void loadXML(ticpp::Element *_populationXML, EventParams &_eventParams);
	void reloadXML(ticpp::Element *_populationXML, EventParams &_eventParams);

	//this data structure contains prevalence parameters differ in value by age buckets
	class AgeBucketPrevalenceInfo
	{
	public:
		int minAgeMth;			//the min age that this bucket represents
		int maxAgeMth;			//the max age that this bucket represents

		double proportionOfPopulation[DmgProfile::ENDGender]; //determines size as proportion of the population
		double chanceCSW[DmgProfile::ENDGender]; //determines chance of being csw on model initialization
		double numInfectedCSW[DmgProfile::ENDGender];		//number of males and female csw in this bucket that are infected (at prevalence delay)
		double numInfectedRisk[DmgProfile::ENDGender][Person::ENDRiskLevel]; //number of male and female non-csw in this bucket that are infected (at prevalence delay)

		inline AgeBucketPrevalenceInfo();

		AgeBucketPrevalenceInfo(int _minAgeMth, int _maxAgeMth,
		                        double _proportionOfPopulationMale,
		                        double _proportionOfPopulationFemale,
		                        double _numInfectedCSWMale,
		                        double _numInfectedCSWFemale,
		                        double _numInfectedNonCSWMalesLowRisk,
		                        double _numInfectedNonCSWFemalesLowRisk,
		                        double _numInfectedNonCSWMalesHighRisk,
		                        double _numInfectedNonCSWFemalesHighRisk);
		void print(EventParams &_eventParams);
		inline void copyToSelf(AgeBucketPrevalenceInfo _abpInfo);

		//private:
		//Calculated statistics
		//int numberInfected[DmgProfile::ENDGender][Person::ENDRiskLevel]; //Calculation involves knowing population size and
	};

	//this will be set as the Sim::eventParams.debugLevel
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

	double assort[SexualPartnership::ENDType]; //assortativeness parameter one for each partnership type

	unsigned int populationID;	//the ID of the population that these parameters correspond to

	//initial stats -- determines the prevalence of a demographic before the simulation starts
	double initproportionMarried;
	//determines percentage of people in regular relationships at start
	double initproportionRegular;
	//initial proportion of pop as CSW
	double initProbCSW[DmgProfile::ENDGender];
	//max age of csw in months
	int CSWEndAgeMth[DmgProfile::ENDGender];

	//prevalence parameters stratified by age.
	std::vector<AgeBucketPrevalenceInfo *> initialAgeBuckets;

	//holds the population-level parameters for population of males and the population of females
	const Male::SubPopParams *maleParams;
	const Female::SubPopParams *femaleParams;
	//this is a quick way to check whether a partnership is technically a fling or not
	// right now, behavior for males is the only one that has been coded
	bool partnershipsHaveDuration[DmgProfile::ENDGender][SexualPartnership::ENDType];

	//Costs
	double costs[CostsTracker::EndCostSources];

	Params();
	~Params();

	void init(ticpp::Element *_populationXML, unsigned int _populationID, EventParams &_eventParams);

public:
	double getBirthRate();
};
