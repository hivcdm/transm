#include "BucketSexualMixing.h"
#include <iostream>

#include "../../../util/Util.h"
#include "../../../util/rand/RandomNums.h"


BucketSexualMixing::BucketSexualMixing(DmgProfile::ProfileID _id, const string *_bucketLabel, unsigned int _popID, int _minAge, int _maxAge, TimeGranularity _timeGranularity, const double _assort[]) :
DmgProfileBucket(_id, _bucketLabel, false) {
	assert( (_timeGranularity == MONTH) || (_timeGranularity == YEAR));
	assert( (_minAge >= 0) && (_maxAge >= _minAge));

	this->timeGranularity = _timeGranularity;
	this->minAge = _minAge;
	this->maxAge = _maxAge;
	this->popID = _popID;
	for (int i = 0; i < SexualPartnership::ENDType; i++)
		this->assort[i] = _assort[i];
//ERINWASHERE
	//set capacity of circular buffer
	unsigned int numAgeBuckets = this->maxAge - this->minAge + 1;
	personsByAge = new BucketAllAges(numAgeBuckets);
	//personsByAge = new BucketAllAges();

	//initialize all the BucketAges in the BucketSexualMixing, a circular buffer will hold all people of a certain age
	unsigned int currAge = this->minAge;
	while( currAge <= this->maxAge) {
		BucketAge *pToAge = new BucketAge(_id, _popID, _assort);
		assert(pToAge->getBinID() == _id);
		personsByAge->push_back(pToAge);

		currAge++;
	}
}

BucketSexualMixing::~BucketSexualMixing()
{
    //it looks like the members will be destructed...?
    //delete entitiesByAge;
    BucketAllAges::iterator agesIter = personsByAge->begin();
    //BucketAllAges::iterator agesIterCopy;
    //go through each BucketAge and delete it
    while(agesIter != personsByAge->end())
    {
	(*agesIter)->clear();
	//Copy it over and increment
	BucketAge *bucketAgeToDelete = (*agesIter);
	agesIter = personsByAge->erase(agesIter);
	delete bucketAgeToDelete;
    }

    personsByAge->clear();
    delete personsByAge;
}


unsigned int BucketSexualMixing::getCorrectBufferIndex(Person *_p) {
	//get person's age in right time granularity
	unsigned int pAge = _p->getAge(this->timeGranularity);
	if (!Util::withinRange<unsigned int>(pAge, this->minAge, this->maxAge)) {
		cout << "Age is " << _p->getAge(MONTH) << " but minAge is " << this->minAge << " and max age is " << this->maxAge << endl;
	}
	assert(Util::withinRange(pAge, this->minAge, this->maxAge));

	/*//if this person's DmgProfile doesn't match this->DmgProfile, return -1
	if (this->getProfileID() != _p->getDmgProfile()->getProfileID()){
		cout << "WRONG PROFILE" << endl;
		return (unsigned int)this->personsByAge->size();
	}*/

	//age determines place in the circular buffer
	if (!Util::withinRange<unsigned int>(pAge, this->minAge, this->maxAge)) {
		//If person is out of range of this buffer, return -1 which is an invalid entry
		return (unsigned int)this->personsByAge->size();
	} else {
		return (pAge - this->minAge);
	}
}

//clears all elements from this index without deleting members
void BucketSexualMixing::clear() {
	BucketAllAges::iterator agesIter;
	//go through each BucketAge and clear it
	for (agesIter = personsByAge->begin(); agesIter != personsByAge->end(); agesIter++) {
		(*agesIter)->clear();
	}
}

Person* BucketSexualMixing::drawMember(RandomNums& _randomNums, SexualPartnership::Type _partnershipType, bool _remove) {
	if( this->size() == 0)
		return NULL;
	//Use minAge and maxAge for bucket

	//default use risk level of low
	//Assortative param will set it to random anyhow
	return this->getRandomPerson(_randomNums,this->minAge, this->maxAge, Person::LOW, _partnershipType, _remove);
}

/***
@param _remove - will remove this person from the BucketSexualMixing if this is true
***/
Person* BucketSexualMixing::drawMember(RandomNums& _randomNums, Person *_chooser,SexualPartnership::Type _partnershipType,bool _remove) {

	//these determine the bounds of which ages we will consider
	int minDesired = 0;
	int maxDesired = INT_MAX;

	if(_chooser) {
		double ageYoungerYears = _chooser->rollForAgeDifference(_partnershipType, _randomNums);
		int ageYoungerMonths = (int)(12*ageYoungerYears + 0.5);
		//AgeYoungerMonths can be negative so we need to make sure the range stays between both the min and the max age
		minDesired = min(_chooser->getAge(MONTH) - (ageYoungerMonths + 6), (int) (this->maxAge));
		maxDesired = max(_chooser->getAge(MONTH) - (ageYoungerMonths - 6), (int) (this->minAge));
		assert(minDesired <= maxDesired);
	}

	//min and max age of Entitys that _chooser can pick from this Person container
	int minAgeDesired = max((int) (this->minAge), minDesired);
	int maxAgeDesired = min((int) (this->maxAge), maxDesired);



	return this->getRandomPerson(_randomNums, minAgeDesired, maxAgeDesired, _chooser->getRiskLevel(), _partnershipType, _remove);
}

//will remove this Person (if he or she exists) from the index
bool BucketSexualMixing::erase(Person *_person){
	assert(_person != NULL);
	unsigned int correctIndex = this->getCorrectBufferIndex(_person);

	//if this person wouldn't be in this DmgProfileBucket, then return false
	if(correctIndex >= this->personsByAge->size()){
		return false;
	}

	//bool removed =  this->entitiesByAge->at(correctIndex)->erase(_person);
	bool removed = this->personsByAge->at(correctIndex)->erase(_person);

	if(removed)
		_person->setCurrBucketProfileID(DmgProfile::END);

	return removed;
}

//tells whether _person exists in the index
bool BucketSexualMixing::exists(Person *_person){
	unsigned int correctIndex = this->getCorrectBufferIndex(_person);

	//if this person wouldn't be in this DmgProfileBucket, then return false
	if( correctIndex >= this->personsByAge->size()) return false;

	return this->personsByAge->at(correctIndex)->exists(_person);
}

//counts number of infected people this EntityPool
unsigned long BucketSexualMixing::getNumInfected() {
	BucketAllAges::iterator ageBucketIter;
	unsigned long totalInfected = 0;
	//go through each Index and get # infected.
	for ( ageBucketIter = personsByAge->begin(); ageBucketIter != personsByAge->end(); ageBucketIter++) {
		totalInfected += (*ageBucketIter)->getNumInfected();
	}
	return totalInfected;
}

//counts number of infected people this EntityPool by generation
unsigned long BucketSexualMixing::getNumInfected(int generation) {
	BucketAllAges::iterator ageBucketIter;
	unsigned long totalInfected = 0;
	//go through each Index and get # infected.
	for ( ageBucketIter = personsByAge->begin(); ageBucketIter != personsByAge->end(); ageBucketIter++) {
		totalInfected += (*ageBucketIter)->getNumInfected(generation);
	}
	return totalInfected;
}

/*
 * @returns: total number of infected persons in this risk group
 */
unsigned long BucketSexualMixing::getNumInfected(Person::RiskLevel _risk){
	BucketAllAges::iterator ageBucketIter;
	unsigned long totalInfected = 0;
	//go through each Index and get # infected by risk.
	for ( ageBucketIter = personsByAge->begin(); ageBucketIter != personsByAge->end(); ageBucketIter++) {
		totalInfected += (*ageBucketIter)->getNumInfected(_risk);
	}
	return totalInfected;
}

Person* BucketSexualMixing::getRandomPerson(RandomNums& _randomNums, unsigned int _ageLowerBound, unsigned int _ageUpperBound, Person::RiskLevel _risk, SexualPartnership::Type _partnershipType, bool _remove) {

	if (_ageLowerBound < this->minAge){
		//Set this to avoid negative unsigned int, which is just asking for trouble
		_ageLowerBound = this->minAge;
	}
	//get the age buckets that we will search within
	unsigned int minIndex = std::max<unsigned int>(_ageLowerBound - this->minAge, 0);
	unsigned int maxIndex = std::min<unsigned int>(_ageUpperBound - this->minAge, (unsigned int)this->personsByAge->size()-1);

	//Use this->assort to determine whether to use random or _risk bin
	Person::RiskLevel riskToDraw;

	if (_randomNums.chance(this->assort[_partnershipType])){
		riskToDraw = _risk;
	}
	else {
		riskToDraw = Person::ENDRiskLevel;
	}

	//figure out # of eligible people
	unsigned int currIndex = minIndex;
	unsigned long numMarbles = 0;
	while(currIndex <= maxIndex) {
		numMarbles+= this->personsByAge->at(currIndex)->numChoices(riskToDraw);
		currIndex++;
	}

	//If no one is within the requested age range, return NULL.
	if (numMarbles == 0){
		//Recursively expand search with _ageLowerBound and _ageUpperBound expanding by 6 months
		//Only recurse if there are partners available in this bucket to save time
		numMarbles = 0;
		for (currIndex = 0; currIndex < this->personsByAge->size(); currIndex++){
			numMarbles += this->personsByAge->at(currIndex)->numChoices(riskToDraw);
		}
		if (numMarbles > 0){
			//Check first that minIndex > 0 and maxIndex < this->personsByAge->size() - 1
			if (minIndex > 0 || maxIndex < this->personsByAge->size() - 1){
				unsigned int newAgeLowerBound = _ageLowerBound;
				//Make sure the ageLowerBound being recursively passed in is non-negative to avoid those pesky unsigned int issues
				if (newAgeLowerBound < 6){
					newAgeLowerBound = 0;
				} else{
					newAgeLowerBound = _ageLowerBound - 6;
				}
				return this->getRandomPerson(_randomNums, newAgeLowerBound, _ageUpperBound + 6, _risk,_partnershipType, _remove);
			} else {
				return NULL;
			}
		} else {
			return NULL;
		}
	}

	//use the randPick to determine which AgeBucket to draw from
	unsigned long randPick = _randomNums.randInt(0,numMarbles-1);
	//keep looping through all the indexes within personsByAge until we get the one that contains the randPick'th person with age between [_ageLowerBound, _ageUpperBound]
	currIndex = minIndex;
	while (currIndex <= maxIndex) {
		//if we're at the right AgeBucket
		if(randPick < this->personsByAge->at(currIndex)->numChoices(riskToDraw)) {
			BucketAge *ageBucket = this->personsByAge->at(currIndex);
			Person *p = ageBucket->drawMember(_randomNums, _risk, _partnershipType,(riskToDraw == Person::ENDRiskLevel), _remove);
			//we have to tell the person that they are not part of a bucket anymore
			if(_remove && p) {
				p->setCurrBucketProfileID(DmgProfile::END);
			}
			return p;

		}
		randPick -= this->personsByAge->at(currIndex)->numChoices(riskToDraw);
		currIndex++;
	}

	return NULL;
}

//will index a New Person
bool BucketSexualMixing::insert(Person *_person){
	//Don't allow person with wrong DmgProfile to be inserted
	if (_person->getDmgProfile()->getProfileID() != this->getProfileID()){
		return false;
	}
	unsigned int correctIndex = this->getCorrectBufferIndex(_person);

	//if this person belongs in this DmgProfileBucket
	if(correctIndex < this->personsByAge->size())  {
		//check to see that this person isn't already in here
		if (this->personsByAge->at(correctIndex)->exists(_person)){
			return false;
		}
		bool inserted = this->personsByAge->at(correctIndex)->insert(_person);
		if(inserted) {
			_person->setCurrBucketProfileID( this->getProfileID());
		}
		return inserted;
	} else {
		cerr << "Trying to insert person with invalid age (" << _person->getAge(this->timeGranularity) << " " << ((this->timeGranularity == MONTH) ? "months" : "years") << ")" << endl;
		cerr << "Valid ages are between " << minAge << " and " <<  maxAge << " inclusive" << endl;
		cerr << "If age is valid, person may have an invalid DmgProfile";
		_person->print(cerr,"");
		return false;
	}
}

/*BucketSexualMixing::JIterator BucketSexualMixing::iterator() {
	#if defined(__APPLE__)
	return typename BucketSexualMixing::JIterator( new DmgProfileBucket::JavaStyleIterator(this) );
	#endif
#if defined(WIN32)
	return BucketSexualMixing::JIterator( new DmgProfileBucket::JavaStyleIterator(this) );
#endif
}*/


void BucketSexualMixing::print(ostream& _outStream, std::string _prefix){
	//The iterator of the BucketAges in the circular buffer
	BucketAllAges::iterator bucketIter;
	for (bucketIter = this->begin(); bucketIter != this->end(); bucketIter++) {
		//The iterator of all (unique) persons in the BucketAge
		//for (personIter = (*bucketIter)->begin(); personIter != (*bucketIter)->end(); personIter++){
			//(*personIter)->print(_outStream,_prefix);
		//}
		//A print function was written for BucketAge... lets use it
		(*bucketIter)->print(_outStream,_prefix);
	}
}

//returns the # of entities in this index
unsigned long BucketSexualMixing::size() {
	BucketAllAges::iterator bucketIter;
	unsigned long total = 0;
	//go through each Index and get # infected.
	for ( bucketIter = personsByAge->begin(); bucketIter != personsByAge->end(); bucketIter++) {
		total +=(*bucketIter)->size();
	}
	return total;
}


//-------------< Begin iterator methods >------------------//
BucketSexualMixing::BucketAllAges::iterator BucketSexualMixing::begin() {
	return this->personsByAge->begin();
}

BucketSexualMixing::BucketAllAges::iterator BucketSexualMixing::end(){
	return this->personsByAge->end();
}

BucketAge* BucketSexualMixing::getOldest(){
	return this->personsByAge->back();
}

BucketAge* BucketSexualMixing::getYoungest(){
	return this->personsByAge->front();
}
//--------------< End iterator methods >-------------------//
//-------< Begin additional methods based on this structure >-------//
/*
 * @returns: total number of persons in this with age between minMonthAge and maxMonthAge
 */
unsigned long BucketSexualMixing::sizeByAge(int minMonthAge, int maxMonthAge) {
	//get the age buckets that we will count within
	unsigned int minIndex = std::max<unsigned int>(minMonthAge - this->minAge, 0);
	unsigned int maxIndex = std::min<unsigned int>(maxMonthAge - this->minAge, (unsigned int)this->personsByAge->size()-1);
	unsigned int currIndex = minIndex;
	unsigned long total = 0;
	while(currIndex <= maxIndex) {
		total+= this->personsByAge->at(currIndex)->size();
		currIndex++;
	}
	return total;
}

/*
 * @returns: total number of infected persons in this with age between minMonthAge and maxMonthAge
 */
unsigned long BucketSexualMixing::sizeInfectedByAge(int minMonthAge, int maxMonthAge) {
	//get the age buckets that we will count within
	unsigned int minIndex = std::max<unsigned int>(minMonthAge - this->minAge, 0);
	unsigned int maxIndex = std::min<unsigned int>(maxMonthAge - this->minAge, (unsigned int)this->personsByAge->size()-1);
	unsigned int currIndex = minIndex;
	unsigned long total = 0;
	while(currIndex <= maxIndex) {
		total+= this->personsByAge->at(currIndex)->getNumInfected();
		currIndex++;
	}
	return total;
}

/*
 * @returns: total number of marbles in all FVs associated with _risk
 * BucketAges in this
 */
unsigned long BucketSexualMixing::sizeRisk(Person::RiskLevel _risk){
	assert(_risk <= Person::ENDRiskLevel);
	BucketAllAges::iterator bucketIter;
	unsigned long total = 0;
	//go through each Index and get # random marbles
	for ( bucketIter = personsByAge->begin(); bucketIter != personsByAge->end(); bucketIter++) {
		total +=(*bucketIter)->getNumRisk(_risk);
	}
	return total;
}

/*
 * @returns: total number of unique persons in this bucket that is CSW with given _risk
 * BucketAges in this
 */
unsigned long BucketSexualMixing::sizeRiskCSW(Person::RiskLevel _risk){
	assert(_risk <= Person::ENDRiskLevel);
	BucketAllAges::iterator bucketIter;
	unsigned long total = 0;
	//go through each Index and get # random marbles
	for ( bucketIter = personsByAge->begin(); bucketIter != personsByAge->end(); bucketIter++) {
		total +=(*bucketIter)->getNumRiskCSW(_risk);
	}
	return total;
}
/*
 * @returns: total number of unique persons in this bucket with given risk level and hiv status
 * across all BucketAges in this;
 */
unsigned long BucketSexualMixing::sizeRiskHIVStatus(Person::RiskLevel _risk,Person::HIVStatus _hivStatus){
	assert(_risk <= Person::ENDRiskLevel);
	assert(_hivStatus <= Person::ENDHIVStatus);
	BucketAllAges::iterator bucketIter;
	unsigned long total = 0;
	//go through each Index and get # random marbles
	for ( bucketIter = personsByAge->begin(); bucketIter != personsByAge->end(); bucketIter++) {
		total +=(*bucketIter)->getNumRiskHIVStatus(_risk, _hivStatus);
	}
	return total;
}

/*
 * @returns: total number of marbles in all Random Risk FVs across all
 * BucketAges in this
 */
unsigned long BucketSexualMixing::sizeRandom(){
	BucketAllAges::iterator bucketIter;
	unsigned long total = 0;
	//go through each Index and get # random marbles
	for ( bucketIter = personsByAge->begin(); bucketIter != personsByAge->end(); bucketIter++) {
		total +=(*bucketIter)->numRandomRiskChoices();
	}
	return total;
}

/*
 * @function: increaseInfected();
 * @effects: if _person.isInfected, increases number of infected persons for
 * the BucketAge which corresponds to _person
 * @return: returns true if _person.isInfected and number was increased and
 * false otherwise
 */
bool BucketSexualMixing::increaseInfected(Person *_person){
	if (!(_person->isInfected())){
		//_person isn't infected
		return false;
	}

	int index = this->getCorrectBufferIndex(_person);
	if (index >= this->personsByAge->size()){
		//_person is not in this Bucket collection
		return false;
	}

	return this->personsByAge->at(index)->increaseInfected(_person);
}

/* @function: changeHIVstatus
 * @effects: if person is in this Bucket and thier hiv status changes decrement the old status and increment new status
 */
void BucketSexualMixing::changeHIVStatus(Person *_p, Person::HIVStatus _orig, Person::HIVStatus _new){
	int index = this->getCorrectBufferIndex(_p);
	if (index >= this->personsByAge->size()){
		return;
	}
	this->personsByAge->at(index)->changeHIVStatus(_p, _orig, _new);
}

/*
 * @effects: Sets all persons in oldest BucketAge to die and processes their
 * deaths, pushes new BucketAge to front (for incoming births), sets all other persons
 * to age++ (to account for new index of BucketAge
 * @returns: List of persons expired out of this (of old age)
 */
list<Person*> BucketSexualMixing::ageOneTimeStep(){
	//Kill off the oldest
	BucketAge *oldestPersons = this->getOldest();

	//Iterate through all oldest remove them from this and put them in list of "expired" persons to be returned
	list<Person*> toReturn;
	vector<Person*>::iterator personIterator = oldestPersons->begin();
	while (personIterator != oldestPersons->end() && oldestPersons->size() > 0){
		try {
			Person* oldPerson = (*personIterator);
			//We're going to remove oldPerson, so advance the iterator now before it gets confused
			personIterator++;
			toReturn.push_back(oldPerson);
			this->erase(oldPerson);
			oldPerson->ageOneTimeUnit();
		} catch(std::exception& e) {
			cout << e.what() << endl;
			personIterator = oldestPersons->end();
		}
	}

	//Insert the new BucketAge for incoming youngest
	BucketAge *pNewYoungest = new BucketAge(this->getProfileID(), this->popID, this->assort);
	this->personsByAge->push_front(pNewYoungest);

	//Delete the OldestPersons bucket
	//this->personsByAge->pop_back();
	delete oldestPersons;

	assert(this->personsByAge->size() <= this->maxAge - this->minAge + 1);

	//Iterate through and age everyone
	BucketAllAges::iterator bucketIter;
	for (bucketIter = this->begin(); bucketIter != this->end(); bucketIter++) {
		//The iterator of all (unique) persons in the BucketAge
		for (personIterator = (*bucketIter)->begin(); personIterator != (*bucketIter)->end(); personIterator++){
			(*personIterator)->ageOneTimeUnit();
			assert(this->exists(*personIterator));
		}
	}
	return toReturn;
}

//--------< End additional methods based on this structure >--------//

//---------------< Begin methods for BucketSexualMixing::JavaStyleIterator >--------------------//
/*
BucketSexualMixing::JavaStyleIterator::JavaStyleIterator(BucketSexualMixing *_bucket){
	this->currBuffIndex = 0;
	this->entityCircularBuff = _bucket->entitiesByAge;
	this->currNumIndexJIterator = this->entityCircularBuff->at(currBuffIndex)->iterator();
}

//returns true if the element that was last returned by next() has been removed using remove()
bool BucketSexualMixing::JavaStyleIterator::alreadyRemoved() {
	return this->currNumIndexJIterator->alreadyRemoved();
}

//returns the spot right after last member of this pool
bool BucketSexualMixing::JavaStyleIterator::hasNext() {
	//if current grid index contains entities, then return true
	if( this->currNumIndexJIterator->hasNext() )
		return true;
	else {
		//else, find next grid index with people in it
		while ((this->currBuffIndex + 1) < this->entityCircularBuff->size())		{
			//we are relinquishing the auto_ptr control...
			this->currNumIndexJIterator.reset(NULL);
			this->currBuffIndex = this->currBuffIndex+1;

			//if this has any elements, then iterate through
			if(this->entityCircularBuff->at(currBuffIndex)->size()) {
				this->currNumIndexJIterator = this->entityCircularBuff->at(currBuffIndex)->iterator();
				return true;
			}
		} // end while
	} // end else

	return false;
}

//this will be used to get the next in line
Person* BucketSexualMixing::JavaStyleIterator::next() {
	if(this->hasNext()) {
		return this->currNumIndexJIterator->next();
	} else
		return NULL;
}

//removes from the collection the last element returned by the iterator
bool BucketSexualMixing::JavaStyleIterator::remove() {
	bool removed = this->currNumIndexJIterator->remove();

	if(removed) {
		//we need to change this
		assert(Constants::TODO_DEF);
		Person * p = this->currNumIndexJIterator->get();
		p->setCurrBucketProfileID( DmgProfile::END);
	}


	return removed;
}

//lets us reuse an iterator, resets to beginning of current collection
void BucketSexualMixing::JavaStyleIterator::reset() {
	this->currNumIndexJIterator.reset(0);
	currBuffIndex = 0;
	this->currNumIndexJIterator = this->entityCircularBuff->at(currBuffIndex)->iterator();
}

BucketSexualMixing::JavaStyleIterator::~JavaStyleIterator() {
	//cout << "BucketSexualMixing::JavaStyleIterator::~JavaStyleIterator()" << endl;
	this->currNumIndexJIterator.reset(NULL);

	entityCircularBuff = NULL;

}*/

//---------------< End methods for BucketSexualMixing::JavaStyleIterator >--------------------//
