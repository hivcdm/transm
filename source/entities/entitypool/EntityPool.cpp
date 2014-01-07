/**
This file contains the implementations for the methods of EntityPool
***/

#include <sstream>
using namespace std;


#include "EntityPool.h"

#include "./../../Constants.h"
#include "./../../util/Util.h"
#include "./../Person.h"
#include "./bucket/BucketSexualMixing.h"


//-------------< Begin methods for EntityPool >-------------------//

bool EntityPool::addEntity(Person *_person) {
	//gets the DmgProfileBucket that this person is supposed to be a part of based on their DmgProfile
	DmgProfileBucket *bucket = this->entityBuckets->at(_person->getDmgProfile()->getProfileID());
	assert(bucket != NULL);
	//bucket's insert method should take care of the _person->setCurrBucketProfileID(...)
	return bucket->insert(_person);
}

DmgProfileBucket* EntityPool::getBucket(DmgProfile::ProfileID _profileID) {
	return this->entityBuckets->at(_profileID);
}

void EntityPool::print(ostream& _outStream) {
	DmgProfileBucket *bucket = NULL;

	//iterate through all buckets
	int currBucketIndex = 0;
	while(currBucketIndex < this->entityBuckets->size()) {
		bucket = this->entityBuckets->at(currBucketIndex);
		//if bucket == NULL, that means we are not using this particular DmgProfile during this simulation
		if( bucket == NULL) {
			currBucketIndex++;
			continue;
		}
		_outStream << *(bucket->getLabel()) << Constants::COLON << Constants::TAB << Constants::TAB << "Gender" << Constants::TAB << "ID" << Constants::TAB << "Profile" << Constants::TAB << "Age" << Constants::TAB << "cd4" << Constants::TAB << "hvl" << endl;
		//print out all members
		bucket->print(_outStream, Constants::TABTAB);
		_outStream << endl;
		currBucketIndex++;
	} //while(currBucketIndex < this->entityBuckets->size()) {
}

//print out all the labels of all the Buckets in the EntityPool. separate each by TAB
//if _printPropInfected == true, then include a column for #infected for each DmgProfileBucket
void EntityPool::printBucketLabels(ostream& _outStream, bool _printPropInfected) {
	DmgProfileBucket *bucket = NULL;
	int currBucketIndex = 0;

	//iterate through all buckets
	while(currBucketIndex < this->entityBuckets->size()) {
		bucket = this->entityBuckets->at(currBucketIndex);
		//if bucket == NULL, that means we are not using this particular DmgProfile during this sim
		if( bucket == NULL) {
			currBucketIndex++;
			 continue;
		}
		if(_printPropInfected)
			_outStream << *(bucket->getLabel()) << " (Infected)" << Constants::TAB;
		_outStream << (*bucket->getLabel()) << " (Total)" << Constants::TAB;
		if( DmgProfile::get(bucket->getProfileID(), DmgProfile::SEXUAL_ACTIVITY_STATUS) == DmgProfile::SA){
			_outStream << (*bucket->getLabel()) << " (HR)" << Constants::TAB;
			_outStream << (*bucket->getLabel()) << " (LR)" << Constants::TAB;
		}
		currBucketIndex++;
	} //while(currBucketIndex < this->entityBuckets->size()) {
}

//list out # people in each DmgProfileBucket
void EntityPool::printBucketSizes(ostream& _outStream, string _prefix, bool _printPropInfected, unsigned long &_totalInfected, unsigned long &_totalSize, unsigned long &_totalSexuallyActive,unsigned long &_totalInSteady, unsigned long &_totalInRegular,bool _includeLabels) {
	_totalInfected = 0;
	_totalSize = 0;
	_totalInSteady=0;
	_totalInRegular=0;
	_totalSexuallyActive=0;

	DmgProfileBucket *bucket = NULL;	//pointer to current DmgProfileBucket we are looking at

	int currBucketIndex = 0;		//the ProfileID of the current DmgProfileBucket we are looking at
	//iterate through all buckets and append current DmgProfileBucket sizes to a string buffer
	while(currBucketIndex < this->entityBuckets->size()) {
		bucket = this->entityBuckets->at(currBucketIndex);
		if( bucket == NULL) {
			currBucketIndex++;
			continue;
		}

		//will include the label on the same line if _includeLabels == true
		//used for high level debug traces
		if(_includeLabels)
			_outStream << *(bucket->getLabel()) << ":";

		//# people infected in current DmgProfileBucket
		long numInfected = bucket->getNumInfected();
		//if _printPropInfected == true, print out number of infected folk in the DmgProfileBucket
		if(_printPropInfected) {
			_outStream << numInfected;
			//separate this value by TAB if _includeLabels == false, else separate by '/'
			if(_includeLabels)
				_outStream << "/";
			else
				_outStream << Constants::TAB;
		}

		//print out # people in current DmgProfileBucket
		long bucketSize = bucket->size();
		_outStream << bucketSize << Constants::TAB;
		if( DmgProfile::get(bucket->getProfileID(), DmgProfile::SEXUAL_ACTIVITY_STATUS) == DmgProfile::SA){
			long bucketSizeHR = ((BucketSexualMixing *) bucket)->sizeRisk(Person::HIGH);
			long bucketSizeLR = ((BucketSexualMixing *) bucket)->sizeRisk(Person::LOW);
			_outStream << bucketSizeHR << Constants::TAB << bucketSizeLR << Constants::TAB;
		}

		_totalSize += bucketSize;
		_totalInfected += numInfected;
		currBucketIndex++;
	} //while(currBucketIndex < this->entityBuckets->size()) {

	for(list<Person *>::iterator maleIter=this->allMales.begin();maleIter!=this->allMales.end();maleIter++){
		Person * male=*maleIter;
		if(male->getDmgProfile()->get(DmgProfile::SEXUAL_ACTIVITY_STATUS)==DmgProfile::SA){
			_totalSexuallyActive++;
		}
		if(male->hasPartnership(SexualPartnership::STEADY)){
			_totalInSteady++;
		}
		if(male->hasPartnership(SexualPartnership::REGULAR)){
			_totalInRegular++;
		}
	}
	for(list<Person *>::iterator femaleIter=this->allFemales.begin();femaleIter!=this->allFemales.end();femaleIter++){
		Person * female=*femaleIter;
		if(female->getDmgProfile()->get(DmgProfile::SEXUAL_ACTIVITY_STATUS)==DmgProfile::SA){
			_totalSexuallyActive++;
		}
		if(female->hasPartnership(SexualPartnership::STEADY)){
			_totalInSteady++;
		}
		if(female->hasPartnership(SexualPartnership::REGULAR)){
			_totalInRegular++;
		}
	}
}

bool EntityPool::removeEntity(Person *_person)
{
	assert(_person != NULL);

	bool removed = false;

	//bucket will take care of the _person->setCurrBucketProfileID(DmgProfile::END)
	DmgProfile::ProfileID personProfID = _person->getCurrBucketProfileID();

	if (personProfID >= DmgProfile::END)
	{
		removed = false;
	}
	else
	{
		try
		{
			removed = getBucket(personProfID)->erase(_person);
		}
		catch (std::out_of_range &e)
		{

			std::cerr << "Trying to remove person from DMG Profile Bucket resulted in an out of range exception: " << e.what() << "\n";
			std::cerr << "If this person is of maximum age, they were probably already removed and you can disregard this message." << endl;
			_person->print(cerr, "Person attempted to remove: ");
		}
		catch (std::exception &e)
		{
			std::cerr << "Trying to remove person from DMG Profile Bucket resulted in an exception: " << e.what() << "\n";
		}
	}

	return removed;
}


bool EntityPool::refreshDmgProfileBucket(Person *_person, list<Person*>::iterator */*_p_Iter*/, bool forceRefresh)
{
	assert(_person != NULL);

	bool success = false;

	if (!forceRefresh)
	{
		//Don't want to waste time calling this if person is just going to be removed from and re-added to the same bucket
		assert(_person->getCurrBucketProfileID() != _person->getDmgProfile()->getProfileID());
	}

	try
	{
		success = this->removeEntity(_person);
	}
	catch(std::out_of_range &e)
	{
		std::cout << "Trying to remove a person threw an out of range error (current profile ID is DmgProfile::END?): " << e.what() << "\n";
	}
	catch(std::exception &e)
	{
		std::cout << "Trying to remove a person threw an exception: " << e.what() << "\n";
	}

	return success && this->addEntity(_person);
}

unsigned long EntityPool::size() {
	DmgProfileBucket *bucket = NULL;
	unsigned long size = 0;		//total of the zie
	int currBucketIndex = 0;

	//iterate through all buckets
	while(currBucketIndex < this->entityBuckets->size()) {
		bucket = this->entityBuckets->at(currBucketIndex);
		//if bucket == NULL, that means we are not using this particular DmgProfile during this sim
		if( bucket == NULL) {
			currBucketIndex++;
			continue;
		}
		size+= this->entityBuckets->at(currBucketIndex)->size();
		currBucketIndex++;
	}	//while(currBucketIndex < this->entityBuckets->size()) {

	return size;
}

//calculate the current number of persons with a given DmgProfile ID
unsigned long EntityPool::size(DmgProfile::ProfileID _profileID){
	DmgProfileBucket *bucket = this->getBucket(_profileID);
	if ( bucket == NULL ){
		return 0;
	}
	else {
		return bucket->size();
	}
}

//calculate the current number of persons that are not sexually active in the entity pool
unsigned long EntityPool::sizeNotSexuallyActive(){
	DmgProfileBucket *bucket = NULL;
	unsigned long size = 0;		//total of the zie
	int currBucketIndex = 0;

	//iterate through all buckets
	while(currBucketIndex < this->entityBuckets->size()) {
		bucket = this->entityBuckets->at(currBucketIndex);
		//if bucket == NULL, that means we are not using this particular DmgProfile during this sim
		if( bucket == NULL) {
			currBucketIndex++;
			continue;
		}
		//Only add the sizes of non-sexually active buckets
		if( DmgProfile::get(bucket->getProfileID(), DmgProfile::SEXUAL_ACTIVITY_STATUS) == DmgProfile::NA){
			size+= this->entityBuckets->at(currBucketIndex)->size();
		}
		currBucketIndex++;
	}	//while(currBucketIndex < this->entityBuckets->size()) {

	return size;
}

//calculate the current number of persons that are not sexually active in the entity pool with a given demographic
unsigned long EntityPool::sizeNotSexuallyActive(DmgProfile::Gender _gender){
	DmgProfileBucket *bucket = NULL;
	unsigned long size = 0;		//total of the zie
	int currBucketIndex = 0;

	//iterate through all buckets
	while(currBucketIndex < this->entityBuckets->size()) {
		bucket = this->entityBuckets->at(currBucketIndex);
		//if bucket == NULL, that means we are not using this particular DmgProfile during this sim
		if( bucket == NULL) {
			currBucketIndex++;
			continue;
		}
		//Only add the sizes of non-sexually active buckets that match demographic profile
		if( DmgProfile::get(bucket->getProfileID(), DmgProfile::SEXUAL_ACTIVITY_STATUS) == DmgProfile::NA){
			if(DmgProfile::get(bucket->getProfileID(), DmgProfile::GENDER) == _gender)
			{
				size+= this->entityBuckets->at(currBucketIndex)->size();
			}
		}
		currBucketIndex++;
	}	//while(currBucketIndex < this->entityBuckets->size()) {

	return size;
}

//calculate the current number of persons that are sexually active in the entity pool with a given demographic
unsigned long EntityPool::sizeSexuallyActive(DmgProfile::Gender _gender, Person::RiskLevel _risk){
	DmgProfileBucket *bucket = NULL;
	unsigned long size = 0;		//total of the zie
	int currBucketIndex = 0;

	//iterate through all buckets
	while(currBucketIndex < this->entityBuckets->size()) {
		bucket = this->entityBuckets->at(currBucketIndex);
		//if bucket == NULL, that means we are not using this particular DmgProfile during this sim
		if( bucket == NULL) {
			currBucketIndex++;
			continue;
		}
		//Only add the sizes of sexually active buckets that match demographic profile
		if( DmgProfile::get(bucket->getProfileID(), DmgProfile::SEXUAL_ACTIVITY_STATUS) == DmgProfile::SA){
			if(DmgProfile::get(bucket->getProfileID(), DmgProfile::GENDER) == _gender)
			{
				size+= ((BucketSexualMixing*)(this->entityBuckets->at(currBucketIndex)))->sizeRisk(_risk);
			}
		}
		currBucketIndex++;
	}	//while(currBucketIndex < this->entityBuckets->size()) {

	return size;
}
//calculate the current number of sexually active persons within the specified age range
unsigned long EntityPool::sizeSexuallyActiveByAge(int minAgeMonths, int maxAgeMonths) {
	DmgProfileBucket *bucket = NULL;
	unsigned long size = 0;		//total of the zie
	int currBucketIndex = 0;

	//iterate through all buckets
	while(currBucketIndex < this->entityBuckets->size()) {
		bucket = this->entityBuckets->at(currBucketIndex);
		//if bucket == NULL, that means we are not using this particular DmgProfile during this sim
		if( bucket == NULL) {
			currBucketIndex++;
			continue;
		}
		//Only add the sizes of sexually active buckets
		if( DmgProfile::get(bucket->getProfileID(), DmgProfile::SEXUAL_ACTIVITY_STATUS) == DmgProfile::SA){
			size+= ((BucketSexualMixing*) (this->entityBuckets->at(currBucketIndex)))->sizeByAge(minAgeMonths, maxAgeMonths);
		}
		currBucketIndex++;
	}	//while(currBucketIndex < this->entityBuckets->size()) {

	return size;
}

//calculate the current number of sexually active persons within the specified age range and gender
unsigned long EntityPool::sizeSexuallyActiveByAge(int minAgeMonths, int maxAgeMonths, DmgProfile::Gender _gender) {
	DmgProfileBucket *bucket = NULL;
	unsigned long size = 0;		//total of the zie
	int currBucketIndex = 0;

	//iterate through all buckets
	while(currBucketIndex < this->entityBuckets->size()) {
		bucket = this->entityBuckets->at(currBucketIndex);
		//if bucket == NULL, that means we are not using this particular DmgProfile during this sim
		if( bucket == NULL) {
			currBucketIndex++;
			continue;
		}
		//Only add the sizes of sexually active buckets
		if( DmgProfile::get(bucket->getProfileID(), DmgProfile::SEXUAL_ACTIVITY_STATUS) == DmgProfile::SA){
			if (DmgProfile::get(bucket->getProfileID(), DmgProfile::GENDER) == _gender){
				size+= ((BucketSexualMixing*) (this->entityBuckets->at(currBucketIndex)))->sizeByAge(minAgeMonths, maxAgeMonths);
			}
		}
		currBucketIndex++;
	}	//while(currBucketIndex < this->entityBuckets->size()) {

	return size;
}
//-------------------< Begin allMale and allFemale functions >----------------//
/*
 * adds Person to DmgProfileBucket AND allMales or allFemales depending on gender
 */
bool EntityPool::addPersonToAll(Person *_p){
	if (_p->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE){
		this->allMales.push_back(_p);
	}
	else{
		assert(_p->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::FEMALE);
		this->allFemales.push_back(_p);
	}

	return this->addEntity(_p);
}

/*
 * removes Person from DmgProfileBucket AND allMales or allFemales depending on gender
 * should be used only when *(_pIter) dies or when deleting this
 */
list<Person*>::iterator EntityPool::removePersonFromAll(list<Person*>::iterator _pIter){
	list<Person*>::iterator toReturn;
	this->removeEntity(*_pIter);
	if ((*_pIter)->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE){
		toReturn = this->allMales.erase(_pIter);
	}
	else if((*_pIter)->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::FEMALE){
		toReturn = this->allFemales.erase(_pIter);
	}
	else{
		(*_pIter)->print(cerr, "Not removing person of indiscriminate gender: ");
		toReturn = _pIter;
	}
	return toReturn;
}

/*
 * Returns allMales->begin()
 */
list<Person*>::iterator EntityPool::begin(DmgProfile::Gender _gender){
	if (_gender == DmgProfile::MALE){
		return this->allMales.begin();
	}
	else{
		return this->allFemales.begin();
	}
}


/*
 * Returns allMales->end()
 */
list<Person*>::iterator EntityPool::end(DmgProfile::Gender _gender){
	if (_gender == DmgProfile::MALE){
		return this->allMales.end();
	}
	else{
		return this->allFemales.end();
	}
}

//-------------------< End allMale and allFemale functions >----------------//
//---------------< Begin constructors and destructors >-------------------------//


//creates a New EntityPool
// @param _SAEntAgeMths age of sexual debut
EntityPool::EntityPool(int _SAEntAgeMths, unsigned int _popID, const double _assort[]) {

	//allocate space for Buckets and set to NULL
	this->entityBuckets = new std::vector<DmgProfileBucket*>(DmgProfile::TotalNumBuckets,NULL);

	//this helps us select the buckets we want to use in the sim
	// initializing to END values will select all buckets
	DmgProfile selector;
	//contains the BucketID's of the buckets we want to use in this simulation
	std::vector<DmgProfile::ProfileID> validBucketIDs;

	//we only want 2 NA buckets (male, female)  b/c they aren't involved in sexual mixing
	//so instantiate 2 of the NA Buckets (NA, Hetero, nonCSW
	selector.set(DmgProfile::SEXUAL_ACTIVITY_STATUS, DmgProfile::NA);
	selector.set(DmgProfile::SEXUAL_ORIENTATION, DmgProfile::HETERO);
	selector.set(DmgProfile::RELATIONSHIP_STATUS, DmgProfile::SINGLE);
	selector.set(DmgProfile::EMPLOYMENT, DmgProfile::NON_CSW);
	selector.selectProfileIDs(validBucketIDs, NULL);
	//check if we only have 2 buckets
	assert(validBucketIDs.size() == DmgProfile::ENDGender);

	//We want to instantiate all heterosexual SA Buckets
	selector.set(DmgProfile::END);
	selector.set(DmgProfile::SEXUAL_ACTIVITY_STATUS, DmgProfile::SA);
	selector.set(DmgProfile::SEXUAL_ORIENTATION, DmgProfile::HETERO);
	selector.selectProfileIDs(validBucketIDs, NULL);

	//instantiate the spaces for all our buckets. The # of buckets depends on class BucketClassifiers
	//these people are stored in a more complicated DmgProfileBucket b/c they are involved in sexual mixing
	for(unsigned int i = 0; i< validBucketIDs.size(); ++i) {
		DmgProfile::ProfileID currBucketID = validBucketIDs.at(i);

		//at this point, DmgProfile still matches the DmgProfile::SA
		if( DmgProfile::NA == DmgProfile::get(validBucketIDs.at(i),DmgProfile::SEXUAL_ACTIVITY_STATUS)) {
			//make an NA bucket
			this->entityBuckets->at(currBucketID) = new DmgProfileBucket(currBucketID, DmgProfile::toString(currBucketID), true);
		} else {
			//make an SA bucket -- we leave this here for compiling purposes
			// should insert the full-vector structure
			assert(Constants::TODO_DEF);

			//CSW can't be in STEADY relationships
			bool invalidCombo = (DmgProfile::CSW == DmgProfile::get(validBucketIDs.at(i),DmgProfile::EMPLOYMENT)) &&
								(DmgProfile::NON_SINGLE == DmgProfile::get(validBucketIDs.at(i),DmgProfile::RELATIONSHIP_STATUS)) ;
			if ( !invalidCombo)
				//ERINWASHERE
				//CHANGES WENT HERE!!
				//this->entityBuckets->at(currBucketID) = new DmgProfileBucket(currBucketID, DmgProfile::toString(currBucketID), true);
			this->entityBuckets->at(currBucketID) = new BucketSexualMixing(currBucketID,DmgProfile::toString(currBucketID),_popID,_SAEntAgeMths,12*Person::maxYrForDeathStats + 1,MONTH, _assort);
		}
	}	//for(unsigned int i = 0; validBucketIDs.size(); ++i) {
}

EntityPool::~EntityPool(void) {

	//Delete all people in allFemales and allMales in order to prevent memory leaks
	list<Person*>::iterator p_Iter;
	for (int gend = DmgProfile::MALE; gend < DmgProfile::ENDGender; gend++){
			DmgProfile::Gender gender = DmgProfile::MALE;
			if (gend == DmgProfile::FEMALE){
				gender = DmgProfile::FEMALE;
			}
			p_Iter = this->begin(gender);
			while (p_Iter != this->end(gender)) {
				Person *p = (*p_Iter);
				//Advances p_Iter one in the list, so no increment is necessary
				p_Iter = this->removePersonFromAll(p_Iter);
				delete p;
			}
	}

	this->allMales.clear();
	this->allFemales.clear();

	//iterate through all buckets and delete them
	for(BaseEnumCls::Enum j = 0; j < this->entityBuckets->size(); ++j) {
		if(this->entityBuckets->at(j) != NULL){
			if(DmgProfile::NA !=
				DmgProfile::get(this->entityBuckets->at(j)->getProfileID(), DmgProfile::SEXUAL_ACTIVITY_STATUS)){
					delete (BucketSexualMixing*)this->entityBuckets->at(j);
			}
			else {
				delete this->entityBuckets->at(j);
			}
		}
	} //for(BaseEnumCls::Enum j = 0; j < this->entityBuckets->size(); ++j) {

	//delete the vector
	delete this->entityBuckets;

}

//---------------< END constructors and destructors >-------------------------//

//-------------< End methods for EntityPool >-------------------//




