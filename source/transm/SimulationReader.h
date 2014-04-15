#pragma once

#include <memory>
#include <string>

class SimulationBuilder;
class Simulation;

class SimulationReader
{
public:
	SimulationReader(SimulationBuilder &builder);

	void operator=(const SimulationReader &) = delete;

	void ConstructSimulation(const std::string &filename);

private:
	SimulationBuilder &builder_;
};