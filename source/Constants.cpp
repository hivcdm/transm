#include "Constants.h"

string const Constants::ASTERISK = "*";
string const Constants::BLANK = "";
string const Constants::COLON = ":";
string const Constants::TAB = "\t";
string const Constants::TABTAB = "\t\t";
string const Constants::SPACE = " ";
string const Constants::UNDERSCORE = "_";
string const Constants::BatchStatFileName[ENDBatchStatsVariables] = {"prevalence", "SAprevalence", "incidence", "populationSize", "numberInfected", "newInfections"};
bool const Constants::SHOULD_NOT_BE_CALLING_ME = false;
bool const Constants::TODO = false;
const bool Constants::TODO_DEF = true;
const bool Constants::TODO_LO_PRI = true;
int const Constants::PREVALENT_INFECTION = 0;
//bool const Constants::INCIDENT_INFECTION = false;
bool const Constants::REMOVE = true;
bool const Constants::DONT_REMOVE = false;
bool const Constants::CONSENT_IS_REQUIRED = true;
bool const Constants::CONSENT_NOT_REQUIRED = false;
bool const Constants::EXCLUDE_NULL_BINS = true;
bool const Constants::INCLUDE_NULL_BINS = false;
bool const Constants::SHOW_INFECTED = true;
bool const Constants::NO_SHOW_INFECTED = false;
bool const Constants::NEED_TO_DELETE = true;
bool const Constants::DO_NOT_DELETE = false;
