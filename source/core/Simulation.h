#pragma once

#include <unordered_set>

#include "Outputs.h"
#include "Population.h"
#include "../data/EventParams.h"
#include "../statistics/PopulationStatistics.h"
#include "../util/HighResolutionTimer.h"
#include "../util/Nullable.h"
#include "../util/xml/pugixml.hpp"

class InfectionsTracker;

class SimulationIntervention
{
public:
    SimulationIntervention(const std::string &catgory, const std::string &subcategory);
    void Apply(Simulation &s);
};

class PopulationIntervention
{
public:
    PopulationIntervention(const std::string &catgory, const std::string &subcategory);
    void Apply(Population &p);
};

class IndividualIntervention
{
public:
    IndividualIntervention(const std::string &catgory, const std::string &subcategory);
    void Apply(Person *p);
};

class TargetGroup
{
public:
    struct PopulationTarget
    {
        static PopulationTarget FromString(const std::string &s);
        static PopulationTarget Any;

        Nullable<Person::RiskLevel> risk_level;
        Nullable<DemographicProfile::Employment> employment;
        Nullable<DemographicProfile::SexualActivityStatus> sexual_activity_status;
        Nullable<DemographicProfile::Gender> gender;
        Nullable<DemographicProfile::RelationshipStatus> relationship_status;
        Nullable<DemographicProfile::SexualOrientation> sexual_orientation;
        Nullable<int> age_lower;
        Nullable<int> age_upper;
        Nullable<Person::HIVStatus> observed_hiv_status;
        Nullable<bool> on_treatment;

        bool operator==(const PopulationTarget &other) const
        {
            return risk_level == other.risk_level &&
                employment == other.employment &&
                sexual_activity_status == other.sexual_activity_status &&
                gender == other.gender &&
                relationship_status == other.relationship_status &&
                sexual_orientation == other.sexual_orientation &&
                age_lower == other.age_lower &&
                age_upper == other.age_upper &&
                observed_hiv_status == other.observed_hiv_status &&
                on_treatment == other.on_treatment;
        }

        bool operator!=(const PopulationTarget &other) const { return !(*this == other); }
    };

    TargetGroup(int start, int end, bool open, bool permanent, Nullable<PopulationTarget> target);

    void Update(int simulation_time, const std::unordered_set<Person *> &new_people);

    void AddPartition(const std::string &label, bool trace, double proportion, 
        std::vector<SimulationIntervention> simulation_interventions, 
        std::vector<PopulationIntervention> population_interventions,
        std::vector<IndividualIntervention> individual_interventions);

private:
    struct
    {
        int start;
        int end;
    } enrollment_period_;

    bool open_;
    bool permanent_effect_;

    class Partition
    {
    public:
        double GetProportion() const { return proportion_; }
        void Add(Person *p) { members_.insert(p); }
    private:
        friend class TargetGroup;
        std::unordered_set<Person *> members_;
        std::string label_;
        bool trace_;
        double proportion_;
        std::vector<SimulationIntervention> simulation_interventions_;
        std::vector<PopulationIntervention> population_interventions_;
        std::vector<IndividualIntervention> individual_interventions_;
    };

    std::vector<Partition> partitions_;

    Nullable<PopulationTarget> target_;
};

class Simulation
{
public:
	typedef std::function<void(const std::string &)> MessageCallback;

	Simulation();

	~Simulation();

	Outputs Run(MessageCallback message_callback);

	//returns eventParams.cepacRunStats for adding to the general popstats
	RunStats &GetCEPACRunStats();

	//returns population->popStats information for creating popStats-like file for transmission output
	PopulationStatistics &GetPopulationStatistics();

	EventParams &GetEventParams();

	double GetPrevalence() { return prevalence_; }

	double GetIncidence() { return incidence_; }

	int GetTotalTime() { return totalTime_; }

	int GetTime() { return time_; }

	void RegisterTargetGroup(const std::string &group_label, TargetGroup &group);

	Population &GetPopulation() { return population_; }
    const Population &GetPopulation() const { return population_; }

	void SetDuration(int duration) { duration_ = duration; }

	void SetFixedSeed(int seed);

	void SetName(const std::string &name) { name_ = name; parameters_.simName = name; }

	void AddLifeExpectancyRecordTime(int time) { population_.populationStatistics.addLifeExpectancyRecordTime(time); }

private:
	friend class SimulationBuilder;

	struct TreatmentFile
	{
		std::string file_name;
		int file_number;
		int time;
		int target_population;
	};

	typedef std::array<TreatmentFile, Constants::NUMBER_OF_CEPAC_FILES> CepacTreatmentFiles;
	typedef std::array<TreatmentFile, Constants::NUMBER_OF_ROLLOUT_FILES> RolloutTreatmentFiles;

	void FirstStep();

	void LastStep();

	void Step();

	/** Returns true if all simContexts loaded correctly */
	bool LoadCepacSimContexts(const CepacTreatmentFiles &treatment_files);

	/** */
	bool LoadRolloutSimContexts(const RolloutTreatmentFiles &treatment_files);

	/** Sets the Non aids death from a cepac simcontext */
	void SetNonAidsDeathFromCepac(SimContext &context, std::vector<double> &male, std::vector<double> &female);

	/** perform one timestep of simulation */
    std::size_t SimulateMonth();

    void UpdateGroups(const std::unordered_set<Person *> &new_people);

	std::string name_;

	/** current time in the simulation */
	int time_;

	/** number of months to run this file in a sequence*/
	int duration_;

	/** housekeeping parameters that are universal to each event in the simulation */
	EventParams parameters_;

	/** current population */
	Population population_;

	bool failedCalibration_;

	bool hasPassedFirstMonthCalibPrev_;

	int monthOfFirstMonthCalibPrev_;

	int totalTime_;

	int fixedSeed_;

	double incidence_;

	double prevalence_;

	HighResolutionTimer timer_;

	Outputs outputs_;

	CepacTreatmentFiles cepac_treatment_files_;

	RolloutTreatmentFiles rollout_treatment_files_;

    std::vector<TargetGroup> groups_;
};

