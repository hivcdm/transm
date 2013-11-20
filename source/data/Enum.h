#pragma once

#include <iostream>
#include <string>
#include <vector>
#include "../util/Util.h"
#include <assert.h>


/**
This function allows enums to use prefix add notation for enums
**/
template<class E>
E& operator++(E &_e)  {
  return _e = E(_e + 1);
}

template<class E>
E& operator--(E &_e)  {
  return _e = E(_e - 1);
}

/**
This class wraps enums to give additional functionality such as conversions to strings and bounds checking

Limitations :
1) enums must start at 0 and only increase in value by 1
2) uses unsigned int's instead of actual enum type

Right now it's a small class, but it may be helpful later if more enum-type functionality is needed

this is a class that all EnumCls<E> inherit from. This allows us to put objects of EnumCls<E> with different E in
the same container
@author schung5
**/
class BaseEnumCls {

public:

	typedef unsigned int Enum;
	
	enum NULL_ENUM {	};

	const static string WILDCARD;
/*
	class ExceptionBadEnum : public exception {

		Enum e;

		ExceptionBadEnum(Enum _e) {
			this->e = _e;
		}

		virtual const char* what() const throw() {
			ostringstream o
			return "Bad Enum 
		}	
	};
*/
protected:	
	//flag is set to true if this class has been initialized with string values
	bool initialized;

	//string representation of the enums
	std::vector<std::string> strs;		

	//kept for bounds checking
	Enum max;
	Enum min;

	//total number of available enums
	unsigned int numEnums;	

public :

	BaseEnumCls();

	BaseEnumCls(const char* _strs[], unsigned int _numEnums);

	~BaseEnumCls();

	void appendEnumStr(std::ostream& _output, Enum _e) const;

	/**
	Return the number of non-wildcard values for this EnumCls
	**/	
	unsigned int getNumEnums() const;

	//gets lowest non-wildcard value as an int
	Enum getMin() const;

	//gets highest non-wildcard value as an unsigned int
	Enum getMax() const;

	//gets wildcard as an unsigned int
	Enum getWildcard() const;

	/**
	Given a string, respresents to return the enum value that matches it
	If string does not match any value, returns this->getMax() + 1
	**/
	Enum fromString(const string& _str) const;

	/**
	Stores the string representation of enum
	**/	
	void init(const char** _strs, unsigned int _numEnums);

	/**
	Returns true if _e is in [max, min] or is the wildcard value
	***/
	bool isValidEnum(Enum _e) const;

	/**
	Returns true if _e is in [max, min] 
	***/
	bool isValidNonWildCard(Enum _e) const;

	/**
	Returns the string* representatin of _enum
	We return a pointer to save compute speed. We don't want a 
	new string to be allocated cor each call
	**/
	const string* toString(Enum _enum) const;
};

/**
Basically will return things as the actual enum type as opposed to unsigned int like BaseEnumCls
**/
template <class E>
class EnumCls : public BaseEnumCls {
	
public:			
	//makes this compatible with BaseEnumCls
	typedef E Enum;

	EnumCls();
	
	EnumCls(const char* _strs[], unsigned int _numEnums);

	/**
	Takes the string representation of _e and appends it to _output
	**/
	void appendEnumStr(std::ostream& _output, E& _e);	

	E getMin();		//lowest value of E		
	E getMax();		//highest value of E		


	/**
	Converts a string representation of an enumeraion to its enum value
	**/
	E toEnum(std::string _enumStr);

	/**
	returns _enum as 
	**/
	E toEnum(int _enum);
};

template <class E>
EnumCls<E>::EnumCls(){
	this->initialized = false;
}

template <class E>
EnumCls<E>::EnumCls(const char* _strs[], unsigned int _numEnums) {	
	this->init(_strs, _numEnums);
}
	
template <class E>
void EnumCls<E>::appendEnumStr(std::ostream& _output, E& _e) {	
	_output << this->toString(_e);
}

template <class E>
E EnumCls<E>::getMin() {
	return E(this->min);
}

template <class E>
E EnumCls<E>::getMax() {
	 return E(this->max);
}

template <class E>
E EnumCls<E>::toEnum(int _enum) {
	assert( _enum >= 0);
	assert( _enum <= this->numEnums);
	return E(_enum);
}

template <class E>
E EnumCls<E>::toEnum(std::string _enumStr) {
	//look through all enum strings to see if we have a match
	for(size_t i = 0; i < this->numEnums; i++) {
		if( _enumStr.compare( this->strs.at(i)) == 0)
			return E(i);
	}
	cerr << "Error: EnumCls " << _enumStr << " does not exist" << endl;
	return E(this->numEnums);
}
	

