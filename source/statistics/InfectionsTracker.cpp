#include "InfectionsTracker.h"
#include "../Constants.h"
#include "../Population.h"

#include <iostream>
#include <sstream>

using namespace std;

InfectionsTracker::InfectionsTracker() {

	//zero out all infection tallies
	for(int i = 0; i < SexualPartnership::ENDType; ++i){
		for(int j = 0; j < DmgProfile::TotalNumBuckets; ++j) {
			for (int g = 0; g < this->NUMBER_GENERATIONS_TO_TRACE; g++)
				this->currPrevalentInfections[j][g] = 0;
			for(int k = 0; k < DmgProfile::TotalNumBuckets; ++k)
				this->incidentInfections[i][j][k] = 0;
		}
	}

	for (int i = 0; i < Person::ENDHVLStrata; ++i){
		this->totalIncidentInfections[i] = 0;
		this->totalExposures[i] = 0;
		this->currTimeStepIncidentInfs[i] = 0;
		this->currTimeExposures[i] = 0;
	}
	for (int i = 0; i < Person::ENDRiskLevel; i++){
		for (int j = 0; j < DmgProfile::ENDGender; j++){
			for (int k = 0; k < DmgProfile::ENDEmployment; k++){
				this->currTimeStepIncidentInfsRiskGenderEmployment[i][j][k] = 0;
				this->totalIncidentInfsRiskGenderEmployment[i][j][k] = 0;
			}
		}
	}
	this->currTimeStepNumInfected = 0;
	this->currTimeStepCD4InfectionSum = 0;
	this->currTimeStepCD4InfectionSumSq = 0;

	for (int i = 0; i < DmgProfile::ENDGender; i++){
		this->totalIncidentInfsGender[i] = 0;
		this->currTimeStepNumInfectedGender[i] = 0;
		this->currTimeStepAgeInfectionSumGender[i] = 0;
		this->currTimeStepAgeInfectionSumSqGender[i] = 0;
	}
	for (int i = 0; i < Person::ENDRiskLevel; i++){
		this->totalIncidentInfsRiskCSW[i] = 0;
		this->totalIncidentInfsRisk[i] = 0;
	}
	for (int i = 0; i < Person::ENDRiskLevel; i++){
		for (int j = 0; j < DmgProfile::ENDGender; j++){
			for (int k = 0; k < DmgProfile::ENDEmployment; k++){
				this->currTimeStepNumInfectedRiskGenderEmployment[i][j][k] = 0;
				this->currTimeStepAgeInfectionSumRiskGenderEmployment[i][j][k] = 0;
				this->currTimeStepAgeInfectionSumSqRiskGenderEmployment[i][j][k] = 0;
			}
		}
	}

	this->currTimeStep = 0;
	this->lastTwelveIncidenceRates = *(new deque<double>(12, 0.0));
	

}

unsigned long InfectionsTracker::getCurrTimeStepIncidentInfsTotal(){
	unsigned long infections = 0;
	for (int i = 0; i < Person::ENDHVLStrata; ++i){
		infections += this->currTimeStepIncidentInfs[i];
	}
	return infections;
}


double InfectionsTracker::calculateAnnualIncidence(){
	assert(this->lastTwelveIncidenceRates.size() == 12);
	deque<double>::iterator it;
	double incidence = 0;

	for (it = this->lastTwelveIncidenceRates.begin(); it != this->lastTwelveIncidenceRates.end(); it++){
		incidence += *(it);
	}

	return incidence;
}

void InfectionsTracker::addToDetailedTrace(DmgProfile::ProfileID _profileID) {
	this->profileIDsForDetailedTrace.push_back(_profileID);
}

unsigned long InfectionsTracker::getNumIncidentInfections() {
	unsigned long infections = 0;
	for (int i = 0; i < Person::ENDHVLStrata; i++){
		infections += this->totalIncidentInfections[i];
	}
	return infections;
}

unsigned long InfectionsTracker::getNumIncidentInfections(DmgProfile::ProfileID _infectorsProfileID, DmgProfile::ProfileID _infectedsProfileID) {
	assert( Util::withinRange( _infectorsProfileID, DmgProfile::MIN, DmgProfile::MAX));
	assert( Util::withinRange( _infectedsProfileID, DmgProfile::MIN, DmgProfile::MAX));

	//it seems that while loops are generally faster than for loops?
	int currPartnershipType = SexualPartnership::TypeEnum.getMin();
	int endPartnershipType = SexualPartnership::ENDType;

	unsigned long infections = 0;

	//loop through each type of partnerships and tally the amount of infections where:
	//	the infectors had DmgProfie::ProfileID = _infectorsProfileID and
	//	the infecteds had DmgProfie::ProfileID = _infectedsProfileID
	while(currPartnershipType <= endPartnershipType) {
		infections += this->incidentInfections[currPartnershipType][_infectorsProfileID][_infectedsProfileID];
	}

	return infections;
}

unsigned long InfectionsTracker::getNumIncidentInfections(SexualPartnership::Type _partnershipType, DmgProfile::ProfileID _infectorsProfileID, DmgProfile::ProfileID _infectedsProfileID){
	assert (_partnershipType != SexualPartnership::ENDType);
	assert( Util::withinRange( _infectorsProfileID, DmgProfile::MIN, DmgProfile::MAX));
	assert( Util::withinRange( _infectedsProfileID, DmgProfile::MIN, DmgProfile::MAX));

	return this->incidentInfections[_partnershipType][_infectorsProfileID][_infectedsProfileID];
}

//returns the prevalence rate among sexually active pop
double InfectionsTracker::getSAPrev(Population* _population){
	long totalInfected = 0;
	long currPopSize = _population->getSize();
	long currSAPopSize = currPopSize - _population->getNASize();
	//Currently Infected
	//total the current infections
	for(int i = 0; i < DmgProfile::TotalNumBuckets; i++) {
		for (int j = 0; j < this->NUMBER_GENERATIONS_TO_TRACE; j++)
			totalInfected += this->currPrevalentInfections[i][j];
	}

	//current pop and prevalence of sexually active population
	//Need total number of infected for SA population only
	int totalInfectedSA = totalInfected;
	DmgProfile NAProfile;
	NAProfile.set(DmgProfile::SEXUAL_ACTIVITY_STATUS, DmgProfile::NA);
	vector<DmgProfile::ProfileID> NAProfileIDs;
	NAProfile.selectProfileIDs(NAProfileIDs,NULL);
	for (int i = 0; i < NAProfileIDs.size(); i++){
		for (int j = 0; j < this->NUMBER_GENERATIONS_TO_TRACE; j++)
			totalInfectedSA -= this->currPrevalentInfections[NAProfileIDs[i]][j];
	}

	if (currSAPopSize == 0){
		return -1;
	}
	else{
		double currPrevalenceSA = (totalInfectedSA)/(double)currSAPopSize;
		return currPrevalenceSA;
	}
}

/*
* Resests counting of incident infections for time step
*/
void InfectionsTracker::resetIncidentInfections(long _time){
	/** Reset counter for incident infections and exposures for current timestep*/
	for (int i = 0; i < Person::ENDHVLStrata; i++){
		this->currTimeStepIncidentInfs[i] = 0;
		this->currTimeExposures[i] = 0;
	}
	
	for (int i = 0; i < Person::ENDRiskLevel; i++){
		for (int j = 0; j < DmgProfile::ENDGender; j++){
			for (int k = 0; k < DmgProfile::ENDEmployment; k++){
				this->currTimeStepIncidentInfsRiskGenderEmployment[i][j][k] = 0;
			}
		}
	}

	this->currTimeStepNumInfected = 0;
	this->currTimeStepCD4InfectionSum = 0;
	this->currTimeStepCD4InfectionSumSq = 0;

	for (int i = 0; i < DmgProfile::ENDGender; i++){
		this->currTimeStepAgeInfectionSumGender[i] = 0;
		this->currTimeStepAgeInfectionSumSqGender[i] = 0;
		this->currTimeStepNumInfectedGender[i] = 0;
	}

	for (int i = 0; i < Person::ENDRiskLevel; i++){
		for (int j = 0; j < DmgProfile::ENDGender; j++){
			for (int k = 0; k < DmgProfile::ENDEmployment; k++){
				this->currTimeStepAgeInfectionSumRiskGenderEmployment[i][j][k] = 0;
				this->currTimeStepAgeInfectionSumSqRiskGenderEmployment[i][j][k] = 0;
				this->currTimeStepNumInfectedRiskGenderEmployment[i][j][k] = 0;
			}
		}
	}
	for(vector<boost::tuple<long,int,int> >::iterator ageIt=this->currTimeStepIncidentInfsAgeMale.begin(); ageIt!=this->currTimeStepIncidentInfsAgeMale.end(); ageIt++) {
		(*ageIt).get<Population::AGE_RANGE_SIZE>() = 0;
	}
	for(vector<boost::tuple<long,int,int> >::iterator ageIt=this->currTimeStepIncidentInfsAgeFemale.begin(); ageIt!=this->currTimeStepIncidentInfsAgeFemale.end(); ageIt++) {
		(*ageIt).get<Population::AGE_RANGE_SIZE>() = 0;
	}

	this->currTimeStep = _time;
}


/*
initializes the counters for incident infections by age and gender 
*/
void InfectionsTracker::initializeIncidentInfectionsByAge(vector<boost::tuple<long, int, int> > &_incMale, vector<boost::tuple<long, int, int> > &_incFemale,  vector<boost::tuple<long, int, int> > &_totalIncAge){
	this->currTimeStepIncidentInfsAgeMale = _incMale;
	this->currTimeStepIncidentInfsAgeFemale = _incFemale;
	this->totalIncidentInfsAge = _totalIncAge;
}

/*
 * Records a new exposure regardless of whether an infection happened or not
 */
void InfectionsTracker::recordExposure(long _time, const Person *_infector){
	/** Reset counter for incident infections and exposures for current timestep if this is the first time an InfectionsTracker function has been called */
	if(_time > this->currTimeStep) {
		resetIncidentInfections(_time);
	}

	this->currTimeExposures[_infector->getHVL()]++;
	this->totalExposures[_infector->getHVL()]++;
}

//records a New infection and also prints the infection out to a trace
void InfectionsTracker::recordIncidentInfection(long _time, SexualPartnership::Type _partnershipType, const Person *_infector, const Person *_infected, bool _print, ostream &_traceOutStream) {
	assert (_time >= 0);
	//check to see if the people are valid: not null, not dead, in a valid bucket
	assert((_infector != NULL) && (_infector->isAlive()) && ( Util::withinRange( _infector->getDmgProfile()->getProfileID(), DmgProfile::MIN, DmgProfile::MAX) ) );
	assert((_infected != NULL) && (_infected->isAlive()) && ( Util::withinRange( _infected->getDmgProfile()->getProfileID(), DmgProfile::MIN, DmgProfile::MAX) ) );
	assert(_partnershipType < SexualPartnership::ENDType);

	//reset counter for incident infections and exposures for current timestep
	if(_time > this->currTimeStep) {
		resetIncidentInfections(_time);
	}

	//record infection
	++(this->incidentInfections[_partnershipType][_infector->getDmgProfile()->getProfileID()][_infected->getDmgProfile()->getProfileID()]);

	//print out infection for trace
	if(_print) {
		_infector->print(_traceOutStream,Constants::TABTAB);
		_traceOutStream << Constants::TABTAB << "Just Infected (" << *(SexualPartnership::TypeEnum.toString(_partnershipType)) << "): ";
		_infected->print(_traceOutStream,Constants::BLANK);
	}

	//The person doing the infecting should be infected!
	assert(_infector->getHVL() > Person::UNINFECTED);

	this->currTimeStepIncidentInfs[_infector->getHVL()]++;
	this->currTimeStepIncidentInfsRiskGenderEmployment[_infected->getRiskLevel()][_infected->getDmgProfileVal(DmgProfile::GENDER)][_infected->getDmgProfileVal(DmgProfile::EMPLOYMENT)]++;
	this->totalIncidentInfections[_infector->getHVL()]++;
	int infectedAge = _infected->getAge(MONTH);
	double infectorCD4 = _infector->cd4;

	DmgProfile::Gender infectedGender = (DmgProfile::Gender) _infected->getDmgProfileVal(DmgProfile::GENDER);
	Person::RiskLevel infectedRisk = _infected->getRiskLevel(); 
	int minAge,maxAge;

	this->currTimeStepNumInfected++;
	this->currTimeStepCD4InfectionSum += infectorCD4;
	this->currTimeStepCD4InfectionSumSq += infectorCD4 * infectorCD4;

	this->currTimeStepNumInfectedGender[infectedGender]++;
	this->currTimeStepAgeInfectionSumGender[infectedGender] += infectedAge;
	this->currTimeStepAgeInfectionSumSqGender[infectedGender] += infectedAge*infectedAge;

	this->currTimeStepNumInfectedRiskGenderEmployment[infectedRisk][_infected->getDmgProfileVal(DmgProfile::GENDER)][_infected->getDmgProfileVal(DmgProfile::EMPLOYMENT)]++;
	this->currTimeStepAgeInfectionSumRiskGenderEmployment[infectedRisk][_infected->getDmgProfileVal(DmgProfile::GENDER)][_infected->getDmgProfileVal(DmgProfile::EMPLOYMENT)] += infectedAge;
	this->currTimeStepAgeInfectionSumSqRiskGenderEmployment[infectedRisk][_infected->getDmgProfileVal(DmgProfile::GENDER)][_infected->getDmgProfileVal(DmgProfile::EMPLOYMENT)] += infectedAge*infectedAge;

	if (_infected->getDmgProfileVal(DmgProfile::EMPLOYMENT) == DmgProfile::CSW){
		this->totalIncidentInfsRiskCSW[infectedRisk]++;
	}
	this->totalIncidentInfsRisk[_infected->getRiskLevel()]++;
	this->totalIncidentInfsRiskGenderEmployment[_infected->getRiskLevel()][_infected->getDmgProfileVal(DmgProfile::GENDER)][_infected->getDmgProfileVal(DmgProfile::EMPLOYMENT)]++;
	if (_infected->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE){
		this->totalIncidentInfsGender[DmgProfile::MALE]++;
		for(vector<boost::tuple<long,int,int> >::iterator ageIt=this->currTimeStepIncidentInfsAgeMale.begin(); ageIt!=this->currTimeStepIncidentInfsAgeMale.end(); ageIt++) {
			minAge = (*ageIt).get<Population::MIN_AGE_IN_MONTHS>();
			maxAge = (*ageIt).get<Population::MAX_AGE_IN_MONTHS>();
			if (infectedAge >= minAge && infectedAge <= maxAge){
				(*ageIt).get<Population::AGE_RANGE_SIZE>()++;
				break;
			}
		}
	}
	else if (_infected->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::FEMALE){
		this->totalIncidentInfsGender[DmgProfile::FEMALE]++;
		for(vector<boost::tuple<long,int,int> >::iterator ageIt=this->currTimeStepIncidentInfsAgeFemale.begin(); ageIt!=this->currTimeStepIncidentInfsAgeFemale.end(); ageIt++) {
			minAge = (*ageIt).get<Population::MIN_AGE_IN_MONTHS>();
			maxAge = (*ageIt).get<Population::MAX_AGE_IN_MONTHS>();
			if (infectedAge >= minAge && infectedAge <= maxAge){
				(*ageIt).get<Population::AGE_RANGE_SIZE>()++;
				break;
			}
		}
	}

	for(vector<boost::tuple<long,int,int> >::iterator ageIt=this->totalIncidentInfsAge.begin(); ageIt!=this->totalIncidentInfsAge.end(); ageIt++) {
		minAge = (*ageIt).get<Population::MIN_AGE_IN_MONTHS>();
		maxAge = (*ageIt).get<Population::MAX_AGE_IN_MONTHS>();
		if (infectedAge >= minAge && infectedAge <= maxAge){
			(*ageIt).get<Population::AGE_RANGE_SIZE>()++;
			break;
		}
	}


}

/**
records the cd4 at transmission (requested by clinical out stream)
**/
void InfectionsTracker::recordCD4AtTransmission(ostream &_outStream){
	if (this->currTimeStepNumInfected !=0)
	{
		double cd4Mean = this->currTimeStepCD4InfectionSum/this->currTimeStepNumInfected;
		double cd4SD = sqrt(this->currTimeStepCD4InfectionSumSq/this->currTimeStepNumInfected - cd4Mean*cd4Mean);
		_outStream << this->currTimeStepNumInfected << Constants::TAB << cd4Mean << Constants::TAB << cd4SD << Constants::TAB;
	}
	else{
		_outStream << this->currTimeStepNumInfected << Constants::TAB << "N/A" << Constants::TAB << "N/A" << Constants::TAB;
	}

}

void InfectionsTracker::setPrevalentInfections(long _time, unsigned long _prevalenceByBucket[DmgProfile::TotalNumBuckets][NUMBER_GENERATIONS_TO_TRACE], const vector <boost::tuple<long, int, int> > & _prevalenceByAgeMale, const vector <boost::tuple<long, int, int> > & _prevalenceByAgeFemale, unsigned long _prevalenceByRiskGenderEmployment[Person::ENDRiskLevel][DmgProfile::ENDGender][DmgProfile::ENDEmployment]) {
	DmgProfile::ProfileID currProfileID = DmgProfile::MIN;
	while(currProfileID <= DmgProfile::MAX) {
		for (int i = 0; i < this->NUMBER_GENERATIONS_TO_TRACE; i++)
			currPrevalentInfections[currProfileID][i] = _prevalenceByBucket[currProfileID][i];
		currProfileID++;
	}
	for (int i = 0; i< Person::ENDRiskLevel; i++){
		for (int j = 0; j < DmgProfile::ENDGender; j++){
			for (int k = 0; k < DmgProfile::ENDEmployment; k++){
				this->currPrevalentInfectionsRiskGenderEmployment[i][j][k] = _prevalenceByRiskGenderEmployment[i][j][k];
			}
		}
	}

	this->currPrevalentInfectionsAgeMale = _prevalenceByAgeMale;
	this->currPrevalentInfectionsAgeFemale = _prevalenceByAgeFemale;
}

//-----------------< Begin functions to print out infections >-----------------------//

int InfectionsTracker::printInfections(EventParams& _eventParams, long _time, ostream &_outStream, Population *_population) {
	assert (_time >= 0 );

	//total # infections this month
	int totalInfected = 0;
	//current population size
	long currPopSize = _population->getSize();
	long currSAPopSize = currPopSize - _population->getNASize();

	//total # of age ranges to print out
	vector< boost::tuple<long, int, int> > currSizeByAgeRange = _population->getSizeByAgeRange();
	int numAgeRanges = currSizeByAgeRange.size();


	//write headers for infections sheet
	if( _time == 0) {
		ostringstream firstRow;
		ostringstream secondRow;
		ostringstream thirdRow;

		//incidence-related headers
		firstRow << "Epidemiology Outputs" << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << "Month" << Constants::TAB;

		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << "New Infections" << Constants::TAB;

		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << "Total Infected in History (Prevalent Cases Excluded)" << Constants::TAB;

		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << "Currently Infected" << Constants::TAB;

		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << "Pop Size" << Constants::TAB;

		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << "Prevalence" << Constants::TAB;

		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << "SA Pop Size" << Constants::TAB;

		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << "SA Prevalence" << Constants::TAB;

		//write out headers for population by age\sexual activity
		firstRow << "Prevalent Cases" << Constants::TAB;
		secondRow << "Non-Sexually Active Population" << Constants::TAB;
		thirdRow << "All ages" << Constants::TAB;

		for (int i = 0; i < numAgeRanges; i++){
			if (i == 0){
				secondRow << "Sexually Active Population";
			}
			firstRow << Constants::TAB;
			secondRow << Constants::TAB;
			thirdRow << boost::tuples::get<Population::MIN_AGE_IN_MONTHS>(currSizeByAgeRange.at(i)) << "-" << boost::tuples::get<Population::MAX_AGE_IN_MONTHS>(currSizeByAgeRange.at(i))<< Constants::TAB;
		}

		//write out headers for population by gender
		firstRow << "Prevalent Cases" << Constants::TAB << Constants::TAB;
		secondRow << "Gender" << Constants::TAB << Constants::TAB;
		thirdRow << "Males" << Constants::TAB << "Females" << Constants::TAB;

		//write out headers for population by gender and age
		firstRow << "Males" << Constants::TAB;
		secondRow << "Non-Sexually Active Population" << Constants::TAB;
		thirdRow << "All Ages" << Constants::TAB;

		for (int i = 0; i < numAgeRanges; i++){
			if (i == 0){
				secondRow << "Sexually Active Population";
			}
			firstRow << Constants::TAB;
			secondRow << Constants::TAB;
			thirdRow << boost::tuples::get<Population::MIN_AGE_IN_MONTHS>(this->currPrevalentInfectionsAgeMale.at(i)) << "-" << boost::tuples::get<Population::MAX_AGE_IN_MONTHS>(this->currPrevalentInfectionsAgeMale.at(i))<< Constants::TAB;
		}

		firstRow << "Females" << Constants::TAB;
		secondRow << "Non-Sexually Active Population" << Constants::TAB;
		thirdRow << "All Ages" << Constants::TAB;

		for (int i = 0; i < numAgeRanges; i++){
			if (i == 0){
				secondRow << "Sexually Active Population";
			}
			firstRow << Constants::TAB;
			secondRow << Constants::TAB;
			thirdRow << boost::tuples::get<Population::MIN_AGE_IN_MONTHS>(this->currPrevalentInfectionsAgeFemale.at(i)) << "-" << boost::tuples::get<Population::MAX_AGE_IN_MONTHS>(this->currPrevalentInfectionsAgeFemale.at(i))<< Constants::TAB;
		}

		//write out headers for number infected by risk
		firstRow << "Prevalent Cases" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "Risk Group" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << "CSW High Risk" << Constants::TAB << "CSW Low Risk" << Constants::TAB << "Non-CSW High Risk Male" << Constants::TAB << "Non-CSW High Risk Female" << Constants::TAB << "Non-CSW Low Risk Male" << Constants::TAB << "Non-CSW Low Risk Female" << Constants::TAB;

		//write out headers for number of infections by generation (7 tabs)
		firstRow << "Prevalent Cases" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "Number Infected by Generation" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << "0 (Prev.)" << Constants::TAB << "First" << Constants::TAB << "Second" << Constants::TAB << "Third" << Constants::TAB << "Fourth" << Constants::TAB << "Fifth+" << Constants::TAB;

		//write out headers for number of exposures and infections by HVL
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB; 
		secondRow << Constants::TAB << "Exposures by HVL" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << Constants::TAB << "HVL 0-20" << Constants::TAB << "HVL 21-500" << Constants::TAB << "HVL 501-3000" << Constants::TAB << "HVL 3001-10000" << Constants::TAB << "HVL 10000-30000" << Constants::TAB << "HVL 30001-100000" << Constants::TAB << "HVL 100000+" << Constants::TAB << "HVL Primary" << Constants::TAB << "HVL Late Stage" << Constants::TAB;

		firstRow << Constants::TAB << "Incident Cases" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << Constants::TAB << "Infections by HVL (of Infector)" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << Constants::TAB << "HVL 0-20" << Constants::TAB << "HVL 21-500" << Constants::TAB << "HVL 501-3000" << Constants::TAB << "HVL 3001-10000" << Constants::TAB << "HVL 10000-30000" << Constants::TAB << "HVL 30001-100000" << Constants::TAB << "HVL 100000+" << Constants::TAB << "HVL Primary" << Constants::TAB << "HVL Late Stage" << Constants::TAB;

		//write out headers for number of infections by Age and gender
		for (int i = 0; i < numAgeRanges; i++){
			if (i == 0){
				firstRow << "Incident Cases";
				secondRow << "Infections By Age";
			}
			firstRow << Constants::TAB;
			secondRow << Constants::TAB;
			thirdRow << boost::tuples::get<Population::MIN_AGE_IN_MONTHS>(currSizeByAgeRange.at(i)) << "-" << boost::tuples::get<Population::MAX_AGE_IN_MONTHS>(currSizeByAgeRange.at(i))<< Constants::TAB;
		}

		firstRow << "Incident Cases" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "Gender" << Constants::TAB << Constants::TAB << "Risk Group" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << "Male" << Constants::TAB << "Female" << Constants::TAB << "CSW High Risk" << Constants::TAB << "CSW Low Risk" << Constants::TAB << "Non-CSW High Risk Male" << Constants::TAB << "Non-CSW High Risk Female" << Constants::TAB << "Non-CSW Low Risk Male" << Constants::TAB << "Non-CSW Low Risk Female" << Constants::TAB;

		for (int i = 0; i < numAgeRanges; i++){
			if (i == 0){
				firstRow << "Incident Cases";
				secondRow << "Male (By Age)";
			}
			firstRow << Constants::TAB;
			secondRow << Constants::TAB;
			thirdRow << boost::tuples::get<Population::MIN_AGE_IN_MONTHS>(currSizeByAgeRange.at(i)) << "-" << boost::tuples::get<Population::MAX_AGE_IN_MONTHS>(currSizeByAgeRange.at(i))<< Constants::TAB;
		}

		for (int i = 0; i < numAgeRanges; i++){
			if (i == 0){
				firstRow << "Incident Cases";
				secondRow << "Female (By Age)";
			}
			firstRow << Constants::TAB;
			secondRow << Constants::TAB;
			thirdRow << boost::tuples::get<Population::MIN_AGE_IN_MONTHS>(currSizeByAgeRange.at(i)) << "-" << boost::tuples::get<Population::MAX_AGE_IN_MONTHS>(currSizeByAgeRange.at(i))<< Constants::TAB;
		}

		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << Constants::TAB;

		//write out headers for incident infections by relationship type
		for (int i = 0; i < SexualPartnership::ENDType; i++){
			if (i == 0){
				firstRow << "Total Infected in History (Prevalent Cases Excluded)";
				secondRow << "Relationship Type";
			}
			firstRow << Constants::TAB;
			secondRow << Constants::TAB;
			thirdRow << *(SexualPartnership::TypeEnum.toString(i)) << Constants::TAB;
		}

		firstRow << Constants::TAB;
		secondRow << "Infectors:" << Constants::TAB;
		thirdRow << "Infecteds:" << Constants::TAB;

		//write out headers that tally infections from one DmgProfile to another
		//loop through all used ProfileID's and create internal string buffer headers for future timestep trace output
		list<DmgProfile::ProfileID>::iterator infectorProfileID = this->profileIDsForDetailedTrace.begin();
		while(infectorProfileID != this->profileIDsForDetailedTrace.end()) {
			//first row profile str refers to infectors
			firstRow << "Total Infected in History (Prevalent Cases Excluded)";
			secondRow << *DmgProfile::toString(*infectorProfileID);
			list<DmgProfile::ProfileID>::iterator infectedProfileID = this->profileIDsForDetailedTrace.begin();
			while(infectedProfileID != this->profileIDsForDetailedTrace.end()) {
				firstRow << Constants::TAB;
				secondRow << Constants::TAB;
				//second row profile str refers to infecteds
				thirdRow << *DmgProfile::toString(*infectedProfileID) << Constants::TAB;
				infectedProfileID++;
			} //while(infectorProfileID < this->profileIDsForDetailedTrace.end()) {
			infectorProfileID++;
		} //while(infectedProfileID < this->profileIDsForDetailedTrace.end()) {

		//write out headers that tally total infections based on age and gender and risk group
		for (int i = 0; i < numAgeRanges; i++){
			if (i == 0){
				firstRow << "Total Infected in History (Prevalent Cases Excluded)";
				secondRow << "Infections By Age";
			}
			firstRow << Constants::TAB;
			secondRow << Constants::TAB;
			thirdRow << boost::tuples::get<Population::MIN_AGE_IN_MONTHS>(currSizeByAgeRange.at(i)) << "-" << boost::tuples::get<Population::MAX_AGE_IN_MONTHS>(currSizeByAgeRange.at(i))<< Constants::TAB;
		}

		firstRow << "Total Infected in History (Prevalent Cases Excluded)" << Constants::TAB << Constants::TAB;
		secondRow << "Gender" << Constants::TAB << Constants::TAB;
		thirdRow << "Male" << Constants::TAB << "Female" << Constants::TAB;

		firstRow << "Total Infected in History (Prevalent Cases Excluded)" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "Risk Group" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << "CSW High Risk" << Constants::TAB << "CSW Low Risk" << Constants::TAB << "Non-CSW High Risk Male" << Constants::TAB << "Non-CSW High Risk Female" << Constants::TAB << "Non-CSW Low Risk Male" << Constants::TAB << "Non-CSW Low Risk Female" << Constants::TAB;

		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << Constants::TAB;

		firstRow << "Age at Infection" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "Male" << Constants::TAB << Constants::TAB << "Female" << Constants::TAB << Constants::TAB;
		thirdRow << "Mean" << Constants::TAB << "SD" << Constants::TAB << "Mean" << Constants::TAB << "SD" << Constants::TAB;

		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "CSW" << Constants::TAB << Constants::TAB << "CSW High Risk" << Constants::TAB << Constants::TAB << "CSW Low Risk"  << Constants::TAB << Constants::TAB;
		thirdRow << "Mean" << Constants::TAB << "SD" << Constants::TAB << "Mean" << Constants::TAB << "SD" << Constants::TAB << "Mean" << Constants::TAB << "SD" << Constants::TAB;

		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "Non-CSW High Risk Male" << Constants::TAB << Constants::TAB << "Non-CSW High Risk Female" << Constants::TAB << Constants::TAB << "Non-CSW Low Risk Male"  << Constants::TAB << Constants::TAB << "Non-CSW Low Risk Female"  << Constants::TAB << Constants::TAB;
		thirdRow << "Mean" << Constants::TAB << "SD" << Constants::TAB << "Mean" << Constants::TAB << "SD" << Constants::TAB << "Mean" << Constants::TAB << "SD" << Constants::TAB << "Mean" << Constants::TAB << "SD" << Constants::TAB;



		//write out string buffers to trace file
		_outStream << firstRow.str() << endl;
		_outStream << secondRow.str() << endl;
		_outStream << thirdRow.str() << endl;

	} //if( _time == 0) {


	//if no incident infections happened during this time, then make sure that we have 0 in the currTime incident infections and exposures
	if(_time > this->currTimeStep) {
		resetIncidentInfections(_time);
	}

	//get current total infections
	//Month
	if (_time == 0)
		_outStream << "init" << Constants::TAB;
	else
		_outStream << _time << Constants::TAB;
	//New Infections
	_outStream << this->getCurrTimeStepIncidentInfsTotal() << Constants::TAB;

	//Print to BatchStats file if NEWINFECTIONS stream is open
	if (_eventParams.BatchStatsStream[NEWINFECTIONS].is_open())
		_eventParams.BatchStatsStream[NEWINFECTIONS] << this->getCurrTimeStepIncidentInfsTotal() << Constants::TAB;

	double monthlyIncidence = 1.0 * (this->getCurrTimeStepIncidentInfsTotal())/(double)currPopSize;
	//Push the monthly incidence onto the deque
	this->lastTwelveIncidenceRates.push_back(monthlyIncidence);
	//Pop off the oldest incidence rate
	this->lastTwelveIncidenceRates.pop_front();
	assert(this->lastTwelveIncidenceRates.size() == 12);

	//Calculate yearly incidence rate
	double incidence = this->calculateAnnualIncidence();
	//Print to BatchStats file if INCIDENCE stream is open
	if (_eventParams.BatchStatsStream[INCIDENCE].is_open())
		_eventParams.BatchStatsStream[INCIDENCE] << incidence << Constants::TAB;
#if !defined ( CONSOLE )
	//Send incidence to incidence widget -- Should be in halves of a percent (* 200)
	_eventParams.displaybox->currentIncidence = (int) (200.0 * incidence);
#endif

	//Total Infected in History
	_outStream << this->getNumIncidentInfections() << Constants::TAB;

	//Currently Infected
	//total the current infections
	for(int i = 0; i < DmgProfile::TotalNumBuckets; i++) {
		for (int j = 0; j < this->NUMBER_GENERATIONS_TO_TRACE; j++)
			totalInfected += this->currPrevalentInfections[i][j];
	}
	_outStream << totalInfected << Constants::TAB;
	//Print to BatchStats file if CURRENTLYINFECTED stream is open
	if (_eventParams.BatchStatsStream[CURRENTLYINFECTED].is_open())
		_eventParams.BatchStatsStream[CURRENTLYINFECTED] << totalInfected << Constants::TAB;

	//Total Population Size
	_outStream << currPopSize << Constants::TAB;
	//Print to BatchStats file if POPULATION stream is open
	if(_eventParams.BatchStatsStream[POPULATION].is_open())
		_eventParams.BatchStatsStream[POPULATION] << currPopSize << Constants::TAB;

	//current prevalence
	double currPrevalence = totalInfected/(double)currPopSize;
	_outStream << currPrevalence << Constants::TAB;
	//Print to BatchStats file if PREVALENCE stream is open
	if (_eventParams.BatchStatsStream[PREVALENCE].is_open())
		_eventParams.BatchStatsStream[PREVALENCE] << currPrevalence << Constants::TAB;

	//current pop and prevalence of sexually active population
	//Need total number of infected for SA population only
	int totalInfectedSA = totalInfected;
	DmgProfile NAProfile;
	NAProfile.set(DmgProfile::SEXUAL_ACTIVITY_STATUS, DmgProfile::NA);
	vector<DmgProfile::ProfileID> NAProfileIDs;
	NAProfile.selectProfileIDs(NAProfileIDs,NULL);
	for (int i = 0; i < NAProfileIDs.size(); i++){
		for (int j = 0; j < this->NUMBER_GENERATIONS_TO_TRACE; j++)
			totalInfectedSA -= this->currPrevalentInfections[NAProfileIDs[i]][j];
	}

	double currPrevalenceSA = (totalInfectedSA)/(double)currSAPopSize;
	_outStream << currSAPopSize << Constants::TAB;
	_outStream << currPrevalenceSA << Constants::TAB;
	//Print up a batchstats file!
	if (_eventParams.BatchStatsStream[PREVALENCESA].is_open())
		_eventParams.BatchStatsStream[PREVALENCESA] << currPrevalenceSA << Constants::TAB;

	_population->popStats->recordPrevalenceAndIncidence(_time, currPrevalence, currPrevalenceSA, incidence);

	//Multiply by 100 and round to nearest integer for graphical output
	int intPrevalence = (int) (100 * currPrevalence + 0.5);

	//prev cases by age\sexual activity
	int totalInfectedNA = totalInfected - totalInfectedSA;
	_outStream << totalInfectedNA << Constants::TAB;

	for (int i = 0; i < numAgeRanges; i++){
		_outStream << boost::tuples::get<Population::AGE_RANGE_SIZE>(this->currPrevalentInfectionsAgeMale.at(i)) + boost::tuples::get<Population::AGE_RANGE_SIZE>(this->currPrevalentInfectionsAgeFemale.at(i)) << Constants::TAB;
	}

	//Print out population size and number infected by gender
	int totalInfectedGender[DmgProfile::ENDGender];
	int totalInfectedSAGender[DmgProfile::ENDGender];
	totalInfectedGender[DmgProfile::MALE] = 0;
	totalInfectedSAGender[DmgProfile::MALE] = 0;
	totalInfectedGender[DmgProfile::FEMALE] = 0;
	totalInfectedSAGender[DmgProfile::FEMALE] = 0;
	DmgProfile GenderProfile;

	for (int i = 0; i < numAgeRanges; i++){
		totalInfectedSAGender[DmgProfile::MALE] += boost::tuples::get<Population::AGE_RANGE_SIZE>(this->currPrevalentInfectionsAgeMale.at(i));
		totalInfectedSAGender[DmgProfile::FEMALE] += boost::tuples::get<Population::AGE_RANGE_SIZE>(this->currPrevalentInfectionsAgeFemale.at(i));
	}

	//First tally the infected men
	GenderProfile.set(DmgProfile::GENDER, DmgProfile::MALE);
	vector<DmgProfile::ProfileID> GenderProfileIDs;
	GenderProfile.selectProfileIDs(GenderProfileIDs, NULL);
	for (int i = 0; i < GenderProfileIDs.size(); i++){
		for (int j = 0; j < this->NUMBER_GENERATIONS_TO_TRACE; j++){
			totalInfectedGender[DmgProfile::MALE] += this->currPrevalentInfections[GenderProfileIDs[i]][j];
		}
	}

	//Next tally the infected women
	GenderProfile.set(DmgProfile::GENDER, DmgProfile::FEMALE);
	GenderProfileIDs.clear();
	GenderProfile.selectProfileIDs(GenderProfileIDs, NULL);
	for (int i = 0; i < GenderProfileIDs.size(); i++){
		for (int j = 0; j < this->NUMBER_GENERATIONS_TO_TRACE; j++)
			totalInfectedGender[DmgProfile::FEMALE] += this->currPrevalentInfections[GenderProfileIDs[i]][j];
	}

	//Actually print the size and infections by gender
	_outStream << totalInfectedGender[DmgProfile::MALE] << Constants::TAB << totalInfectedGender[DmgProfile::FEMALE] << Constants::TAB;

	//output infections by age and gender
	_outStream << totalInfectedGender[DmgProfile::MALE] - totalInfectedSAGender[DmgProfile::MALE] << Constants::TAB;
	for (int i = 0; i < numAgeRanges; i++){
		_outStream << boost::tuples::get<Population::AGE_RANGE_SIZE>(this->currPrevalentInfectionsAgeMale.at(i)) << Constants::TAB;
	}

	_outStream << totalInfectedGender[DmgProfile::FEMALE] - totalInfectedSAGender[DmgProfile::FEMALE] << Constants::TAB;
	for (int i = 0; i < numAgeRanges; i++){
		_outStream << boost::tuples::get<Population::AGE_RANGE_SIZE>(this->currPrevalentInfectionsAgeFemale.at(i)) << Constants::TAB;
	}

	//output prevalent infections by risk
	int totalInfectedCSW = 0;
	DmgProfile CSWProfile;
	CSWProfile.set(DmgProfile::EMPLOYMENT, DmgProfile::CSW);
	vector<DmgProfile::ProfileID> CSWProfileIDs;
	CSWProfile.selectProfileIDs(CSWProfileIDs,NULL);
	for (int i = 0; i < CSWProfileIDs.size(); i++){
		for (int j = 0; j < this->NUMBER_GENERATIONS_TO_TRACE; j++)
			totalInfectedCSW += this->currPrevalentInfections[CSWProfileIDs[i]][j];
	}
	_outStream << this->currPrevalentInfectionsRiskGenderEmployment[Person::HIGH][DmgProfile::MALE][DmgProfile::CSW] + this->currPrevalentInfectionsRiskGenderEmployment[Person::HIGH][DmgProfile::FEMALE][DmgProfile::CSW] << Constants::TAB << this->currPrevalentInfectionsRiskGenderEmployment[Person::LOW][DmgProfile::MALE][DmgProfile::CSW] + this->currPrevalentInfectionsRiskGenderEmployment[Person::LOW][DmgProfile::FEMALE][DmgProfile::CSW] << Constants::TAB << this->currPrevalentInfectionsRiskGenderEmployment[Person::HIGH][DmgProfile::MALE][DmgProfile::NON_CSW] << Constants::TAB << this->currPrevalentInfectionsRiskGenderEmployment[Person::HIGH][DmgProfile::FEMALE][DmgProfile::NON_CSW]<< Constants::TAB << this->currPrevalentInfectionsRiskGenderEmployment[Person::LOW][DmgProfile::MALE][DmgProfile::NON_CSW] << Constants::TAB << this->currPrevalentInfectionsRiskGenderEmployment[Person::LOW][DmgProfile::FEMALE][DmgProfile::NON_CSW] << Constants::TAB;

	//We're hard wiring 6 (Prev + 5) generations of reporting for now
	int totalInfectedByGeneration[NUMBER_GENERATIONS_TO_TRACE];
	for (int i = 0; i < this->NUMBER_GENERATIONS_TO_TRACE; i++){
		totalInfectedByGeneration[i] = 0;
		for (int j = 0; j < DmgProfile::TotalNumBuckets; j++){
			totalInfectedByGeneration[i] += this->currPrevalentInfections[j][i];
		}
	}
	//write out number of infections by generation (7 tabs)
	for (int i = 0; i < this->NUMBER_GENERATIONS_TO_TRACE; i++){
		_outStream << totalInfectedByGeneration[i] << Constants::TAB;
	}

	/** Print out exposures and infections by viral load */
	_outStream << Constants::TAB;
	for (int i = 0; i < Person::ENDHVLStrata; i++)
		_outStream << this->currTimeExposures[i] << Constants::TAB;
	_outStream << Constants::TAB;
	for (int i = 0; i < Person::ENDHVLStrata; i++)
		_outStream << this->currTimeStepIncidentInfs[i] << Constants::TAB;

	//Print out incident infections by age and gender and risk
	int incidentInfsMale = 0;
	int incidentInfsFemale = 0;
	for (int i = 0; i < numAgeRanges; i++){
		_outStream << boost::tuples::get<Population::AGE_RANGE_SIZE>(this->currTimeStepIncidentInfsAgeMale.at(i))+ boost::tuples::get<Population::AGE_RANGE_SIZE>(this->currTimeStepIncidentInfsAgeFemale.at(i))<< Constants::TAB;
		incidentInfsMale+=boost::tuples::get<Population::AGE_RANGE_SIZE>(this->currTimeStepIncidentInfsAgeMale.at(i));
		incidentInfsFemale+=boost::tuples::get<Population::AGE_RANGE_SIZE>(this->currTimeStepIncidentInfsAgeFemale.at(i));
	}
	_outStream << incidentInfsMale << Constants::TAB << incidentInfsFemale << Constants::TAB;
	_outStream << this->currTimeStepIncidentInfsRiskGenderEmployment[Person::HIGH][DmgProfile::MALE][DmgProfile::CSW] + this->currTimeStepIncidentInfsRiskGenderEmployment[Person::HIGH][DmgProfile::FEMALE][DmgProfile::CSW] << Constants::TAB << this->currTimeStepIncidentInfsRiskGenderEmployment[Person::LOW][DmgProfile::MALE][DmgProfile::CSW] + this->currTimeStepIncidentInfsRiskGenderEmployment[Person::LOW][DmgProfile::FEMALE][DmgProfile::CSW] << Constants::TAB << this->currTimeStepIncidentInfsRiskGenderEmployment[Person::HIGH][DmgProfile::MALE][DmgProfile::NON_CSW] << Constants::TAB << this->currTimeStepIncidentInfsRiskGenderEmployment[Person::HIGH][DmgProfile::FEMALE][DmgProfile::NON_CSW]<< Constants::TAB << this->currTimeStepIncidentInfsRiskGenderEmployment[Person::LOW][DmgProfile::MALE][DmgProfile::NON_CSW] << Constants::TAB << this->currTimeStepIncidentInfsRiskGenderEmployment[Person::LOW][DmgProfile::FEMALE][DmgProfile::NON_CSW] << Constants::TAB;

	for (int i = 0; i < numAgeRanges; i++){
		_outStream << boost::tuples::get<Population::AGE_RANGE_SIZE>(this->currTimeStepIncidentInfsAgeMale.at(i)) << Constants::TAB;
	}
	for (int i = 0; i < numAgeRanges; i++){
		_outStream << boost::tuples::get<Population::AGE_RANGE_SIZE>(this->currTimeStepIncidentInfsAgeFemale.at(i))<< Constants::TAB;
	}

	// Print out incident infections by relationship type
	_outStream << Constants::TAB;
	for (int partnershipType = 0; partnershipType < SexualPartnership::ENDType; partnershipType++){
		int infectionsByType = 0;
		//tally all the infections that happened with partnershipType for all possible infector and infected DmgProfiles
		list<DmgProfile::ProfileID>::iterator infectorProfileID = this->profileIDsForDetailedTrace.begin();
			while(infectorProfileID != this->profileIDsForDetailedTrace.end()) {
				list<DmgProfile::ProfileID>::iterator infectedProfileID = this->profileIDsForDetailedTrace.begin();
				while(infectedProfileID != this->profileIDsForDetailedTrace.end()) {
					infectionsByType += this->incidentInfections[partnershipType][*infectorProfileID][*infectedProfileID];
					infectedProfileID++;
				} //while(infectorProfileID < this->profileIDsForDetailedTrace.end()) {
				infectorProfileID++;
			} //while(infectedProfileID < this->profileIDsForDetailedTrace.end()) {
		_outStream << infectionsByType << Constants::TAB;
	}

	_outStream << Constants::TAB;


	//write out all incident infections that happened in history
	// only ProfileID's in profileIDsForDetailedTrace are included
	list<DmgProfile::ProfileID>::iterator infectorProfileID = this->profileIDsForDetailedTrace.begin();
	while(infectorProfileID != this->profileIDsForDetailedTrace.end()) {
		list<DmgProfile::ProfileID>::iterator infectedProfileID = this->profileIDsForDetailedTrace.begin();
		while(infectedProfileID != this->profileIDsForDetailedTrace.end()) {
			//tally all the infections that happened from curr infectorProfileID -> curr infectedProfileID
			unsigned long infs = 0;
			for(int partnershipType = 0; partnershipType < SexualPartnership::ENDType; ++partnershipType) {
				infs += this->incidentInfections[partnershipType][*infectorProfileID][*infectedProfileID];
			}
			_outStream << infs << Constants::TAB;
			infectedProfileID++;
		} //while(infectorProfileID < this->profileIDsForDetailedTrace.end()) {
		infectorProfileID++;
	} //while(infectedProfileID < this->profileIDsForDetailedTrace.end()) {

	//write out all incident infections that happened in history stratified by age and gender and risk
	for (int i = 0; i < numAgeRanges; i++){
		_outStream << boost::tuples::get<Population::AGE_RANGE_SIZE>(this->totalIncidentInfsAge.at(i)) << Constants::TAB;
	}
	_outStream << this->totalIncidentInfsGender[DmgProfile::MALE] << Constants::TAB << this->totalIncidentInfsGender[DmgProfile::FEMALE] << Constants::TAB;
	_outStream << this->totalIncidentInfsRiskGenderEmployment[Person::HIGH][DmgProfile::MALE][DmgProfile::CSW] + this->totalIncidentInfsRiskGenderEmployment[Person::HIGH][DmgProfile::FEMALE][DmgProfile::CSW] << Constants::TAB << this->totalIncidentInfsRiskGenderEmployment[Person::LOW][DmgProfile::MALE][DmgProfile::CSW] + this->totalIncidentInfsRiskGenderEmployment[Person::LOW][DmgProfile::FEMALE][DmgProfile::CSW] << Constants::TAB << this->totalIncidentInfsRiskGenderEmployment[Person::HIGH][DmgProfile::MALE][DmgProfile::NON_CSW] << Constants::TAB << this->totalIncidentInfsRiskGenderEmployment[Person::HIGH][DmgProfile::FEMALE][DmgProfile::NON_CSW]<< Constants::TAB << this->totalIncidentInfsRiskGenderEmployment[Person::LOW][DmgProfile::MALE][DmgProfile::NON_CSW] << Constants::TAB << this->totalIncidentInfsRiskGenderEmployment[Person::LOW][DmgProfile::FEMALE][DmgProfile::NON_CSW] << Constants::TAB;

	//write out age of infection for incident infections that month (mean and SD)
	_outStream << Constants::TAB;

	for (int i = 0; i < DmgProfile::ENDGender; i++){
		if (this->currTimeStepNumInfectedGender[i] != 0){
			double ageMean = this->currTimeStepAgeInfectionSumGender[i]/(double) this->currTimeStepNumInfectedGender[i];
			double ageSD = sqrt(this->currTimeStepAgeInfectionSumSqGender[i]/(double) this->currTimeStepNumInfectedGender[i] - ageMean*ageMean);
			_outStream << ageMean << Constants::TAB << ageSD << Constants::TAB;
		}
		else{
			_outStream << "N/A" << Constants::TAB << "N/A" << Constants::TAB;
		}
	}
	
	double numInfectedCSW = 0;
	for (int i = 0; i < Person::ENDRiskLevel; i++){
		for (int j = 0; j < DmgProfile::ENDGender; j++){
			numInfectedCSW += this->currTimeStepNumInfectedRiskGenderEmployment[i][j][DmgProfile::CSW];
		}
	}
	if (numInfectedCSW != 0){
		double ageSumCSW  = 0;
		double ageSumSqCSW = 0;
		for (int i = 0; i < Person::ENDRiskLevel; i++){
			for (int j = 0; j < DmgProfile::ENDGender; j++){
				ageSumCSW += this->currTimeStepAgeInfectionSumRiskGenderEmployment[i][j][DmgProfile::CSW];
				ageSumSqCSW += this->currTimeStepAgeInfectionSumSqRiskGenderEmployment[i][j][DmgProfile::CSW];
			}
		}
		double ageMeanCSW = ageSumCSW/numInfectedCSW;
		double ageSDCSW = sqrt(ageSumSqCSW/numInfectedCSW - ageMeanCSW * ageMeanCSW);
		_outStream << ageMeanCSW << Constants::TAB << ageSDCSW << Constants::TAB;
	}
	else{
		_outStream << "N/A" << Constants::TAB << "N/A" << Constants::TAB;
	}

	for (int i = Person::ENDRiskLevel-1; i >= 0; i--){
		double numInfected = 0;
		for (int j = 0; j < DmgProfile::ENDGender; j++){
			numInfected += this->currTimeStepNumInfectedRiskGenderEmployment[i][j][DmgProfile::CSW];
		}
		if (numInfected != 0){
			double ageSum  = 0;
			double ageSumSq = 0;
			for (int j = 0; j < DmgProfile::ENDGender; j++){
				ageSum += this->currTimeStepAgeInfectionSumRiskGenderEmployment[i][j][DmgProfile::CSW];
				ageSumSq += this->currTimeStepAgeInfectionSumSqRiskGenderEmployment[i][j][DmgProfile::CSW];
			}
			double ageMean = ageSum/numInfected;
			double ageSD = sqrt(ageSumSq/numInfected - ageMean*ageMean);
			_outStream << ageMean << Constants::TAB << ageSD << Constants::TAB;
		}
		else{
			_outStream << "N/A" << Constants::TAB << "N/A" << Constants::TAB;
		}
	}

	for (int i = Person::ENDRiskLevel-1; i >= 0; i--){
		for (int j = 0; j < DmgProfile::ENDGender; j++){
			double numInfected = this->currTimeStepNumInfectedRiskGenderEmployment[i][j][DmgProfile::NON_CSW];
			if (numInfected != 0){
				double ageSum  = this->currTimeStepAgeInfectionSumRiskGenderEmployment[i][j][DmgProfile::NON_CSW];
				double ageSumSq = this->currTimeStepAgeInfectionSumSqRiskGenderEmployment[i][j][DmgProfile::NON_CSW];

				double ageMean = ageSum/numInfected;
				double ageSD = sqrt(ageSumSq/numInfected - ageMean*ageMean);
				_outStream << ageMean << Constants::TAB << ageSD << Constants::TAB;
			}
			else{
				_outStream << "N/A" << Constants::TAB << "N/A" << Constants::TAB;
			}
		}
	}


	_outStream << endl;

	return intPrevalence;
}
