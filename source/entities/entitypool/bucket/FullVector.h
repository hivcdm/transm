/*
 * FullVector.h
 *
 *  Created on: Nov 3, 2008
 *      Author: errhode
 */
#pragma once

#include <vector>

class Person;

/*
 * This provides a std::vector< Person > that is guaranteed to always be densely
 * populated (i.e. no empty slots).  It used for the random selection process with
 * a stratified distribution.
 *
 * Because a person may be included more than once in the FullVector, it should not be
 * used to iterate through all Persons.
*/

class FullVector
{
public:
	// Member types
	typedef Person * value_type;
	typedef std::vector<value_type> container;
	typedef container::allocator_type allocator_type;
	typedef container::size_type size_type;
	typedef container::difference_type difference_type;
	typedef container::reference reference;
	typedef container::const_reference const_reference;
	typedef container::pointer pointer;
	typedef container::const_pointer const_pointer;
	typedef container::iterator iterator;
	typedef container::const_iterator const_iterator;
	typedef container::reverse_iterator reverse_iterator;
	typedef container::const_reverse_iterator const_reverse_iterator;

	// Member functions
	FullVector();

	FullVector(const container &vP);

	FullVector(int num, Person *p);

	~FullVector() {}

	// ELement access
	reference at(size_type pos) { return vPerson.at(pos); }

	const_reference at(size_type pos) const { return vPerson.at(pos); }

	reference operator[](size_type pos) { return vPerson[pos]; }

	const_reference operator[](size_type pos) const { return vPerson[pos]; }

	reference front() { return vPerson.front(); }

	const_reference front() const { return vPerson.front(); }

	reference back() { return vPerson.back(); }

	const_reference back() const { return vPerson.back(); }

	pointer data() { return vPerson.data(); }

	const_pointer data() const { return vPerson.data(); }

	// Iterators
	iterator begin() { return vPerson.begin(); }

	const_iterator begin() const { return vPerson.cbegin(); }

	const_iterator cbegin() const { return vPerson.cbegin(); }

	iterator end() { return vPerson.end(); }

	const_iterator end() const { return vPerson.cend(); }

	const_iterator cend() const { return vPerson.cend(); }

	reverse_iterator rbegin() { return vPerson.rbegin(); }

	const_reverse_iterator rbegin() const { return vPerson.crbegin(); }

	const_reverse_iterator crbegin() const { return vPerson.crbegin(); }

	reverse_iterator rend() { return vPerson.rend(); }

	const_reverse_iterator rend() const { return vPerson.crend(); }

	const_reverse_iterator crend() const { return vPerson.crend(); }

	// Capacity
	bool empty() const { return vPerson.empty(); }

	size_type size() const { return vPerson.size(); }

	size_type max_size() const { return vPerson.max_size(); }

	void reserve(size_type new_cap) { vPerson.reserve(new_cap); }

	size_type capacity() const { return vPerson.capacity(); }

	// Modifiers
	void clear() { vPerson.clear(); }

	void push_back(const reference value);

	//void push_back(value_type &&value) { vPerson.push_back(value); }

	//New functions specific to the FullVector class

	bool contains(Person *_p) const;

	/* @function remove
	 * @arguments: Person* p
	 * @effects: Removes all pointers to person p from the vector while keeping the vector dense
	 * @return: Returns true if all pointers were successfully removed, returns false otherwise
	 */
	bool remove(Person *p);

	/* @function add
	 * @arguments: Person* p, int num
	 * @effects: Adds num copies of p to this; updates p.FVindices accordingly
	 */
	void add(Person *p, size_type num);

	/* @function selectout
	 * @arguments: int index
	 * @effects: removes Person* p at index from FullVector while keeping the vector dense
	 * @returns: Person* p at index
	 */
	Person *selectout(size_type index);

	/* @function swapelements
	 * @arguments: int index1, int index2
	 * @effects: Sets vPerson[index1] = vPerson[index2] and vice versa;
	 * updates the FVindices of moved persons accordingly
	 * @returns: true if the swap was made, false otherwise
	 */
	bool swapelements(size_type index1, size_type index2);

	void print() const;

	int getID() const { return ID; }

protected:
	static int FVcounter;

	container vPerson;

	int ID;
};
