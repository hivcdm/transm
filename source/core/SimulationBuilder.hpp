#pragma once

#include "Simulation.hpp"
#include "PopulationParameters.hpp"

class SimulationBuilder
{
public:
	virtual void Reset() = 0;
	virtual void SetInputFile(const std::string &filename) = 0;
	virtual void CheckVersion() = 0;
	virtual void ReadSimulationParameters() = 0;
	virtual void ReadPopulationParameters() = 0;
	virtual void InitializePopulation() = 0;
};