#pragma once

#include <string>

#include "SimulationBuilder.h"
#include "Simulation.h"
#include "../util/CepacInputParser.h"
#include "../entities/behaviors/SexualBehavior.h"
#include "util/xml/pugixml.hpp"

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

private:
	static std::pair<std::string, std::string> ExtractParameter(const pugi::xml_node &node);

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

	template<typename T>
	T GetTemplate(const pugi::xml_node &node, std::function<void(Simulation &, T)> callback)
	{
		auto extracted = ExtractParameter(node);
		if(extracted.first != "")
		{
			if(parameters_.find(extracted.first) != parameters_.end())
			{
				for(auto &parameter : parameters_[extracted.first])
				{
					simulation_.RegisterSimulationIntervention(parameter.time, std::bind(callback, std::placeholders::_1, from_string<T>(parameter.value)));
				}
			}
		}
		return from_string<T>(extracted.second);
	}

	template<typename T>
	T GetTargetedTemplate(const pugi::xml_node &node, std::function<void(Simulation &, T, Nullable<PopulationTarget>)> callback)
	{
		auto extracted = ExtractParameter(node);
		if(extracted.first != "")
		{
			if(parameters_.find(extracted.first) != parameters_.end())
			{
				for(auto &parameter : parameters_[extracted.first])
				{
					simulation_.RegisterSimulationIntervention(parameter.time, std::bind(callback, std::placeholders::_1, from_string<T>(parameter.value), parameter.target));
				}
			}
		}
		return from_string<T>(extracted.second);
	}

	EventParams::RolloutEligibility ReadRolloutEligibility();

	Female::SubPopParams ReadFemaleSubPopParams();

	Male::SubPopParams ReadMaleSubPopParams();

	SexualBehavior ReadSexualBehavior(SexualPartnership::Type type);

	pugi::xml_document document_;

	Simulation simulation_;

	struct Parameter
	{
		int time;
		std::string value;
		Nullable<PopulationTarget> target;
	};

	std::unordered_map<std::string, std::vector<Parameter>> parameters_;

	PopulationParameters population_parameters;
};
