#include "include.h"

/* Constructor takes in the patient object and determines if updatesCanOccur */
EndMonthUpdater::EndMonthUpdater(Patient *patient) : StateUpdater(patient) {

}

/* Destructor is empty, no cleanup required */
EndMonthUpdater::~EndMonthUpdater(void) {

}

/* performInitialUpdates perform all of the state and statistics updates upon patient creation */
void EndMonthUpdater::performInitialUpdates() {
	// Call the parent function to perform general updates and initialization
	StateUpdater::performInitialUpdates();
} /* end performInitialUpdates */

/* performMonthlyUpdates perform all of the state and statistics updates for a simulated month */
void EndMonthUpdater::performMonthlyUpdates() {
	// Use a half month for applicable costs and survival stats in the month of death
	double percentOfMonth = 1.0;
	if (!patient->isAlive()) {
		percentOfMonth = 0.5;
	}

	// Increment the monthly patient costs and statistics for routine care
	if (patient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_NEG) {
		SimContext::GENDER_TYPE gender = patient->getGeneralState()->gender;
		int ageCat = patient->getGeneralState()->ageCategoryHIVInfection;
		incrementCostsMisc(simContext->getCostInputs()->routineCareCostHIVNegative[gender][ageCat], percentOfMonth);
	}
	else if (!patient->getMonitoringState()->isDetectedHIVPositive) {
		SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
		double cost = percentOfMonth * simContext->getHIVTestInputs()->monthCostHIVUndetected[cd4Strata];
		incrementCostsMisc(cost, percentOfMonth);
	}
	else {
		SimContext::CD4_STRATA cd4Strata = patient->getMonitoringState()->currObservedCD4Strata;
		// Default to using true CD4 for routince care costs if observed CD4 is unknown
		if (!patient->getMonitoringState()->hasObservedCD4)
			cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
		SimContext::GENDER_TYPE gender = patient->getGeneralState()->gender;
		SimContext::ART_STATES artState = (patient->getARTState()->isOnART) ? SimContext::ART_ON_STATE : SimContext::ART_OFF_STATE;
		int ageCat = patient->getGeneralState()->ageCategoryHIVInfection;
		const double *costArray = simContext->getCostInputs()->routineCareCostHIVPositive[artState][cd4Strata][gender][ageCat];
		incrementCostsMisc(costArray, percentOfMonth);
	}

	// Increment costs for ART treatments
	if (patient->getARTState()->isOnART) {
		int artLineNum = patient->getARTState()->currRegimenNum;
		double cost = 0.0;
		if (patient->getGeneralState()->ageCategoryPediatrics == SimContext::PEDS_AGE_ADULT) {
			cost = simContext->getARTInputs(artLineNum)->costMonthly;
		}
		else if (patient->getGeneralState()->ageCategoryPediatrics == SimContext::PEDS_AGE_EARLY_NUM) {
			cost = simContext->getPedsARTInputs(artLineNum)->costMonthlyLate;
		}
		else {
			cost = simContext->getPedsARTInputs(artLineNum)->costMonthlyEarly;
		}
		// Adjust cost for non-responders by factor of their probability of filling prescriptions
		if (patient->getARTState()->responseTypeCurrRegimen == SimContext::RESP_TYPE_NON) {
			cost *= simContext->getHeterogeneityInputs()->probFillARTPrescriptionsNonResponder;
		}
		incrementCostsART(artLineNum, cost);
	}

	// Increment costs for OI prophylaxis treatments
	if (patient->getProphState()->currTotalNumProphsOn > 0) {
		for (int i = 0; i < SimContext::OI_NUM; i++) {
			if (patient->getProphState()->isOnProph[i]) {
				int prophNum = patient->getProphState()->currProphNum[i];
				SimContext::PROPH_TYPE prophType = patient->getProphState()->currProphType[i];
				double cost = simContext->getProphInputs(prophType,i,prophNum)->costMonthly;
				incrementCostsProph((SimContext::OI_TYPE) i, prophNum, cost);
			}
		}
	}

	// Increment costs for TB proph and treatments
	if (patient->getTBState()->isOnProph) {
		int prophNum = patient->getTBState()->currProphNum;
		double cost = simContext->getTBInputs()->tbProphInputs[prophNum]->costMonthly;
		incrementCostsTBProph(prophNum, cost);
	}
	if (patient->getTBState()->isOnTreatment) {
		SimContext::TB_TREATM_STAGE treatStage = patient->getTBState()->currTreatmentStage;
		double artMult = 1.0;
		if (patient->getARTState()->isOnART)
			artMult = simContext->getTBInputs()->monthlyTreatedARTMultiplier[treatStage];
		const double *costs = simContext->getTBInputs()->monthlyTreatedCosts[treatStage];
		incrementCostsTBTreatment(costs, artMult);
	}

	// Increment costs of mortality in the month of death
	if (!patient->isAlive()) {
		SimContext::DTH_CAUSES causeOfDeath = patient->getDiseaseState()->causeOfDeath;
		SimContext::ART_STATES artState = (patient->getARTState()->isOnART) ? SimContext::ART_ON_STATE : SimContext::ART_OFF_STATE;
		if (causeOfDeath < SimContext::OI_NUM) {
			if (patient->getMonitoringState()->clinicVisitType != SimContext::CLINIC_INITIAL) {
				const double *costArray = simContext->getCostInputs()->deathCostTreated[artState][causeOfDeath];
				incrementCostsMisc(costArray, 1.0);
			}
			else {
				const double *costArray = simContext->getCostInputs()->deathCostUntreated[artState][causeOfDeath];
				incrementCostsMisc(costArray, 1.0);
			}
		}
		else if (causeOfDeath == SimContext::DTH_CHRAIDS) {
			if (patient->getMonitoringState()->isDetectedHIVPositive) {
				if (patient->getMonitoringState()->clinicVisitType != SimContext::CLINIC_INITIAL) {
					const double *costArray = simContext->getCostInputs()->deathCostTreated[artState][SimContext::DTH_CHRAIDS];
					incrementCostsMisc(costArray, 1.0);
				}
				else {
					const double *costArray = simContext->getCostInputs()->deathCostUntreated[artState][SimContext::DTH_CHRAIDS];
					incrementCostsMisc(costArray, 1.0);
				}
			}
			else {
				double cost = simContext->getHIVTestInputs()->chronicDeathCostHIVUndetected;
				incrementCostsMisc(cost, 1.0);
			}
		}
		else if (causeOfDeath == SimContext::DTH_NONAIDS) {
			if (patient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_NEG) {
				double cost = simContext->getHIVTestInputs()->deathCostHIVNegative;
				incrementCostsMisc(cost, 1.0);
			}
			else if (patient->getMonitoringState()->isDetectedHIVPositive) {
				if (patient->getMonitoringState()->clinicVisitType != SimContext::CLINIC_INITIAL) {
					const double *costArray = simContext->getCostInputs()->deathCostTreated[artState][SimContext::DTH_NONAIDS];
					incrementCostsMisc(costArray, 1.0);
				}
				else {
					const double *costArray = simContext->getCostInputs()->deathCostUntreated[artState][SimContext::DTH_NONAIDS];
					incrementCostsMisc(costArray, 1.0);
				}
			}
			else {
				double cost = simContext->getHIVTestInputs()->nonAIDSDeathCostHIVUndetected;
				incrementCostsMisc(cost, 1.0);
			}
		}
		else if ((causeOfDeath >= SimContext::DTH_CHRM_1) && (causeOfDeath < SimContext::DTH_CHRM_1 + SimContext::CHRM_NUM)) {
			double cost = simContext->getCHRMsInputs()->costDeathCHRMs[causeOfDeath - SimContext::DTH_CHRM_1];
			incrementCostsMisc(cost, 1.0);
		}
		// ART toxicity death is incurred in the MortalityUpdater
		// Other causes of death incur no additional costs
	}

	// Use the background age-based QOL multiplier, applied to all patients
	SimContext::GENDER_TYPE gender = patient->getGeneralState()->gender;
	int ageYears = patient->getGeneralState()->ageMonths / 12;
	accumulateQOLMultiplier(simContext->getQOLInputs()->nonAIDSBackgroundQOL[gender][ageYears]);

	// Assess routine care QOL or QOL in the month of death
	if (patient->isAlive()) {
		// If alive, use QOL multiplier with either acute OI, undetected HIV, or routine care levels
		if (patient->getDiseaseState()->hasCurrTrueOI) {
			SimContext::OI_TYPE oiType = patient->getDiseaseState()->typeCurrTrueOI;
			accumulateQOLMultiplier(simContext->getQOLInputs()->acuteOIQOL[oiType]);
		}
		else if (!patient->getMonitoringState()->isDetectedHIVPositive) {
			SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
			accumulateQOLMultiplier(simContext->getHIVTestInputs()->monthQOLHIVUndetected[cd4Strata]);
		}
		else {
			SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
			double routineCareQOL = simContext->getQOLInputs()->routineCareQOL[cd4Strata][SimContext::OI_NONE];
			for (int i = 0; i < SimContext::OI_NUM; i++) {
				if (patient->getDiseaseState()->hasTrueOIHistory[i] &&
					(simContext->getQOLInputs()->routineCareQOL[cd4Strata][i] < routineCareQOL)) {
						routineCareQOL = simContext->getQOLInputs()->routineCareQOL[cd4Strata][i];
				}
			}
			accumulateQOLMultiplier(routineCareQOL);
		}
		// If patient has two or more different OI histories, add an additional 20% QOL reduction
		int numOIs = 0;
		for (int i = 0; i < SimContext::OI_NUM; i++) {
			if (patient->getDiseaseState()->hasTrueOIHistory[i]) {
				numOIs++;
				if (numOIs > 1) {
					accumulateQOLMultiplier(0.8);
					break;
				}
			}
		}
	}
	else {
		// If month of death, use QOL multiplier for the cause of death
		SimContext::DTH_CAUSES causeOfDeath = patient->getDiseaseState()->causeOfDeath;
		if ((causeOfDeath < SimContext::OI_NUM)) {
			accumulateQOLMultiplier(simContext->getQOLInputs()->deathBasicQOL[causeOfDeath]);
		}
		else if (causeOfDeath == SimContext::DTH_CHRAIDS) {
			if (!patient->getMonitoringState()->isDetectedHIVPositive) {
				accumulateQOLMultiplier(simContext->getHIVTestInputs()->chronicDeathQOLHIVUndetected);
			}
			else {
				accumulateQOLMultiplier(simContext->getQOLInputs()->deathBasicQOL[SimContext::DTH_CHRAIDS]);
			}
		}
		else if (causeOfDeath == SimContext::DTH_NONAIDS) {
			if (patient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_NEG) {
				accumulateQOLMultiplier(simContext->getHIVTestInputs()->deathQOLHIVNegative);
			}
			else if (!patient->getMonitoringState()->isDetectedHIVPositive) {
				accumulateQOLMultiplier(simContext->getHIVTestInputs()->nonAIDSDeathQOLHIVUndetected);
			}
			else {
				accumulateQOLMultiplier(simContext->getQOLInputs()->deathBasicQOL[SimContext::DTH_NONAIDS]);
			}
		}
		else if ((causeOfDeath >= SimContext::DTH_CHRM_1) && (causeOfDeath < SimContext::DTH_CHRM_1 + SimContext::CHRM_NUM)) {
			accumulateQOLMultiplier(simContext->getCHRMsInputs()->QOLMultDeathCHRMs[causeOfDeath - SimContext::DTH_CHRM_1]);
		}
		// Other causes of death incur no additional QOL multipliers
	}

	// Update additional state, statistics and tracing for regular month or month of death
	if (patient->isAlive()) {
		// Increment the patient, overall, and longitudinal survival stats
		updatePatientSurvival(percentOfMonth);
		updateOverallSurvival(percentOfMonth);
		updateLongitSurvival();

		// update the patients OI history with any new OIs and OI history logging stats
		if (patient->getDiseaseState()->infectedHIVState != SimContext::HIV_INF_NEG) {
			setOIHistory();
			updateOIHistoryLogging();
			// Update the ART suppression stats
			updateARTEfficacyStats();
		}

		// If tracing is enabled, print out either monthly status
		if (patient->getGeneralState()->tracingEnabled) {
			if (patient->getDiseaseState()->infectedHIVState != SimContext::HIV_INF_NEG) {
				if (patient->getGeneralState()->ageCategoryPediatrics < SimContext::PEDS_AGE_LATE) {
					tracer->printTrace(1, "  %d upd: true CD4 perc %1.2f %s, true HVL %s;\n", patient->getGeneralState()->monthNum,
						patient->getDiseaseState()->currTrueCD4Percentage, SimContext::CD4_STRATA_STRS[patient->getDiseaseState()->currTrueCD4Strata],
						SimContext::HVL_STRATA_STRS[patient->getDiseaseState()->currTrueHVLStrata]);
				}
				else {
					tracer->printTrace(1, "  %d upd: true CD4 %1.0f %s, true HVL %s;\n", patient->getGeneralState()->monthNum,
						patient->getDiseaseState()->currTrueCD4, SimContext::CD4_STRATA_STRS[patient->getDiseaseState()->currTrueCD4Strata],
						SimContext::HVL_STRATA_STRS[patient->getDiseaseState()->currTrueHVLStrata]);
				}
			}
			tracer->printTrace(1, "  %d mth: LM %1.2lf QA %1.2lf, $ %1.0lf;\n", patient->getGeneralState()->monthNum,
				patient->getGeneralState()->LMsDiscounted, patient->getGeneralState()->qualityAdjustLMsDiscounted,
				patient->getGeneralState()->costsDiscounted);
		}

		// Increment the simulation month number and patient age
		incrementMonth();
		// Increment the discount factor
		incrementDiscountFactor(simContext->getRunSpecsInputs()->discountFactor);
	}
	else {
		// Increment the patient and overall survival stats
		updatePatientSurvival(percentOfMonth);
		updateOverallSurvival(percentOfMonth);

		// Update population statistics and add the patient summary if death occurred
		updatePopulationStats();
		addPatientSummary();

		// If tracing is enabled, print out death tracing
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d DEATH %s;\n", patient->getGeneralState()->monthNum,
				SimContext::DTH_CAUSES_STRS[patient->getDiseaseState()->causeOfDeath]);
			tracer->printTrace(1, "  LMs %1.2lf QA %1.2lf $ %1.0lf ;\n",
				patient->getGeneralState()->LMsDiscounted,
				patient->getGeneralState()->qualityAdjustLMsDiscounted,
				patient->getGeneralState()->costsDiscounted);
			tracer->printTrace(1, "  END PATIENT\n\n");
		}
	}
} /* end performMonthlyUpdates */

