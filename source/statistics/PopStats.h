#pragma once

#include <fstream>
#include <vector>
#include <unordered_map>

#include "StatsRecord.h"
#include "InfectionsTracker.h"
#include "CostsTracker.h"
#include "../data/Enum.h"
#include "../entities/classifiers/SexualPartnership.h"
#include "../util/ticpp/ticpp.h"

class Person;

/**
This class contains population level statistics
**/
class PopStats {
	//----------< Begin type declarations >-------------------//
	//if we change things here, make sure to to change the LifeStatsStr
public:
	enum InitHIVStatus {
		PREVALENT,
		NON_PREVALENT,
		ENDInitHIVStatus
	};

	enum LifeStats {
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

	static const char *LifeStatsStr[PopStats::ENDLifeStats];


	struct SingleTimeStats
	{
		long timeOfStats;
		double prevalence;
		double SAprevalence; 	//Prevalence of sexually active population only
		double incidence; 		//TODO: Incidence will be an average over the 12 months leading up to the given time point
		long cumulativeNumberDead;	//Total number of persons who have died since time 0
	};

	class SingleLEStats{
	public:
		SingleLEStats();
		long deathsByAge[Person::maxYrForDeathStats];//number of deaths in that time period by age
		long popByAge[Person::maxYrForDeathStats]; //number of people in that age bucket
	};

	class SinglePartAcqStats{
	public:
		static const int NUM_PARTNER_BINS = 16; //Number of partner bins to store for freq plot
		SinglePartAcqStats();
		long partnerFreq[NUM_PARTNER_BINS]; //Number of people with specified number of partners
	};

	class SurvivalStats{
	public:
		SurvivalStats();
		unsigned long timeToDeathGenderSum[DmgProfile::ENDGender]; //sum of time to death for people who die during model run (used to calculate mean) strat by gender
		unsigned long timeToDeathGenderSumSquare[DmgProfile::ENDGender];//sum square of time to death strat by gender (used to calculate SD)
		unsigned int numDeathGender[DmgProfile::ENDGender]; //number who died strat by gender 

		unsigned long timeToDeathEmplRiskSum[DmgProfile::ENDEmployment][Person::ENDRiskLevel]; //time to death stratified by CSW status and Risk level
		unsigned long timeToDeathEmplRiskSumSquare[DmgProfile::ENDEmployment][Person::ENDRiskLevel];
		unsigned int numDeathEmplRisk[DmgProfile::ENDEmployment][Person::ENDRiskLevel];

		unsigned long timeToDeathHIVStatusSum[ENDInitHIVStatus]; //time to death stratified by initial HIV status
		unsigned long timeToDeathHIVStatusSumSquare[ENDInitHIVStatus];
		unsigned int numDeathHIVStatus[ENDInitHIVStatus];

		unsigned long timeToInfOrDeathGenderSum[DmgProfile::ENDGender]; //sum of time to infection or death for people who die during model run (used to calculate mean) strat by gender
		unsigned long timeToInfOrDeathGenderSumSquare[DmgProfile::ENDGender];//sum square of time to infection or death strat by gender (used to calculate SD)
		unsigned int numInfOrDeathGender[DmgProfile::ENDGender]; //number who got infected or died (not counting initial HIV+ prevalent cases) 

		unsigned long timeToInfOrDeathEmplRiskSum[DmgProfile::ENDEmployment][Person::ENDRiskLevel]; //time to inf or death stratified by CSW status and Risk level
		unsigned long timeToInfOrDeathEmplRiskSumSquare[DmgProfile::ENDEmployment][Person::ENDRiskLevel];
		unsigned int numInfOrDeathEmplRisk[DmgProfile::ENDEmployment][Person::ENDRiskLevel];

		unsigned long timeFromInfToDeathGenderSum[DmgProfile::ENDGender]; //sum of time from infection to death for people who die during model run (used to calculate mean) strat by gender
		unsigned long timeFromInfToDeathGenderSumSquare[DmgProfile::ENDGender];//sum square of time from infection to death strat by gender (used to calculate SD)
		unsigned int numInfDeathGender[DmgProfile::ENDGender]; //number who died while infected (counting prevalent HIV+)

		unsigned long timeFromInfToDeathEmplRiskSum[DmgProfile::ENDEmployment][Person::ENDRiskLevel]; //time from inf to death stratified by CSW status and Risk level
		unsigned long timeFromInfToDeathEmplRiskSumSquare[DmgProfile::ENDEmployment][Person::ENDRiskLevel];
		unsigned int numInfDeathEmplRisk[DmgProfile::ENDEmployment][Person::ENDRiskLevel];

	};
private:
	//TODO: Have structure for prevalence and incidence at 5 time points (maybe by default these are 1, 0.2*maxTime, 0.4*maxTime, etc?)
	//These times have to be defined!
	static const int NUM_TIMES_TO_RECORD = 5;
	long timeToRecord[NUM_TIMES_TO_RECORD];

	static const int NUM_TIMES_TO_RECORD_LE=5;
	long timeToRecordLE[NUM_TIMES_TO_RECORD_LE];
	double medianLECI;

	static const int NUM_TIMES_TO_RECORD_PARTACQ = 5;
	long timeToRecordPartAcq[NUM_TIMES_TO_RECORD_PARTACQ];
	bool printHeaderPartAcq;

	std::vector<SingleTimeStats*> selectedSummaryStats;

	int monthOf1990;
	bool calculateShiftedOutcomes;
	int relativeYear;
	int yearStartPrevalentInfections;
	int yearStartSexuallyActivePopSize;
	int yearlyCumulativeSexuallyActivePopSize;
	int yearlyIncidentInfections;
	int yearlyTests;
	std::set<Person *> uniqueYearlyEligible;
	std::set<Person *> uniqueYearlyTreated;
	std::vector<int> yearlyTestsByResult;

	//TODO: Is this necessary?
	//long cumulativeNumberDead; //This is a running tally of the total number of persons who have died during the course of the simulation

public:

	//----------< End type declarations >-------------------//
	SingleLEStats* selectedLEStats;
	SinglePartAcqStats* selectedPartAcqStats;
	SurvivalStats* survivalStats;

	InfectionsTracker infectionsTracker;	//tallies infections and generates statistics

	CostsTracker costsTracker;			//tallies all costs

	EnumCls<PopStats::LifeStats> *enumClass; //used in lifeStats; declared here so that deletion is possible

	StatsRecord<LifeStats, BaseEnumCls::NULL_ENUM> *lifeStats;

	PopStats(long maxTime,ticpp::Element* _LEOutputNode, ticpp::Element* _partAcqOutputNode);
	~PopStats();

	//processes a person's death
	void processDeath(Person *_p, EventParams& _eventParams);

	//processes a death that occurs after maxTime (for average life expectancy stats)
	void processPostMaxTimeDeath(Person *_p);

	void printLMStats(std::ostream &_outStream);

	void printLEStats(std::ostream &_outStream,long currTime);

	void printPartAcqStats(std::ostream &_outStream, long currTime);

	void printSurvivalStats(std::ostream &_outStream);

	void printShiftedOutcomes(std::ostream &_outStream, int currTime);

	//records an incident infection (calls InfectionTracker's method)
	void recordIncidentInfection(EventParams& _eventParams, long _time, SexualPartnership::Type _partnershipType, const Person *_infector, const Person *_infected, bool _print, ostream &_traceOutStream);

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

	void recordPrevalenceAndIncidence(long currTime, double _prevalence, double _SAprevalence, double _incidence, int saPopSize, int monthlyIncident, int monthlyPrevalent);

	void enableShiftedOutcomes(int monthOf1990);
	void resetYear(int newYear);
	void recordYearStartStats(int sexuallyActivePopSize, int prevalentCases);
	void recordTestStats(int numTests, const std::vector<int> &numTestsByResult);
	void recordEligiblePerson(Person *person);
	void recordTreatment(Person *person);

	std::vector<PopStats::SingleTimeStats*>* getSelectedSummaryStats();
};
