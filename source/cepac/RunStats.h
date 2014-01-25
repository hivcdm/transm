#pragma once

#include "include.h"

/**
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
	/** Make the StateUpdater class a friend class so it can modify the private data */
	friend class StateUpdater;

	/* Constructors and Destructor */
	RunStats(string runName, SimContext *simContext);
	~RunStats(void);

	/** Constants used for the survival stats groups, excluding top and bottom X percent */
	static const int NUM_SURVIVAL_GROUPS = 4;
	enum SURVIVAL_GROUPS {SURVIVAL_ALL, SURVIVAL_EXCL_LONG, SURVIVAL_EXCL_SHORT, SURVIVAL_EXCL_LONG_AND_SHORT};
	static const int TRUNC_HISTOGRAM_PERC = 5;
	static const int MAX_NUM_HISTOGRAM_BUCKETS = 250;

	/** PatientSummary class holds information that must be stored for every cohort
		even after death, needed to calculate median and other non-incremental statistics */
	class PatientSummary {
	public:
		/** Total costs accrued by the patient */
		double costs;
		/** Total life months lived by the patient */
		double LMs;
		/** Total quality adjusted life months lived by the patient */
		double QALMs;
		/** A struct containing an operator that compares the costs of two summaries that returns true if the first summary had lower costs than the second summary */
		struct compareLMs {
			bool operator()(const PatientSummary &p1, const PatientSummary &p2) const {
				return p1.LMs < p2.LMs;
			}
		};
	}; /* end PatientSummary */

	/** PopulationSummary holds summary aggregate statistics and other misc information */
	class PopulationSummary {
	public:
		// Basic run and set information
		/** The name of the run set this summary belongs to */
		string runSetName;
		/** The name of the run corresponding to this summary */
		string runName;
		/** The date of the run */
		string runDate;
		/** The time the run finished */
		string runTime;
		// Number of patient and clinic visit aggregates
		/** The number of Persons in this cohort */
		int numCohorts;
		/** The number of HIV positive Persons in this cohort */
		int numCohortsHIVPositive;
		/** The total number of clinic visits in this run */
		int totalClinicVisits;
		// Overall costs, life months, and quality adjusted life month statistics
		/** The total cost accrued in this run*/
		double costsSum;
		/** The average costs per Person */
		double costsAverage;
		/** The sum of the squares of the costs per person (for calculating the standard deviation) */
		double costsSumSquares;
		/** The standard deviation of the costs per person */
		double costsStdDev;
		/** The lowest cost per person */
		double costsLowerBound;
		/** The highest cost per person */
		double costsUpperBound;
		/** The total number of life months lived in this run */
		double LMsSum;
		/** The average life months lived per Person */
		double LMsAverage;
		/** The sum of the squares of the life months lived per person (for calculating the standard deviation) */
		double LMsSumSquares;
		/** The standard deviation of the life months lived per person */
		double LMsStdDev;
		/** The lowest life months lived per person */
		double LMsLowerBound;
		/** The highest life months lived per person */
		double LMsUpperBound;
		/** The total number of quality adjusted life months lived in this run */
		double QALMsSum;
		/** The average quality adjusted life months lived per Person */
		double QALMsAverage;
		/** The sum of the squares of the quality adjusted life months lived per person (for calculating the standard deviation) */
		double QALMsSumSquares;
		/** The standard deviation of the quality adjusted life months lived per person */
		double QALMsStdDev;
		/** The lowest quality adjusted life months lived per person */
		double QALMsLowerBound;
		/** The highest quality adjusted life months lived per person */
		double QALMsUpperBound;
		// Aggregate statistics after X number of ART failures
		/** The number of ART failures stratified by ART line */
		int numFailART[SimContext::ART_NUM_LINES+1];
		/** The total costs stratified by most recent ART line failed */
		double costsFailARTSum[SimContext::ART_NUM_LINES+1];
		/** The average cost per person stratified by most recent ART line failed */
		double costsFailARTAverage[SimContext::ART_NUM_LINES+1];
		/** The total life months lived stratified by most recent ART line failed */
		double LMsFailARTSum[SimContext::ART_NUM_LINES+1];
		/** The average life months lived per person stratified by most recent ART line failed */
		double LMsFailARTAverage[SimContext::ART_NUM_LINES+1];
		/** The total quality adjusted life months lived stratified by most recent ART line failed */
		double QALMsFailARTSum[SimContext::ART_NUM_LINES+1];
		/** The average quality adjusted life months lived per person stratified by most recent ART line failed */
		double QALMsFailARTAverage[SimContext::ART_NUM_LINES+1];
		// Overall statistics for HIV positive only
		/** The total costs accrued for HIV positive persons */
		double costsHIVPositiveSum;
		/** The average costs per person accrued for HIV positive persons */
		double costsHIVPositiveAverage;
		/** The total life months lived by HIV positive Persons*/
		double LMsHIVPositiveSum;
		/** The average life months per person lived by HIV positive Persons*/
		double LMsHIVPositiveAverage;
		/** The total quality adjusted life months lived by HIV positive Persons */
		double QALMsHIVPositiveSum;
		/** The average quality adjusted life months per person lived by HIV positive Persons */
		double QALMsHIVPositiveAverage;
	}; /* end PopulationSummary */

	/** HIVScreening holds the statistics related to HIV screening and detection (i.e. the "testing module") */
	class HIVScreening {
	public:
		// number of HIV positive and negative cases
		/** Total number of prevalent HIV cases */
		int numPrevalentCases;
		/** Total number of incident HIV cases */
		int numIncidentCases;
		/** Total number of HIV negative (never infected) cases */
		int numHIVNegative;
		/** Total number of HIV cases */
		int numHIVPositiveTotal;
		/** Initial counts stratified by HIV state */
		int numPatientsInitialHIVState[SimContext::HIV_EXT_INF_NUM];
		// num prevalent HIV positive patients at detection,
		//	stratified by CD4 x HIV state and HVL x HIV state
		/** Number of prevalent HIV positive patients who become detected */
		int numAtDetectionPrevalent;
		/** Number of prevalent HIV positive patients stratified by HIV state at detection */
		int numAtDetectionPrevalentHIV[SimContext::HIV_INF_NUM];
		/** Number of prevalent HIV positive patients stratified by CD4 strata */
		int numAtDetectionPrevalentCD4[SimContext::CD4_NUM_STRATA];
		/** Number of prevalent HIV positive patients stratified by HIV state at detection and CD4 strata */
		int numAtDetectionPrevalentCD4HIV[SimContext::CD4_NUM_STRATA][SimContext::HIV_INF_NUM];
		/** Number of prevalent HIV positive patients stratified by HVL strata */
		int numAtDetectionPrevalentHVL[SimContext::HVL_NUM_STRATA];
		/** Number of prevalent HIV positive patients stratified by HIV state at detection and HVL strata */
		int numAtDetectionPrevalentHVLHIV[SimContext::HVL_NUM_STRATA][SimContext::HIV_INF_NUM];
		// num incident HIV positive patients at detection,
		//	stratified by CD4 x HIV state and HVL x HIV state
		/** Number of incident HIV positive patients who become detected */
		int numAtDetectionIncident;
		/** Number of incident HIV positive patients stratified by HIV state at detection */
		int numAtDetectionIncidentHIV[SimContext::HIV_INF_NUM];
		/** Number of incident HIV positive patients stratified by CD4 strata */
		int numAtDetectionIncidentCD4[SimContext::CD4_NUM_STRATA];
		/** Percentage of incident HIV positive patients stratified by CD4 strata */
		double percentAtDetectionIncidentCD4[SimContext::CD4_NUM_STRATA];
		/** Number of incident HIV positive patients stratified by HIV state and CD4 strata at detection */
		int numAtDetectionIncidentCD4HIV[SimContext::CD4_NUM_STRATA][SimContext::HIV_INF_NUM];
		/** Number of incident HIV positive patients stratified by HVL strata at detection*/
		int numAtDetectionIncidentHVL[SimContext::HVL_NUM_STRATA];
		/** Percentage of incident HIV positive patients stratified by HVL strata at detection*/
		double percentAtDetectionIncidentHVL[SimContext::HVL_NUM_STRATA];
		/** Number of incident HIV positive patients stratified by HIV state and HVL strata at detection */
		int numAtDetectionIncidentHVLHIV[SimContext::HVL_NUM_STRATA][SimContext::HIV_INF_NUM];
		//num patients at linkage
		//stratified by CD4 x HIV state and HVL x HIV state
		/** Number of HIV positive patients who become linked */
		int numAtLinkage;
		/** Number of HIV positive patients stratified by HIV state at linkage */
		int numAtLinkageHIV[SimContext::HIV_INF_NUM];
		/** Number of HIV positive patients stratified by CD4 strata */
		int numAtLinkageCD4[SimContext::CD4_NUM_STRATA];
		/** Number of HIV positive patients stratified by HVL strata */
		int numAtLinkageHVL[SimContext::HVL_NUM_STRATA];
		/** Number of HIV positive patients stratified by HIV state at linkage and HVL strata */
		int numAtLinkageHVLHIV[SimContext::HVL_NUM_STRATA][SimContext::HIV_INF_NUM];
		/** Number of HIV positive patients stratified by HIV state and CD4 strata at linkage */
		int numAtLinkageCD4HIV[SimContext::CD4_NUM_STRATA][SimContext::HIV_INF_NUM];

		// average CD4 levels at the time of detection (incident and prevalent),
		//	stratified by HIV state
		/** Total CD4 count (incident cases) at time of detection */
		double CD4AtDetectionPrevalentSum;
		/** Average CD4 count (incident cases) at time of detection */
		double CD4AtDetectionPrevalentAverage;
		/** Total CD4 count (incident cases) at time of detection stratified by HIV state at detection */
		double CD4AtDetectionPrevalentSumHIV[SimContext::HIV_INF_NUM];
		/** Average CD4 count (incident cases) at time of detection stratified by HIV state at detection */
		double CD4AtDetectionPrevalentAverageHIV[SimContext::HIV_INF_NUM];
		/** Total CD4 count (incident cases) at time of detection */
		double CD4AtDetectionIncidentSum;
		/** Average CD4 count (incident cases) at time of detection */
		double CD4AtDetectionIncidentAverage;
		/** Total CD4 count (incident cases) at time of detection stratified by HIV state at detection */
		double CD4AtDetectionIncidentSumHIV[SimContext::HIV_INF_NUM];
		/** Average CD4 count (incident cases) at time of detection stratified by HIV state at detection */
		double CD4AtDetectionIncidentAverageHIV[SimContext::HIV_INF_NUM];
		// average CD4 levels at the time of linkage,
		//	stratified by HIV state
		/** Total CD4 count at time of linkage */
		double CD4AtLinkageSum;
		/** Average CD4 count at time of linkage*/
		double CD4AtLinkageAverage;
		/** Total CD4 count at time of linkage stratified by HIV state at linkage */
		double CD4AtLinkageSumHIV[SimContext::HIV_INF_NUM];
		/** Average CD4 count at time of linkage stratified by HIV state at linkage */
		double CD4AtLinkageAverageHIV[SimContext::HIV_INF_NUM];
		// number of months until infection for incident cases
		/** Total number of months until infection for incident cases */
		double monthsToInfectionSum;
		/** Average number of months until infection for incident cases */
		double monthsToInfectionAverage;
		/** Sum of squares of the number of months until infection for incident cases (for calculating standard deviation) */
		double monthsToInfectionSumSquares;
		/** Standard deviation of months until infection for incident cases */
		double monthsToInfectionStdDev;
		// number of months after infection until detection for incident cases
		/** Total number of months after infection until detection for incident cases */
		double monthsAfterInfectionToDetectionSum;
		/** Average number of months after infection until detection for incident cases */
		double monthsAfterInfectionToDetectionAverage;
		/** Sum of squares of the number of months after infection until detection for incident cases */
		double monthsAfterInfectionToDetectionSumSquares;
		/** Standard deviation of the number of months after infection until detection for incident cases */
		double monthsAfterInfectionToDetectionStdDev;
		// number of months to detection for incident and prevalent cases
		/** Total number of months to detection for prevalent cases */
		double monthsToDetectionPrevalentSum;
		/** Average number of months to detection for prevalent cases */
		double monthsToDetectionPrevalentAverage;
		/** Sum of squares of the number of months to detection for prevalent cases (for calculating standard deviation) */
		double monthsToDetectionPrevalentSumSquares;
		/** Standard deviation of the number of months to detection for prevalent cases */
		double monthsToDetectionPrevalentStdDev;
		/** Total number of months to detection for incident cases from model start */
		double monthsToDetectionIncidentSum;
		/** Average number of months to detection for incident cases from model start */
		double monthsToDetectionIncidentAverage;
		/** Sum of squares of the number of months to detection for incident cases from model start (for calculating standard deviation) */
		double monthsToDetectionIncidentSumSquares;
		/** Standard deviation of the number of months to detection for incident cases from model start */
		double monthsToDetectionIncidentStdDev;
		// number of months to linkage
		/** Total number of months to linkage*/
		double monthsToLinkageSum;
		/** Average number of months to linkage*/
		double monthsToLinkageAverage;
		/** Sum of squares of the number of months to linkage (for calculating standard deviation) */
		double monthsToLinkageSumSquares;
		/** Standard deviation of the number of months to linkage */
		double monthsToLinkageStdDev;
		// number of months to linkage stratified by means of detection
		double monthsToLinkageSumMeans[SimContext::HIV_DET_NUM];
		double monthsToLinkageAverageMeans[SimContext::HIV_DET_NUM];
		double monthsToLinkageSumSquaresMeans[SimContext::HIV_DET_NUM];
		double monthsToLinkageStdDevMeans[SimContext::HIV_DET_NUM];

		// age months at time of detection for incident and prevalent cases
		/** Total age months at time of detection for prevalent cases*/
		double ageMonthsAtDetectionPrevalentSum;
		/** Average age months at time of detection for prevalent cases*/
		double ageMonthsAtDetectionPrevalentAverage;
		/** Sum of squares of age months at time of detection for prevalent cases (for calculating standard deviation) */
		double ageMonthsAtDetectionPrevalentSumSquares;
		/** Standard deviation of age months at time of detection for prevalent cases*/
		double ageMonthsAtDetectionPrevalentStdDev;
		/** Total age months at time of detection for incident cases*/
		double ageMonthsAtDetectionIncidentSum;
		/** Average age months at time of detection for incident cases*/
		double ageMonthsAtDetectionIncidentAverage;
		/** Sum of squares of age months at time of detection for incident cases (for calculating standard deviation) */
		double ageMonthsAtDetectionIncidentSumSquares;
		/** Standard deviation of age months at time of detection for incident cases*/
		double ageMonthsAtDetectionIncidentStdDev;
		// age months at time of linkage
		/** Total age months at time of linkage*/
		double ageMonthsAtLinkageSum;
		/** Average age months at time of linkage*/
		double ageMonthsAtLinkageAverage;
		/** Sum of squares of age months at time of linkage (for calculating standard deviation) */
		double ageMonthsAtLinkageSumSquares;
		/** Standard deviation of age months at time of linkage*/
		double ageMonthsAtLinkageStdDev;

		// number patients detected by gender, means of detection, and type of OI presenting
		/** Number of detected patients stratified by gender */
		int numDetectedGender[SimContext::GENDER_NUM];
		/** Number of detected prevalent patients stratified by detection means */
		int numDetectedPrevalentMeans[SimContext::HIV_DET_NUM];
		/** Number of detected incident patients stratified by detection means */
		int numDetectedIncidentMeans[SimContext::HIV_DET_NUM];
		/** Number of linked cases startified by link means*/
		int numLinkedMeans[SimContext::HIV_DET_NUM];
		/** Number of detected patients stratified by type of OI leading to detection */
		int numDetectedByOIs[SimContext::OI_NUM];
		/** Number of detected patients by OI who were previously detected by other means strat by OI */
		int numDetectedByOIsPrevDetected[SimContext::OI_NUM];
		// patient accepting distribution, number that accept and return, and testing intervals
		/** Patient test accepting distributino stratified by HIV status */
		int numTestingAcceptRate[SimContext::TEST_ACCEPT_NUM][SimContext::HIV_EXT_INF_NUM];
		/** Distribution of HIV testing intervals */
		int numTestingInterval[SimContext::HIV_TEST_FREQ_NUM];
		/** Total number of test acceptances */
		int numAcceptTest;
		/** Total number of test refusals */
		int numRefuseTest;
		/** Total number of returns for results */
		int numReturnForResults;
		/** Total number tests where the patient did not return for results*/
		int numNoReturnForResults;

		/** Total number of Lab Staging Accepts */
		int numAcceptLabStaging;
		/** Total number of Lab Staging refusals */
		int numRefuseLabStaging;
		/** Total number of returns for results for Lab Staging*/
		int numReturnForResultsLabStaging;
		/** Total number tests where the patient did not return for results of Lab Staging*/
		int numNoReturnForResultsLabStaging;
		/** Total number of people who link to care through Lab Staging */
		int numLinkLabStaging;
		/** Total number of people who do not link to care through Lab Staging */
		int numNoLinkLabStaging;
		/** Number Accepting Lab Staging stratified by HIV state*/
		int numAcceptLabStagingHIVState[SimContext::HIV_POS_NUM];
		/** Number Returning for results of Lab Staging stratified by HIV state*/
		int numReturnLabStagingHIVState[SimContext::HIV_POS_NUM];
		/** Number of people returning to care through lab staging stratified by Obsv CD4*/
		int numReturnLabStagingObsvCD4[SimContext::CD4_NUM_STRATA];
		/** Number of people returning to care through lab staging stratified by True CD4*/
		int numReturnLabStagingTrueCD4[SimContext::CD4_NUM_STRATA];
		/** Number of people returning through lab staging stratified by Observed and True CD4*/
		int numReturnLabStagingObsvTrueCD4[SimContext::CD4_NUM_STRATA][SimContext::CD4_NUM_STRATA];
		/** Number of people linking to care through lab staging stratified by Obsv CD4*/
		int numLinkLabStagingObsvCD4[SimContext::CD4_NUM_STRATA];
		/** Number of people linking to care through lab staging stratified by True CD4*/
		int numLinkLabStagingTrueCD4[SimContext::CD4_NUM_STRATA];
		/** Number of people linking through lab staging stratified by Observed and True CD4*/
		int numLinkLabStagingObsvTrueCD4[SimContext::CD4_NUM_STRATA][SimContext::CD4_NUM_STRATA];

		// number of tests performed and number of each result
		/** Number of HIV tests given stratified by HIV state */
		int numTestsHIVState[SimContext::HIV_EXT_INF_NUM];
		/** Number of Test results given to prevalent patients */
		int numTestResultsPrevalent;
		/** Number of Test results given to prevalent patients stratified by test result */
		int numTestResultsPrevalentType[SimContext::TEST_RESULT_NUM];
		/** Number of Test results given to incident patients */
		int numTestResultsIncident;
		/** Number of Test results given to incident patients stratified by test result */
		int numTestResultsIncidentType[SimContext::TEST_RESULT_NUM];
		/** Number of Test results given to HIV negative patients */
		int numTestResultsHIVNegative;
		/** Number of Test results given to HIV negative patients stratified by test result */
		int numTestResultsHIVNegativeType[SimContext::TEST_RESULT_NUM];
	};

	/** SurvivalStats holds total survival statistics, used with different survival
		group types that exclude X percent bottom and top survival numbers */
	class SurvivalStats {
	public:
		/** Histogram of life months survival */
		map<int,int> LMsHistogram;
		// Aggregate life months statistics
		/** Minimum number of life months lived */
		double LMsMin;
		/** Maximum number of life months lived */
		double LMsMax;
		/** Median number of life months lived */
		double LMsMedian;
		/** Sum deviation from median number of life months lived */
		double LMsSumDeviationMedian;
		/** Average deviation from median number of life months lived */
		double LMsAverageDeviationMedian;
		/** Total number of life months lived */
		double LMsSum;
		/** Average number of life months lived */
		double LMsMean;
		/** Sum Deviation of the number of life months lived */
		double LMsSumDeviation;
		/** Average Deviation of the number of life months lived */
		double LMsAverageDeviation;
		/** Sum Deviation Squares of the number of life months lived */
		double LMsSumDeviationSquares;
		/** Standard Deviation of the number of life months lived */
		double LMsStdDev;
		/** Variance of the number of life months lived */
		double LMsVariance;
		/** Sum Deviation Cubes of the number of life months lived */
		double LMsSumDeviationCubes;
		/** Skew of the number of life months lived */
		double LMsSkew;
		/** Sum Deviation Quads of the number of life months lived */
		double LMsSumDeviationQuads;
		/** Kurtosis of the number of life months lived */
		double LMsKurtosis;
		// Costs and quality adjusted life months statistics
		/** Total costs accrued */
		double costsSum;
		/** Total costs accrued per person */
		double costsMean;
		/** Sum of squares of costs accrued */
		double costsSumSquares;
		/** Standard deviation of costs accrued */
		double costsStdDev;
		/** Total quality adjusted life months lived */
		double QALMsSum;
		/** Average quality adjusted life months lived */
		double QALMsMean;
		/** Sum of squares of quality adjusted life months lived */
		double QALMsSumSquares;
		/** Standard deviation of quality adjusted life months lived */
		double QALMsStdDev;
	}; /* end SurvivalStats */

	/** InitialDistributions class contains statistics about the initial patient state */
	class InitialDistributions {
	public:
		// CD4 and HVL histograms
		/** Histogram of initial CD4 stratas */
		int numPatientsCD4Level[SimContext::CD4_NUM_STRATA];
		/** Histogram of initial HVL stratas */
		int numPatientsHVLLevel[SimContext::HVL_NUM_STRATA];
		/** Histogram of initial setpoint levels */
		int numPatientsHVLSetpointLevel[SimContext::HVL_NUM_STRATA];
		// Age and gender statistics
		/** Total initial age (in months) */
		double sumInitialAgeMonths;
		/** Average initial age in months */
		double averageInitialAgeMonths;
		/** Total number of male patients */
		int numMalePatients;
		/** Total number of female patients */
		int numFemalePatients;
		/** Prior OI history statistics */
		int numPriorOIHistories[SimContext::OI_NUM];
		/** ART response type for CD4 effects */
		int numARTResposneTypes[SimContext::CD4_RESPONSE_NUM_TYPES];
		/** Prevalent generic risk factors */
		int numRiskFactors[SimContext::RISK_FACT_NUM];
		/** Pediatrics HIV and maternal state */
		int numInitialPediatrics[SimContext::PEDS_HIV_NUM][SimContext::PEDS_MOM_HIV_NUM];
	}; /* end InitialDistributions */

	/** CHRMsStats class contains aggregates and distributions of CHRMs occurrences  (CHRM = chronic-hepatic-renal-malignancies) */
	class CHRMsStats {
	public:
		/** The number of people that had a CHRM stratified by CHRM */
		int numPatientsWithCHRM[SimContext::CHRM_NUM];
		/** The number of prevalent CHRMs stratified by CHRM */
		int numPrevalentCHRM[SimContext::CHRM_NUM];
		/** The number of prevalent CHRMs stratified by CD4 */
		int numPrevalentCD4[SimContext::CD4_NUM_STRATA];
		/** The number of prevalent CHRMs stratified by both CHRM and CD4 */
		int numPrevalentCHRMCD4[SimContext::CHRM_NUM][SimContext::CD4_NUM_STRATA];
		/** The number of incident CHRMs stratified by CHRM */
		int numIncidentCHRM[SimContext::CHRM_NUM];
		/** The number of incident CHRMs stratified by CD4 */
		int numIncidentCD4[SimContext::CD4_NUM_STRATA];
		/** The number of incident CHRMs stratified by both CHRM and CD4 */
		int numIncidentCHRMCD4[SimContext::CHRM_NUM][SimContext::CD4_NUM_STRATA];
		/** The number of CHRM related deaths stratified by CHRM */
		int numDeathsCHRM[SimContext::CHRM_NUM];
		/** The number of CHRM related deaths stratified by CD4 */
		int numDeathsCD4[SimContext::CD4_NUM_STRATA];
		/** The number of CHRM related deaths stratified by both CHRM and CD4 */
		int numDeathsCHRMCD4[SimContext::CHRM_NUM][SimContext::CD4_NUM_STRATA];
	};

	/** OIStats class contains aggregates and distributions of OI occurrences */
	class OIStats {
	public:
		// Primary, secondary and detected OI counts,
		//	stratified by OI type, CD4 level, and OI type x CD4 level
		/** Primary OI count stratified by OI type */
		int numPrimaryOIsOI[SimContext::OI_NUM];
		/** Primary OI count stratified by CD4 strata */
		int numPrimaryOIsCD4[SimContext::CD4_NUM_STRATA];
		/** Primary OI count stratified by OI type and CD4 strata */
		int numPrimaryOIsCD4OI[SimContext::CD4_NUM_STRATA][SimContext::OI_NUM];
		/** Secondary OI count stratified by OI type */
		int numSecondaryOIsOI[SimContext::OI_NUM];
		/** Secondary OI count stratified by CD4 strata */
		int numSecondaryOIsCD4[SimContext::CD4_NUM_STRATA];
		/** Secondary OI count stratified by OI type and CD4 strata */
		int numSecondaryOIsCD4OI[SimContext::CD4_NUM_STRATA][SimContext::OI_NUM];
		/** Detected OI count stratified by OI type */
		int numDetectedOIsOI[SimContext::OI_NUM];
		/** Detected OI count stratified by CD4 strata */
		int numDetectedOIsCD4[SimContext::CD4_NUM_STRATA];
		/** Detected OI count stratified by OI type and CD4 strata */
		int numDetectedOIsCD4OI[SimContext::CD4_NUM_STRATA][SimContext::OI_NUM];
		// OI history logging - number of patients and calculated probabilities of having OI history
		//	stratified by OI x HVL, OI x CD4, and OI x HVL x CD4
		/** Total number of patients stratified by viral load (used for calculating averages) */
		int numPatientsHVL[SimContext::HVL_NUM_STRATA];
		/** Total number of patients stratified by CD4 strata (used for calculating averages) */
		int numPatientsCD4[SimContext::CD4_NUM_STRATA];
		/** Total number of patients stratified by viral load and CD4 strata (used for calculating averages) */
		int numPatientsHVLCD4[SimContext::HVL_NUM_STRATA][SimContext::CD4_NUM_STRATA];
		/** Total number of patients with a history of each OI type stratified by viral load (used for calculating averages) */
		int numPatientsOIHistoryHVL[SimContext::OI_NUM][SimContext::HVL_NUM_STRATA];
		/** Total number of patients with a history of each OI type stratified by CD4 count (used for calculating averages) */
		int numPatientsOIHistoryCD4[SimContext::OI_NUM][SimContext::CD4_NUM_STRATA];
		/** Total number of patients with a history of each OI type stratified by viral load and CD4 count (used for calculating averages) */
		int numPatientsOIHistoryHVLCD4[SimContext::OI_NUM][SimContext::HVL_NUM_STRATA][SimContext::CD4_NUM_STRATA];
		/** Proportion of patients with a history of each OI stratified by viral load */
		double probPatientsOIHistoryHVL[SimContext::OI_NUM][SimContext::HVL_NUM_STRATA];
		/** Proportion of patients with a history of each OI stratified by CD4 strata */
		double probPatientsOIHistoryCD4[SimContext::OI_NUM][SimContext::CD4_NUM_STRATA];
		/** Proportion of patients with a history of each OI stratified by viral load and CD4 strata */
		double probPatientsOIHistoryHVLCD4[SimContext::OI_NUM][SimContext::HVL_NUM_STRATA][SimContext::CD4_NUM_STRATA];
		/** Number of months spent at each viral load strata (used for calculating averages) */
		int numMonthsHVL[SimContext::HVL_NUM_STRATA];
		/** Number of months spent at each CD4 strata (used for calculating averages) */
		int numMonthsCD4[SimContext::CD4_NUM_STRATA];
		/** Number of months spent at each viral load and CD4 strata combination (used for calculating averages) */
		int numMonthsHVLCD4[SimContext::HVL_NUM_STRATA][SimContext::CD4_NUM_STRATA];
		/** Number of months spent at each viral load strata with a history of each OI */
		int numMonthsOIHistoryHVL[SimContext::OI_NUM][SimContext::HVL_NUM_STRATA];
		/** Number of months spent at each CD4 strata with a history of each OI */
		int numMonthsOIHistoryCD4[SimContext::OI_NUM][SimContext::CD4_NUM_STRATA];
		/** Number of months spent at each viral load strata and CD4 strata combination with a history of each OI */
		int numMonthsOIHistoryHVLCD4[SimContext::OI_NUM][SimContext::HVL_NUM_STRATA][SimContext::CD4_NUM_STRATA];
		/** Calculated proportion of months spent with a history of OI in each viral load strata*/
		double probMonthsOIHistoryHVL[SimContext::OI_NUM][SimContext::HVL_NUM_STRATA];
		/** Calculated proportion of months spent with a history of OI in each CD4 strata*/
		double probMonthsOIHistoryCD4[SimContext::OI_NUM][SimContext::CD4_NUM_STRATA];
		/** Calculated proportion of months spent with a history of OI in each viral load and CD4 strata combination */
		double probMonthsOIHistoryHVLCD4[SimContext::OI_NUM][SimContext::HVL_NUM_STRATA][SimContext::CD4_NUM_STRATA];
	}; /* end OIStats */

	/** DeathStats contains the statistics and distributions of causes of deaths */
	class DeathStats {
	public:
		// Number of deaths stratified by Cause, CD4, and Cause x CD4
		/** Number of deaths stratified by death cause */
		int numDeathsType[SimContext::DTH_NUM_CAUSES];
		/** Number of deaths stratified by CD4 strata */
		int numDeathsCD4[SimContext::CD4_NUM_STRATA];
		/** Number of deaths stratified by death cause and CD4 strata */
		int numDeathsCD4Type[SimContext::CD4_NUM_STRATA][SimContext::DTH_NUM_CAUSES];
		/** Number of deaths stratified by care status */
		int numDeathsCare[SimContext::HIV_CARE_NUM];
		/** Number of deaths stratified by death cause and care status */
		int numDeathsCareType[SimContext::HIV_CARE_NUM][SimContext::DTH_NUM_CAUSES];

		/** Total Number of deaths due to Tox*/
		int numToxDeaths;
		/** Number of deaths due to Tox stratified by cd4*/
		int numToxDeathsCD4[SimContext::CD4_NUM_STRATA];
		/** Number of deaths due to Tox stratified by cd4 and vl*/
		int numToxDeathsCD4HVL[SimContext::CD4_NUM_STRATA][SimContext::HVL_NUM_STRATA];
		/** Number of deaths due to Tox stratified by CD4 VL and OI Hist*/
		int numToxDeathsCD4HVLOIHist[SimContext::CD4_NUM_STRATA][SimContext::HVL_NUM_STRATA][SimContext::OI_NUM];
		/** probability of having oi history for those who died from tox stratifed by cd4 and hvl and OI */
		double probOiHistToxDeathsCD4HVLOIHist[SimContext::CD4_NUM_STRATA][SimContext::HVL_NUM_STRATA][SimContext::OI_NUM];
		/** distribution of people who died from tox based on cd4 and hvl*/
		double hvlDistribToxDeathCD4HVL[SimContext::CD4_NUM_STRATA][SimContext::HVL_NUM_STRATA];

		/** Number of deaths of patients with CHRMs stratified by death cause and CHRM type**/
		int numDeathsWithCHRMsTypeCHRM[SimContext::DTH_NUM_CAUSES][SimContext::CHRM_NUM];
		/** Number of deaths of patients without CHRMs stratified by death cause**/
		int numDeathsWithoutCHRMsType[SimContext::DTH_NUM_CAUSES];
		/** Number of HIV negative deaths */
		int numDeathsUninfected;
		/** Number of deaths stratified by HVL and CD4 combination */
		int numDeathsHVLCD4[SimContext::HVL_NUM_STRATA][SimContext::CD4_NUM_STRATA];
		/** Number of deaths stratified by HVL */
		int numDeathsHVL[SimContext::HVL_NUM_STRATA];
		// Number of chronic and non-AIDS deaths stratified by CD4 and OI history
		/** Number of chronic AIDS deaths with no OI history */
		int numChronicAIDSDeathsNoOIHistory;
		/** Number of chronic AIDS deaths with no OI history stratified by CD4 strata */
		int numChronicAIDSDeathsNoOIHistoryCD4[SimContext::CD4_NUM_STRATA];
		/** Number of chronic AIDS deaths with OI history */
		int numChronicAIDSDeathsOIHistory;
		/** Number of chronic AIDS deaths with OI history stratified by CD4 strata */
		int numChronicAIDSDeathsOIHistoryCD4[SimContext::CD4_NUM_STRATA];
		/** Number of non-AIDS deaths with no OI history */
		int numNonAIDSDeathsNoOIHistory;
		/** Number of non-AIDS deaths with no OI history stratified by CD4 strata */
		int numNonAIDSDeathsNoOIHistoryCD4[SimContext::CD4_NUM_STRATA];
		/** Number of non-AIDS deaths with OI history */
		int numNonAIDSDeathsOIHistory;
		/** Number of non-AIDS deaths with OI history stratified by CD4 strata */
		int numNonAIDSDeathsOIHistoryCD4[SimContext::CD4_NUM_STRATA];
		/** Sum of CD4's of all patients who died from art Tox*/
		double ToxDeathsCD4Sum;
		/** Sum of Squares of CD4's of all patients who died from Tox*/
		double ToxDeathsCD4SumSquares;
		/** mean CD4 of all patients who died from art Tox*/
		double ToxDeathsCD4Mean;
		/** std dev CD4 of all patients who died from art Tox*/
		double ToxDeathsCD4StdDev;

	}; /* end DeathStats */

	/** OverallSurvival stats contains aggregate survival statistics, separate from
		SurvivalSummary since these will only be calculated for all cohorts and not subset groups */
	class OverallSurvival {
	public:
		// life months without OI history, with OI history, and both
		//	stratified by totals and CD4
		/** Total life months lived without OI history */
		double LMsNoOIHistory;
		/** Total life months lived without OI history stratified by CD4 strata */
		double LMsNoOIHistoryCD4[SimContext::CD4_NUM_STRATA];
		/** Total life months lived with OI history */
		double LMsOIHistory;
		/** Total life months lived with OI history stratified by CD4 strata */
		double LMsOIHistoryCD4[SimContext::CD4_NUM_STRATA];
		/** Total life months lived */
		double LMsTotal;
		/** Total life months lived stratified by CD4 strata */
		double LMsTotalCD4[SimContext::CD4_NUM_STRATA];
		// life months stratified by HVL and HVL setpoint
		/** Total life months lived stratified by HVL strata */
		double LMsHVL[SimContext::HVL_NUM_STRATA];
		/** Total life months lived stratified by setpoint HVL strata */
		double LMsHVLSetpoint[SimContext::HVL_NUM_STRATA];
		// life months with and without OI history, stratified by OI type
		/** Total life months lived without OI history stratified by OI type */
		double LMsNoOIHistoryOIs[SimContext::OI_NUM];
		/** Total life months lived with OI history stratified by OI type */
		double LMsOIHistoryOIs[SimContext::OI_NUM];
		/** Total life months lived with CHRM History stratified by CHRM type */
		double LMsCHRMHistoryCHRMs[SimContext::CHRM_NUM];
		// life months stratified by HIV state and HIV positive total
		/** Total life months lived with HIV */
		double LMsHIVPositive;
		/** Total life months lived in each HIV state */
		double LMsHIVState[SimContext::HIV_ID_NUM];
		/** Total quality adjusted life months lived with HIV */
		double QALMsHIVPositive;
		/** Total quality adjusted life months lived in each HIV state */
		double QALMsHIVState[SimContext::HIV_ID_NUM];
		// life months in screening state and regular model
		/** Total life months lived in the screening state */
		double LMsInScreening;
		/** Total quality adjusted life months lived in the screening state */
		double QALMsInScreening;
		/** Total life months lived in "regular CEPAC" (i.e. non-screening state) */
		double LMsInRegularCEPAC;
		// life months by gender
		/** Total life months lived stratified by gender */
		double LMsGender[SimContext::GENDER_NUM];
		/** Total quality adjusted life months lived stratified by gender */
		double QALMsGender[SimContext::GENDER_NUM];
	}; /* end OverallSurvival */

	/** OverallCosts class contains aggregate cost stats */
	class OverallCosts {
	public:
		// costs without OI history, with OI history, and both
		//	stratified by totals and CD4
		/** Total costs accrued without OI history */
		double costsNoOIHistory;
		/** Total costs accrued without OI history stratified by CD4 */
		double costsNoOIHistoryCD4[SimContext::CD4_NUM_STRATA];
		/** Total costs accrued with OI history */
		double costsOIHistory;
		/** Total costs accrued with OI history stratified by CD4 */
		double costsOIHistoryCD4[SimContext::CD4_NUM_STRATA];
		/** Total costs accrued */
		double costsTotal;
		/** Total costs accrued stratified by CD4 */
		double costsTotalCD4[SimContext::CD4_NUM_STRATA];
		// costs stratified by HVL and HVL setpoint
		/** Total costs accrued stratified by HVL */
		double costsHVL[SimContext::HVL_NUM_STRATA];
		/** Total costs accrued stratified by HVL setpoint */
		double costsHVLSetpoint[SimContext::HVL_NUM_STRATA];
		// direct medical costs for prophylaxis, stratified by total, OI, and OI x proph line
		/** Total direct medical costs for prophylaxis */
		double directCostsProph;
		/** Direct medical costs for prophylaxis stratified by OI type */
		double directCostsProphOIs[SimContext::OI_NUM];
		/** Direct medical costs for prophylaxis stratified by OI type and prophylaxis line */
		double directCostsProphOIsProph[SimContext::OI_NUM][SimContext::PROPH_NUM];
		// direct costs for ART, stratified by total and ART line
		/** Direct costs for ART */
		double directCostsART;
		/** Direct costs for ART stratified by ART line*/
		double directCostsARTLine[SimContext::ART_NUM_LINES];
		// costs stratified by HIV states and total for HIV positives
		/** Total costs for HIV positive patients */
		double costsHIVPositive;
		/** Total costs stratified by HIV states */
		double costsHIVState[SimContext::HIV_ID_NUM];
		// costs for various screening and testing
		/** Total costs for CD4 tests */
		double costsCD4Testing;
		/** Total costs for HVL tests */
		double costsHVLTesting;
		/** Total costs for clinic visits */
		double costsClinicVisits;
		/** Total costs for HIV screening tests */
		double costsHIVScreeningTests;
		/** Total costs for HIV screening miscellaneous costs */
		double costsHIVScreeningMisc;
		/** Total Costs for Lab Staging tests */
		double costsLabStagingTests;
		/** Total costs for Lab Staging misc costs*/
		double costsLabStagingMisc;
		// undiscounted costs stratified by cost type and unclassified costs
		/** Total undiscounted costs stratified by cost type */
		double totalUndiscountedCosts[SimContext::COST_NUM_TYPES];
		/** Total undiscounted unclassified costs */
		double totalUndiscountedCostsUnclassified;
		// costs for drugs and toxicities
		/** Costs due to drugs */
		double costsDrugs;
		/**costs due to interventions*/
		double costsInterventions;
		/** Costs due to toxicity*/
		double costsToxicity;
		/** Costs due to CHRMs*/
		double costsCHRMs[SimContext::CHRM_NUM];
		/** Total costs stratified by gender */
		double costsGender[SimContext::GENDER_NUM];
		/** Total costs accrued in simplified pediatric model*/
		double costsPeds;
	}; /* end OverallCosts */

	/** TBStats class contains stats about TB occurrences, treatments, and costs */
	class TBStats {
	public:
		/** Total number with TB disease at model entry, stratified by TB type x TB state */
		int numInStateAtEntry[SimContext::TB_NUM_STRAINS][SimContext::TB_NUM_STATES];
		// number with various TB treatment effects, stratified by TB type x TB treatment stage
		/** Total number who start on TB treatment stratified by TB type x TB treatment stage */
		int numStartOnTreatment[SimContext::TB_NUM_STRAINS][SimContext::TB_TREATM_STAGE_NUM];
		/** Total number who drop out of TB treatment stratified by TB type x TB treatment stage */
		int numDropoutTreatment[SimContext::TB_NUM_STRAINS][SimContext::TB_TREATM_STAGE_NUM];
		/** Total number who are cured of TB when they drop out of treatment stratified by TB type x TB treatment stage */
		int numCuredAtTreatmentDropout[SimContext::TB_NUM_STRAINS][SimContext::TB_TREATM_STAGE_NUM];
		/** Total number who finish treatment stratified by TB type x TB treatment stage */
		int numFinishTreatment[SimContext::TB_NUM_STRAINS][SimContext::TB_TREATM_STAGE_NUM];
		/** Total number who are cured after finishing treatment stratified by TB type x TB treatment stage */
		int numCuredAtTreatmentFinish[SimContext::TB_NUM_STRAINS][SimContext::TB_TREATM_STAGE_NUM];
		/** Total number with increased resistance after finishing treatment stratified by TB type x TB treatment stage */
		int numIncreaseResistanceAtTreatmentFinish[SimContext::TB_NUM_STRAINS][SimContext::TB_TREATM_STAGE_NUM];
		// total number of latent and active infections, stratified by TB type
		/** Total number of latent infections, stratified by TB type */
		int numLatentInfections[SimContext::TB_NUM_STRAINS];
		/** Total number of active infections, stratified by TB type */
		int numActiveInfections[SimContext::TB_NUM_STRAINS];
		// number of reactivations and reinfections from latent TB, stratified by TB type
		/** Total number of reactivations from latent TB stratified by TB type */
		int numReactivationsLatent[SimContext::TB_NUM_STRAINS];
		/** Total number of reinfections from latent TB stratified by TB type */
		int numReinfectionsLatent[SimContext::TB_NUM_STRAINS];
		// number of relapses and reinfections from history of active TB, stratified by TB type
		/** Total number of relapses from history of active TB stratified by TB type */
		int numRelapsesHistoryActive[SimContext::TB_NUM_STRAINS];
		/** Total number of reinfections from history of active TB stratified by TB type */
		int numReinfectionsHistoryActive[SimContext::TB_NUM_STRAINS];
		// number of spontaneous resolutions and deaths from active TB, stratified by TB type
		/** Total number of spontaneous resolutions from active TB, stratified by TB type */
		int numSpontaneousResolutions[SimContext::TB_NUM_STRAINS];
		/** Total number of deaths from active TB, stratified by TB type */
		int numDeaths[SimContext::TB_NUM_STRAINS];
		// number of toxicities stratified by treatment stage and proph line
		/** Total number of minor toxicities stratified by treatment stage */
		int numTreatmentMinorToxicity[SimContext::TB_TREATM_STAGE_NUM];
		/** Total number of major toxicities stratified by treatment stage */
		int numTreatmentMajorToxicity[SimContext::TB_TREATM_STAGE_NUM];
		/** Total number of minor toxicities stratified by proph line */
		int numProphMinorToxicity[SimContext::PROPH_NUM];
		/** Total number of major toxicities stratified by proph line */
		int numProphMajorToxicity[SimContext::PROPH_NUM];
	}; /* end TBStats */

	/** LTFUStats contains stats about patient loss to follow up and return to care */
	class LTFUStats {
	public:
		// totals for patients that were ever lost and deaths while lost
		/** Total number of patients ever lost*/
		int numPatientsLost;
		/** Total number of patients who ever returned */
		int numPatientsReturned;
		/** Total number of deaths occurring while lost from care */
		int numDeathsWhileLost;
		// totals for times ltfu and rtc stratified by CD4
		/** Total number of LTFU occurrences */
		int numLostToFollowUp;
		/** Total number of LTFU occurrences stratified by CD4*/
		int numLostToFollowUpCD4[SimContext::CD4_NUM_STRATA];
		/** Total number of RTC occurrences */
		int numReturnToCare;
		/** Total number of RTC occurrences stratified by CD4*/
		int numReturnToCareCD4[SimContext::CD4_NUM_STRATA];
		/** Total number of deaths occurring while lost from care stratified by CD4*/
		int numDeathsWhileLostCD4[SimContext::CD4_NUM_STRATA];
		// time lost before returning to care
		/** Total time lost before returning to care */
		double monthsLostBeforeReturnSum;
		/** Average time lost before returning to care per patient*/
		double monthsLostBeforeReturnMean;
		/** Sum of squares of total time lost before returning to care (for calculating standard deviation) */
		double monthsLostBeforeReturnSumSquares;
		/** Standard deviation of time lost before returning to care per patient*/
		double monthsLostBeforeReturnStdDev;
		// number lost to follow up during ART line or pre/post ART
		/** Number lost to follow up while on each ART line */
		int numLostToFollowUpART[SimContext::ART_NUM_LINES];
		/** Number lost to follow up pre-ART */
		int numLostToFollowUpPreART;
		/** Number lost to follow up post-ART */
		int numLostToFollowUpPostART;
		// number return to care on prev ART line or subsequent ART line
		/** Number who returned to care and started previous ART line */
		int numReturnOnPrevART[SimContext::ART_NUM_LINES];
		/** Number who returned to care and started the next ART line */
		int numReturnOnNextART[SimContext::ART_NUM_LINES];
		/** Number who returned to care pre-ART */
		int numReturnToCarePreART;
		/** Number who returned to care post-ART */
		int numReturnToCarePostART;
		// number of deaths while lost by prev ART line
		/** Number of deaths while lost stratified by most recent ART line taken */
		int numDeathsWhileLostART[SimContext::ART_NUM_LINES];
		/** Number of deaths while lost pre-ART */
		int numDeathsWhileLostPreART;
		/** Number of deaths while lost post-ART */
		int numDeathsWhileLostPostART;
	};

	/** ProphStats class contains statistics about prophylaxis treatments and toxicities */
	class ProphStats {
	public:
		// number of toxicities stratified by OI type and OI type x proph line
		/** Number of minor toxicities stratified by OI type and prophylaxis line */
		int numMinorToxicity[SimContext::OI_NUM][SimContext::PROPH_NUM];
		/** Number of minor toxicities stratified by OI type */
		int numMinorToxicityTotal[SimContext::OI_NUM];
		/** Number of major toxicities stratified by OI type and prophylaxis line */
		int numMajorToxicity[SimContext::OI_NUM][SimContext::PROPH_NUM];
		/** Number of major toxicities stratified by OI type */
		int numMajorToxicityTotal[SimContext::OI_NUM];
		// average true CD4, observed CD4, and times prophylaxis initiated for primary and
		//	secondary OIs, stratified by OI type x proph line
		/** Total true CD4 initiated for primary and secondary OIs, stratified by OI type and proph line*/
		double trueCD4InitProphSum[SimContext::PROPH_NUM_TYPES][SimContext::OI_NUM][SimContext::PROPH_NUM];
		/** Average true CD4 initiated for primary and secondary OIs, stratified by OI type and proph line*/
		double trueCD4InitProphMean[SimContext::PROPH_NUM_TYPES][SimContext::OI_NUM][SimContext::PROPH_NUM];
		/** Total observed CD4 initiated for primary and secondary OIs, stratified by OI type and proph line*/
		double observedCD4InitProphSum[SimContext::PROPH_NUM_TYPES][SimContext::OI_NUM][SimContext::PROPH_NUM];
		/** Average observed CD4 initiated for primary and secondary OIs, stratified by OI type and proph line*/
		double observedCD4InitProphMean[SimContext::PROPH_NUM_TYPES][SimContext::OI_NUM][SimContext::PROPH_NUM];
		/** Total number of times primary and secondary prophylaxis was started, stratified by OI type and proph line*/
		int numTimesInitProph[SimContext::PROPH_NUM_TYPES][SimContext::OI_NUM][SimContext::PROPH_NUM];
	}; /* end ProphStats */

	/**  ARTStats contains stats about ART treatments and toxicities */
	class ARTStats {
	public:
		// months in suppressed state, stratified by total and ART line
		/** Total months in suppressed state */
		int monthsSuppressed;
		/** Total months in suppressed state, stratified by ART line */
		int monthsSuppressedLine[SimContext::ART_NUM_LINES];
		// months in partially suppressed and failed state,
		//	stratified ART line, HVL, and ART line x HVL
		/** Total months in partially suppressed state stratified by ART line */
		int monthsPartiallySuppressedLine[SimContext::ART_NUM_LINES];
		/** Total months in partially suppressed state stratified by HVL */
		int monthsPartiallySuppressedHVL[SimContext::HVL_NUM_STRATA];
		/** Total months in partially suppressed state stratified by ART line and HVL */
		int monthsPartiallySuppressedLineHVL[SimContext::ART_NUM_LINES][SimContext::HVL_NUM_STRATA];
		/** Total months in failed state stratified by ART line */
		int monthsFailedLine[SimContext::ART_NUM_LINES];
		/** Total months in failed state stratified by HVL */
		int monthsFailedHVL[SimContext::HVL_NUM_STRATA];
		/** Total months in failed state stratified by ART line and HVL */
		int monthsFailedLineHVL[SimContext::ART_NUM_LINES][SimContext::HVL_NUM_STRATA];
		// statistics at ART initiation, stratified by ART line x response type
		/** Total number initiating ART stratified by ART line */
		int numOnARTAtInit[SimContext::ART_NUM_LINES];
		/** Total number initiating ART stratified by ART line, heterogeneity outcomes, and response type */
		int numOnARTAtInitResp[SimContext::ART_NUM_LINES][SimContext::HET_NUM_OUTCOMES][SimContext::RESP_NUM_TYPES];
		/** Total true CD4 counts at ART initialization stratified by ART line */
		double trueCD4AtInitSum[SimContext::ART_NUM_LINES];
		/** Average true CD4 counts at ART initialization stratified by ART line */
		double trueCD4AtInitMean[SimContext::ART_NUM_LINES];
		/** Total true CD4 counts at ART initialization stratified by ART line,hetergeneity outcomes, and response type */
		double trueCD4AtInitSumResp[SimContext::ART_NUM_LINES][SimContext::HET_NUM_OUTCOMES][SimContext::RESP_NUM_TYPES];
		/** Average true CD4 counts at ART initialization stratified by ART line and response type */
		double trueCD4AtInitMeanResp[SimContext::ART_NUM_LINES][SimContext::HET_NUM_OUTCOMES][SimContext::RESP_NUM_TYPES];
		/** Total observed CD4 counts at ART initialization stratified by ART line */
		double observedCD4AtInitSum[SimContext::ART_NUM_LINES];
		/** Average observed CD4 counts at ART initialization stratified by ART line */
		double observedCD4AtInitMean[SimContext::ART_NUM_LINES];
		/** Total observed CD4 counts at ART initialization stratified by ART line and response type */
		double observedCD4AtInitSumResp[SimContext::ART_NUM_LINES][SimContext::HET_NUM_OUTCOMES][SimContext::RESP_NUM_TYPES];
		/** Average observed CD4 counts at ART initialization stratified by ART line and response type */
		double observedCD4AtInitMeanResp[SimContext::ART_NUM_LINES][SimContext::HET_NUM_OUTCOMES][SimContext::RESP_NUM_TYPES];
		/** Histogram of ART efficacies drawn at ART initialization stratified by ART line and efficacy type */
		int numDrawEfficacyAtInit[SimContext::ART_NUM_LINES][SimContext::ART_EFF_NUM_TYPES];
		/** Histogram of ART efficacies drawn at ART initialization stratified by ART line and efficacy type and response type */
		int numDrawEfficacyAtInitResp[SimContext::ART_NUM_LINES][SimContext::ART_EFF_NUM_TYPES][SimContext::HET_NUM_OUTCOMES][SimContext::RESP_NUM_TYPES];
		/** Histogram of CD4 response type drawn at ART initialization stratified by ART line and CD4 response type */
		int numCD4ResponseTypeAtInit[SimContext::ART_NUM_LINES][SimContext::CD4_RESPONSE_NUM_TYPES];
		/** Histogram of CD4 response type drawn at ART initialization stratified by ART line and CD4 response type  and ART response type */
		int numCD4ResponseTypeAtInitResp[SimContext::ART_NUM_LINES][SimContext::CD4_RESPONSE_NUM_TYPES][SimContext::HET_NUM_OUTCOMES][SimContext::RESP_NUM_TYPES];
		/** Histogram of risk factors at ART initialization stratified by ART line and risk factor */
		int numWithRiskFactorAtInit[SimContext::ART_NUM_LINES][SimContext::RISK_FACT_NUM];
		/** Histogram of risk factors at ART initialization stratified by ART line and risk factor and ART response type */
		int numWithRiskFactorAtInitResp[SimContext::ART_NUM_LINES][SimContext::RISK_FACT_NUM][SimContext::HET_NUM_OUTCOMES][SimContext::RESP_NUM_TYPES];
		// statistics at ART true failure, stratified by ART line x response type
		/** Number of ART true failure, stratified by ART line */
		int numTrueFailure[SimContext::ART_NUM_LINES];
		/** Number of ART true failure, stratified by ART line x response type */
		int numTrueFailureResp[SimContext::ART_NUM_LINES][SimContext::HET_NUM_OUTCOMES][SimContext::RESP_NUM_TYPES];
		/** Total true CD4 at true failure stratified by ART line */
		double trueCD4AtTrueFailureSum[SimContext::ART_NUM_LINES];
		/** Average true CD4 at true failure stratified by ART line */
		double trueCD4AtTrueFailureMean[SimContext::ART_NUM_LINES];
		/** Total true CD4 at true failure stratified by ART line x response type */
		double trueCD4AtTrueFailureSumResp[SimContext::ART_NUM_LINES][SimContext::HET_NUM_OUTCOMES][SimContext::RESP_NUM_TYPES];
		/** Average true CD4 at true failure stratified by ART line x response type */
		double trueCD4AtTrueFailureMeanResp[SimContext::ART_NUM_LINES][SimContext::HET_NUM_OUTCOMES][SimContext::RESP_NUM_TYPES];
		/** Total observed CD4 at true failure stratified by ART line */
		double observedCD4AtTrueFailureSum[SimContext::ART_NUM_LINES];
		/** Average observed CD4 at true failure stratified by ART line */
		double observedCD4AtTrueFailureMean[SimContext::ART_NUM_LINES];
		/** Total observed CD4 at true failure stratified by ART line x response type */
		double observedCD4AtTrueFailureSumResp[SimContext::ART_NUM_LINES][SimContext::HET_NUM_OUTCOMES][SimContext::RESP_NUM_TYPES];
		/** Average observed CD4 at true failure stratified by ART line x response type */
		double observedCD4AtTrueFailureMeanResp[SimContext::ART_NUM_LINES][SimContext::HET_NUM_OUTCOMES][SimContext::RESP_NUM_TYPES];
		/** Total months to true failure stratified by ART line */
		double monthsToTrueFailureSum[SimContext::ART_NUM_LINES];
		/** Average months to true failure stratified by ART line */
		double monthsToTrueFailureMean[SimContext::ART_NUM_LINES];
		/** Total months to true failure stratified by ART line and response type */
		double monthsToTrueFailureSumResp[SimContext::ART_NUM_LINES][SimContext::HET_NUM_OUTCOMES][SimContext::RESP_NUM_TYPES];
		/** Average months to true failure stratified by ART line and response type */
		double monthsToTrueFailureMeanResp[SimContext::ART_NUM_LINES][SimContext::HET_NUM_OUTCOMES][SimContext::RESP_NUM_TYPES];
		/** Sum of squares of months to true failure stratified by ART line (used to calculate standard deviation) */
		double monthsToTrueFailureSumSquares[SimContext::ART_NUM_LINES];
		/** Standard deviation of months to true failure stratified by ART line */
		double monthsToTrueFailureStdDev[SimContext::ART_NUM_LINES];
		/** Sum of squares of months to true failure stratified by ART line and response type (used to calculate standard deviation) */
		double monthsToTrueFailureSumSquaresResp[SimContext::ART_NUM_LINES][SimContext::HET_NUM_OUTCOMES][SimContext::RESP_NUM_TYPES];
		/** Standard deviation of months to true failure stratified by ART line and response type */
		double monthsToTrueFailureStdDevResp[SimContext::ART_NUM_LINES][SimContext::HET_NUM_OUTCOMES][SimContext::RESP_NUM_TYPES];
		// total number and true/observed CD4 at observed failure,
		//	stratified by ART line and ART line x failure type
		/** Total number of observed failures stratified by ART line */
		int numObservedFailure[SimContext::ART_NUM_LINES];
		/** Total number of observed failures stratified by ART line and failure type */
		int numObservedFailureType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		/** Total number of observed failures after true failure stratified by ART line */
		int numObservedFailureAfterTrue[SimContext::ART_NUM_LINES];
		/** Total number of observed failures after true failure stratified by ART line and failure type */
		int numObservedFailureAfterTrueType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		/** Total number never observed to fail stratified by ART line */
		int numNeverObservedFailure[SimContext::ART_NUM_LINES];
		/** Total true CD4 at observed failure stratified by ART line */
		double trueCD4AtObservedFailureSum[SimContext::ART_NUM_LINES];
		/** Average true CD4 at observed failure stratified by ART line */
		double trueCD4AtObservedFailureMean[SimContext::ART_NUM_LINES];
		/** Total true CD4 at observed failure stratified by ART line and failure type */
		double trueCD4AtObservedFailureSumType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		/** Average true CD4 at observed failure stratified by ART line and failure type */
		double trueCD4AtObservedFailureMeanType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		/** Total observed CD4 at observed failure stratified by ART line */
		double observedCD4AtObservedFailureSum[SimContext::ART_NUM_LINES];
		/** Average observed CD4 at observed failure stratified by ART line */
		double observedCD4AtObservedFailureMean[SimContext::ART_NUM_LINES];
		/** Total observed CD4 at observed failure stratified by ART line and failure type */
		double observedCD4AtObservedFailureSumType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		/** Average observed CD4 at observed failure stratified by ART line and failure type */
		double observedCD4AtObservedFailureMeanType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		// average and std dev of number of months between true failure and observed failure,
		//	stratified by ART line and ART line x failure type
		/** Total months to observed failure stratified by ART line */
		double monthsToObservedFailureSum[SimContext::ART_NUM_LINES];
		/** Average months to observed failure stratified by ART line */
		double monthsToObservedFailureMean[SimContext::ART_NUM_LINES];
		/** Total months to observed failure stratified by ART line and failure type */
		double monthsToObservedFailureSumType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		/** Average months to observed failure stratified by ART line and failure type */
		double monthsToObservedFailureMeanType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		/** Total sum of squares of months to observed failure stratified by ART line (used for calculating standard deviation) */
		double monthsToObservedFailureSumSquares[SimContext::ART_NUM_LINES];
		/** Standard deviation of months to observed failure stratified by ART line */
		double monthsToObservedFailureStdDev[SimContext::ART_NUM_LINES];
		/** Total sum of squares of months to observed failure stratified by ART line and failure type (used for calculating standard deviation) */
		double monthsToObservedFailureSumSquaresType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		/** Standard deviation of months to observed failure stratified by ART line and failure type */
		double monthsToObservedFailureStdDevType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_FAIL_TYPES];
		// number who stop ART and true/observed CD4, stratified by ART line and ART line x stop type
		/** Total number who stop ART stratified by ART line*/
		int numStop[SimContext::ART_NUM_LINES];
		/** Total number who stop ART stratified by ART line x stop type */
		int numStopType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		/** Total number who stop ART after true failure stratified by ART line */
		int numStopAfterTrueFailure[SimContext::ART_NUM_LINES];
		/** Total number who stop ART after true failure stratified by ART line x stop type */
		int numStopAfterTrueFailureType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		/** Total number who never stop ART stratified by ART line*/
		int numNeverStop[SimContext::ART_NUM_LINES];
		/** Total true CD4 at ART stop, stratified by ART line */
		double trueCD4AtStopSum[SimContext::ART_NUM_LINES];
		/** Average true CD4 at ART stop, stratified by ART line */
		double trueCD4AtStopMean[SimContext::ART_NUM_LINES];
		/** Total true CD4 at ART stop, stratified by ART line x stop type */
		double trueCD4AtStopSumType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		/** Average true CD4 at ART stop, stratified by ART line x stop type*/
		double trueCD4AtStopMeanType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		/** Total observed CD4 at ART stop, stratified by ART line */
		double observedCD4AtStopSum[SimContext::ART_NUM_LINES];
		/** Average observed CD4 at ART stop, stratified by ART line */
		double observedCD4AtStopMean[SimContext::ART_NUM_LINES];
		/** Total observed CD4 at ART stop, stratified by ART line x stop type*/
		double observedCD4AtStopSumType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		/** Average observed CD4 at ART stop, stratified by ART line x stop type*/
		double observedCD4AtStopMeanType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		// months on art before stopping, stratified by ART line and ART line x stop type
		/** Total months on art before stopping, stratified by ART line */
		double monthsToStopSum[SimContext::ART_NUM_LINES];
		/** Average months on art before stopping, stratified by ART line */
		double monthsToStopMean[SimContext::ART_NUM_LINES];
		/** Total months on art before stopping, stratified by ART line x stop type */
		double monthsToStopSumType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		/** Average months on art before stopping, stratified by ART line x stop type */
		double monthsToStopMeanType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		/** Sum of squares of months on art before stopping, stratified by ART line (for calculating standard deviation) */
		double monthsToStopSumSquares[SimContext::ART_NUM_LINES];
		/** Standard deviation of months on art before stopping, stratified by ART line */
		double monthsToStopStdDev[SimContext::ART_NUM_LINES];
		/** Sum of squares of months on art before stopping, stratified by ART line x stop type (for calculating standard deviation) */
		double monthsToStopSumSquaresType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		/** Standard deviation of months on art before stopping, stratified by ART line x stop type */
		double monthsToStopStdDevType[SimContext::ART_NUM_LINES][SimContext::ART_NUM_STOP_TYPES];
		// number on ART, number suppressed, and HVL drop at specified month time points,
		//	stratified by ART line x month to record
		/** Number on ART stratified by ART line and month to record */
		int numOnARTAtMonth[SimContext::ART_NUM_LINES][SimContext::ART_NUM_MTHS_RECORD];
		/** Number suppressed on ART stratified by ART line and month to record */
		int numSuppressedAtMonth[SimContext::ART_NUM_LINES][SimContext::ART_NUM_MTHS_RECORD];
		/** Total number of HVL drops stratified by ART line and month to record */
		double HVLDropsAtMonthSum[SimContext::ART_NUM_LINES][SimContext::ART_NUM_MTHS_RECORD];
		/** Average number of HVL drops stratified by ART line and month to record */
		double HVLDropsAtMonthMean[SimContext::ART_NUM_LINES][SimContext::ART_NUM_MTHS_RECORD];
		/** Sum of squares of HVL drops stratified by ART line and month to record (for calculating standard deviation) */
		double HVLDropsAtMonthSumSquares[SimContext::ART_NUM_LINES][SimContext::ART_NUM_MTHS_RECORD];
		/** Standard deviation of HVL drops stratified by ART line and month to record */
		double HVLDropsAtMonthStdDev[SimContext::ART_NUM_LINES][SimContext::ART_NUM_MTHS_RECORD];
		/** Distribution at ART initiation, stratified by ART line x CD4 x HVL */
		int distributionAtInit[SimContext::ART_NUM_LINES][SimContext::CD4_NUM_STRATA][SimContext::HVL_NUM_STRATA];
		/** number of toxicity cases, stratified by ART line x tox severity x HVL */
		int numToxicityCases[SimContext::ART_NUM_LINES][SimContext::ART_NUM_TOX_SEVERITY][SimContext::HVL_NUM_STRATA];
		/** number of toxicity deaths, stratified by ART line x HVL */
		int numToxicityDeaths[SimContext::ART_NUM_LINES][SimContext::HVL_NUM_STRATA];
		// number of STI interruptions, restarts, endpoints, and patients with interruptions,
		//	stratified by ART line x STI cycle
		/** Total number of STI interruptions, stratified by ART line x STI cycle */
		int numSTIInterruptions[SimContext::ART_NUM_LINES][SimContext::STI_NUM_TRACKED];
		/** Total number of STI restarts, stratified by ART line x STI cycle */
		int numSTIRestarts[SimContext::ART_NUM_LINES][SimContext::STI_NUM_TRACKED];
		/** Total number of STI endpoints, stratified by ART line x STI cycle */
		int numSTIEndpoints[SimContext::ART_NUM_LINES][SimContext::STI_NUM_TRACKED];
		/** Total number of patients with interruptions, stratified by ART line x STI cycle */
		int numPatientsWithSTIInterruptions[SimContext::ART_NUM_LINES][SimContext::STI_NUM_TRACKED];
		// number and duartion of STI interruptions, stratified by ART line
		/** Total number of STI interruptions stratified by ART line */
		double numSTIInterruptionsSum[SimContext::ART_NUM_LINES];
		/** Average number of STI interruptions stratified by ART line */
		double numSTIInterruptionsMean[SimContext::ART_NUM_LINES];
		/** Total duration of STI interruptions stratified by ART line */
		double monthsOnSTIInterruptionSum[SimContext::ART_NUM_LINES];
		/** Average duration of STI interruptions stratified by ART line */
		double monthsOnSTIInterruptionMean[SimContext::ART_NUM_LINES];
	}; /* end ARTStats */

	/** TimeSummary class contains monthly/yearly longitudinal stats */
	class TimeSummary {
	public:
		/** Time period month or year */
		int timePeriod;
		/** number alive at this time*/
		int numAlive;
		/** number alive at this time with CHRMs**/
		int numAliveWithCHRMs;
		/** number alive at this time without CHRMs**/
		int numAliveWithoutCHRMs;
		/** number alive at this time stratified by CHRM**/
		int numAliveCHRM[SimContext::CHRM_NUM];
		/** number alive at this time, stratified by HIV state */
		int numAliveType[SimContext::HIV_ID_NUM];
		/** number alive at this time, stratified by Care state*/
		int numAliveCare[SimContext::HIV_CARE_NUM];
		/** number alive at this time stratified by HIV state and care state */
		int numAliveTypeCare[SimContext::HIV_ID_NUM][SimContext::HIV_CARE_NUM];
		/** number alive at this time with CHRMs, stratified by HIV state */
		int numAliveWithCHRMsType[SimContext::HIV_ID_NUM];
		/** number alive at this time withoutCHRMs, stratified by HIV state */
		int numAliveWithoutCHRMsType[SimContext::HIV_ID_NUM];
		/** number alive at this time, stratified by HIV state  and CHRM*/
		int numAliveTypeCHRMs[SimContext::HIV_ID_NUM][SimContext::CHRM_NUM];
		/** number of pediatrics patients alive at this time, stratified by HIV state */
		int numAlivePediatrics[SimContext::PEDS_HIV_NUM];
		/** Number of incident HIV infections */
		int numIncidentHIVInfections;
		/** Number of incident Chrms stratified by chrm*/
		int numIncidentCHRMs[SimContext::CHRM_NUM];
		/** Sum of all QOL multipliers applied that month -- reported at Ben Linas's request, 2/3/2010 -- errhode */
		double sumQOLmultipliers;
		// true and observed CD4 and HVL
		/** Total true CD4 this month */
		double trueCD4Sum;
		/** Average true CD4 this month */
		double trueCD4Mean;
		/** Total sum of squares of true CD4 this month */
		double trueCD4SumSquares;
		/** Standard deviation of true CD4 this month */
		double trueCD4StdDev;
		/** True CD4 stratified by care status */
		double trueCD4SumCare[SimContext::HIV_CARE_NUM];
		double trueCD4MeanCare[SimContext::HIV_CARE_NUM];
		double trueCD4SumSquaresCare[SimContext::HIV_CARE_NUM];
		double trueCD4StdDevCare[SimContext::HIV_CARE_NUM];
		/** Total observed CD4 this month */
		double observedCD4Sum;
		/** Average observed CD4 this month */
		double observedCD4Mean;
		/** Total sum of squares of observed CD4 this month */
		double observedCD4SumSquares;
		/** Standard deviation of observed CD4 this month */
		double observedCD4StdDev;
		/** observed CD4 stratified by care status */
		double observedCD4SumCare[SimContext::HIV_CARE_NUM];
		double observedCD4MeanCare[SimContext::HIV_CARE_NUM];
		double observedCD4SumSquaresCare[SimContext::HIV_CARE_NUM];
		double observedCD4StdDevCare[SimContext::HIV_CARE_NUM];
		/** Total true CD4 percentage this month */
		double trueCD4PercentageSum;
		/** Average true CD4 percentage this month */
		double trueCD4PercentageMean;
		/** Total sum of squares of true CD4 percentage this month */
		double trueCD4PercentageSumSquares;
		/** Standard deviation of true CD4 percentage this month */
		double trueCD4PercentageStdDev;
		/** Total true HVL this month */
		double trueHVLSum;
		/** Average true HVL this month */
		double trueHVLMean;
		/** Total sum of squares of true HVL this month */
		double trueHVLSumSquares;
		/** Standard deviation of true HVL this month */
		double trueHVLStdDev;
		/** Total observed HVL this month */
		double observedHVLSum;
		/** Average observed HVL this month */
		double observedHVLMean;
		/** Total sum of squares of observed HVL this month */
		double observedHVLSumSquares;
		/** Standard deviation of observed HVL this month */
		double observedHVLStdDev;
		/** distribution of true CD4 stratified by ART state x CD4 */
		int trueCD4ARTDistribution[SimContext::ART_NUM_STATES][SimContext::CD4_NUM_STRATA];
		/** distribution of observed CD4 stratified by CD4 */
		int observedCD4Distribution[SimContext::CD4_NUM_STRATA];
		/** distribution of observed CD4 stratifie by CD4 and Care state */
		int observedCD4DistributionCare[SimContext::CD4_NUM_STRATA][SimContext::HIV_CARE_NUM];
		/** distribution of true HVL stratified by HVL */
		int trueHVLDistribution[SimContext::HVL_NUM_STRATA];
		/** distribution of observed HVL stratified by HVL */
		int observedHVLDistribution[SimContext::HVL_NUM_STRATA];
		/** distribution of true CD4/HVL stratified by ART state x CD4 x HVL */
		int trueCD4HVLARTDistribution[SimContext::ART_NUM_STATES][SimContext::CD4_NUM_STRATA][SimContext::HVL_NUM_STRATA];
		/** number in each ART efficacy state */
		int numARTEfficacyState[SimContext::ART_EFF_NUM_TYPES];
		/** number with primary OIs, stratified by OI type */
		int numPrimaryOIs[SimContext::OI_NUM];
		/** number with secondary OIs, stratified by OI type */
		int numSecondaryOIs[SimContext::OI_NUM];
		/** number with OI history by type */
		int numWithOIHistory[SimContext::OI_NUM];
		/** number without any OI history  */
		int numWithoutOIHistory;
		/** number with first OI , stratified by OI type */
		int numWithFirstOI[SimContext::OI_NUM];
		/** total number of deaths*/
		int numDeaths;
		/** deaths among infected status*/
		int numDeathsInf[SimContext::HIV_ID_NUM];
		/** deaths by care state*/
		int numDeathsCare[SimContext::HIV_CARE_NUM];
		/** number of people with chrms stratified by Age */
		int numCHRMsAgeTotal[SimContext::CHRM_AGE_CAT_NUM];
		/** number of people with chrms stratified by Gender */
		int numCHRMsGenderTotal[SimContext::GENDER_NUM];
		/** number of people with chrms stratified by cd4 */
		int numCHRMsCD4Total[SimContext::CD4_NUM_STRATA];
		/** number with specific CHRM stratified by Age*/
		int numCHRMsAge[SimContext::CHRM_NUM][SimContext::CHRM_AGE_CAT_NUM];
		/** number with specific CHRM stratified by Sex*/
		int numCHRMsGender[SimContext::CHRM_NUM][SimContext::GENDER_NUM];
		/** number with specific CHRM stratified by CD4*/
		int numCHRMsCD4[SimContext::CHRM_NUM][SimContext::CD4_NUM_STRATA];
		/** deaths from first OI, stratified by OI type */
		int numDeathsFromFirstOI[SimContext::OI_NUM];
		/** num of deaths by cause of death type */
		int numDeathsType[SimContext::DTH_NUM_CAUSES];
		/** num of deaths of HIV + by cause of death type */
		int numHIVPosDeathsType[SimContext::DTH_NUM_CAUSES];
		/** num of deaths by cause of death type and care state */
		int numDeathsTypeCare[SimContext::DTH_NUM_CAUSES][SimContext::HIV_CARE_NUM];
		/** Number of deaths of patients with CHRMs stratified by death cause and CHRM type**/
		int numDeathsWithCHRMsTypeCHRM[SimContext::DTH_NUM_CAUSES][SimContext::CHRM_NUM];
		/**number of deaths of patients with chrms stratified by chrm type*/
		int numDeathsWithCHRMsCHRM[SimContext::CHRM_NUM];
		/** Number of deaths of patients without CHRMs stratified by death cause**/
		int numDeathsWithoutCHRMsType[SimContext::DTH_NUM_CAUSES];
		/**Total number of deaths of patients without chrms*/
		int numDeathsWithoutCHRMs;
		// costs of various testing and total monthly cost
		/** Monthly costs of CD4 testing */
		double costsCD4Testing;
		/** Monthly costs of HVL testing */
		double costsHVLTesting;
		/** Monthly costs of clinic visits */
		double costsClinicVisits;
		/** Monthly costs of HIV testing */
		double costsHIVTests;
		/** Monthly costs of miscellaneous HIV */
		double costsHIVMisc;
		/** Monthly costs of Lab Staging Testing */
		double costsLabStagingTests;
		/** Monthly costs of miscellaneous Lab Staging */
		double costsLabStagingMisc;
		/** Monthly costs due to pediatrics */
		double costsPeds;
		/** Monthly costs total */
		double totalMonthlyCohortCosts;
		/** costs of prophylaxis, stratified by OI type x proph line */
		double costsProph[SimContext::OI_NUM][SimContext::PROPH_NUM];
		/** costs of ART, stratified by ART line */
		double costsART[SimContext::ART_NUM_LINES];
		/** costs of CHRMs, stratified by CHRM */
		double costsCHRMs[SimContext::CHRM_NUM];
		/** Number on ART, stratified by ART line */
		int numOnART[SimContext::ART_NUM_LINES];
		/** Number of HIV+ in_care who are pre-ART */
		int numInCarePreART;
		/** Number of HIV+ in care who are post-ART */
		int numInCarePostART;
		/** Number starting particular ART line this month, stratified by ART line */
		int numStartingART[SimContext::ART_NUM_LINES];
		/** Number of HIV+ in_care who are starting pre-ART this month */
		int numStartingPreART;
		/** Number of HIV+ in care who are starting post-ART this month*/
		int numStartingPostART;
		/** Number on ART including continuing costs, stratified by ART line */
		int numOnARTIncludingContinuedCosts[SimContext::ART_NUM_LINES];
		/** Number lost to follow up stratified by ART line */
		int numLostToFollowUpART[SimContext::ART_NUM_LINES];
		/** Total number of people becoming lost this month */
		int numStartingLostToFollowUpART[SimContext::ART_NUM_LINES];
		/** Number lost to follow up who are pre-ART */
		int numLostToFollowUpPreART;
		/** Total number of people becoming lost this month who are pre art*/
		int numStartingLostToFollowUpPreART;
		/** Number lost to follow up who are post-ART */
		int numLostToFollowUpPostART;
		/** Total number of people becoming lost this month who are post-ART*/
		int numStartingLostToFollowUpPostART;
		/** number return to care on prev ART line, stratified by ART line */
		int numReturnOnPrevART[SimContext::ART_NUM_LINES];
		/** number return to care on subsequent ART line, stratified by ART line */
		int numReturnOnNextART[SimContext::ART_NUM_LINES];
		/** number return to care who are pre-ART */
		int numReturnToCarePreART;
		/** number return to care who are post-ART */
		int numReturnToCarePostART;
		/** num of death while LTFU stratified by ART line */
		int numDeathsWhileLostART[SimContext::ART_NUM_LINES];
		/** num of death while LTFU who are pre-ART */
		int numDeathsWhileLostPreART;
		/** num of death while LTFU who are post-ART */
		int numDeathsWhileLostPostART;
		/** num of death not detected while HIV+*/
		int numDeathsUndetectedInfected;
		/** num of death detected while in care(linked) */
		int numDeathsDetectedLinked;
		/** num of death Detected while never linked */
		int numDeathsDetectedNeverLinked;
		/** num of deaths Detected while LTFU (previously in care)*/
		int numDeathsDetectedLTFU;
		/** num of death hiv negative */
		int numDeathsUninfected;
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
	/** Pointer to the associated simulation context */
	SimContext *simContext;
	/** Stats file name */
	string statsFileName;
	/** Stats file pointer */
	FILE *statsFile;

	/** Statistics subclass object */
	PopulationSummary popSummary;
	/** Statistics subclass object */
	HIVScreening hivScreening;
	/** Statistics subclass object */
	SurvivalStats survivalStats[NUM_SURVIVAL_GROUPS];
	/** Statistics subclass object */
	InitialDistributions initialDistributions;
	/** Statistics subclass object */
	CHRMsStats chrmsStats;
	/** Statistics subclass object */
	OIStats oiStats;
	/** Statistics subclass object */
	DeathStats deathStats;
	/** Statistics subclass object */
	OverallSurvival overallSurvival;
	/** Statistics subclass object */
	OverallCosts overallCosts;
	/** Statistics subclass object */
	TBStats tbStats;
	/** Statistics subclass object */
	LTFUStats ltfuStats;
	/** Statistics subclass object */
	ProphStats prophStats;
	/** Statistics subclass object */
	ARTStats artStats;
	/** vector of PatientSummary objects for all cohorts in this context,
	//	uses actual object sinces subclass only contains 3 native type members, copy is cheap */
	vector<PatientSummary> patients;
	/** vector of TimeSummary object for each month/year time period,
	//	use pointer to object since subclass is complex and copy would be expensive */
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

/** \brief getPopulationSumary returns a const pointer to the PopulationSummary statistics object */
inline const RunStats::PopulationSummary *RunStats::getPopulationSummary() {
	return &popSummary;
}

/** \brief getHIVScreening returns a const pointer to the HIVScreening statistics object */
inline const RunStats::HIVScreening *RunStats::getHIVScreening() {
	return &hivScreening;
}


/** \brief getSurvivalStats returns a const pointer to the specified subgroup's SurvivalStats object */
inline const RunStats::SurvivalStats *RunStats::getSurvivalStats(int groupType) {
	return &(survivalStats[groupType]);
}

/** \brief getInitialDistributions returns a const pointer to the InitialDistributions object */
inline const RunStats::InitialDistributions *RunStats::getInitialDistributions() {
	return &initialDistributions;
}

/** \brief getOIStats returns a const pointer to the CHRMsStats object */
inline const RunStats::CHRMsStats *RunStats::getCHRMsStats() {
	return &chrmsStats;
}

/** \brief getOIStats returns a const pointer to the OIStats object */
inline const RunStats::OIStats *RunStats::getOIStats() {
	return &oiStats;
}

/** \brief getDeathStats returns a const pointer to the DeathStats object */
inline const RunStats::DeathStats *RunStats::getDeathStats() {
	return &deathStats;
}

/** \brief getOverallSurvival returns a const pointer to the OverallSurvival object */
inline const RunStats::OverallSurvival *RunStats::getOverallSurvival() {
	return &overallSurvival;
}

/** \brief getOverallCosts returns a const pointer to the OverallCosts object */
inline const RunStats::OverallCosts *RunStats::getOverallCosts() {
	return &overallCosts;
}

/** \brief getTBStats returns a const pointer to the TBStats object */
inline const RunStats::TBStats *RunStats::getTBStats() {
	return &tbStats;
}

/** \brief getLTFUStats returns a const pointer to the LTFUStats object */
inline const RunStats::LTFUStats *RunStats::getLTFUStats() {
	return &ltfuStats;
}

/** \brief getProphStats returns a const pointer to the ProphStats object */
inline const RunStats::ProphStats *RunStats::getProphStats() {
	return &prophStats;
}

/** \brief getARTStats returns a const pointer to the ArtStats object */
inline const RunStats::ARTStats *RunStats::getARTStats() {
	return &artStats;
}

/** \brief getTimeSummary returns a const pointer to the specified TimeSummary object,
	returns null if one does not exist for this time period */
inline const RunStats::TimeSummary *RunStats::getTimeSummary(unsigned int timePeriod) {
	if (timePeriod < timeSummaries.size())
		return timeSummaries[timePeriod];
	return NULL;
}
