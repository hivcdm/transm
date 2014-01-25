#include "include.h"

/** \brief Constructor takes in the patient object */
BeginMonthUpdater::BeginMonthUpdater(Patient *patient) : StateUpdater(patient) {

}

/** \brief Destructor is empty, no cleanup required */
BeginMonthUpdater::~BeginMonthUpdater(void) {

}

/** \brief performInitialUpdates perform all of the state and statistics updates upon patient creation */
void BeginMonthUpdater::performInitialUpdates() {

	/** First call the parent function to perform general updates and initialization */
	StateUpdater::performInitialUpdates();

	/** Initialize the general patient state */
	int patientNum = runStats->getPopulationSummary()->numCohorts + 1;
	/** Increase numCohorts here so that the next created patient has a different patientNum, even if this patient isn't dead (i.e. in the transmission model) */
	this->incrementCohortSize();
	bool tracingEnabled = (patientNum <= SimContext::numPatientsToTrace);
	initializePatient(patientNum, tracingEnabled);

	/** Determine the patients gender */
	double randNum = CepacUtil::getRandomDouble(20010, patient);
	SimContext::GENDER_TYPE gender = SimContext::GENDER_FEMALE;
	if (randNum < simContext->getCohortInputs()->maleGenderDistribution)
		gender = SimContext::GENDER_MALE;

	/** Determine the patients age */
	double ageMonthsMean;
	double ageMonthsStdDev;
	if (simContext->getPedsInputs()->enablePediatricsModel){
		ageMonthsMean=simContext->getPedsInputs()->initialAgeMean;
		ageMonthsStdDev=simContext->getPedsInputs()->initialAgeStdDev;
	}
	else{
		ageMonthsMean = simContext->getCohortInputs()->initialAgeMean;
		ageMonthsStdDev = simContext->getCohortInputs()->initialAgeStdDev;
	}

	int ageMonths = (int) (CepacUtil::getRandomGaussian(ageMonthsMean, ageMonthsStdDev, 20020, patient) + 0.5);
	if (ageMonths < 0)
		ageMonths = 0;
	else if (ageMonths > 1200)
		ageMonths = 1200;

	/** If we want the pre-assigned age and gender (i.e. transmission model), set that here */
	if (this->patient->getGeneralState()->predefinedAgeAndGender){
		gender = this->patient->getGeneralState()->gender;
		ageMonths = this->patient->getGeneralState()->ageMonths;
	}

	/** Set the age and gender of the patient */
	setPatientAgeGender(gender, ageMonths);

	/** Set the patients prevalence of risk factors */
	for (int i = 0; i < SimContext::RISK_FACT_NUM; i++) {
		bool hasRisk = false;
		randNum = CepacUtil::getRandomDouble(60050, patient);
		if (randNum < simContext->getCohortInputs()->probRiskFactorPrev[i])
			hasRisk = true;
		setRiskFactor(i, hasRisk, true);
	}
} /* end performInitialUpdates */
//

/** \brief performMonthlyUpdates perform all of the state and statistics updates for a simulated month */
void BeginMonthUpdater::performMonthlyUpdates() {
	/** Set the discount factor, and reset the QOL multiplier, non-AIDS death increase, and acute OI */
	setQOLMultiplier(1.0);
	setNonAIDSDeathRateMultiplier(1.0);
	setCurrTrueOI(SimContext::OI_NONE);
	clearMortalityRisks();

	/** Do special processing for the initial month: */
	if (patient->getGeneralState()->monthNum == patient->getGeneralState()->initialMonthNum) {
	//if (patient->getGeneralState()->monthNum == 0) {
		if (patient->getGeneralState()->tracingEnabled) {
			tracer->printTrace(1, "\n\nBEGIN PATIENT %d\n", patient->getGeneralState()->patientNum);
		}

		if (patient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_NEG) {
			/** Print out initial patient tracing for HIV negative patients */
			if (patient->getGeneralState()->tracingEnabled) {
				tracer->printTrace(1, "  gender: %s, init age: %d mths (%1.2lf yrs)\n",
					(patient->getGeneralState()->gender == SimContext::GENDER_MALE) ? "male" : "female",
					patient->getGeneralState()->ageMonths, patient->getGeneralState()->ageMonths / 12.0 );
				tracer->printTrace(1, "  CD4 response: %s, init VisitType %s, Implement proph %s art %s;\n",
					SimContext::CD4_RESPONSE_STRS[patient->getARTState()->CD4ResponseType],
					SimContext::CLINIC_VISITS_STRS[patient->getMonitoringState()->clinicVisitType],
					(patient->getARTState()->mayReceiveART) ? "YES" : "NO",
					(patient->getProphState()->mayReceiveProph) ? "YES" : "NO" );
				if (patient->getGeneralState()->ageCategoryPediatrics < SimContext::PEDS_AGE_ADULT) {
					tracer->printTrace(1, "  Peds HIV state: %s\n",
						SimContext::PEDS_HIV_STATE_STRS[patient->getDiseaseState()->infectedPediatricsHIVState]);
				}
				tracer->printTrace(1, "  HIV state: %s\n",
					SimContext::HIV_INF_STRS[patient->getDiseaseState()->infectedHIVState]);
				for (int i = 0; i < SimContext::CHRM_NUM; i++) {
					if (patient->getDiseaseState()->hasTrueCHRMs[i]) {
						tracer->printTrace(1, "  init prevalent CHRMs: %s\n", SimContext::CHRM_STRS[i]);
					}
				}
			}
		}
		else {
			/** If patient is initially infected, update the initial distribution at time of infection statistics */
			updateInitialDistributions();

			/** Print out initial patient tracing for prevalent HIV positive patients */
			if (patient->getGeneralState()->tracingEnabled) {
				tracer->printTrace(1, "  gender: %s, init age: %d mths (%1.2lf yrs)\n",
					(patient->getGeneralState()->gender == SimContext::GENDER_MALE) ? "male" : "female",
					patient->getGeneralState()->ageMonths, patient->getGeneralState()->ageMonths / 12.0 );
				if (patient->getGeneralState()->ageCategoryPediatrics < SimContext::PEDS_AGE_LATE) {
					tracer->printTrace(1, "  init CD4 perc: %1.3f %s;\n", patient->getDiseaseState()->currTrueCD4Percentage,
						SimContext::CD4_STRATA_STRS[patient->getDiseaseState()->currTrueCD4Strata]);
				}
				else {
					tracer->printTrace(1, "  init CD4: %1.0f %s;\n", patient->getDiseaseState()->currTrueCD4,
						SimContext::CD4_STRATA_STRS[patient->getDiseaseState()->currTrueCD4Strata]);
				}
				tracer->printTrace(1, "  init HVL: %s, setpt: %s;\n",
					SimContext::HVL_STRATA_STRS[patient->getDiseaseState()->currTrueHVLStrata],
					SimContext::HVL_STRATA_STRS[patient->getDiseaseState()->setpointHVLStrata]);
				tracer->printTrace(1, "  CD4 response: %s, init VisitType %s, Implement proph %s art %s;\n",
					SimContext::CD4_RESPONSE_STRS[patient->getARTState()->CD4ResponseType],
					SimContext::CLINIC_VISITS_STRS[patient->getMonitoringState()->clinicVisitType],
					(patient->getARTState()->mayReceiveART) ? "YES" : "NO",
					(patient->getProphState()->mayReceiveProph) ? "YES" : "NO" );
				if (patient->getGeneralState()->ageCategoryPediatrics < SimContext::PEDS_AGE_ADULT) {
					tracer->printTrace(1, "  Peds HIV state: %s\n",
						SimContext::PEDS_HIV_STATE_STRS[patient->getDiseaseState()->infectedPediatricsHIVState]);
				}
				tracer->printTrace(1, "  HIV state: %s, %s\n",
					SimContext::HIV_INF_STRS[patient->getDiseaseState()->infectedHIVState],
					(patient->getMonitoringState()->isDetectedHIVPositive) ? "detected" : "undetected");
				for (int i = 0; i < SimContext::OI_NUM; i++) {
					if (patient->getDiseaseState()->hasTrueOIHistory[i]) {
						tracer->printTrace(1, "  init OI history: %s\n", SimContext::OI_STRS[i]);
					}
				}
				for (int i = 0; i < SimContext::CHRM_NUM; i++) {
					if (patient->getDiseaseState()->hasTrueCHRMs[i]) {
						tracer->printTrace(1, "  init prevalent CHRMs: %s\n", SimContext::CHRM_STRS[i]);
					}
				}
			}
		}
	}
} /* end performMonthlyUpdates */
