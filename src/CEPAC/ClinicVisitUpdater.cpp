#include "include.h"

/* Constructor takes in the patient as a pointer */
ClinicVisitUpdater::ClinicVisitUpdater(Patient *patient) :
	StateUpdater(patient)
{

}

/* Destructor is empty, no cleanup required */
ClinicVisitUpdater::~ClinicVisitUpdater() {

}

/* performInitialUpdates perform all of the state and statistics updates upon patient creation */
void ClinicVisitUpdater::performInitialUpdates() {
	// First call the parent function to perform general updates and initialization
	StateUpdater::performInitialUpdates();

	// Set the type of clinic visits and available treatments for the patient
	SimContext::CLINIC_VISITS visitType = SimContext::CLINIC_SCHED;
	SimContext::THERAPY_IMPL treatmentType = SimContext::THERAPY_IMPL_PROPH_ART;
	double randNum = CepacUtil::getRandomDouble(60010, patient);
	for (int i = 0; i < SimContext::CLINIC_VISITS_NUM; i++) {
		if ((simContext->getCohortInputs()->clinicVisitTypeDistribution[i] != 0) &&
			(randNum < simContext->getCohortInputs()->clinicVisitTypeDistribution[i])) {
				visitType = (SimContext::CLINIC_VISITS) i;
				break;
		}
		randNum -= simContext->getCohortInputs()->clinicVisitTypeDistribution[i];
	}
	randNum = CepacUtil::getRandomDouble(60020, patient);
	for (int i = 0; i < SimContext::THERAPY_IMPL_NUM; i++) {
		if ((simContext->getCohortInputs()->therapyImplementationDistribution[i] != 0) &&
			(randNum < simContext->getCohortInputs()->therapyImplementationDistribution[i])) {
				treatmentType = (SimContext::THERAPY_IMPL) i;
				break;
		}
		randNum -= simContext->getCohortInputs()->therapyImplementationDistribution[i];
	}
	setClinicVisitType(visitType, treatmentType);

	// Set the patients baseline ART response propensity coefficient
	double baselineMean = simContext->getHeterogeneityInputs()->propRespondBaselineMean;
	double baselineStdDev = simContext->getHeterogeneityInputs()->propRespondBaselineStdDev;
	double baselineCoeff = CepacUtil::getRandomGaussian(baselineMean, baselineStdDev, 60030, patient);
	setARTResponseBaseline(baselineCoeff);

	// Set the patients CD4 response type
	randNum = CepacUtil::getRandomDouble(60040, patient);
	SimContext::CD4_RESPONSE_TYPE responseType = SimContext::CD4_RESPONSE_1;
	for (int i = 0; i < SimContext::CD4_RESPONSE_NUM_TYPES; i++) {
		if ((simContext->getCohortInputs()->CD4ResponseTypeOnARTDistribution[i] != 0) &&
			(randNum < simContext->getCohortInputs()->CD4ResponseTypeOnARTDistribution[i])) {
				responseType = (SimContext::CD4_RESPONSE_TYPE) i;
				break;
		}
		randNum -= simContext->getCohortInputs()->CD4ResponseTypeOnARTDistribution[i];
	}
	setCD4ResponseType(responseType);

	// Schedule the clinic visits and CD4/HVL test
	if (patient->getMonitoringState()->isDetectedHIVPositive) {
		scheduleInitialClinicVisit();
	}
	else {
		scheduleRegularClinicVisit(false);
		scheduleCD4Test(false);
		scheduleHVLTest(false);
	}
	scheduleEmergencyClinicVisit(false);

	// Set the observed health state of the patient to unknown values
	setObservedCD4(false);
	setObservedCD4Percentage(false);
	setObservedHVLStrata(false);
	resetClinicVisitState(true);

	// Initialize LTFU state to not lost and STI state to not interrupted
	setCurrLTFUState(SimContext::LTFU_STATE_NONE);
	setCurrSTIState(SimContext::STI_STATE_NONE);

	// Set the initial ART state to not on ART and determine next available regimen
	setInitialARTState();
	// Identify the first available art line
	bool hasNext = false;
	int nextRegimen = SimContext::NOT_APPL;
	for (int i = 0; i < SimContext::ART_NUM_LINES; i++) {
		if (simContext->getARTInputs(i)) {
				hasNext = true;
				nextRegimen = i;
				break;
		}
	}
	setNextARTRegimen(hasNext, nextRegimen);

	// Set the initial proph state to not on any prophs and determine next available ones
	setInitialProphState();
	// Determine patient prophylaxis non compliance
	randNum = CepacUtil::getRandomDouble(60060, patient);
	if (randNum < simContext->getCohortInputs()->OIProphNonComplianceRisk)
		setProphNonCompliance(true);
	else
		setProphNonCompliance(false);
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		// Set the initial proph that is available for use
		bool hasNext = false;
		int prophNum = SimContext::NOT_APPL;
		SimContext::PROPH_TYPE prophType = SimContext::PROPH_PRIMARY;
		if (patient->getDiseaseState()->hasTrueOIHistory[i])
			prophType = SimContext::PROPH_SECONDARY;
		for (int k = 0; k < SimContext::PROPH_NUM; k++) {
			const SimContext::ProphInputs *prophInput = simContext->getProphInputs(SimContext::PROPH_PRIMARY, i, k);
			if (prophInput) {
				hasNext = true;
				prophNum = k;
				break;
			}
		}
		setNextProph(hasNext, prophType, (SimContext::OI_TYPE) i, prophNum);
	}

	// Set the initial TB proph state to not be on or scheduled for TB proph
	setInitialTBProphState();
	// Identify the first available TB proph line
	hasNext = false;
	int prophNum = SimContext::NOT_APPL;
	for (int i = 0; i < SimContext::PROPH_NUM; i++) {
		if (simContext->getTBInputs()->tbProphInputs[i]) {
			hasNext = true;
			prophNum = i;
			break;
		}
	}
	setNextTBProph(hasNext, prophNum);

	// Set the initial TB treatment state to not be on or scheduled for TB treatment
	setInitialTBTreatmentState();
} /* end performInitialUpdates */

/* performMonthlyUpdates perform all of the state and statistics updates for a simulated month */
void ClinicVisitUpdater::performMonthlyUpdates() {
	if (!willAttendClinicThisMonth())
		return;

	/* handle emergency visits for OIs and determine if OIs are observed */
	performOIDetectionUpdates();

	/* accrue general costs for the clinic visit and increment number of visits */
	int ageCategory = patient->getGeneralState()->ageCategoryClinical;
	SimContext::GENDER_TYPE gender = patient->getGeneralState()->gender;
	const double *costs = simContext->getCostInputs()->clinicVisitCost[gender][ageCategory];
	incrementCostsClinicVisit(costs);
	incrementNumClinicVisits();

	// Print tracing for the clinic visit if enabled
	if (patient->getGeneralState()->tracingEnabled) {
		tracer->printTrace(1, "**%d CLINIC VISIT, $ %1.0lf;\n",
			patient->getGeneralState()->monthNum, patient->getGeneralState()->costsDiscounted);
	}

	/* schedule the next regular clinic visit */
	if (simContext->getTreatmentInputs()->emergencyVisitIsNotRegularVisit) {
		if (patient->getMonitoringState()->hasRegularClinicVisit &&
			(patient->getGeneralState()->monthNum >= patient->getMonitoringState()->monthOfRegularClinicVisit)) {
				scheduleRegularClinicVisit(true, patient->getGeneralState()->monthNum + simContext->getTreatmentInputs()->clinicVisitInterval);
		}
	}
	else {
		scheduleRegularClinicVisit(true, patient->getGeneralState()->monthNum + simContext->getTreatmentInputs()->clinicVisitInterval);
	}
	/* if clinic visit was an emergency one, update state to indicate that it occurred */
	if (patient->getMonitoringState()->hasEmergencyClinicVisit &&
		(patient->getGeneralState()->monthNum >= patient->getMonitoringState()->monthOfEmergencyClinicVisit)) {
			scheduleEmergencyClinicVisit(false);
	}

	/* Evaluate ART and prophylaxis policies and make changes to the treatment programs */
	performARTProgramUpdates();
	performProphProgramUpdates();

	/* Evaluate TB proph and treatment policies and makes to the treatment programs */
	performTBProphProgramUpdates();
	performTBTreatmentProgramUpdates();

	// reset the state for events since the last clinic visit
	resetClinicVisitState();
} /* end performMonthlyUpdates */

/* changes the inputs the updater uses to determine disease progression -- to be used primarily by the transmission model
 * changes the "nextARTRegimen" based on the ART regimens of the new simContext */
void ClinicVisitUpdater::setSimContext(SimContext *newSimContext){
	// First call the parent function to switch to newSimContext
	StateUpdater::setSimContext(newSimContext);
	//this->simContext = newSimContext;

	// determine next available regimen
	//Identify the current (if any) art line or the last ART line taken (if ever)
	int potentialNextARTRegimenNum = 0;
	if (patient->getARTState()->hasTakenART){
		if (patient->getARTState()->isOnART){
			potentialNextARTRegimenNum = patient->getARTState()->currRegimenNum + 1;
		} else {
			potentialNextARTRegimenNum = patient->getARTState()->prevRegimenNum + 1;
		}
	}

	// Identify the next available art line
	// Need to do here or if there are new/different ART lines in the new simContext, the patient will ignore them
	bool hasNext = false;
	int nextRegimen = SimContext::NOT_APPL;
	for (int i = potentialNextARTRegimenNum; i < SimContext::ART_NUM_LINES; i++) {
		if (simContext->getARTInputs(i)) {
				hasNext = true;
				nextRegimen = i;
				break;
		}
	}

	setNextARTRegimen(hasNext, nextRegimen);

	//Identify current proph state and determine next available one for each OI
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		// Set the initial proph that is available for use
		bool hasNext = false;
		int prophNum = SimContext::NOT_APPL;
		SimContext::PROPH_TYPE prophType = SimContext::PROPH_PRIMARY;
		if (patient->getDiseaseState()->hasTrueOIHistory[i])
			prophType = SimContext::PROPH_SECONDARY;

		//The first proph to look for -- will be 0 if the patient has never been on Proph or the current/last proph if patient *has* been on proph
		int potentialNextProphNum = 0;
		if (patient->getProphState()->hasTakenProph[i][prophType]){
			potentialNextProphNum = patient->getProphState()->currProphNum[i];
		}
		for (int k = potentialNextProphNum; k < SimContext::PROPH_NUM; k++) {
			const SimContext::ProphInputs *prophInput = simContext->getProphInputs(SimContext::PROPH_PRIMARY, i, k);
			if (prophInput) {
				hasNext = true;
				prophNum = k;
				break;
			}
		}
		setNextProph(hasNext, prophType, (SimContext::OI_TYPE) i, prophNum);
	}

	//TODO: The same as above for TB proph, plus TB treatment somehow has to be dealt with
	/*//Identify current TB proph state and determine next available one
	// Set the initial TB proph state to not be on or scheduled for TB proph
	setInitialTBProphState();
	// Identify the first available TB proph line
	hasNext = false;
	int prophNum = SimContext::NOT_APPL;
	for (int i = 0; i < SimContext::PROPH_NUM; i++) {
		if (simContext->getTBInputs()->tbProphInputs[i]) {
			hasNext = true;
			prophNum = i;
			break;
		}
	}
	setNextTBProph(hasNext, prophNum);*/
}

/* performOIDetectionUpdates handles the emergency OI clinic visit and
	determines if the current and prior OIs are observed */
void ClinicVisitUpdater::performOIDetectionUpdates() {
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		bool isObserved = false;

		// Determine if OIs are observed
		if (patient->getDiseaseState()->hasCurrTrueOI && (patient->getDiseaseState()->typeCurrTrueOI == i)) {
			// Patient currently has acute OI, always count as observed
			isObserved = true;
		}
		else if (!patient->getMonitoringState()->hadPrevClinicVisit && patient->getDiseaseState()->hasTrueOIHistory[i]) {
			// First clinic visit and patient has history of OI, roll for being observed
			double randNum = CepacUtil::getRandomDouble(60070, patient);
			if (randNum < simContext->getTreatmentInputs()->probDetectOIAtEntry[i]) {
				isObserved = true;
			}
		}
		else if (patient->getMonitoringState()->hadPrevClinicVisit && (patient->getDiseaseState()->numTrueOIsSinceLastVisit[i] > 0)) {
			// Subsequent clinic visit and patient had OI since last visit, roll for being observed
			double randNum = CepacUtil::getRandomDouble(60080, patient);
			if (randNum < simContext->getTreatmentInputs()->probDetectOISinceLastVisit[i]) {
				isObserved = true;
			}
		}

		// If OI is observed, increment number observed and output tracing
		if (isObserved) {
			incrementNumObservedOIs((SimContext::OI_TYPE) i, 1);
			if (patient->getGeneralState()->tracingEnabled) {
				tracer->printTrace(1, "**%d OBSV OI %s;\n",
					patient->getGeneralState()->monthNum, SimContext::OI_STRS[i]);
			}

			// If on ART, determine if observed OI should count towards ART failure
			if (patient->getARTState()->isOnART) {
				int artLineNum = patient->getARTState()->currRegimenNum;
				const SimContext::TreatmentInputs::ARTFailPolicy &failART = simContext->getTreatmentInputs()->failART[artLineNum];
				bool isFailureOI = false;
				if (patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart >= failART.OIsMonthsFromInit) {
					if (failART.OIsEvent[i] == SimContext::ART_FAIL_BY_OI_ANY)
						isFailureOI = true;
					else if (!patient->getDiseaseState()->hasTrueOIHistory[i] &&
						(failART.OIsEvent[i] == SimContext::ART_FAIL_BY_OI_PRIMARY))
							isFailureOI = true;
					else if (patient->getDiseaseState()->hasTrueOIHistory[i] &&
						(failART.OIsEvent[i] == SimContext::ART_FAIL_BY_OI_SECONDARY))
							isFailureOI = true;
				}

				// If OI should count towards failure, increment count and trigger a CD4 or HVL test if
				//	confirmatory testing is needed
				if (isFailureOI) {
					incrementARTFailedOIs();
					if (patient->getARTState()->numFailedOIs >= failART.OIsMinNum) {
						if (failART.diagnoseUseCD4TestsConfirm)
							patient->getCD4TestUpdater()->performMonthlyUpdates();
						if (failART.diagnoseUseHVLTestsConfirm)
							patient->getHVLTestUpdater()->performMonthlyUpdates();
					}
				}
			}
		}
	}
} /* end performOIDetectionUpdates */

/* performARTProgramUpdates evaluates ART policies and alters the treatment program */
void ClinicVisitUpdater::performARTProgramUpdates() {
	// return if ART treatments are not available to the patient
	if (!patient->getARTState()->mayReceiveART)
		return;

	// If patient is currently on ART and not already observed to have failed,
	//	determine if failure is observed this month
	if (patient->getARTState()->isOnART && !patient->getARTState()->hasObservedFailure) {
		if (patient->getARTState()->currSTIState == SimContext::STI_STATE_NONE) {
			SimContext::ART_FAIL_TYPE failType = evaluateFailARTPolicy();
			if (failType != SimContext::ART_FAIL_NOT_FAILED) {
				setCurrARTObservedFailure(failType);
				if (patient->getGeneralState()->tracingEnabled) {
					tracer->printTrace(1, "**%d ART %d FAIL OBSV BY %s;\n", patient->getGeneralState()->monthNum,
						patient->getARTState()->currRegimenNum + 1,
						SimContext::ART_FAIL_TYPE_STRS[patient->getARTState()->typeObservedFailure]);
				}
			}
		}
		else if (patient->getARTState()->currSTIState == SimContext::STI_STATE_RESTART) {
			SimContext::ART_FAIL_TYPE failType = evaluateSTIEndpointPolicy();
			if (failType != SimContext::ART_FAIL_NOT_FAILED) {
				setCurrSTIState(SimContext::STI_STATE_ENDPOINT);
				setCurrARTObservedFailure(failType);
				if (patient->getGeneralState()->tracingEnabled) {
					tracer->printTrace(1, "**%d ART %d STI ENDPOINT OBSV BY %s;\n", patient->getGeneralState()->monthNum,
						patient->getARTState()->currRegimenNum + 1,
						SimContext::ART_FAIL_TYPE_STRS[patient->getARTState()->typeObservedFailure]);
				}
			}
		}
		// If failure was observed this month, determine if regimen should be restarted based on
		// 	the patient's response type
		if (patient->getARTState()->hasObservedFailure) {
			SimContext::RESP_TYPE responseType = patient->getARTState()->responseTypeCurrRegimen;
			double probRestart = simContext->getHeterogeneityInputs()->probRestartARTRegimenAfterFailure[responseType];
			double randNum = CepacUtil::getRandomDouble(60090, patient);
			if (randNum < probRestart) {
				setNextARTRegimen(true, patient->getARTState()->currRegimenNum);
			}
		}
	}

	// If patient is on ART, evaluate stopping criteria
	SimContext::ART_STOP_TYPE stopType = SimContext::ART_STOP_NOT_STOPPED;
	bool stiInterrupt = false;
	if (patient->getARTState()->isOnART) {
		if (patient->getARTState()->currSTIState == SimContext::STI_STATE_NONE) {
			stopType = evaluateStopARTPolicy();
			if (stopType == SimContext::ART_STOP_NOT_STOPPED) {
				if (evaluateSTIInitialStopPolicy()) {
					stopType = SimContext::ART_STOP_STI;
					stiInterrupt = true;
				}
			}
		}
		else if (patient->getARTState()->currSTIState == SimContext::STI_STATE_RESTART) {
			if (evaluateSTISubsequentStopPolicy()) {
				stopType = SimContext::ART_STOP_STI;
				stiInterrupt = true;
			}
		}
	}

	// Stop the current ART regimen if it was determined to do so
	if (stopType != SimContext::ART_STOP_NOT_STOPPED) {
		// For STI stop, update the state to interrupt and set next regimen to the current one
		if (stiInterrupt) {
			setCurrSTIState(SimContext::STI_STATE_INTERRUPT);
			setNextARTRegimen(true, patient->getARTState()->currRegimenNum);
		}

		// Stop the ART regimen and set target HVL back to the setpoint
		stopCurrARTRegimen(stopType);
		setTargetHVLStrata(patient->getDiseaseState()->setpointHVLStrata);

		// Output tracing if enabled
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d TAKEN OFF ART %d by %s;\n", patient->getGeneralState()->monthNum,
				patient->getARTState()->prevRegimenNum + 1,
				SimContext::ART_STOP_TYPE_STRS[patient->getARTState()->typeCurrStop]);
		}

		// If no more lines are available, set the post-ART CD4/HVL testing interval
		if (!patient->getARTState()->hasNextRegimenAvailable) {
			int intervalCD4 = simContext->getTreatmentInputs()->CD4TestingIntervalPostART;
			if (intervalCD4 != SimContext::NOT_APPL)
				scheduleCD4Test(true, patient->getGeneralState()->monthNum + intervalCD4);
			else
				scheduleCD4Test(false);
			int intervalHVL = simContext->getTreatmentInputs()->HVLTestingIntervalPostART;
			if (intervalHVL != SimContext::NOT_APPL)
				scheduleHVLTest(true, patient->getGeneralState()->monthNum + intervalHVL);
			else
				scheduleHVLTest(false);
		}
	}

	// If patient is not on ART, determine if the patient should start a new regimen this month
	bool startNextART = false;
	bool rtcStart = false;
	bool stiRestart = false;
	if (!patient->getARTState()->isOnART) {
		// If returning to care and was on ART when lost, ART should be restarted
		if ((patient->getMonitoringState()->currLTFUState == SimContext::LTFU_STATE_RETURNED) &&
			(patient->getMonitoringState()->monthOfLTFUStateChange == patient->getGeneralState()->monthNum) &&
			patient->getARTState()->hasNextRegimenAvailable &&
			patient->getMonitoringState()->wasOnARTWhenLostToFollowUp) {
				startNextART = true;
				rtcStart = true;
		}
		// If currently on STI interruption, evaluate STI restart criteria
		else if (patient->getARTState()->currSTIState == SimContext::STI_STATE_INTERRUPT) {
				if (evaluateSTIRestartPolicy()) {
					stiRestart = true;
					startNextART = true;
				}
		}
		// Otherwise, evaluate normal ART starting criteria
		else if (evaluateStartARTPolicy()) {
			startNextART = true;
		}
	}

	// Start the next ART regimen if it was determined to do so
	if (startNextART) {
		// Start the ART regimen, update STI state if this is a restart
		if (stiRestart) {
			setCurrSTIState(SimContext::STI_STATE_RESTART);
		}
		startNextARTRegimen();

		// Get the current ART regimen information
		int currRegimen = patient->getARTState()->currRegimenNum;
		int currSubRegimen = patient->getARTState()->currSubRegimenNum;
		const SimContext::ARTInputs *artInput = simContext->getARTInputs(currRegimen);
		const SimContext::PedsARTInputs *pedsART = simContext->getPedsARTInputs(currRegimen);
		SimContext::PEDS_AGE_CAT pedsAgeCat = patient->getGeneralState()->ageCategoryPediatrics;

		// Determine and set the ART response type for this regimen
		double responseLogit = patient->getARTState()->responseBaselineLogit;
		int ageCat = patient->getGeneralState()->ageCategoryHIVInfection;
		responseLogit += simContext->getHeterogeneityInputs()->propRespondAge[ageCat];
		SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
		responseLogit += simContext->getHeterogeneityInputs()->propRespondCD4[cd4Strata];
		if (patient->getGeneralState()->gender == SimContext::GENDER_FEMALE)
			responseLogit += simContext->getHeterogeneityInputs()->propRespondFemale;
		if (patient->getDiseaseState()->typeTrueOIHistory != SimContext::HIST_EXT_N)
			responseLogit += simContext->getHeterogeneityInputs()->propRespondHistoryOIs;
		if (patient->getARTState()->hadPrevToxicity)
			responseLogit += simContext->getHeterogeneityInputs()->propRespondPriorARTToxicity;
		for (int i = 0; i < SimContext::RISK_FACT_NUM; i++) {
			if (patient->getGeneralState()->hasRiskFactor[i])
				responseLogit += simContext->getHeterogeneityInputs()->propRespondRiskFactor[i];
		}
		responseLogit += simContext->getHeterogeneityInputs()->propRespondARTRegimen[currRegimen];
		double responseStdDev = simContext->getHeterogeneityInputs()->propRespondIndividualStdDev;
		responseLogit += CepacUtil::getRandomGaussian(0, responseStdDev, 60100, patient);
		double propRespond = pow(1 + exp(0 - responseLogit), -1);
		setCurrARTResponse(propRespond);

		// Determine and set the initial efficacy of the regimen
		// Determine the probability of suppression
		SimContext::HVL_STRATA hvlStrata = patient->getDiseaseState()->currTrueHVLStrata;
		SimContext::RESP_TYPE responseType = patient->getARTState()->responseTypeCurrRegimen;
		double probSuppress = 0.0;
		if (pedsAgeCat == SimContext::PEDS_AGE_ADULT) {
			probSuppress = artInput->probInitialEfficacy[SimContext::ART_EFF_SUCCESS][hvlStrata];
		}
		else if (pedsAgeCat == SimContext::PEDS_AGE_LATE) {
			probSuppress = pedsART->probInitialEfficacyLate[SimContext::ART_EFF_SUCCESS][hvlStrata];
		}
		else {
			probSuppress = pedsART->probInitialEfficacyEarly[SimContext::ART_EFF_SUCCESS][hvlStrata];
		}
		//If just Returned To Care and restarting the previous ART, use the alternate probability of suppression
		if (rtcStart && currRegimen == patient->getARTState()->prevRegimenNum) {
			if (patient->getARTState()->prevRegimenEfficacy == SimContext::ART_EFF_SUCCESS){
				probSuppress = simContext->getLTFUInputs()->probSuppressionWhenReturnToSuppressed[currRegimen];
			}
			else {
				probSuppress = simContext->getLTFUInputs()->probSuppressionWhenReturnToFailed[currRegimen];
			}
		}
		// Modify probability of suppression by resistance penalty
		for (int i = 0; i <= currRegimen; i++) {
			int numMonths = patient->getARTState()->numMonthsOnUnsuccessfulByRegimen[i];
			double resistFactor = simContext->getTreatmentInputs()->ARTResistancePriorRegimen[currRegimen][i];
			probSuppress *= pow(1 - resistFactor, numMonths);
		}
		for (int i = 0; i < SimContext::HVL_NUM_STRATA; i++) {
			int numMonths = patient->getARTState()->numMonthsOnUnsuccessfulByHVL[i];
			double resistFactor = simContext->getTreatmentInputs()->ARTResistanceHVL[i];
			probSuppress *= pow(1 - resistFactor, numMonths);
		}
		// Multiply probability by the reduction factor from the ART response type
		probSuppress *= patient->getARTState()->responseFactorCurrRegimen;
		// Determine the probability of partial suppression, set to 0 for non-responders
		// 	NOTE: currently does not use resistance penalty, heterogeneity, or LTFU modifiers
		//		will need to be fixed if partial suppression is to be used again
		double probPartialSuppress = 0.0;
		if (pedsAgeCat == SimContext::PEDS_AGE_ADULT) {
			probPartialSuppress = artInput->probInitialEfficacy[SimContext::ART_EFF_PARTIAL][hvlStrata];
		}
		else if (pedsAgeCat == SimContext::PEDS_AGE_LATE) {
			probPartialSuppress = pedsART->probInitialEfficacyLate[SimContext::ART_EFF_PARTIAL][hvlStrata];
		}
		else {
			probPartialSuppress = pedsART->probInitialEfficacyEarly[SimContext::ART_EFF_PARTIAL][hvlStrata];
		}
		// Set the initial efficacy of the regimen using the calculated probabilities
		SimContext::ART_EFF_TYPE efficacy;
		double randNum = CepacUtil::getRandomDouble(60110, patient);
		if ((probSuppress > 0) && (randNum < probSuppress)) {
			efficacy = SimContext::ART_EFF_SUCCESS;
		}
		else if ((probPartialSuppress > 0) && (randNum - probSuppress < probPartialSuppress)) {
			efficacy = SimContext::ART_EFF_PARTIAL;
		}
		else {
			efficacy = SimContext::ART_EFF_FAILURE;
		}
		setCurrARTEfficacy(efficacy, true);

		// Print debugging information if enabled
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d INIT NEW ART %d, $ %1.0lf;\n", patient->getGeneralState()->monthNum,
				patient->getARTState()->currRegimenNum + 1, patient->getGeneralState()->costsDiscounted);
			tracer->printTrace(1, "**%d ART DRAW %s, %s;\n", patient->getGeneralState()->monthNum,
				SimContext::ART_EFF_STRS[efficacy], SimContext::RESP_TYPE_STRS[responseType]);
		}

		// Set the target HVL based on the destined efficacy
		if (efficacy == SimContext::ART_EFF_SUCCESS) {
			setTargetHVLStrata(SimContext::HVL_VLO);
		}
		else if (efficacy == SimContext::ART_EFF_PARTIAL) {
			setTargetHVLStrata(getPartialSuppressTargetHVL(currRegimen));
		}
		else {
			setTargetHVLStrata(patient->getDiseaseState()->setpointHVLStrata);
		}

		// Set the initial CD4 slope for suppressive ART based on the response type,
		//	also set the CD4 envelope regimen and slope if this is the first successful regimen
		if (efficacy != SimContext::ART_EFF_FAILURE) {
			SimContext::CD4_RESPONSE_TYPE cd4Response = patient->getARTState()->CD4ResponseType;
			if (pedsAgeCat == SimContext::PEDS_AGE_ADULT) {
				double cd4SlopeMean = artInput->CD4ChangeOnARTMean[efficacy][cd4Response][0];
				double cd4SlopeStdDev = artInput->CD4ChangeOnARTStdDev[efficacy][cd4Response][0];
				double cd4Slope = CepacUtil::getRandomGaussian(cd4SlopeMean, cd4SlopeStdDev, 60120, patient);
				// Adjust slope by the reduction factor from the ART response type
				cd4Slope *= patient->getARTState()->responseFactorCurrRegimen;
				setCurrRegimenCD4Slope(cd4Slope);
				if (efficacy == SimContext::ART_EFF_SUCCESS) {
					if (!patient->getARTState()->overallCD4Envelope.isActive) {
						setCD4EnvelopeRegimen(SimContext::ENVL_CD4_OVERALL, currRegimen);
						setCD4EnvelopeSlope(SimContext::ENVL_CD4_OVERALL, cd4Slope);
					}
					if (!patient->getARTState()->indivCD4Envelope.isActive) {
						setCD4EnvelopeRegimen(SimContext::ENVL_CD4_INDIV, currRegimen);
						setCD4EnvelopeSlope(SimContext::ENVL_CD4_INDIV, cd4Slope);
					}
				}
			}
			else if (pedsAgeCat == SimContext::PEDS_AGE_LATE) {
				double cd4SlopeMean = pedsART->CD4ChangeOnARTMeanLate[efficacy][cd4Response][0];
				double cd4SlopeStdDev = pedsART->CD4ChangeOnARTStdDevLate[efficacy][cd4Response][0];
				double cd4Slope = CepacUtil::getRandomGaussian(cd4SlopeMean, cd4SlopeStdDev, 60121, patient);
				// Adjust slope by the reduction factor from the ART response type
				cd4Slope *= patient->getARTState()->responseFactorCurrRegimen;
				setCurrRegimenCD4Slope(cd4Slope);
				if (efficacy == SimContext::ART_EFF_SUCCESS) {
					if (!patient->getARTState()->overallCD4Envelope.isActive) {
						setCD4EnvelopeRegimen(SimContext::ENVL_CD4_OVERALL, currRegimen);
						setCD4EnvelopeSlope(SimContext::ENVL_CD4_OVERALL, cd4Slope);
					}
					if (!patient->getARTState()->indivCD4Envelope.isActive) {
						setCD4EnvelopeRegimen(SimContext::ENVL_CD4_INDIV, currRegimen);
						setCD4EnvelopeSlope(SimContext::ENVL_CD4_INDIV, cd4Slope);
					}
				}
			}
			else {
				double cd4PercSlopeMean = pedsART->CD4PercentageChangeOnARTMeanEarly[efficacy][pedsAgeCat][cd4Response][0];
				double cd4PercSlopeStdDev = pedsART->CD4PercentageChangeOnARTStdDevEarly[efficacy][pedsAgeCat][cd4Response][0];
				double cd4PercSlope = CepacUtil::getRandomGaussian(cd4PercSlopeMean, cd4PercSlopeStdDev, 60122, patient);
				// Adjust slope by the reduction factor from the ART response type
				cd4PercSlope *= patient->getARTState()->responseFactorCurrRegimen;
				setCurrRegimenCD4PercentageSlope(cd4PercSlope);
				if (efficacy == SimContext::ART_EFF_SUCCESS) {
					if (!patient->getARTState()->overallCD4PercentageEnvelope.isActive) {
						setCD4EnvelopeRegimen(SimContext::ENVL_CD4_PERC_OVERALL, currRegimen);
						setCD4EnvelopeSlope(SimContext::ENVL_CD4_PERC_OVERALL, cd4PercSlope);
					}
					if (!patient->getARTState()->indivCD4PercentageEnvelope.isActive) {
						setCD4EnvelopeRegimen(SimContext::ENVL_CD4_PERC_INDIV, currRegimen);
						setCD4EnvelopeSlope(SimContext::ENVL_CD4_PERC_INDIV, cd4PercSlope);
					}
				}
			}
		}

		// Accumulate the initial startup cost for this regimen
		if (pedsAgeCat == SimContext::PEDS_AGE_ADULT) {
			incrementCostsART(currRegimen, artInput->costInitial);
		}
		else if (pedsAgeCat == SimContext::PEDS_AGE_LATE) {
			incrementCostsART(currRegimen, pedsART->costInitialLate);
		}
		else {
			incrementCostsART(currRegimen, pedsART->costInitialEarly);
		}

		// Identify the next available art line, need to do here since starting
		//	criteria for the next line may be evaluated before current one is stopped
		bool hasNext = false;
		int nextRegimen = SimContext::NOT_APPL;
		for (int i = patient->getARTState()->currRegimenNum + 1; i < SimContext::ART_NUM_LINES; i++) {
			if (simContext->getARTInputs(i)) {
					hasNext = true;
					nextRegimen = i;
					break;
			}
		}
		setNextARTRegimen(hasNext, nextRegimen);

		// If CD4/HVL tests should happen at ART init, trigger them here if they have not yet occurred
		if (simContext->getTreatmentInputs()->numARTInitialCD4Tests > 0) {
			patient->getCD4TestUpdater()->performMonthlyUpdates();
		}
		if (simContext->getTreatmentInputs()->numARTInitialHVLTests > 0) {
			patient->getHVLTestUpdater()->performMonthlyUpdates();
		}

		// Set the on-ART CD4/HVL testing intervals
		int intervalCD4 = SimContext::NOT_APPL;
		if (patient->getARTState()->hasNextRegimenAvailable)
			intervalCD4 = simContext->getTreatmentInputs()->CD4TestingIntervalOnART[0];
		else
			intervalCD4 = simContext->getTreatmentInputs()->CD4TestingIntervalOnLastART[0];
		if (intervalCD4 != SimContext::NOT_APPL)
			scheduleCD4Test(true, patient->getGeneralState()->monthNum + intervalCD4);
		else
			scheduleCD4Test(false);
		int intervalHVL = SimContext::NOT_APPL;
		if (patient->getARTState()->hasNextRegimenAvailable)
			intervalHVL = simContext->getTreatmentInputs()->HVLTestingIntervalOnART[0];
		else
			intervalHVL = simContext->getTreatmentInputs()->HVLTestingIntervalOnLastART[0];
		if (intervalHVL != SimContext::NOT_APPL)
			scheduleHVLTest(true, patient->getGeneralState()->monthNum + intervalHVL);
		else
			scheduleHVLTest(false);
	}

	// Set the initial ART subregimen or determine if the subregimen needs to be switched
	if (patient->getARTState()->isOnART) {
		int currRegimen = patient->getARTState()->currRegimenNum;
		const SimContext::ARTInputs *artInput = simContext->getARTInputs(currRegimen);
		bool startSubRegimen = false;
		int nextSubRegimen = 0;
		// Always start the first subregimen for a new ART line
		if (startNextART) {
			startSubRegimen = true;
			nextSubRegimen = 0;
		}
		else {
			int currSubRegimen = patient->getARTState()->currSubRegimenNum;
			int monthsSubRegimen = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrSubRegimenStart;
			// Switch subregimen if triggered by a toxicity
			if (patient->getARTState()->hasSevereToxicity) {
				startSubRegimen = true;
				const SimContext::ARTToxicityEffect *toxEffect = patient->getARTState()->severeToxicityEffect;
				const SimContext::ARTInputs::ARTToxicity &toxInputs = artInput->toxicity[toxEffect->ARTSubRegimenNum][toxEffect->toxSeverityType][toxEffect->toxNum];
				nextSubRegimen = toxInputs.switchSubRegimenOnToxicity;
			}
			// Switch subregimen if specified time on subregimen has been exceeded
			else if ((artInput->monthsToSwitchSubRegimen[currSubRegimen] != SimContext::NOT_APPL) &&
				(monthsSubRegimen >= artInput->monthsToSwitchSubRegimen[currSubRegimen])) {
					startSubRegimen = true;
					nextSubRegimen = currSubRegimen + 1;
			}
		}

		if (startSubRegimen) {
			// Start that next ART subregimen
			startNextARTSubRegimen(nextSubRegimen);
			int currSubRegimen = nextSubRegimen;

			// Output tracing for the start of the ART subregimen
			if (patient->getGeneralState()->tracingEnabled) {
				tracer->printTrace(1, "**%d %s ART SUBREGIMEN %d.%d\n",
					patient->getGeneralState()->monthNum, startNextART ? "INIT" : "SWITCH",
					currRegimen + 1, currSubRegimen);
			}

			// Roll for all ART toxicities for new subregimen, toxicity does not occur for non-responders
			if (patient->getARTState()->responseTypeCurrRegimen != SimContext::RESP_TYPE_NON) {
				for (int i = 0; i < SimContext::ART_NUM_TOX_SEVERITY; i++) {
					for (int j = 0; j < SimContext::ART_NUM_TOX_PER_SEVERITY; j++) {
						double randNum = CepacUtil::getRandomDouble(60130, patient);
						if (randNum < artInput->toxicity[currSubRegimen][i][j].probToxicity) {
							bool hasTox = false;
							// Determine if the patient already has this toxicity effect
							const list<SimContext::ARTToxicityEffect> &toxicities = patient->getARTState()->activeToxicityEffects;
							for (list<SimContext::ARTToxicityEffect>::const_iterator k = toxicities.begin(); k != toxicities.end(); k++) {
								if ((k->toxSeverityType == i) && (k->toxNum == j)) {
									hasTox = true;
									break;
								}
							}
							// Add toxicity effect to the active list if it is not already present
							if (!hasTox) {
								double timeToToxMean =  artInput->toxicity[currSubRegimen][i][j].timeToToxicityMean;
								double timeToToxStdDev = artInput->toxicity[currSubRegimen][i][j].timeToToxicityStdDev;
								int timeToTox = (int) (CepacUtil::getRandomGaussian(timeToToxMean, timeToToxStdDev, 60140, patient) + 0.5);
								addARTToxicityEffect((SimContext::ART_TOX_SEVERITY) i, j, timeToTox);
							}
						}
					}
				}
			}
		}
	}
} /* end performARTProgramUpdates */

/* evaluateStartARTPolicy determines if the starting criteria for ART has been met */
bool ClinicVisitUpdater::evaluateStartARTPolicy() {
	// return false if there are no more available regimens
	if (!patient->getARTState()->hasNextRegimenAvailable)
		return false;

	int artLineNum = patient->getARTState()->nextRegimenNum;
	const SimContext::TreatmentInputs::ARTStartPolicy &startART = simContext->getTreatmentInputs()->startART[artLineNum];

	// return false if minimum time before starting has not yet been reached
	if ((startART.minMonthNum != SimContext::NOT_APPL) &&
		(patient->getGeneralState()->monthNum < startART.minMonthNum)) {
			return false;
	}

	// return false if minimum time since last regimen stop has not yet been reached
	if (startART.monthsSincePrevRegimen != SimContext::NOT_APPL) {
		if (patient->getARTState()->isOnART)
			return false;
		if ((patient->getARTState()->monthOfPrevRegimenStop != SimContext::NOT_APPL) &&
			(patient->getGeneralState()->monthNum - patient->getARTState()->monthOfPrevRegimenStop < startART.monthsSincePrevRegimen))
			return false;
	}

	// return false if minimum age for starting regimen has not yet been reached
	if ((simContext->getPedsInputs()->startARTMinAgeMonths[artLineNum] != SimContext::NOT_APPL) &&
		(patient->getGeneralState()->monthNum < simContext->getPedsInputs()->startARTMinAgeMonths[artLineNum])) {
			return false;
	}

	// Evaluate the CD4 only criteria
	double observedCD4 = patient->getMonitoringState()->currObservedCD4;
	if (patient->getMonitoringState()->hasObservedCD4 &&
		(observedCD4 >= startART.CD4BoundsOnly[SimContext::LOWER_BOUND]) &&
		(observedCD4 <= startART.CD4BoundsOnly[SimContext::UPPER_BOUND])) {
			return true;
	}

	// Evaluate the HVL strata only criteria
	SimContext::HVL_STRATA observedHVL = patient->getMonitoringState()->currObservedHVLStrata;
	if (patient->getMonitoringState()->hasObservedHVLStrata &&
		(observedHVL >= startART.HVLBoundsOnly[SimContext::LOWER_BOUND]) &&
		(observedHVL <= startART.HVLBoundsOnly[SimContext::UPPER_BOUND])) {
			return true;
	}

	// Evaluate the CD4 and HVL combined criteria
	if (patient->getMonitoringState()->hasObservedCD4 &&
		(observedCD4 >= startART.CD4BoundsWithHVL[SimContext::LOWER_BOUND]) &&
		(observedCD4 <= startART.CD4BoundsWithHVL[SimContext::UPPER_BOUND]) &&
		patient->getMonitoringState()->hasObservedHVLStrata &&
		(observedHVL >= startART.HVLBoundsWithCD4[SimContext::LOWER_BOUND]) &&
		(observedHVL <= startART.HVLBoundsWithCD4[SimContext::UPPER_BOUND])) {
			return true;
	}

	// Evaluate the CD4 percentage only criteria
	double observedCD4Percent = patient->getMonitoringState()->currObservedCD4Percentage;
	if (patient->getMonitoringState()->hasObservedCD4Percentage &&
		(observedCD4Percent >= simContext->getPedsInputs()->startARTCD4PercentageBoundsOnly[artLineNum][SimContext::LOWER_BOUND]) &&
		(observedCD4Percent <= simContext->getPedsInputs()->startARTCD4PercentageBoundsOnly[artLineNum][SimContext::UPPER_BOUND])) {
			return true;
	}

	// Evaluate the CD4 percentage and HVL combined criteria
	if (patient->getMonitoringState()->hasObservedCD4Percentage &&
		(observedCD4Percent >= simContext->getPedsInputs()->startARTCD4PercentageBoundsWithHVL[artLineNum][SimContext::LOWER_BOUND]) &&
		(observedCD4Percent <= simContext->getPedsInputs()->startARTCD4PercentageBoundsWithHVL[artLineNum][SimContext::UPPER_BOUND]) &&
		patient->getMonitoringState()->hasObservedHVLStrata &&
		(observedHVL >= simContext->getPedsInputs()->startARTHVLBoundsWithCD4Percentage[artLineNum][SimContext::LOWER_BOUND]) &&
		(observedHVL <= simContext->getPedsInputs()->startARTHVLBoundsWithCD4Percentage[artLineNum][SimContext::UPPER_BOUND])) {
			return true;
	}

	// Evaluate the acute OIs since last ART only criteria
	int numOIs = 0;
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		if (startART.OIHistory[i]) {
			numOIs += patient->getARTState()->numObservedOIsSinceFailOrStopART[i];
		}
	}
	if (numOIs >= startART.numOIs)
		return true;

	// Evaluate the acute OIs in patient's history and CD4 count criteria
	if ((observedCD4 != SimContext::NOT_APPL) &&
		(observedCD4 >= startART.CD4BoundsWithOIs[SimContext::LOWER_BOUND]) &&
		(observedCD4 <= startART.CD4BoundsWithOIs[SimContext::UPPER_BOUND])) {
			for (int i = 0; i < SimContext::OI_NUM; i++) {
				if (startART.OIHistoryWithCD4[i] && (patient->getMonitoringState()->numObservedOIsTotal[i] > 0)) {
					return true;
				}
			}
	}

	return false;
} /* end evaluateStartARTPolicy */

/* evaluateFailARTPolicy determines if the observed failure criteria for ART has been met */
SimContext::ART_FAIL_TYPE ClinicVisitUpdater::evaluateFailARTPolicy() {
	int artLineNum = patient->getARTState()->currRegimenNum;
	const SimContext::TreatmentInputs::ARTFailPolicy &failART = simContext->getTreatmentInputs()->failART[artLineNum];

	// check for clinical (OI based) failure
	if (patient->getARTState()->numFailedOIs >= failART.OIsMinNum) {
		if (failART.diagnoseUseHVLTestsConfirm) {
			// Using confirmatory HVL testing, also verify that this criteria has been met
			if (patient->getARTState()->numFailedHVLTests >= failART.diagnoseNumTestsConfirm)
				return SimContext::ART_FAIL_CLINICAL;
		}
		if (failART.diagnoseUseCD4TestsConfirm) {
			// Using confirmatory CD4 testing, also verify that this criteria has been met
			if (patient->getARTState()->numFailedCD4Tests >= failART.diagnoseNumTestsConfirm)
				return SimContext::ART_FAIL_CLINICAL;
		}
		if (!failART.diagnoseUseHVLTestsConfirm && !failART.diagnoseUseCD4TestsConfirm) {
			// No confirmatory testing, return clinical failure
			return SimContext::ART_FAIL_CLINICAL;
		}
	}

	// check for immunologic failure
	if (patient->getARTState()->numFailedCD4Tests >= failART.diagnoseNumTestsFail) {
		if (failART.diagnoseUseHVLTestsConfirm) {
			// Using confirmatory HVL testing, also verify that this criteria has been met
			if (patient->getARTState()->numFailedHVLTests >= failART.diagnoseNumTestsConfirm)
				return SimContext::ART_FAIL_IMMUNOLOGIC;
		}
		else {
			// No confirmatory testing, return immunologic failure
			return SimContext::ART_FAIL_IMMUNOLOGIC;
		}
	}

	// check for virologic failure
	if (patient->getARTState()->numFailedHVLTests >= failART.diagnoseNumTestsFail) {
		return SimContext::ART_FAIL_VIROLOGIC;
	}

	return SimContext::ART_FAIL_NOT_FAILED;
} /* end evaluateFailARTPolicy */

/* evaluateStopARTPolicy determines if the stopping criteria for ART has been met */
SimContext::ART_STOP_TYPE ClinicVisitUpdater::evaluateStopARTPolicy() {
	int artLineNum = patient->getARTState()->currRegimenNum;
	const SimContext::TreatmentInputs::ARTStopPolicy &stopART = simContext->getTreatmentInputs()->stopART[artLineNum];

	// check if maximum months on ART is exceeded
	if ((stopART.maxMonthsOnART != SimContext::NOT_APPL) &&
		(patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart >= stopART.maxMonthsOnART)) {
			return SimContext::ART_STOP_MAX_MTHS;
	}
	// check if maximum age months has been exceeded
	if ((simContext->getPedsInputs()->stopARTMaxAgeMonths[artLineNum] != SimContext::NOT_APPL) &&
		(patient->getGeneralState()->monthNum >= simContext->getPedsInputs()->stopARTMaxAgeMonths[artLineNum])) {
			return SimContext::ART_STOP_MAX_MTHS;
	}

	// check if a major toxicity has occurred and is specified to cause the regimen to be stopped
	if (stopART.withMajorToxicty && patient->getARTState()->hasMajorToxicity)
		return SimContext::ART_STOP_MAJ_TOX;

	// Return not stopped if there has not been an observed failure, or have not reached the
	//	minimum month number or months on ART
	if (!patient->getARTState()->hasObservedFailure)
		return SimContext::ART_STOP_NOT_STOPPED;
	if ((stopART.afterFailMinMonthNum > 0) &&
		(patient->getGeneralState()->monthNum < stopART.afterFailMinMonthNum)) {
			return SimContext::ART_STOP_NOT_STOPPED;
	}
	if ((stopART.afterFailMonthsFromInit > 0) &&
		(patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart < stopART.afterFailMonthsFromInit)) {
			return SimContext::ART_STOP_NOT_STOPPED;
	}

	// If we reach here, observed failure has already occurred
	// check if should stop immediately upon failure
	if (stopART.afterFailImmediate) {
		return SimContext::ART_STOP_FAIL;
	}
	// check if minimum CD4 threshold has been reached
	if ((stopART.afterFailCD4LowerBound != SimContext::NOT_APPL) &&
		patient->getMonitoringState()->hasObservedCD4 &&
		(patient->getMonitoringState()->currObservedCD4 <= stopART.afterFailCD4LowerBound)) {
			return SimContext::ART_STOP_CD4;
	}
	// check if minimum CD4 percentage threshold has been reached
	if ((simContext->getPedsInputs()->stopARTAfterFailCD4PercentageBound[artLineNum] != SimContext::NOT_APPL) &&
		patient->getMonitoringState()->hasObservedCD4Percentage &&
		(patient->getMonitoringState()->currObservedCD4Percentage <= simContext->getPedsInputs()->stopARTAfterFailCD4PercentageBound[artLineNum])) {
			return SimContext::ART_STOP_CD4;
	}
	// check if observance of a severe OI should cause failure
	if (stopART.afterFailWithSevereOI) {
		for (int i = 0; i < SimContext::OI_NUM; i++) {
			if ((patient->getARTState()->numObservedOIsSinceFailOrStopART[i] > 0) &&
				(simContext->getRunSpecsInputs()->severeOIs[i])) {
					return SimContext::ART_STOP_SEV_OI;
			}
		}
	}
	// check if maximum numbers of months since failure has exceeded
	if ((stopART.afterFailMonthsFromObserved != SimContext::NOT_APPL) &&
		(patient->getGeneralState()->monthNum - patient->getARTState()->monthOfObservedFailure >= stopART.afterFailMonthsFromObserved)) {
			return SimContext::ART_STOP_FAIL_MTHS;
	}

	return SimContext::ART_STOP_NOT_STOPPED;
} /* evaluateStopARTPolicy */

/* evaluateSTIInitialStopPolicy determines if the ART treatment should be stopped for the
	initial STI interruption */
bool ClinicVisitUpdater::evaluateSTIInitialStopPolicy() {
	int artLineNum = patient->getARTState()->currRegimenNum;
	const SimContext::STIInputs::InitiationPolicy &policy = simContext->getSTIInputs()->firstInterruption[artLineNum];

	// return not stopped if STI is not enabled for this ART regimen
	if (!simContext->getTreatmentInputs()->enableSTIForART[artLineNum])
		return false;

	// return not stopped if we haven't reached the minimum months for first interruption
	if ((policy.minMonthNum != SimContext::NOT_APPL) &&
		(patient->getGeneralState()->monthNum < policy.minMonthNum))
		return false;
	if ((policy.monthsSinceARTStart != SimContext::NOT_APPL) &&
		(patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart < policy.monthsSinceARTStart))
		return false;

	// Evaluate the CD4 only criteria
	double observedCD4 = patient->getMonitoringState()->currObservedCD4;
	if (patient->getMonitoringState()->hasObservedCD4 &&
		(observedCD4 >= policy.CD4BoundsOnly[SimContext::LOWER_BOUND]) &&
		(observedCD4 <= policy.CD4BoundsOnly[SimContext::UPPER_BOUND])) {
			return true;
	}

	// Evaluate the HVL strata only criteria
	SimContext::HVL_STRATA observedHVL = patient->getMonitoringState()->currObservedHVLStrata;
	if (patient->getMonitoringState()->hasObservedHVLStrata &&
		(observedHVL >= policy.HVLBoundsOnly[SimContext::LOWER_BOUND]) &&
		(observedHVL <= policy.HVLBoundsOnly[SimContext::UPPER_BOUND])) {
			return true;
	}

	// Evaluate the CD4 and HVL combined criteria
	if (patient->getMonitoringState()->hasObservedCD4 &&
		(observedCD4 >= policy.CD4BoundsWithHVL[SimContext::LOWER_BOUND]) &&
		(observedCD4 <= policy.CD4BoundsWithHVL[SimContext::UPPER_BOUND]) &&
		patient->getMonitoringState()->hasObservedHVLStrata &&
		(observedHVL >= policy.HVLBoundsWithCD4[SimContext::LOWER_BOUND]) &&
		(observedHVL <= policy.HVLBoundsWithCD4[SimContext::UPPER_BOUND])) {
			return true;
	}

	// Evaluate the acute OIs since last ART only criteria
	int numOIs = 0;
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		if (policy.OIHistory[i]) {
			numOIs += patient->getARTState()->numObservedOIsSinceFailOrStopART[i];
		}
	}
	if (numOIs >= policy.numOIs)
		return true;

	// Evaluate the acute OIs in patient's history and CD4 count criteria
	if ((observedCD4 != SimContext::NOT_APPL) &&
		(observedCD4 >= policy.CD4BoundsWithOIs[SimContext::LOWER_BOUND]) &&
		(observedCD4 <= policy.CD4BoundsWithOIs[SimContext::UPPER_BOUND])) {
			for (int i = 0; i < SimContext::OI_NUM; i++) {
				if (policy.OIHistoryWithCD4[i] && (patient->getMonitoringState()->numObservedOIsTotal[i] > 0)) {
					return true;
				}
			}
	}

	return false;
} /* evaluateSTIInitialStopPolicy */

/* evaluateSTIEndpointPolicy determines then end of the STI cycle and indicates observed failure */
SimContext::ART_FAIL_TYPE ClinicVisitUpdater::evaluateSTIEndpointPolicy() {
	int artLineNum = patient->getARTState()->currRegimenNum;
	const SimContext::STIInputs::EndpointPolicy &policy = simContext->getSTIInputs()->endpoint[artLineNum];

	// return not failed if we haven't reached the minimum months since STI started
	if ((policy.monthsSinceSTIStart != SimContext::NOT_APPL) &&
		(patient->getGeneralState()->monthNum - patient->getARTState()->monthOfSTIInitialStop < policy.monthsSinceSTIStart))
		return SimContext::ART_FAIL_NOT_FAILED;

	// Evaluate the CD4 only criteria
	double observedCD4 = patient->getMonitoringState()->currObservedCD4;
	if (patient->getMonitoringState()->hasObservedCD4 &&
		(observedCD4 >= policy.CD4BoundsOnly[SimContext::LOWER_BOUND]) &&
		(observedCD4 <= policy.CD4BoundsOnly[SimContext::UPPER_BOUND])) {
			return SimContext::ART_FAIL_IMMUNOLOGIC;
	}

	// Evaluate the HVL strata only criteria
	SimContext::HVL_STRATA observedHVL = patient->getMonitoringState()->currObservedHVLStrata;
	if (patient->getMonitoringState()->hasObservedHVLStrata &&
		(observedHVL >= policy.HVLBoundsOnly[SimContext::LOWER_BOUND]) &&
		(observedHVL <= policy.HVLBoundsOnly[SimContext::UPPER_BOUND])) {
			return SimContext::ART_FAIL_VIROLOGIC;
	}

	// Evaluate the CD4 and HVL combined criteria
	if (patient->getMonitoringState()->hasObservedCD4 &&
		(observedCD4 >= policy.CD4BoundsWithHVL[SimContext::LOWER_BOUND]) &&
		(observedCD4 <= policy.CD4BoundsWithHVL[SimContext::UPPER_BOUND]) &&
		patient->getMonitoringState()->hasObservedHVLStrata &&
		(observedHVL >= policy.HVLBoundsWithCD4[SimContext::LOWER_BOUND]) &&
		(observedHVL <= policy.HVLBoundsWithCD4[SimContext::UPPER_BOUND])) {
			return SimContext::ART_FAIL_VIROLOGIC;
	}

	// Evaluate the acute OIs since last ART only criteria
	int numOIs = 0;
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		if (policy.OIHistory[i]) {
			numOIs += patient->getARTState()->numObservedOIsSinceFailOrStopART[i];
		}
	}
	if (numOIs >= policy.numOIs)
		return SimContext::ART_FAIL_CLINICAL;

	// Evaluate the acute OIs in patient's history and CD4 count criteria
	if ((observedCD4 != SimContext::NOT_APPL) &&
		(observedCD4 >= policy.CD4BoundsWithOIs[SimContext::LOWER_BOUND]) &&
		(observedCD4 <= policy.CD4BoundsWithOIs[SimContext::UPPER_BOUND])) {
			for (int i = 0; i < SimContext::OI_NUM; i++) {
				if (policy.OIHistoryWithCD4[i] && (patient->getMonitoringState()->numObservedOIsTotal[i] > 0)) {
					return SimContext::ART_FAIL_CLINICAL;
				}
			}
	}

	return SimContext::ART_FAIL_NOT_FAILED;
} /* evaluateSTIEndpointPolicy */

/* evaluateSTIRestartPolicy determines if ART should be restarted while interrupted */
bool ClinicVisitUpdater::evaluateSTIRestartPolicy() {
	int artLineNum = patient->getARTState()->nextRegimenNum;

	// Evaluate the CD4 only criteria
	double observedCD4 = patient->getMonitoringState()->currObservedCD4;
	if (patient->getMonitoringState()->hasObservedCD4 &&
		(observedCD4 >= simContext->getSTIInputs()->ARTRestartCD4Bounds[artLineNum][SimContext::LOWER_BOUND]) &&
		(observedCD4 <= simContext->getSTIInputs()->ARTRestartCD4Bounds[artLineNum][SimContext::UPPER_BOUND])) {
			return true;
	}

	// Evaluate the HVL strata only criteria
	SimContext::HVL_STRATA observedHVL = patient->getMonitoringState()->currObservedHVLStrata;
	if (patient->getMonitoringState()->hasObservedHVLStrata &&
		(observedHVL >= simContext->getSTIInputs()->ARTRestartHVLBounds[artLineNum][SimContext::LOWER_BOUND]) &&
		(observedHVL <= simContext->getSTIInputs()->ARTRestartHVLBounds[artLineNum][SimContext::UPPER_BOUND])) {
			return true;
	}

	return false;
} /* evaluateSTIRestartPolicy */

/* evaluateSTISubsequentStopPolicy determines if the ART treatment should be stopped for
	subsequent STI interruptions */
bool ClinicVisitUpdater::evaluateSTISubsequentStopPolicy() {
	int artLineNum = patient->getARTState()->nextRegimenNum;

	// Evaluate the CD4 only criteria
	double observedCD4 = patient->getMonitoringState()->currObservedCD4;
	if (patient->getMonitoringState()->hasObservedCD4) {
		if ((simContext->getSTIInputs()->ARTRestopCD4Bounds[artLineNum][SimContext::UPPER_BOUND] != SimContext::NOT_APPL) &&
			(observedCD4 > simContext->getSTIInputs()->ARTRestopCD4Bounds[artLineNum][SimContext::UPPER_BOUND])) {
				return true;
		}
		if ((simContext->getSTIInputs()->ARTRestopCD4Bounds[artLineNum][SimContext::LOWER_BOUND] != SimContext::NOT_APPL) &&
			(observedCD4 < simContext->getSTIInputs()->ARTRestopCD4Bounds[artLineNum][SimContext::LOWER_BOUND])) {
				return true;
		}
	}

	// Evaluate the HVL strata only criteria
	SimContext::HVL_STRATA observedHVL = patient->getMonitoringState()->currObservedHVLStrata;
	if (patient->getMonitoringState()->hasObservedHVLStrata) {
		if ((simContext->getSTIInputs()->ARTRestopHVLBounds[artLineNum][SimContext::UPPER_BOUND] != SimContext::NOT_APPL) &&
			(observedHVL > simContext->getSTIInputs()->ARTRestopHVLBounds[artLineNum][SimContext::UPPER_BOUND])) {
				return true;
		}
		if ((simContext->getSTIInputs()->ARTRestopHVLBounds[artLineNum][SimContext::LOWER_BOUND] != SimContext::NOT_APPL) &&
			(observedHVL < simContext->getSTIInputs()->ARTRestopHVLBounds[artLineNum][SimContext::LOWER_BOUND])) {
				return true;
		}
	}

	return false;
} /* evaluateSTISubsequentStopPolicy */

/* performProphProgramUpdates evaluates prophylaxis policies and alters the treatment program */
void ClinicVisitUpdater::performProphProgramUpdates() {
	// return if prohylaxis are not avaiable to the patient
	if (!patient->getProphState()->mayReceiveProph)
		return;

	// Determine if observed OIs will cause a switch to using secondary prophs and
	//	stopping current primary proph
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		if (patient->getMonitoringState()->numObservedOIsSinceLastVisit[i] > 0) {
			// Skip if patient is already using or is set to use secondary prophs
			if (patient->getProphState()->isOnProph[i] &&
				(patient->getProphState()->currProphType[i] == SimContext::PROPH_SECONDARY))
				continue;
			if (patient->getProphState()->hasNextProphAvailable[i] &&
				(patient->getProphState()->nextProphType[i] == SimContext::PROPH_SECONDARY))
				continue;
			double randNum = CepacUtil::getRandomDouble(60150, patient);
			if (randNum < simContext->getTreatmentInputs()->probSwitchSecondaryProph[i]) {
				if (patient->getProphState()->isOnProph[i]) {
					// Stop the current proph
					SimContext::PROPH_TYPE prophType = patient->getProphState()->currProphType[i];
					int prophNum = patient->getProphState()->currProphNum[i];
					stopCurrProph((SimContext::OI_TYPE) i);
					if (patient->getGeneralState()->tracingEnabled) {
						tracer->printTrace(1, "**%d STOP %s PROPH %d for OI %s;\n", patient->getGeneralState()->monthNum,
							SimContext::PROPH_TYPE_STRS[SimContext::PROPH_PRIMARY],
							prophNum + 1, SimContext::OI_STRS[i]);
					}
				}
				// Set the next proph that is available for use
				bool hasNext = false;
				SimContext::PROPH_TYPE nextProphType = SimContext::PROPH_SECONDARY;
				int nextProphNum = SimContext::NOT_APPL;
				for (int j = 0; j < SimContext::PROPH_NUM; j++) {
					const SimContext::ProphInputs *prophInput = simContext->getProphInputs(nextProphType,i,j);
					if (prophInput) {
						hasNext = true;
						nextProphNum = j;
						break;
					}
				}
				setNextProph(hasNext, nextProphType, (SimContext::OI_TYPE) i, nextProphNum);
			}
		}
	}

	// Handle the appropriate stopping/switching for a toxicity or months to switch reached
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		if (patient->getProphState()->isOnProph[i]) {
			SimContext::PROPH_TYPE prophType = patient->getProphState()->currProphType[i];
			int prophNum = patient->getProphState()->currProphNum[i];
			const SimContext::ProphInputs *prophInput = simContext->getProphInputs(prophType,i,prophNum);
			int monthsOnProph = patient->getGeneralState()->monthNum - patient->getProphState()->monthOfProphStart[i];
			SimContext::PROPH_TOX_TYPE toxType = patient->getProphState()->typeProphToxicity[i];

			// Check for a toxicity causing a switch or reached months to switch proph
			if (((toxType == SimContext::PROPH_TOX_MAJOR) && prophInput->switchOnMajorToxicity) ||
				((toxType == SimContext::PROPH_TOX_MINOR) && prophInput->switchOnMinorToxicity) ||
				((prophInput->monthsToSwitch != SimContext::NOT_APPL) && (monthsOnProph >= prophInput->monthsToSwitch))) {
				// Stop the current proph
				stopCurrProph((SimContext::OI_TYPE) i);
				if (patient->getGeneralState()->tracingEnabled) {
					tracer->printTrace(1, "**%d STOP %s PROPH %d for OI %s;\n", patient->getGeneralState()->monthNum,
						SimContext::PROPH_TYPE_STRS[prophType],
						prophNum + 1, SimContext::OI_STRS[i]);
				}

				// Set the next proph that is available for use
				bool hasNext = false;
				int nextProphNum = SimContext::NOT_APPL;
				for (int j = prophNum + 1; j < SimContext::PROPH_NUM; j++) {
					const SimContext::ProphInputs *nextProphInput = simContext->getProphInputs(prophType,i,j);
					if (nextProphInput) {
						hasNext = true;
						nextProphNum = j;
						break;
					}
				}
				setNextProph(hasNext, prophType, (SimContext::OI_TYPE) i, nextProphNum);

				// If next proph is available, start it now
				if (hasNext) {
					startNextProph((SimContext::OI_TYPE) i);
					if (patient->getGeneralState()->tracingEnabled) {
						tracer->printTrace(1, "**%d SWITCH %s PROPH TO %d for OI %s;\n", patient->getGeneralState()->monthNum,
							SimContext::PROPH_TYPE_STRS[prophType],
							nextProphNum + 1, SimContext::OI_STRS[i]);
					}
				}
			}
		}
	}

	// If on prophs, determine if stopping policy criteria has been met for each OI
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		SimContext::OI_TYPE oiType = (SimContext::OI_TYPE) i;
		if (patient->getProphState()->isOnProph[i]) {
			// Patient is currently on proph for this OI, evaluate stopping policy
			SimContext::PROPH_TYPE prophType = patient->getProphState()->currProphType[oiType];
			if (evaluateStopProphPolicy(prophType, oiType)) {
				// Stop the current proph
				SimContext::PROPH_TYPE prophType = patient->getProphState()->currProphType[i];
				int prophNum = patient->getProphState()->currProphNum[i];
				stopCurrProph(oiType);
				if (patient->getGeneralState()->tracingEnabled) {
					tracer->printTrace(1, "**%d STOP %s PROPH %d for OI %s;\n", patient->getGeneralState()->monthNum,
						SimContext::PROPH_TYPE_STRS[prophType],
						prophNum + 1, SimContext::OI_STRS[i]);
				}
			}
		}
	}

	// If not on prophs, determine if starting policy criteria has been met for each OI
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		SimContext::OI_TYPE oiType = (SimContext::OI_TYPE) i;
		if (!patient->getProphState()->isOnProph[i]) {
			// Patient is currently not on any proph for this OI, evaluate starting policy
			//	and if stopping policy is not also immediately met
			SimContext::PROPH_TYPE prophType = patient->getProphState()->nextProphType[oiType];
			if (evaluateStartProphPolicy(prophType, oiType) && !evaluateStopProphPolicy(prophType, oiType)) {
				// Start a new proph if there is another one available
				startNextProph(oiType);
				if (patient->getGeneralState()->tracingEnabled) {
					tracer->printTrace(1, "**%d START %s PROPH %d for OI %s;\n", patient->getGeneralState()->monthNum,
						SimContext::PROPH_TYPE_STRS[patient->getProphState()->currProphType[i]],
						patient->getProphState()->currProphNum[i] + 1, SimContext::OI_STRS[i]);
				}
			}
		}
	}
} /* end performProphProgramUpdates */

/* evaluateStartProphPolicy determines if the start criteria for the proph has been met */
bool ClinicVisitUpdater::evaluateStartProphPolicy(SimContext::PROPH_TYPE prophType, SimContext::OI_TYPE oiType) {
	// return false if there is not another available proph
	if (!patient->getProphState()->hasNextProphAvailable[oiType])
		return false;

	bool hasPassedOneCriteria = false;
	bool hasFailedOneCriteria = false;
	const SimContext::TreatmentInputs::ProphStartPolicy &prophStart = simContext->getTreatmentInputs()->startProph[prophType][oiType];

	// Evaluate the minimum month for starting critera, return false if it is not met
	int monthNum = patient->getGeneralState()->monthNum;
	if ((prophStart.minMonthNum != SimContext::NOT_APPL) && (monthNum < prophStart.minMonthNum))
		return false;

	// Evaluate the current CD4 level criteria
	if (patient->getMonitoringState()->hasObservedCD4) {
		double currCD4 = patient->getMonitoringState()->currObservedCD4;
		if ((currCD4 >= prophStart.currCD4Bounds[SimContext::LOWER_BOUND]) && (currCD4 <= prophStart.currCD4Bounds[SimContext::UPPER_BOUND])) {
			hasPassedOneCriteria = true;
		}
		else {
			hasFailedOneCriteria = true;
		}
	}

	// Evaluate the minimum CD4 level criteria,
	if (patient->getMonitoringState()->hasObservedCD4) {
		double minCD4 = patient->getMonitoringState()->minObservedCD4;
		if ((minCD4 >= prophStart.minCD4Bounds[SimContext::LOWER_BOUND]) && (minCD4 <= prophStart.minCD4Bounds[SimContext::UPPER_BOUND])) {
			hasPassedOneCriteria = true;
		}
		else {
			hasFailedOneCriteria = true;
		}
	}

	// Evaluate the OI history criteria, skip if all the inputs are unspecified
	bool useHistory = false;
	bool hasPassedOneOIHist = false;
	bool useNoHistory = false;
	bool hasFailedOneOIHist = false;
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		if (prophStart.OIHistory[i] == 1) {
			useHistory = true;
			if (patient->getMonitoringState()->numObservedOIsTotal[i] > 0)
				hasPassedOneOIHist = true;
		}
		else if (prophStart.OIHistory[i] == 0) {
			useNoHistory = true;
			if (patient->getMonitoringState()->numObservedOIsTotal[i] > 0)
				hasFailedOneOIHist = true;
		}
	}
	if (useHistory && useNoHistory) {
		if (hasPassedOneOIHist && !hasFailedOneOIHist)
			hasPassedOneCriteria = true;
		else
			hasFailedOneCriteria = true;
	}
	else if (useHistory) {
		if (hasPassedOneOIHist)
			hasPassedOneCriteria = true;
		else
			hasFailedOneCriteria = true;
	}
	else if (useNoHistory) {
		if (!hasFailedOneOIHist)
			hasPassedOneCriteria = true;
		else
			hasFailedOneCriteria = true;
	}

	// return true if using or evaluation and at least one criteria has been met,
	//	return true if using and evaluation and at least one criteria has been met and none have failed,
	//	return false otherwise
	if (prophStart.useOrEvaluation && hasPassedOneCriteria)
		return true;
	if (!prophStart.useOrEvaluation && hasPassedOneCriteria && !hasFailedOneCriteria)
		return true;
	return false;
} /* end evaluateStartProphPolicy */

/* evaluateStopProphPolicy determines if the stopping criteria for the proph has been met */
bool ClinicVisitUpdater::evaluateStopProphPolicy(SimContext::PROPH_TYPE prophType, SimContext::OI_TYPE oiType) {
	bool hasPassedOneCriteria = false;
	bool hasFailedOneCriteria = false;
	const SimContext::TreatmentInputs::ProphStopPolicy &prophStop = simContext->getTreatmentInputs()->stopProph[prophType][oiType];

	// Evaluate the minimum month # and months on proph stopping criteria, return true if they are met
	int monthNum = patient->getGeneralState()->monthNum;
	if ((prophStop.minMonthNum != SimContext::NOT_APPL) && (monthNum >= prophStop.minMonthNum))
		return true;
	if (patient->getProphState()->isOnProph[oiType]) {
		int monthsOnProph = monthNum - patient->getProphState()->monthOfProphStart[oiType];
		if ((prophStop.monthsOnProph != SimContext::NOT_APPL) && (monthsOnProph >= prophStop.monthsOnProph))
			return true;
	}

	// Evaluate the current CD4 level criteria
	if (patient->getMonitoringState()->hasObservedCD4) {
		double currCD4 = patient->getMonitoringState()->currObservedCD4;
		if ((currCD4 > prophStop.currCD4Bounds[SimContext::UPPER_BOUND]) || (currCD4 < prophStop.currCD4Bounds[SimContext::LOWER_BOUND])) {
			hasPassedOneCriteria = true;
		}
		else {
			hasFailedOneCriteria = true;
		}
	}

	// Evaluate the minimum CD4 level criteria
	if (patient->getMonitoringState()->hasObservedCD4) {
		double minCD4 = patient->getMonitoringState()->minObservedCD4;
		if ((minCD4 > prophStop.minCD4Bounds[SimContext::UPPER_BOUND]) || (minCD4 < prophStop.minCD4Bounds[SimContext::LOWER_BOUND])) {
			hasPassedOneCriteria = true;
		}
		else {
			hasFailedOneCriteria = true;
		}
	}

	// Evaluate the OI history criteria, skip if all the inputs are unspecified
	bool useHistory = false;
	bool hasPassedOneOIHist = false;
	bool useNoHistory = false;
	bool hasFailedOneOIHist = false;
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		if (prophStop.OIHistory[i] == 1) {
			useHistory = true;
			if (patient->getMonitoringState()->numObservedOIsTotal[i] > 0)
				hasPassedOneOIHist = true;
		}
		else if (prophStop.OIHistory[i] == 0) {
			useNoHistory = true;
			if (patient->getMonitoringState()->numObservedOIsTotal[i] > 0)
				hasFailedOneOIHist = true;
		}
	}
	if (useHistory && useNoHistory) {
		if (hasPassedOneOIHist && !hasFailedOneOIHist)
			hasPassedOneCriteria = true;
		else
			hasFailedOneCriteria = true;
	}
	else if (useHistory) {
		if (hasPassedOneOIHist)
			hasPassedOneCriteria = true;
		else
			hasFailedOneCriteria = true;
	}
	else if (useNoHistory) {
		if (!hasFailedOneOIHist)
			hasPassedOneCriteria = true;
		else
			hasFailedOneCriteria = true;
	}

	// return true if using or evaluation and at least one criteria has been met,
	//	return true if using and evaluation and at least one criteria has been met and none have failed,
	//	return false otherwise
	if (prophStop.useOrEvaluation && hasPassedOneCriteria)
		return true;
	if (!prophStop.useOrEvaluation && hasPassedOneCriteria && !hasFailedOneCriteria)
		return true;
	return false;
} /* evaluateStopProphPolicy */

/* performTBProphProgramUpdates evaluates TB proph policies and alters the treatment program */
void ClinicVisitUpdater::performTBProphProgramUpdates() {
	if (!patient->getProphState()->mayReceiveProph)
		return;

	// If a major proph toxicity occurred, stop the current TB proph
	if (patient->getTBState()->isOnProph && patient->getTBState()->hasMajorProphToxicity) {
		// Stop the TB proph
		int currProph = patient->getTBState()->currProphNum;
		stopCurrTBProph();

		// Output tracing if enabled
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d STOP TB PROPH %d;\n",
				patient->getGeneralState()->monthNum, currProph + 1);
		}

		// Set the next TB proph that is available for use
		bool hasNext = false;
		int nextProph = SimContext::NOT_APPL;
		for (int i = currProph + 1; i < SimContext::PROPH_NUM; i++) {
			if (simContext->getTBInputs()->tbProphInputs[i]) {
				hasNext = true;
				nextProph = i;
				break;
			}
		}
		setNextTBProph(hasNext, nextProph);
	}

	// If patient is on proph, evaluate if they should stop and update if so
	if (patient->getTBState()->isOnProph) {
		if (evaluateStopTBProphPolicy()) {
			// Stop the TB proph
			int currProph = patient->getTBState()->currProphNum;
			stopCurrTBProph();

			// Output tracing if enabled
			if (patient->getGeneralState()->tracingEnabled) {
				tracer->printTrace(1, "**%d STOP TB PROPH %d;\n",
					patient->getGeneralState()->monthNum, currProph + 1);
			}
		}
	}
	else {
		// If proph available and patient is not on proph or scheduled, evaluate whether to start and lag time
		if (patient->getTBState()->hasNextProphAvailable && !patient->getTBState()->isScheduledForProph) {
			if (evaluateStartTBProphPolicy()) {
				// Roll for prob of receiving proph
				double probProph = 0.0;
				if (patient->getARTState()->isOnART)
					probProph = simContext->getTBInputs()->probReceiveProphOnART;
				else
					probProph = simContext->getTBInputs()->probReceiveProphOffART;
				double randNum = CepacUtil::getRandomDouble(60160, patient);
				if (randNum < probProph) {
					double lagTimeMean = simContext->getTBInputs()->monthsLagToStartProphMean;
					double lagTimeStdDev = simContext->getTBInputs()->monthsLagToStartProphStdDev;
					int lagTime = (int) (CepacUtil::getRandomGaussian(lagTimeMean, lagTimeStdDev, 60170, patient) + 0.5);
					scheduleNextTBProph(patient->getGeneralState()->monthNum + lagTime);

					// Output tracing if enabled
					if (patient->getGeneralState()->tracingEnabled) {
						tracer->printTrace(1, "**%d LAG to TB proph, %d months;\n",
							patient->getGeneralState()->monthNum, lagTime);
					}
				}
			}
		}
	}

	// If acute TB occurs and proph is scheduled for a future time, cancel it
	if (patient->getTBState()->isScheduledForProph &&
		patient->getDiseaseState()->hasCurrTrueOI &&
		(patient->getDiseaseState()->typeCurrTrueOI == SimContext::OI_TB)) {
			unscheduleNextTBProph();
	}

	// Start proph if scheduled to do so and reached month of schedule
	if (patient->getTBState()->isScheduledForProph &&
		(patient->getGeneralState()->monthNum >= patient->getTBState()->monthOfProphStart)) {
			startNextTBProph();

			// Output tracing if enabled
			if (patient->getGeneralState()->tracingEnabled) {
				tracer->printTrace(1, "**%d START TB PROPH %d;\n",
					patient->getGeneralState()->monthNum,
					patient->getTBState()->currProphNum + 1);
			}
	}
} /* end performTBProphProgramUpdates */

/* evaluateStartTBProphPolicy determines if the start criteria for TB proph has been met */
bool ClinicVisitUpdater::evaluateStartTBProphPolicy() {
	// return false if there is not another available proph
	if (patient->getTBState()->nextProphNum == SimContext::NOT_APPL)
		return false;
	// return false if TB is active or on treatment
	if ((patient->getTBState()->currTrueTBDiseaseState == SimContext::TB_STATE_ACTIVE) ||
		(patient->getTBState()->isOnTreatment))
		return false;

	bool hasPassedOneCriteria = false;
	bool hasFailedOneCriteria = false;

	// Evaluate the current observed CD4 criteria
	if (patient->getMonitoringState()->hasObservedCD4) {
		double currCD4 = patient->getMonitoringState()->currObservedCD4;
		if ((simContext->getTBInputs()->startProphCurrentCD4Bounds[SimContext::LOWER_BOUND] != SimContext::NOT_APPL) ||
			(simContext->getTBInputs()->startProphCurrentCD4Bounds[SimContext::UPPER_BOUND] != SimContext::NOT_APPL)) {
				if ((currCD4 >= simContext->getTBInputs()->startProphCurrentCD4Bounds[SimContext::LOWER_BOUND]) &&
					(currCD4 <= simContext->getTBInputs()->startProphCurrentCD4Bounds[SimContext::UPPER_BOUND])) {
					hasPassedOneCriteria = true;
				}
				else {
					hasFailedOneCriteria = true;
				}
		}
	}

	// Evaluate the minimum observed CD4 criteria
	if (patient->getMonitoringState()->hasObservedCD4) {
		double minCD4 = patient->getMonitoringState()->minObservedCD4;
		if ((simContext->getTBInputs()->startProphMinCD4Bounds[SimContext::LOWER_BOUND] != SimContext::NOT_APPL) ||
			(simContext->getTBInputs()->startProphMinCD4Bounds[SimContext::UPPER_BOUND] != SimContext::NOT_APPL)) {
				if ((minCD4 >= simContext->getTBInputs()->startProphMinCD4Bounds[SimContext::LOWER_BOUND]) &&
					(minCD4 <= simContext->getTBInputs()->startProphMinCD4Bounds[SimContext::UPPER_BOUND])) {
					hasPassedOneCriteria = true;
				}
				else {
					hasFailedOneCriteria = true;
				}
		}
	}

	// Evaluate history of active TB
	if (simContext->getTBInputs()->startProphKnownActiveHistory != SimContext::NOT_APPL) {
		if (patient->getTBState()->hasObservedHistoryActiveTB) {
			hasPassedOneCriteria = true;
		}
		else {
			hasFailedOneCriteria = true;
		}
	}

	// Evaluate ART initiation criteria
	if (simContext->getTBInputs()->startProphAtARTInitiation != SimContext::NOT_APPL) {
		if (patient->getARTState()->isOnART) {
			hasPassedOneCriteria = true;
		}
		else {
			hasFailedOneCriteria = true;
		}
	}

	// return true if using or evaluation and at least one criteria has been met,
	//	return true if using and evaluation and at least one criteria has been met and none have failed,
	//	return false otherwise
	if (simContext->getTBInputs()->startProphUseOrEvaluation && hasPassedOneCriteria)
		return true;
	if (!simContext->getTBInputs()->startProphUseOrEvaluation && hasPassedOneCriteria && !hasFailedOneCriteria)
		return true;
	return false;
} /* end evaluateStartTBProphPolicy */

/* evaluateStopTBProphPolicy determines if the sopping criteria for TB proph has been met */
bool ClinicVisitUpdater::evaluateStopTBProphPolicy() {
	// return true if patient has acute TB this month, stop proph
	if (patient->getDiseaseState()->hasCurrTrueOI &&
		(patient->getDiseaseState()->typeCurrTrueOI == SimContext::OI_TB))
		return true;

	bool hasPassedOneCriteria = false;
	bool hasFailedOneCriteria = false;

	// Evaluate the current observed CD4 criteria,
	//	return true if within range and using OR evalatuation,
	//	return false if outside range and using AND evaluation
	if (patient->getMonitoringState()->hasObservedCD4) {
		double currCD4 = patient->getMonitoringState()->currObservedCD4;
		if ((simContext->getTBInputs()->stopProphCurrentCD4Bounds[SimContext::LOWER_BOUND] != SimContext::NOT_APPL) ||
			(simContext->getTBInputs()->stopProphCurrentCD4Bounds[SimContext::UPPER_BOUND] != SimContext::NOT_APPL)) {
				if ((currCD4 >= simContext->getTBInputs()->stopProphCurrentCD4Bounds[SimContext::LOWER_BOUND]) &&
					(currCD4 <= simContext->getTBInputs()->stopProphCurrentCD4Bounds[SimContext::UPPER_BOUND])) {
					hasPassedOneCriteria = true;
				}
				else {
					hasFailedOneCriteria = true;
				}
		}
	}

	// Evaluate ART initiation criteria,
	//	return true if on ART and using OR evaluation,
	//	return false if not and using AND evaluation
	if (simContext->getTBInputs()->stopProphAtARTInitiation != SimContext::NOT_APPL) {
		if (patient->getARTState()->isOnART) {
			hasPassedOneCriteria = true;
		}
		else {
			hasFailedOneCriteria = true;
		}
	}

	// Evaluate months on proph criteria,
	//	return true if on ART and using OR evaluation,
	//	return false if not and using AND evaluation
	if (simContext->getTBInputs()->stopProphAtARTInitiation != SimContext::NOT_APPL) {
		int monthsOnProph = patient->getGeneralState()->monthNum - patient->getTBState()->monthOfProphStart;
		if (monthsOnProph >= simContext->getTBInputs()->stopProphNumMonths) {
			hasPassedOneCriteria = true;
		}
		else {
			hasFailedOneCriteria = true;
		}
	}

	// return true if using or evaluation and at least one criteria has been met,
	//	return true if using and evaluation and at least one criteria has been met and none have failed,
	//	return false otherwise
	if (simContext->getTBInputs()->stopProphUseOrEvaluation && hasPassedOneCriteria)
		return true;
	if (!simContext->getTBInputs()->stopProphUseOrEvaluation && hasPassedOneCriteria && !hasFailedOneCriteria)
		return true;
	return false;
} /* end evaluateStopTBProphPolicy */

/* performTBTreatmentProgramUpdates evaluates TB treatment policies and alters the treatment program */
void ClinicVisitUpdater::performTBTreatmentProgramUpdates() {
	// Evaluate new treatment if patient has acute TB and is not yet scheduled for treatment, or
	//	if patient has active TB and this is the first visit
	bool treatmentEligible = false;
	if (patient->getDiseaseState()->hasCurrTrueOI &&
		(patient->getDiseaseState()->typeCurrTrueOI == SimContext::OI_TB) &&
		!patient->getTBState()->isOnTreatment &&
		!patient->getTBState()->isScheduledForTreatment)
			treatmentEligible = true;
	else if (!patient->getMonitoringState()->hadPrevClinicVisit &&
		(patient->getTBState()->currTrueTBDiseaseState == SimContext::TB_STATE_ACTIVE))
			treatmentEligible = true;
	if (treatmentEligible) {
			// Roll for whether or not a next TB treatment should be schedule
			double probTreat = 0.0;
			if (patient->getARTState()->isOnART)
				probTreat = simContext->getTBInputs()->probReceiveTreatmentOnART;
			else
				probTreat = simContext->getTBInputs()->probReceiveTreatmentOffART;
			double randNum = CepacUtil::getRandomDouble(60180, patient);
			if (randNum < probTreat) {
				// Roll for the next stage of treatment
				SimContext::TB_STRAIN tbStrain = patient->getTBState()->currTrueTBResistanceStrain;
				int lineNum = 0;
				SimContext::TB_TREATM_STAGE treatStage = SimContext::TB_TREATM_STAGE_1_NEW;
				randNum = CepacUtil::getRandomDouble(60190, patient);
				for (int i = 0; i < SimContext::TB_TREATM_LINES_NUM; i++) {
					if (randNum < simContext->getTBInputs()->probInitialTreatmentLine[tbStrain][i]) {
						lineNum = i;
						break;
					}
					randNum -= simContext->getTBInputs()->probInitialTreatmentLine[tbStrain][i];
				}
				if (lineNum == 1)
					treatStage = SimContext::TB_TREATM_STAGE_2;
				else if (lineNum == 2)
					treatStage = SimContext::TB_TREATM_STAGE_3;

				// Determine time lag to treatment start
				double timeLagMean = simContext->getTBInputs()->monthsLagToStartTreatmentMean[treatStage];
				double timeLagStdDev = simContext->getTBInputs()->monthsLagToStartTreatmentStdDev[treatStage];
				int timeLag = (int) (CepacUtil::getRandomGaussian(timeLagMean, timeLagStdDev, 60200, patient) + 0.5);
				if (timeLag < 0)
					timeLag = 0;

				// Set the next TB treatment stage to begin with the give lag
				scheduleNextTBTreatment(treatStage, patient->getGeneralState()->monthNum + timeLag);

				// Output tracing if enabled
				if (patient->getGeneralState()->tracingEnabled) {
					tracer->printTrace(1, "**%d LAG TO TB TREATMENT %s IN %d MTHS;\n",
						patient->getGeneralState()->monthNum,
						SimContext::TB_TREATM_STAGE_STRS[treatStage], timeLag);
				}
			}

			// Increment costs for acute TB here, need to determine treatment first
			if (patient->getTBState()->isScheduledForTreatment) {
				SimContext::TB_TREATM_STAGE treatStage = patient->getTBState()->nextTreatmentStage;
				const double *costArray = simContext->getTBInputs()->acuteActiveTreatedCosts[treatStage];
				double costMultiplier = simContext->getTBInputs()->acuteActiveTreatedARTMultiplier[treatStage];
				incrementCostsMisc(costArray, costMultiplier);
			}
			else {
				const double *costArray = simContext->getTBInputs()->acuteUntreatedCosts;
				incrementCostsMisc(costArray, 1.0);
			}
	}

	// Evaluate treatment changes if intended duration has been exceeded
	if (patient->getTBState()->isOnTreatment) {
		int monthsTreat = patient->getGeneralState()->monthNum - patient->getTBState()->monthOfTreatmentStart;
		SimContext::TB_TREATM_STAGE treatStage = patient->getTBState()->currTreatmentStage;
		SimContext::TB_STRAIN tbStrain = patient->getTBState()->currTrueTBResistanceStrain;
		if (monthsTreat >= simContext->getTBInputs()->monthsTreatmentDuration[treatStage]) {
			// If treatment succeeded, set to history of active state
			if (patient->getTBState()->currTrueTBDiseaseState == SimContext::TB_STATE_TREATM_SUCC) {
				stopCurrTBTreatment(true, true);
				setTBDiseaseState(SimContext::TB_STATE_HIST_ACTV);

				// Output tracing if enabled
				if (patient->getGeneralState()->tracingEnabled) {
					tracer->printTrace(1, "**%d TB TREAT SUCCESS %s;\n",
						patient->getGeneralState()->monthNum, SimContext::TB_TREATM_STAGE_STRS[treatStage]);
				}
			}

			// If treatment failed, return to active state and determine the next treatment step
			if (patient->getTBState()->currTrueTBDiseaseState == SimContext::TB_STATE_TREATM_FAILING) {
				// Stop current treatment, update state and statistics
				stopCurrTBTreatment(true, false);
				setTBDiseaseState(SimContext::TB_STATE_ACTIVE);

				// Output tracing if enabled
				if (patient->getGeneralState()->tracingEnabled) {
					tracer->printTrace(1, "**%d TB TREAT FAILED %s;\n",
						patient->getGeneralState()->monthNum, SimContext::TB_TREATM_STAGE_STRS[treatStage]);
				}

				// Roll for developing increased resistance from failed treatment
				double randNum = CepacUtil::getRandomDouble(60210, patient);
				if ((randNum < simContext->getTBInputs()->probIncreasedResistanceNotCured[treatStage]) &&
					(tbStrain < SimContext::TB_STRAIN_XDR)) {
						increaseTBDrugResistance(true);
						SimContext::TB_STRAIN newTBStrain = patient->getTBState()->currTrueTBResistanceStrain;
						if (patient->getGeneralState()->tracingEnabled) {
							tracer->printTrace(1, "**%d TB TREAT INCR RESIST %s;\n",
								patient->getGeneralState()->monthNum, SimContext::TB_STRAIN_STRS[newTBStrain]);
						}
				}

				// Roll for the next subsequent course of treatment
				SimContext::TB_TREATM_STAGE newTreatStage;
				bool contTreat = false;
				randNum = CepacUtil::getRandomDouble(60220, patient);
				if ((simContext->getTBInputs()->probRepeatTreatmentAfterFailure[treatStage] > 0) &&
					(randNum < simContext->getTBInputs()->probRepeatTreatmentAfterFailure[treatStage])) {
						// Repeat line 1 if using first line 1
						if (treatStage == SimContext::TB_TREATM_STAGE_1_NEW) {
							contTreat = true;
							newTreatStage = SimContext::TB_TREATM_STAGE_1_RPT;
						}
				}
				else {
					randNum -= simContext->getTBInputs()->probRepeatTreatmentAfterFailure[treatStage];
					if ((simContext->getTBInputs()->probNextTreatmentAfterFailure[treatStage] > 0) &&
						(randNum < simContext->getTBInputs()->probNextTreatmentAfterFailure[treatStage])) {
							// Repeat line 1 from new line 1, go onto next line otherwise
							if ((treatStage == SimContext::TB_TREATM_STAGE_1_NEW)) {
								contTreat = true;
								newTreatStage = SimContext::TB_TREATM_STAGE_1_RPT;
							}
							else if (treatStage == SimContext::TB_TREATM_STAGE_1_RPT) {
								contTreat = true;
								newTreatStage = SimContext::TB_TREATM_STAGE_2;
							}
							else if (treatStage == SimContext::TB_TREATM_STAGE_2) {
								contTreat = true;
								newTreatStage = SimContext::TB_TREATM_STAGE_3;
							}
					}
					else {
						randNum -= simContext->getTBInputs()->probNextTreatmentAfterFailure[treatStage];
						if ((simContext->getTBInputs()->probSkipTreatmentAfterFailure[treatStage] > 0) &&
							(randNum < simContext->getTBInputs()->probSkipTreatmentAfterFailure[treatStage])) {
								// Skip to line 3 from either line 1 new or repeat
								if ((treatStage == SimContext::TB_TREATM_STAGE_1_NEW) || (treatStage == SimContext::TB_TREATM_STAGE_1_RPT)) {
									contTreat = true;
									newTreatStage = SimContext::TB_TREATM_STAGE_3;
								}
						}
					}
				}

				// If additional treatments will occur, calculate lag to start and schedule it
				if (contTreat) {
					// Determine time lag to treatment start
					double timeLagMean = simContext->getTBInputs()->monthsLagToStartTreatmentMean[newTreatStage];
					double timeLagStdDev = simContext->getTBInputs()->monthsLagToStartTreatmentStdDev[newTreatStage];
					int timeLag = (int) (CepacUtil::getRandomGaussian(timeLagMean, timeLagStdDev, 60230, patient) + 0.5);
					if (timeLag < 0)
						timeLag = 0;

					// Schedule the next treatment
					scheduleNextTBTreatment(newTreatStage, patient->getGeneralState()->monthNum + timeLag);

					// Output tracing if enabled
					if (patient->getGeneralState()->tracingEnabled) {
						tracer->printTrace(1, "**%d LAG TO TB TREATMENT %s IN %d MTHS;\n",
							patient->getGeneralState()->monthNum,
							SimContext::TB_TREATM_STAGE_STRS[newTreatStage], timeLag);
					}
				}
				else {
					// If no more treatments will occur, output tracing if enabled
					if (patient->getGeneralState()->tracingEnabled) {
						tracer->printTrace(1, "**%d TB TREAT STOP;\n", patient->getGeneralState()->monthNum);
					}
				}
			}
		}
	}

	// Start patient on treatment if they still have active TB are scheduled for it this month
	if ((patient->getTBState()->currTrueTBDiseaseState == SimContext::TB_STATE_ACTIVE) &&
		patient->getTBState()->isScheduledForTreatment &&
		(patient->getGeneralState()->monthNum >= patient->getTBState()->monthOfTreatmentStart)) {
			startNextTBTreatment();

			// Roll for and update the destined outcome of the treatment
			SimContext::TB_TREATM_STAGE treatStage = patient->getTBState()->currTreatmentStage;
			SimContext::TB_STRAIN tbStrain = patient->getTBState()->currTrueTBResistanceStrain;
			double randNum = CepacUtil::getRandomDouble(60240, patient);
			if (randNum < simContext->getTBInputs()->probCuredAfterTreatment[tbStrain][treatStage]) {
				setTBDiseaseState(SimContext::TB_STATE_TREATM_SUCC);
			}
			else {
				setTBDiseaseState(SimContext::TB_STATE_TREATM_FAILING);
			}

			// Output tracing if enabled
			if (patient->getGeneralState()->tracingEnabled) {
				tracer->printTrace(1, "**%d START TB TREATMENT %s(%s);\n",
					patient->getGeneralState()->monthNum, SimContext::TB_TREATM_STAGE_STRS[treatStage],
					(patient->getTBState()->currTrueTBDiseaseState == SimContext::TB_STATE_TREATM_SUCC) ? "succ" : "fail");
			}
	}
} /* end performTBTreatmentProgramUpdates */

