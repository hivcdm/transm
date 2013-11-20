#pragma once

#include <vector>
#include <string>
#include <fstream>

using namespace std;

/**
This class parses the parts of a CEPAC .in file that is needed to run the transmission model
It also stores the values that it has parsed and provides accessor functions
@author schung5
**/
class ParseCepacInput {

	//fstream of a valid CEPAC .in file
	fstream cepacInputFile;

public :
	/**
	opens a CEPAC .in file for reading and reads desired model parameters
	@param _inputFile path to CEPAC file
	@author schung5
	**/
	ParseCepacInput(string _inputFile);

	/**
	from a CEPAC .in file stream, get the nonAIDS death for men and women
	@author schung5
	**/
	void getNonAIDSDeath (vector<double> &_maleProbs ,vector<double> &_femaleProbs);			
};