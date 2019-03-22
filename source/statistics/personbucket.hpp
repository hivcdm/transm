#pragma once

#include <utility>
#include <vector>

#include "bucket.hpp"
#include "entities/demographicprofile.hpp"
#include "entities/entity.hpp"

namespace transm {

class PersonBucket : public Bucket
{
public:
    PersonBucket(const Entity &person, const std::vector<AgeRange> &ageGroups)
    {
        const DemographicProfile *demographicProfile = person.getDemographicProfile();

        for (auto demographic : enum_iterator<DemographicProfile::Demographic>())
        {
            std::string demoString = DemographicStrs[(std::size_t)demographic];
            std::size_t demoValue = (std::size_t)demographicProfile->get(demographic);
            values_.emplace(demoString, demoValue);
        }

        RiskLevel riskLevel = person.getRiskLevel();
        values_.emplace("RISK_LEVEL", (std::size_t)riskLevel);

        CD4Strata cd4Stratum = person.getCd4Stratum();
        values_.emplace("CD4_STRATUM", (std::size_t)cd4Stratum);

        int ageGroup = -1;
        auto age = person.getAge();
        for(size_t i = 0; i < ageGroups.size(); ++i)
        {
            if(age >= ageGroups[i].lower && age <= ageGroups[i].upper)
            {
                ageGroup = (int)i;
            }
        }
        assert(ageGroup != -1);
        values_.emplace("AGE_GROUP", ageGroup);
    }
};

} // namespace transm
