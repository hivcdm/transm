#pragma once

#include <iostream>
#include <list>
#include <set>
#include <vector>

#include "include.h"

#include "classifiers/DemographicProfile.h"
#include "classifiers/SexualPartnership.h"
#include "entitypool/FullVector.h"
#include "core/Constants.h"
#include "statistics/StatsRecord.h"
#include "utility/Utility.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/composite_key.hpp>

class ArtRolloutTracker;
class CostsTracker;
class EntityPool;
class EventParams;
class FullVector;
class InfectionsTracker;
class RandomNumberGenerator;

/// <summary>
/// All individuals in the simulation are of this class, or something derived from this
/// </summary>
/// <remarks>
/// Fields and Methods are divided into the following categories:
/// Physical, Relational, DemographicProfile-related, other
/// </remarks>
class Person
{
	/// <summary>
	/// used to create unique id's for each person
	/// this increments every time a New person is created
	/// </summary>
	static long idCounter;

public:

	virtual void Circumcise() = 0;

	void SetSexualActivityDelay(int delay) { sexualActivityDelay = delay; }

	int GetSexualActivityDelay() const { return sexualActivityDelay; }

	virtual bool IsCircumcised() const = 0;

	virtual void SetProportionHighRisk(DemographicProfile::Employment employment, double proportion) = 0;

	void SetPreExposureProphylaxisAdherence(double adherence) { preExposureProphylaxisAdherence_ = adherence; }

	virtual void SetPreExposureProphylaxisEfficacy(double efficacy) = 0;

	virtual double GetPreExposureProphylaxisEfficacy() const = 0;

	/// <summary>
	/// every Person's CD4 count falls in a CD4 strata - used in CEPAC
	/// </summary>
	enum CD4Strata
	{
		CD4_ZERO,
		CD4_ONE,
		CD4_TWO,
		CD4_THREE,
		CD4_FOUR,
		CD4_FIVE,
		ENDCD4Strata
	};

	/// <summary>
	/// every Person's hvl level falls in an HVL stratum (values in copies/mL)
	/// </summary>
	enum HVLStrata
	{
		/// <summary>
		/// HIV-
		/// </summary>
		UNINFECTED = -1,
		/// <summary>
		/// 0-20
		/// </summary>
		HVL_ZERO,
		/// <summary>
		/// 21-500
		/// </summary>
		HVL_ONE,
		/// <summary>
		/// 501-3000
		/// </summary>
		HVL_TWO,
		/// <summary>
		/// 3001-10000
		/// </summary>
		HVL_THREE,
		/// <summary>
		/// 10001-30000
		/// </summary>
		HVL_FOUR,
		/// <summary>
		/// 30001-100000
		/// </summary>
		HVL_FIVE,
		/// <summary>
		/// 100000+
		/// </summary>
		HVL_SIX,
		/// <summary>
		/// Initial stage of disease progression
		/// </summary>
		HVL_PRIMARY,
		/// <summary>
		/// Final stage of disease progression
		/// </summary>
		HVL_LATESTAGE,
		ENDHVLStrata
	};

	enum HIVStatus
	{
		NEGATIVE, //hiv negative
		OBSERVED_ACUTE,
		UNOBSERVED_ACUTE,
		OBSERVED_CHRONIC,
		UNOBSERVED_CHRONIC,
		OBSERVED_LATESTAGE,//Late stage takes precedence over chronic (acute cases are never latestage)
		UNOBSERVED_LATESTAGE,
		ENDHIVStatus,
        ANY_POSITIVE,
        ANY_OBSERVED_POSITIVE,
        ANY_NOT_OBSERVED_POSITIVE
	};

	enum DeathStatus
	{
		ALIVE, //not dead
		DTH_OI,
		DTH_CHRAIDS,
		DTH_NONAIDS,
		DTH_TOX_ART,
		DTH_TOX_PROPH,
		DTH_OTHER,
		ENDDeathStatus
	};

	enum RiskLevel   //used for assortativeness
	{
		LOW,
		HIGH,
		ENDRiskLevel
	};

    virtual void SetPartnershipRejectionChance(RiskLevel risk, SexualPartnership::Type partnershipType, double chance) = 0;
    virtual double GetPartnershipRejectionChance(RiskLevel risk, SexualPartnership::Type partnershipType) const = 0;
    virtual void SetOverrideChanceCondomUse(double chance) = 0;
    virtual double GetOverrideChanceCondomUse() const = 0;
    bool HasOverrideChanceCondomUse() const { return GetOverrideChanceCondomUse() != -1; }

	// we have made these stats referenceable by enum so that we can more easily create customizeable outputs or reports...
	// we can perhaps have easier look-up of stat descriptions if we choose to write some up
	enum Stats
	{
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
	static const std::vector<std::string> StatsStr;
	//this is a enum class wrapper that has helpful enum-related functions
	static EnumCls<Stats> StatsEnum;
	//this is a type declaration of a class that keeps track of statistics defined in enum Stats
	typedef ::StatsRecord<Stats, BaseEnumCls::nullptr_ENUM> StatsRecord;


	//the CEPAC death table has stats for 0-100 years old.
	//  people automatically die at this age in the dynamic model
	const static int maxYrForDeathStats = 101;

	//contains probabilities of nonAIDS-death, read from CEPAC .in file
	static std::vector<double>probDeathNatCauses[(std::size_t)DemographicProfile::Gender::Last];

	//there is option to print patient traces to a text file. this keeps track of how many we've done so far
	static int numTracesSoFar;

    virtual void SetTransmissionCoefficient(HVLStrata stratum, double coefficient) = 0;

    virtual void SetAssortativeness(SexualPartnership::Type partnership_type, double assortativeness) = 0;

    void UsePreExposureProphylaxis(double adherence);

    void SetTargetedCepacContext(SimContext *context) { setSimContext(context); targetedCepacContext_ = context; }

    bool HasTargetedCepacContext() const { return targetedCepacContext_ != nullptr; }

    SimContext *GetTargetedCepacContext() const { return targetedCepacContext_; }

protected:
	unsigned int populationID;			//keeps track of which population this Person belongs to
	unsigned long id;					//person's unique id number

    double preExposureProphylaxisAdherence_;

    SimContext *targetedCepacContext_;

    //person's current demographic profile - values in here depend on person's
    //physical, relational state, and other preferences
    DemographicProfile dmgProfile;
    //person keeps track of which BucketDemographicProfile they are currently in
    //  this value should stay equal to dmgProfile->getProfileID()
    //  sometimes a person's dmgProfile is changed, so we have to refresh their
    //  place in the EntityPool
    DemographicProfile::ProfileID currentBucketID;

	//Person's relational state
	//contains all current partnerships including CSW and Casual
	std::list<SexualPartnership *> partners[(int)SexualPartnership::Type::ENDType];

	//array of number of partners over persons history stratified by partnership type
	int numPartnersInHistory[(int)SexualPartnership::Type::ENDType];

	//array of month of their farthest current partnership dissolution time for each partnership type.  initialized to zero
	int monthOfLatestPartnershipDissolution[(int)SexualPartnership::Type::ENDType];

	//array of month of latest concurrent relationship for each partnership type. Only updated for 12 months before calibration
	int monthOfLatestConcurrent;

	// Contains the number of partnerships the person tried to form over time, but didn't
	// (usually due to no partners available or re-hooking up with a current partner)
	int unformedPartnershipsTotal[(int)SexualPartnership::Type::ENDType];
	int unformedPartnershipsLatestTime[(int)SexualPartnership::Type::ENDType];



	//TODO: What is this and is it still used?  It doesn't look like it.
	double sexualActivityLevel;			//relative risky behavior level
	int generationOfInfection;			//Which generation was the person infected in
	bool wentThroughCEPAC;				//flag to indicate whether this person has CEPAC data
	//MonthEvent *healthAfterInfection;	//this contains monthly health states as generated by CEPAC
	Patient *cepacPatient;				//a pointer to a CEPAC patient object which stores all post-infection health states
	double CEPACcosts;					//A running tally of all costs accrued through CEPAC

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
	map<FullVector *, vector<unsigned int>> FVindices; //The indices of the the person in their assigned FullVector

    int sexualActivityDelay;

public:

	//dummy constructor
	Person();

	//this constructor creates an actual person that can be simulated. It is generally called by Male and Female
	// we pass in _eventParams because becomeInfected() needs it...
	Person(int _age, unsigned int _populationID);

	virtual ~Person();

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

	CD4Strata getCd4Stratum() const;
	HVLStrata getHvlStratum() const;

	bool isEligibleForTreatment(const SimContext::TreatmentInputs::ARTStartPolicy &artStartPolicy);

	//This is for keeping dead people around for graph printing reasons
	//It mimics the destructor without destroying the Person object.
	void deletePersonWithoutDeleting();

	//---------------< Start Physical-state related methods >------------------------//

	void ageOneTimeUnit();

	/*
	 * call this to infect person...
	 * if CEPAC bridge is in place, will call CEPAC to determine the health 
	 * trajectory of this person
	 * @params _prevalentInfection if true, than this person was a prevalent 
	 * infection
	*/
	void becomeInfected(int _generationOfInfection, EventParams &_eventParams);

	void seedInfection(int _generationOfInfection, EventParams &_eventParams,
		bool chronicInfection);

	/*
	 * Initializes cepacPatient using the persons current age, gender, and infection status.
	 * Prevalent cases should call "becomeInfected" before calling this function; incident cases will become infected later
	 */
	void initializeCEPACpatient(EventParams &_eventParams);

	void updateCEPACpatient(EventParams &_eventParams);

	virtual double getChanceBecomeCsw() const = 0;

	/**
	 * @return generationOfInfection
	 */

	int getGenerationOfInfection(bool cap_at_5 = true) const;

	/**
	*	returns the number of partners by partnership type
	*/
	int getNumPartners(SexualPartnership::Type);

	/**
	*	returns the number of partners by partnership type that are either the samerisk or different
	*/
	int getNumPartners(SexualPartnership::Type, bool);

	double getQualityOfLife() const { return cepacPatient != nullptr ? cepacPatient->getGeneralState()->QOLMultiplier : 1; }

	double getCepacDiscountFactor(int month, double discount_rate) const 
	{ 
		return 1 / std::pow(discount_rate, month);
	}

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
	 * @return hvl
	 */
	HVLStrata getHVL() const
	{
        return hvl;
	}

	/** this calculates the FOI towards Person _p (this uses the Transmission coefficient) per event
	// @param _p - partner
	// @param _parteringType - whether this is a fling or steadyCouple */
	virtual double getFOI(Person *_p, SexualPartnership::Type _partnershipType, EventParams &_eventParams) = 0;

	/** get the transmission coefficient of the person... based on HVL */
	virtual double getTransmissionCoeff() = 0;

	//returns true if person is currently alive
	bool isAlive() const;

	//returns true if person is currently infected
	bool isInfected();

	// Self explanatory I'd say
	bool isSexuallyActive();

	bool isCSW() const;

	bool isMale() const;

	/**
	//see whether person dies. If they went through CEPAC, use health trace. else roll against nonAIDS death probs
	**/
	bool rollForDeath(RandomNumberGenerator &_randomNums);

	//update health status of HIV infected people -- i.e. cd4, hvl, art, etc.
	//  in version 1, this information is taken from CEPAC model
	// @returns: costs (accrued in CEPAC) of updating health
	double updateHealthStatus(EventParams &_eventParams, ArtRolloutTracker *testTracker, CostsTracker *costsTracker);

	//Call this after all transmission/population dynamics are done.
	//Runs infected through CEPAC until they die and adds their LM etc to CEPAC stats
	void runCEPACtoDeath(RandomNumberGenerator &_randomNums);

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
	bool setFVindices(vector<unsigned int> FVind, FullVector *FV);

	/* @function: addFVindices
	 * @arguments: int index, FullVector* FV
	 * @effects: If FV[index] points to this and index is not already a member of
	 * this.FVindices, adds index to this.FVindices
	 * @return: true if index was added to this.FVindices or false otherwise
	 */
	bool addFVindices(int index, FullVector *FV);

	/* @function: removeFVindices
	 * @arguments: int index, FullVector* FV
	 * @effects: If FV[index] does not point to this, removes index from this.FVindices
	 * @return: true if index was removed from this.FVindices, false otherwise
	 */
	bool removeFVindices(int index, FullVector *FV);

	/* @function: memberFVindices
	 * @arguments: int index, FullVector* FV
	 * @effects: none
	 * @return: true iff this.FVindices contains index
	 */
	bool memberFVindices(int index, FullVector *FV);

	/* @function: getFVindices
	 * @arguments: none
	 * @effects: none
	 * @return: copy of this.FVindices
	 */
	std::vector<unsigned int> getFVindices(FullVector *FV);

	//----------------< End FullVector related methods >-------------------------//

	//-------------< Start BucketAge related methods >----------------------//
	/* @function: getRiskLevel
	 * @return: this.risk
	 */
	Person::RiskLevel getRiskLevel() const;

	/* @function: getHIVStatus
	 * @return: this.hivStatus
	 */
	Person::HIVStatus getHIVStatus() const;

	/* @function: getSexualActivity
	 * @return: this.activityLevel
	 */
	int getSexualActivity();

	//---------------< Start DemographicProfile related methods >------------------------//

	/*
	changes this person to sexually active
	and initializes the CEPAC person
	*/
	void becomeSexuallyActive(EventParams &_eventParams);

	//returns structure that holds current DemographicProfile
	const DemographicProfile *getDemographicProfile() const;
	BaseEnumCls::Enum getDemographicProfileVal(DemographicProfile::Demographic _demographic) const;

    template<typename D>
    D getDemographicProfileVal() const;

	//sets and gets current BucketDemographicProfile membership
	DemographicProfile::ProfileID getCurrBucketProfileID();
	void setCurrBucketProfileID(DemographicProfile::ProfileID _profileID);

	//returns true if the person's DemographicProfile matches their current BucketDemographicProfile membership
	bool inCorrectBucketDemographicProfile();

	/*
	changes isSexWorker with probability taken from population prevalence of CSW (or initial csw chance if prevalent population)
	*/
	void rollForBecomeSexWorker(EventParams &_eventParams, bool _isInit, double initialProb = 0.0);

	/*
	stop being csw
	*/
	void quitSexWork(EventParams &_eventParams);

	/*
	rerolls risk group based on if they are csw or not.  Called after rolling for becoming sex worker
	*/
	virtual void rerollRiskGroup(EventParams &_eventParams) = 0;

	/*
	*Sets a new SimContext for the person
	*/
	void setSimContext(SimContext *newSimContext);
	//---------------< END DemographicProfile related methods >------------------------//

	//-----------------< Start methods Partnering/Selection Methods >-----------------------//
	//stores data to indicate that this person is in a sexual partnership
	// if this partnership is STEADY, then will change RelationshipStatus
	void addPartnership(SexualPartnership *_partnership);

	//returns true if this person is available for steady partnership
	// however, this does not change the person's DemographicProfile value that corresponds to DemographicProfile::Demographic::RelationshipStatus
	bool availableForPartnership(SexualPartnership::Type _partnershipType) const;

	//use this when looking for a partner,
	//  @param _partnershipType - the type of partnership this person is looking to form
	//  @param _availablePools	- a set of pools that this person can choose from
	//  @param _remove - if true, than we will also remove the person from the EntityPool
	// returns: a Person from one of the person pools in _availablePools
	/*
	virtual Person *choosePartner(RandomNumberGenerator &_randomNums, EntityPool *_availableEntities,
				      SexualPartnership::Type _partnershipType, bool _remove);
					  */
	// fling with Person _p
	// this is used for SexualPartnership::Type where there is no duration associated with the partnership (i.e. CASUAL, CSW)
	//  will roll dice to see how many encounters there are during this fling...
	//  returns pointer to a newly infected person. returns nullptr if no infection occurred
	Person *fling(Person *_p, SexualPartnership::Type _partnershipType, EventParams &_eventParams,
	              InfectionsTracker *infTrack);

	/*
	returns true if this person is already in some sort of REGULAR or STEADY partnership with _p
	*/
	bool isPartneredWith(Person *_p);

	//Return true if the person is in a relationship of the given type
	bool hasPartnership(SexualPartnership::Type);

	// Return true if person is in ANY partnership
	bool hasPartnership();

	/*******
	These enums expose characterstics of a person for the purpose of indexing or to assist for partner selection.

	It is used a data structure that has a template argument for sorting key

	If we change any enums here, we should change:
		class Person::Sorter;
		_KeyValType getMinPartnerSelectVal(SexualPartnership::Type _partnershipType, Gender _partnerGender) const; - for class Male, Female
		_KeyValType getMaxPartnerSelectVal(SexualPartnership::Type _partnershipType, Gender _partnerGender) const; - for class Male, Female
	********/
	enum SelectingCriteria
	{
		AGE,		//unsigned int
		SEXUAL_ACTIVITY_LEVEL,	//double between 0 and 1
		ID,						//unsigned int
		ENDSelectingCriteria
	};
	// gets the upper and lower bounds for an acceptable SelectingCriteria values of a potential partner
	//we are not allowed to have virtual templated functions... so we are forced to set return as double
	//  @param _partnershipType - type of partnership this person is seeking
	//	@param _partnerGender - gender of prospective partner
	virtual double getMinPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const = 0;
	virtual double getMaxPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const = 0;

	//Returns the age difference (in years) to center around
	virtual double rollForAgeDifference(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums) = 0;

	/*
	checks to see whether the duration limit of any SexualPartnerships have elapsed and will add them to a list to be removed
	note: this method does not remove any partnerships from this person.
	@param _fromDeath we are ending b/c this person has died. so will force all partnerships of this type to end
	@param _partnershipsToEnd when method is complete, _partnershipsToEnd will contain partnerships that should end.
	@return number of partnerships ended
	*/
	long getPartnershipsToEnd(long _currTime, SexualPartnership::Type _partnershipType,
	                          list<SexualPartnership *> &_partnershipsToEnd, bool _fromDeath);

	/*
	have sex with all partners where the SexualPartnership has a duration. To prevent double-counting activity (iterator hits both partners)
	sexual activity will only happen for the SexualPartnerships where this person is partner1
	@return returns a pointer to the person who infected this person.
	*/
	Person *allPartnerSexualActivity(EventParams &_eventParams, SexualPartnership::Type _partnershipType,
	                                 list<Person *> &_newlyInfected, InfectionsTracker *infTrack);

	//returns whether this person could partner with Person _p
	//  split this by gender because there might be behaviour differences between them
	virtual bool possibleMatch(SexualPartnership::Type _partnershipType, Person *_p) = 0;

	//removes indications that this person is a particular sexual partnership
	//  this is called when that partnership separates
	//  will remove pointers to the SexualPartnership from both partners' partner lists
	void removePartnership(SexualPartnership *_partnership);

	//for a New partnership, roll how this person wants to be in this relationship
	virtual int rollForNewPartnershipDuration(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums, Person *_p) = 0;

	/*
	for a particular month, choose how many partners of _partnershipType this Person will have
	*/
	virtual int rollForNumPartners(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType) = 0;

	//for a particular partner, choose how many events this male will have
	virtual int rollNumEventsPerPartner(Person *_p, RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType) = 0;

	/*
	sexual activity with person _p. This can happen within context of class SexualPartnership or just between to Persons
	@param _p partner for sexual activity
	@param _numActs number of sexual acts that happened
	@param _randomNums random number generator
	@param _infectionsTracker tracks the number of inf
	returns a pointer to a person who has been newly infected. nullptr if no infection occured
	*/
	Person *sexualActivity(Person *_p, int _numActs, SexualPartnership::Type _partnershipType, EventParams &_eventParams,
	                       InfectionsTracker *infTrack);

	//-----------------< END methods Partnering/Selection Methods >-----------------------//


	//-----------------< Begin getters, setters, and helper methods >--------------//

	//gets the age of the person in desired granularity
	int getAge(TimeGranularity _granularity) const;

	//returns the unique id number of this person
	unsigned long getID() const;

	unsigned int getPopulationID();

	//returns traceMe
	bool trace();
	//sets traceMe to true
	void setToBeTraced();

	void enableInfectionTrace(int _generationOfInfection,
				  EventParams &_eventParams);

	const Person::StatsRecord *getStats();

	//prints out person's id information
    void print(std::ostream &_outStream, const std::string &_prefix) const;

    void printCurrentPartners(std::ostream &_outStream, const std::string &_prefix);

	//Writes the state of the patient to file.  This state can be reloaded on a different run.
	virtual void saveState(std::ostream &_outStream, long currTime);

	//Unformed partnership tallies getters and setters -- the total should never be reset, only the "latest" (i.e. current time step)
	int getTotalUnformedPartnerships(SexualPartnership::Type type);
	int getLatestUnformedPartnerships(SexualPartnership::Type type);
	void increaseUnformedPartnershipTallies(SexualPartnership::Type type);
	void resetLatestUnformedPartnerships(SexualPartnership::Type type);

	bool isOnArt()
	{
		return cepacPatient && cepacPatient->getARTState()->isOnART;
	}

	virtual void SetChanceCondomUsePerEvent(RiskLevel risk, SexualPartnership::Type partnershipType, BetaDist dist, RandomNumberGenerator &rng) = 0;
	virtual const BetaDist GetChanceCondomUsePerEvent(RiskLevel risk, SexualPartnership::Type partnershipType) = 0;

	virtual void SetCoitalEventsPerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, double meanEvents) = 0;

	virtual void SetPartnershipDuration(RiskLevel risk, SexualPartnership::Type partnershipType, ShiftedLogNormalDist dist) = 0;

	virtual void SetAverageYearsYounger(SexualPartnership::Type partnershipType, NormalDist dist) = 0;

    virtual void SetAcquisitionRatePerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, LogNormalDist dist, RandomNumberGenerator &rng) = 0;

    virtual void SetChanceBecomeSexWorker(double chance) = 0;

	/// <summary>
	/// this class has a method that compares two Entities based on the desired key
	/// _PSC holds the key that we search and index against.
	/// _DEFAULTKEY provides a 2nd layer of ordering if people have identical _PSC
	/// true is returned if key value of _p1 >= _p2. If key values are equal, then sorts based on Person's EntityID num
	/// </summary>
	template <Person::SelectingCriteria _PSC, class _KeyValType>
	class Sorter
	{
	public :
		//gets value associated with _p
		static inline _KeyValType getSortKey(Person *_p)
		{
			switch(_PSC)
			{
			case AGE: return (_KeyValType)_p->age;
			case SEXUAL_ACTIVITY_LEVEL: return (_KeyValType)_p->sexualActivityLevel;
			case ID: return (_KeyValType)_p->id;
			}

			throw std::runtime_error("Invalid Sorting key");
		}

		//functor associated with the < operator. Generally used for template args in in sets and maps
		inline bool operator()(const Person *_p1, const Person *_p2) const
		{
			return getSortKey(_p1) < getSortKey(_p2);
		}

	};

	void reset_costs() 
	{  
		monthly_cepac_costs_discounted_ = 0; 
		monthly_cepac_costs_undiscounted_ = 0;
		monthly_cdm_costs_discounted_ = 0; 
		monthly_cdm_costs_undiscounted_ = 0;
	}

	void add_cepac_cost(double cost_undiscounted, double cost_discounted) 
	{
		monthly_cepac_costs_undiscounted_ += cost_undiscounted; 
		monthly_cepac_costs_discounted_ += cost_discounted;
	}
	double get_monthly_cepac_costs_undiscounted() const { return monthly_cepac_costs_undiscounted_; }
	double get_monthly_cepac_costs_discounted() const { return monthly_cepac_costs_discounted_; }
	void add_cdm_cost(double cost_undiscounted, double cost_discounted) 
	{ 
		monthly_cdm_costs_undiscounted_ += cost_undiscounted; 
		monthly_cdm_costs_discounted_ += cost_discounted;
	}
	double get_monthly_cdm_costs_undiscounted() const { return monthly_cdm_costs_undiscounted_; }
	double get_monthly_cdm_costs_discounted() const { return monthly_cdm_costs_discounted_; }

    bool UsingPrEP()
    {
        return using_prep_this_month_;
    }

private:
    //Return the current index of which SimContext should be used to update the
    //health of a patient
    int getCEPACSimContextIndex(EventParams &_eventParams);

    double updateHealthCosts(EventParams &_eventParams,
			     CostsTracker *costsTracker,
			     const RunStats::OverallCosts before,
			     const RunStats::OverallCosts after);
    void updateTestingStatus(EventParams &_eventParams,
			     ArtRolloutTracker *testTracker,
			     const RunStats::HIVScreening before,
			     const RunStats::HIVScreening after);
    void traceTreatmentChange(EventParams &_eventParams, bool after);
    void traceCD4Change(EventParams &_eventParams, double before, double after);
    void traceHVLChange(EventParams &_eventParams, HVLStrata before,
			HVLStrata after);
    void traceHIVChange(EventParams &_eventParams, HIVStatus before,
			HIVStatus after);

    double monthly_cepac_costs_undiscounted_;
    double monthly_cepac_costs_discounted_;
    double monthly_cdm_costs_undiscounted_;
    double monthly_cdm_costs_discounted_;

    bool using_prep_this_month_;
};
