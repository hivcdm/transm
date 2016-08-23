#pragma once

#include <unordered_set>
#include <pugixml.hpp>

#include "include.h"

#include "batchstatus.hpp"
#include "intervention.hpp"
#include "population.hpp"
#include "targetgroup.hpp"
#include "parameters/eventparams.hpp"
#include "parameters/simulationparameters.hpp"
#include "statistics/outputs.hpp"
#include "statistics/populationstatistics.hpp"
#include "utility/highresolutiontimer.hpp"
#include "utility/nullable.hpp"
#include "utility/runtimepredictor.hpp"

namespace transm {

class InfectionsTracker;

class Simulation
{
public:
	Simulation(BatchStatus &batch_status);

	~Simulation();

    void Initialize(SimulationParameters &parameters);

	Outputs Run();

	//returns eventParams.cepacRunStats for adding to the general popstats
	RunStats &GetCEPACRunStats();

	//returns eventParams.cepacCostStats for adding to the general popstats
	CostStats &GetCEPACCostStats();

	//returns population->popStats information for creating popStats-like file for transmission output
	PopulationStatisticsOld &GetPopulationStatistics();

	EventParams &GetEventParams();

	double GetPrevalence() { return prevalence_; }

	double GetIncidence() { return incidence_; }

	Time GetTime() { return time_; }

	void RegisterTargetGroup(const TargetGroup &group);

	Population &GetPopulation() { return population_; }
    const Population &GetPopulation() const { return population_; }

	void SetDuration(TimeSpan duration) { duration_ = duration; }

	void SetFixedSeed(int seed);

	void SetName(const std::string &name) 
    { 
        name_ = name; 
        parameters_.simName = name; 
        batch_status_.set_state(name_, SimState::queued);
    }

	void AddLifeExpectancyRecordTime(Time time) { population_.populationStatistics.addLifeExpectancyRecordTime(time); }

    void SetLifeExpectancyConfidenceInterval(double ci) { population_.populationStatistics.setMedianLECI(ci); }

	void AddPartnerAcquisitionRecordTime(Time time) { population_.populationStatistics.addPartnerAcquisitionRecordTime(time); }

    void RegisterPopulationIntervention(const Intervention &intervention);

private:
	friend class SimulationBuilderXml;
    friend class Intervention;

	struct TreatmentFile
	{
		std::string file_name;
		int file_number;
		Time time;
		int target_population;
	};

    using TreatmentFiles = std::vector<TreatmentFile>;

	void FirstStep();

	void LastStep();

	void Step();

	/** Returns true if all simContexts loaded correctly */
	bool LoadCepacSimContexts(const TreatmentFiles &treatment_files);

	/** */
	bool LoadRolloutSimContexts(const TreatmentFiles &treatment_files);

	/** Sets the Non aids death from a cepac simcontext */
	void SetNonAidsDeathFromCepac(SimContext &context, std::vector<double> &male, std::vector<double> &female);

	/** perform one timestep of simulation */
    std::size_t SimulateMonth();

    void UpdateInterventions(const std::unordered_set<Entity *> &dead_people);

	std::string name_;

	/** current time in the simulation */
	Time time_;

	/** number of months to run this file in a sequence*/
	TimeSpan duration_;

	/** housekeeping parameters that are universal to each event in the simulation */
	EventParams parameters_;

	/** current population */
	Population population_;

	bool failedCalibration_;

	bool hasPassedFirstMonthCalibPrev_;

	Time monthOfFirstMonthCalibPrev_;

	uint32_t rng_seed_;

	double incidence_;

	double prevalence_;

	HighResolutionTimer timer_;

	Outputs outputs_;

	TreatmentFiles cepac_treatment_files_;

	TreatmentFiles rollout_treatment_files_;

    std::vector<TargetGroup> groups_;

    std::vector<Intervention> interventions_;

    std::vector<std::pair<int, double>> population_size_;

    RunTimePredictor run_time_predictor_;

    double start_time_;

    BatchStatus &batch_status_;
};

} // namespace transm
