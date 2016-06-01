#include <fstream>
#include <iostream>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShlObj.h>
#include <tchar.h>
#else
#include <unistd.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include "Utility.h"
#include "filesystem.h"

namespace {
std::string to_string(TimeGranularity granularity)
{
	switch(granularity)
	{
	case TimeGranularity::Day: return "day";
	case TimeGranularity::Month: return "month";
	case TimeGranularity::Year: return "year";
	default: throw std::runtime_error("bad granularity");
	}
}
}

Version Version::FromString(const std::string &version_string)
{
	Version v;
	auto major_minor_separator = version_string.find('.');
	v.major = std::stoi(version_string.substr(0, major_minor_separator));
	auto revision_separator = version_string.find('.', major_minor_separator + 1);
	v.minor = std::stoi(version_string.substr(major_minor_separator + 1, revision_separator));
	if(revision_separator != std::string::npos)
	{
		v.patch = std::stoi(version_string.substr(revision_separator + 1));
	}
	return v;
}

std::string Version::ToString(const Version &version)
{
	return std::to_string(version.major) + "." + std::to_string(version.minor) + "." + std::to_string(version.patch);
}

int Version::Compare(const Version &v1, const Version &v2, bool ignore_patch)
{
	if(v1.major != v2.major)
	{
		return v1.major - v2.major;
	}

	if(v1.minor != v2.minor)
	{
		return v1.minor - v2.minor;
	}

	if(!ignore_patch && v1.patch != v2.patch)
	{
		return v1.patch - v2.patch;
	}

	return 0;
}

double Utility::dayToMonthMult = 1.0 / 30;
double Utility::dayToYearMult = 1.0 / 365;
double Utility::monthToYearMult = 1.0 / 12;

std::string get_executable_name()
{
#ifdef __APPLE__
    std::array<char, 1024> path;
    uint32_t size = static_cast<uint32_t>(path.size());

    if (_NSGetExecutablePath(path.data(), &size) == 0)
    {
        std::string executable_string(path.begin(), std::find(path.begin(), path.end(), '\0'));
	transm::path executable_path(executable_string);
        return executable_path.filename().string();
    }

    throw std::runtime_error("buffer too small, " + std::to_string(path.size()) + ", should be: " + std::to_string(size));
#elif defined(_WIN32)
    std::array<TCHAR, MAX_PATH> buffer;
    DWORD result = GetModuleFileName(nullptr, buffer.data(), (DWORD)buffer.size());

    if (result == 0 || result == buffer.size())
    {
        throw std::runtime_error("GetModuleFileName failed or buffer was too small");
    }

    auto full_string = std::string(buffer.begin(), buffer.begin() + result);
    return transm::path(full_string).stem().string();
#else
    char arg1[20];
    char exepath[PATH_MAX + 1] = { 0 };

    sprintf(arg1, "/proc/%d/exe", getpid());
    readlink(arg1, exepath, 1024);
    std::string full_path(exepath);
    return full_path.substr(full_path.find_last_of('/'));
#endif
}

Version Utility::get_model_version()
{
//	I am leaving this horror here for posterity. GA
//  auto exe_name = get_executable_name();
//  auto hyphen_index = exe_name.find_last_of('-');
//  assert(hyphen_index != std::string::npos);
//  auto version_string = exe_name.substr(hyphen_index + 2);
//  return Version::FromString(version_string);

    return Version::FromString(Constants::VERSION);
}

//convert _val from one TimeGranularity to another
unsigned int Utility::convertTime(TimeGranularity _from, TimeGranularity _to, double _val)
{
	assert(_val >= 0);
	unsigned int converted_value = 0;

    if(_from == TimeGranularity::Day && _to == TimeGranularity::Day)
	{
		converted_value = (int)_val;
	}
    else if(_from == TimeGranularity::Day && _to == TimeGranularity::Month)
	{
		converted_value = (int)floor(_val * Utility::dayToMonthMult);
	}
    else if(_from == TimeGranularity::Day && _to == TimeGranularity::Year)
	{
		converted_value = (int)floor(_val * Utility::dayToYearMult);
	}
    else if(_from == TimeGranularity::Month && _to == TimeGranularity::Day)
	{
		converted_value = (int)_val * 30;
	}
    else if(_from == TimeGranularity::Month && _to == TimeGranularity::Month)
	{
		converted_value = (int)_val;
	}
    else if(_from == TimeGranularity::Month && _to == TimeGranularity::Year)
	{
		converted_value = (int)floor(_val * Utility::monthToYearMult);
	}
    else if(_from == TimeGranularity::Year && _to == TimeGranularity::Day)
	{
		converted_value = (int)_val * 365;
	}
    else if(_from == TimeGranularity::Year && _to == TimeGranularity::Month)
	{
		converted_value = (int)_val * 12;
	}
    else if(_from == TimeGranularity::Year && _to == TimeGranularity::Year)
	{
		converted_value = (int)_val;
	}
	else
	{
		throw std::runtime_error("convertToTime - invalid time granularity..." + to_string(_from) + " or " + to_string(_to));
	}

	return converted_value;
}

bool Utility::isNormDistZero(const NormalDist _normDist)
{
	return (_normDist.mean == 0) && (_normDist.stddev == 0);
}

void Utility::normalize(std::vector<double> &_weights)
{
	double total = 0;

	//see what the values currently total to
	for(unsigned int i = 0; i < _weights.size(); i++)
	{
		total += _weights.at(i);
	}

	//we want to divide by the total, but division is slow. so we will multiply by the inverse
	total = 1 / total;

	//normalize each proportionate value so that the sum of them ~ 1
	for(unsigned int i = 0; i < _weights.size(); i++)
	{
		_weights.at(i) = _weights.at(i) * total;
	}
}

//converts a probability to a rate
double Utility::probToRate(double _prob)
{
	assert(Utility::withinRange<double>(_prob, 0.0, 1.0));
	return -log(1 - _prob);
}

//converts a rate to a probability
double Utility::rateToProb(double _rate)
{
	//convert the cumulative rate back into a probability
	return 1 - exp(-_rate);
}

double Utility::computeCepacDiscountFactor(int month, double discount_rate)
{
	return 1 / std::pow(discount_rate, month);
}

//this function was taken from
// http://www.oopweb.com/CPP/Documents/CPPHOWTO/Volume/C++Programming-HOWTO-7.html
void Utility::Tokenize(const std::string &str,
                    std::vector<std::string> &tokens,
                    const std::string &delimiters = " ")
{
	// Skip delimiters at beginning.
	std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	std::string::size_type pos = str.find_first_of(delimiters, lastPos);

	while(std::string::npos != pos || std::string::npos != lastPos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}
}


bool Utility::validProbability(double _prob)
{
	return Utility::withinRange(_prob, 0.0, 1.0);
}
