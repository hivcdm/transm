#pragma once

#include "data/EventParams.h"
#include "statistics/PopStats.h"
#include "util/ticpp/ticpp.h"

class Population;
class InfectionsTracker;

class Sim
{
public:
	Sim(const std::string &xmlFile);		//creates a simulation object

	~Sim();

	void Initialize();

	bool Step();

	RunStats *GetCEPACRunStats();	//returns this->eventParams.cepacRunStats for adding to the general popstats

	PopStats *GetPopStats(); //returns this->population->popStats information for creating popStats-like file for transmission output

	EventParams *GetEventParams();

	double GetPrevalence() { return prevalence_; }

	double GetIncidence() { return incidence_; }

	int GetTotalTime() { return totalTime_; }

	int GetTime() { return time_; }

private:
	void FirstStep();

	void LastStep();

	/** Returns true if all simContexts loaded correctly */
	bool SetCEPACSimContexts(ticpp::Element *cepacInterventionNode);

	/** */
	bool SetRolloutSimContexts(ticpp::Element *rolloutInterventionNode);

	/** Sets the Non aids death from a cepac simcontext */
	void SetNonAidsDeathFromCepac(SimContext *cepacSimContext, std::vector<double> &_maleProbs ,
	                              std::vector<double> &_femaleProbs);

	/** perform one timestep of simulation */
	int SimulateMonth();

	/** Load new eligibility when rollout sim context changes */
	void UpdateEligibility(ticpp::Element *rolloutInterventionNode);

	/** loads the next set of input files if seq: returns false if no next input */
	bool LoadInput(const std::string &xmlFile);

	const std::string xmlFile_;

	/** current time in the simulation */
	int time_;

	/** number of months to run this file in a sequence*/
	int duration_;

	/** pointer to current population */
	Population *population_;

	/** housekeeping parameters that are universal to each event in the simulation */
	EventParams parameters_;

	/** determines whether this simulation is a sequence of .xml files */
	bool isSequence_;

	/** position in sequence */
	int sequencePosition_;

	/** number of total files in sequence */
	int numberInSequence_;

	/** number of months to delay application of initial prevalence inputs */
	int prevalenceDelay_;

	bool failedCalibration_;

	bool hasPassedFirstMonthCalibPrev_;

	int monthOfFirstMonthCalibPrev_;

	int totalTime_;

	double incidence_;

	double prevalence_;
};

