#include <iostream>

#include "bucketsexualmixing.hpp"
#include "core/simulation.hpp"
#include "utility/utility.hpp"
#include "utility/randomnumbergenerator.hpp"

namespace transm {

void BucketSexualMixing::forEach(std::function<void(Entity *)> callback)
{
    for(auto bucket_age : *this)
    {
        for(auto person : *bucket_age)
        {
            callback(person);
        }
    }
}

BucketSexualMixing::BucketSexualMixing(DemographicProfile::ProfileID _id, 
	const string *_bucketLabel, unsigned int _popID, Age _minAge, Age _maxAge, 
	const std::map<SexualPartnership::Type, double> &_assort) :
	BucketDemographicProfile(_id, _bucketLabel, false),
    assort(_assort)
{
	assert(_minAge.in_months() >= 0);
	assert(_maxAge >= _minAge);

	minAge = _minAge;
	maxAge = _maxAge;
	popID = _popID;

	//ERINWASHERE
	//set capacity of circular buffer
	unsigned int numAgeBuckets = (int)(maxAge - minAge).in_months() + 1;
	personsByAge = new BucketAllAges(numAgeBuckets);
	//personsByAge = new BucketAllAges();
	//initialize all the BucketAges in the BucketSexualMixing, a circular buffer will hold all people of a certain age
	auto currAge = minAge;

	while(currAge <= maxAge)
	{
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


unsigned int BucketSexualMixing::getCorrectBufferIndex(Entity *_p)
{
	//get person's age in right time granularity
	auto pAge = _p->getAge();

	if(pAge < minAge || pAge > maxAge)
	{
		throw std::runtime_error("Age is " + std::to_string(pAge.in_months()) + " but minAge is "
			+ std::to_string(minAge.in_months()) + " and max age is " + std::to_string(maxAge.in_months()));
	}

	return static_cast<unsigned int>((pAge - minAge).in_months());
}

//clears all elements from this index without deleting members
void BucketSexualMixing::clear()
{
	BucketAllAges::iterator agesIter;

	//go through each BucketAge and clear it
	for(agesIter = personsByAge->begin(); agesIter != personsByAge->end(); agesIter++)
	{
		(*agesIter)->clear();
	}
}

Entity *BucketSexualMixing::drawMember(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType, bool _remove)
{
	if(size() == 0)
	{
		return nullptr;
	}

	//Use minAge and maxAge for bucket
	//default use risk level of low
	//Assortative param will set it to random anyhow
	return getRandomPerson(_randomNums, minAge, maxAge, Entity::RiskLevel::LOW, _partnershipType, _remove);
}

/***
@param _remove - will remove this person from the BucketSexualMixing if this is true
***/
Entity *BucketSexualMixing::drawMember(RandomNumberGenerator &_randomNums, Entity *_chooser, SexualPartnership::Type _partnershipType, bool _remove)
{
	//these determine the bounds of which ages we will consider
	auto minDesired = Age::from_months(0);
	auto maxDesired = Age::from_months(INT_MAX);

	//TODO-GA: This method seems unnecessarily convoluted.
	if(_chooser)
	{
		double ageYoungerYears = _chooser->rollForAgeDifference(_partnershipType, _randomNums);
		TimeSpan ageDifference(0, (int)(12 * ageYoungerYears + 0.5));
		//AgeYoungerMonths can be negative so we need to make sure the range stays between both the min and the max age
        minDesired = min(_chooser->getAge() - (ageDifference + TimeSpan(0, 6)), maxAge);
		maxDesired = max(_chooser->getAge() - (ageDifference - TimeSpan(0, 6)), minAge);
		assert(minDesired <= maxDesired);
	}

	//min and max age of Entitys that _chooser can pick from this Person container
	auto minAgeDesired = max(minAge, minDesired);
	auto maxAgeDesired = min(maxAge, maxDesired);
	return getRandomPerson(_randomNums, minAgeDesired, maxAgeDesired, _chooser->getRiskLevel(), _partnershipType, _remove);
}

//will remove this Person (if he or she exists) from the index
bool BucketSexualMixing::erase(Entity *_person)
{
	assert(_person != nullptr);
	unsigned int correctIndex = getCorrectBufferIndex(_person);

	//if this person wouldn't be in this BucketDemographicProfile, then return false
	if(correctIndex >= personsByAge->size())
	{
		return false;
	}

	//bool removed =  entitiesByAge->at(correctIndex)->erase(_person);
	bool removed = personsByAge->at(correctIndex)->erase(_person);

	if(removed)
	{
		_person->setCurrBucketProfileID(DemographicProfile::END);
	}

	return removed;
}

//tells whether _person exists in the index
bool BucketSexualMixing::exists(Entity *_person)
{
	unsigned int correctIndex = getCorrectBufferIndex(_person);

	//if this person wouldn't be in this BucketDemographicProfile, then return false
	if(correctIndex >= personsByAge->size())
	{
		return false;
	}

	return personsByAge->at(correctIndex)->exists(_person);
}

//counts number of infected people this EntityPool
unsigned long BucketSexualMixing::getNumInfected()
{
	BucketAllAges::iterator ageBucketIter;
	unsigned long totalInfected = 0;

	//go through each Index and get # infected.
	for(ageBucketIter = personsByAge->begin(); ageBucketIter != personsByAge->end(); ageBucketIter++)
	{
		totalInfected += (*ageBucketIter)->getNumInfected();
	}

	return totalInfected;
}

//counts number of infected people this EntityPool by generation
unsigned long BucketSexualMixing::getNumInfected(int generation)
{
	BucketAllAges::iterator ageBucketIter;
	unsigned long totalInfected = 0;

	//go through each Index and get # infected.
	for(ageBucketIter = personsByAge->begin(); ageBucketIter != personsByAge->end(); ageBucketIter++)
	{
		totalInfected += (*ageBucketIter)->getNumInfected(generation);
	}

	return totalInfected;
}

/*
 * @returns: total number of infected persons in this risk group
 */
unsigned long BucketSexualMixing::getNumInfected(Entity::RiskLevel _risk)
{
	BucketAllAges::iterator ageBucketIter;
	unsigned long totalInfected = 0;

	//go through each Index and get # infected by risk.
	for(ageBucketIter = personsByAge->begin(); ageBucketIter != personsByAge->end(); ageBucketIter++)
	{
		totalInfected += (*ageBucketIter)->getNumInfected(_risk);
	}

	return totalInfected;
}

Entity *BucketSexualMixing::getRandomPerson(RandomNumberGenerator &_randomNums, Age _ageLowerBound,
        Age _ageUpperBound, Entity::RiskLevel _risk, SexualPartnership::Type _partnershipType, bool _remove)
{
	if(_ageLowerBound < minAge)
	{
		//Set this to avoid negative unsigned int, which is just asking for trouble
		_ageLowerBound = minAge;
	}

	//get the age buckets that we will search within
	auto minIndex = max(_ageLowerBound - minAge, TimeSpan(0, 0));
	auto maxIndex = min(_ageUpperBound - minAge, TimeSpan(0, (int)personsByAge->size() - 1));

	//Use assort to determine whether to use random or _risk bin
    //Use random by default
    Entity::RiskLevel riskToDraw = Entity::RiskLevel::Last;

	if(_randomNums.chance(assort[_partnershipType]))
	{
		riskToDraw = _risk;
	}

	//figure out # of eligible people
	auto currIndex = minIndex;
	unsigned long numMarbles = 0;

	while(currIndex <= maxIndex)
	{
		numMarbles += personsByAge->at((std::size_t)currIndex.in_months())->numChoices(riskToDraw);
		currIndex++;
	}

	//If no one is within the requested age range, return nullptr.
	if(numMarbles == 0)
	{
		//Recursively expand search with _ageLowerBound and _ageUpperBound expanding by 6 months
		//Only recurse if there are partners available in this bucket to save time
		numMarbles = 0;

		for(currIndex = TimeSpan(0, 0); currIndex.in_months() < (double)personsByAge->size(); currIndex++)
		{
			numMarbles += personsByAge->at((std::size_t)currIndex.in_months())->numChoices(riskToDraw);
		}

		if(numMarbles > 0)
		{
			//Check first that minIndex > 0 and maxIndex < personsByAge->size() - 1
			if(minIndex.in_months() > 0 || maxIndex.in_months() < (int)personsByAge->size() - 1)
			{
				auto newAgeLowerBound = _ageLowerBound;

				//Make sure the ageLowerBound being recursively passed in is non-negative to avoid those pesky unsigned int issues
				if(newAgeLowerBound < Time::from_months(6))
				{
					newAgeLowerBound = Time::from_months(0);
				}
				else
				{
					newAgeLowerBound = _ageLowerBound - TimeSpan(0, 6);
				}

				return getRandomPerson(_randomNums, newAgeLowerBound, _ageUpperBound + TimeSpan(0, 6), _risk, _partnershipType, _remove);
			}
			else
			{
				return nullptr;
			}
		}
		else
		{
			return nullptr;
		}
	}

	//use the randPick to determine which AgeBucket to draw from
	int randPick = _randomNums.randInt(0, numMarbles - 1);
	//keep looping through all the indexes within personsByAge until we get the one that contains the randPick'th person with age between [_ageLowerBound, _ageUpperBound]
	currIndex = minIndex;

	while(currIndex <= maxIndex)
	{
		//if we're at the right AgeBucket
		if(randPick < personsByAge->at((std::size_t)currIndex.in_months())->numChoices(riskToDraw))
		{
			BucketAge *ageBucket = personsByAge->at((std::size_t)currIndex.in_months());
			Entity *p = ageBucket->drawMember(_randomNums, _risk, _partnershipType, (riskToDraw == Entity::RiskLevel::Last), _remove);

			//we have to tell the person that they are not part of a bucket anymore
			if(_remove && p)
			{
				p->setCurrBucketProfileID(DemographicProfile::END);
			}

			return p;
		}

		randPick -= personsByAge->at((std::size_t)currIndex.in_months())->numChoices(riskToDraw);
		currIndex++;
	}

	return nullptr;
}

//will index a New Person
bool BucketSexualMixing::insert(Entity *_person)
{
	//Don't allow person with wrong DemographicProfile to be inserted
	if(_person->getDemographicProfile()->getProfileID() != getProfileID())
	{
		return false;
	}

	unsigned int correctIndex = getCorrectBufferIndex(_person);

	//if this person belongs in this BucketDemographicProfile
	if(correctIndex < personsByAge->size())
	{
		//check to see that this person isn't already in here
		if(personsByAge->at(correctIndex)->exists(_person))
		{
			return false;
		}

		bool inserted = personsByAge->at(correctIndex)->insert(_person);

		if(inserted)
		{
			_person->setCurrBucketProfileID(getProfileID());
		}

		return inserted;
	}
	else
	{
		throw 1;
//		cerr << "Trying to insert person with invalid age (" << _person->getAge().in_months() << " months)" << std::endl;
///		cerr << "Valid ages are between " << minAge.in_months() << " and " <<  maxAge.in_months() << " inclusive" << std::endl;
//		cerr << "If age is valid, person may have an invalid DemographicProfile";
		//_person->print(cerr, "");
//		return false;
	}
}

//returns the # of entities in this index
unsigned long BucketSexualMixing::size()
{
	BucketAllAges::iterator bucketIter;
	unsigned long total = 0;

	//go through each Index and get # infected.
	for(bucketIter = personsByAge->begin(); bucketIter != personsByAge->end(); bucketIter++)
	{
		total += (*bucketIter)->size();
	}

	return total;
}

BucketSexualMixing::BucketAllAges::iterator BucketSexualMixing::begin()
{
	return personsByAge->begin();
}

BucketSexualMixing::BucketAllAges::iterator BucketSexualMixing::end()
{
	return personsByAge->end();
}

BucketAge *BucketSexualMixing::getOldest()
{
	return personsByAge->back();
}

BucketAge *BucketSexualMixing::getYoungest()
{
	return personsByAge->front();
}

/*
 * @returns: total number of persons in this with age between minMonthAge and maxMonthAge
 */
unsigned long BucketSexualMixing::sizeByAge(Age minMonthAge, Age maxMonthAge)
{
	//get the age buckets that we will count within
	auto minIndex = max(minMonthAge - minAge, TimeSpan(0, 0));
	auto maxIndex = min(maxMonthAge - minAge, TimeSpan(0, (int)personsByAge->size() - 1));
	auto currIndex = minIndex;
	unsigned long total = 0;

	while(currIndex <= maxIndex)
	{
		total += personsByAge->at((std::size_t)currIndex.in_months())->size();
		currIndex++;
	}

	return total;
}

/*
 * @returns: total number of infected persons in this with age between minMonthAge and maxMonthAge
 */
unsigned long BucketSexualMixing::sizeInfectedByAge(Age minMonthAge, Age maxMonthAge)
{
	//get the age buckets that we will count within
	auto minIndex = max(minMonthAge - minAge, TimeSpan(0, 0));
	auto maxIndex = min(maxMonthAge - minAge, TimeSpan(0, (int)personsByAge->size() - 1));
	auto currIndex = minIndex;
	unsigned long total = 0;

	while(currIndex <= maxIndex)
	{
		total += personsByAge->at((std::size_t)currIndex.in_months())->getNumInfected();
		currIndex++;
	}

	return total;
}

/*
 * @returns: total number of marbles in all FVs associated with _risk
 * BucketAges in this
 */
unsigned long BucketSexualMixing::sizeRisk(Entity::RiskLevel _risk)
{
	BucketAllAges::iterator bucketIter;
	unsigned long total = 0;

	//go through each Index and get # random marbles
	for(bucketIter = personsByAge->begin(); bucketIter != personsByAge->end(); bucketIter++)
	{
		total += (*bucketIter)->getNumRisk(_risk);
	}

	return total;
}

/*
 * @returns: total number of unique persons in this bucket that is CSW with given _risk
 * BucketAges in this
 */
unsigned long BucketSexualMixing::sizeRiskCSW(Entity::RiskLevel _risk)
{
	BucketAllAges::iterator bucketIter;
	unsigned long total = 0;

	//go through each Index and get # random marbles
	for(bucketIter = personsByAge->begin(); bucketIter != personsByAge->end(); bucketIter++)
	{
		total += (*bucketIter)->getNumRiskCSW(_risk);
	}

	return total;
}
/*
 * @returns: total number of unique persons in this bucket with given risk level and hiv status
 * across all BucketAges in this;
 */
unsigned long BucketSexualMixing::sizeRiskHIVStatus(Entity::RiskLevel _risk, Entity::HIVStatus _hivStatus)
{
	BucketAllAges::iterator bucketIter;
	unsigned long total = 0;

	//go through each Index and get # random marbles
	for(bucketIter = personsByAge->begin(); bucketIter != personsByAge->end(); bucketIter++)
	{
		total += (*bucketIter)->getNumRiskHIVStatus(_risk, _hivStatus);
	}

	return total;
}

/*
 * @returns: total number of marbles in all Random Risk FVs across all
 * BucketAges in this
 */
unsigned long BucketSexualMixing::sizeRandom()
{
	BucketAllAges::iterator bucketIter;
	unsigned long total = 0;

	//go through each Index and get # random marbles
	for(bucketIter = personsByAge->begin(); bucketIter != personsByAge->end(); bucketIter++)
	{
		total += (*bucketIter)->numRandomRiskChoices();
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
bool BucketSexualMixing::increaseInfected(Entity *_person)
{
	if(!(_person->isInfected()))
	{
		//_person isn't infected
		return false;
	}

	int index = getCorrectBufferIndex(_person);

	if(index >= static_cast<int>(personsByAge->size()))
	{
		//_person is not in this Bucket collection
		return false;
	}

	return personsByAge->at(index)->increaseInfected(_person);
}

/* @function: changeHIVstatus
 * @effects: if person is in this Bucket and thier hiv status changes decrement the old status and increment new status
 */
void BucketSexualMixing::changeHIVStatus(Entity *_p, Entity::HIVStatus _orig, Entity::HIVStatus _new)
{
	int index = getCorrectBufferIndex(_p);

	if(index >= static_cast<int>(personsByAge->size()))
	{
		return;
	}

	personsByAge->at(index)->changeHIVStatus(_p, _orig, _new);
}

/*
 * @effects: Sets all persons in oldest BucketAge to die and processes their
 * deaths, pushes new BucketAge to front (for incoming births), sets all other persons
 * to age++ (to account for new index of BucketAge
 * @returns: List of persons expired out of this (of old age)
 */
list<Entity *> BucketSexualMixing::ageOneTimeStep()
{
	//Kill off the oldest
	BucketAge *oldestPersons = getOldest();
	//Iterate through all oldest remove them from this and put them in list of "expired" persons to be returned
	std::list<Entity *> toReturn;
	auto personIterator = oldestPersons->begin();

	while(personIterator != oldestPersons->end() && oldestPersons->size() > 0)
	{
		try
		{
			Entity *oldPerson = (*personIterator);
			//We're going to remove oldPerson, so advance the iterator now before it gets confused
			personIterator++;
			toReturn.push_back(oldPerson);
			erase(oldPerson);
			oldPerson->ageOneTimeUnit();
		}
		catch(std::exception &e)
		{
			cout << e.what() << std::endl;
			personIterator = oldestPersons->end();
		}
	}

	//Insert the new BucketAge for incoming youngest
	BucketAge *pNewYoungest = new BucketAge(getProfileID(), popID, assort);
	personsByAge->push_front(pNewYoungest);
	//Delete the OldestPersons bucket
	//personsByAge->pop_back();
	delete oldestPersons;
	assert(TimeSpan(0, (int)personsByAge->size()) <= maxAge - minAge + TimeSpan::Month);
	//Iterate through and age everyone
	BucketAllAges::iterator bucketIter;

	for(bucketIter = begin(); bucketIter != end(); bucketIter++)
	{
		//The iterator of all (unique) persons in the BucketAge
		for(personIterator = (*bucketIter)->begin(); personIterator != (*bucketIter)->end(); personIterator++)
		{
			(*personIterator)->ageOneTimeUnit();
			assert(exists(*personIterator));
		}
	}

	return toReturn;
}

} // namespace transm
