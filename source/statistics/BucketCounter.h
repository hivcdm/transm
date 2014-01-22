#pragma once

#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <functional>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <vector>

#include "Bucket.h"

class BucketCounter
{
	typedef std::unordered_map<Bucket, std::vector<int>, bucket_hash<Bucket>, bucket_equal_to<Bucket>> BucketContainer;

	struct query_equal
	{
		query_equal(const std::vector<int> &indices) : indices(indices) {}

		bool operator()(const BucketContainer::value_type &b)
		{
			for(int i = 0; i < indices.size(); i++)
			{
				if(indices[i] != -1 && b.first.GetValue(i) != indices[i])
				{
					return false;
				}
			}

			return true;
		}

		std::vector<int> indices;
	};

public:
	BucketCounter() {}

	BucketCounter(const std::vector<std::string> &buckets, const std::vector<std::string> &counts);

	void operator=(const BucketCounter &rhs)
	{
		countNames_.assign(rhs.countNames_.begin(), rhs.countNames_.end());
		bucketNames_.assign(rhs.bucketNames_.begin(), rhs.bucketNames_.end());

		counts_.clear();
		for(auto pair : rhs.counts_)
		{
			counts_[pair.first] = pair.second;
		}
	}

	void Reset();

	void Increment(const Bucket &bucket, const std::string &count);

	template<typename ... Ts>
	int GetCount(const std::string &count, const Ts &... query)
	{
		auto predicate = query_equal(BuildQueryIndices(query...));
		auto bucketIterator = std::find_if(counts_.begin(), counts_.end(), predicate);
		int sum = 0;
		int countIndex = std::distance(countNames_.begin(), std::find(countNames_.begin(), countNames_.end(), count));

		while(bucketIterator != counts_.end())
		{
			sum += bucketIterator->second[countIndex];
			bucketIterator = std::find_if(++bucketIterator, counts_.end(), predicate);
		}

		return sum;
	}

	template<typename ... Ts>
	std::vector<int> GetCounts(const Ts &... query)
	{
		auto predicate = query_equal(BuildQueryIndices(query...));
		auto bucketIterator = std::find_if(counts_.begin(), counts_.end(), predicate);
		std::vector<int> sums(countNames_.size(), 0);

		while(bucketIterator != counts_.end())
		{
			std::transform(sums.begin(), sums.end(), bucketIterator->second.begin(), sums.begin(), std::plus<int>());
			bucketIterator = std::find_if(++bucketIterator, counts_.end(), predicate);
		}

		return sums;
	}

private:
	template<typename ... Ts>
	std::vector<int> BuildQueryIndices(const Ts &... query)
	{
		const int size = sizeof...(query);
		std::pair<std::string, int> r[size] = {query...};

		std::vector<int> indices;

		for(auto key : bucketNames_)
		{
			indices.push_back(-1);
			for(auto pair : r)
			{
				if(pair.first == key)
				{
					indices.back() = pair.second;
					break;
				}
			}
		}

		return indices;
	}

	std::vector<std::string> countNames_;
	BucketContainer counts_;
	std::vector<std::string> bucketNames_;
};