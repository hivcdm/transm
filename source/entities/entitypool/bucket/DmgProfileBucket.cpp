#include "DmgProfileBucket.h"
#include "../../Person.h"
#include <assert.h>
#include <algorithm>
class RandomNums;

//------------< Begin Implemented Methods >----------------//
//returns the DmgProfileBucket's ID number
DmgProfile::ProfileID DmgProfileBucket::getProfileID() {
	return this->dmgProfileID;
}
/**
This method returns entity index for dmgprofilebucket
**/
DmgProfileBucket::PersonSet * DmgProfileBucket::getEntityIndex(){
	return this->simpleEntityIndex;
}

/**
This method will return a label for this DmgProfileBucket
**/
const string* DmgProfileBucket::getLabel() {
	return this->bucketLabel;
}

//------------< End Implemented Methods >----------------//

//------------------< Begin DmgProfileBucket Virtual methods >------------------//
void DmgProfileBucket::clear() {
	assert(this->simpleEntityIndex != NULL);
	this->simpleEntityIndex->clear();
}

Person* DmgProfileBucket::drawMember(RandomNums& _randomNums, SexualPartnership::Type _partnershipType, bool _remove) {
	assert(this->simpleEntityIndex != NULL);
	Person* removed = this->simpleEntityIndex->drawMember(_randomNums, _partnershipType, _remove);

	//we have to tell the person that they are not part of a bucket anymore
	if(_remove && removed) {
		removed->setCurrBucketProfileID(DmgProfile::END);
	}

	return removed;
}

Person *DmgProfileBucket::drawMember(RandomNums& _randomNums, Person */*_chooser*/, SexualPartnership::Type _partnershipType, bool _remove)
{
	//we have to implement the more complicated drawing process
	//This is done in BucketSexualMixing
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	cerr << "DmgProfileBucket::drawMember(RandomNums& _randomNums, Person *_chooser,SexualPartnership::Type _partnershipType, bool _remove) was called: only persons in a BucketSexualMixing should be drawing partners!" << endl;
/*
	Person* removed = ______________;
	if(_remove && removed) {
		removed->setCurrBucketProfileID(DmgProfile::END);
	}
*/
	return drawMember(_randomNums, _partnershipType,_remove);
}

bool DmgProfileBucket::erase(Person *_person)
{
    assert(this->simpleEntityIndex != NULL);
    assert(_person != NULL);
    try
    {
	return this->simpleEntityIndex->erase(_person);
    }
    catch (std::out_of_range& e) {
	std::cout << "Out of range: " << e.what() << "\n";
    }
    catch (std::exception& e) {
	std::cout << "Some other exception: " << e.what() << "\n";
    }

    return false;
}

bool DmgProfileBucket::exists(Person *_person){
	assert(this->simpleEntityIndex != NULL);
	assert(_person!=NULL);
	return this->simpleEntityIndex->exists(_person);
}

unsigned long DmgProfileBucket::getNumInfected() {
	assert(this->simpleEntityIndex != NULL);
	return this->simpleEntityIndex->getNumInfected();
}

unsigned long DmgProfileBucket::getNumInfected(int generation) {
	assert(this->simpleEntityIndex != NULL);
	return this->simpleEntityIndex->getNumInfected(generation);
}

bool DmgProfileBucket::insert(Person *_person){
	assert(this->simpleEntityIndex != NULL);
	assert(_person!=NULL);
	this->simpleEntityIndex->insert(_person);
	//let the _person know of their new DmgProfileBucket membership
	_person->setCurrBucketProfileID(this->getProfileID());
	return true;
}

/*DmgProfileBucket::JIterator DmgProfileBucket::iterator() {
	assert(this->simpleEntityIndex != NULL);
	return JIterator( new JavaStyleIterator(this) );
}*/

void DmgProfileBucket::print(std::ostream& _outStream, std::string _prefix){
	assert(this->simpleEntityIndex != NULL);
	this->simpleEntityIndex->print(_outStream,_prefix);
}

unsigned long DmgProfileBucket::size() {
	assert(this->simpleEntityIndex != NULL);
	return this->simpleEntityIndex->size();
}

/*
 * @effects: Ages everyone in the bucket one timestep
 * @returns: List of persons too old for timestep (should always be null)
 */
list<Person*> DmgProfileBucket::ageOneTimeStep(){
	list<Person*> lP;
	multimap<unsigned long, Person*>::iterator pIter;
	for (pIter = this->simpleEntityIndex->begin(); pIter != this->simpleEntityIndex->end(); pIter++){
		(pIter->second)->ageOneTimeUnit();
	}

	return lP;
}
//------------------< End DmgProfileBucket Virtual methods >------------------//


//-----------------< Begin Constructors and Destructors >----------------//
//this function should not be used in this sim, it's just here for a default constructor
DmgProfileBucket::DmgProfileBucket(){
	simpleEntityIndex = NULL;
}

//this creates a simple DmgProfileBucket with an index that is sorted by age
DmgProfileBucket::DmgProfileBucket(int _id, const string *_bucketLabel, bool _simpleIndex){
	this->dmgProfileID = _id;
	this->bucketLabel = _bucketLabel;
	//if this is a simple index, then use field simpleEntityIndex
	this->simpleEntityIndex = _simpleIndex ? new PersonSet() : NULL;
}

DmgProfileBucket::~DmgProfileBucket(void){
	delete this->simpleEntityIndex;
}
//-----------------< End Constructors and Destructors >----------------//


//-----------< Begin Methods for DmgProfileBucket::JavaStyleIterator >--------------//
/*
DmgProfileBucket::JavaStyleIterator::JavaStyleIterator() {
}

DmgProfileBucket::JavaStyleIterator::JavaStyleIterator(DmgProfileBucket* _bucket) {
	this->pIter = _bucket->simpleEntityIndex->iterator();

	//initialize this iterator to iterate from first element
	this->reset();
}


bool DmgProfileBucket::JavaStyleIterator::alreadyRemoved() {
	return this->pIter->alreadyRemoved();
}


bool DmgProfileBucket::JavaStyleIterator::hasNext() {
	return this->pIter->hasNext();
}


Person * DmgProfileBucket::JavaStyleIterator::next(){
	return this->pIter->next();
}


bool DmgProfileBucket::JavaStyleIterator::remove() {
	//save a pointer to this person
	Person *p = this->pIter->get();

	//try to remove them from the DmgProfileBucket
	bool removed = this->pIter->remove();

	//set this person as not being part of any Bucket
	if(removed && p) {
		p->setCurrBucketProfileID(DmgProfile::END);
	}

	return removed;
}


void DmgProfileBucket::JavaStyleIterator::reset() {
	this->pIter->reset();
}


DmgProfileBucket::JavaStyleIterator::~JavaStyleIterator() {
}
*/
