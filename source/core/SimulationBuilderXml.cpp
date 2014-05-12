#include <boost/filesystem.hpp>

#include "SimulationBuilderXml.h"
#include "../util/enum_iterator.h"

#ifdef __APPLE__
namespace std {
template <typename T, typename... Args>
auto make_unique(Args&&... args) -> std::unique_ptr<T>
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
}
#endif

namespace {
std::string to_string(SexualPartnership::Type type)
{
	switch(type)
	{
	case SexualPartnership::Type::Steady: return "Steady";
	case SexualPartnership::Type::Regular: return "Regular";
	case SexualPartnership::Type::Casual: return "Casual";
	case SexualPartnership::Type::Csw: return "Csw";
	default: throw std::runtime_error("unknown type");
	}
}
}

template<>
bool SimulationBuilderXml::from_string(const std::string &value_string)
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
int SimulationBuilderXml::from_string(const std::string &value_string)
{
	return std::stoi(value_string);
}

template<>
double SimulationBuilderXml::from_string(const std::string &value_string)
{
	return std::stod(value_string);
}

template<>
std::string SimulationBuilderXml::from_string(const std::string &value)
{
	return value;
}

template<>
std::array<double, 7> SimulationBuilderXml::from_string(const std::string &value_string)
{
	std::array<double, 7> values;
	std::stringstream ss(value_string);
	for(int i = 0; i < 7; i++)
	{
		ss >> values[i];
	}
	return values;
}

void SimulationBuilderXml::Reset()
{
	parameters_.clear();
}

void SimulationBuilderXml::SetInputFile(const std::string &filename)
{
	document_.load_file(filename.c_str());
	simulation_.SetName(boost::filesystem::path(filename).stem().string());
}

void SimulationBuilderXml::CheckVersion()
{
	auto version_string = Attr<std::string>(document_.child("simulation"), "version");
	auto version = Version::FromString(version_string);

	if(Version::Compare(version, Utility::MODEL_VERSION, true) != 0)
	{
		throw std::runtime_error("bad input version");
	}
}

void SimulationBuilderXml::LoadTemplateParameters()
{
	auto simulation_node = document_.child("simulation");

	for(auto intervention_iter : simulation_node.select_nodes("/simulation/interventions/generalInterventions/intervention"))
	{
		auto intervention_node = intervention_iter.node();

		Parameter parameter;
		parameter.time = Attr<int>(intervention_node, "time");
		parameter.value = Attr<std::string>(intervention_node, "value");
		auto key = Attr<std::string>(intervention_node, "key");

		for(auto attribute : intervention_node.attributes())
		{
			std::string name(attribute.name());
			if(name.substr(0, 7) != "target-")
			{
				continue;
			}
			parameter.target.has_value = true;
			std::string value(attribute.as_string());
			if(name == "target-gender")
			{
				parameter.target.value.gender.has_value = true;
				if(value == "male")
				{
					parameter.target.value.gender.value = DemographicProfile::MALE;
				}
				else if(value == "female")
				{
					parameter.target.value.gender.value = DemographicProfile::FEMALE;
				}
			}
			else if(name == "target-employment")
			{
				parameter.target.value.employment.has_value = true;
				if(value == "csw")
				{
					parameter.target.value.employment.value = DemographicProfile::CSW;
				}
				else if(value == "non-csw")
				{
					parameter.target.value.employment.value = DemographicProfile::NON_CSW;
				}
			}
			else if(name == "target-risk-group")
			{
				parameter.target.value.risk_level.has_value = true;
				if(value == "high")
				{
					parameter.target.value.risk_level.value = Person::HIGH;
				}
				else if(value == "low")
				{
					parameter.target.value.risk_level.value = Person::LOW;
				}
			}
			else if(name == "target-age")
			{
				parameter.target.value.age_lower.has_value = true;
				parameter.target.value.age_upper.has_value = true;
				std::string age_range = value;
				auto hyphen_index = age_range.find('-');
				if(hyphen_index == std::string::npos)
				{
					throw std::runtime_error("age range must be of the form [lower]-[upper]");
				}
				parameter.target.value.age_lower.value = std::stoi(age_range.substr(0, hyphen_index));
				parameter.target.value.age_upper.value = std::stoi(age_range.substr(hyphen_index + 1));
				if(parameter.target.value.age_lower.value > parameter.target.value.age_upper.value)
				{
					throw std::runtime_error("age range lower bound must be less than upper bound");
				}
				if(parameter.target.value.age_lower.value < 0)
				{
					throw std::runtime_error("age range lower bound must be >= 0");
				}
			}
			else if(name == "target-sexual-activity-status")
			{
				parameter.target.value.gender.has_value = true;
				if(value == "active")
				{
					parameter.target.value.gender.value = DemographicProfile::MALE;
				}
				else if(value == "not-active")
				{
					parameter.target.value.gender.value = DemographicProfile::FEMALE;
				}
			}
			else if(name == "target-sexual-orientation")
			{
				parameter.target.value.sexual_orientation.has_value = true;
				if(value == "hetero")
				{
					parameter.target.value.sexual_orientation.value = DemographicProfile::HETERO;
				}
				else if(value == "homo")
				{
					parameter.target.value.sexual_orientation.value = DemographicProfile::HOMO;
				}
			}
			else if(name == "target-relationship-status")
			{
				parameter.target.value.relationship_status.has_value = true;
				if(value == "single")
				{
					parameter.target.value.relationship_status.value = DemographicProfile::SINGLE;
				}
				else if(value == "non-single")
				{
					parameter.target.value.relationship_status.value = DemographicProfile::NON_SINGLE;
				}
			}
			else if(name == "target-treatment-status")
			{
				parameter.target.value.on_treatment.has_value = true;
				if(value == "treated")
				{
					parameter.target.value.on_treatment.value = true;
				}
				else if(value == "untreated")
				{
					parameter.target.value.on_treatment.value = false;
				}
			}
			else if(name == "target-hiv-status")
			{
				parameter.target.value.observed_hiv_status.has_value = true;
				if(value == "observed-acute")
				{
					parameter.target.value.observed_hiv_status.value = Person::OBSERVED_ACUTE;
				}
				else if(value == "observed-chronic")
				{
					parameter.target.value.observed_hiv_status.value = Person::OBSERVED_CHRONIC;
				}
				else if(value == "observed-late-stage")
				{
					parameter.target.value.observed_hiv_status.value = Person::OBSERVED_LATESTAGE;
				}
				else if(value == "unobserved-acute")
				{
					parameter.target.value.observed_hiv_status.value = Person::UNOBSERVED_ACUTE;
				}
				else if(value == "unobserved-chronic")
				{
					parameter.target.value.observed_hiv_status.value = Person::UNOBSERVED_CHRONIC;
				}
				else if(value == "unobserved-late-stage")
				{
					parameter.target.value.observed_hiv_status.value = Person::UNOBSERVED_LATESTAGE;
				}
			}
			else
			{
				throw std::runtime_error("unknown target type: " + name);
			}
		}

		parameters_[key].push_back(parameter);
	}
}

void SimulationBuilderXml::ReadSimulationParameters()
{
	auto simulation_node = document_.child("simulation");
	auto &parameters = simulation_.GetEventParams();

	simulation_.SetFixedSeed(Text<int>(simulation_node.child("fixedSeed")));
	simulation_.SetDuration(Text<int>(simulation_node.child("duration")));

	parameters.debugLevel = static_cast<DebugLevel>(Text<int>(simulation_node.child("debugLevel")));
	parameters.monthOf1990 = Text<int>(simulation_node.child("monthOf1990"));
	parameters.delayPrevalence = Text<int>(simulation_node.child("population").child("initialState").child("delay"));

	//save Concurrency Definitions
	auto concurrency_node = simulation_node.child("concurrencyDefinition");
	for(int i = 0; i < 16; i++)
	{
		auto definition_node = concurrency_node.find_child_by_attribute("definition", "id", std::to_string(i).c_str());
		auto &definition = parameters.concurrencyDef[i];
		definition.minPartnershipsNeeded = Text<int>(definition_node.child("minNeeded"));
		definition.useDefinition = Text<bool>(definition_node.child("allow"));
	}

    static const std::map<EventParams::TraceFile::Type, std::string> trace_files =
    {
        {EventParams::TraceFile::Type::Population, "population"},
        {EventParams::TraceFile::Type::Infection, "infection"},
        {EventParams::TraceFile::Type::Partnership, "partnership"},
        {EventParams::TraceFile::Type::Survival, "survival"},
        {EventParams::TraceFile::Type::CostEffectiveness, "costEffectiveness"},
        {EventParams::TraceFile::Type::Clinical, "clinical"},
        {EventParams::TraceFile::Type::Events, "events"},
        {EventParams::TraceFile::Type::Health, "health"},
        {EventParams::TraceFile::Type::SinglePerson, "singlePerson"},
        {EventParams::TraceFile::Type::LifeExpectancy, "lifeExpectancy"},
        {EventParams::TraceFile::Type::PartnerAcquisition, "partnerAcquisition"},
        {EventParams::TraceFile::Type::CalibrationStatistics, "calibrationStatistics"},
        {EventParams::TraceFile::Type::ArtRollout, "artRollout"},
        {EventParams::TraceFile::Type::ShiftedOutcomes, "shiftedOutcomes"}
    };

	auto trace_files_node = simulation_node.child("traceFiles");
	for(auto trace_file : trace_files)
	{
		auto trace_file_node = trace_files_node.child(trace_file.second.c_str());
		parameters.trace_files[trace_file.first].enabled = Attr<bool>(trace_file_node, "enabled");
        parameters.trace_files[trace_file.first].extension = Text<std::string>(trace_file_node.child("extension"));
        std::string fileName = "results/" + parameters.simName + "-" + parameters.trace_files[trace_file.first].extension;
        parameters.trace_files[trace_file.first].file.open(fileName, ios::out);
        parameters.trace_files[trace_file.first].toss = Attr<bool>(trace_file_node, "tossIfCalibFail");
	}

	parameters.numToTrace = Text<int>(simulation_node.child("traceFiles").child("singlePerson").child("numberToTracePerAgeRange"));
	parameters.numNewbornsToTrace = Text<int>(simulation_node.child("traceFiles").child("singlePerson").child("numberNewbornsToTrace"));
	parameters.monthTraceNewborns = Text<int>(simulation_node.child("traceFiles").child("singlePerson").child("monthTraceNewborns"));
	parameters.tracePrevalentCases = Text<bool>(simulation_node.child("traceFiles").child("singlePerson").child("tracePrevalentCases"));

	for(auto time_node : simulation_node.child("traceFiles").child("lifeExpectancy").children("time"))
	{
		simulation_.AddLifeExpectancyRecordTime(Text<int>(time_node));
	}

	auto calibration_node = simulation_node.child("calibration");
	parameters.calibrationInputs.useCalibration = Attr<bool>(calibration_node, "enabled");

	if(parameters.calibrationInputs.useCalibration)
	{
		auto &calib = parameters.calibrationInputs;
		calib.monthOfCalibration = Text<int>(calibration_node.child("monthOfCalibration"));
		calib.steadyPrevPopulation = Text<int>(calibration_node.child("monthOfCalibration"));
		calib.steadyPrevBounds.lower = Text<double>(calibration_node.child("monthOfCalibration"));
		calib.steadyPrevBounds.upper = Text<double>(calibration_node.child("monthOfCalibration"));
		calib.casualPrevPopulation = Text<int>(calibration_node.child("monthOfCalibration"));
		calib.casualPrevBounds.lower = Text<double>(calibration_node.child("monthOfCalibration"));
		calib.casualPrevBounds.upper = Text<double>(calibration_node.child("monthOfCalibration"));
		calib.CSWPrevPopulation = Text<int>(calibration_node.child("monthOfCalibration"));
		calib.CSWPrevBounds.lower = Text<double>(calibration_node.child("monthOfCalibration"));
		calib.CSWPrevBounds.upper = Text<double>(calibration_node.child("monthOfCalibration"));
		calib.propInConcurrentPopulation = Text<int>(calibration_node.child("monthOfCalibration"));
		calib.propInConcurrentBounds.lower = Text<double>(calibration_node.child("monthOfCalibration"));
		calib.propInConcurrentBounds.upper = Text<double>(calibration_node.child("monthOfCalibration"));
		calib.numActsPopulation = Text<int>(calibration_node.child("monthOfCalibration"));
		calib.numActsBounds.lower = Text<double>(calibration_node.child("monthOfCalibration"));
		calib.numActsBounds.upper = Text<double>(calibration_node.child("monthOfCalibration"));
		calib.femaleCasualPrevRatio = Text<double>(calibration_node.child("monthOfCalibration"));
		calib.femalePropInConcurrentRatio = Text<double>(calibration_node.child("monthOfCalibration"));
		calib.femaleNumActsLRtoHRRatio = Text<double>(calibration_node.child("monthOfCalibration"));

		for(int i = 0; i < Constants::NUMBER_CALIBRATION_PREVS; i++)
		{
			auto name = "time" + std::to_string(i);
			calib.calendarPrevs[i] = Text<double>(calibration_node.child("calendarPrevalence").child(name.c_str()));
		}

		for(int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
		{
			auto name = "storePoint" + std::to_string(i) + "Mth";
			calib.saveStateTimePoints[i] = Text<int>(calibration_node.child(name.c_str()));
		}

		calib.thresholdPrevMult = Text<double>(calibration_node.child("thresholdMultiplier"));
	}

	auto interventions_node = simulation_node.child("interventions");
	parameters.useRollout = Attr<bool>(interventions_node.child("artRolloutIntervention"), "enabled");

	if(parameters.useRollout)
	{
		auto scaling_node = interventions_node.child("artRolloutIntervention").child("dynamicTreatmentScaling");
		parameters.dynamicFeedbackPeriod = Text<int>(scaling_node.child("feedbackPeriod"));
		parameters.enableDynamicTreatmentScaling = Attr<bool>(scaling_node, "enabled");

		for(auto treatment_file_node : interventions_node.select_nodes("artRolloutIntervention/rolloutTreatmentFiles/rolloutFile"))
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
				auto contextToAdd = std::make_unique<SimContext>(file_name.substr(0, file_name.find(CepacUtil::FILE_EXTENSION_FOR_INPUT)));
				parameters.rolloutSimContexts.push_back(new EventParams::RolloutContext(time, std::move(contextToAdd), target_population));
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
					Person::probDeathNatCauses[DemographicProfile::MALE] = probabilities[0];
					Person::probDeathNatCauses[DemographicProfile::FEMALE] = probabilities[1];
				}
			}
		}

		parameters.rolloutEligibility = ReadRolloutEligibility();

		for(auto target : interventions_node.select_nodes("artRolloutIntervention/targetRolloutProportions/target"))
		{
			auto year = Attr<int>(target.node(), "year");
			parameters.targetYearlyRolloutProportions[year] = Text<double>(target.node());
		}

		parameters.cepacTracer = new Tracer(parameters.simName, parameters.rolloutSimContexts[0]->rolloutSimContext.get(), 1);
		parameters.cepacRunStats = new RunStats(parameters.simName, parameters.rolloutSimContexts[0]->rolloutSimContext.get());
	}
	else
	{
        for(auto treatment_file_node : interventions_node.select_nodes("cepacIntervention/cepacTreatmentFiles/treatmentFile"))
        {
            int time = treatment_file_node.node().child("time").text().as_int();

            if(time > -1)
            {
                std::string file_name = treatment_file_node.node().child("fileName").text().as_string();
                int file_number = treatment_file_node.node().child("fileNumber").text().as_int();

                parameters.timesToSwitchSimContext[file_number] = time;

                //Make sure the number of CEPAC input files from the .xml file is not greater than the number expected by the code!
                assert(file_number < Constants::NUMBER_OF_CEPAC_FILES);

                //Set the CEPAC simContext from the specified CEPAC .in file
                auto contextToAdd = new SimContext(file_name.substr(0, file_name.find(CepacUtil::FILE_EXTENSION_FOR_INPUT)));
                contextToAdd->numPatientsToTrace = 0;
                parameters.cepacSimContexts.push_back(contextToAdd);

                //Read in the inputs
                try
                {
                    contextToAdd->readInputs();
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
                    Person::probDeathNatCauses[DemographicProfile::MALE] = probabilities[0];
                    Person::probDeathNatCauses[DemographicProfile::FEMALE] = probabilities[1];
                }
            }
        }

		parameters.cepacTracer = new Tracer(parameters.simName, parameters.cepacSimContexts[0], 1);
		parameters.cepacRunStats = new RunStats(parameters.simName, parameters.cepacSimContexts[0]);
	}
}

Simulation &SimulationBuilderXml::GetResult()
{
	return simulation_;
}

void SimulationBuilderXml::InitializePopulation()
{
	auto &population = simulation_.GetPopulation();

	population.SetParameters(population_parameters);

	//Get the initial marriage prevalence based on percent male high risk and rate and duration of steady relationships
	double pHigh = population_parameters.GetMaleParameters().getProportionHighRisk(DemographicProfile::NON_CSW);
	double marriageRateH = population_parameters.GetMaleParameters().getSexualBehavior(SexualPartnership::Type::Steady).getAcquisitionRatePerMonth(
		Person::HIGH).getMean();
	double marriageRateL = population_parameters.GetMaleParameters().getSexualBehavior(SexualPartnership::Type::Steady).getAcquisitionRatePerMonth(
		Person::LOW).getMean();
	double marriageDurationH = population_parameters.GetMaleParameters().getSexualBehavior(
		SexualPartnership::Type::Steady).getPartnershipDurationMth(Person::HIGH).getMean();
	double marriageDurationL = population_parameters.GetMaleParameters().getSexualBehavior(
		SexualPartnership::Type::Steady).getPartnershipDurationMth(Person::LOW).getMean();
	auto proportion_married = (1 - pHigh) * (marriageRateL * marriageDurationL) / (1 + marriageRateL *
		marriageDurationL) + pHigh * (marriageRateH * marriageDurationH) / (1 + marriageRateH * marriageDurationH);
	//Get the initial regular prevalence based on percent male high risk and rate and duration of regular relationships
	double regularRateH = population_parameters.GetMaleParameters().getSexualBehavior(SexualPartnership::Type::Regular).getAcquisitionRatePerMonth(
		Person::HIGH).getMean();
	double regularRateL = population_parameters.GetMaleParameters().getSexualBehavior(SexualPartnership::Type::Regular).getAcquisitionRatePerMonth(
		Person::LOW).getMean();
	double regularDurationH = population_parameters.GetMaleParameters().getSexualBehavior(
		SexualPartnership::Type::Regular).getPartnershipDurationMth(Person::HIGH).getMean();
	double regularDurationL = population_parameters.GetMaleParameters().getSexualBehavior(
		SexualPartnership::Type::Regular).getPartnershipDurationMth(Person::LOW).getMean();
	auto proportion_regular = (1 - pHigh) * (regularRateL * regularDurationL) + pHigh *
		(regularRateH * regularDurationH);

	//create EntityPool - this will contain all Entities
	auto entities = std::make_unique<EntityPool>(population_parameters.getAgeSexualDebut(), population.GetId(), population_parameters.GetAssortativeness());
	population.entities.swap(entities);

	//initialize infection trace generator print detailed info about certain ProfileID's
	// in this case, all ProfileID's w/ non-nullptr BucketDemographicProfiles
	DemographicProfile::ProfileID currProfileID = DemographicProfile::MIN;

	while(currProfileID <= DemographicProfile::MAX)
	{
		//if it is being used in this Population, then append to _profileIDs
		if((population.entities->getBucket(currProfileID) != nullptr) &&
			(DemographicProfile::get(currProfileID, DemographicProfile::SEXUAL_ACTIVITY_STATUS) != DemographicProfile::NA))
		{
			population.GetPopulationStatistics().infectionsTracker.addToDetailedTrace(currProfileID);
		}

		currProfileID++;
	}

	//initialize structures that hold people who can initiate and 'agree' to relationships.
	population.InitPartnershipBuckets();

	if(population.parameters_.useRollout)
	{
		population.ApplyRolloutContext(population.parameters_, 0);
	}

	// Create the people in the population 
	long totalNumMales = Utility::round<long>(population_parameters.GetInitialSize() * population_parameters.GetMaleProportion());
	long totalNumFemales = std::max<long>(population_parameters.GetInitialSize() - totalNumMales, 0);
	//the params.xml file should have detailed the prevalent characteristics of each age bucket
	//  we will go through each age bucket and create the part of the prevalent population that falls within the bucket
	std::vector<AgeRange> ageRanges;

	for(auto ageBucketParams : population_parameters.GetInitialAgeBuckets())
	{
		auto numMalesInCurrentBucket = Utility::round<std::size_t>(totalNumMales * ageBucketParams.proportionOfPopulation[DemographicProfile::MALE]);
		auto numFemalesInCurrentBucket = Utility::round<std::size_t>(totalNumFemales * ageBucketParams.proportionOfPopulation[DemographicProfile::FEMALE]);

		//calc how many people are in the current age range
		auto currentBucketSize = numMalesInCurrentBucket + numFemalesInCurrentBucket;

		//Number of persons of each gender to be traced in detailed output file
		auto numToTrace = static_cast<std::size_t>(population.GetNumberToTrace());

		for(std::size_t count = 0; count < currentBucketSize; count++)
		{
			//Determine whether or not person should be traced in SinglePersonTrace
			bool tracePerson = (count < numToTrace || (count >= numMalesInCurrentBucket && (count - numMalesInCurrentBucket) < numToTrace));

			//create a person, males first and females second
			auto gender = (count < numMalesInCurrentBucket) ? DemographicProfile::MALE : DemographicProfile::FEMALE;
			auto person = population.GeneratePerson(simulation_.GetEventParams(), gender, &ageBucketParams, tracePerson);

			//add the created person to the EntityPool
			population.entities->addPersonToAll(person);
		}

		AgeRange ageRange = {ageBucketParams.minAgeMth, ageBucketParams.maxAgeMth};
		AgeRangeSizePair ageRangeSize = std::make_pair(ageRange, currentBucketSize);

		//Add a tuple to the currSizeByAgeRange vector along with the initial size of the age range
		ageRanges.push_back(ageRange);
		population.currSizeByAgeRange.push_back(ageRangeSize);
		population.currSizeByAgeRangeMale.push_back(std::make_pair(ageRange, numMalesInCurrentBucket));
		population.currSizeByAgeRangeFemale.push_back(std::make_pair(ageRange, numFemalesInCurrentBucket));
	}

    population.populationStatistics.artTracker.SetAgeRanges(ageRanges);

	if(population.parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled)
	{
		population.parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << std::endl << "Now creating initial partnerships... " << std::endl;
	}

	// create prevalent Regular Partnerships (time = 0) before creating prevalent marriages
	//the demographics that we are pulling the eligibles from
	DemographicProfile selector;
	selector.set(DemographicProfile::SEXUAL_ACTIVITY_STATUS, DemographicProfile::SA);
	selector.set(DemographicProfile::GENDER, DemographicProfile::MALE);
	selector.set(DemographicProfile::SEXUAL_ORIENTATION, DemographicProfile::HETERO);
	selector.set(DemographicProfile::RELATIONSHIP_STATUS, DemographicProfile::SINGLE);
	selector.set(DemographicProfile::EMPLOYMENT, DemographicProfile::NON_CSW);
	std::vector<DemographicProfile::ProfileID> bucketIDs;
	//TODO:fix code below, i've put placeholders for multiple singles buckets, but right now we only use 1 of each gender
	//errhode: Is this taken care of with the whole agebucket inside SexualMixingBucket thing?
	selector.selectProfileIDs(bucketIDs, nullptr);
	BucketDemographicProfile *singleMales = population.entities->getBucket(bucketIDs.at(0));
	selector.set(DemographicProfile::GENDER, DemographicProfile::FEMALE);
	bucketIDs.clear();
	selector.selectProfileIDs(bucketIDs, nullptr);
	BucketDemographicProfile *singleFemales = population.entities->getBucket(bucketIDs.at(0));
	// create prevalent formSteadyPartnerships (time = 0)
	//the demographics that we are pulling the eligibles from -- same as for regular;
	//can just use the previous singleMales and singleFemales buckets
	//number of couples -- % married of adult population by DemographicProfile::SAStatus / 2
	int numCouples = Utility::round<long>(proportion_married * (singleMales->size() + singleFemales->size()) * 0.5);

	while(numCouples > 0)
	{
		//break if there are no more people to marry...
		if(!singleFemales->size() || !singleMales->size())
		{
			break;
		}

		//choose a random male from the pool
		Male *m = (Male *)singleMales->drawMember(population.parameters_.randomNums, SexualPartnership::Type::Steady, false);

		if(m == nullptr)
		{
			break;
		}

		//try to form partnership, will add Male back to the pool if partnership was formed
		population.CreatePartnerships(population.parameters_, m, nullptr, SexualPartnership::Type::Steady, true);
		numCouples--;
	} //while(numCouples > 0) {

	//number of regular couples -- % married of adult population by DemographicProfile::SAStatus / 2
	//Note that some people may end up in multiple relationships -- this should come out in the wash (?)
	numCouples = Utility::round<int>(proportion_regular * (singleMales->size() + singleFemales->size()) * 0.5);

	while(numCouples > 0)
	{
		//break if there are no more people to pair off...
		//this shouldn't be a problem unless we start with no men or no women as we are not shifting the pairs to non_single status
		if(!singleFemales->size() || !singleMales->size())
		{
			break;
		}

		//choose a random male from the pool
		Male *m = (Male *)singleMales->drawMember(population.parameters_.randomNums, SexualPartnership::Type::Regular, false);

		if(m == nullptr)
		{
			break;
		}

		//form partnership, will add Male back to the pool if partnership was formed
		population.CreatePartnerships(population.parameters_, m, nullptr, SexualPartnership::Type::Regular, true);
		numCouples--;
	} //while(numCouples > 0) {

	//count the size of the population and store value
	population.UpdateSize();
}

EventParams::RolloutEligibility SimulationBuilderXml::ReadRolloutEligibility()
{
    auto eligibility_node = document_.select_single_node("/simulation/interventions/artRolloutIntervention/rolloutEligibility").node();
    EventParams::RolloutEligibility eligibility;

    // OIHist
    auto oi_hist_node = eligibility_node.select_single_node("criteria[@name='OIHist']").node();
    eligibility.oiHistRank = Text<int>(oi_hist_node.child("rank"));
    eligibility.oiHistNumToStart = Text<int>(oi_hist_node.child("numOIToStart"));

    // CD4
    auto cd4_node = eligibility_node.select_single_node("criteria[@name='CD4']").node();
    eligibility.cd4Rank = Text<int>(cd4_node.child("rank"));
    eligibility.cd4Bounds.lower = Text<int>(cd4_node.child("CD4Lwr"));
    eligibility.cd4Bounds.upper = Text<int>(cd4_node.child("CD4Upp"));

    // CD4OIHist
    auto cd4_oi_hist_node = eligibility_node.select_single_node("criteria[@name='CD4OIHist']").node();
    eligibility.cd4OiHistRank = Text<int>(cd4_oi_hist_node.child("rank"));
    eligibility.cd4OiHistCd4Bounds.lower = Text<int>(cd4_oi_hist_node.child("CD4Lwr"));
    eligibility.cd4OiHistCd4Bounds.upper = Text<int>(cd4_oi_hist_node.child("CD4Upp"));

    // HVL
    auto hvl_node = eligibility_node.select_single_node("criteria[@name='HVL']").node();
    eligibility.hvlRank = Text<int>(hvl_node.child("rank"));
    eligibility.hvlBounds.lower = Text<int>(hvl_node.child("HVLLwr"));
    eligibility.hvlBounds.upper = Text<int>(hvl_node.child("HVLUpp"));

    // CD4HVL
    auto cd4_hvl_node = eligibility_node.select_single_node("criteria[@name='CD4HVL']").node();
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

SexualBehavior SimulationBuilderXml::ReadSexualBehavior(SexualPartnership::Type type)
{
	auto path = "/simulation/population/entities/entity[@type='Male']/behavior/partnershipTypes/partnership[@type='" + to_string(type) + "']";
	auto node = document_.select_single_node(path.c_str()).node();

	SexualBehavior result(type);

	population_parameters.setAssortativeness(type, Text<double>(node.child("assortativeness")));

	auto bucket_path = "selectionCriteria/availableBuckets/bucket";
	for(const auto &bucket_settings : node.select_nodes(bucket_path))
	{
		SexualBehavior::AvailableBucket bucket;
		bucket.dmgProfileSelector.parse(Text<std::string>(bucket_settings.node().child("profile")));
		bucket.weight = Text<double>(bucket_settings.node().child("weight"));
		result.AddAvailableBucket(bucket);
	}

	result.setAverageYearsYounger(Text<NormalDist>(node.child("selectionCriteria").child("averageYearsYounger")));

	for(auto risk : {Person::LOW, Person::HIGH})
	{
		std::string suffix = risk == Person::LOW ? "LowRisk" : "HighRisk";

		result.setAcquisitionRatePerMonth(risk, Text<LogNormalDist>(node.child(("acquisitionRate" + suffix).c_str())));
		//XXX:this should be a double, but old implementations mistakenly casted it to int
		//we will continue to do this to maintain reproduciblity for now
		result.setCoitalEventsPerMonth(risk, Text<int>(node.child(("coitalEventsPerMonth" + suffix).c_str()).child("distribution").child("mean")));
		result.setChanceCondomUsePerEvent(risk, Text<BetaDist>(node.child(("chanceCondomUsePerEvent" + suffix).c_str())));
		result.setPartnershipDuration(risk, Text<ShiftedLogNormalDist>(node.child(("partnershipDurationMth" + suffix).c_str())));
	}

	return result;
}

Male::SubPopParams SimulationBuilderXml::ReadMaleSubPopParams()
{
	auto node = document_.select_single_node("/simulation/population/entities/entity[@type='Male']").node();

	Male::SubPopParams result;

	auto behavior_node = node.child("behavior");
    result.setChanceBecomeCsw(Text<double>(behavior_node.child("chanceBecomeSexWorker")));
	result.setPartnerAcqMultWithSteady(Person::HIGH, Text<double>(behavior_node.child("partnerAcqMultWithSteadyHighRisk")));
	result.setPartnerAcqMultWithSteady(Person::LOW, Text<double>(behavior_node.child("partnerAcqMultWithSteadyLowRisk")));

	bool use_high_risk_multiplier = Attr<bool>(behavior_node.child("highRiskAcqRateMultiplier"), "enabled");
	double high_risk_multiplier = Text<double>(behavior_node.child("highRiskAcqRateMultiplier"));
	bool use_csw_high_risk_multiplier = Attr<bool>(behavior_node.child("highRiskCswAcqRateMultiplier"), "enabled");
	double csw_high_risk_multiplier = Text<double>(behavior_node.child("highRiskCswAcqRateMultiplier"));

	result.setCoefficientVariation(false, 0);

	for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
	{
		auto params = ReadSexualBehavior(partnership_type);

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

		result.addSexualBehavior(params);
	}

	NormalDist activityLevel;
	activityLevel.mean = 1;
	activityLevel.stddev = 0;
	result.setActivityLevel(activityLevel);

	result.setProportionHighRisk(DemographicProfile::CSW, Text<double>(behavior_node.child("proportionHighRiskCsw")));
    result.setProportionHighRisk(DemographicProfile::NON_CSW, Text<double>(behavior_node.child("proportionHighRiskNonCsw")));

	auto discountingStartAgeYrs = Text<int>(behavior_node.child("ageDiscounting").child("startAgeYrs"));
	auto acquisitionDiscByYr = Text<double>(behavior_node.child("ageDiscounting").child("acquisitionDiscByYr"));
	auto coitalActsDiscByYr = Text<double>(behavior_node.child("ageDiscounting").child("coitalActsDiscByYr"));
	result.setAgeDiscounting(discountingStartAgeYrs, acquisitionDiscByYr, coitalActsDiscByYr);

	auto health_node = node.child("health");
	result.setCircucmsionProtectEfficacy(Text<double>(health_node.child("circumcisionProtectEfficacy")));
	result.setCondomProtectEff(Text<double>(health_node.child("condomProtectEfficacy")));

	auto transmission_node = node.child("health").child("transmissionCoefficients");
	auto transmission_coefficients = Text<std::array<double, 7>>(transmission_node.child("valsByHVL"));
	for(int i = 0; i < 7; i++)
	{
		result.setTransmitPerEventCoeff(Person::HVLStrata(i), transmission_coefficients[i]);
	}
	result.setTransmitPerEventCoeff(Person::HVL_PRIMARY, Text<double>(transmission_node.child("primary")));
	result.setTransmitPerEventCoeff(Person::HVL_LATESTAGE, Text<double>(transmission_node.child("lateStage")));

	return result;
}


Female::SubPopParams SimulationBuilderXml::ReadFemaleSubPopParams()
{
	auto node = document_.select_single_node("/simulation/population/entities/entity[@type='Female']").node();

	Female::SubPopParams result;

	auto behavior_node = node.child("behavior");
	result.setChanceBecomeCsw(Text<double>(behavior_node.child("chanceBecomeSexWorker")));
	result.setProportionHighRisk(DemographicProfile::NON_CSW, Text<double>(behavior_node.child("proportionHighRiskNonCsw")));
	result.setProportionHighRisk(DemographicProfile::CSW, Text<double>(behavior_node.child("proportionHighRiskCsw")));

	NormalDist activityLevel;
	activityLevel.mean = 1;
	activityLevel.stddev = 0;
	result.setActivityLevel(activityLevel);

	auto transmission_node = node.child("health").child("transmissionCoefficients");
	auto transmission_coefficients = Text<std::array<double, 7>>(transmission_node.child("valsByHVL"));
	for(int i = 0; i < 7; i++)
	{
		result.setTransmitPerEventCoeff(Person::HVLStrata(i), transmission_coefficients[i]);
	}
	result.setTransmitPerEventCoeff(Person::HVL_PRIMARY, Text<double>(transmission_node.child("primary")));
	result.setTransmitPerEventCoeff(Person::HVL_LATESTAGE, Text<double>(transmission_node.child("lateStage")));

	return result;
}

void SimulationBuilderXml::ReadPopulationParameters()
{
	auto population_node = document_.child("simulation").child("population");

	auto initial_state_node = population_node.child("initialState");
	population_parameters.SetInitialSize(Text<int>(initial_state_node.child("size")));

	//get initial age distribution
	for(auto age_bucket_node : initial_state_node.child("ageDistributionYrs").children("range"))
	{
		population_parameters.GetInitialAgeBuckets().emplace_back(
            Utility::convertTime(TimeGranularity::Year, TimeGranularity::Month, Attr<int>(age_bucket_node, "lower")),
            Utility::convertTime(TimeGranularity::Year, TimeGranularity::Month, Attr<int>(age_bucket_node, "upper")) + 11,
			Text<double>(age_bucket_node.child("distribMale")),
			Text<double>(age_bucket_node.child("distribFemale")),
			Text<int>(age_bucket_node.child("numInfectedMaleCsw")),
			Text<int>(age_bucket_node.child("numInfectedFemaleCsw")),
			Text<int>(age_bucket_node.child("numInfectedMaleLowRisk")),
			Text<int>(age_bucket_node.child("numInfectedFemaleLowRisk")),
			Text<int>(age_bucket_node.child("numInfectedMaleHighRisk")),
			Text<int>(age_bucket_node.child("numInfectedFemaleHighRisk")));
	}

	population_parameters.SetInitialCswProportion(DemographicProfile::MALE, Text<double>(initial_state_node.child("chanceBeingCswMale")));
	population_parameters.SetInitialCswProportion(DemographicProfile::FEMALE, Text<double>(initial_state_node.child("chanceBeingCswFemale")));
    population_parameters.SetCswEndAge(DemographicProfile::MALE, Utility::convertTime(TimeGranularity::Year, TimeGranularity::Month, Text<int>(initial_state_node.child("cswEndAgeMale"))));
    population_parameters.SetCswEndAge(DemographicProfile::FEMALE, Utility::convertTime(TimeGranularity::Year, TimeGranularity::Month, Text<int>(initial_state_node.child("cswEndAgeFemale"))));

	//normalize %population values for each age bucket
	std::array<double, DemographicProfile::ENDGender> totalPopulationproportionages;
	totalPopulationproportionages.fill(0);

	//get the total of proportionage values of AgeBucketPrevalencInfo.proportionOfPopulation
	for(int i = 0; i < DemographicProfile::ENDGender; i++)
	{
		for(auto &age_bucket : population_parameters.GetInitialAgeBuckets())
		{
			totalPopulationproportionages[i] += age_bucket.proportionOfPopulation[i];
		}

		//normalize each proportionage value so that the sum of them == 1
		for(auto &age_bucket : population_parameters.GetInitialAgeBuckets())
		{
			age_bucket.proportionOfPopulation[i] /= totalPopulationproportionages[i];
		}
	}

	population_parameters.setBirthRate(Text<double>(population_node.child("birthRate")));
	population_parameters.setProportionMale(Text<double>(population_node.child("proportionMale")));
	population_parameters.setProportionCircumcised(Text<double>(population_node.child("proportionCircumcised")));
	population_parameters.setAgeSexualDebut(Text<int>(population_node.child("ageSexualDebutYrs")), TimeGranularity::Year);

	auto defaultMaleParams = ReadMaleSubPopParams();
	population_parameters.SetMaleParameters(defaultMaleParams);
	auto defaultFemaleParams = ReadFemaleSubPopParams();
	population_parameters.SetFemaleParameters(defaultFemaleParams);

	//save flags to indicate whether particular partnership types have duration or not
	for(auto type : enum_iterator<SexualPartnership::Type>())
	{
		auto has_duration = !(defaultMaleParams.getSexualBehavior(SexualPartnership::Type(type)).getPartnershipDurationMth(Person::LOW).isZeroDistrib)
			&& !(defaultMaleParams.getSexualBehavior(SexualPartnership::Type(type)).getPartnershipDurationMth(Person::HIGH).isZeroDistrib);
		population_parameters.SetPartnershipHasDuration(DemographicProfile::MALE, type, has_duration);
		population_parameters.SetPartnershipHasDuration(DemographicProfile::FEMALE, type, false);
	}

    pugi::xml_node costs_node = document_.select_single_node("/simulation/traceFiles/costEffectiveness").node();

    //Costs
    population_parameters.SetCondomCost(Text<double>(costs_node.child("condomCost")));
    population_parameters.SetCircumcisionCost(Text<double>(costs_node.child("circumcisionCost")));
}

Nullable<TargetGroup::PopulationTarget> ParseGroupEligibility(pugi::xml_node criteria_node)
{
    Nullable<TargetGroup::PopulationTarget> target;

    for(auto criterion_node : criteria_node.children())
    {
        target.has_value = true;

        if(std::string(criterion_node.name()) == "gender")
        {
            target.value.gender.has_value = true;
            std::string gender_string = criterion_node.text().as_string();

            if(gender_string == "male")
            {
                target.value.gender.value = DemographicProfile::MALE;
            }
            else if(gender_string == "female")
            {
                target.value.gender.value = DemographicProfile::FEMALE;
            }
            else
            {
                throw std::runtime_error("invalid gender");
            }
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

SimulationBuilderXml::InterventionsTuple SimulationBuilderXml::ParseInterventions(pugi::xml_node interventions_node)
{
    std::vector<SimulationIntervention> simulation_interventions;
    std::vector<PopulationIntervention> population_interventions;
    std::vector<IndividualIntervention> individual_interventions;

    for(auto intervention_node : interventions_node.children("intervention"))
    {
        auto category = Attr<std::string>(intervention_node, "category");
        auto subcategory = Attr<std::string>(intervention_node, "subcategory");

        if(category == "simulation")
        {
            simulation_interventions.push_back(SimulationIntervention(category, subcategory));
        }
        else if(category == "population")
        {
            population_interventions.push_back(PopulationIntervention(category, subcategory));
        }
        else if(category == "art-rollout-eligibility")
        {
            simulation_interventions.push_back(SimulationIntervention(category, subcategory));
        }
        else if(category == "male-behavior")
        {
            individual_interventions.push_back(IndividualIntervention(category, subcategory));
        }
        else if(category == "male-health")
        {
            individual_interventions.push_back(IndividualIntervention(category, subcategory));
        }
        else if(category == "female-behavior")
        {
            individual_interventions.push_back(IndividualIntervention(category, subcategory));
        }
        else if(category == "female-health")
        {
            individual_interventions.push_back(IndividualIntervention(category, subcategory));
        }
    }

    return std::make_tuple(simulation_interventions, population_interventions, 
        individual_interventions);
}

std::vector<TargetGroup> SimulationBuilderXml::ReadGroups()
{
    pugi::xml_node groups_node = 
        document_.select_single_node("/simulation/interventions/groups").node();
    std::vector<TargetGroup> groups;

    for(auto group_node : groups_node.children("target-group"))
    {
        auto enrollment_period_string = 
            Text<std::string>(group_node.child("enrollment-period"));
        auto enrollment_period = ParseRange(enrollment_period_string);
        bool permanent = Text<bool>(group_node.child("permanent-effect"));
        bool open = Text<bool>(group_node.child("open"));
        auto target = ParseGroupEligibility(group_node.child("eligiblity-criteria"));

        TargetGroup group(enrollment_period.first, enrollment_period.second, 
            open, permanent, target);

        for(auto partition_node : group_node.child("partitions").children("partition"))
        {
            std::string label = Attr<std::string>(partition_node, "label");
            auto proportion = Text<double>(partition_node.child("proportion"));
            auto trace = Text<bool>(partition_node.child("trace"));
            auto interventions = ParseInterventions(partition_node.child("interventions"));

            group.AddPartition(label, trace, proportion, std::get<0>(interventions),
                std::get<1>(interventions), std::get<2>(interventions));
        }

        groups.push_back(group);
    }

    return groups;
}