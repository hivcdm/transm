#pragma once

#include <utility>
#include <vector>

#include "bucket.hpp"
#include "entities/person.hpp"

class PersonBucket : public Bucket
{
public:
	PersonBucket(const Person &person, const std::vector<AgeRange> &ageGroups)
	{
		const DemographicProfile *demographicProfile = person.getDemographicProfile();
        DemographicProfile::SexualActivityStatus sexualActivityStatus = static_cast<DemographicProfile::SexualActivityStatus>
                (demographicProfile->get(DemographicProfile::Demographic::SexualActivityStatus));
        values_.push_back((std::size_t)sexualActivityStatus);
        DemographicProfile::Gender gender = static_cast<DemographicProfile::Gender>(demographicProfile->get(DemographicProfile::Demographic::Gender));
        values_.push_back((std::size_t)gender);
        DemographicProfile::SexualOrientation sexualOrientation = static_cast<DemographicProfile::SexualOrientation>(demographicProfile->get(
                    DemographicProfile::Demographic::SexualOrientation));
        values_.push_back((std::size_t)sexualOrientation);
        DemographicProfile::RelationshipStatus relationshipStatus = static_cast<DemographicProfile::RelationshipStatus>(demographicProfile->get(
                    DemographicProfile::Demographic::RelationshipStatus));
        values_.push_back((std::size_t)relationshipStatus);
        DemographicProfile::Employment employment = static_cast<DemographicProfile::Employment>(demographicProfile->get(
                                                DemographicProfile::Demographic::Employment));
        values_.push_back((std::size_t)employment);
        Person::RiskLevel riskLevel = person.getRiskLevel();
		values_.push_back(riskLevel);
        int ageGroup = -1;
        int age = person.getAge(TimeGranularity::Month);

        for(size_t i = 0; i < ageGroups.size(); ++i)
        {
                if(age >= ageGroups[i].lower && age <= ageGroups[i].upper)
                {
                        ageGroup = (int)i;
                }
        }

        assert(ageGroup != -1);
		values_.push_back(ageGroup);
        Person::CD4Strata cd4Stratum = person.getCd4Stratum();
		values_.push_back(cd4Stratum);
	}
};
