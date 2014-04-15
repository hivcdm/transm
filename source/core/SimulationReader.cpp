
#include "SimulationReader.h"
#include "SimulationBuilder.h"
#include "PopulationParameters.h"
#include "entities/behaviors/SexualBehavior.h"

SimulationReader::SimulationReader(SimulationBuilder &builder) : builder_(builder)
{

}

void SimulationReader::ConstructSimulation(const std::string &filename)
{
	builder_.Reset();
	builder_.SetInputFile(filename);
	builder_.CheckVersion();
	builder_.LoadTemplateParameters();
	builder_.ReadSimulationParameters();
	builder_.ReadPopulationParameters();
	builder_.InitializePopulation();
}