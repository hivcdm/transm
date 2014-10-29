#include <iostream>
#include <sstream>

#include "infectionstracker.hpp"
#include "core/constants.hpp"
#include "core/population.hpp"

namespace transm {

InfectionsTracker::InfectionsTracker()
{
	//zero out all infection tallies
	for(int i = 0; i < (int)SexualPartnership::Type::ENDType; ++i)
	{
		for(unsigned int j = 0; j < DemographicProfile::TotalNumBuckets; ++j)
		{
			for(int g = 0; g < NUMBER_GENERATIONS_TO_TRACE; g++)
			{
				currPrevalentInfections[j][g] = 0;
			}

			for(unsigned int k = 0; k < DemographicProfile::TotalNumBuckets; ++k)
			{
				incidentInfections[i][j][k] = 0;
			}
		}
	}

	for(std::size_t i = 0; i < (std::size_t)Entity::HVLStrata::Last; ++i)
	{
		totalIncidentInfections[i] = 0;
		totalExposures[i] = 0;
		currTimeStepIncidentInfs[i] = 0;
		currTimeExposures[i] = 0;
	}

	for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
	{
        for(std::size_t j = 0; j < (std::size_t)DemographicProfile::Gender::Last; j++)
		{
            for(std::size_t k = 0; k < (std::size_t)DemographicProfile::Employment::Last; k++)
			{
				currTimeStepIncidentInfsRiskGenderEmployment[i][j][k] = 0;
				totalIncidentInfsRiskGenderEmployment[i][j][k] = 0;
			}
		}
	}

	currTimeStepNumInfected = 0;
	currTimeStepCD4InfectionSum = 0;
	currTimeStepCD4InfectionSumSq = 0;

    for(std::size_t i = 0; i < (std::size_t)DemographicProfile::Gender::Last; i++)
	{
		totalIncidentInfsGender[i] = 0;
		currTimeStepNumInfectedGender[i] = 0;
		currTimeStepAgeInfectionSumGender[i] = 0;
		currTimeStepAgeInfectionSumSqGender[i] = 0;
	}

    for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
	{
		totalIncidentInfsRiskCSW[i] = 0;
		totalIncidentInfsRisk[i] = 0;
	}

    for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
	{
        for(std::size_t j = 0; j < (std::size_t)DemographicProfile::Gender::Last; j++)
		{
            for(std::size_t k = 0; k < (std::size_t)DemographicProfile::Employment::Last; k++)
			{
				currTimeStepNumInfectedRiskGenderEmployment[i][j][k] = 0;
				currTimeStepAgeInfectionSumRiskGenderEmployment[i][j][k] = 0;
				currTimeStepAgeInfectionSumSqRiskGenderEmployment[i][j][k] = 0;
			}
		}
	}

	currTimeStep = 0;
	lastTwelveIncidenceRates = *(new deque<double>(12, 0.0));
}

unsigned long InfectionsTracker::getCurrTimeStepIncidentInfsTotal()
{
	unsigned long infections = 0;

	for(std::size_t i = 0; i < (std::size_t)Entity::HVLStrata::Last; ++i)
	{
		infections += currTimeStepIncidentInfs[i];
	}

	return infections;
}

double InfectionsTracker::calculateAnnualIncidence()
{
	assert(lastTwelveIncidenceRates.size() == 12);
	deque<double>::iterator it;
	double incidence = 0;

	for(it = lastTwelveIncidenceRates.begin(); it != lastTwelveIncidenceRates.end(); it++)
	{
		incidence += *(it);
	}

	return incidence;
}

void InfectionsTracker::addToDetailedTrace(DemographicProfile::ProfileID _profileID)
{
	profileIDsForDetailedTrace.push_back(_profileID);
}

unsigned long InfectionsTracker::getNumIncidentInfections()
{
	unsigned long infections = 0;

	for(std::size_t i = 0; i < (std::size_t)Entity::HVLStrata::Last; i++)
	{
		infections += totalIncidentInfections[i];
	}

	return infections;
}

/*
unsigned long InfectionsTracker::getNumIncidentInfections(DemographicProfile::ProfileID _infectorsProfileID,
        DemographicProfile::ProfileID _infectedsProfileID)
{
	assert(Utility::within_range(_infectorsProfileID, DemographicProfile::MIN, DemographicProfile::MAX));
	assert(Utility::within_range(_infectedsProfileID, DemographicProfile::MIN, DemographicProfile::MAX));
	//it seems that while loops are generally faster than for loops?
	auto currPartnershipType = SexualPartnership::Type::First;
	auto endPartnershipType = SexualPartnership::Type::ENDType;
	unsigned long infections = 0;

	//loop through each type of partnerships and tally the amount of infections where:
	//	the infectors had DmgProfie::ProfileID = _infectorsProfileID and
	//	the infecteds had DmgProfie::ProfileID = _infectedsProfileID
	while(currPartnershipType <= endPartnershipType)
	{
		throw std::runtime_error("how can this stop?");
		infections += incidentInfections[(int)currPartnershipType][_infectorsProfileID][_infectedsProfileID];
	}

	return infections;
}
*/

unsigned long InfectionsTracker::getNumIncidentInfections(SexualPartnership::Type _partnershipType,
        DemographicProfile::ProfileID _infectorsProfileID, DemographicProfile::ProfileID _infectedsProfileID)
{
	assert(_partnershipType != SexualPartnership::Type::ENDType);
	assert(Utility::within_range(_infectorsProfileID, DemographicProfile::MIN, DemographicProfile::MAX));
	assert(Utility::within_range(_infectedsProfileID, DemographicProfile::MIN, DemographicProfile::MAX));
	return incidentInfections[(int)_partnershipType][_infectorsProfileID][_infectedsProfileID];
}

//returns the prevalence rate among sexually active pop
double InfectionsTracker::getSAPrev(Population &_population)
{
	long totalInfected = 0;
    std::size_t currPopSize = _population.GetSize();
    std::size_t currSAPopSize = currPopSize - _population.GetNASize();

	//Currently Infected
	//total the current infections
	for(unsigned int i = 0; i < DemographicProfile::TotalNumBuckets; i++)
	{
		for(int j = 0; j < NUMBER_GENERATIONS_TO_TRACE; j++)
		{
			totalInfected += currPrevalentInfections[i][j];
		}
	}

	//current pop and prevalence of sexually active population
	//Need total number of infected for SA population only
	int totalInfectedSA = totalInfected;
	DemographicProfile NAProfile;
    NAProfile.set(DemographicProfile::Demographic::SexualActivityStatus, (std::size_t)DemographicProfile::SexualActivityStatus::NotActive);
	std::vector<DemographicProfile::ProfileID> NAProfileIDs;
	NAProfile.selectProfileIDs(NAProfileIDs, nullptr);

    for(std::size_t i = 0; i < NAProfileIDs.size(); i++)
	{
        for(std::size_t j = 0; j < NUMBER_GENERATIONS_TO_TRACE; j++)
		{
			totalInfectedSA -= currPrevalentInfections[NAProfileIDs[i]][j];
		}
	}

	if(currSAPopSize == 0)
	{
		return -1;
	}
	else
	{
		double currPrevalenceSA = (totalInfectedSA) / (double)currSAPopSize;
		return currPrevalenceSA;
	}
}

/*
* Resests counting of incident infections for time step
*/
void InfectionsTracker::resetIncidentInfections(long _time)
{
	/** Reset counter for incident infections and exposures for current timestep*/
    for(std::size_t i = 0; i < (std::size_t)Entity::HVLStrata::Last; i++)
	{
		currTimeStepIncidentInfs[i] = 0;
		currTimeExposures[i] = 0;
	}

    for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
	{
        for(std::size_t j = 0; j < (std::size_t)DemographicProfile::Gender::Last; j++)
		{
            for(std::size_t k = 0; k < (std::size_t)DemographicProfile::Employment::Last; k++)
			{
				currTimeStepIncidentInfsRiskGenderEmployment[i][j][k] = 0;
			}
		}
	}

	currTimeStepNumInfected = 0;
	currTimeStepCD4InfectionSum = 0;
	currTimeStepCD4InfectionSumSq = 0;

    for(std::size_t i = 0; i < (std::size_t)DemographicProfile::Gender::Last; i++)
	{
		currTimeStepAgeInfectionSumGender[i] = 0;
		currTimeStepAgeInfectionSumSqGender[i] = 0;
		currTimeStepNumInfectedGender[i] = 0;
	}

    for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
	{
        for(std::size_t j = 0; j < (std::size_t)DemographicProfile::Gender::Last; j++)
		{
            for(std::size_t k = 0; k < (std::size_t)DemographicProfile::Employment::Last; k++)
			{
				currTimeStepAgeInfectionSumRiskGenderEmployment[i][j][k] = 0;
				currTimeStepAgeInfectionSumSqRiskGenderEmployment[i][j][k] = 0;
				currTimeStepNumInfectedRiskGenderEmployment[i][j][k] = 0;
			}
		}
	}

	for(auto &ageRangeSize : currTimeStepIncidentInfsAgeMale)
	{
		ageRangeSize.second = 0;
	}

	for(auto &ageRangeSize : currTimeStepIncidentInfsAgeFemale)
	{
		ageRangeSize.second = 0;
	}

	currTimeStep = _time;
}


/*
initializes the counters for incident infections by age and gender
*/
void InfectionsTracker::initializeIncidentInfectionsByAge(const AgeRangeSizeContainer &_incMale, 
	const AgeRangeSizeContainer &_incFemale, const AgeRangeSizeContainer &_totalIncAge)
{
	currTimeStepIncidentInfsAgeMale = _incMale;
	currTimeStepIncidentInfsAgeFemale = _incFemale;
	totalIncidentInfsAge = _totalIncAge;
}

/*
 * Records a new exposure regardless of whether an infection happened or not
 */
void InfectionsTracker::recordExposure(long _time, const Entity *_infector)
{
	/** Reset counter for incident infections and exposures for current timestep if this is the first time an InfectionsTracker function has been called */
	if(_time > static_cast<long>(currTimeStep))
	{
		resetIncidentInfections(_time);
	}

    currTimeExposures[(std::size_t)_infector->getHVL()]++;
    totalExposures[(std::size_t)_infector->getHVL()]++;
}

//records a New infection and also prints the infection out to a trace
void InfectionsTracker::recordIncidentInfection(long _time, SexualPartnership::Type _partnershipType,
        const Entity *_infector, const Entity *_infected, bool _print, ostream &_traceOutStream)
{
	assert(_time >= 0);
	//check to see if the people are valid: not null, not dead, in a valid bucket
	assert((_infector != nullptr) && (_infector->isAlive())
	       && (Utility::within_range(_infector->getDemographicProfile()->getProfileID(), DemographicProfile::MIN, DemographicProfile::MAX)));
	assert((_infected != nullptr) && (_infected->isAlive())
	       && (Utility::within_range(_infected->getDemographicProfile()->getProfileID(), DemographicProfile::MIN, DemographicProfile::MAX)));
	assert(_partnershipType < SexualPartnership::Type::ENDType);

	//reset counter for incident infections and exposures for current timestep
	if(_time > static_cast<long>(currTimeStep))
	{
		resetIncidentInfections(_time);
	}

	//record infection
	++(incidentInfections[(int)_partnershipType][_infector->getDemographicProfile()->getProfileID()][_infected->getDemographicProfile()->getProfileID()]);

	//print out infection for trace
	if(_print)
	{
		_infector->print(_traceOutStream, Constants::TABTAB);
		_traceOutStream << Constants::TABTAB << "Just Infected (" << SexualPartnership::TypeStrings.at(_partnershipType) << "): ";
		_infected->print(_traceOutStream, Constants::BLANK);
	}

	//The person doing the infecting should be infected!
    assert(_infector->getHVL() != Entity::HVLStrata::UNINFECTED);
	currTimeStepIncidentInfs[(std::size_t)_infector->getHVL()]++;
    currTimeStepIncidentInfsRiskGenderEmployment[(std::size_t)_infected->getRiskLevel()][_infected->getDemographicProfileVal(
	            DemographicProfile::Demographic::Gender)][_infected->getDemographicProfileVal(DemographicProfile::Demographic::Employment)]++;
    totalIncidentInfections[(std::size_t)_infector->getHVL()]++;
	int infectedAge = _infected->getAge(TimeGranularity::Month);
	double infectorCD4 = _infector->cd4;
	DemographicProfile::Gender infectedGender = (DemographicProfile::Gender) _infected->getDemographicProfileVal(DemographicProfile::Demographic::Gender);
	Entity::RiskLevel infectedRisk = _infected->getRiskLevel();

	currTimeStepNumInfected++;
	currTimeStepCD4InfectionSum += infectorCD4;
	currTimeStepCD4InfectionSumSq += infectorCD4 * infectorCD4;
    currTimeStepNumInfectedGender[(std::size_t)infectedGender]++;
    currTimeStepAgeInfectionSumGender[(std::size_t)infectedGender] += infectedAge;
    currTimeStepAgeInfectionSumSqGender[(std::size_t)infectedGender] += infectedAge * infectedAge;
    currTimeStepNumInfectedRiskGenderEmployment[(std::size_t)infectedRisk][_infected->getDemographicProfileVal(
	            DemographicProfile::Demographic::Gender)][_infected->getDemographicProfileVal(DemographicProfile::Demographic::Employment)]++;
    currTimeStepAgeInfectionSumRiskGenderEmployment[(std::size_t)infectedRisk][_infected->getDemographicProfileVal(
	            DemographicProfile::Demographic::Gender)][_infected->getDemographicProfileVal(DemographicProfile::Demographic::Employment)] += infectedAge;
    currTimeStepAgeInfectionSumSqRiskGenderEmployment[(std::size_t)infectedRisk][_infected->getDemographicProfileVal(
	            DemographicProfile::Demographic::Gender)][_infected->getDemographicProfileVal(DemographicProfile::Demographic::Employment)] += infectedAge * infectedAge;

    if(_infected->getDemographicProfileVal(DemographicProfile::Demographic::Employment) == (std::size_t)DemographicProfile::Employment::Csw)
	{
        totalIncidentInfsRiskCSW[(std::size_t)infectedRisk]++;
	}

    totalIncidentInfsRisk[(std::size_t)_infected->getRiskLevel()]++;
    totalIncidentInfsRiskGenderEmployment[(std::size_t)_infected->getRiskLevel()][_infected->getDemographicProfileVal(
	            DemographicProfile::Demographic::Gender)][_infected->getDemographicProfileVal(DemographicProfile::Demographic::Employment)]++;

    bool isMale = _infected->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Male;
	auto &genderIncidentInfectionsByAge = isMale ? currTimeStepIncidentInfsAgeMale : currTimeStepIncidentInfsAgeFemale;
    totalIncidentInfsGender[isMale ? (std::size_t)DemographicProfile::Gender::Male : (std::size_t)DemographicProfile::Gender::Female]++;
	
	for(auto &ageRangeSize : genderIncidentInfectionsByAge)
	{
		if(infectedAge >= ageRangeSize.first.lower && infectedAge <= ageRangeSize.first.upper)
		{
			ageRangeSize.second++;
			break;
		}
	}

	for(auto &ageRangeSize : totalIncidentInfsAge)
	{
		if(infectedAge >= ageRangeSize.first.lower && infectedAge <= ageRangeSize.first.upper)
		{
			ageRangeSize.second++;
			break;
		}
	}
}

/**
records the cd4 at transmission (requested by clinical out stream)
**/
void InfectionsTracker::recordCD4AtTransmission(ostream &_outStream)
{
	if(currTimeStepNumInfected != 0)
	{
		double cd4Mean = currTimeStepCD4InfectionSum / currTimeStepNumInfected;
		double cd4SD = sqrt(currTimeStepCD4InfectionSumSq / currTimeStepNumInfected - cd4Mean * cd4Mean);
		_outStream << currTimeStepNumInfected << Constants::TAB << cd4Mean << Constants::TAB << cd4SD << Constants::TAB;
	}
	else
	{
		_outStream << currTimeStepNumInfected << Constants::TAB << "N/A" << Constants::TAB << "N/A" << Constants::TAB;
	}
}

void InfectionsTracker::setPrevalentInfections(long /*_time*/,
        unsigned long _prevalenceByBucket[DemographicProfile::TotalNumBuckets][NUMBER_GENERATIONS_TO_TRACE],
		const AgeRangeSizeContainer &_prevalenceByAgeMale,
		const AgeRangeSizeContainer &_prevalenceByAgeFemale,
        unsigned long
        _prevalenceByRiskGenderEmployment[(std::size_t)Entity::RiskLevel::Last][(std::size_t)DemographicProfile::Gender::Last][(std::size_t)DemographicProfile::Employment::Last])
{
	DemographicProfile::ProfileID currProfileID = DemographicProfile::MIN;

	while(currProfileID <= DemographicProfile::MAX)
	{
		for(int i = 0; i < NUMBER_GENERATIONS_TO_TRACE; i++)
		{
			currPrevalentInfections[currProfileID][i] = _prevalenceByBucket[currProfileID][i];
		}

		currProfileID++;
	}

	for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
	{
        for(std::size_t j = 0; j < (std::size_t)DemographicProfile::Gender::Last; j++)
		{
            for(std::size_t k = 0; k < (std::size_t)DemographicProfile::Employment::Last; k++)
			{
				currPrevalentInfectionsRiskGenderEmployment[i][j][k] = _prevalenceByRiskGenderEmployment[i][j][k];
			}
		}
	}

	currPrevalentInfectionsAgeMale = _prevalenceByAgeMale;
	currPrevalentInfectionsAgeFemale = _prevalenceByAgeFemale;
}

int InfectionsTracker::printInfections(EventParams &_eventParams, long _time, ostream &_outStream, Population *_population)
{
	assert(_time >= 0);
	//total # infections this month
	int totalInfected = 0;
	//current population size
    std::size_t currPopSize = _population->GetSize();
    std::size_t currSAPopSize = currPopSize - _population->GetNASize();
	//total # of age ranges to print out
    auto currSizeByAgeRange = _population->GetAgeRanges();
	int numAgeRanges = (int)currSizeByAgeRange.size();

	//write headers for infections sheet
	if(_time == 0)
	{
		ostringstream firstRow;
		ostringstream secondRow;
		ostringstream thirdRow;
		//incidence-related headers
		firstRow << "Epidemiology Outputs" << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << "Month" << Constants::TAB;
		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << "New Infections" << Constants::TAB;
		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << "Total Infected in History (Prevalent Cases Excluded)" << Constants::TAB;
		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << "Currently Infected" << Constants::TAB;
		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << "Pop Size" << Constants::TAB;
		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << "Prevalence" << Constants::TAB;
		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << "SA Pop Size" << Constants::TAB;
		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << "SA Prevalence" << Constants::TAB;
		//write out headers for population by age\sexual activity
		firstRow << "Prevalent Cases" << Constants::TAB;
		secondRow << "Non-Sexually Active Population" << Constants::TAB;
		thirdRow << "All ages" << Constants::TAB;

		for(int i = 0; i < numAgeRanges; i++)
		{
			if(i == 0)
			{
				secondRow << "Sexually Active Population";
			}

			firstRow << Constants::TAB;
			secondRow << Constants::TAB;
			thirdRow << currSizeByAgeRange.at(i).lower << "-" << currSizeByAgeRange.at(i).upper << Constants::TAB;
		}

		//write out headers for population by gender
		firstRow << "Prevalent Cases" << Constants::TAB << Constants::TAB;
		secondRow << "Gender" << Constants::TAB << Constants::TAB;
		thirdRow << "Males" << Constants::TAB << "Females" << Constants::TAB;
		//write out headers for population by gender and age
		firstRow << "Males" << Constants::TAB;
		secondRow << "Non-Sexually Active Population" << Constants::TAB;
		thirdRow << "All Ages" << Constants::TAB;

		for(int i = 0; i < numAgeRanges; i++)
		{
			if(i == 0)
			{
				secondRow << "Sexually Active Population";
			}

			firstRow << Constants::TAB;
			secondRow << Constants::TAB;
			thirdRow << currPrevalentInfectionsAgeMale.at(i).first.lower << "-" << currPrevalentInfectionsAgeMale.at(i).first.upper << Constants::TAB;
		}

		firstRow << "Females" << Constants::TAB;
		secondRow << "Non-Sexually Active Population" << Constants::TAB;
		thirdRow << "All Ages" << Constants::TAB;

		for(int i = 0; i < numAgeRanges; i++)
		{
			if(i == 0)
			{
				secondRow << "Sexually Active Population";
			}

			firstRow << Constants::TAB;
			secondRow << Constants::TAB;
			thirdRow << currPrevalentInfectionsAgeFemale.at(i).first.lower << "-" << currPrevalentInfectionsAgeFemale.at(i).first.upper << Constants::TAB;
		}

		//write out headers for number infected by risk
		firstRow << "Prevalent Cases" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB
		         << Constants::TAB;
		secondRow << "Risk Group" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB;
		thirdRow << "CSW High Risk" << Constants::TAB << "CSW Low Risk" << Constants::TAB << "Non-CSW High Risk Male" <<
		         Constants::TAB << "Non-CSW High Risk Female" << Constants::TAB << "Non-CSW Low Risk Male" << Constants::TAB <<
		         "Non-CSW Low Risk Female" << Constants::TAB;
		//write out headers for number of infections by generation (7 tabs)
		firstRow << "Prevalent Cases" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB
		         << Constants::TAB;
		secondRow << "Number Infected by Generation" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB;
		thirdRow << "0 (Prev.)" << Constants::TAB << "First" << Constants::TAB << "Second" << Constants::TAB << "Third" <<
		         Constants::TAB << "Fourth" << Constants::TAB << "Fifth+" << Constants::TAB;
		//write out headers for number of exposures and infections by HVL
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << Constants::TAB << "Exposures by HVL" << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << Constants::TAB << "HVL 0-20" << Constants::TAB << "HVL 21-500" << Constants::TAB << "HVL 501-3000" <<
		         Constants::TAB << "HVL 3001-10000" << Constants::TAB << "HVL 10000-30000" << Constants::TAB << "HVL 30001-100000" <<
		         Constants::TAB << "HVL 100000+" << Constants::TAB << "HVL Primary" << Constants::TAB << "HVL Late Stage" <<
		         Constants::TAB;
		firstRow << Constants::TAB << "Incident Cases" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB
		         << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << Constants::TAB << "Infections by HVL (of Infector)" << Constants::TAB << Constants::TAB << Constants::TAB
		          << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << Constants::TAB << "HVL 0-20" << Constants::TAB << "HVL 21-500" << Constants::TAB << "HVL 501-3000" <<
		         Constants::TAB << "HVL 3001-10000" << Constants::TAB << "HVL 10000-30000" << Constants::TAB << "HVL 30001-100000" <<
		         Constants::TAB << "HVL 100000+" << Constants::TAB << "HVL Primary" << Constants::TAB << "HVL Late Stage" <<
		         Constants::TAB;

		//write out headers for number of infections by Age and gender
		for(int i = 0; i < numAgeRanges; i++)
		{
			if(i == 0)
			{
				firstRow << "Incident Cases";
				secondRow << "Infections By Age";
			}

			firstRow << Constants::TAB;
			secondRow << Constants::TAB;
			thirdRow << currSizeByAgeRange.at(i).lower << "-" << currSizeByAgeRange.at(i).upper << Constants::TAB;
		}

		firstRow << "Incident Cases" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB
		         << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "Gender" << Constants::TAB << Constants::TAB << "Risk Group" << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << "Male" << Constants::TAB << "Female" << Constants::TAB << "CSW High Risk" << Constants::TAB <<
		         "CSW Low Risk" << Constants::TAB << "Non-CSW High Risk Male" << Constants::TAB << "Non-CSW High Risk Female" <<
		         Constants::TAB << "Non-CSW Low Risk Male" << Constants::TAB << "Non-CSW Low Risk Female" << Constants::TAB;

		for(int i = 0; i < numAgeRanges; i++)
		{
			if(i == 0)
			{
				firstRow << "Incident Cases";
				secondRow << "Male (By Age)";
			}

			firstRow << Constants::TAB;
			secondRow << Constants::TAB;
			thirdRow << currSizeByAgeRange.at(i).lower << "-" << currSizeByAgeRange.at(i).upper << Constants::TAB;
		}

		for(int i = 0; i < numAgeRanges; i++)
		{
			if(i == 0)
			{
				firstRow << "Incident Cases";
				secondRow << "Female (By Age)";
			}

			firstRow << Constants::TAB;
			secondRow << Constants::TAB;
			thirdRow << currSizeByAgeRange.at(i).lower << "-" << currSizeByAgeRange.at(i).upper << Constants::TAB;
		}

		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << Constants::TAB;

		//write out headers for incident infections by relationship type
		for(int i = 0; i < (int)SexualPartnership::Type::ENDType; i++)
		{
			if(i == 0)
			{
				firstRow << "Total Infected in History (Prevalent Cases Excluded)";
				secondRow << "Relationship Type";
			}

			firstRow << Constants::TAB;
			secondRow << Constants::TAB;
			thirdRow << SexualPartnership::TypeStrings.at(SexualPartnership::Type(i)) << Constants::TAB;
		}

		firstRow << Constants::TAB;
		secondRow << "Infectors:" << Constants::TAB;
		thirdRow << "Infecteds:" << Constants::TAB;
		//write out headers that tally infections from one DemographicProfile to another
		//loop through all used ProfileID's and create internal string buffer headers for future timestep trace output
		auto infectorProfileID = profileIDsForDetailedTrace.begin();

		while(infectorProfileID != profileIDsForDetailedTrace.end())
		{
			//first row profile str refers to infectors
			firstRow << "Total Infected in History (Prevalent Cases Excluded)";
			secondRow << *DemographicProfile::toString(*infectorProfileID);
			auto infectedProfileID = profileIDsForDetailedTrace.begin();

			while(infectedProfileID != profileIDsForDetailedTrace.end())
			{
				firstRow << Constants::TAB;
				secondRow << Constants::TAB;
				//second row profile str refers to infecteds
				thirdRow << *DemographicProfile::toString(*infectedProfileID) << Constants::TAB;
				infectedProfileID++;
			}

			infectorProfileID++;
		}

		//write out headers that tally total infections based on age and gender and risk group
		for(int i = 0; i < numAgeRanges; i++)
		{
			if(i == 0)
			{
				firstRow << "Total Infected in History (Prevalent Cases Excluded)";
				secondRow << "Infections By Age";
			}

			firstRow << Constants::TAB;
			secondRow << Constants::TAB;
			thirdRow << currSizeByAgeRange.at(i).lower << "-" << currSizeByAgeRange.at(i).upper << Constants::TAB;
		}

		firstRow << "Total Infected in History (Prevalent Cases Excluded)" << Constants::TAB << Constants::TAB;
		secondRow << "Gender" << Constants::TAB << Constants::TAB;
		thirdRow << "Male" << Constants::TAB << "Female" << Constants::TAB;
		firstRow << "Total Infected in History (Prevalent Cases Excluded)" << Constants::TAB << Constants::TAB << Constants::TAB
		         << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "Risk Group" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB;
		thirdRow << "CSW High Risk" << Constants::TAB << "CSW Low Risk" << Constants::TAB << "Non-CSW High Risk Male" <<
		         Constants::TAB << "Non-CSW High Risk Female" << Constants::TAB << "Non-CSW Low Risk Male" << Constants::TAB <<
		         "Non-CSW Low Risk Female" << Constants::TAB;
		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << Constants::TAB;
		firstRow << "Age at Infection" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "Male" << Constants::TAB << Constants::TAB << "Female" << Constants::TAB << Constants::TAB;
		thirdRow << "Mean" << Constants::TAB << "SD" << Constants::TAB << "Mean" << Constants::TAB << "SD" << Constants::TAB;
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "CSW" << Constants::TAB << Constants::TAB << "CSW High Risk" << Constants::TAB << Constants::TAB <<
		          "CSW Low Risk"  << Constants::TAB << Constants::TAB;
		thirdRow << "Mean" << Constants::TAB << "SD" << Constants::TAB << "Mean" << Constants::TAB << "SD" << Constants::TAB <<
		         "Mean" << Constants::TAB << "SD" << Constants::TAB;
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB << Constants::TAB;
		secondRow << "Non-CSW High Risk Male" << Constants::TAB << Constants::TAB << "Non-CSW High Risk Female" <<
		          Constants::TAB << Constants::TAB << "Non-CSW Low Risk Male"  << Constants::TAB << Constants::TAB <<
		          "Non-CSW Low Risk Female"  << Constants::TAB << Constants::TAB;
		thirdRow << "Mean" << Constants::TAB << "SD" << Constants::TAB << "Mean" << Constants::TAB << "SD" << Constants::TAB <<
		         "Mean" << Constants::TAB << "SD" << Constants::TAB << "Mean" << Constants::TAB << "SD" << Constants::TAB;
		//write out string buffers to trace file
		_outStream << firstRow.str() << std::endl;
		_outStream << secondRow.str() << std::endl;
		_outStream << thirdRow.str() << std::endl;
	}

	//if no incident infections happened during this time, then make sure that we have 0 in the currTime incident infections and exposures
	if(_time > static_cast<long>(currTimeStep))
	{
		resetIncidentInfections(_time);
	}

	//get current total infections
	//Month
	if(_time == 0)
	{
		_outStream << "init" << Constants::TAB;
	}
	else
	{
		_outStream << _time << Constants::TAB;
	}

	//New Infections
	_outStream << getCurrTimeStepIncidentInfsTotal() << Constants::TAB;

	//Print to BatchStats file if NEWINFECTIONS stream is open
    if(_eventParams.BatchStatsStream[BatchStatsVariables::NEWINFECTIONS].is_open())
	{
        _eventParams.BatchStatsStream[BatchStatsVariables::NEWINFECTIONS] << getCurrTimeStepIncidentInfsTotal() << Constants::TAB;
	}

	double monthlyIncidence = 1.0 * (getCurrTimeStepIncidentInfsTotal()) / (double)currPopSize;
	//Push the monthly incidence onto the deque
	lastTwelveIncidenceRates.push_back(monthlyIncidence);
	//Pop off the oldest incidence rate
	lastTwelveIncidenceRates.pop_front();
	assert(lastTwelveIncidenceRates.size() == 12);
	//Calculate yearly incidence rate
	double incidence = calculateAnnualIncidence();

	//Print to BatchStats file if INCIDENCE stream is open
    if(_eventParams.BatchStatsStream[BatchStatsVariables::INCIDENCE].is_open())
	{
        _eventParams.BatchStatsStream[BatchStatsVariables::INCIDENCE] << incidence << Constants::TAB;
	}

	//Total Infected in History
	_outStream << getNumIncidentInfections() << Constants::TAB;

	//Currently Infected
	//total the current infections
	for(unsigned int i = 0; i < DemographicProfile::TotalNumBuckets; i++)
	{
		for(int j = 0; j < NUMBER_GENERATIONS_TO_TRACE; j++)
		{
			totalInfected += currPrevalentInfections[i][j];
		}
	}

	_outStream << totalInfected << Constants::TAB;

	//Print to BatchStats file if CURRENTLYINFECTED stream is open
	if(_eventParams.BatchStatsStream[BatchStatsVariables::CURRENTLYINFECTED].is_open())
	{
        _eventParams.BatchStatsStream[BatchStatsVariables::CURRENTLYINFECTED] << totalInfected << Constants::TAB;
	}

	//Total Population Size
	_outStream << currPopSize << Constants::TAB;

	//Print to BatchStats file if POPULATION stream is open
    if(_eventParams.BatchStatsStream[BatchStatsVariables::POPULATION].is_open())
	{
        _eventParams.BatchStatsStream[BatchStatsVariables::POPULATION] << currPopSize << Constants::TAB;
	}

	//current prevalence
	double currPrevalence = totalInfected / (double)currPopSize;
	_outStream << currPrevalence << Constants::TAB;

	//Print to BatchStats file if PREVALENCE stream is open
    if(_eventParams.BatchStatsStream[BatchStatsVariables::PREVALENCE].is_open())
	{
        _eventParams.BatchStatsStream[BatchStatsVariables::PREVALENCE] << currPrevalence << Constants::TAB;
	}

	//current pop and prevalence of sexually active population
	//Need total number of infected for SA population only
	int totalInfectedSA = totalInfected;
	DemographicProfile NAProfile;
    NAProfile.set(DemographicProfile::Demographic::SexualActivityStatus, (std::size_t)DemographicProfile::SexualActivityStatus::NotActive);
	std::vector<DemographicProfile::ProfileID> NAProfileIDs;
	NAProfile.selectProfileIDs(NAProfileIDs, nullptr);

	for(size_t i = 0; i < NAProfileIDs.size(); i++)
	{
		for(size_t j = 0; j < NUMBER_GENERATIONS_TO_TRACE; j++)
		{
			totalInfectedSA -= currPrevalentInfections[NAProfileIDs[i]][j];
		}
	}

	double currPrevalenceSA = (totalInfectedSA) / (double)currSAPopSize;
	_outStream << currSAPopSize << Constants::TAB;
	_outStream << currPrevalenceSA << Constants::TAB;

	//Print up a batchstats file!
    if(_eventParams.BatchStatsStream[BatchStatsVariables::PREVALENCESA].is_open())
	{
        _eventParams.BatchStatsStream[BatchStatsVariables::PREVALENCESA] << currPrevalenceSA << Constants::TAB;
	}

	//Multiply by 100 and round to nearest integer for graphical output
	int intPrevalence = (int)(100 * currPrevalence + 0.5);
	//prev cases by age\sexual activity
	int totalInfectedNA = totalInfected - totalInfectedSA;
	_outStream << totalInfectedNA << Constants::TAB;

	for(int i = 0; i < numAgeRanges; i++)
	{
		_outStream << currPrevalentInfectionsAgeMale.at(i).second + currPrevalentInfectionsAgeFemale.at(i).second << Constants::TAB;
	}

	//Print out population size and number infected by gender
    int totalInfectedGender[(std::size_t)DemographicProfile::Gender::Last];
    int totalInfectedSAGender[(std::size_t)DemographicProfile::Gender::Last];
    totalInfectedGender[(std::size_t)DemographicProfile::Gender::Male] = 0;
    totalInfectedSAGender[(std::size_t)DemographicProfile::Gender::Male] = 0;
    totalInfectedGender[(std::size_t)DemographicProfile::Gender::Female] = 0;
    totalInfectedSAGender[(std::size_t)DemographicProfile::Gender::Female] = 0;
	DemographicProfile GenderProfile;

	for(int i = 0; i < numAgeRanges; i++)
	{
        totalInfectedSAGender[(std::size_t)DemographicProfile::Gender::Male] += (int)currPrevalentInfectionsAgeMale.at(i).second;
        totalInfectedSAGender[(std::size_t)DemographicProfile::Gender::Female] += (int)currPrevalentInfectionsAgeFemale.at(i).second;
	}

	//First tally the infected men
    GenderProfile.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Male);
	std::vector<DemographicProfile::ProfileID> GenderProfileIDs;
	GenderProfile.selectProfileIDs(GenderProfileIDs, nullptr);

	for(size_t i = 0; i < GenderProfileIDs.size(); i++)
	{
		for(size_t j = 0; j < NUMBER_GENERATIONS_TO_TRACE; j++)
		{
            totalInfectedGender[(std::size_t)DemographicProfile::Gender::Male] += currPrevalentInfections[GenderProfileIDs[i]][j];
		}
	}

	//Next tally the infected women
    GenderProfile.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Female);
	GenderProfileIDs.clear();
	GenderProfile.selectProfileIDs(GenderProfileIDs, nullptr);

	for(size_t i = 0; i < GenderProfileIDs.size(); i++)
	{
		for(size_t j = 0; j < NUMBER_GENERATIONS_TO_TRACE; j++)
		{
            totalInfectedGender[(std::size_t)DemographicProfile::Gender::Female] += currPrevalentInfections[GenderProfileIDs[i]][j];
		}
	}

	//Actually print the size and infections by gender
    _outStream << totalInfectedGender[(std::size_t)DemographicProfile::Gender::Male] << Constants::TAB << totalInfectedGender[(std::size_t)DemographicProfile::Gender::Female] <<
	           Constants::TAB;
	//output infections by age and gender
    _outStream << totalInfectedGender[(std::size_t)DemographicProfile::Gender::Male] - totalInfectedSAGender[(std::size_t)DemographicProfile::Gender::Male] << Constants::TAB;

	for(int i = 0; i < numAgeRanges; i++)
	{
		_outStream << currPrevalentInfectionsAgeMale.at(i).second << Constants::TAB;
	}

    _outStream << totalInfectedGender[(std::size_t)DemographicProfile::Gender::Female] - totalInfectedSAGender[(std::size_t)DemographicProfile::Gender::Female] << Constants::TAB;

	for(int i = 0; i < numAgeRanges; i++)
	{
		_outStream << currPrevalentInfectionsAgeFemale.at(i).second << Constants::TAB;
	}

	//output prevalent infections by risk
	int totalInfectedCSW = 0;
	DemographicProfile CSWProfile;
    CSWProfile.set(DemographicProfile::Demographic::Employment, (std::size_t)DemographicProfile::Employment::Csw);
	std::vector<DemographicProfile::ProfileID> CSWProfileIDs;
	CSWProfile.selectProfileIDs(CSWProfileIDs, nullptr);

	for(size_t i = 0; i < CSWProfileIDs.size(); i++)
	{
		for(size_t j = 0; j < NUMBER_GENERATIONS_TO_TRACE; j++)
		{
			totalInfectedCSW += currPrevalentInfections[CSWProfileIDs[i]][j];
		}
	}

    _outStream << currPrevalentInfectionsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Gender::Male][(std::size_t)DemographicProfile::Employment::Csw] +
        currPrevalentInfectionsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Gender::Female][(std::size_t)DemographicProfile::Employment::Csw] << Constants::TAB
        << currPrevalentInfectionsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Gender::Male][(std::size_t)DemographicProfile::Employment::Csw] +
        currPrevalentInfectionsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Gender::Female][(std::size_t)DemographicProfile::Employment::Csw] << Constants::TAB <<
        currPrevalentInfectionsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Gender::Male][(std::size_t)DemographicProfile::Employment::NonCsw] << Constants::TAB
        << currPrevalentInfectionsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Gender::Female][(std::size_t)DemographicProfile::Employment::NonCsw] <<
        Constants::TAB << currPrevalentInfectionsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Gender::Male][(std::size_t)DemographicProfile::Employment::NonCsw]
	           << Constants::TAB <<
               currPrevalentInfectionsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Gender::Female][(std::size_t)DemographicProfile::Employment::NonCsw] <<
	           Constants::TAB;
	//We're hard wiring 6 (Prev + 5) generations of reporting for now
	int totalInfectedByGeneration[NUMBER_GENERATIONS_TO_TRACE];

	for(int i = 0; i < NUMBER_GENERATIONS_TO_TRACE; i++)
	{
		totalInfectedByGeneration[i] = 0;

		for(unsigned int j = 0; j < DemographicProfile::TotalNumBuckets; j++)
		{
			totalInfectedByGeneration[i] += currPrevalentInfections[j][i];
		}
	}

	//write out number of infections by generation (7 tabs)
	for(int i = 0; i < NUMBER_GENERATIONS_TO_TRACE; i++)
	{
		_outStream << totalInfectedByGeneration[i] << Constants::TAB;
	}

	/** Print out exposures and infections by viral load */
	_outStream << Constants::TAB;

	for(std::size_t i = 0; i < (std::size_t)Entity::HVLStrata::Last; i++)
	{
		_outStream << currTimeExposures[i] << Constants::TAB;
	}

	_outStream << Constants::TAB;

	for(std::size_t i = 0; i < (std::size_t)Entity::HVLStrata::Last; i++)
	{
		_outStream << currTimeStepIncidentInfs[i] << Constants::TAB;
	}

	//Print out incident infections by age and gender and risk
	int incidentInfsMale = 0;
	int incidentInfsFemale = 0;

	for(int i = 0; i < numAgeRanges; i++)
	{
		_outStream << currTimeStepIncidentInfsAgeMale.at(i).second + currTimeStepIncidentInfsAgeFemale.at(i).second << Constants::TAB;
		incidentInfsMale += (int)currTimeStepIncidentInfsAgeMale.at(i).second;
		incidentInfsFemale += (int)currTimeStepIncidentInfsAgeFemale.at(i).second;
	}

	_outStream << incidentInfsMale << Constants::TAB << incidentInfsFemale << Constants::TAB;
    _outStream << currTimeStepIncidentInfsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Gender::Male][(std::size_t)DemographicProfile::Employment::Csw] +
        currTimeStepIncidentInfsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Gender::Female][(std::size_t)DemographicProfile::Employment::Csw] << Constants::TAB
        << currTimeStepIncidentInfsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Gender::Male][(std::size_t)DemographicProfile::Employment::Csw] +
        currTimeStepIncidentInfsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Gender::Female][(std::size_t)DemographicProfile::Employment::Csw] << Constants::TAB
        << currTimeStepIncidentInfsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Gender::Male][(std::size_t)DemographicProfile::Employment::NonCsw] <<
	           Constants::TAB <<
               currTimeStepIncidentInfsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Gender::Female][(std::size_t)DemographicProfile::Employment::NonCsw] <<
               Constants::TAB << currTimeStepIncidentInfsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Gender::Male][(std::size_t)DemographicProfile::Employment::NonCsw]
	           << Constants::TAB <<
               currTimeStepIncidentInfsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Gender::Female][(std::size_t)DemographicProfile::Employment::NonCsw] <<
	           Constants::TAB;

	for(int i = 0; i < numAgeRanges; i++)
	{
		_outStream << currTimeStepIncidentInfsAgeMale.at(i).second << Constants::TAB;
	}

	for(int i = 0; i < numAgeRanges; i++)
	{
		_outStream << currTimeStepIncidentInfsAgeFemale.at(i).second << Constants::TAB;
	}

	// Print out incident infections by relationship type
	_outStream << Constants::TAB;

	for(int partnershipType = 0; partnershipType < (int)SexualPartnership::Type::ENDType; partnershipType++)
	{
		int infectionsByType = 0;
		//tally all the infections that happened with partnershipType for all possible infector and infected DemographicProfiles
		auto infectorProfileID = profileIDsForDetailedTrace.begin();

		while(infectorProfileID != profileIDsForDetailedTrace.end())
		{
			auto infectedProfileID = profileIDsForDetailedTrace.begin();

			while(infectedProfileID != profileIDsForDetailedTrace.end())
			{
				infectionsByType += incidentInfections[partnershipType][*infectorProfileID][*infectedProfileID];
				infectedProfileID++;
			}

			infectorProfileID++;
		}

		_outStream << infectionsByType << Constants::TAB;
	}

	_outStream << Constants::TAB;
	//write out all incident infections that happened in history
	// only ProfileID's in profileIDsForDetailedTrace are included
	auto infectorProfileID = profileIDsForDetailedTrace.begin();

	while(infectorProfileID != profileIDsForDetailedTrace.end())
	{
		auto infectedProfileID = profileIDsForDetailedTrace.begin();

		while(infectedProfileID != profileIDsForDetailedTrace.end())
		{
			//tally all the infections that happened from curr infectorProfileID -> curr infectedProfileID
			unsigned long infs = 0;

			for(int partnershipType = 0; partnershipType < (int)SexualPartnership::Type::ENDType; ++partnershipType)
			{
				infs += incidentInfections[partnershipType][*infectorProfileID][*infectedProfileID];
			}

			_outStream << infs << Constants::TAB;
			infectedProfileID++;
		}

		infectorProfileID++;
	}

	//write out all incident infections that happened in history stratified by age and gender and risk
	for(int i = 0; i < numAgeRanges; i++)
	{
		_outStream << totalIncidentInfsAge.at(i).second << Constants::TAB;
	}

    _outStream << totalIncidentInfsGender[(std::size_t)DemographicProfile::Gender::Male] << Constants::TAB <<
        totalIncidentInfsGender[(std::size_t)DemographicProfile::Gender::Female] << Constants::TAB;
    _outStream << totalIncidentInfsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Gender::Male][(std::size_t)DemographicProfile::Employment::Csw] +
        totalIncidentInfsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Gender::Female][(std::size_t)DemographicProfile::Employment::Csw] << Constants::TAB <<
        totalIncidentInfsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Gender::Male][(std::size_t)DemographicProfile::Employment::Csw] +
        totalIncidentInfsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Gender::Female][(std::size_t)DemographicProfile::Employment::Csw] << Constants::TAB <<
        totalIncidentInfsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Gender::Male][(std::size_t)DemographicProfile::Employment::NonCsw] << Constants::TAB <<
        totalIncidentInfsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Gender::Female][(std::size_t)DemographicProfile::Employment::NonCsw] << Constants::TAB <<
        totalIncidentInfsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Gender::Male][(std::size_t)DemographicProfile::Employment::NonCsw] << Constants::TAB <<
        totalIncidentInfsRiskGenderEmployment[(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Gender::Female][(std::size_t)DemographicProfile::Employment::NonCsw] << Constants::TAB;
	//write out age of infection for incident infections that month (mean and SD)
	_outStream << Constants::TAB;

    for(std::size_t i = 0; i < (std::size_t)DemographicProfile::Gender::Last; i++)
	{
		if(currTimeStepNumInfectedGender[i] != 0)
		{
			double ageMean = currTimeStepAgeInfectionSumGender[i] / (double) currTimeStepNumInfectedGender[i];
			double ageSD = sqrt(currTimeStepAgeInfectionSumSqGender[i] / (double) currTimeStepNumInfectedGender[i] -
			                    ageMean * ageMean);
			_outStream << ageMean << Constants::TAB << ageSD << Constants::TAB;
		}
		else
		{
			_outStream << "N/A" << Constants::TAB << "N/A" << Constants::TAB;
		}
	}

	double numInfectedCSW = 0;

	for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
	{
        for(std::size_t j = 0; j < (std::size_t)DemographicProfile::Gender::Last; j++)
		{
            numInfectedCSW += currTimeStepNumInfectedRiskGenderEmployment[i][j][(std::size_t)DemographicProfile::Employment::Csw];
		}
	}

	if(numInfectedCSW != 0)
	{
		double ageSumCSW  = 0;
		double ageSumSqCSW = 0;

		for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
		{
            for(std::size_t j = 0; j < (std::size_t)DemographicProfile::Gender::Last; j++)
			{
                ageSumCSW += currTimeStepAgeInfectionSumRiskGenderEmployment[i][j][(std::size_t)DemographicProfile::Employment::Csw];
                ageSumSqCSW += currTimeStepAgeInfectionSumSqRiskGenderEmployment[i][j][(std::size_t)DemographicProfile::Employment::Csw];
			}
		}

		double ageMeanCSW = ageSumCSW / numInfectedCSW;
		double ageSDCSW = sqrt(ageSumSqCSW / numInfectedCSW - ageMeanCSW * ageMeanCSW);
		_outStream << ageMeanCSW << Constants::TAB << ageSDCSW << Constants::TAB;
	}
	else
	{
		_outStream << "N/A" << Constants::TAB << "N/A" << Constants::TAB;
	}

	for(int i = (std::size_t)Entity::RiskLevel::Last - 1; i >= 0; i--)
	{
		double numInfected = 0;

        for(std::size_t j = 0; j < (std::size_t)DemographicProfile::Gender::Last; j++)
		{
            numInfected += currTimeStepNumInfectedRiskGenderEmployment[i][j][(std::size_t)DemographicProfile::Employment::Csw];
		}

		if(numInfected != 0)
		{
			double ageSum  = 0;
			double ageSumSq = 0;

            for(std::size_t j = 0; j < (std::size_t)DemographicProfile::Gender::Last; j++)
			{
                ageSum += currTimeStepAgeInfectionSumRiskGenderEmployment[i][j][(std::size_t)DemographicProfile::Employment::Csw];
                ageSumSq += currTimeStepAgeInfectionSumSqRiskGenderEmployment[i][j][(std::size_t)DemographicProfile::Employment::Csw];
			}

			double ageMean = ageSum / numInfected;
			double ageSD = sqrt(ageSumSq / numInfected - ageMean * ageMean);
			_outStream << ageMean << Constants::TAB << ageSD << Constants::TAB;
		}
		else
		{
			_outStream << "N/A" << Constants::TAB << "N/A" << Constants::TAB;
		}
	}

	for(int i = (std::size_t)Entity::RiskLevel::Last - 1; i >= 0; i--)
	{
        for(std::size_t j = 0; j < (std::size_t)DemographicProfile::Gender::Last; j++)
		{
            double numInfected = currTimeStepNumInfectedRiskGenderEmployment[i][j][(std::size_t)DemographicProfile::Employment::NonCsw];

			if(numInfected != 0)
			{
                double ageSum = currTimeStepAgeInfectionSumRiskGenderEmployment[i][j][(std::size_t)DemographicProfile::Employment::NonCsw];
                double ageSumSq = currTimeStepAgeInfectionSumSqRiskGenderEmployment[i][j][(std::size_t)DemographicProfile::Employment::NonCsw];
				double ageMean = ageSum / numInfected;
				double ageSD = sqrt(ageSumSq / numInfected - ageMean * ageMean);
				_outStream << ageMean << Constants::TAB << ageSD << Constants::TAB;
			}
			else
			{
				_outStream << "N/A" << Constants::TAB << "N/A" << Constants::TAB;
			}
		}
	}

	_outStream << std::endl;
	return intPrevalence;
}

} // namespace transm
