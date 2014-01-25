#include "include.h"

/** \brief Constructor takes in the patient object and determines if updatesCanOccur */
MortalityUpdater::MortalityUpdater(Patient *patient) : StateUpdater(patient) {

}

/** \brief Destructor is empty, no cleanup required */
MortalityUpdater::~MortalityUpdater(void) {

}

/** \brief performInitialUpdates perform all of the state and statistics updates upon patient creation */
void MortalityUpdater::performInitialUpdates() {
	/** Calls the parent function to perform general updates and initialization */
	StateUpdater::performInitialUpdates();
} /* end performInitialUpdates */

/** \brief performMonthlyUpdates perform all of the state and statistics updates for a simulated month */
void MortalityUpdater::performMonthlyUpdates() {
	const SimContext::NatHistInputs *natHist = simContext->getNatHistInputs();

	/** If using pediatrics and simplified mortality, calculate death with single life tables */
	if (simContext->getPedsInputs()->enablePediatricsModel && simContext->getPedsInputs()->enableSimplifiedBehavior) {
		/** - Load the proper probability of death */
		SimContext::PEDS_AGE_CAT ageCat = patient->getGeneralState()->ageCategoryPediatrics;
		SimContext::GENDER_TYPE gender = patient->getGeneralState()->gender;
		SimContext::PEDS_HIV_STATE hivState = patient->getDiseaseState()->infectedPediatricsHIVState;
		double probDeath = 0.0;
		if ((ageCat == SimContext::PEDS_AGE_LATE) || (ageCat == SimContext::PEDS_AGE_ADULT)) {
			probDeath = 1.0;
		}
		else if (hivState == SimContext::PEDS_HIV_NEG) {
			if (patient->getGeneralState()->maternalInfectedHIVState == SimContext::PEDS_MOM_HIV_NEG)
				probDeath = simContext->getPedsInputs()->probDeathHIVNegativeNonexposed[gender][ageCat];
			else
				probDeath = simContext->getPedsInputs()->probDeathHIVNegativeExposed[gender][ageCat];
		}
		else {
			if (patient->getARTState()->isOnPediatricART)
				probDeath = simContext->getPedsInputs()->probDeathHIVPositive[hivState][SimContext::ART_ON_STATE][gender][ageCat];
			else
				probDeath = simContext->getPedsInputs()->probDeathHIVPositive[hivState][SimContext::ART_OFF_STATE][gender][ageCat];
		}

		/** - Modify by maternal mortality rate multiplier */
		if (!patient->getGeneralState()->isMotherAlive) {
			double rateMult = simContext->getPedsInputs()->probDeathMaternalRateMultiplier;
			probDeath = CepacUtil::probRateMultiply(probDeath, rateMult);
		}
		/** - Modify by replacement fed mortality rate multiplier */
		if (patient->getGeneralState()->breastfeedingStatus == SimContext::PEDS_BF_REPL && patient->getGeneralState()->monthNum<patient->getGeneralState()->monthOfReplacementFeedingStart+simContext->getPedsInputs()->ReplacementFedMultiplierDuration) {
			double rateMult = simContext->getPedsInputs()->probDeathReplacementFedMultiplier;
			probDeath = CepacUtil::probRateMultiply(probDeath, rateMult);
		}

		/** - Roll for death and update state if death occurs */
		double randNum = CepacUtil::getRandomDouble(120005, patient);
		if (randNum < probDeath) {
			setCauseOfDeath(SimContext::DTH_NONAIDS);
		}

		/** - Return before regular CEPAC mortality calculation */
		return;
	}

	/** For adults, add the mortality risk from non-AIDS death */
	double probNonAIDSDeath = 0.0;
	SimContext::GENDER_TYPE gender = patient->getGeneralState()->gender;
	SimContext::PEDS_AGE_CAT pedsAgeCat = patient->getGeneralState()->ageCategoryPediatrics;
	if (pedsAgeCat >= SimContext::PEDS_AGE_LATE) {
		// Use the adult lifetables for probability of nonAIDS death
		int ageYears = patient->getGeneralState()->ageMonths / 12;
		if (ageYears > SimContext::AGE_MAXIMUM) {
			probNonAIDSDeath = 1.0;
		}
		else {
			probNonAIDSDeath = natHist->monthlyNonAIDSDeathProb[gender][ageYears];
		}
	}
	else {
		// Use the pediatrics lifetables for early childhood
		if (simContext->getPedsInputs()->useExposedUninfectedDefs && patient->getDiseaseState()->isExposed)
			probNonAIDSDeath = simContext->getPedsInputs()->probNonAIDSDeathExposedUninfectedEarly[gender][pedsAgeCat];
		else
			probNonAIDSDeath = simContext->getPedsInputs()->probNonAIDSDeathEarly[gender][pedsAgeCat];
	}
	double rateMult = patient->getGeneralState()->nonAIDSDeathRateMultiplier;
	probNonAIDSDeath = CepacUtil::probRateMultiply(probNonAIDSDeath, rateMult);
	if (probNonAIDSDeath > 0) {
		addMortalityRisk(SimContext::DTH_NONAIDS, probNonAIDSDeath);
	}

	/** If HIV-positive, calculate probability of chronic AIDS death */
	if (patient->getDiseaseState()->infectedHIVState != SimContext::HIV_INF_NEG) {
		/** - Determine prob of chronic AIDS death */
		double probAIDSDeath = 0.0;
		SimContext::HIST_EXT oiHistory = patient->getDiseaseState()->typeTrueOIHistory;
		SimContext::PEDS_AGE_CAT pedsAgeCat = patient->getGeneralState()->ageCategoryPediatrics;
		if (pedsAgeCat == SimContext::PEDS_AGE_ADULT) {
			/** - Use the adult probability of chronic AIDS death */
			SimContext::CD4_STRATA currCD4 = patient->getDiseaseState()->currTrueCD4Strata;
			SimContext::CD4_STRATA minCD4 = patient->getDiseaseState()->minTrueCD4Strata;
			/** - Account for chrAIDS death fraction on benefit */
			double fractionOfBenefit = simContext->getRunSpecsInputs()->deathFractionOfBenefit[SimContext::DTH_CHRAIDS];
			probAIDSDeath = fractionOfBenefit * natHist->chronicAIDSDeathProbOffART[oiHistory][currCD4] +
				(1 - fractionOfBenefit) * natHist->chronicAIDSDeathProbOffART[oiHistory][minCD4];
			/** - Adjust for ART effect */
			if (patient->getARTState()->isOnART) {
				double rateMult = simContext->getNatHistInputs()->chronicAIDSDeathProbOnARTMult[oiHistory][currCD4];
				/** - Adjust multiplier between full ART effect and no ART effect according to the factor from the ART response type */
				rateMult = 1 - (patient->getARTState()->responseFactorCurrRegimen[SimContext::HET_OUTCOME_ARTEFFECT_MORT] * (1 - rateMult));
				probAIDSDeath = CepacUtil::probRateMultiply(probAIDSDeath, rateMult);
			}
		}
		else if (pedsAgeCat == SimContext::PEDS_AGE_LATE) {
			// Use the late childhood probability of chronic AIDS death
			SimContext::CD4_STRATA currCD4 = patient->getDiseaseState()->currTrueCD4Strata;
			probAIDSDeath = simContext->getPedsInputs()->probChronicAIDSDeathLate[oiHistory][currCD4];
			// If on ART, adjust for heterogeneity based pediatric ART effect
			if (patient->getARTState()->isOnART) {
				double rateMult = simContext->getPedsInputs()->chronicAIDSDeathProbOnARTMultLate[oiHistory][currCD4];
				// Adjust multiplier between full ART effect and no ART effect according
				//	to the factor from the ART response type
				rateMult = 1 - (patient->getARTState()->responseFactorCurrRegimen[SimContext::HET_OUTCOME_ARTEFFECT_MORT] * (1 - rateMult));
				probAIDSDeath = CepacUtil::probRateMultiply(probAIDSDeath, rateMult);
			}
		}
		else {
			// Use the early childhood probability of chronic AIDS death
			SimContext::PEDS_CD4_PERC cd4PercStrata = patient->getDiseaseState()->currTrueCD4PercentageStrata;
			probAIDSDeath = simContext->getPedsInputs()->probChronicAIDSDeathEarly[oiHistory][pedsAgeCat][cd4PercStrata];
			// If on ART, adjust for heterogeneity based pediatric ART effect
			if (patient->getARTState()->isOnART) {
				double rateMult = 1.0;
				int monthsOnART = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart;
				if (monthsOnART <= simContext->getPedsInputs()->stageBoundsChronicAIDSDeathProbOnARTMultEarly[0])
					rateMult = simContext->getPedsInputs()->chronicAIDSDeathProbOnARTMultEarly[cd4PercStrata][0];
				else if (monthsOnART <= simContext->getPedsInputs()->stageBoundsChronicAIDSDeathProbOnARTMultEarly[1])
					rateMult = simContext->getPedsInputs()->chronicAIDSDeathProbOnARTMultEarly[cd4PercStrata][1];
				else
					rateMult = simContext->getPedsInputs()->chronicAIDSDeathProbOnARTMultEarly[cd4PercStrata][2];
				// Adjust multiplier between full ART effect and no ART effect according
				//	to the factor from the ART response type
				rateMult = 1 - (patient->getARTState()->responseFactorCurrRegimen[SimContext::HET_OUTCOME_ARTEFFECT_MORT] * (1 - rateMult));
				probAIDSDeath = CepacUtil::probRateMultiply(probAIDSDeath, rateMult);
			}
		}

		/** Add mortality risks of chronic AIDS death */
		if (probAIDSDeath > 0) {
			addMortalityRisk(SimContext::DTH_CHRAIDS, probAIDSDeath);
		}
	}

	/** Calculate the combined probability of the patient not dying this month */
	bool deathOccurs = false;
	int causeOfDeathId = 0;
	double probNoDeath = 1.0;
	const vector<SimContext::MortalityRisk> &mortalityRisks = patient->getDiseaseState()->mortalityRisks;
	int numRisks = mortalityRisks.size();
	for (int i = 0; i < numRisks; i++) {
		/** Set as the cause of death if any prob is >= 1 */
		if (mortalityRisks[i].probDeath >= 1) {
			deathOccurs = true;
			causeOfDeathId = i;
			break;
		}
		/** Accumulate the probability of death not occurring: \f$ p(noDeath) = \prod_{i \in MortalityRisks} (1 - p(Death_i)) \f$ */
		probNoDeath *= (1 - mortalityRisks[i].probDeath);
	}

	if (!deathOccurs) {
		/** Roll for death not occurring this month, return if patient survives */
		double randNum = CepacUtil::getRandomDouble(120010, patient);
		if (randNum < probNoDeath)
			return;

		// Calculate the individual probability of each cause of death occurring and none of the others
		//Change to rate per Milt's direction
		/*double *indivProbDeath = new double[numRisks];
		double sumOfProbs = 0;
		for (int i = 0; i < numRisks; i++) {
			indivProbDeath[i] = (probNoDeath / (1 - mortalityRisks[i].probDeath)) * mortalityRisks[i].probDeath;
			sumOfProbs += indivProbDeath[i];
		}*/
		/** 2/2/2010: Calculate the rate each cause of death occurs (per Milt) -- errhode */
		double *indivRateDeath = new double[numRisks];
		double sumOfRates = 0;
		for (int i = 0; i < numRisks; i++) {
			indivRateDeath[i] = CepacUtil::probToRate(mortalityRisks[i].probDeath);
			sumOfRates += indivRateDeath[i];
		}

		/** Roll for the cause of death from a normalized distribution of the individual rates */
		randNum = CepacUtil::getRandomDouble(120020, patient);
		for (int i = 0; i < numRisks; i++) {
			indivRateDeath[i] = indivRateDeath[i] / sumOfRates;
			if ((indivRateDeath[i] > 0) && (randNum < indivRateDeath[i])) {
				deathOccurs = true;
				causeOfDeathId = i;
				break;
			}
			randNum -= indivRateDeath[i];
		}
		delete [] indivRateDeath;
	}

	/** Set the cause of death */
	if (deathOccurs) {
		setCauseOfDeath(mortalityRisks[causeOfDeathId].causeOfDeath);
		/** Special case for ART toxicity, add cost here */
		if (mortalityRisks[causeOfDeathId].causeOfDeath == SimContext::DTH_TOX_ART) {
			incrementCostsMisc(mortalityRisks[causeOfDeathId].costDeath, 1.0);
		}
	}
} /* end performMonthlyUpdates */
