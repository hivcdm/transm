#pragma once

#include "../../data/Enum.h"
#include "../../Constants.h"

#include <map>

class Person;

/**
This class corresponds to a Person's demographic profile.

It contains a tuple of the Person's demographic classifiers. The elements of the tuple are defined by enum Demographic.

Each unique tuple corresponds to a ProfileID. Each ProfileID corresponds to a DmgProfileBucket within EntityPool

Each Person contains an instance of DmgProfile which then determines which DmgProfileBucket they will be contained in

@author schung5
**/
class DmgProfile
{

public:

	//-------------< Begin Enums that hold relevent Demographics for DmgProfileBucket Placement >------------------//
	/*
		Unfortunately, when we want to change enum Demographic, we also need to change:
			void initEnums()
			fields/variables: demographicStrs,enumStrs,TotalNumBuckets

		For each enum, the END*** value is considered as a wildcard value in profile selection-related functions
	*/
	enum Demographic
	{
		SEXUAL_ACTIVITY_STATUS,
		GENDER,
		SEXUAL_ORIENTATION,
		RELATIONSHIP_STATUS,
		EMPLOYMENT,
		//LOCATION,	 this was proposed at one point and might come back someday
		ENDDemographic
	};

	//The ordering of the SexualActivityStatus enums matter b/c of aging.
	//Bad case: We age all NA first. Someone ages out of the NA bucket into an SA one.
	//			 Then we age all SA folks. If we aren't careful, then someone who
	//			 just joined an SA bucket might be aged again.
	enum SexualActivityStatus
	{
		SA,
		NA,
		ENDSexualActivityStatus,
	};

	//Every Person is one of these genders
	enum Gender
	{
		MALE,
		FEMALE,
		ENDGender,
	};

	enum SexualOrientation
	{
		HETERO,
		HOMO,
		ENDSexualOrientation,
	};


	//The ordering of the RelationshipStatus enums matter b/c of deaths.
	//we actually need to check death in NON_SINGLEs before singles. B/c there is a chance that both
	// members of the couples are dead. so what will happen is that 1 will be returned to the single's pool
	// so we don't want to skip processing this person. We could check both at the same time, but that could possibly
	// create a ripple effect of linked couple's needing the separate at the same time
	enum RelationshipStatus
	{
		NON_SINGLE,
		SINGLE,
		ENDRelationshipStatus
	};

	enum Employment
	{
		NON_CSW,
		CSW,
		/*
		//TRUCK_DRIVER
		*/
		ENDEmployment,
	};

	/*
	enum Location {
		URBAN,
		RURAL,
	};
	*/

	static Demographic MaxDemographic;
	//we have to statically define this here, b/c we  use this value elsewhere to statically declare arrays...
	static const unsigned int TotalNumBuckets = ENDSexualActivityStatus *ENDGender *ENDSexualOrientation
	        *ENDRelationshipStatus *ENDEmployment;

	//-------------< END Enums that hold relevent Demographics for DmgProfileBucket Placement >------------------//

	//------------< Begin type and struct definitions >--------------//

	//each unique profile has a unique integer value assigned to it.
	// tuples that have wildcard values do not have a profileID
	typedef int ProfileID;

	// functor for operator <. This is used in maps.
	struct less
	{
		bool operator()(const DmgProfile &_a, const DmgProfile &_b) const
		{
			return (_a < _b);
		}
	};

	//------------< END type and struct definitions >--------------//

	//------------< Begin fields >--------------//

	static const ProfileID
	NOT_UNIQUE;	//used as a return value to getProfileID to signify that the current tuple of enums inside this class contain a wildcard
	static const ProfileID MIN;			//min possible ProfileID
	static const ProfileID MAX;			//max useable ProfileID. i.e. a bucket exists for it
	static const ProfileID END;			//this is the ProfileID when all enums are at their wildcard value

private:
	//storage of actual enum values for a DmgProfile object. It's basically a tuple.
	BaseEnumCls::Enum enums[DmgProfile::ENDDemographic];

	//these hold class wrappers of each enum to allow for easy printing and iterating of demographic vals
	static std::vector<BaseEnumCls> DemographicEnumCls;

	//allows for quick look-up of ProfileID given a tuple
	static std::map<DmgProfile, ProfileID, DmgProfile::less> ProfileToProfileID;
	//allows for quick look-up of tuple given the dmgProfileID
	static std::vector<std::unique_ptr<const DmgProfile>> ProfileIDtoProfile;
	static std::vector <std::string> ProfileIDtoStr;

	//------------< End fields >--------------//

	//is called if initProfileIDMapCalled == false
	static void initProfileIDMap();
	//is called if initEnumsCalled == false
	static void initEnums();


public:

	/**
	operators. All these are implemented less efficiently b/c I couldn't figure out
	why memcmp wouldn't work.

	author: schung5
	**/


	bool operator==(const DmgProfile) const;
	bool operator!=(const DmgProfile) const;
	// left-most numbers have more weight for equality purposes than right ones
	// this isn't a result of anything to do with what the demographics are, it's just a
	// way to simplify making unique values
	bool operator<(const DmgProfile) const;
	bool operator<=(const DmgProfile) const;
	void operator=(const DmgProfile);
	//	sets this tuple to the next unique value
	void operator++(int);



	//initializes person with END values for dmgProfile. i.e. uninitialized
	DmgProfile();

	//copy constructor
	DmgProfile(const DmgProfile &_dmgProfile);

	//initializes person with enums values that correspond to _profileID
	DmgProfile(ProfileID _profileID);

	/**
	//get the value of a particular demographic that is stored in this tuple
	@author schung5
	**/
	BaseEnumCls::Enum get(Demographic _demographic) const;

	/**
	@return a profile ID if this DmgProfile contains a unique tuple of enums. If it contains If it has a wildcard in it, returns NOT_UNIQUE
	**/
	ProfileID getProfileID() const;

	/**
	returns true if this tuple matches the pattern in _selector
	the END*[enum]* values would be the wildcards
	@author schung5
	**/
	bool match(const DmgProfile &_selector) const;

	/**
	Will take a string representation of a tuple and store the values
	@author TBD
	**/
	void parse(std::string _tupleStr);

	/**
	//appends _prefix and string representation to output stream
	@author schung5
	**/
	void print(std::ostream &_outStream, std::string _prefix) const;

	//Saves the state of the dmgProfile to file
	void saveState(std::ostream &_outStream);

	/**
	Given the enums in this object, returns any Buckets that match the enum pattern
	Right now, we can only specify 1 desired value for each enum, or a wildcard for each enum.
	@param _selected This method will append matching ProfileID's to this std::vector
	@param _available If != nullptr, then this method will select from ProfileID's in this std::vector
	**/
	void selectProfileIDs(std::vector<ProfileID> &_selected, const std::vector<ProfileID> *_available) const;

	/**
	set the value of the particular demographic that is stored in this tuple
	Part of the reason that we don't have a set method for all Demographic values at once is that
		every time we change enum Demographic, we'd probably have to touch many places in the code.
		So you can only change one value at a time and potentially match multiple DmgBuckets when
		using the values inside a DmgProfile as a DmgProfileBucket selector.
	@author schung5
	**/
	void set(Demographic _demographic, BaseEnumCls::Enum _enum);

	/**
	set the values of the particular demographic that is stored in this tuple to match what
	the _profileID
	@author schung5
	**/
	void set(ProfileID _profileID);

	/**
	sets the tuple in this to match the one in _dmgProfile
	**/
	void set(const DmgProfile &_dmgProfile);

	/**
	//appends the string representation to this profile
	We return a pointer to save compute speed. We don't want a
	new string to be allocated cor each call

	@author schung5
	**/
	const std::string *toString() const;

public :

	//----------------< Begin Static Methods >------------------------------//

	//deallocates the statically stored strings we generated for fast lookup
	//this is to prevent any memory leaks
	static void deallocStaticMembers();

	static const BaseEnumCls *getEnumCls(Demographic _demographic);

	//given a ProfileID and a category, returns the value of that category that corresponds with the _profileID
	static BaseEnumCls::Enum get(ProfileID _profileID, Demographic _demographic);

	//given a ProfileID, returns a tuple of dmgProfile
	static const DmgProfile *getDemographics(ProfileID _profileID);

	/*
	gets a concatenated string of the string representation of all demographic values
	Looks up in a mapping of _profileID -> tring
	We return a pointer to save compute speed. We don't want a
	new string to be allocated cor each call
	*/
	static const std::string *toString(ProfileID _profileID);

	//gets a string representation of the _demographic value of the tuple that corresponds to _profileID
	static const std::string *getString(ProfileID _profileID, Demographic _demographic);

	//----------------< End Static Methods >------------------------------//

};
