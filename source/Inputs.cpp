#include <sstream>
#include <boost/filesystem.hpp>

#include "Inputs.h"
#include "util/enum_iterator.h"

namespace {

std::string ExtractPossibleTemplateParameter(TemplateParameter parameter_type, 
	const std::string &value_string, std::unordered_map<std::string, TemplateParameter> &template_key_map)
{
	std::string result = value_string;

	if(value_string.front() == '{' && value_string.back() == '}')
	{
		auto separator = value_string.find(',');
		auto key = value_string.substr(1, separator);
		result = value_string.substr(separator + 1, value_string.length() - 1);
		template_key_map[key] = parameter_type;
	}

	return result;
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
	return target;
}

BetaDist ParseBetaDistribution(const ticpp::Element &node, bool use_coefficient_variation, double coefficient_of_variation)
{
	auto &distrib = *node.FirstChildElement("Distrib");
	BetaDist dist = {0};
	auto alpha_node = distrib.FirstChildElement("alpha", false);
	if(alpha_node != nullptr)
	{
		dist.alpha = std::stod(alpha_node->GetText());
		dist.beta = std::stod(distrib.FirstChildElement("beta")->GetText());
	}
	else
	{
		double mean;
		double stddev;

		if(use_coefficient_variation)
		{
			mean = std::stod(distrib.FirstChildElement("mean")->GetText());
			stddev = mean * coefficient_of_variation;
		}
		else
		{
			mean = std::stod(distrib.FirstChildElement("mean")->GetText());
			stddev = std::stod(distrib.FirstChildElement("stdDev")->GetText());
		}

		double sampleSize = mean * (1 - mean) / (stddev * stddev) - 1;
		dist.alpha = mean * sampleSize;
		dist.beta = (1 - mean) * sampleSize;
	}

	return dist;
}

NormalDist ParseNormalDistribution(const ticpp::Element &node)
{
	auto &distrib = *node.FirstChildElement("Distrib");
	if(distrib.FirstChildElement("type")->GetText() != "Normal")
	{
		throw std::runtime_error("wrong type");
	}
	NormalDist dist = {0};
	dist.mean = std::stod(distrib.FirstChildElement("mean")->GetText());
	dist.stddev = std::stod(distrib.FirstChildElement("stdDev")->GetText());
	return dist;
}

LogNormalDist ParseLogNormalDistribution(const ticpp::Element &node, bool use_coefficient_variation, double coefficient_of_variation)
{
	auto &distrib = *node.FirstChildElement("Distrib");
	auto type = distrib.FirstChildElement("type")->GetText();
	LogNormalDist dist;
	if(type == "LogNormal")
	{
		dist.mu = std::stod(distrib.FirstChildElement("mu")->GetText());
		dist.sigma = std::stod(distrib.FirstChildElement("sigma")->GetText());
	}
	else if(type == "Normal")
	{
		double mean = std::stod(distrib.FirstChildElement("mean")->GetText());
		double stddev = std::stod(distrib.FirstChildElement("stdDev")->GetText());

		if(mean <= 0)
		{
			dist.mu = 0;
			dist.sigma = 0;
			dist.isZeroDistrib = true;
			return dist;
		}

		if(use_coefficient_variation)
		{
			stddev = mean * coefficient_of_variation;
		}

		dist.mu = log(mean) - 0.5 * log(1 + (stddev * stddev) / (mean * mean));
		dist.sigma = sqrt(log(1 + (stddev * stddev) / (mean * mean)));
	}
	else
	{
		throw std::runtime_error("wrong type");
	}

	return dist;
}

double ParsePoissonDistribution(const ticpp::Element &node)
{
	auto &distrib = *node.FirstChildElement("Distrib");
	if(distrib.FirstChildElement("type")->GetText() != "Poisson")
	{
		throw std::runtime_error("wrong type");
	}
	return std::stod(distrib.FirstChildElement("mean")->GetText());
}

ShiftedLogNormalDist ParseShiftedLogNormalDistribution(const ticpp::Element &node)
{
	auto &distrib = *node.FirstChildElement("Distrib");
	ShiftedLogNormalDist dist;
	auto mu_node = distrib.FirstChildElement("mu", false);
	if(mu_node != nullptr)
	{
		dist.mu = std::stod(mu_node->GetText());
		dist.sigma = std::stod(distrib.FirstChildElement("sigma")->GetText());
		dist.shift = std::stod(distrib.FirstChildElement("shift")->GetText());
	}
	else
	{
		double mean = std::stod(distrib.FirstChildElement("mean")->GetText());
		double stddev = std::stod(distrib.FirstChildElement("stdDev")->GetText());

		if(mean <= 0)
		{
			dist.mu = 0;
			dist.sigma = 0;
			dist.shift = 0;
			dist.isZeroDistrib = true;
			return dist;
		}

		dist.shift = std::stod(distrib.FirstChildElement("shift")->GetText());
		dist.mu = log(mean - dist.shift) - 0.5 * log(1 + (stddev * stddev) / ((mean - dist.shift) * (mean - dist.shift)));
		dist.sigma = sqrt(log(1 + (stddev * stddev) / ((mean - dist.shift) * (mean - dist.shift))));
	}

	return dist;
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

	auto &costs_node = *population_node.FirstChildElement("costs");
	inputs.LoadCosts(costs_node);

	auto &interventions_node = *population_node.FirstChildElement("interventions");
	inputs.LoadInterventions(interventions_node);

	inputs.ValidateSimulationParameters();
	inputs.ValidateTracingSettings();
	inputs.ValidateCalibrationSettings();
	inputs.ValidatePopulationSettings();
	inputs.ValidateCosts();
	inputs.ValidateInterventions();

	return inputs;
};

Inputs::Inputs(const Inputs &other)
{
	(*this) = other;
}

Inputs::Inputs()
{

}

void Inputs::operator=(const Inputs &other)
{
	filename_ = other.filename_;
	run_name_ = other.run_name_;
	version_ = other.version_;
	debug_level_ = other.debug_level_;
	duration_ = other.duration_;
	fixed_seed_ = other.fixed_seed_;
	month_of_1990_ = other.month_of_1990_;
	number_to_trace_ = other.number_to_trace_;
	number_newborns_to_trace_ = other.number_newborns_to_trace_;
	month_trace_newborns_ = other.month_trace_newborns_;
	trace_prevalent_cases_ = other.trace_prevalent_cases_;
	life_expectancy_record_times_ = other.life_expectancy_record_times_;
	life_expectancy_median_condfidence_interval_ = other.life_expectancy_median_condfidence_interval_;
	partner_acquisition_record_times_ = other.partner_acquisition_record_times_;
	trace_files_ = other.trace_files_;
	concurrency_definitions_ = other.concurrency_definitions_;
	calibration_settings_ = other.calibration_settings_;
	population_settings_ = other.population_settings_;
	costs_ = other.costs_;
	interventions_ = other.interventions_;
	template_key_map_ = other.template_key_map_;
	time_dependent_parameters_ = other.time_dependent_parameters_;
}

void Inputs::LoadSimulationParameters(const ticpp::Element &root_node)
{
	version_ = Version::FromString(root_node.FirstChildElement("inputVersion")->GetText());
	debug_level_ = std::stoi(root_node.FirstChildElement("debugLevel")->GetText());
	duration_ = std::stoi(root_node.FirstChildElement("timeLimitMth")->GetText());
	fixed_seed_ = std::stoi(root_node.FirstChildElement("fixedSeed")->GetText());
	month_of_1990_ = std::stoi(root_node.FirstChildElement("monthOf1990")->GetText());

	auto &life_expectancy_node = *root_node.FirstChildElement("lifeExpectancyOutput");
	for(int i = 0; i < 5; i++)
	{
		auto time = std::stoi(life_expectancy_node.FirstChildElement("time" + std::to_string(i + 1))->GetText());
		life_expectancy_record_times_.push_back(time);
	}
	life_expectancy_median_condfidence_interval_ = 
		std::stod(life_expectancy_node.FirstChildElement("medianCI")->GetText());

	auto &partner_acq_output_node = *root_node.FirstChildElement("partnerAcqOutput");
	for(int i = 0; i < 5; i++)
	{
		auto time = std::stoi(partner_acq_output_node.FirstChildElement("time" + std::to_string(i + 1))->GetText());
		partner_acquisition_record_times_.push_back(time);
	}

	auto &concurrency_node = *root_node.FirstChildElement("concurrencyDefinition");
	for(int i = 0; i < 16; i++)
	{
		auto &def_node = *concurrency_node.FirstChildElement("def" + std::to_string(i));
		concurrency_definitions_[i].allow = std::stoi(def_node.FirstChildElement("allow")->GetText()) != 0;
		concurrency_definitions_[i].minimum_needed = std::stoi(def_node.FirstChildElement("minNeeded")->GetText());
	}
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
		{TraceFile::Type::Infection, "infection"},
		{TraceFile::Type::Partnership, "partnership"}, 
		{TraceFile::Type::Survival, "survival"}, 
		{TraceFile::Type::CostEffectiveness, "costEffectiveness"}, 
		{TraceFile::Type::Clinical, "clinical"}, 
		{TraceFile::Type::Events, "events"}, 
		{TraceFile::Type::Health, "health"}, 
		{TraceFile::Type::SinglePerson, "singleperson"}, 
		{TraceFile::Type::LifeExpectancy, "le"}, 
		{TraceFile::Type::PartnerAcquisition, "partacq"},
		{TraceFile::Type::CalibrationStatistics, "calibStats"},
		{TraceFile::Type::ArtRollout, "artRollout"},
		{TraceFile::Type::ShiftedOutcomes, "shiftedOutcomes"}
	};

	for(auto type : enum_iterator<TraceFile::Type>())
	{
		TraceFile trace_file;
		trace_file.type = type;
		std::string node_value = type_strings.at(type);
		trace_file.enabled = write_trace_node.FirstChildElement(node_value)->GetText() != "0";
		trace_file.extension = extension_names_node.FirstChildElement(node_value)->GetText();
		trace_file.toss = toss_files_node.FirstChildElement("traceFile" + std::to_string(int(type)))->GetText() != "0";
		trace_files_[type] = trace_file;
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

		auto extension = trace_file_key_value.second.extension.substr(trace_file_key_value.second.extension.length() - 4);
		if(extension != ".xls" && extension != ".txt")
		{
			throw std::runtime_error("trace file should have extension .xls");
		}
	}
}

void Inputs::LoadCalibrationSettings(const ticpp::Element &calibration_node)
{
	calibration_settings_.enabled = calibration_node.FirstChildElement("useCalibration")->GetText() != "0";
	if(calibration_settings_.enabled)
	{
		throw std::runtime_error("reading of calibration inputs is not yet implemented");
	}
}

void Inputs::ValidateCalibrationSettings()
{

}

void Inputs::LoadPopulationSettings(const ticpp::Element &population_node)
{
	population_settings_.id = population_node.FirstChildElement("id")->GetText();

	auto &initial_state_node = *population_node.FirstChildElement("initialState");

	population_settings_.initial_size = 
		std::stoi(initial_state_node.FirstChildElement("size")->GetText());
	population_settings_.prevalence_delay = 
		std::stoi(initial_state_node.FirstChildElement("delay")->GetText());

	auto &age_distribution_node = *initial_state_node.FirstChildElement("ageDistributionYrs");
	auto range_node = age_distribution_node.FirstChildElement("range");

	while(range_node != nullptr)
	{
		PopulationSettings::AgeDistributionStratum stratum;

		stratum.lower = std::stoi(range_node->FirstChildElement("minAge")->GetText());
		stratum.upper = std::stoi(range_node->FirstChildElement("maxAge")->GetText());
		stratum.male_distribution = 
			std::stod(range_node->FirstChildElement("distribMale")->GetText());
		stratum.female_distribution = 
			std::stod(range_node->FirstChildElement("distribFemale")->GetText());
		stratum.num_infected_male_csw = 
			std::stoi(range_node->FirstChildElement("numInfectedMaleCSW")->GetText());
		stratum.num_infected_male_high_risk = 
			std::stoi(range_node->FirstChildElement("numInfectedMaleHighRisk")->GetText());
		stratum.num_infected_male_low_risk = 
			std::stoi(range_node->FirstChildElement("numInfectedMaleLowRisk")->GetText());
		stratum.num_infected_female_csw = 
			std::stoi(range_node->FirstChildElement("numInfectedFemaleCSW")->GetText());
		stratum.num_infected_female_high_risk = 
			std::stoi(range_node->FirstChildElement("numInfectedFemaleHighRisk")->GetText());
		stratum.num_infected_female_low_risk = 
			std::stoi(range_node->FirstChildElement("numInfectedFemaleLowRisk")->GetText());

		population_settings_.age_distributions.push_back(stratum);

		range_node = range_node->NextSiblingElement("range", false);
	}

	population_settings_.csw_end_age_male = 
		std::stoi(initial_state_node.FirstChildElement("CSWEndAgeMale")->GetText());
	population_settings_.csw_end_age_female = 
		std::stoi(initial_state_node.FirstChildElement("CSWEndAgeFemale")->GetText());
	population_settings_.initial_chance_csw_male = 
		std::stod(initial_state_node.FirstChildElement("chanceBeingCSWMale")->GetText());
	population_settings_.initial_chance_csw_female = 
		std::stod(initial_state_node.FirstChildElement("chanceBeingCSWFemale")->GetText());

	population_settings_.birth_rate = 
		std::stod(ExtractPossibleTemplateParameter(TemplateParameter::BirthRate, population_node.FirstChildElement("birthRate")->GetText(), template_key_map_));
	population_settings_.proportion_male = 
		std::stod(ExtractPossibleTemplateParameter(TemplateParameter::ProportionMale, population_node.FirstChildElement("proportionMale")->GetText(), template_key_map_));
	population_settings_.proportion_circumcised = 
		std::stod(ExtractPossibleTemplateParameter(TemplateParameter::ProportionCircumcised, population_node.FirstChildElement("proportionCircumcised")->GetText(), template_key_map_));
	population_settings_.age_sexual_debut = 
		std::stoi(ExtractPossibleTemplateParameter(TemplateParameter::AgeSexualDebutYears, population_node.FirstChildElement("ageSexualDebutYrs")->GetText(), template_key_map_));
		

	auto &base_entities_node = *population_node.FirstChildElement("entityTypes")->FirstChildElement("baseEntities");

	auto entity_node = base_entities_node.FirstChildElement("baseEntity");
	while(entity_node != nullptr)
	{
		auto type = entity_node->FirstChildElement("type")->GetText();
		if(type == "Male")
		{
			auto &male_settings = population_settings_.male_settings;
			auto &behavior_node = *entity_node->FirstChildElement("behavior");

			male_settings.chance_become_sex_worker = std::stod(behavior_node.FirstChildElement("chanceBecomeSexWorker")->GetText());
			male_settings.partner_acquisition_multiplier_with_steady_high = std::stod(behavior_node.FirstChildElement("partnerAcqMultWithSteadyHighRisk")->GetText());
			male_settings.partner_acquisition_multiplier_with_steady_low = std::stod(behavior_node.FirstChildElement("partnerAcqMultWithSteadyLowRisk")->GetText());
			male_settings.enable_high_risk_multiplier = behavior_node.FirstChildElement("UseHighRiskMultiplier")->GetText() != "0";
			male_settings.high_risk_multiplier = std::stod(behavior_node.FirstChildElement("HighRiskAcqRateMultiplier")->GetText());
			male_settings.enable_csw_high_risk_multiplier = behavior_node.FirstChildElement("UseCSWHighRiskMultiplier")->GetText() != "0";
			male_settings.csw_high_risk_multiplier = std::stod(behavior_node.FirstChildElement("CSWHighRiskAcqRateMultiplier")->GetText());
			male_settings.use_coefficient_variation = std::stoi(behavior_node.FirstChildElement("heterogeneity")->FirstChildElement("varMethod")->GetText()) == 0;
			male_settings.coefficient_of_variation = std::stod(behavior_node.FirstChildElement("heterogeneity")->FirstChildElement("coeffVar")->GetText());

			auto &partnership_types_node = *behavior_node.FirstChildElement("partnershipTypes");
			auto partnership_node = partnership_types_node.FirstChildElement("partnership");

			while(partnership_node != nullptr)
			{
				auto partnership_type_string = partnership_node->FirstChildElement("type")->GetText();

				auto matching_type = std::find_if(SexualPartnership::TypeStrings.begin(), SexualPartnership::TypeStrings.end(), 
				[&](std::pair<SexualPartnership::Type, std::string> a)
				{ 
					return a.second == partnership_type_string; 
				});
				if(matching_type == SexualPartnership::TypeStrings.end())
				{
					throw std::runtime_error("unknown partnership type: " + partnership_type_string);
				}

				PopulationSettings::MaleSettings::PartnershipSettings settings;
				settings.type = matching_type->first;

				settings.acquisition_rate_high_risk = ParseLogNormalDistribution(*partnership_node->FirstChildElement("acquisitionRateHighRisk"), 
					male_settings.use_coefficient_variation, male_settings.coefficient_of_variation);
				settings.acquisition_rate_low_risk = ParseLogNormalDistribution(*partnership_node->FirstChildElement("acquisitionRateLowRisk"),
					male_settings.use_coefficient_variation, male_settings.coefficient_of_variation);

				auto &selection_criteria_node = *partnership_node->FirstChildElement("selectionCriteria");
				auto &available_buckets_node = *selection_criteria_node.FirstChildElement("availableBuckets");
				auto bucket_node = available_buckets_node.FirstChildElement("bucket");

				while(bucket_node != nullptr)
				{
					auto dmg_profile = bucket_node->FirstChildElement("DmgProfile")->GetText();
					auto weighted_value = std::stod(bucket_node->FirstChildElement("weightedValue")->GetText());
					settings.available_buckets[dmg_profile] = weighted_value;
					bucket_node = bucket_node->NextSiblingElement("bucket", false);
				}

				settings.average_years_younger = ParseNormalDistribution(*selection_criteria_node.FirstChildElement("AverageYearsYounger"));
				settings.coital_events_per_month_high_risk = ParsePoissonDistribution(*partnership_node->FirstChildElement("coitalEventsPerMonthHighRisk"));
				settings.chance_condom_user_per_event_high_risk = ParseBetaDistribution(*partnership_node->FirstChildElement("chanceCondomUsePerEventHighRisk"),
					male_settings.use_coefficient_variation, male_settings.coefficient_of_variation);
				settings.partnership_duration_months_high_risk = ParseShiftedLogNormalDistribution(*partnership_node->FirstChildElement("partnershipDurationMthHighRisk"));
				settings.coital_events_per_month_low_risk = ParsePoissonDistribution(*partnership_node->FirstChildElement("coitalEventsPerMonthLowRisk"));
				settings.chance_condom_user_per_event_low_risk = ParseBetaDistribution(*partnership_node->FirstChildElement("chanceCondomUsePerEventLowRisk"),
					male_settings.use_coefficient_variation, male_settings.coefficient_of_variation);
				settings.partnership_duration_months_low_risk = ParseShiftedLogNormalDistribution(*partnership_node->FirstChildElement("partnershipDurationMthLowRisk"));

				population_settings_.male_settings.partnership_settings[matching_type->first] = settings;

				partnership_node = partnership_node->NextSiblingElement("partnership", false);
			}

			male_settings.activity_level = ParseNormalDistribution(*behavior_node.FirstChildElement("activityLevel"));
			male_settings.proportion_high_risk_csw = std::stod(behavior_node.FirstChildElement("proportionHighRiskCSW")->GetText());
			male_settings.proportion_high_risk_non_csw = std::stod(behavior_node.FirstChildElement("proportionHighRiskNonCSW")->GetText());

			male_settings.age_discounting_start_age = std::stoi(behavior_node.FirstChildElement("ageDiscounting")->FirstChildElement("startAgeYrs")->GetText());
			male_settings.acquisition_rate_discounting_yearly = std::stod(behavior_node.FirstChildElement("ageDiscounting")->FirstChildElement("acquisitionDiscByYr")->GetText());
			male_settings.coital_acts_discounting_yearly = std::stod(behavior_node.FirstChildElement("ageDiscounting")->FirstChildElement("coitalActsDiscByYr")->GetText());

			auto &health_node = *entity_node->FirstChildElement("health");
			male_settings.circumcision_protection_efficacy = std::stod(health_node.FirstChildElement("circumcisionProtectEfficacy")->GetText());
			male_settings.condom_protection_efficacy = std::stod(health_node.FirstChildElement("condomProtectEfficacy")->GetText());
			auto &transmission_coefficients_node = *health_node.FirstChildElement("transmissionCoefficients");
			auto transmission_coeffiecients = transmission_coefficients_node.FirstChildElement("valsByHVL")->GetText();
			std::stringstream ss(transmission_coeffiecients);
			for(int i = 0; i < 7; i++)
			{
				ss >> male_settings.transmission_coefficients[i];
			}
			male_settings.transmission_coefficients[7] = std::stod(transmission_coefficients_node.FirstChildElement("primary")->GetText());
			male_settings.transmission_coefficients[8] = std::stod(transmission_coefficients_node.FirstChildElement("lateStage")->GetText());
		}
		else if(type == "Female")
		{
			auto &female_settings = population_settings_.female_settings;
			auto &behavior_node = *entity_node->FirstChildElement("behavior");
			female_settings.chance_become_sex_worker = std::stod(behavior_node.FirstChildElement("chanceBecomeSexWorker")->GetText());
			female_settings.proportion_high_risk_csw = std::stod(behavior_node.FirstChildElement("proportionHighRiskCSW")->GetText());
			female_settings.proportion_high_risk_non_csw = std::stod(behavior_node.FirstChildElement("proportionHighRiskNonCSW")->GetText());
			female_settings.activity_level = ParseNormalDistribution(*behavior_node.FirstChildElement("activityLevel"));
			auto &transmission_coefficients_node = *entity_node->FirstChildElement("health")->FirstChildElement("transmissionCoefficients");
			auto transmission_coeffiecients = transmission_coefficients_node.FirstChildElement("valsByHVL")->GetText();
			std::stringstream ss(transmission_coeffiecients);
			for(int i = 0; i < 7; i++)
			{
				ss >> female_settings.transmission_coefficients[i];
			}
			female_settings.transmission_coefficients[7] = std::stod(transmission_coefficients_node.FirstChildElement("primary")->GetText());
			female_settings.transmission_coefficients[8] = std::stod(transmission_coefficients_node.FirstChildElement("lateStage")->GetText());
		}
		else
		{
			throw std::runtime_error("unknown entity type");
		}

		entity_node = entity_node->NextSiblingElement("baseEntity", false);
	}

	// TODO: assortativeness should be inside partnership type
	auto &assortativeness_node = *population_node.FirstChildElement("assortativeness");
	population_settings_.male_settings.partnership_settings[SexualPartnership::Type::Steady].assortativeness = 
		std::stod(assortativeness_node.FirstChildElement("steady")->GetText());
	population_settings_.male_settings.partnership_settings[SexualPartnership::Type::Regular].assortativeness =
		std::stod(assortativeness_node.FirstChildElement("regular")->GetText());
	population_settings_.male_settings.partnership_settings[SexualPartnership::Type::Casual].assortativeness =
		std::stod(assortativeness_node.FirstChildElement("casual")->GetText());
	population_settings_.male_settings.partnership_settings[SexualPartnership::Type::Csw].assortativeness =
		std::stod(assortativeness_node.FirstChildElement("csw")->GetText());
}

void Inputs::ValidatePopulationSettings()
{

}

void Inputs::LoadCosts(const ticpp::Element &costs_node)
{
	costs_.condom_cost = std::stod(costs_node.FirstChildElement("condomCost")->GetText());
	costs_.circumcision_cost = std::stod(costs_node.FirstChildElement("circumcisionCost")->GetText());
}

void Inputs::ValidateCosts()
{

}

void Inputs::LoadInterventions(const ticpp::Element &interventions_node)
{
	auto &art_rollout_node = *interventions_node.FirstChildElement("artRolloutIntervention");
	interventions_.use_art_rollout = art_rollout_node.FirstChildElement("useRollout")->GetText() != "0";

	if(interventions_.use_art_rollout)
	{
		auto &rollout_files_node = *art_rollout_node.FirstChildElement("rolloutTreatmentFiles");
		auto rollout_file_node = rollout_files_node.FirstChildElement("rolloutFile");

		for(auto &current_file : interventions_.rollout_treatment_files)
		{
			if(rollout_file_node == nullptr)
			{
				throw std::runtime_error("there should be 13 files under rolloutTreatmentFiles");
			}

			current_file.time = std::stoi(rollout_file_node->FirstChildElement("time")->GetText());

			if(current_file.time >= 0)
			{
				current_file.file_name = rollout_file_node->FirstChildElement("fileName")->GetText();
				current_file.file_number = std::stoi(rollout_file_node->FirstChildElement("fileNumber")->GetText());
				current_file.target_population = std::stoi(rollout_file_node->FirstChildElement("popToApply")->GetText());
			}

			rollout_file_node = rollout_file_node->NextSiblingElement(false);
		}

		auto &target_proportions_node = *art_rollout_node.FirstChildElement("targetRolloutProportions");
		auto target_node = target_proportions_node.FirstChildElement("target");

		while(target_node != nullptr)
		{
			auto year = std::stoi(target_node->GetAttribute("year"));
			auto proportion = std::stod(target_node->GetText());
			interventions_.target_rollout_proportions[year] = proportion;
			target_node = target_node->NextSiblingElement("target", false);
		}
	}
	else
	{
		auto &cepac_intervention_node = *interventions_node.FirstChildElement("cepacIntervention");
		auto &cepac_files_node = *cepac_intervention_node.FirstChildElement("cepacTreatmentFiles");
		auto &cepac_file_time_node = *cepac_intervention_node.FirstChildElement("cohortTimes")->FirstChildElement("cohort");
		auto cepac_file_node = cepac_intervention_node.FirstChildElement("treatmentFile");

		for(auto &current_file : interventions_.cepac_treatment_files)
		{
			if(cepac_file_node == nullptr)
			{
				throw std::runtime_error("there should be 5 files under cepacTreatmentFiles");
			}

			current_file.file_name = cepac_file_node->FirstChildElement("fileName")->GetText();
			current_file.file_number = std::stoi(cepac_file_node->FirstChildElement("fileNumber")->GetText());

			if(current_file.file_number > 0)
			{
				current_file.time = std::stoi(cepac_file_time_node.FirstChildElement("time" + std::to_string(current_file.file_number))->GetText());
			}

			cepac_file_node = cepac_file_node->NextSiblingElement(false);
		}
	}

	auto &miscellaneous_interventions = *interventions_node.FirstChildElement("miscellaneousInterventions");
	auto current_intervention = miscellaneous_interventions.FirstChildElement(false);

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

		unmatched_parameters_.push_back(param);
		
		current_intervention = current_intervention->NextSiblingElement(false);
	}
}

void Inputs::ValidateInterventions()
{

}