#include <time.h>
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <set>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#ifdef WIN32
#include <io.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "Simulation.h"

#include "Constants.h"
#include "Population.h"
#include "cepac/include.h"
#include "cepacbridge/CepacInputParser.h"
#include "data/EventParams.h"
#include "graphviz/graphVizParse.h"
#include "util/HighResolutionTimer.h"
#include "util/Util.h"
#include "entities/classifiers/DmgProfile.h"
#include "entities/behaviors/SexualBehaviorParams.h"

Simulation::Simulation(const std::string &xmlFile)
    : xmlFile_(xmlFile),
	  failedCalibration_(false),
	  hasPassedFirstMonthCalibPrev_(false),
	  monthOfFirstMonthCalibPrev_(0)
{
	
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

void Simulation::Initialize()
{
	boost::filesystem::path xmlPath(xmlFile_);
	parameters_.simName = xmlPath.stem().string();

	LoadInput(xmlFile_);

	parameters_.displayOut("Sim name is " + parameters_.simName + "\n");
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
	population_->calculatePrevalentPopulationSize(0);

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

bool Simulation::Step()
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

	int totalSize = SimulateMonth();

	//print out new infection stats
	population_->calculatePrevalentPopulationSize(time_);

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
			return false;
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
		return false;
	}

	return true;
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

void Simulation::LoadInput(const std::string &xmlFile)
{
	ticpp::Document doc(xmlFile);
	doc.LoadFile();
	parameters_.displayOut("Simulation Parameters\n");
	ticpp::Element *simParams = doc.FirstChildElement("simulation");

	int fixedSeed = 0;
	simParams->FirstChildElement("fixedSeed")->GetText(&fixedSeed);

	std::stringstream seedMessage;
	if(fixedSeed == -1)
	{
		seedMessage << "Using random seed";
	}
	else if(fixedSeed == 0)
	{
		seedMessage << "Using default fixed seed";
	}
	else
	{
		seedMessage << "Using fixed seed = " << fixedSeed;
	}
	seedMessage << std::endl;
	std::string seedMessageStr = seedMessage.str();

	parameters_.displayOut(seedMessageStr.c_str());
	simParams->FirstChildElement("monthOf1990")->GetText(&parameters_.monthOf1990);

	double inputVersion = 0;
	simParams->FirstChildElement("inputVersion")->GetText(&inputVersion);

	parameters_.displayOut("Input Version =");
	parameters_.displayOut(boost::lexical_cast<std::string>(inputVersion).c_str());
	parameters_.displayOut("\n");

	if(inputVersion != Util::INPUT_VERSION)
	{
		parameters_.displayOut("Input Version for ");
		parameters_.displayOut(xmlFile.c_str());
		parameters_.displayOut(" is not ");
		parameters_.displayOut(boost::lexical_cast<std::string>(Util::INPUT_VERSION).c_str());
		parameters_.displayOut(".  Stopping model execution!\n");

		throw std::runtime_error("bad input version");
	}

	//save Debug Level
	simParams->FirstChildElement("debugLevel")->GetText<int>((int *)&parameters_.debugLevel);
	parameters_.displayOut("\tDebug Level = ");
	parameters_.displayOut(boost::lexical_cast<std::string>(parameters_.debugLevel).c_str());
	parameters_.displayOut("\n");

	//save Concurrency Definitions
	for(int i = 0; i < Constants::NUMBER_CONCURRENCY_DEFS; i++)
	{
		int minNeeded = 0;
		auto defName = "def" + boost::lexical_cast<std::string>(i);
		auto definitionElement = simParams->FirstChildElement("concurrencyDefinition")->FirstChildElement(defName);
		definitionElement->FirstChildElement("minNeeded")->GetText(&minNeeded);
		bool useDef = false;
		definitionElement->FirstChildElement("allow")->GetText(&useDef);
		parameters_.concurrencyDef[i] = new EventParams::ConcurrencyDef(minNeeded, useDef);
	}

	//save which trace files to output
	std::string traceIDs[] = {"population", "infection", "partnership", "survival", "costEffectiveness", "clinical", "events", "health", "singleperson", "le", "partacq", "calibStats", "artRollout", "shiftedOutcomes"};

	for(int i = 0; i < Constants::NUMBER_OF_TRACE_FILES; i++)
	{
		simParams->FirstChildElement("writeTrace")->FirstChildElement(traceIDs[i])->GetText(&parameters_.outputTrace[i]);
		simParams->FirstChildElement("extensionNames")->FirstChildElement(traceIDs[i])->GetText(&parameters_.traceExtensions[i]);
	}

	//save calibration inputs
	ticpp::Element *calibParams = simParams->FirstChildElement("calibration");
	calibParams->FirstChildElement("useCalibration")->GetText(&parameters_.calibrationInputs.useCalibration);

	if(parameters_.calibrationInputs.useCalibration)
	{
		calibParams->FirstChildElement("monthOfCalibration")->GetText<int>(&parameters_.calibrationInputs.monthOfCalibration);
		ticpp::Element *outcomeParams = calibParams->FirstChildElement("partnershipOutcomes");

		outcomeParams->FirstChildElement("steadyPrev")->FirstChildElement("popOfInterest")->GetText(&parameters_.calibrationInputs.steadyPrevPopulation);
		outcomeParams->FirstChildElement("steadyPrev")->FirstChildElement("lwrBound")->GetText(&parameters_.calibrationInputs.steadyPrevBounds[Constants::LOWER]);
		outcomeParams->FirstChildElement("steadyPrev")->FirstChildElement("uprBound")->GetText(&parameters_.calibrationInputs.steadyPrevBounds[Constants::UPPER]);
		outcomeParams->FirstChildElement("casualPrev")->FirstChildElement("popOfInterest")->GetText(&parameters_.calibrationInputs.casualPrevPopulation);
		outcomeParams->FirstChildElement("casualPrev")->FirstChildElement("lwrBound")->GetText(&parameters_.calibrationInputs.casualPrevBounds[Constants::LOWER]);
		outcomeParams->FirstChildElement("casualPrev")->FirstChildElement("uprBound")->GetText(&parameters_.calibrationInputs.casualPrevBounds[Constants::UPPER]);
		outcomeParams->FirstChildElement("cswPrev")->FirstChildElement("popOfInterest")->GetText(&parameters_.calibrationInputs.CSWPrevPopulation);
		outcomeParams->FirstChildElement("cswPrev")->FirstChildElement("lwrBound")->GetText(&parameters_.calibrationInputs.CSWPrevBounds[Constants::LOWER]);
		outcomeParams->FirstChildElement("cswPrev")->FirstChildElement("uprBound")->GetText(&parameters_.calibrationInputs.CSWPrevBounds[Constants::UPPER]);
		outcomeParams->FirstChildElement("propInCon")->FirstChildElement("popOfInterest")->GetText(&parameters_.calibrationInputs.propInConcurrentPopulation);
		outcomeParams->FirstChildElement("propInCon")->FirstChildElement("lwrBound")->GetText(&parameters_.calibrationInputs.propInConcurrentBounds[Constants::LOWER]);
		outcomeParams->FirstChildElement("propInCon")->FirstChildElement("uprBound")->GetText(&parameters_.calibrationInputs.propInConcurrentBounds[Constants::UPPER]);
		outcomeParams->FirstChildElement("numActs")->FirstChildElement("popOfInterest")->GetText(&parameters_.calibrationInputs.numActsPopulation);
		outcomeParams->FirstChildElement("numActs")->FirstChildElement("lwrBound")->GetText(&parameters_.calibrationInputs.numActsBounds[Constants::LOWER]);
		outcomeParams->FirstChildElement("numActs")->FirstChildElement("uprBound")->GetText(&parameters_.calibrationInputs.numActsBounds[Constants::UPPER]);
		outcomeParams->FirstChildElement("femaleCasualPrev")->FirstChildElement("ratio")->GetText(&parameters_.calibrationInputs.femaleCasualPrevRatio);
		outcomeParams->FirstChildElement("femalePropInCon")->FirstChildElement("ratio")->GetText(&parameters_.calibrationInputs.femalePropInConcurrentRatio);
		outcomeParams->FirstChildElement("femaleNumActsLRtoHR")->FirstChildElement("ratio")->GetText(&parameters_.calibrationInputs.femaleNumActsLRtoHRRatio);

		for(int i = 0; i < Constants::NUMBER_OF_TRACE_FILES; i++)
		{
			calibParams->FirstChildElement("tossFiles")->FirstChildElement("traceFile" + std::to_string(i))->GetText(&parameters_.calibrationInputs.tossFiles[i]);
		}

		for(int i = 0; i < Constants::NUMBER_CALIBRATION_PREVS; i++)
		{
			calibParams->FirstChildElement("calendarPrevalence")->FirstChildElement("time" + std::to_string(i))->GetText(&parameters_.calibrationInputs.calendarPrevs[i]);
		}

		for(int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
		{
			calibParams->FirstChildElement("storePoint" + std::to_string(i) + "Mth")->GetText(&parameters_.calibrationInputs.saveStateTimePoints[i]);
		}

		calibParams->FirstChildElement("thresholdMultiplier")->GetText(&parameters_.calibrationInputs.thresholdPrevMult);
	}

	simParams->FirstChildElement("timeLimitMth")->GetText(&duration_);
	parameters_.displayOut("\tTime steps = ");
	parameters_.displayOut(boost::lexical_cast<std::string>(duration_).c_str());
	parameters_.displayOut("\n");

	simParams->FirstChildElement("population")->FirstChildElement("initialState")->FirstChildElement("delay")->GetText(&prevalenceDelay_);
	parameters_.delayPrevalence = prevalenceDelay_;
	parameters_.displayOut("\tDelay Prevalence = ");
	parameters_.displayOut(boost::lexical_cast<std::string>(prevalenceDelay_).c_str());
	parameters_.displayOut("\n");
	bool filesLoaded;

	//Load the CEPAC files
	if(simParams->FirstChildElement("population")->FirstChildElement("interventions")->FirstChildElement("artRolloutIntervention")->FirstChildElement("useRollout")->GetText() == "1")
	{
		//use art rollout input files
		parameters_.useRollout = true;
		filesLoaded = SetRolloutSimContexts(simParams->FirstChildElement("population")->FirstChildElement("interventions")->FirstChildElement("artRolloutIntervention"));

		int proportionYear = 2002;
		ticpp::Iterator<ticpp::Element> proportionIterator;

		for(proportionIterator = proportionIterator.begin(simParams->FirstChildElement("population")->FirstChildElement("interventions")->FirstChildElement("artRolloutIntervention")->FirstChildElement("targetRolloutProportions")); proportionIterator != proportionIterator.end(); ++proportionIterator)
		{
			int year = std::stoi(proportionIterator.Get()->GetAttribute("year"));
			assert(year == proportionYear++);
			double proportion = 0;
			proportionIterator.Get()->GetText(&proportion);
			parameters_.targetYearlyRolloutProportions.push_back(proportion);
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

	simParams->FirstChildElement("numberToTracePerAgeRange")->GetText(&parameters_.numToTrace);
	simParams->FirstChildElement("numberNewbornsToTrace")->GetText(&parameters_.numNewbornsToTrace);
	simParams->FirstChildElement("monthTraceNewborns")->GetText(&parameters_.monthTraceNewborns);
	parameters_.numNewbornsTraced = 0;
	simParams->FirstChildElement("tracePrevalentCases")->GetText(&parameters_.tracePrevalentCases);

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

	LoadTimeDependentParameters(simParams->FirstChildElement("timeDependentParameters", false));
}

void Simulation::LoadTimeDependentParameters(ticpp::Element *timeDependentParametersElement)
{
	if(!timeDependentParametersElement)
	{
		return;
	}

	ticpp::Iterator<ticpp::Element> child;
	for(child = child.begin(timeDependentParametersElement); child != child.end(); child++)
	{
		TimeDependentParameter parameter;
		std::string value;
		child->GetValue<std::string>(&value);
		assert(value == "parameter");
		parameter.time = child->GetAttribute<int>("time");
		parameter.key = child->GetAttribute("key");
		parameter.value = child->GetAttribute("value");
		timeDependentParameters_.push_back(parameter);
	}
}

/*
* This function records all of the SimContext files to be used by the CEPAC disease model throughout the run of the transmission model
*/

bool Simulation::SetCEPACSimContexts(ticpp::Element *cepacInterventionNode)
{
	parameters_.displayOut("CEPAC Files: \n");
	ticpp::Element *treatmentFilesNode = cepacInterventionNode->FirstChildElement("cepacTreatmentFiles");
	//Only iterates through Element nodes with value "ElementValue"
	ticpp::Iterator<ticpp::Element> treatmentFileNode("treatmentFile");

	for(treatmentFileNode = treatmentFileNode.begin(treatmentFilesNode); treatmentFileNode != treatmentFileNode.end();
	        treatmentFileNode++)
	{
		std::string fileName = (*treatmentFileNode).FirstChildElement("fileName")->GetTextOrDefault("");

		int fileNumber = 0;
		treatmentFileNode->FirstChildElement("fileNumber")->GetText(&fileNumber);

		//Make sure the number of CEPAC input files from the .xml file is not greater than the number expected by the code!
		assert(fileNumber < Constants::NUMBER_OF_CEPAC_FILES);
		parameters_.displayOut("\t");
		parameters_.displayOut(boost::lexical_cast<std::string>(fileNumber).c_str());
		parameters_.displayOut(": ");
		parameters_.displayOut(fileName.c_str());
		parameters_.displayOut("\n");
		//Set the CEPAC simContext from the specified CEPAC .in file
		parameters_.cepacSimContexts.push_back(new SimContext(fileName.substr(0,
		        fileName.find(CepacUtil::FILE_EXTENSION_FOR_INPUT))));
		assert(parameters_.cepacSimContexts.size() == (static_cast<size_t>(fileNumber) + 1));
		//parameters_.cepacSimContext = new SimContext(cepacInputFile.substr(0, cepacInputFile.find(CepacUtil::FILE_EXTENSION_FOR_INPUT)));
		//Don't trace any CEPAC patients -- the output doesn't make any sense and it just gets overly large for no reason
		//TODO: The reason is because the CEPAC Patient number doesn't get updated until the patient dies: this should be changed!
		//parameters_.cepacSimContext->numPatientsToTrace = 0;
		parameters_.cepacSimContexts.at(fileNumber)->numPatientsToTrace = 0;

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
			parameters_.displayOut("File '");
			parameters_.displayOut(fileName.c_str());
			parameters_.displayOut("' generates error:\n\t");
			parameters_.displayOut(errorString.c_str());
			parameters_.displayOut("\n");
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
			cohortTimesNode->FirstChildElement(timeString.c_str())->GetText(&parameters_.timesToSwitchSimContext[i]);
		}

		//std::cout << "Time " << i << ": " << parameters_.timesToSwitchSimContext[i] << "\n";
	}

	return true;
}

/*
* This function records all of the Rollout SimContext files to be used by the CEPAC disease model throughout the run of the transmission model
*/

bool Simulation::SetRolloutSimContexts(ticpp::Element *rolloutInterventionNode)
{
	parameters_.displayOut("Rollout CEPAC Files: \n");
	ticpp::Element *rolloutFilesNode = rolloutInterventionNode->FirstChildElement("rolloutTreatmentFiles");
	//Only iterates through Element nodes with value "ElementValue"
	ticpp::Iterator<ticpp::Element> rolloutFileNode("rolloutFile");

	for(rolloutFileNode = rolloutFileNode.begin(rolloutFilesNode); rolloutFileNode != rolloutFileNode.end(); rolloutFileNode++)
	{
		std::string fileName = rolloutFileNode->FirstChildElement("fileName")->GetTextOrDefault("");

		int fileNumber = 0;
		rolloutFileNode->FirstChildElement("fileNumber")->GetText(&fileNumber);

		int popToApply = 0;
		rolloutFileNode->FirstChildElement("popToApply")->GetTextOrDefault(&popToApply, -1);

		int timeToApply = 0;
		rolloutFileNode->FirstChildElement("time")->GetTextOrDefault(&timeToApply, -1);

		//Make sure the number of CEPAC input files from the .xml file is not greater than the number expected by the code!
		assert(fileNumber < Constants::NUMBER_OF_ROLLOUT_FILES);

		parameters_.displayOut("\t" + std::to_string(fileNumber) + ": " + fileName + "\n");

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
			parameters_.displayOut("File '" + fileName + "' generates error:\n");
			parameters_.displayOut("\t" + errorString);
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

	ticpp::Element *eligNodes = rolloutInterventionNode->FirstChildElement("rolloutEligibility");
	ticpp::Iterator<ticpp::Element> criteriaNode("criteria");

	for(criteriaNode = criteriaNode.begin(eligNodes); criteriaNode != criteriaNode.end(); criteriaNode++)
	{
		std::string criteriaName = (*criteriaNode).FirstChildElement("name")->GetText();

		if(criteriaName == "OIHist")
		{
			criteriaNode->FirstChildElement("rank")->GetText(&parameters_.rolloutEligibility.oiHistRank);

			for(int i = 0; i < Constants::NUMBER_OF_OIS; i++)
			{
				criteriaNode->FirstChildElement("OI" + std::to_string(i))->GetText(&parameters_.rolloutEligibility.oiHistOIs[i]);
			}

			criteriaNode->FirstChildElement("numOIToStart")->GetText(&parameters_.rolloutEligibility.oiHistNumToStart);
		}
		else if(criteriaName == "CD4")
		{
			criteriaNode->FirstChildElement("rank")->GetText(&parameters_.rolloutEligibility.cd4Rank);
			criteriaNode->FirstChildElement("CD4Lwr")->GetText(&parameters_.rolloutEligibility.cd4Bounds[Constants::LOWER]);
			criteriaNode->FirstChildElement("CD4Upp")->GetText(&parameters_.rolloutEligibility.cd4Bounds[Constants::UPPER]);
		}
		else if(criteriaName == "CD4OIHist")
		{
			criteriaNode->FirstChildElement("rank")->GetText(&parameters_.rolloutEligibility.cd4OiHistRank);
			criteriaNode->FirstChildElement("CD4Lwr")->GetText(&parameters_.rolloutEligibility.cd4OiHistCd4Bounds[Constants::LOWER]);
			criteriaNode->FirstChildElement("CD4Upp")->GetText(&parameters_.rolloutEligibility.cd4OiHistCd4Bounds[Constants::UPPER]);

			for(int i = 0; i < Constants::NUMBER_OF_OIS; i++)
			{
				criteriaNode->FirstChildElement("OI" + std::to_string(i))->GetText(&parameters_.rolloutEligibility.cd4OiHistOIs[i]);
			}
		}
		else if(criteriaName == "HVL")
		{
			criteriaNode->FirstChildElement("rank")->GetText(&parameters_.rolloutEligibility.hvlRank);
			criteriaNode->FirstChildElement("HVLLwr")->GetText(&parameters_.rolloutEligibility.hvlBounds[Constants::LOWER]);
			criteriaNode->FirstChildElement("HVLUpp")->GetText(&parameters_.rolloutEligibility.hvlBounds[Constants::UPPER]);
		}
		else if(criteriaName == "CD4HVL")
		{
			criteriaNode->FirstChildElement("rank")->GetText(&parameters_.rolloutEligibility.cd4HvlRank);
			criteriaNode->FirstChildElement("CD4Lwr")->GetText(&parameters_.rolloutEligibility.cd4HvlCd4Bounds[Constants::LOWER]);
			criteriaNode->FirstChildElement("CD4Upp")->GetText(&parameters_.rolloutEligibility.cd4HvlCd4Bounds[Constants::UPPER]);
			criteriaNode->FirstChildElement("HVLLwr")->GetText(&parameters_.rolloutEligibility.cd4HvlHvlBounds[Constants::LOWER]);
			criteriaNode->FirstChildElement("HVLUpp")->GetText(&parameters_.rolloutEligibility.cd4HvlHvlBounds[Constants::UPPER]);
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

	for(auto parameter : timeDependentParameters_)
	{
		if(parameter.time == parameters_.currTime)
		{
			// OIHist
			if(parameter.key == "artRolloutIntervention/rolloutEligibility/OIHist/rank")
			{
				parameters_.rolloutEligibility.oiHistRank = std::stoi(parameter.value);
				checkRanks = true;
			}
			else if(parameter.key == "artRolloutIntervention/rolloutEligibility/OIHist/OI0"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/OIHist/OI1"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/OIHist/OI2"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/OIHist/OI3"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/OIHist/OI4"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/OIHist/OI5"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/OIHist/OI6"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/OIHist/OI7"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/OIHist/OI8"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/OIHist/OI9"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/OIHist/OI10"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/OIHist/OI11"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/OIHist/OI12"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/OIHist/OI13"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/OIHist/OI14")
			{
				int oiNumber = std::stoi(parameter.key.substr(parameter.key.length() - 1, 1));
				if(parameter.key[parameter.key.length() - 2] != 'I')
				{
					oiNumber = std::stoi(parameter.key.substr(parameter.key.length() - 2, 2));
				}
				parameters_.rolloutEligibility.oiHistOIs[oiNumber] = std::stoi(parameter.value) != 0;
			}
			else if(parameter.key == "artRolloutIntervention/rolloutEligibility/OIHist/numOIToStart")
			{
				parameters_.rolloutEligibility.oiHistNumToStart = std::stoi(parameter.value);
			}
			// CD4
			else if(parameter.key == "artRolloutIntervention/rolloutEligibility/CD4/rank")
			{
				parameters_.rolloutEligibility.cd4Rank = std::stoi(parameter.value);
				checkRanks = true;
			}
			else if(parameter.key == "artRolloutIntervention/rolloutEligibility/CD4/CD4Upp")
			{
				parameters_.rolloutEligibility.cd4Bounds[1] = std::stoi(parameter.value);
			}
			else if(parameter.key == "artRolloutIntervention/rolloutEligibility/CD4/CD4Lwr")
			{
				parameters_.rolloutEligibility.cd4Bounds[0] = std::stoi(parameter.value);
			}
			// CD4OIHist
			else if(parameter.key == "artRolloutIntervention/rolloutEligibility/CD4OIHist/rank")
			{
				parameters_.rolloutEligibility.cd4OiHistRank = std::stoi(parameter.value);
				checkRanks = true;
			}
			else if(parameter.key == "artRolloutIntervention/rolloutEligibility/CD4OIHist/OI0"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/CD4OIHist/OI1"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/CD4OIHist/OI2"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/CD4OIHist/OI3"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/CD4OIHist/OI4"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/CD4OIHist/OI5"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/CD4OIHist/OI6"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/CD4OIHist/OI7"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/CD4OIHist/OI8"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/CD4OIHist/OI9"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/CD4OIHist/OI10"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/CD4OIHist/OI11"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/CD4OIHist/OI12"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/CD4OIHist/OI13"
				|| parameter.key == "artRolloutIntervention/rolloutEligibility/CD4OIHist/OI14")
			{
				int oiNumber = std::stoi(parameter.key.substr(parameter.key.length() - 1, 1));
				if(parameter.key[parameter.key.length() - 2] != 'I')
				{
					oiNumber = std::stoi(parameter.key.substr(parameter.key.length() - 2, 2));
				}
				parameters_.rolloutEligibility.oiHistOIs[oiNumber] = std::stoi(parameter.value) != 0;
			}
			else if(parameter.key == "artRolloutIntervention/rolloutEligibility/CD4OIHist/CD4Upp")
			{
				parameters_.rolloutEligibility.cd4OiHistCd4Bounds[1] = std::stoi(parameter.value);
			}
			else if(parameter.key == "artRolloutIntervention/rolloutEligibility/CD4OIHist/CD4Lwr")
			{
				parameters_.rolloutEligibility.cd4OiHistCd4Bounds[0] = std::stoi(parameter.value);
			}
			// HVL
			else if(parameter.key == "artRolloutIntervention/rolloutEligibility/HVL/rank")
			{
				parameters_.rolloutEligibility.hvlRank = std::stoi(parameter.value);
				checkRanks = true;
			}
			else if(parameter.key == "artRolloutIntervention/rolloutEligibility/HVL/HVLUpp")
			{
				parameters_.rolloutEligibility.hvlBounds[1] = std::stoi(parameter.value);
			}
			else if(parameter.key == "artRolloutIntervention/rolloutEligibility/HVL/HVLLwr")
			{
				parameters_.rolloutEligibility.hvlBounds[0] = std::stoi(parameter.value);
			}
			// CD4HVL
			else if(parameter.key == "artRolloutIntervention/rolloutEligibility/CD4HVL/rank")
			{
				parameters_.rolloutEligibility.cd4HvlRank = std::stoi(parameter.value);
				checkRanks = true;
			}
			else if(parameter.key == "artRolloutIntervention/rolloutEligibility/CD4HVL/CD4Upp")
			{
				parameters_.rolloutEligibility.cd4HvlCd4Bounds[1] = std::stoi(parameter.value);
			}
			else if(parameter.key == "artRolloutIntervention/rolloutEligibility/CD4HVL/CD4Lwr")
			{
				parameters_.rolloutEligibility.cd4HvlCd4Bounds[0] = std::stoi(parameter.value);
			}
			else if(parameter.key == "artRolloutIntervention/rolloutEligibility/CD4HVL/HVLUpp")
			{
				parameters_.rolloutEligibility.cd4HvlHvlBounds[1] = std::stoi(parameter.value);
			}
			else if(parameter.key == "artRolloutIntervention/rolloutEligibility/CD4HVL/HVLLwr")
			{
				parameters_.rolloutEligibility.cd4HvlHvlBounds[0] = std::stoi(parameter.value);
			}
			else if(parameter.key == "behavior/male/steady/chanceCondomUsePerEventHighRisk")
			{
				auto beta = ParseBeta(parameter.value);
				population_->popWideParams.setChanceCondomUsePerEvent(Person::HIGH, SexualPartnership::STEADY, beta);
			}
			else if(parameter.key == "behavior/male/steady/chanceCondomUsePerEventLowRisk")
			{
				auto beta = ParseBeta(parameter.value);
				population_->popWideParams.setChanceCondomUsePerEvent(Person::LOW, SexualPartnership::STEADY, beta);
			}
			else if(parameter.key == "behavior/male/regular/chanceCondomUsePerEventHighRisk")
			{
				auto beta = ParseBeta(parameter.value);
				population_->popWideParams.setChanceCondomUsePerEvent(Person::HIGH, SexualPartnership::REGULAR, beta);
			}
			else if(parameter.key == "behavior/male/regular/chanceCondomUsePerEventLowRisk")
			{
				auto beta = ParseBeta(parameter.value);
				population_->popWideParams.setChanceCondomUsePerEvent(Person::LOW, SexualPartnership::REGULAR, beta);
			}
			else if(parameter.key == "behavior/male/casual/chanceCondomUsePerEventHighRisk")
			{
				auto beta = ParseBeta(parameter.value);
				population_->popWideParams.setChanceCondomUsePerEvent(Person::HIGH, SexualPartnership::CASUAL, beta);
			}
			else if(parameter.key == "behavior/male/casual/chanceCondomUsePerEventLowRisk")
			{
				auto beta = ParseBeta(parameter.value);
				population_->popWideParams.setChanceCondomUsePerEvent(Person::LOW, SexualPartnership::CASUAL, beta);
			}
			else if(parameter.key == "behavior/male/csw/chanceCondomUsePerEventHighRisk")
			{
				auto beta = ParseBeta(parameter.value);
				population_->popWideParams.setChanceCondomUsePerEvent(Person::HIGH, SexualPartnership::CSW, beta);
			}
			else if(parameter.key == "behavior/male/csw/chanceCondomUsePerEventLowRisk")
			{
				auto beta = ParseBeta(parameter.value);
				population_->popWideParams.setChanceCondomUsePerEvent(Person::LOW, SexualPartnership::CSW, beta);
			}
			else if(parameter.key == "population/proportionCircumcised")
			{
				population_->popWideParams.setProportionCircumcised(std::stod(parameter.value));
			}
			else
			{
				throw std::runtime_error("unknown parameter: " + parameter.key);
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
