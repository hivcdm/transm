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
		const DmgProfile *demographicProfile = person.getDmgProfile();
        DmgProfile::SexualActivityStatus sexualActivityStatus = static_cast<DmgProfile::SexualActivityStatus>
                (demographicProfile->get(DmgProfile::SEXUAL_ACTIVITY_STATUS));
		values_.push_back(sexualActivityStatus);
        DmgProfile::Gender gender = static_cast<DmgProfile::Gender>(demographicProfile->get(DmgProfile::GENDER));
		values_.push_back(gender);
        DmgProfile::SexualOrientation sexualOrientation = static_cast<DmgProfile::SexualOrientation>(demographicProfile->get(
                    DmgProfile::SEXUAL_ORIENTATION));
		values_.push_back(sexualOrientation);
        DmgProfile::RelationshipStatus relationshipStatus = static_cast<DmgProfile::RelationshipStatus>(demographicProfile->get(
                    DmgProfile::RELATIONSHIP_STATUS));
		values_.push_back(relationshipStatus);
        DmgProfile::Employment employment = static_cast<DmgProfile::Employment>(demographicProfile->get(
                                                DmgProfile::EMPLOYMENT));
		values_.push_back(employment);
        Person::RiskLevel riskLevel = person.getRiskLevel();
		values_.push_back(riskLevel);
        int ageGroup = -1;
        int age = person.getAge(MONTH);

        for(size_t i = 0; i < ageGroups.size(); ++i)
        {
                if(age >= ageGroups[i].lower && age <= ageGroups[i].upper)
                {
                        ageGroup = i;
                }
        }

        assert(ageGroup != -1);
		values_.push_back(ageGroup);
        Person::CD4Strata cd4Stratum = person.getCd4Stratum();
		values_.push_back(cd4Stratum);
	}
};