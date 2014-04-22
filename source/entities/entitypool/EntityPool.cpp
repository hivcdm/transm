/**
This file contains the implementations for the methods of EntityPool
***/

#include <sstream>

#include "EntityPool.h"
#include "../../core/Constants.h"
#include "../../util/Utility.h"
#include "../Person.h"
#include "BucketSexualMixing.h"

bool EntityPool::addEntity(Person *_person)
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

void EntityPool::print(ostream &_outStream)
{
	BucketDemographicProfile *bucket = nullptr;
	//iterate through all buckets
	size_t currBucketIndex = 0;

	while(currBucketIndex < entityBuckets.size())
	{
		bucket = entityBuckets.at(currBucketIndex);

		//if bucket == nullptr, that means we are not using this particular DemographicProfile during this simulation
		if(bucket == nullptr)
		{
			currBucketIndex++;
			continue;
		}

		_outStream << *(bucket->getLabel()) << Constants::COLON << Constants::TAB << Constants::TAB << "Gender" <<
		           Constants::TAB << "ID" << Constants::TAB << "Profile" << Constants::TAB << "Age" << Constants::TAB << "cd4" <<
		           Constants::TAB << "hvl" << std::endl;
		//print out all members
		bucket->print(_outStream, Constants::TABTAB);
		_outStream << std::endl;
		currBucketIndex++;
	} //while(currBucketIndex < entityBuckets.size()) {
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
			_outStream << *(bucket->getLabel()) << " (Infected)" << Constants::TAB;
		}

		_outStream << (*bucket->getLabel()) << " (Total)" << Constants::TAB;

		if(DemographicProfile::get(bucket->getProfileID(), DemographicProfile::SEXUAL_ACTIVITY_STATUS) == DemographicProfile::SA)
		{
			_outStream << (*bucket->getLabel()) << " (HR)" << Constants::TAB;
			_outStream << (*bucket->getLabel()) << " (LR)" << Constants::TAB;
		}

		currBucketIndex++;
	} //while(currBucketIndex < entityBuckets.size()) {
}

//list out # people in each BucketDemographicProfile
void EntityPool::printBucketSizes(ostream &_outStream, string, bool _printPropInfected,
                                  unsigned long &_totalInfected, unsigned long &_totalSize, unsigned long &_totalSexuallyActive,
                                  unsigned long &_totalInSteady, unsigned long &_totalInRegular, bool _includeLabels)
{
	_totalInfected = 0;
	_totalSize = 0;
	_totalInSteady = 0;
	_totalInRegular = 0;
	_totalSexuallyActive = 0;
	BucketDemographicProfile *bucket = nullptr;	//pointer to current BucketDemographicProfile we are looking at
	size_t currBucketIndex = 0;		//the ProfileID of the current BucketDemographicProfile we are looking at

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
				_outStream << Constants::TAB;
			}
		}

		//print out # people in current BucketDemographicProfile
		long bucketSize = bucket->size();
		_outStream << bucketSize << Constants::TAB;

		if(DemographicProfile::get(bucket->getProfileID(), DemographicProfile::SEXUAL_ACTIVITY_STATUS) == DemographicProfile::SA)
		{
			long bucketSizeHR = ((BucketSexualMixing *) bucket)->sizeRisk(Person::HIGH);
			long bucketSizeLR = ((BucketSexualMixing *) bucket)->sizeRisk(Person::LOW);
			_outStream << bucketSizeHR << Constants::TAB << bucketSizeLR << Constants::TAB;
		}

		_totalSize += bucketSize;
		_totalInfected += numInfected;
		currBucketIndex++;
	} //while(currBucketIndex < entityBuckets.size()) {

	for(list<Person *>::iterator maleIter = allMales.begin(); maleIter != allMales.end(); maleIter++)
	{
		Person *male = *maleIter;

		if(male->getDemographicProfile()->get(DemographicProfile::SEXUAL_ACTIVITY_STATUS) == DemographicProfile::SA)
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

	for(list<Person *>::iterator femaleIter = allFemales.begin(); femaleIter != allFemales.end(); femaleIter++)
	{
		Person *female = *femaleIter;

		if(female->getDemographicProfile()->get(DemographicProfile::SEXUAL_ACTIVITY_STATUS) == DemographicProfile::SA)
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

bool EntityPool::removeEntity(Person *_person)
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
		catch(std::out_of_range &e)
		{
			std::cerr << "Trying to remove person from DMG Profile Bucket resulted in an out of range exception: " << e.what() <<
			          "\n";
			std::cerr << "If this person is of maximum age, they were probably already removed and you can disregard this message."
			          << std::endl;
			_person->print(cerr, "Person attempted to remove: ");
		}
		catch(std::exception &e)
		{
			std::cerr << "Trying to remove person from DMG Profile Bucket resulted in an exception: " << e.what() << "\n";
		}
	}

	return removed;
}


bool EntityPool::refreshBucketDemographicProfile(Person *_person, list<Person *>::iterator * /*_p_Iter*/, bool forceRefresh)
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
	}	//while(currBucketIndex < entityBuckets.size()) {

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
		if(DemographicProfile::get(bucket->getProfileID(), DemographicProfile::SEXUAL_ACTIVITY_STATUS) == DemographicProfile::NA)
		{
			size += entityBuckets.at(currBucketIndex)->size();
		}

		currBucketIndex++;
	}	//while(currBucketIndex < entityBuckets.size()) {

	return size;
}

//calculate the current number of persons that are not sexually active in the entity pool with a given demographic
unsigned long EntityPool::sizeNotSexuallyActive(DemographicProfile::Gender _gender)
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

		//Only add the sizes of non-sexually active buckets that match demographic profile
		if(DemographicProfile::get(bucket->getProfileID(), DemographicProfile::SEXUAL_ACTIVITY_STATUS) == DemographicProfile::NA)
		{
			if(DemographicProfile::get(bucket->getProfileID(), DemographicProfile::GENDER) == static_cast<BaseEnumCls::Enum>(_gender))
			{
				size += entityBuckets.at(currBucketIndex)->size();
			}
		}

		currBucketIndex++;
	}	//while(currBucketIndex < entityBuckets.size()) {

	return size;
}

//calculate the current number of persons that are sexually active in the entity pool with a given demographic
unsigned long EntityPool::sizeSexuallyActive(DemographicProfile::Gender _gender, Person::RiskLevel _risk)
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

		//Only add the sizes of sexually active buckets that match demographic profile
		if(DemographicProfile::get(bucket->getProfileID(), DemographicProfile::SEXUAL_ACTIVITY_STATUS) == DemographicProfile::SA)
		{
			if(DemographicProfile::get(bucket->getProfileID(), DemographicProfile::GENDER) == static_cast<BaseEnumCls::Enum>(_gender))
			{
				size += ((BucketSexualMixing *)(entityBuckets.at(currBucketIndex)))->sizeRisk(_risk);
			}
		}

		currBucketIndex++;
	}	//while(currBucketIndex < entityBuckets.size()) {

	return size;
}
//calculate the current number of sexually active persons within the specified age range
unsigned long EntityPool::sizeSexuallyActiveByAge(int minAgeMonths, int maxAgeMonths)
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
		if(DemographicProfile::get(bucket->getProfileID(), DemographicProfile::SEXUAL_ACTIVITY_STATUS) == DemographicProfile::SA)
		{
			size += ((BucketSexualMixing *)(entityBuckets.at(currBucketIndex)))->sizeByAge(minAgeMonths, maxAgeMonths);
		}

		currBucketIndex++;
	}	//while(currBucketIndex < entityBuckets.size()) {

	return size;
}

//calculate the current number of sexually active persons within the specified age range and gender
unsigned long EntityPool::sizeSexuallyActiveByAge(int minAgeMonths, int maxAgeMonths, DemographicProfile::Gender _gender)
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
		if(DemographicProfile::get(bucket->getProfileID(), DemographicProfile::SEXUAL_ACTIVITY_STATUS) == DemographicProfile::SA)
		{
			if(DemographicProfile::get(bucket->getProfileID(), DemographicProfile::GENDER) == static_cast<BaseEnumCls::Enum>(_gender))
			{
				size += ((BucketSexualMixing *)(entityBuckets.at(currBucketIndex)))->sizeByAge(minAgeMonths, maxAgeMonths);
			}
		}

		currBucketIndex++;
	}	//while(currBucketIndex < entityBuckets.size()) {

	return size;
}
//-------------------< Begin allMale and allFemale functions >----------------//
/*
 * adds Person to BucketDemographicProfile AND allMales or allFemales depending on gender
 */
bool EntityPool::addPersonToAll(Person *_p)
{
	if(_p->getDemographicProfileVal(DemographicProfile::GENDER) == DemographicProfile::MALE)
	{
		allMales.push_back(_p);
	}
	else
	{
		assert(_p->getDemographicProfileVal(DemographicProfile::GENDER) == DemographicProfile::FEMALE);
		allFemales.push_back(_p);
	}

	return addEntity(_p);
}

/*
 * removes Person from BucketDemographicProfile AND allMales or allFemales depending on gender
 * should be used only when *(_pIter) dies or when deleting this
 */
list<Person *>::iterator EntityPool::removePersonFromAll(list<Person *>::iterator _pIter)
{
	list<Person *>::iterator toReturn;
	removeEntity(*_pIter);

	if((*_pIter)->getDemographicProfileVal(DemographicProfile::GENDER) == DemographicProfile::MALE)
	{
		toReturn = allMales.erase(_pIter);
	}
	else if((*_pIter)->getDemographicProfileVal(DemographicProfile::GENDER) == DemographicProfile::FEMALE)
	{
		toReturn = allFemales.erase(_pIter);
	}
	else
	{
		(*_pIter)->print(cerr, "Not removing person of indiscriminate gender: ");
		toReturn = _pIter;
	}

	return toReturn;
}

/*
 * Returns allMales->begin()
 */
list<Person *>::iterator EntityPool::begin(DemographicProfile::Gender _gender)
{
	if(_gender == DemographicProfile::MALE)
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
list<Person *>::iterator EntityPool::end(DemographicProfile::Gender _gender)
{
	if(_gender == DemographicProfile::MALE)
	{
		return allMales.end();
	}
	else
	{
		return allFemales.end();
	}
}

//-------------------< End allMale and allFemale functions >----------------//
//---------------< Begin constructors and destructors >-------------------------//


//creates a New EntityPool
// @param _SAEntAgeMths age of sexual debut
EntityPool::EntityPool(int _SAEntAgeMths, unsigned int _popID, const std::array<double, (int)SexualPartnership::Type::ENDType> &_assort)
{
	//allocate space for Buckets and set to nullptr
	entityBuckets = std::vector<BucketDemographicProfile *>(DemographicProfile::TotalNumBuckets, nullptr);
	//this helps us select the buckets we want to use in the sim
	// initializing to END values will select all buckets
	DemographicProfile selector;
	//contains the BucketID's of the buckets we want to use in this simulation
	std::vector<DemographicProfile::ProfileID> validBucketIDs;
	//we only want 2 NA buckets (male, female)  b/c they aren't involved in sexual mixing
	//so instantiate 2 of the NA Buckets (NA, Hetero, nonCSW
	selector.set(DemographicProfile::SEXUAL_ACTIVITY_STATUS, DemographicProfile::NA);
	selector.set(DemographicProfile::SEXUAL_ORIENTATION, DemographicProfile::HETERO);
	selector.set(DemographicProfile::RELATIONSHIP_STATUS, DemographicProfile::SINGLE);
	selector.set(DemographicProfile::EMPLOYMENT, DemographicProfile::NON_CSW);
	selector.selectProfileIDs(validBucketIDs, nullptr);
	//check if we only have 2 buckets
	assert(validBucketIDs.size() == DemographicProfile::ENDGender);
	//We want to instantiate all heterosexual SA Buckets
	selector.set(DemographicProfile::END);
	selector.set(DemographicProfile::SEXUAL_ACTIVITY_STATUS, DemographicProfile::SA);
	selector.set(DemographicProfile::SEXUAL_ORIENTATION, DemographicProfile::HETERO);
	selector.selectProfileIDs(validBucketIDs, nullptr);

	//instantiate the spaces for all our buckets. The # of buckets depends on class BucketClassifiers
	//these people are stored in a more complicated BucketDemographicProfile b/c they are involved in sexual mixing
	for(unsigned int i = 0; i < validBucketIDs.size(); ++i)
	{
		DemographicProfile::ProfileID currBucketID = validBucketIDs.at(i);

		//at this point, DemographicProfile still matches the DemographicProfile::SA
		if(DemographicProfile::NA == DemographicProfile::get(validBucketIDs.at(i), DemographicProfile::SEXUAL_ACTIVITY_STATUS))
		{
			//make an NA bucket
			entityBuckets.at(currBucketID) = new BucketDemographicProfile(currBucketID, DemographicProfile::toString(currBucketID), true);
		}
		else
		{
			//CSW can't be in STEADY relationships
			bool invalidCombo = (DemographicProfile::CSW == DemographicProfile::get(validBucketIDs.at(i), DemographicProfile::EMPLOYMENT)) &&
			                    (DemographicProfile::NON_SINGLE == DemographicProfile::get(validBucketIDs.at(i), DemographicProfile::RELATIONSHIP_STATUS)) ;

			if(!invalidCombo)
			{
				entityBuckets.at(currBucketID) = new BucketSexualMixing(currBucketID, DemographicProfile::toString(currBucketID), _popID,
                    _SAEntAgeMths, 12 * Person::maxYrForDeathStats + 1, TimeGranularity::Month, _assort);
			}
		}
	}	//for(unsigned int i = 0; validBucketIDs.size(); ++i) {
}

EntityPool::~EntityPool(void)
{
	//Delete all people in allFemales and allMales in order to prevent memory leaks
	list<Person *>::iterator p_Iter;

	for(int gend = DemographicProfile::MALE; gend < DemographicProfile::ENDGender; gend++)
	{
		DemographicProfile::Gender gender = DemographicProfile::MALE;

		if(gend == DemographicProfile::FEMALE)
		{
			gender = DemographicProfile::FEMALE;
		}

		p_Iter = begin(gender);

		while(p_Iter != end(gender))
		{
			Person *p = (*p_Iter);
			//Advances p_Iter one in the list, so no increment is necessary
			p_Iter = removePersonFromAll(p_Iter);
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
			if(DemographicProfile::NA != DemographicProfile::get(entityBuckets.at(j)->getProfileID(), DemographicProfile::SEXUAL_ACTIVITY_STATUS))
			{
				delete(BucketSexualMixing *)entityBuckets.at(j);
			}
			else
			{
				delete entityBuckets.at(j);
			}
		}
	} //for(BaseEnumCls::Enum j = 0; j < entityBuckets.size(); ++j) {
}
