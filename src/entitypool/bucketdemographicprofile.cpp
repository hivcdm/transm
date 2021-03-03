#include <assert.h>
#include <algorithm>

#include "bucketdemographicprofile.hpp"
#include "core/simulation.hpp"

namespace transm {

void BucketDemographicProfile::forEach(std::function<void(Entity *)> callback)
{
    for(auto person : *this)
    {
        callback(person.second);
    }
}

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

Entity *BucketDemographicProfile::drawMember(RandomNumberGenerator &, Entity *, SexualPartnership::Type, bool)
{
    throw std::runtime_error("not implemented");
}

/**
This method will return a label for this BucketDemographicProfile
**/
const std::string *BucketDemographicProfile::getLabel()
{
	return bucketLabel;
}

void BucketDemographicProfile::clear()
{
	assert(simpleEntityIndex != nullptr);
	simpleEntityIndex->clear();
}

Entity *BucketDemographicProfile::drawMember(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType, bool _remove)
{
	assert(simpleEntityIndex != nullptr);
	Entity *removed = simpleEntityIndex->drawMember(_randomNums, _partnershipType, _remove);

	//we have to tell the person that they are not part of a bucket anymore
	if(_remove && removed)
	{
		removed->setCurrBucketProfileID(DemographicProfile::END);
	}

	return removed;
}

bool BucketDemographicProfile::erase(Entity *_person)
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

bool BucketDemographicProfile::exists(Entity *_person)
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

bool BucketDemographicProfile::insert(Entity *_person)
{
	assert(simpleEntityIndex != nullptr);
	assert(_person != nullptr);
	simpleEntityIndex->insert(_person);
	//let the _person know of their new BucketDemographicProfile membership
	_person->setCurrBucketProfileID(getProfileID());
	return true;
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
std::list<Entity *> BucketDemographicProfile::ageOneTimeStep()
{
	std::list<Entity *> lP;

	for(auto pIter = simpleEntityIndex->begin(); pIter != simpleEntityIndex->end(); pIter++)
	{
		(pIter->second)->ageOneTimeUnit();
	}

	return lP;
}

//this function should not be used in this sim, it's just here for a default constructor
BucketDemographicProfile::BucketDemographicProfile()
{
	simpleEntityIndex = nullptr;
}

//this creates a simple BucketDemographicProfile with an index that is sorted by age
BucketDemographicProfile::BucketDemographicProfile(int _id, const std::string *_bucketLabel, bool _simpleIndex)
{
	dmgProfileID = _id;
	bucketLabel = _bucketLabel;
	//if this is a simple index, then use field simpleEntityIndex
	simpleEntityIndex = _simpleIndex ? new PersonSet() : nullptr;
}

BucketDemographicProfile::~BucketDemographicProfile()
{
	delete simpleEntityIndex;
}

} // namespace transm
