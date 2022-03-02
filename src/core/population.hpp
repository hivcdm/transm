#ifndef POPULATION_HPP
#define POPULATION_HPP

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "intervention.hpp"
#include "entities/entity.hpp"
#include "entities/entitytypes.hpp"
#include "entities/female.hpp"
#include "entities/male.hpp"
#include "entitypool/entitypool.hpp"
#include "parameters/populationparameters.hpp"
#include "parameters/agebucketprevalenceinfo.hpp"
#include "parameters/parameterdefinitions.hpp"
#include "parameters/agerangesizecontainer.hpp"
#include "statistics/monthlystats.hpp"
#include "statistics/populationstatisticsold.hpp"
#include "utility/randomnumbergenerator.hpp"
#include "utility/nullable.hpp"

namespace transm {

/** This class contains the main simulation logic.
 *  The population contains an 
 *  
 *  @param EntityPool which is further subdivided into 
 *  @param Buckets */

    class Population {
    public:
        
        /** This is the main circular buffer containing the BucketAge structures */
        using BucketAllAges = boost::circular_buffer_space_optimized<BucketAge *>;

        using RiskArray = std::array<std::size_t, (std::size_t) RiskLevel::Last>;

        /** Creates a new @param population object given an XML input subtree which contains the @param parameters */
        explicit Population(EventParams &parameters);

        /** Default destructor */
        ~Population();

        void operator=(const Population &) = delete;

        unsigned int GetId() const { return populationID; }

        void Initialize(const PopulationParameters &parameters);

        void SetCondomCost(double condom_cost) { popWideParams.condomCost = condom_cost; }

        void SetCircumcisionCost(double circumcision_cost) { popWideParams.circumcisionCost = circumcision_cost; }

        void Circumcise(Entity *p);

        void Circumcise(double proportion);

        PopulationParameters &GetParameters() { return popWideParams; }

        const PopulationParameters &GetParameters() const { return popWideParams; }

        SimContext *LoadCepacFile(const std::string &cepac_file) { return parameters_.LoadCepacContext(cepac_file); }

        std::vector<Entity *> Find(std::function<bool(Entity *)> predicate);

        std::vector<Entity *> FindNonCircumcised();

        /**
         * determines which DemographicProfiles have the power to initiate relationships and determines which
         * relationships they can have */
        void InitPartnershipBuckets();

        /** initializes the counters for incident infections by age for infections tracker */
        void InitIncidentInfectionsByAge();

        int UpdateTreatmentSlots(double rolloutProportion);

        /** 
         * Applies the incident prevalence inputs to the current population 
         * (this may be delayed based on delay parameter) */
        void ApplyIncidentPrevalence(EventParams &_eventParams);

        /**
         * Checks to see if there is a new cepac input file to apply to certain portions of the population 
         * if rollout is being used */
        void ApplyRolloutContext(EventParams &_eventParams, Time time);

        void StartTreatment(Entity *person, SimContext *treatedContext);

        /** Applies Treatment to certain portions of the population if ART Rollout is turned on */
        void ApplyARTRollout(EventParams &_eventParams);

        /** 
         * Applies calibration procedure to determine if partnership prevalence in population lies in 
         * the bounds provided */
        bool PassesPartnershipCalibration(EventParams &_eventParams);

        /** everyone in population ages one year infected persons age another month in CEPAC */
        void UpdatePhysicalState(EventParams &_eventParams, bool calculateLE, bool newLEPeriod);

        /** counts the number of people in each age bucket used for LE */
        void UpdateAgeBucketsLE();

        /**
         * will form, dissolve partnerships and have sexual activity monthly sexual activity within population.
         * when transmissions occur, run the incident case through CEPAC to get their future life trajectory
         * returns the # of New people of each type who was infected */
        void UpdatePartnerships(EventParams &_eventParams);

        void WritePartnershipNetwork(EventParams &_eventParams);
        
        /** counts the total size of the population and updates internal state */
        std::size_t UpdateSize();

        std::vector<AgeRange> GetAgeRanges() const;
        
        /** resets the monthly statistics */
        void ResetMonthlyStats();

        /**
         * After all of the population dynamics have run through, run all of the remaining infected persons through 
         * CEPAC until they die to get the life expectancy and such in the CEPAC output files. */
        void UpdateFinalPhysicalState(EventParams &_eventParams);
        
        /** gets the age bucket of the person */
        AgeBucketPrevalenceInfo &GetAgeBucket(Entity *);
        
        /** gets the index of the age bucket of the person */
        int GetAgeBucketIndex(Entity *);

        /** returns internal count of how big the current population is */
        std::size_t GetSize() const;

        /** returns internal count of how big the current population is */
        std::size_t GetNASize() const;

        std::size_t GetSASize();

        /** returns internal count of how big the current population is */
        std::size_t GetSize(DemographicProfile::Gender gender);

        /** returns internal count of how big the current population is */
        std::size_t GetSize(DemographicProfile::ProfileID profileID);

        /** returns internal count of how big the current population is */
        std::size_t GetSASize(DemographicProfile::ProfileID profileID, RiskLevel _risk);

        /** returns internal count of how big the current population is */
        std::size_t GetCSWSize(DemographicProfile::ProfileID profileID, RiskLevel _risk);

        const PopulationStatisticsOld &GetPopulationStatistics() const { return populationStatistics; }

        PopulationStatisticsOld &GetPopulationStatistics() { return populationStatistics; }

        const std::unordered_set<Entity *> &GetDeadPeopleThisMonth() const { return dead_people_this_month_; }

        int GetNumberToTrace() const { return parameters_.numToTrace; }

        void RegisterIntervention(const Intervention &intervention);

        std::size_t GetNumberCircumcised() { return num_circumcised_sa + num_circumcised_na; }

    private:
        friend class Intervention;

        friend class SimulationBuilderXml;

        friend class Simulation;

        friend class PopulationStatistics;

        friend class InfectionsTracker;

        /**
         * forms creates partnerships of a particular type for 1 person. Will make sure that each partner is in the
         * correct BucketDemographicProfile if _partnershipType == STEADY, then this will remove the partner from
         * the EntityIndex (as they are now NOT_SINGLE)
         *
         * @param _eventParams
         * @param _initiator person who is trying to find a STEADY REGULAR, CASUAL, or CSW partner
         * ERINSAYS: _p_Iter removed for now -- may be replaced when list of allMales and allFemales are implemented
         * @param _p_Iter an iterator that points to _initiator for fast removal from a BucketDemographicProfile.
         * If this is nullptr, then it's ignored
         * @param _partnershipType particular type of partnership that _initiator is looking to form
         * @param _forceNumPartnersOne if true will force _initiator to create just one partnership of type
         * _partnership type (useful for initial regular partnerships
         * @return number of partnerships formed */
        unsigned long CreatePartnerships(EventParams &_eventParams, Male *_initiator,
                                         std::list<Entity *>::iterator *_p_Iter,
                                         SexualPartnership::Type _partnershipType, bool _forceNumPartnersOne = false);

        /**
         * dissolves a list of particular sexual partnerships. Removes the pointer to the SexualPartnership from
         * each member and then deletes it */
        void DissolveSexualPartnerships(EventParams &_eventParams, Entity *_initiator,
                                        std::list<SexualPartnership *> &_partnershipsToEnd);

        /**
         * create birthRate * currSize people who are age 0 and add them to the
         * DemographicProfile::SexualActivityStatus::NotActive population */
        void Births(EventParams &_eventParams);

        /** create the initial people in the population */
        void GenerateInitialEntities();

        /** function used by both GenerateInitialEntities and Births to create the people in the population */
        void GenerateEntities(const DemographicProfile &profile, unsigned long numInProfileToCreate,
                              AgeRange *ageRange);

        /**
         * @param _gender gender of person we want to create
         * @param _ageBucketParams	parameters that determine a prevalent person's characteristics.
         * If this is nullptr, then this method will create a newborn
         * @return a newly formed person */
        Entity *GenerateEntity(EventParams &_eventParams, const DemographicProfile &profile,
                               Age age, bool toTrace);

        /** helper funtions for ApplyIncidentPrevalence() */
        void prevalentInfectionsFromCoefficients();

        int infectSeedPopulation(std::vector<Entity *> seedList, std::size_t seedPopulation);

        void prevalentInfectionsFromCount();

        void applyPrevalentInfection(Entity *p);

        bool rollForChronicInfection(RandomNumberGenerator &_randomNums);

        /**
         *processes the death of 1 person, updates statistics, removes that person from any relationships
         *@param _deceased pointer to deceased person */
        void ProcessDeath(EventParams &_eventParams, Entity *_p, bool calculateLE);

        void DetermineRankings(const RolloutEligibility &criteria);

        /**
         * calculates the number of HIV cases for each sexually active BucketDemographicProfile
         * and stores it in _infectionsTracker */
        std::size_t CalcPrevalentPopulation(Time time);

        /**
         * this is called at the end of each method that affects the population members
         * prints out trace information such as the size of each BucketDemographicProfile
         *
         * @param _d debug level that we should print at
         * @param _totalAffected the number of people affected by the most recent events
         * @param _totalAffectedLabel a label that identifies the meaning behind the value _totalAffected
         * @param _showInfected if true, will indicate how many people are currently infected in each BucketDemographicProfile */
        void
        PrintMethodResults(EventParams &_eventParams, const std::string &_methodName, const std::string &_eventLabel,
                           long _totalAffected,
                           const std::string &_totalAffectedLabel, bool _showInfections);

        /** this saves the state of the population and writes to file  */
        void SaveState(std::ostream &_outStream, Time currTime);

        /**
         * this is called at the end of each month to print the statistics about each population to the Population.out file
         *
         * @param _time the current time in the simulation
         * @param _outStream the stream to print */
        void PrintPopulationHeaders(Time _time, std::ostream &_outStream);

        void PrintPopulation(EventParams &_eventParams, Time _time, std::ostream &_outStream);

        /**
         * This is called at end of each month to print statistics about the behavior of the population to the
         * Behavior.out file */
        void PrintPartnerships(EventParams &_eventParams, Time _time, std::ostream &_outStream);

        /**
         * This is called at end of each month to print statistics about the clinical status of the population to
         * the Clinical.out file */
        void PrintClinical(EventParams &_eventParams, Time _time, std::ostream &_outStream);

        void PrintARTRolloutOutcomes(EventParams &_eventParams, std::ostream &_outStream);

        void PrintPrepOutcomes(EventParams &_eventParams, std::ostream &_outStream);

        /** This is called at specified time points to record the partner frequency */
        void RecordPartAcqFreq();

        void RecordShiftedOutcomes(EventParams &_eventParams, std::ostream &_outStream);

        void ResetPartnershipTracking();

        void RecordPartnership(const Entity *initiator, const Entity *partner);

        void PrintPartnershipTracking(std::ostream &_outStream, Time currTime);

        void RecordInfection(const Entity *infectee, const Entity *infector, Time infection_time);

        void OnRiskGroupChanged(const Entity *entity);

        using ChangedRiskGroupEventHandler = std::function<void(const Entity *)>;
        std::vector<ChangedRiskGroupEventHandler> risk_group_changed_;

    private:

        /** This is used to assign each New population a unique id */
        static unsigned int idCounter;

        /** this number is used to access the Population stratified parameters for Male and Female */
        unsigned int populationID;

        /** name of population */
        std::string populationLabel;

        /** current size of the population */
        std::size_t currSize;

        /** Size of non-sexually active */
        std::size_t currNASize;

        /** Size of CSW's */
        std::size_t currCSWSize;

        /** Size by Risk */
        RiskArray currSizeRisk;

        /** Size of CSW's by Risk */
        RiskArray currSizeRiskCSW;

        /** Size of CSW's by Risk and gender */
        std::unordered_map<DemographicProfile::ProfileID, RiskArray> currSizeProfileRiskCSW;

        /** Size by gender */
        std::size_t currSizeGender[(std::size_t) DemographicProfile::Gender::Last];

        /** Size by entity type. */
        std::unordered_map<DemographicProfile::ProfileID, std::size_t> currSizeProfile;

        /** Non-sexually active by gender */
        std::unordered_map<DemographicProfile::ProfileID, std::size_t> currNASizeByProfile;

        /** Sexually active by risk and gender */
        std::unordered_map<DemographicProfile::ProfileID, RiskArray> currSASizeProfileRisk;

        /** Births this month */
        std::size_t currBirths;

        /** Num Died this month by Death Cause */
        std::size_t currDeathCauses[(std::size_t) DeathStatus::Last];

        std::unordered_map<DemographicProfile::ProfileID, AgeRangeSizeContainer> currSizeByProfileAgeRange;

        /** The people who are infected but still untreated (Only used for rollout) */
        std::list<Entity *> rolloutUntreatedPool;

        /** The people who are currently being treated (Only used for rollout) */
        std::list<Entity *> rolloutTreatedPool;

        std::size_t num_circumcised_na;
        std::size_t num_circumcised_sa;

        PopulationParameters popWideParams;

        /**
         * A container for all the people. This is a compartmentalized container that lets us
         * access different types of people based on criteria. It also has an iterator that lets
         * us access all the through a java style iterator interface */
        std::unique_ptr<EntityPool> entities;

        /** List of available demographic profile generated by EntityPool */
        std::vector<DemographicProfile::ProfileID> demographicProfileIDs;

        /** Sexual Partnership Types allowed by the parameters mapped to profile IDs */
        std::map<DemographicProfile::ProfileID, std::vector<SexualPartnership::Type>> profilesToPartnershipTypes;

        std::array<std::vector<Entity *>, 5> rankedForTreatment;

        /** tallies the statistics that the population generates throughout the simulation */
        PopulationStatisticsOld populationStatistics;

        EventParams &parameters_;

        double treatmentCorrectionFactor_;

        std::unordered_set<Entity *> dead_people_this_month_;

        std::vector<Intervention> interventions_;

    public:
        struct EntitySummary {
            unsigned long person_id;
            Time time_infected;
            int infection_number;
            int generation_number;
            int infected_by;
            Age age_at_infection;
            Time time_of_death;
            DemographicProfile profile;
            RiskLevel risk_group;
        };



    private:
        std::unordered_map<unsigned long, EntitySummary> individual_summaries_;

        static const int NumIndividualSummaries = 10000;

        void SaveIndividualSummaries(std::ostream &stream) const;

        std::unordered_map<std::string, MonthlyStats> trace_files_;

        std::size_t debug_num_on_prep_;

        std::map<DemographicProfile::ProfileID, std::map<DemographicProfile::ProfileID, unsigned long>>
                newPartnershipCountByProfile;
    };

} // namespace transm


#endif /* POPULATION_HPP */