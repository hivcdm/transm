#pragma once

#include "./util/ticpp/ticpp.h"
#include "./data/EventParams.h"
#if !defined(CONSOLE)
#include "./gui/DisplayBox.h"
#endif
#include "./statistics/PopStats.h"

class Population;
class InfectionsTracker;

class Sim
{
private:
    //-----------------< Start data fields >--------------------//

    //TODO: Does it really make sense to use long instead of int here?
    /** current time in the simulation */
    long currTime;

    /** time to end simulation */
    long maxTime;

    /**number of months to run this file in a sequence*/
    long seqRunTime;

    /** pointer to current population */
    Population *currPopulation;

    /** housekeeping parameters that are universal to each event in the simulation */
    EventParams eventParams;

    /** True if there was an error parsing the XML input file */
    bool XMLerror;					//

    //-----------------< End data fields >--------------------//

    //Returns true if all simContexts loaded correctly
    bool setCEPACSimContexts(ticpp::Element *cepacInterventionNode);
    bool setRolloutSimContexts(ticpp::Element *rolloutInterventionNode);

    //Sets the Non aids death from a cepac simcontext
    void setNonAidsDeathFromCepac(SimContext *cepacSimContext, std::vector<double> &_maleProbs , std::vector<double> &_femaleProbs);

    long timeStep();		//perform one timestep of simulation
    bool loadNextInput();  //loads the next set of input files if seq: returns false if no next input
    bool isSeq; //determines whether this simulation is a sequence of .xml files 
    long seqPos; //position in sequence
    long numInSeq; //number of total files in sequence
    long delayPrevalence;//number of months to delay application of initial prevalence inputs

public:
    void run(int _numSteps);

    bool getError();				//returns XMLerror

    long getMaxTime();				//returns this->maxTime

    RunStats* getCEPACRunStats();	//returns this->eventParams.cepacRunStats for adding to the general popstats

    PopStats* getPopStats(); //returns this->population->popStats information for creating popStats-like file for transmission output

    EventParams *getEventParams();

    Sim(string _paramsXML,
#if !defined( CONSOLE )
	DisplayBox *dbox,
#endif
	bool _genGraphViz = false, bool useFixedSeed = false);		//creates a simulation object

    ~Sim(void);
};

