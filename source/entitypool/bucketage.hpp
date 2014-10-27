#pragma once

#include <vector>

#include "fullvector.hpp"
#include "entities/demographicprofile.hpp"

namespace transm {

class BucketAge
{

	//If this number gets changed, also change it in InfectionsTracker.h
	static const int NUMBER_GENERATIONS_TO_TRACE = 6;

public:
	//Constructor
	BucketAge();
    BucketAge(DemographicProfile::ProfileID BinID, unsigned int popID, const std::map<SexualPartnership::Type, double> &_assort);

	//Destructor
	~BucketAge();

	//clears all elements from this index
	void clear();

	//tells whether _person exists in the index
	//Use person's internal index to help verify
	bool exists(Entity *p);

	//will return how many HIV infected people are currently in the index
	//Store as a number?
	unsigned long getNumInfected();

	unsigned long getNumInfected(int generation);
	unsigned long getNumInfected(Entity::RiskLevel _risk);

	//prints every person in this index to _outStream
    void print(ostream &_outStream, const std::string &_prefix);

	/*
	 * Uses the random number generator (_randomNums) to randomly draw a person
	 * -- Either draws from the FV related to _riskLevel or the random FV if _use_random == true
	 * -- Removes returned person from this is _remove == true
	 */
	Entity *drawMember(RandomNumberGenerator &_randomNums, Entity::RiskLevel _riskLevel, SexualPartnership::Type _partnershipType,
	                   bool _use_random, bool _remove);

	/* @function: erase
	 * @effects: removes _person from this by removing _person from all FVs; decrements numPersons by 1;
	 * if _person is an infected individual, decrements numInfected by 1
	 * @returns: true if _person was previously a member of this and was successfully removed, false is
	 * _person was not a member of this
	 */
	bool erase(Entity *_person);
	bool erase(Entity *_person, bool print);


	//will index a new person
	/* @function: insert
	 * @effects: adds _person to this by adding _person to appropriate FV: FVNoDist (every _person),
	 * and FVProbDist_random and either FVProbDist_high or FVProbDist_low depending on _person.risk and
	 * assort
	 * @returns: true if person was successfully added, false otherwise
	 */
	bool insert(Entity *_person);

	/* @function: begin
	 * @returns: An iterator of LLNoDist: the linked list of persons with
	 * exactly one copy of each person in the Bucket
	 */
	std::vector<Entity *>::iterator begin();

	/* @function: end
	 * @returns: An iterator of LLNoDist: the linked list of persons with
	 * exactly one copy of each person in the Bucket
	 */
	std::vector<Entity *>::iterator end();

	DemographicProfile::ProfileID getBinID();

	/* @function: size
	 * @returns: The integer number of unique Persons in the bucket
	 */
	unsigned long size();

	/* @function: getNumRisk
	 * @returns: The integer number of unique Persons in the bucket with a given risk
	 */
	unsigned long getNumRisk(Entity::RiskLevel _risk);

	/* @function: getNumRiskCSW
	 * @returns: The integer number of unique Persons in the bucket with a given risk that is CSW
	 */
	unsigned long getNumRiskCSW(Entity::RiskLevel _risk);

	/* @function: getNumRiskHIVStatus
	 * @returns: The integer number of unique Persons in the bucket with a given risk and hivStatus
	 */
	unsigned long getNumRiskHIVStatus(Entity::RiskLevel _risk, Entity::HIVStatus _hivStatus);

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
	 * If _risk = (std::size_t)Entity::RiskLevel::Last, returns the number of persons in the random risk bucket
	 */

	int numChoices(Entity::RiskLevel _risk);

	/* @function: increaseInfected
	 * @effects: if person is in this BucketAge and is infected, increases the tally of numInfected
	 * @returns: true if numInfected was increased
	 */

	bool increaseInfected(Entity *_p);

	/* @function: changeHIVstatus
	 * @effects: if person is in this BucketAge and thier hiv status changes decrement the old status and increment new status
	 */
	void changeHIVStatus(Entity *_p, Entity::HIVStatus _orig, Entity::HIVStatus _new);

    void printAll(ostream &_outStream, const std::string &_prefix);

private:
	//FVinfected and FVuninfected keep track of number of persons and number of uninfected vs. infected persons by generation of infection
	std::vector<FullVector *> FVinfected;
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
    std::map<SexualPartnership::Type, double> assort;

	bool UpdateNeeded;
	DemographicProfile::ProfileID currentBinID;
	unsigned int populationID;
	unsigned long numPersons;
	unsigned long numInfected;
	unsigned long numRisk[(std::size_t)Entity::RiskLevel::Last];
	unsigned long numRiskCSW[(std::size_t)Entity::RiskLevel::Last]; // number of csw persons by risk bucket
	unsigned long numInfectedRisk[(std::size_t)Entity::RiskLevel::Last];
	unsigned long numRiskHIVStatus[(std::size_t)Entity::RiskLevel::Last][(std::size_t)Entity::HIVStatus::Last];
};

} // namespace transm
