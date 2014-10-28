#include <iostream>
#include <numeric>
#include <set>
#include <include.h>

#include "simulation.hpp"
#include "constants.hpp"
#include "population.hpp"
#include "utility/cepacinputparser.hpp"
#include "parameters/eventparams.hpp"
#include "entities/demographicprofile.hpp"
#include "entities/sexualbehavior.hpp"
#include "utility/highresolutiontimer.hpp"
#include "utility/utility.hpp"

namespace transm {

TargetGroup::PopulationTarget TargetGroup::PopulationTarget::Any;

void Intervention::Apply(Simulation &simulation)
{
    if(simulation_intervention_)
    {
        simulation_intervention_(simulation);
    }
}

void Intervention::Apply(Population &population)
{
    if(population_intervention_)
    {
        population_intervention_(population);
    }
}

void Intervention::Apply(Population &population, Entity *person)
{
    if(population_individual_intervention_)
    {
        population_individual_intervention_(population, person);
    }
    else if(individual_intervention_)
    {
        individual_intervention_(person);
    }
}

void Intervention::Apply(Entity *person)
{
    if(individual_intervention_)
    {
        individual_intervention_(person);
    }
}

bool Intervention::IsActive(int current_time) const
{
    if(duration_ == -1)
    {
        return current_time >= time_;
    }

    return current_time >= time_ && current_time <= time_ + duration_;
}

bool Intervention::IsFirstMonth(int current_time) const
{
    return current_time == time_;
}

bool Intervention::IsCompleted(int current_time) const
{
    return current_time > time_ + duration_;
}

void TargetGroup::Update(Population &population, int current_time, 
    RandomNumberGenerator &rng, const std::unordered_set<Entity *> &dead_people)
{
    if(enrollment_period_.start > current_time)
    {
        return;
    }

    for(auto person : dead_people)
    {
        Remove(person);
    }

    auto match = [&](Entity *person)
    {
        if(target_.has_value)
        {
            if(target_.value.employment.has_value
                && target_.value.employment.value != person->getDemographicProfileVal<DemographicProfile::Employment>())
            {
                return false;
            }

            if(target_.value.gender.has_value
                && target_.value.gender.value != (DemographicProfile::Gender)person->getDemographicProfileVal(DemographicProfile::Demographic::Gender))
            {
                return false;
            }

            if(target_.value.relationship_status.has_value
                && target_.value.relationship_status.value != (DemographicProfile::RelationshipStatus)person->getDemographicProfileVal(DemographicProfile::Demographic::RelationshipStatus))
            {
                return false;
            }

            if(target_.value.sexual_activity_status.has_value
                && target_.value.sexual_activity_status.value != (DemographicProfile::SexualActivityStatus)person->getDemographicProfileVal(DemographicProfile::Demographic::SexualActivityStatus))
            {
                return false;
            }

            if(target_.value.sexual_orientation.has_value
                && target_.value.sexual_orientation.value != (DemographicProfile::SexualOrientation)person->getDemographicProfileVal(DemographicProfile::Demographic::SexualOrientation))
            {
                return false;
            }

            if(target_.value.age_lower.has_value
                && target_.value.age_lower.value > person->getAge(TimeGranularity::Month))
            {
                return false;
            }

            if(target_.value.age_upper.has_value
                && target_.value.age_upper.value < person->getAge(TimeGranularity::Month))
            {
                return false;
            }

            if(target_.value.observed_hiv_status.has_value
                && target_.value.observed_hiv_status.value != person->getHIVStatus())
            {
                return false;
            }

            if(target_.value.on_treatment.has_value
                && target_.value.on_treatment.value != person->isOnArt())
            {
                return false;
            }

            if(target_.value.circumcised.has_value
                && target_.value.circumcised.value != person->IsCircumcised())
            {
                return false;
            }

            if(target_.value.risk_level.has_value
                && target_.value.risk_level.value != person->getRiskLevel())
            {
                return false;
            }
        }

        return true; 
    };

    if((open_ && enrollment_period_.end >= current_time) || enrollment_period_.start == current_time)
    {
        std::vector<Entity *> people;

        for(auto person : population.Find(match))
        {
            if(!InGroup(person))
            {
                people.push_back(person);
            }
        }

        if(people.empty())
        {
            return;
        }

        if(partitions_.size() == 1 && partitions_.front().GetProportion() == 1)
        {
            std::for_each(people.begin(), people.end(), [&](Entity *p) { AssignToPartition(population, p, 0); });
        }
        else
        {
            auto generate_rand = [&](int i) { return rng.randInt() % i; };
            std::random_shuffle(people.begin(), people.end(), generate_rand);

            std::vector<std::pair<std::size_t, std::size_t>> partition_allocations;

            std::size_t num_allocated = 0;
            double sum_proportion = 0;

            for(int i = 0; i < (int)partitions_.size(); i++)
            {
                sum_proportion += partitions_[i].GetProportion();
                auto partition_allocation = (std::size_t)(partitions_[i].GetProportion() * people.size());

                if(partition_allocation != 0)
                {
                    partition_allocations.push_back(std::make_pair(i, partition_allocation));
                    num_allocated += partition_allocation;
                }
            }

            while(num_allocated < people.size())
            {
                double rand = rng.rand();
                std::size_t random_allocation_index = 0;

                for(auto &partition : partitions_)
                {
                    rand -= partition.GetProportion();

                    if(rand < 0)
                    {
                        break;
                    }

                    random_allocation_index++;
                }

                auto partition_allocations_iter = std::find_if(partition_allocations.begin(), partition_allocations.end(),
                    [=](const std::pair<std::size_t, std::size_t> &p) { return p.first == random_allocation_index; });

                if(partition_allocations_iter == partition_allocations.end())
                {
                    partition_allocations.push_back(std::make_pair(random_allocation_index, 1));
                }
                else
                {
                    partition_allocations_iter->second++;
                }

                num_allocated++;
            }

            auto person_iter = people.begin();

            while(!partition_allocations.empty())
            {
                int allocations_index = (int)(rng.randInt() % partition_allocations.size());
                auto partition_index = partition_allocations[allocations_index].first;

                if(partition_index < partitions_.size())
                {
                    AssignToPartition(population, *person_iter, (int)partition_index);
                }

                if(--partition_allocations[allocations_index].second == 0)
                {
                    partition_allocations.erase(partition_allocations.begin() + allocations_index);
                }

                person_iter++;
            }
        }
    }
}

Intervention::Intervention(int time, int duration) : time_(time), duration_(duration)
{
}

TargetGroup::TargetGroup(const std::string &label, int start, int end, bool open, bool permanent, Nullable<PopulationTarget> target)
    : enrollment_period_({start, end}),
      open_(open),
      permanent_effect_(permanent),
      label_(label),
      target_(target)
{

}

void TargetGroup::AddPartition(const std::string &label, bool trace, double proportion,
    std::vector<Intervention> interventions)
{
    Partition p;
    p.label_ = label;
    p.trace_ = trace;
    p.proportion_ = proportion;
    p.interventions_ = interventions;
    partitions_.push_back(p);
}

void Simulation::RegisterPopulationIntervention(const Intervention &intervention)
{
    interventions_.push_back(intervention);
}

Simulation::Simulation(BatchStatus &batch_status)
    : time_(0),
      parameters_(),
      population_(parameters_),
      failedCalibration_(false),
      hasPassedFirstMonthCalibPrev_(false),
      monthOfFirstMonthCalibPrev_(0),
      incidence_(0),
      prevalence_(0),
      batch_status_(batch_status)
{
}

Simulation::~Simulation()
{
}

void Simulation::SetFixedSeed(int seed)
{
    // Seed is Minnesota Twins retired numbers... yes, I am a dork (Erin, not Thomas!)
    seed = seed == 0 ? 36291434 : seed;

	if(seed < 0)
	{
        CepacUtil::setRandomSeedType(true);
        rng_seed_ = (uint32_t)time(0);
	}
    else
    {
        CepacUtil::setRandomSeedType(false);
        rng_seed_ = seed;
    }

    parameters_.randomNums.reset(rng_seed_);
}

void Simulation::FirstStep()
{
    run_time_predictor_.SetTotalMonths(duration_);

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

	for(auto batchstat : enum_iterator<BatchStatsVariables>())
	{
        auto filename = "batchstats-" + Constants::BatchStatFileName.at(batchstat) + ".out";
		parameters_.BatchStatsStream[batchstat].open(filename, ios::out | ios::app);
	}

	//output seed used for this run
	if(parameters_.trace_files[EventParams::TraceFile::Type::Events].enabled)
	{
		parameters_.trace_files[EventParams::TraceFile::Type::Events] << "Seed = " << parameters_.randomNums.getSeed() << std::endl;
		parameters_.trace_files[EventParams::TraceFile::Type::Events] << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
			Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << "Sexually Active Population"
			<< Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
			Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
			Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
			"Non Sexually Active Population" << std::endl;
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::ShiftedOutcomes].enabled)
	{
		population_.populationStatistics.enableShiftedOutcomes(parameters_.monthOf1990);
	}

    UpdateInterventions(population_.GetDeadPeopleThisMonth());

	//initialize/reset monthly stats
	population_.ResetMonthlyStats();
	//initialize incident infections by age
	population_.InitIncidentInfectionsByAge();

	if(parameters_.delayPrevalence == 0)
	{
		population_.ApplyIncidentPrevalence(parameters_);
	}

	//print out prevalent infection stats & headers for rest of infection stats
	population_.CalcPrevalentPopulation(0);

	//Print out run name for first column of BatchStats files (if streams are open)
    for(auto batchstat : enum_iterator<BatchStatsVariables>())
	{
		if(parameters_.BatchStatsStream[batchstat].is_open())
		{
			parameters_.BatchStatsStream[batchstat] << parameters_.simName << Constants::TAB;
		}
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::Infection].enabled)
	{
        population_.populationStatistics.infectionsTracker.printInfections(parameters_, time_, parameters_.trace_files[EventParams::TraceFile::Type::Infection].file, &population_);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::Partnership].enabled)
	{
		population_.PrintPartnerships(parameters_, time_, parameters_.trace_files[EventParams::TraceFile::Type::Partnership].file);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::Clinical].enabled)
	{
        population_.PrintClinical(parameters_, time_, parameters_.trace_files[EventParams::TraceFile::Type::Clinical].file);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::Population].enabled)
	{
        population_.PrintPopulation(parameters_, time_, parameters_.trace_files[EventParams::TraceFile::Type::Population].file);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::ArtRollout].enabled)
	{
        population_.PrintARTRolloutOutcomes(parameters_, parameters_.trace_files[EventParams::TraceFile::Type::ArtRollout].file);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::CostEffectiveness].enabled)
	{
        population_.populationStatistics.costsTracker.PrintCosts(parameters_.currTime, parameters_.trace_files[EventParams::TraceFile::Type::CostEffectiveness].file);
	}

	if(parameters_.debugLevel == DebugLevel::One)
	{
		population_.PrintMethodResults(parameters_, "--", "initialization", 0, "--", true);
	}
}

void Simulation::Step()
{
    time_++;

    for(auto &intervention : interventions_)
    {
        if(intervention.IsActive(time_))
        {
            if(intervention.AffectsSimulation())
            {
                intervention.Apply(*this);
            }

            if(intervention.AffectsPopulation())
            {
                intervention.Apply(population_);
            }

            if(intervention.AffectsIndividual())
            {
                population_.entities->forEach([&](Entity *p) { intervention.Apply(p); });
            }
        }
    }

    auto new_end = std::remove_if(interventions_.begin(), interventions_.end(), 
        [=](const Intervention &i) { return i.IsCompleted(time_); });
    interventions_.erase(new_end, interventions_.end());

	if(parameters_.useRollout)
	{
		population_.ApplyRolloutContext(parameters_, time_);
	}

    SimulateMonth();

	//print out new infection stats
	population_.CalcPrevalentPopulation(time_);

    if(parameters_.trace_files[EventParams::TraceFile::Type::Infection].enabled)
	{
        population_.populationStatistics.infectionsTracker.printInfections(parameters_, time_, parameters_.trace_files[EventParams::TraceFile::Type::Infection].file, &population_);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::Population].enabled)
	{
        population_.PrintPopulation(parameters_, time_, parameters_.trace_files[EventParams::TraceFile::Type::Population].file);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::Partnership].enabled)
	{
        population_.PrintPartnerships(parameters_, time_, parameters_.trace_files[EventParams::TraceFile::Type::Partnership].file);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::Clinical].enabled)
	{
        population_.PrintClinical(parameters_, time_, parameters_.trace_files[EventParams::TraceFile::Type::Clinical].file);
	}

	//For now, this must come after infectionsTracker.printInfections as it is what calculate prevalence
    if(parameters_.trace_files[EventParams::TraceFile::Type::ShiftedOutcomes].enabled)
	{
        population_.RecordShiftedOutcomes(parameters_, parameters_.trace_files[EventParams::TraceFile::Type::ShiftedOutcomes].file);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::ArtRollout].enabled)
	{
        population_.PrintARTRolloutOutcomes(parameters_, parameters_.trace_files[EventParams::TraceFile::Type::ArtRollout].file);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::CostEffectiveness].enabled)
	{
        population_.populationStatistics.costsTracker.PrintCosts(parameters_.currTime, parameters_.trace_files[EventParams::TraceFile::Type::CostEffectiveness].file);
	}

	if(parameters_.calibrationInputs.useCalibration && parameters_.calibrationInputs.monthOfCalibration == time_)
	{
		//If this run doesn't pass the partnership calibration stop the run and discard specified trace files
		if(!population_.PassesPartnershipCalibration(parameters_))
		{
			return;
		}
	}

	if(parameters_.calibrationInputs.useCalibration)
	{
		if(!hasPassedFirstMonthCalibPrev_)
		{
            double SAPrev = population_.populationStatistics.infectionsTracker.getSAPrev(population_);

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
				population_.SaveState(parameters_.popStateStream[i], parameters_.currTime);
			}
		}
	}

	population_.ResetMonthlyStats();

    if(time_ > 5)
    {
        run_time_predictor_.Update(std::make_pair(time_, timer_.GetTime() - start_time_));
        int seconds_remaining = (int)run_time_predictor_.GetEstimatedTimeRemaining();
        int hours_remaining = seconds_remaining / 3600;
        seconds_remaining -= hours_remaining * 3600;
        int minutes_remaining = seconds_remaining / 60;
        seconds_remaining -= minutes_remaining * 60;
        parameters_.displayOut(run_time_predictor_.MakeProgressBar(40) + " " +
            std::to_string(time_) + " " + std::to_string(hours_remaining) + ":" +
            std::to_string(minutes_remaining) + ":" + std::to_string(seconds_remaining) + "\n");
    }
    else
    {
        if(time_ > 1)
        {
            run_time_predictor_.Update(std::make_pair(time_, timer_.GetTime() - start_time_));
        }

        parameters_.displayOut("Estimating time remaining...\n");
    }

    start_time_ = timer_.GetTime();

    prevalence_ = population_.GetPopulationStatistics().infectionsTracker.getSAPrev(population_);
    incidence_ = population_.GetPopulationStatistics().infectionsTracker.getCurrTimeStepIncidentInfsTotal() / (double)population_.GetSize();
}

void Simulation::LastStep()
{
	//print survival statistics
    if(parameters_.trace_files[EventParams::TraceFile::Type::Survival].enabled)
	{
        population_.populationStatistics.printSurvivalStats(parameters_.trace_files[EventParams::TraceFile::Type::Survival].file);
	}

	//Run every infected person left through CEPAC until they die
	if(failedCalibration_)
	{
		parameters_.displayOut("Partnership Calibration Failed...Deleting specified trace files...\n");
	}
	else
	{
		parameters_.displayOut("Running all remaining persons through CEPAC until they die...\n");
		population_.UpdateFinalPhysicalState(parameters_);
		parameters_.displayOut("Done!\n");
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::Infection].enabled)
	{
        population_.populationStatistics.printLMStats(parameters_.trace_files[EventParams::TraceFile::Type::Infection].file);
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
        for(auto &trace_file : parameters_.trace_files)
        {
            if(trace_file.second.toss)
            {
                trace_file.second.file.close();
                std::string fileName = parameters_.simName;
                fileName.append("-" + trace_file.second.extension);
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

    outputs_.intervention_outcomes.Write(parameters_.simName + "-InterventionOutcomes.xls");

    std::fstream summary_stream(parameters_.simName + "-IndividualSummaries.json", std::ios::out);
    population_.SaveIndividualSummaries(summary_stream);
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
		auto stem = path(treatment_file.file_name).stem().string();
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
			Entity::probDeathNatCauses[(std::size_t)DemographicProfile::Gender::Male] = probabilities[0];
            Entity::probDeathNatCauses[(std::size_t)DemographicProfile::Gender::Female] = probabilities[1];
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

void Simulation::UpdateInterventions(const std::unordered_set<Entity *> &dead_people)
{
    if(groups_.empty()) return;

    for(auto &group : groups_)
    {
        group.Update(population_, time_, parameters_.randomNums, dead_people);
    }

    outputs_.intervention_outcomes.Update(time_);
}

void Simulation::RegisterTargetGroup(const TargetGroup &group)
{
    groups_.push_back(group);

    if(groups_.size() == 1)
    {
        outputs_.intervention_outcomes.RegisterGroupContainer(groups_);
    }
}

/***
This function executes one timestep of the simulation
The ordering of events within this function determines the ordering of events in each timestep
****/
std::size_t Simulation::SimulateMonth()
{
	parameters_.currTime = time_;

	//change non AIDS death if it is time to switch cepac files
	if(parameters_.itIsTimeToSwitchSimContext() && !parameters_.useRollout)
	{
        int simIndex = 0;

        for(std::size_t i = 0; i < parameters_.cepacSimContexts.size(); i++)
        {
            if(parameters_.currTime > parameters_.timesToSwitchSimContext[i])
            {
                simIndex = static_cast<int>(i);
            }
        }

        SetNonAidsDeathFromCepac(*parameters_.cepacSimContexts[simIndex], 
            Entity::probDeathNatCauses[(std::size_t)DemographicProfile::Gender::Male],
            Entity::probDeathNatCauses[(std::size_t)DemographicProfile::Gender::Female]);
	}

	//output the current timestep of the simulation
    if(parameters_.debugLevel > DebugLevel::One && parameters_.trace_files[EventParams::TraceFile::Type::Events].enabled)
	{
		parameters_.trace_files[EventParams::TraceFile::Type::Events] << "T:" << time_ << " : Start of Timestep" << std::endl;
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled)
	{
		parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << std::endl << "** Time " << time_ << ": " << std::endl;
	}

	bool recordLE = false;
	bool recordPartAcq  =  false;
	bool firstMonthToRecord = false;
	bool lastMonthToRecord = false;

    if(population_.populationStatistics.isTimeToRecordLE(time_))
	{
		recordLE = true;
	}

    if(population_.populationStatistics.isTimeToRecordPartAcq(time_))
	{
		recordPartAcq = true;
	}

    if(population_.populationStatistics.isFirstMonthToRecordLE(time_))
	{
		firstMonthToRecord = true;
	}

    if(population_.populationStatistics.isTimeToPrintLE(time_))
	{
		lastMonthToRecord = true;
	}

	population_.UpdatePhysicalState(parameters_, recordLE, firstMonthToRecord);

    UpdateInterventions(population_.GetDeadPeopleThisMonth());

	if(parameters_.useRollout)
	{
		population_.ApplyARTRollout(parameters_);
	}

	population_.Births(parameters_);

	if(lastMonthToRecord)
	{
		population_.UpdateAgeBucketsLE();

        if(parameters_.trace_files[EventParams::TraceFile::Type::LifeExpectancy].enabled)
		{
            population_.populationStatistics.printLEStats(parameters_.trace_files[EventParams::TraceFile::Type::LifeExpectancy].file, time_);
		}

        delete population_.populationStatistics.selectedLEStats;
        population_.populationStatistics.selectedLEStats = nullptr;
	}

	//steadyCouple, flings, and dissolveSexualPartnerships
	population_.UpdatePartnerships(parameters_);

	if(recordPartAcq)
	{
        population_.populationStatistics.selectedPartAcqStats = new PopulationStatisticsOld::SinglePartAcqStats();
		population_.RecordPartAcqFreq();

        if(parameters_.trace_files[EventParams::TraceFile::Type::PartnerAcquisition].enabled)
		{
            population_.populationStatistics.printPartAcqStats(parameters_.trace_files[EventParams::TraceFile::Type::PartnerAcquisition].file, time_);
		}

        delete population_.populationStatistics.selectedPartAcqStats;
        population_.populationStatistics.selectedPartAcqStats = nullptr;
	}

	//apply incident prevalence
	if(parameters_.delayPrevalence != 0 && parameters_.delayPrevalence == time_)
	{
		population_.ApplyIncidentPrevalence(parameters_);
	}

	//Will confirm that population_.currSize is correct and update size of age ranges
	return population_.UpdateSize();
}

RunStats &Simulation::GetCEPACRunStats()
{
	return *parameters_.cepacRunStats;
}

PopulationStatisticsOld &Simulation::GetPopulationStatistics()
{
    return population_.populationStatistics;
}

EventParams &Simulation::GetEventParams()
{
	return parameters_;
}

void Simulation::Initialize(SimulationParameters &parameters)
{
    name_ = parameters.GetName();
    duration_ = parameters.GetDuration();
    SetFixedSeed(parameters.GetFixedSeed());
    parameters_.simName = name_;
    parameters_.debugLevel = parameters.GetDebugLevel();
    parameters_.monthOf1990 = parameters.GetMonthOf1990();
    parameters_.calibrationInputs = parameters.GetCalibrationParameters();
    parameters_.delayPrevalence = parameters.GetInitialInfectionDelay();
    auto intervention_params = parameters.GetInterventionParameters();
    parameters_.useRollout = intervention_params.intervention_type == InterventionParameters::InterventionType::Art;
    parameters_.enableDynamicTreatmentScaling = intervention_params.dynamic_feedback_enabled;
    parameters_.dynamicFeedbackPeriod = intervention_params.dynamic_feedback_period;
    parameters_.rolloutEligibility = intervention_params.eligibility_criteria;

    auto load_context = [](const std::string &file_name)
    {
        //Set the CEPAC simContext from the specified CEPAC .in file
        auto context = new SimContext(file_name.substr(0, file_name.find(CepacUtil::FILE_EXTENSION_FOR_INPUT)));
        //Don't trace any CEPAC patients -- the output doesn't make any sense and it just gets overly large for no reason
        //TODO: The reason is because the CEPAC Patient number doesn't get updated until the patient dies: this should be changed!
        context->numPatientsToTrace = 0;

        //Read in the inputs
        try
        {
            context->readInputs();
        }
        catch(std::string errorString)
        {
            throw std::runtime_error("error loading rollout file, " + file_name + ": " + errorString);
        }

        return context;
    };

    if(parameters_.useRollout)
    {
        parameters_.untreatedContext = load_context(intervention_params.default_cepac_file.filename);

        for(auto &cepac_file : intervention_params.cepac_files)
        {
            if(cepac_file.time == 0 && cepac_file.target_population == 0) continue; // skip untreated context
            auto context = load_context(cepac_file.filename);
            auto rollout_context = new RolloutContext(cepac_file.time, std::unique_ptr<SimContext>(context), cepac_file.target_population);
            parameters_.rolloutSimContexts.push_back(rollout_context);
        }

        parameters_.cepacTracer = new Tracer(name_, parameters_.untreatedContext, 1);
        parameters_.cepacRunStats = new RunStats(name_, parameters_.untreatedContext);
    }
    else
    {
        parameters_.cepacSimContexts.push_back(load_context(intervention_params.default_cepac_file.filename));

        std::size_t i = 0;
        for(auto &cepac_file : intervention_params.cepac_files)
        {
            auto context = load_context(cepac_file.filename);
            parameters_.timesToSwitchSimContext[i++] = cepac_file.time;
            parameters_.cepacSimContexts.push_back(context);
        }

        parameters_.cepacTracer = new Tracer(name_, parameters_.cepacSimContexts[0], 1);
        parameters_.cepacRunStats = new RunStats(name_, parameters_.cepacSimContexts[0]);
    }

    auto tracing_parameters = parameters.GetTracingParameters();
    parameters_.tracePrevalentCases = tracing_parameters.trace_prevalent_cases;
    parameters_.numToTrace = tracing_parameters.num_to_trace;
    parameters_.numNewbornsTraced = 0;
    parameters_.numNewbornsToTrace = tracing_parameters.num_newborns_to_trace;
    parameters_.monthTraceNewborns = tracing_parameters.month_trace_newborns;

    CepacUtil::changeDirectoryToResults();

    for(auto trace_file : tracing_parameters.files)
    {
        auto string_to_type = [](const std::string &type_string)
        {
            if(type_string == "artRollout") return EventParams::TraceFile::Type::ArtRollout;
            if(type_string == "calibrationStatistics") return EventParams::TraceFile::Type::CalibrationStatistics;
            if(type_string == "clinical") return EventParams::TraceFile::Type::Clinical;
            if(type_string == "costEffectiveness") return EventParams::TraceFile::Type::CostEffectiveness;
            if(type_string == "events") return EventParams::TraceFile::Type::Events;
            if(type_string == "health") return EventParams::TraceFile::Type::Health;
            if(type_string == "infection") return EventParams::TraceFile::Type::Infection;
            if(type_string == "lifeExpectancy") return EventParams::TraceFile::Type::LifeExpectancy;
            if(type_string == "partnerAcquisition") return EventParams::TraceFile::Type::PartnerAcquisition;
            if(type_string == "partnership") return EventParams::TraceFile::Type::Partnership;
            if(type_string == "population") return EventParams::TraceFile::Type::Population;
            if(type_string == "shiftedOutcomes") return EventParams::TraceFile::Type::ShiftedOutcomes;
            if(type_string == "singlePerson") return EventParams::TraceFile::Type::SinglePerson;
            if(type_string == "survival") return EventParams::TraceFile::Type::Survival;
            throw std::runtime_error("invalid trace file name: " + type_string);
        };

        auto type = string_to_type(trace_file.first);
        parameters_.trace_files[type].enabled = trace_file.second.enabled;
        parameters_.trace_files[type].extension = trace_file.second.extension;
        parameters_.trace_files[type].toss = trace_file.second.toss;
        parameters_.trace_files[type].type = type;
        auto file_name = name_ + "-" + trace_file.second.extension;
        parameters_.trace_files[type].file.open(file_name.c_str());
    }

    for(auto intervention : parameters.GetPopulationInterventions())
    {
        RegisterPopulationIntervention(intervention);
    }

    for(auto group : parameters.GetTargetGroups())
    {
        RegisterTargetGroup(group.second);
    }

    population_.Initialize(parameters.GetPopulationParameters());
}

Outputs Simulation::Run(MessageCallback message_callback)
{
    batch_status_.set_state(name_, SimState::running);
    batch_status_.set_process_id(name_, Utility::get_current_process_id());

	MessageCallback old = parameters_.messageCallback;
	parameters_.messageCallback = message_callback;

	if(time_ == 0)
	{
		FirstStep();
	}

	while(time_ < duration_)
	{
		Step();
        batch_status_.set_percent_complete(name_, static_cast<int>(100.0 * time_ / duration_));
	}

	LastStep();

	parameters_.messageCallback = old;
    batch_status_.set_state(name_, SimState::completed);

	return outputs_;
}

} // namespace transm
