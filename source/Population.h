#pragma once

#include <cstddef>
#include <fstream>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <ticpp/ticpp.h>

#include "entities/Person.h"
#include "entities/Male.h"
#include "entities/Female.h"
#include "entities/entitypool/EntityPool.h"
#include "graphviz/graphVizParse.h"
#include "statistics/PopStats.h"
#include "util/rand/RandomNums.h"

enum DebugLevel;

/*
  This class contains the main simulation logic

  The population contains an EntityPool which is further subdivided into Buckets
*/

class Population
{
private:
#include "PopulationParams.h"

public:
	//Indices of the size by age range tuple
	enum IndicesOfSizeByAgeRange
	{
		AGE_RANGE_SIZE = 0,
		MIN_AGE_IN_MONTHS,
		MAX_AGE_IN_MONTHS,
		ENDIndicesOfSizeByAgeRange
	};

	struct AgeRange
	{
		int lower;
		int upper;
	};

	typedef std::pair<AgeRange, std::size_t> AgeRangeSizePair;
	typedef std::vector<AgeRangeSizePair> AgeRangeSizeContainer;

	//This is the main circular buffer containing the BucketAge structures
	typedef boost::circular_buffer_space_optimized<BucketAge *> BucketAllAges;

	//creates a new population object given an XML input subtree which contains the parameters
	Population(EventParams &_eventParams, ticpp::Element *_popParamsNode, ticpp::Element *_LEOutputNode,
	           ticpp::Element *_partAcqOutputNode, int _maxTime);

	~Population();

	//updates population with parameters from new file
	void updatePopulation(EventParams &_eventParams, ticpp::Element *_popParamsNode);

	//initialization-related method
	//determines which DemographicProfiles have the power to initiate relationships and determines which
	//relationships they can have
	void initPartnershipBuckets();

	/*
	 * Print out to a summary stats file -- this should only be run at the end of the simulation!
	 */
	void printSummaryStats(EventParams &_eventParams);

	/*
	 * initializes the counters for incidient infections by age for infectionstracker
	 */
	void initIncidentInfectionsByAge();

	/*
	 * Applies the incident prevalence inputs to the current population (this may be delayed based on delay parameter)
	 */
	void applyIncidentPrevalence(EventParams &_eventParams);

	/*
	 * Checks to see if there is a new cepac input file to apply to certain portions of the population if rollout is being used
	 */
	void applyRolloutContext(EventParams &_eventParams, int time);

	void startTreatment(Person *person, SimContext *treatedContext);

	/*
	 * Applies Treatment to certain portions of the population if ART Rollout is turned on
	 */
	void applyARTRollout(EventParams &_eventParams);

	//Applys calibration procedure to determine if partnership prevalence in population lies in the bounds provided
	bool passesPartnershipCalibration(EventParams &_eventParams);

	//create birthRate * currSize people who are age 0 and add them to the DmgProfile::NA population
	void births(EventParams &_eventParams);

	//everyone in population ages one year
	//infected persons age another month in CEPAC
	void updatePhysicalState(EventParams &_eventParams, bool calculateLE, bool newLEPeriod);

	//counts the number of people in each age bucket used for LE
	void updateAgeBucketsLE();

	//will form, dissolve partnerships and have sexual activity
	//monthly sexual activity within population.
	//when transmissions occur, run the incident case through CEPAC to get their future life trajectory
	//  returns the # of New people of each type who was infected
	void updatePartnerships(EventParams &_eventParams);

	//counts the total size of the population and updates internal state
	int updateSize();

	//resets the monthly statistics
	void resetMonthlyStats();

	//After all of the population dynamics have run through, run all of the remaining infected persons through CEPAC until they die to get the life expectancy and such in the CEPAC output files.
	void updateFinalPhysicalState(EventParams &_eventParams);

	//gets the age bucket of the person
	Params::AgeBucketPrevalenceInfo *getAgeBucket(Person * person) const;

	//gets the index of the age bucket of the person
	std::size_t getAgeBucketIndex(Person *person) const;

	//returns internal count of how big the current population is
	std::size_t getSize() const;

	std::size_t getNASize() const;

	std::size_t getSize(DmgProfile::Gender gender) const;

	std::size_t getSASize(DmgProfile::Gender _gender, Person::RiskLevel _risk) const;

	std::size_t getCSWSize(DmgProfile::Gender _gender, Person::RiskLevel _risk) const;

	AgeRangeSizeContainer getSizeByAgeRange() const;

	PopStats *getPopStats() { return popStats; }

private:
	friend class Simulation;

	static const size_t num_eligibility_ranks = 5;

	/** this is used to assign each New population a unique id */
	static int idCounter;

	/** this number is used to access the Population stratified parameters for Male and Female*/
	int populationID;

	/** string name of population */
	std::string populationLabel;

	//tallies the statistics that the population generates throughout the simulation
	PopStats *popStats;

	/** The graph of all relationships over time, used to generate graphviz output */
	GraphVizGraphElements *graph;

	/** current size of the population */
	size_t currSize;

	/** Size of non-sexually active */
	size_t currNASize;

	/** Size of CSW's */
	size_t currCSWSize;

	/** Size by Risk */
	std::array<std::size_t, Person::ENDRiskLevel> currSizeRisk;

	/** Size of CSW's by Risk */
	std::array<std::size_t, Person::ENDRiskLevel>  currSizeRiskCSW;

	/** Size of CSW's by Risk and gender */
	std::array<std::array<std::size_t, Person::ENDRiskLevel>, DmgProfile::ENDGender> currSizeGenderRiskCSW;

	/** Size by gender */
	std::array<std::size_t, DmgProfile::ENDGender> currSizeGender;

	/** non-sexually active by gender */
	std::array<std::size_t, DmgProfile::ENDGender> currNASizeByGender;

	/** sexually active by risk and gender **/
	std::array<std::array<std::size_t, Person::ENDRiskLevel>, DmgProfile::ENDGender>  currSASizeGenderRisk;

	/** Num Died this month by Death Cause */
	std::array<std::size_t, Person::ENDDeathStatus> currDeathCauses;

	//holds the tallies for any New partnerships that were made and ended in the current month
	std::array<std::size_t, SexualPartnership::ENDType> newPartnershipCount;

	//Number of attemptedPartnerships may be higher than the actual partnerships formed if there weren't enough females/males tried to repartner with current partners
	std::array<std::size_t, SexualPartnership::ENDType> attemptedPartnershipCount;

	std::array<std::size_t, SexualPartnership::ENDType> endedPartnershipCount;

	/** Size by age range: tuple is size, minAge, maxAge
	//These only include those who are sexually active
	* //Note: There is an enum for labeling the indices of the above tuple in the Public parameters
	**/
	AgeRangeSizeContainer currSizeByAgeRange;

	AgeRangeSizeContainer currSizeByAgeRangeMale;

	AgeRangeSizeContainer currSizeByAgeRangeFemale;

	Params popWideParams;

	/** a container for all the people. This is a compartmentalized container that lets us
	//  access different types of people based on criteria. It also has an iterator that lets
	//  us access all the through a java style iterator interface */
	EntityPool *entities;

	/** fling initiators -- use BucketSexualMixing, not DmgProfileBucket because all persons
	//participating in partnerships are sexually active by definition */
	std::map<BucketSexualMixing *, std::vector<SexualPartnership::Type>> partneringInitiators;

	std::map<DmgProfile::ProfileID, std::vector<SexualPartnership::Type>> profilesToPartnershipTypes;

	/** stores eligible receivers Buckets for each type of partnership -- use BucketSexualMixing,
	//not DmgProfileBucket because all persons participating in partnerships are sexually
	//active by definition */
	std::vector<BucketSexualMixing *> potentialPartnerBuckets[SexualPartnership::ENDType];

	/** stores weights of each eligible bucket. we keep this as a separate vector so we can
	//  use pre-existing normalization and random index chooser functions. */
	std::vector<double> eligibleBucketWeights[SexualPartnership::ENDType];

	/** The people who are infected but still untreated (Only used for rollout) */
	std::list<Person *> rolloutUntreatedPool;

	/**The people who are currently being treated (Only used for rollout) */
	std::list<Person *> rolloutTreatedPool;

	/** The people who meet CDM treatment eligibility criteria
	 * they may be selected to switch to rolloutTeatedPool if slots are available.
	 */
	std::array<std::vector<Person *>, num_eligibility_ranks> rolloutEligibilityRankings;

	/*
	//forms creates partnerships of a particular type for 1 person. Will make sure that each partner is in the correct DmgProfileBucket
	//if _partnershipType == STEADY, then this will remove the partner from the EntityIndex (as they are now NOT_SINGLE)
	@param _eventParams
	@param _initiator person who is trying to find a STEADY REGULAR, CASUAL, or CSW partner
	//ERINSAYS: _p_Iter removed for now -- may be replaced when list of allMales and allFemales are implemented
	@param _p_Iter an iterator that points to _initiator for fast removal from a DmgProfileBucket. If this is nullptr, then it's ignored
	@param _partnershipType particular type of partnership that _initiator is looking to form
	@param _forceNumPartnersOne if true will force _initiator to create just one partnership of type _partnership type (useful for initial regular partnerships
	@return number of partnerships formed
	*/
	unsigned int createPartnerships(EventParams &_eventParams, Person *_initiator, std::list<Person *>::iterator *_p_Iter,
	                                 SexualPartnership::Type _partnershipType, bool _forceNumPartnersOne = false);

	/*
	  dissolves a list of particular sexual partnerships. Removes the pointer to the SexualPartnership from each member and then deletes it
	*/
	void dissolveSexualPartnerships(EventParams &_eventParams, Person *_initiator,
	                                std::list<SexualPartnership *> &_partnershipsToEnd);

	/**
	   @param _gender gender of person we want to create
	   @param _ageBucketParams	parameters that determine a prevalent person's characteristics. If this is nullptr, then this method will create a newborn
	   @return a newly formed person
	**/
	Person *generatePerson(EventParams &_eventParams, DmgProfile::Gender _gender,
	                       Population::Params::AgeBucketPrevalenceInfo *_ageBucketParams, bool toTrace);

	/*
	  processes the death of 1 person, updates statistics, removes that person from any relationships
	  @param _deceased pointer to deceased person
	*/
	void processDeath(EventParams &_eventParams, Person *_p, bool calculateLE);

	void calculateRolloutEligibilityRankings(const EventParams::RolloutEligibility &criteria);

	//calculates the number of HIV cases for each sexually active DmgProfileBucket and stores it in _infectionsTracker
	int calculatePrevalentPopulationSize(int _time);

	/**
	   this is called at the end of each method that affects the population members
	   prints out trace information such as the size of each DmgProfileBucket
	   @param _d debug level that we should print at
	   @param _totalAffected the number of people affected by the most recent events
	   @param _totalAffectedLabel a label that identifies the meaning behind the value _totalAffected
	   @param _showInfected if true, will indicate how many people are currently infected in each DmgProfileBucket
	**/
	void printMethodResults(EventParams &_eventParams, string _methodName, string _eventLabel, int _totalAffected,
	                        std::string _totalAffectedLabel, bool _showInfections);

	/**
	   this saves the state of the population and writes to file
	**/
	void saveState(std::ostream &_outStream, int currTime);

	/**
	   this is called at the end of each month to print the statistics about each population to the Population.out file
	   @param _time the current time in the simulation
	   @param _outStream the stream to print
	**/
	void printPopulation(EventParams &_eventParams, int _time, std::ostream &_outStream);

	/**
	   this is called at end of each month to print statistics about the behavior of the population to the Behavior.out file
	**/
	void printPartnerships(EventParams &_eventParams, int _time, std::ostream &_outStream);

	/**
	   this is called at end of each month to print statistics about the clinical status of the population to the Clinical.out file
	**/
	void printClinical(EventParams &_eventParams, int _time, std::ostream &_outStream);

	void printARTRolloutOutcomes(EventParams &_eventParams, std::ostream &_outStream);

	/**
	   this is called at specified time points to record the partner frequency
	**/
	void recordPartAcqFreq();

	void recordShiftedOutcomes(EventParams &_eventParams, std::ostream &_outStream);
};
