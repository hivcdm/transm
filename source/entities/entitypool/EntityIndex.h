#pragma once

#include <map>
#include <list>
#include <iostream>
#include <typeinfo>


#include "../Person.h"
#include "../../util/Utility.h"
#include "../../util/rand/RandomNumberGenerator.h"

/// <summary>
/// This class indexes Person objects based on numerical key values
/// Internal storage is managed by the a multi_map with an underlying B+ tree.
/// 
/// _PSC holds the key that we search and index against.
/// </summary>
template <Person::SelectingCriteria _PSC, class _KeyValType>
class EntityIndex
{

private:

	friend class JavaStyleIterator;

	typedef std::multimap<_KeyValType, Person *> PersonMultiMap;
	typedef typename PersonMultiMap::iterator CPPIterator;

	PersonMultiMap personMultiMap;

	unsigned long numPeople;		//number of people in index
public :

	EntityIndex();
	~EntityIndex();


	//---------------< Start Methods inherited from EntityContainerInterface >-------------------//
	//clears all elements from this index
	void clear();

	//draw any member from this pool, this function has a speed optimization
	//this function is used by class BucketSexualMixing
	//  draw a particular key first to narrow down potentials
	//	then choose randomly from among the potentials with that key
	//  assumption - all keys have an equal opportunity of being picked regardless
	//				  of the # of Entitys with that key
	//				- if a key is chosen where there are no entities, choose the next
	//					key w/ members in it
	Person *drawMember(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType, bool _remove);

	//draw a member from this pool
	Person *drawMember(RandomNumberGenerator &_randomNums, Person *_chooser, SexualPartnership::Type _partnershipType, bool _remove);

	//erases the _person from the index. Returns true if this person was actually part of the index and
	//  was erased
	bool erase(Person *_person);

	//tells whether _person exists in the index
	bool exists(Person *_person);

	//draws person at position _randomAccessIndex in this index. This is random access...slow but necessary
	Person *getMember(unsigned long _randomAccessIndex, bool _remove);

	//will return how many HIV infected people are currently in the index
	unsigned long getNumInfected();
	unsigned long getNumInfected(int generation);

	//will index a new person
	bool insert(Person *_person);

	//will remove this person (if he or she exists) from the index
	//returns the # of entities in this index
	unsigned int size();

	//prints every person in this index to _outStream
	void print(ostream &_outStream, std::string _prefix);

	//----------------< Begin iterator methods >------------------------//
	typename multimap<_KeyValType, Person *>::iterator begin();

	typename multimap<_KeyValType, Person *>::iterator end();
	//-----------------< End iterator methods >-------------------------//

	//---------------< End Methods inherited from EntityContainerInterface >-------------------//

private:

	//finds the location of an entry in the index,
	CPPIterator find(Person *_person);
public:

	/**

	The interface matches that of the Java 1.5.0 Iterator interface, with the addition of a reset() method
		public:
		JIterator(EntityIndex)
		~JIterator()
		T next();
		bool hasNext();
		void reset();
		void remove();
	***/

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
		Person *get();

		//this will be used to get the next in line
		Person *next();

		//removes from the collection the last element returned by the iterator
		bool remove();

		//lets us reuse an iterator, resets to beginning of current collection
		void reset();

		~JavaStyleIterator();
	};

	typedef auto_ptr<JavaStyleIterator> JIterator;

	//returns a EntityIndex<_PSC,_KeyValType>::JIterator
	typename EntityIndex<_PSC, _KeyValType>::JIterator iterator();

};

//-----------< Begin Methods for EntityIndex<_PSC,_KeyValType>::JavaStyleIterator >--------------//

template <Person::SelectingCriteria _PSC, class _KeyValType>
EntityIndex<_PSC, _KeyValType>::JavaStyleIterator::JavaStyleIterator(EntityIndex<_PSC, _KeyValType> *_index)
{
	index = _index;
	//initialize this iterator to iterate from first element
	reset();
}

//returns true if the element that was last returned by next() has been removed using remove()
template <Person::SelectingCriteria _PSC, class _KeyValType>
bool EntityIndex<_PSC, _KeyValType>::JavaStyleIterator::alreadyRemoved()
{
	return currRemoved;
}

//gets the last element returned by the iterator
template <Person::SelectingCriteria _PSC, class _KeyValType>
Person *EntityIndex<_PSC, _KeyValType>::JavaStyleIterator::get()
{
	return (*currElement).second;
}

//returns true if there are more members to iterate through
template <Person::SelectingCriteria _PSC, class _KeyValType>
bool EntityIndex<_PSC, _KeyValType>::JavaStyleIterator::hasNext()
{
	return (nextElement != end);
}

//this will be used to get the next in line
template <Person::SelectingCriteria _PSC, class _KeyValType>
Person *EntityIndex<_PSC, _KeyValType>::JavaStyleIterator::next()
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
template <Person::SelectingCriteria _PSC, class _KeyValType>
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
	}  //if(!currRemoved && (currElement != end)) {

	return false;
}

template <Person::SelectingCriteria _PSC, class _KeyValType>
void EntityIndex<_PSC, _KeyValType>::JavaStyleIterator::reset()
{
	currElement = nextElement =
	                        index->personMultiMap.begin();	//pointer to element to current element to return
	currRemoved = false;
	end = index->personMultiMap.end();				//pointer to end of set
}

template <Person::SelectingCriteria _PSC, class _KeyValType>
EntityIndex<_PSC, _KeyValType>::JavaStyleIterator::~JavaStyleIterator()
{
	index = nullptr;
}

//-----------< End Methods for EntityIndex<_PSC,_KeyValType>::JavaStyleIterator >--------------//


//-----------< Begin Methods for EntityIndex<_PSC,_KeyValType> >--------------//

template <Person::SelectingCriteria _PSC, class _KeyValType>
EntityIndex<_PSC, _KeyValType>::EntityIndex()
{
	numPeople = 0;
}


template <Person::SelectingCriteria _PSC, class _KeyValType>
EntityIndex<_PSC, _KeyValType>::~EntityIndex()
{
	//delete every Relational Person in this BucketDemographicProfile
	//EntityIndex<_PSC, _KeyValType>::
	JIterator iter = iterator();

	while(iter->hasNext())
	{
		Person *r = iter->next();
		iter->remove();
		delete r;
	}
}


template <Person::SelectingCriteria _PSC, class _KeyValType>
void EntityIndex<_PSC, _KeyValType>::clear()
{
	personMultiMap.clear();
	numPeople = 0;
}


template <Person::SelectingCriteria _PSC, class _KeyValType>
Person *EntityIndex<_PSC, _KeyValType>::drawMember(RandomNumberGenerator &_randomNums,
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

template <Person::SelectingCriteria _PSC, class _KeyValType>
Person *EntityIndex<_PSC, _KeyValType>::drawMember(RandomNumberGenerator &_randomNums, Person *_chooser,
        SexualPartnership::Type _partnershipType, bool _remove)
{
	int numPotentials = 0; //how many potential people in this range
	cout << "EntityIndex drawMember being called for person " << _chooser->getID() << endl;
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
	} //while (iter != potentialsOverMaxAge) {

	Person *person = iter->second;

	//if we choose the potential, then return partner
	if(_remove && person)
	{
		//do some book keeping of counts before deleting
		numPeople--;
		personMultiMap.erase(iter);
	}

	return person;
}

template <Person::SelectingCriteria _PSC, class _KeyValType>
Person *EntityIndex<_PSC, _KeyValType>::getMember(unsigned long _randomAccessIndex, bool _remove)
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

	Person *person = iter->second;

	//remove person if _remove == true
	if(_remove && person)
	{
		numPeople--;
		personMultiMap.erase(iter);
	}

	return person;
}

template <Person::SelectingCriteria _PSC, class _KeyValType>
bool EntityIndex<_PSC, _KeyValType>::erase(Person *_person)
{
	std::pair<CPPIterator, CPPIterator> personsWithKey = personMultiMap.equal_range(
	            Person::Sorter<_PSC, _KeyValType>::getSortKey(_person));
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
			} //if(curr->second == _person) {
		} //for( i = ii.first; i != ii.second; ++i ) {
	}  //if ( personsWithKey.first != personMultiMap.end()) {

	return false;
}

template <Person::SelectingCriteria _PSC, class _KeyValType>
bool EntityIndex<_PSC, _KeyValType>::exists(Person *_person)
{
	std::pair<CPPIterator, CPPIterator> personsWithKey = personMultiMap.equal_range(
	            Person::Sorter<_PSC, _KeyValType>::getSortKey(_person));
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
		} //for( i = ii.first; i != ii.second; ++i ) {
	}  //if ( personsWithKey.first != personMultiMap.end()) {

	return false;
}


template <Person::SelectingCriteria _PSC, class _KeyValType>
typename EntityIndex<_PSC, _KeyValType>::CPPIterator EntityIndex<_PSC, _KeyValType>::find(Person *_person)
{
	std::pair<CPPIterator, CPPIterator> personsWithKey = personMultiMap.equal_range(
	            Person::Sorter<_PSC, _KeyValType>::getSortKey(_person));
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
		} //for( i = ii.first; i != ii.second; ++i ) {
	}  //if ( personsWithKey.first != personMultiMap.end()) {

	personMultiMap.end();
}

template <Person::SelectingCriteria _PSC, class _KeyValType>
unsigned long EntityIndex<_PSC, _KeyValType>::getNumInfected()
{
	unsigned long numInfected = 0;
	//iterates through all elements
	//EntityIndex<_PSC, _KeyValType>::
	CPPIterator iter = personMultiMap.begin();

	while(iter != personMultiMap.end())
	{
		if((iter->second)->isInfected())
		{
			numInfected++;
		}

		iter++;
	} //while(iter != personMultiMap.end()) {

	return numInfected;
}

template <Person::SelectingCriteria _PSC, class _KeyValType>
unsigned long EntityIndex<_PSC, _KeyValType>::getNumInfected(int generation)
{
	unsigned long numInfected = 0;
	//iterates through all elements
	//EntityIndex<_PSC, _KeyValType>::
	CPPIterator iter = personMultiMap.begin();

	while(iter != personMultiMap.end())
	{
		if((iter->second)->isInfected() && (iter->second)->getGenerationOfInfection() == generation)
		{
			numInfected++;
		}

		iter++;
	} //while(iter != personMultiMap.end()) {

	return numInfected;
}

template <Person::SelectingCriteria _PSC, class _KeyValType>
bool EntityIndex<_PSC, _KeyValType>::insert(Person *_person)
{
	pair<_KeyValType, Person *> toInsert = pair<_KeyValType, Person *>(Person::Sorter<_PSC, _KeyValType>::getSortKey(
	        _person), _person);
	personMultiMap.insert(toInsert);
	//if this person was successfully inserted, then update some book-keeping
	numPeople++;
	return true;
}

template <Person::SelectingCriteria _PSC, class _KeyValType>
unsigned int EntityIndex<_PSC, _KeyValType>::size()
{
	assert(numPeople == personMultiMap.size());
	return numPeople;
}

template <Person::SelectingCriteria _PSC, class _KeyValType>
void EntityIndex<_PSC, _KeyValType>::print(ostream &_outStream, std::string _prefix)
{
	//iterates through all elements
	//EntityIndex<_PSC, _KeyValType>::
	CPPIterator iter = personMultiMap.begin();

	while(iter != personMultiMap.end())
	{
		(iter->second)->print(_outStream, _prefix);
		_outStream << "PARTNERS ARE: ";
		(iter->second)->printCurrentPartners(_outStream, "");
		int i;

		for(i = 0; i < 8; i++)
		{
			_outStream << "-------------------------------------------------" << Constants::TAB;
		}

		_outStream << endl;
		iter++;
	}
}

template <Person::SelectingCriteria _PSC, class _KeyValType>
typename EntityIndex<_PSC, _KeyValType>::JIterator EntityIndex<_PSC, _KeyValType>::iterator()
{
	return typename EntityIndex<_PSC, _KeyValType>::JIterator(new JavaStyleIterator(this));
}

//----------------< Begin iterator methods >------------------------//
template <Person::SelectingCriteria _PSC, class _KeyValType>
typename multimap<_KeyValType, Person *>::iterator EntityIndex<_PSC, _KeyValType>::begin()
{
	return personMultiMap.begin();
}

template <Person::SelectingCriteria _PSC, class _KeyValType>
typename multimap<_KeyValType, Person *>::iterator EntityIndex<_PSC, _KeyValType>::end()
{
	return personMultiMap.end();
}
//-----------------< End iterator methods >-------------------------//

//-----------< End Methods for EntityIndex<_PSC,_KeyValType> >--------------//







