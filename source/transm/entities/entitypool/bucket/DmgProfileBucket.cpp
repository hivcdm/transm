#include <assert.h>
#include <algorithm>

#include "DmgProfileBucket.h"
#include "../../Person.h"
#include "../../../Simulation.h"

/**
* mark a function parameter as unused and avoid
* the corresponding compiler warning.
* wrap around the parameter name, e.g. void f(int UNUSED(x))
**/
#define UNUSED(param)

//------------< Begin Implemented Methods >----------------//
//returns the DmgProfileBucket's ID number
DmgProfile::ProfileID DmgProfileBucket::getProfileID()
{
	return dmgProfileID;
}
/**
This method returns entity index for dmgprofilebucket
**/
DmgProfileBucket::PersonSet *DmgProfileBucket::getEntityIndex()
{
	return simpleEntityIndex;
}

/**
This method will return a label for this DmgProfileBucket
**/
const string *DmgProfileBucket::getLabel()
{
	return bucketLabel;
}

void DmgProfileBucket::Apply(const PopulationTarget &target, std::function<void(Person*)> modifier)
{
	for(auto &id_person_pair : *simpleEntityIndex)
	{
		auto person = id_person_pair.second;
		if(!((target.age_lower.has_value && target.age_lower.value < person->getAge(MONTH))
			|| (target.age_upper.has_value && target.age_upper.value > person->getAge(MONTH))
			|| (target.observed_hiv_status.has_value && target.observed_hiv_status.value != person->getHIVStatus())
			|| (target.on_treatment.has_value && target.on_treatment.value != person->isOnArt())
			|| (target.risk_level.has_value && target.risk_level.value != person->getRiskLevel())))
		{
			modifier(id_person_pair.second);
		}
	}
}

//------------< End Implemented Methods >----------------//

//------------------< Begin DmgProfileBucket Virtual methods >------------------//
void DmgProfileBucket::clear()
{
	assert(simpleEntityIndex != nullptr);
	simpleEntityIndex->clear();
}

Person *DmgProfileBucket::drawMember(RandomNums &_randomNums, SexualPartnership::Type _partnershipType, bool _remove)
{
	assert(simpleEntityIndex != nullptr);
	Person *removed = simpleEntityIndex->drawMember(_randomNums, _partnershipType, _remove);

	//we have to tell the person that they are not part of a bucket anymore
	if(_remove && removed)
	{
		removed->setCurrBucketProfileID(DmgProfile::END);
	}

	return removed;
}

Person *DmgProfileBucket::drawMember(RandomNums &_randomNums, Person *UNUSED(_chooser),
                                     SexualPartnership::Type _partnershipType, bool _remove)
{
	//we have to implement the more complicated drawing process
	//This is done in BucketSexualMixing
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	cerr << "DmgProfileBucket::drawMember(RandomNums& _randomNums, Person *_chooser,SexualPartnership::Type _partnershipType, bool _remove) was called: only persons in a BucketSexualMixing should be drawing partners!"
	     << endl;
	/*
		Person* removed = ______________;
		if(_remove && removed) {
			removed->setCurrBucketProfileID(DmgProfile::END);
		}
	*/
	return drawMember(_randomNums, _partnershipType, _remove);
}

bool DmgProfileBucket::erase(Person *_person)
{
	assert(simpleEntityIndex != nullptr);
	assert(_person != nullptr);

	try
	{
		return simpleEntityIndex->erase(_person);
	}
	catch(std::out_of_range &e)
	{
		std::cout << "Out of range: " << e.what() << "\n";
	}
	catch(std::exception &e)
	{
		std::cout << "Some other exception: " << e.what() << "\n";
	}

	return false;
}

bool DmgProfileBucket::exists(Person *_person)
{
	assert(simpleEntityIndex != nullptr);
	assert(_person != nullptr);
	return simpleEntityIndex->exists(_person);
}

unsigned long DmgProfileBucket::getNumInfected()
{
	assert(simpleEntityIndex != nullptr);
	return simpleEntityIndex->getNumInfected();
}

unsigned long DmgProfileBucket::getNumInfected(int generation)
{
	assert(simpleEntityIndex != nullptr);
	return simpleEntityIndex->getNumInfected(generation);
}

bool DmgProfileBucket::insert(Person *_person)
{
	assert(simpleEntityIndex != nullptr);
	assert(_person != nullptr);
	simpleEntityIndex->insert(_person);
	//let the _person know of their new DmgProfileBucket membership
	_person->setCurrBucketProfileID(getProfileID());
	return true;
}

/*DmgProfileBucket::JIterator DmgProfileBucket::iterator() {
	assert(simpleEntityIndex != nullptr);
	return JIterator( new JavaStyleIterator(this) );
}*/

void DmgProfileBucket::print(std::ostream &_outStream, std::string _prefix)
{
	assert(simpleEntityIndex != nullptr);
	simpleEntityIndex->print(_outStream, _prefix);
}

unsigned long DmgProfileBucket::size()
{
	assert(simpleEntityIndex != nullptr);
	return simpleEntityIndex->size();
}

/*
 * @effects: Ages everyone in the bucket one timestep
 * @returns: List of persons too old for timestep (should always be null)
 */
list<Person *> DmgProfileBucket::ageOneTimeStep()
{
	list<Person *> lP;
	multimap<unsigned long, Person *>::iterator pIter;

	for(pIter = simpleEntityIndex->begin(); pIter != simpleEntityIndex->end(); pIter++)
	{
		(pIter->second)->ageOneTimeUnit();
	}

	return lP;
}
//------------------< End DmgProfileBucket Virtual methods >------------------//


//-----------------< Begin Constructors and Destructors >----------------//
//this function should not be used in this sim, it's just here for a default constructor
DmgProfileBucket::DmgProfileBucket()
{
	simpleEntityIndex = nullptr;
}

//this creates a simple DmgProfileBucket with an index that is sorted by age
DmgProfileBucket::DmgProfileBucket(int _id, const string *_bucketLabel, bool _simpleIndex)
{
	dmgProfileID = _id;
	bucketLabel = _bucketLabel;
	//if this is a simple index, then use field simpleEntityIndex
	simpleEntityIndex = _simpleIndex ? new PersonSet() : nullptr;
}

DmgProfileBucket::~DmgProfileBucket(void)
{
	delete simpleEntityIndex;
}
//-----------------< End Constructors and Destructors >----------------//


//-----------< Begin Methods for DmgProfileBucket::JavaStyleIterator >--------------//
/*
DmgProfileBucket::JavaStyleIterator::JavaStyleIterator() {
}

DmgProfileBucket::JavaStyleIterator::JavaStyleIterator(DmgProfileBucket* _bucket) {
	pIter = _bucket->simpleEntityIndex->iterator();

	//initialize this iterator to iterate from first element
	reset();
}


bool DmgProfileBucket::JavaStyleIterator::alreadyRemoved() {
	return pIter->alreadyRemoved();
}


bool DmgProfileBucket::JavaStyleIterator::hasNext() {
	return pIter->hasNext();
}


Person * DmgProfileBucket::JavaStyleIterator::next(){
	return pIter->next();
}


bool DmgProfileBucket::JavaStyleIterator::remove() {
	//save a pointer to this person
	Person *p = pIter->get();

	//try to remove them from the DmgProfileBucket
	bool removed = pIter->remove();

	//set this person as not being part of any Bucket
	if(removed && p) {
		p->setCurrBucketProfileID(DmgProfile::END);
	}

	return removed;
}


void DmgProfileBucket::JavaStyleIterator::reset() {
	pIter->reset();
}


DmgProfileBucket::JavaStyleIterator::~JavaStyleIterator() {
}
*/
