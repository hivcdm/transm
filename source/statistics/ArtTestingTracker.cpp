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

ArtTestingTracker::ArtTestingTracker() :
    numTestsOffered(0),
    numTestsAccepted(0),
    numTestsReturnedFor(0)
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
			testsByBucketCounter.Increment(CountingBucket(person, ageRanges));
			numTestsByResult[result]++;
		}
	}
}

void ArtTestingTracker::recordEligiblePerson(Person *person)
{
	eligibleByBucketCounter.Increment(CountingBucket(person, ageRanges));
}

void ArtTestingTracker::recordTreatedPerson(Person *person)
{
	enrolledByBucketCounter.Increment(CountingBucket(person, ageRanges));
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

		SetHeaderCell(column + 1, 2, "Sexually Active Population");
		for(size_t i = 0; i < ageRanges.size(); ++i)
		{
			std::stringstream rangeString;
			rangeString << std::get<0>(ageRanges[i]) << "-" << std::get<1>(ageRanges[i]);
			SetHeaderCell(column + 1 + i, 3, rangeString.str());
		}
	}

	for(int cd4StratumIndex = 0; cd4StratumIndex < Person::ENDCD4Strata; ++cd4StratumIndex, ++column)
	{
		SetHeaderCell(column, 3, SimContext::CD4_STRATA_STRS[cd4StratumIndex]);
	}

	for(int testResultIndex = 0; testResultIndex < SimContext::TEST_RESULT_NUM; ++testResultIndex, ++column)
	{
		SetHeaderCell(column, 3, SimContext::TEST_RESULT_STRS[testResultIndex]);
	}

	SetHeaderCell(column, 1, "Number Eligible");

	SetHeaderCell(column, 2, "Gender");
	SetHeaderCell(column, 3, "Females");
	SetHeaderCell(column + 1, 3, "Males");

	column += 2;

	for(int genderIndex = 0; genderIndex < 2; genderIndex++, column += (ageRanges.size() + 1))
	{
		SetHeaderCell(column, 1, genderIndex == 0 ? "Females" : "Males");

		SetHeaderCell(column, 2, "Non-Sexually Active Population");
		SetHeaderCell(column, 3, "All ages");

		SetHeaderCell(column + 1, 2, "Sexually Active Population");
		for(size_t i = 0; i < ageRanges.size(); ++i)
		{
			std::stringstream rangeString;
			rangeString << std::get<0>(ageRanges[i]) << "-" << std::get<1>(ageRanges[i]);
			SetHeaderCell(column + 1 + i, 3, rangeString.str());
		}
	}

	SetHeaderCell(column, 1, "Number Enrolled in ART");

	//SetHeaderCell(column, 3, "Total");

	SetHeaderCell(column, 2, "Gender");
	SetHeaderCell(column, 3, "Females");
	SetHeaderCell(column + 1, 3, "Males");

	column += 2;

	for(int genderIndex = 0; genderIndex < 2; genderIndex++, column += (ageRanges.size() + 1))
	{
		SetHeaderCell(column, 1, genderIndex == 0 ? "Females" : "Males");

		SetHeaderCell(column, 2, "Non-Sexually Active Population");
		SetHeaderCell(column, 3, "All ages");

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

	PushElement(static_cast<int>(_population->getSize()));
	PushElement(numTestsOffered);
	PushElement(numTestsAccepted);
	PushElement(numTestsReturnedFor);

	CountingBucket bucket = CountingBucket(DmgProfile::ENDSexualActivityStatus, DmgProfile::ENDGender, DmgProfile::ENDSexualOrientation,
		DmgProfile::ENDRelationshipStatus, DmgProfile::ENDEmployment, Person::ENDRiskLevel, ageRanges.size(), Person::ENDCD4Strata);

	bucket.gender = DmgProfile::FEMALE;
	PushElement(testsByBucketCounter.GetCount(bucket));
	bucket.gender = DmgProfile::MALE;
	PushElement(testsByBucketCounter.GetCount(bucket));

	bucket.sexualActivityStatus = DmgProfile::NA;
	PushElement(testsByBucketCounter.GetCount(bucket));

	bucket.sexualActivityStatus = DmgProfile::SA;
	bucket.gender = DmgProfile::FEMALE;
	for(bucket.ageGroup = 0; bucket.ageGroup < ageRanges.size(); ++bucket.ageGroup)
	{
		PushElement(testsByBucketCounter.GetCount(bucket));
	}

	bucket.gender = DmgProfile::MALE;
	PushElement(testsByBucketCounter.GetCount(CountingBucket(bucket)));
	for(bucket.ageGroup = 0; bucket.ageGroup < ageRanges.size(); ++bucket.ageGroup)
	{
		PushElement(testsByBucketCounter.GetCount(bucket));
	}

	bucket.gender = DmgProfile::ENDGender;
	bucket.sexualActivityStatus = DmgProfile::ENDSexualActivityStatus;
	for(bucket.cd4Stratum = (Person::CD4Strata)0; bucket.cd4Stratum < Person::ENDCD4Strata; ++bucket.cd4Stratum)
	{
		PushElement(testsByBucketCounter.GetCount(bucket));
	}

	

	for(int i = 0; i < SimContext::TEST_RESULT_NUM; ++i)
	{
		PushElement(numTestsByResult[i]);
	}

	bucket.gender = DmgProfile::FEMALE;
	bucket.ageGroup = ageRanges.size();
	PushElement(eligibleByBucketCounter.GetCount(bucket));
	bucket.gender = DmgProfile::MALE;
	PushElement(eligibleByBucketCounter.GetCount(bucket));

	bucket.sexualActivityStatus = DmgProfile::NA;
	PushElement(eligibleByBucketCounter.GetCount(bucket));

	bucket.sexualActivityStatus = DmgProfile::SA;
	bucket.gender = DmgProfile::FEMALE;
	for(bucket.ageGroup = 0; bucket.ageGroup < ageRanges.size(); ++bucket.ageGroup)
	{
		PushElement(eligibleByBucketCounter.GetCount(bucket));
	}

	bucket.gender = DmgProfile::MALE;
	PushElement(testsByBucketCounter.GetCount(CountingBucket(bucket)));
	for(bucket.ageGroup = 0; bucket.ageGroup < ageRanges.size(); ++bucket.ageGroup)
	{
		PushElement(eligibleByBucketCounter.GetCount(bucket));
	}

	bucket.sexualActivityStatus = DmgProfile::ENDSexualActivityStatus;
	bucket.gender = DmgProfile::FEMALE;
	bucket.ageGroup = ageRanges.size();
	PushElement(enrolledByBucketCounter.GetCount(bucket));
	bucket.gender = DmgProfile::MALE;
	PushElement(enrolledByBucketCounter.GetCount(bucket));

	bucket.sexualActivityStatus = DmgProfile::NA;
	PushElement(enrolledByBucketCounter.GetCount(bucket));

	bucket.sexualActivityStatus = DmgProfile::SA;
	bucket.gender = DmgProfile::FEMALE;
	for(bucket.ageGroup = 0; bucket.ageGroup < ageRanges.size(); ++bucket.ageGroup)
	{
		PushElement(enrolledByBucketCounter.GetCount(bucket));
	}

	bucket.gender = DmgProfile::MALE;
	PushElement(testsByBucketCounter.GetCount(CountingBucket(bucket)));
	for(bucket.ageGroup = 0; bucket.ageGroup < ageRanges.size(); ++bucket.ageGroup)
	{
		PushElement(enrolledByBucketCounter.GetCount(bucket));
	}
}

void ArtTestingTracker::Reset()
{
	testsByBucketCounter.Clear();
	eligibleByBucketCounter.Clear();
	enrolledByBucketCounter.Clear();

	for(int resultIndex = 0; resultIndex < SimContext::TEST_RESULT_NUM; ++resultIndex)
	{
		numTestsByResult[resultIndex] = 0;
	}
}
