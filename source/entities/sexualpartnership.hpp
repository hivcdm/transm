#pragma once

#include <functional>

#include "transmissiontype.hpp"
#include "parameters/eventparams.hpp"
#include "utility/enum.hpp"
#include "utility/randomnumbergenerator.hpp"

namespace transm {

class Entity;
class InfectionsTracker;
class PopulationStatisticsOld;

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
		SteadyMsm,
		RegularMsm,
		CasualMsm,
		CswMsm,
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

	Entity *partners[2];			//this contains copies of pointers of partners

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
	SexualPartnership(Entity *_person1, Entity *_person2, EventParams &_eventParams,
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
	Entity *getPartner1();

	/**
	Gets the pointer to partner 2. Should be female if this couple is heterosexual
	@author schung5
	**/
	Entity *getPartner2();


	/**
	@param _member one of the members of the couple
	@returns the other member of the couple
	**/
	Entity *getOtherPartner(Entity *_member);

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
	bool isMember(Entity *_p);

	/**
	@author schung5
	**/
    void printPartners(ostream &_outStream, const std::string &_prefix);

	/**
	* Saves state of this partnership to file
	**/
	void saveState(ostream &_outStream, int personID, long currTime);

	/**
	//models sexual activity in a couple.
	@return returns a pointer to a person who has been newly infected. nullptr if no infection occured
	@author schung5
	**/
    Entity *monthlySexualActivity(EventParams &_eventParams, InfectionsTracker *infTrack, const std::unordered_map<TransmissionType, std::array<double, 10ULL>> &transmission_coefficients);

	int getTimeOfFormation()
	{
		return timePartnerFormation;
	}
	int getTimeOfDissolution()
	{
		return timePartnerDissolution;
	}
};

} // namespace transm

namespace std {

/// <summary>
/// Specialize std::hash for SexualPartnership::Type
/// </summary>
template<>
struct hash<transm::SexualPartnership::Type>
{
    using underlying_type = underlying_type<transm::SexualPartnership::Type>::type;
    using hasher = hash<underlying_type>;

    size_t operator()(const transm::SexualPartnership::Type &t) const
    {
        return hasher()(static_cast<underlying_type>(t));
    }
};

} // namespace std
