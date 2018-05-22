#pragma once

#include <iostream>
#include <list>
#include <map>
#include <typeinfo>

#include "entities/entity.hpp"
#include "utility/utility.hpp"
#include "utility/randomnumbergenerator.hpp"

namespace transm {

/// <summary>
/// This class indexes Person objects based on numerical key values
/// Internal storage is managed by the a multi_map with an underlying B+ tree.
/// 
/// _PSC holds the key that we search and index against.
/// </summary>
template <Entity::SelectingCriteria _PSC, class _KeyValType>
class EntityIndex
{

private:

	friend class JavaStyleIterator;

    using PersonMultiMap = std::multimap<_KeyValType, Entity *>;
    using CPPIterator = typename PersonMultiMap::iterator;

	PersonMultiMap personMultiMap;

	unsigned long numPeople;		//number of people in index
public :

	EntityIndex();
	~EntityIndex();

	//clears all elements from this index
	void clear();

    /// <summary>
	/// draw any member from this pool, this function has a speed optimization
    /// </summary>
    /// <remarks>
	/// this function is used by class BucketSexualMixing
	/// draw a particular key first to narrow down potentials
	/// then choose randomly from among the potentials with that key
	/// assumption - all keys have an equal opportunity of being picked regardless
    ///			  of the # of Entitys with that key
	///			- if a key is chosen where there are no entities, choose the next
	///				key w/ members in it
    /// </remarks>
	Entity *drawMember(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType, bool _remove);

    /// <summary>
	/// draw a member from this pool
    /// </summary>
	Entity *drawMember(RandomNumberGenerator &_randomNums, Entity *_chooser, SexualPartnership::Type _partnershipType, bool _remove);

    /// <summary>
	/// Erases the _person from the index. Returns true if this person was 
    /// actually part of the index and was erased.
    /// </summary>
	bool erase(Entity *_person);

    /// <summary>
	/// tells whether _person exists in the index
    /// </summary>
	bool exists(Entity *_person);

    /// </summary>
	/// draws person at position _randomAccessIndex in this index. This is random access...slow but necessary
    /// </summary>
	Entity *getMember(unsigned long _randomAccessIndex, bool _remove);

    /// <summary>
	/// will return how many HIV infected people are currently in the index
    /// </summary>
	unsigned long getNumInfected();

    /// <summary>
    /// will return how many HIV infected people are currently in the index in the given infection generation.
    /// </summary>
	unsigned long getNumInfected(int generation);

    /// <summary>
	/// will index a new person
    /// </summary>
	bool insert(Entity *_person);

    /// <summary>
	/// will remove this person (if he or she exists) from the index
	/// returns the # of entities in this index
    /// </summary>
	unsigned int size();

	typename std::multimap<_KeyValType, Entity *>::iterator begin();
	typename std::multimap<_KeyValType, Entity *>::iterator end();
private:

	//finds the location of an entry in the index,
	CPPIterator find(Entity *_person);
public:

	class JavaStyleIterator
	{
		EntityIndex<_PSC, _KeyValType> *index; //pool that this iterator will run through

		bool currRemoved;

		//this are internal pointers that keep track of where the JavaStyleIterator is
		CPPIterator currElement;
		CPPIterator nextElement;
		CPPIterator end;

	public:
		JavaStyleIterator(EntityIndex<_PSC, _KeyValType> *_index);

		//returns true if the element that was last returned by next() has been removed using remove()
		bool alreadyRemoved();

		//returns true if there are more members to iterate through
		bool hasNext();

		//get the person that was last returned
		Entity *get();

		//this will be used to get the next in line
		Entity *next();

		//removes from the collection the last element returned by the iterator
		bool remove();

		//lets us reuse an iterator, resets to beginning of current collection
		void reset();

		~JavaStyleIterator();
	};

    using JIterator = unique_ptr<JavaStyleIterator>;

	//returns a EntityIndex<_PSC,_KeyValType>::JIterator
	typename EntityIndex<_PSC, _KeyValType>::JIterator iterator();

};

template <Entity::SelectingCriteria _PSC, class _KeyValType>
EntityIndex<_PSC, _KeyValType>::JavaStyleIterator::JavaStyleIterator(EntityIndex<_PSC, _KeyValType> *_index)
{
	index = _index;
	//initialize this iterator to iterate from first element
	reset();
}

//returns true if the element that was last returned by next() has been removed using remove()
template <Entity::SelectingCriteria _PSC, class _KeyValType>
bool EntityIndex<_PSC, _KeyValType>::JavaStyleIterator::alreadyRemoved()
{
	return currRemoved;
}

//gets the last element returned by the iterator
template <Entity::SelectingCriteria _PSC, class _KeyValType>
Entity *EntityIndex<_PSC, _KeyValType>::JavaStyleIterator::get()
{
	return (*currElement).second;
}

//returns true if there are more members to iterate through
template <Entity::SelectingCriteria _PSC, class _KeyValType>
bool EntityIndex<_PSC, _KeyValType>::JavaStyleIterator::hasNext()
{
	return (nextElement != end);
}

//this will be used to get the next in line
template <Entity::SelectingCriteria _PSC, class _KeyValType>
Entity *EntityIndex<_PSC, _KeyValType>::JavaStyleIterator::next()
{
	//return the element that was stored previously
	currElement = nextElement;
	//store the next element to return on then next call of next()
	nextElement++;
	//reset flag that indicates whether remove() has been called on current element
	currRemoved = false;
	return (*currElement).second;
}

//removes from the collection the last element returned by the iterator
template <Entity::SelectingCriteria _PSC, class _KeyValType>
bool EntityIndex<_PSC, _KeyValType>::JavaStyleIterator::remove()
{
	//remove the person that was most recently returned by the iterator
	if(!currRemoved && (currElement != end))
	{
		//user is removing first element, the we have to do a special adjustment
		if(currElement == nextElement)
		{
			nextElement++;
			index->personMultiMap.erase(currElement);
			currElement = nextElement;
		}
		else
		{
			index->personMultiMap.erase(currElement);
		}

		//do some book keeping for the index and the current iterator
		currRemoved = true;
		index->numPeople--;
		return true;
	}

	return false;
}

template <Entity::SelectingCriteria _PSC, class _KeyValType>
void EntityIndex<_PSC, _KeyValType>::JavaStyleIterator::reset()
{
    //pointer to element to current element to return
	currElement = nextElement = index->personMultiMap.begin();
	currRemoved = false;
    //pointer to end of set
	end = index->personMultiMap.end();
}

template <Entity::SelectingCriteria _PSC, class _KeyValType>
EntityIndex<_PSC, _KeyValType>::JavaStyleIterator::~JavaStyleIterator()
{
	index = nullptr;
}

template <Entity::SelectingCriteria _PSC, class _KeyValType>
EntityIndex<_PSC, _KeyValType>::EntityIndex()
{
	numPeople = 0;
}


template <Entity::SelectingCriteria _PSC, class _KeyValType>
EntityIndex<_PSC, _KeyValType>::~EntityIndex()
{
	//delete every Relational Person in this BucketDemographicProfile
	JIterator iter = iterator();

	while(iter->hasNext())
	{
		Entity *r = iter->next();
		iter->remove();
		delete r;
	}
}


template <Entity::SelectingCriteria _PSC, class _KeyValType>
void EntityIndex<_PSC, _KeyValType>::clear()
{
	personMultiMap.clear();
	numPeople = 0;
}


template <Entity::SelectingCriteria _PSC, class _KeyValType>
Entity *EntityIndex<_PSC, _KeyValType>::drawMember(RandomNumberGenerator &_randomNums,
        SexualPartnership::Type /*_partnershipType*/, bool _remove)
{
	//assume that everyone in this pool has an equal shot at being chosen
	int numPotentials = size();

	//return nullptr if this index is empty
	if(numPotentials == 0)
	{
		return nullptr;
	}

	return drawMember(_randomNums, nullptr, SexualPartnership::Type::ENDType, _remove);
}

template <Entity::SelectingCriteria _PSC, class _KeyValType>
Entity *EntityIndex<_PSC, _KeyValType>::drawMember(RandomNumberGenerator &_randomNums, Entity *_chooser,
        SexualPartnership::Type _partnershipType, bool _remove)
{
	int numPotentials = 0; //how many potential people in this range
	cout << "EntityIndex drawMember being called for person " << _chooser->getID() << std::endl;
	//these determine the bounds of which keys we will consider
	_KeyValType minDesired = std::numeric_limits<_KeyValType>::min();
	_KeyValType maxDesired = std::numeric_limits<_KeyValType>::max();

	//if there is a specific choose then make sure they want a specific kind of partnership
	if(_chooser != nullptr)
	{
		assert(_partnershipType != SexualPartnership::Type::ENDType);
		minDesired = (_KeyValType)_chooser->getMinPartnerSelectVal(_PSC, _partnershipType);
		maxDesired = (_KeyValType)_chooser->getMaxPartnerSelectVal(_PSC, _partnershipType);
		assert(minDesired <= maxDesired);
	}

	//the lower and upper bound of potential partner pool
	CPPIterator potentialsStart =  personMultiMap.lower_bound(minDesired);
	CPPIterator potentialsEnd =  personMultiMap.upper_bound(maxDesired);
	//count how many potentials there are
	CPPIterator iter = potentialsStart;

	while(iter != potentialsEnd)
	{
		numPotentials++;
		iter++;
	}

	//if nobody is available, stop now
	if(numPotentials == 0)
	{
		return nullptr;
	}

	//get an individual
	int toPick = _randomNums.randInt(0, numPotentials - 1);
	int peopleChecked = 0;
	iter = potentialsStart;

	//try for all the available potentials from youngest to oldest
	while((iter != potentialsEnd) && (peopleChecked < toPick))
	{
		iter++;
		peopleChecked++;
	}

	Entity *person = iter->second;

	//if we choose the potential, then return partner
	if(_remove && person)
	{
		//do some book keeping of counts before deleting
		numPeople--;
		personMultiMap.erase(iter);
	}

	return person;
}

template <Entity::SelectingCriteria _PSC, class _KeyValType>
Entity *EntityIndex<_PSC, _KeyValType>::getMember(unsigned long _randomAccessIndex, bool _remove)
{
	assert(_randomAccessIndex < numPeople);

	//return nullptr if there are no more people
	if(numPeople == 0)
	{
		return nullptr;
	}

	//traverse to person at _randomAccessIndex
	CPPIterator iter = personMultiMap.begin();

	while((iter != personMultiMap.end()) && (_randomAccessIndex > 0))
	{
		iter++;
		_randomAccessIndex--;
	}

	Entity *person = iter->second;

	//remove person if _remove == true
	if(_remove && person)
	{
		numPeople--;
		personMultiMap.erase(iter);
	}

	return person;
}

template <Entity::SelectingCriteria _PSC, class _KeyValType>
bool EntityIndex<_PSC, _KeyValType>::erase(Entity *_person)
{
	std::pair<CPPIterator, CPPIterator> personsWithKey = personMultiMap.equal_range(
	            Entity::Sorter<_PSC, _KeyValType>::getSortKey(_person));
	CPPIterator curr;

	//if people with the same key as _person exist
	if(personsWithKey.first != personMultiMap.end())
	{
		//iterate through all entries that match this key to find _person
		for(curr = personsWithKey.first; curr != personsWithKey.second; ++curr)
		{
			//if we have found that person, erase them
			if(curr->second == _person)
			{
				personMultiMap.erase(curr);
				numPeople--;
				return true;
			}
		}
	}

	return false;
}

template <Entity::SelectingCriteria _PSC, class _KeyValType>
bool EntityIndex<_PSC, _KeyValType>::exists(Entity *_person)
{
	std::pair<CPPIterator, CPPIterator> personsWithKey = personMultiMap.equal_range(
	            Entity::Sorter<_PSC, _KeyValType>::getSortKey(_person));
	CPPIterator curr;

	//if people with the same key as _person exist
	if(personsWithKey.first != personMultiMap.end())
	{
		//iterate through all entries that match this key to find _person
		for(curr = personsWithKey.first; curr != personsWithKey.second; ++curr)
		{
			//if we have found that person, erase them
			if(curr->second == _person)
			{
				return true;
			}
		}
	}

	return false;
}


template <Entity::SelectingCriteria _PSC, class _KeyValType>
typename EntityIndex<_PSC, _KeyValType>::CPPIterator EntityIndex<_PSC, _KeyValType>::find(Entity *_person)
{
	std::pair<CPPIterator, CPPIterator> personsWithKey = personMultiMap.equal_range(
	            Entity::Sorter<_PSC, _KeyValType>::getSortKey(_person));
	CPPIterator curr;

	//if people with the same key as _person exist
	if(personsWithKey.first != personMultiMap.end())
	{
		//iterate through all entries that match this key to find _person
		for(curr = personsWithKey.first; curr != personsWithKey.second; ++curr)
		{
			if(curr->second == _person)
			{
				return curr;
			}
		}
	}

	personMultiMap.end();
}

template <Entity::SelectingCriteria _PSC, class _KeyValType>
unsigned long EntityIndex<_PSC, _KeyValType>::getNumInfected()
{
	unsigned long numInfected = 0;
	//iterates through all elements
	CPPIterator iter = personMultiMap.begin();

	while(iter != personMultiMap.end())
	{
		if((iter->second)->isInfected())
		{
			numInfected++;
		}

		iter++;
	}

	return numInfected;
}

template <Entity::SelectingCriteria _PSC, class _KeyValType>
unsigned long EntityIndex<_PSC, _KeyValType>::getNumInfected(int generation)
{
	unsigned long numInfected = 0;
	//iterates through all elements
	CPPIterator iter = personMultiMap.begin();

	while(iter != personMultiMap.end())
	{
		if((iter->second)->isInfected() && (iter->second)->getGenerationOfInfection() == generation)
		{
			numInfected++;
		}

		iter++;
	}

	return numInfected;
}

template <Entity::SelectingCriteria _PSC, class _KeyValType>
bool EntityIndex<_PSC, _KeyValType>::insert(Entity *_person)
{
	pair<_KeyValType, Entity *> toInsert = pair<_KeyValType, Entity *>(Entity::Sorter<_PSC, _KeyValType>::getSortKey(
	        _person), _person);
	personMultiMap.insert(toInsert);
	//if this person was successfully inserted, then update some book-keeping
	numPeople++;
	return true;
}

template <Entity::SelectingCriteria _PSC, class _KeyValType>
unsigned int EntityIndex<_PSC, _KeyValType>::size()
{
	assert(numPeople == personMultiMap.size());
	return numPeople;
}

template <Entity::SelectingCriteria _PSC, class _KeyValType>
typename EntityIndex<_PSC, _KeyValType>::JIterator EntityIndex<_PSC, _KeyValType>::iterator()
{
	return typename EntityIndex<_PSC, _KeyValType>::JIterator(new JavaStyleIterator(this));
}

template <Entity::SelectingCriteria _PSC, class _KeyValType>
typename std::multimap<_KeyValType, Entity *>::iterator EntityIndex<_PSC, _KeyValType>::begin()
{
	return personMultiMap.begin();
}

template <Entity::SelectingCriteria _PSC, class _KeyValType>
typename std::multimap<_KeyValType, Entity *>::iterator EntityIndex<_PSC, _KeyValType>::end()
{
	return personMultiMap.end();
}

} // namespace transm
