#include "include.h"

/* Constructor takes in the patient as a pointer */
HIVTestingUpdater::HIVTestingUpdater(Patient *patient) : StateUpdater(patient) {

}

/* Destructor is empty, no cleanup required */
HIVTestingUpdater::~HIVTestingUpdater() {

}

/* performInitialUpdates perform all of the state and statistics updates upon patient creation */
void HIVTestingUpdater::performInitialUpdates() {
	// First call the parent function to perform general updates and initialization
	StateUpdater::performInitialUpdates();

	// If not using the testing module, set all HIV positive patients to detected and return
	if (!simContext->getHIVTestInputs()->enableHIVTesting) {
		if (patient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_NEG)
			setDetectedHIVState(false);
		else
			setDetectedHIVState(true, SimContext::HIV_DET_INITIAL);
		return;
	}

	// If using the testing module, determine if patient should enter the model detected
	SimContext::HIV_INF infectState = patient->getDiseaseState()->infectedHIVState;
	double randNum = CepacUtil::getRandomDouble(100010, patient);
	if (randNum < simContext->getHIVTestInputs()->probHIVDetectionInitial[infectState]) {
		setDetectedHIVState(true, SimContext::HIV_DET_INITIAL);
	}
	else {
		setDetectedHIVState(false);
	}

	// If patient is not detected as positive, initialize the HIV testing freq and acceptance rate
	if (!patient->getMonitoringState()->isDetectedHIVPositive) {
		// Default HIV testing parameters to NOT_APPL
		int intervalTestIndex = 0;
		int acceptanceRateIndex = 0;
		bool hasNextTest = false;
		int monthNextTest = SimContext::NOT_APPL;
		if (simContext->getHIVTestInputs()->HIVTestAvailable) {
			// Set the HIV testing interval and time for next test
			double randNum = CepacUtil::getRandomDouble(100020, patient);
			for (int i = 0; i < SimContext::HIV_TEST_FREQ_NUM; i++) {
				if ((randNum < simContext->getHIVTestInputs()->HIVTestingProbability[i]) &&
					(simContext->getHIVTestInputs()->HIVTestingProbability[i] > 0)) {
						if (simContext->getHIVTestInputs()->HIVTestingInterval[i] > 0) {
							hasNextTest = true;
							monthNextTest = 0;
							intervalTestIndex = i;
						}
						break;
				}
				randNum -= simContext->getHIVTestInputs()->HIVTestingProbability[i];
			}

			// Set the HIV testing acceptance rate
			randNum = CepacUtil::getRandomDouble(100030, patient);
			SimContext::HIV_EXT_INF extInfectedState;
			if (!patient->getMonitoringState()->isHighRiskForHIV && (patient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_NEG)) {
				extInfectedState = SimContext::HIV_EXT_INF_NEG_LO;
			}
			else {
				extInfectedState = (SimContext::HIV_EXT_INF) patient->getDiseaseState()->infectedHIVState;
			}
			for (int i = 0; i < SimContext::TEST_ACCEPT_NUM; i++) {
				if ((randNum < simContext->getHIVTestInputs()->HIVTestAcceptDistribution[extInfectedState][i]) &&
					(simContext->getHIVTestInputs()->HIVTestAcceptDistribution[extInfectedState][i] > 0)) {
						acceptanceRateIndex = i;
						break;
				}
				randNum -= simContext->getHIVTestInputs()->HIVTestAcceptDistribution[extInfectedState][i];
			}
		}
		setHIVTestingParams(intervalTestIndex, acceptanceRateIndex);
		scheduleHIVTest(hasNextTest, monthNextTest);
	}
} /* end performInitialUpdates */

/* changes the inputs the updater uses to determine disease progression -- to be used primarily by the transmission model
 * changes the testing frequency and acceptance rate based on the testing strategy of the new simContext */
void HIVTestingUpdater::setSimContext(SimContext *newSimContext){
	StateUpdater::setSimContext(newSimContext);

	//Only update the testing strategy if there is a strategy to update and the patient is undetected
	if (this->simContext->getHIVTestInputs()->enableHIVTesting && !patient->getMonitoringState()->isDetectedHIVPositive){
		// Default HIV testing parameters to NOT_APPL
		int intervalTestIndex = 0;
		int acceptanceRateIndex = 0;
		bool hasNextTest = patient->getMonitoringState()->hasScheduledHIVTest;
		int monthNextTest = patient->getMonitoringState()->monthOfScheduledHIVTest;
		if (simContext->getHIVTestInputs()->HIVTestAvailable) {
			// Set the HIV testing interval and time for next test
			double randNum = CepacUtil::getRandomDouble(100020, patient);
			for (int i = 0; i < SimContext::HIV_TEST_FREQ_NUM; i++) {
				if ((randNum < simContext->getHIVTestInputs()->HIVTestingProbability[i]) &&
					(simContext->getHIVTestInputs()->HIVTestingProbability[i] > 0)) {
						if (simContext->getHIVTestInputs()->HIVTestingInterval[i] > 0) {
							hasNextTest = true;
							intervalTestIndex = i;
							//If we're turning testing on for this patient, set monthNextTest to be randomly determined between current month and currMonth+testingInterval
							//0.5 is added because casting as int just takes the floor... adding 0.5 causes correct rounding
							monthNextTest = patient->getGeneralState()->monthNum + ((int) (CepacUtil::getRandomDouble(100025, patient) * simContext->getHIVTestInputs()->HIVTestingInterval[i] + 0.5));
						}
						break;
				}
				randNum -= simContext->getHIVTestInputs()->HIVTestingProbability[i];
			}

			// Set the HIV testing acceptance rate
			randNum = CepacUtil::getRandomDouble(100030, patient);
			SimContext::HIV_EXT_INF extInfectedState;
			if (!patient->getMonitoringState()->isHighRiskForHIV && (patient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_NEG)) {
				extInfectedState = SimContext::HIV_EXT_INF_NEG_LO;
			}
			else {
				extInfectedState = (SimContext::HIV_EXT_INF) patient->getDiseaseState()->infectedHIVState;
			}
			for (int i = 0; i < SimContext::TEST_ACCEPT_NUM; i++) {
				if ((randNum < simContext->getHIVTestInputs()->HIVTestAcceptDistribution[extInfectedState][i]) &&
					(simContext->getHIVTestInputs()->HIVTestAcceptDistribution[extInfectedState][i] > 0)) {
						acceptanceRateIndex = i;
						break;
				}
				randNum -= simContext->getHIVTestInputs()->HIVTestAcceptDistribution[extInfectedState][i];
			}
		}
		setHIVTestingParams(intervalTestIndex, acceptanceRateIndex);
		scheduleHIVTest(hasNextTest, monthNextTest);
	}
}

/* performMonthlyUpdates perform all of the state and statistics updates for a simulated month */
void HIVTestingUpdater::performMonthlyUpdates() {
	if (patient->getMonitoringState()->isDetectedHIVPositive)
		return;

	// perform the regular and background HIV screenings if patient is not already detected as HIV positive
	performRegularScreeningUpdates();
	if (!patient->getMonitoringState()->isDetectedHIVPositive) {
		performBackgroundScreeningUpdates();
	}
} /* end performMonthlyUpdates */

/* performRegularScreeningUpdates determines if a screening occurs, if its accepted, if
	they are detected, and updates the associated state and statistics */
void HIVTestingUpdater::performRegularScreeningUpdates() {
	// Return if this is not the month of the next test
	if (!patient->getMonitoringState()->hasScheduledHIVTest ||
		(patient->getGeneralState()->monthNum < patient->getMonitoringState()->monthOfScheduledHIVTest))
			return;

	SimContext::HIV_INF infectedState = patient->getDiseaseState()->infectedHIVState;
	SimContext::HIV_EXT_INF extInfectedState;
	if (!patient->getMonitoringState()->isHighRiskForHIV && (infectedState == SimContext::HIV_INF_NEG)) {
		extInfectedState = SimContext::HIV_EXT_INF_NEG_LO;
	}
	else {
		extInfectedState = (SimContext::HIV_EXT_INF) patient->getDiseaseState()->infectedHIVState;
	}

	// Accrue the initial startup cost for all patients, regardless of test acceptance
	//ERINWASHERE
	if (patient->getGeneralState()->monthNum == patient->getGeneralState()->initialMonthNum) {
	//if (patient->getGeneralState()->monthNum == 0) {
		double cost = simContext->getHIVTestInputs()->HIVTestInitialCost[extInfectedState];
		incrementCostsHIVTest(cost);
		// Output tracing if enabled
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d HIV SCREENING STARTUP, $ %1.0lf\n",
				patient->getGeneralState()->monthNum, patient->getGeneralState()->costsDiscounted);
		}
	}

	// Check if scheduled testing is accepted by patient
	double randNum = CepacUtil::getRandomDouble(100040, patient);
	if (randNum < patient->getMonitoringState()->acceptanceRateHIVTest) {
		// Test is accepted, accrue the cost of performing the test
		double cost = simContext->getHIVTestInputs()->HIVTestCost[extInfectedState];
		incrementCostsHIVTest(cost);

		// Check if patient returns for test result
		randNum = CepacUtil::getRandomDouble(100050, patient);
		if (randNum < simContext->getHIVTestInputs()->HIVTestReturnRate[infectedState]) {
			// Determine the test result
			randNum = CepacUtil::getRandomDouble(100060, patient);
			if (randNum < simContext->getHIVTestInputs()->HIVTestPositiveRate[infectedState]) {
				// Increment cost, set QOL, and update identified state for a positive test result
				cost = simContext->getHIVTestInputs()->HIVTestPositiveCost[infectedState];
				incrementCostsHIVMisc(cost);
				if (infectedState != SimContext::HIV_INF_NEG) {
					setDetectedHIVState(true, SimContext::HIV_DET_SCREENING);
					scheduleInitialClinicVisit();
					cost = simContext->getHIVTestInputs()->HIVTestDetectionCost[infectedState];
					incrementCostsHIVMisc(cost);
				}
				accumulateQOLMultiplier(simContext->getHIVTestInputs()->HIVTestPositiveQOLMultiplier[infectedState]);

				// Update statistics for the accepted and positive HIV test result
				updateHIVTestingStats(true, true, true);

				// Output tracing if enabled
				if (patient->getGeneralState()->tracingEnabled) {
					tracer->printTrace(1, "**%d HIV TEST ACCEPT, RETURN, %s POSITIVE, $ %1.0lf\n",
						patient->getGeneralState()->monthNum,
						(infectedState != SimContext::HIV_INF_NEG) ? "TRUE" : "FALSE",
						patient->getGeneralState()->costsDiscounted);
				}
			}
			else {
				// Increment cost and set QOL for a negative test result
				cost = simContext->getHIVTestInputs()->HIVTestNegativeCost[infectedState];
				incrementCostsHIVMisc(cost);
				accumulateQOLMultiplier(simContext->getHIVTestInputs()->HIVTestNegativeQOLMultiplier[infectedState]);

				// Update statistics for the accepted and negative HIV test result
				updateHIVTestingStats(true, true, false);

				// Output tracing if enabled
				if (patient->getGeneralState()->tracingEnabled) {
					tracer->printTrace(1, "**%d HIV TEST ACCEPT, RETURN, %s NEGATIVE, $ %1.0lf\n",
						patient->getGeneralState()->monthNum,
						(infectedState == SimContext::HIV_INF_NEG) ? "TRUE" : "FALSE",
						patient->getGeneralState()->costsDiscounted);
				}
			}
		}
		else {
			// Increment cost for patient not returning for test results
			cost = simContext->getHIVTestInputs()->HIVTestNonReturnCost[extInfectedState];
			incrementCostsHIVMisc(cost);

			// Update statistics for the accepted HIV test with no return for results
			updateHIVTestingStats(true, false, false);

			// Output tracing if enabled
			if (patient->getGeneralState()->tracingEnabled) {
				tracer->printTrace(1, "**%d HIV TEST ACCEPT, NON-RETURN, $ %1.0lf\n",
					patient->getGeneralState()->monthNum, patient->getGeneralState()->costsDiscounted);
			}
		}
	}
	else {
		// Update statistics for the refused HIV test
		updateHIVTestingStats(false, false, false);

		// Output tracing if enabled
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d HIV TEST NOT ACCEPTED, $ %1.0lf\n",
				patient->getGeneralState()->monthNum, patient->getGeneralState()->costsDiscounted);
		}
	}

	// Schedule the next HIV test
	if (!patient->getMonitoringState()->isDetectedHIVPositive) {
		scheduleHIVTest(true, patient->getGeneralState()->monthNum + patient->getMonitoringState()->intervalHIVTest);
	}
} /* end performRegularScreeningUpdates */

/* performBackgroundScreeningUpdates handles whether patient is detected by background
	screening and updates the associated state and statistics */
void HIVTestingUpdater::performBackgroundScreeningUpdates() {
	// Return if patient is HIV negative
	SimContext::HIV_INF infectedState = patient->getDiseaseState()->infectedHIVState;
	if (infectedState == SimContext::HIV_INF_NEG)
		return;

	// Determine if patient is identified by background testing
	double randNum = CepacUtil::getRandomDouble(100070, patient);
	if (randNum < simContext->getHIVTestInputs()->HIVBackgroundDetectionRate[infectedState]) {
		// Increment costs for the background screening
		double cost = simContext->getHIVTestInputs()->HIVBackgroundTestingCost[infectedState];
		incrementCostsHIVMisc(cost);

		// Set identified state for positive result and schedule the initial clinic visit
		setDetectedHIVState(true, SimContext::HIV_DET_BACKGROUND);
		scheduleInitialClinicVisit();

		// Output tracing if enabled
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d HIV BACKGROUND TEST, POSITIVE, $ %1.0lf\n",
				patient->getGeneralState()->monthNum, patient->getGeneralState()->costsDiscounted);
		}
	}
} /* end performBackgroundScreeningUpdates */
