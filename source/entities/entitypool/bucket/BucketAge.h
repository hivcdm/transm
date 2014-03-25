/*
 * BucketAge.h
 *
 *  Created on: Nov 13, 2008
 *      Author: errhode
 */
#pragma once

#include <vector>

#include "FullVector.h"
#include "../../classifiers/DmgProfile.h"

class BucketAge
{

	//If this number gets changed, also change it in InfectionsTracker.h
	static const int NUMBER_GENERATIONS_TO_TRACE = 6;

public:
	//Constructor
	BucketAge();
	BucketAge(DmgProfile::ProfileID BinID, unsigned int popID, const double _assort[]);

	//Destructor
	~BucketAge();


	//------------< Start Methods taken from EntityIndex >-------------//
	//clears all elements from this index
	void clear();

	//tells whether _person exists in the index
	//Use person's internal index to help verify
	bool exists(Person *p);

	//will return how many HIV infected people are currently in the index
	//Store as a number?
	unsigned long getNumInfected();

	unsigned long getNumInfected(int generation);
	unsigned long getNumInfected(Person::RiskLevel _risk);

	//prints every person in this index to _outStream
	void print(ostream &_outStream, std::string _prefix);

	//-------------< End Methods taken from EntityIndex >--------------//


	//-----------< Start Insertion and Retrieval Methods >-------------//
	/*
	 * Uses the random number generator (_randomNums) to randomly draw a person
	 * -- Either draws from the FV related to _riskLevel or the random FV if _use_random == true
	 * -- Removes returned person from this is _remove == true
	 */
	Person *drawMember(RandomNums &_randomNums, Person::RiskLevel _riskLevel, SexualPartnership::Type _partnershipType,
	                   bool _use_random, bool _remove);
	//draw a member from this pool
	//Person* drawMember(int _randomNums, Person *_chooser, int _partnershipType, bool _remove);

	//draws person at position _randomAccessIndex in this index. This is random access...slow but necessary
	//Person* getMember(unsigned long _randomAccessIndex, bool _remove);

	/* @function: erase
	 * @effects: removes _person from this by removing _person from all FVs; decrements numPersons by 1;
	 * if _person is an infected individual, decrements numInfected by 1
	 * @returns: true if _person was previously a member of this and was successfully removed, false is
	 * _person was not a member of this
	 */
	bool erase(Person *_person);
	bool erase(Person *_person, bool print);


	//will index a new person
	/* @function: insert
	 * @effects: adds _person to this by adding _person to appropriate FV: FVNoDist (every _person),
	 * and FVProbDist_random and either FVProbDist_high or FVProbDist_low depending on _person.risk and
	 * assort
	 * @returns: true if person was successfully added, false otherwise
	 */
	bool insert(Person *_person);
	//------------< End Insertion and Retrieval Methods >--------------//
	//------------------< Start Iteration Methods >--------------------//

	/* @function: begin
	 * @returns: An iterator of LLNoDist: the linked list of persons with
	 * exactly one copy of each person in the Bucket
	 */
	vector<Person *>::iterator begin();

	/* @function: end
	 * @returns: An iterator of LLNoDist: the linked list of persons with
	 * exactly one copy of each person in the Bucket
	 */
	vector<Person *>::iterator end();

	//-------------------< End Iteration Methods >---------------------//
	//-----------------< Start Getters and Setters >-------------------//

	DmgProfile::ProfileID getBinID();

	/* @function: size
	 * @returns: The integer number of unique Persons in the bucket
	 */
	unsigned long size();

	/* @function: getNumRisk
	 * @returns: The integer number of unique Persons in the bucket with a given risk
	 */
	unsigned long getNumRisk(Person::RiskLevel _risk);

	/* @function: getNumRiskCSW
	 * @returns: The integer number of unique Persons in the bucket with a given risk that is CSW
	 */
	unsigned long getNumRiskCSW(Person::RiskLevel _risk);

	/* @function: getNumRiskHIVStatus
	 * @returns: The integer number of unique Persons in the bucket with a given risk and hivStatus
	 */
	unsigned long getNumRiskHIVStatus(Person::RiskLevel _risk, Person::HIVStatus _hivStatus);

	/* @function: numHighRiskChoices
	 * @returns: The integer number of (non-unique) Persons in the high risk bucket
	 */

	int numHighRiskChoices();

	/* @function: numLowRiskChoices
	 * @returns: The integer number of (non-unique) Persons in the low risk bucket
	 */

	int numLowRiskChoices();

	/* @function: numRandomRiskChoices
	 * @returns: The integer number of (non-unique) Persons in the random risk bucket
	 */

	int numRandomRiskChoices();

	/* @function: numChoices
	 * @returns: The integer number of (non-unique) Persons in the risk bucket associated with _risk
	 * If _risk = Person::ENDRiskLevel, returns the number of persons in the random risk bucket
	 */

	int numChoices(Person::RiskLevel _risk);

	/* @function: increaseInfected
	 * @effects: if person is in this BucketAge and is infected, increases the tally of numInfected
	 * @returns: true if numInfected was increased
	 */

	bool increaseInfected(Person *_p);

	/* @function: changeHIVstatus
	 * @effects: if person is in this BucketAge and thier hiv status changes decrement the old status and increment new status
	 */
	void changeHIVStatus(Person *_p, Person::HIVStatus _orig, Person::HIVStatus _new);

	void printAll(ostream &_outStream, string _prefix);


	//------------------< End Getters and Setters >--------------------//


private:
	//FVinfected and FVuninfected keep track of number of persons and number of uninfected vs. infected persons by generation of infection
	vector<FullVector *> FVinfected;
	FullVector FVuninfected;
	//FV with no probability distribution: 1 copy of each person
	//Use this for iterator functions
	FullVector FVNoDist;

	//FullVector with probability distribution for low risk persons
	FullVector FVProbDist_low;

	//FullVector with probability distribution for high risk persons
	FullVector FVProbDist_high;

	//FullVector with probability distribution for random risk (all) persons
	FullVector FVProbDist_random;

	//Assortativeness parameter... default to 0 (all chosen from FVProbDist_random)
	double assort[SexualPartnership::Type::ENDType];

	bool UpdateNeeded;
	DmgProfile::ProfileID currentBinID;
	unsigned int populationID;
	unsigned long numPersons;
	unsigned long numInfected;
	unsigned long numRisk[Person::ENDRiskLevel];
	unsigned long numRiskCSW[Person::ENDRiskLevel]; // number of csw persons by risk bucket
	unsigned long numInfectedRisk[Person::ENDRiskLevel];
	unsigned long numRiskHIVStatus[Person::ENDRiskLevel][Person::ENDHIVStatus];
};
