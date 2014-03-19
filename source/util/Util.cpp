#include <iostream>
#include <typeinfo>
#include "Util.h"
#include <boost/filesystem.hpp>

Version Version::FromString(const std::string &version_string)
{
	Version v;
	auto major_minor_separator = version_string.find('.');
	v.major = std::stoi(version_string.substr(0, major_minor_separator));
	auto revision_separator = version_string.find('.', major_minor_separator + 1);
	v.minor = std::stoi(version_string.substr(major_minor_separator + 1, revision_separator));
	if(revision_separator != std::string::npos)
	{
		v.revision = std::stoi(version_string.substr(revision_separator + 1));
	}
	return v;
}

std::string Version::ToString(const Version &version)
{
	return std::to_string(version.major) + "." + std::to_string(version.minor) + "." + std::to_string(version.revision);
}

const Version Util::MODEL_VERSION = {3, 4, 0};

double Util::dayToMonthMult = 1.0 / 30;
double Util::dayToYearMult = 1.0 / 365;
double Util::monthToYearMult = 1.0 / 12;

std::vector<std::string> Util::transmFilesToRun;

void Util::findInputFiles(const std::string &inputDirectory)
{
	boost::filesystem::path directoryPath(inputDirectory);
	boost::filesystem::directory_iterator end_iter;

	if(boost::filesystem::exists(directoryPath) && boost::filesystem::is_directory(directoryPath))
	{
		for(boost::filesystem::directory_iterator dir_iter(directoryPath); dir_iter != end_iter; ++dir_iter)
		{
			if(boost::filesystem::is_regular_file(dir_iter->status()) && dir_iter->path().extension() == ".xml")
			{
                auto stem = dir_iter->path().stem().string();
                auto suffixPosition = stem.rfind("_seq");
                bool valid = false;
                
                if(suffixPosition != std::string::npos)
                {
                    size_t sequenceIndexDigits = stem[suffixPosition + 4] == '0' ? 1 : 2;
                    if(sequenceIndexDigits > 0 && sequenceIndexDigits <= 2)
                    {
                        std::string sequenceIndexString(stem.begin() + suffixPosition + 6 - sequenceIndexDigits, stem.end());
                        try
                        {
                            auto sequenceIndex = std::stoi(sequenceIndexString);
                            valid = sequenceIndex == 1;
                        }
                        catch (std::exception)
                        {
                            
                        }
                    }
                }
                else
                {
                    valid = true;
                }
                
                if(valid)
                {
                    transmFilesToRun.push_back(dir_iter->path().string());
                }
			}
		}
	}
}

//convert _val from one TimeGranularity to another
unsigned int Util::convertTime(TimeGranularity _from, TimeGranularity _to, double _val)
{
	assert(_val >= 0);
	unsigned int converted_value = 0;

	if(_from == DAY && _to == DAY)
	{
		converted_value = (int)_val;
	}
	else if(_from == DAY && _to == MONTH)
	{
		converted_value = (int)floor(_val * Util::dayToMonthMult);
	}
	else if(_from == DAY && _to == YEAR)
	{
		converted_value = (int)floor(_val * Util::dayToYearMult);
	}
	else if(_from == MONTH && _to == DAY)
	{
		converted_value = (int)_val * 30;
	}
	else if(_from == MONTH && _to == MONTH)
	{
		converted_value = (int)_val;
	}
	else if(_from == MONTH && _to == YEAR)
	{
		converted_value = (int)floor(_val * Util::monthToYearMult);
	}
	else if(_from == YEAR && _to == DAY)
	{
		converted_value = (int)_val * 365;
	}
	else if(_from == YEAR && _to == MONTH)
	{
		converted_value = (int)_val * 12;
	}
	else if(_from == YEAR && _to == YEAR)
	{
		converted_value = (int)_val;
	}
	else
	{
		std::cerr << "convertToTime - invalid time granularity..." << _from << " or " << _to << std::endl;
		Util::exitWithPrompt(INVALID_TIME_GRANULARITY);
	}

	return converted_value;
}


void Util::exitWithPrompt(int _exitCode)
{
	std::cerr << std::endl << std::endl << "Press any key to continue";
	std::string x;
	std::getline(std::cin, x);
	exit(_exitCode);
}

bool Util::isNormDistZero(const NormalDist _normDist)
{
	return (_normDist.mean == 0) && (_normDist.stddev == 0);
}

void Util::normalize(std::vector<double> &_weights)
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
double Util::probToRate(double _prob)
{
	assert(Util::withinRange<double>(_prob, 0.0, 1.0));
	return -log(1 - _prob);
}

//converts a rate to a probability
double Util::rateToProb(double _rate)
{
	//convert the cumulative rate back into a probability
	return 1 - exp(-_rate);
}

//this function was taken from
// http://www.oopweb.com/CPP/Documents/CPPHOWTO/Volume/C++Programming-HOWTO-7.html
void Util::Tokenize(const std::string &str,
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


bool Util::validProbability(double _prob)
{
	return Util::withinRange(_prob, 0.0, 1.0);
}
