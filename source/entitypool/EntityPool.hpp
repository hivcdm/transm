#pragma once

#include <cstdlib>
#include <list>
#include <memory>
#include <vector>

#include "BucketSexualMixing.hpp"
#include "entities/DemographicProfile.hpp"
#include "entities/SexualPartnership.hpp"

/// <summary>
/// This is a container for Person objects which are separated into different BucketDemographicProfile depending on
/// their DemographicProfile.
/// </summary>
/// <remarks>
/// This provides a limited interface to the simulation logic contained in class Population so that we
/// can change the underlying data structures without changing the simulation code too much
/// </remarks>
class EntityPool
{
public:
	/// <summary>
	/// Creates a new EntityPool
	/// </summary>
	EntityPool(int ageOfMajority, unsigned int _popID, const std::map<SexualPartnership::Type, double> &_assort);

	~EntityPool();

    void forEach(std::function<void(Person *)> callback);

	/// <summary>
	/// adds an person to the correct bucket in the pool based on their current DemographicProfile
	/// </summary>
	bool addEntity(Person *_person);

	/// <summary>
	/// Return the bucket that matches _profileID
	/// </summary>
	BucketDemographicProfile *getBucket(DemographicProfile::ProfileID _profileID);

	/// <summary>
	/// prints everyone inside the Entitypool. Use sparingly...
	/// lists out all Buckets and members members of each
	/// </summary>
	void print(std::ostream &_outStream);

	/// <summary>
	/// print out all the labels of all the Buckets in the EntityPool. separate each by TAB
	/// if _printPropInfected == true, then include a column for #infected for each BucketDemographicProfile
	/// </summary>
	void printBucketLabels(std::ostream &_outStream, bool _printPropInfected);

	/// <summary>
	/// list out all buckets and their size
	/// </summary>
	/// <remarks>
	/// @param _printPropInfected if == true, then print the fraction of people who are infected
	/// @param _includeLabels if == true, then print out the BucketDemographicProfile label w/ each BucketDemographicProfile size
	/// @param _totalInfected this will be set to total # infecteds currently in the EntityPool
	/// @param _totalSize this will be set to total # of people in the population
	/// @param _includeLabls if == true, then will additionally print BucketDemographicProfile labels on the same line as the size
	/// </remarks>
    void printBucketSizes(std::ostream &_outStream, const std::string &_prefix, bool _printPropInfected, unsigned long &_totalInfected,
	                      unsigned long &_totalSize, unsigned long &_totalSexuallyActive, unsigned long &_totalInSteady,
	                      unsigned long &_totalInRegular, bool _includeLabels);

	/// <summary>
	/// remove _person if exists in pool. returns false if _person is not in pool
	/// We look in the DemographicProfile bucket that the person believes that they are in (_person->getCurrBucketProfileID()) as opposed to their current DemographicProfile
	/// </summary>
	bool removeEntity(Person *_person);

	/// <summary>
	/// if someone is a member of the wrong Bucket (based on their DemographicProfile), will remove and place them in the correct one
	/// @param _person person that we have to move
	/// @param _p_Iter if this is not nullptr, then use this _iter to remove the person. It will be a faster operation than finding them again within the map
	/// </summary>
	bool refreshBucketDemographicProfile(Person *_person, std::list<Person *>::iterator *_p_Iter, bool forceRefresh = false);

	/// <summary>
	/// calculates the current size of the EntityPool
	/// </summary>
	unsigned long size();

	/// <summary>
	/// calculate the current number of persons with a given DemographicProfile ID
	/// </summary>
	unsigned long size(DemographicProfile::ProfileID _profileID);

	/// <summary>
	/// calculate the current number of persons that are not sexually active in the entity pool
	/// </summary>
	unsigned long sizeNotSexuallyActive();

	/// <summary>
	/// calculate the current number of sexually active persons by risk and gender
	/// </summary>
	unsigned long sizeSexuallyActive(DemographicProfile::Gender _gender, Person::RiskLevel risk);

	/// <summary>
	/// calculate the current number of persons that are not sexually active in the entity pool with a given gender
	/// </summary>
	unsigned long sizeNotSexuallyActive(DemographicProfile::Gender _gender);

	/// <summary>
	/// calculate the current number of sexually active persons within the specified age range
	/// </summary>
	unsigned long sizeSexuallyActiveByAge(int minAgeMonths, int maxAgeMonths);

	/// <summary>
	/// calculate the current number of sexually active persons within the specified age range and gender
	/// </summary>
	unsigned long sizeSexuallyActiveByAge(int minAgeMonths, int maxAgeMonths, DemographicProfile::Gender _gender);

	/// <summary>
	/// adds Person to BucketDemographicProfile AND allMales or allFemales depending on gender
	/// </summary>
	bool addPersonToAll(Person *_p);

	/// <summary>
	/// removes Person from BucketDemographicProfile AND allMales or allFemales depending on gender
	/// should be used only when *(_pIter) dies
	/// </summary>
	std::list<Person *>::iterator removePersonFromAll(list<Person *>::iterator _pIter);

	/// <summary>
	/// Returns allMales->begin()
	/// </summary>
	std::list<Person *>::iterator begin(DemographicProfile::Gender _gender);

	/// <summary>
	/// Returns allMales->end()
	/// </summary>
	std::list<Person *>::iterator end(DemographicProfile::Gender _gender);

private:
	/// <summary>
	/// This is a container that holds Person Buckets. This is the authoritative container for the pool.
	/// All the buckets in this std::vector contain all Persons in the sim
	/// Each index of the vector corresponds to a DemographicProfile::ProfileID. This corresponds to
	/// a unique combucketation of DemographicProfile enum values
	/// </summary>
	std::vector<BucketDemographicProfile *> entityBuckets;

	/// <summary>
	/// Master list of males for iterating
	/// </summary>
	std::list<Person *> allMales;

	/// <summary>
	/// Master list of females for iterating
	/// </summary>
	std::list<Person *> allFemales;
};
