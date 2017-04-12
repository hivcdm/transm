#pragma once

#include <unordered_set>
#include <pugixml.hpp>

#include "include.h"
#include "Intervention.h"
#include "Outputs.h"
#include "Population.h"
#include "TargetGroup.h"
#include "data/EventParams.h"
#include "statistics/PopulationStatistics.h"
#include "utility/HighResolutionTimer.h"
#include "utility/Nullable.h"
#include "utility/RunTimePredictor.h"

class InfectionsTracker;

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

	void RegisterTargetGroup(const TargetGroup &group);

	Population &GetPopulation() { return population_; }
    const Population &GetPopulation() const { return population_; }

	void SetDuration(int duration) { duration_ = duration; }

	void SetFixedSeed(int seed);

	void SetName(const std::string &name) { name_ = name; parameters_.simName = name; }

	void AddLifeExpectancyRecordTime(int time) { population_.populationStatistics.addLifeExpectancyRecordTime(time); }

    void SetLifeExpectancyConfidenceInterval(double ci) { population_.populationStatistics.setMedianLECI(ci); }

	void AddPartnerAcquisitionRecordTime(int time) { population_.populationStatistics.addPartnerAcquisitionRecordTime(time); }

    void RegisterIntervention(const Intervention &intervention);

	struct TreatmentFile
	{
		std::string file_name;
		int file_number;
		int time;
		int target_population;
	};

private:
	friend class SimulationBuilderXml;
	friend class Intervention;

	void FirstStep();

	void LastStep();

	void Step();

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

	bool passedCalibration_;

	bool hasPassedFirstMonthCalibPrev_;

	int monthOfFirstMonthCalibPrev_;

	int totalTime_;

	int fixedSeed_;

	double incidence_;

	double prevalence_;

	HighResolutionTimer timer_;

	Outputs outputs_;

    std::vector<TargetGroup> groups_;

    std::vector<Intervention> interventions_;

    std::vector<std::pair<int, double>> population_size_;

    RunTimePredictor run_time_predictor_;

    double start_time_;
};

