#include <iostream>
#include <set>
#include <boost/filesystem.hpp>

#include "Simulation.h"
#include "Constants.h"
#include "Inputs.h"
#include "Population.h"
#include "cepac/include.h"
#include "cepacbridge/CepacInputParser.h"
#include "data/EventParams.h"
#include "entities/classifiers/DmgProfile.h"
#include "entities/behaviors/SexualBehaviorParams.h"
#include "graphviz/graphVizParse.h"
#include "util/HighResolutionTimer.h"
#include "util/Util.h"

Simulation::Simulation(const Inputs &inputs, MessageCallback message_callback)
    : failedCalibration_(false),
	  hasPassedFirstMonthCalibPrev_(false),
	  monthOfFirstMonthCalibPrev_(0),
	  population_(nullptr),
	  inputs_(inputs)
{
	parameters_.simName = inputs_.GetRunName();
	parameters_.displayOut("Sim name is " + parameters_.simName + "\n");

	parameters_.displayOut("Simulation Parameters\n");

	int fixedSeed = inputs_.GetFixedSeed();
	parameters_.monthOf1990 = inputs_.GetMonthOf1990();
	parameters_.displayOut("Input Version =" + Version::ToString(inputs_.GetVersion()) + "\n");

	if(inputs_.GetVersion().major != Util::MODEL_VERSION.major
		|| inputs_.GetVersion().minor != Util::MODEL_VERSION.minor)
	{
		parameters_.displayOut("Input version for " + inputs_.GetFilename() + " is not "
			+ Version::ToString(Util::MODEL_VERSION) + ".  Stopping model execution!\n");

		throw std::runtime_error("bad input version");
	}

	//save Debug Level
	parameters_.debugLevel = DebugLevel(inputs_.GetDebugLevel());
	parameters_.displayOut("\tDebug Level = " + std::to_string(inputs_.GetDebugLevel()) + "\n");

	//save Concurrency Definitions
	int i = 0;
	for(auto &definition : inputs_.GetConcurrencyDefinitions())
	{
		parameters_.concurrencyDef[i++] = new EventParams::ConcurrencyDef(definition.minimum_needed, definition.allow);
	}

	for(auto &trace_file : inputs_.GetTraceFiles())
	{
		parameters_.outputTrace[(int)trace_file.first] = trace_file.second.enabled;
		parameters_.traceExtensions[(int)trace_file.first] = trace_file.second.extension;
		parameters_.calibrationInputs.tossFiles[(int)trace_file.first] = trace_file.second.toss;
	}

	//save calibration inputs
	parameters_.calibrationInputs.useCalibration = inputs_.GetCalibrationSettings().enabled;

	if(parameters_.calibrationInputs.useCalibration)
	{
		parameters_.calibrationInputs.monthOfCalibration = inputs_.GetCalibrationSettings().month;
		parameters_.calibrationInputs.steadyPrevPopulation = inputs_.GetCalibrationSettings().steady_target.target;
		parameters_.calibrationInputs.steadyPrevBounds[Constants::LOWER] = inputs_.GetCalibrationSettings().steady_target.lower;
		parameters_.calibrationInputs.steadyPrevBounds[Constants::UPPER] = inputs_.GetCalibrationSettings().steady_target.upper;
		parameters_.calibrationInputs.casualPrevPopulation = inputs_.GetCalibrationSettings().casual_target.target;
		parameters_.calibrationInputs.casualPrevBounds[Constants::LOWER] = inputs_.GetCalibrationSettings().casual_target.lower;
		parameters_.calibrationInputs.casualPrevBounds[Constants::UPPER] = inputs_.GetCalibrationSettings().casual_target.upper;
		parameters_.calibrationInputs.CSWPrevPopulation = inputs_.GetCalibrationSettings().csw_target.target;
		parameters_.calibrationInputs.CSWPrevBounds[Constants::LOWER] = inputs_.GetCalibrationSettings().csw_target.lower;
		parameters_.calibrationInputs.CSWPrevBounds[Constants::UPPER] = inputs_.GetCalibrationSettings().csw_target.upper;
		parameters_.calibrationInputs.propInConcurrentPopulation = inputs_.GetCalibrationSettings().proportion_concurrent_target;
		parameters_.calibrationInputs.propInConcurrentBounds[Constants::LOWER] = inputs_.GetCalibrationSettings().proportion_concurrent_target.lower;
		parameters_.calibrationInputs.propInConcurrentBounds[Constants::UPPER] = inputs_.GetCalibrationSettings().proportion_concurrent_target.upper;
		parameters_.calibrationInputs.numActsPopulation = inputs_.GetCalibrationSettings().num_acts_target.target;
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

	duration_ = inputs_.GetDuration();
	parameters_.displayOut("\tTime steps = " + std::to_string(duration_) + "\n");

	parameters_.delayPrevalence = inputs_.GetPopulationSettings().prevalence_delay;
	parameters_.displayOut("\tDelay Prevalence = " + std::to_string(prevalenceDelay_) + "\n");

	bool filesLoaded = false;

	//Load the CEPAC files
	if(inputs_.GetInterventions().use_art_rollout)
	{
		//use art rollout input files
		parameters_.useRollout = true;
		filesLoaded = LoadRolloutSimContexts(inputs_.GetInterventions().rollout_treatment_files);

		int proportionYear = 2002;
		ticpp::Iterator<ticpp::Element> proportionIterator;

		for(proportionIterator = proportionIterator.begin(simParams->FirstChildElement("population")->FirstChildElement("interventions")->FirstChildElement("artRolloutIntervention")->FirstChildElement("targetRolloutProportions")); proportionIterator != proportionIterator.end(); ++proportionIterator)
		{
			int year = boost::lexical_cast<int>(proportionIterator.Get()->GetAttribute("year"));
			assert(year == proportionYear++);
			parameters_.targetYearlyRolloutProportions.push_back(proportionIterator.Get()->GetText<double>());
		}
	}
	else
	{
		//use standard cepac input files
		parameters_.useRollout = false;
		filesLoaded = SetCEPACSimContexts(simParams->FirstChildElement("population")->FirstChildElement("interventions")->FirstChildElement("cepacIntervention"));
	}

	if(!filesLoaded)
	{
		//If the files didn't successfully load, don't run the model!
		parameters_.displayOut("FILE ERROR: Stopping model execution for ");
		parameters_.displayOut(xmlFile.c_str());
		parameters_.displayOut("\n");

		throw std::runtime_error("error loading files");
	}

	//Set up CEPAC output (runStats)
	CepacUtil::setRandomSeedType(fixedSeed == -1);

	if(fixedSeed > -1)
	{
		//Seed is Minnesota Twins retired numbers... yes, I am a dork
		parameters_.randomNums.reset(fixedSeed == 0 ? 36291434 : fixedSeed);
	}

	if(parameters_.useRollout)
	{
		parameters_.cepacRunStats = new RunStats(parameters_.simName, parameters_.rolloutSimContexts[0]->rolloutSimContext);
	}
	else
	{
		parameters_.cepacRunStats = new RunStats(parameters_.simName, parameters_.cepacSimContexts[0]);
	}

	//Setup up CEPAC traces (these will probably be unreadable, but oh well)
	if(parameters_.useRollout)
	{
		parameters_.cepacTracer = new Tracer(parameters_.simName, parameters_.rolloutSimContexts[0]->rolloutSimContext, 1);
	}
	else
	{
		parameters_.cepacTracer = new Tracer(parameters_.simName, parameters_.cepacSimContexts[0], 1);
	}

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

	parameters_.numToTrace = simParams->FirstChildElement("numberToTracePerAgeRange")->GetText<int>();
	parameters_.numNewbornsToTrace = simParams->FirstChildElement("numberNewbornsToTrace")->GetText<int>();
	parameters_.monthTraceNewborns = simParams->FirstChildElement("monthTraceNewborns")->GetText<int>();
	parameters_.numNewbornsTraced = 0;
	parameters_.tracePrevalentCases = simParams->FirstChildElement("tracePrevalentCases")->GetText<int>() != 0;

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

	time_ = 0;
	parameters_.currTime = 0;

	//create a population
	// maybe someday we can have multiple interacting populations
	//  in that case, we'll have to change the PopulationParams to not put the values in the static Male, Female, and SteadyCouple fields
	population_ = new Population(parameters_, simParams->FirstChildElement("population"),
		simParams->FirstChildElement("lifeExpectancyOutput"), simParams->FirstChildElement("partnerAcqOutput"), duration_);

	if(parameters_.monthOf1990 > 0 && parameters_.outputTrace[EventParams::TraceFileType::ShiftedOutcomes])
	{
		population_->popStats->enableShiftedOutcomes(parameters_.monthOf1990);
	}
}

Simulation::~Simulation()
{
	parameters_.close();

	if(population_ != nullptr)
	{
		delete population_;
	}

	DmgProfile::deallocStaticMembers();
}

void Simulation::FirstStep()
{
	//initialize/reset monthly stats
	population_->resetMonthlyStats();
	//initialize incident infections by age
	population_->initIncidentInfectionsByAge();

	if(prevalenceDelay_ == 0)
	{
		population_->applyIncidentPrevalence(parameters_);
	}

	//print out prevalent infection stats & headers for rest of infection stats
	population_->calcPrevalentPopulation(0);

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
		population_->popStats->infectionsTracker.printInfections(parameters_, time_, parameters_.traceStreams[EventParams::TraceFileType::Infection], population_);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::Partnership])
	{
		population_->printPartnerships(parameters_, time_, parameters_.traceStreams[EventParams::TraceFileType::Partnership]);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::Clinical])
	{
		population_->printClinical(parameters_, time_, parameters_.traceStreams[EventParams::TraceFileType::Clinical]);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::Population])
	{
		population_->printPopulation(parameters_, time_, parameters_.traceStreams[EventParams::TraceFileType::Population]);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::ArtRollout])
	{
		population_->printARTRolloutOutcomes(parameters_, parameters_.traceStreams[EventParams::TraceFileType::ArtRollout]);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::CostEffectiveness])
	{
		population_->popStats->costsTracker.PrintCosts(parameters_.currTime, parameters_.traceStreams[EventParams::TraceFileType::CostEffectiveness]);
	}

	if(parameters_.debugLevel == DEBUG1)
	{
		population_->printMethodResults(parameters_, "--", "initialization", 0, "--", Constants::SHOW_INFECTED);
	}
}

void Simulation::Step()
{
	if(time_ == 0)
	{
		FirstStep();
	}

	time_++;

	double begin = timer_.GetTime();

	if(parameters_.useRollout)
	{
		population_->applyRolloutContext(parameters_, time_);
	}

	long totalSize = SimulateMonth();

	//print out new infection stats
	population_->calcPrevalentPopulation(time_);

	if(parameters_.outputTrace[EventParams::TraceFileType::Infection])
	{
		prevalence_ = population_->popStats->infectionsTracker.printInfections(parameters_, time_, parameters_.traceStreams[EventParams::TraceFileType::Infection], population_);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::Population])
	{
		population_->printPopulation(parameters_, time_, parameters_.traceStreams[EventParams::TraceFileType::Population]);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::Partnership])
	{
		population_->printPartnerships(parameters_, time_, parameters_.traceStreams[EventParams::TraceFileType::Partnership]);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::Clinical])
	{
		population_->printClinical(parameters_, time_, parameters_.traceStreams[EventParams::TraceFileType::Clinical]);
	}

	//For now, this must come after infectionsTracker.printInfections as it is what calculate prevalence
	if(parameters_.outputTrace[EventParams::TraceFileType::ShiftedOutcomes])
	{
		population_->recordShiftedOutcomes(parameters_, parameters_.traceStreams[EventParams::TraceFileType::ShiftedOutcomes]);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::ArtRollout])
	{
		population_->printARTRolloutOutcomes(parameters_, parameters_.traceStreams[EventParams::TraceFileType::ArtRollout]);
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::CostEffectiveness])
	{
		population_->popStats->costsTracker.PrintCosts(parameters_.currTime, parameters_.traceStreams[EventParams::TraceFileType::CostEffectiveness]);
	}

	if(parameters_.calibrationInputs.useCalibration && parameters_.calibrationInputs.monthOfCalibration == time_)
	{
		//If this run doesn't pass the partnership calibration stop the run and discard specified trace files
		if(!population_->passesPartnershipCalibration(parameters_))
		{
			return;
		}
	}

	if(parameters_.calibrationInputs.useCalibration)
	{
		if(!hasPassedFirstMonthCalibPrev_)
		{
			double SAPrev = population_->popStats->infectionsTracker.getSAPrev(population_);

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
				population_->saveState(parameters_.popStateStream[i], parameters_.currTime);
			}
		}
	}

	population_->resetMonthlyStats();

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

	if(time_ == duration_)
	{
		LastStep();
	}
}

void Simulation::LastStep()
{
	//print survival statistics
	if(parameters_.outputTrace[EventParams::TraceFileType::Survival])
	{
		population_->popStats->printSurvivalStats(parameters_.traceStreams[EventParams::TraceFileType::Survival]);
	}

	//Run every infected person left through CEPAC until they die
	if(failedCalibration_)
	{
		parameters_.displayOut("Partnership Calibration Failed...Deleting specified trace files...\n");
	}
	else
	{
		parameters_.displayOut("Running all remaining persons through CEPAC until they die...\n");
		population_->updateFinalPhysicalState(parameters_);
		parameters_.displayOut("Done!\n");
	}

	if(parameters_.outputTrace[EventParams::TraceFileType::Infection])
	{
		population_->popStats->printLMStats(parameters_.traceStreams[EventParams::TraceFileType::Infection]);
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

bool Simulation::LoadCepacSimContexts(const Interventions::CepacTreatmentFiles &treatment_files)
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

	//Get the times for the CEPAC input files to switch
	//TODO: Eventually there will be different times for different 'cohorts' i.e. subpopulations
	ticpp::Element *cohortTimesNode = cepacInterventionNode->FirstChildElement("cohortTimes")->FirstChildElement("cohort");

	for(int i = 0; i < Constants::NUMBER_OF_CEPAC_FILES; i++)
	{
		//By default, the first "time to switch" should be 0 (i.e. the first CEPAC .in file applies at time 0)
		if(i == 0)
		{
			parameters_.timesToSwitchSimContext[i] = 0;
		}
		else
		{
			std::string timeString = "time";
			timeString.append(boost::lexical_cast<std::string>(i));
			parameters_.timesToSwitchSimContext[i] = cohortTimesNode->FirstChildElement(timeString.c_str())->GetText<int>();
		}

		//std::cout << "Time " << i << ": " << parameters_.timesToSwitchSimContext[i] << "\n";
	}

	return true;
}

/*
* This function records all of the Rollout SimContext files to be used by the CEPAC disease model throughout the run of the transmission model
*/

bool Simulation::LoadRolloutSimContexts(const Interventions::RolloutTreatmentFiles &treatment_files)
{
	parameters_.displayOut("Rollout CEPAC Files: \n");

	for(const auto &treatment_file : treatment_files)
	{
		std::string fileName = (*rolloutFileNode).FirstChildElement("fileName")->GetTextOrDefault("");
		int fileNumber = (*rolloutFileNode).FirstChildElement("fileNumber")->GetText<int>();
		int popToApply;
		int timeToApply;
		(*rolloutFileNode).FirstChildElement("popToApply")->GetTextOrDefault<int>(&popToApply, -1);
		(*rolloutFileNode).FirstChildElement("time")->GetTextOrDefault<int>(&timeToApply, -1);
		//Make sure the number of CEPAC input files from the .xml file is not greater than the number expected by the code!
		assert(fileNumber < Constants::NUMBER_OF_ROLLOUT_FILES);
		parameters_.displayOut("\t");
		parameters_.displayOut(boost::lexical_cast<std::string>(fileNumber).c_str());
		parameters_.displayOut(": ");
		parameters_.displayOut(fileName.c_str());
		parameters_.displayOut("\n");
		//Set the CEPAC simContext from the specified CEPAC .in file
		SimContext *contextToAdd = nullptr;

		if(fileName == "" || timeToApply == -1)
		{
			continue;
		}

		contextToAdd = new SimContext(fileName.substr(0, fileName.find(CepacUtil::FILE_EXTENSION_FOR_INPUT)));
		parameters_.rolloutSimContexts.push_back(new EventParams::RolloutContext(timeToApply, contextToAdd, popToApply));
		//Don't trace any CEPAC patients -- the output doesn't make any sense and it just gets overly large for no reason
		//TODO: The reason is because the CEPAC Patient number doesn't get updated until the patient dies: this should be changed!
		//parameters_.cepacSimContext->numPatientsToTrace = 0;
		parameters_.rolloutSimContexts.at(fileNumber)->rolloutSimContext->numPatientsToTrace = 0;

		//Read in the inputs
		try
		{
			parameters_.rolloutSimContexts.back()->rolloutSimContext->readInputs();
		}
		catch(std::string errorString)
		{
			//if we can't find it and we wanted to use CEPAC, display error
			parameters_.displayOut("*****************************************\n");
			parameters_.displayOut("WARNING!\n");
			parameters_.displayOut("*****************************************\n");
			parameters_.displayOut("File '");
			parameters_.displayOut(fileName);
			parameters_.displayOut("' generates error:\n\t");
			parameters_.displayOut(errorString);
			return false;
		}

		//From the first file only, get the death tables for non-AIDS death
		if(fileNumber == 0)
		{
			CepacInputParser cepacInput(fileName);
			auto probabilities = cepacInput.parseNonAidsDeathProbabilities();
			Person::probDeathNatCauses[DmgProfile::MALE] = probabilities[0];
			Person::probDeathNatCauses[DmgProfile::FEMALE] = probabilities[1];
		}
	}

	for(criteriaNode = criteriaNode.begin(eligNodes); criteriaNode != criteriaNode.end(); criteriaNode++)
	{
		std::string criteriaName = (*criteriaNode).FirstChildElement("name")->GetText();

		if(criteriaName == "OIHist")
		{
			parameters_.rolloutEligibility.oiHistRank = (*criteriaNode).FirstChildElement("rank")->GetText<int>();

			for(int i = 0; i < Constants::NUMBER_OF_OIS; i++)
			{
				std::string index_string = boost::lexical_cast<std::string, int>(i);
				bool enabled = (*criteriaNode).FirstChildElement("OI" + index_string)->GetText<int>() != 0;
				parameters_.rolloutEligibility.oiHistOIs[i] = enabled;
			}

			parameters_.rolloutEligibility.oiHistNumToStart =
			    (*criteriaNode).FirstChildElement("numOIToStart")->GetText<int>();
		}
		else if(criteriaName == "CD4")
		{
			parameters_.rolloutEligibility.cd4Rank = (*criteriaNode).FirstChildElement("rank")->GetText<int>();
			parameters_.rolloutEligibility.cd4Bounds[Constants::LOWER] =
			    (*criteriaNode).FirstChildElement("CD4Lwr")->GetText<int>();
			parameters_.rolloutEligibility.cd4Bounds[Constants::UPPER] =
			    (*criteriaNode).FirstChildElement("CD4Upp")->GetText<int>();
		}
		else if(criteriaName == "CD4OIHist")
		{
			parameters_.rolloutEligibility.cd4OiHistRank = (*criteriaNode).FirstChildElement("rank")->GetText<int>();
			parameters_.rolloutEligibility.cd4OiHistCd4Bounds[Constants::LOWER] =
			    (*criteriaNode).FirstChildElement("CD4Lwr")->GetText<int>();
			parameters_.rolloutEligibility.cd4OiHistCd4Bounds[Constants::UPPER] =
			    (*criteriaNode).FirstChildElement("CD4Upp")->GetText<int>();

			for(int i = 0; i < Constants::NUMBER_OF_OIS; i++)
			{
				std::string index_string = boost::lexical_cast<std::string, int>(i);
				bool enabled = (*criteriaNode).FirstChildElement("OI" + index_string)->GetText<int>() != 0;
				parameters_.rolloutEligibility.cd4OiHistOIs[i] = enabled;
			}
		}
		else if(criteriaName == "HVL")
		{
			parameters_.rolloutEligibility.hvlRank = (*criteriaNode).FirstChildElement("rank")->GetText<int>();
			parameters_.rolloutEligibility.hvlBounds[Constants::LOWER] =
			    (*criteriaNode).FirstChildElement("HVLLwr")->GetText<int>();
			parameters_.rolloutEligibility.hvlBounds[Constants::UPPER] =
			    (*criteriaNode).FirstChildElement("HVLUpp")->GetText<int>();
		}
		else if(criteriaName == "CD4HVL")
		{
			parameters_.rolloutEligibility.cd4HvlRank = (*criteriaNode).FirstChildElement("rank")->GetText<int>();
			parameters_.rolloutEligibility.cd4HvlCd4Bounds[Constants::LOWER] =
			    (*criteriaNode).FirstChildElement("CD4Lwr")->GetText<int>();
			parameters_.rolloutEligibility.cd4HvlCd4Bounds[Constants::UPPER] =
			    (*criteriaNode).FirstChildElement("CD4Upp")->GetText<int>();
			parameters_.rolloutEligibility.cd4HvlHvlBounds[Constants::LOWER] =
			    (*criteriaNode).FirstChildElement("HVLLwr")->GetText<int>();
			parameters_.rolloutEligibility.cd4HvlHvlBounds[Constants::UPPER] =
			    (*criteriaNode).FirstChildElement("HVLUpp")->GetText<int>();
		}
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
	bool checkRanks = false;

	for(const auto &template_key_pair : inputs_.GetTemplateKeyMap())
	{
		auto key = template_key_pair.first;
		auto parameter_type = template_key_pair.second;
		auto parameter = inputs_.GetTimeDependentParameters()[parameter_type];

		if(parameter.time == parameters_.currTime)
		{
			// OIHist
			switch(parameter_type)
			{
			case TemplateParameter::AgeSexualDebutYears:
				population_->popWideParams.setAgeSexualDebut(parameter.value);
				break;
			case TemplateParameter::BirthRate:
				population_->popWideParams.setBirthRate(parameter.value);
				break;
			case TemplateParameter::ProportionCircumcised:
				population_->popWideParams.setProportionCircumcised(parameter.value);
				break;
			case TemplateParameter::ProportionMale:
				population_->popWideParams.setProportionMale(parameter.value);
				break;
			case TemplateParameter::OIHistRank:
				parameters_.rolloutEligibility.oiHistRank = static_cast<int>(parameter.value);
				checkRanks = true;
				break;
			case TemplateParameter::OIHistOI0:
				parameters_.rolloutEligibility.oiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::OIHistOI1:
				parameters_.rolloutEligibility.oiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::OIHistOI2:
				parameters_.rolloutEligibility.oiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::OIHistOI3:
				parameters_.rolloutEligibility.oiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::OIHistOI4:
				parameters_.rolloutEligibility.oiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::OIHistOI5:
				parameters_.rolloutEligibility.oiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::OIHistOI6:
				parameters_.rolloutEligibility.oiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::OIHistOI7:
				parameters_.rolloutEligibility.oiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::OIHistOI8:
				parameters_.rolloutEligibility.oiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::OIHistOI9:
				parameters_.rolloutEligibility.oiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::OIHistOI10:
				parameters_.rolloutEligibility.oiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::OIHistOI11:
				parameters_.rolloutEligibility.oiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::OIHistOI12:
				parameters_.rolloutEligibility.oiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::OIHistOI13:
				parameters_.rolloutEligibility.oiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::OIHistOI14:
				parameters_.rolloutEligibility.oiHistOIs[0] = parameter.value != 0;
				break; 
			case TemplateParameter::OIHistNumOIToStart:
				parameters_.rolloutEligibility.oiHistNumToStart = parameter.value;
				break;
			case TemplateParameter::CD4Rank:
				parameters_.rolloutEligibility.cd4Rank = parameter.value;
				checkRanks = true;
				break;
			case TemplateParameter::CD4CD4Upp:
				parameters_.rolloutEligibility.cd4Bounds[1] = parameter.value;
				break;
			case TemplateParameter::CD4CD4Lwr:
				parameters_.rolloutEligibility.cd4Bounds[0] = parameter.value;
				break;
			case TemplateParameter::CD4OIHistRank:
				parameters_.rolloutEligibility.cd4OiHistRank = parameter.value;
				checkRanks = true;
				break;
			case TemplateParameter::CD4OIHistOI0:
				parameters_.rolloutEligibility.cd4OiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::CD4OIHistOI1:
				parameters_.rolloutEligibility.cd4OiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::CD4OIHistOI2:
				parameters_.rolloutEligibility.cd4OiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::CD4OIHistOI3:
				parameters_.rolloutEligibility.cd4OiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::CD4OIHistOI4:
				parameters_.rolloutEligibility.cd4OiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::CD4OIHistOI5:
				parameters_.rolloutEligibility.cd4OiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::CD4OIHistOI6:
				parameters_.rolloutEligibility.cd4OiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::CD4OIHistOI7:
				parameters_.rolloutEligibility.cd4OiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::CD4OIHistOI8:
				parameters_.rolloutEligibility.cd4OiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::CD4OIHistOI9:
				parameters_.rolloutEligibility.cd4OiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::CD4OIHistOI10:
				parameters_.rolloutEligibility.cd4OiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::CD4OIHistOI11:
				parameters_.rolloutEligibility.cd4OiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::CD4OIHistOI12:
				parameters_.rolloutEligibility.cd4OiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::CD4OIHistOI13:
				parameters_.rolloutEligibility.cd4OiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::CD4OIHistOI14:
				parameters_.rolloutEligibility.cd4OiHistOIs[0] = parameter.value != 0;
				break;
			case TemplateParameter::CD4OIHistCD4Upp:
				parameters_.rolloutEligibility.cd4OiHistCd4Bounds[1] = parameter.value;
				break;
			case TemplateParameter::CD4OIHistCD4Lwr:
				parameters_.rolloutEligibility.cd4OiHistCd4Bounds[0] = parameter.value;
				break;
			case TemplateParameter::HVLRank:
				parameters_.rolloutEligibility.hvlRank = parameter.value;
				checkRanks = true;
			case TemplateParameter::HVLHVLUpp:
				parameters_.rolloutEligibility.hvlBounds[1] = parameter.value;
			case TemplateParameter::HVLHVLLwr:
				parameters_.rolloutEligibility.hvlBounds[0] = parameter.value;
			case TemplateParameter::CD4HVLRank:
				parameters_.rolloutEligibility.cd4HvlRank = parameter.value;
				checkRanks = true;
			case TemplateParameter::CD4HVLCD4Upp:
				parameters_.rolloutEligibility.cd4HvlCd4Bounds[1] = parameter.value;
			case TemplateParameter::CD4HVLCD4Lwr:
				parameters_.rolloutEligibility.cd4HvlCd4Bounds[0] = parameter.value;
			case TemplateParameter::CD4HVLHVLUpp:
				parameters_.rolloutEligibility.cd4HvlHvlBounds[1] = parameter.value;
			case TemplateParameter::CD4HVLHVLLwr:
				parameters_.rolloutEligibility.cd4HvlHvlBounds[0] = parameter.value;
			case TemplateParameter::SteadyChanceCondomUsePerEventHighRisk:
				auto beta = ParseBeta(parameter.value);
				population_->popWideParams.setChanceCondomUsePerEvent(Person::HIGH, SexualPartnership::STEADY, beta);
			case TemplateParameter::SteadyChanceCondomUsePerEventLowRisk:
				auto beta = ParseBeta(parameter.value);
				population_->popWideParams.setChanceCondomUsePerEvent(Person::LOW, SexualPartnership::STEADY, beta);
			case TemplateParameter::RegularChanceCondomUsePerEventHighRisk:
				auto beta = ParseBeta(parameter.value);
				population_->popWideParams.setChanceCondomUsePerEvent(Person::HIGH, SexualPartnership::REGULAR, beta);
			case TemplateParameter::RegularChanceCondomUsePerEventLowRisk:
				auto beta = ParseBeta(parameter.value);
				population_->popWideParams.setChanceCondomUsePerEvent(Person::LOW, SexualPartnership::REGULAR, beta);
			case TemplateParameter::CasualChanceCondomUsePerEventHighRisk:
				auto beta = ParseBeta(parameter.value);
				population_->popWideParams.setChanceCondomUsePerEvent(Person::HIGH, SexualPartnership::CASUAL, beta);
			case TemplateParameter::CasualChanceCondomUsePerEventLowRisk:
				auto beta = ParseBeta(parameter.value);
				population_->popWideParams.setChanceCondomUsePerEvent(Person::LOW, SexualPartnership::CASUAL, beta);
			case TemplateParameter::CswChanceCondomUsePerEventHighRisk:
				auto beta = ParseBeta(parameter.value);
				population_->popWideParams.setChanceCondomUsePerEvent(Person::HIGH, SexualPartnership::CSW, beta);
			case TemplateParameter::CswChanceCondomUsePerEventLowRisk:
				auto beta = ParseBeta(parameter.value);
				population_->popWideParams.setChanceCondomUsePerEvent(Person::LOW, SexualPartnership::CSW, beta);
				break;
			default:
				throw std::runtime_error("not implemented");
			}
		}
	}

	if(checkRanks)
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
}

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

	if(population_->popStats->isTimeToRecordLE(time_))
	{
		recordLE = true;
	}

	if(population_->popStats->isTimeToRecordPartAcq(time_))
	{
		recordPartAcq = true;
	}

	if(population_->popStats->isFirstMonthToRecordLE(time_))
	{
		firstMonthToRecord = true;
	}

	if(population_->popStats->isTimeToPrintLE(time_))
	{
		lastMonthToRecord = true;
	}

	population_->updatePhysicalState(parameters_, recordLE, firstMonthToRecord);

	if(parameters_.useRollout)
	{
		population_->applyARTRollout(parameters_);
	}

	population_->births(parameters_);

	if(lastMonthToRecord)
	{
		population_->updateAgeBucketsLE();

		if(parameters_.outputTrace[EventParams::TraceFileType::LifeExpectancy])
		{
			population_->popStats->printLEStats(parameters_.traceStreams[EventParams::TraceFileType::LifeExpectancy], time_);
		}

		delete population_->popStats->selectedLEStats;
		population_->popStats->selectedLEStats = nullptr;
	}

	//steadyCouple, flings, and dissolveSexualPartnerships
	population_->updatePartnerships(parameters_);

	if(recordPartAcq)
	{
		population_->popStats->selectedPartAcqStats = new PopStats::SinglePartAcqStats();
		population_->recordPartAcqFreq();

		if(parameters_.outputTrace[EventParams::TraceFileType::PartnershipAcquisition])
		{
			population_->popStats->printPartAcqStats(parameters_.traceStreams[EventParams::TraceFileType::PartnershipAcquisition], time_);
		}

		delete population_->popStats->selectedPartAcqStats;
		population_->popStats->selectedPartAcqStats = nullptr;
	}

	//apply incident prevalence
	if(prevalenceDelay_ != 0 && prevalenceDelay_ == time_)
	{
		population_->applyIncidentPrevalence(parameters_);
	}

	//Will confirm that population_->currSize is correct and update size of age ranges
	return population_->updateSize();
}

RunStats *Simulation::GetCEPACRunStats()
{
	return parameters_.cepacRunStats;
}

PopStats *Simulation::GetPopStats()
{
	return population_->popStats;
}

EventParams *Simulation::GetEventParams()
{
	return &parameters_;
}
