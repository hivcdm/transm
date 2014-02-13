#include "PopStats.h"
#include "../Constants.h"
#include "../entities/Person.h"
#include <vector>
#include <boost/math/special_functions/erf.hpp>

const std::string PopStats::LifeStatsStr[PopStats::ENDLifeStats] =
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

PopStats::PopStats(long maxTime, ticpp::Element *_LEOutputNode, ticpp::Element *_partAcqOutputNode) :
	calculateShiftedOutcomes(false),
	monthOf1990(0),
	yearlyTestsByResult(4)
{
	enumClass = new EnumCls<PopStats::LifeStats>(PopStats::LifeStatsStr, PopStats::ENDLifeStats);
	lifeStats = new StatsRecord<PopStats::LifeStats, BaseEnumCls::nullptr_ENUM>(enumClass);
	survivalStats = new SurvivalStats();
	//Set up the timeToRecord vector... by default, record at every 1/4 of the maxTime
	this->timeToRecord[0] = 1;
	this->selectedLEStats = nullptr;
	this->selectedPartAcqStats = nullptr;

	for(int i = 1; i < NUM_TIMES_TO_RECORD; i++)
	{
		this->timeToRecord[i] = (long((1 / ((double) NUM_TIMES_TO_RECORD - 1)) * maxTime * i + 0.5));
	}

	//Set up the time to record LE vector using inputs from the .xml file
	for(int i = 1; i <= NUM_TIMES_TO_RECORD_LE; i++)
	{
		this->timeToRecordLE[i - 1] = boost::lexical_cast<long>(_LEOutputNode->FirstChildElement("time" +
		                              boost::lexical_cast<std::string>(i))->GetText());
	}

	this->medianLECI = boost::lexical_cast<double>(_LEOutputNode->FirstChildElement("medianCI")->GetText());

	//Set up the time to record partAcq vector using inputs from the .xml file
	for(int i = 1; i <= NUM_TIMES_TO_RECORD_PARTACQ; i++)
	{
		this->timeToRecordPartAcq[i - 1] = boost::lexical_cast<long>(_partAcqOutputNode->FirstChildElement("time" +
		                                   boost::lexical_cast<std::string>(i))->GetText());
	}

	this->printHeaderPartAcq = true;
	/*std::cout << "Max time is " << maxTime << " and the times to record are ";
	for (int i = 0; i < NUM_TIMES_TO_RECORD; i++){
		std::cout << this->timeToRecord[i] << "; ";
	}
	std::cout << std::endl;*/
}

PopStats::SingleLEStats::SingleLEStats()
{
	for(int i = 0; i < Person::maxYrForDeathStats; i++)
	{
		this->deathsByAge[i] = 0;
		this->popByAge[i] = 0;
	}
}

PopStats::SinglePartAcqStats::SinglePartAcqStats()
{
	for(int i = 0; i < this->NUM_PARTNER_BINS; i++)
	{
		this->partnerFreq[i] = 0;
	}
}

PopStats::SurvivalStats::SurvivalStats()
{
	for(int i = 0; i < DmgProfile::ENDGender; i++)
	{
		this->numDeathGender[i] = 0;
		this->timeToDeathGenderSum[i] = 0;
		this->timeToDeathGenderSumSquare[i] = 0;
		this->numInfOrDeathGender[i] = 0;
		this->timeToInfOrDeathGenderSum[i] = 0;
		this->timeToInfOrDeathGenderSumSquare[i] = 0;
		this->numInfDeathGender[i] = 0;
		this->timeFromInfToDeathGenderSum[i] = 0;
		this->timeFromInfToDeathGenderSumSquare[i] = 0;
	}

	for(int i = 0; i < DmgProfile::ENDEmployment; i++)
	{
		for(int j = 0; j < Person::ENDRiskLevel; j++)
		{
			this->numDeathEmplRisk[i][j] = 0;
			this->timeToDeathEmplRiskSum[i][j] = 0;
			this->timeToDeathEmplRiskSumSquare[i][j] = 0;
			this->numInfOrDeathEmplRisk[i][j] = 0;
			this->timeToInfOrDeathEmplRiskSum[i][j] = 0;
			this->timeToInfOrDeathEmplRiskSumSquare[i][j] = 0;
			this->numInfDeathEmplRisk[i][j] = 0;
			this->timeFromInfToDeathEmplRiskSum[i][j] = 0;
			this->timeFromInfToDeathEmplRiskSumSquare[i][j] = 0;
		}
	}

	for(int i = 0; i < ENDInitHIVStatus; i++)
	{
		this->numDeathHIVStatus[i] = 0;
		this->timeToDeathHIVStatusSum[i] = 0;
		this->timeToDeathHIVStatusSumSquare[i] = 0;
	}
}
PopStats::~PopStats()
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
void PopStats::processDeath(Person *_p, EventParams &_eventParams)
{
	assert((_p != nullptr));
	assert((!_p->isAlive()));
	DmgProfile::Gender gend = (DmgProfile::Gender) _p->getDmgProfileVal(DmgProfile::GENDER);
	DmgProfile::Employment cswStatus = (DmgProfile::Employment) _p->getDmgProfileVal(DmgProfile::EMPLOYMENT);
	Person::RiskLevel risk = _p->getRiskLevel();
	int prevDelay = _eventParams.delayPrevalence;

	if(_eventParams.currTime > prevDelay)
	{
		//time spent in model after prev delay until death
		int timeToDeath = min<int>(_p->age - _p->initAge, _eventParams.currTime - prevDelay);
		this->survivalStats->numDeathGender[gend]++;
		this->survivalStats->timeToDeathGenderSum[gend] += timeToDeath;
		this->survivalStats->timeToDeathGenderSumSquare[gend] += timeToDeath * timeToDeath;
		this->survivalStats->numDeathEmplRisk[cswStatus][risk]++;
		this->survivalStats->timeToDeathEmplRiskSum[cswStatus][risk] += timeToDeath;
		this->survivalStats->timeToDeathEmplRiskSumSquare[cswStatus][risk] += timeToDeath * timeToDeath;

		if(_p->getGenerationOfInfection() == Constants::PREVALENT_INFECTION)  //initial prev case
		{
			this->survivalStats->numDeathHIVStatus[PREVALENT]++;
			this->survivalStats->timeToDeathHIVStatusSum[PREVALENT] += timeToDeath;
			this->survivalStats->timeToDeathHIVStatusSumSquare[PREVALENT] += timeToDeath * timeToDeath;
		}
		else
		{
			this->survivalStats->numDeathHIVStatus[NON_PREVALENT]++;
			this->survivalStats->timeToDeathHIVStatusSum[NON_PREVALENT] += timeToDeath;
			this->survivalStats->timeToDeathHIVStatusSumSquare[NON_PREVALENT] += timeToDeath * timeToDeath;
		}

		if(!_p->isInfected())
		{
			this->survivalStats->numInfOrDeathGender[gend]++;
			this->survivalStats->timeToInfOrDeathGenderSum[gend] += timeToDeath;
			this->survivalStats->timeToInfOrDeathGenderSumSquare[gend] += timeToDeath * timeToDeath;
			this->survivalStats->numInfOrDeathEmplRisk[cswStatus][risk]++;
			this->survivalStats->timeToInfOrDeathEmplRiskSum[cswStatus][risk] += timeToDeath;
			this->survivalStats->timeToInfOrDeathEmplRiskSumSquare[cswStatus][risk] += timeToDeath * timeToDeath;
		}
		else
		{
			int timeFromInfToDeath = _p->age - _p->ageInfected;
			this->survivalStats->numInfDeathGender[gend]++;
			this->survivalStats->timeFromInfToDeathGenderSum[gend] += timeFromInfToDeath;
			this->survivalStats->timeFromInfToDeathGenderSumSquare[gend] += timeFromInfToDeath * timeFromInfToDeath;
			this->survivalStats->numInfDeathEmplRisk[cswStatus][risk]++;
			this->survivalStats->timeFromInfToDeathEmplRiskSum[cswStatus][risk] += timeFromInfToDeath;
			this->survivalStats->timeFromInfToDeathEmplRiskSumSquare[cswStatus][risk] += timeFromInfToDeath * timeFromInfToDeath;
		}
	}

	const Person::StatsRecord *stats = _p->getStats();

	if(_p->isInfected())
	{
		this->lifeStats->incrStat(PopStats::TOTAL_HIV_POS_LM, stats->getStat(Person::STAT_TOTAL_LM));
		this->lifeStats->incrStat(PopStats::TOTAL_HIV_POS_POSTINFECT_LM, stats->getStat(Person::STAT_HIV_POS_POSTINFECT_LM));
		this->lifeStats->incrStat(PopStats::TOTAL_HIV_POS_DTHS, 1);
		this->lifeStats->incrStat(PopStats::TOTAL_HIV_POS, 1);
	}
	else
	{
		this->lifeStats->incrStat(PopStats::TOTAL_HIV_NEG_LM, stats->getStat(Person::STAT_TOTAL_LM));
		this->lifeStats->incrStat(PopStats::TOTAL_HIV_NEG_DTHS, 1);
		this->lifeStats->incrStat(PopStats::TOTAL_HIV_NEG, 1);
		//this person was exposed to virus but not infected
		//this->infectionStats->incrStat( PopStats::TOTAL_EXPOSED_BUT_NOT_INFECTED, stats->getStat(Person::STAT_EXPOSURES_BEFORE_INF));
	}

	this->lifeStats->incrStat(PopStats::TOTAL_LM, stats->getStat(Person::STAT_TOTAL_LM));
}

void PopStats::processPostMaxTimeDeath(Person *_p)
{
	assert((_p != nullptr));
	//assert((!_p->isAlive()));
	//assert(!(_p->cepacPatient->isAlive()));
	const Person::StatsRecord *stats = _p->getStats();

	if(_p->isInfected())
	{
		this->lifeStats->incrStat(PopStats::TOTAL_HIV_POS_LM, stats->getStat(Person::STAT_TOTAL_LM));
		this->lifeStats->incrStat(PopStats::TOTAL_HIV_POS_POSTINFECT_LM, stats->getStat(Person::STAT_HIV_POS_POSTINFECT_LM));
		//Don't count the number of deaths for final tally if they didn't time within the time frame
		//this->lifeStats->incrStat( PopStats::TOTAL_HIV_POS_DTHS, 1);
		this->lifeStats->incrStat(PopStats::TOTAL_HIV_POS, 1);
	}
	else
	{
		this->lifeStats->incrStat(PopStats::TOTAL_HIV_NEG_LM, stats->getStat(Person::STAT_TOTAL_LM));
		//Don't count the number of deaths for final tally if they didn't time within the time frame
		//this->lifeStats->incrStat( PopStats::TOTAL_HIV_NEG_DTHS, 1);
		this->lifeStats->incrStat(PopStats::TOTAL_HIV_NEG, 1);
		//this person was exposed to virus but not infected
		//this->infectionStats->incrStat( PopStats::TOTAL_EXPOSED_BUT_NOT_INFECTED, stats->getStat(Person::STAT_EXPOSURES_BEFORE_INF));
	}

	this->lifeStats->incrStat(PopStats::TOTAL_LM, stats->getStat(Person::STAT_TOTAL_LM));
}

void PopStats::printLMStats(std::ostream &_outStream)
{
	long infectedDeaths = static_cast<long>(this->lifeStats->getStat(PopStats::TOTAL_HIV_POS_DTHS));
	long uninfectedDeaths = static_cast<long>(this->lifeStats->getStat(PopStats::TOTAL_HIV_NEG_DTHS));
	long totalDeaths = infectedDeaths + uninfectedDeaths;
	long infectedPersons = static_cast<long>(this->lifeStats->getStat(PopStats::TOTAL_HIV_POS));
	long uninfectedPersons = static_cast<long>(this->lifeStats->getStat(PopStats::TOTAL_HIV_NEG));
	long totalPersons = infectedPersons + uninfectedPersons;
	string invalid("----");
	_outStream << "Statistics (Only includes negative people who have died)\tValue\tUnits" << endl;
	_outStream << "Infected Deaths (in time period)\t" << infectedDeaths << endl;
	_outStream << "Uninfected Deaths (in time period)\t" << uninfectedDeaths << endl;
	_outStream << "Total Deaths\t" << totalDeaths << endl;

	if(uninfectedDeaths > 0)
	{
		assert(Constants::TODO_DEF);
		//		double infectivity = this->infectionStats->getStat( PopStats::TOTAL_EXPOSED_BUT_NOT_INFECTED) / (this->infectionStats->getStat( PopStats::TOTAL_EXPOSED_BUT_NOT_INFECTED) + infectedDeaths);
		//	_outStream << "Crude infectivity\t" << infectivity << endl;
		_outStream << "HIV- LM\t" << this->lifeStats->getStat(PopStats::TOTAL_HIV_NEG_LM) / uninfectedPersons  << "\tMths" <<
		           endl;
	}

	if(infectedDeaths > 0)
	{
		//TODO: Double check if this makes any sense at all
		_outStream << "HIV+ LM\t" << this->lifeStats->getStat(PopStats::TOTAL_HIV_POS_LM) / infectedPersons << "\tMths" << endl;
		_outStream << "HIV+ Survival\t" <<   this->lifeStats->getStat(PopStats::TOTAL_HIV_POS_POSTINFECT_LM) / infectedPersons
		           << "\tMths" << endl;
		//TODO: Whoa, is this wrong! Calculate R0 correctly and don't round to an integer!
		_outStream << "Avg # people that someone infects\t" <<   double(this->infectionsTracker.getNumIncidentInfections() +
		           0.0) / (infectedPersons + 0.0) << endl;
	}

	if(totalDeaths > 0)
	{
		_outStream << "Population Avg. LM\t" << this->lifeStats->getStat(PopStats::TOTAL_LM) / totalPersons <<  "\tMths" <<
		           endl;
	}

	/*std::cout << "This run:" << std::endl << "Time:\t";
	for (int i = 0; i < NUM_TIMES_TO_RECORD; i++){
		std::cout << this->selectedSummaryStats.at(i)->timeOfStats << "\t";
	}
	std::cout << std::endl << "Prev:\t";
	for (int i = 0; i < NUM_TIMES_TO_RECORD; i++){
		std::cout << this->selectedSummaryStats.at(i)->prevalence << "\t";
	}
	std::cout << std::endl << "SAprv:\t";
	for (int i = 0; i < NUM_TIMES_TO_RECORD; i++){
		std::cout << this->selectedSummaryStats.at(i)->SAprevalence << "\t";
	}
	std::cout << std::endl << "Incid:\t";
	for (int i = 0; i < NUM_TIMES_TO_RECORD; i++){
		std::cout << this->selectedSummaryStats.at(i)->incidence << "\t";
	}
	std::cout << std::endl;*/
}

void PopStats::printSurvivalStats(std::ostream &_outStream)
{
	ostringstream firstRow;
	ostringstream secondRow;
	ostringstream thirdRow;
	ostringstream fourthRow;
	ostringstream fifthRow;
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
	unsigned int numTotal = this->survivalStats->numInfOrDeathGender[DmgProfile::MALE] +
	                        this->survivalStats->numInfOrDeathGender[DmgProfile::FEMALE];

	if(numTotal != 0)
	{
		double timeMean = (this->survivalStats->timeToInfOrDeathGenderSum[DmgProfile::MALE] +
		                   this->survivalStats->timeToInfOrDeathGenderSum[DmgProfile::FEMALE]) / (double) numTotal;
		double timeSD = sqrt((this->survivalStats->timeToInfOrDeathGenderSumSquare[DmgProfile::MALE] +
		                      this->survivalStats->timeToInfOrDeathGenderSumSquare[DmgProfile::FEMALE]) / (double) numTotal - timeMean * timeMean);
		fourthRow << timeMean << Constants::TAB;
		fifthRow << timeSD << Constants::TAB;
	}
	else
	{
		fourthRow << "N/A" << Constants::TAB;
		fifthRow << "N/A" << Constants::TAB;
	}

	for(int i = 0; i < DmgProfile::ENDGender; i++)
	{
		if(this->survivalStats->numInfOrDeathGender[i] != 0)
		{
			double timeMean = this->survivalStats->timeToInfOrDeathGenderSum[i] / (double)
			                  this->survivalStats->numInfOrDeathGender[i];
			double timeSD = sqrt(this->survivalStats->timeToInfOrDeathGenderSumSquare[i] / (double)
			                     this->survivalStats->numInfOrDeathGender[i] - timeMean * timeMean);
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
	unsigned int numCSW = this->survivalStats->numInfOrDeathEmplRisk[DmgProfile::CSW][Person::HIGH] +
	                      this->survivalStats->numInfOrDeathEmplRisk[DmgProfile::CSW][Person::LOW];

	if(numCSW != 0)
	{
		double timeMean = (this->survivalStats->timeToInfOrDeathEmplRiskSum[DmgProfile::CSW][Person::HIGH] +
		                   this->survivalStats->timeToInfOrDeathEmplRiskSum[DmgProfile::CSW][Person::LOW]) / (double) numCSW;
		double timeSD = sqrt((this->survivalStats->timeToInfOrDeathEmplRiskSumSquare[DmgProfile::CSW][Person::HIGH] +
		                      this->survivalStats->timeToInfOrDeathEmplRiskSumSquare[DmgProfile::CSW][Person::LOW]) /
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
		if(this->survivalStats->numInfOrDeathEmplRisk[DmgProfile::NON_CSW][j] != 0)
		{
			double timeMean = this->survivalStats->timeToInfOrDeathEmplRiskSum[DmgProfile::NON_CSW][j] /
			                  (double) this->survivalStats->numInfOrDeathEmplRisk[DmgProfile::NON_CSW][j];
			double timeSD = sqrt(this->survivalStats->timeToInfOrDeathEmplRiskSumSquare[DmgProfile::NON_CSW][j] /
			                     (double) this->survivalStats->numInfOrDeathEmplRisk[DmgProfile::NON_CSW][j] - timeMean * timeMean);
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
	numTotal = this->survivalStats->numDeathGender[DmgProfile::MALE] +
	           this->survivalStats->numDeathGender[DmgProfile::FEMALE];

	if(numTotal != 0)
	{
		double timeMean = (this->survivalStats->timeToDeathGenderSum[DmgProfile::MALE] +
		                   this->survivalStats->timeToDeathGenderSum[DmgProfile::FEMALE]) / (double) numTotal;
		double timeSD = sqrt((this->survivalStats->timeToDeathGenderSumSquare[DmgProfile::MALE] +
		                      this->survivalStats->timeToDeathGenderSumSquare[DmgProfile::FEMALE]) / (double) numTotal - timeMean * timeMean);
		fourthRow << timeMean << Constants::TAB;
		fifthRow << timeSD << Constants::TAB;
	}
	else
	{
		fourthRow << "N/A" << Constants::TAB;
		fifthRow << "N/A" << Constants::TAB;
	}

	for(int i = 0; i < DmgProfile::ENDGender; i++)
	{
		if(this->survivalStats->numDeathGender[i] != 0)
		{
			double timeMean = this->survivalStats->timeToDeathGenderSum[i] / (double) this->survivalStats->numDeathGender[i];
			double timeSD = sqrt(this->survivalStats->timeToDeathGenderSumSquare[i] / (double)
			                     this->survivalStats->numDeathGender[i] - timeMean * timeMean);
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
	numCSW = this->survivalStats->numDeathEmplRisk[DmgProfile::CSW][Person::HIGH] +
	         this->survivalStats->numDeathEmplRisk[DmgProfile::CSW][Person::LOW];

	if(numCSW != 0)
	{
		double timeMean = (this->survivalStats->timeToDeathEmplRiskSum[DmgProfile::CSW][Person::HIGH] +
		                   this->survivalStats->timeToDeathEmplRiskSum[DmgProfile::CSW][Person::LOW]) / (double) numCSW;
		double timeSD = sqrt((this->survivalStats->timeToDeathEmplRiskSumSquare[DmgProfile::CSW][Person::HIGH] +
		                      this->survivalStats->timeToDeathEmplRiskSumSquare[DmgProfile::CSW][Person::LOW]) / (double) numCSW - timeMean *
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
		if(this->survivalStats->numDeathEmplRisk[DmgProfile::NON_CSW][j] != 0)
		{
			double timeMean = this->survivalStats->timeToDeathEmplRiskSum[DmgProfile::NON_CSW][j] /
			                  (double) this->survivalStats->numDeathEmplRisk[DmgProfile::NON_CSW][j];
			double timeSD = sqrt(this->survivalStats->timeToDeathEmplRiskSumSquare[DmgProfile::NON_CSW][j] /
			                     (double) this->survivalStats->numDeathEmplRisk[DmgProfile::NON_CSW][j] - timeMean * timeMean);
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
		if(this->survivalStats->numDeathHIVStatus[i] != 0)
		{
			double timeMean = this->survivalStats->timeToDeathHIVStatusSum[i] / (double) this->survivalStats->numDeathHIVStatus[i];
			double timeSD = sqrt(this->survivalStats->timeToDeathHIVStatusSumSquare[i] / (double)
			                     this->survivalStats->numDeathHIVStatus[i] - timeMean * timeMean);
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
	numTotal = this->survivalStats->numInfDeathGender[DmgProfile::MALE] +
	           this->survivalStats->numInfDeathGender[DmgProfile::FEMALE];

	if(numTotal != 0)
	{
		double timeMean = (this->survivalStats->timeFromInfToDeathGenderSum[DmgProfile::MALE] +
		                   this->survivalStats->timeFromInfToDeathGenderSum[DmgProfile::FEMALE]) / (double) numTotal;
		double timeSD = sqrt((this->survivalStats->timeFromInfToDeathGenderSumSquare[DmgProfile::MALE] +
		                      this->survivalStats->timeFromInfToDeathGenderSumSquare[DmgProfile::FEMALE]) / (double) numTotal - timeMean * timeMean);
		fourthRow << timeMean << Constants::TAB;
		fifthRow << timeSD << Constants::TAB;
	}
	else
	{
		fourthRow << "N/A" << Constants::TAB;
		fifthRow << "N/A" << Constants::TAB;
	}

	for(int i = 0; i < DmgProfile::ENDGender; i++)
	{
		if(this->survivalStats->numInfDeathGender[i] != 0)
		{
			double timeMean = this->survivalStats->timeFromInfToDeathGenderSum[i] / (double)
			                  this->survivalStats->numInfDeathGender[i];
			double timeSD = sqrt(this->survivalStats->timeFromInfToDeathGenderSumSquare[i] / (double)
			                     this->survivalStats->numInfDeathGender[i] - timeMean * timeMean);
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
	numCSW = this->survivalStats->numInfDeathEmplRisk[DmgProfile::CSW][Person::HIGH] +
	         this->survivalStats->numInfDeathEmplRisk[DmgProfile::CSW][Person::LOW];

	if(numCSW != 0)
	{
		double timeMean = (this->survivalStats->timeFromInfToDeathEmplRiskSum[DmgProfile::CSW][Person::HIGH] +
		                   this->survivalStats->timeFromInfToDeathEmplRiskSum[DmgProfile::CSW][Person::LOW]) / (double) numCSW;
		double timeSD = sqrt((this->survivalStats->timeFromInfToDeathEmplRiskSumSquare[DmgProfile::CSW][Person::HIGH] +
		                      this->survivalStats->timeFromInfToDeathEmplRiskSumSquare[DmgProfile::CSW][Person::LOW]) /
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
		if(this->survivalStats->numInfDeathEmplRisk[DmgProfile::NON_CSW][j] != 0)
		{
			double timeMean = this->survivalStats->timeFromInfToDeathEmplRiskSum[DmgProfile::NON_CSW][j] /
			                  (double) this->survivalStats->numInfDeathEmplRisk[DmgProfile::NON_CSW][j];
			double timeSD = sqrt(this->survivalStats->timeFromInfToDeathEmplRiskSumSquare[DmgProfile::NON_CSW][j] /
			                     (double) this->survivalStats->numInfDeathEmplRisk[DmgProfile::NON_CSW][j] - timeMean * timeMean);
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
	_outStream << firstRow.str() << endl;
	_outStream << secondRow.str() << endl;
	_outStream << thirdRow.str() << endl;
	_outStream << fourthRow.str() << endl;
	_outStream << fifthRow.str() << endl;
}
void PopStats::printLEStats(std::ostream &_outStream, long currTime)
{
	assert((this->selectedLEStats != nullptr));
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
		this->selectedLEStats->popByAge[i] += this->selectedLEStats->deathsByAge[i];
		lifeTablePop[0] += this->selectedLEStats->popByAge[i];
	}

	for(int i = 0; i < Person::maxYrForDeathStats; i++)
	{
		if(this->selectedLEStats->popByAge[i] == 0)
		{
			proportionalDeathRate[i] = 0.0;
		}
		else
		{
			proportionalDeathRate[i] = this->selectedLEStats->deathsByAge[i] / ((float)this->selectedLEStats->popByAge[i]);
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
	medianCIBound = medianSE * sqrt(2.0) * boost::math::erf_inv(this->medianLECI);
	_outStream << "LIFE EXPECTANCY FOR TIME " << currTime << endl;

	for(int i = 0; i < NUM_LE_CAT; i++)
	{
		_outStream << lifeExpectancyStrs[i] << "\t";
	}

	_outStream << endl;

	for(int i = 0; i < Person::maxYrForDeathStats; i++)
	{
		_outStream << i << "\t" << this->selectedLEStats->deathsByAge[i] << "\t" << this->selectedLEStats->popByAge[i] << "\t"
		           << lifeTablePop[i] << "\t" << lifeTableDeaths[i] << "\t" << proportionalDeathRate[i] << "\t" <<
		           lifeTableMidpointSurvival[i] << "\t" << lifeTableTotalRemainingYears[i] << "\t" << lifeTableLifeExpectancy[i] << "\t";

		if(i == 0)
		{
			_outStream << medianLE << "\t" << medianSE << "\t" << medianCIBound;
		}

		_outStream << endl;
	}
}

void PopStats::printShiftedOutcomes(std::ostream &_outStream, int year)
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

	double yearStartPrevalence = static_cast<double>(this->yearStartPrevalentInfections) / yearStartSexuallyActivePopSize;
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

void PopStats::printPartAcqStats(std::ostream &_outStream, long currTime)
{
	assert(this->selectedPartAcqStats != nullptr);

	if(this->printHeaderPartAcq)
	{
		_outStream << Constants::TAB << "Frequency of Number of Partners In History" << endl;
		_outStream << "Time";

		for(int i = 0; i < PopStats::SinglePartAcqStats::NUM_PARTNER_BINS; i++)
		{
			_outStream << Constants::TAB << i;
		}

		_outStream << "+" << endl;
		this->printHeaderPartAcq = false;
	}

	_outStream << "month " << currTime;

	for(int i = 0; i < PopStats::SinglePartAcqStats::NUM_PARTNER_BINS; i++)
	{
		_outStream << Constants::TAB << this->selectedPartAcqStats->partnerFreq[i];
	}

	_outStream << endl;
}
void PopStats::recordIncidentInfection(EventParams &_eventParams, long _time, SexualPartnership::Type _partnershipType,
                                       const  Person *_infector, const Person *_infected, bool _print, ostream &_traceOutStream)
{
	assert((_infector != nullptr) && (_infector->isAlive()));
	assert((_infected != nullptr) && (_infected->isAlive()));
	assert(_time >= 0);
	DmgProfile::Gender gend = (DmgProfile::Gender) _infected->getDmgProfileVal(DmgProfile::GENDER);
	DmgProfile::Employment cswStatus = (DmgProfile::Employment) _infected->getDmgProfileVal(DmgProfile::EMPLOYMENT);
	Person::RiskLevel risk = _infected->getRiskLevel();
	int prevDelay = _eventParams.delayPrevalence;

	if(_eventParams.currTime > prevDelay)
	{
		//time spent in model after prev delay until death
		int timeToInfection = min<int>(_infected->age - _infected->initAge, _eventParams.currTime - prevDelay);
		this->survivalStats->numInfOrDeathGender[gend]++;
		this->survivalStats->timeToInfOrDeathGenderSum[gend] += timeToInfection;
		this->survivalStats->timeToInfOrDeathGenderSumSquare[gend] += timeToInfection * timeToInfection;
		this->survivalStats->numInfOrDeathEmplRisk[cswStatus][risk]++;
		this->survivalStats->timeToInfOrDeathEmplRiskSum[cswStatus][risk] += timeToInfection;
		this->survivalStats->timeToInfOrDeathEmplRiskSumSquare[cswStatus][risk] += timeToInfection * timeToInfection;
	}

	this->infectionsTracker.recordIncidentInfection(_time, _partnershipType, _infector, _infected, _print, _traceOutStream);
}

long PopStats::getNextTimeToRecord(long currTime)
{
	int nextTime = std::numeric_limits<int>().max();

	for(int i = 0; i < NUM_TIMES_TO_RECORD; i++)
	{
		if(this->timeToRecord[i] < nextTime && this->timeToRecord[i] >= currTime)
		{
			nextTime = this->timeToRecord[i];
		}
	}

	return nextTime;
}

bool PopStats::isTimeToRecord(long currTime)
{
	for(int i = 0; i < NUM_TIMES_TO_RECORD; i++)
	{
		if(this->timeToRecord[i] == currTime)
		{
			return true;
		}
	}

	return false;
}

bool PopStats::isTimeToRecordLE(long currTime)
{
	for(int i = 0; i < NUM_TIMES_TO_RECORD_LE; i++)
	{
		if(timeToRecordLE[i] == (currTime - 1) / 12)
		{
			return true;
		}
	}

	return false;
}

bool PopStats::isTimeToRecordPartAcq(long currTime)
{
	for(int i = 0; i < NUM_TIMES_TO_RECORD_PARTACQ; i++)
	{
		if(this->timeToRecordPartAcq[i] == currTime)
		{
			return true;
		}
	}

	return false;
}

bool PopStats::isFirstMonthToRecordLE(long currTime)
{
	for(int i = 0; i < NUM_TIMES_TO_RECORD_LE; i++)
	{
		if(timeToRecordLE[i] * 12 == (currTime - 1))
		{
			return true;
		}
	}

	return false;
}

bool PopStats::isTimeToPrintLE(long currTime)
{
	for(int i = 0; i < NUM_TIMES_TO_RECORD_LE; i++)
	{
		if((timeToRecordLE[i] * 12 + 11) == (currTime - 1))
		{
			return true;
		}
	}

	return false;
}

void PopStats::enableShiftedOutcomes(int monthOf1990)
{
	calculateShiftedOutcomes = true;
	this->monthOf1990 = monthOf1990;
	resetYear(1990);
}

void PopStats::recordPrevalenceAndIncidence(long currTime, double _prevalence, double _SAprevalence, double _incidence,
        int saPopSize, int monthlyIncident, int monthlyPrevalent)
{
	for(int i = 0; i < NUM_TIMES_TO_RECORD; i++)
	{
		if(this->timeToRecord[i] == currTime)
		{
			SingleTimeStats *statistics = new SingleTimeStats();
			statistics->timeOfStats = currTime;
			statistics->prevalence = _prevalence;
			statistics->incidence  = _incidence;
			statistics->SAprevalence = static_cast<long>(_SAprevalence);
			statistics->cumulativeNumberDead = static_cast<long>(this->lifeStats->getStat(PopStats::TOTAL_HIV_NEG_DTHS) +
			                                   this->lifeStats->getStat(PopStats::TOTAL_HIV_POS_DTHS));
			this->selectedSummaryStats.push_back(statistics);
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

std::vector<PopStats::SingleTimeStats *> *PopStats::getSelectedSummaryStats()
{
	return &(this->selectedSummaryStats);
}

void PopStats::recordYearStartStats(int sexuallyActivePopSize, int prevalentCases)
{
	yearStartSexuallyActivePopSize = sexuallyActivePopSize;
	yearStartPrevalentInfections = prevalentCases;
}

void PopStats::recordTestStats(int numTests, const std::vector<int> &numTestsByResult)
{
	yearlyTests += numTests;

	for(int i = 0; i < SimContext::TEST_RESULT_NUM; i++)
	{
		yearlyTestsByResult[i] += numTestsByResult[i];
	}
}

void PopStats::recordTreatmentAccessEligiblity(Person *person)
{
	uniqueYearlyEligibleForTreatmentAccess.insert(person);
	artTracker.recordTreatmentAccessEligiblity(person);
}

void PopStats::recordTreatmentAccess(Person *person)
{
	uniqueYearlyAccessingTreatment.insert(person);
	artTracker.recordTreatmentAccess(person);
}

void PopStats::recordTreatmentEligiblity(Person *person)
{
	uniqueYearlyEligibleForTreatment.insert(person);
	artTracker.recordTreatmentEligiblity(person);
}

void PopStats::recordTreatment(Person *person)
{
	uniqueYearlyTreated.insert(person);
	artTracker.recordTreatment(person);
}

void PopStats::resetYear(int newYear)
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