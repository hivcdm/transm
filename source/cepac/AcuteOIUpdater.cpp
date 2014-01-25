#include "include.h"

/** \brief Constructor takes in the associated patient object */
AcuteOIUpdater::AcuteOIUpdater(Patient *patient) : StateUpdater(patient) {

}

/** \brief Destructor is empty, no cleanup required */
AcuteOIUpdater::~AcuteOIUpdater(void) {

}

/** \brief performInitialUpdates perform all of the state and statistics updates upon patient creation */
void AcuteOIUpdater::performInitialUpdates() {
	/** Calls the parent function to perform general updates and initialization */
	StateUpdater::performInitialUpdates();

	/** If patient has chronic HIV, determine the OI history and asymp/symp state */
	if (patient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_ASYMP_CHR_POS) {
		/** Draw the initial OI history */
		bool hasHistory[SimContext::OI_NUM];
		for (int i = 0; i < SimContext::OI_NUM; i++) {
			SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
			SimContext::HVL_STRATA hvlStrata = patient->getDiseaseState()->currTrueHVLStrata;
			double randNum = CepacUtil::getRandomDouble(10010, patient);
			if (randNum < simContext->getCohortInputs()->probOIHistoryAtEntry[cd4Strata][hvlStrata][i]) {
				hasHistory[i] = true;
			}
			else {
				hasHistory[i] = false;
			}
		}
		if ((patient->getTBState()->currTrueTBDiseaseState == SimContext::TB_STATE_ACTIVE) ||
			(patient->getTBState()->currTrueTBDiseaseState == SimContext::TB_STATE_HIST_ACTV)) {
				hasHistory[SimContext::OI_TB] = true;
		}
		setInitialOIHistory(hasHistory);

		/** Update the HIV state to symptomatic if there is a history of OIs */
		if (patient->getDiseaseState()->typeTrueOIHistory != SimContext::HIST_EXT_N) {
			setInfectedHIVState(SimContext::HIV_INF_SYMP_CHR_POS, true);
		}
	}
	else {
		/** Otherwise, set initial OI history to none */
		bool hasHistory[SimContext::OI_NUM];
		for (int i = 0; i < SimContext::OI_NUM; i++) {
			hasHistory[i] = false;
		}
		setInitialOIHistory(hasHistory);
	}
} /* end performInitialUpdates */

/** \brief performMonthlyUpdates perform all of the state and statistics updates for a simulated month */
void AcuteOIUpdater::performMonthlyUpdates() {
	/** Determine if an acute OI occurs this month, return if not */
	SimContext::OI_TYPE oiType = determineAcuteOI();
	if (oiType == SimContext::OI_NONE)
		return;


	/** Update state for the occurrence of the OI */
	setCurrTrueOI(oiType);
	if (patient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_ASYMP_CHR_POS)
		setInfectedHIVState(SimContext::HIV_INF_SYMP_CHR_POS);

	/** Determine prob of death from this OI and add to mortality factors */
	determineDeathByOI(oiType);

	/** Print out tracing message for OI occurrence */
	if (patient->getGeneralState()->tracingEnabled) {
		if (!patient->getDiseaseState()->hasTrueOIHistory[oiType]) {
			tracer->printTrace(1, "**%d PRIMARY OI %s;\n", patient->getGeneralState()->monthNum,
				SimContext::OI_STRS[oiType]);
		}
		else {
			tracer->printTrace(1, "**%d SECONDARY OI %s;\n", patient->getGeneralState()->monthNum,
				SimContext::OI_STRS[oiType]);
		}
	}

	/** If patient is undetected, determine if detected by OI and schedule tests if so */
	bool rollForOIDet = true;
	if (patient->getMonitoringState()->isDetectedHIVPositive){
		rollForOIDet=false;
		if (simContext->getHIVTestInputs()->CD4TestAvailable && !patient->getMonitoringState()->hadPrevClinicVisit)
			rollForOIDet=true;
	}
	if (rollForOIDet) {
		double randNum = CepacUtil::getRandomDouble(10020, patient);
		if (randNum < simContext->getHIVTestInputs()->probHIVDetectionWithOI[oiType]) {
			if (patient->getGeneralState()->tracingEnabled){
				if (patient->getMonitoringState()->isDetectedHIVPositive)
					tracer->printTrace(1, "**%d HIV DETECTED BY OI PREV DETECTED;\n", patient->getGeneralState()->monthNum);
				else
					tracer->printTrace(1, "**%d HIV DETECTED BY OI;\n", patient->getGeneralState()->monthNum);
			}
			if (patient->getMonitoringState()->isDetectedHIVPositive){
				setDetectedHIVState(true, SimContext::HIV_DET_OI_PREV_DET, oiType);
				setLinkedState(true, SimContext::HIV_DET_OI_PREV_DET);
			}
			else{
				setDetectedHIVState(true, SimContext::HIV_DET_OI, oiType);
				setLinkedState(true, SimContext::HIV_DET_OI);
			}
			// Set this month as a clinic visit
			scheduleInitialClinicVisit();
		}
	}
	else {
		/** If patient is detected and goes to clinic for OIs, trigger a clinic visit this month */
		if (patient->getMonitoringState()->clinicVisitType != SimContext::CLINIC_INITIAL)
			scheduleEmergencyClinicVisit(true, patient->getGeneralState()->monthNum);
	}

	/** Determine if patient should receive acute OI treatment */
	/**	ISSUE - clinic visit type and going to clinic/treatment for OIs is poorly specified */
	bool treatOI = false;
	if (patient->getMonitoringState()->isDetectedHIVPositive) {
		if (patient->getMonitoringState()->clinicVisitType != SimContext::CLINIC_INITIAL) {
			if (patient->getMonitoringState()->currLTFUState == SimContext::LTFU_STATE_LOST) {
				double randNum = CepacUtil::getRandomDouble(10030, patient);
				if (randNum < simContext->getLTFUInputs()->probRemainOnOITreatment) {

					treatOI = true;
				}
				if (patient->getGeneralState()->tracingEnabled) {
					tracer->printTrace(1, "**%d%sTREATING OI WHILE LTFU;\n", patient->getGeneralState()->monthNum,
						treatOI ? " " : " NOT ");
				}
			}
			else  {
				treatOI = true;
			}
		}
	}

	/** Accrue the cost of the acute OI treatment or non-treatment */
	const double *costOI;
	double costFactor = 1.0;
	SimContext::ART_STATES artState = (patient->getARTState()->isOnART) ? SimContext::ART_ON_STATE : SimContext::ART_OFF_STATE;
	SimContext::PEDS_COST_AGE costAgeCat=patient->getGeneralState()->ageCategoryPedsCost;
	if (treatOI) {
		if(costAgeCat==SimContext::PEDS_COST_AGE_ADULT){
			costOI = simContext->getCostInputs()->acuteOICostTreated[artState][oiType];
		}
		else{
			costOI= simContext->getPedsCostInputs()->acuteOICostTreated[costAgeCat][artState][oiType];
		}
	}
	else {
		if(costAgeCat==SimContext::PEDS_COST_AGE_ADULT){
			costOI = simContext->getCostInputs()->acuteOICostUntreated[artState][oiType];

		}
		else{
			costOI = simContext->getPedsCostInputs()->acuteOICostUntreated[costAgeCat][artState][oiType];

		}

	}

	/** Adjust costs for resistance penalty */
	if (patient->getProphState()->isOnProph[oiType] && patient->getProphState()->useProphResistance[oiType]) {
		int prophNum = patient->getProphState()->currProphNum[oiType];
		SimContext::PROPH_TYPE prophType = patient->getProphState()->currProphType[oiType];
		if (patient->getGeneralState()->ageCategoryPediatrics>=SimContext::PEDS_AGE_LATE){
			costFactor=1+simContext->getProphInputs(prophType,oiType,prophNum)->costFactorResistance;
		}
		else{
			costFactor = 1 + simContext->getPedsProphInputs(prophType,oiType,prophNum)->costFactorResistance;
		}
	}


	incrementCostsMisc(costOI, costFactor);
} /* end performMonthlyUpdates */

/** \brief determineAcuteOI returns the acute OI that occurs this month or OI_NONE if no OI occurs */
SimContext::OI_TYPE AcuteOIUpdater::determineAcuteOI() {
	const SimContext::NatHistInputs *natHist = simContext->getNatHistInputs();

	/** First calculate the maximum efficacy of OI prophylaxis */
	double maxEfficacy[SimContext::OI_NUM];
	bool hasEfficacy[SimContext::OI_NUM];
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		hasEfficacy[i] = false;
	}
	if (patient->getProphState()->currTotalNumProphsOn > 0) {
		for (int i = 0; i < SimContext::OI_NUM; i++) {
			if (patient->getProphState()->isOnProph[i]) {
				SimContext::PROPH_TYPE prophType = patient->getProphState()->currProphType[i];
				int prophNum = patient->getProphState()->currProphNum[i];
				const SimContext::ProphInputs *prophInput;
				if(patient->getGeneralState()->ageCategoryPediatrics>=SimContext::PEDS_AGE_LATE){
					prophInput=simContext->getProphInputs(prophType,i,prophNum);
				}
				else{
					prophInput=simContext->getPedsProphInputs(prophType,i,prophNum);
				}
				for (int j = 0; j < SimContext::OI_NUM; j++) {
					double efficacy = 0.0;
					if (!patient->getDiseaseState()->hasTrueOIHistory[j]) {
						efficacy = prophInput->primaryOIEfficacy[j];
					}
					else {
						efficacy = prophInput->secondaryOIEfficacy[j];
					}
					if (efficacy != SimContext::NOT_APPL) {
						if (!hasEfficacy[j] || (efficacy > maxEfficacy[j])) {
							hasEfficacy[j] = true;
							maxEfficacy[j] = efficacy;
						}
					}
				}
			}
		}
	}

	/** Calculate the probability of each OI occurring or no OIs occurring this month */
	SimContext::CD4_STRATA currCD4Strata = patient->getDiseaseState()->currTrueCD4Strata;
	SimContext::CD4_STRATA minCD4Strata = patient->getDiseaseState()->minTrueCD4Strata;
	SimContext::HIST_EXT oiHist = patient->getDiseaseState()->typeTrueOIHistory;
	double probOI[SimContext::OI_NUM];
	double probNoOIs = 1.0;
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		SimContext::HIST_TYPE histType = (patient->getDiseaseState()->hasTrueOIHistory[i]) ? SimContext::HIST_Y : SimContext::HIST_N;
		SimContext::PEDS_AGE_CAT pedsAgeCat = patient->getGeneralState()->ageCategoryPediatrics;
		if (pedsAgeCat == SimContext::PEDS_AGE_ADULT) {
			/** If an adult patient, use the adult OI probabilities */
			/** Extract the natural history probability based on CD4, and OI history,
				modify base probability by OI fraction of benefit */
			double fractionOfBenefit = simContext->getRunSpecsInputs()->OIsFractionOfBenefit[i];
			probOI[i] = fractionOfBenefit * natHist->monthlyOIProbOffART[currCD4Strata][i][histType] +
				(1 - fractionOfBenefit) * natHist->monthlyOIProbOffART[minCD4Strata][i][histType];
			/** Adjust for ART effect */
			if (patient->getARTState()->isOnART) {
				double rateMult = simContext->getNatHistInputs()->monthlyOIProbOnARTMult[currCD4Strata][i];
				/** Adjust multiplier between full ART effect and no ART effect according
					to the factor from the ART response type (heterogeneity)*/
				rateMult = 1 - (patient->getARTState()->responseFactorCurrRegimen[SimContext::HET_OUTCOME_ARTEFFECT_OI] * (1 - rateMult));
				probOI[i] = CepacUtil::probRateMultiply(probOI[i], rateMult);
			}
		}
		else if (pedsAgeCat == SimContext::PEDS_AGE_LATE) {
			/** If SimContext::PEDS_AGE_LATE, use the late childhood OI probabilities */
			probOI[i] = simContext->getPedsInputs()->probAcuteOILate[i][currCD4Strata][histType];
			/* If on ART, adjust for heterogeneity based pediatric ART effect */
			if (patient->getARTState()->isOnART) {
				double rateMult = simContext->getPedsInputs()->monthlyOIProbOnARTMultLate[currCD4Strata][i];
				// Adjust multiplier between full ART effect and no ART effect according
				//	to the factor from the ART response type
				rateMult = 1 - (patient->getARTState()->responseFactorCurrRegimen[SimContext::HET_OUTCOME_ARTEFFECT_OI] * (1 - rateMult));
				probOI[i] = CepacUtil::probRateMultiply(probOI[i], rateMult);
			}
		}
		else {
			/** If an early childhood patient, use the early childhood OI probabilities */
			SimContext::PEDS_CD4_PERC cd4PercStrata = patient->getDiseaseState()->currTrueCD4PercentageStrata;
			probOI[i] = simContext->getPedsInputs()->probAcuteOIEarly[i][pedsAgeCat][cd4PercStrata][histType];
			/** If on ART, adjust for heterogeneity based pediatric ART effect */
			if (patient->getARTState()->isOnART) {
				double rateMult = 1.0;
				int monthsOnART = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart;
				if (monthsOnART <= simContext->getPedsInputs()->stageBoundsMonthlyOIProbOnARTMultEarly[0])
					rateMult = simContext->getPedsInputs()->monthlyOIProbOnARTMultEarly[cd4PercStrata][0];
				else if (monthsOnART <= simContext->getPedsInputs()->stageBoundsMonthlyOIProbOnARTMultEarly[1])
					rateMult = simContext->getPedsInputs()->monthlyOIProbOnARTMultEarly[cd4PercStrata][1];
				else
					rateMult = simContext->getPedsInputs()->monthlyOIProbOnARTMultEarly[cd4PercStrata][2];
				// Adjust multiplier between full ART effect and no ART effect according
				//	to the factor from the ART response type
				rateMult = 1 - (patient->getARTState()->responseFactorCurrRegimen[SimContext::HET_OUTCOME_ARTEFFECT_OI] * (1 - rateMult));
				probOI[i] = CepacUtil::probRateMultiply(probOI[i], rateMult);
			}
		}

		/** Modify the probability by the maximum OI proph efficacy (alter by compliance/resistance) */
		if (hasEfficacy[i]) {
			if (patient->getProphState()->isNonCompliant)
				maxEfficacy[i] *= (1 - simContext->getCohortInputs()->OIProphNonComplianceDegree);
			if (patient->getProphState()->isOnProph[i] && patient->getProphState()->useProphResistance[i]) {
				SimContext::PROPH_TYPE prophType = patient->getProphState()->currProphType[i];
				int prophNum = patient->getProphState()->currProphNum[i];
				if (patient->getGeneralState()->ageCategoryPediatrics>=SimContext::PEDS_AGE_LATE){
					maxEfficacy[i] *= (1 - simContext->getProphInputs(prophType,i,prophNum)->percentResistance);
				}
				else{
					maxEfficacy[i] *= (1 - simContext->getPedsProphInputs(prophType,i,prophNum)->percentResistance);
				}

			}
			probOI[i] = CepacUtil::probRateMultiply(probOI[i], 1 - maxEfficacy[i]);
		}

		/** If probability is 1, return the OI since it must happen */
		if (probOI[i] == 1.0)
			return ((SimContext::OI_TYPE) i);

		/** Accumulate the probability of no OIs occurring: \f$ p_{noOI} = \prod_{i \in OIs} (1 - prob_{OI_i})\f$ */
		probNoOIs *= (1 - probOI[i]);
	}

	/** Roll for No OIs occurring, return OI_NONE if none occur */
	double randNum = CepacUtil::getRandomDouble(10040, patient);
	if (randNum < probNoOIs)
		return SimContext::OI_NONE;

	/** Treat prob of OIs as independent probabilities, calculate prob of each OI occurring with
		none of the other ones and normalize these combined probabilities */
	double sumOfProbs = 0.0;
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		probOI[i] = (probNoOIs / (1 - probOI[i])) * probOI[i];
		sumOfProbs += probOI[i];
	}
	/** Use the distribution of normalized probs to determine which OI occurs */
	randNum = CepacUtil::getRandomDouble(10050, patient);
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		probOI[i] = probOI[i] / sumOfProbs;
		if ((probOI[i] > 0) && (randNum < probOI[i])) {
			return ((SimContext::OI_TYPE) i);
		}
		randNum -= probOI[i];
	}

	return SimContext::OI_NONE;
} /* end determineAcuteOI */

/** \brief determineDeathByOI determines the risk for acute OI death
 * \param oiType a SimContext::OI_TYPE specifying which OI to use to calculate the risk of death*/
void AcuteOIUpdater::determineDeathByOI(SimContext::OI_TYPE oiType) {
	const SimContext::NatHistInputs *natHist = simContext->getNatHistInputs();
	SimContext::CD4_STRATA currCD4Strata = patient->getDiseaseState()->currTrueCD4Strata;
	SimContext::CD4_STRATA minCD4Strata = patient->getDiseaseState()->minTrueCD4Strata;

	/** Determine probability of death occurring from this OI.  Use the probability that corresponds to the patient's age (adult, late childhood, early childhood). */
	double probDeath;
	SimContext::PEDS_AGE_CAT pedsAgeCat = patient->getGeneralState()->ageCategoryPediatrics;
	SimContext::HIST_TYPE histType = (patient->getDiseaseState()->hasTrueOIHistory[oiType]) ? SimContext::HIST_Y : SimContext::HIST_N;
	if (pedsAgeCat == SimContext::PEDS_AGE_ADULT) {
		// Use adult probability of OI death
		/** Adjust by fraction of benefit */
		double fractionOfBenefit = simContext->getRunSpecsInputs()->OIsFractionOfBenefit[oiType];
		if (patient->getMonitoringState()->clinicVisitType != SimContext::CLINIC_INITIAL) {
			probDeath = fractionOfBenefit * natHist->probDeathFromOITreated[currCD4Strata][oiType][histType] +
				(1 - fractionOfBenefit) * natHist->probDeathFromOITreated[minCD4Strata][oiType][histType];
		}
		else {
			probDeath = fractionOfBenefit * natHist->probDeathFromOIUntreated[currCD4Strata][oiType][histType] +
				(1 - fractionOfBenefit) * natHist->probDeathFromOIUntreated[minCD4Strata][oiType][histType];
		}
	}
	else if (pedsAgeCat == SimContext::PEDS_AGE_LATE) {
		// Use late childhood probability of OI death
		if (patient->getMonitoringState()->clinicVisitType != SimContext::CLINIC_INITIAL)
			probDeath = simContext->getPedsInputs()->probDeathAcuteOITreatedLate[oiType][currCD4Strata][histType];
		else
			probDeath = simContext->getPedsInputs()->probDeathAcuteOIUntreatedLate[oiType][currCD4Strata][histType];
	}
	else {
		// Use early childhood probability of OI death
		SimContext::PEDS_CD4_PERC cd4PercStrata = patient->getDiseaseState()->currTrueCD4PercentageStrata;
		if (patient->getMonitoringState()->clinicVisitType != SimContext::CLINIC_INITIAL)
			probDeath = simContext->getPedsInputs()->probDeathAcuteOITreatedEarly[oiType][cd4PercStrata][histType];
		else
			probDeath = simContext->getPedsInputs()->probDeathAcuteOIUntreatedEarly[oiType][cd4PercStrata][histType];
	}

	/** Adjust probability by prophylaxis resistance */
	if (patient->getProphState()->isOnProph[oiType] && patient->getProphState()->useProphResistance[oiType]) {
		int prophNum = patient->getProphState()->currProphNum[oiType];
		SimContext::PROPH_TYPE prophType = patient->getProphState()->currProphType[oiType];
		const SimContext::ProphInputs *prophInput;
		if(patient->getGeneralState()->ageCategoryPediatrics>=SimContext::PEDS_AGE_LATE){
			prophInput=simContext->getProphInputs(prophType,oiType,prophNum);
		}
		else{
			prophInput=simContext->getPedsProphInputs(prophType,oiType,prophNum);
		}
		probDeath = CepacUtil::probRateMultiply(probDeath, 1 + prophInput->mortalityFactorResistance);
	}

	/** If risk of death is non-zero, add to the monthly mortality risk */
	if (probDeath > 0) {
		addMortalityRisk((SimContext::DTH_CAUSES) oiType, probDeath);
	}
} /* end determineDeathByOI */

