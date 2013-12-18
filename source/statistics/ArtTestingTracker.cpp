#include "ArtTestingTracker.h"
#include "../Population.h"
#include "../entities/Person.h"
#include "../cepac/SimContext.h"

//const char *ArtTestingTracker::RISK_GROUP_NAMES[] = {
//	"CSW High Risk", "CSW Low Risk", "Non-CSW High Risk Male", "Non-CSW High Risk Female", "Non-CSW Low Risk Male", "Non-CSW Low Risk Female"
//};

ArtTestingTracker::ArtTestingTracker()
{
	Reset();
}

ArtTestingTracker::~ArtTestingTracker()
{

}

void ArtTestingTracker::recordTest(Person *person, bool accepted, bool returned, SimContext::TEST_RESULT result)
{
	numTestsOffered++;

	if(accepted)
	{
		numTestsAccepted++;

		if(returned)
		{
			int riskIndex = person->getRiskLevel();
			const DmgProfile *demographicProfile = person->getDmgProfile();
			int genderIndex = demographicProfile->get(DmgProfile::GENDER);
			int employmentIndex = demographicProfile->get(DmgProfile::EMPLOYMENT);

			numTestsByBucket[riskIndex][genderIndex][employmentIndex]++;
			numTestsByResult[result]++;
		}
	}
}

void ArtTestingTracker::printArtRolloutOutcomes(int time, std::ostream &_outStream, Population *_population)
{
	std::vector<std::pair<int, int> > ageRanges;
	std::vector< boost::tuple<long, int, int> > currSizeByAgeRange = _population->getSizeByAgeRange();
	int numAgeRanges = currSizeByAgeRange.size();

	for(int i = 0; i < numAgeRanges; ++i)
	{
		int minAge = boost::tuples::get<Population::MIN_AGE_IN_MONTHS>(currSizeByAgeRange.at(i));
		int maxAge = boost::tuples::get<Population::MAX_AGE_IN_MONTHS>(currSizeByAgeRange.at(i));
		ageRanges.push_back(std::make_pair(minAge, maxAge));
	}

	if(time == 0)
	{
		buildHeader(ageRanges);
		PrintHeader(_outStream);
	}
}

void ArtTestingTracker::buildHeader(const std::vector<std::pair<int, int> > &ageRanges)
{
	SetHeaderCell(1, 1, "ART Rollout Outcomes");

	SetHeaderCell(1, 3, "Time");
	SetHeaderCell(2, 3, "Population Size");

	SetHeaderCell(3, 1, "Number of Tests");

	SetHeaderCell(3, 2, "Totals");
	SetHeaderCell(3, 3, "Offered");
	SetHeaderCell(4, 3, "Accepted");
	SetHeaderCell(5, 3, "Returned for Results");

	SetHeaderCell(6, 2, "Gender");
	SetHeaderCell(6, 3, "Females");
	SetHeaderCell(7, 3, "Males");

	int column = 8;
	for(int genderIndex = 0; genderIndex < 2; genderIndex++, column += (ageRanges.size() + 1))
	{
		SetHeaderCell(column, 1, genderIndex == 0 ? "Females" : "Males");

		SetHeaderCell(column, 2, "Non-Sexually Active Population");
		SetHeaderCell(column, 3, "All ages");
		//TODO: print all ages here

		SetHeaderCell(column + 1, 2, "Sexually Active Population");
		for(size_t i = 0; i < ageRanges.size(); ++i)
		{
			std::stringstream rangeString;
			rangeString << std::get<0>(ageRanges[i]) << "-" << std::get<1>(ageRanges[i]);
			SetHeaderCell(column + 1 + i, 3, rangeString.str());
		}
	}
}

void ArtTestingTracker::buildNumTestsHeader()
{

}

void ArtTestingTracker::buildNumEligibleHeader()
{

}

void ArtTestingTracker::buildNumEnrolledHeader()
{

}

void ArtTestingTracker::Reset()
{
	numTestsOffered = 0;
	numTestsAccepted = 0;

	for(int riskIndex = 0; riskIndex < Person::ENDRiskLevel; ++riskIndex)
	{
		for(int genderIndex = 0; genderIndex < DmgProfile::ENDGender; ++genderIndex)
		{
			for(int employmentIndex = 0; employmentIndex < DmgProfile::ENDGender; ++employmentIndex)
			{
				numTestsByBucket[riskIndex][genderIndex][employmentIndex] = 0;
			}
		}
	}

	for(int resultIndex = 0; resultIndex < SimContext::TEST_RESULT_NUM; ++resultIndex)
	{
		numTestsByResult[resultIndex] = 0;
	}
}