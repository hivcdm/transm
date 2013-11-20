#include "include.h"

/* Constructor takes run name and associated simulation context pointer as parameters */
RunStats::RunStats(string runName, SimContext *simContext) {
	statsFileName = runName;
	statsFileName.append(CepacUtil::FILE_EXTENSION_FOR_OUTPUT);
	this->simContext = simContext;

	/* Clear the vectors for patient and time summaries */
	patients.clear();
	timeSummaries.clear();

	/* Initialize the statistics subclasses */
	initPopulationSummary();
	initHIVScreening();
	initSurvivalStats();
	initInitialDistributions();
	initCHRMsStats();
	initOIStats();
	initDeathStats();
	initOverallSurvival();
	initOverallCosts();
	initTBStats();
	initLTFUStats();
	initProphStats();
	initARTStats();
} /* end Constructor */

/* Destructor clears vectors and frees the allocated TimeSummary objects */
RunStats::~RunStats(void) {
	for (vector<TimeSummary *>::iterator s = timeSummaries.begin(); s != timeSummaries.end(); s++) {
		TimeSummary *summary = *s;
		delete summary;
	}
	timeSummaries.clear();
	patients.clear();
} /* end Destructor */

/* finalizeStats calculate all aggregate statistics and values to be outputted */
void RunStats::finalizeStats() {
	finalizePopulationSummary();
	finalizeHIVScreening();
	finalizeSurvivalStats();
	finalizeInitialDistributions();
	finalizeCHRMsStats();
	finalizeOIStats();
	finalizeDeathStats();
	finalizeOverallSurvival();
	finalizeOverallCosts();
	finalizeTBStats();
	finalizeLTFUStats();
	finalizeProphStats();
	finalizeARTStats();
	finalizeTimeSummaries();
}; /* end finalizeStats */

/* writeStatsFile outputs all statistics to the stats file */
void RunStats::writeStatsFile() {
	CepacUtil::changeDirectoryToResults();
	statsFile = CepacUtil::openFile(statsFileName.c_str(), "w");
	if (statsFile == NULL) {
		statsFileName.append("-tmp");
		statsFile = CepacUtil::openFile(statsFileName.c_str(), "w");
		if (statsFile == NULL) {
			string errorString = "   ERROR - Could not write stats or temporary stats file";
			throw errorString;
		}
	}

	writePopulationSummary();
	writeHIVScreening();
	writeSurvivalStats();
	writeInitialDistributions();
	writeCHRMsStats();
	writeOIStats();
	writeDeathStats();
	writeOverallSurvival();
	writeOverallCosts();
	writeTBStats();
	writeLTFUStats();
	writeProphStats();
	writeARTStats();
	writeTimeSummaries();

	CepacUtil::closeFile(statsFile);
} /* end writeStatsFile */

/* initPopulationSummary initializes the PopulationSummary object */
void RunStats::initPopulationSummary() {
	popSummary.numCohorts = 0;
	popSummary.numCohortsHIVPositive = 0;
	popSummary.costsSum = 0;
	popSummary.costsAverage = 0;
	popSummary.costsSumSquares = 0;
	popSummary.costsStdDev = 0;
	popSummary.costsLowerBound = DBL_MAX;
	popSummary.costsUpperBound = DBL_MIN;
	popSummary.LMsSum = 0;
	popSummary.LMsAverage = 0;
	popSummary.LMsSumSquares = 0;
	popSummary.LMsStdDev = 0;
	popSummary.LMsLowerBound = DBL_MAX;
	popSummary.LMsUpperBound = DBL_MIN;
	popSummary.QALMsSum = 0;
	popSummary.QALMsAverage = 0;
	popSummary.QALMsSumSquares = 0;
	popSummary.QALMsStdDev = 0;
	popSummary.QALMsLowerBound = DBL_MAX;
	popSummary.QALMsUpperBound = DBL_MIN;
	for (int i = 0; i <= SimContext::ART_NUM_LINES; i++) {
		popSummary.numFailART[i] = 0;
		popSummary.costsFailARTSum[i] = 0;
		popSummary.costsFailARTAverage[i] = 0;
		popSummary.LMsFailARTSum[i] = 0;
		popSummary.LMsFailARTAverage[i] = 0;
		popSummary.QALMsFailARTSum[i] = 0;
		popSummary.QALMsFailARTAverage[i] = 0;
	}
	popSummary.totalClinicVisits = 0;
	popSummary.costsHIVPositiveSum = 0;
	popSummary.costsHIVPositiveAverage = 0;
	popSummary.LMsHIVPositiveSum = 0;
	popSummary.LMsHIVPositiveAverage = 0;
	popSummary.QALMsHIVPositiveSum = 0;
	popSummary.QALMsHIVPositiveAverage = 0;
} /* end initPopulationSummary */

/* initHIVScreening initializes the HIVScreening object */
void RunStats::initHIVScreening() {
	hivScreening.numPrevalentCases = 0;
	hivScreening.numIncidentCases = 0;
	hivScreening.numHIVNegative = 0;
	hivScreening.numHIVPositiveTotal = 0;
	for (int i = 0; i < SimContext::HIV_EXT_INF_NUM; i++) {
		hivScreening.numPatientsInitialHIVState[i] = 0;
		hivScreening.numTestsHIVState[i] = 0;
	}
	hivScreening.numAtDetectionPrevalent = 0;
	hivScreening.numAtDetectionIncident = 0;
	for (int j = 0; j < SimContext::HIV_INF_NUM; j++) {
		hivScreening.numAtDetectionPrevalentHIV[j] = 0;
		hivScreening.numAtDetectionIncidentHIV[j] = 0;
		for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
			hivScreening.numAtDetectionPrevalentCD4[i] = 0;
			hivScreening.numAtDetectionPrevalentCD4HIV[i][j] = 0;
			hivScreening.numAtDetectionIncidentCD4[i] = 0;
			hivScreening.percentAtDetectionIncidentCD4[i] = 0;
			hivScreening.numAtDetectionIncidentCD4HIV[i][j] = 0;
		}
		for (int i = 0; i < SimContext::HVL_NUM_STRATA; i++) {
			hivScreening.numAtDetectionPrevalentHVL[i] = 0;
			hivScreening.numAtDetectionPrevalentHVLHIV[i][j] = 0;
			hivScreening.numAtDetectionIncidentHVL[i] = 0;
			hivScreening.percentAtDetectionIncidentHVL[i] = 0;
			hivScreening.numAtDetectionIncidentHVLHIV[i][j] = 0;
		}
	}
	hivScreening.CD4AtDetectionPrevalentSum = 0;
	hivScreening.CD4AtDetectionPrevalentAverage = 0;
	hivScreening.CD4AtDetectionIncidentSum = 0;
	hivScreening.CD4AtDetectionIncidentAverage = 0;
	for (int i = 0; i < SimContext::HIV_INF_NUM; i++) {
		hivScreening.CD4AtDetectionPrevalentSumHIV[i] = 0;
		hivScreening.CD4AtDetectionPrevalentAverageHIV[i] = 0;
		hivScreening.CD4AtDetectionIncidentSumHIV[i] = 0;
		hivScreening.CD4AtDetectionIncidentAverageHIV[i] = 0;
	}
	hivScreening.monthsToInfectionSum = 0;
	hivScreening.monthsToInfectionAverage = 0;
	hivScreening.monthsToInfectionSumSquares = 0;
	hivScreening.monthsToInfectionStdDev = 0;
	hivScreening.monthsAfterInfectionToDetectionSum = 0;
	hivScreening.monthsAfterInfectionToDetectionAverage = 0;
	hivScreening.monthsAfterInfectionToDetectionSumSquares = 0;
	hivScreening.monthsAfterInfectionToDetectionStdDev = 0;
	hivScreening.monthsToDetectionPrevalentSum = 0;
	hivScreening.monthsToDetectionPrevalentAverage = 0;
	hivScreening.monthsToDetectionPrevalentSumSquares = 0;
	hivScreening.monthsToDetectionPrevalentStdDev = 0;
	hivScreening.monthsToDetectionIncidentSum = 0;
	hivScreening.monthsToDetectionIncidentAverage = 0;
	hivScreening.monthsToDetectionIncidentSumSquares = 0;
	hivScreening.monthsToDetectionIncidentStdDev = 0;
	hivScreening.ageMonthsAtDetectionPrevalentSum = 0;
	hivScreening.ageMonthsAtDetectionPrevalentAverage = 0;
	hivScreening.ageMonthsAtDetectionPrevalentSumSquares = 0;
	hivScreening.ageMonthsAtDetectionPrevalentStdDev = 0;
	hivScreening.ageMonthsAtDetectionIncidentSum = 0;
	hivScreening.ageMonthsAtDetectionIncidentAverage = 0;
	hivScreening.ageMonthsAtDetectionIncidentSumSquares = 0;
	hivScreening.ageMonthsAtDetectionIncidentStdDev = 0;
	hivScreening.numDetectedGender[SimContext::GENDER_MALE] = 0;
	hivScreening.numDetectedGender[SimContext::GENDER_FEMALE] = 0;
	for (int i = 0; i < SimContext::HIV_DET_NUM; i++) {
		hivScreening.numDetectedPrevalentMeans[i] = 0;
		hivScreening.numDetectedIncidentMeans[i] = 0;
	}
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		hivScreening.numDetectedByOIs[i] = 0;
	}
	for (int i = 0; i < SimContext::TEST_ACCEPT_NUM; i++) {
		for (int j = 0; j < SimContext::HIV_EXT_INF_NUM; j++) {
			hivScreening.numTestingAcceptRate[i][j] = 0;
		}
	}
	for (int i = 0; i < SimContext::HIV_TEST_FREQ_NUM; i++) {
		hivScreening.numTestingInterval[i] = 0;
	}
	hivScreening.numAcceptTest = 0;
	hivScreening.numRefuseTest = 0;
	hivScreening.numReturnForResults = 0;
	hivScreening.numNoReturnForResults = 0;
	hivScreening.numTestResultsPrevalent = 0;
	hivScreening.numTestResultsIncident = 0;
	hivScreening.numTestResultsHIVNegative = 0;
	for (int i = 0; i < SimContext::TEST_RESULT_NUM; i++) {
		hivScreening.numTestResultsPrevalentType[i] = 0;
		hivScreening.numTestResultsIncidentType[i] = 0;
		hivScreening.numTestResultsHIVNegativeType[i] = 0;
	}
} /* end initHIVScreening */

/* initSurvivalStats initializes the SurvivalStats objects for each of the subgroups */
void RunStats::initSurvivalStats() {
	for (int i = 0; i < NUM_SURVIVAL_GROUPS; i++) {
		survivalStats[i].LMsHistogram.clear();
		survivalStats[i].LMsMin = DBL_MAX;
		survivalStats[i].LMsMax = DBL_MIN;
		survivalStats[i].LMsMedian = 0;
		survivalStats[i].LMsSumDeviationMedian = 0;
		survivalStats[i].LMsAverageDeviationMedian = 0;
		survivalStats[i].LMsSum = 0;
		survivalStats[i].LMsMean = 0;
		survivalStats[i].LMsSumDeviation = 0;
		survivalStats[i].LMsAverageDeviation = 0;
		survivalStats[i].LMsSumDeviationSquares = 0;
		survivalStats[i].LMsStdDev = 0;
		survivalStats[i].LMsVariance = 0;
		survivalStats[i].LMsSumDeviationCubes = 0;
		survivalStats[i].LMsSkew = 0;
		survivalStats[i].LMsSumDeviationQuads = 0;
		survivalStats[i].LMsKurtosis = 0;
		survivalStats[i].costsSum = 0;
		survivalStats[i].costsMean = 0;
		survivalStats[i].costsSumSquares = 0;
		survivalStats[i].costsStdDev = 0;
		survivalStats[i].QALMsSum = 0;
		survivalStats[i].QALMsMean = 0;
		survivalStats[i].QALMsSumSquares = 0;
		survivalStats[i].QALMsStdDev = 0;
	}
} /* end initSurvivalStats */

/* initInitialDistributions initializes the InititalDistribution object */
void RunStats::initInitialDistributions() {
	for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
		initialDistributions.numPatientsCD4Level[i] = 0;
	}
	for (int i = 0; i < SimContext::HVL_NUM_STRATA; i++) {
		initialDistributions.numPatientsHVLLevel[i] = 0;
		initialDistributions.numPatientsHVLSetpointLevel[i] = 0;
	}
	initialDistributions.sumInitialAgeMonths = 0;
	initialDistributions.averageInitialAgeMonths = 0;
	initialDistributions.numMalePatients = 0;
	initialDistributions.numFemalePatients = 0;
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		initialDistributions.numPriorOIHistories[i] = 0;
	}
	for (int i = 0; i < SimContext::CD4_RESPONSE_NUM_TYPES; i++) {
		initialDistributions.numARTResposneTypes[i] = 0;
	}
	for (int i = 0; i < SimContext::RISK_FACT_NUM; i++) {
		initialDistributions.numRiskFactors[i] = 0;
	}
	for (int i = 0; i < SimContext::PEDS_HIV_NUM; i++) {
		for (int j = 0; j < SimContext::PEDS_MOM_HIV_NUM; j++) {
			initialDistributions.numInitialPediatrics[i][j] = 0;
		}
	}
} /* end initInitialDistributions */

/* initCHRMsStats initializes the CHRMsStats object */
void RunStats::initCHRMsStats() {
	for (int i = 0; i < SimContext::CHRM_NUM; i++) {
		chrmsStats.numPrevalentCHRM[i] = 0;
		chrmsStats.numIncidentCHRM[i] = 0;
		chrmsStats.numDeathsCHRM[i] = 0;
	}
	for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
		chrmsStats.numPrevalentCD4[i] = 0;
		chrmsStats.numIncidentCD4[i] = 0;
		chrmsStats.numDeathsCD4[i] = 0;
	}
	for (int i = 0; i < SimContext::CHRM_NUM; i++) {
		for (int j = 0; j < SimContext::CD4_NUM_STRATA; j++) {
			chrmsStats.numPrevalentCHRMCD4[i][j] = 0;
			chrmsStats.numIncidentCHRMCD4[i][j] = 0;
			chrmsStats.numDeathsCHRMCD4[i][j] = 0;
		}
	}
} /* end initCHRMsStats */

/* initOIStats initializes the OIStats object */
void RunStats::initOIStats() {
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		oiStats.numPrimaryOIsOI[i] = 0;
		oiStats.numSecondaryOIsOI[i] = 0;
		oiStats.numDetectedOIsOI[i] = 0;
	}
	for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
		oiStats.numPrimaryOIsCD4[i] = 0;
		oiStats.numSecondaryOIsCD4[i] = 0;
		oiStats.numDetectedOIsCD4[i] = 0;
	}
	for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
		for (int j = 0; j < SimContext::OI_NUM; j++) {
			oiStats.numPrimaryOIsCD4OI[i][j] = 0;
			oiStats.numSecondaryOIsCD4OI[i][j] = 0;
			oiStats.numDetectedOIsCD4OI[i][j] = 0;
		}
	}
	for (int i = 0; i < SimContext::HVL_NUM_STRATA; i++) {
		oiStats.numPatientsHVL[i] = 0;
		oiStats.numMonthsHVL[i] = 0;
	}
	for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
		oiStats.numPatientsCD4[i] = 0;
		oiStats.numMonthsCD4[i] = 0;
	}
	for (int i = 0; i < SimContext::HVL_NUM_STRATA; i++) {
		for (int j = 0; j < SimContext::CD4_NUM_STRATA; j++) {
			oiStats.numPatientsHVLCD4[i][j] = 0;
			oiStats.numMonthsHVLCD4[i][j] = 0;
		}
	}
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		for (int j = 0; j < SimContext::HVL_NUM_STRATA; j++) {
			oiStats.numPatientsOIHistoryHVL[i][j] = 0;
			oiStats.probPatientsOIHistoryHVL[i][j] = 0;
			oiStats.numMonthsOIHistoryHVL[i][j] = 0;
			oiStats.probMonthsOIHistoryHVL[i][j] = 0;
		}
	}
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		for (int j = 0; j < SimContext::CD4_NUM_STRATA; j++) {
			oiStats.numPatientsOIHistoryCD4[i][j] = 0;
			oiStats.probPatientsOIHistoryCD4[i][j] = 0;
			oiStats.numMonthsOIHistoryCD4[i][j] = 0;
			oiStats.probMonthsOIHistoryCD4[i][j] = 0;
		}
	}
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		for (int j = 0; j < SimContext::HVL_NUM_STRATA; j++) {
			for (int k = 0; k < SimContext::CD4_NUM_STRATA; k++) {
				oiStats.numPatientsOIHistoryHVLCD4[i][j][k] = 0;
				oiStats.probPatientsOIHistoryHVLCD4[i][j][k] = 0;
				oiStats.numMonthsOIHistoryHVLCD4[i][j][k] = 0;
				oiStats.probMonthsOIHistoryHVLCD4[i][j][k] = 0;
			}
		}
	}
} /* end initOIStats */

/* initDeathStats initializes the DeathStats object */
void RunStats::initDeathStats() {
	deathStats.numDeathsUninfected = 0;
	for (int i = 0; i < SimContext::DTH_NUM_CAUSES; i++) {
		deathStats.numDeathsType[i] = 0;
	}
	for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
		deathStats.numDeathsCD4[i] = 0;
	}
	for (int i = 0; i < SimContext::HVL_NUM_STRATA; i++) {
		deathStats.numDeathsHVL[i] = 0;
	}
	for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
		for (int j = 0; j < SimContext::DTH_NUM_CAUSES; j++) {
			deathStats.numDeathsCD4Type[i][j] = 0;
		}
	}
	for (int i = 0; i < SimContext::HVL_NUM_STRATA; i++) {
		for (int j = 0; j < SimContext::CD4_NUM_STRATA; j++) {
			deathStats.numDeathsHVLCD4[i][j] = 0;
		}
	}
	deathStats.numChronicAIDSDeathsNoOIHistory = 0;
	deathStats.numChronicAIDSDeathsOIHistory = 0;
	deathStats.numNonAIDSDeathsNoOIHistory = 0;
	deathStats.numNonAIDSDeathsOIHistory = 0;
	for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
		deathStats.numChronicAIDSDeathsNoOIHistoryCD4[i] = 0;
		deathStats.numChronicAIDSDeathsOIHistoryCD4[i] = 0;
		deathStats.numNonAIDSDeathsNoOIHistoryCD4[i] = 0;
		deathStats.numNonAIDSDeathsOIHistoryCD4[i] = 0;
	}
} /* end initDeathStats */

/* initOverallSurvival initializes the OverallSurvival object */
void RunStats::initOverallSurvival() {
	overallSurvival.LMsNoOIHistory = 0;
	overallSurvival.LMsOIHistory = 0;
	overallSurvival.LMsTotal = 0;
	for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
		overallSurvival.LMsNoOIHistoryCD4[i] = 0;
		overallSurvival.LMsOIHistoryCD4[i] = 0;
		overallSurvival.LMsTotalCD4[i] = 0;
	}
	for (int i = 0; i < SimContext::HVL_NUM_STRATA; i++) {
		overallSurvival.LMsHVL[i] = 0;
		overallSurvival.LMsHVLSetpoint[i] = 0;
	}
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		overallSurvival.LMsNoOIHistoryOIs[i] = 0;
		overallSurvival.LMsOIHistoryOIs[i] = 0;
	}
	overallSurvival.LMsHIVPositive = 0;
	overallSurvival.QALMsHIVPositive = 0;
	for (int i = 0; i < SimContext::HIV_ID_NUM; i++) {
		overallSurvival.LMsHIVState[i] = 0;
		overallSurvival.QALMsHIVState[i] = 0;
	}
	overallSurvival.LMsInScreening = 0;
	overallSurvival.QALMsInScreening = 0;
	overallSurvival.LMsInRegularCEPAC = 0;
	for (int i = 0; i < SimContext::GENDER_NUM; i++) {
		overallSurvival.LMsGender[i] = 0;
		overallSurvival.QALMsGender[i] = 0;
	}
} /* end initOverallSurvival */

/* initOverallCosts initializes the OverallCosts object */
void RunStats::initOverallCosts() {
	overallCosts.costsNoOIHistory = 0;
	overallCosts.costsOIHistory = 0;
	overallCosts.costsTotal = 0;
	for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
		overallCosts.costsNoOIHistoryCD4[i] = 0;
		overallCosts.costsOIHistoryCD4[i] = 0;
		overallCosts.costsTotalCD4[i] = 0;
	}
	for (int i = 0; i < SimContext::HVL_NUM_STRATA; i++) {
		overallCosts.costsHVL[i] = 0;
		overallCosts.costsHVLSetpoint[i] = 0;
	}
	overallCosts.directCostsProph = 0;
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		overallCosts.directCostsProphOIs[i] = 0;
		for (int j = 0; j < SimContext::PROPH_NUM; j++) {
			overallCosts.directCostsProphOIsProph[i][j] = 0;
		}
	}
	overallCosts.directCostsART = 0;
	for (int i = 0; i < SimContext::ART_NUM_LINES; i++) {
		overallCosts.directCostsARTLine[i] = 0;
	}
	overallCosts.costsHIVPositive = 0;
	for (int i = 0; i < SimContext::HIV_ID_NUM; i++) {
		overallCosts.costsHIVState[i] = 0;
	}
	overallCosts.costsCD4Testing = 0;
	overallCosts.costsHVLTesting = 0;
	overallCosts.costsClinicVisits = 0;
	overallCosts.costsHIVScreeningTests = 0;
	overallCosts.costsHIVScreeningMisc = 0;
	for (int i = 0; i < SimContext::COST_NUM_TYPES; i++) {
		overallCosts.totalUndiscountedCosts[i] = 0;
	}
	overallCosts.totalUndiscountedCostsUnclassified = 0;
	overallCosts.costsDrugs = 0;
	overallCosts.costsToxicity = 0;
	for (int i = 0; i < SimContext::GENDER_NUM; i++) {
		overallCosts.costsGender[i] = 0;
	}
} /* end initOverallCost */

/* initTBStats initializes the TBStats object */
void RunStats::initTBStats() {
	for (int i = 0; i < SimContext::TB_NUM_STRAINS; i++) {
		tbStats.numLatentInfections[i] = 0;
		tbStats.numActiveInfections[i] = 0;
		tbStats.numReactivationsLatent[i] = 0;
		tbStats.numReinfectionsLatent[i] = 0;
		tbStats.numRelapsesHistoryActive[i] = 0;
		tbStats.numReinfectionsHistoryActive[i] = 0;
		tbStats.numSpontaneousResolutions[i] = 0;
		tbStats.numDeaths[i] = 0;
	}
	for (int i = 0; i < SimContext::TB_TREATM_STAGE_NUM; i++) {
		tbStats.numTreatmentMinorToxicity[i] = 0;
		tbStats.numTreatmentMajorToxicity[i] = 0;
	}
	for (int i = 0; i < SimContext::PROPH_NUM; i++) {
		tbStats.numProphMinorToxicity[i] = 0;
		tbStats.numProphMajorToxicity[i] = 0;
	}
	for (int i = 0; i < SimContext::TB_NUM_STRAINS; i++) {
		for (int j = 0; j < SimContext::TB_NUM_STATES; j++) {
			tbStats.numInStateAtEntry[i][j] = 0;
		}
	}
	for (int i = 0; i < SimContext::TB_NUM_STRAINS; i++) {
		for (int j = 0; j < SimContext::TB_TREATM_STAGE_NUM; j++) {
			tbStats.numStartOnTreatment[i][j] = 0;
			tbStats.numDropoutTreatment[i][j] = 0;
			tbStats.numCuredAtTreatmentDropout[i][j] = 0;
			tbStats.numFinishTreatment[i][j] = 0;
			tbStats.numCuredAtTreatmentFinish[i][j] = 0;
			tbStats.numIncreaseResistanceAtTreatmentFinish[i][j] = 0;
		}
	}
} /* end initTBStats */

/* initLTFUStats initializes the LTFUStats object */
void RunStats::initLTFUStats() {
	ltfuStats.numPatientsLost = 0;
	ltfuStats.numPatientsReturned = 0;
	ltfuStats.numDeathsWhileLost = 0;
	ltfuStats.numLostToFollowUp = 0;
	ltfuStats.numReturnToCare = 0;
	for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
		ltfuStats.numLostToFollowUpCD4[i] = 0;
		ltfuStats.numReturnToCareCD4[i] = 0;
		ltfuStats.numDeathsWhileLostCD4[i] = 0;
	}
	ltfuStats.monthsLostBeforeReturnSum = 0;
	ltfuStats.monthsLostBeforeReturnMean = 0;
	ltfuStats.monthsLostBeforeReturnSumSquares = 0;
	ltfuStats.monthsLostBeforeReturnStdDev = 0;
	ltfuStats.numLostToFollowUpPreART = 0;
	ltfuStats.numLostToFollowUpPostART = 0;
	ltfuStats.numReturnToCarePreART = 0;
	ltfuStats.numReturnToCarePostART = 0;
	ltfuStats.numDeathsWhileLostPreART = 0;
	ltfuStats.numDeathsWhileLostPostART = 0;
	for (int i = 0; i < SimContext::ART_NUM_LINES; i++) {
		ltfuStats.numLostToFollowUpART[i] = 0;
		ltfuStats.numReturnOnPrevART[i] = 0;
		ltfuStats.numReturnOnNextART[i] = 0;
		ltfuStats.numDeathsWhileLostART[i] = 0;
	}
} /* end initLTFUStats */

/* initProphStats initializes the ProphStats object */
void RunStats::initProphStats() {
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		prophStats.numMinorToxicityTotal[i] = 0;
		prophStats.numMajorToxicityTotal[i] = 0;
		for (int j = 0; j < SimContext::PROPH_NUM; j++) {
			prophStats.numMinorToxicity[i][j] = 0;
			prophStats.numMajorToxicity[i][j] = 0;
			for (int k = 0; k < SimContext::PROPH_NUM_TYPES; k++) {
				prophStats.trueCD4InitProphSum[k][i][j] = 0;
				prophStats.trueCD4InitProphMean[k][i][j] = 0;
				prophStats.observedCD4InitProphSum[k][i][j] = 0;
				prophStats.observedCD4InitProphMean[k][i][j] = 0;
				prophStats.numTimesInitProph[k][i][j] = 0;
			}
		}
	}
} /* end initProphStats */

/* initARTStats initializes the ARTStats object */
void RunStats::initARTStats() {
	artStats.monthsSuppressed = 0;
	for (int i = 0; i < SimContext::HVL_NUM_STRATA; i++) {
		artStats.monthsPartiallySuppressedHVL[i] = 0;
		artStats.monthsFailedHVL[i] = 0;
	}
	for (int i = 0; i < SimContext::ART_NUM_LINES; i++) {
		artStats.monthsSuppressedLine[i] = 0;
		artStats.monthsPartiallySuppressedLine[i] = 0;
		artStats.monthsFailedLine[i] = 0;
		artStats.numOnARTAtInit[i] = 0;
		artStats.trueCD4AtInitSum[i] = 0;
		artStats.trueCD4AtInitMean[i] = 0;
		artStats.observedCD4AtInitSum[i] = 0;
		artStats.observedCD4AtInitMean[i] = 0;
		for (int j = 0; j < SimContext::ART_EFF_NUM_TYPES; j++) {
			artStats.numDrawEfficacyAtInit[i][j] = 0;
		}
		for (int j = 0; j < SimContext::CD4_RESPONSE_NUM_TYPES; j++) {
			artStats.numCD4ResponseTypeAtInit[i][j] = 0;
		}
		for (int j = 0; j < SimContext::RISK_FACT_NUM; j++) {
			artStats.numWithRiskFactorAtInit[i][j] = 0;
		}
		artStats.numTrueFailure[i] = 0;
		artStats.trueCD4AtTrueFailureSum[i] = 0;
		artStats.trueCD4AtTrueFailureMean[i] = 0;
		artStats.observedCD4AtTrueFailureSum[i] = 0;
		artStats.observedCD4AtTrueFailureMean[i] = 0;
		artStats.monthsToTrueFailureSum[i] = 0;
		artStats.monthsToTrueFailureMean[i] = 0;
		artStats.monthsToTrueFailureSumSquares[i] = 0;
		artStats.monthsToTrueFailureStdDev[i] = 0;
		for (int j = 0; j < SimContext::RESP_NUM_TYPES; j++) {
			artStats.numOnARTAtInitResp[i][j] = 0;
			artStats.trueCD4AtInitSum[i] = 0;
			artStats.trueCD4AtInitMean[i] = 0;
			artStats.observedCD4AtInitSum[i] = 0;
			artStats.observedCD4AtInitMean[i] = 0;
			for (int k = 0; k < SimContext::ART_EFF_NUM_TYPES; k++) {
				artStats.numDrawEfficacyAtInitResp[i][k][j] = 0;
			}
			for (int k = 0; k < SimContext::CD4_RESPONSE_NUM_TYPES; k++) {
				artStats.numCD4ResponseTypeAtInitResp[i][k][j] = 0;
			}
			for (int k = 0; k < SimContext::RISK_FACT_NUM; k++) {
				artStats.numWithRiskFactorAtInitResp[i][k][j] = 0;
			}
			artStats.numTrueFailureResp[i][j] = 0;
			artStats.trueCD4AtTrueFailureSumResp[i][j] = 0;
			artStats.trueCD4AtTrueFailureMeanResp[i][j] = 0;
			artStats.observedCD4AtTrueFailureSumResp[i][j] = 0;
			artStats.observedCD4AtTrueFailureMeanResp[i][j] = 0;
			artStats.monthsToTrueFailureSumResp[i][j] = 0;
			artStats.monthsToTrueFailureMeanResp[i][j] = 0;
			artStats.monthsToTrueFailureSumSquaresResp[i][j] = 0;
			artStats.monthsToTrueFailureStdDevResp[i][j] = 0;
		}
		artStats.numSTIInterruptionsSum[i] = 0;
		artStats.numSTIInterruptionsMean[i] = 0;
		artStats.monthsOnSTIInterruptionSum[i] = 0;
		artStats.monthsOnSTIInterruptionMean[i] = 0;
		for (int j = 0; j < SimContext::HVL_NUM_STRATA; j++) {
			artStats.monthsPartiallySuppressedLineHVL[i][j] = 0;
			artStats.monthsFailedLineHVL[i][j] = 0;
		}
		artStats.numObservedFailure[i] = 0;
		artStats.numObservedFailureAfterTrue[i] = 0;
		artStats.numNeverObservedFailure[i] = 0;
		artStats.trueCD4AtObservedFailureSum[i] = 0;
		artStats.trueCD4AtObservedFailureMean[i] = 0;
		artStats.observedCD4AtObservedFailureSum[i] = 0;
		artStats.observedCD4AtObservedFailureMean[i] = 0;
		artStats.monthsToObservedFailureSum[i] = 0;
		artStats.monthsToObservedFailureMean[i] = 0;
		artStats.monthsToObservedFailureSumSquares[i] = 0;
		artStats.monthsToObservedFailureStdDev[i] = 0;
		for (int j = 0; j < SimContext::ART_NUM_FAIL_TYPES; j++) {
			artStats.numObservedFailureType[i][j] = 0;
			artStats.numObservedFailureAfterTrueType[i][j] = 0;
			artStats.trueCD4AtObservedFailureSumType[i][j] = 0;
			artStats.trueCD4AtObservedFailureMeanType[i][j] = 0;
			artStats.observedCD4AtObservedFailureSumType[i][j] = 0;
			artStats.observedCD4AtObservedFailureMeanType[i][j] = 0;
			artStats.monthsToObservedFailureSumType[i][j] = 0;
			artStats.monthsToObservedFailureMeanType[i][j] = 0;
			artStats.monthsToObservedFailureSumSquaresType[i][j] = 0;
			artStats.monthsToObservedFailureStdDevType[i][j] = 0;
		}
		artStats.numStop[i] = 0;
		artStats.numStopAfterTrueFailure[i] = 0;
		artStats.numNeverStop[i] = 0;
		artStats.trueCD4AtStopSum[i] = 0;
		artStats.trueCD4AtStopMean[i] = 0;
		artStats.observedCD4AtStopSum[i] = 0;
		artStats.observedCD4AtStopMean[i] = 0;
		artStats.monthsToStopSum[i] = 0;
		artStats.monthsToStopMean[i] = 0;
		artStats.monthsToStopSumSquares[i] = 0;
		artStats.monthsToStopStdDev[i] = 0;
		for (int j = 0; j < SimContext::ART_NUM_STOP_TYPES; j++) {
			artStats.numStopType[i][j] = 0;
			artStats.numStopAfterTrueFailureType[i][j] = 0;
			artStats.trueCD4AtStopSumType[i][j] = 0;
			artStats.trueCD4AtStopMeanType[i][j] = 0;
			artStats.observedCD4AtStopSumType[i][j] = 0;
			artStats.observedCD4AtStopMeanType[i][j] = 0;
			artStats.monthsToStopSumType[i][j] = 0;
			artStats.monthsToStopMeanType[i][j] = 0;
			artStats.monthsToStopSumSquaresType[i][j] = 0;
			artStats.monthsToStopStdDevType[i][j] = 0;
		}
		for (int j = 0; j < SimContext::ART_NUM_MTHS_RECORD; j++) {
			artStats.numOnARTAtMonth[i][j] = 0;
			artStats.numSuppressedAtMonth[i][j] = 0;
			artStats.HVLDropsAtMonthSum[i][j] = 0;
			artStats.HVLDropsAtMonthMean[i][j] = 0;
			artStats.HVLDropsAtMonthSumSquares[i][j] = 0;
			artStats.HVLDropsAtMonthStdDev[i][j] = 0;
		}
		for (int j = 0; j < SimContext::CD4_NUM_STRATA; j++) {
			for (int k = 0; k < SimContext::HVL_NUM_STRATA; k++) {
				artStats.distributionAtInit[i][j][k] = 0;
			}
		}
		for (int j = 0; j < SimContext::ART_NUM_TOX_SEVERITY; j++) {
			for (int k = 0; k < SimContext::HVL_NUM_STRATA; k++) {
				artStats.numToxicityCases[i][j][k] = 0;
			}
		}
		for (int j = 0; j < SimContext::HVL_NUM_STRATA; j++) {
			artStats.numToxicityDeaths[i][j] = 0;
		}
		for (int j = 0; j < SimContext::STI_NUM_TRACKED; j++) {
			artStats.numSTIInterruptions[i][j] = 0;
			artStats.numSTIRestarts[i][j] = 0;
			artStats.numSTIEndpoints[i][j] = 0;
			artStats.numPatientsWithSTIInterruptions[i][j] = 0;
		}
	}
} /* end initARTStats */

/* initTimeSummary initializes the given TimeSummary object */
void RunStats::initTimeSummary(TimeSummary *currTime) {
	// Initialize all the time summary values
	currTime->timePeriod = 0;
	currTime->numAlive = 0;
	for (int i = 0; i < SimContext::HIV_ID_NUM; i++) {
		currTime->numAliveType[i] = 0;
	}
	for (int i = 0; i < SimContext::PEDS_HIV_NUM; i++) {
		currTime->numAlivePediatrics[i] = 0;
	}
	currTime->numIncidentHIVInfections = 0;
	currTime->sumQOLmultipliers = 0;
	currTime->trueCD4Sum = 0;
	currTime->trueCD4Mean = 0;
	currTime->trueCD4SumSquares = 0;
	currTime->trueCD4StdDev = 0;
	currTime->observedCD4Sum = 0;
	currTime->observedCD4Mean = 0;
	currTime->observedCD4SumSquares = 0;
	currTime->observedCD4StdDev = 0;
	currTime->trueCD4PercentageSum = 0;
	currTime->trueCD4PercentageMean = 0;
	currTime->trueCD4PercentageSumSquares = 0;
	currTime->trueCD4PercentageStdDev = 0;
	currTime->trueHVLSum = 0;
	currTime->trueHVLMean = 0;
	currTime->trueHVLSumSquares = 0;
	currTime->trueHVLStdDev = 0;
	currTime->observedHVLSum = 0;
	currTime->observedHVLMean = 0;
	currTime->observedHVLSumSquares = 0;
	currTime->observedHVLStdDev = 0;
	for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
		for (int j = 0; j < SimContext::ART_NUM_STATES; j++) {
			currTime->trueCD4ARTDistribution[j][i] = 0;
		}
		currTime->observedCD4Distribution[i] = 0;
	}
	for (int i = 0; i < SimContext::HVL_NUM_STRATA; i++) {
		currTime->trueHVLDistribution[i] = 0;
		currTime->observedHVLDistribution[i] = 0;
	}
	for (int i = 0; i < SimContext::ART_NUM_STATES; i++) {
		for (int j = 0; j < SimContext::HVL_NUM_STRATA; j++) {
			for (int k = 0; k < SimContext::CD4_NUM_STRATA; k++) {
				currTime->trueCD4HVLARTDistribution[i][j][k] = 0;
			}
		}
	}
	for (int i = 0; i < SimContext::ART_EFF_NUM_TYPES; i++) {
		currTime->numARTEfficacyState[i] = 0;
	}
	currTime->numWithoutOIHistory = 0;
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		currTime->numPrimaryOIs[i] = 0;
		currTime->numSecondaryOIs[i] = 0;
		currTime->numWithOIHistory[i] = 0;
		currTime->numWithFirstOI[i] = 0;
		currTime->numDeathsFromFirstOI[i] = 0;
	}
	for (int i = 0; i < SimContext::DTH_NUM_CAUSES; i++) {
		currTime->numDeathsType[i] = 0;
	}
	currTime->costsCD4Testing = 0;
	currTime->costsHVLTesting = 0;
	currTime->costsClinicVisits = 0;
	currTime->costsHIVTests = 0;
	currTime->costsHIVMisc = 0;
	currTime->totalMonthlyCohortCosts = 0;
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		for (int j = 0; j < SimContext::PROPH_NUM; j++) {
			currTime->costsProph[i][j] = 0;
		}
	}
	for (int i = 0; i < SimContext::ART_NUM_LINES; i++) {
		currTime->costsART[i] = 0;
		currTime->numOnART[i] = 0;
		currTime->numOnARTIncludingContinuedCosts[i] = 0;
		currTime->numLostToFollowUpART[i] = 0;
		currTime->numReturnOnPrevART[i] = 0;
		currTime->numReturnOnNextART[i] = 0;
		currTime->numDeathsWhileLostART[i] = 0;
	}
	currTime->numLostToFollowUpPreART = 0;
	currTime->numLostToFollowUpPostART = 0;
	currTime->numReturnToCarePreART = 0;
	currTime->numReturnToCarePostART = 0;
	currTime->numDeathsWhileLostPreART = 0;
	currTime->numDeathsWhileLostPostART = 0;
} /* end initTimeSummary */

/* finalizePopulationSummary calculates aggregate statistics for the PopulationSummary object */
void RunStats::finalizePopulationSummary() {
	char tmpbuf[256];

	// Copy the run/set name and get the time/date of completion
	const SimContext::RunSpecsInputs *runSpecs = simContext->getRunSpecsInputs();
	popSummary.runSetName = runSpecs->runSetName;
	popSummary.runName = runSpecs->runName;
	CepacUtil::getDateString(tmpbuf, 255);
	popSummary.runDate = tmpbuf;
	CepacUtil::getTimeString(tmpbuf, 255);
	popSummary.runTime = tmpbuf;

	// Calculate the total costs and life month statistics from all patients
	popSummary.costsAverage = popSummary.costsSum / popSummary.numCohorts;
	popSummary.costsStdDev = sqrt(popSummary.costsSumSquares / popSummary.numCohorts - popSummary.costsAverage * popSummary.costsAverage);
	popSummary.costsLowerBound = popSummary.costsAverage - (1.96 * popSummary.costsStdDev / sqrt((double)popSummary.numCohorts));
	popSummary.costsUpperBound = popSummary.costsAverage + (1.96 * popSummary.costsStdDev / sqrt((double)popSummary.numCohorts));
	popSummary.LMsAverage = popSummary.LMsSum / popSummary.numCohorts;
	popSummary.LMsStdDev = sqrt(popSummary.LMsSumSquares / popSummary.numCohorts - popSummary.LMsAverage * popSummary.LMsAverage);
	popSummary.LMsLowerBound = popSummary.LMsAverage - (1.96 * popSummary.LMsStdDev / sqrt((double)popSummary.numCohorts));
	popSummary.LMsUpperBound = popSummary.LMsAverage + (1.96 * popSummary.LMsStdDev / sqrt((double)popSummary.numCohorts));
	popSummary.QALMsAverage = popSummary.QALMsSum / popSummary.numCohorts;
	popSummary.QALMsStdDev = sqrt(popSummary.QALMsSumSquares / popSummary.numCohorts - popSummary.QALMsAverage * popSummary.QALMsAverage);
	popSummary.QALMsLowerBound = popSummary.QALMsAverage - (1.96 * popSummary.QALMsStdDev / sqrt((double)popSummary.numCohorts));
	popSummary.QALMsUpperBound = popSummary.QALMsAverage + (1.96 * popSummary.QALMsStdDev / sqrt((double)popSummary.numCohorts));

	// Calculate the total costs and life month statistics from patients with X number of ART failures
	for (int i = 0; i <= SimContext::ART_NUM_LINES; i++) {
		if (popSummary.numFailART[i] > 0) {
			popSummary.costsFailARTAverage[i] = popSummary.costsFailARTSum[i] / popSummary.numFailART[i];
			popSummary.LMsFailARTAverage[i] = popSummary.LMsFailARTSum[i] / popSummary.numFailART[i];
			popSummary.QALMsFailARTAverage[i] = popSummary.QALMsFailARTSum[i] / popSummary.numFailART[i];
		}
	}

	// Calculate the total costs and life month statistics from the HIV positive patients
	if (popSummary.numCohortsHIVPositive > 0) {
		popSummary.costsHIVPositiveAverage = popSummary.costsHIVPositiveSum / popSummary.numCohortsHIVPositive;
		popSummary.LMsHIVPositiveAverage = popSummary.LMsHIVPositiveSum / popSummary.numCohortsHIVPositive;
		popSummary.QALMsHIVPositiveAverage = popSummary.QALMsHIVPositiveSum / popSummary.numCohortsHIVPositive;
	}
} /* end finalizePopulationSummary */

/* finalizeHIVScreening calculates aggregate statistics for the HIVScreening object */
void RunStats::finalizeHIVScreening() {
	hivScreening.numHIVPositiveTotal = hivScreening.numPrevalentCases + hivScreening.numIncidentCases;

	// Calculate sums for number at detection by CD4, HVL, HIV state
	for (int j = 0; j < SimContext::HIV_INF_NUM; j++) {
		for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
			hivScreening.numAtDetectionPrevalent += hivScreening.numAtDetectionPrevalentCD4HIV[i][j];
			hivScreening.numAtDetectionIncident += hivScreening.numAtDetectionIncidentCD4HIV[i][j];
			hivScreening.numAtDetectionPrevalentHIV[j] += hivScreening.numAtDetectionPrevalentCD4HIV[i][j];
			hivScreening.numAtDetectionIncidentHIV[j] += hivScreening.numAtDetectionIncidentCD4HIV[i][j];
			hivScreening.numAtDetectionPrevalentCD4[i] += hivScreening.numAtDetectionPrevalentCD4HIV[i][j];
			hivScreening.numAtDetectionIncidentCD4[i] += hivScreening.numAtDetectionIncidentCD4HIV[i][j];
		}
		for (int i = 0; i < SimContext::HVL_NUM_STRATA; i++) {
			hivScreening.numAtDetectionPrevalentHVL[i] += hivScreening.numAtDetectionPrevalentHVLHIV[i][j];
			hivScreening.numAtDetectionIncidentHVL[i] += hivScreening.numAtDetectionIncidentHVLHIV[i][j];
		}
	}
	for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
		if (hivScreening.numAtDetectionIncident > 0)
			hivScreening.percentAtDetectionIncidentCD4[i] = 100.0 * hivScreening.numAtDetectionIncidentCD4[i] / hivScreening.numAtDetectionIncident;
	}
	for (int i = 0; i < SimContext::HVL_NUM_STRATA; i++) {
		if (hivScreening.numAtDetectionIncident > 0)
			hivScreening.percentAtDetectionIncidentHVL[i] = 100.0 * hivScreening.numAtDetectionIncidentHVL[i] / hivScreening.numAtDetectionIncident;
	}

	// Calculate average CD4s at time of detection
	for (int i = 0; i < SimContext::HIV_INF_NUM; i++) {
		hivScreening.CD4AtDetectionPrevalentSum += hivScreening.CD4AtDetectionPrevalentSumHIV[i];
		hivScreening.CD4AtDetectionIncidentSum += hivScreening.CD4AtDetectionIncidentSumHIV[i];
		if (hivScreening.numAtDetectionPrevalentHIV[i] > 0)
			hivScreening.CD4AtDetectionPrevalentAverageHIV[i] = hivScreening.CD4AtDetectionPrevalentSumHIV[i] / hivScreening.numAtDetectionPrevalentHIV[i];
		if (hivScreening.numAtDetectionIncidentHIV[i] > 0)
			hivScreening.CD4AtDetectionIncidentAverageHIV[i] = hivScreening.CD4AtDetectionIncidentSumHIV[i] / hivScreening.numAtDetectionIncidentHIV[i];
	}
	if (hivScreening.numAtDetectionPrevalent > 0)
		hivScreening.CD4AtDetectionPrevalentAverage = hivScreening.CD4AtDetectionPrevalentSum / hivScreening.numAtDetectionPrevalent;
	if (hivScreening.numAtDetectionIncident > 0)
		hivScreening.CD4AtDetectionIncidentAverage = hivScreening.CD4AtDetectionIncidentSum / hivScreening.numAtDetectionIncident;

	// Calculate averages and standard deviations for time to infection and detection
	if (hivScreening.numIncidentCases > 0) {
		hivScreening.monthsToInfectionAverage = hivScreening.monthsToInfectionSum / hivScreening.numIncidentCases;
		hivScreening.monthsToInfectionStdDev = sqrt(hivScreening.monthsToInfectionSumSquares / hivScreening.numIncidentCases - hivScreening.monthsToInfectionAverage * hivScreening.monthsToInfectionAverage);
	}
	if (hivScreening.numAtDetectionPrevalent > 0) {
		hivScreening.monthsToDetectionPrevalentAverage = hivScreening.monthsToDetectionPrevalentSum / hivScreening.numAtDetectionPrevalent;
		hivScreening.monthsToDetectionPrevalentStdDev = sqrt(hivScreening.monthsToDetectionPrevalentSumSquares / hivScreening.numAtDetectionPrevalent - hivScreening.monthsToDetectionPrevalentAverage * hivScreening.monthsToDetectionPrevalentAverage);
		hivScreening.ageMonthsAtDetectionPrevalentAverage = hivScreening.ageMonthsAtDetectionPrevalentSum / hivScreening.numAtDetectionPrevalent;
		hivScreening.ageMonthsAtDetectionPrevalentStdDev = sqrt(hivScreening.ageMonthsAtDetectionPrevalentSumSquares / hivScreening.numAtDetectionPrevalent - hivScreening.ageMonthsAtDetectionPrevalentAverage * hivScreening.ageMonthsAtDetectionPrevalentAverage);
	}
	if (hivScreening.numAtDetectionIncident > 0) {
		hivScreening.monthsAfterInfectionToDetectionAverage = hivScreening.monthsAfterInfectionToDetectionSum / hivScreening.numAtDetectionIncident;
		hivScreening.monthsAfterInfectionToDetectionStdDev = sqrt(hivScreening.monthsAfterInfectionToDetectionSumSquares / hivScreening.numAtDetectionIncident - hivScreening.monthsAfterInfectionToDetectionAverage * hivScreening.monthsAfterInfectionToDetectionAverage);
		hivScreening.monthsToDetectionIncidentAverage = hivScreening.monthsToDetectionIncidentSum / hivScreening.numAtDetectionIncident;
		hivScreening.monthsToDetectionIncidentStdDev = sqrt(hivScreening.monthsToDetectionIncidentSumSquares / hivScreening.numAtDetectionIncident - hivScreening.monthsToDetectionIncidentAverage * hivScreening.monthsToDetectionIncidentAverage);
		hivScreening.ageMonthsAtDetectionIncidentAverage = hivScreening.ageMonthsAtDetectionIncidentSum / hivScreening.numAtDetectionIncident;
		hivScreening.ageMonthsAtDetectionIncidentStdDev = sqrt(hivScreening.ageMonthsAtDetectionIncidentSumSquares / hivScreening.numAtDetectionIncident - hivScreening.ageMonthsAtDetectionIncidentAverage * hivScreening.ageMonthsAtDetectionIncidentAverage);
	}

	// Calculate sums for test results by infection type
	for (int i = 0; i < SimContext::TEST_RESULT_NUM; i++) {
		hivScreening.numTestResultsPrevalent += hivScreening.numTestResultsPrevalentType[i];
		hivScreening.numTestResultsIncident += hivScreening.numTestResultsIncidentType[i];
		hivScreening.numTestResultsHIVNegative += hivScreening.numTestResultsHIVNegativeType[i];
	}
} /* end finalizeHIVScreening */

/* finalizeSurvivalStats calculates aggregate statistics for the SurvivalStats objects */
void RunStats::finalizeSurvivalStats() {
	if (patients.size() == 0)
		return;

	// Sort the patient summaries by life months
	sort(patients.begin(), patients.end(), PatientSummary::compareLMs());

	// Calculate the upper bound, lower bound, and median of the survival groups,
	//	also initial histogram starting point and bucket size
	int numTruncate = patients.size() * TRUNC_HISTOGRAM_PERC / 100;
	int lowerBoundNum[NUM_SURVIVAL_GROUPS];
	int upperBoundNum[NUM_SURVIVAL_GROUPS];
	int medianNum[NUM_SURVIVAL_GROUPS];
	int numCohorts[NUM_SURVIVAL_GROUPS];
	int currBucket[NUM_SURVIVAL_GROUPS];
	int bucketSize[NUM_SURVIVAL_GROUPS];
	for (int i = 0; i < NUM_SURVIVAL_GROUPS; i++) {
		if ((i == SURVIVAL_EXCL_SHORT) || (i == SURVIVAL_EXCL_LONG_AND_SHORT))
			lowerBoundNum[i] = numTruncate;
		else
			lowerBoundNum[i] = 0;
		if ((i == SURVIVAL_EXCL_LONG) || (i == SURVIVAL_EXCL_LONG_AND_SHORT))
			upperBoundNum[i] = patients.size() - numTruncate - 1;
		else
			upperBoundNum[i] = patients.size() - 1;
		numCohorts[i] = upperBoundNum[i] - lowerBoundNum[i] + 1;
		medianNum[i] = (upperBoundNum[i] + lowerBoundNum[i]) / 2;
		survivalStats[i].LMsMin = patients[lowerBoundNum[i]].LMs;
		survivalStats[i].LMsMax = patients[upperBoundNum[i]].LMs;
		survivalStats[i].LMsMedian = patients[medianNum[i]].LMs;
		currBucket[i] = (int) floor(survivalStats[i].LMsMin);
		bucketSize[i] = (int) floor((survivalStats[i].LMsMax - survivalStats[i].LMsMin) / MAX_NUM_HISTOGRAM_BUCKETS) + 1;
	}

	// Initital loop over the patient summaries only calculates mean values and histogram
	int patientNum = 0;
	for (vector<PatientSummary>::iterator i = patients.begin(); i != patients.end(); i++) {
		const PatientSummary &currPatient = *i;
		for (int j = 0; j < NUM_SURVIVAL_GROUPS; j++) {
			if ((patientNum < lowerBoundNum[j]) || (patientNum > upperBoundNum[j]))
				continue;
			survivalStats[j].LMsSum += currPatient.LMs;
			survivalStats[j].costsSum += currPatient.costs;
			survivalStats[j].QALMsSum += currPatient.QALMs;
			while (currPatient.LMs > currBucket[j]) {
				currBucket[j] += bucketSize[j];
				survivalStats[j].LMsHistogram[currBucket[j]] = 0;
			}
			survivalStats[j].LMsHistogram[currBucket[j]]++;
		}
		patientNum++;
	}
	for (int i = 0; i < NUM_SURVIVAL_GROUPS; i++) {
		survivalStats[i].LMsMean = survivalStats[i].LMsSum / numCohorts[i];
		survivalStats[i].costsMean = survivalStats[i].costsSum / numCohorts[i];
		survivalStats[i].QALMsMean = survivalStats[i].QALMsSum / numCohorts[i];
	}

	// Second loop calculates all other statistics, need means for skew and kurtosis
	patientNum = 0;
	for (vector<PatientSummary>::iterator i = patients.begin(); i != patients.end(); i++) {
		const PatientSummary &currPatient = *i;
		for (int j = 0; j < NUM_SURVIVAL_GROUPS; j++) {
			if ((patientNum < lowerBoundNum[j]) || (patientNum > upperBoundNum[j]))
				continue;
			double devLMMedian = survivalStats[j].LMsMedian - currPatient.LMs;
			devLMMedian = (devLMMedian < 0.0) ? -devLMMedian : devLMMedian;
			survivalStats[j].LMsSumDeviationMedian += devLMMedian;
			double devLM = currPatient.LMs - survivalStats[j].LMsMean;
			survivalStats[j].LMsSumDeviation += (devLM < 0.0) ? -devLM : devLM;;
			double devLMAccum = devLM * devLM;
			survivalStats[j].LMsSumDeviationSquares += devLMAccum;
			devLMAccum *= devLM;
			survivalStats[j].LMsSumDeviationCubes += devLMAccum;
			devLMAccum *= devLM;
			survivalStats[j].LMsSumDeviationQuads += devLMAccum;
			survivalStats[j].costsSumSquares += currPatient.costs * currPatient.costs;
			survivalStats[j].QALMsSumSquares += currPatient.QALMs * currPatient.QALMs;
		}
		patientNum++;
	}
	for (int i = 0; i < NUM_SURVIVAL_GROUPS; i++) {
		survivalStats[i].LMsAverageDeviationMedian = survivalStats[i].LMsSumDeviationMedian / numCohorts[i];
		survivalStats[i].LMsAverageDeviation = survivalStats[i].LMsSumDeviation / numCohorts[i];
		survivalStats[i].LMsVariance = survivalStats[i].LMsSumDeviationSquares / numCohorts[i];
		survivalStats[i].LMsStdDev = sqrt(survivalStats[i].LMsVariance);
		survivalStats[i].LMsSkew = survivalStats[i].LMsSumDeviationCubes / (numCohorts[i] * survivalStats[i].LMsVariance * survivalStats[i].LMsStdDev);
		survivalStats[i].LMsKurtosis = survivalStats[i].LMsSumDeviationQuads / (numCohorts[i] * survivalStats[i].LMsVariance * survivalStats[i].LMsVariance) - 3;
		survivalStats[i].costsStdDev = sqrt(survivalStats[i].costsSumSquares / numCohorts[i] - survivalStats[i].costsMean * survivalStats[i].costsMean);
		survivalStats[i].QALMsStdDev = sqrt(survivalStats[i].QALMsSumSquares / numCohorts[i] - survivalStats[i].QALMsMean * survivalStats[i].QALMsMean);
	}
} /* end finalizeSurvivalStats */

/* finalizeInitialDistributions calculates aggregate statistics for the InitialDistributions object */
void RunStats::finalizeInitialDistributions() {
	// Finalize initial distribution stats
	if (popSummary.numCohortsHIVPositive > 0)
		initialDistributions.averageInitialAgeMonths = initialDistributions.sumInitialAgeMonths / popSummary.numCohortsHIVPositive;
} /* end finalizeInitialDistributions */

/* finalizeCHRMsStats calculates aggregate statistics for the CHRMsStats object */
void RunStats::finalizeCHRMsStats() {
	for (int i = 0; i < SimContext::CHRM_NUM; i++) {
		for (int j = 0; j < SimContext::CD4_NUM_STRATA; j++) {
			chrmsStats.numPrevalentCHRM[i] +=  chrmsStats.numPrevalentCHRMCD4[i][j];
			chrmsStats.numPrevalentCD4[j] +=  chrmsStats.numPrevalentCHRMCD4[i][j];
			chrmsStats.numIncidentCHRM[i] +=  chrmsStats.numIncidentCHRMCD4[i][j];
			chrmsStats.numIncidentCD4[j] +=  chrmsStats.numIncidentCHRMCD4[i][j];
			chrmsStats.numDeathsCHRM[i] +=  chrmsStats.numDeathsCHRMCD4[i][j];
			chrmsStats.numDeathsCD4[j] +=  chrmsStats.numDeathsCHRMCD4[i][j];
		}
	}
} /* end finalizeCHRMsStats */

/* finalizeOIStats calculates aggregate statistics for the OIStats object */
void RunStats::finalizeOIStats() {
	// Accumulate the OI occurrence numbers
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		for (int j = 0; j < SimContext::CD4_NUM_STRATA; j++) {
			oiStats.numPrimaryOIsCD4[j] += oiStats.numPrimaryOIsCD4OI[j][i];
			oiStats.numPrimaryOIsOI[i] += oiStats.numPrimaryOIsCD4OI[j][i];
			oiStats.numSecondaryOIsCD4[j] += oiStats.numSecondaryOIsCD4OI[j][i];
			oiStats.numSecondaryOIsOI[i] += oiStats.numSecondaryOIsCD4OI[j][i];
			oiStats.numDetectedOIsCD4[j] += oiStats.numDetectedOIsCD4OI[j][i];
			oiStats.numDetectedOIsOI[i] += oiStats.numDetectedOIsCD4OI[j][i];
		}
	}

	// Accumulate the OI history numbers
	for (int j = 0; j < SimContext::HVL_NUM_STRATA; j++) {
		for (int k = 0; k < SimContext::CD4_NUM_STRATA; k++) {
			oiStats.numMonthsCD4[k] += oiStats.numMonthsHVLCD4[j][k];
			oiStats.numMonthsHVL[j] += oiStats.numMonthsHVLCD4[j][k];
			oiStats.numPatientsCD4[k] += oiStats.numPatientsHVLCD4[j][k];
			oiStats.numPatientsHVL[j] += oiStats.numPatientsHVLCD4[j][k];
			for (int i = 0; i < SimContext::OI_NUM; i++) {
				oiStats.numMonthsOIHistoryCD4[i][k] += oiStats.numMonthsOIHistoryHVLCD4[i][j][k];
				oiStats.numMonthsOIHistoryHVL[i][j] += oiStats.numMonthsOIHistoryHVLCD4[i][j][k];
				oiStats.numPatientsOIHistoryCD4[i][k] += oiStats.numPatientsOIHistoryHVLCD4[i][j][k];
				oiStats.numPatientsOIHistoryHVL[i][j] += oiStats.numPatientsOIHistoryHVLCD4[i][j][k];
			}
		}
	}

	// Calculate the OI history probabilities
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		for (int j = 0; j < SimContext::CD4_NUM_STRATA; j++) {
			if (oiStats.numPatientsCD4[j] > 0)
				oiStats.probPatientsOIHistoryCD4[i][j] = 1.0 * oiStats.numPatientsOIHistoryCD4[i][j] / oiStats.numPatientsCD4[j];
			if (oiStats.numMonthsCD4[j] > 0)
				oiStats.probMonthsOIHistoryCD4[i][j] = 1.0 * oiStats.numMonthsOIHistoryCD4[i][j] / oiStats.numMonthsCD4[j];
		}
	}
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		for (int j = 0; j < SimContext::HVL_NUM_STRATA; j++) {
			if (oiStats.numPatientsHVL[j] > 0)
				oiStats.probPatientsOIHistoryHVL[i][j] = 1.0 * oiStats.numPatientsOIHistoryHVL[i][j] / oiStats.numPatientsHVL[j];
			if (oiStats.numMonthsHVL[j] > 0)
				oiStats.probMonthsOIHistoryHVL[i][j] = 1.0 * oiStats.numMonthsOIHistoryHVL[i][j] / oiStats.numMonthsHVL[j];
		}
	}
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		for (int j = 0; j < SimContext::HVL_NUM_STRATA; j++) {
			for (int k = 0; k < SimContext::CD4_NUM_STRATA; k++) {
				if (oiStats.numPatientsHVLCD4[j][k] > 0)
					oiStats.probPatientsOIHistoryHVLCD4[i][j][k] = 1.0 * oiStats.numPatientsOIHistoryHVLCD4[i][j][k] / oiStats.numPatientsHVLCD4[j][k];
				if (oiStats.numMonthsHVLCD4[j][k] > 0)
					oiStats.probMonthsOIHistoryHVLCD4[i][j][k] = 1.0 * oiStats.numMonthsOIHistoryHVLCD4[i][j][k] / oiStats.numMonthsHVLCD4[j][k];
			}
		}
	}
} /* end finalizeOIStats */

/* finalizeDeathStats calculates aggregate statistics for the DeathStats object */
void RunStats::finalizeDeathStats() {
	// Finalize death stats
	for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
		for (int j = 0; j < SimContext::DTH_NUM_CAUSES; j++) {
			deathStats.numDeathsCD4[i] += deathStats.numDeathsCD4Type[i][j];
			deathStats.numDeathsType[j] += deathStats.numDeathsCD4Type[i][j];
		}
	}
	for (int i = 0; i < SimContext::HVL_NUM_STRATA; i++) {
		for (int j = 0; j < SimContext::CD4_NUM_STRATA; j++) {
			deathStats.numDeathsHVL[i] += deathStats.numDeathsHVLCD4[i][j];
		}
	}
	for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
		deathStats.numChronicAIDSDeathsNoOIHistory += deathStats.numChronicAIDSDeathsNoOIHistoryCD4[i];
		deathStats.numChronicAIDSDeathsOIHistory += deathStats.numChronicAIDSDeathsOIHistoryCD4[i];
		deathStats.numNonAIDSDeathsNoOIHistory += deathStats.numNonAIDSDeathsNoOIHistoryCD4[i];
		deathStats.numNonAIDSDeathsOIHistory += deathStats.numNonAIDSDeathsOIHistoryCD4[i];
	}
} /* end finalizeDeathStats */

/* finalizeOverallSurvival calculates aggregate statistics for the OverallSurvival object */
void RunStats::finalizeOverallSurvival() {
	// Finalize the overall survival stats
	for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
		overallSurvival.LMsTotalCD4[i] = overallSurvival.LMsNoOIHistoryCD4[i] + overallSurvival.LMsOIHistoryCD4[i];
		overallSurvival.LMsNoOIHistory += overallSurvival.LMsNoOIHistoryCD4[i];
		overallSurvival.LMsOIHistory += overallSurvival.LMsOIHistoryCD4[i];
		overallSurvival.LMsTotal += overallSurvival.LMsTotalCD4[i];
	}
	overallSurvival.LMsHIVPositive = overallSurvival.LMsHIVState[SimContext::HIV_ID_IDEN] +
		overallSurvival.LMsHIVState[SimContext::HIV_ID_UNID];
	overallSurvival.QALMsHIVPositive = overallSurvival.QALMsHIVState[SimContext::HIV_ID_IDEN] +
		overallSurvival.QALMsHIVState[SimContext::HIV_ID_UNID];
} /* end finalizeOverallSurvival */

/* finalizeOverallCosts calculates aggregate statistics for the OverallCosts object */
void RunStats::finalizeOverallCosts() {
	for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
		overallCosts.costsTotalCD4[i] = overallCosts.costsOIHistoryCD4[i] + overallCosts.costsNoOIHistoryCD4[i];
		overallCosts.costsTotal += overallCosts.costsTotalCD4[i];
		overallCosts.costsOIHistory += overallCosts.costsOIHistoryCD4[i];
		overallCosts.costsNoOIHistory += overallCosts.costsNoOIHistoryCD4[i];
	}
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		for (int j = 0; j < SimContext::PROPH_NUM; j++) {
			overallCosts.directCostsProph += overallCosts.directCostsProphOIsProph[i][j];
			overallCosts.directCostsProphOIs[i] += overallCosts.directCostsProphOIsProph[i][j];
		}
	}
	for (int i = 0; i < SimContext::ART_NUM_LINES; i++) {
		overallCosts.directCostsART += overallCosts.directCostsARTLine[i];
	}
	overallCosts.costsHIVPositive = overallCosts.costsHIVState[SimContext::HIV_ID_UNID] + overallCosts.costsHIVState[SimContext::HIV_ID_IDEN];
} /* end finalizeOverallCosts */

/* finalizeTBStats calculates aggregate statistics for the TBStats object */
void RunStats::finalizeTBStats() {

} /* end finalizeTBStats */

/* finalizeLTFUStats calculates aggregate statistics for the LTFUStats object */
void RunStats::finalizeLTFUStats() {
	for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
		ltfuStats.numLostToFollowUp += ltfuStats.numLostToFollowUpCD4[i];
		ltfuStats.numReturnToCare += ltfuStats.numReturnToCareCD4[i];
		ltfuStats.numDeathsWhileLost += ltfuStats.numDeathsWhileLostCD4[i];
	}
	if (ltfuStats.numReturnToCare > 0) {
		ltfuStats.monthsLostBeforeReturnMean = ltfuStats.monthsLostBeforeReturnSum / ltfuStats.numReturnToCare;
		ltfuStats.monthsLostBeforeReturnStdDev = sqrt(ltfuStats.monthsLostBeforeReturnSumSquares / ltfuStats.numReturnToCare - ltfuStats.monthsLostBeforeReturnMean * ltfuStats.monthsLostBeforeReturnMean);
	}
} /* end finalizeLTFUStats */

/* finalizeProphStats calculates aggregate statistics for the ProphStats object */
void RunStats::finalizeProphStats() {
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		for (int j = 0; j < SimContext::PROPH_NUM_TYPES; j++) {
			prophStats.numMajorToxicityTotal[i] += prophStats.numMajorToxicity[i][j];
			prophStats.numMinorToxicityTotal[i] += prophStats.numMinorToxicity[i][j];
			for (int k = 0; k < SimContext::PROPH_NUM_TYPES; k++) {
				if (prophStats.numTimesInitProph[k][i][j] > 0) {
					prophStats.trueCD4InitProphMean[k][i][j] = prophStats.trueCD4InitProphSum[k][i][j] / prophStats.numTimesInitProph[k][i][j];
					prophStats.observedCD4InitProphMean[k][i][j] = prophStats.observedCD4InitProphSum[k][i][j] / prophStats.numTimesInitProph[k][i][j];
				}
			}
		}
	}
} /* end finalizeProphStats */

/* finalizeARTStats calculates aggregate statistics for the ARTStats object */
void RunStats::finalizeARTStats() {
	for (int i = 0; i < SimContext::ART_NUM_LINES; i++) {
		// Finalize the months of ART suppression stats
		artStats.monthsSuppressed += artStats.monthsSuppressedLine[i];
		for (int j = 0; j < SimContext::HVL_NUM_STRATA; j++) {
			artStats.monthsPartiallySuppressedLine[i] += artStats.monthsPartiallySuppressedLineHVL[i][j];
			artStats.monthsPartiallySuppressedHVL[j] += artStats.monthsPartiallySuppressedLineHVL[i][j];
			artStats.monthsFailedLine[i] += artStats.monthsFailedLineHVL[i][j];
			artStats.monthsFailedHVL[j] += artStats.monthsFailedLineHVL[i][j];
		}

		// Finalize stats for beginning ART
		for (int j = 0; j < SimContext::RESP_NUM_TYPES; j++) {
			artStats.numOnARTAtInit[i] += artStats.numOnARTAtInitResp[i][j];
			artStats.trueCD4AtInitSum[i] += artStats.trueCD4AtInitSumResp[i][j];
			artStats.observedCD4AtInitSum[i] += artStats.trueCD4AtInitSumResp[i][j];
			for (int k = 0; k < SimContext::ART_EFF_NUM_TYPES; k++) {
				artStats.numDrawEfficacyAtInit[i][k] += artStats.numDrawEfficacyAtInitResp[i][k][j];
			}
			for (int k = 0; k < SimContext::CD4_RESPONSE_NUM_TYPES; k++) {
				artStats.numCD4ResponseTypeAtInit[i][k] += artStats.numCD4ResponseTypeAtInitResp[i][k][j];
			}
			for (int k = 0; k < SimContext::RISK_FACT_NUM; k++) {
				artStats.numWithRiskFactorAtInit[i][k] += artStats.numWithRiskFactorAtInitResp[i][k][j];
			}
			if (artStats.numOnARTAtInitResp[i][j] > 0) {
				artStats.trueCD4AtInitMeanResp[i][j] = artStats.trueCD4AtInitSumResp[i][j] / artStats.numOnARTAtInitResp[i][j];
				artStats.observedCD4AtInitMeanResp[i][j] = artStats.observedCD4AtInitSumResp[i][j] / artStats.numOnARTAtInitResp[i][j];
			}
		}
		if (artStats.numOnARTAtInit[i] > 0) {
			artStats.trueCD4AtInitMean[i] = artStats.trueCD4AtInitSum[i] / artStats.numOnARTAtInit[i];
			artStats.observedCD4AtInitMean[i] = artStats.observedCD4AtInitSum[i] / artStats.numOnARTAtInit[i];
		}

		// Finalize stats for true ART failures
		for (int j = 0; j < SimContext::RESP_NUM_TYPES; j++) {
			artStats.numTrueFailure[i] += artStats.numTrueFailureResp[i][j];
			artStats.trueCD4AtTrueFailureSum[i] += artStats.trueCD4AtTrueFailureSumResp[i][j];
			artStats.observedCD4AtTrueFailureSum[i] += artStats.observedCD4AtTrueFailureSumResp[i][j];
			artStats.monthsToTrueFailureSum[i] += artStats.monthsToTrueFailureSumResp[i][j];
			artStats.monthsToTrueFailureSumSquares[i] += artStats.monthsToTrueFailureSumSquaresResp[i][j];
			if (artStats.numTrueFailureResp[i][j] > 0) {
				artStats.trueCD4AtTrueFailureMeanResp[i][j] = artStats.trueCD4AtTrueFailureSumResp[i][j] / artStats.numTrueFailureResp[i][j];
				artStats.observedCD4AtTrueFailureMeanResp[i][j] = artStats.observedCD4AtTrueFailureSumResp[i][j] / artStats.numTrueFailureResp[i][j];
				artStats.monthsToTrueFailureMeanResp[i][j] = artStats.monthsToTrueFailureSumResp[i][j] / artStats.numTrueFailureResp[i][j];
				artStats.monthsToTrueFailureStdDevResp[i][j] = sqrt(artStats.monthsToTrueFailureSumSquaresResp[i][j] / artStats.numTrueFailureResp[i][j] - artStats.monthsToTrueFailureMeanResp[i][j] * artStats.monthsToTrueFailureMeanResp[i][j]);
			}
		}
		if (artStats.numTrueFailure[i] > 0) {
			artStats.trueCD4AtTrueFailureMean[i] = artStats.trueCD4AtTrueFailureSum[i] / artStats.numTrueFailure[i];
			artStats.observedCD4AtTrueFailureMean[i] = artStats.observedCD4AtTrueFailureSum[i] / artStats.numTrueFailure[i];
			artStats.monthsToTrueFailureMean[i] = artStats.monthsToTrueFailureSum[i] / artStats.numTrueFailure[i];
			artStats.monthsToTrueFailureStdDev[i] = sqrt(artStats.monthsToTrueFailureSumSquares[i] / artStats.numTrueFailure[i] - artStats.monthsToTrueFailureMean[i] * artStats.monthsToTrueFailureMean[i]);
		}

		// Finalize stats for observed ART failures
		for (int j = 0; j < SimContext::ART_NUM_FAIL_TYPES; j++) {
			artStats.numObservedFailure[i] += artStats.numObservedFailureType[i][j];
			artStats.numObservedFailureAfterTrue[i] += artStats.numObservedFailureAfterTrueType[i][j];
			artStats.trueCD4AtObservedFailureSum[i] += artStats.trueCD4AtObservedFailureSumType[i][j];
			artStats.observedCD4AtObservedFailureSum[i] += artStats.observedCD4AtObservedFailureSumType[i][j];
			artStats.monthsToObservedFailureSum[i] += artStats.monthsToObservedFailureSumType[i][j];
			artStats.monthsToObservedFailureSumSquares[i] += artStats.monthsToObservedFailureSumSquaresType[i][j];
			if (artStats.numObservedFailureType[i][j] > 0) {
				artStats.trueCD4AtObservedFailureMeanType[i][j] = artStats.trueCD4AtObservedFailureSumType[i][j] / artStats.numObservedFailureType[i][j];
				artStats.observedCD4AtObservedFailureMeanType[i][j] = artStats.observedCD4AtObservedFailureSumType[i][j] / artStats.numObservedFailureType[i][j];
				artStats.monthsToObservedFailureMeanType[i][j] = artStats.monthsToObservedFailureSumType[i][j] / artStats.numObservedFailureType[i][j];
				artStats.monthsToObservedFailureStdDevType[i][j] = sqrt(artStats.monthsToObservedFailureSumSquaresType[i][j] / artStats.numObservedFailureType[i][j] - artStats.monthsToObservedFailureMeanType[i][j] * artStats.monthsToObservedFailureMeanType[i][j]);
			}
		}
		artStats.numNeverObservedFailure[i] = artStats.numOnARTAtInit[i] - artStats.numObservedFailure[i];
		if (artStats.numObservedFailure[i] > 0) {
			artStats.trueCD4AtObservedFailureMean[i] = artStats.trueCD4AtObservedFailureSum[i] / artStats.numObservedFailure[i];
			artStats.observedCD4AtObservedFailureMean[i] = artStats.observedCD4AtObservedFailureSum[i] / artStats.numObservedFailure[i];
			artStats.monthsToObservedFailureMean[i] = artStats.monthsToObservedFailureSum[i] / artStats.numObservedFailure[i];
			artStats.monthsToObservedFailureStdDev[i] = sqrt(artStats.monthsToObservedFailureSumSquares[i] / artStats.numObservedFailure[i] - artStats.monthsToObservedFailureMean[i] * artStats.monthsToObservedFailureMean[i]);
		}

		// Finalize stats for stopping ART
		for (int j = 0; j < SimContext::ART_NUM_STOP_TYPES; j++) {
			artStats.numStop[i] += artStats.numStopType[i][j];
			artStats.numStopAfterTrueFailure[i] += artStats.numStopAfterTrueFailureType[i][j];
			artStats.trueCD4AtStopSum[i] += artStats.trueCD4AtStopSumType[i][j];
			artStats.observedCD4AtStopSum[i] += artStats.observedCD4AtStopSumType[i][j];
			artStats.monthsToStopSum[i] += artStats.monthsToStopSumType[i][j];
			artStats.monthsToStopSumSquares[i] += artStats.monthsToStopSumSquaresType[i][j];
			if (artStats.numStopType[i][j] > 0) {
				artStats.trueCD4AtStopMeanType[i][j] = artStats.trueCD4AtStopSumType[i][j] / artStats.numStopType[i][j];
				artStats.observedCD4AtStopMeanType[i][j] = artStats.observedCD4AtStopSumType[i][j] / artStats.numStopType[i][j];
				artStats.monthsToStopMeanType[i][j] = artStats.monthsToStopSumType[i][j] / artStats.numStopType[i][j];
				artStats.monthsToStopStdDevType[i][j] = sqrt(artStats.monthsToStopSumSquaresType[i][j] / artStats.numStopType[i][j] - artStats.monthsToStopMeanType[i][j] * artStats.monthsToStopMeanType[i][j]);
			}
		}
		artStats.numNeverStop[i] = artStats.numOnARTAtInit[i] - artStats.numStop[i];
		if (artStats.numStop[i] > 0) {
			artStats.trueCD4AtStopMean[i] = artStats.trueCD4AtStopSum[i] / artStats.numStop[i];
			artStats.observedCD4AtStopMean[i] = artStats.observedCD4AtStopSum[i] / artStats.numStop[i];
			artStats.monthsToStopMean[i] = artStats.monthsToStopSum[i] / artStats.numStop[i];
			artStats.monthsToStopStdDev[i] = sqrt(artStats.monthsToStopSumSquares[i] / artStats.numStop[i] - artStats.monthsToStopMean[i] * artStats.monthsToStopMean[i]);
		}

		// finalize stats for selected month efficacy totals
		for (int j = 0; j < SimContext::ART_NUM_MTHS_RECORD; j++) {
			if (artStats.numOnARTAtMonth[i][j] > 0) {
				artStats.HVLDropsAtMonthMean[i][j] = artStats.HVLDropsAtMonthSum[i][j] / artStats.numOnARTAtMonth[i][j];
				artStats.HVLDropsAtMonthStdDev[i][j] = sqrt(artStats.HVLDropsAtMonthSumSquares[i][j] / artStats.numOnARTAtMonth[i][j] - artStats.HVLDropsAtMonthMean[i][j] * artStats.HVLDropsAtMonthMean[i][j]);
			}
		}

		// finalize stats for STI
		for (int j = 0; j < SimContext::STI_NUM_CYCLES; j++) {
			artStats.numSTIInterruptionsSum[i] += artStats.numSTIInterruptions[i][j];
		}
		if (artStats.numOnARTAtInit[i] > 0) {
			artStats.numSTIInterruptionsMean[i] = artStats.numSTIInterruptionsSum[i] / artStats.numOnARTAtInit[i];
		}
		if (artStats.numSTIInterruptionsSum[i] > 0) {
			artStats.monthsOnSTIInterruptionMean[i] = artStats.monthsOnSTIInterruptionSum[i] / artStats.numSTIInterruptionsSum[i];
		}
	}
} /* end finalizeARTStats */

/* finalizeTimeSummaries calculates aggregate statistics for the TimeSummaries object */
void RunStats::finalizeTimeSummaries() {
	// Finalize the TimeSummary objects stats
	for (vector<TimeSummary *>::iterator i = timeSummaries.begin(); i != timeSummaries.end(); i++) {
		TimeSummary *currTime = *i;
		int numHIVPositive = 0;
		for (int j = 0; j < SimContext::HIV_ID_NUM; j++) {
			currTime->numAlive += currTime->numAliveType[j];
			if (j != SimContext::HIV_ID_NEG)
				numHIVPositive += currTime->numAliveType[j];
		}
		int numPedsHIVPositive = 0;
		for (int j = 0; j < SimContext::PEDS_HIV_NUM; j++) {
			if (j != SimContext::PEDS_HIV_NEG)
				numPedsHIVPositive += currTime->numAlivePediatrics[j];
		}
		int numObservedCD4 = 0;
		for (int i = 0; i < SimContext::CD4_NUM_STRATA; i++) {
			numObservedCD4 += currTime->observedCD4Distribution[i];
		}
		int numObservedHVL = 0;
		for (int i = 0; i < SimContext::HVL_NUM_STRATA; i++) {
			numObservedHVL += currTime->observedHVLDistribution[i];
		}
		if (numHIVPositive > 0) {
			currTime->trueCD4Mean = currTime->trueCD4Sum / numHIVPositive;
			currTime->trueCD4StdDev = sqrt(currTime->trueCD4SumSquares / numHIVPositive - currTime->trueCD4Mean * currTime->trueCD4Mean);
			currTime->trueHVLMean = currTime->trueHVLSum / numHIVPositive;
			currTime->trueHVLStdDev = sqrt(currTime->trueHVLSumSquares / numHIVPositive - currTime->trueHVLMean * currTime->trueHVLMean);
		}
		if (numPedsHIVPositive > 0) {
			currTime->trueCD4PercentageMean = currTime->trueCD4PercentageSum / numPedsHIVPositive;
			currTime->trueCD4PercentageStdDev = sqrt(currTime->trueCD4PercentageSumSquares / numPedsHIVPositive - currTime->trueCD4PercentageMean * currTime->trueCD4PercentageMean);
		}
		if (numObservedCD4 > 0) {
			currTime->observedCD4Mean = currTime->observedCD4Sum / numObservedCD4;
			currTime->observedCD4StdDev = sqrt(currTime->observedCD4SumSquares / numObservedCD4 - currTime->observedCD4Mean * currTime->observedCD4Mean);
		}
		if (numObservedHVL > 0) {
			currTime->observedHVLMean = currTime->observedHVLSum / numObservedHVL;
			currTime->observedHVLStdDev = sqrt(currTime->observedHVLSumSquares / numObservedHVL - currTime->observedHVLMean * currTime->observedHVLMean);
		}
		for (int j = 0; j < SimContext::ART_NUM_STATES; j++) {
			for (int k = 0; k < SimContext::CD4_NUM_STRATA; k++) {
				for (int l = 0; l < SimContext::HVL_NUM_STRATA; l++) {
					currTime->trueHVLDistribution[l] += currTime->trueCD4HVLARTDistribution[j][k][l];
					currTime->trueCD4ARTDistribution[j][k] += currTime->trueCD4HVLARTDistribution[j][k][l];
				}
			}
		}
	}
} /* end finalizeTimeSummaries */

/* writePopulationSummary outputs the PopulationSummary statistics to the stats file */
void RunStats::writePopulationSummary() {
	int i;

	// Print out the section header
	fprintf(statsFile, "POPULATION SUMMARY MEASURES (run completed %s,", popSummary.runDate.c_str() );
	fprintf(statsFile, "%s)\n[Program version %s, build %s]", popSummary.runTime.c_str(),
		CepacUtil::CEPAC_VERSION_STRING, CepacUtil::CEPAC_EXECUTABLE_COMPILED_DATE);

	// Print out costs and LM totals
	fprintf(statsFile,"\n\tOutcome/Measure \tAverage \tStd Dev \tLB \tUB");
	fprintf(statsFile,"\n\tCosts \t%1.0lf \t%1.0lf \t%1.0lf \t%1.0lf",
		popSummary.costsAverage, popSummary.costsStdDev,
		popSummary.costsLowerBound, popSummary.costsUpperBound);
	fprintf(statsFile,"\n\tLife Months \t%1.4lf \t%1.4lf \t%1.4lf \t%1.4lf",
		popSummary.LMsAverage, popSummary.LMsStdDev,
		popSummary.LMsLowerBound, popSummary.LMsUpperBound);
	fprintf(statsFile,"\n\tQuality-Adj Life Mths \t%1.4lf \t%1.4lf \t%1.4lf \t%1.4lf",
		popSummary.QALMsAverage, popSummary.QALMsStdDev,
		popSummary.QALMsLowerBound, popSummary.QALMsUpperBound);

	// output avg costs, LMs, QALMs by line of ARTs
	fprintf(statsFile,"\n\t \tAvg Costs \tAvg LMs \tAvg QALMs");
	for (i = 0; i < SimContext::ART_NUM_LINES + 1; ++i) {
		fprintf(statsFile,"\n\tUp to %d ART(s) obsv fail \t%1.0lf \t%1.4lf \t%1.4lf", i,
			popSummary.costsFailARTAverage[i], popSummary.LMsFailARTAverage[i],
			popSummary.QALMsFailARTAverage[i]);
	}
	fprintf(statsFile,"\n\tTotal Clinic Visits \t%1lu", popSummary.totalClinicVisits);
	fprintf(statsFile,"\n\tOnly HIV+ patients \t%1.0lf \t%1.4lf \t%1.4lf",
		popSummary.costsHIVPositiveAverage, popSummary.LMsHIVPositiveAverage,
		popSummary.QALMsHIVPositiveAverage);
} /* end writePopulationSummary */

/* writeHIVScreening outputs the HIVScreening statistics to the stats file */
void RunStats::writeHIVScreening() {
	int i, j;

	fprintf(statsFile, "\nHIV SCREENING MODULE");
	fprintf(statsFile, "\n\tHIV+ Cases (Preval):\t%lu", hivScreening.numPrevalentCases);
	fprintf(statsFile, "\t\tHIV- Cases (Preval):\t%lu", hivScreening.numHIVNegative);
	fprintf(statsFile, "\n\tHIV+ Cases (Incident):\t%lu", hivScreening.numIncidentCases);
	fprintf(statsFile, "\t\tAvgMths to Inf (Incid):\t%1.2lf\t(%1.2lf SD)",
		hivScreening.monthsToInfectionAverage, hivScreening.monthsToInfectionStdDev);
	fprintf(statsFile, "\n\tTotal HIV+ Cases:\t%lu", hivScreening.numHIVPositiveTotal);

	fprintf(statsFile, "\n\t");
	for (i = 0; i < SimContext::HIV_EXT_INF_NUM; ++i)
		fprintf(statsFile, "\t%s", SimContext::HIV_EXT_INF_STRS[(i+1)%SimContext::HIV_EXT_INF_NUM ]);
	fprintf(statsFile, "\n\tInit States (Prevalence):");
	for (i = 0; i < SimContext::HIV_EXT_INF_NUM; ++i)
		fprintf(statsFile, "\t%lu", hivScreening.numPatientsInitialHIVState[(i+1)%SimContext::HIV_EXT_INF_NUM ]);

	fprintf(statsFile, "\n\tCD4 at Detection (Preval):");
	for (i = 1; i < SimContext::HIV_INF_NUM; ++i)  // don't start at HIV neg; make sure mod this if HIVneg #def chgs
		fprintf(statsFile, "\t%s", SimContext::HIV_INF_STRS[i]);
	fprintf(statsFile, "\tTotal");
	for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j) {
		fprintf(statsFile, "\n\t%s", SimContext::CD4_STRATA_STRS[j]);
		for (i = 1; i < SimContext::HIV_INF_NUM; ++i) {
			fprintf(statsFile, "\t%lu", hivScreening.numAtDetectionPrevalentCD4HIV[j][i]);
		}
		fprintf(statsFile, "\t%lu", hivScreening.numAtDetectionPrevalentCD4[j]);
	}
	fprintf(statsFile, "\n\tTotal");
	for (i = 1; i < SimContext::HIV_INF_NUM; ++i)
		fprintf(statsFile, "\t%lu", hivScreening.numAtDetectionPrevalentHIV[i]);
	fprintf(statsFile, "\n\tMean CD4 Count:");
	for (i = 1; i < SimContext::HIV_INF_NUM; ++i) {
		fprintf(statsFile, "\t%1.2lf", hivScreening.CD4AtDetectionPrevalentAverageHIV[i]);
	}
	fprintf(statsFile, "\t%1.2lf", hivScreening.CD4AtDetectionPrevalentAverage);

	fprintf(statsFile, "\n\tHVL at Detection (Preval):");
	for (i = 1; i < SimContext::HIV_INF_NUM; ++i)  // don't start at HIV neg; make sure mod this if HIVneg #def chgs
		fprintf(statsFile, "\t%s", SimContext::HIV_INF_STRS[i]);
	fprintf(statsFile, "\tTotal");
	for (j = SimContext::HVL_NUM_STRATA - 1; j >= 0; --j) {
		fprintf(statsFile, "\n\t%s", SimContext::HVL_STRATA_STRS[j]);
		for (i = 1; i < SimContext::HIV_INF_NUM; ++i) {
			fprintf(statsFile, "\t%lu", hivScreening.numAtDetectionPrevalentHVLHIV[j][i]);
		}
		fprintf(statsFile, "\t%lu", hivScreening.numAtDetectionPrevalentHVL[j]);
	}

	fprintf(statsFile, "\n\tCD4 at Detection (Incid):");
	for (i = 1; i < SimContext::HIV_INF_NUM; ++i)  // don't start at HIV neg; make sure mod this if HIVneg #def chgs
		fprintf(statsFile, "\t%s", SimContext::HIV_INF_STRS[i]);
	fprintf(statsFile, "\tTotal\tTotal %%");
	for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j) {
		fprintf(statsFile, "\n\t%s", SimContext::CD4_STRATA_STRS[j]);
		for (i = 1; i < SimContext::HIV_INF_NUM; ++i) {
			fprintf(statsFile, "\t%lu", hivScreening.numAtDetectionIncidentCD4HIV[j][i]);
		}
		fprintf(statsFile, "\t%lu\t%1.2lf", hivScreening.numAtDetectionIncidentCD4[j],
			hivScreening.percentAtDetectionIncidentCD4[j]);
	}
	fprintf(statsFile, "\n\tTotal");
	for (i = 1; i < SimContext::HIV_INF_NUM; ++i)
		fprintf(statsFile, "\t%lu", hivScreening.numAtDetectionIncidentHIV[i]);
	fprintf(statsFile, "\n\tMean CD4 Count:");
	for (i = 1; i < SimContext::HIV_INF_NUM; ++i) {
		fprintf(statsFile, "\t%1.2lf", hivScreening.CD4AtDetectionIncidentAverageHIV[i]);
	}
	fprintf(statsFile, "\t%1.2lf", hivScreening.CD4AtDetectionIncidentAverage);

	fprintf(statsFile, "\n\tHVL at Detection (Incid):");
	for (i = 1; i < SimContext::HIV_INF_NUM; ++i)  // don't start at HIV neg; make sure mod this if HIVneg #def chgs
		fprintf(statsFile, "\t%s", SimContext::HIV_INF_STRS[i]);
	fprintf(statsFile, "\tTotal\tTotal %%");
	for (j = SimContext::HVL_NUM_STRATA - 1; j >= 0; --j) {
		fprintf(statsFile, "\n\t%s", SimContext::HVL_STRATA_STRS[j]);
		for (i = 1; i < SimContext::HIV_INF_NUM; ++i) {
			fprintf(statsFile, "\t%lu", hivScreening.numAtDetectionIncidentHVLHIV[j][i]);
		}
		fprintf(statsFile, "\t%lu\t%1.2lf", hivScreening.numAtDetectionIncidentHVL[j],
			hivScreening.percentAtDetectionIncidentHVL[j]);
	}

	fprintf(statsFile, "\n\tAvg Mths to Det (Preval):\t%1.2lf\t(%1.2lf SD)",
		hivScreening.monthsToDetectionPrevalentAverage, hivScreening.monthsToDetectionPrevalentStdDev);
	fprintf(statsFile, "\t\tAvg Age Mths at Det:\t%1.2lf\t(%1.2lf SD)",
		hivScreening.ageMonthsAtDetectionPrevalentAverage, hivScreening.ageMonthsAtDetectionPrevalentStdDev);
	fprintf(statsFile, "\n\tAvg Mths to Det (Incid):\t%1.2lf\t(%1.2lf SD)",
		hivScreening.monthsToDetectionIncidentAverage, hivScreening.monthsToDetectionIncidentStdDev);
	fprintf(statsFile, "\t\tAvg Age Mths at Det:\t%1.2lf\t(%1.2lf SD)",
		hivScreening.ageMonthsAtDetectionIncidentAverage, hivScreening.ageMonthsAtDetectionIncidentStdDev);
	fprintf(statsFile, "\n\tAvg Mths to Det Since Inf (Incid):\t%1.2lf\t(%1.2lf SD)",
		hivScreening.monthsAfterInfectionToDetectionAverage, hivScreening.monthsAfterInfectionToDetectionStdDev);
	fprintf(statsFile, "\n\t\tMale\tFemale");
	fprintf(statsFile, "\n\tGender at Detection:\t%lu\t%lu",
		hivScreening.numDetectedGender[SimContext::GENDER_MALE], hivScreening.numDetectedGender[SimContext::GENDER_FEMALE]);

	fprintf(statsFile, "\n\t");
	for (i = 0; i < SimContext::HIV_DET_NUM; ++i)
		fprintf(statsFile, "\t%s", SimContext::HIV_DET_STRS[i]);
	fprintf(statsFile, "\n\tMeans of Detection (Preval):");
	for (i = 0; i < SimContext::HIV_DET_NUM; ++i)
		fprintf(statsFile, "\t%lu", hivScreening.numDetectedPrevalentMeans[i]);
	fprintf(statsFile, "\n\tMeans of Detection (Incid):");
	for (i = 0; i < SimContext::HIV_DET_NUM; ++i)
		fprintf(statsFile, "\t%lu", hivScreening.numDetectedIncidentMeans[i]);
	fprintf(statsFile, "\n\t");
	for (i = 0; i < SimContext::OI_NUM; ++i)
		fprintf(statsFile, "\t%s", SimContext::OI_STRS[i]);
	fprintf(statsFile, "\n\tPresenting OI:");
	for (i = 0; i < SimContext::OI_NUM; ++i)
		fprintf(statsFile, "\t%lu", hivScreening.numDetectedByOIs[i]);

	fprintf(statsFile, "\n\t");
	for (i = 0; i < SimContext::HIV_EXT_INF_NUM; ++i)
		fprintf(statsFile, "\t%s", SimContext::HIV_EXT_INF_STRS[(i+1)%SimContext::HIV_EXT_INF_NUM ]);
	for (j = 0; j < SimContext::TEST_ACCEPT_NUM; ++j) {
		fprintf(statsFile, "\n\tPatient Test Accept Distrib %d:", j+1);
		for (i = 0; i < SimContext::HIV_EXT_INF_NUM; ++i)
			fprintf(statsFile, "\t%lu", hivScreening.numTestingAcceptRate[j][(i+1)%SimContext::HIV_EXT_INF_NUM]);
	}
	fprintf(statsFile, "\n\t\tAccept\tRefuse");
	fprintf(statsFile, "\n\tTest accepts:\t%lu\t%lu", hivScreening.numAcceptTest, hivScreening.numRefuseTest);
	fprintf(statsFile, "\n\tTest returns:\t%lu\t%lu", hivScreening.numReturnForResults, hivScreening.numNoReturnForResults);
	fprintf(statsFile, "\n\t");
	for (i = 0; i < SimContext::HIV_TEST_FREQ_NUM; ++i)
		fprintf(statsFile, "\t[/%dmths]", simContext->getHIVTestInputs()->HIVTestingInterval[i]);
	fprintf(statsFile, "\n\tPatient Testing Freq in Prog:");
	for (i = 0; i < SimContext::HIV_TEST_FREQ_NUM; ++i)
		fprintf(statsFile, "\t%lu", hivScreening.numTestingInterval[i]);

	fprintf(statsFile, "\n\t");
	for (i = 0; i < SimContext::TEST_RESULT_NUM; ++i)
		fprintf(statsFile, "\t%s", SimContext::TEST_RESULT_STRS[i]);
	fprintf(statsFile, "\tTotal");
	fprintf(statsFile, "\n\t#s Test Results (Preval):\t%lu\tN/A\tN/A\t%lu\t%lu",
		hivScreening.numTestResultsPrevalentType[SimContext::TEST_TRUE_POS],
		hivScreening.numTestResultsPrevalentType[SimContext::TEST_FALSE_NEG],
		hivScreening.numTestResultsPrevalent);
	fprintf(statsFile, "\n\t#s Test Results (Incid):\t%lu\tN/A\tN/A\t%lu\t%lu",
		hivScreening.numTestResultsIncidentType[SimContext::TEST_TRUE_POS],
		hivScreening.numTestResultsIncidentType[SimContext::TEST_FALSE_NEG],
		hivScreening.numTestResultsIncident);
	fprintf(statsFile, "\n\t#s Test Results (HIVneg):\tN/A\t%lu\t%lu\tN/A\t%lu",
		hivScreening.numTestResultsHIVNegativeType[SimContext::TEST_FALSE_POS],
		hivScreening.numTestResultsHIVNegativeType[SimContext::TEST_TRUE_NEG],
		hivScreening.numTestResultsHIVNegative);

	fprintf(statsFile, "\n\t");
	for (i = 0; i < SimContext::HIV_EXT_INF_NUM; ++i)
		fprintf(statsFile, "\t%s", SimContext::HIV_EXT_INF_STRS[(i+1)%SimContext::HIV_EXT_INF_NUM]);
	fprintf(statsFile, "\n\t#s Tests Done by State:");
	for (i = 0; i < SimContext::HIV_EXT_INF_NUM; ++i)
		fprintf(statsFile, "\t%lu", hivScreening.numTestsHIVState[(i+1)%SimContext::HIV_EXT_INF_NUM]);
} /* end HIVScreening */

/* writeSurvivalStats outputs the SurvivalStats statistics to the stats file */
void RunStats::writeSurvivalStats() {
	int numTruncate = patients.size() * TRUNC_HISTOGRAM_PERC / 100;
	for (int i = 0; i < NUM_SURVIVAL_GROUPS; i++) {
		SurvivalStats &currSurvival = survivalStats[i];

		// Print out the section headers
		switch (i) {
			case SURVIVAL_ALL:
				fprintf(statsFile, "\nLIFE MONTH SURVIVAL OF ENTIRE COHORT"); break;
			case SURVIVAL_EXCL_LONG:
				fprintf(statsFile, "\nLIFE MONTH SURVIVAL EXCLUDING LONGEST %d%% (%d patients) LMs",
					TRUNC_HISTOGRAM_PERC, numTruncate); break;
			case SURVIVAL_EXCL_SHORT:
				fprintf(statsFile, "\nLIFE MONTH SURVIVAL EXCLUDING SHORTEST %d%% (%d patients) LMs",
					TRUNC_HISTOGRAM_PERC, numTruncate); break;
			case SURVIVAL_EXCL_LONG_AND_SHORT:
				fprintf(statsFile, "\nLIFE MONTH SURVIVAL EXCLUDING LONGEST & SHORTEST %d%% (%d patients) LMs",
					TRUNC_HISTOGRAM_PERC, numTruncate*2);
		}

		// Print out the histogram information
		fprintf(statsFile, "\n\tLM bucket (UB, excl):");
		for (map<int,int>::iterator j = currSurvival.LMsHistogram.begin();
			j != currSurvival.LMsHistogram.end(); j++) {
				fprintf(statsFile, "\t%1ld", j->first);
		}
		fprintf(statsFile, "\n\t# Patients:");
		for (map<int,int>::iterator j = currSurvival.LMsHistogram.begin();
			j != currSurvival.LMsHistogram.end(); j++) {
				fprintf(statsFile, "\t%1ld", j->second);
		}

		// output min, max, median, & mean values
		fprintf(statsFile, "\n\tLM Min:\t%1.4lf\t\tLM Max:\t%1.4lf",
			currSurvival.LMsMin, currSurvival.LMsMax);
		fprintf(statsFile, "\n\tLM Median:\t%1.4lf\t\tLM AvgDev:\t%1.4lf",
			currSurvival.LMsMedian, currSurvival.LMsAverageDeviationMedian);
		fprintf(statsFile, "\n\tLM Mean:\t%1.4lf\t\tLM StdDev:\t%1.4lf\t\tLM AvgDev:\t%1.4lf",
			currSurvival.LMsMean, currSurvival.LMsStdDev, currSurvival.LMsAverageDeviation);
		fprintf(statsFile, "\n\tLM Variance:\t%1.4lf", currSurvival.LMsVariance);
		fprintf(statsFile, "\t\tLM Skew:\t%1.4lf\t\tLM Kurtosis:\t%1.4lf",
			currSurvival.LMsSkew, currSurvival.LMsKurtosis);

		// output cost and QALM values
		fprintf(statsFile, "\n\tCost Mean:\t%1.2lf\t\tCost StDv:\t%1.2lf",
			currSurvival.costsMean, currSurvival.costsStdDev);
		fprintf(statsFile, "\n\tQALM Mean:\t%1.4lf\t\tQALM StDv:\t%1.4lf",
			currSurvival.QALMsMean, currSurvival.QALMsStdDev);
	}
} /* end writeSurvivalStats */

/* writeInitialDistributions outputs the InitialDistributions statistics to the stats file */
void RunStats::writeInitialDistributions() {
	int i, j;
	fprintf(statsFile,"\nINITIAL DISTRIBUTIONS");
    fprintf(statsFile,"\n\tCD4 Count Level \t# Patients \t\tHVL Setpt Lvl \t# Patients \t\tCurr HVL Lvl \t# Patients");
	fprintf(statsFile,"\n\tVHI (>500) \t%1ld \t\tVHI (>100k) \t%1ld \t\tVHI \t%1ld",
		initialDistributions.numPatientsCD4Level[SimContext::CD4_VHI],
		initialDistributions.numPatientsHVLLevel[SimContext::HVL_VHI],
		initialDistributions.numPatientsHVLSetpointLevel[SimContext::HVL_VHI] );
	fprintf(statsFile,"\n\tHI (300-500) \t%1ld \t\tHI (30k-100k) \t%1ld \t\tHI \t%1ld",
		initialDistributions.numPatientsCD4Level[SimContext::CD4__HI],
		initialDistributions.numPatientsHVLLevel[SimContext::HVL__HI],
		initialDistributions.numPatientsHVLSetpointLevel[SimContext::HVL__HI] );
	fprintf(statsFile,"\n\tMHI (200-300) \t%1ld \t\tMHI (10k-30k) \t%1ld \t\tMHI \t%1ld",
		initialDistributions.numPatientsCD4Level[SimContext::CD4_MHI],
		initialDistributions.numPatientsHVLLevel[SimContext::HVL_MHI],
		initialDistributions.numPatientsHVLSetpointLevel[SimContext::HVL_MHI] );
	fprintf(statsFile,"\n\tMLO (100-200) \t%1ld \t\tMED (3k-10k) \t%1ld \t\tMED \t%1ld",
		initialDistributions.numPatientsCD4Level[SimContext::CD4_MLO],
		initialDistributions.numPatientsHVLLevel[SimContext::HVL_MED],
		initialDistributions.numPatientsHVLSetpointLevel[SimContext::HVL_MED] );
	fprintf(statsFile,"\n\tLO (50-100) \t%1ld \t\tMLO (500-3k) \t%1ld \t\tMLO \t%1ld",
		initialDistributions.numPatientsCD4Level[SimContext::CD4__LO],
		initialDistributions.numPatientsHVLLevel[SimContext::HVL_MLO],
		initialDistributions.numPatientsHVLSetpointLevel[SimContext::HVL_MLO] );
	fprintf(statsFile,"\n\tVLO (0-50) \t%1ld \t\tLO (20-500) \t%1ld \t\tLO \t%1ld",
		initialDistributions.numPatientsCD4Level[SimContext::CD4_VLO],
		initialDistributions.numPatientsHVLLevel[SimContext::HVL__LO],
		initialDistributions.numPatientsHVLSetpointLevel[SimContext::HVL__LO] );
	fprintf(statsFile,"\n\t \t \t\tVLO (0-20) \t%1ld \t\tVLO \t%1ld",
		initialDistributions.numPatientsHVLLevel[SimContext::HVL_VLO],
		initialDistributions.numPatientsHVLSetpointLevel[SimContext::HVL_VLO] );

	fprintf(statsFile,"\n\tAvg Init Age(Mths): \t%1.0lf \n\tMale Patients: \t%1ld \t\tFemale Patients: \t%1ld\n\t",
		initialDistributions.averageInitialAgeMonths,
		initialDistributions.numMalePatients, initialDistributions.numFemalePatients);
	for (i = 0; i < SimContext::OI_NUM; ++i)
		fprintf(statsFile," \t%s", SimContext::OI_STRS[i]);
	fprintf(statsFile,"\n\tPrior OI Histories Distrib:");
	for (i = 0; i < SimContext::OI_NUM; ++i)
		fprintf(statsFile," \t%ld", initialDistributions.numPriorOIHistories[i]);
	fprintf(statsFile, "\n\t");
	for (i = 0; i < SimContext::CD4_RESPONSE_NUM_TYPES; i++)
		fprintf(statsFile, "\t%s", SimContext::CD4_RESPONSE_STRS[i]);
	fprintf(statsFile, "\n\tART Response Types");
	for (i = 0; i < SimContext::CD4_RESPONSE_NUM_TYPES; i++)
		fprintf(statsFile, "\t%ld", initialDistributions.numARTResposneTypes[i]);

	fprintf(statsFile, "\n\t");
	for (i = 0; i < SimContext::RISK_FACT_NUM; i++) {
		fprintf(statsFile, "\tRisk %d", i + 1);
	}
	fprintf(statsFile, "\n\tRisk Factors");
	for (i = 0; i < SimContext::RISK_FACT_NUM; i++) {
		fprintf(statsFile, "\t%ld", initialDistributions.numRiskFactors[i]);
	}
	fprintf(statsFile, "\n\tPeds/Maternal");
	for (i = 0; i < SimContext::PEDS_MOM_HIV_NUM; i++) {
		fprintf(statsFile, "\t%s", SimContext::PEDS_MOM_HIV_STRS[i]);
	}
	for (i = 0; i < SimContext::PEDS_HIV_NUM; i++) {
		if (i == SimContext::PEDS_HIV_POS_PP)
			continue;
		fprintf(statsFile, "\n\t%s", SimContext::PEDS_HIV_STATE_STRS[i]);
		for (j = 0; j < SimContext::PEDS_MOM_HIV_NUM; j++) {
			fprintf(statsFile, "\t%ld", initialDistributions.numInitialPediatrics[i][j]);
		}
	}
} /* end writeInitialDistributions */

/* writeCHRMsStats outputs the CHRMsStats statistics to the stats file */
void RunStats::writeCHRMsStats() {
	fprintf(statsFile, "\nCHRMs SUMMARIES");

	fprintf(statsFile, "\n\tPrevalent");
	for (int i = 0; i < SimContext::CHRM_NUM; i++) {
		fprintf(statsFile, "\t%s", SimContext::CHRM_STRS[i]);
	}
	fprintf(statsFile, "\tTotal");
	for (int j = SimContext::CD4_NUM_STRATA - 1; j >= 0; j--) {
		fprintf(statsFile, "\n\t%s", SimContext::CD4_STRATA_STRS[j]);
		for (int i = 0; i < SimContext::CHRM_NUM; i++) {
			fprintf(statsFile, "\t%ld", chrmsStats.numPrevalentCHRMCD4[i][j]);
		}
		fprintf(statsFile, "\t%ld", chrmsStats.numPrevalentCD4[j]);
	}
	fprintf(statsFile, "\n\tTotal");
	for (int i = 0; i < SimContext::CHRM_NUM; i++) {
		fprintf(statsFile, "\t%ld", chrmsStats.numPrevalentCHRM[i]);
	}

	fprintf(statsFile, "\n\tIncident");
	for (int i = 0; i < SimContext::CHRM_NUM; i++) {
		fprintf(statsFile, "\t%s", SimContext::CHRM_STRS[i]);
	}
	fprintf(statsFile, "\tTotal");
	for (int j = SimContext::CD4_NUM_STRATA - 1; j >= 0; j--) {
		fprintf(statsFile, "\n\t%s", SimContext::CD4_STRATA_STRS[j]);
		for (int i = 0; i < SimContext::CHRM_NUM; i++) {
			fprintf(statsFile, "\t%ld", chrmsStats.numIncidentCHRMCD4[i][j]);
		}
		fprintf(statsFile, "\t%ld", chrmsStats.numIncidentCD4[j]);
	}
	fprintf(statsFile, "\n\tTotal");
	for (int i = 0; i < SimContext::CHRM_NUM; i++) {
		fprintf(statsFile, "\t%ld", chrmsStats.numIncidentCHRM[i]);
	}

	fprintf(statsFile, "\n\tDeaths");
	for (int i = 0; i < SimContext::CHRM_NUM; i++) {
		fprintf(statsFile, "\t%s", SimContext::CHRM_STRS[i]);
	}
	fprintf(statsFile, "\tTotal");
	for (int j = SimContext::CD4_NUM_STRATA - 1; j >= 0; j--) {
		fprintf(statsFile, "\n\t%s", SimContext::CD4_STRATA_STRS[j]);
		for (int i = 0; i < SimContext::CHRM_NUM; i++) {
			fprintf(statsFile, "\t%ld", chrmsStats.numDeathsCHRMCD4[i][j]);
		}
		fprintf(statsFile, "\t%ld", chrmsStats.numDeathsCD4[j]);
	}
	fprintf(statsFile, "\n\tTotal");
	for (int i = 0; i < SimContext::CHRM_NUM; i++) {
		fprintf(statsFile, "\t%ld", chrmsStats.numDeathsCHRM[i]);
	}
} /* end writeCHRMsStats */

/* writeOIStats outputs the OIStats statistics to the stats file */
void RunStats::writeOIStats() {
	int i, j, k;
	fprintf(statsFile,"\nOI SUMMARIES");
	fprintf(statsFile,"\n\tType/OI");

	// Write out total OI summaries
	for (i = 0; i < SimContext::OI_NUM; ++i)
		fprintf(statsFile," \t%s", SimContext::OI_STRS[i] );
	fprintf(statsFile,"\n\t# Primary OIs");
	for (i = 0; i < SimContext::OI_NUM; ++i)
		fprintf(statsFile," \t%1lu", oiStats.numPrimaryOIsOI[i]);
	fprintf(statsFile,"\n\t# Secondary OIs");
	for (i = 0; i < SimContext::OI_NUM; ++i)
		fprintf(statsFile," \t%1lu", oiStats.numSecondaryOIsOI[i]);

	// Write out number of primary, secondary, and detected OIs
	fprintf(statsFile,"\n\tPrimary OIs");
	for (i = 0; i < SimContext::OI_NUM; ++i)
		fprintf(statsFile," \t%s", SimContext::OI_STRS[i]);
	fprintf(statsFile," \tTotal");
	for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j) {
		fprintf(statsFile,"\n\t%s", SimContext::CD4_STRATA_STRS[j]);
		for (i = 0; i < SimContext::OI_NUM; ++i)
			fprintf(statsFile," \t%1lu", oiStats.numPrimaryOIsCD4OI[j][i]);
		fprintf(statsFile," \t%1lu", oiStats.numPrimaryOIsCD4[j]);
	}
	fprintf(statsFile,"\n\tSecondary OIs");
	for (i = 0; i < SimContext::OI_NUM; ++i)
		fprintf(statsFile," \t%s", SimContext::OI_STRS[i]);
	fprintf(statsFile," \tTotal");
	for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j) {
		fprintf(statsFile,"\n\t%s", SimContext::CD4_STRATA_STRS[j]);
		for (i = 0; i < SimContext::OI_NUM; ++i)
			fprintf(statsFile," \t%1lu", oiStats.numSecondaryOIsCD4OI[j][i]);
		fprintf(statsFile," \t%1lu", oiStats.numSecondaryOIsCD4[j]);
	}
	fprintf(statsFile,"\n\tDetected OIs");
	for (i = 0; i < SimContext::OI_NUM; ++i)
		fprintf(statsFile," \t%s", SimContext::OI_STRS[i]);
	fprintf(statsFile," \tTotal");
	for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j) {
		fprintf(statsFile,"\n\t%s", SimContext::CD4_STRATA_STRS[j]);
		for (i = 0; i < SimContext::OI_NUM; ++i) {
			fprintf(statsFile," \t%1lu", oiStats.numDetectedOIsCD4OI[j][i]);
		}
		fprintf(statsFile," \t%1lu", oiStats.numDetectedOIsCD4[j]);
	}
	fprintf(statsFile,"\n\tTotal");
	for (i = 0; i < SimContext::OI_NUM; ++i) {
		fprintf(statsFile," \t%1lu", oiStats.numDetectedOIsOI[i]);
	}

	// Print out prior OI history logging information
	fprintf(statsFile,"\nPRIOR OI HIST PROB AS PROPORTION OF PATIENTS (LOGGED)");
	for (i = 0; i < SimContext::OI_NUM; ++i) {
		fprintf(statsFile,"\n\tOI: %s", SimContext::OI_STRS[i]);
		for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j)
			fprintf(statsFile,"\t%s", SimContext::CD4_STRATA_STRS[j]);
		fprintf(statsFile,"\tTotal");
		for ( k = SimContext::HVL_NUM_STRATA - 1; k >= 0; --k ) {
			fprintf(statsFile,"\n\t%s", SimContext::HVL_STRATA_STRS[k]);
			for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j) {
				if (oiStats.numPatientsHVLCD4[k][j] == 0)
					fprintf(statsFile,"\tN/A");
				else {
					fprintf(statsFile,"\t%1.4lf", oiStats.probPatientsOIHistoryHVLCD4[i][k][j]);
				}  // else
			}  // for j
			if (oiStats.numPatientsHVL[k] == 0)
				fprintf(statsFile,"\tN/A");
			else
				fprintf(statsFile,"\t%1.4lf", oiStats.probPatientsOIHistoryHVL[i][k]);
		}  // for k
		fprintf(statsFile,"\n\tTotal");
		for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j) {
			if (oiStats.numPatientsCD4[j] == 0)
				fprintf(statsFile,"\tN/A");
			else
				fprintf(statsFile,"\t%1.4lf", oiStats.probPatientsOIHistoryCD4[i][j]);
		}  // for j
	}  // for i
	fprintf(statsFile,"\nPRIOR OI HIST PROB BY PATIENT MTHS (LOGGED)");
	for (i = 0; i < SimContext::OI_NUM; ++i) {
		fprintf(statsFile,"\n\tOI: %s", SimContext::OI_STRS[i]);
		for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j)
			fprintf(statsFile,"\t%s", SimContext::CD4_STRATA_STRS[j]);
		fprintf(statsFile,"\tTotal");
		for ( k = SimContext::HVL_NUM_STRATA - 1; k >= 0; --k ) {
			fprintf(statsFile,"\n\t%s", SimContext::HVL_STRATA_STRS[k]);
			for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j) {
				if (oiStats.numMonthsHVLCD4[k][j] == 0)
					fprintf(statsFile,"\tN/A");
				else {
					fprintf(statsFile,"\t%1.4lf", oiStats.probMonthsOIHistoryHVLCD4[i][k][j]);
				}  // else
			}  // for j
			if (oiStats.numMonthsHVL[k] == 0)
				fprintf(statsFile,"\tN/A");
			else
				fprintf(statsFile,"\t%1.4lf", oiStats.probMonthsOIHistoryHVL[i][k]);
		}  // for k
		fprintf(statsFile,"\n\tTotal");
		for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j) {
			if (oiStats.numMonthsCD4[j] == 0)
				fprintf(statsFile,"\tN/A");
			else
				fprintf(statsFile,"\t%1.4lf", oiStats.probMonthsOIHistoryCD4[i][j]);
		}  // for j
	}  // for i
} /* end writeOIStats */

/* writeDeathStats outputs the DeathStats statistics to the stats file */
void RunStats::writeDeathStats() {
	int i, j;
	fprintf(statsFile,"\nCAUSES OF DEATH");

	// Print out OI and other causes of death statistics
	fprintf(statsFile,"\n\tCD4 Count Level");
	for (i = 0; i < SimContext::DTH_NUM_CAUSES; ++i)
		fprintf(statsFile," \t%s", SimContext::DTH_CAUSES_STRS[i] );
	fprintf(statsFile," \tTotal");
	for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j) {
		fprintf(statsFile,"\n\t%s", SimContext::CD4_STRATA_STRS[j]);
		for (i = 0; i < SimContext::OI_NUM; ++i)
			fprintf(statsFile," \t%1lu", deathStats.numDeathsCD4Type[j][i]);
		for (i = SimContext::OI_NUM; i < SimContext::DTH_NUM_CAUSES; ++i)
			fprintf(statsFile," \t%1lu", deathStats.numDeathsCD4Type[j][i]);
		fprintf(statsFile," \t%1lu", deathStats.numDeathsCD4[j]);
	}
	fprintf(statsFile,"\n\t# Deaths");
	for (i = 0; i < SimContext::OI_NUM; ++i)
		fprintf(statsFile," \t%1lu", deathStats.numDeathsType[i]);
	for (i = SimContext::OI_NUM; i < SimContext::DTH_NUM_CAUSES; ++i)
		fprintf(statsFile," \t%1lu", deathStats.numDeathsType[i]);
	fprintf(statsFile," \t%1ld", popSummary.numCohorts);
	fprintf(statsFile,"\n\tHIVnegs");
	for (i = 0; i <= SimContext::DTH_CHRAIDS; ++i)
		fprintf(statsFile,"\tN/A");
	fprintf(statsFile," \t%1lu\tN/A\tN/A\t%1lu", deathStats.numDeathsUninfected, deathStats.numDeathsUninfected);

	// Print out CD4/HVL death distribution
	fprintf(statsFile,"\n\tDeath Distrib");
	for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j)
		fprintf(statsFile," \t%s", SimContext::CD4_STRATA_STRS[j]);
	fprintf(statsFile," \tTotal");
	for (i = SimContext::HVL_NUM_STRATA - 1; i >= 0; --i) {
		fprintf(statsFile,"\n\t%s", SimContext::HVL_STRATA_STRS[i]);
		for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j)
			fprintf(statsFile," \t%1lu", deathStats.numDeathsHVLCD4[i][j]);
		fprintf(statsFile," \t%1lu", deathStats.numDeathsHVL[i]);
	}

	// Print chronic and non-AIDS deaths by CD4 and OI history
	fprintf(statsFile,"\n\tCD4/OIhist");
	for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j)
		fprintf(statsFile," \t%s", SimContext::CD4_STRATA_STRS[j]);
	fprintf(statsFile," \tTotal");
	fprintf(statsFile," \n\tchrAIDS [woOIHist]");
	for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j)
		fprintf(statsFile," \t%1lu", deathStats.numChronicAIDSDeathsNoOIHistoryCD4[j]);
	fprintf(statsFile," \t%1lu", deathStats.numChronicAIDSDeathsNoOIHistory);
	fprintf(statsFile," \n\tchrAIDS [w.OIHist]");
	for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j)
		fprintf(statsFile," \t%1lu", deathStats.numChronicAIDSDeathsOIHistoryCD4[j]);
	fprintf(statsFile," \t%1lu", deathStats.numChronicAIDSDeathsOIHistory);
	fprintf(statsFile," \n\tnonAIDS [woOIHist]");
	for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j)
		fprintf(statsFile," \t%1lu", deathStats.numNonAIDSDeathsNoOIHistoryCD4[j]);
	fprintf(statsFile," \t%1lu", deathStats.numNonAIDSDeathsNoOIHistory);
	fprintf(statsFile," \n\tnonAIDS [w.OIHist]");
	for (j = SimContext::CD4_NUM_STRATA - 1; j >= 0; --j)
		fprintf(statsFile," \t%1lu", deathStats.numNonAIDSDeathsOIHistoryCD4[j]);
	fprintf(statsFile," \t%1lu", deathStats.numNonAIDSDeathsOIHistory);
} /* end writeDeathStats */

/* writeOverallSurvival outputs the OverallSurvival statistics to the stats file */
void RunStats::writeOverallSurvival() {
	int i;
    fprintf(statsFile,"\nOVERALL SURVIVAL");

	// output total LMs by CD4 strata (w/ and w/o history of any OI)
    fprintf(statsFile,"\n\tCD4 Strata: ");
	for (i = SimContext::CD4_NUM_STRATA - 1; i >= 0; --i)
		fprintf(statsFile,"\t%s", SimContext::CD4_STRATA_STRS[i]);
    fprintf(statsFile,"\tTotal");
    fprintf(statsFile,"\n\tLife Months [woOIHist]");
	for (i = SimContext::CD4_NUM_STRATA - 1; i >= 0; --i) {
		fprintf(statsFile,"\t%1.0lf", overallSurvival.LMsNoOIHistoryCD4[i]);
	}
	fprintf(statsFile,"\t%1.0lf", overallSurvival.LMsNoOIHistory);
    fprintf(statsFile,"\n\tLife Months [w.OIHist]");
	for (i = SimContext::CD4_NUM_STRATA - 1; i >= 0; --i) {
		fprintf(statsFile,"\t%1.0lf", overallSurvival.LMsOIHistoryCD4[i]);
	}
	fprintf(statsFile,"\t%1.0lf", overallSurvival.LMsOIHistory);
    fprintf(statsFile,"\n\tLife Months [Total]");
	for (i = SimContext::CD4_NUM_STRATA - 1; i >= 0; --i) {
		fprintf(statsFile,"\t%1.0lf", overallSurvival.LMsTotalCD4[i]);
	}
	fprintf(statsFile,"\t%1.0lf", overallSurvival.LMsTotal);

	// output total LMs by curr HVL and HVL setpoint
    fprintf(statsFile,"\n\tHVL Strata: ");
	for (i = SimContext::HVL_NUM_STRATA - 1; i >= 0; --i)
		fprintf(statsFile,"\t%s", SimContext::HVL_STRATA_STRS[i]);
    fprintf(statsFile,"\n\tLife Mths, HVL Setpt ");
	for (i = SimContext::HVL_NUM_STRATA - 1; i >= 0; --i)
	    fprintf(statsFile," \t%1.0lf", overallSurvival.LMsHVLSetpoint[i]);
    fprintf(statsFile,"\n\tLife Mths, Curr HVL ");
	for (i = SimContext::HVL_NUM_STRATA - 1; i >= 0; --i)
	    fprintf(statsFile," \t%1.0lf", overallSurvival.LMsHVL[i]);

	// output total LMs by history or no history of each indiv OI
    fprintf(statsFile,"\n\tOI: ");
	for (i = 0; i < SimContext::OI_NUM; ++i)
		fprintf(statsFile,"\t%s", SimContext::OI_STRS[i]);
    fprintf(statsFile,"\n\tLife Mths, no OI hist");
	for (i = 0; i < SimContext::OI_NUM; ++i)
	    fprintf(statsFile," \t%1.0lf", overallSurvival.LMsNoOIHistoryOIs[i]);
    fprintf(statsFile,"\n\tLife Mths, with OI hist");
	for (i = 0; i < SimContext::OI_NUM; ++i)
	    fprintf(statsFile," \t%1.0lf", overallSurvival.LMsOIHistoryOIs[i]);

	// output LMs by HIV screening states
	fprintf(statsFile, "\n\t");
	for (i = 0; i < SimContext::HIV_ID_NUM; ++i)
		fprintf(statsFile, "\t%s", SimContext::HIV_ID_STRS[i]);
	fprintf(statsFile, "\t(HIVpos)");
	fprintf(statsFile, "\n\tLMs by HIV State:");
	for (i = 0; i < SimContext::HIV_ID_NUM; ++i)
		fprintf(statsFile, "\t%1.0lf", overallSurvival.LMsHIVState[i]);
	fprintf(statsFile, "\t%1.0lf", overallSurvival.LMsHIVPositive);

	// output other misc LM stats
	fprintf(statsFile, "\n\t\tLMs\tQALMs");
	fprintf(statsFile, "\n\tLMs in HIV Scr Module:\t%1.0lf\t%1.0lf",
		overallSurvival.LMsInScreening, overallSurvival.QALMsInScreening);
	fprintf(statsFile, "\t\tLMs in \"Reg CEPAC\":\t%1.0lf", overallSurvival.LMsInRegularCEPAC);

	// output survival by gender
	fprintf(statsFile, "\n\t");
	for (i = 0; i < SimContext::GENDER_NUM; i++)
		fprintf(statsFile, "\t%s", SimContext::GENDER_STRS[i]);
	fprintf(statsFile, "\n\tLMs Gender");
	for (i = 0; i < SimContext::GENDER_NUM; i++)
		fprintf(statsFile, "\t%1.0lf", overallSurvival.LMsGender[i]);
	fprintf(statsFile, "\n\tQALMs Gender");
	for (i = 0; i < SimContext::GENDER_NUM; i++)
		fprintf(statsFile, "\t%1.0lf", overallSurvival.QALMsGender[i]);
} /* end writeOverallSurvival */

/* writeOverallCosts outputs the OverallCosts statistics to the stats file */
void RunStats::writeOverallCosts() {
	int i, j;
    fprintf(statsFile,"\nOVERALL COSTS");

	// output total costs by CD4 strata (w/ and w/o history of any OI)
    fprintf(statsFile,"\n\tCD4 Strata: ");
	for (i = SimContext::CD4_NUM_STRATA - 1; i >= 0; --i)
		fprintf(statsFile,"\t%s", SimContext::CD4_STRATA_STRS[i]);
    fprintf(statsFile,"\tTotal");
    fprintf(statsFile,"\n\tCosts [woOIHist]");
	for (i = SimContext::CD4_NUM_STRATA - 1; i >= 0; --i) {
	    fprintf(statsFile," \t%1.0lf", overallCosts.costsNoOIHistoryCD4[i]);
	}
	fprintf(statsFile,"\t%1.0lf", overallCosts.costsNoOIHistory);
    fprintf(statsFile,"\n\tCosts [w.OIHist]");
	for (i = SimContext::CD4_NUM_STRATA - 1; i >= 0; --i) {
		fprintf(statsFile," \t%1.0lf", overallCosts.costsOIHistoryCD4[i]);
	}
	fprintf(statsFile,"\t%1.0lf", overallCosts.costsOIHistory);
    fprintf(statsFile,"\n\tCosts [Total]");
	for (i = SimContext::CD4_NUM_STRATA - 1; i >= 0; --i) {
	    fprintf(statsFile," \t%1.0lf", overallCosts.costsTotalCD4[i]);
	}
	fprintf(statsFile,"\t%1.0lf", overallCosts.costsTotal);

	// output costs by HVL and HVL setpoint
    fprintf(statsFile,"\n\tHVL Strata: ");
	for (i = SimContext::HVL_NUM_STRATA - 1; i >= 0; --i)
		fprintf(statsFile,"\t%s", SimContext::HVL_STRATA_STRS[i]);
    fprintf(statsFile,"\n\tCosts, HVL Setpt ");
	for (i = SimContext::HVL_NUM_STRATA - 1; i >= 0; --i)
	    fprintf(statsFile," \t%1.0lf", overallCosts.costsHVLSetpoint[i]);
    fprintf(statsFile,"\n\tCosts, Curr HVL ");
	for (i = SimContext::HVL_NUM_STRATA - 1; i >= 0; --i)
	    fprintf(statsFile," \t%1.0lf", overallCosts.costsHVL[i]);

	// output proph and art costs
    fprintf(statsFile,"\n\tDirect Proph Costs");
	for (i = 0; i < SimContext::OI_NUM; ++i)
		fprintf(statsFile," \t%s", SimContext::OI_STRS[i]);
    fprintf(statsFile," \tAll");
	for (j = 0; j < SimContext::PROPH_NUM; ++j) {
	    fprintf(statsFile,"\n\tProph %d", j + 1);
		for (i = 0; i < SimContext::OI_NUM; ++i)
		    fprintf(statsFile," \t%1.0lf", overallCosts.directCostsProphOIsProph[i][j]);
	}
    fprintf(statsFile,"\n\tTotal Proph Costs");
	for (i = 0; i < SimContext::OI_NUM; ++i) {
	    fprintf(statsFile," \t%1.0lf", overallCosts.directCostsProphOIs[i]);
	}
    fprintf(statsFile," \t%1.0lf\n\t", overallCosts.directCostsProph);
	for (i = 0; i < SimContext::ART_NUM_LINES; ++i)
	    fprintf(statsFile," \tART %d", i+1);
	fprintf(statsFile,"\tTotal");
    fprintf(statsFile,"\n\tDirect ART Costs:");
	for (i = 0; i < SimContext::ART_NUM_LINES; ++i) {
	    fprintf(statsFile," \t%1.0lf", overallCosts.directCostsARTLine[i]);
	}
    fprintf(statsFile,"\t%1.0lf", overallCosts.directCostsART);

	// Other misc costs
	fprintf(statsFile,"\n\t \tCD4 Tests \tHVL Tests \tClinicVisits");
    fprintf(statsFile,"\n\tTesting Costs: \t%1.0lf \t%1.0lf \t%1.0lf",
		overallCosts.costsCD4Testing, overallCosts.costsHVLTesting, overallCosts.costsClinicVisits);
	fprintf(statsFile, "\n\t\tTests\tMisc");
	fprintf(statsFile, "\n\tHIV Screening Costs:\t%1.0lf\t%1.0lf",
		overallCosts.costsHIVScreeningTests, overallCosts.costsHIVScreeningMisc);
	fprintf(statsFile, "\n\t\tDirectMedical\tDirectNonMedical\tTimeCosts\tIndirect\tUnclassified\t\tDrugCosts\tToxicity");
	fprintf(statsFile, "\n\tTotal Undiscounted Costs:\t%1.0lf\t%1.0lf\t%1.0lf\t%1.0lf\t%1.0lf\t\t%1.0lf\t%1.0lf",
		overallCosts.totalUndiscountedCosts[0], overallCosts.totalUndiscountedCosts[1],
		overallCosts.totalUndiscountedCosts[2], overallCosts.totalUndiscountedCosts[3],
		overallCosts.totalUndiscountedCostsUnclassified, overallCosts.costsDrugs, overallCosts.costsToxicity);

	// output costs by gender
	fprintf(statsFile, "\n\t");
	for (i = 0; i < SimContext::GENDER_NUM; i++)
		fprintf(statsFile, "\t%s", SimContext::GENDER_STRS[i]);
	fprintf(statsFile, "\n\tCosts Gender");
	for (i = 0; i < SimContext::GENDER_NUM; i++)
		fprintf(statsFile, "\t%1.0lf", overallCosts.costsGender[i]);
} /* end writeOverallCosts */

/* writeTBStats outputs the TBStats statistics to the stats file */
void RunStats::writeTBStats() {
	int i, j;
	fprintf(statsFile, "\nTB SUMMARY EVENTS");

	fprintf(statsFile, "\n\tTB State On Entry:");
	for ( j = 0; j < SimContext::TB_NUM_STATES; ++j )
		fprintf(statsFile, "\t%s", SimContext::TB_STATE_STRS[j]);
	for ( i = 0; i < SimContext::TB_NUM_STRAINS; ++i ) {
		fprintf(statsFile, "\n\t%s", SimContext::TB_STRAIN_STRS[i]);
		for ( j = 0; j < SimContext::TB_NUM_STATES; ++j )
			fprintf(statsFile, "\t%lu", tbStats.numInStateAtEntry[i][j]);
	}
	fprintf(statsFile, "\n\tStart on TB Treatment:");
	for ( j = 0; j < SimContext::TB_TREATM_STAGE_NUM; ++j )
		fprintf(statsFile, "\t%s", SimContext::TB_TREATM_STAGE_STRS[j]);
	for ( i = 0; i < SimContext::TB_NUM_STRAINS; ++i ) {
		fprintf(statsFile, "\n\t%s", SimContext::TB_STRAIN_STRS[i]);
		for ( j = 0; j < SimContext::TB_TREATM_STAGE_NUM; ++j )
			fprintf(statsFile, "\t%lu", tbStats.numStartOnTreatment[i][j]);
	}
	fprintf(statsFile, "\n\tDropout TB Treatment:");
	for ( j = 0; j < SimContext::TB_TREATM_STAGE_NUM; ++j )
		fprintf(statsFile, "\t%s", SimContext::TB_TREATM_STAGE_STRS[j]);
	for ( i = 0; i < SimContext::TB_NUM_STRAINS; ++i ) {
		fprintf(statsFile, "\n\t%s", SimContext::TB_STRAIN_STRS[i]);
		for ( j = 0; j < SimContext::TB_TREATM_STAGE_NUM; ++j )
			fprintf(statsFile, "\t%lu", tbStats.numDropoutTreatment[i][j]);
	}
	fprintf(statsFile, "\n\tTB Cure at Treatment Dropout:");
	for ( j = 0; j < SimContext::TB_TREATM_STAGE_NUM; ++j )
		fprintf(statsFile, "\t%s", SimContext::TB_TREATM_STAGE_STRS[j]);
	for ( i = 0; i < SimContext::TB_NUM_STRAINS; ++i ) {
		fprintf(statsFile, "\n\t%s", SimContext::TB_STRAIN_STRS[i]);
		for ( j = 0; j < SimContext::TB_TREATM_STAGE_NUM; ++j )
			fprintf(statsFile, "\t%lu", tbStats.numCuredAtTreatmentDropout[i][j]);
	}
	fprintf(statsFile, "\n\tComplete TB Treatment:");
	for ( j = 0; j < SimContext::TB_TREATM_STAGE_NUM; ++j )
		fprintf(statsFile, "\t%s", SimContext::TB_TREATM_STAGE_STRS[j]);
	for ( i = 0; i < SimContext::TB_NUM_STRAINS; ++i ) {
		fprintf(statsFile, "\n\t%s", SimContext::TB_STRAIN_STRS[i]);
		for ( j = 0; j < SimContext::TB_TREATM_STAGE_NUM; ++j )
			fprintf(statsFile, "\t%lu", tbStats.numFinishTreatment[i][j]);
	}
	fprintf(statsFile, "\n\tTB Cure at Treatment Completion:");
	for ( j = 0; j < SimContext::TB_TREATM_STAGE_NUM; ++j )
		fprintf(statsFile, "\t%s", SimContext::TB_TREATM_STAGE_STRS[j]);
	for ( i = 0; i < SimContext::TB_NUM_STRAINS; ++i ) {
		fprintf(statsFile, "\n\t%s", SimContext::TB_STRAIN_STRS[i]);
		for ( j = 0; j < SimContext::TB_TREATM_STAGE_NUM; ++j )
			fprintf(statsFile, "\t%lu", tbStats.numCuredAtTreatmentFinish[i][j]);
	}
	fprintf(statsFile, "\n\tTB Increase Resistance at Treatment Completion:");
	for ( j = 0; j < SimContext::TB_TREATM_STAGE_NUM; ++j )
		fprintf(statsFile, "\t%s", SimContext::TB_TREATM_STAGE_STRS[j]);
	for ( i = 0; i < SimContext::TB_NUM_STRAINS; ++i ) {
		fprintf(statsFile, "\n\t%s", SimContext::TB_STRAIN_STRS[i]);
		for ( j = 0; j < SimContext::TB_TREATM_STAGE_NUM; ++j )
			fprintf(statsFile, "\t%lu", tbStats.numIncreaseResistanceAtTreatmentFinish[i][j]);
	}
	fprintf(statsFile, "\n\t");
	for ( i = 0; i < SimContext::TB_NUM_STRAINS; ++i )
		fprintf(statsFile, "\t%s", SimContext::TB_STRAIN_STRS[i]);
	fprintf(statsFile, "\n\tTotal TB Latent Infections");
	for ( i = 0; i < SimContext::TB_NUM_STRAINS; ++i )
		fprintf(statsFile, "\t%lu", tbStats.numLatentInfections[i]);
	fprintf(statsFile, "\n\tTotal TB Active Infections");
	for ( i = 0; i < SimContext::TB_NUM_STRAINS; ++i )
		fprintf(statsFile, "\t%lu", tbStats.numActiveInfections[i]);
	fprintf(statsFile, "\n\tTotal Reactivations from Latent TB");
	for ( i = 0; i < SimContext::TB_NUM_STRAINS; ++i )
		fprintf(statsFile, "\t%lu", tbStats.numReactivationsLatent[i]);
	fprintf(statsFile, "\n\tTotal Reinfections from Latent TB");
	for ( i = 0; i < SimContext::TB_NUM_STRAINS; ++i )
		fprintf(statsFile, "\t%lu", tbStats.numReinfectionsLatent[i]);
	fprintf(statsFile, "\n\tTotal Relapses from History Active TB");
	for ( i = 0; i < SimContext::TB_NUM_STRAINS; ++i )
		fprintf(statsFile, "\t%lu", tbStats.numRelapsesHistoryActive[i]);
	fprintf(statsFile, "\n\tTotal Reinfections from History Active TB");
	for ( i = 0; i < SimContext::TB_NUM_STRAINS; ++i )
		fprintf(statsFile, "\t%lu", tbStats.numReinfectionsHistoryActive[i]);
	fprintf(statsFile, "\n\tTotal TB Spontaneous Resolution");
	for ( i = 0; i < SimContext::TB_NUM_STRAINS; ++i )
		fprintf(statsFile, "\t%lu", tbStats.numSpontaneousResolutions[i]);
	fprintf(statsFile, "\n\tTotal Deaths from TB");
	for ( i = 0; i < SimContext::TB_NUM_STRAINS; ++i )
		fprintf(statsFile, "\t%lu", tbStats.numDeaths[i]);
	fprintf(statsFile, "\n\t");
	for ( i = 0; i < SimContext::TB_TREATM_STAGE_NUM; ++i )
		fprintf(statsFile, "\t%s", SimContext::TB_TREATM_STAGE_STRS[i]);
	fprintf(statsFile, "\n\tTotal TB Treatment Minor Toxicity");
	for ( i = 0; i < SimContext::TB_TREATM_STAGE_NUM; ++i )
		fprintf(statsFile, "\t%lu", tbStats.numTreatmentMinorToxicity[i]);
	fprintf(statsFile, "\n\tTotal TB Treatment Major Toxicity");
	for ( i = 0; i < SimContext::TB_TREATM_STAGE_NUM; ++i )
		fprintf(statsFile, "\t%lu", tbStats.numTreatmentMajorToxicity[i]);
	fprintf(statsFile, "\n\t");
	for ( i = 0; i < SimContext::PROPH_NUM; ++i )
		fprintf(statsFile, "\tProph%d", i + 1);
	fprintf(statsFile, "\n\tTotal TB Proph Minor Toxicity");
	for ( i = 0; i < SimContext::PROPH_NUM; ++i )
		fprintf(statsFile, "\t%lu", tbStats.numProphMinorToxicity[i]);
	fprintf(statsFile, "\n\tTotal TB Proph Major Toxicity");
	for ( i = 0; i < SimContext::PROPH_NUM; ++i )
		fprintf(statsFile, "\t%lu", tbStats.numProphMajorToxicity[i]);
} /* end writeTBStats */

/* writeLTFUStats outputs the LTFUStats statistics to the stats file */
void RunStats::writeLTFUStats() {
	int i, j;

	//print out LTFU/RTC stats
	fprintf(statsFile, "\nLOST TO FOLLOW UP AND RETURN TO CARE STATS\n");
	fprintf(statsFile, "\tTotal persons experiencing LTFU:\t%d\n", ltfuStats.numPatientsLost);
	fprintf(statsFile, "\tTotal persons experiencing RTC:\t%d\n", ltfuStats.numPatientsReturned);
	fprintf(statsFile, "\tTotal deaths while LTFU:\t%d\n\t", ltfuStats.numDeathsWhileLost);

	for (i = SimContext::CD4_NUM_STRATA - 1; i >= 0; --i)
		fprintf(statsFile,"\t%s", SimContext::CD4_STRATA_STRS[i]);
    fprintf(statsFile,"\tTotal");
    fprintf(statsFile,"\n\tTotal LTFU by CD4");
	for (i = SimContext::CD4_NUM_STRATA - 1; i >= 0; --i) {
	    fprintf(statsFile," \t%d", ltfuStats.numLostToFollowUpCD4[i]);
	}
	fprintf(statsFile,"\t%d", ltfuStats.numLostToFollowUp);
    fprintf(statsFile,"\n\tTotal RTC by CD4");
	for (i = SimContext::CD4_NUM_STRATA - 1; i >= 0; --i) {
	    fprintf(statsFile," \t%d", ltfuStats.numReturnToCareCD4[i]);
	}
	fprintf(statsFile,"\t%d", ltfuStats.numReturnToCare);
    fprintf(statsFile,"\n\tTotal Deaths while Lost by CD4");
	for (i = SimContext::CD4_NUM_STRATA - 1; i >= 0; --i) {
	    fprintf(statsFile," \t%d", ltfuStats.numDeathsWhileLostCD4[i]);
	}
	fprintf(statsFile,"\t%d\n\t", ltfuStats.numDeathsWhileLost);

	fprintf(statsFile,"\tMean \tStdDev \n\t");
	fprintf(statsFile,"Months Lost Before Returning:\t%1.2lf\t%1.2lf\n\t",
			ltfuStats.monthsLostBeforeReturnMean, ltfuStats.monthsLostBeforeReturnStdDev);

	for ( j = 0; j < SimContext::ART_NUM_LINES; ++j )
		fprintf(statsFile, "\tART%d", j+1);
	fprintf(statsFile, "\tPre-ART\tPost-ART(Off ART)\n");
	fprintf(statsFile, "\tTotal LTFU by ART:");
	for ( j = 0; j < SimContext::ART_NUM_LINES; ++j )
		fprintf(statsFile, "\t%d", ltfuStats.numLostToFollowUpART[j]);
	fprintf(statsFile, "\t%d", ltfuStats.numLostToFollowUpPreART);
	fprintf(statsFile, "\t%d", ltfuStats.numLostToFollowUpPostART);
	fprintf(statsFile, "\n\tTotal RTC by ART continuing on previous ART:");
	for ( j = 0; j < SimContext::ART_NUM_LINES; ++j )
		fprintf(statsFile, "\t%d", ltfuStats.numReturnOnPrevART[j]);
	fprintf(statsFile, "\t%d", ltfuStats.numReturnToCarePreART);
	fprintf(statsFile, "\t%d", ltfuStats.numReturnToCarePostART);
	fprintf(statsFile, "\n\tTotal RTC by ART switching to next ART:");
	for ( j = 0; j < SimContext::ART_NUM_LINES; ++j )
		fprintf(statsFile, "\t%d", ltfuStats.numReturnOnNextART[j]);
	fprintf(statsFile, "\n\tTotal Death while Lost by ART:");
	for ( j = 0; j < SimContext::ART_NUM_LINES; ++j )
		fprintf(statsFile, "\t%d", ltfuStats.numDeathsWhileLostART[j]);
	fprintf(statsFile, "\t%d", ltfuStats.numDeathsWhileLostPreART);
	fprintf(statsFile, "\t%d", ltfuStats.numDeathsWhileLostPostART);
} /* end writeLTFUStats */

/* writeProphStats outputs the ProphStats statistics to the stats file */
void RunStats::writeProphStats() {
	int i,j;

	// output proph toxicities stats
	fprintf(statsFile,"\nOI PROPH TOXICITY EVENTS");
	fprintf(statsFile,"\n\tMinor Tox Events");
	for (i = 0; i < SimContext::OI_NUM; ++i)
		fprintf(statsFile," \t%s", SimContext::OI_STRS[i]);
	for (i = 0; i < SimContext::PROPH_NUM; ++i) {
		fprintf(statsFile,"\n\tProph %d", i+1);
		for (j = 0; j < SimContext::OI_NUM; ++j)
			fprintf(statsFile," \t%1lu", prophStats.numMinorToxicity[j][i]);
	}
	fprintf(statsFile,"\n\tTotal");
	for (j = 0; j < SimContext::OI_NUM; ++j)
		fprintf(statsFile," \t%1lu", prophStats.numMinorToxicityTotal[j]);
	fprintf(statsFile,"\n\tMajor Tox Events");
	for (i = 0; i < SimContext::OI_NUM; ++i)
		fprintf(statsFile," \t%s", SimContext::OI_STRS[i]);
	for (i = 0; i < SimContext::PROPH_NUM; ++i) {
		fprintf(statsFile,"\n\tProph %d", i+1);
		for (j = 0; j < SimContext::OI_NUM; ++j)
			fprintf(statsFile," \t%1lu", prophStats.numMajorToxicity[j][i]);
	}
	fprintf(statsFile,"\n\tTotal");
	for (j = 0; j < SimContext::OI_NUM; ++j)
		fprintf(statsFile," \t%1lu", prophStats.numMajorToxicityTotal[j]);

	// print out all primary proph stats
	fprintf(statsFile, "\nPRIMARY OI PROPH MEAN CD4 AT INIT\n\t");
	for ( i = 0; i < SimContext::PROPH_NUM; ++i )
		fprintf(statsFile, "\tProph%d True\tObsv CD4\tTimes Init'd", i+1, i+1);
	for ( j = 0; j < SimContext::OI_NUM; ++j ) {
		fprintf(statsFile, "\n\t%s", SimContext::OI_STRS[j]);
		for ( i = 0; i < SimContext::PROPH_NUM; ++i ) {
			fprintf(statsFile, "\t%1.0lf \t%1.0lf \t%d",
				prophStats.trueCD4InitProphMean[SimContext::PROPH_PRIMARY][j][i],
				prophStats.observedCD4InitProphMean[SimContext::PROPH_PRIMARY][j][i],
				prophStats.numTimesInitProph[SimContext::PROPH_PRIMARY][j][i]);
		}
	}

	// print out all secondary proph stats
	fprintf(statsFile, "\nSECONDARY OI PROPH MEAN CD4 AT INIT\n\t");
	for ( i = 0; i < SimContext::PROPH_NUM; ++i )
		fprintf(statsFile, "\tProph%d True\tObsv CD4\tTimes Init'd", i+1, i+1);
	for ( j = 0; j < SimContext::OI_NUM; ++j ) {
		fprintf(statsFile, "\n\t%s", SimContext::OI_STRS[j]);
		for ( i = 0; i < SimContext::PROPH_NUM; ++i ) {
			fprintf(statsFile, "\t%1.0lf \t%1.0lf \t%d",
				prophStats.trueCD4InitProphMean[SimContext::PROPH_SECONDARY][j][i],
				prophStats.observedCD4InitProphMean[SimContext::PROPH_SECONDARY][j][i],
				prophStats.numTimesInitProph[SimContext::PROPH_SECONDARY][j][i]);
		}
	}
} /* end ProphStats */

/* writeARTStats outputs the ARTStats statistics to the stats file */
void RunStats::writeARTStats() {
	int i, j, k;
	const SimContext::RunSpecsInputs *runSpecs = simContext->getRunSpecsInputs();

	// print out months on partially suppressed or failed ART
	fprintf(statsFile, "\nMONTHS IN SUPPRESSED/PARTIALLY SUPPRESSED/FAILED STATES ON ART\n\t");
	for ( j = 0; j < SimContext::ART_NUM_LINES; ++j )
		fprintf(statsFile, "\tART%d", j+1);
	fprintf(statsFile, "\tTotal");
	fprintf(statsFile, "\nMonths suppressed\t");
	for ( j = 0; j < SimContext::ART_NUM_LINES; ++j ) {
		fprintf(statsFile, "\t%1lu", artStats.monthsSuppressedLine[j]);
	}
	fprintf(statsFile, "\t%1lu", artStats.monthsSuppressed);
	fprintf(statsFile, "\nMonths partially suppressed");
	for ( i = SimContext::HVL_NUM_STRATA - 1; i >= 0; --i ) {
		fprintf(statsFile, "\n\t%s", SimContext::HVL_STRATA_STRS[i]);
		for ( j = 0; j < SimContext::ART_NUM_LINES; ++j ) {
			fprintf(statsFile, "\t%1lu", artStats.monthsPartiallySuppressedLineHVL[j][i]);
		}
		fprintf(statsFile, "\t%1lu", artStats.monthsPartiallySuppressedHVL[i]);
	}
	fprintf(statsFile, "\n\tTotal");
	for ( j = 0; j < SimContext::ART_NUM_LINES; ++j ) {
		fprintf(statsFile, "\t%1lu", artStats.monthsPartiallySuppressedLine[j]);
	}
	fprintf(statsFile, "\nMonths failed");
	for ( i = SimContext::HVL_NUM_STRATA - 1; i >= 0; --i ) {
		fprintf(statsFile, "\n\t%s", SimContext::HVL_STRATA_STRS[i]);
		for ( j = 0; j < SimContext::ART_NUM_LINES; ++j ) {
			fprintf(statsFile, "\t%1lu", artStats.monthsFailedLineHVL[j][i]);
		}
		fprintf(statsFile, "\t%1lu", artStats.monthsFailedHVL[i]);
	}
	fprintf(statsFile, "\n\tTotal");
	for ( j = 0; j < SimContext::ART_NUM_LINES; ++j ) {
		fprintf(statsFile, "\t%1lu", artStats.monthsFailedLine[j]);
	}

	// print out all ART stats
	for ( j = 0; j < SimContext::ART_NUM_LINES; ++j ) {
		fprintf(statsFile, "\nART %d STATS", j+1);
		// print out ART initiation stats
		fprintf(statsFile, "\n\tAt init: \t# Total \tAvgTrueCD4 \tAvgObsvCD4 \t#Drawn Supp \t#Drawn Part Supp \t#Drawn Fail");
		for (k = 0; k < SimContext::CD4_RESPONSE_NUM_TYPES; k++) {
			fprintf(statsFile, "\tCD4 Type %1lu", k + 1);
		}
		for (k = 0; k < SimContext::RISK_FACT_NUM; k++) {
			fprintf(statsFile, "\tRisk Factor %1lu", k + 1);
		}
		fprintf(statsFile, "\n\tAny response: \t%1lu \t%1.2lf \t%1.2lf",
			artStats.numOnARTAtInit[j], artStats.trueCD4AtInitMean[j], artStats.observedCD4AtInitMean[j]);
		for (k = 0; k < SimContext::ART_EFF_NUM_TYPES; k++) {
			fprintf(statsFile, "\t%1lu", artStats.numDrawEfficacyAtInit[j][k]);
		}
		for (k = 0; k < SimContext::CD4_RESPONSE_NUM_TYPES; k++) {
			fprintf(statsFile, "\t%1lu", artStats.numCD4ResponseTypeAtInit[j][k]);
		}
		for (k = 0; k < SimContext::RISK_FACT_NUM; k++) {
			fprintf(statsFile, "\t%1lu", artStats.numWithRiskFactorAtInit[j][k]);
		}
		for (i = 0; i < SimContext::RESP_NUM_TYPES; i++) {
			fprintf(statsFile, "\n\t%s \t%1lu \t%1.2lf \t%1.2lf", SimContext::RESP_TYPE_STRS[i],
				artStats.numOnARTAtInitResp[j][i], artStats.trueCD4AtInitMeanResp[j][i], artStats.observedCD4AtInitMeanResp[j][i]);
			for (k = 0; k < SimContext::ART_EFF_NUM_TYPES; k++) {
				fprintf(statsFile, "\t%1lu", artStats.numDrawEfficacyAtInitResp[j][k][i]);
			}
			for (k = 0; k < SimContext::CD4_RESPONSE_NUM_TYPES; k++) {
				fprintf(statsFile, "\t%1lu", artStats.numCD4ResponseTypeAtInitResp[j][k][i]);
			}
			for (k = 0; k < SimContext::RISK_FACT_NUM; k++) {
				fprintf(statsFile, "\t%1lu", artStats.numWithRiskFactorAtInitResp[j][k][i]);
			}
		}

		// print out ART true failure stats
		fprintf(statsFile, "\n\tAt ART true fail: \t# Total \tAvgTrueCD4 \tAvgObsvCD4 \tMthsToFail(Mean) \tMthsToFail(StdDev)");
		fprintf(statsFile, "\n\tAny response: \t%1lu \t%1.2lf \t%1.2lf \t%1.2lf \t%1.2lf",
			artStats.numTrueFailure[j], artStats.trueCD4AtTrueFailureMean[j],
			artStats.observedCD4AtTrueFailureMean[j], artStats.monthsToTrueFailureMean[j],
			artStats.monthsToTrueFailureStdDev[j]);
		for (i = 0; i < SimContext::RESP_NUM_TYPES; i++) {
			fprintf(statsFile, "\n\t%s \t%1lu \t%1.2lf \t%1.2lf \t%1.2lf \t%1.2lf", SimContext::RESP_TYPE_STRS[i],
				artStats.numTrueFailureResp[j][i], artStats.trueCD4AtTrueFailureMeanResp[j][i],
				artStats.observedCD4AtTrueFailureMeanResp[j][i], artStats.monthsToTrueFailureMeanResp[j][i],
				artStats.monthsToTrueFailureStdDevResp[j][i]);
		}

		// print ART observed failure statistics
		fprintf(statsFile, "\n\tAt ART obsv fail: \t# Total \t# with true fail \tAvgTrueCD4 \tAvgObsvCD4 \tMthsToObsvFail(Mean) \tMthsToObsvFail(StdDev)");
		fprintf(statsFile, "\n\tAny fail diagnosis \t%1lu \t%1lu \t%1.2lf \t%1.2lf \t%1.2lf \t%1.2lf",
			artStats.numObservedFailure[j], artStats.numObservedFailureAfterTrue[j],
			artStats.trueCD4AtObservedFailureMean[j], artStats.observedCD4AtObservedFailureMean[j],
			artStats.monthsToObservedFailureMean[j], artStats.monthsToObservedFailureStdDev[j]);
		for (i = 0; i < SimContext::ART_NUM_FAIL_TYPES; ++i) {
			fprintf(statsFile, "\n\t%s \t%1lu \t%1lu \t%1.2lf \t%1.2lf \t%1.2lf \t%1.2lf",
				SimContext::ART_FAIL_TYPE_STRS[i],
				artStats.numObservedFailureType[j][i], artStats.numObservedFailureAfterTrueType[j][i],
				artStats.trueCD4AtObservedFailureMeanType[j][i], artStats.observedCD4AtObservedFailureMeanType[j][i],
				artStats.monthsToObservedFailureMeanType[j][i], artStats.monthsToObservedFailureStdDevType[j][i]);
		}
		fprintf(statsFile, "\n\tNo fail diagnoses \t%1lu", artStats.numNeverObservedFailure[j]);

		// print ART stop statistics
		fprintf(statsFile, "\n\tAt ART stop: \t# Total \t# with true fail \tAvgTrueCD4 \tAvgObsvCD4 \tMthsToStop(Mean) \tMthsToStop(StdDev)");
		fprintf(statsFile, "\n\tAll \t%1lu \t%1lu \t%1.2lf \t%1.2lf \t%1.2lf \t%1.2lf",
			artStats.numStop[j], artStats.numStopAfterTrueFailure[j],
			artStats.trueCD4AtStopMean[j], artStats.observedCD4AtStopMean[j],
			artStats.monthsToStopMean[j], artStats.monthsToStopStdDev[j]);
		for (i = 0; i < SimContext::ART_NUM_STOP_TYPES; ++i) {
			fprintf(statsFile, "\n\t%s \t%1lu \t%1lu \t%1.2lf \t%1.2lf \t%1.2lf \t%1.2lf",
				SimContext::ART_STOP_TYPE_STRS[i],
				artStats.numStopType[j][i], artStats.numStopAfterTrueFailureType[j][i],
				artStats.trueCD4AtStopMeanType[j][i], artStats.observedCD4AtStopMeanType[j][i],
				artStats.monthsToStopMeanType[j][i], artStats.monthsToStopStdDevType[j][i]);
		}
		fprintf(statsFile, "\n\tNever stopped \t%1lu", artStats.numNeverStop[j]);

		// print number of patients on ART at intervals
		fprintf(statsFile, "\n\t \t# at Mth %d \t# Supp, Mth %d \t# at Mth %d \t# Supp, Mth %d \t# at Mth %d \t# Supp, Mth %d",
			runSpecs->monthRecordARTEfficacy[0], runSpecs->monthRecordARTEfficacy[0],
			runSpecs->monthRecordARTEfficacy[1], runSpecs->monthRecordARTEfficacy[1],
			runSpecs->monthRecordARTEfficacy[2], runSpecs->monthRecordARTEfficacy[2]);
		fprintf(statsFile, "\n\tNumber Patients: \t%1lu \t%1lu \t%1lu \t%1lu \t%1lu \t%1lu",
			artStats.numOnARTAtMonth[j][0], artStats.numSuppressedAtMonth[j][0],
			artStats.numOnARTAtMonth[j][1], artStats.numSuppressedAtMonth[j][1],
			artStats.numOnARTAtMonth[j][2], artStats.numSuppressedAtMonth[j][2]);
		fprintf(statsFile, "\n\t \tMean, Mth %d \tSD, Mth %d \tMean, Mth %d \tSD, Mth %d \tMean, Mth %d \tSD, Mth %d",
			runSpecs->monthRecordARTEfficacy[0], runSpecs->monthRecordARTEfficacy[0],
			runSpecs->monthRecordARTEfficacy[1], runSpecs->monthRecordARTEfficacy[1],
			runSpecs->monthRecordARTEfficacy[2], runSpecs->monthRecordARTEfficacy[2]);
		fprintf(statsFile, "\n\tHVL Drops: \t%1.4lf \t%1.4lf \t%1.4lf \t%1.4lf \t%1.4lf \t%1.4lf",
			artStats.HVLDropsAtMonthMean[j][0], artStats.HVLDropsAtMonthStdDev[j][0],
			artStats.HVLDropsAtMonthMean[j][1], artStats.HVLDropsAtMonthStdDev[j][1],
			artStats.HVLDropsAtMonthMean[j][2], artStats.HVLDropsAtMonthStdDev[j][2]);

		// print out initial art patient distributions and toxicities
		fprintf(statsFile,"\n\tPat Distrib at Init:");
		for ( i = SimContext::HVL_NUM_STRATA - 1; i >= 0; --i )
			fprintf(statsFile," \t%s", SimContext::HVL_STRATA_STRS[i]);
		for ( i = SimContext::CD4_NUM_STRATA - 1; i >= 0; --i ) {
			fprintf(statsFile,"\n\t%s", SimContext::CD4_STRATA_STRS[i]);
			for ( k = SimContext::HVL_NUM_STRATA - 1; k >= 0; --k )
				fprintf(statsFile, " \t%1lu", artStats.distributionAtInit[j][i][k]);
		}
		fprintf(statsFile, "\n\tMinor Tox Cases:");
		for ( k = SimContext::HVL_NUM_STRATA - 1; k >= 0; --k )
			fprintf(statsFile, " \t%1lu", artStats.numToxicityCases[j][SimContext::ART_TOX_MINOR][k]);
		fprintf(statsFile, "\n\tChronic Tox Cases:");
		for ( k = SimContext::HVL_NUM_STRATA - 1; k >= 0; --k )
			fprintf(statsFile, " \t%1lu", artStats.numToxicityCases[j][SimContext::ART_TOX_CHRONIC][k]);
		fprintf(statsFile, "\n\tMajor Tox Cases:");
		for ( k = SimContext::HVL_NUM_STRATA - 1; k >= 0; --k )
			fprintf(statsFile, " \t%1lu", artStats.numToxicityCases[j][SimContext::ART_TOX_MAJOR][k]);
		fprintf(statsFile, "\n\tDeath Tox Cases:");
		for ( k = SimContext::HVL_NUM_STRATA - 1; k >= 0; --k )
			fprintf(statsFile, " \t%1lu", artStats.numToxicityDeaths[j][k]);

		// print out ART STI stats
		fprintf(statsFile, "\n\t");
		for ( k = 1; k <= SimContext::STI_NUM_TRACKED; ++k )
			fprintf(statsFile, "\tCycle %d", k);
		fprintf(statsFile, "+\t\t");
		for ( k = 1; k <= SimContext::STI_NUM_TRACKED; ++k )
			fprintf(statsFile, "\tCycle %d", k);
		fprintf(statsFile, "+\t\t");
		for ( k = 1; k <= SimContext::STI_NUM_TRACKED; ++k )
			fprintf(statsFile, "\tCycle %d", k);
		fprintf(statsFile, "+");
		fprintf(statsFile, "\n\tInterruptions:");
		for ( k = 0; k < SimContext::STI_NUM_TRACKED; ++k )
			fprintf(statsFile, "\t%lu", artStats.numSTIInterruptions[j][k]);
		fprintf(statsFile, "\t\tRestarts:");
		for ( k = 0; k < SimContext::STI_NUM_TRACKED; ++k )
			fprintf(statsFile, "\t%lu", artStats.numSTIRestarts[j][k]);
		fprintf(statsFile, "\t\tSTI Endpoint:");
		for ( k = 0; k < SimContext::STI_NUM_TRACKED; ++k )
			fprintf(statsFile, "\t%lu", artStats.numSTIEndpoints[j][k]);
		fprintf(statsFile,"\n\t");
		for ( k = 0; k < SimContext::STI_NUM_TRACKED; k++ ) {
			fprintf(statsFile, "\t%d Interrupts", k+1);
		}
		fprintf(statsFile, "+\n\t#People who have had n interrupts");
		for ( k = 0; k < SimContext::STI_NUM_TRACKED; ++k )
			fprintf(statsFile, "\t%u", artStats.numPatientsWithSTIInterruptions[j][k]);
		fprintf(statsFile, "\n\tMean Interruptions:");
		fprintf(statsFile, "\t%lf", artStats.numSTIInterruptionsMean[j]);
		fprintf(statsFile, "\n\tMean Interruption Duration:");
		fprintf(statsFile, "\t%lf", artStats.monthsOnSTIInterruptionMean[j]);
	}
} /* end writeARTStats */

/* writeTimeSummaries outputs the vector of TimeSummary objects statistics to the stats file */
void RunStats::writeTimeSummaries() {
	int j, k;
	const SimContext::RunSpecsInputs *runSpecs = simContext->getRunSpecsInputs();

	if (runSpecs->longitLoggingLevel == SimContext::LONGIT_SUMM_NONE)
		return;

	for (vector<TimeSummary *>::iterator t = timeSummaries.begin(); t != timeSummaries.end(); t++) {
		TimeSummary *currTime = *t;

		if (runSpecs->longitLoggingLevel == SimContext::LONGIT_SUMM_YR_DET)
			fprintf(statsFile,"\nCOHORT SUMMARY FOR YEAR %d END", currTime->timePeriod);
		else
			fprintf(statsFile,"\nCOHORT SUMMARY FOR MONTH %d", currTime->timePeriod);

		// output number alive by hiv state and total QOL applied
		fprintf(statsFile,"\n\t");
		for (j = 0; j < SimContext::HIV_ID_NUM; ++j)
			fprintf(statsFile, "\t%s", SimContext::HIV_ID_STRS[j]);
		fprintf(statsFile,"\tTotal\t");
		for (j = 0; j < SimContext::PEDS_HIV_NUM; ++j)
			fprintf(statsFile, "\t%s", SimContext::PEDS_HIV_STATE_STRS[j]);
		fprintf(statsFile, "\t\tIncident HIV+");
		if (runSpecs->longitLoggingLevel == SimContext::LONGIT_SUMM_MTH_DET){
			fprintf(statsFile,"\t\tTotal QOL applied");
		}
		fprintf(statsFile,"\n\t# Alive:");
		for (j = 0; j < SimContext::HIV_ID_NUM; ++j) {
			fprintf(statsFile, "\t%1lu", currTime->numAliveType[j]);
		}
		fprintf(statsFile, "\t%1lu\t", currTime->numAlive);
		for (j = 0; j < SimContext::PEDS_HIV_NUM; ++j)
			fprintf(statsFile, "\t%1lu", currTime->numAlivePediatrics[j]);
		fprintf(statsFile, "\t\t%1lu", currTime->numIncidentHIVInfections);
		if (runSpecs->longitLoggingLevel == SimContext::LONGIT_SUMM_MTH_DET){
			fprintf(statsFile, "\t\t%1.2f", currTime->sumQOLmultipliers);
		}

		// output true and observed CD4 and HVL
		if (runSpecs->longitLoggingLevel == SimContext::LONGIT_SUMM_MTH_BRF) {
			fprintf(statsFile,"\t\tMeanTrueCD4 (/infd): \t%1.0lf", currTime->trueCD4Mean );
			fprintf(statsFile,"\t\tMeanTrueHVL (/infd): \t%1.0lf", currTime->trueHVLMean );
		} else {
			fprintf(statsFile,"\n\tMean True CD4 (/infd):\t%1.0lf\t(%1.0lf SD)",
				currTime->trueCD4Mean, currTime->trueCD4StdDev );
			fprintf(statsFile,"\t\tMean Obsv CD4 (/infd): \t%1.0lf \t(%1.0lf SD)",
				currTime->observedCD4Mean, currTime->observedCD4StdDev );
			fprintf(statsFile,"\t\tMean CD4 Perc (/infd): \t%1.2lf \t(%1.2lf SD)",
				currTime->trueCD4PercentageMean, currTime->trueCD4PercentageStdDev );
			fprintf(statsFile,"\n\tMean True HVL (/infd): \t%1.0lf \t(%1.0lf SD)",
				currTime->trueHVLMean, currTime->trueHVLStdDev );
			fprintf(statsFile,"\t\tMean Obsv HVL (/infd): \t%1.0lf \t(%1.0lf SD)",
				currTime->observedHVLMean, currTime->observedHVLStdDev );
		}

		if (runSpecs->longitLoggingLevel != SimContext::LONGIT_SUMM_MTH_BRF) {
			// output CD4 and HVL distribs
			fprintf(statsFile,"\n\tCD4 Strata Distrib");
			for ( j = 0; j < SimContext::CD4_NUM_STRATA; ++j )
				fprintf(statsFile," \t%s", SimContext::CD4_STRATA_STRS[j]);
			fprintf(statsFile,"\n\tObsv CD4:");
			for ( j = 0; j < SimContext::CD4_NUM_STRATA; ++j )
				fprintf(statsFile," \t%1lu", currTime->observedCD4Distribution[j]);
			fprintf(statsFile,"\n\tTrue CD4/HVL Strata Distrib");
			for ( j = 0; j < SimContext::HVL_NUM_STRATA; ++j )
				fprintf(statsFile," \t%s", SimContext::HVL_STRATA_STRS[j]);
			fprintf(statsFile," \tTotal");
			for ( j = 0; j < SimContext::CD4_NUM_STRATA; ++j ) {
				fprintf(statsFile,"\n\tOffART:%s", SimContext::CD4_STRATA_STRS[j]);
				for ( k = 0; k < SimContext::HVL_NUM_STRATA; ++k ) {
					fprintf(statsFile," \t%1lu", currTime->trueCD4HVLARTDistribution[SimContext::ART_OFF_STATE][j][k] );
				}
				fprintf(statsFile," \t%1lu", currTime->trueCD4ARTDistribution[SimContext::ART_OFF_STATE][j]);
			}
			for ( j = 0; j < SimContext::CD4_NUM_STRATA; ++j ) {
				fprintf(statsFile,"\n\tOnART:%s", SimContext::CD4_STRATA_STRS[j]);
				for ( k = 0; k < SimContext::HVL_NUM_STRATA; ++k ) {
					fprintf(statsFile," \t%1lu", currTime->trueCD4HVLARTDistribution[SimContext::ART_ON_STATE][j][k] );
				}
				fprintf(statsFile," \t%1lu", currTime->trueCD4ARTDistribution[SimContext::ART_ON_STATE][j]);
				switch (j) {
					case 0:
						fprintf(statsFile, " \t\tSuppressed State on ART");
						break;
					case 1:
						fprintf(statsFile, " \t\tSuppressed \t%1lu",
							currTime->numARTEfficacyState[SimContext::ART_EFF_SUCCESS]);
						break;
					case 2:
						fprintf(statsFile, " \t\tPartial Supp \t%1lu",
							currTime->numARTEfficacyState[SimContext::ART_EFF_PARTIAL]);
						break;
					case 3:
						fprintf(statsFile, " \t\tFailure \t%1lu",
							currTime->numARTEfficacyState[SimContext::ART_EFF_FAILURE]);
						break;
				}
			}
			fprintf(statsFile,"\n\tTotal by True HVL");
			for ( k = 0; k < SimContext::HVL_NUM_STRATA; ++k ) {
				fprintf(statsFile," \t%1lu", currTime->trueHVLDistribution[k]);
			}
			fprintf(statsFile,"\n\tObsv HVL Strata Distrib");
			for ( j = 0; j < SimContext::HVL_NUM_STRATA; ++j )
				fprintf(statsFile," \t%1lu", currTime->observedHVLDistribution[j]);
		}

		// output OI distribs for only this month (not cumulative)
		fprintf(statsFile,"\n\tOIs Distrib");
		for ( j = 0; j < SimContext::OI_NUM; ++j )
			fprintf(statsFile," \t%s", SimContext::OI_STRS[j]);
		if (runSpecs->longitLoggingLevel == SimContext::LONGIT_SUMM_MTH_BRF) {
			fprintf(statsFile,"\n\tTot OI evts:");
			for ( j = 0; j < SimContext::OI_NUM; ++j )
				fprintf(statsFile," \t%1lu", currTime->numPrimaryOIs[j] + currTime->numSecondaryOIs[j]);
		}
		else {
			fprintf(statsFile,"\n\tPrim OI evts:");
			for ( j = 0; j < SimContext::OI_NUM; ++j )
				fprintf(statsFile," \t%1lu", currTime->numPrimaryOIs[j]);
			fprintf(statsFile,"\n\tSec OI evts:");
			for ( j = 0; j < SimContext::OI_NUM; ++j )
				fprintf(statsFile," \t%1lu", currTime->numSecondaryOIs[j]);
		}

		// output #patients with OI hists (is cumulative), and # w/o hist of any OI
		fprintf(statsFile,"\n\t# w/ OIhist:");
		for ( j = 0; j < SimContext::OI_NUM; ++j )
			fprintf(statsFile," \t%1lu", currTime->numWithOIHistory[j]);
		fprintf(statsFile,"\t\t# None OI hist:\t%1lu", currTime->numWithoutOIHistory);
		if (runSpecs->longitLoggingLevel == SimContext::LONGIT_SUMM_MTH_BRF)
			continue;

		// output #patients with "first" OI during this period
		fprintf(statsFile,"\n\t# w/ first OI:");
		for ( j = 0; j < SimContext::OI_NUM; ++j )
			fprintf(statsFile," \t%1lu", currTime->numWithFirstOI[j]);
		fprintf(statsFile,"\n\tDths from first OI:");
		for ( j = 0; j < SimContext::OI_NUM; ++j )
			fprintf(statsFile," \t%1lu", currTime->numDeathsFromFirstOI[j]);

		// output dth distribs for only this month (not cumulative)
		fprintf(statsFile,"\n\tDths Distrib");
		for ( j = 0; j < SimContext::DTH_NUM_CAUSES; ++j )
			fprintf(statsFile," \t%s", SimContext::DTH_CAUSES_STRS[j]);
		fprintf(statsFile,"\n\tDth evts:");
		for ( j = 0; j < SimContext::DTH_NUM_CAUSES; ++j )
			fprintf(statsFile," \t%1lu", currTime->numDeathsType[j]);
		fprintf(statsFile,"\n\tDth events while LTFU:");
		for ( j = 0; j < SimContext::DTH_NUM_CAUSES; ++j )
			fprintf(statsFile," \t%1lu", currTime->numDeathsType[j]);

		// output cost distribs for only this month (not cumulative)
		fprintf(statsFile,"\n\t\tCD4\tHVL\tClinic\tHIVtests\tHIVmisc");
		fprintf(statsFile,"\n\tTesting costs: \t%1.0lf \t%1.0lf \t%1.0lf \t%1.0lf \t%1.0lf",
			currTime->costsCD4Testing, currTime->costsHVLTesting, currTime->costsClinicVisits,
			currTime->costsHIVTests, currTime->costsHIVMisc);
		fprintf(statsFile,"\n\tTotal Cohort Mth costs: \t%1.2lf", currTime->totalMonthlyCohortCosts);
		fprintf(statsFile,"\n\tProph Costs");
		for (j = 0; j < SimContext::OI_NUM; ++j)
			fprintf(statsFile," \t%s", SimContext::OI_STRS[j]);
		for (j = 0; j < SimContext::PROPH_NUM; ++j) {
			fprintf(statsFile,"\n\tProph %d", j + 1);
			for (k = 0; k < SimContext::OI_NUM; ++k)
				fprintf(statsFile," \t%1.0lf", currTime->costsProph[k][j]);
		}
		fprintf(statsFile,"\n\t");
		for (j = 0; j < SimContext::ART_NUM_LINES; ++j)
			fprintf(statsFile, " \tART %d", j + 1);
		fprintf(statsFile, " \tPre-ART \tPost-ART (Off ART)");
		fprintf(statsFile,"\n\tART Costs:");
		for (j = 0; j < SimContext::ART_NUM_LINES; ++j)
			fprintf(statsFile," \t%1.0lf", currTime->costsART[j]);
		fprintf(statsFile,"\n\tNum on ART:");
		for (j = 0; j < SimContext::ART_NUM_LINES; ++j)
			fprintf(statsFile," \t%lu", currTime->numOnART[j]);
		fprintf(statsFile,"\n\tOn ART (InclContinCosts):");
		for (j = 0; j < SimContext::ART_NUM_LINES; ++j)
			fprintf(statsFile," \t%lu", currTime->numOnARTIncludingContinuedCosts[j]);
		fprintf(statsFile,"\n\tNum LTFU:");
		for (j = 0; j < SimContext::ART_NUM_LINES; ++j)
			fprintf(statsFile," \t%lu", currTime->numLostToFollowUpART[j]);
		fprintf(statsFile," \t%lu", currTime->numLostToFollowUpPreART);
		fprintf(statsFile," \t%lu", currTime->numLostToFollowUpPostART);
		fprintf(statsFile,"\n\tNum RTC (continue previous regimen):");
		for (j = 0; j < SimContext::ART_NUM_LINES; ++j)
			fprintf(statsFile," \t%lu", currTime->numReturnOnPrevART[j]);
		fprintf(statsFile," \t%lu", currTime->numReturnToCarePreART);
		fprintf(statsFile," \t%lu", currTime->numReturnToCarePostART);
		fprintf(statsFile,"\n\tNum RTC (switch regimen):");
		for (j = 0; j < SimContext::ART_NUM_LINES; ++j)
			fprintf(statsFile," \t%lu", currTime->numReturnOnNextART[j]);
		fprintf(statsFile,"\n\tNum Deaths while Lost:");
		for (j = 0; j < SimContext::ART_NUM_LINES; ++j)
			fprintf(statsFile," \t%lu", currTime->numDeathsWhileLostART[j]);
		fprintf(statsFile," \t%lu", currTime->numDeathsWhileLostPreART);
		fprintf(statsFile," \t%lu", currTime->numDeathsWhileLostPostART);
	}
} /* end writeTimeSummaries */


