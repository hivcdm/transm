#include "include.h"

/** Constructor takes in the patient object */
StateUpdater::StateUpdater(Patient *patient) :
		patient(patient)
{

}

/** Destructor is empty, no cleanup required */
StateUpdater::~StateUpdater(void) {

}

/**
 * \brief Virtual function to perform the initial updates upon patient creation.
 * Sets the simContext, runStats, and tracer to match that of this->patient */
void StateUpdater::performInitialUpdates() {
	// Copy the pointers to the simContext, runStats, and tracer objects
	this->simContext = this->patient->simContext;
	this->runStats = this->patient->runStats;
	this->tracer = this->patient->tracer;
}

/**
 * \brief Virtual function to perform all updates for a simulated month.
 * Empty for now, no actions to perform if child does not override
 * */
void StateUpdater::performMonthlyUpdates() {
	// Empty for now, no actions to perform if child does not override
}

/** Virtual function changes the inputs the updater uses to determine disease progression -- to be used primarily by the transmission model */
void StateUpdater::setSimContext(SimContext *newSimContext){
	this->simContext = newSimContext;
}

/** \brief initializePatient initializes the patient's basic state
 * \param patientNum an integer argument that assigns the patient a (hopefully unique!) identifier
 * \param tracingEnabled a bool argument that is true if this patient is to be output in the trace file
 *
 * This function initializes patient->monthNum to the initial month number (0 unless otherwise specified by the transmission model),
 * all costs to 0, all life months (LMs) and discounted LMs to 0, the discount factor to 1, and marks the patient as alive */
void StateUpdater::initializePatient(int patientNum, bool tracingEnabled) {
	patient->generalState.patientNum = patientNum;
	patient->generalState.tracingEnabled = tracingEnabled;
	patient->generalState.monthNum = patient->generalState.initialMonthNum;
	patient->generalState.costsDiscounted = 0;
	patient->generalState.LMsDiscounted = 0;
	patient->generalState.qualityAdjustLMsDiscounted = 0;
	patient->generalState.discountFactor = 1.0;
	patient->generalState.loggedPatientOIs = false;
	patient->diseaseState.isAlive = true;
	patient->diseaseState.isExposed = false;
}

/** \brief setPatientAgeGender set the patients age and gender
 * \param gender a SimContext::GENDER_TYPE (male or female)
 * \param ageMonths an integer specifying the patient's initial age
 *
 * The patient's age categories are also set using helper functions
 * \see StateUpdater::getAgeCategoryClinical(int)
 * \see StateUpdater::getAgeCategoryHIVInfection(int)
 * \see StateUpdater::getAgeCategoryCHRMs(int)
 * \see StateUpdater::getAgeCategoryPediatrics(int) */
void StateUpdater::setPatientAgeGender(SimContext::GENDER_TYPE gender, int ageMonths) {
	patient->generalState.gender = gender;
	patient->generalState.ageMonths = ageMonths;
	patient->generalState.ageCategoryClinical = getAgeCategoryClinical(ageMonths);
	patient->generalState.ageCategoryHIVInfection = getAgeCategoryHIVInfection(ageMonths);
	patient->generalState.ageCategoryCHRMs = getAgeCategoryCHRMs(ageMonths);
	patient->generalState.ageCategoryPediatrics = getAgeCategoryPediatrics(ageMonths);
	patient->generalState.ageCategoryPedsCost = getAgeCategoryPediatricsCost(ageMonths);
	patient->generalState.ageCategoryPedsARTCost = getAgeCategoryPediatricsARTCost(ageMonths);
} /* end setPatientAgeGender */

/** \brief setInitialARTState initializes the patients ARTState object
 *
 *  Initializes all ART variables: by default, isOnART and hasTakenART are false and number of observed
 *  failures is 0.  The CD4 envelope and CD4 percentage envelope is set to non-active, all
 *  "number of months since" data relating to ART is set to 0, and all toxicity effects are cleared.
 **/
void StateUpdater::setInitialARTState() {
	patient->artState.isOnART = false;
	patient->artState.hasTakenART = false;
	patient->artState.numObservedFailures = 0;
	patient->artState.overallCD4Envelope.isActive = false;
	patient->artState.indivCD4Envelope.isActive = false;
	patient->artState.overallCD4PercentageEnvelope.isActive = false;
	patient->artState.indivCD4PercentageEnvelope.isActive = false;
	patient->artState.hadPrevToxicity = false;

	patient->artState.monthOfNewCD4MultArtFail=0;
	patient->artState.currCD4MultArtFail=1.0;
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

/**
 * \brief setInitialProphState initializes the patients ProphState object
 *
 * Initializes the state to reflect that no prophylaxis drugs are currently taken
 * or have a history of being taken
 * */
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

/** \brief setInitialTBProphState sets the initial TB proph state to not be on any TB prophs
 *
 *  Also sets the patient to not be scheduled to start any TB proph
 * */
void StateUpdater::setInitialTBProphState() {
	patient->tbState.isOnProph = false;
	patient->tbState.isScheduledForProph = false;
} /* end setInitialTBProphState */

/** \brief setInitialTBTreatmentState sets the initial TB treatment state to not be on TB treatment
 *
 *  Also sets the patient to not be scheduled to start any TB treatment
 * */
void StateUpdater::setInitialTBTreatmentState() {
	patient->tbState.isOnTreatment = false;
	patient->tbState.isScheduledForTreatment = false;
} /* end setInitialTBTreatmentState */

/** \brief incrementMonth increments the simulation month number and patient age by 1
 *
 *  Also resets the age categories using helper functions
 * \see StateUpdater::getAgeCategoryClinical(int)
 * \see StateUpdater::getAgeCategoryHIVInfection(int)
 * \see StateUpdater::getAgeCategoryCHRMs(int)
 * \see StateUpdater::getAgeCategoryPediatrics(int)
 *
 * */
void StateUpdater::incrementMonth() {
	patient->generalState.monthNum++;
	patient->generalState.ageMonths++;

	// Update the clinical, HIV infection, and pediatrics age category
	patient->generalState.ageCategoryClinical = getAgeCategoryClinical(patient->getGeneralState()->ageMonths);
	patient->generalState.ageCategoryHIVInfection = getAgeCategoryHIVInfection(patient->getGeneralState()->ageMonths);
	patient->generalState.ageCategoryCHRMs = getAgeCategoryCHRMs(patient->getGeneralState()->ageMonths);
	patient->generalState.ageCategoryPediatrics = getAgeCategoryPediatrics(patient->getGeneralState()->ageMonths);
	patient->generalState.ageCategoryPedsCost = getAgeCategoryPediatricsCost(patient->getGeneralState()->ageMonths);
} /* end incrementMonth */

/** \brief incrementDiscountFactor adjusts the discounting factor for each new month
 *
 * \param amount a double, the inverse of which is multiplied by the current discount factor
 *
 * \f$DiscountFactor_{new} = \frac{1}{amount} * DiscountFactor_{old}\f$
 * */
void StateUpdater::incrementDiscountFactor(double amount) {
	patient->generalState.discountFactor *= (1 / amount);
} /* end incrementDiscountFactor */

/** \brief setQOLMultiplier resets the quality of life factor back to 1
 * \param newQOL a double indicating the new QOL Multiplier
 **/
void StateUpdater::setQOLMultiplier(double newQOL=1.0) {
	if(newQOL<0.0 || newQOL>1.0 ){
		patient->generalState.QOLMultiplier=1.0;
		return;
	}
	patient->generalState.QOLMultiplier = newQOL;
} /* end setQOLMultiplier */

/** \brief accumulateQOLMultiplier accumulates the QOL by multiplying the new factor with the existing one
 *
 *  \param amount a double indicating the new factor to accumulate the QOL by
 *
 *  \f$QOL_{new} = QOL_{old} * amount\f$*/
void StateUpdater::accumulateQOLMultiplier(double amount) {
	patient->generalState.QOLMultiplier *= amount;
} /* end accumulateQOLMultiplier */

/** \brief setNonAIDSDeathRateMultiplier sets the value for additional probability of nonAIDS death
 *
 *  \param amount a double representing the new probability of nonAIDS death
 **/
void StateUpdater::setNonAIDSDeathRateMultiplier(double amount) {
	patient->generalState.nonAIDSDeathRateMultiplier = amount;
} /* end setNonAIDSDeathRateMultiplier */

/** \brief accumulateNonAIDSDeathRateMultiplier increments the value for additional probability of nonAIDS death
 *
 *  \param amount a double representing the new factor to accumulate in the nonAIDS death rate
 *
 *  \f$nonAIDSDeathRateMultiplier_{new} = nonAIDSDeathRateMultiplier_{old}*amount\f$
 **/
void StateUpdater::accumulateNonAIDSDeathRateMultiplier(double amount) {
	patient->generalState.nonAIDSDeathRateMultiplier *= amount;
} /* end accumulateNonAIDSDeathRateMultiplier */

/** \brief setInfectedHIVState sets the patient to the specified HIV infection state and updates statistics
 *
 *	\param infectedState a SimContext::HIV_INF (HIV infection state) to set the patient to
 *	\param isInitial a bool marking whether or not this is an initial (i.e. prevalent) case or not (i.e. incident case)
 *	\param isHighRisk a bool marking whether or not the patient is high risk; this only matters for HIV negative persons
 *	who draw from a different incident infection distribution
 *
 *	All statistics counting different infection types/times are incremented in this function
 **/
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

/** \brief setInfectedPediatricsHIVState sets the pediatrics HIV state and updates statistics
 *
 * \param hivState a SimContext::PEDS_HIV_STATE (HIV infection state for pediatrics) that the patient is set to
 * \param isInitial a bool that is currently unused
 **/
void StateUpdater::setInfectedPediatricsHIVState(SimContext::PEDS_HIV_STATE hivState, bool isInitial) {
	patient->diseaseState.infectedPediatricsHIVState = hivState;
} /* end setInfectedPediatricsHIVState */

/** \brief setInfectedMaternalHIVState sets the maternal HIV state for pediatrics and updates statistics
 *
 *  \param hivState a SimContext::PEDS_MOM_HIV_STATE that specifies the (pediatric) patient's mother's HIV status
 *  \param isInitial a bool indicating whether this is a prevalent or incident mother infection; prevalent cases are recorded
 *  in the initial peds infection tallies
 **/
void StateUpdater::setInfectedMaternalHIVState(SimContext::PEDS_MOM_HIV_STATE hivState, bool isInitial) {
	patient->generalState.isMotherAlive = true;
	patient->generalState.maternalInfectedHIVState = hivState;
	if (hivState != SimContext::PEDS_MOM_HIV_NEG){
		patient->generalState.monthOfMaternalHIVInfection = patient->generalState.monthNum;
		if (!isInitial){
			if (simContext->getPedsInputs()->exposedUninfectedDefsEarly[SimContext::PEDS_EXPOSED_BREASTFEEDING] && patient->getGeneralState()->breastfeedingStatus != SimContext::PEDS_BF_REPL){
				setExposedPediatricsState(true);
			}
			else if (simContext->getPedsInputs()->exposedUninfectedDefsEarly[SimContext::PEDS_EXPOSED_AFTER_WEANING] && patient->getGeneralState()->breastfeedingStatus == SimContext::PEDS_BF_REPL){
				setExposedPediatricsState(true);
			}
		}
	}
	if (isInitial) {
		runStats->initialDistributions.numInitialPediatrics[patient->diseaseState.infectedPediatricsHIVState][hivState]++;
	}
} /* end setInfectedMaternalHIVState */

/** setExposedHIVState sets the exposure state for pediatrics*/
void StateUpdater::setExposedPediatricsState(bool exposedState){
	patient->diseaseState.isExposed = exposedState;
}

/** \brief setBreastfeedingStatus for pediatrics and updates statistics
 *
 * \param bfType a SimContext::PEDS_BF_TYPE to set the breastfeeding status to
 **/
void StateUpdater::setBreastfeedingStatus(SimContext::PEDS_BF_TYPE bfType) {
	patient->generalState.breastfeedingStatus = bfType;
	if(bfType==SimContext::PEDS_BF_REPL){
		patient->generalState.monthOfReplacementFeedingStart=patient->generalState.monthNum;
	}
} /* end setBreastfeedingStatus */

/** \brief setPediatricsART sets whether or not the infant is on ART
 *  \param isOnART a bool that indicates whether or not the infant is on pediatric ART
 **/
void StateUpdater::setPediatricsART(bool isOnART) {
	patient->artState.isOnPediatricART = isOnART;
} /* end setPediatricsART */

/** \brief setCareState sets the patient's care state
 *
 *  \param typeCare a SimContext::HIV_Care indicating the state of patient care
 **/
void StateUpdater::setCareState(SimContext::HIV_CARE typeCare){
	patient->monitoringState.careState = typeCare;
}

/** \brief setDetectedHIVState sets the patient's detection status and updates statistics
 *
 *  \param isDetected a bool indicating whether or not the patient has been detected with HIV
 *  \param typeDetection a SimContext::HIV_DET indicating the type of detection
 *  \param oiType a SimContext::OI_TYPE indicating which OI (default OI_NONE) triggered the detection
 *
 *  If isDetected is true, all detection statistics are updated.
 **/
void StateUpdater::setDetectedHIVState(bool isDetected, SimContext::HIV_DET typeDetection, SimContext::OI_TYPE oiType) {
	patient->monitoringState.isDetectedHIVPositive = isDetected;
	// Update statistics for a newly detected patient, if screening module is enabled
	if (isDetected) {
		setCareState(SimContext::HIV_CARE_UNLINKED);
		double cd4Value = patient->diseaseState.currTrueCD4;
		SimContext::CD4_STRATA cd4Strata = patient->diseaseState.currTrueCD4Strata;
		SimContext::HVL_STRATA hvlStrata = patient->diseaseState.currTrueHVLStrata;
		SimContext::HIV_INF hivState = patient->diseaseState.infectedHIVState;
		int monthNum = patient->generalState.monthNum;
		int ageMonths = patient->generalState.ageMonths;
		SimContext::GENDER_TYPE gender = patient->generalState.gender;
		if (typeDetection != SimContext::HIV_DET_BACKGROUND_PREV_DET && typeDetection != SimContext::HIV_DET_OI_PREV_DET && typeDetection != SimContext::HIV_DET_SCREENING_PREV_DET)
			runStats->hivScreening.numDetectedGender[gender]++;
		if (typeDetection == SimContext::HIV_DET_OI) {
			runStats->hivScreening.numDetectedByOIs[oiType]++;
		}
		if (typeDetection == SimContext::HIV_DET_OI_PREV_DET){
			runStats->hivScreening.numDetectedByOIsPrevDetected[oiType]++;
		}
		if (patient->diseaseState.isPrevalentHIVCase) {
			runStats->hivScreening.numDetectedPrevalentMeans[typeDetection]++;
			if (typeDetection != SimContext::HIV_DET_BACKGROUND_PREV_DET && typeDetection != SimContext::HIV_DET_OI_PREV_DET && typeDetection != SimContext::HIV_DET_SCREENING_PREV_DET){
				runStats->hivScreening.numAtDetectionPrevalentCD4HIV[cd4Strata][hivState]++;
				runStats->hivScreening.numAtDetectionPrevalentHVLHIV[hvlStrata][hivState]++;
				runStats->hivScreening.CD4AtDetectionPrevalentSumHIV[hivState] += cd4Value;
				runStats->hivScreening.monthsToDetectionPrevalentSum += monthNum;
				runStats->hivScreening.monthsToDetectionPrevalentSumSquares += monthNum * monthNum;
				runStats->hivScreening.ageMonthsAtDetectionPrevalentSum += ageMonths;
				runStats->hivScreening.ageMonthsAtDetectionPrevalentSumSquares += ageMonths * ageMonths;
			}
		}
		else {
			runStats->hivScreening.numDetectedIncidentMeans[typeDetection]++;
			if (typeDetection != SimContext::HIV_DET_BACKGROUND_PREV_DET && typeDetection != SimContext::HIV_DET_OI_PREV_DET && typeDetection != SimContext::HIV_DET_SCREENING_PREV_DET){
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
	}//end if(isdetected)
	else{
		if(patient->diseaseState.infectedHIVState!=SimContext::HIV_INF_NEG)
			setCareState(SimContext::HIV_CARE_UNDETECTED);
	}
} /* end setDetectedHIVState */

/** \brief setLinkedHIVState sets the patient's Link status
  *  \param typeDetection a SimContext::HIV_DET indicating the type of detection
 **/
void StateUpdater::setLinkedState(bool isLinked, SimContext::HIV_DET typeLinked) {
	// Update statistics for a newly linked patient, if screening module is enabled
	patient->monitoringState.isLinked = isLinked;
	if (isLinked){
		patient->monitoringState.monthOfLinkage=patient->generalState.monthNum;
		setCareState(SimContext::HIV_CARE_IN_CARE);
		SimContext::CD4_STRATA cd4Strata = patient->diseaseState.currTrueCD4Strata;
		SimContext::HVL_STRATA hvlStrata = patient->diseaseState.currTrueHVLStrata;
		SimContext::HIV_INF hivState = patient->diseaseState.infectedHIVState;
		double cd4Value = patient->diseaseState.currTrueCD4;
		int monthNum = patient->generalState.monthNum;
		int ageMonths = patient->generalState.ageMonths;

		runStats->hivScreening.numLinkedMeans[typeLinked]++;
		runStats->hivScreening.numAtLinkageCD4HIV[cd4Strata][hivState]++;
		runStats->hivScreening.numAtLinkageHVLHIV[hvlStrata][hivState]++;
		runStats->hivScreening.CD4AtLinkageSumHIV[hivState]+=cd4Value;
		runStats->hivScreening.monthsToLinkageSum += monthNum;
		runStats->hivScreening.monthsToLinkageSumSquares += monthNum*monthNum;
		runStats->hivScreening.ageMonthsAtLinkageSum += ageMonths;
		runStats->hivScreening.ageMonthsAtLinkageSumSquares += ageMonths * ageMonths;

		runStats->hivScreening.monthsToLinkageSumMeans[typeLinked] += monthNum;
		runStats->hivScreening.monthsToLinkageSumSquaresMeans[typeLinked] += monthNum*monthNum;
	}
} /* end setLinkedHIVState */

/** \brief updateHIVTestingStats updates all statistics after an HIV testing event
 *
 * \param acceptTest a bool that indicates whether or not the patient accepted the test -- returnResults and isPositive are
 * irrelevant if this is false
 * \param returnResults a bool that indicates whether or not the patient returned for the result -- if this is false,
 * isPositive is irrelevant
 * \param isPositive a bool indicating whether or not the patient's test came back HIV positive
 *
 * If the patient accepted the test and returned for the test, the test results are recorded in the statistics depending
 * on whether it was a true positive, true negative, false positive, or false negative.
 **/
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

/** \brief updateLabStagingStats updates all statistics after an Lab Staging event
 *
 * \param acceptTest a bool that indicates whether or not the patient accepted the test -- returnResults and isPositive are
 * irrelevant if this is false
 * \param returnResults a bool that indicates whether or not the patient returned for the result -- if this is false,
 * hasLinked is irrelevant
 * \param hasLinked a bool indicating whether or not the patient links to care
 *
 **/
void StateUpdater::updateLabStagingStats(bool acceptTest, bool returnResults, bool hasLinked) {
	if (patient->diseaseState.infectedHIVState == SimContext::HIV_INF_NEG)
		return;
	if (!acceptTest) {
		runStats->hivScreening.numRefuseLabStaging++;
		return;
	}
	SimContext::HIV_POS infectedState = (SimContext::HIV_POS) (patient->diseaseState.infectedHIVState-1);

	runStats->hivScreening.numAcceptLabStagingHIVState[infectedState]++;
	runStats->hivScreening.numAcceptLabStaging++;

	if (!returnResults) {
		runStats->hivScreening.numNoReturnForResultsLabStaging++;
		return;
	}
	SimContext::CD4_STRATA trueCD4Strata = patient->diseaseState.currTrueCD4Strata;
	SimContext::CD4_STRATA obsvCD4Strata = patient->monitoringState.currObservedCD4Strata;

	runStats->hivScreening.numReturnLabStagingHIVState[infectedState]++;
	runStats->hivScreening.numReturnForResultsLabStaging++;

	runStats->hivScreening.numReturnLabStagingObsvCD4[obsvCD4Strata]++;
	runStats->hivScreening.numReturnLabStagingTrueCD4[trueCD4Strata]++;
	runStats->hivScreening.numReturnLabStagingObsvTrueCD4[obsvCD4Strata][trueCD4Strata]++;
	if(!hasLinked){
		runStats->hivScreening.numNoLinkLabStaging++;
		return;
	}

	runStats->hivScreening.numLinkLabStagingObsvCD4[obsvCD4Strata]++;
	runStats->hivScreening.numLinkLabStagingTrueCD4[trueCD4Strata]++;
	runStats->hivScreening.numLinkLabStagingObsvTrueCD4[obsvCD4Strata][trueCD4Strata]++;
	runStats->hivScreening.numLinkLabStaging++;
} /* end updateHIVTestingStats */

/** \brief setHIVTestingParams sets the interval and acceptance rate for HIV testing
 *
 * \param intervalIndex an integer representing which HIV Testing Interval to assign to the patient based on user specified stratification
 * \param acceptanceRateIndex an integer representing which HIV Test Acceptance Rate to assign to the patient based on user specified stratification
 *
 * The patient's HIV testing interval and acceptance rate is set and the runStats relating to the number of patients in each testing interval and acceptance rate are incremented
 **/
void StateUpdater::setHIVTestingParams(int intervalIndex, int acceptanceRateIndex) {
	SimContext::HIV_EXT_INF extInfectedState = (SimContext::HIV_EXT_INF) patient->diseaseState.infectedHIVState;
	if ((patient->diseaseState.infectedHIVState == SimContext::HIV_INF_NEG) && !patient->getMonitoringState()->isHighRiskForHIV)
		extInfectedState = SimContext::HIV_EXT_INF_NEG_LO;
	patient->monitoringState.intervalHIVTest = simContext->getHIVTestInputs()->HIVTestingInterval[intervalIndex];
	patient->monitoringState.acceptanceRateHIVTest = simContext->getHIVTestInputs()->HIVTestAcceptRate[extInfectedState][acceptanceRateIndex];

	runStats->hivScreening.numTestingInterval[intervalIndex]++;
	runStats->hivScreening.numTestingAcceptRate[acceptanceRateIndex][extInfectedState]++;
} /* setHIVTestingParams */

/** \brief setChanceCD4Test sets whether the patient has had an option to get a cd4 test
 *
 * \param hadChance a bool indicating whether or not patient had a chance for cd4 test
 **/
void StateUpdater::setChanceCD4Test(bool hadChance) {
	patient->monitoringState.hadChanceCD4Test=hadChance;
} /* end setChanceCD4Test */

/** \brief setChanceHVLTest sets whether the patient has had an option to get a HVL test
 *
 * \param hadChance a bool indicating whether or not patient had a chance for HVL test
 **/
void StateUpdater::setChanceHVLTest(bool hadChance) {
	patient->monitoringState.hadChanceHVLTest=hadChance;
} /* end setChanceHVLTest */

/** \brief scheduleHIVTest sets the month of the next HIV test
 *
 * \param hasNext a bool indicating whether or not an HIV test should be scheduled
 * \param monthNum an integer specifying which month the next HIV test should be scheduled for if hasNext is true
 **/
void StateUpdater::scheduleHIVTest(bool hasNext, int monthNum) {
	patient->monitoringState.hasScheduledHIVTest = hasNext;
	if (hasNext)
		patient->monitoringState.monthOfScheduledHIVTest = monthNum;
} /* end scheduleHIVTest */

/** \brief scheduleCD4Test sets the month of the next CD4 test
 *
 * \param hasNext a bool indicating whether or not a CD4 test should be scheduled
 * \param monthNum an integer specifying which month the next CD4 test should be scheduled for if hasNext is true
 **/
void StateUpdater::scheduleCD4Test(bool hasNext, int monthNum) {
	if (monthNum<simContext->getTreatmentInputs()->CD4TestingLag){
		patient->monitoringState.hasScheduledCD4Test=false;
		return;
	}
	patient->monitoringState.hadChanceCD4Test=true;
	patient->monitoringState.hasScheduledCD4Test = hasNext;
	if (hasNext)
		patient->monitoringState.monthOfScheduledCD4Test = monthNum;
} /* end scheduleCD4Test */

/** \brief scheduleHVLTest sets the month of the next HVL test
 *
 * \param hasNext a bool indicating whether or not an HVL test should be scheduled
 * \param monthNum an integer specifying which month the next HVL test should be scheduled for if hasNext is true
 **/
void StateUpdater::scheduleHVLTest(bool hasNext, int monthNum) {
	if (monthNum<simContext->getTreatmentInputs()->HVLTestingLag){
		patient->monitoringState.hasScheduledHVLTest=false;
		return;
	}
	patient->monitoringState.hadChanceHVLTest=true;
	patient->monitoringState.hasScheduledHVLTest = hasNext;
	if (hasNext)
		patient->monitoringState.monthOfScheduledHVLTest = monthNum;
} /* end scheduleHVLTest */

/** \brief setClinicVisitType sets the conditions for a clinic visit and available treatments
 *
 * \param visitType a SimContext::CLINIC_VISITS specifying what type of clinic visit this is
 * \param treatmentType a SimContext::THERAPY_IMPL indicating what the treatment type the patient has available
 *
 * If treatmentType is SimContext::THERAPY_IMPL_NONE, the patient may neither receive ART nor receive prophylaxis.
 * If treatmentType is SimContext::THERAPY_IMPL_PROPH, the patient may receive prophylaxis, but not ART.
 * If treatmentType is SimContext::THERAPY_IMPL_PROPH_ART, the patient may receive both ART and prophylaxis
 * */
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
			break;
	}
} /* end setClinicVisitType */

/** \brief setResponseBaseline sets the baseline propensity to respond coefficient
 *
 * \param baseline a double representing the baseline propensity to respond logit
 **/
void StateUpdater::setResponseBaseline(double baseline) {
	patient->generalState.responseBaselineLogit = baseline;
} /* end setResponseBaseline */

/** \brief setPreARTResponseBase sets the baseline propensity to respond coefficient for preART
 *
 * \param baseline a double representing the baseline propensity to respond logit
 **/
void StateUpdater::setPreARTResponseBase(double baseline) {
	patient->generalState.responseLogitPreARTBase = baseline;
} /* end setPreARTResponseBase */

/** \brief setARTResponsCurrRegimenBase sets the propensity to respond coefficient for the current ART regimen without adherence interventions
 *
 * \param respLogit a double representing the propensity to respond logit
 * \param responseRegimenIncrLogit is the regimen specific increment to propensity to respond
 **/
void StateUpdater::setARTResponseCurrRegimenBase(double responseLogit, double responseRegimenIncrLogit) {
	patient->artState.responseLogitCurrRegimenBase = responseLogit;
	patient->artState.responseLogitCurrRegimenIncrement = responseRegimenIncrLogit;
} /* end setARTResponseCurrRegimenBase */

/** \brief setCD4ResponseType sets the predisposed CD4 response type of the patient
 *
 * \param responseType the SimContext::CD4_RESPONSE_TYPE to set as the patient's CD4 response type
 **/
void StateUpdater::setCD4ResponseType(SimContext::CD4_RESPONSE_TYPE responseType) {
	patient->artState.CD4ResponseType = responseType;
} /* end setCD4ResponseType */

/** \brief setRiskFactor sets whether or not the patient has risk factor x
 *
 * \param riskNum an integer specifying the risk number
 * \param hasRisk a bool specifying whether or not the patient has the specified risk factor
 * \param isInitial a bool indicating whether or not this is the initial state of the risk factor
 *
 * If isInitial is true, the runStats corresponding to tallying the initial distribution of risk factors is incremented
 **/
void StateUpdater::setRiskFactor(int riskNum, bool hasRisk, bool isInitial) {
	patient->generalState.hasRiskFactor[riskNum] = hasRisk;

	if (isInitial && hasRisk) {
		runStats->initialDistributions.numRiskFactors[riskNum]++;
	}
} /* end setRiskFactor */

/** \brief scheduleInitialCD4Test sets the initial cd4 tests after they become available
 *
 * \see StateUpdater::scheduleCD4Test(bool, int)
 * \see StateUpdater::scheduleHVLTest(bool, int)
 * */
void StateUpdater::scheduleInitialCD4Test(int monthNum) {
	if (!patient->artState.hasTakenART){
		if (patient->diseaseState.currTrueCD4 > simContext->getTreatmentInputs()->testingIntervalCD4Threshold) {
			if (simContext->getTreatmentInputs()->CD4TestingIntervalPreARTHighCD4 != SimContext::NOT_APPL)
				scheduleCD4Test(true, monthNum);
			else
				scheduleCD4Test(false);
		}
		else {
			if (simContext->getTreatmentInputs()->CD4TestingIntervalPreARTLowCD4 != SimContext::NOT_APPL)
				scheduleCD4Test(true, monthNum);
			else
				scheduleCD4Test(false);
		}
	}
	else if (patient->artState.hasNextRegimenAvailable){
		int monthsOnART = patient->generalState.monthNum - patient->artState.monthOfCurrRegimenStart;
		if (monthsOnART < simContext->getTreatmentInputs()->testingIntervalARTMonthsThreshold){
			if(simContext->getTreatmentInputs()->CD4TestingIntervalOnART[0] != SimContext::NOT_APPL){
				scheduleCD4Test(true,monthNum);
			}
			else{
				scheduleCD4Test(false);
			}
		}
		else{
			if(simContext->getTreatmentInputs()->CD4TestingIntervalOnART[1]!= SimContext::NOT_APPL){
				scheduleCD4Test(true,monthNum);
			}
			else{
				scheduleCD4Test(false);
			}
		}
	}
	else if (!patient->artState.hasObservedFailure){
		int monthsOnART = patient->generalState.monthNum - patient->artState.monthOfCurrRegimenStart;
		if (monthsOnART < simContext->getTreatmentInputs()->testingIntervalLastARTMonthsThreshold){
			if(simContext->getTreatmentInputs()->CD4TestingIntervalOnLastART[0] != SimContext::NOT_APPL){
				scheduleCD4Test(true,monthNum);
			}
			else{
				scheduleCD4Test(false);
			}
		}
		else{
			if(simContext->getTreatmentInputs()->CD4TestingIntervalOnLastART[1]!= SimContext::NOT_APPL){
				scheduleCD4Test(true,monthNum);
			}
			else{
				scheduleCD4Test(false);
			}
		}
	}
	else{
		if(simContext->getTreatmentInputs()->CD4TestingIntervalPostART != SimContext::NOT_APPL){
			scheduleCD4Test(true,monthNum);
		}
		else{
			scheduleCD4Test(false);
		}
	}
}

/** \brief scheduleInitialHVLTest sets the initial hvl tests after they become available
 *
 * \see StateUpdater::scheduleCD4Test(bool, int)
 * \see StateUpdater::scheduleHVLTest(bool, int)
 * */
void StateUpdater::scheduleInitialHVLTest(int monthNum) {
	if (!patient->artState.hasTakenART){
			if (patient->diseaseState.currTrueCD4 > simContext->getTreatmentInputs()->testingIntervalCD4Threshold) {
				if (simContext->getTreatmentInputs()->HVLTestingIntervalPreARTHighCD4 != SimContext::NOT_APPL)
					scheduleHVLTest(true, monthNum);
				else
					scheduleHVLTest(false);
			}
			else {
				if (simContext->getTreatmentInputs()->HVLTestingIntervalPreARTLowCD4 != SimContext::NOT_APPL)
					scheduleHVLTest(true, monthNum);
				else
					scheduleHVLTest(false);
			}
		}
		else if (patient->artState.hasNextRegimenAvailable){
			int monthsOnART = patient->generalState.monthNum - patient->artState.monthOfCurrRegimenStart;
			if (monthsOnART < simContext->getTreatmentInputs()->testingIntervalARTMonthsThreshold){
				if(simContext->getTreatmentInputs()->HVLTestingIntervalOnART[0] != SimContext::NOT_APPL){
					scheduleHVLTest(true,monthNum);
				}
				else{
					scheduleHVLTest(false);
				}
			}
			else{
				if(simContext->getTreatmentInputs()->HVLTestingIntervalOnART[1]!= SimContext::NOT_APPL){
					scheduleHVLTest(true,monthNum);
				}
				else{
					scheduleHVLTest(false);
				}
			}
		}
		else if (!patient->artState.hasObservedFailure){
			int monthsOnART = patient->generalState.monthNum - patient->artState.monthOfCurrRegimenStart;
			if (monthsOnART < simContext->getTreatmentInputs()->testingIntervalLastARTMonthsThreshold){
				if(simContext->getTreatmentInputs()->HVLTestingIntervalOnLastART[0] != SimContext::NOT_APPL){
					scheduleHVLTest(true,monthNum);
				}
				else{
					scheduleHVLTest(false);
				}
			}
			else{
				if(simContext->getTreatmentInputs()->HVLTestingIntervalOnLastART[1]!= SimContext::NOT_APPL){
					scheduleHVLTest(true,monthNum);
				}
				else{
					scheduleHVLTest(false);
				}
			}
		}
		else{
			if(simContext->getTreatmentInputs()->HVLTestingIntervalPostART != SimContext::NOT_APPL){
				scheduleHVLTest(true,monthNum);
			}
			else{
				scheduleHVLTest(false);
			}
		}

}

/** \brief scheduleInitialClinicVisit sets the month of initial clinic visit, CD4 test, and HVL test
 *
 * The initial clinic visit is set to occur in the current month.  Depending on user specification for
 * pre-ART behavior with a given CD4 threshhold, CD4 and HVL tests either are or aren't scheduled for the
 * current month based on the patient's CD4 count.
 *
 * \see StateUpdater::scheduleCD4Test(bool, int)
 * \see StateUpdater::scheduleHVLTest(bool, int)
 * */
void StateUpdater::scheduleInitialClinicVisit() {
	int monthNum = patient->generalState.monthNum;
	scheduleRegularClinicVisit(true, monthNum);
	if(patient->generalState.monthNum>=simContext->getTreatmentInputs()->CD4TestingLag){
		scheduleInitialCD4Test(monthNum);
	}
	if(patient->generalState.monthNum>=simContext->getTreatmentInputs()->HVLTestingLag){
		scheduleInitialHVLTest(monthNum);
	}
} /* end scheduleInitialClinicVisit */

/** \brief scheduleRegularClinicVisit sets the month of the next regularly scheduled clinic visit
 *
 * \param hasNext a bool indicating whether or not the patient should schedule an upcoming clinic visit
 * \param monthNum an integer indicating which month the regular clinic visit should be scheduled for if hasNext is true
 * \param scheduleInitialCD4 indicates whether to attempt to schedule an initial cd4 test or not.  This is by default true
 * \param scheduleInitialHVL indicates whether to attempt to schedule an initial HVL test or not.  This is by default true
 **/
void StateUpdater::scheduleRegularClinicVisit(bool hasNext, int monthNum,bool scheduleInitialCD4,bool scheduleInitialHVL) {
	patient->monitoringState.hasRegularClinicVisit = hasNext;
	if (hasNext){
		patient->monitoringState.monthOfRegularClinicVisit = monthNum;
		if(scheduleInitialCD4){
			if(monthNum>=simContext->getTreatmentInputs()->CD4TestingLag && !patient->monitoringState.hadChanceCD4Test){
				scheduleInitialCD4Test(monthNum);
			}
		}
		if(scheduleInitialHVL){
			if(monthNum>=simContext->getTreatmentInputs()->HVLTestingLag && !patient->monitoringState.hadChanceHVLTest){
				scheduleInitialHVLTest(monthNum);
			}
		}
	}
} /* end scheduleRegularClinicVisit */

/** \brief scheduleEmergencyClinicVisit sets the month of the next emergency clinic visit
 *
 * \param hasNext a bool indicating whether or not the patient should schedule an emergency clinic visit
 * \param monthNum an integer indicating which month the emergency clinic visit should be scheduled for if hasNext is true
 * */
void StateUpdater::scheduleEmergencyClinicVisit(bool hasNext, int monthNum,bool scheduleInitialCD4,bool scheduleInitialHVL) {
	patient->monitoringState.hasEmergencyClinicVisit = hasNext;
	if (hasNext){
		patient->monitoringState.monthOfEmergencyClinicVisit = monthNum;
		if(scheduleInitialCD4){
			if(monthNum>=simContext->getTreatmentInputs()->CD4TestingLag && !patient->monitoringState.hadChanceCD4Test){
				scheduleInitialCD4Test(monthNum);
			}
		}
		if(scheduleInitialHVL){
			if(monthNum>=simContext->getTreatmentInputs()->HVLTestingLag && !patient->monitoringState.hadChanceHVLTest){
				scheduleInitialHVLTest(monthNum);
			}
		}
	}
} /* end scheduleEmergencyClinicVisit */

/** \brief resetCliniVisitState resets state keeping track of event since the last clinic visit
 *
 * \param isInitial a bool indicating whether or not this is called before the first clinic visit.
 * If isInitial is true, "hadPrevClinicVisit" is set to false (because no clinic visit has occurred).
 * Otherwise, "hadPrevClinicVisit" is set to true.
 *
 * The counters for number of observed and true OIs since last clinic visit are also reset by this function
 **/
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

/** \brief incrementNumClinicVisits increments the total number of clinic visits by 1*/
void StateUpdater::incrementNumClinicVisits() {
	runStats->popSummary.totalClinicVisits++;
} /* end incrementNumClinicVisits */

/** \brief incrementNumObservedOIs increments the patients observed OIs during a clinic visit
 *
 * \param oiType a SimContext::OI_TYPE specifying the OI that was observed
 * \param numObserved the number of OIs that were observed
 *
 * The patient's number of observed OIs of type oiType are incremented by numObserved.
 **/
void StateUpdater::incrementNumObservedOIs(SimContext::OI_TYPE oiType, int numObserved) {
	// Update the patient state for the observed OI
	patient->monitoringState.numObservedOIsTotal[oiType] += numObserved;
	patient->monitoringState.numObservedOIsSinceLastVisit[oiType] += numObserved;
	patient->artState.numObservedOIsSinceFailOrStopART[oiType] += numObserved;

	// Update the statistics for the observed OI
	SimContext::CD4_STRATA cd4Strata = patient->diseaseState.currTrueCD4Strata;
	runStats->oiStats.numDetectedOIsCD4OI[cd4Strata][oiType] += numObserved;
} /* end incrementNumObservedOIs */

/** \brief setCurrLTFUStats updates the state and statisitics for a patient being LTFU or RTC
 *
 * \param ltfuState a SimContext::LTFU_STATE indicating which LTFU state to set the patient to
 *
 * The month of state change is recorded, as well as the number of months in the previous state.
 * If the new state is SimContext::LTFU_STATE_LOST, the patient's monitoring state keeps track of
 * whether or not the patient was on ART when entering this state.  The patient's previous state
 * is recorded in the monitoring state.  RunStats statistics related to LTFU are also updated.
 **/
void StateUpdater::setCurrLTFUState(SimContext::LTFU_STATE ltfuState) {
	// Update patient state for initial state, lost to follow up, or return to care
	patient->monitoringState.currLTFUState = ltfuState;
	int monthsPrevState = patient->generalState.monthNum - patient->monitoringState.monthOfLTFUStateChange;
	patient->monitoringState.monthOfLTFUStateChange = patient->generalState.monthNum;
	if (ltfuState == SimContext::LTFU_STATE_LOST) {
		setCareState(SimContext::HIV_CARE_LTFU);
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
				currTime->numStartingLostToFollowUpART[patient->artState.currRegimenNum]++;
			else if (!patient->artState.hasTakenART)
				currTime->numStartingLostToFollowUpPreART++;
			else
				currTime->numStartingLostToFollowUpPostART++;
		}
	}
	else if (ltfuState == SimContext::LTFU_STATE_RETURNED) {
		setCareState(SimContext::HIV_CARE_RTC);
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

/** \brief startNextARTRegimen updates the state to begin the next ART treatment regimen
 *
 * The patient state is updated to reflect the beginning of an ART regimen (resetting counters, etc).
 * ART initiation statistics are NOT updated here.  That is done after ART efficacy is determined
 **/
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

/** \brief startNextARTSubRegimen updates the state to begin the next ART treatment subregimen
 *
 * Only counters relating to subregimens (i.e. toxicity and subregimen counters) are updated here
 *
 * \param nextSubRegimen an integer specifying the index of the new subregimen to start
 **/
void StateUpdater::startNextARTSubRegimen(int nextSubRegimen) {
	patient->artState.currSubRegimenNum = nextSubRegimen;
	patient->artState.monthOfCurrSubRegimenStart = patient->generalState.monthNum;
	patient->artState.hasMajorToxicity = false;
	patient->artState.hasSevereToxicity = false;
} /* end startNextARTSubRegmin */

/** \brief setCurrARTEfficacy updates the destined efficacy of the ART regimen
 *
 * The ART suppression level is set based on efficacyType.  If this is the initial draw, ART statistics are
 * updated here.  Statistics are also updated if efficacyType indicates true failure.
 *
 * \param efficacyType a SimContext::ART_EFF_TYPE indicating the efficacy of the current ART regimen
 * \param isInitial a boolean that is true if this is the first time this ART regimen has had its efficacy set
 **/
void StateUpdater::setCurrARTEfficacy(SimContext::ART_EFF_TYPE efficacyType, bool isInitial) {
	// Update patient state with new ART suppression level
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

		SimContext::CD4_RESPONSE_TYPE cd4Response = patient->artState.CD4ResponseType;

		for(int i=0;i<SimContext::HET_NUM_OUTCOMES;i++){
			SimContext::RESP_TYPE responseType = patient->artState.responseTypeCurrRegimen[i];
			runStats->artStats.numOnARTAtInitResp[artLineNum][i][responseType]++;
			runStats->artStats.trueCD4AtInitSumResp[artLineNum][i][responseType] += patient->diseaseState.currTrueCD4;
			if(patient->monitoringState.hasObservedCD4){
				runStats->artStats.observedCD4AtInitSumResp[artLineNum][i][responseType] += patient->monitoringState.currObservedCD4;
			}
			runStats->artStats.numDrawEfficacyAtInitResp[artLineNum][efficacyType][i][responseType]++;
			runStats->artStats.numCD4ResponseTypeAtInitResp[artLineNum][cd4Response][i][responseType]++;

			for (int j = 0; j < SimContext::RISK_FACT_NUM; j++) {
				if (patient->getGeneralState()->hasRiskFactor[i]){
					runStats->artStats.numWithRiskFactorAtInitResp[artLineNum][j][i][responseType]++;
				}
			}
		}
		runStats->artStats.distributionAtInit[artLineNum][cd4Strata][hvlStrata]++;
		runStats->artStats.numOnARTAtInit[artLineNum]++;


		runStats->artStats.trueCD4AtInitSum[artLineNum]+=patient->diseaseState.currTrueCD4;
		if (patient->monitoringState.hasObservedCD4){
			runStats->artStats.observedCD4AtInitSum[artLineNum]+=patient->monitoringState.currObservedCD4;
		}


		runStats->artStats.numDrawEfficacyAtInit[artLineNum][efficacyType]++;
		runStats->artStats.numCD4ResponseTypeAtInit[artLineNum][cd4Response]++;

		for (int i = 0; i < SimContext::RISK_FACT_NUM; i++) {
			if (patient->getGeneralState()->hasRiskFactor[i]){
				runStats->artStats.numWithRiskFactorAtInit[artLineNum][i]++;
				break;
			}
		}
	}
	if (efficacyType == SimContext::ART_EFF_FAILURE) {
		// Update true failure statistics, same as initial if efficacy was failure
		int artLineNum = patient->artState.currRegimenNum;
		int monthsToFail = patient->generalState.monthNum - patient->getARTState()->monthOfCurrRegimenStart;

		for(int i=0;i<SimContext::HET_NUM_OUTCOMES;i++){
			SimContext::RESP_TYPE responseType = patient->artState.responseTypeCurrRegimen[i];
			runStats->artStats.numTrueFailureResp[artLineNum][i][responseType]++;
			runStats->artStats.trueCD4AtTrueFailureSumResp[artLineNum][i][responseType] += patient->diseaseState.currTrueCD4;
			if (patient->monitoringState.hasObservedCD4){
				runStats->artStats.observedCD4AtTrueFailureSumResp[artLineNum][i][responseType] += patient->monitoringState.currObservedCD4;
			}
			runStats->artStats.monthsToTrueFailureSumResp[artLineNum][i][responseType] += monthsToFail;
			runStats->artStats.monthsToTrueFailureSumSquaresResp[artLineNum][i][responseType] += monthsToFail * monthsToFail;
		}
		runStats->artStats.numTrueFailure[artLineNum]++;
		runStats->artStats.trueCD4AtTrueFailureSum[artLineNum]+=patient->diseaseState.currTrueCD4;
		if (patient->monitoringState.hasObservedCD4){
			runStats->artStats.observedCD4AtTrueFailureSum[artLineNum] += patient->monitoringState.currObservedCD4;
		}

		runStats->artStats.monthsToTrueFailureSum[artLineNum] += monthsToFail;
		runStats->artStats.monthsToTrueFailureSumSquares[artLineNum]+= monthsToFail * monthsToFail;
	}
} /* end setCurrARTEfficacy */

/** \brief setCurrARTResponse sets the calculated ART propensity to respond and response type
 *
 * The response type is set based on propRespond and the user defined responseTypeThresholds found in
 * the heterogeneity inputs.  The response factor is calculated as:
 * \f$ \frac{propRespond - prop_{Lower}}{prop_{Upper} - prop_{Lower}}\f$
 *	this value is 0 if the response=L1 and 1 if response=L2
 *
 * \param propRespond a double indicating the propensity to respond
 **/
void StateUpdater::setCurrARTResponse(double responseLogit) {
	// Update the patient state with the propensity to respond and response type
	double propRespond = pow(1 + exp(0 - responseLogit), -1);
	int artLineNum = patient->artState.currRegimenNum;
	patient->artState.responseLogitCurrRegimen=responseLogit;

	for(int i=0;i<SimContext::HET_NUM_OUTCOMES;i++){
		double L1,L2;

		if (patient->generalState.ageCategoryPediatrics == SimContext::PEDS_AGE_ADULT){
			L1=simContext->getARTInputs(artLineNum)->responseTypeThresholds[i][0];
			L2=simContext->getARTInputs(artLineNum)->responseTypeThresholds[i][1];
		}
		else if (patient->generalState.ageCategoryPediatrics == SimContext::PEDS_AGE_LATE){
			L1=simContext->getPedsARTInputs(artLineNum)->responseTypeThresholdsLate[i][0];
			L2=simContext->getPedsARTInputs(artLineNum)->responseTypeThresholdsLate[i][1];
		}
		else{
			L1=simContext->getPedsARTInputs(artLineNum)->responseTypeThresholdsEarly[i][0];
			L2=simContext->getPedsARTInputs(artLineNum)->responseTypeThresholdsEarly[i][1];
		}

		if (propRespond > L2) {
			patient->artState.responseTypeCurrRegimen[i] = SimContext::RESP_TYPE_FULL;
			patient->artState.responseFactorCurrRegimen[i] = 1.0;
		}
		else if (propRespond > L1) {
			patient->artState.responseTypeCurrRegimen[i] = SimContext::RESP_TYPE_PARTIAL;
			patient->artState.responseFactorCurrRegimen[i] = (propRespond - L1) / (L2 - L1);

			if (i == SimContext::HET_OUTCOME_SUPP || i == SimContext::HET_OUTCOME_LATEFAIL){
				double exponent;
				if (patient->generalState.ageCategoryPediatrics == SimContext::PEDS_AGE_ADULT){
					exponent = simContext->getARTInputs(artLineNum)->responseTypeExponents[i];
				}
				else if (patient->generalState.ageCategoryPediatrics == SimContext::PEDS_AGE_LATE){
					exponent = simContext->getPedsARTInputs(artLineNum)->responseTypeExponentsLate[i];
				}
				else{
					exponent = simContext->getPedsARTInputs(artLineNum)->responseTypeExponentsEarly[i];
				}
				patient->artState.responseFactorCurrRegimen[i] = pow(patient->artState.responseFactorCurrRegimen[i], exponent);
			}
		}
		else {
			patient->artState.responseTypeCurrRegimen[i] = SimContext::RESP_TYPE_NON;
			patient->artState.responseFactorCurrRegimen[i] = 0.0;
		}
	}

	double upperValue,lowerValue,responseFactor;

	if (patient->generalState.ageCategoryPediatrics == SimContext::PEDS_AGE_ADULT){
		upperValue=simContext->getARTInputs(artLineNum)->responseTypeValues[SimContext::HET_OUTCOME_SUPP][1];
		lowerValue=simContext->getARTInputs(artLineNum)->responseTypeValues[SimContext::HET_OUTCOME_SUPP][0];
		responseFactor=patient->artState.responseFactorCurrRegimen[SimContext::HET_OUTCOME_SUPP];
		patient->artState.probInitialEfficacy=lowerValue+responseFactor*(upperValue-lowerValue);

		upperValue=simContext->getARTInputs(artLineNum)->responseTypeValues[SimContext::HET_OUTCOME_LATEFAIL][1];
		lowerValue=simContext->getARTInputs(artLineNum)->responseTypeValues[SimContext::HET_OUTCOME_LATEFAIL][0];
		responseFactor=patient->artState.responseFactorCurrRegimen[SimContext::HET_OUTCOME_LATEFAIL];
		patient->artState.probLateFail=lowerValue+responseFactor*(upperValue-lowerValue);
	}
	else if (patient->generalState.ageCategoryPediatrics == SimContext::PEDS_AGE_LATE){
		upperValue=simContext->getPedsARTInputs(artLineNum)->responseTypeValuesLate[SimContext::HET_OUTCOME_SUPP][1];
		lowerValue=simContext->getPedsARTInputs(artLineNum)->responseTypeValuesLate[SimContext::HET_OUTCOME_SUPP][0];
		responseFactor=patient->artState.responseFactorCurrRegimen[SimContext::HET_OUTCOME_SUPP];
		patient->artState.probInitialEfficacy=lowerValue+responseFactor*(upperValue-lowerValue);

		upperValue=simContext->getPedsARTInputs(artLineNum)->responseTypeValuesLate[SimContext::HET_OUTCOME_LATEFAIL][1];
		lowerValue=simContext->getPedsARTInputs(artLineNum)->responseTypeValuesLate[SimContext::HET_OUTCOME_LATEFAIL][0];
		responseFactor=patient->artState.responseFactorCurrRegimen[SimContext::HET_OUTCOME_LATEFAIL];
		patient->artState.probLateFail=lowerValue+responseFactor*(upperValue-lowerValue);
	}
	else{
		upperValue=simContext->getPedsARTInputs(artLineNum)->responseTypeValuesEarly[SimContext::HET_OUTCOME_SUPP][1];
		lowerValue=simContext->getPedsARTInputs(artLineNum)->responseTypeValuesEarly[SimContext::HET_OUTCOME_SUPP][0];
		responseFactor=patient->artState.responseFactorCurrRegimen[SimContext::HET_OUTCOME_SUPP];
		patient->artState.probInitialEfficacy=lowerValue+responseFactor*(upperValue-lowerValue);

		upperValue=simContext->getPedsARTInputs(artLineNum)->responseTypeValuesEarly[SimContext::HET_OUTCOME_LATEFAIL][1];
		lowerValue=simContext->getPedsARTInputs(artLineNum)->responseTypeValuesEarly[SimContext::HET_OUTCOME_LATEFAIL][0];
		responseFactor=patient->artState.responseFactorCurrRegimen[SimContext::HET_OUTCOME_LATEFAIL];
		patient->artState.probLateFail=lowerValue+responseFactor*(upperValue-lowerValue);
	}

	if (patient->generalState.ageCategoryPediatrics == SimContext::PEDS_AGE_ADULT){
		patient->artState.probFillPrescriptionNonResp=simContext->getARTInputs(artLineNum)->probFillARTPrescriptionsNonResponder;
		patient->artState.probRestartAfterFail=simContext->getARTInputs(artLineNum)->probRestartARTRegimenAfterFailure[patient->artState.responseTypeCurrRegimen[SimContext::HET_OUTCOME_RESTART]];
	}
	else if (patient->generalState.ageCategoryPediatrics == SimContext::PEDS_AGE_LATE){
		patient->artState.probFillPrescriptionNonResp=simContext->getPedsARTInputs(artLineNum)->probFillARTPrescriptionsNonResponderLate;
		patient->artState.probRestartAfterFail=simContext->getPedsARTInputs(artLineNum)->probRestartARTRegimenAfterFailureLate[patient->artState.responseTypeCurrRegimen[SimContext::HET_OUTCOME_RESTART]];
	}
	else{
		patient->artState.probFillPrescriptionNonResp=simContext->getPedsARTInputs(artLineNum)->probFillARTPrescriptionsNonResponderEarly;
		patient->artState.probRestartAfterFail=simContext->getPedsARTInputs(artLineNum)->probRestartARTRegimenAfterFailureEarly[patient->artState.responseTypeCurrRegimen[SimContext::HET_OUTCOME_RESTART]];
	}

} /* end setCurrARTResponse */

/**
 *  Starts an adherence intervention for the patient
 */
void StateUpdater::startAdherenceIntervention(){
	patient->artState.monthOfAdherenceStart=patient->generalState.monthNum;
	patient->artState.isOnAdherenceIntervention=true;
}/* end startAdherenceIntervention */

/** \brief setTargetHVLStrata updates the target HVL while on ART or post ART
 *
 * \param targetHVL a SimContext::HVL_STRATA indicating the target HVL strata
 **/
void StateUpdater::setTargetHVLStrata(SimContext::HVL_STRATA targetHVL) {
	patient->diseaseState.targetHVLStrata = targetHVL;
} /* end setTargetHVLStrata */

/** \brief setPatientNatHistSlopePerc sets the Natural history cd4 decline Perc
 *
 * \param cd4Perc a double representing the CD4 Increment Percent
 **/
void StateUpdater::setPatientNatHistSlopePerc(double cd4Perc){
	patient->diseaseState.patientSpecificCD4DeclinePerc = cd4Perc;
	patient->diseaseState.hasDrawnPatientSpecificCD4Decline = true;
}/* end setPatientNatHistSlopePerc */

/** \brief setCurrRegimenCD4Slope sets the CD4 slope for the current ART regimen
 *
 * \param cd4slope a double representing the new CD4 slope
 **/
void StateUpdater::setCurrRegimenCD4Slope(double cd4Slope) {
	patient->artState.currRegimenCD4Slope = cd4Slope;
} /* end setCurrRegimenCD4Slope */

/** \brief setCurrRegimenCD4PercentageSlope sets the CD4 percentage slope for the current ART regimen
 *
 * This is used for the pediatric model
 *
 * \param cd4PercSlope a double representing the new CD4 percentage slope
 **/
void StateUpdater::setCurrRegimenCD4PercentageSlope(double cd4PercSlope) {
	patient->artState.currRegimenCD4PercentageSlope = cd4PercSlope;
} /* end setCurrRegimenCD4PercentageSlope */

/** \brief setCD4EnvelopeRegimen initializes the specified CD4 envelope type
 *
 * \param envelopeType a SimContext::ENVL_CD4_TYPE indicating whether this is a percentage envelope (pediatrics) or individual CD4 count (adult)
 * \param artLineNum an integer indicating which ART regimen this envelope is tied to
 **/
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

/** \brief setCD4EnvelopeSlope updates the slope used for the specified CD4 envelope type
 * \param envelopeType a SimContext::ENVL_CD4_TYPE indicating whether this is a percentage envelope (pediatrics) or individual CD4 count (adult)
 * \param cd4slope a double representing the CD4 slope (or CD4 percentage slope) of the envelope
 **/
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

/** \brief incrementCD4Envelope increments the specified CD4 envelope's level according to hypothetical ART success
 *
 * \param envelopeType a SimContext::ENVL_CD4_TYPE indicating whether this is a percentage envelope (pediatrics) or individual CD4 count (adult)
 * \param changeCD4 a double representing the amount to increase the envelope CD4 (or CD4 percentage) by
 * */
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

/** \brief setCurrARTRegimenFailure updates the state to begin the next ART treatment regimen
 *
 * ART statistics for observed failure are updated here.
 *
 * \param failType a SimContext::ART_FAIL_TYPE indicating the type of observed failure */
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

/** \brief stopCurrARTRegimen updates the state to begin the next ART treatment regimen
 *
 * ART stopping statistics are updated here
 *
 * \param stopType a SimContext::ART_STOP_TYPE indicating the reason the current ART regimen is being stopped */
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

	// Stop Adherence Intervention if on one
	if(patient->artState.isOnAdherenceIntervention){
		stopAdherenceIntervention();
	}
} /* stopCurrARTRegimen */

/** \brief stopAdherenceIntervention updates the state to end adherence intervention
 *
 * Adherence intervention stopping statistics are updated here
 *
**/
void StateUpdater::stopAdherenceIntervention() {
	patient->artState.isOnAdherenceIntervention=false;
} /* stopAdherenceIntervention */

/** \brief setNextARTRegimen updates the next ART regimen that is available for use
 *
 * \param hasNext a bool that is true if there is a "next ART regimen" available
 * \param artLineNum an integer indicating the ART regimen number of the next regimen -- this is only set if hasNext is true
 * */
void StateUpdater::setNextARTRegimen(bool hasNext, int artLineNum) {
	patient->artState.hasNextRegimenAvailable = hasNext;
	if (hasNext)
		patient->artState.nextRegimenNum = artLineNum;
} /* end setNextARTRegimen */

/** \brief incrementMonthsUnsuccessfulART increments the number of months on failed/partial ART stratified by HVL */
void StateUpdater::incrementMonthsUnsuccessfulART() {
	patient->artState.numMonthsOnUnsuccessfulByRegimen[patient->artState.currRegimenNum]++;
	SimContext::HVL_STRATA hvlStrata = patient->diseaseState.currTrueHVLStrata;
	patient->artState.numMonthsOnUnsuccessfulByHVL[hvlStrata]++;
} /* end incrementMonthsUnsuccessfulART */

/** \brief addARTToxicityEffect adds the occurrence of a new toxicity to the active effects list
 *
 * \param severity a SimContext::ART_TOX_SEVERITY indicating the severity of the new toxicity
 * \param toxNum an integer representing the index of the toxicity
 * \param timeToTox an integer represent the number of months until the toxicity takes effect
 **/
void StateUpdater::addARTToxicityEffect(SimContext::ART_TOX_SEVERITY severity, int toxNum, int timeToTox) {
	SimContext::ARTToxicityEffect toxicity;
	toxicity.toxSeverityType = severity;
	toxicity.toxNum = toxNum;
	toxicity.monthOfToxStart = patient->generalState.monthNum + timeToTox;
	toxicity.ARTRegimenNum = patient->artState.currRegimenNum;
	toxicity.ARTSubRegimenNum = patient->artState.currSubRegimenNum;
	patient->artState.activeToxicityEffects.push_back(toxicity);
} /* end addARTToxicityEffect */

/** \brief removeARTToxicityEffect removes the specified toxicity from the active effects list
 *
 * \param toxIter a pointer to an iterator pointing to the toxicity that should be removed
 *
 **/
void StateUpdater::removeARTToxicityEffect(list<SimContext::ARTToxicityEffect>::const_iterator &toxIter) {
	// Slightly hackish way to get a regular iterator from a const iterator, this is necessary
	//	since erase requires a regular iterator
	list<SimContext::ARTToxicityEffect>::const_iterator beginIter = patient->artState.activeToxicityEffects.begin();
	list<SimContext::ARTToxicityEffect>::iterator eraseIter = patient->artState.activeToxicityEffects.begin();
	advance(eraseIter, distance(beginIter, toxIter));

	// Erase the appropriate element from the toxicity list
	patient->artState.activeToxicityEffects.erase(eraseIter);
} /* end removeARTToxicityEffect */

/** \brief setARTToxicity updates the patient state and stats for the occurrence of a toxicity
 *
 * \param toxEffect a pointer to a SimConstant::ARTToxicityEffect indicating the toxicity effect to be set
 *
 * Statistics for ART toxicity are updated here
 **/
void StateUpdater::setARTToxicity(const SimContext::ARTToxicityEffect &toxEffect) {
	// Update the patient state as a severe toxicity if it causes a subregimen switch
	const SimContext::ARTInputs::ARTToxicity &toxInputs = simContext->getARTInputs(toxEffect.ARTRegimenNum)->toxicity[toxEffect.ARTSubRegimenNum][toxEffect.toxSeverityType][toxEffect.toxNum];
	bool isSevere = false;
	if (toxEffect.toxSeverityType == SimContext::ART_TOX_MAJOR) {
		patient->artState.hasMajorToxicity = true;
		if(patient->getGeneralState()->ageCategoryPediatrics>=SimContext::PEDS_AGE_LATE){
			if (simContext->getTreatmentInputs()->stopART[toxEffect.ARTRegimenNum].withMajorToxicty){
				isSevere = true;
			}
		}
		else{
			if (simContext->getPedsInputs()->stopART[toxEffect.ARTRegimenNum].withMajorToxicty){
				isSevere = true;
			}
		}
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

/** \brief incrementARTFailedCD4Tests increments the number of failed CD4 tests counting towards ART failure */
void StateUpdater::incrementARTFailedCD4Tests() {
	patient->artState.numFailedCD4Tests++;
} /* end incrementARTFailedCD4Tests */

/** \brief resetARTFailedCD4Tests sets the number of failed CD4 tests counting towards ART failure back to 0 */
void StateUpdater::resetARTFailedCD4Tests() {
	patient->artState.numFailedCD4Tests = 0;
} /* end resetARTFailedCD4Tests */

/** \brief incrementARTFailedHVLTests increments the number of failed CD4 tests counting towards ART failure */
void StateUpdater::incrementARTFailedHVLTests() {
	patient->artState.numFailedHVLTests++;
} /* end incrementARTFailedHVLTests */

/** \brief resetARTFailedHVLTests sets the number of failed HVL tests counting towards ART failure back to 0 */
void StateUpdater::resetARTFailedHVLTests() {
	patient->artState.numFailedHVLTests = 0;
} /* end resetARTFailedHVLTests */

/** \brief incrementARTFailedOIs increments the number of observed OIs counting towards ART failure */
void StateUpdater::incrementARTFailedOIs() {
	patient->artState.numFailedOIs++;
} /* end incrementARTFailedOIs */

/** \brief resetARTFailedOIs resets the number of observed OIs counting towards ART failure */
void StateUpdater::resetARTFailedOIs() {
	patient->artState.numFailedOIs = 0;
} /* end resetARTFailedOIs */

/** \brief setCurrSTIState updates the STI state for the current ART regimen
 * \param newSTIState a SimContext::STI_STATE to transition the patient to
 *
 * Statistics related to STI are updated here
 **/
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

/** \brief setProphNonCompliance sets whether or not the patient complies with proph
 *
 * \param isNonCompliant a boolean that is true if the patient is compliant
 **/
void StateUpdater::setProphNonCompliance(bool isNonCompliant) {
	patient->prophState.isNonCompliant = isNonCompliant;
} /* setProphNonCompliance */

/** \brief startNextProph updates state to beginning using next proph
 *
 * \param oiType a SimContext::OI_TYPE representing the OI which should have its prophylaxis updated
 *
 * It is assumed that a "next proph" exists for oiType
 **/
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

/** \brief stopCurrProph updates state to stop using current proph
 *
 * \param oiType a SimContext::OI_TYPE indicating which prophylaxis to stop
 **/
void StateUpdater::stopCurrProph(SimContext::OI_TYPE oiType) {
	patient->prophState.isOnProph[oiType] = false;
	patient->prophState.currTotalNumProphsOn--;
} /* end stopCurrProph */

/** \brief setNextProph updates whether primary or secondary proph should be used for the OI
 *
 * \param hasNext a boolean that is true if there is a "next prophylaxis" to set; if hasNext is false, no "next Prophylaxis" is set
 * \param prophType a SimContext::PROPH_TYPE indicating the type of prophylaxis the "next proph" should be
 * \param oiType a SimContext::OI_TYPE indicating which OI the "next proph" should be set for
 * \param prophNum an integer indicating the which prophylaxis should be next
 **/
void StateUpdater::setNextProph(bool hasNext, SimContext::PROPH_TYPE prophType, SimContext::OI_TYPE oiType, int prophNum) {


	patient->prophState.hasNextProphAvailable[oiType] = hasNext;

	if (hasNext) {
		patient->prophState.nextProphType[oiType] = prophType;
		patient->prophState.nextProphNum[oiType] = prophNum;

	}

} /* end setUserProphType */

/** \brief setProphToxicity records the toxicity and updates stats
 *
 * \param isMajor a boolean that is true if it a SimContext::PROPH_TOX_MAJOR (major toxicity)
 * \param oiType a SimContext::OI_TYPE indicating which prophylaxis has the toxicity
 *
 * Statistics for toxicity are updated here
 **/
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

/** \brief setProphResistance updates the flag to indicate that proph resistance has occurred
 *
 * \param oiType a SimContext::OI_TYPE indicating which prophylaxis resistance should be updated
 *
 * This only sets useProphResistance to true for the assigned prophylaxis
 **/
void StateUpdater::setProphResistance(SimContext::OI_TYPE oiType) {
	patient->prophState.useProphResistance[oiType] = true;
}

/** \brief setTBDiseaseState updates the TB disease state
 * \param newTBState a SimContext::TB_STATE marking the new TB state; if this is SimContext::TB_STATE_ACTIVE, "hasObservedHistoryActive" is set to true
 *
 * The month number of the change in state is also updated
 **/
void StateUpdater::setTBDiseaseState(SimContext::TB_STATE newTBState, SimContext::TB_HIST_ACTV_STATE newTBHistActiveState) {
	patient->tbState.currTrueTBDiseaseState = newTBState;
	patient->tbState.currHistOfActiveSubstate = newTBHistActiveState;
	patient->tbState.monthOfTBStateChange = patient->generalState.monthNum;
	if (newTBState == SimContext::TB_STATE_ACTIVE)
		patient->tbState.hasObservedHistoryActiveTB = true;
	if (newTBState == SimContext::TB_STATE_HIST_ACTV)
		assert(newTBHistActiveState != SimContext::TB_HIST_ACTV_NO_HIST_ACTV);
} /* end setTBDiseaseState */

/** \brief setTBResistanceStrain updates the TB disease drug resistance
 * \param newTBStrain a SimContext::TB_STRAIN that the patient will be set to be resistant to -- any previous resistance is overridden by this state
 **/
void StateUpdater::setTBResistanceStrain(SimContext::TB_STRAIN newTBStrain) {
	patient->tbState.currTrueTBResistanceStrain = newTBStrain;
} /* end setTBResistanceStrain */

/** \brief countInitialTBState counts the TB state/strain at entry and records it in the runStats
 *
 * This function only should be called after the initial state has been set
 */
void StateUpdater::countInitialTBState() {
	// Update statistics for the initial TB infection
	SimContext::TB_STRAIN tbStrain = patient->tbState.currTrueTBResistanceStrain;
	SimContext::TB_STATE tbState = patient->tbState.currTrueTBDiseaseState;
	//Update the "atEntry" stats
	runStats->tbStats.numInStateAtEntry[tbStrain][tbState]++;
}

/** \brief setNewTBInfection updates the state and statistics for a new TB infection occurring
 *
 * TB infection statistics are updated here
 *
 * \param infectType a SimContext::TB_INFECT indicating whether this is an incident or prevalent infection
 * \param isActive a boolean that is true if this is an active (i.e. not latent) infection
 * \param monthsSince an integer indicating the number of months since the patient was infected (only applicable for prevalent infections)
 **/
void StateUpdater::setNewTBInfection(SimContext::TB_INFECT infectType, bool isActive, int monthsSince) {
	// Update statistics for the new TB infection
	SimContext::TB_STRAIN tbStrain = patient->tbState.currTrueTBResistanceStrain;
	SimContext::TB_STATE tbState = patient->tbState.currTrueTBDiseaseState;

	// Update patient state for month of most recent infection
	patient->tbState.monthOfTBInfection = patient->generalState.monthNum;
	if (infectType == SimContext::TB_INFECT_PREVALENT) {
		patient->tbState.monthOfTBStateChange -= monthsSince;
		patient->tbState.monthOfTBInfection -= monthsSince;
	}

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

/** \brief setTBSpontaneousResolution increments number of TB spontaneous resolutions for the current resistance strain */
void StateUpdater::setTBSpontaneousResolution() {
	runStats->tbStats.numSpontaneousResolutions[patient->tbState.currTrueTBResistanceStrain]++;
} /* end setTBSpontaneousResolution */

/** \brief startNextTBProph updates state to beginning using next TB proph
 *
 * This assumes there is a next TB proph already assigned
 **/
void StateUpdater::startNextTBProph() {
	patient->tbState.isOnProph = true;
	patient->tbState.hadProph = true;
	patient->tbState.currProphNum = patient->tbState.nextProphNum;
	patient->tbState.monthOfProphStart = patient->generalState.monthNum;
	patient->tbState.isScheduledForProph = false;
	patient->tbState.hasMajorProphToxicity = false;
} /* end startNextTBProph */

/** \brief stopCurrTBProph updates state to stop using current TB proph */
void StateUpdater::stopCurrTBProph() {
	patient->tbState.isOnProph = false;
} /* end stopCurrTBProph */

/** \brief setNextTBProph updates proph num to be used next for TB
 *
 * \param hasNext a boolean that is true if there is a "next prophylaxis" to set; if hasNext is false, no "next Prophylaxis" is set
 * \param prophNum an integer indicating the which prophylaxis should be next
 **/
void StateUpdater::setNextTBProph(bool hasNext, int prophNum) {
	patient->tbState.hasNextProphAvailable = hasNext;
	if (hasNext)
		patient->tbState.nextProphNum = prophNum;
} /* end setNextTBProph */

/** \brief scheduleNextTBProph updates the time lag for scheduling the next TB proph
 * \param monthStart an integer representing the month that the next TB proph should start
 **/
void StateUpdater::scheduleNextTBProph(int monthStart) {
	patient->tbState.isScheduledForProph = true;
	patient->tbState.monthOfProphStart = monthStart;
} /* end scheduleNextTBProph */

/** \brief unscheduleNextTBProph removes the next scheduled start of TB proph */
void StateUpdater::unscheduleNextTBProph() {
	patient->tbState.isScheduledForProph = false;
	patient->tbState.monthOfProphStart = SimContext::NOT_APPL;
} /* end unscheduleNextTBProph */

/** \brief setTBProphToxicity records the TB proph toxicity
 *
 * \param isMajor a boolean that is true if the new toxicity to be record is a major toxicity
 **/
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

/** \brief startNextTBTreatment updates the patient state and statistics to begin the next TB treatment */
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

/** \brief stopCurrTBTreatment updates the patient state and statistics to stop the current TB treatment
 *
 * \param isFinished a boolean that is false if the patient dropped out of treatment before it finished
 * \param isCured a boolean that is true if the patient was cured while on treatment
 **/
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

/** \brief scheduleNextTBTreatment updates the patients next scheduled TB treatment and lag time to start
 *
 * \param treatStage a SimContext::TB_TREATM_STAGE indicating the next treatment stage the patient should go on
 * \param monthStart an integer indicating which month the next treatment stage (treatStage) should start
 **/
void StateUpdater::scheduleNextTBTreatment(SimContext::TB_TREATM_STAGE treatStage, int monthStart) {
	patient->tbState.isScheduledForTreatment = true;
	patient->tbState.nextTreatmentStage = treatStage;
	patient->tbState.monthOfTreatmentStart = monthStart;
} /* end scheduleNextTBTreatment */

/** \brief unscheduleNextTBTreatment removes the next scheduled start of TB treatment */
void StateUpdater::unscheduleNextTBTreatment() {
	patient->tbState.isScheduledForTreatment = false;
} /* end unscheduleNextTBTreatmet */

/** \brief increaseTBDrugResistance updates that patient state and stats for an increase in TB drug resistance
 *
 * \param fromTreatment a boolean that is true if the increased resistance is due to failed treatment
 **/
void StateUpdater::increaseTBDrugResistance(bool fromTreatment) {
	// Update the patient state for the increased drug resistance
	SimContext::TB_TREATM_STAGE treatStage = patient->tbState.currTreatmentStage;
	SimContext::TB_STRAIN tbStrain = patient->tbState.currTrueTBResistanceStrain;
	patient->tbState.currTrueTBResistanceStrain = (SimContext::TB_STRAIN) (tbStrain + 1);

	// Update the statistics for the increased drug resistance
	if (fromTreatment)
		runStats->tbStats.numIncreaseResistanceAtTreatmentFinish[tbStrain][treatStage]++;
} /* end increaseTBDrugResistance */

/** \brief setTBTreatmentToxicity records the TB proph toxicity
 *
 * \param isMajor a boolean that is true if this is a major toxicity
 *
 * Statistics related to TB treatment toxicity are updated here
 **/
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

/** \brief setTrueCHRMsState updates the patients state for occurrence of CHRMs diseases
 *
 * \param chrmNum is an integer representing which CHRM is being set
 * \param hasCHRM a boolean that is true if the patient has the CHRM chrmNum
 * \param isInitial a boolean that is true if this is the state being set at time 0
 * \param monthsStart an integer representing the number of months prior to the current month that the CHRM state started (this is only in effect if isInitial is true)
 **/
void StateUpdater::setTrueCHRMsState(int chrmNum, bool hasCHRM, bool isInitial, int monthsStart) {
	if(hasCHRM && (isInitial || !patient->diseaseState.hasTrueCHRMs[chrmNum])){
		runStats->chrmsStats.numPatientsWithCHRM[chrmNum]++;
	}
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

	// Update the longitudinal statistics for the occurrence of the CHRM
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		if (hasCHRM) {
			SimContext::CD4_STRATA cd4Strata = patient->diseaseState.currTrueCD4Strata;
			if (!isInitial)
				currTime->numIncidentCHRMs[chrmNum]++;
		}
	}
}

/** \brief setInitialOIHistory updates the patient state for initial OI history
 *
 * \param hasHistory[SimContext::OI_NUM] an array of booleans indicating which OI the patient has an initial history of
 **/
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

/** \brief setOIHistory updates the patient state for OI history
 *
 * OI history is set based on whether or not an OI occurred this month.  This is called at the
 * end of the month since current acute OI should not count as history until the next month */
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

/** \brief setCurrTrueOI updates the patient state when an acute OI event occurs or resets back to none
 *
 * \param oiType a SimContext::OI_TYPE indicating which OI occured
 *
 * RunStats related to acute OIs are updated here
 **/
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

/** \brief clearMortalityRisks clears the list of possible death risks for the month */
void StateUpdater::clearMortalityRisks() {
	patient->diseaseState.mortalityRisks.clear();
} /* end clearMortalityRisks */

/** \brief addMortalityRisk adds a new risk of death for the month
 *
 * \param causeOfDeath a SimContext::DTH_CAUSES indicating the new risk of death
 * \param probDeath a double representing the probability of death due to the new cause
 * \param costDeath a double representing the cost accrued by this type of death
 **/
void StateUpdater::addMortalityRisk(SimContext::DTH_CAUSES causeOfDeath, double probDeath, double costDeath) {
	SimContext::MortalityRisk deathRisk;
	deathRisk.causeOfDeath = causeOfDeath;
	deathRisk.probDeath = probDeath;
	deathRisk.costDeath = costDeath;
	patient->diseaseState.mortalityRisks.push_back(deathRisk);
} /* end addMortalityRisk */

/** \brief setCauseOfDeath updates the patient state to reflect that death has occurred
 *
 * \param causeOfDeath a SimContext::DTH_CAUSES representing the cause of death
 *
 * RunStats statistics relating to the patient's state at death are updated here
 **/
void StateUpdater::setCauseOfDeath(SimContext::DTH_CAUSES causeOfDeath) {
	// Update the patient state to reflect that death has occurred
	patient->diseaseState.isAlive = false;
	patient->diseaseState.causeOfDeath = causeOfDeath;

	runStats->deathStats.numDeathsCareType[patient->monitoringState.careState][patient->diseaseState.causeOfDeath]++;
	// update the run statistics to reflect that death has occurred
	if (patient->diseaseState.infectedHIVState == SimContext::HIV_INF_NEG) {
		runStats->deathStats.numDeathsUninfected++;
	}
	else {
		runStats->deathStats.numDeathsCD4Type[patient->diseaseState.currTrueCD4Strata][patient->diseaseState.causeOfDeath]++;
		runStats->deathStats.numDeathsHVLCD4[patient->diseaseState.currTrueHVLStrata][patient->diseaseState.currTrueCD4Strata]++;
		bool hasCHRM=false;
		for(int i=0;i<SimContext::CHRM_NUM;i++){
			if(patient->diseaseState.hasTrueCHRMs[i]){
				runStats->deathStats.numDeathsWithCHRMsTypeCHRM[patient->diseaseState.causeOfDeath][i]++;
				hasCHRM=true;
			}
		}
		if(!hasCHRM){
			runStats->deathStats.numDeathsWithoutCHRMsType[patient->diseaseState.causeOfDeath]++;
		}

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
			SimContext::CD4_STRATA cd4Strata=patient->diseaseState.currTrueCD4Strata;

			runStats->deathStats.numToxDeaths++;
			runStats->deathStats.ToxDeathsCD4Sum+=patient->diseaseState.currTrueCD4;
			runStats->deathStats.ToxDeathsCD4SumSquares+=patient->diseaseState.currTrueCD4*patient->diseaseState.currTrueCD4;

			runStats->artStats.numToxicityDeaths[currRegimen][hvlStrata]++;
			runStats->deathStats.numToxDeathsCD4HVL[cd4Strata][hvlStrata]++;
			runStats->deathStats.numToxDeathsCD4[cd4Strata]++;

			for (int i=0;i<SimContext::OI_NUM;i++){
				if(patient->diseaseState.hasTrueOIHistory[i]){
					runStats->deathStats.numToxDeathsCD4HVLOIHist[cd4Strata][hvlStrata][i]++;
				}
			}
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
		currTime->numDeathsTypeCare[patient->diseaseState.causeOfDeath][patient->monitoringState.careState]++;
		SimContext::HIV_ID detectedState = patient->monitoringState.isDetectedHIVPositive ? SimContext::HIV_ID_IDEN : SimContext::HIV_ID_UNID;
		if (patient->diseaseState.infectedHIVState == SimContext::HIV_INF_NEG)
			detectedState = SimContext::HIV_ID_NEG;
		currTime->numDeathsInf[detectedState]++;
		currTime->numDeathsCare[patient->monitoringState.careState]++;

		if (patient->diseaseState.infectedHIVState == SimContext::HIV_INF_NEG){
			currTime->numDeathsUninfected++;
		}
		else{
			if(patient->monitoringState.isDetectedHIVPositive){
				if (patient->monitoringState.hadPrevClinicVisit && patient->monitoringState.currLTFUState != SimContext::LTFU_STATE_LOST){
					currTime->numDeathsDetectedLinked++;
				}
				else if (!patient->monitoringState.hadPrevClinicVisit){
					currTime->numDeathsDetectedNeverLinked++;
				}
			}
			else{
				currTime->numDeathsUndetectedInfected++;
			}
		}

		bool hasCHRM=false;
		for(int i=0;i<SimContext::CHRM_NUM;i++){
			if(patient->diseaseState.hasTrueCHRMs[i]){
				currTime->numDeathsWithCHRMsTypeCHRM[patient->diseaseState.causeOfDeath][i]++;
				currTime->numDeathsWithCHRMsCHRM[i]++;
				hasCHRM=true;
			}
		}
		if (!hasCHRM){
			currTime->numDeathsWithoutCHRMsType[patient->diseaseState.causeOfDeath]++;
			currTime->numDeathsWithoutCHRMs++;
		}

		if (patient->monitoringState.currLTFUState == SimContext::LTFU_STATE_LOST) {
			if (patient->monitoringState.isDetectedHIVPositive){
				currTime->numDeathsDetectedLTFU++;
			}
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

/** \brief setMaternalDeath updates the patient state for pediatric maternal death
 *
 *	Breast feeding is also stopped here: If mom is dead, the infant can't be breastfeeding anymore!
 **/
void StateUpdater::setMaternalDeath() {
	patient->generalState.isMotherAlive = false;
	//If mom is dead, the infant can't be breastfeeding anymore!
	patient->generalState.breastfeedingStatus = SimContext::PEDS_BF_REPL;
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

/** \brief updatePopulationStats updates the final population summary after a death occurs
 *
 * All of the patient's running lifetime totals (such as cost and life months) are added to the population totals here
 **/
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
		if(!patient->monitoringState.isLinked){
			runStats->hivScreening.numLinkedMeans[SimContext::HIV_DET_UNDETECTED]++;
		}
	}
} /* end updatePopulationStats */

/** \brief AddPatientSummary creates a patient summary object and adds it to the patients vector
 *
 * Only HIV positive patients up to 1,000,000 are added
 **/
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

/** \brief setTrueCD4 updates the patients actual CD4 level and confines it within the bounds of the envelope
 *
 * Also checks if it is the patients minimum CD4 value
 **/
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

/** \brief setTrueCD4Percentage updates the pediatrics patients actual CD4 percentage
 * \param newCD4Perc a double (which should be between 0 and 1 inclusive) that represents the new CD4 percentage
 * \param isInitial a boolean that is true if this is the first time the CD4 percentage is being set
 *
 * This function also checks if newCD4Perc is the patient's minimum CD4 percentage
 **/
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

/** \brief setCD4MultOnARTFail updates the cd4 decline mutiplier on failed ART
 * \param monthOfNewCD4Decline an int that represents the next month in which a new lag period can be set
 * \param newCD4Mult a double setting the current cd4 decline multiplier on failed ART
 *
 **/
void StateUpdater::setCD4MultOnARTFail(int monthOfNewCD4Decline,double newCD4Mult){
	patient->artState.monthOfNewCD4MultArtFail=monthOfNewCD4Decline;
	patient->artState.currCD4MultArtFail=newCD4Mult;
}

/** \brief setTrueHVLStrata set the patient's actual HVL strata to the given level
 *
 * \param newHVL a SimContext::HVL_STRATA indicating the new HVL strata to be set
 **/
void StateUpdater::setTrueHVLStrata(SimContext::HVL_STRATA newHVL) {
	// update the HVL strata
	patient->diseaseState.currTrueHVLStrata = newHVL;
} /* end setTrueHVLStrata */

/** \brief setSetpointHVLStrata sets the patients setpoint HVL level
 *
 * \param newSetpoint a SimContext::HVL_STRATA indicating the new HVL setpoint
 **/
void StateUpdater::setSetpointHVLStrata(SimContext::HVL_STRATA newSetpoint) {
	// update the HVL setpoint strata
	patient->diseaseState.setpointHVLStrata = newSetpoint;
} /* end setSetpointHVLStrata */

/** \brief setObservedCD4 updates the patients observed CD4 level and confines it within the bounds
 *
 * \param isKnown a boolean that is true if there is an observed CD4 value; if isKnown is false, the patient has no observed CD4
 * \param cd4Value a double representing the new observed cd4Value (this is only in effect if isKnown is true)
 *
 * The function also checks if cd4Value is the new minimum or maximum observed cd4 value
 **/
void StateUpdater::setObservedCD4(bool isKnown, double cd4Value, bool isLabStaging) {
	// If it is unknown (for untested patients), set the CD4 to unknown and return
	if (!isKnown) {
		patient->monitoringState.hasObservedCD4 = false;
		patient->monitoringState.hasObservedCD4NonLabStaging = false;
		return;
	}

	// make sure observed CD4 is not below zero
	patient->monitoringState.monthOfObservedCD4 = patient->generalState.monthNum;
	if(!isLabStaging)
		patient->monitoringState.monthOfObservedCD4NonLabStaging = patient->generalState.monthNum;
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
	if (!isLabStaging)
		patient->monitoringState.hasObservedCD4NonLabStaging = true;
} /* end setObservedCD4 */

/** \brief setObservedCD4Percentage updates the patients observed CD4 percentage and confines it within the bounds
 *
 * \param isKnown a boolean that is true if there is an observed CD4 percentage; if isKnown is false, the patient has no observed CD4 percentage
 * \param cd4Percent a double (that should be between 0 and 1 inclusive) representing the new observed cd4 percentage (this is only in effect if isKnown is true)
 *
 * The function also checks if cd4Percent is the new minimum or maximum observed cd4 percentage
 **/
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

/** \brief setObservedHVLStrata updates the patients observed HVL strata
 *
 * \param isKnown a boolean that is true if there is an observed HVL strata; if isKnown is false, the patient has no observed HVL
 * \param hvlStrata a SimContext::HVL_STRATA representing the new observed hvlStrata (this is only in effect if isKnown is true)
 *
 * The function also checks if hvlStrata is the new minimum or maximum observed hvl
 **/
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

/** \brief incrementCostsHIVTest adds an HIV testing cost to the patients total
 *
 * \param cost a double representing the cost to be incremented
 *
 * The costs will be discounted
 *
 * \see StateUpdater::incrementCostsCommon(double, double)
 **/
void StateUpdater::incrementCostsHIVTest(double cost) {
	runStats->overallCosts.costsHIVScreeningTests += cost * patient->generalState.discountFactor;
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		currTime->costsHIVTests += cost * patient->generalState.discountFactor;
	}
	incrementCostsCommon(cost, 1.0);
} /* end incrementCostsHIVTest */

/** \brief incrementCostsHIVMisc adds an HIV misc related cost to the patients total
 *
 * \param cost a double representing the cost to be incremented
 *
 * The costs will be discounted
 *
 * \see StateUpdater::incrementCostsCommon(double, double)
 **/
void StateUpdater::incrementCostsHIVMisc(double cost) {
	runStats->overallCosts.costsHIVScreeningMisc += cost * patient->generalState.discountFactor;
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		currTime->costsHIVMisc += cost * patient->generalState.discountFactor;
	}
	incrementCostsCommon(cost, 1.0);
} /* end incrementCostsHIVTest */

/** \brief incrementCostsLabStagingTest adds a Lab Staging testing cost to the patients total
 *
 * \param cost a double representing the cost to be incremented
 *
 * The costs will be discounted
 *
 * \see StateUpdater::incrementCostsCommon(double, double)
 **/
void StateUpdater::incrementCostsLabStagingTest(double cost) {
	runStats->overallCosts.costsLabStagingTests += cost * patient->generalState.discountFactor;
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		currTime->costsLabStagingTests += cost * patient->generalState.discountFactor;
	}
	incrementCostsCommon(cost, 1.0);
} /* end incrementCostsLabStagingTest */

/** \brief incrementCostsLabStagingMisc adds a Lab Staging misc related cost to the patients total
 *
 * \param cost a double representing the cost to be incremented
 *
 * The costs will be discounted
 *
 * \see StateUpdater::incrementCostsCommon(double, double)
 **/
void StateUpdater::incrementCostsLabStagingMisc(double cost) {
	runStats->overallCosts.costsLabStagingMisc += cost * patient->generalState.discountFactor;
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		currTime->costsLabStagingMisc += cost * patient->generalState.discountFactor;
	}
	incrementCostsCommon(cost, 1.0);
} /* end incrementCostsLabStagingTest */

/** \brief incrementCostsCD4Test adds the CD4 testing related costs to the patients total
 *
 * \param costArray a pointer to an array of doubles of size SimContext::COST_NUM_TYPES representing the costs to be added
 *
 * The costs will be discounted
 *
 * \see StateUpdater::incrementCostsCommon(const double*, double)
 **/
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

/* \brief incrementCostsHVLTest adds the HVL testing related costs to the patients total
 *
 * \param costArray a pointer to an array of doubles of size SimContext::COST_NUM_TYPES representing the costs to be added
 *
 * The costs will be discounted
 *
 * \see StateUpdater::incrementCostsCommon(const double*, double)
 **/
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

/** \brief incrementCostsClinicVisit adds the general clinic visit costs to the patients total
 *
 * \param costArray a pointer to an array of doubles of size SimContext::COST_NUM_TYPES representing the costs to be added
 *
 * The costs will be discounted.
 *
 * \see StateUpdater::incrementCostsCommon(const double*, double)
 */
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

/** \brief incrementCostsART adds an ART treatment cost to the patients total
 *
 * \param artLineNum an integer representing the line of ART that the cost stems from
 * \param cost a double representing the cost to be added
 *
 * The cost will be discounted
 *
 * \see StateUpdater::incrementCostsCommon(double, double)
 **/
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


/** \brief incrementCostsIntervention adds an Intervention cost to the patients total
 *
 * \param cost a double representing the cost to be added
 *
 * The cost will be discounted
 *
 * \see StateUpdater::incrementCostsCommon(double, double)
 **/
void StateUpdater::incrementCostsIntervention(double cost) {
	double discountedCost = cost * patient->getGeneralState()->discountFactor;
	runStats->overallCosts.costsInterventions += cost;
	incrementCostsCommon(cost, 1.0);
} /* end incrementCostsART */

/** \brief incrementCostsProph adds a prophylaxis treatment cost to the patients total
 *
 * \param oiType a SimContext::OI_TYPE indicating the OI prophylaxis regimen that caused the cost
 * \param prophNum an integer representing the index of which specific prophylaxis caused the cost
 * \param cost a double representing the cost to be added
 *
 * The cost will be discounted
 *
 * \see StateUpdater::incrementCostsCommon(double, double)
 **/
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

/** \brief incrementCostsTBProph adds a TB proph cost to patients total
 *
 * \param prophNum an integer representing which prophylaxis regimen caused the cost to be accrued
 * \param cost a double representing the cost to be added
 *
 * The cost will be discounted
 *
 * \see StateUpdater::incrementCostsCommon(double, double)
 **/
void StateUpdater::incrementCostsTBProph(int prophNum, double cost) {
	runStats->overallCosts.costsDrugs += cost * patient->getGeneralState()->discountFactor;
	incrementCostsCommon(cost, 1.0);
} /* end incrementCostsTBProph */

/** \brief incrementCostsTBTreatment adds a TB proph cost to patients total
 *
 * \param costArray a pointer to an array of doubles of size SimContext::COST_NUM_TYPES representing the costs to be added
 * \param percent a double representing the additional amount the cost should be reduced (multiplicative)
 *
 * The costs will be discounted
 *
 * \see StateUpdater::incrementCostsCommon(const double*, double) */
void StateUpdater::incrementCostsTBTreatment(const double *costArray, double percent) {
	double discountedCostTotal = 0.0;
	for (int i = 0; i < SimContext::COST_NUM_TYPES; i++) {
		discountedCostTotal += costArray[i];
	}
	discountedCostTotal *= patient->generalState.discountFactor * percent;
	runStats->overallCosts.costsDrugs += discountedCostTotal;
	incrementCostsCommon(costArray, percent);
} /* end incrementCostsTBTreatment */

/** \brief incrementCostsToxicity adds a toxicity cost to the patients total
 *
 * \param cost a double representing the cost to be added
 *
 * This cost is NOT discounted
 *
 * \see StateUpdater::incrementCostsCommon(double, double)
 **/
void StateUpdater::incrementCostsToxicity(double cost) {
	runStats->overallCosts.costsToxicity += cost;
	incrementCostsCommon(cost, 1.0);
} /* end incrementCostsToxicity */


/** \brief incrementCostsCHRMs adds a CHRMs cost to the patients total
 *
 * \param cost a double representing the cost to be added
 *
 * This cost is discounted
 *
 * \see StateUpdater::incrementCostsCommon(double, double)
 **/
void StateUpdater::incrementCostsCHRMs(SimContext::CHRM_TYPE CHRMType,double cost) {
	runStats->overallCosts.costsCHRMs[CHRMType] += cost*patient->generalState.discountFactor;
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		currTime->costsCHRMs[CHRMType]+= cost*patient->generalState.discountFactor;
	}
	incrementCostsCommon(cost, 1.0);
} /* end incrementCostsCHRMs */


/** \brief incrementCostsPeds adds a pediatric cost to the patient's total
 *
 * \param cost a double representing the cost to be added
 *
 * This cost is NOT discounted
 *
 * \see StateUpdater::incrementCostsCommon(double, double)
 **/
void StateUpdater::incrementCostsPeds(double cost) {
	runStats->overallCosts.costsPeds += cost;
	RunStats::TimeSummary *currTime = getTimeSummaryForUpdate();
	if (currTime) {
		currTime->costsPeds += cost;
	}
	incrementCostsCommon(cost, 1.0);
}
/* end incrementCostsPeds */

/** \brief incrementCostsMisc adds a miscellaneous cost to the patients total costs
 *
 *	overloaded to take in either a single cost value or a COST_NUM_TYPES sized array of costs
 *	\param cost a double representing the cost to be added
 *	\param percent a double representing the percent of the cost to apply
 *
 *	This cost is NOT discounted.
 *
 *	\see StateUpdater::incrementCostsCommon(double, double)
 */
void StateUpdater::incrementCostsMisc(double cost, double percent) {
	incrementCostsCommon(cost, percent);
} /* end incrementCostsMisc */

/** \brief incrementCostsMisc adds a miscellaneous cost to the patients total costs
 *
 *	overloaded to take in either a single cost value or a COST_NUM_TYPES sized array of costs
 *	\param costArray a pointer to an array of doubles of size SimContext::COST_NUM_TYPES representing the costs to be added
 *	\param percent a double representing the percent of the cost to apply
 *
 *	This cost is NOT discounted.
 *
 *	\see StateUpdater::incrementCostsCommon(const double*, double)
 */
void StateUpdater::incrementCostsMisc(const double *costArray, double percent, double multiplier) {
	incrementCostsCommon(costArray, percent, multiplier);
} /* end incrementCostsMisc */

/** \brief updateInitialDistributions updates the initial statistics for patients upon infection */
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

/** \brief updatePatientSurvival updates the patient state for discounted LMs and QALMs for the individual patient,
 * used with a half month length if death occurred that month
 *
 * \param percentOfMonth a double representing the percent of the month the patient was alive: if the patient died this month, this value should be 0.5.  If the patient did not die, this value should be 1.0
 **/
void StateUpdater::updatePatientSurvival(double percentOfMonth) {
	patient->generalState.LMsDiscounted += percentOfMonth * patient->generalState.discountFactor;
	patient->generalState.qualityAdjustLMsDiscounted +=
		percentOfMonth * patient->generalState.discountFactor * patient->generalState.QOLMultiplier;
} /* end updatePatientSurvival */

/** \brief updateOverallSurvival updates the statistics for stratified LMs and QALMs for the whole population,
 *	used with a half month length if death occurred that month
 *
 *	\param percentOfMonth a double representing the percent of the month the patient was alive: if the patient died this month, this value should be 0.5.  If the patient did not die, this value should be 1.0
 **/
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
	for(int i=0;i<SimContext::CHRM_NUM;i++){
		if(patient->diseaseState.hasTrueCHRMs[i]){
			runStats->overallSurvival.LMsCHRMHistoryCHRMs[i]+=
					percentOfMonth*patient->generalState.discountFactor;
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

/** \brief updateLongitSurvival updates the longitudinal statistics related to survival
 *
 * If no longitudinal logging, nothing happens.  If yearly logging was specified and the current month is not the end of the year, nothing is logged.
 **/
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
		currTime->numAliveCare[patient->monitoringState.careState]++;
		currTime->numAliveTypeCare[detectedState][patient->monitoringState.careState]++;
		bool hasCHRMs=false;
		for(int i=0;i<SimContext::CHRM_NUM;i++){
			if(patient->diseaseState.hasTrueCHRMs[i]){
				currTime->numAliveTypeCHRMs[detectedState][i]++;
				currTime->numCHRMsAge[i][patient->generalState.ageCategoryCHRMs]++;
				currTime->numCHRMsGender[i][patient->generalState.gender]++;
				currTime->numCHRMsCD4[i][patient->diseaseState.currTrueCD4Strata]++;
				hasCHRMs=true;
			}
		}
		if(hasCHRMs){

			currTime->numCHRMsAgeTotal[patient->generalState.ageCategoryCHRMs]++;
			currTime->numCHRMsGenderTotal[patient->generalState.gender]++;
			currTime->numCHRMsCD4Total[patient->diseaseState.currTrueCD4Strata]++;

			currTime->numAliveWithCHRMsType[detectedState]++;
		}
		else{
			currTime->numAliveWithoutCHRMsType[detectedState]++;
		}

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
		currTime->trueCD4SumCare[patient->monitoringState.careState] += patient->diseaseState.currTrueCD4;
		currTime->trueCD4SumSquaresCare[patient->monitoringState.careState] += patient->diseaseState.currTrueCD4 * patient->diseaseState.currTrueCD4;
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
			currTime->observedCD4SumCare[patient->monitoringState.careState] += patient->monitoringState.currObservedCD4;
			currTime->observedCD4SumSquaresCare[patient->monitoringState.careState] += patient->monitoringState.currObservedCD4 * patient->monitoringState.currObservedCD4;
			currTime->observedCD4Distribution[patient->monitoringState.currObservedCD4Strata]++;
			currTime->observedCD4DistributionCare[patient->monitoringState.currObservedCD4Strata][patient->monitoringState.careState]++;
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
			if (patient->artState.monthOfCurrRegimenStart == patient->generalState.monthNum)
				currTime->numStartingART[patient->artState.currRegimenNum]++;
			currTime->numOnARTIncludingContinuedCosts[patient->artState.currRegimenNum]++;
			currTime->numARTEfficacyState[patient->artState.currRegimenEfficacy]++;
		}
		else {
			SimContext::HIV_CARE careState = patient->monitoringState.careState;
			if (careState == SimContext::HIV_CARE_IN_CARE || careState == SimContext::HIV_CARE_RTC){
				if(!patient->artState.hasTakenART){
					currTime->numInCarePreART++;
					if (patient->monitoringState.monthOfLinkage == patient->generalState.monthNum)
						currTime->numStartingPreART++;
				}
				else{
					currTime->numInCarePostART++;
					if (patient->artState.monthOfPrevRegimenStop == patient->generalState.monthNum)
						currTime->numStartingPostART++;
				}
			}
			currTime->trueCD4HVLARTDistribution[SimContext::ART_OFF_STATE][patient->diseaseState.currTrueCD4Strata][patient->diseaseState.currTrueHVLStrata]++;
		}
		if (patient->monitoringState.careState == SimContext::HIV_CARE_LTFU){
			if(patient->monitoringState.wasOnARTWhenLostToFollowUp){
				currTime->numLostToFollowUpART[patient->artState.prevRegimenNum]++;
			}
			else if(!patient->artState.hasTakenART){
				currTime->numLostToFollowUpPreART++;
			}
			else{
				currTime->numLostToFollowUpPostART++;
			}
		}
	}
} /* end updateLongitSurvival */

/** \brief updateOIHistoryLogging updates the statistics for patient OI history logging */
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

/** \brief updateARTEfficacyStats updates the totals for months in ART suppression and HVL drops */
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

/** \brief willAttendClinicThisMonth returns true if patient will go in for a scheduled clinic visit or OI emergency clinic visit this month
 *
 * \return true if the patient should have a clinic visit this month, false otherwise
 **/
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

/** \brief getPartialSuppressTargetHVL determines the ART target HVL when entering partial suppression
 *
 * \param artLineNum an integer indicating the current ART line number
 *
 * ISSUE: Should the new target HVL level be able to go lower if the setpoint is LO or MLO?
 *
 * \return the SimContext::HVL_STRATA corresponding to the partial suppression target HVL for the given ART line
 **/
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

/** \brief getCD4Strata returns the CD4 strata for a given value
 *
 * \param valueCD4 a double indicating the CD4 value that should have its strata returned
 *
 * \return a SimContext::CD4_STRATA indicating the CD4 strata corresponding to the exact CD4 value
 **/
SimContext::CD4_STRATA StateUpdater::getCD4Strata(double valueCD4) {
	for (int i = 0; i < SimContext::CD4_NUM_STRATA - 1; i++) {
		if (valueCD4 < simContext->getRunSpecsInputs()->CD4StrataUpperBounds[i]) {
			return ((SimContext::CD4_STRATA) i);
		}
	}
	return SimContext::CD4_VHI;
}

/** \brief getCD4PercentageStrata returns the CD4 percentage strata for a given value
 *
 * \param percCD4 a double (which should be between 0 and 1 inclusive) that indicates the CD4 percentage that should have its strata returned
 *
 * \return a SimContext::PEDS_CD4_PERC indicating the CD4 percentage strata corresponding to the exact CD4 percentage
 **/
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

/** \brief getAgeCategoryClinical returns the clinic visit age category for the given age
 *
 * \param ageMonths an integer representing the exact age of which to return the age category
 *
 * \return an integer representing the index of the clinical age categories
 **/
int StateUpdater::getAgeCategoryClinical(int ageMonths) {
	int ageYears = ageMonths / 12;
	int ageCategory = ageYears / 5;
	if (ageCategory < SimContext::AGE_CATEGORIES)
		return ageCategory;
	return SimContext::AGE_CATEGORIES - 1;
}

/** \brief getAgeCategoryHIVInfection returns the HIV testing age category for the given age
 *
 * \param ageMonths an integer representing the exact age of which to return the age category
 *
 * \return an integer representing the index of the HIV infection age categories
 **/
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

/** \brief getAgeCategoryCHRMs returns the CHRMs age category for the given age
 *
 * \param ageMonths an integer representing the exact age of which to return the age category
 *
 * \return an integer representing the index of the CHRMs age categories
 **/
int StateUpdater::getAgeCategoryCHRMs(int ageMonths) {
	int ageYears = ageMonths / 12;
	if (ageYears < 20)
		return 0;
	if (ageYears <= 29)
		return 1;
	if (ageYears <= 39)
		return 2;
	if (ageYears <= 49)
		return 3;
	if (ageYears <= 59)
		return 4;
	if (ageYears <= 69)
		return 5;
	return 6;
}

/** \brief getAgeCategoryPediatrics returns the Pediatrics testing category for the given age
 *
 * \param ageMonths an integer representing the exact age of which to return the age category
 *
 * \return an integer representing the index of the pediatric age categories
 **/
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

/** \brief getAgeCategoryPediatricsCost returns the Pediatrics Cost category for the given age
 *
 * \param ageMonths an integer representing the exact age of which to return the age category
 *
 * \return an integer representing the index of the pediatric cost age categories
 **/
SimContext::PEDS_COST_AGE StateUpdater::getAgeCategoryPediatricsCost(int ageMonths) {
	if(!simContext->getPedsInputs()->enablePediatricsModel)
		return SimContext::PEDS_COST_AGE_ADULT;

	if (ageMonths < 24)
		return SimContext::PEDS_COST_AGE_1;
	if (ageMonths < 60)
		return SimContext::PEDS_COST_AGE_2;
	if (ageMonths < 156)
		return SimContext::PEDS_COST_AGE_3;
	if(ageMonths<216)
		return SimContext::PEDS_COST_AGE_4;
	else
		return SimContext::PEDS_COST_AGE_ADULT;
}

/** \brief getAgeCategoryPediatricsARTCost returns the Pediatrics ART Cost category for the given age
 *
 * \param ageMonths an integer representing the exact age of which to return the age category
 *
 * \return an integer representing the index of the pediatric cost ART age categories
 **/
SimContext::PEDS_ART_COST_AGE StateUpdater::getAgeCategoryPediatricsARTCost(int ageMonths) {
	if(!simContext->getPedsInputs()->enablePediatricsModel)
		return SimContext::PEDS_ART_COST_AGE_ADULT;

	if (ageMonths < 6)
		return SimContext::PEDS_ART_COST_AGE_5MTH;
	if (ageMonths < 12)
		return SimContext::PEDS_ART_COST_AGE_11MTH;
	if (ageMonths < 36)
		return SimContext::PEDS_ART_COST_AGE_2YR;
	if (ageMonths < 60)
		return SimContext::PEDS_ART_COST_AGE_4YR;
	if (ageMonths < 96)
		return SimContext::PEDS_ART_COST_AGE_7YR;
	if(ageMonths < 156)
		return SimContext::PEDS_ART_COST_AGE_12YR;
	else
		return SimContext::PEDS_ART_COST_AGE_ADULT;
}

/** \brief getTimeSummary returns a non-const pointer to the TimeSummary object for the current time period,
 *
 * \return the TimeSummary for the current time; creates a new one if needed or returns null if not keeping longitudinal stats
 **/
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

/** \brief incrementCostsCommon increases all general cost stats that are independent of the type of cost
 *
 * overloaded to take in either a single cost value or a COST_NUM_TYPES sized array of costs
 *
 * This cost will be discounted
 *
 *  \param cost a double representing the cost to be added
 *	\param percent a double representing the percent of the cost to apply
 **/
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

/** \brief incrementCostsCommon increases all general cost stats that are independent of the type of cost
 *
 * overloaded to take in either a single cost value or a COST_NUM_TYPES sized array of costs
 *
 * This cost will be discounted
 *
 *  \param costArray a pointer to an array of doubles of size SimContext::COST_NUM_TYPES representing the costs to be added
 *	\param percent a double representing the percent of the cost to apply
 **/
void StateUpdater::incrementCostsCommon(const double *costArray, double percent, double multiplier) {
	double discountFactor = patient->generalState.discountFactor;
	double discountedCostArray[SimContext::COST_NUM_TYPES];
	double discountedCostTotal = 0;

	// Calculated the discounted cost of each type and the total costs
	for (int i = 0; i < SimContext::COST_NUM_TYPES; i++) {
		discountedCostArray[i] = costArray[i] * discountFactor * percent * multiplier;
		discountedCostTotal += discountedCostArray[i];
	}



	// Update the patient state costs
	patient->generalState.costsDiscounted += discountedCostTotal;

	// Update the sum of costs by each cost type
	for (int i = 0; i < SimContext::COST_NUM_TYPES; i++) {
		runStats->overallCosts.totalUndiscountedCosts[i] += costArray[i] * percent * multiplier;
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
