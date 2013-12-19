#pragma once

#include <array>
#include <utility>
#include <vector>

#include "../entities/classifiers/DmgProfile.h"
#include "../entities/Person.h"

struct CountingBucket
{
	CountingBucket(DmgProfile::SexualActivityStatus sexualActivityStatus,
	               DmgProfile::Gender gender,
				   DmgProfile::SexualOrientation sexualOrientation,
				   DmgProfile::RelationshipStatus relationshipStatus,
				   DmgProfile::Employment employment,
				   Person::RiskLevel riskLevel,
		int ageGroup) :
	    sexualActivityStatus(sexualActivityStatus),
		gender(gender),
		sexualOrientation(sexualOrientation),
		relationshipStatus(relationshipStatus),
		employment(employment),
		riskLevel(riskLevel),
		ageGroup(ageGroup) {}

	CountingBucket(std::array<int, 7> array) :
		sexualActivityStatus(static_cast<DmgProfile::SexualActivityStatus>(array[0])),
		gender(static_cast<DmgProfile::Gender>(array[1])),
		sexualOrientation(static_cast<DmgProfile::SexualOrientation>(array[2])),
		relationshipStatus(static_cast<DmgProfile::RelationshipStatus>(array[3])),
		employment(static_cast<DmgProfile::Employment>(array[4])),
		riskLevel(static_cast<Person::RiskLevel>(array[5])),
		ageGroup(array[6]) {}

	DmgProfile::SexualActivityStatus sexualActivityStatus;
	DmgProfile::Gender gender;
	DmgProfile::SexualOrientation sexualOrientation;
	DmgProfile::RelationshipStatus relationshipStatus;
	DmgProfile::Employment employment;
	Person::RiskLevel riskLevel;
	int ageGroup;

	std::array<int, 7> toArray() const
	{
	    std::array<int, 7> a = {{sexualActivityStatus, gender, sexualOrientation, relationshipStatus, employment, riskLevel, ageGroup}};
	    return a;
	}

	std::array<std::vector<int>, 7> toCartesianArray(const CountingBucket &endBucket) const
	{
		std::array<std::vector<int>, 7> r;
		std::array<int, 7> endArray = endBucket.toArray();
		std::array<int, 7> selfArray = toArray();

		for(int parameterIndex = 0; parameterIndex < 7; parameterIndex++)
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

	const int NUM_PARAMETERS = 7;
};

inline bool operator==(const CountingBucket &a, const CountingBucket &b)
{
	return a.sexualActivityStatus == b.sexualActivityStatus &&
		a.gender == b.gender &&
		a.sexualOrientation == b.sexualOrientation &&
		a.relationshipStatus == b.relationshipStatus &&
		a.employment == b.employment &&
		a.riskLevel == b.riskLevel &&
		a.ageGroup == b.ageGroup;
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
			return seed;
		}
	};

}
