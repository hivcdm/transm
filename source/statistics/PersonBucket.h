#pragma once

#include <utility>
#include <vector>

#include "Bucket.h"
#include "../entities/Person.h"

class PersonBucket : public Bucket
{
public:
	PersonBucket(const Person &person, const std::vector<AgeRange> &ageGroups)
	{
		const DemographicProfile *demographicProfile = person.getDemographicProfile();
        DemographicProfile::SexualActivityStatus sexualActivityStatus = static_cast<DemographicProfile::SexualActivityStatus>
                (demographicProfile->get(DemographicProfile::SEXUAL_ACTIVITY_STATUS));
		values_.push_back(sexualActivityStatus);
        DemographicProfile::Gender gender = static_cast<DemographicProfile::Gender>(demographicProfile->get(DemographicProfile::GENDER));
		values_.push_back(gender);
        DemographicProfile::SexualOrientation sexualOrientation = static_cast<DemographicProfile::SexualOrientation>(demographicProfile->get(
                    DemographicProfile::SEXUAL_ORIENTATION));
		values_.push_back(sexualOrientation);
        DemographicProfile::RelationshipStatus relationshipStatus = static_cast<DemographicProfile::RelationshipStatus>(demographicProfile->get(
                    DemographicProfile::RELATIONSHIP_STATUS));
		values_.push_back(relationshipStatus);
        DemographicProfile::Employment employment = static_cast<DemographicProfile::Employment>(demographicProfile->get(
                                                DemographicProfile::EMPLOYMENT));
		values_.push_back(employment);
        Person::RiskLevel riskLevel = person.getRiskLevel();
		values_.push_back(riskLevel);
        int ageGroup = -1;
        int age = person.getAge(MONTH);

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