#pragma once

#include "Simulation.h"
#include "cepacbridge/CepacInputParser.h"
#include "util/xml/pugixml.hpp"

class Serializer
{
public:
	template<typename T>
	void Serialize(const T &source, pugi::xml_node &destination);

	template<typename T>
	std::unordered_map<std::string, TemplateParameter> Deserialize(const pugi::xml_node &source, T &destination);

	template<>
	void Serialize(const Simulation &source, pugi::xml_node &destination)
	{

	}

	template<>
	std::unordered_map<std::string, TemplateParameter> Deserialize(const pugi::xml_node &source, Simulation &destination)
	{
		std::unordered_map<std::string, TemplateParameter> parameter_keys;

		auto &parameters = destination.parameters_;

		parameters.debugLevel = static_cast<DebugLevel>(source.child("debugLevel").text().as_int());

		destination.fixedSeed_ = source.select_single_node("/simulation/fixedSeed").node().text().as_int();
		parameters.monthOf1990 = source.select_single_node("/simulation/monthOf1990").node().text().as_int();

		auto version = Version::FromString(source.select_single_node("/simulation/inputVersion").node().text().as_string());

		if(Version::Compare(version, Util::MODEL_VERSION, true) != 0)
		{
			throw std::runtime_error("bad input version");
		}

		//save Concurrency Definitions
		for(int i = 0; i < 16; i++)
		{
			auto path = "/simulation/concurrencyDefinition/def" + std::to_string(i);
			auto definition_node = source.select_single_node(path.c_str());
			int minimum_needed = definition_node.node().child("minNeeded").text().as_int();
			bool allow = definition_node.node().child("minNeeded").text().as_int() != 0;
			parameters.concurrencyDef[i] = EventParams::ConcurrencyDef(minimum_needed, allow);
		}

		static const auto trace_files = {"population", "infection", "partnership", "survival",
			"costEffectiveness", "clinical", "events", "health", "singleperson", "le", "partacq",
			"calibStats", "artRollout", "shiftedOutcomes"};

		std::size_t trace_file_index = 0;
		for(auto trace_file : trace_files)
		{
			parameters.outputTrace[trace_file_index] =
				source.select_single_node("/simulation/writeTrace").node().child(trace_file).text().as_int() != 0;
			parameters.traceExtensions[trace_file_index] =
				source.select_single_node("/simulation/extensionNames").node().child(trace_file).text().as_string();
			trace_file_index++;
		}

		//save calibration inputs
		parameters.calibrationInputs.useCalibration =
			source.select_single_node("/simulation/calibration/useCalibration").node().text().as_int() != 0;
		/*
		if(parameters.calibrationInputs.useCalibration)
		{
		parameters.calibrationInputs.monthOfCalibration =
		source.select_single_node("/simulation/calibration/monthOfCalibration").node().text().as_int();
		parameters.calibrationInputs.steadyPrevPopulation = (int)inputs_.GetCalibrationSettings().steady_target.target;
		parameters.calibrationInputs.steadyPrevBounds[Constants::LOWER] = inputs_.GetCalibrationSettings().steady_target.lower;
		parameters.calibrationInputs.steadyPrevBounds[Constants::UPPER] = inputs_.GetCalibrationSettings().steady_target.upper;
		parameters.calibrationInputs.casualPrevPopulation = (int)inputs_.GetCalibrationSettings().casual_target.target;
		parameters.calibrationInputs.casualPrevBounds[Constants::LOWER] = inputs_.GetCalibrationSettings().casual_target.lower;
		parameters.calibrationInputs.casualPrevBounds[Constants::UPPER] = inputs_.GetCalibrationSettings().casual_target.upper;
		parameters.calibrationInputs.CSWPrevPopulation = (int)inputs_.GetCalibrationSettings().csw_target.target;
		parameters.calibrationInputs.CSWPrevBounds[Constants::LOWER] = inputs_.GetCalibrationSettings().csw_target.lower;
		parameters.calibrationInputs.CSWPrevBounds[Constants::UPPER] = inputs_.GetCalibrationSettings().csw_target.upper;
		parameters.calibrationInputs.propInConcurrentPopulation = (int)inputs_.GetCalibrationSettings().proportion_concurrent_target.target;
		parameters.calibrationInputs.propInConcurrentBounds[Constants::LOWER] = inputs_.GetCalibrationSettings().proportion_concurrent_target.lower;
		parameters.calibrationInputs.propInConcurrentBounds[Constants::UPPER] = inputs_.GetCalibrationSettings().proportion_concurrent_target.upper;
		parameters.calibrationInputs.numActsPopulation = (int)inputs_.GetCalibrationSettings().num_acts_target.target;
		parameters.calibrationInputs.numActsBounds[Constants::LOWER] = inputs_.GetCalibrationSettings().num_acts_target.lower;
		parameters.calibrationInputs.numActsBounds[Constants::UPPER] = inputs_.GetCalibrationSettings().num_acts_target.upper;
		parameters.calibrationInputs.femaleCasualPrevRatio = inputs_.GetCalibrationSettings().female_casual_prevalence;
		parameters.calibrationInputs.femalePropInConcurrentRatio = inputs_.GetCalibrationSettings().female_proportion_in_concurrent;
		parameters.calibrationInputs.femaleNumActsLRtoHRRatio = inputs_.GetCalibrationSettings().female_proportion_lr_to_hr;

		for(int i = 0; i < Constants::NUMBER_CALIBRATION_PREVS; i++)
		{
		parameters.calibrationInputs.calendarPrevs[i] =
		calibParams->FirstChildElement("calendarPrevalence")->FirstChildElement("time" + boost::lexical_cast<std::string>
		(i))->GetText<double>();
		}

		for(int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
		{
		parameters.calibrationInputs.saveStateTimePoints[i] = calibParams->FirstChildElement("storePoint" +
		boost::lexical_cast<std::string>(i)+"Mth")->GetText<int>();
		}

		parameters.calibrationInputs.thresholdPrevMult =
		calibParams->FirstChildElement("thresholdMultiplier")->GetText<double>();
		}
		*/

		destination.duration_ = source.select_single_node("/simulation/timeLimitMth").node().text().as_int();
		parameters.delayPrevalence =
			source.select_single_node("/simulation/population/initialState/delay").node().text().as_int();

		auto interventions_node = source.child("population").child("interventions");
		parameters.useRollout =
			interventions_node.child("artRolloutIntervention").child("useRollout").text().as_int() != 0;

		if(parameters.useRollout)
		{
			std::size_t treatment_file_index = 0;
			for(auto treatment_file_node : source.select_nodes("/simulation/population/interventions/artRolloutIntervention/rolloutTreatmentFiles/rolloutFile"))
			{
				int time = treatment_file_node.node().child("time").text().as_int();

				if(time > -1)
				{
					std::string file_name = treatment_file_node.node().child("fileName").text().as_string();
					int file_number = treatment_file_node.node().child("fileNumber").text().as_int();
					int target_population = treatment_file_node.node().child("popToApply").text().as_int();

					//Make sure the number of CEPAC input files from the .xml file is not greater than the number expected by the code!
					assert(file_number < Constants::NUMBER_OF_ROLLOUT_FILES);

					//Set the CEPAC simContext from the specified CEPAC .in file
					SimContext *contextToAdd = new SimContext(file_name.substr(0, file_name.find(CepacUtil::FILE_EXTENSION_FOR_INPUT)));
					parameters.rolloutSimContexts.push_back(new EventParams::RolloutContext(time, contextToAdd, target_population));
					//Don't trace any CEPAC patients -- the output doesn't make any sense and it just gets overly large for no reason
					//TODO: The reason is because the CEPAC Patient number doesn't get updated until the patient dies: this should be changed!
					//parameters.cepacSimContext->numPatientsToTrace = 0;
					parameters.rolloutSimContexts.at(file_number)->rolloutSimContext->numPatientsToTrace = 0;

					//Read in the inputs
					try
					{
						parameters.rolloutSimContexts.back()->rolloutSimContext->readInputs();
					}
					catch(std::string errorString)
					{
						throw std::runtime_error("error loading rollout file, " + file_name + ": " + errorString);
					}

					//From the first file only, get the death tables for non-AIDS death
					if(file_number == 0)
					{
						CepacInputParser cepacInput(file_name);
						auto probabilities = cepacInput.parseNonAidsDeathProbabilities();
						Person::probDeathNatCauses[DmgProfile::MALE] = probabilities[0];
						Person::probDeathNatCauses[DmgProfile::FEMALE] = probabilities[1];
					}
				}
			}

			auto eligibility_node =
				source.select_single_node("/simulation/population/interventions/artRolloutIntervention/rolloutEligibility").node();

			// OIHist
			parameters.rolloutEligibility.oiHistRank = TryExtractTemplateParameter<int>(
				eligibility_node.select_single_node("criteria[name=\"OIHist\"]/rank").node(), 
				TemplateParameter::OIHistRank, parameter_keys);
			parameters.rolloutEligibility.oiHistOIs[0] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"OIHist\"]/OI0").node(),
				TemplateParameter::OIHistOI0, parameter_keys);
			parameters.rolloutEligibility.oiHistOIs[1] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"OIHist\"]/OI1").node(),
				TemplateParameter::OIHistOI1, parameter_keys);
			parameters.rolloutEligibility.oiHistOIs[2] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"OIHist\"]/OI2").node(),
				TemplateParameter::OIHistOI2, parameter_keys);
			parameters.rolloutEligibility.oiHistOIs[3] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"OIHist\"]/OI3").node(),
				TemplateParameter::OIHistOI3, parameter_keys);
			parameters.rolloutEligibility.oiHistOIs[4] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"OIHist\"]/OI4").node(),
				TemplateParameter::OIHistOI4, parameter_keys);
			parameters.rolloutEligibility.oiHistOIs[5] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"OIHist\"]/OI5").node(),
				TemplateParameter::OIHistOI5, parameter_keys);
			parameters.rolloutEligibility.oiHistOIs[6] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"OIHist\"]/OI6").node(),
				TemplateParameter::OIHistOI6, parameter_keys);
			parameters.rolloutEligibility.oiHistOIs[7] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"OIHist\"]/OI7").node(),
				TemplateParameter::OIHistOI7, parameter_keys);
			parameters.rolloutEligibility.oiHistOIs[8] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"OIHist\"]/OI8").node(),
				TemplateParameter::OIHistOI8, parameter_keys);
			parameters.rolloutEligibility.oiHistOIs[9] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"OIHist\"]/OI9").node(),
				TemplateParameter::OIHistOI9, parameter_keys);
			parameters.rolloutEligibility.oiHistOIs[10] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"OIHist\"]/OI10").node(),
				TemplateParameter::OIHistOI10, parameter_keys);
			parameters.rolloutEligibility.oiHistOIs[11] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"OIHist\"]/OI11").node(),
				TemplateParameter::OIHistOI11, parameter_keys);
			parameters.rolloutEligibility.oiHistOIs[12] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"OIHist\"]/OI12").node(),
				TemplateParameter::OIHistOI12, parameter_keys);
			parameters.rolloutEligibility.oiHistOIs[13] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"OIHist\"]/OI13").node(),
				TemplateParameter::OIHistOI13, parameter_keys);
			parameters.rolloutEligibility.oiHistOIs[14] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"OIHist\"]/OI14").node(),
				TemplateParameter::OIHistOI14, parameter_keys);
			parameters.rolloutEligibility.oiHistNumToStart = TryExtractTemplateParameter<int>(
				eligibility_node.select_single_node("criteria[name=\"OIHist\"]/numOIToStart").node(),
				TemplateParameter::OIHistNumOIToStart, parameter_keys);

			// CD4
			parameters.rolloutEligibility.cd4Rank = TryExtractTemplateParameter<int>(
				eligibility_node.select_single_node("criteria[name=\"CD4\"]/rank").node(),
				TemplateParameter::CD4Rank, parameter_keys);
			parameters.rolloutEligibility.cd4Bounds[0] = TryExtractTemplateParameter<int>(
				eligibility_node.select_single_node("criteria[name=\"CD4\"]/CD4Lwr").node(),
				TemplateParameter::CD4CD4Lwr, parameter_keys);
			parameters.rolloutEligibility.cd4Bounds[1] = TryExtractTemplateParameter<int>(
				eligibility_node.select_single_node("criteria[name=\"CD4\"]/CD4Upp").node(),
				TemplateParameter::CD4CD4Upp, parameter_keys);

			// CD4OIHist
			parameters.rolloutEligibility.cd4OiHistRank = TryExtractTemplateParameter<int>(
				eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/rank").node(),
				TemplateParameter::CD4OIHistRank, parameter_keys);
			parameters.rolloutEligibility.cd4OiHistCd4Bounds[0] = TryExtractTemplateParameter<int>(
				eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/CD4Lwr").node(),
				TemplateParameter::CD4OIHistCD4Lwr, parameter_keys);
			parameters.rolloutEligibility.cd4OiHistCd4Bounds[1] = TryExtractTemplateParameter<int>(
				eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/CD4Upp").node(),
				TemplateParameter::CD4OIHistCD4Upp, parameter_keys);
			parameters.rolloutEligibility.cd4OiHistOIs[0] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/OI0").node(),
				TemplateParameter::CD4OIHistOI0, parameter_keys);
			parameters.rolloutEligibility.cd4OiHistOIs[1] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/OI1").node(),
				TemplateParameter::CD4OIHistOI1, parameter_keys);
			parameters.rolloutEligibility.cd4OiHistOIs[2] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/OI2").node(),
				TemplateParameter::CD4OIHistOI2, parameter_keys);
			parameters.rolloutEligibility.cd4OiHistOIs[3] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/OI3").node(),
				TemplateParameter::CD4OIHistOI3, parameter_keys);
			parameters.rolloutEligibility.cd4OiHistOIs[4] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/OI4").node(),
				TemplateParameter::CD4OIHistOI4, parameter_keys);
			parameters.rolloutEligibility.cd4OiHistOIs[5] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/OI5").node(),
				TemplateParameter::CD4OIHistOI5, parameter_keys);
			parameters.rolloutEligibility.cd4OiHistOIs[6] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/OI6").node(),
				TemplateParameter::CD4OIHistOI6, parameter_keys);
			parameters.rolloutEligibility.cd4OiHistOIs[7] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/OI7").node(),
				TemplateParameter::CD4OIHistOI7, parameter_keys);
			parameters.rolloutEligibility.cd4OiHistOIs[8] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/OI8").node(),
				TemplateParameter::CD4OIHistOI8, parameter_keys);
			parameters.rolloutEligibility.cd4OiHistOIs[9] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/OI9").node(),
				TemplateParameter::CD4OIHistOI9, parameter_keys);
			parameters.rolloutEligibility.cd4OiHistOIs[10] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/OI10").node(),
				TemplateParameter::CD4OIHistOI10, parameter_keys);
			parameters.rolloutEligibility.cd4OiHistOIs[11] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/OI11").node(),
				TemplateParameter::CD4OIHistOI11, parameter_keys);
			parameters.rolloutEligibility.cd4OiHistOIs[12] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/OI12").node(),
				TemplateParameter::CD4OIHistOI12, parameter_keys);
			parameters.rolloutEligibility.cd4OiHistOIs[13] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/OI13").node(),
				TemplateParameter::CD4OIHistOI13, parameter_keys);
			parameters.rolloutEligibility.cd4OiHistOIs[14] = TryExtractTemplateParameter<bool>(
				eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/OI14").node(),
				TemplateParameter::CD4OIHistOI14, parameter_keys);

			// HVL
			parameters.rolloutEligibility.hvlRank = TryExtractTemplateParameter<int>(
				eligibility_node.select_single_node("criteria[name=\"HVL\"]/rank").node(),
				TemplateParameter::HVLRank, parameter_keys);
			parameters.rolloutEligibility.hvlBounds[0] = TryExtractTemplateParameter<int>(
				eligibility_node.select_single_node("criteria[name=\"HVL\"]/HVLLwr").node(),
				TemplateParameter::HVLHVLLwr, parameter_keys);
			parameters.rolloutEligibility.hvlBounds[1] = TryExtractTemplateParameter<int>(
				eligibility_node.select_single_node("criteria[name=\"HVL\"]/HVLUpp").node(),
				TemplateParameter::HVLHVLUpp, parameter_keys);

			// CD4HVL
			parameters.rolloutEligibility.cd4HvlRank = TryExtractTemplateParameter<int>(
				eligibility_node.select_single_node("criteria[name=\"CD4HVL\"]/rank").node(),
				TemplateParameter::CD4HVLRank, parameter_keys);
			parameters.rolloutEligibility.cd4HvlCd4Bounds[0] = TryExtractTemplateParameter<int>(
				eligibility_node.select_single_node("criteria[name=\"CD4HVL\"]/CD4Lwr").node(),
				TemplateParameter::CD4HVLCD4Lwr, parameter_keys);
			parameters.rolloutEligibility.cd4HvlCd4Bounds[1] = TryExtractTemplateParameter<int>(
				eligibility_node.select_single_node("criteria[name=\"CD4HVL\"]/CD4Upp").node(),
				TemplateParameter::CD4HVLCD4Upp, parameter_keys);
			parameters.rolloutEligibility.cd4HvlHvlBounds[0] = TryExtractTemplateParameter<int>(
				eligibility_node.select_single_node("criteria[name=\"CD4HVL\"]/HVLLwr").node(),
				TemplateParameter::CD4HVLHVLLwr, parameter_keys);
			parameters.rolloutEligibility.cd4HvlHvlBounds[1] = TryExtractTemplateParameter<int>(
				eligibility_node.select_single_node("criteria[name=\"CD4HVL\"]/HVLUpp").node(),
				TemplateParameter::CD4HVLHVLUpp, parameter_keys);

			for(auto target : interventions_node.select_nodes("artRolloutIntervention/targetRolloutProportions/target"))
			{
				parameters.targetYearlyRolloutProportions[target.node().attribute("year").as_int()] = target.node().text().as_double();
			}

			parameters.cepacTracer = new Tracer(parameters.simName, parameters.rolloutSimContexts[0]->rolloutSimContext, 1);
			parameters.cepacRunStats = new RunStats(parameters.simName, parameters.rolloutSimContexts[0]->rolloutSimContext);
		}
		else
		{
			parameters.cepacTracer = new Tracer(parameters.simName, parameters.cepacSimContexts[0], 1);
			parameters.cepacRunStats = new RunStats(parameters.simName, parameters.cepacSimContexts[0]);
		}

		parameters.numToTrace =
			source.select_single_node("/simulation/numberToTracePerAgeRange").node().text().as_int();
		parameters.numNewbornsToTrace =
			source.select_single_node("/simulation/numberNewbornsToTrace").node().text().as_int();
		parameters.monthTraceNewborns =
			source.select_single_node("/simulation/monthTraceNewborns").node().text().as_int();
		parameters.tracePrevalentCases =
			source.select_single_node("/simulation/tracePrevalentCases").node().text().as_int() != 0;

		CepacUtil::setRandomSeedType(destination.fixedSeed_ == -1);

		if(destination.fixedSeed_ > -1)
		{
			//Seed is Minnesota Twins retired numbers... yes, I am a dork
			parameters.randomNums.reset(destination.fixedSeed_ == 0 ? 36291434 : destination.fixedSeed_);
		}

		auto population_parameter_keys = Deserialize(source.child("population"), destination.population_);
		for(const auto &parameter_key_pair : population_parameter_keys)
		{
			if(parameter_keys.find(parameter_key_pair.first) != parameter_keys.end())
			{
				throw std::runtime_error("duplicate parameters");
			}

			parameter_keys[parameter_key_pair.first] = parameter_key_pair.second;
		}

		auto misc_interventions_node = source.child("interventions").child("miscellaneousInterventions");

		for(auto intervention_iter : source.select_nodes("population/interventions/miscellaneousInterventions/intervention"))
		{
			auto intervention_node = intervention_iter.node();

			Simulation::TimeDependentParameter parameter;
			parameter.time = intervention_node.attribute("time").as_int();
			parameter.key = std::string(intervention_node.attribute("key").as_string());
			parameter.value = std::string(intervention_node.attribute("value").as_string());

			auto parameter_key_iter = parameter_keys.find(parameter.key);

			if(parameter_key_iter == parameter_keys.end())
			{
				throw std::runtime_error("no matching parameter");
			}

			switch(parameter_key_iter->second)
			{
			case TemplateParameter::CD4CD4Upp:
				parameter.simulation_modifier = [&](const std::string &v) { parameters.rolloutEligibility.cd4Bounds[1] = std::stoi(v); };
				break;
			case TemplateParameter::OIHistRank:
				parameter.simulation_modifier = [&](const std::string &v) { parameters.rolloutEligibility.oiHistRank = std::stoi(v); };
				break;
			case TemplateParameter::CD4Rank:
				parameter.simulation_modifier = [&](const std::string &v) { parameters.rolloutEligibility.cd4Rank = std::stoi(v); };
				break;
			case TemplateParameter::OIHistOI3:
				parameter.simulation_modifier = [&](const std::string &v) { parameters.rolloutEligibility.oiHistOIs[3] = std::stoi(v); };
				break;
			case TemplateParameter::OIHistOI4:
				parameter.simulation_modifier = [&](const std::string &v) { parameters.rolloutEligibility.oiHistOIs[4] = std::stoi(v); };
				break;
			case TemplateParameter::OIHistOI5:
				parameter.simulation_modifier = [&](const std::string &v) { parameters.rolloutEligibility.oiHistOIs[5] = std::stoi(v); };
				break;
			case TemplateParameter::OIHistOI11:
				parameter.simulation_modifier = [&](const std::string &v) { parameters.rolloutEligibility.oiHistOIs[11] = std::stoi(v); };
				break;
			}

			destination.time_dependent_parameters_[parameter_key_iter->second] = parameter;
		}

		return parameter_keys;
	}

	template<>
	std::unordered_map<std::string, TemplateParameter> Deserialize(const pugi::xml_node &source, Population &destination)
	{
		destination.Deserialize(source);
		return std::unordered_map<std::string, TemplateParameter>();
	}

private:
	template<typename T>
	T TryExtractTemplateParameter(const pugi::xml_node &node, TemplateParameter parameter, std::unordered_map<std::string, TemplateParameter> &parameter_keys);

	template<>
	bool TryExtractTemplateParameter(const pugi::xml_node &node, TemplateParameter parameter, std::unordered_map<std::string, TemplateParameter> &parameter_keys)
	{
		return std::stoi(TryExtractTemplateParameter<std::string>(node, parameter, parameter_keys)) != 0;
	}

	template<>
	int TryExtractTemplateParameter(const pugi::xml_node &node, TemplateParameter parameter, std::unordered_map<std::string, TemplateParameter> &parameter_keys)
	{
		return std::stoi(TryExtractTemplateParameter<std::string>(node, parameter, parameter_keys));
	}

	template<>
	double TryExtractTemplateParameter(const pugi::xml_node &node, TemplateParameter parameter, std::unordered_map<std::string, TemplateParameter> &parameter_keys)
	{
		return std::stod(TryExtractTemplateParameter<std::string>(node, parameter, parameter_keys));
	}

	template<>
	std::string TryExtractTemplateParameter(const pugi::xml_node &node, TemplateParameter parameter, std::unordered_map<std::string, TemplateParameter> &parameter_keys)
	{
		std::string node_text(node.text().as_string());
		if(node_text.front() == '{' && node_text.back() == '}')
		{
			auto comma_index = node_text.find(',');
			auto key = node_text.substr(1, comma_index - 1);
			auto initial_value = node_text.substr(comma_index + 1, node_text.length() - comma_index - 2);
			parameter_keys[key] = parameter;
			return initial_value;
		}
		return node_text;
	}
};