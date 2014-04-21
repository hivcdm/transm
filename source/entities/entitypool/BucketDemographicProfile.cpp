#include <assert.h>
#include <algorithm>

#include "BucketDemographicProfile.h"
#include "../Person.h"
#include "../../core/Simulation.h"

/**
* mark a function parameter as unused and avoid
* the corresponding compiler warning.
* wrap around the parameter name, e.g. void f(int UNUSED(x))
**/
#define UNUSED(param)

//------------< Begin Implemented Methods >----------------//
//returns the BucketDemographicProfile's ID number
DemographicProfile::ProfileID BucketDemographicProfile::getProfileID()
{
	return dmgProfileID;
}
/**
This method returns entity index for dmgprofilebucket
**/
BucketDemographicProfile::PersonSet *BucketDemographicProfile::getEntityIndex()
{
	return simpleEntityIndex;
}

Person *BucketDemographicProfile::drawMember(RandomNumberGenerator &_randomNums, Person *_chooser, SexualPartnership::Type _partnershipType, bool _remove)
{
    throw std::runtime_error("not imlemented");
}

/**
This method will return a label for this BucketDemographicProfile
**/
const string *BucketDemographicProfile::getLabel()
{
	return bucketLabel;
}

void BucketDemographicProfile::Apply(const PopulationTarget &target, RandomNumberGenerator &rng, std::function<void(Person*)> modifier, double probability)
{
	for(auto &id_person_pair : *simpleEntityIndex)
	{
		auto person = id_person_pair.second;
        if(!((target.age_lower.has_value && target.age_lower.value < person->getAge(TimeGranularity::Month))
            || (target.age_upper.has_value && target.age_upper.value > person->getAge(TimeGranularity::Month))
			|| (target.observed_hiv_status.has_value && target.observed_hiv_status.value != person->getHIVStatus())
			|| (target.on_treatment.has_value && target.on_treatment.value != person->isOnArt())
			|| (target.risk_level.has_value && target.risk_level.value != person->getRiskLevel())))
		{
			if(rng.chance(probability))
			{
				modifier(id_person_pair.second);
			}
		}
	}
}

//------------< End Implemented Methods >----------------//

//------------------< Begin BucketDemographicProfile Virtual methods >------------------//
void BucketDemographicProfile::clear()
{
	assert(simpleEntityIndex != nullptr);
	simpleEntityIndex->clear();
}

Person *BucketDemographicProfile::drawMember(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType, bool _remove)
{
	assert(simpleEntityIndex != nullptr);
	Person *removed = simpleEntityIndex->drawMember(_randomNums, _partnershipType, _remove);

	//we have to tell the person that they are not part of a bucket anymore
	if(_remove && removed)
	{
		removed->setCurrBucketProfileID(DemographicProfile::END);
	}

	return removed;
}

bool BucketDemographicProfile::erase(Person *_person)
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

bool BucketDemographicProfile::exists(Person *_person)
{
	assert(simpleEntityIndex != nullptr);
	assert(_person != nullptr);
	return simpleEntityIndex->exists(_person);
}

unsigned long BucketDemographicProfile::getNumInfected()
{
	assert(simpleEntityIndex != nullptr);
	return simpleEntityIndex->getNumInfected();
}

unsigned long BucketDemographicProfile::getNumInfected(int generation)
{
	assert(simpleEntityIndex != nullptr);
	return simpleEntityIndex->getNumInfected(generation);
}

bool BucketDemographicProfile::insert(Person *_person)
{
	assert(simpleEntityIndex != nullptr);
	assert(_person != nullptr);
	simpleEntityIndex->insert(_person);
	//let the _person know of their new BucketDemographicProfile membership
	_person->setCurrBucketProfileID(getProfileID());
	return true;
}

/*BucketDemographicProfile::JIterator BucketDemographicProfile::iterator() {
	assert(simpleEntityIndex != nullptr);
	return JIterator( new JavaStyleIterator(this) );
}*/

void BucketDemographicProfile::print(std::ostream &_outStream, std::string _prefix)
{
	assert(simpleEntityIndex != nullptr);
	simpleEntityIndex->print(_outStream, _prefix);
}

unsigned long BucketDemographicProfile::size()
{
	assert(simpleEntityIndex != nullptr);
	return simpleEntityIndex->size();
}

/*
 * @effects: Ages everyone in the bucket one timestep
 * @returns: List of persons too old for timestep (should always be null)
 */
list<Person *> BucketDemographicProfile::ageOneTimeStep()
{
	list<Person *> lP;
	multimap<unsigned long, Person *>::iterator pIter;

	for(pIter = simpleEntityIndex->begin(); pIter != simpleEntityIndex->end(); pIter++)
	{
		(pIter->second)->ageOneTimeUnit();
	}

	return lP;
}
//------------------< End BucketDemographicProfile Virtual methods >------------------//


//-----------------< Begin Constructors and Destructors >----------------//
//this function should not be used in this sim, it's just here for a default constructor
BucketDemographicProfile::BucketDemographicProfile()
{
	simpleEntityIndex = nullptr;
}

//this creates a simple BucketDemographicProfile with an index that is sorted by age
BucketDemographicProfile::BucketDemographicProfile(int _id, const string *_bucketLabel, bool _simpleIndex)
{
	dmgProfileID = _id;
	bucketLabel = _bucketLabel;
	//if this is a simple index, then use field simpleEntityIndex
	simpleEntityIndex = _simpleIndex ? new PersonSet() : nullptr;
}

BucketDemographicProfile::~BucketDemographicProfile(void)
{
	delete simpleEntityIndex;
}
//-----------------< End Constructors and Destructors >----------------//


//-----------< Begin Methods for BucketDemographicProfile::JavaStyleIterator >--------------//
/*
BucketDemographicProfile::JavaStyleIterator::JavaStyleIterator() {
}

BucketDemographicProfile::JavaStyleIterator::JavaStyleIterator(BucketDemographicProfile* _bucket) {
	pIter = _bucket->simpleEntityIndex->iterator();

	//initialize this iterator to iterate from first element
	reset();
}


bool BucketDemographicProfile::JavaStyleIterator::alreadyRemoved() {
	return pIter->alreadyRemoved();
}


bool BucketDemographicProfile::JavaStyleIterator::hasNext() {
	return pIter->hasNext();
}


Person * BucketDemographicProfile::JavaStyleIterator::next(){
	return pIter->next();
}


bool BucketDemographicProfile::JavaStyleIterator::remove() {
	//save a pointer to this person
	Person *p = pIter->get();

	//try to remove them from the BucketDemographicProfile
	bool removed = pIter->remove();

	//set this person as not being part of any Bucket
	if(removed && p) {
		p->setCurrBucketProfileID(DemographicProfile::END);
	}

	return removed;
}


void BucketDemographicProfile::JavaStyleIterator::reset() {
	pIter->reset();
}


BucketDemographicProfile::JavaStyleIterator::~JavaStyleIterator() {
}
*/
