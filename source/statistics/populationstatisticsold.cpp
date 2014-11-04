#include <vector>
#include <boost/math/special_functions/erf.hpp>

#include "populationstatisticsold.hpp"
#include "core/constants.hpp"
#include "entities/entity.hpp"

namespace transm {

const std::vector<std::string> PopulationStatisticsOld::LifeStatsStr =
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

PopulationStatisticsOld::PopulationStatisticsOld() 
	: monthOf1990(0),
	  calculateShiftedOutcomes(false),
	  yearlyTestsByResult(4)
{
	assert(PopulationStatisticsOld::LifeStatsStr.size() == PopulationStatisticsOld::ENDLifeStats);
	enumClass = new EnumCls<PopulationStatisticsOld::LifeStats>(PopulationStatisticsOld::LifeStatsStr);
	lifeStats = new StatsRecord<PopulationStatisticsOld::LifeStats, BaseEnumCls::NULL_ENUM>(enumClass);
	survivalStats = new SurvivalStats();
	//Set up the timeToRecord vector... by default, record at every 1/4 of the maxTime
	timesToRecord.push_back(1);
	selectedLEStats = nullptr;
	selectedPartAcqStats = nullptr;

	printHeaderPartAcq = true;
}

PopulationStatisticsOld::SingleLEStats::SingleLEStats()
{
	for(int i = 0; i < Entity::maxYrForDeathStats; i++)
	{
		deathsByAge[i] = 0;
		popByAge[i] = 0;
	}
}

PopulationStatisticsOld::SinglePartAcqStats::SinglePartAcqStats()
{
	for(int i = 0; i < NUM_PARTNER_BINS; i++)
	{
		partnerFreq[i] = 0;
	}
}

PopulationStatisticsOld::SurvivalStats::SurvivalStats()
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
	    for(std::size_t j = 0; j < (std::size_t)Entity::RiskLevel::Last; j++)
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
PopulationStatisticsOld::~PopulationStatisticsOld()
{
	delete lifeStats;
	delete enumClass;
	delete selectedLEStats;
	delete selectedPartAcqStats;
	delete survivalStats;
}
void PopulationStatisticsOld::processDeath(Entity *_p, EventParams &_eventParams)
{
	assert((_p != nullptr));
	assert((!_p->isAlive()));
	DemographicProfile::Gender gend = (DemographicProfile::Gender) _p->getDemographicProfileVal(DemographicProfile::Demographic::Gender);
	DemographicProfile::Employment cswStatus = (DemographicProfile::Employment) _p->getDemographicProfileVal(DemographicProfile::Demographic::Employment);
	Entity::RiskLevel risk = _p->getRiskLevel();
	int prevDelay = _eventParams.delayPrevalence;

	if(_eventParams.currTime > prevDelay)
	{
		//time spent in model after prev delay until death
		int timeToDeath = min<int>(_p->age - _p->initAge, _eventParams.currTime - prevDelay);
		survivalStats->numDeathGender[(std::size_t)gend]++;
        survivalStats->timeToDeathGenderSum[(std::size_t)gend] += timeToDeath;
        survivalStats->timeToDeathGenderSumSquare[(std::size_t)gend] += timeToDeath * timeToDeath;
        survivalStats->numDeathEmplRisk[(std::size_t)cswStatus][(std::size_t)risk]++;
        survivalStats->timeToDeathEmplRiskSum[(std::size_t)cswStatus][(std::size_t)risk] += timeToDeath;
        survivalStats->timeToDeathEmplRiskSumSquare[(std::size_t)cswStatus][(std::size_t)risk] += timeToDeath * timeToDeath;

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
            survivalStats->numInfOrDeathEmplRisk[(std::size_t)cswStatus][(std::size_t)risk]++;
            survivalStats->timeToInfOrDeathEmplRiskSum[(std::size_t)cswStatus][(std::size_t)risk] += timeToDeath;
            survivalStats->timeToInfOrDeathEmplRiskSumSquare[(std::size_t)cswStatus][(std::size_t)risk] += timeToDeath * timeToDeath;
		}
		else
		{
			int timeFromInfToDeath = _p->age - _p->ageInfected;
            survivalStats->numInfDeathGender[(std::size_t)gend]++;
            survivalStats->timeFromInfToDeathGenderSum[(std::size_t)gend] += timeFromInfToDeath;
            survivalStats->timeFromInfToDeathGenderSumSquare[(std::size_t)gend] += timeFromInfToDeath * timeFromInfToDeath;
            survivalStats->numInfDeathEmplRisk[(std::size_t)cswStatus][(std::size_t)risk]++;
            survivalStats->timeFromInfToDeathEmplRiskSum[(std::size_t)cswStatus][(std::size_t)risk] += timeFromInfToDeath;
            survivalStats->timeFromInfToDeathEmplRiskSumSquare[(std::size_t)cswStatus][(std::size_t)risk] += timeFromInfToDeath * timeFromInfToDeath;
		}
	}

	const Entity::StatsRecord *stats = _p->getStats();

	if(_p->isInfected())
	{
		lifeStats->incrStat(PopulationStatisticsOld::TOTAL_HIV_POS_LM, stats->getStat(Entity::Stats::STAT_TOTAL_LM));
		lifeStats->incrStat(PopulationStatisticsOld::TOTAL_HIV_POS_POSTINFECT_LM, stats->getStat(Entity::Stats::STAT_HIV_POS_POSTINFECT_LM));
		lifeStats->incrStat(PopulationStatisticsOld::TOTAL_HIV_POS_DTHS, 1);
		lifeStats->incrStat(PopulationStatisticsOld::TOTAL_HIV_POS, 1);
	}
	else
	{
		lifeStats->incrStat(PopulationStatisticsOld::TOTAL_HIV_NEG_LM, stats->getStat(Entity::Stats::STAT_TOTAL_LM));
		lifeStats->incrStat(PopulationStatisticsOld::TOTAL_HIV_NEG_DTHS, 1);
		lifeStats->incrStat(PopulationStatisticsOld::TOTAL_HIV_NEG, 1);
		//this person was exposed to virus but not infected
		//infectionStats->incrStat( PopulationStatisticsOld::TOTAL_EXPOSED_BUT_NOT_INFECTED, stats->getStat(Entity::Stats::STAT_EXPOSURES_BEFORE_INF));
	}

	lifeStats->incrStat(PopulationStatisticsOld::TOTAL_LM, stats->getStat(Entity::Stats::STAT_TOTAL_LM));
}

void PopulationStatisticsOld::processPostMaxTimeDeath(Entity *_p)
{
	assert((_p != nullptr));
	//assert((!_p->isAlive()));
	//assert(!(_p->cepacPatient->isAlive()));
	const Entity::StatsRecord *stats = _p->getStats();

	if(_p->isInfected())
	{
		lifeStats->incrStat(PopulationStatisticsOld::TOTAL_HIV_POS_LM, stats->getStat(Entity::Stats::STAT_TOTAL_LM));
		lifeStats->incrStat(PopulationStatisticsOld::TOTAL_HIV_POS_POSTINFECT_LM, stats->getStat(Entity::Stats::STAT_HIV_POS_POSTINFECT_LM));
		lifeStats->incrStat(PopulationStatisticsOld::TOTAL_HIV_POS, 1);
	}
	else
	{
		lifeStats->incrStat(PopulationStatisticsOld::TOTAL_HIV_NEG_LM, stats->getStat(Entity::Stats::STAT_TOTAL_LM));
		lifeStats->incrStat(PopulationStatisticsOld::TOTAL_HIV_NEG, 1);
	}

	lifeStats->incrStat(PopulationStatisticsOld::TOTAL_LM, stats->getStat(Entity::Stats::STAT_TOTAL_LM));
}

void PopulationStatisticsOld::printLMStats(std::ostream &_outStream)
{
	long infectedDeaths = static_cast<long>(lifeStats->getStat(PopulationStatisticsOld::TOTAL_HIV_POS_DTHS));
	long uninfectedDeaths = static_cast<long>(lifeStats->getStat(PopulationStatisticsOld::TOTAL_HIV_NEG_DTHS));
	long totalDeaths = infectedDeaths + uninfectedDeaths;
	long infectedPersons = static_cast<long>(lifeStats->getStat(PopulationStatisticsOld::TOTAL_HIV_POS));
	long uninfectedPersons = static_cast<long>(lifeStats->getStat(PopulationStatisticsOld::TOTAL_HIV_NEG));
	long totalPersons = infectedPersons + uninfectedPersons;

	std::string invalid("----");

	_outStream << "Statistics (Only includes negative people who have died)\tValue\tUnits" << std::endl;
    _outStream << "Infected Deaths (in time period)\t" << infectedDeaths << std::endl;
    _outStream << "Uninfected Deaths (in time period)\t" << uninfectedDeaths << std::endl;
    _outStream << "Total Deaths\t" << totalDeaths << std::endl;

	if(uninfectedDeaths > 0)
	{
		_outStream << "HIV- LM\t" << lifeStats->getStat(PopulationStatisticsOld::TOTAL_HIV_NEG_LM) / uninfectedPersons  << "\tMths" << std::endl;
	}

	if(infectedDeaths > 0)
	{
		//TODO: Double check if this makes any sense at all
        _outStream << "HIV+ LM\t" << lifeStats->getStat(PopulationStatisticsOld::TOTAL_HIV_POS_LM) / infectedPersons << "\tMths" << std::endl;
		_outStream << "HIV+ Survival\t" <<   lifeStats->getStat(PopulationStatisticsOld::TOTAL_HIV_POS_POSTINFECT_LM) / infectedPersons
            << "\tMths" << std::endl;
		//TODO: Whoa, is this wrong! Calculate R0 correctly and don't round to an integer!
		_outStream << "Avg # people that someone infects\t" <<   double(infectionsTracker.getNumIncidentInfections() +
            0.0) / (infectedPersons + 0.0) << std::endl;
	}

	if(totalDeaths > 0)
	{
		_outStream << "Population Avg. LM\t" << lifeStats->getStat(PopulationStatisticsOld::TOTAL_LM) / totalPersons <<  "\tMths" <<
            std::endl;
	}
}

void PopulationStatisticsOld::printSurvivalStats(std::ostream &_outStream)
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
    unsigned int numCSW = survivalStats->numInfOrDeathEmplRisk[(std::size_t)DemographicProfile::Employment::Csw][(std::size_t)Entity::RiskLevel::HIGH] +
        survivalStats->numInfOrDeathEmplRisk[(std::size_t)DemographicProfile::Employment::Csw][(std::size_t)Entity::RiskLevel::LOW];

	if(numCSW != 0)
	{
        double timeMean = (survivalStats->timeToInfOrDeathEmplRiskSum[(std::size_t)DemographicProfile::Employment::Csw][(std::size_t)Entity::RiskLevel::HIGH] +
            survivalStats->timeToInfOrDeathEmplRiskSum[(std::size_t)DemographicProfile::Employment::Csw][(std::size_t)Entity::RiskLevel::LOW]) / (double)numCSW;
        double timeSD = sqrt((survivalStats->timeToInfOrDeathEmplRiskSumSquare[(std::size_t)DemographicProfile::Employment::Csw][(std::size_t)Entity::RiskLevel::HIGH] +
            survivalStats->timeToInfOrDeathEmplRiskSumSquare[(std::size_t)DemographicProfile::Employment::Csw][(std::size_t)Entity::RiskLevel::LOW]) /
		                     (double) numCSW - timeMean * timeMean);
		fourthRow << timeMean << Constants::TAB;
		fifthRow << timeSD << Constants::TAB;
	}
	else
	{
		fourthRow << "N/A" << Constants::TAB;
		fifthRow << "N/A" << Constants::TAB;
	}

	for(std::size_t j = 0; j < (std::size_t)Entity::RiskLevel::Last; j++)
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
    numCSW = survivalStats->numDeathEmplRisk[(std::size_t)DemographicProfile::Employment::Csw][(std::size_t)Entity::RiskLevel::HIGH] +
        survivalStats->numDeathEmplRisk[(std::size_t)DemographicProfile::Employment::Csw][(std::size_t)Entity::RiskLevel::LOW];

	if(numCSW != 0)
	{
        double timeMean = (survivalStats->timeToDeathEmplRiskSum[(std::size_t)DemographicProfile::Employment::Csw][(std::size_t)Entity::RiskLevel::HIGH] +
            survivalStats->timeToDeathEmplRiskSum[(std::size_t)DemographicProfile::Employment::Csw][(std::size_t)Entity::RiskLevel::LOW]) / (double)numCSW;
        double timeSD = sqrt((survivalStats->timeToDeathEmplRiskSumSquare[(std::size_t)DemographicProfile::Employment::Csw][(std::size_t)Entity::RiskLevel::HIGH] +
            survivalStats->timeToDeathEmplRiskSumSquare[(std::size_t)DemographicProfile::Employment::Csw][(std::size_t)Entity::RiskLevel::LOW]) / (double)numCSW - timeMean *
		                     timeMean);
		fourthRow << timeMean << Constants::TAB;
		fifthRow << timeSD << Constants::TAB;
	}
	else
	{
		fourthRow << "N/A" << Constants::TAB;
		fifthRow << "N/A" << Constants::TAB;
	}

	for(std::size_t j = 0; j < (std::size_t)Entity::RiskLevel::Last; j++)
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
    numCSW = survivalStats->numInfDeathEmplRisk[(std::size_t)DemographicProfile::Employment::Csw][(std::size_t)Entity::RiskLevel::HIGH] +
        survivalStats->numInfDeathEmplRisk[(std::size_t)DemographicProfile::Employment::Csw][(std::size_t)Entity::RiskLevel::LOW];

	if(numCSW != 0)
	{
        double timeMean = (survivalStats->timeFromInfToDeathEmplRiskSum[(std::size_t)DemographicProfile::Employment::Csw][(std::size_t)Entity::RiskLevel::HIGH] +
            survivalStats->timeFromInfToDeathEmplRiskSum[(std::size_t)DemographicProfile::Employment::Csw][(std::size_t)Entity::RiskLevel::LOW]) / (double)numCSW;
        double timeSD = sqrt((survivalStats->timeFromInfToDeathEmplRiskSumSquare[(std::size_t)DemographicProfile::Employment::Csw][(std::size_t)Entity::RiskLevel::HIGH] +
            survivalStats->timeFromInfToDeathEmplRiskSumSquare[(std::size_t)DemographicProfile::Employment::Csw][(std::size_t)Entity::RiskLevel::LOW]) /
		                     (double) numCSW - timeMean * timeMean);
		fourthRow << timeMean << Constants::TAB;
		fifthRow << timeSD << Constants::TAB;
	}
	else
	{
		fourthRow << "N/A" << Constants::TAB;
		fifthRow << "N/A" << Constants::TAB;
	}

	for(std::size_t j = 0; j < (std::size_t)Entity::RiskLevel::Last; j++)
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
void PopulationStatisticsOld::printLEStats(std::ostream &_outStream, long currTime)
{
	assert((selectedLEStats != nullptr));
	double proportionalDeathRate[Entity::maxYrForDeathStats];//proportionaldeathrate=number of deaths/total number of people for each age bucket
	double lifeTablePop[Entity::maxYrForDeathStats];//number of people who survive to age bucket for a hypothetical Pop of n people
	double lifeTableDeaths[Entity::maxYrForDeathStats];//number of deaths in life table for hypothetical Population
	double lifeTableMidpointSurvival[Entity::maxYrForDeathStats];//number of people who survive to midpoint of age cat
	double lifeTableTotalRemainingYears[Entity::maxYrForDeathStats];//total person years left for all individuals who survive to age cat
	double lifeTableLifeExpectancy[Entity::maxYrForDeathStats];//mean number of years expected until death for survivors to age cat
	double survivalFunction[Entity::maxYrForDeathStats];//proportion of pop that survive to year x
	int medianLELowerIndex = 0; //The index for which the survivalFunction is just over .5
	double medianLE;//The median Life Expectancy
	double medianDensity; //The value of the density function at the median
	double medianSE;//Standard Error around median
	double medianCIBound;//Confidance Interval bounds
	lifeTablePop[0] = 0;

	//add back people who died in that year
	for(int i = 0; i < Entity::maxYrForDeathStats; i++)
	{
		selectedLEStats->popByAge[i] += selectedLEStats->deathsByAge[i];
		lifeTablePop[0] += selectedLEStats->popByAge[i];
	}

	for(int i = 0; i < Entity::maxYrForDeathStats; i++)
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
	proportionalDeathRate[Entity::maxYrForDeathStats - 1] = 1;
	lifeTableTotalRemainingYears[Entity::maxYrForDeathStats - 1] = 0;

	for(int i = Entity::maxYrForDeathStats - 2; i >= 0; i--)
	{
		lifeTableTotalRemainingYears[i] = lifeTableTotalRemainingYears[i + 1] + lifeTableMidpointSurvival[i];
		lifeTableLifeExpectancy[i] = lifeTableTotalRemainingYears[i] / lifeTableMidpointSurvival[i];
	}

	for(int i = 0; i < Entity::maxYrForDeathStats; i++)
	{
		if(survivalFunction[i] < .5)
		{
			break;
		}

		medianLELowerIndex = i;
	}

	if(medianLELowerIndex != Entity::maxYrForDeathStats - 1)
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

	for(int i = 0; i < Entity::maxYrForDeathStats; i++)
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

void PopulationStatisticsOld::UpdateIncidenceCalculations()
{
    std::size_t sum_incident = 0;
    std::size_t sum_negative_sa = 0;
    std::size_t sum_male_incident = 0;
    std::size_t sum_male_negative_sa = 0;

    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
        sum_incident += currentMonthIncident[entity_type];
        sum_negative_sa += currentMonthSANegative[entity_type];

        if(std::string(entity_type) != "female")
        {
            sum_male_incident += currentMonthIncident[entity_type];
            sum_male_negative_sa += currentMonthSANegative[entity_type];
        }

        if(currentMonthSANegative[entity_type] > 0)
        {
            yearlyCumulativeIncidenceByEntityType[entity_type] += currentMonthIncident[entity_type] / static_cast<double>(currentMonthSANegative[entity_type]);
        }
    }

    if(sum_negative_sa > 0)
    {
        yearlyCumulativeIncidence += sum_incident / static_cast<double>(sum_negative_sa);
    }

    if(sum_male_negative_sa > 0)
    {
        yearlyCumulativeIncidenceMale += sum_male_incident / static_cast<double>(sum_male_negative_sa);
    }

    currentMonthIncident.clear();
    currentMonthSANegative.clear();
}

void PopulationStatisticsOld::printShiftedOutcomes(std::ostream &_outStream, int year)
{
	assert(calculateShiftedOutcomes);
	std::string testTypes[] = {"True Positive", "False Positive", "True Negative", "False Negative"};

	if(year == 1990)
	{
		_outStream << "Shifted Outcomes" << std::endl;
		_outStream << Constants::TAB;
		
        for(auto entity_type : {"", "Male", "Male:Hetero", "Male:Msmw", "Male:Msm", "Female"})
        {
            _outStream << entity_type;
            _outStream << Constants::TAB;
            _outStream << Constants::TAB;
            _outStream << Constants::TAB;
            _outStream << Constants::TAB;
            _outStream << Constants::TAB;
        }
		_outStream << "Screening Results";
		_outStream << std::endl;
		_outStream << "Year";
		_outStream << Constants::TAB;

        for(std::size_t i = 0; i < 6; i++)
        {
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
        }

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

    //std::size_t sum_sa_pop_size = 0;
    std::size_t sum_incident_infections = 0;
    std::size_t sum_prevalent_infections = 0;
    std::size_t sum_year_start_sa_pop_size = 0;

    //std::size_t sum_sa_pop_size_male = 0;
    std::size_t sum_incident_infections_male = 0;
    std::size_t sum_prevalent_infections_male = 0;
    std::size_t sum_year_start_sa_pop_size_male = 0;

    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
        //sum_sa_pop_size += yearlyCumulativeSexuallyActivePopSize[entity_type];
        sum_year_start_sa_pop_size += yearStartSexuallyActivePopSize[entity_type];
        sum_prevalent_infections += yearStartPrevalentInfections[entity_type];
        sum_incident_infections += yearlyIncidentInfections[entity_type];

        if(std::string(entity_type) != "female")
        {
            //sum_sa_pop_size_male += yearlyCumulativeSexuallyActivePopSize[entity_type];
            sum_year_start_sa_pop_size_male += yearStartSexuallyActivePopSize[entity_type];
            sum_prevalent_infections_male += yearStartPrevalentInfections[entity_type];
            sum_incident_infections_male += yearlyIncidentInfections[entity_type];
        }
    }

    double yearStartPrevalence = static_cast<double>(sum_prevalent_infections) / sum_year_start_sa_pop_size;

	_outStream << year;
	_outStream << Constants::TAB;
	_outStream << sum_year_start_sa_pop_size;
	_outStream << Constants::TAB;
	_outStream << sum_incident_infections;
	_outStream << Constants::TAB;
	_outStream << sum_prevalent_infections;
	_outStream << Constants::TAB;
	_outStream << yearStartPrevalence;
	_outStream << Constants::TAB;
	_outStream << yearlyCumulativeIncidence;
	_outStream << Constants::TAB;

    double yearStartPrevalenceMale = static_cast<double>(sum_prevalent_infections_male) / sum_year_start_sa_pop_size_male;

    _outStream << sum_year_start_sa_pop_size_male << Constants::TAB;
    _outStream << sum_incident_infections_male << Constants::TAB;
    _outStream << sum_prevalent_infections_male << Constants::TAB;
    _outStream << yearStartPrevalenceMale << Constants::TAB;
    _outStream << yearlyCumulativeIncidenceMale << Constants::TAB;

    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
        _outStream << yearStartSexuallyActivePopSize[entity_type] << Constants::TAB;
        _outStream << yearlyIncidentInfections[entity_type] << Constants::TAB;
        _outStream << yearStartPrevalentInfections[entity_type] << Constants::TAB;
        _outStream << static_cast<double>(yearStartPrevalentInfections[entity_type]) / yearStartSexuallyActivePopSize[entity_type] << Constants::TAB;        
        _outStream << yearlyCumulativeIncidenceByEntityType[entity_type] << Constants::TAB;
    }

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

void PopulationStatisticsOld::printPartAcqStats(std::ostream &_outStream, long currTime)
{
	assert(selectedPartAcqStats != nullptr);

	if(printHeaderPartAcq)
	{
		_outStream << Constants::TAB << "Frequency of Number of Partners In History" << std::endl;
		_outStream << "Time";

		for(int i = 0; i < PopulationStatisticsOld::SinglePartAcqStats::NUM_PARTNER_BINS; i++)
		{
			_outStream << Constants::TAB << i;
		}

		_outStream << "+" << std::endl;
		printHeaderPartAcq = false;
	}

	_outStream << "month " << currTime;

	for(int i = 0; i < PopulationStatisticsOld::SinglePartAcqStats::NUM_PARTNER_BINS; i++)
	{
		_outStream << Constants::TAB << selectedPartAcqStats->partnerFreq[i];
	}

	_outStream << std::endl;
}
void PopulationStatisticsOld::recordIncidentInfection(EventParams &_eventParams, long _time, SexualPartnership::Type _partnershipType,
                                       const  Entity *_infector, const Entity *_infected, bool _print, ostream &_traceOutStream)
{
	assert((_infector != nullptr) && (_infector->isAlive()));
	assert((_infected != nullptr) && (_infected->isAlive()));
	assert(_time >= 0);
	DemographicProfile::Gender gend = (DemographicProfile::Gender) _infected->getDemographicProfileVal(DemographicProfile::Demographic::Gender);
	DemographicProfile::Employment cswStatus = (DemographicProfile::Employment) _infected->getDemographicProfileVal(DemographicProfile::Demographic::Employment);
	Entity::RiskLevel risk = _infected->getRiskLevel();
	int prevDelay = _eventParams.delayPrevalence;

	if(_eventParams.currTime > prevDelay)
	{
		//time spent in model after prev delay until death
		int timeToInfection = min<int>(_infected->age - _infected->initAge, _eventParams.currTime - prevDelay);
        survivalStats->numInfOrDeathGender[(std::size_t)gend]++;
        survivalStats->timeToInfOrDeathGenderSum[(std::size_t)gend] += timeToInfection;
        survivalStats->timeToInfOrDeathGenderSumSquare[(std::size_t)gend] += timeToInfection * timeToInfection;
        survivalStats->numInfOrDeathEmplRisk[(std::size_t)cswStatus][(std::size_t)risk]++;
        survivalStats->timeToInfOrDeathEmplRiskSum[(std::size_t)cswStatus][(std::size_t)risk] += timeToInfection;
        survivalStats->timeToInfOrDeathEmplRiskSumSquare[(std::size_t)cswStatus][(std::size_t)risk] += timeToInfection * timeToInfection;
	}

    yearlyIncidentInfections[_infected->getEntityType()]++;
    currentMonthIncident[_infected->getEntityType()]++;

	infectionsTracker.recordIncidentInfection(_time, _partnershipType, _infector, _infected, _print, _traceOutStream);
}

long PopulationStatisticsOld::getNextTimeToRecord(long currTime)
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

bool PopulationStatisticsOld::isTimeToRecord(long currTime)
{
	return std::find(timesToRecord.begin(), timesToRecord.end(), currTime) != timesToRecord.end();
}

bool PopulationStatisticsOld::isTimeToRecordLE(long currTime)
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

bool PopulationStatisticsOld::isTimeToRecordPartAcq(long currTime)
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

bool PopulationStatisticsOld::isFirstMonthToRecordLE(long currTime)
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

bool PopulationStatisticsOld::isTimeToPrintLE(long currTime)
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

void PopulationStatisticsOld::enableShiftedOutcomes(int monthOf1990)
{
	calculateShiftedOutcomes = true;
	this->monthOf1990 = monthOf1990;
	resetYear(1990);
}

void PopulationStatisticsOld::recordEntity(int time, Entity *e)
{
    if(e->getDemographicProfileVal<DemographicProfile::SexualActivityStatus>() == DemographicProfile::SexualActivityStatus::Active && !e->isInfected())
    {
        currentMonthSANegative[e->getEntityType()]++;
    }

    if(calculateShiftedOutcomes
        && time >= monthOf1990
        && (time - monthOf1990) % 12 == 0)
    {
        if(e->getDemographicProfileVal<DemographicProfile::SexualActivityStatus>() == DemographicProfile::SexualActivityStatus::Active)
        {
            yearStartSexuallyActivePopSize[e->getEntityType()]++;
        }

        if(e->isInfected())
        {
            yearStartPrevalentInfections[e->getEntityType()]++;
        }
    }
}


/*
	for(auto record_time : timesToRecord)
	{
		if(record_time == currTime)
		{
			SingleTimeStats *statistics = new SingleTimeStats();
			statistics->timeOfStats = currTime;
			statistics->prevalence = _prevalence;
			statistics->incidence  = _incidence;
			statistics->SAprevalence = static_cast<long>(_SAprevalence);
			statistics->cumulativeNumberDead = static_cast<long>(lifeStats->getStat(PopulationStatisticsOld::TOTAL_HIV_NEG_DTHS) +
			                                   lifeStats->getStat(PopulationStatisticsOld::TOTAL_HIV_POS_DTHS));
			selectedSummaryStats.push_back(statistics);
			break;
		}
	}
    */

std::vector<PopulationStatisticsOld::SingleTimeStats *> *PopulationStatisticsOld::getSelectedSummaryStats()
{
	return &selectedSummaryStats;
}

void PopulationStatisticsOld::recordTestStats(int numTests, const std::vector<int> &numTestsByResult)
{
	yearlyTests += numTests;

	for(int i = 0; i < SimContext::TEST_RESULT_NUM; i++)
	{
		yearlyTestsByResult[i] += numTestsByResult[i];
	}
}

void PopulationStatisticsOld::recordTreatmentAccessEligiblity(Entity *person)
{
	uniqueYearlyEligibleForTreatmentAccess.insert(person);
	artTracker.recordTreatmentAccessEligiblity(person);
}

void PopulationStatisticsOld::recordTreatmentAccess(Entity *person)
{
	uniqueYearlyAccessingTreatment.insert(person);
	artTracker.recordTreatmentAccess(person);
}

void PopulationStatisticsOld::recordTreatmentEligiblity(Entity *person)
{
	uniqueYearlyEligibleForTreatment.insert(person);
	artTracker.recordTreatmentEligiblity(person);
}

void PopulationStatisticsOld::recordTreatment(Entity *person)
{
	uniqueYearlyTreated.insert(person);
	artTracker.recordTreatment(person);
}

void PopulationStatisticsOld::resetYear(int newYear)
{
	relativeYear = newYear;

	yearStartPrevalentInfections.clear();
	yearStartSexuallyActivePopSize.clear();
	yearlyCumulativeIncidenceByEntityType.clear();
    yearlyCumulativeIncidence = 0;
    yearlyCumulativeIncidenceMale = 0;
	yearlyIncidentInfections.clear();
	yearlyTests = 0;

	yearlyTestsByResult.assign(yearlyTestsByResult.size(), 0);

	uniqueYearlyEligibleForTreatmentAccess.clear();
	uniqueYearlyAccessingTreatment.clear();
	uniqueYearlyEligibleForTreatment.clear();
	uniqueYearlyTreated.clear();
}

} // namespace transm
