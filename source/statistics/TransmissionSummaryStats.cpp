#include "TransmissionSummaryStats.h"
#include "PopulationStatistics.h"
#include "../data/EventParams.h"

/* Constructor takes summariesFileName as input, clears summaries vector */
TransmissionSummaryStats::TransmissionSummaryStats(const std::string &summariesFileName)
{
	this->summariesFileName = summariesFileName;
	summaries.clear();
}

/* Destructor frees allocated Summary objects and clears summaries vector */
TransmissionSummaryStats::~TransmissionSummaryStats(void)
{
	for(std::vector<TransmissionSummary *>::iterator j = summaries.begin(); j != summaries.end(); j++)
	{
		TransmissionSummary *summary = *j;
		delete summary;
	}

	summaries.clear();
}

TransmissionSummaryStats::TransmissionSummary::~TransmissionSummary()
{
	//std::vector<PopulationStatistics::SingleTimeStats*>* selectedSummaryStats;
	for(std::vector<PopulationStatistics::SingleTimeStats *>::iterator iter = selectedSummaryStats->begin();
	        iter != selectedSummaryStats->end(); iter++)
	{
		PopulationStatistics::SingleTimeStats *singleTimeStat = *iter;
		delete singleTimeStat;
	}

	selectedSummaryStats->clear();
	delete selectedSummaryStats;
}

/* addRunStats adds a new summary to the vector from a RunStats object */
void TransmissionSummaryStats::addPopulationStatistics(PopulationStatistics &popStats, EventParams &eventParams)
{
	/* Create a new summary object */
	TransmissionSummary *summary = new TransmissionSummary();
	/* Copy the population summary stats */
	//const RunStats::PopulationSummary *popSummary = runStats->getPopulationSummary();
	//TODO: This doesn't mean anything for now...
	summary->runSetName = eventParams.simName;
	summary->runName = eventParams.simName;
	//summary->runDate = eventParams.
	//summary->runTime = popSummary->runTime;
	//summary->numCohorts = popSummary->numCohorts;
	summary->selectedSummaryStats = new std::vector<PopulationStatistics::SingleTimeStats *>();
	int time = popStats.getNextTimeToRecord(0);
	std::vector<PopulationStatistics::SingleTimeStats *>::iterator statsIterator = popStats.getSelectedSummaryStats()->begin();

	while(time > 0 && statsIterator != popStats.getSelectedSummaryStats()->end())
	{
		while(statsIterator != popStats.getSelectedSummaryStats()->end() && (*statsIterator)->timeOfStats < time)
		{
			statsIterator++;
		}

		if(statsIterator != popStats.getSelectedSummaryStats()->end())
		{
			summary->timeToRecord[summary->selectedSummaryStats->size()] = time;
			summary->selectedSummaryStats->push_back(*statsIterator);
			time = popStats.getNextTimeToRecord(time + 1);
		}
	}

	summary->LMsAverage = popStats.lifeStats->getStat(PopulationStatistics::TOTAL_LM) / (popStats.lifeStats->getStat(
	                          PopulationStatistics::TOTAL_HIV_POS) + popStats.lifeStats->getStat(PopulationStatistics::TOTAL_HIV_NEG));
	summary->HIVPosLMAverage = popStats.lifeStats->getStat(PopulationStatistics::TOTAL_HIV_POS_LM) / popStats.lifeStats->getStat(
	                               PopulationStatistics::TOTAL_HIV_POS) ;
	summary->HIVNegLMAverage = popStats.lifeStats->getStat(PopulationStatistics::TOTAL_HIV_NEG_LM) / popStats.lifeStats->getStat(
	                               PopulationStatistics::TOTAL_HIV_NEG);
	summary->HIVPosSurvivalAverage = popStats.lifeStats->getStat(PopulationStatistics::TOTAL_HIV_POS_POSTINFECT_LM) /
	                                 popStats.lifeStats->getStat(PopulationStatistics::TOTAL_HIV_POS) ;
	//TODO: Fix me!
	summary->AverageNumberOfPeopleEachPersonInfects = 1;
	// Add the new summary to the summaries vector
	summaries.push_back(summary);
} /* end addRunStats */

/* writeSummariesFile appends the summary information to the popstats.out file */
void TransmissionSummaryStats::writeSummariesFile()
{
	// Open the popstats file and write header if needed
	CepacUtil::changeDirectoryToResults();
	summaryStatsStream.open(summariesFileName.c_str(), ios::out | ios::app);
	writeSummariesFileHeader();

	// Loop over the individual run summaries of the summaries vector
	for(vector<TransmissionSummary *>::iterator i = summaries.begin(); i != summaries.end(); i++)
	{
		TransmissionSummary *summary = *i;
		summaryStatsStream << summary->runName << "\t";
		summaryStatsStream << summary->LMsAverage << "\t";
		summaryStatsStream << summary->HIVPosLMAverage << "\t";
		summaryStatsStream << summary->HIVNegLMAverage << "\t";
		summaryStatsStream << summary->HIVPosSurvivalAverage << "\t";
		summaryStatsStream << summary->AverageNumberOfPeopleEachPersonInfects << "\t";

		for(size_t j = 0; j < summary->selectedSummaryStats->size(); j++)
		{
			PopulationStatistics::SingleTimeStats *singleTimeStat = summary->selectedSummaryStats->at(j);
			summaryStatsStream << singleTimeStat->timeOfStats << "\t";
			summaryStatsStream << singleTimeStat->prevalence << "\t";
			summaryStatsStream << singleTimeStat->SAprevalence << "\t";
			summaryStatsStream << singleTimeStat->incidence << "\t";
			summaryStatsStream << singleTimeStat->cumulativeNumberDead << "\t";
		}

		summaryStatsStream << std::endl;
	}

	summaryStatsStream.close();
} /* end writeSummariesFile */

/* writes out summaries file header */
void TransmissionSummaryStats::writeSummariesFileHeader()
{
	int i;
	summaryStatsStream << "RunName\t Average LM \t HIV+ LM\t HIV- LM\t HIV+ Survival\t R_0\t";

	for(i = 0; i < NUM_TIMES_TO_RECORD; i++)
	{
		summaryStatsStream << "Time\t Prevalence\t SA Prevalence\t Incidence\t Cumulative No. Dead\t";
	}

	summaryStatsStream << std::endl;
} /* end writeSummariesFileHeader */
