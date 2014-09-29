#pragma once

#include <array>
#include <cassert>
#include <unordered_map>

#include "statistics/InterventionOutcomes.hpp"

class Outputs
{
public:
	Outputs() {}
    InterventionOutcomes intervention_outcomes;
};
