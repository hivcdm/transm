#include <sstream>

#include "entitypool.hpp"
#include "bucketsexualmixing.hpp"
#include "core/constants.hpp"
#include "entities/entity.hpp"
#include "utility/utility.hpp"

namespace transm {

//creates a New EntityPool
EntityPool::EntityPool(Age _ageOfMajority, unsigned int _popID, const std::map<SexualPartnership::Type, double> &_assort)
{
	//allocate space for Buckets and set to nullptr
	entityBuckets = std::vector<BucketDemographicProfile *>(DemographicProfile::TotalNumBuckets, nullptr);

	//contains the BucketID's of the buckets we want to use in this simulation
	std::vector<DemographicProfile::ProfileID> validBucketIDs;

	// Instantiate the Male Not-Sexually Active Buckets = (NA, Male, *, Single, nonCSW) -- adds three buckets
	DemographicProfile naMaleSelector;
	naMaleSelector.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Male);
    naMaleSelector.set(DemographicProfile::Demographic::SexualActivityStatus, (std::size_t)DemographicProfile::SexualActivityStatus::NotActive);
    naMaleSelector.set(DemographicProfile::Demographic::RelationshipStatus, (std::size_t)DemographicProfile::RelationshipStatus::Single);
    naMaleSelector.set(DemographicProfile::Demographic::Employment, (std::size_t)DemographicProfile::Employment::NonCsw);
	naMaleSelector.selectProfileIDs(validBucketIDs, nullptr);
    assert(validBucketIDs.size() == 3);

	// Instantiate the Female Not-Sexually Active Buckets = (NA, Female, Msw, Single, nonCSW) -- adds one bucket
	DemographicProfile naFemaleSelector;
	naFemaleSelector.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Female);
	naFemaleSelector.set(DemographicProfile::Demographic::SexualOrientation, (std::size_t)DemographicProfile::SexualOrientation::Msw);
    naFemaleSelector.set(DemographicProfile::Demographic::SexualActivityStatus, (std::size_t)DemographicProfile::SexualActivityStatus::NotActive);
    naFemaleSelector.set(DemographicProfile::Demographic::RelationshipStatus, (std::size_t)DemographicProfile::RelationshipStatus::Single);
    naFemaleSelector.set(DemographicProfile::Demographic::Employment, (std::size_t)DemographicProfile::Employment::NonCsw);
	naFemaleSelector.selectProfileIDs(validBucketIDs, nullptr);
    assert(validBucketIDs.size() == 4);

	// Instantiate the Male Sexually Active Buckets = (SA, Male, *, *, nonCSW) -- adds six buckets
	DemographicProfile saMaleSelector;
	saMaleSelector.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Male);
    saMaleSelector.set(DemographicProfile::Demographic::SexualActivityStatus, (std::size_t)DemographicProfile::SexualActivityStatus::Active);
    saMaleSelector.set(DemographicProfile::Demographic::Employment, (std::size_t)DemographicProfile::Employment::NonCsw);
	saMaleSelector.selectProfileIDs(validBucketIDs, nullptr);
    assert(validBucketIDs.size() == 10);

	// Instantiate the Female Sexually Active Buckets = (SA, Female, Msw, *, *) -- adds four buckets
	DemographicProfile saFemaleSelector;
	saFemaleSelector.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Female);
	saFemaleSelector.set(DemographicProfile::Demographic::SexualOrientation, (std::size_t)DemographicProfile::SexualOrientation::Msw);
    saFemaleSelector.set(DemographicProfile::Demographic::SexualActivityStatus, (std::size_t)DemographicProfile::SexualActivityStatus::Active);
	saFemaleSelector.selectProfileIDs(validBucketIDs, nullptr);
    assert(validBucketIDs.size() == 14);

	//instantiate the spaces for all our buckets. The # of buckets depends on class BucketClassifiers
	//these people are stored in a more complicated BucketDemographicProfile b/c they are involved in sexual mixing
	for(unsigned int i = 0; i < validBucketIDs.size(); ++i)
	{
		DemographicProfile::ProfileID currBucketID = validBucketIDs.at(i);

		//at this index into validBucketIDs, we are still making NA buckets
        if((std::size_t)DemographicProfile::SexualActivityStatus::NotActive ==
			DemographicProfile::get(validBucketIDs.at(i), DemographicProfile::Demographic::SexualActivityStatus))
		{
			// make a Non-Active bucket
			entityBuckets.at(currBucketID) = new BucketDemographicProfile(currBucketID, DemographicProfile::toString(currBucketID), true);
			validProfileIDs.push_back(currBucketID);
		}
		else
		{
			//Special case: CSW can't be in STEADY relationships
            bool invalidCombo = ((std::size_t)DemographicProfile::Employment::Csw ==
				DemographicProfile::get(validBucketIDs.at(i), DemographicProfile::Demographic::Employment)) &&
                ((std::size_t)DemographicProfile::RelationshipStatus::NonSingle == DemographicProfile::get(validBucketIDs.at(i),
					DemographicProfile::Demographic::RelationshipStatus));

			if(!invalidCombo)
			{
				entityBuckets.at(currBucketID) = new BucketSexualMixing(currBucketID, DemographicProfile::toString(currBucketID), _popID,
                    _ageOfMajority, Age::from_months(12 * Entity::maxYrForDeathStats + 1), _assort);
				validProfileIDs.push_back(currBucketID);

			}
		}
	}
}

EntityPool::~EntityPool()
{
	//Delete all people in allFemales and allMales in order to prevent memory leaks
	std::list<Entity *>::iterator p_Iter;

	for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		p_Iter = begin(gender);

		while(p_Iter != end(gender))
		{
			Entity *p = (*p_Iter);
			//Advances p_Iter one in the list, so no increment is necessary
			p_Iter = removeEntityFromAll(p_Iter);
			delete p;
		}
	}

	allMales.clear();
	allFemales.clear();

	//iterate through all buckets and delete them
	for(BaseEnumCls::Enum j = 0; j < entityBuckets.size(); ++j)
	{
		if(entityBuckets.at(j) != nullptr)
		{
            if((std::size_t)DemographicProfile::SexualActivityStatus::NotActive != DemographicProfile::get(entityBuckets.at(j)->getProfileID(), DemographicProfile::Demographic::SexualActivityStatus))
			{
				delete(BucketSexualMixing *)entityBuckets.at(j);
			}
			else
			{
				delete entityBuckets.at(j);
			}
		}
	}
}

void EntityPool::forEach(std::function<void(Entity *)> callback)
{
    for(auto bucket : entityBuckets)
    {
        if(bucket == nullptr)
        {
            continue;
        }

        bucket->forEach(callback);
    }
}

std::vector<DemographicProfile::ProfileID> EntityPool::getProfileIDs()
{
	return validProfileIDs;
}

bool EntityPool::addEntity(Entity *_person)
{
	//gets the BucketDemographicProfile that this person is supposed to be a part of based on their DemographicProfile
	BucketDemographicProfile *bucket = entityBuckets.at(_person->getDemographicProfile()->getProfileID());
	assert(bucket != nullptr);
	//bucket's insert method should take care of the _person->setCurrBucketProfileID(...)
	return bucket->insert(_person);
}

BucketDemographicProfile *EntityPool::getBucket(DemographicProfile::ProfileID _profileID)
{
	return entityBuckets.at(_profileID);
}

//print out all the labels of all the Buckets in the EntityPool. separate each by TAB
//if _printPropInfected == true, then include a column for #infected for each BucketDemographicProfile
void EntityPool::printBucketLabels(ostream &_outStream, bool _printPropInfected)
{
	BucketDemographicProfile *bucket = nullptr;
	size_t currBucketIndex = 0;

	//iterate through all buckets
	while(currBucketIndex < entityBuckets.size())
	{
		bucket = entityBuckets.at(currBucketIndex);

		//if bucket == nullptr, that means we are not using this particular DemographicProfile during this sim
		if(bucket == nullptr)
		{
			currBucketIndex++;
			continue;
		}

		if(_printPropInfected)
		{
			_outStream << *(bucket->getLabel()) << " (Infected)" << Constants::Tab;
		}

		_outStream << (*bucket->getLabel()) << " (Total)" << Constants::Tab;

        if(DemographicProfile::get(bucket->getProfileID(), DemographicProfile::Demographic::SexualActivityStatus) == (std::size_t)DemographicProfile::SexualActivityStatus::Active)
		{
			_outStream << (*bucket->getLabel()) << " (HR)" << Constants::Tab;
			_outStream << (*bucket->getLabel()) << " (LR)" << Constants::Tab;
		}

		currBucketIndex++;
	}
}

//list out # people in each BucketDemographicProfile
void EntityPool::printBucketSizes(std::ostream &_outStream, const std::string &, bool _printPropInfected,
                                  unsigned long &_totalInfected, unsigned long &_totalSize, unsigned long &_totalSexuallyActive,
                                  unsigned long &_totalInSteady, unsigned long &_totalInRegular, bool _includeLabels)
{
	_totalInfected = 0;
	_totalSize = 0;
	_totalInSteady = 0;
	_totalInRegular = 0;
	_totalSexuallyActive = 0;
	BucketDemographicProfile *bucket = nullptr;	//pointer to current BucketDemographicProfile we are looking at
	std::size_t currBucketIndex = 0;		//the ProfileID of the current BucketDemographicProfile we are looking at

	//iterate through all buckets and append current BucketDemographicProfile sizes to a string buffer
	while(currBucketIndex < entityBuckets.size())
	{
		bucket = entityBuckets.at(currBucketIndex);

		if(bucket == nullptr)
		{
			currBucketIndex++;
			continue;
		}

		//will include the label on the same line if _includeLabels == true
		//used for high level debug traces
		if(_includeLabels)
		{
			_outStream << *(bucket->getLabel()) << ":";
		}

		//# people infected in current BucketDemographicProfile
		long numInfected = bucket->getNumInfected();

		//if _printPropInfected == true, print out number of infected folk in the BucketDemographicProfile
		if(_printPropInfected)
		{
			_outStream << numInfected;

			//separate this value by TAB if _includeLabels == false, else separate by '/'
			if(_includeLabels)
			{
				_outStream << "/";
			}
			else
			{
				_outStream << Constants::Tab;
			}
		}

		//print out # people in current BucketDemographicProfile
		long bucketSize = bucket->size();
		_outStream << bucketSize << Constants::Tab;

        if(DemographicProfile::get(bucket->getProfileID(), DemographicProfile::Demographic::SexualActivityStatus) == (std::size_t)DemographicProfile::SexualActivityStatus::Active)
		{
			long bucketSizeHR = ((BucketSexualMixing *) bucket)->sizeRisk(Entity::RiskLevel::HIGH);
			long bucketSizeLR = ((BucketSexualMixing *) bucket)->sizeRisk(Entity::RiskLevel::LOW);
			_outStream << bucketSizeHR << Constants::Tab << bucketSizeLR << Constants::Tab;
		}

		_totalSize += bucketSize;
		_totalInfected += numInfected;
		currBucketIndex++;
	}

	for(list<Entity *>::iterator maleIter = allMales.begin(); maleIter != allMales.end(); maleIter++)
	{
		Entity *male = *maleIter;

        if(male->getDemographicProfile()->get(DemographicProfile::Demographic::SexualActivityStatus) == (std::size_t)DemographicProfile::SexualActivityStatus::Active)
		{
			_totalSexuallyActive++;
		}

		if(male->hasPartnership(SexualPartnership::Type::Steady))
		{
			_totalInSteady++;
		}

		if(male->hasPartnership(SexualPartnership::Type::Regular))
		{
			_totalInRegular++;
		}
	}

	for(list<Entity *>::iterator femaleIter = allFemales.begin(); femaleIter != allFemales.end(); femaleIter++)
	{
		Entity *female = *femaleIter;

        if(female->getDemographicProfile()->get(DemographicProfile::Demographic::SexualActivityStatus) == (std::size_t)DemographicProfile::SexualActivityStatus::Active)
		{
			_totalSexuallyActive++;
		}

		if(female->hasPartnership(SexualPartnership::Type::Steady))
		{
			_totalInSteady++;
		}

		if(female->hasPartnership(SexualPartnership::Type::Regular))
		{
			_totalInRegular++;
		}
	}
}

bool EntityPool::removeEntity(Entity *_person)
{
	assert(_person != nullptr);
	bool removed = false;
	//bucket will take care of the _person->setCurrBucketProfileID(DemographicProfile::END)
	DemographicProfile::ProfileID personProfID = _person->getCurrBucketProfileID();

	if(personProfID >= DemographicProfile::END)
	{
		removed = false;
	}
	else
	{
		try
		{
			auto bucket = getBucket(personProfID);
			removed = bucket->erase(_person);
		}
		catch(std::out_of_range &)
		{
			throw 1;
//			std::cerr << "Trying to remove person from DMG Profile Bucket resulted in an out of range exception: " << e.what() <<
			          //"\n";
			//std::cerr << "If this person is of maximum age, they were probably already removed and you can disregard this message."
			          //<< std::endl;
			//_person->print(cerr, "Person attempted to remove: ");
		}
		catch(std::exception &e)
		{
			std::cerr << "Trying to remove person from DMG Profile Bucket resulted in an exception: " << e.what() << "\n";
		}
	}

	return removed;
}


bool EntityPool::refreshBucketDemographicProfile(Entity *_person, list<Entity *>::iterator * /*_p_Iter*/, bool forceRefresh)
{
	assert(_person != nullptr);
	bool success = false;

	if(!forceRefresh)
	{
		//Don't want to waste time calling this if person is just going to be removed from and re-added to the same bucket
		assert(_person->getCurrBucketProfileID() != _person->getDemographicProfile()->getProfileID());
	}

	try
	{
		success = removeEntity(_person);
	}
	catch(std::out_of_range &e)
	{
		std::cout << "Trying to remove a person threw an out of range error (current profile ID is DemographicProfile::END?): " <<
		          e.what() << "\n";
	}
	catch(std::exception &e)
	{
		std::cout << "Trying to remove a person threw an exception: " << e.what() << "\n";
	}

	return success && addEntity(_person);
}

unsigned long EntityPool::size()
{
	BucketDemographicProfile *bucket = nullptr;
	unsigned long size = 0;		//total of the zie
	size_t currBucketIndex = 0;

	//iterate through all buckets
	while(currBucketIndex < entityBuckets.size())
	{
		bucket = entityBuckets.at(currBucketIndex);

		//if bucket == nullptr, that means we are not using this particular DemographicProfile during this sim
		if(bucket == nullptr)
		{
			currBucketIndex++;
			continue;
		}

		size += entityBuckets.at(currBucketIndex)->size();
		currBucketIndex++;
	}

	return size;
}

//calculate the current number of persons with a given DemographicProfile ID
unsigned long EntityPool::size(DemographicProfile::ProfileID _profileID)
{
	BucketDemographicProfile *bucket = getBucket(_profileID);

	if(bucket == nullptr)
	{
		return 0;
	}
	else
	{
		return bucket->size();
	}
}

//calculate the current number of persons that are not sexually active in the entity pool
unsigned long EntityPool::sizeNotSexuallyActive()
{
	BucketDemographicProfile *bucket = nullptr;
	unsigned long size = 0;		//total of the zie
	size_t currBucketIndex = 0;

	//iterate through all buckets
	while(currBucketIndex < entityBuckets.size())
	{
		bucket = entityBuckets.at(currBucketIndex);

		//if bucket == nullptr, that means we are not using this particular DemographicProfile during this sim
		if(bucket == nullptr)
		{
			currBucketIndex++;
			continue;
		}

		//Only add the sizes of non-sexually active buckets
        if(DemographicProfile::get(bucket->getProfileID(), DemographicProfile::Demographic::SexualActivityStatus) == (std::size_t)DemographicProfile::SexualActivityStatus::NotActive)
		{
			size += entityBuckets.at(currBucketIndex)->size();
		}

		currBucketIndex++;
	}

	return size;
}

//calculate the current number of persons that are not sexually active in the entity pool with a given demographic
std::size_t EntityPool::sizeNotSexuallyActive(const std::string &entity_type)
{
    std::size_t count = 0;
    forEach([&](Entity *e) { if(e->getEntityType() == entity_type) count++; });
    return count;
}

//calculate the current number of persons that are sexually active in the entity pool with a given demographic
std::size_t EntityPool::sizeSexuallyActive(const std::string &entity_type, Entity::RiskLevel _risk)
{
    std::size_t count = 0;
    forEach([&](Entity *e) { if(e->getEntityType() == entity_type && _risk == e->getRiskLevel()) count++; });
    return count;
}

//calculate the current number of sexually active persons within the specified age range
unsigned long EntityPool::sizeSexuallyActiveByAge(Age minAgeMonths, Age maxAgeMonths)
{
	BucketDemographicProfile *bucket = nullptr;
	unsigned long size = 0;		//total of the zie
	size_t currBucketIndex = 0;

	//iterate through all buckets
	while(currBucketIndex < entityBuckets.size())
	{
		bucket = entityBuckets.at(currBucketIndex);

		//if bucket == nullptr, that means we are not using this particular DemographicProfile during this sim
		if(bucket == nullptr)
		{
			currBucketIndex++;
			continue;
		}

		//Only add the sizes of sexually active buckets
        if(DemographicProfile::get(bucket->getProfileID(), DemographicProfile::Demographic::SexualActivityStatus) == (std::size_t)DemographicProfile::SexualActivityStatus::Active)
		{
			size += ((BucketSexualMixing *)(entityBuckets.at(currBucketIndex)))->sizeByAge(minAgeMonths, maxAgeMonths);
		}

		currBucketIndex++;
	}

	return size;
}

//calculate the current number of sexually active persons within the specified age range and gender
std::size_t EntityPool::sizeSexuallyActiveByAge(Age minAgeMonths, Age maxAgeMonths, const std::string &entity_type)
{
    std::size_t count = 0;
    forEach([&](Entity *e)
    {
	if(e->getEntityType() == entity_type && e->isSexuallyActive() &&
	    e->getAge() >= minAgeMonths && e->getAge() <= maxAgeMonths)
		    count++;
    });
    return count;
}

/*
 * adds Person to BucketDemographicProfile AND allMales or allFemales depending on gender
 */
bool EntityPool::addEntityToAll(Entity *_p)
{
    if(_p->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Male)
	{
		allMales.push_back(_p);
	}
	else
	{
        assert(_p->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Female);
		allFemales.push_back(_p);
	}

	return addEntity(_p);
}

/*
 * removes Person from BucketDemographicProfile AND allMales or allFemales depending on gender
 * should be used only when *(_pIter) dies or when deleting this
 */
std::list<Entity *>::iterator EntityPool::removeEntityFromAll(std::list<Entity *>::iterator _pIter)
{
	std::list<Entity *>::iterator toReturn;
	removeEntity(*_pIter);

    if((*_pIter)->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Male)
	{
		toReturn = allMales.erase(_pIter);
	}
    else if((*_pIter)->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Female)
	{
		toReturn = allFemales.erase(_pIter);
	}
	else
	{
		throw 1;
		//(*_pIter)->print(cerr, "Not removing person of indiscriminate gender: ");
		//toReturn = _pIter;
	}

	return toReturn;
}

/*
 * Returns allMales->begin()
 */
list<Entity *>::iterator EntityPool::begin(DemographicProfile::Gender _gender)
{
	if(_gender == DemographicProfile::Gender::Male)
	{
		return allMales.begin();
	}
	else
	{
		return allFemales.begin();
	}
}


/*
 * Returns allMales->end()
 */
list<Entity *>::iterator EntityPool::end(DemographicProfile::Gender _gender)
{
	if(_gender == DemographicProfile::Gender::Male)
	{
		return allMales.end();
	}
	else
	{
		return allFemales.end();
	}
}


void EntityPool::resetPeoplePerAge()
{
	std::fill(malesPerAge.begin(), malesPerAge.end(), 0);
	std::fill(femalesPerAge.begin(), femalesPerAge.end(), 0);
}

void EntityPool::countEntitiesPerAge()
{
	// Just to be sure, we do not want to read old values in case there are age gaps
	resetPeoplePerAge();

	std::list<Entity *>::iterator p_Iter;

	// We count everyone's age in years and store it in two gender-specific arrays
	for (p_Iter = this->begin(DemographicProfile::Gender::Male); p_Iter !=
            this->end(DemographicProfile::Gender::Male); p_Iter++)
	{
		malesPerAge[(*p_Iter)->getAge().in_months()]++;
	}

	for (p_Iter = this->begin(DemographicProfile::Gender::Female); p_Iter !=
            this->end(DemographicProfile::Gender::Female); p_Iter++)
	{
		femalesPerAge[(*p_Iter)->getAge().in_months()]++;
	}
	return;
}

unsigned long EntityPool::sizeByAgeFemales(Age minAgeMonths, Age maxAgeMonths)
{
	int ageMonth = 0;
	unsigned long ageCount = 0;
	for (ageMonth = minAgeMonths.in_months(); ageMonth <= maxAgeMonths.in_months(); ageMonth++)
	{
		ageCount+=femalesPerAge[ageMonth];
	}
	return ageCount;
}

unsigned long EntityPool::sizeByAgeMales(Age minAgeMonths, Age maxAgeMonths)
{
	int ageMonth = 0;
	unsigned long ageCount = 0;
	for (ageMonth = minAgeMonths.in_months(); ageMonth <= maxAgeMonths.in_months(); ageMonth++)
	{
		ageCount+=malesPerAge[ageMonth];
	}
	return ageCount;
}

unsigned long EntityPool::sizeByAge(Age minAgeMonths, Age maxAgeMonths)
{
	int ageMonth = 0;
	unsigned long ageCount = 0;
	for (ageMonth = minAgeMonths.in_months(); ageMonth <= maxAgeMonths.in_months(); ageMonth++)
	{
		ageCount+=femalesPerAge[ageMonth];
		ageCount+=malesPerAge[ageMonth];

	}
	return ageCount;
}

} // namespace transm
