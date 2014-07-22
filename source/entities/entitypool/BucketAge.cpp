/*
 * BucketAge.cpp
 *
 *  Created on: Nov 20, 2008
 *      Author: errhode
 */

#include "BucketAge.h"

//Constructor
BucketAge::BucketAge()
{
	numPersons = 0;
	numInfected = 0;

	for(int i = 0; i < Person::ENDRiskLevel; i++)
	{
		numRisk[i] = 0;
		numRiskCSW[i] = 0;
		numInfectedRisk[i] = 0;

		for(int j = 0; j < Person::ENDHIVStatus; j++)
		{
			numRiskHIVStatus[i][j] = 0;
		}
	}

	for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
	{
		assort[partnership_type] = 0;
	}
}

BucketAge::BucketAge(DemographicProfile::ProfileID BinID, unsigned int popID, const std::map<SexualPartnership::Type, double> &_assort)
: assort(_assort)
{
	currentBinID = BinID;
	populationID = popID;
	numPersons = 0;
	numInfected = 0;

	for(int i = 0; i < Person::ENDRiskLevel; i++)
	{
		numRisk[i] = 0;
		numRiskCSW[i] = 0;
		numInfectedRisk[i] = 0;

		for(int j = 0; j < Person::ENDHIVStatus; j++)
		{
			numRiskHIVStatus[i][j] = 0;
		}
	}

	//Initialize the infected FVs
	for(int i = 0; i < NUMBER_GENERATIONS_TO_TRACE; i++)
	{
		FullVector *emptyFV = new FullVector();
		FVinfected.push_back(emptyFV);
	}
}

//Destructor
BucketAge::~BucketAge()
{
	for(int i = 0; i < NUMBER_GENERATIONS_TO_TRACE; i++)
	{
		FVinfected[i]->clear();
		delete FVinfected[i];
	}

	FVinfected.clear();
	FVinfected.clear();
	FVuninfected.clear();
	FVProbDist_low.clear();
	FVProbDist_high.clear();
	FVProbDist_random.clear();
	FVNoDist.clear();
}

//------------< Start Methods taken from EntityIndex >-------------//
//clears all elements from this index
void BucketAge::clear()
{
	for(int i = 0; i < NUMBER_GENERATIONS_TO_TRACE; i++)
	{
		assert(i < 6);
		FVinfected[i]->clear();
	}

	FVuninfected.clear();
	FVProbDist_high.clear();
	FVProbDist_low.clear();
	FVProbDist_random.clear();
	FVNoDist.clear();
	numPersons = 0;
	numInfected = 0;

	for(int i = 0; i < Person::ENDRiskLevel; i++)
	{
		numRisk[i] = 0;
		numRiskCSW[i] = 0;
		numInfectedRisk[i] = 0;

		for(int j = 0; j < Person::ENDHIVStatus; j++)
		{
			numRiskHIVStatus[i][j] = 0;
		}
	}
}

//tells whether _person exists in the index
bool BucketAge::exists(Person *p)
{
	return (FVNoDist.exists(p));
}

//will return how many HIV infected people are currently in the index
//Store as a number?  No -- Not costing significant time
unsigned long BucketAge::getNumInfected()
{
	unsigned long total = 0;

	for(int i = 0; i < NUMBER_GENERATIONS_TO_TRACE; i++)
	{
		total += FVinfected[i]->size();
	}

	return total;
}

unsigned long BucketAge::getNumInfected(int generation)
{
	return FVinfected[generation]->size();
}

unsigned long BucketAge::getNumInfected(Person::RiskLevel _risk)
{
	return numInfectedRisk[_risk];
}

//prints every person in this index to _outStream
void BucketAge::print(ostream &_outStream, const std::string &_prefix)
{
	vector<Person *>::iterator PersonIter = begin();

	while(PersonIter != end())
	{
		(*PersonIter)->print(_outStream, _prefix);
		PersonIter++;
	}
}

//-------------< End Methods taken from EntityIndex >--------------//


//-----------< Start Insertion and Retrieval Methods >-------------//
//draw any member from this pool, this function has a speed optimization
//this function is used by class BucketSexualMixing
//  draw a particular key first to narrow down potentials
//	then choose randomly from among the potentials with that key
//  assumption - all keys have an equal opportunity of being picked regardless
//				  of the # of Entities with that key
//				- if a key is chosen where there are no entities, choose the next
//					key w/ members in it
//TESTED... without random number generator
Person *BucketAge::drawMember(RandomNumberGenerator &_randomNums, Person::RiskLevel _riskLevel,
                              SexualPartnership::Type /*_partnershipType*/, bool _use_random, bool _remove)
{
	if(numPersons == 0)
	{
		return nullptr;
	}

	FullVector *toDrawFrom;

	if(_use_random)
	{
		toDrawFrom = &(FVProbDist_random);
	}
	else if(_riskLevel == Person::LOW)
	{
		toDrawFrom = &(FVProbDist_low);
	}
	else if(_riskLevel == Person::HIGH)
	{
		toDrawFrom = &(FVProbDist_high);
	}
	else
	{
		cerr << "Error: attempting to draw a member from invalid risk level: " << _riskLevel << std::endl;
		return nullptr;
	}

	if(toDrawFrom->size() > 0)
	{
		int toPick = _randomNums.randInt(0, toDrawFrom->size() - 1);

		if(_remove)
		{
			//Remove person from all FV
			Person *personToReturn = toDrawFrom->selectout(toPick);
			erase(personToReturn);
			return personToReturn;
		}
		else
		{
			return toDrawFrom->at(toPick);
		}
	}
	else
	{
		cerr << "Error: requesting person from empty set: " << DemographicProfile::toString(currentBinID) << std::endl;
		return nullptr;
	}
}

/* @function: erase
 * @effects: removes _person from this by removing _person from all FVs; decrements numPersons by 1;
 * if _person is an infected individual, decrements numInfected by 1
 * @returns: true if _person was previously a member of this and was successfully removed, false is
 * _person was not a member of this
 */
bool BucketAge::erase(Person *_person)
{
	if(exists(_person))
	{
		//Remove from all FVs
		bool removed[6];
		removed[0] = FVProbDist_high.remove(_person);
		removed[1] = FVProbDist_low.remove(_person);
		removed[2] = FVProbDist_random.remove(_person);
		removed[3] = false;

		for(int i = 0; i < NUMBER_GENERATIONS_TO_TRACE; i++)
		{
			bool removeInf = FVinfected[i]->remove(_person);
			removed[3] = (removed[3] || removeInf);
		}

		removed[4] = FVuninfected.remove(_person);
		assert((removed[0] || removed[1] || removed[2]) && (removed[3] || removed[4]));
		//Remove from linked list
		FVNoDist.remove(_person);

		//If infected, reduce count of numInfected
		if(_person->isInfected())
		{
			numInfected--;
			numInfectedRisk[_person->getRiskLevel()]--;
		}

		//Reduce count of number of people
		numPersons--;
		numRisk[_person->getRiskLevel()]--;
		numRiskHIVStatus[_person->getRiskLevel()][_person->getHIVStatus()]--;

		if(DemographicProfile::get(_person->getCurrBucketProfileID(), DemographicProfile::Demographic::Employment) == (std::size_t)DemographicProfile::Employment::Csw)
		{
			numRiskCSW[_person->getRiskLevel()]--;
		}

		return ((removed[0] || removed[1] || removed[2]) && (removed[3] || removed[4]));
	}
	else
	{
		return false;
	}
}

/* @function: insert
 * @effects: adds _person to this by adding _person to appropriate FV: FVinfected (_person.isInfected)
 * or FVuninfected, and FVProbDist_random and either FVProbDist_high or FVProbDist_low depending on
 * _person.risk and assortativeness variable (to be defined later)
 * @returns: true if person was successfully added, false otherwise
 */
//TESTED (without global assort param)
bool BucketAge::insert(Person *_person)
{
	if(exists(_person))
	{
		cerr << "Adding person to a bucket they are already in!" << std::endl;
		return false;
	}

	int marbles = _person->getSexualActivity();
	//Using Mark Lipsitch's sexual mixing algorithm based on the assortativeness value
	//Update: Mark says this is double counting the assortativeness!
	//Just put the same amount of marbles in each box
	//Update again: Mathematically proved that the two methods are the same... putting the same number of marbles in each box has less potential for bugs`
	int marblesInRandomFV = marbles;
	//int marblesInRandomFV = (int)((1 - assort) * marbles + 0.5);
	int marblesInRiskFV = marbles;
	//int marblesInRiskFV = marbles - marblesInRandomFV;

	if(_person->getRiskLevel() == Person::HIGH)
	{
		FVProbDist_high.add(_person, marblesInRiskFV);
	}
	else
	{
		FVProbDist_low.add(_person, marblesInRiskFV);
	}

	FVProbDist_random.add(_person, marblesInRandomFV);

	//Add person to infected/uninfected list (as appropriate) for size purposes
	if(_person->isInfected())
	{
		assert(_person->getGenerationOfInfection() >= 0);
		FVinfected[_person->getGenerationOfInfection()]->add(_person, 1);
	}
	else
	{
		FVuninfected.add(_person, 1);
	}

	//Also add single copy to linked list for iterating
	FVNoDist.add(_person, 1);
	//Increment number of persons and number of infected person (if necessary)
	numPersons++;
	numRisk[_person->getRiskLevel()]++;
	numRiskHIVStatus[_person->getRiskLevel()][_person->getHIVStatus()]++;

    if(_person->getDemographicProfileVal(DemographicProfile::Demographic::Employment) == (std::size_t)DemographicProfile::Employment::Csw)
	{
		numRiskCSW[_person->getRiskLevel()]++;
	}

	if(_person->isInfected())
	{
		numInfected++;
		numInfectedRisk[_person->getRiskLevel()]++;
	}

	return true;
}

//------------< End Insertion and Retrieval Methods >--------------//
//------------------< Start Iteration Methods >--------------------//
/* @function: begin
 * @returns: An iterator of LLNoDist: the FullVector of person's with
 * exactly one copy of each person in the Bucket
 */
vector<Person *>::iterator BucketAge::begin()
{
	return FVNoDist.begin();
}

/* @function: end
 * @returns: An iterator of LLNoDist: the FullVector of person's with
 * exactly one copy of each person in the Bucket
 */
vector<Person *>::iterator BucketAge::end()
{
	return FVNoDist.end();
}

//-------------------< End Iteration Methods >---------------------//
//-----------------< Start Getters and Setters >-------------------//

DemographicProfile::ProfileID BucketAge::getBinID()
{
	return currentBinID;
}

/* @function: size
 * @returns: The integer number of unique Persons in the bucket
 */
unsigned long BucketAge::size()
{
	return FVNoDist.size();
}

/* @function: getNumRisk
 * @returns: The integer number of unique Persons in the bucket with given risk
 */
unsigned long BucketAge::getNumRisk(Person::RiskLevel _risk)
{
	return numRisk[_risk];
}

/* @function: getNumRiskCSW
 * @returns: The integer number of unique Persons in the bucket with given risk that is CSW
 */
unsigned long BucketAge::getNumRiskCSW(Person::RiskLevel _risk)
{
	return numRiskCSW[_risk];
}

/* @function: getNumRiskHIVStatus
 * @returns: The integer number of unique Persons in the bucket with given risk and HIV Status
 */
unsigned long BucketAge::getNumRiskHIVStatus(Person::RiskLevel _risk, Person::HIVStatus _hivStatus)
{
	return numRiskHIVStatus[_risk][_hivStatus];
}

/* @function: numHighRiskChoices
 * @returns: The integer number of (non-unique) Persons in the high risk bucket
 */
//TESTED
int BucketAge::numHighRiskChoices()
{
	return FVProbDist_high.size();
}

/* @function: numLowRiskChoices
 * @returns: The integer number of (non-unique) Persons in the low risk bucket
 */
//TESTED
int BucketAge::numLowRiskChoices()
{
	return FVProbDist_low.size();
}

/* @function: numRandomRiskChoices
 * @returns: The integer number of (non-unique) Persons in the random risk bucket
 */
//TESTED
int BucketAge::numRandomRiskChoices()
{
	return FVProbDist_random.size();
}

/* @function: numChoices
 * @returns: The integer number of (non-unique) Persons in the risk bucket associated with _risk
 * If _risk = Person::ENDRiskLevel, returns the number of persons in the random risk bucket
 */

int BucketAge::numChoices(Person::RiskLevel _risk)
{
	if(_risk == Person::HIGH)
	{
		return numHighRiskChoices();
	}
	else if(_risk == Person::LOW)
	{
		return numLowRiskChoices();
	}
	else if(_risk == Person::ENDRiskLevel)
	{
		return numRandomRiskChoices();
	}
	else
	{
		return -1;
	}
}

/* @function: increaseInfected
 * @effects: if person is in this BucketAge and is infected, increases the tally of numInfected
 * @returns: true if numInfected was increased
 */

bool BucketAge::increaseInfected(Person *_p)
{
	if(_p->isInfected())
	{
		if(FVuninfected.exists(_p))
		{
			numInfected++;
			numInfectedRisk[_p->getRiskLevel()]++;
			FVuninfected.remove(_p);
			FVinfected[_p->getGenerationOfInfection()]->add(_p, 1);
			return true;
		}
		else
		{
			//This means (should mean?) person is either not in this or is already counted as infected
			return false;
		}
	}
	else
	{
		return false;
	}
}

/* @function: changeHIVstatus
 * @effects: if person is in this BucketAge and thier hiv status changes decrement the old status and increment new status
 */
void BucketAge::changeHIVStatus(Person *_p, Person::HIVStatus _orig, Person::HIVStatus _new)
{
	numRiskHIVStatus[_p->getRiskLevel()][_orig]--;
	numRiskHIVStatus[_p->getRiskLevel()][_new]++;
}

//Pseudo-TESTED... should use print function later on
void BucketAge::printAll(std::ostream &_outStream, const std::string &_prefix)
{
	_outStream << _prefix << std::endl;
	_outStream << "Infected:	";

	for(int i = 0; i < NUMBER_GENERATIONS_TO_TRACE; i++)
	{
		FVinfected[i]->print();
	}

	_outStream << "Uninfected: 	";
	FVuninfected.print();
	_outStream << "Random Risk: ";
	FVProbDist_random.print();
	_outStream << "Low Risk:    ";
	FVProbDist_low.print();
	_outStream << "High Risk:   ";
	FVProbDist_high.print();
}
