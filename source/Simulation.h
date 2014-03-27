#pragma once

#include "Inputs.h"
#include "Outputs.h"
#include "data/EventParams.h"
#include "statistics/PopStats.h"
#include "util/HighResolutionTimer.h"
#include "util/ticpp/ticpp.h"

class InfectionsTracker;
class Population;

class Simulation
{
public:
	typedef std::function<void(const std::string &)> MessageCallback;

	//creates a simulation object
	Simulation(const Inputs &inputs, MessageCallback message_callback);

	~Simulation();

	Outputs Run();

	//returns eventParams.cepacRunStats for adding to the general popstats
	RunStats *GetCEPACRunStats();

	//returns population->popStats information for creating popStats-like file for transmission output
	PopStats *GetPopStats();

	EventParams *GetEventParams();

	double GetPrevalence() { return prevalence_; }

	double GetIncidence() { return incidence_; }

	int GetTotalTime() { return totalTime_; }

	int GetTime() { return time_; }

private:
	void FirstStep();

	void LastStep();

	void Step();

	/** Returns true if all simContexts loaded correctly */
	bool LoadCepacSimContexts(const Interventions::CepacTreatmentFiles &treatment_files);

	/** */
	bool LoadRolloutSimContexts(const Interventions::RolloutTreatmentFiles &treatment_files);

	/** Sets the Non aids death from a cepac simcontext */
	void SetNonAidsDeathFromCepac(SimContext &context, std::vector<double> &male, std::vector<double> &female);

	/** perform one timestep of simulation */
	int SimulateMonth();

	void UpdateTimeDependentParameters();

	const Inputs &inputs_;

	const std::string xmlFile_;

	/** current time in the simulation */
	int time_;

	/** number of months to run this file in a sequence*/
	int duration_;

	/** pointer to current population */
	Population *population_;

	/** housekeeping parameters that are universal to each event in the simulation */
	EventParams parameters_;

	bool failedCalibration_;

	bool hasPassedFirstMonthCalibPrev_;

	int monthOfFirstMonthCalibPrev_;

	int totalTime_;

	double incidence_;

	double prevalence_;

	HighResolutionTimer timer_;

	Outputs outputs_;
};

