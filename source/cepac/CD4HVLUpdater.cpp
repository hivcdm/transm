#include "include.h"

/** \brief Constructor takes in the patient object */
CD4HVLUpdater::CD4HVLUpdater(Patient *patient) : StateUpdater(patient) {

}

/** \brief Destructor is empty, no cleanup required */
CD4HVLUpdater::~CD4HVLUpdater(void) {

}

/** \brief performInitialUpdates perform all of the state and statistics updates upon patient creation */
void CD4HVLUpdater::performInitialUpdates() {
	/** Calls the parent function to perform general updates and initialization */
	StateUpdater::performInitialUpdates();
} /* end performInitialUpdates */

/** \brief performMonthlyUpdates perform all of the state and statistics updates for a simulated month */
void CD4HVLUpdater::performMonthlyUpdates() {
	const SimContext::NatHistInputs *natHist = simContext->getNatHistInputs();
	SimContext::PEDS_AGE_CAT ageCat = patient->getGeneralState()->ageCategoryPediatrics;

	/** determine if HVL should be adjusted towards the target HVL */
	SimContext::HVL_STRATA hvlStrata = patient->getDiseaseState()->currTrueHVLStrata;
	SimContext::HVL_STRATA targetHVL = patient->getDiseaseState()->targetHVLStrata;

	//Draws the patient specific cd4 decline increment if it hasn't been drawn
	if (!patient->getDiseaseState()->hasDrawnPatientSpecificCD4Decline){
		double cd4Perc=CepacUtil::getRandomGaussian(0, natHist->monthlyCD4DeclineBtwSubject,40005,patient);
		int count = 0;
		while(cd4Perc<-1)
		{
			count++;
			cd4Perc=CepacUtil::getRandomGaussian(0, natHist->monthlyCD4DeclineBtwSubject,40005+count,patient);
		}
		setPatientNatHistSlopePerc(cd4Perc);
	}

	if (targetHVL != hvlStrata) {
		int artLineNum;
		SimContext::ART_EFF_TYPE efficacy;
		if (patient->getARTState()->isOnART) {
			artLineNum = patient->getARTState()->currRegimenNum;
			efficacy = patient->getARTState()->currRegimenEfficacy;
		}
		else {
			artLineNum = patient->getARTState()->prevRegimenNum;
			efficacy = SimContext::ART_EFF_FAILURE;
		}
		double probHVLChange = 0.0;
		int numHVLChange = 0;

		if (ageCat == SimContext::PEDS_AGE_ADULT) {
			probHVLChange = simContext->getARTInputs(artLineNum)->monthlyProbHVLChange[efficacy];
			numHVLChange = simContext->getARTInputs(artLineNum)->monthlyNumStrataHVLChange[efficacy];
		}
		else if (ageCat == SimContext::PEDS_AGE_LATE) {
			probHVLChange = simContext->getPedsARTInputs(artLineNum)->monthlyProbHVLChangeLate[efficacy];
			numHVLChange = simContext->getPedsARTInputs(artLineNum)->monthlyNumStrataHVLChangeLate[efficacy];
		}
		else {
			probHVLChange = simContext->getPedsARTInputs(artLineNum)->monthlyProbHVLChangeEarly[efficacy];
			numHVLChange = simContext->getPedsARTInputs(artLineNum)->monthlyNumStrataHVLChangeEarly[efficacy];
		}
		double randNum = CepacUtil::getRandomDouble(40010, patient);
		if (randNum < probHVLChange) {
			int newHVL = hvlStrata;
			if (targetHVL < hvlStrata) {
				newHVL -= numHVLChange;
				if (newHVL < targetHVL)
					newHVL = targetHVL;
			}
			else {
				newHVL += numHVLChange;
				if (newHVL > targetHVL)
					newHVL = targetHVL;
			}
			setTrueHVLStrata((SimContext::HVL_STRATA) newHVL);
		}
	}

	if (ageCat >= SimContext::PEDS_AGE_LATE) {
		/** For adults, update the overall CD4 envelope if it has been established */
		if (patient->getARTState()->overallCD4Envelope.isActive) {
			double cd4Slope = patient->getARTState()->overallCD4Envelope.slope;
			incrementCD4Envelope(SimContext::ENVL_CD4_OVERALL, cd4Slope);
		}
		/** For adults, update the individual regimen CD4 envelope if it has been established */
		if (patient->getARTState()->indivCD4Envelope.isActive) {
			double cd4Slope = patient->getARTState()->indivCD4Envelope.slope;
			incrementCD4Envelope(SimContext::ENVL_CD4_INDIV, cd4Slope);
		}
	}
	else {
		/** For pediatric patients, update the overall CD4 percentage envelope if it has been established */
		if (patient->getARTState()->overallCD4PercentageEnvelope.isActive) {
			double cd4PercSlope = patient->getARTState()->overallCD4PercentageEnvelope.slope;
			incrementCD4Envelope(SimContext::ENVL_CD4_PERC_OVERALL, cd4PercSlope);
		}
		/** For pediatric patients, update the individual regimen CD4 percentage envelope if it has been established*/
		if (patient->getARTState()->indivCD4PercentageEnvelope.isActive) {
			double cd4PercSlope = patient->getARTState()->indivCD4PercentageEnvelope.slope;
			incrementCD4Envelope(SimContext::ENVL_CD4_PERC_INDIV, cd4PercSlope);
		}
	}

	/** For adults, update the true CD4 count */
	if (ageCat == SimContext::PEDS_AGE_ADULT) {
		/** - Calculate the natural history slope and lower bound */
		SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
		double natHistSlopeMean = simContext->getNatHistInputs()->monthlyCD4DeclineMean[cd4Strata][hvlStrata];
		natHistSlopeMean+=natHistSlopeMean * patient->getDiseaseState()->patientSpecificCD4DeclinePerc;
		double natHistSlopeStdDev = simContext->getNatHistInputs()->monthlyCD4DeclineStdDev[cd4Strata][hvlStrata];
		double natHistSlope = CepacUtil::getRandomGaussian(natHistSlopeMean, natHistSlopeStdDev, 40030, patient);

		double natHistCD4Bound = patient->getDiseaseState()->minTrueCD4 - natHistSlope;
		double newCD4 = patient->getDiseaseState()->currTrueCD4;
		if (patient->getARTState()->isOnART) {
			SimContext::ART_EFF_TYPE efficacy = patient->getARTState()->currRegimenEfficacy;
			int artLineNum = patient->getARTState()->currRegimenNum;
			const SimContext::ARTInputs *artInput = simContext->getARTInputs(artLineNum);
			if (efficacy != SimContext::ART_EFF_FAILURE) {
				/** - If patient is on suppressive ART, use the current regimen slope with monthly std dev */
				double onARTSlope = patient->getARTState()->currRegimenCD4Slope;
				double onARTMonthlyStdDev = artInput->secondaryCD4ChangeOnARTStdDev;
				onARTSlope += CepacUtil::getRandomGaussian(0, onARTMonthlyStdDev, 40040, patient);
				newCD4 += onARTSlope;
			}
			else {
				/** - If patient is on failed ART, use the natural history failed ART multiplier with monthly std dev */
				SimContext::CD4_RESPONSE_TYPE responseType = patient->getARTState()->CD4ResponseType;
				int monthsFailed = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfEfficacyChange;
				double onARTMult = 1.0;

				if (monthsFailed <= artInput->stageBoundCD4ChangeOnARTFail) {
					// Use first input if failing is due to late failure, skip to second for initial failure
					if (patient->getARTState()->monthOfEfficacyChange != patient->getARTState()->monthOfCurrRegimenStart){
						//late fail
						onARTMult = artInput->CD4MultiplierOnFailedART[responseType][0];
						if (monthsFailed == 1){
							//first month on failed art
							int monthOfNewCD4Mult=patient->getGeneralState()->monthNum+artInput->stageBoundCD4ChangeOnARTFail;
							setCD4MultOnARTFail(monthOfNewCD4Mult,onARTMult);
						}
					}
					else{
						//early fail
						if(patient->getGeneralState()->monthNum >= patient->getARTState()->monthOfNewCD4MultArtFail){
							onARTMult = artInput->CD4MultiplierOnFailedART[responseType][1];
						}
						else{
							onARTMult=patient->getARTState()->currCD4MultArtFail;
						}
					}
				}
				else{
					if (patient->getGeneralState()->monthNum >= patient->getARTState()->monthOfNewCD4MultArtFail){
						onARTMult = artInput->CD4MultiplierOnFailedART[responseType][1];
					}
					else{
						onARTMult = patient->getARTState()->currCD4MultArtFail;
					}
				}
				double onARTSlope = -1.0 * onARTMult * natHistSlope;
				double onARTMonthlyStdDev = artInput->secondaryCD4ChangeOnARTStdDev;
				onARTSlope += CepacUtil::getRandomGaussian(0, onARTMonthlyStdDev, 40050, patient);
				newCD4 += onARTSlope;
			}
		}
		else if (patient->getARTState()->hasTakenART) {
			/** - If Patient is no longer taking ART (but did in the past), use natural history and off ART multiplier */
			int artLineNum = patient->getARTState()->prevRegimenNum;
			const SimContext::ARTInputs *artInput = simContext->getARTInputs(artLineNum);
			SimContext::ART_EFF_TYPE efficacy = patient->getARTState()->prevRegimenEfficacy;
			double offARTMult = 1.0;
			if (hvlStrata != patient->getDiseaseState()->setpointHVLStrata)
				offARTMult = artInput->monthlyCD4MultiplierOffARTPreSetpoint[efficacy];
			else
				offARTMult = artInput->monthlyCD4MultiplierOffARTPostSetpoint[efficacy];
			newCD4 += (-1.0 * offARTMult * natHistSlope);
		}
		else {
			/** - If Patient has never taken ART, use only natural history */
			newCD4 += (-1.0 * natHistSlope);
		}

		/** Use the calculated new CD4 value unless it goes below the natural history bound */
		if (newCD4 >= natHistCD4Bound)
			setTrueCD4(newCD4);
		else
			setTrueCD4(natHistCD4Bound);
	}
	else if (ageCat == SimContext::PEDS_AGE_LATE) {
		/** For late childhood, calculate the CD4 count the same as for adults, but using the late childhood inputs*/
		// Calculate the natural history slope and lower bound
		SimContext::CD4_STRATA cd4Strata = patient->getDiseaseState()->currTrueCD4Strata;
		double natHistSlopeMean = simContext->getNatHistInputs()->monthlyCD4DeclineMean[cd4Strata][hvlStrata];
		natHistSlopeMean+=natHistSlopeMean*patient->getDiseaseState()->patientSpecificCD4DeclinePerc;
		double natHistSlopeStdDev = simContext->getNatHistInputs()->monthlyCD4DeclineStdDev[cd4Strata][hvlStrata];
		double natHistSlope = CepacUtil::getRandomGaussian(natHistSlopeMean, natHistSlopeStdDev, 40031, patient);

		double natHistCD4Bound = patient->getDiseaseState()->minTrueCD4 - natHistSlope;
		double newCD4 = patient->getDiseaseState()->currTrueCD4;
		if (patient->getARTState()->isOnART) {
			SimContext::ART_EFF_TYPE efficacy = patient->getARTState()->currRegimenEfficacy;
			int artLineNum = patient->getARTState()->currRegimenNum;

			const SimContext::PedsARTInputs *pedsART = simContext->getPedsARTInputs(artLineNum);
			if (efficacy != SimContext::ART_EFF_FAILURE) {
				// Patient is on suppressive ART, use the current regimen slope with monthly std dev
				double onARTSlope = patient->getARTState()->currRegimenCD4Slope;
				double onARTMonthlyStdDev = pedsART->secondaryCD4ChangeOnARTStdDevLate;
				onARTSlope += CepacUtil::getRandomGaussian(0, onARTMonthlyStdDev, 40041, patient);
				newCD4 += onARTSlope;
			}
			else {
				// Patient is on failed ART, use the natural history failed ART multiplier with monthly std dev
				SimContext::CD4_RESPONSE_TYPE responseType = patient->getARTState()->CD4ResponseType;
				int monthsFailed = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfEfficacyChange;
				double onARTMult = 1.0;

				if (monthsFailed <= pedsART->stageBoundCD4ChangeOnARTFailLate) {
					// Use first input if failing is due to late failure, skip to second for initial failure
					if (patient->getARTState()->monthOfEfficacyChange != patient->getARTState()->monthOfCurrRegimenStart){
						//late fail
						onARTMult = pedsART->CD4MultiplierOnFailedARTLate[responseType][0];
						if (monthsFailed == 1){
							//first month on failed art
							int monthOfNewCD4Mult=patient->getGeneralState()->monthNum+pedsART->stageBoundCD4ChangeOnARTFailLate;
							setCD4MultOnARTFail(monthOfNewCD4Mult,onARTMult);
						}
					}
					else{
						//early fail
						if (patient->getGeneralState()->monthNum >= patient->getARTState()->monthOfNewCD4MultArtFail){
							onARTMult = pedsART->CD4MultiplierOnFailedARTLate[responseType][1];
						}
						else{
							onARTMult = patient->getARTState()->currCD4MultArtFail;
						}
					}
				}
				else{
					if (patient->getGeneralState()->monthNum >= patient->getARTState()->monthOfNewCD4MultArtFail){
						onARTMult = pedsART->CD4MultiplierOnFailedARTLate[responseType][1];
					}
					else{
						onARTMult = patient->getARTState()->currCD4MultArtFail;
					}
				}
				double onARTSlope = -1.0 * onARTMult * natHistSlope;
				double onARTMonthlyStdDev = pedsART->secondaryCD4ChangeOnARTStdDevLate;
				onARTSlope += CepacUtil::getRandomGaussian(0, onARTMonthlyStdDev, 40051, patient);
				newCD4 += onARTSlope;
			}
		}
		else if (patient->getARTState()->hasTakenART) {
			// Patient is no longer taking ART, use natural history and off ART multiplier
			int artLineNum = patient->getARTState()->prevRegimenNum;
			const SimContext::PedsARTInputs *pedsART = simContext->getPedsARTInputs(artLineNum);
			SimContext::ART_EFF_TYPE efficacy = patient->getARTState()->prevRegimenEfficacy;
			double offARTMult = 1.0;
			if (hvlStrata != patient->getDiseaseState()->setpointHVLStrata)
				offARTMult = pedsART->monthlyCD4MultiplierOffARTPreSetpointLate[efficacy];
			else
				offARTMult = pedsART->monthlyCD4MultiplierOffARTPostSetpointLate[efficacy];
			newCD4 += (-1.0 * offARTMult * natHistSlope);
		}
		else {
			// Patient has never taken ART, use only natural history
			newCD4 += (-1.0 * natHistSlope);
		}

		// Use the calculated new CD4 value unless it goes below the natural history bound
		if (newCD4 >= natHistCD4Bound)
			setTrueCD4(newCD4);
		else
			setTrueCD4(natHistCD4Bound);
	}
	else {
		/** For early childhood Patient, update the true CD4 percentage */
		/** - Calculate the natural history slope and lower bound */
		SimContext::PEDS_HIV_STATE pedsHIVState = patient->getDiseaseState()->infectedPediatricsHIVState;
		SimContext::PEDS_CD4_PERC cd4PercStrata = patient->getDiseaseState()->currTrueCD4PercentageStrata;
		double natHistSlope = simContext->getPedsInputs()->monthlyCD4PercentDecline[pedsHIVState][ageCat][cd4PercStrata];
		double natHistCD4PercBound = patient->getDiseaseState()->minTrueCD4Percentage - natHistSlope;
		double newCD4Perc = patient->getDiseaseState()->currTrueCD4Percentage;
		if (patient->getARTState()->isOnART) {
			SimContext::ART_EFF_TYPE efficacy = patient->getARTState()->currRegimenEfficacy;
			int artLineNum = patient->getARTState()->currRegimenNum;
			const SimContext::PedsARTInputs *pedsART = simContext->getPedsARTInputs(artLineNum);
			if (efficacy != SimContext::ART_EFF_FAILURE) {
				/** - If Patient is on suppressive ART, use the current regimen slope with monthly std dev */
				double onARTSlope = patient->getARTState()->currRegimenCD4PercentageSlope;
				double onARTMonthlyStdDev = pedsART->secondaryCD4PercentageChangeOnARTStdDevEarly;
				onARTSlope += CepacUtil::getRandomGaussian(0, onARTMonthlyStdDev, 40042, patient);
				newCD4Perc += onARTSlope;
			}
			else {
				/** - If Patient is on failed ART, use the natural history failed ART multiplier with monthly std dev */
				SimContext::CD4_RESPONSE_TYPE responseType = patient->getARTState()->CD4ResponseType;
				int monthsFailed = patient->getGeneralState()->monthNum - patient->getARTState()->monthOfEfficacyChange;
				double onARTMult = 1.0;

				if (monthsFailed <= pedsART->stageBoundCD4PercentageChangeOnARTFailEarly) {
					// Use first input if failing is due to late failure, skip to second for initial failure
					if (patient->getARTState()->monthOfEfficacyChange != patient->getARTState()->monthOfCurrRegimenStart){
						//late fail
						onARTMult = pedsART->CD4PercentageMultiplierOnFailedARTEarly[responseType][0];
						if (monthsFailed == 1){
							//first month on failed art
							int monthOfNewCD4Mult=patient->getGeneralState()->monthNum+pedsART->stageBoundCD4PercentageChangeOnARTFailEarly;
							setCD4MultOnARTFail(monthOfNewCD4Mult,onARTMult);
						}
					}
					else{
						//early fail
						if (patient->getGeneralState()->monthNum >= patient->getARTState()->monthOfNewCD4MultArtFail){
							onARTMult = pedsART->CD4PercentageMultiplierOnFailedARTEarly[responseType][1];
						}
						else{
							onARTMult = patient->getARTState()->currCD4MultArtFail;
						}
					}
				}
				else{
					if (patient->getGeneralState()->monthNum >= patient->getARTState()->monthOfNewCD4MultArtFail){
						onARTMult = pedsART->CD4PercentageMultiplierOnFailedARTEarly[responseType][1];
					}
					else{
						onARTMult = patient->getARTState()->currCD4MultArtFail;
					}
				}
				double onARTSlope = -1.0 * onARTMult * natHistSlope;
				double onARTMonthlyStdDev = pedsART->secondaryCD4PercentageChangeOnARTStdDevEarly;
				onARTSlope += CepacUtil::getRandomGaussian(0, onARTMonthlyStdDev, 40052, patient);
				newCD4Perc += onARTSlope;
			}
		}
		else if (patient->getARTState()->hasTakenART) {
			/** - If Patient is no longer taking ART, use natural history and off ART multiplier */
			int artLineNum = patient->getARTState()->prevRegimenNum;
			const SimContext::PedsARTInputs *pedsART = simContext->getPedsARTInputs(artLineNum);
			SimContext::ART_EFF_TYPE efficacy = patient->getARTState()->prevRegimenEfficacy;
			double offARTMult = 1.0;
			if (hvlStrata != patient->getDiseaseState()->setpointHVLStrata)
				offARTMult = pedsART->monthlyCD4PercentageMultiplierOffARTPreSetpointEarly[efficacy];
			else
				offARTMult = pedsART->monthlyCD4PercentageMultiplierOffARTPostSetpointEarly[efficacy];

			newCD4Perc += (-1.0 * offARTMult * natHistSlope);
		}
		else {
			/** - Patient has never taken ART, use only natural history */
			newCD4Perc += (-1.0 * natHistSlope);
		}

		/** Use the calculated new CD4 percentage unless it goes below the natural history bound */
		if (newCD4Perc >= natHistCD4PercBound){
			setTrueCD4Percentage(newCD4Perc);
		}
		else
			setTrueCD4Percentage(natHistCD4PercBound);
	}
} /* end performMonthlyUpdates */

