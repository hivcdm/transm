/*
 * main.cpp
 *
 *  Created on: Sep 22, 2010
 *      Author: errhode
 */

#include "cepac/include.h"
#include "util/Util.h"
#include "statistics/TransmissionSummaryStats.h"
#include "Sim.h"


/* Main function for a console based application */
int main(int argc, char *argv[])
{
    /* Console application uses the directory given on the command line or the
       current directory if none is given (like the old model) */
    if (argc > 1)
    {
	CepacUtil::inputsDirectory = argv[1];
	CepacUtil::changeDirectoryToInputs();
	//Call this so that relative directories can be used as input (i.e. "../")
	CepacUtil::useCurrentDirectoryForInputs();
    }
    else
    {
	CepacUtil::useCurrentDirectoryForInputs();
    }

    Util::findInputFiles();

    CepacUtil::createResultsDirectory();
    SummaryStats *cepacSummaryStats = new SummaryStats("cepacPopstats.out");
    TransmissionSummaryStats *transSummaryStats = new TransmissionSummaryStats("summaryStats.out");

    for (int i = 0; i < Util::transmFilesToRun.size(); i++)
    {
	//Changing back to the input directory because over the course of Sim->run, the directory gets changed to results
	CepacUtil::changeDirectoryToInputs();
	std::cout << "Running File: " << Util::transmFilesToRun[i] << std::endl;
	//Console version will not use GraphViz and will use random seed by result
	Sim *s = new Sim(Util::transmFilesToRun[i], false, false);
	if (!(s->getError()))
	{
	    //Run the simulation the desired number of time steps
	    s->run(s->getMaxTime());

	    //Get CEPAC runStats from eventsParams and add to cepacSummaryStats
	    cepacSummaryStats->addRunStats(s->getCEPACRunStats());

	    //Get transmission popStats and add to transSummaryStats
	    transSummaryStats->addPopStats(s->getPopStats(), s->getEventParams());

	    delete s;
	}
    }

    //Finalize CEPAC summary stats and print the popstats file
    cepacSummaryStats->finalizeStats();
    try
    {
	cepacSummaryStats->writeSummariesFile();
	transSummaryStats->writeSummariesFile();
    }
    catch (std::string errorString)
    {
	cout << errorString << "\n";
    }

    delete cepacSummaryStats;
    delete transSummaryStats;
}
