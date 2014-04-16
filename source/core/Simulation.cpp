#include <iostream>
#include <set>
#include <boost/filesystem.hpp>

#include "Simulation.h"
#include "core/Constants.h"
#include "Population.h"
#include "../cepac44a/include.h"
#include "../util/CepacInputParser.h"
#include "data/EventParams.h"
#include "entities/classifiers/DemographicProfile.h"
#include "entities/behaviors/SexualBehavior.h"
#include "util/HighResolutionTimer.h"
#include "util/Utility.h"

PopulationTarget PopulationTarget::Any;

PopulationTarget PopulationTarget::FromString(const std::string &s)
{
	PopulationTarget target;

	std::stringstream ss(s);
	int i = 0;
	while(ss)
	{
		std::string line;
		std::getline(ss, line, ':');
		switch(i++)
		{
		case 0:
			target.risk_level.has_value = line != "*";
			if(target.risk_level.has_value)
			{
				target.risk_level.value = static_cast<Person::RiskLevel>(std::stoi(line));
			}
			break;
		case 1:
			target.employment.has_value = line != "*";
			if(target.employment.has_value)
			{
				target.employment.value = static_cast<DemographicProfile::Employment>(std::stoi(line));
			}
			break;
		case 2:
			target.sexual_activity_status.has_value = line != "*";
			if(target.sexual_activity_status.has_value)
			{
				target.sexual_activity_status.value = static_cast<DemographicProfile::SexualActivityStatus>(std::stoi(line));
			}
			break;
		case 3:
			target.gender.has_value = line != "*";
			if(target.gender.has_value)
			{
				target.gender.value = static_cast<DemographicProfile::Gender>(std::stoi(line));
			}
			break;
		case 4:
			target.relationship_status.has_value = line != "*";
			if(target.relationship_status.has_value)
			{
				target.relationship_status.value = static_cast<DemographicProfile::RelationshipStatus>(std::stoi(line));
			}
			break;
		case 5:
			target.sexual_orientation.has_value = line != "*";
			if(target.sexual_orientation.has_value)
			{
				target.sexual_orientation.value = static_cast<DemographicProfile::SexualOrientation>(std::stoi(line));
			}
			break;
		case 6:
			target.age_lower.has_value = line != "*";
			if(target.age_lower.has_value)
			{
				target.age_lower.value = std::stoi(line);
			}
			break;
		case 7:
			target.age_upper.has_value = line != "*";
			if(target.age_upper.has_value)
			{
				target.age_upper.value = std::stoi(line);
			}
			break;
		case 8:
			target.observed_hiv_status.has_value = line != "*";
			if(target.observed_hiv_status.has_value)
			{
				target.observed_hiv_status.value = static_cast<Person::HIVStatus>(std::stoi(line));
			}
			break;
		case 9:
			target.on_treatment.has_value = line != "*";
			if(target.on_treatment.has_value)
			{
				target.on_treatment.value = line[0] != 'f' && line[0] != 'n';
			}
			break;
		}
	}
	return target;
}

Simulation::Simulation()
    : time_(0),
      parameters_(),
      population_(parameters_),
      failedCalibration_(false),
      hasPassedFirstMonthCalibPrev_(false),
      monthOfFirstMonthCalibPrev_(0)
{
}

Simulation::~Simulation()
{
}

void Simulation::SetRolloutEligibilityRank(const std::string &criterion, int rank)
{
	if(criterion == "OIHist") parameters_.rolloutEligibility.oiHistRank = rank;
	else if(criterion == "CD4") parameters_.rolloutEligibility.cd4Rank = rank;
	else if(criterion == "CD4OIHist") parameters_.rolloutEligibility.cd4OiHistRank = rank;
	else if(criterion == "HVL") parameters_.rolloutEligibility.hvlRank = rank;
	else if(criterion == "CD4HVL") parameters_.rolloutEligibility.cd4HvlRank = rank;
	else throw std::runtime_error("bad criterion name");
}

void Simulation::RegisterSimulationIntervention(int time, SimulationIntervention intervention)
{
	auto time_parameters_iter = simulation_interventions.begin();
	while(time_parameters_iter != simulation_interventions.end() && time_parameters_iter->first < time)
	{
		time_parameters_iter++;
	}
	if(time_parameters_iter == simulation_interventions.end() || time_parameters_iter->first > time)
	{
		time_parameters_iter = simulation_interventions.emplace(time_parameters_iter, std::make_pair(time, std::vector<SimulationIntervention>()));
	}
	time_parameters_iter->second.push_back(intervention);
}

void Simulation::SetFixedSeed(int seed)
{
	fixedSeed_ = seed;

	CepacUtil::setRandomSeedType(seed == -1);

	if(seed > -1)
	{
		//Seed is Minnesota Twins retired numbers... yes, I am a dork
		parameters_.randomNums.reset(seed == 0 ? 36291434 : seed);
	}
}

void Simulation::FirstStep()
{
	//No longer creating a CEPAC trace file, but we still need to change over to the results folder before creating any other output files
	CepacUtil::changeDirectoryToResults();

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
		//parameters_.cepacSimContext = new SimContext(cepacInputFile.substr(0, cepacInputFile.find(CepacUtility::FILE_EXTENSION_FOR_INPUT)));
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
			Person::probDeathNatCauses[DemographicProfile::MALE] = probabilities[0];
			Person::probDeathNatCauses[DemographicProfile::FEMALE] = probabilities[1];
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

void Simulation::UpdateTimeDependentParameters()
{
	bool simulation_changed = false;

	if(!simulation_interventions.empty() && simulation_interventions.front().first == parameters_.currTime)
	{
		for(auto &intervention : simulation_interventions.front().second)
		{
			intervention(*this);
			simulation_changed = true;
		}
		simulation_interventions.pop_front();
	}

	if(simulation_changed)
	{
		ValidateState();
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

		SetNonAidsDeathFromCepac(*parameters_.cepacSimContexts[simIndex], Person::probDeathNatCauses[DemographicProfile::MALE], Person::probDeathNatCauses[DemographicProfile::FEMALE]);
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
		population_.popStats.selectedPartAcqStats = new PopulationStatistics::SinglePartAcqStats();
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

PopulationStatistics &Simulation::GetPopulationStatistics()
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
