#include "include.h"

/* Constructor takes run name as parameter */
SimContext::SimContext(string runName) {
	inputFileName = runName;
	inputFileName.append(".in");
	runSpecsInputs.runName = runName;
}

/* Destructor cleans up the allocated memory for art and proph inputs */
SimContext::~SimContext(void) {
	for (int i = 0; i < ART_NUM_LINES; i++) {
		delete artInputs[i];
	}
	for (int i = 0; i < PROPH_NUM_TYPES; i++) {
		for (int j = 0; j < OI_NUM; j++) {
			for (int k = 0; k < PROPH_NUM; k++) {
				delete prophsInputs[i][j][k];
			}
		}
	}
}

/* Initialize the number of patients to trace to 50 */
int SimContext::numPatientsToTrace = 50;

/* Initialize the constant character strings */
const char *SimContext::CD4_STRATA_STRS[] = {
	"CD4vlo", "CD4_lo", "CD4mlo", "CD4mhi", "CD4_hi", "CD4vhi", "CD4unk"
};
const char *SimContext::HVL_STRATA_STRS[] = {
	"HVLvlo", "HVL_lo", "HVLmlo", "HVLmed", "HVLmhi", "HVL_hi", "HVLvhi", "HVLunk"
};
const double SimContext::HVL_STRATA_MIDPTS[] = {
	10.0, 250.0, 1750.0, 6500.0, 20000.0, 65000.0, 550000.0
};
const char *SimContext::GENDER_STRS[] = {
	"male", "female"
};
char SimContext::OI_STRS[SimContext::OI_NUM][32];	// OI_STRS is set during readRunSpecsInputs
char SimContext::DTH_CAUSES_STRS[SimContext::DTH_NUM_CAUSES][32]; 	// OI_STRS is set during readRunSpecsInputs and readCHRMsInputs
const char *SimContext::HIST_OI_CATS_STRS[] = {
	"NoOIHist", "MildOIHist", "SevrOIHist"
};
char SimContext::CHRM_STRS[SimContext::CHRM_NUM][32];	// CHRM_STRS is set during readCHRMsInputs
const char *SimContext::CLINIC_VISITS_STRS[] = {
	"initial", "acute", "sched"
};
const char *SimContext::ART_EFF_STRS[] = {
	"suppressed", "partial_suppressed", "failure"
};
const char *SimContext::CD4_RESPONSE_STRS[] = {
	"Type_1", "Type_2", "Type_3", "Type_4"
};
const char *SimContext::RESP_TYPE_STRS[] = {
	"Full Responder", "Partial Responder", "Non Responder"
};
const char *SimContext::ART_TOX_SEVERITY_STRS[] = {
	"Min","Chr","Maj","Dth"
};
const char *SimContext::ART_FAIL_TYPE_STRS[] = {
	"Virologic", "Immunologic", "Clinical", "Not Failed"
};
const char *SimContext::ART_STOP_TYPE_STRS[] = {
	"Max Months on ART", "With Major Toxicity", "On Observed Failure", "Fail and CD4", "Fail and Severe OI",
	"Fail and Max Months", "LTFU", "Not Stopped", "STI"
};
const char *SimContext::PROPH_TYPE_STRS[] = {
	"PRIMARY", "SECONDARY"
};
const char *SimContext::TB_STRAIN_STRS[] = {
	"dsTB", "mdrTB", "xdrTB"
};
const char *SimContext::TB_STATE_STRS[] = {
	"latent", "active", "treatment(succeeding)","historyActive", "treatment(failing)"
};
const char *SimContext::TB_TREATM_STAGE_STRS[] = {
	"L1", "L1rpt", "L2", "L3"
};
const char *SimContext::HIV_ID_STRS[] = {
	"HIVneg", "unidentHIV+", "identHIV+"
};
const char *SimContext::HIV_INF_STRS[] = {
	"HIVneg", "HIVasym", "HIVsymp", "HIVacut"
};
const char *SimContext::HIV_EXT_INF_STRS[] =  {
	"HIVneg_hiRisk", "HIVasym", "HIVsymp", "HIVacut", "HIVneg_loRisk"
};
const char *SimContext::HIV_DET_STRS[] = {
	"initial", "HIVscreening", "HIVbackground", "presentingOI", "unidentified"
};
const char *SimContext::TEST_RESULT_STRS[] = {
	"truePos", "falsPos", "trueNeg", "falsNeg"
};
const char *SimContext::PEDS_HIV_STATE_STRS[] = {
	"HIVposIU", "HIVposIP", "HIVposPP", "HIVneg"
};
const char *SimContext::PEDS_MOM_HIV_STRS[] = {
	"AIDSonART", "AIDSinCare", "AIDSnoCare", "nonAIDSonART", "nonAIDSinCare", "nonAIDSnoCare",
	"AcuteHIV", "HIVneg"
};
const char *SimContext::PEDS_AGE_CAT_STRS[] = {
	"0-2mth", "3-5mth", "6-8mth", "9-11mth", "12-14mth", "15-17mth", "18-23mth", "2yr", "3yr", "4yr", ">4yr"
};
const char *SimContext::PEDS_CD4_PERC_STRS[] = {
	"0-5perc", "5-10perc", "10-15perc", "15-20perc", "20-25perc", "25-30perc", "30-35perc", ">35perc"
};

/* readInputs function reads in all the inputs from the given input file,
	throws exception if there is an error */
void SimContext::readInputs() {
	/* Open the input file for reading */
	CepacUtil::changeDirectoryToInputs();
	inputFile = CepacUtil::openFile(inputFileName.c_str(), "r");
	if (inputFile == NULL) {
		string errorString = "   ERROR - Could not open input file ";
		errorString.append(inputFileName);
		throw errorString;
	}

	/* Read all the input data from the file */
	readRunSpecsInputs();
	readCohortInputs();
	readLTFUInputs();
	readHeterogeneityInputs();
	readHIVTestInputs();
	readNatHistInputs();
	readCHRMsInputs();
	readQOLInputs();
	readCostInputs();
	readTreatmentInputsPart1();
	readARTInputs();
	readTreatmentInputsPart2();
	readProphInputs();
	readSTIInputs();
	readTBInputs();
	readPedsInputs();
	readPedsARTInputs();

	/* Close the input file */
	CepacUtil::closeFile(inputFile);
} /* end readInputs */

/* readRunSpecsInputs reads data from the RunSpecs tab of the input sheet */
void SimContext::readRunSpecsInputs() {
	char buffer[256];
	int i, tempBool;

	// read in name of set this run belongs to
	readAndSkipPast( "Runset", inputFile );
	fscanf( inputFile, "%299s", buffer );
	runSpecsInputs.runSetName = buffer;
	// read in cohort size
	readAndSkipPast( "CohortSize", inputFile );
	fscanf( inputFile, "%ld", &runSpecsInputs.numCohorts );
	// read in discount rate, convert to monthly rate from yearly
	readAndSkipPast( "DiscFactor", inputFile );
	fscanf( inputFile, "%lf", &runSpecsInputs.discountFactor );
	runSpecsInputs.discountFactor = pow(1.0 + runSpecsInputs.discountFactor, 1.0 / 12.0);
	// read in max actual CD4 count for patient
	readAndSkipPast( "MaxPatCD4", inputFile );
	fscanf( inputFile, "%lf", &runSpecsInputs.maxPatientCD4 );
	// read in whether to enable ART CD4 envelope
	readAndSkipPast( "EnableARTCD4Env", inputFile );
	fscanf( inputFile, "%d", &runSpecsInputs.enableARTCD4Envelope );

	// read in mth times A - C to rec ART eff
	readAndSkipPast( "MthRecARTEffA", inputFile );
	fscanf( inputFile, "%d", &runSpecsInputs.monthRecordARTEfficacy[0] );
	readAndSkipPast( "MthRecARTEffB", inputFile );
	fscanf( inputFile, "%d", &runSpecsInputs.monthRecordARTEfficacy[1] );
	readAndSkipPast( "MthRecARTEffC", inputFile );
	fscanf( inputFile, "%d", &runSpecsInputs.monthRecordARTEfficacy[2] );
	// read in whether to use time to init rand seeds
	readAndSkipPast( "RandSeedByTime", inputFile );
	fscanf( inputFile, "%d", &tempBool);
	runSpecsInputs.randomSeedByTime = (bool) tempBool;
	// read in user locale
	readAndSkipPast( "UserLocale", inputFile );
	fscanf( inputFile, " %32s", buffer );
	runSpecsInputs.userProgramLocale = buffer;

	// read in input's internal version
	readAndSkipPast( "InpVer", inputFile );
	fscanf( inputFile, " %20s", buffer );
	// Verify input version, throw error if it does not match up
	if (strcmp(buffer, CepacUtil::CEPAC_INPUT_VERSION) != 0) {
		string errorString = "   ERROR - Input file version incompatible with the CEPAC executable";
		throw errorString;
	}
	runSpecsInputs.inputVersion = buffer;
	// read in the model version
	readAndSkipPast( "ModelVer", inputFile );
	fscanf( inputFile, " %20s", buffer );
	runSpecsInputs.modelVersion = buffer;

	// read in user defined OI names, and set causes of death names
	readAndSkipPast( "OIstrs", inputFile );
	for ( i = 0; i < OI_NUM; ++i ) {
		fscanf( inputFile, " %32s", OI_STRS[i] );
		strcpy(DTH_CAUSES_STRS[i], OI_STRS[i]);
	}
	strcpy(DTH_CAUSES_STRS[DTH_CHRAIDS], "chrAIDS");
	strcpy(DTH_CAUSES_STRS[DTH_NONAIDS], "nonAIDS");
	strcpy(DTH_CAUSES_STRS[DTH_TOX_ART], "toxART");
	strcpy(DTH_CAUSES_STRS[DTH_TOX_PROPH], "toxProph");
	strcpy(DTH_CAUSES_STRS[DTH_TOX_TB_PROPH], "toxTBProph");
	strcpy(DTH_CAUSES_STRS[DTH_TOX_TB_TREATM], "toxTBTreatment");

	// read in whether to output monthly cohort summaries to file
	readAndSkipPast( "LongitLogCohort", inputFile );
	fscanf( inputFile, "%d", &runSpecsInputs.longitLoggingLevel );
	// read in OIs considered as first OIs in the log
	readAndSkipPast( "LongitLogFirstOIs", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%d", &(runSpecsInputs.firstOIsLongitLogging[i]) );
	fscanf( inputFile, "%d", &runSpecsInputs.firstOIsChronicLongitLogging );

	// read in whether to output CD4 distribution of OI histories
	readAndSkipPast( "LogPriorOIHistProb", inputFile );
	fscanf( inputFile, "%d", &tempBool);
	runSpecsInputs.enableOIHistoryLogging = (bool) tempBool;
	// read in the number of ART failures pat has to log OI hists
	readAndSkipPast( "LogOIHistwithARTfails", inputFile );
	fscanf( inputFile, "%d", &runSpecsInputs.numARTFailuresForOIHistoryLogging );
	// read in CD4 bounds to constrain when pat mths are included in OI hist logging
	readAndSkipPast( "LogOIHistwithCD4", inputFile );
	fscanf( inputFile, "%lf %lf", &(runSpecsInputs.CD4BoundsForOIHistoryLogging[LOWER_BOUND]), &(runSpecsInputs.CD4BoundsForOIHistoryLogging[UPPER_BOUND]) );
	// read in HVL bnds to constrain when pat mths included in OI hist logging
	readAndSkipPast( "LogOIHistwithHVL", inputFile );
	fscanf( inputFile, "%d %d", &(runSpecsInputs.HVLBoundsForOIHistoryLogging[LOWER_BOUND]), &(runSpecsInputs.HVLBoundsForOIHistoryLogging[UPPER_BOUND]) );
	// read in whether to excl pat mths from OI hist log if there's OI hist of given OI
	readAndSkipPast( "LogOIHistwithOIs", inputFile );
	for ( i = 0; i < OI_NUM; ++i ) {
		fscanf( inputFile, "%d", &tempBool);
		tempBool = (bool) runSpecsInputs.OIsToExcludeOIHistoryLogging[i];
	}

	// read in OI Fraction of Benefit
	readAndSkipPast( "FOB_OIs", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%lf", &(runSpecsInputs.OIsFractionOfBenefit[i]) );
	// read in dth FOBs
	readAndSkipPast( "FOB_Dth", inputFile );
	for ( i = 0; i < DTH_NUM_CAUSES_BASIC; ++i )
		fscanf( inputFile, "%lf", &(runSpecsInputs.deathFractionOfBenefit[i]) );
	// read in severe OI classification
	readAndSkipPast( "Severe_OIs", inputFile );
	for ( i = 0; i < OI_NUM; ++i ) {
		fscanf( inputFile, "%d", &tempBool);
		runSpecsInputs.severeOIs[i] = (bool) tempBool;
	}
	// read in CD4 count boundaries for strata
	readAndSkipPast( "CD4Bounds", inputFile );
	for ( i = CD4__HI; i >= CD4_VLO; i-- )
		fscanf( inputFile, "%lf", &(runSpecsInputs.CD4StrataUpperBounds[i]) );
} /* end readRunSpecsInputs */

/* readCohortInputs reads data from the Cohort tab of the input sheet */
void SimContext::readCohortInputs() {
	int i, j, k;
	double dTemp;

	// read in popul initial CD4 distrib
	readAndSkipPast( "InitCD4", inputFile );
	fscanf(inputFile,"%lf %lf", &(cohortInputs.initialCD4Mean), &(cohortInputs.initialCD4StdDev));
	// read in popul initial HVL distrib
	for ( j = CD4_NUM_STRATA - 1; j >= 0; --j ) {
		readAndSkipPast( "InitHVL", inputFile );
		readAndSkipPast( CD4_STRATA_STRS[j], inputFile );
		for ( i = HVL_NUM_STRATA - 1; i >= HVL_VLO; --i )
			fscanf( inputFile, "%lf", &(cohortInputs.initialHVLDistribution[j][i]) );
	}
	// read in popul initial age (mths) distrib
	readAndSkipPast( "InitAge", inputFile );
	fscanf(inputFile,"%lf %lf", &cohortInputs.initialAgeMean, &cohortInputs.initialAgeStdDev);
	// read in male percentage of cohort
	readAndSkipPast( "InitGender", inputFile );
	fscanf( inputFile, "%lf", &cohortInputs.maleGenderDistribution );
	// read in OI proph noncompliance prob and degree
	readAndSkipPast( "ProphNonCompliance", inputFile );
	fscanf( inputFile, "%lf %lf", &cohortInputs.OIProphNonComplianceRisk, &cohortInputs.OIProphNonComplianceDegree );

	// read in distribution of clinic visit patient types
	readAndSkipPast( "PatClinicTypes", inputFile );
	dTemp = 1.0;
	for ( i = 0; i < CLINIC_VISITS_NUM - 1; ++i ) {
		fscanf( inputFile, "%lf", &(cohortInputs.clinicVisitTypeDistribution[i]) );
		dTemp -= cohortInputs.clinicVisitTypeDistribution[i];
	}
	cohortInputs.clinicVisitTypeDistribution[CLINIC_VISITS_NUM - 1] = dTemp;
	// read in distribution of proph and ART implement patient types
	readAndSkipPast( "PatTreatmentTypes", inputFile );
	dTemp = 1.0;
	for ( i = 0; i < THERAPY_IMPL_NUM - 1; ++i ) {
		fscanf( inputFile, "%lf", &(cohortInputs.therapyImplementationDistribution[i]) );
		dTemp -= cohortInputs.therapyImplementationDistribution[i];
	}
	cohortInputs.therapyImplementationDistribution[THERAPY_IMPL_NUM - 1] = dTemp;
	// read in distribution of CD4 response types on ART
	readAndSkipPast("PatCD4ResponeTypeOnART", inputFile);
	dTemp = 1.0;
	for (i = 0; i < CD4_RESPONSE_NUM_TYPES - 1; i++) {
		fscanf(inputFile, "%lf", &(cohortInputs.CD4ResponseTypeOnARTDistribution[i]));
		dTemp -= cohortInputs.CD4ResponseTypeOnARTDistribution[i];
	}
	cohortInputs.CD4ResponseTypeOnARTDistribution[CD4_RESPONSE_NUM_TYPES - 1] = dTemp;

	// read in popul prob of prev OI histories
	readAndSkipPast( "PriorOIHistAtEntry", inputFile );
	for ( i = 0; i < OI_NUM; ++i ) {
		readAndSkipPast( OI_STRS[i], inputFile );
		for ( k = HVL_NUM_STRATA - 1; k >= 0; --k ) {
			readAndSkipPast( HVL_STRATA_STRS[k], inputFile );
			for ( j = CD4_NUM_STRATA - 1; j >= 0; --j )
				fscanf( inputFile, "%lf", &(cohortInputs.probOIHistoryAtEntry[j][k][i]) );
		}
	}

	// read in prevalence and incidence of generic risk factors
	readAndSkipPast("ProbRiskFactorPrev", inputFile);
	for (int i = 0; i < RISK_FACT_NUM; i++) {
		fscanf(inputFile, "%lf ", &(cohortInputs.probRiskFactorPrev[i]));
	}
	readAndSkipPast("ProbRiskFactorIncid", inputFile);
	for (int i = 0; i < RISK_FACT_NUM; i++) {
		fscanf(inputFile, "%lf ", &(cohortInputs.probRiskFactorIncid[i]));
	}
} /* end readCohortInputs */

/* readTreatmentInputsPart11 reads data from the Treatment tab of the input sheet,
	split in the middle for the reading of UserARTs */
void SimContext::readTreatmentInputsPart1() {
	int i, j, tempBool;

	// read in clinic visit interval
	readAndSkipPast( "IntvlClinicVisit", inputFile );
	fscanf( inputFile, "%d", &(treatmentInputs.clinicVisitInterval) );
	// read in probabilities of detecting patient's prior OI history
	readAndSkipPast( "ProbDetOI_Entry", inputFile );
	for ( j = 0; j < OI_NUM; ++j )
		fscanf( inputFile, "%lf", &(treatmentInputs.probDetectOIAtEntry[j]) );
	readAndSkipPast( "ProbDetOI_LastVst", inputFile );
	for ( j = 0; j < OI_NUM; ++j )
		fscanf( inputFile, "%lf", &(treatmentInputs.probDetectOISinceLastVisit[j]) );
	// read in probabilities of switching to secondary proph at OI event
	readAndSkipPast( "ProbSwitchSecProph", inputFile );
	for ( j = 0; j < OI_NUM; ++j )
		fscanf( inputFile, "%lf", &(treatmentInputs.probSwitchSecondaryProph[j]) );

	// read in CD4 and on ART months thresholds used for testing frequency
	readAndSkipPast( "IntvlCD4Tst_CD4Threshold", inputFile );
	fscanf( inputFile, "%lf", &treatmentInputs.testingIntervalCD4Threshold );
	readAndSkipPast("IntvlCD4Tst_MonthsThreshold", inputFile);
	fscanf( inputFile, "%d %d", &treatmentInputs.testingIntervalARTMonthsThreshold,
			&treatmentInputs.testingIntervalLastARTMonthsThreshold);
	// read in CD4/HVL test intervals
	readAndSkipPast( "IntvlCD4Tst", inputFile );
	fscanf( inputFile, "%d %d %d %d %d %d %d",
		&treatmentInputs.CD4TestingIntervalPreARTHighCD4, &treatmentInputs.CD4TestingIntervalPreARTLowCD4,
		&treatmentInputs.CD4TestingIntervalOnART[0], &treatmentInputs.CD4TestingIntervalOnART[1],
		&treatmentInputs.CD4TestingIntervalOnLastART[0], &treatmentInputs.CD4TestingIntervalOnLastART[1],
		&treatmentInputs.CD4TestingIntervalPostART);
	readAndSkipPast( "IntvlHVLTst", inputFile );
	fscanf( inputFile, "%d %d %d %d %d %d %d",
		&treatmentInputs.HVLTestingIntervalPreARTHighCD4, &treatmentInputs.HVLTestingIntervalPreARTLowCD4,
		&treatmentInputs.HVLTestingIntervalOnART[0], &treatmentInputs.HVLTestingIntervalOnART[1],
		&treatmentInputs.HVLTestingIntervalOnLastART[0], &treatmentInputs.HVLTestingIntervalOnLastART[1],
		&treatmentInputs.HVLTestingIntervalPostART );

	// read in HVL test err prob (lower & higher)
	readAndSkipPast( "HVLtestErrProb", inputFile );
	fscanf( inputFile, "%lf %lf", &treatmentInputs.probHVLTestErrorHigher, &treatmentInputs.probHVLTestErrorLower );
	// read in CD4 test err deviation
	readAndSkipPast( "CD4testErrSDev", inputFile );
	fscanf( inputFile, "%lf", &treatmentInputs.CD4TestStdDevPercentage );

	// read in whether to perform ART obsv fail CD4/HVL tests outside clinic visit
	readAndSkipPast( "ObsvARTFailTestOnRegClinicVst", inputFile );
	fscanf( inputFile, "%d", &tempBool);
	treatmentInputs.ARTFailureOnlyAtRegularVisit = (bool) tempBool;
	// read in numbers of CD4/HVL tests outside clinic visit at ART initiation
	readAndSkipPast( "ARTInitHVLTestsWOClinicVst", inputFile );
	fscanf( inputFile, "%d", &treatmentInputs.numARTInitialHVLTests );
	readAndSkipPast( "ARTInitCD4TestsWOClinicVst", inputFile );
	fscanf( inputFile, "%d", &treatmentInputs.numARTInitialCD4Tests );
	// read in if OI visits are not treated as regular clinic visits
	readAndSkipPast( "OIVstAsNotSchedClinicVst", inputFile );
	fscanf( inputFile, "%d", &tempBool);
	treatmentInputs.emergencyVisitIsNotRegularVisit = (bool) tempBool;

	//ART starting criteria
	// read in CD4 bounds
	readAndSkipPast2( "ARTstart_CD4", "upp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(treatmentInputs.startART[i].CD4BoundsOnly[UPPER_BOUND]) );
	readAndSkipPast2( "ARTstart_CD4", "lwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(treatmentInputs.startART[i].CD4BoundsOnly[LOWER_BOUND]) );
	// read in HVL bounds to administer ARTs
	readAndSkipPast2( "ARTstart_HVL", "upp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(treatmentInputs.startART[i].HVLBoundsOnly[UPPER_BOUND]) );
	readAndSkipPast2( "ARTstart_HVL", "lwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(treatmentInputs.startART[i].HVLBoundsOnly[LOWER_BOUND]) );
	// read in CD4 & HVL bounds to administer ARTs
	readAndSkipPast2( "ARTstart_CD4HVL", "CD4upp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(treatmentInputs.startART[i].CD4BoundsWithHVL[UPPER_BOUND]) );
	readAndSkipPast2( "ARTstart_CD4HVL", "CD4lwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(treatmentInputs.startART[i].CD4BoundsWithHVL[LOWER_BOUND]) );
	readAndSkipPast2( "ARTstart_CD4HVL", "HVLupp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(treatmentInputs.startART[i].HVLBoundsWithCD4[UPPER_BOUND]) );
	readAndSkipPast2( "ARTstart_CD4HVL", "HVLlwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(treatmentInputs.startART[i].HVLBoundsWithCD4[LOWER_BOUND]) );
	// read in OI criteria to administer ARTs
	for (j = 0; j < OI_NUM; ++j) {
		readAndSkipPast2( "ARTstart_OIs", OI_STRS[j], inputFile );
		for (i = 0; i < ART_NUM_LINES; ++i) {
			fscanf( inputFile, "%d", &tempBool);
			treatmentInputs.startART[i].OIHistory[j] = (bool) tempBool;
		}
	}
	readAndSkipPast2( "ARTstart_OIs", "numOIs", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(treatmentInputs.startART[i].numOIs) );
	// read in CD4 & OI criteria to administer ARTs
	readAndSkipPast2( "ARTstart_CD4OI", "CD4upp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(treatmentInputs.startART[i].CD4BoundsWithOIs[UPPER_BOUND]) );
	readAndSkipPast2( "ARTstart_CD4OI", "CD4lwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(treatmentInputs.startART[i].CD4BoundsWithOIs[LOWER_BOUND]) );
	for (j = 0; j < OI_NUM; ++j) {
		readAndSkipPast2( "ARTstart_CD4OI", OI_STRS[j], inputFile );
		for (i = 0; i < ART_NUM_LINES; ++i) {
			fscanf( inputFile, "%d", &tempBool);
			treatmentInputs.startART[i].OIHistoryWithCD4[j] = (bool) tempBool;
		}
	}
	// read in minimum mth # to start ART
	readAndSkipPast2( "ARTstart", "minMthNum", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(treatmentInputs.startART[i].minMonthNum) );
	readAndSkipPast2( "ARTstart", "MthsSincePrevRegStop", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(treatmentInputs.startART[i].monthsSincePrevRegimen) );
} /* endReadTreatmentInputsPart1 */

/* readTreatmentInputsPart2 reads data from the second half of the Treatment tab of the input sheet,
	split in the middle for the reading of UserARTs */
void SimContext::readTreatmentInputsPart2() {
	int i, j, tempBool;
	char buffer[256];

	// read in whether to enable interruption of ART
	readAndSkipPast( "EnableSTIforART", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i ) {
		fscanf( inputFile, "%d", &tempBool);
		treatmentInputs.enableSTIForART[i] = (bool) tempBool;
	}

	// ART Failure parameters
	// read in # HVL lvls to incr for fail diag
	readAndSkipPast( "ARTfail_hvlNumIncr", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.failART[i].HVLNumIncrease) );
	// read in absolute HVL counts for fail diag
	readAndSkipPast( "ARTfail_hvlAbsol", inputFile );
	readAndSkipPast( "uppBnd", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.failART[i].HVLBounds[UPPER_BOUND]) );
	readAndSkipPast( "ARTfail_hvlAbsol", inputFile );
	readAndSkipPast( "lwrBnd", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.failART[i].HVLBounds[LOWER_BOUND]) );
	// read in true/false use HVL as setpoint for fail diag
	readAndSkipPast( "ARTfail_hvlAtSetptAsFailDiag", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i ) {
		fscanf( inputFile, "%d", &tempBool);
		treatmentInputs.failART[i].HVLFailAtSetpoint = (bool) tempBool;
	}
	// read in # of months before using HVL criteria
	readAndSkipPast( "ARTfail_hvlMthsFromInit", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.failART[i].HVLMonthsFromInit) );
	// read in CD4 percentage to decr for fail diag
	readAndSkipPast( "ARTfail_cd4PercDrop", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.failART[i].CD4PercentageDrop) );
	// read in true/false use CD4 as below pre-ART nadir for fail diag
	readAndSkipPast( "ARTfail_cd4BelowPreARTNadir", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i ) {
		fscanf( inputFile, "%d", &tempBool);
		treatmentInputs.failART[i].CD4BelowPreARTNadir = (bool) tempBool;
	}
	// read in absolute CD4 counts as OR criteria for fail diag
	readAndSkipPast( "ARTfail_cd4AbsolOR", inputFile );
	readAndSkipPast( "uppBnd", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.failART[i].CD4BoundsOR[UPPER_BOUND]) );
	readAndSkipPast( "ARTfail_cd4AbsolOR", inputFile );
	readAndSkipPast( "lwrBnd", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.failART[i].CD4BoundsOR[LOWER_BOUND]) );
	// read in absolute CD4 counts as AND criteria for fail diag
	readAndSkipPast( "ARTfail_cd4AbsolAND", inputFile );
	readAndSkipPast( "uppBnd", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.failART[i].CD4BoundsAND[UPPER_BOUND]) );
	readAndSkipPast( "ARTfail_cd4AbsolAND", inputFile );
	readAndSkipPast( "lwrBnd", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.failART[i].CD4BoundsAND[LOWER_BOUND]) );
	// read in # of months before using CD4 criteria
	readAndSkipPast( "ARTfail_cd4MthsFromInit", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.failART[i].CD4MonthsFromInit) );
	// read in whether to treat OI event as ART fail diag
	for ( j = 0; j < OI_NUM; ++j ) {
		readAndSkipPast( "ARTfail_OIs", inputFile );
		readAndSkipPast( OI_STRS[j], inputFile );
		for ( i = 0; i < ART_NUM_LINES; ++i )
			fscanf( inputFile, "%d", &(treatmentInputs.failART[i].OIsEvent[j]) );
	}
	readAndSkipPast( "ARTfail_OIsMinNum", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.failART[i].OIsMinNum) );
	readAndSkipPast( "ARTfail_OIsMthsFromInit", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.failART[i].OIsMonthsFromInit) );
	// read in ART failure diagnoses criteria parameters
	readAndSkipPast( "ARTfail_diagNumTestsFail", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.failART[i].diagnoseNumTestsFail) );
	readAndSkipPast( "ARTfail_diagUseHVLTestsConfirm", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i ) {
		fscanf( inputFile, "%d", &tempBool);
		treatmentInputs.failART[i].diagnoseUseHVLTestsConfirm = (bool) tempBool;
	}
	readAndSkipPast( "ARTfail_diagUseCD4TestsConfirm", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i ) {
		fscanf( inputFile, "%d", &tempBool);
		treatmentInputs.failART[i].diagnoseUseCD4TestsConfirm = (bool) tempBool;
	}
	readAndSkipPast( "ARTfail_diagNumTestsConfirm", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.failART[i].diagnoseNumTestsConfirm) );

	//read in ART stopping policy
	// read in maximum number of months to be on ART
	readAndSkipPast( "ARTstop_MaxMthsOnART", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.stopART[i].maxMonthsOnART));
	// read in stop on major toxicity
	readAndSkipPast("ARTstop_MajorToxicity", inputFile);
	for (i = 0; i < ART_NUM_LINES; i++) {
		fscanf(inputFile, "%d", &tempBool);
		treatmentInputs.stopART[i].withMajorToxicty = (bool) tempBool;
	}
	// read in criteria to use after failure has been observed
	readAndSkipPast( "ARTstop_OnFailImmed", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i ) {
		fscanf( inputFile, "%d", &tempBool);
		treatmentInputs.stopART[i].afterFailImmediate = (bool) tempBool;
	}
	readAndSkipPast( "ARTstop_OnFailBelowCD4", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.stopART[i].afterFailCD4LowerBound));
	readAndSkipPast( "ARTstop_OnFailSevereOI", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i ) {
		fscanf( inputFile, "%d", &tempBool);
		treatmentInputs.stopART[i].afterFailWithSevereOI = (bool) tempBool;
	}
	readAndSkipPast( "ARTstop_OnFailMthsAfterObsv", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.stopART[i].afterFailMonthsFromObserved));
	// read in minimum month number to stop ART
	readAndSkipPast( "ARTstop_OnFailMinMthNum", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.stopART[i].afterFailMinMonthNum) );
	readAndSkipPast( "ARTstop_OnFailMthsFromInit", inputFile );
	for ( i = 0; i < ART_NUM_LINES; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.stopART[i].afterFailMonthsFromInit) );

	// read in ART resistance penalty parameters
	for (i = 0; i < ART_NUM_LINES; i++) {
		sprintf(buffer, "reg_%d", i + 1);
		readAndSkipPast2("ARTresistRed_initSucc", buffer, inputFile);
		for (j = 0; j < ART_NUM_LINES; j++) {
			fscanf(inputFile, "%lf", &(treatmentInputs.ARTResistancePriorRegimen[i][j]));
		}
	}
	readAndSkipPast( "ARTresistRed_HVL", inputFile );
	for (i = HVL_NUM_STRATA - 1; i >= 0; --i) {
		fscanf(inputFile, "%lf", &(treatmentInputs.ARTResistanceHVL[i]));
	}

	// read in primary OI proph regimen starting criteria
	readAndSkipPast( "PriProphStart", inputFile );
	readAndSkipPast( "boolFlag", inputFile );
	for ( i = 0; i < OI_NUM; ++i ) {
		fscanf( inputFile, "%d", &tempBool);
		treatmentInputs.startProph[PROPH_PRIMARY][i].useOrEvaluation = (bool) tempBool;
	}
	readAndSkipPast( "PriProphStart", inputFile );
	readAndSkipPast( "curCD4upp", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.startProph[PROPH_PRIMARY][i].currCD4Bounds[UPPER_BOUND]) );
	readAndSkipPast( "PriProphStart", inputFile );
	readAndSkipPast( "curCD4lwr", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.startProph[PROPH_PRIMARY][i].currCD4Bounds[LOWER_BOUND]) );
	readAndSkipPast( "PriProphStart", inputFile );
	readAndSkipPast( "minCD4upp", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.startProph[PROPH_PRIMARY][i].minCD4Bounds[UPPER_BOUND]) );
	readAndSkipPast( "PriProphStart", inputFile );
	readAndSkipPast( "minCD4lwr", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.startProph[PROPH_PRIMARY][i].minCD4Bounds[LOWER_BOUND]) );
	for ( j = 0; j < OI_NUM; ++j ) {
		readAndSkipPast( "PriProphStart", inputFile );
		readAndSkipPast( OI_STRS[j], inputFile );
		for ( i = 0; i < OI_NUM; ++i )
			fscanf( inputFile, "%d", &(treatmentInputs.startProph[PROPH_PRIMARY][i].OIHistory[j]) );
	}
	readAndSkipPast( "PriProphStart", inputFile );
	readAndSkipPast( "minMthNum", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.startProph[PROPH_PRIMARY][i].minMonthNum) );

	// read in primary OI proph regimen stopping criteria
	readAndSkipPast( "PriProphStop", inputFile );
	readAndSkipPast( "boolFlag", inputFile );
	for ( i = 0; i < OI_NUM; ++i ) {
		fscanf( inputFile, "%d", &tempBool);
		treatmentInputs.stopProph[PROPH_PRIMARY][i].useOrEvaluation = (bool) tempBool;
	}
	readAndSkipPast( "PriProphStop", inputFile );
	readAndSkipPast( "curCD4upp", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.stopProph[PROPH_PRIMARY][i].currCD4Bounds[UPPER_BOUND]) );
	readAndSkipPast( "PriProphStop", inputFile );
	readAndSkipPast( "curCD4lwr", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.stopProph[PROPH_PRIMARY][i].currCD4Bounds[LOWER_BOUND]) );
	readAndSkipPast( "PriProphStop", inputFile );
	readAndSkipPast( "minCD4upp", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.stopProph[PROPH_PRIMARY][i].minCD4Bounds[UPPER_BOUND]) );
	readAndSkipPast( "PriProphStop", inputFile );
	readAndSkipPast( "minCD4lwr", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.stopProph[PROPH_PRIMARY][i].minCD4Bounds[LOWER_BOUND]) );
	for ( j = 0; j < OI_NUM; ++j ) {
		readAndSkipPast( "PriProphStop", inputFile );
		readAndSkipPast( OI_STRS[j], inputFile );
		for ( i = 0; i < OI_NUM; ++i )
			fscanf( inputFile, "%d", &(treatmentInputs.stopProph[PROPH_PRIMARY][i].OIHistory[j]) );
	}
	readAndSkipPast( "PriProphStop", inputFile );
	readAndSkipPast( "minMthNum", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.stopProph[PROPH_PRIMARY][i].minMonthNum) );
	readAndSkipPast( "PriProphStop", inputFile );
	readAndSkipPast( "mthsOnProph", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.stopProph[PROPH_PRIMARY][i].monthsOnProph) );

	// read in secondart OI proph regimen starting criteria
	readAndSkipPast( "SecProphStart", inputFile );
	readAndSkipPast( "boolFlag", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.startProph[PROPH_SECONDARY][i].useOrEvaluation) );
	readAndSkipPast( "SecProphStart", inputFile );
	readAndSkipPast( "curCD4upp", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.startProph[PROPH_SECONDARY][i].currCD4Bounds[UPPER_BOUND]) );
	readAndSkipPast( "SecProphStart", inputFile );
	readAndSkipPast( "curCD4lwr", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.startProph[PROPH_SECONDARY][i].currCD4Bounds[LOWER_BOUND]) );
	readAndSkipPast( "SecProphStart", inputFile );
	readAndSkipPast( "minCD4upp", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.startProph[PROPH_SECONDARY][i].minCD4Bounds[UPPER_BOUND]) );
	readAndSkipPast( "SecProphStart", inputFile );
	readAndSkipPast( "minCD4lwr", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.startProph[PROPH_SECONDARY][i].minCD4Bounds[LOWER_BOUND]) );
	for ( j = 0; j < OI_NUM; ++j ) {
		readAndSkipPast( "SecProphStart", inputFile );
		readAndSkipPast( OI_STRS[j], inputFile );
		for ( i = 0; i < OI_NUM; ++i )
			fscanf( inputFile, "%d", &(treatmentInputs.startProph[PROPH_SECONDARY][i].OIHistory[j]) );
	}
	readAndSkipPast( "SecProphStart", inputFile );
	readAndSkipPast( "minMthNum", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.startProph[PROPH_SECONDARY][i].minMonthNum) );

	// read in secondary OI proph regimen stopping criteria
	readAndSkipPast( "SecProphStop", inputFile );
	readAndSkipPast( "boolFlag", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.stopProph[PROPH_SECONDARY][i].useOrEvaluation) );
	readAndSkipPast( "SecProphStop", inputFile );
	readAndSkipPast( "curCD4upp", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.stopProph[PROPH_SECONDARY][i].currCD4Bounds[UPPER_BOUND]) );
	readAndSkipPast( "SecProphStop", inputFile );
	readAndSkipPast( "curCD4lwr", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.stopProph[PROPH_SECONDARY][i].currCD4Bounds[LOWER_BOUND]) );
	readAndSkipPast( "SecProphStop", inputFile );
	readAndSkipPast( "minCD4upp", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.stopProph[PROPH_SECONDARY][i].minCD4Bounds[UPPER_BOUND]) );
	readAndSkipPast( "SecProphStop", inputFile );
	readAndSkipPast( "minCD4lwr", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%lf", &(treatmentInputs.stopProph[PROPH_SECONDARY][i].minCD4Bounds[LOWER_BOUND]) );
	for ( j = 0; j < OI_NUM; ++j ) {
		readAndSkipPast( "SecProphStop", inputFile );
		readAndSkipPast( OI_STRS[j], inputFile );
		for ( i = 0; i < OI_NUM; ++i )
			fscanf( inputFile, "%d", &(treatmentInputs.stopProph[PROPH_SECONDARY][i].OIHistory[j]) );
	}
	readAndSkipPast( "SecProphStop", inputFile );
	readAndSkipPast( "minMthNum", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.stopProph[PROPH_SECONDARY][i].minMonthNum) );
	readAndSkipPast( "SecProphStop", inputFile );
	readAndSkipPast( "mthsOnProph", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%d", &(treatmentInputs.stopProph[PROPH_SECONDARY][i].monthsOnProph) );
} /* end readTreatmentInputsPart2 */

/* readLTFUInputs reads data from the LTFU tab of the input sheet */
void SimContext::readLTFUInputs() {
	int tempBool;

	// read in LTFU variables
	readAndSkipPast( "UseLTFU", inputFile);
	fscanf( inputFile, "%d", &tempBool);
	ltfuInputs.useLTFU = (bool) tempBool;
	readAndSkipPast( "LTFUBackground", inputFile);
	fscanf( inputFile, "%lf %lf", &(ltfuInputs.regressionCoefficientsLTFUPreART[LTFU_BACKGROUND]),
		&(ltfuInputs.regressionCoefficientsLTFUPostART[LTFU_BACKGROUND]));
	readAndSkipPast( "LTFUAge", inputFile);
	fscanf( inputFile, "%lf %lf", &(ltfuInputs.regressionCoefficientsLTFUPreART[LTFU_AGE]),
		&(ltfuInputs.regressionCoefficientsLTFUPostART[LTFU_AGE]));
	readAndSkipPast( "LTFUGender", inputFile);
	fscanf( inputFile, "%lf %lf", &(ltfuInputs.regressionCoefficientsLTFUPreART[LTFU_GENDER]),
		&(ltfuInputs.regressionCoefficientsLTFUPostART[LTFU_GENDER]));
	readAndSkipPast( "LTFUHistory", inputFile);
	fscanf( inputFile, "%lf %lf", &(ltfuInputs.regressionCoefficientsLTFUPreART[LTFU_HISTORY]),
		&(ltfuInputs.regressionCoefficientsLTFUPostART[LTFU_HISTORY]));
	readAndSkipPast( "LTFUT1", inputFile);
	fscanf( inputFile, "%lf %lf", &(ltfuInputs.regressionCoefficientsLTFUPreART[LTFU_T1]),
		&(ltfuInputs.regressionCoefficientsLTFUPostART[LTFU_T1]));
	readAndSkipPast( "LTFUT1T2", inputFile);
	fscanf( inputFile, "%lf %lf", &(ltfuInputs.regressionCoefficientsLTFUPreART[LTFU_T1_T2]),
		&(ltfuInputs.regressionCoefficientsLTFUPostART[LTFU_T1_T2]));
	readAndSkipPast( "LTFUT2", inputFile);
	fscanf( inputFile, "%lf %lf", &(ltfuInputs.regressionCoefficientsLTFUPreART[LTFU_T2]),
		&(ltfuInputs.regressionCoefficientsLTFUPostART[LTFU_T2]));
	readAndSkipPast( "LTFUTimeInt", inputFile);
	fscanf( inputFile, "%d %d", &(ltfuInputs.timeBoundsFromARTInitLTFU[0]), &(ltfuInputs.timeBoundsFromARTInitLTFU[1]));
	readAndSkipPast( "LTFUAgeThreshhold", inputFile);
	fscanf( inputFile, "%d", &ltfuInputs.ageThresholdLTFU);
	readAndSkipPast( "pLTFUOIProph", inputFile);
	fscanf( inputFile, "%lf", &ltfuInputs.probRemainOnOIProph);
	readAndSkipPast( "pLTFUOITreat", inputFile);
	fscanf( inputFile, "%lf", &ltfuInputs.probRemainOnOITreatment);

	//Return to Care variables...
	readAndSkipPast( "RTCMinMonthsLost", inputFile);
	fscanf( inputFile, "%d", &ltfuInputs.minMonthsRemainLost);
	readAndSkipPast( "RTCBackground", inputFile);
	fscanf( inputFile, "%lf", &(ltfuInputs.regressionCoefficientsRTC[RTC_BACKGROUND]));
	readAndSkipPast( "RTCCD4", inputFile);
	fscanf( inputFile, "%lf", &(ltfuInputs.regressionCoefficientsRTC[RTC_CD4]));
	readAndSkipPast( "RTCAcuteSevereOI", inputFile);
	fscanf( inputFile, "%lf", &(ltfuInputs.regressionCoefficientsRTC[RTC_ACUTESEVEREOI]));
	readAndSkipPast( "RTCAcuteMildOI", inputFile);
	fscanf( inputFile, "%lf", &(ltfuInputs.regressionCoefficientsRTC[RTC_ACUTEMILDOI]));
	readAndSkipPast( "RTCCD4Threshhold", inputFile);
	fscanf( inputFile, "%lf", &ltfuInputs.CD4ThresholdRTC);
	readAndSkipPast("RTCSevereOIType", inputFile);
	for (int i = 0; i < OI_NUM; i++) {
		fscanf(inputFile, "%d ", &tempBool);
		ltfuInputs.severeOIsRTC[i] = (int) tempBool;
	}
	readAndSkipPast( "RTCMaxTimePrevOnART", inputFile);
	fscanf( inputFile, "%d", &ltfuInputs.maxMonthsAfterObservedFailureToRestartRegimen);
	readAndSkipPast( "RTCProbTakeSameART", inputFile);
	fscanf( inputFile, "%lf", &ltfuInputs.probRestartRegimenWithoutObsvervedFailure);
	readAndSkipPast( "RTCProbSuppPrevFail", inputFile);
	for (int i = 0; i < ART_NUM_LINES; i++) {
		fscanf(inputFile, "%lf", &(ltfuInputs.probSuppressionWhenReturnToFailed[i]));
	}
	readAndSkipPast( "RTCProbSuppPrevSupp", inputFile);
	for (int i = 0; i < ART_NUM_LINES; i++) {
		fscanf(inputFile, "%lf", &(ltfuInputs.probSuppressionWhenReturnToSuppressed[i]));
	}
} /* end readLTFUInputs */

/* readHeterogeneityInputs reads data from the Heterogeneity tab of the input sheet */
void SimContext::readHeterogeneityInputs() {
	// read in the propensity to respond coefficients
	readAndSkipPast("PropRespBaseline", inputFile);
	fscanf(inputFile, "%lf %lf", &(heterogeneityInputs.propRespondBaselineMean),
			&(heterogeneityInputs.propRespondBaselineStdDev));
	readAndSkipPast("PropRespAge", inputFile);
	for (int i = 0; i < RESP_AGE_CAT_NUM; i++) {
		fscanf(inputFile, "%lf ", &(heterogeneityInputs.propRespondAge[i]));
	}
	readAndSkipPast("PropRespCD4", inputFile);
	for (int i = CD4_NUM_STRATA - 1; i >= 0; i--) {
		fscanf(inputFile, "%lf ", &(heterogeneityInputs.propRespondCD4[i]));
	}
	readAndSkipPast("PropRespFemale", inputFile);
	fscanf(inputFile, "%lf", &(heterogeneityInputs.propRespondFemale));
	readAndSkipPast("PropRespHistOIs", inputFile);
	fscanf(inputFile, "%lf", &(heterogeneityInputs.propRespondHistoryOIs));
	readAndSkipPast("PropRespPriorARTTox", inputFile);
	fscanf(inputFile, "%lf", &(heterogeneityInputs.propRespondPriorARTToxicity));
	readAndSkipPast("PropRespRiskFactors", inputFile);
	for (int i = 0; i < RISK_FACT_NUM; i++) {
		fscanf(inputFile, "%lf ", &(heterogeneityInputs.propRespondRiskFactor[i]));
	}
	readAndSkipPast("PropRespARTRegimens", inputFile);
	for (int i = 0; i < ART_NUM_LINES; i++) {
		fscanf(inputFile, "%lf ", &(heterogeneityInputs.propRespondARTRegimen[i]));
	}
	readAndSkipPast("PropRespIndivStdDev", inputFile);
	fscanf(inputFile, "%lf", &(heterogeneityInputs.propRespondIndividualStdDev));

	// read in the clinical response parameters
	readAndSkipPast("ResponseTypeThresholdsL1", inputFile);
	for (int i = 0; i < ART_NUM_LINES; i++) {
		fscanf(inputFile, "%lf ", &(heterogeneityInputs.responseTypeThresholds[i][0]));
	}
	readAndSkipPast("ResponseTypeThresholdsL2", inputFile);
	for (int i = 0; i < ART_NUM_LINES; i++) {
		fscanf(inputFile, "%lf ", &(heterogeneityInputs.responseTypeThresholds[i][1]));
	}
	readAndSkipPast("ProbFillRxNonResponders", inputFile);
	fscanf(inputFile, "%lf", &(heterogeneityInputs.probFillARTPrescriptionsNonResponder));
	readAndSkipPast("ProbRestartRegimen", inputFile);
	for (int i = 0; i < ART_NUM_LINES; i++) {
		fscanf(inputFile, "%lf ", &(heterogeneityInputs.probRestartARTRegimenAfterFailure[i]));
	}

	// read in the adherence intervention parameters

} /* end readHeterogeneityInputs */

/* readSTIInputs reads data from the STI tab of the input sheet */
void SimContext::readSTIInputs() {
	char tmpBuf1[256], tmpBuf2[256];
	int i, j, tempBool;

	// read in STI initiation parameters
	// read in CD4 bounds
	readAndSkipPast2( "STIstart_CD4", "upp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(stiInputs.firstInterruption[i].CD4BoundsOnly[UPPER_BOUND]) );
	readAndSkipPast2( "STIstart_CD4", "lwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(stiInputs.firstInterruption[i].CD4BoundsOnly[LOWER_BOUND]) );
	// read in HVL bounds to begin STI
	readAndSkipPast2( "STIstart_HVL", "upp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(stiInputs.firstInterruption[i].HVLBoundsOnly[UPPER_BOUND]) );
	readAndSkipPast2( "STIstart_HVL", "lwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(stiInputs.firstInterruption[i].HVLBoundsOnly[LOWER_BOUND]) );
	// read in CD4 & HVL bounds to begin STI
	readAndSkipPast2( "STIstart_CD4HVL", "CD4upp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(stiInputs.firstInterruption[i].CD4BoundsWithHVL[UPPER_BOUND]) );
	readAndSkipPast2( "STIstart_CD4HVL", "CD4lwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(stiInputs.firstInterruption[i].CD4BoundsWithHVL[LOWER_BOUND]) );
	readAndSkipPast2( "STIstart_CD4HVL", "HVLupp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(stiInputs.firstInterruption[i].HVLBoundsWithCD4[UPPER_BOUND]) );
	readAndSkipPast2( "STIstart_CD4HVL", "HVLlwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(stiInputs.firstInterruption[i].HVLBoundsWithCD4[LOWER_BOUND]) );
	// read in OI criteria to begin STI
	for (j = 0; j < OI_NUM; ++j) {
		readAndSkipPast2( "STIstart_OIs", OI_STRS[j], inputFile );
		for (i = 0; i < ART_NUM_LINES; ++i) {
			fscanf( inputFile, "%d", &tempBool);
			stiInputs.firstInterruption[i].OIHistory[j] = tempBool;
		}
	}
	readAndSkipPast2( "STIstart_OIs", "numOIs", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(stiInputs.firstInterruption[i].numOIs) );
	// read in CD4 & OI criteria to begin STI
	readAndSkipPast2( "STIstart_CD4OI", "CD4upp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(stiInputs.firstInterruption[i].CD4BoundsWithOIs[UPPER_BOUND]) );
	readAndSkipPast2( "STIstart_CD4OI", "CD4lwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(stiInputs.firstInterruption[i].CD4BoundsWithOIs[LOWER_BOUND]) );
	for (j = 0; j < OI_NUM; ++j) {
		readAndSkipPast2( "STIstart_CD4OI", OI_STRS[j], inputFile );
		for (i = 0; i < ART_NUM_LINES; ++i) {
			fscanf( inputFile, "%d", &tempBool);
			stiInputs.firstInterruption[i].OIHistoryWithCD4[j] = (bool) tempBool;
		}
	}
	// read in minimum mth # to beginSTI
	readAndSkipPast2( "STIstart", "minMthNum", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(stiInputs.firstInterruption[i].minMonthNum) );
	readAndSkipPast2( "STIstart", "minMthNum_ARTinit", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(stiInputs.firstInterruption[i].monthsSinceARTStart) );

	// read in STI ART restarting parameters
	readAndSkipPast2( "STI_restartART", "cd4upp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(stiInputs.ARTRestartCD4Bounds[i][UPPER_BOUND]) );
	readAndSkipPast2( "STI_restartART", "cd4lwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(stiInputs.ARTRestartCD4Bounds[i][LOWER_BOUND]) );
	readAndSkipPast2( "STI_restartART", "hvlupp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(stiInputs.ARTRestartHVLBounds[i][UPPER_BOUND]) );
	readAndSkipPast2( "STI_restartART", "hvllwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(stiInputs.ARTRestartHVLBounds[i][LOWER_BOUND]) );

	// read in STI ART successive interruption parameters
	readAndSkipPast2( "STI_restopART", "cd4upp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(stiInputs.ARTRestopCD4Bounds[i][UPPER_BOUND]) );
	readAndSkipPast2( "STI_restopART", "cd4lwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(stiInputs.ARTRestopCD4Bounds[i][LOWER_BOUND]) );
	readAndSkipPast2( "STI_restopART", "hvlupp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(stiInputs.ARTRestopHVLBounds[i][UPPER_BOUND]) );
	readAndSkipPast2( "STI_restopART", "hvllwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(stiInputs.ARTRestopHVLBounds[i][LOWER_BOUND]) );

	// read in STI endpoint parameters
	// read in CD4 bounds
	readAndSkipPast2( "STIendpt_CD4", "upp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(stiInputs.endpoint[i].CD4BoundsOnly[UPPER_BOUND]) );
	readAndSkipPast2( "STIendpt_CD4", "lwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(stiInputs.endpoint[i].CD4BoundsOnly[LOWER_BOUND]) );
	// read in HVL bounds for STI endpoint
	readAndSkipPast2( "STIendpt_HVL", "upp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(stiInputs.endpoint[i].HVLBoundsOnly[UPPER_BOUND]) );
	readAndSkipPast2( "STIendpt_HVL", "lwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(stiInputs.endpoint[i].HVLBoundsOnly[LOWER_BOUND]) );
	// read in CD4 & HVL bounds for STI endpoint
	readAndSkipPast2( "STIendpt_CD4HVL", "CD4upp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(stiInputs.endpoint[i].CD4BoundsWithHVL[UPPER_BOUND]) );
	readAndSkipPast2( "STIendpt_CD4HVL", "CD4lwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(stiInputs.endpoint[i].CD4BoundsWithHVL[LOWER_BOUND]) );
	readAndSkipPast2( "STIendpt_CD4HVL", "HVLupp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(stiInputs.endpoint[i].HVLBoundsWithCD4[UPPER_BOUND]) );
	readAndSkipPast2( "STIendpt_CD4HVL", "HVLlwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(stiInputs.endpoint[i].HVLBoundsWithCD4[LOWER_BOUND]) );
	// read in OI criteria for STI endpoint
	for (j = 0; j < OI_NUM; ++j) {
		readAndSkipPast2( "STIendpt_OIs", OI_STRS[j], inputFile );
		for (i = 0; i < ART_NUM_LINES; ++i) {
			fscanf( inputFile, "%d", &tempBool);
			stiInputs.endpoint[i].OIHistory[j] = (bool) tempBool;
		}
	}
	readAndSkipPast2( "STIendpt_OIs", "numOIs", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(stiInputs.endpoint[i].numOIs) );
	// read in CD4 & OI criteria for STI endpoint
	readAndSkipPast2( "STIendpt_CD4OI", "CD4upp", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(stiInputs.endpoint[i].CD4BoundsWithOIs[UPPER_BOUND]) );
	readAndSkipPast2( "STIendpt_CD4OI", "CD4lwr", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%lf", &(stiInputs.endpoint[i].CD4BoundsWithOIs[LOWER_BOUND]) );
	for (j = 0; j < OI_NUM; ++j) {
		readAndSkipPast2( "STIendpt_CD4OI", OI_STRS[j], inputFile );
		for (i = 0; i < ART_NUM_LINES; ++i) {
			fscanf( inputFile, "%d", &tempBool);
			stiInputs.endpoint[i].OIHistoryWithCD4[j] = (bool) tempBool;
		}
	}
	// read in minimum mth # for STI endpoint
	readAndSkipPast2( "STIendpt", "minMthNum", inputFile );
	for (i = 0; i < ART_NUM_LINES; ++i)
		fscanf( inputFile, "%d", &(stiInputs.endpoint[i].monthsSinceSTIStart) );
} /* end readSTIInputs */

/* readProphInputs reads data from the UserProphs tab of the input sheet */
void SimContext::readProphInputs() {
	char scratch[256], buffer[256];
	int i, j, k, tempBool;

	for ( k = 0; k < OI_NUM; ++k) {
		for ( i = 0; i < PROPH_NUM; ++i ) {
			// read in OI proph id and name
			sprintf( scratch, "OI%d_PriProph%d", k + 1, i + 1 );
			readAndSkipPast( scratch, inputFile );
			readAndSkipPast( "Id", inputFile );
			int idNum;
			fscanf( inputFile, " %d", &idNum);
			// continue to next proph if this one is unspecified
			if (idNum == NOT_APPL) {
				prophsInputs[PROPH_PRIMARY][k][i] = NULL;
				continue;
			}
			// allocate a proph input structure
			prophsInputs[PROPH_PRIMARY][k][i] = new ProphInputs();

			// read in OI proph efficacy (for primary proph, primary OIs only in LDC model)
			readAndSkipPast( scratch, inputFile );
			readAndSkipPast( "EffPriOIs", inputFile );
			for ( j = 0; j < OI_NUM; ++j )
				fscanf( inputFile, "%lf", &(prophsInputs[PROPH_PRIMARY][k][i]->primaryOIEfficacy[j]) );

			// read in OI primary proph efficacy on secondary OIs
			readAndSkipPast( scratch, inputFile );
			readAndSkipPast( "EffSecOIs", inputFile );
			for ( j = 0; j < OI_NUM; ++j )
				fscanf( inputFile, "%lf", &(prophsInputs[PROPH_PRIMARY][k][i]->secondaryOIEfficacy[j]) );

			// read in proph resist prob, level of proph resistance, time of proph resistance,
			// cost factor of proph resistance, & mortality factor of proph resistance
			readAndSkipPast( scratch, inputFile );
			readAndSkipPast( "Resist", inputFile );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_PRIMARY][k][i]->monthlyProbResistance) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_PRIMARY][k][i]->percentResistance) );
			fscanf( inputFile, "%d", &(prophsInputs[PROPH_PRIMARY][k][i]->timeOfResistance) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_PRIMARY][k][i]->costFactorResistance) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_PRIMARY][k][i]->mortalityFactorResistance) );

			// read in min & maj tox for proph
			readAndSkipPast( scratch, inputFile );
			readAndSkipPast( "Tox", inputFile );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_PRIMARY][k][i]->probMinorToxicity) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_PRIMARY][k][i]->probMajorToxicity) );
			fscanf( inputFile, "%d", &(prophsInputs[PROPH_PRIMARY][k][i]->monthsToToxicity) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_PRIMARY][k][i]->probDeathMajorToxicity) );

			// read in costs and QOL for proph
			readAndSkipPast( scratch, inputFile );
			readAndSkipPast( "CostQOL", inputFile );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_PRIMARY][k][i]->costMonthly) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_PRIMARY][k][i]->costMinorToxicity) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_PRIMARY][k][i]->QOLMinorToxicity) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_PRIMARY][k][i]->costMajorToxicity) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_PRIMARY][k][i]->QOLMajorToxicity) );

			// read in proph switching inputs
			readAndSkipPast(scratch, inputFile);
			readAndSkipPast("Switch", inputFile);
			fscanf( inputFile, "%d", &(prophsInputs[PROPH_PRIMARY][k][i]->monthsToSwitch) );
			fscanf( inputFile, "%d", &tempBool);
			prophsInputs[PROPH_PRIMARY][k][i]->switchOnMinorToxicity = (bool) tempBool;
			fscanf( inputFile, "%d", &tempBool);
			prophsInputs[PROPH_PRIMARY][k][i]->switchOnMajorToxicity = (bool) tempBool;
		}
	}

	for ( k = 0; k < OI_NUM; ++k) {
		for ( i = 0; i < PROPH_NUM; ++i ) {
			// read in OI proph id and name
			sprintf( scratch, "OI%d_SecProph%d", k + 1, i + 1 );
			readAndSkipPast( scratch, inputFile );
			readAndSkipPast( "Id", inputFile );
			int idNum;
			fscanf( inputFile, " %d", &idNum);
			// continue to next proph if this one is unspecified
			if (idNum == NOT_APPL) {
				prophsInputs[PROPH_SECONDARY][k][i] = NULL;
				continue;
			}
			// allocate a proph input structure
			prophsInputs[PROPH_SECONDARY][k][i] = new ProphInputs();

			// read in OI proph efficacy (for primary proph, primary OIs only in LDC model)
			readAndSkipPast( scratch, inputFile );
			readAndSkipPast( "EffPriOIs", inputFile );
			for ( j = 0; j < OI_NUM; ++j )
				fscanf( inputFile, "%lf", &(prophsInputs[PROPH_SECONDARY][k][i]->primaryOIEfficacy[j]) );

			// read in OI primary proph efficacy on secondary OIs
			readAndSkipPast( scratch, inputFile );
			readAndSkipPast( "EffSecOIs", inputFile );
			for ( j = 0; j < OI_NUM; ++j )
				fscanf( inputFile, "%lf", &(prophsInputs[PROPH_SECONDARY][k][i]->secondaryOIEfficacy[j]) );

			// read in proph resist prob, level of proph resistance, time of proph resistance,
			// cost factor of proph resistance, & mortality factor of proph resistance
			readAndSkipPast( scratch, inputFile );
			readAndSkipPast( "Resist", inputFile );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_SECONDARY][k][i]->monthlyProbResistance) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_SECONDARY][k][i]->percentResistance) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_SECONDARY][k][i]->timeOfResistance) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_SECONDARY][k][i]->costFactorResistance) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_SECONDARY][k][i]->mortalityFactorResistance) );

			// read in min & maj tox for proph
			readAndSkipPast( scratch, inputFile );
			readAndSkipPast( "Tox", inputFile );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_SECONDARY][k][i]->probMinorToxicity) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_SECONDARY][k][i]->probMajorToxicity) );
			fscanf( inputFile, "%d", &(prophsInputs[PROPH_SECONDARY][k][i]->monthsToToxicity) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_SECONDARY][k][i]->probDeathMajorToxicity) );

			// read in costs and QOL for proph
			readAndSkipPast( scratch, inputFile );
			readAndSkipPast( "CostQOL", inputFile );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_SECONDARY][k][i]->costMonthly) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_SECONDARY][k][i]->costMinorToxicity) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_SECONDARY][k][i]->QOLMinorToxicity) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_SECONDARY][k][i]->costMajorToxicity) );
			fscanf( inputFile, "%lf", &(prophsInputs[PROPH_SECONDARY][k][i]->QOLMajorToxicity) );

			// read in proph switching inputs
			readAndSkipPast(scratch, inputFile);
			readAndSkipPast("Switch", inputFile);
			fscanf( inputFile, "%d", &(prophsInputs[PROPH_SECONDARY][k][i]->monthsToSwitch) );
			fscanf( inputFile, "%d", &tempBool);
			prophsInputs[PROPH_SECONDARY][k][i]->switchOnMinorToxicity = (bool) tempBool;
			fscanf( inputFile, "%d", &tempBool);
			prophsInputs[PROPH_SECONDARY][k][i]->switchOnMajorToxicity = (bool) tempBool;
		}
	}
} /* end readProphInputs */

/* readARTInputs reads data from the UserARTs tab of the input sheet */
void SimContext::readARTInputs() {
	char tmpBuf[256], buffer[256];
	int i, j, k;
	double tempCost;
	FILE *file = inputFile;

	for (int artNum = 1; artNum <= ART_NUM_LINES; artNum++) {
		// read in regimen id num and name
		sprintf(tmpBuf, "ART%dId", artNum);
		readAndSkipPast( tmpBuf, file );
		int idNum;
		fscanf( file, " %d", &idNum );
		// skip to next regimen if this one is not specified
		if (idNum == NOT_APPL) {
			artInputs[artNum - 1] = NULL;
			continue;
		}
		// create new regimen input structure
		artInputs[artNum - 1] = new ARTInputs();
		ARTInputs &artInput = *(artInputs[artNum - 1]);

		// read in one-time startup cost
		sprintf(tmpBuf, "ART%dInitCost", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf", &artInput.costInitial );
		// read in additional cost from the treatment tab and add to initial cost
		sprintf(tmpBuf, "ART%dInitCostAdd", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf", &tempCost );
		artInput.costInitial += tempCost;
		// read in monthly cost
		sprintf(tmpBuf, "ART%dMthCost", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf", &artInput.costMonthly );
		// read in additional cost from the treatment tab and add to monthly cost
		sprintf(tmpBuf, "ART%dMthCostAdd", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf", &tempCost );
		artInput.costMonthly += tempCost;

		// read in efficacy time horizon
		sprintf(tmpBuf, "ART%dEffTimeHorizon", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%d", &artInput.efficacyTimeHorizon );
		// read in chances of ART success
		sprintf(tmpBuf, "ART%dSuccessProb", artNum);
		readAndSkipPast( tmpBuf, file );
		readAndSkipPast( "Succ", file );
		for (i = HVL_NUM_STRATA - 1; i >= 0; --i)
			fscanf( file, "%lf", &(artInput.probInitialEfficacy[ART_EFF_SUCCESS][i]) );
		readAndSkipPast( tmpBuf, file );
		readAndSkipPast( "Part", file );
		for (i = HVL_NUM_STRATA - 1; i >= 0; --i)
			fscanf( file, "%lf", &(artInput.probInitialEfficacy[ART_EFF_PARTIAL][i]) );
		for (i = HVL_NUM_STRATA - 1; i >= 0; --i)
			artInput.probInitialEfficacy[ART_EFF_FAILURE][i] = 1.0 - artInput.probInitialEfficacy[ART_EFF_SUCCESS][i] - artInput.probInitialEfficacy[ART_EFF_PARTIAL][i];

		// read in distribution of partial suppression
		sprintf(tmpBuf, "ART%dDistribPartial", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf %lf", &(artInput.partialSuppressionDistribution[0]),
			&(artInput.partialSuppressionDistribution[1]),
			&(artInput.partialSuppressionDistribution[2]) );
		// read in chances of late failure/partial suppression
		sprintf(tmpBuf, "ART%dLateFail_Succ", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf", &artInput.probLateFailFromSuppress );
		sprintf(tmpBuf, "ART%dLatePart_Succ", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf", &artInput.probLatePartialSuppressFromSuppress );
		sprintf(tmpBuf, "ART%dLateFail_Part", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf", &artInput.probLateFailFromPartialSuppress );
		// read in mth by which all would fail
		sprintf(tmpBuf, "ART%dMthForceFail", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%d", &artInput.forceFailAtMonth );

		// read in CD4 effect on ART
		sprintf(tmpBuf, "ART%dMthStageCD4Eff_Succ", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%d %d", &(artInput.stageBoundsCD4ChangeOnART[ART_EFF_SUCCESS][0]), &(artInput.stageBoundsCD4ChangeOnART[ART_EFF_SUCCESS][1]));
		for (j = 0; j < CD4_RESPONSE_NUM_TYPES; j++) {
			sprintf(tmpBuf, "ART%dCD4EffSlope_Succ", artNum);
			readAndSkipPast2( tmpBuf, CD4_RESPONSE_STRS[j], file );
			fscanf( file, "%lf %lf %lf %lf %lf %lf",
				&(artInput.CD4ChangeOnARTMean[ART_EFF_SUCCESS][j][0]), &(artInput.CD4ChangeOnARTStdDev[ART_EFF_SUCCESS][j][0]),
				&(artInput.CD4ChangeOnARTMean[ART_EFF_SUCCESS][j][1]), &(artInput.CD4ChangeOnARTStdDev[ART_EFF_SUCCESS][j][1]),
				&(artInput.CD4ChangeOnARTMean[ART_EFF_SUCCESS][j][2]), &(artInput.CD4ChangeOnARTStdDev[ART_EFF_SUCCESS][j][2]));
		}
		sprintf(tmpBuf, "ART%dMthStageCD4Eff_Part", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%d %d", &(artInput.stageBoundsCD4ChangeOnART[ART_EFF_PARTIAL][0]), &(artInput.stageBoundsCD4ChangeOnART[ART_EFF_PARTIAL][1]));
		for (j = 0; j < CD4_RESPONSE_NUM_TYPES; j++) {
			sprintf(tmpBuf, "ART%dCD4EffSlope_Part", artNum);
			readAndSkipPast2( tmpBuf, CD4_RESPONSE_STRS[j], file );
			fscanf( file, "%lf %lf %lf %lf %lf %lf",
				&(artInput.CD4ChangeOnARTMean[ART_EFF_PARTIAL][j][0]), &(artInput.CD4ChangeOnARTStdDev[ART_EFF_PARTIAL][j][0]),
				&(artInput.CD4ChangeOnARTMean[ART_EFF_PARTIAL][j][1]), &(artInput.CD4ChangeOnARTStdDev[ART_EFF_PARTIAL][j][1]),
				&(artInput.CD4ChangeOnARTMean[ART_EFF_PARTIAL][j][2]), &(artInput.CD4ChangeOnARTStdDev[ART_EFF_PARTIAL][j][2]));
		}
		sprintf(tmpBuf, "ART%dMthStageCD4Eff_Fail", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%d %d", &(artInput.stageBoundsCD4ChangeOnART[ART_EFF_FAILURE][0]), &(artInput.stageBoundsCD4ChangeOnART[ART_EFF_FAILURE][1]));
		for (j = 0; j < CD4_RESPONSE_NUM_TYPES; j++) {
			sprintf(tmpBuf, "ART%dCD4EffMult_Fail", artNum);
			readAndSkipPast2( tmpBuf, CD4_RESPONSE_STRS[j], file );
			fscanf( file, "%lf %lf %lf %lf %lf %lf",
				&(artInput.CD4MultiplierOnFailedART[j][0]),
				&(artInput.CD4MultiplierOnFailedART[j][1]),
				&(artInput.CD4MultiplierOnFailedART[j][2]));
		}
		sprintf(tmpBuf, "ART%dMthCD4SecStdDev", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf", &artInput.secondaryCD4ChangeOnARTStdDev);

		// read in CD4 effect off ART
		sprintf(tmpBuf, "ART%dCD4EffOffART_Succ", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf", &(artInput.monthlyCD4MultiplierOffARTPreSetpoint[ART_EFF_SUCCESS]),
			&(artInput.monthlyCD4MultiplierOffARTPostSetpoint[ART_EFF_SUCCESS]) );
		sprintf(tmpBuf, "ART%dCD4EffOffART_Part", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf", &(artInput.monthlyCD4MultiplierOffARTPreSetpoint[ART_EFF_PARTIAL]),
			&(artInput.monthlyCD4MultiplierOffARTPostSetpoint[ART_EFF_PARTIAL]) );
		sprintf(tmpBuf, "ART%dCD4EffOffART_Fail", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf", &(artInput.monthlyCD4MultiplierOffARTPreSetpoint[ART_EFF_FAILURE]),
			&(artInput.monthlyCD4MultiplierOffARTPostSetpoint[ART_EFF_FAILURE]) );

		// read in HVL change rate
		sprintf(tmpBuf, "ART%dHVLChgRate", artNum);
		readAndSkipPast( tmpBuf, file );
		readAndSkipPast( "Supp", file );
		fscanf( file, "%lf %d", &(artInput.monthlyProbHVLChange[ART_EFF_SUCCESS]), &(artInput.monthlyNumStrataHVLChange[ART_EFF_SUCCESS]) );
		readAndSkipPast( tmpBuf, file );
		readAndSkipPast( "PartSupp", file );
		fscanf( file, "%lf %d", &(artInput.monthlyProbHVLChange[ART_EFF_PARTIAL]), &(artInput.monthlyNumStrataHVLChange[ART_EFF_PARTIAL]) );
		readAndSkipPast( tmpBuf, file );
		readAndSkipPast( "Fail", file );
		fscanf( file, "%lf %d", &(artInput.monthlyProbHVLChange[ART_EFF_FAILURE]), &(artInput.monthlyNumStrataHVLChange[ART_EFF_FAILURE]) );

		//read in toxicity structure
		for (i = 0; i < ART_NUM_SUBREGIMENS; i++) {
			for (j = 0; j < ART_NUM_TOX_SEVERITY; j++) {
				for (k = 0; k < ART_NUM_TOX_PER_SEVERITY; k++) {
					sprintf(tmpBuf, "ART%dToxicity1.%d",artNum, i);
					readAndSkipPast( tmpBuf, file );
					//read in name of toxicity
					fscanf( file, "%32s", buffer);
					artInput.toxicity[i][j][k].toxicityName = buffer;
					//these parameters are shared by all toxicities
					fscanf( file, "%lf %lf %lf %lf %d %lf %d %d",
						&(artInput.toxicity[i][j][k].probToxicity),
						&(artInput.toxicity[i][j][k].timeToToxicityMean),
						&(artInput.toxicity[i][j][k].timeToToxicityStdDev),
						&(artInput.toxicity[i][j][k].QOLMultiplier),
						&(artInput.toxicity[i][j][k].QOLDuration),
						&(artInput.toxicity[i][j][k].costAmount),
						&(artInput.toxicity[i][j][k].costDuration),
						&(artInput.toxicity[i][j][k].switchSubRegimenOnToxicity));
					//depending in the severity if the toxicity, there might be extra parameters
					if (j == ART_TOX_CHRONIC) {
						fscanf( file, "%d %lf %d",
							&(artInput.toxicity[i][j][k].timeToChronicDeathImpact),
							&(artInput.toxicity[i][j][k].chronicDeathIncrease),
							&(artInput.toxicity[i][j][k].chronicDeathDuration));
					}
					else if (j == ART_TOX_MAJOR) {
						fscanf( file, "%lf %lf",
							 &(artInput.toxicity[i][j][k].probAcuteDeathMajorToxicity),
							 &(artInput.toxicity[i][j][k].costAcuteDeathMajorToxicity));
					}
				}
			}
			sprintf(tmpBuf, "ART%dToxicity1.%d",artNum, i);
			readAndSkipPast( tmpBuf, file );
			readAndSkipPast("TimeToSwitch", file);
			fscanf( file, "%d",&(artInput.monthsToSwitchSubRegimen[i]));
		}
	}
} /* end readARTInputs */

/* readNatHistInputs reads data from the NatHist tab of the input sheet */
void SimContext::readNatHistInputs() {
	int i, j;

	// read in chronic AIDS dth prob
	for ( j = 0; j < HIST_EXT_NUM; ++j ) {
		readAndSkipPast( "ChrAIDSDthProb_noART", inputFile );
		readAndSkipPast( HIST_OI_CATS_STRS[j], inputFile );
		for ( i = CD4_NUM_STRATA - 1; i >= 0; --i )
			fscanf( inputFile, "%lf", &(natHistInputs.chronicAIDSDeathProbOffART[j][i]) );
	}
	for ( j = 0; j < HIST_EXT_NUM; ++j ) {
		readAndSkipPast( "ChrAIDSDthProb_onART_Mult", inputFile );
		readAndSkipPast( HIST_OI_CATS_STRS[j], inputFile );
		for ( i = CD4_NUM_STRATA - 1; i >= 0; --i )
			fscanf( inputFile, "%lf", &(natHistInputs.chronicAIDSDeathProbOnARTMult[j][i]) );
	}

	// read in acute OI prob w/ no OI hist, not on ART
	for ( j = CD4_NUM_STRATA - 1; j >= 0; --j ) {
		readAndSkipPast( "OIProb_NoHist_noART", inputFile );
		readAndSkipPast( CD4_STRATA_STRS[j], inputFile );
		for ( i = 0; i < OI_NUM; ++i )
			fscanf( inputFile, "%lf", &(natHistInputs.monthlyOIProbOffART[j][i][HIST_N]) );
	}
	// read in acute OI prob w/ OI hist, not on ART
	for ( j = CD4_NUM_STRATA - 1; j >= 0; --j ) {
		readAndSkipPast( "OIProb_Hist_noART", inputFile );
		readAndSkipPast( CD4_STRATA_STRS[j], inputFile );
		for ( i = 0; i < OI_NUM; ++i )
			fscanf( inputFile, "%lf", &(natHistInputs.monthlyOIProbOffART[j][i][HIST_Y]) );
	}
	// read in acute OI prob on ART multipliers
	for ( j = CD4_NUM_STRATA - 1; j >= 0; --j ) {
		readAndSkipPast( "OIProb_onART_Mult", inputFile );
		readAndSkipPast( CD4_STRATA_STRS[j], inputFile );
		for ( i = 0; i < OI_NUM; ++i )
			fscanf( inputFile, "%lf", &(natHistInputs.monthlyOIProbOnARTMult[j][i]) );
	}

	// read in dth from acute OI prob w/ no OI hist - treated
	for ( j = CD4_NUM_STRATA - 1; j >= 0; --j ) {
		readAndSkipPast( "OIDthProb_NoHist_treated", inputFile );
		readAndSkipPast( CD4_STRATA_STRS[j], inputFile );
		for ( i = 0; i < OI_NUM; ++i )
			fscanf( inputFile, "%lf", &(natHistInputs.probDeathFromOITreated[j][i][HIST_N]) );
	}
	// read in dth from acute OI prob w/ OI hist - treated
	for ( j = CD4_NUM_STRATA - 1; j >= 0; --j ) {
		readAndSkipPast( "OIDthProb_Hist_treated", inputFile );
		readAndSkipPast( CD4_STRATA_STRS[j], inputFile );
		for ( i = 0; i < OI_NUM; ++i )
			fscanf( inputFile, "%lf", &(natHistInputs.probDeathFromOITreated[j][i][HIST_Y]) );
	}
	// read in dth from acute OI prob w/ no OI hist - untreated
	for ( j = CD4_NUM_STRATA - 1; j >= 0; --j ) {
		readAndSkipPast( "OIDthProb_NoHist_untreated", inputFile );
		readAndSkipPast( CD4_STRATA_STRS[j], inputFile );
		for ( i = 0; i < OI_NUM; ++i )
			fscanf( inputFile, "%lf", &(natHistInputs.probDeathFromOIUntreated[j][i][HIST_N]) );
	}
	// read in dth from acute OI prob w/ OI hist - untreated
	for ( j = CD4_NUM_STRATA - 1; j >= 0; --j ) {
		readAndSkipPast( "OIDthProb_Hist_untreated", inputFile );
		readAndSkipPast( CD4_STRATA_STRS[j], inputFile );
		for ( i = 0; i < OI_NUM; ++i )
			fscanf( inputFile, "%lf", &(natHistInputs.probDeathFromOIUntreated[j][i][HIST_Y]) );
	}

	// read in nat hist (Mellors) decl
	for ( j = CD4_NUM_STRATA - 1; j >= 0; --j ) {
		readAndSkipPast( "BslCD4Decl_Mean", inputFile );
		readAndSkipPast( CD4_STRATA_STRS[j], inputFile );
		for ( i = HVL_NUM_STRATA - 1; i >= HVL_VLO; --i )
			fscanf( inputFile, "%lf", &(natHistInputs.monthlyCD4DeclineMean[j][i]) );
		readAndSkipPast( "BslCD4Decl_SDev", inputFile );
		readAndSkipPast( CD4_STRATA_STRS[j], inputFile );
		for ( i = HVL_NUM_STRATA - 1; i >= HVL_VLO; --i )
			fscanf( inputFile, "%lf", &(natHistInputs.monthlyCD4DeclineStdDev[j][i]) );
	}

	// read in non AIDS dth prob
	readAndSkipPast( "NonAIDSDthProb_Male", inputFile );
	for ( i = AGE_STARTING; i < AGE_YRS; ++i )
		fscanf( inputFile, "%lf", &(natHistInputs.monthlyNonAIDSDeathProb[GENDER_MALE][i]) );
	readAndSkipPast( "NonAIDSDthProb_Female", inputFile );
	for ( i = AGE_STARTING; i < AGE_YRS; ++i )
		fscanf( inputFile, "%lf", &(natHistInputs.monthlyNonAIDSDeathProb[GENDER_FEMALE][i]) );
} /* end readNatHistInputs */

/* readCHRMsInputs reads data from the CHRMs tab of the input sheet */
void SimContext::readCHRMsInputs() {
	char scratch[256];

	// Read in CHRMs names
	readAndSkipPast("CHRMstrs", inputFile);
	for (int i = 0; i < CHRM_NUM; i++) {
		fscanf( inputFile, "%32s", CHRM_STRS[i] );
		strcpy(DTH_CAUSES_STRS[DTH_CHRM_1 + i], CHRM_STRS[i]);
	}

	// Read in probability of prevalent CHRMs, modifiers, and months since start
	for (int i = 0; i < CHRM_NUM; i++) {
		sprintf(scratch, "ProbPrevCHRM_%s", CHRM_STRS[i]);
		readAndSkipPast2(scratch, "HIVneg", inputFile);
		for (int k = 0; k < GENDER_NUM; k++) {
			for (int l = 0; l < CHRM_AGE_CAT_NUM; l++) {
				fscanf(inputFile, "%lf", &(chrmsInputs.probPrevalentCHRMsHIVneg[i][k][l]));
			}
		}
		for (int j = CD4_NUM_STRATA - 1; j >= 0; j--) {
			readAndSkipPast2(scratch, CD4_STRATA_STRS[j], inputFile);
			for (int k = 0; k < GENDER_NUM; k++) {
				for (int l = 0; l < CHRM_AGE_CAT_NUM; l++) {
					fscanf(inputFile, "%lf", &(chrmsInputs.probPrevalentCHRMs[i][j][k][l]));
				}
			}
		}
	}
	for (int i = 0; i < CHRM_NUM; i++) {
		readAndSkipPast2("PrevCHRMRiskLogit", CHRM_STRS[i], inputFile);
		for (int j = 0; j < RISK_FACT_NUM; j++) {
			fscanf(inputFile, "%lf", &(chrmsInputs.probPrevalentCHRMsRiskFactorLogit[i][j]));
		}
	}
	for (int i = 0; i < CHRM_NUM; i++) {
		readAndSkipPast2("PrevCHRMNumMonths", CHRM_STRS[i], inputFile);
		fscanf(inputFile, "%lf %lf", &(chrmsInputs.prevalentCHRMsMonthsSinceStartMean[i]),
				&(chrmsInputs.prevalentCHRMsMonthsSinceStartStdDev[i]));
	}

	// Read in probability of incident CHRMs and modifiers
	for (int i = 0; i < CHRM_NUM; i++) {
		sprintf(scratch, "ProbIncidCHRM_%s", CHRM_STRS[i]);
		readAndSkipPast2(scratch, "HIVneg", inputFile);
		for (int k = 0; k < GENDER_NUM; k++) {
			for (int l = 0; l < CHRM_AGE_CAT_NUM; l++) {
				fscanf(inputFile, "%lf", &(chrmsInputs.probIncidentCHRMsHIVneg[i][k][l]));
			}
		}
		for (int j = CD4_NUM_STRATA - 1; j >= 0; j--) {
			readAndSkipPast2(scratch, CD4_STRATA_STRS[j], inputFile);
			for (int k = 0; k < GENDER_NUM; k++) {
				for (int l = 0; l < CHRM_AGE_CAT_NUM; l++) {
					fscanf(inputFile, "%lf", &(chrmsInputs.probIncidentCHRMs[i][j][k][l]));
				}
			}
		}
	}
	for (int i = 0; i < CHRM_NUM; i++) {
		readAndSkipPast2("IncidCHRMOnARTMult", CHRM_STRS[i], inputFile);
		for (int j = CD4_NUM_STRATA - 1; j >= 0; j--) {
			fscanf(inputFile, "%lf", &(chrmsInputs.probIncidentCHRMsOnARTMult[i][j]));
		}
	}
	for (int i = 0; i < CHRM_NUM; i++) {
		readAndSkipPast2("IncidCHRMRiskLogit", CHRM_STRS[i], inputFile);
		for (int j = 0; j < RISK_FACT_NUM; j++) {
			fscanf(inputFile, "%lf", &(chrmsInputs.probIncidentCHRMsRiskFactorLogit[i][j]));
		}
	}
	for (int i = 0; i < CHRM_NUM; i++) {
		readAndSkipPast2("IncidCHRMHistoryLogit", CHRM_STRS[i], inputFile);
		for (int j = 0; j < CHRM_NUM; j++) {
			fscanf(inputFile, "%lf", &(chrmsInputs.probIncidentCHRMsPriorHistoryLogit[i][j]));
		}
	}

	// Read in probability of CHRMs death and stage bounds
	for (int i = 0; i < CHRM_NUM; i++) {
		readAndSkipPast2("ProbDeathCHRMBounds", CHRM_STRS[i], inputFile);
		for (int j = 0; j < CHRM_TIME_PER_NUM - 1; j++) {
			fscanf(inputFile, "%d", &(chrmsInputs.probDeathCHRMsStageBounds[i][j]));
		}
		for (int j = 0; j < CHRM_TIME_PER_NUM; j++) {
			sprintf(scratch, "ProbDeathCHRM_%s_T%d", CHRM_STRS[i], j + 1);
			readAndSkipPast2(scratch, "HIVneg", inputFile);
			for (int l = 0; l < GENDER_NUM; l++) {
				for (int m = 0; m < CHRM_AGE_CAT_NUM; m++) {
					fscanf(inputFile, "%lf", &(chrmsInputs.probDeathCHRMsHIVneg[i][j][l][m]));
				}
			}
			for (int k = CD4_NUM_STRATA - 1; k >= 0; k--) {
				readAndSkipPast2(scratch, CD4_STRATA_STRS[k], inputFile);
				for (int l = 0; l < GENDER_NUM; l++) {
					for (int m = 0; m < CHRM_AGE_CAT_NUM; m++) {
						fscanf(inputFile, "%lf", &(chrmsInputs.probDeathCHRMs[i][j][k][l][m]));
					}
				}
			}
		}
	}


	// Read in cost of CHRMs and stage bounds
	for (int i = 0; i < CHRM_NUM; i++) {
		readAndSkipPast2("CostCHRMBounds", CHRM_STRS[i], inputFile);
		for (int j = 0; j < CHRM_TIME_PER_NUM - 1; j++) {
			fscanf(inputFile, "%d", &(chrmsInputs.costCHRMsStageBounds[i][j]));
		}
		for (int j = 0; j < CHRM_TIME_PER_NUM; j++) {
			sprintf(scratch, "CostCHRM_%s", CHRM_STRS[i]);
			readAndSkipPast(scratch, inputFile);
			sprintf(scratch, "T%d", j + 1);
			readAndSkipPast(scratch, inputFile);
			for (int k = 0; k < GENDER_NUM; k++) {
				for (int l = 0; l < CHRM_AGE_CAT_NUM; l++) {
					fscanf(inputFile, "%lf", &(chrmsInputs.costCHRMs[i][j][k][l]));
				}
			}
		}
	}
	readAndSkipPast("CostDeathCHRM", inputFile);
	for (int i = 0; i < CHRM_NUM; i++) {
		fscanf(inputFile, "%lf", &(chrmsInputs.costDeathCHRMs[i]));
	}

	// Read in QOL of CHRMs and stage bounds
	for (int i = 0; i < CHRM_NUM; i++) {
		readAndSkipPast2("QOLCHRMBounds", CHRM_STRS[i], inputFile);
		for (int j = 0; j < CHRM_TIME_PER_NUM - 1; j++) {
			fscanf(inputFile, "%d", &(chrmsInputs.QOLMultCHRMsStageBounds[i][j]));
		}
		for (int j = 0; j < CHRM_TIME_PER_NUM; j++) {
			sprintf(scratch, "QOLCHRM_%s", CHRM_STRS[i]);
			readAndSkipPast(scratch, inputFile);
			sprintf(scratch, "T%d", j + 1);
			readAndSkipPast(scratch, inputFile);
			for (int k = 0; k < GENDER_NUM; k++) {
				for (int l = 0; l < CHRM_AGE_CAT_NUM; l++) {
					fscanf(inputFile, "%lf", &(chrmsInputs.QOLMultCHRMs[i][j][k][l]));
				}
			}
		}
	}
	readAndSkipPast("QOLMultDeathCHRM", inputFile);
	for (int i = 0; i < CHRM_NUM; i++) {
		fscanf(inputFile, "%lf", &(chrmsInputs.QOLMultDeathCHRMs[i]));
	}
} /* end readCHRMsInputs */

/* readCostInputs reads data from the Cost tab of the input sheet */
void SimContext::readCostInputs() {
	char scratch[256];
	int i, j, k;

	// read in acute OI costs
	for ( i = 0; i < OI_NUM; ++i ) {
		readAndSkipPast( "CostAcuteOI_noART_treated", inputFile );
		readAndSkipPast( OI_STRS[i], inputFile );
		for (j = 0; j < COST_NUM_TYPES; j++) {
			fscanf(inputFile, "%lf", &(costInputs.acuteOICostTreated[ART_OFF_STATE][i][j]) );
		}
	}
	for ( i = 0; i < OI_NUM; ++i ) {
		readAndSkipPast( "CostAcuteOI_noART_untreated", inputFile );
		readAndSkipPast( OI_STRS[i], inputFile );
		for (j = 0; j < COST_NUM_TYPES; j++) {
			fscanf(inputFile, "%lf", &(costInputs.acuteOICostUntreated[ART_OFF_STATE][i][j]) );
		}
	}
	for ( i = 0; i < OI_NUM; ++i ) {
		readAndSkipPast( "CostAcuteOI_onART_treated", inputFile );
		readAndSkipPast( OI_STRS[i], inputFile );
		for (j = 0; j < COST_NUM_TYPES; j++) {
			fscanf(inputFile, "%lf", &(costInputs.acuteOICostTreated[ART_ON_STATE][i][j]) );
		}
	}
	for ( i = 0; i < OI_NUM; ++i ) {
		readAndSkipPast( "CostAcuteOI_onART_untreated", inputFile );
		readAndSkipPast( OI_STRS[i], inputFile );
		for (j = 0; j < COST_NUM_TYPES; j++) {
			fscanf(inputFile, "%lf", &(costInputs.acuteOICostUntreated[ART_ON_STATE][i][j]) );
		}
	}

	// read in CD4 / HVL test costs
	readAndSkipPast( "CostCD4Test", inputFile );
	for (i = 0; i < COST_NUM_TYPES; i++) {
		fscanf(inputFile, "%lf", &(costInputs.CD4TestCost[i]) );
	}
	readAndSkipPast( "CostHVLTest", inputFile );
	for (i = 0; i < COST_NUM_TYPES; i++) {
		fscanf(inputFile, "%lf", &(costInputs.HVLTestCost[i]) );
	}

	// read in death from OI costs
	for ( i = 0; i < DTH_NUM_CAUSES_BASIC; ++i ) {
		readAndSkipPast( "CostDth_noART_treated", inputFile );
		readAndSkipPast( DTH_CAUSES_STRS[i], inputFile);
		for (j = 0; j < COST_NUM_TYPES; j++) {
			fscanf(inputFile, "%lf", &(costInputs.deathCostTreated[ART_OFF_STATE][i][j]) );
		}
	}
	for ( i = 0; i < DTH_NUM_CAUSES_BASIC; ++i ) {
		readAndSkipPast( "CostDth_noART_untreated", inputFile );
		readAndSkipPast( DTH_CAUSES_STRS[i], inputFile);
		for (j = 0; j < COST_NUM_TYPES; j++) {
			fscanf(inputFile, "%lf", &(costInputs.deathCostUntreated[ART_OFF_STATE][i][j]) );
		}
	}
	for ( i = 0; i < DTH_NUM_CAUSES_BASIC; ++i ) {
		readAndSkipPast( "CostDth_onART_treated", inputFile );
		readAndSkipPast( DTH_CAUSES_STRS[i], inputFile);
		for (j = 0; j < COST_NUM_TYPES; j++) {
			fscanf(inputFile, "%lf", &(costInputs.deathCostTreated[ART_ON_STATE][i][j]) );
		}
	}
	for ( i = 0; i < DTH_NUM_CAUSES_BASIC; ++i ) {
		readAndSkipPast( "CostDth_onART_untreated", inputFile );
		readAndSkipPast( DTH_CAUSES_STRS[i], inputFile);
		for (j = 0; j < COST_NUM_TYPES; j++) {
			fscanf(inputFile, "%lf", &(costInputs.deathCostUntreated[ART_ON_STATE][i][j]) );
		}
	}

	// read in routine care costs for HIV-neg
	readAndSkipPast("CostRoutine_HIVneg_dmed", inputFile);
	for (i = 0; i < GENDER_NUM; i++) {
		for (j = 0; j < CHRM_AGE_CAT_NUM; j++) {
			fscanf(inputFile, "%lf", &(costInputs.routineCareCostHIVNegative[i][j][COST_DIR_MED]));
		}
	}
	readAndSkipPast("CostRoutine_HIVneg_nmed", inputFile);
	for (i = 0; i < GENDER_NUM; i++) {
		for (j = 0; j < CHRM_AGE_CAT_NUM; j++) {
			fscanf(inputFile, "%lf", &(costInputs.routineCareCostHIVNegative[i][j][COST_DIR_NONMED]));
		}
	}
	readAndSkipPast("CostRoutine_HIVneg_time", inputFile);
	for (i = 0; i < GENDER_NUM; i++) {
		for (j = 0; j < CHRM_AGE_CAT_NUM; j++) {
			fscanf(inputFile, "%lf", &(costInputs.routineCareCostHIVNegative[i][j][COST_TIME]));
		}
	}
	readAndSkipPast("CostRoutine_HIVneg_indr", inputFile);
	for (i = 0; i < GENDER_NUM; i++) {
		for (j = 0; j < CHRM_AGE_CAT_NUM; j++) {
			fscanf(inputFile, "%lf", &(costInputs.routineCareCostHIVNegative[i][j][COST_INDIR]));
		}
	}
	// read in routine car costs for HIV positive, not on ART
	for (i = CD4_NUM_STRATA - 1; i >= 0; i--) {
		readAndSkipPast2("CostRoutine_HIVpos_noART_dmed", CD4_STRATA_STRS[i], inputFile);
		for (j = 0; j < GENDER_NUM; j++) {
			for (k = 0; k < CHRM_AGE_CAT_NUM; k++) {
				fscanf(inputFile, "%lf", &(costInputs.routineCareCostHIVPositive[ART_OFF_STATE][i][j][k][COST_DIR_MED]));
			}
		}
		readAndSkipPast2("CostRoutine_HIVpos_noART_nmed", CD4_STRATA_STRS[i], inputFile);
		for (j = 0; j < GENDER_NUM; j++) {
			for (k = 0; k < CHRM_AGE_CAT_NUM; k++) {
				fscanf(inputFile, "%lf", &(costInputs.routineCareCostHIVPositive[ART_OFF_STATE][i][j][k][COST_DIR_NONMED]));
			}
		}
		readAndSkipPast2("CostRoutine_HIVpos_noART_time", CD4_STRATA_STRS[i], inputFile);
		for (j = 0; j < GENDER_NUM; j++) {
			for (k = 0; k < CHRM_AGE_CAT_NUM; k++) {
				fscanf(inputFile, "%lf", &(costInputs.routineCareCostHIVPositive[ART_OFF_STATE][i][j][k][COST_TIME]));
			}
		}
		readAndSkipPast2("CostRoutine_HIVpos_noART_indr", CD4_STRATA_STRS[i], inputFile);
		for (j = 0; j < GENDER_NUM; j++) {
			for (k = 0; k < CHRM_AGE_CAT_NUM; k++) {
				fscanf(inputFile, "%lf", &(costInputs.routineCareCostHIVPositive[ART_OFF_STATE][i][j][k][COST_INDIR]));
			}
		}
	}
	// read in routine car costs for HIV positive, on ART
	for (i = CD4_NUM_STRATA - 1; i >= 0; i--) {
		readAndSkipPast2("CostRoutine_HIVpos_onART_dmed", CD4_STRATA_STRS[i], inputFile);
		for (j = 0; j < GENDER_NUM; j++) {
			for (k = 0; k < CHRM_AGE_CAT_NUM; k++) {
				fscanf(inputFile, "%lf", &(costInputs.routineCareCostHIVPositive[ART_ON_STATE][i][j][k][COST_DIR_MED]));
			}
		}
		readAndSkipPast2("CostRoutine_HIVpos_onART_nmed", CD4_STRATA_STRS[i], inputFile);
		for (j = 0; j < GENDER_NUM; j++) {
			for (k = 0; k < CHRM_AGE_CAT_NUM; k++) {
				fscanf(inputFile, "%lf", &(costInputs.routineCareCostHIVPositive[ART_ON_STATE][i][j][k][COST_DIR_NONMED]));
			}
		}
		readAndSkipPast2("CostRoutine_HIVpos_onART_time", CD4_STRATA_STRS[i], inputFile);
		for (j = 0; j < GENDER_NUM; j++) {
			for (k = 0; k < CHRM_AGE_CAT_NUM; k++) {
				fscanf(inputFile, "%lf", &(costInputs.routineCareCostHIVPositive[ART_ON_STATE][i][j][k][COST_TIME]));
			}
		}
		readAndSkipPast2("CostRoutine_HIVpos_onART_indr", CD4_STRATA_STRS[i], inputFile);
		for (j = 0; j < GENDER_NUM; j++) {
			for (k = 0; k < CHRM_AGE_CAT_NUM; k++) {
				fscanf(inputFile, "%lf", &(costInputs.routineCareCostHIVPositive[ART_ON_STATE][i][j][k][COST_INDIR]));
			}
		}
	}

	// read in contact/clinic vist costs
	for ( i = 0; i < GENDER_NUM; ++i) {
		for ( j = 0; j < AGE_CATEGORIES; ++j ) {
			sprintf( scratch, "CostVisit_%s", GENDER_STRS[i]);
			readAndSkipPast( scratch, inputFile );
			sprintf( scratch, "Age%d", j );
			readAndSkipPast( scratch, inputFile );
			for (k = 0; k < COST_NUM_TYPES; k++) {
				fscanf(inputFile, "%lf", &(costInputs.clinicVisitCost[i][j][k]) );
			}
		}
	}
} /* end readCostInputs */

/* readTBInputs reads data from the TB tab of the input sheet */
void SimContext::readTBInputs() {
	char buffer[256], tmpBuf[256];
	int i, j, k, tempBool;
	FILE *file = inputFile;

	// read in TB history and distribution at entry
	for (i = CD4_NUM_STRATA - 1; i >= 0; --i) {
		readAndSkipPast( "ProbTB_Entry", file );
		readAndSkipPast(CD4_STRATA_STRS[i], file);
		fscanf( file, "%lf %lf %lf", &(tbInputs.distributionTBStateAtEntry[i][TB_STATE_LATENT]),
				&(tbInputs.distributionTBStateAtEntry[i][TB_STATE_ACTIVE]),
				&(tbInputs.distributionTBStateAtEntry[i][TB_STATE_HIST_ACTV]));
		tbInputs.distributionTBStateAtEntry[i][TB_STATE_TREATM_SUCC] = 0.0;
	}
	readAndSkipPast( "DistTB_Entry_Latent", file );
	fscanf( file, "%lf %lf", &(tbInputs.distributionTBStrainAtEntry[TB_STRAIN_MDR][TB_STATE_LATENT]),
			&(tbInputs.distributionTBStrainAtEntry[TB_STRAIN_XDR][TB_STATE_LATENT]));
	tbInputs.distributionTBStrainAtEntry[TB_STRAIN_DS][TB_STATE_LATENT] = 1 - tbInputs.distributionTBStrainAtEntry[TB_STRAIN_MDR][TB_STATE_LATENT] - tbInputs.distributionTBStrainAtEntry[TB_STRAIN_XDR][TB_STATE_LATENT];
	readAndSkipPast( "DistTB_Entry_Active", file );
	fscanf( file, "%lf %lf", &(tbInputs.distributionTBStrainAtEntry[TB_STRAIN_MDR][TB_STATE_ACTIVE]),
			&(tbInputs.distributionTBStrainAtEntry[TB_STRAIN_XDR][TB_STATE_ACTIVE]));
	tbInputs.distributionTBStrainAtEntry[TB_STRAIN_DS][TB_STATE_ACTIVE] = 1 - tbInputs.distributionTBStrainAtEntry[TB_STRAIN_MDR][TB_STATE_ACTIVE] - tbInputs.distributionTBStrainAtEntry[TB_STRAIN_XDR][TB_STATE_ACTIVE];
	readAndSkipPast( "DistTB_Entry_HistActive", file );
	fscanf( file, "%lf %lf", &(tbInputs.distributionTBStrainAtEntry[TB_STRAIN_MDR][TB_STATE_HIST_ACTV]),
			&(tbInputs.distributionTBStrainAtEntry[TB_STRAIN_XDR][TB_STATE_HIST_ACTV]));
	tbInputs.distributionTBStrainAtEntry[TB_STRAIN_DS][TB_STATE_HIST_ACTV] = 1 - tbInputs.distributionTBStrainAtEntry[TB_STRAIN_MDR][TB_STATE_HIST_ACTV] - tbInputs.distributionTBStrainAtEntry[TB_STRAIN_XDR][TB_STATE_HIST_ACTV];
	readAndSkipPast( "PropEarlyLatentTB_Entry", file );
	fscanf( file, "%lf", &tbInputs.percentLatentTBIsEarly);
	readAndSkipPast( "MthsSinceInfEarlyLatentTB_Entry", file );
	fscanf( file, "%lf %lf", &tbInputs.monthsSinceEarlyLatentMean, &tbInputs.monthsSinceEarlyLatentStdDev );
	readAndSkipPast( "MthsSinceInfLateLatentTB_Entry", file );
	fscanf( file, "%lf %lf", &tbInputs.monthsSinceLateLatentMean, &tbInputs.monthsSinceLateLatentStdDev );
	for (i = 0; i < TB_NUM_STRAINS; ++i) {
		readAndSkipPast2( "MthsInfNotOnTreatActvTB_Entry", TB_STRAIN_STRS[i], file );
		fscanf( file, "%lf %lf", &(tbInputs.monthsInfectedNotTreatedMean[i]),
			&(tbInputs.monthsInfectedNotTreatedStdDev[i]) );
	}

	// nat hist for no TB history
	readAndSkipPast( "ProbMthTBIncid_NoHist_OffART", file );
	for (i = CD4_NUM_STRATA - 1; i >= 0; --i)
		fscanf( file, "%lf", &(tbInputs.probInfectionNoHistoryOffART[i]));
	readAndSkipPast( "MthPerTBIncid_NoHist_OnART", file );
	for (i = 0; i < TB_MTH_PERIODS_NUM; ++i)
		fscanf( file, "%d", &(tbInputs.multiplierInfectionStageBoundsNoHistoryOnART[i]));
	for (i = CD4_NUM_STRATA - 1; i >= 0; --i) {
		readAndSkipPast( "ProbMthTBIncid_NoHist_OnART", file );
		readAndSkipPast( CD4_STRATA_STRS[i], file );
		for (j = 0; j < TB_MTH_PERIODS_NUM; ++j)
			fscanf( file, "%lf", &(tbInputs.multiplierInfectionNoHistoryOnART[i][j]));
	}
	readAndSkipPast( "DistActiveTBIncid_NoHist", file );
	fscanf( file, "%lf", &tbInputs.probActiveInfectionNoHistory);
	readAndSkipPast( "DistIncidTBType_NoHist", file );
	fscanf( file, "%lf %lf", &(tbInputs.distributionInfectionStrainNoHistory[TB_STRAIN_MDR]), &(tbInputs.distributionInfectionStrainNoHistory[TB_STRAIN_XDR]));
	tbInputs.distributionInfectionStrainNoHistory[TB_STRAIN_DS] = 1 - tbInputs.distributionInfectionStrainNoHistory[TB_STRAIN_MDR] - tbInputs.distributionInfectionStrainNoHistory[TB_STRAIN_XDR];

	// nat hist for latent TB
	readAndSkipPast( "MthPerTBReactiv_Latent_OffART", file );
	fscanf( file, "%d %d", &(tbInputs.probReactivationStageBoundsLatentOffART[0]),
		&(tbInputs.probReactivationStageBoundsLatentOffART[1]));
	for (i = CD4_NUM_STRATA - 1; i >= 0; --i) {
		readAndSkipPast( "ProbMthTBReactiv_Latent_OffART", file );
		readAndSkipPast( CD4_STRATA_STRS[i], file );
		for (j = 0; j < TB_NUM_STRAINS; ++j)
			fscanf( file, "%lf %lf", &(tbInputs.probReactivationLatentOffART[i][j][0]),
			&(tbInputs.probReactivationLatentOffART[i][j][1]));
	}
	readAndSkipPast( "MthPerTBReactiv_Latent_OnART", file );
	for (i = 0; i < TB_MTH_PERIODS_NUM; ++i)
		fscanf( file, "%d", &(tbInputs.multiplierReactivationStageBoundsLatentOnART[i]));
	for (i = CD4_NUM_STRATA - 1; i >= 0; --i) {
		readAndSkipPast( "MultMthTBReactiv_Latent_OnART", file );
		readAndSkipPast( CD4_STRATA_STRS[i], file );
		for (j = 0; j < TB_MTH_PERIODS_NUM; ++j)
			fscanf( file, "%lf", &(tbInputs.multiplierReactivationLatentOnART[i][j]));
	}
	readAndSkipPast( "ProbMthTBReinfect_Latent_OffART", file );
	for (i = CD4_NUM_STRATA - 1; i >= 0; --i)
		fscanf( file, "%lf", &(tbInputs.probReinfectionLatentOffART[i]));
	readAndSkipPast( "MthPerTBReinfect_Latent_OnART", file );
	for (i = 0; i < TB_MTH_PERIODS_NUM; ++i)
		fscanf( file, "%d", &(tbInputs.multiplierReinfectionStageBoundsLatentOnART[i]));
	for (i = CD4_NUM_STRATA - 1; i >= 0; --i) {
		readAndSkipPast( "MultMthTBReinfect_Latent_OnART", file );
		readAndSkipPast( CD4_STRATA_STRS[i], file );
		for (j = 0; j < TB_MTH_PERIODS_NUM; ++j)
			fscanf( file, "%lf", &(tbInputs.multiplierReinfectionLatentOnART[i][j]));
	}
	readAndSkipPast( "DistActiveTBReinfect_Latent", file );
	fscanf( file, "%lf", &tbInputs.probActiveReinfectionLatent);
	readAndSkipPast( "DistReinfectTBType_Latent", file );
	fscanf( file, "%lf %lf", &(tbInputs.distributionReinfectionStrainLatent[TB_STRAIN_MDR]), &(tbInputs.distributionReinfectionStrainLatent[TB_STRAIN_XDR]));
	tbInputs.distributionReinfectionStrainLatent[TB_STRAIN_DS] = 1 - tbInputs.distributionReinfectionStrainLatent[TB_STRAIN_MDR] - tbInputs.distributionReinfectionStrainLatent[TB_STRAIN_XDR];
	readAndSkipPast( "ReinfectionSupercedeOption", file );
	fscanf( file, "%d", &tbInputs.reinfectionSupercedeOption);

	// nat hist for history of active TB
	readAndSkipPast( "ProbMthTBRelapse_HistActv_OffART", file );
	for (i = CD4_NUM_STRATA - 1; i >= 0; --i)
		fscanf( file, "%lf", &(tbInputs.probRelapseHistoryActiveOffART[i]));
	readAndSkipPast( "MthPerTBRelapse_HistActv_OnART", file );
	for (i = 0; i < TB_MTH_PERIODS_NUM; ++i)
		fscanf( file, "%d", &(tbInputs.multiplierRelapseStageBoundsHistoryActiveOnART[i]));
	for (i = CD4_NUM_STRATA - 1; i >= 0; --i) {
		readAndSkipPast( "ProbMthTBRelapse_HistActv_OnART", file );
		readAndSkipPast( CD4_STRATA_STRS[i], file );
		for (j = 0; j < TB_MTH_PERIODS_NUM; ++j)
			fscanf( file, "%lf", &(tbInputs.multiplierRelapseHistoryActiveOnART[i][j]));
	}

	// nat hist for active TB
	for (i = 0; i < TB_NUM_STRAINS; ++i) {
		sprintf(tmpBuf, "MthPerTBResol_Active_%s", TB_STRAIN_STRS[i]);
        readAndSkipPast( tmpBuf, file );
		for (j = 0; j < TB_MTH_PERIODS_NUM; ++j)
			fscanf( file, "%d", &(tbInputs.probSpontaneousResolutionStageBounds[i][j]));
		sprintf(tmpBuf, "MthProbTBResol_Active_%s", TB_STRAIN_STRS[i]);
		for (j = CD4_NUM_STRATA - 1; j >= 0; --j) {
	        readAndSkipPast( tmpBuf, file );
	        readAndSkipPast( CD4_STRATA_STRS[j], file );
			for (k = 0; k <= TB_MTH_PERIODS_NUM+1; ++k)
				fscanf( file, "%lf", &(tbInputs.probSpontaneousResolution[i][j][k]));
		}
	}
	readAndSkipPast( "AcuteTBMortProb", file );
	for (i = CD4_NUM_STRATA - 1; i >= 0; --i)
		fscanf( file, "%lf", &(tbInputs.probAcuteMortality[i]));
	for (i = 0; i < TB_NUM_STRAINS; ++i) {
		sprintf(tmpBuf, "MthPerTBExtMort_Active_%s", TB_STRAIN_STRS[i]);
        readAndSkipPast( tmpBuf, file );
		for (j = 0; j < TB_MTH_PERIODS_NUM; ++j)
			fscanf( file, "%d", &(tbInputs.probExtendedMortalityStageBoundsOffART[i][j]));
		sprintf(tmpBuf, "MthProbTBExtMort_Active_%s", TB_STRAIN_STRS[i]);
		for (j = CD4_NUM_STRATA - 1; j >= 0; --j) {
	        readAndSkipPast( tmpBuf, file );
	        readAndSkipPast( CD4_STRATA_STRS[j], file );
			for (k = 0; k < TB_MTH_PERIODS_NUM+1; ++k)
				fscanf( file, "%lf", &(tbInputs.probExtendedMortalityOffART[i][j][k]));
		}
	}
    readAndSkipPast( "MultProbTBExtMort_Active_OnART", file );
	for (i = CD4_NUM_STRATA - 1; i >= 0; --i)
		fscanf( file, "%lf", &(tbInputs.multiplierExtendedMortalityOnART[i]));

	// TB treatment
	readAndSkipPast( "ProbRecvTBTreatm_OffART", file );
	fscanf( file, "%lf", &tbInputs.probReceiveTreatmentOffART);
    readAndSkipPast( "ProbRecvTBTreatm_OnART", file );
	fscanf( file, "%lf", &tbInputs.probReceiveTreatmentOnART);
    readAndSkipPast( "MthsLagTBTreatm_Mean", file );
	for (i = 0; i < TB_TREATM_STAGE_NUM; ++i)
		fscanf( file, "%lf", &(tbInputs.monthsLagToStartTreatmentMean[i]));
    readAndSkipPast( "MthsLagTBTreatm_SDev", file );
	for (i = 0; i < TB_TREATM_STAGE_NUM; ++i)
		fscanf( file, "%lf", &(tbInputs.monthsLagToStartTreatmentStdDev[i]));
    readAndSkipPast( "MthsDurTBTreatm", file );
	for (i = 0; i < TB_TREATM_STAGE_NUM; ++i)
		fscanf( file, "%d", &(tbInputs.monthsTreatmentDuration[i]));
    readAndSkipPast( "ProbDropoutTBTreatm", file );
	for (i = 0; i < TB_TREATM_STAGE_NUM; ++i)
		fscanf( file, "%lf", &(tbInputs.probTreatmentDropout[i]));
	for (i = 0; i < TB_NUM_STRAINS; ++i) {
		sprintf(tmpBuf, "ProbRecvLine_InitTBTreatm_%s", TB_STRAIN_STRS[i]);
        readAndSkipPast( tmpBuf, file );
		for (j = 0; j < TB_TREATM_LINES_NUM; ++j)
			fscanf( file, "%lf", &(tbInputs.probInitialTreatmentLine[i][j]));
	}
	for (i = 0; i < TB_TREATM_STAGE_NUM; ++i) {
		sprintf(tmpBuf, "ProbRecvLine_RetreatTB_%s", TB_TREATM_STAGE_STRS[i]);
        readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf", &(tbInputs.probRepeatTreatmentAfterFailure[i]));
		fscanf( file, "%lf", &(tbInputs.probNextTreatmentAfterFailure[i]));
		fscanf( file, "%lf", &(tbInputs.probSkipTreatmentAfterFailure[i]));
		tbInputs.probDiscontinueTreatmentAfterFailure[i] = 1.0 - tbInputs.probRepeatTreatmentAfterFailure[i]
			- tbInputs.probNextTreatmentAfterFailure[i] - tbInputs.probSkipTreatmentAfterFailure[i];
	}
	for (i = 0; i < TB_NUM_STRAINS; ++i) {
		sprintf(tmpBuf, "ProbTBTreatm_Cure_%s", TB_STRAIN_STRS[i]);
        readAndSkipPast( tmpBuf, file );
		for (j = 0; j < TB_TREATM_STAGE_NUM; ++j)
			fscanf( file, "%lf", &(tbInputs.probCuredAfterTreatment[i][j]));
	}
    readAndSkipPast( "ProbTBTreatmIncrResist", file );
	for (i = 0; i < TB_TREATM_STAGE_NUM; ++i)
		fscanf( file, "%lf", &(tbInputs.probIncreasedResistanceNotCured[i]));

	// TB Costs
	readAndSkipPast( "Costs_AcuteTB_Untreated", file );
    fscanf( file, "%lf %lf %lf %lf", &(tbInputs.acuteUntreatedCosts[0]), &(tbInputs.acuteUntreatedCosts[1]),
		&(tbInputs.acuteUntreatedCosts[2]), &(tbInputs.acuteUntreatedCosts[3]));
	for (i = 0; i < TB_TREATM_STAGE_NUM; ++i) {
	    readAndSkipPast( "Costs_TB_Treated", file );
		sprintf(tmpBuf, "Acute_%s", TB_TREATM_STAGE_STRS[i]);
        readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf %lf %lf %lf", &(tbInputs.acuteActiveTreatedCosts[i][0]),
			&(tbInputs.acuteActiveTreatedCosts[i][1]),
			&(tbInputs.acuteActiveTreatedCosts[i][2]),
			&(tbInputs.acuteActiveTreatedCosts[i][3]),
			&(tbInputs.acuteActiveTreatedARTMultiplier[i]));
		readAndSkipPast( "Costs_TB_Treated", file );
		sprintf(tmpBuf, "Mthly_%s", TB_TREATM_STAGE_STRS[i]);
        readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf %lf %lf %lf", &(tbInputs.monthlyTreatedCosts[i][0]),
			&(tbInputs.monthlyTreatedCosts[i][1]),
			&(tbInputs.monthlyTreatedCosts[i][2]),
			&(tbInputs.monthlyTreatedCosts[i][3]),
			&(tbInputs.monthlyTreatedARTMultiplier[i]));
	}
	for (i = 0; i < TB_TREATM_STAGE_NUM; ++i) {
	    readAndSkipPast( "TBTreatmToxicity_CostsQol", file );
	    readAndSkipPast( TB_TREATM_STAGE_STRS[i], file );
		fscanf( file, "%lf", &(tbInputs.costTreatmentMinorToxicity[i]));
		fscanf( file, "%lf", &(tbInputs.QOLMultiplierTreatmentMinorToxicity[i]));
		fscanf( file, "%lf", &(tbInputs.costTreatmentMajorToxicity[i]));
		fscanf( file, "%lf", &(tbInputs.QOLMultiplierTreatmentMajorToxicity[i]));
	}
	for (i = 0; i < TB_TREATM_STAGE_NUM; ++i) {
	    readAndSkipPast( "TBTreatmToxicity_Prob", file );
	    readAndSkipPast( TB_TREATM_STAGE_STRS[i], file );
		fscanf( file, "%lf", &(tbInputs.probTreatmentMinorToxicity[i]));
		fscanf( file, "%lf", &(tbInputs.probTreatmentMinorToxicityARTMultiplier[i]));
		fscanf( file, "%lf", &(tbInputs.probTreatmentMajorToxicity[i]));
		fscanf( file, "%lf", &(tbInputs.probTreatmentMajorToxicityARTMultiplier[i]));
		fscanf( file, "%d", &(tbInputs.monthsToTreatmentToxicity[i]));
		fscanf( file, "%lf", &(tbInputs.probDeathTreatmentMajorToxicity[i]));
	}

    // prophylaxis policy for TB
    readAndSkipPast( "TBProphStart_boolFlag", file );
	fscanf( file, "%d", &tempBool);
	tbInputs.startProphUseOrEvaluation = (bool) tempBool;
    readAndSkipPast( "TBProphStart_curCD4", file );
	fscanf( file, "%lf %lf", &(tbInputs.startProphCurrentCD4Bounds[LOWER_BOUND]),
		&(tbInputs.startProphCurrentCD4Bounds[UPPER_BOUND]));
    readAndSkipPast( "TBProphStart_minCD4", file );
	fscanf( file, "%lf %lf", &(tbInputs.startProphMinCD4Bounds[LOWER_BOUND]),
		&(tbInputs.startProphMinCD4Bounds[UPPER_BOUND]));
	readAndSkipPast( "TBProphStart_knownHistAct", file);
	fscanf( file, "%d", &tbInputs.startProphKnownActiveHistory);
	readAndSkipPast( "TBProphStart_atARTStart", file);
	fscanf( file, "%d", &tbInputs.startProphAtARTInitiation);
	readAndSkipPast( "TBProphStart_propToReceiveUponQual", file);
	fscanf(file, "%lf %lf", &tbInputs.probReceiveProphOffART, &tbInputs.probReceiveProphOnART);
	readAndSkipPast( "TBProphStart_lagToStartUponQual", file);
	fscanf(file, "%lf %lf", &tbInputs.monthsLagToStartProphMean, &tbInputs.monthsLagToStartProphStdDev);
	readAndSkipPast( "TBProphStop_mthDropoutProb", file);
	fscanf(file, "%lf", &tbInputs.probDropoffProph);
    readAndSkipPast( "TBProphStop_boolFlag", file );
	fscanf( file, "%d", &tempBool);
	tbInputs.startProphUseOrEvaluation = (bool) tempBool;
    readAndSkipPast( "TBProphStop_curCD4", file );
	fscanf( file, "%lf %lf", &(tbInputs.stopProphCurrentCD4Bounds[LOWER_BOUND]),
		&(tbInputs.stopProphCurrentCD4Bounds[UPPER_BOUND]));
	readAndSkipPast( "TBProphStop_atARTStart", file );
	fscanf( file, "%d", &tbInputs.stopProphAtARTInitiation);
	readAndSkipPast( "TBProphStop_mthOnProph", file );
	fscanf( file, "%d", &tbInputs.stopProphNumMonths);

	//prophylaxis efficacy and cost for TB
	int prophId;
	for (i = 0; i < PROPH_NUM; ++i) {
		// Skip this proph if it is not a real regimen
		sprintf(tmpBuf, "TBProph%d_Id", i + 1);
        readAndSkipPast( tmpBuf, file );
        fscanf(file, "%d", &prophId);
        if (prophId == SimContext::NOT_APPL) {
        	tbInputs.tbProphInputs[i] = NULL;
        	continue;
        }

        // Allocate space for the regimen inputs
        tbInputs.tbProphInputs[i] = new TBInputs::TBProph();

		sprintf(tmpBuf, "TBProph%d_eff_noHist", i + 1);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf", &(tbInputs.tbProphInputs[i]->efficacyNoHistory) );
		sprintf(tmpBuf, "TBProph%d_eff_reactiv", i + 1);
        readAndSkipPast( tmpBuf, file );
		for (j = 0; j < TB_NUM_STRAINS; ++j)
			fscanf( file, "%lf", &(tbInputs.tbProphInputs[i]->efficacyReactivation[j]) );
		sprintf(tmpBuf, "TBProph%d_eff_reinfct", i + 1);
        readAndSkipPast( tmpBuf, file );
		for (j = 0; j < TB_NUM_STRAINS; ++j)
			fscanf( file, "%lf", &(tbInputs.tbProphInputs[i]->efficacyReinfection[j]) );
		sprintf(tmpBuf, "TBProph%d_costQol", i + 1);
        readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf %lf %lf %lf", &(tbInputs.tbProphInputs[i]->costMonthly),
			&(tbInputs.tbProphInputs[i]->costMinorToxicity),
			&(tbInputs.tbProphInputs[i]->QOLMultiplierMinorToxicity),
			&(tbInputs.tbProphInputs[i]->costMajorToxicity),
			&(tbInputs.tbProphInputs[i]->QOLMultiplierMajorToxicity) );
		sprintf(tmpBuf, "TBProph%d_toxProb", i + 1);
        readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf %lf %lf %d %lf", &(tbInputs.tbProphInputs[i]->probMinorToxicity),
			&(tbInputs.tbProphInputs[i]->probMinorToxicityARTMultiplier),
			&(tbInputs.tbProphInputs[i]->probMajorToxicity),
			&(tbInputs.tbProphInputs[i]->probMajorToxicityARTMultiplier),
			&(tbInputs.tbProphInputs[i]->monthsToToxicity),
			&(tbInputs.tbProphInputs[i]->probDeathMajorToxicity));
		sprintf(tmpBuf, "TBProph%d_resistProb", i + 1);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf", &(tbInputs.tbProphInputs[i]->probIncreasedResistanceFailure) );
	}
} /* end readTBInputs */

/* readQOLInputs reads data from the QOL tab of the input sheet */
void SimContext::readQOLInputs() {
	int i, j;

	// read in routine QOL
	for ( i = CD4_NUM_STRATA - 1; i >= 0; --i ) {
		readAndSkipPast( "QOLRoutine", inputFile );
		readAndSkipPast( CD4_STRATA_STRS[i], inputFile );
		fscanf( inputFile, "%lf", &(qolInputs.routineCareQOL[i][OI_NUM]) );
		for ( j = 0; j < OI_NUM; ++j )
			fscanf( inputFile, "%lf", &(qolInputs.routineCareQOL[i][j]) );
	}

	// read in acute OI QOL
	readAndSkipPast( "QOLAcuteOI", inputFile );
	for ( i = 0; i < OI_NUM; ++i )
		fscanf( inputFile, "%lf", &(qolInputs.acuteOIQOL[i]) );
	// read in death QOL
	readAndSkipPast( "QOLDeath", inputFile );
	for ( i = 0; i < DTH_NUM_CAUSES_BASIC; ++i )
		fscanf( inputFile, "%lf", &(qolInputs.deathBasicQOL[i]) );

	// read in non-AIDS background QOL
	readAndSkipPast( "QOLBackgroundMale", inputFile );
	for (i = 0; i < AGE_YRS; i++) {
		fscanf(inputFile, "%lf", &(qolInputs.nonAIDSBackgroundQOL[GENDER_MALE][i]));
	}
	readAndSkipPast( "QOLBackgroundFemale", inputFile );
	for (i = 0; i < AGE_YRS; i++) {
		fscanf(inputFile, "%lf", &(qolInputs.nonAIDSBackgroundQOL[GENDER_FEMALE][i]));
	}
} /* end readQOLInputs */

/* readHIVTestInputs reads data from the HIVTest tab of the input sheet */
void SimContext::readHIVTestInputs() {
	char buffer[256];
	int i, j, k, tempBool;

	// read in enable HIV testing module
	readAndSkipPast( "EnableHIVtest", inputFile );
	fscanf( inputFile, "%d", &tempBool);
	testingInputs.enableHIVTesting = (bool) tempBool;
	// read in HIV test available
	readAndSkipPast( "HIVtestAvail", inputFile );
	fscanf( inputFile, "%d", &tempBool);
	testingInputs.HIVTestAvailable = (bool) tempBool;
	// read in whether to use alt HIV+ stopping rule
	readAndSkipPast( "AltStopRuleEnable", inputFile );
	fscanf( inputFile, "%d", &tempBool);
	testingInputs.useAlternateStoppingRule = (bool) tempBool;
	if (testingInputs.enableHIVTesting != true)
		testingInputs.useAlternateStoppingRule = false;
	readAndSkipPast( "AltStopRuleTotHIV", inputFile );
	fscanf( inputFile, "%ld", &testingInputs.totalCohortsWithHIVPositiveLimit );
	readAndSkipPast( "AltStopRuleTotCohort", inputFile );
	fscanf( inputFile, "%ld", &testingInputs.totalCohortsWithoutHIVPositiveLimit );

	// read in distribution of HIV patient states
	readAndSkipPast( "HIVdistNegLow", inputFile );
	fscanf( inputFile, "%lf", &(testingInputs.initialHIVDistribution[HIV_EXT_INF_NEG_LO]));
	readAndSkipPast( "HIVdistNegHigh", inputFile );
	fscanf( inputFile, "%lf", &(testingInputs.initialHIVDistribution[HIV_EXT_INF_NEG_HI]) );
	readAndSkipPast( "HIVdistPosAcute", inputFile );
	fscanf( inputFile, "%lf", &(testingInputs.initialHIVDistribution[HIV_EXT_INF_ACUTE_SYN]) );
	readAndSkipPast( "HIVdistPosChr", inputFile );
	fscanf( inputFile, "%lf", &(testingInputs.initialHIVDistribution[HIV_EXT_INF_ASYMP_CHR_POS]) );
	testingInputs.initialHIVDistribution[HIV_EXT_INF_SYMP_CHR_POS] = 0.0;

	// read in distribution of inital CD4s and HVLs for acute HIV infections
	readAndSkipPast( "HIVacuteCD4dist", inputFile );
	fscanf( inputFile, "%lf %lf", &(testingInputs.initialAcuteCD4DistributionMean),
		&(testingInputs.initialAcuteCD4DistributionStdDev) );
	readAndSkipPast( "HIVacuteHVLdist", inputFile );
	for ( k = HVL_NUM_STRATA - 1; k >= 0; --k ) {
		fscanf( inputFile, "%lf", &(testingInputs.initialAcuteHVLDistribution[0][k]) );
		for (i = 1; i < CD4_NUM_STRATA; ++i)
			testingInputs.initialAcuteHVLDistribution[i][k] = testingInputs.initialAcuteHVLDistribution[0][k];
	}

	// read in monthly HIV infection rate for HIV neg's
	readAndSkipPast( "HIVmthIncid", inputFile );
	readAndSkipPast( "hiRisk", inputFile );
	for (i = 0; i < AGE_CAT_TEST; ++i) {
		fscanf( inputFile, "%lf", &(testingInputs.probHIVInfection[i][HIV_BEHAV_HI]));
	}
	readAndSkipPast( "HIVmthIncid", inputFile );
	readAndSkipPast( "loRisk", inputFile );
	for (i = 0; i < AGE_CAT_TEST; ++i) {
		fscanf( inputFile, "%lf", &(testingInputs.probHIVInfection[i][HIV_BEHAV_LO]));
	}

	// read in probability of being initially detected as HIV positive upon model entry
	readAndSkipPast( "HIVdetectAcute", inputFile );
	fscanf( inputFile, "%lf", &(testingInputs.probHIVDetectionInitial[HIV_INF_ACUTE_SYN]));
	readAndSkipPast( "HIVdetectAsympChr", inputFile );
	fscanf( inputFile, "%lf", &(testingInputs.probHIVDetectionInitial[HIV_INF_ASYMP_CHR_POS]));
	readAndSkipPast( "HIVdetectSympChr", inputFile );
	fscanf( inputFile, "%lf", &(testingInputs.probHIVDetectionInitial[HIV_INF_SYMP_CHR_POS]));
	testingInputs.probHIVDetectionInitial[HIV_INF_NEG] = 0.0;

	// read in probability that someone who hasn't been detected gets detected when they get a particular OI
	readAndSkipPast( "HIVProbDetAtOI", inputFile );
	for (i = 0; i < OI_NUM; ++i) {
		fscanf( inputFile, "%lf", &(testingInputs.probHIVDetectionWithOI[i]) );
	}

	// read in frequency of HIV screening
	readAndSkipPast( "HIVtestFreqInterval", inputFile );
	for (i = 0; i < HIV_TEST_FREQ_NUM; ++i)
		fscanf( inputFile, "%d", &(testingInputs.HIVTestingInterval[i]) );
	readAndSkipPast( "HIVtestFreqProb", inputFile );
	for (i = 0; i < HIV_TEST_FREQ_NUM; ++i)
		fscanf( inputFile, "%lf", &(testingInputs.HIVTestingProbability[i]) );

	// read in costs for special HIV- and undet HIV+ states
	readAndSkipPast( "HIVnegDthCost", inputFile );
	fscanf( inputFile, "%lf", &(testingInputs.deathCostHIVNegative) );
	readAndSkipPast( "HIVundetCD4Cost", inputFile );
	for (i = CD4_NUM_STRATA - 1; i >= 0; --i)
		fscanf( inputFile, "%lf", &(testingInputs.monthCostHIVUndetected[i]) );
	readAndSkipPast( "HIVundetChrDthCost", inputFile );
	fscanf( inputFile, "%lf", &(testingInputs.chronicDeathCostHIVUndetected) );
	readAndSkipPast( "HIVundetNonDthCost", inputFile );
	fscanf( inputFile, "%lf", &(testingInputs.nonAIDSDeathCostHIVUndetected) );
	// read in QOLs for special HIV- and undet HIV+ states
	readAndSkipPast( "HIVnegDthQOL", inputFile );
	fscanf( inputFile, "%lf", &(testingInputs.deathQOLHIVNegative) );
	readAndSkipPast( "HIVundetCD4QOL", inputFile );
	for (i = CD4_NUM_STRATA - 1; i >= 0; --i)
		fscanf( inputFile, "%lf", &(testingInputs.monthQOLHIVUndetected[i]) );
	readAndSkipPast( "HIVundetChrDthQOL", inputFile );
	fscanf( inputFile, "%lf", &(testingInputs.chronicDeathQOLHIVUndetected) );
	readAndSkipPast( "HIVundetNonDthQOL", inputFile );
	fscanf( inputFile, "%lf", &(testingInputs.nonAIDSDeathQOLHIVUndetected) );

	// read in number of mths from acute HIV to chronic HIV state
	readAndSkipPast( "MthsAcuteToChrHIV", inputFile );
	fscanf( inputFile, "%d", &(testingInputs.monthsFromAcuteToChronic) );
	// read in CD4 transitions from HIV acute to chronic
	readAndSkipPast( "MeanCD4ChgAtChrHIVTrans", inputFile );
	for (i = HVL_NUM_STRATA - 1; i >= 0; --i)
		fscanf( inputFile, "%lf", &(testingInputs.CD4ChangeAtChronicHIVMean[i]) );
	readAndSkipPast( "SDevCD4ChgAtChrHIVTrans", inputFile );
	for (i = HVL_NUM_STRATA - 1; i >= 0; --i)
		fscanf( inputFile, "%lf", &(testingInputs.CD4ChangeAtChronicHIVStdDev[i]) );
	// read in HVL transitions from HIV acute to chronic
	for (i = HVL_NUM_STRATA - 1; i >= 0; --i) {
		readAndSkipPast( "HVLDistribAtChrHIVTrans", inputFile );
		readAndSkipPast( HVL_STRATA_STRS[i], inputFile );
		for ( j = HVL_NUM_STRATA - 1; j >= 0; --j )
			fscanf( inputFile, "%lf", &(testingInputs.HVLDistributionAtChronicHIV[i][j]) );
	}

	// read in enrollment characteristics of HIV screening program
	for (j = 0; j < TEST_ACCEPT_NUM; ++j) {
		sprintf( buffer, "HIVtestAcceptDist%d", j+1 );
		readAndSkipPast( buffer, inputFile );
		fscanf( inputFile, "%lf %lf %lf %lf %lf",
			&(testingInputs.HIVTestAcceptDistribution[HIV_EXT_INF_NEG_HI][j]),
			&(testingInputs.HIVTestAcceptDistribution[HIV_EXT_INF_NEG_LO][j]),
			&(testingInputs.HIVTestAcceptDistribution[HIV_EXT_INF_ASYMP_CHR_POS][j]),
			&(testingInputs.HIVTestAcceptDistribution[HIV_EXT_INF_SYMP_CHR_POS][j]),
			&(testingInputs.HIVTestAcceptDistribution[HIV_EXT_INF_ACUTE_SYN][j]) );
	}
	for (j = 0; j < TEST_ACCEPT_NUM; ++j) {
		sprintf( buffer, "HIVtestAcceptRate%d", j+1 );
		readAndSkipPast( buffer, inputFile );
		fscanf( inputFile, "%lf %lf %lf %lf %lf",
			&(testingInputs.HIVTestAcceptRate[HIV_EXT_INF_NEG_HI][j]),
			&(testingInputs.HIVTestAcceptRate[HIV_EXT_INF_NEG_LO][j]),
			&(testingInputs.HIVTestAcceptRate[HIV_EXT_INF_ASYMP_CHR_POS][j]),
			&(testingInputs.HIVTestAcceptRate[HIV_EXT_INF_SYMP_CHR_POS][j]),
			&(testingInputs.HIVTestAcceptRate[HIV_EXT_INF_ACUTE_SYN][j]) );
	}

	// read in HIV testing program costs
	readAndSkipPast( "HIVtestStartupCost", inputFile );
	fscanf( inputFile, "%lf %lf %lf %lf %lf",
		&(testingInputs.HIVTestInitialCost[HIV_EXT_INF_NEG_HI]),
		&(testingInputs.HIVTestInitialCost[HIV_EXT_INF_NEG_LO]),
		&(testingInputs.HIVTestInitialCost[HIV_EXT_INF_ASYMP_CHR_POS]),
		&(testingInputs.HIVTestInitialCost[HIV_EXT_INF_SYMP_CHR_POS]),
		&(testingInputs.HIVTestInitialCost[HIV_EXT_INF_ACUTE_SYN]) );
	readAndSkipPast( "HIVtestNonRetCost", inputFile );
	fscanf( inputFile, "%lf %lf %lf %lf %lf",
		&(testingInputs.HIVTestNonReturnCost[HIV_EXT_INF_NEG_HI]),
		&(testingInputs.HIVTestNonReturnCost[HIV_EXT_INF_NEG_LO]),
		&(testingInputs.HIVTestNonReturnCost[HIV_EXT_INF_ASYMP_CHR_POS]),
		&(testingInputs.HIVTestNonReturnCost[HIV_EXT_INF_SYMP_CHR_POS]),
		&(testingInputs.HIVTestNonReturnCost[HIV_EXT_INF_ACUTE_SYN]) );
	readAndSkipPast( "HIVtestDetectCost", inputFile );
	fscanf( inputFile, "%lf %lf %lf",
		&(testingInputs.HIVTestDetectionCost[HIV_INF_ASYMP_CHR_POS]),
		&(testingInputs.HIVTestDetectionCost[HIV_INF_SYMP_CHR_POS]),
		&(testingInputs.HIVTestDetectionCost[HIV_INF_ACUTE_SYN]) );
	testingInputs.HIVTestDetectionCost[HIV_INF_NEG] = 0;
	readAndSkipPast( "HIVtestBgDetectRate", inputFile );
	fscanf( inputFile, "%lf %lf %lf",
		&(testingInputs.HIVBackgroundDetectionRate[HIV_INF_ASYMP_CHR_POS]),
		&(testingInputs.HIVBackgroundDetectionRate[HIV_INF_SYMP_CHR_POS]),
		&(testingInputs.HIVBackgroundDetectionRate[HIV_INF_ACUTE_SYN]) );
	testingInputs.HIVBackgroundDetectionRate[HIV_INF_NEG] = 0;
	readAndSkipPast( "HIVtestBgDetectCost", inputFile );
	fscanf( inputFile, "%lf %lf %lf",
		&(testingInputs.HIVBackgroundTestingCost[HIV_INF_ASYMP_CHR_POS]),
		&(testingInputs.HIVBackgroundTestingCost[HIV_INF_SYMP_CHR_POS]),
		&(testingInputs.HIVBackgroundTestingCost[HIV_INF_ACUTE_SYN]) );
	testingInputs.HIVBackgroundTestingCost[HIV_INF_NEG] = 0;

	// read in HIV test characteristics
	readAndSkipPast( "HIVtestRetRate", inputFile );
	fscanf( inputFile, "%lf %lf %lf %lf",
		&(testingInputs.HIVTestReturnRate[HIV_EXT_INF_NEG_HI]),
		&(testingInputs.HIVTestReturnRate[HIV_EXT_INF_ASYMP_CHR_POS]),
		&(testingInputs.HIVTestReturnRate[HIV_EXT_INF_SYMP_CHR_POS]),
		&(testingInputs.HIVTestReturnRate[HIV_EXT_INF_ACUTE_SYN]) );
	testingInputs.HIVTestReturnRate[HIV_EXT_INF_NEG_LO] = testingInputs.HIVTestReturnRate[HIV_EXT_INF_NEG_HI];
	readAndSkipPast( "HIVtestPosRate", inputFile );
	fscanf( inputFile, "%lf %lf %lf %lf",
		&(testingInputs.HIVTestPositiveRate[HIV_INF_NEG]),
		&(testingInputs.HIVTestPositiveRate[HIV_INF_ASYMP_CHR_POS]),
		&(testingInputs.HIVTestPositiveRate[HIV_INF_SYMP_CHR_POS]),
		&(testingInputs.HIVTestPositiveRate[HIV_INF_ACUTE_SYN]) );
	readAndSkipPast( "HIVtestPosCost", inputFile );
	fscanf( inputFile, "%lf %lf %lf %lf",
		&(testingInputs.HIVTestPositiveCost[HIV_INF_NEG]),
		&(testingInputs.HIVTestPositiveCost[HIV_INF_ASYMP_CHR_POS]),
		&(testingInputs.HIVTestPositiveCost[HIV_INF_SYMP_CHR_POS]),
		&(testingInputs.HIVTestPositiveCost[HIV_INF_ACUTE_SYN]) );
	readAndSkipPast( "HIVtestNegCost", inputFile );
	fscanf( inputFile, "%lf %lf %lf %lf",
		&(testingInputs.HIVTestNegativeCost[HIV_INF_NEG]),
		&(testingInputs.HIVTestNegativeCost[HIV_INF_ASYMP_CHR_POS]),
		&(testingInputs.HIVTestNegativeCost[HIV_INF_SYMP_CHR_POS]),
		&(testingInputs.HIVTestNegativeCost[HIV_INF_ACUTE_SYN]) );
	readAndSkipPast( "HIVtestPosQOLMult", inputFile );
	fscanf( inputFile, "%lf %lf %lf %lf",
		&(testingInputs.HIVTestPositiveQOLMultiplier[HIV_INF_NEG]),
		&(testingInputs.HIVTestPositiveQOLMultiplier[HIV_INF_ASYMP_CHR_POS]),
		&(testingInputs.HIVTestPositiveQOLMultiplier[HIV_INF_SYMP_CHR_POS]),
		&(testingInputs.HIVTestPositiveQOLMultiplier[HIV_INF_ACUTE_SYN]) );
	readAndSkipPast( "HIVtestNegQOLMult", inputFile );
	fscanf( inputFile, "%lf %lf %lf %lf",
		&(testingInputs.HIVTestNegativeQOLMultiplier[HIV_INF_NEG]),
		&(testingInputs.HIVTestNegativeQOLMultiplier[HIV_INF_ASYMP_CHR_POS]),
		&(testingInputs.HIVTestNegativeQOLMultiplier[HIV_INF_SYMP_CHR_POS]),
		&(testingInputs.HIVTestNegativeQOLMultiplier[HIV_INF_ACUTE_SYN]) );
	readAndSkipPast( "HIVtestCost", inputFile );
	fscanf( inputFile, "%lf %lf %lf %lf",
		&(testingInputs.HIVTestCost[HIV_EXT_INF_NEG_HI]),
		&(testingInputs.HIVTestCost[HIV_EXT_INF_ASYMP_CHR_POS]),
		&(testingInputs.HIVTestCost[HIV_EXT_INF_SYMP_CHR_POS]),
		&(testingInputs.HIVTestCost[HIV_EXT_INF_ACUTE_SYN]) );
	testingInputs.HIVTestCost[HIV_EXT_INF_NEG_LO] = testingInputs.HIVTestCost[HIV_EXT_INF_NEG_HI];
} /* end readHIVTestInputs */

/* readPedsInputs reads data from the Peds tab of the input sheet */
void SimContext::readPedsInputs() {
	int i, j, k;
	int tempInt;
	char scratch[256];

	// read in enable pediatrics module, initial HIV state, and BF status
	readAndSkipPast("EnablePeds", inputFile);
	fscanf(inputFile, "%d", &tempInt);
	pedsInputs.enablePediatricsModel = (bool) tempInt;
	readAndSkipPast("EnableSimplified", inputFile);
	fscanf(inputFile, "%d", &tempInt);
	pedsInputs.enableSimplifiedBehavior = (bool) tempInt;
	readAndSkipPast("DistPrevHIVPosIU", inputFile);
	for (i = 0; i < PEDS_MOM_HIV_NUM; i++) {
		fscanf(inputFile, " %lf", &(pedsInputs.initialHIVStateDistribution[PEDS_HIV_POS_IU][i]));
	}
	readAndSkipPast("DistPrevHIVPosIP", inputFile);
	for (i = 0; i < PEDS_MOM_HIV_NUM; i++) {
		fscanf(inputFile, " %lf", &(pedsInputs.initialHIVStateDistribution[PEDS_HIV_POS_IP][i]));
	}
	readAndSkipPast("DistPrevHIVNeg", inputFile);
	for (i = 0; i < PEDS_MOM_HIV_NUM; i++) {
		fscanf(inputFile, " %lf", &(pedsInputs.initialHIVStateDistribution[PEDS_HIV_NEG][i]));
	}
	for (i = 0; i < PEDS_MOM_HIV_NUM; i++) {
		pedsInputs.initialHIVStateDistribution[PEDS_HIV_POS_PP][i] = 0;
	}
	readAndSkipPast("BreastfedDist", inputFile);
	double tempSum = 0.0;
	for (i = 0; i < PEDS_BF_NUM - 1; i++) {
		fscanf(inputFile, " %lf", &(pedsInputs.initialBFDistribution[i]));
		tempSum += pedsInputs.initialBFDistribution[i];
	}
	pedsInputs.initialBFDistribution[SimContext::PEDS_BF_REPL] = 1 - tempSum;
	readAndSkipPast("BreastfedDuration", inputFile);
	fscanf(inputFile, " %d", &(pedsInputs.initialBFDuration));

	// read in initial CD4 percentage
	readAndSkipPast("InitCD4PercPrevIU", inputFile);
	fscanf(inputFile, "%lf %lf", &(pedsInputs.initialCD4PercentageIUMean),
			&(pedsInputs.initialCD4PercentageIUStdDev));
	readAndSkipPast("InitCD4PercPrevIP", inputFile);
	fscanf(inputFile, "%lf %lf", &(pedsInputs.initialCD4PercentageIPMean),
			&(pedsInputs.initialCD4PercentageIPStdDev));
	for (i = 0; i < PEDS_AGE_INFANT_NUM; i++) {
		readAndSkipPast2("InitCD4PercIncidPP", PEDS_AGE_CAT_STRS[i], inputFile);
		fscanf(inputFile, "%lf %lf", &(pedsInputs.initialCD4PercentagePPMean[i]),
				&(pedsInputs.initialCD4PercentagePPStdDev[i]));
	}

	// read in initial HVL strata
	readAndSkipPast("InitHVLPrevIU", inputFile);
	for (j = HVL_NUM_STRATA - 1; j >= 0; j--) {
		fscanf(inputFile, "%lf ", &(pedsInputs.initialHVLDistributionIU[j]));
	}
	readAndSkipPast("InitHVLPrevIP", inputFile);
	for (j = HVL_NUM_STRATA - 1; j >= 0; j--) {
		fscanf(inputFile, "%lf ", &(pedsInputs.initialHVLDistributionIP[j]));
	}
	for (i = 0; i < PEDS_AGE_INFANT_NUM; i++) {
		readAndSkipPast2("InitHVLIncidPP", PEDS_AGE_CAT_STRS[i], inputFile);
		for (j = HVL_NUM_STRATA - 1; j >= 0; j--) {
			fscanf(inputFile, "%lf ", &(pedsInputs.initialHVLDistributionPP[i][j]));
		}
	}

	// read in the mapping from age and CD4% to adult CD4 strata
	for (i = 0; i < PEDS_AGE_EARLY_NUM; i++) {
		readAndSkipPast2("AdultCD4Strata", PEDS_AGE_CAT_STRS[i], inputFile);
		for (j = 0; j < PEDS_CD4_PERC_NUM; j++) {
			fscanf(inputFile, "%d ", &tempInt);
			pedsInputs.adultCD4Strata[i][j] = (CD4_STRATA) tempInt;
		}
	}

	// read in the monthly natural history CD4% decline
	for (i = 0; i < PEDS_AGE_EARLY_NUM; i++) {
		readAndSkipPast2("CD4PercDeclinePrevIU", PEDS_AGE_CAT_STRS[i], inputFile);
		for (j = 0; j < PEDS_CD4_PERC_NUM; j++) {
			fscanf(inputFile, "%lf ", &(pedsInputs.monthlyCD4PercentDecline[PEDS_HIV_POS_IU][i][j]));
		}
	}
	for (i = 0; i < PEDS_AGE_EARLY_NUM; i++) {
		readAndSkipPast2("CD4PercDeclinePrevIP", PEDS_AGE_CAT_STRS[i], inputFile);
		for (j = 0; j < PEDS_CD4_PERC_NUM; j++) {
			fscanf(inputFile, "%lf ", &(pedsInputs.monthlyCD4PercentDecline[PEDS_HIV_POS_IP][i][j]));
		}
	}
	for (i = 0; i < PEDS_AGE_EARLY_NUM; i++) {
		readAndSkipPast2("CD4PercDeclineIncidPP", PEDS_AGE_CAT_STRS[i], inputFile);
		for (j = 0; j < PEDS_CD4_PERC_NUM; j++) {
			fscanf(inputFile, "%lf ", &(pedsInputs.monthlyCD4PercentDecline[PEDS_HIV_POS_PP][i][j]));
		}
	}

	// read in the transition from CD4% to absolute CD4
	for (i = 0; i < PEDS_CD4_PERC_NUM; i++) {
		readAndSkipPast2("CD4PercTransition", PEDS_CD4_PERC_STRS[i], inputFile);
		fscanf(inputFile, "%lf %lf", &(pedsInputs.absoluteCD4TransitionMean[i]),
				&(pedsInputs.absoluteCD4TransitionStdDev[i]));
	}

	// read in the transition from childhood HVL to adult setpoint HVL
	for (i = HVL_NUM_STRATA - 1; i >= 0; i--) {
		readAndSkipPast2("HVLTransPeds", HVL_STRATA_STRS[i], inputFile);
		for (j = HVL_NUM_STRATA - 1; j >= 0; j--) {
			fscanf(inputFile, "%lf ", &(pedsInputs.setpointHVLTransition[i][j]));
		}
	}

	// read in the probability of chronic AIDS death, for both early and late childhood
	for (i = 0; i < PEDS_AGE_EARLY_NUM; i++) {
		readAndSkipPast2("ProbChrAIDSDeathNoOIHist", PEDS_AGE_CAT_STRS[i], inputFile);
		for (j = 0; j < PEDS_CD4_PERC_NUM; j++) {
			fscanf(inputFile, "%lf ", &(pedsInputs.probChronicAIDSDeathEarly[HIST_EXT_N][i][j]));
		}
	}
	for (i = 0; i < PEDS_AGE_EARLY_NUM; i++) {
		readAndSkipPast2("ProbChrAIDSDeathMildOIHist", PEDS_AGE_CAT_STRS[i], inputFile);
		for (j = 0; j < PEDS_CD4_PERC_NUM; j++) {
			fscanf(inputFile, "%lf ", &(pedsInputs.probChronicAIDSDeathEarly[HIST_EXT_MILD][i][j]));
		}
	}
	for (i = 0; i < PEDS_AGE_EARLY_NUM; i++) {
		readAndSkipPast2("ProbChrAIDSDeathSvrOIHist", PEDS_AGE_CAT_STRS[i], inputFile);
		for (j = 0; j < PEDS_CD4_PERC_NUM; j++) {
			fscanf(inputFile, "%lf ", &(pedsInputs.probChronicAIDSDeathEarly[HIST_EXT_SEVR][i][j]));
		}
	}
	for (i = CD4_NUM_STRATA - 1; i >= 0; i--) {
		readAndSkipPast2("ProbChrAIDSDeathLate", CD4_STRATA_STRS[i], inputFile);
		for (j = 0; j < HIST_EXT_NUM; j++) {
			fscanf(inputFile, "%lf ", &(pedsInputs.probChronicAIDSDeathLate[j][i]));
		}
	}

	// read in the probability of non-AIDS death, early childhood only, late uses NatHist tables
	readAndSkipPast("ProbNonAIDSDeathMaleEarly", inputFile);
	for (i = 0; i < PEDS_AGE_EARLY_NUM; i++) {
		fscanf(inputFile, "%lf ", &(pedsInputs.probNonAIDSDeathEarly[GENDER_MALE][i]));
	}
	readAndSkipPast("ProbNonAIDSDeathFemaleEarly", inputFile);
	for (i = 0; i < PEDS_AGE_EARLY_NUM; i++) {
		fscanf(inputFile, "%lf ", &(pedsInputs.probNonAIDSDeathEarly[GENDER_FEMALE][i]));
	}

	// read in the probability of acute OIs, for early and late childhood
	for (i = 0; i < OI_NUM; i++) {
		sprintf(scratch, "Prob_%s_NoHist", OI_STRS[i]);
		for (j = 0; j < PEDS_AGE_EARLY_NUM; j++) {
			readAndSkipPast2(scratch, PEDS_AGE_CAT_STRS[j], inputFile);
			for (k = 0; k < PEDS_CD4_PERC_NUM; k++) {
				fscanf(inputFile, "%lf ", &(pedsInputs.probAcuteOIEarly[i][j][k][HIST_N]));
			}
		}
		sprintf(scratch, "Prob_%s_WithHist", OI_STRS[i]);
		for (j = 0; j < PEDS_AGE_EARLY_NUM; j++) {
			readAndSkipPast2(scratch, PEDS_AGE_CAT_STRS[j], inputFile);
			for (k = 0; k < PEDS_CD4_PERC_NUM; k++) {
				fscanf(inputFile, "%lf ", &(pedsInputs.probAcuteOIEarly[i][j][k][HIST_Y]));
			}
		}
	}
	for (i = CD4_NUM_STRATA - 1; i >= 0; i--) {
		readAndSkipPast2("ProbOIsNoHistLate", CD4_STRATA_STRS[i], inputFile);
		for (j = 0; j < OI_NUM; j++) {
			fscanf(inputFile, "%lf ", &(pedsInputs.probAcuteOILate[j][i][HIST_N]));
		}
	}
	for (i = CD4_NUM_STRATA - 1; i >= 0; i--) {
		readAndSkipPast2("ProbOIsWithHistLate", CD4_STRATA_STRS[i], inputFile);
		for (j = 0; j < OI_NUM; j++) {
			fscanf(inputFile, "%lf ", &(pedsInputs.probAcuteOILate[j][i][HIST_Y]));
		}
	}

	// read in the probability of death from acute OIs, for early and late childhood
	for (i = 0; i < PEDS_CD4_PERC_NUM; i++) {
		readAndSkipPast2("ProbDeathOIsTreatedNoHist", PEDS_CD4_PERC_STRS[i], inputFile);
		for (j = 0; j < OI_NUM; j++) {
			fscanf(inputFile, "%lf ", &(pedsInputs.probDeathAcuteOITreatedEarly[j][i][HIST_N]));
		}
	}
	for (i = 0; i < PEDS_CD4_PERC_NUM; i++) {
		readAndSkipPast2("ProbDeathOIsTreatedWithHist", PEDS_CD4_PERC_STRS[i], inputFile);
		for (j = 0; j < OI_NUM; j++) {
			fscanf(inputFile, "%lf ", &(pedsInputs.probDeathAcuteOITreatedEarly[j][i][HIST_Y]));
		}
	}
	for (i = 0; i < PEDS_CD4_PERC_NUM; i++) {
		readAndSkipPast2("ProbDeathOIsUntreatedNoHist", PEDS_CD4_PERC_STRS[i], inputFile);
		for (j = 0; j < OI_NUM; j++) {
			fscanf(inputFile, "%lf ", &(pedsInputs.probDeathAcuteOIUntreatedEarly[j][i][HIST_N]));
		}
	}
	for (i = 0; i < PEDS_CD4_PERC_NUM; i++) {
		readAndSkipPast2("ProbDeathOIsUntreatedWithHist", PEDS_CD4_PERC_STRS[i], inputFile);
		for (j = 0; j < OI_NUM; j++) {
			fscanf(inputFile, "%lf ", &(pedsInputs.probDeathAcuteOIUntreatedEarly[j][i][HIST_Y]));
		}
	}
	for (i = CD4_NUM_STRATA - 1; i >= 0; i--) {
		readAndSkipPast2("ProbDeathOIsTreatedNoHistLate", CD4_STRATA_STRS[i], inputFile);
		for (j = 0; j < OI_NUM; j++) {
			fscanf(inputFile, "%lf ", &(pedsInputs.probDeathAcuteOITreatedLate[j][i][HIST_N]));
		}
	}
	for (i = CD4_NUM_STRATA - 1; i >= 0; i--) {
		readAndSkipPast2("ProbDeathOIsTreatedWithHistLate", CD4_STRATA_STRS[i], inputFile);
		for (j = 0; j < OI_NUM; j++) {
			fscanf(inputFile, "%lf ", &(pedsInputs.probDeathAcuteOITreatedLate[j][i][HIST_Y]));
		}
	}
	for (i = CD4_NUM_STRATA - 1; i >= 0; i--) {
		readAndSkipPast2("ProbDeathOIsUntreatedNoHistLate", CD4_STRATA_STRS[i], inputFile);
		for (j = 0; j < OI_NUM; j++) {
			fscanf(inputFile, "%lf ", &(pedsInputs.probDeathAcuteOIUntreatedLate[j][i][HIST_N]));
		}
	}
	for (i = CD4_NUM_STRATA - 1; i >= 0; i--) {
		readAndSkipPast2("ProbDeathOIsUntreatedWithHistLate", CD4_STRATA_STRS[i], inputFile);
		for (j = 0; j < OI_NUM; j++) {
			fscanf(inputFile, "%lf ", &(pedsInputs.probDeathAcuteOIUntreatedLate[j][i][HIST_Y]));
		}
	}

	// read in maternal infection and mortality
	readAndSkipPast("ProbMaternalInfect", inputFile);
	fscanf(inputFile, "%lf ", &pedsInputs.probMaternalHIVInfection);
	readAndSkipPast("ProbMaternalDeath", inputFile);
	for (i = 0; i < SimContext::PEDS_MOM_HIV_NUM; i++) {
		fscanf(inputFile, "%lf ", &(pedsInputs.probMaternalDeath[i]));
	}

	// read in pediatrics incident PP infection
	readAndSkipPast2("ProbPedsInfect", "Exclusive", inputFile);
	for (i = 0; i < SimContext::PEDS_MOM_HIV_POS_NUM; i++) {
		fscanf(inputFile, "%lf ", &(pedsInputs.probHIVInfectionPP[SimContext::PEDS_BF_EXCL][i]));
	}
	readAndSkipPast2("ProbPedsInfect", "Mixed", inputFile);
	for (i = 0; i < SimContext::PEDS_MOM_HIV_POS_NUM; i++) {
		fscanf(inputFile, "%lf ", &(pedsInputs.probHIVInfectionPP[SimContext::PEDS_BF_MIXED][i]));
	}
	readAndSkipPast2("ProbPedsInfect", "Replacement", inputFile);
	for (i = 0; i < SimContext::PEDS_MOM_HIV_POS_NUM; i++) {
		fscanf(inputFile, "%lf ", &(pedsInputs.probHIVInfectionPP[SimContext::PEDS_BF_REPL][i]));
	}

	// read in simplified pediatrics mortality tables
	readAndSkipPast2("ProbPedsDeath", "NonexposedMale", inputFile);
	for (i = 0; i < SimContext::PEDS_AGE_CHILD_NUM; i++) {
		fscanf(inputFile, "%lf ", &(pedsInputs.probDeathHIVNegativeNonexposed[SimContext::GENDER_MALE][i]));
	}
	readAndSkipPast2("ProbPedsDeath", "NonexposedFemale", inputFile);
	for (i = 0; i < SimContext::PEDS_AGE_CHILD_NUM; i++) {
		fscanf(inputFile, "%lf ", &(pedsInputs.probDeathHIVNegativeNonexposed[SimContext::GENDER_FEMALE][i]));
	}
	readAndSkipPast2("ProbPedsDeath", "ExposedMale", inputFile);
	for (i = 0; i < SimContext::PEDS_AGE_CHILD_NUM; i++) {
		fscanf(inputFile, "%lf ", &(pedsInputs.probDeathHIVNegativeExposed[SimContext::GENDER_MALE][i]));
	}
	readAndSkipPast2("ProbPedsDeath", "ExposedFemale", inputFile);
	for (i = 0; i < SimContext::PEDS_AGE_CHILD_NUM; i++) {
		fscanf(inputFile, "%lf ", &(pedsInputs.probDeathHIVNegativeExposed[SimContext::GENDER_FEMALE][i]));
	}
	readAndSkipPast2("ProbPedsDeath", "PrevOffARTMale", inputFile);
	for (i = 0; i < SimContext::PEDS_AGE_CHILD_NUM; i++) {
		fscanf(inputFile, "%lf ", &(pedsInputs.probDeathHIVPositive[SimContext::PEDS_HIV_POS_IP][SimContext::ART_OFF_STATE][SimContext::GENDER_MALE][i]));
		pedsInputs.probDeathHIVPositive[SimContext::PEDS_HIV_POS_IU][SimContext::ART_OFF_STATE][SimContext::GENDER_MALE][i] = pedsInputs.probDeathHIVPositive[SimContext::PEDS_HIV_POS_IP][SimContext::ART_OFF_STATE][SimContext::GENDER_MALE][i];
	}
	readAndSkipPast2("ProbPedsDeath", "PrevOffARTFemale", inputFile);
	for (i = 0; i < SimContext::PEDS_AGE_CHILD_NUM; i++) {
		fscanf(inputFile, "%lf ", &(pedsInputs.probDeathHIVPositive[SimContext::PEDS_HIV_POS_IP][SimContext::ART_OFF_STATE][SimContext::GENDER_FEMALE][i]));
		pedsInputs.probDeathHIVPositive[SimContext::PEDS_HIV_POS_IU][SimContext::ART_OFF_STATE][SimContext::GENDER_FEMALE][i] = pedsInputs.probDeathHIVPositive[SimContext::PEDS_HIV_POS_IP][SimContext::ART_OFF_STATE][SimContext::GENDER_FEMALE][i];
	}
	readAndSkipPast2("ProbPedsDeath", "PrevOnARTMale", inputFile);
	for (i = 0; i < SimContext::PEDS_AGE_CHILD_NUM; i++) {
		fscanf(inputFile, "%lf ", &(pedsInputs.probDeathHIVPositive[SimContext::PEDS_HIV_POS_IP][SimContext::ART_ON_STATE][SimContext::GENDER_MALE][i]));
		pedsInputs.probDeathHIVPositive[SimContext::PEDS_HIV_POS_IU][SimContext::ART_ON_STATE][SimContext::GENDER_MALE][i] = pedsInputs.probDeathHIVPositive[SimContext::PEDS_HIV_POS_IP][SimContext::ART_ON_STATE][SimContext::GENDER_MALE][i];
	}
	readAndSkipPast2("ProbPedsDeath", "PrevOnARTFemale", inputFile);
	for (i = 0; i < SimContext::PEDS_AGE_CHILD_NUM; i++) {
		fscanf(inputFile, "%lf ", &(pedsInputs.probDeathHIVPositive[SimContext::PEDS_HIV_POS_IP][SimContext::ART_ON_STATE][SimContext::GENDER_FEMALE][i]));
		pedsInputs.probDeathHIVPositive[SimContext::PEDS_HIV_POS_IU][SimContext::ART_ON_STATE][SimContext::GENDER_FEMALE][i] = pedsInputs.probDeathHIVPositive[SimContext::PEDS_HIV_POS_IP][SimContext::ART_ON_STATE][SimContext::GENDER_FEMALE][i];
	}
	readAndSkipPast2("ProbPedsDeath", "IncidOffARTMale", inputFile);
	for (i = 0; i < SimContext::PEDS_AGE_CHILD_NUM; i++) {
		fscanf(inputFile, "%lf ", &(pedsInputs.probDeathHIVPositive[SimContext::PEDS_HIV_POS_PP][SimContext::ART_OFF_STATE][SimContext::GENDER_MALE][i]));
	}
	readAndSkipPast2("ProbPedsDeath", "IncidOffARTFemale", inputFile);
	for (i = 0; i < SimContext::PEDS_AGE_CHILD_NUM; i++) {
		fscanf(inputFile, "%lf ", &(pedsInputs.probDeathHIVPositive[SimContext::PEDS_HIV_POS_PP][SimContext::ART_OFF_STATE][SimContext::GENDER_FEMALE][i]));
	}
	readAndSkipPast2("ProbPedsDeath", "IncidOnARTMale", inputFile);
	for (i = 0; i < SimContext::PEDS_AGE_CHILD_NUM; i++) {
		fscanf(inputFile, "%lf ", &(pedsInputs.probDeathHIVPositive[SimContext::PEDS_HIV_POS_PP][SimContext::ART_ON_STATE][SimContext::GENDER_MALE][i]));
	}
	readAndSkipPast2("ProbPedsDeath", "IncidOnARTFemale", inputFile);
	for (i = 0; i < SimContext::PEDS_AGE_CHILD_NUM; i++) {
		fscanf(inputFile, "%lf ", &(pedsInputs.probDeathHIVPositive[SimContext::PEDS_HIV_POS_PP][SimContext::ART_ON_STATE][SimContext::GENDER_FEMALE][i]));
	}
	readAndSkipPast("ProbPedsDeathMaternalMult", inputFile);
	fscanf(inputFile, "%lf ", &(pedsInputs.probDeathMaternalRateMultiplier));
	readAndSkipPast("ProbPedsDeathReplFedMult", inputFile);
	fscanf(inputFile, "%lf ", &(pedsInputs.probDeathReplacementFedMultiplier));
	readAndSkipPast("ProbPedsARTTreatment", inputFile);
	fscanf(inputFile, "%lf %lf", &(pedsInputs.probStartART[SimContext::PEDS_HIV_POS_IP]),
			&(pedsInputs.probStartART[SimContext::PEDS_HIV_POS_PP]));
	pedsInputs.probStartART[SimContext::PEDS_HIV_POS_IU] = pedsInputs.probStartART[SimContext::PEDS_HIV_POS_IP];

	// read in ART policies inputs
	// read in maximum cd4 percentages and testing intervals
	readAndSkipPast("MaxPedsCD4Perc", inputFile);
	for (i = 0; i < SimContext::PEDS_AGE_EARLY_NUM; i++) {
		fscanf(inputFile, "%lf", &(pedsInputs.maxCD4Percentage[i]));
	}
	readAndSkipPast("IntvlCD4TstPreARTPeds", inputFile);
	fscanf(inputFile, "%ld %ld", &(pedsInputs.CD4TestingIntervalPreARTEarly),
			&(pedsInputs.CD4TestingIntervalPreARTLate));
	readAndSkipPast("IntvlHVLTstPreARTPeds", inputFile);
	fscanf(inputFile, "%ld %ld", &(pedsInputs.HVLTestingIntervalPreARTEarly),
			&(pedsInputs.HVLTestingIntervalPreARTLate));

	// read in start ART policy
	readAndSkipPast2("ARTstart_CD4Perc", "upp", inputFile);
	for (i = 0; i < SimContext::ART_NUM_LINES; i++) {
		fscanf(inputFile, "%lf", &(pedsInputs.startARTCD4PercentageBoundsOnly[i][SimContext::UPPER_BOUND]));
	}
	readAndSkipPast2("ARTstart_CD4Perc", "lwr", inputFile);
	for (i = 0; i < SimContext::ART_NUM_LINES; i++) {
		fscanf(inputFile, "%lf", &(pedsInputs.startARTCD4PercentageBoundsOnly[i][SimContext::LOWER_BOUND]));
	}
	readAndSkipPast2("ARTstart_CD4PercHVL", "CD4upp", inputFile);
	for (i = 0; i < SimContext::ART_NUM_LINES; i++) {
		fscanf(inputFile, "%lf", &(pedsInputs.startARTCD4PercentageBoundsWithHVL[i][SimContext::UPPER_BOUND]));
	}
	readAndSkipPast2("ARTstart_CD4PercHVL", "CD4lwr", inputFile);
	for (i = 0; i < SimContext::ART_NUM_LINES; i++) {
		fscanf(inputFile, "%lf", &(pedsInputs.startARTCD4PercentageBoundsWithHVL[i][SimContext::LOWER_BOUND]));
	}
	readAndSkipPast2("ARTstart_CD4PercHVL", "HVLupp", inputFile);
	for (i = 0; i < SimContext::ART_NUM_LINES; i++) {
		fscanf(inputFile, "%ld", &(pedsInputs.startARTHVLBoundsWithCD4Percentage[i][SimContext::UPPER_BOUND]));
	}
	readAndSkipPast2("ARTstart_CD4PercHVL", "HVLlwr", inputFile);
	for (i = 0; i < SimContext::ART_NUM_LINES; i++) {
		fscanf(inputFile, "%ld", &(pedsInputs.startARTHVLBoundsWithCD4Percentage[i][SimContext::LOWER_BOUND]));
	}
	readAndSkipPast2("ARTstart", "MinMthsAge", inputFile);
	for (i = 0; i < SimContext::ART_NUM_LINES; i++) {
		fscanf(inputFile, "%ld", &(pedsInputs.startARTMinAgeMonths[i]));
	}

	// read in ART observed failure policy inputs
	readAndSkipPast("ARTfail_cd4PercAbsolDrop", inputFile);
	for (i = 0; i < SimContext::ART_NUM_LINES; i++) {
		fscanf(inputFile, "%lf", &(pedsInputs.failARTCD4PercentageAbsolDrop[i]));
	}
	readAndSkipPast("ARTfail_cd4PercBelowPreARTNadir", inputFile);
	for (i = 0; i < SimContext::ART_NUM_LINES; i++) {
		fscanf(inputFile, "%ld", &tempInt);
		pedsInputs.failARTCD4PercentageBelowNadir[i] = (bool) tempInt;
	}
	readAndSkipPast2("ARTfail_cd4PercAbsolOR", "uppBnd", inputFile);
	for (i = 0; i < SimContext::ART_NUM_LINES; i++) {
		fscanf(inputFile, "%lf", &(pedsInputs.failARTCD4PercentageBoundsOR[i][SimContext::UPPER_BOUND]));
	}
	readAndSkipPast2("ARTfail_cd4PercAbsolOR", "lwrBnd", inputFile);
	for (i = 0; i < SimContext::ART_NUM_LINES; i++) {
		fscanf(inputFile, "%lf", &(pedsInputs.failARTCD4PercentageBoundsOR[i][SimContext::LOWER_BOUND]));
	}
	readAndSkipPast2("ARTfail_cd4PercAbsolAND", "uppBnd", inputFile);
	for (i = 0; i < SimContext::ART_NUM_LINES; i++) {
		fscanf(inputFile, "%lf", &(pedsInputs.failARTCD4PercentageBoundsAND[i][SimContext::UPPER_BOUND]));
	}
	readAndSkipPast2("ARTfail_cd4PercAbsolAND", "lwrBnd", inputFile);
	for (i = 0; i < SimContext::ART_NUM_LINES; i++) {
		fscanf(inputFile, "%lf", &(pedsInputs.failARTCD4PercentageBoundsAND[i][SimContext::LOWER_BOUND]));
	}
	readAndSkipPast("ARTfail_cd4PercMthsFromInit", inputFile);
	for (i = 0; i < SimContext::ART_NUM_LINES; i++) {
		fscanf(inputFile, "%ld", &(pedsInputs.failARTMonthsFromInit[i]));
	}

	// read in ART stop policy inputs
	readAndSkipPast("ARTstop_MaxMthsAge", inputFile);
	for (i = 0; i < SimContext::ART_NUM_LINES; i++) {
		fscanf(inputFile, "%ld", &(pedsInputs.stopARTMaxAgeMonths[i]));
	}
	readAndSkipPast("ARTstop_OnFailBelowCD4Perc", inputFile);
	for (i = 0; i < SimContext::ART_NUM_LINES; i++) {
		fscanf(inputFile, "%lf", &(pedsInputs.stopARTAfterFailCD4PercentageBound[i]));
	}

	// read in ART effect rate multipliers
	readAndSkipPast("MthStageRateMultChrDeathPedsEarly", inputFile);
	fscanf(inputFile, "%ld %ld", &(pedsInputs.stageBoundsChronicAIDSDeathProbOnARTMultEarly[0]),
			&(pedsInputs.stageBoundsChronicAIDSDeathProbOnARTMultEarly[1]));
	for (i = 0; i < SimContext::PEDS_CD4_PERC_NUM; i++) {
		readAndSkipPast2("RateMultChrDeathPedsEarly", SimContext::PEDS_CD4_PERC_STRS[i], inputFile);
		for (j = 0; j < 3; j++) {
			fscanf(inputFile, "%lf", &(pedsInputs.chronicAIDSDeathProbOnARTMultEarly[i][j]));
		}
	}
	readAndSkipPast("MthStageRateMultOIsPedsEarly", inputFile);
	fscanf(inputFile, "%ld %ld", &(pedsInputs.stageBoundsMonthlyOIProbOnARTMultEarly[0]),
			&(pedsInputs.stageBoundsMonthlyOIProbOnARTMultEarly[1]));
	for (i = 0; i < SimContext::PEDS_CD4_PERC_NUM; i++) {
		readAndSkipPast2("RateMultOIsPedsEarly", SimContext::PEDS_CD4_PERC_STRS[i], inputFile);
		for (j = 0; j < 3; j++) {
			fscanf(inputFile, "%lf", &(pedsInputs.monthlyOIProbOnARTMultEarly[i][j]));
		}
	}
	for (i = SimContext::CD4_NUM_STRATA - 1; i >= 0; i--) {
		readAndSkipPast2("RateMultChrDeathPedsLate", SimContext::CD4_STRATA_STRS[i], inputFile);
		for (j = 0; j < SimContext::HIST_EXT_NUM; j++) {
			fscanf(inputFile, "%lf", &(pedsInputs.chronicAIDSDeathProbOnARTMultLate[i][j]));
		}
	}
	for (i = SimContext::CD4_NUM_STRATA - 1; i >= 0; i--) {
		readAndSkipPast2("RateMultOIsPedsLate", SimContext::CD4_STRATA_STRS[i], inputFile);
		for (j = 0; j < SimContext::OI_NUM; j++) {
			fscanf(inputFile, "%lf", &(pedsInputs.monthlyOIProbOnARTMultLate[i][j]));
		}
	}
} /* end readPedsInputs */

/* readPedsARTInputs reads data from the UserARTs tab of the input sheet */
void SimContext::readPedsARTInputs() {
	char tmpBuf[256], tmpBuf2[256];
	int i, j, k;
	double tempCost;
	FILE *file = inputFile;

	for (int artNum = 1; artNum <= ART_NUM_LINES; artNum++) {
		// read in regimen id num and name
		sprintf(tmpBuf, "ART%dIdPeds", artNum);
		readAndSkipPast( tmpBuf, file );
		int idNum;
		fscanf( file, " %d", &idNum );
		// skip to next regimen if this one is not specified
		if (idNum == NOT_APPL) {
			pedsARTInputs[artNum - 1] = NULL;
			continue;
		}
		// create new regimen input structure
		pedsARTInputs[artNum - 1] = new PedsARTInputs();
		PedsARTInputs &pedsART = *(pedsARTInputs[artNum - 1]);

		// read in one-time startup cost
		sprintf(tmpBuf, "ART%dInitCostPeds", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf", &pedsART.costInitialEarly, &pedsART.costInitialLate );
		// read in monthly cost
		sprintf(tmpBuf, "ART%dMthCostPeds", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf", &pedsART.costMonthlyEarly, &pedsART.costMonthlyLate );

		// read in efficacy time horizon
		sprintf(tmpBuf, "ART%dEffTimeHorizonPeds", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%d %d", &pedsART.efficacyTimeHorizonEarly, &pedsART.efficacyTimeHorizonLate );
		// read in chances of ART success
		sprintf(tmpBuf, "ART%dSuccessProbPedsEarly", artNum);
		readAndSkipPast( tmpBuf, file );
		readAndSkipPast( "Succ", file );
		for (i = HVL_NUM_STRATA - 1; i >= 0; --i)
			fscanf( file, "%lf", &(pedsART.probInitialEfficacyEarly[ART_EFF_SUCCESS][i]) );
		readAndSkipPast( tmpBuf, file );
		readAndSkipPast( "Part", file );
		for (i = HVL_NUM_STRATA - 1; i >= 0; --i)
			fscanf( file, "%lf", &(pedsART.probInitialEfficacyEarly[ART_EFF_PARTIAL][i]) );
		for (i = HVL_NUM_STRATA - 1; i >= 0; --i)
			pedsART.probInitialEfficacyEarly[ART_EFF_FAILURE][i] = 1.0 - pedsART.probInitialEfficacyEarly[ART_EFF_SUCCESS][i] - pedsART.probInitialEfficacyEarly[ART_EFF_PARTIAL][i];
		sprintf(tmpBuf, "ART%dSuccessProbPedsLate", artNum);
		readAndSkipPast( tmpBuf, file );
		readAndSkipPast( "Succ", file );
		for (i = HVL_NUM_STRATA - 1; i >= 0; --i)
			fscanf( file, "%lf", &(pedsART.probInitialEfficacyLate[ART_EFF_SUCCESS][i]) );
		readAndSkipPast( tmpBuf, file );
		readAndSkipPast( "Part", file );
		for (i = HVL_NUM_STRATA - 1; i >= 0; --i)
			fscanf( file, "%lf", &(pedsART.probInitialEfficacyLate[ART_EFF_PARTIAL][i]) );
		for (i = HVL_NUM_STRATA - 1; i >= 0; --i)
			pedsART.probInitialEfficacyLate[ART_EFF_FAILURE][i] = 1.0 - pedsART.probInitialEfficacyLate[ART_EFF_SUCCESS][i] - pedsART.probInitialEfficacyLate[ART_EFF_PARTIAL][i];

		// read in distribution of partial suppression
		sprintf(tmpBuf, "ART%dDistribPartialPedsEarly", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf %lf", &(pedsART.partialSuppressionDistributionEarly[0]),
			&(pedsART.partialSuppressionDistributionEarly[1]),
			&(pedsART.partialSuppressionDistributionEarly[2]) );
		sprintf(tmpBuf, "ART%dDistribPartialPedsLate", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf %lf", &(pedsART.partialSuppressionDistributionLate[0]),
			&(pedsART.partialSuppressionDistributionLate[1]),
			&(pedsART.partialSuppressionDistributionLate[2]) );
		// read in chances of late failure/partial suppression
		sprintf(tmpBuf, "ART%dLateFail_SuccPeds", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf", &pedsART.probLateFailFromSuppressEarly, &pedsART.probLateFailFromSuppressLate);
		sprintf(tmpBuf, "ART%dLatePart_SuccPeds", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf", &pedsART.probLatePartialSuppressFromSuppressEarly, &pedsART.probLatePartialSuppressFromSuppressLate );
		sprintf(tmpBuf, "ART%dLateFail_PartPeds", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf", &pedsART.probLateFailFromPartialSuppressEarly, &pedsART.probLateFailFromPartialSuppressLate );
		// read in mth by which all would fail
		sprintf(tmpBuf, "ART%dMthForceFailPeds", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%d %d", &pedsART.forceFailAtMonthEarly, &pedsART.forceFailAtMonthLate );

		// read in CD4 effect on ART
		sprintf(tmpBuf, "ART%dMthStageCD4Eff_SuccPedsEarly", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%d %d", &(pedsART.stageBoundsCD4PercentageChangeOnARTEarly[ART_EFF_SUCCESS][0]),
				&(pedsART.stageBoundsCD4PercentageChangeOnARTEarly[ART_EFF_SUCCESS][1]));
		sprintf(tmpBuf, "ART%dCD4EffSlope_SuccPedsEarly", artNum);
		for (j = 0; j < PEDS_AGE_EARLY_NUM; j++) {
			for (k = 0; k < CD4_RESPONSE_NUM_TYPES; k++) {
				sprintf(tmpBuf2, "%s%s", PEDS_AGE_CAT_STRS[j], CD4_RESPONSE_STRS[k]);
				readAndSkipPast2( tmpBuf, tmpBuf2, file );
				fscanf( file, "%lf %lf %lf %lf %lf %lf",
					&(pedsART.CD4PercentageChangeOnARTMeanEarly[ART_EFF_SUCCESS][j][k][0]), &(pedsART.CD4PercentageChangeOnARTStdDevEarly[ART_EFF_SUCCESS][j][k][0]),
					&(pedsART.CD4PercentageChangeOnARTMeanEarly[ART_EFF_SUCCESS][j][k][1]), &(pedsART.CD4PercentageChangeOnARTStdDevEarly[ART_EFF_SUCCESS][j][k][1]),
					&(pedsART.CD4PercentageChangeOnARTMeanEarly[ART_EFF_SUCCESS][j][k][2]), &(pedsART.CD4PercentageChangeOnARTStdDevEarly[ART_EFF_SUCCESS][j][k][2]));
			}
		}
		sprintf(tmpBuf, "ART%dMthStageCD4Eff_SuccPedsLate", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%d %d", &(pedsART.stageBoundsCD4ChangeOnARTLate[ART_EFF_SUCCESS][0]),
				&(pedsART.stageBoundsCD4ChangeOnARTLate[ART_EFF_SUCCESS][1]));
		sprintf(tmpBuf, "ART%dCD4EffSlope_SuccPedsLate", artNum);
		for (j = 0; j < CD4_RESPONSE_NUM_TYPES; j++) {
			readAndSkipPast2(tmpBuf, CD4_RESPONSE_STRS[j], file);
			fscanf(file, "%lf %lf %lf %lf %lf %lf",
				&(pedsART.CD4ChangeOnARTMeanLate[ART_EFF_SUCCESS][j][0]), &(pedsART.CD4ChangeOnARTStdDevLate[ART_EFF_SUCCESS][j][0]),
				&(pedsART.CD4ChangeOnARTMeanLate[ART_EFF_SUCCESS][j][1]), &(pedsART.CD4ChangeOnARTStdDevLate[ART_EFF_SUCCESS][j][1]),
				&(pedsART.CD4ChangeOnARTMeanLate[ART_EFF_SUCCESS][j][2]), &(pedsART.CD4ChangeOnARTStdDevLate[ART_EFF_SUCCESS][j][2]));
		}
		sprintf(tmpBuf, "ART%dMthStageCD4Eff_PartPedsEarly", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%d %d", &(pedsART.stageBoundsCD4PercentageChangeOnARTEarly[ART_EFF_PARTIAL][0]),
				&(pedsART.stageBoundsCD4PercentageChangeOnARTEarly[ART_EFF_PARTIAL][1]));
		sprintf(tmpBuf, "ART%dCD4EffSlope_PartPedsEarly", artNum);
		for (j = 0; j < PEDS_AGE_EARLY_NUM; j++) {
			for (k = 0; k < CD4_RESPONSE_NUM_TYPES; k++) {
				sprintf(tmpBuf2, "%s%s", PEDS_AGE_CAT_STRS[j], CD4_RESPONSE_STRS[k]);
				readAndSkipPast2( tmpBuf, tmpBuf2, file );
				fscanf( file, "%lf %lf %lf %lf %lf %lf",
					&(pedsART.CD4PercentageChangeOnARTMeanEarly[ART_EFF_PARTIAL][j][k][0]), &(pedsART.CD4PercentageChangeOnARTStdDevEarly[ART_EFF_PARTIAL][j][k][0]),
					&(pedsART.CD4PercentageChangeOnARTMeanEarly[ART_EFF_PARTIAL][j][k][1]), &(pedsART.CD4PercentageChangeOnARTStdDevEarly[ART_EFF_PARTIAL][j][k][1]),
					&(pedsART.CD4PercentageChangeOnARTMeanEarly[ART_EFF_PARTIAL][j][k][2]), &(pedsART.CD4PercentageChangeOnARTStdDevEarly[ART_EFF_PARTIAL][j][k][2]));
			}
		}
		sprintf(tmpBuf, "ART%dMthStageCD4Eff_PartPedsLate", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%d %d", &(pedsART.stageBoundsCD4ChangeOnARTLate[ART_EFF_PARTIAL][0]),
				&(pedsART.stageBoundsCD4ChangeOnARTLate[ART_EFF_PARTIAL][1]));
		sprintf(tmpBuf, "ART%dCD4EffSlope_PartPedsLate", artNum);
		for (j = 0; j < CD4_RESPONSE_NUM_TYPES; j++) {
			readAndSkipPast2(tmpBuf, CD4_RESPONSE_STRS[j], file);
			fscanf(file, "%lf %lf %lf %lf %lf %lf",
				&(pedsART.CD4ChangeOnARTMeanLate[ART_EFF_PARTIAL][j][0]), &(pedsART.CD4ChangeOnARTStdDevLate[ART_EFF_PARTIAL][j][0]),
				&(pedsART.CD4ChangeOnARTMeanLate[ART_EFF_PARTIAL][j][1]), &(pedsART.CD4ChangeOnARTStdDevLate[ART_EFF_PARTIAL][j][1]),
				&(pedsART.CD4ChangeOnARTMeanLate[ART_EFF_PARTIAL][j][2]), &(pedsART.CD4ChangeOnARTStdDevLate[ART_EFF_PARTIAL][j][2]));
		}
		sprintf(tmpBuf, "ART%dMthStageCD4Eff_FailPedsEarly", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%d %d", &(pedsART.stageBoundsCD4PercentageChangeOnARTEarly[ART_EFF_FAILURE][0]),
				&(pedsART.stageBoundsCD4PercentageChangeOnARTEarly[ART_EFF_FAILURE][1]));
		sprintf(tmpBuf, "ART%dCD4EffMult_FailPedsEarly", artNum);
		for (j = 0; j < CD4_RESPONSE_NUM_TYPES; j++) {
			readAndSkipPast2(tmpBuf, CD4_RESPONSE_STRS[j], file);
			fscanf( file, "%lf %lf %lf",
				&(pedsART.CD4PercentageMultiplierOnFailedARTEarly[j][0]),
				&(pedsART.CD4PercentageMultiplierOnFailedARTEarly[j][1]),
				&(pedsART.CD4PercentageMultiplierOnFailedARTEarly[j][2]));
		}
		sprintf(tmpBuf, "ART%dMthCD4SecStdDevPedsEarly", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf", &pedsART.secondaryCD4PercentageChangeOnARTStdDevEarly);
		sprintf(tmpBuf, "ART%dMthStageCD4Eff_FailPedsLate", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%d %d", &(pedsART.stageBoundsCD4ChangeOnARTLate[ART_EFF_FAILURE][0]),
				&(pedsART.stageBoundsCD4ChangeOnARTLate[ART_EFF_FAILURE][1]));
		sprintf(tmpBuf, "ART%dCD4EffMult_FailPedsLate", artNum);
		for (j = 0; j < CD4_RESPONSE_NUM_TYPES; j++) {
			readAndSkipPast2(tmpBuf, CD4_RESPONSE_STRS[j], file);
			fscanf( file, "%lf %lf %lf",
				&(pedsART.CD4MultiplierOnFailedARTLate[j][0]),
				&(pedsART.CD4MultiplierOnFailedARTLate[j][1]),
				&(pedsART.CD4MultiplierOnFailedARTLate[j][2]));
		}
		sprintf(tmpBuf, "ART%dMthCD4SecStdDevPedsLate", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf", &pedsART.secondaryCD4ChangeOnARTStdDevLate);

		// read in CD4 effect off ART
		sprintf(tmpBuf, "ART%dCD4EffOffART_SuccPedsEarly", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf", &(pedsART.monthlyCD4PercentageMultiplierOffARTPreSetpointEarly[ART_EFF_SUCCESS]),
			&(pedsART.monthlyCD4PercentageMultiplierOffARTPostSetpointEarly[ART_EFF_SUCCESS]) );
		sprintf(tmpBuf, "ART%dCD4EffOffART_PartPedsEarly", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf", &(pedsART.monthlyCD4PercentageMultiplierOffARTPreSetpointEarly[ART_EFF_PARTIAL]),
			&(pedsART.monthlyCD4PercentageMultiplierOffARTPostSetpointEarly[ART_EFF_PARTIAL]) );
		sprintf(tmpBuf, "ART%dCD4EffOffART_FailPedsEarly", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf", &(pedsART.monthlyCD4PercentageMultiplierOffARTPreSetpointEarly[ART_EFF_FAILURE]),
			&(pedsART.monthlyCD4PercentageMultiplierOffARTPostSetpointEarly[ART_EFF_FAILURE]) );
		sprintf(tmpBuf, "ART%dCD4EffOffART_SuccPedsLate", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf", &(pedsART.monthlyCD4MultiplierOffARTPreSetpointLate[ART_EFF_SUCCESS]),
			&(pedsART.monthlyCD4MultiplierOffARTPostSetpointLate[ART_EFF_SUCCESS]) );
		sprintf(tmpBuf, "ART%dCD4EffOffART_PartPedsLate", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf", &(pedsART.monthlyCD4MultiplierOffARTPreSetpointLate[ART_EFF_PARTIAL]),
			&(pedsART.monthlyCD4MultiplierOffARTPostSetpointLate[ART_EFF_PARTIAL]) );
		sprintf(tmpBuf, "ART%dCD4EffOffART_FailPedsLate", artNum);
		readAndSkipPast( tmpBuf, file );
		fscanf( file, "%lf %lf", &(pedsART.monthlyCD4MultiplierOffARTPreSetpointLate[ART_EFF_FAILURE]),
			&(pedsART.monthlyCD4MultiplierOffARTPostSetpointLate[ART_EFF_FAILURE]) );

		// read in HVL change rate
		sprintf(tmpBuf, "ART%dHVLChgRatePedsEarly", artNum);
		readAndSkipPast2( tmpBuf, "Supp", file );
		fscanf( file, "%lf %d", &(pedsART.monthlyProbHVLChangeEarly[ART_EFF_SUCCESS]),
				&(pedsART.monthlyNumStrataHVLChangeEarly[ART_EFF_SUCCESS]) );
		sprintf(tmpBuf, "ART%dHVLChgRatePedsEarly", artNum);
		readAndSkipPast2( tmpBuf, "PartSupp", file );
		fscanf( file, "%lf %d", &(pedsART.monthlyProbHVLChangeEarly[ART_EFF_PARTIAL]),
				&(pedsART.monthlyNumStrataHVLChangeEarly[ART_EFF_PARTIAL]) );
		sprintf(tmpBuf, "ART%dHVLChgRatePedsEarly", artNum);
		readAndSkipPast2( tmpBuf, "Fail", file );
		fscanf( file, "%lf %d", &(pedsART.monthlyProbHVLChangeEarly[ART_EFF_FAILURE]),
				&(pedsART.monthlyNumStrataHVLChangeEarly[ART_EFF_FAILURE]) );
		sprintf(tmpBuf, "ART%dHVLChgRatePedsLate", artNum);
		readAndSkipPast2( tmpBuf, "Supp", file );
		fscanf( file, "%lf %d", &(pedsART.monthlyProbHVLChangeLate[ART_EFF_SUCCESS]),
				&(pedsART.monthlyNumStrataHVLChangeLate[ART_EFF_SUCCESS]) );
		sprintf(tmpBuf, "ART%dHVLChgRatePedsLate", artNum);
		readAndSkipPast2( tmpBuf, "PartSupp", file );
		fscanf( file, "%lf %d", &(pedsART.monthlyProbHVLChangeLate[ART_EFF_PARTIAL]),
				&(pedsART.monthlyNumStrataHVLChangeLate[ART_EFF_PARTIAL]) );
		sprintf(tmpBuf, "ART%dHVLChgRatePedsLate", artNum);
		readAndSkipPast2( tmpBuf, "Fail", file );
		fscanf( file, "%lf %d", &(pedsART.monthlyProbHVLChangeLate[ART_EFF_FAILURE]),
				&(pedsART.monthlyNumStrataHVLChangeLate[ART_EFF_FAILURE]) );
	}
} /* end readPedsARTInputs */

/* readAndSkipPast and readAndSkipPast2 skip over the given text in the input file */
bool SimContext::readAndSkipPast(const char* searchStr, FILE* file) {
	char temp[513];
	fscanf(file, "%512s", temp);
	while ( strcmp(temp, searchStr) != 0 ) {
		fscanf(file, "%512s", temp);
		if ( feof(file) ) {
		  //printf("\nWARNING: unexpected end of input file. Looking for %s",searchStr);
			return false;
		}
	}
	return true;
}  // readAndSkipPast
bool SimContext::readAndSkipPast2( const char* searchStr1, const char* searchStr2, FILE* file ) {
	bool ret = readAndSkipPast(searchStr1, file);
	if (ret == true)
		ret = readAndSkipPast(searchStr2, file);
	return ret;
}  // readAndSkipPast2

