#include "Constants.h"
#include "Version.h"

std::string const Constants::VERSION = VERSION;
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
