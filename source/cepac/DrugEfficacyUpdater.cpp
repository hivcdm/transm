#include "include.h"

/** \brief Constructor takes in the patient object */
DrugEfficacyUpdater::DrugEfficacyUpdater(Patient *patient) : StateUpdater(patient) {

}

/** \brief Destructor is empty, no cleanup required */
DrugEfficacyUpdater::~DrugEfficacyUpdater(void) {

}

/** \brief performInitialUpdates perform all of the state and statistics updates upon patient creation */
void DrugEfficacyUpdater::performInitialUpdates() {
	/** Calls the parent function to perform general updates and initialization */
	StateUpdater::performInitialUpdates();
} /* end performInitialUpdates */

/** \brief performMonthlyUpdates perform all of the state and statistics updates for a simulated month
 *
 * - If on ART, calls DrugEfficacyUpdater::performARTEfficacyUpdates
 * - If either CD4 envelope or CD4 percentage envelope is active, calls DrugEfficacyUpdater::performARTEnvelopeEfficacyUpdates
 * - If on any (non-TB) OI prophs, calls DrugEfficacyUpdater::performProphEfficacyUpdates
 * - If on TB prophylaxis, calls DrugEfficacyUpdater::performTBProphEfficacyUpdates
 * - If on TB treatment, calls DrugEfficacyUpdater::performTBTreatmentEfficacyUpdates
 **/
void DrugEfficacyUpdater::performMonthlyUpdates() {
	if (patient->getARTState()->isOnART)
		performARTEfficacyUpdates();
	if (patient->getARTState()->overallCD4Envelope.isActive ||
		patient->getARTState()->overallCD4PercentageEnvelope.isActive)
			performARTEnvelopeEfficacyUpdates();
	if (patient->getProphState()->currTotalNumProphsOn > 0)
		performProphEfficacyUpdates();
	if (patient->getTBState()->isOnProph)
		performTBProphEfficacyUpdates();
	if (patient->getTBState()->isOnTreatment)
		performTBTreatmentEfficacyUpdates();
} /* end performMonthlyUpdates */

/** \brief performARTEfficacyUpdates handles all efficacy updates for ART regimens */
void DrugEfficacyUpdater::performARTEfficacyUpdates() {

	int artLineNum = patient->getARTState()->currRegimenNum;
	int subRegNum = patient->getARTState()->currSubRegimenNum;
	SimContext::ART_EFF_TYPE efficacy = patient->getARTState()->currRegimenEfficacy;
	int monthsEfficacy = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfEfficacyChange;
	SimContext::RESP_TYPE responseType = patient->getARTState()->responseTypeCurrRegimen[SimContext::HET_OUTCOME_RESIST];
	SimContext::CD4_RESPONSE_TYPE cd4Response = patient->getARTState()->CD4ResponseType;

	const SimContext::ARTInputs *artInputs = simContext->getARTInputs(artLineNum);
	const SimContext::PedsARTInputs *pedsART = simContext->getPedsARTInputs(artLineNum);
	SimContext::PEDS_AGE_CAT pedsAgeCat = patient->getGeneralState()->ageCategoryPediatrics;

	/** Increment the number of months on partial suppress or failed ART for resistance,
	/*	only count if patient is a full/partial responder */
	if ((efficacy != SimContext::ART_EFF_SUCCESS) && (responseType != SimContext::RESP_TYPE_NON)) {
		incrementMonthsUnsuccessfulART();
	}

	/** Check if patient is suppressed and has reached the next CD4 slope time segment, if so redraw slope */
	if (efficacy != SimContext::ART_EFF_FAILURE) {
		if (pedsAgeCat == SimContext::PEDS_AGE_ADULT) {
			for (int i = 0; i < 2; i++) {
				if (monthsEfficacy == artInputs->stageBoundsCD4ChangeOnART[efficacy][i]) {
					double cd4SlopeMean = artInputs->CD4ChangeOnARTMean[efficacy][cd4Response][i + 1];
					double cd4SlopeStdDev = artInputs->CD4ChangeOnARTStdDev[efficacy][cd4Response][i + 1];
					double cd4Slope = CepacUtil::getRandomGaussian(cd4SlopeMean, cd4SlopeStdDev, 70010, patient);
					setCurrRegimenCD4Slope(cd4Slope);
				}
			}
		}
		else if (pedsAgeCat == SimContext::PEDS_AGE_LATE) {
			for (int i = 0; i < 2; i++) {
				if (monthsEfficacy == pedsART->stageBoundsCD4ChangeOnARTLate[efficacy][i]) {
					double cd4SlopeMean = pedsART->CD4ChangeOnARTMeanLate[efficacy][cd4Response][i + 1];
					double cd4SlopeStdDev = pedsART->CD4ChangeOnARTStdDevLate[efficacy][cd4Response][i + 1];
					double cd4Slope = CepacUtil::getRandomGaussian(cd4SlopeMean, cd4SlopeStdDev, 70011, patient);

					setCurrRegimenCD4Slope(cd4Slope);

				}
			}
		}
		else {
			for (int i = 0; i < 2; i++) {
				if (monthsEfficacy == pedsART->stageBoundsCD4PercentageChangeOnARTEarly[efficacy][i]) {
					int ageStartedEfficacyChange=patient->getGeneralState()->ageMonths-monthsEfficacy;
					SimContext::PEDS_AGE_CAT pedsAgeCatStartedEfficacy=getAgeCategoryPediatrics(ageStartedEfficacyChange);
					double cd4PercSlopeMean = pedsART->CD4PercentageChangeOnARTMeanEarly[efficacy][pedsAgeCatStartedEfficacy][cd4Response][i + 1];
					double cd4PercSlopeStdDev = pedsART->CD4PercentageChangeOnARTStdDevEarly[efficacy][pedsAgeCatStartedEfficacy][cd4Response][i + 1];
					double cd4PercSlope = CepacUtil::getRandomGaussian(cd4PercSlopeMean, cd4PercSlopeStdDev, 70012, patient);

					setCurrRegimenCD4PercentageSlope(cd4PercSlope);
				}
			}
		}
	}

	/** Trigger an emergency clinic visit if max months on ART or months to
	/*	subregimen switch has been reached */
	int monthsOnART = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart;
	if (patient->getGeneralState()->ageCategoryPediatrics>=SimContext::PEDS_AGE_LATE){
		const SimContext::TreatmentInputs::ARTStopPolicy &stopART = simContext->getTreatmentInputs()->stopART[artLineNum];
		if ((stopART.maxMonthsOnART != SimContext::NOT_APPL) &&
			(monthsOnART >= stopART.maxMonthsOnART)) {
				scheduleEmergencyClinicVisit(true, patient->getGeneralState()->monthNum);
		}
	}
	else{
		const SimContext::PedsInputs::ARTStopPolicy &stopART = simContext->getPedsInputs()->stopART[artLineNum];
		if ((stopART.maxMonthsOnART != SimContext::NOT_APPL) &&
			(monthsOnART >= stopART.maxMonthsOnART)) {
				scheduleEmergencyClinicVisit(true, patient->getGeneralState()->monthNum);
		}
	}

	int monthsOnSubReg = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrSubRegimenStart;
	if ((artInputs->monthsToSwitchSubRegimen[subRegNum] != SimContext::NOT_APPL) &&
		(monthsOnSubReg >= artInputs->monthsToSwitchSubRegimen[subRegNum])) {
			scheduleEmergencyClinicVisit(true, patient->getGeneralState()->monthNum);
	}

	/** Check if force failure month has been reached if it is set */
	if (efficacy != SimContext::ART_EFF_FAILURE) {
		bool forceFail = false;
		if (pedsAgeCat == SimContext::PEDS_AGE_ADULT){
			if ((artInputs->forceFailAtMonth != SimContext::NOT_APPL) &&
			(monthsOnART >= artInputs->forceFailAtMonth))
				forceFail = true;
		}
		else if (pedsAgeCat == SimContext::PEDS_AGE_LATE) {
			if ((pedsART->forceFailAtMonthLate != SimContext::NOT_APPL) &&
			(monthsOnART >= pedsART->forceFailAtMonthLate))
				forceFail = true;
		}
		else if ((pedsART->forceFailAtMonthEarly != SimContext::NOT_APPL) &&
			(monthsOnART >= pedsART->forceFailAtMonthEarly))
				forceFail = true;
		if (forceFail) {
			/** If forcing fail:
			 *  - Set the efficacy to failure
			 */
			setCurrARTEfficacy(SimContext::ART_EFF_FAILURE, false);
			/** - Set the target HVL to the setpoint HVL */
			setTargetHVLStrata(patient->getDiseaseState()->setpointHVLStrata);
			/** - Output tracing if enabled */
			if (patient->getGeneralState()->tracingEnabled) {
				tracer->printTrace(1, "**%d ART LATE FAIL;\n", patient->getGeneralState()->monthNum);
			}
			return;
		}
	}

	/** If this is the patient's initial suppression on this regimen (use indiv envelope to determine)
	//	and it is within the efficacy time horizon, do not roll for any late transitions */
	if (pedsAgeCat == SimContext::PEDS_AGE_ADULT) {
		int efficacyHorizon=artInputs->efficacyTimeHorizon;
		if (patient->getARTState()->indivCD4Envelope.isActive &&
			(patient->getARTState()->monthOfCurrRegimenStart == patient->getARTState()->indivCD4Envelope.monthOfStart) &&
			(patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart <= efficacyHorizon)) {
				return;
		}
	}
	else if (pedsAgeCat == SimContext::PEDS_AGE_LATE) {
		int efficacyHorizon=pedsART->efficacyTimeHorizonLate;
		if (patient->getARTState()->indivCD4Envelope.isActive &&
			(patient->getARTState()->monthOfCurrRegimenStart == patient->getARTState()->indivCD4Envelope.monthOfStart) &&
			(patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart <= efficacyHorizon)) {
				return;
		}
	}
	else {
		int efficacyHorizon=pedsART->efficacyTimeHorizonEarly;
		if (patient->getARTState()->indivCD4PercentageEnvelope.isActive &&
			(patient->getARTState()->monthOfCurrRegimenStart == patient->getARTState()->indivCD4PercentageEnvelope.monthOfStart) &&
			(patient->getGeneralState()->monthNum - patient->getARTState()->monthOfCurrRegimenStart <= efficacyHorizon)) {
				return;
		}
	}



	if (efficacy == SimContext::ART_EFF_SUCCESS) {
		/** If patient is suppressed, roll for late fail or late partial transition */
		double probFail = 0.0;
		double probPartial= 0.0;
		probFail = patient->getARTState()->probLateFail;


		double randNum = CepacUtil::getRandomDouble(70020, patient);
		if ((probFail > 0) && (randNum < probFail)) {
			/** If fail:
			* - Set the efficacy to failure */
			setCurrARTEfficacy(SimContext::ART_EFF_FAILURE, false);
			/** - Set the target HVL to the setpoint HVL */
			setTargetHVLStrata(patient->getDiseaseState()->setpointHVLStrata);
			/** - Output tracing if enabled */
			if (patient->getGeneralState()->tracingEnabled) {
				tracer->printTrace(1, "**%d ART LATE FAIL;\n", patient->getGeneralState()->monthNum);
			}
		}
		else if ((probPartial > 0) && (randNum - probFail < probPartial)) {
			/** If transitioning to partial suppression: */
			setCurrARTEfficacy(SimContext::ART_EFF_PARTIAL, false);
			/** - Determine and set the new target HVL level */
			setTargetHVLStrata(getPartialSuppressTargetHVL(artLineNum));
			/** - Set the CD4 slope for partially suppressed ART based on patient's response type */
			if (pedsAgeCat == SimContext::PEDS_AGE_ADULT) {
				double cd4SlopeMean = artInputs->CD4ChangeOnARTMean[SimContext::ART_EFF_PARTIAL][cd4Response][0];
				double cd4SlopeStdDev = artInputs->CD4ChangeOnARTStdDev[SimContext::ART_EFF_PARTIAL][cd4Response][0];
				double cd4Slope = CepacUtil::getRandomGaussian(cd4SlopeMean, cd4SlopeStdDev, 70030, patient);

				setCurrRegimenCD4Slope(cd4Slope);
			}
			else if (pedsAgeCat == SimContext::PEDS_AGE_LATE) {
				double cd4SlopeMean = pedsART->CD4ChangeOnARTMeanLate[SimContext::ART_EFF_PARTIAL][cd4Response][0];
				double cd4SlopeStdDev = pedsART->CD4ChangeOnARTStdDevLate[SimContext::ART_EFF_PARTIAL][cd4Response][0];
				double cd4Slope = CepacUtil::getRandomGaussian(cd4SlopeMean, cd4SlopeStdDev, 70031, patient);

				setCurrRegimenCD4Slope(cd4Slope);
			}
			else {
				double cd4PercSlopeMean = pedsART->CD4PercentageChangeOnARTMeanEarly[SimContext::ART_EFF_PARTIAL][pedsAgeCat][cd4Response][0];
				double cd4PercSlopeStdDev = pedsART->CD4PercentageChangeOnARTStdDevEarly[SimContext::ART_EFF_PARTIAL][pedsAgeCat][cd4Response][0];
				double cd4PercSlope = CepacUtil::getRandomGaussian(cd4PercSlopeMean, cd4PercSlopeStdDev, 70032, patient);

				setCurrRegimenCD4Slope(cd4PercSlope);
			}
			/** - Output tracing if enabled */
			if (patient->getGeneralState()->tracingEnabled) {
				tracer->printTrace(1, "**%d ART LATE PARTIAL;\n", patient->getGeneralState()->monthNum);
			}
		}
	}

} /* end performARTEfficacyUpdates */

/** \brief performARTEnvelopeEfficacyUpdates handles the efficacy updates of the ART CD4 envelope */
void DrugEfficacyUpdater::performARTEnvelopeEfficacyUpdates() {
	SimContext::PEDS_AGE_CAT pedsAgeCat = patient->getGeneralState()->ageCategoryPediatrics;
	if (pedsAgeCat == SimContext::PEDS_AGE_ADULT) {
		/** Check if patient has reached the next CD4 envelope slope time segment, if so redraw slope */
		int overallEnvLineNum = patient->getARTState()->overallCD4Envelope.regimenNum;
		int monthOverallEnvStart = patient->getARTState()->overallCD4Envelope.monthOfStart;
		int monthsSinceOverallEnv = patient->getGeneralState()->monthNum - monthOverallEnvStart;
		bool hasIndivEnv = patient->getARTState()->indivCD4Envelope.isActive;
		int indivEnvLineNum = patient->getARTState()->indivCD4Envelope.regimenNum;
		const SimContext::ARTInputs *artInputs = simContext->getARTInputs(overallEnvLineNum);
		for (int i = 0; i < 2; i++) {
			if (monthsSinceOverallEnv == artInputs->stageBoundsCD4ChangeOnART[SimContext::ART_EFF_SUCCESS][i]) {
				/** If patient is still suppressed on the regimen that is setting the envelope,
				//	use the current CD4 slope as the CD4 envelope slope */
				if (patient->getARTState()->isOnART &&
					(patient->getARTState()->currRegimenEfficacy == SimContext::ART_EFF_SUCCESS) &&
					(patient->getARTState()->currRegimenNum == overallEnvLineNum) &&
					(patient->getARTState()->monthOfCurrRegimenStart == monthOverallEnvStart)) {
						setCD4EnvelopeSlope(SimContext::ENVL_CD4_OVERALL, patient->getARTState()->currRegimenCD4Slope);
						if (hasIndivEnv && (overallEnvLineNum == indivEnvLineNum))
							setCD4EnvelopeSlope(SimContext::ENVL_CD4_INDIV, patient->getARTState()->currRegimenCD4Slope);
				}
				else {
					/** Otherwise, draw for a new CD4 envelope slope */
					SimContext::CD4_RESPONSE_TYPE cd4Response = patient->getARTState()->CD4ResponseType;
					double cd4SlopeMean = artInputs->CD4ChangeOnARTMean[SimContext::ART_EFF_SUCCESS][cd4Response][i + 1];
					double cd4SlopeStdDev = artInputs->CD4ChangeOnARTStdDev[SimContext::ART_EFF_SUCCESS][cd4Response][i + 1];
					double cd4Slope = CepacUtil::getRandomGaussian(cd4SlopeMean, cd4SlopeStdDev, 70050, patient);

					setCD4EnvelopeSlope(SimContext::ENVL_CD4_OVERALL, cd4Slope);
					if (hasIndivEnv && (overallEnvLineNum == indivEnvLineNum))
						setCD4EnvelopeSlope(SimContext::ENVL_CD4_INDIV, cd4Slope);
				}
			}
		}

		/** Update the individual regimen CD4 envelope */
		if (hasIndivEnv && (indivEnvLineNum != overallEnvLineNum)) {
			int monthIndivEnvStart = patient->getARTState()->indivCD4Envelope.monthOfStart;
			int monthsSinceIndivEnv = patient->getGeneralState()->monthNum - monthIndivEnvStart;
			/** Check if patient has reached the next CD4 envelope slope time segment, if so redraw slope */
			const SimContext::ARTInputs *artInputs = simContext->getARTInputs(indivEnvLineNum);
			for (int i = 0; i < 2; i++) {
				if (monthsSinceIndivEnv == artInputs->stageBoundsCD4ChangeOnART[SimContext::ART_EFF_SUCCESS][i]) {
					/** If patient is still suppressed on the regimen that is setting the envelope,
					//	use the current CD4 slope as the CD4 envelope slope */
					if (patient->getARTState()->isOnART &&
						(patient->getARTState()->currRegimenEfficacy == SimContext::ART_EFF_SUCCESS) &&
						(patient->getARTState()->currRegimenNum == indivEnvLineNum) &&
						(patient->getARTState()->monthOfCurrRegimenStart == monthIndivEnvStart)) {
							setCD4EnvelopeSlope(SimContext::ENVL_CD4_INDIV, patient->getARTState()->currRegimenCD4Slope);
					}
					else {
						/** Otherwise, draw for a new CD4 envelope slope */
						SimContext::CD4_RESPONSE_TYPE cd4Response = patient->getARTState()->CD4ResponseType;
						double cd4SlopeMean = artInputs->CD4ChangeOnARTMean[SimContext::ART_EFF_SUCCESS][cd4Response][i + 1];
						double cd4SlopeStdDev = artInputs->CD4ChangeOnARTStdDev[SimContext::ART_EFF_SUCCESS][cd4Response][i + 1];
						double cd4Slope = CepacUtil::getRandomGaussian(cd4SlopeMean, cd4SlopeStdDev, 70051, patient);

						setCD4EnvelopeSlope(SimContext::ENVL_CD4_INDIV, cd4Slope);
					}
				}
			}
		}
	}
	else if (pedsAgeCat == SimContext::PEDS_AGE_LATE) {
		// Check if patient has reached the next CD4 envelope slope time segment, if so redraw slope
		int overallEnvLineNum = patient->getARTState()->overallCD4Envelope.regimenNum;
		int monthOverallEnvStart = patient->getARTState()->overallCD4Envelope.monthOfStart;
		int monthsSinceOverallEnv = patient->getGeneralState()->monthNum - monthOverallEnvStart;
		bool hasIndivEnv = patient->getARTState()->indivCD4Envelope.isActive;
		int indivEnvLineNum = patient->getARTState()->indivCD4Envelope.regimenNum;

		const SimContext::PedsARTInputs *pedsART = simContext->getPedsARTInputs(overallEnvLineNum);
		for (int i = 0; i < 2; i++) {
			if (monthsSinceOverallEnv == pedsART->stageBoundsCD4ChangeOnARTLate[SimContext::ART_EFF_SUCCESS][i]) {
				// If patient is still suppressed on the regimen that is setting the envelope,
				//	use the current CD4 slope as the CD4 envelope slope
				if (patient->getARTState()->isOnART &&
					(patient->getARTState()->currRegimenEfficacy == SimContext::ART_EFF_SUCCESS) &&
					(patient->getARTState()->currRegimenNum == overallEnvLineNum) &&
					(patient->getARTState()->monthOfCurrRegimenStart == monthOverallEnvStart)) {
						setCD4EnvelopeSlope(SimContext::ENVL_CD4_OVERALL, patient->getARTState()->currRegimenCD4Slope);
						if (hasIndivEnv && (overallEnvLineNum == indivEnvLineNum))
							setCD4EnvelopeSlope(SimContext::ENVL_CD4_INDIV, patient->getARTState()->currRegimenCD4Slope);
				}
				else {
					// Otherwise, draw for a new CD4 envelope slope
					SimContext::CD4_RESPONSE_TYPE cd4Response = patient->getARTState()->CD4ResponseType;
					double cd4SlopeMean = pedsART->CD4ChangeOnARTMeanLate[SimContext::ART_EFF_SUCCESS][cd4Response][i + 1];
					double cd4SlopeStdDev = pedsART->CD4ChangeOnARTStdDevLate[SimContext::ART_EFF_SUCCESS][cd4Response][i + 1];
					double cd4Slope = CepacUtil::getRandomGaussian(cd4SlopeMean, cd4SlopeStdDev, 70052, patient);

					setCD4EnvelopeSlope(SimContext::ENVL_CD4_OVERALL, cd4Slope);
					if (hasIndivEnv && (overallEnvLineNum == indivEnvLineNum))
						setCD4EnvelopeSlope(SimContext::ENVL_CD4_INDIV, cd4Slope);
				}
			}
		}

		// Update the individual regimen CD4 envelope
		if (hasIndivEnv && (indivEnvLineNum != overallEnvLineNum)) {
			int monthIndivEnvStart = patient->getARTState()->indivCD4Envelope.monthOfStart;
			int monthsSinceIndivEnv = patient->getGeneralState()->monthNum - monthIndivEnvStart;
			// Check if patient has reached the next CD4 envelope slope time segment, if so redraw slope

			const SimContext::PedsARTInputs *pedsART = simContext->getPedsARTInputs(indivEnvLineNum);
			for (int i = 0; i < 2; i++) {
				if (monthsSinceIndivEnv == pedsART->stageBoundsCD4ChangeOnARTLate[SimContext::ART_EFF_SUCCESS][i]) {
					// If patient is still suppressed on the regimen that is setting the envelope,
					//	use the current CD4 slope as the CD4 envelope slope
					if (patient->getARTState()->isOnART &&
						(patient->getARTState()->currRegimenEfficacy == SimContext::ART_EFF_SUCCESS) &&
						(patient->getARTState()->currRegimenNum == indivEnvLineNum) &&
						(patient->getARTState()->monthOfCurrRegimenStart == monthIndivEnvStart)) {
							setCD4EnvelopeSlope(SimContext::ENVL_CD4_INDIV, patient->getARTState()->currRegimenCD4Slope);
					}
					else {
						// Otherwise, draw for a new CD4 envelope slope
						SimContext::CD4_RESPONSE_TYPE cd4Response = patient->getARTState()->CD4ResponseType;
						double cd4SlopeMean = pedsART->CD4ChangeOnARTMeanLate[SimContext::ART_EFF_SUCCESS][cd4Response][i + 1];
						double cd4SlopeStdDev = pedsART->CD4ChangeOnARTStdDevLate[SimContext::ART_EFF_SUCCESS][cd4Response][i + 1];
						double cd4Slope = CepacUtil::getRandomGaussian(cd4SlopeMean, cd4SlopeStdDev, 70053, patient);

						setCD4EnvelopeSlope(SimContext::ENVL_CD4_INDIV, cd4Slope);
					}
				}
			}
		}
	}
	else {
		/** For Peds, use the same process as above, but with the CD4 Percentage envelope */
		// Check if patient has reached the next CD4 envelope slope time segment, if so redraw slope
		int overallEnvLineNum = patient->getARTState()->overallCD4PercentageEnvelope.regimenNum;
		int monthOverallEnvStart = patient->getARTState()->overallCD4PercentageEnvelope.monthOfStart;
		int monthsSinceOverallEnv = patient->getGeneralState()->monthNum - monthOverallEnvStart;
		bool hasIndivEnv = patient->getARTState()->indivCD4PercentageEnvelope.isActive;
		int indivEnvLineNum = patient->getARTState()->indivCD4PercentageEnvelope.regimenNum;
		const SimContext::PedsARTInputs *pedsART = simContext->getPedsARTInputs(overallEnvLineNum);
		for (int i = 0; i < 2; i++) {
			if (monthsSinceOverallEnv == pedsART->stageBoundsCD4PercentageChangeOnARTEarly[SimContext::ART_EFF_SUCCESS][i]) {
				// If patient is still suppressed on the regimen that is setting the envelope,
				//	use the current CD4 slope as the CD4 envelope slope
				if (patient->getARTState()->isOnART &&
					(patient->getARTState()->currRegimenEfficacy == SimContext::ART_EFF_SUCCESS) &&
					(patient->getARTState()->currRegimenNum == overallEnvLineNum) &&
					(patient->getARTState()->monthOfCurrRegimenStart == monthOverallEnvStart)) {
						setCD4EnvelopeSlope(SimContext::ENVL_CD4_PERC_OVERALL, patient->getARTState()->currRegimenCD4PercentageSlope);
						if (hasIndivEnv && (overallEnvLineNum == indivEnvLineNum))
							setCD4EnvelopeSlope(SimContext::ENVL_CD4_PERC_INDIV, patient->getARTState()->currRegimenCD4PercentageSlope);
				}
				else {
					// Otherwise, draw for a new CD4 envelope slope
					SimContext::CD4_RESPONSE_TYPE cd4Response = patient->getARTState()->CD4ResponseType;
					double cd4PercSlopeMean = pedsART->CD4PercentageChangeOnARTMeanEarly[SimContext::ART_EFF_SUCCESS][pedsAgeCat][cd4Response][i + 1];
					double cd4PercSlopeStdDev = pedsART->CD4PercentageChangeOnARTStdDevEarly[SimContext::ART_EFF_SUCCESS][pedsAgeCat][cd4Response][i + 1];
					double cd4PercSlope = CepacUtil::getRandomGaussian(cd4PercSlopeMean, cd4PercSlopeStdDev, 70054, patient);

					setCD4EnvelopeSlope(SimContext::ENVL_CD4_PERC_OVERALL, cd4PercSlope);
					if (hasIndivEnv && (overallEnvLineNum == indivEnvLineNum))
						setCD4EnvelopeSlope(SimContext::ENVL_CD4_PERC_INDIV, cd4PercSlope);
				}
			}
		}

		// Update the individual regimen CD4 envelope
		if (hasIndivEnv && (indivEnvLineNum != overallEnvLineNum)) {
			int monthIndivEnvStart = patient->getARTState()->indivCD4PercentageEnvelope.monthOfStart;
			int monthsSinceIndivEnv = patient->getGeneralState()->monthNum - monthIndivEnvStart;
			// Check if patient has reached the next CD4 envelope slope time segment, if so redraw slope
			const SimContext::PedsARTInputs *pedsART = simContext->getPedsARTInputs(indivEnvLineNum);
			for (int i = 0; i < 2; i++) {
				if (monthsSinceIndivEnv == pedsART->stageBoundsCD4PercentageChangeOnARTEarly[SimContext::ART_EFF_SUCCESS][i]) {
					// If patient is still suppressed on the regimen that is setting the envelope,
					//	use the current CD4 slope as the CD4 envelope slope
					if (patient->getARTState()->isOnART &&
						(patient->getARTState()->currRegimenEfficacy == SimContext::ART_EFF_SUCCESS) &&
						(patient->getARTState()->currRegimenNum == indivEnvLineNum) &&
						(patient->getARTState()->monthOfCurrRegimenStart == monthIndivEnvStart)) {
							setCD4EnvelopeSlope(SimContext::ENVL_CD4_PERC_INDIV, patient->getARTState()->currRegimenCD4PercentageSlope);
					}
					else {
						// Otherwise, draw for a new CD4 envelope slope
						SimContext::CD4_RESPONSE_TYPE cd4Response = patient->getARTState()->CD4ResponseType;
						double cd4PercSlopeMean = pedsART->CD4PercentageChangeOnARTMeanEarly[SimContext::ART_EFF_SUCCESS][pedsAgeCat][cd4Response][i + 1];
						double cd4PercSlopeStdDev = pedsART->CD4PercentageChangeOnARTStdDevEarly[SimContext::ART_EFF_SUCCESS][pedsAgeCat][cd4Response][i + 1];
						double cd4PercSlope = CepacUtil::getRandomGaussian(cd4PercSlopeMean, cd4PercSlopeStdDev, 70055, patient);

						setCD4EnvelopeSlope(SimContext::ENVL_CD4_PERC_INDIV, cd4PercSlope);
					}
				}
			}
		}
	}
} /* end performARTEnvelopeEfficacyUpdates */

/** \brief performProphEfficacyUpdates handles all efficacy updates for prophylaxis */
void DrugEfficacyUpdater::performProphEfficacyUpdates() {
	/** Loop over all prophs that the patient is currently on */
	for (int i = 0; i < SimContext::OI_NUM; i++) {
		if (patient->getProphState()->isOnProph[i]) {
			SimContext::PROPH_TYPE prophType = patient->getProphState()->currProphType[i];
			int prophNum = patient->getProphState()->currProphNum[i];
			const SimContext::ProphInputs *prophInputs;
			if(patient->getGeneralState()->ageCategoryPediatrics>=SimContext::PEDS_AGE_LATE){
				prophInputs=simContext->getProphInputs(prophType, i, prophNum);
			}
			else{
				prophInputs=simContext->getPedsProphInputs(prophType, i, prophNum);
			}
			/** Trigger an emergency clinic visit if max month number, months on proph,
			//	or month to switch proph line has been reached */
			int monthsOnProph = patient->getGeneralState()->monthNum - patient->getProphState()->monthOfProphStart[i];
			const SimContext::TreatmentInputs::ProphStopPolicy &stopProph = simContext->getTreatmentInputs()->stopProph[prophType][i];
			if ((stopProph.minMonthNum != SimContext::NOT_APPL)&&
				(patient->getGeneralState()->monthNum >= stopProph.minMonthNum)) {
					scheduleEmergencyClinicVisit(true, patient->getGeneralState()->monthNum);
			}
			else if ((stopProph.monthsOnProph != SimContext::NOT_APPL) &&
				(monthsOnProph >= stopProph.monthsOnProph)) {
					scheduleEmergencyClinicVisit(true, patient->getGeneralState()->monthNum);
			}
			else if ((prophInputs->monthsToSwitch != SimContext::NOT_APPL) &&
				(monthsOnProph >= prophInputs->monthsToSwitch)) {
					scheduleEmergencyClinicVisit(true, patient->getGeneralState()->monthNum);
			}

			/** Evaluate if proph resistance should begin for this drug */
			if (!patient->getProphState()->useProphResistance[i]) {
				double resistTime = prophInputs->timeOfResistance;
				if (patient->getProphState()->isNonCompliant)
					resistTime = resistTime / (1 - simContext->getCohortInputs()->OIProphNonComplianceDegree);
				if (patient->getGeneralState()->monthNum - patient->getProphState()->monthOfProphStart[i] > resistTime) {
					double randNum = CepacUtil::getRandomDouble(70060, patient);
					if (randNum < prophInputs->monthlyProbResistance) {
						setProphResistance((SimContext::OI_TYPE) i);
					}
				}
			}
		}
	}
} /* end performProphEfficacyUpdates */

/** \brief performTBProphEfficacyUpdates handles all efficacy updates for TB prophylaxis */
void DrugEfficacyUpdater::performTBProphEfficacyUpdates() {
	/** Roll for TB proph dropout */
	double randNum = CepacUtil::getRandomDouble(70070, patient);
	if (randNum < simContext->getTBInputs()->probDropoffProph) {
		int prophNum = patient->getTBState()->currProphNum;
		stopCurrTBProph();

		/** Output tracing if enabled */
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "**%d TB PROPH %d DROPOUT;\n",
				patient->getGeneralState()->monthNum, prophNum + 1);
		}
	}
} /* end performTBProphEfficacyUpdates */

/** \brief performTBTreatmentEfficacyUpdates handles all efficacy updates for TB treatment */
void DrugEfficacyUpdater::performTBTreatmentEfficacyUpdates() {
	int monthsTreat = patient->getGeneralState()->monthNum - patient->getTBState()->monthOfTreatmentStart;
	int monthsInfect = patient->getGeneralState()->monthNum - patient->getTBState()->monthOfTBInfection;
	SimContext::TB_TREATM_STAGE treatStage = patient->getTBState()->currTreatmentStage;
	SimContext::TB_STRAIN tbStrain = patient->getTBState()->currTrueTBResistanceStrain;
	SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;

	/** If intended duration of treatment has not yet been reached, roll for dropout */
	if (monthsTreat < simContext->getTBInputs()->monthsTreatmentDuration[treatStage]) {
		double randNum = CepacUtil::getRandomDouble(70080, patient);
		if (randNum < simContext->getTBInputs()->probTreatmentDropout[treatStage]) {
			/** - Output tracing if enabled */
			if (patient->getGeneralState()->tracingEnabled) {
				tracer->printTrace(1, "**%d TB TREAT %s DROPOUT;\n",
					patient->getGeneralState()->monthNum, SimContext::TB_TREATM_STAGE_STRS[treatStage]);
			}

			/** - Calculate the probability of the partial treatment resulting in cure */
			double probSpontResol = simContext->getTBInputs()->probSpontaneousResolution[tbStrain][cd4Strata][SimContext::TB_MTH_PERIODS_NUM];
			for (int i = 0; i < SimContext::TB_MTH_PERIODS_NUM; i++) {
				if (monthsInfect < simContext->getTBInputs()->probSpontaneousResolutionStageBounds[tbStrain][i]) {
					probSpontResol = simContext->getTBInputs()->probSpontaneousResolution[tbStrain][cd4Strata][i];
					break;
				}
			}
			double percentTreat = ((double) monthsTreat / simContext->getTBInputs()->monthsTreatmentDuration[treatStage]);
			double totalProbTrueOrFalseCure = simContext->getTBInputs()->probCuredAfterTreatment[tbStrain][treatStage][SimContext::TB_CURE_TRUE] +
											simContext->getTBInputs()->probCuredAfterTreatment[tbStrain][treatStage][SimContext::TB_CURE_FALSE];
			double probCure = percentTreat * totalProbTrueOrFalseCure +	(1 - percentTreat) * probSpontResol;

			/** - Roll for TB being cured after treatment */
			double randNum = CepacUtil::getRandomDouble(60240, patient); //use same seed as used to determine outcome of treatment in clinicVisitUpdater
			if (randNum < probCure) {
				/** - If partial treatment was successful, transition to history of active */
				stopCurrTBTreatment(false, true);
				/** Determine the history of active substate, assuming additive probabilities */
				SimContext::TB_HIST_ACTV_STATE histActiveSubstate = SimContext::TB_HIST_ACTV_AFTER_SELF;
				if (randNum < percentTreat * simContext->getTBInputs()->probCuredAfterTreatment[tbStrain][treatStage][SimContext::TB_CURE_TRUE]){
					histActiveSubstate = SimContext::TB_HIST_ACTV_AFTER_TRUE;
				} else if (randNum < percentTreat * totalProbTrueOrFalseCure){
					histActiveSubstate = SimContext::TB_HIST_ACTV_AFTER_FALSE;
				}
				setTBDiseaseState(SimContext::TB_STATE_HIST_ACTV, histActiveSubstate);
				/** - Output tracing if enabled */
				if (patient->getGeneralState()->tracingEnabled) {
					tracer->printTrace(1, "**%d TB CURED @ DROPOUT (move to %s);\n", patient->getGeneralState()->monthNum, SimContext::TB_HIST_ACTV_STATE_STRS[histActiveSubstate]);
				}
			}
			else {
				/** If partial treatment failed, return back to the active TB state */
				stopCurrTBTreatment(false, false);
				setTBDiseaseState(SimContext::TB_STATE_ACTIVE);
			}
		}
	}
} /* end performTBTreatmentEfficacyUpdates */
