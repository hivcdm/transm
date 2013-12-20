#pragma once

#include <array>
#include <utility>
#include <vector>

#include "../entities/classifiers/DmgProfile.h"
#include "../entities/Person.h"

const int NUM_COUNTING_BUCKET_PARAMETERS = 8;

struct CountingBucket
{
	CountingBucket(DmgProfile::SexualActivityStatus sexualActivityStatus,
	               DmgProfile::Gender gender,
				   DmgProfile::SexualOrientation sexualOrientation,
				   DmgProfile::RelationshipStatus relationshipStatus,
				   DmgProfile::Employment employment,
				   Person::RiskLevel riskLevel,
				   int ageGroup,
				   Person::CD4Strata cd4Stratum) :
	    sexualActivityStatus(sexualActivityStatus),
		gender(gender),
		sexualOrientation(sexualOrientation),
		relationshipStatus(relationshipStatus),
		employment(employment),
		riskLevel(riskLevel),
		ageGroup(ageGroup),
		cd4Stratum(cd4Stratum) {}

	CountingBucket(std::array<int, NUM_COUNTING_BUCKET_PARAMETERS> array) :
		sexualActivityStatus(static_cast<DmgProfile::SexualActivityStatus>(array[0])),
		gender(static_cast<DmgProfile::Gender>(array[1])),
		sexualOrientation(static_cast<DmgProfile::SexualOrientation>(array[2])),
		relationshipStatus(static_cast<DmgProfile::RelationshipStatus>(array[3])),
		employment(static_cast<DmgProfile::Employment>(array[4])),
		riskLevel(static_cast<Person::RiskLevel>(array[5])),
		ageGroup(array[6]) {}

	CountingBucket(Person *person, const std::vector<std::pair<int, int> > &ageRanges)
	{
		const DmgProfile *demographicProfile = person->getDmgProfile();

		sexualActivityStatus = static_cast<DmgProfile::SexualActivityStatus>(demographicProfile->get(DmgProfile::SEXUAL_ACTIVITY_STATUS));
		gender = static_cast<DmgProfile::Gender>(demographicProfile->get(DmgProfile::GENDER));
		sexualOrientation = static_cast<DmgProfile::SexualOrientation>(demographicProfile->get(DmgProfile::SEXUAL_ORIENTATION));
		relationshipStatus = static_cast<DmgProfile::RelationshipStatus>(demographicProfile->get(DmgProfile::RELATIONSHIP_STATUS));
		employment = static_cast<DmgProfile::Employment>(demographicProfile->get(DmgProfile::EMPLOYMENT));
		riskLevel = person->getRiskLevel();
		ageGroup = -1;

		int age = person->getAge(MONTH);
		for(size_t i = 0; i < ageRanges.size(); ++i)
		{
			if(age >= ageRanges[i].first && age <= ageRanges[i].second)
			{
				ageGroup = i;
			}
		}
		assert(ageGroup != -1);
		cd4Stratum = person->cd
	}

	DmgProfile::SexualActivityStatus sexualActivityStatus;
	DmgProfile::Gender gender;
	DmgProfile::SexualOrientation sexualOrientation;
	DmgProfile::RelationshipStatus relationshipStatus;
	DmgProfile::Employment employment;
	Person::RiskLevel riskLevel;
	int ageGroup;
	Person::CD4Strata cd4Stratum;

	std::array<int, NUM_COUNTING_BUCKET_PARAMETERS> toArray() const
	{
		std::array<int, NUM_COUNTING_BUCKET_PARAMETERS> a = {{sexualActivityStatus, gender, sexualOrientation, relationshipStatus, employment, riskLevel, ageGroup}};
	    return a;
	}

	std::array<std::vector<int>, NUM_COUNTING_BUCKET_PARAMETERS> toCartesianArray(const CountingBucket &endBucket) const
	{
		std::array<std::vector<int>, NUM_COUNTING_BUCKET_PARAMETERS> r;
		std::array<int, NUM_COUNTING_BUCKET_PARAMETERS> endArray = endBucket.toArray();
		std::array<int, NUM_COUNTING_BUCKET_PARAMETERS> selfArray = toArray();

		for(int parameterIndex = 0; parameterIndex < NUM_COUNTING_BUCKET_PARAMETERS; parameterIndex++)
		{
			int maxValue = endArray[parameterIndex];
			if(selfArray[parameterIndex] < maxValue)
			{
				r[parameterIndex].push_back(selfArray[parameterIndex]);
			}
			else
			{
				for(int i = 0; i < maxValue; ++i)
				{
					r[parameterIndex].push_back(i);
				}
			}
		}

		return r;
	}
};

inline bool operator==(const CountingBucket &a, const CountingBucket &b)
{
	return a.sexualActivityStatus == b.sexualActivityStatus &&
		a.gender == b.gender &&
		a.sexualOrientation == b.sexualOrientation &&
		a.relationshipStatus == b.relationshipStatus &&
		a.employment == b.employment &&
		a.riskLevel == b.riskLevel &&
		a.ageGroup == b.ageGroup &&
		a.cd4Stratum == b.cd4Stratum;
}

namespace std {

	template <>
	struct hash<CountingBucket>
	{
		std::size_t operator()(const CountingBucket& e) const
		{
			std::size_t seed = 0;
			hash_combine(seed, e.sexualActivityStatus);
			hash_combine(seed, e.gender);
			hash_combine(seed, e.sexualOrientation);
			hash_combine(seed, e.relationshipStatus);
			hash_combine(seed, e.employment);
			hash_combine(seed, e.riskLevel);
			hash_combine(seed, e.ageGroup);
			hash_combine(seed, e.cd4Stratum);
			return seed;
		}
	};

}
