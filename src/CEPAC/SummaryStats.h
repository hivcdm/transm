#pragma once

#include "include.h"

/*
	SummaryStats class contains a list of the summary statistics from each of the 
	input files (simulation contexts) that are executed in a given run of the model.  
	It contains the functions to generate a summary from a given RunStats object, add it to 
	the list of summaries, and output all the summaries to the popstats file.  Main calls these 
	functions to add each new summary and append to popstats.out at the end of the run.
*/
class SummaryStats
{
public:
	/* Contstructor and Destructor */
	SummaryStats(string summariesFileName);
	~SummaryStats(void);

	/* Summary class stores the summary information that is written to the popstats file */
	class Summary {
	public:
		string runSetName;
		string runName;
		string runDate;
		string runTime;
		int numCohorts;
		double costsAverage;
		double LMsAverage;
		double QALMsAverage;
		double costEffectivenessLYs;
		double costEffectivenessQALYs;
		double costsHIVPositiveAverage;
		double LMsHIVPositiveAverage;
		double QALMsHIVPositiveAverage;
		double numPrimaryOIsPer1000[SimContext::OI_NUM];
		double numDeathsPer1000[SimContext::DTH_NUM_CAUSES];
		double monthsToDetectionIncidentAverage;
		double monthsToDetectionPrevalentAverage;
		double CD4AtDetectionIncidentAverage;
		double CD4AtDetectionPrevalentAverage;
		double numOIsPer1000[SimContext::OI_NUM];
		double numOIDeathsPer1000[SimContext::OI_NUM];
		double numDetecedOIsPer1000[SimContext::OI_NUM];
		double numClinicVisitsPer1000;
		struct compareCosts {
			bool operator()(const Summary *s1, const Summary *s2) const {
				return s1->costsAverage < s2->costsAverage;
			}
		};
	}; /* end Summary */

	/* addRunStats adds a new summary to the vector from a RunStats object */
	void addRunStats(RunStats *runStats);
	/* finalizeStats calculates the final cost-effectiveness ratios for each run */
	void finalizeStats();
	/* writeSummariesFile appends the summary inforation to the popstats.out file */
	void writeSummariesFile();

private:
	/* list of run set vectors of individual run Summary objects,
		uses Summary pointers since objects are large and copy is expensive */
	// TODO: a map from strings to summary vectors would be more efficient, had problems
	//	using this in VC++
	list<vector<Summary *> > summaries;

	/* summaries file name and file pointer */
	string summariesFileName;
	FILE *summariesFile;

	/* writes out popstats file header */
	void writeSummariesFileHeader();
};
