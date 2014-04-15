#include <cassert>
#include <iostream>

#include "CepacInputParser.h"
#include "../core/Constants.h"
#include "../util/Utility.h"

CepacInputParser::CepacInputParser(const std::string &filename)
{
	inputStream_.open(filename.c_str(), std::ios::in);
}

std::array<std::vector<double>, 2> CepacInputParser::parseNonAidsDeathProbabilities()
{
	std::array<std::vector<double>, 2> probabilities;

	//if we tried to open the CEPAC file, just silently fail and use hardcoded defaults
	if(!inputStream_.fail())
	{
		std::vector<std::string> maleValues;		//holds the tokenized input of the probabilities of non-aids death
		std::vector<std::string> femaleValues;
		std::string currLine;				//contains the current line of the file we're looking at

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
		Utility::Tokenize(currLine, maleValues, Constants::TAB);
		std::getline(inputStream_, currLine);
		//currLine should now contain row for female non AIDS death probabilities
		Utility::Tokenize(currLine, femaleValues, Constants::TAB);

		//generate the non-aids death probabilitiy
		//we start the loop at 1 instead of 0 b/c first token contains a text label of the row. the probabilities start at index 1
		for(size_t i = 1; i < maleValues.size(); ++i)
		{
			probabilities[0].push_back(Utility::fromString<double>(maleValues.at(i)));
			probabilities[1].push_back(Utility::fromString<double>(femaleValues.at(i)));
		}
	}

	//we should really reset the pointer to the beginning of the file
	//also we need to check the correctness of this method
	assert(Constants::TODO_LO_PRI);

	return probabilities;
}
