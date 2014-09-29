#pragma once

#include <fstream>
#include <unordered_map>
#include <vector>

#include "ArtRolloutTracker.hpp"
#include "CostsTracker.hpp"
#include "InfectionsTracker.hpp"
#include "StatsRecord.hpp"
#include "data/Enum.hpp"
#include "entities/SexualPartnership.hpp"

class Person;

/// <summary>
/// This class contains population level statistics
/// </summary>
class PopulationStatistics
{
	// Note: If we change things here, make sure to to change the LifeStatsStr.
public:
	enum InitHIVStatus
	{
		PREVALENT,
		NON_PREVALENT,
		ENDInitHIVStatus
	};

	enum LifeStats
	{
		TOTAL_LM,							//total life months of the population
		TOTAL_HIV_NEG_LM,							//total life months of all HIV-
		TOTAL_HIV_NEG_DTHS,						//total number of deaths in HIV
		TOTAL_HIV_NEG,						//total number of HIV negative persons (could be greater than number of deaths as it includes those who died after time ended)
		TOTAL_HIV_POS_LM,					//total life months of all HIV+
		TOTAL_HIV_POS_POSTINFECT_LM,				//total life months of the HIV+ after point of infection
		TOTAL_HIV_POS_DTHS,						//total deaths emong HIV+
		TOTAL_HIV_POS,						//Denominator of HIV positive persons (will be greater than number of deaths as it includes those who died after time ended)
		ENDLifeStats
	};

	static const std::vector<std::string> LifeStatsStr;


	struct SingleTimeStats
	{
		long timeOfStats;
		double prevalence;
		double SAprevalence; 	//Prevalence of sexually active population only
		double incidence; 		//TODO: Incidence will be an average over the 12 months leading up to the given time point
		long cumulativeNumberDead;	//Total number of persons who have died since time 0
	};

	class SingleLEStats
	{
	public:
		SingleLEStats();
		long deathsByAge[Person::maxYrForDeathStats];//number of deaths in that time period by age
		long popByAge[Person::maxYrForDeathStats]; //number of people in that age bucket
	};

	class SinglePartAcqStats
	{
	public:
        //Number of partner bins to store for freq plot
		static const int NUM_PARTNER_BINS = 16;

		SinglePartAcqStats();

        //Number of people with specified number of partners
		long partnerFreq[NUM_PARTNER_BINS];
	};

	class SurvivalStats
	{
	public:
		SurvivalStats();

        //sum of time to death for people who die during model run (used to calculate mean) strat by gender
		unsigned long timeToDeathGenderSum[(std::size_t)DemographicProfile::Gender::Last];
        //sum square of time to death strat by gender (used to calculate SD)
		unsigned long timeToDeathGenderSumSquare[(std::size_t)DemographicProfile::Gender::Last];
        //number who died strat by gender
        unsigned int numDeathGender[(std::size_t)DemographicProfile::Gender::Last];

        //time to death stratified by CSW status and Risk level
		unsigned long timeToDeathEmplRiskSum[(std::size_t)DemographicProfile::Employment::Last][Person::ENDRiskLevel];
        unsigned long timeToDeathEmplRiskSumSquare[(std::size_t)DemographicProfile::Employment::Last][Person::ENDRiskLevel];
        unsigned int numDeathEmplRisk[(std::size_t)DemographicProfile::Employment::Last][Person::ENDRiskLevel];

        //time to death stratified by initial HIV status
		unsigned long timeToDeathHIVStatusSum[ENDInitHIVStatus];
		unsigned long timeToDeathHIVStatusSumSquare[ENDInitHIVStatus];
		unsigned int numDeathHIVStatus[ENDInitHIVStatus];

        //sum of time to infection or death for people who die during model run (used to calculate mean) strat by gender
		unsigned long timeToInfOrDeathGenderSum[(std::size_t)DemographicProfile::Gender::Last];
        //sum square of time to infection or death strat by gender (used to calculate SD)
		unsigned long timeToInfOrDeathGenderSumSquare[(std::size_t)DemographicProfile::Gender::Last];
        //number who got infected or died (not counting initial HIV+ prevalent cases)
		unsigned int numInfOrDeathGender[(std::size_t)DemographicProfile::Gender::Last];

        //time to inf or death stratified by CSW status and Risk level
		unsigned long timeToInfOrDeathEmplRiskSum[(std::size_t)DemographicProfile::Employment::Last][Person::ENDRiskLevel];
        unsigned long timeToInfOrDeathEmplRiskSumSquare[(std::size_t)DemographicProfile::Employment::Last][Person::ENDRiskLevel];
        unsigned int numInfOrDeathEmplRisk[(std::size_t)DemographicProfile::Employment::Last][Person::ENDRiskLevel];

        //sum of time from infection to death for people who die during model run (used to calculate mean) strat by gender
		unsigned long timeFromInfToDeathGenderSum[(std::size_t)DemographicProfile::Gender::Last]; 
        //sum square of time from infection to death strat by gender (used to calculate SD)
		unsigned long timeFromInfToDeathGenderSumSquare[(std::size_t)DemographicProfile::Gender::Last];
        //number who died while infected (counting prevalent HIV+)
        unsigned int numInfDeathGender[(std::size_t)DemographicProfile::Gender::Last];

        //time from inf to death stratified by CSW status and Risk level
		unsigned long timeFromInfToDeathEmplRiskSum[(std::size_t)DemographicProfile::Employment::Last][Person::ENDRiskLevel]; 
        unsigned long timeFromInfToDeathEmplRiskSumSquare[(std::size_t)DemographicProfile::Employment::Last][Person::ENDRiskLevel];
        unsigned int numInfDeathEmplRisk[(std::size_t)DemographicProfile::Employment::Last][Person::ENDRiskLevel];

	};
private:
	std::vector<int> timesToRecord;

	std::vector<int> timesToRecordLE;
	double medianLECI;

	std::vector<int> timesToRecordPartAcq;
	bool printHeaderPartAcq;

	std::vector<SingleTimeStats *> selectedSummaryStats;

	int monthOf1990;
	bool calculateShiftedOutcomes;
	int relativeYear;
	int yearStartPrevalentInfections;
	int yearStartSexuallyActivePopSize;
	int yearlyCumulativeSexuallyActivePopSize;
	int yearlyIncidentInfections;
	int yearlyTests;
	std::set<Person *> uniqueYearlyEligibleForTreatmentAccess;
	std::set<Person *> uniqueYearlyAccessingTreatment;
	std::set<Person *> uniqueYearlyEligibleForTreatment;
	std::set<Person *> uniqueYearlyTreated;
    std::vector<int> yearlyTestsByResult;

public:

	//----------< End type declarations >-------------------//
	SingleLEStats *selectedLEStats;
	SinglePartAcqStats *selectedPartAcqStats;
	SurvivalStats *survivalStats;

	InfectionsTracker infectionsTracker;	//tallies infections and generates statistics

	CostsTracker costsTracker;			//tallies all costs

	ArtRolloutTracker artTracker; // records art rollout outcomes

	EnumCls<PopulationStatistics::LifeStats> *enumClass; //used in lifeStats; declared here so that deletion is possible

	StatsRecord<LifeStats, BaseEnumCls::nullptr_ENUM> *lifeStats;

	PopulationStatistics();
	~PopulationStatistics();

	//processes a person's death
	void processDeath(Person *_p, EventParams &_eventParams);

	//processes a death that occurs after maxTime (for average life expectancy stats)
	void processPostMaxTimeDeath(Person *_p);

	void printLMStats(std::ostream &_outStream);

	void printLEStats(std::ostream &_outStream, long currTime);

	void printPartAcqStats(std::ostream &_outStream, long currTime);

	void printSurvivalStats(std::ostream &_outStream);

	void printShiftedOutcomes(std::ostream &_outStream, int currTime);

	//records an incident infection (calls InfectionTracker's method)
	void recordIncidentInfection(EventParams &_eventParams, long _time, SexualPartnership::Type _partnershipType,
	                             const Person *_infector, const Person *_infected, bool _print, ostream &_traceOutStream);

	//returns the next time greater than or equal to currTime in the list
	long getNextTimeToRecord(long currTime);

	//returns true if currTime is in timeToRecord[NUM_TIMES_TO_RECORD]
	bool isTimeToRecord(long currTime);

	//returns true if currTime is in timeToRecordLE
	bool isTimeToRecordLE(long currTime);
	bool isFirstMonthToRecordLE(long currTime);
	bool isTimeToPrintLE(long currTime);

	//returns true if currTime is in timeToRecordPartAcq
	bool isTimeToRecordPartAcq(long currTime);

	void recordPrevalenceAndIncidence(long currTime, double _prevalence, double _SAprevalence, double _incidence,
	                                  int saPopSize, int monthlyIncident, int monthlyPrevalent);

	void enableShiftedOutcomes(int monthOf1990);
	void resetYear(int newYear);
	void recordYearStartStats(int sexuallyActivePopSize, int prevalentCases);
	void recordTestStats(int numTests, const std::vector<int> &numTestsByResult);
	void recordTreatmentAccessEligiblity(Person *person);
	void recordTreatmentAccess(Person *person);
	void recordTreatmentEligiblity(Person *person);
	void recordTreatment(Person *person);

	void addLifeExpectancyRecordTime(int time) { timesToRecordLE.push_back(time); }
    void setMedianLECI(double ci) { medianLECI = ci; }

	std::vector<PopulationStatistics::SingleTimeStats *> *getSelectedSummaryStats();
};
