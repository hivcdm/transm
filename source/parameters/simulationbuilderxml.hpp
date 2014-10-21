#pragma once

#include <string>
#include <pugixml.hpp>

#include "simulationbuilder.hpp"
#include "core/batchstatus.hpp"
#include "core/simulation.hpp"
#include "entities/sexualbehavior.hpp"
#include "utility/cepacinputparser.hpp"

namespace transm {

class SimulationBuilderXml : public SimulationBuilder
{
public:
	SimulationBuilderXml(BatchStatus &status) : simulation_(status) {}

	Simulation &GetResult();

	void Reset();

	void SetInputFile(const std::string &filename);

	void CheckVersion();

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

    using EntityDistributions = std::unordered_map<std::string, double>;

    EntityDistributions ReadEntityDistributions(pugi::xml_node node);

	EventParams::RolloutEligibility ReadRolloutEligibility();

    std::unordered_map<TransmissionType, std::array<double, Entity::ENDHVLStrata>> ReadTransmissionCoefficients();

	Female::SubPopParams ReadFemaleSubPopParams();

	Male::SubPopParams ReadMaleSubPopParams();

    Msm::SubPopParams ReadMsmSubPopParams();

    Msmw::SubPopParams ReadBiMaleSubPopParams();

    std::vector<Intervention> ParseInterventions(pugi::xml_node interventions_node, bool individual);

	SexualBehavior ReadSexualBehavior(const std::string &entity_type, SexualPartnership::Type type);

    Intervention ReadIntervention(pugi::xml_node &node, bool individual);

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

} // namespace transm
