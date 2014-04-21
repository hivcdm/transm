#include "Constants.h"

std::string const Constants::ASTERISK = "*";
std::string const Constants::BLANK = "";
std::string const Constants::COLON = ":";
std::string const Constants::TAB = "\t";
std::string const Constants::TABTAB = "\t\t";
std::string const Constants::SPACE = " ";
std::string const Constants::UNDERSCORE = "_";

const std::map<BatchStatsVariables, std::string> Constants::BatchStatFileName = 
{
    {BatchStatsVariables::PREVALENCE, "prevalence"},
    {BatchStatsVariables::PREVALENCESA, "SAprevalence"},
    {BatchStatsVariables::INCIDENCE, "incidence"},
    {BatchStatsVariables::POPULATION, "populationSize"},
    {BatchStatsVariables::CURRENTLYINFECTED, "numberInfected"},
    {BatchStatsVariables::NEWINFECTIONS, "newInfections"}
};

const int Constants::PREVALENT_INFECTION = 0;
//const bool Constants::INCIDENT_INFECTION = false;
const bool Constants::REMOVE = true;
const bool Constants::DONT_REMOVE = false;
const bool Constants::CONSENT_IS_REQUIRED = true;
const bool Constants::CONSENT_NOT_REQUIRED = false;
const bool Constants::EXCLUDE_NULL_BINS = true;
const bool Constants::INCLUDE_NULL_BINS = false;
const bool Constants::SHOW_INFECTED = true;
const bool Constants::NO_SHOW_INFECTED = false;
const bool Constants::NEED_TO_DELETE = true;
const bool Constants::DO_NOT_DELETE = false;
