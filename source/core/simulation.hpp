#pragma once

#include <unordered_set>
#include <pugixml.hpp>

#include "batchstatus.hpp"
#include "intervention.hpp"
#include "outputs.hpp"
#include "population.hpp"
#include "targetgroup.hpp"
#include "data/eventparams.hpp"
#include "statistics/populationstatistics.hpp"
#include "utility/highresolutiontimer.hpp"
#include "utility/nullable.hpp"
#include "utility/runtimepredictor.hpp"

class InfectionsTracker;

class Simulation
{
public:
	typedef std::function<void(const std::string &)> MessageCallback;

	Simulation(BatchStatus &batch_status);

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

	void RegisterTargetGroup(const TargetGroup &group);

	Population &GetPopulation() { return population_; }
    const Population &GetPopulation() const { return population_; }

	void SetDuration(int duration) { duration_ = duration; }

	void SetFixedSeed(int seed);

	void SetName(const std::string &name) 
    { 
        name_ = name; 
        parameters_.simName = name; 
        batch_status_.set_state(name_, SimState::queued);
    }

	void AddLifeExpectancyRecordTime(int time) { population_.populationStatistics.addLifeExpectancyRecordTime(time); }

    void SetLifeExpectancyConfidenceInterval(double ci) { population_.populationStatistics.setMedianLECI(ci); }

    void RegisterIntervention(const Intervention &intervention);

private:
	friend class SimulationBuilderXml;
    friend class Intervention;

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

    void UpdateInterventions(const std::unordered_set<Person *> &dead_people);

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

    std::vector<Intervention> interventions_;

    std::vector<std::pair<int, double>> population_size_;

    RunTimePredictor run_time_predictor_;

    double start_time_;

    BatchStatus &batch_status_;
};

