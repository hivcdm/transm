/*
 * TransmissionSummaryStats.cpp
 *
 *  Created on: Jun 24, 2010
 *      Author: errhode
 */

#include "TransmissionSummaryStats.h"
#include "PopStats.h"
#include "../data/EventParams.h"

/* Constructor takes summariesFileName as input, clears summaries vector */
TransmissionSummaryStats::TransmissionSummaryStats(string summariesFileName) {
	this->summariesFileName = summariesFileName;
	summaries.clear();
} /* end Constructor */

/* Destructor frees allocated Summary objects and clears summaries vector */
TransmissionSummaryStats::~TransmissionSummaryStats(void)
{
	for (vector<TransmissionSummary *>::iterator j = summaries.begin(); j != summaries.end(); j++) {
		TransmissionSummary *summary = *j;
		delete summary;
	}
	summaries.clear();
} /* end Destructor */

TransmissionSummaryStats::TransmissionSummary::~TransmissionSummary(){
	//std::vector<PopStats::SingleTimeStats*>* selectedSummaryStats;
	for (std::vector<PopStats::SingleTimeStats*>::iterator iter = selectedSummaryStats->begin(); iter != selectedSummaryStats->end(); iter++){
		PopStats::SingleTimeStats *singleTimeStat = *iter;
		delete singleTimeStat;
	}

	selectedSummaryStats->clear();
	delete selectedSummaryStats;
}

/* addRunStats adds a new summary to the vector from a RunStats object */
void TransmissionSummaryStats::addPopStats(PopStats *popStats, EventParams *eventParams) {
	/* Create a new summary object */
	TransmissionSummary *summary = new TransmissionSummary();

	/* Copy the population summary stats */
	//const RunStats::PopulationSummary *popSummary = runStats->getPopulationSummary();
	//TODO: This doesn't mean anything for now...
	summary->runSetName = eventParams->simName;
	summary->runName = eventParams->simName;
	//summary->runDate = eventParams.
	//summary->runTime = popSummary->runTime;
	//summary->numCohorts = popSummary->numCohorts;
	summary->selectedSummaryStats = new std::vector<PopStats::SingleTimeStats*>(popStats->getSelectedSummaryStats()->begin(), popStats->getSelectedSummaryStats()->end());
	
	for (int i = 0; i < summary->selectedSummaryStats->size(); i++){
		summary->timeToRecord[i] = summary->selectedSummaryStats->at(i)->timeOfStats;
	}
	summary->LMsAverage = popStats->lifeStats->getStat(PopStats::TOTAL_LM)/(popStats->lifeStats->getStat(PopStats::TOTAL_HIV_POS) + popStats->lifeStats->getStat(PopStats::TOTAL_HIV_NEG));
	summary->HIVPosLMAverage = popStats->lifeStats->getStat(PopStats::TOTAL_HIV_POS_LM)/popStats->lifeStats->getStat(PopStats::TOTAL_HIV_POS) ;
	summary->HIVNegLMAverage = popStats->lifeStats->getStat(PopStats::TOTAL_HIV_NEG_LM)/popStats->lifeStats->getStat(PopStats::TOTAL_HIV_NEG);
	summary->HIVPosSurvivalAverage = popStats->lifeStats->getStat(PopStats::TOTAL_HIV_POS_POSTINFECT_LM)/popStats->lifeStats->getStat(PopStats::TOTAL_HIV_POS) ;
	//TODO: Fix me!
	summary->AverageNumberOfPeopleEachPersonInfects = 1;

	// Add the new summary to the summaries vector
	summaries.push_back(summary);

} /* end addRunStats */

/* writeSummariesFile appends the summary information to the popstats.out file */
void TransmissionSummaryStats::writeSummariesFile() {
	// Open the popstats file and write header if needed
	CepacUtil::changeDirectoryToResults();

	this->summaryStatsStream.open(this->summariesFileName.c_str(), ios::out | ios::app);
	writeSummariesFileHeader();

	// Loop over the individual run summaries of the summaries vector
	int j;
	for (vector<TransmissionSummary *>::iterator i = this->summaries.begin(); i != this->summaries.end(); i++) {
		TransmissionSummary *summary = *i;
		this->summaryStatsStream << summary->runName << "\t";
		this->summaryStatsStream << summary->LMsAverage << "\t";
		this->summaryStatsStream << summary->HIVPosLMAverage << "\t";
		this->summaryStatsStream << summary->HIVNegLMAverage << "\t";
		this->summaryStatsStream << summary->HIVPosSurvivalAverage << "\t";
		this->summaryStatsStream << summary->AverageNumberOfPeopleEachPersonInfects << "\t";

		for (j = 0; j < summary->selectedSummaryStats->size(); j++){
			PopStats::SingleTimeStats *singleTimeStat = summary->selectedSummaryStats->at(j);
			this->summaryStatsStream << singleTimeStat->timeOfStats << "\t";
			this->summaryStatsStream << singleTimeStat->prevalence << "\t";
			this->summaryStatsStream << singleTimeStat->SAprevalence << "\t";
			this->summaryStatsStream << singleTimeStat->incidence << "\t";
			this->summaryStatsStream << singleTimeStat->cumulativeNumberDead << "\t";
		}
		this->summaryStatsStream << std::endl;
	}

	this->summaryStatsStream.close();
} /* end writeSummariesFile */

/* writes out summaries file header */
void TransmissionSummaryStats::writeSummariesFileHeader() {
	int i;
	this->summaryStatsStream << "RunName\t Average LM \t HIV+ LM\t HIV- LM\t HIV+ Survival\t R_0\t";
	for (i = 0; i < NUM_TIMES_TO_RECORD; i++){
		this->summaryStatsStream << "Time\t Prevalence\t SA Prevalence\t Incidence\t Cumulative No. Dead\t";
	}
	this->summaryStatsStream << std::endl;
} /* end writeSummariesFileHeader */
