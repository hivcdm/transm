#include "BucketCounter.h"

BucketCounter::BucketCounter(const std::vector<std::string> &buckets, const std::vector<std::string> &counts)
: countNames_(counts), bucketNames_(buckets)
{

}

void BucketCounter::Reset()
{
	counts_.clear();
}

void BucketCounter::Increment(const Bucket &bucket, const std::string &count)
{
	if(counts_.find(bucket) == counts_.end())
	{
		counts_[bucket] = std::vector<std::size_t>(countNames_.size(), 0);
	}

	size_t countIndex = std::distance(countNames_.begin(), std::find(countNames_.begin(), countNames_.end(), count));
	++counts_[bucket][countIndex];
}
