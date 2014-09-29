#include <memory>
#include <sstream>
#include <typeinfo>

#include "DemographicProfile.h"
#include "../Person.h"
#include "../../util/Utility.h"
#include "../../util/make_unique.h"

//declare strings of Enums
const std::vector<std::string> demographicStrs = {"SEXUAL_ACTIVITY_STATUS", "GENDER", "SEXUAL_ORIENTATION", "RELATIONSHIP_STATUS", "EMPLOYMENT"};

const std::vector<std::vector<std::string>> enumStrs =
{
	{"SA", "NA"},
	{"MALE", "FEMALE"},
	{"HETERO", "HOMO", "BI"},
	{"NON_SINGLE", "SINGLE"},
	{"NON_CSW", "CSW"}
};
DemographicProfile::Demographic DemographicProfile::MaxDemographic = DemographicProfile::Demographic((std::size_t)DemographicProfile::Demographic::Last - 1);

std::vector<BaseEnumCls> DemographicProfile::DemographicEnumCls;
std::vector<std::unique_ptr<const DemographicProfile>> DemographicProfile::ProfileIDtoProfile;
std::vector<string> DemographicProfile::ProfileIDtoStr;
std::map<DemographicProfile, DemographicProfile::ProfileID, DemographicProfile::less> DemographicProfile::ProfileToProfileID;

//declare fields of class DemographicProfile
DemographicProfile::ProfileID NOT_UNIQUE = -1;
const DemographicProfile::ProfileID DemographicProfile::NOT_UNIQUE = DemographicProfile::ProfileID(
            -1);	//used as a return value to getProfileID to signify that the current tuple of enums inside this class contain a wildcard
const DemographicProfile::ProfileID DemographicProfile::MIN = DemographicProfile::ProfileID(0);
const DemographicProfile::ProfileID DemographicProfile::MAX = DemographicProfile::TotalNumBuckets - 1;
const DemographicProfile::ProfileID DemographicProfile::END = DemographicProfile::ProfileID((1 + (std::size_t)SexualActivityStatus::Last) * (1 + (std::size_t)Gender::Last) *
    (1 + (std::size_t)SexualOrientation::Last) * (1 + (std::size_t)RelationshipStatus::Last) * (1 + (std::size_t)Employment::Last));

//----------------< Start Functions & fields for class DemographicProfile >--------------------//

DemographicProfile::DemographicProfile()
{
	set(DemographicProfile::END);
}

DemographicProfile::DemographicProfile(const DemographicProfile &_dmgProfile)
{
	set(_dmgProfile);
}

DemographicProfile::DemographicProfile(ProfileID _profileID)
{
	assert((_profileID == DemographicProfile::END) || Utility::withinRange(_profileID, DemographicProfile::MIN, DemographicProfile::MAX));
	set(_profileID);
}

bool DemographicProfile::operator==(const DemographicProfile _a) const
{
	//i don't know why we can't use memcmp, but it seems to not be working correctly.
	//return (::memcmp( enums, _a.enums, DemographicProfile::Demographic::Last) == 0);
	// it thinks that {1,0,0,0} is equal to {1,1,1,1}
	//check from left-most value for equality
	DemographicProfile::Demographic currDemographic = DemographicProfile::Demographic(0);

	while(currDemographic < DemographicProfile::Demographic::Last)
	{
        if(enums[(std::size_t)currDemographic] != _a.enums[(std::size_t)currDemographic])
		{
			return false;
		}

		currDemographic = DemographicProfile::Demographic((std::size_t)currDemographic + 1);
	}

	return true;
}

bool DemographicProfile::operator!=(const DemographicProfile _a) const
{
	return !(*this == _a);
}

bool DemographicProfile::operator<(const DemographicProfile _a) const
{
	if(*this == _a)
	{
		return false;
	}

	//check from left-most value for greater than
	DemographicProfile::Demographic currDemographic = DemographicProfile::MaxDemographic;
	DemographicProfile::Demographic min = DemographicProfile::Demographic(0);

	while(currDemographic >= min)
	{
        if(enums[(std::size_t)currDemographic] > _a.enums[(std::size_t)currDemographic])
		{
			return false;
		}

        if(enums[(std::size_t)currDemographic] < _a.enums[(std::size_t)currDemographic])
		{
			return true;
		}

        currDemographic = (Demographic)(((std::size_t)currDemographic) - 1);
	}

	return true;
}

bool DemographicProfile::operator<=(const DemographicProfile _a) const
{
	if(*this == _a)
	{
		return true;
	}

	if(*this < _a)
	{
		return true;
	}

	return false;
}

void DemographicProfile::operator=(const DemographicProfile _a)
{
	DemographicProfile::Demographic currDemographic = DemographicProfile::Demographic(0);

	//set the internal enum fields equal to _a
	while(currDemographic < DemographicProfile::Demographic::Last)
	{
		set(currDemographic, _a.get(currDemographic));
        currDemographic = (Demographic)((std::size_t)currDemographic + 1);
	}
}

void DemographicProfile::operator++(int)
{
	if(DemographicProfile::DemographicEnumCls.size() == 0)
	{
		DemographicProfile::initEnums();
	}

	//set an assert to check if we are at max value
	assert(getProfileID() != DemographicProfile::MAX);
	assert(getProfileID() != DemographicProfile::END);
	DemographicProfile::Demographic currPlace = DemographicProfile::MaxDemographic;
	DemographicProfile::Demographic min = DemographicProfile::Demographic(0);
	//add one to the right-most column
	set(currPlace, get(currPlace) + 1);
	//carry if needed
	bool carry = false;

	while(currPlace >= min)
	{
		if(carry)
		{
			set(currPlace, get(currPlace) + 1);
		}

		if(get(currPlace) > DemographicProfile::getEnumCls(currPlace)->getMax())
		{
			carry = true;
			set(currPlace, DemographicProfile::getEnumCls(currPlace)->getMin());
		}
		else
		{
			carry = false;
		}

		currPlace = (Demographic)((std::size_t)currPlace - 1);
	} //while(currPlace >= DemographicProfile::DemographicCategoriesCls.getMin())  {
}

BaseEnumCls::Enum DemographicProfile::get(DemographicProfile::Demographic _demographic) const
{
	assert(Utility::withinRange(_demographic, DemographicProfile::Demographic(0), DemographicProfile::MaxDemographic));
    return enums[(std::size_t)_demographic];
}

DemographicProfile::ProfileID DemographicProfile::getProfileID() const
{
	//the map tupleToID stores tuple->id
	return DemographicProfile::ProfileToProfileID[*this];
}


bool DemographicProfile::match(const DemographicProfile &_selector) const
{
	DemographicProfile::Demographic currDemographic = DemographicProfile::Demographic(0);

	//loop through each Demographic
	while(currDemographic <= DemographicProfile::MaxDemographic)
	{
		//skip if wildcard
        if(_selector.get(currDemographic) == DemographicProfile::DemographicEnumCls.at((std::size_t)currDemographic).getWildcard())
		{
            currDemographic = DemographicProfile::Demographic((std::size_t)currDemographic + 1);
			continue;
		}

		//if enums don't match, then this tuple doesn't match
        if(_selector.get(currDemographic) != (enums[(std::size_t)currDemographic]))
		{
			return false;
		}

        currDemographic = DemographicProfile::Demographic((std::size_t)currDemographic + 1);
	}

	return true;
}

void DemographicProfile::parse(string _tupleStr)
{
	if(DemographicProfile::DemographicEnumCls.size() == 0)
	{
		DemographicProfile::initEnums();
	}

	//we want to tokenize a string representatino of a tuple
	vector<string> tokens;
	//currently allowed delimiters are: "_"
	Utility::Tokenize(_tupleStr, tokens, Constants::COLON);

	//make sure we have correct amount of tokens
    if(tokens.size() != (std::size_t)DemographicProfile::Demographic::Last)
	{
		throw std::runtime_error("Tuple String " + _tupleStr + " is not valid");
	}

	//encode each part of the string into an enum value
	DemographicProfile::Demographic currDemographic = DemographicProfile::Demographic(0);
	const BaseEnumCls *currCategoryCls;

	//loop through each Demographic
	while(currDemographic < DemographicProfile::Demographic::Last)
	{
		//get the current Demographic helper enum
		currCategoryCls = DemographicProfile::getEnumCls(currDemographic);
		//get the enum value from _tupleStr's tokens
        BaseEnumCls::Enum e = currCategoryCls->fromString(tokens.at((std::size_t)currDemographic));
		//if we get currCategoryCls->fromString(tokens.at(currDemographic)) to throw an exception, then we can use a better error msg
		//cerr << "Tuple String " << _tupleStr << " is not valid. (" << _tupleStr.at(currDemographic) << ")" << std::endl;
		set(currDemographic, e);
        currDemographic = DemographicProfile::Demographic((std::size_t)currDemographic + 1);
	} //while(currDemographic < DemographicProfile::Demographic::Last) {
}


void DemographicProfile::print(std::ostream &_outStream, const std::string &_prefix) const
{
	_outStream << _prefix << DemographicProfile::ProfileIDtoStr[getProfileID()];
}

void DemographicProfile::saveState(std::ostream &_outStream)
{
	_outStream << "dmg:";
	_outStream << "[";

    for(int i = 0; i < (std::size_t)Demographic::Last; i++)
	{
		_outStream << enums[i];

        if(i != (std::size_t)Demographic::Last - 1)
		{
			_outStream << ",";
		}
	}

	_outStream << "]," << std::endl;
}
void DemographicProfile::selectProfileIDs(std::vector<ProfileID> &_selected, const std::vector<ProfileID> *_available) const
{
	if(DemographicProfile::ProfileIDtoProfile.size() == 0)
	{
		DemographicProfile::initProfileIDMap();
	}

	size_t i = 0;

	if(_available)
	{
		//loop through subset of all possible ProfileID's
		while(i < _available->size())
		{
			//if the profile that correponds to the ProfileID is matched by this(which is the selector here)
			//  then add the ProfileID to the resulting vector
			if(DemographicProfile::ProfileIDtoProfile.at(_available->at(i))->match(*this))
			{
				_selected.push_back(_available->at(i));
			}

			i++;
		} //while(i < _available->size()) {
	}
	else
	{
		//loop though all possible ProfileID
		do
		{
			if(DemographicProfile::ProfileIDtoProfile.at(i)->match(*this))
			{
				_selected.push_back((ProfileID)i);
			}

			i++;
		}
		while(i <= DemographicProfile::MAX);
	}//if(_available) {
}

void DemographicProfile::set(DemographicProfile::Demographic _demographic, BaseEnumCls::Enum _enum)
{
	if(DemographicProfile::DemographicEnumCls.size() == 0)
	{
		DemographicProfile::initEnums();
	}

	assert(Utility::withinRange(_demographic, DemographicProfile::Demographic(0), DemographicProfile::MaxDemographic));
	assert(DemographicProfile::getEnumCls(_demographic)->isValidNonWildCard(_enum)
	       || (_enum == DemographicProfile::getEnumCls(_demographic)->getWildcard()));
    enums[(std::size_t)_demographic] = _enum;
}


void DemographicProfile::set(DemographicProfile::ProfileID _profileID)
{
	assert((_profileID == DemographicProfile::END) || Utility::withinRange(_profileID, DemographicProfile::MIN, DemographicProfile::MAX));

	if(DemographicProfile::DemographicEnumCls.size() == 0)
	{
		DemographicProfile::initEnums();
	}

	if(DemographicProfile::ProfileIDtoProfile.size() == 0)
	{
		DemographicProfile::initProfileIDMap();
	}

	if(_profileID == DemographicProfile::END)
	{
		//set each value of Tuple to wildcard
		DemographicProfile::Demographic currDemographic = DemographicProfile::Demographic(0);

		while(currDemographic < DemographicProfile::Demographic::Last)
		{
            set(currDemographic, DemographicProfile::DemographicEnumCls.at((std::size_t)currDemographic).getNumEnums());
            currDemographic = DemographicProfile::Demographic((std::size_t)currDemographic + 1);
		}//while(currDemographic < DemographicProfile::Demographic::Last) {
	}
	else
	{
		//look up the tuple that matches this ProfileID and internalize values
		if(DemographicProfile::ProfileIDtoProfile.size() == 0)
		{
			DemographicProfile::initProfileIDMap();
		}

		set(*(ProfileIDtoProfile.at(_profileID)));
	}
}

void DemographicProfile::set(const DemographicProfile &_dmgProfile)
{
	DemographicProfile::Demographic currDemographic = DemographicProfile::Demographic(0);

	while(currDemographic < DemographicProfile::Demographic::Last)
	{
		set(currDemographic, _dmgProfile.get(currDemographic));
        currDemographic = DemographicProfile::Demographic((std::size_t)currDemographic + 1);
	}
}

const string *DemographicProfile::toString() const
{
	return &DemographicProfile::ProfileIDtoStr[getProfileID()];
}

//----------------< End Functions for class DemographicProfile >--------------------//


//--------------------< Begin Static Methods >-----------------------//

BaseEnumCls::Enum DemographicProfile::get(ProfileID _profileID, Demographic _demographic)
{
	assert(Utility::withinRange(_profileID, DemographicProfile::MIN, DemographicProfile::MAX));
	assert(Utility::withinRange(_demographic, DemographicProfile::Demographic(0), DemographicProfile::MaxDemographic));
	return DemographicProfile::ProfileIDtoProfile.at(_profileID)->get(_demographic);
}

const BaseEnumCls *DemographicProfile::getEnumCls(DemographicProfile::Demographic _demographic)
{
	assert(Utility::withinRange(_demographic, DemographicProfile::Demographic(0), DemographicProfile::MaxDemographic));

	if(DemographicProfile::DemographicEnumCls.size() == 0)
	{
		DemographicProfile::initEnums();
	}

    return &(DemographicProfile::DemographicEnumCls.at((std::size_t)_demographic));
}

const string *DemographicProfile::getString(ProfileID _profileID, Demographic _demographic)
{
	assert(Utility::withinRange(_profileID, DemographicProfile::MIN, DemographicProfile::MAX));
	assert(Utility::withinRange(_demographic, DemographicProfile::Demographic(0), DemographicProfile::MaxDemographic));
    return DemographicProfile::DemographicEnumCls.at((std::size_t)_demographic).toString(DemographicProfile::get(_profileID, _demographic));
}

const string *DemographicProfile::toString(ProfileID _profileID)
{
	assert(Utility::withinRange(_profileID, DemographicProfile::MIN, DemographicProfile::MAX));

	//make sure the internal dmgProfileID-related  fields are initiated before returning any information to the outside
	if(DemographicProfile::ProfileIDtoProfile.size() == 0)
	{
		DemographicProfile::initProfileIDMap();
	}

	return &DemographicProfile::ProfileIDtoStr.at(_profileID);
}

void DemographicProfile::initEnums()
{
	DemographicProfile::DemographicEnumCls.push_back(EnumCls<SexualActivityStatus>(enumStrs[(std::size_t)DemographicProfile::Demographic::SexualActivityStatus]));
    DemographicProfile::DemographicEnumCls.push_back(EnumCls<Gender>(enumStrs[(std::size_t)DemographicProfile::Demographic::Gender]));
    DemographicProfile::DemographicEnumCls.push_back(EnumCls<SexualOrientation>(enumStrs[(std::size_t)DemographicProfile::Demographic::SexualOrientation]));
    DemographicProfile::DemographicEnumCls.push_back(EnumCls<RelationshipStatus>(enumStrs[(std::size_t)DemographicProfile::Demographic::RelationshipStatus]));
    DemographicProfile::DemographicEnumCls.push_back(EnumCls<Employment>(enumStrs[(std::size_t)DemographicProfile::Demographic::Employment]));
}

void DemographicProfile::initProfileIDMap()
{
	if(DemographicProfile::DemographicEnumCls.size() == 0)
	{
		DemographicProfile::initEnums();
	}

	//see how many buckets there are and allocate space for them
	DemographicProfile::ProfileIDtoProfile.resize(DemographicProfile::TotalNumBuckets);
	DemographicProfile::ProfileIDtoStr.resize(DemographicProfile::TotalNumBuckets);
	//used to populate look-ups data structures with DemographicProfile
	DemographicProfile currDemographicProfile;
	//set currDemographicProfile to all min values. we can't use DemographicProfile.set(ProfileID) b/c we need to make it through this
	//  function before it is used
	DemographicProfile::Demographic currDemographic = DemographicProfile::Demographic(0);

	while(currDemographic < DemographicProfile::Demographic::Last)
	{
		currDemographicProfile.set(currDemographic, DemographicProfile::getEnumCls(currDemographic)->getMin());
        currDemographic = DemographicProfile::Demographic((std::size_t)currDemographic + 1);
	}

	//iterate through all possible ProfileID's and corresponding tuples
	for(ProfileID currProfileID = DemographicProfile::MIN; currProfileID <= DemographicProfile::MAX; currProfileID++)
	{
		//map DemographicProfile -> ProfileID
		DemographicProfile::ProfileToProfileID[currDemographicProfile] = currProfileID;
		//map ProfileID -> DemographicProfile
		DemographicProfile::ProfileIDtoProfile.at(currProfileID) = std::make_unique<const DemographicProfile>(currDemographicProfile);
		//generate the string representation of current profileID
		stringstream currEnumStr;

        for(Demographic category = DemographicProfile::Demographic((std::size_t)DemographicProfile::Demographic::Last - 1);
            category >= DemographicProfile::Demographic(0); category = DemographicProfile::Demographic((std::size_t)category - 1))
		{
			//Don't print out SA and HETERO (for now -- too redundant)
			if(category != DemographicProfile::Demographic::SexualActivityStatus)
			{
                currEnumStr << *(DemographicProfile::DemographicEnumCls.at((std::size_t)category).toString(currDemographicProfile.get(category)));

				if(category > DemographicProfile::Demographic(1))
				{
					currEnumStr << Constants::COLON;
				}
			}
		}

		//map ProfileID->string
		DemographicProfile::ProfileIDtoStr.at(currProfileID).append(currEnumStr.str());

		//if we are at the last profile, then don't increment anymore
		if(currProfileID == DemographicProfile::MAX)
		{
			break;
		}

		//get the next set of unique values for the next round in loop
		currDemographicProfile++;
	}
}

//--------< End fields and functions to change if enum Demographic values change >-------------------//

