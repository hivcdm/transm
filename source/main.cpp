#include "cepac/include.h"
#include "util/Util.h"
#include "statistics/TransmissionSummaryStats.h"
#include "Sim.h"

int main(int argc, char *argv[])
{
	/* Console application uses the directory given on the command line or the
	   current directory if none is given (like the old model) */
	if(argc > 1)
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

	Util::findInputFiles(argv[1]);
	CepacUtil::createResultsDirectory();
	SummaryStats *cepacSummaryStats = new SummaryStats("cepacPopstats.out");
	TransmissionSummaryStats *transSummaryStats = new TransmissionSummaryStats("summaryStats.out");

	for(int i = 0; i < Util::transmFilesToRun.size(); i++)
	{
		//Changing back to the input directory because over the course of Sim->run, the directory gets changed to results
		CepacUtil::changeDirectoryToInputs();
		std::cout << "Running File: " << Util::transmFilesToRun[i] << std::endl;
		//Console version will not use GraphViz and will use random seed by result
		Sim s(Util::transmFilesToRun[i]);

		while(!s.GetEventParams()->outputMessageQueue.empty())
		{
			std::cout << s.GetEventParams()->outputMessageQueue.front();
			s.GetEventParams()->outputMessageQueue.pop_front();
		}

		s.Initialize();

		while(s.Step())
		{
			while(!s.GetEventParams()->outputMessageQueue.empty())
			{
				std::cout << s.GetEventParams()->outputMessageQueue.front();
				s.GetEventParams()->outputMessageQueue.pop_front();
			}
		}

		cepacSummaryStats->addRunStats(s.GetCEPACRunStats());
		transSummaryStats->addPopStats(s.GetPopStats(), s.GetEventParams());
	}

	//Finalize CEPAC summary stats and print the popstats file
	cepacSummaryStats->finalizeStats();

	try
	{
		cepacSummaryStats->writeSummariesFile();
		transSummaryStats->writeSummariesFile();
	}
	catch(std::string errorString)
	{
		cout << errorString << "\n";
	}

	delete cepacSummaryStats;
	delete transSummaryStats;
}
