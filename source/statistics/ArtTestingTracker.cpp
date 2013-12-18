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
	++numTestsOffered;

	if(accepted)
	{
		++numTestsAccepted;

		if(returned)
		{
			int riskIndex = person->getRiskLevel();
			const DmgProfile *demographicProfile = person->getDmgProfile();
			int genderIndex = demographicProfile->get(DmgProfile::GENDER);
			int employmentIndex = demographicProfile->get(DmgProfile::EMPLOYMENT);

			++numTestsByBucket[riskIndex][genderIndex][employmentIndex];
			++numTestsByResult[result];
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
	SetHeaderCell(0, 0, "ART Rollout Outcomes");
	SetHeaderCell(2, 0, "Females");
	SetHeaderCell(10, 0, "Males");
	SetHeaderCell(20, 0, "CD4 Count");
	SetHeaderCell(1, 0, "Non-Sexually Active Population");
	SetHeaderCell(2, 0, "Sexually Active Population");
	SetHeaderCell(10, 0, "Non-Sexually Active Population");
	SetHeaderCell(11, 0, "Sexually Active Population");
	SetHeaderCell(15, 0, "Risk Group");
	SetHeaderCell(16, 0, "Female");
	SetHeaderCell(18, 0, "Male");
	SetHeaderCell(19, 0, "Test Result");

	SetHeaderCell(1, 2, "All ages");
	for(size_t i = 0; i < ageRanges.size(); ++i)
	{
		std::stringstream rangeString;
		rangeString << std::get<0>(ageRanges[i]) << "-" << std::get<1>(ageRanges[i]);
		SetHeaderCell(2, 2, rangeString.str());
	}

	SetHeaderCell(1, 2, "All ages");
	for(size_t i = 0; i < ageRanges.size(); ++i)
	{
		std::stringstream rangeString;
		rangeString << std::get<0>(ageRanges[i]) << "-" << std::get<1>(ageRanges[i]);
		SetHeaderCell(2, 2, rangeString.str());
	}

	SetHeaderCell(20, 2, "Number of Test Offered");
	SetHeaderCell(20, 2, "Number of Test Accepted");
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
	std::fill(&numTestsByBucket[0][0][0], &numTestsByBucket[0][0][0] + sizeof(numTestsByBucket), 0);
	std::fill(&numTestsByResult[0], &numTestsByResult[0] + SimContext::TEST_RESULT_NUM, 0);
}