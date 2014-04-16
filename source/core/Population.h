#pragma once

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "PopulationParameters.h"
#include "data/AgeRangeSizeContainer.h"
#include "entities/Female.h"
#include "entities/Male.h"
#include "entities/Person.h"
#include "entities/entitypool/EntityPool.h"
#include "statistics/PopulationStatistics.h"
#include "util/rand/RandomNumberGenerator.h"

enum DebugLevel;

/// <summary>
/// This class contains the main simulation logic
/// </summary>
/// <remarks>
/// The population contains an EntityPool which is further subdivided into Buckets
/// </remarks>
class Population
{
public:
	//This is the main circular buffer containing the BucketAge structures
	typedef boost::circular_buffer_space_optimized<BucketAge *> BucketAllAges;

	//creates a new population object given an XML input subtree which contains the parameters
	Population(EventParams &parameters);
	~Population();
	void operator=(const Population &) = delete;

	unsigned int GetId() const { return populationID; }

	void SetParameters(const PopulationParameters &parameters) { popWideParams = parameters; }

	void SetCondomCost(double condom_cost) { popWideParams.condomCost = condom_cost; }

	void SetCircumcisionCost(double circumcision_cost) { popWideParams.circumcisionCost = circumcision_cost; }

	void circumcise(RandomNumberGenerator &rng, double proportion, PopulationTarget target);

	void circumcise(Person *p);

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

	int UpdateTreatmentSlots(double rolloutProportion);

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

	//create birthRate * currSize people who are age 0 and add them to the DemographicProfile::NA population
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
	long updateSize();

	//resets the monthly statistics
	void resetMonthlyStats();

	//After all of the population dynamics have run through, run all of the remaining infected persons through CEPAC until they die to get the life expectancy and such in the CEPAC output files.
	void updateFinalPhysicalState(EventParams &_eventParams);

	//gets the age bucket of the person
	PopulationParameters::AgeBucketPrevalenceInfo &getAgeBucket(Person *);
	//gets the index of the age bucket of the person
	int getAgeBucketIndex(Person *);

	//returns internal count of how big the current population is
	long getSize();
	long getNASize();
	long getSize(DemographicProfile::Gender gender);
	long getSASize(DemographicProfile::Gender _gender, Person::RiskLevel _risk);
	long getCSWSize(DemographicProfile::Gender _gender, Person::RiskLevel _risk);

	const AgeRangeSizeContainer &getSizeByAgeRange() const { return currSizeByAgeRange; }

	const PopulationStatistics &getPopulationStatistics() const { return popStats; }

	PopulationStatistics &getPopulationStatistics() { return popStats; }

	void Apply(const PopulationTarget &target, RandomNumberGenerator &rng, std::function<void(Person *)> modifier, double probability);

	void ValidateState();

	int GetNumberToTrace() const { return parameters_.numToTrace; }

private:
	friend class SimulationBuilderXml;

	/*
	//forms creates partnerships of a particular type for 1 person. Will make sure that each partner is in the correct BucketDemographicProfile
	//if _partnershipType == STEADY, then this will remove the partner from the EntityIndex (as they are now NOT_SINGLE)
	@param _eventParams
	@param _initiator person who is trying to find a STEADY REGULAR, CASUAL, or CSW partner
	//ERINSAYS: _p_Iter removed for now -- may be replaced when list of allMales and allFemales are implemented
	@param _p_Iter an iterator that points to _initiator for fast removal from a BucketDemographicProfile. If this is nullptr, then it's ignored
	@param _partnershipType particular type of partnership that _initiator is looking to form
	@param _forceNumPartnersOne if true will force _initiator to create just one partnership of type _partnership type (useful for initial regular partnerships
	@return number of partnerships formed
	*/
	unsigned long createPartnerships(EventParams &_eventParams, Person *_initiator, std::list<Person *>::iterator *_p_Iter,
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
	Person *generatePerson(EventParams &_eventParams, DemographicProfile::Gender _gender,
	                       PopulationParameters::AgeBucketPrevalenceInfo *_ageBucketParams, bool toTrace);

	/*
	  processes the death of 1 person, updates statistics, removes that person from any relationships
	  @param _deceased pointer to deceased person
	*/
	void processDeath(EventParams &_eventParams, Person *_p, bool calculateLE);

	void determineRankings(const EventParams::RolloutEligibility &criteria);

	//-----------< END helper methods  >--------------------//


	//-----------< BEGIN getters,setters, and print functions >--------------------//
	//calculates the number of HIV cases for each sexually active BucketDemographicProfile and stores it in _infectionsTracker
	long calcPrevalentPopulation(long _time);

	/**
	   this is called at the end of each method that affects the population members
	   prints out trace information such as the size of each BucketDemographicProfile
	   @param _d debug level that we should print at
	   @param _totalAffected the number of people affected by the most recent events
	   @param _totalAffectedLabel a label that identifies the meaning behind the value _totalAffected
	   @param _showInfected if true, will indicate how many people are currently infected in each BucketDemographicProfile
	**/
	void printMethodResults(EventParams &_eventParams, string _methodName, string _eventLabel, long _totalAffected,
	                        string _totalAffectedLabel, bool _showInfections);

	/**
	   this saves the state of the population and writes to file
	**/
	void saveState(std::ostream &_outStream, long currTime);

	/**
	   this is called at the end of each month to print the statistics about each population to the Population.out file
	   @param _time the current time in the simulation
	   @param _outStream the stream to print
	**/
	void printPopulation(EventParams &_eventParams, long _time, std::ostream &_outStream);

	/**
	   this is called at end of each month to print statistics about the behavior of the population to the Behavior.out file
	**/
	void printPartnerships(EventParams &_eventParams, long _time, std::ostream &_outStream);

	/**
	   this is called at end of each month to print statistics about the clinical status of the population to the Clinical.out file
	**/
	void printClinical(EventParams &_eventParams, long _time, std::ostream &_outStream);

	void printARTRolloutOutcomes(EventParams &_eventParams, std::ostream &_outStream);

	/**
	   this is called at specified time points to record the partner frequency
	**/
	void recordPartAcqFreq();

	void recordShiftedOutcomes(EventParams &_eventParams, std::ostream &_outStream);

	/** this is used to assign each New population a unique id */
	static unsigned int idCounter;

	/** this number is used to access the Population stratified parameters for Male and Female*/
	unsigned int populationID;
	/** string name of population */
	string populationLabel;

	friend class Simulation;

	/** current size of the population */
	long currSize;
	/** Size of non-sexually active */
	long currNASize;
	/** Size of CSW's */
	long currCSWSize;
	/** Size by Risk */
	long currSizeRisk[Person::ENDRiskLevel];
	/** Size of CSW's by Risk */
	long currSizeRiskCSW[Person::ENDRiskLevel];
	/** Size of CSW's by Risk and gender */
	long currSizeGenderRiskCSW[DemographicProfile::ENDGender][Person::ENDRiskLevel];
	/** Size by gender */
	long currSizeGender[DemographicProfile::ENDGender];
	/** non-sexually active by gender */
	long currNASizeByGender[DemographicProfile::ENDGender];
	/** sexually active by risk and gender **/
	long currSASizeGenderRisk[DemographicProfile::ENDGender][Person::ENDRiskLevel];
	/** Num Died this month by Death Cause */
	long currDeathCauses[Person::ENDDeathStatus];
	/** Size by age range: tuple is size, minAge, maxAge
	//These only include those who are sexually active
	* //Note: There is an enum for labeling the indices of the above tuple in the Public parameters
	**/
	AgeRangeSizeContainer currSizeByAgeRange;
	AgeRangeSizeContainer currSizeByAgeRangeMale;
	AgeRangeSizeContainer currSizeByAgeRangeFemale;
	/** The people who are infected but still untreated (Only used for rollout)
	*/
	std::list<Person *> rolloutUntreatedPool;

	/**The people who are currently being treated (Only used for rollout)
	*/
	std::list<Person *> rolloutTreatedPool;

	PopulationParameters popWideParams;

	/** a container for all the people. This is a compartmentalized container that lets us
	//  access different types of people based on criteria. It also has an iterator that lets
	//  us access all the through a java style iterator interface */
	std::unique_ptr<EntityPool> entities;

	/** fling initiators -- use BucketSexualMixing, not BucketDemographicProfile because all persons
	//participating in partnerships are sexually active by definition */
	std::map<BucketSexualMixing *, std::vector<SexualPartnership::Type>> partneringInitiators;
	std::map<DemographicProfile::ProfileID, std::vector<SexualPartnership::Type>> profilesToPartnershipTypes;
	/** stores eligible receivers Buckets for each type of partnership -- use BucketSexualMixing,
	//not BucketDemographicProfile because all persons participating in partnerships are sexually
	//active by definition */
	std::vector<BucketSexualMixing *> potentialPartnerBuckets[(int)SexualPartnership::Type::ENDType];
	/** stores weights of each eligible bucket. we keep this as a separate vector so we can
	//  use pre-existing normalization and random index chooser functions. */
	std::vector<double> eligibleBucketWeights[(int)SexualPartnership::Type::ENDType];

	std::array<std::vector<Person *>, 5> rankedForTreatment;

	PopulationStatistics popStats;	//tallies the statistics that the population generates throughout the simulation

	EventParams &parameters_;

	double treatmentCorrectionFactor;
};
