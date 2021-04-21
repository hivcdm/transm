#pragma once

#include <fstream>

#include "populationstatisticsold.hpp"
#include "parameters/eventparams.hpp"

namespace transm {

/// <summary>
/// SummaryStats class contains a list of the summary statistics from each of the
/// input files (simulation contexts) that are executed in a given run of the model.
/// It contains the functions to generate a summary from a given RunStats object, add it to
/// the list of summaries, and output all the summaries to the popstats file.  Main calls these
/// functions to add each new summary and append to popstats.out at the end of the run.
/// </summary>
class TransmissionSummaryStats
{
public:
	static const int NUM_TIMES_TO_RECORD = 5;

	TransmissionSummaryStats(const std::string &summariesFileName);
	~TransmissionSummaryStats();

	/// <summary>
	/// Summary class stores the summary information that is written to the popstats file
	/// </summary>
	class TransmissionSummary
	{
	public:
		~TransmissionSummary();

		std::string runSetName;
		std::string runName;
		//string runDate;
		//string runTime;
		//int numCohorts;
		//TODO: Have structure for prevalence and incidence at 5 time points (maybe by default these are 1, 0.2*maxTime, 0.4*maxTime, etc?)
		//These times have to be defined!
		Time timeToRecord[NUM_TIMES_TO_RECORD];
		std::vector<PopulationStatisticsOld::SingleTimeStats *> *selectedSummaryStats;
		double LMsAverage;
		double HIVPosLMAverage;
		double HIVNegLMAverage;
		double HIVPosSurvivalAverage;
		double AverageNumberOfPeopleEachPersonInfects;
	};

	/* addRunStats adds a new summary to the vector from a RunStats object */
	void addPopulationStatistics(PopulationStatisticsOld &popStats, EventParams &eventParams);
	/* finalizeStats calculates the final cost-effectiveness ratios for each run */
	//void finalizeStats();
	/* writeSummariesFile appends the summary information to the popstats.out file */
	void writeSummariesFile();

private:
	/* vector of individual run Summary objects,
		uses Summary pointers since objects are large and copy is expensive */
	std::vector<TransmissionSummary *> summaries;

	/* summaries file name and file pointer */
	std::string summariesFileName;
	std::fstream summaryStatsStream;

	/* writes out popstats file header */
	void writeSummariesFileHeader();
};

} // namespace transm
