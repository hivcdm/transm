/*
 * FullVector.cpp
 *
 *  Created on: Nov 3, 2008
 *      Author: errhode
 */

#include "FullVector.h"
#include "../../Person.h"

int FullVector::FVcounter = 0;

//Constructors

FullVector::FullVector() : ID(FVcounter++)
{
}

FullVector::FullVector(const container &vP) : ID(FVcounter++)
{
	vPerson = vP;
	int i = 0;

	for(auto person : vP)
	{
		person->addFVindices(i, this);
		i++;
	}
}

FullVector::FullVector(int num, Person *p) : ID(FVcounter++)
{
	container vP(num, p);
	vPerson = vP;

	for(int i = 0; i < num; i++)
	{
		p->addFVindices(i, this);
		assert(p->memberFVindices(i, this));
	}
}

bool FullVector::contains(Person *_p) const
{
	//If this is empty, return false
	if(empty())
	{
		return false;
	}

	//Use persons internal vector to test
	auto pind = _p->getFVindices(this);

	if(pind.size() > 0)
	{
		if(this->vPerson[pind[0]] == _p)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

void FullVector::push_back(const reference value)
{
	vPerson.push_back(value);
	value->addFVindices(static_cast<int>(vPerson.size() - 1), this);
}

/* @function remove
 * @arguments: Person* p
 * @effects: Removes all pointers to person p from the vector while keeping the vector dense
 * @return: Returns true if all pointers were successfully removed, returns false otherwise (including if person was not in FV)
 */
bool FullVector::remove(Person *p)
{
	auto pIndices = p->getFVindices(this);

	if(pIndices.empty())
	{
		return false;
	}

	for(auto pIndIter = pIndices.begin(); pIndIter != pIndices.end(); pIndIter++)
	{
		if(*pIndIter < vPerson.size() && vPerson[*pIndIter]->getID() == p->getID())
		{
			Person *p2 = nullptr;
			//Move p2 from end to p's spot (check p2 != p)
			//The do... while continues to pop_back vPerson for as long as the end of vPerson is p
			do
			{
				p2 = vPerson.back();
				vPerson.pop_back();

				if(p2->getID() != p->getID())
				{
					vPerson[*pIndIter] = p2;
					p2->removeFVindices(static_cast<int>(vPerson.size()), this);
					p2->addFVindices(*pIndIter, this);
				}
			}
			while(p2->getID() == p->getID() && vPerson.size() > *pIndIter);

			//pop_back
			//remove index from p
			//remove then add index from p2
		}

		p->removeFVindices(*pIndIter, this);
	}

	return true;
}

/* @function add
 * @arguments: Person* p, int num
 * @effects: Adds num copies of p to this; updates p.FVindices accordingly
 */
void FullVector::add(Person *p, size_type num)
{
	for(size_type i = 0; i < num; i++)
	{
		push_back(p);
	}
}

/* @function selectout
 * @arguments: int index
 * @effects: removes Person* p at index from FullVector while keeping the vector dense
 * @returns: Person* p at index
 */
Person *FullVector::selectout(size_type index)
{
	if(index < vPerson.size())
	{
		Person *p = vPerson[index];
		Person *p2 = vPerson.back();
		vPerson[index] = p2;
		vPerson.pop_back();

		//If we're selecting out the end of the vector, don't move the end element back to index
		if(p2 != p)
		{
			p->removeFVindices(static_cast<int>(index), this);
			p2->addFVindices(static_cast<int>(index), this);
		}

		p2->removeFVindices(static_cast<int>(vPerson.size()), this);
		return p;
	}
	else
	{
		return nullptr;
	}
}

/* @function swapelements
 * @arguments: int index1, int index2
 * @effects: Sets vPerson[index1] = vPerson[index2] and vice versa;
 * updates the FVindices of moved persons accordingly
 * @returns: true if the swap was made, false otherwise
 */
bool FullVector::swapelements(size_type index1, size_type index2)
{
	if(index1 < vPerson.size() && index2 < vPerson.size())
	{
		Person *p1 = vPerson[index1];
		Person *p2 = vPerson[index2];

		if(p1 != p2)
		{
			vPerson[index1] = p2;
			vPerson[index2] = p1;
			p1->removeFVindices(static_cast<int>(index1), this);
			p1->addFVindices(static_cast<int>(index2), this);
			p2->removeFVindices(static_cast<int>(index2), this);
			p2->addFVindices(static_cast<int>(index1), this);
		}

		return true;
	}
	else
	{
		return false;
	}
}

void FullVector::print() const
{
	vector<Person *>::iterator iter;
	cout << "FV = : ";

	for(const auto person : *this)
	{
		cout << person->getID() << " : ";
	}

	cout << endl;
}
