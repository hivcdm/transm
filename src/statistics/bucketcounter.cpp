#include "bucketcounter.hpp"

namespace transm {

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
    size_t countIndex = std::distance(countNames_.begin(),
        std::find(countNames_.begin(), countNames_.end(), count));

    BucketContainer::iterator itr = counts_.find(bucket);
    if(itr == counts_.end())
    {
        auto values = std::vector<int>(countNames_.size(), 0);
        values[countIndex]++;
        counts_.emplace(bucket, values);
    } else {
        itr->second[countIndex]++;
    }
}

} // namespace transm
