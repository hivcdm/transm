#include <iostream>
#include <typeinfo>
#include "Util.h"
/* Include platform specific header files */
#if defined(_LINUX)
#include <sys/io.h>
#endif
#if defined(_WIN32)
#include <io.h>
#include <direct.h>	//for _mkdir and _CHDIR
#else
#include <sys/types.h>
#include <sys/stat.h>
//used to find files in a directory
#include <glob.h>
#endif

const double Util::MODEL_VERSION = 3.33;
const double Util::INPUT_VERSION = 3.33;

double Util::dayToMonthMult = 1.0/30;
double Util::dayToYearMult = 1.0/365;
double Util::monthToYearMult = 1.0/12;

std::vector<std::string> Util::transmFilesToRun;

#if defined ( CONSOLE )
void Util::findInputFiles(){
#if defined(_WIN32)
    long hFile;
    struct _finddata_t tFileInfo;
    hFile = _findfirst( "*.xml", &tFileInfo );
    int nInputFiles = 0;
    string fileName;

    //get the list of files that we have to process
    transmFilesToRun.clear();
    do {
	fileName = (char *) tFileInfo.name;
	transmFilesToRun.push_back(fileName);
	nInputFiles++;
    } while ( _findnext ( hFile, &tFileInfo ) == 0 );
    _findclose( hFile );
#else
    glob_t files;
    glob("*.xml", GLOB_ERR, NULL, &files);
    int nInputFiles = 0;
    string fileName;

    transmFilesToRun.clear();
    //get the list of files that we have to process
    int i;
    for( i = 0; i < files.gl_pathc; i++) {
	fileName = (char *) files.gl_pathv[i];
	size_t fileNameLength=fileName.length();
	size_t seqFirstStartIndex=fileName.rfind("_seq01");
	size_t seqStartIndex=fileName.rfind("_seq");
	if(seqStartIndex==(fileNameLength-10)){
	    if(seqFirstStartIndex==string::npos){
		continue;
	    }
	}
	transmFilesToRun.push_back(fileName);
	++nInputFiles;
    }
    globfree( &files);
#endif
}
#endif


//convert _val from one TimeGranularity to another
unsigned int Util::convertTime(TimeGranularity _from, TimeGranularity _to, double _val )
{
    assert (_val >= 0);

	unsigned int converted_value = 0;

	if(_from == DAY && _to == DAY)
	{
		converted_value = (int)_val;
	}
	else if(_from == DAY && _to == MONTH)
	{
		converted_value = (int)floor(_val*Util::dayToMonthMult);
	}
	else if(_from == DAY && _to == YEAR)
	{
		converted_value = (int)floor(_val*Util::dayToYearMult);
	}
	else if(_from == MONTH && _to == DAY)
	{
		converted_value = (int)_val*30;
	}
	else if(_from == MONTH && _to == MONTH)
	{
		converted_value = (int)_val;
	}
	else if(_from == MONTH && _to == YEAR)
	{
		converted_value = (int)floor(_val*Util::monthToYearMult);
	}
	else if(_from == YEAR && _to == DAY)
	{
		converted_value = (int)_val*365;
	}
	else if(_from == YEAR && _to == MONTH)
	{
		converted_value = (int)_val*12;
	}
	else if(_from == YEAR && _to == YEAR)
	{
		converted_value = (int)_val;
	}
	else
	{
		cerr << "convertToTime - invalid time granularity..." << _from << " or " << _to << endl;
		Util::exitWithPrompt(INVALID_TIME_GRANULARITY);
	}

	return converted_value;
}


void Util::exitWithPrompt(int _exitCode) {
    cerr << endl << endl << "Press any key to continue";
    string x;
    getline(cin,x);
    exit(_exitCode);
}

bool Util::isNormDistZero(const NormalDist _normDist) {
    return (_normDist.mean == 0) && (_normDist.stddev == 0);
}

void Util::normalize( vector<double> &_weights) {
    double total = 0;

    //see what the values currently total to
    for(unsigned int i = 0; i < _weights.size(); i++)
	total += _weights.at(i);

    //we want to divide by the total, but division is slow. so we will multiply by the inverse
    total = 1 / total;

    //normalize each proportionate value so that the sum of them ~ 1
    for(unsigned int i = 0; i < _weights.size(); i++)
	_weights.at(i) = _weights.at(i) * total;
}

//converts a probability to a rate
double Util::probToRate(double _prob){
    assert(Util::withinRange<double>(_prob, 0.0,1.0));
    return -log(1 - _prob);
}

//converts a rate to a probability
double Util::rateToProb(double _rate){
    //convert the cumulative rate back into a probability
    return 1 - exp(-_rate);
}

/**
   rounds a double into a long
**/
long Util::round(double _d) {
    double decimals = _d - floor(_d);

    if ( decimals >= 0.5)
	return (long)ceil(_d);
    else
	return (long)floor(_d);
}

//this function was taken from
// http://www.oopweb.com/CPP/Documents/CPPHOWTO/Volume/C++Programming-HOWTO-7.html
void Util::Tokenize(const string& str,
		    vector<string>& tokens,
		    const string& delimiters = " ") {
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos){
	// Found a token, add it to the vector.
	tokens.push_back(str.substr(lastPos, pos - lastPos));
	// Skip delimiters.  Note the "not_of"
	lastPos = str.find_first_not_of(delimiters, pos);
	// Find next "non-delimiter"
	pos = str.find_first_of(delimiters, lastPos);
    }
}


bool Util::validProbability(double _prob) {
    return Util::withinRange(_prob,0.0,1.0);
}
