#pragma once

#include "include.h"

/*
	The StateUpdater class is the base class for all simulation updater classes in the model.
	All of its child classes should implement the performInitialUpdates and performMonthlyUpdates
	virtual functions to perform the patient initialization and subsequent monthly updates to
	both the patient state and statistics objects.  If these functions are omitted in a
	given child class, the parent functions defined in this class will be used instead.
	The child classes cannot directly modify the patient or statistics information, instead they
	must use the update functions provided in this parent class (which is a friend class to both
	Patient and RunStats).
*/
class StateUpdater
{
public:
	/* Constructor and Destructor */
	StateUpdater(Patient *patient);
	virtual ~StateUpdater(void);

	/* performInitialUpdates perform all of the state and statistics updates upon patient creation */
	virtual void performInitialUpdates();
	/* performMonthlyUpdates perform all of the state and statistics updates for a simulated month */
	virtual void performMonthlyUpdates();
	/* changes the inputs the updater uses to determine disease progression -- to be used primarily by the transmission model*/
	void setSimContext(SimContext *newSimContext);

protected:

	/* Pointers to the patient state, simulation context, tracer, and run stats objects */
	Patient *patient;
	SimContext *simContext;
	RunStats *runStats;
	Tracer *tracer;

	/* Updater functions that are invoked by the StateUpdater child classes,
		child classes cannot directly modify state and must use these functions */
	/* initializePatient initializes the patients basic state */
	void initializePatient(int patientNum, bool tracingEnabled);
	/* setPatientAgeGender set the patients age and gender */
	void setPatientAgeGender(SimContext::GENDER_TYPE gender, int ageMonths);
	/* setInitialARTState sets the initial ART state to not be on ART */
	void setInitialARTState();
	/* setInitialProphState sets the initial proph state to not be on any prophs */
	void setInitialProphState();
	/* setInitialTBProphState sets the initial TB proph state to not be on any TB prophs */
	void setInitialTBProphState();
	/* setInitialTBTreatmentState sets the initial TB treatment state to not be on TB treatment */
	void setInitialTBTreatmentState();
	/* incrementMonth updates the simulation month number and patient age */
	void incrementMonth();
	/* incrementDiscountFactor adjusts the discounting factor */
	void incrementDiscountFactor(double amount);
	/* setQOLMultiplier resets the quality of life factor back to the specified level */
	void setQOLMultiplier(double newQOL);
	/* accumulateQOLMultiplier accumulates the QOL by multiplying the new factor with the existing one */
	void accumulateQOLMultiplier(double amount);
	/* setNonAIDSDeathRateMultiplier sets the value for additional probability of nonAIDS death */
	void setNonAIDSDeathRateMultiplier(double amount);
	/* accumulateNonAIDSDeathRateMultiplier increments the value for additional probability of nonAIDS death */
	void accumulateNonAIDSDeathRateMultiplier(double amount);
	/* setInfectedHIVState sets the patients to being HIV infected and updates statistics */
	void setInfectedHIVState(SimContext::HIV_INF infectedState, bool isInitial = false, bool isHighRisk = false);
	/* setInfectedPediatricsHIVState sets the pediatrics HIV state and updates statistics */
	void setInfectedPediatricsHIVState(SimContext::PEDS_HIV_STATE hivState, bool isInitial = false);
	/* setInfectedMaternalHIVState sets the maternal HIV state for pediatrics and updates statistics */
	void setInfectedMaternalHIVState(SimContext::PEDS_MOM_HIV_STATE hivState, bool isInitial = false);
	/* setBreastfeedingStatus for pediatrics and updates statistics */
	void setBreastfeedingStatus(SimContext::PEDS_BF_TYPE bfType);
	/* setPediatricsART sets whether or not the infant is on ART */
	void setPediatricsART(bool isOnART);
	/* setDetectedHIVState sets the patients to being detected as HIV positive and updates statistics */
	void setDetectedHIVState(bool isDetected, SimContext::HIV_DET typeDetection = SimContext::HIV_DET_UNDETECTED, SimContext::OI_TYPE oiType = SimContext::OI_NONE);
	/* updateHIVTestingStats updates all statistics after an HIV testing event */
	void updateHIVTestingStats(bool acceptTest, bool returnResults, bool isPositive);
	/* setHIVTestingParams sets the interval and acceptance rate for HIV testing */
	void setHIVTestingParams(int intervalIndex, int acceptanceRateIndex);
	/* scheduleHIVTest sets the month of the next HIV test */
	void scheduleHIVTest(bool hasNext, int monthNum = 0);
	/* scheduleCD4Test sets the month of the next CD4 test */
	void scheduleCD4Test(bool hasNext, int monthNum = 0);
	/* scheduleHVLTest sets the month of the next HVL test */
	void scheduleHVLTest(bool hasNext, int monthNum = 0);
	/* setClinicVisitType sets the conditions for a clinic visit and available treatments */
	void setClinicVisitType(SimContext::CLINIC_VISITS visitType, SimContext::THERAPY_IMPL treatmentType);
	/* setARTResponseBaseline sets the baseline propensity to respond coeffecient */
	void setARTResponseBaseline(double baseline);
	/* setCD4ResponseType sets the predisposed CD4 ART response type of the patient */
	void setCD4ResponseType(SimContext::CD4_RESPONSE_TYPE responseType);
	/* setRiskFactor sets whether or not the patient has risk factor x */
	void setRiskFactor(int riskNum, bool hasRisk, bool isInitial = false);
	/* scheduleInitialClinicVisit sets the month of initial clinic visit, CD4 test, and HVL test */
	void scheduleInitialClinicVisit();
	/* scheduleRegularClinicVisit sets the month of the next regularly scheduled clinic visit */
	void scheduleRegularClinicVisit(bool hasNext, int monthNum = 0);
	/* scheduleEmergencyClinicVisit sets the month of the next emergency clinic visit */
	void scheduleEmergencyClinicVisit(bool hasNext, int monthNum = 0);
	/* resetCliniVisitState resets state keeping track of event since the last clinic visit */
	void resetClinicVisitState(bool isInitial = false);
	/* incrementNumClinicVisits increments the total number of clinic visits */
	void incrementNumClinicVisits();
	/* incrementNumObservedOIs increments the patients observed OIs and statistics */
	void incrementNumObservedOIs(SimContext::OI_TYPE oiType, int numObserved);
	/* setCurrLTFUStats updates the state and statisitics for a patient being LTFU or RTC */
	void setCurrLTFUState(SimContext::LTFU_STATE ltfuState);
	/* startNextARTRegimen updates the state to begin the next ART treatment regimen */
	void startNextARTRegimen();
	/* startNextARTSubRegimen updates the state to begin the next ART treatment subregimen */
	void startNextARTSubRegimen(int nextSubRegimen);
	/* setCurrARTRegimen updates the destined efficacy of the ART regimen */
	void setCurrARTEfficacy(SimContext::ART_EFF_TYPE efficacyType, bool isInitial);
	/* setCurrARTResponse sets the calculated ART propensity to respond and response type */
	void setCurrARTResponse(double propRespond);
	/* setTargetHVLStrata updates the target HVL while on ART or post ART */
	void setTargetHVLStrata(SimContext::HVL_STRATA targetHVL);
	/* setCurrRegimenCD4Slope sets the CD4 slope for the current ART regimen */
	void setCurrRegimenCD4Slope(double cd4Slope);
	/* setCurrRegimenCD4PercentageSlope sets the CD4 percentage slope for the current ART regimen */
	void setCurrRegimenCD4PercentageSlope(double cd4PercSlope);
	/* setCD4EnvelopeRegimen initializes the specified CD4 envelope type */
	void setCD4EnvelopeRegimen(SimContext::ENVL_CD4_TYPE envelopeType, int artLineNum);
	/* setCD4EnvelopeSlope updates the slope used for the specified CD4 envelope type */
	void setCD4EnvelopeSlope(SimContext::ENVL_CD4_TYPE envelopeType, double cd4Slope);
	/* incrementCD4Envelope increments the specified CD4 envelope's level according to hypothetical ART success */
	void incrementCD4Envelope(SimContext::ENVL_CD4_TYPE envelopeType, double changeCD4);
	/* setCurrARTObservedFailue updates the state to begin the next ART treatment regimen */
	void setCurrARTObservedFailure(SimContext::ART_FAIL_TYPE failType);
	/* stopCurrARTRegimen updates the state to begin the next ART treatment regimen */
	void stopCurrARTRegimen(SimContext::ART_STOP_TYPE stopType);
	/* setNextARTRegimen updates the next ART regimen that is available for use */
	void setNextARTRegimen(bool hasNext, int artLineNum = 0);
	/* incrementMonthsUnsuccessfulART increments the number of months on failed/partial ART by HVL */
	void incrementMonthsUnsuccessfulART();
	/* addARTToxicityEffect adds the occurrence of a new toxicity to the active effects list */
	void addARTToxicityEffect(SimContext::ART_TOX_SEVERITY severity, int toxNum, int timeToTox);
	/* removeARTToxicityEffect removes the specified toxicity from the active effects list */
	void removeARTToxicityEffect(list<SimContext::ARTToxicityEffect>::const_iterator &toxIter);
	/* setARTToxicity updates the patient state and stats for the occurrence of a toxicity */
	void setARTToxicity(const SimContext::ARTToxicityEffect &toxEffect);
	/* incrementARTFailedCD4Tests increments the number of failed CD4 tests counting towards ART failure */
	void incrementARTFailedCD4Tests();
	/* resetARTFailedCD4Tests sets the number of failed CD4 tests counting towards ART failure back to 0 */
	void resetARTFailedCD4Tests();
	/* incrementARTFailedHVLTests increments the number of failed CD4 tests counting towards ART failure */
	void incrementARTFailedHVLTests();
	/* resetARTFailedHVLTests sets the number of failed CD4 tests counting towards ART failure back to 0 */
	void resetARTFailedHVLTests();
	/* incrementARTFailedOIs increments the number of observed OIs counting towards ART failure */
	void incrementARTFailedOIs();
	/* resetARTFailedOIs resets the number of observed OIs counting towards ART failure */
	void resetARTFailedOIs();
	/* setCurrSTIState updates the STI state for the current ART regimen */
	void setCurrSTIState(SimContext::STI_STATE newSTIState);
	/* setProphNonCompliance sets whether or not the patient complies with proph */
	void setProphNonCompliance(bool isNonCompliant);
	/* startNextProph updates state to beginning using next proph */
	void startNextProph(SimContext::OI_TYPE oiType);
	/* stopCurrProph updates state to stop using current proph */
	void stopCurrProph(SimContext::OI_TYPE oiType);
	/* setNextProph updates type of proph and proph num to be used for the OI */
	void setNextProph(bool hasNext, SimContext::PROPH_TYPE prophType, SimContext::OI_TYPE oiType, int prophNum);
	/* setProphToxicity records the toxicity and updates stats */
	void setProphToxicity(bool isMajor, SimContext::OI_TYPE oiType);
	/* setProphResistance updates the flag to indicate that proph resistance has occurred */
	void setProphResistance(SimContext::OI_TYPE oiType);
	/* setTBDiseaseState updates the TB disease state */
	void setTBDiseaseState(SimContext::TB_STATE newTBState);
	/* setTBResistanceStrain updates the TB disease drug resistance */
	void setTBResistanceStrain(SimContext::TB_STRAIN newTBStrain);
	/* setNewTBInfection updates the state and statistics for a new TB infections occurring */
	void setNewTBInfection(SimContext::TB_INFECT infectType, bool isActive, int monthsSince = 0);
	/* setTBSpontaneousResolution increments number of TB spontaneous resolutions */
	void setTBSpontaneousResolution();
	/* startNextTBProph updates state to beginning using next TB proph */
	void startNextTBProph();
	/* stopCurrProph updates state to stop using current TB proph */
	void stopCurrTBProph();
	/* setNextTBProph updates proph num to be used next for TB */
	void setNextTBProph(bool hasNext, int prophNum = 0);
	/* scheduleNextTBProph updates the time lag for scheduling the next TB proph */
	void scheduleNextTBProph(int monthStart);
	/* unscheduleNextTBProph removes the next scheduled start of TB proph */
	void unscheduleNextTBProph();
	/* setTBProphToxicity records the TB proph toxicity */
	void setTBProphToxicity(bool isMajor);
	/* startNextTBTreatment updates the patient state and statistics to begin the next TB treatment */
	void startNextTBTreatment();
	/* stopCurrTBTreatment updates the patient state and statistics to stop the current TB treatment */
	void stopCurrTBTreatment(bool isFinished, bool isCured);
	/* scheduleNextTBTreatment updates the patients next scheduled TB treatment and lag time to start */
	void scheduleNextTBTreatment(SimContext::TB_TREATM_STAGE treatStage, int monthStart);
	/* unscheduleNextTBTreatment removes the next scheduled start of TB treatment */
	void unscheduleNextTBTreatment();
	/* increaseTBDrugResistance updates that patient state and stats for an increase in TB drug resistance */
	void increaseTBDrugResistance(bool fromTreatment);
	/* setTBTreatmentToxicity records the TB treatment toxicity */
	void setTBTreatmentToxicity(bool isMajor);
	/* setTrueCHRMsState updates the patients state for occurrence of CHRMs diseases */
	void setTrueCHRMsState(int chrmNum, bool hasCHRM, bool isInitial = false, int monthsStart = 0);
	/* setInitialOIHistory updates the patient state for initial OI history */
	void setInitialOIHistory(bool hasHistory[SimContext::OI_NUM]);
	/* setOIHistory updates the patient state for OI history, called at end of month
		since current acute OI should not count as history until the next month */
	void setOIHistory();
	/* setCurrTrueOI updates the patient state and statistics when an acute OI event occurs or resets back to none */
	void setCurrTrueOI(SimContext::OI_TYPE oiType);
	/* clearMortalityRisks clears the list of possible death risks for the month */
	void clearMortalityRisks();
	/* addMortalityRisk adds a new risk of death for the month */
	void addMortalityRisk(SimContext::DTH_CAUSES causeOfDeath, double probDeath, double costDeath = 0.0);
	/* setCauseOfDeath updates the patient state to reflect that death has occurred */
	void setCauseOfDeath(SimContext::DTH_CAUSES causeOfDeath);
	/* setMaternalDeath updates the patient state for pediatric maternal death */
	void setMaternalDeath();
	/* Increases the total number of persons who have been initialized by one */
	void incrementCohortSize();
	/* updatePopulationStats updates the final population summary after a death occurs */
	void updatePopulationStats();
	/* AddPatientSummary creates a patient summary object and adds it to the patients vector */
	void addPatientSummary();
	/* setTrueCD4 updates the patients actual CD4 level and confines it within the bounds,
		also checks if it is the patients minimum CD4 value */
	void setTrueCD4(double newCD4, bool isInitial = false);
	/* setTrueCD4Percentage updates the pediatrics patients actual CD4 percentage */
	void setTrueCD4Percentage(double newCD4Perc, bool isInitial = false);
	/* setTrueHVLStrata set the patient's actual HVL strata to the given level */
	void setTrueHVLStrata(SimContext::HVL_STRATA newHVL);
	/* setSetpointHVLStrata sets the patients setpoint HVL level */
	void setSetpointHVLStrata(SimContext::HVL_STRATA newSetpoint);
	/* setObservedCD4 updates the patients observed CD4 level and confines it within the bounds */
	void setObservedCD4(bool isKnown, double cd4Value = 0.0);
	/* setObservedCD4Percentage updates the patients observed CD4 percentage and confines it within the bounds */
	void setObservedCD4Percentage(bool isKnown, double cd4Percent = 0.0);
	/* setObservedHVLStrata updates the patients observed HVL strata */
	void setObservedHVLStrata(bool isKnown, SimContext::HVL_STRATA hvlStrata = SimContext::HVL_VLO);
	/* incrementCostsHIVTest adds an HIV testing cost to the patients total */
	void incrementCostsHIVTest(double cost);
	/* incrementCostsHIVMisc adds an HIV misc related cost to the patients total */
	void incrementCostsHIVMisc(double cost);
	/* incrementCostsCD4Test adds the CD4 testing related costs to the patients total */
	void incrementCostsCD4Test(const double *costArray);
	/* incrementCostsHVLTest adds the HVL testing related costs to the patients total */
	void incrementCostsHVLTest(const double *costArray);
	/* incrementCostsClinicVisit adds the general clinic visit costs to the patients total */
	void incrementCostsClinicVisit(const double *costArray);
	/* incrementCostsART adds an ART treatment cost to the patients total */
	void incrementCostsART(int artLineNum, double cost);
	/* incrementCostsProph adds a prophylaxis treatment cost to the patients total */
	void incrementCostsProph(SimContext::OI_TYPE oiType, int prophNum, double cost);
	/* incrementCostsTBProph adds a TB proph cost to patients total */
	void incrementCostsTBProph(int prophNum, double cost);
	/* incrementCostsTBTreatment adds a TB proph cost to patients total */
	void incrementCostsTBTreatment(const double *costArray, double percent);
	/* incrementCostsToxicity adds a toxicity cost to the patients total */
	void incrementCostsToxicity(double cost);
	/* incrementCostsMisc adds a miscellaneous cost to the patients total costs,
		overloaded to take in either a single cost value or a COST_NUM_TYPES sized array of costs */
	void incrementCostsMisc(double cost, double percent);
	void incrementCostsMisc(const double *costArray, double percent);
	/* updateInitialDistributions updates the inital statistics for patients upon infection */
	void updateInitialDistributions();
	/* updatePatientSurvival updates the patient state for discounted LMs and QALMs,
		used with a half month length if death occurred that month */
	void updatePatientSurvival(double percentOfMonth);
	/* updateOverallSurvival updates the statistics for stratified LMs and QALMs,
		used with a half month length if death occurred that month */
	void updateOverallSurvival(double percentOfMonth);
	/* updateLongitSurvival updates the longitudinal statistics related to survival */
	void updateLongitSurvival();
	/* updateARTEfficacyStats updates the totals for months in ART suppression and HVL drops */
	void updateARTEfficacyStats();
	/* updateOIHistoryLogging updates the statistics for patient OI history */
	void updateOIHistoryLogging();

	/* Protected utility functions that are used by multiple state updater child classes */
	/* willAttendClinicThisMonth returns true if patient will go in for a scheduled clinic
		visit or OI emergency clinic visit this month */
	bool willAttendClinicThisMonth();
	/* getPartialSuppressTargetHVL determines the ART target HVL when entering partial suppression */
	SimContext::HVL_STRATA getPartialSuppressTargetHVL(int artLineNum);

private:
	/* Private utility functions that are used by multiple updater functions */
	/* getCD4Strata returns the CD4 strata for a given value */
	SimContext::CD4_STRATA getCD4Strata(double valueCD4);
	/* getCD4PercentageStrata returns the CD4 percentage strata for a given value */
	SimContext::PEDS_CD4_PERC getCD4PercentageStrata(double percCD4);
	/* getAgeCategoryClinical returns the clinic visit age category for the given age */
	int getAgeCategoryClinical(int ageMonths);
	/* getAgeCategoryHIVInfection returns the HIV testing age category for the given age */
	int getAgeCategoryHIVInfection(int ageMonths);
	/* getAgeCategoryPediatrics returns the Pediatrics testing category for the given age */
	SimContext::PEDS_AGE_CAT getAgeCategoryPediatrics(int ageMonths);
	/* getTimeSummary returns a non-const pointer to the TimeSummary object for the current time period,
		creates a new one if needed or returns null if not keeping longitudinal stats */
	RunStats::TimeSummary *getTimeSummaryForUpdate();
	/* incrementCostsCommon increases all general cost stats that are independent of the type of cost,
		overloaded to take in either a single cost value or a COST_NUM_TYPES sized array of costs */
	void incrementCostsCommon(double cost, double percent);
	void incrementCostsCommon(const double *costArray, double percent);
};
