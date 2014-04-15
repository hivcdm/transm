#pragma once

#include "include.h"

/*
	RunStats class contains the aggregate statistics for all cohorts run for a given
	input file (simulation context).  Contains all the functions to update these statistics
	and generate the output files.  Its statistics are updated by the StateUpdater objects
	that are called every month for each patient.  Read only access to the
	statistics is provided through accessor functions that return const pointers to the
	subclass objects.
*/
class RunStats
{
public:
	/* Make the StateUpdater class a friend class so it can modify the private data */
	friend class StateUpdater;

	/* Constructors and Destructor */
	RunStats(string runName, SimContext *simContext);
	~RunStats(void);

	/* Constants used for the survival stats groups, excluding top and bottom X percent */
	static const int NUM_SURVIVAL_GROUPS = 4;
	enum SURVIVAL_GROUPS {SURVIVAL_ALL, SURVIVAL_EXCL_LONG, SURVIVAL_EXCL_SHORT, SURVIVAL_EXCL_LONG_AND_SHORT};
	static const int TRUNC_HISTOGRAM_PERC = 5;
	static const int MAX_NUM_HISTOGRAM_BUCKETS = 250;

	/* PatientSummary class holds information that must be stored for every cohort
		even after death, needed to calculate median and other non-incremental statistics */
	class PatientSummary {
	public:
		double costs;
		double LMs;
		double QALMs;
		struct compareLMs {
			bool operator()(const PatientSummary &p1, const PatientSummary &p2) const {
				return p1.LMs < p2.LMs;
			}
		};
	}; /* end PatientSummary */

	/* PopulationSummary holds summary aggregate statistics and other misc information */
	class PopulationSummary {
	public:
		// Basic run and set information
		string runSetName;
		string runName;
		string runDate;
		string runTime;
		// Number of patient and clinic visit aggregates
		int numCohorts;
		int numCohortsHIVPositive;
		int totalClinicVisits;
		// Overall costs, life months, and quality adjusted life month statistics
		double costsSum;
		double costsAverage;
		double costsSumSquares;
		double costsStdDev;
		double costsLowerBound;
		double costsUpperBound;
		double LMsSum;
		double LMsAverage;
		double LMsSumSquares;
		double LMsStdDev;
		double LMsLowerBound;
		double LMsUpperBound;
		double QALMsSum;
		double QALMsAverage;
		double QALMsSumSquares;
		double QALMsStdDev;
		double QALMsLowerBound;
		double QALMsUpperBound;
		// Aggegate statistics after X number of ART failures
		int numFailART[SimContext::ART_NUM_LINES+1];
		double costsFailARTSum[SimContext::ART_NUM_LINES+1];
		double costsFailARTAverage[SimContext::ART_NUM_LINES+1];
		double LMsFailARTSum[SimContext::ART_NUM_LINES+1];
		double LMsFailARTAverage[SimContext::ART_NUM_LINES+1];
		double QALMsFailARTSum[SimContext::ART_NUM_LINES+1];
		double QALMsFailARTAverage[SimContext::ART_NUM_LINES+1];
		// Overall statistics for HIV positive only
		double costsHIVPositiveSum;
		double costsHIVPositiveAverage;
		double LMsHIVPositiveSum;
		double LMsHIVPositiveAverage;
		double QALMsHIVPositiveSum;
		double QALMsHIVPositiveAverage;
	}; /* end PopulationSummary */

	class HIVScreening {
	public:
		// number of HIV positive and negative cases
		int numPrevalentCases;
		int numIncidentCases;
		int numHIVNegative;
		int numHIVPositiveTotal;
		// initial counts stratified by HIV state
		int numPatientsInitialHIVState[SimContext::HIV_EXT_INF_NUM];
		// num prevalent HIV positive patients at detection,
		//	stratified by CD4 x HIV state and HVL x HIV state
		int numAtDetectionPrevalent;
		int numAtDetectionPrevalentHIV[SimContext::HIV_INF_NUM];
		int numAtDetectionPrevalentCD4[SimContext::CD4_NUM_STRATA];
		int numAtDetectionPrevalentCD4HIV[SimContext::CD4_NUM_STRATA][SimContext::HIV_INF_NUM];
		int numAtDetectionPrevalentHVL[SimContext::HVL_NUM_STRATA];
		int numAtDetectionPrevalentHVLHIV[SimContext::HVL_NUM_STRATA][SimContext::HIV_INF_NUM];
		// num incident HIV positive patients at detection,
		//	stratified by CD4 x HIV state and HVL x HIV state
		int numAtDetectionIncident;
		int numAtDetectionIncidentHIV[SimContext::HIV_INF_NUM];
		int numAtDetectionIncidentCD4[SimContext::CD4_NUM_STRATA];
		double percentAtDetectionIncidentCD4[SimContext::CD4_NUM_STRATA];
		int numAtDetectionIncidentCD4HIV[SimContext::CD4_NUM_STRATA][SimContext::HIV_INF_NUM];
		int numAtDetectionIncidentHVL[SimContext::HVL_NUM_STRATA];
		double percentAtDetectionIncidentHVL[SimContext::HVL_NUM_STRATA];
		int numAtDetectionIncidentHVLHIV[SimContext::HVL_NUM_STRATA][SimContext::HIV_INF_NUM];
		// average CD4 levels at the time of detection (icident and prevalent),
		//	stratified by HIV state
		double CD4AtDetectionPrevalentSum;
		double CD4AtDetectionPrevalentAverage;
		double CD4AtDetectionPrevalentSumHIV[SimContext::HIV_INF_NUM];
		double CD4AtDetectionPrevalentAverageHIV[SimContext::HIV_INF_NUM];
		double CD4AtDetectionIncidentSum;
		double CD4AtDetectionIncidentAverage;
		double CD4AtDetectionIncidentSumHIV[SimContext::HIV_INF_NUM];
		double CD4AtDetectionIncidentAverageHIV[SimContext::HIV_INF_NUM];
		// number of months until infection for incident cases
		double monthsToInfectionSum;
		double monthsToInfectionAverage;
		double monthsToInfectionSumSquares;
		double monthsToInfectionStdDev;
		// number of months after infection until detection for incident cases
		double monthsAfterInfectionToDetectionSum;
		double monthsAfterInfectionToDetectionAverage;
		double monthsAfterInfectionToDetectionSumSquares;
		double monthsAfterInfectionToDetectionStdDev;
		// number of months to detection for incident and prevalent cases
		double monthsToDetectionPrevalentSum;
		double monthsToDetectionPrevalentAverage;
		double monthsToDetectionPrevalentSumSquares;
		double monthsToDetectionPrevalentStdDev;
		double monthsToDetectionIncidentSum;
		double monthsToDetectionIncidentAverage;
		double monthsToDetectionIncidentSumSquares;
		double monthsToDetectionIncidentStdDev;
		// age months at time of detection for incident and prevalent cases
		double ageMonthsAtDetectionPrevalentSum;
		double ageMonthsAtDetectionPrevalentAverage;
		double ageMonthsAtDetectionPrevalentSumSquares;
		double ageMonthsAtDetectionPrevalentStdDev;
		double ageMonthsAtDetectionIncidentSum;
		double ageMonthsAtDetectionIncidentAverage;
		double ageMonthsAtDetectionIncidentSumSquares;
		double ageMonthsAtDetectionIncidentStdDev;
		// number patients detected by gender, means of detection, and type of OI presenting
		int numDetectedGender[SimContext::GENDER_NUM];
		int numDetectedPrevalentMeans[SimContext::HIV_DET_NUM];
		int numDetectedIncidentMeans[SimContext::HIV_DET_NUM];
		int numDetectedByOIs[SimContext::OI_NUM];
		// patient accepting distribution, number that accept and return, and testing intervals
		int numTestingAcceptRate[SimContext::TEST_ACCEPT_NUM][SimContext::HIV_EXT_INF_NUM];
		int numTestingInterval[SimContext::HIV_TEST_FREQ_NUM];
		int numAcceptTest;
		int numRefuseTest;
		int numReturnForResults;
		int numNoReturnForResults;
		// number of tests performed and number of each result
		int numTestsHIVState[SimContext::HIV_EXT_INF_NUM];
		int numTestResultsPrevalent;
		int numTestResultsPrevalentType[SimContext::TEST_RESULT_NUM];
		int numTestResultsIncident;
		int numTestResultsIncidentType[SimContext::TEST_RESULT_NUM];
		int numTestResultsHIVNegative;
		int numTestResultsHIVNegativeType[SimContext::TEST_RESULT_NUM];
	};

	/* SurvivalStats holds total survival statistics, used with different survival
		group types that exclude X percent bottom and top survival numbers */
	class SurvivalStats {
	public:
		// Histogram of life months survival
		map<int,int> LMsHistogram;
		// Aggregate life months statistics
		double LMsMin;
		double LMsMax;
		double LMsMedian;
		double LMsSumDeviationMedian;
		double LMsAverageDeviationMedian;
		double LMsSum;
		double LMsMean;
		double LMsSumDeviation;
		double LMsAverageDeviation;
		double LMsSumDeviationSquares;
		double LMsStdDev;
		double LMsVariance;
		double LMsSumDeviationCubes;
		double LMsSkew;
		double LMsSumDeviationQuads;
		double LMsKurtosis;
		// Costs and quality adjusted life months statistics
		double costsSum;
		double costsMean;
		double costsSumSquares;
		double costsStdDev;
		double QALMsSum;
		double QALMsMean;
		double QALMsSumSquares;
		double QALMsStdDev;
	}; /* end SurvivalStats */

	/* InitialDistributions class contains statistics about the initial patient state */
	class InitialDistributions {
	public:
		// CD4 and HVL histograms
		int numPatientsCD4Level[SimContext::CD4_NUM_STRATA];
		int numPatientsHVLLevel[SimContext::HVL_NUM_STRATA];
		int numPatientsHVLSetpointLevel[SimContext::HVL_NUM_STRATA];
		// Age and gender statistics
		double sumInitialAgeMonths;
		double averageInitialAgeMonths;
		int numMalePatients;
		int numFemalePatients;
		// Prior OI history statistics
		int numPriorOIHistories[SimContext::OI_NUM];
		// ART response type for CD4 effects
		int numARTResposneTypes[SimContext::CD4_RESPONSE_NUM_TYPES];
		// Prevalent generic risk factors
		int numRiskFactors[SimContext::RISK_FACT_NUM];
		// Pediatrics HIV and maternal state
		int numInitialPediatrics[SimContext::PEDS_HIV_NUM][SimContext::PEDS_MOM_HIV_NUM];
	}; /* end InitialDistributions */

	/* CHRMsStats class contains aggregates and distributions of CHRMs occurrences */
	class CHRMsStats {
	public:
		int numPrevalentCHRM[SimContext::CHRM_NUM];
		int numPrevalentCD4[SimContext::CD4_NUM_STRATA];
		int numPrevalentCHRMCD4[SimContext::CHRM_NUM][SimContext::CD4_NUM_STRATA];
		int numIncidentCHRM[SimContext::CHRM_NUM];
		int numIncidentCD4[SimContext::CD4_NUM_STRATA];
		int numIncidentCHRMCD4[SimContext::CHRM_NUM][SimContext::CD4_NUM_STRATA];
		int numDeathsCHRM[SimContext::CHRM_NUM];
		int numDeathsCD4[SimContext::CD4_NUM_STRATA];
		int numDeathsCHRMCD4[SimContext::CHRM_NUM][SimContext::CD4_NUM_STRATA];
	};

	/* OIStats class contains aggregates and distributions of OI occurrences */
	class OIStats {
	public:
		// Primary, secondary and detected OI counts,
		//	stratified by OI type, CD4 level, and OI type x CD4 level
		int numPrimaryOIsOI[SimContext::OI_NUM];
		int numPrimaryOIsCD4[SimContext::CD4_NUM_STRATA];
		int numPrimaryOIsCD4OI[SimContext::CD4_NUM_STRATA][SimContext::OI_NUM];
		int numSecondaryOIsOI[SimContext::OI_NUM];
		int numSecondaryOIsCD4[SimContext::CD4_NUM_STRATA];
		int numSecondaryOIsCD4OI[SimContext::CD4_NUM_STRATA][SimContext::OI_NUM];
		int numDetectedOIsOI[SimContext::OI_NUM];
		int numDetectedOIsCD4[SimContext::CD4_NUM_STRATA];
		int numDetectedOIsCD4OI[SimContext::CD4_NUM_STRATA][SimContext::OI_NUM];
		// OI history logging - number of patients and calculated probabilities of having OI history
		//	stratified by OI x HVL, OI x CD4, and OI x HVL x CD4
		int numPatientsHVL[SimContext::HVL_NUM_STRATA];
		int numPatientsCD4[SimContext::CD4_NUM_STRATA];
		int numPatientsHVLCD4[SimContext::HVL_NUM_STRATA][SimContext::CD4_NUM_STRATA];
		int numPatientsOIHistoryHVL[SimContext::OI_NUM][SimContext::HVL_NUM_STRATA];
		int numPatientsOIHistoryCD4[SimContext::OI_NUM][SimContext::CD4_NUM_STRATA];
		int numPatientsOIHistoryHVLCD4[SimContext::OI_NUM][SimContext::HVL_NUM_STRATA][SimContext::CD4_NUM_STRATA];
		double probPatientsOIHistoryHVL[SimContext::OI_NUM][SimContext::HVL_NUM_STRATA];
		double probPatientsOIHistoryCD4[SimContext::OI_NUM][SimContext::CD4_NUM_STRATA];
		double probPatientsOIHistoryHVLCD4[SimContext::OI_NUM][SimContext::HVL_NUM_STRATA][SimContext::CD4_NUM_STRATA];
		int numMonthsHVL[SimContext::HVL_NUM_STRATA];
		int numMonthsCD4[SimContext::CD4_NUM_STRATA];
		int numMonthsHVLCD4[SimContext::HVL_NUM_STRATA][SimContext::CD4_NUM_STRATA];
		int numMonthsOIHistoryHVL[SimContext::OI_NUM][SimContext::HVL_NUM_STRATA];
		int numMonthsOIHistoryCD4[SimContext::OI_NUM][SimContext::CD4_NUM_STRATA];
		int numMonthsOIHistoryHVLCD4[SimContext::OI_NUM][SimContext::HVL_NUM_STRATA][SimContext::CD4_NUM_STRATA];
		double probMonthsOIHistoryHVL[SimContext::OI_NUM][SimContext::HVL_NUM_STRATA];
		double probMonthsOIHistoryCD4[SimContext::OI_NUM][SimContext::CD4_NUM_STRATA];
		double probMonthsOIHistoryHVLCD4[SimContext::OI_NUM][SimContext::HVL_NUM_STRATA][SimContext::CD4_NUM_STRATA];
	}; /* end OIStats */

	/* DeathStats contains the statistics and distributions of causes of deaths */
	class DeathStats {
	public:
		// Number of deaths stratified by Cause, CD4, and Cause x CD4
		int numDeathsType[SimContext::DTH_NUM_CAUSES];
		int numDeathsCD4[SimContext::CD4_NUM_STRATA];
		int numDeathsCD4Type[SimContext::CD4_NUM_STRATA][SimContext::DTH_NUM_CAUSES];
		// Number of HIV negative deaths
		int numDeathsUninfected;
		// Number of deaths stratified by HVL and HVL x CD4
		int numDeathsHVLCD4[SimContext::HVL_NUM_STRATA][SimContext::CD4_NUM_STRATA];
		int numDeathsHVL[SimContext::HVL_NUM_STRATA];
		// Number of chronic and non-AIDS deaths stratified by CD4 and OI history
		int numChronicAIDSDeathsNoOIHistory;
		int numChronicAIDSDeathsNoOIHistoryCD4[SimContext::CD4_NUM_STRATA];
		int numChronicAIDSDeathsOIHistory;
		int numChronicAIDSDeathsOIHistoryCD4[SimContext::CD4_NUM_STRATA];
		int numNonAIDSDeathsNoOIHistory;
		int numNonAIDSDeathsNoOIHistoryCD4[SimContext::CD4_NUM_STRATA];
		int numNonAIDSDeathsOIHistory;
		int numNonAIDSDeathsOIHistoryCD4[SimContext::CD4_NUM_STRATA];
	}; /* end DeathStats */

	/* OverallSurvival stats contains aggregate survival statistics, seperate from
		SurvivalSummary since these will only be calculated for all cohorts and not subset groups */
	class OverallSurvival {
	public:
		// life months without OI history, with OI history, and both
		//	stratified by totals and CD4
		double LMsNoOIHistory;
		double LMsNoOIHistoryCD4[SimContext::CD4_NUM_STRATA];
		double LMsOIHistory;
		double LMsOIHistoryCD4[SimContext::CD4_NUM_STRATA];
		double LMsTotal;
		double LMsTotalCD4[SimContext::CD4_NUM_STRATA];
		// life months stratified by HVL and HVL setpoint
		double LMsHVL[SimContext::HVL_NUM_STRATA];
		double LMsHVLSetpoint[SimContext::HVL_NUM_STRATA];
		// life months with and without OI history, stratified by OI type
		double LMsNoOIHistoryOIs[SimContext::OI_NUM];
		double LMsOIHistoryOIs[SimContext::OI_NUM];
		// life months stratified by HIV state and HIV positive total
		double LMsHIVPositive;
		double LMsHIVState[SimContext::HIV_ID_NUM];
		double QALMsHIVPositive;
		double QALMsHIVState[SimContext::HIV_ID_NUM];
		// life months in screening state and regular model
		double LMsInScreening;
		double QALMsInScreening;
		double LMsInRegularCEPAC;
		// life months by gender
		double LMsGender[SimContext::GENDER_NUM];
		double QALMsGender[SimContext::GENDER_NUM];
	}; /* end OverallSurvival */

	/* OverallCosts class contains aggregate cost stats */
	class OverallCosts {
	public:
		// costs without OI history, with OI history, and both
		//	stratified by totals and CD4
		double costsNoOIHistory;
		double costsNoOIHistoryCD4[SimContext::CD4_NUM_STRATA];
		double costsOIHistory;
		double costsOIHistoryCD4[SimContext::CD4_NUM_STRATA];
		double costsTotal;
		double costsTotalCD4[SimContext::CD4_NUM_STRATA];
		// costs stratified by HVL and HVL setpoint
		double costsHVL[SimContext::HVL_NUM_STRATA];
		double costsHVLSetpoint[SimContext::HVL_NUM_STRATA];
		// direct medical costs for prophylaxis, stratified by total, OI, and OI x proph line
		double directCostsProph;
		double directCostsProphOIs[SimContext::OI_NUM];
		double directCostsProphOIsProph[SimContext::OI_NUM][SimContext::PROPH_NUM];
		// direct costs for ART, stratied by total and ART line
		double directCostsART;
		double directCostsARTLine[SimContext::ART_NUM_LINES];
		// costs stratified by HIV stats and total for HIV positives
		double costsHIVPositive;
		double costsHIVState[SimContext::HIV_ID_NUM];
		// costs for various screening and testing
		double costsCD4Testing;
		double costsHVLTesting;
		double costsClinicVisits;
		double costsHIVScreeningTests;
		double costsHIVScreeningMisc;
		// undiscounted costs stratified by cost type and unclassified costs
		double totalUndiscountedCosts[SimContext::COST_NUM_TYPES];
		double totalUndiscountedCostsUnclassified;
		// costs for drugs and toxicities
		double costsDrugs;
		double costsToxicity;
		// costs by gender
		double costsGender[SimContext::GENDER_NUM];
	}; /* end OverallCosts */

	/* TBStats class contains stats about TB occurrences, treatments, and costs */
	class TBStats {
	public:
		// number with TB disease at model entry, stratified by TB type x TB state
		int numInStateAtEntry[SimContext::TB_NUM_STRAINS][SimContext::TB_NUM_STATES];
		// number with various TB treatment effects, stratified by TB type x TB treatment stage
		int numStartOnTreatment[SimContext::TB_NUM_STRAINS][SimContext::TB_TREATM_STAGE_NUM];
		int numDropoutTreatment[SimContext::TB_NUM_STRAINS][SimContext::TB_TREATM_STAGE_NUM];
		int numCuredAtTreatmentDropout[SimContext::TB_NUM_STRAINS][SimContext::TB_TREATM_STAGE_NUM];
		int numFinishTreatment[SimContext::TB_NUM_STRAINS][SimContext::TB_TREATM_STAGE_NUM];
		int numCuredAtTreatmentFinish[SimContext::TB_NUM_STRAINS][SimContext::TB_TREATM_STAGE_NUM];
		int numIncreaseResistanceAtTreatmentFinish[SimContext::TB_NUM_STRAINS][SimContext::TB_TREATM_STAGE_NUM];
		// total number of latent and active infections, stratified by TB type
		int numLatentInfections[SimContext::TB_NUM_STRAINS];
		int numActiveInfections[SimContext::TB_NUM_STRAINS];
		// number of reactivations and reinfections from latent TB, stratified by TB type
		int numReactivationsLatent[SimContext::TB_NUM_STRAINS];
		int numReinfectionsLatent[SimContext::TB_NUM_STRAINS];
		// number of relapses and reinfections from history of active TB, stratified by TB type
		int numRelapsesHistoryActive[SimContext::TB_NUM_STRAINS];
		int numReinfectionsHistoryActive[SimContext::TB_NUM_STRAINS];
		// number of spontaneous resolutions and deaths from active TB, stratified by TB type
		int numSpontaneousResolutions[SimContext::TB_NUM_STRAINS];
		int numDeaths[SimContext::TB_NUM_STRAINS];
		// number of toxicities stratified by treatment stage and proph line
		int numTreatmentMinorToxicity[SimContext::TB_TREATM_STAGE_NUM];
		int numTreatmentMajorToxicity[SimContext::TB_TREATM_STAGE_NUM];
		int numProphMinorToxicity[SimContext::PROPH_NUM];
		int numProphMajorToxicity[SimContext::PROPH_NUM];
	}; /* end TBStats */

	/* LTFUStats contains stats about patient loss to follow up and return to care */
	class LTFUStats {
	public:
		// totals for patients that were ever lost and deaths while lost
		int numPatientsLost;
		int numPatientsReturned;
		int numDeathsWhileLost;
		// totals for times ltfu and rtc stratified by CD4
		int numLostToFollowUp;
		int numLostToFollowUpCD4[SimContext::CD4_NUM_STRATA];
		int numReturnToCare;
		int numReturnToCareCD4[SimContext::CD4_NUM_STRATA];
		int numDeathsWhileLostCD4[SimContext::CD4_NUM_STRATA];
		// time lost before returning to care
		double monthsLostBeforeReturnSum;
		double monthsLostBeforeReturnMean;
		double monthsLostBeforeReturnSumSquares;
		double monthsLostBeforeReturnStdDev;
		// number lost to follow up during ART line or pre/post ART
		int numLostToFollowUpART[SimContext::ART_NUM_LINES];
		int numLostToFollowUpPreART;
		int numLostToFollowUpPostART;
		// number return to care on prev ART line or subsequent ART line
		int numReturnOnPrevART[SimContext::ART_NUM_LINES];
		int numReturnOnNextART[SimContext::ART_NUM_LINES];
		int numReturnToCarePreART;
		int numReturnToCarePostART;
		// number of deaths while lost by prev ART line
		int numDeathsWhileLostART[SimContext::ART_NUM_LINES];
		int numDeathsWhileLostPreART;
		int numDeathsWhileLostPostART;
	};

	/* ProphStats class contains statistics about prophylaxis treatments and toxicities */
	class ProphStats {
	public:
		// number of toxicities stratified by OI type and OI type x proph line
		int numMinorToxicity[SimContext::OI_NUM][SimContext::PROPH_NUM];
		int numMinorToxicityTotal[SimContext::OI_NUM];
		int numMajorToxicity[SimContext::OI_NUM][SimContext::PROPH_NUM];
		int numMajorToxicityTotal[SimContext::OI_NUM];
		// average true CD4, observed CD4, and times prophylaxis initiated for primary and
		//	secondary OIs, stratified by OI type x proph line
		double trueCD4InitProphSum[SimContext::PROPH_NUM_TYPES][SimContext::OI_NUM][SimContext::PROPH_NUM];
		double trueCD4InitProphMean[SimContext::PROPH_NUM_TYPES][SimContext::OI_NUM][SimContext::PROPH_NUM];
		double observedCD4InitProphSum[SimContext::PROPH_NUM_TYPES][SimContext::OI_NUM][SimContext::PROPH_NUM];
		double observedCD4InitProphMean[SimContext::PROPH_NUM_TYPES][SimContext::OI_NUM][SimContext::PROPH_NUM];
		int numTimesInitProph[SimContext::PROPH_NUM_TYPES][SimContext::OI_NUM][SimContext::PROPH_NUM];
	}; /* end ProphStats */

	/*  ARTStats contains stats about ART treatments and toxicities */
	class ARTStats {
	public:
		// months in suppressed state, stratified by total and ART line
		int monthsSuppressed;
		int monthsSuppressedLine[SimContext::ART_NUM_LINES];
		// months in partially suppressed and failed state,
		//	stratified ART line, HVL, and ART line x HVL
		int monthsPartiallySuppressedLine[SimContext::ART_NUM_LINES];
		int monthsPartiallySuppressedHVL[SimContext::HVL_NUM_STRATA];
		int monthsPartiallySuppressedLineHVL[SimContext::ART_NUM_LINES][SimContext::HVL_NUM_STRATA];
		int monthsFailedLine[SimContext::ART_NUM_LINES];
		int monthsFailedHVL[SimContext::HVL_NUM_STRATA];
		int monthsFailedLineHVL[SimContext::ART_NUM_LINES][SimContext::HVL_NUM_STRATA];
		// statistics at ART initiation, stratified by ART line x response type
		int numOnARTAtInit[SimContext::ART_NUM_LINES];
		int numOnARTAtInitResp[SimContext::ART_NUM_LINES][SimContext::RESP_NUM_TYPES];
		double trueCD4AtInitSum[SimContext::ART_NUM_LINES];
		double trueCD4AtInitMean[SimContext::ART_NUM_LINES];
		double trueCD4AtInitSumResp[SimContext::ART_NUM_LINES][SimContext::RESP_NUM_TYPES];
		double trueCD4AtInitMeanResp[SimContext::ART_NUM_LINES][SimContext::RESP_NUM_TYPES];
		double observedCD4AtInitSum[SimContext::ART_NUM_LINES];
		double observedCD4AtInitMean[SimContext::ART_NUM_LINES];
		double observedCD4AtInitSumResp[SimContext::ART_NUM_LINES][SimContext::RESP_NUM_TYPES];
		double observedCD4AtInitMeanResp[SimContext::ART_NUM_LINES][SimContext::RESP_NUM_TYPES];
		int numDrawEfficacyAtInit[SimContext::ART_NUM_LINES][SimContext::ART_EFF_NUM_TYPES];
		int numDrawEfficacyAtInitResp[SimContext::ART_NUM_LINES][SimContext::ART_EFF_NUM_TYPES][SimContext::RESP_NUM_TYPES];
		int numCD4ResponseTypeAtInit[SimContext::ART_NUM_LINES][SimContext::CD4_RESPONSE_NUM_TYPES];
		int numCD4ResponseTypeAtInitResp[SimContext::ART_NUM_LINES][SimContext::CD4_RESPONSE_NUM_TYPES][SimContext::RESP_NUM_TYPES];
		int numWithRiskFactorAtInit[SimContext::ART_NUM_LINES][SimContext::RISK_FACT_NUM];
		int numWithRiskFactorAtInitResp[SimContext::ART_NUM_LINES][SimContext::RISK_FACT_NUM][SimContext::RESP_NUM_TYPES];
		// statistics at ART true failure, stratified by ART line x response type
		int numTrueFailure[SimContext::ART_NUM_LINES];
		int numTrueFailureResp[SimContext::ART_NUM_LINES][SimContext::RESP_NUM_TYPES];
		double trueCD4AtTrueFailureSum[SimContext::ART_NUM_LINES];
		double trueCD4AtTrueFailureMean[SimContext::ART_NUM_LINES];
		double trueCD4AtTrueFailureSumResp[SimContext::ART_NUM_LINES][SimContext::RESP_NUM_TYPES];;
		double trueCD4AtTrueFailureMeanResp[SimContext::ART_NUM_LINES][SimContext::RESP_NUM_TYPES];;
		double observedCD4AtTrueFailureSum[SimContext::ART_NUM_LINES];
		double observedCD4AtTrueFailureMean[SimContext::ART_NUM_LINES];
		double observedCD4AtTrueFailureSumResp[SimContext::ART_NUM_LINES][SimContext::RESP_NUM_TYPES];
		double observedCD4AtTrueFailureMeanResp[SimContext::ART_NUM_LINES][SimContext::RESP_NUM_TYPES];
		double monthsToTrueFailureSum[SimContext::ART_NUM_LINES];
		double monthsToTrueFailureMean[SimContext::ART_NUM_LINES];
		double monthsToTrueFailureSumResp[SimContext::ART_NUM_LINES][SimContext::RESP_NUM_TYPES];
		double monthsToTrueFailureMeanResp[SimContext::ART_NUM_LINES][SimContext::RESP_NUM_TYPES];
		double monthsToTrueFailureSumSquares[SimContext::ART_NUM_LINES];
		double monthsToTrueFailureStdDev[SimContext::ART_NUM_LINES];
		double monthsToTrueFailureSumSquaresResp[SimContext::ART_NUM_LINES][SimContext::RESP_NUM_TYPES];
		double monthsToTrueFailureStdDevResp[SimContext::ART_NUM_LINES][SimContext::RESP_NUM_TYPES];
		// total number and true/observed CD4 at observed failure,
		//	stratified by ART line and ART line x failure type
		int numObservedFailure[SimContext::ART_NUM_LINES];
		int numObservedFailureType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		int numObservedFailureAfterTrue[SimContext::ART_NUM_LINES];
		int numObservedFailureAfterTrueType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		int numNeverObservedFailure[SimContext::ART_NUM_LINES];
		double trueCD4AtObservedFailureSum[SimContext::ART_NUM_LINES];
		double trueCD4AtObservedFailureMean[SimContext::ART_NUM_LINES];
		double trueCD4AtObservedFailureSumType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		double trueCD4AtObservedFailureMeanType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		double observedCD4AtObservedFailureSum[SimContext::ART_NUM_LINES];
		double observedCD4AtObservedFailureMean[SimContext::ART_NUM_LINES];
		double observedCD4AtObservedFailureSumType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		double observedCD4AtObservedFailureMeanType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		// average and std dev of number of months between true failure and observed failure,
		//	stratified by ART line and ART line x failure type
		double monthsToObservedFailureSum[SimContext::ART_NUM_LINES];
		double monthsToObservedFailureMean[SimContext::ART_NUM_LINES];
		double monthsToObservedFailureSumType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		double monthsToObservedFailureMeanType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		double monthsToObservedFailureSumSquares[SimContext::ART_NUM_LINES];
		double monthsToObservedFailureStdDev[SimContext::ART_NUM_LINES];
		double monthsToObservedFailureSumSquaresType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		double monthsToObservedFailureStdDevType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		// number who stop ART and true/observed CD4, stratified by ART line and ART line x stop type
		int numStop[SimContext::ART_NUM_LINES];
		int numStopType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		int numStopAfterTrueFailure[SimContext::ART_NUM_LINES];
		int numStopAfterTrueFailureType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		int numNeverStop[SimContext::ART_NUM_LINES];
		double trueCD4AtStopSum[SimContext::ART_NUM_LINES];
		double trueCD4AtStopMean[SimContext::ART_NUM_LINES];
		double trueCD4AtStopSumType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		double trueCD4AtStopMeanType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		double observedCD4AtStopSum[SimContext::ART_NUM_LINES];
		double observedCD4AtStopMean[SimContext::ART_NUM_LINES];
		double observedCD4AtStopSumType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		double observedCD4AtStopMeanType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		// months on art before stopping, stratified by ART line and ART line x stop type
		double monthsToStopSum[SimContext::ART_NUM_LINES];
		double monthsToStopMean[SimContext::ART_NUM_LINES];
		double monthsToStopSumType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		double monthsToStopMeanType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		double monthsToStopSumSquares[SimContext::ART_NUM_LINES];
		double monthsToStopStdDev[SimContext::ART_NUM_LINES];
		double monthsToStopSumSquaresType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		double monthsToStopStdDevType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		// number on ART, number suppressed, and HVL drop at specified month time points,
		//	stratified by ART line x month to record
		int numOnARTAtMonth[SimContext::ART_NUM_LINES][SimContext::ART_NUM_MTHS_RECORD];
		int numSuppressedAtMonth[SimContext::ART_NUM_LINES][SimContext::ART_NUM_MTHS_RECORD];
		double HVLDropsAtMonthSum[SimContext::ART_NUM_LINES][SimContext::ART_NUM_MTHS_RECORD];
		double HVLDropsAtMonthMean[SimContext::ART_NUM_LINES][SimContext::ART_NUM_MTHS_RECORD];
		double HVLDropsAtMonthSumSquares[SimContext::ART_NUM_LINES][SimContext::ART_NUM_MTHS_RECORD];
		double HVLDropsAtMonthStdDev[SimContext::ART_NUM_LINES][SimContext::ART_NUM_MTHS_RECORD];
		// distribution at ART initiation, stratified by ART line x CD4 x HVL
		int distributionAtInit[SimContext::ART_NUM_LINES][SimContext::CD4_NUM_STRATA][SimContext::HVL_NUM_STRATA];
		// number of toxicity cases, stratified by ART line x tox severity x HVL
		int numToxicityCases[SimContext::ART_NUM_LINES][SimContext::ART_NUM_TOX_SEVERITY][SimContext::HVL_NUM_STRATA];
		// number of toxicity deaths, stratified by ART line x HVL
		int numToxicityDeaths[SimContext::ART_NUM_LINES][SimContext::HVL_NUM_STRATA];
		// number of STI interruptions, restarts, endpoints, and patients with interruptions,
		//	stratified by ART line x STI cycle
		int numSTIInterruptions[SimContext::ART_NUM_LINES][SimContext::STI_NUM_TRACKED];
		int numSTIRestarts[SimContext::ART_NUM_LINES][SimContext::STI_NUM_TRACKED];
		int numSTIEndpoints[SimContext::ART_NUM_LINES][SimContext::STI_NUM_TRACKED];
		int numPatientsWithSTIInterruptions[SimContext::ART_NUM_LINES][SimContext::STI_NUM_TRACKED];
		// number and duartion of STI interruptions, stratified by ART line
		double numSTIInterruptionsSum[SimContext::ART_NUM_LINES];
		double numSTIInterruptionsMean[SimContext::ART_NUM_LINES];
		double monthsOnSTIInterruptionSum[SimContext::ART_NUM_LINES];
		double monthsOnSTIInterruptionMean[SimContext::ART_NUM_LINES];
	}; /* end ARTStats */

	/* TimeSummary class contains monthly/yearly longitudinal stats */
	class TimeSummary {
	public:
		// time period month or year
		int timePeriod;
		// num alive at this time, stratified by total and HIV state
		int numAlive;
		int numAliveType[SimContext::HIV_ID_NUM];
		int numAlivePediatrics[SimContext::PEDS_HIV_NUM];
		// Incident HIV infections
		int numIncidentHIVInfections;
		// Sum of all QOL multipliers applied that month -- reported at Ben Linas's request, 2/3/2010 -- errhode
		double sumQOLmultipliers;
		// true and observed CD4 and HVL
		double trueCD4Sum;
		double trueCD4Mean;
		double trueCD4SumSquares;
		double trueCD4StdDev;
		double observedCD4Sum;
		double observedCD4Mean;
		double observedCD4SumSquares;
		double observedCD4StdDev;
		double trueCD4PercentageSum;
		double trueCD4PercentageMean;
		double trueCD4PercentageSumSquares;
		double trueCD4PercentageStdDev;
		double trueHVLSum;
		double trueHVLMean;
		double trueHVLSumSquares;
		double trueHVLStdDev;
		double observedHVLSum;
		double observedHVLMean;
		double observedHVLSumSquares;
		double observedHVLStdDev;
		// distribution of true CD4 stratified by ART state x CD4
		int trueCD4ARTDistribution[SimContext::ART_NUM_STATES][SimContext::CD4_NUM_STRATA];
		// distribution of observed CD4 stratified by CD4
		int observedCD4Distribution[SimContext::CD4_NUM_STRATA];
		// distribution of true and observed HVL stratified by HVL
		int trueHVLDistribution[SimContext::HVL_NUM_STRATA];
		int observedHVLDistribution[SimContext::HVL_NUM_STRATA];
		// distribution of true CD4/HVL stratified by ART state x CD4 x HVL
		int trueCD4HVLARTDistribution[SimContext::ART_NUM_STATES][SimContext::CD4_NUM_STRATA][SimContext::HVL_NUM_STRATA];
		// number in each ART efficacy state
		int numARTEfficacyState[SimContext::ART_EFF_NUM_TYPES];
		// number with primary and secondary OIs, stratified by OI type
		int numPrimaryOIs[SimContext::OI_NUM];
		int numSecondaryOIs[SimContext::OI_NUM];
		// number with OI history by type and number without any OI history
		int numWithOIHistory[SimContext::OI_NUM];
		int numWithoutOIHistory;
		// number with first OI and deaths from first OI, stratified by OI type
		int numWithFirstOI[SimContext::OI_NUM];
		int numDeathsFromFirstOI[SimContext::OI_NUM];
		// num of deaths by cause of death type
		int numDeathsType[SimContext::DTH_NUM_CAUSES];
		// costs of various testing and total monthly cost
		double costsCD4Testing;
		double costsHVLTesting;
		double costsClinicVisits;
		double costsHIVTests;
		double costsHIVMisc;
		double totalMonthlyCohortCosts;
		// costs of prophylaxis, stratified by OI type x proph line
		double costsProph[SimContext::OI_NUM][SimContext::PROPH_NUM];
		// costs of ART, number on ART, and number LTFU, stratified by ART line
		double costsART[SimContext::ART_NUM_LINES];
		int numOnART[SimContext::ART_NUM_LINES];
		int numOnARTIncludingContinuedCosts[SimContext::ART_NUM_LINES];
		// number lost to follow up during ART line or pre/post ART
		int numLostToFollowUpART[SimContext::ART_NUM_LINES];
		int numLostToFollowUpPreART;
		int numLostToFollowUpPostART;
		// number return to care on prev ART line or subsequent ART line
		int numReturnOnPrevART[SimContext::ART_NUM_LINES];
		int numReturnOnNextART[SimContext::ART_NUM_LINES];
		int numReturnToCarePreART;
		int numReturnToCarePostART;
		// num of death while LTFU by ART
		int numDeathsWhileLostART[SimContext::DTH_NUM_CAUSES];
		int numDeathsWhileLostPreART;
		int numDeathsWhileLostPostART;
	}; /* end TimeSummary */

	/* Accessor functions returning const pointers to the statistics subclass objects */
	const PopulationSummary *getPopulationSummary();
	const HIVScreening *getHIVScreening();
	const SurvivalStats *getSurvivalStats(int groupType);
	const InitialDistributions *getInitialDistributions();
	const CHRMsStats *getCHRMsStats();
	const OIStats *getOIStats();
	const DeathStats *getDeathStats();
	const OverallSurvival *getOverallSurvival();
	const OverallCosts *getOverallCosts();
	const TBStats *getTBStats();
	const LTFUStats *getLTFUStats();
	const ProphStats *getProphStats();
	const ARTStats *getARTStats();
	const TimeSummary *getTimeSummary(unsigned int timePeriod);

	/* Functions to calculate final aggregate statistics and to write out the stats file */
	void finalizeStats();
	void writeStatsFile();

private:
	/* Pointer to the associated simulation context */
	SimContext *simContext;
	/* Stats file name and file pointer */
	string statsFileName;
	FILE *statsFile;

	/* Statistics subclass objects */
	PopulationSummary popSummary;
	HIVScreening hivScreening;
	SurvivalStats survivalStats[NUM_SURVIVAL_GROUPS];
	InitialDistributions initialDistributions;
	CHRMsStats chrmsStats;
	OIStats oiStats;
	DeathStats deathStats;
	OverallSurvival overallSurvival;
	OverallCosts overallCosts;
	TBStats tbStats;
	LTFUStats ltfuStats;
	ProphStats prophStats;
	ARTStats artStats;
	// vector of PatientSummary objects for all cohorts in this context,
	//	uses actual object sinces subclass only contains 3 native type members, copy is cheap
	vector<PatientSummary> patients;
	// vector of TimeSummary object for each month/year time period,
	//	use pointer to object since subclass is complex and copy would be expensive
	vector<TimeSummary *> timeSummaries;

	/* Initialization functions for statistics objects, called by constructor */
	void initPopulationSummary();
	void initHIVScreening();
	void initSurvivalStats();
	void initInitialDistributions();
	void initCHRMsStats();
	void initOIStats();
	void initDeathStats();
	void initOverallSurvival();
	void initOverallCosts();
	void initTBStats();
	void initLTFUStats();
	void initProphStats();
	void initARTStats();
	void initTimeSummary(TimeSummary *currTime);

	/* Functions to finalize aggregate statistics before printing out */
	void finalizePopulationSummary();
	void finalizeHIVScreening();
	void finalizeSurvivalStats();
	void finalizeInitialDistributions();
	void finalizeCHRMsStats();
	void finalizeOIStats();
	void finalizeDeathStats();
	void finalizeOverallSurvival();
	void finalizeOverallCosts();
	void finalizeTBStats();
	void finalizeLTFUStats();
	void finalizeProphStats();
	void finalizeARTStats();
	void finalizeTimeSummaries();

	/* Functions to write out each subclass object to the statistics file, called by writeStatsFile */
	void writePopulationSummary();
	void writeHIVScreening();
	void writeSurvivalStats();
	void writeInitialDistributions();
	void writeCHRMsStats();
	void writeOIStats();
	void writeDeathStats();
	void writeOverallSurvival();
	void writeOverallCosts();
	void writeTBStats();
	void writeLTFUStats();
	void writeProphStats();
	void writeARTStats();
	void writeTimeSummaries();
};

/* getPopulationSumary returns a const pointer to the PopulationSummary statistics object */
inline const RunStats::PopulationSummary *RunStats::getPopulationSummary() {
	return &popSummary;
}

/* getHIVScreening returns a const pointer to the HIVScreening statistics object */
inline const RunStats::HIVScreening *RunStats::getHIVScreening() {
	return &hivScreening;
}


/* getSurvivalStats returns a const pointer to the specified subgroup's SurvivalStats object */
inline const RunStats::SurvivalStats *RunStats::getSurvivalStats(int groupType) {
	return &(survivalStats[groupType]);
}

/* getInitialDistributions returns a const pointer to the InitialDistributions object */
inline const RunStats::InitialDistributions *RunStats::getInitialDistributions() {
	return &initialDistributions;
}

/* getOIStats returns a const pointer to the CHRMsStats object */
inline const RunStats::CHRMsStats *RunStats::getCHRMsStats() {
	return &chrmsStats;
}

/* getOIStats returns a const pointer to the OIStats object */
inline const RunStats::OIStats *RunStats::getOIStats() {
	return &oiStats;
}

/* getDeathStats returns a const pointer to the DeathStats object */
inline const RunStats::DeathStats *RunStats::getDeathStats() {
	return &deathStats;
}

/* getOverallSurvival returns a const pointer to the OverallSurvival object */
inline const RunStats::OverallSurvival *RunStats::getOverallSurvival() {
	return &overallSurvival;
}

/* getOverallCosts returns a const pointer to the OverallCosts object */
inline const RunStats::OverallCosts *RunStats::getOverallCosts() {
	return &overallCosts;
}

/* getTBStats returns a const pointer to the TBStats object */
inline const RunStats::TBStats *RunStats::getTBStats() {
	return &tbStats;
}

/* getLTFUStats returns a const pointer to the LTFUStats object */
inline const RunStats::LTFUStats *RunStats::getLTFUStats() {
	return &ltfuStats;
}

/* getProphStats returns a const pointer to the ProphStats object */
inline const RunStats::ProphStats *RunStats::getProphStats() {
	return &prophStats;
}

/* getARTStats returns a const pointer to the ArtStats object */
inline const RunStats::ARTStats *RunStats::getARTStats() {
	return &artStats;
}

/* getTimeSummary returns a const pointer to the specified TimeSummary object,
	returns null if one does not exist for this time period */
inline const RunStats::TimeSummary *RunStats::getTimeSummary(unsigned int timePeriod) {
	if (timePeriod < timeSummaries.size())
		return timeSummaries[timePeriod];
	return NULL;
}
