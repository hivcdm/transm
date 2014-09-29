#pragma once

#include <array>
#include <fstream>
#include <string>
#include <vector>

/**
This class parses the parts of a CEPAC .in file that is needed to run the transmission model
It also stores the values that it has parsed and provides accessor functions
@author schung5
**/
class CepacInputParser
{
public :
	/**
	opens a CEPAC .in file for reading and reads desired model parameters
	@param _inputFile path to CEPAC file
	@author schung5
	**/
	CepacInputParser(const std::string &filename);

	/**
	from a CEPAC .in file stream, get the nonAIDS death for men and women
	@author schung5
	**/
	std::array<std::vector<double>, 2> parseNonAidsDeathProbabilities();

private:
	//fstream of a valid CEPAC .in file
	std::fstream inputStream_;
};