#include <include.h>

#include "ArtRolloutTracker.hpp"
#include "PersonBucket.hpp"
#include "core/Population.hpp"
#include "entities/Person.hpp"

const std::string ArtRolloutTracker::RISK_GROUP_NAMES[] =
{
	"Non-CSW Low-Risk Male",
	"Non-CSW Low-Risk Female",
	"Non-CSW High-Risk Male",
	"Non-CSW High-Risk Female",
	"CSW Low-Risk Female",
	"CSW High-Risk Female"
};

const std::string ArtRolloutTracker::TRACKED_OUTCOMES[] =
{
"test_result", "eligible_for_access", "accessing_treatment", "eligible_for_treatment", "treated"
};

const std::string BUCKETS[] =
{
	"sexualActivityStatus",
	"gender",
	"sexualOrientation",
	"relationshipStatus",
	"employment",
	"riskLevel",
	"ageGroup",
	"cd4Stratum"
};

ArtRolloutTracker::ArtRolloutTracker() :
	numTestsOffered(0),
	numTestsAccepted(0),
	numTestsReturnedFor(0),
	numTestsByResult(SimContext::TEST_RESULT_NUM)
{
	std::vector<std::string> tracked;
	for(auto outcome : TRACKED_OUTCOMES)
	{
		tracked.push_back(outcome);
	}
	std::vector<std::string> buckets;
	for(auto bucket : BUCKETS)
	{
		buckets.push_back(bucket);
	}
	counter = BucketCounter(buckets, tracked);
	Reset();
}

ArtRolloutTracker::~ArtRolloutTracker()
{
}

void ArtRolloutTracker::SetAgeRanges(const std::vector<AgeRange> &ageRanges)
{
	this->ageRanges = ageRanges;
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
			counter.Increment(PersonBucket(*person, ageRanges), "test_result");
			numTestsByResult[result]++;
		}
	}
}

void ArtRolloutTracker::recordTreatmentAccessEligiblity(Person *person)
{
	counter.Increment(PersonBucket(*person, ageRanges), "eligible_for_access");
}

void ArtRolloutTracker::recordTreatmentAccess(Person *person)
{
	counter.Increment(PersonBucket(*person, ageRanges), "accessing_treatment");
}

void ArtRolloutTracker::recordTreatmentEligiblity(Person *person)
{
	counter.Increment(PersonBucket(*person, ageRanges), "eligible_for_treatment");
}

void ArtRolloutTracker::recordTreatment(Person *person)
{
	counter.Increment(PersonBucket(*person, ageRanges), "treated");
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
	SetHeaderCell(5, 3, "Returned For Results");

	int column = 6;

	for(auto outcome : TRACKED_OUTCOMES)
	{
		std::string section_header = "";
		if(outcome == "eligible_for_access")
		{
			section_header = "Number Eligible for Access to Treatment";
		}
		else if(outcome == "accessing_treatment")
		{
			section_header = "Number Accessing Treatment";
		}
		else if(outcome == "eligible_for_treatment")
		{
			section_header = "Number Eligible for Treatment";
		}
		else if(outcome == "treated")
		{
			section_header = "Number Treated";
		}

		SetHeaderCell(column, 1, section_header);
		SetHeaderCell(column, 2, "Gender");
		SetHeaderCell(column++, 3, "Males");
		SetHeaderCell(column++, 3, "Females");

		for(auto gender : {"Males", "Females"})
		{
			SetHeaderCell(column, 1, gender);
			SetHeaderCell(column, 2, "Non-Sexually Active Population");
			SetHeaderCell(column, 3, "All ages");
			SetHeaderCell(column + 1, 2, "Sexually Active Population");
			column++;

			for(size_t i = 0; i < ageRanges.size(); ++i, ++column)
			{
				std::stringstream rangeString;
				rangeString << ageRanges[i].lower << "-" << ageRanges[i].upper;
				SetHeaderCell(column + 1 + (int)i, 3, rangeString.str());
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

		if(outcome == "test_result")
		{
			SetHeaderCell(column, 2, "Test Result");

			for(int testResultIndex = 0; testResultIndex < SimContext::TEST_RESULT_NUM; ++testResultIndex, ++column)
			{
				SetHeaderCell(column, 3, SimContext::TEST_RESULT_STRS[testResultIndex]);
			}
		}
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

	PushElement(static_cast<int>(_population->GetSize()));
	PushElement(numTestsOffered);
	PushElement(numTestsAccepted);
	PushElement(numTestsReturnedFor);

	for(auto outcome : TRACKED_OUTCOMES)
	{
		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
            PushElement(counter.GetCount(outcome, std::make_pair("gender", (int)gender)));
		}

        for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
            PushElement(counter.GetCount(outcome, std::make_pair("gender", (int)gender), std::make_pair("sexualActivityStatus", (int)DemographicProfile::SexualActivityStatus::NotActive)));

			for(std::size_t ageGroup = 0; ageGroup < ageRanges.size(); ++ageGroup)
			{
                PushElement(counter.GetCount(outcome, std::make_pair("gender", (int)gender), std::make_pair("sexualActivityStatus", (int)DemographicProfile::SexualActivityStatus::Active), std::make_pair("ageGroup", (int)ageGroup)));
			}
		}

		for(Person::CD4Strata cd4Stratum = static_cast<Person::CD4Strata>(0); cd4Stratum < Person::ENDCD4Strata; ++cd4Stratum)
		{
			PushElement(counter.GetCount(outcome, std::make_pair("cd4Stratum", cd4Stratum)));
		}

		for(auto employment : enum_iterator<DemographicProfile::Employment>())
		{
			for(Person::RiskLevel riskLevel = static_cast<Person::RiskLevel>(0); riskLevel < Person::ENDRiskLevel; ++riskLevel)
			{
                for(auto gender : enum_iterator<DemographicProfile::Gender>())
				{
					// We don't include Male CSWs for now
					if(gender == DemographicProfile::Gender::Male && employment == DemographicProfile::Employment::Csw)
					{
						continue;
					}

                    PushElement(counter.GetCount(outcome, std::make_pair("gender", (int)gender), std::make_pair("employment", (int)employment), std::make_pair("riskLevel", (int)riskLevel)));
				}
			}
		}

		if(outcome == "test_result")
		{
			for(int i = 0; i < SimContext::TEST_RESULT_NUM; ++i)
			{
				PushElement(numTestsByResult[i]);
			}
		}
	}
}

void ArtRolloutTracker::Reset()
{
	counter.Reset();

	for(int resultIndex = 0; resultIndex < SimContext::TEST_RESULT_NUM; ++resultIndex)
	{
		numTestsByResult[resultIndex] = 0;
	}

	numTestsOffered = 0;
	numTestsAccepted = 0;
	numTestsReturnedFor = 0;
}
