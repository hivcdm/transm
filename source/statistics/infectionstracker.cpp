#include <iostream>
#include <numeric>
#include <sstream>

#include "infectionstracker.hpp"
#include "core/constants.hpp"
#include "core/population.hpp"

namespace transm {

InfectionsTracker::InfectionsTracker() : 
  lastTwelveIncidenceRates(12, 0.0)
{
	//zero out all infection tallies
	for(int i = 0; i < (int)SexualPartnership::Type::ENDType; ++i)
	{
		for(unsigned int j = 0; j < DemographicProfile::TotalNumBuckets; ++j)
		{
			for(int g = 0; g < NUMBER_GENERATIONS_TO_TRACE; g++)
			{
				currPrevalentInfections[g][j] = 0;
			}

			for(unsigned int k = 0; k < DemographicProfile::TotalNumBuckets; ++k)
			{
				incidentInfectionsByDemographic[i][j][k] = 0;
			}
		}

        for(int j = 0; j < 4; j++)
        {
            for(int k = 0; k < 4; k++)
            {
                incidentInfectionsByEntityType[i][j][k] = 0;
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

    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
        for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
        {
            for(std::size_t k = 0; k < (std::size_t)DemographicProfile::Employment::Last; k++)
            {
                currTimeStepIncidentInfsEntityTypeRiskEmployment[entity_type][i][k] = 0;
                totalIncidentInfsEntityTypeRiskEmployment[entity_type][i][k] = 0;
            }
        }
    }

	currTimeStepNumInfected = 0;
	currTimeStepCD4InfectionSum = 0;
	currTimeStepCD4InfectionSumSq = 0;

    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
		totalIncidentInfsEntityType[entity_type] = 0;
        currTimeStepNumInfectedEntityType[entity_type] = 0;
        currTimeStepAgeInfectionSumEntityType[entity_type] = 0;
		currTimeStepAgeInfectionSumSqEntityType[entity_type] = 0;
	}

    for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
	{
		totalIncidentInfsRiskCSW[i] = 0;
		totalIncidentInfsRisk[i] = 0;
	}

    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
        for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
        {
            for(std::size_t k = 0; k < (std::size_t)DemographicProfile::Employment::Last; k++)
            {
                currTimeStepNumInfectedEntityTypeRiskEmployment[entity_type][i][k] = 0;
                currTimeStepAgeInfectionSumEntityTypeRiskEmployment[entity_type][i][k] = 0;
                currTimeStepAgeInfectionSumSqEntityTypeRiskEmployment[entity_type][i][k] = 0;
            }
        }
    }
}

std::size_t InfectionsTracker::getCurrTimeStepIncidentInfsTotal()
{
	std::size_t infections = 0;

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

std::size_t InfectionsTracker::getNumIncidentInfections()
{
	std::size_t infections = 0;

	for(std::size_t i = 0; i < (std::size_t)Entity::HVLStrata::Last; i++)
	{
		infections += totalIncidentInfections[i];
	}

	return infections;
}

//returns the prevalence rate among sexually active pop
double InfectionsTracker::getSAPrev(Population &_population)
{
	std::size_t totalInfected = 0;
    std::size_t currPopSize = _population.GetSize();
    std::size_t currSAPopSize = currPopSize - _population.GetNASize();

	//Currently Infected
	//total the current infections
	for(unsigned int i = 0; i < DemographicProfile::TotalNumBuckets; i++)
	{
		for(int j = 0; j < NUMBER_GENERATIONS_TO_TRACE; j++)
		{
			totalInfected += currPrevalentInfections[j][i];
		}
	}

	//current pop and prevalence of sexually active population
	//Need total number of infected for SA population only
	auto totalInfectedSA = totalInfected;
	DemographicProfile NAProfile;
    NAProfile.set(DemographicProfile::Demographic::SexualActivityStatus, (std::size_t)DemographicProfile::SexualActivityStatus::NotActive);
	std::vector<DemographicProfile::ProfileID> NAProfileIDs;
	NAProfile.selectProfileIDs(NAProfileIDs, nullptr);

    for(std::size_t i = 0; i < NAProfileIDs.size(); i++)
	{
        for(std::size_t j = 0; j < NUMBER_GENERATIONS_TO_TRACE; j++)
		{
			totalInfectedSA -= currPrevalentInfections[j][NAProfileIDs[i]];
		}
	}

	if(currSAPopSize == 0)
	{
		return -1;
	}
	else
	{
        return totalInfectedSA / static_cast<double>(currSAPopSize);
	}
}

/*
* Resests counting of incident infections for time step
*/
void InfectionsTracker::resetIncidentInfections(Time time)
{
    /** Reset counter for incident infections and exposures for current timestep*/
    for(std::size_t i = 0; i < (std::size_t)Entity::HVLStrata::Last; i++)
    {
        currTimeStepIncidentInfs[i] = 0;
        currTimeExposures[i] = 0;
    }

    currTimeStepNumInfected = 0;
    currTimeStepCD4InfectionSum = 0;
    currTimeStepCD4InfectionSumSq = 0;

    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
        for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
        {
            for(std::size_t k = 0; k < (std::size_t)DemographicProfile::Employment::Last; k++)
            {
                currTimeStepIncidentInfsEntityTypeRiskEmployment[entity_type][i][k] = 0;
            }
        }

        currTimeStepAgeInfectionSumEntityType[entity_type] = 0;
        currTimeStepAgeInfectionSumSqEntityType[entity_type] = 0;
        currTimeStepNumInfectedEntityType[entity_type] = 0;

        for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
        {
            for(std::size_t k = 0; k < (std::size_t)DemographicProfile::Employment::Last; k++)
            {
                currTimeStepAgeInfectionSumEntityTypeRiskEmployment[entity_type][i][k] = 0;
                currTimeStepAgeInfectionSumSqEntityTypeRiskEmployment[entity_type][i][k] = 0;
                currTimeStepNumInfectedEntityTypeRiskEmployment[entity_type][i][k] = 0;
            }
        }

        for(auto &ageRangeSize : currTimeStepIncidentInfsEntityTypeAge[entity_type])
        {
            ageRangeSize.second = 0;
        }
    }

	currTimeStep = time;
}


/*
initializes the counters for incident infections by age and gender
*/
void InfectionsTracker::initializeIncidentInfectionsByAge(
    const std::unordered_map<std::string, AgeRangeSizeContainer> &incident_by_entity_type_age,
    const AgeRangeSizeContainer &incident_by_age)
{
    currTimeStepIncidentInfsEntityTypeAge = incident_by_entity_type_age;
    totalIncidentInfsAge = incident_by_age;
}

// Records a new exposure regardless of whether an infection happened or not
void InfectionsTracker::recordExposure(Time time, const Entity *_infector)
{
	// Reset counter for incident infections and exposures for current timestep 
    // if this is the first time an InfectionsTracker function has been called.
	if(time > currTimeStep)
	{
		resetIncidentInfections(time);
	}

    currTimeExposures[(std::size_t)_infector->getHVL()]++;
    totalExposures[(std::size_t)_infector->getHVL()]++;
}

//records a New infection and also prints the infection out to a trace
void InfectionsTracker::recordIncidentInfection(Time time,
    SexualPartnership::Type _partnershipType, const Entity *_infector,
    const Entity *_infected)
{
    assert(time.in_months() >= 0);
    //check to see if the people are valid: not null, not dead, in a valid bucket
    assert((_infector != nullptr) && (_infector->isAlive())
        && (Utility::within_range(_infector->getDemographicProfile()->getProfileID(), DemographicProfile::MIN, DemographicProfile::MAX)));
    assert((_infected != nullptr) && (_infected->isAlive())
        && (Utility::within_range(_infected->getDemographicProfile()->getProfileID(), DemographicProfile::MIN, DemographicProfile::MAX)));
    assert(_partnershipType < SexualPartnership::Type::ENDType);

    //reset counter for incident infections and exposures for current timestep
    if(time > currTimeStep)
    {
        resetIncidentInfections(time);
    }

    //record infection
    incidentInfectionsByDemographic[(std::size_t)_partnershipType][_infector->getDemographicProfile()->getProfileID()][_infected->getDemographicProfile()->getProfileID()]++;
    auto type_to_index = [](const std::string &entity_type)
    {
        if(entity_type == "male") return 0;
        if(entity_type == "msmw") return 1;
        if(entity_type == "msm") return 2;
        if(entity_type == "female") return 3;
        throw std::runtime_error("invalid");
    };
    incidentInfectionsByEntityType[(std::size_t)_partnershipType][type_to_index(_infector->getEntityType())][type_to_index(_infected->getEntityType())]++;

	//The person doing the infecting should be infected!
    assert(_infector->getHVL() != Entity::HVLStrata::UNINFECTED);
	currTimeStepIncidentInfs[(std::size_t)_infector->getHVL()]++;
    currTimeStepIncidentInfsEntityTypeRiskEmployment[_infected->getEntityType()][(std::size_t)_infected->getRiskLevel()][_infected->getDemographicProfileVal(DemographicProfile::Demographic::Employment)]++;
    totalIncidentInfections[(std::size_t)_infector->getHVL()]++;
	auto infectedAge = _infected->getAge();
	double infectorCD4 = _infector->cd4;
	Entity::RiskLevel infectedRisk = _infected->getRiskLevel();

	auto square = [](double d) { return d * d; };

	currTimeStepNumInfected++;
	currTimeStepCD4InfectionSum += infectorCD4;
	currTimeStepCD4InfectionSumSq += infectorCD4 * infectorCD4;
    currTimeStepNumInfectedEntityType[_infected->getEntityType()]++;
	currTimeStepAgeInfectionSumEntityType[_infected->getEntityType()] += (std::size_t)infectedAge.in_months();
	currTimeStepAgeInfectionSumSqEntityType[_infected->getEntityType()] += (std::size_t)square(infectedAge.in_months());
    currTimeStepNumInfectedEntityTypeRiskEmployment[_infected->getEntityType()][(std::size_t)infectedRisk][_infected->getDemographicProfileVal(DemographicProfile::Demographic::Employment)]++;
	currTimeStepAgeInfectionSumEntityTypeRiskEmployment[_infected->getEntityType()][(std::size_t)infectedRisk][_infected->getDemographicProfileVal(DemographicProfile::Demographic::Employment)] += (std::size_t)infectedAge.in_months();
	currTimeStepAgeInfectionSumSqEntityTypeRiskEmployment[_infected->getEntityType()][(std::size_t)infectedRisk][_infected->getDemographicProfileVal(DemographicProfile::Demographic::Employment)] += (std::size_t)square(infectedAge.in_months());

    if(_infected->getDemographicProfileVal(DemographicProfile::Demographic::Employment) == (std::size_t)DemographicProfile::Employment::Csw)
	{
        totalIncidentInfsRiskCSW[(std::size_t)infectedRisk]++;
	}

    totalIncidentInfsRisk[(std::size_t)_infected->getRiskLevel()]++;
    totalIncidentInfsEntityTypeRiskEmployment[_infected->getEntityType()][(std::size_t)_infected->getRiskLevel()][_infected->getDemographicProfileVal(DemographicProfile::Demographic::Employment)]++;

    auto &genderIncidentInfectionsByAge = currTimeStepIncidentInfsEntityTypeAge[_infected->getEntityType()];
    totalIncidentInfsEntityType[_infected->getEntityType()]++;
	
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
		_outStream << currTimeStepNumInfected << Constants::Tab << cd4Mean << Constants::Tab << cd4SD << Constants::Tab;
	}
	else
	{
		_outStream << currTimeStepNumInfected << Constants::Tab << "N/A" << Constants::Tab << "N/A" << Constants::Tab;
	}
}

void InfectionsTracker::setPrevalentInfections(
    std::array<DemographicArray, NUMBER_GENERATIONS_TO_TRACE> _prevalenceByBucket,
    const EntityTypeMap<AgeRangeSizeContainer> &_prevalenceByEntityTypeAge,
    EntityTypeMap<RiskEmploymentArray> _prevalenceByEntityTypeRiskEmployment)
{
	DemographicProfile::ProfileID currProfileID = DemographicProfile::MIN;

	while(currProfileID <= DemographicProfile::MAX)
	{
		for(int i = 0; i < NUMBER_GENERATIONS_TO_TRACE; i++)
		{
			currPrevalentInfections[i][currProfileID] = _prevalenceByBucket[i][currProfileID];
		}

		currProfileID++;
	}

    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
        currPrevalentInfectionsEntityType[entity_type] = 0;

        for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
        {
            for(std::size_t k = 0; k < (std::size_t)DemographicProfile::Employment::Last; k++)
            {
                currPrevalentInfectionsEntityTypeRiskEmployment[entity_type][i][k] = _prevalenceByEntityTypeRiskEmployment[entity_type][i][k];
                currPrevalentInfectionsEntityType[entity_type] += _prevalenceByEntityTypeRiskEmployment[entity_type][i][k];
            }
        }

        currPrevalentInfectionsEntityTypeAge[entity_type] = _prevalenceByEntityTypeAge.at(entity_type);
    }
}

int InfectionsTracker::printInfections(EventParams &/*_eventParams*/, Time time, std::ostream &_outStream, Population *_population)
{
	assert(time.in_months() >= 0);
	//total # infections this month
	std::size_t totalInfected = 0;
	//current population size
    std::size_t currPopSize = _population->GetSize();
    std::size_t currSAPopSize = currPopSize - _population->GetNASize();
	//total # of age ranges to print out
    auto currSizeByAgeRange = _population->GetAgeRanges();
	auto numAgeRanges = currSizeByAgeRange.size();

	//write headers for infections sheet
	if(time.in_months() == 0)
	{
		std::ostringstream firstRow;
        std::ostringstream secondRow;
        std::ostringstream thirdRow;
		//incidence-related headers
		firstRow << "Epidemiology Outputs" << Constants::Tab;
		secondRow << Constants::Tab;
		thirdRow << "Month" << Constants::Tab;
		firstRow << Constants::Tab;
		secondRow << Constants::Tab;
		thirdRow << "New Infections" << Constants::Tab;
		firstRow << Constants::Tab;
		secondRow << Constants::Tab;
		thirdRow << "Total Infected in History (Prevalent Cases Excluded)" << Constants::Tab;
		firstRow << Constants::Tab;
		secondRow << Constants::Tab;
		thirdRow << "Currently Infected" << Constants::Tab;
		firstRow << Constants::Tab;
		secondRow << Constants::Tab;
		thirdRow << "Pop Size" << Constants::Tab;
		firstRow << Constants::Tab;
		secondRow << Constants::Tab;
		thirdRow << "Prevalence" << Constants::Tab;
		firstRow << Constants::Tab;
		secondRow << Constants::Tab;
		thirdRow << "SA Pop Size" << Constants::Tab;
		firstRow << Constants::Tab;
		secondRow << Constants::Tab;
		thirdRow << "SA Prevalence" << Constants::Tab;
		//write out headers for population by age\sexual activity
		firstRow << "Prevalent Cases" << Constants::Tab;
		secondRow << "Non-Sexually Active Population" << Constants::Tab;
		thirdRow << "All ages" << Constants::Tab;

		for(std::size_t i = 0; i < numAgeRanges; i++)
		{
			if(i == 0)
			{
				secondRow << "Sexually Active Population";
			}

			firstRow << Constants::Tab;
			secondRow << Constants::Tab;
			thirdRow << currSizeByAgeRange.at(i) << Constants::Tab;
		}

		//write out headers for population by gender
        firstRow << "Prevalent Cases";
        secondRow << "Entity Type";

        for(auto entity_type : {"Male:Hetero", "Male:Msmw", "Male:Msm", "Female"})
        {
            firstRow << Constants::Tab;
            secondRow << Constants::Tab;
            thirdRow << entity_type << Constants::Tab;
        }

		//write out headers for population by gender and age
        for(auto entity_type : {"Male:Hetero", "Male:Msmw", "Male:Msm", "Female"})
        {
            firstRow << entity_type << Constants::Tab;
            secondRow << "Non-Sexually Active Population" << Constants::Tab;
            thirdRow << "All Ages" << Constants::Tab;

            for(std::size_t i = 0; i < numAgeRanges; i++)
            {
                if(i == 0)
                {
                    secondRow << "Sexually Active Population";
                }

                firstRow << Constants::Tab;
                secondRow << Constants::Tab;
                thirdRow << currPrevalentInfectionsEntityTypeAge["female"].at(i).first << Constants::Tab;
            }
        }

		//write out headers for number infected by risk
        firstRow << "Prevalent Cases" << Constants::Tab << Constants::Tab;
        secondRow << "Risk Group" << Constants::Tab << Constants::Tab;
        thirdRow << "CSW High Risk" << Constants::Tab;
        thirdRow << "CSW Low Risk" << Constants::Tab;

        for(auto entity_type : {"Male:Hetero", "Male:Msmw", "Male:Msm", "Female"})
        {
            firstRow << Constants::Tab;
            secondRow << Constants::Tab;
            thirdRow << "Non-CSW High Risk " << entity_type << Constants::Tab;
        }
		         
        for(auto entity_type : {"Male:Hetero", "Male:Msmw", "Male:Msm", "Female"})
        {
            firstRow << Constants::Tab;
            secondRow << Constants::Tab;
            thirdRow << "Non-CSW Low Risk " << entity_type << Constants::Tab;
        }

		//write out headers for number of infections by generation (7 tabs)
		firstRow << "Prevalent Cases" << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab
		         << Constants::Tab;
		secondRow << "Number Infected by Generation" << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
		          Constants::Tab << Constants::Tab;
		thirdRow << "0 (Prev.)" << Constants::Tab << "First" << Constants::Tab << "Second" << Constants::Tab << "Third" <<
		         Constants::Tab << "Fourth" << Constants::Tab << "Fifth+" << Constants::Tab;
		//write out headers for number of exposures and infections by HVL
		firstRow << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
		         Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
		secondRow << Constants::Tab << "Exposures by HVL" << Constants::Tab << Constants::Tab << Constants::Tab <<
		          Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
		thirdRow << Constants::Tab << "HVL 0-20" << Constants::Tab << "HVL 21-500" << Constants::Tab << "HVL 501-3000" <<
		         Constants::Tab << "HVL 3001-10000" << Constants::Tab << "HVL 10000-30000" << Constants::Tab << "HVL 30001-100000" <<
		         Constants::Tab << "HVL 100000+" << Constants::Tab << "HVL Primary" << Constants::Tab << "HVL Late Stage" <<
		         Constants::Tab;
		firstRow << Constants::Tab << "Incident Cases" << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab
		         << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
		secondRow << Constants::Tab << "Infections by HVL (of Infector)" << Constants::Tab << Constants::Tab << Constants::Tab
		          << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
		thirdRow << Constants::Tab << "HVL 0-20" << Constants::Tab << "HVL 21-500" << Constants::Tab << "HVL 501-3000" <<
		         Constants::Tab << "HVL 3001-10000" << Constants::Tab << "HVL 10000-30000" << Constants::Tab << "HVL 30001-100000" <<
		         Constants::Tab << "HVL 100000+" << Constants::Tab << "HVL Primary" << Constants::Tab << "HVL Late Stage" <<
		         Constants::Tab;

		//write out headers for number of infections by Age and gender
		for(std::size_t i = 0; i < numAgeRanges; i++)
		{
			if(i == 0)
			{
				firstRow << "Incident Cases";
				secondRow << "Infections By Age";
			}

			firstRow << Constants::Tab;
			secondRow << Constants::Tab;
			thirdRow << currSizeByAgeRange.at(i) << Constants::Tab;
		}

        firstRow << "Incident Cases" << Constants::Tab;
        secondRow << "Entity Type";

        for(auto entity_type : {"Male:Hetero", "Male:Msmw", "Male:Msm", "Female"})
        {
            firstRow << Constants::Tab;
            secondRow << Constants::Tab;
            thirdRow << entity_type << Constants::Tab;
        }

        secondRow << "Risk Group" << Constants::Tab;
        thirdRow << "CSW High Risk" << Constants::Tab;
        thirdRow << "CSW Low Risk" << Constants::Tab;

        for(auto entity_type : {"Male:Hetero", "Male:Msmw", "Male:Msm", "Female"})
        {
            firstRow << Constants::Tab;
            secondRow << Constants::Tab;
            thirdRow << "Non-CSW High Risk " << entity_type << Constants::Tab;
        }

        for(auto entity_type : {"Male:Hetero", "Male:Msmw", "Male:Msm", "Female"})
        {
            firstRow << Constants::Tab;
            secondRow << Constants::Tab;
            thirdRow << "Non-CSW Low Risk " << entity_type << Constants::Tab;
        }

        firstRow << Constants::Tab;
        secondRow << Constants::Tab;

        for(auto entity_type : {"Male:Hetero", "Male:Msmw", "Male:Msm", "Female"})
        {
	  for(std::size_t i = 0; i < numAgeRanges; i++)
            {
                if(i == 0)
                {
                    firstRow << "Incident Cases";
                    secondRow << entity_type << " (By Age)";
                }

                firstRow << Constants::Tab;
                secondRow << Constants::Tab;
                thirdRow << currSizeByAgeRange.at(i) << Constants::Tab;
            }
        }

		firstRow << Constants::Tab;
		secondRow << Constants::Tab;
		thirdRow << Constants::Tab;

		//write out headers for incident infections by relationship type
        firstRow << "Total Infected in History (Prevalent Cases Excluded)";
        secondRow << "Relationship Type";
        for(auto header : {"Steady:Hetero", "Regular:Hetero", "Casual:Hetero", "CSW:Hetero", "Steady:BiSex/Fem", "Regular:BiSex/Fem", "Casual:BiSeX/Fem", "CSW:BiSex/Fem", "Steady:BiSex/Male", "Regular:BiSex/Male", "Casual:BiSex/Male", "CSW:BiSex/Male", "Steady:MSM", "Regular:MSM", "Casual:MSM", "CSW:MSM"})
		{
			firstRow << Constants::Tab;
			secondRow << Constants::Tab;
			thirdRow << header << Constants::Tab;
		}

		firstRow << Constants::Tab;
		secondRow << "Infectors:" << Constants::Tab;
		thirdRow << "Infecteds:" << Constants::Tab;
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
				firstRow << Constants::Tab;
				secondRow << Constants::Tab;
				//second row profile str refers to infecteds
				thirdRow << *DemographicProfile::toString(*infectedProfileID) << Constants::Tab;
				infectedProfileID++;
			}

			infectorProfileID++;
		}

		//write out headers that tally total infections based on age and gender and risk group
		for(std::size_t i = 0; i < numAgeRanges; i++)
		{
			if(i == 0)
			{
				firstRow << "Total Infected in History (Prevalent Cases Excluded)";
				secondRow << "Infections By Age";
			}

			firstRow << Constants::Tab;
			secondRow << Constants::Tab;
			thirdRow << currSizeByAgeRange.at(i) << Constants::Tab;
		}

        firstRow << "Total Infected in History (Prevalent Cases Excluded)";
        secondRow << "Entity Type";

        for(auto entity_type : {"Male:Hetero", "Male:Msmw", "Male:Msm", "Female"})
        {
            firstRow << Constants::Tab;
            secondRow << Constants::Tab;
            thirdRow << entity_type << Constants::Tab;
        }

        firstRow << "Total Infected in History (Prevalent Cases Excluded)" << Constants::Tab;
        secondRow << "Risk Group" << Constants::Tab;

        firstRow << Constants::Tab;
        secondRow << Constants::Tab;

        thirdRow << "CSW High Risk" << Constants::Tab;
        thirdRow << "CSW Low Risk" << Constants::Tab;

        for(auto entity_type : {"Male:Hetero", "Male:Msmw", "Male:Msm", "Female"})
        {
            firstRow << Constants::Tab;
            secondRow << Constants::Tab;
            thirdRow << "Non-CSW High Risk " << entity_type << Constants::Tab;
        }

        for(auto entity_type : {"Male:Hetero", "Male:Msmw", "Male:Msm", "Female"})
        {
            firstRow << Constants::Tab;
            secondRow << Constants::Tab;
            thirdRow << "Non-CSW Low Risk " << entity_type << Constants::Tab;
        }
		firstRow << Constants::Tab;
		secondRow << Constants::Tab;
		thirdRow << Constants::Tab;
        firstRow << "Age at Infection" << Constants::Tab;
        for(auto entity_type : {"Male:Hetero", "Male:Msmw", "Male:Msm", "Female"})
        {
            firstRow << Constants::Tab << Constants::Tab;
            thirdRow << "Mean" << Constants::Tab << "SD" << Constants::Tab;
            secondRow << entity_type << Constants::Tab << Constants::Tab;
        }
       
		secondRow << "CSW" << Constants::Tab << Constants::Tab << "CSW High Risk" << Constants::Tab << Constants::Tab <<
		          "CSW Low Risk"  << Constants::Tab << Constants::Tab;
		thirdRow << "Mean" << Constants::Tab << "SD" << Constants::Tab << "Mean" << Constants::Tab << "SD" << Constants::Tab <<
		         "Mean" << Constants::Tab << "SD" << Constants::Tab;
		firstRow << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
		         Constants::Tab << Constants::Tab;

        for(auto entity_type : {"Male:Hetero", "Male:Msmw", "Male:Msm", "Female"})
        {
            firstRow << Constants::Tab << Constants::Tab;
            secondRow << "Non-CSW High Risk " << entity_type << Constants::Tab << Constants::Tab;
            thirdRow << "Mean" << Constants::Tab << "SD" << Constants::Tab;
        }

        for(auto entity_type : {"Male:Hetero", "Male:Msmw", "Male:Msm", "Female"})
        {
            firstRow << Constants::Tab << Constants::Tab;
            secondRow << "Non-CSW Low Risk " << entity_type << Constants::Tab << Constants::Tab;
            thirdRow << "Mean" << Constants::Tab << "SD" << Constants::Tab;
        }

		//write out string buffers to trace file
		_outStream << firstRow.str() << std::endl;
		_outStream << secondRow.str() << std::endl;
		_outStream << thirdRow.str() << std::endl;
	}

	//if no incident infections happened during this time, then make sure that we have 0 in the currTime incident infections and exposures
	if(time > currTimeStep)
	{
		resetIncidentInfections(time);
	}

	//get current total infections
	//Month
	if(time.in_months() == 0)
	{
		_outStream << "init" << Constants::Tab;
	}
	else
	{
		_outStream << time.in_months() << Constants::Tab;
	}

	//New Infections
	_outStream << getCurrTimeStepIncidentInfsTotal() << Constants::Tab;

	double monthlyIncidence = 1.0 * (getCurrTimeStepIncidentInfsTotal()) / (double)currPopSize;
	//Push the monthly incidence onto the deque
	lastTwelveIncidenceRates.push_back(monthlyIncidence);
	//Pop off the oldest incidence rate
	lastTwelveIncidenceRates.pop_front();
	assert(lastTwelveIncidenceRates.size() == 12);

	//Total Infected in History
	_outStream << getNumIncidentInfections() << Constants::Tab;

	//Currently Infected
	//total the current infections
	for(unsigned int i = 0; i < DemographicProfile::TotalNumBuckets; i++)
	{
		for(int j = 0; j < NUMBER_GENERATIONS_TO_TRACE; j++)
		{
			totalInfected += currPrevalentInfections[j][i];
		}
	}

	_outStream << totalInfected << Constants::Tab;

	//Total Population Size
	_outStream << currPopSize << Constants::Tab;

	//current prevalence
	double currPrevalence = totalInfected / (double)currPopSize;
	_outStream << currPrevalence << Constants::Tab;

	//current pop and prevalence of sexually active population
	//Need total number of infected for SA population only
	std::size_t totalInfectedSA = totalInfected;
	DemographicProfile NAProfile;
    NAProfile.set(DemographicProfile::Demographic::SexualActivityStatus, (std::size_t)DemographicProfile::SexualActivityStatus::NotActive);
	std::vector<DemographicProfile::ProfileID> NAProfileIDs;
	NAProfile.selectProfileIDs(NAProfileIDs, nullptr);

	for(size_t i = 0; i < NAProfileIDs.size(); i++)
	{
		for(size_t j = 0; j < NUMBER_GENERATIONS_TO_TRACE; j++)
		{
			totalInfectedSA -= currPrevalentInfections[j][NAProfileIDs[i]];
		}
	}

	double currPrevalenceSA = (totalInfectedSA) / (double)currSAPopSize;
	_outStream << currSAPopSize << Constants::Tab;
	_outStream << currPrevalenceSA << Constants::Tab;

	//Multiply by 100 and round to nearest integer for graphical output
	int intPrevalence = (int)(100 * currPrevalence + 0.5);
	//prev cases by age\sexual activity
	auto totalInfectedNA = totalInfected - totalInfectedSA;
	_outStream << totalInfectedNA << Constants::Tab;

	for(std::size_t i = 0; i < numAgeRanges; i++)
	{
        std::size_t sum_entity_type_age = 0;

        for(auto entity_type : {"male", "msmw", "msm", "female"})
        {
            sum_entity_type_age += currPrevalentInfectionsEntityTypeAge[entity_type].at(i).second;
        }

        _outStream << sum_entity_type_age << Constants::Tab;
	}

	//Print out population size and number infected by gender
    std::unordered_map<std::string, std::size_t> totalInfectedEntityType;
    std::unordered_map<std::string, std::size_t> totalInfectedSAEntityType;
	DemographicProfile GenderProfile;

	for(std::size_t i = 0; i < numAgeRanges; i++)
	{
        for(auto entity_type : {"male", "msmw", "msm", "female"})
        {
            totalInfectedSAEntityType[entity_type] += currPrevalentInfectionsEntityTypeAge[entity_type].at(i).second;
        }
	}

	//Actually print the size and infections by gender
    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
        _outStream << totalInfectedEntityType[entity_type] << Constants::Tab;
    }

	//output infections by age and gender
    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
        _outStream << totalInfectedEntityType[entity_type] - totalInfectedSAEntityType[entity_type] << Constants::Tab;

        for(std::size_t i = 0; i < numAgeRanges; i++)
        {
            _outStream << currPrevalentInfectionsEntityTypeAge[entity_type].at(i).second << Constants::Tab;
        }
    }

	//output prevalent infections by risk
	std::size_t totalInfectedCSW = 0;
	DemographicProfile CSWProfile;
    CSWProfile.set(DemographicProfile::Demographic::Employment, (std::size_t)DemographicProfile::Employment::Csw);
	std::vector<DemographicProfile::ProfileID> CSWProfileIDs;
	CSWProfile.selectProfileIDs(CSWProfileIDs, nullptr);

	for(std::size_t i = 0; i < CSWProfileIDs.size(); i++)
	{
		for(std::size_t j = 0; j < NUMBER_GENERATIONS_TO_TRACE; j++)
		{
			totalInfectedCSW += currPrevalentInfections[j][CSWProfileIDs[i]];
		}
	}

    _outStream
        << currPrevalentInfectionsEntityTypeRiskEmployment["male"][(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::Csw]
        + currPrevalentInfectionsEntityTypeRiskEmployment["msmw"][(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::Csw]
        + currPrevalentInfectionsEntityTypeRiskEmployment["msm"][(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::Csw]
        + currPrevalentInfectionsEntityTypeRiskEmployment["female"][(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::Csw]
        << Constants::Tab;
    _outStream
        << currPrevalentInfectionsEntityTypeRiskEmployment["male"][(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::Csw]
        + currPrevalentInfectionsEntityTypeRiskEmployment["msmw"][(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::Csw]
        + currPrevalentInfectionsEntityTypeRiskEmployment["msm"][(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::Csw]
        + currPrevalentInfectionsEntityTypeRiskEmployment["female"][(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::Csw]
        << Constants::Tab;

    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
        _outStream
            << currPrevalentInfectionsEntityTypeRiskEmployment[entity_type][(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::NonCsw]
            << Constants::Tab;
    }

    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
        _outStream
            << currPrevalentInfectionsEntityTypeRiskEmployment[entity_type][(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::NonCsw]
            << Constants::Tab;
    }

	//We're hard wiring 6 (Prev + 5) generations of reporting for now
	std::size_t totalInfectedByGeneration[NUMBER_GENERATIONS_TO_TRACE];

	for(int i = 0; i < NUMBER_GENERATIONS_TO_TRACE; i++)
	{
		totalInfectedByGeneration[i] = 0;

		for(std::size_t j = 0; j < DemographicProfile::TotalNumBuckets; j++)
		{
			totalInfectedByGeneration[i] += currPrevalentInfections[i][j];
		}
	}

	//write out number of infections by generation (7 tabs)
	for(int i = 0; i < NUMBER_GENERATIONS_TO_TRACE; i++)
	{
		_outStream << totalInfectedByGeneration[i] << Constants::Tab;
	}

	/** Print out exposures and infections by viral load */
	_outStream << Constants::Tab;

	for(std::size_t i = 1; i < (std::size_t)Entity::HVLStrata::Last; i++)
	{
		_outStream << currTimeExposures[i] << Constants::Tab;
	}

	_outStream << Constants::Tab;

	for(std::size_t i = 1; i < (std::size_t)Entity::HVLStrata::Last; i++)
	{
		_outStream << currTimeStepIncidentInfs[i] << Constants::Tab;
	}

	//Print out incident infections by age and gender and risk
	std::unordered_map<std::string, std::size_t> incidentInfsEntityType;
    //std::size_t sumIncidentInfs = 0;

    for(std::size_t i = 0; i < numAgeRanges; i++)
	{
        std::size_t sumIncidentAge = 0;

        for(auto entity_type : {"male", "msmw", "msm", "female"})
        {
            auto incident_entity_type_age = currTimeStepIncidentInfsEntityTypeAge[entity_type].at(i).second;
            incidentInfsEntityType[entity_type] += incident_entity_type_age;
            //sumIncidentInfs += incident_entity_type_age;
            sumIncidentAge += incident_entity_type_age;
        }

        _outStream << sumIncidentAge << Constants::Tab;
	}

    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
        _outStream << incidentInfsEntityType[entity_type] << Constants::Tab;
    }

    _outStream
        << currTimeStepIncidentInfsEntityTypeRiskEmployment["male"][(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::Csw]
        + currTimeStepIncidentInfsEntityTypeRiskEmployment["msmw"][(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::Csw]
        + currTimeStepIncidentInfsEntityTypeRiskEmployment["msm"][(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::Csw]
        + currTimeStepIncidentInfsEntityTypeRiskEmployment["female"][(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::Csw]
        << Constants::Tab;

    _outStream
        << currTimeStepIncidentInfsEntityTypeRiskEmployment["male"][(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::Csw]
        + currTimeStepIncidentInfsEntityTypeRiskEmployment["msmw"][(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::Csw]
        + currTimeStepIncidentInfsEntityTypeRiskEmployment["msm"][(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::Csw]
        + currTimeStepIncidentInfsEntityTypeRiskEmployment["female"][(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::Csw]
        << Constants::Tab;

    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
        _outStream
            << currTimeStepIncidentInfsEntityTypeRiskEmployment[entity_type][(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::NonCsw]
            << Constants::Tab;
    }

    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
        _outStream
            << currTimeStepIncidentInfsEntityTypeRiskEmployment[entity_type][(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::NonCsw]
            << Constants::Tab;
    }

    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
        for(std::size_t i = 0; i < numAgeRanges; i++)
        {
            _outStream << currTimeStepIncidentInfsEntityTypeAge[entity_type].at(i).second << Constants::Tab;
        }
    }

	// Print out incident infections by relationship type
	_outStream << Constants::Tab;

    //steady,reg,cas,csw:male<->fem
    _outStream << incidentInfectionsByEntityType[0][0][3] + incidentInfectionsByEntityType[0][3][0] << Constants::Tab;
    _outStream << incidentInfectionsByEntityType[1][0][3] + incidentInfectionsByEntityType[1][3][0] << Constants::Tab;
    _outStream << incidentInfectionsByEntityType[2][0][3] + incidentInfectionsByEntityType[2][3][0] << Constants::Tab;
    _outStream << incidentInfectionsByEntityType[3][0][3] + incidentInfectionsByEntityType[3][3][0] << Constants::Tab;
    //steady,reg,cas,csw:msmw<->fem
    _outStream << incidentInfectionsByEntityType[0][1][3] + incidentInfectionsByEntityType[0][3][1] << Constants::Tab;
    _outStream << incidentInfectionsByEntityType[1][1][3] + incidentInfectionsByEntityType[1][3][1] << Constants::Tab;
    _outStream << incidentInfectionsByEntityType[2][1][3] + incidentInfectionsByEntityType[2][3][1] << Constants::Tab;
    _outStream << incidentInfectionsByEntityType[3][1][3] + incidentInfectionsByEntityType[3][3][1] << Constants::Tab;
    //steadymsm,regmsm,casmsm,cswmsm:msmw<->msm
    _outStream << incidentInfectionsByEntityType[0][1][2] + incidentInfectionsByEntityType[0][2][1] << Constants::Tab;
    _outStream << incidentInfectionsByEntityType[1][1][2] + incidentInfectionsByEntityType[1][2][1] << Constants::Tab;
    _outStream << incidentInfectionsByEntityType[2][1][2] + incidentInfectionsByEntityType[2][2][1] << Constants::Tab;
    _outStream << incidentInfectionsByEntityType[3][1][2] + incidentInfectionsByEntityType[3][2][1] << Constants::Tab;
    //steadymsm,regmsm,casmsm,cswmsm:msm<->msm
    _outStream << incidentInfectionsByEntityType[0][2][2] + incidentInfectionsByEntityType[0][2][2] << Constants::Tab;
    _outStream << incidentInfectionsByEntityType[1][2][2] + incidentInfectionsByEntityType[1][2][2] << Constants::Tab;
    _outStream << incidentInfectionsByEntityType[2][2][2] + incidentInfectionsByEntityType[2][2][2] << Constants::Tab;
    _outStream << incidentInfectionsByEntityType[3][2][2] + incidentInfectionsByEntityType[3][2][2] << Constants::Tab;

	_outStream << Constants::Tab;
	//write out all incident infections that happened in history
	// only ProfileID's in profileIDsForDetailedTrace are included
	auto infectorProfileID = profileIDsForDetailedTrace.begin();

	while(infectorProfileID != profileIDsForDetailedTrace.end())
	{
		auto infectedProfileID = profileIDsForDetailedTrace.begin();

		while(infectedProfileID != profileIDsForDetailedTrace.end())
		{
			//tally all the infections that happened from curr infectorProfileID -> curr infectedProfileID
            std::size_t infs = 0;

			for(int partnershipType = 0; partnershipType < (int)SexualPartnership::Type::ENDType; ++partnershipType)
			{
				infs += incidentInfectionsByDemographic[partnershipType][*infectorProfileID][*infectedProfileID];
			}

			_outStream << infs << Constants::Tab;
			infectedProfileID++;
		}

		infectorProfileID++;
	}

	//write out all incident infections that happened in history stratified by age and gender and risk
    for(std::size_t i = 0; i < numAgeRanges; i++)
	{
		_outStream << totalIncidentInfsAge.at(i).second << Constants::Tab;
	}

    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
        _outStream << totalIncidentInfsEntityType[entity_type] << Constants::Tab;
    }

    _outStream
        << totalIncidentInfsEntityTypeRiskEmployment["male"][(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::Csw]
        + totalIncidentInfsEntityTypeRiskEmployment["msmw"][(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::Csw]
        + totalIncidentInfsEntityTypeRiskEmployment["msm"][(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::Csw]
        + totalIncidentInfsEntityTypeRiskEmployment["female"][(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::Csw]
        << Constants::Tab;

    _outStream
        << totalIncidentInfsEntityTypeRiskEmployment["male"][(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::Csw]
        + totalIncidentInfsEntityTypeRiskEmployment["msmw"][(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::Csw]
        + totalIncidentInfsEntityTypeRiskEmployment["msm"][(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::Csw]
        + totalIncidentInfsEntityTypeRiskEmployment["female"][(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::Csw]
        << Constants::Tab;

    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
        _outStream
            << totalIncidentInfsEntityTypeRiskEmployment[entity_type][(std::size_t)Entity::RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::NonCsw]
            << Constants::Tab;
    }

    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
        _outStream
            << totalIncidentInfsEntityTypeRiskEmployment[entity_type][(std::size_t)Entity::RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::NonCsw]
            << Constants::Tab;
    }

	//write out age of infection for incident infections that month (mean and SD)
	_outStream << Constants::Tab;

    for(auto entity_type : {"male", "msmw", "msm", "female"})
    {
		if(currTimeStepNumInfectedEntityType[entity_type] != 0)
		{
			double ageMean = currTimeStepAgeInfectionSumEntityType[entity_type] / (double) currTimeStepNumInfectedEntityType[entity_type];
			double ageSD = sqrt(currTimeStepAgeInfectionSumSqEntityType[entity_type] / (double) currTimeStepNumInfectedEntityType[entity_type] -
			                    ageMean * ageMean);
			_outStream << ageMean << Constants::Tab << ageSD << Constants::Tab;
		}
		else
		{
			_outStream << "N/A" << Constants::Tab << "N/A" << Constants::Tab;
		}
	}

	double numInfectedCSW = 0;

	for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
	{
        for(auto entity_type : {"male", "msmw", "msm", "female"})
        {
            numInfectedCSW += currTimeStepNumInfectedEntityTypeRiskEmployment[entity_type][i][(std::size_t)DemographicProfile::Employment::Csw];
		}
	}

	if(numInfectedCSW != 0)
	{
		double ageSumCSW  = 0;
		double ageSumSqCSW = 0;

		for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
		{
            for(auto entity_type : {"male", "msmw", "msm", "female"})
            {
                ageSumCSW += currTimeStepAgeInfectionSumEntityTypeRiskEmployment[entity_type][i][(std::size_t)DemographicProfile::Employment::Csw];
                ageSumSqCSW += currTimeStepAgeInfectionSumSqEntityTypeRiskEmployment[entity_type][i][(std::size_t)DemographicProfile::Employment::Csw];
			}
		}

		double ageMeanCSW = ageSumCSW / numInfectedCSW;
		double ageSDCSW = sqrt(ageSumSqCSW / numInfectedCSW - ageMeanCSW * ageMeanCSW);
		_outStream << ageMeanCSW << Constants::Tab << ageSDCSW << Constants::Tab;
	}
	else
	{
		_outStream << "N/A" << Constants::Tab << "N/A" << Constants::Tab;
	}

	for(int i = (int)Entity::RiskLevel::Last - 1; i >= 0; i--)
	{
		double numInfected = 0;

        for(auto entity_type : {"male", "msmw", "msm", "female"})
        {
            numInfected += currTimeStepNumInfectedEntityTypeRiskEmployment[entity_type][i][(std::size_t)DemographicProfile::Employment::Csw];
		}

		if(numInfected != 0)
		{
			double ageSum  = 0;
			double ageSumSq = 0;

            for(auto entity_type : {"male", "msmw", "msm", "female"})
            {
                ageSum += currTimeStepAgeInfectionSumEntityTypeRiskEmployment[entity_type][i][(std::size_t)DemographicProfile::Employment::Csw];
                ageSumSq += currTimeStepAgeInfectionSumSqEntityTypeRiskEmployment[entity_type][i][(std::size_t)DemographicProfile::Employment::Csw];
			}

			double ageMean = ageSum / numInfected;
			double ageSD = sqrt(ageSumSq / numInfected - ageMean * ageMean);
			_outStream << ageMean << Constants::Tab << ageSD << Constants::Tab;
		}
		else
		{
			_outStream << "N/A" << Constants::Tab << "N/A" << Constants::Tab;
		}
	}

	for(int i = (int)Entity::RiskLevel::Last - 1; i >= 0; i--)
	{
        for(auto entity_type : {"male", "msmw", "msm", "female"})
        {
            auto numInfected = currTimeStepNumInfectedEntityTypeRiskEmployment[entity_type][i][(std::size_t)DemographicProfile::Employment::NonCsw];

			if(numInfected != 0)
			{
                double ageSum = static_cast<double>(currTimeStepAgeInfectionSumEntityTypeRiskEmployment[entity_type][i][(std::size_t)DemographicProfile::Employment::NonCsw]);
                double ageSumSq = static_cast<double>(currTimeStepAgeInfectionSumSqEntityTypeRiskEmployment[entity_type][i][(std::size_t)DemographicProfile::Employment::NonCsw]);
				double ageMean = ageSum / numInfected;
                double ageSD = std::sqrt(ageSumSq / numInfected - ageMean * ageMean);
				_outStream << ageMean << Constants::Tab << ageSD << Constants::Tab;
			}
			else
			{
				_outStream << "N/A" << Constants::Tab << "N/A" << Constants::Tab;
			}
		}
	}

	_outStream << std::endl;
	return intPrevalence;
}

} // namespace transm
