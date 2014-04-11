#pragma once

#include <boost/filesystem.hpp>

#include "Simulation.h"
#include "cepacbridge/CepacInputParser.h"
#include "entities/behaviors/SexualBehaviorParams.h"
#include "util/xml/pugixml.hpp"
#include "util/enum_iterator.h"

class SimulationBuilder
{
public:
    void Create(const std::string &filename, Simulation &sim);

private:
    template<typename T>
    T Get(const std::string &path);

    template<typename T>
    void GetTargetedTemplate(const std::string &path, T &result, std::function<void(Person *, T)> callback);

    template<typename T>
    void GetTemplate(const std::string &path, T &result);

    std::pair<std::string, std::string> ExtractParameter(const std::string &path);
    void GetTargetedTemplateHack(const std::string &path, double &result, std::function<void(Person *, double)> callback);
    std::string to_string(SexualPartnership::Type type);
    void Get(const std::string &base_path, EventParams::RolloutEligibility &eligibility);
    void BuildPopulation(const std::string &path, Population &population);

    pugi::xml_node root_;
    std::unordered_map<std::string, std::vector<Simulation::TimeDependentParameter>> time_dependent_parameters_;
};

template<>
int SimulationBuilder::Get(const std::string &path)
{
    return root_.select_single_node(path.c_str()).node().text().as_int();
}

template<>
bool SimulationBuilder::Get(const std::string &path)
{
    return Get<int>(path) != 0;
}

template<>
double SimulationBuilder::Get(const std::string &path)
{
    return root_.select_single_node(path.c_str()).node().text().as_double();
}

template<>
std::string SimulationBuilder::Get(const std::string &path)
{
    return root_.select_single_node(path.c_str()).node().text().as_string();
}


template<>
void SimulationBuilder::GetTargetedTemplate(const std::string &base_path, NormalDist &result, std::function<void(Person *, NormalDist)> callback)
{
    auto extracted_mean = ExtractParameter(base_path + "/Distrib/mean");
    auto extracted_stddev = ExtractParameter(base_path + "/Distrib/stdDev");

    if(extracted_mean.first != "")
    {
	if(extracted_stddev.first == "")
	{
	    throw std::runtime_error("must set mean and stddev at the same time");
	}

	auto match_mean = time_dependent_parameters_.find(extracted_mean.first);
	auto match_stddev = time_dependent_parameters_.find(extracted_stddev.first);

	if(match_mean == time_dependent_parameters_.end())
	{
	    throw std::runtime_error("Unmatched parameter");
	}

	if(match_stddev == time_dependent_parameters_.end())
	{
	    throw std::runtime_error("must set mean and stddev at the same time");
	}

	for(auto &parameter_mean : match_mean->second)
	{
	    auto parameter_stddev = std::find_if(match_stddev->second.begin(), match_stddev->second.end(), [=](Simulation::TimeDependentParameter &p) { return parameter_mean.time == p.time; });
	    if(parameter_stddev == match_stddev->second.end())
	    {
		throw std::runtime_error("must set mean and stddev at the same time");
	    }
	    NormalDist future_value;
	    future_value.mean = std::stod(parameter_mean.value);
	    future_value.stddev = std::stod(parameter_stddev->value);
	    parameter_mean.population_modifier = [=](Person *p) { callback(p, future_value); };
	}
    }

    result.mean = std::stod(extracted_mean.second);
    result.stddev = std::stod(extracted_stddev.second);
}

template<>
void SimulationBuilder::GetTargetedTemplate(const std::string &base_path, BetaDist &result, std::function<void(Person *, BetaDist)> callback)
{
    auto extracted_mean = ExtractParameter(base_path + "/Distrib/mean");
    auto extracted_stddev = ExtractParameter(base_path + "/Distrib/stdDev");

    if(extracted_mean.first != "")
    {
	if(extracted_stddev.first == "")
	{
	    throw std::runtime_error("must set mean and stddev at the same time");
	}

	auto match_mean = time_dependent_parameters_.find(extracted_mean.first);
	auto match_stddev = time_dependent_parameters_.find(extracted_stddev.first);

	if(match_mean == time_dependent_parameters_.end())
	{
	    throw std::runtime_error("Unmatched parameter");
	}

	if(match_stddev == time_dependent_parameters_.end())
	{
	    throw std::runtime_error("must set mean and stddev at the same time");
	}

	for(auto &parameter_mean : match_mean->second)
	{
	    auto parameter_stddev = std::find_if(match_stddev->second.begin(), match_stddev->second.end(), [=](Simulation::TimeDependentParameter &p) { return parameter_mean.time == p.time; });
	    if(parameter_stddev == match_stddev->second.end())
	    {
		throw std::runtime_error("must set mean and stddev at the same time");
	    }
	    auto future_mean = std::stod(parameter_mean.value);
	    auto future_stddev = std::stod(parameter_stddev->value);
	    double future_ss = future_mean * (1 - future_mean) / (future_stddev * future_stddev) - 1;
	    BetaDist future_value;
	    future_value.alpha = future_mean * future_ss;
	    future_value.beta = (1 - future_mean) * future_ss;
	    parameter_mean.population_modifier = [=](Person *p) { callback(p, future_value); };
	    parameter_stddev->population_modifier = [=](Person *p) { callback(p, future_value); };
	}
    }

    double sampleSize = std::stod(extracted_mean.second) * (1 - std::stod(extracted_mean.second)) / (std::stod(extracted_stddev.second) * std::stod(extracted_stddev.second)) - 1;

    result.alpha = std::stod(extracted_mean.second) * sampleSize;
    result.beta = (1 - std::stod(extracted_mean.second)) * sampleSize;
}

template<>
void SimulationBuilder::GetTargetedTemplate(const std::string &base_path, LogNormalDist &result, std::function<void(Person *, LogNormalDist)> callback)
{
    auto extracted_mean = ExtractParameter(base_path + "/Distrib/mean");
    auto extracted_stddev = ExtractParameter(base_path + "/Distrib/stdDev");

    if(extracted_mean.first != "")
    {
	if(extracted_stddev.first == "")
	{
	    throw std::runtime_error("must set mean and stddev at the same time");
	}

	auto match_mean = time_dependent_parameters_.find(extracted_mean.first);
	auto match_stddev = time_dependent_parameters_.find(extracted_stddev.first);

	if(match_mean == time_dependent_parameters_.end())
	{
	    throw std::runtime_error("Unmatched parameter");
	}

	if(match_stddev == time_dependent_parameters_.end())
	{
	    throw std::runtime_error("must set mean and stddev at the same time");
	}

	for(auto &parameter_mean : match_mean->second)
	{
	    auto parameter_stddev = std::find_if(match_stddev->second.begin(), match_stddev->second.end(), [=](Simulation::TimeDependentParameter &p) { return parameter_mean.time == p.time; });
	    if(parameter_stddev == match_stddev->second.end())
	    {
		throw std::runtime_error("must set mean and stddev at the same time");
	    }
	    LogNormalDist future_value;
	    future_value.mu = std::stod(parameter_mean.value);
	    future_value.sigma = std::stod(parameter_stddev->value);
	    parameter_mean.population_modifier = [=](Person *p) { callback(p, future_value); };
	    parameter_stddev->population_modifier = [=](Person *p) { callback(p, future_value); };
	}
    }

    if(std::stod(extracted_mean.second) <= 0)
    {
	result.mu = 0;
	result.sigma = 0;
	result.isZeroDistrib = true;
    }
    else
    {
	result.mu = log(std::stod(extracted_mean.second)) - 0.5 * log(1 + (std::stod(extracted_stddev.second) * std::stod(extracted_stddev.second)) / (std::stod(extracted_mean.second) * std::stod(extracted_mean.second)));
	result.sigma = sqrt(log(1 + (std::stod(extracted_stddev.second) * std::stod(extracted_stddev.second)) / (std::stod(extracted_mean.second) * std::stod(extracted_mean.second))));
    }
}

template<>
void SimulationBuilder::GetTargetedTemplate(const std::string &base_path, ShiftedLogNormalDist &result, std::function<void(Person *, ShiftedLogNormalDist)> callback)
{
    auto extracted_mean = ExtractParameter(base_path + "/Distrib/mean");
    auto extracted_stddev = ExtractParameter(base_path + "/Distrib/stdDev");
    auto extracted_shift = ExtractParameter(base_path + "/Distrib/shift");

    if(extracted_mean.first != "")
    {
	if(extracted_stddev.first == "" || extracted_shift.first == "")
	{
	    throw std::runtime_error("must set mean and stddev at the same time");
	}

	auto match_mean = time_dependent_parameters_.find(extracted_mean.first);
	auto match_stddev = time_dependent_parameters_.find(extracted_stddev.first);
	auto match_shift = time_dependent_parameters_.find(extracted_shift.first);

	if(match_mean == time_dependent_parameters_.end())
	{
	    throw std::runtime_error("Unmatched parameter");
	}

	if(match_stddev == time_dependent_parameters_.end() || match_shift == time_dependent_parameters_.end())
	{
	    throw std::runtime_error("must set mean, stddev, and shift at the same time");
	}

	for(auto &parameter_mean : match_mean->second)
	{
	    auto parameter_stddev = std::find_if(match_stddev->second.begin(), match_stddev->second.end(), [=](Simulation::TimeDependentParameter &p) { return parameter_mean.time == p.time; });
	    auto parameter_shift = std::find_if(match_shift->second.begin(), match_shift->second.end(), [=](Simulation::TimeDependentParameter &p) { return parameter_mean.time == p.time; });

	    if(parameter_stddev == match_stddev->second.end() || parameter_shift == match_shift->second.end())
	    {
		throw std::runtime_error("must set mean, stddev, and shift at the same time");
	    }

	    ShiftedLogNormalDist future_value;
	    if(std::stod(parameter_mean.value) <= 0)
	    {
		future_value.mu = 0;
		future_value.sigma = 0;
		future_value.shift = 0;
		future_value.isZeroDistrib = true;
	    }
	    else
	    {
		future_value.shift = std::stod(parameter_shift->value);
		future_value.mu = log(std::stod(parameter_mean.value) - result.shift) - 0.5 * log(1 + (std::stod(parameter_stddev->value) * std::stod(parameter_shift->value)) / ((std::stod(parameter_mean.value) - result.shift) * (std::stod(parameter_mean.value) - result.shift)));
		future_value.sigma = sqrt(log(1 + (std::stod(parameter_shift->value) * std::stod(parameter_shift->value)) / ((std::stod(parameter_mean.value) - result.shift) * (std::stod(parameter_mean.value) - result.shift))));
	    }
	    parameter_mean.population_modifier = [=](Person *p) { callback(p, future_value); };
	    parameter_stddev->population_modifier = [=](Person *p) { callback(p, future_value); };
	}
    }

    if(std::stod(extracted_mean.second) <= 0)
    {
	result.mu = 0;
	result.sigma = 0;
	result.shift = 0;
	result.isZeroDistrib = true;
    }
    else
    {
	result.shift = std::stod(extracted_shift.second);
	result.mu = log(std::stod(extracted_mean.second) - result.shift) - 0.5 * log(1 + (std::stod(extracted_stddev.second) * std::stod(extracted_stddev.second)) / ((std::stod(extracted_mean.second) - result.shift) * (std::stod(extracted_mean.second) - result.shift)));
	result.sigma = sqrt(log(1 + (std::stod(extracted_stddev.second) * std::stod(extracted_stddev.second)) / ((std::stod(extracted_mean.second) - result.shift) * (std::stod(extracted_mean.second) - result.shift))));
    }
}

template<>
void SimulationBuilder::GetTargetedTemplate(const std::string &path, int &result, std::function<void(Person *, int)> callback)
{
    auto extracted = ExtractParameter(path);

    if(extracted.first != "")
    {
	auto match = time_dependent_parameters_.find(extracted.first);

	if(match == time_dependent_parameters_.end())
	{
	    throw std::runtime_error("Unmatched parameter");
	}

	for(auto &parameter : match->second)
	{
	    auto future_value = std::stoi(parameter.value);
	    parameter.population_modifier = [=](Person *p) { callback(p, future_value); };
	}
    }

    result = std::stoi(extracted.second);
}

template<>
void SimulationBuilder::GetTemplate(const std::string &path, bool &result)
{
    auto extracted = ExtractParameter(path);

    if(extracted.first != "")
    {
	auto match = time_dependent_parameters_.find(extracted.first);

	if(match == time_dependent_parameters_.end())
	{
	    throw std::runtime_error("Unmatched parameter");
	}

	for(auto &parameter : match->second)
	{
	    auto future_value = std::stoi(parameter.value) != 0;
	    parameter.simulation_modifier = [=, &result]() { result = future_value; };
	}
    }

    result = std::stoi(extracted.second) != 0;
}

template<>
void SimulationBuilder::GetTemplate(const std::string &path, int &result)
{
    auto extracted = ExtractParameter(path);

    if(extracted.first != "")
    {
	auto match = time_dependent_parameters_.find(extracted.first);

	if(match == time_dependent_parameters_.end())
	{
	    throw std::runtime_error("Unmatched parameter");
	}

	for(auto &parameter : match->second)
	{
	    auto future_value = std::stoi(parameter.value);

	    if(parameter.target_population.has_value)
	    {
		throw std::runtime_error("shouldn't have population specified, this is a simulation parameter");
	    }

	    parameter.simulation_modifier = [=, &result]() { result = future_value; };
	}
    }

    result = std::stoi(extracted.second);
}

template<>
void SimulationBuilder::GetTemplate(const std::string &path, double &result)
{
    auto extracted = ExtractParameter(path);

    if(extracted.first != "")
    {
	auto match = time_dependent_parameters_.find(extracted.first);

	if(match == time_dependent_parameters_.end())
	{
	    throw std::runtime_error("Unmatched parameter");
	}

	for(auto &parameter : match->second)
	{
	    auto future_value = std::stod(parameter.value);
	    parameter.simulation_modifier = [=, &result]() { result = future_value; };
	}
    }

    result = std::stod(extracted.second);
}

template<>
void SimulationBuilder::GetTemplate(const std::string &path, std::array<double, 9> &result)
{
    auto extracted = ExtractParameter(path);

    if(extracted.first != "")
    {
	auto match = time_dependent_parameters_.find(extracted.first);

	if(match == time_dependent_parameters_.end())
	{
	    throw std::runtime_error("Unmatched parameter");
	}

	for(auto &parameter : match->second)
	{
	    std::stringstream ss(parameter.value);
	    for(int i = 0; i < 7; i++)
	    {
		double future_value;
		ss >> future_value;
		parameter.simulation_modifier = [=, &result]() { result[i] = future_value; };
	    }
	}
    }

    std::stringstream ss(extracted.second);
    for(int i = 0; i < 7; i++)
    {
	double value;
	ss >> value;
	result[i] = value;
    }
}

template<>
SexualBehaviorParams SimulationBuilder::Get(const std::string &base_path)
{
    SexualBehaviorParams result;

    std::string type_string = Get<std::string>(base_path + "/type");
    if(type_string == "CSW") result.partnershipType = SexualPartnership::Type::Csw;
    if(type_string == "Casual") result.partnershipType = SexualPartnership::Type::Casual;
    if(type_string == "Regular") result.partnershipType = SexualPartnership::Type::Regular;
    if(type_string == "Steady") result.partnershipType = SexualPartnership::Type::Steady;

    GetTargetedTemplate<LogNormalDist>(base_path + "/acquisitionRateLowRisk", result.acquisitionRatePerMonth[Person::LOW],
				       std::bind(&Person::SetAcquisitionRatePerMonth, std::placeholders::_1, Person::LOW, result.partnershipType, std::placeholders::_2));
    GetTargetedTemplate<LogNormalDist>(base_path + "/acquisitionRateHighRisk", result.acquisitionRatePerMonth[Person::HIGH],
				       std::bind(&Person::SetAcquisitionRatePerMonth, std::placeholders::_1, Person::LOW, result.partnershipType, std::placeholders::_2));

    auto path = base_path + "/selectionCriteria/availableBuckets/bucket";
    for(const auto &bucket_settings : root_.select_nodes(path.c_str()))
    {
	SexualBehaviorParams::AvailableBucket bucket;
	bucket.dmgProfileSelector.parse(bucket_settings.node().child("DmgProfile").text().as_string());
	bucket.weight = bucket_settings.node().child("weightedValue").text().as_double();
	result.availableBuckets.push_back(bucket);
    }

    GetTargetedTemplate<NormalDist>(base_path + "/selectionCriteria/AverageYearsYounger", result.averageYearsYounger,
				    std::bind(&Person::SetAverageYearsYounger, std::placeholders::_1, result.partnershipType, std::placeholders::_2));

    //XXX:this should be a double, but old implementations mistakenly casted it to int
    //we will continue to do this to maintain reproduciblity for now
    GetTargetedTemplateHack(base_path + "/coitalEventsPerMonthLowRisk/Distrib/mean", 
			    result.coitalEventsPerMonth[Person::LOW], 
			    std::bind(&Person::SetCoitalEventsPerMonth, std::placeholders::_1, Person::LOW, result.partnershipType, std::placeholders::_2));
    GetTargetedTemplateHack(base_path + "/coitalEventsPerMonthHighRisk/Distrib/mean",
			    result.coitalEventsPerMonth[Person::HIGH],
			    std::bind(&Person::SetCoitalEventsPerMonth, std::placeholders::_1, Person::HIGH, result.partnershipType, std::placeholders::_2));

    GetTargetedTemplate<BetaDist>(base_path + "/chanceCondomUsePerEventLowRisk",
				  result.chanceCondomUsePerEvent[Person::LOW],
				  std::bind(&Person::SetChanceCondomUsePerEvent, std::placeholders::_1, Person::LOW, result.partnershipType, std::placeholders::_2));
    GetTargetedTemplate<BetaDist>(base_path + "/chanceCondomUsePerEventHighRisk",
				  result.chanceCondomUsePerEvent[Person::HIGH],
				  std::bind(&Person::SetChanceCondomUsePerEvent, std::placeholders::_1, Person::HIGH, result.partnershipType, std::placeholders::_2));

    GetTargetedTemplate<ShiftedLogNormalDist>(base_path + "/partnershipDurationMthLowRisk", 
					      result.partnershipDurationMth[Person::LOW],
					      std::bind(&Person::SetPartnershipDuration, std::placeholders::_1, Person::LOW, result.partnershipType, std::placeholders::_2));
    GetTargetedTemplate<ShiftedLogNormalDist>(base_path + "/partnershipDurationMthHighRisk", 
					      result.partnershipDurationMth[Person::HIGH],
					      std::bind(&Person::SetPartnershipDuration, std::placeholders::_1, Person::LOW, result.partnershipType, std::placeholders::_2));

    return result;
}

template<>
Male::SubPopParams SimulationBuilder::Get(const std::string &base_path)
{
    Male::SubPopParams result;

    GetTemplate(base_path + "/behavior/chanceBecomeSexWorker", result.chanceBecomeCSW);
    GetTemplate<double>(base_path + "/behavior/partnerAcqMultWithSteadyHighRisk", result.partnerAcqMultWithSteady[Person::HIGH]);
    GetTemplate<double>(base_path + "/behavior/partnerAcqMultWithSteadyLowRisk", result.partnerAcqMultWithSteady[Person::LOW]);

    bool use_high_risk_multiplier = Get<bool>(base_path + "/behavior/UseHighRiskMultiplier");
    double high_risk_multiplier = Get<double>(base_path + "/behavior/HighRiskAcqRateMultiplier");
    bool use_csw_high_risk_multiplier = Get<bool>(base_path + "/behavior/UseCSWHighRiskMultiplier");
    double csw_high_risk_multiplier = Get<double>(base_path + "/behavior/CSWHighRiskAcqRateMultiplier");

    //Coefficient of Variation
    result.useCoefficientVariation = Get<int>(base_path + "/behavior/heterogeneity/varMethod") == 0;
    result.coefficientOfVariation = Get<double>(base_path + "/behavior/heterogeneity/coeffVar");

    //iterate through each Person in partnershipTypes
    result.sexualBehaviorParams.clear();
    for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
    {
	auto path = base_path + "/behavior/partnershipTypes/partnership[type='" + to_string(partnership_type) + "']";
	auto params = Get<SexualBehaviorParams>(path);

	if(use_high_risk_multiplier)
	{
	    if(partnership_type == SexualPartnership::Type::Csw && use_csw_high_risk_multiplier)
	    {
		params.SetHighRiskMultiplier(csw_high_risk_multiplier);
	    }
	    else
	    {
		params.SetHighRiskMultiplier(high_risk_multiplier);
	    }
	}

	if(result.useCoefficientVariation)
	{
	    params.ApplyCoefficientVariation(result.coefficientOfVariation);
	}

	result.sexualBehaviorParams.push_back(params);
    }

    result.activityLevel.mean = Get<double>(base_path + "/behavior/activityLevel/Distrib/mean");
    result.activityLevel.stddev = Get<double>(base_path + "/behavior/activityLevel/Distrib/stdDev");

    GetTemplate(base_path + "/behavior/proportionHighRiskCSW", result.proportionHighRisk[DmgProfile::CSW]);
    GetTemplate(base_path + "/behavior/proportionHighRiskNonCSW", result.proportionHighRisk[DmgProfile::NON_CSW]);

    //saves partner acq rate and acts discounting
    GetTemplate(base_path + "/behavior/ageDiscounting/startAgeYrs", result.partneringDiscStartAgeYrs);
    GetTemplate(base_path + "/behavior/ageDiscounting/acquisitionDiscByYr", result.partneringAcqDiscPerYr);
    GetTemplate(base_path + "/behavior/ageDiscounting/coitalActsDiscByYr", result.partneringActsDiscPerYr);

    int numMults = Person::maxYrForDeathStats - result.partneringDiscStartAgeYrs + 1;
    double acqMult = 1 - result.partneringAcqDiscPerYr;
    double actsMult = 1 - result.partneringActsDiscPerYr;

    //generate vectors that contain discount multipliers. will cover from [partneringDiscStartAgeYrs,Person::maxYrForDeathStats]
    result.partneringAcqDiscMult.clear();
    result.partneringActsDiscMult.clear();
    result.partneringAcqDiscMult.push_back(acqMult);
    result.partneringActsDiscMult.push_back(actsMult);

    for(int i = 1; i < numMults; ++i)
    {
	result.partneringAcqDiscMult.push_back(result.partneringAcqDiscMult.at(i - 1)*acqMult);
	result.partneringActsDiscMult.push_back(result.partneringActsDiscMult.at(i - 1)*actsMult);
    }

    GetTemplate(base_path + "/health/circumcisionProtectEfficacy", result.circumProtectEff);
    GetTemplate(base_path + "/health/condomProtectEfficacy", result.condomProtectEff);

    GetTemplate(base_path + "/health/transmissionCoefficients/valsByHVL", result.transmitPerEventCoeffs);
    GetTemplate(base_path + "/health/transmissionCoefficients/primary", result.transmitPerEventCoeffs[7]);
    GetTemplate(base_path + "/health/transmissionCoefficients/lateStage", result.transmitPerEventCoeffs[8]);

    return result;
}

template<>
Female::SubPopParams SimulationBuilder::Get(const std::string &base_path)
{
    Female::SubPopParams result;

    auto behavior_path = base_path + "/behavior";
    GetTemplate(behavior_path + "/chanceBecomeSexWorker", result.chanceBecomeCSW);
    GetTemplate(behavior_path + "/proportionHighRiskNonCSW", result.proportionHighRisk[DmgProfile::NON_CSW]);
    GetTemplate(behavior_path + "/proportionHighRiskCSW", result.proportionHighRisk[DmgProfile::CSW]);

    result.activityLevel.mean = Get<double>(behavior_path + "/activityLevel/Distrib/mean");
    result.activityLevel.stddev = Get<double>(behavior_path + "/activityLevel/Distrib/stdDev");

    GetTemplate(base_path + "/health/transmissionCoefficients/valsByHVL", result.transmitPerEventCoeffs);
    GetTemplate(base_path + "/health/transmissionCoefficients/primary", result.transmitPerEventCoeffs[7]);
    GetTemplate(base_path + "/health/transmissionCoefficients/lateStage", result.transmitPerEventCoeffs[8]);

    return result;
}

template<>
PopulationParams SimulationBuilder::Get(const std::string &base_path)
{
    PopulationParams result;

    auto node = root_.select_single_node(base_path.c_str()).node();

    auto initial_state_node = node.child("initialState");
    result.initSize = initial_state_node.child("size").text().as_int();

    //get initial age distribution
    for(auto age_bucket_node : initial_state_node.child("ageDistributionYrs").children("range"))
    {
	result.initialAgeBuckets.emplace_back(
	    Util::convertTime(YEAR, MONTH, age_bucket_node.child("minAge").text().as_int()),
	    Util::convertTime(YEAR, MONTH, age_bucket_node.child("maxAge").text().as_int()) + 11,
	    age_bucket_node.child("distribMale").text().as_double(),
	    age_bucket_node.child("distribFemale").text().as_double(),
	    age_bucket_node.child("numInfectedMaleCSW").text().as_int(),
	    age_bucket_node.child("numInfectedFemaleCSW").text().as_int(),
	    age_bucket_node.child("numInfectedMaleLowRisk").text().as_int(),
	    age_bucket_node.child("numInfectedFemaleLowRisk").text().as_int(),
	    age_bucket_node.child("numInfectedMaleHighRisk").text().as_int(),
	    age_bucket_node.child("numInfectedFemaleHighRisk").text().as_int());
    }

    result.initProbCSW[DmgProfile::MALE] = initial_state_node.child("chanceBeingCSWMale").text().as_double();
    result.initProbCSW[DmgProfile::FEMALE] = initial_state_node.child("chanceBeingCSWFemale").text().as_double();
    result.CSWEndAgeMth[DmgProfile::MALE] = Util::convertTime(YEAR, MONTH, initial_state_node.child("CSWEndAgeMale").text().as_int());
    result.CSWEndAgeMth[DmgProfile::FEMALE] = Util::convertTime(YEAR, MONTH, initial_state_node.child("CSWEndAgeFemale").text().as_int());

    //normalize %population values for each age bucket
    double totalPopulationproportionages[DmgProfile::ENDGender];

    //get the total of proportionage values of AgeBucketPrevalencInfo.proportionOfPopulation
    for(int i = 0; i < DmgProfile::ENDGender; i++)
    {
	totalPopulationproportionages[i] = 0.0;

	for(size_t ageBucketNum = 0; ageBucketNum < result.initialAgeBuckets.size(); ageBucketNum++)
	{
	    totalPopulationproportionages[i] = totalPopulationproportionages[i] + result.initialAgeBuckets.at(
		ageBucketNum).proportionOfPopulation[i];
	}

	//normalize each proportionage value so that the sum of them == 1
	for(size_t ageBucketNum = 0; ageBucketNum < result.initialAgeBuckets.size(); ageBucketNum++)
	{
	    result.initialAgeBuckets.at(ageBucketNum).proportionOfPopulation[i] = result.initialAgeBuckets.at(
		ageBucketNum).proportionOfPopulation[i] / totalPopulationproportionages[i];
	}
    }

    //dmgProfile parameters
    GetTemplate<double>("/simulation/population/birthRate", result.birthRate);
    GetTemplate<double>("/simulation/population/proportionMale", result.proportionMale);
    GetTemplate<double>("/simulation/population/proportionCircumcised", result.circumcised);
    int ageSexualDebutYears = 0;
    GetTemplate<int>("/simulation/population/ageSexualDebutYrs", ageSexualDebutYears);
    result.SAEntAgeMths = Util::convertTime(YEAR, MONTH, ageSexualDebutYears);

    GetTemplate("/simulation/population/assortativeness/steady", result.assort[(int)SexualPartnership::Type::Steady]);
    GetTemplate("/simulation/population/assortativeness/regular", result.assort[(int)SexualPartnership::Type::Regular]);
    GetTemplate("/simulation/population/assortativeness/casual", result.assort[(int)SexualPartnership::Type::Casual]);
    GetTemplate("/simulation/population/assortativeness/csw", result.assort[(int)SexualPartnership::Type::Csw]);

    result.defaultMaleParams = Get<Male::SubPopParams>(base_path + "/entityTypes/baseEntities/baseEntity[type='Male']");
    result.defaultFemaleParams = Get<Female::SubPopParams>(base_path + "/entityTypes/baseEntities/baseEntity[type='Female']");

    //Get the initial marriage prevalence based on percent male high risk and rate and duration of steady relationships
    double pHigh = result.defaultMaleParams.getProportionHighRisk(DmgProfile::NON_CSW);
    double marriageRateH = result.defaultMaleParams.getSexualBehaviorParams(SexualPartnership::Type::Steady).getAcquisitionRatePerMonth(
	Person::HIGH).getMean();
    double marriageRateL = result.defaultMaleParams.getSexualBehaviorParams(SexualPartnership::Type::Steady).getAcquisitionRatePerMonth(
	Person::LOW).getMean();
    double marriageDurationH = result.defaultMaleParams.getSexualBehaviorParams(
	SexualPartnership::Type::Steady).getPartnershipDurationMth(Person::HIGH).getMean();
    double marriageDurationL = result.defaultMaleParams.getSexualBehaviorParams(
	SexualPartnership::Type::Steady).getPartnershipDurationMth(Person::LOW).getMean();
    result.initproportionMarried = (1 - pHigh) * (marriageRateL * marriageDurationL) / (1 + marriageRateL *
											marriageDurationL) + pHigh * (marriageRateH * marriageDurationH) / (1 + marriageRateH * marriageDurationH);
    //Get the initial regular prevalence based on percent male high risk and rate and duration of regular relationships
    double regularRateH = result.defaultMaleParams.getSexualBehaviorParams(SexualPartnership::Type::Regular).getAcquisitionRatePerMonth(
	Person::HIGH).getMean();
    double regularRateL = result.defaultMaleParams.getSexualBehaviorParams(SexualPartnership::Type::Regular).getAcquisitionRatePerMonth(
	Person::LOW).getMean();
    double regularDurationH = result.defaultMaleParams.getSexualBehaviorParams(
	SexualPartnership::Type::Regular).getPartnershipDurationMth(Person::HIGH).getMean();
    double regularDurationL = result.defaultMaleParams.getSexualBehaviorParams(
	SexualPartnership::Type::Regular).getPartnershipDurationMth(Person::LOW).getMean();
    result.initproportionRegular = (1 - pHigh) * (regularRateL * regularDurationL) + pHigh *
	(regularRateH * regularDurationH);

    //Costs
    GetTemplate<double>(base_path + "/costs/condomCost", result.condomCost);
    GetTemplate<double>(base_path + "/costs/circumcisionCost", result.circumcisionCost);

    //save flags to indicate whether particular partnership types have duration or not
    for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
    {
	result.partnershipsHaveDuration[DmgProfile::MALE][type] =
	    !(result.defaultMaleParams.getSexualBehaviorParams(SexualPartnership::Type(type)).getPartnershipDurationMth(Person::LOW).isZeroDistrib)
	    && !(result.defaultMaleParams.getSexualBehaviorParams(SexualPartnership::Type(type)).getPartnershipDurationMth(Person::HIGH).isZeroDistrib);
	result.partnershipsHaveDuration[DmgProfile::FEMALE][type] = false;
    }

    return result;
}
