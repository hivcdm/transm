#include "./DmgProfile.h"
#include "../Person.h"
#include "./../../util/Util.h"
#include <typeinfo>
#include <sstream>

//declare strings of Enums
const std::string demographicStrs[DmgProfile::ENDDemographic] = {"SEXUAL_ACTIVITY_STATUS", "GENDER", "SEXUAL_ORIENTATION", "RELATIONSHIP_STATUS", "EMPLOYMENT"};
//fix this... not all dmgProfile will only have 2 choices
const std::string enumStrs[DmgProfile::ENDDemographic][2] =
{
	{"SA", "NA"},
	{"MALE", "FEMALE"},
	{"HETERO", "HOMO"},
	{"NON_SINGLE", "SINGLE"},
	{"NON_CSW", "CSW"}
};
DmgProfile::Demographic DmgProfile::MaxDemographic = DmgProfile::Demographic(DmgProfile::ENDDemographic - 1);

std::vector<BaseEnumCls> DmgProfile::DemographicEnumCls;
std::vector <const DmgProfile *> DmgProfile::ProfileIDtoProfile;
std::vector <string> DmgProfile::ProfileIDtoStr;
std::map<DmgProfile, DmgProfile::ProfileID, DmgProfile::less> DmgProfile::ProfileToProfileID;

//declare fields of class DmgProfile
DmgProfile::ProfileID NOT_UNIQUE = -1;
const DmgProfile::ProfileID DmgProfile::NOT_UNIQUE = DmgProfile::ProfileID(
            -1);	//used as a return value to getProfileID to signify that the current tuple of enums inside this class contain a wildcard
const DmgProfile::ProfileID DmgProfile::MIN = DmgProfile::ProfileID(0);
const DmgProfile::ProfileID DmgProfile::MAX = DmgProfile::TotalNumBuckets - 1;
const DmgProfile::ProfileID DmgProfile::END = DmgProfile::ProfileID((1 + ENDSexualActivityStatus) * (1 + ENDGender) *
        (1 + ENDSexualOrientation) * (1 + ENDRelationshipStatus) * (1 + ENDEmployment));

//----------------< Start Functions & fields for class DmgProfile >--------------------//

DmgProfile::DmgProfile()
{
	this->set(DmgProfile::END);
}

DmgProfile::DmgProfile(const DmgProfile &_dmgProfile)
{
	this->set(_dmgProfile);
}

DmgProfile::DmgProfile(ProfileID _profileID)
{
	assert((_profileID == DmgProfile::END) || Util::withinRange(_profileID, DmgProfile::MIN, DmgProfile::MAX));
	this->set(_profileID);
}

bool DmgProfile::operator==(const DmgProfile _a) const
{
	//i don't know why we can't use memcmp, but it seems to not be working correctly.
	//return (::memcmp( this->enums, _a.enums, DmgProfile::ENDDemographic) == 0);
	// it thinks that {1,0,0,0} is equal to {1,1,1,1}
	//check from left-most value for equality
	DmgProfile::Demographic currDemographic = DmgProfile::Demographic(0);

	while(currDemographic < DmgProfile::ENDDemographic)
	{
		if(this->enums[currDemographic] != _a.enums[currDemographic])
		{
			return false;
		}

		currDemographic = DmgProfile::Demographic(currDemographic + 1);
	}

	return true;
}

bool DmgProfile::operator!=(const DmgProfile _a) const
{
	return !(*this == _a);
}

bool DmgProfile::operator<(const DmgProfile _a) const
{
	if(*this == _a)
	{
		return false;
	}

	//check from left-most value for greater than
	DmgProfile::Demographic currDemographic = DmgProfile::MaxDemographic;
	DmgProfile::Demographic min = DmgProfile::Demographic(0);

	while(currDemographic >= min)
	{
		if(this->enums[currDemographic] > _a.enums[currDemographic])
		{
			return false;
		}

		if(this->enums[currDemographic] < _a.enums[currDemographic])
		{
			return true;
		}

		--currDemographic;
	}

	return true;
}

bool DmgProfile::operator<=(const DmgProfile _a) const
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

void DmgProfile::operator=(const DmgProfile _a)
{
	DmgProfile::Demographic currDemographic = DmgProfile::Demographic(0);

	//set the internal enum fields equal to _a
	while(currDemographic < DmgProfile::ENDDemographic)
	{
		this->set(currDemographic, _a.get(currDemographic));
	}
}

void DmgProfile::operator++(int)
{
	if(DmgProfile::DemographicEnumCls.size() == 0)
	{
		DmgProfile::initEnums();
	}

	//set an assert to check if we are at max value
	assert(this->getProfileID() != DmgProfile::MAX);
	assert(this->getProfileID() != DmgProfile::END);
	DmgProfile::Demographic currPlace = DmgProfile::MaxDemographic;
	DmgProfile::Demographic min = DmgProfile::Demographic(0);
	//add one to the right-most column
	this->set(currPlace, this->get(currPlace) + 1);
	//carry if needed
	bool carry = false;

	while(currPlace >= min)
	{
		if(carry)
		{
			this->set(currPlace, this->get(currPlace) + 1);
		}

		if(this->get(currPlace) > DmgProfile::getEnumCls(currPlace)->getMax())
		{
			carry = true;
			this->set(currPlace, DmgProfile::getEnumCls(currPlace)->getMin());
		}
		else
		{
			carry = false;
		}

		--currPlace;
	} //while(currPlace >= DmgProfile::DemographicCategoriesCls.getMin())  {
}

BaseEnumCls::Enum DmgProfile::get(DmgProfile::Demographic _demographic) const
{
	assert(Util::withinRange(_demographic, DmgProfile::Demographic(0), DmgProfile::MaxDemographic));
	return this->enums[_demographic];
}

DmgProfile::ProfileID DmgProfile::getProfileID() const
{
	//the map tupleToID stores tuple->id
	return DmgProfile::ProfileToProfileID[*this];
}


bool DmgProfile::match(const DmgProfile &_selector) const
{
	DmgProfile::Demographic currDemographic = DmgProfile::Demographic(0);

	//loop through each Demographic
	while(currDemographic <= DmgProfile::MaxDemographic)
	{
		//skip if wildcard
		if(_selector.get(currDemographic) == DmgProfile::DemographicEnumCls.at(currDemographic).getWildcard())
		{
			currDemographic = DmgProfile::Demographic(currDemographic + 1);
			continue;
		}

		//if enums don't match, then this tuple doesn't match
		if(_selector.get(currDemographic) != (this->enums[currDemographic]))
		{
			return false;
		}

		currDemographic = DmgProfile::Demographic(currDemographic + 1);
	}

	return true;
}

void DmgProfile::parse(string _tupleStr)
{
	if(DmgProfile::DemographicEnumCls.size() == 0)
	{
		DmgProfile::initEnums();
	}

	//we want to tokenize a string representatino of a tuple
	vector<string> tokens;
	//currently allowed delimiters are: "_"
	Util::Tokenize(_tupleStr, tokens, Constants::COLON);

	//make sure we have correct amount of tokens
	if(tokens.size() != DmgProfile::ENDDemographic)
	{
		cerr << "Tuple String " << _tupleStr << " is not valid" << endl;
		Util::exitWithPrompt(-1);
	}

	//encode each part of the string into an enum value
	DmgProfile::Demographic currDemographic = DmgProfile::Demographic(0);
	const BaseEnumCls *currCategoryCls;

	//loop through each Demographic
	while(currDemographic < DmgProfile::ENDDemographic)
	{
		//get the current Demographic helper enum
		currCategoryCls = DmgProfile::getEnumCls(currDemographic);
		//get the enum value from _tupleStr's tokens
		BaseEnumCls::Enum e = currCategoryCls->fromString(tokens.at(currDemographic));
		//if we get currCategoryCls->fromString(tokens.at(currDemographic)) to throw an exception, then we can use a better error msg
		//cerr << "Tuple String " << _tupleStr << " is not valid. (" << _tupleStr.at(currDemographic) << ")" << endl;
		this->set(currDemographic, e);
		currDemographic = DmgProfile::Demographic(currDemographic + 1);
	} //while(currDemographic < DmgProfile::ENDDemographic) {
}


void DmgProfile::print(ostream &_outStream, string _prefix) const
{
	_outStream << _prefix << DmgProfile::ProfileIDtoStr[this->getProfileID()];
}

void DmgProfile::saveState(ostream &_outStream)
{
	_outStream << "dmg:";
	_outStream << "[";

	for(int i = 0; i < this->ENDDemographic; i++)
	{
		_outStream << this->enums[i];

		if(i != this->ENDDemographic - 1)
		{
			_outStream << ",";
		}
	}

	_outStream << "]," << endl;
}
void DmgProfile::selectProfileIDs(std::vector<ProfileID> &_selected, const std::vector<ProfileID> *_available) const
{
	if(DmgProfile::ProfileIDtoProfile.size() == 0)
	{
		DmgProfile::initProfileIDMap();
	}

	size_t i = 0;

	if(_available)
	{
		//loop through subset of all possible ProfileID's
		while(i < _available->size())
		{
			//if the profile that correponds to the ProfileID is matched by this(which is the selector here)
			//  then add the ProfileID to the resulting vector
			if(DmgProfile::ProfileIDtoProfile.at(_available->at(i))->match(*this))
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
			if(DmgProfile::ProfileIDtoProfile.at(i)->match(*this))
			{
				_selected.push_back(i);
			}

			i++;
		}
		while(i <= DmgProfile::MAX);
	}//if(_available) {
}

void DmgProfile::set(DmgProfile::Demographic _demographic, BaseEnumCls::Enum _enum)
{
	if(DmgProfile::DemographicEnumCls.size() == 0)
	{
		DmgProfile::initEnums();
	}

	assert(Util::withinRange(_demographic, DmgProfile::Demographic(0), DmgProfile::MaxDemographic));
	assert(DmgProfile::getEnumCls(_demographic)->isValidNonWildCard(_enum)
	       || (_enum == DmgProfile::getEnumCls(_demographic)->getWildcard()));
	this->enums[_demographic] = _enum;
}


void DmgProfile::set(DmgProfile::ProfileID _profileID)
{
	assert((_profileID == DmgProfile::END) || Util::withinRange(_profileID, DmgProfile::MIN, DmgProfile::MAX));

	if(DmgProfile::DemographicEnumCls.size() == 0)
	{
		DmgProfile::initEnums();
	}

	if(DmgProfile::ProfileIDtoProfile.size() == 0)
	{
		DmgProfile::initProfileIDMap();
	}

	if(_profileID == DmgProfile::END)
	{
		//set each value of Tuple to wildcard
		DmgProfile::Demographic currDemographic = DmgProfile::Demographic(0);

		while(currDemographic < DmgProfile::ENDDemographic)
		{
			this->set(currDemographic, DmgProfile::DemographicEnumCls.at(currDemographic).getNumEnums());
			currDemographic = DmgProfile::Demographic(currDemographic + 1);
		}//while(currDemographic < DmgProfile::ENDDemographic) {
	}
	else
	{
		//look up the tuple that matches this ProfileID and internalize values
		if(DmgProfile::ProfileIDtoProfile.size() == 0)
		{
			DmgProfile::initProfileIDMap();
		}

		this->set(*(this->ProfileIDtoProfile.at(_profileID)));
	}
}

void DmgProfile::set(const DmgProfile &_dmgProfile)
{
	DmgProfile::Demographic currDemographic = DmgProfile::Demographic(0);

	while(currDemographic < DmgProfile::ENDDemographic)
	{
		this->set(currDemographic, _dmgProfile.get(currDemographic));
		currDemographic = DmgProfile::Demographic(currDemographic + 1);
	}
}

const string *DmgProfile::toString() const
{
	return &DmgProfile::ProfileIDtoStr[this->getProfileID()];
}

//----------------< End Functions for class DmgProfile >--------------------//


//--------------------< Begin Static Methods >-----------------------//

const BaseEnumCls::Enum DmgProfile::get(ProfileID _profileID, Demographic _demographic)
{
	assert(Util::withinRange(_profileID, DmgProfile::MIN, DmgProfile::MAX));
	assert(Util::withinRange(_demographic, DmgProfile::Demographic(0), DmgProfile::MaxDemographic));
	return DmgProfile::ProfileIDtoProfile.at(_profileID)->get(_demographic);
}

const BaseEnumCls *DmgProfile::getEnumCls(DmgProfile::Demographic _demographic)
{
	assert(Util::withinRange(_demographic, DmgProfile::Demographic(0), DmgProfile::MaxDemographic));

	if(DmgProfile::DemographicEnumCls.size() == 0)
	{
		DmgProfile::initEnums();
	}

	return &(DmgProfile::DemographicEnumCls.at(_demographic));
}

const string *DmgProfile::getString(ProfileID _profileID, Demographic _demographic)
{
	assert(Util::withinRange(_profileID, DmgProfile::MIN, DmgProfile::MAX));
	assert(Util::withinRange(_demographic, DmgProfile::Demographic(0), DmgProfile::MaxDemographic));
	return DmgProfile::DemographicEnumCls.at(_demographic).toString(DmgProfile::get(_profileID, _demographic));
}

const string *DmgProfile::toString(ProfileID _profileID)
{
	assert(Util::withinRange(_profileID, DmgProfile::MIN, DmgProfile::MAX));

	//make sure the internal dmgProfileID-related  fields are initiated before returning any information to the outside
	if(DmgProfile::ProfileIDtoProfile.size() == 0)
	{
		DmgProfile::initProfileIDMap();
	}

	return &DmgProfile::ProfileIDtoStr.at(_profileID);
}

//--------------------< End Static Methods >-----------------------//

//--------< Start fields and functions to change if enum Demographic values change >-------------------//


void DmgProfile::deallocStaticMembers()
{
	for(size_t i = 0; i < DmgProfile::ProfileIDtoProfile.size(); ++i)
	{
		delete DmgProfile::ProfileIDtoProfile.at(i);
		DmgProfile::ProfileIDtoProfile.at(i) = nullptr;
	}

	DmgProfile::ProfileToProfileID.clear();
	DmgProfile::ProfileIDtoProfile.clear();
	DmgProfile::ProfileIDtoStr.clear();
	DmgProfile::DemographicEnumCls.clear();
}

void DmgProfile::initEnums()
{
	DmgProfile::DemographicEnumCls.push_back(EnumCls<SexualActivityStatus>(enumStrs[SEXUAL_ACTIVITY_STATUS],
	        ENDSexualActivityStatus));
	DmgProfile::DemographicEnumCls.push_back(EnumCls<Gender>(enumStrs[GENDER], ENDGender));
	DmgProfile::DemographicEnumCls.push_back(EnumCls<SexualOrientation>(enumStrs[SEXUAL_ORIENTATION],
	        ENDSexualOrientation));
	DmgProfile::DemographicEnumCls.push_back(EnumCls<RelationshipStatus>(enumStrs[RELATIONSHIP_STATUS],
	        ENDRelationshipStatus));
	DmgProfile::DemographicEnumCls.push_back(EnumCls<Employment>(enumStrs[EMPLOYMENT], ENDEmployment));
}

void DmgProfile::initProfileIDMap()
{
	if(DmgProfile::DemographicEnumCls.size() == 0)
	{
		DmgProfile::initEnums();
	}

	//see how many buckets there are and allocate space for them
	DmgProfile::ProfileIDtoProfile.resize(DmgProfile::TotalNumBuckets);
	DmgProfile::ProfileIDtoStr.resize(DmgProfile::TotalNumBuckets);
	//used to populate look-ups data structures with DemographicProfile
	DmgProfile currDmgProfile;
	//set currDmgProfile to all min values. we can't use DmgProfile.set(ProfileID) b/c we need to make it through this
	//  function before it is used
	DmgProfile::Demographic currDemographic = DmgProfile::Demographic(0);

	while(currDemographic < DmgProfile::ENDDemographic)
	{
		currDmgProfile.set(currDemographic, DmgProfile::getEnumCls(currDemographic)->getMin());
		currDemographic = DmgProfile::Demographic(currDemographic + 1);
	}

	//iterate through all possible ProfileID's and corresponding tuples
	for(ProfileID currProfileID = DmgProfile::MIN; currProfileID <= DmgProfile::MAX; currProfileID++)
	{
		//map DemographicProfile -> ProfileID
		DmgProfile::ProfileToProfileID[currDmgProfile] = currProfileID;
		//map ProfileID -> DemographicProfile
		DmgProfile::ProfileIDtoProfile.at(currProfileID) = new DmgProfile(currDmgProfile);
		//generate the string representation of current profileID
		stringstream currEnumStr;

		for(Demographic category = DmgProfile::Demographic(DmgProfile::ENDDemographic - 1);
		        category >= DmgProfile::Demographic(0); category = DmgProfile::Demographic(category - 1))
		{
			//Don't print out SA and HETERO (for now -- too redundant)
			if(category != DmgProfile::SEXUAL_ACTIVITY_STATUS && category != DmgProfile::SEXUAL_ORIENTATION)
			{
				currEnumStr << *(DmgProfile::DemographicEnumCls.at(category).toString(currDmgProfile.get(category)));

				if(category > DmgProfile::Demographic(1))
				{
					currEnumStr << Constants::COLON;
				}
			}
		}

		//map ProfileID->string
		DmgProfile::ProfileIDtoStr.at(currProfileID).append(currEnumStr.str());

		//if we are at the last profile, then don't increment anymore
		if(currProfileID == DmgProfile::MAX)
		{
			break;
		}

		//get the next set of unique values for the next round in loop
		currDmgProfile++;
	}
}

//--------< End fields and functions to change if enum Demographic values change >-------------------//

