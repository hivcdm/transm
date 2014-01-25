#include "include.h"

/** \brief Constructor takes in the patient object */
TBDiseaseUpdater::TBDiseaseUpdater(Patient *patient) : StateUpdater(patient) {

}

/** \brief Destructor is empty, no cleanup required */
TBDiseaseUpdater::~TBDiseaseUpdater(void) {

}

/** \brief performInitialUpdates perform all of the state and statistics updates upon patient creation */
void TBDiseaseUpdater::performInitialUpdates() {
	/** First calls the parent function to perform general updates and initialization */
	StateUpdater::performInitialUpdates();

	/** Set the initial TB disease state, always none for HIV negative patients */
	if (patient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_NEG) {
		setTBDiseaseState(SimContext::TB_STATE_NO_HIST);
		return;
	}
	SimContext::TB_STATE tbState = SimContext::TB_STATE_NO_HIST;
	SimContext::TB_HIST_ACTV_STATE histActiveSubstate = SimContext::TB_HIST_ACTV_NO_HIST_ACTV;
	SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
	double randNum = CepacUtil::getRandomDouble(140010, patient);
	for (int i = 0; i < SimContext::TB_NUM_INIT_STATES; i++) {
		if ((simContext->getTBInputs()->distributionTBStateAtEntry[cd4Strata][i] > 0) &&
			(randNum < simContext->getTBInputs()->distributionTBStateAtEntry[cd4Strata][i])) {
				tbState = (SimContext::TB_STATE) i;
				if (tbState == SimContext::TB_STATE_HIST_ACTV)
					histActiveSubstate = SimContext::TB_HIST_ACTV_AFTER_TRUE;
				break;
		}
		randNum -= simContext->getTBInputs()->distributionTBStateAtEntry[cd4Strata][i];
	}
	setTBDiseaseState(tbState, histActiveSubstate);

	if (tbState != SimContext::TB_STATE_NO_HIST) {
		// Set the initial TB resistance strain
		SimContext::TB_STRAIN tbStrain = SimContext::TB_STRAIN_DS;
		randNum = CepacUtil::getRandomDouble(140020, patient);
		for (int j = 0; j < SimContext::TB_NUM_STRAINS; j++) {
			if ((simContext->getTBInputs()->distributionTBStrainAtEntry[j][tbState] > 0) &&
				(randNum < simContext->getTBInputs()->distributionTBStrainAtEntry[j][tbState])) {
					tbStrain = (SimContext::TB_STRAIN) j;
					break;
			}
			randNum -= simContext->getTBInputs()->distributionTBStrainAtEntry[j][tbState];
		}
		setTBResistanceStrain(tbStrain);

		/** If latent TB, set months since infection */
		if (tbState == SimContext::TB_STATE_LATENT) {
			randNum = CepacUtil::getRandomDouble(140030, patient);
			if (randNum < simContext->getTBInputs()->percentLatentTBIsEarly) {
				double monthsMean = simContext->getTBInputs()->monthsSinceEarlyLatentMean;
				double monthsStdDev = simContext->getTBInputs()->monthsSinceEarlyLatentStdDev;
				int months = (int) (CepacUtil::getRandomGaussian(monthsMean, monthsStdDev, 140040, patient) + 0.5);
				setNewTBInfection(SimContext::TB_INFECT_PREVALENT, false, months);
			}
			else {
				double monthsMean = simContext->getTBInputs()->monthsSinceLateLatentMean;
				double monthsStdDev = simContext->getTBInputs()->monthsSinceLateLatentStdDev;
				int months = (int) (CepacUtil::getRandomGaussian(monthsMean, monthsStdDev, 140050, patient) + 0.5);
				setNewTBInfection(SimContext::TB_INFECT_PREVALENT, false, months);
			}
		}

		/** If active TB, set months since infection */
		if (tbState == SimContext::TB_STATE_ACTIVE) {
			double monthsMean = simContext->getTBInputs()->monthsInfectedNotTreatedMean[tbStrain];
			double monthsStdDev = simContext->getTBInputs()->monthsInfectedNotTreatedStdDev[tbStrain];
			int months = (int) (CepacUtil::getRandomGaussian(monthsMean, monthsStdDev, 140060, patient) + 0.5);
			setNewTBInfection(SimContext::TB_INFECT_PREVALENT, true, months);
		}

		/** Add the initial TB state to the runStats for "at entry" */
		countInitialTBState();
	}
} /* end performInitialUpdates */

/** \brief performMonthlyUpdates perform all of the state and statistics updates for a simulated month
 *
 * Depending on Patient's current true TB disease state, call one of the following
 * 	- TBDiseaseUpdater::performNoHistoryTBUpdates()
 *  - TBDiseaseUpdater::performLatentTBUpdates()
 *  - TBDiseaseUpdater::performActiveTBUpdates()
 *  - TBDiseaseUpdater::performOnTreatmentTBUpdates()
 *  - TBDiseaseUpdater::performHistActiveTBUpdates()
 **/
void TBDiseaseUpdater::performMonthlyUpdates() {
	switch (patient->getTBState()->currTrueTBDiseaseState) {
		case SimContext::TB_STATE_NO_HIST:
			performNoHistoryTBUpdates();
			break;
		case SimContext::TB_STATE_LATENT:
			performLatentTBUpdates();
			break;
		case SimContext::TB_STATE_ACTIVE:
			performActiveTBUpdates();
			break;
		case SimContext::TB_STATE_TREATM_TRUE_SUCC:
		case SimContext::TB_STATE_TREATM_FALSE_SUCC:
		case SimContext::TB_STATE_TREATM_FAILING:
			performOnTreatmentTBUpdates();
			break;
		case SimContext::TB_STATE_HIST_ACTV:
			performHistActiveTBUpdates();
			break;
	}

	/** If acute TB occurred this month */
	if (patient->getDiseaseState()->hasCurrTrueOI && (patient->getDiseaseState()->typeCurrTrueOI == SimContext::OI_TB)) {
		/** Add the risk of mortality from acute TB */
		SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
		if (simContext->getTBInputs()->probAcuteMortality[cd4Strata] > 0) {
			std::cout << "We're adding mortality risk of " << simContext->getTBInputs()->probAcuteMortality[cd4Strata] << std::endl;
			addMortalityRisk(SimContext::DTH_OI_TB, simContext->getTBInputs()->probAcuteMortality[cd4Strata]);
		}

		/** If patient was on proph and developed acute TB, roll for prob of increased resistance */
		if (patient->getTBState()->isOnProph) {
			int prophNum = patient->getTBState()->currProphNum;
			SimContext::TB_STRAIN tbStrain = patient->getTBState()->currTrueTBResistanceStrain;
			double randNum = CepacUtil::getRandomDouble(140070, patient);
			if ((randNum < simContext->getTBInputs()->tbProphInputs[prophNum]->probIncreasedResistanceFailure) &&
				(tbStrain < SimContext::TB_STRAIN_XDR)) {
					increaseTBDrugResistance(false);
					SimContext::TB_STRAIN newTBStrain = patient->getTBState()->currTrueTBResistanceStrain;
					if (patient->getGeneralState()->tracingEnabled) {
						tracer->printTrace(1, "**%d TB PROPH INCR RESIST %s;\n",
							patient->getGeneralState()->monthNum, SimContext::TB_STRAIN_STRS[newTBStrain]);
					}
			}
		}

		/** If patient is undetected, roll for detection and schedule initial visit if so */
		bool rollForOIDet = true;
			if (patient->getMonitoringState()->isDetectedHIVPositive){
				rollForOIDet=false;
				if (simContext->getHIVTestInputs()->CD4TestAvailable && !patient->getMonitoringState()->hadPrevClinicVisit)
					rollForOIDet=true;
			}
		if (rollForOIDet) {
			double randNum = CepacUtil::getRandomDouble(140080, patient);
			if (randNum < simContext->getHIVTestInputs()->probHIVDetectionWithOI[SimContext::OI_TB]) {
				if (patient->getMonitoringState()->isDetectedHIVPositive){
					setDetectedHIVState(true, SimContext::HIV_DET_OI_PREV_DET, SimContext::OI_TB);
					setLinkedState(true, SimContext::HIV_DET_OI_PREV_DET);
				}
				else{
					setDetectedHIVState(true, SimContext::HIV_DET_OI, SimContext::OI_TB);
					setLinkedState(true, SimContext::HIV_DET_OI);
				}
				// Set this month as a clinic visit
				scheduleInitialClinicVisit();
				if (patient->getGeneralState()->tracingEnabled){
					if (patient->getMonitoringState()->isDetectedHIVPositive)
						tracer->printTrace(1, "**%d HIV DETECTED BY OI PREV DETECTED;\n", patient->getGeneralState()->monthNum);
					else
						tracer->printTrace(1, "**%d HIV DETECTED BY OI;\n", patient->getGeneralState()->monthNum);
				}
			}
		}
		else {
			/** If patient is detected and goes to clinic for OIs, trigger a clinic visit this month */
			if (patient->getMonitoringState()->clinicVisitType != SimContext::CLINIC_INITIAL)
				scheduleEmergencyClinicVisit(true, patient->getGeneralState()->monthNum);
		}
	}

	/** Add the extended TB mortality risk from a current or previous infection */
	SimContext::TB_STATE tbState = patient->getTBState()->currTrueTBDiseaseState;
	double probTBDeath = 0.0;
	if ((tbState == SimContext::TB_STATE_ACTIVE) || (tbState == SimContext::TB_STATE_TREATM_FAILING)) {
		int monthsInfect = patient->getGeneralState()->monthNum - patient->getTBState()->monthOfTBInfection;
		if (monthsInfect > 0) {
			SimContext::CD4_STRATA currCD4 = patient->getDiseaseState()->currTrueCD4Strata;
			SimContext::TB_STRAIN tbStrain = patient->getTBState()->currTrueTBResistanceStrain;
			probTBDeath = simContext->getTBInputs()->probExtendedMortalityOffART[tbStrain][currCD4][SimContext::TB_MTH_PERIODS_NUM];
			for (int i = 0; i < SimContext::TB_MTH_PERIODS_NUM; i++) {
				if (monthsInfect < simContext->getTBInputs()->probExtendedMortalityStageBoundsOffART[tbStrain][i]) {
					probTBDeath = simContext->getTBInputs()->probExtendedMortalityOffART[tbStrain][currCD4][i];
					break;
				}
			}
			/** Adjust for ART effect */
			if (patient->getARTState()->isOnART) {
				double rateMult = simContext->getTBInputs()->multiplierExtendedMortalityOnART[currCD4];
				/** Adjust multiplier between full ART effect and no ART effect according
				//	to the factor from the ART response type */
				rateMult = 1 - (patient->getARTState()->responseFactorCurrRegimen[SimContext::HET_OUTCOME_ARTEFFECT_MORT] * (1 - rateMult));
				probTBDeath = CepacUtil::probRateMultiply(probTBDeath, rateMult);
			}
		}
	}
	else if (tbState == SimContext::TB_STATE_HIST_ACTV) {
		SimContext::CD4_STRATA currCD4 = patient->getDiseaseState()->currTrueCD4Strata;
		SimContext::TB_STRAIN tbStrain = patient->getTBState()->currTrueTBResistanceStrain;
		probTBDeath = simContext->getTBInputs()->probExtendedMortalityOffART[tbStrain][currCD4][SimContext::TB_MTH_PERIODS_NUM];
		// Adjust for ART effect
		if (patient->getARTState()->isOnART) {
			double rateMult = simContext->getTBInputs()->multiplierExtendedMortalityOnART[currCD4];
			// Adjust multiplier between full ART effect and no ART effect according
			//	to the factor from the ART response type
			rateMult = 1 - (patient->getARTState()->responseFactorCurrRegimen[SimContext::HET_OUTCOME_ARTEFFECT_MORT] * (1 - rateMult));
			probTBDeath = CepacUtil::probRateMultiply(probTBDeath, rateMult);
		}
	}
	if (probTBDeath > 0) {
		addMortalityRisk(SimContext::DTH_OI_TB, probTBDeath);
	}

	/** If patient is scheduled to begin TB treatment this month or has reached the
	//	duration of TB treatment, trigger an emergency clinic visit */
	if ((patient->getTBState()->currTrueTBDiseaseState == SimContext::TB_STATE_ACTIVE) &&
		patient->getTBState()->isScheduledForTreatment &&
		(patient->getGeneralState()->monthNum >= patient->getTBState()->monthOfTreatmentStart)) {
			scheduleEmergencyClinicVisit(true, patient->getGeneralState()->monthNum);
	}
	else if (patient->getTBState()->isOnTreatment) {
		int monthsTreat = patient->getGeneralState()->monthNum - patient->getTBState()->monthOfTreatmentStart;
		SimContext::TB_TREATM_STAGE treatStage = patient->getTBState()->currTreatmentStage;
		if (monthsTreat >= simContext->getTBInputs()->monthsTreatmentDuration[treatStage]) {
			scheduleEmergencyClinicVisit(true, patient->getGeneralState()->monthNum);
		}
	}
} /* end performMonthlyUpdates */

/** \brief performNoHistoryTBUpdates determines if a first TB infection occurs from the no history state*/
void TBDiseaseUpdater::performNoHistoryTBUpdates() {
	const SimContext::TBInputs *tbInputs = simContext->getTBInputs();

	/** Calculate the probability of a first infection */
	SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
	double probInfect = tbInputs->probInfectionNoHistoryOffART[cd4Strata];
	/** Adjust for ART effect */
	if (patient->getARTState()->isOnART) {
		int monthsOnART = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart;
		for (int i = 0; i < SimContext::TB_MTH_PERIODS_NUM; i++) {
			if (monthsOnART < tbInputs->multiplierInfectionStageBoundsNoHistoryOnART[i]) {
				double rateMult = tbInputs->multiplierInfectionNoHistoryOnART[cd4Strata][i];
				/** Adjust multiplier between full ART effect and no ART effect according
				//	to the factor from the ART response type */
				rateMult = 1 - (patient->getARTState()->responseFactorCurrRegimen[SimContext::HET_OUTCOME_ARTEFFECT_OI] * (1 - rateMult));
				probInfect = CepacUtil::probRateMultiply(probInfect, rateMult);
				break;
			}
		}
	}
	/** Modify prob by proph efficacy if on TB proph */
	if (patient->getTBState()->isOnProph) {
		int prophNum = patient->getTBState()->currProphNum;
		probInfect = CepacUtil::probRateMultiply(probInfect, 1 - simContext->getTBInputs()->tbProphInputs[prophNum]->efficacyNoHistory);
	}

	/** Determine if infection occurs */
	double randNum = CepacUtil::getRandomDouble(140090, patient);
	if (randNum < probInfect) {
		/** Roll for resistance strain of infection */
		SimContext::TB_STRAIN newTBStrain = SimContext::TB_STRAIN_DS;
		randNum = CepacUtil::getRandomDouble(140100, patient);
		for (int i = 0; i < SimContext::TB_NUM_STRAINS; i++) {
			if (randNum < tbInputs->distributionInfectionStrainNoHistory[i]) {
				newTBStrain = (SimContext::TB_STRAIN) i;
				break;
			}
			randNum -= tbInputs->distributionInfectionStrainNoHistory[i];
		}

		/** Roll for active or latent infection */
		SimContext::TB_STATE newTBState = SimContext::TB_STATE_LATENT;
		randNum = CepacUtil::getRandomDouble(140110, patient);
		if (randNum < tbInputs->probActiveInfectionNoHistory) {
			newTBState = SimContext::TB_STATE_ACTIVE;
		}

		/** Update TB state for initial infection */
		setTBDiseaseState(newTBState);
		setTBResistanceStrain(newTBStrain);
		setNewTBInfection(SimContext::TB_INFECT_INITIAL, (newTBState == SimContext::TB_STATE_ACTIVE));

		/** Output tracing if enabled */
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d TB INFECTION %s %s;\n", patient->getGeneralState()->monthNum,
				SimContext::TB_STRAIN_STRS[patient->getTBState()->currTrueTBResistanceStrain],
				SimContext::TB_STATE_STRS[patient->getTBState()->currTrueTBDiseaseState]);
		}

		/** If active TB, set as an acute OI */
		if (newTBState == SimContext::TB_STATE_ACTIVE) {
			setCurrTrueOI(SimContext::OI_TB);
		}
	}
} /* end performNoHistoryTBUpdates */

/** \brief performLatentTBUpdates determines if TB reactivates or a reinfection occurs from the latent state */
void TBDiseaseUpdater::performLatentTBUpdates() {
	const SimContext::TBInputs *tbInputs = simContext->getTBInputs();

	/** Calculate probability of reactivation */
	double probReactivate = 0;
	SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
	SimContext::TB_STRAIN tbStrain = patient->getTBState()->currTrueTBResistanceStrain;
	for (int i = 0; i < SimContext::TB_MTH_PERIODS_NUM - 1; i++) {
		int monthsInfected = patient->getGeneralState()->monthNum - patient->getTBState()->monthOfTBInfection;
		if (monthsInfected < tbInputs->probReactivationStageBoundsLatentOffART[i]) {
			probReactivate = tbInputs->probReactivationLatentOffART[cd4Strata][tbStrain][i];
			break;
		}
	}
	/** Modify prob by ART effect if on ART */
	if (patient->getARTState()->isOnART) {
		int monthsOnART = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart;
		for (int i = 0; i < SimContext::TB_MTH_PERIODS_NUM; i++) {
			if (monthsOnART < tbInputs->multiplierReactivationStageBoundsLatentOnART[i]) {
				double rateMult = tbInputs->multiplierReactivationLatentOnART[cd4Strata][i];
				/** Adjust multiplier between full ART effect and no ART effect according
				//	to the factor from the ART response type */
				rateMult = 1 - (patient->getARTState()->responseFactorCurrRegimen[SimContext::HET_OUTCOME_ARTEFFECT_OI] * (1 - rateMult));
				probReactivate = CepacUtil::probRateMultiply(probReactivate, rateMult);
				break;
			}
		}
	}
	/** Modify prob by proph efficacy if on TB proph */
	if (patient->getTBState()->isOnProph) {
		int prophNum = patient->getTBState()->currProphNum;
		SimContext::TB_STRAIN tbStrain = patient->getTBState()->currTrueTBResistanceStrain;
		probReactivate = CepacUtil::probRateMultiply(probReactivate, 1 - simContext->getTBInputs()->tbProphInputs[prophNum]->efficacyReactivation[tbStrain]);
	}

	/** Determine the probability of reinfection, modify by ART effect */
	double probReinfect = tbInputs->probReinfectionLatentOffART[cd4Strata];
	if (patient->getARTState()->isOnART) {
		int monthsOnART = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart;
		for (int i = 0; i < SimContext::TB_MTH_PERIODS_NUM; i++) {
			if (monthsOnART < tbInputs->multiplierReinfectionStageBoundsLatentOnART[i]) {
				double rateMult = tbInputs->multiplierReinfectionLatentOnART[cd4Strata][i];
				/** Adjust multiplier between full ART effect and no ART effect according
				//	to the factor from the ART response type */
				rateMult = 1 - (patient->getARTState()->responseFactorCurrRegimen[SimContext::HET_OUTCOME_ARTEFFECT_OI] * (1 - rateMult));
				probReinfect = CepacUtil::probRateMultiply(probReinfect, rateMult);
				break;
			}
		}
	}
	/** Modify prob by proph efficacy if on TB proph */
	if (patient->getTBState()->isOnProph) {
		int prophNum = patient->getTBState()->currProphNum;
		SimContext::TB_STRAIN tbStrain = patient->getTBState()->currTrueTBResistanceStrain;
		probReinfect = CepacUtil::probRateMultiply(probReinfect, 1 - simContext->getTBInputs()->tbProphInputs[prophNum]->efficacyReinfection[tbStrain]);
	}

	/** Calculate probability and roll for neither reactivation or reinfection, return in neither occurs */
	double probNoInfect = (1 - probReactivate) * (1 - probReinfect);
	double randNum = CepacUtil::getRandomDouble(140120, patient);
	if (randNum < probNoInfect)
		return;

	/** Either reactivation or reinfection occurred, determine normalized distribution of infection type */
	double probOnlyReactivate = probReactivate * (1 - probReinfect);
	double probOnlyReinfect = probReinfect * (1 - probReactivate);
	double distReactivate = 1.0;
	if ((probOnlyReactivate + probOnlyReinfect) > 0)
		distReactivate = probOnlyReactivate / (probOnlyReactivate + probOnlyReinfect);

	/** Roll for reactivation occurring, otherwise reinfection occurred */
	randNum = CepacUtil::getRandomDouble(140130, patient);
	if (randNum < distReactivate) {
		setTBDiseaseState(SimContext::TB_STATE_ACTIVE);
		setNewTBInfection(SimContext::TB_INFECT_REACTIVATE, true);

		// Output tracing if enabled
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d TB REACTIVATION %s %s;\n", patient->getGeneralState()->monthNum,
				SimContext::TB_STRAIN_STRS[patient->getTBState()->currTrueTBResistanceStrain],
				SimContext::TB_STATE_STRS[patient->getTBState()->currTrueTBDiseaseState]);
		}

		/** If reactivation, set as an acute OI and return instead of rolling for reinfection */
		setCurrTrueOI(SimContext::OI_TB);
	}
	else {
		/** Roll for resistance strain of reinfection while not on ART*/
		SimContext::TB_STRAIN newTBStrain = SimContext::TB_STRAIN_DS;
		randNum = CepacUtil::getRandomDouble(140140, patient);
		for (int i = 0; i < SimContext::TB_NUM_STRAINS; i++) {
			if (randNum < tbInputs->distributionReinfectionStrainLatent[i]) {
				newTBStrain = (SimContext::TB_STRAIN) i;
				break;
			}
			randNum -= tbInputs->distributionReinfectionStrainLatent[i];
		}
		/** Use reinfection supercede option to determine if old strain supercedes new one */
		if ((tbInputs->reinfectionSupercedeOption == SimContext::TB_REINFECT_SENS_OVER_RESIST) &&
			(newTBStrain > tbStrain))
			newTBStrain = tbStrain;
		else if ((tbInputs->reinfectionSupercedeOption == SimContext::TB_REINFECT_RESIST_OVER_SENS) &&
			(newTBStrain < tbStrain))
			newTBStrain = tbStrain;

		/** Roll for active or latent infection */
		SimContext::TB_STATE newTBState = SimContext::TB_STATE_LATENT;
		randNum = CepacUtil::getRandomDouble(140150, patient);
		if (randNum < tbInputs->probActiveReinfectionLatent) {
			newTBState = SimContext::TB_STATE_ACTIVE;
		}

		/** Update TB state for reinfection */
		setTBDiseaseState(newTBState);
		setTBResistanceStrain(newTBStrain);
		setNewTBInfection(SimContext::TB_INFECT_REINFECT, (newTBState == SimContext::TB_STATE_ACTIVE));

		// Output tracing if enabled
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d TB REINFECTION %s %s;\n", patient->getGeneralState()->monthNum,
				SimContext::TB_STRAIN_STRS[patient->getTBState()->currTrueTBResistanceStrain],
				SimContext::TB_STATE_STRS[patient->getTBState()->currTrueTBDiseaseState]);
		}

		/** If active, set as an acute OI */
		if (newTBState == SimContext::TB_STATE_ACTIVE) {
			setCurrTrueOI(SimContext::OI_TB);
		}
	}
} /* end performLatentTBUpdates */

/** \brief performActiveTBUpdates determines if a spontaneous resolution occurs from the active state */
void TBDiseaseUpdater::performActiveTBUpdates() {
	const SimContext::TBInputs *tbInputs = simContext->getTBInputs();

	/** Calculate probability of spontaneous resolution */
	SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
	SimContext::TB_STRAIN tbStrain = patient->getTBState()->currTrueTBResistanceStrain;
	double probResolve = tbInputs->probSpontaneousResolution[tbStrain][cd4Strata][SimContext::TB_MTH_PERIODS_NUM];
	for (int i = 0; i < SimContext::TB_MTH_PERIODS_NUM; i++) {
		int monthsInfect = patient->getGeneralState()->monthNum - patient->getTBState()->monthOfTBInfection;
		if (monthsInfect < tbInputs->probSpontaneousResolutionStageBounds[tbStrain][i]) {
			probResolve = tbInputs->probSpontaneousResolution[tbStrain][cd4Strata][i];
			break;
		}
	}

	/** Roll for spontaneous resolution and update state */
	double randNum = CepacUtil::getRandomDouble(140160, patient);
	if (randNum < probResolve) {
		/** If resolved, set to history of active, with a substate of self cure */
		setTBDiseaseState(SimContext::TB_STATE_HIST_ACTV, SimContext::TB_HIST_ACTV_AFTER_SELF);
		setTBSpontaneousResolution();

		/** If resolution, unschedule pending TB treatment if one is scheduled */
		if (patient->getTBState()->isScheduledForTreatment)
			unscheduleNextTBTreatment();

		/** Output tracing if enabled */
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d TB SPONT RESOL %s %s;\n", patient->getGeneralState()->monthNum,
				SimContext::TB_STRAIN_STRS[patient->getTBState()->currTrueTBResistanceStrain],
				SimContext::TB_STATE_STRS[patient->getTBState()->currTrueTBDiseaseState]);
		}
	}
} /* end performActiveTBUpdates */

/** \brief performOnTreatmentTBUpdates determines any disease changes while on treatment */
void TBDiseaseUpdater::performOnTreatmentTBUpdates() {
	/** Does nothing for now, treatment outcomes/changes are handled in ClinicVisitUpdater */
} /* end performOnTreatmentTBUpdates */

/** \brief performHistActiveTBUpdates determines if TB relapse occurs from the history of active state */
void TBDiseaseUpdater::performHistActiveTBUpdates() {
	const SimContext::TBInputs *tbInputs = simContext->getTBInputs();

	/** Calculate probability of relapse, modify by ART effect */
	/** Adjust by style of history of active state */
	SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
	SimContext::TB_STRAIN tbStrain = patient->getTBState()->currTrueTBResistanceStrain;
	double probRelapse = tbInputs->probRelapseHistoryActiveAfterTrueCureOffART[cd4Strata];
	if (patient->getARTState()->isOnART) {
		int monthsOnART = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart;
		for (int i = 0; i < SimContext::TB_MTH_PERIODS_NUM; i++) {
			if (monthsOnART < tbInputs->multiplierRelapseStageBoundsHistoryActiveAfterTrueCureOnART[i]) {
				double rateMult = tbInputs->multiplierRelapseHistoryActiveAfterTrueCureOnART[cd4Strata][i];
				/** Use a different probability of relapse based on the history of active substate */
				switch (patient->getTBState()->currHistOfActiveSubstate) {
					case SimContext::TB_HIST_ACTV_AFTER_FALSE:
						rateMult = tbInputs->multiplierRelapseHistoryActiveAfterFalseCureOnART[cd4Strata][i]; break;
					case SimContext::TB_HIST_ACTV_AFTER_SELF:
						rateMult = tbInputs->multiplierRelapseHistoryActiveAfterSelfCureOnART[cd4Strata][i]; break;
					case SimContext::TB_HIST_ACTV_AFTER_TRUE:
						break;
				}
				/** Adjust multiplier between full ART effect and no ART effect according
				//	to the factor from the ART response type */
				rateMult = 1 - (patient->getARTState()->responseFactorCurrRegimen[SimContext::HET_OUTCOME_ARTEFFECT_OI] * (1 - rateMult));
				probRelapse = CepacUtil::probRateMultiply(probRelapse, rateMult);
				break;
			}
		}
	}

	/** Roll for relapse and update patient state */
	double randNum = CepacUtil::getRandomDouble(140170, patient);
	if (randNum < probRelapse) {
		setTBDiseaseState(SimContext::TB_STATE_ACTIVE);
		setNewTBInfection(SimContext::TB_INFECT_RELAPSE, true);

		/** Output tracing if enabled */
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d TB RELAPSE %s %s;\n", patient->getGeneralState()->monthNum,
				SimContext::TB_STRAIN_STRS[patient->getTBState()->currTrueTBResistanceStrain],
				SimContext::TB_STATE_STRS[patient->getTBState()->currTrueTBDiseaseState]);
		}

		/** If TB occurs, set as an acute OI */
		setCurrTrueOI(SimContext::OI_TB);
	}
} /* end performHistActiveTBUpdates */

