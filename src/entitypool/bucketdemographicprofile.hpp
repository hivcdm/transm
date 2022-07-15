#ifndef BUCKETDEMOGRAPHICPROFILE_HPP
#define BUCKETDEMOGRAPHICPROFILE_HPP

#include "entityindex.hpp"
#include "entities/demographicprofile.hpp"
#include "entities/entity.hpp"

namespace transm {

class Entity;
class RandomNumberGenerator;

 /**
  * This class is a simple container for Entitys and allows us to add, count, get, and remove them
  * <remarks>
  * This class is related to class DemographicProfile in that for each unique DemographicProfile, there is one and only one BucketDemographicProfile
  * The internal representation of the entities is a set. People are unsorted.
  * </remarks> */
class BucketDemographicProfile
{
public:
    /** this is an index based on person's ID */
    using PersonSet = EntityIndex<Entity::ID, unsigned long>;

    /** this function should not be used in this sim, it's just here for a default constructor */
    BucketDemographicProfile();

    /**
     * this creates a BucketDemographicProfile object
     * @param _id sets this as this bucket's ID
     * @param _simpleIndex - if this is true, then this BucketDemographicProfile uses an EntityIndex */
    BucketDemographicProfile(int _id, const std::string *_bucketLabel, bool _simpleIndex);

    /** deletes all entities inside this BucketDemographicProfile */
    virtual ~BucketDemographicProfile();

     /** returns the BucketDemographicProfile's ID number */
	DemographicProfile::ProfileID getProfileID();

	PersonSet *getEntityIndex();

    /** This method will return a label for this BucketDemographicProfile */
    const std::string *getLabel();

    /** empties this BucketDemographicProfile */
	virtual void clear();

    /**
	 * choose random person from the BucketDemographicProfile
     *
     * <remarks>
	 *  _remove - if true, then will remove the chosen person from the BucketDemographicProfile
     * </remarks> */
	virtual Entity *drawMember(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType, bool _remove);

    /**
	 * Draws a partner from this Bucket on behalf of _chooser. This will take into account the
	 * chooser's partner selection requirements for the given partnership type
     * <remarks>
	 * @param _randomNums random number generator
	 * @param _chooser the person who is choosing a partner
	 * @param _partnershipType the type of partner this person is looking for
	 * @param _remove - will remove this person from the bucket
	 * </remarks> */
	virtual Entity *drawMember(RandomNumberGenerator &_randomNums, Entity *_chooser, SexualPartnership::Type _partnershipType, bool _remove);

	virtual bool exists(Entity *_person);

    /** counts number of infected people this EntityPool */
	virtual unsigned long getNumInfected();

    /** counts number of infected people this EntityPool */
	virtual unsigned long getNumInfected(int generation);

    /** this adds member into the pool
	 * _toInsert - the Person being added to the pool. */
	virtual bool insert(Entity *_toInsert);

    /** if _p exists in the bucket, will remove. remove true if existed */
	virtual bool erase(Entity *_p);

    /** returns the size of this BucketDemographicProfile */
	virtual unsigned long size();

    /**
	 * @effects: Ages everyone in the bucket one timestep
	 * @returns: List of persons too old for timestep (should be placed into other bucket) */
	virtual std::list<Entity *> ageOneTimeStep();

    virtual void forEach(std::function<void(Entity *)> callback);

    std::multimap<unsigned long, Entity *>::iterator begin() { return simpleEntityIndex->begin(); }

    std::multimap<unsigned long, Entity *>::iterator end() { return simpleEntityIndex->end(); }

private:
    friend class JavaStyleIterator;

    const std::string *bucketLabel;

    /** holds all the entities in this index */
    PersonSet *simpleEntityIndex;

    /** ID of BucketDemographicProfile. id's go from 0 -> total number of buckets in EntityPool */
    DemographicProfile::ProfileID dmgProfileID;
};

} // namespace transm


#endif /* BUCKETDEMOGRAPHICPROFILE_HPP */