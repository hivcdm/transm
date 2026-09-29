#include "bucketcounter.hpp"

#include <utility>

namespace transm {

BucketCounter::BucketCounter(std::vector<std::string> buckets, std::vector<std::string> counts)
: countNames_(std::move(counts)), bucketNames_(std::move(buckets))
{

}

void BucketCounter::Reset()
{
    counts_.clear();
}

void BucketCounter::Increment(const Bucket &bucket, const std::string &count)
{
    /* A count name that was never registered has no slot. std::find then
     * returns end(), and the index used to fall through to countNames_.size()
     * -- one past the end -- so the ++ below wrote outside the vector.
     *
     * This is not hypothetical: ArtRolloutTracker registers only nine outcomes
     * (see the "can't take more than 9" note on TRACKED_OUTCOMES) but still
     * increments ten more -- unlinked, return_to_care, ltfu, firstlineART,
     * secondlineART, test_result, eligible_for_treatment, eligible_for_access,
     * accessing_treatment, death_on_treatment -- every month. Those writes
     * usually landed in allocator padding and did nothing, but occasionally
     * corrupted heap metadata, which surfaced as a segfault at process exit.
     *
     * Unregistered outcomes were never printed, so skipping them changes no
     * output; it only removes the out-of-bounds write. */
    auto name = std::find(countNames_.begin(), countNames_.end(), count);
    if (name == countNames_.end())
        return;
    size_t countIndex = std::distance(countNames_.begin(), name);

    auto itr = counts_.find(bucket);
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
