#pragma once
#include "EntityIndex.h"
#include "../../classifiers/DmgProfile.h"

class Person;
class RandomNums;
using namespace std;

/***
    This class is a simple container for Entitys and allows us to add, count, get, and remove them

	This class is related to class DmgProfile in that for each unique DmgProfile, there is one and only one DmgProfileBucket

	The internal representation of the entities is a set. People are unsorted.
***/

class DmgProfileBucket {

	//this is an index based on person's ID
	typedef EntityIndex<Person::ID,unsigned long> PersonSet;

	friend class JavaStyleIterator;

	//holds all the entities in this index
	PersonSet *simpleEntityIndex;

	DmgProfile::ProfileID dmgProfileID;		//ID of DmgProfileBucket. id's go from 0 -> total number of buckets in EntityPool
	const string* bucketLabel;

public:

	//------------< Begin Implemented Methods >----------------//

	//returns the DmgProfileBucket's ID number
	DmgProfile::ProfileID getProfileID();
	
	PersonSet* getEntityIndex();
	/**
	This method will return a label for this DmgProfileBucket
	**/
	const string* getLabel();

	//------------< End Implemented Methods >----------------//


	//------------------< Begin DmgProfileBucket Virtual methods >------------------//

	//empties this DmgProfileBucket
	virtual void clear();

	//choose random person from the DmgProfileBucket
	//  _remove - if true, then will remove the chosen person from the DmgProfileBucket
	virtual Person* drawMember(RandomNums &_randomNums, SexualPartnership::Type _partnershipType, bool _remove);

	/***
	Draws a partner from this Bucket on behalf of _chooser. This will take into account the
	chooser's partner selection requirements for the given partnership type
	@param _randomNums random number generator
	@param _chooser the person who is choosing a partner
	@param _partnershipType the type of partner this person is looking for
	@param _remove - will remove this person from the bucket
	***/
	virtual Person* drawMember(RandomNums& _randomNums, Person *_chooser,SexualPartnership::Type _partnershipType, bool _remove);

	virtual bool exists(Person *_person);

	/** Methods to get information about pool **/
	//counts number of infected people this EntityPool
	virtual unsigned long getNumInfected();
	virtual unsigned long getNumInfected(int generation);

	//this adds member into the pool
	//  _toInsert - the Person being added to the pool.
	virtual bool insert(Person *_toInsert);

	//if _p exists in the bucket, will remove. remove true if existed
	virtual bool erase(Person *_p);

	//returns the size of this DmgProfileBucket
	virtual unsigned long size();

	/** Methods to help with debugging **/

	//lists all members of a specified entitypool on a different line
	//  _prefix - will append this string to the front of each member and then print
	virtual void print(ostream& _outStream, string _prefix);

	/*
	 * @effects: Ages everyone in the bucket one timestep
	 * @returns: List of persons too old for timestep (should be placed into other bucket)
	 */
	virtual list<Person*> ageOneTimeStep();


	//------------------< End DmgProfileBucket Virtual methods >------------------//

public:

	//-----------------< Begin Constructors and Destructors >----------------//
	//this function should not be used in this sim, it's just here for a default constructor
	DmgProfileBucket();

	//this creates a DmgProfileBucket object
	//@param _id sets this as this bucket's ID
	//@param _simpleIndex - if this is true, then this DmgProfileBucket uses an EntityIndex
	DmgProfileBucket(int _id, const string *_bucketLabel, bool _simpleIndex);

	//deletes all entities inside this DmgProfileBucket
	~DmgProfileBucket(void);

	//-----------------< End Constructors and Destructors >----------------//

	/**

	The interface matches that of the Java 1.5.0 Iterator interface, with the addition of a reset() method
		public:
		JIterator(EntityIndex)
		~JIterator()
		T next();
		bool hasNext();
		void reset();
		void remove();

	This was created to:
		1) make the code easier to read (a matter of personal preference)
		2) hide the internal data structures from the outside
	***/
/*
protected:
	class JavaStyleIterator {

		PersonSet::JIterator pIter;

		public:

			JavaStyleIterator();

			JavaStyleIterator(DmgProfileBucket *_bucket);

			//returns true if the element that was last returned by next() has been removed using remove()
			virtual bool alreadyRemoved();

			//returns the spot right after last member of this pool
			virtual bool hasNext();

			//this will be used to get the next in line
			virtual Person * next();

			//removes from the collection the last element returned by the iterator
			virtual bool remove();

			//lets us reuse an iterator, resets to beginning of current collection
			virtual void reset();

			//keeping this as virtual is really important!
			// orelse, you will call base class destructor which might not be helpful
			virtual ~JavaStyleIterator();
	};

 public :
	//iterators for this class
	typedef auto_ptr<JavaStyleIterator> JIterator;

	virtual JIterator iterator();
*/
};
