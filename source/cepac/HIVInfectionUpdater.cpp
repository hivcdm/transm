#include "include.h"

/** \brief Constructor takes in the patient object */
HIVInfectionUpdater::HIVInfectionUpdater(Patient *patient) : StateUpdater(patient) {

}

/** \brief Destructor is empty, no cleanup required */
HIVInfectionUpdater::~HIVInfectionUpdater(void) {

}

/** \brief performInitialUpdates perform all of the state and statistics updates upon patient creation */
void HIVInfectionUpdater::performInitialUpdates() {
	/** First calls the parent function to perform general updates and initialization */
	StateUpdater::performInitialUpdates();

	/** Determine the initial HIV state */
	SimContext::HIV_INF infectedState = SimContext::HIV_INF_NEG;
	SimContext::PEDS_HIV_STATE pedsHIVState = SimContext::PEDS_HIV_NEG;
	SimContext::PEDS_MOM_HIV_STATE momHIVState = SimContext::PEDS_MOM_HIV_NEG;
	bool isHighRisk = true;
	if (simContext->getPedsInputs()->enablePediatricsModel) {
		/** For pediatrics: */
		/** - Roll for and set the initial pediatrics and maternal HIV state */
		double randNum = CepacUtil::getRandomDouble(90010, patient);
		bool setType = false;
		for (int i = 0; i < SimContext::PEDS_HIV_NUM; i++) {
			for (int j = 0; j < SimContext::PEDS_MOM_HIV_NUM; j++) {
				if ((simContext->getPedsInputs()->initialHIVStateDistribution[i][j] != 0) &&
					(randNum < simContext->getPedsInputs()->initialHIVStateDistribution[i][j])) {
						pedsHIVState = (SimContext::PEDS_HIV_STATE) i;
						momHIVState = (SimContext::PEDS_MOM_HIV_STATE) j;
						setType = true;
						break;
				}
				randNum -= simContext->getPedsInputs()->initialHIVStateDistribution[i][j];
			}
			if (setType)
				break;
		}
		setInfectedPediatricsHIVState(pedsHIVState, true);
		setInfectedMaternalHIVState(momHIVState, true);
		if (simContext->getPedsInputs()->exposedUninfectedDefsEarly[SimContext::PEDS_EXPOSED_MOTHER_CHRONIC] && momHIVState < SimContext::PEDS_MOM_ACUTE){
			setExposedPediatricsState(true);
		}
		else if(simContext->getPedsInputs()->exposedUninfectedDefsEarly[SimContext::PEDS_EXPOSED_MOTHER_ACUTE] && momHIVState == SimContext::PEDS_MOM_ACUTE){
			setExposedPediatricsState(true);
		}
		else if (simContext->getPedsInputs()->exposedUninfectedDefsEarly[SimContext::PEDS_EXPOSED_MOM_NEG] && momHIVState == SimContext::PEDS_MOM_HIV_NEG){
			setExposedPediatricsState(true);
		}

		/** - Roll for and set the breastfeeding status and duration */
		randNum = CepacUtil::getRandomDouble(90015, patient);
		SimContext::PEDS_BF_TYPE bfType = SimContext::PEDS_BF_EXCL;
		for (int i = 0; i < SimContext::PEDS_BF_NUM; i++) {
			if ((simContext->getPedsInputs()->initialBFDistribution[i] != 0) &&
				(randNum < simContext->getPedsInputs()->initialBFDistribution[i])) {
					bfType = (SimContext::PEDS_BF_TYPE) i;
					break;
			}
			randNum -= simContext->getPedsInputs()->initialBFDistribution[i];
		}
		setBreastfeedingStatus(bfType);

		/** - For simplified Peds model, determine if HIV+ infant should start ART */
		if (simContext->getPedsInputs()->enableSimplifiedBehavior && (pedsHIVState != SimContext::PEDS_HIV_NEG)) {
			randNum = CepacUtil::getRandomDouble(90016, patient);
			if (randNum < simContext->getPedsInputs()->probStartART[pedsHIVState])
				setPediatricsART(true);
			else
				setPediatricsART(false);
		}

		/** - Set the regular infected state to chronic asymptomatic if positive, negative if not */
		if (pedsHIVState != SimContext::PEDS_HIV_NEG)
			setInfectedHIVState(SimContext::HIV_INF_ACUTE_SYN, true);
		else{
			setInfectedHIVState(SimContext::HIV_INF_NEG, true);
			setCareState(SimContext::HIV_CARE_NEG);
		}
	}
	else {
		if (simContext->getHIVTestInputs()->enableHIVTesting) {
			/** For adults: */
			/** - Roll for initial HIV state from distribution if using HIV testing module */
			double randNum = CepacUtil::getRandomDouble(90020, patient);
			for (int i = 0; i < SimContext::HIV_EXT_INF_NUM; i++) {
				if ((simContext->getHIVTestInputs()->initialHIVDistribution[i] != 0) &&
					(randNum < simContext->getHIVTestInputs()->initialHIVDistribution[i])) {
						if (i == SimContext::HIV_EXT_INF_NEG_LO) {
							infectedState = SimContext::HIV_INF_NEG;
							isHighRisk = false;
						}
						else {
							infectedState = (SimContext::HIV_INF) i;
						}
						break;
				}
				randNum -= simContext->getHIVTestInputs()->initialHIVDistribution[i];
			}

			/** - If patient was preset to not being a prevalent case (likely from transmission model) set them to HIV negative state */
			if (patient->getGeneralState()->predefinedAgeAndGender && !patient->getDiseaseState()->isPrevalentHIVCase){
				//Set non-prevalent cases to HIV Negative
				infectedState = (SimContext::HIV_INF_NEG);
			}
		}
		else {
			/** - Always use chronic HIV if HIV testing module is disabled */
			infectedState = SimContext::HIV_INF_ASYMP_CHR_POS;
			setCareState(SimContext::HIV_CARE_IN_CARE);
		}

		/** isInitial is true if this is a prevalent case (was true by default, but this changed with transmission model's need to predefine incidence cases) */
		bool isInitial = !(patient->getGeneralState()->predefinedAgeAndGender && !patient->getDiseaseState()->isPrevalentHIVCase);
		setInfectedHIVState(infectedState, isInitial, isHighRisk);
		if (infectedState == SimContext::HIV_INF_NEG)
			setCareState(SimContext::HIV_CARE_NEG);

	}

	/** Set the initial CD4 value for adults, CD4 percentage for pediatrics */
	if (simContext->getPedsInputs()->enablePediatricsModel) {
		/** Set the initial true CD4 percentage and strata for HIV positive infants */
		double cd4PercMean = 0;
		double cd4PercStdDev = 0;
		if (pedsHIVState == SimContext::PEDS_HIV_POS_IU) {
			cd4PercMean = simContext->getPedsInputs()->initialCD4PercentageIUMean;
			cd4PercStdDev = simContext->getPedsInputs()->initialCD4PercentageIUStdDev;
		}
		else if (pedsHIVState == SimContext::PEDS_HIV_POS_IP) {
			cd4PercMean = simContext->getPedsInputs()->initialCD4PercentageIPMean;
			cd4PercStdDev = simContext->getPedsInputs()->initialCD4PercentageIPStdDev;
		}
		double cd4PercValue = CepacUtil::getRandomGaussian(cd4PercMean, cd4PercStdDev, 90030, patient);
		setTrueCD4Percentage(cd4PercValue, true);
	}
	else {
		/** Set the initial true CD4 value and strata if HIV positive adult */
		double cd4Mean = 0;
		double cd4StdDev = 0;
		if (infectedState == SimContext::HIV_INF_ACUTE_SYN) {
			cd4Mean = simContext->getHIVTestInputs()->initialAcuteCD4DistributionMean;
			cd4StdDev = simContext->getHIVTestInputs()->initialAcuteCD4DistributionStdDev;
		}
		else if (infectedState == SimContext::HIV_INF_ASYMP_CHR_POS) {
			cd4Mean = simContext->getCohortInputs()->initialCD4Mean;
			cd4StdDev = simContext->getCohortInputs()->initialCD4StdDev;
		}
		double cd4Value = CepacUtil::getRandomGaussian(cd4Mean, cd4StdDev, 90040, patient);
		setTrueCD4(cd4Value, true);
	}

	/** Set the initial HVL strata if HIV positive, get distribution for pediatrics or adult */
	const double *hvlDist = NULL;
	if (simContext->getPedsInputs()->enablePediatricsModel) {
		if (pedsHIVState == SimContext::PEDS_HIV_POS_IU) {
			hvlDist = simContext->getPedsInputs()->initialHVLDistributionIU;
		}
		else if (pedsHIVState == SimContext::PEDS_HIV_POS_IP) {
			hvlDist = simContext->getPedsInputs()->initialHVLDistributionIP;
		}
	}
	else {
		if (infectedState == SimContext::HIV_INF_ACUTE_SYN) {
			SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
			hvlDist = simContext->getHIVTestInputs()->initialAcuteHVLDistribution[cd4Strata];
		}
		else if (infectedState == SimContext::HIV_INF_ASYMP_CHR_POS) {
			SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
			hvlDist = simContext->getCohortInputs()->initialHVLDistribution[cd4Strata];
		}
	}
	if (hvlDist != NULL) {
		SimContext::HVL_STRATA hvlStrata = SimContext::HVL__LO;
		double randNum = CepacUtil::getRandomDouble(90050, patient);
		for (int i = SimContext::HVL_NUM_STRATA - 1; i >= 0; i--) {
			if ((hvlDist[i] != 0) && (randNum < hvlDist[i])) {
				hvlStrata = (SimContext::HVL_STRATA) i;
				break;
			}
			randNum -= hvlDist[i];
		}
		setTrueHVLStrata(hvlStrata);
		setSetpointHVLStrata(hvlStrata);
		setTargetHVLStrata(hvlStrata);
	}
} /* end performInitialUpdates */

/** \brief performMonthlyUpdates perform all of the state and statistics updates for a simulated month */
void HIVInfectionUpdater::performMonthlyUpdates() {
	/** If using pediatrics, determine maternal updates and early to late childhood transition by calling HIVInfectionUpdater::performPediatricDiseaseUpdates() */
	if (simContext->getPedsInputs()->enablePediatricsModel) {
		performPediatricDiseaseUpdates();
	}

	/** determines if HIV negative patients become infected  by calling HIVInfectionUpdater::performHIVNegativeUpdates() */
	if (patient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_NEG) {
		performHIVNegativeUpdates();
	}

	/** Perform acute to chronic transition if this should occur this month by calling HIVInfectionUpdater::performAcuteToChronicHIVUpdates() */
	if ((patient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_ACUTE_SYN) &&
		(patient->getDiseaseState()->monthOfAcuteToChronicHIV == patient->getGeneralState()->monthNum)) {
			performAcuteToChronicHIVUpdates();
	}
} /* end performMonthlyUpdates */

/** \brief performHIVNegativeUpdates determines if HIV negative patients become infected */
void HIVInfectionUpdater::performHIVNegativeUpdates() {
	if (patient->getGeneralState()->ageCategoryPediatrics == SimContext::PEDS_AGE_ADULT) {
		/** If adult, roll for adult HIV infection */
		int ageCategory = patient->getGeneralState()->ageCategoryHIVInfection;
		SimContext::HIV_BEHAV riskType = patient->getMonitoringState()->isHighRiskForHIV ? SimContext::HIV_BEHAV_HI : SimContext::HIV_BEHAV_LO;
		double randNum = CepacUtil::getRandomDouble(90060, patient);
		//Roll for infection
		if (randNum < simContext->getHIVTestInputs()->probHIVInfection[ageCategory][riskType]) {
			/** - If infected, initialize the new infection by calling HIVInfectionUpdater::performHIVNewInfectionUpdates() */
			performHIVNewInfectionUpdates();
		}//if (randNum < simContext->getHIVTestInputs()->probHIVInfection[ageCategory][riskType])
	}
	else {
		/** Roll for infant PP HIV infection */
		SimContext::PEDS_MOM_HIV_STATE momHIVState = patient->getGeneralState()->maternalInfectedHIVState;
		if (momHIVState != SimContext::PEDS_MOM_HIV_NEG) {
			SimContext::PEDS_BF_TYPE bfType = patient->getGeneralState()->breastfeedingStatus;
			double randNum = CepacUtil::getRandomDouble(90065, patient);
			if (randNum < simContext->getPedsInputs()->probHIVInfectionPP[bfType][momHIVState]) {
				/** - If infected, initialize the new infection by calling HIVInfectionUpdater::performHIVNewInfectionUpdates() */
				performHIVNewInfectionUpdates();
			}//end we infected a pediatric patient
		}//end mom is HIV negative
	}//end we're dealing with a baby
} /* end performHIVNegativeUpdates */

/** \brief performHIVNewInfectionUpdates handles the new infection of an HIV negative patient
 *
 *  Note: This functions assumes that an HIV infection is scheduled to occur */
void HIVInfectionUpdater::performHIVNewInfectionUpdates(){
	/** Don't do anything if the patient is already infected */
	if (patient->getDiseaseState()->infectedHIVState != SimContext::HIV_INF_NEG){
		return;
	}
	/** Use one set of infection rules for adults and another for Peds */
	if (patient->getGeneralState()->ageCategoryPediatrics == SimContext::PEDS_AGE_ADULT){
		/** For adults:
		/* - Update the Patient's infected state to acute infected */
		setInfectedHIVState(SimContext::HIV_INF_ACUTE_SYN);

		/** - Set the CD4 level for an incident case of acute HIV infection */
		double cd4Mean = simContext->getHIVTestInputs()->initialAcuteCD4DistributionMean;
		double cd4StdDev = simContext->getHIVTestInputs()->initialAcuteCD4DistributionStdDev;
		setTrueCD4(CepacUtil::getRandomGaussian(cd4Mean, cd4StdDev, 90070, patient), true);

		/** - Set the HVL level for an incident case of acute HIV infection */
		SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
		double randNum = CepacUtil::getRandomDouble(90080, patient);
		for (int i = SimContext::HVL_NUM_STRATA - 1; i >= 0; i--) {
			if ((simContext->getHIVTestInputs()->initialAcuteHVLDistribution[cd4Strata][i] != 0) &&
				(randNum < simContext->getHIVTestInputs()->initialAcuteHVLDistribution[cd4Strata][i])) {
					setTrueHVLStrata((SimContext::HVL_STRATA) i);
					setSetpointHVLStrata((SimContext::HVL_STRATA) i);
					setTargetHVLStrata((SimContext::HVL_STRATA) i);
					break;
			}//If we choose to set to this HVL Strata...
			randNum -= simContext->getHIVTestInputs()->initialAcuteHVLDistribution[cd4Strata][i];
		}//end for loop through all HVL strata

		/** - Update the initial distributions at time of infection statistics */
		updateInitialDistributions();

		/** - Print tracing information for infection and the initial CD4/HVL */
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d HIV INFECTION;\n", patient->getGeneralState()->monthNum);
			tracer->printTrace(1, "  %d init CD4: %1.0f;\n", patient->getGeneralState()->monthNum,
				patient->getDiseaseState()->currTrueCD4);
			tracer->printTrace(1, "  %d init HVL: %s, setpt: %s;\n", patient->getGeneralState()->monthNum,
				SimContext::HVL_STRATA_STRS[patient->getDiseaseState()->currTrueHVLStrata],
				SimContext::HVL_STRATA_STRS[patient->getDiseaseState()->setpointHVLStrata]);
		}//if we're tracing the patient
	}//if patient is an adult
	/** Or process a Pediatric infection: */
	else{
		/** - Update pediatrics and regular HIV state to positive */
		setInfectedPediatricsHIVState(SimContext::PEDS_HIV_POS_PP);
		setInfectedHIVState(SimContext::HIV_INF_ACUTE_SYN);

		/** - Set the initial CD4 percentage for the newly infected infant */
		SimContext::PEDS_AGE_CAT ageCat = patient->getGeneralState()->ageCategoryPediatrics;
		double percCD4Mean = simContext->getPedsInputs()->initialCD4PercentagePPMean[ageCat];
		double percCD4StdDev = simContext->getPedsInputs()->initialCD4PercentagePPStdDev[ageCat];
		double percCD4 = CepacUtil::getRandomGaussian(percCD4Mean, percCD4StdDev, 90066, patient);
		setTrueCD4Percentage(percCD4, true);

		/** - Set the initial childhood HVL setpoint */
		SimContext::HVL_STRATA hvlStrata = SimContext::HVL_VLO;
		double randNum = CepacUtil::getRandomDouble(90067, patient);
		for (int i = SimContext::HVL_NUM_STRATA - 1; i >= 0; i--) {
			if ((simContext->getPedsInputs()->initialHVLDistributionPP[ageCat][i] != 0) &&
				(randNum < simContext->getPedsInputs()->initialHVLDistributionPP[ageCat][i])) {
					hvlStrata = (SimContext::HVL_STRATA) i;
					break;
			}
			randNum -= simContext->getPedsInputs()->initialHVLDistributionPP[ageCat][i];
		}
		setTrueHVLStrata(hvlStrata);
		setSetpointHVLStrata(hvlStrata);
		setTargetHVLStrata(hvlStrata);

		/** - For simplified Peds model, determine if HIV+ infant should start ART */
		if (simContext->getPedsInputs()->enableSimplifiedBehavior) {
			randNum = CepacUtil::getRandomDouble(90068, patient);
			if (randNum < simContext->getPedsInputs()->probStartART[SimContext::PEDS_HIV_POS_PP])
				setPediatricsART(true);
			else
				setPediatricsART(false);
		}

		/** - Print tracing information for infection and the initial CD4/HVL */
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d HIV INFECTION;\n", patient->getGeneralState()->monthNum);
			tracer->printTrace(1, "  %d init CD4 perc: %1.3f;\n", patient->getGeneralState()->monthNum,
				patient->getDiseaseState()->currTrueCD4Percentage);
			tracer->printTrace(1, "  %d init HVL: %s, setpt: %s;\n", patient->getGeneralState()->monthNum,
				SimContext::HVL_STRATA_STRS[patient->getDiseaseState()->currTrueHVLStrata],
				SimContext::HVL_STRATA_STRS[patient->getDiseaseState()->setpointHVLStrata]);
			tracer->printTrace(1, "  %d pediatric ART: %s\n", patient->getGeneralState()->monthNum,
				patient->getARTState()->isOnPediatricART ? "yes" : "no");
		}
	}//if patient is pediatric

}/* end performHIVNewInfectionUpdates*/

/** \brief performAcuteToChronicHIVUpdates handles the transition from acute to chronic HIV */
void HIVInfectionUpdater::performAcuteToChronicHIVUpdates() {
	/** Set the infection state based on the prior OI history (symptomatic or asymptomatic)*/
	if (patient->getDiseaseState()->typeTrueOIHistory == SimContext::HIST_EXT_N) {
		setInfectedHIVState(SimContext::HIV_INF_ASYMP_CHR_POS);
	}
	else {
		setInfectedHIVState(SimContext::HIV_INF_SYMP_CHR_POS);
	}

	/** Update the true CD4 levels according to the chronic transition parameters */
	SimContext::HVL_STRATA hvlStrata = patient->getDiseaseState()->currTrueHVLStrata;
	double cd4SlopeMean = simContext->getHIVTestInputs()->CD4ChangeAtChronicHIVMean[hvlStrata];
	double cd4SlopeStdDev = simContext->getHIVTestInputs()->CD4ChangeAtChronicHIVStdDev[hvlStrata];
	double cd4Slope = CepacUtil::getRandomGaussian(cd4SlopeMean, cd4SlopeStdDev, 90090, patient);
	setTrueCD4(patient->getDiseaseState()->currTrueCD4 + cd4Slope);

	/** Update setpoint HVL level based on the specified distribution of current to new levels */
	SimContext::HVL_STRATA hvlSetpoint = patient->getDiseaseState()->setpointHVLStrata;
	double randNum = CepacUtil::getRandomDouble(90100, patient);
	for (int i = 0; i < SimContext::HVL_NUM_STRATA; i++) {
		if ((simContext->getHIVTestInputs()->HVLDistributionAtChronicHIV[hvlSetpoint][i] > 0) &&
			(randNum < simContext->getHIVTestInputs()->HVLDistributionAtChronicHIV[hvlSetpoint][i])) {
				hvlSetpoint = (SimContext::HVL_STRATA) i;
				setSetpointHVLStrata(hvlSetpoint);
				break;
		}
		randNum -= simContext->getHIVTestInputs()->HVLDistributionAtChronicHIV[hvlSetpoint][i];
	}
	/** If patient is not on ART, set true and target HVL to the setpoint level */
	if (!patient->getARTState()->isOnART || (patient->getARTState()->currRegimenEfficacy == SimContext::ART_EFF_FAILURE)) {
		setTrueHVLStrata(hvlSetpoint);
		setTargetHVLStrata(hvlSetpoint);
	}

	/** Print out trace information about transition */
	if (patient->getGeneralState()->tracingEnabled) {
		tracer->printTrace(1, "**%d HIV ACUTE TO CHR: CD4 %1.0f, HVLsetpt %s;\n",
			patient->getGeneralState()->monthNum, patient->getDiseaseState()->currTrueCD4,
			SimContext::HVL_STRATA_STRS[patient->getDiseaseState()->setpointHVLStrata]);
	}
} /* end performAcuteToChronicHIVUpdates */

/** \brief performPediatricDiseaseUpdates determines maternal updates and early to late childhood transition */
void HIVInfectionUpdater::performPediatricDiseaseUpdates() {
	if (patient->getGeneralState()->isMotherAlive) {
		/** Update breastfeeding status if duration is reached or exclusive to mixed transition */
		SimContext::PEDS_BF_TYPE bfType = patient->getGeneralState()->breastfeedingStatus;
		if (simContext->getPedsInputs()->initialBFDuration == patient->getGeneralState()->monthNum) {
			setBreastfeedingStatus(SimContext::PEDS_BF_REPL);
		}
		else if ((bfType == SimContext::PEDS_BF_EXCL) && (patient->getGeneralState()->monthNum == SimContext::PEDS_BF_MTHS_EXCL)) {
			setBreastfeedingStatus(SimContext::PEDS_BF_MIXED);
		}

		/** Handle transitions in maternal HIV infection state */
		SimContext::PEDS_MOM_HIV_STATE momHIVState = patient->getGeneralState()->maternalInfectedHIVState;
		if (momHIVState == SimContext::PEDS_MOM_HIV_NEG) {
			/** Roll for incident maternal infection if HIV-negative */
			double randNum = CepacUtil::getRandomDouble(90105, patient);
			if (randNum < simContext->getPedsInputs()->probMaternalHIVInfection) {
				setInfectedMaternalHIVState(SimContext::PEDS_MOM_ACUTE);
				if (patient->getGeneralState()->tracingEnabled) {
					tracer->printTrace(1, "**%d MATERNAL HIV INFECTION\n", patient->getGeneralState()->monthNum);
				}
			}
		}
		else if ((momHIVState == SimContext::PEDS_MOM_ACUTE) &&
			(patient->getGeneralState()->monthNum - patient->getGeneralState()->monthOfMaternalHIVInfection == SimContext::PEDS_MOM_MTHS_ACUTE)) {
				/** If mom's acute time period is reached, transition to chronic non-AIDS not-in-care */
				setInfectedMaternalHIVState(SimContext::PEDS_MOM_NOAIDS_NOCARE);
		}

		/** Roll for maternal mortality */
		momHIVState = patient->getGeneralState()->maternalInfectedHIVState;
		double randNum = CepacUtil::getRandomDouble(90106, patient);
		if (randNum < simContext->getPedsInputs()->probMaternalDeath[momHIVState]) {
			setMaternalDeath();
			if (patient->getGeneralState()->tracingEnabled) {
				tracer->printTrace(1, "**%d MATERNAL DEATH\n", patient->getGeneralState()->monthNum);
			}
		}
	}

	/** Handle conversions if this is the month of transition from early to late childhood */
	if ((patient->getDiseaseState()->infectedPediatricsHIVState < SimContext::PEDS_HIV_POS_NUM) &&
		(patient->getGeneralState()->ageMonths == (SimContext::PEDS_YEAR_EARLY * 12))) {

			/** - Convert the CD4 percentage to an absolute CD4 value */
			SimContext::PEDS_CD4_PERC cd4PercStrata = patient->getDiseaseState()->currTrueCD4PercentageStrata;
			double cd4Mean = simContext->getPedsInputs()->absoluteCD4TransitionMean[cd4PercStrata];
			double cd4StdDev = simContext->getPedsInputs()->absoluteCD4TransitionStdDev[cd4PercStrata];
			double cd4Value = CepacUtil::getRandomGaussian(cd4Mean, cd4StdDev, 90110, patient);
			setTrueCD4(cd4Value, true);


			if (patient->getARTState()->isOnART) {
				int artLineNum = patient->getARTState()->currRegimenNum;
				setCD4EnvelopeRegimen(SimContext::ENVL_CD4_OVERALL,artLineNum);
				setCD4EnvelopeRegimen(SimContext::ENVL_CD4_INDIV,artLineNum);
				if(patient->getARTState()->currRegimenEfficacy != SimContext::ART_EFF_FAILURE){
					/** - Roll for new CD4 slope if on suppressive ART */
					int monthsOnART = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart;
					SimContext::ART_EFF_TYPE efficacy = patient->getARTState()->currRegimenEfficacy;
					SimContext::CD4_RESPONSE_TYPE cd4Resp = patient->getARTState()->CD4ResponseType;
					const SimContext::PedsARTInputs *pedsART = simContext->getPedsARTInputs(artLineNum);
					int stageNum = 0;
					if (monthsOnART > pedsART->stageBoundsCD4ChangeOnARTLate[efficacy][0] && monthsOnART<=pedsART->stageBoundsCD4ChangeOnARTLate[efficacy][1])
						stageNum = 1;
					else if (monthsOnART > pedsART->stageBoundsCD4ChangeOnARTLate[efficacy][1])
						stageNum = 2;
					double slopeMean = pedsART->CD4ChangeOnARTMeanLate[efficacy][cd4Resp][stageNum];
					double slopeStdDev = pedsART->CD4ChangeOnARTStdDevLate[efficacy][cd4Resp][stageNum];
					double slopeValue = CepacUtil::getRandomGaussian(slopeMean, slopeStdDev, 90111, patient);

					setCurrRegimenCD4Slope(slopeValue);

					setCD4EnvelopeSlope(SimContext::ENVL_CD4_OVERALL,slopeValue);
					setCD4EnvelopeSlope(SimContext::ENVL_CD4_INDIV,slopeValue);
				}
			}

			/** - Convert the childhood HVL setpoint to the adult setpoint HVL value */
			SimContext::HVL_STRATA currSetpoint = patient->getDiseaseState()->setpointHVLStrata;
			SimContext::HVL_STRATA newSetpoint = currSetpoint;
			double randNum = CepacUtil::getRandomDouble(90120, patient);
			for (int i = 0; i < SimContext::HVL_NUM_STRATA; i++) {
				if ((simContext->getPedsInputs()->setpointHVLTransition[currSetpoint][i] > 0) &&
					(randNum < simContext->getPedsInputs()->setpointHVLTransition[currSetpoint][i])) {
						newSetpoint = (SimContext::HVL_STRATA) i;
						break;
				}
				randNum -= simContext->getPedsInputs()->setpointHVLTransition[currSetpoint][i];
			}
			setSetpointHVLStrata(newSetpoint);
			/** - If not on ART or on failed ART, also set the true HVL to the new setpoint */
			if (!patient->getARTState()->isOnART || (patient->getARTState()->currRegimenEfficacy == SimContext::ART_EFF_FAILURE)) {
				setTrueHVLStrata(newSetpoint);
				setTargetHVLStrata(newSetpoint);
			}

			/** - Reset observed CD4 and HVL values, the early childhood ones are no longer relevant */
			setObservedCD4(false);
			setObservedHVLStrata(false);
	}

	/**Handle conversions if this is the month of transition from late childhood to adult*/
	if ((patient->getDiseaseState()->infectedHIVState != SimContext::HIV_INF_NEG) &&
			(patient->getGeneralState()->ageMonths == (SimContext::PEDS_YEAR_LATE * 12))) {
		/** - Roll for new CD4 slope if on suppressive ART */
		if (patient->getARTState()->isOnART && (patient->getARTState()->currRegimenEfficacy != SimContext::ART_EFF_FAILURE)) {
			int monthsOnART = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart;
			SimContext::ART_EFF_TYPE efficacy = patient->getARTState()->currRegimenEfficacy;
			SimContext::CD4_RESPONSE_TYPE cd4Resp = patient->getARTState()->CD4ResponseType;
			int artLineNum = patient->getARTState()->currRegimenNum;
			const SimContext::ARTInputs *ART = simContext->getARTInputs(artLineNum);
			int stageNum = 0;
			if (monthsOnART > ART->stageBoundsCD4ChangeOnART[efficacy][0] && monthsOnART<=ART->stageBoundsCD4ChangeOnART[efficacy][1])
				stageNum = 1;
			else if (monthsOnART > ART->stageBoundsCD4ChangeOnART[efficacy][1])
				stageNum = 2;
			double slopeMean = ART->CD4ChangeOnARTMean[efficacy][cd4Resp][stageNum];
			double slopeStdDev = ART->CD4ChangeOnARTStdDev[efficacy][cd4Resp][stageNum];
			double slopeValue = CepacUtil::getRandomGaussian(slopeMean, slopeStdDev, 90112, patient);

			setCurrRegimenCD4Slope(slopeValue);
		}
	}

} /* end performPediatricDiseaseUpdates */

