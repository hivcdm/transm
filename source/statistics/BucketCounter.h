#pragma once

#include <array>
#include <unordered_map>
#include <vector>

#include "CountingBucket.h"

class BucketCounter
{
	typedef std::unordered_map<CountingBucket, int> CountMap;

public:
	BucketCounter(int numAgeGroups = 1) :
		endBucket(DmgProfile::ENDSexualActivityStatus, 
		          DmgProfile::ENDGender,
				  DmgProfile::ENDSexualOrientation,
				  DmgProfile::ENDRelationshipStatus,
				  DmgProfile::ENDEmployment,
				  Person::ENDRiskLevel,
				  numAgeGroups,
				  Person::ENDCD4Strata)
	{
	}

	~BucketCounter() {}

	void SetNumAgeGroups(int numAgeGroups)
	{
		endBucket.ageGroup = numAgeGroups;
	}

	void Increment(const CountingBucket &bucket)
	{
		countMap[bucket]++;
	}

	int GetCount(const CountingBucket &bucket)
	{
		int count = 0;
		for(auto &filter : GenerateCartesianProduct(bucket))
		{
			if(countMap.find(filter) != countMap.end())
			{
				count += countMap[filter];
			}
		}
		return count;
	}

	void Clear()
	{
		countMap.clear();
	}

private:
	struct CartesianIterators
	{
		std::vector<int>::const_iterator begin;
		std::vector<int>::const_iterator end;
		std::vector<int>::const_iterator me;
	};

	std::vector<CountingBucket> GenerateCartesianProduct(const CountingBucket &bucket)
	{
		std::vector<CartesianIterators> cartesianIterators;
		auto bucketArray = bucket.toCartesianArray(endBucket);
		std::vector<CountingBucket> resultSet;

		for(auto bucketArrayIterator = bucketArray.begin(); bucketArrayIterator != bucketArray.end(); ++bucketArrayIterator)
		{
			CartesianIterators currentIterators = {bucketArrayIterator->begin(), bucketArrayIterator->end(), bucketArrayIterator->begin()};
			cartesianIterators.push_back(currentIterators);
		}

		while(true)
		{
			std::array<int, NUM_COUNTING_BUCKET_PARAMETERS> result;
			for(int i = 0; i < NUM_COUNTING_BUCKET_PARAMETERS; i++)
			{
				result[i] = *cartesianIterators[i].me;
			}
			resultSet.push_back(CountingBucket(result));

			for(auto cartesianIterator = cartesianIterators.begin();;)
			{
				++(cartesianIterator->me);
				if(cartesianIterator->me == cartesianIterator->end)
				{
					if(cartesianIterator + 1 == cartesianIterators.end())
					{
						return resultSet;
					}
					else
					{
						cartesianIterator->me = cartesianIterator->begin;
						++cartesianIterator;
					}
				}
				else 
				{
					break;
				}
			}
		}
	}

	CountMap countMap;
	CountingBucket endBucket;
};
