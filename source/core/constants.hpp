#pragma once

#include <map>
#include <string>

namespace transm {

class Constants
{
public:
	static const std::string Asterisk;
	static const std::string Blank;
	static const std::string Colon;
	static const std::string Underscore;
	static const std::string Tab;
	static const std::string Space;

	static const int NumberOfOIs = 15;

	static const int NumberConcurrencyDefs = 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2;
	static const int NumberCalibrationPrevs = 13;

	static const int InitialInfection = 0;
};

} // namespace transm
