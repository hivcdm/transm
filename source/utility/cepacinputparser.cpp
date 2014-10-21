#include <cassert>
#include <iostream>

#include "cepacinputparser.hpp"
#include "core/constants.hpp"
#include "utility/utility.hpp"

namespace transm {

CepacInputParser::CepacInputParser(const std::string &filename)
{
	inputStream_.open(filename.c_str(), std::ios::in);
}

//TODO:we need to check the correctness of this method
std::array<std::vector<double>, 2> CepacInputParser::parseNonAidsDeathProbabilities()
{
	std::array<std::vector<double>, 2> probabilities;

	//if we tried to open the CEPAC file, just silently fail and use hardcoded defaults
	if(!inputStream_.fail())
	{
        //contains the current line of the file we're looking at
        std::string currLine;

		//go through CEPAC .in file until we find the right row
		do
		{
			std::getline(inputStream_, currLine);

			if(inputStream_.eof())
			{
				throw std::runtime_error("CEPAC Input file did not contain non-AIDS death probability (NonAIDSDthProb_Male)");
			}
		} while(currLine.find("NonAIDSDthProb_Male", 0) == std::string::npos);

		//currLine should now contain row for male non AIDS death probabilities
		auto male_values = Utility::tokenize(currLine, Constants::TAB);

		std::getline(inputStream_, currLine);

		//currLine should now contain row for female non AIDS death probabilities
		auto female_values = Utility::tokenize(currLine, Constants::TAB);

		//generate the non-aids death probabilitiy
		//we start the loop at 1 instead of 0 b/c first token contains a text label of the row. the probabilities start at index 1
		for(std::size_t i = 1; i < male_values.size(); ++i)
		{
			probabilities[0].push_back(Utility::from_string<double>(male_values.at(i)));
			probabilities[1].push_back(Utility::from_string<double>(female_values.at(i)));
		}
	}

	//TODO:we should really reset the pointer to the beginning of the file

	return probabilities;
}

} // namespace transm
