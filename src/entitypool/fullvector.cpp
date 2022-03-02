#include <vector>
#include "fullvector.hpp"

namespace transm {

long FullVector::FVcounter = 0;

/** Constructors */
FullVector::FullVector()
{
	std::vector<Entity *> vP;
	vPerson = vP;
	ID = FullVector::FVcounter;
	FullVector::FVcounter++;
}

FullVector::FullVector(std::vector<Entity *> vP)
{
	vPerson = vP;
	vector<Entity *>::iterator vPiter;
	int i = 0;

	for(vPiter = vP.begin(); vPiter != vP.end(); vPiter++)
	{
		(*vPiter)->addFVindices(i, this);
		i++;
	}

	ID = FullVector::FVcounter;
	FullVector::FVcounter++;
}

FullVector::FullVector(int num, Entity *p)
{
	vector<Entity *> vP(num, p);
	vPerson = vP;
	int i;

	for(i = 0; i < num; i++)
	{
		p->addFVindices(i, this);
		assert(p->memberFVindices(i, this));
	}

	ID = FullVector::FVcounter;
	FullVector::FVcounter++;
}

/** Destructor */
FullVector::~FullVector()
{
	vPerson.clear();
}

/** Vector class wrapper functions */
/* \TESTED */
Entity *FullVector::operator[](int index)
{
	return vPerson[index];
}

/* \TESTED */
Entity *FullVector::at(int loc)
{
	return vPerson.at(loc);
}

/* \UNTESTED */
Entity *FullVector::back()
{
	return vPerson.back();
}

/* \TESTED */
vector<Entity *>::iterator FullVector::begin()
{
	return vPerson.begin();
}

/* \UNTESTED */
int FullVector::capacity()
{
	return (int)vPerson.capacity();
}

void FullVector::clear()
{
//	while (vPerson.size() > 0){
//		remove(vPerson[0]);
//	}
	vPerson.clear();
}

/* \UNTESTED */
bool FullVector::empty()
{
	return vPerson.empty();
}

bool FullVector::exists(Entity *_p)
{
	/* If this is empty, return false */
	if(empty())
	{
		return false;
	}

	/* Use persons internal vector to test */
	vector<unsigned int> pind = _p->getFVindices(this);

	if(!pind.empty())
	{
		if(vPerson[pind[0]] == _p)
			return true;
		else
			return false;
	}
	else
		return false;
}

/* \TESTED */
vector<Entity *>::iterator FullVector::end()
{
	return vPerson.end();
}

/* \UNTESTED */
Entity *FullVector::front()
{
	return vPerson.front();
}

/* \UNTESTED */
int FullVector::max_size()
{
	return (int)vPerson.max_size();
}

/* \TESTED */
void FullVector::push_back(Entity *p)
{
	vPerson.push_back(p);
	p->addFVindices((int)vPerson.size() - 1, this);
}

/* \TESTED */
vector<Entity *>::reverse_iterator FullVector::rbegin()
{
	return vPerson.rbegin();
}

/* \TESTED */
vector<Entity *>::reverse_iterator FullVector::rend()
{
	return vPerson.rend();
}

/* \UNTESTED */
void FullVector::reserve(int size)
{
	vPerson.reserve(size);
}

/* \TESTED */
int FullVector::size()
{
	return (int)vPerson.size();
}

/**
 * @function remove
 * @arguments: Person* p
 * @effects: Removes all pointers to person p from the vector while keeping the vector dense
 * @return: Returns true if all pointers were successfully removed, returns false otherwise (including if person was not in FV) */
/* \TESTED */
bool FullVector::remove(Entity *p)
{
	vector<unsigned int> pIndices = p->getFVindices(this);

	if(pIndices.empty())
	{
		return false;
	}

	vector<unsigned int>::iterator pIndIter;
	Entity *p2;

	for(pIndIter = pIndices.begin(); pIndIter != pIndices.end(); pIndIter++)
	{
		if(*pIndIter < vPerson.size() && vPerson[*pIndIter]->getID() == p->getID())
		{
			/* Move p2 from end to p's spot (check p2 != p) */
			/* The do... while continues to pop_back vPerson for as long as the end of vPerson is p */
			do
			{
				p2 = vPerson.back();
				vPerson.pop_back();

				if(p2->getID() != p->getID())
				{
					vPerson[*pIndIter] = p2;
					p2->removeFVindices((int)vPerson.size(), this);
					p2->addFVindices(*pIndIter, this);
				}
			}
			while(p2->getID() == p->getID() && vPerson.size() > *pIndIter);

			/* pop_back */
			/* remove index from p */
			/* remove then add index from p2 */
		}

		p->removeFVindices(*pIndIter, this);
	}

	return true;
}

/**
 * @function add
 * @arguments: Person* p, int num
 * @effects: Adds num copies of p to this; updates p.FVindices accordingly */
/* \TESTED */
void FullVector::add(Entity *p, int num)
{
	int i;

	for(i = 0; i < num; i++)
		push_back(p);
}

/**
 * @function selectout
 * @arguments: int index
 * @effects: removes Person* p at index from FullVector while keeping the vector dense
 * @returns: Person* p at index */
/* \TESTED */
Entity *FullVector::selectout(int index)
{
	if(index < static_cast<int>(vPerson.size()))
	{
		Entity *p = vPerson[index];
		Entity *p2 = vPerson.back();
		vPerson[index] = p2;
		vPerson.pop_back();

		/* If we're selecting out the end of the vector, don't move the end element back to index */
		if(p2 != p)
		{
			p->removeFVindices(index, this);
			p2->addFVindices(index, this);
		}

		p2->removeFVindices((int)vPerson.size(), this);
		return p;
	}
	else
	{
		return nullptr;
	}
}

/**
 * @function swapelements
 * @arguments: int index1, int index2
 * @effects: Sets vPerson[index1] = vPerson[index2] and vice versa;
 * updates the FVindices of moved persons accordingly
 * @returns: true if the swap was made, false otherwise */
/* \TESTED */
bool FullVector::swapelements(int index1, int index2)
{
	if(index1 < static_cast<int>(vPerson.size()) && index2 < static_cast<int>(vPerson.size()))
	{
		Entity *p1 = vPerson[index1];
		Entity *p2 = vPerson[index2];

		if(p1 != p2)
		{
			vPerson[index1] = p2;
			vPerson[index2] = p1;
			p1->removeFVindices(index1, this);
			p1->addFVindices(index2, this);
			p2->removeFVindices(index2, this);
			p2->addFVindices(index1, this);
		}

		return true;
	}
	else
	{
		return false;
	}
}

void FullVector::print()
{
	vector<Entity *>::iterator iter;
	cout << "FV = : ";

	for(iter = begin(); iter != end(); iter++)
	{
		cout << (*iter)->getID() << " : ";
	}

	cout << std::endl;
}

int FullVector::getID() const
{
	return ID;
}

} // namespace transm
