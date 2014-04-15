#pragma once

//this creates a DmgProfileBucket with an optimized data structure for sexual selection
//  people are put in different buckets based on age. Each bucket is sorted by sexualActivity coefficient
//  this is b/c SA folks are involved in fling and steadyCouple activity throughout the sim.
//	a more specialized and optimized data structure was necessary to make this program run at
//	a reasonable speed
// @param _id the ID number of the DmgProfileBucket
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

#include "EntityIndex.h"
#include "DmgProfileBucket.h"
#include "BucketAge.h"

class Person;

/**
This class contains Person objects in different buckets based on age

The # of buckets depend on the timestep granularity used in the simulation
**/
class BucketSexualMixing : public DmgProfileBucket
{


	//friend class DmgProfileBucket::JavaStyleIterator;

	//min and max age that this DmgProfileBucket holds
	unsigned int minAge;
	unsigned int maxAge;
	TimeGranularity timeGranularity;

	//TODO:Put into DmgProfileBucket??
	unsigned int popID;

public:
	//This is the main circular buffer containing the BucketAge structures
	typedef boost::circular_buffer_space_optimized<BucketAge *> BucketAllAges;

private :
	//this contains a circular buffer composed of BucketAges
	//each BucketAge contains the people are that are of the same ageMth
	//Each month persons age by shifting the index of their BucketAge
	//The BucketAge corresponding to maxAge is replaced by one corresponding to the new minAge
	BucketAllAges *personsByAge;

	//Assortativeness parameter for Mark Lipsitch's assortativeness algorithm
	std::array<double, (int)SexualPartnership::Type::ENDType> assort;

public :

	/**
	@param _id bucket ID associated with this DmgProfileBucket
	@param _minAgeInYrs min age that of people found in this bucket (in years)
	@param _maxAgeInYrs age of people found in this bucket (in years)
	@param _timeGranularity people will be bucketed by either MONTH or YEAR of age. This determines performance of selection when the behavior is heterogeneous vs. homogeneous
	**/
	BucketSexualMixing(DmgProfile::ProfileID _id, const string *_bucketLabel, unsigned int _popID, int _minAge, int _maxAge,
		TimeGranularity _timeGranularity, const std::array<double, (int)SexualPartnership::Type::ENDType> &_assort);
	virtual ~BucketSexualMixing();

	//-------------< Begin inherited from class DmgProfileBucket >---------------------//
	//clears all elements from this index without deleting members
	//TESTED
	void clear();

	void Apply(const PopulationTarget &target, std::function<void(Person*)> modifier);

	//TESTED
	Person *drawMember(RandomNums &_randomNums, SexualPartnership::Type _partnershipType, bool _remove);

	/***
	 * Draws random person for the age range desired by person for partnership type
	@param _randomNums
	@param _remove - will remove this person from the BucketSexualMixing if this is true
	@return person that fits age range criteria for _chooser and _partnershipType.  If no such
	person exists, returns nullptr
	***/
	//TESTED
	Person *drawMember(RandomNums &_randomNums, Person *_chooser, SexualPartnership::Type _partnershipType, bool _remove);

	//will remove this Person (if he or she exists) from the index
	//TESTED
	bool erase(Person *_person);

	//tells whether _person exists in the index
	//TESTED
	bool exists(Person *_person);

	//counts number of infected people this EntityPool
	//TESTED*
	unsigned long getNumInfected();
	//UNTESTED
	unsigned long getNumInfected(int generation);

	/*
	*returns the number of unique infected people by risk group
	*/
	unsigned long getNumInfected(Person::RiskLevel _risk);

	//will index a new Person
	//TESTED
	bool insert(Person *_person);

	//TESTED
	void print(ostream &_outStream, std::string _prefix);

	//returns the # of entities in this index
	//TESTED
	unsigned long size();

	//-------------< End inherited from class DmgProfileBucket >---------------------//
	//-------------< Begin iterator methods >------------------//
	//TESTED
	BucketAllAges::iterator begin();

	//TESTED
	BucketAllAges::iterator end();

	//--------------< End iterator methods >-------------------//
	//-------< Begin additional methods based on this structure >-------//
	/*
	 * @params: minMonthAge, maxMonthAge
	 * @returns: total number of persons in this with age between minMonthAge and maxMonthAge
	 */
	unsigned long sizeByAge(int minMonthAge, int maxMonthAge);

	/*
	 * @params: minMonthAge, maxMonthAge
	 * @returns: total number of inftected persons in this with age between minMonthAge and maxMonthAge
	 */
	unsigned long sizeInfectedByAge(int minMonthAge, int maxMonthAge);

	/*
	 * @returns: total number of marbles in all FVs associated with _risk
	 * across all BucketAges in this; If _risk = Person::ENDRiskLevel,
	 * returns the number of persons in the random risk bucket
	 */
	//TESTED
	unsigned long sizeRisk(Person::RiskLevel _risk);

	/*
	 * @returns: total number of unique persons in this bucket with given risk level that is CSW
	 * across all BucketAges in this; If _risk = Person::ENDRiskLevel,
	 * returns the number of persons in the random risk bucket
	 */
	unsigned long sizeRiskCSW(Person::RiskLevel _risk);

	/*
	 * @returns: total number of unique persons in this bucket with given risk level and hiv status
	 * across all BucketAges in this;
	 */
	unsigned long sizeRiskHIVStatus(Person::RiskLevel _risk, Person::HIVStatus _hivStatus);

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
	bool increaseInfected(Person *_person);

	/* @function: changeHIVstatus
	 * @effects: if person is in this Bucket and thier hiv status changes decrement the old status and increment new status
	 */
	void changeHIVStatus(Person *_p, Person::HIVStatus _orig, Person::HIVStatus _new);

	/*
	 * @effects: Sets all persons in oldest BucketAge to die and processes their
	 * deaths, pushes new BucketAge to front (for incoming births), sets all other persons
	 * to age++ (to account for new index of BucketAge
	 * @returns: List of persons set to die (of old age)
	 */
	//TESTED
	list<Person *> ageOneTimeStep();

	//--------< End additional methods based on this structure >--------//
private :

	/*
	gets the offset of the BucketAge in personsByAge which would contain person _p.
	if _p does not belong in this DmgProfileBucket, then will return size of personsByAge
	*/
	//TESTED
	unsigned int getCorrectBufferIndex(Person *_p);

	//gets a random person with age in [_ageLowerBound,_ageUpperBound]
	//TESTED
	Person *getRandomPerson(RandomNums &_randomNums, unsigned int _ageLowerBound, unsigned int _ageUpperBound,
	                        Person::RiskLevel _risk, SexualPartnership::Type _partnershipType, bool _remove);

	//Returns AgeBucket of oldest persons
	//TESTED
	BucketAge *getOldest();

	//Returns AgeBucket of youngest persons
	//TESTED
	BucketAge *getYoungest();

	/**

	The interface matches that of the Java 1.5.0 Iterator interface, with the addition of a reset() method
		public:
		JIterator(AgeIndex)
		~JIterator()
		T next();
		bool hasNext();
		void reset();
		void remove();
	***/

	/*protected:
		class JavaStyleIterator : DmgProfileBucket::JavaStyleIterator{

		public:

			EntityAgeBuffer *entityCircularBuff;
			unsigned int currBuffIndex;
			SexualActivityIndex::JIterator currNumIndexJIterator;

			JavaStyleIterator();
			JavaStyleIterator(BucketSexualMixing *_bucket);

			//returns true if the element that was last returned by next() has been removed using remove()
			bool alreadyRemoved();

			//returns the spot right after last member of this pool
			bool hasNext();

			//this will be used to get the next in line
			Person* next();

			//removes from the collection the last element returned by the iterator
			bool remove();

			//lets us reuse an iterator, resets to beginning of current collection
			void reset();

			~JavaStyleIterator();
		};

		JIterator iterator();
	*/
};

