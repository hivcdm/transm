
#include "SimulationReader.hpp"
#include "SimulationBuilder.hpp"
#include "PopulationParameters.hpp"
#include "entities/SexualBehavior.hpp"

SimulationReader::SimulationReader(SimulationBuilder &builder) : builder_(builder)
{

}

void SimulationReader::ConstructSimulation(const std::string &filename)
{
	builder_.Reset();
	builder_.SetInputFile(filename);
	builder_.CheckVersion();
	builder_.ReadSimulationParameters();
	builder_.ReadPopulationParameters();
	builder_.InitializePopulation();
}