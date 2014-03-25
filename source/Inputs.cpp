#include <sstream>
#include <boost/filesystem.hpp>

#include "Inputs.h"

namespace {

void ParsePossibleTemplateParameter(TemplateParameter parameter_type, double &result, const std::string &value_string,
	std::unordered_map<std::string, TemplateParameter> &template_key_map)
{
	if(value_string.front() == '{' && value_string.back() == '}')
	{
		auto separator = value_string.find(',');
		auto key = value_string.substr(1, separator);
		result = std::stod(value_string.substr(separator + 1, value_string.length() - 1));
		template_key_map[key] = parameter_type;
	}
	else
	{
		result = std::stod(value_string);
	}
}

PopulationTarget ParsePopulationTarget(const std::string &target_string)
{
	PopulationTarget target;
	std::stringstream stream(target_string);
	std::string part;
	std::getline(stream, part, ':');
	if(part != "*")
	{
		target.age_lower.value = std::stoi(part);
		target.age_lower.has_value = true;
	}
	std::getline(stream, part, ':');
	if(part != "*")
	{
		target.age_upper.value = std::stoi(part);
		target.age_upper.has_value = true;
	}
	std::getline(stream, part, ':');
	if(part != "*")
	{
		target.employment.value = static_cast<DmgProfile::Employment>(std::stoi(part));
		target.employment.has_value = true;
	}
	std::getline(stream, part, ':');
	if(part != "*")
	{
		target.gender.value = static_cast<DmgProfile::Gender>(std::stoi(part));
		target.gender.has_value = true;
	}
	std::getline(stream, part, ':');
	if(part != "*")
	{
		target.observed_hiv_status.value = static_cast<Person::HIVStatus>(std::stoi(part));
		target.observed_hiv_status.has_value = true;
	}
	std::getline(stream, part, ':');
	if(part != "*")
	{
		target.on_treatment.value = std::stoi(part) != 0;
		target.on_treatment.has_value = true;
	}
	std::getline(stream, part, ':');
	if(part != "*")
	{
		target.relationship_status.value = static_cast<DmgProfile::RelationshipStatus>(std::stoi(part));
		target.relationship_status.has_value = true;
	}
	std::getline(stream, part, ':');
	if(part != "*")
	{
		target.risk_level.value = static_cast<Person::RiskLevel>(std::stoi(part));
		target.risk_level.has_value = true;
	}
	std::getline(stream, part, ':');
	if(part != "*")
	{
		target.sexual_activity_status.value = static_cast<DmgProfile::SexualActivityStatus>(std::stoi(part));
		target.sexual_activity_status.has_value = true;
	}
	std::getline(stream, part, ':');
	if(part != "*")
	{
		target.sexual_orientation.value = static_cast<DmgProfile::SexualOrientation>(std::stoi(part));
		target.sexual_orientation.has_value = true;
	}
}

} // namespace 

Inputs Inputs::FromFile(const std::string &filename)
{
	ticpp::Document xml;
	xml.LoadFile(filename);

	Inputs inputs;

	inputs.filename_ = filename;
	inputs.run_name_ = boost::filesystem::path(filename).stem().string();

	auto &root_node = *xml.FirstChildElement("simulation");
	inputs.LoadSimulationParameters(root_node);

	auto &calibration_node = *root_node.FirstChildElement("calibration");
	inputs.LoadCalibrationSettings(calibration_node);

	auto &write_trace_node = *root_node.FirstChildElement("writeTrace");
	auto &extension_names_node = *root_node.FirstChildElement("extensionNames");
	auto &toss_files_node = *calibration_node.FirstChildElement("tossFiles");
	inputs.LoadTracingSettings(root_node, write_trace_node, extension_names_node, toss_files_node);

	auto &population_node = *root_node.FirstChildElement("population");
	inputs.LoadPopulationSettings(population_node);

	auto &costs_node = *root_node.FirstChildElement("costs");
	inputs.LoadCosts(costs_node);

	auto &interventions_node = *root_node.FirstChildElement("interventions");
	inputs.LoadInterventions(interventions_node);

	inputs.ValidateSimulationParameters();
	inputs.ValidateTracingSettings();
	inputs.ValidateCalibrationSettings();
	inputs.ValidatePopulationSettings();
	inputs.ValidateCosts();
	inputs.ValidateInterventions();

	return inputs;
};

void Inputs::LoadSimulationParameters(const ticpp::Element &root_node)
{
	std::string version_string = root_node.FirstChildElement("inputVersion")->GetText();
	auto major_separator = version_string.find('.');
	version_.major = std::stoi(version_string.substr(0, major_separator));
	auto minor_separator = version_string.find('.', major_separator);
	version_.minor = std::stoi(version_string.substr(major_separator + 1, minor_separator));
	if(minor_separator != std::string::npos)
	{
		version_.patch = std::stoi(version_string.substr(minor_separator + 1));
	}

	debug_level_ = std::stoi(root_node.FirstChildElement("debugLevel")->GetText());
	duration_ = std::stoi(root_node.FirstChildElement("timeLimitMth")->GetText());
	fixed_seed_ = std::stoi(root_node.FirstChildElement("fixedSeed")->GetText());
	month_of_1990_ = std::stoi(root_node.FirstChildElement("monthOf1990")->GetText());
}

void Inputs::ValidateSimulationParameters()
{
	if(version_.major != 3)
	{
		throw std::runtime_error("Invalid version");
	}
}

void Inputs::LoadTracingSettings(const ticpp::Element &root_node, const ticpp::Element &write_trace_node,
	const ticpp::Element &extension_names_node, const ticpp::Element &toss_files_node)
{
	static const std::unordered_map<TraceFile::Type, std::string> type_strings = 
	{
		{TraceFile::Type::Population, "population"},
		{TraceFile::Type::Population, "population"},
		{TraceFile::Type::Population, "population"}, 
		{TraceFile::Type::Population, "population"}, 
		{TraceFile::Type::Population, "population"}, 
		{TraceFile::Type::Population, "population"}, 
		{TraceFile::Type::Population, "population"}, 
		{TraceFile::Type::Population, "population"}, 
		{TraceFile::Type::Population, "population"}, 
		{TraceFile::Type::Population, "population"}, 
		{TraceFile::Type::Population, "population"}
	};

	for(int type = static_cast<int>(TraceFile::Type::First); type != static_cast<int>(TraceFile::Type::Last); type)
	{
		TraceFile trace_file;
		trace_file.type = static_cast<TraceFile::Type>(type);
		std::string node_value = type_strings.at(static_cast<TraceFile::Type>(type));
		trace_file.enabled = write_trace_node.FirstChildElement(node_value)->GetText() != "0";
		trace_file.extension = extension_names_node.FirstChildElement(node_value)->GetText();
		trace_file.toss = toss_files_node.FirstChildElement("traceFile" + std::to_string(int(type)))->GetText() != "0";
		trace_files_[static_cast<TraceFile::Type>(type)] = trace_file;
	}

	number_to_trace_ = std::stoi(root_node.FirstChildElement("numberToTracePerAgeRange")->GetText());
	number_newborns_to_trace_ = std::stoi(root_node.FirstChildElement("numberNewbornsToTrace")->GetText());
	month_trace_newborns_ = std::stoi(root_node.FirstChildElement("monthTraceNewborns")->GetText());
	trace_prevalent_cases_ = std::stoi(root_node.FirstChildElement("tracePrevalentCases")->GetText());
}

void Inputs::ValidateTracingSettings()
{
	for(auto &trace_file_key_value : trace_files_)
	{
		if(trace_file_key_value.first != trace_file_key_value.second.type)
		{
			throw std::runtime_error("not equal");
		}

		if(trace_file_key_value.second.extension.substr(trace_file_key_value.second.extension.length() - 4) != ".xls")
		{
			throw std::runtime_error("trace file should have extension .xls");
		}
	}
}

void Inputs::LoadCalibrationSettings(const ticpp::Element &calibration_node)
{

}

void Inputs::ValidateCalibrationSettings()
{

}

void Inputs::LoadPopulationSettings(const ticpp::Element &population_node)
{
	ParsePossibleTemplateParameter(TemplateParameter::BirthRate, population_settings_.birth_rate,
		population_node.FirstChildElement("birthRate")->GetText(), template_key_map_);
	ParsePossibleTemplateParameter(TemplateParameter::ProportionMale, population_settings_.proportion_male,
		population_node.FirstChildElement("proportionMale")->GetText(), template_key_map_);
	ParsePossibleTemplateParameter(TemplateParameter::ProportionCircumcised, population_settings_.proportion_circumcised,
		population_node.FirstChildElement("proportionCircumcised")->GetText(), template_key_map_);
	ParsePossibleTemplateParameter(TemplateParameter::AgeSexualDebutYears, population_settings_.age_sexual_debut,
		population_node.FirstChildElement("ageSexualDebutYrs")->GetText(), template_key_map_);
}

void Inputs::ValidatePopulationSettings()
{

}

void Inputs::LoadCosts(const ticpp::Element &costs_node)
{

}

void Inputs::ValidateCosts()
{

}

void Inputs::LoadInterventions(const ticpp::Element &interventions_node)
{
	auto &miscellaneous_interventions = *interventions_node.FirstChildElement("miscellaneousInterventions");
	auto current_intervention = miscellaneous_interventions.FirstChildElement();

	while(current_intervention != nullptr)
	{
		TimeDependentParameter param;

		param.key = current_intervention->GetAttribute("key");
		param.time = std::stoi(current_intervention->GetAttribute("time"));
		param.value = std::stod(current_intervention->GetAttribute("value"));
		param.target_population.has_value = current_intervention->GetAttribute("targetPopulation") != "";

		if(param.target_population.has_value)
		{
			param.target_population.value = ParsePopulationTarget(current_intervention->GetAttribute("targetPopulation"));
		}
		
		current_intervention = current_intervention->NextSiblingElement();
	}
}

void Inputs::ValidateInterventions()
{

}