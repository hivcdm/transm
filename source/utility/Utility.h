#pragma once

#include <set>
#include <string>
#include <vector>

#include "utility/RandomNumberGenerator.h"
#include "../core/Constants.h"

struct Version
{
	static Version FromString(const std::string &version_string);
	static std::string ToString(const Version &version);
	static int Compare(const Version &v1, const Version &v2, bool ignore_patch = false);

	int major = 0;
	int minor = 1;
	int patch = 0;
};

class Utility
{

	//usually we would divide to convert between these time increments
	//but division is more expensive, so multiply by inverse instead.
	static double dayToMonthMult;
	static double dayToYearMult;
	static double monthToYearMult;
public:
    /// <summary>
    /// Return a version object that represents the version of the running model.
    /// </summary>
    static Version get_model_version();

	//convert _val from one TimeGranularity to another
	static unsigned int convertTime(TimeGranularity _from, TimeGranularity _to, double _val);

	/**
	Converts a string value to another datatype
	**/
	template <class T>
	static T fromString(std::string _s);

	//returns true if _elem is a member of _set
	template <class T>
	static bool memberOf(std::set<T> _set, T _elem);

	//retursn true if _elem is a member of _vector
	template <class T>
	static bool memberOf(std::vector<T> _vector, T _elem);

	static void normalize(std::vector<double> &_weights);

	//converts a probability to a rate
	static double probToRate(double _prob);
	//converts a rate to a probability
	static double rateToProb(double _rate);

	static double computeCepacDiscountFactor(int month, double discount_rate);

	template <typename T>
	static T round(double d)
	{
		double decimals = d - floor(d);

		if(decimals >= 0.5)
		{
			return static_cast<T>(ceil(d));
		}
		else
		{
			return static_cast<T>(floor(d));
		}
	}

	static bool isNormDistZero(const NormalDist _normDist);
	/**
	returns true if _val is within [_min,_max]
	**/
	template <class T>
	static bool withinRange(T _val, T _min, T _max);

	//returns true if 0.0 <= _prob <= 1.0
	static bool validProbability(double _prob);

	//this function was taken from
	// http://www.oopweb.com/CPP/Documents/CPPHOWTO/Volume/C++Programming-HOWTO-7.html
	static void Tokenize(const std::string &str, std::vector<std::string> &tokens, const std::string &delimiters);
};

//returns true if _elem is a member of _set
template <class T>
bool Utility::memberOf(std::set<T> _set, T _elem)
{
	return (_set.find(_elem) != _set.end())	;
}

template <class T>
bool Utility::memberOf(std::vector<T> _vector, T _elem)
{
	for(int i = 0; i < _vector.size(); i++)
	{
		if(_vector.at(i) == _elem)
		{
			return true;
		}
	}

	return false;
}


/**
Converts a string value to another datatype
**/
template <class T>
T Utility::fromString(std::string _s)
{
	std::istringstream converter(_s);
	T converted;
	converter >> converted;
	return converted;
}

template <class T>
bool Utility::withinRange(T _val, T _min, T _max)
{
	return ((_min <= _val) && (_max >= _val));
}
