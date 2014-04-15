#include "include.h"

/* Constructor takes in the patient object */
CD4TestUpdater::CD4TestUpdater(Patient *patient) : StateUpdater(patient) {

}

/* Destructor is empty, no cleanup required */
CD4TestUpdater::~CD4TestUpdater(void) {

}

/* performInitialUpdates perform all of the state and statistics updates upon patient creation */
void CD4TestUpdater::performInitialUpdates() {
	// Call the parent function to perform general updates and initialization
	StateUpdater::performInitialUpdates();
} /* end performInitialUpdates */

/* performMonthlyUpdates perform all of the state and statistics updates for a simulated month */
void CD4TestUpdater::performMonthlyUpdates() {
	// Return if a CD4 test has already occurred for this month or patient is LTFU
	if (patient->getMonitoringState()->hasObservedCD4 &&
		(patient->getMonitoringState()->monthOfObservedCD4 == patient->getGeneralState()->monthNum))
			return;
	if (patient->getMonitoringState()->hasObservedCD4Percentage &&
		(patient->getMonitoringState()->monthOfObservedCD4Percentage == patient->getGeneralState()->monthNum))
			return;
	if (patient->getMonitoringState()->currLTFUState == SimContext::LTFU_STATE_LOST)
		return;

	// Conditions for a regularly scheduled test, also triggers a clinic visit if test should be done
	bool performTest = false;
	if (patient->getMonitoringState()->hasScheduledCD4Test &&
		(patient->getGeneralState()->monthNum >= patient->getMonitoringState()->monthOfScheduledCD4Test)) {
			scheduleEmergencyClinicVisit(true, patient->getGeneralState()->monthNum);
			performTest = true;
	}
	// Conditions for an initial ART test or repeat test after failure
	if (patient->getARTState()->isOnART) {
		int artLineNum = patient->getARTState()->currRegimenNum;
		const SimContext::TreatmentInputs::ARTFailPolicy &failART = simContext->getTreatmentInputs()->failART[artLineNum];
		if (!simContext->getTreatmentInputs()->ARTFailureOnlyAtRegularVisit) {
			if ((patient->getARTState()->numFailedCD4Tests > 0) &&
				(patient->getARTState()->numFailedCD4Tests < failART.diagnoseNumTestsFail))
					performTest = true;
			else if (failART.diagnoseUseCD4TestsConfirm &&
				(patient->getARTState()->numFailedOIs >= failART.OIsMinNum) &&
				(patient->getARTState()->numFailedCD4Tests < failART.diagnoseNumTestsConfirm))
					performTest = true;
		}
		if (patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart < simContext->getTreatmentInputs()->numARTInitialCD4Tests)
			performTest = true;
	}
	if (!performTest)
		return;

	// Calculate the observed CD4 level using the standard deviation of testing error
	if (patient->getGeneralState()->ageCategoryPediatrics >= SimContext::PEDS_AGE_LATE) {
		// Late childhood and adults use absolute CD4
		double stdDevPerc = simContext->getTreatmentInputs()->CD4TestStdDevPercentage;
		double stdDev = stdDevPerc * patient->getDiseaseState()->currTrueCD4;
		double cd4Value = patient->getDiseaseState()->currTrueCD4 + CepacUtil::getRandomGaussian(0, stdDev, 50010, patient);
		setObservedCD4(true, cd4Value);
	}
	else {
		// Early childhood uses CD4 percentage
		double stdDevPerc = simContext->getTreatmentInputs()->CD4TestStdDevPercentage;
		double stdDev = stdDevPerc * patient->getDiseaseState()->currTrueCD4Percentage;
		double cd4Percent = patient->getDiseaseState()->currTrueCD4Percentage + CepacUtil::getRandomGaussian(0, stdDev, 50011, patient);
		setObservedCD4Percentage(true, cd4Percent);
	}

	// Accumulate the costs of the CD4 test
	const double *costs = simContext->getCostInputs()->CD4TestCost;
	incrementCostsCD4Test(costs);

	// Print tracing for the CD4 test if enabled
	if (patient->getGeneralState()->tracingEnabled) {
		if (patient->getGeneralState()->ageCategoryPediatrics >= SimContext::PEDS_AGE_LATE) {
			tracer->printTrace(1, "  %d CD4 TEST: obsv CD4 %1.0lf, $ %1.0lf;\n",
				patient->getGeneralState()->monthNum, patient->getMonitoringState()->currObservedCD4,
				patient->getGeneralState()->costsDiscounted);
		}
		else {
			tracer->printTrace(1, "  %d CD4 TEST: obsv CD4 perc %1.2lf, $ %1.0lf;\n",
				patient->getGeneralState()->monthNum, patient->getMonitoringState()->currObservedCD4Percentage,
				patient->getGeneralState()->costsDiscounted);
		}
	}

	// If on ART, determine if this CD4 test counts as a failed test
	bool failedCD4 = false;
	if (patient->getARTState()->isOnART && (patient->getARTState()->monthOfCurrRegimenStart != patient->getGeneralState()->monthNum)) {
		int artLineNum = patient->getARTState()->currRegimenNum;
		const SimContext::TreatmentInputs::ARTFailPolicy &failART = simContext->getTreatmentInputs()->failART[artLineNum];

		// Late childhood and adults use absolute CD4 criteria
		if (patient->getGeneralState()->ageCategoryPediatrics >= SimContext::PEDS_AGE_LATE) {
			// Has the percentage CD4 drop from maximum been exceeded
			if (failART.CD4PercentageDrop != SimContext::NOT_APPL) {
				double currCD4 = patient->getMonitoringState()->currObservedCD4;
				double maxCD4 = patient->getARTState()->maxObservedCD4OnCurrART;
				if ((maxCD4 - currCD4) / maxCD4 >= failART.CD4PercentageDrop) {
					failedCD4 = true;
				}
			}
			// Has the CD4 count dropped below the pre-ART nadir
			if (!failedCD4 && failART.CD4BelowPreARTNadir) {
				double currCD4 = patient->getMonitoringState()->currObservedCD4;
				double minCD4 = patient->getMonitoringState()->minObservedCD4;
				if (currCD4 <= minCD4) {
					failedCD4 = true;
				}
			}
			// Is CD4 level outside the absolute OR bounds
			if (!failedCD4 && (failART.CD4BoundsOR[SimContext::LOWER_BOUND] != SimContext::NOT_APPL)) {
				double currCD4 = patient->getMonitoringState()->currObservedCD4;
				if (currCD4 < failART.CD4BoundsOR[SimContext::LOWER_BOUND]) {
					failedCD4 = true;
				}
			}
			if (!failedCD4 && (failART.CD4BoundsOR[SimContext::UPPER_BOUND] != SimContext::NOT_APPL)) {
				double currCD4 = patient->getMonitoringState()->currObservedCD4;
				if (currCD4 > failART.CD4BoundsOR[SimContext::UPPER_BOUND]) {
					failedCD4 = true;
				}
			}

			// If failed criteria, evaluate CD4 level outside the absolute AND bounds
			if (failedCD4 && (failART.CD4BoundsAND[SimContext::LOWER_BOUND] != SimContext::NOT_APPL)) {
				double currCD4 = patient->getMonitoringState()->currObservedCD4;
				if (currCD4 >= failART.CD4BoundsAND[SimContext::LOWER_BOUND]) {
					failedCD4 = false;
				}
			}
			if (failedCD4 && (failART.CD4BoundsAND[SimContext::UPPER_BOUND] != SimContext::NOT_APPL)) {
				double currCD4 = patient->getMonitoringState()->currObservedCD4;
				if (currCD4 <= failART.CD4BoundsAND[SimContext::UPPER_BOUND]) {
					failedCD4 = false;
				}
			}
			// If failed criteria, evaluate if months on ART has been reached
			if (failedCD4 && (failART.CD4MonthsFromInit > 0)) {
				int monthsOnART = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart;
				if (monthsOnART < failART.CD4MonthsFromInit) {
					failedCD4 = false;
				}
			}
		}
		else {
			// Early childhood uses CD4 percentage criteria
			// Has CD4 percentage dropped from the absolute peak while on this regimen
			if (simContext->getPedsInputs()->failARTCD4PercentageAbsolDrop[artLineNum] != SimContext::NOT_APPL) {
				double currCD4 = patient->getMonitoringState()->currObservedCD4Percentage;
				double maxCD4 = patient->getARTState()->maxObservedCD4PercentageOnCurrART;
				if ((maxCD4 - currCD4) / maxCD4 >= simContext->getPedsInputs()->failARTCD4PercentageAbsolDrop[artLineNum]) {
					failedCD4 = true;
				}
			}
			// Has the CD4 percentage dropped below the pre-ART nadir
			if (!failedCD4 && simContext->getPedsInputs()->failARTCD4PercentageBelowNadir[artLineNum]) {
				double currCD4 = patient->getMonitoringState()->currObservedCD4Percentage;
				double minCD4 = patient->getMonitoringState()->minObservedCD4Percentage;
				if (currCD4 <= minCD4) {
					failedCD4 = true;
				}
			}
			// Is CD4 percentage outside the absolute OR bounds
			if (!failedCD4 && (simContext->getPedsInputs()->failARTCD4PercentageBoundsOR[artLineNum][SimContext::LOWER_BOUND] != SimContext::NOT_APPL)) {
				double currCD4 = patient->getMonitoringState()->currObservedCD4Percentage;
				if (currCD4 < simContext->getPedsInputs()->failARTCD4PercentageBoundsOR[artLineNum][SimContext::LOWER_BOUND]) {
					failedCD4 = true;
				}
			}
			if (!failedCD4 && (simContext->getPedsInputs()->failARTCD4PercentageBoundsOR[artLineNum][SimContext::UPPER_BOUND] != SimContext::NOT_APPL)) {
				double currCD4 = patient->getMonitoringState()->currObservedCD4Percentage;
				if (currCD4 > simContext->getPedsInputs()->failARTCD4PercentageBoundsOR[artLineNum][SimContext::UPPER_BOUND]) {
					failedCD4 = true;
				}
			}
			// If failed criteria, evaluate CD4 percentage outside the absolute AND bounds
			if (failedCD4 && (simContext->getPedsInputs()->failARTCD4PercentageBoundsAND[artLineNum][SimContext::LOWER_BOUND] != SimContext::NOT_APPL)) {
				double currCD4 = patient->getMonitoringState()->currObservedCD4Percentage;
				if (currCD4 >= simContext->getPedsInputs()->failARTCD4PercentageBoundsAND[artLineNum][SimContext::LOWER_BOUND]) {
					failedCD4 = false;
				}
			}
			if (failedCD4 && (simContext->getPedsInputs()->failARTCD4PercentageBoundsAND[artLineNum][SimContext::UPPER_BOUND] != SimContext::NOT_APPL)) {
				double currCD4 = patient->getMonitoringState()->currObservedCD4Percentage;
				if (currCD4 <= simContext->getPedsInputs()->failARTCD4PercentageBoundsAND[artLineNum][SimContext::UPPER_BOUND]) {
					failedCD4 = false;
				}
			}
			// If failed criteria, evaluate if months on ART has been reached
			if (failedCD4 && (simContext->getPedsInputs()->failARTMonthsFromInit[artLineNum] > 0)) {
				int monthsOnART = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart;
				if (monthsOnART < simContext->getPedsInputs()->failARTMonthsFromInit[artLineNum]) {
					failedCD4 = false;
				}
			}
		}

		// If failed the CD4 test, increment the number of failed tests
		if (failedCD4) {
			incrementARTFailedCD4Tests();

			if (failART.diagnoseUseHVLTestsConfirm) {
				// Using HVL confirmatory tests, trigger HVL test if the immunologic criteria has been met
				if (patient->getARTState()->numFailedCD4Tests >= failART.diagnoseNumTestsFail)
					patient->getHVLTestUpdater()->performMonthlyUpdates();
			}
			else {
				// Not using HVL confirmatory testing, check for only immunologic failure
				if (patient->getARTState()->numFailedCD4Tests >= failART.diagnoseNumTestsFail)
					scheduleEmergencyClinicVisit(true, patient->getGeneralState()->monthNum);
				if (failART.diagnoseUseCD4TestsConfirm) {
					// Using CD4 confirmatory testing, trigger an emergency clinic visit if this
					//	failed test confirms the clinical failure
					if ((patient->getARTState()->numFailedOIs >= failART.OIsMinNum) &&
						(patient->getARTState()->numFailedCD4Tests >= failART.diagnoseNumTestsConfirm))
							scheduleEmergencyClinicVisit(true, patient->getGeneralState()->monthNum);
				}
			}
		}
		else {
			// Test was not failed, reset the count of failed tests if there were previous failures
			if (patient->getARTState()->numFailedCD4Tests > 0)
				resetARTFailedCD4Tests();
			// If this was a confirmatory CD4 test, also reset the number of failed clinical
			//	tests that caused it
			if (failART.diagnoseUseCD4TestsConfirm &&
				(patient->getARTState()->numFailedOIs >= failART.OIsMinNum))
					resetARTFailedOIs();
		}
	}

	// If this was a scheduled CD4 test, determine if another one should be scheduled
	if (patient->getMonitoringState()->hasScheduledCD4Test &&
		(patient->getGeneralState()->monthNum >= patient->getMonitoringState()->monthOfScheduledCD4Test)) {
		int testingInterval;
		if (!patient->getARTState()->hasTakenART) {
			if (patient->getGeneralState()->ageCategoryPediatrics == SimContext::PEDS_AGE_ADULT) {
				// Patient has not yet begun ART, use CD4 threshold to determine testing interval
				if (patient->getMonitoringState()->currObservedCD4 > simContext->getTreatmentInputs()->testingIntervalCD4Threshold) {
					testingInterval = simContext->getTreatmentInputs()->CD4TestingIntervalPreARTHighCD4;
				}
				else {
					testingInterval = simContext->getTreatmentInputs()->CD4TestingIntervalPreARTLowCD4;
				}
			}
			else if (patient->getGeneralState()->ageCategoryPediatrics == SimContext::PEDS_AGE_LATE) {
				testingInterval = simContext->getPedsInputs()->CD4TestingIntervalPreARTLate;
			}
			else {
				testingInterval = simContext->getPedsInputs()->CD4TestingIntervalPreARTEarly;
			}
		}
		else if (patient->getARTState()->hasNextRegimenAvailable) {
			// Patient has taken ART and is not on the last line of ART, use months on ART threshold to
			//	determine testing interval
			int monthsOnART = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart;
			if (monthsOnART < simContext->getTreatmentInputs()->testingIntervalARTMonthsThreshold)
				testingInterval = simContext->getTreatmentInputs()->CD4TestingIntervalOnART[0];
			else
				testingInterval = simContext->getTreatmentInputs()->CD4TestingIntervalOnART[1];
		}
		else if (!patient->getARTState()->hasObservedFailure) {
			// Patient is on last ART line and has no yet been observed to fail, use months on ART
			//	threshold to determine testing interval
			int monthsOnART = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart;
			if (monthsOnART < simContext->getTreatmentInputs()->testingIntervalLastARTMonthsThreshold)
				testingInterval = simContext->getTreatmentInputs()->CD4TestingIntervalOnLastART[0];
			else
				testingInterval = simContext->getTreatmentInputs()->CD4TestingIntervalOnLastART[1];
		}
		else {
			// Patient has failed last ART line, use post-ART testing interval
			testingInterval = simContext->getTreatmentInputs()->CD4TestingIntervalPostART;
		}

		// Schedule the CD4 test if there should be a next one
		if (testingInterval != SimContext::NOT_APPL) {
			scheduleCD4Test(true, patient->getGeneralState()->monthNum + testingInterval);
		}
		else {
			scheduleCD4Test(false);
		}
	}
} /* end performMonthlyUpdates */
