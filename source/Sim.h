#pragma once

#include "data/EventParams.h"
#ifndef CONSOLE
#include "gui/DisplayBox.h"
#endif
#include "statistics/PopStats.h"
#include "util/ticpp/ticpp.h"

class Population;
class InfectionsTracker;
class DisplayBox;

class Sim
{
public:
	Sim(std::string xmlFile, DisplayBox *dbox);		//creates a simulation object

	~Sim();

	void run(int numSteps);

	bool getError();				//returns XMLerror

	int getMaxTime();				//returns this->maxTime

	RunStats *getCEPACRunStats();	//returns this->eventParams.cepacRunStats for adding to the general popstats

	PopStats *getPopStats(); //returns this->population->popStats information for creating popStats-like file for transmission output

	EventParams *getEventParams();

private:
    /** Returns true if all simContexts loaded correctly */
    bool setCEPACSimContexts(ticpp::Element *cepacInterventionNode);

	/** */
    bool setRolloutSimContexts(ticpp::Element *rolloutInterventionNode);

    /** Sets the Non aids death from a cepac simcontext */
    void setNonAidsDeathFromCepac(SimContext *cepacSimContext, std::vector<double> &_maleProbs , std::vector<double> &_femaleProbs);

	/** perform one timestep of simulation */
    int timeStep();

	/** loads the next set of input files if seq: returns false if no next input */
    bool loadNextInput();

	/** current time in the simulation */
	int currTime;

	/** time to end simulation */
	int maxTime;

	/** number of months to run this file in a sequence*/
	int seqRunTime;

	/** pointer to current population */
	Population *currPopulation;

	/** housekeeping parameters that are universal to each event in the simulation */
	EventParams eventParams;

	/** True if there was an error parsing the XML input file */
	bool XMLerror;

	/** determines whether this simulation is a sequence of .xml files */
    bool isSeq;

	/** position in sequence */
    int seqPos;

	/** number of total files in sequence */
    int numInSeq;

	/** number of months to delay application of initial prevalence inputs */
    int delayPrevalence;
};

