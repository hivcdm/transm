#pragma once

#include <utility>
#include <vector>

#include "utility/time.hpp"

namespace transm {

struct AgeRange
{
	Age lower;
	Age upper;
};

inline std::ostream &operator<<(std::ostream &stream, const AgeRange &range)
{
	stream << range.lower.in_months();
	stream << "-";
	stream << range.upper.in_months();
	return stream;
}

using AgeRangeSizePair = std::pair<AgeRange, std::size_t>;
using AgeRangeSizeContainer = std::vector<AgeRangeSizePair>;

} // namespace transm
