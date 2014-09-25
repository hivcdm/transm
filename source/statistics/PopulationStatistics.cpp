#include <vector>
#include <boost/math/special_functions/erf.hpp>

#include "PopulationStatistics.h"
#include "../core/Constants.h"
#include "../entities/Person.h"

const std::vector<std::string> PopulationStatistics::LifeStatsStr =
{
	"TOTAL_LM",
	"TOTAL_HIV_NEG_LM",
	"TOTAL_HIV_NEG_DTHS",
	"TOTAL_HIV_NEG",
	"TOTAL_HIV_POS_LM",
	"TOTAL_HIV_POS_POSTINFECT_LM",
	"TOTAL_HIV_POS_DTHS",
	"TOTAL_HIV_POS",
};

//declare strings of Enums
const int NUM_LE_CAT = 12; //number of life expectancy categories
const char *lifeExpectancyStrs[NUM_LE_CAT] = {"Age(yr)", "raw deaths", "raw pop", "n", "deaths", "death rate", "midpoint survivorship", "total remaining time", "life expectancy", "median LE", "median LE Standard Error", "median LE Confidence Bounds"};

PopulationStatistics::PopulationStatistics() 
	: monthOf1990(0),
	  calculateShiftedOutcomes(false),
	  yearlyTestsByResult(4)
{
	assert(PopulationStatistics::LifeStatsStr.size() == PopulationStatistics::ENDLifeStats);
	enumClass = new EnumCls<PopulationStatistics::LifeStats>(PopulationStatistics::LifeStatsStr);
	lifeStats = new StatsRecord<PopulationStatistics::LifeStats, BaseEnumCls::nullptr_ENUM>(enumClass);
	survivalStats = new SurvivalStats();
	//Set up the timeToRecord vector... by default, record at every 1/4 of the maxTime
	timesToRecord.push_back(1);
	selectedLEStats = nullptr;
	selectedPartAcqStats = nullptr;

	printHeaderPartAcq = true;
}

PopulationStatistics::SingleLEStats::SingleLEStats()
{
	for(int i = 0; i < Person::maxYrForDeathStats; i++)
	{
		deathsByAge[i] = 0;
		popByAge[i] = 0;
	}
}

PopulationStatistics::SinglePartAcqStats::SinglePartAcqStats()
{
	for(int i = 0; i < NUM_PARTNER_BINS; i++)
	{
		partnerFreq[i] = 0;
	}
}

PopulationStatistics::SurvivalStats::SurvivalStats()
{
	for(std::size_t i = 0; i < (std::size_t)DemographicProfile::Gender::Last; i++)
	{
		numDeathGender[i] = 0;
		timeToDeathGenderSum[i] = 0;
		timeToDeathGenderSumSquare[i] = 0;
		numInfOrDeathGender[i] = 0;
		timeToInfOrDeathGenderSum[i] = 0;
		timeToInfOrDeathGenderSumSquare[i] = 0;
		numInfDeathGender[i] = 0;
		timeFromInfToDeathGenderSum[i] = 0;
		timeFromInfToDeathGenderSumSquare[i] = 0;
	}

    for(std::size_t i = 0; i < (std::size_t)DemographicProfile::Employment::Last; i++)
	{
		for(int j = 0; j < Person::ENDRiskLevel; j++)
		{
			numDeathEmplRisk[i][j] = 0;
			timeToDeathEmplRiskSum[i][j] = 0;
			timeToDeathEmplRiskSumSquare[i][j] = 0;
			numInfOrDeathEmplRisk[i][j] = 0;
			timeToInfOrDeathEmplRiskSum[i][j] = 0;
			timeToInfOrDeathEmplRiskSumSquare[i][j] = 0;
			numInfDeathEmplRisk[i][j] = 0;
			timeFromInfToDeathEmplRiskSum[i][j] = 0;
			timeFromInfToDeathEmplRiskSumSquare[i][j] = 0;
		}
	}

	for(int i = 0; i < ENDInitHIVStatus; i++)
	{
		numDeathHIVStatus[i] = 0;
		timeToDeathHIVStatusSum[i] = 0;
		timeToDeathHIVStatusSumSquare[i] = 0;
	}
}
PopulationStatistics::~PopulationStatistics()
{
	delete lifeStats;
	delete enumClass;
	delete selectedLEStats;
	delete selectedPartAcqStats;
	delete survivalStats;
	//Don't delete the SingleTimeStats because they get used in the TransmissionSummaryStats
	/*for (vector<SingleTimeStats*>::iterator j = selectedSummaryStats.begin(); j != selectedSummaryStats.end(); j++) {
		SingleTimeStats *summary = *j;
		delete summary;
	}*/
	//Don't delete the selectedSummaryStats because they get used in the TransmissionSummaryStats
	//selectedSummaryStats.clear();
}
void PopulationStatistics::processDeath(Person *_p, EventParams &_eventParams)
{
	assert((_p != nullptr));
	assert((!_p->isAlive()));
	DemographicProfile::Gender gend = (DemographicProfile::Gender) _p->getDemographicProfileVal(DemographicProfile::Demographic::Gender);
	DemographicProfile::Employment cswStatus = (DemographicProfile::Employment) _p->getDemographicProfileVal(DemographicProfile::Demographic::Employment);
	Person::RiskLevel risk = _p->getRiskLevel();
	int prevDelay = _eventParams.delayPrevalence;

	if(_eventParams.currTime > prevDelay)
	{
		//time spent in model after prev delay until death
		int timeToDeath = min<int>(_p->age - _p->initAge, _eventParams.currTime - prevDelay);
		survivalStats->numDeathGender[(std::size_t)gend]++;
        survivalStats->timeToDeathGenderSum[(std::size_t)gend] += timeToDeath;
        survivalStats->timeToDeathGenderSumSquare[(std::size_t)gend] += timeToDeath * timeToDeath;
        survivalStats->numDeathEmplRisk[(std::size_t)cswStatus][risk]++;
        survivalStats->timeToDeathEmplRiskSum[(std::size_t)cswStatus][risk] += timeToDeath;
        survivalStats->timeToDeathEmplRiskSumSquare[(std::size_t)cswStatus][risk] += timeToDeath * timeToDeath;

		if(_p->getGenerationOfInfection() == Constants::PREVALENT_INFECTION)  //initial prev case
		{
			survivalStats->numDeathHIVStatus[PREVALENT]++;
			survivalStats->timeToDeathHIVStatusSum[PREVALENT] += timeToDeath;
			survivalStats->timeToDeathHIVStatusSumSquare[PREVALENT] += timeToDeath * timeToDeath;
		}
		else
		{
			survivalStats->numDeathHIVStatus[NON_PREVALENT]++;
			survivalStats->timeToDeathHIVStatusSum[NON_PREVALENT] += timeToDeath;
			survivalStats->timeToDeathHIVStatusSumSquare[NON_PREVALENT] += timeToDeath * timeToDeath;
		}

		if(!_p->isInfected())
		{
            survivalStats->numInfOrDeathGender[(std::size_t)gend]++;
            survivalStats->timeToInfOrDeathGenderSum[(std::size_t)gend] += timeToDeath;
            survivalStats->timeToInfOrDeathGenderSumSquare[(std::size_t)gend] += timeToDeath * timeToDeath;
            survivalStats->numInfOrDeathEmplRisk[(std::size_t)cswStatus][risk]++;
            survivalStats->timeToInfOrDeathEmplRiskSum[(std::size_t)cswStatus][risk] += timeToDeath;
            survivalStats->timeToInfOrDeathEmplRiskSumSquare[(std::size_t)cswStatus][risk] += timeToDeath * timeToDeath;
		}
		else
		{
			int timeFromInfToDeath = _p->age - _p->ageInfected;
            survivalStats->numInfDeathGender[(std::size_t)gend]++;
            survivalStats->timeFromInfToDeathGenderSum[(std::size_t)gend] += timeFromInfToDeath;
            survivalStats->timeFromInfToDeathGenderSumSquare[(std::size_t)gend] += timeFromInfToDeath * timeFromInfToDeath;
            survivalStats->numInfDeathEmplRisk[(std::size_t)cswStatus][risk]++;
            survivalStats->timeFromInfToDeathEmplRiskSum[(std::size_t)cswStatus][risk] += timeFromInfToDeath;
            survivalStats->timeFromInfToDeathEmplRiskSumSquare[(std::size_t)cswStatus][risk] += timeFromInfToDeath * timeFromInfToDeath;
		}
	}

	const Person::StatsRecord *stats = _p->getStats();

	if(_p->isInfected())
	{
		lifeStats->incrStat(PopulationStatistics::TOTAL_HIV_POS_LM, stats->getStat(Person::STAT_TOTAL_LM));
		lifeStats->incrStat(PopulationStatistics::TOTAL_HIV_POS_POSTINFECT_LM, stats->getStat(Person::STAT_HIV_POS_POSTINFECT_LM));
		lifeStats->incrStat(PopulationStatistics::TOTAL_HIV_POS_DTHS, 1);
		lifeStats->incrStat(PopulationStatistics::TOTAL_HIV_POS, 1);
	}
	else
	{
		lifeStats->incrStat(PopulationStatistics::TOTAL_HIV_NEG_LM, stats->getStat(Person::STAT_TOTAL_LM));
		lifeStats->incrStat(PopulationStatistics::TOTAL_HIV_NEG_DTHS, 1);
		lifeStats->incrStat(PopulationStatistics::TOTAL_HIV_NEG, 1);
		//this person was exposed to virus but not infected
		//infectionStats->incrStat( PopulationStatistics::TOTAL_EXPOSED_BUT_NOT_INFECTED, stats->getStat(Person::STAT_EXPOSURES_BEFORE_INF));
	}

	lifeStats->incrStat(PopulationStatistics::TOTAL_LM, stats->getStat(Person::STAT_TOTAL_LM));
}

void PopulationStatistics::processPostMaxTimeDeath(Person *_p)
{
	assert((_p != nullptr));
	//assert((!_p->isAlive()));
	//assert(!(_p->cepacPatient->isAlive()));
	const Person::StatsRecord *stats = _p->getStats();

	if(_p->isInfected())
	{
		lifeStats->incrStat(PopulationStatistics::TOTAL_HIV_POS_LM, stats->getStat(Person::STAT_TOTAL_LM));
		lifeStats->incrStat(PopulationStatistics::TOTAL_HIV_POS_POSTINFECT_LM, stats->getStat(Person::STAT_HIV_POS_POSTINFECT_LM));
		//Don't count the number of deaths for final tally if they didn't time within the time frame
		//lifeStats->incrStat( PopulationStatistics::TOTAL_HIV_POS_DTHS, 1);
		lifeStats->incrStat(PopulationStatistics::TOTAL_HIV_POS, 1);
	}
	else
	{
		lifeStats->incrStat(PopulationStatistics::TOTAL_HIV_NEG_LM, stats->getStat(Person::STAT_TOTAL_LM));
		//Don't count the number of deaths for final tally if they didn't time within the time frame
		//lifeStats->incrStat( PopulationStatistics::TOTAL_HIV_NEG_DTHS, 1);
		lifeStats->incrStat(PopulationStatistics::TOTAL_HIV_NEG, 1);
		//this person was exposed to virus but not infected
		//infectionStats->incrStat( PopulationStatistics::TOTAL_EXPOSED_BUT_NOT_INFECTED, stats->getStat(Person::STAT_EXPOSURES_BEFORE_INF));
	}

	lifeStats->incrStat(PopulationStatistics::TOTAL_LM, stats->getStat(Person::STAT_TOTAL_LM));
}

void PopulationStatistics::printLMStats(std::ostream &_outStream)
{
	long infectedDeaths = static_cast<long>(lifeStats->getStat(PopulationStatistics::TOTAL_HIV_POS_DTHS));
	long uninfectedDeaths = static_cast<long>(lifeStats->getStat(PopulationStatistics::TOTAL_HIV_NEG_DTHS));
	long totalDeaths = infectedDeaths + uninfectedDeaths;
	long infectedPersons = static_cast<long>(lifeStats->getStat(PopulationStatistics::TOTAL_HIV_POS));
	long uninfectedPersons = static_cast<long>(lifeStats->getStat(PopulationStatistics::TOTAL_HIV_NEG));
	long totalPersons = infectedPersons + uninfectedPersons;

	std::string invalid("----");

	_outStream << "Statistics (Only includes negative people who have died)\tValue\tUnits" << std::endl;
    _outStream << "Infected Deaths (in time period)\t" << infectedDeaths << std::endl;
    _outStream << "Uninfected Deaths (in time period)\t" << uninfectedDeaths << std::endl;
    _outStream << "Total Deaths\t" << totalDeaths << std::endl;

	if(uninfectedDeaths > 0)
	{
		_outStream << "HIV- LM\t" << lifeStats->getStat(PopulationStatistics::TOTAL_HIV_NEG_LM) / uninfectedPersons  << "\tMths" << std::endl;
	}

	if(infectedDeaths > 0)
	{
		//TODO: Double check if this makes any sense at all
        _outStream << "HIV+ LM\t" << lifeStats->getStat(PopulationStatistics::TOTAL_HIV_POS_LM) / infectedPersons << "\tMths" << std::endl;
		_outStream << "HIV+ Survival\t" <<   lifeStats->getStat(PopulationStatistics::TOTAL_HIV_POS_POSTINFECT_LM) / infectedPersons
            << "\tMths" << std::endl;
		//TODO: Whoa, is this wrong! Calculate R0 correctly and don't round to an integer!
		_outStream << "Avg # people that someone infects\t" <<   double(infectionsTracker.getNumIncidentInfections() +
            0.0) / (infectedPersons + 0.0) << std::endl;
	}

	if(totalDeaths > 0)
	{
		_outStream << "Population Avg. LM\t" << lifeStats->getStat(PopulationStatistics::TOTAL_LM) / totalPersons <<  "\tMths" <<
            std::endl;
	}
}

void PopulationStatistics::printSurvivalStats(std::ostream &_outStream)
{
    std::ostringstream firstRow;
    std::ostringstream secondRow;
    std::ostringstream thirdRow;
    std::ostringstream fourthRow;
    std::ostringstream fifthRow;

	firstRow <<  "Survival Outputs" << Constants::TAB << Constants::TAB;
	secondRow << Constants::TAB << Constants::TAB;
	thirdRow << "Overall" << Constants::TAB << Constants::TAB;
	fourthRow << Constants::TAB << "Mean" << Constants::TAB;
	fifthRow << Constants::TAB << "SD" << Constants::TAB;
	//Time to Infection or Death
	firstRow << "Time to Infection or Death (initial HIV- population)" << Constants::TAB << Constants::TAB <<
	         Constants::TAB;
	secondRow << Constants::TAB << "Gender" << Constants::TAB << Constants::TAB;
	thirdRow << "Total" << Constants::TAB << "Male" << Constants::TAB << "Female" << Constants::TAB;
    unsigned int numTotal = survivalStats->numInfOrDeathGender[(std::size_t)DemographicProfile::Gender::Male] +
        survivalStats->numInfOrDeathGender[(std::size_t)DemographicProfile::Gender::Female];

	if(numTotal != 0)
	{
        double timeMean = (survivalStats->timeToInfOrDeathGenderSum[(std::size_t)DemographicProfile::Gender::Male] +
            survivalStats->timeToInfOrDeathGenderSum[(std::size_t)DemographicProfile::Gender::Female]) / (double)numTotal;
        double timeSD = sqrt((survivalStats->timeToInfOrDeathGenderSumSquare[(std::size_t)DemographicProfile::Gender::Male] +
            survivalStats->timeToInfOrDeathGenderSumSquare[(std::size_t)DemographicProfile::Gender::Female]) / (double)numTotal - timeMean * timeMean);
		fourthRow << timeMean << Constants::TAB;
		fifthRow << timeSD << Constants::TAB;
	}
	else
	{
		fourthRow << "N/A" << Constants::TAB;
		fifthRow << "N/A" << Constants::TAB;
	}

    for(std::size_t i = 0; i < (std::size_t)DemographicProfile::Gender::Last; i++)
	{
		if(survivalStats->numInfOrDeathGender[i] != 0)
		{
			double timeMean = survivalStats->timeToInfOrDeathGenderSum[i] / (double)
			                  survivalStats->numInfOrDeathGender[i];
			double timeSD = sqrt(survivalStats->timeToInfOrDeathGenderSumSquare[i] / (double)
			                     survivalStats->numInfOrDeathGender[i] - timeMean * timeMean);
			fourthRow << timeMean << Constants::TAB;
			fifthRow << timeSD << Constants::TAB;
		}
		else
		{
			fourthRow << "N/A" << Constants::TAB;
			fifthRow << "N/A" << Constants::TAB;
		}
	}

	firstRow << Constants::TAB << Constants::TAB << Constants::TAB;
	secondRow << "Risk Group" << Constants::TAB << "Non-CSW" << Constants::TAB << "Non-CSW" << Constants::TAB;
	thirdRow << "CSW" << Constants::TAB << "High Risk" << Constants::TAB << "Low Risk" << Constants::TAB;
    unsigned int numCSW = survivalStats->numInfOrDeathEmplRisk[(std::size_t)DemographicProfile::Employment::Csw][Person::HIGH] +
        survivalStats->numInfOrDeathEmplRisk[(std::size_t)DemographicProfile::Employment::Csw][Person::LOW];

	if(numCSW != 0)
	{
        double timeMean = (survivalStats->timeToInfOrDeathEmplRiskSum[(std::size_t)DemographicProfile::Employment::Csw][Person::HIGH] +
            survivalStats->timeToInfOrDeathEmplRiskSum[(std::size_t)DemographicProfile::Employment::Csw][Person::LOW]) / (double)numCSW;
        double timeSD = sqrt((survivalStats->timeToInfOrDeathEmplRiskSumSquare[(std::size_t)DemographicProfile::Employment::Csw][Person::HIGH] +
            survivalStats->timeToInfOrDeathEmplRiskSumSquare[(std::size_t)DemographicProfile::Employment::Csw][Person::LOW]) /
		                     (double) numCSW - timeMean * timeMean);
		fourthRow << timeMean << Constants::TAB;
		fifthRow << timeSD << Constants::TAB;
	}
	else
	{
		fourthRow << "N/A" << Constants::TAB;
		fifthRow << "N/A" << Constants::TAB;
	}

	for(int j = 0; j < Person::ENDRiskLevel; j++)
	{
        if(survivalStats->numInfOrDeathEmplRisk[(std::size_t)DemographicProfile::Employment::NonCsw][j] != 0)
		{
            double timeMean = survivalStats->timeToInfOrDeathEmplRiskSum[(std::size_t)DemographicProfile::Employment::NonCsw][j] /
                (double)survivalStats->numInfOrDeathEmplRisk[(std::size_t)DemographicProfile::Employment::NonCsw][j];
            double timeSD = sqrt(survivalStats->timeToInfOrDeathEmplRiskSumSquare[(std::size_t)DemographicProfile::Employment::NonCsw][j] /
                (double)survivalStats->numInfOrDeathEmplRisk[(std::size_t)DemographicProfile::Employment::NonCsw][j] - timeMean * timeMean);
			fourthRow << timeMean << Constants::TAB;
			fifthRow << timeSD << Constants::TAB;
		}
		else
		{
			fourthRow << "N/A" << Constants::TAB;
			fifthRow << "N/A" << Constants::TAB;
		}
	}

	//Time to Death
	firstRow << "Time to Death" << Constants::TAB << Constants::TAB << Constants::TAB;
	secondRow << Constants::TAB << "Gender" << Constants::TAB << Constants::TAB;
	thirdRow << "Total" << Constants::TAB << "Male" << Constants::TAB << "Female" << Constants::TAB;
    numTotal = survivalStats->numDeathGender[(std::size_t)DemographicProfile::Gender::Male] +
        survivalStats->numDeathGender[(std::size_t)DemographicProfile::Gender::Female];

	if(numTotal != 0)
	{
        double timeMean = (survivalStats->timeToDeathGenderSum[(std::size_t)DemographicProfile::Gender::Male] +
            survivalStats->timeToDeathGenderSum[(std::size_t)DemographicProfile::Gender::Female]) / (double)numTotal;
        double timeSD = sqrt((survivalStats->timeToDeathGenderSumSquare[(std::size_t)DemographicProfile::Gender::Male] +
            survivalStats->timeToDeathGenderSumSquare[(std::size_t)DemographicProfile::Gender::Female]) / (double)numTotal - timeMean * timeMean);
		fourthRow << timeMean << Constants::TAB;
		fifthRow << timeSD << Constants::TAB;
	}
	else
	{
		fourthRow << "N/A" << Constants::TAB;
		fifthRow << "N/A" << Constants::TAB;
	}

    for(std::size_t i = 0; i < (std::size_t)DemographicProfile::Gender::Last; i++)
	{
		if(survivalStats->numDeathGender[i] != 0)
		{
			double timeMean = survivalStats->timeToDeathGenderSum[i] / (double) survivalStats->numDeathGender[i];
			double timeSD = sqrt(survivalStats->timeToDeathGenderSumSquare[i] / (double)
			                     survivalStats->numDeathGender[i] - timeMean * timeMean);
			fourthRow << timeMean << Constants::TAB;
			fifthRow << timeSD << Constants::TAB;
		}
		else
		{
			fourthRow << "N/A" << Constants::TAB;
			fifthRow << "N/A" << Constants::TAB;
		}
	}

	firstRow << Constants::TAB << Constants::TAB << Constants::TAB;
	secondRow << "Risk Group" << Constants::TAB << "Non-CSW" << Constants::TAB << "Non-CSW" << Constants::TAB;
	thirdRow << "CSW" << Constants::TAB << "High Risk" << Constants::TAB << "Low Risk" << Constants::TAB;
    numCSW = survivalStats->numDeathEmplRisk[(std::size_t)DemographicProfile::Employment::Csw][Person::HIGH] +
        survivalStats->numDeathEmplRisk[(std::size_t)DemographicProfile::Employment::Csw][Person::LOW];

	if(numCSW != 0)
	{
        double timeMean = (survivalStats->timeToDeathEmplRiskSum[(std::size_t)DemographicProfile::Employment::Csw][Person::HIGH] +
            survivalStats->timeToDeathEmplRiskSum[(std::size_t)DemographicProfile::Employment::Csw][Person::LOW]) / (double)numCSW;
        double timeSD = sqrt((survivalStats->timeToDeathEmplRiskSumSquare[(std::size_t)DemographicProfile::Employment::Csw][Person::HIGH] +
            survivalStats->timeToDeathEmplRiskSumSquare[(std::size_t)DemographicProfile::Employment::Csw][Person::LOW]) / (double)numCSW - timeMean *
		                     timeMean);
		fourthRow << timeMean << Constants::TAB;
		fifthRow << timeSD << Constants::TAB;
	}
	else
	{
		fourthRow << "N/A" << Constants::TAB;
		fifthRow << "N/A" << Constants::TAB;
	}

	for(int j = 0; j < Person::ENDRiskLevel; j++)
	{
        if(survivalStats->numDeathEmplRisk[(std::size_t)DemographicProfile::Employment::NonCsw][j] != 0)
		{
            double timeMean = survivalStats->timeToDeathEmplRiskSum[(std::size_t)DemographicProfile::Employment::NonCsw][j] /
                (double)survivalStats->numDeathEmplRisk[(std::size_t)DemographicProfile::Employment::NonCsw][j];
            double timeSD = sqrt(survivalStats->timeToDeathEmplRiskSumSquare[(std::size_t)DemographicProfile::Employment::NonCsw][j] /
                (double)survivalStats->numDeathEmplRisk[(std::size_t)DemographicProfile::Employment::NonCsw][j] - timeMean * timeMean);
			fourthRow << timeMean << Constants::TAB;
			fifthRow << timeSD << Constants::TAB;
		}
		else
		{
			fourthRow << "N/A" << Constants::TAB;
			fifthRow << "N/A" << Constants::TAB;
		}
	}

	firstRow << Constants::TAB << Constants::TAB;
	secondRow << "Init HIV Status" << Constants::TAB << Constants::TAB;
	thirdRow << "HIV+ (Initial)" << Constants::TAB << "HIV- (Initial)" << Constants::TAB;

	for(int i = 0; i < ENDInitHIVStatus; i++)
	{
		if(survivalStats->numDeathHIVStatus[i] != 0)
		{
			double timeMean = survivalStats->timeToDeathHIVStatusSum[i] / (double) survivalStats->numDeathHIVStatus[i];
			double timeSD = sqrt(survivalStats->timeToDeathHIVStatusSumSquare[i] / (double)
			                     survivalStats->numDeathHIVStatus[i] - timeMean * timeMean);
			fourthRow << timeMean << Constants::TAB;
			fifthRow << timeSD << Constants::TAB;
		}
		else
		{
			fourthRow << "N/A" << Constants::TAB;
			fifthRow << "N/A" << Constants::TAB;
		}
	}

	//Time From Infection to Death
	firstRow << "Time from Infection to Death" << Constants::TAB << Constants::TAB << Constants::TAB;
	secondRow << Constants::TAB << "Gender" << Constants::TAB << Constants::TAB;
	thirdRow << "Total" << Constants::TAB << "Male" << Constants::TAB << "Female" << Constants::TAB;
    numTotal = survivalStats->numInfDeathGender[(std::size_t)DemographicProfile::Gender::Male] +
        survivalStats->numInfDeathGender[(std::size_t)DemographicProfile::Gender::Female];

	if(numTotal != 0)
	{
        double timeMean = (survivalStats->timeFromInfToDeathGenderSum[(std::size_t)DemographicProfile::Gender::Male] +
            survivalStats->timeFromInfToDeathGenderSum[(std::size_t)DemographicProfile::Gender::Female]) / (double)numTotal;
        double timeSD = sqrt((survivalStats->timeFromInfToDeathGenderSumSquare[(std::size_t)DemographicProfile::Gender::Male] +
            survivalStats->timeFromInfToDeathGenderSumSquare[(std::size_t)DemographicProfile::Gender::Female]) / (double)numTotal - timeMean * timeMean);
		fourthRow << timeMean << Constants::TAB;
		fifthRow << timeSD << Constants::TAB;
	}
	else
	{
		fourthRow << "N/A" << Constants::TAB;
		fifthRow << "N/A" << Constants::TAB;
	}

    for(std::size_t i = 0; i < (std::size_t)DemographicProfile::Gender::Last; i++)
	{
		if(survivalStats->numInfDeathGender[i] != 0)
		{
			double timeMean = survivalStats->timeFromInfToDeathGenderSum[i] / (double)
			                  survivalStats->numInfDeathGender[i];
			double timeSD = sqrt(survivalStats->timeFromInfToDeathGenderSumSquare[i] / (double)
			                     survivalStats->numInfDeathGender[i] - timeMean * timeMean);
			fourthRow << timeMean << Constants::TAB;
			fifthRow << timeSD << Constants::TAB;
		}
		else
		{
			fourthRow << "N/A" << Constants::TAB;
			fifthRow << "N/A" << Constants::TAB;
		}
	}

	firstRow << Constants::TAB << Constants::TAB << Constants::TAB;
	secondRow << "Risk Group" << Constants::TAB << "Non-CSW" << Constants::TAB << "Non-CSW" << Constants::TAB;
	thirdRow << "CSW" << Constants::TAB << "High Risk" << Constants::TAB << "Low Risk" << Constants::TAB;
    numCSW = survivalStats->numInfDeathEmplRisk[(std::size_t)DemographicProfile::Employment::Csw][Person::HIGH] +
        survivalStats->numInfDeathEmplRisk[(std::size_t)DemographicProfile::Employment::Csw][Person::LOW];

	if(numCSW != 0)
	{
        double timeMean = (survivalStats->timeFromInfToDeathEmplRiskSum[(std::size_t)DemographicProfile::Employment::Csw][Person::HIGH] +
            survivalStats->timeFromInfToDeathEmplRiskSum[(std::size_t)DemographicProfile::Employment::Csw][Person::LOW]) / (double)numCSW;
        double timeSD = sqrt((survivalStats->timeFromInfToDeathEmplRiskSumSquare[(std::size_t)DemographicProfile::Employment::Csw][Person::HIGH] +
            survivalStats->timeFromInfToDeathEmplRiskSumSquare[(std::size_t)DemographicProfile::Employment::Csw][Person::LOW]) /
		                     (double) numCSW - timeMean * timeMean);
		fourthRow << timeMean << Constants::TAB;
		fifthRow << timeSD << Constants::TAB;
	}
	else
	{
		fourthRow << "N/A" << Constants::TAB;
		fifthRow << "N/A" << Constants::TAB;
	}

	for(int j = 0; j < Person::ENDRiskLevel; j++)
	{
        if(survivalStats->numInfDeathEmplRisk[(std::size_t)DemographicProfile::Employment::NonCsw][j] != 0)
		{
            double timeMean = survivalStats->timeFromInfToDeathEmplRiskSum[(std::size_t)DemographicProfile::Employment::NonCsw][j] /
                (double)survivalStats->numInfDeathEmplRisk[(std::size_t)DemographicProfile::Employment::NonCsw][j];
            double timeSD = sqrt(survivalStats->timeFromInfToDeathEmplRiskSumSquare[(std::size_t)DemographicProfile::Employment::NonCsw][j] /
                (double)survivalStats->numInfDeathEmplRisk[(std::size_t)DemographicProfile::Employment::NonCsw][j] - timeMean * timeMean);
			fourthRow << timeMean << Constants::TAB;
			fifthRow << timeSD << Constants::TAB;
		}
		else
		{
			fourthRow << "N/A" << Constants::TAB;
			fifthRow << "N/A" << Constants::TAB;
		}
	}

	//write out string buffers to trace file
    _outStream << firstRow.str() << std::endl;
    _outStream << secondRow.str() << std::endl;
    _outStream << thirdRow.str() << std::endl;
    _outStream << fourthRow.str() << std::endl;
    _outStream << fifthRow.str() << std::endl;
}
void PopulationStatistics::printLEStats(std::ostream &_outStream, long currTime)
{
	assert((selectedLEStats != nullptr));
	double proportionalDeathRate[Person::maxYrForDeathStats];//proportionaldeathrate=number of deaths/total number of people for each age bucket
	double lifeTablePop[Person::maxYrForDeathStats];//number of people who survive to age bucket for a hypothetical Pop of n people
	double lifeTableDeaths[Person::maxYrForDeathStats];//number of deaths in life table for hypothetical Population
	double lifeTableMidpointSurvival[Person::maxYrForDeathStats];//number of people who survive to midpoint of age cat
	double lifeTableTotalRemainingYears[Person::maxYrForDeathStats];//total person years left for all individuals who survive to age cat
	double lifeTableLifeExpectancy[Person::maxYrForDeathStats];//mean number of years expected until death for survivors to age cat
	double survivalFunction[Person::maxYrForDeathStats];//proportion of pop that survive to year x
	int medianLELowerIndex = 0; //The index for which the survivalFunction is just over .5
	double medianLE;//The median Life Expectancy
	double medianDensity; //The value of the density function at the median
	double medianSE;//Standard Error around median
	double medianCIBound;//Confidance Interval bounds
	lifeTablePop[0] = 0;

	//add back people who died in that year
	for(int i = 0; i < Person::maxYrForDeathStats; i++)
	{
		selectedLEStats->popByAge[i] += selectedLEStats->deathsByAge[i];
		lifeTablePop[0] += selectedLEStats->popByAge[i];
	}

	for(int i = 0; i < Person::maxYrForDeathStats; i++)
	{
		if(selectedLEStats->popByAge[i] == 0)
		{
			proportionalDeathRate[i] = 0.0;
		}
		else
		{
			proportionalDeathRate[i] = selectedLEStats->deathsByAge[i] / ((float)selectedLEStats->popByAge[i]);
		}

		if(i != 0)
		{
			lifeTablePop[i] = lifeTablePop[i - 1] - lifeTableDeaths[i - 1];
			lifeTableMidpointSurvival[i - 1] = (lifeTablePop[i] + lifeTablePop[i - 1]) / 2.0;
		}

		survivalFunction[i] = lifeTablePop[i] / lifeTablePop[0];
		lifeTableDeaths[i] = proportionalDeathRate[i] * lifeTablePop[i];
	}

	//everyone dies at last age bucket
	proportionalDeathRate[Person::maxYrForDeathStats - 1] = 1;
	lifeTableTotalRemainingYears[Person::maxYrForDeathStats - 1] = 0;

	for(int i = Person::maxYrForDeathStats - 2; i >= 0; i--)
	{
		lifeTableTotalRemainingYears[i] = lifeTableTotalRemainingYears[i + 1] + lifeTableMidpointSurvival[i];
		lifeTableLifeExpectancy[i] = lifeTableTotalRemainingYears[i] / lifeTableMidpointSurvival[i];
	}

	for(int i = 0; i < Person::maxYrForDeathStats; i++)
	{
		if(survivalFunction[i] < .5)
		{
			break;
		}

		medianLELowerIndex = i;
	}

	if(medianLELowerIndex != Person::maxYrForDeathStats - 1)
	{
		medianLE = medianLELowerIndex + (survivalFunction[medianLELowerIndex] - .5) / (survivalFunction[medianLELowerIndex] -
		           survivalFunction[medianLELowerIndex + 1]);
	}
	else
	{
		medianLE = medianLELowerIndex + (survivalFunction[medianLELowerIndex] - .5) / survivalFunction[medianLELowerIndex];
	}

	medianDensity = survivalFunction[medianLELowerIndex] * proportionalDeathRate[medianLELowerIndex];
	medianSE = 1 / (2 * medianDensity * sqrt(lifeTablePop[0]));
	medianCIBound = medianSE * sqrt(2.0) * boost::math::erf_inv(medianLECI);
	_outStream << "LIFE EXPECTANCY FOR TIME " << currTime << std::endl;

	for(int i = 0; i < NUM_LE_CAT; i++)
	{
		_outStream << lifeExpectancyStrs[i] << "\t";
	}

	_outStream << std::endl;

	for(int i = 0; i < Person::maxYrForDeathStats; i++)
	{
		_outStream << i << "\t" << selectedLEStats->deathsByAge[i] << "\t" << selectedLEStats->popByAge[i] << "\t"
		           << lifeTablePop[i] << "\t" << lifeTableDeaths[i] << "\t" << proportionalDeathRate[i] << "\t" <<
		           lifeTableMidpointSurvival[i] << "\t" << lifeTableTotalRemainingYears[i] << "\t" << lifeTableLifeExpectancy[i] << "\t";

		if(i == 0)
		{
			_outStream << medianLE << "\t" << medianSE << "\t" << medianCIBound;
		}

		_outStream << std::endl;
	}
}

void PopulationStatistics::printShiftedOutcomes(std::ostream &_outStream, int year)
{
	assert(calculateShiftedOutcomes);
	std::string testTypes[] = {"True Positive", "False Positive", "True Negative", "False Negative"};

	if(year == 1990)
	{
		_outStream << "Shifted Outcomes" << std::endl;
		_outStream << Constants::TAB;
		_outStream << Constants::TAB;
		_outStream << Constants::TAB;
		_outStream << "Number Infected";
		_outStream << Constants::TAB;
		_outStream << Constants::TAB;
		_outStream << "Screening Results";
		_outStream << std::endl;
		_outStream << "Year";
		_outStream << Constants::TAB;
		_outStream << "SA Pop Size";
		_outStream << Constants::TAB;
		_outStream << "Incident";
		_outStream << Constants::TAB;
		_outStream << "Prevalent";
		_outStream << Constants::TAB;
		_outStream << "SA Prevalence";
		_outStream << Constants::TAB;
		_outStream << "Annual Incidence";
		_outStream << Constants::TAB;
		_outStream << "Total Tests";
		_outStream << Constants::TAB;

		for(int i = 0; i < SimContext::TEST_RESULT_NUM; i++)
		{
			_outStream << testTypes[i];
			_outStream << Constants::TAB;
		}

		_outStream << "Number Eligible for Access to Treatment";
		_outStream << Constants::TAB;
		_outStream << "Number Acessing Treatment";
		_outStream << Constants::TAB;
		_outStream << "Number Eligible for ART";
		_outStream << Constants::TAB;
		_outStream << "Number Receiving ART";
		_outStream << std::endl;
	}

	double yearStartPrevalence = static_cast<double>(yearStartPrevalentInfections) / yearStartSexuallyActivePopSize;
	double yearlyIncidence = static_cast<double>(yearlyIncidentInfections) / yearlyCumulativeSexuallyActivePopSize * 12;
	_outStream << year;
	_outStream << Constants::TAB;
	_outStream << yearStartSexuallyActivePopSize;
	_outStream << Constants::TAB;
	_outStream << yearlyIncidentInfections;
	_outStream << Constants::TAB;
	_outStream << yearStartPrevalentInfections;
	_outStream << Constants::TAB;
	_outStream << yearStartPrevalence;
	_outStream << Constants::TAB;
	_outStream << yearlyIncidence;
	_outStream << Constants::TAB;
	_outStream << yearlyTests;
	_outStream << Constants::TAB;

	for(int i = 0; i < SimContext::TEST_RESULT_NUM; i++)
	{
		_outStream << yearlyTestsByResult[i];
		_outStream << Constants::TAB;
	}

	_outStream << uniqueYearlyEligibleForTreatmentAccess.size();
	_outStream << Constants::TAB;
	_outStream << uniqueYearlyAccessingTreatment.size();
	_outStream << Constants::TAB;
	_outStream << uniqueYearlyEligibleForTreatment.size();
	_outStream << Constants::TAB;
	_outStream << uniqueYearlyTreated.size();
	_outStream << std::endl;
}

void PopulationStatistics::printPartAcqStats(std::ostream &_outStream, long currTime)
{
	assert(selectedPartAcqStats != nullptr);

	if(printHeaderPartAcq)
	{
		_outStream << Constants::TAB << "Frequency of Number of Partners In History" << std::endl;
		_outStream << "Time";

		for(int i = 0; i < PopulationStatistics::SinglePartAcqStats::NUM_PARTNER_BINS; i++)
		{
			_outStream << Constants::TAB << i;
		}

		_outStream << "+" << std::endl;
		printHeaderPartAcq = false;
	}

	_outStream << "month " << currTime;

	for(int i = 0; i < PopulationStatistics::SinglePartAcqStats::NUM_PARTNER_BINS; i++)
	{
		_outStream << Constants::TAB << selectedPartAcqStats->partnerFreq[i];
	}

	_outStream << std::endl;
}
void PopulationStatistics::recordIncidentInfection(EventParams &_eventParams, long _time, SexualPartnership::Type _partnershipType,
                                       const  Person *_infector, const Person *_infected, bool _print, ostream &_traceOutStream)
{
	assert((_infector != nullptr) && (_infector->isAlive()));
	assert((_infected != nullptr) && (_infected->isAlive()));
	assert(_time >= 0);
	DemographicProfile::Gender gend = (DemographicProfile::Gender) _infected->getDemographicProfileVal(DemographicProfile::Demographic::Gender);
	DemographicProfile::Employment cswStatus = (DemographicProfile::Employment) _infected->getDemographicProfileVal(DemographicProfile::Demographic::Employment);
	Person::RiskLevel risk = _infected->getRiskLevel();
	int prevDelay = _eventParams.delayPrevalence;

	if(_eventParams.currTime > prevDelay)
	{
		//time spent in model after prev delay until death
		int timeToInfection = min<int>(_infected->age - _infected->initAge, _eventParams.currTime - prevDelay);
        survivalStats->numInfOrDeathGender[(std::size_t)gend]++;
        survivalStats->timeToInfOrDeathGenderSum[(std::size_t)gend] += timeToInfection;
        survivalStats->timeToInfOrDeathGenderSumSquare[(std::size_t)gend] += timeToInfection * timeToInfection;
        survivalStats->numInfOrDeathEmplRisk[(std::size_t)cswStatus][risk]++;
        survivalStats->timeToInfOrDeathEmplRiskSum[(std::size_t)cswStatus][risk] += timeToInfection;
        survivalStats->timeToInfOrDeathEmplRiskSumSquare[(std::size_t)cswStatus][risk] += timeToInfection * timeToInfection;
	}

	infectionsTracker.recordIncidentInfection(_time, _partnershipType, _infector, _infected, _print, _traceOutStream);
}

long PopulationStatistics::getNextTimeToRecord(long currTime)
{
	int nextTime = std::numeric_limits<int>().max();

	for(auto record_time : timesToRecord)
	{
		if(record_time < nextTime && record_time >= currTime)
		{
			nextTime = record_time;
		}
	}

	return nextTime;
}

bool PopulationStatistics::isTimeToRecord(long currTime)
{
	return std::find(timesToRecord.begin(), timesToRecord.end(), currTime) != timesToRecord.end();
}

bool PopulationStatistics::isTimeToRecordLE(long currTime)
{
	for(auto le_time : timesToRecordLE)
	{
		if(le_time == currTime)
		{
			return true;
		}
	}

	return false;
}

bool PopulationStatistics::isTimeToRecordPartAcq(long currTime)
{
	for(auto part_acq_time : timesToRecordPartAcq)
	{
		if(part_acq_time == currTime)
		{
			return true;
		}
	}

	return false;
}

bool PopulationStatistics::isFirstMonthToRecordLE(long currTime)
{
	for(auto le_time : timesToRecordLE)
	{
		if(le_time == currTime)
		{
			return true;
		}
	}

	return false;
}

bool PopulationStatistics::isTimeToPrintLE(long currTime)
{
	for(auto le_time : timesToRecordLE)
	{
		if((le_time + 11) == currTime)
		{
			return true;
		}
	}

	return false;
}

void PopulationStatistics::enableShiftedOutcomes(int monthOf1990)
{
	calculateShiftedOutcomes = true;
	this->monthOf1990 = monthOf1990;
	resetYear(1990);
}

void PopulationStatistics::recordPrevalenceAndIncidence(long currTime, double _prevalence, double _SAprevalence, double _incidence,
        int saPopSize, int monthlyIncident, int monthlyPrevalent)
{
	for(auto record_time : timesToRecord)
	{
		if(record_time == currTime)
		{
			SingleTimeStats *statistics = new SingleTimeStats();
			statistics->timeOfStats = currTime;
			statistics->prevalence = _prevalence;
			statistics->incidence  = _incidence;
			statistics->SAprevalence = static_cast<long>(_SAprevalence);
			statistics->cumulativeNumberDead = static_cast<long>(lifeStats->getStat(PopulationStatistics::TOTAL_HIV_NEG_DTHS) +
			                                   lifeStats->getStat(PopulationStatistics::TOTAL_HIV_POS_DTHS));
			selectedSummaryStats.push_back(statistics);
			break;
		}
	}

	if(calculateShiftedOutcomes && currTime >= monthOf1990)
	{
		if((currTime - monthOf1990) % 12 == 0)
		{
			recordYearStartStats(saPopSize, monthlyPrevalent);
		}

		yearlyIncidentInfections += monthlyIncident;
		yearlyCumulativeSexuallyActivePopSize += saPopSize;
	}
}

std::vector<PopulationStatistics::SingleTimeStats *> *PopulationStatistics::getSelectedSummaryStats()
{
	return &(selectedSummaryStats);
}

void PopulationStatistics::recordYearStartStats(int sexuallyActivePopSize, int prevalentCases)
{
	yearStartSexuallyActivePopSize = sexuallyActivePopSize;
	yearStartPrevalentInfections = prevalentCases;
}

void PopulationStatistics::recordTestStats(int numTests, const std::vector<int> &numTestsByResult)
{
	yearlyTests += numTests;

	for(int i = 0; i < SimContext::TEST_RESULT_NUM; i++)
	{
		yearlyTestsByResult[i] += numTestsByResult[i];
	}
}

void PopulationStatistics::recordTreatmentAccessEligiblity(Person *person)
{
	uniqueYearlyEligibleForTreatmentAccess.insert(person);
	artTracker.recordTreatmentAccessEligiblity(person);
}

void PopulationStatistics::recordTreatmentAccess(Person *person)
{
	uniqueYearlyAccessingTreatment.insert(person);
	artTracker.recordTreatmentAccess(person);
}

void PopulationStatistics::recordTreatmentEligiblity(Person *person)
{
	uniqueYearlyEligibleForTreatment.insert(person);
	artTracker.recordTreatmentEligiblity(person);
}

void PopulationStatistics::recordTreatment(Person *person)
{
	uniqueYearlyTreated.insert(person);
	artTracker.recordTreatment(person);
}

void PopulationStatistics::resetYear(int newYear)
{
	relativeYear = newYear;

	yearStartPrevalentInfections = 0;
	yearStartSexuallyActivePopSize = 0;
	yearlyCumulativeSexuallyActivePopSize = 0;
	yearlyIncidentInfections = 0;
	yearlyTests = 0;

	yearlyTestsByResult.assign(yearlyTestsByResult.size(), 0);

	uniqueYearlyEligibleForTreatmentAccess.clear();
	uniqueYearlyAccessingTreatment.clear();
	uniqueYearlyEligibleForTreatment.clear();
	uniqueYearlyTreated.clear();
}
