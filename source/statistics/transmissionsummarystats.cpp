#include "transmissionsummarystats.hpp"
#include "populationstatisticsold.hpp"
#include "parameters/eventparams.hpp"

namespace transm {

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
	for(std::vector<PopulationStatisticsOld::SingleTimeStats *>::iterator iter = selectedSummaryStats->begin();
	        iter != selectedSummaryStats->end(); iter++)
	{
		PopulationStatisticsOld::SingleTimeStats *singleTimeStat = *iter;
		delete singleTimeStat;
	}

	selectedSummaryStats->clear();
	delete selectedSummaryStats;
}

/* addRunStats adds a new summary to the vector from a RunStats object */
void TransmissionSummaryStats::addPopulationStatistics(PopulationStatisticsOld &popStats, EventParams &eventParams)
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
	summary->selectedSummaryStats = new std::vector<PopulationStatisticsOld::SingleTimeStats *>();
	auto time = popStats.getNextTimeToRecord(Time::Zero);
	std::vector<PopulationStatisticsOld::SingleTimeStats *>::iterator statsIterator = popStats.getSelectedSummaryStats()->begin();

	while(time > Time::Zero && statsIterator != popStats.getSelectedSummaryStats()->end())
	{
		while(statsIterator != popStats.getSelectedSummaryStats()->end() && (*statsIterator)->timeOfStats < time)
		{
			statsIterator++;
		}

		if(statsIterator != popStats.getSelectedSummaryStats()->end())
		{
			summary->timeToRecord[summary->selectedSummaryStats->size()] = time;
			summary->selectedSummaryStats->push_back(*statsIterator);
			time = popStats.getNextTimeToRecord(time + TimeSpan::Month);
		}
	}

	summary->LMsAverage = popStats.lifeStats->getStat(PopulationStatisticsOld::TOTAL_LM) / (popStats.lifeStats->getStat(
	                          PopulationStatisticsOld::TOTAL_HIV_POS) + popStats.lifeStats->getStat(PopulationStatisticsOld::TOTAL_HIV_NEG));
	summary->HIVPosLMAverage = popStats.lifeStats->getStat(PopulationStatisticsOld::TOTAL_HIV_POS_LM) / popStats.lifeStats->getStat(
	                               PopulationStatisticsOld::TOTAL_HIV_POS) ;
	summary->HIVNegLMAverage = popStats.lifeStats->getStat(PopulationStatisticsOld::TOTAL_HIV_NEG_LM) / popStats.lifeStats->getStat(
	                               PopulationStatisticsOld::TOTAL_HIV_NEG);
	summary->HIVPosSurvivalAverage = popStats.lifeStats->getStat(PopulationStatisticsOld::TOTAL_HIV_POS_POSTINFECT_LM) /
	                                 popStats.lifeStats->getStat(PopulationStatisticsOld::TOTAL_HIV_POS) ;
	//TODO: Fix me!
	summary->AverageNumberOfPeopleEachPersonInfects = 1;
	// Add the new summary to the summaries vector
	summaries.push_back(summary);
} /* end addRunStats */

/* writeSummariesFile appends the summary information to the popstats.out file */
void TransmissionSummaryStats::writeSummariesFile()
{
	// Open the popstats file and write header if needed
	Utility::changeDirectoryToResults();
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
			PopulationStatisticsOld::SingleTimeStats *singleTimeStat = summary->selectedSummaryStats->at(j);
			summaryStatsStream << singleTimeStat->timeOfStats.in_months() << "\t";
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
}

} // namespace transm
