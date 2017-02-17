#pragma once

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Intervention.h"
#include "PopulationParameters.h"
#include "../data/AgeRangeSizeContainer.h"
#include "../entities/Female.h"
#include "../entities/Male.h"
#include "../entities/Person.h"
#include "../entities/entitypool/EntityPool.h"
#include "../statistics/PopulationStatistics.h"
#include "utility/RandomNumberGenerator.h"
#include "utility/Nullable.h"

/// <summary>
/// This class contains the main simulation logic
/// </summary>
/// <remarks>
/// The population contains an EntityPool which is further subdivided into Buckets
/// </remarks>
class Population
{
public:
	/// <summary>
    /// This is the main circular buffer containing the BucketAge structures
    /// </summary>
	typedef boost::circular_buffer_space_optimized<BucketAge *> BucketAllAges;

	/// <summary>
    /// Creates a new population object given an XML input subtree which contains the parameters
    /// </summary>
	Population(EventParams &parameters);

	~Population();

	void operator=(const Population &) = delete;

	unsigned int GetId() const { return populationID; }

	void SetParameters(const PopulationParameters &parameters) { popWideParams = parameters; }

	void SetCondomCost(double condom_cost) { popWideParams.condomCost = condom_cost; }

	void SetCircumcisionCost(double circumcision_cost) { popWideParams.circumcisionCost = circumcision_cost; }

	void Circumcise(Person *p);

    SimContext *LoadCepacFile(const std::string &cepac_file) { return parameters_.LoadCepacContext(cepac_file); }

    std::vector<Person *> Find(std::function<bool(Person *)> predicate);

    /// <summary>
	/// determines which DemographicProfiles have the power to initiate relationships and determines which
	/// relationships they can have
    /// </summary>
	void InitPartnershipBuckets();

	/// <summary>
	/// Print out to a summary stats file -- this should only be run at the end of the simulation!
	/// </summary>
	void PrintSummaryStats(EventParams &_eventParams);

    /// <summary>
	/// initializes the counters for incident infections by age for infections tracker
    /// </summary>
	void InitIncidentInfectionsByAge();

    /// <summary>
    ///
    /// </summary>
	int UpdateTreatmentSlots(double rolloutProportion);

    /// <summary>
	/// Applies the incident prevalence inputs to the current population (this may be delayed based on delay parameter)
    /// </summary>
	void ApplyIncidentPrevalence(EventParams &_eventParams);

    /// <summary>
	/// Checks to see if there is a new cepac input file to apply to certain portions of the population if rollout is being used
    /// </summary>
	void ApplyRolloutContext(EventParams &_eventParams, int time);

	void StartTreatment(Person *person, SimContext *treatedContext);

    /// <summary>
    /// Applies Treatment to certain portions of the population if ART Rollout is turned on
    /// </summary>
	void ApplyARTRollout(EventParams &_eventParams);

    /// <summary>
    /// Applies calibration procedure to determine if partnership prevalence in population lies in the bounds provided
    /// </summary>
	bool PassesPartnershipCalibration(EventParams &_eventParams);

    /// <summary>
    /// create birthRate * currSize people who are age 0 and add them to the DemographicProfile::SexualActivityStatus::NotActive population
    /// </summary>
	void Births(EventParams &_eventParams);

    /// <summary>
    /// everyone in population ages one year
    /// infected persons age another month in CEPAC
    /// </summary>
	void UpdatePhysicalState(EventParams &_eventParams, bool calculateLE, bool newLEPeriod);

    /// <summary>
    /// counts the number of people in each age bucket used for LE
    /// </summary>
	void UpdateAgeBucketsLE();

    /// <summary>
    /// will form, dissolve partnerships and have sexual activity
    /// monthly sexual activity within population.
    /// when transmissions occur, run the incident case through CEPAC to get their future life trajectory
    /// returns the # of New people of each type who was infected
    /// </summary>
	void UpdatePartnerships(EventParams &_eventParams);

    /// <summary>
    /// counts the total size of the population and updates internal state
    /// </summary>
	std::size_t UpdateSize();

    /// <summary>
    /// resets the monthly statistics
    /// </summary>
	void ResetMonthlyStats();

    /// <summary>
    /// After all of the population dynamics have run through, run all of the remaining infected persons through CEPAC until they die to get the life expectancy and such in the CEPAC output files.
    /// </summary>
	void UpdateFinalPhysicalState(EventParams &_eventParams);

    /// <summary>
    /// gets the age bucket of the person
    /// </summary>
	PopulationParameters::AgeBucketPrevalenceInfo &GetAgeBucket(Person *);

    /// <summary>
    /// gets the index of the age bucket of the person
    /// </summary>
	int GetAgeBucketIndex(Person *);

    /// <summary>
    /// returns internal count of how big the current population is
    /// </summary>
    std::size_t GetSize();

    /// <summary>
    /// returns internal count of how big the current population is
    /// </summary>
    std::size_t GetNASize();

    /// <summary>
    /// returns internal count of how big the current population is
    /// </summary>
    std::size_t GetSize(DemographicProfile::Gender gender);

    /// <summary>
    /// returns internal count of how big the current population is
    /// </summary>
    std::size_t GetSASize(DemographicProfile::Gender _gender, Person::RiskLevel _risk);

    /// <summary>
    /// returns internal count of how big the current population is
    /// </summary>
    std::size_t GetCSWSize(DemographicProfile::Gender _gender, Person::RiskLevel _risk);

    /// <summary>
    ///
    /// </summary>
	const AgeRangeSizeContainer &GetSizeByAgeRange() const { return currSizeByAgeRange; }

    /// <summary>
    ///
    /// </summary>
	const PopulationStatistics &GetPopulationStatistics() const { return populationStatistics; }

    /// <summary>
    ///
    /// </summary>
    PopulationStatistics &GetPopulationStatistics() { return populationStatistics; }

    const std::unordered_set<Person *> &GetDeadPeopleThisMonth() const { return dead_people_this_month_; }

    /// <summary>
    ///
    /// </summary>
	int GetNumberToTrace() const { return parameters_.numToTrace; }

    void RegisterIntervention(const Intervention &intervention);

private:
    friend class Intervention;
    friend class SimulationBuilderXml;
    friend class Simulation;

	/// <summary>
	/// forms creates partnerships of a particular type for 1 person. Will make sure that each partner is in the correct BucketDemographicProfile
	/// if _partnershipType == STEADY, then this will remove the partner from the EntityIndex (as they are now NOT_SINGLE)
    /// </summary>
    /// <remarks>
	/// @param _eventParams
	/// @param _initiator person who is trying to find a STEADY REGULAR, CASUAL, or CSW partner
	/// ERINSAYS: _p_Iter removed for now -- may be replaced when list of allMales and allFemales are implemented
	/// @param _p_Iter an iterator that points to _initiator for fast removal from a BucketDemographicProfile. If this is nullptr, then it's ignored
	/// @param _partnershipType particular type of partnership that _initiator is looking to form
	/// @param _forceNumPartnersOne if true will force _initiator to create just one partnership of type _partnership type (useful for initial regular partnerships
	/// @return number of partnerships formed
    /// </remarks>
	unsigned long CreatePartnerships(EventParams &_eventParams, Person *_initiator, std::list<Person *>::iterator *_p_Iter,
	                                 SexualPartnership::Type _partnershipType, bool _forceNumPartnersOne = false);

    /// <summary>
	/// dissolves a list of particular sexual partnerships. Removes the pointer to the SexualPartnership from each member and then deletes it
    /// </summary>
	void DissolveSexualPartnerships(EventParams &_eventParams, Person *_initiator,
	                                std::list<SexualPartnership *> &_partnershipsToEnd);

    /// <summary>
    /// @param _gender gender of person we want to create
    /// @param _ageBucketParams	parameters that determine a prevalent person's characteristics. If this is nullptr, then this method will create a newborn
    /// @return a newly formed person
    /// </summary>
	Person *GeneratePerson(EventParams &_eventParams,
			       DemographicProfile::Gender _gender,
	                       PopulationParameters::AgeBucketPrevalenceInfo
			       *_ageBucketParams, bool toTrace);

	/// <summary>
	/// helper funtions for ApplyIncidentPrevalence()
	/// <summary>
	void ApplyPrevalentInfection(Person *p);
	void ApplyPrevalentInfections(DemographicProfile::Gender _gender,
				      std::vector<std::array<int, 3>> bucket);
	bool rollForChronicInfection(RandomNumberGenerator &_randomNums);

    /// <summary>
	/// processes the death of 1 person, updates statistics, removes that person from any relationships
	/// @param _deceased pointer to deceased person
    /// </summary>
	void ProcessDeath(EventParams &_eventParams, Person *_p, bool calculateLE);

    /// <summary>
    /// 
    /// </summary>
	void DetermineRankings(const EventParams::RolloutEligibility &criteria);

    /// <summary>
	/// calculates the number of HIV cases for each sexually active BucketDemographicProfile and stores it in _infectionsTracker
    /// </summary>
	long CalcPrevalentPopulation(long _time);

    /// <summary>
	/// this is called at the end of each method that affects the population members
    /// prints out trace information such as the size of each BucketDemographicProfile
    /// @param _d debug level that we should print at
    /// @param _totalAffected the number of people affected by the most recent events
    /// @param _totalAffectedLabel a label that identifies the meaning behind the value _totalAffected
    /// @param _showInfected if true, will indicate how many people are currently infected in each BucketDemographicProfile
    /// </summary>
    void PrintMethodResults(EventParams &_eventParams, const std::string &_methodName, const std::string &_eventLabel, long _totalAffected,
        const std::string &_totalAffectedLabel, bool _showInfections);

    /// <summary>
	/// this saves the state of the population and writes to file
    /// </summary>
	void SaveState(std::ostream &_outStream, long currTime);

    /// <summary>
	/// this is called at the end of each month to print the statistics about each population to the Population.out file
	/// @param _time the current time in the simulation
	/// @param _outStream the stream to print
    /// </summary>
	void PrintPopulation(EventParams &_eventParams, long _time, std::ostream &_outStream);

    /// <summary>
    /// this is called at end of each month to print statistics about the behavior of the population to the Behavior.out file
    /// </summary>
	void PrintPartnerships(EventParams &_eventParams, long _time, std::ostream &_outStream);

    /// <summary>
	/// this is called at end of each month to print statistics about the clinical status of the population to the Clinical.out file
    /// </summary>
	void PrintClinical(EventParams &_eventParams, long _time, std::ostream &_outStream);

    /// <summary>
    /// </summary>
	void PrintARTRolloutOutcomes(EventParams &_eventParams, std::ostream &_outStream);

    /// <summary>
	/// this is called at specified time points to record the partner frequency
    /// </summary>
	void RecordPartAcqFreq();

    /// <summary>
    /// </summary>
	void RecordShiftedOutcomes(EventParams &_eventParams, std::ostream &_outStream);

    void RecordInfection(const Person *infectee, const Person *infector, int time);

    /// <summary>
    /// this is used to assign each New population a unique id
    /// </summary>
    static unsigned int idCounter;

    /// <summary>
	/// this number is used to access the Population stratified parameters for Male and Female
    /// </summary>
	unsigned int populationID;

    /// <summary>
	/// name of population
    /// </summary>
	std::string populationLabel;

    /// <summary>
	/// current size of the population
    /// </summary>
	std::size_t currSize;

    /// <summary>
	/// Size of non-sexually active
    /// </summary>
    std::size_t currNASize;

    /// <summary>
	/// Size of CSW's
    /// </summary>
    std::size_t currCSWSize;

    /// <summary>
	/// Size by Risk
    /// </summary>
    std::size_t currSizeRisk[Person::ENDRiskLevel];

    /// <summary>
	/// Size of CSW's by Risk
    /// </summary>
    std::size_t currSizeRiskCSW[Person::ENDRiskLevel];

    /// <summary>
	/// Size of CSW's by Risk and gender
    /// </summary>
    std::size_t currSizeGenderRiskCSW[(std::size_t)DemographicProfile::Gender::Last][Person::ENDRiskLevel];

    /// <summary>
	/// Size by gender
    /// </summary>
    std::size_t currSizeGender[(std::size_t)DemographicProfile::Gender::Last];

    /// <summary>
	/// non-sexually active by gender
    /// </summary>
    std::size_t currNASizeByGender[(std::size_t)DemographicProfile::Gender::Last];

    /// <summary>
	/// sexually active by risk and gender
    /// </summary>
    std::size_t currSASizeGenderRisk[(std::size_t)DemographicProfile::Gender::Last][Person::ENDRiskLevel];

    /// <summary>
	/// Num Died this month by Death Cause
    /// </summary>
    std::size_t currDeathCauses[Person::ENDDeathStatus];

    /// <summary>
	/// Size by age range: tuple is size, minAge, maxAge
	/// These only include those who are sexually active
    /// </summary>
    /// <remarks>
	/// There is an enum for labeling the indices of the above tuple in the Public parameters
	/// </remarks>
	AgeRangeSizeContainer currSizeByAgeRange;

    /// <summary>
    /// </summary>
	AgeRangeSizeContainer currSizeByAgeRangeMale;

    /// <summary>
    /// </summary>
	AgeRangeSizeContainer currSizeByAgeRangeFemale;

    /// <summary>
	/// The people who are infected but still untreated (Only used for rollout)
    /// </summary>
	std::list<Person *> rolloutUntreatedPool;

    /// <summary>
	/// The people who are currently being treated (Only used for rollout)
    /// </summary>
	std::list<Person *> rolloutTreatedPool;

    void recordMale(Person *person);
    std::size_t num_circumcised_na;
    std::size_t num_circumcised_sa;

    /// <summary>
    /// </summary>
	PopulationParameters popWideParams;

    /// <summary>
	/// a container for all the people. This is a compartmentalized container that lets us
	/// access different types of people based on criteria. It also has an iterator that lets
	/// us access all the through a java style iterator interface 
    /// </summary>
	std::unique_ptr<EntityPool> entities;



    /// <summary>
	/// fling initiators -- use BucketSexualMixing, not BucketDemographicProfile because all persons
	/// participating in partnerships are sexually active by definition 
    /// </summary>
	std::map<BucketSexualMixing *, std::vector<SexualPartnership::Type>> partneringInitiators;

    /// <summary>
    /// </summary>
	std::map<DemographicProfile::ProfileID, std::vector<SexualPartnership::Type>> profilesToPartnershipTypes;

    /// <summary>
	/// stores eligible receivers Buckets for each type of partnership -- use BucketSexualMixing,
	/// not BucketDemographicProfile because all persons participating in partnerships are sexually
	/// active by definition
    /// </summary>
	std::map<SexualPartnership::Type, std::vector<BucketSexualMixing *>> potentialPartnerBuckets;

	/// <summary>
    /// stores weights of each eligible bucket. we keep this as a separate vector so we can
	/// use pre-existing normalization and random index chooser functions.
    /// </summary>
	std::map<SexualPartnership::Type, std::vector<double>> eligibleBucketWeights;

    /// <summary>
    /// </summary>
	std::array<std::vector<Person *>, 5> rankedForTreatment;

    /// <summary>
    /// tallies the statistics that the population generates throughout the simulation
    /// </summary>
    PopulationStatistics populationStatistics;

    /// <summary>
    /// </summary>
	EventParams &parameters_;

    /// <summary>
    /// </summary>
	double treatmentCorrectionFactor_;

    std::unordered_set<Person *> dead_people_this_month_;

    std::vector<Intervention> interventions_;

    public:
    struct PersonSummary
    {
        unsigned long person_id;
        int time_infected;
        int infection_number;
        int generation_number;
        int infected_by;
        int age_at_infection;
        int time_of_death;
        DemographicProfile profile;
        Person::RiskLevel risk_group;
    };

    private:
    std::unordered_map<unsigned long, PersonSummary> individual_summaries_;

    static const int NumIndividualSummaries = 1000;

    void SaveIndividualSummaries(std::ostream &stream) const;

    std::size_t debug_num_on_prep_;
};
