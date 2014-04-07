#include <iostream>
#include <set>
#include <boost/filesystem.hpp>

#include "Simulation.h"
#include "Constants.h"
#include "Population.h"
#include "cepac/include.h"
#include "cepacbridge/CepacInputParser.h"
#include "data/EventParams.h"
#include "entities/classifiers/DmgProfile.h"
#include "entities/behaviors/SexualBehaviorParams.h"
#include "graphviz/graphVizParse.h"
#include "util/HighResolutionTimer.h"
#include "util/Util.h"

void Simulation::Deserialize(const pugi::xml_node &simulation_node)
{
	parameters_.debugLevel = static_cast<DebugLevel>(simulation_node.select_single_node("/simulation/debugLevel").node().text().as_int());

	fixedSeed_ = simulation_node.select_single_node("/simulation/fixedSeed").node().text().as_int();
	parameters_.monthOf1990 = simulation_node.select_single_node("/simulation/monthOf1990").node().text().as_int();
	
	auto version = Version::FromString(simulation_node.select_single_node("/simulation/inputVersion").node().text().as_string());

	if(Version::Compare(version, Util::MODEL_VERSION, true) != 0)
	{
		throw std::runtime_error("bad input version");
	}

	//save Concurrency Definitions
	for(int i = 0; i < 16; i++)
	{
		auto path = "/simulation/concurrencyDefinition/def" + std::to_string(i);
		auto definition_node = simulation_node.select_single_node(path.c_str());
		int minimum_needed = definition_node.node().child("minNeeded").text().as_int();
		bool allow = definition_node.node().child("minNeeded").text().as_int() != 0;
		parameters_.concurrencyDef[i] = EventParams::ConcurrencyDef(minimum_needed, allow);
	}

	static const auto trace_files = {"population", "infection", "partnership", "survival",
		"costEffectiveness", "clinical", "events", "health", "singleperson", "le", "partacq",
		"calibStats", "artRollout", "shiftedOutcomes"};

	std::size_t trace_file_index = 0;
	for(auto trace_file : trace_files)
	{
		parameters_.outputTrace[trace_file_index] = 
			simulation_node.select_single_node("/simulation/writeTrace").node().child(trace_file).text().as_int() != 0;
		parameters_.traceExtensions[trace_file_index] = 
			simulation_node.select_single_node("/simulation/extensionNames").node().child(trace_file).text().as_string();
		trace_file_index++;
	}

	//save calibration inputs
	parameters_.calibrationInputs.useCalibration = 
		simulation_node.select_single_node("/simulation/calibration/useCalibration").node().text().as_int() != 0;
	/*
	if(parameters_.calibrationInputs.useCalibration)
	{
		parameters_.calibrationInputs.monthOfCalibration = 
			simulation_node.select_single_node("/simulation/calibration/monthOfCalibration").node().text().as_int();
		parameters_.calibrationInputs.steadyPrevPopulation = (int)inputs_.GetCalibrationSettings().steady_target.target;
		parameters_.calibrationInputs.steadyPrevBounds[Constants::LOWER] = inputs_.GetCalibrationSettings().steady_target.lower;
		parameters_.calibrationInputs.steadyPrevBounds[Constants::UPPER] = inputs_.GetCalibrationSettings().steady_target.upper;
		parameters_.calibrationInputs.casualPrevPopulation = (int)inputs_.GetCalibrationSettings().casual_target.target;
		parameters_.calibrationInputs.casualPrevBounds[Constants::LOWER] = inputs_.GetCalibrationSettings().casual_target.lower;
		parameters_.calibrationInputs.casualPrevBounds[Constants::UPPER] = inputs_.GetCalibrationSettings().casual_target.upper;
		parameters_.calibrationInputs.CSWPrevPopulation = (int)inputs_.GetCalibrationSettings().csw_target.target;
		parameters_.calibrationInputs.CSWPrevBounds[Constants::LOWER] = inputs_.GetCalibrationSettings().csw_target.lower;
		parameters_.calibrationInputs.CSWPrevBounds[Constants::UPPER] = inputs_.GetCalibrationSettings().csw_target.upper;
		parameters_.calibrationInputs.propInConcurrentPopulation = (int)inputs_.GetCalibrationSettings().proportion_concurrent_target.target;
		parameters_.calibrationInputs.propInConcurrentBounds[Constants::LOWER] = inputs_.GetCalibrationSettings().proportion_concurrent_target.lower;
		parameters_.calibrationInputs.propInConcurrentBounds[Constants::UPPER] = inputs_.GetCalibrationSettings().proportion_concurrent_target.upper;
		parameters_.calibrationInputs.numActsPopulation = (int)inputs_.GetCalibrationSettings().num_acts_target.target;
		parameters_.calibrationInputs.numActsBounds[Constants::LOWER] = inputs_.GetCalibrationSettings().num_acts_target.lower;
		parameters_.calibrationInputs.numActsBounds[Constants::UPPER] = inputs_.GetCalibrationSettings().num_acts_target.upper;
		parameters_.calibrationInputs.femaleCasualPrevRatio = inputs_.GetCalibrationSettings().female_casual_prevalence;
		parameters_.calibrationInputs.femalePropInConcurrentRatio = inputs_.GetCalibrationSettings().female_proportion_in_concurrent;
		parameters_.calibrationInputs.femaleNumActsLRtoHRRatio = inputs_.GetCalibrationSettings().female_proportion_lr_to_hr;

		for(int i = 0; i < Constants::NUMBER_CALIBRATION_PREVS; i++)
		{
		parameters_.calibrationInputs.calendarPrevs[i] =
		calibParams->FirstChildElement("calendarPrevalence")->FirstChildElement("time" + boost::lexical_cast<std::string>
		(i))->GetText<double>();
		}

		for(int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
		{
		parameters_.calibrationInputs.saveStateTimePoints[i] = calibParams->FirstChildElement("storePoint" +
		boost::lexical_cast<std::string>(i)+"Mth")->GetText<int>();
		}

		parameters_.calibrationInputs.thresholdPrevMult =
		calibParams->FirstChildElement("thresholdMultiplier")->GetText<double>();
	}
	*/

	duration_ = simulation_node.select_single_node("/simulation/timeLimitMth").node().text().as_int();
	parameters_.delayPrevalence = 
		simulation_node.select_single_node("/simulation/population/initialState/delay").node().text().as_int();
	parameters_.useRollout = 
		simulation_node.select_single_node("/simulation/population/interventions/artRolloutIntervention/useRollout").node().text().as_int() != 0;

	if(parameters_.useRollout)
	{
		std::size_t treatment_file_index = 0;
		for(auto treatment_file_node : simulation_node.select_nodes("/simulation/population/interventions/artRolloutIntervention/rolloutTreatmentFiles/rolloutFile"))
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
				parameters_.rolloutSimContexts.push_back(new EventParams::RolloutContext(time, contextToAdd, target_population));
				//Don't trace any CEPAC patients -- the output doesn't make any sense and it just gets overly large for no reason
				//TODO: The reason is because the CEPAC Patient number doesn't get updated until the patient dies: this should be changed!
				//parameters_.cepacSimContext->numPatientsToTrace = 0;
				parameters_.rolloutSimContexts.at(file_number)->rolloutSimContext->numPatientsToTrace = 0;

				//Read in the inputs
				try
				{
					parameters_.rolloutSimContexts.back()->rolloutSimContext->readInputs();
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
			simulation_node.select_single_node("/simulation/population/interventions/artRolloutIntervention/rolloutEligibility").node();

		// OIHist
		parameters_.rolloutEligibility.oiHistRank = 
			eligibility_node.select_single_node("criteria[name=\"OIHist\"]/rank").node().text().as_int();
		parameters_.rolloutEligibility.oiHistNumToStart =
			eligibility_node.select_single_node("criteria[name=\"OIHist\"]/numOIToStart").node().text().as_int();

		// CD4
		parameters_.rolloutEligibility.cd4Rank =
			eligibility_node.select_single_node("criteria[name=\"CD4\"]/rank").node().text().as_int();
		parameters_.rolloutEligibility.cd4Bounds[0] =
			eligibility_node.select_single_node("criteria[name=\"CD4\"]/CD4Lwr").node().text().as_int();
		parameters_.rolloutEligibility.cd4Bounds[1] =
			eligibility_node.select_single_node("criteria[name=\"CD4\"]/CD4Upp").node().text().as_int();

		// CD4OIHist
		parameters_.rolloutEligibility.cd4OiHistRank =
			eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/rank").node().text().as_int();
		parameters_.rolloutEligibility.cd4OiHistCd4Bounds[0] =
			eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/CD4Lwr").node().text().as_int();
		parameters_.rolloutEligibility.cd4OiHistCd4Bounds[1] =
			eligibility_node.select_single_node("criteria[name=\"CD4OIHist\"]/CD4Upp").node().text().as_int();

		// HVL
		parameters_.rolloutEligibility.hvlRank =
			eligibility_node.select_single_node("criteria[name=\"HVL\"]/rank").node().text().as_int();
		parameters_.rolloutEligibility.hvlBounds[0] =
			eligibility_node.select_single_node("criteria[name=\"HVL\"]/HVLLwr").node().text().as_int();
		parameters_.rolloutEligibility.hvlBounds[1] =
			eligibility_node.select_single_node("criteria[name=\"HVL\"]/HVLUpp").node().text().as_int();

		// CD4HVL
		parameters_.rolloutEligibility.cd4HvlRank =
			eligibility_node.select_single_node("criteria[name=\"CD4HVL\"]/rank").node().text().as_int();
		parameters_.rolloutEligibility.cd4HvlCd4Bounds[0] =
			eligibility_node.select_single_node("criteria[name=\"CD4HVL\"]/CD4Lwr").node().text().as_int();
		parameters_.rolloutEligibility.cd4HvlCd4Bounds[1] =
			eligibility_node.select_single_node("criteria[name=\"CD4HVL\"]/CD4Upp").node().text().as_int();
		parameters_.rolloutEligibility.cd4HvlHvlBounds[0] =
			eligibility_node.select_single_node("criteria[name=\"CD4HVL\"]/HVLLwr").node().text().as_int();
		parameters_.rolloutEligibility.cd4HvlHvlBounds[1] =
			eligibility_node.select_single_node("criteria[name=\"CD4HVL\"]/HVLUpp").node().text().as_int();

		// OIHist/CD4OIHist OIs
		for(int i = 0; i < 15; i++)
		{
			auto path = "criteria[name=\"OIHist\"]/OI" + std::to_string(i);
			parameters_.rolloutEligibility.oiHistOIs[i] =
				eligibility_node.select_single_node(path.c_str()).node().text().as_int() != 0;
			path = "criteria[name=\"CD4OIHist\"]/OI" + std::to_string(i);
			parameters_.rolloutEligibility.cd4OiHistOIs[i] =
				eligibility_node.select_single_node(path.c_str()).node().text().as_int() != 0;
		}

		parameters_.cepacTracer = new Tracer(parameters_.simName, parameters_.rolloutSimContexts[0]->rolloutSimContext, 1);
		parameters_.cepacRunStats = new RunStats(parameters_.simName, parameters_.rolloutSimContexts[0]->rolloutSimContext);
	}
	else
	{
		parameters_.cepacTracer = new Tracer(parameters_.simName, parameters_.cepacSimContexts[0], 1);
		parameters_.cepacRunStats = new RunStats(parameters_.simName, parameters_.cepacSimContexts[0]);
	}

	parameters_.numToTrace = 
		simulation_node.select_single_node("/simulation/numberToTracePerAgeRange").node().text().as_int();
	parameters_.numNewbornsToTrace = 
		simulation_node.select_single_node("/simulation/numberNewbornsToTrace").node().text().as_int();
	parameters_.monthTraceNewborns = 
		simulation_node.select_single_node("/simulation/monthTraceNewborns").node().text().as_int();
	parameters_.tracePrevalentCases = 
		simulation_node.select_single_node("/simulation/tracePrevalentCases").node().text().as_int() != 0;

	CepacUtil::setRandomSeedType(fixedSeed_ == -1);

	if(fixedSeed_ > -1)
	{
		//Seed is Minnesota Twins retired numbers... yes, I am a dork
		parameters_.randomNums.reset(fixedSeed_ == 0 ? 36291434 : fixedSeed_);
	}

	population_.Deserialize(simulation_node.child("population"));
}

void Simulation::Serialize(pugi::xml_node &document)
{

}

Simulation::Simulation(const std::string &run_name)
    : failedCalibration_(false),
	  hasPassedFirstMonthCalibPrev_(false),
	  monthOfFirstMonthCalibPrev_(0),
	  name_(run_name),
	  time_(0),
	  parameters_(),
	  population_(parameters_)
{
	parameters_.simName = run_name;
}

Simulation::~Simulation()
{
}

void Simulation::FirstStep()
{
	//No longer creating a CEPAC trace file, but we still need to change over to the results folder before creating any other output files
	CepacUtil::changeDirectoryToResults();

	//initialize the trace files for population and events and infections and costs
	for(int i = 0; i < Constants::NUMBER_OF_TRACE_FILES; i++)
	{
		if(parameters_.outputTrace[i])
		{
			std::string fileName = parameters_.simName;
			fileName.append("-" + parameters_.traceExtensions[i]);
			parameters_.traceStreams[i].open(fileName.c_str(), ios::out);
		}
	}

	if(parameters_.calibrationInputs.useCalibration)
	{
		for(int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
		{
			std::string fileName = parameters_.simName;
			fileName.append("-popState" + boost::lexical_cast<std::string>(i)+".pop");
			parameters_.popStateStream[i].open(fileName.c_str(), ios::out);
		}
	}

	BatchStatsVariables batchstat;

	for(batchstat = BatchStatsVariables(0); batchstat < ENDBatchStatsVariables;
		batchstat = BatchStatsVariables(batchstat + 1))
	{
		parameters_.BatchStatsStream[batchstat].open(("batchstats-" + Constants::BatchStatFileName[batchstat] + ".out").c_str(),
			ios::out | ios::app);
	}

	//output seed used for this run
	if(parameters_.outputTrace[EventParams::TraceFileType::Events])
	{
		parameters_.traceStreams[EventParams::TraceFileType::Events] << "Seed = " << parameters_.randomNums.getSeed() << std::endl;
		parameters_.traceStreams[EventParams::TraceFileType::Events] << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
			Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << "Sexually Active Population"
			<< Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
			Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
			Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
			"Non Sexually Active Population" << std::endl;
	}

	if(parameters_.monthOf1990 > 0 && parameters_.outputTrace[EventParams::TraceFileType::ShiftedOutcomes])
	{
		population_.popStats.enableShiftedOutcomes(parameters_.monthOf1990);
	}

	//initialize/reset monthly stats
	population_.resetMonthlyStats();
	//initialize incident infections by age
	population_.initIncidentInfectionsByAge();

	if(parameters_.delayPrevalence == 0)
	{
		population_.applyIncidentPrevalence(parameters_);
	}

	//print out prevalent infection stats & headers for rest of infection stats
	population_.calcPrevalentPopulation(0);

	//Print out run name for first column of BatchStats files (if streams are open)
	for(int i = 0; i < ENDBatchStatsVariables; i++)
	{
		if(parameters_.BatchStatsStream[i].is_open())
		{
			parameters_.BatchStatsStream[i] << parameters_.simName << Constants::TAB;
		}
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::Infection])
	{
		population_.popStats.infectionsTracker.printInfections(parameters_, time_, parameters_.traceStreams[EventParams::TraceFileType::Infection], &population_);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::Partnership])
	{
		population_.printPartnerships(parameters_, time_, parameters_.traceStreams[EventParams::TraceFileType::Partnership]);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::Clinical])
	{
		population_.printClinical(parameters_, time_, parameters_.traceStreams[EventParams::TraceFileType::Clinical]);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::Population])
	{
		population_.printPopulation(parameters_, time_, parameters_.traceStreams[EventParams::TraceFileType::Population]);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::ArtRollout])
	{
		population_.printARTRolloutOutcomes(parameters_, parameters_.traceStreams[EventParams::TraceFileType::ArtRollout]);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::CostEffectiveness])
	{
		population_.popStats.costsTracker.PrintCosts(parameters_.currTime, parameters_.traceStreams[EventParams::TraceFileType::CostEffectiveness]);
	}

	if(parameters_.debugLevel == DEBUG1)
	{
		population_.printMethodResults(parameters_, "--", "initialization", 0, "--", Constants::SHOW_INFECTED);
	}
}

void Simulation::Step()
{
	time_++;

	double begin = timer_.GetTime();

	if(parameters_.useRollout)
	{
		population_.applyRolloutContext(parameters_, time_);
	}

	long totalSize = SimulateMonth();

	//print out new infection stats
	population_.calcPrevalentPopulation(time_);

	if(parameters_.outputTrace[EventParams::TraceFileType::Infection])
	{
		prevalence_ = population_.popStats.infectionsTracker.printInfections(parameters_, time_, parameters_.traceStreams[EventParams::TraceFileType::Infection], &population_);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::Population])
	{
		population_.printPopulation(parameters_, time_, parameters_.traceStreams[EventParams::TraceFileType::Population]);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::Partnership])
	{
		population_.printPartnerships(parameters_, time_, parameters_.traceStreams[EventParams::TraceFileType::Partnership]);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::Clinical])
	{
		population_.printClinical(parameters_, time_, parameters_.traceStreams[EventParams::TraceFileType::Clinical]);
	}

	//For now, this must come after infectionsTracker.printInfections as it is what calculate prevalence
	if(parameters_.outputTrace[EventParams::TraceFileType::ShiftedOutcomes])
	{
		population_.recordShiftedOutcomes(parameters_, parameters_.traceStreams[EventParams::TraceFileType::ShiftedOutcomes]);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::ArtRollout])
	{
		population_.printARTRolloutOutcomes(parameters_, parameters_.traceStreams[EventParams::TraceFileType::ArtRollout]);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::CostEffectiveness])
	{
		population_.popStats.costsTracker.PrintCosts(parameters_.currTime, parameters_.traceStreams[EventParams::TraceFileType::CostEffectiveness]);
	}

	if(parameters_.calibrationInputs.useCalibration && parameters_.calibrationInputs.monthOfCalibration == time_)
	{
		//If this run doesn't pass the partnership calibration stop the run and discard specified trace files
		if(!population_.passesPartnershipCalibration(parameters_))
		{
			return;
		}
	}

	if(parameters_.calibrationInputs.useCalibration)
	{
		if(!hasPassedFirstMonthCalibPrev_)
		{
			double SAPrev = population_.popStats.infectionsTracker.getSAPrev(population_);

			if(SAPrev != -1 && SAPrev >= parameters_.calibrationInputs.thresholdPrevMult * parameters_.calibrationInputs.calendarPrevs[0])
			{
				hasPassedFirstMonthCalibPrev_ = true;
				monthOfFirstMonthCalibPrev_ = parameters_.currTime;
			}
		}

		for(int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
		{
			if(hasPassedFirstMonthCalibPrev_ && parameters_.currTime == monthOfFirstMonthCalibPrev_ + parameters_.calibrationInputs.saveStateTimePoints[i])
			{
				population_.saveState(parameters_.popStateStream[i], parameters_.currTime);
			}
		}
	}

	population_.resetMonthlyStats();

	double end = timer_.GetTime();
	std::ostringstream elapsedStringStream;
	elapsedStringStream.precision(3);
	elapsedStringStream << std::fixed << (end - begin);
	std::string elapsedString = elapsedStringStream.str();

	parameters_.displayOut("Timestep(");
	std::string timeString = boost::lexical_cast<std::string>(time_);
	parameters_.displayOut(timeString.c_str());
	parameters_.displayOut("): compute time elapsed = ");
	parameters_.displayOut(elapsedString.c_str());
	parameters_.displayOut(". size = ");
	parameters_.displayOut(boost::lexical_cast<std::string>(totalSize).c_str());
	parameters_.displayOut("\n");
}

void Simulation::LastStep()
{
	//print survival statistics
	if(parameters_.outputTrace[EventParams::TraceFileType::Survival])
	{
		population_.popStats.printSurvivalStats(parameters_.traceStreams[EventParams::TraceFileType::Survival]);
	}

	//Run every infected person left through CEPAC until they die
	if(failedCalibration_)
	{
		parameters_.displayOut("Partnership Calibration Failed...Deleting specified trace files...\n");
	}
	else
	{
		parameters_.displayOut("Running all remaining persons through CEPAC until they die...\n");
		population_.updateFinalPhysicalState(parameters_);
		parameters_.displayOut("Done!\n");
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::Infection])
	{
		population_.popStats.printLMStats(parameters_.traceStreams[EventParams::TraceFileType::Infection]);
	}

	//finalize and print CEPAC output, but only if at least one patient went through CEPAC
	try
	{
		if(parameters_.cepacRunStats->getPopulationSummary()->numCohorts > 0)
		{
			parameters_.cepacRunStats->finalizeStats();
			parameters_.cepacRunStats->writeStatsFile();
		}
	}
	catch(std::string errorString)
	{
		parameters_.displayOut(errorString);
	}

	//if failed partnership calibration toss unneeded files
	if(failedCalibration_)
	{
		for(int i = 0; i < Constants::NUMBER_OF_TRACE_FILES; i++)
		{
			if(parameters_.calibrationInputs.tossFiles[i])
			{
				parameters_.traceStreams[i].close();
				std::string fileName = parameters_.simName;
				fileName.append("-" + parameters_.traceExtensions[i]);
				remove(fileName.c_str());
			}
		}

		for(int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
		{
			std::string fileName = parameters_.simName;
			fileName.append("-popState" + boost::lexical_cast<std::string>(i)+".pop");
			remove(fileName.c_str());
		}
	}
}

/*
* This function records all of the SimContext files to be used by the CEPAC disease model throughout the run of the transmission model
*/

bool Simulation::LoadCepacSimContexts(const CepacTreatmentFiles &treatment_files)
{
	parameters_.displayOut("CEPAC Files: \n");

	for(const auto &treatment_file : treatment_files)
	{
		parameters_.displayOut("\t" + std::to_string(treatment_file.file_number) + ": " 
			+ treatment_file.file_name + "\n");

		//Set the CEPAC simContext from the specified CEPAC .in file
		auto stem = boost::filesystem::path(treatment_file.file_name).stem().string();
		parameters_.cepacSimContexts.push_back(new SimContext(stem));
		assert(parameters_.cepacSimContexts.size() == static_cast<size_t>(treatment_file.file_number) + 1);
		//parameters_.cepacSimContext = new SimContext(cepacInputFile.substr(0, cepacInputFile.find(CepacUtil::FILE_EXTENSION_FOR_INPUT)));
		parameters_.cepacSimContexts.at(treatment_file.file_number)->numPatientsToTrace = 0;

		//Read in the inputs
		try
		{
			parameters_.cepacSimContexts.back()->readInputs();
		}
		catch(std::string errorString)
		{
			//if we can't find it and we wanted to use CEPAC, display error
			parameters_.displayOut("*****************************************\n");
			parameters_.displayOut("WARNING!\n");
			parameters_.displayOut("*****************************************\n");
			parameters_.displayOut("File '" + treatment_file.file_name + "' generates error:\n");
			parameters_.displayOut("\t" + errorString + "\n");
			return false;
		}

		//From the first file only, get the death tables for non-AIDS death
		if(treatment_file.file_number == 0)
		{
			CepacInputParser cepacInput(treatment_file.file_name);
			auto probabilities = cepacInput.parseNonAidsDeathProbabilities();
			Person::probDeathNatCauses[DmgProfile::MALE] = probabilities[0];
			Person::probDeathNatCauses[DmgProfile::FEMALE] = probabilities[1];
		}
	}

	for(int i = 0; i < Constants::NUMBER_OF_CEPAC_FILES; i++)
	{
		//By default, the first "time to switch" should be 0 (i.e. the first CEPAC .in file applies at time 0)
		parameters_.timesToSwitchSimContext[i] = 
			i == 0 ? 0 : cepac_treatment_files_[i].time;
	}

	return true;
}

/***
Sets the Non aids death from a cepac simcontext
***/
void Simulation::SetNonAidsDeathFromCepac(SimContext &cepacSimContext, std::vector<double> &male, std::vector<double> &female)
{
	male.clear();
	female.clear();

	for(int i = 0; i <= SimContext::AGE_YRS; i++)
	{
		male.push_back(cepacSimContext.getNatHistInputs()->monthlyNonAIDSDeathProb[SimContext::GENDER_MALE][i]);
		female.push_back(cepacSimContext.getNatHistInputs()->monthlyNonAIDSDeathProb[SimContext::GENDER_FEMALE][i]);
	}
}

BetaDist ParseBeta(const std::string &distributionString)
{
	auto commaIndex = distributionString.find(',');
	auto type = distributionString.substr(0, commaIndex);
	auto secondCommaIndex = distributionString.find(',', commaIndex + 1);

	BetaDist dist;

	if(type == "normal")
	{
		auto mean = std::stod(distributionString.substr(commaIndex + 1, secondCommaIndex - commaIndex));
		auto stdDev = std::stod(distributionString.substr(secondCommaIndex + 1));
		auto sampleSize = mean * (1 - mean) / (stdDev * stdDev) - 1;

		dist.alpha = mean * sampleSize;
		dist.beta = (1 - mean) * sampleSize;
	}
	else if(type == "beta")
	{
		dist.alpha = std::stod(distributionString.substr(commaIndex + 1, secondCommaIndex - commaIndex));
		dist.beta = std::stod(distributionString.substr(secondCommaIndex + 1));
	}
	else
	{
		throw std::runtime_error("unknown distribution");
	}

	return dist;
}

void Simulation::UpdateTimeDependentParameters()
{
	bool simulation_changed = false;
	bool population_changed = false;

	for(const auto &type_parameter_pair : time_dependent_parameters_)
	{
		const auto &parameter = type_parameter_pair.second;

		if(parameter.time == parameters_.currTime)
		{
			if(parameter.target_population.has_value)
			{
				population_.Apply(parameter.target_population.value, parameter.population_modifier);
				population_changed = true;
			}
			else
			{
				parameter.simulation_modifier(*this);
				simulation_changed = true;
			}
		}
	}

	if(simulation_changed)
	{
		ValidateState();
	}

	if(population_changed)
	{
		population_.ValidateState();
	}
}

void Simulation::ValidateState()
{
	for(int i = 1; i <= 5; i++)
	{
		int matching = int(parameters_.rolloutEligibility.oiHistRank == i)
			+ int(parameters_.rolloutEligibility.cd4Rank == i)
			+ int(parameters_.rolloutEligibility.cd4OiHistRank == i)
			+ int(parameters_.rolloutEligibility.hvlRank == i)
			+ int(parameters_.rolloutEligibility.cd4HvlRank == i);
		if(matching != 1)
		{
			throw std::runtime_error("need a single elegibility criterion for each rank 1..5: " + std::to_string(matching));
		}
	}
}

/*
			// OIHist
			switch(type_parameter_pair.first)
			{
			case TemplateParameter::AgeSexualDebutYears:
				population_.popWideParams.setAgeSexualDebut(std::stod(parameter.value));
				break;
			case TemplateParameter::BirthRate:
				population_.popWideParams.setBirthRate(std::stod(parameter.value));
				break;
			case TemplateParameter::ProportionCircumcised:
				population_.popWideParams.setProportionCircumcised(std::stod(parameter.value));
				break;
			case TemplateParameter::ProportionMale:
				population_.popWideParams.setProportionMale(std::stod(parameter.value));
				break;
			case TemplateParameter::OIHistRank:
				parameters_.rolloutEligibility.oiHistRank = std::stoi(parameter.value);
				checkRanks = true;
				break;
			case TemplateParameter::OIHistOI0:
				parameters_.rolloutEligibility.oiHistOIs[0] = parameter.value != "0";
				break;
			case TemplateParameter::OIHistOI1:
				parameters_.rolloutEligibility.oiHistOIs[1] = parameter.value != "0";
				break;
			case TemplateParameter::OIHistOI2:
				parameters_.rolloutEligibility.oiHistOIs[2] = parameter.value != "0";
				break;
			case TemplateParameter::OIHistOI3:
				parameters_.rolloutEligibility.oiHistOIs[3] = parameter.value != "0";
				break;
			case TemplateParameter::OIHistOI4:
				parameters_.rolloutEligibility.oiHistOIs[4] = parameter.value != "0";
				break;
			case TemplateParameter::OIHistOI5:
				parameters_.rolloutEligibility.oiHistOIs[5] = parameter.value != "0";
				break;
			case TemplateParameter::OIHistOI6:
				parameters_.rolloutEligibility.oiHistOIs[6] = parameter.value != "0";
				break;
			case TemplateParameter::OIHistOI7:
				parameters_.rolloutEligibility.oiHistOIs[7] = parameter.value != "0";
				break;
			case TemplateParameter::OIHistOI8:
				parameters_.rolloutEligibility.oiHistOIs[8] = parameter.value != "0";
				break;
			case TemplateParameter::OIHistOI9:
				parameters_.rolloutEligibility.oiHistOIs[9] = parameter.value != "0";
				break;
			case TemplateParameter::OIHistOI10:
				parameters_.rolloutEligibility.oiHistOIs[10] = parameter.value != "0";
				break;
			case TemplateParameter::OIHistOI11:
				parameters_.rolloutEligibility.oiHistOIs[11] = parameter.value != "0";
				break;
			case TemplateParameter::OIHistOI12:
				parameters_.rolloutEligibility.oiHistOIs[12] = parameter.value != "0";
				break;
			case TemplateParameter::OIHistOI13:
				parameters_.rolloutEligibility.oiHistOIs[13] = parameter.value != "0";
				break;
			case TemplateParameter::OIHistOI14:
				parameters_.rolloutEligibility.oiHistOIs[14] = parameter.value != "0";
				break; 
			case TemplateParameter::OIHistNumOIToStart:
				parameters_.rolloutEligibility.oiHistNumToStart = std::stoi(parameter.value);
				break;
			case TemplateParameter::CD4Rank:
				parameters_.rolloutEligibility.cd4Rank = std::stoi(parameter.value);
				checkRanks = true;
				break;
			case TemplateParameter::CD4CD4Upp:
				parameters_.rolloutEligibility.cd4Bounds[1] = std::stoi(parameter.value);
				break;
			case TemplateParameter::CD4CD4Lwr:
				parameters_.rolloutEligibility.cd4Bounds[0] = std::stoi(parameter.value);
				break;
			case TemplateParameter::CD4OIHistRank:
				parameters_.rolloutEligibility.cd4OiHistRank = std::stoi(parameter.value);
				checkRanks = true;
				break;
			case TemplateParameter::CD4OIHistOI0:
				parameters_.rolloutEligibility.cd4OiHistOIs[0] = parameter.value != "0";
				break;
			case TemplateParameter::CD4OIHistOI1:
				parameters_.rolloutEligibility.cd4OiHistOIs[1] = parameter.value != "0";
				break;
			case TemplateParameter::CD4OIHistOI2:
				parameters_.rolloutEligibility.cd4OiHistOIs[2] = parameter.value != "0";
				break;
			case TemplateParameter::CD4OIHistOI3:
				parameters_.rolloutEligibility.cd4OiHistOIs[3] = parameter.value != "0";
				break;
			case TemplateParameter::CD4OIHistOI4:
				parameters_.rolloutEligibility.cd4OiHistOIs[4] = parameter.value != "0";
				break;
			case TemplateParameter::CD4OIHistOI5:
				parameters_.rolloutEligibility.cd4OiHistOIs[5] = parameter.value != "0";
				break;
			case TemplateParameter::CD4OIHistOI6:
				parameters_.rolloutEligibility.cd4OiHistOIs[6] = parameter.value != "0";
				break;
			case TemplateParameter::CD4OIHistOI7:
				parameters_.rolloutEligibility.cd4OiHistOIs[7] = parameter.value != "0";
				break;
			case TemplateParameter::CD4OIHistOI8:
				parameters_.rolloutEligibility.cd4OiHistOIs[8] = parameter.value != "0";
				break;
			case TemplateParameter::CD4OIHistOI9:
				parameters_.rolloutEligibility.cd4OiHistOIs[9] = parameter.value != "0";
				break;
			case TemplateParameter::CD4OIHistOI10:
				parameters_.rolloutEligibility.cd4OiHistOIs[10] = parameter.value != "0";
				break;
			case TemplateParameter::CD4OIHistOI11:
				parameters_.rolloutEligibility.cd4OiHistOIs[11] = parameter.value != "0";
				break;
			case TemplateParameter::CD4OIHistOI12:
				parameters_.rolloutEligibility.cd4OiHistOIs[12] = parameter.value != "0";
				break;
			case TemplateParameter::CD4OIHistOI13:
				parameters_.rolloutEligibility.cd4OiHistOIs[13] = parameter.value != "0";
				break;
			case TemplateParameter::CD4OIHistOI14:
				parameters_.rolloutEligibility.cd4OiHistOIs[14] = parameter.value != "0";
				break;
			case TemplateParameter::CD4OIHistCD4Upp:
				parameters_.rolloutEligibility.cd4OiHistCd4Bounds[1] = std::stoi(parameter.value);
				break;
			case TemplateParameter::CD4OIHistCD4Lwr:
				parameters_.rolloutEligibility.cd4OiHistCd4Bounds[0] = std::stoi(parameter.value);
				break;
			case TemplateParameter::HVLRank:
				parameters_.rolloutEligibility.hvlRank = std::stoi(parameter.value);
				checkRanks = true;
				break;
			case TemplateParameter::HVLHVLUpp:
				parameters_.rolloutEligibility.hvlBounds[1] = std::stoi(parameter.value);
				break;
			case TemplateParameter::HVLHVLLwr:
				parameters_.rolloutEligibility.hvlBounds[0] = std::stoi(parameter.value);
				break;
			case TemplateParameter::CD4HVLRank:
				parameters_.rolloutEligibility.cd4HvlRank = std::stoi(parameter.value);
				checkRanks = true;
				break;
			case TemplateParameter::CD4HVLCD4Upp:
				parameters_.rolloutEligibility.cd4HvlCd4Bounds[1] = std::stoi(parameter.value);
				break;
			case TemplateParameter::CD4HVLCD4Lwr:
				parameters_.rolloutEligibility.cd4HvlCd4Bounds[0] = std::stoi(parameter.value);
				break;
			case TemplateParameter::CD4HVLHVLUpp:
				parameters_.rolloutEligibility.cd4HvlHvlBounds[1] = std::stoi(parameter.value);
				break;
			case TemplateParameter::CD4HVLHVLLwr:
				parameters_.rolloutEligibility.cd4HvlHvlBounds[0] = std::stoi(parameter.value);
				break;
			case TemplateParameter::SteadyChanceCondomUsePerEventHighRisk:
			{
				auto beta = ParseBeta(parameter.value);
				auto modifier = [&](Person *p) { p->SetChanceCondomUsePerEvent(Person::HIGH, SexualPartnership::Type::Steady, beta); };
				if(parameter.target_population.has_value)
				{
					population_.Apply(parameter.target_population.value, modifier);
				}
				else
				{
					population_.Apply(modifier);
				}
				break;
			}
			case TemplateParameter::SteadyChanceCondomUsePerEventLowRisk:
			{
				auto beta = ParseBeta(parameter.value);
				auto modifier = [&](Person *p) { p->SetChanceCondomUsePerEvent(Person::LOW, SexualPartnership::Type::Steady, beta); };
				if(parameter.target_population.has_value)
				{
					population_.Apply(parameter.target_population.value, modifier);
				}
				else
				{
					population_.Apply(modifier);
				}
				break;
			}
			case TemplateParameter::RegularChanceCondomUsePerEventHighRisk:
			{
				auto beta = ParseBeta(parameter.value);
				auto modifier = [&](Person *p) { p->SetChanceCondomUsePerEvent(Person::HIGH, SexualPartnership::Type::Regular, beta); };
				if(parameter.target_population.has_value)
				{
					population_.Apply(parameter.target_population.value, modifier);
				}
				else
				{
					population_.Apply(modifier);
				}
				break;
			}
			case TemplateParameter::RegularChanceCondomUsePerEventLowRisk:
			{
				auto beta = ParseBeta(parameter.value);
				auto modifier = [&](Person *p) { p->SetChanceCondomUsePerEvent(Person::LOW, SexualPartnership::Type::Regular, beta); };
				if(parameter.target_population.has_value)
				{
					population_.Apply(parameter.target_population.value, modifier);
				}
				else
				{
					population_.Apply(modifier);
				}
				break;
			}
			case TemplateParameter::CasualChanceCondomUsePerEventHighRisk:
			{
				auto beta = ParseBeta(parameter.value);
				auto modifier = [&](Person *p) { p->SetChanceCondomUsePerEvent(Person::HIGH, SexualPartnership::Type::Casual, beta); };
				if(parameter.target_population.has_value)
				{
					population_.Apply(parameter.target_population.value, modifier);
				}
				else
				{
					population_.Apply(modifier);
				}
				break;
			}
			case TemplateParameter::CasualChanceCondomUsePerEventLowRisk:
			{
				auto beta = ParseBeta(parameter.value);
				auto modifier = [&](Person *p) { p->SetChanceCondomUsePerEvent(Person::LOW, SexualPartnership::Type::Casual, beta); };
				if(parameter.target_population.has_value)
				{
					population_.Apply(parameter.target_population.value, modifier);
				}
				else
				{
					population_.Apply(modifier);
				}
				break;
			}
			case TemplateParameter::CswChanceCondomUsePerEventHighRisk:
			{
				auto beta = ParseBeta(parameter.value);
				auto modifier = [&](Person *p) { p->SetChanceCondomUsePerEvent(Person::HIGH, SexualPartnership::Type::Csw, beta); };
				if(parameter.target_population.has_value)
				{
					population_.Apply(parameter.target_population.value, modifier);
				}
				else
				{
					population_.Apply(modifier);
				}
				break;
			}
			case TemplateParameter::CswChanceCondomUsePerEventLowRisk:
			{
				auto beta = ParseBeta(parameter.value);
				auto modifier = [&](Person *p) { p->SetChanceCondomUsePerEvent(Person::LOW, SexualPartnership::Type::Csw, beta); };
				if(parameter.target_population.has_value)
				{
					population_.Apply(parameter.target_population.value, modifier);
				}
				else
				{
					population_.Apply(modifier);
				}
				break;
			}
			default:
				throw std::runtime_error("not implemented");
			}
		}
	}
}
*/

/***
This function executes one timestep of the simulation
The ordering of events within this function determines the ordering of events in each timestep
****/
int Simulation::SimulateMonth()
{
	parameters_.currTime = time_;

	UpdateTimeDependentParameters();

	//change non AIDS death if it is time to switch cepac files
	if(parameters_.itIsTimeToSwitchSimContext() && !parameters_.useRollout)
	{
		int simIndex = 0;

		for(int i = 0; i < Constants::NUMBER_OF_CEPAC_FILES; i++)
		{
			if(parameters_.currTime > parameters_.timesToSwitchSimContext[i])
			{
				simIndex = i;
			}
		}

		SetNonAidsDeathFromCepac(*parameters_.cepacSimContexts[simIndex], Person::probDeathNatCauses[DmgProfile::MALE], Person::probDeathNatCauses[DmgProfile::FEMALE]);
	}

	//output the current timestep of the simulation
	if(parameters_.debugLevel > DEBUG1 && parameters_.outputTrace[EventParams::TraceFileType::Events])
	{
		parameters_.traceStreams[EventParams::TraceFileType::Events] << "T:" << time_ << " : Start of Timestep" << std::endl;
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::Singleperson])
	{
		parameters_.traceStreams[EventParams::TraceFileType::Singleperson] << std::endl << "** Time " << time_ << ": " << std::endl;
	}

	bool recordLE = false;
	bool recordPartAcq  =  false;
	bool firstMonthToRecord = false;
	bool lastMonthToRecord = false;

	if(population_.popStats.isTimeToRecordLE(time_))
	{
		recordLE = true;
	}

	if(population_.popStats.isTimeToRecordPartAcq(time_))
	{
		recordPartAcq = true;
	}

	if(population_.popStats.isFirstMonthToRecordLE(time_))
	{
		firstMonthToRecord = true;
	}

	if(population_.popStats.isTimeToPrintLE(time_))
	{
		lastMonthToRecord = true;
	}

	population_.updatePhysicalState(parameters_, recordLE, firstMonthToRecord);

	if(parameters_.useRollout)
	{
		population_.applyARTRollout(parameters_);
	}

	population_.births(parameters_);

	if(lastMonthToRecord)
	{
		population_.updateAgeBucketsLE();

		if(parameters_.outputTrace[EventParams::TraceFileType::LifeExpectancy])
		{
			population_.popStats.printLEStats(parameters_.traceStreams[EventParams::TraceFileType::LifeExpectancy], time_);
		}

		delete population_.popStats.selectedLEStats;
		population_.popStats.selectedLEStats = nullptr;
	}

	//steadyCouple, flings, and dissolveSexualPartnerships
	population_.updatePartnerships(parameters_);

	if(recordPartAcq)
	{
		population_.popStats.selectedPartAcqStats = new PopStats::SinglePartAcqStats();
		population_.recordPartAcqFreq();

		if(parameters_.outputTrace[EventParams::TraceFileType::PartnershipAcquisition])
		{
			population_.popStats.printPartAcqStats(parameters_.traceStreams[EventParams::TraceFileType::PartnershipAcquisition], time_);
		}

		delete population_.popStats.selectedPartAcqStats;
		population_.popStats.selectedPartAcqStats = nullptr;
	}

	//apply incident prevalence
	if(parameters_.delayPrevalence != 0 && parameters_.delayPrevalence == time_)
	{
		population_.applyIncidentPrevalence(parameters_);
	}

	//Will confirm that population_.currSize is correct and update size of age ranges
	return population_.updateSize();
}

RunStats &Simulation::GetCEPACRunStats()
{
	return *parameters_.cepacRunStats;
}

PopStats &Simulation::GetPopStats()
{
	return population_.popStats;
}

EventParams &Simulation::GetEventParams()
{
	return parameters_;
}

Outputs Simulation::Run(MessageCallback message_callback)
{
	MessageCallback old = parameters_.messageCallback;
	parameters_.messageCallback = message_callback;

	if(time_ == 0)
	{
		FirstStep();
	}

	while(time_ < duration_)
	{
		Step();
	}

	LastStep();

	parameters_.messageCallback = old;

	return outputs_;
}