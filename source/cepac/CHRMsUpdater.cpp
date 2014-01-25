#include "include.h"

/** \brief Constructor takes in the associated patient object */
CHRMsUpdater::CHRMsUpdater(Patient *patient) : StateUpdater(patient) {

}

/** \brief Destructor is empty, no cleanup required */
CHRMsUpdater::~CHRMsUpdater(void) {

}

/** \brief performInitialUpdates perform all of the state and statistics updates upon patient creation */
void CHRMsUpdater::performInitialUpdates() {
	/** Calls the parent function to perform general updates and initialization */
	StateUpdater::performInitialUpdates();

	/** Roll for prevalent CHRMs at model entry */
	for (int i = 0; i < SimContext::CHRM_NUM; i++) {
		// Set the base prevalence probability
		double probCHRM = 0.0;
		SimContext::GENDER_TYPE gender = patient->getGeneralState()->gender;
		int ageCat = patient->getGeneralState()->ageCategoryCHRMs;
		SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
		if (patient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_NEG) {
			probCHRM = simContext->getCHRMsInputs()->probPrevalentCHRMsHIVneg[i][gender][ageCat];
		}
		else {
			probCHRM = simContext->getCHRMsInputs()->probPrevalentCHRMs[i][cd4Strata][gender][ageCat];
		}

		/** Modify probability by generic risk factor logit adjustments */
		double logitCHRM = CepacUtil::probToLogit(probCHRM);
		for (int j = 0; j < SimContext::RISK_FACT_NUM; j++) {
			if (patient->getGeneralState()->hasRiskFactor[j])
				logitCHRM += simContext->getCHRMsInputs()->probPrevalentCHRMsRiskFactorLogit[i][j];
		}
		probCHRM = CepacUtil::logitToProb(logitCHRM);

		/** Determine if CHRM occurs and update state */
		double randNum = CepacUtil::getRandomDouble(150010, patient);
		if (randNum < probCHRM) {
			// Patient has CHRM i at model entry, roll for month of start and update state
			double monthsMean = simContext->getCHRMsInputs()->prevalentCHRMsMonthsSinceStartMean[i];
			double monthsStdDev = simContext->getCHRMsInputs()->prevalentCHRMsMonthsSinceStartStdDev[i];
			int monthsStart = (int) (CepacUtil::getRandomGaussian(monthsMean, monthsStdDev, 150020, patient) + 0.5);
			setTrueCHRMsState(i, true, true, monthsStart);
		}
		else {
			setTrueCHRMsState(i, false);
		}
	}
} /* end performInitialUpdates */

/** \brief performMonthlyUpdates perform all of the state and statistics updates for a simulated month */
void CHRMsUpdater::performMonthlyUpdates() {
	/** Roll for patient developing each CHRM if they don't already have it */
	for (int i = 0; i < SimContext::CHRM_NUM; i++) {
		if (!patient->getDiseaseState()->hasTrueCHRMs[i]) {
			/** - Set the base incidence probability */
			SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
			SimContext::GENDER_TYPE gender = patient->getGeneralState()->gender;
			int ageCat = patient->getGeneralState()->ageCategoryCHRMs;
			double probCHRM = 0.0;
			if (patient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_NEG) {
				probCHRM = simContext->getCHRMsInputs()->probIncidentCHRMsHIVneg[i][gender][ageCat];
			}
			else {
				probCHRM = simContext->getCHRMsInputs()->probIncidentCHRMs[i][cd4Strata][gender][ageCat];
			}

			/** - If on ART, modify probability by on ART rate multiplier */
			if (patient->getARTState()->isOnART) {
				double rateMult = simContext->getCHRMsInputs()->probIncidentCHRMsOnARTMult[i][cd4Strata];
				/** 	- Adjust multiplier between full ART effect and no ART effect according	to the factor from the ART response type */
				rateMult = 1 - (patient->getARTState()->responseFactorCurrRegimen[SimContext::HET_OUTCOME_ARTEFFECT_CHRMS] * (1 - rateMult));
				probCHRM = CepacUtil::probRateMultiply(probCHRM, rateMult);
			}

			/** - Modify probability by generic risk factor logit adjustments */
			double logitCHRM = CepacUtil::probToLogit(probCHRM);
			for (int j = 0; j < SimContext::RISK_FACT_NUM; j++) {
				if (patient->getGeneralState()->hasRiskFactor[j])
					logitCHRM += simContext->getCHRMsInputs()->probIncidentCHRMsRiskFactorLogit[i][j];
			}

			/** - Modify probability by history of other CHRMs logit adjustments */
			for (int j = 0; j < SimContext::CHRM_NUM; j++) {
				if (patient->getDiseaseState()->hasTrueCHRMs[j] &&
					(patient->getDiseaseState()->monthOfCHRMsStart[j] < patient->getGeneralState()->monthNum)) {
						logitCHRM += simContext->getCHRMsInputs()->probIncidentCHRMsPriorHistoryLogit[i][j];
				}
			}
			probCHRM = CepacUtil::logitToProb(logitCHRM);

			/** Determine if CHRM occurs and update state if so */
			double randNum = CepacUtil::getRandomDouble(150030, patient);
			if (randNum < probCHRM) {
				setTrueCHRMsState(i, true);
				/** Print out tracing for the incidence of the CHRM */
				if (patient->getGeneralState()->tracingEnabled) {
					tracer->printTrace(1, "**%d INCIDENT CHRMs %s;\n",
							patient->getGeneralState()->monthNum, SimContext::CHRM_STRS[i]);
				}
			}
		}
	}

	double totalProbDeath=0;
	double maxProbDeath=0;
	double probDeathArray[SimContext::CHRM_NUM];
	/** For each CHRM condition that exists, handle the costs, QOL, and risk of mortality */
	for (int i = 0; i < SimContext::CHRM_NUM; i++) {
		probDeathArray[i]=0;
		if (patient->getDiseaseState()->hasTrueCHRMs[i]) {
			/** Get the stage of CHRMs and patient state */
			SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
			SimContext::GENDER_TYPE gender = patient->getGeneralState()->gender;
			int ageCat = patient->getGeneralState()->ageCategoryCHRMs;
			int monthsSince = patient->getGeneralState()->monthNum - patient->getDiseaseState()->monthOfCHRMsStart[i];
			int stage = SimContext::CHRM_TIME_PER_NUM - 1;
			for (int j = 0; j < SimContext::CHRM_TIME_PER_NUM - 1; j++) {
				if (monthsSince <= simContext->getCHRMsInputs()->probDeathCHRMsStageBounds[i][j]) {
					stage = j;
					break;
				}
			}

			/** Add the risk of mortality */
			double probDeath = 0.0;
			if (patient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_NEG) {
				probDeath = simContext->getCHRMsInputs()->probDeathCHRMsHIVneg[i][stage][gender][ageCat];
			}
			else {
				probDeath = simContext->getCHRMsInputs()->probDeathCHRMs[i][stage][cd4Strata][gender][ageCat];
			}
			if(probDeath>maxProbDeath){
				maxProbDeath=probDeath;
			}
			probDeathArray[i]=probDeath;
			totalProbDeath+=probDeath;

			/** Accumulate the costs of each CHRM */
			double costCHRM = simContext->getCHRMsInputs()->costCHRMs[i][stage][gender][ageCat];
			incrementCostsCHRMs((SimContext::CHRM_TYPE)i,costCHRM);

			/** Accumulate the QOL multipliers for each CHRM */
			double qolCHRM = simContext->getCHRMsInputs()->QOLMultCHRMs[i][stage][gender][ageCat];
			accumulateQOLMultiplier(qolCHRM);
		}
	}

	if(totalProbDeath>0){
		double partialSumProbDeath=0.0;
		double randNum=CepacUtil::getRandomDouble(150040, patient);
		for (int i = 0; i < SimContext::CHRM_NUM; i++) {
			partialSumProbDeath+=probDeathArray[i]/totalProbDeath;
			if (randNum<=partialSumProbDeath) {
				SimContext::DTH_CAUSES causeDeath = (SimContext::DTH_CAUSES) (SimContext::DTH_CHRM_1 + i);
				addMortalityRisk(causeDeath, maxProbDeath);
				break;
			}
		}
	}



} /* end performMonthlyUpdates */
