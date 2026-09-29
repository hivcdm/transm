#ifndef BUCKETCOUNTER_HPP
#define BUCKETCOUNTER_HPP

#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <functional>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "bucket.hpp"

namespace transm {

struct QueryField
{
    std::string key;
	int value;
	bool wildcard;
};

class BucketCounter
{
    using BucketContainer = std::unordered_map<Bucket, std::vector<int>,
          bucket_hash<Bucket>, bucket_equal_to<Bucket>>;

	struct query_equal
	{
		query_equal(std::vector<QueryField> indices) : indices(std::move(indices)) {}

		bool operator()(const BucketContainer::value_type &b)
		{
			for (auto & indice : indices)
			{
				if (!indice.wildcard && !b.first.HasKeyValue(indice.key, indice.value))
				{
					return false;
				}
			}

			return true;
		}

		std::vector<QueryField> indices;
	};

public:
	BucketCounter() {}

	BucketCounter(std::vector<std::string> buckets, std::vector<std::string> counts);

	void operator=(const BucketCounter &rhs)
	{
		countNames_.assign(rhs.countNames_.begin(), rhs.countNames_.end());
		bucketNames_.assign(rhs.bucketNames_.begin(), rhs.bucketNames_.end());

		counts_.clear();
		for(const auto& pair : rhs.counts_)
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
        /* Same guard as Increment(): an unregistered name has no slot, and the
         * index would otherwise run one past the end of every bucket's vector. */
        auto name = std::find(countNames_.begin(), countNames_.end(), count);
        if (name == countNames_.end())
            return 0;
        int countIndex = (int)std::distance(countNames_.begin(), name);

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
            std::transform(sums.begin(), sums.end(), bucketIterator->second.begin(),
                sums.begin(), std::plus<int>());
            bucketIterator = std::find_if(++bucketIterator, counts_.end(), predicate);
        }

        return sums;
    }

private:
	template<typename ... Ts>
	std::vector<QueryField> BuildQueryIndices(const Ts &... query)
	{
		const std::size_t size = sizeof...(query);
		std::pair<std::string, int> queries[size] = {query...};

		std::vector<QueryField> indices;

		for(const auto& key : bucketNames_)
		{
			indices.push_back({"*", 0, true});
			for(auto pair : queries)
			{
				if(pair.first == key)
				{
                    indices.back().key = pair.first;
					indices.back().value = pair.second;
					indices.back().wildcard = false;
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

} // namespace transm


#endif /* BUCKETCOUNTER_HPP */