#include "include.h"

// Disable warning for unsafe use of this pointer in initialization,
//	state updater constructor only copies the pointer and does not access any of its fields so it is safe
#pragma warning(disable:4355)

/* Constructor takes in the patient number, simulation context, run stats object, and tracing object
	Initializes all subclass state values */
Patient::Patient(SimContext *simContext, RunStats *runStats, Tracer *tracer,  bool _predefinedAgeAndGender, int _ageMonths, SimContext::GENDER_TYPE _gender, bool _setAsIncidentCase, int startingMonth) :
		simContext(simContext),
		runStats(runStats),
		tracer(tracer),
		beginMonthUpdater(this),
		hivInfectionUpdater(this),
		chrmsUpdater(this),
		drugToxicityUpdater(this),
		tbDiseaseUpdater(this),
		acuteOIUpdater(this),
		mortalityUpdater(this),
		cd4HVLUpdater(this),
		hivTestingUpdater(this),
		behaviorUpdater(this),
		drugEfficacyUpdater(this),
		cd4TestUpdater(this),
		hvlTestUpdater(this),
		clinicVisitUpdater(this),
		endMonthUpdater(this)
{
	//Determine if Age and Gender and Incident Case status are input defined rather than drawn from a distribution
	this->generalState.predefinedAgeAndGender = _predefinedAgeAndGender;
	if (this->generalState.predefinedAgeAndGender){
		this->generalState.ageMonths = _ageMonths;
		this->generalState.gender = _gender;
		this->diseaseState.isPrevalentHIVCase = !(_setAsIncidentCase);
	}

	//Set the initial time
	this->generalState.initialMonthNum = startingMonth;

	beginMonthUpdater.performInitialUpdates();
	hivInfectionUpdater.performInitialUpdates();
	chrmsUpdater.performInitialUpdates();
	drugToxicityUpdater.performInitialUpdates();
	tbDiseaseUpdater.performInitialUpdates();
	acuteOIUpdater.performInitialUpdates();
	mortalityUpdater.performInitialUpdates();
	cd4HVLUpdater.performInitialUpdates();
	hivTestingUpdater.performInitialUpdates();
	behaviorUpdater.performInitialUpdates();
	drugEfficacyUpdater.performInitialUpdates();
	cd4TestUpdater.performInitialUpdates();
	hvlTestUpdater.performInitialUpdates();
	clinicVisitUpdater.performInitialUpdates();
	endMonthUpdater.performInitialUpdates();
}

/* Cleanup the state updater classes */
Patient::~Patient(void) {

}

/* simulateMonth runs a single month of simulation for this patient, and updates
	its state and runStats statistics */
void Patient::simulateMonth() {
	beginMonthUpdater.performMonthlyUpdates();

	/* Disease and General Health updaters */
	//If this is being run in the transmission model (i.e. age and gender were predefined) ignore all incidence (i.e. do not perform hivInfection updates until after infection)
	if (!(generalState.predefinedAgeAndGender && diseaseState.infectedHIVState == SimContext::HIV_INF_NEG)){
		hivInfectionUpdater.performMonthlyUpdates();
	}
	chrmsUpdater.performMonthlyUpdates();
	if (diseaseState.infectedHIVState != SimContext::HIV_INF_NEG) {
		drugToxicityUpdater.performMonthlyUpdates();
		tbDiseaseUpdater.performMonthlyUpdates();
		// Roll for occurrence of an acute OI if acute TB did not happen this month
		if (!diseaseState.hasCurrTrueOI) {
			acuteOIUpdater.performMonthlyUpdates();
		}
	}
	mortalityUpdater.performMonthlyUpdates();
	if (!diseaseState.isAlive) {
		endMonthUpdater.performMonthlyUpdates();
		return;
	}
	if (diseaseState.infectedHIVState != SimContext::HIV_INF_NEG) {
		cd4HVLUpdater.performMonthlyUpdates();
	}

	/* Treatment, Monitoring, and Behavior updaters */
	hivTestingUpdater.performMonthlyUpdates();
	if (diseaseState.infectedHIVState != SimContext::HIV_INF_NEG) {
		behaviorUpdater.performMonthlyUpdates();
		drugEfficacyUpdater.performMonthlyUpdates();
		cd4TestUpdater.performMonthlyUpdates();
		hvlTestUpdater.performMonthlyUpdates();
		clinicVisitUpdater.performMonthlyUpdates();
	}

	endMonthUpdater.performMonthlyUpdates();
} /* endSimulateMonth */

/* saveState saves the state of the patient to file format using a JSON like notation */
void Patient::saveState(ostream & _outStream){
	//general state
	_outStream << "patient:{" << endl;
	_outStream << "mth:" << this->generalState.monthNum << "," << endl;
	_outStream << "initMth:" << this->generalState.initialMonthNum << "," << endl;
	_outStream << "discCost:" << this->generalState.costsDiscounted << "," << endl;
	_outStream << "discLM:" << this->generalState.LMsDiscounted << "," << endl;
	_outStream << "discQALM:" << this->generalState.qualityAdjustLMsDiscounted << "," << endl;

	//disease state
	int mthAcuToChr=-1,mthInf=-1,trueCD4=-1,minCD4=-1,trueHVL=-1,setHVL=-1,tarHVL=-1;
	if(this->diseaseState.infectedHIVState!=SimContext::HIV_INF_NEG){
		mthAcuToChr=this->diseaseState.monthOfAcuteToChronicHIV;
		mthInf=this->diseaseState.monthOfHIVInfection;
		trueCD4=this->diseaseState.currTrueCD4;
		minCD4=this->diseaseState.minTrueCD4;
		trueHVL=this->diseaseState.currTrueHVLStrata;
		setHVL=this->diseaseState.setpointHVLStrata;
		tarHVL=this->diseaseState.targetHVLStrata;
	}
	_outStream << "hivState:" << this->diseaseState.infectedHIVState << "," << endl;
	_outStream << "mthInf:" << mthInf << "," << endl;
	_outStream << "mthAcuToChr:" << mthAcuToChr << "," << endl;
	_outStream << "trueCD4:" << trueCD4 << "," << endl;
	_outStream << "minCD4:" << minCD4 << "," << endl;
	_outStream << "trueHVL:" << trueHVL << "," << endl;
	_outStream << "setHVL:" << setHVL << "," << endl;
	_outStream << "tarHVL:" << tarHVL << "," << endl;
	_outStream << "hasOI:" << this->diseaseState.hasCurrTrueOI << "," << endl;
	if (this->diseaseState.hasCurrTrueOI)
		_outStream << "typeOI:" << this->diseaseState.typeCurrTrueOI << "," << endl;
	_outStream << "typeOIhist:" << this->diseaseState.typeTrueOIHistory << "," << endl;
	
	bool isFirst=true;
	_outStream << "hasOIhist:[";
	for (int i=0; i<SimContext::OI_NUM; i++){
		if (!isFirst)
			_outStream << ",";
		isFirst=false;
		_outStream << this->diseaseState.hasTrueOIHistory[i];
	}
	_outStream << "]" << "," << endl;

	isFirst=true;
	_outStream << "numOILastVst:[";
	for (int i=0; i<SimContext::OI_NUM; i++){
		if (!isFirst)
			_outStream << ",";
		isFirst=false;
		_outStream << this->diseaseState.numTrueOIsSinceLastVisit[i];
	}
	_outStream << "]" << "," << endl;

	isFirst=true;
	_outStream << "mortRisks:[";
	for (vector<SimContext::MortalityRisk>::iterator it=this->diseaseState.mortalityRisks.begin(); it!=this->diseaseState.mortalityRisks.end(); it++){
		if (!isFirst)
			_outStream << ",";
		isFirst=false;
		(*it).saveState(_outStream);
	}
	_outStream << "]}";
}
