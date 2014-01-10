#include <time.h>
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <set>
#include <boost/lexical_cast.hpp>

#ifdef WIN32
#include <io.h>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
const char PATH_SEPARATOR = '\\';
#else 
const char PATH_SEPARATOR = '/';
#endif

#include "Sim.h"

#include "Constants.h"
#include "Population.h"
#include "cepacbridge/ParseCepacInput.h"
#include "graphviz/graphVizParse.h"
#include "data/EventParams.h"
#include "entities/classifiers/DmgProfile.h"
#include "util/Util.h"
#include "util/Timer.h"
#include "cepac/include.h"

class DisplayBox;

Sim::Sim(std::string _paramsXML, DisplayBox *dbox)
{
	if(dbox)
	{
		eventParams.displaybox = dbox;
	}

	//Character to seperate directories / for linux, mac and \ for windows
	std::string dirSepChar = std::string(1, PATH_SEPARATOR);

	//Remove the .xml and the directory from _paramsXML to get the sim name
	eventParams.simName = _paramsXML.substr(_paramsXML.find_last_of(dirSepChar) + 1);
	eventParams.simName = eventParams.simName.substr(0, eventParams.simName.length() - 4);

	size_t simNameLength = eventParams.simName.length();
	size_t seqStartIndex = eventParams.simName.rfind("_seq01");

	seqPos = 1;//set the position in the sequence to 1
	numInSeq = 1; //initialize the total number of files in sequence
	maxTime = 0; //if sequence we will sum up the individual seq times to get the real max time

	if (seqStartIndex != std::string::npos)
	{
		//file is the start of a sequence
		isSeq = true;
		eventParams.simName = eventParams.simName.substr(0, simNameLength - std::string("_seq00").length());

		//count number of files in sequence
		for(;;)
		{
			std::string positionString = boost::lexical_cast<std::string>(numInSeq / 10) + boost::lexical_cast<std::string>(numInSeq % 10);	

			std::ifstream inputFile;
			//boost::filesystem::path fullpath(_paramsXML);
			//boost::filesystem::path newpath = fullpath.parent_path()/ (eventParams.simName+"_seq"+positionString+".xml");
			std::string filename = _paramsXML.substr(0, _paramsXML.find_last_of(dirSepChar)) + dirSepChar + eventParams.simName + "_seq" + positionString + ".xml";
#if defined( CONSOLE )
			filename = eventParams.simName + "_seq" + positionString + ".xml";
#endif
			std::cout << "filename:" << filename << std::endl;
			inputFile.open(filename.c_str(), std::ifstream::in);
			if (inputFile.fail())
			{
				inputFile.close();
				numInSeq--;
				break;
			}


			inputFile.close();

			ticpp::Document doc(filename);
			try
			{
				doc.LoadFile();
				ticpp::Element *simParams = doc.FirstChildElement("simulation");
				maxTime+=simParams->FirstChildElement("timeRunSeq")->GetText<int>();
			} 
			catch(ticpp::Exception &_e)
			{
				eventParams.displayOut("Sim: Exception raised: ");
				eventParams.displayOut(_e.m_details.c_str());
				eventParams.displayOut("\n");
				XMLerror = true;
				return;
			}

			numInSeq++;
		}//end while
	}
	else
	{
		isSeq = false;
	}


	eventParams.displayOut("Max time at begining");

	eventParams.displayOut(boost::lexical_cast<std::string>(maxTime).c_str());
	eventParams.displayOut("\n");


	eventParams.displayOut("Sim name is ");
	eventParams.displayOut(eventParams.simName.c_str());
	eventParams.displayOut("\n");

	XMLerror = false; //Assume by default that there won't be an XMLerror

	// Load a document
	ticpp::Document doc(_paramsXML);

	try
	{
		doc.LoadFile();

		eventParams.displayOut("Simulation Parameters\n");

		ticpp::Element *simParams = doc.FirstChildElement("simulation");

		int fixedSeed = simParams->FirstChildElement("fixedSeed")->GetText<int>();
		std::stringstream seedMessage;
		if (fixedSeed == -1)
			seedMessage << "Using random seed";
		else if (fixedSeed == 0)
			seedMessage << "Using default fixed seed";
		else
			seedMessage << "Using fixed seed = " << fixedSeed;
		seedMessage << std::endl;
		std::string seedMessageStr = seedMessage.str();
		eventParams.displayOut(seedMessageStr.c_str());

		eventParams.monthOf1990 = simParams->FirstChildElement("monthOf1990")->GetText<int>();

		int proportionYear = 2002;
		ticpp::Iterator<ticpp::Element> proportionIterator;
		for(proportionIterator = proportionIterator.begin(simParams->FirstChildElement("targetRolloutProportions"));
			proportionIterator != proportionIterator.end();
			++proportionIterator)
		{
			int year = boost::lexical_cast<int>(proportionIterator.Get()->GetAttribute("year"));
			assert(year == proportionYear++);
			eventParams.targetYearlyRolloutProportions.push_back(proportionIterator.Get()->GetText<double>());
		}

		double inputVersion = simParams->FirstChildElement("inputVersion")->GetText<double>();
		eventParams.displayOut("Input Version =");
		eventParams.displayOut(boost::lexical_cast<std::string>(inputVersion).c_str());
		eventParams.displayOut("\n");

		if (inputVersion != Util::INPUT_VERSION)
		{
			eventParams.displayOut("Input Version for ");
			eventParams.displayOut(_paramsXML.c_str());
			eventParams.displayOut(" is not ");
			eventParams.displayOut(boost::lexical_cast<std::string>(Util::INPUT_VERSION).c_str());
			eventParams.displayOut(".  Stopping model execution!\n");

			XMLerror = true;
			return;
		}

		//save Debug Level
		eventParams.debugLevel = DebugLevel(simParams->FirstChildElement("debugLevel")->GetText<int>());
		eventParams.displayOut("\tDebug Level = ");
		eventParams.displayOut(boost::lexical_cast<std::string>(eventParams.debugLevel).c_str());
		eventParams.displayOut("\n");

		//save Concurrency Definitions
		for (int i = 0; i < Constants::NUMBER_CONCURRENCY_DEFS; i++)
		{
			int minNeeded = simParams->FirstChildElement("concurrencyDefinition")->FirstChildElement("def" + boost::lexical_cast<std::string>(i))->FirstChildElement("minNeeded")->GetText<int>();
			bool useDef = simParams->FirstChildElement("concurrencyDefinition")->FirstChildElement("def" + boost::lexical_cast<std::string>(i))->FirstChildElement("allow")->GetText<int>() != 0;
			eventParams.concurrencyDef[i] = new EventParams::ConcurrencyDef(minNeeded, useDef);
		}

		//save which trace files to output
		std::string traceIDs[] = {"population", "infection", "partnership", "survival", "cost", "clinical", "events", "health", "singleperson", "le","partacq", "calibStats", "artRollout", "shiftedOutcomes"};
		for (int i = 0; i < Constants::NUMBER_OF_TRACE_FILES; i++)
		{
			eventParams.outputTrace[i] = simParams->FirstChildElement("writeTrace")->FirstChildElement(traceIDs[i])->GetText<int>() != 0;
			eventParams.traceExtensions[i] = simParams->FirstChildElement("extensionNames")->FirstChildElement(traceIDs[i])->GetText();
		}

		//save calibration inputs
		ticpp::Element *calibParams = simParams->FirstChildElement("calibration");
		eventParams.calibrationInputs.useCalibration = calibParams->FirstChildElement("useCalibration")->GetText<int>() != 0;

		if (eventParams.calibrationInputs.useCalibration)
		{
			eventParams.calibrationInputs.monthOfCalibration = calibParams->FirstChildElement("monthOfCalibration")->GetText<int>();

			ticpp::Element *outcomeParams = calibParams->FirstChildElement("partnershipOutcomes");
			eventParams.calibrationInputs.steadyPrevPopulation = outcomeParams->FirstChildElement("steadyPrev")->FirstChildElement("popOfInterest")->GetText<int>();
			eventParams.calibrationInputs.steadyPrevBounds[Constants::LOWER] = outcomeParams->FirstChildElement("steadyPrev")->FirstChildElement("lwrBound")->GetText<double>();
			eventParams.calibrationInputs.steadyPrevBounds[Constants::UPPER] = outcomeParams->FirstChildElement("steadyPrev")->FirstChildElement("uprBound")->GetText<double>();

			eventParams.calibrationInputs.casualPrevPopulation = outcomeParams->FirstChildElement("casualPrev")->FirstChildElement("popOfInterest")->GetText<int>();
			eventParams.calibrationInputs.casualPrevBounds[Constants::LOWER] = outcomeParams->FirstChildElement("casualPrev")->FirstChildElement("lwrBound")->GetText<double>();
			eventParams.calibrationInputs.casualPrevBounds[Constants::UPPER] = outcomeParams->FirstChildElement("casualPrev")->FirstChildElement("uprBound")->GetText<double>();

			eventParams.calibrationInputs.CSWPrevPopulation = outcomeParams->FirstChildElement("cswPrev")->FirstChildElement("popOfInterest")->GetText<int>();
			eventParams.calibrationInputs.CSWPrevBounds[Constants::LOWER] = outcomeParams->FirstChildElement("cswPrev")->FirstChildElement("lwrBound")->GetText<double>();
			eventParams.calibrationInputs.CSWPrevBounds[Constants::UPPER] = outcomeParams->FirstChildElement("cswPrev")->FirstChildElement("uprBound")->GetText<double>();

			eventParams.calibrationInputs.propInConcurrentPopulation = outcomeParams->FirstChildElement("propInCon")->FirstChildElement("popOfInterest")->GetText<int>();
			eventParams.calibrationInputs.propInConcurrentBounds[Constants::LOWER] = outcomeParams->FirstChildElement("propInCon")->FirstChildElement("lwrBound")->GetText<double>();
			eventParams.calibrationInputs.propInConcurrentBounds[Constants::UPPER] = outcomeParams->FirstChildElement("propInCon")->FirstChildElement("uprBound")->GetText<double>();

			eventParams.calibrationInputs.numActsPopulation = outcomeParams->FirstChildElement("numActs")->FirstChildElement("popOfInterest")->GetText<int>();
			eventParams.calibrationInputs.numActsBounds[Constants::LOWER] = outcomeParams->FirstChildElement("numActs")->FirstChildElement("lwrBound")->GetText<double>();
			eventParams.calibrationInputs.numActsBounds[Constants::UPPER] = outcomeParams->FirstChildElement("numActs")->FirstChildElement("uprBound")->GetText<double>();

			eventParams.calibrationInputs.femaleCasualPrevRatio = outcomeParams->FirstChildElement("femaleCasualPrev")->FirstChildElement("ratio")->GetText<double>();
			eventParams.calibrationInputs.femalePropInConcurrentRatio = outcomeParams->FirstChildElement("femalePropInCon")->FirstChildElement("ratio")->GetText<double>();
			eventParams.calibrationInputs.femaleNumActsLRtoHRRatio = outcomeParams->FirstChildElement("femaleNumActsLRtoHR")->FirstChildElement("ratio")->GetText<double>();

			for (int i = 0; i < Constants::NUMBER_OF_TRACE_FILES; i++)
			{
				eventParams.calibrationInputs.tossFiles[i] = calibParams->FirstChildElement("tossFiles")->FirstChildElement("traceFile" + boost::lexical_cast<std::string>(i))->GetText<int>() != 0;
			}

			for (int i = 0; i < Constants::NUMBER_CALIBRATION_PREVS; i++)
			{
				eventParams.calibrationInputs.calendarPrevs[i] = calibParams->FirstChildElement("calendarPrevalence")->FirstChildElement("time"+boost::lexical_cast<std::string>(i))->GetText<double>();
			}
			for (int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
			{
				eventParams.calibrationInputs.saveStateTimePoints[i] = calibParams->FirstChildElement("storePoint"+boost::lexical_cast<std::string>(i)+"Mth")->GetText<int>();
			}
			eventParams.calibrationInputs.thresholdPrevMult = calibParams->FirstChildElement("thresholdMultiplier")->GetText<double>();
		}

		if (isSeq)
		{
			//save run time for sequences
			seqRunTime = simParams->FirstChildElement("timeRunSeq")->GetText<int>();
			eventParams.displayOut("\tTime steps = ");
			eventParams.displayOut(boost::lexical_cast<std::string>(maxTime).c_str());
			eventParams.displayOut("\n");
		}
		else
		{
			//save simulation run time
			maxTime = simParams->FirstChildElement("timeLimitMth")->GetText<int>();
			seqRunTime = 0;//ignore sequence run time if running single file
			eventParams.displayOut("\tTime steps = ");
			eventParams.displayOut(boost::lexical_cast<std::string>(maxTime).c_str());
			eventParams.displayOut("\n");
		}
		delayPrevalence = simParams->FirstChildElement("population")->FirstChildElement("initialState")->FirstChildElement("delay")->GetText<int>();
		eventParams.delayPrevalence = delayPrevalence;
		eventParams.displayOut("\tDelay Prevalence = ");
		eventParams.displayOut(boost::lexical_cast<std::string>(delayPrevalence).c_str());
		eventParams.displayOut("\n");

		bool filesLoaded;

		//Load the CEPAC files
		if (simParams->FirstChildElement("population")->FirstChildElement("interventions")->FirstChildElement("artRolloutIntervention")->FirstChildElement("useRollout")->GetText<int>() == 1)
		{
			//use art rollout input files
			eventParams.useRollout = true;
			filesLoaded = setRolloutSimContexts(simParams->FirstChildElement("population")->FirstChildElement("interventions")->FirstChildElement("artRolloutIntervention"));
		}
		else
		{
			//use standard cepac input files
			eventParams.useRollout = false;
			filesLoaded = setCEPACSimContexts(simParams->FirstChildElement("population")->FirstChildElement("interventions")->FirstChildElement("cepacIntervention"));
		}

		if (!filesLoaded)
		{
			//If the files didn't successfully load, don't run the model!
			eventParams.displayOut("FILE ERROR: Stopping model execution for ");
			eventParams.displayOut(_paramsXML.c_str());
			eventParams.displayOut("\n");
			XMLerror = true;
			return;
		}

		//Set up CEPAC output (runStats)

		CepacUtil::setRandomSeedType(fixedSeed == -1);
		if (fixedSeed > -1)
		{
			//Seed is Minnesota Twins retired numbers... yes, I am a dork
			eventParams.randomNums.reset(fixedSeed == 0 ? 36291434 : fixedSeed);
		}

		if (eventParams.useRollout)
		{
			eventParams.cepacRunStats = new RunStats(eventParams.simName,eventParams.rolloutSimContexts[0]->rolloutSimContext);
		}
		else
		{
			eventParams.cepacRunStats = new RunStats(eventParams.simName, eventParams.cepacSimContexts[0]);
		}

		//Read in CEPAC death tables
		//TODO: Just use the data in SimContext?
		//Note: Moved this to Sim::setCEPACSimContexts
		/*ParseCepacInput cepacInput(cepacInputFile);
		cepacInput.getNonAIDSDeath(Person::probDeathNatCauses[DmgProfile::MALE], Person::probDeathNatCauses[DmgProfile::FEMALE]);*/

		//Setup up CEPAC traces (these will probably be unreadable, but oh well)
		if (eventParams.useRollout)
		{
			eventParams.cepacTracer = new Tracer(eventParams.simName, eventParams.rolloutSimContexts[0]->rolloutSimContext, 1);
		}
		else
		{
			eventParams.cepacTracer = new Tracer(eventParams.simName, eventParams.cepacSimContexts[0], 1);
		}

		//No longer creating a CEPAC trace file, but we still need to change over to the results folder before creating any other output files
		CepacUtil::changeDirectoryToResults();

		//If in the future it is decided to use the trace file again, uncomment out the "closeTraceFile" command in eventParams.h
		//eventParams.cepacTracer->openTraceFile();
		//eventParams.cepacTracer->printTraceHeader();

		//initialize the trace files for population and events and infections and costs
		for (int i = 0; i < Constants::NUMBER_OF_TRACE_FILES; i++)
		{
			if (eventParams.outputTrace[i])
			{
				std::string fileName = eventParams.simName;
				fileName.append("-" + eventParams.traceExtensions[i]);
				eventParams.traceStreams[i].open(fileName.c_str(), ios::out);
			}
		}

		eventParams.numToTrace = simParams->FirstChildElement("numberToTracePerAgeRange")->GetText<int>();
		eventParams.numNewbornsToTrace = simParams->FirstChildElement("numberNewbornsToTrace")->GetText<int>();
		eventParams.monthTraceNewborns = simParams->FirstChildElement("monthTraceNewborns")->GetText<int>();
		eventParams.numNewbornsTraced = 0;

		eventParams.tracePrevalentCases = simParams->FirstChildElement("tracePrevalentCases")->GetText<int>() != 0;

		if (eventParams.calibrationInputs.useCalibration)
		{
			for (int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
			{
				std::string fileName = eventParams.simName;
				fileName.append("-popState"+boost::lexical_cast<std::string>(i)+".pop");
				eventParams.popStateStream[i].open(fileName.c_str(),ios::out);
			}
		}
		//open the batchstats files that were requested by the user
		//in console version, print all batchstats
		BatchStatsVariables batchstat;
		for (batchstat = BatchStatsVariables(0); batchstat < ENDBatchStatsVariables; batchstat = BatchStatsVariables(batchstat + 1))
		{
#if !defined( CONSOLE )
			if (eventParams.displaybox->BatchStatsTrack[batchstat])
			{
#endif
				eventParams.BatchStatsStream[batchstat].open(("batchstats-" + Constants::BatchStatFileName[batchstat] + ".out").c_str(), ios::out | ios::app);
#if !defined( CONSOLE )
			}
#endif
		}

		//Open the summaryStats stream
		//eventParams.summaryStatsStream.open("summaryStats.out", ios::out | ios::app);

		//output seed used for this run
		if (eventParams.outputTrace[EventParams::EVENTS])
		{
			eventParams.traceStreams[EventParams::EVENTS] << "Seed = " << eventParams.randomNums.getSeed() << std::endl;
			eventParams.traceStreams[EventParams::EVENTS] << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << "Sexually Active Population" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << "Non Sexually Active Population" << std::endl;
		}
		currTime = 0;
		eventParams.currTime = currTime;

		//create a population
		// maybe someday we can have multiple interacting populations
		//  in that case, we'll have to change the PopulationParams to not put the values in the static Male, Female, and SteadyCouple fields
		currPopulation = new Population(eventParams, simParams->FirstChildElement("population"),simParams->FirstChildElement("lifeExpectancyOutput"), simParams->FirstChildElement("partnerAcqOutput"),maxTime);

		if (eventParams.monthOf1990 > 0)
		{
			currPopulation->popStats->enableShiftedOutcomes(eventParams.monthOf1990);
		}
	}
	catch(ticpp::Exception &_e)
	{
		eventParams.displayOut("Sim: Exception raised: ");
		eventParams.displayOut(_e.m_details.c_str());
		eventParams.displayOut("\n");
		XMLerror = true;
		return;
	}


	doc.Clear();
}

Sim::~Sim(void)
{
	eventParams.close();

	if (currPopulation != NULL)
	{
		delete currPopulation;
	}

	DmgProfile::deallocStaticMembers();
}


void Sim::run(int _numSteps)
{
	assert(_numSteps > 0);
	int stepsToRun = _numSteps;
	bool failsCalibration = false; //if fails calibration stop the run and delete specified trace files
	bool hasPassedFirstMonthCalibPrev = false;
	int monthOfFirstMonthCalibPrev = 0;

	//initialize/reset monthly stats
	currPopulation->resetMonthlyStats();

	//initialize incident infections by age
	currPopulation->initIncidentInfectionsByAge();

	if (delayPrevalence == 0)
	{
		currPopulation->applyIncidentPrevalence(eventParams);
	}

	//print out prevalent infection stats & headers for rest of infection stats
	currPopulation->calcPrevalentPopulation(0);

	//Print out run name for first column of BatchStats files (if streams are open)
	for (int i = 0; i < ENDBatchStatsVariables; i++)
	{
		if (eventParams.BatchStatsStream[i].is_open())
		{
			eventParams.BatchStatsStream[i] << eventParams.simName << Constants::TAB;
		}
	}

	if (eventParams.outputTrace[EventParams::INFECTION])
	{
		currPopulation->popStats->infectionsTracker.printInfections(eventParams, currTime, eventParams.traceStreams[EventParams::INFECTION], currPopulation);
	}
	if (eventParams.outputTrace[EventParams::PARTNERSHIP])
	{
		currPopulation->printPartnerships(eventParams, currTime, eventParams.traceStreams[EventParams::PARTNERSHIP]);
	}
	if (eventParams.outputTrace[EventParams::CLINICAL])
	{
		currPopulation->printClinical(eventParams, currTime, eventParams.traceStreams[EventParams::CLINICAL]);
	}
	if (eventParams.outputTrace[EventParams::POPULATION])
	{
		currPopulation->printPopulation(eventParams, currTime,eventParams.traceStreams[EventParams::POPULATION]);
	}
	if(eventParams.outputTrace[EventParams::ARTROLLOUT])
	{
		currPopulation->printARTRolloutOutcomes(eventParams, eventParams.traceStreams[EventParams::ARTROLLOUT]);
	}

	if (eventParams.debugLevel == DEBUG1)
	{
		currPopulation->printMethodResults(eventParams,"--", "initialization", 0, "--", Constants::SHOW_INFECTED);
	}
	//loop for multiple input files
	do
	{
		int startTime = currTime+1;
		if (isSeq)
		{
			stepsToRun = seqRunTime;
		}

		//ERINWASHERE
		//GRAPHVIZHERE
		/*if (eventParams.genGraphViz)
		currPopulation->printGraphVizNodes(eventParams); */

		SimulationTimer timer;
		timer.Start();

		//this is the main loop for each timestep
		for (int t = startTime; t<= stepsToRun+startTime-1; t++)
		{
			//this is used measure the general compute performance of the simulation
			//  records the time that the timestep was started
			double begin = timer.GetTime();

			if (eventParams.useRollout)
			{
				currPopulation->applyRolloutContext(eventParams, t);
			}

			long totalSize = timeStep();

			//keeps track of the time it takes to run 1 timestep of this model
			double end = timer.GetTime();
			std::ostringstream elapsedStringStream;
			elapsedStringStream.precision(3);
			elapsedStringStream << std::fixed << (end - begin);
			std::string elapsedString = elapsedStringStream.str();

			//int timeElapsed = end - begin;
			eventParams.displayOut("Timestep(");
			std::string timeString = boost::lexical_cast<std::string>(t);
			eventParams.displayOut(timeString.c_str());
			eventParams.displayOut("): compute time elapsed = ");
			eventParams.displayOut(elapsedString.c_str());
			eventParams.displayOut(". size = ");
			eventParams.displayOut(boost::lexical_cast<std::string>(totalSize).c_str());
			eventParams.displayOut("\n");

			//print out new infection stats
			this->currPopulation->calcPrevalentPopulation(t);
			int prev = 0;
			if (this->eventParams.outputTrace[EventParams::INFECTION])
			{
				prev = this->currPopulation->popStats->infectionsTracker.printInfections(this->eventParams, t, this->eventParams.traceStreams[EventParams::INFECTION], this->currPopulation);
			}
			if (this->eventParams.outputTrace[EventParams::POPULATION])
			{
				this->currPopulation->printPopulation(this->eventParams, t,this->eventParams.traceStreams[EventParams::POPULATION]);
			}
			if (this->eventParams.outputTrace[EventParams::PARTNERSHIP])
			{
				this->currPopulation->printPartnerships(this->eventParams, this->currTime, this->eventParams.traceStreams[EventParams::PARTNERSHIP]);
			}
			if (this->eventParams.outputTrace[EventParams::CLINICAL])
			{
				this->currPopulation->printClinical(this->eventParams,this->currTime, this->eventParams.traceStreams[EventParams::CLINICAL]);
			}
			if(eventParams.outputTrace[EventParams::SHIFTEDOUTCOMES]) //For now, this must come after infectionsTracker.printInfections as it is what calculate prevalence
			{
				currPopulation->recordShiftedOutcomes(eventParams, eventParams.traceStreams[EventParams::SHIFTEDOUTCOMES]);
			}
			if(eventParams.outputTrace[EventParams::ARTROLLOUT])
			{
				currPopulation->printARTRolloutOutcomes(eventParams, eventParams.traceStreams[EventParams::ARTROLLOUT]);
			}

			if (this->eventParams.calibrationInputs.useCalibration && this->eventParams.calibrationInputs.monthOfCalibration == t)
			{
				//If this run doesn't pass the partnership calibration stop the run and discard specified trace files
				if (!this->currPopulation->passesPartnershipCalibration(this->eventParams))
				{
					failsCalibration = true;
					break;
				}
			}

			if (this->eventParams.calibrationInputs.useCalibration)
			{
				if (!hasPassedFirstMonthCalibPrev)
				{
					double SAPrev = this->currPopulation->popStats->infectionsTracker.getSAPrev(this->currPopulation);
					if (SAPrev != -1 && SAPrev >= this->eventParams.calibrationInputs.thresholdPrevMult*this->eventParams.calibrationInputs.calendarPrevs[0])
					{
						hasPassedFirstMonthCalibPrev = true;
						monthOfFirstMonthCalibPrev = this->eventParams.currTime;
					}
				}
				for (int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
				{
					if (hasPassedFirstMonthCalibPrev && this->eventParams.currTime == monthOfFirstMonthCalibPrev+this->eventParams.calibrationInputs.saveStateTimePoints[i])
					{
						this->currPopulation->saveState(this->eventParams.popStateStream[i],this->eventParams.currTime);
					}
				}
			}

#ifndef CONSOLE
			this->eventParams.displaybox->currPrev = prev;
			this->eventParams.displaybox->prevalenceWidget->Update();
			this->eventParams.displaybox->prevalenceWidget->Refresh();
			wxYield();

			this->eventParams.displaybox->incidenceWidget->Update();
			this->eventParams.displaybox->incidenceWidget->Refresh();
			wxYield();

			this->eventParams.displaybox->currentRunProgress = (100.0 * t)/_numSteps + 0.5;
			this->eventParams.displaybox->singleProgressWidget->Update();
			this->eventParams.displaybox->singleProgressWidget->Refresh();
			wxYield();
#endif
			this->currPopulation->resetMonthlyStats();
		}//for (int t = 1; t<= _numSteps; t++ ) {

		if (failsCalibration)
		{
			break;
		}
	} while (this->loadNextInput()); //end of do loop

	//print survival statistics
	if (this->eventParams.outputTrace[EventParams::SURVIVAL])
	{
		this->currPopulation->popStats->printSurvivalStats(this->eventParams.traceStreams[EventParams::SURVIVAL]);
	}

	//Run every infected person left through CEPAC until they die
	if (failsCalibration)
	{
		this->eventParams.displayOut("Partnership Calibration Failed...Deleting specified trace files...\n");
	}
	else
	{
		this->eventParams.displayOut("Running all remaining persons through CEPAC until they die...\n");
		this->currPopulation->updateFinalPhysicalState(this->eventParams);
		this->eventParams.displayOut("Done!\n");
	}

	if (this->eventParams.genGraphViz)
	{
		this->currPopulation->graph->printGraphVizFiles(_numSteps, this->eventParams.simName);
	}

	if (this->eventParams.outputTrace[EventParams::INFECTION])
	{
		this->currPopulation->popStats->printLMStats(this->eventParams.traceStreams[EventParams::INFECTION]);
	}

	//Print out all of the costs
	if (this->eventParams.outputTrace[EventParams::COST])
	{
		this->currPopulation->popStats->costsTracker.printCosts(this->eventParams.traceStreams[EventParams::COST]);
	}

	//finalize and print CEPAC output, but only if at least one patient went through CEPAC
	try
	{
		if (this->eventParams.cepacRunStats->getPopulationSummary()->numCohorts > 0)
		{
			this->eventParams.cepacRunStats->finalizeStats();
			this->eventParams.cepacRunStats->writeStatsFile();
		}
	}
	catch (std::string errorString)
	{
		this->eventParams.displayOut(errorString.c_str());
		this->eventParams.displayOut("\n");
	}

	//if failed partnership calibration toss unneeded files
	if (failsCalibration)
	{
		for(int i = 0; i < Constants::NUMBER_OF_TRACE_FILES; i++)
		{
			if (this->eventParams.calibrationInputs.tossFiles[i]){
				this->eventParams.traceStreams[i].close();
				std::string fileName = this->eventParams.simName;
				fileName.append("-" + this->eventParams.traceExtensions[i]);
				remove(fileName.c_str());
			}
		}
		for(int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
		{
			std::string fileName = this->eventParams.simName;
			fileName.append("-popState"+boost::lexical_cast<std::string>(i)+".pop");
			remove(fileName.c_str());
		}
	}

}
/*
*	This function loads the next input file for use in a sequence 
*	returns false if no next input or if not a sequence
*/
bool Sim::loadNextInput()
{
	if (!(this->isSeq) || seqPos>=numInSeq)
	{
		return false;
	}

	seqPos++;
	std::string positionString = boost::lexical_cast<std::string>(seqPos / 10)+boost::lexical_cast<std::string>(seqPos % 10);
	std::ifstream inputFile;

	std::string filename = "../" + this->eventParams.simName + "_seq" + positionString + ".xml";
	ticpp::Document doc(filename);
	try
	{
		doc.LoadFile();

		this->eventParams.displayOut("Simulation Parameters\n");

		ticpp::Element *simParams = doc.FirstChildElement("simulation");

		//save run time for sequences
		this->seqRunTime = simParams->FirstChildElement("timeRunSeq")->GetText<int>();
		this->maxTime = 0;//ignore max time if running sequence
		this->eventParams.displayOut("\tTime steps = ");
		this->eventParams.displayOut(boost::lexical_cast<std::string>(maxTime).c_str());
		this->eventParams.displayOut("\n");

		//create a population
		// maybe someday we can have multiple interacting populations
		//  in that case, we'll have to change the PopulationParams to not put the values in the static Male, Female, and SteadyCouple fields
		this->currPopulation->updatePopulation(this->eventParams,simParams->FirstChildElement("population"));

	}
	catch(ticpp::Exception &_e)
	{
		this->eventParams.displayOut("Sim: Exception raised: ");
		this->eventParams.displayOut(_e.m_details.c_str());
		this->eventParams.displayOut("\n");
		this->XMLerror = true;
	}

	doc.Clear();

	return true;
}


/*
* This function records all of the SimContext files to be used by the CEPAC disease model throughout the run of the transmission model
*/

bool Sim::setCEPACSimContexts(ticpp::Element *cepacInterventionNode)
{
	this->eventParams.displayOut("CEPAC Files: \n");
	ticpp::Element *treatmentFilesNode = cepacInterventionNode->FirstChildElement("cepacTreatmentFiles");
	//Only iterates through Element nodes with value "ElementValue"
	ticpp::Iterator<ticpp::Element> treatmentFileNode("treatmentFile");
	for (treatmentFileNode = treatmentFileNode.begin(treatmentFilesNode); treatmentFileNode != treatmentFileNode.end(); treatmentFileNode++)
	{
		std::string fileName = (*treatmentFileNode).FirstChildElement("fileName")->GetText();
		int fileNumber = (*treatmentFileNode).FirstChildElement("fileNumber")->GetText<int>();
		//Make sure the number of CEPAC input files from the .xml file is not greater than the number expected by the code!
		assert(fileNumber < Constants::NUMBER_OF_CEPAC_FILES);
		this->eventParams.displayOut("\t");
		this->eventParams.displayOut(boost::lexical_cast<std::string>(fileNumber).c_str());
		this->eventParams.displayOut(": ");
		this->eventParams.displayOut(fileName.c_str());
		this->eventParams.displayOut("\n");
		//Set the CEPAC simContext from the specified CEPAC .in file
		this->eventParams.cepacSimContexts.push_back(new SimContext(fileName.substr(0, fileName.find(CepacUtil::FILE_EXTENSION_FOR_INPUT))));
		assert(this->eventParams.cepacSimContexts.size() == (static_cast<size_t>(fileNumber) + 1));
		//this->eventParams.cepacSimContext = new SimContext(cepacInputFile.substr(0, cepacInputFile.find(CepacUtil::FILE_EXTENSION_FOR_INPUT)));
		//Don't trace any CEPAC patients -- the output doesn't make any sense and it just gets overly large for no reason
		//TODO: The reason is because the CEPAC Patient number doesn't get updated until the patient dies: this should be changed!
		//this->eventParams.cepacSimContext->numPatientsToTrace = 0;
		this->eventParams.cepacSimContexts.at(fileNumber)->numPatientsToTrace = 0;

		//Read in the inputs
		try
		{
			this->eventParams.cepacSimContexts.back()->readInputs();
		}
		catch (std::string errorString)
		{
			//if we can't find it and we wanted to use CEPAC, display error
			this->eventParams.displayOut("*****************************************\n");
			this->eventParams.displayOut("WARNING!\n");
			this->eventParams.displayOut("*****************************************\n");
			this->eventParams.displayOut("File '");
			this->eventParams.displayOut(fileName.c_str());
			this->eventParams.displayOut("' generates error:\n\t");
			this->eventParams.displayOut(errorString.c_str());
			this->eventParams.displayOut("\n");
			this->XMLerror = true;
			return false;
		}

		//From the first file only, get the death tables for non-AIDS death
		if (fileNumber == 0)
		{
			ParseCepacInput cepacInput(fileName);
			cepacInput.getNonAIDSDeath(Person::probDeathNatCauses[DmgProfile::MALE], Person::probDeathNatCauses[DmgProfile::FEMALE]);
		}
	}

	//Get the times for the CEPAC input files to switch

	//TODO: Eventually there will be different times for different 'cohorts' i.e. subpopulations
	ticpp::Element *cohortTimesNode = cepacInterventionNode->FirstChildElement("cohortTimes")->FirstChildElement("cohort");
	for (int i = 0; i < Constants::NUMBER_OF_CEPAC_FILES; i++)
	{
		//By default, the first "time to switch" should be 0 (i.e. the first CEPAC .in file applies at time 0)
		if (i == 0)
		{
			this->eventParams.timesToSwitchSimContext[i] = 0;
		}
		else
		{
			std::string timeString = "time";
			timeString.append(boost::lexical_cast<std::string>(i));
			this->eventParams.timesToSwitchSimContext[i] = cohortTimesNode->FirstChildElement(timeString.c_str())->GetText<int>();
		}
		//std::cout << "Time " << i << ": " << this->eventParams.timesToSwitchSimContext[i] << "\n";
	}

	return true;
}

/*
* This function records all of the Rollout SimContext files to be used by the CEPAC disease model throughout the run of the transmission model
*/

bool Sim::setRolloutSimContexts(ticpp::Element *rolloutInterventionNode)
{
	this->eventParams.displayOut("Rollout CEPAC Files: \n");
	ticpp::Element *rolloutFilesNode = rolloutInterventionNode->FirstChildElement("rolloutTreatmentFiles");
	//Only iterates through Element nodes with value "ElementValue"
	ticpp::Iterator<ticpp::Element> rolloutFileNode("rolloutFile");

	for (rolloutFileNode = rolloutFileNode.begin(rolloutFilesNode); rolloutFileNode != rolloutFileNode.end(); rolloutFileNode++)
	{
		std::string fileName = (*rolloutFileNode).FirstChildElement("fileName")->GetTextOrDefault("");
		int fileNumber = (*rolloutFileNode).FirstChildElement("fileNumber")->GetText<int>();
		int popToApply;
		int timeToApply;

		(*rolloutFileNode).FirstChildElement("popToApply")->GetTextOrDefault<int>(&popToApply,-1);
		(*rolloutFileNode).FirstChildElement("time")->GetTextOrDefault<int>(&timeToApply,-1);

		//Make sure the number of CEPAC input files from the .xml file is not greater than the number expected by the code!
		assert(fileNumber < Constants::NUMBER_OF_ROLLOUT_FILES);
		this->eventParams.displayOut("\t");
		this->eventParams.displayOut(boost::lexical_cast<std::string>(fileNumber).c_str());
		this->eventParams.displayOut(": ");
		this->eventParams.displayOut(fileName.c_str());
		this->eventParams.displayOut("\n");
		//Set the CEPAC simContext from the specified CEPAC .in file
		SimContext *contextToAdd = NULL;
		if (fileName == "" || timeToApply == -1)
		{
			continue;
		}

		contextToAdd = new SimContext(fileName.substr(0, fileName.find(CepacUtil::FILE_EXTENSION_FOR_INPUT)));
		this->eventParams.rolloutSimContexts.push_back(new EventParams::RolloutContext(timeToApply,contextToAdd,popToApply));

		//Don't trace any CEPAC patients -- the output doesn't make any sense and it just gets overly large for no reason
		//TODO: The reason is because the CEPAC Patient number doesn't get updated until the patient dies: this should be changed!
		//this->eventParams.cepacSimContext->numPatientsToTrace = 0;
		this->eventParams.rolloutSimContexts.at(fileNumber)->rolloutSimContext->numPatientsToTrace = 0;

		//Read in the inputs
		try
		{
			this->eventParams.rolloutSimContexts.back()->rolloutSimContext->readInputs();
		}
		catch (std::string errorString)
		{
			//if we can't find it and we wanted to use CEPAC, display error
			this->eventParams.displayOut("*****************************************\n");
			this->eventParams.displayOut("WARNING!\n");
			this->eventParams.displayOut("*****************************************\n");
			this->eventParams.displayOut("File '");
			this->eventParams.displayOut(fileName.c_str());
			this->eventParams.displayOut("' generates error:\n\t");
			this->eventParams.displayOut(errorString.c_str());
			this->eventParams.displayOut("\n");
			this->XMLerror = true;
			return false;
		}

		//From the first file only, get the death tables for non-AIDS death
		if (fileNumber == 0)
		{ 
			ParseCepacInput cepacInput(fileName);
			cepacInput.getNonAIDSDeath(Person::probDeathNatCauses[DmgProfile::MALE], Person::probDeathNatCauses[DmgProfile::FEMALE]);
		}
	}

	ticpp::Element *eligNodes = rolloutInterventionNode->FirstChildElement("rolloutEligibility");
	ticpp::Iterator<ticpp::Element> criteriaNode("criteria");
	for (criteriaNode = criteriaNode.begin(eligNodes);criteriaNode!=criteriaNode.end();criteriaNode++)
	{
		std::string criteriaName = (*criteriaNode).FirstChildElement("name")->GetText();
		if (criteriaName == "OIHist")
		{
			this->eventParams.rolloutEligibility.oiHistRank=(*criteriaNode).FirstChildElement("rank")->GetText<int>();
			for (int i = 0; i<Constants::NUMBER_OF_OIS; i++)
			{
				std::string index_string = boost::lexical_cast<std::string, int>(i);
				bool enabled = (*criteriaNode).FirstChildElement("OI" + index_string)->GetText<int>() != 0;
				this->eventParams.rolloutEligibility.oiHistOIs[i] = enabled;
			}
			this->eventParams.rolloutEligibility.oiHistNumToStart = (*criteriaNode).FirstChildElement("numOIToStart")->GetText<int>();
		}
		else if (criteriaName == "CD4")
		{
			this->eventParams.rolloutEligibility.cd4Rank = (*criteriaNode).FirstChildElement("rank")->GetText<int>();
			this->eventParams.rolloutEligibility.cd4Bounds[Constants::LOWER] = (*criteriaNode).FirstChildElement("CD4Lwr")->GetText<int>();
			this->eventParams.rolloutEligibility.cd4Bounds[Constants::UPPER] = (*criteriaNode).FirstChildElement("CD4Upp")->GetText<int>();
		}
		else if (criteriaName == "CD4OIHist")
		{
			this->eventParams.rolloutEligibility.cd4OiHistRank = (*criteriaNode).FirstChildElement("rank")->GetText<int>();
			this->eventParams.rolloutEligibility.cd4OiHistCd4Bounds[Constants::LOWER] = (*criteriaNode).FirstChildElement("CD4Lwr")->GetText<int>();
			this->eventParams.rolloutEligibility.cd4OiHistCd4Bounds[Constants::UPPER] = (*criteriaNode).FirstChildElement("CD4Upp")->GetText<int>();
			for (int i = 0;i<Constants::NUMBER_OF_OIS;i++)
			{
				std::string index_string = boost::lexical_cast<std::string, int>(i);
				bool enabled = (*criteriaNode).FirstChildElement("OI" + index_string)->GetText<int>() != 0;
				this->eventParams.rolloutEligibility.cd4OiHistOIs[i] = enabled;
			}
		}
		else if (criteriaName == "HVL")
		{
			this->eventParams.rolloutEligibility.hvlRank = (*criteriaNode).FirstChildElement("rank")->GetText<int>();
			this->eventParams.rolloutEligibility.hvlBounds[Constants::LOWER] = (*criteriaNode).FirstChildElement("HVLLwr")->GetText<int>();
			this->eventParams.rolloutEligibility.hvlBounds[Constants::UPPER] = (*criteriaNode).FirstChildElement("HVLUpp")->GetText<int>();
		}
		else if (criteriaName == "CD4HVL")
		{
			this->eventParams.rolloutEligibility.cd4HvlRank = (*criteriaNode).FirstChildElement("rank")->GetText<int>();
			this->eventParams.rolloutEligibility.cd4HvlCd4Bounds[Constants::LOWER] = (*criteriaNode).FirstChildElement("CD4Lwr")->GetText<int>();
			this->eventParams.rolloutEligibility.cd4HvlCd4Bounds[Constants::UPPER] = (*criteriaNode).FirstChildElement("CD4Upp")->GetText<int>();
			this->eventParams.rolloutEligibility.cd4HvlHvlBounds[Constants::LOWER] = (*criteriaNode).FirstChildElement("HVLLwr")->GetText<int>();
			this->eventParams.rolloutEligibility.cd4HvlHvlBounds[Constants::UPPER] = (*criteriaNode).FirstChildElement("HVLUpp")->GetText<int>();
		}
	}

	return true;
}

/***
Sets the Non aids death from a cepac simcontext
***/
void Sim::setNonAidsDeathFromCepac(SimContext *cepacSimContext, std::vector<double> &_maleProbs , std::vector<double> &_femaleProbs)
{
	_maleProbs.clear();
	_femaleProbs.clear();
	for (int i = 0; i <= SimContext::AGE_YRS; i++)
	{
		_maleProbs.push_back(cepacSimContext->getNatHistInputs()->monthlyNonAIDSDeathProb[SimContext::GENDER_MALE][i]);
		_femaleProbs.push_back(cepacSimContext->getNatHistInputs()->monthlyNonAIDSDeathProb[SimContext::GENDER_FEMALE][i]);
	}
}

/***
This function executes one timestep of the simulation
The ordering of events within this function determines the ordering of events in each timestep
****/
int Sim::timeStep()
{
	currTime++;

	eventParams.currTime = currTime;

	//change non AIDS death if it is time to switch cepac files
	if(eventParams.itIsTimeToSwitchSimContext() && !eventParams.useRollout)
	{
		int simIndex = 0;
		for(int i = 0; i < Constants::NUMBER_OF_CEPAC_FILES; i++)
		{
			if(eventParams.currTime > eventParams.timesToSwitchSimContext[i])
			{
				simIndex = i;
			}
		}

		setNonAidsDeathFromCepac(eventParams.cepacSimContexts[simIndex], Person::probDeathNatCauses[DmgProfile::MALE], Person::probDeathNatCauses[DmgProfile::FEMALE]);
	}

	//output the current timestep of the simulation
	if(eventParams.debugLevel > DEBUG1 && eventParams.outputTrace[EventParams::EVENTS])
	{
		eventParams.traceStreams[EventParams::EVENTS] << "T:" << currTime << " : Start of Timestep" << std::endl;
	}

	if(eventParams.outputTrace[EventParams::SINGLEPERSON])
	{
		this->eventParams.traceStreams[EventParams::SINGLEPERSON] << std::endl << "** Time " << currTime << ": " << std::endl;
	}

	bool recordLE = false;
	bool recordPartAcq  =  false;
	bool firstMonthToRecord = false;
	bool lastMonthToRecord = false;

	if(currPopulation->popStats->isTimeToRecordLE(currTime))
	{
		recordLE = true;
	}
	if(currPopulation->popStats->isTimeToRecordPartAcq(currTime))
	{
		recordPartAcq = true;
	}

	if(currPopulation->popStats->isFirstMonthToRecordLE(currTime))
	{
		firstMonthToRecord = true;
	}

	if(currPopulation->popStats->isTimeToPrintLE(currTime))
	{
		lastMonthToRecord = true;
	}

	currPopulation->updatePhysicalState(eventParams, recordLE, firstMonthToRecord);

	if(eventParams.useRollout)
	{
		currPopulation->applyARTRollout(eventParams);
	}

	currPopulation->births(eventParams);

	if(lastMonthToRecord)
	{
		currPopulation->updateAgeBucketsLE();
		if(eventParams.outputTrace[EventParams::LE])
		{
			currPopulation->popStats->printLEStats(eventParams.traceStreams[EventParams::LE], currTime);
		}
		delete currPopulation->popStats->selectedLEStats;
		currPopulation->popStats->selectedLEStats = NULL;
	}

	//steadyCouple, flings, and dissolveSexualPartnerships
	currPopulation->updatePartnerships(eventParams);

	if (recordPartAcq)
	{
		currPopulation->popStats->selectedPartAcqStats = new PopStats::SinglePartAcqStats();
		currPopulation->recordPartAcqFreq();
		if (eventParams.outputTrace[EventParams::PARTACQ])
		{
			currPopulation->popStats->printPartAcqStats(eventParams.traceStreams[EventParams::PARTACQ], currTime);
		}
		delete currPopulation->popStats->selectedPartAcqStats;
		currPopulation->popStats->selectedPartAcqStats = NULL;
	}
	//apply incident prevalence
	if (delayPrevalence != 0 && delayPrevalence == currTime)
	{
		currPopulation->applyIncidentPrevalence(eventParams);
	}
	//Will confirm that currPopulation->currSize is correct and update size of age ranges
	return currPopulation->updateSize();
}

bool Sim::getError()
{
	return XMLerror;
}

int Sim::getMaxTime()
{
	return maxTime;
}

RunStats *Sim::getCEPACRunStats()
{
	return eventParams.cepacRunStats;
}

PopStats *Sim::getPopStats()
{
	return currPopulation->popStats;
}

EventParams *Sim::getEventParams()
{
	return &eventParams;
}
