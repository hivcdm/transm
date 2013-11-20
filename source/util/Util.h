#pragma once

#include "./rand/RandomNums.h"
#include "./../Constants.h"
#include <set>
#include <vector>
#include <string>
#include <sstream>
#include <assert.h>

#include <limits.h>

using namespace std;

class Util {

	//usually we would divide to convert between these time increments
	//but division is more expensive, so multiply by inverse instead.
	static double dayToMonthMult;
	static double dayToYearMult;
	static double monthToYearMult;
public:
	/* Constant values for transmission model version and file/directory information */
		static const double MODEL_VERSION;
		static const double INPUT_VERSION;

//Only have the "filesToRun" in Util for the console version... otherwise it is taken care of by DisplayBox
//#if defined (CONSOLE)
		static std::vector<std::string> transmFilesToRun;
		static void findInputFiles();
//#endif


	//convert _val from one TimeGranularity to another
	static unsigned int convertTime(TimeGranularity _from, TimeGranularity _to, double _val );

	/**
	Converts a string value to another datatype
	**/
	template <class T>
	static T fromString(string _s);

	/**
	Prints a prompt and exits after user hits return
	***/
	static void exitWithPrompt(int _exitCode);

	//returns the minimum possible value of certain primitive types
	template <class T>
	static T getMin();

	//returns the maximum possible value of certain primitive types
	template <class T>
	static T getMax();

	//returns true if _elem is a member of _set
	template <class T>
	static bool memberOf(set<T> _set, T _elem);

	//retursn true if _elem is a member of _vector
	template <class T>
	static bool memberOf(vector<T> _vector, T _elem);

	static void normalize( vector<double> &_weights);

	//converts a probability to a rate
	static double probToRate(double _prob);
	//converts a rate to a probability
	static double rateToProb(double _rate);
	/**
	rounds a double into a long
	**/
	static long round(double _d);

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
	static void Tokenize(const string& str, vector<string>& tokens, const string& delimiters);
};

//returns true if _elem is a member of _set
template <class T>
bool Util::memberOf(set<T> _set, T _elem) {
	return (_set.find(_elem) != _set.end())	;
}

template <class T>
bool Util::memberOf(vector<T> _vector, T _elem) {
	for(int i = 0; i < _vector.size(); i++) {
		if ( _vector.at(i) == _elem) return true;
	}
	return false;
}


//returns the minimum possible value of certain types
template <class T>
T Util::getMin() {
	if(typeid(T) == typeid(int))
		return INT_MIN;
	else if((typeid(T) == typeid(long)) || (typeid(T) == typeid(double)))
		return LONG_MIN;
	else if(typeid(T) == typeid(char))
		return CHAR_MIN;
	else if((typeid(T) == typeid(unsigned long)) || (typeid(T) == typeid(unsigned int)))
		return 0;
	else {
		cerr << "Util::getMin() - tried to pass in illegal type" << endl;
		assert(false);
		Util::exitWithPrompt(-1);
		return -1;
	}
}

//returns the maximum possible value of certain types
template <class T>
T Util::getMax() {
	if(typeid(T) == typeid(int))
		return INT_MAX;
	else if( (typeid(T) == typeid(long)) || (typeid(T) == typeid(double)) )
		return LONG_MAX;
	else if(typeid(T) == typeid(char))
		return CHAR_MAX;
	else if(typeid(T) == typeid(unsigned int))
		return UINT_MAX;
	else if(typeid(T) == typeid(unsigned long))
		return ULONG_MAX;
	else {
		cerr << "Util::getMax() - tried to pass in illegal type:" << endl;
		assert(false);
		Util::exitWithPrompt(-1);
		return -1;

	}
}

/**
Converts a string value to another datatype
**/
template <class T>
T Util::fromString(string _s) {
	std::istringstream converter(_s);
	T converted;
	converter >> converted;
	return converted;
}

template <class T>
bool Util::withinRange(T _val, T _min, T _max) {
	return (( _min <= _val) && (_max >= _val));
}
