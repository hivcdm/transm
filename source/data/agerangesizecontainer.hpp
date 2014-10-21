#pragma once

#include <utility>
#include <vector>

namespace transm {

struct AgeRange
{
	int lower;
	int upper;
};

using AgeRangeSizePair = std::pair<AgeRange, std::size_t>;
using AgeRangeSizeContainer = std::vector<AgeRangeSizePair>;

} // namespace transm
