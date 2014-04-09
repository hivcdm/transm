#include "Enum.h"
#include "../util/Util.h"
#include "../Constants.h"


const std::string BaseEnumCls::WILDCARD = "*";

BaseEnumCls::BaseEnumCls()
{
	initialized = false;
}

BaseEnumCls::BaseEnumCls(const std::vector<std::string> _strs)
{
	init(_strs);
}

BaseEnumCls::~BaseEnumCls()
{
}

void BaseEnumCls::appendEnumStr(std::ostream &_output, BaseEnumCls::Enum _e) const
{
	assert(initialized);
	assert(isValidNonWildCard(_e));
	_output << strs.at(_e);
}

//gets lowest valid value as an int
BaseEnumCls::Enum BaseEnumCls::getMin() const
{
	assert(initialized);
	return min;
}

//gets highest valid value as an int
BaseEnumCls::Enum BaseEnumCls::getMax() const
{
	assert(initialized);
	return max;
}

BaseEnumCls::Enum BaseEnumCls::getWildcard() const
{
	assert(initialized);
	return max + 1;
}

BaseEnumCls::Enum BaseEnumCls::fromString(const std::string &_str) const
{
	assert(initialized);
	BaseEnumCls::Enum currEnum = getMin();

	while(currEnum <= getMax())
	{
		if(!strs.at(currEnum).compare(_str))
		{
			return currEnum;
		}

		currEnum++;
	}

	if(!_str.compare(BaseEnumCls::WILDCARD))
	{
		return getWildcard();
	}
	else
	{
		throw std::runtime_error("Illegal string representation of enum (" + _str + ")");
	}
}

/**
Return the number of valid values for this Enum
**/
unsigned int BaseEnumCls::getNumEnums() const
{
	assert(initialized);
	return numEnums;
}


/**
Stores the string representation of enum E
**/
void BaseEnumCls::init(const std::vector<std::string> _strs)
{
	initialized = true;
	numEnums = _strs.size();
	min = 0;
	max = numEnums - 1;

	//save all the strings
	for(size_t i = 0; i < numEnums; i++)
	{
		std::string to_push = _strs[i];
		strs.push_back(to_push);
	}

	//make an extra space for wildcard
	strs.push_back("*");
}

bool BaseEnumCls::isValidEnum(BaseEnumCls::Enum _e) const
{
	assert(initialized);
	return (isValidNonWildCard(_e) || (_e == getWildcard()));
}

bool BaseEnumCls::isValidNonWildCard(BaseEnumCls::Enum _e) const
{
	assert(initialized);
	return Util::withinRange<BaseEnumCls::Enum>(_e, getMin(), getMax());
}

const std::string *BaseEnumCls::toString(BaseEnumCls::Enum _e) const
{
	assert(initialized);
	assert(isValidEnum(_e));
	return &strs.at(_e);
}
