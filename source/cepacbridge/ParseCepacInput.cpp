#pragma once

#include <string>
#include <fstream>
#include <vector>

#include "ParseCepacInput.h"
#include "../Constants.h"
#include "../util/Util.h"


using namespace std;

ParseCepacInput::ParseCepacInput(string _inputFile) {
	cepacInputFile.open ( _inputFile.c_str() ,ios::in);   
}

void ParseCepacInput::getNonAIDSDeath (vector<double> &_maleProbs ,vector<double> &_femaleProbs) {
	//if we tried to open the CEPAC file, just silently fail and use hardcoded defaults
	if(cepacInputFile.fail()) {
		return;
	}

	vector<string> maleValues;		//holds the tokenized input of the probabilities of non-aids death
	vector<string> femaleValues;	
	string currLine;				//contains the current line of the file we're looking at

	//go through CEPAC .in file until we find the right row
	do {
		getline(this->cepacInputFile, currLine);			
		if( this->cepacInputFile.eof()) {
			cerr << "CEPAC Input file did not contain non-AIDS death probability (NonAIDSDthProb_Male)";
			Util::exitWithPrompt(INVALID_CEPAC_FILE);
		}			
	} while ( currLine.find("NonAIDSDthProb_Male",0) == string::npos);
	
	//currLine should now contain row for male non AIDS death probabilities
	Util::Tokenize( currLine, maleValues, Constants::TAB);		
	getline(this->cepacInputFile, currLine);
	//currLine should now contain row for female non AIDS death probabilities
	Util::Tokenize( currLine, femaleValues, Constants::TAB);
		
	//generate the non-aids death probabilitiy
	//we start the loop at 1 instead of 0 b/c first token contains a text label of the row. the probabilities start at index 1
	for(int i = 1; i < maleValues.size(); ++i) {									
		_maleProbs.push_back(Util::fromString<double>(maleValues.at(i)));
		_femaleProbs.push_back(Util::fromString<double>(femaleValues.at(i)));
	}	

	//we should really reset the pointer to the beginning of the file	
	//also we need to check the correctness of this method
	assert(Constants::TODO_LO_PRI);
}