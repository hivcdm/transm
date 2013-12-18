#include "Enum.h"
#include "../util/Util.h"
#include "../Constants.h"


const string BaseEnumCls::WILDCARD = "*";

BaseEnumCls::BaseEnumCls() {
	initialized = false;
}

BaseEnumCls::BaseEnumCls(const std::string _strs[], unsigned int _numEnums) {
	this->init(_strs, _numEnums);
}

BaseEnumCls::~BaseEnumCls() {
}

void BaseEnumCls::appendEnumStr(std::ostream& _output, BaseEnumCls::Enum _e) const{
	assert(this->initialized);
	assert(this->isValidNonWildCard(_e));
	_output << this->strs.at(_e);
}

//gets lowest valid value as an int
BaseEnumCls::Enum BaseEnumCls::getMin() const {
	assert(this->initialized);
	return this->min;
}

//gets highest valid value as an int
BaseEnumCls::Enum BaseEnumCls::getMax() const{
	assert(this->initialized);
	return this->max;
}

BaseEnumCls::Enum BaseEnumCls::getWildcard() const {
	assert(this->initialized);
	return this->max + 1;
}

BaseEnumCls::Enum BaseEnumCls::fromString(const string& _str) const{
	assert(this->initialized);
	//search through all valid enums
	int currEnum = this->getMin();
	while(currEnum <= this->getMax()) {
		//if we find a match, then return the enum
		if(!this->strs.at(currEnum).compare(_str))
			return currEnum;
		currEnum++;
	}

	//if we have a wildcard value
	if(!_str.compare(BaseEnumCls::WILDCARD))
		return this->getWildcard();

	cerr << "Illegal string representation of enum (" << _str << ")";
	Util::exitWithPrompt(-1);

	return -1;
}

/**
Return the number of valid values for this Enum
**/
unsigned int BaseEnumCls::getNumEnums() const{
	assert(this->initialized);
	return numEnums;
}


/**
Stores the string representation of enum E
**/
void BaseEnumCls::init(const std::string _strs[], unsigned int _numEnums) {
	initialized = true;

	this->numEnums = _numEnums;

	this->min = 0;
	this->max = this->numEnums - 1;

	//save all the strings
	for(size_t i = 0; i < this->numEnums; i++)
	{
		std::string to_push = _strs[i];
		this->strs.push_back(to_push);
	}

	//make an extra space for wildcard
	this->strs.push_back("*");
}

bool BaseEnumCls::isValidEnum(BaseEnumCls::Enum _e) const {
	assert(this->initialized);
	return (this->isValidNonWildCard(_e) || (_e == this->getWildcard()));
}

bool BaseEnumCls::isValidNonWildCard(BaseEnumCls::Enum _e) const {
	assert(this->initialized);
	return Util::withinRange<BaseEnumCls::Enum>(_e, this->getMin(), this->getMax());
}

const string* BaseEnumCls::toString(BaseEnumCls::Enum _e) const{
	assert(this->initialized);
	assert(this->isValidEnum(_e));
	return &this->strs.at(_e);
}
