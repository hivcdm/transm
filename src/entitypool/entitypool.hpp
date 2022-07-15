#ifndef ENTITYPOOL_HPP
#define ENTITYPOOL_HPP

#include <cstdlib>
#include <list>
#include <memory>
#include <vector>

#include "bucketsexualmixing.hpp"
#include "entities/entitytypes.hpp"
#include "entities/demographicprofile.hpp"
#include "entities/sexualpartnership.hpp"
#include "parameters/populationparameters.hpp"

namespace transm {

 /**
  * This is a container for Person objects which are separated into different BucketDemographicProfile depending on
  * their DemographicProfile.
  *
  * This provides a limited interface to the simulation logic contained in class Population so that we
  * can change the underlying data structures without changing the simulation code too much */
class EntityPool
{
public:

    /** Creates a new EntityPool */
	EntityPool(const PopulationParameters &parameters, unsigned int _popID, const std::map<SexualPartnership::Type, double> &_assort);

	~EntityPool();

    void forEach(std::function<void(Entity *)> callback);

    /** adds an person to the correct bucket in the pool based on their current DemographicProfile */
	bool addEntity(Entity *_person);

    /** Return the profileIDs for all buckets */
	std::vector<DemographicProfile::ProfileID> getProfileIDs();

    /** Return the profileIDs for initiator buckets	*/
	std::vector<DemographicProfile::ProfileID> getInitiatorProfileIDs();

    /** Return the profileIDs for partner buckets */
	std::vector<DemographicProfile::ProfileID> getPartnerProfileIDs();

    /** Return the bucket that matches _profileID */
	BucketDemographicProfile *getBucket(DemographicProfile::ProfileID _profileID);

    /**
     * prints everyone inside the Entitypool. Use sparingly...
     * lists out all Buckets and members members of each */
	void print(std::ostream &_outStream);

    /**
     * print out all the labels of all the Buckets in the EntityPool. separate each by TAB
     * if _printPropInfected == true, then include a column for #infected for each BucketDemographicProfile	*/
	void printBucketLabels(std::ostream &_outStream, bool _printPropInfected);

    /**
     * list out all buckets and their size
     *
	 * @param _printPropInfected if == true, then print the fraction of people who are infected
	 * @param _includeLabels if == true, then print out the BucketDemographicProfile label w/ each BucketDemographicProfile size
	 * @param _totalInfected this will be set to total # infecteds currently in the EntityPool
	 * @param _totalSize this will be set to total # of people in the population
	 * @param _includeLabls if == true, then will additionally print BucketDemographicProfile labels on the same line as the size */
    void printBucketSizes(std::ostream &_outStream, const std::string &_prefix, bool _printPropInfected, unsigned long &_totalInfected,
	                      unsigned long &_totalSize, unsigned long &_totalSexuallyActive, unsigned long &_totalInSteady,
	                      unsigned long &_totalInRegular, bool _includeLabels);

    /**
	 * remove _person if exists in pool. returns false if _person is not in pool
	 * We look in the DemographicProfile bucket that the person believes that they are in
     * (_person->getCurrBucketProfileID()) as opposed to their current DemographicProfile */
	bool removeEntity(Entity *_person);

    /**
	 * if someone is a member of the wrong Bucket (based on their DemographicProfile), will remove and place them in the correct one
	 * @param _person person that we have to move
	 * @param _p_Iter if this is not nullptr, then use this _iter to remove the person. It will be a faster operation
     * than finding them again within the map */
	bool refreshBucketDemographicProfile(Entity *_person, std::list<Entity *>::iterator *_p_Iter, bool forceRefresh = false);

    /** calculates the current size of the EntityPool */
	unsigned long size();

    /** calculate the current number of persons with a given DemographicProfile ID	*/
	unsigned long size(DemographicProfile::ProfileID _profileID);

    /** calculate the current number of persons that are not sexually active in the entity pool	*/
	unsigned long sizeNotSexuallyActive();

    /** calculate the current number of sexually active persons by risk and gender	*/
	std::size_t sizeSexuallyActive(const std::string &entity_type, RiskLevel risk);

    /** calculate the current number of persons that are not sexually active in the entity pool with a given gender	*/
	std::size_t sizeNotSexuallyActive(const std::string &entity_type);

    /** calculate the current number of sexually active persons within the specified age range */
	unsigned long sizeSexuallyActiveByAge(Age minAge, Age maxAge);

    /** calculate the current number of sexually active persons within the specified age range and gender */
	std::size_t sizeSexuallyActiveByAge(Age minAge, Age maxAge, const std::string &entity_type);

    /** calculate the current number of people within the specified age range */
	unsigned long sizeByAge(Age minAgeMonths, Age maxAgeMonths);

    /** calculate the current number of males within the specified age range */
	unsigned long sizeByAgeMales(Age minAgeMonths, Age maxAgeMonths);

    /** calculate the current number of females within the specified age range */
	unsigned long sizeByAgeFemales(Age minAgeMonths, Age maxAgeMonths);

    /** adds Person to BucketDemographicProfile AND allMales or allFemales depending on gender */
	bool addEntityToAll(Entity *_p);

    /**
	 * removes Person from BucketDemographicProfile AND allMales or allFemales depending on gender
	 * should be used only when *(_pIter) dies */
	std::list<Entity *>::iterator removeEntityFromAll(list<Entity *>::iterator _pIter);

    /** Returns allMales->begin() */
	std::list<Entity *>::iterator begin(DemographicProfile::Gender _gender);

    /** Returns allMales->end()	*/
	std::list<Entity *>::iterator end(DemographicProfile::Gender _gender);

    /** Updates the tally of males and females per year of age */
	void countEntitiesPerAge();

private:
	/**
	 * This is a container that holds Person Buckets. This is the authoritative container for the pool.
	 * All the buckets in this std::vector contain all Persons in the sim
	 * Each index of the vector corresponds to a DemographicProfile::ProfileID. This corresponds to
	 * a unique combucketation of DemographicProfile enum values */
	std::vector<BucketDemographicProfile *> entityBuckets;

    /** List of profile ids for all the buckets in the pool	*/
	std::vector<DemographicProfile::ProfileID> validProfileIDs;

    /**
	 * List of profile ids for initiators in the buckets in the pool
     * Used to enumerate initiator in initiator by partner output counts */
	std::vector<DemographicProfile::ProfileID> validInitiatorProfileIDs;

    /**
	 * List of profile ids for initiators in the buckets in the pool
     * Used to enumerate partners in initiator by partner output counts	*/
	std::vector<DemographicProfile::ProfileID> validPartnerProfileIDs;

    /** Master list of males for iterating */
	std::list<Entity *> allMales;

    /** Master list of females for iterating */
	std::list<Entity *> allFemales;

    /** Quick way to keep track of people's age	*/
	std::array<unsigned long, Entity::maxYrForDeathStats * 12 + 1> malesPerAge;
	std::array<unsigned long, Entity::maxYrForDeathStats * 12 + 1> femalesPerAge;

	void resetPeoplePerAge();

};

} // namespace transm


#endif /* ENTITYPOOL_HPP */