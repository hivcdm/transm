#pragma once

#include <array>
#include <cassert>
#include <unordered_map>

#include "../statistics/InterventionOutcomes.h"

class Outputs
{
public:
	Outputs() {}
    InterventionOutcomes intervention_outcomes;
};
