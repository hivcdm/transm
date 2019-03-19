#include <include.h>

#include "preptracker.hpp"
#include "personbucket.hpp"
#include "core/population.hpp"

namespace transm {

const std::string PrepTracker::RISK_GROUP_NAMES[] =
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
    "CSW High-Risk Male:Msm",
};

const std::string PrepTracker::TRACKED_OUTCOMES[] =
{
    "eligible",
    "accessing",
    "adherent",
    "loss_to_care",
    "return_to_care"
};

const std::string BUCKETS[] =
{
    "sexualActivityStatus",
    "gender",
    "sexualOrientation",
    "relationshipStatus",
    "employment",
    "riskLevel",
    "ageGroup"
};

PrepTracker::PrepTracker() :
    numOnPrEP(0)
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

PrepTracker::~PrepTracker()
{
}

void PrepTracker::SetAgeRanges(const std::vector<AgeRange> &ageRanges)
{
    this->ageRanges = ageRanges;
}

void PrepTracker::recordTreatmentSlots(int numSlots)
{
    numTreatmentSlots = numSlots;
}

void PrepTracker::recordEligiblity(Entity *person)
{
    counter.Increment(PersonBucket(*person, ageRanges), "eligible");
}

void PrepTracker::recordAccess(Entity *person)
{
    counter.Increment(PersonBucket(*person, ageRanges), "accessing");
}

void PrepTracker::recordAdherence(Entity *person)
{
    numOnPrEP++;
    counter.Increment(PersonBucket(*person, ageRanges), "adherent");
}

void PrepTracker::recordLossToCare(Entity *person)
{
    counter.Increment(PersonBucket(*person, ageRanges), "loss_to_care");
}

void PrepTracker::recordReturnToCare(Entity *person)
{
    counter.Increment(PersonBucket(*person, ageRanges), "return_to_care");
}

void PrepTracker::printPrepOutcomes(Time time, std::ostream &_outStream, Population *_population)
{
    if(time == Time::Zero)
    {
        buildHeader();
        PrintHeader(_outStream);
    }

    buildRow(time, _population);
    PrintRow(_outStream);
    Reset();
}

void PrepTracker::buildHeader()
{
    SetHeaderCell(1, 1, "Prep Outcomes");
    SetHeaderCell(1, 3, "Time");
    SetHeaderCell(2, 3, "Population Size");
    SetHeaderCell(3, 3, "Number Treatment Slots");
    SetHeaderCell(4, 3, "PrEP Adherent");

    int column = 5;

    for(auto outcome : TRACKED_OUTCOMES)
    {
        std::string section_header = "";
        if(outcome == "eligible")
        {
            section_header = "Number Eligible for Prep";
        }
        else if(outcome == "accessing")
        {
            section_header = "Number with Access to Prep";
        }
        else if(outcome == "adherent")
        {
            section_header = "Number Adherent";
        }
        else if(outcome == "loss_to_care")
        {
                section_header = "Number Lost";
        }
        else if (outcome == "return_to_care")
        {
                section_header = "Number Return to Care";
        }

        SetHeaderCell(column, 1, section_header);
        SetHeaderCell(column, 2, "Gender");
        SetHeaderCell(column++, 3, "Males");
        SetHeaderCell(column++, 3, "Females");
        SetHeaderCell(column, 2, "Orientation");
        SetHeaderCell(column++, 3, "Males:Msw");
        SetHeaderCell(column++, 3, "Males:Msmw");
        SetHeaderCell(column++, 3, "Males:Msm");

        for(auto gender : {"Males", "Females", "Males:Msw", "Males:Msmw", "Males:Msm", })
        {
            SetHeaderCell(column, 1, gender);
            SetHeaderCell(column, 2, "Non-Sexually Active Population");
            SetHeaderCell(column, 3, "All ages");
            SetHeaderCell(column+1, 2, "Sexually Active Population");
            column++;

            for(const auto &age_range : ageRanges)
            {
                std::stringstream rangeString;
                rangeString << age_range;
                SetHeaderCell(column++, 3, rangeString.str());
            }
        }

        SetHeaderCell(column, 2, "Risk Group");

        std::size_t num_risk_group_names = sizeof(RISK_GROUP_NAMES) / sizeof(RISK_GROUP_NAMES[0]);
        for(std::size_t riskGroupIndex = 0; riskGroupIndex < num_risk_group_names; ++riskGroupIndex, ++column)
        {
            SetHeaderCell(column, 3, RISK_GROUP_NAMES[riskGroupIndex]);
        }
    }
}

void PrepTracker::buildRow(Time time, Population *_population)
{
    if(time == Time::Zero)
    {
        PushElement("init");
    }
    else
    {
        PushElement(time.in_months());
    }

    PushElement(static_cast<int>(_population->GetSize()));
    PushElement(numTreatmentSlots);
    PushElement(numOnPrEP);

    for(auto outcome : TRACKED_OUTCOMES)
    {
        // Add the count by Gender
        for(auto gender : enum_iterator<DemographicProfile::Gender>())
        {
            PushElement(counter.GetCount(outcome, std::make_pair("gender", (int)gender)));
        }

        // Add the count of Males by Orienation
        for(auto orientation : enum_iterator<DemographicProfile::SexualOrientation>())
        {
            PushElement(counter.GetCount(outcome,
                std::make_pair("gender", (int)DemographicProfile::Gender::Male),
                std::make_pair("sexualOrientation", (int)orientation)));
        }

        // Add the count of by gender, sexual activity status and age
        for(auto gender : enum_iterator<DemographicProfile::Gender>())
        {
            PushElement(counter.GetCount(outcome,
                std::make_pair("gender", (int)gender),
                std::make_pair("sexualActivityStatus",
                    (int)DemographicProfile::SexualActivityStatus::NotActive)));

            for(std::size_t ageGroup = 0; ageGroup <ageRanges.size(); ++ageGroup)
            {
                PushElement(counter.GetCount(outcome,
                    std::make_pair("gender", (int)gender),
                    std::make_pair("sexualActivityStatus",
                        (int)DemographicProfile::SexualActivityStatus::Active),
                    std::make_pair("ageGroup", (int)ageGroup)));
            }
        }

        // Add the count of Males by orienation, sexual activity status and age
        for(auto orientation : enum_iterator<DemographicProfile::SexualOrientation>())
        {
            PushElement(counter.GetCount(outcome,
                std::make_pair("gender", (int)DemographicProfile::Gender::Male),
                std::make_pair("sexualOrientation", (int)orientation),
                std::make_pair("sexualActivityStatus", (int)DemographicProfile::SexualActivityStatus::NotActive)));

            for(std::size_t ageGroup = 0; ageGroup < ageRanges.size(); ++ageGroup)
            {
                PushElement(counter.GetCount(outcome,
                    std::make_pair("gender", (int)DemographicProfile::Gender::Male),
                    std::make_pair("sexualOrientation", (int)orientation),
                    std::make_pair("sexualActivityStatus", (int)DemographicProfile::SexualActivityStatus::Active),
                    std::make_pair("ageGroup", (int)ageGroup)));
            }
        }

        // Add the count by gender, employment and risk
        for(auto employment : enum_iterator<DemographicProfile::Employment>())
        {
            for(auto riskLevel : enum_iterator<RiskLevel>())
            {
                for(auto gender : enum_iterator<DemographicProfile::Gender>())
                {
                    // We don't include Low-Risk CSWs
                    if(riskLevel == RiskLevel::LOW &&
                       employment == DemographicProfile::Employment::Csw)
                    {
                        continue;
                    }
                    PushElement(counter.GetCount(outcome,
                        std::make_pair("gender", (int)gender),
                        std::make_pair("employment", (int)employment),
                        std::make_pair("riskLevel", (int)riskLevel)));
                }
            }
        }

        // Add the count of Males by orientation, employment and risk
        for(auto employment : enum_iterator<DemographicProfile::Employment>())
        {
            for(auto riskLevel : enum_iterator<RiskLevel>())
            {
                for(auto orientation : enum_iterator<DemographicProfile::SexualOrientation>())
                {
                    // We don't include Low-Risk CSWs
                    if(riskLevel == RiskLevel::LOW &&
                       employment == DemographicProfile::Employment::Csw)
                    {
                        continue;
                    }
                    PushElement(counter.GetCount(outcome,
                        std::make_pair("gender", (int)DemographicProfile::Gender::Male),
                        std::make_pair("sexualOrientation", (int)orientation),
                        std::make_pair("employment", (int)employment),
                        std::make_pair("riskLevel", (int)riskLevel)));
                }
            }
        }
    }
}

void PrepTracker::Reset()
{
    counter.Reset();

    numTreatmentSlots = 0;
    numOnPrEP = 0;
}

} // namespace transm
