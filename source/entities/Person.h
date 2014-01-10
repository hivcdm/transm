#pragma once

#include <list>
#include "../statistics/StatsRecord.h"
#include "./classifiers/DmgProfile.h"
#include "./classifiers/SexualPartnership.h"
#include "./../Constants.h"
#include "../util/Util.h"
#include "./entitypool/bucket/FullVector.h"
#include <iostream>
#include <vector>
#include <set>
#include "../cepac/Patient.h"
#include "../graphviz/graphVizParse.h"

class ArtRolloutTracker;
class EntityPool;
class EventParams;
class FullVector;
class InfectionsTracker;
class RandomNums;

enum TimeGranularity;

/**
All individuals in the simulation are of this class, or something derived from this

Fields and Methods are divided into the following categories:
	Physical, Relational, DmgProfile-related, other

@author schung5
***/
class Person {

	//used to create unique id's for each person
	//this increments every time a New person is created
	static long idCounter;

public:

	//a simple linked list of SexualPartnerships
	//typedef list<SexualPartnership*> PartnerList;

	//-------------------< Start static fields / enums >-----------------------//

	//every Person's CD4 count falls in a CD4 strata - used in CEPAC
	enum CD4Strata {
		CD4_ZERO,
		CD4_ONE,
		CD4_TWO,
		CD4_THREE,
		CD4_FOUR,
		CD4_FIVE,
		ENDCD4Strata
	};

	//every Person's hvl level falls in an HVL strata
	enum HVLStrata {		//copies/ml
		UNINFECTED = -1,
		HVL_ZERO,		//0-20
		HVL_ONE,		//21-500
		HVL_TWO,		//501-3000
		HVL_THREE,		//3001-10000
		HVL_FOUR,		//10001-30000
		HVL_FIVE,		//30001-100000
		HVL_SIX,		//100000+
		HVL_PRIMARY,
		HVL_LATESTAGE,
		ENDHVLStrata
	};

	enum HIVStatus{
		NEGATIVE, //hiv negative
		OBSERVED_ACUTE,
		UNOBSERVED_ACUTE,
		OBSERVED_CHRONIC,
		UNOBSERVED_CHRONIC,
		OBSERVED_LATESTAGE,//Late stage takes precedence over chronic (acute cases are never latestage)
		UNOBSERVED_LATESTAGE,
		ENDHIVStatus
	};

	enum DeathStatus{
		ALIVE, //not dead
		DTH_OI,
		DTH_CHRAIDS,
		DTH_NONAIDS,
		DTH_TOX_ART,
		DTH_TOX_PROPH,
		DTH_OTHER,
		ENDDeathStatus
	};

	enum RiskLevel { //used for assortativeness
		LOW,
		HIGH,
		ENDRiskLevel
	};

	// we have made these stats referenceable by enum so that we can more easily create customizeable outputs or reports...
	// we can perhaps have easier look-up of stat descriptions if we choose to write some up
	enum Stats {
		STAT_TOTAL_LM,							//months lived during sim.
		STAT_HIV_NEG_LM,						//life months lived as HIV-
		STAT_HIV_POS_POSTINFECT_LM,				//life months lived after HIV infection
		STAT_EXPOSURES_BEFORE_INF,				//# times exposed to HIV but not infected
		STAT_NUM_INFECTED,						//number of people that someone infected
		STAT_AGE_AT_INFECTION_MTH,				//age when infection occurred
		STAT_TIME_OF_INFECTION_MTH,				//calendar time of infection
		STAT_GENERATION_OF_INFECTION,			//generation of infection: prevalent case is 0, otherwise (1+generation of infectors infection)
		STAT_ENDStats
	};
	//string representations of enum Stats
	static const std::string StatsStr[STAT_ENDStats];
	//this is a enum class wrapper that has helpful enum-related functions
	static EnumCls<Stats> StatsEnum;
	//this is a type declaration of a class that keeps track of statistics defined in enum Stats
	typedef ::StatsRecord<Stats, BaseEnumCls::NULL_ENUM> StatsRecord;


	//the CEPAC death table has stats for 0-100 years old.
	//  people automatically die at this age in the dynamic model
	const static int maxYrForDeathStats = 101;

	//contains probabilities of nonAIDS-death, read from CEPAC .in file
	static std::vector<double>probDeathNatCauses[DmgProfile::ENDGender];

	//there is option to print patient traces to a text file. this keeps track of how many we've done so far
	static int numTracesSoFar;

	//-------------------< End static fields / enums >-----------------------//

protected:

	//-------------------< Start Data Fields >-----------------------//

	//identifying information
	unsigned int populationID;			//keeps track of which population this Person belongs to
	unsigned long id;					//person's unique id number

	//person's current demographic profile - values in here depend on person's physical, relational state, and other preferences
	DmgProfile dmgProfile;
	//person keeps track of which DmgProfileBucket they are currently in
	//  this value should stay equal to dmgProfile->getProfileID()
	//  sometimes a person's dmgProfile is changed, so we have to refresh their place in the EntityPool
	DmgProfile::ProfileID currentBucketID;

	//Person's relational state
	//contains all current partnerships including CSW and Casual
	list<SexualPartnership*> partners[SexualPartnership::ENDType];

	//array of number of partners over persons history stratified by partnership type
	int numPartnersInHistory[SexualPartnership::ENDType];

	//array of month of their farthest current partnership dissolution time for each partnership type.  initialized to zero
	int monthOfLatestPartnershipDissolution[SexualPartnership::ENDType];

	//array of month of latest concurrent relationship for each partnership type. Only updated for 12 months before calibration
	int monthOfLatestConcurrent;

	// Contains the number of partnerships the person tried to form over time, but didn't
	// (usually due to no partners available or re-hooking up with a current partner)
	int unformedPartnershipsTotal[SexualPartnership::ENDType];
	int unformedPartnershipsLatestTime[SexualPartnership::ENDType];



	//TODO: What is this and is it still used?  It doesn't look like it.
	double sexualActivityLevel;			//relative risky behavior level
	int generationOfInfection;			//Which generation was the person infected in
	bool wentThroughCEPAC;				//flag to indicate whether this person has CEPAC data
	//MonthEvent *healthAfterInfection;	//this contains monthly health states as generated by CEPAC
	Patient *cepacPatient;				//a pointer to a CEPAC patient object which stores all post-infection health states
	double CEPACcosts;					//A running tally of all costs accrued through CEPAC

	/** The graph node used for printing out the final graphs */
	GraphVizGraphElements::personNode *graphNode;

	//Used to keep track of number of acts this month
	int numActsThisMonth;

	//Used to keep track of number of condoms used in a given month for cost purposes
	int condomsUsedThisMonth;
	//Used to keep track of whether or not a condom was used the last time getFOI was called
	bool condomUsedLastFOICalculation;


	//currently defaults to "LOW" and 1
	RiskLevel risk;
	int activityLevel; //sexual activity level of person -- relates to number of "marbles" in selection pool

	//statistical information from this individual
	StatsRecord stats;

	//True if this person should be followed in singlePersonTrace file
	bool traceMe;

	//The indices which point to the person in their assigned FullVector
	map<FullVector*, vector<unsigned int> > FVindices;  //The indices of the the person in their assigned FullVector


	//-------------------< End Data Fields >-----------------------//

public:

	//dummy constructor
	Person();

	//this constructor creates an actual person that can be simulated. It is generally called by Male and Female
	// we pass in _eventParams because becomeInfected() needs it...
	Person(EventParams &_eventParams, int _age, //bool _infected,
			unsigned int _populationID);

	~Person(void);

	//Person's physical state
	unsigned int age;					//age of Person (in months)
	unsigned int initAge;				//age of Person on model init (in months)
	int ageInfected;						//age of Person when they got infected (-1 for uninfected)
	bool death;							//whether this person is dead or not
	HIVStatus hivStatus;				//Person's infected status
	double cd4;							//CD4 cell count
	HVLStrata hvl;						//HIV Viral Load
	HVLStrata currentTrueHvl;			//HIV Viral Load of person from patient object.  No primary or late stage stratas
	bool isObserved;					//Whether this person has observed HIV
	bool oiHistory[Constants::NUMBER_OF_OIS]; //OI HIstory
	DeathStatus deathStatus;

	CD4Strata getCd4Stratum();

	//This is for keeping dead people around for graph printing reasons
	//It mimics the destructor without destroying the Person object.
	void deletePersonWithoutDeleting();

	//---------------< Start Physical-state related methods >------------------------//

	void ageOneTimeUnit();

	/*
		call this to infect person...
		if CEPAC bridge is in place, will call CEPAC to determine the health trajectory of this person
		@params _prevalentInfection if true, than this person was a prevalent infection
	*/
	void becomeInfected(int _generationOfInfection, EventParams& _eventParams);

	/*
	 * Initializes this->cepacPatient using the persons current age, gender, and infection status.
	 * Prevalent cases should call "becomeInfected" before calling this function; incident cases will become infected later
	 */
	void initialCEPACpatient(EventParams& _eventParams);

	/**
	 * @return this->generationOfInfection
	 */

	int getGenerationOfInfection();

	/**
	*	returns the number of partners by partnership type
	*/
	int getNumPartners(SexualPartnership::Type);

	/**
	*	returns the number of partners by partnership type that are either the samerisk or different
	*/
	int getNumPartners(SexualPartnership::Type, bool);

	/**
	* returns the number of partners in history
	*/
	int getNumPartnersInHistory();

	/**
	* returns number of partners in history stratified by type
	*/
	int getNumPartnersInHistory(SexualPartnership::Type);

	/**
	* returns month of latest partnership dissolution (may be in the future) for given partner type
	*/
	int getMonthOfLatestPartnershipDissolution(SexualPartnership::Type);

	//gets and sets month of latest concurrent
	void setMonthOfLatestConcurrent(int);
	int getMonthOfLatestConcurrent();

	/**
	 * @return this->hvl
	 */
	HVLStrata getHVL() const { return this->hvl; }

	/** this calculates the FOI towards Person _p (this uses the Transmission coefficient) per event
	// @param _p - partner
	// @param _parteringType - whether this is a fling or steadyCouple */
	virtual double getFOI(Person *_p, SexualPartnership::Type _partnershipType, EventParams& _eventParams);

	/** get the transmission coefficient of the person... based on HVL */
	virtual double getTransmissionCoeff();

	//returns true if person is currently alive
	bool isAlive() const;

	//returns true if person is currently infected
	bool isInfected();

	/**
	//see whether person dies. If they went through CEPAC, use health trace. else roll against nonAIDS death probs
	**/
	bool rollForDeath(RandomNums &_randomNums);

	//update health status of HIV infected people -- i.e. cd4, hvl, art, etc.
	//  in version 1, this information is taken from CEPAC model
	// @returns: costs (accrued in CEPAC) of updating health
	double updateHealthStatus(EventParams& _eventParams, ArtRolloutTracker *testTracker);

	//Call this after all transmission/population dynamics are done.
	//Runs infected through CEPAC until they die and adds their LM etc to CEPAC stats
	void runCEPACtoDeath(RandomNums& _randomNums);

	//Sets the condom total to 0
	void resetCondomUsage();

	//Sets numacts to 0
	void resetNumActs();

	//Return the number of condoms used
	int getCondomsUsedThisMonth();

	//Return num acts this month
	int getNumActsThisMonth();

	//Increase condoms used this month by a given number (default 1)
	void incrementCondomsUsedThisMonth(int condoms = 1);

	//Increase num acts this month
	void incrementNumActsThisMonth(int _numActs);

	//Return true if a condom was used the last time FOI was called
	bool getCondomUsedLastFOICalculation();

	//---------------< END Physical-state related methods >------------------------//

	//-------------< Start FullVector indices related methods >----------------------//
	/* @function: setFVindices
	 * @arguments: vector<int> FVind, FullVector* FV
	 * @effects: if this.FVindices is currently empty and all indices correlate with members
	 * of FV that point to this, sets this.FVindices to FVind
	 * @return: true if this.FVindices was set to FVind or false otherwise
	 */
	bool setFVindices(vector<unsigned int> FVind, FullVector* FV);

	/* @function: addFVindices
	 * @arguments: int index, FullVector* FV
	 * @effects: If FV[index] points to this and index is not already a member of
	 * this.FVindices, adds index to this.FVindices
	 * @return: true if index was added to this.FVindices or false otherwise
	 */
	bool addFVindices(int index, FullVector* FV);

	/* @function: removeFVindices
	 * @arguments: int index, FullVector* FV
	 * @effects: If FV[index] does not point to this, removes index from this.FVindices
	 * @return: true if index was removed from this.FVindices, false otherwise
	 */
	bool removeFVindices(int index, FullVector* FV);

	/* @function: memberFVindices
	 * @arguments: int index, FullVector* FV
	 * @effects: none
	 * @return: true iff this.FVindices contains index
	 */
	bool memberFVindices(int index, FullVector* FV);

	/* @function: getFVindices
	 * @arguments: none
	 * @effects: none
	 * @return: copy of this.FVindices
	 */
	vector<unsigned int> getFVindices(FullVector* FV);

	//----------------< End FullVector related methods >-------------------------//

	//-------------< Start BucketAge related methods >----------------------//
	/* @function: getRiskLevel
	 * @return: this.risk
	 */
	const Person::RiskLevel getRiskLevel() const;

	/* @function: getHIVStatus
	 * @return: this.hivStatus
	 */
	const Person::HIVStatus getHIVStatus() const;

	/* @function: getSexualActivity
	 * @return: this.activityLevel
	 */
	int getSexualActivity();

	//---------------< Start DmgProfile related methods >------------------------//

	/*
	changes this person to sexually active
	and initializes the CEPAC person
	*/
	void becomeSexuallyActive(EventParams& _eventParams);

	//returns structure that holds current DemographicProfile
	const DmgProfile* getDmgProfile() const;
	BaseEnumCls::Enum getDmgProfileVal(DmgProfile::Demographic _demographic) const;

	//sets and gets current DmgProfileBucket membership
	DmgProfile::ProfileID getCurrBucketProfileID();
	void setCurrBucketProfileID(DmgProfile::ProfileID _profileID);

	//returns true if the person's DmgProfile matches their current DmgProfileBucket membership
	bool inCorrectDmgProfileBucket();

	/*
	changes isSexWorker with probability taken from population prevalence of CSW (or initial csw chance if prevalent population)
	*/
	void rollForBecomeSexWorker(EventParams& _eventParams, bool _isInit, double initialProb = 0.0);

	/*
	stop being csw
	*/
	void quitSexWork(EventParams &_eventParams);

	/*
	rerolls risk group based on if they are csw or not.  Called after rolling for becoming sex worker
	*/
	virtual void rerollRiskGroup(EventParams& _eventParams);

	/*
	*Sets a new SimContext for the person
	*/
	void setSimContext(SimContext * newSimContext);
	//---------------< END DmgProfile related methods >------------------------//

   //-----------------< Start methods Partnering/Selection Methods >-----------------------//
	//stores data to indicate that this person is in a sexual partnership
        // if this partnership is STEADY, then will change RelationshipStatus
	void addPartnership(SexualPartnership *_partnership);

	//returns true if this person is available for steady partnership
	// however, this does not change the person's DmgProfile value that corresponds to DmgProfile::RELATIONSHIP_STATUS
	bool availableForPartnership(SexualPartnership::Type _partnershipType) const;

	//use this when looking for a partner,
	//  @param _partnershipType - the type of partnership this person is looking to form
	//  @param _availablePools	- a set of pools that this person can choose from
	//  @param _remove - if true, than we will also remove the person from the EntityPool
	// returns: a Person from one of the person pools in _availablePools
	virtual Person* choosePartner(SexualPartnership::Type _partnershipType, EntityPool *_availableEntities, bool _remove);

	// fling with Person _p
	// this is used for SexualPartnership::Type where there is no duration associated with the partnership (i.e. CASUAL, CSW)
	//  will roll dice to see how many encounters there are during this fling...
	//  returns pointer to a newly infected person. returns NULL if no infection occurred
	Person* fling(Person *_p, SexualPartnership::Type _partnershipType, EventParams& _eventParams, InfectionsTracker *infTrack);

	/*
	returns true if this person is already in some sort of REGULAR or STEADY partnership with _p
	*/
	bool isPartneredWith(Person *_p);

	//Return true if the person is in a relationship of the given type
	bool hasPartnership(SexualPartnership::Type);

	/*******
	These enums expose characterstics of a person for the purpose of indexing or to assist for partner selection.

	It is used a data structure that has a template argument for sorting key

	If we change any enums here, we should change:
		class Person::Sorter;
		_KeyValType getMinPartnerSelectVal(SexualPartnership::Type _partnershipType, Gender _partnerGender) const; - for class Male, Female
		_KeyValType getMaxPartnerSelectVal(SexualPartnership::Type _partnershipType, Gender _partnerGender) const; - for class Male, Female
	********/
	enum SelectingCriteria {
		AGE,		//unsigned int
		SEXUAL_ACTIVITY_LEVEL,	//double between 0 and 1
		ID,						//unsigned int
		ENDSelectingCriteria
	};
	// gets the upper and lower bounds for an acceptable SelectingCriteria values of a potential partner
	//we are not allowed to have virtual templated functions... so we are forced to set return as double
	//  @param _partnershipType - type of partnership this person is seeking
	//	@param _partnerGender - gender of prospective partner
	virtual double getMinPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const;
	virtual double getMaxPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const;

	//Returns the age difference (in years) to center around
	virtual double rollForAgeDifference(SexualPartnership::Type _partnershipType, RandomNums& _randomNums);

	/*
	checks to see whether the duration limit of any SexualPartnerships have elapsed and will add them to a list to be removed
	note: this method does not remove any partnerships from this person.
	@param _fromDeath we are ending b/c this person has died. so will force all partnerships of this type to end
	@param _partnershipsToEnd when method is complete, _partnershipsToEnd will contain partnerships that should end.
	@return number of partnerships ended
	*/
	long getPartnershipsToEnd(long _currTime,SexualPartnership::Type _partnershipType, list<SexualPartnership*> &_partnershipsToEnd, bool _fromDeath);

	/*
	have sex with all partners where the SexualPartnership has a duration. To prevent double-counting activity (iterator hits both partners)
	sexual activity will only happen for the SexualPartnerships where this person is partner1
	@return returns a pointer to the person who infected this person.
	*/
	Person* allPartnerSexualActivity(EventParams& _eventParams, SexualPartnership::Type _partnershipType, list<Person*> &_newlyInfected, InfectionsTracker *infTrack);

	//returns whether this person could partner with Person _p
	//  split this by gender because there might be behaviour differences between them
	virtual bool possibleMatch(SexualPartnership::Type _partnershipType, Person *_p);

	//removes indications that this person is a particular sexual partnership
	//  this is called when that partnership separates
	//  will remove pointers to the SexualPartnership from both partners' partner lists
	void removePartnership(SexualPartnership *_partnership);

	//for a New partnership, roll how this person wants to be in this relationship
	virtual int rollForNewPartnershipDuration(SexualPartnership::Type _partnershipType, RandomNums& _randomNums, Person *_p);

	/*
	for a particular month, choose how many partners of _partnershipType this Person will have
	*/
	virtual int rollForNumPartners(RandomNums& _randomNums, SexualPartnership::Type _partnershipType);

	//for a particular partner, choose how many events this male will have
	virtual int rollNumEventsPerPartner(Person *_p, RandomNums& _randomNums, SexualPartnership::Type _partnershipType);

	/*
	sexual activity with person _p. This can happen within context of class SexualPartnership or just between to Persons
	@param _p partner for sexual activity
	@param _numActs number of sexual acts that happened
	@param _randomNums random number generator
	@param _infectionsTracker tracks the number of inf
	returns a pointer to a person who has been newly infected. NULL if no infection occured
	*/
	Person* sexualActivity(Person *_p, int _numActs, SexualPartnership::Type _partnershipType, EventParams& _eventParams, InfectionsTracker *infTrack);

	//-----------------< END methods Partnering/Selection Methods >-----------------------//


	//-----------------< Begin getters, setters, and helper methods >--------------//

	//gets the age of the person in desired granularity
	int getAge(TimeGranularity _granularity) const;

	//returns the unique id number of this person
	unsigned long getID();

	unsigned int getPopulationID();

	//returns traceMe
	bool trace();
	//sets traceMe to true
	void setToBeTraced();

	const Person::StatsRecord* getStats();

	//prints out person's id information
	void print(ostream& _outStream, string _prefix) const;

	void printCurrentPartners(ostream& _outStream, string _prefix);
	
	//Writes the state of the patient to file.  This state can be reloaded on a different run.
	virtual void saveState(ostream& _outStream, long currTime);

	//Unformed partnership tallies getters and setters -- the total should never be reset, only the "latest" (i.e. current time step)
	int getTotalUnformedPartnerships(SexualPartnership::Type type);
	int getLatestUnformedPartnerships(SexualPartnership::Type type);
	void increaseUnformedPartnershipTallies(SexualPartnership::Type type);
	void resetLatestUnformedPartnerships(SexualPartnership::Type type);

	GraphVizGraphElements::personNode* getPersonNode(){ return this->graphNode; }

	bool isOnArt() { return cepacPatient && cepacPatient->getARTState()->isOnART; }

	//-----------------< END getters, setters, and helper methods >--------------//





	/**
	//this class has a method that compares two Entities based on the desired key
	//_PSC holds the key that we search and index against.
	//_DEFAULTKEY provides a 2nd layer of ordering if people have identical _PSC
	// true is returned if key value of _p1 >= _p2. If key values are equal, then sorts based on Person's EntityID num
	**/
	template <Person::SelectingCriteria _PSC, class _KeyValType>
	class Sorter{
	public :
		//gets value associated with _p
		static inline _KeyValType getSortKey(Person* _p) {
			switch(_PSC) {
				case AGE : return (_KeyValType)_p->age;
				case SEXUAL_ACTIVITY_LEVEL : return (_KeyValType)_p->sexualActivityLevel;
				case ID : return (_KeyValType)_p->id;
				default : cerr << "Invalid Sorting key :" << _PSC; Util::exitWithPrompt(-1);
			}
			return 0;
		}

		//functor associated with the < operator. Generally used for template args in in sets and maps
		inline bool operator()(const Person* _p1, const Person* _p2) const {
			return getSortKey(_p1) < getSortKey(_p2);
		}

	};

private:
	//Return the current index of which SimContext should be used to update the health of a patient
	int getCEPACSimContextIndex(EventParams& _eventParams);
};
