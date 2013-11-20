#pragma once

#include <cstdlib>
#include <memory>
#include <vector>
#include <list>

using namespace std;

#include "./../classifiers/DmgProfile.h"
#include "./../classifiers/SexualPartnership.h"
#include "./bucket/BucketSexualMixing.h"

/**
	This is a container for Person objects which are separated into different DmgProfileBucket depending on
	their DmgProfile.

	This provides a limited interface to the simulation logic contained in class Population so that we
	can change the underlying data structures without changing the simulation code too much
**/
class EntityPool {

private:
	//This is a container that holds Person Buckets. This is the authoritative container for the pool.
	//			All the buckets in this std::vector contain all Persons in the sim
	//		Each index of the vector corresponds to a DmgProfile::ProfileID. This corresponds to
	//			a unique combucketation of DmgProfile enum values
	std::vector<DmgProfileBucket*> *entityBuckets;

	//Master lists of males and females for iterating
	list<Person*> allMales;
	list<Person*> allFemales;

public:

	/**
	adds an person to the correct bucket in the pool based on their current DmgProfile
	**/
	bool addEntity(Person *_person);

	/**
	Return the bucket that matches _profileID
	**/
	DmgProfileBucket* getBucket(DmgProfile::ProfileID _profileID);

	/**
	prints everyone inside the Entitypool. Use sparingly...
	lists out all Buckets and members members of each
	**/
	void print(ostream& _outStream);

	//print out all the labels of all the Buckets in the EntityPool. separate each by TAB
	//if _printPropInfected == true, then include a column for #infected for each DmgProfileBucket
	void printBucketLabels(ostream& _outStream, bool _printPropInfected);

	/*list out all buckets and their size
	 @param _printPropInfected if == true, then print the fraction of people who are infected
	 @param _includeLabels if == true, then print out the DmgProfileBucket label w/ each DmgProfileBucket size
	 @param _totalInfected this will be set to total # infecteds currently in the EntityPool
	 @param _totalSize this will be set to total # of people in the population
	 @param _includeLabls if == true, then will additionally print DmgProfileBucket labels on the same line as the size
	*/
	void printBucketSizes(ostream& _outStream, string _prefix, bool _printPropInfected, unsigned long &_totalInfected, unsigned long &_totalSize,unsigned long &_totalSexuallyActive,unsigned long &_totalInSteady, unsigned long &_totalInRegular,bool _includeLabels);

	/**
	remove _person if exists in pool. returns false if _person is not in pool
	We look in the DmgProfile bucket that the person believes that they are in (_person->getCurrBucketProfileID()) as opposed to their current DmgProfile
	**/
	bool removeEntity(Person *_person);

	/**
	if someone is a member of the wrong Bucket (based on their DmgProfile), will remove and place them in the correct one
	@param _person person that we have to move
	@param _p_Iter if this is not NULL, then use this _iter to remove the person. It will be a faster operation than finding them again within the map
	**/
	bool refreshDmgProfileBucket(Person *_person, list<Person*>::iterator *_p_Iter, bool forceRefresh = false);

	//calculates the current size of the EntityPool
	unsigned long size();

	//calculate the current number of persons with a given DmgProfile ID
	unsigned long size(DmgProfile::ProfileID _profileID);

	//calculate the current number of persons that are not sexually active in the entity pool
	unsigned long sizeNotSexuallyActive();

	//calculate the current number of sexually active persons by risk and gender
	unsigned long sizeSexuallyActive(DmgProfile::Gender _gender, Person::RiskLevel risk);

	//calculate the current number of persons that are not sexually active in the entity pool with a given gender
	unsigned long sizeNotSexuallyActive(DmgProfile::Gender _gender);

	//calculate the current number of sexually active persons within the specified age range
	unsigned long sizeSexuallyActiveByAge(int minAgeMonths, int maxAgeMonths);
	
	//calculate the current number of sexually active persons within the specified age range and gender
	unsigned long sizeSexuallyActiveByAge(int minAgeMonths, int maxAgeMonths, DmgProfile::Gender _gender);


	//-------------------< Begin allMale and allFemale functions >----------------//
	/*
	 * adds Person to DmgProfileBucket AND allMales or allFemales depending on gender
	 */
	bool addPersonToAll(Person *_p);

	/*
	 * removes Person from DmgProfileBucket AND allMales or allFemales depending on gender
	 * should be used only when *(_pIter) dies
	 */
	list<Person*>::iterator removePersonFromAll(list<Person*>::iterator _pIter);

	/*
	 * Returns allMales->begin()
	 */
	list<Person*>::iterator begin(DmgProfile::Gender _gender);


	/*
	 * Returns allMales->end()
	 */
	list<Person*>::iterator end(DmgProfile::Gender _gender);


	//-------------------< Begin allMale and allFemale functions >----------------//

public:

	//creates a 'new EntityPool
	EntityPool(int _SAEntAgeMths, unsigned int _popID, const double _assort[]);

	~EntityPool(void);
};

