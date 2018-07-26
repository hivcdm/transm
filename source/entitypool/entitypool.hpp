#pragma once

#include <cstdlib>
#include <list>
#include <memory>
#include <vector>

#include "bucketsexualmixing.hpp"
#include "entities/demographicprofile.hpp"
#include "entities/sexualpartnership.hpp"

namespace transm {

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
	EntityPool(Age ageOfMajority, unsigned int _popID, const std::map<SexualPartnership::Type, double> &_assort);

	~EntityPool();

    void forEach(std::function<void(Entity *)> callback);

	/// <summary>
	/// adds an person to the correct bucket in the pool based on their current DemographicProfile
	/// </summary>
	bool addEntity(Entity *_person);

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
	bool removeEntity(Entity *_person);

	/// <summary>
	/// if someone is a member of the wrong Bucket (based on their DemographicProfile), will remove and place them in the correct one
	/// @param _person person that we have to move
	/// </summary>
	bool refreshBucketDemographicProfile(Entity *_person, bool forceRefresh = false);

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
	std::size_t sizeSexuallyActive(const std::string &entity_type, Entity::RiskLevel risk);

	/// <summary>
	/// calculate the current number of persons that are not sexually active in the entity pool with a given gender
	/// </summary>
	std::size_t sizeNotSexuallyActive(const std::string &entity_type);

	/// <summary>
	/// calculate the current number of sexually active persons within the specified age range
	/// </summary>
	unsigned long sizeSexuallyActiveByAge(Age minAge, Age maxAge);

	/// <summary>
	/// calculate the current number of sexually active persons within the specified age range and gender
	/// </summary>
	std::size_t sizeSexuallyActiveByAge(Age minAge, Age maxAge, const std::string &entity_type);

	/// <summary>
	/// calculate the current number of people within the specified age range
	/// </summary>
	unsigned long sizeByAge(Age minAgeMonths, Age maxAgeMonths);

	/// <summary>
	/// calculate the current number of males within the specified age range
	/// </summary>
	unsigned long sizeByAgeMales(Age minAgeMonths, Age maxAgeMonths);

	/// <summary>
	/// calculate the current number of females within the specified age range
	/// </summary>
	unsigned long sizeByAgeFemales(Age minAgeMonths, Age maxAgeMonths);

	/// <summary>
	/// adds Person to BucketDemographicProfile AND allMales or allFemales depending on gender
	/// </summary>
	bool addEntityToAll(Entity *_p);

	/// <summary>
	/// removes Person from BucketDemographicProfile AND allMales or allFemales depending on gender
	/// should be used only when *(_pIter) dies
	/// </summary>
	std::list<Entity *>::iterator removeEntityFromAll(list<Entity *>::iterator _pIter);

	/// <summary>
	/// Returns allMales->begin()
	/// </summary>
	std::list<Entity *>::iterator begin(DemographicProfile::Gender _gender);

	/// <summary>
	/// Returns allMales->end()
	/// </summary>
	std::list<Entity *>::iterator end(DemographicProfile::Gender _gender);

	/// <summary>
	/// Updates the tally of males and females per year of age
	/// </summary>
	void countEntitiesPerAge();

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
	std::list<Entity *> allMales;

	/// <summary>
	/// Master list of females for iterating
	/// </summary>
	std::list<Entity *> allFemales;

	/// <summary>
	/// Quick way to keep track of people's age
	/// </summary>
	std::array<unsigned long, Entity::maxYrForDeathStats * 12 + 1> malesPerAge;
	std::array<unsigned long, Entity::maxYrForDeathStats * 12 + 1> femalesPerAge;

	void resetPeoplePerAge();
};

} // namespace transm
