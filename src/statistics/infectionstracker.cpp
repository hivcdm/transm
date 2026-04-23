#include <iostream>
#include <numeric>
#include <sstream>

#include "infectionstracker.hpp"
#include "core/constants.hpp"
#include "core/population.hpp"

namespace transm {

InfectionsTracker::InfectionsTracker() :
	lastTwelvePopIncidenceRates(12, 0.0),
	lastTwelveSAPopIncidenceRates(12, 0.0)
{
}

void InfectionsTracker::Initialize()
{
	/* zero out all infection tallies */
	for(int i = 0; i < (int)SexualPartnership::Type::Last; ++i)
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

		for(std::size_t j = 0; j < (std::size_t)DemographicProfile::Gender::Last; j++)
		{
			for(std::size_t k = 0; k < (std::size_t)DemographicProfile::Gender::Last; k++)
			{
				incidentInfectionsByGender[i][j][k] = 0;
			}
		}

		currTimeExposuresByType[i] = 0;
		currTimeCondomUseByType[i] = 0;
	}

	for(std::size_t i = 0; i < (std::size_t)HVLStrata::Last; ++i)
	{
		totalIncidentInfections[i] = 0;
		totalExposures[i] = 0;
		currTimeStepIncidentInfs[i] = 0;
		currTimeExposures[i] = 0;
	}
	currTimeTotalExposures = 0;

    for(auto gender : enum_iterator<DemographicProfile::Gender>())
    {
		auto entity_type = (std::size_t)gender;
        for(std::size_t i = 0; i < (std::size_t)RiskLevel::Last; i++)
        {
            for(std::size_t k = 0; k < (std::size_t)DemographicProfile::Employment::Last; k++)
            {
                currTimeStepIncidentInfsGenderRiskEmployment[entity_type][i][k] = 0;
                totalIncidentInfsGenderRiskEmployment[entity_type][i][k] = 0;
            }
        }
    }

    currTimeStepNumInfected = 0;
    currTimeStepCD4InfectionSum = 0;
    currTimeStepCD4InfectionSumSq = 0;

    for(auto gender : enum_iterator<DemographicProfile::Gender>())
    {
		auto entity_type = (std::size_t)gender;
		totalIncidentInfsGender[entity_type] = 0;
        currTimeStepNumInfectedGender[entity_type] = 0;
        currTimeStepAgeInfectionSumGender[entity_type] = 0;
		currTimeStepAgeInfectionSumSqGender[entity_type] = 0;
	}

    for(std::size_t i = 0; i < (std::size_t)RiskLevel::Last; i++)
	{
		totalIncidentInfsRiskCSW[i] = 0;
		totalIncidentInfsRisk[i] = 0;
	}

    for(auto gender : enum_iterator<DemographicProfile::Gender>())
    {
		auto entity_type = (std::size_t)gender;
        for(std::size_t i = 0; i < (std::size_t)RiskLevel::Last; i++)
        {
            for(std::size_t k = 0; k < (std::size_t)DemographicProfile::Employment::Last; k++)
            {
                currTimeStepNumInfectedGenderRiskEmployment[entity_type][i][k] = 0;
                currTimeStepAgeInfectionSumGenderRiskEmployment[entity_type][i][k] = 0;
                currTimeStepAgeInfectionSumSqGenderRiskEmployment[entity_type][i][k] = 0;
            }
        }
    }
}

void InfectionsTracker::addToDetailedTrace(DemographicProfile::ProfileID _profileID)
{
	profileIDsForDetailedTrace.push_back(_profileID);
}

std::size_t InfectionsTracker::getCurrTimeStepIncidentInfsTotal()
{
	std::size_t infections = 0;

	for(std::size_t i = 0; i < (std::size_t)HVLStrata::Last; ++i)
	{
		infections += currTimeStepIncidentInfs[i];
	}

	return infections;
}

double InfectionsTracker::getPopAnnualIncidence()
{
	return calculateAnnualIncidence(lastTwelvePopIncidenceRates);
}

double InfectionsTracker::getSAPopAnnualIncidence()
{
	return calculateAnnualIncidence(lastTwelveSAPopIncidenceRates);
}

double InfectionsTracker::calculateAnnualIncidence(std::deque<double> lastTwelveIncidenceRates)
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

unsigned int InfectionsTracker::getCurrTimeStepNumInfected()
{
 	return InfectionsTracker::currTimeStepNumInfected;
}

std::size_t InfectionsTracker::getNumIncidentInfections()
{
	std::size_t infections = 0;

	for(std::size_t i = 0; i < (std::size_t)HVLStrata::Last; i++)
	{
		infections += totalIncidentInfections[i];
	}

	return infections;
}

/* returns the prevalence rate among sexually active pop */
double InfectionsTracker::getSAPrev(Population &_population)
{
	std::size_t totalInfected = 0;
    std::size_t currPopSize = _population.GetSize();
    std::size_t currSAPopSize = currPopSize - _population.GetNASize();

	/* Currently Infected */
	/* total the current infections */
	for(unsigned int i = 0; i < DemographicProfile::TotalNumBuckets; i++)
	{
		for(int j = 0; j < NUMBER_GENERATIONS_TO_TRACE; j++)
		{
			totalInfected += currPrevalentInfections[j][i];
		}
	}

	/* current pop and prevalence of sexually active population */
	/* Need total number of infected for SA population only */
	auto totalInfectedSA = totalInfected;
	DemographicProfile NAProfile;
    NAProfile.set(DemographicProfile::Demographic::SexualActivityStatus, (std::size_t)DemographicProfile::SexualActivityStatus::NotActive);
	std::vector<DemographicProfile::ProfileID> NAProfileIDs;
	NAProfile.selectProfileIDs(NAProfileIDs, nullptr);

    for(int NAProfileID : NAProfileIDs)
	{
        for(std::size_t j = 0; j < NUMBER_GENERATIONS_TO_TRACE; j++)
		{
			totalInfectedSA -= currPrevalentInfections[j][NAProfileID];
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

/* Resests counting of incident infections for time step */
void InfectionsTracker::resetIncidentInfections(Time time)
{
    /** Reset counter for incident infections and exposures for current timestep */
    for(std::size_t i = 0; i < (std::size_t)HVLStrata::Last; i++)
    {
        currTimeStepIncidentInfs[i] = 0;
        currTimeExposures[i] = 0;
    }
    currTimeTotalExposures = 0;
    currTimeStepNumInfected = 0;
    currTimeStepCD4InfectionSum = 0;
    currTimeStepCD4InfectionSumSq = 0;
    currTimeCondomUse = 0;

    for(int i = 0; i < (int)SexualPartnership::Type::Last; ++i)
    {
	    currTimeExposuresByType[i] = 0;
	    currTimeCondomUseByType[i] = 0;
    }

    for(auto gender : enum_iterator<DemographicProfile::Gender>())
    {
		auto entity_type = (std::size_t)gender;
        for(std::size_t i = 0; i < (std::size_t)RiskLevel::Last; i++)
        {
            for(std::size_t k = 0; k < (std::size_t)DemographicProfile::Employment::Last; k++)
            {
                currTimeStepIncidentInfsGenderRiskEmployment[entity_type][i][k] = 0;
            }
        }

        currTimeStepAgeInfectionSumGender[entity_type] = 0;
        currTimeStepAgeInfectionSumSqGender[entity_type] = 0;
        currTimeStepNumInfectedGender[entity_type] = 0;

        for(std::size_t i = 0; i < (std::size_t)RiskLevel::Last; i++)
        {
            for(std::size_t k = 0; k < (std::size_t)DemographicProfile::Employment::Last; k++)
            {
                currTimeStepAgeInfectionSumGenderRiskEmployment[entity_type][i][k] = 0;
                currTimeStepAgeInfectionSumSqGenderRiskEmployment[entity_type][i][k] = 0;
                currTimeStepNumInfectedGenderRiskEmployment[entity_type][i][k] = 0;
            }
        }

        for(auto &ageRangeSize : currTimeStepIncidentInfsGenderAge[entity_type])
        {
            ageRangeSize.second = 0;
        }
    }

	currTimeStep = time;
}


/* Initializes the counters for incident infections by age and gender */
void InfectionsTracker::initializeIncidentInfectionsByAge(
    const GenderArray<AgeRangeSizeContainer> &incident_by_gender_age,
    const OrientationArray<AgeRangeSizeContainer> &incident_by_orientation_age,
    const AgeRangeSizeContainer &incident_by_age)
{
    currTimeStepIncidentInfsGenderAge = incident_by_gender_age;
    currTimeStepIncidentInfsOrientationAge = incident_by_orientation_age;
    totalIncidentInfsAge = incident_by_age;
    totalIncidentInfsAge = incident_by_age;
}

/* Records a new exposure regardless of whether an infection happened or not */
void InfectionsTracker::recordExposure(Time time, const Entity *_infector,
    SexualPartnership::Type partnershipType, bool condomUsed)
{
	/* Reset counter for incident infections and exposures for current timestep */
	/* if this is the first time an InfectionsTracker function has been called. */
	if(time > currTimeStep)
	{
		resetIncidentInfections(time);
	}

	currTimeExposures[(std::size_t)_infector->getHVL()]++;
	currTimeTotalExposures++;
	currTimeExposuresByType[(std::size_t)partnershipType]++;
	totalExposures[(std::size_t)_infector->getHVL()]++;

	if (condomUsed) {
		currTimeCondomUse++;
		currTimeCondomUseByType[(std::size_t)partnershipType]++;
	}

}

/* records a New infection and also prints the infection out to a trace */
void InfectionsTracker::recordIncidentInfection(Time time,
    SexualPartnership::Type _partnershipType, const Entity *_infector,
    const Entity *_infected)
{
    assert(time.in_months() >= 0);

    /* check to see if the people are valid: not null, not dead, in a valid bucket */
    assert((_infector != nullptr) && (_infector->isAlive())
        && (Utility::within_range(_infector->getDemographicProfile()->getProfileID(), DemographicProfile::MIN, DemographicProfile::MAX)));
    assert((_infected != nullptr) && (_infected->isAlive())
        && (Utility::within_range(_infected->getDemographicProfile()->getProfileID(), DemographicProfile::MIN, DemographicProfile::MAX)));
    assert(_partnershipType < SexualPartnership::Type::Last);

    /* reset counter for incident infections and exposures for current timestep */
    if(time > currTimeStep)
    {
        resetIncidentInfections(time);
    }

    /* The person doing the infecting should be infected! */
    assert(_infector->getHVL() != HVLStrata::UNINFECTED);

    /* record infection */
    incidentInfectionsByDemographic[(std::size_t)_partnershipType][_infector->getDemographicProfile()->getProfileID()][_infected->getDemographicProfile()->getProfileID()]++;

	auto infectorGender = (std::size_t)_infector->getDemographicProfileVal<DemographicProfile::Gender>();
	auto infectedGender = (std::size_t)_infected->getDemographicProfileVal<DemographicProfile::Gender>();
	auto infectedEmployment = (std::size_t)_infected->getDemographicProfileVal<DemographicProfile::Employment>();

    incidentInfectionsByGender[(std::size_t)_partnershipType][infectorGender][infectedGender]++;

	currTimeStepIncidentInfs[(std::size_t)_infector->getHVL()]++;
    currTimeStepIncidentInfsGenderRiskEmployment[infectedGender][(std::size_t)_infected->getRiskLevel()][infectedEmployment]++;
    totalIncidentInfections[(std::size_t)_infector->getHVL()]++;
	auto infectedAge = _infected->getAge();
	double infectorCD4 = _infector->cd4;
	RiskLevel infectedRisk = _infected->getRiskLevel();

	auto square = [](double d) { return d * d; };

	currTimeStepNumInfected++;
	currTimeStepCD4InfectionSum += infectorCD4;
	currTimeStepCD4InfectionSumSq += infectorCD4 * infectorCD4;
    currTimeStepNumInfectedGender[infectedGender]++;
	currTimeStepAgeInfectionSumGender[infectedGender] += (std::size_t)infectedAge.in_months();
	currTimeStepAgeInfectionSumSqGender[infectedGender] += (std::size_t)square(infectedAge.in_months());
    currTimeStepNumInfectedGenderRiskEmployment[infectedGender][(std::size_t)infectedRisk][infectedEmployment]++;
	currTimeStepAgeInfectionSumGenderRiskEmployment[infectedGender][(std::size_t)infectedRisk][infectedEmployment] += (std::size_t)infectedAge.in_months();
	currTimeStepAgeInfectionSumSqGenderRiskEmployment[infectedGender][(std::size_t)infectedRisk][infectedEmployment] += (std::size_t)square(infectedAge.in_months());

    if(infectedEmployment == (std::size_t)DemographicProfile::Employment::Csw)
	{
        totalIncidentInfsRiskCSW[(std::size_t)infectedRisk]++;
	}

    totalIncidentInfsRisk[(std::size_t)_infected->getRiskLevel()]++;
    totalIncidentInfsGenderRiskEmployment[infectedGender][(std::size_t)_infected->getRiskLevel()][infectedEmployment]++;

    auto &genderIncidentInfectionsByAge = currTimeStepIncidentInfsGenderAge[infectedGender];
    totalIncidentInfsGender[infectedGender]++;

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

/* records the cd4 at transmission (requested by clinical out stream) */
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

void InfectionsTracker::setPrevalentInfections(std::array<DemographicArray, NUMBER_GENERATIONS_TO_TRACE> _prevalenceByBucket,
        const GenderArray<AgeRangeSizeContainer> &_prevalenceByGenderAge,
        GenderArray<RiskEmploymentArray> _prevalenceByGenderRiskEmployment,
        const OrientationArray<AgeRangeSizeContainer> &_prevalenceByOrientationAge,
        OrientationArray<RiskEmploymentArray> _prevalenceByOrientationRiskEmployment)
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

    for(auto gender : enum_iterator<DemographicProfile::Gender>())
    {
		auto entity_type = (std::size_t)gender;
        currPrevalentInfectionsGender[entity_type] = 0;

        for(std::size_t i = 0; i < (std::size_t)RiskLevel::Last; i++)
        {
            for(std::size_t k = 0; k < (std::size_t)DemographicProfile::Employment::Last; k++)
            {
                currPrevalentInfectionsGenderRiskEmployment[entity_type][i][k] = _prevalenceByGenderRiskEmployment[entity_type][i][k];
                currPrevalentInfectionsGender[entity_type] += _prevalenceByGenderRiskEmployment[entity_type][i][k];

                {
                    currPrevalentInfectionsGenderRiskEmployment[entity_type][i][k] = _prevalenceByGenderRiskEmployment[entity_type][i][k];
                    currPrevalentInfectionsGender[entity_type] += _prevalenceByGenderRiskEmployment[entity_type][i][k];
                }
            }
        }
        currPrevalentInfectionsGenderAge[entity_type] = _prevalenceByGenderAge.at(entity_type);
    }

    for(auto orientation : enum_iterator<DemographicProfile::SexualOrientation>())
    {
		auto entity_type = (std::size_t)orientation;
        currPrevalentInfectionsOrientation[entity_type] = 0;

        for(std::size_t i = 0; i < (std::size_t)RiskLevel::Last; i++)
        {
            for(std::size_t k = 0; k < (std::size_t)DemographicProfile::Employment::Last; k++)
            {
                currPrevalentInfectionsOrientationRiskEmployment[entity_type][i][k] = _prevalenceByOrientationRiskEmployment[entity_type][i][k];
                currPrevalentInfectionsOrientation[entity_type] += _prevalenceByOrientationRiskEmployment[entity_type][i][k];

                {
                    currPrevalentInfectionsOrientationRiskEmployment[entity_type][i][k] = _prevalenceByOrientationRiskEmployment[entity_type][i][k];
                    currPrevalentInfectionsOrientation[entity_type] += _prevalenceByOrientationRiskEmployment[entity_type][i][k];
                }
            }
        }
        currPrevalentInfectionsOrientationAge[entity_type] = _prevalenceByOrientationAge.at(entity_type);
    }
}

int InfectionsTracker::printInfections(EventParams &/*_eventParams*/, Time time, std::ostream &_outStream, Population *_population)
{
	assert(time.in_months() >= 0);

	/* total # infections this month */
	std::size_t totalInfected = 0;

	/* current population size */
	std::size_t currPopSize = _population->GetSize();
	std::size_t currSAPopSize = currPopSize - _population->GetNASize();

	/* total # of age ranges to print out */
	auto currSizeByAgeRange = _population->GetAgeRanges();
	auto numAgeRanges = currSizeByAgeRange.size();

	/* write headers for infections sheet */
	if(time.in_months() == 0)
	{
		std::ostringstream firstRow;
        std::ostringstream secondRow;
        std::ostringstream thirdRow;

		/* incidence-related headers */
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
		thirdRow << "Past-Year Pop Incidence" << Constants::Tab;
		firstRow << Constants::Tab;
		secondRow << Constants::Tab;
		thirdRow << "SA Pop Size" << Constants::Tab;
		firstRow << Constants::Tab;
		secondRow << Constants::Tab;
		thirdRow << "SA Prevalence" << Constants::Tab;
		firstRow << Constants::Tab;
		secondRow << Constants::Tab;
		thirdRow << "Past-Year SAPop Incidence" << Constants::Tab;

        /* write headers for by gender */
		firstRow << "Prevalent Cases";
        secondRow << "By Gender";
		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
            firstRow << Constants::Tab;
            secondRow << Constants::Tab;
            auto entity_type = (std::size_t)gender;
            thirdRow << DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::Gender)
                        .at(entity_type) << Constants::Tab;
        }

        /* write headers for by orientation */
        firstRow << "Prevalent Cases";
        secondRow << "By Orientation";
		for(auto orientation : enum_iterator<DemographicProfile::SexualOrientation>())
		{
            firstRow << Constants::Tab;
            secondRow << Constants::Tab;
            auto entity_type = (std::size_t)orientation;
            thirdRow << DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::SexualOrientation)
                        .at(entity_type) << Constants::Tab;
        }

		/* write out headers for population by age\sexual activity */
		firstRow << "Prevalent Cases";
        secondRow << "By Age";
		for(std::size_t i = 0; i < numAgeRanges; i++)
		{
			firstRow << Constants::Tab;
			secondRow << Constants::Tab;
			thirdRow << currSizeByAgeRange.at(i) << Constants::Tab;
		}

		/* write out headers for population by gender and age */
		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
            std::size_t entity_type = (std::size_t)gender;
            for(std::size_t i = 0; i < numAgeRanges; i++)
            {
                if(i == 0)
                {
                    firstRow << "Prevalent Cases";
                    secondRow << DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::Gender)
                        .at(entity_type);
                }

                firstRow << Constants::Tab;
                secondRow << Constants::Tab;
                thirdRow << currPrevalentInfectionsGenderAge[entity_type].at(i).first << Constants::Tab;
            }
        }

        /* write out headers for number infected by risk */
        firstRow << "Prevalent Cases" << Constants::Tab << Constants::Tab;
        secondRow << "By Risk Group" << Constants::Tab << Constants::Tab;
        thirdRow << "CSW High Risk" << Constants::Tab;
        thirdRow << "CSW Low Risk" << Constants::Tab;

		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			std::string entity_type = DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::Gender).at((std::size_t)gender);
            firstRow << Constants::Tab;
            secondRow << Constants::Tab;
            thirdRow << "Non-CSW High Risk " << entity_type << Constants::Tab;
        }

		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			std::string entity_type = DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::Gender).at((std::size_t)gender);
            firstRow << Constants::Tab;
            secondRow << Constants::Tab;
            thirdRow << "Non-CSW Low Risk " << entity_type << Constants::Tab;
        }

		/* write out headers for number of infections by generation (7 tabs) */
		firstRow << "Prevalent Cases" << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab
		         << Constants::Tab;
		secondRow << "Number Infected by Generation" << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
		          Constants::Tab << Constants::Tab;
		thirdRow << "0 (Prev.)" << Constants::Tab << "First" << Constants::Tab << "Second" << Constants::Tab << "Third" <<
		         Constants::Tab << "Fourth" << Constants::Tab << "Fifth+" << Constants::Tab;

        /* write out headers for number of exposures and infections by HVL */
		firstRow << "Exposures by HVL" << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
		         Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
		secondRow << Constants::Tab << Constants::Tab << Constants::Tab <<
		          Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
		thirdRow << "HVL 0-20" << Constants::Tab << "HVL 21-500" << Constants::Tab << "HVL 501-3000" <<
		         Constants::Tab << "HVL 3001-10000" << Constants::Tab << "HVL 10000-30000" << Constants::Tab << "HVL 30001-100000" <<
		         Constants::Tab << "HVL 100000+" << Constants::Tab << "HVL Primary" << Constants::Tab << "HVL Late Stage" <<
		         Constants::Tab;
		firstRow  << "Incident Cases" << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab
		         << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
		secondRow << "Infections by HVL (of Infector)" << Constants::Tab << Constants::Tab << Constants::Tab
		          << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
		thirdRow << "HVL 0-20" << Constants::Tab << "HVL 21-500" << Constants::Tab << "HVL 501-3000" <<
		         Constants::Tab << "HVL 3001-10000" << Constants::Tab << "HVL 10000-30000" << Constants::Tab << "HVL 30001-100000" <<
		         Constants::Tab << "HVL 100000+" << Constants::Tab << "HVL Primary" << Constants::Tab << "HVL Late Stage" <<
		         Constants::Tab;

		/* write out headers for number of infections by Age and gender */
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

		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			std::string entity_type = DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::Gender).at((std::size_t)gender);
            firstRow << Constants::Tab;
            secondRow << Constants::Tab;
            thirdRow << entity_type << Constants::Tab;
        }

        secondRow << "Risk Group" << Constants::Tab;
        thirdRow << "CSW High Risk" << Constants::Tab;
        thirdRow << "CSW Low Risk" << Constants::Tab;

		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			std::string entity_type = DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::Gender).at((std::size_t)gender);
            firstRow << Constants::Tab;
            secondRow << Constants::Tab;
            thirdRow << "Non-CSW High Risk " << entity_type << Constants::Tab;
        }

		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			std::string entity_type = DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::Gender).at((std::size_t)gender);
            firstRow << Constants::Tab;
            secondRow << Constants::Tab;
            thirdRow << "Non-CSW Low Risk " << entity_type << Constants::Tab;
        }

        firstRow << Constants::Tab;
        secondRow << Constants::Tab;

		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			std::string entity_type = DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::Gender).at((std::size_t)gender);
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

		/* write out headers for incident infections by relationship type */
        firstRow << "Total Infected in History (Prevalent Cases Excluded)";
        secondRow << "Relationship Type";

        for(auto header : {"Steady", "Regular", "Casual", "CSW"})
		{
			firstRow << Constants::Tab;
			secondRow << Constants::Tab;
			thirdRow << header << Constants::Tab;
		}

		/* write out headers that tally total infections based on age and gender and risk group */
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

		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			std::string entity_type = DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::Gender).at((std::size_t)gender);
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

		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			std::string entity_type = DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::Gender).at((std::size_t)gender);
            firstRow << Constants::Tab;
            secondRow << Constants::Tab;
            thirdRow << "Non-CSW High Risk " << entity_type << Constants::Tab;
        }

		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			std::string entity_type = DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::Gender).at((std::size_t)gender);
            firstRow << Constants::Tab;
            secondRow << Constants::Tab;
            thirdRow << "Non-CSW Low Risk " << entity_type << Constants::Tab;
        }

        firstRow << "Age at Infection" << Constants::Tab;
		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			std::string entity_type = DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::Gender).at((std::size_t)gender);
            firstRow << Constants::Tab << Constants::Tab;
			secondRow << entity_type << Constants::Tab << Constants::Tab;
            thirdRow << "Mean" << Constants::Tab << "SD" << Constants::Tab;
        }

		secondRow << "CSW" << Constants::Tab << Constants::Tab <<
		  "CSW High Risk" << Constants::Tab << Constants::Tab <<
		  "CSW Low Risk"  << Constants::Tab << Constants::Tab;
		thirdRow << "Mean" << Constants::Tab << "SD" << Constants::Tab <<
		  "Mean" << Constants::Tab << "SD" << Constants::Tab <<
		  "Mean" << Constants::Tab << "SD" << Constants::Tab;
		firstRow << Constants::Tab << Constants::Tab << Constants::Tab <<
		  Constants::Tab << Constants::Tab;

		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			std::string entity_type = DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::Gender).at((std::size_t)gender);
            firstRow << Constants::Tab << Constants::Tab;
            secondRow << "Non-CSW High Risk " << entity_type << Constants::Tab << Constants::Tab;
            thirdRow << "Mean" << Constants::Tab << "SD" << Constants::Tab;
        }

		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			std::string entity_type = DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::Gender).at((std::size_t)gender);
            firstRow << Constants::Tab << Constants::Tab;
            secondRow << "Non-CSW Low Risk " << entity_type << Constants::Tab << Constants::Tab;
            thirdRow << "Mean" << Constants::Tab << "SD" << Constants::Tab;
        }

		firstRow << "Condom Use" << Constants::Tab << Constants::Tab;;
		secondRow << Constants::Tab << Constants::Tab;
		thirdRow << "Total Exposures" << Constants::Tab;
		thirdRow << "Total Condoms Used" << Constants::Tab;

		secondRow << "Condom Use Per Event By Partnership Type";

        for(auto header : {"Steady", "Regular", "Casual", "CSW"})
		{
			firstRow << Constants::Tab;
			secondRow << Constants::Tab;
			thirdRow << header << Constants::Tab;
		}

#ifdef WHO_INFECTED_WHOM
		/* write out headers that tally infections from one DemographicProfile to another */
		/* loop through all used ProfileID's and create internal string buffer headers for future timestep trace output */
		auto infectorProfileID = profileIDsForDetailedTrace.begin();
		while(infectorProfileID != profileIDsForDetailedTrace.end())
		{
			/* first row profile str refers to infectors */
			firstRow << "Total Infected in History (Prevalent Cases Excluded)";
			secondRow << *DemographicProfile::toString(*infectorProfileID);
			auto infectedProfileID = profileIDsForDetailedTrace.begin();

			while(infectedProfileID != profileIDsForDetailedTrace.end())
			{
				firstRow << Constants::Tab;
				secondRow << Constants::Tab;

				/* second row profile str refers to infecteds */
				thirdRow << *DemographicProfile::toString(*infectedProfileID) << Constants::Tab;
				infectedProfileID++;
			}

			infectorProfileID++;
		}
#endif

		/* Write out string buffers to trace file */
		_outStream << firstRow.str() << std::endl;
		_outStream << secondRow.str() << std::endl;
		_outStream << thirdRow.str() << std::endl;

	}

    //* END OF HEADERS *//

	/* if no incident infections happened during this time, then make sure that we have 0 in the currTime
	 * incident infections and exposures */
	if(time > currTimeStep)
	{
		resetIncidentInfections(time);
	}

	/** get current total infections */
	/* Month */
	if(time.in_months() == 0)
	{
		_outStream << "init" << Constants::Tab;
	}
	else
	{
		_outStream << time.in_months() << Constants::Tab;
	}

	/* New Infections */
	std::size_t newInfections = getCurrTimeStepIncidentInfsTotal();
	_outStream << newInfections << Constants::Tab;

	/* DEBUG: sanity-check counting invariant — compare HVL sum (newInfections) against
	 * two other counters that should equal it: flat gender counter and sum of age buckets. */
	{
		std::size_t genderSum = 0;
		std::size_t ageBucketSum = 0;
		for(auto g : enum_iterator<DemographicProfile::Gender>())
		{
			auto gi = (std::size_t)g;
			genderSum += currTimeStepNumInfectedGender[gi];
			for(const auto &ab : currTimeStepIncidentInfsGenderAge[gi])
			{
				ageBucketSum += ab.second;
			}
		}
		if(genderSum != newInfections || ageBucketSum != newInfections)
		{
			std::cerr << "[InfectionsTracker] invariant violated at month "
			          << time.in_months() << ": newInfections=" << newInfections
			          << " genderCounter(M+F)=" << genderSum
			          << " ageBucketSum(M+F)=" << ageBucketSum
			          << " (gender M=" << currTimeStepNumInfectedGender[(std::size_t)DemographicProfile::Gender::Male]
			          << " F=" << currTimeStepNumInfectedGender[(std::size_t)DemographicProfile::Gender::Female]
			          << ")" << std::endl;
		}
	}

	/* Total Infected in History */
	_outStream << getNumIncidentInfections() << Constants::Tab;

	/** Currently Infected */
	/* total the current infections */
	for(unsigned int i = 0; i < DemographicProfile::TotalNumBuckets; i++)
	{
		for(int j = 0; j < NUMBER_GENERATIONS_TO_TRACE; j++)
		{
			totalInfected += currPrevalentInfections[j][i];
		}
	}
	_outStream << totalInfected << Constants::Tab;

	/* Total Population Size */
	_outStream << currPopSize << Constants::Tab;

	/* Current Population Prevalence */
	double currPrevalence = totalInfected / (double)currPopSize;
	_outStream << currPrevalence << Constants::Tab;

	/* Calculate the past-year incidence for the TOTAL POPULATION */
	double monthlyPopIncidence = double(newInfections) / double(currPopSize - totalInfected);

	/* Push the monthly incidence onto the deque */
	lastTwelvePopIncidenceRates.push_back(monthlyPopIncidence);

	/* Pop off the oldest incidence rate */
	lastTwelvePopIncidenceRates.pop_front();
	assert(lastTwelvePopIncidenceRates.size() == 12);
	_outStream << calculateAnnualIncidence(lastTwelvePopIncidenceRates) << Constants::Tab;

	/* Current pop and prevalence of sexually active population */
	/* Need total number of infected for SA population only */
	std::size_t totalInfectedSA = totalInfected;
	DemographicProfile NAProfile;
	NAProfile.set(DemographicProfile::Demographic::SexualActivityStatus,
	    (std::size_t)DemographicProfile::SexualActivityStatus::NotActive);
	std::vector<DemographicProfile::ProfileID> NAProfileIDs;
	NAProfile.selectProfileIDs(NAProfileIDs, nullptr);
	for(int NAProfileID : NAProfileIDs)
	{
		for(size_t j = 0; j < NUMBER_GENERATIONS_TO_TRACE; j++)
		{
			totalInfectedSA -= currPrevalentInfections[j][NAProfileID];
		}
	}
	double currPrevalenceSA = (totalInfectedSA) / (double)currSAPopSize;
	_outStream << currSAPopSize << Constants::Tab;
	_outStream << currPrevalenceSA << Constants::Tab;

	/* Calculate the past-year incidence for the SEXUALLY ACTIVE POPULATION */
	double monthlySAPopIncidence = double(newInfections) / double(currSAPopSize - totalInfectedSA);

	/* Push the monthly incidence onto the deque */
	lastTwelveSAPopIncidenceRates.push_back(monthlySAPopIncidence);

	/* Pop off the oldest incidence rate */
	lastTwelveSAPopIncidenceRates.pop_front();
	_outStream << calculateAnnualIncidence(lastTwelveSAPopIncidenceRates) << Constants::Tab;

	/* Multiply by 100 and round to nearest integer for graphical output */
	int intPrevalence = (int)(100 * currPrevalence + 0.5);

    /* output monthly incident infections by gender, derived by summing the age-bucket
     * counter so the total always matches the "Infections By Age" totals. */
    for(auto gender : enum_iterator<DemographicProfile::Gender>())
    {
        auto entity_type = (std::size_t)gender;
        std::size_t monthlyIncidentForGender = 0;
        for(const auto &ageRangeSize : currTimeStepIncidentInfsGenderAge[entity_type])
        {
            monthlyIncidentForGender += ageRangeSize.second;
        }
        _outStream << monthlyIncidentForGender << Constants::Tab;
    }

    /* sum and output infections by orientation */
    OrientationArray<std::size_t> totalInfectedOrientation = {0};
	for(std::size_t i = 0; i < numAgeRanges; i++)
	{
		for(auto orientation : enum_iterator<DemographicProfile::SexualOrientation>())
		{
			auto entity_type = (std::size_t)orientation;
            totalInfectedOrientation[entity_type] += currPrevalentInfectionsOrientationAge[entity_type].at(i).second;
        }
	}
    for(auto orientation : enum_iterator<DemographicProfile::SexualOrientation>())
    {
        auto entity_type = (std::size_t)orientation;
        _outStream << totalInfectedOrientation[entity_type] << Constants::Tab;
    }

	/* output infections by age */
    for(std::size_t i = 0; i < numAgeRanges; i++)
    {
        std::size_t sum_all_genders = 0;
        for(auto gender : enum_iterator<DemographicProfile::Gender>())
        {
            auto entity_type = (std::size_t)gender;
            sum_all_genders += currPrevalentInfectionsGenderAge[entity_type].at(i).second;
        }
        _outStream << sum_all_genders << Constants::Tab;
    }

	/* output infections by age and gender */
    for(auto gender : enum_iterator<DemographicProfile::Gender>())
    {
        auto entity_type = (std::size_t)gender;
        for(std::size_t i = 0; i < numAgeRanges; i++)
        {
            _outStream << currPrevalentInfectionsGenderAge[entity_type].at(i).second << Constants::Tab;
        }
    }

	/* output prevalent infections by risk */
	std::size_t totalInfectedCSW = 0;
	DemographicProfile CSWProfile;
    CSWProfile.set(DemographicProfile::Demographic::Employment, (std::size_t)DemographicProfile::Employment::Csw);
	std::vector<DemographicProfile::ProfileID> CSWProfileIDs;
	CSWProfile.selectProfileIDs(CSWProfileIDs, nullptr);

	for(int CSWProfileID : CSWProfileIDs)
	{
		for(std::size_t j = 0; j < NUMBER_GENERATIONS_TO_TRACE; j++)
		{
			totalInfectedCSW += currPrevalentInfections[j][CSWProfileID];
		}
	}

    _outStream
	  << currPrevalentInfectionsGenderRiskEmployment[(std::size_t)DemographicProfile::Gender::Male][(std::size_t)RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::Csw]
        + currPrevalentInfectionsGenderRiskEmployment[(std::size_t)DemographicProfile::Gender::Female][(std::size_t)RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::Csw]
        << Constants::Tab;
    _outStream
        << currPrevalentInfectionsGenderRiskEmployment[(std::size_t)DemographicProfile::Gender::Male][(std::size_t)RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::Csw]
        + currPrevalentInfectionsGenderRiskEmployment[(std::size_t)DemographicProfile::Gender::Female][(std::size_t)RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::Csw]
        << Constants::Tab;

	for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		auto entity_type = (std::size_t)gender;
        _outStream
            << currPrevalentInfectionsGenderRiskEmployment[entity_type][(std::size_t)RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::NonCsw]
            << Constants::Tab;
    }

	for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		auto entity_type = (std::size_t)gender;
        _outStream
            << currPrevalentInfectionsGenderRiskEmployment[entity_type][(std::size_t)RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::NonCsw]
            << Constants::Tab;
    }

	/* We're hard wiring 6 (Prev + 5) generations of reporting for now */
	std::size_t totalInfectedByGeneration[NUMBER_GENERATIONS_TO_TRACE];

	for(int i = 0; i < NUMBER_GENERATIONS_TO_TRACE; i++)
	{
		totalInfectedByGeneration[i] = 0;

		for(std::size_t j = 0; j < DemographicProfile::TotalNumBuckets; j++)
		{
			totalInfectedByGeneration[i] += currPrevalentInfections[i][j];
		}
	}

	/* Write out number of infections by generation (7 tabs) */
	for(unsigned long i : totalInfectedByGeneration)
	{
		_outStream << i << Constants::Tab;
	}

	/* Print out exposures and infections by viral load */
	for(std::size_t i = 1; i < (std::size_t)HVLStrata::Last; i++)
	{
		_outStream << currTimeExposures[i] << Constants::Tab;
	}
	for(std::size_t i = 1; i < (std::size_t)HVLStrata::Last; i++)
	{
		_outStream << currTimeStepIncidentInfs[i] << Constants::Tab;
	}

	/* Print out incident infections by age and gender and risk */
	GenderArray<std::size_t> incidentInfsGender;
//    std::size_t sumIncidentInfs = 0;

    for(std::size_t i = 0; i < numAgeRanges; i++)
	{
        std::size_t sumIncidentAge = 0;

		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			auto entity_type = (std::size_t)gender;
            auto incident_entity_type_age = currTimeStepIncidentInfsGenderAge[entity_type].at(i).second;
            incidentInfsGender[entity_type] += incident_entity_type_age;
//            sumIncidentInfs += incident_entity_type_age;
            sumIncidentAge += incident_entity_type_age;
        }

        _outStream << sumIncidentAge << Constants::Tab;
	}

	for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		auto entity_type = (std::size_t)gender;
        _outStream << incidentInfsGender[entity_type] << Constants::Tab;
    }

    _outStream
	  << currTimeStepIncidentInfsGenderRiskEmployment[(std::size_t)DemographicProfile::Gender::Male][(std::size_t)RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::Csw]
        + currTimeStepIncidentInfsGenderRiskEmployment[(std::size_t)DemographicProfile::Gender::Female][(std::size_t)RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::Csw]
        << Constants::Tab;

    _outStream
        << currTimeStepIncidentInfsGenderRiskEmployment[(std::size_t)DemographicProfile::Gender::Male][(std::size_t)RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::Csw]
        + currTimeStepIncidentInfsGenderRiskEmployment[(std::size_t)DemographicProfile::Gender::Female][(std::size_t)RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::Csw]
        << Constants::Tab;

	for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		auto entity_type = (std::size_t)gender;
        _outStream
            << currTimeStepIncidentInfsGenderRiskEmployment[entity_type][(std::size_t)RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::NonCsw]
            << Constants::Tab;
    }

	for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		auto entity_type = (std::size_t)gender;
        _outStream
            << currTimeStepIncidentInfsGenderRiskEmployment[entity_type][(std::size_t)RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::NonCsw]
            << Constants::Tab;
    }

	for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		auto entity_type = (std::size_t)gender;
        for(std::size_t i = 0; i < numAgeRanges; i++)
        {
            _outStream << currTimeStepIncidentInfsGenderAge[entity_type].at(i).second << Constants::Tab;
        }
    }

	/* Print out incident infections by relationship type */
    /* steady,reg,cas,csw:male<->fem */
    _outStream << incidentInfectionsByGender[0][0][3] + incidentInfectionsByGender[0][3][0] << Constants::Tab;
    _outStream << incidentInfectionsByGender[1][0][3] + incidentInfectionsByGender[1][3][0] << Constants::Tab;
    _outStream << incidentInfectionsByGender[2][0][3] + incidentInfectionsByGender[2][3][0] << Constants::Tab;
    _outStream << incidentInfectionsByGender[3][0][3] + incidentInfectionsByGender[3][3][0] << Constants::Tab;

	/* write out all incident infections that happened in history stratified by age and gender and risk */
    for(std::size_t i = 0; i < numAgeRanges; i++)
	{
		_outStream << totalIncidentInfsAge.at(i).second << Constants::Tab;
	}

	for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		auto entity_type = (std::size_t)gender;
        _outStream << totalIncidentInfsGender[entity_type] << Constants::Tab;
    }

    _outStream
	  << totalIncidentInfsGenderRiskEmployment[(std::size_t)DemographicProfile::Gender::Male][(std::size_t)RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::Csw]
        + totalIncidentInfsGenderRiskEmployment[(std::size_t)DemographicProfile::Gender::Female][(std::size_t)RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::Csw]
        << Constants::Tab;

    _outStream
        << totalIncidentInfsGenderRiskEmployment[(std::size_t)DemographicProfile::Gender::Male][(std::size_t)RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::Csw]
        + totalIncidentInfsGenderRiskEmployment[(std::size_t)DemographicProfile::Gender::Female][(std::size_t)RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::Csw]
        << Constants::Tab;

	for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		auto entity_type = (std::size_t)gender;
        _outStream
            << totalIncidentInfsGenderRiskEmployment[entity_type][(std::size_t)RiskLevel::HIGH][(std::size_t)DemographicProfile::Employment::NonCsw]
            << Constants::Tab;
    }

	for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		auto entity_type = (std::size_t)gender;
        _outStream
            << totalIncidentInfsGenderRiskEmployment[entity_type][(std::size_t)RiskLevel::LOW][(std::size_t)DemographicProfile::Employment::NonCsw]
            << Constants::Tab;
    }

	/* write out age of infection for incident infections that month (mean and SD) */
	for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		auto entity_type = (std::size_t)gender;
		if(currTimeStepNumInfectedGender[entity_type] != 0)
		{
			double ageMean = currTimeStepAgeInfectionSumGender[entity_type] / (double) currTimeStepNumInfectedGender[entity_type];
			double ageSD = sqrt(currTimeStepAgeInfectionSumSqGender[entity_type] / (double) currTimeStepNumInfectedGender[entity_type] -
			                    ageMean * ageMean);
			_outStream << ageMean << Constants::Tab << ageSD << Constants::Tab;
		}
		else
		{
			_outStream << "N/A" << Constants::Tab << "N/A" << Constants::Tab;
		}
	}

	double numInfectedCSW = 0;

	for(std::size_t i = 0; i < (std::size_t)RiskLevel::Last; i++)
	{
		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			auto entity_type = (std::size_t)gender;
            numInfectedCSW += currTimeStepNumInfectedGenderRiskEmployment[entity_type][i][(std::size_t)DemographicProfile::Employment::Csw];
		}
	}

	if(numInfectedCSW != 0)
	{
		double ageSumCSW  = 0;
		double ageSumSqCSW = 0;

		for(std::size_t i = 0; i < (std::size_t)RiskLevel::Last; i++)
		{
			for(auto gender : enum_iterator<DemographicProfile::Gender>())
			{
				auto entity_type = (std::size_t)gender;
                ageSumCSW += currTimeStepAgeInfectionSumGenderRiskEmployment[entity_type][i][(std::size_t)DemographicProfile::Employment::Csw];
                ageSumSqCSW += currTimeStepAgeInfectionSumSqGenderRiskEmployment[entity_type][i][(std::size_t)DemographicProfile::Employment::Csw];
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

	for(int i = (int)RiskLevel::Last - 1; i >= 0; i--)
	{
		double numInfected = 0;

		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			auto entity_type = (std::size_t)gender;
            numInfected += currTimeStepNumInfectedGenderRiskEmployment[entity_type][i][(std::size_t)DemographicProfile::Employment::Csw];
		}

		if(numInfected != 0)
		{
			double ageSum  = 0;
			double ageSumSq = 0;

			for(auto gender : enum_iterator<DemographicProfile::Gender>())
			{
				auto entity_type = (std::size_t)gender;
                ageSum += currTimeStepAgeInfectionSumGenderRiskEmployment[entity_type][i][(std::size_t)DemographicProfile::Employment::Csw];
                ageSumSq += currTimeStepAgeInfectionSumSqGenderRiskEmployment[entity_type][i][(std::size_t)DemographicProfile::Employment::Csw];
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

	for(int i = (int)RiskLevel::Last - 1; i >= 0; i--)
	{
		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			auto entity_type = (std::size_t)gender;
            auto numInfected = currTimeStepNumInfectedGenderRiskEmployment[entity_type][i][(std::size_t)DemographicProfile::Employment::NonCsw];

			if(numInfected != 0)
			{
                auto ageSum = static_cast<double>(currTimeStepAgeInfectionSumGenderRiskEmployment[entity_type][i][(std::size_t)DemographicProfile::Employment::NonCsw]);
                auto ageSumSq = static_cast<double>(currTimeStepAgeInfectionSumSqGenderRiskEmployment[entity_type][i][(std::size_t)DemographicProfile::Employment::NonCsw]);
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

	/* Sex Act and Condom Use Outputs */
	_outStream << currTimeTotalExposures << Constants::Tab;
	_outStream << currTimeCondomUse << Constants::Tab;

	for(int partnershipType = 0; partnershipType < (std::size_t)SexualPartnership::Type::Last; ++partnershipType)
	{
		auto exposures =  currTimeExposuresByType[partnershipType];
		auto condoms_used = currTimeCondomUseByType[partnershipType];

		if (exposures > 0)
			_outStream << (double)condoms_used / (double)exposures <<  Constants::Tab;
		else
			_outStream << 0 << Constants::Tab;
	}

#ifdef WHO_INFECTED_WHOM
	/* write out all incident infections that happened in history */
	/* only ProfileID's in profileIDsForDetailedTrace are included */
	auto infectorProfileID = profileIDsForDetailedTrace.begin();
	while(infectorProfileID != profileIDsForDetailedTrace.end())
	{
		auto infectedProfileID = profileIDsForDetailedTrace.begin();

		while(infectedProfileID != profileIDsForDetailedTrace.end())
		{
			/* tally all the infections that happened from curr infectorProfileID -> curr infectedProfileID */
            std::size_t infs = 0;

			for(int partnershipType = 0; partnershipType < (int)SexualPartnership::Type::Last; ++partnershipType)
			{
				infs += incidentInfectionsByDemographic[partnershipType][*infectorProfileID][*infectedProfileID];
			}

			_outStream << infs << Constants::Tab;
			infectedProfileID++;
		}

		infectorProfileID++;
	}
#endif

	_outStream << std::endl;
	return intPrevalence;
}

} // namespace transm
