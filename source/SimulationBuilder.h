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
	void Create(const std::string &filename, Simulation &sim)
	{
		time_dependent_parameters_.clear();

		pugi::xml_document document;
		document.load_file(filename.c_str());
		root_ = document.child("simulation");

		for(auto intervention_iter : root_.select_nodes("/simulation/population/interventions/miscellaneousInterventions/intervention"))
		{
			auto intervention_node = intervention_iter.node();

			Simulation::TimeDependentParameter parameter;
			parameter.time = intervention_node.attribute("time").as_int();
			parameter.key = intervention_node.attribute("key").as_string();
			parameter.value = intervention_node.attribute("value").as_string();
			if(intervention_node.attribute("target") != nullptr)
			{
				parameter.target_population.has_value = true;
				parameter.target_population.value = PopulationTarget::FromString(intervention_node.attribute("target").as_string());
			}

			time_dependent_parameters_[parameter.key].push_back(parameter);
		}

		auto &parameters = sim.parameters_;

		sim.fixedSeed_ = Get<int>("/simulation/fixedSeed");
		parameters.debugLevel = static_cast<DebugLevel>(Get<int>("/simulation/debugLevel"));
		parameters.monthOf1990 = Get<int>("/simulation/monthOf1990");

		auto version = Version::FromString(Get<std::string>("/simulation/inputVersion"));

		if(Version::Compare(version, Util::MODEL_VERSION, true) != 0)
		{
			throw std::runtime_error("bad input version");
		}

		//save Concurrency Definitions
		for(int i = 0; i < 16; i++)
		{
			auto path = "/simulation/concurrencyDefinition/def" + std::to_string(i);
			auto definition_node = root_.select_single_node(path.c_str());
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
				root_.select_single_node("/simulation/writeTrace").node().child(trace_file).text().as_int() != 0;
			parameters.traceExtensions[trace_file_index] =
				root_.select_single_node("/simulation/extensionNames").node().child(trace_file).text().as_string();
			trace_file_index++;
		}

		parameters.calibrationInputs.useCalibration = Get<bool>("/simulation/calibration/useCalibration");

		if(parameters.calibrationInputs.useCalibration)
		{
			auto &calib = parameters.calibrationInputs;
			calib.monthOfCalibration = Get<int>("/simulation/calibration/monthOfCalibration");
			calib.steadyPrevPopulation = Get<int>("/simulation/calibration/monthOfCalibration");
			calib.steadyPrevBounds[Constants::LOWER] = Get<double>("/simulation/calibration/monthOfCalibration");
			calib.steadyPrevBounds[Constants::UPPER] = Get<double>("/simulation/calibration/monthOfCalibration");
			calib.casualPrevPopulation = Get<int>("/simulation/calibration/monthOfCalibration");
			calib.casualPrevBounds[Constants::LOWER] = Get<double>("/simulation/calibration/monthOfCalibration");
			calib.casualPrevBounds[Constants::UPPER] = Get<double>("/simulation/calibration/monthOfCalibration");
			calib.CSWPrevPopulation = Get<int>("/simulation/calibration/monthOfCalibration");
			calib.CSWPrevBounds[Constants::LOWER] = Get<double>("/simulation/calibration/monthOfCalibration");
			calib.CSWPrevBounds[Constants::UPPER] = Get<double>("/simulation/calibration/monthOfCalibration");
			calib.propInConcurrentPopulation = Get<int>("/simulation/calibration/monthOfCalibration");
			calib.propInConcurrentBounds[Constants::LOWER] = Get<double>("/simulation/calibration/monthOfCalibration");
			calib.propInConcurrentBounds[Constants::UPPER] = Get<double>("/simulation/calibration/monthOfCalibration");
			calib.numActsPopulation = Get<int>("/simulation/calibration/monthOfCalibration");
			calib.numActsBounds[Constants::LOWER] = Get<double>("/simulation/calibration/monthOfCalibration");
			calib.numActsBounds[Constants::UPPER] = Get<double>("/simulation/calibration/monthOfCalibration");
			calib.femaleCasualPrevRatio = Get<double>("/simulation/calibration/monthOfCalibration");
			calib.femalePropInConcurrentRatio = Get<double>("/simulation/calibration/monthOfCalibration");
			calib.femaleNumActsLRtoHRRatio = Get<double>("/simulation/calibration/monthOfCalibration");

			for(int i = 0; i < Constants::NUMBER_CALIBRATION_PREVS; i++)
			{
				auto path = "/simulation/calibration/calendarPrevalence/time" + std::to_string(i);
				calib.calendarPrevs[i] = Get<double>(path);
			}

			for(int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
			{
				auto path = "/simulation/calibration/storePoint" + std::to_string(i) + "Mth";
				calib.saveStateTimePoints[i] = Get<int>(path);
			}

			calib.thresholdPrevMult = Get<double>("/simulation/calibration/thresholdMultiplier");
		}

		sim.duration_ = Get<int>("/simulation/timeLimitMth");
		parameters.delayPrevalence = Get<int>("/simulation/population/initialState/delay");

		parameters.useRollout = Get<bool>("/simulation/population/interventions/artRolloutIntervention/useRollout");

		if(parameters.useRollout)
		{
			std::size_t treatment_file_index = 0;
			for(auto treatment_file_node : root_.select_nodes("/simulation/population/interventions/artRolloutIntervention/rolloutTreatmentFiles/rolloutFile"))
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

			parameters.rolloutEligibility = Get<EventParams::RolloutEligibility>("/simulation/population/interventions/artRolloutIntervention/rolloutEligibility");

			for(auto target : root_.select_nodes("/simulation/population/interventions/artRolloutIntervention/targetRolloutProportions/target"))
			{
				int year = target.node().attribute("year").as_int();
				double proportion = target.node().text().as_double();
				parameters.targetYearlyRolloutProportions[year] = proportion;
			}

			parameters.cepacTracer = new Tracer(parameters.simName, parameters.rolloutSimContexts[0]->rolloutSimContext, 1);
			parameters.cepacRunStats = new RunStats(parameters.simName, parameters.rolloutSimContexts[0]->rolloutSimContext);
		}
		else
		{
			parameters.cepacTracer = new Tracer(parameters.simName, parameters.cepacSimContexts[0], 1);
			parameters.cepacRunStats = new RunStats(parameters.simName, parameters.cepacSimContexts[0]);
		}

		parameters.numToTrace = Get<int>("/simulation/numberToTracePerAgeRange");
		parameters.numNewbornsToTrace = Get<int>("/simulation/numberNewbornsToTrace");
		parameters.monthTraceNewborns = Get<int>("/simulation/monthTraceNewborns");
		parameters.tracePrevalentCases = Get<bool>("/simulation/tracePrevalentCases");

		CepacUtil::setRandomSeedType(sim.fixedSeed_ == -1);

		if(sim.fixedSeed_ > -1)
		{
			//Seed is Minnesota Twins retired numbers... yes, I am a dork
			parameters.randomNums.reset(sim.fixedSeed_ == 0 ? 36291434 : sim.fixedSeed_);
		}

		BuildPopulation("/simulation/population", sim.population_);

		for(auto &parameters : time_dependent_parameters_)
		{
			std::copy(parameters.second.begin(), parameters.second.end(), std::back_inserter(sim.time_dependent_parameters_));
		}
	}

private:
	template<typename T>
	T Get(const std::string &path);

	template<>
	bool Get(const std::string &path)
	{
		return Get<int>(path) != 0;
	}

	template<>
	int Get(const std::string &path)
	{
		return root_.select_single_node(path.c_str()).node().text().as_int();
	}

	template<>
	double Get(const std::string &path)
	{
		return root_.select_single_node(path.c_str()).node().text().as_double();
	}

	template<>
	std::string Get(const std::string &path)
	{
		return root_.select_single_node(path.c_str()).node().text().as_string();
	}

	void BuildPopulation(const std::string &path, Population &population)
	{
		if(population.parameters_.useRollout)
		{
			population.applyRolloutContext(population.parameters_, 0);
		}

		//save population parameters
		population.popWideParams = Get<PopulationParams>(path);

		//create EntityPool - this will contain all Entities
		population.entities = new EntityPool(population.popWideParams.SAEntAgeMths, population.populationID, population.popWideParams.assort);
		//initialize infection trace generator print detailed info about certain ProfileID's
		// in this case, all ProfileID's w/ non-nullptr DmgProfileBuckets
		DmgProfile::ProfileID currProfileID = DmgProfile::MIN;

		while(currProfileID <= DmgProfile::MAX)
		{
			//if it is being used in this Population, then append to _profileIDs
			if((population.entities->getBucket(currProfileID) != nullptr) &&
				(DmgProfile::get(currProfileID, DmgProfile::SEXUAL_ACTIVITY_STATUS) != DmgProfile::NA))
			{
				population.popStats.infectionsTracker.addToDetailedTrace(currProfileID);
			}

			currProfileID++;
		} //while(currProfileID <= DmgProfile::MAX) {

		//initialize structures that hold people who can initiate and 'agree' to relationships.
		population.initPartnershipBuckets();

		/** Create the people in the population **/
		long totalNumMales = Util::round<long>(population.popWideParams.initSize * population.popWideParams.proportionMale);
		long totalNumFemales = std::max<long>(population.popWideParams.initSize - totalNumMales, 0);
		//the params.xml file should have detailed the prevalent characteristics of each age bucket
		//  we will go through each age bucket and create the part of the prevalent population that falls within the bucket
		std::vector<AgeRange> ageRanges;

		for(auto ageBucketParams : population.popWideParams.initialAgeBuckets)
		{
			auto numMalesInCurrentBucket = Util::round<std::size_t>(totalNumMales * ageBucketParams.proportionOfPopulation[DmgProfile::MALE]);
			auto numFemalesInCurrentBucket = Util::round<std::size_t>(totalNumFemales * ageBucketParams.proportionOfPopulation[DmgProfile::FEMALE]);

			//calc how many people are in the current age range
			auto currentBucketSize = numMalesInCurrentBucket + numFemalesInCurrentBucket;

			//Number of persons of each gender to be traced in detailed output file
			auto numToTrace = static_cast<std::size_t>(population.parameters_.numToTrace);

			for(std::size_t count = 0; count < currentBucketSize; count++)
			{
				//Determine whether or not person should be traced in SinglePersonTrace
				bool tracePerson = (count < numToTrace || (count >= numMalesInCurrentBucket && (count - numMalesInCurrentBucket) < numToTrace));

				//create a person, males first and females second
				auto gender = (count < numMalesInCurrentBucket) ? DmgProfile::MALE : DmgProfile::FEMALE;
				auto person = population.generatePerson(population.parameters_, gender, &ageBucketParams, tracePerson);

				//add the created person to the EntityPool
				population.entities->addPersonToAll(person);
			}//end for (int count

			AgeRange ageRange = {ageBucketParams.minAgeMth, ageBucketParams.maxAgeMth};
			AgeRangeSizePair ageRangeSize = std::make_pair(ageRange, currentBucketSize);

			//Add a tuple to the currSizeByAgeRange vector along with the initial size of the age range
			ageRanges.push_back(ageRange);
			population.currSizeByAgeRange.push_back(ageRangeSize);
			population.currSizeByAgeRangeMale.push_back(std::make_pair(ageRange, numMalesInCurrentBucket));
			population.currSizeByAgeRangeFemale.push_back(std::make_pair(ageRange, numFemalesInCurrentBucket));
		}

		population.popStats.artTracker.SetAgeRanges(ageRanges);

		if(population.parameters_.outputTrace[EventParams::TraceFileType::Singleperson])
		{
			population.parameters_.traceStreams[EventParams::TraceFileType::Singleperson] << endl << "Now creating initial partnerships... " << endl;
		}

		/** create prevalent Regular Partnerships (time = 0) before creating prevalent marriages **/
		//the demographics that we are pulling the eligibles from
		DmgProfile selector;
		selector.set(DmgProfile::SEXUAL_ACTIVITY_STATUS, DmgProfile::SA);
		selector.set(DmgProfile::GENDER, DmgProfile::MALE);
		selector.set(DmgProfile::SEXUAL_ORIENTATION, DmgProfile::HETERO);
		selector.set(DmgProfile::RELATIONSHIP_STATUS, DmgProfile::SINGLE);
		selector.set(DmgProfile::EMPLOYMENT, DmgProfile::NON_CSW);
		std::vector<DmgProfile::ProfileID> bucketIDs;
		//fix code below, i've put placeholders for multiple singles buckets, but right now we only use 1 of each gender
		//errhode: Is this taken care of with the whole agebucket inside SexualMixingBucket thing?
		assert(Constants::TODO_LO_PRI);
		selector.selectProfileIDs(bucketIDs, nullptr);
		DmgProfileBucket *singleMales = population.entities->getBucket(bucketIDs.at(0));
		selector.set(DmgProfile::GENDER, DmgProfile::FEMALE);
		bucketIDs.clear();
		selector.selectProfileIDs(bucketIDs, nullptr);
		DmgProfileBucket *singleFemales = population.entities->getBucket(bucketIDs.at(0));
		/** create prevalent formSteadyPartnerships (time = 0) **/
		//the demographics that we are pulling the eligibles from -- same as for regular;
		//can just use the previous singleMales and singleFemales buckets
		//number of couples -- % married of adult population by DmgProfile::SAStatus / 2
		int numCouples = Util::round<long>(population.popWideParams.initproportionMarried * (singleMales->size() + singleFemales->size()) * 0.5);

		while(numCouples > 0)
		{
			//break if there are no more people to marry...
			if(!singleFemales->size() || !singleMales->size())
			{
				break;
			}

			//choose a random male from the pool
			Male *m = (Male *)singleMales->drawMember(population.parameters_.randomNums, SexualPartnership::Type::Steady, Constants::DONT_REMOVE);

			if(m == nullptr)
			{
				break;
			}

			//try to form partnership, will add Male back to the pool if partnership was formed
			population.createPartnerships(population.parameters_, m, nullptr, SexualPartnership::Type::Steady, true);
			numCouples--;
		} //while(numCouples > 0) {

		//number of regular couples -- % married of adult population by DmgProfile::SAStatus / 2
		//Note that some people may end up in multiple relationships -- this should come out in the wash (?)
		numCouples = Util::round<int>(population.popWideParams.initproportionRegular * (singleMales->size() + singleFemales->size()) * 0.5);

		while(numCouples > 0)
		{
			//break if there are no more people to pair off...
			//this shouldn't be a problem unless we start with no men or no women as we are not shifting the pairs to non_single status
			if(!singleFemales->size() || !singleMales->size())
			{
				break;
			}

			//choose a random male from the pool
			Male *m = (Male *)singleMales->drawMember(population.parameters_.randomNums, SexualPartnership::Type::Regular, Constants::DONT_REMOVE);

			if(m == nullptr)
			{
				break;
			}

			//form partnership, will add Male back to the pool if partnership was formed
			population.createPartnerships(population.parameters_, m, nullptr, SexualPartnership::Type::Regular, true);
			numCouples--;
		} //while(numCouples > 0) {

		//count the size of the population and store value
		population.updateSize();

		population.graph = new GraphVizGraphElements();
	}

	template<>
	EventParams::RolloutEligibility Get(const std::string &base_path)
	{
		EventParams::RolloutEligibility eligibility;

		// OIHist
		std::string path = base_path + "/criteria[name=\"OIHist\"]/";
		GetTemplate(path + "rank", eligibility.oiHistRank);
		GetTemplate(path + "OI0", eligibility.oiHistOIs[0]);
		GetTemplate(path + "OI1", eligibility.oiHistOIs[1]);
		GetTemplate(path + "OI2", eligibility.oiHistOIs[2]);
		GetTemplate(path + "OI3", eligibility.oiHistOIs[3]);
		GetTemplate(path + "OI4", eligibility.oiHistOIs[4]);
		GetTemplate(path + "OI5", eligibility.oiHistOIs[5]);
		GetTemplate(path + "OI6", eligibility.oiHistOIs[6]);
		GetTemplate(path + "OI7", eligibility.oiHistOIs[7]);
		GetTemplate(path + "OI8", eligibility.oiHistOIs[8]);
		GetTemplate(path + "OI9", eligibility.oiHistOIs[9]);
		GetTemplate(path + "OI10", eligibility.oiHistOIs[10]);
		GetTemplate(path + "OI11", eligibility.oiHistOIs[11]);
		GetTemplate(path + "OI12", eligibility.oiHistOIs[12]);
		GetTemplate(path + "OI13", eligibility.oiHistOIs[13]);
		GetTemplate(path + "OI14", eligibility.oiHistOIs[14]);
		GetTemplate(path + "numOIToStart", eligibility.oiHistNumToStart);

		// CD4
		path = base_path + "/criteria[name=\"CD4\"]/";
		GetTemplate<int>(path + "rank", eligibility.cd4Rank);
		GetTemplate<int>(path + "CD4Lwr", eligibility.cd4Bounds[0]);
		GetTemplate<int>(path + "CD4Upp", eligibility.cd4Bounds[1]);

		// CD4OIHist
		path = base_path + "/criteria[name=\"CD4OIHist\"]/";
		GetTemplate<int>(path + "rank", eligibility.cd4OiHistRank);
		GetTemplate<int>(path + "CD4Lwr", eligibility.cd4OiHistCd4Bounds[0]);
		GetTemplate<int>(path + "CD4Upp", eligibility.cd4OiHistCd4Bounds[1]);
		GetTemplate<bool>(path + "OI0", eligibility.cd4OiHistOIs[0]);
		GetTemplate<bool>(path + "OI1", eligibility.cd4OiHistOIs[1]);
		GetTemplate<bool>(path + "OI2", eligibility.cd4OiHistOIs[2]);
		GetTemplate<bool>(path + "OI3", eligibility.cd4OiHistOIs[3]);
		GetTemplate<bool>(path + "OI4", eligibility.cd4OiHistOIs[4]);
		GetTemplate<bool>(path + "OI5", eligibility.cd4OiHistOIs[5]);
		GetTemplate<bool>(path + "OI6", eligibility.cd4OiHistOIs[6]);
		GetTemplate<bool>(path + "OI7", eligibility.cd4OiHistOIs[7]);
		GetTemplate<bool>(path + "OI8", eligibility.cd4OiHistOIs[8]);
		GetTemplate<bool>(path + "OI9", eligibility.cd4OiHistOIs[9]);
		GetTemplate<bool>(path + "OI10", eligibility.cd4OiHistOIs[10]);
		GetTemplate<bool>(path + "OI11", eligibility.cd4OiHistOIs[11]);
		GetTemplate<bool>(path + "OI12", eligibility.cd4OiHistOIs[12]);
		GetTemplate<bool>(path + "OI13", eligibility.cd4OiHistOIs[13]);
		GetTemplate<bool>(path + "OI14", eligibility.cd4OiHistOIs[14]);

		// HVL
		path = base_path + "/criteria[name=\"HVL\"]/";
		GetTemplate<int>(path + "rank", eligibility.hvlRank);
		GetTemplate<int>(path + "HVLLwr", eligibility.hvlBounds[0]);
		GetTemplate<int>(path + "HVLUpp", eligibility.hvlBounds[1]);

		// CD4HVL
		path = base_path + "/criteria[name=\"CD4HVL\"]/";
		GetTemplate<int>(path + "rank", eligibility.cd4HvlRank);
		GetTemplate<int>(path + "CD4Lwr", eligibility.cd4HvlCd4Bounds[0]);
		GetTemplate<int>(path + "CD4Upp", eligibility.cd4HvlCd4Bounds[1]);
		GetTemplate<int>(path + "HVLLwr", eligibility.cd4HvlHvlBounds[0]);
		GetTemplate<int>(path + "HVLUpp", eligibility.cd4HvlHvlBounds[1]);

		return eligibility;
	}

	template<>
	PopulationParams Get(const std::string &base_path)
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

	template<>
	SexualBehaviorParams Get(const std::string &base_path)
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

	std::string to_string(SexualPartnership::Type type)
	{
		switch(type)
		{
		case SexualPartnership::Type::Steady: return "Steady";
		case SexualPartnership::Type::Regular: return "Regular";
		case SexualPartnership::Type::Casual: return "Casual";
		case SexualPartnership::Type::Csw: return "CSW";
		}
		throw std::runtime_error("unknown type");
	}

	template<>
	Male::SubPopParams Get(const std::string &base_path)
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
	Female::SubPopParams Get(const std::string &base_path)
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

	template<typename T>
	void GetTargetedTemplate(const std::string &path, T &result, std::function<void(Person *, T)> callback);

	template<>
	void GetTargetedTemplate(const std::string &base_path, NormalDist &result, std::function<void(Person *, NormalDist)> callback)
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

		double sampleSize = std::stod(extracted_mean.second) * (1 - std::stod(extracted_mean.second)) / (std::stod(extracted_stddev.second) * std::stod(extracted_stddev.second)) - 1;

		result.mean = std::stod(extracted_mean.second);
		result.stddev = std::stod(extracted_stddev.second);
	}

	template<>
	void GetTargetedTemplate(const std::string &base_path, BetaDist &result, std::function<void(Person *, BetaDist)> callback)
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
	void GetTargetedTemplate(const std::string &base_path, LogNormalDist &result, std::function<void(Person *, LogNormalDist)> callback)
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

		double sampleSize = std::stod(extracted_mean.second) * (1 - std::stod(extracted_mean.second)) / (std::stod(extracted_stddev.second) * std::stod(extracted_stddev.second)) - 1;

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
	void GetTargetedTemplate(const std::string &base_path, ShiftedLogNormalDist &result, std::function<void(Person *, ShiftedLogNormalDist)> callback)
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

	void GetTargetedTemplateHack(const std::string &path, double &result, std::function<void(Person *, double)> callback)
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
	void GetTargetedTemplate(const std::string &path, int &result, std::function<void(Person *, int)> callback)
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

	template<typename T>
	void GetTemplate(const std::string &path, T &result);

	template<>
	void GetTemplate(const std::string &path, bool &result)
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
	void GetTemplate(const std::string &path, int &result)
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
	void GetTemplate(const std::string &path, double &result)
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
	void GetTemplate(const std::string &path, std::array<double, 9> &result)
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

	std::pair<std::string, std::string> ExtractParameter(const std::string &path)
	{
		std::string text(root_.select_single_node(path.c_str()).node().text().as_string());
		if(text.front() == '{' && text.back() == '}')
		{
			auto comma_index = text.find(',');
			auto key = text.substr(1, comma_index - 1);
			auto initial_value = text.substr(comma_index + 1, text.length() - comma_index - 2);
			return std::make_pair(key, initial_value);
		}
		return std::make_pair("", text);
	}

	pugi::xml_node root_;
	std::unordered_map<std::string, std::vector<Simulation::TimeDependentParameter>> time_dependent_parameters_;
};