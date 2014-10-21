#include "simulationbuilderxml.hpp"
#include "utility/enum_iterator.hpp"
#include "utility/filesystem.hpp"
#include "utility/make_unique.hpp"

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
    case SexualPartnership::Type::SteadyMsm: return "steady-msm";
    case SexualPartnership::Type::RegularMsm: return "regular-msm";
    case SexualPartnership::Type::CasualMsm: return "casual-msm";
    case SexualPartnership::Type::CswMsm: return "csw-msm";
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

NormalDist SimulationBuilderXml::GetNormalDist(const pugi::xml_node node)
{
    NormalDist dist;
    auto dist_node = node.child("distribution");
    dist.mean = Text<double>(dist_node.child("mean"));
    dist.stddev = Text<double>(dist_node.child("stdDev"));
    return dist;
}

LogNormalDist SimulationBuilderXml::GetLogNormalDist(const pugi::xml_node node)
{
    auto dist = GetNormalDist(node);
    return LogNormalDist::FromNormal(dist);
}

BetaDist SimulationBuilderXml::GetBetaDist(const pugi::xml_node node)
{
    auto dist = GetNormalDist(node);
    return BetaDist::FromNormal(dist);
}

ShiftedLogNormalDist SimulationBuilderXml::GetShiftedLogNormalDist(const pugi::xml_node node)
{
    auto dist = GetNormalDist(node);
    auto shift = Text<double>(node.child("distribution").child("shift"));
    return ShiftedLogNormalDist::FromShiftedNormal(dist, shift);
}

void SimulationBuilderXml::Reset()
{
	parameters_.clear();
}

void SimulationBuilderXml::SetInputFile(const std::string &filename)
{
	document_.load_file(filename.c_str());
	simulation_.SetName(path(filename).stem().string());
}

void SimulationBuilderXml::CheckVersion()
{
	auto version_string = Attr<std::string>(document_.child("simulation"), "version");
	auto version = Version::from_string(version_string);

	if(Version::compare(version, Utility::get_model_version(), true) != 0)
	{
		throw std::runtime_error("bad input version");
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

    auto initial_infections_node = simulation_node.child("population").child("initialInfections");
    parameters.delayPrevalence = Text<int>(initial_infections_node.child("delay"));

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

    simulation_.SetLifeExpectancyConfidenceInterval(Text<double>(simulation_node.child("traceFiles").child("lifeExpectancy").child("medianConfidenceInterval")));
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
                    Entity::probDeathNatCauses[(std::size_t)DemographicProfile::Gender::Male] = probabilities[0];
                    Entity::probDeathNatCauses[(std::size_t)DemographicProfile::Gender::Female] = probabilities[1];
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
                    Entity::probDeathNatCauses[(std::size_t)DemographicProfile::Gender::Male] = probabilities[0];
                    Entity::probDeathNatCauses[(std::size_t)DemographicProfile::Gender::Female] = probabilities[1];
                }
            }
        }

		parameters.cepacTracer = new Tracer(parameters.simName, parameters.cepacSimContexts[0], 1);
		parameters.cepacRunStats = new RunStats(parameters.simName, parameters.cepacSimContexts[0]);
	}

    auto population_interventions_node = simulation_node.child("interventions").child("populationInterventions");
    for(auto intervention : ParseInterventions(population_interventions_node, false))
    {
        simulation_.RegisterIntervention(intervention);
    }

    for(auto group : ReadGroups())
    {
        simulation_.RegisterTargetGroup(group.second);
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
	double pHigh = population_parameters.GetMaleParameters().getProportionHighRisk(DemographicProfile::Employment::NonCsw);
	double marriageRateH = population_parameters.GetMaleParameters().getSexualBehavior(SexualPartnership::Type::Steady).getAcquisitionRatePerMonth(
		Entity::HIGH).getMean();
	double marriageRateL = population_parameters.GetMaleParameters().getSexualBehavior(SexualPartnership::Type::Steady).getAcquisitionRatePerMonth(
		Entity::LOW).getMean();
	double marriageDurationH = population_parameters.GetMaleParameters().getSexualBehavior(
		SexualPartnership::Type::Steady).getPartnershipDurationMth(Entity::HIGH).getMean();
	double marriageDurationL = population_parameters.GetMaleParameters().getSexualBehavior(
		SexualPartnership::Type::Steady).getPartnershipDurationMth(Entity::LOW).getMean();
	auto proportion_married = (1 - pHigh) * (marriageRateL * marriageDurationL) / (1 + marriageRateL *
		marriageDurationL) + pHigh * (marriageRateH * marriageDurationH) / (1 + marriageRateH * marriageDurationH);
	//Get the initial regular prevalence based on percent male high risk and rate and duration of regular relationships
	double regularRateH = population_parameters.GetMaleParameters().getSexualBehavior(SexualPartnership::Type::Regular).getAcquisitionRatePerMonth(
		Entity::HIGH).getMean();
	double regularRateL = population_parameters.GetMaleParameters().getSexualBehavior(SexualPartnership::Type::Regular).getAcquisitionRatePerMonth(
		Entity::LOW).getMean();
	double regularDurationH = population_parameters.GetMaleParameters().getSexualBehavior(
		SexualPartnership::Type::Regular).getPartnershipDurationMth(Entity::HIGH).getMean();
	double regularDurationL = population_parameters.GetMaleParameters().getSexualBehavior(
		SexualPartnership::Type::Regular).getPartnershipDurationMth(Entity::LOW).getMean();
	auto proportion_regular = (1 - pHigh) * (regularRateL * regularDurationL) + pHigh *
		(regularRateH * regularDurationH);

    std::map<SexualPartnership::Type, double> assort;

    for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
    {
        if(!population_parameters.GetMaleParameters().hasSexualBehavior(partnership_type)) continue;
        auto assortativeness = population_parameters.GetMaleParameters().getSexualBehavior(partnership_type).getAssortativeness();
        assort[partnership_type] = assortativeness;
    }

	//create EntityPool - this will contain all Entities
	auto entities = std::make_unique<EntityPool>(population_parameters.getAgeOfMajority(), population.GetId(), assort);
	population.entities.swap(entities);

	//initialize infection trace generator print detailed info about certain ProfileID's
	// in this case, all ProfileID's w/ non-nullptr BucketDemographicProfiles
	DemographicProfile::ProfileID currProfileID = DemographicProfile::MIN;

	while(currProfileID <= DemographicProfile::MAX)
	{
		//if it is being used in this Population, then append to _profileIDs
		if((population.entities->getBucket(currProfileID) != nullptr) &&
            (DemographicProfile::get(currProfileID, DemographicProfile::Demographic::SexualActivityStatus) != (std::size_t)DemographicProfile::SexualActivityStatus::NotActive))
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
	auto totalNumHeteroMales = Utility::round<std::size_t>(population_parameters.GetInitialSize() * population_parameters.getBirthProportion("hetero-male"));
    auto totalNumMsmws = Utility::round<std::size_t>(population_parameters.GetInitialSize() * population_parameters.getBirthProportion("msmw"));
    auto totalNumMsms = Utility::round<std::size_t>(population_parameters.GetInitialSize() * population_parameters.getBirthProportion("msm"));
    auto totalNumMales = totalNumMsmws + totalNumHeteroMales + totalNumMsms;
    auto totalNumFemales = std::max<std::size_t>(population_parameters.GetInitialSize() - totalNumMales, 0);
    
    std::size_t createdHeteroMales = 0, createdMsmws = 0, createdMsms = 0;

	//the params.xml file should have detailed the prevalent characteristics of each age bucket
	//  we will go through each age bucket and create the part of the prevalent population that falls within the bucket
	std::vector<AgeRange> ageRanges;

	for(auto &ageBucketParams : population_parameters.GetInitialAgeBuckets())
	{
        auto last = totalNumMsms + totalNumMsmws > 0 ? ageBucketParams.minAgeMth == population_parameters.GetInitialAgeBuckets().back().minAgeMth
            && ageBucketParams.maxAgeMth == population_parameters.GetInitialAgeBuckets().back().maxAgeMth : false;

        auto numHeteroMalesInCurrentBucket = last ? totalNumHeteroMales - createdHeteroMales : Utility::round<std::size_t>(totalNumHeteroMales * ageBucketParams.entityProportions["hetero-male"]);
        auto numMsmsInCurrentBucket = last ? totalNumMsms - createdMsms : Utility::round<std::size_t>(totalNumMsms * ageBucketParams.entityProportions["msm"]);
        auto numMsmwsInCurrentBucket = last ? totalNumMsmws - createdMsmws : Utility::round<std::size_t>(totalNumMsmws * ageBucketParams.entityProportions["msmw"]);
        auto numMalesInCurrentBucket = numHeteroMalesInCurrentBucket + numMsmsInCurrentBucket + numMsmwsInCurrentBucket;
        auto numFemalesInCurrentBucket = Utility::round<std::size_t>(totalNumFemales * ageBucketParams.entityProportions["female"]);

		//calc how many people are in the current age range
		auto currentBucketSize = numHeteroMalesInCurrentBucket + numMsmsInCurrentBucket + numMsmwsInCurrentBucket + numFemalesInCurrentBucket;

		//Number of persons of each gender to be traced in detailed output file
		auto numToTrace = static_cast<std::size_t>(population.GetNumberToTrace());

		for(std::size_t count = 0; count < currentBucketSize; count++)
		{
			//Determine whether or not person should be traced in SinglePersonTrace
			bool tracePerson = (count < numToTrace || (count >= numHeteroMalesInCurrentBucket && (count - numHeteroMalesInCurrentBucket) < numToTrace));

			//create a person, males first and females second
            std::string entity_type = count < numMalesInCurrentBucket ? "male" : "female";

            if(entity_type == "male")
            {
                if(count > numHeteroMalesInCurrentBucket)
                {
                    if(count > numHeteroMalesInCurrentBucket + numMsmsInCurrentBucket)
                    {
                        entity_type = "msmw";
                        createdMsmws++;
                    }
                    else
                    {
                        entity_type = "msm";
                        createdMsms++;
                    }
                }
                else
                {
                    createdHeteroMales++;
                }
            }

			auto person = population.GenerateEntity(simulation_.GetEventParams(), entity_type, &ageBucketParams, tracePerson);

			//add the created person to the EntityPool
			population.entities->addEntityToAll(person);
		}

		AgeRange ageRange = {ageBucketParams.minAgeMth, ageBucketParams.maxAgeMth};
		AgeRangeSizePair ageRangeSize = std::make_pair(ageRange, currentBucketSize);

		//Add a tuple to the currSizeByAgeRange vector along with the initial size of the age range
		ageRanges.push_back(ageRange);
		population.currSizeByAgeRange.push_back(ageRangeSize);
		population.currSizeByAgeRangeMale.push_back(std::make_pair(ageRange, numHeteroMalesInCurrentBucket));
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
    selector.set(DemographicProfile::Demographic::SexualActivityStatus, (std::size_t)DemographicProfile::SexualActivityStatus::Active);
    selector.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Male);
    selector.set(DemographicProfile::Demographic::SexualOrientation, (std::size_t)DemographicProfile::SexualOrientation::Heterosexual);
    selector.set(DemographicProfile::Demographic::RelationshipStatus, (std::size_t)DemographicProfile::RelationshipStatus::Single);
    selector.set(DemographicProfile::Demographic::Employment, (std::size_t)DemographicProfile::Employment::NonCsw);
	std::vector<DemographicProfile::ProfileID> bucketIDs;
	//TODO:fix code below, i've put placeholders for multiple singles buckets, but right now we only use 1 of each gender
	//errhode: Is this taken care of with the whole agebucket inside SexualMixingBucket thing?
	selector.selectProfileIDs(bucketIDs, nullptr);
	BucketDemographicProfile *singleMales = population.entities->getBucket(bucketIDs.at(0));
    selector.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Female);
	bucketIDs.clear();
	selector.selectProfileIDs(bucketIDs, nullptr);
	BucketDemographicProfile *singleFemales = population.entities->getBucket(bucketIDs.at(0));
	// create prevalent formSteadyPartnerships (time = 0)
	//the demographics that we are pulling the eligibles from -- same as for regular;
	//can just use the previous singleMales and singleFemales buckets
	//number of couples -- % married of adult population by DemographicProfile::SexualActivityStatus::ActiveStatus / 2
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
	}

	//number of regular couples -- % married of adult population by DemographicProfile::SexualActivityStatus::ActiveStatus / 2
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
	}

	//count the size of the population and store value
	population.UpdateSize();
}

std::unordered_map<TransmissionType, std::array<double, Entity::ENDHVLStrata>> SimulationBuilderXml::ReadTransmissionCoefficients()
{
    std::unordered_map<TransmissionType, std::array<double, Entity::ENDHVLStrata>> coefficient_map;

    auto read_coefficients = [](pugi::xml_node node) 
    {
        std::array<double, Entity::ENDHVLStrata> coefficients = {0};
        double value;

        for(auto hvl : enum_iterator<Entity::HVLStrata>())
        {
            switch(hvl)
            {
            case Entity::UNINFECTED:
                continue;
            case Entity::HVL_PRIMARY: 
                value = node.child("primary").text().as_double();
                break;
            case Entity::HVL_LATESTAGE: 
                value = node.child("lateStage").text().as_double();
                break;
            default: 
                value = node.child(("hvl" + std::to_string((int)hvl)).c_str()).text().as_double();
                break;
            }

            coefficients[hvl] = value;
        }

        return coefficients;
    };

    auto coeff_node = document_.select_single_node("/simulation/population/transmissionCoefficients").node();
    coefficient_map[TransmissionType::male_to_female] = read_coefficients(coeff_node.child("maleToFemale"));
    coefficient_map[TransmissionType::female_to_male] = read_coefficients(coeff_node.child("femaleToMale"));
    coefficient_map[TransmissionType::male_to_male] = read_coefficients(coeff_node.child("maleToMale"));

    return coefficient_map;
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

SexualBehavior SimulationBuilderXml::ReadSexualBehavior(const std::string &entity_type, SexualPartnership::Type type)
{
	auto path = "/simulation/population/entities/entity[@type='" + entity_type + "']/behavior/partnershipTypes/partnership[@type='" + to_string(type) + "']";
	auto node = document_.select_single_node(path.c_str()).node();

    if(node == nullptr)
    {
        throw std::runtime_error("behavior not defined for entity type " + entity_type + " and partnership type " + to_string(type));
    }

	SexualBehavior result(type);

    result.setAssortativeness(Text<double>(node.child("assortativeness")));

	auto bucket_path = "selectionCriteria/availableBuckets/bucket";
	for(const auto &bucket_settings : node.select_nodes(bucket_path))
	{
		SexualBehavior::AvailableBucket bucket;
		bucket.dmgProfileSelector.parse(Text<std::string>(bucket_settings.node().child("profile")));
		bucket.weight = Text<double>(bucket_settings.node().child("weight"));
		result.AddAvailableBucket(bucket);
	}

	result.setAverageYearsYounger(GetNormalDist(node.child("selectionCriteria").child("averageYearsYounger")));

	for(auto risk : {Entity::LOW, Entity::HIGH})
	{
        auto risk_node = node.child(risk == Entity::LOW ? "lowRisk" : "highRisk");

		result.setAcquisitionRatePerMonth(risk, GetLogNormalDist(risk_node.child("acquisitionRate")));
        result.setCoitalEventsPerMonth(risk, Text<double>(risk_node.child("coitalEventsPerMonth").child("distribution").child("mean")));
        result.setChanceCondomUsePerEvent(risk, GetBetaDist(risk_node.child("chanceCondomUsePerEvent")));
        result.setPartnershipDuration(risk, GetShiftedLogNormalDist(risk_node.child("partnershipDurationMth")));
	}

	return result;
}

Male::SubPopParams SimulationBuilderXml::ReadMaleSubPopParams()
{
	auto node = document_.select_single_node("/simulation/population/entities/entity[@type='hetero-male']").node();

	Male::SubPopParams result;

	auto behavior_node = node.child("behavior");
    result.SetCswEndAge(Utility::convert_time(TimeGranularity::Year, TimeGranularity::Month, Text<double>(behavior_node.child("cswEndAge"))));
    result.SetChanceBecomeCsw(Text<double>(behavior_node.child("chanceBecomeSexWorker")));
	result.SetPartnerAcqMultWithSteady(Entity::HIGH, Text<double>(behavior_node.child("partnerAcqMultWithSteadyHighRisk")));
	result.SetPartnerAcqMultWithSteady(Entity::LOW, Text<double>(behavior_node.child("partnerAcqMultWithSteadyLowRisk")));

	bool use_high_risk_multiplier = Attr<bool>(behavior_node.child("highRiskAcqRateMultiplier"), "enabled");
	double high_risk_multiplier = Text<double>(behavior_node.child("highRiskAcqRateMultiplier"));
	bool use_csw_high_risk_multiplier = Attr<bool>(behavior_node.child("highRiskCswAcqRateMultiplier"), "enabled");
	double csw_high_risk_multiplier = Text<double>(behavior_node.child("highRiskCswAcqRateMultiplier"));

	result.SetCoefficientVariation(false, 0);

	for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
	{
        try
        {
            auto params = ReadSexualBehavior("hetero-male", partnership_type);

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

	NormalDist activityLevel;
	activityLevel.mean = 1;
	activityLevel.stddev = 0;
	result.SetActivityLevel(activityLevel);

	result.SetProportionHighRisk(DemographicProfile::Employment::Csw, Text<double>(behavior_node.child("proportionHighRiskCsw")));
    result.SetProportionHighRisk(DemographicProfile::Employment::NonCsw, Text<double>(behavior_node.child("proportionHighRiskNonCsw")));

	auto discountingStartAgeYrs = Text<int>(behavior_node.child("ageDiscounting").child("startAgeYrs"));
	auto acquisitionDiscByYr = Text<double>(behavior_node.child("ageDiscounting").child("acquisitionDiscByYr"));
	auto coitalActsDiscByYr = Text<double>(behavior_node.child("ageDiscounting").child("coitalActsDiscByYr"));
	result.setAgeDiscounting(discountingStartAgeYrs, acquisitionDiscByYr, coitalActsDiscByYr);

	auto health_node = node.child("health");
	result.SetCircucmsionProtectEfficacy(Text<double>(health_node.child("circumcisionProtectEfficacy")));
	result.SetCondomProtectEff(Text<double>(health_node.child("condomProtectEfficacy")));

    result.SetMaxPartnershipRejections(Text<int>(behavior_node.child("maxPartnershipRejections")));

	return result;
}

Msm::SubPopParams SimulationBuilderXml::ReadMsmSubPopParams()
{
    auto node = document_.select_single_node("/simulation/population/entities/entity[@type='msm']").node();
    
    Msm::SubPopParams result;

    if(node == nullptr)
    {
        std::cout << "MSM entity not found in XML, skipping..." << std::endl;
        return result;
    }

    auto behavior_node = node.child("behavior");
    result.SetCswEndAge(Utility::convert_time(TimeGranularity::Year, TimeGranularity::Month, Text<double>(behavior_node.child("cswEndAge"))));
    result.SetChanceBecomeCsw(Text<double>(behavior_node.child("chanceBecomeSexWorker")));
    result.SetPartnerAcqMultWithSteady(Entity::HIGH, Text<double>(behavior_node.child("partnerAcqMultWithSteadyHighRisk")));
    result.SetPartnerAcqMultWithSteady(Entity::LOW, Text<double>(behavior_node.child("partnerAcqMultWithSteadyLowRisk")));

    bool use_high_risk_multiplier = Attr<bool>(behavior_node.child("highRiskAcqRateMultiplier"), "enabled");
    double high_risk_multiplier = Text<double>(behavior_node.child("highRiskAcqRateMultiplier"));
    bool use_csw_high_risk_multiplier = Attr<bool>(behavior_node.child("highRiskCswAcqRateMultiplier"), "enabled");
    double csw_high_risk_multiplier = Text<double>(behavior_node.child("highRiskCswAcqRateMultiplier"));

    result.SetCoefficientVariation(false, 0);

    for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
    {
        try
        {
            auto params = ReadSexualBehavior("msm", partnership_type);

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

    NormalDist activityLevel;
    activityLevel.mean = 1;
    activityLevel.stddev = 0;
    result.SetActivityLevel(activityLevel);

    result.SetProportionHighRisk(DemographicProfile::Employment::Csw, Text<double>(behavior_node.child("proportionHighRiskCsw")));
    result.SetProportionHighRisk(DemographicProfile::Employment::NonCsw, Text<double>(behavior_node.child("proportionHighRiskNonCsw")));

    auto discountingStartAgeYrs = Text<int>(behavior_node.child("ageDiscounting").child("startAgeYrs"));
    auto acquisitionDiscByYr = Text<double>(behavior_node.child("ageDiscounting").child("acquisitionDiscByYr"));
    auto coitalActsDiscByYr = Text<double>(behavior_node.child("ageDiscounting").child("coitalActsDiscByYr"));
    result.setAgeDiscounting(discountingStartAgeYrs, acquisitionDiscByYr, coitalActsDiscByYr);

    auto health_node = node.child("health");
    result.SetCircucmsionProtectEfficacy(Text<double>(health_node.child("circumcisionProtectEfficacy")));
    result.SetCondomProtectEff(Text<double>(health_node.child("condomProtectEfficacy")));

    result.SetMaxPartnershipRejections(Text<int>(behavior_node.child("maxPartnershipRejections")));

    return result;
}

Msmw::SubPopParams SimulationBuilderXml::ReadBiMaleSubPopParams()
{
    auto node = document_.select_single_node("/simulation/population/entities/entity[@type='Msmw']").node();

    Msmw::SubPopParams result;

    if(node == nullptr)
    {
        std::cout << "Bisexual male entity not found in XML, skipping..." << std::endl;
        return result;
    }

    auto behavior_node = node.child("behavior");
    result.SetCswEndAge(Utility::convert_time(TimeGranularity::Year, TimeGranularity::Month, Text<double>(behavior_node.child("cswEndAge"))));
    result.SetChanceBecomeCsw(Text<double>(behavior_node.child("chanceBecomeSexWorker")));
    result.SetPartnerAcqMultWithSteady(Entity::HIGH, Text<double>(behavior_node.child("partnerAcqMultWithSteadyHighRisk")));
    result.SetPartnerAcqMultWithSteady(Entity::LOW, Text<double>(behavior_node.child("partnerAcqMultWithSteadyLowRisk")));

    bool use_high_risk_multiplier = Attr<bool>(behavior_node.child("highRiskAcqRateMultiplier"), "enabled");
    double high_risk_multiplier = Text<double>(behavior_node.child("highRiskAcqRateMultiplier"));
    bool use_csw_high_risk_multiplier = Attr<bool>(behavior_node.child("highRiskCswAcqRateMultiplier"), "enabled");
    double csw_high_risk_multiplier = Text<double>(behavior_node.child("highRiskCswAcqRateMultiplier"));

    result.SetCoefficientVariation(false, 0);

    for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
    {
        try
        {
            auto params = ReadSexualBehavior("msmw", partnership_type);

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

    NormalDist activityLevel;
    activityLevel.mean = 1;
    activityLevel.stddev = 0;
    result.SetActivityLevel(activityLevel);

    result.SetProportionHighRisk(DemographicProfile::Employment::Csw, Text<double>(behavior_node.child("proportionHighRiskCsw")));
    result.SetProportionHighRisk(DemographicProfile::Employment::NonCsw, Text<double>(behavior_node.child("proportionHighRiskNonCsw")));

    auto discountingStartAgeYrs = Text<int>(behavior_node.child("ageDiscounting").child("startAgeYrs"));
    auto acquisitionDiscByYr = Text<double>(behavior_node.child("ageDiscounting").child("acquisitionDiscByYr"));
    auto coitalActsDiscByYr = Text<double>(behavior_node.child("ageDiscounting").child("coitalActsDiscByYr"));
    result.setAgeDiscounting(discountingStartAgeYrs, acquisitionDiscByYr, coitalActsDiscByYr);

    auto health_node = node.child("health");
    result.SetCircucmsionProtectEfficacy(Text<double>(health_node.child("circumcisionProtectEfficacy")));
    result.SetCondomProtectEff(Text<double>(health_node.child("condomProtectEfficacy")));

    result.SetMaxPartnershipRejections(Text<int>(behavior_node.child("maxPartnershipRejections")));

    return result;
}

Female::SubPopParams SimulationBuilderXml::ReadFemaleSubPopParams()
{
	auto node = document_.select_single_node("/simulation/population/entities/entity[@type='female']").node();

	Female::SubPopParams result;

	auto behavior_node = node.child("behavior");
    result.SetCswEndAge(Utility::convert_time(TimeGranularity::Year, TimeGranularity::Month, Text<double>(behavior_node.child("cswEndAge"))));
	result.SetChanceBecomeCsw(Text<double>(behavior_node.child("chanceBecomeSexWorker")));
	result.SetProportionHighRisk(DemographicProfile::Employment::NonCsw, Text<double>(behavior_node.child("proportionHighRiskNonCsw")));
	result.SetProportionHighRisk(DemographicProfile::Employment::Csw, Text<double>(behavior_node.child("proportionHighRiskCsw")));

	NormalDist activityLevel;
	activityLevel.mean = 1;
	activityLevel.stddev = 0;
	result.SetActivityLevel(activityLevel);

	return result;
}

SimulationBuilderXml::EntityDistributions SimulationBuilderXml::ReadEntityDistributions(pugi::xml_node node)
{
    EntityDistributions distributions;

    for(auto distribution_node : node.children("distribution"))
    {
        std::string entity_type = distribution_node.attribute("type").as_string();
        distributions[entity_type] = distribution_node.text().as_double();
    }

    return distributions;
}

void SimulationBuilderXml::ReadPopulationParameters()
{
	auto population_node = document_.child("simulation").child("population");

    auto initial_infections_node = population_node.child("initialInfections");

	auto initial_state_node = population_node.child("initialState");
	population_parameters.SetInitialSize(Text<int>(initial_state_node.child("size")));

	//get initial age distribution
	for(auto age_bucket_node : initial_state_node.child("entityDistributions").children("ageRange"))
	{
        auto dist = ReadEntityDistributions(age_bucket_node);

        auto bucket_age_range = std::string(age_bucket_node.attribute("lower").as_string()) + "-" + age_bucket_node.attribute("upper").as_string();

        std::size_t numInfectedCSWMale = 0, numInfectedCSWFemale = 0,
            numInfectedNonCSWMalesLowRisk = 0, numInfectedNonCSWFemalesLowRisk = 0,
            numInfectedNonCSWMalesHighRisk = 0, numInfectedNonCSWFemalesHighRisk = 0;

        for(auto profile_node : initial_infections_node.children("profile"))
        {
            DemographicProfile profile;
            profile.parse(profile_node.attribute("bucket").as_string());
            std::string risk_string = profile_node.attribute("risk").as_string();
            std::string age_range = profile_node.attribute("age-range").as_string();

            if(age_range == bucket_age_range)
            {
                auto gender = (DemographicProfile::Gender)profile.get(DemographicProfile::Demographic::Gender);
                auto risk = risk_string == "high" ? Entity::RiskLevel::HIGH : Entity::RiskLevel::LOW;
                auto csw = (DemographicProfile::Employment)profile.get(DemographicProfile::Demographic::Employment);
                auto number = profile_node.text().as_int();

                if(risk == Entity::RiskLevel::HIGH && gender == DemographicProfile::Gender::Male && csw == DemographicProfile::Employment::Csw)
                {
                    numInfectedCSWMale += number;
                }
                else if(risk == Entity::RiskLevel::HIGH && gender == DemographicProfile::Gender::Male && csw == DemographicProfile::Employment::NonCsw)
                {
                    numInfectedNonCSWMalesHighRisk += number;
                }
                else if(risk == Entity::RiskLevel::LOW && gender == DemographicProfile::Gender::Male && csw == DemographicProfile::Employment::NonCsw)
                {
                    numInfectedNonCSWMalesLowRisk += number;
                }
                else if(risk == Entity::RiskLevel::HIGH && gender == DemographicProfile::Gender::Female && csw == DemographicProfile::Employment::Csw)
                {
                    numInfectedCSWFemale += number;
                }
                else if(risk == Entity::RiskLevel::HIGH && gender == DemographicProfile::Gender::Female && csw == DemographicProfile::Employment::NonCsw)
                {
                    numInfectedNonCSWFemalesHighRisk += number;
                }
                else if(risk == Entity::RiskLevel::LOW && gender == DemographicProfile::Gender::Female && csw == DemographicProfile::Employment::NonCsw)
                {
                    numInfectedNonCSWFemalesLowRisk += number;
                }
            }
        }

        population_parameters.GetInitialAgeBuckets().emplace_back(
            Utility::convert_time(TimeGranularity::Year, TimeGranularity::Month, Attr<int>(age_bucket_node, "lower")),
            Utility::convert_time(TimeGranularity::Year, TimeGranularity::Month, Attr<int>(age_bucket_node, "upper")) + 11,
            dist,
            numInfectedCSWMale,  numInfectedCSWFemale,
            numInfectedNonCSWMalesLowRisk, numInfectedNonCSWFemalesLowRisk,
            numInfectedNonCSWMalesHighRisk, numInfectedNonCSWFemalesHighRisk);
	}

	//normalize %population values for each age bucket
    double totalFemaleProportion = 0;
    double totalHeteroMaleProportion = 0;
    double totalMsmwProportion = 0;
    double totalMsmProportion = 0;

    population_parameters.SetTransmissionCoefficients(ReadTransmissionCoefficients());

	//get the total of proportionage values of AgeBucketPrevalencInfo.proportionOfPopulation
		for(auto &age_bucket : population_parameters.GetInitialAgeBuckets())
		{
			totalFemaleProportion += age_bucket.entityProportions["female"];
            totalHeteroMaleProportion += age_bucket.entityProportions["hetero-male"];
            totalMsmwProportion += age_bucket.entityProportions["msmw"];
            totalMsmProportion += age_bucket.entityProportions["msm"];
		}

		//normalize each proportionage value so that the sum of them == 1
		for(auto &age_bucket : population_parameters.GetInitialAgeBuckets())
		{
			if(totalFemaleProportion != 0) age_bucket.entityProportions["female"] /= totalFemaleProportion;
            if(totalHeteroMaleProportion != 0) age_bucket.entityProportions["hetero-male"] /= totalHeteroMaleProportion;
            if(totalMsmwProportion != 0) age_bucket.entityProportions["msmw"] /= totalMsmwProportion;
            if(totalMsmProportion != 0) age_bucket.entityProportions["msm"] /= totalMsmProportion;
		}

    auto births_node = population_node.child("births");
	population_parameters.setBirthRate(Text<double>(births_node.child("rate")));

    for(const auto &dist : ReadEntityDistributions(births_node.child("entityDistributions")))
    {
        population_parameters.setBirthProportion(dist.first, dist.second);
    }

	population_parameters.setProportionCircumcised(Text<double>(population_node.child("proportionMaleCircumcised")));
	population_parameters.setAgeOfMajority(Text<int>(population_node.child("ageOfMajority")), TimeGranularity::Year);

	auto defaultMaleParams = ReadMaleSubPopParams();
	population_parameters.SetMaleParameters(defaultMaleParams);
    auto defaultMsmParams = ReadMsmSubPopParams();
    population_parameters.SetMsmParameters(defaultMsmParams);
    auto defaultBiMaleParams = ReadBiMaleSubPopParams();
    population_parameters.SetBiMaleParameters(defaultBiMaleParams);
	auto defaultFemaleParams = ReadFemaleSubPopParams();
	population_parameters.SetFemaleParameters(defaultFemaleParams);

    population_parameters.SetInitialCswProportion("male", defaultMaleParams.getChanceBecomeCSW());
    population_parameters.SetInitialCswProportion("female", defaultFemaleParams.GetChanceBecomeCSW());
    population_parameters.SetCswEndAge("male", defaultMaleParams.GetCswEndAge());
    population_parameters.SetCswEndAge("female", defaultFemaleParams.GetCswEndAge());

	//save flags to indicate whether particular partnership types have duration or not
	for(auto type : enum_iterator<SexualPartnership::Type>())
	{
        if(!defaultMaleParams.hasSexualBehavior(type)) continue;
		auto has_duration = !(defaultMaleParams.getSexualBehavior(SexualPartnership::Type(type)).getPartnershipDurationMth(Entity::LOW).isZeroDistrib)
			&& !(defaultMaleParams.getSexualBehavior(SexualPartnership::Type(type)).getPartnershipDurationMth(Entity::HIGH).isZeroDistrib);
		population_parameters.SetPartnershipHasDuration(DemographicProfile::Gender::Male, type, has_duration);
		population_parameters.SetPartnershipHasDuration(DemographicProfile::Gender::Female, type, false);
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
                target.value.observed_hiv_status.value = Entity::NEGATIVE;
            }
            else if(value == "observed-acute")
            {
                target.value.observed_hiv_status.value = Entity::OBSERVED_ACUTE;
            }
            else if(value == "unobserved-acute")
            {
                target.value.observed_hiv_status.value = Entity::UNOBSERVED_ACUTE;
            }
            else if(value == "observed-chronic")
            {
                target.value.observed_hiv_status.value = Entity::OBSERVED_CHRONIC;
            }
            else if(value == "unobserved-chronic")
            {
                target.value.observed_hiv_status.value = Entity::UNOBSERVED_CHRONIC;
            }
            else if(value == "observed-latestage")
            {
                target.value.observed_hiv_status.value = Entity::OBSERVED_LATESTAGE;
            }
            else if(value == "unobserved-latestage")
            {
                target.value.observed_hiv_status.value = Entity::UNOBSERVED_LATESTAGE;
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
                target.value.risk_level.value = Entity::HIGH;
            }
            else if(value == "low")
            {
                target.value.risk_level.value = Entity::LOW;
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

            if(value == "hetero")
            {
                target.value.sexual_orientation.value = DemographicProfile::SexualOrientation::Heterosexual;
            }
            else if(value == "homo")
            {
                target.value.sexual_orientation.value = DemographicProfile::SexualOrientation::Homosexual;
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

std::vector<Intervention> SimulationBuilderXml::ParseInterventions(pugi::xml_node interventions_node, bool individual)
{
    std::vector<Intervention> interventions;

    for(auto intervention_node : interventions_node.children())
    {
        interventions.push_back(ReadIntervention(intervention_node, individual));
    }

    return interventions;
}

std::unordered_map<std::string, TargetGroup> SimulationBuilderXml::ReadGroups()
{
    pugi::xml_node groups_node = 
        document_.select_single_node("/simulation/interventions/groups").node();
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

        TargetGroup group(label, enrollment_period.first, enrollment_period.second, 
            open, permanent, target);

        for(auto partition_node : group_node.child("partitions").children("partition"))
        {
            std::string label = Attr<std::string>(partition_node, "label");
            auto proportion = Text<double>(partition_node.child("proportion"));
            auto trace = Text<bool>(partition_node.child("trace"));
            auto interventions = ParseInterventions(partition_node.child("interventions"), true);

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
    OverrideChanceCondomUse
};

const std::map<KnownIntervention, std::string> KnownInterventionStrings =
{
    {KnownIntervention::Circumcise, "circumcise"},
    {KnownIntervention::BirthRate, "birthRate"},
    {KnownIntervention::ProportionMale, "proportionMale"},
    {KnownIntervention::ProportionCircumcised, "proportionCircumcised"},
    {KnownIntervention::ChanceBecomeSexWorker, "chanceBecomeSexWorker"},
    {KnownIntervention::DelaySexualActivity, "delaySexualActivity"},
    {KnownIntervention::TransmissionCoefficient, "transmissionCoefficient"},
    {KnownIntervention::ProportionHighRisk, "proportionHighRisk"},
    {KnownIntervention::AverageYearsYounger, "averageYearsYounger"},
    {KnownIntervention::PartnerAcquisitionRate, "partnerAcquisitionRate"},
    {KnownIntervention::CoitalEventsPerMonth, "coitalEventsPerMonth"},
    {KnownIntervention::ChanceCondomUse, "chanceCondomUse"},
    {KnownIntervention::PartnershipDuration, "partnershipDuration"},
    {KnownIntervention::RolloutEligibility, "rolloutEligibility"},
    {KnownIntervention::PartnershipRejectionChance, "partnershipRejectionChance"},
    {KnownIntervention::OverrideChanceCondomUse, "overrideChanceCondomUse"}
};

template<>
KnownIntervention SimulationBuilderXml::from_string(const std::string &intervention)
{
    for(auto pair : KnownInterventionStrings)
    {
        if(pair.second == intervention)
        {
            return pair.first;
        }
    }

    throw std::runtime_error("unknown intervention: " + intervention);
}

template<>
Entity::RiskLevel SimulationBuilderXml::from_string(const std::string &risk)
{
    if(risk == "high") return Entity::RiskLevel::HIGH;
    if(risk == "low") return Entity::RiskLevel::LOW;

    throw std::runtime_error("unknown risk level: " + risk);
}

template<>
DemographicProfile::Gender SimulationBuilderXml::from_string(const std::string &gender)
{
    if(gender == "male") return DemographicProfile::Gender::Male;
    if(gender == "female") return DemographicProfile::Gender::Female;

    throw std::runtime_error("unknown gender: " + gender);
}

template<>
DemographicProfile::Employment SimulationBuilderXml::from_string(const std::string &employment)
{
    if(employment == "csw") return DemographicProfile::Employment::Csw;
    if(employment == "non-csw") return DemographicProfile::Employment::NonCsw;

    throw std::runtime_error("unknown employment: " + employment);
}

template<>
Entity::HVLStrata SimulationBuilderXml::from_string(const std::string &hvl_string)
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
SexualPartnership::Type SimulationBuilderXml::from_string(const std::string &type_string)
{
    if(type_string == "steady") return SexualPartnership::Type::Steady;
    if(type_string == "regular") return SexualPartnership::Type::Regular;
    if(type_string == "casual") return SexualPartnership::Type::Casual;
    if(type_string == "csw") return SexualPartnership::Type::Csw;

    throw std::runtime_error("unknown partnership type: " + type_string);
}

Intervention SimulationBuilderXml::ReadIntervention(pugi::xml_node &node, bool individual)
{
    int time = node.attribute("time") != nullptr ? Attr<int>(node, "time") : -1;
    int duration = node.attribute("duration") != nullptr ? Attr<int>(node, "duration") : -1;

    Intervention intervention(time, duration);

    auto intervention_type = from_string<KnownIntervention>(node.name());

    if(individual)
    {
        switch(intervention_type)
        {
        case KnownIntervention::Circumcise:
        {
            intervention.SetPopulationIndividualCallback(
                [=](Population &population, Entity *person) { 
                    population.Circumcise(person); });
            break;
        }
        case KnownIntervention::ChanceBecomeSexWorker:
        {
            auto chance = Text<double>(node);
            intervention.SetIndividualCallback(
                [=](Entity *person) { 
                    person->SetChanceBecomeSexWorker(chance); });
            break;
        }
        case KnownIntervention::DelaySexualActivity:
        {
            auto months = Text<int>(node);
            intervention.SetIndividualCallback(
                [=](Entity *person) { 
                    person->SetSexualActivityDelay(months); });
            break;
        }
        case KnownIntervention::TransmissionCoefficient:
        {
            /*
            auto hvl_stratum = Attr<Entity::HVLStrata>(node, "hvl");
            auto coefficient = Text<double>(node);
            intervention.SetIndividualCallback(
                [=](Entity *person) { 
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
                [=](Entity *person) { 
                    person->SetAverageYearsYounger(partnership_type, dist); });
            break;
        }
        case KnownIntervention::PartnerAcquisitionRate:
        {
            auto risk = Attr<Entity::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = GetLogNormalDist(node);
            intervention.SetIndividualCallback(
                [=](Entity *person) { 
                    person->SetAcquisitionRatePerMonth(risk, partnership_type, dist, simulation_.GetEventParams().randomNums); });
            break;
        }
        case KnownIntervention::CoitalEventsPerMonth:
        {
            auto risk = Attr<Entity::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = Text<double>(node.child("distribution").child("mean"));
            intervention.SetIndividualCallback(
                [=](Entity *person) { 
                    person->SetCoitalEventsPerMonth(risk, partnership_type, dist); });
            break;
        }
        case KnownIntervention::ChanceCondomUse:
        {
            auto risk = Attr<Entity::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = GetBetaDist(node);
            intervention.SetIndividualCallback(
                [=](Entity *person) {
                    person->SetChanceCondomUsePerEvent(risk, partnership_type, dist, simulation_.GetEventParams().randomNums); });
            break;
        }
        case KnownIntervention::PartnershipDuration:
        {
            auto risk = Attr<Entity::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = GetShiftedLogNormalDist(node);
            intervention.SetIndividualCallback(
                [=](Entity *person) { 
                    person->SetPartnershipDuration(risk, partnership_type, dist); });
            break;
        }
        case KnownIntervention::PartnershipRejectionChance:
        {
            auto risk = Attr<Entity::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto chance = Text<double>(node);
            intervention.SetIndividualCallback(
                [=](Entity *person) {
                person->SetPartnershipRejectionChance(risk, partnership_type, chance); });
            break;
        }
        case KnownIntervention::OverrideChanceCondomUse:
        {
            auto chance = Text<double>(node);
            intervention.SetIndividualCallback(
                [=](Entity *person) {
                person->SetOverrideChanceCondomUse(chance); });
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
                [=](Population &p) { 
                    p.popWideParams.setBirthRate(value); });
            break;
        }
        case KnownIntervention::ProportionMale:
        {
            auto value = Text<double>(node);
            intervention.SetPopulationCallback(
                [=](Population &p) { 
                    p.popWideParams.setBirthProportion("male", value); });
            break;
        }
        case KnownIntervention::ProportionCircumcised:
        {
            auto value = Text<double>(node);
            intervention.SetPopulationCallback(
                [=](Population &p) {
                    p.popWideParams.setProportionCircumcised(value); });
            break;
        }
        case KnownIntervention::ChanceBecomeSexWorker:
        {
            auto gender = Attr<DemographicProfile::Gender>(node, "gender");
            auto chance = Text<double>(node);
            intervention.SetPopulationCallback(
                [=](Population &p) { p.popWideParams.SetChanceBecomeCsw(gender, chance); });
            intervention.SetIndividualCallback([=](Entity *person)
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
                [=](Population &p) { p.popWideParams.SetSexualActivityDelay(months); });
            intervention.SetIndividualCallback(
                [=](Entity *person) { person->SetSexualActivityDelay(months); });
            break;
        }
        case KnownIntervention::TransmissionCoefficient:
        {
            /*
            auto gender = Attr<DemographicProfile::Gender>(node, "gender");
            auto hvl_stratum = Attr<Entity::HVLStrata>(node, "hvl");
            auto coefficient = Text<double>(node);
            intervention.SetPopulationCallback(
                [=](Population &p) { p.popWideParams.SetTransmissionCoefficient(gender, hvl_stratum, coefficient); });
            intervention.SetIndividualCallback([=](Entity *person)
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
                [=](Population &p) { p.popWideParams.SetProportionHighRisk(gender, employment, proportion); });
            intervention.SetIndividualCallback([=](Entity *person)
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
                [=](Population &p) { p.popWideParams.SetAverageYearsYounger(partnership_type, dist); });
            intervention.SetIndividualCallback([=](Entity *person) 
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
                [=](Population &p) { p.popWideParams.SetAcquisitionRatePerMonth(risk, partnership_type, dist); });
            intervention.SetIndividualCallback([=](Entity *person)
            {
                if((DemographicProfile::Gender)person->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == DemographicProfile::Gender::Male)
                {
                    person->SetAcquisitionRatePerMonth(risk, partnership_type, dist, simulation_.GetEventParams().randomNums);
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
                [=](Population &p) { p.popWideParams.SetCoitalEventsPerMonth(risk, partnership_type, dist); });
            intervention.SetIndividualCallback([=](Entity *person)
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
            auto risk = Attr<Entity::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = GetBetaDist(node);
            intervention.SetPopulationCallback(
                [=](Population &p) { p.popWideParams.SetChanceCondomUsePerEvent(risk, partnership_type, dist); });
            intervention.SetIndividualCallback([=](Entity *person)
            {
                if((DemographicProfile::Gender)person->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == DemographicProfile::Gender::Male)
                {
                    person->SetChanceCondomUsePerEvent(risk, partnership_type, dist, simulation_.GetEventParams().randomNums);
                }
            });
            break;
        }
        case KnownIntervention::PartnershipDuration:
        {
            auto risk = Attr<Entity::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = GetShiftedLogNormalDist(node);
            intervention.SetPopulationCallback(
                [=](Population &p) { p.popWideParams.SetPartnershipDuration(risk, partnership_type, dist); });
            intervention.SetIndividualCallback([=](Entity *person) 
            { 
                if((DemographicProfile::Gender)person->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == DemographicProfile::Gender::Male)
                {
                    person->SetPartnershipDuration(risk, partnership_type, dist);
                }
            });
            break;
        }
        case KnownIntervention::RolloutEligibility:
        {
            int new_value = Text<int>(node);
            std::string criterion = Attr<std::string>(node, "criterion");
            std::string parameter_name = Attr<std::string>(node, "parameter");

            if(criterion == "OIHist")
            {
                if(parameter_name == "rank")
                {
                    intervention.SetSimulationCallback(
                        [=](Simulation &s) { s.parameters_.rolloutEligibility.oiHistRank = new_value; });
                }
                else if(parameter_name.substr(0, 2) == "OI")
                {
                    int oi_number = std::stoi(parameter_name.substr(2));
                    intervention.SetSimulationCallback(
                        [=](Simulation &s) { s.parameters_.rolloutEligibility.oiHistOIs[oi_number] = new_value != 0; });
                }
                else if(parameter_name == "numOIToStart")
                {
                    intervention.SetSimulationCallback(
                        [=](Simulation &s) { s.parameters_.rolloutEligibility.oiHistNumToStart = new_value; });
                }
                else
                {
                    throw std::runtime_error("invalid parameter: " + parameter_name);
                }
            }
            else if(criterion == "CD4")
            {
                if(parameter_name == "rank")
                {
                    intervention.SetSimulationCallback(
                        [=](Simulation &s) { s.parameters_.rolloutEligibility.cd4Rank = new_value; });
                }
                else if(parameter_name == "CD4Lwr")
                {
                    intervention.SetSimulationCallback(
                        [=](Simulation &s) { s.parameters_.rolloutEligibility.cd4Bounds.lower = new_value; });
                }
                else if(parameter_name == "CD4Upp")
                {
                    intervention.SetSimulationCallback(
                        [=](Simulation &s) { s.parameters_.rolloutEligibility.cd4Bounds.upper = new_value; });
                }
                else
                {
                    throw std::runtime_error("invalid parameter: " + parameter_name);
                }
            }
            else if(criterion == "CD4OIHist")
            {
                if(parameter_name == "rank")
                {
                    intervention.SetSimulationCallback(
                        [=](Simulation &s) { s.parameters_.rolloutEligibility.cd4OiHistRank = new_value; });
                }
                else if(parameter_name == "CD4Lwr")
                {
                    intervention.SetSimulationCallback(
                        [=](Simulation &s) { s.parameters_.rolloutEligibility.cd4OiHistCd4Bounds.lower = new_value; });
                }
                else if(parameter_name == "CD4Upp")
                {
                    intervention.SetSimulationCallback(
                        [=](Simulation &s) { s.parameters_.rolloutEligibility.cd4OiHistCd4Bounds.upper = new_value; });
                }
                else if(parameter_name.substr(0, 2) == "OI")
                {
                    int oi_number = std::stoi(parameter_name.substr(2));
                    intervention.SetSimulationCallback(
                        [=](Simulation &s) { s.parameters_.rolloutEligibility.cd4OiHistOIs[oi_number] = new_value != 0; });
                }
                else
                {
                    throw std::runtime_error("invalid parameter: " + parameter_name);
                }
            }
            else if(criterion == "HVL")
            {
                if(parameter_name == "rank")
                {
                    intervention.SetSimulationCallback(
                        [=](Simulation &s) { s.parameters_.rolloutEligibility.hvlRank = new_value; });
                }
                else if(parameter_name == "HVLLwr")
                {
                    intervention.SetSimulationCallback(
                        [=](Simulation &s) { s.parameters_.rolloutEligibility.hvlBounds.lower = new_value; });
                }
                else if(parameter_name == "HVLUpp")
                {
                    intervention.SetSimulationCallback(
                        [=](Simulation &s) { s.parameters_.rolloutEligibility.hvlBounds.upper = new_value; });
                }
                else
                {
                    throw std::runtime_error("invalid parameter: " + parameter_name);
                }
            }
            else if(criterion == "CD4HVL")
            {
                if(parameter_name == "rank")
                {
                    intervention.SetSimulationCallback(
                        [=](Simulation &s) { s.parameters_.rolloutEligibility.cd4HvlRank = new_value; });
                }
                else if(parameter_name == "CD4Lwr")
                {
                    intervention.SetSimulationCallback(
                        [=](Simulation &s) { s.parameters_.rolloutEligibility.cd4HvlCd4Bounds.lower = new_value; });
                }
                else if(parameter_name == "CD4Upp")
                {
                    intervention.SetSimulationCallback(
                        [=](Simulation &s) { s.parameters_.rolloutEligibility.cd4HvlCd4Bounds.upper = new_value; });
                }
                else if(parameter_name == "HVLLwr")
                {
                    intervention.SetSimulationCallback(
                        [=](Simulation &s) { s.parameters_.rolloutEligibility.cd4HvlHvlBounds.lower = new_value; });
                }
                else if(parameter_name == "HVLUpp")
                {
                    intervention.SetSimulationCallback(
                        [=](Simulation &s) { 
                            s.parameters_.rolloutEligibility.cd4HvlHvlBounds.upper = new_value; });
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
