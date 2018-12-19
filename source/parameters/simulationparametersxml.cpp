#include "simulationparameters.hpp"
#include "core/population.hpp"
#include "core/simulation.hpp"
#include "utility/cepacinputparser.hpp"

namespace transm {

namespace {
std::string to_string(SexualPartnership::Type type)
{
    switch(type)
    {
    case SexualPartnership::Type::Steady: return "steady";
    case SexualPartnership::Type::Regular: return "regular";
    case SexualPartnership::Type::Casual: return "casual";
    case SexualPartnership::Type::Csw: return "csw";
    default: throw std::runtime_error("unknown type");
    }
}
}

template<>
bool SimulationParametersXml::from_string(const std::string &value_string)
{
    if(value_string == "0" || value_string == "false")
    {
        return false;
    }
    else if(value_string == "1" || value_string == "true")
    {
        return true;
    }

    throw std::runtime_error("boolean value should be one of: 0, 1, true, false. found " + value_string);
}

template<>
int SimulationParametersXml::from_string(const std::string &value_string)
{
    return std::stoi(value_string);
}

template<>
double SimulationParametersXml::from_string(const std::string &value_string)
{
    return std::stod(value_string);
}

template<>
std::string SimulationParametersXml::from_string(const std::string &value)
{
    return value;
}

template<>
std::array<double, 7> SimulationParametersXml::from_string(const std::string &value_string)
{
    std::array<double, 7> values;
    std::stringstream ss(value_string);
    for(int i = 0; i < 7; i++)
    {
        ss >> values[i];
    }
    return values;
}

NormalDist SimulationParametersXml::GetNormalDist(const pugi::xml_node node) const
{
    NormalDist dist;
    auto dist_node = node.child("distribution");
    dist.mean = Text<double>(dist_node.child("mean"));
    dist.stddev = Text<double>(dist_node.child("stdDev"));
    return dist;
}

LogNormalDist SimulationParametersXml::GetLogNormalDist(const pugi::xml_node node) const
{
    auto dist = GetNormalDist(node);
    return LogNormalDist::FromNormal(dist);
}

BetaDist SimulationParametersXml::GetBetaDist(const pugi::xml_node node) const
{
    auto dist = GetNormalDist(node);
    return BetaDist::FromNormal(dist);
}

ShiftedLogNormalDist SimulationParametersXml::GetShiftedLogNormalDist(const pugi::xml_node node) const
{
    auto dist = GetNormalDist(node);
    auto shift = Text<double>(node.child("distribution").child("shift"));
    return ShiftedLogNormalDist::FromShiftedNormal(dist, shift);
}

SimulationParametersXml::SimulationParametersXml(const path &filename)
{
    //XXX: verify that it's safe to give a c_str of a temporary
    document_.load_file(filename.string().c_str());
    name_ = filename.stem().string();
}

SimulationParametersXml::~SimulationParametersXml()
{
}

SimulationParametersXml::InterventionsContainer SimulationParametersXml::GetPopulationInterventions() const
{
    InterventionsContainer interventions;

    for(auto node : document_.child("simulation").child("interventions").child("populationInterventions").children())
    {
        interventions.push_back(GetIntervention(node, false));
    }

    return interventions;
}

Version SimulationParametersXml::GetVersion() const
{
    auto version_string = Attr<std::string>(document_.child("simulation"), "version");
    auto version = Version::from_string(version_string);

    if(Version::compare(version, Utility::get_model_version(), true) != 0)
    {
        throw std::runtime_error("bad input version");
    }

    return version;
}

uint32_t SimulationParametersXml::GetFixedSeed() const
{
    auto simulation_node = document_.child("simulation");
    return Text<int>(simulation_node.child("fixedSeed"));
}

int SimulationParametersXml::GetDuration() const
{
    auto simulation_node = document_.child("simulation");
    return Text<int>(simulation_node.child("duration"));
}

int SimulationParametersXml::GetMonthOf1990() const
{
    auto simulation_node = document_.child("simulation");
    return Text<int>(simulation_node.child("monthOf1990"));
}

int SimulationParametersXml::GetInitialInfectionDelay() const
{
    auto simulation_node = document_.child("simulation");
    auto initial_infections_node = simulation_node.child("population").child("initialInfections");
    return Text<int>(initial_infections_node.child("delay"));
}

SimulationParameters::ConcurrencyDefinition SimulationParametersXml::GetConcurrencyDefinition() const
{
    SimulationParameters::ConcurrencyDefinition definitions;

    auto simulation_node = document_.child("simulation");
    auto concurrency_node = simulation_node.child("concurrencyDefinition");

    for(int i = 0; i < Constants::NumberConcurrencyDefs; i++)
    {
        auto definition_node = concurrency_node.find_child_by_attribute("definition", "id", std::to_string(i).c_str());
        auto &definition = definitions[i];

        if (definition_node == nullptr)
        {
            definition.minPartnershipsNeeded = 2;
            definition.useDefinition = false;
        }
        else
        {
            definition.minPartnershipsNeeded = Text<int>(definition_node.child("minNeeded"));
            definition.useDefinition = Text<bool>(definition_node.child("allow"));
        }
    }

    return definitions;
}

SimulationParameters::TracingParameters SimulationParametersXml::GetTracingParameters() const
{
    SimulationParameters::TracingParameters parameters;

    auto simulation_node = document_.child("simulation");
    auto trace_files_node = simulation_node.child("traceFiles");

    for(auto trace_file_node : trace_files_node.children())
    {
        std::string name = trace_file_node.name();
        parameters.files[name].enabled = Attr<bool>(trace_file_node, "enabled");
        parameters.files[name].extension = Text<std::string>(trace_file_node.child("extension"));
        parameters.files[name].toss = Attr<bool>(trace_file_node, "tossIfCalibFail");
    }
    
    parameters.num_to_trace = Text<int>(simulation_node.child("traceFiles").child("singlePerson").child("numberToTracePerAgeRange"));
    parameters.num_newborns_to_trace = Text<int>(simulation_node.child("traceFiles").child("singlePerson").child("numberNewbornsToTrace"));
    parameters.month_trace_newborns = Time::from_months(Text<int>(simulation_node.child("traceFiles").child("singlePerson").child("monthTraceNewborns")));
    parameters.trace_prevalent_cases = Text<bool>(simulation_node.child("traceFiles").child("singlePerson").child("tracePrevalentCases"));

    parameters.life_expectancy_ci = Text<double>(simulation_node.child("traceFiles").child("lifeExpectancy").child("medianConfidenceInterval"));

    for(auto time_node : simulation_node.child("traceFiles").child("lifeExpectancy").children("time"))
    {
        parameters.life_expectancy_record_times.push_back(Text<int>(time_node));
    }

    for(auto time_node : simulation_node.child("traceFiles").child("partnerAcquisition").children("time"))
    {
    parameters.partner_acquisition_record_times.push_back(Text<int>(time_node));
    }

    return parameters;
}

CalibrationInputs SimulationParametersXml::GetCalibrationParameters() const
{
    CalibrationInputs calib;

    auto simulation_node = document_.child("simulation");
    auto calibration_node = simulation_node.child("calibration");
    calib.useCalibration = Attr<bool>(calibration_node, "enabled");

    if(calib.useCalibration)
    {
        calib.monthOfCalibration = Time::from_months(Text<int>(calibration_node.child("monthOfCalibration")));

    auto partnerships_node = calibration_node.child("partnershipOutcomes");
    if (partnerships_node)
    {
        calib.steadyPrevPopulation = Text<int>(partnerships_node.child("steadyPrev").child("popOfInterest"));
        calib.steadyPrevBounds.lower = Text<double>(partnerships_node.child("steadyPrev").child("lwrBound"));
        calib.steadyPrevBounds.upper = Text<double>(partnerships_node.child("steadyPrev").child("uprBound"));
        calib.casualPrevPopulation = Text<int>(partnerships_node.child("casualPrev").child("popOfInterest"));
        calib.casualPrevBounds.lower = Text<double>(partnerships_node.child("casualPrev").child("lwrBound"));
        calib.casualPrevBounds.upper = Text<double>(partnerships_node.child("casualPrev").child("uprBound"));
        calib.CSWPrevPopulation = Text<int>(partnerships_node.child("cswPrev").child("popOfInterest"));
        calib.CSWPrevBounds.lower = Text<double>(partnerships_node.child("cswPrev").child("lwrBound"));
        calib.CSWPrevBounds.upper = Text<double>(partnerships_node.child("cswPrev").child("uprBound"));
        calib.propInConcurrentPopulation = Text<int>(partnerships_node.child("propInCon").child("popOfInterest"));
        calib.propInConcurrentBounds.lower = Text<double>(partnerships_node.child("propInCon").child("lwrBound"));
        calib.propInConcurrentBounds.upper = Text<double>(partnerships_node.child("propInCon").child("uprBound"));
        calib.numActsPopulation = Text<int>(partnerships_node.child("numActs").child("popOfInterest"));
        calib.numActsBounds.lower = Text<double>(partnerships_node.child("numActs").child("lwrBound"));
        calib.numActsBounds.upper = Text<double>(partnerships_node.child("numActs").child("uprBound"));
        calib.femaleCasualPrevRatio = Text<double>(partnerships_node.child("femaleCasualPrev").child("ratio"));
        calib.femalePropInConcurrentRatio = Text<double>(partnerships_node.child("femalePropInCon").child("ratio"));
        calib.femaleNumActsLRtoHRRatio = Text<double>(partnerships_node.child("femaleNumActsLRtoHR").child("ratio"));
    }

    auto yearly_incidence_ranges_node = calibration_node.child("yearlyIncidenceRanges");
    for (auto incidence_range_node : yearly_incidence_ranges_node.children("yearlyIncidenceRange"))
    {
        // Add a check that sa population incidence is within the range at time
        Time time = Time::from_months(incidence_range_node.attribute("time").as_int());
        double lower = incidence_range_node.attribute("lower").as_double();
        double upper = incidence_range_node.attribute("upper").as_double();
        std::pair<double,double> range(lower, upper);

        calib.yearlyIncidenceRanges[time] = range;
        }
    }

    return calib;
}

InterventionParameters SimulationParametersXml::GetInterventionParameters() const
{
    auto simulation_node = document_.child("simulation");
    auto interventions_node = simulation_node.child("interventions");

    InterventionParameters parameters;

    if(Attr<bool>(interventions_node.child("artRolloutIntervention"), "enabled"))
    {
        parameters.intervention_type = InterventionParameters::InterventionType::Art;
    }
    else
    {
        parameters.intervention_type = InterventionParameters::InterventionType::Cepac;
    }

    if(parameters.intervention_type == InterventionParameters::InterventionType::Art)
    {
        auto scaling_node = interventions_node.child("artRolloutIntervention").child("dynamicTreatmentScaling");

        parameters.dynamic_feedback_period = Text<int>(scaling_node.child("feedbackPeriod"));
        parameters.dynamic_feedback_enabled = Attr<bool>(scaling_node, "enabled");

        for(auto treatment_file_node : interventions_node.select_nodes("artRolloutIntervention/rolloutTreatmentFiles/rolloutFile"))
        {
            auto time = Time::from_months(treatment_file_node.node().child("time").text().as_int());

            if(time >= Time::Zero)
            {
                std::string file_name = treatment_file_node.node().child("fileName").text().as_string();
                int file_number = treatment_file_node.node().child("fileNumber").text().as_int();
                int target_population = treatment_file_node.node().child("popToApply").text().as_int();

                InterventionParameters::CepacFile file;
                file.target_population = target_population;
                file.filename = file_name;
                file.time = time;

                parameters.cepac_files.push_back(file);

                //From the first file only, get the death tables for non-AIDS death
                if(file_number == 0)
                {
                    parameters.default_cepac_file = file;
                    CepacInputParser cepacInput(file_name);
                    auto probabilities = cepacInput.parseNonAidsDeathProbabilities();
                    Entity::probDeathNatCauses[(std::size_t)DemographicProfile::Gender::Male] = probabilities[0];
                    Entity::probDeathNatCauses[(std::size_t)DemographicProfile::Gender::Female] = probabilities[1];
                }
            }
        }

        parameters.eligibility_criteria = GetRolloutEligibility();
    parameters.rollout_proportion_denominator = GetRolloutDenominator();

        for(auto target : interventions_node.select_nodes("artRolloutIntervention/targetRolloutProportions/target"))
        {
            auto year = Attr<int>(target.node(), "year");
            parameters.target_yearly_rollout_proportions.push_back({year, Text<double>(target.node())});
        }
    }
    else
    {
        for(auto treatment_file_node : interventions_node.select_nodes("cepacIntervention/cepacTreatmentFiles/treatmentFile"))
        {
            auto time = Time::from_months(treatment_file_node.node().child("time").text().as_int());

            if(time >= Time::Zero)
            {
                std::string file_name = treatment_file_node.node().child("fileName").text().as_string();
                int file_number = treatment_file_node.node().child("fileNumber").text().as_int();

                InterventionParameters::CepacFile file;
                file.target_population = 0;
                file.filename = file_name;
                file.time = time;

                parameters.cepac_files.push_back(file);

                //From the first file only, get the death tables for non-AIDS death
                if(file_number == 0)
                {
                    parameters.default_cepac_file = file;
                    CepacInputParser cepacInput(file_name);
                    auto probabilities = cepacInput.parseNonAidsDeathProbabilities();
                    Entity::probDeathNatCauses[(std::size_t)DemographicProfile::Gender::Male] = probabilities[0];
                    Entity::probDeathNatCauses[(std::size_t)DemographicProfile::Gender::Female] = probabilities[1];
                }
            }
        }
    }

    return parameters;
}

std::unordered_map<TransmissionType, std::array<double, (std::size_t)Entity::HVLStrata::Last>> SimulationParametersXml::GetTransmissionCoefficients() const
{
    std::unordered_map<TransmissionType, std::array<double, (std::size_t)Entity::HVLStrata::Last>> coefficient_map;

    auto read_coefficients = [](pugi::xml_node node) 
    {
        std::array<double, (std::size_t)Entity::HVLStrata::Last> coefficients = {{0}};
        double value;

        for(auto hvl : enum_iterator<Entity::HVLStrata>())
        {
            if(hvl == Entity::HVLStrata::UNINFECTED) continue;

            switch(hvl)
            {
            case Entity::HVLStrata::UNINFECTED:
                continue;
            case Entity::HVLStrata::HVL_PRIMARY:
                value = node.child("primary").text().as_double();
                break;
            case Entity::HVLStrata::HVL_LATESTAGE:
                value = node.child("lateStage").text().as_double();
                break;
            default: 
                value = node.child(("hvl" + std::to_string(((int)hvl) - 1)).c_str()).text().as_double();
                break;
            }

            coefficients[(std::size_t)hvl] = value;
        }

        return coefficients;
    };

    auto coeff_node = document_.select_node("/simulation/population/transmissionCoefficients").node();
    coefficient_map[TransmissionType::male_to_female] = read_coefficients(coeff_node.child("maleToFemale"));
    coefficient_map[TransmissionType::female_to_male] = read_coefficients(coeff_node.child("femaleToMale"));
    coefficient_map[TransmissionType::male_to_male] = read_coefficients(coeff_node.child("maleToMale"));

    return coefficient_map;
}

RolloutEligibility SimulationParametersXml::GetRolloutEligibility() const
{
    auto eligibility_node = document_.select_node("/simulation/interventions/artRolloutIntervention/rolloutEligibility").node();
    RolloutEligibility eligibility;

    // Identified
    auto identified_node = eligibility_node.select_node("criteria[@name='Identified']").node();
    if (identified_node.child("status"))
    eligibility.isIdentified = Text<bool>(identified_node.child("status"));
    else
    eligibility.isIdentified = false;


    // OIHist
    auto oi_hist_node = eligibility_node.select_node("criteria[@name='OIHist']").node();
    eligibility.oiHistRank = Text<int>(oi_hist_node.child("rank"));
    eligibility.oiHistNumToStart = Text<int>(oi_hist_node.child("numOIToStart"));

    // CD4
    auto cd4_node = eligibility_node.select_node("criteria[@name='CD4']").node();
    eligibility.cd4Rank = Text<int>(cd4_node.child("rank"));
    eligibility.cd4Bounds.lower = Text<int>(cd4_node.child("CD4Lwr"));
    eligibility.cd4Bounds.upper = Text<int>(cd4_node.child("CD4Upp"));

    // CD4OIHist
    auto cd4_oi_hist_node = eligibility_node.select_node("criteria[@name='CD4OIHist']").node();
    eligibility.cd4OiHistRank = Text<int>(cd4_oi_hist_node.child("rank"));
    eligibility.cd4OiHistCd4Bounds.lower = Text<int>(cd4_oi_hist_node.child("CD4Lwr"));
    eligibility.cd4OiHistCd4Bounds.upper = Text<int>(cd4_oi_hist_node.child("CD4Upp"));

    // HVL
    auto hvl_node = eligibility_node.select_node("criteria[@name='HVL']").node();
    eligibility.hvlRank = Text<int>(hvl_node.child("rank"));
    eligibility.hvlBounds.lower = Text<int>(hvl_node.child("HVLLwr"));
    eligibility.hvlBounds.upper = Text<int>(hvl_node.child("HVLUpp"));

    // CD4HVL
    auto cd4_hvl_node = eligibility_node.select_node("criteria[@name='CD4HVL']").node();
    eligibility.cd4HvlRank = Text<int>(cd4_hvl_node.child("rank"));
    eligibility.cd4HvlCd4Bounds.lower = Text<int>(cd4_hvl_node.child("CD4Lwr"));
    eligibility.cd4HvlCd4Bounds.upper = Text<int>(cd4_hvl_node.child("CD4Upp"));
    eligibility.cd4HvlHvlBounds.lower = Text<int>(cd4_hvl_node.child("HVLLwr"));
    eligibility.cd4HvlHvlBounds.upper = Text<int>(cd4_hvl_node.child("HVLUpp"));

    for(int i = 0; i < 15; i++)
    {
        std::string oi_name = std::string("OI") + std::to_string(i);
        eligibility.oiHistOIs[i] = Text<bool>(oi_hist_node.child(oi_name.c_str()));
        eligibility.cd4OiHistOIs[i] = Text<bool>(cd4_oi_hist_node.child(oi_name.c_str()));
    }

    return eligibility;
}

RolloutDenominator SimulationParametersXml::GetRolloutDenominator() const
{
    auto node = document_.select_node("/simulation/interventions/artRolloutIntervention/targetRolloutProportions").node();
    RolloutDenominator denom = RolloutDenominator::DEFAULT;

    try {
        std::string value = Attr<std::string>(node, "proportionDenominator");

    if (value == "population") {
        denom = RolloutDenominator::POPULATION;
    } else if (value == "eligible") {
        denom = RolloutDenominator::ELIGIBLE;
    }
    } catch (std::string err) {
        // denominator not specified
    }

    return denom;
}

SexualBehavior SimulationParametersXml::GetSexualBehavior(const std::string &entity_type, SexualPartnership::Type type) const
{
    auto path = "/simulation/population/entities/entity[@type='" + entity_type + "']/behavior/partnershipTypes/partnership[@type='" + to_string(type) + "']";
    auto node = document_.select_node(path.c_str()).node();

    if(node == nullptr)
    {
        throw std::runtime_error("behavior not defined for entity type " + entity_type + " and partnership type " + to_string(type));
    }

    SexualBehavior result(type);

    auto assortivityNode = node.child("selectionCriteria").child("assortivity");
    result.setRiskAssortativeness(Text<double>(assortivityNode.child("riskAssortivity")));
    result.setRaceAssortativeness(Text<double>(assortivityNode.child("raceAssortivity")));
    result.setEthnicAssortativeness(Text<double>(assortivityNode.child("ethnicAssortivity")));

    result.setChanceChooseWithSteady(Text<double>(assortivityNode.child("chanceChooseWithSteady")));
    result.setChanceMsmwChooseMale(Text<double>(assortivityNode.child("chanceMsmwChooseMale")));
    result.setChanceMsmChooseMsmw(Text<double>(assortivityNode.child("chanceMsmChooseMsmw")));

    result.setAverageYearsYounger(GetNormalDist(node.child("selectionCriteria").child("averageYearsYounger")));

    for(auto risk : {Entity::RiskLevel::LOW, Entity::RiskLevel::HIGH})
    {
        auto risk_node = node.child(risk == Entity::RiskLevel::LOW ? "lowRisk" : "highRisk");

        result.setAcquisitionRatePerMonth(risk, GetLogNormalDist(risk_node.child("acquisitionRate")));
        result.setCoitalEventsPerMonth(risk, Text<double>(risk_node.child("coitalEventsPerMonth").child("distribution").child("mean")));
        result.setChanceCondomUsePerEvent(risk, GetBetaDist(risk_node.child("chanceCondomUsePerEvent")));
        result.setPartnershipDuration(risk, GetShiftedLogNormalDist(risk_node.child("partnershipDurationMth")));
    }

    return result;
}

Male::SubPopParams SimulationParametersXml::GetMaleSubPopParams() const
{
    auto node = document_.select_node("/simulation/population/entities/entity[@type='male']").node();

    Male::SubPopParams result;

    auto behavior_node = node.child("behavior");
    result.SetCswEndAge(Age(Text<int>(behavior_node.child("cswEndAge")), 0));
    result.SetChanceBecomeCsw(Text<double>(behavior_node.child("chanceBecomeSexWorker")));
    result.SetPartnerAcqMultWithSteady(Entity::RiskLevel::HIGH, Text<double>(behavior_node.child("partnerAcqMultWithSteadyHighRisk")));
    result.SetPartnerAcqMultWithSteady(Entity::RiskLevel::LOW, Text<double>(behavior_node.child("partnerAcqMultWithSteadyLowRisk")));

    bool use_high_risk_multiplier = Attr<bool>(behavior_node.child("highRiskAcqRateMultiplier"), "enabled");
    double high_risk_multiplier = Text<double>(behavior_node.child("highRiskAcqRateMultiplier"));
    bool use_csw_high_risk_multiplier = Attr<bool>(behavior_node.child("highRiskCswAcqRateMultiplier"), "enabled");
    double csw_high_risk_multiplier = Text<double>(behavior_node.child("highRiskCswAcqRateMultiplier"));

    result.SetCoefficientVariation(false, 0);

	for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
	{
        try
        {
            // for each initiating DemographicProfile type
			auto params = GetSexualBehavior("male", partnership_type);

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

            result.AddSexualBehavior(params);
        }
        catch(std::runtime_error &e)
        {
            std::cout << e.what() << std::endl;
            continue;
        }
	}

	result.SetProportionHighRisk(DemographicProfile::Employment::Csw, Text<double>(behavior_node.child("proportionHighRiskCsw")));
    result.SetProportionHighRisk(DemographicProfile::Employment::NonCsw, Text<double>(behavior_node.child("proportionHighRiskNonCsw")));

	auto discountingStartAgeYrs = Text<int>(behavior_node.child("ageDiscounting").child("startAgeYrs"));
	auto acquisitionDiscByYr = Text<double>(behavior_node.child("ageDiscounting").child("acquisitionDiscByYr"));
	auto coitalActsDiscByYr = Text<double>(behavior_node.child("ageDiscounting").child("coitalActsDiscByYr"));
	result.setAgeDiscounting(Age(discountingStartAgeYrs, 0), acquisitionDiscByYr, coitalActsDiscByYr);
	result.SetMaxPartnershipRejections(Text<int>(behavior_node.child("maxPartnershipRejections")));

	auto health_node = node.child("health");
	result.SetCircucmsionProtectEfficacy(Text<double>(health_node.child("circumcisionProtectEfficacy")));
	result.SetCondomProtectEff(Text<double>(health_node.child("condomProtectEfficacy")));
	result.SetPreExposureProphylaxisEfficacy(Text<double>(health_node.child("preExposureProphylaxisEfficacy")));

	// This will be moved in the xml to /simulation/population/entities [@type='male']/health
	auto circumcision_node = document_.select_node("/simulation/population/proportionMaleCircumcised").node();
	result.SetProportionCircumcised(Text<double>(circumcision_node));

	return result;
}

Female::SubPopParams SimulationParametersXml::GetFemaleSubPopParams() const
{
    auto node = document_.select_node("/simulation/population/entities/entity[@type='female']").node();

    Female::SubPopParams result;

	auto behavior_node = node.child("behavior");
	result.SetCswEndAge(Age(Text<int>(behavior_node.child("cswEndAge")), 0));
	result.SetChanceBecomeCsw(Text<double>(behavior_node.child("chanceBecomeSexWorker")));
	result.SetProportionHighRisk(DemographicProfile::Employment::NonCsw, Text<double>(behavior_node.child("proportionHighRiskNonCsw")));
	result.SetProportionHighRisk(DemographicProfile::Employment::Csw, Text<double>(behavior_node.child("proportionHighRiskCsw")));

	auto health_node = node.child("health");
	result.SetPreExposureProphylaxisEfficacy(Text<double>(health_node.child("preExposureProphylaxisEfficacy")));
	result.SetVaginalMicrobicideEfficacy(Text<double>(health_node.child("vaginalMicrobicideEfficacy")));

	return result;
}

/*
 * The demographic profiles specified in the xml for this simulation are defined
 * in this function. It must be kept in sync with all other instances of demographicProfiles.
 */
SimulationParametersXml::EntityDistributions SimulationParametersXml::GetEntityDistributions(pugi::xml_node node) const
{
    EntityDistributions distributions;

    for(auto distribution_node : node.children("distribution"))
    {
	    DemographicProfile profile;

        if (distribution_node.attribute("type"))
        {
            std::string entity_type = distribution_node.attribute("type").as_string();
            if (entity_type == "female")
            {
                profile.set(DemographicProfile::Demographic::Gender,
                    (std::size_t)DemographicProfile::Gender::Female);
                // females are only msw (well, wsm)
                profile.set(DemographicProfile::Demographic::SexualOrientation,
					(std::size_t)DemographicProfile::SexualOrientation::Msw);
            }
            else if (entity_type == "male")
            {
                profile.set(DemographicProfile::Demographic::Gender,
                    (std::size_t)DemographicProfile::Gender::Male);
            } else {
                throw std::runtime_error("Unknown entity type in xml node " +
                    std::string(distribution_node.name()));
            }
        }
        else if (distribution_node.attribute("bucket"))
        {
            std::string bucketString = distribution_node.attribute("bucket").as_string();
            profile.parse(bucketString);
        }
        distributions.push_back(DemographicProfile::DoublePair(profile,
			distribution_node.text().as_double()));
    }

    return distributions;
}

//normalize the initial population values for each age bucket
inline void SimulationParametersXml::NormalizeEntityDistributions(PopulationParameters &parameters) const
{
	std::vector<DemographicProfile::DoublePair> totalProfileProportions;

	// first sum the totals per bucket
    for(auto &ageBucketParams : parameters.GetInitialAgeBuckets())
    {
		for (auto profileDoublePair : ageBucketParams.GetEntityProportions())
		{
			DemographicProfile profile = profileDoublePair.first;
			double value = profileDoublePair.second;

			auto iter = std::find_if(totalProfileProportions.begin(),
				totalProfileProportions.end(),
				[&](const DemographicProfile::DoublePair pair)
				    { return pair.first == profile; }
				);

			if (iter == totalProfileProportions.end())
			{
				// add it to the totals
				totalProfileProportions.push_back(
				  DemographicProfile::DoublePair(profile, value));
			}
			else
			{
				// increase the existing value
				iter->second += value;
			}
		}
    }
	// then divide the value of in each bucket by the total for that profile
	for(auto &ageBucketParams : parameters.GetInitialAgeBuckets())
	{
		for (auto profileDoublePair : ageBucketParams.GetEntityProportions())
		{
			DemographicProfile profile = profileDoublePair.first;
			double value = profileDoublePair.second;

			auto iter = std::find_if(totalProfileProportions.begin(),
				totalProfileProportions.end(),
				[&](const DemographicProfile::DoublePair pair)
				    { return pair.first == profile; }
				);
			if (iter->second != 0.0)
			{
				ageBucketParams.SetEntityProportion(profile, value/iter->second);
			}
		}
	}
}

PopulationParameters SimulationParametersXml::GetPopulationParameters() const
{
    auto population_node = document_.child("simulation").child("population");

    PopulationParameters parameters;
    auto initial_state_node = population_node.child("initialState");
    parameters.SetInitialSize(Text<int>(initial_state_node.child("size")));

    //get initial age distribution
    for (auto age_bucket_node : initial_state_node.child("entityDistributions").children("ageRange"))
	{
	    auto distributions = GetEntityDistributions(age_bucket_node);
	    parameters.GetInitialAgeBuckets().emplace_back(
		  Age(Attr<int>(age_bucket_node, "lower"), 0),
		  Age(Attr<int>(age_bucket_node, "upper"), 11),
		  distributions);
    }
	// set the age ranges specified by the xml --
	// these are used mostly in printing headers in output files
	parameters.SetAgeRanges();

    NormalizeEntityDistributions(parameters);

    auto births_node = population_node.child("births");
    parameters.SetBirthRate(Text<double>(births_node.child("rate")));
	auto distributions = GetEntityDistributions(births_node.child("entityDistributions"));
	for (auto distrib : distributions) {
		parameters.SetBirthProportion(distrib.first, distrib.second);
	}

    auto initial_infections_node = population_node.child("initialInfections");
    parameters.SetSeedDelay(Text<int>(initial_infections_node.child("delay")));
    parameters.SetUseSeedCoefficients(
	Text<bool>(initial_infections_node.child("useCoefficients")));
    parameters.SetChanceChronicInfection(
	Text<double>(initial_infections_node.child("chanceSeedChronicInfection")));
    parameters.SetSeedPrevalence(
	Text<double>(initial_infections_node.child("seedPrevalence")));

    for (auto infection_target_node : initial_infections_node.children("profile"))
    {
		PopulationParameters::PopulationTarget target;

		if (infection_target_node.attribute("bucket") != nullptr)
		{
			DemographicProfile profile;
			profile.parse(infection_target_node.attribute("bucket").as_string());
			target.profile = { true, profile };
		}

		if (infection_target_node.attribute("age-range") != nullptr)
		{
			std::string range_string(infection_target_node.attribute("age-range").as_string());
			auto hyphen_index = range_string.find('-');

			assert(hyphen_index != std::string::npos);

			std::string min_string = range_string.substr(0, hyphen_index);

			if (!min_string.empty())
			{
				target.min_age = { true, Age(std::stoi(min_string),0) };
			}

			std::string max_string = range_string.substr(hyphen_index + 1);

			if (!max_string.empty())
			{
				target.max_age = { true, Age(std::stoi(max_string),11) };
			}
		}

		if (infection_target_node.attribute("risk") != nullptr)
		{
			std::string risk_string = infection_target_node.attribute("risk").as_string();
			assert(risk_string == "high" || risk_string == "low");
			target.risk = { true, risk_string == "high" ? Entity::RiskLevel::HIGH : Entity::RiskLevel::LOW };
		}

		parameters.AddInfectionTarget(target, infection_target_node.text().as_int());
    }

    parameters.SetTransmissionCoefficients(GetTransmissionCoefficients());

    parameters.SetAgeOfMajority(Age(Text<int>(population_node.child("ageOfMajority")), 0));

    auto defaultMaleParams = GetMaleSubPopParams();
    parameters.SetMaleParameters(defaultMaleParams);
    auto defaultFemaleParams = GetFemaleSubPopParams();
    parameters.SetFemaleParameters(defaultFemaleParams);

    //save flags to indicate whether particular partnership types have duration or not
    for(auto type : enum_iterator<SexualPartnership::Type>()) {
        if(!defaultMaleParams.hasSexualBehavior(type)) continue;
		auto has_duration =
		  !(defaultMaleParams.getSexualBehavior(SexualPartnership::Type(type)).getPartnershipDurationMth(Entity::RiskLevel::LOW).isZeroDistrib) &&
		  !(defaultMaleParams.getSexualBehavior(SexualPartnership::Type(type)).getPartnershipDurationMth(Entity::RiskLevel::HIGH).isZeroDistrib);
		parameters.SetPartnershipHasDuration(DemographicProfile::Gender::Male, type, has_duration);
		parameters.SetPartnershipHasDuration(DemographicProfile::Gender::Female, type, false);
    }

    pugi::xml_node costs_node = document_.select_node("/simulation/traceFiles/costEffectiveness").node();

    //Costs
    parameters.SetCondomCost(Text<double>(costs_node.child("condomCost")));
    parameters.SetCircumcisionCost(Text<double>(costs_node.child("circumcisionCost")));
    parameters.SetPrEPCost(Text<double>(costs_node.child("prEPCost")));
    parameters.SetVaginalMicrobicideCost(Text<double>(costs_node.child("vaginalMicrobicideCost")));

    return parameters;
}

Nullable<TargetGroup::PopulationTarget> ParseGroupEligibility(pugi::xml_node criteria_node)
{
    Nullable<TargetGroup::PopulationTarget> target;

    for(auto criterion_node : criteria_node.children())
    {
        target.has_value = true;

        std::string name = criterion_node.name();
        std::string value = criterion_node.text().as_string();

        if(name == "gender")
        {
            target.value.gender.has_value = true;

            if(value == "male")
            {
                target.value.gender.value = DemographicProfile::Gender::Male;
            }
            else if(value == "female")
            {
                target.value.gender.value = DemographicProfile::Gender::Female;
            }
            else
            {
                throw std::runtime_error("invalid group target value for " + name + ": " + value);
            }
        }
        else if(name == "circumcised")
        {
            target.value.circumcised.has_value = true;

            if(value == "true")
            {
                target.value.circumcised.value = true;
            }
            else if(value == "false")
            {
                target.value.circumcised.value = false;
            }
            else
            {
                throw std::runtime_error("invalid group target value for " + name + ": " + value);
            }
        }
        else if(name == "hiv-status")
        {
            target.value.observed_hiv_status.has_value = true;

            if(value == "negative")
            {
                target.value.observed_hiv_status.value = Entity::HIVStatus::NEGATIVE;
            }
            else if(value == "observed-acute")
            {
                target.value.observed_hiv_status.value = Entity::HIVStatus::OBSERVED_ACUTE;
            }
            else if(value == "unobserved-acute")
            {
                target.value.observed_hiv_status.value = Entity::HIVStatus::UNOBSERVED_ACUTE;
            }
            else if(value == "observed-chronic")
            {
                target.value.observed_hiv_status.value = Entity::HIVStatus::OBSERVED_CHRONIC;
            }
            else if(value == "unobserved-chronic")
            {
                target.value.observed_hiv_status.value = Entity::HIVStatus::UNOBSERVED_CHRONIC;
            }
            else if(value == "observed-latestage")
            {
                target.value.observed_hiv_status.value = Entity::HIVStatus::OBSERVED_LATESTAGE;
            }
            else if(value == "unobserved-latestage")
            {
                target.value.observed_hiv_status.value = Entity::HIVStatus::UNOBSERVED_LATESTAGE;
            }
	    else if (value == "any-positive")
	    {
		target.value.observed_hiv_status.value = Entity::HIVStatus::ANY_POSITIVE;
	    }
	    else if (value == "not-observed-positive")
            {
                target.value.observed_hiv_status.value = Entity::HIVStatus::ANY_NOT_OBSERVED_POSITIVE;
            }
            else if (value == "observed-positive")
            {
                target.value.observed_hiv_status.value = Entity::HIVStatus::ANY_OBSERVED_POSITIVE;
            }
            else
            {
                throw std::runtime_error("invalid group target value for " + name + ": " + value);
            }
        }
        else if(name == "age")
        {
            target.value.age_lower.has_value = true;
            target.value.age_upper.has_value = true;

            if(value.find('-') != std::string::npos)
            {
                target.value.age_lower.value = std::stoi(value.substr(0, value.find('-')));
                target.value.age_upper.value = std::stoi(value.substr(value.find('-') + 1));
            }
            else
            {
                target.value.age_lower.value = std::stoi(value);
                target.value.age_upper.value = std::stoi(value);
            }

            if(target.value.age_lower.value > target.value.age_upper.value)
            {
                throw std::runtime_error("age range lower bound must be less than or equal to upper bound");
            }

            if(target.value.age_lower.value < 0)
            {
                throw std::runtime_error("age range lower bound must be greater than or equal to 0");
            }
        }
        else if(name == "employment")
        {
            target.value.employment.has_value = true;

            if(value == "csw")
            {
                target.value.employment.value = DemographicProfile::Employment::Csw;
            }
            else if(value == "non-csw")
            {
                target.value.employment.value = DemographicProfile::Employment::NonCsw;
            }
            else
            {
                throw std::runtime_error("invalid group target value for " + name + ": " + value);
            }
        }
        else if(name == "risk-group")
        {
            target.value.risk_level.has_value = true;

            if(value == "high")
            {
                target.value.risk_level.value = Entity::RiskLevel::HIGH;
            }
            else if(value == "low")
            {
                target.value.risk_level.value = Entity::RiskLevel::LOW;
            }
            else
            {
                throw std::runtime_error("invalid group target value for " + name + ": " + value);
            }
        }
        else if(name == "sexual-activity-status")
        {
            target.value.sexual_activity_status.has_value = true;

            if(value == "active")
            {
                target.value.sexual_activity_status.value = DemographicProfile::SexualActivityStatus::Active;
            }
            else if(value == "not-active")
            {
                target.value.sexual_activity_status.value = DemographicProfile::SexualActivityStatus::NotActive;
            }
            else
            {
                throw std::runtime_error("invalid group target value for " + name + ": " + value);
            }
        }
        else if(name == "sexual-orientation")
        {
            target.value.sexual_orientation.has_value = true;

            if(value == "msw")
            {
                target.value.sexual_orientation.value = DemographicProfile::SexualOrientation::Msw;
            }
            else if(value == "msmw")
            {
                target.value.sexual_orientation.value = DemographicProfile::SexualOrientation::Msmw;
            }
            else if(value == "msm")
            {
                target.value.sexual_orientation.value = DemographicProfile::SexualOrientation::Msm;
            }
            else
            {
                throw std::runtime_error("invalid group target value for " + name + ": " + value);
            }
        }
        else if(name == "relationship-status")
        {
            target.value.relationship_status.has_value = true;

            if(value == "single")
            {
                target.value.relationship_status.value = DemographicProfile::RelationshipStatus::Single;
            }
            else if(value == "non-single")
            {
                target.value.relationship_status.value = DemographicProfile::RelationshipStatus::NonSingle;
            }
            else
            {
                throw std::runtime_error("invalid group target value for " + name + ": " + value);
            }
        }
        else if(name == "treatment-status")
        {
            target.value.on_treatment.has_value = true;

            if(value == "treated")
            {
                target.value.on_treatment.value = true;
            }
            else if(value == "untreated")
            {
                target.value.on_treatment.value = false;
            }
            else
            {
                throw std::runtime_error("invalid group target value for " + name + ": " + value);
            }
        }
        else
        {
            throw std::runtime_error("invalid group eligibility criterion: " + name);
        }
    }

    return target;
}

std::pair<int, int> ParseRange(const std::string &range_string, bool require_both = false)
{
    int lower_bound, upper_bound = -1;

    if(range_string.find('-') != std::string::npos)
    {
        lower_bound = std::stoi(range_string.substr(0, range_string.find('-')));
        upper_bound = std::stoi(range_string.substr(range_string.find('-') + 1));
    }
    else
    {
        lower_bound = std::stoi(range_string);

        if(require_both)
        {
            throw std::runtime_error("range should be of the form <lower>-<upper>");
        }
    }

    return {lower_bound, upper_bound};
}

std::vector<Intervention> SimulationParametersXml::GetInterventions(pugi::xml_node interventions_node, bool individual) const
{
    std::vector<Intervention> interventions;

    for(auto intervention_node : interventions_node.children())
    {
        interventions.push_back(GetIntervention(intervention_node, individual));
    }

    return interventions;
}

std::unordered_map<std::string, TargetGroup> SimulationParametersXml::GetTargetGroups() const
{
    pugi::xml_node groups_node = document_.select_node("/simulation/interventions/groups").node();
    std::unordered_map<std::string, TargetGroup> groups;

    for(auto group_node : groups_node.children("group"))
    {
        auto label = Attr<std::string>(group_node, "label");
        auto enrollment_period_string =
            Text<std::string>(group_node.child("enrollment-period"));
        auto enrollment_period = ParseRange(enrollment_period_string);
        bool permanent = Text<bool>(group_node.child("permanent-effect"));
        bool open = Text<bool>(group_node.child("open-enrollment"));
        auto target = ParseGroupEligibility(group_node.child("eligibility-criteria"));

        TargetGroup group(label, Time::from_months(enrollment_period.first),
			Time::from_months(enrollment_period.second), open, target);

        for(auto partition_node : group_node.child("partitions").children("partition"))
        {
            std::string label = Attr<std::string>(partition_node, "label");
            auto proportion = Text<double>(partition_node.child("proportion"));
            auto trace = Text<bool>(partition_node.child("trace"));
            auto interventions = GetInterventions(partition_node.child("interventions"), true);

            group.AddPartition(label, trace, proportion, interventions);
        }

        auto group_label = Attr<std::string>(group_node, "label");
        groups.emplace(std::make_pair(group_label, group));
    }

    return groups;
}

std::vector<std::string> split_string(const std::string &string, char delim)
{
    std::vector<std::string> split;
    std::stringstream ss(string);
    std::string part;
    while(std::getline(ss, part, delim))
    {
        split.push_back(part);
    }
    return split;
}

enum class KnownIntervention
{
    Circumcise,
    BirthRate,
    FertilityRate,
    ProportionMale,
    ProportionCircumcised,
    ChanceBecomeSexWorker,
    DelaySexualActivity,
    TransmissionCoefficient,
    ProportionHighRisk,
    AverageYearsYounger,
    PartnerAcquisitionRate,
    CoitalEventsPerMonth,
    ChanceCondomUse,
    PartnershipDuration,
    RolloutEligibility,
    PartnershipRejectionChance,
    OverrideChanceCondomUse,
    CepacContext,
    VaginalMicrobicideUse,
    PreExposureProphylaxisUse
};

const std::map<KnownIntervention, std::string> KnownInterventionStrings =
{
     { KnownIntervention::Circumcise, "circumcise" },
     { KnownIntervention::BirthRate, "birthRate" },
     { KnownIntervention::FertilityRate, "fertilityRate" },
     { KnownIntervention::ProportionMale, "proportionMale" },
     { KnownIntervention::ProportionCircumcised, "proportionCircumcised" },
     { KnownIntervention::ChanceBecomeSexWorker, "chanceBecomeSexWorker" },
     { KnownIntervention::DelaySexualActivity, "delaySexualActivity" },
     { KnownIntervention::TransmissionCoefficient, "transmissionCoefficient" },
     { KnownIntervention::ProportionHighRisk, "proportionHighRisk" },
     { KnownIntervention::AverageYearsYounger, "averageYearsYounger" },
     { KnownIntervention::PartnerAcquisitionRate, "partnerAcquisitionRate" },
     { KnownIntervention::CoitalEventsPerMonth, "coitalEventsPerMonth" },
     { KnownIntervention::ChanceCondomUse, "chanceCondomUse" },
     { KnownIntervention::PartnershipDuration, "partnershipDuration" },
     { KnownIntervention::RolloutEligibility, "rolloutEligibility" },
     { KnownIntervention::PartnershipRejectionChance, "partnershipRejectionChance" },
     { KnownIntervention::OverrideChanceCondomUse, "overrideChanceCondomUse" },
     { KnownIntervention::CepacContext, "cepacContext" },
     { KnownIntervention::VaginalMicrobicideUse, "vaginalMicrobicideAdherence" },
     { KnownIntervention::PreExposureProphylaxisUse, "preExposureProphylaxisAdherence" }
};

template<>
KnownIntervention SimulationParametersXml::from_string(const std::string &intervention)
{
    auto match = std::find_if(KnownInterventionStrings.begin(), 
        KnownInterventionStrings.end(), 
        [&](const std::pair<KnownIntervention, std::string> &e) 
    { 
        return e.second == intervention; 
    });

    if (match == KnownInterventionStrings.end()) {
      throw std::runtime_error("unknown intervention: " + intervention);
    }

    return match->first;
}

template<>
Entity::RiskLevel SimulationParametersXml::from_string(const std::string &risk)
{
    if(risk == "high") return Entity::RiskLevel::HIGH;
    if(risk == "low") return Entity::RiskLevel::LOW;

    throw std::runtime_error("unknown risk level: " + risk);
}

template<>
DemographicProfile::Gender SimulationParametersXml::from_string(const std::string &gender)
{
    if(gender == "male") return DemographicProfile::Gender::Male;
    if(gender == "female") return DemographicProfile::Gender::Female;

    throw std::runtime_error("unknown gender: " + gender);
}

template<>
DemographicProfile::Employment SimulationParametersXml::from_string(const std::string &employment)
{
    if(employment == "csw") return DemographicProfile::Employment::Csw;
    if(employment == "non-csw") return DemographicProfile::Employment::NonCsw;

    throw std::runtime_error("unknown employment: " + employment);
}

template<>
Entity::HVLStrata SimulationParametersXml::from_string(const std::string &hvl_string)
{
    if(hvl_string == "-1" || hvl_string == "uninfected") return Entity::HVLStrata::UNINFECTED;
    if(hvl_string == "0") return Entity::HVLStrata::HVL_ZERO;
    if(hvl_string == "1") return Entity::HVLStrata::HVL_ONE;
    if(hvl_string == "2") return Entity::HVLStrata::HVL_TWO;
    if(hvl_string == "3") return Entity::HVLStrata::HVL_THREE;
    if(hvl_string == "4") return Entity::HVLStrata::HVL_FOUR;
    if(hvl_string == "5") return Entity::HVLStrata::HVL_FIVE;
    if(hvl_string == "6") return Entity::HVLStrata::HVL_SIX;
    if(hvl_string == "7" || hvl_string == "primary") return Entity::HVLStrata::HVL_PRIMARY;
    if(hvl_string == "8" || hvl_string == "late-stage") return Entity::HVLStrata::HVL_LATESTAGE;

    throw std::runtime_error("unknown hvl stratum: " + hvl_string);
}

template<>
double SimulationParametersXml::TransformInterventionValue(double target_value,
    double curr_value, Time time, TimeSpan duration, Time current_time) const
{
    double new_value;

    int lapsed = duration.in_months() - (current_time.in_months() - time.in_months()) + 1;
    double coeff = (target_value - curr_value) / lapsed;
    new_value = curr_value + coeff;

    return new_value;
}

/*
 * Calculates the current normal distribution when transforming an intervention.
 */
template<>
NormalDist SimulationParametersXml::TransformInterventionValue(NormalDist target_dist,
    NormalDist curr_dist, Time time, TimeSpan duration, Time current_time) const
{
	NormalDist new_dist;

	// calculate the amount to add each time step so we don't have to store a coeff value
	int lapsed = duration.in_months() - (current_time.in_months() - time.in_months()) + 1;
	double mean_coeff = (target_dist.mean - curr_dist.mean) / lapsed;
	double stddev_coeff = (target_dist.stddev - curr_dist.stddev) / lapsed;
	new_dist.mean = curr_dist.mean + mean_coeff;
	new_dist.stddev = curr_dist.stddev + stddev_coeff;

	return new_dist;
}

void SimulationParametersXml::SetChanceCondomUseCallback(pugi::xml_node &node,
    Intervention &intervention, bool individual) const
{
    auto risk = Attr<Entity::RiskLevel>(node, "risk");
    auto type = Attr<SexualPartnership::Type>(node, "type");
    bool transform = false;
    if (node.child("transform"))
	transform = Text<bool>(node.child("transform"));
    NormalDist target_dist = GetNormalDist(node);
    // check that conversion from normal to beta is possible
    BetaDist::FromNormal(target_dist);

    Time time = intervention.GetTime();
    TimeSpan duration = intervention.GetDuration();
    if (!individual) {
	intervention.SetPopulationCallback(
	    [=](Time current_time, Population &p) {
		BetaDist target_beta_dist;
		if (transform) {
		    // increase or descrease to the target value over the duration
		    NormalDist curr_dist = BetaDist::ToNormal(p.GetParameters().
			GetChanceCondomUsePerEvent(risk, type));
		    target_beta_dist = BetaDist::FromNormal(TransformInterventionValue(
			target_dist, curr_dist, time, duration, current_time));
		} else {
		    // set the target value immediately
		    target_beta_dist = BetaDist::FromNormal(target_dist);
		}

		p.GetParameters().SetChanceCondomUsePerEvent(risk, type, target_beta_dist);
	    }
	);
    }
    intervention.SetIndividualCallback(
	[=](Time current_time, Entity *person) {
	    BetaDist target_beta_dist;
	    if (transform) {
		// increase or descrease to the target value over the duration
		NormalDist curr_dist = BetaDist::ToNormal(person->
		    GetChanceCondomUsePerEvent(risk, type));
		target_beta_dist = BetaDist::FromNormal(TransformInterventionValue(
		    target_dist, curr_dist, time, duration, current_time));
	    } else {
		// set the target value immediately
		target_beta_dist = BetaDist::FromNormal(target_dist);
	    }

	    person->SetChanceCondomUsePerEvent(risk, type, target_beta_dist,
		GetRandomNumberGenerator());
	}
    );
}

void SimulationParametersXml::SetProportionCircumcisedCallback(pugi::xml_node &node,
    Intervention &intervention) const
{
    bool transform = false;
    if (node.child("transform"))
	transform = Text<bool>(node.child("transform"));
    double target_value = Text<double>(node.child("proportion"));

    Time time = intervention.GetTime();
    TimeSpan duration = intervention.GetDuration();
    intervention.SetPopulationCallback (
	[=](Time current_time, Population &p) {
	    double new_value = target_value;
	    if (transform) {
		// increase or descrease to the target value over the duration
	        double curr_value = p.GetParameters().GetProportionCircumcised();
		new_value = TransformInterventionValue(
		    target_value, curr_value, time, duration, current_time);
	    }

	    p.GetParameters().SetProportionCircumcised(new_value);
	}
    );
}

void SimulationParametersXml::SetCircumciseCallback(pugi::xml_node &node,
    Intervention &intervention, bool individual) const
{
    if (individual) {
	intervention.SetPopulationIndividualCallback(
	    [=](Time current_time, Population &population, Entity *person) {
		population.Circumcise(person);
	    }
	);
    } else {
	bool transform = false;
	if (node.child("transform"))
	    transform = Text<bool>(node.child("transform"));
	double target_value = Text<double>(node.child("proportion"));

	Time time = intervention.GetTime();
	TimeSpan duration = intervention.GetDuration();
	intervention.SetPopulationCallback (
	    [=](Time current_time, Population &p) {
		double curr_value = p.GetParameters().GetProportionCircumcised();
	        double new_value = target_value;
		if (transform) {
		    // increase or descrease to the target value over the duration
		    curr_value = (double) p.GetNumberCircumcised() /
			(double) p.GetSize(DemographicProfile::Gender::Male);
		    new_value = TransformInterventionValue(
			target_value, curr_value, time, duration, current_time);
		}

		p.Circumcise(abs(new_value - curr_value));
	    }
	);
    }
}

template<>
SexualPartnership::Type SimulationParametersXml::from_string(const std::string &type_string)
{
    if(type_string == "steady") return SexualPartnership::Type::Steady;
    if(type_string == "regular") return SexualPartnership::Type::Regular;
    if(type_string == "casual") return SexualPartnership::Type::Casual;
    if(type_string == "csw") return SexualPartnership::Type::Csw;

    throw std::runtime_error("unknown partnership type: " + type_string);
}

Intervention SimulationParametersXml::GetIntervention(pugi::xml_node &node, bool individual) const
{
    int time = node.attribute("time") != nullptr ? Attr<int>(node, "time") : -1;
    int duration = node.attribute("duration") != nullptr ? Attr<int>(node, "duration") : -1;

    Intervention intervention(Time(0, time), TimeSpan(0, duration));

    auto intervention_type = from_string<KnownIntervention>(node.name());

    if(individual)
    {
        switch(intervention_type)
        {
        case KnownIntervention::Circumcise:
        {
	    SetCircumciseCallback(node, intervention, individual);
            break;
        }
        case KnownIntervention::ChanceBecomeSexWorker:
        {
            auto chance = Text<double>(node);
            intervention.SetIndividualCallback(
                [=](Time current_time, Entity *person) { 
                    person->SetChanceBecomeSexWorker(chance); });
            break;
        }
        case KnownIntervention::DelaySexualActivity:
        {
            auto months = Text<int>(node);
            intervention.SetIndividualCallback(
                [=](Time current_time, Entity *person) { 
                    person->SetSexualActivityDelay(TimeSpan(0, months)); });
            break;
        }
        case KnownIntervention::TransmissionCoefficient:
        {
            /*
            auto hvl_stratum = Attr<Entity::HVLStrata>(node, "hvl");
            auto coefficient = Text<double>(node);
            intervention.SetIndividualCallback(
                [=](Time current_time, Entity *person) { 
                    person->SetTransmissionCoefficient(hvl_stratum, coefficient); });
            break;
            */
            throw std::runtime_error("not implemented");
        }
        case KnownIntervention::AverageYearsYounger:
        {
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = GetNormalDist(node);
            intervention.SetIndividualCallback(
                [=](Time current_time, Entity *person) { 
                    person->SetAverageYearsYounger(partnership_type, dist); });
            break;
        }
        case KnownIntervention::PartnerAcquisitionRate:
        {
            auto risk = Attr<Entity::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = GetLogNormalDist(node);
            intervention.SetIndividualCallback(
                [=](Time current_time, Entity *person) { 
                    person->SetAcquisitionRatePerMonth(risk, partnership_type, dist, GetRandomNumberGenerator()); });
            break;
        }
        case KnownIntervention::CoitalEventsPerMonth:
        {
            auto risk = Attr<Entity::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = Text<double>(node.child("distribution").child("mean"));
            intervention.SetIndividualCallback(
                [=](Time current_time, Entity *person) {
                    person->SetCoitalEventsPerMonth(risk, partnership_type, dist); });
            break;
        }
        case KnownIntervention::ChanceCondomUse:
        {
	    SetChanceCondomUseCallback(node, intervention, individual);
        }
        case KnownIntervention::PartnershipDuration:
        {
            auto risk = Attr<Entity::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = GetShiftedLogNormalDist(node);
            intervention.SetIndividualCallback(
                [=](Time current_time, Entity *person) {
                    person->SetPartnershipDuration(risk, partnership_type, dist); });
            break;
        }
        case KnownIntervention::PartnershipRejectionChance:
        {
            auto risk = Attr<Entity::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto chance = Text<double>(node);
            intervention.SetIndividualCallback(
                [=](Time current_time, Entity *person) {
                person->SetPartnershipRejectionChance(risk, partnership_type, chance); });
            break;
        }
        case KnownIntervention::OverrideChanceCondomUse:
        {
            auto chance = Text<double>(node);
            intervention.SetIndividualCallback(
                [=](Time current_time, Entity *person) {
                person->SetOverrideChanceCondomUse(chance); });
            break;
        }
        case KnownIntervention::CepacContext:
         {
             auto cepac_file = Text<std::string>(node);
             intervention.SetPopulationIndividualCallback([=](Time current_time, Population &pop, Entity *person) 
             {
                 if (person->getDemographicProfileVal<DemographicProfile::SexualActivityStatus>() != DemographicProfile::SexualActivityStatus::Active)
                 {
		  return;
                 }
 
                 auto context = pop.LoadCepacFile(cepac_file);
                 person->SetTargetedCepacContext(context);
             });
             break;
         }
         case KnownIntervention::VaginalMicrobicideUse:
         {
             auto adherence = Text<double>(node);
             intervention.SetIndividualCallback([=](Time current_time, Entity *person) 
             {
                 if (person->getDemographicProfileVal<DemographicProfile::Gender>() != DemographicProfile::Gender::Female)
                 {
		  return;
                 }
 
                 auto female = static_cast<Female *>(person);
                 female->SetVaginalMicrobicideAdherence(adherence); 
             });
             break;
         }
         case KnownIntervention::PreExposureProphylaxisUse:
         {
             auto adherence = Text<double>(node);
             intervention.SetIndividualCallback([=](Time current_time, Entity *person) 
             {
                if (person->getHIVStatus() == Entity::HIVStatus::OBSERVED_ACUTE
                    || person->getHIVStatus() == Entity::HIVStatus::OBSERVED_LATESTAGE
                    || person->getHIVStatus() == Entity::HIVStatus::OBSERVED_CHRONIC)
		{
		  return;
                }
 
                 person->UsePreExposureProphylaxis(adherence); 
             });
             break;
         }
        default:
        {
            std::string message = "Intervention cannot be applied to a specific sub-population: ";
            message.append(KnownInterventionStrings.at(intervention_type));
            throw std::runtime_error(message);
        }
        }
    }
    else
    {
        switch(intervention_type)
        {
        case KnownIntervention::BirthRate:
        {
            auto value = Text<double>(node);
            intervention.SetPopulationCallback(
                [=](Time current_time, Population &p) {
		    p.GetParameters().SetUseBirthRate(true);
		    p.GetParameters().ClearFertilityRates();
		    p.GetParameters().SetBirthRate(value); });
            break;
        }
	case KnownIntervention::FertilityRate:
	{
	    std::vector<FertilityRate> rates;
	    for(auto child : node.children("rateForAgeRange")) {
		auto lower = Age(Attr<int>(child, "lower"), 0);
		auto upper = Age(Attr<int>(child, "upper"), 11);
		auto value = Text<double>(child);
		FertilityRate rate(lower, upper, value);
		rates.push_back(rate);
	    }
	    intervention.SetPopulationCallback(
		[=](Time current_time, Population &p) {
		    p.GetParameters().SetUseBirthRate(false);
		    p.GetParameters().ClearFertilityRates();
		    for ( auto rate : rates)
			p.GetParameters().PushFertilityRate(rate);
		}
	    );
	    break;
	}
	case KnownIntervention::Circumcise:
        {
	    SetCircumciseCallback(node, intervention, individual);
            break;
        }
        case KnownIntervention::ProportionMale:
        {
            auto value = Text<double>(node);
            intervention.SetPopulationCallback(
                [=](Time current_time, Population &p) {
		    DemographicProfile profile;
		    profile.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Male);
		    p.GetParameters().SetBirthProportion(profile.getProfileID(), value);
		});
            break;
        }
        case KnownIntervention::ProportionCircumcised:
        {
	    SetProportionCircumcisedCallback(node, intervention);
            break;
        }
        case KnownIntervention::ChanceBecomeSexWorker:
        {
            auto gender = Attr<DemographicProfile::Gender>(node, "gender");
            auto chance = Text<double>(node);
            intervention.SetPopulationCallback(
                [=](Time current_time, Population &p) { p.GetParameters().SetChanceBecomeCsw(gender, chance); });
            intervention.SetIndividualCallback([=](Time current_time, Entity *person)
            {
                if((DemographicProfile::Gender)person->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == gender)
                {
                    person->SetChanceBecomeSexWorker(chance);
                }
            });
            break;
        }
        case KnownIntervention::DelaySexualActivity:
        {
            auto months = Text<int>(node);
            intervention.SetPopulationCallback(
				[=](Time current_time, Population &p) { p.GetParameters().SetSexualActivityDelay(TimeSpan(0, months)); });
            intervention.SetIndividualCallback(
                [=](Time current_time, Entity *person) { person->SetSexualActivityDelay(TimeSpan(0, months)); });
            break;
        }
        case KnownIntervention::TransmissionCoefficient:
        {
            /*
            auto gender = Attr<DemographicProfile::Gender>(node, "gender");
            auto hvl_stratum = Attr<Entity::HVLStrata>(node, "hvl");
            auto coefficient = Text<double>(node);
            intervention.SetPopulationCallback(
                [=](Time current_time, Population &p) { p.popWideParams.SetTransmissionCoefficient(gender, hvl_stratum, coefficient); });
            intervention.SetIndividualCallback([=](Time current_time, Entity *person)
            {
                if((DemographicProfile::Gender)person->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == gender)
                {
                    person->SetTransmissionCoefficient(hvl_stratum, coefficient);
                }
            });
            break;
            */
            throw std::runtime_error("not implemented");
        }
        case KnownIntervention::ProportionHighRisk:
        {
            auto gender = Attr<DemographicProfile::Gender>(node, "gender");
            auto employment = Attr<DemographicProfile::Employment>(node, "employment");
            auto proportion = Text<double>(node);
            intervention.SetPopulationCallback(
                [=](Time current_time, Population &p) { p.GetParameters().SetProportionHighRisk(gender, employment, proportion); });
            intervention.SetIndividualCallback([=](Time current_time, Entity *person)
            {
                if(gender == (DemographicProfile::Gender)person->getDemographicProfileVal(DemographicProfile::Demographic::Gender))
                {
                    person->SetProportionHighRisk(employment, proportion);
                }
            });
            break;
        }
        case KnownIntervention::AverageYearsYounger:
        {
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = GetNormalDist(node);
            intervention.SetPopulationCallback(
                [=](Time current_time, Population &p) { p.GetParameters().SetAverageYearsYounger(partnership_type, dist); });
            intervention.SetIndividualCallback([=](Time current_time, Entity *person) 
            { 
                if((DemographicProfile::Gender)person->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == DemographicProfile::Gender::Male)
                {
                    person->SetAverageYearsYounger(partnership_type, dist);
                }
            });
            break;
        }
        case KnownIntervention::PartnerAcquisitionRate:
        {
            auto risk = Attr<Entity::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = GetLogNormalDist(node);
            intervention.SetPopulationCallback(
                [=](Time current_time, Population &p) { p.GetParameters().SetAcquisitionRatePerMonth(risk, partnership_type, dist); });
            intervention.SetIndividualCallback([=](Time current_time, Entity *person)
            {
                if((DemographicProfile::Gender)person->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == DemographicProfile::Gender::Male)
                {
                    person->SetAcquisitionRatePerMonth(risk, partnership_type, dist, GetRandomNumberGenerator());
                }
            });
            break;
        }
        case KnownIntervention::CoitalEventsPerMonth:
        {
            auto risk = Attr<Entity::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = Text<double>(node.child("distribution").child("mean"));
            intervention.SetPopulationCallback(
                [=](Time current_time, Population &p) { p.GetParameters().SetCoitalEventsPerMonth(risk, partnership_type, dist); });
            intervention.SetIndividualCallback([=](Time current_time, Entity *person)
            {
                if((DemographicProfile::Gender)person->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == DemographicProfile::Gender::Male)
                {
                    person->SetCoitalEventsPerMonth(risk, partnership_type, dist);
                }
            });
            break;
        }
        case KnownIntervention::ChanceCondomUse:
        {
	    SetChanceCondomUseCallback(node, intervention, individual);
            break;
        }
        case KnownIntervention::PartnershipDuration:
        {
            auto risk = Attr<Entity::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = GetShiftedLogNormalDist(node);
            intervention.SetPopulationCallback(
                [=](Time current_time, Population &p) { p.GetParameters().SetPartnershipDuration(risk, partnership_type, dist); });
            intervention.SetIndividualCallback([=](Time current_time, Entity *person)
            {
		if (person->isMale())
                {
                    person->SetPartnershipDuration(risk, partnership_type, dist);
                }
            });
            break;
        }
        case KnownIntervention::RolloutEligibility:
        {
            std::string criterion = Attr<std::string>(node, "criterion");
            std::string parameter_name = Attr<std::string>(node, "parameter");

	    if (criterion == "Identified")
	    {
		intervention.SetSimulationCallback(
		    [=](Time current_time, Simulation &s) {
			s.GetEventParams().rolloutEligibility.isIdentified = Text<bool>(node);
		    });
	    }
	    else if(criterion == "OIHist")
            {
		int new_value = Text<int>(node);
                if(parameter_name == "rank")
                {
                    intervention.SetSimulationCallback(
                        [=](Time current_time, Simulation &s) {
				s.GetEventParams().rolloutEligibility.oiHistRank = new_value; });
                }
                else if(parameter_name.substr(0, 2) == "OI")
                {
                    int oi_number = std::stoi(parameter_name.substr(2));
                    intervention.SetSimulationCallback(
                        [=](Time current_time, Simulation &s) { s.GetEventParams().rolloutEligibility.oiHistOIs[oi_number] = new_value != 0; });
                }
                else if(parameter_name == "numOIToStart")
                {
                    intervention.SetSimulationCallback(
                        [=](Time current_time, Simulation &s) { s.GetEventParams().rolloutEligibility.oiHistNumToStart = new_value; });
                }
                else
                {
                    throw std::runtime_error("invalid parameter: " + parameter_name);
                }
            }
            else if(criterion == "CD4")
            {
		int new_value = Text<int>(node);
                if(parameter_name == "rank")
                {
                    intervention.SetSimulationCallback(
                        [=](Time current_time, Simulation &s) { s.GetEventParams().rolloutEligibility.cd4Rank = new_value; });
                }
                else if(parameter_name == "CD4Lwr")
                {
                    intervention.SetSimulationCallback(
                        [=](Time current_time, Simulation &s) { s.GetEventParams().rolloutEligibility.cd4Bounds.lower = new_value; });
                }
                else if(parameter_name == "CD4Upp")
                {
                    intervention.SetSimulationCallback(
                        [=](Time current_time, Simulation &s) { s.GetEventParams().rolloutEligibility.cd4Bounds.upper = new_value; });
                }
                else
                {
                    throw std::runtime_error("invalid parameter: " + parameter_name);
                }
            }
            else if(criterion == "CD4OIHist")
            {
		int new_value = Text<int>(node);
                if(parameter_name == "rank")
                {
                    intervention.SetSimulationCallback(
                        [=](Time current_time, Simulation &s) { s.GetEventParams().rolloutEligibility.cd4OiHistRank = new_value; });
                }
                else if(parameter_name == "CD4Lwr")
                {
                    intervention.SetSimulationCallback(
                        [=](Time current_time, Simulation &s) { s.GetEventParams().rolloutEligibility.cd4OiHistCd4Bounds.lower = new_value; });
                }
                else if(parameter_name == "CD4Upp")
                {
                    intervention.SetSimulationCallback(
                        [=](Time current_time, Simulation &s) { s.GetEventParams().rolloutEligibility.cd4OiHistCd4Bounds.upper = new_value; });
                }
                else if(parameter_name.substr(0, 2) == "OI")
                {
                    int oi_number = std::stoi(parameter_name.substr(2));
                    intervention.SetSimulationCallback(
                        [=](Time current_time, Simulation &s) { s.GetEventParams().rolloutEligibility.cd4OiHistOIs[oi_number] = new_value != 0; });
                }
                else
                {
                    throw std::runtime_error("invalid parameter: " + parameter_name);
                }
            }
            else if(criterion == "HVL")
            {
		int new_value = Text<int>(node);
		if(parameter_name == "rank")
                {
                    intervention.SetSimulationCallback(
                        [=](Time current_time, Simulation &s) { s.GetEventParams().rolloutEligibility.hvlRank = new_value; });
                }
                else if(parameter_name == "HVLLwr")
                {
                    intervention.SetSimulationCallback(
                        [=](Time current_time, Simulation &s) { s.GetEventParams().rolloutEligibility.hvlBounds.lower = new_value; });
                }
                else if(parameter_name == "HVLUpp")
                {
                    intervention.SetSimulationCallback(
                        [=](Time current_time, Simulation &s) { s.GetEventParams().rolloutEligibility.hvlBounds.upper = new_value; });
                }
                else
                {
                    throw std::runtime_error("invalid parameter: " + parameter_name);
                }
            }
            else if(criterion == "CD4HVL")
            {
		int new_value = Text<int>(node);
                if(parameter_name == "rank")
                {
                    intervention.SetSimulationCallback(
                        [=](Time current_time, Simulation &s) { s.GetEventParams().rolloutEligibility.cd4HvlRank = new_value; });
                }
                else if(parameter_name == "CD4Lwr")
                {
                    intervention.SetSimulationCallback(
                        [=](Time current_time, Simulation &s) { s.GetEventParams().rolloutEligibility.cd4HvlCd4Bounds.lower = new_value; });
                }
                else if(parameter_name == "CD4Upp")
                {
                    intervention.SetSimulationCallback(
                        [=](Time current_time, Simulation &s) { s.GetEventParams().rolloutEligibility.cd4HvlCd4Bounds.upper = new_value; });
                }
                else if(parameter_name == "HVLLwr")
                {
                    intervention.SetSimulationCallback(
                        [=](Time current_time, Simulation &s) { s.GetEventParams().rolloutEligibility.cd4HvlHvlBounds.lower = new_value; });
                }
                else if(parameter_name == "HVLUpp")
                {
                    intervention.SetSimulationCallback(
                        [=](Time current_time, Simulation &s) {
                        s.GetEventParams().rolloutEligibility.cd4HvlHvlBounds.upper = new_value; });
                }
                else
                {
                    throw std::runtime_error("invalid parameter: " + parameter_name);
                }
            }
            else
            {
                throw std::runtime_error("invalid rollout eligibility criterion for intervention: " + criterion);
            }
            break;
        }
        default:
        {
            std::string message = "Intervention cannot be applied to population: ";
            message.append(KnownInterventionStrings.at(intervention_type));
            throw std::runtime_error(message);
        }
        }
    }

    return intervention;
}

} // namespace transm
