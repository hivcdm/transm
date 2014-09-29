#pragma once

#include "EntityIndex.hpp"
#include "entities/DemographicProfile.hpp"

class Person;
class RandomNumberGenerator;

/// <summary>
/// This class is a simple container for Entitys and allows us to add, count, get, and remove them
/// </summary>
/// <remarks>
/// This class is related to class DemographicProfile in that for each unique DemographicProfile, there is one and only one BucketDemographicProfile
/// The internal representation of the entities is a set. People are unsorted.
/// </remarks>
class BucketDemographicProfile
{
public:
    /// <summary>
    /// this is an index based on person's ID
    /// </summary>
    typedef EntityIndex<Person::ID, unsigned long> PersonSet;

    /// <summary>
    /// this function should not be used in this sim, it's just here for a default constructor
    /// </summary>
    BucketDemographicProfile();

    /// <summary>
    /// this creates a BucketDemographicProfile object
    /// @param _id sets this as this bucket's ID
    /// @param _simpleIndex - if this is true, then this BucketDemographicProfile uses an EntityIndex
    /// </summary>
    BucketDemographicProfile(int _id, const std::string *_bucketLabel, bool _simpleIndex);

    /// <summary>
    /// deletes all entities inside this BucketDemographicProfile
    /// </summary>
    virtual ~BucketDemographicProfile();

    /// <summary>
	/// returns the BucketDemographicProfile's ID number
    /// </summary>
	DemographicProfile::ProfileID getProfileID();

    /// <summary>
    /// </summary>
	PersonSet *getEntityIndex();

    /// <summary>
	/// This method will return a label for this BucketDemographicProfile
    /// </summary>
    const std::string *getLabel();

    /// <summary>
	/// empties this BucketDemographicProfile
    /// </summary>
	virtual void clear();

    /// <summary>
	/// choose random person from the BucketDemographicProfile
    /// <summary>
    /// <remarks>
	///  _remove - if true, then will remove the chosen person from the BucketDemographicProfile
    /// </remarks>
	virtual Person *drawMember(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType, bool _remove);

    /// <summary>
	/// Draws a partner from this Bucket on behalf of _chooser. This will take into account the
	/// chooser's partner selection requirements for the given partnership type
    /// </summary>
    /// <remarks>
	/// @param _randomNums random number generator
	/// @param _chooser the person who is choosing a partner
	/// @param _partnershipType the type of partner this person is looking for
	/// @param _remove - will remove this person from the bucket
	/// </remarks>
	virtual Person *drawMember(RandomNumberGenerator &_randomNums, Person *_chooser, SexualPartnership::Type _partnershipType, bool _remove);

    /// <summary>
    /// </summary>
	virtual bool exists(Person *_person);

    /// <summary>
	/// counts number of infected people this EntityPool
    /// </summary>
	virtual unsigned long getNumInfected();

    /// <summary>
    /// counts number of infected people this EntityPool
    /// </summary>
	virtual unsigned long getNumInfected(int generation);

    /// <summary>
	/// this adds member into the pool
	/// _toInsert - the Person being added to the pool.
    /// </summary>
	virtual bool insert(Person *_toInsert);

    /// <summary>
	/// if _p exists in the bucket, will remove. remove true if existed
    /// </summary>
	virtual bool erase(Person *_p);

    /// <summary>
	/// returns the size of this BucketDemographicProfile
    /// </summary>
	virtual unsigned long size();

    /// <summary>
	/// lists all members of a specified entitypool on a different line
	/// _prefix - will append this string to the front of each member and then print
    /// </summary>
    virtual void print(std::ostream &_outStream, const std::string &_prefix);

    /// <summary>
	/// @effects: Ages everyone in the bucket one timestep
	/// @returns: List of persons too old for timestep (should be placed into other bucket)
    /// </summary>
	virtual std::list<Person *> ageOneTimeStep();

    virtual void forEach(std::function<void(Person *)> callback);

    std::multimap<unsigned long, Person *>::iterator begin() { return simpleEntityIndex->begin(); }

    std::multimap<unsigned long, Person *>::iterator end() { return simpleEntityIndex->end(); }

private:
    friend class JavaStyleIterator;

    const std::string *bucketLabel;

    /// <summary>
    /// holds all the entities in this index
    /// </summary>
    PersonSet *simpleEntityIndex;

    /// <summary>
    /// ID of BucketDemographicProfile. id's go from 0 -> total number of buckets in EntityPool
    /// </summmary>
    DemographicProfile::ProfileID dmgProfileID;
};
