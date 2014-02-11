#pragma once

#include "data/EventParams.h"
#include "statistics/PopStats.h"
#include "util/ticpp/ticpp.h"

class Population;
class InfectionsTracker;

struct TimeDependentParameter
{
	int time;
	std::string key;
	std::string value;
};

class Simulation
{
public:
	Simulation(const std::string &xmlFile);		//creates a simulation object

	~Simulation();

	void Initialize();

	bool Step();

	RunStats *GetCEPACRunStats();	//returns this->eventParams.cepacRunStats for adding to the general popstats

	PopStats *GetPopStats(); //returns this->population->popStats information for creating popStats-like file for transmission output

	EventParams *GetEventParams();

	double GetPrevalence() { return prevalence_; }

	double GetIncidence() { return incidence_; }

	int GetTotalTime() { return totalTime_; }

	int GetTime() { return time_; }

	void SetMessageCallback(const std::function<void(const std::string &)> &callback) { parameters_.messageCallback = callback; }

private:
	void FirstStep();

	void LastStep();

	/** Returns true if all simContexts loaded correctly */
	bool SetCEPACSimContexts(ticpp::Element *cepacInterventionNode);

	/** */
	bool SetRolloutSimContexts(ticpp::Element *rolloutInterventionNode);

	/** Sets the Non aids death from a cepac simcontext */
	void SetNonAidsDeathFromCepac(SimContext &context, std::vector<double> &male, std::vector<double> &female);

	/** perform one timestep of simulation */
	int SimulateMonth();

	/** loads the next set of input files if seq: returns false if no next input */
	void LoadInput(const std::string &xmlFile);

	void LoadTimeDependentParameters(ticpp::Element *timeDependentParametersElement);

	void UpdateTimeDependentParameters();

	const std::string xmlFile_;

	/** current time in the simulation */
	int time_;

	/** number of months to run this file in a sequence*/
	int duration_;

	/** pointer to current population */
	Population *population_;

	/** housekeeping parameters that are universal to each event in the simulation */
	EventParams parameters_;

	/** number of months to delay application of initial prevalence inputs */
	int prevalenceDelay_;

	bool failedCalibration_;

	bool hasPassedFirstMonthCalibPrev_;

	int monthOfFirstMonthCalibPrev_;

	int totalTime_;

	double incidence_;

	double prevalence_;

	std::vector<TimeDependentParameter> timeDependentParameters_;
};

