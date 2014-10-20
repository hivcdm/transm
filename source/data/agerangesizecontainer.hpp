#pragma once

#include <utility>
#include <vector>

struct AgeRange
{
	int lower;
	int upper;
};

typedef std::pair<AgeRange, std::size_t> AgeRangeSizePair;
typedef std::vector<AgeRangeSizePair> AgeRangeSizeContainer;
