#include "include.h"

/* Constructor takes in the patient object */
StateUpdater::StateUpdater(Patient *patient) :
		patient(patient)
{

}

/* Destructor is empty, no cleanup required */
StateUpdater::~StateUpdater(void) {

}

/* Virtual function to perform the initial updates upon patient creation */
void StateUpdater::performInitialUpdates() {
	// Copy the pointers to the simContext, runStats, and tracer objects
	this->simContext = this->patient->simContext;
	this->runStats = this->patient->runStats;
	this->tracer = this->patient->tracer;
}

/* Virtual function to perform all updates for a simulated month */
void StateUpdater::performMonthlyUpdates() {
	// Empty for now, no actions to perform if child does not override
}

/* Virtual function changes the inputs the updater uses to determine disease progression -- to be used primarily by the transmission model*/
void StateUpdater::setSimContext(SimContext *newSimContext){
	this->simContext = newSimContext;
}

/* initializePatient initializes the patients basic state */
void StateUpdater::initializePatient(int patientNum, bool tracingEnabled) {
	patient->generalState.patientNum = patientNum;
	patient->generalState.tracingEnabled = tracingEnabled;
	//ERINWASHERE
	patient->generalState.monthNum = patient->generalState.initialMonthNum;
	patient->generalState.costsDiscounted = 0;
	patient->generalState.LMsDiscounted = 0;
	patient->generalState.qualityAdjustLMsDiscounted = 0;
	patient->generalState.discountFactor = 1.0;
	patient->generalState.loggedPatientOIs = false;
	patient->diseaseState.isAlive = true;
}

/* setPatientAgeGender set the patients age and gender */
void StateUpdater::setPatientAgeGender(SimContext::GENDER_TYPE gender, int ageMonths) {
	patient->generalState.gender = gender;
	patient->generalState.ageMonths = ageMonths;
	patient->generalState.ageCategoryClinical = getAgeCategoryClinical(ageMonths);
	patient->generalState.ageCategoryHIVInfection = getAgeCategoryHIVInfection(ageMonths);
	patient->generalState.ageCategoryPediatrics = getAgeCategoryPediatrics(ageMonths);
} /* end setPatientAgeGender */

/* setInitialARTState initializes the patients ARTState object */
void StateUpdater::setInitialARTState() {
	patient->artState.isOnART = false;
	patient->artState.hasTakenART = false;
	patient->artState.numObservedFailures = 0;
	patient->artState.overallCD4Envelope.isActive = false;
	patient->artState.indivCD4Envelope.isActive = false;
	patient->artState.overallCD4PercentageEnvelope.isActive = false;
	patient->artState.indivCD4PercentageEnvelope.isActive = false;
	patient->artState.hadPrevToxicity = false;
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		patient->artState.numObservedOIsSinceFailOrStopART[i] = 0;
	}
	for (int i = 0; i < SimContext::ART_NUM_LINES; i++) {
		patient->artState.numMonthsOnUnsuccessfulByRegimen[i] = 0;
	}
	for (int i = 0; i < SimContext::HVL_NUM_STRATA; i++) {
		patient->artState.numMonthsOnUnsuccessfulByHVL[i] = 0;
	}
	patient->artState.activeToxicityEffects.clear();
} /* end setInitialARTState */

/* setInitialProphState initializes the patients ProphState object */
void StateUpdater::setInitialProphState() {
	// Initialize state to reflect that no drugs are currently taken and
	patient->prophState.currTotalNumProphsOn = 0;
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		patient->prophState.isOnProph[i] = false;
		for (int j = 0; j < SimContext::PROPH_NUM_TYPES; j++){
			patient->prophState.hasTakenProph[i][j] = false;
		}
	}
} /* end setInitialProphState */

/* setInitialTBProphState sets the initial TB proph state to not be on any TB prophs */
void StateUpdater::setInitialTBProphState() {
	patient->tbState.isOnProph = false;
	patient->tbState.isScheduledForProph = false;
} /* end setInitialTBProphState */

/* setInitialTBTreatmentState sets the initial TB treatment state to not be on TB treatment */
void StateUpdater::setInitialTBTreatmentState() {
	patient->tbState.isOnTreatment = false;
	patient->tbState.isScheduledForTreatment = false;
} /* end setInitialTBTreatmentState */

/* incrementMonth updates the simulation month number and patient age */
void StateUpdater::incrementMonth() {
	patient->generalState.monthNum++;
	patient->generalState.ageMonths++;

	// Update the clinical, HIV infection, and pediatrics age category
	patient->generalState.ageCategoryClinical = getAgeCategoryClinical(patient->getGeneralState()->ageMonths);
	patient->generalState.ageCategoryHIVInfection = getAgeCategoryHIVInfection(patient->getGeneralState()->ageMonths);
	patient->generalState.ageCategoryPediatrics = getAgeCategoryPediatrics(patient->getGeneralState()->ageMonths);
} /* end incrementMonth */

/* incrementDiscountFactor adjusts the discounting factor for each new month */
void StateUpdater::incrementDiscountFactor(double amount) {
	patient->generalState.discountFactor *= (1 / amount);
} /* end incrementDiscountFactor */

/* setQOLMultiplier resets the quality of life factor back to 1 */
void StateUpdater::setQOLMultiplier(double newQOL) {
	patient->generalState.QOLMultiplier = 1.0;
} /* end setQOLMultiplier */

/* accumulateQOLMultiplier accumulates the QOL by multiplying the new factor with the existing one */
void StateUpdater::accumulateQOLMultiplier(double amount) {
	patient->generalState.QOLMultiplier *= amount;
} /* end accumulateQOLMultiplier */

/* setNonAIDSDeathRateMultiplier sets the value for additional probability of nonAIDS death */
void StateUpdater::setNonAIDSDeathRateMultiplier(double amount) {
	patient->generalState.nonAIDSDeathRateMultiplier = amount;
} /* end setNonAIDSDeathRateMultiplier */

/* accumulateNonAIDSDeathRateMultiplier increments the value for additional probability of nonAIDS death */
void StateUpdater::accumulateNonAIDSDeathRateMultiplier(double amount) {
	patient->generalState.nonAIDSDeathRateMultiplier *= amount;
} /* end accumulateNonAIDSDeathRateMultiplier */

/* setInfectedHIVState sets the patients to being HIV infected and updates statistics */
void StateUpdater::setInfectedHIVState(SimContext::HIV_INF infectedState, bool isInitial, bool isHighRisk) {
	patient->diseaseState.infectedHIVState = infectedState;
	if (infectedState != SimContext::HIV_INF_NEG) {
		patient->diseaseState.monthOfHIVInfection = patient->generalState.monthNum;
	}
	if (infectedState == SimContext::HIV_INF_ACUTE_SYN) {
		patient->diseaseState.monthOfAcuteToChronicHIV = patient->generalState.monthNum + simContext->getHIVTestInputs()->monthsFromAcuteToChronic;
	}
	if (isInitial) {
		if (infectedState == SimContext::HIV_INF_NEG) {
			patient->diseaseState.isPrevalentHIVCase = false;
			patient->monitoringState.isHighRiskForHIV = isHighRisk;
		}
		else {
			patient->diseaseState.isPrevalentHIVCase = true;
		}
	}

	// Update statistics for a prevalent or incident infection
	if (isInitial) {
		SimContext::HIV_EXT_INF extInfectedState = (SimContext::HIV_EXT_INF) infectedState;
		if ((infectedState == SimContext::HIV_INF_NEG) && !isHighRisk)
			extInfectedState = SimContext::HIV_EXT_INF_NEG_LO;
		runStats->hivScreening.numPatientsInitialHIVState[extInfectedState]++;
		if (infectedState == SimContext::HIV_INF_NEG) {
			runStats->hivScreening.numHIVNegative++;
		}
		else if (infectedState == SimContext::HIV_INF_SYMP_CHR_POS) {
			runStats->hivScreening.numPatientsInitialHIVState[SimContext::HIV_INF_ASYMP_CHR_POS]--;
		}
		else {
			runStats->hivScreening.numPrevalentCases++;
		}
	}
	else if (infectedState == SimContext::HIV_INF_ACUTE_SYN) {
		runStats->hivScreening.numIncidentCases++;
		runStats->hivScreening.monthsToInfectionSum += patient->generalState.monthNum;
		runStats->hivScreening.monthsToInfectionSumSquares += patient->generalState.monthNum * patient->generalState.monthNum;
		RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
		if (currTime) {
			currTime->numIncidentHIVInfections++;
		}
	}
} /* end setInfectedHIVState */

/* setInfectedPediatricsHIVState sets the pediatrics HIV state and updates statistics */
void StateUpdater::setInfectedPediatricsHIVState(SimContext::PEDS_HIV_STATE hivState, bool isInitial) {
	patient->diseaseState.infectedPediatricsHIVState = hivState;
} /* end setInfectedPediatricsHIVState */

/* setInfectedMaternalHIVState sets the maternal HIV state for pediatrics and updates statistics */
void StateUpdater::setInfectedMaternalHIVState(SimContext::PEDS_MOM_HIV_STATE hivState, bool isInitial) {
	patient->generalState.isMotherAlive = true;
	patient->generalState.maternalInfectedHIVState = hivState;
	if (hivState != SimContext::PEDS_MOM_HIV_NEG)
		patient->generalState.monthOfMaternalHIVInfection = patient->generalState.monthNum;

	if (isInitial) {
		runStats->initialDistributions.numInitialPediatrics[patient->diseaseState.infectedPediatricsHIVState][hivState]++;
	}
} /* end setInfectedMaternalHIVState */

/* setBreastfeedingStatus for pediatrics and updates statistics */
void StateUpdater::setBreastfeedingStatus(SimContext::PEDS_BF_TYPE bfType) {
	patient->generalState.breastfeedingStatus = bfType;
} /* end setBreastfeedingStatus */

/* setPediatricsART sets whether or not the infant is on ART */
void StateUpdater::setPediatricsART(bool isOnART) {
	patient->artState.isOnPediatricART = isOnART;
} /* end setPediatricsART */

/* setDetectedHIVState sets the patients to being detected as HIV positive and updates statistics */
void StateUpdater::setDetectedHIVState(bool isDetected, SimContext::HIV_DET typeDetection, SimContext::OI_TYPE oiType) {
	patient->monitoringState.isDetectedHIVPositive = isDetected;
	// Update statistics for a newly detected patient, if screening module is enabled
	if (isDetected) {
		double cd4Value = patient->diseaseState.currTrueCD4;
		SimContext::CD4_STRATA cd4Strata = patient->diseaseState.currTrueCD4Strata;
		SimContext::HVL_STRATA hvlStrata = patient->diseaseState.currTrueHVLStrata;
		SimContext::HIV_INF hivState = patient->diseaseState.infectedHIVState;
		int monthNum = patient->generalState.monthNum;
		int ageMonths = patient->generalState.ageMonths;
		SimContext::GENDER_TYPE gender = patient->generalState.gender;
		runStats->hivScreening.numDetectedGender[gender]++;
		if (typeDetection == SimContext::HIV_DET_OI) {
			runStats->hivScreening.numDetectedByOIs[oiType]++;
		}
		if (patient->diseaseState.isPrevalentHIVCase) {
			runStats->hivScreening.numDetectedPrevalentMeans[typeDetection]++;
			runStats->hivScreening.numAtDetectionPrevalentCD4HIV[cd4Strata][hivState]++;
			runStats->hivScreening.numAtDetectionPrevalentHVLHIV[hvlStrata][hivState]++;
			runStats->hivScreening.CD4AtDetectionPrevalentSumHIV[hivState] += cd4Value;
			runStats->hivScreening.monthsToDetectionPrevalentSum += monthNum;
			runStats->hivScreening.monthsToDetectionPrevalentSumSquares += monthNum * monthNum;
			runStats->hivScreening.ageMonthsAtDetectionPrevalentSum += ageMonths;
			runStats->hivScreening.ageMonthsAtDetectionPrevalentSumSquares += ageMonths * ageMonths;
		}
		else {
			runStats->hivScreening.numDetectedIncidentMeans[typeDetection]++;
			runStats->hivScreening.numAtDetectionIncidentCD4HIV[cd4Strata][hivState]++;
			runStats->hivScreening.numAtDetectionIncidentHVLHIV[hvlStrata][hivState]++;
			runStats->hivScreening.CD4AtDetectionIncidentSumHIV[hivState] += cd4Value;
			runStats->hivScreening.monthsToDetectionIncidentSum += monthNum;
			runStats->hivScreening.monthsToDetectionIncidentSumSquares += monthNum * monthNum;
			runStats->hivScreening.ageMonthsAtDetectionIncidentSum += ageMonths;
			runStats->hivScreening.ageMonthsAtDetectionIncidentSumSquares += ageMonths * ageMonths;
			int monthDiff = monthNum - patient->diseaseState.monthOfHIVInfection;
			runStats->hivScreening.monthsAfterInfectionToDetectionSum += monthDiff;
			runStats->hivScreening.monthsAfterInfectionToDetectionSumSquares += monthDiff * monthDiff;
		}
	}
} /* end setDetectedHIVState */

/* updateHIVTestingStats updates all statistics after an HIV testing event */
void StateUpdater::updateHIVTestingStats(bool acceptTest, bool returnResults, bool isPositive) {
	if (!acceptTest) {
		runStats->hivScreening.numRefuseTest++;
		return;
	}
	SimContext::HIV_EXT_INF infectedState;
	if (!patient->monitoringState.isHighRiskForHIV && (patient->diseaseState.infectedHIVState == SimContext::HIV_INF_NEG)) {
		infectedState = SimContext::HIV_EXT_INF_NEG_LO;
	}
	else {
		infectedState = (SimContext::HIV_EXT_INF) patient->diseaseState.infectedHIVState;
	}
	runStats->hivScreening.numTestsHIVState[infectedState]++;
	runStats->hivScreening.numAcceptTest++;

	if (!returnResults) {
		runStats->hivScreening.numNoReturnForResults++;
		return;
	}
	runStats->hivScreening.numReturnForResults++;

	if (isPositive) {
		if (patient->diseaseState.infectedHIVState == SimContext::HIV_INF_NEG) {
			runStats->hivScreening.numTestResultsHIVNegativeType[SimContext::TEST_FALSE_POS]++;
		}
		else if (patient->diseaseState.isPrevalentHIVCase) {
			runStats->hivScreening.numTestResultsPrevalentType[SimContext::TEST_TRUE_POS]++;
		}
		else {
			runStats->hivScreening.numTestResultsIncidentType[SimContext::TEST_TRUE_POS]++;
		}
	}
	else {
		if (patient->diseaseState.infectedHIVState == SimContext::HIV_INF_NEG) {
			runStats->hivScreening.numTestResultsHIVNegativeType[SimContext::TEST_TRUE_NEG]++;
		}
		else if (patient->diseaseState.isPrevalentHIVCase) {
			runStats->hivScreening.numTestResultsPrevalentType[SimContext::TEST_FALSE_NEG]++;
		}
		else {
			runStats->hivScreening.numTestResultsIncidentType[SimContext::TEST_FALSE_NEG]++;
		}
	}
} /* end updateHIVTestingStats */

/* setHIVTestingParams sets the interval and acceptanc rate for HIV testing */
void StateUpdater::setHIVTestingParams(int intervalIndex, int acceptanceRateIndex) {
	SimContext::HIV_EXT_INF extInfectedState = (SimContext::HIV_EXT_INF) patient->diseaseState.infectedHIVState;
	if ((patient->diseaseState.infectedHIVState == SimContext::HIV_INF_NEG) && !patient->getMonitoringState()->isHighRiskForHIV)
		extInfectedState = SimContext::HIV_EXT_INF_NEG_LO;
	patient->monitoringState.intervalHIVTest = simContext->getHIVTestInputs()->HIVTestingInterval[intervalIndex];
	patient->monitoringState.acceptanceRateHIVTest = simContext->getHIVTestInputs()->HIVTestAcceptRate[extInfectedState][acceptanceRateIndex];

	runStats->hivScreening.numTestingInterval[intervalIndex]++;
	runStats->hivScreening.numTestingAcceptRate[acceptanceRateIndex][extInfectedState]++;
} /* setHIVTestingParams */

/* scheduleHIVTest sets the month of the next HIV test */
void StateUpdater::scheduleHIVTest(bool hasNext, int monthNum) {
	patient->monitoringState.hasScheduledHIVTest = hasNext;
	if (hasNext)
		patient->monitoringState.monthOfScheduledHIVTest = monthNum;
} /* end scheduleHIVTest */

/* scheduleCD4Test sets the month of the next CD4 test */
void StateUpdater::scheduleCD4Test(bool hasNext, int monthNum) {
	patient->monitoringState.hasScheduledCD4Test = hasNext;
	if (hasNext)
		patient->monitoringState.monthOfScheduledCD4Test = monthNum;
} /* end scheduleCD4Test */

/* scheduleHVLTest sets the month of the next HVL test */
void StateUpdater::scheduleHVLTest(bool hasNext, int monthNum) {
	patient->monitoringState.hasScheduledHVLTest = hasNext;
	if (hasNext)
		patient->monitoringState.monthOfScheduledHVLTest = monthNum;
} /* end scheduleHVLTest */

/* setClinicVisitType sets the conditions for a clinic visit and available treatments */
void StateUpdater::setClinicVisitType(SimContext::CLINIC_VISITS visitType, SimContext::THERAPY_IMPL treatmentType) {
	patient->monitoringState.clinicVisitType = visitType;
	switch (treatmentType) {
		case SimContext::THERAPY_IMPL_NONE:
			patient->prophState.mayReceiveProph = false;
			patient->artState.mayReceiveART = false;
			break;
		case SimContext::THERAPY_IMPL_PROPH:
			patient->prophState.mayReceiveProph = true;
			patient->artState.mayReceiveART = false;
			break;
		case SimContext::THERAPY_IMPL_PROPH_ART:
		default:
			patient->prophState.mayReceiveProph = true;
			patient->artState.mayReceiveART = true;
	}
} /* end setClinicVisitType */

/* setARTResponseBaseline sets the baseline propensity to respond coefficient */
void StateUpdater::setARTResponseBaseline(double baseline) {
	patient->artState.responseBaselineLogit = baseline;
} /* end setARTResponseBaseline */

/* setCD4ResponseType sets the predisposed CD4 response type of the patient */
void StateUpdater::setCD4ResponseType(SimContext::CD4_RESPONSE_TYPE responseType) {
	patient->artState.CD4ResponseType = responseType;
} /* end setCD4ResponseType */

/* setRiskFactor sets whether or not the patient has risk factor x */
void StateUpdater::setRiskFactor(int riskNum, bool hasRisk, bool isInitial) {
	patient->generalState.hasRiskFactor[riskNum] = hasRisk;

	if (isInitial && hasRisk) {
		runStats->initialDistributions.numRiskFactors[riskNum]++;
	}
} /* end setRiskFactor */

/* scheduleInitialClinicVisit sets the month of initial clinic visit, CD4 test, and HVL test */
void StateUpdater::scheduleInitialClinicVisit() {
	int monthNum = patient->generalState.monthNum;
	scheduleRegularClinicVisit(true, monthNum);
	if (patient->diseaseState.currTrueCD4 > simContext->getTreatmentInputs()->testingIntervalCD4Threshold) {
		if (simContext->getTreatmentInputs()->CD4TestingIntervalPreARTHighCD4 != SimContext::NOT_APPL)
			scheduleCD4Test(true, monthNum);
		else
			scheduleCD4Test(false);
		if (simContext->getTreatmentInputs()->HVLTestingIntervalPreARTHighCD4 != SimContext::NOT_APPL)
			scheduleHVLTest(true, monthNum);
		else
			scheduleHVLTest(false);
	}
	else {
		if (simContext->getTreatmentInputs()->CD4TestingIntervalPreARTLowCD4 != SimContext::NOT_APPL)
			scheduleCD4Test(true, monthNum);
		else
			scheduleCD4Test(false);
		if (simContext->getTreatmentInputs()->HVLTestingIntervalPreARTLowCD4 != SimContext::NOT_APPL)
			scheduleHVLTest(true, monthNum);
		else
			scheduleHVLTest(false);
	}
} /* end scheduleInitialClinicVisit */

/* scheduleRegularClinicVisit sets the month of the next regularly scheduled clinic visit */
void StateUpdater::scheduleRegularClinicVisit(bool hasNext, int monthNum) {
	patient->monitoringState.hasRegularClinicVisit = hasNext;
	if (hasNext)
		patient->monitoringState.monthOfRegularClinicVisit = monthNum;
} /* end scheduleRegularClinicVisit */

/* scheduleEmergencyClinicVisit sets the month of the next emergency clinic visit */
void StateUpdater::scheduleEmergencyClinicVisit(bool hasNext, int monthNum) {
	patient->monitoringState.hasEmergencyClinicVisit = hasNext;
	if (hasNext)
		patient->monitoringState.monthOfEmergencyClinicVisit = monthNum;
} /* end scheduleEmergencyClinicVisit */

/* resetCliniVisitState resets state keeping track of event since the last clinic visit */
void StateUpdater::resetClinicVisitState(bool isInitial) {
	if (isInitial)
		patient->monitoringState.hadPrevClinicVisit = false;
	else
		patient->monitoringState.hadPrevClinicVisit = true;
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		if (isInitial)
			patient->monitoringState.numObservedOIsTotal[i] = 0;
		patient->diseaseState.numTrueOIsSinceLastVisit[i] = 0;
		patient->monitoringState.numObservedOIsSinceLastVisit[i] = 0;
	}
}

/* incrementNumClinicVisits increments the total number of clinic visits */
void StateUpdater::incrementNumClinicVisits() {
	runStats->popSummary.totalClinicVisits++;
} /* end incrementNumClinicVisits */

/* incrementNumObservedOIs increments the patients observed OIs during a clinic visit */
void StateUpdater::incrementNumObservedOIs(SimContext::OI_TYPE oiType, int numObserved) {
	// Update the patient state for the observed OI
	patient->monitoringState.numObservedOIsTotal[oiType] += numObserved;
	patient->monitoringState.numObservedOIsSinceLastVisit[oiType] += numObserved;
	patient->artState.numObservedOIsSinceFailOrStopART[oiType] += numObserved;

	// Update the statistics for the observed OI
	SimContext::CD4_STRATA cd4Strata = patient->diseaseState.currTrueCD4Strata;
	runStats->oiStats.numDetectedOIsCD4OI[cd4Strata][oiType] += numObserved;
} /* end incrementNumObservedOIs */

/* setCurrLTFUStats updates the state and statisitics for a patient being LTFU or RTC */
void StateUpdater::setCurrLTFUState(SimContext::LTFU_STATE ltfuState) {
	// Update patient state for initial state, lost to follow up, or return to care
	patient->monitoringState.currLTFUState = ltfuState;
	int monthsPrevState = patient->generalState.monthNum - patient->monitoringState.monthOfLTFUStateChange;
	patient->monitoringState.monthOfLTFUStateChange = patient->generalState.monthNum;
	if (ltfuState == SimContext::LTFU_STATE_LOST) {
		if (patient->artState.isOnART)
			patient->monitoringState.wasOnARTWhenLostToFollowUp = true;
		else
			patient->monitoringState.wasOnARTWhenLostToFollowUp = false;
	}

	// Update statistics for lost to follow up or return to care
	if (ltfuState == SimContext::LTFU_STATE_LOST) {
		runStats->ltfuStats.numLostToFollowUpCD4[patient->diseaseState.currTrueCD4Strata]++;
		if (!patient->monitoringState.hadPrevLTFU)
			runStats->ltfuStats.numPatientsLost++;
		if (patient->artState.isOnART)
			runStats->ltfuStats.numLostToFollowUpART[patient->artState.currRegimenNum]++;
		else if (!patient->artState.hasTakenART)
			runStats->ltfuStats.numLostToFollowUpPreART++;
		else
			runStats->ltfuStats.numLostToFollowUpPostART++;
		// Update longitudinal statistics for LTFU
		RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
		if (currTime) {
			if (patient->artState.isOnART)
				currTime->numLostToFollowUpART[patient->artState.currRegimenNum]++;
			else if (!patient->artState.hasTakenART)
				currTime->numLostToFollowUpPreART++;
			else
				currTime->numLostToFollowUpPostART++;
		}
	}
	else if (ltfuState == SimContext::LTFU_STATE_RETURNED) {
		runStats->ltfuStats.numReturnToCareCD4[patient->diseaseState.currTrueCD4Strata]++;
		runStats->ltfuStats.monthsLostBeforeReturnSum += monthsPrevState;
		runStats->ltfuStats.monthsLostBeforeReturnSumSquares += monthsPrevState * monthsPrevState;
		if (!patient->monitoringState.hadPrevRTC)
			runStats->ltfuStats.numPatientsReturned++;
		if (patient->monitoringState.wasOnARTWhenLostToFollowUp &&
			patient->artState.hasNextRegimenAvailable) {
				if (patient->artState.nextRegimenNum == patient->artState.prevRegimenNum)
					runStats->ltfuStats.numReturnOnPrevART[patient->artState.nextRegimenNum]++;
				else
					runStats->ltfuStats.numReturnOnNextART[patient->artState.prevRegimenNum]++;
		}
		else if (!patient->artState.hasTakenART) {
			runStats->ltfuStats.numReturnToCarePreART++;
		}
		else {
			runStats->ltfuStats.numReturnToCarePostART++;
		}
		// Update longitudinal statistics for RTC
		RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
		if (currTime) {
			if (patient->monitoringState.wasOnARTWhenLostToFollowUp &&
				patient->artState.hasNextRegimenAvailable) {
					if (patient->artState.nextRegimenNum == patient->artState.prevRegimenNum)
						currTime->numReturnOnPrevART[patient->artState.nextRegimenNum]++;
					else
						currTime->numReturnOnNextART[patient->artState.prevRegimenNum]++;
			}
			else if (!patient->artState.hasTakenART)
				currTime->numReturnToCarePreART++;
			else
				currTime->numReturnToCarePostART++;
		}
	}

	// Update previous LTFU/RTC state after updating statistics
	if (ltfuState == SimContext::LTFU_STATE_NONE) {
		patient->monitoringState.hadPrevLTFU = false;
		patient->monitoringState.hadPrevRTC = false;
	}
	else if ((ltfuState == SimContext::LTFU_STATE_LOST) && !patient->monitoringState.hadPrevLTFU) {
		patient->monitoringState.hadPrevLTFU = true;
	}
	else if ((ltfuState == SimContext::LTFU_STATE_RETURNED) && !patient->monitoringState.hadPrevRTC) {
		patient->monitoringState.hadPrevRTC = true;
	}
} /* end setCurrLTFUState */

/* startNextARTRegimen updates the state to begin the next ART treatment regimen */
void StateUpdater::startNextARTRegimen() {
	// Update patient state for beginning ART regimen
	patient->artState.isOnART = true;
	patient->artState.currRegimenNum = patient->artState.nextRegimenNum;
	patient->artState.monthOfCurrRegimenStart = patient->generalState.monthNum;
	patient->artState.numFailedCD4Tests = 0;
	patient->artState.numFailedHVLTests = 0;
	patient->artState.numFailedOIs = 0;
	patient->artState.hasObservedFailure = false;
	patient->artState.typeObservedFailure = SimContext::ART_FAIL_NOT_FAILED;
	if (!patient->artState.hasTakenART || (patient->artState.currRegimenNum != patient->artState.prevRegimenNum)) {
		patient->artState.indivCD4Envelope.isActive = false;
		patient->artState.indivCD4PercentageEnvelope.isActive = false;
	}
	if (patient->monitoringState.hasObservedCD4)
		patient->artState.maxObservedCD4OnCurrART = patient->monitoringState.currObservedCD4Strata;
	if (patient->monitoringState.hasObservedCD4Percentage)
		patient->artState.maxObservedCD4PercentageOnCurrART = patient->monitoringState.currObservedCD4Percentage;
	if (patient->monitoringState.hasObservedHVLStrata) {
		patient->artState.observedHVLStrataAtRegimenStart = patient->monitoringState.currObservedHVLStrata;
		patient->artState.minObservedHVLStrataOnCurrART = patient->monitoringState.currObservedHVLStrata;
	}
	if (!patient->artState.hasTakenART)
		patient->artState.hasTakenART = true;

	// ART initiation statistics are only set once the efficacy is determined
} /* end startNextARTRegimen */

/* startNextARTSubRegimen updates the state to begin the next ART treatment subregimen */
void StateUpdater::startNextARTSubRegimen(int nextSubRegimen) {
	patient->artState.currSubRegimenNum = nextSubRegimen;
	patient->artState.monthOfCurrSubRegimenStart = patient->generalState.monthNum;
	patient->artState.hasMajorToxicity = false;
	patient->artState.hasSevereToxicity = false;
} /* end startNextARTSubRegmin */

/* setCurrARTEfficacy updates the destined efficacy of the ART regimen */
void StateUpdater::setCurrARTEfficacy(SimContext::ART_EFF_TYPE efficacyType, bool isInitial) {
	// Update patient state with new ART suppresseion level
	patient->artState.currRegimenEfficacy = efficacyType;
	// Set month of efficacy change for initial draw or transition to failure
	if (isInitial || (efficacyType == SimContext::ART_EFF_FAILURE)) {
		patient->artState.monthOfEfficacyChange = patient->getGeneralState()->monthNum;
	}

	// Update statistics for the initial efficacy and true failure, dont update for STI restarts
	if (patient->getARTState()->currSTIState == SimContext::STI_STATE_RESTART)
		return;
	if (isInitial) {
		// Update statistics for beginning ART regimen, don't update for STI restarts
		int artLineNum = patient->artState.currRegimenNum;
		SimContext::CD4_STRATA cd4Strata = patient->diseaseState.currTrueCD4Strata;
		SimContext::HVL_STRATA hvlStrata = patient->diseaseState.currTrueHVLStrata;
		SimContext::RESP_TYPE responseType = patient->artState.responseTypeCurrRegimen;
		SimContext::CD4_RESPONSE_TYPE cd4Response = patient->artState.CD4ResponseType;
		runStats->artStats.distributionAtInit[artLineNum][cd4Strata][hvlStrata]++;
		runStats->artStats.numOnARTAtInitResp[artLineNum][responseType]++;
		runStats->artStats.trueCD4AtInitSumResp[artLineNum][responseType] += patient->diseaseState.currTrueCD4;
		if (patient->monitoringState.hasObservedCD4)
			runStats->artStats.observedCD4AtInitSumResp[artLineNum][responseType] += patient->monitoringState.currObservedCD4;
		runStats->artStats.numDrawEfficacyAtInitResp[artLineNum][efficacyType][responseType]++;
		runStats->artStats.numCD4ResponseTypeAtInitResp[artLineNum][cd4Response][responseType]++;
		for (int i = 0; i < SimContext::RISK_FACT_NUM; i++) {
			if (patient->getGeneralState()->hasRiskFactor[i])
				runStats->artStats.numWithRiskFactorAtInitResp[artLineNum][i][responseType]++;
		}
	}
	if (efficacyType == SimContext::ART_EFF_FAILURE) {
		// Update true failure statistics, same as initial if efficacy was failure
		int artLineNum = patient->artState.currRegimenNum;
		SimContext::RESP_TYPE responseType = patient->artState.responseTypeCurrRegimen;
		runStats->artStats.numTrueFailureResp[artLineNum][responseType]++;
		runStats->artStats.trueCD4AtTrueFailureSumResp[artLineNum][responseType] += patient->diseaseState.currTrueCD4;
		if (patient->monitoringState.hasObservedCD4)
			runStats->artStats.observedCD4AtTrueFailureSumResp[artLineNum][responseType] += patient->monitoringState.currObservedCD4;
		int monthsToFail = patient->generalState.monthNum - patient->getARTState()->monthOfCurrRegimenStart;
		runStats->artStats.monthsToTrueFailureSumResp[artLineNum][responseType] += monthsToFail;
		runStats->artStats.monthsToTrueFailureSumSquaresResp[artLineNum][responseType] += monthsToFail * monthsToFail;
	}
} /* end setCurrARTEfficacy */

/* setCurrARTResponse sets the calculated ART propensity to respond and response type */
void StateUpdater::setCurrARTResponse(double propRespond) {
	// Update the patient state with the propensity to respond and response type
	int artLineNum = patient->artState.currRegimenNum;
	patient->artState.responsePropensityCurrRegimen = propRespond;
	if (propRespond > simContext->getHeterogeneityInputs()->responseTypeThresholds[artLineNum][0]) {
		patient->artState.responseTypeCurrRegimen = SimContext::RESP_TYPE_FULL;
		patient->artState.responseFactorCurrRegimen = 1.0;
	}
	else if (propRespond > simContext->getHeterogeneityInputs()->responseTypeThresholds[artLineNum][1]) {
		patient->artState.responseTypeCurrRegimen = SimContext::RESP_TYPE_PARTIAL;
		double propUpper = simContext->getHeterogeneityInputs()->responseTypeThresholds[artLineNum][0];
		double propLower = simContext->getHeterogeneityInputs()->responseTypeThresholds[artLineNum][1];
		patient->artState.responseFactorCurrRegimen = (propRespond - propLower) / (propUpper - propLower);
	}
	else {
		patient->artState.responseTypeCurrRegimen = SimContext::RESP_TYPE_NON;
		patient->artState.responseFactorCurrRegimen = 0.0;
	}
} /* end setCurrARTResponse */

/* setTargetHVLStrata updates the target HVL while on ART or post ART */
void StateUpdater::setTargetHVLStrata(SimContext::HVL_STRATA targetHVL) {
	patient->diseaseState.targetHVLStrata = targetHVL;
} /* end setTargetHVLStrata */

/* setCurrRegimenCD4Slope sets the CD4 slope for the current ART regimen */
void StateUpdater::setCurrRegimenCD4Slope(double cd4Slope) {
	patient->artState.currRegimenCD4Slope = cd4Slope;
} /* end setCurrRegimenCD4Slope */

/* setCurrRegimenCD4PercentageSlope sets the CD4 percentage slope for the current ART regimen */
void StateUpdater::setCurrRegimenCD4PercentageSlope(double cd4PercSlope) {
	patient->artState.currRegimenCD4PercentageSlope = cd4PercSlope;
} /* end setCurrRegimenCD4PercentageSlope */

/* setCD4EnvelopeRegimen initializes the specified CD4 envelope type */
void StateUpdater::setCD4EnvelopeRegimen(SimContext::ENVL_CD4_TYPE envelopeType, int artLineNum) {
	SimContext::CD4Envelope *envelope;
	if (envelopeType == SimContext::ENVL_CD4_OVERALL) {
		envelope = &(patient->artState.overallCD4Envelope);
		envelope->value = patient->diseaseState.currTrueCD4;
	}
	else if (envelopeType == SimContext::ENVL_CD4_INDIV) {
		envelope = &(patient->artState.indivCD4Envelope);
		envelope->value = patient->diseaseState.currTrueCD4;
	}
	else if (envelopeType == SimContext::ENVL_CD4_PERC_OVERALL) {
		envelope = &(patient->artState.overallCD4PercentageEnvelope);
		envelope->value = patient->diseaseState.currTrueCD4Percentage;
	}
	else {
		envelope = &(patient->artState.indivCD4PercentageEnvelope);
		envelope->value = patient->diseaseState.currTrueCD4Percentage;
	}
	envelope->isActive = true;
	envelope->regimenNum = artLineNum;
	envelope->monthOfStart = patient->generalState.monthNum;
} /* end setCD4EnvelopeRegimen */

/* setCD4EnvelopeSlope updates the slope used for the specified CD4 envelope type */
void StateUpdater::setCD4EnvelopeSlope(SimContext::ENVL_CD4_TYPE envelopeType, double cd4Slope) {
	if (cd4Slope < 0)
		cd4Slope = 0;
	if (envelopeType == SimContext::ENVL_CD4_OVERALL)
		patient->artState.overallCD4Envelope.slope = cd4Slope;
	else if (envelopeType == SimContext::ENVL_CD4_INDIV)
		patient->artState.indivCD4Envelope.slope = cd4Slope;
	else if (envelopeType == SimContext::ENVL_CD4_PERC_OVERALL)
		patient->artState.overallCD4PercentageEnvelope.slope = cd4Slope;
	else
		patient->artState.indivCD4PercentageEnvelope.slope = cd4Slope;
} /* end setCD4EnvelopeSlope */

/* incrementCD4Envelope increments the specified CD4 envelope's level according to hypothetical ART success */
void StateUpdater::incrementCD4Envelope(SimContext::ENVL_CD4_TYPE envelopeType, double changeCD4) {
	if (envelopeType == SimContext::ENVL_CD4_OVERALL)
		patient->artState.overallCD4Envelope.value += changeCD4;
	else if (envelopeType == SimContext::ENVL_CD4_INDIV)
		patient->artState.indivCD4Envelope.value += changeCD4;
	else if (envelopeType == SimContext::ENVL_CD4_PERC_OVERALL)
		patient->artState.overallCD4PercentageEnvelope.value += changeCD4;
	else
		patient->artState.indivCD4PercentageEnvelope.value += changeCD4;
} /* end incrementCD4Envelope */

/* setCurrARTRegimenFailue updates the state to begin the next ART treatment regimen */
void StateUpdater::setCurrARTObservedFailure(SimContext::ART_FAIL_TYPE failType) {
	// Update patient state for observed failure of an ART regimen
	patient->artState.hasObservedFailure = true;
	patient->artState.numObservedFailures++;
	patient->artState.typeObservedFailure = failType;
	patient->artState.monthOfObservedFailure = patient->getGeneralState()->monthNum;
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		patient->artState.numObservedOIsSinceFailOrStopART[i] = 0;
	}

	// Update statistics for observed failure of an ART regimen
	int artLineNum = patient->artState.currRegimenNum;
	runStats->artStats.numObservedFailureType[artLineNum][failType]++;
	runStats->artStats.trueCD4AtObservedFailureSumType[artLineNum][failType] += patient->diseaseState.currTrueCD4;
	if (patient->monitoringState.hasObservedCD4)
		runStats->artStats.observedCD4AtObservedFailureSumType[artLineNum][failType] += patient->monitoringState.currObservedCD4;
	if (patient->artState.currRegimenEfficacy == SimContext::ART_EFF_FAILURE)
		runStats->artStats.numObservedFailureAfterTrueType[artLineNum][failType]++;
	int monthsToObserve = patient->generalState.monthNum - patient->artState.monthOfCurrRegimenStart;
	runStats->artStats.monthsToObservedFailureSumType[artLineNum][failType] += monthsToObserve;
	runStats->artStats.monthsToObservedFailureSumSquaresType[artLineNum][failType] += monthsToObserve * monthsToObserve;
} /* setCurrARTObservedFailure() */

/* stopCurrARTRegimen updates the state to begin the next ART treatment regimen */
void StateUpdater::stopCurrARTRegimen(SimContext::ART_STOP_TYPE stopType) {
	int artLineNum = patient->artState.currRegimenNum;
	int monthsToStop = patient->generalState.monthNum - patient->artState.monthOfCurrRegimenStart;

	// Update patient state for stopping an ART regimen
	patient->artState.isOnART = false;
	patient->artState.typeCurrStop = stopType;
	patient->artState.prevRegimenNum = patient->artState.currRegimenNum;
	patient->artState.prevRegimenEfficacy = patient->artState.currRegimenEfficacy;
	patient->artState.monthOfPrevRegimenStop = patient->generalState.monthNum;
	if (!patient->artState.hasObservedFailure) {
		for (int i = 0; i < SimContext::OI_NUM; i++) {
			patient->artState.numObservedOIsSinceFailOrStopART[i] = 0;
		}
	}

	// Update statistics for stopping an ART regimen, dont update for STI interrupts
	if (patient->getARTState()->currSTIState == SimContext::STI_STATE_INTERRUPT)
		return;
	runStats->artStats.numStopType[artLineNum][stopType]++;
	runStats->artStats.trueCD4AtStopSumType[artLineNum][stopType] += patient->diseaseState.currTrueCD4;
	if (patient->monitoringState.hasObservedCD4)
		runStats->artStats.observedCD4AtStopSumType[artLineNum][stopType] += patient->monitoringState.currObservedCD4;
	if (patient->artState.prevRegimenEfficacy == SimContext::ART_EFF_FAILURE)
		runStats->artStats.numStopAfterTrueFailureType[artLineNum][stopType]++;
	runStats->artStats.monthsToStopSumType[artLineNum][stopType] += monthsToStop;
	runStats->artStats.monthsToStopSumSquaresType[artLineNum][stopType] += monthsToStop * monthsToStop;
} /* stopCurrARTRegimen */

/* setNextARTRegimen updates the next ART regimen that is available for use */
void StateUpdater::setNextARTRegimen(bool hasNext, int artLineNum) {
	patient->artState.hasNextRegimenAvailable = hasNext;
	if (hasNext)
		patient->artState.nextRegimenNum = artLineNum;
} /* end setNextARTRegimen */

/* incrementMonthsUnsuccessfulART increments the number of months on failed/partial ART by HVL */
void StateUpdater::incrementMonthsUnsuccessfulART() {
	patient->artState.numMonthsOnUnsuccessfulByRegimen[patient->artState.currRegimenNum]++;
	SimContext::HVL_STRATA hvlStrata = patient->diseaseState.currTrueHVLStrata;
	patient->artState.numMonthsOnUnsuccessfulByHVL[hvlStrata]++;
} /* end incrementMonthsUnsuccessfulART */

/* addARTToxicityEffect adds the occurrence of a new toxicity to the active effects list */
void StateUpdater::addARTToxicityEffect(SimContext::ART_TOX_SEVERITY severity, int toxNum, int timeToTox) {
	SimContext::ARTToxicityEffect toxicity;
	toxicity.toxSeverityType = severity;
	toxicity.toxNum = toxNum;
	toxicity.monthOfToxStart = patient->generalState.monthNum + timeToTox;
	toxicity.ARTRegimenNum = patient->artState.currRegimenNum;
	toxicity.ARTSubRegimenNum = patient->artState.currSubRegimenNum;
	patient->artState.activeToxicityEffects.push_back(toxicity);
} /* end addARTToxicityEffect */

/* removeARTToxicityEffect removes the specified toxicity from the active effects list */
void StateUpdater::removeARTToxicityEffect(list<SimContext::ARTToxicityEffect>::const_iterator &toxIter) {
	// Slightly hackish way to get a regular iterator from a const iterator, this is necessary
	//	since erase requires a regular iterator
	list<SimContext::ARTToxicityEffect>::const_iterator beginIter = patient->artState.activeToxicityEffects.begin();
	list<SimContext::ARTToxicityEffect>::iterator eraseIter = patient->artState.activeToxicityEffects.begin();
	advance(eraseIter, distance(beginIter, toxIter));

	// Erase the appropriate element from the toxicity list
	patient->artState.activeToxicityEffects.erase(eraseIter);
} /* end removeARTToxicityEffect */

/* setARTToxicity updates the patient state and stats for the occurrence of a toxicity */
void StateUpdater::setARTToxicity(const SimContext::ARTToxicityEffect &toxEffect) {
	// Update the patient state as a severe toxicity if it causes a subregimen switch
	const SimContext::ARTInputs::ARTToxicity &toxInputs = simContext->getARTInputs(toxEffect.ARTRegimenNum)->toxicity[toxEffect.ARTSubRegimenNum][toxEffect.toxSeverityType][toxEffect.toxNum];
	bool isSevere = false;
	if (toxEffect.toxSeverityType == SimContext::ART_TOX_MAJOR) {
		patient->artState.hasMajorToxicity = true;
		if (simContext->getTreatmentInputs()->stopART[toxEffect.ARTRegimenNum].withMajorToxicty)
			isSevere = true;
	}
	if (toxInputs.switchSubRegimenOnToxicity != SimContext::NOT_APPL)
		isSevere = true;
	if (isSevere) {
		if (!patient->artState.hasSevereToxicity) {
			patient->artState.hasSevereToxicity = true;
			patient->artState.severeToxicityEffect = &toxEffect;
		}
		else {
			const SimContext::ARTToxicityEffect *severeToxEffect = patient->artState.severeToxicityEffect;
			const SimContext::ARTInputs::ARTToxicity &severeToxInputs = simContext->getARTInputs(severeToxEffect->ARTRegimenNum)->toxicity[severeToxEffect->ARTSubRegimenNum][severeToxEffect->toxSeverityType][severeToxEffect->toxNum];
			if ((toxEffect.toxSeverityType > severeToxEffect->toxSeverityType) ||
				(toxInputs.chronicDeathIncrease > severeToxInputs.chronicDeathIncrease) ||
				(toxInputs.probAcuteDeathMajorToxicity > severeToxInputs.probAcuteDeathMajorToxicity) ||
				(toxInputs.QOLMultiplier < severeToxInputs.QOLMultiplier)) {
					patient->artState.severeToxicityEffect = &toxEffect;
			}
		}
	}
	patient->artState.hadPrevToxicity = true;

	// Update the statistics for the ART toxicity
	int currRegimen = toxEffect.ARTRegimenNum;
	SimContext::ART_TOX_SEVERITY severity = toxEffect.toxSeverityType;
	SimContext::HVL_STRATA hvlStrata = patient->diseaseState.currTrueHVLStrata;
	runStats->artStats.numToxicityCases[currRegimen][severity][hvlStrata]++;
} /* end setARTToxicity */

/* incrementARTFailedCD4Tests increments the number of failed CD4 tests counting towards ART failure */
void StateUpdater::incrementARTFailedCD4Tests() {
	patient->artState.numFailedCD4Tests++;
} /* end incrementARTFailedCD4Tests */

/* resetARTFailedCD4Tests sets the number of failed CD4 tests counting towards ART failure back to 0 */
void StateUpdater::resetARTFailedCD4Tests() {
	patient->artState.numFailedCD4Tests = 0;
} /* end resetARTFailedCD4Tests */

/* incrementARTFailedHVLTests increments the number of failed CD4 tests counting towards ART failure */
void StateUpdater::incrementARTFailedHVLTests() {
	patient->artState.numFailedHVLTests++;
} /* end incrementARTFailedHVLTests */

/* resetARTFailedHVLTests sets the number of failed HVL tests counting towards ART failure back to 0 */
void StateUpdater::resetARTFailedHVLTests() {
	patient->artState.numFailedHVLTests = 0;
} /* end resetARTFailedHVLTests */

/* incrementARTFailedOIs increments the number of observed OIs counting towards ART failure */
void StateUpdater::incrementARTFailedOIs() {
	patient->artState.numFailedOIs++;
} /* end incrementARTFailedOIs */

/* resetARTFailedOIs resets the number of observed OIs counting towards ART failure */
void StateUpdater::resetARTFailedOIs() {
	patient->artState.numFailedOIs = 0;
} /* end resetARTFailedOIs */

/* setCurrSTIState updates the STI state for the current ART regimen */
void StateUpdater::setCurrSTIState(SimContext::STI_STATE newSTIState) {
	SimContext::STI_STATE prevSTIState = patient->artState.currSTIState;
	int prevMonthOfChange = patient->artState.monthOfSTIStateChange;
	patient->artState.currSTIState = newSTIState;
	patient->artState.monthOfSTIStateChange = patient->generalState.monthNum;
	if (newSTIState == SimContext::STI_STATE_INTERRUPT) {
		patient->artState.numSTIInterruptionsOnCurrRegimen++;
		if (prevSTIState == SimContext::STI_STATE_NONE)
			patient->artState.monthOfSTIInitialStop = patient->generalState.monthNum;
	}
	else if (newSTIState == SimContext::STI_STATE_NONE) {
		patient->artState.numSTIInterruptionsOnCurrRegimen = 0;
	}

	int cycle = patient->artState.numSTIInterruptionsOnCurrRegimen - 1;
	if (cycle >= SimContext::STI_NUM_TRACKED)
		cycle = SimContext::STI_NUM_TRACKED - 1;
	if (newSTIState == SimContext::STI_STATE_INTERRUPT) {
		runStats->artStats.numSTIInterruptions[patient->artState.currRegimenNum][cycle]++;
		runStats->artStats.numPatientsWithSTIInterruptions[patient->artState.currRegimenNum][cycle]++;
	}
	else if (newSTIState == SimContext::STI_STATE_RESTART) {
		runStats->artStats.numSTIRestarts[patient->artState.nextRegimenNum][cycle]++;
		runStats->artStats.monthsOnSTIInterruptionSum[patient->artState.nextRegimenNum] += patient->generalState.monthNum - prevMonthOfChange;
	}
	else if (newSTIState == SimContext::STI_STATE_ENDPOINT) {
		runStats->artStats.numSTIEndpoints[patient->artState.currRegimenNum][cycle]++;
	}
} /* end setCurrSTIState */

/* setProphNonCompliance sets whether or not the patient complies with proph */
void StateUpdater::setProphNonCompliance(bool isNonCompliant) {
	patient->prophState.isNonCompliant = isNonCompliant;
} /* setProphNonCompliance */

/* startNextProph updates state to beginning using next proph, returns false if none available */
void StateUpdater::startNextProph(SimContext::OI_TYPE oiType) {
	SimContext::PROPH_TYPE prophType = patient->prophState.nextProphType[oiType];
	int prophNum = patient->prophState.nextProphNum[oiType];

	// Proph is available, update state to begin taking the proph
	patient->prophState.isOnProph[oiType] = true;
	patient->prophState.hasTakenProph[oiType][prophType] = true;
	patient->prophState.currProphType[oiType] = prophType;
	patient->prophState.currProphNum[oiType] = prophNum;
	patient->prophState.monthOfProphStart[oiType] = patient->generalState.monthNum;
	patient->prophState.typeProphToxicity[oiType] = SimContext::PROPH_TOX_NONE;
	patient->prophState.useProphResistance[oiType] = false;
	patient->prophState.currTotalNumProphsOn++;
	// Update statistics for starting the proph
	runStats->prophStats.trueCD4InitProphSum[prophType][oiType][prophNum] += patient->diseaseState.currTrueCD4;
	if (patient->monitoringState.hasObservedCD4)
		runStats->prophStats.observedCD4InitProphSum[prophType][oiType][prophNum] += patient->monitoringState.currObservedCD4;
	runStats->prophStats.numTimesInitProph[prophType][oiType][prophNum]++;
} /* end startNextProph */

/* stopCurrProph updates state to stop using current proph */
void StateUpdater::stopCurrProph(SimContext::OI_TYPE oiType) {
	patient->prophState.isOnProph[oiType] = false;
	patient->prophState.currTotalNumProphsOn--;
} /* end stopCurrProph */

/* setNextProph updates whether primary or secondary proph should be used for the OI */
void StateUpdater::setNextProph(bool hasNext, SimContext::PROPH_TYPE prophType, SimContext::OI_TYPE oiType, int prophNum) {
	patient->prophState.hasNextProphAvailable[oiType] = hasNext;
	if (hasNext) {
		patient->prophState.nextProphType[oiType] = prophType;
		patient->prophState.nextProphNum[oiType] = prophNum;
	}
} /* end setUserProphType */

/* setProphToxicity records the toxicity and updates stats */
void StateUpdater::setProphToxicity(bool isMajor, SimContext::OI_TYPE oiType) {
	// Update the patient state for the toxicity
	int prophNum = patient->prophState.currProphNum[oiType];
	if (isMajor)
		patient->prophState.typeProphToxicity[oiType] = SimContext::PROPH_TOX_MAJOR;
	else
		patient->prophState.typeProphToxicity[oiType] = SimContext::PROPH_TOX_MINOR;

	// Update the statistics for the toxicity occurrence
	if (isMajor)
		runStats->prophStats.numMajorToxicity[oiType][prophNum]++;
	else
		runStats->prophStats.numMinorToxicity[oiType][prophNum]++;
} /* end setProphToxicity */

/* setProphResistance updates the flag to indicate that proph resistance has occurred */
void StateUpdater::setProphResistance(SimContext::OI_TYPE oiType) {
	patient->prophState.useProphResistance[oiType] = true;
}

/* setTBDiseaseState updates the TB disease state */
void StateUpdater::setTBDiseaseState(SimContext::TB_STATE newTBState) {
	patient->tbState.currTrueTBDiseaseState = newTBState;
	patient->tbState.monthOfTBStateChange = patient->generalState.monthNum;
	if (newTBState == SimContext::TB_STATE_ACTIVE)
		patient->tbState.hasObservedHistoryActiveTB = true;
} /* end setTBDiseaseState */

/* setTBResistanceStrain updates the TB disease drug resistance */
void StateUpdater::setTBResistanceStrain(SimContext::TB_STRAIN newTBStrain) {
	patient->tbState.currTrueTBResistanceStrain = newTBStrain;
} /* end setTBResistanceStrain */

/* setNewTBInfection updates the state and statistics for a new TB infection occurring */
void StateUpdater::setNewTBInfection(SimContext::TB_INFECT infectType, bool isActive, int monthsSince) {
	// Update patient state for month of most recent infection
	patient->tbState.monthOfTBInfection = patient->generalState.monthNum;
	if (infectType == SimContext::TB_INFECT_PREVALENT) {
		patient->tbState.monthOfTBStateChange -= monthsSince;
		patient->tbState.monthOfTBInfection -= monthsSince;
	}

	// Update statistics for the new TB infection
	SimContext::TB_STRAIN tbStrain = patient->tbState.currTrueTBResistanceStrain;
	if (isActive)
		runStats->tbStats.numActiveInfections[tbStrain]++;
	else
		runStats->tbStats.numLatentInfections[tbStrain]++;
	if (infectType == SimContext::TB_INFECT_REACTIVATE)
		runStats->tbStats.numReactivationsLatent[tbStrain]++;
	else if (infectType == SimContext::TB_INFECT_REINFECT)
		runStats->tbStats.numReinfectionsLatent[tbStrain]++;
	else if (infectType == SimContext::TB_INFECT_RELAPSE)
		runStats->tbStats.numRelapsesHistoryActive[tbStrain]++;
} /* end setNewTBInfection */

/* setTBSpontaneousResolution increments number of TB spontaneous resolutions */
void StateUpdater::setTBSpontaneousResolution() {
	runStats->tbStats.numSpontaneousResolutions[patient->tbState.currTrueTBResistanceStrain]++;
} /* end setTBSpontaneousResolution */

/* startNextTBProph updates state to beginning using next TB proph */
void StateUpdater::startNextTBProph() {
	patient->tbState.isOnProph = true;
	patient->tbState.currProphNum = patient->tbState.nextProphNum;
	patient->tbState.monthOfProphStart = patient->generalState.monthNum;
	patient->tbState.isScheduledForProph = false;
	patient->tbState.hasMajorProphToxicity = false;
} /* end startNextTBProph */

/* stopCurrTBProph updates state to stop using current TB proph */
void StateUpdater::stopCurrTBProph() {
	patient->tbState.isOnProph = false;
} /* end stopCurrTBProph */

/* setNextTBProph updates proph num to be used next for TB */
void StateUpdater::setNextTBProph(bool hasNext, int prophNum) {
	patient->tbState.hasNextProphAvailable = hasNext;
	if (hasNext)
		patient->tbState.nextProphNum = prophNum;
} /* end setNextTBProph */

/* scheduleNextTBProph updates the time lag for scheduling the next TB proph */
void StateUpdater::scheduleNextTBProph(int monthStart) {
	patient->tbState.isScheduledForProph = true;
	patient->tbState.monthOfProphStart = monthStart;
} /* end scheduleNextTBProph */

/* unscheduleNextTBProph removes the next scheduled start of TB proph */
void StateUpdater::unscheduleNextTBProph() {
	patient->tbState.isScheduledForProph = false;
	patient->tbState.monthOfProphStart = SimContext::NOT_APPL;
} /* end unscheduleNextTBProph */

/* setTBProphToxicity records the TB proph toxicity */
void StateUpdater::setTBProphToxicity(bool isMajor) {
	// Update the patient state for a major toxicity
	if (isMajor)
		patient->tbState.hasMajorProphToxicity = true;

	// Update the statistics for the toxicity occurrence
	int prophNum = patient->tbState.currProphNum;
	if (isMajor)
		runStats->tbStats.numProphMajorToxicity[prophNum]++;
	else
		runStats->tbStats.numProphMinorToxicity[prophNum]++;
} /* end setTBProphToxicity */

/* startNextTBTreatment updates the patient state and statistics to begin the next TB treatment */
void StateUpdater::startNextTBTreatment() {
	// Update patient state for starting a new TB treatment stage
	patient->tbState.isOnTreatment = true;
	patient->tbState.currTreatmentStage = patient->tbState.nextTreatmentStage;
	patient->tbState.monthOfTreatmentStart = patient->generalState.monthNum;
	patient->tbState.isScheduledForTreatment = false;
	patient->tbState.hasMajorTreatmentToxicity = false;

	// Update statistics for the starting a new TB treatment stage
	SimContext::TB_TREATM_STAGE treatStage = patient->tbState.currTreatmentStage;
	SimContext::TB_STRAIN tbStrain = patient->tbState.currTrueTBResistanceStrain;
	runStats->tbStats.numStartOnTreatment[tbStrain][treatStage]++;
} /* end setNextTBTreatment */

/* stopCurrTBTreatment updates the patient state and statistics to stop the current TB treatment */
void StateUpdater::stopCurrTBTreatment(bool isFinished, bool isCured) {
	// Update patient state for stopping TB treatment
	patient->tbState.isOnTreatment = false;

	// Update the statistics for dropping out or completing treatment, and if cured
	SimContext::TB_TREATM_STAGE treatStage = patient->tbState.currTreatmentStage;
	SimContext::TB_STRAIN tbStrain = patient->tbState.currTrueTBResistanceStrain;
	if (isFinished) {
		runStats->tbStats.numFinishTreatment[tbStrain][treatStage]++;
		if (isCured)
			runStats->tbStats.numCuredAtTreatmentFinish[tbStrain][treatStage]++;
	}
	else {
		runStats->tbStats.numDropoutTreatment[tbStrain][treatStage]++;
		if (isCured)
			runStats->tbStats.numCuredAtTreatmentDropout[tbStrain][treatStage]++;
	}
} /* end stopCurrTBTreatment */

/* scheduleNextTBTreatment updates the patients next scheduled TB treatment and lag time to start */
void StateUpdater::scheduleNextTBTreatment(SimContext::TB_TREATM_STAGE treatStage, int monthStart) {
	patient->tbState.isScheduledForTreatment = true;
	patient->tbState.nextTreatmentStage = treatStage;
	patient->tbState.monthOfTreatmentStart = monthStart;
} /* end scheduleNextTBTreatment */

/* unscheduleNextTBTreatment removes the next scheduled start of TB treatment */
void StateUpdater::unscheduleNextTBTreatment() {
	patient->tbState.isScheduledForTreatment = false;
} /* end unscheduleNextTBTreatmet */

/* increaseTBDrugResistance updates that patient state and stats for an increase in TB drug resistance */
void StateUpdater::increaseTBDrugResistance(bool fromTreatment) {
	// Update the patient state for the increased drug resistance
	SimContext::TB_TREATM_STAGE treatStage = patient->tbState.currTreatmentStage;
	SimContext::TB_STRAIN tbStrain = patient->tbState.currTrueTBResistanceStrain;
	patient->tbState.currTrueTBResistanceStrain = (SimContext::TB_STRAIN) (tbStrain + 1);

	// Update the statistics for the increased drug resistance
	if (fromTreatment)
		runStats->tbStats.numIncreaseResistanceAtTreatmentFinish[tbStrain][treatStage]++;
} /* end increaseTBDrugResistance */

/* setCurrTBTreatmenToxicity records the TB proph toxicity */
void StateUpdater::setTBTreatmentToxicity(bool isMajor) {
	// Update the patient state for a major toxicity
	if (isMajor)
		patient->tbState.hasMajorTreatmentToxicity = true;

	// Update the statistics for the toxicity occurrence
	SimContext::TB_TREATM_STAGE treatStage = patient->tbState.currTreatmentStage;
	if (isMajor)
		runStats->tbStats.numTreatmentMajorToxicity[treatStage]++;
	else
		runStats->tbStats.numTreatmentMinorToxicity[treatStage]++;
} /* end setTBTreatmentToxicity */

/* setTrueCHRMsState updates the patients state for occurrence of CHRMs diseases */
void StateUpdater::setTrueCHRMsState(int chrmNum, bool hasCHRM, bool isInitial, int monthsStart) {
	patient->diseaseState.hasTrueCHRMs[chrmNum] = hasCHRM;
	if (isInitial)
		patient->diseaseState.monthOfCHRMsStart[chrmNum] = patient->generalState.monthNum - monthsStart;
	else
		patient->diseaseState.monthOfCHRMsStart[chrmNum] = patient->generalState.monthNum;

	if (hasCHRM) {
		SimContext::CD4_STRATA cd4Strata = patient->diseaseState.currTrueCD4Strata;
		if (isInitial)
			runStats->chrmsStats.numPrevalentCHRMCD4[chrmNum][cd4Strata]++;
		else
			runStats->chrmsStats.numIncidentCHRMCD4[chrmNum][cd4Strata]++;
	}
}

/* setInitialOIHistory updates the patient state for initial OI history */
void StateUpdater::setInitialOIHistory(bool hasHistory[SimContext::OI_NUM]) {
	patient->diseaseState.typeTrueOIHistory = SimContext::HIST_EXT_N;
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		patient->diseaseState.hasTrueOIHistory[i] = hasHistory[i];
		if (hasHistory[i]) {
			if (simContext->getRunSpecsInputs()->severeOIs[i]) {
				patient->diseaseState.typeTrueOIHistory = SimContext::HIST_EXT_SEVR;
			}
			else if (patient->diseaseState.typeTrueOIHistory != SimContext::HIST_EXT_SEVR) {
				patient->diseaseState.typeTrueOIHistory = SimContext::HIST_EXT_MILD;
			}
		}
	}
} /* setInitialOIHistory */

/* setOIHistory updates the patient state for OI history, called at end of month
	since current acute OI should not count as history until the next month */
void StateUpdater::setOIHistory() {
	if (patient->diseaseState.hasCurrTrueOI) {
		SimContext::OI_TYPE oiType = patient->diseaseState.typeCurrTrueOI;
		patient->diseaseState.hasTrueOIHistory[oiType] = true;
		if (simContext->getRunSpecsInputs()->severeOIs[oiType]) {
			patient->diseaseState.typeTrueOIHistory = SimContext::HIST_EXT_SEVR;
		}
		else if (patient->diseaseState.typeTrueOIHistory != SimContext::HIST_EXT_SEVR) {
			patient->diseaseState.typeTrueOIHistory = SimContext::HIST_EXT_MILD;
		}
	}
} /* end setOIHistory */

/* setCurrTrueOI updates the patient state when an acute OI event occurs or resets back to none */
void StateUpdater::setCurrTrueOI(SimContext::OI_TYPE oiType) {
	// Update the patient state for the occurrence of the OI
	if (oiType == SimContext::OI_NONE) {
		patient->diseaseState.hasCurrTrueOI = false;
		return;
	}
	patient->diseaseState.hasCurrTrueOI = true;
	patient->diseaseState.typeCurrTrueOI = oiType;
	patient->diseaseState.numTrueOIsSinceLastVisit[oiType]++;

	// Update the statistics for the occurrence of the OI
	SimContext::CD4_STRATA cd4Strata = patient->diseaseState.currTrueCD4Strata;
	if (!patient->diseaseState.hasTrueOIHistory[oiType]) {
		runStats->oiStats.numPrimaryOIsCD4OI[cd4Strata][oiType]++;
	}
	else {
		runStats->oiStats.numSecondaryOIsCD4OI[cd4Strata][oiType]++;
	}

	// Update the longitudinal statistics for the occurrence of the OI
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		SimContext::OI_TYPE oiType = patient->diseaseState.typeCurrTrueOI;
		if (!patient->diseaseState.hasTrueOIHistory[oiType]) {
			currTime->numPrimaryOIs[oiType]++;
		}
		else {
			currTime->numSecondaryOIs[oiType]++;
		}
		if ((patient->diseaseState.typeTrueOIHistory == SimContext::HIST_EXT_N) &&
			(simContext->getRunSpecsInputs()->firstOIsLongitLogging[oiType])) {
			currTime->numWithFirstOI[oiType]++;
			if (!patient->diseaseState.isAlive) {
				currTime->numDeathsFromFirstOI[oiType]++;
			}
		}
	}
} /* end setCurrTrueOI */

/* clearMortalityRisks clears the list of possible death risks for the month */
void StateUpdater::clearMortalityRisks() {
	patient->diseaseState.mortalityRisks.clear();
} /* end clearMortalityRisks */

/* addMortalityRisk adds a new risk of death for the month */
void StateUpdater::addMortalityRisk(SimContext::DTH_CAUSES causeOfDeath, double probDeath, double costDeath) {
	SimContext::MortalityRisk deathRisk;
	deathRisk.causeOfDeath = causeOfDeath;
	deathRisk.probDeath = probDeath;
	deathRisk.costDeath = costDeath;
	patient->diseaseState.mortalityRisks.push_back(deathRisk);
} /* end addMortalityRisk */

/* setCauseOfDeath updates the patient state to reflect that death has occurred */
void StateUpdater::setCauseOfDeath(SimContext::DTH_CAUSES causeOfDeath) {
	// Update the patient state to reflect that death has occurred
	patient->diseaseState.isAlive = false;
	patient->diseaseState.causeOfDeath = causeOfDeath;

	// update the run statistics to reflect that death has occurred
	if (patient->diseaseState.infectedHIVState == SimContext::HIV_INF_NEG) {
		runStats->deathStats.numDeathsUninfected++;
	}
	else {
		runStats->deathStats.numDeathsCD4Type[patient->diseaseState.currTrueCD4Strata][patient->diseaseState.causeOfDeath]++;
		runStats->deathStats.numDeathsHVLCD4[patient->diseaseState.currTrueHVLStrata][patient->diseaseState.currTrueCD4Strata]++;
		if (patient->getMonitoringState()->currLTFUState == SimContext::LTFU_STATE_LOST) {
			runStats->ltfuStats.numDeathsWhileLostCD4[patient->diseaseState.currTrueCD4Strata]++;
			if (patient->monitoringState.wasOnARTWhenLostToFollowUp) {
				runStats->ltfuStats.numDeathsWhileLostART[patient->artState.prevRegimenNum]++;
			}
			else if (!patient->artState.hasTakenART) {
				runStats->ltfuStats.numDeathsWhileLostPreART++;
			}
			else {
				runStats->ltfuStats.numDeathsWhileLostPostART++;
			}
		}
		if (causeOfDeath == SimContext::DTH_CHRAIDS) {
			if (patient->diseaseState.typeTrueOIHistory == SimContext::HIST_EXT_N)
				runStats->deathStats.numChronicAIDSDeathsNoOIHistoryCD4[patient->diseaseState.currTrueCD4Strata]++;
			else
				runStats->deathStats.numChronicAIDSDeathsOIHistoryCD4[patient->diseaseState.currTrueCD4Strata]++;
		}
		else if (causeOfDeath == SimContext::DTH_NONAIDS) {
			if (patient->diseaseState.typeTrueOIHistory == SimContext::HIST_EXT_N)
				runStats->deathStats.numNonAIDSDeathsNoOIHistoryCD4[patient->diseaseState.currTrueCD4Strata]++;
			else
				runStats->deathStats.numNonAIDSDeathsOIHistoryCD4[patient->diseaseState.currTrueCD4Strata]++;
		}
		else if (causeOfDeath == SimContext::DTH_TOX_ART) {
			int currRegimen = patient->artState.currRegimenNum;
			SimContext::HVL_STRATA hvlStrata = patient->diseaseState.currTrueHVLStrata;
			runStats->artStats.numToxicityDeaths[currRegimen][hvlStrata]++;
		}
		else if (causeOfDeath == SimContext::DTH_OI_TB) {
			SimContext::TB_STRAIN tbStrain = patient->tbState.currTrueTBResistanceStrain;
			runStats->tbStats.numDeaths[tbStrain]++;
		}
		else if ((causeOfDeath >= SimContext::DTH_CHRM_1) && (causeOfDeath < SimContext::DTH_CHRM_1 + SimContext::CHRM_NUM)) {
			runStats->chrmsStats.numDeathsCHRMCD4[causeOfDeath - SimContext::DTH_CHRM_1][patient->diseaseState.currTrueCD4Strata]++;
		}
	}

	// update the longitudinal death statistics if requested, will be NULL if not
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		currTime->numDeathsType[patient->diseaseState.causeOfDeath]++;
		if (patient->monitoringState.currLTFUState == SimContext::LTFU_STATE_LOST) {
			if (patient->monitoringState.wasOnARTWhenLostToFollowUp) {
				currTime->numDeathsWhileLostART[patient->artState.prevRegimenNum]++;
			}
			else if (!patient->artState.hasTakenART) {
				currTime->numDeathsWhileLostPreART++;
			}
			else {
				currTime->numDeathsWhileLostPostART++;
			}
		}
	}
} /* end setCauseOfDeath */

/* setMaternalDeath updates the patient state for pediatric maternal death */
void StateUpdater::setMaternalDeath() {
	patient->generalState.isMotherAlive = false;
} /* end setMaternalDeath */

/** \brief incrementCohortSize increments the total population size by one
 *
 * This is done at patient creation instead of patient death (with the other population statistics)
 * for the sake of the transmission model: since the transmission model runs multiple patients at once,
 * waiting until the end to increment the cohort size results in all persons having a patientNum of 1
 */
void StateUpdater::incrementCohortSize(){
	runStats->popSummary.numCohorts++;
}

/* updatePopulationStats updates the final population summary after a death occurs */
void StateUpdater::updatePopulationStats() {
	// Increment the number of cohorts and HIV positive cohorts
	//The total population is now incremented at patient creation
	//runStats->popSummary.numCohorts++;
	if (patient->diseaseState.infectedHIVState != SimContext::HIV_INF_NEG) {
		runStats->popSummary.numCohortsHIVPositive++;
	}

	// Update sums for costs and life months of all patients
	runStats->popSummary.costsSum += patient->generalState.costsDiscounted;
	runStats->popSummary.costsSumSquares += patient->generalState.costsDiscounted * patient->generalState.costsDiscounted;
	runStats->popSummary.LMsSum += patient->generalState.LMsDiscounted;
	runStats->popSummary.LMsSumSquares += patient->generalState.LMsDiscounted * patient->generalState.LMsDiscounted;
	runStats->popSummary.QALMsSum += patient->generalState.qualityAdjustLMsDiscounted;
	runStats->popSummary.QALMsSumSquares += patient->generalState.qualityAdjustLMsDiscounted * patient->generalState.qualityAdjustLMsDiscounted;

	// Update sums for costs and life months of patients with X number of ART failures
	for (int i = 0; i <= SimContext::ART_NUM_LINES; i++) {
		if (i >= patient->artState.numObservedFailures) {
			runStats->popSummary.numFailART[i]++;
			runStats->popSummary.costsFailARTSum[i] += patient->generalState.costsDiscounted;
			runStats->popSummary.LMsFailARTSum[i] += patient->generalState.LMsDiscounted;
			runStats->popSummary.QALMsFailARTSum[i] += patient->generalState.qualityAdjustLMsDiscounted;
		}
	}

	// Update sums for costs and life months of HIV positive patients
	if (patient->diseaseState.infectedHIVState != SimContext::HIV_INF_NEG) {
		runStats->popSummary.costsHIVPositiveSum += patient->generalState.costsDiscounted;
		runStats->popSummary.LMsHIVPositiveSum += patient->generalState.LMsDiscounted;
		runStats->popSummary.QALMsHIVPositiveSum += patient->generalState.qualityAdjustLMsDiscounted;
		// If patient dies HIV positive and undetected, update the associated HIV screening stats
		if (!patient->monitoringState.isDetectedHIVPositive) {
			if (patient->diseaseState.isPrevalentHIVCase)
				runStats->hivScreening.numDetectedPrevalentMeans[SimContext::HIV_DET_UNDETECTED]++;
			else
				runStats->hivScreening.numDetectedIncidentMeans[SimContext::HIV_DET_UNDETECTED]++;
		}
	}
} /* end updatePopulationStats */

void StateUpdater::addPatientSummary() {
	// only include the HIV positive patients up to 1,000,000
	if (patient->diseaseState.infectedHIVState == SimContext::HIV_INF_NEG)
		return;
	if (patient->generalState.patientNum > 1000000)
		return;

	// add a new PatientSummary to the vector for this patient
	RunStats::PatientSummary patientSummary;
	patientSummary.costs = patient->generalState.costsDiscounted;
	patientSummary.LMs = patient->generalState.LMsDiscounted;
	patientSummary.QALMs = patient->generalState.qualityAdjustLMsDiscounted;
	runStats->patients.push_back(patientSummary);
} /* end addPatientSummary */

/* setTrueCD4 updates the patients actual CD4 level and confines it within the bounds,
	also checks if it is the patients minimum CD4 value */
void StateUpdater::setTrueCD4(double newCD4, bool isInitial) {
	// make sure CD4 does not exceed upper and lower bounds, and CD4 envelopes if set
	patient->diseaseState.currTrueCD4 = newCD4;
	if (patient->diseaseState.currTrueCD4 < 0.0) {
		patient->diseaseState.currTrueCD4 = 0.0;
	}
	else if ((simContext->getRunSpecsInputs()->maxPatientCD4 != SimContext::NOT_APPL) &&
		(patient->diseaseState.currTrueCD4 > simContext->getRunSpecsInputs()->maxPatientCD4)) {
			patient->diseaseState.currTrueCD4 = simContext->getRunSpecsInputs()->maxPatientCD4;
	}
	else {
		if (!isInitial && patient->artState.overallCD4Envelope.isActive &&
				(patient->diseaseState.currTrueCD4 > patient->artState.overallCD4Envelope.value)) {
			patient->diseaseState.currTrueCD4 = patient->artState.overallCD4Envelope.value;
		}
		if (!isInitial && patient->artState.indivCD4Envelope.isActive &&
				(patient->diseaseState.currTrueCD4 > patient->artState.indivCD4Envelope.value)) {
			patient->diseaseState.currTrueCD4 = patient->artState.indivCD4Envelope.value;
		}
	}

	// update the true CD4 strata
	patient->diseaseState.currTrueCD4Strata = getCD4Strata(patient->diseaseState.currTrueCD4);

	// update the minimum true CD4 if needed
	if (isInitial || (patient->diseaseState.currTrueCD4 < patient->diseaseState.minTrueCD4)) {
		patient->diseaseState.minTrueCD4 = patient->diseaseState.currTrueCD4;
		patient->diseaseState.minTrueCD4Strata = patient->diseaseState.currTrueCD4Strata;
	}
} /* end setTrueCD4 */

/* setTrueCD4Percentage updates the pediatrics patients actual CD4 percentage */
void StateUpdater::setTrueCD4Percentage(double newCD4Perc, bool isInitial) {
	// make sure CD4 percentage is within 0 and 1, and below the maximum CD4 value
	double maxCD4Perc = simContext->getPedsInputs()->maxCD4Percentage[patient->generalState.ageCategoryPediatrics];
	patient->diseaseState.currTrueCD4Percentage = newCD4Perc;
	if (patient->diseaseState.currTrueCD4Percentage < 0)
		patient->diseaseState.currTrueCD4Percentage = 0;
	else if (patient->diseaseState.currTrueCD4Percentage > maxCD4Perc)
		patient->diseaseState.currTrueCD4Percentage = maxCD4Perc;
	else if (patient->diseaseState.currTrueCD4Percentage > 1)
		patient->diseaseState.currTrueCD4Percentage = 1;
	else {
		if (!isInitial && patient->artState.overallCD4PercentageEnvelope.isActive &&
				(patient->diseaseState.currTrueCD4Percentage > patient->artState.overallCD4PercentageEnvelope.value)) {
			patient->diseaseState.currTrueCD4Percentage = patient->artState.overallCD4PercentageEnvelope.value;
		}
		if (!isInitial && patient->artState.indivCD4PercentageEnvelope.isActive &&
				(patient->diseaseState.currTrueCD4Percentage > patient->artState.indivCD4PercentageEnvelope.value)) {
			patient->diseaseState.currTrueCD4Percentage = patient->artState.indivCD4PercentageEnvelope.value;
		}
	}


	// update the true CD4 percentage strata
	patient->diseaseState.currTrueCD4PercentageStrata = getCD4PercentageStrata(patient->diseaseState.currTrueCD4Percentage);

	// update the minimum true CD4 percentage if needed
	if (isInitial || (patient->diseaseState.currTrueCD4Percentage < patient->diseaseState.minTrueCD4Percentage)) {
		patient->diseaseState.minTrueCD4Percentage = patient->diseaseState.currTrueCD4Percentage;
	}

	// updater the corresponding adult true CD4 strata for pediatrics patients,
	//	set minimum if this is the initial or below the previous minimum CD4 strata
	patient->diseaseState.currTrueCD4Strata = simContext->getPedsInputs()->adultCD4Strata[patient->generalState.ageCategoryPediatrics][patient->diseaseState.currTrueCD4PercentageStrata];
	if (isInitial || (patient->diseaseState.currTrueCD4Strata < patient->diseaseState.minTrueCD4Strata)) {
		patient->diseaseState.minTrueCD4Strata = patient->diseaseState.currTrueCD4Strata;
	}
} /* setTrueCD4Percentage */

/* setTrueHVLStrata set the patient's actual HVL strata to the given level */
void StateUpdater::setTrueHVLStrata(SimContext::HVL_STRATA newHVL) {
	// update the HVL strata
	patient->diseaseState.currTrueHVLStrata = newHVL;
} /* end setTrueHVLStrata */

/* setSetpointHVLStrata sets the patients setpoint HVL level */
void StateUpdater::setSetpointHVLStrata(SimContext::HVL_STRATA newSetpoint) {
	// update the HVL setpoint strata
	patient->diseaseState.setpointHVLStrata = newSetpoint;
} /* end setSetpointHVLStrata */

/* setObservedCD4 updates the patients observed CD4 level and confines it within the bounds */
void StateUpdater::setObservedCD4(bool isKnown, double cd4Value) {
	// If it is unknown (for untested patients), set the CD4 to unknown and return
	if (!isKnown) {
		patient->monitoringState.hasObservedCD4 = false;
		return;
	}

	// make sure observed CD4 is not below zero
	patient->monitoringState.monthOfObservedCD4 = patient->generalState.monthNum;
	patient->monitoringState.currObservedCD4 = cd4Value;
	if (patient->monitoringState.currObservedCD4 < 0.0) {
		patient->monitoringState.currObservedCD4 = 0.0;
	}

	// update the observed CD4 strata
	patient->monitoringState.currObservedCD4Strata = getCD4Strata(patient->monitoringState.currObservedCD4);

	// set the min and max observed CD4s if needed
	if (!patient->monitoringState.hasObservedCD4 ||
		(patient->monitoringState.currObservedCD4 < patient->monitoringState.minObservedCD4)) {
			patient->monitoringState.minObservedCD4 = patient->monitoringState.currObservedCD4;
	}
	if (patient->artState.isOnART) {
		if (!patient->monitoringState.hasObservedCD4 ||
			(patient->monitoringState.currObservedCD4 > patient->artState.maxObservedCD4OnCurrART)) {
				patient->artState.maxObservedCD4OnCurrART = patient->monitoringState.currObservedCD4;
		}
	}

	// set that the patient now has an observed CD4
	if (!patient->monitoringState.hasObservedCD4)
		patient->monitoringState.hasObservedCD4 = true;
} /* end setObservedCD4 */

/* setObservedCD4Percentage updates the patients observed CD4 percentage and confines it within the bounds */
void StateUpdater::setObservedCD4Percentage(bool isKnown, double cd4Percent) {
	// If it is unknown (for untested patients), set the CD4 to unknown and return
	if (!isKnown) {
		patient->monitoringState.hasObservedCD4Percentage = false;
		return;
	}

	// Make sure the observed value is between 0 and 1
	patient->monitoringState.monthOfObservedCD4Percentage = patient->generalState.monthNum;
	patient->monitoringState.currObservedCD4Percentage = cd4Percent;
	if (patient->monitoringState.currObservedCD4Percentage < 0)
		patient->monitoringState.currObservedCD4Percentage = 0;
	else if (patient->monitoringState.currObservedCD4Percentage > 1)
		patient->monitoringState.currObservedCD4Percentage = 1;

	// update the observed CD4 percentage strata
	patient->monitoringState.currObservedCD4PercentageStrata = getCD4PercentageStrata(patient->monitoringState.currObservedCD4);

	// set the min and max observed CD4s if needed
	if (!patient->monitoringState.hasObservedCD4Percentage ||
		(patient->monitoringState.currObservedCD4Percentage < patient->monitoringState.minObservedCD4Percentage)) {
			patient->monitoringState.minObservedCD4Percentage = patient->monitoringState.currObservedCD4Percentage;
	}
	if (patient->artState.isOnART) {
		if (!patient->monitoringState.hasObservedCD4Percentage ||
			(patient->monitoringState.currObservedCD4Percentage > patient->artState.maxObservedCD4PercentageOnCurrART)) {
				patient->artState.maxObservedCD4PercentageOnCurrART = patient->monitoringState.currObservedCD4Percentage;
		}
	}

	// set that the patient now has an observed CD4 percentage
	if (!patient->monitoringState.hasObservedCD4Percentage)
		patient->monitoringState.hasObservedCD4Percentage = true;
} /* end setObservedCD4Percentage */

/* setObservedHVLStrata updates the patients observed HVL strata */
void StateUpdater::setObservedHVLStrata(bool isKnown, SimContext::HVL_STRATA hvlStrata) {
	// If value is unobserved (for untest patients) set the state and return
	if (!isKnown) {
		patient->monitoringState.hasObservedHVLStrata = false;
		return;
	}

	// Update the observed HVL strata
	patient->monitoringState.monthOfObservedHVLStrata = patient->generalState.monthNum;
	patient->monitoringState.currObservedHVLStrata = hvlStrata;

	// set the maximum observed HVL, used as a proxy for the HVL setpoint
	if (!patient->monitoringState.hasObservedHVLStrata ||
		(patient->monitoringState.currObservedHVLStrata > patient->monitoringState.maxObservedHVLStrata)) {
			patient->monitoringState.maxObservedHVLStrata = patient->monitoringState.currObservedHVLStrata;
	}

	// set the initial or minimum observed HVL on ART
	if (patient->getARTState()->isOnART) {
		if (patient->artState.monthOfCurrRegimenStart == patient->generalState.monthNum)
			patient->artState.observedHVLStrataAtRegimenStart = patient->monitoringState.currObservedHVLStrata;
		if (!patient->monitoringState.hasObservedHVLStrata ||
			(patient->monitoringState.currObservedHVLStrata < patient->artState.minObservedHVLStrataOnCurrART)) {
				patient->artState.minObservedHVLStrataOnCurrART = patient->monitoringState.currObservedHVLStrata;
		}
	}

	// set that the patient now has an observed HVL
	if (!patient->monitoringState.hasObservedHVLStrata) {
		patient->monitoringState.hasObservedHVLStrata = true;
	}
} /* end setObservedHVLStrata */

/* incrementCostsHIVTest adds an HIV testing cost to the patients total */
void StateUpdater::incrementCostsHIVTest(double cost) {
	runStats->overallCosts.costsHIVScreeningTests += cost * patient->generalState.discountFactor;
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		currTime->costsHIVTests += cost * patient->generalState.discountFactor;
	}
	incrementCostsCommon(cost, 1.0);
} /* end incrementCostsHIVTest */

/* incrementCostsHIVMisc adds an HIV misc related cost to the patients total */
void StateUpdater::incrementCostsHIVMisc(double cost) {
	runStats->overallCosts.costsHIVScreeningMisc += cost * patient->generalState.discountFactor;
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		currTime->costsHIVMisc += cost * patient->generalState.discountFactor;
	}
	incrementCostsCommon(cost, 1.0);
} /* end incrementCostsHIVTest */

/* incrementCostsCD4Test adds the CD4 testing related costs to the patients total */
void StateUpdater::incrementCostsCD4Test(const double *costArray) {
	double discountedCostTotal = 0.0;
	for (int i = 0; i < SimContext::COST_NUM_TYPES; i++) {
		discountedCostTotal += costArray[i];
	}
	discountedCostTotal *= patient->generalState.discountFactor;
	runStats->overallCosts.costsCD4Testing += discountedCostTotal;
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		currTime->costsCD4Testing += discountedCostTotal;
	}
	incrementCostsCommon(costArray, 1.0);
} /* end incrementCostsCD4Test */

/* incrementCostsHVLTest adds the HVL testing related costs to the patients total */
void StateUpdater::incrementCostsHVLTest(const double *costArray) {
	double discountedCostTotal = 0.0;
	for (int i = 0; i < SimContext::COST_NUM_TYPES; i++) {
		discountedCostTotal += costArray[i];
	}
	discountedCostTotal *= patient->generalState.discountFactor;
	runStats->overallCosts.costsHVLTesting += discountedCostTotal;
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		currTime->costsHVLTesting += discountedCostTotal;
	}
	incrementCostsCommon(costArray, 1.0);
} /* end incrementCostsHVLTest */

/* incrementCostsClinicVisit adds the general clinic visit costs to the patients total */
void StateUpdater::incrementCostsClinicVisit(const double *costArray) {
	double discountedCostTotal = 0.0;
	for (int i = 0; i < SimContext::COST_NUM_TYPES; i++) {
		discountedCostTotal += costArray[i];
	}
	discountedCostTotal *= patient->generalState.discountFactor;
	runStats->overallCosts.costsClinicVisits += discountedCostTotal;
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		currTime->costsClinicVisits += discountedCostTotal;
	}
	incrementCostsCommon(costArray, 1.0);
} /* end incrementCostsClinicVisit */

/* incrementCostsART adds an ART treatment cost to the patients total */
void StateUpdater::incrementCostsART(int artLineNum, double cost) {
	double discountedCost = cost * patient->getGeneralState()->discountFactor;
	runStats->overallCosts.costsDrugs += cost;
	runStats->overallCosts.directCostsARTLine[artLineNum] += discountedCost;
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		currTime->costsART[artLineNum] += discountedCost;
	}
	incrementCostsCommon(cost, 1.0);
} /* end incrementCostsART */

/* incrementCostsProph adds a prophylaxis treatment cost to the patients total */
void StateUpdater::incrementCostsProph(SimContext::OI_TYPE oiType, int prophNum, double cost) {
	double discountedCost = cost * patient->getGeneralState()->discountFactor;
	runStats->overallCosts.costsDrugs += cost;
	runStats->overallCosts.directCostsProphOIsProph[oiType][prophNum] += discountedCost;
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		currTime->costsProph[oiType][prophNum] += discountedCost;
	}
	incrementCostsCommon(cost, 1.0);
} /* incrementCostsProph */

/* incrementCostsTBProph adds a TB proph cost to patients total */
void StateUpdater::incrementCostsTBProph(int prophNum, double cost) {
	runStats->overallCosts.costsDrugs += cost * patient->getGeneralState()->discountFactor;
	incrementCostsCommon(cost, 1.0);
} /* end incrementCostsTBProph */

/* incrementCostsTBTreatment adds a TB proph cost to patients total */
void StateUpdater::incrementCostsTBTreatment(const double *costArray, double percent) {
	double discountedCostTotal = 0.0;
	for (int i = 0; i < SimContext::COST_NUM_TYPES; i++) {
		discountedCostTotal += costArray[i];
	}
	discountedCostTotal *= patient->generalState.discountFactor * percent;
	runStats->overallCosts.costsDrugs += discountedCostTotal;
	incrementCostsCommon(costArray, percent);
} /* end incrementCostsTBTreatment */

/* incrementCostsToxicity adds a toxicity cost to the patients total */
void StateUpdater::incrementCostsToxicity(double cost) {
	runStats->overallCosts.costsToxicity += cost;
	incrementCostsCommon(cost, 1.0);
} /* end incrementCostsToxicity */

/* incrementCostsMisc adds a miscellaneous cost to the patients total costs,
	overloaded to take in either a single cost value or a COST_NUM_TYPES sized array of costs */
void StateUpdater::incrementCostsMisc(double cost, double percent) {
	incrementCostsCommon(cost, percent);
} /* end incrementCostsMisc */

void StateUpdater::incrementCostsMisc(const double *costArray, double percent) {
	incrementCostsCommon(costArray, percent);
} /* end incrementCostsMisc */

/* updateInitialDistributions updates the initial statistics for patients upon infection */
void StateUpdater::updateInitialDistributions() {
	runStats->initialDistributions.numPatientsCD4Level[patient->diseaseState.currTrueCD4Strata]++;
	runStats->initialDistributions.numPatientsHVLLevel[patient->diseaseState.currTrueHVLStrata]++;
	runStats->initialDistributions.numPatientsHVLSetpointLevel[patient->diseaseState.setpointHVLStrata]++;
	runStats->initialDistributions.sumInitialAgeMonths += patient->generalState.ageMonths;
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		if (patient->diseaseState.hasTrueOIHistory[i]) {
			runStats->initialDistributions.numPriorOIHistories[i]++;
		}
	}
	if (patient->generalState.gender == SimContext::GENDER_MALE)
		runStats->initialDistributions.numMalePatients++;
	else
		runStats->initialDistributions.numFemalePatients++;
	runStats->initialDistributions.numARTResposneTypes[patient->artState.CD4ResponseType]++;
} /* end updateInitialDistributions */

/* updatePatientSurvival updates the patient state for discounted LMs and QALMs,
	used with a half month length if death occurred that month */
void StateUpdater::updatePatientSurvival(double percentOfMonth) {
	patient->generalState.LMsDiscounted += percentOfMonth * patient->generalState.discountFactor;
	patient->generalState.qualityAdjustLMsDiscounted +=
		percentOfMonth * patient->generalState.discountFactor * patient->generalState.QOLMultiplier;
} /* end updatePatientSurvival */

/* updateOverallSurvival updates the statistics for stratified LMs and QALMs,
	used with a half month length if death occurred that month */
void StateUpdater::updateOverallSurvival(double percentOfMonth) {
	SimContext::HIV_ID detectedState = patient->monitoringState.isDetectedHIVPositive ? SimContext::HIV_ID_IDEN : SimContext::HIV_ID_UNID;
	if (patient->diseaseState.infectedHIVState == SimContext::HIV_INF_NEG)
		detectedState = SimContext::HIV_ID_NEG;

	// Update survival statistics by detected HIV state first
	runStats->overallSurvival.LMsHIVState[detectedState] +=
		percentOfMonth * patient->generalState.discountFactor;
	runStats->overallSurvival.QALMsHIVState[detectedState] +=
		percentOfMonth * patient->generalState.discountFactor * patient->generalState.QOLMultiplier;

	// If HIV negative, only update a few additional survival statistics and return
	if (patient->diseaseState.infectedHIVState == SimContext::HIV_INF_NEG) {
		runStats->overallSurvival.LMsInScreening +=
			percentOfMonth * patient->generalState.discountFactor;
		runStats->overallSurvival.QALMsInScreening +=
			percentOfMonth * patient->generalState.discountFactor * patient->generalState.QOLMultiplier;
		return;
	}

	// Update all the overall survival statistics for HIV positive patients
	if (patient->diseaseState.typeTrueOIHistory == SimContext::HIST_EXT_N) {
		runStats->overallSurvival.LMsNoOIHistoryCD4[patient->diseaseState.currTrueCD4Strata] +=
			percentOfMonth * patient->generalState.discountFactor;
	}
	else {
		runStats->overallSurvival.LMsOIHistoryCD4[patient->diseaseState.currTrueCD4Strata] +=
			percentOfMonth * patient->generalState.discountFactor;
	}
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		if (patient->diseaseState.hasTrueOIHistory[i]) {
			runStats->overallSurvival.LMsOIHistoryOIs[i] +=
				percentOfMonth * patient->generalState.discountFactor;
		}
		else {
			runStats->overallSurvival.LMsNoOIHistoryOIs[i] +=
				percentOfMonth * patient->generalState.discountFactor;
		}
	}
	runStats->overallSurvival.LMsHVL[patient->diseaseState.currTrueHVLStrata] +=
		percentOfMonth * patient->generalState.discountFactor;
	runStats->overallSurvival.LMsHVLSetpoint[patient->diseaseState.setpointHVLStrata] +=
		percentOfMonth * patient->generalState.discountFactor;
	runStats->overallSurvival.LMsInRegularCEPAC +=
		percentOfMonth * patient->generalState.discountFactor;
	runStats->overallSurvival.LMsGender[patient->generalState.gender] +=
		percentOfMonth * patient->generalState.discountFactor;
	runStats->overallSurvival.QALMsGender[patient->generalState.gender] +=
		percentOfMonth * patient->generalState.discountFactor * patient->generalState.QOLMultiplier;
} /* end updateOverallSurvival */

/* updateLongitSurvival updates the longitudinal statistics related to survival */
void StateUpdater::updateLongitSurvival() {
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		// return if using yearly longitudinal logging and current month is not the end of the year
		if ((simContext->getRunSpecsInputs()->longitLoggingLevel == SimContext::LONGIT_SUMM_YR_DET) &&
			(patient->generalState.monthNum %12 != 11))
				return;

		SimContext::HIV_ID detectedState = patient->monitoringState.isDetectedHIVPositive ? SimContext::HIV_ID_IDEN : SimContext::HIV_ID_UNID;
		if (patient->diseaseState.infectedHIVState == SimContext::HIV_INF_NEG)
			detectedState = SimContext::HIV_ID_NEG;

		currTime->numAliveType[detectedState]++;
		if (patient->generalState.ageCategoryPediatrics < SimContext::PEDS_AGE_LATE)
			currTime->numAlivePediatrics[patient->diseaseState.infectedPediatricsHIVState]++;

		//Increase QOL multiplier sum
		currTime->sumQOLmultipliers += patient->generalState.QOLMultiplier;

		if (patient->diseaseState.infectedHIVState == SimContext::HIV_INF_NEG) {
			currTime->numWithoutOIHistory++;
			return;
		}
		if (patient->diseaseState.typeTrueOIHistory == SimContext::HIST_EXT_N) {
			currTime->numWithoutOIHistory++;
		}
		else {
			for (int i = 0; i < SimContext::OI_NUM; i++) {
				if (patient->diseaseState.hasTrueOIHistory[i]) {
					currTime->numWithOIHistory[i]++;
				}
			}
		}

		currTime->trueCD4Sum += patient->diseaseState.currTrueCD4;
		currTime->trueCD4SumSquares += patient->diseaseState.currTrueCD4 * patient->diseaseState.currTrueCD4;
		if (patient->generalState.ageCategoryPediatrics < SimContext::PEDS_AGE_LATE) {
			currTime->trueCD4PercentageSum += patient->diseaseState.currTrueCD4Percentage;
			currTime->trueCD4PercentageSumSquares += patient->diseaseState.currTrueCD4Percentage * patient->diseaseState.currTrueCD4Percentage;
		}
		double hvlValue = SimContext::HVL_STRATA_MIDPTS[patient->diseaseState.currTrueHVLStrata];
		currTime->trueHVLSum += hvlValue;
		currTime->trueHVLSumSquares += hvlValue * hvlValue;
		if (patient->monitoringState.hasObservedCD4) {
			currTime->observedCD4Sum += patient->monitoringState.currObservedCD4;
			currTime->observedCD4SumSquares += patient->monitoringState.currObservedCD4 * patient->monitoringState.currObservedCD4;
			currTime->observedCD4Distribution[patient->monitoringState.currObservedCD4Strata]++;
		}
		if (patient->monitoringState.hasObservedHVLStrata) {
			hvlValue = SimContext::HVL_STRATA_MIDPTS[patient->monitoringState.currObservedHVLStrata];
			currTime->observedHVLSum += hvlValue;
			currTime->observedHVLSumSquares += hvlValue * hvlValue;
			currTime->observedHVLDistribution[patient->monitoringState.currObservedHVLStrata]++;
		}
		if (patient->artState.isOnART) {
			currTime->trueCD4HVLARTDistribution[SimContext::ART_ON_STATE][patient->diseaseState.currTrueCD4Strata][patient->diseaseState.currTrueHVLStrata]++;
			currTime->numOnART[patient->artState.currRegimenNum]++;
			currTime->numOnARTIncludingContinuedCosts[patient->artState.currRegimenNum]++;
			currTime->numARTEfficacyState[patient->artState.currRegimenEfficacy]++;
		}
		else {
			currTime->trueCD4HVLARTDistribution[SimContext::ART_OFF_STATE][patient->diseaseState.currTrueCD4Strata][patient->diseaseState.currTrueHVLStrata]++;
		}
	}
} /* end updateLongitSurvival */

/* updateOIHistoryLogging updates the statistics for patient OI history logging */
void StateUpdater::updateOIHistoryLogging() {
	double cd4Level = patient->diseaseState.currTrueCD4;
	SimContext::CD4_STRATA cd4Strata = patient->diseaseState.currTrueCD4Strata;
	SimContext::HVL_STRATA hvlStrata = patient->diseaseState.currTrueHVLStrata;
	int numARTFailures = patient->artState.numObservedFailures;

	// Return if OI history logging is not enabled or logging criteria is not met
	if (!simContext->getRunSpecsInputs()->enableOIHistoryLogging)
		return;
	if ((cd4Level < simContext->getRunSpecsInputs()->CD4BoundsForOIHistoryLogging[SimContext::LOWER_BOUND]) ||
		(cd4Level > simContext->getRunSpecsInputs()->CD4BoundsForOIHistoryLogging[SimContext::UPPER_BOUND]))
		return;
	if ((hvlStrata < simContext->getRunSpecsInputs()->HVLBoundsForOIHistoryLogging[SimContext::LOWER_BOUND]) ||
		(hvlStrata > simContext->getRunSpecsInputs()->HVLBoundsForOIHistoryLogging[SimContext::UPPER_BOUND]))
		return;
	if (simContext->getRunSpecsInputs()->numARTFailuresForOIHistoryLogging != SimContext::NOT_APPL) {
		if (patient->getARTState()->isOnART)
			return;
		if (numARTFailures != simContext->getRunSpecsInputs()->numARTFailuresForOIHistoryLogging)
			return;
	}
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		if (patient->diseaseState.hasTrueOIHistory[i] && simContext->getRunSpecsInputs()->OIsToExcludeOIHistoryLogging[i])
			return;
	}

	// Increment the stratified total months and months with OI history
	runStats->oiStats.numMonthsHVLCD4[hvlStrata][cd4Strata]++;
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		if (patient->diseaseState.hasTrueOIHistory[i]) {
			runStats->oiStats.numMonthsOIHistoryHVLCD4[i][hvlStrata][cd4Strata]++;
		}
	}

	// Increment the stratified number of patients and patients with OI history,
	//	only called once for each patient
	if (!patient->generalState.loggedPatientOIs) {
		runStats->oiStats.numPatientsHVLCD4[hvlStrata][cd4Strata]++;
		for (int i = 0; i < SimContext::OI_NUM; i++) {
			if (patient->diseaseState.hasTrueOIHistory[i]) {
				runStats->oiStats.numPatientsOIHistoryHVLCD4[i][hvlStrata][cd4Strata]++;
			}
		}
		patient->generalState.loggedPatientOIs = true;
	}
} /* end updateOIHistoryLogging */

/* updateARTEfficacyStats updates the totals for months in ART suppression and HVL drops */
void StateUpdater::updateARTEfficacyStats() {
	if (!patient->getARTState()->isOnART)
		return;

	// Update the months in suppression states
	int artLineNum = patient->artState.currRegimenNum;
	SimContext::ART_EFF_TYPE efficacy = patient->getARTState()->currRegimenEfficacy;
	SimContext::HVL_STRATA hvlStrata = patient->getDiseaseState()->currTrueHVLStrata;
	switch (efficacy) {
		case SimContext::ART_EFF_SUCCESS:
			runStats->artStats.monthsSuppressedLine[artLineNum]++;
			break;
		case SimContext::ART_EFF_PARTIAL:
			runStats->artStats.monthsPartiallySuppressedLineHVL[artLineNum][hvlStrata]++;
			break;
		case SimContext::ART_EFF_FAILURE:
			runStats->artStats.monthsFailedLineHVL[artLineNum][hvlStrata]++;
			break;
	}

	// Update the number suppressed at HVL drops if month is specified for efficacy recording
	for (int i = 0; i < SimContext::ART_NUM_MTHS_RECORD; i++) {
		int monthsOnART = patient->generalState.monthNum - patient->artState.monthOfCurrRegimenStart;
		if (monthsOnART == simContext->getRunSpecsInputs()->monthRecordARTEfficacy[i]) {
			runStats->artStats.numOnARTAtMonth[artLineNum][i]++;
			if (patient->diseaseState.currTrueHVLStrata <= SimContext::HVL_SUPPRESSION)
				runStats->artStats.numSuppressedAtMonth[artLineNum][i]++;
			int hvlDrop = patient->diseaseState.setpointHVLStrata - patient->diseaseState.currTrueHVLStrata;
			runStats->artStats.HVLDropsAtMonthSum[artLineNum][i] += hvlDrop;
			runStats->artStats.HVLDropsAtMonthSumSquares[artLineNum][i] += hvlDrop * hvlDrop;
			break;
		}
	}
} /* end updateARTEfficacyStats */

/* willAttendClinicThisMonth returns true if patient will go in for a scheduled clinic
	visit or OI emergency clinic visit this month */
bool StateUpdater::willAttendClinicThisMonth() {
	// No clinic visits can occur while the patient is lost or undetected as HIV positive
	if (patient->getMonitoringState()->currLTFUState == SimContext::LTFU_STATE_LOST)
		return false;
	if (!patient->getMonitoringState()->isDetectedHIVPositive)
		return false;

	// Test if a regularly scheduled clinic visit will occur this month, also depends on the
	//	patient's type for when they attend scheduled visits
	if (patient->getMonitoringState()->hasRegularClinicVisit &&
		(patient->getGeneralState()->monthNum >= patient->getMonitoringState()->monthOfRegularClinicVisit)) {
			if (!patient->getMonitoringState()->hadPrevClinicVisit)
				return true;
			if (patient->getARTState()->isOnART)
				return true;
			if (patient->getProphState()->currTotalNumProphsOn > 0)
				return true;
			if (patient->getMonitoringState()->clinicVisitType == SimContext::CLINIC_SCHED)
				return true;
	}

	// Determine if an emergency clinic visit has been triggered for this month
	if (patient->getMonitoringState()->hasEmergencyClinicVisit &&
		(patient->getGeneralState()->monthNum >= patient->getMonitoringState()->monthOfEmergencyClinicVisit)) {
			return true;
	}

	return false;
} /* end willAttendClinicThisMonth */

/* getPartialSuppressTargetHVL determines the ART target HVL when entering partial suppression */
SimContext::HVL_STRATA StateUpdater::getPartialSuppressTargetHVL(int artLineNum) {
	// Determine the new target HVL level
	// ISSUE: Should this be able to go lower if setpoint is LO or MLO?
	const SimContext::ARTInputs *artInput = simContext->getARTInputs(artLineNum);
	int numStrata = patient->getDiseaseState()->setpointHVLStrata - SimContext::HVL_SUPPRESSION - 1;
	if (numStrata < 1) {
		return patient->getDiseaseState()->setpointHVLStrata;
	}
	if (numStrata > 3)
		numStrata = 3;
	double sumProb = 0.0;
	for (int i = 0; i < numStrata; i++) {
		sumProb += artInput->partialSuppressionDistribution[i];
	}
	int hvlTarget = patient->getDiseaseState()->setpointHVLStrata - 1;
	double randNum = CepacUtil::getRandomDouble(130010, patient);
	for (int i = 0; i < numStrata; i++) {
		if ((artInput->partialSuppressionDistribution[i] > 0) && (randNum < artInput->partialSuppressionDistribution[i] / sumProb)) {
			hvlTarget = patient->getDiseaseState()->setpointHVLStrata - (i + 1);
			break;
		}
		randNum -= artInput->partialSuppressionDistribution[i];
	}
	return ((SimContext::HVL_STRATA) hvlTarget);
} /* end getPartialSuppressTargetHVL */

/* getCD4Strata returns the CD4 strata for a given value  */
SimContext::CD4_STRATA StateUpdater::getCD4Strata(double valueCD4) {
	for (int i = 0; i < SimContext::CD4_NUM_STRATA - 1; i++) {
		if (valueCD4 < simContext->getRunSpecsInputs()->CD4StrataUpperBounds[i]) {
			return ((SimContext::CD4_STRATA) i);
		}
	}
	return SimContext::CD4_VHI;
}

/* getCD4PercentageStrata returns the CD4 percentage strata for a given value */
SimContext::PEDS_CD4_PERC StateUpdater::getCD4PercentageStrata(double percCD4) {
	if (percCD4 < 0.05)
		return SimContext::PEDS_CD4_PERC_L5;
	if (percCD4 < 0.1)
		return SimContext::PEDS_CD4_PERC_L10;
	if (percCD4 < 0.15)
		return SimContext::PEDS_CD4_PERC_L15;
	if (percCD4 < 0.2)
		return SimContext::PEDS_CD4_PERC_L20;
	if (percCD4 < 0.25)
		return SimContext::PEDS_CD4_PERC_L25;
	if (percCD4 < 0.3)
		return SimContext::PEDS_CD4_PERC_L30;
	if (percCD4 < 0.35)
		return SimContext::PEDS_CD4_PERC_L35;
	return SimContext::PEDS_CD4_PERC_HIGHER;
}

/* getAgeCategoryClinical returns the clinic visit age category for the given age */
int StateUpdater::getAgeCategoryClinical(int ageMonths) {
	int ageYears = ageMonths / 12;
	int ageCategory = ageYears / 5;
	if (ageCategory < SimContext::AGE_CATEGORIES)
		return ageCategory;
	return SimContext::AGE_CATEGORIES - 1;
}

/* getAgeCategoryHIVInfection returns the HIV testing age category for the given age */
int StateUpdater::getAgeCategoryHIVInfection(int ageMonths) {
	int ageYears = ageMonths / 12;
	if (ageYears < 18)
		return 0;
	if (ageYears <= 25)
		return 1;
	if (ageYears <= 30)
		return 2;
	if (ageYears <= 35)
		return 3;
	if (ageYears <= 40)
		return 4;
	if (ageYears <= 45)
		return 5;
	return 6;
}

/* getAgeCategoryPediatrics returns the Pediatrics testing category for the given age */
SimContext::PEDS_AGE_CAT StateUpdater::getAgeCategoryPediatrics(int ageMonths) {
	if (!simContext->getPedsInputs()->enablePediatricsModel)
		return SimContext::PEDS_AGE_ADULT;

	if (ageMonths < 3)
		return SimContext::PEDS_AGE_2MTH;
	if (ageMonths < 6)
		return SimContext::PEDS_AGE_5MTH;
	if (ageMonths < 9)
		return SimContext::PEDS_AGE_8MTH;
	if (ageMonths < 12)
		return SimContext::PEDS_AGE_11MTH;
	if (ageMonths < 15)
		return SimContext::PEDS_AGE_14MTH;
	if (ageMonths < 18)
		return SimContext::PEDS_AGE_17MTH;
	if (ageMonths < 24)
		return SimContext::PEDS_AGE_23MTH;
	if (ageMonths < 36)
		return SimContext::PEDS_AGE_2YR;
	if (ageMonths < 48)
		return SimContext::PEDS_AGE_3YR;
	if (ageMonths < 60)
		return SimContext::PEDS_AGE_4YR;
	if (ageMonths < 156)
		return SimContext::PEDS_AGE_LATE;
	return SimContext::PEDS_AGE_ADULT;
}

/* getTimeSummary returns a non-const pointer to the TimeSummary object for the current time period,
	creates a new one if needed or returns null if not keeping longitudinal stats */
RunStats::TimeSummary *StateUpdater::getTimeSummaryForUpdate() {
	int timePeriod;
	SimContext::LONGIT_SUMM_TYPE longitLevel = simContext->getRunSpecsInputs()->longitLoggingLevel;

	if (longitLevel != SimContext::LONGIT_SUMM_NONE) {
		if (longitLevel == SimContext::LONGIT_SUMM_YR_DET)
			timePeriod = patient->generalState.monthNum / 12;
		else
			timePeriod = patient->generalState.monthNum;

		if (timePeriod < (int) runStats->timeSummaries.size())
			return runStats->timeSummaries[timePeriod];

		// Add new time summaries until reach desired time period
		for (int i = (int) runStats->timeSummaries.size(); i <= timePeriod; i++) {
			RunStats::TimeSummary *currTime = new RunStats::TimeSummary();
			runStats->initTimeSummary(currTime);
			currTime->timePeriod = i;
			runStats->timeSummaries.push_back(currTime);
		}
		return runStats->timeSummaries[timePeriod];
	}

	return NULL;
} /* end getTimeSummaryForUpdate */

/* incrementCostsCommon increases all general cost stats that are independent of the type of cost */
void StateUpdater::incrementCostsCommon(double cost, double percent) {
	double discountFactor = patient->generalState.discountFactor;
	double discountedCost = cost * discountFactor * percent;

	// Update the patient state costs
	patient->generalState.costsDiscounted += discountedCost;

	// Update the run stats overall costs
	if (patient->getDiseaseState()->infectedHIVState != SimContext::HIV_INF_NEG) {
		SimContext::CD4_STRATA cd4Strata = patient->diseaseState.currTrueCD4Strata;
		SimContext::HVL_STRATA hvlStrata = patient->diseaseState.currTrueHVLStrata;
		SimContext::HVL_STRATA setpointHVL = patient->diseaseState.setpointHVLStrata;
		runStats->overallCosts.costsTotalCD4[cd4Strata] += discountedCost;
		if (patient->diseaseState.typeTrueOIHistory == SimContext::HIST_EXT_N)
			runStats->overallCosts.costsNoOIHistoryCD4[cd4Strata] += discountedCost;
		else
			runStats->overallCosts.costsOIHistoryCD4[cd4Strata] += discountedCost;
		runStats->overallCosts.costsHVL[hvlStrata] += discountedCost;
		runStats->overallCosts.costsHVLSetpoint[setpointHVL] += discountedCost;
	}
	SimContext::HIV_ID detectedState = patient->monitoringState.isDetectedHIVPositive ? SimContext::HIV_ID_IDEN : SimContext::HIV_ID_UNID;
	if (patient->diseaseState.infectedHIVState == SimContext::HIV_INF_NEG)
		detectedState = SimContext::HIV_ID_NEG;
	runStats->overallCosts.costsHIVState[detectedState] += discountedCost;
	runStats->overallCosts.costsGender[patient->generalState.gender] += discountedCost;

	// Update the longitudinal cost statistics
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		currTime->totalMonthlyCohortCosts += discountedCost;
	}
} /* end incrementCostsCommon */

void StateUpdater::incrementCostsCommon(const double *costArray, double percent) {
	double discountFactor = patient->generalState.discountFactor;
	double discountedCostArray[SimContext::COST_NUM_TYPES];
	double discountedCostTotal = 0;

	// Calculated the discounted cost of each type and the total costs
	for (int i = 0; i < SimContext::COST_NUM_TYPES; i++) {
		discountedCostArray[i] = costArray[i] * discountFactor * percent;
		discountedCostTotal += discountedCostArray[i];
	}

	// Update the patient state costs
	patient->generalState.costsDiscounted += discountedCostTotal;

	// Update the sum of costs by each cost type
	for (int i = 0; i < SimContext::COST_NUM_TYPES; i++) {
		runStats->overallCosts.totalUndiscountedCosts[i] += costArray[i] * percent;
	}

	// Update the run stats overall costs
	if (patient->getDiseaseState()->infectedHIVState != SimContext::HIV_INF_NEG) {
		SimContext::CD4_STRATA cd4Strata = patient->diseaseState.currTrueCD4Strata;
		SimContext::HVL_STRATA hvlStrata = patient->diseaseState.currTrueHVLStrata;
		SimContext::HVL_STRATA setpointHVL = patient->diseaseState.setpointHVLStrata;
		runStats->overallCosts.costsTotalCD4[cd4Strata] += discountedCostTotal;
		if (patient->diseaseState.typeTrueOIHistory == SimContext::HIST_EXT_N)
			runStats->overallCosts.costsNoOIHistoryCD4[cd4Strata] += discountedCostTotal;
		else
			runStats->overallCosts.costsOIHistoryCD4[cd4Strata] += discountedCostTotal;
		runStats->overallCosts.costsHVL[hvlStrata] += discountedCostTotal;
		runStats->overallCosts.costsHVLSetpoint[setpointHVL] += discountedCostTotal;
	}
	SimContext::HIV_ID detectedState = patient->monitoringState.isDetectedHIVPositive ? SimContext::HIV_ID_IDEN : SimContext::HIV_ID_UNID;
	if (patient->diseaseState.infectedHIVState == SimContext::HIV_INF_NEG)
		detectedState = SimContext::HIV_ID_NEG;
	runStats->overallCosts.costsHIVState[detectedState] += discountedCostTotal;
	runStats->overallCosts.costsGender[patient->generalState.gender] += discountedCostTotal;

	// Update the longitudinal cost statistics
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		currTime->totalMonthlyCohortCosts += discountedCostTotal;
	}
} /* end incrementCostsCommon */
