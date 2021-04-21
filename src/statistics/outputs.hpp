#pragma once

#include <array>
#include <cassert>
#include <unordered_map>

#include "statistics/interventionoutcomes.hpp"
#include "statistics/populationstatistics.hpp"

namespace transm {

class Outputs
{
public:
	Outputs() {}
    InterventionOutcomes intervention_outcomes;
    PopulationStatistics population;
};

} // namespace transm
