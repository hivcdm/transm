#include "ArtTestingTracker.h"
#include "../Population.h"
#include "../entities/Person.h"
#include "../cepac/SimContext.h"

const std::string ArtTestingTracker::RISK_GROUP_NAMES[] = 
{
	"CSW High Risk", 
	"CSW Low Risk",
	"Non-CSW High Risk Male",
	"Non-CSW High Risk Female",
	"Non-CSW Low Risk Male",
	"Non-CSW Low Risk Female"
};

ArtTestingTracker::ArtTestingTracker()
{
	Reset();
}

ArtTestingTracker::~ArtTestingTracker()
{

}

void ArtTestingTracker::SetAgeRanges(const std::vector<boost::tuple<long, int, int> > &ageRangeSizes)
{
	ageRanges.clear();

	int numAgeRanges = static_cast<int>(ageRangeSizes.size());

	for(int i = 0; i < numAgeRanges; ++i)
	{
		int minAge = boost::tuples::get<Population::MIN_AGE_IN_MONTHS>(ageRangeSizes.at(i));
		int maxAge = boost::tuples::get<Population::MAX_AGE_IN_MONTHS>(ageRangeSizes.at(i));
		ageRanges.push_back(std::make_pair(minAge, maxAge));
	}

	testsByBucketCounter.SetNumAgeGroups(numAgeRanges);
}

void ArtTestingTracker::recordTest(Person *person, bool accepted, bool returned, SimContext::TEST_RESULT result)
{
	numTestsOffered++;

	if(accepted)
	{
		numTestsAccepted++;

		if(returned)
		{
			numTestsReturnedFor++;

			int age = person->getAge(MONTH);
			int ageRangeIndex = -1;
			for(size_t i = 0; i < ageRanges.size(); ++i)
			{
				if(age >= ageRanges[i].first && age <= ageRanges[i].second)
				{
					ageRangeIndex = i;
				}
			}
			assert(ageRangeIndex != -1);

			const DmgProfile *demographicProfile = person->getDmgProfile();
			
			CountingBucket bucket(static_cast<DmgProfile::SexualActivityStatus>(demographicProfile->get(DmgProfile::SEXUAL_ACTIVITY_STATUS)),
				static_cast<DmgProfile::Gender>(demographicProfile->get(DmgProfile::GENDER)),
				static_cast<DmgProfile::SexualOrientation>(demographicProfile->get(DmgProfile::SEXUAL_ORIENTATION)),
				static_cast<DmgProfile::RelationshipStatus>(demographicProfile->get(DmgProfile::RELATIONSHIP_STATUS)),
				static_cast<DmgProfile::Employment>(demographicProfile->get(DmgProfile::EMPLOYMENT)),
				person->getRiskLevel(),
				ageRangeIndex);

			testsByBucketCounter.Increment(bucket);
			numTestsByResult[result]++;
		}
	}
}

void ArtTestingTracker::printArtRolloutOutcomes(int time, std::ostream &_outStream, Population *_population)
{
	if(time == 0)
	{
		buildHeader();
		PrintHeader(_outStream);
	}

	buildRow(time, _population);
	PrintRow(_outStream);
	Reset();
}

void ArtTestingTracker::buildHeader()
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

void ArtTestingTracker::buildRow(int time, Population *_population)
{
	if(time == 0)
	{
		PushElement("init");
	}
	else
	{
		PushElement(time);
	}

	PushElement(_population->getSize());
	PushElement(numTestsOffered);
	PushElement(numTestsAccepted);
	PushElement(numTestsReturnedFor);
	PushElement(testsByBucketCounter.GetCount(CountingBucket(DmgProfile::ENDSexualActivityStatus,
		DmgProfile::FEMALE,
		DmgProfile::ENDSexualOrientation,
		DmgProfile::ENDRelationshipStatus,
		DmgProfile::ENDEmployment,
		Person::ENDRiskLevel,
		ageRanges.size())));
	PushElement(testsByBucketCounter.GetCount(CountingBucket(DmgProfile::ENDSexualActivityStatus,
		DmgProfile::MALE,
		DmgProfile::ENDSexualOrientation,
		DmgProfile::ENDRelationshipStatus,
		DmgProfile::ENDEmployment,
		Person::ENDRiskLevel,
		ageRanges.size())));
	PushElement(testsByBucketCounter.GetCount(CountingBucket(DmgProfile::NA,
		DmgProfile::FEMALE,
		DmgProfile::ENDSexualOrientation,
		DmgProfile::ENDRelationshipStatus,
		DmgProfile::ENDEmployment,
		Person::ENDRiskLevel,
		ageRanges.size())));
	for(int i = 0; i < ageRanges.size(); i++)
	{
		PushElement(testsByBucketCounter.GetCount(CountingBucket(DmgProfile::SA,
			DmgProfile::FEMALE,
			DmgProfile::ENDSexualOrientation,
			DmgProfile::ENDRelationshipStatus,
			DmgProfile::ENDEmployment,
			Person::ENDRiskLevel,
			i)));
	}
	PushElement(testsByBucketCounter.GetCount(CountingBucket(DmgProfile::NA,
		DmgProfile::MALE,
		DmgProfile::ENDSexualOrientation,
		DmgProfile::ENDRelationshipStatus,
		DmgProfile::ENDEmployment,
		Person::ENDRiskLevel,
		ageRanges.size())));
	for(int i = 0; i < ageRanges.size(); i++)
	{
		PushElement(testsByBucketCounter.GetCount(CountingBucket(DmgProfile::SA,
			DmgProfile::MALE,
			DmgProfile::ENDSexualOrientation,
			DmgProfile::ENDRelationshipStatus,
			DmgProfile::ENDEmployment,
			Person::ENDRiskLevel,
			i)));
	}
}

void ArtTestingTracker::Reset()
{
	numTestsOffered = 0;
	numTestsAccepted = 0;
	numTestsReturnedFor = 0;

	testsByBucketCounter.Clear();

	for(int resultIndex = 0; resultIndex < SimContext::TEST_RESULT_NUM; ++resultIndex)
	{
		numTestsByResult[resultIndex] = 0;
	}
}