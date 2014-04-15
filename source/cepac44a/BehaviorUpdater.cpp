#include "include.h"

/* Constructor takes in the patient object */
BehaviorUpdater::BehaviorUpdater(Patient *patient) : StateUpdater(patient) {

}

/* Destructor is empty, no cleanup required */
BehaviorUpdater::~BehaviorUpdater(void) {

}

/* performInitialUpdates perform all of the state and statistics updates upon patient creation */
void BehaviorUpdater::performInitialUpdates() {
	// Call the parent function to perform general updates and initialization
	StateUpdater::performInitialUpdates();
} /* end performInitialUpdates */

/* performMonthlyUpdates perform all of the state and statistics updates for a simulated month */
void BehaviorUpdater::performMonthlyUpdates() {
	// Roll for incidence of generic risk factors
	for (int i = 0; i < SimContext::RISK_FACT_NUM; i++) {
		if (!patient->getGeneralState()->hasRiskFactor[i]) {
			double randNum = CepacUtil::getRandomDouble(30010, patient);
			if (randNum < simContext->getCohortInputs()->probRiskFactorIncid[i]) {
				setRiskFactor(i, true);
			}
		}
	}

	// Return if LTFU is not enabled
	if (!simContext->getLTFUInputs()->useLTFU)
		return;

	// If patient is LTFU, roll for return to care
	if (patient->getMonitoringState()->currLTFUState == SimContext::LTFU_STATE_LOST) {
		// Return if we have not reached min months and there is not an acute OI
		int monthsLost = patient->getGeneralState()->monthNum - patient->getMonitoringState()->monthOfLTFUStateChange;
		if ((monthsLost < simContext->getLTFUInputs()->minMonthsRemainLost) &&
			!patient->getDiseaseState()->hasCurrTrueOI)
			return;

		// Calculate the probability of return to care
		double logitRTC = simContext->getLTFUInputs()->regressionCoefficientsRTC[SimContext::RTC_BACKGROUND];
		if (patient->getDiseaseState()->currTrueCD4 < simContext->getLTFUInputs()->CD4ThresholdRTC)
			logitRTC += simContext->getLTFUInputs()->regressionCoefficientsRTC[SimContext::RTC_CD4];
		if (patient->getDiseaseState()->hasCurrTrueOI){
			if (simContext->getLTFUInputs()->severeOIsRTC[patient->getDiseaseState()->typeCurrTrueOI])
				logitRTC += simContext->getLTFUInputs()->regressionCoefficientsRTC[SimContext::RTC_ACUTESEVEREOI];
			else
				logitRTC += simContext->getLTFUInputs()->regressionCoefficientsRTC[SimContext::RTC_ACUTEMILDOI];
		}
		double probRTC = pow(1 + exp(0 - logitRTC), -1);

		// Roll for return to care and update state if it occurs
		double randNum = CepacUtil::getRandomDouble(30020, patient);
		if (randNum < probRTC) {
			setCurrLTFUState(SimContext::LTFU_STATE_RETURNED);

			// Set the month of next clinic visit to the current month
			scheduleRegularClinicVisit(true, patient->getGeneralState()->monthNum);

			// Output tracing if enabled
			if (patient->getGeneralState()->tracingEnabled) {
				tracer->printTrace(1, "**%d PATIENT RETURNED TO CARE;\n", patient->getGeneralState()->monthNum);
			}
		}

		return;
	}

	// Calculate the probability of LTFU
	bool isPostART = false;
	if (patient->getARTState()->hasTakenART)
		isPostART = true;
	double logitLTFU = 0.0;
	if (isPostART) {
		logitLTFU = simContext->getLTFUInputs()->regressionCoefficientsLTFUPostART[SimContext::LTFU_BACKGROUND];
		if (patient->getGeneralState()->ageMonths < simContext->getLTFUInputs()->ageThresholdLTFU)
			logitLTFU += simContext->getLTFUInputs()->regressionCoefficientsLTFUPostART[SimContext::LTFU_AGE];
		if (patient->getGeneralState()->gender == SimContext::GENDER_FEMALE)
			logitLTFU += simContext->getLTFUInputs()->regressionCoefficientsLTFUPostART[SimContext::LTFU_GENDER];
		if (patient->getMonitoringState()->currLTFUState == SimContext::LTFU_STATE_RETURNED)
			logitLTFU += simContext->getLTFUInputs()->regressionCoefficientsLTFUPostART[SimContext::LTFU_HISTORY];
		if (patient->getARTState()->isOnART) {
			int monthsOnART = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart;
			if (monthsOnART < simContext->getLTFUInputs()->timeBoundsFromARTInitLTFU[0])
				logitLTFU += simContext->getLTFUInputs()->regressionCoefficientsLTFUPostART[SimContext::LTFU_T1];
			else if ((monthsOnART >= simContext->getLTFUInputs()->timeBoundsFromARTInitLTFU[0]) &&
				(monthsOnART <= simContext->getLTFUInputs()->timeBoundsFromARTInitLTFU[1]))
				logitLTFU += simContext->getLTFUInputs()->regressionCoefficientsLTFUPostART[SimContext::LTFU_T1_T2];
			else
				logitLTFU += simContext->getLTFUInputs()->regressionCoefficientsLTFUPostART[SimContext::LTFU_T2];
		}
	}
	else {
		logitLTFU = simContext->getLTFUInputs()->regressionCoefficientsLTFUPreART[SimContext::LTFU_BACKGROUND];
		if (patient->getGeneralState()->ageMonths < simContext->getLTFUInputs()->ageThresholdLTFU)
			logitLTFU += simContext->getLTFUInputs()->regressionCoefficientsLTFUPreART[SimContext::LTFU_AGE];
		if (patient->getGeneralState()->gender == SimContext::GENDER_FEMALE)
			logitLTFU += simContext->getLTFUInputs()->regressionCoefficientsLTFUPreART[SimContext::LTFU_GENDER];
		if (patient->getMonitoringState()->currLTFUState == SimContext::LTFU_STATE_RETURNED)
			logitLTFU += simContext->getLTFUInputs()->regressionCoefficientsLTFUPreART[SimContext::LTFU_HISTORY];
	}
	double probLTFU = pow(1 + exp(0 - logitLTFU), -1);

	// Roll for LTFU and update state if so
	double randNum = CepacUtil::getRandomDouble(30030, patient);
	if (randNum < probLTFU) {
		setCurrLTFUState(SimContext::LTFU_STATE_LOST);

		// Stop the current ART and determine if previous ART regimen should be restarted at return
		//	instead of proceeding to the next one (default)
		if (patient->getARTState()->isOnART) {
			if (patient->getARTState()->currSTIState != SimContext::STI_STATE_NONE) {
				setCurrSTIState(SimContext::STI_STATE_NONE);
			}
			stopCurrARTRegimen(SimContext::ART_STOP_LTFU);
			// Set the target HVL back to the setpoint
			setTargetHVLStrata(patient->getDiseaseState()->setpointHVLStrata);

			//Determine if patient should go back to current regimen; default to yes if there is no next regimen
			if (patient->getARTState()->currRegimenEfficacy == SimContext::ART_EFF_FAILURE && patient->getARTState()->hasNextRegimenAvailable) {
				if (patient->getARTState()->hasObservedFailure) {
					int monthsFail = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfObservedFailure;
					if (monthsFail < simContext->getLTFUInputs()->maxMonthsAfterObservedFailureToRestartRegimen) {
						setNextARTRegimen(true, patient->getARTState()->prevRegimenNum);
					}
				}
				else {
					randNum = CepacUtil::getRandomDouble(30040, patient);
					if (randNum < simContext->getLTFUInputs()->probRestartRegimenWithoutObsvervedFailure) {
						setNextARTRegimen(true, patient->getARTState()->prevRegimenNum);
					}
				}
			}
			else {
				setNextARTRegimen(true, patient->getARTState()->prevRegimenNum);
			}

			// Output tracing if enabled
			if (patient->getGeneralState()->tracingEnabled) {
				tracer->printTrace(1, "**%d TAKEN OFF ART %d by %s;\n", patient->getGeneralState()->monthNum,
					patient->getARTState()->prevRegimenNum + 1,
					SimContext::ART_STOP_TYPE_STRS[patient->getARTState()->typeCurrStop]);
			}
		}

		// Roll for and stop the current prophs and TB prophs
		randNum = CepacUtil::getRandomDouble(30050, patient);
		if (randNum >= simContext->getLTFUInputs()->probRemainOnOIProph) {
			for (int i = 0; i < SimContext::OI_NUM; i++) {
				if (patient->getProphState()->isOnProph[i]) {
					if (patient->getGeneralState()->tracingEnabled) {
						tracer->printTrace(1, "**%d STOP %s PROPH %d for OI %s;\n", patient->getGeneralState()->monthNum,
							SimContext::PROPH_TYPE_STRS[patient->getProphState()->currProphType[i]],
							patient->getProphState()->currProphNum[i] + 1, SimContext::OI_STRS[i]);
					}
					stopCurrProph((SimContext::OI_TYPE) i);
				}
			}
			if (patient->getTBState()->isOnProph) {
				if (patient->getGeneralState()->tracingEnabled) {
					tracer->printTrace(1, "**%d STOP TB PROPH %d;\n",
						patient->getGeneralState()->monthNum, patient->getTBState()->currProphNum + 1);
				}
				stopCurrTBProph();
			}
		}

		// Stop all subsequent clinic visits until patient returns to care
		scheduleRegularClinicVisit(false);
		scheduleEmergencyClinicVisit(false);

		// Output tracing if enabled
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d PATIENT LOST TO FOLLOW UP;\n", patient->getGeneralState()->monthNum);
		}
	}
} /* end performMonthlyUpdates */
