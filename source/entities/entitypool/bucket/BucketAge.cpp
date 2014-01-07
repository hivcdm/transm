/*
 * BucketAge.cpp
 *
 *  Created on: Nov 20, 2008
 *      Author: errhode
 */

#include "BucketAge.h"

	//Constructor
BucketAge::BucketAge(){
	this->numPersons = 0;
	this->numInfected = 0;
	for (int i = 0; i < Person::ENDRiskLevel; i++){
		this->numRisk[i] = 0;
		this->numRiskCSW[i] = 0;
		this->numInfectedRisk[i] = 0;
		for (int j = 0; j < Person::ENDHIVStatus; j++){
			this->numRiskHIVStatus[i][j] = 0;
		}
	}
	for (int i = 0; i < SexualPartnership::ENDType; i++)
		this->assort[i] = 0;
}

BucketAge::BucketAge(DmgProfile::ProfileID BinID, unsigned int popID, const double _assort[]){
	this->currentBinID = BinID;
	this->populationID = popID;
	this->numPersons = 0;
	this->numInfected = 0;
	for (int i = 0; i < Person::ENDRiskLevel; i++){
		this->numRisk[i] = 0;
		this->numRiskCSW[i] = 0;
		this->numInfectedRisk[i] = 0;
		for (int j = 0; j < Person::ENDHIVStatus; j++){
			this->numRiskHIVStatus[i][j] = 0;
		}
	}
	for (int i = 0; i < SexualPartnership::ENDType; i++)
		this->assort[i] = _assort[i];
	//Initialize the infected FVs
	for (int i = 0; i < this->NUMBER_GENERATIONS_TO_TRACE; i++){
		FullVector* emptyFV = new FullVector();
		FVinfected.push_back(emptyFV);
	}
}

	//Destructor
	BucketAge::~BucketAge(){
		for (int i = 0; i < this->NUMBER_GENERATIONS_TO_TRACE; i++){
			this->FVinfected[i]->clear();
			delete this->FVinfected[i];
		}
		FVinfected.clear();
		this->FVinfected.clear();
		this->FVuninfected.clear();
		this->FVProbDist_low.clear();
		this->FVProbDist_high.clear();
		this->FVProbDist_random.clear();
		this->FVNoDist.clear();
	}

	//------------< Start Methods taken from EntityIndex >-------------//
	//clears all elements from this index
	void BucketAge::clear(){
		for (int i = 0; i < this->NUMBER_GENERATIONS_TO_TRACE; i++){
			assert(i < 6);
			this->FVinfected[i]->clear();
		}
		this->FVuninfected.clear();
		this->FVProbDist_high.clear();
		this->FVProbDist_low.clear();
		this->FVProbDist_random.clear();
		this->FVNoDist.clear();
		this->numPersons = 0;
		this->numInfected = 0;
		for (int i = 0; i < Person::ENDRiskLevel; i++){
			this->numRisk[i] = 0;
			this->numRiskCSW[i] = 0;
			this->numInfectedRisk[i] = 0;
			for (int j = 0; j < Person::ENDHIVStatus; j++){
				this->numRiskHIVStatus[i][j] = 0;
			}
		}
	}

	//tells whether _person exists in the index
	bool BucketAge::exists(Person* p){
		return (this->FVNoDist.exists(p));
	}

	//will return how many HIV infected people are currently in the index
	//Store as a number?  No -- Not costing significant time
	unsigned long BucketAge::getNumInfected(){
		unsigned long total = 0;
		for (int i = 0; i < this->NUMBER_GENERATIONS_TO_TRACE; i++){
			total += this->FVinfected[i]->size();
		}
		return total;
	}

	unsigned long BucketAge::getNumInfected(int generation){
		return this->FVinfected[generation]->size();
	}
	
	unsigned long BucketAge::getNumInfected(Person::RiskLevel _risk){
		return this->numInfectedRisk[_risk];
	}

	//prints every person in this index to _outStream
	void BucketAge::print(ostream& _outStream, std::string _prefix){
		vector<Person*>::iterator PersonIter = this->begin();
		while (PersonIter != this->end()){
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
	Person *BucketAge::drawMember(RandomNums& _randomNums, Person::RiskLevel _riskLevel, SexualPartnership::Type /*_partnershipType*/, bool _use_random, bool _remove)
	{
		if (this->numPersons == 0)
		{
			return NULL;
		}
		FullVector *toDrawFrom;
		if (_use_random){
			toDrawFrom = &(this->FVProbDist_random);
		}
		else if (_riskLevel == Person::LOW){
			toDrawFrom = &(this->FVProbDist_low);
		}
		else if (_riskLevel == Person::HIGH){
			toDrawFrom = &(this->FVProbDist_high);
		}
		else{
			cerr << "Error: attempting to draw a member from invalid risk level: " << _riskLevel << endl;
			return NULL;
		}

		if (toDrawFrom->size() > 0){
			int toPick = _randomNums.randInt(0, toDrawFrom->size() - 1);
			if (_remove){
				//Remove person from all FV
				Person* personToReturn = toDrawFrom->selectout(toPick);
				this->erase(personToReturn);
				return personToReturn;
			}
			else{
				return toDrawFrom->at(toPick);
			}
		}

		else{
			cerr << "Error: requesting person from empty set: " << DmgProfile::toString(this->currentBinID) << endl;
			return NULL;
		}
	}

	/* @function: erase
	 * @effects: removes _person from this by removing _person from all FVs; decrements numPersons by 1;
	 * if _person is an infected individual, decrements numInfected by 1
	 * @returns: true if _person was previously a member of this and was successfully removed, false is
	 * _person was not a member of this
	 */
	bool BucketAge::erase(Person *_person){
		if (this->exists(_person)){
			//Remove from all FVs
			bool removed[6];
			removed[0] = this->FVProbDist_high.remove(_person);
			removed[1] = this->FVProbDist_low.remove(_person);
			removed[2] = this->FVProbDist_random.remove(_person);
			removed[3] = false;
			for (int i = 0; i < this->NUMBER_GENERATIONS_TO_TRACE; i++){
				bool removeInf = this->FVinfected[i]->remove(_person);
				removed[3] = (removed[3] || removeInf);
			}
			removed[4] = this->FVuninfected.remove(_person);
			assert((removed[0] || removed[1] || removed[2]) && (removed[3] || removed[4]));

			//Remove from linked list
			this->FVNoDist.remove(_person);

			//If infected, reduce count of numInfected
			if (_person->isInfected()){
				this->numInfected--;
				this->numInfectedRisk[_person->getRiskLevel()]--;
			}
			
			//Reduce count of number of people
			this->numPersons--;
			this->numRisk[_person->getRiskLevel()]--;
			this->numRiskHIVStatus[_person->getRiskLevel()][_person->getHIVStatus()]--;
			if (DmgProfile::get(_person->getCurrBucketProfileID(),DmgProfile::EMPLOYMENT) == DmgProfile::CSW){
				this->numRiskCSW[_person->getRiskLevel()]--;
			}
			return ((removed[0] || removed[1] || removed[2]) && (removed[3] || removed[4]));
		}
		else{
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
	bool BucketAge::insert(Person *_person){
		if (this->exists(_person)){
			cerr << "Adding person to a bucket they are already in!" << endl;
			return false;
		}

		int marbles = _person->getSexualActivity();

		//Using Mark Lipsitch's sexual mixing algorithm based on the assortativeness value
		//Update: Mark says this is double counting the assortativeness!
		//Just put the same amount of marbles in each box
		//Update again: Mathematically proved that the two methods are the same... putting the same number of marbles in each box has less potential for bugs`
		int marblesInRandomFV = marbles;
		//int marblesInRandomFV = (int)((1 - this->assort) * marbles + 0.5);
		int marblesInRiskFV = marbles;
		//int marblesInRiskFV = marbles - marblesInRandomFV;

		if (_person->getRiskLevel() == Person::HIGH){
			this->FVProbDist_high.add(_person, marblesInRiskFV);
		}
		else{
			this->FVProbDist_low.add(_person, marblesInRiskFV);
		}

		this->FVProbDist_random.add(_person, marblesInRandomFV);

		//Add person to infected/uninfected list (as appropriate) for size purposes
		if (_person->isInfected()){
			assert(_person->getGenerationOfInfection() >= 0);
			this->FVinfected[_person->getGenerationOfInfection()]->add(_person, 1);
		}
		else{
			this->FVuninfected.add(_person, 1);
		}
		//Also add single copy to linked list for iterating
		this->FVNoDist.add(_person, 1);

		//Increment number of persons and number of infected person (if necessary)
		this->numPersons++;
		this->numRisk[_person->getRiskLevel()]++;
		this->numRiskHIVStatus[_person->getRiskLevel()][_person->getHIVStatus()]++;
		if (_person->getDmgProfileVal(DmgProfile::EMPLOYMENT) == DmgProfile::CSW){
			this->numRiskCSW[_person->getRiskLevel()]++;
		}
		if (_person->isInfected()){
			this->numInfected++;
			this->numInfectedRisk[_person->getRiskLevel()]++;
		}

		return true;
	}

	//------------< End Insertion and Retrieval Methods >--------------//
	//------------------< Start Iteration Methods >--------------------//
	/* @function: begin
	 * @returns: An iterator of LLNoDist: the FullVector of person's with
	 * exactly one copy of each person in the Bucket
	 */
	vector<Person*>::iterator BucketAge::begin(){
		return this->FVNoDist.begin();
	}

	/* @function: end
	 * @returns: An iterator of LLNoDist: the FullVector of person's with
	 * exactly one copy of each person in the Bucket
	 */
	vector<Person*>::iterator BucketAge::end(){
		return this->FVNoDist.end();
	}

	//-------------------< End Iteration Methods >---------------------//
	//-----------------< Start Getters and Setters >-------------------//

	DmgProfile::ProfileID BucketAge::getBinID(){
		return this->currentBinID;
	}

	/* @function: size
	 * @returns: The integer number of unique Persons in the bucket
	 */
	unsigned long BucketAge::size(){
		return this->FVNoDist.size();
	}

	/* @function: getNumRisk
	 * @returns: The integer number of unique Persons in the bucket with given risk 
	 */
	unsigned long BucketAge::getNumRisk(Person::RiskLevel _risk){
		return this->numRisk[_risk];
	}

	/* @function: getNumRiskCSW
	 * @returns: The integer number of unique Persons in the bucket with given risk that is CSW
	 */
	unsigned long BucketAge::getNumRiskCSW(Person::RiskLevel _risk){
		return this->numRiskCSW[_risk];
	}

	/* @function: getNumRiskHIVStatus
	 * @returns: The integer number of unique Persons in the bucket with given risk and HIV Status
	 */
	unsigned long BucketAge::getNumRiskHIVStatus(Person::RiskLevel _risk, Person::HIVStatus _hivStatus){
		return this->numRiskHIVStatus[_risk][_hivStatus];
	}

	/* @function: numHighRiskChoices
	 * @returns: The integer number of (non-unique) Persons in the high risk bucket
	 */
	//TESTED
	int BucketAge::numHighRiskChoices(){
		return this->FVProbDist_high.size();
	}

	/* @function: numLowRiskChoices
	 * @returns: The integer number of (non-unique) Persons in the low risk bucket
	 */
	//TESTED
	int BucketAge::numLowRiskChoices(){
		return this->FVProbDist_low.size();
	}

	/* @function: numRandomRiskChoices
	 * @returns: The integer number of (non-unique) Persons in the random risk bucket
	 */
	//TESTED
	int BucketAge::numRandomRiskChoices(){
		return this->FVProbDist_random.size();
	}

	/* @function: numChoices
	 * @returns: The integer number of (non-unique) Persons in the risk bucket associated with _risk
	 * If _risk = Person::ENDRiskLevel, returns the number of persons in the random risk bucket
	 */

	int BucketAge::numChoices(Person::RiskLevel _risk){
		if (_risk == Person::HIGH){
			return this->numHighRiskChoices();
		}
		else if (_risk == Person::LOW){
			return this->numLowRiskChoices();
		}
		else if (_risk == Person::ENDRiskLevel){
			return this->numRandomRiskChoices();
		}
		else {
			return -1;
		}
	}

	/* @function: increaseInfected
	 * @effects: if person is in this BucketAge and is infected, increases the tally of numInfected
	 * @returns: true if numInfected was increased
	 */

	bool BucketAge::increaseInfected(Person *_p){
		if (_p->isInfected()){
			if (this->FVuninfected.exists(_p)){
				this->numInfected++;
				this->numInfectedRisk[_p->getRiskLevel()]++;
				this->FVuninfected.remove(_p);
				this->FVinfected[_p->getGenerationOfInfection()]->add(_p, 1);
				return true;
			}
			else{
			//This means (should mean?) person is either not in this or is already counted as infected
			return false;
			}
		}
		else{
			return false;
		}
	}

	/* @function: changeHIVstatus
	 * @effects: if person is in this BucketAge and thier hiv status changes decrement the old status and increment new status
	 */
	void BucketAge::changeHIVStatus(Person *_p, Person::HIVStatus _orig, Person::HIVStatus _new){
		this->numRiskHIVStatus[_p->getRiskLevel()][_orig]--;
		this->numRiskHIVStatus[_p->getRiskLevel()][_new]++;
	}

	//Pseudo-TESTED... should use print function later on
	void BucketAge::printAll(ostream& _outStream, string _prefix){
		_outStream << _prefix << endl;
		_outStream << "Infected:	";
		for (int i = 0; i < this->NUMBER_GENERATIONS_TO_TRACE; i++)
			this->FVinfected[i]->print();
		_outStream << "Uninfected: 	";
		this->FVuninfected.print();
		_outStream << "Random Risk: ";
		this->FVProbDist_random.print();
		_outStream << "Low Risk:    ";
		this->FVProbDist_low.print();
		_outStream << "High Risk:   ";
		this->FVProbDist_high.print();
	}
