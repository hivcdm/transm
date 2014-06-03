#pragma once

#include <string>

#include "SimulationBuilder.h"
#include "Simulation.h"
#include "../util/CepacInputParser.h"
#include "../entities/behaviors/SexualBehavior.h"
#include "../util/xml/pugixml.hpp"

class SimulationBuilderXml : public SimulationBuilder
{
public:
	SimulationBuilderXml() : simulation_() {}

	Simulation &GetResult();

	void Reset();

	void SetInputFile(const std::string &filename);

	void CheckVersion();

	void LoadTemplateParameters();

	void ReadSimulationParameters();

	void ReadPopulationParameters();

	void InitializePopulation();

    std::unordered_map<std::string, TargetGroup> ReadGroups();

private:
	template<typename T>
	static T from_string(const std::string &value_string);

	template<typename T>
	static T Text(const pugi::xml_node &node)
	{
		return from_string<T>(node.text().as_string());
	}

	template<typename T>
	static T Attr(const pugi::xml_node &node, const std::string &name)
	{
		return from_string<T>(node.attribute(name.c_str()).as_string());
	}

	EventParams::RolloutEligibility ReadRolloutEligibility();

	Female::SubPopParams ReadFemaleSubPopParams();

	Male::SubPopParams ReadMaleSubPopParams();

    std::vector<Intervention> ParseInterventions(pugi::xml_node interventions_node, bool individual);

	SexualBehavior ReadSexualBehavior(SexualPartnership::Type type);

    NormalDist GetNormalDist(const pugi::xml_node node);
    LogNormalDist GetLogNormalDist(const pugi::xml_node node);
    BetaDist GetBetaDist(const pugi::xml_node node);
    ShiftedLogNormalDist GetShiftedLogNormalDist(const pugi::xml_node node);

	pugi::xml_document document_;

	Simulation simulation_;

	struct Parameter
	{
		int time;
		std::string value;
		Nullable<TargetGroup::PopulationTarget> target;
	};

	std::unordered_map<std::string, std::vector<Parameter>> parameters_;

	PopulationParameters population_parameters;
};
