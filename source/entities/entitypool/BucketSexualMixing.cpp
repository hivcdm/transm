#include <iostream>

#include "BucketSexualMixing.h"
#include "core/Simulation.h"
#include "utility/Utility.h"
#include "utility/RandomNumberGenerator.h"

void BucketSexualMixing::forEach(std::function<void(Person *)> callback)
{
    for(auto bucket_age : *this)
    {
        for(auto person : *bucket_age)
        {
            callback(person);
        }
    }
}

BucketSexualMixing::BucketSexualMixing(DemographicProfile::ProfileID _id, const string *_bucketLabel, unsigned int _popID,
    int _minAge, int _maxAge, TimeGranularity _timeGranularity, const std::map<SexualPartnership::Type, double> &_assort) :
	BucketDemographicProfile(_id, _bucketLabel, false),
    assort(_assort)
{
    assert((_timeGranularity == TimeGranularity::Month) || (_timeGranularity == TimeGranularity::Year));
	assert((_minAge >= 0) && (_maxAge >= _minAge));
	timeGranularity = _timeGranularity;
	minAge = _minAge;
	maxAge = _maxAge;
	popID = _popID;

	//ERINWASHERE
	//set capacity of circular buffer
	unsigned int numAgeBuckets = maxAge - minAge + 1;
	personsByAge = new BucketAllAges(numAgeBuckets);
	//personsByAge = new BucketAllAges();
	//initialize all the BucketAges in the BucketSexualMixing, a circular buffer will hold all people of a certain age
	unsigned int currAge = minAge;

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


unsigned int BucketSexualMixing::getCorrectBufferIndex(Person *_p)
{
	//get person's age in right time granularity
	unsigned int pAge = _p->getAge(timeGranularity);

	if(!Utility::withinRange<unsigned int>(pAge, minAge, maxAge))
	{
        cout << "Age is " << _p->getAge(TimeGranularity::Month) << " but minAge is " << minAge << " and max age is " << maxAge <<
		     endl;
	}

	assert(Utility::withinRange(pAge, minAge, maxAge));

	/*//if this person's DemographicProfile doesn't match DemographicProfile, return -1
	if (getProfileID() != _p->getDemographicProfile()->getProfileID()){
		cout << "WRONG PROFILE" << std::endl;
		return (unsigned int)personsByAge->size();
	}*/

	//age determines place in the circular buffer
	if(!Utility::withinRange<unsigned int>(pAge, minAge, maxAge))
	{
		//If person is out of range of this buffer, return -1 which is an invalid entry
		return (unsigned int)personsByAge->size();
	}
	else
	{
		return (pAge - minAge);
	}
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

Person *BucketSexualMixing::drawMember(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType, bool _remove)
{
	if(size() == 0)
	{
		return nullptr;
	}

	//Use minAge and maxAge for bucket
	//default use risk level of low
	//Assortative param will set it to random anyhow
	return getRandomPerson(_randomNums, minAge, maxAge, Person::LOW, _partnershipType, _remove);
}

/***
@param _remove - will remove this person from the BucketSexualMixing if this is true
***/
Person *BucketSexualMixing::drawMember(RandomNumberGenerator &_randomNums, Person *_chooser, SexualPartnership::Type _partnershipType, bool _remove)
{
	//these determine the bounds of which ages we will consider
	int minDesired = 0;
	int maxDesired = INT_MAX;

	//TODO-GA: This method seems unnecessarily convoluted.
	if(_chooser)
	{
		double ageYoungerYears = _chooser->rollForAgeDifference(_partnershipType, _randomNums);
		int ageYoungerMonths = (int)(12 * ageYoungerYears + 0.5);
		//AgeYoungerMonths can be negative so we need to make sure the range stays between both the min and the max age
        minDesired = min(_chooser->getAge(TimeGranularity::Month) - (ageYoungerMonths + 6), (int)(maxAge));
        maxDesired = max(_chooser->getAge(TimeGranularity::Month) - (ageYoungerMonths - 6), (int)(minAge));
		assert(minDesired <= maxDesired);
	}

	//min and max age of Entitys that _chooser can pick from this Person container
	int minAgeDesired = max((int)(minAge), minDesired);
	int maxAgeDesired = min((int)(maxAge), maxDesired);
	return getRandomPerson(_randomNums, minAgeDesired, maxAgeDesired, _chooser->getRiskLevel(), _partnershipType, _remove);
}

//will remove this Person (if he or she exists) from the index
bool BucketSexualMixing::erase(Person *_person)
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
bool BucketSexualMixing::exists(Person *_person)
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
unsigned long BucketSexualMixing::getNumInfected(Person::RiskLevel _risk)
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

Person *BucketSexualMixing::getRandomPerson(RandomNumberGenerator &_randomNums, unsigned int _ageLowerBound,
        unsigned int _ageUpperBound, Person::RiskLevel _risk, SexualPartnership::Type _partnershipType, bool _remove)
{
	if(_ageLowerBound < minAge)
	{
		//Set this to avoid negative unsigned int, which is just asking for trouble
		_ageLowerBound = minAge;
	}

	//get the age buckets that we will search within
	unsigned int minIndex = std::max<unsigned int>(_ageLowerBound - minAge, 0);
	unsigned int maxIndex = std::min<unsigned int>(_ageUpperBound - minAge,
	                        (unsigned int)personsByAge->size() - 1);
	//Use assort to determine whether to use random or _risk bin
	Person::RiskLevel riskToDraw;

	if(_randomNums.chance(assort[_partnershipType]))
	{
		riskToDraw = _risk;
	}
	else
	{
		riskToDraw = Person::ENDRiskLevel;
	}

	//figure out # of eligible people
	unsigned int currIndex = minIndex;
	unsigned long numMarbles = 0;

	while(currIndex <= maxIndex)
	{
		numMarbles += personsByAge->at(currIndex)->numChoices(riskToDraw);
		currIndex++;
	}

	//If no one is within the requested age range, return nullptr.
	if(numMarbles == 0)
	{
		//Recursively expand search with _ageLowerBound and _ageUpperBound expanding by 6 months
		//Only recurse if there are partners available in this bucket to save time
		numMarbles = 0;

		for(currIndex = 0; currIndex < personsByAge->size(); currIndex++)
		{
			numMarbles += personsByAge->at(currIndex)->numChoices(riskToDraw);
		}

		if(numMarbles > 0)
		{
			//Check first that minIndex > 0 and maxIndex < personsByAge->size() - 1
			if(minIndex > 0 || maxIndex < personsByAge->size() - 1)
			{
				unsigned int newAgeLowerBound = _ageLowerBound;

				//Make sure the ageLowerBound being recursively passed in is non-negative to avoid those pesky unsigned int issues
				if(newAgeLowerBound < 6)
				{
					newAgeLowerBound = 0;
				}
				else
				{
					newAgeLowerBound = _ageLowerBound - 6;
				}

				return getRandomPerson(_randomNums, newAgeLowerBound, _ageUpperBound + 6, _risk, _partnershipType, _remove);
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
		if(randPick < personsByAge->at(currIndex)->numChoices(riskToDraw))
		{
			BucketAge *ageBucket = personsByAge->at(currIndex);
			Person *p = ageBucket->drawMember(_randomNums, _risk, _partnershipType, (riskToDraw == Person::ENDRiskLevel), _remove);

			//we have to tell the person that they are not part of a bucket anymore
			if(_remove && p)
			{
				p->setCurrBucketProfileID(DemographicProfile::END);
			}

			return p;
		}

		randPick -= personsByAge->at(currIndex)->numChoices(riskToDraw);
		currIndex++;
	}

	return nullptr;
}

//will index a New Person
bool BucketSexualMixing::insert(Person *_person)
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
		cerr << "Trying to insert person with invalid age (" << _person->getAge(timeGranularity) << " " << ((
            timeGranularity == TimeGranularity::Month) ? "months" : "years") << ")" << std::endl;
		cerr << "Valid ages are between " << minAge << " and " <<  maxAge << " inclusive" << std::endl;
		cerr << "If age is valid, person may have an invalid DemographicProfile";
		_person->print(cerr, "");
		return false;
	}
}

void BucketSexualMixing::print(ostream &_outStream, const std::string &_prefix)
{
	//The iterator of the BucketAges in the circular buffer
	BucketAllAges::iterator bucketIter;

	for(bucketIter = begin(); bucketIter != end(); bucketIter++)
	{
		(*bucketIter)->print(_outStream, _prefix);
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


//-------------< Begin iterator methods >------------------//
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
//--------------< End iterator methods >-------------------//
//-------< Begin additional methods based on this structure >-------//
/*
 * @returns: total number of persons in this with age between minMonthAge and maxMonthAge
 */
unsigned long BucketSexualMixing::sizeByAge(int minMonthAge, int maxMonthAge)
{
	//get the age buckets that we will count within
	unsigned int minIndex = std::max<unsigned int>(minMonthAge - minAge, 0);
	unsigned int maxIndex = std::min<unsigned int>(maxMonthAge - minAge,
	                        (unsigned int)personsByAge->size() - 1);
	unsigned int currIndex = minIndex;
	unsigned long total = 0;

	while(currIndex <= maxIndex)
	{
		total += personsByAge->at(currIndex)->size();
		currIndex++;
	}

	return total;
}

/*
 * @returns: total number of infected persons in this with age between minMonthAge and maxMonthAge
 */
unsigned long BucketSexualMixing::sizeInfectedByAge(int minMonthAge, int maxMonthAge)
{
	//get the age buckets that we will count within
	unsigned int minIndex = std::max<unsigned int>(minMonthAge - minAge, 0);
	unsigned int maxIndex = std::min<unsigned int>(maxMonthAge - minAge,
	                        (unsigned int)personsByAge->size() - 1);
	unsigned int currIndex = minIndex;
	unsigned long total = 0;

	while(currIndex <= maxIndex)
	{
		total += personsByAge->at(currIndex)->getNumInfected();
		currIndex++;
	}

	return total;
}

/*
 * @returns: total number of marbles in all FVs associated with _risk
 * BucketAges in this
 */
unsigned long BucketSexualMixing::sizeRisk(Person::RiskLevel _risk)
{
	assert(_risk <= Person::ENDRiskLevel);
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
unsigned long BucketSexualMixing::sizeRiskCSW(Person::RiskLevel _risk)
{
	assert(_risk <= Person::ENDRiskLevel);
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
unsigned long BucketSexualMixing::sizeRiskHIVStatus(Person::RiskLevel _risk, Person::HIVStatus _hivStatus)
{
	assert(_risk <= Person::ENDRiskLevel);
	assert(_hivStatus <= Person::ENDHIVStatus);
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
bool BucketSexualMixing::increaseInfected(Person *_person)
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
void BucketSexualMixing::changeHIVStatus(Person *_p, Person::HIVStatus _orig, Person::HIVStatus _new)
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
list<Person *> BucketSexualMixing::ageOneTimeStep()
{
	//Kill off the oldest
	BucketAge *oldestPersons = getOldest();
	//Iterate through all oldest remove them from this and put them in list of "expired" persons to be returned
	list<Person *> toReturn;
	vector<Person *>::iterator personIterator = oldestPersons->begin();

	while(personIterator != oldestPersons->end() && oldestPersons->size() > 0)
	{
		try
		{
			Person *oldPerson = (*personIterator);
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
	assert(personsByAge->size() <= maxAge - minAge + 1);
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
