#include "include.h"

/** \brief Constructor takes in the patient as a pointer */
HIVTestingUpdater::HIVTestingUpdater(Patient *patient) : StateUpdater(patient) {

}

/** \brief Destructor is empty, no cleanup required */
HIVTestingUpdater::~HIVTestingUpdater() {

}

/** \brief performInitialUpdates perform all of the state and statistics updates upon patient creation */
void HIVTestingUpdater::performInitialUpdates() {
	/** First calls the parent function to perform general updates and initialization */
	StateUpdater::performInitialUpdates();

	/** If not using the testing module, set all HIV positive patients to detected and return */
	if (!simContext->getHIVTestInputs()->enableHIVTesting) {
		if (patient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_NEG){
			setDetectedHIVState(false);
			setLinkedState(false);
		}
		else{
			setDetectedHIVState(true, SimContext::HIV_DET_INITIAL);
			setLinkedState(true, SimContext::HIV_DET_INITIAL);
		}
		return;
	}

	/** If using the testing module, determine if patient should enter the model detected */
	SimContext::HIV_INF infectState = patient->getDiseaseState()->infectedHIVState;
	double randNum = CepacUtil::getRandomDouble(100010, patient);
	if (randNum < simContext->getHIVTestInputs()->probHIVDetectionInitial[infectState]) {
		setDetectedHIVState(true, SimContext::HIV_DET_INITIAL);
		setLinkedState(true, SimContext::HIV_DET_INITIAL);
	}
	else {
		setDetectedHIVState(false);
		setLinkedState(false);
	}

	/** If patient is not detected as positive, initialize the HIV testing freq and acceptance rate */
	if (!patient->getMonitoringState()->isDetectedHIVPositive) {
		// Default HIV testing parameters to NOT_APPL
		int intervalTestIndex = 0;
		int acceptanceRateIndex = 0;
		bool hasNextTest = false;
		int monthNextTest = SimContext::NOT_APPL;
		if (simContext->getHIVTestInputs()->HIVTestAvailable) {
			/** Set the HIV testing interval and time for next test */
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

			/** Set the HIV testing acceptance rate */
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

/** \brief setSimContext changes the inputs the updater uses to determine disease progression -- to be used primarily by the transmission model
 * changes the testing frequency and acceptance rate based on the testing strategy of the new simContext
 *
 * \param newSimContext a pointer to a SimContext object that patient should switch calling inputs from
 **/
void HIVTestingUpdater::setSimContext(SimContext *newSimContext){
	/** First calls the parent function to switch the simContext */
	StateUpdater::setSimContext(newSimContext);

	/** Only update the testing strategy if there is a strategy to update and the patient is undetected */
	if (this->simContext->getHIVTestInputs()->enableHIVTesting && !patient->getMonitoringState()->isDetectedHIVPositive){
		// Default HIV testing parameters to NOT_APPL
		int intervalTestIndex = 0;
		int acceptanceRateIndex = 0;
		bool hasNextTest = patient->getMonitoringState()->hasScheduledHIVTest;
		int monthNextTest = patient->getMonitoringState()->monthOfScheduledHIVTest;
		if (simContext->getHIVTestInputs()->HIVTestAvailable) {
			/** Set the HIV testing interval and time for next test */
			double randNum = CepacUtil::getRandomDouble(100020, patient);
			for (int i = 0; i < SimContext::HIV_TEST_FREQ_NUM; i++) {
				if ((randNum < simContext->getHIVTestInputs()->HIVTestingProbability[i]) &&
					(simContext->getHIVTestInputs()->HIVTestingProbability[i] > 0)) {
						if (simContext->getHIVTestInputs()->HIVTestingInterval[i] > 0) {
							hasNextTest = true;
							intervalTestIndex = i;
							/** If we're turning testing on for this patient, set monthNextTest to be randomly determined between current month and currMonth+testingInterval
							//0.5 is added because casting as int just takes the floor... adding 0.5 causes correct rounding */
							monthNextTest = patient->getGeneralState()->monthNum + ((int) (CepacUtil::getRandomDouble(100025, patient) * simContext->getHIVTestInputs()->HIVTestingInterval[i] + 0.5));
						}
						break;
				}
				randNum -= simContext->getHIVTestInputs()->HIVTestingProbability[i];
			}

			/** Set the HIV testing acceptance rate */
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

/** \brief performMonthlyUpdates perform all of the state and statistics updates for a simulated month */
void HIVTestingUpdater::performMonthlyUpdates() {

	if (patient->getMonitoringState()->isLinked)
		return;

	/** perform the regular and background HIV screenings if patient is not already detected as HIV positive by calling HIVTestingUpdater::performRegularScreeningUpdates() and HIVTestingUpdater::performBackgroundScreeningUpdates() */
	performRegularScreeningUpdates();
	if (!patient->getMonitoringState()->isLinked) {
		performBackgroundScreeningUpdates();
	}
} /* end performMonthlyUpdates */

/** \brief performRegularScreeningUpdates determines if a screening occurs, if its accepted, if
	they are detected, and updates the associated state and statistics */
void HIVTestingUpdater::performRegularScreeningUpdates() {
	/** Return if this is not the month of the next test */
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

	/** Accrue the initial startup cost for all patients, regardless of test acceptance */
	if (patient->getGeneralState()->monthNum == patient->getGeneralState()->initialMonthNum) {
		double cost = simContext->getHIVTestInputs()->HIVTestInitialCost[extInfectedState];
		incrementCostsHIVTest(cost);
		// Output tracing if enabled
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d HIV SCREENING STARTUP, $ %1.0lf\n",
				patient->getGeneralState()->monthNum, patient->getGeneralState()->costsDiscounted);
		}
	}

	/** Check if scheduled testing is accepted by patient */
	double randNum = CepacUtil::getRandomDouble(100040, patient);
	if (randNum < patient->getMonitoringState()->acceptanceRateHIVTest) {
		/** If test is accepted, accrue the cost of performing the test */
		double cost = simContext->getHIVTestInputs()->HIVTestCost[extInfectedState];
		incrementCostsHIVTest(cost);

		/** Check if patient returns for test result */
		randNum = CepacUtil::getRandomDouble(100050, patient);
		if (randNum < simContext->getHIVTestInputs()->HIVTestReturnRate[infectedState]) {
			/** Determine the test result */
			randNum = CepacUtil::getRandomDouble(100060, patient);
			if (randNum < simContext->getHIVTestInputs()->HIVTestPositiveRate[infectedState]) {
				/** Increment cost, set QOL, and update identified state for a positive test result */
				cost = simContext->getHIVTestInputs()->HIVTestPositiveCost[infectedState];
				incrementCostsHIVMisc(cost);
				accumulateQOLMultiplier(simContext->getHIVTestInputs()->HIVTestPositiveQOLMultiplier[infectedState]);
				/** Update statistics for the accepted and positive HIV test result */
				updateHIVTestingStats(true, true, true);

				// Output tracing if enabled
				if (patient->getGeneralState()->tracingEnabled) {
					tracer->printTrace(1, "**%d HIV TEST ACCEPT, RETURN, %s POSITIVE, $ %1.0lf\n",
						patient->getGeneralState()->monthNum,
						(infectedState != SimContext::HIV_INF_NEG) ? "TRUE" : "FALSE",
						patient->getGeneralState()->costsDiscounted);
				}

				if (infectedState != SimContext::HIV_INF_NEG) {
					cost = simContext->getHIVTestInputs()->HIVTestDetectionCost[infectedState];
					incrementCostsHIVMisc(cost);
					bool wasPrevDetected= patient->getMonitoringState()->isDetectedHIVPositive;
					if (wasPrevDetected)
						setDetectedHIVState(true, SimContext::HIV_DET_SCREENING_PREV_DET);
					else
						setDetectedHIVState(true, SimContext::HIV_DET_SCREENING);


					if (simContext->getHIVTestInputs()->CD4TestAvailable)
						performLabStagingUpdates(wasPrevDetected);
					else{
						if (wasPrevDetected)
							setLinkedState(true,SimContext::HIV_DET_SCREENING_PREV_DET);
						else
							setLinkedState(true, SimContext::HIV_DET_SCREENING);
						scheduleInitialClinicVisit();
					}
					return;
				}
			}
			else {
				/** Increment cost and set QOL for a negative test result */
				cost = simContext->getHIVTestInputs()->HIVTestNegativeCost[infectedState];
				incrementCostsHIVMisc(cost);
				accumulateQOLMultiplier(simContext->getHIVTestInputs()->HIVTestNegativeQOLMultiplier[infectedState]);

				/** Update statistics for the accepted and negative HIV test result */
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
			/** Increment cost for patient not returning for test results */
			cost = simContext->getHIVTestInputs()->HIVTestNonReturnCost[extInfectedState];
			incrementCostsHIVMisc(cost);

			/** Update statistics for the accepted HIV test with no return for results */
			updateHIVTestingStats(true, false, false);

			// Output tracing if enabled
			if (patient->getGeneralState()->tracingEnabled) {
				tracer->printTrace(1, "**%d HIV TEST ACCEPT, NON-RETURN, $ %1.0lf\n",
					patient->getGeneralState()->monthNum, patient->getGeneralState()->costsDiscounted);
			}
		}
	}
	else {
		/** Update statistics for the refused HIV test */
		updateHIVTestingStats(false, false, false);

		// Output tracing if enabled
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d HIV TEST NOT ACCEPTED, $ %1.0lf\n",
				patient->getGeneralState()->monthNum, patient->getGeneralState()->costsDiscounted);
		}
	}

	/** Schedule the next HIV test */
	scheduleHIVTest(true, patient->getGeneralState()->monthNum + patient->getMonitoringState()->intervalHIVTest);

} /* end performRegularScreeningUpdates */

/** \brief performBackgroundScreeningUpdates handles whether patient is detected by background
	screening and updates the associated state and statistics */
void HIVTestingUpdater::performBackgroundScreeningUpdates() {
	/** Return if patient is HIV negative */
	SimContext::HIV_INF infectedState = patient->getDiseaseState()->infectedHIVState;
	if (infectedState == SimContext::HIV_INF_NEG)
		return;

	/** Determine if patient is identified by background testing */
	double randNum = CepacUtil::getRandomDouble(100070, patient);
	if (randNum < simContext->getHIVTestInputs()->HIVBackgroundDetectionRate[infectedState]) {
		/** Increment costs for the background screening */
		double cost = simContext->getHIVTestInputs()->HIVBackgroundTestingCost[infectedState];
		incrementCostsHIVMisc(cost);

		/** Set identified state for positive result and schedule the initial clinic visit */
		if (patient->getMonitoringState()->isDetectedHIVPositive){
			setDetectedHIVState(true, SimContext::HIV_DET_BACKGROUND_PREV_DET);
			setLinkedState(true, SimContext::HIV_DET_BACKGROUND_PREV_DET);
		}
		else{
			setDetectedHIVState(true, SimContext::HIV_DET_BACKGROUND);
			setLinkedState(true, SimContext::HIV_DET_BACKGROUND);
		}
		scheduleInitialClinicVisit();

		/** Output tracing if enabled */
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d HIV BACKGROUND TEST, POSITIVE, $ %1.0lf\n",
				patient->getGeneralState()->monthNum, patient->getGeneralState()->costsDiscounted);
		}
	}
} /* end performBackgroundScreeningUpdates */

/** \brief performLabStagingUpdates handles whether a patient is given a cd4 test, if they accept and updates the associated state and statistics*/
void HIVTestingUpdater::performLabStagingUpdates(bool wasPrevDetected){
	if (patient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_NEG)
		return;

	SimContext::HIV_POS infectedState = (SimContext::HIV_POS) (patient->getDiseaseState()->infectedHIVState-1);

	/** Accrue the initial startup cost for all patients, regardless of test acceptance */
	if (patient->getGeneralState()->monthNum == patient->getGeneralState()->initialMonthNum) {
		double cost = simContext->getHIVTestInputs()->CD4TestInitialCost[infectedState];
		incrementCostsLabStagingTest(cost);
		// Output tracing if enabled
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d LAB STAGING STARTUP, $ %1.0lf\n",
				patient->getGeneralState()->monthNum, patient->getGeneralState()->costsDiscounted);
		}
	}

	/** Check if scheduled testing is accepted by patient */
	double randNum = CepacUtil::getRandomDouble(100080, patient);
	if (randNum < simContext->getHIVTestInputs()->CD4TestAcceptRate[infectedState]) {
		/** If test is accepted, accrue the cost of performing the test */
		double cost = simContext->getHIVTestInputs()->CD4TestCost[infectedState];
		incrementCostsLabStagingTest(cost);

		/** Check if patient returns for test result */
		randNum = CepacUtil::getRandomDouble(100090, patient);
		if (randNum < simContext->getHIVTestInputs()->CD4TestReturnRate[infectedState]) {
			/** If performing a test, calculate the observed CD4 level using the standard deviation of testing error */
			double stdDevPerc = simContext->getHIVTestInputs()->CD4TestStdDevPercentage;
			double stdDev = stdDevPerc * patient->getDiseaseState()->currTrueCD4;
			double cd4Value = patient->getDiseaseState()->currTrueCD4 + CepacUtil::getRandomGaussian(0, stdDev, 100100, patient);
			setObservedCD4(true, cd4Value, true);

			/**Increment cost for return for result**/
			double cost = simContext->getHIVTestInputs()->CD4TestReturnCost[infectedState];
			incrementCostsLabStagingMisc(cost);

			/** Determine Linkage to Care */
			randNum = CepacUtil::getRandomDouble(100110, patient);
			if (randNum < simContext->getHIVTestInputs()->CD4TestLinkageRate[patient->getMonitoringState()->currObservedCD4Strata]){
				//patient linked to care
				updateLabStagingStats(true,true,true);
				tracer->printTrace(1, "**%d LAB STAGING ACCEPT, RETURN, LINK: obsv CD4 %1.0lf, $ %1.0lf\n",
					patient->getGeneralState()->monthNum, patient->getMonitoringState()->currObservedCD4, patient->getGeneralState()->costsDiscounted);
				if (wasPrevDetected)
					setLinkedState(true, SimContext::HIV_DET_SCREENING_PREV_DET);
				else
					setLinkedState(true, SimContext::HIV_DET_SCREENING);
				scheduleInitialClinicVisit();
				return;
			}
			else{
				updateLabStagingStats(true,true,false);
				tracer->printTrace(1, "**%d LAB STAGING ACCEPT, RETURN, NON-LINK: obsv CD4 %1.0lf, $ %1.0lf\n",
					patient->getGeneralState()->monthNum, patient->getMonitoringState()->currObservedCD4, patient->getGeneralState()->costsDiscounted);
			}
		}
		else {
			/** Increment cost for patient not returning for test results */
			double cost = simContext->getHIVTestInputs()->CD4TestNonReturnCost[infectedState];
			incrementCostsLabStagingMisc(cost);
			updateLabStagingStats(true,false,false);
			// Output tracing if enabled
			if (patient->getGeneralState()->tracingEnabled) {
				tracer->printTrace(1, "**%d LAB STAGING ACCEPT, NON-RETURN, $ %1.0lf\n",
					patient->getGeneralState()->monthNum, patient->getGeneralState()->costsDiscounted);
			}
		}
	}
	else {
		updateLabStagingStats(false,false,false);
		// Output tracing if enabled
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d LAB STAGING NOT ACCEPTED, $ %1.0lf\n",
				patient->getGeneralState()->monthNum, patient->getGeneralState()->costsDiscounted);
		}
	}

	/** Schedule the next HIV test */
	scheduleHIVTest(true, patient->getGeneralState()->monthNum + patient->getMonitoringState()->intervalHIVTest);
}
