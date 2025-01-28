#include <include.h>

#include "artrollouttracker.hpp"
#include "personbucket.hpp"
#include "core/population.hpp"

namespace transm {

const std::string ArtRolloutTracker::RISK_GROUP_NAMES[] =
        {
                "Non-CSW Low-Risk Male",
                "Non-CSW High-Risk Male",
                "CSW High-Risk Male",
                "Non-CSW Low-Risk Female",
                "Non-CSW High-Risk Female",
                "CSW High-Risk Female",
                "Non-CSW Low-Risk Male:Msw",
                "Non-CSW Low-Risk Male:Msmw",
                "Non-CSW Low-Risk Male:Msm",
                "Non-CSW High-Risk Male:Msw",
                "Non-CSW High-Risk Male:Msmw",
                "Non-CSW High-Risk Male:Msm",
                "CSW High-Risk Male:Msw",
                "CSW High-Risk Male:Msmw",
                "CSW High-Risk Male:Msm"
        };

/* TODO: This cant's take more than 9 quantity! */
const std::string ArtRolloutTracker::TRACKED_OUTCOMES[] =
        {
                "infected",
                "plwh", /* people living with HIV */
                "undiagnosed", 
                "in_care",
                "new_diagnosis",
                "enrolled_within_30",
                // "suppressed_vl",
                "unlinked",
                "treated", /* people on ART */
                "focus",
                // "return_to_care",
                "ltfu"
                // "firstlineART",
                // "secondlineART"
        };

const std::string ArtRolloutTracker::BUCKETS[] =
        {
                "SEXUAL_ACTIVITY_STATUS",
                "GENDER",
                "RACE",
                "ETHNICITY",
                "SEXUAL_ORIENTATION",
                "RELATIONSHIP_STATUS",
                "EMPLOYMENT",
                "RISK_LEVEL",
                "AGE_GROUP",
                "CD4_STRATUM"
        };

ArtRolloutTracker::ArtRolloutTracker() :
        numTestsOffered(0),
        numTestsAccepted(0),
        numTestsReturnedFor(0),
        numTestsByResult(SimContext::TEST_RESULT_NUM) 
{
    std::vector<std::string> tracked;
    for (const auto &outcome : TRACKED_OUTCOMES) {
        tracked.push_back(outcome);
    }
    std::vector<std::string> buckets;
    for (const auto &bucket : BUCKETS) {
        buckets.push_back(bucket);
    }
    counter = BucketCounter(buckets, tracked);
    Reset();
}

ArtRolloutTracker::~ArtRolloutTracker() = default;

void ArtRolloutTracker::SetAgeRanges(const std::vector<AgeRange> &ageRanges) {
    this->ageRanges = ageRanges;
}

void ArtRolloutTracker::recordTest(Entity *person, bool accepted, bool returned, SimContext::TEST_RESULT result) {
    numTestsOffered++;

    if (accepted) {
        numTestsAccepted++;

        if (returned) {
            numTestsReturnedFor++;
            counter.Increment(PersonBucket(*person, ageRanges), "test_result");
            numTestsByResult[result]++;
        }
    }
}

void ArtRolloutTracker::recordUndiagnosed(Entity *person) {
    counter.Increment(PersonBucket(*person, ageRanges), "undiagnosed");
}

void ArtRolloutTracker::recordTreatmentAccessEligibility(Entity *person) {
    counter.Increment(PersonBucket(*person, ageRanges), "eligible_for_access");
}

void ArtRolloutTracker::recordTreatmentAccess(Entity *person) {
    counter.Increment(PersonBucket(*person, ageRanges), "accessing_treatment");
}

void ArtRolloutTracker::recordTreatmentEligibility(Entity *person) {
    counter.Increment(PersonBucket(*person, ageRanges), "eligible_for_treatment");
}

void ArtRolloutTracker::recordTreatment(Entity *person) {
    counter.Increment(PersonBucket(*person, ageRanges), "treated");
}

void ArtRolloutTracker::recordTreatmentDeath(Entity *person) {
    counter.Increment(PersonBucket(*person, ageRanges), "death_on_treatment");
}

void ArtRolloutTracker::recordTreatmentSlots(int numSlots) {
    numTreatmentSlots = numSlots;
}

void ArtRolloutTracker::printArtRolloutOutcomes(Time time, std::ostream &_outStream, Population *_population) {
    if (time == Time::Zero) {
        buildHeader();
        PrintHeader(_outStream);
    }

    buildRow(time, _population);
    PrintRow(_outStream);
    Reset();
}

void ArtRolloutTracker::buildHeader() {
    SetHeaderCell(1, 1, "ART Rollout Outcomes");
    SetHeaderCell(1, 3, "Time");
    SetHeaderCell(2, 3, "Population Size");
    SetHeaderCell(3, 3, "Number Treatment Slots");
    SetHeaderCell(4, 1, "Number of Tests");
    SetHeaderCell(4, 2, "Totals");
    SetHeaderCell(4, 3, "Offered");
    SetHeaderCell(5, 3, "Accepted");
    SetHeaderCell(6, 3, "Returned For Results");

    int column = 7;

    for (const auto &outcome : TRACKED_OUTCOMES) {
        std::string section_header;
        if (outcome == "eligible_for_access") {
            section_header = "Number Eligible for Access to Treatment";
        } else if (outcome == "accessing_treatment") {
            section_header = "Number Accessing Treatment";
        } else if (outcome == "eligible_for_treatment") {
            section_header = "Number Eligible for Treatment";
        } else if (outcome == "treated") {
            section_header = "On ART"; //previoulu "In Treatment"
        } else if (outcome == "death_on_treatment") {
            section_header = "Deaths on Treatment";
        } else if (outcome == "num_treatment_slots") {
            section_header = "Number Treatment Slots";
        } else if (outcome == "in_care") {
            section_header = "Number in Care";
        } else if (outcome == "suppressed_vl") {
            section_header = "Suppressed VL";
        } else if (outcome == "enrolled_within_30") {
            section_header = "Enrolled Within 30 days";
        } else if (outcome == "plwh") {
            section_header = "Number Detected";
        } else if (outcome == "new_diagnosis") {
            section_header = "New Diagnosis";
        } else if (outcome == "ltfu") {
            section_header = "Loss To Follow Up";
        } else if (outcome == "unlinked") {
            section_header = "Number of Unlinked";
        } else if (outcome == "infected") {
            section_header = "Number Infected";
        } else if (outcome == "firstlineART") {
            section_header = "First Line ART";
        } else if (outcome == "secondlineART") {
            section_header = "Second Line ART";
        } else if (outcome == "return_to_care") {
            section_header = "Return To Care";
        } else if (outcome == "undiagnosed") {
            section_header = "Undiagnosed";
        } else if (outcome == "focus") {
            section_header = "Selected for FOCUS Study";
        }


        SetHeaderCell(column, 1, section_header);
        SetHeaderCell(column, 2, "Gender");
        SetHeaderCell(column++, 3, "Males");
        SetHeaderCell(column++, 3, "Females");
        SetHeaderCell(column, 2, "Orientation");
        SetHeaderCell(column++, 3, "Males:Msw");
        SetHeaderCell(column++, 3, "Males:Msmw");
        SetHeaderCell(column++, 3, "Males:Msm");
        SetHeaderCell(column, 2, "Race/Ethnicity Group");
        for (auto race : enum_iterator<DemographicProfile::Race>()) {
            if ((std::size_t)race >= DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::Race).size()) {
                throw std::runtime_error("Invalid Race index in DemographicEnumStrs.");
            }

            std::string raceStr = DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::Race).at((std::size_t)race);

            for (auto ethnicity : enum_iterator<DemographicProfile::Ethnicity>()) {
                if ((std::size_t)ethnicity >= DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::Ethnicity).size()) {
                    throw std::runtime_error("Invalid Ethnicity index in DemographicEnumStrs.");
                }

                std::string ethStr = DemographicEnumStrs.at((std::size_t)DemographicProfile::Demographic::Ethnicity).at((std::size_t)ethnicity);

                std::string label = raceStr + ":" + ethStr;
                SetHeaderCell(column++, 3, label);
            }
        }

//            for(auto gender : {"Males", "Females", "Males:Msw", "Males:Msmw", "Males:Msm", })
//            {
//                SetHeaderCell(column, 1, gender);
//                SetHeaderCell(column, 2, "Non-Sexually Active Population");
//                SetHeaderCell(column, 3, "All ages");
//                SetHeaderCell(column+1, 2, "Sexually Active Population");
//                column++;
//
//                for(const auto &age_range : ageRanges)
//                {
//                    std::stringstream rangeString;
//                    rangeString << age_range;
//                    SetHeaderCell(column++, 3, rangeString.str());
//                }
//            }

//            SetHeaderCell(column, 2, "CD4 Stratum");
//
//            for(auto cd4stratum : enum_iterator<CD4Strata>())
//            {
//                SetHeaderCell(column++, 3, SimContext::CD4_STRATA_STRS[(std::size_t)cd4stratum]);
//            }

//            SetHeaderCell(column, 2, "Risk Group");
//
//            std::size_t num_risk_group_names = sizeof(RISK_GROUP_NAMES) / sizeof(RISK_GROUP_NAMES[0]);
//            for(std::size_t riskGroupIndex = 0; riskGroupIndex < num_risk_group_names; ++riskGroupIndex, ++column)
//            {
//                SetHeaderCell(column, 3, RISK_GROUP_NAMES[riskGroupIndex]);
//            }

//            if(outcome == "test_result")
//            {
//                SetHeaderCell(column, 2, "Test Result");
//
//                for(int testResultIndex = 0; testResultIndex < SimContext::TEST_RESULT_NUM; ++testResultIndex, ++column)
//                {
//                    SetHeaderCell(column, 3, SimContext::TEST_RESULT_STRS[testResultIndex]);
//                }
//            }
    }
}

void ArtRolloutTracker::buildRow(Time time, Population *_population) {
    if (time == Time::Zero) {
        PushElement("init");
    } else {
        PushElement(time.in_months());
    }

    PushElement(static_cast<int>(_population->GetSize()));
    PushElement(numTreatmentSlots);
    PushElement(numTestsOffered);
    PushElement(numTestsAccepted);
    PushElement(numTestsReturnedFor);

    for (const auto &outcome : TRACKED_OUTCOMES) {
        /* Add the count by Gender */
        for (auto gender : enum_iterator<DemographicProfile::Gender>()) {
            PushElement(counter.GetCount(outcome, std::make_pair("GENDER", (int) gender)));
        }

        /* Add the count of Males by Orienation */
        for (auto orientation : enum_iterator<DemographicProfile::SexualOrientation>()) {
            PushElement(counter.GetCount(outcome,
                                         std::make_pair("GENDER", (int) DemographicProfile::Gender::Male),
                                         std::make_pair("SEXUAL_ORIENTATION", (int) orientation)));
        }
        /* Add the count by gender, race */
//            for(auto gender : enum_iterator<DemographicProfile::Gender>())
//            {
        for (auto race : enum_iterator<DemographicProfile::Race>()) {
            for (auto ethnicity : enum_iterator<DemographicProfile::Ethnicity>()) {
                PushElement(counter.GetCount(outcome,
                                             std::make_pair("RACE", (int) race),
                                             std::make_pair("ETHNICITY", (int) ethnicity)));
            }
        }
//            }
        /* Add the count of by gender, sexual activity status and age */
//            for(auto gender : enum_iterator<DemographicProfile::Gender>())
//            {
//                PushElement(counter.GetCount(outcome,
//                                             std::make_pair("GENDER", (int)gender),
//                                             std::make_pair("SEXUAL_ACTIVITY_STATUS",
//                                                            (int)DemographicProfile::SexualActivityStatus::NotActive)));
//
//                for(std::size_t ageGroup = 0; ageGroup <ageRanges.size(); ++ageGroup)
//                {
//                    PushElement(counter.GetCount(outcome,
//                                                 std::make_pair("GENDER", (int)gender),
//                                                 std::make_pair("SEXUAL_ACTIVITY_STATUS",
//                                                                (int)DemographicProfile::SexualActivityStatus::Active),
//                                                 std::make_pair("AGE_GROUP", (int)ageGroup)));
//                }
//            }

        /* Add the count of Males by orienation, sexual activity status and age */
//            for(auto orientation : enum_iterator<DemographicProfile::SexualOrientation>())
//            {
//                PushElement(counter.GetCount(outcome,
//                                             std::make_pair("GENDER", (int)DemographicProfile::Gender::Male),
//                                             std::make_pair("SEXUAL_ORIENTATION", (int)orientation),
//                                             std::make_pair("SEXUAL_ACTIVITY_STATUS", (int)DemographicProfile::SexualActivityStatus::NotActive)));
//
//                for(std::size_t ageGroup = 0; ageGroup < ageRanges.size(); ++ageGroup)
//                {
//                    PushElement(counter.GetCount(outcome,
//                                                 std::make_pair("GENDER", (int)DemographicProfile::Gender::Male),
//                                                 std::make_pair("SEXUAL_ORIENTATION", (int)orientation),
//                                                 std::make_pair("SEXUAL_ACTIVITY_STATUS", (int)DemographicProfile::SexualActivityStatus::Active),
//                                                 std::make_pair("AGE_GROUP", (int)ageGroup)));
//                }
//            }

//            for(std::size_t i = 0; i < (std::size_t)CD4Strata::Last; ++i)
//            {
//                PushElement(counter.GetCount(outcome, std::make_pair("CD4_STRATUM", (int)i)));
//            }

        /* Add the count by gender, employment and risk */
//            for(auto employment : enum_iterator<DemographicProfile::Employment>())
//            {
//                for(auto riskLevel : enum_iterator<RiskLevel>())
//                {
//                    for(auto gender : enum_iterator<DemographicProfile::Gender>())
//                    {
//                        // We don't include Low-Risk CSWs
//                        if(riskLevel == RiskLevel::LOW &&
//                           employment == DemographicProfile::Employment::Csw)
//                        {
//                            continue;
//                        }
//                        PushElement(counter.GetCount(outcome,
//                                                     std::make_pair("GENDER", (int)gender),
//                                                     std::make_pair("EMPLOYMENT", (int)employment),
//                                                     std::make_pair("RISK_LEVEL", (int)riskLevel)));
//                    }
//                }
//            }

        /* Add the count of Males by orientation, employment and risk */
//            for(auto employment : enum_iterator<DemographicProfile::Employment>())
//            {
//                for(auto riskLevel : enum_iterator<RiskLevel>())
//                {
//                    for(auto orientation : enum_iterator<DemographicProfile::SexualOrientation>())
//                    {
//                        // We don't include Low-Risk CSWs
//                        if(riskLevel == RiskLevel::LOW &&
//                           employment == DemographicProfile::Employment::Csw)
//                        {
//                            continue;
//                        }
//                        PushElement(counter.GetCount(outcome,
//                                                     std::make_pair("GENDER", (int)DemographicProfile::Gender::Male),
//                                                     std::make_pair("SEXUAL_ORIENTATION", (int)orientation),
//                                                     std::make_pair("EMPLOYMENT", (int)employment),
//                                                     std::make_pair("RISK_LEVEL", (int)riskLevel)));
//                    }
//                }
//            }


        // if (outcome == "test_result") {
        //     for (int i = 0; i < SimContext::TEST_RESULT_NUM; ++i) {
        //         PushElement(numTestsByResult[i]);
        //     }
        // }
    }
}

void ArtRolloutTracker::Reset() {
    counter.Reset();

    for (int resultIndex = 0; resultIndex < SimContext::TEST_RESULT_NUM; ++resultIndex) {
        numTestsByResult[resultIndex] = 0;
    }

    numTestsOffered = 0;
    numTestsAccepted = 0;
    numTestsReturnedFor = 0;
    numTreatmentSlots = 0;
}

void ArtRolloutTracker::recordSuppressedVL(Entity *person) {
    counter.Increment(PersonBucket(*person, ageRanges), "suppressed_vl");
}

void ArtRolloutTracker::recordEnrolledInThirtyDays(Entity *person) {
    counter.Increment(PersonBucket(*person, ageRanges), "enrolled_within_30");
}

/* Counts number of people living with HIV+ */
void ArtRolloutTracker::recordNewDiagnosis(Entity *person) {
    counter.Increment(PersonBucket(*person, ageRanges), "new_diagnosis");
}

/* Counts number of people on HIV_CARE_IN_CARE */
void ArtRolloutTracker::recordInCare(Entity *person) {
    counter.Increment(PersonBucket(*person, ageRanges), "in_care");

}

/* Counts number of people living with HIV+ */
void ArtRolloutTracker::recordPLWH(Entity *person) {
    counter.Increment(PersonBucket(*person, ageRanges), "plwh");
}

/* Counts number of people living with HIV+ */
void ArtRolloutTracker::recordLTFU(Entity *person) {
    counter.Increment(PersonBucket(*person, ageRanges), "ltfu");
}

/* Counts number of people living with HIV+ */
void ArtRolloutTracker::recordUnlinked(Entity *person) {
    counter.Increment(PersonBucket(*person, ageRanges), "unlinked");
}

/* Counts number of people infected */
void ArtRolloutTracker::recordInfected(Entity *person) {
    counter.Increment(PersonBucket(*person, ageRanges), "infected");
}

/* Counts number of people on second line of ART */
    void ArtRolloutTracker::recordRTC(Entity *person) {
        counter.Increment(PersonBucket(*person, ageRanges), "return_to_care");
    }

/* Counts number of people on first line of ART */
void ArtRolloutTracker::recordFirstLine(Entity *person) {
    counter.Increment(PersonBucket(*person, ageRanges), "firstlineART");
}

/* Counts number of people on second line of ART */
    void ArtRolloutTracker::recordSecondLine(Entity *person) {
        counter.Increment(PersonBucket(*person, ageRanges), "secondlineART");
    }

void ArtRolloutTracker::recordFOCUS(Entity *person) {
    counter.Increment(PersonBucket(*person, ageRanges), "focus");
}



// namespace transm
}