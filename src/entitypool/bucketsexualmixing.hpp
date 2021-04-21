#pragma once

//this creates a BucketDemographicProfile with an optimized data structure for sexual selection
//  people are put in different buckets based on age. Each bucket is sorted by sexualActivity coefficient
//  this is b/c SA folks are involved in fling and steadyCouple activity throughout the sim.
//	a more specialized and optimized data structure was necessary to make this program run at
//	a reasonable speed
// @param _id the ID number of the BucketDemographicProfile
// @param _minAge minimum age in years allowed
// @param _maxAge maximum age in years allowed
// @param _sexuallyActive true if people in this bucket are sexually active. Either all are sexually active or all are not



/***
This container puts people of different age in separate indexes
Each index contains people of same age by year. Right now the index is implemented by a set.

errhode 4/20/2009: The index has been replaced by BucketAge, which implements a FullVector.
***/

#include <list>
#include <boost/circular_buffer.hpp>

#include "entityindex.hpp"
#include "bucketdemographicprofile.hpp"
#include "bucketage.hpp"
#include "entities/entity.hpp"

namespace transm {

// class Entity;

/**
This class contains Person objects in different buckets based on age

The # of buckets depend on the timestep granularity used in the simulation
**/
class BucketSexualMixing : public BucketDemographicProfile
{
	//min and max age that this BucketDemographicProfile holds
	Age minAge;
	Age maxAge;

	//TODO:Put into BucketDemographicProfile??
	unsigned int popID;

public:
	//This is the main circular buffer containing the BucketAge structures
    using BucketAllAges = boost::circular_buffer_space_optimized<BucketAge *>;

    void SetAssortativeness(SexualPartnership::Type type, double assortativeness) { assort[type] = assortativeness; }

private :
	//this contains a circular buffer composed of BucketAges
	//each BucketAge contains the people are that are of the same ageMth
	//Each month persons age by shifting the index of their BucketAge
	//The BucketAge corresponding to maxAge is replaced by one corresponding to the new minAge
	BucketAllAges *personsByAge;

	//Assortativeness parameter for Mark Lipsitch's assortativeness algorithm
    std::map<SexualPartnership::Type, double> assort;

public :
	/**
	@param _id bucket ID associated with this BucketDemographicProfile
	@param _minAgeInYrs min age that of people found in this bucket (in years)
	@param _maxAgeInYrs age of people found in this bucket (in years)
	@param _timeGranularity people will be bucketed by either MONTH or YEAR of age. This determines performance of selection when the behavior is heterogeneous vs. homogeneous
	**/
	BucketSexualMixing(DemographicProfile::ProfileID _id, 
		const std::string *_bucketLabel, unsigned int _popID, Age minAge, 
		Age maxAge, const std::map<SexualPartnership::Type, double> &_assort);

	virtual ~BucketSexualMixing();

    /// <summary>
	/// clears all elements from this index without deleting members.
    /// </summary>
	void clear();

	Entity *drawMember(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType, bool _remove);

	/***
	 * Draws random person for the age range desired by person for partnership type
	@param _randomNums
	@param _remove - will remove this person from the BucketSexualMixing if this is true
	@return person that fits age range criteria for _chooser and _partnershipType.  If no such
	person exists, returns nullptr
	***/
	//TESTED
	Entity *drawMember(RandomNumberGenerator &_randomNums, Entity *_chooser, SexualPartnership::Type _partnershipType, bool _remove);

	//will remove this Person (if he or she exists) from the index
	//TESTED
	bool erase(Entity *_person);

	//tells whether _person exists in the index
	//TESTED
	bool exists(Entity *_person);

	//counts number of infected people this EntityPool
	//TESTED*
	unsigned long getNumInfected();
	//UNTESTED
	unsigned long getNumInfected(int generation);

	/*
	*returns the number of unique infected people by risk group
	*/
	unsigned long getNumInfected(RiskLevel _risk);

	//will index a new Person
	//TESTED
	bool insert(Entity *_person);

	//TESTED
    void print(ostream &_outStream, const std::string &_prefix);

	//returns the # of entities in this index
	//TESTED
	unsigned long size();

    void forEach(std::function<void(Entity *)> callback);

	BucketAllAges::iterator begin();

	//TESTED
	BucketAllAges::iterator end();

	/*
	 * @params: minMonthAge, maxMonthAge
	 * @returns: total number of persons in this with age between minMonthAge and maxMonthAge
	 */
	unsigned long sizeByAge(Age minAge, Age maxAge);

	/*
	 * @params: minMonthAge, maxMonthAge
	 * @returns: total number of infected persons in this with age between minMonthAge and maxMonthAge
	 */
	unsigned long sizeInfectedByAge(Age minAge, Age maxAge);

	/*
	 * @returns: total number of marbles in all FVs associated with _risk
	 * across all BucketAges in this; If _risk = (std::size_t)RiskLevel::Last,
	 * returns the number of persons in the random risk bucket
	 */
	//TESTED
	unsigned long sizeRisk(RiskLevel _risk);

	/*
	 * @returns: total number of unique persons in this bucket with given risk level that is CSW
	 * across all BucketAges in this; If _risk = (std::size_t)RiskLevel::Last,
	 * returns the number of persons in the random risk bucket
	 */
	unsigned long sizeRiskCSW(RiskLevel _risk);

	/*
	 * @returns: total number of unique persons in this bucket with given risk level and hiv status
	 * across all BucketAges in this;
	 */
	unsigned long sizeRiskHIVStatus(RiskLevel _risk, HIVStatus _hivStatus);

	/*
	 * @returns: total number of marbles in all Random Risk FVs across all
	 * BucketAges in this
	 */
	//TESTED
	unsigned long sizeRandom();

	/*
	 * @function: increaseInfected();
	 * @effects: if _person.isInfected, increases number of infected persons for
	 * the BucketAge which corresponds to _person
	 * @return: returns true if _person.isInfected and number was increased and
	 * false otherwise
	 */
	//TESTED
	bool increaseInfected(Entity *_person);

	/* @function: changeHIVstatus
	 * @effects: if person is in this Bucket and thier hiv status changes decrement the old status and increment new status
	 */
	void changeHIVStatus(Entity *_p, HIVStatus _orig, HIVStatus _new);

	/*
	 * @effects: Sets all persons in oldest BucketAge to die and processes their
	 * deaths, pushes new BucketAge to front (for incoming births), sets all other persons
	 * to age++ (to account for new index of BucketAge
	 * @returns: List of persons set to die (of old age)
	 */
	//TESTED
	std::list<Entity *> ageOneTimeStep();

private :

	/*
	gets the offset of the BucketAge in personsByAge which would contain person _p.
	if _p does not belong in this BucketDemographicProfile, then will return size of personsByAge
	*/
	//TESTED
	unsigned int getCorrectBufferIndex(Entity *_p);

	//gets a random person with age in [_ageLowerBound,_ageUpperBound]
	//TESTED
	Entity *getRandomPerson(RandomNumberGenerator &_randomNums, Age _ageLowerBound, Age _ageUpperBound,
	                        RiskLevel _risk, SexualPartnership::Type _partnershipType, bool _remove);

	//Returns AgeBucket of oldest persons
	//TESTED
	BucketAge *getOldest();

	//Returns AgeBucket of youngest persons
	//TESTED
	BucketAge *getYoungest();
};

} // namespace transm
