#pragma once

#include <fstream>
#include <unordered_map>
#include <vector>
#include <ticpp/ticpp.h>

#include "ArtRolloutTracker.h"
#include "CostsTracker.h"
#include "InfectionsTracker.h"
#include "StatsRecord.h"
#include "../data/Enum.h"
#include "../entities/classifiers/SexualPartnership.h"

class Person;

/**
This class contains population level statistics
**/
class PopStats
{
	//----------< Begin type declarations >-------------------//
	//if we change things here, make sure to to change the LifeStatsStr
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
		int timeOfStats;
		double prevalence;
		double SAprevalence; 	//Prevalence of sexually active population only
		double incidence; 		//TODO: Incidence will be an average over the 12 months leading up to the given time point
		int cumulativeNumberDead;	//Total number of persons who have died since time 0
	};

	class SingleLEStats
	{
	public:
		SingleLEStats();
		int deathsByAge[Person::maxYrForDeathStats];//number of deaths in that time period by age
		int popByAge[Person::maxYrForDeathStats]; //number of people in that age bucket
	};

	class SinglePartAcqStats
	{
	public:
		static const int NUM_PARTNER_BINS = 16; //Number of partner bins to store for freq plot
		SinglePartAcqStats();
		int partnerFreq[NUM_PARTNER_BINS]; //Number of people with specified number of partners
	};

	class SurvivalStats
	{
	public:
		SurvivalStats();
		unsigned int
		timeToDeathGenderSum[DmgProfile::ENDGender]; //sum of time to death for people who die during model run (used to calculate mean) strat by gender
		unsigned int
		timeToDeathGenderSumSquare[DmgProfile::ENDGender];//sum square of time to death strat by gender (used to calculate SD)
		unsigned int numDeathGender[DmgProfile::ENDGender]; //number who died strat by gender

		unsigned int
		timeToDeathEmplRiskSum[DmgProfile::ENDEmployment][Person::ENDRiskLevel]; //time to death stratified by CSW status and Risk level
		unsigned int timeToDeathEmplRiskSumSquare[DmgProfile::ENDEmployment][Person::ENDRiskLevel];
		unsigned int numDeathEmplRisk[DmgProfile::ENDEmployment][Person::ENDRiskLevel];

		unsigned int timeToDeathHIVStatusSum[ENDInitHIVStatus]; //time to death stratified by initial HIV status
		unsigned int timeToDeathHIVStatusSumSquare[ENDInitHIVStatus];
		unsigned int numDeathHIVStatus[ENDInitHIVStatus];

		unsigned int
		timeToInfOrDeathGenderSum[DmgProfile::ENDGender]; //sum of time to infection or death for people who die during model run (used to calculate mean) strat by gender
		unsigned int
		timeToInfOrDeathGenderSumSquare[DmgProfile::ENDGender];//sum square of time to infection or death strat by gender (used to calculate SD)
		unsigned int
		numInfOrDeathGender[DmgProfile::ENDGender]; //number who got infected or died (not counting initial HIV+ prevalent cases)

		unsigned int
		timeToInfOrDeathEmplRiskSum[DmgProfile::ENDEmployment][Person::ENDRiskLevel]; //time to inf or death stratified by CSW status and Risk level
		unsigned int timeToInfOrDeathEmplRiskSumSquare[DmgProfile::ENDEmployment][Person::ENDRiskLevel];
		unsigned int numInfOrDeathEmplRisk[DmgProfile::ENDEmployment][Person::ENDRiskLevel];

		unsigned int
		timeFromInfToDeathGenderSum[DmgProfile::ENDGender]; //sum of time from infection to death for people who die during model run (used to calculate mean) strat by gender
		unsigned int
		timeFromInfToDeathGenderSumSquare[DmgProfile::ENDGender];//sum square of time from infection to death strat by gender (used to calculate SD)
		unsigned int numInfDeathGender[DmgProfile::ENDGender]; //number who died while infected (counting prevalent HIV+)

		unsigned int
		timeFromInfToDeathEmplRiskSum[DmgProfile::ENDEmployment][Person::ENDRiskLevel]; //time from inf to death stratified by CSW status and Risk level
		unsigned int timeFromInfToDeathEmplRiskSumSquare[DmgProfile::ENDEmployment][Person::ENDRiskLevel];
		unsigned int numInfDeathEmplRisk[DmgProfile::ENDEmployment][Person::ENDRiskLevel];

	};
private:
	//TODO: Have structure for prevalence and incidence at 5 time points (maybe by default these are 1, 0.2*maxTime, 0.4*maxTime, etc?)
	//These times have to be defined!
	static const int NUM_TIMES_TO_RECORD = 5;
	int timeToRecord[NUM_TIMES_TO_RECORD];

	static const int NUM_TIMES_TO_RECORD_LE = 5;
	int timeToRecordLE[NUM_TIMES_TO_RECORD_LE];
	double medianLECI;

	static const int NUM_TIMES_TO_RECORD_PARTACQ = 5;
	int timeToRecordPartAcq[NUM_TIMES_TO_RECORD_PARTACQ];
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
	std::array<std::size_t, SimContext::TEST_RESULT_NUM> yearlyTestsByResult;

public:

	//----------< End type declarations >-------------------//
	SingleLEStats *selectedLEStats;
	SinglePartAcqStats *selectedPartAcqStats;
	SurvivalStats *survivalStats;

	InfectionsTracker infectionsTracker;	//tallies infections and generates statistics

	CostsTracker costsTracker;			//tallies all costs

	ArtRolloutTracker artTracker; // records art rollout outcomes

	EnumCls<PopStats::LifeStats> *enumClass; //used in lifeStats; declared here so that deletion is possible

	StatsRecord<LifeStats, BaseEnumCls::NULL_ENUM> *lifeStats;

	PopStats(int maxTime, ticpp::Element *_LEOutputNode, ticpp::Element *_partAcqOutputNode);
	~PopStats();

	//processes a person's death
	void processDeath(Person *_p, EventParams &_eventParams);

	//processes a death that occurs after maxTime (for average life expectancy stats)
	void processPostMaxTimeDeath(Person *_p);

	void printLMStats(std::ostream &_outStream);

	void printLEStats(std::ostream &_outStream, int currTime);

	void printPartAcqStats(std::ostream &_outStream, int currTime);

	void printSurvivalStats(std::ostream &_outStream);

	void printShiftedOutcomes(std::ostream &_outStream, int currTime);

	//records an incident infection (calls InfectionTracker's method)
	void recordIncidentInfection(EventParams &_eventParams, int _time, SexualPartnership::Type _partnershipType,
	                             const Person *_infector, const Person *_infected, bool _print, ostream &_traceOutStream);

	//returns the next time greater than or equal to currTime in the list
	int getNextTimeToRecord(int currTime);

	//returns true if currTime is in timeToRecord[NUM_TIMES_TO_RECORD]
	bool isTimeToRecord(int currTime);

	//returns true if currTime is in timeToRecordLE
	bool isTimeToRecordLE(int currTime);
	bool isFirstMonthToRecordLE(int currTime);
	bool isTimeToPrintLE(int currTime);

	//returns true if currTime is in timeToRecordPartAcq
	bool isTimeToRecordPartAcq(int currTime);

	void recordPrevalenceAndIncidence(int currTime, double _prevalence, double _SAprevalence, double _incidence,
	                                  int saPopSize, int monthlyIncident, int monthlyPrevalent);

	void enableShiftedOutcomes(int monthOf1990);
	void resetYear(int newYear);
	void recordYearStartStats(int sexuallyActivePopSize, int prevalentCases);
	void recordTestStats(int numTests, const std::array<std::size_t, SimContext::TEST_RESULT_NUM> &numTestsByResult);
	void recordTreatmentAccessEligiblity(Person *person);
	void recordTreatmentAccess(Person *person);
	void recordTreatmentEligiblity(Person *person);
	void recordTreatment(Person *person);

	std::vector<PopStats::SingleTimeStats *> *getSelectedSummaryStats();
};
