#include "SimulationBuilderXml.h"
#include "utility/enum_iterator.h"
#include "utility/filesystem.h"
#include "utility/make_unique.h"
#include "CostStats.h"

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
	simulation_.SetName(transm::path(filename).stem().string());
}

void SimulationBuilderXml::CheckVersion()
{
	auto version_string = Attr<std::string>(document_.child("simulation"), "version");
	auto version = Version::FromString(version_string);

	if(Version::Compare(version, Utility::get_model_version(), true) != 0)
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

	//TODO-GA: This needs slows things down considerably, and is often left on by mistake. Disabled for non-debug builds. GA
	//parameters.debugLevel = static_cast<DebugLevel>(Text<int>(simulation_node.child("debugLevel")));
#ifndef NDEBUG
	parameters.debugLevel = static_cast<DebugLevel>(Text<int>(simulation_node.child("debugLevel")));
#else
	parameters.debugLevel = static_cast<DebugLevel>(0);
#endif
	parameters.monthOf1990 = Text<int>(simulation_node.child("monthOf1990"));

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
		//TODO-GA: This is NOT a normal output. Disabled for non-debug builds. GA
#ifndef NDEBUG
        {EventParams::TraceFile::Type::SinglePerson, "singlePerson"},
#endif
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
	for(auto time_node : simulation_node.child("traceFiles").child("partnerAcquisition").children("time"))
	{
		simulation_.AddPartnerAcquisitionRecordTime(Text<int>(time_node));
	}

	auto calibration_node = simulation_node.child("calibration");
	parameters.calibrationInputs.useCalibration = Attr<bool>(calibration_node, "enabled");
	if(parameters.calibrationInputs.useCalibration)
	{
		auto &calib = parameters.calibrationInputs;
		calib.monthOfCalibration = Text<int>(calibration_node.child("time"));
		calib.steadyPrevPopulation = Text<int>(calibration_node.child("partnershipOutcomes").child("steadyPrev").child("popOfInterest"));
		calib.steadyPrevBounds.lower = Text<double>(calibration_node.child("partnershipOutcomes").child("steadyPrev").child("lwrBound"));
		calib.steadyPrevBounds.upper = Text<double>(calibration_node.child("partnershipOutcomes").child("steadyPrev").child("uprBound"));
		calib.casualPrevPopulation = Text<int>(calibration_node.child("partnershipOutcomes").child("casualPrev").child("popOfInterest"));
		calib.casualPrevBounds.lower = Text<double>(calibration_node.child("partnershipOutcomes").child("casualPrev").child("lwrBound"));
		calib.casualPrevBounds.upper = Text<double>(calibration_node.child("partnershipOutcomes").child("casualPrev").child("uprBound"));
		calib.CSWPrevPopulation = Text<int>(calibration_node.child("partnershipOutcomes").child("cswPrev").child("popOfInterest"));
		calib.CSWPrevBounds.lower = Text<double>(calibration_node.child("partnershipOutcomes").child("cswPrev").child("lwrBound"));
		calib.CSWPrevBounds.upper = Text<double>(calibration_node.child("partnershipOutcomes").child("cswPrev").child("uprBound"));
		calib.propInConcurrentPopulation = Text<int>(calibration_node.child("partnershipOutcomes").child("propInCon").child("popOfInterest"));
		calib.propInConcurrentBounds.lower = Text<double>(calibration_node.child("partnershipOutcomes").child("propInCon").child("lwrBound"));
		calib.propInConcurrentBounds.upper = Text<double>(calibration_node.child("partnershipOutcomes").child("propInCon").child("uprBound"));
		calib.numActsPopulation = Text<int>(calibration_node.child("partnershipOutcomes").child("numActs").child("popOfInterest"));
		calib.numActsBounds.lower = Text<double>(calibration_node.child("partnershipOutcomes").child("numActs").child("lwrBound"));
		calib.numActsBounds.upper = Text<double>(calibration_node.child("partnershipOutcomes").child("numActs").child("uprBound"));
		calib.femaleCasualPrevRatio = Text<double>(calibration_node.child("partnershipOutcomes").child("femaleCasualPrev").child("ratio"));
		calib.femalePropInConcurrentRatio = Text<double>(calibration_node.child("partnershipOutcomes").child("femalePropInCon").child("ratio"));
		calib.femaleNumActsLRtoHRRatio = Text<double>(calibration_node.child("partnershipOutcomes").child("femaleNumActsLRtoHR").child("ratio"));

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
		//TODO-GA: this somehow disappeared from the xml files, let's set it to 1 for the moment.
		//calib.thresholdPrevMult = Text<double>(calibration_node.child("thresholdMultiplier"));
		calib.thresholdPrevMult = 1.00;
	}

	auto interventions_node = simulation_node.child("interventions");

	ParseCepacSimContexts(interventions_node, parameters);

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

void SimulationBuilderXml::ParseCepacSimContexts(const pugi::xml_node &interventions_node,
    EventParams &parameters)
{
    parameters.useRollout = Attr<bool>(
	interventions_node.child("artRolloutIntervention"), "enabled");

    std::string treatment_files_path;
    if(parameters.useRollout) {
	auto art_rollout_node = interventions_node.child("artRolloutIntervention");
	auto scaling_node = art_rollout_node.child("dynamicTreatmentScaling");
	parameters.dynamicFeedbackPeriod = Text<int>(scaling_node.child("feedbackPeriod"));
	parameters.enableDynamicTreatmentScaling = Attr<bool>(scaling_node, "enabled");

	// Use ART rollout files as the CEPAC SimContext
	treatment_files_path = "artRolloutIntervention/rolloutTreatmentFiles/rolloutFile";

	parameters.rolloutEligibility = ReadRolloutEligibility();
	parameters.rolloutProportionDenom = ReadRolloutDenominator();
	for(auto target : art_rollout_node.select_nodes("targetRolloutProportions/target")) {
	    auto year = Attr<int>(target.node(), "year");
	    parameters.targetYearlyRolloutProportions[year] = Text<double>(target.node());
	}
    } else {
	// Use standard treatment files as the CEPAC SimContext
	treatment_files_path = "cepacIntervention/cepacTreatmentFiles/treatmentFile";
    }

    // Read the input CEPAC file information
    for(auto treatment_file_node : interventions_node.select_nodes(treatment_files_path.c_str())) {
	ParseSimContextFile(treatment_file_node.node(), parameters);
    }

    //From the first file only, get the death tables for non-AIDS death
    SimContext *simContext = parameters.cepacSimContexts.front()->simContext.get();
    // and set the cepac output trace to the first simContext file
    parameters.cepacTracer = new Tracer(parameters.simName, simContext, 1);
    parameters.cepacRunStats = new RunStats(parameters.simName, simContext);
    parameters.capacCostStats = new CostStats(parameters.simName, simContext);
}

void SimulationBuilderXml::ParseSimContextFile(const pugi::xml_node &treatment_file_node,
    EventParams &parameters)
{
    int time = treatment_file_node.child("time").text().as_int();

    if(time > -1) {
	std::string file_name = treatment_file_node.child("fileName").text().as_string();
	int file_number = treatment_file_node.child("fileNumber").text().as_int();
	int target_population = treatment_file_node.child("popToApply").text().as_int();

	//Set the CEPAC simContext from the specified CEPAC .in file
	auto contextToAdd = std::make_unique<SimContext>(
	    file_name.substr(0, file_name.find(CepacUtil::FILE_EXTENSION_FOR_INPUT)));
	EventParams::CepacSimContext *cdmSimContext =
	    new EventParams::CepacSimContext(time, std::move(contextToAdd), target_population);
	parameters.cepacSimContexts.push_back(cdmSimContext);

	// Note: can't use *cmdSimContext since std::move(contextToAdd) clears the pointer
	SimContext *simContext = parameters.cepacSimContexts.back()->simContext.get();
	simContext->numPatientsToTrace = 0;

	//Read in the inputs
	try	{
	   simContext->readInputs();
	} catch(std::string errorString) {
	    throw std::runtime_error("error loading CEPAC file, " + file_name + ": " + errorString);
	}
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

    std::map<SexualPartnership::Type, double> assort;

    for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
    {
        auto assortativeness = population_parameters.GetMaleParameters().getSexualBehavior(partnership_type).getAssortativeness();
        assort[partnership_type] = assortativeness;
    }

	//create EntityPool - this will contain all Entities
	auto entities = std::make_unique<EntityPool>(population_parameters.getAgeOfMajority(), population.GetId(), population_parameters, assort);
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
	long totalNumMales = Utility::round<long>(population_parameters.GetInitialSize() * population_parameters.GetMaleProportion());
	long totalNumFemales = std::max<long>(population_parameters.GetInitialSize() - totalNumMales, 0);
	//the params.xml file should have detailed the prevalent characteristics of each age bucket
	//  we will go through each age bucket and create the part of the prevalent population that falls within the bucket
	std::vector<AgeRange> ageRanges;

	for (auto ageBucketParams : population_parameters.GetAgeDistributions()) {
	    auto numMalesInCurrentBucket = Utility::round<std::size_t>(
		totalNumMales * ageBucketParams.proportionOfPopulation[(std::size_t)DemographicProfile::Gender::Male]);
	    auto numFemalesInCurrentBucket = Utility::round<std::size_t>(
		totalNumFemales * ageBucketParams.proportionOfPopulation[(std::size_t)DemographicProfile::Gender::Female]);

	    //calc how many people are in the current age range
	    auto currentBucketSize = numMalesInCurrentBucket + numFemalesInCurrentBucket;

	    //Number of persons of each gender to be traced in detailed output file
	    auto numToTrace = static_cast<std::size_t>(population.GetNumberToTrace());

	    for(std::size_t count = 0; count < currentBucketSize; count++) {
		//Determine whether or not person should be traced in SinglePersonTrace
		bool tracePerson = (count < numToTrace || (count >= numMalesInCurrentBucket && (count - numMalesInCurrentBucket) < numToTrace));

		//create a person, males first and females second
		auto gender = (count < numMalesInCurrentBucket) ? DemographicProfile::Gender::Male : DemographicProfile::Gender::Female;
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
	} //while(numCouples > 0) {

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
	} //while(numCouples > 0) {

	//count the size of the population and store value
	population.UpdateSize();
}

EventParams::RolloutEligibility SimulationBuilderXml::ReadRolloutEligibility()
{
    auto eligibility_node = document_.select_node("/simulation/interventions/artRolloutIntervention/rolloutEligibility").node();
    EventParams::RolloutEligibility eligibility;

    // Identified
    auto identified_node = eligibility_node.select_single_node("criteria[@name='Identified']").node();
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

EventParams::RolloutDenominator SimulationBuilderXml::ReadRolloutDenominator()
{
    auto node = document_.select_node("/simulation/interventions/artRolloutIntervention/targetRolloutProportions").node();
    EventParams::RolloutDenominator denom = EventParams::RolloutDenominator::DEFAULT;

    try {
        std::string value = Attr<std::string>(node, "proportionDenominator");

	if (value == "population") {
	    denom = EventParams::RolloutDenominator::POPULATION;
	} else if (value == "eligible") {
	    denom =  EventParams::RolloutDenominator::ELIGIBLE;
	}
    } catch (std::string err) {
        // denominator not specified
    }

    return denom;
}

SexualBehavior SimulationBuilderXml::ReadSexualBehavior(SexualPartnership::Type type)
{
	auto path = "/simulation/population/entities/entity[@type='Male']/behavior/partnershipTypes/partnership[@type='" + to_string(type) + "']";
	auto node = document_.select_node(path.c_str()).node();

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

	for(auto risk : {Person::LOW, Person::HIGH})
	{
		std::string suffix = risk == Person::LOW ? "LowRisk" : "HighRisk";

		result.setAcquisitionRatePerMonth(risk, GetLogNormalDist(node.child(("acquisitionRate" + suffix).c_str())));
		//XXX:this should be a double, but old implementations mistakenly casted it to int
		//we will continue to do this to maintain reproduciblity for now
		result.setCoitalEventsPerMonth(risk, Text<double>(node.child(("coitalEventsPerMonth" + suffix).c_str()).child("distribution").child("mean")));
		result.setChanceCondomUsePerEvent(risk, GetBetaDist(node.child(("chanceCondomUsePerEvent" + suffix).c_str())));
		result.setPartnershipDuration(risk, GetShiftedLogNormalDist(node.child(("partnershipDurationMth" + suffix).c_str())));
	}

	return result;
}

Male::SubPopParams SimulationBuilderXml::ReadMaleSubPopParams()
{
	auto node = document_.select_node("/simulation/population/entities/entity[@type='Male']").node();

	Male::SubPopParams result;

	auto behavior_node = node.child("behavior");
	result.SetChanceBecomeCsw(Text<double>(behavior_node.child("chanceBecomeSexWorker")));
	result.SetPartnerAcqMultWithSteady(Person::HIGH, Text<double>(behavior_node.child("partnerAcqMultWithSteadyHighRisk")));
	result.SetPartnerAcqMultWithSteady(Person::LOW, Text<double>(behavior_node.child("partnerAcqMultWithSteadyLowRisk")));

	bool use_high_risk_multiplier = Attr<bool>(behavior_node.child("highRiskAcqRateMultiplier"), "enabled");
	double high_risk_multiplier = Text<double>(behavior_node.child("highRiskAcqRateMultiplier"));
	bool use_csw_high_risk_multiplier = Attr<bool>(behavior_node.child("highRiskCswAcqRateMultiplier"), "enabled");
	double csw_high_risk_multiplier = Text<double>(behavior_node.child("highRiskCswAcqRateMultiplier"));

	result.SetCoefficientVariation(false, 0);

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

		result.AddSexualBehavior(params);
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
    result.SetPreExposureProphylaxisEfficacy(Text<double>(health_node.child("preExposureProphylaxisEfficacy")));

	auto transmission_node = node.child("health").child("transmissionCoefficients");
	auto transmission_coefficients = Text<std::array<double, 7>>(transmission_node.child("valsByHVL"));
	for(int i = 0; i < 7; i++)
	{
		result.SetTransmitPerEventCoeff(Person::HVLStrata(i), transmission_coefficients[i]);
	}
	result.SetTransmitPerEventCoeff(Person::HVL_PRIMARY, Text<double>(transmission_node.child("primary")));
	result.SetTransmitPerEventCoeff(Person::HVL_LATESTAGE, Text<double>(transmission_node.child("lateStage")));

    result.SetMaxPartnershipRejections(Text<int>(behavior_node.child("maxPartnershipRejections")));

	return result;
}


Female::SubPopParams SimulationBuilderXml::ReadFemaleSubPopParams()
{
	auto node = document_.select_node("/simulation/population/entities/entity[@type='Female']").node();

	Female::SubPopParams result;

	auto behavior_node = node.child("behavior");
	result.SetChanceBecomeCsw(Text<double>(behavior_node.child("chanceBecomeSexWorker")));
	result.SetProportionHighRisk(DemographicProfile::Employment::NonCsw, Text<double>(behavior_node.child("proportionHighRiskNonCsw")));
	result.SetProportionHighRisk(DemographicProfile::Employment::Csw, Text<double>(behavior_node.child("proportionHighRiskCsw")));

	NormalDist activityLevel;
	activityLevel.mean = 1;
	activityLevel.stddev = 0;
	result.SetActivityLevel(activityLevel);

    auto health_node = node.child("health");
    result.SetPreExposureProphylaxisEfficacy(Text<double>(health_node.child("preExposureProphylaxisEfficacy")));
    result.SetVaginalMicrobicideEfficacy(Text<double>(health_node.child("vaginalMicrobicideEfficacy")));
	auto transmission_node = health_node.child("transmissionCoefficients");
	auto transmission_coefficients = Text<std::array<double, 7>>(transmission_node.child("valsByHVL"));
	for(int i = 0; i < 7; i++)
	{
		result.SetTransmitPerEventCoeff(Person::HVLStrata(i), transmission_coefficients[i]);
	}
	result.SetTransmitPerEventCoeff(Person::HVL_PRIMARY, Text<double>(transmission_node.child("primary")));
	result.SetTransmitPerEventCoeff(Person::HVL_LATESTAGE, Text<double>(transmission_node.child("lateStage")));

	return result;
}

void SimulationBuilderXml::ReadPopulationParameters()
{
    auto population_node = document_.child("simulation").child("population");

    auto initial_state_node = population_node.child("initialState");
    population_parameters.SetInitialSize(Text<int>(initial_state_node.child("size")));
	
    population_parameters.SetInitialCswProportion(DemographicProfile::Gender::Male,
        Text<double>(initial_state_node.child("chanceBeingCswMale")));
    population_parameters.SetInitialCswProportion(DemographicProfile::Gender::Female,
        Text<double>(initial_state_node.child("chanceBeingCswFemale")));
    population_parameters.SetCswEndAge(DemographicProfile::Gender::Male,
        Utility::convertTime(TimeGranularity::Year, TimeGranularity::Month,
			     Text<int>(initial_state_node.child("cswEndAgeMale"))));
    population_parameters.SetCswEndAge(DemographicProfile::Gender::Female,
        Utility::convertTime(TimeGranularity::Year, TimeGranularity::Month,
			     Text<int>(initial_state_node.child("cswEndAgeFemale"))));

    //get initial age distribution
    for(auto age_bucket_node : initial_state_node.child("ageDistribution").children("range")) {
	population_parameters.GetAgeDistributions().emplace_back(
	    Utility::convertTime(TimeGranularity::Year, TimeGranularity::Month,
				 Attr<int>(age_bucket_node, "lower")),
	    Utility::convertTime(TimeGranularity::Year, TimeGranularity::Month,
				 Attr<int>(age_bucket_node, "upper")) + 11,
	    Text<double>(age_bucket_node.child("distribMale")),
	    Text<double>(age_bucket_node.child("distribFemale")));
    }
    
    //normalize %population values for each age bucket
    std::array<double, (std::size_t)DemographicProfile::Gender::Last> totalPopulationproportionages;
    totalPopulationproportionages.fill(0);

    //get the total of proportionage values
    for(std::size_t i = 0; i < (std::size_t)DemographicProfile::Gender::Last; i++) {
	for(auto &age_bucket : population_parameters.GetAgeDistributions()) {
	    totalPopulationproportionages[i] += age_bucket.proportionOfPopulation[i];
	}

	//normalize each proportionage value so that the sum of them == 1
	for(auto &age_bucket : population_parameters.GetAgeDistributions()) {
	    age_bucket.proportionOfPopulation[i] /= totalPopulationproportionages[i];
	}
    }

    //get parameters related to the seeding of infected individuals
    auto seed_distrib_node = initial_state_node.child("seedDistribution");
    population_parameters.SetSeedDelay(Attr<int>(seed_distrib_node, "seedDelay"));
    population_parameters.SetUseSeedCoefficients(
	Attr<bool>(seed_distrib_node, "useCoefficients"));
    population_parameters.SetChanceChronicInfection(
	Text<double>(seed_distrib_node.child("chanceSeedChronicInfection")));
    population_parameters.SetSeedPrevalence(
	Text<double>(seed_distrib_node.child("seedPrevalence")));

    for(auto range_node : seed_distrib_node.children("range")) {
	population_parameters.GetSeedDistributions().emplace_back(
	    Utility::convertTime(TimeGranularity::Year, TimeGranularity::Month,
				 Attr<int>(range_node, "lower")),
	    Utility::convertTime(TimeGranularity::Year, TimeGranularity::Month,
				 Attr<int>(range_node, "upper")) + 11,
	    Text<int>(range_node.child("numInfectedMaleCsw")),
	    Text<int>(range_node.child("numInfectedFemaleCsw")),
	    Text<int>(range_node.child("numInfectedMaleLowRisk")),
	    Text<int>(range_node.child("numInfectedFemaleLowRisk")),
	    Text<int>(range_node.child("numInfectedMaleHighRisk")),
	    Text<int>(range_node.child("numInfectedFemaleHighRisk")));
    }

    population_parameters.setBirthRate(
	Text<double>(population_node.child("birthRate")));
    population_parameters.setProportionMale(
	Text<double>(population_node.child("proportionMale")));
    population_parameters.setProportionCircumcised(
	Text<double>(population_node.child("proportionCircumcised")));
    population_parameters.setAgeOfMajority(
	Text<int>(population_node.child("ageOfMajority")), TimeGranularity::Year);

    auto defaultMaleParams = ReadMaleSubPopParams();
    population_parameters.SetMaleParameters(defaultMaleParams);
    auto defaultFemaleParams = ReadFemaleSubPopParams();
    population_parameters.SetFemaleParameters(defaultFemaleParams);

    //save flags to indicate whether particular partnership types have duration or not
    for(auto type : enum_iterator<SexualPartnership::Type>()) {
	auto has_duration = !(defaultMaleParams.getSexualBehavior(SexualPartnership::Type(type))
			      .getPartnershipDurationMth(Person::LOW).isZeroDistrib)
	    && !(defaultMaleParams.getSexualBehavior(SexualPartnership::Type(type))
		 .getPartnershipDurationMth(Person::HIGH).isZeroDistrib);
	population_parameters.SetPartnershipHasDuration(
	    DemographicProfile::Gender::Male, type, has_duration);
	population_parameters.SetPartnershipHasDuration(
	    DemographicProfile::Gender::Female, type, false);
    }

    pugi::xml_node costs_node = document_.select_node(
	"/simulation/traceFiles/costEffectiveness").node();

    //Costs
    population_parameters.SetCondomCost(Text<double>(costs_node.child("condomCost")));
    population_parameters.SetCircumcisionCost(
	Text<double>(costs_node.child("circumcisionCost")));
    population_parameters.SetPrEPCost(Text<double>(costs_node.child("prEPCost")));
    population_parameters.SetVaginalMicrobicideCost(
	Text<double>(costs_node.child("vaginalMicrobicideCost")));
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
                target.value.observed_hiv_status.value = Person::NEGATIVE;
            }
            else if(value == "observed-acute")
            {
                target.value.observed_hiv_status.value = Person::OBSERVED_ACUTE;
            }
            else if(value == "unobserved-acute")
            {
                target.value.observed_hiv_status.value = Person::UNOBSERVED_ACUTE;
            }
            else if(value == "observed-chronic")
            {
                target.value.observed_hiv_status.value = Person::OBSERVED_CHRONIC;
            }
            else if(value == "unobserved-chronic")
            {
                target.value.observed_hiv_status.value = Person::UNOBSERVED_CHRONIC;
            }
            else if(value == "observed-latestage")
            {
                target.value.observed_hiv_status.value = Person::OBSERVED_LATESTAGE;
            }
            else if(value == "unobserved-latestage")
            {
                target.value.observed_hiv_status.value = Person::UNOBSERVED_LATESTAGE;
            }
            else if(value == "any-positive")
            {
                target.value.observed_hiv_status.value = Person::ANY_POSITIVE;
            }
            else if (value == "not-observed-positive")
            {
                target.value.observed_hiv_status.value = Person::ANY_NOT_OBSERVED_POSITIVE;
            }
            else if (value == "observed-positive")
            {
                target.value.observed_hiv_status.value = Person::ANY_OBSERVED_POSITIVE;
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
                target.value.risk_level.value = Person::HIGH;
            }
            else if(value == "low")
            {
                target.value.risk_level.value = Person::LOW;
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
        document_.select_node("/simulation/interventions/groups").node();
    std::unordered_map<std::string, TargetGroup> groups;

    for(auto group_node : groups_node.children("group"))
    {
        auto label = Attr<std::string>(group_node, "label");
        auto enrollment_start = Text<int>(group_node.child("enrollment-start"));
        auto enrollment_end = Text<int>(group_node.child("enrollment-end"));
        bool open = Text<bool>(group_node.child("open-enrollment"));
        auto target = ParseGroupEligibility(group_node.child("eligibility-criteria"));

        TargetGroup group(label, enrollment_start, enrollment_end,
            open, target);

        for(auto partition_node : group_node.child("partitions").children("partition"))
        {
            std::string label = Attr<std::string>(partition_node, "label");
            auto proportion = Text<double>(partition_node.child("proportion"));
            auto trace = Text<bool>(partition_node.child("trace"));
            auto interventions = ParseInterventions(partition_node.child("interventions"), true);

            group.AddPartition(label, trace, proportion, interventions);
        }

        groups.emplace(std::make_pair(label, group));
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
KnownIntervention SimulationBuilderXml::from_string(const std::string &intervention)
{
    auto match = std::find_if(KnownInterventionStrings.begin(), 
        KnownInterventionStrings.end(), 
        [&](const std::pair<KnownIntervention, std::string> &e) 
    { 
        return e.second == intervention; 
    });

    if (match == KnownInterventionStrings.end())
    {
        throw std::runtime_error("unknown intervention: " + intervention);
    }

    return match->first;
}

template<>
Person::RiskLevel SimulationBuilderXml::from_string(const std::string &risk)
{
    if(risk == "high") return Person::RiskLevel::HIGH;
    if(risk == "low") return Person::RiskLevel::LOW;

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
Person::HVLStrata SimulationBuilderXml::from_string(const std::string &hvl_string)
{
    if(hvl_string == "-1" || hvl_string == "uninfected") return Person::HVLStrata::UNINFECTED;
    if(hvl_string == "0") return Person::HVLStrata::HVL_ZERO;
    if(hvl_string == "1") return Person::HVLStrata::HVL_ONE;
    if(hvl_string == "2") return Person::HVLStrata::HVL_TWO;
    if(hvl_string == "3") return Person::HVLStrata::HVL_THREE;
    if(hvl_string == "4") return Person::HVLStrata::HVL_FOUR;
    if(hvl_string == "5") return Person::HVLStrata::HVL_FIVE;
    if(hvl_string == "6") return Person::HVLStrata::HVL_SIX;
    if(hvl_string == "7" || hvl_string == "primary") return Person::HVLStrata::HVL_PRIMARY;
    if(hvl_string == "8" || hvl_string == "late-stage") return Person::HVLStrata::HVL_LATESTAGE;

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

template<>
double SimulationBuilderXml::TransformInterventionValue(double target_value,
    double curr_value, int time, int duration)
{
    double new_value;

    int lapsed = duration - (simulation_.GetTime() - time) + 1;
    double coeff = (target_value - curr_value) / lapsed;
    new_value = curr_value + coeff;

    return new_value;
}

/*
 * Calculates the current normal distribution when transforming an intervention.
 */
template<>
NormalDist SimulationBuilderXml::TransformInterventionValue(NormalDist target_dist,
    NormalDist curr_dist, int time, int duration)
{
    NormalDist new_dist;

    // calculate the amount to add each time step so we don't have to store a coeff value
    int lapsed = duration - (simulation_.GetTime() - time) + 1;
    double mean_coeff = (target_dist.mean - curr_dist.mean) / lapsed;
    double stddev_coeff = (target_dist.stddev - curr_dist.stddev) / lapsed;
    new_dist.mean = curr_dist.mean + mean_coeff;
    new_dist.stddev = curr_dist.stddev + stddev_coeff;

    return new_dist;
}

void SimulationBuilderXml::SetChanceCondomUseCallback(pugi::xml_node &node,
    Intervention &intervention, bool individual)
{
    auto risk = Attr<Person::RiskLevel>(node, "risk");
    auto type = Attr<SexualPartnership::Type>(node, "type");
    bool transform = false;
    if (node.child("transform"))
	transform = Text<bool>(node.child("transform"));
    NormalDist target_dist = GetNormalDist(node);
    // check that conversion from normal to beta is possible
    BetaDist::FromNormal(target_dist);

    int time = intervention.GetTime();
    int duration = intervention.GetDuration();
    if (!individual) {
	intervention.SetPopulationCallback(
	    [=](Population &p) {
		BetaDist target_beta_dist;
		if (transform) {
		    // increase or descrease to the target value over the duration
		    NormalDist curr_dist = BetaDist::ToNormal(p.popWideParams.
		        GetChanceCondomUsePerEvent(risk, type));
		    target_beta_dist = BetaDist::FromNormal(TransformInterventionValue(
		        target_dist, curr_dist, time, duration));
		} else {
		    // set the target value immediately
		    target_beta_dist = BetaDist::FromNormal(target_dist);
		}

		p.popWideParams.SetChanceCondomUsePerEvent(risk, type, target_beta_dist);
	    }
	);
    }
    intervention.SetIndividualCallback(
	[=](Person *person) {
	    BetaDist target_beta_dist;
	    if (transform) {
		// increase or descrease to the target value over the duration
		NormalDist curr_dist = BetaDist::ToNormal(person->
	            GetChanceCondomUsePerEvent(risk, type));
		target_beta_dist = BetaDist::FromNormal(TransformInterventionValue(
		    target_dist, curr_dist, time, duration));
	    } else {
		// set the target value immediately
		target_beta_dist = BetaDist::FromNormal(target_dist);
	    }

	    person->SetChanceCondomUsePerEvent(risk, type, target_beta_dist,
					       simulation_.GetEventParams().randomNums);
	}
    );
}

void SimulationBuilderXml::SetProportionCircumcisedCallback(pugi::xml_node &node,
    Intervention &intervention)
{
    bool transform = false;
    if (node.child("transform"))
	transform = Text<bool>(node.child("transform"));
    double target_value = Text<double>(node.child("proportion"));

    int time = intervention.GetTime();
    int duration = intervention.GetDuration();
    intervention.SetPopulationCallback (
	[=](Population &p) {
	    double new_value = target_value;
	    if (transform) {
	        // increase or descrease to the target value over the duration
	        double curr_value = p.popWideParams.getProportionCircumcised();
		new_value = TransformInterventionValue(
		    target_value, curr_value, time, duration);
	    }

	    p.popWideParams.setProportionCircumcised(new_value);
	}
    );
}

void SimulationBuilderXml::SetCircumciseCallback(pugi::xml_node &node,
    Intervention &intervention, bool individual)
{
    if (individual) {
      intervention.SetPopulationIndividualCallback(
        [=](Population &population, Person *person) {
	    population.Circumcise(person);
        }
      );
    } else {
        bool transform = false;
	if (node.child("transform"))
	    transform = Text<bool>(node.child("transform"));
        double target_value = Text<double>(node.child("proportion"));

        int time = intervention.GetTime();
	int duration = intervention.GetDuration();
	intervention.SetPopulationCallback (
	  [=](Population &p) {
	      double curr_value = p.popWideParams.getProportionCircumcised();
	      double new_value = target_value;
	      if (transform) {
	          // increase or descrease to the actual value over the duration
		  curr_value = (double) (p.num_circumcised_sa + p.num_circumcised_na) /
		      (double) p.GetSize(DemographicProfile::Gender::Male);
		  new_value = TransformInterventionValue(
		      target_value, curr_value, time, duration);
	      }
	      p.Circumcise(abs(new_value - curr_value));
	  }
        );
    }
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
	    SetCircumciseCallback(node, intervention, individual);
            break;
        }
        case KnownIntervention::ChanceBecomeSexWorker:
        {
            auto chance = Text<double>(node);
            intervention.SetIndividualCallback(
                [=](Person *person) {
                    person->SetChanceBecomeSexWorker(chance); });
            break;
        }
        case KnownIntervention::DelaySexualActivity:
        {
            auto months = Text<int>(node);
            intervention.SetIndividualCallback(
                [=](Person *person) { 
                    person->SetSexualActivityDelay(months); });
            break;
        }
        case KnownIntervention::TransmissionCoefficient:
        {
            auto hvl_stratum = Attr<Person::HVLStrata>(node, "hvl");
            auto coefficient = Text<double>(node);
            intervention.SetIndividualCallback(
                [=](Person *person) { 
                    person->SetTransmissionCoefficient(hvl_stratum, coefficient); });
            break;
        }
        case KnownIntervention::AverageYearsYounger:
        {
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = GetNormalDist(node);
            intervention.SetIndividualCallback(
                [=](Person *person) { 
                    person->SetAverageYearsYounger(partnership_type, dist); });
            break;
        }
        case KnownIntervention::PartnerAcquisitionRate:
        {
            auto risk = Attr<Person::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = GetLogNormalDist(node);
            intervention.SetIndividualCallback(
                [=](Person *person) { 
                    person->SetAcquisitionRatePerMonth(risk, partnership_type, dist, simulation_.GetEventParams().randomNums); });
            break;
        }
        case KnownIntervention::CoitalEventsPerMonth:
        {
            auto risk = Attr<Person::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = Text<double>(node.child("distribution").child("mean"));
            intervention.SetIndividualCallback(
                [=](Person *person) { 
                    person->SetCoitalEventsPerMonth(risk, partnership_type, dist); });
            break;
        }
        case KnownIntervention::ChanceCondomUse:
        {
	    SetChanceCondomUseCallback(node, intervention, individual);
            break;
        }
        case KnownIntervention::PartnershipDuration:
        {
            auto risk = Attr<Person::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = GetShiftedLogNormalDist(node);
            intervention.SetIndividualCallback(
                [=](Person *person) { 
                    person->SetPartnershipDuration(risk, partnership_type, dist); });
            break;
        }
        case KnownIntervention::PartnershipRejectionChance:
        {
            auto risk = Attr<Person::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto chance = Text<double>(node);
            intervention.SetIndividualCallback(
                [=](Person *person) {
                person->SetPartnershipRejectionChance(risk, partnership_type, chance); });
            break;
        }
        case KnownIntervention::OverrideChanceCondomUse:
        {
            auto chance = Text<double>(node);
            intervention.SetIndividualCallback(
                [=](Person *person) {
                person->SetOverrideChanceCondomUse(chance); });
            break;
        }
        case KnownIntervention::CepacContext:
        {
            auto cepac_file = Text<std::string>(node);
            intervention.SetPopulationIndividualCallback([=](Population &pop, Person *person) 
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
            intervention.SetIndividualCallback([=](Person *person) 
            {
                if (person->isMale())
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
            intervention.SetIndividualCallback([=](Person *person) 
            {
                if (person->getHIVStatus() == Person::HIVStatus::OBSERVED_ACUTE
                    || person->getHIVStatus() == Person::HIVStatus::OBSERVED_LATESTAGE
                    || person->getHIVStatus() == Person::HIVStatus::OBSERVED_CHRONIC)
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
                [=](Population &p) {
                    p.popWideParams.setBirthRate(value); });
            break;
        }
	case KnownIntervention::FertilityRate:
	{
	    std::vector<FertilityRate> rates;
	    for(auto child : node.children("rateForAgeRange")) {
		auto lower = Utility::convertTime(TimeGranularity::Year, TimeGranularity::Month,
				     Attr<int>(child, "lower"));
		auto upper = Utility::convertTime(TimeGranularity::Year, TimeGranularity::Month,
						  Attr<int>(child, "upper")) + 11;
		auto value = Text<double>(child);
		FertilityRate rate(lower, upper, value);
		rates.push_back(rate);
	    }
	    intervention.SetPopulationCallback(
		[=](Population &p) {
		    p.popWideParams.UseBirthRate = false;
		    p.popWideParams.ClearFertilityRates();
		    for ( auto rate : rates)
			p.popWideParams.PushFertilityRate(rate);
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
                [=](Population &p) {
                    p.popWideParams.setProportionMale(value); });
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
                [=](Population &p) { p.popWideParams.SetChanceBecomeCsw(gender, chance); });
            intervention.SetIndividualCallback([=](Person *person)
            {
                if(person->isMale())
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
                [=](Person *person) { person->SetSexualActivityDelay(months); });
            break;
        }
        case KnownIntervention::TransmissionCoefficient:
        {
            auto gender = Attr<DemographicProfile::Gender>(node, "gender");
            auto hvl_stratum = Attr<Person::HVLStrata>(node, "hvl");
            auto coefficient = Text<double>(node);
            intervention.SetPopulationCallback(
                [=](Population &p) { p.popWideParams.SetTransmissionCoefficient(gender, hvl_stratum, coefficient); });
            intervention.SetIndividualCallback([=](Person *person)
            {
                if((DemographicProfile::Gender)person->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == gender)
                {
                    person->SetTransmissionCoefficient(hvl_stratum, coefficient);
                }
            });
            break;
        }
        case KnownIntervention::ProportionHighRisk:
        {
            auto gender = Attr<DemographicProfile::Gender>(node, "gender");
            auto employment = Attr<DemographicProfile::Employment>(node, "employment");
            auto proportion = Text<double>(node);
            intervention.SetPopulationCallback(
                [=](Population &p) { p.popWideParams.SetProportionHighRisk(gender, employment, proportion); });
            intervention.SetIndividualCallback([=](Person *person)
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
            intervention.SetIndividualCallback([=](Person *person) 
            { 
                if (person->isMale())
                {
                    person->SetAverageYearsYounger(partnership_type, dist);
                }
            });
            break;
        }
        case KnownIntervention::PartnerAcquisitionRate:
        {
            auto risk = Attr<Person::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = GetLogNormalDist(node);
            intervention.SetPopulationCallback(
                [=](Population &p) { p.popWideParams.SetAcquisitionRatePerMonth(risk, partnership_type, dist); });
            intervention.SetIndividualCallback([=](Person *person)
            {
                if (person->isMale())
                {
                    person->SetAcquisitionRatePerMonth(risk, partnership_type, dist, simulation_.GetEventParams().randomNums);
                }
            });
            break;
        }
        case KnownIntervention::CoitalEventsPerMonth:
        {
            auto risk = Attr<Person::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = Text<double>(node.child("distribution").child("mean"));
            intervention.SetPopulationCallback(
                [=](Population &p) {
		    p.popWideParams.SetCoitalEventsPerMonth(risk, partnership_type, dist); });
            intervention.SetIndividualCallback([=](Person *person)
            {
	      if (person->isMale())
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
            auto risk = Attr<Person::RiskLevel>(node, "risk");
            auto partnership_type = Attr<SexualPartnership::Type>(node, "type");
            auto dist = GetShiftedLogNormalDist(node);
            intervention.SetPopulationCallback(
                [=](Population &p) { p.popWideParams.SetPartnershipDuration(risk, partnership_type, dist); });
            intervention.SetIndividualCallback([=](Person *person) 
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
            std::string criterion = Attr<std::string>(node, "criterion");
            std::string parameter_name = Attr<std::string>(node, "parameter");

	    if (criterion == "Identified")
	    {
		intervention.SetSimulationCallback(
		    [=](Simulation &s) {
			s.parameters_.rolloutEligibility.isIdentified = Text<bool>(node);
		    });
	    }
	    else if(criterion == "OIHist")
            {
		int new_value = Text<int>(node);
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
		int new_value = Text<int>(node);
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
		int new_value = Text<int>(node);
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
		int new_value = Text<int>(node);
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
		int new_value = Text<int>(node);
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
