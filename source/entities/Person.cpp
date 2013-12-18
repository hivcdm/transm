#include "Person.h"
#include "Male.h"
#include "Female.h"
#include "classifiers/SexualPartnership.h"
#include "../Constants.h"
#include "../data/EventParams.h"
#include "../util/Util.h"
#include "../util/rand/RandomNums.h"
#include "../statistics/InfectionsTracker.h"
#include "../statistics/ArtTestingTracker.h"

class EntityPool;

long Person::idCounter = 0;
int Person::numTracesSoFar = 0;

//This is pretty much only used by the NA folks who are NA at the end of the model and need to have their LMs added to total
//TODO: But maybe they shouldn't?
vector<double> Person::probDeathNatCauses[DmgProfile::ENDGender];

const std::string Person::StatsStr[Person::STAT_ENDStats] = {
		"TOTAL_LM",
		"HIV_NEG_LM_INSIM",
		"HIV_POS_POSTINFECT_LM_INSIM",
		"EXPOSURES_BEFORE_INF",
		"NUM_INFECTED",
		"AGE_AT_INFECTION_MTH",
		"TIME_OF_INFECTION_MTH",
		"GENERATION_OF_INFECTION"
};

EnumCls<Person::Stats> Person::StatsEnum(Person::StatsStr, Person::STAT_ENDStats);

//----------------< Start Methods for Person >-------------------//
//All implemented methods are in alphabetical order execept for the constructors/destructors (at bottom of Person section)
//The virtual methods are implemeted by Male and Female and are the very bottom fo the file


void Person::ageOneTimeUnit() {
	age++;
}

Person* Person::allPartnerSexualActivity(EventParams& _eventParams,SexualPartnership::Type _partnershipType, list<Person*> &_newlyInfected, InfectionsTracker *infTrack) {
	assert( _partnershipType < SexualPartnership::ENDType);

  //iterate through all partnerships of SexualActivity::Type _partnershipType and have them engage in sexual activity
	list<SexualPartnership*>::iterator iter = this->partners[_partnershipType].begin();
	list<SexualPartnership*>::iterator iterEnd = this->partners[_partnershipType].end();
	//becomes non-NULL only when this person gets infected. We are saving the partner who infected this person
	Person *infectedMe = NULL;

	while(iter != iterEnd) {
	        //initiate sexual activity only if you are partner1
		if((*iter)->getPartner1() == this) {
			Person *infected = (*iter)->monthlySexualActivity(_eventParams, infTrack);
			//if you or your partners got infected, the infected joins the _newlyInfected list
			if(infected != NULL) {
				_newlyInfected.push_back(infected);
				//if you got infected, then you have to save the person who infected you for record keeping
				if(infected == this)
					infectedMe = (*iter)->getOtherPartner(this);
			} //if(infected != NULL) {
		}
		iter++;
	} //while(iter != iterEnd) {

	return infectedMe;
}

bool Person::availableForPartnership(SexualPartnership::Type _partnershipType) const {
  if( _partnershipType == SexualPartnership::STEADY) {
    return (this->partners[ SexualPartnership::STEADY].empty());
  } else
    return true;
}

void Person::addPartnership(SexualPartnership *_partnership){
	assert( _partnership != NULL);
	assert( (_partnership->getPartner1() != NULL) );
	assert(_partnership->getPartner1()->isAlive());
	assert( (_partnership->getPartner2() != NULL) );
	assert((_partnership->getPartner2()->isAlive()));

	this->partners[_partnership->getType()].push_back(_partnership);
	this->numPartnersInHistory[_partnership->getType()]++;
	this->monthOfLatestPartnershipDissolution[_partnership->getType()]=max(this->monthOfLatestPartnershipDissolution[_partnership->getType()],_partnership->getDissolutionTime());

	//if a STEADY partnership was added && we are SINGLE, the we need to change or RELATIONSHIP_STATUS
	if( (_partnership->getType() == SexualPartnership::STEADY) &&
		(!this->partners[SexualPartnership::STEADY].empty()) &&
		(this->getDmgProfileVal(DmgProfile::RELATIONSHIP_STATUS) == DmgProfile::SINGLE)) {
			this->dmgProfile.set(DmgProfile::RELATIONSHIP_STATUS, DmgProfile::NON_SINGLE);
	}
}

void Person::becomeInfected(int _generationOfInfection, EventParams& _eventParams) {
	this->hvl = HVL_PRIMARY;	//hvl needs to be set even for people who are about to go through CEPAC so that this->isInfected() correctly returns true
	this->cd4 = -1;			//CD4 doesn't affect much in the transmission model yet... will be updated with CEPAC
	this->ageInfected = this->age;
	this->generationOfInfection = _generationOfInfection;

	if (_eventParams.outputTrace[EventParams::SINGLEPERSON] && this->trace()){
		if (this->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE){
			_eventParams.traceStreams[EventParams::SINGLEPERSON] << "@ Male ";
		}
		else {
			_eventParams.traceStreams[EventParams::SINGLEPERSON] << "@ Female ";
		}
		_eventParams.traceStreams[EventParams::SINGLEPERSON] << this->getID() << " has ";
		if (_generationOfInfection == 0){
			_eventParams.traceStreams[EventParams::SINGLEPERSON] << "a prevalent case of HIV";
		}
		else{
			_eventParams.traceStreams[EventParams::SINGLEPERSON] << "an incident case of HIV";
		}
		_eventParams.traceStreams[EventParams::SINGLEPERSON] << "!" << endl;
	}


	this->stats.setStat(Person::STAT_TIME_OF_INFECTION_MTH, _eventParams.currTime);
	this->stats.setStat(Person::STAT_AGE_AT_INFECTION_MTH, this->getAge(MONTH));
	this->stats.setStat(Person::STAT_GENERATION_OF_INFECTION, _generationOfInfection);

	//if a CEPAC person exists (i.e. they were created earlier and thus this is an incident case), set them to infected
	if (this->wentThroughCEPAC){
		this->cepacPatient->forceNewInfection();
		this->cd4 = this->cepacPatient->getDiseaseState()->currTrueCD4;

		//update HVL state
		SimContext::HVL_STRATA hvlStrata = this->cepacPatient->getDiseaseState()->currTrueHVLStrata;
		if (hvlStrata == SimContext::HVL_VLO)
			this->hvl = HVL_ZERO;		//0-20
		else if (hvlStrata == SimContext::HVL__LO)
			this->hvl = HVL_ONE;		//21-500
		else if (hvlStrata == SimContext::HVL_MLO)
			this->hvl = HVL_TWO;		//501-3000
		else if (hvlStrata == SimContext::HVL_MED)
			this->hvl = HVL_THREE;		//3001-10000
		else if (hvlStrata == SimContext::HVL_MHI)
			this->hvl = HVL_FOUR;		//10001-30000
		else if (hvlStrata == SimContext::HVL__HI)
			this->hvl = HVL_FIVE;		//30001-100000
		else if (hvlStrata == SimContext::HVL_VHI)
			this->hvl = HVL_SIX;		//100000+
		else{
			cerr << "Invalid CEPAC API infection state: " << *(SimContext::HVL_STRATA_STRS[hvlStrata]);
			Util::exitWithPrompt(-1);
		}

		this->currentTrueHvl=this->hvl;

		if (this->cepacPatient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_ACUTE_SYN){
			this->hvl = HVL_PRIMARY;
			if (this->cepacPatient->getMonitoringState()->isDetectedHIVPositive)
				this->hivStatus = OBSERVED_ACUTE;
			else
				this->hivStatus = UNOBSERVED_ACUTE;
		}
		//Late stage is defined as having failed the last ART regimen (or having no art regimens to start with) and a CD4 <= 50
		else if ((!(this->cepacPatient->getARTState()->hasNextRegimenAvailable) &&
				(!(this->cepacPatient->getARTState()->isOnART) || this->cepacPatient->getARTState()->hasObservedFailure)) &&
				this->cepacPatient->getDiseaseState()->currTrueCD4 <= 50){
			this->hvl = HVL_LATESTAGE;
			if (this->cepacPatient->getMonitoringState()->isDetectedHIVPositive)
				this->hivStatus = OBSERVED_LATESTAGE;
			else
				this->hivStatus = UNOBSERVED_LATESTAGE;
		}
		else{
			if (this->cepacPatient->getMonitoringState()->isDetectedHIVPositive)
				this->hivStatus = OBSERVED_CHRONIC;
			else
				this->hivStatus = UNOBSERVED_CHRONIC;
		}

		//Update OI History
		for (int i=0;i<Constants::NUMBER_OF_OIS;i++){
			this->oiHistory[i]=this->cepacPatient->getDiseaseState()->hasTrueOIHistory[i];
		}
	} else {
		//Infection of prevalent cases happens when CEPAC person is initialized
		this->initialCEPACpatient(_eventParams);
	}

	/** Update the graphNode to have value for time of infection */
	this->graphNode->timeInfected = _eventParams.currTime;
}

void Person::initialCEPACpatient(EventParams& _eventParams){
	//Only initialize the person if they haven't already been initialized!  (Prevalent cases will get called to initialize twice!)
	if (!this->wentThroughCEPAC){

		this->wentThroughCEPAC = true;

		//determine if prevalent or incident case
		//Prevalent cases will be set to be infected prior to initialization
		//"Incident" cases are not yet infected and will be initialized later.
		bool setAsIncidentCase = !this->isInfected();

		//initial CEPAC patient for this person
		SimContext::GENDER_TYPE cepacGender = SimContext::GENDER_FEMALE;
		if (this->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE)
			cepacGender = SimContext::GENDER_MALE;
		//MULTIRUN
		/*if (_eventParams.currTime > 20){
			this->cepacPatient = new Patient(_eventParams.cepacSimContext2, _eventParams.cepacRunStats, _eventParams.cepacTracer,
					true, this->getAge(MONTH), cepacGender, (_generationOfInfection > 0));
		} else {*/
		//TODO: Switch to multiple input sheets!
		SimContext * simContextToUse;
		if (_eventParams.useRollout){
			//When patients are initialized they are added to the untreated pool
			simContextToUse=_eventParams.untreatedContext;
		}
		else{
			simContextToUse=_eventParams.cepacSimContexts[this->getCEPACSimContextIndex(_eventParams)];
		}
		this->cepacPatient = new Patient(simContextToUse, _eventParams.cepacRunStats, _eventParams.cepacTracer,
					true, this->getAge(MONTH), cepacGender, setAsIncidentCase, _eventParams.currTime);
		//}
		//Only update hvl and cd4 if the patient is infected
		//update HVL and CD4  and infection status for this Person if they are infected
		if (this->isInfected()){
			this->cd4 = this->cepacPatient->getDiseaseState()->currTrueCD4;

			//update HVL state
			SimContext::HVL_STRATA hvlStrata = this->cepacPatient->getDiseaseState()->currTrueHVLStrata;
			if (hvlStrata == SimContext::HVL_VLO)
				this->hvl = HVL_ZERO;		//0-20
			else if (hvlStrata == SimContext::HVL__LO)
				this->hvl = HVL_ONE;		//21-500
			else if (hvlStrata == SimContext::HVL_MLO)
				this->hvl = HVL_TWO;		//501-3000
			else if (hvlStrata == SimContext::HVL_MED)
				this->hvl = HVL_THREE;		//3001-10000
			else if (hvlStrata == SimContext::HVL_MHI)
				this->hvl = HVL_FOUR;		//10001-30000
			else if (hvlStrata == SimContext::HVL__HI)
				this->hvl = HVL_FIVE;		//30001-100000
			else if (hvlStrata == SimContext::HVL_VHI)
				this->hvl = HVL_SIX;		//100000+
			else{
				cerr << "Invalid CEPAC API infection state: " << *(SimContext::HVL_STRATA_STRS[hvlStrata]);
				Util::exitWithPrompt(-1);
			}

			this->currentTrueHvl=this->hvl;

			if (this->cepacPatient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_ACUTE_SYN){
				this->hvl = HVL_PRIMARY;
				if (this->cepacPatient->getMonitoringState()->isDetectedHIVPositive)
					this->hivStatus = OBSERVED_ACUTE;
				else
					this->hivStatus = UNOBSERVED_ACUTE;
			}
			//Late stage is defined as having failed the last ART regimen (or having no art regimens to start with) and a CD4 <= 50
			else if ((!(this->cepacPatient->getARTState()->hasNextRegimenAvailable) &&
					(!(this->cepacPatient->getARTState()->isOnART) || this->cepacPatient->getARTState()->hasObservedFailure)) &&
					this->cepacPatient->getDiseaseState()->currTrueCD4 <= 50){
				this->hvl = HVL_LATESTAGE;
				if (this->cepacPatient->getMonitoringState()->isDetectedHIVPositive)
					this->hivStatus = OBSERVED_LATESTAGE;
				else
					this->hivStatus = UNOBSERVED_LATESTAGE;
			}
			else{
				if (this->cepacPatient->getMonitoringState()->isDetectedHIVPositive)
					this->hivStatus = OBSERVED_CHRONIC;
				else
					this->hivStatus = UNOBSERVED_CHRONIC;
			}

			//Update OI History
			for (int i=0;i<Constants::NUMBER_OF_OIS;i++){
				this->oiHistory[i]=this->cepacPatient->getDiseaseState()->hasTrueOIHistory[i];
			}

		}//if (isInfected)

		Person::numTracesSoFar++;
	}
}

int Person::getGenerationOfInfection(){
	//TODO: Make this a constant!
	if (this->generationOfInfection > 5){
		return 5;
	} else {
		return this->generationOfInfection;
	}
}

int Person::getNumPartners(SexualPartnership::Type _type){
	return this->partners[_type].size();
}

int Person::getNumPartners(SexualPartnership::Type _type, bool sameRisk){
	int numPartners = 0;
	for (list<SexualPartnership*>::iterator partnerIter = this->partners[_type].begin(); partnerIter != this->partners[_type].end(); partnerIter++){
		Person * partner;
		if((*partnerIter)->getPartner1() == this) {
			partner = (*partnerIter)->getPartner2();
		}
		else{
			partner = (*partnerIter)->getPartner1();
		}
		bool isSameRisk = this->risk==partner->getRiskLevel();
		if (isSameRisk == sameRisk){
			numPartners+=1;
		}
	}
	
	return numPartners;
}

int Person::getNumPartnersInHistory(SexualPartnership::Type _type){
	return this->numPartnersInHistory[_type];
}
int Person::getNumPartnersInHistory(){
	int total = 0;
	for (int i = 0; i < SexualPartnership::ENDType; i++){
		total+= this->numPartnersInHistory[i];
	}
	return total;
}
int Person::getMonthOfLatestPartnershipDissolution(SexualPartnership::Type _type){
	return this->monthOfLatestPartnershipDissolution[_type];
}
int Person::getMonthOfLatestConcurrent(){
	return this->monthOfLatestConcurrent;
}
void Person::setMonthOfLatestConcurrent(int _month){
	this->monthOfLatestConcurrent = _month;
}
void Person::becomeSexuallyActive(EventParams& _eventParams) {
	this->dmgProfile.set(DmgProfile::SEXUAL_ACTIVITY_STATUS, DmgProfile::SA);
	//CEPAC person needs to be initialized
	this->initialCEPACpatient(_eventParams);
	//Pass the info the personNode
	this->graphNode->timeSA = _eventParams.currTime;

	if (_eventParams.outputTrace[EventParams::SINGLEPERSON] && this->trace()){
		if (this->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE){
			_eventParams.traceStreams[EventParams::SINGLEPERSON] << " % Male ";
		}
		else {
			_eventParams.traceStreams[EventParams::SINGLEPERSON] << " % Female ";
		}
		_eventParams.traceStreams[EventParams::SINGLEPERSON] << this->getID() << " becomes sexually active" << endl;
	}
}

Person* Person::fling(Person *_p, SexualPartnership::Type _partnershipType, EventParams& _eventParams, InfectionsTracker *infTrack) {
	assert( (_p!=NULL) );
	assert(_p->isAlive());
	assert( _partnershipType < SexualPartnership::ENDType);

	int numActs = this->rollNumEventsPerPartner(_p, _eventParams.randomNums, _partnershipType);

	return this->sexualActivity(_p, numActs, _partnershipType, _eventParams, infTrack);
}

int Person::getAge(TimeGranularity _granularity) const{
	assert( _granularity < ENDTimeGranularity);
	if (_granularity == MONTH)
		return this->age;
	else
		return Util::convertTime(MONTH,_granularity,this->age);
}


DmgProfile::ProfileID Person::getCurrBucketProfileID() {
	return this->currentBucketID;
}

const DmgProfile * Person::getDmgProfile() const{
	return &this->dmgProfile;
}

BaseEnumCls::Enum Person::getDmgProfileVal(DmgProfile::Demographic _demographic) const {
	return this->dmgProfile.get(_demographic);
}

unsigned long Person::getID(){
	return this->id;
}

long Person::getPartnershipsToEnd(long _currTime, SexualPartnership::Type _partnershipType, list<SexualPartnership*> &_partnershipsToEnd, bool _fromDeath) {
	assert( _partnershipType < SexualPartnership::ENDType);
	assert ((_currTime >= 0) || _fromDeath);

	if( this->partners[_partnershipType].size() == 0)
		return 0;

	//iterate through all current partnerships that had any duration to them.
	//The iterator points to class SexualPartnership
	list<SexualPartnership*>::iterator iter = this->partners[_partnershipType].begin();
	list<SexualPartnership*>::iterator iterEnd = this->partners[_partnershipType].end();

	long numEnded = 0;
	//go through all partnerships
	while(iter != iterEnd) {
	  //if it's time for that partnership to end, then put that partnership is the list for deletion
	  if( (*iter)->checkTimeForSplit(_currTime) || _fromDeath) {
		 _partnershipsToEnd.push_back(*iter);
		numEnded++;
	  }

	iter++;

	} //	while(iter != iterEnd) {

	return numEnded;
}

//Begin Unformed Partnership helper methods
int Person::getTotalUnformedPartnerships(SexualPartnership::Type type){
	return this->unformedPartnershipsTotal[type];
}

int Person::getLatestUnformedPartnerships(SexualPartnership::Type type){
	return this->unformedPartnershipsLatestTime[type];
}

void Person::increaseUnformedPartnershipTallies(SexualPartnership::Type type){
	this->unformedPartnershipsLatestTime[type] += 1;
	this->unformedPartnershipsTotal[type] += 1;
}

void Person::resetLatestUnformedPartnerships(SexualPartnership::Type type){
	this->unformedPartnershipsLatestTime[type] = 0;
}

//End Unformed Partnership helper methods

unsigned int Person::getPopulationID() {
	return this->populationID;
}

//returns traceMe
bool Person::trace(){
	return this->traceMe;
}
//sets traceMe to true
void Person::setToBeTraced(){
	this->traceMe = true;
}

const Person::StatsRecord* Person::getStats() {
	return &this->stats;
}

bool Person::inCorrectDmgProfileBucket() {
	return (this->dmgProfile.getProfileID() == this->currentBucketID);
}

bool Person::isAlive() const{
	if (this == NULL){
		return false;
	}
	return !this->death;
}

bool Person::isPartneredWith(Person *_p) {
	assert( (_p!=NULL) );
	assert(_p->isAlive());

	for(SexualPartnership::Type partnershipType = SexualPartnership::Type(0); partnershipType < SexualPartnership::ENDType; ++partnershipType) {
	  //iterate through each partnership and check if _p is a member of one of them
		list<SexualPartnership*>::iterator iter = this->partners[partnershipType].begin();
		list<SexualPartnership*>::iterator endIter = this->partners[partnershipType].end();
	  while(iter != endIter) {
	    if((*iter)->isMember(_p)) return true;
	    iter++;
	  }
	}

	return false;
}

bool Person::hasPartnership(SexualPartnership::Type partnershipType){
	if(this->partners[partnershipType].size()>0){
		return true;
	}

	return false;

}
void Person::print(ostream& _outStream, string _prefix) const{

	_outStream << _prefix << endl;
	_outStream << ((this->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE) ? "Male" : "Female") << Constants::TAB;
	_outStream << "ID: " << id << Constants::TAB;

	_outStream << "(";
	this->getDmgProfile()->print(_outStream,"");
	_outStream << ")";

	_outStream <<  Constants::TAB << "Age(mos.): " << this->getAge(MONTH);
	_outStream <<  Constants::TAB << "CD4: " << cd4;
	_outStream << Constants::TAB << "HVL: " << hvl;
	_outStream << Constants::TAB << "Risk: " << ((this->risk == Person::HIGH) ? "HIGH" : "LOW");
	_outStream << Constants::TAB << "Marbles: " << activityLevel;
	_outStream << endl;
}

void Person::printCurrentPartners(ostream& _outStream, string _prefix) {

	for(SexualPartnership::Type type = SexualPartnership::Type(0); type < SexualPartnership::ENDType; ++type) {
		list<SexualPartnership*>::iterator iter = this->partners[type].begin();
		list<SexualPartnership*>::iterator iterEnd = this->partners[type].end();

	  if(iter != iterEnd) {
	    _outStream << *(SexualPartnership::TypeEnum.toString(type)) << Constants::COLON << endl;
	  }

	  while(iter != iterEnd ) {
	    Person *partner = (*iter)->getOtherPartner(this);
	    partner->print(_outStream, "\t\t");
	    iter++;
	  }
	} //	for(SexualPartnership::Type type = SexualPartnership::Type(0); type < SexualPartnership::ENDType; ++type) {
}
/**
*This function saves the state of the patient to file
*Uses Json like notation
*/
void Person::saveState(ostream & _outStream, long currTime) {
	_outStream << "id:" << this->id << "," << endl; //id
	this->dmgProfile.saveState(_outStream); //dmg profile
	_outStream << "curBktID:" << this->currentBucketID << "," << endl; //bucket id (contains same information as dmgprofile)

	//save all the sexual partnerships
	_outStream << "partners:[";
	bool firstPartner = true;
	for(int i = 0; i < SexualPartnership::ENDType; i++){ //loop through partnership types
		for(list<SexualPartnership*>::iterator it = this->partners[i].begin(); it != this->partners[i].end(); it++){ //loop through all partners
			if(!firstPartner)
				_outStream << ",";
			firstPartner = false;
			(*it)->saveState(_outStream, this->id, currTime);
		}
	}
	_outStream << "]," << endl;

	_outStream << "partnerHist:[";
	firstPartner = true;
	for(int i = 0; i < SexualPartnership::ENDType; i++){ //loop through partnership types
		if(!firstPartner)
			_outStream << ",";
		firstPartner = false;
		_outStream << this->numPartnersInHistory[i];
	}
	_outStream << "]," << endl;

	_outStream << "genInf:" << this->generationOfInfection << "," << endl; //generation of infection
	_outStream << "risk:" << this->risk << "," << endl; //risk Level
	_outStream << "activity:" << this->activityLevel << "," << endl; //activity Level
	_outStream << "age:" << this->age << "," << endl; //age
	_outStream << "initAge:" << this->initAge << "," << endl; //initial age
	_outStream << "dead:" << this->death << "," << endl; //death
	_outStream << "hvl:" << this->hvl; //hvl in transmission includes primary and late stage

	//patient data
	//	if (this->wentThroughCEPAC){
	//		_outStream << "," << endl;
	//		this->cepacPatient->saveState(_outStream);
	//}
	/*
	//save the full vector indices of person
	_outStream << "fvInd:[";
	bool firstFV=true;
	for (map<FullVector*, vector<unsigned int> >::iterator it=this->FVindices.begin(); it != this->FVindices.end(); it++){ //loop over all Full Vectors
	if (!firstFV)
	_outStream << ",";
	firstFV=false;
	//full vector id and indices of person in fv
	_outStream << "{" << "fvID:" << (*it).first->getID() << ",ind:[";
	bool firstIndex=true;
	vector <unsigned int> * indicesPtr=&(*it).second;
	for (vector <unsigned int>::iterator indIter=indicesPtr->begin(); indIter != indicesPtr->end(); indIter++){
	if (!firstIndex)
	_outStream << ",";
	firstIndex=false;
	_outStream << *indIter;
	}
	_outStream << "]}";
	}
	_outStream << "]";
	*/
}
bool Person::isInfected() {
	return (this->hvl > UNINFECTED);
}

void Person::removePartnership(SexualPartnership *_partnership){
	assert( _partnership != NULL);

	this->partners[_partnership->getType()].remove(_partnership);

	//if a STEADY partnership was removed and we have no more, then we should be set to SINGLE
	if( (_partnership->getType() == SexualPartnership::STEADY) &&
		(this->partners[SexualPartnership::STEADY].empty()) &&
		(this->getDmgProfileVal(DmgProfile::RELATIONSHIP_STATUS) == DmgProfile::NON_SINGLE)) {
			this->dmgProfile.set(DmgProfile::RELATIONSHIP_STATUS, DmgProfile::SINGLE);
	}
}

void Person::rollForBecomeSexWorker(EventParams& _eventParams, bool _isInit, double initialProb) {
	//see whether this person will become a CSW when they make their sexual debut
	double currGenderChanceBecomeCSW;
	if (_isInit)
		currGenderChanceBecomeCSW = initialProb;
	else
		currGenderChanceBecomeCSW = (this->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE) ? Male::getPopParams(this->populationID)->getChanceBecomeCSW() : Female::getPopParams(this->populationID)->getChanceBecomeCSW();
	
	if( _eventParams.randomNums.chance(currGenderChanceBecomeCSW)){
		if (_eventParams.outputTrace[EventParams::SINGLEPERSON] && this->trace()){
			if (this->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE){
				_eventParams.traceStreams[EventParams::SINGLEPERSON] << " % Male ";
			}
			else {
				_eventParams.traceStreams[EventParams::SINGLEPERSON] << " % Female ";
			}
			_eventParams.traceStreams[EventParams::SINGLEPERSON] << this->getID() << " becomes CSW" << endl;
		}
		this->dmgProfile.set(DmgProfile::EMPLOYMENT, DmgProfile::CSW);
	}
}

void Person::quitSexWork(EventParams& _eventParams){
	if (_eventParams.outputTrace[EventParams::SINGLEPERSON] && this->trace()){
		if (this->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE){
			_eventParams.traceStreams[EventParams::SINGLEPERSON] << " % Male ";
		}
		else {
			_eventParams.traceStreams[EventParams::SINGLEPERSON] << " % Female ";
		}
		_eventParams.traceStreams[EventParams::SINGLEPERSON] << this->getID() << " quits being CSW" << endl;
	}
	this->dmgProfile.set(DmgProfile::EMPLOYMENT, DmgProfile::NON_CSW);
}

void Person::rerollRiskGroup(EventParams& _eventParams){
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	cerr << "Person::rerollRiskGroup()" << endl;
}

//determine whether this person died
bool Person::rollForDeath(RandomNums& _randomNums) {

	assert(this->death == false);

	//if person is too old, then they automatically die
	if(this->getAge(MONTH) >= (12 * Person::maxYrForDeathStats)) {
		this->death = true;
		this->deathStatus = DTH_OTHER;
	}
	//if they have gone through CEPAC
	else if(this->wentThroughCEPAC) {
		//CEPAC determines month of death
		//check if cepacPatient is dead
		if (!this->cepacPatient->isAlive()){
			this->death = true;
			SimContext::DTH_CAUSES causeOfDeath = this->cepacPatient->getDiseaseState()->causeOfDeath;
			if (causeOfDeath < SimContext::DTH_CHRAIDS)
				this->deathStatus = DTH_OI;	//oi death
			else if (causeOfDeath == SimContext::DTH_CHRAIDS)
				this->deathStatus = DTH_CHRAIDS;
			else if (causeOfDeath == SimContext::DTH_NONAIDS)
				this->deathStatus = DTH_NONAIDS;
			else if (causeOfDeath == SimContext::DTH_TOX_ART)
				this->deathStatus = DTH_TOX_ART;
			else if (causeOfDeath == SimContext::DTH_TOX_PROPH)
				this->deathStatus = DTH_TOX_PROPH;
			else
				this->deathStatus = DTH_OTHER;
		}
	}
	//There may be those remaining that didn't go through CEPAC: the non-sexually actives!
	else{
		//this part of the function is for uninfected persons
		assert (Person::probDeathNatCauses[this->getDmgProfileVal(DmgProfile::GENDER)].size() > 0 );
		//if this person is past Person::maxYrForDeathStats, they should not be alive
		//get the correct probability of death for this person's gender and age
		if (this->getAge(YEAR) >= Person::probDeathNatCauses[this->getDmgProfileVal(DmgProfile::GENDER)].size()){
			cout << "The age is " << this->getAge(YEAR) << endl;
		}
		double deathRate = Person::probDeathNatCauses[this->getDmgProfileVal(DmgProfile::GENDER)].at(this->getAge(YEAR));
		this->death = _randomNums.chance(deathRate);
		if (this->death)
			this->deathStatus = DTH_NONAIDS;
	}

	//if they died, collect statistics
	if(this->death) {
		this->stats.setStat(STAT_TOTAL_LM, this->getAge(MONTH));
		this->stats.setStat(STAT_HIV_NEG_LM, this->getAge(MONTH) - (this->isInfected() ? this->stats.getStat(STAT_TIME_OF_INFECTION_MTH) : 0) );
		this->stats.setStat(STAT_HIV_POS_POSTINFECT_LM, this->stats.getStat(STAT_TOTAL_LM) - this->stats.getStat(STAT_AGE_AT_INFECTION_MTH));
	}

	return this->death;

}

void Person::setCurrBucketProfileID(DmgProfile::ProfileID _profileID) {
	this->currentBucketID = _profileID;
}

void Person::setSimContext(SimContext * newSimContext){
	this->cepacPatient->setSimContext(newSimContext);
}

Person* Person::sexualActivity(Person *_p, int _numActs, SexualPartnership::Type _partnershipType, EventParams& _eventParams, InfectionsTracker *infTrack) {
	assert( (_p!=NULL) );
	assert(_p->isAlive());
	assert( _partnershipType < SexualPartnership::ENDType);
	//TODO: CONDOM STUFF!

	if (_eventParams.outputTrace[EventParams::SINGLEPERSON] && (this->trace() || _p->trace())){
		if (this->trace()){
			_eventParams.traceStreams[EventParams::SINGLEPERSON] << "# Male " << this->getID() << " engages in " << _numActs << " acts with his " << *(SexualPartnership::TypeEnum.toString(_partnershipType)) << " " << _p->getID() << endl;
		}
		else {
			_eventParams.traceStreams[EventParams::SINGLEPERSON] << "# Female " << _p->getID() << " engages in " << _numActs << " acts with her " << *(SexualPartnership::TypeEnum.toString(_partnershipType)) << " " << this->getID() << endl;
		}
	}

	//increment numacts for this person and partner
	this->incrementNumActsThisMonth(_numActs);
	_p->incrementNumActsThisMonth(_numActs);

	//people are either both already infected or both uninfected
	if( this->isInfected() == _p->isInfected()) {
		return NULL;
	}

	//if infection occurs, return true
	Person *infected = this->isInfected() ? this : _p;
	Person *uninfected = this->isInfected() ? _p : this;

	bool transmissionOccured = false;
	for (int i = 0; i < _numActs; i++)
	{
		/** Regardless of infection, record the exposure */
		infTrack->recordExposure(_eventParams.currTime, infected);

		//force of infection from infected to uninfected
		double foifPerEvent = infected->getFOI(uninfected, _partnershipType, _eventParams);

		//If a condom was used, increase the number of condoms used for each person by numActs
		if (infected->getCondomUsedLastFOICalculation())
		{
			this->incrementCondomsUsedThisMonth(_numActs);
			_p->incrementCondomsUsedThisMonth(_numActs);
		}

		if (_eventParams.randomNums.chance(foifPerEvent))
		{
			transmissionOccured = true;
		}
	}

	//perform _numActs and see whether someone gets infected
	if( transmissionOccured) {
		if(_eventParams.outputTrace[EventParams::SINGLEPERSON] && (this->trace() || _p->trace())){
			_eventParams.traceStreams[EventParams::SINGLEPERSON] << " !!# " << infected->getID() << " infected " << uninfected->getID() << "!" << endl;
		}

		//record who infected whom
		if( infected == this)
			this->stats.incrStat(Person::STAT_NUM_INFECTED,1);
		else
			_p->stats.incrStat(Person::STAT_NUM_INFECTED,1);

		//if they get infected, then change status of uninfected to infected and count infection
		//The generation of infected for the newly infected will be 1+ the infected persons generation
		uninfected->becomeInfected( infected->getGenerationOfInfection() + 1,_eventParams);
		return uninfected;
	} else {
		if(_eventParams.outputTrace[EventParams::SINGLEPERSON] && (this->trace() || _p->trace())){
			_eventParams.traceStreams[EventParams::SINGLEPERSON] << " !# " << infected->getID() << " exposed but did not infect " << uninfected->getID() << "!" << endl;
		}

		//uninfected person was exposed but not infected
		uninfected->stats.incrStat(Person::STAT_EXPOSURES_BEFORE_INF, _numActs);
	}//if( !_eventParams.randomNums.chance(pow( 1-foifPerEvent, eventsThisMonth)) ) {

	return NULL;
}

double Person::updateHealthStatus(EventParams& _eventParams, ArtTestingTracker *testTracker) {
	//if this person has died, then don't update.
	if (!this->isAlive())
		return 0;

	//if we haven't put this person through CEPAC, then don't bother w/ second part
	if(!this->wentThroughCEPAC)
		return 0;

	double costThisMonth = 0;

	if (_eventParams.useRollout){
		
	}
	else{
		//Adjust the CEPAC SimContext depending on what time it is
		if (_eventParams.itIsTimeToSwitchSimContext()){
			this->cepacPatient->setSimContext(_eventParams.cepacSimContexts[this->getCEPACSimContextIndex(_eventParams)]);
		}
	}

	RunStats::HIVScreening hivScreeningBefore = *_eventParams.cepacRunStats->getHIVScreening();

	//run this person's patient info one month forward in CEPAC
	this->cepacPatient->simulateMonth();

	RunStats::HIVScreening hivScreeningAfter = *_eventParams.cepacRunStats->getHIVScreening();

	//Update this patient's costs
	costThisMonth = this->cepacPatient->getGeneralState()->costsDiscounted - this->CEPACcosts;
	this->CEPACcosts = this->cepacPatient->getGeneralState()->costsDiscounted;

	//update HVL and CD4 for this Person if they are infected
	if(this->isInfected()){
		this->cd4 = this->cepacPatient->getDiseaseState()->currTrueCD4;

		//update HVL state
		SimContext::HVL_STRATA hvlStrata = this->cepacPatient->getDiseaseState()->currTrueHVLStrata;
		if(hvlStrata == SimContext::HVL_VLO)
			this->hvl = HVL_ZERO;		//0-20
		else if(hvlStrata == SimContext::HVL__LO)
			this->hvl = HVL_ONE;		//21-500
		else if(hvlStrata == SimContext::HVL_MLO)
			this->hvl = HVL_TWO;		//501-3000
		else if(hvlStrata == SimContext::HVL_MED)
			this->hvl = HVL_THREE;		//3001-10000
		else if(hvlStrata == SimContext::HVL_MHI)
			this->hvl = HVL_FOUR;		//10001-30000
		else if(hvlStrata == SimContext::HVL__HI)
			this->hvl = HVL_FIVE;		//30001-100000
		else if(hvlStrata == SimContext::HVL_VHI)
			this->hvl = HVL_SIX;		//100000+
		else{
			cerr << "Invalid CEPAC API infection state: " << *(SimContext::HVL_STRATA_STRS[hvlStrata]);
			Util::exitWithPrompt(-1);
		}

		this->currentTrueHvl = this->hvl;

		if(this->cepacPatient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_ACUTE_SYN){
			this->hvl = HVL_PRIMARY;
			if(this->cepacPatient->getMonitoringState()->isDetectedHIVPositive)
				this->hivStatus = OBSERVED_ACUTE;
			else
				this->hivStatus = UNOBSERVED_ACUTE;
		}
		//Late stage is defined as having failed the last ART regimen (or having no art regimens to start with) and a CD4 <= 50
		else if((!(this->cepacPatient->getARTState()->hasNextRegimenAvailable) &&
			(!(this->cepacPatient->getARTState()->isOnART) || this->cepacPatient->getARTState()->hasObservedFailure)) &&
			this->cepacPatient->getDiseaseState()->currTrueCD4 <= 50){
			this->hvl = HVL_LATESTAGE;
			if(this->cepacPatient->getMonitoringState()->isDetectedHIVPositive)
				this->hivStatus = OBSERVED_LATESTAGE;
			else
				this->hivStatus = UNOBSERVED_LATESTAGE;
		}
		else{
			if(this->cepacPatient->getMonitoringState()->isDetectedHIVPositive)
				this->hivStatus = OBSERVED_CHRONIC;
			else
				this->hivStatus = UNOBSERVED_CHRONIC;
		}

		//Update OI History
		for(int i = 0; i < Constants::NUMBER_OF_OIS; i++){
			this->oiHistory[i] = this->cepacPatient->getDiseaseState()->hasTrueOIHistory[i];
		}
	}

	bool offeredTest = hivScreeningAfter.numAcceptTest > hivScreeningBefore.numAcceptTest || hivScreeningAfter.numRefuseTest > hivScreeningBefore.numRefuseTest;
	bool acceptedTest = offeredTest && hivScreeningAfter.numAcceptTest > hivScreeningBefore.numAcceptTest;
	bool returnedForResults = hivScreeningAfter.numReturnForResults > hivScreeningBefore.numReturnForResults;
	SimContext::TEST_RESULT testResult = (SimContext::TEST_RESULT)0;

	if(returnedForResults)
	{
		if(hivScreeningAfter.numTestResultsHIVNegativeType[SimContext::TEST_FALSE_POS] > hivScreeningBefore.numTestResultsHIVNegativeType[SimContext::TEST_FALSE_POS])
		{
			testResult = SimContext::TEST_FALSE_POS;
		}
		else if(hivScreeningAfter.numTestResultsPrevalentType[SimContext::TEST_TRUE_POS] > hivScreeningBefore.numTestResultsPrevalentType[SimContext::TEST_TRUE_POS] ||
			(hivScreeningAfter.numTestResultsIncidentType[SimContext::TEST_TRUE_POS] > hivScreeningBefore.numTestResultsIncidentType[SimContext::TEST_TRUE_POS]))
		{
			testResult = SimContext::TEST_TRUE_POS;
		}
		else if(hivScreeningAfter.numTestResultsHIVNegativeType[SimContext::TEST_TRUE_POS] > hivScreeningBefore.numTestResultsHIVNegativeType[SimContext::TEST_TRUE_POS])
		{
			testResult = SimContext::TEST_TRUE_POS;
		}
		else if(hivScreeningAfter.numTestResultsPrevalentType[SimContext::TEST_FALSE_NEG] > hivScreeningBefore.numTestResultsPrevalentType[SimContext::TEST_FALSE_NEG] ||
			(hivScreeningAfter.numTestResultsIncidentType[SimContext::TEST_FALSE_NEG] > hivScreeningBefore.numTestResultsIncidentType[SimContext::TEST_FALSE_NEG]))
		{
			testResult = SimContext::TEST_FALSE_NEG;
		}
	}

	if(_eventParams.outputTrace[EventParams::ARTROLLOUT])
	{
		if(offeredTest)
		{
			testTracker->recordTest(this, acceptedTest, returnedForResults, testResult);
		}
	}

	return costThisMonth;
}

//Call this after all transmission/population dynamics are done.
//Runs infected through CEPAC until they die and adds their LM etc to CEPAC stats
void Person::runCEPACtoDeath(RandomNums& _randomNums){
	if (!this->isAlive()){
		/** If we're already dead, don't do anything */
		return;
	}
	if (!this->wentThroughCEPAC){
		while (!this->rollForDeath(_randomNums)){
			this->ageOneTimeUnit();
		}
	} else {
		while (this->cepacPatient->isAlive()){
			this->ageOneTimeUnit();
			this->cepacPatient->simulateMonth();
		}
		this->rollForDeath(_randomNums);
	}
}

void Person::resetCondomUsage(){
	this->condomsUsedThisMonth = 0;
}

void Person::resetNumActs(){
	this->numActsThisMonth = 0;
}
int Person::getCondomsUsedThisMonth(){
	return this->condomsUsedThisMonth;
}

int Person::getNumActsThisMonth(){
	return this->numActsThisMonth;
}

void Person::incrementCondomsUsedThisMonth(int condoms){
	this->condomsUsedThisMonth += condoms;
}

void Person::incrementNumActsThisMonth(int _numActs){
	this->numActsThisMonth += _numActs;
}
bool Person::getCondomUsedLastFOICalculation(){
	return this->condomUsedLastFOICalculation;
}

//-------------< Start FullVector indices related methods >----------------------//
/* @function: setFVindices
 * @arguments: vector<int> FVind, FullVector* FV
 * @effects: if this.FVindices is currently empty and all indices correlate with members
 * of FV that point to this, sets this.FVindices to FVind
 * @return: true if this.FVindices was set to FVind or false otherwise
 */
bool Person::setFVindices(vector<unsigned int> FVind, FullVector* FV){
	//First make sure there is no vector already associated with FV
	map<FullVector*, vector<unsigned int> >::iterator iter = this->FVindices.find(FV);
	if (iter == this->FVindices.end()){
		//Make sure all members of FVind are indices of FV pointing to this
		bool FVmatch = true;
		vector<unsigned int>::iterator iter;
		for(iter = FVind.begin(); iter != FVind.end(); iter++){
			if (FV->at(*iter)->getID() != this->id){
				//Set FVmatch to false if one of the members of FVind is not an index to a pointer to this in FV
				FVmatch = false;
			}
		}

		if (FVmatch){
		this->FVindices[FV] = FVind;
		return true;
		}
	}
	return false;
}


/* @function: addFVindices
 * @arguments: int index, FullVector* FV
 * @effects: If FV[index] points to this and index is not already a member of
 * this.FVindices, adds index to this.FVindices
 * @return: true if index was added to this.FVindices or false otherwise
 */
bool Person::addFVindices(int index, FullVector* FV){
	if (FV->at(index)->getID() == this->id){
		//Will be used to check if index is already in the appropriate FVindex
		bool indexAlreadyInFVindices = false;
		//Will store the appropriate FVindex;
		vector<unsigned int>* FVindex;

		//See if the FVindices for FV exists
		map<FullVector*, vector<unsigned int> >::iterator mIter = this->FVindices.find(FV);
		if (mIter != this->FVindices.end()){
			FVindex = &(mIter->second);
			//Check that index is not already in this.FVindices
			vector<unsigned int>::iterator iter;
			for(iter = FVindex->begin(); iter != FVindex->end(); iter++){
				if (*iter == index){
					indexAlreadyInFVindices = true;
					break;
				}//if (*iter == index)
			}//for (iter = FVindex->begin(); ...
		}//if (mIter != this->FVindices.end())
		else{
			//Create new vector<unsigned int> for FV if one doesn't exist
			vector<unsigned int> newFVindex;
			this->FVindices[FV] = newFVindex;
			mIter = this->FVindices.find(FV);
			FVindex = &(mIter->second);
			assert(FVindex->empty());
		}

		if (!indexAlreadyInFVindices){
		//	cout << "doing the adding..." << endl;
			FVindex->push_back(index);
			return true;
		}
	}
	return false;
}

/* @function: removeFVindices
 * @arguments: int index, FullVector* FV
 * @effects: If FV[index] does not point to this, removes index from this.FVindices
 * @return: true if index was removed from this.FVindices, false otherwise
 */
bool Person::removeFVindices(int index, FullVector* FV){
	//Check if FV[index] points to this... but first check if FV[index] is within the size of FV
	bool conditionsToRemoveAreGo = (index > (FV->size() - 1));
	if (!conditionsToRemoveAreGo){
		conditionsToRemoveAreGo = (FV->at(index)->getID() != this->id);
	}
	if(conditionsToRemoveAreGo){
		map<FullVector*, vector<unsigned int> >::iterator mIter = this->FVindices.find(FV);
		if (mIter != this->FVindices.end()){
			vector<unsigned int>* FVindex = &(mIter->second);
			vector<unsigned int>::iterator iter;
			for(iter = FVindex->begin(); iter != FVindex->end(); iter++){
				if (*iter == index){
					FVindex->erase( iter );
					//If we've removed the last index in FVindex, remove it from the map of FVindices
					if (FVindex->empty()){
						FVindices.erase(mIter);
					}
					return true;
				}
			}
		}
	}
	return false;
}

/* @function: memberFVindices
 * @arguments: int index
 * @effects: none
 * @return: true iff this.FVindices contains index
 */
bool Person::memberFVindices(int index, FullVector* FV){
	map<FullVector*, vector<unsigned int> >::iterator mIter = this->FVindices.find(FV);
	if (mIter != this->FVindices.end()){
		vector<unsigned int> FVindex = mIter->second;
		vector<unsigned int>::iterator iter;
		for(iter = FVindex.begin(); iter != FVindex.end(); iter++){
			if (*iter == index)
				return true;
		}
	}
	return false;
}

/* @function: getFVindices
 * @arguments: none
 * @effects: none
 * @return: copy of this.FVindices
 */
vector<unsigned int> Person::getFVindices(FullVector* FV){
	vector<unsigned int> vcopy;
	/*map<FullVector*, vector<int> >::iterator iter = this->FVindices.find(FV);
	if (iter != this->FVindices.end()){
		vcopy.assign(iter->second.begin(), iter->second.end());
	}*/
	//ERINWASHERE
	//NEW

	vector<unsigned int> personsIndices = this->FVindices[FV];
	if (personsIndices.size() > 0){
		vcopy.assign(personsIndices.begin(), personsIndices.end());
	}
	//ENDNEW
	return vcopy;
}

//-------------< End FullVector indices related methods >----------------------//

//-------------< Start BucketAge related methods >----------------------//
/* @function: getRiskLevel
 * @return: this.risk
 */
const Person::RiskLevel Person::getRiskLevel() const{
	return this->risk;
}

/* @function: getHivStatus
 * @return: this.hivStatus
 */
const Person::HIVStatus Person::getHIVStatus() const{
	return this->hivStatus;
}

/* @function: getSexualActivity
 * @return: this.activityLevel
 */
int Person::getSexualActivity(){
	return this->activityLevel;
}

/**** Start constructors, destructors, initializers *****/

Person::Person(){
	/*this->healthAfterInfection = NULL;*/
	this->risk = LOW;
	this->traceMe = false;
	this->generationOfInfection = -1;
	this->ageInfected = -1;
	this->wentThroughCEPAC = false;
	this->cepacPatient = NULL;
	for(SexualPartnership::Type type = SexualPartnership::Type(0); type < SexualPartnership::ENDType; ++type) {
		this->unformedPartnershipsLatestTime[type] = 0;
		this->unformedPartnershipsTotal[type] = 0;
		this->numPartnersInHistory[type] = 0;
		this->monthOfLatestPartnershipDissolution[type] = 0;
	}
	this->monthOfLatestConcurrent = 0;
	this->CEPACcosts = 0;

	this->graphNode = new GraphVizGraphElements::personNode(0, true, 0);
}

//this constructor is used by the Male and Female classes
Person::Person(	EventParams &_eventParams, int _age, unsigned int _populationID)	{
	assert( _populationID >= 0);

	this->id = Person::idCounter++;
	this->populationID = _populationID;
#ifndef TESTING
	if (!Util::withinRange<int>(_age,0, Util::convertTime(YEAR,MONTH,Person::maxYrForDeathStats))){
		if (_age < 0){
			_age = 0;
		}
		else {
			//cout << "SOMEONE WAS TOO OLD (" << _age << ")!  MAKING THEM " << Util::convertTime(YEAR, MONTH, Person::maxYrForDeathStats) << "!" << endl;
			_age = Util::convertTime(YEAR, MONTH, Person::maxYrForDeathStats);
		}
	}
#endif
	this->age = _age;
	this->initAge = _age;
	this->ageInfected = -1;

	this->death = false;
	this->deathStatus = ALIVE;
	this->sexualActivityLevel = 1.0;
	//this->healthAfterInfection = NULL;
	this->cepacPatient = NULL;
	this->CEPACcosts = 0;
	this->wentThroughCEPAC = false;
	this->generationOfInfection = -1;
	this->stats.init(&Person::StatsEnum);
	this->traceMe = false;
	for(SexualPartnership::Type type = SexualPartnership::Type(0); type < SexualPartnership::ENDType; ++type) {
		this->unformedPartnershipsLatestTime[type] = 0;
		this->unformedPartnershipsTotal[type] = 0;
		this->numPartnersInHistory[type] = 0;
		this->monthOfLatestPartnershipDissolution[type] = 0;
	}
	this->monthOfLatestConcurrent = 0;
	this->resetNumActs();

	//set the person's initial demographic profile. gender is set within the Male/Female constructors
	//everyone is set as NA, but you can call becomeSexuallyActive(_eventParams) elsewhere if you want this person to be SA
	this->dmgProfile.set(DmgProfile::SEXUAL_ACTIVITY_STATUS, DmgProfile::NA);
	this->dmgProfile.set(DmgProfile::SEXUAL_ORIENTATION, DmgProfile::HETERO);
	this->dmgProfile.set(DmgProfile::RELATIONSHIP_STATUS, DmgProfile::SINGLE);
	//everyone is set as NON_CSW, but you can call becomeCSW() elsewhere if you want this person to be CSW
	this->dmgProfile.set(DmgProfile::EMPLOYMENT, DmgProfile::NON_CSW);

	this->hivStatus = NEGATIVE;
	this->cd4 = -1;
	this->hvl = UNINFECTED;

	//this->dmgProfile -- default constructor sets everything to wildcards
	//this person isn't a member of any bucket yet. this value will be changed when EntityPool adds or removes the person
	this->currentBucketID = DmgProfile::END;

	/** Create the new graph element keeping track of all relationship history via edges */
	this->graphNode = new GraphVizGraphElements::personNode(this->id, (this->dmgProfile.get(DmgProfile::GENDER) == DmgProfile::MALE), _eventParams.currTime);
}

Person::~Person(void) {
	//If this person went through CEPAC, delete their CEPACpatient
	//TODO: If they're not dead, force kill them (in CEPAC) to log the stats (?)
	//Didn't I do this somewhere?
	delete this->cepacPatient;

	//take person out of all current relationships
	list<SexualPartnership*>::iterator toDelete;
	for(SexualPartnership::Type type = SexualPartnership::Type(0); type < SexualPartnership::ENDType; ++type) {
		list<SexualPartnership*>::iterator iter = this->partners[type].begin();
		list<SexualPartnership*>::iterator iterEnd = this->partners[type].end();

	  while(iter != iterEnd ) {
	    toDelete = iter;
	    iter++;
	    delete (*toDelete);
	  }
	}

	//Remove person from all FVs

	/*map<FullVector*, vector<unsigned int> >::iterator FViter = this->FVindices.begin();
	while (FViter != this->FVindices.end()){
		FViter->first->remove(this);
		FViter++;
	}*/

	delete graphNode;
}

void Person::deletePersonWithoutDeleting(){
	//Don't delete the cepacPatient -- this causes a weird exception when you try to delete it at the close of simulation, so keep it around

	//take person out of all current relationships
	list<SexualPartnership*>::iterator toDelete;
	for(SexualPartnership::Type type = SexualPartnership::Type(0); type < SexualPartnership::ENDType; ++type) {
		list<SexualPartnership*>::iterator iter = this->partners[type].begin();
		list<SexualPartnership*>::iterator iterEnd = this->partners[type].end();

	  while(iter != iterEnd ) {
	    toDelete = iter;
	    iter++;
	    delete (*toDelete);
	  }
	}

	map<FullVector*, vector<unsigned int> >::iterator FViter = this->FVindices.begin();
	while (FViter != this->FVindices.end()){
		FViter->first->remove(this);
		FViter++;
	}
}

/**** End constructors, destructors, initializers *****/

int Person::getCEPACSimContextIndex(EventParams& _eventParams)
{
    int returnValue = 0;
    for (int i = 0; i < Constants::NUMBER_OF_CEPAC_FILES; i++)
    {
	if (_eventParams.currTime > _eventParams.timesToSwitchSimContext[i])
	    returnValue = i;
    }
    
    return returnValue;
}

//----------------< End Methods for Person >-------------------//





//----------------< Start Methods for to be implemented by Male and Female >-------------------//


Person* Person::choosePartner(SexualPartnership::Type _partnershipType, EntityPool *_availableEntities, bool _remove) {
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	cerr << "Called Person::choosePartner()" << endl;
	return NULL;
}

double Person::getFOI(Person *_p, SexualPartnership::Type _partnershipType, EventParams& _eventParams){
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	cerr << "Called Person::getFOI()" << endl;
	return 0.0;
}

double Person::getMinPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const {
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	cerr << "Person::getMinPartnerSelectVal()" << endl;
	return 0.0;
}



double Person::getMaxPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const {
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	cerr << "Person::getMaxPartnerSelectVal()" << endl;
	return 0.0;
}

double Person::rollForAgeDifference(SexualPartnership::Type _partnershipType, RandomNums& _randomNums){
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	cerr << "Person::rollForAgeDifference()" << endl;
	return 0.0;
}

double Person::getTransmissionCoeff() {
	cerr << "Called Person::getTransmissionCoeff()" << endl;
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	return 0.0;
}

bool Person::possibleMatch(SexualPartnership::Type _partnershipType, Person *_p) {
	cerr << "Called Person::possibleMatch()" << endl;
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	return false;
}

int Person::rollForNewPartnershipDuration(SexualPartnership::Type _partnershipType, RandomNums& _randomNums, Person *_p){
	cerr << "Called Person::rollForNewPartnershipDuration()" << endl;
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	return false;
}

int Person::rollForNumPartners(RandomNums& _randomNums, SexualPartnership::Type _partnershipType) {
	cerr << "Called Person::rollForNumPartners()" << endl;
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	return false;

}

int Person::rollNumEventsPerPartner(Person *_p, RandomNums& _randomNums, SexualPartnership::Type _partnershipType){
	cerr << "Called Person::rollNumEventsPerPartner()" << endl;
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	return false;

}


//----------------< Start Methods for to be implemented by Male and Female >-------------------//


