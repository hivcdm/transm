#pragma once

#include "./../../util/rand/RandomNums.h"
#include "./../../data/Enum.h"
#include "./../../data/EventParams.h"

class Person;
class PopStats;
class InfectionsTracker;

/***
This class represents a SexualPartnership that lasts more than 1 month
Each member has a pointer to this object

If this couple is heterosexual, then by default, getPartner1() returns the Male
	while getPartner2() returns the Female

@author schung5
***/
class SexualPartnership
{

public :
	//this enum is used for when we are matching people
	//  will type of partnership determines partner criteria
	//Note: if this enum is modified, then also modify TypeEnumStrs
	enum class Type
	{
		Steady,
		Regular,
		Casual,
		Csw,
		//SteadyMsm,
		//RegularMsm,
		//CasualMsm,
		//CswMsm,
		//SteadyBisexual,
		//RegularBisexual,
		//CasualBisexual,
		//CswBisexual
		ENDType,
		Last = ENDType,
		First = Steady
	};
	
	static const std::map<Type, std::string> TypeStrings;

protected :

	//identifies the type of sexual relationship this is
	Type type;

	long timePartnerFormation;				//the time that this couple was formed
	long timePartnerDissolution;			//time that this partnership will dissolve

	Person *partners[2];			//this contains copies of pointers of partners

public :

	/**
	dummy constructor
	@author schung5
	**/
	SexualPartnership();

	/**
	Stores members and calculate the time of dissolution.

	Currently, _person1's personality determines how long this couple will stay together

	@param _person1 First person in the couple. If this is a heterosexual couple, make sure to put this one as Male
	@param _person2 Second person in the couple. If this is a heterosexual couple, make sure to put this one as Female
	@author schung5
	**/
	SexualPartnership(Person *_person1, Person *_person2, EventParams &_eventParams,
	                  SexualPartnership::Type _partnershipType);

	/**
	remove this couple from each member's list of current couples
	does not change the members but wipes the copy of the pointers held in this object.
	@author schung5
	**/
	virtual ~SexualPartnership();

	/**
	checks to see if current time matches the time that this couple is meant to split-up
	@param _currTime the current time in the simulation
	@returns true if _currTime >= timePartnerDissolution
	@author schung5
	**/
	bool checkTimeForSplit(long _currTime);

	/**
	Gets the pointer to partner 1. Should be male if this couple is heterosexual
	@author schung5
	**/
	Person *getPartner1();

	/**
	Gets the pointer to partner 2. Should be female if this couple is heterosexual
	@author schung5
	**/
	Person *getPartner2();


	/**
	@param _member one of the members of the couple
	@returns the other member of the couple
	**/
	Person *getOtherPartner(Person *_member);

	/**
	Gets what the type of this partnership is
	@author schung5
	**/
	Type getType();

	/**
	Get time of dissolution
	**/
	int getDissolutionTime();

	/**
	returns true if _p is a member of this partnership
	@author schung5
	**/
	bool isMember(Person *_p);

	/**
	@author schung5
	**/
	void printPartners(ostream &_outStream, string _prefix);

	/**
	* Saves state of this partnership to file
	**/
	void saveState(ostream &_outStream, int personID, long currTime);

	/**
	//models sexual activity in a couple.
	@return returns a pointer to a person who has been newly infected. nullptr if no infection occured
	@author schung5
	**/
	Person *monthlySexualActivity(EventParams &_eventParams, InfectionsTracker *infTrack);

	int getTimeOfFormation()
	{
		return timePartnerFormation;
	}
	int getTimeOfDissolution()
	{
		return timePartnerDissolution;
	}
};
