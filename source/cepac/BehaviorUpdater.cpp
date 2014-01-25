#include "include.h"

/** \brief Constructor takes in the patient object */
BehaviorUpdater::BehaviorUpdater(Patient *patient) : StateUpdater(patient) {

}

/** \brief Destructor is empty, no cleanup required */
BehaviorUpdater::~BehaviorUpdater(void) {

}

/** \brief performInitialUpdates perform all of the state and statistics updates upon patient creation */
void BehaviorUpdater::performInitialUpdates() {
	/** Call the parent function to perform general updates and initialization */
	StateUpdater::performInitialUpdates();
	if (simContext->getLTFUInputs()->useLTFU)
		setPreARTResponseBase(CepacUtil::getRandomGaussian(simContext->getLTFUInputs()->propRespondLTFUPreARTLogitMean, simContext->getLTFUInputs()->propRespondLTFUPreARTLogitStdDev, 30005, patient));

} /* end performInitialUpdates */

/** \brief performMonthlyUpdates perform all of the state and statistics updates for a simulated month */
void BehaviorUpdater::performMonthlyUpdates() {
	/**Update Efficacy for adherence intervention*/
	if(patient->getARTState()->isOnAdherenceIntervention){
		int monthsOnIntervention=patient->getGeneralState()->monthNum-patient->getARTState()->monthOfAdherenceStart;
		if(monthsOnIntervention==simContext->getHeterogeneityInputs()->stageBoundsInterventionEfficacy[0]){
			/**Set initial efficacy for adherence intervention*/
			double efficacyMean = simContext->getHeterogeneityInputs()->interventionEfficacyMean[1];
			double efficacyStdDev = simContext->getHeterogeneityInputs()->interventionEfficacyStdDev[1];
			double efficacyCoeff = CepacUtil::getRandomGaussian(efficacyMean, efficacyStdDev, 30060, patient);
			double responseLogit=patient->getARTState()->responseLogitCurrRegimenBase;
			responseLogit+=efficacyCoeff;
			setCurrARTResponse(responseLogit);
		}
		else if(monthsOnIntervention==simContext->getHeterogeneityInputs()->stageBoundsInterventionEfficacy[1]){
			/**Set initial efficacy for adherence intervention*/
			double efficacyMean = simContext->getHeterogeneityInputs()->interventionEfficacyMean[2];
			double efficacyStdDev = simContext->getHeterogeneityInputs()->interventionEfficacyStdDev[2];
			double efficacyCoeff = CepacUtil::getRandomGaussian(efficacyMean, efficacyStdDev, 30070, patient);
			double responseLogit=patient->getARTState()->responseLogitCurrRegimenBase;
			responseLogit+=efficacyCoeff;
			setCurrARTResponse(responseLogit);
		}
	}

	/** Roll for incidence of generic risk factors */
	for (int i = 0; i < SimContext::RISK_FACT_NUM; i++) {
		if (!patient->getGeneralState()->hasRiskFactor[i]) {
			double randNum = CepacUtil::getRandomDouble(30010, patient);
			if (randNum < simContext->getCohortInputs()->probRiskFactorIncid[i]) {
				setRiskFactor(i, true);
			}
		}
	}

	/** Return if LTFU is not enabled */
	if (!simContext->getLTFUInputs()->useLTFU)
		return;

	/** If patient is LTFU, roll for return to care */
	if (patient->getMonitoringState()->currLTFUState == SimContext::LTFU_STATE_LOST) {
		/** Return if we have not reached min months and there is not an acute OI */
		int monthsLost = patient->getGeneralState()->monthNum - patient->getMonitoringState()->monthOfLTFUStateChange;
		if ((monthsLost < simContext->getLTFUInputs()->minMonthsRemainLost) &&
			!patient->getDiseaseState()->hasCurrTrueOI)
			return;

		/** Calculate the probability of return to care */
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

		/** Roll for return to care and update state if it occurs */
		double randNum = CepacUtil::getRandomDouble(30020, patient);
		if (randNum < probRTC) {
			setCurrLTFUState(SimContext::LTFU_STATE_RETURNED);

			/** If RTC, set the month of next clinic visit to the current month */
			scheduleRegularClinicVisit(true, patient->getGeneralState()->monthNum);

			// Output tracing if enabled
			if (patient->getGeneralState()->tracingEnabled) {
				tracer->printTrace(1, "**%d PATIENT RETURNED TO CARE;\n", patient->getGeneralState()->monthNum);
			}
		}

		return;
	}

	/** If not already LTFU, calculate the probability of LTFU using user specified logits*/
	if(!patient->getMonitoringState()->hadPrevClinicVisit)
		return;

	bool isOnART = false;
	if (patient->getARTState()->isOnART)
		isOnART = true;
	double logitLTFU = 0.0;
	if (isOnART) {
		logitLTFU = patient->getARTState()->responseLogitCurrRegimen;
	}
	else {
		logitLTFU = patient->getGeneralState()->responseBaselineLogit;
		int ageCat = patient->getGeneralState()->ageCategoryHIVInfection;
		SimContext::PEDS_AGE_CAT pedsAgeCat = patient->getGeneralState()->ageCategoryPediatrics;

		if(pedsAgeCat==SimContext::PEDS_AGE_ADULT){
			logitLTFU += simContext->getHeterogeneityInputs()->propRespondAge[ageCat];
			SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
			logitLTFU += simContext->getHeterogeneityInputs()->propRespondCD4[cd4Strata];
		}
		else if(pedsAgeCat==SimContext::PEDS_AGE_LATE){
			logitLTFU += simContext->getHeterogeneityInputs()->propRespondAgeLate;
			SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
			logitLTFU += simContext->getHeterogeneityInputs()->propRespondCD4[cd4Strata];
		}
		else{
			logitLTFU += simContext->getHeterogeneityInputs()->propRespondAgeEarly;
		}

		if (patient->getGeneralState()->gender == SimContext::GENDER_FEMALE)
			logitLTFU += simContext->getHeterogeneityInputs()->propRespondFemale;
		if (patient->getDiseaseState()->typeTrueOIHistory != SimContext::HIST_EXT_N)
			logitLTFU += simContext->getHeterogeneityInputs()->propRespondHistoryOIs;
		if (patient->getARTState()->hadPrevToxicity)
			logitLTFU += simContext->getHeterogeneityInputs()->propRespondPriorARTToxicity;
		for (int i = 0; i < SimContext::RISK_FACT_NUM; i++) {
			if (patient->getGeneralState()->hasRiskFactor[i])
				logitLTFU += simContext->getHeterogeneityInputs()->propRespondRiskFactor[i];
		}
		logitLTFU+=patient->getGeneralState()->responseLogitPreARTBase;
	}



	//calculate prob of LTFU from outcome function
	double propRespondLTFU = pow(1 + exp(0 - logitLTFU), -1);
	double L1 = simContext->getLTFUInputs()->responseThresholdLTFU[0];
	double L2 = simContext->getLTFUInputs()->responseThresholdLTFU[1];
	double respFactor;

	if (propRespondLTFU > L2)
		respFactor = 1.0;
	else if (propRespondLTFU > L1)
		respFactor = (propRespondLTFU - L1) / (L2 - L1);
	else
		respFactor = 0.0;

	double lowerValue = simContext->getLTFUInputs()->responseValueLTFU[0];
	double upperValue = simContext->getLTFUInputs()->responseValueLTFU[1];
	double probLTFU = lowerValue+respFactor*(upperValue-lowerValue);

	/** Roll for LTFU and update state if so */
	double randNum = CepacUtil::getRandomDouble(30030, patient);
	if (randNum < probLTFU) {
		setCurrLTFUState(SimContext::LTFU_STATE_LOST);

		/** If newly LTFU, stop the current ART and determine if previous ART regimen should be restarted at return
			instead of proceeding to the next one (default) */
		if (patient->getARTState()->isOnART) {
			if (patient->getARTState()->currSTIState != SimContext::STI_STATE_NONE) {
				setCurrSTIState(SimContext::STI_STATE_NONE);
			}
			stopCurrARTRegimen(SimContext::ART_STOP_LTFU);
			/** Set the target HVL back to the setpoint */
			setTargetHVLStrata(patient->getDiseaseState()->setpointHVLStrata);

			/** Determine if patient should go back to current regimen; default to yes if there is no next regimen */
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

		/** Roll for and stop the current OI prophs and TB prophs */
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

		/** Stop all subsequent clinic visits until patient returns to care */
		scheduleRegularClinicVisit(false);
		scheduleEmergencyClinicVisit(false);

		// Output tracing if enabled
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d PATIENT LOST TO FOLLOW UP;\n", patient->getGeneralState()->monthNum);
		}
	}
} /* end performMonthlyUpdates */
