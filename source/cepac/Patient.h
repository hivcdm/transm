#pragma once

#include "include.h"

/*
	Patient class holds all of the stats for a patient running in the simulation.  The main
	entry point is the simulateMonth function, which calls all of the StateUpdaters to run the
	CEPAC simulations for a single month and update the patient state and statistics.
	The Patient class creates and initializes the StateUpdater objects for each particular
	Patient as part of the constructor.  Read only access to the patient state objects
	are provided through accessor functions that return const pointers to the
	subclass objects.
*/
class Patient
{
public:
	/* Make the StateUpdater class a friend class so it can modify the private data */
	friend class StateUpdater;

	/* Constructor and Destructor */
	Patient(SimContext *simContext, RunStats *runStats, Tracer *tracer, bool _predefinedAgeAndGender = false, int _ageMonths = 0, SimContext::GENDER_TYPE _gender = SimContext::GENDER_FEMALE, bool _setAsIncidentCase = false, int startingMonth = 0);
	~Patient(void);

	/* GeneralState class holds information about the patients initial characteristics
		and total costs and survival numbers */
	class GeneralState {
	public:
		int patientNum;
		bool predefinedAgeAndGender; //Will default to false; true if transmission (or other?) wants to assign patient age and gender instead of drawing from distribution
		bool tracingEnabled;
		double discountFactor;
		double QOLMultiplier;
		double nonAIDSDeathRateMultiplier;
		int monthNum;
		int initialMonthNum;
		int ageMonths;
		int ageCategoryClinical;
		int ageCategoryHIVInfection;
		SimContext::PEDS_AGE_CAT ageCategoryPediatrics;
		SimContext::GENDER_TYPE gender;
		bool hasRiskFactor[SimContext::RISK_FACT_NUM];
		bool isMotherAlive;
		SimContext::PEDS_MOM_HIV_STATE maternalInfectedHIVState;
		int monthOfMaternalHIVInfection;
		SimContext::PEDS_BF_TYPE breastfeedingStatus;
		double costsDiscounted;
		double LMsDiscounted;
		double qualityAdjustLMsDiscounted;
		bool loggedPatientOIs;
	}; /* end GeneralState */

	/* DiseaseState contains information about the HIV infection state, CD4/HVL levels,
		OIs and OI history, and death status */
	class DiseaseState {
	public:
		SimContext::HIV_INF infectedHIVState;
		SimContext::PEDS_HIV_STATE infectedPediatricsHIVState;
		bool isPrevalentHIVCase;
		int monthOfHIVInfection;
		int monthOfAcuteToChronicHIV;
		double currTrueCD4;
		double minTrueCD4;
		SimContext::CD4_STRATA currTrueCD4Strata;
		SimContext::CD4_STRATA minTrueCD4Strata;
		double currTrueCD4Percentage;
		double minTrueCD4Percentage;
		SimContext::PEDS_CD4_PERC currTrueCD4PercentageStrata;
		SimContext::HVL_STRATA currTrueHVLStrata;
		SimContext::HVL_STRATA setpointHVLStrata;
		SimContext::HVL_STRATA targetHVLStrata;
		bool hasCurrTrueOI;
		SimContext::OI_TYPE typeCurrTrueOI;
		SimContext::HIST_EXT typeTrueOIHistory;
		bool hasTrueOIHistory[SimContext::OI_NUM];
		int numTrueOIsSinceLastVisit[SimContext::OI_NUM];
		bool hasTrueCHRMs[SimContext::CHRM_NUM];
		int monthOfCHRMsStart[SimContext::CHRM_NUM];
		bool isAlive;
		vector<SimContext::MortalityRisk> mortalityRisks;
		SimContext::DTH_CAUSES causeOfDeath;
	}; /* end DiseaseState */

	/* MonitoringState contains information for testing, clinical visits, and observed health state */
	class MonitoringState {
	public:
		bool isDetectedHIVPositive;
		bool isHighRiskForHIV;
		int intervalHIVTest;
		bool hasScheduledHIVTest;
		int monthOfScheduledHIVTest;
		double acceptanceRateHIVTest;
		bool hasObservedCD4;
		int monthOfObservedCD4;
		double currObservedCD4;
		double minObservedCD4;
		SimContext::CD4_STRATA currObservedCD4Strata;
		bool hasObservedCD4Percentage;
		int monthOfObservedCD4Percentage;
		double currObservedCD4Percentage;
		double minObservedCD4Percentage;
		SimContext::PEDS_CD4_PERC currObservedCD4PercentageStrata;
		bool hasObservedHVLStrata;
		int monthOfObservedHVLStrata;
		SimContext::HVL_STRATA currObservedHVLStrata;
		SimContext::HVL_STRATA maxObservedHVLStrata;
		int numObservedOIsTotal[SimContext::OI_NUM];
		int numObservedOIsSinceLastVisit[SimContext::OI_NUM];
		SimContext::CLINIC_VISITS clinicVisitType;
		bool hasRegularClinicVisit;
		int monthOfRegularClinicVisit;
		bool hasEmergencyClinicVisit;
		int monthOfEmergencyClinicVisit;
		bool hadPrevClinicVisit;
		bool hasScheduledCD4Test;
		int monthOfScheduledCD4Test;
		bool hasScheduledHVLTest;
		int monthOfScheduledHVLTest;
		SimContext::LTFU_STATE currLTFUState;
		bool hadPrevLTFU;
		bool hadPrevRTC;
		int monthOfLTFUStateChange;
		bool wasOnARTWhenLostToFollowUp;
	}; /* end MonitoringState */

	/* ProphState holds information about the current and previous prophylaxis taken,
		and state pertinent to future prophylaxis policy */
	class ProphState {
	public:
		bool mayReceiveProph;
		bool isNonCompliant;
		int currTotalNumProphsOn;
		bool isOnProph[SimContext::OI_NUM];
		SimContext::PROPH_TYPE currProphType[SimContext::OI_NUM];
		int currProphNum[SimContext::OI_NUM];
		int monthOfProphStart[SimContext::OI_NUM];
		bool hasNextProphAvailable[SimContext::OI_NUM];
		SimContext::PROPH_TYPE nextProphType[SimContext::OI_NUM];
		int nextProphNum[SimContext::OI_NUM];
		SimContext::PROPH_TOX_TYPE typeProphToxicity[SimContext::OI_NUM];
		bool useProphResistance[SimContext::OI_NUM];
		bool hasTakenProph[SimContext::OI_NUM][SimContext::PROPH_NUM_TYPES];
	}; /* end ProphState */

	/* ARTState holds information about current and previous ART regimens taken, and
		state pertinent to future ART treatment policy */
	class ARTState {
	public:
		bool mayReceiveART;
		bool isOnART;
		bool hasTakenART;
		bool isOnPediatricART;
		SimContext::CD4_RESPONSE_TYPE CD4ResponseType;
		int currRegimenNum;
		int monthOfCurrRegimenStart;
		int prevRegimenNum;
		int monthOfPrevRegimenStop;
		bool hasNextRegimenAvailable;
		int nextRegimenNum;
		int currSubRegimenNum;
		int monthOfCurrSubRegimenStart;
		SimContext::ART_EFF_TYPE currRegimenEfficacy;
		SimContext::ART_EFF_TYPE prevRegimenEfficacy;
		int monthOfEfficacyChange;
		double responseBaselineLogit;
		double responsePropensityCurrRegimen;
		double responseFactorCurrRegimen;
		SimContext::RESP_TYPE responseTypeCurrRegimen;
		double currRegimenCD4Slope;
		double currRegimenCD4PercentageSlope;
		SimContext::CD4Envelope overallCD4Envelope;
		SimContext::CD4Envelope indivCD4Envelope;
		SimContext::CD4Envelope overallCD4PercentageEnvelope;
		SimContext::CD4Envelope indivCD4PercentageEnvelope;
		SimContext::HVL_STRATA observedHVLStrataAtRegimenStart;
		double maxObservedCD4OnCurrART;
		double maxObservedCD4PercentageOnCurrART;
		SimContext::HVL_STRATA minObservedHVLStrataOnCurrART;
		int numObservedOIsSinceFailOrStopART[SimContext::OI_NUM];
		int numFailedCD4Tests;
		int numFailedHVLTests;
		int numFailedOIs;
		bool hasObservedFailure;
		SimContext::ART_FAIL_TYPE typeObservedFailure;
		int monthOfObservedFailure;
		int numObservedFailures;
		SimContext::ART_STOP_TYPE typeCurrStop;
		int numMonthsOnUnsuccessfulByRegimen[SimContext::ART_NUM_LINES];
		int numMonthsOnUnsuccessfulByHVL[SimContext::HVL_NUM_STRATA];
		list<SimContext::ARTToxicityEffect> activeToxicityEffects;
		bool hasMajorToxicity;
		bool hasSevereToxicity;
		const SimContext::ARTToxicityEffect *severeToxicityEffect;
		bool hadPrevToxicity;
		SimContext::STI_STATE currSTIState;
		int monthOfSTIStateChange;
		int monthOfSTIInitialStop;
		int numSTIInterruptionsOnCurrRegimen;
	}; /* end ARTState */

	/* TBState holds information about the current TB disease and treatment, and
		information pertinent to future TB treatment policy */
	class TBState {
	public:
		SimContext::TB_STATE currTrueTBDiseaseState;
		SimContext::TB_STRAIN currTrueTBResistanceStrain;
		int monthOfTBStateChange;
		int monthOfTBInfection;
		bool hasObservedHistoryActiveTB;
		bool isOnProph;
		bool isScheduledForProph;
		int currProphNum;
		int monthOfProphStart;
		int nextProphNum;
		bool hasNextProphAvailable;
		bool hasMajorProphToxicity;
		bool isOnTreatment;
		bool isScheduledForTreatment;
		SimContext::TB_TREATM_STAGE currTreatmentStage;
		SimContext::TB_TREATM_STAGE nextTreatmentStage;
		int monthOfTreatmentStart;
		bool hasMajorTreatmentToxicity;
	}; /* end TBState */

	/* Accessor functions return const pointers to the Patient state subclass objects */
	const GeneralState *getGeneralState();
	const DiseaseState *getDiseaseState();
	const MonitoringState *getMonitoringState();
	const ProphState *getProphState();
	const ARTState *getARTState();
	const TBState *getTBState();

	/* Accessor functions return pointers to the CD4/HVL test state updaters */
	CD4TestUpdater *getCD4TestUpdater();
	HVLTestUpdater *getHVLTestUpdater();

	/* simulateMonth runs a single month of simulation for this patient, and updates
		its state and runStats statistics */
	void simulateMonth();
	/* Force new infection sets the patient to a new infected state -- to be used primarily by the transmission model*/
	void forceNewInfection();
	/* Changes the inputs that the patient uses to determine disease progression -- to be used primarily by the transmission model*/
	void setSimContext(SimContext *newSimContext);
	/* isAlive returns true if the patient is alive, false if dead */
	bool isAlive();
	/* saveState saves the state of the patient to file format using a JSON like notation */
	void saveState(ostream & _outStream);

	SimContext *getSimContext() { return simContext; }

private:
	/* pointers to the simulation context, statistics object, and trace object */
	SimContext *simContext;
	RunStats *runStats;
	Tracer *tracer;

	/* Patient state subclass objects */
	GeneralState generalState;
	DiseaseState diseaseState;
	MonitoringState monitoringState;
	ProphState prophState;
	ARTState artState;
	TBState tbState;

	/* State updater objects */
	BeginMonthUpdater beginMonthUpdater;
	HIVInfectionUpdater hivInfectionUpdater;
	CHRMsUpdater chrmsUpdater;
	DrugToxicityUpdater drugToxicityUpdater;
	TBDiseaseUpdater tbDiseaseUpdater;
	AcuteOIUpdater acuteOIUpdater;
	MortalityUpdater mortalityUpdater;
	CD4HVLUpdater cd4HVLUpdater;
	HIVTestingUpdater hivTestingUpdater;
	BehaviorUpdater behaviorUpdater;
	DrugEfficacyUpdater drugEfficacyUpdater;
	CD4TestUpdater cd4TestUpdater;
	HVLTestUpdater hvlTestUpdater;
	ClinicVisitUpdater clinicVisitUpdater;
	EndMonthUpdater endMonthUpdater;
};

/* getGeneralState returns a const pointer to the GeneralState object */
inline const Patient::GeneralState *Patient::getGeneralState() {
	return &generalState;
}

/* getDiseaseState returns a const pointer to the DiseaseState object */
inline const Patient::DiseaseState *Patient::getDiseaseState() {
	return &diseaseState;
}

/* getMonitoringState returns a const pointer to the MonitoringState object */
inline const Patient::MonitoringState *Patient::getMonitoringState() {
	return &monitoringState;
}

/* getProphState returns a const pointer to the ProphState object */
inline const Patient::ProphState *Patient::getProphState() {
	return &prophState;
}

/* getARTState returns a const pointer to the ARTState object */
inline const Patient::ARTState *Patient::getARTState() {
	return &artState;
}

/* getTBState returns a const pointer to the TBState object */
inline const Patient::TBState *Patient::getTBState() {
	return &tbState;
}

/* getCD4TestUpdater returns a pointer to the CD4 test state updater */
inline CD4TestUpdater *Patient::getCD4TestUpdater() {
	return &cd4TestUpdater;
}

/* getHVLTestUpdater returns a pointer to the HVL test state updater */
inline HVLTestUpdater *Patient::getHVLTestUpdater() {
	return &hvlTestUpdater;
}

/* Force new infection if the patient is negative; to be called by the transmission model*/
inline void Patient::forceNewInfection() {
	if (diseaseState.infectedHIVState == SimContext::HIV_INF_NEG){
		this->hivInfectionUpdater.performHIVNewInfectionUpdates();
	}
}

/* Changes the inputs that the patient uses to determine disease progression -- to be used primarily by the transmission model*/
inline void Patient::setSimContext(SimContext *newSimContext){
	this->simContext = newSimContext;
	//Set simContext for all stateUpdaters
	beginMonthUpdater.setSimContext(newSimContext);
	hivInfectionUpdater.setSimContext(newSimContext);
	chrmsUpdater.setSimContext(newSimContext);
	drugToxicityUpdater.setSimContext(newSimContext);
	tbDiseaseUpdater.setSimContext(newSimContext);
	acuteOIUpdater.setSimContext(newSimContext);
	mortalityUpdater.setSimContext(newSimContext);
	cd4HVLUpdater.setSimContext(newSimContext);
	hivTestingUpdater.setSimContext(newSimContext);
	behaviorUpdater.setSimContext(newSimContext);
	drugEfficacyUpdater.setSimContext(newSimContext);
	cd4TestUpdater.setSimContext(newSimContext);
	hvlTestUpdater.setSimContext(newSimContext);
	clinicVisitUpdater.setSimContext(newSimContext);
	endMonthUpdater.setSimContext(newSimContext);
}

/* isAlive returns true if the patient is alive, false if dead */
inline bool Patient::isAlive() {
	if (diseaseState.isAlive)
		return true;
	return false;
} /* end isAlive */


