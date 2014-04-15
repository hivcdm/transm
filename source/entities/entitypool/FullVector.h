#pragma once

#include <vector>
#include "../Person.h"

class Person;

/// <summary>
/// This provides a std::vector< Person > that is guaranteed to always be densely
/// populated (i.e. no empty slots).  It used for the random selection process with
/// a stratified distribution.
/// </summary>
/// <remarks>
/// Because a person may be included more than once in the FullVector, it should not be
/// used to iterate through all Persons.
/// </remarks>
class FullVector
{
	static long FVcounter;

public:
	FullVector();
	FullVector(vector<Person *> vP);
	FullVector(int num, Person *p);

	~FullVector();

	/*
	* WARNING: This operator can be used to retrieve values, but NOT to set them.
	* Use the protected function "set" to set values at a specific index.
	*/
	Person *operator[](int index);

	Person *at(int loc);

	Person *back();

	std::vector<Person *>::iterator begin();

	int capacity();

	void clear();

	bool empty();

	bool exists(Person *_p);

	std::vector<Person *>::iterator end();

	Person *front();

	int max_size();

	void push_back(Person *p);

	std::vector<Person *>::reverse_iterator rbegin();

	std::vector<Person *>::reverse_iterator rend();

	void reserve(int size);

	int size();

	//New functions specific to the FullVector class

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
	void add(Person *p, int num);

	/* @function selectout
	 * @arguments: int index
	 * @effects: removes Person* p at index from FullVector while keeping the vector dense
	 * @returns: Person* p at index
	 */
	Person *selectout(int index);

	/* @function swapelements
	 * @arguments: int index1, int index2
	 * @effects: Sets vPerson[index1] = vPerson[index2] and vice versa;
	 * updates the FVindices of moved persons accordingly
	 * @returns: true if the swap was made, false otherwise
	 */
	bool swapelements(int index1, int index2);

	void print();

	int getID();

protected:
	std::vector<Person *> vPerson;
	int ID;
};
