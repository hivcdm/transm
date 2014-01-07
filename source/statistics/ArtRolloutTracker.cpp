#include "ArtRolloutTracker.h"
#include "../Population.h"
#include "../entities/Person.h"
#include "../cepac/SimContext.h"

const std::string ArtRolloutTracker::RISK_GROUP_NAMES[] = 
{
	"Non-CSW Low-Risk Male",
	"Non-CSW Low-Risk Female",
	"Non-CSW High-Risk Male",
	"Non-CSW High-Risk Female",
	"CSW Low-Risk Female",
	"CSW High-Risk Female"
};

ArtRolloutTracker::ArtRolloutTracker() :
    numTestsOffered(0),
    numTestsAccepted(0),
    numTestsReturnedFor(0),
	numTestsByResult(SimContext::TEST_RESULT_NUM)
{
	Reset();
}

ArtRolloutTracker::~ArtRolloutTracker()
{

}

void ArtRolloutTracker::SetAgeRanges(const std::vector<boost::tuple<long, int, int> > &ageRangeSizes)
{
	ageRanges.clear();

	int numAgeRanges = static_cast<int>(ageRangeSizes.size());

	for(int i = 0; i < numAgeRanges; ++i)
	{
		int minAge = boost::tuples::get<Population::MIN_AGE_IN_MONTHS>(ageRangeSizes.at(i));
		int maxAge = boost::tuples::get<Population::MAX_AGE_IN_MONTHS>(ageRangeSizes.at(i));
		ageRanges.push_back(std::make_pair(minAge, maxAge));
	}

	testsByBucketCounter.SetAgeRanges(ageRanges);
	eligibleByBucketCounter.SetAgeRanges(ageRanges);
	treatedByBucketCounter.SetAgeRanges(ageRanges);
}

void ArtRolloutTracker::recordTest(Person *person, bool accepted, bool returned, SimContext::TEST_RESULT result)
{
	numTestsOffered++;

	if(accepted)
	{
		numTestsAccepted++;

		if(returned)
		{
			numTestsReturnedFor++;
			testsByBucketCounter.Increment(person);
			numTestsByResult[result]++;
		}
	}
}

void ArtRolloutTracker::recordEligiblePerson(Person *person)
{
	eligibleByBucketCounter.Increment(person);
}

void ArtRolloutTracker::recordTreatment(Person *person)
{
	treatedByBucketCounter.Increment(person);
}

void ArtRolloutTracker::printArtRolloutOutcomes(int time, std::ostream &_outStream, Population *_population)
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

void ArtRolloutTracker::buildHeader()
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
	SetHeaderCell(6, 3, "Males");
	SetHeaderCell(7, 3, "Females");

	int column = 8;
	for(int genderIndex = 0; genderIndex < 2; genderIndex++, column += (ageRanges.size() + 1))
	{
		SetHeaderCell(column, 1, genderIndex == 0 ? "Males" : "Females");

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

	SetHeaderCell(column, 2, "CD4 Stratum");

	for(int cd4StratumIndex = 0; cd4StratumIndex < Person::ENDCD4Strata; ++cd4StratumIndex, ++column)
	{
		SetHeaderCell(column, 3, SimContext::CD4_STRATA_STRS[cd4StratumIndex]);
	}

	SetHeaderCell(column, 2, "Risk Group");

	for(int riskGroupIndex = 0; riskGroupIndex < 6; ++riskGroupIndex, ++column)
	{
		SetHeaderCell(column, 3, RISK_GROUP_NAMES[riskGroupIndex]);
	}

	SetHeaderCell(column, 2, "Test Result");

	for(int testResultIndex = 0; testResultIndex < SimContext::TEST_RESULT_NUM; ++testResultIndex, ++column)
	{
		SetHeaderCell(column, 3, SimContext::TEST_RESULT_STRS[testResultIndex]);
	}

	SetHeaderCell(column, 1, "Number Eligible for Treatment");

	SetHeaderCell(column, 2, "Gender");
	SetHeaderCell(column, 3, "Males");
	SetHeaderCell(column + 1, 3, "Females");

	column += 2;

	for(int genderIndex = 0; genderIndex < 2; genderIndex++, column += (ageRanges.size() + 1))
	{
		SetHeaderCell(column, 1, genderIndex == 0 ? "Males" : "Females");

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

	SetHeaderCell(column, 2, "CD4 Stratum");

	for(int cd4StratumIndex = 0; cd4StratumIndex < Person::ENDCD4Strata; ++cd4StratumIndex, ++column)
	{
		SetHeaderCell(column, 3, SimContext::CD4_STRATA_STRS[cd4StratumIndex]);
	}

	SetHeaderCell(column, 2, "Risk Group");

	for(int riskGroupIndex = 0; riskGroupIndex < 6; ++riskGroupIndex, ++column)
	{
		SetHeaderCell(column, 3, RISK_GROUP_NAMES[riskGroupIndex]);
	}

	SetHeaderCell(column, 1, "Number Enrolled in ART");

	SetHeaderCell(column, 2, "Gender");
	SetHeaderCell(column, 3, "Males");
	SetHeaderCell(column + 1, 3, "Females");

	column += 2;

	for(int genderIndex = 0; genderIndex < 2; genderIndex++, column += (ageRanges.size() + 1))
	{
		SetHeaderCell(column, 1, genderIndex == 0 ? "Males" : "Females");

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

	SetHeaderCell(column, 2, "CD4 Stratum");

	for(int cd4StratumIndex = 0; cd4StratumIndex < Person::ENDCD4Strata; ++cd4StratumIndex, ++column)
	{
		SetHeaderCell(column, 3, SimContext::CD4_STRATA_STRS[cd4StratumIndex]);
	}

	SetHeaderCell(column, 2, "Risk Group");

	for(int riskGroupIndex = 0; riskGroupIndex < 6; ++riskGroupIndex, ++column)
	{
		SetHeaderCell(column, 3, RISK_GROUP_NAMES[riskGroupIndex]);
	}
}

void ArtRolloutTracker::buildRow(int time, Population *_population)
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

	for(DmgProfile::Gender gender = static_cast<DmgProfile::Gender>(0); gender < DmgProfile::ENDGender; ++gender)
	{
		PushElement(testsByBucketCounter.GetCountByGender(gender));
	}

	for(DmgProfile::Gender gender = static_cast<DmgProfile::Gender>(0); gender < DmgProfile::ENDGender; ++gender)
	{
		PushElement(testsByBucketCounter.GetCountByGenderSexualActivity(gender, DmgProfile::NA));

		for(int ageGroup = 0; ageGroup < ageRanges.size(); ++ageGroup)
		{
			PushElement(testsByBucketCounter.GetCountByGenderSexualActivityAge(gender, DmgProfile::SA, ageGroup));
		}
	}

	for(Person::CD4Strata cd4Stratum = static_cast<Person::CD4Strata>(0); cd4Stratum < Person::ENDCD4Strata; ++cd4Stratum)
	{
		PushElement(testsByBucketCounter.GetCountByCd4(cd4Stratum));
	}

	for(DmgProfile::Employment employment = static_cast<DmgProfile::Employment>(0); employment < DmgProfile::ENDEmployment; ++employment)
	{
		for(Person::RiskLevel riskLevel = static_cast<Person::RiskLevel>(0); riskLevel < Person::ENDRiskLevel; ++riskLevel)
		{
			for(DmgProfile::Gender gender = static_cast<DmgProfile::Gender>(0); gender < DmgProfile::ENDGender; ++gender)
			{
				// We don't include Male CSWs for now
				if(gender == DmgProfile::MALE && employment == DmgProfile::CSW)
				{
					continue;
				}

				PushElement(testsByBucketCounter.GetCountByRiskGroup(riskLevel, gender, employment));
			}
		}
	}

	for(int i = 0; i < SimContext::TEST_RESULT_NUM; ++i)
	{
		PushElement(numTestsByResult[i]);
	}

	for(DmgProfile::Gender gender = static_cast<DmgProfile::Gender>(0); gender < DmgProfile::ENDGender; ++gender)
	{
		PushElement(eligibleByBucketCounter.GetCountByGender(gender));
	}

	for(DmgProfile::Gender gender = static_cast<DmgProfile::Gender>(0); gender < DmgProfile::ENDGender; ++gender)
	{
		PushElement(eligibleByBucketCounter.GetCountByGenderSexualActivity(gender, DmgProfile::NA));

		for(int ageGroup = 0; ageGroup < ageRanges.size(); ++ageGroup)
		{
			PushElement(eligibleByBucketCounter.GetCountByGenderSexualActivityAge(gender, DmgProfile::SA, ageGroup));
		}
	}

	for(Person::CD4Strata cd4Stratum = static_cast<Person::CD4Strata>(0); cd4Stratum < Person::ENDCD4Strata; ++cd4Stratum)
	{
		PushElement(eligibleByBucketCounter.GetCountByCd4(cd4Stratum));
	}

	for(DmgProfile::Employment employment = static_cast<DmgProfile::Employment>(0); employment < DmgProfile::ENDEmployment; ++employment)
	{
		for(Person::RiskLevel riskLevel = static_cast<Person::RiskLevel>(0); riskLevel < Person::ENDRiskLevel; ++riskLevel)
		{
			for(DmgProfile::Gender gender = static_cast<DmgProfile::Gender>(0); gender < DmgProfile::ENDGender; ++gender)
			{
				// We don't include Male CSWs for now
				if(gender == DmgProfile::MALE && employment == DmgProfile::CSW)
				{
					continue;
				}

				PushElement(eligibleByBucketCounter.GetCountByRiskGroup(riskLevel, gender, employment));
			}
		}
	}

	for(DmgProfile::Gender gender = static_cast<DmgProfile::Gender>(0); gender < DmgProfile::ENDGender; ++gender)
	{
		PushElement(treatedByBucketCounter.GetCountByGender(gender));
	}

	for(DmgProfile::Gender gender = static_cast<DmgProfile::Gender>(0); gender < DmgProfile::ENDGender; ++gender)
	{
		PushElement(treatedByBucketCounter.GetCountByGenderSexualActivity(gender, DmgProfile::NA));

		for(int ageGroup = 0; ageGroup < ageRanges.size(); ++ageGroup)
		{
			PushElement(treatedByBucketCounter.GetCountByGenderSexualActivityAge(gender, DmgProfile::SA, ageGroup));
		}
	}

	for(Person::CD4Strata cd4Stratum = static_cast<Person::CD4Strata>(0); cd4Stratum < Person::ENDCD4Strata; ++cd4Stratum)
	{
		PushElement(treatedByBucketCounter.GetCountByCd4(cd4Stratum));
	}

	for(DmgProfile::Employment employment = static_cast<DmgProfile::Employment>(0); employment < DmgProfile::ENDEmployment; ++employment)
	{
		for(Person::RiskLevel riskLevel = static_cast<Person::RiskLevel>(0); riskLevel < Person::ENDRiskLevel; ++riskLevel)
		{
			for(DmgProfile::Gender gender = static_cast<DmgProfile::Gender>(0); gender < DmgProfile::ENDGender; ++gender)
			{
				// We don't include Male CSWs for now
				if(gender == DmgProfile::MALE && employment == DmgProfile::CSW)
				{
					continue;
				}

				PushElement(treatedByBucketCounter.GetCountByRiskGroup(riskLevel, gender, employment));
			}
		}
	}
}

void ArtRolloutTracker::Reset()
{
	testsByBucketCounter.Reset();
	eligibleByBucketCounter.Reset();
	treatedByBucketCounter.Reset();

	for(int resultIndex = 0; resultIndex < SimContext::TEST_RESULT_NUM; ++resultIndex)
	{
		numTestsByResult[resultIndex] = 0;
	}

	numTestsOffered = 0;
	numTestsAccepted = 0;
	numTestsReturnedFor = 0;
}
