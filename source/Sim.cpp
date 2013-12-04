#define TIXML_USE_TICPP
//Comment me out to create console version of model
//#define USEGUI

#include "Sim.h"

#include <time.h>
#include <string.h>
//ADDED FOR MULT FILES
#include <cstdlib>
#include <iostream>
#include <stdio.h>

#if defined(WIN32)
#include <io.h>
#endif
//END ADD
#include "./Constants.h"
#include "./Population.h"
#include "./cepacbridge/cepac_api.h"
#include "./cepacbridge/ParseCepacInput.h"
#include "./graphviz/graphVizParse.h"
#include "./data/EventParams.h"
#include "./entities/classifiers/DmgProfile.h"
#include "./util/Util.h"
#include "./cepac/include.h"
#include "boost/lexical_cast.hpp"


#include <set>


#ifdef _MSC_VER
// Use Visual C++'s memory checking functionality
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif // _MSC_VER


#if defined(WIN32) || defined(_WIN32) 
const char PATH_SEPARATOR = '\\';
#else 
const char PATH_SEPARATOR = '/';
#endif

class DisplayBox;

/*
void test()
{
    const std::string bucketLabel = "foo";
    int i;
    int j;

    BucketSexualMixing bsMix = BucketSexualMixing(26, &bucketLabel, 0, 2, 5, MONTH);

    std::vector<Female> vF;
    for (i = 2; i <= 5; i++)
    {
	for (j = 0; j < 3; j++)
	{
	    vF.push_back(Female(this->eventParams, i, false, 0));
	}
    }

    for (i = 0; i < vF.size(); i++)
    {
	bsMix.insert(&(vF[i]));
    }

    cout << "Size of bsMix is " << bsMix.size() << std::endl;
    cout << "Number of random marbles is " << bsMix.sizeRandom() << std::endl;
    cout << "Number of high risk marbles is " << bsMix.sizeRisk(Person::HIGH) << std::endl;
    cout << "Number of low risk marbles is " << bsMix.sizeRisk(Person::LOW) << std::endl;
    cout << "Number of random marbles (alt) is " << bsMix.sizeRisk(Person::ENDRiskLevel) << std::endl;

    if (bsMix.exists(&vF[6]))
    {
	std::cout << "We know female 6 is here!" << std::endl;
    }
    if (bsMix.erase(&vF[6]))
    {
	std::cout << "Female 6 is gone!" << std::endl;
    }
    if (bsMix.exists(&vF[6]))
    {
	std::cout << "Um, but she's still here... " << std::endl;
    }
    else
    {
	std::cout << "Yeah, she's really gone" << std::endl;
    }

    std::cout << "Size of bsMix is " << bsMix.size() << std::endl;
    std::cout << "Number of random marbles is " << bsMix.sizeRandom() << std::endl;
    std::cout << "Number of high risk marbles is " << bsMix.sizeRisk(Person::HIGH) << std::endl;
    std::cout << "Number of low risk marbles is " << bsMix.sizeRisk(Person::LOW) << std::endl;
    std::cout << "Number of random marbles (alt) is " << bsMix.sizeRisk(Person::ENDRiskLevel) << std::endl;

    std::cout << "Number of infected persons: " << bsMix.getNumInfected() << std::endl;
    std::cout << "Infecting female 3..." << std::endl;
    vF[3].becomeInfected(0, this->eventParams);
    bsMix.increaseInfected(&vF[3]);
    bsMix.increaseInfected(&vF[3]);
    bsMix.increaseInfected(&vF[6]);
    bsMix.increaseInfected(&vF[2]);
    std::cout << "Number of infected persons: " << bsMix.getNumInfected() << std::endl;

    if (bsMix.erase(&vF[3]))
    {
	std::cout << "Female 3 is gone!" << std::endl;
    }
    std::cout << "Number of infected persons: " << bsMix.getNumInfected() << std::endl;
    bsMix.insert(&vF[3]);
    std::cout << "Female 3 is back!" << std::endl;
    if (bsMix.exists(&vF[3]))
    {
	std::cout << "Yes, we know she's back!" << std::endl;
    }
    else
    {
	std::cout << "Uh, we don't know she's back?" << std::endl;
    }
    std::cout << "Number of infected persons: " << bsMix.getNumInfected() << std::endl;
    if (bsMix.insert(&vF[3]))
    {
	std::cout << "Female 3 was added a second time?!?!" << std::endl;
    }
    std::cout << "Number of infected persons: " << bsMix.getNumInfected() << std::endl;

    Male m = Male(this->eventParams, 5, false, false, 0);
    std::cout << "A man was made!" << std::endl;
    std::cout << "He is " << m.getAge(MONTH) << " months old." << std::endl;

    //m.becomeInfected(0, this->eventParams);

    if (bsMix.insert(&m))
    {
	std::cout << "Uh... we totally added a man to this girl's night out. Not cool." << std::endl;
    }
    else
    {
	std::cout << "NO BOYS ALLOWED -- as it should be!" << std::endl;
    }

    if (bsMix.increaseInfected(&m))
    {
	std::cout << "Um, that should not have happened!" << std::endl;
    }
    else
    {
	std::cout << "Okay, cool, bsMix.numInfected++ didn't happen." << std::endl;
    }


    std::cout << "Number of infected persons: " << bsMix.getNumInfected() << std::endl;
*
  Person *p;
  for (i = 0; i < 11; i++){
  p = bsMix.drawMember(this->eventParams.randomNums, &m, SexualPartnership::STEADY, false);
  if (!(p == NULL)){
  std::cout << ": " << p->getID() << " :";
  }
  }
  std::cout << std::endl;
  std::cout << "Size of bsMix is " << bsMix.size() << std::endl;
  std::cout << "Number of random marbles is " << bsMix.sizeRandom() << std::endl;
  std::cout << "Number of high risk marbles is " << bsMix.sizeRisk(Person::HIGH) << std::endl;
  std::cout << "Number of low risk marbles is " << bsMix.sizeRisk(Person::LOW) << std::endl;
  std::cout << "Number of random marbles (alt) is " << bsMix.sizeRisk(Person::ENDRiskLevel) << std::endl;/

    std::cout << "*************************" << std::endl;
    //bsMix.print(std::cout, "Before aging: ");

    std::list<Person*> lP = bsMix.ageOneTimeStep();
    std::list<Person*>::iterator lPiter;
    for (lPiter = lP.begin(); lPiter != lP.end(); lPiter++)
    {
	(*lPiter)->print(std::cout, "SENT TO DIE: ");
    }
    //bsMix.print(std::cout, "Left in: ");

    *Person *p = bsMix.drawMember(this->eventParams.randomNums, &m, SexualPartnership::STEADY, false);
      m.print(std::cout, "Man is: ");

      p->print(std::cout, "He drew this person: ");*
    std::cout << "Size of bsMix is now " << bsMix.size() << std::endl;
}
*/

Sim::Sim(std::string _paramsXML, DisplayBox *dbox, bool _genGraphViz)
{
#if !defined( CONSOLE )
    //Setting the displaybox to write to for text controls
    //note: this->eventParams.displaybox->textctrl->Update() must be called each time you use the print out as a stream in order to update in real time
    //this is only used for GUI builds
    this->eventParams.displaybox = dbox;
#endif

    //Character to seperate directories / for linux, mac and \ for windows
    std::string dirSepChar = std::string(1, PATH_SEPARATOR);

    //Remove the .xml and the directory from _paramsXML to get the sim name
    this->eventParams.simName = _paramsXML.substr(_paramsXML.find_last_of(dirSepChar) + 1);
    this->eventParams.simName = this->eventParams.simName.substr(0, this->eventParams.simName.length() - 4);

    size_t simNameLength = this->eventParams.simName.length();
    size_t seqStartIndex = this->eventParams.simName.rfind("_seq01");

    seqPos = 1;//set the position in the sequence to 1
    numInSeq = 1; //initialize the total number of files in sequence
    maxTime = 0; //if sequence we will sum up the individual seq times to get the real max time

    if (seqStartIndex != std::string::npos)
    {
	//file is the start of a sequence
	isSeq = true;
	this->eventParams.simName = this->eventParams.simName.substr(0, simNameLength - std::string("_seq00").length());
	
	//count number of files in sequence
	while(true)
	{
	    std::string positionString = boost::lexical_cast<std::string>(numInSeq / 10) + boost::lexical_cast<std::string>(numInSeq % 10);	

	    std::ifstream inputFile;
	    //boost::filesystem::path fullpath(_paramsXML);
	    //boost::filesystem::path newpath = fullpath.parent_path()/ (this->eventParams.simName+"_seq"+positionString+".xml");
	    std::string filename = _paramsXML.substr(0, _paramsXML.find_last_of(dirSepChar)) + dirSepChar + this->eventParams.simName + "_seq" + positionString + ".xml";
#if defined( CONSOLE )
	    filename = this->eventParams.simName + "_seq" + positionString + ".xml";
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
		this->eventParams.displayOut("Sim: Exception raised: ");
		this->eventParams.displayOut(_e.m_details.c_str());
		this->eventParams.displayOut("\n");
		this->XMLerror = true;
		return;
	    }

	    numInSeq++;
	}//end while
    }
    else
    {
	isSeq = false;
    }


    this->eventParams.displayOut("Max time at begining");

    this->eventParams.displayOut(boost::lexical_cast<std::string>(this->maxTime).c_str());
    this->eventParams.displayOut("\n");


    this->eventParams.displayOut("Sim name is ");
    this->eventParams.displayOut(this->eventParams.simName.c_str());
    this->eventParams.displayOut("\n");

    this->XMLerror = false; //Assume by default that there won't be an XMLerror

    // Load a document
    ticpp::Document doc(_paramsXML);

    bool useFixedSeed = false;

    try
    {
	doc.LoadFile();

	this->eventParams.displayOut("Simulation Parameters\n");

	ticpp::Element *simParams = doc.FirstChildElement("simulation");

	useFixedSeed = simParams->FirstChildElement("enableFixedSeed", false)->GetText<int>();
	this->eventParams.displayOut(useFixedSeed ? "Using fixed seed" : "Not using fixed seed");

	double inputVersion = simParams->FirstChildElement("inputVersion")->GetText<double>();
	this->eventParams.displayOut("Input Version =");
	this->eventParams.displayOut(boost::lexical_cast<std::string>(inputVersion).c_str());
	this->eventParams.displayOut("\n");

	if (inputVersion != Util::INPUT_VERSION)
	{
	    this->eventParams.displayOut("Input Version for ");
	    this->eventParams.displayOut(_paramsXML.c_str());
	    this->eventParams.displayOut(" is not ");
	    this->eventParams.displayOut(boost::lexical_cast<std::string>(Util::INPUT_VERSION).c_str());
	    this->eventParams.displayOut(".  Stopping model execution!\n");

	    this->XMLerror = true;
	    return;
	}

	//save Debug Level
	this->eventParams.debugLevel = DebugLevel(simParams->FirstChildElement("debugLevel")->GetText<int>());
	this->eventParams.displayOut("\tDebug Level = ");
	this->eventParams.displayOut(boost::lexical_cast<std::string>(this->eventParams.debugLevel).c_str());
	this->eventParams.displayOut("\n");

	//save Concurrency Definitions
	for (int i = 0; i < Constants::NUMBER_CONCURRENCY_DEFS; i++)
	{
	    int minNeeded = simParams->FirstChildElement("concurrencyDefinition")->FirstChildElement("def" + boost::lexical_cast<std::string>(i))->FirstChildElement("minNeeded")->GetText<int>();
	    bool useDef = simParams->FirstChildElement("concurrencyDefinition")->FirstChildElement("def" + boost::lexical_cast<std::string>(i))->FirstChildElement("allow")->GetText<int>();
	    this->eventParams.concurrencyDef[i] = new EventParams::ConcurrencyDef(minNeeded, useDef);
	}

	//save which trace files to output
	std::string traceIDs[] = {"population", "infection", "partnership", "survival", "cost", "clinical", "events", "health", "singleperson", "le","partacq", "calibStats"};
	for (int i = 0; i < Constants::NUMBER_OF_TRACE_FILES; i++)
	{
	    this->eventParams.outputTrace[i] = (bool)simParams->FirstChildElement("writeTrace")->FirstChildElement(traceIDs[i])->GetText<int>();
	    this->eventParams.traceExtensions[i] = simParams->FirstChildElement("extensionNames")->FirstChildElement(traceIDs[i])->GetText();
	}

	//save calibration inputs
	ticpp::Element *calibParams = simParams->FirstChildElement("calibration");
	this->eventParams.calibrationInputs.useCalibration = (bool)calibParams->FirstChildElement("useCalibration")->GetText<int>();

	if (this->eventParams.calibrationInputs.useCalibration)
	{
	    this->eventParams.calibrationInputs.monthOfCalibration = calibParams->FirstChildElement("monthOfCalibration")->GetText<int>();
			
	    ticpp::Element *outcomeParams = calibParams->FirstChildElement("partnershipOutcomes");
	    this->eventParams.calibrationInputs.steadyPrevPopulation = outcomeParams->FirstChildElement("steadyPrev")->FirstChildElement("popOfInterest")->GetText<int>();
	    this->eventParams.calibrationInputs.steadyPrevBounds[Constants::LOWER] = outcomeParams->FirstChildElement("steadyPrev")->FirstChildElement("lwrBound")->GetText<double>();
	    this->eventParams.calibrationInputs.steadyPrevBounds[Constants::UPPER] = outcomeParams->FirstChildElement("steadyPrev")->FirstChildElement("uprBound")->GetText<double>();

	    this->eventParams.calibrationInputs.casualPrevPopulation = outcomeParams->FirstChildElement("casualPrev")->FirstChildElement("popOfInterest")->GetText<int>();
	    this->eventParams.calibrationInputs.casualPrevBounds[Constants::LOWER] = outcomeParams->FirstChildElement("casualPrev")->FirstChildElement("lwrBound")->GetText<double>();
	    this->eventParams.calibrationInputs.casualPrevBounds[Constants::UPPER] = outcomeParams->FirstChildElement("casualPrev")->FirstChildElement("uprBound")->GetText<double>();

	    this->eventParams.calibrationInputs.CSWPrevPopulation = outcomeParams->FirstChildElement("cswPrev")->FirstChildElement("popOfInterest")->GetText<int>();
	    this->eventParams.calibrationInputs.CSWPrevBounds[Constants::LOWER] = outcomeParams->FirstChildElement("cswPrev")->FirstChildElement("lwrBound")->GetText<double>();
	    this->eventParams.calibrationInputs.CSWPrevBounds[Constants::UPPER] = outcomeParams->FirstChildElement("cswPrev")->FirstChildElement("uprBound")->GetText<double>();

	    this->eventParams.calibrationInputs.propInConcurrentPopulation = outcomeParams->FirstChildElement("propInCon")->FirstChildElement("popOfInterest")->GetText<int>();
	    this->eventParams.calibrationInputs.propInConcurrentBounds[Constants::LOWER] = outcomeParams->FirstChildElement("propInCon")->FirstChildElement("lwrBound")->GetText<double>();
	    this->eventParams.calibrationInputs.propInConcurrentBounds[Constants::UPPER] = outcomeParams->FirstChildElement("propInCon")->FirstChildElement("uprBound")->GetText<double>();
			
	    this->eventParams.calibrationInputs.numActsPopulation = outcomeParams->FirstChildElement("numActs")->FirstChildElement("popOfInterest")->GetText<int>();
	    this->eventParams.calibrationInputs.numActsBounds[Constants::LOWER] = outcomeParams->FirstChildElement("numActs")->FirstChildElement("lwrBound")->GetText<double>();
	    this->eventParams.calibrationInputs.numActsBounds[Constants::UPPER] = outcomeParams->FirstChildElement("numActs")->FirstChildElement("uprBound")->GetText<double>();

	    this->eventParams.calibrationInputs.femaleCasualPrevRatio = outcomeParams->FirstChildElement("femaleCasualPrev")->FirstChildElement("ratio")->GetText<double>();
	    this->eventParams.calibrationInputs.femalePropInConcurrentRatio = outcomeParams->FirstChildElement("femalePropInCon")->FirstChildElement("ratio")->GetText<double>();
	    this->eventParams.calibrationInputs.femaleNumActsLRtoHRRatio = outcomeParams->FirstChildElement("femaleNumActsLRtoHR")->FirstChildElement("ratio")->GetText<double>();

	    for (int i = 0; i < Constants::NUMBER_OF_TRACE_FILES; i++)
	    {
		this->eventParams.calibrationInputs.tossFiles[i] = (bool)calibParams->FirstChildElement("tossFiles")->FirstChildElement("traceFile" + boost::lexical_cast<std::string>(i))->GetText<int>();
	    }

	    for (int i = 0; i < Constants::NUMBER_CALIBRATION_PREVS; i++)
	    {
		this->eventParams.calibrationInputs.calendarPrevs[i] = calibParams->FirstChildElement("calendarPrevalence")->FirstChildElement("time"+boost::lexical_cast<std::string>(i))->GetText<double>();
	    }
	    for (int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
	    {
		this->eventParams.calibrationInputs.saveStateTimePoints[i] = calibParams->FirstChildElement("storePoint"+boost::lexical_cast<std::string>(i)+"Mth")->GetText<int>();
	    }
	    this->eventParams.calibrationInputs.thresholdPrevMult = calibParams->FirstChildElement("thresholdMultiplier")->GetText<double>();
	}

	if (isSeq)
	{
	    //save run time for sequences
	    this->seqRunTime = simParams->FirstChildElement("timeRunSeq")->GetText<int>();
	    this->eventParams.displayOut("\tTime steps = ");
	    this->eventParams.displayOut(boost::lexical_cast<std::string>(maxTime).c_str());
	    this->eventParams.displayOut("\n");
	}
	else
	{
	    //save simulation run time
	    this->maxTime = simParams->FirstChildElement("timeLimitMth")->GetText<int>();
	    this->seqRunTime = 0;//ignore sequence run time if running single file
	    this->eventParams.displayOut("\tTime steps = ");
	    this->eventParams.displayOut(boost::lexical_cast<std::string>(maxTime).c_str());
	    this->eventParams.displayOut("\n");
	}
	this->delayPrevalence = simParams->FirstChildElement("population")->FirstChildElement("initialState")->FirstChildElement("delay")->GetText<int>();
	this->eventParams.delayPrevalence = this->delayPrevalence;
	this->eventParams.displayOut("\tDelay Prevalence = ");
	this->eventParams.displayOut(boost::lexical_cast<std::string>(delayPrevalence).c_str());
	this->eventParams.displayOut("\n");

	bool filesLoaded;

	//Load the CEPAC files
	if (simParams->FirstChildElement("population")->FirstChildElement("interventions")->FirstChildElement("artRolloutIntervention")->FirstChildElement("useRollout")->GetText<int>() == 1)
	{
	    //use art rollout input files
	    eventParams.useRollout = true;
	    filesLoaded = this->setRolloutSimContexts(simParams->FirstChildElement("population")->FirstChildElement("interventions")->FirstChildElement("artRolloutIntervention"));
	}
	else
	{
	    //use standard cepac input files
	    eventParams.useRollout = false;
	    filesLoaded = this->setCEPACSimContexts(simParams->FirstChildElement("population")->FirstChildElement("interventions")->FirstChildElement("cepacIntervention"));
	}

	if (!filesLoaded)
	{
	    //If the files didn't successfully load, don't run the model!
	    this->eventParams.displayOut("FILE ERROR: Stopping model execution for ");
	    this->eventParams.displayOut(_paramsXML.c_str());
	    this->eventParams.displayOut("\n");
	    this->XMLerror = true;
	    return;
	}

	//Set up CEPAC output (runStats)

	//if using fixed seed, reset the random number generator
	CepacUtil::setRandomSeedType(!useFixedSeed);
	if (useFixedSeed)
	{
	    //Seed is Minnesota Twins retired numbers... yes, I am a dork
	    this->eventParams.randomNums.reset(36291434);
	}
	if (this->eventParams.useRollout)
	{
	    this->eventParams.cepacRunStats = new RunStats(this->eventParams.simName,this->eventParams.rolloutSimContexts[0]->rolloutSimContext);
	}
	else
	{
	    this->eventParams.cepacRunStats = new RunStats(this->eventParams.simName, this->eventParams.cepacSimContexts[0]);
	}
		
	//Read in CEPAC death tables
	//TODO: Just use the data in SimContext?
	//Note: Moved this to Sim::setCEPACSimContexts
	/*ParseCepacInput cepacInput(cepacInputFile);
	  cepacInput.getNonAIDSDeath(Person::probDeathNatCauses[DmgProfile::MALE], Person::probDeathNatCauses[DmgProfile::FEMALE]);*/

	//Setup up CEPAC traces (these will probably be unreadable, but oh well)
	if (this->eventParams.useRollout)
	{
	    this->eventParams.cepacTracer = new Tracer(this->eventParams.simName, this->eventParams.rolloutSimContexts[0]->rolloutSimContext, 1);
	}
	else
	{
	    this->eventParams.cepacTracer = new Tracer(this->eventParams.simName, this->eventParams.cepacSimContexts[0], 1);
	}
		
	//No longer creating a CEPAC trace file, but we still need to change over to the results folder before creating any other output files
	CepacUtil::changeDirectoryToResults();

	//If in the future it is decided to use the trace file again, uncomment out the "closeTraceFile" command in eventParams.h
	//this->eventParams.cepacTracer->openTraceFile();
	//this->eventParams.cepacTracer->printTraceHeader();

	//initialize the trace files for population and events and infections and costs
	for (int i = 0; i < Constants::NUMBER_OF_TRACE_FILES; i++)
	{
	    if (this->eventParams.outputTrace[i])
	    {
		std::string fileName = this->eventParams.simName;
		fileName.append("-" + this->eventParams.traceExtensions[i]);
		this->eventParams.traceStreams[i].open(fileName.c_str(), ios::out);
	    }
	}

	this->eventParams.numToTrace = simParams->FirstChildElement("numberToTracePerAgeRange")->GetText<int>();
	this->eventParams.numNewbornsToTrace = simParams->FirstChildElement("numberNewbornsToTrace")->GetText<int>();
	this->eventParams.monthTraceNewborns = simParams->FirstChildElement("monthTraceNewborns")->GetText<int>();
	this->eventParams.numNewbornsTraced = 0;

	if (this->eventParams.calibrationInputs.useCalibration)
	{
	    for (int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
	    {
		std::string fileName = this->eventParams.simName;
		fileName.append("-popState"+boost::lexical_cast<std::string>(i)+".pop");
		this->eventParams.popStateStream[i].open(fileName.c_str(),ios::out);
	    }
	}
	//open the batchstats files that were requested by the user
	//in console version, print all batchstats
	BatchStatsVariables batchstat;
	for (batchstat = BatchStatsVariables(0); batchstat < ENDBatchStatsVariables; batchstat = BatchStatsVariables(batchstat + 1))
	{
#if !defined( CONSOLE )
	    if (this->eventParams.displaybox->BatchStatsTrack[batchstat])
	    {
#endif
		this->eventParams.BatchStatsStream[batchstat].open(("batchstats-" + Constants::BatchStatFileName[batchstat] + ".out").c_str(), ios::out | ios::app);
#if !defined( CONSOLE )
	    }
#endif
	}

	//Open the summaryStats stream
	//this->eventParams.summaryStatsStream.open("summaryStats.out", ios::out | ios::app);

	//output seed used for this run
	if (this->eventParams.outputTrace[EventParams::EVENTS])
	{
	    this->eventParams.traceStreams[EventParams::EVENTS] << "Seed = " << this->eventParams.randomNums.getSeed() << std::endl;
	    this->eventParams.traceStreams[EventParams::EVENTS] << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << "Sexually Active Population" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << "Non Sexually Active Population" << std::endl;
	}
	this->currTime = 0;
	this->eventParams.currTime = this->currTime;

	//create a population
	// maybe someday we can have multiple interacting populations
	//  in that case, we'll have to change the PopulationParams to not put the values in the static Male, Female, and SteadyCouple fields
	this->currPopulation = new Population(this->eventParams, simParams->FirstChildElement("population"),simParams->FirstChildElement("lifeExpectancyOutput"), simParams->FirstChildElement("partnerAcqOutput"),this->maxTime);

	//GRAPHVIZHERE
	if (_genGraphViz)
	{
	    //TODO: We no longer care about starting conditions and don't use the old methods of generating graphs
	    this->eventParams.genGraphViz = true;
	}

    }
    catch(ticpp::Exception &_e)
    {
	this->eventParams.displayOut("Sim: Exception raised: ");
	this->eventParams.displayOut(_e.m_details.c_str());
	this->eventParams.displayOut("\n");
	this->XMLerror = true;
	return;
    }


    doc.Clear();
}

Sim::~Sim(void)
{
    this->eventParams.close();

    if (this->currPopulation != NULL)
    {
	delete this->currPopulation;
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
    this->currPopulation->resetMonthlyStats();

    //initialize incident infections by age
    this->currPopulation->initIncidentInfectionsByAge();

    if (this->delayPrevalence == 0)
    {
	this->currPopulation->applyIncidentPrevalence(eventParams);
    }

    //print out prevalent infection stats & headers for rest of infection stats
    this->currPopulation->calcPrevalentPopulation(0);

    //Print out run name for first column of BatchStats files (if streams are open)
    for (int i = 0; i < ENDBatchStatsVariables; i++)
    {
	if (this->eventParams.BatchStatsStream[i].is_open())
	{
	    this->eventParams.BatchStatsStream[i] << this->eventParams.simName << Constants::TAB;
	}
    }

    if (this->eventParams.outputTrace[EventParams::INFECTION])
    {
	this->currPopulation->popStats->infectionsTracker.printInfections(this->eventParams, this->currTime, this->eventParams.traceStreams[EventParams::INFECTION], this->currPopulation);
    }
    if (this->eventParams.outputTrace[EventParams::PARTNERSHIP])
    {
	this->currPopulation->printPartnerships(this->eventParams, this->currTime, this->eventParams.traceStreams[EventParams::PARTNERSHIP]);
    }
    if (this->eventParams.outputTrace[EventParams::CLINICAL])
    {
	this->currPopulation->printClinical(this->eventParams, this->currTime, this->eventParams.traceStreams[EventParams::CLINICAL]);
    }
    if (this->eventParams.outputTrace[EventParams::POPULATION])
    {
	this->currPopulation->printPopulation(this->eventParams, this->currTime,this->eventParams.traceStreams[EventParams::POPULATION]);
    }

    if (this->eventParams.debugLevel == DEBUG1)
    {
	this->currPopulation->printMethodResults(eventParams,"--", "initialization", 0, "--", Constants::SHOW_INFECTED);
    }
    //loop for multiple input files
    do
    {
	int startTime = this->currTime+1;
	if (isSeq)
	{
	    stepsToRun = this->seqRunTime;
	}

	//ERINWASHERE
	//GRAPHVIZHERE
	/*if (this->eventParams.genGraphViz)
	  this->currPopulation->printGraphVizNodes(this->eventParams); */
 
	//this is the main loop for each timestep
	for (int t = startTime; t<= stepsToRun+startTime-1; t++)
	{
	    //this is used measure the general compute performance of the simulation
	    //  records the time that the timestep was started
	    time_t begin = time (NULL);

	    if (this->eventParams.useRollout)
	    {
		//if using art rollout switch the default cepac files for the treated and untreated pools
		this->currPopulation->applyRolloutContext(this->eventParams,this->currTime);
	    }
	    long totalSize = this->timeStep();

	    //keeps track of the time it takes to run 1 timestep of this model
	    time_t end = time (NULL);
	    //int timeElapsed = end - begin;
	    this->eventParams.displayOut("Timestep(");
	    std::string timeString = boost::lexical_cast<std::string>(t);
	    this->eventParams.displayOut(timeString.c_str());
	    this->eventParams.displayOut("): compute time elapsed = ");
	    this->eventParams.displayOut(boost::lexical_cast<std::string>((int)(end - begin)).c_str());
	    this->eventParams.displayOut(". size = ");
	    this->eventParams.displayOut(boost::lexical_cast<std::string>(totalSize).c_str());
	    this->eventParams.displayOut("\n");

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

#if !defined( CONSOLE )
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
	assert(this->eventParams.cepacSimContexts.size() == fileNumber + 1);
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

    ticpp::Element *rolloutTimesNode = rolloutInterventionNode->FirstChildElement("rolloutTimeToApply");
    //Only iterates through Element nodes with value "ElementValue"
    ticpp::Iterator<ticpp::Element> rolloutTimeNode("treatmentTime");

    for (rolloutTimeNode = rolloutTimeNode.begin(rolloutTimesNode); rolloutTimeNode != rolloutTimeNode.end(); rolloutTimeNode++)
    {
	int timeToApply;
	double proportionOfPopulation;
	(*rolloutTimeNode).FirstChildElement("time")->GetTextOrDefault<int>(&timeToApply,-1);
	(*rolloutTimeNode).FirstChildElement("propOfPop")->GetTextOrDefault<double>(&proportionOfPopulation,0.0);

	this->eventParams.rolloutTimes.push_back(new EventParams::RolloutTime(timeToApply,proportionOfPopulation));
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
		this->eventParams.rolloutEligibility.oiHistOIs[i] = boost::lexical_cast<bool,int>((*criteriaNode).FirstChildElement("OI" + boost::lexical_cast<std::string, int>(i))->GetText<int>());
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
	    for (int i = 0;i<Constants::NUMBER_OF_OIS;i++){
		this->eventParams.rolloutEligibility.cd4OiHistOIs[i] = boost::lexical_cast<bool,int>((*criteriaNode).FirstChildElement("OI" + boost::lexical_cast<std::string,int>(i))->GetText<int>());
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
long Sim::timeStep()
{
    //advance the internal clock
    this->currTime++;

    this->eventParams.currTime = this->currTime;

    //change non AIDS death if it is time to switch cepac files
    if (this->eventParams.itIsTimeToSwitchSimContext() && !this->eventParams.useRollout)
    {
	int simIndex = 0;
	for (int i = 0; i < Constants::NUMBER_OF_CEPAC_FILES; i++)
	{
	    if (this->eventParams.currTime > this->eventParams.timesToSwitchSimContext[i])
	    {
		simIndex = i;
	    }
	}

	this->setNonAidsDeathFromCepac(this->eventParams.cepacSimContexts[simIndex], Person::probDeathNatCauses[DmgProfile::MALE], Person::probDeathNatCauses[DmgProfile::FEMALE]);
    }

    //output the current timestep of the simulation
    if (this->eventParams.debugLevel > DEBUG1 && this->eventParams.outputTrace[EventParams::EVENTS])
    {
	this->eventParams.traceStreams[EventParams::EVENTS] << "T:" << this->currTime << " : Start of Timestep" << std::endl;
    }

    if (this->eventParams.outputTrace[EventParams::SINGLEPERSON])
    {
	this->eventParams.traceStreams[EventParams::SINGLEPERSON] << std::endl << "** Time " << this->currTime << ": " << std::endl;
    }
	
    bool recordLE = false;
    bool recordPartAcq  =  false;
    bool firstMonthToRecord = false;
    bool lastMonthToRecord = false;
    if (this->currPopulation->popStats->isTimeToRecordLE(this->currTime))
    {
	recordLE = true;
    }
    if (this->currPopulation->popStats->isTimeToRecordPartAcq(this->currTime))
    {
	recordPartAcq = true;
    }

    if (this->currPopulation->popStats->isFirstMonthToRecordLE(this->currTime))
    {
	firstMonthToRecord = true;
    }

    if (this->currPopulation->popStats->isTimeToPrintLE(this->currTime))
    {
	lastMonthToRecord = true;
    }

    this->currPopulation->updatePhysicalState(this->eventParams,recordLE,firstMonthToRecord);
    if (this->eventParams.useRollout)
    {
	this->currPopulation->applyARTRollout(this->eventParams);
    }
    this->currPopulation->births(this->eventParams);

    if (lastMonthToRecord)
    {
	this->currPopulation->updateAgeBucketsLE();
	if (this->eventParams.outputTrace[EventParams::LE])
	{
	    this->currPopulation->popStats->printLEStats(this->eventParams.traceStreams[EventParams::LE],this->currTime);
	}
	delete this->currPopulation->popStats->selectedLEStats;
	this->currPopulation->popStats->selectedLEStats=NULL;
    }

    //steadyCouple, flings, and dissolveSexualPartnerships
    this->currPopulation->updatePartnerships(this->eventParams);

    if (recordPartAcq)
    {
	this->currPopulation->popStats->selectedPartAcqStats = new PopStats::SinglePartAcqStats();
	this->currPopulation->recordPartAcqFreq();
	if (this->eventParams.outputTrace[EventParams::PARTACQ])
	{
	    this->currPopulation->popStats->printPartAcqStats(this->eventParams.traceStreams[EventParams::PARTACQ], this->currTime);
	}
	delete this->currPopulation->popStats->selectedPartAcqStats;
	this->currPopulation->popStats->selectedPartAcqStats = NULL;
    }
    //apply incident prevalence
    if (this->delayPrevalence != 0 && this->delayPrevalence == this->currTime)
    {
	this->currPopulation->applyIncidentPrevalence(eventParams);
    }
    //Will confirm that this->currPopulation->currSize is correct and update size of age ranges
    return this->currPopulation->updateSize();

    return 0;
}

bool Sim::getError()
{
    return this->XMLerror;
}

long Sim::getMaxTime()
{
    return this->maxTime;
}

RunStats *Sim::getCEPACRunStats()
{
    return this->eventParams.cepacRunStats;
}

PopStats *Sim::getPopStats()
{
    return this->currPopulation->popStats;
}

EventParams *Sim::getEventParams()
{
    return &(this->eventParams);
}
