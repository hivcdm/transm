#include <iostream>
#include <numeric>
#include <random>
#include <set>

#include "include.h"

#include "simulation.hpp"
#include "constants.hpp"
#include "population.hpp"
#include "parameters/eventparams.hpp"
#include "entities/entitytypes.hpp"
#include "entities/demographicprofile.hpp"
#include "entities/sexualbehavior.hpp"

namespace transm {

TargetGroup::PopulationTarget TargetGroup::PopulationTarget::Any;

void Intervention::Apply(Time current_time, Simulation &simulation) {
    if (simulation_intervention_) {
        simulation_intervention_(current_time, simulation);
    }
}

void Intervention::Apply(Time current_time, Population &population) {
    if (population_intervention_) {
        population_intervention_(current_time, population);
    }
}

void Intervention::Apply(Time current_time, Population &population, Entity *person) {
    if (population_individual_intervention_) {
        population_individual_intervention_(current_time, population, person);
    } else if (individual_intervention_) {
        individual_intervention_(current_time, person);
    }
}

void Intervention::Apply(Time current_time, Entity *person) {
    if (individual_intervention_) {
        individual_intervention_(current_time, person);
    }
}

bool Intervention::IsActive(Time current_time) const {
    if (duration_ == TimeSpan(0, -1)) {
        return current_time >= time_;
    }

    return current_time >= time_ && current_time <= time_ + duration_;
}

bool Intervention::IsFirstMonth(Time current_time) const {
    return current_time == time_;
}

bool Intervention::IsCompleted(Time current_time) const {
    return current_time > time_ + duration_;
}

void TargetGroup::Update(Population &population, Time current_time,
                         RandomNumberGenerator &rng, const std::unordered_set<Entity *> &dead_people) {
    if (enrollment_period_.start > current_time) {
        return;
    }

    for (auto person : dead_people) {
        Remove(person);
    }

    auto match = [&](Entity *person) {
        if (target_.has_value) {
            if (target_.value.employment.has_value
                && target_.value.employment.value !=
                   person->getDemographicProfileVal<DemographicProfile::Employment>()) {
                return false;
            }

            if (target_.value.gender.has_value
                && target_.value.gender.value != person->getDemographicProfileVal<DemographicProfile::Gender>()) {
                return false;
            }

            if (target_.value.relationship_status.has_value
                && target_.value.relationship_status.value !=
                   person->getDemographicProfileVal<DemographicProfile::RelationshipStatus>()) {
                return false;
            }

            if (target_.value.sexual_activity_status.has_value
                && target_.value.sexual_activity_status.value !=
                   person->getDemographicProfileVal<DemographicProfile::SexualActivityStatus>()) {
                return false;
            }

            if (target_.value.sexual_orientation.has_value
                && target_.value.sexual_orientation.value !=
                   person->getDemographicProfileVal<DemographicProfile::SexualOrientation>()) {
                return false;
            }

            if (target_.value.age_lower.has_value
                && target_.value.age_lower.value >= person->getAge().in_months()) {
                return false;
            }

            if (target_.value.age_upper.has_value
                && target_.value.age_upper.value <= person->getAge().in_months()) {
                return false;
            }

            if (target_.value.observed_hiv_status.has_value) {
                switch (target_.value.observed_hiv_status.value) {
                    case HIVStatus::ANY_POSITIVE:
                        if (person->getHIVStatus() == HIVStatus::NEGATIVE) {
                            return false;
                        }

                        break;
                    case HIVStatus::ANY_NOT_OBSERVED_POSITIVE: {
                        if (person->getHIVStatus() == HIVStatus::OBSERVED_ACUTE
                            || person->getHIVStatus() == HIVStatus::OBSERVED_LATESTAGE
                            || person->getHIVStatus() == HIVStatus::OBSERVED_CHRONIC) {
                            return false;
                        }

                        break;
                    }
                    case HIVStatus::ANY_OBSERVED_POSITIVE: {
                        if (person->getHIVStatus() == HIVStatus::NEGATIVE
                            || person->getHIVStatus() == HIVStatus::UNOBSERVED_LATESTAGE
                            || person->getHIVStatus() == HIVStatus::UNOBSERVED_CHRONIC
                            || person->getHIVStatus() == HIVStatus::UNOBSERVED_ACUTE) {
                            return false;
                        }

                        break;
                    }
                    default:
                        if (target_.value.observed_hiv_status.value != person->getHIVStatus()) {
                            return false;
                        }

                        break;
                }
            }

            if (target_.value.on_treatment.has_value
                && target_.value.on_treatment.value != person->isOnArt()) {
                return false;
            }

            if (target_.value.circumcised.has_value
                && target_.value.circumcised.value != person->IsCircumcised()) {
                return false;
            }

            if (target_.value.risk_level.has_value
                && target_.value.risk_level.value != person->getRiskLevel()) {
                return false;
            }
        }

        return true;
    };

    if ((open_ && enrollment_period_.end >= current_time) || enrollment_period_.start == current_time) {
        std::vector<Entity *> people;

        for (auto person : population.Find(match)) {
            if (!InGroup(person)) {
                people.push_back(person);
            }
        }

        if (people.empty()) return;

        std::vector<int> assignments;
        std::size_t partition_index = 0;
        std::size_t assigned = 0;
        double assigned_partitions = 0;

        for (auto &partition : partitions_) {
            auto proportion = partition.GetProportion();
            auto number = (int) (proportion * people.size());

            assigned_partitions += proportion;
            // lump rounding errors into the last non-empty partition
            if (assigned_partitions == 1.00 || partition_index == partitions_.size() - 1) {
                number = static_cast<int>(people.size() - assigned);
            }

            std::fill_n(std::back_inserter(assignments), number, (int) partition_index);
            assigned += number;
            partition_index++;
        }

        std::default_random_engine generator(rng.randInt());
        std::uniform_real_distribution<> dist(0, 1);

        auto generate_rand = [&](int i) {
            auto rand_01 = dist(generator);
            return static_cast<int>(rand_01 * i);
        };

        std::shuffle(assignments.begin(), assignments.end(), std::mt19937(std::random_device()()));

        for (std::size_t i = 0; i < people.size(); i++) {
            AssignToPartition(current_time, population, people[i], assignments[i]);
        }
    }
}

Intervention::Intervention(Time time, TimeSpan duration) : time_(time), duration_(duration) {
}

TargetGroup::TargetGroup(const std::string &label, Time start, Time end, bool open,
                         Nullable<PopulationTarget> target)
        : enrollment_period_({start, end}),
          open_(open),
          label_(label),
          target_(target) {

}

void TargetGroup::AddPartition(const std::string &label, bool trace, double proportion,
                               std::vector<Intervention> interventions) {
    Partition p;
    p.label_ = label;
    p.trace_ = trace;
    p.proportion_ = proportion;
    p.interventions_ = interventions;
    partitions_.push_back(p);
}

void Simulation::RegisterPopulationIntervention(const Intervention &intervention) {
    interventions_.push_back(intervention);
}

Simulation::Simulation(BatchStatus &batch_status) :
        parameters_(),
        population_(parameters_),
        passedCalibration_(true),
        hasPassedFirstMonthCalibPrev_(false),
        incidence_(0),
        prevalence_(0),
        batch_status_(batch_status),
        // ==========================================================
        // ===== ADDED: Initialize new data storage vectors =====
        // ==========================================================
        yearlyScreeningNumbers(12),
        yearlyFocusProbabilities(12),
        monthlyScreeningNumbers(12),
        monthlyFocusProbabilities(12),
        yearlyScreenedCount(12, 0),
        yearlyScreeningTarget(12, 0),
        currentFocusYear(-1)
{
        // FOCUS Data Scale Factor
        // Simulation is 2.5x smaller than real life, so scale = 1/2.5 = 0.4
        focusDataScaleFactor = 1.0;

        // Real yearly counts from FOCUS study (these are yearly totals to screen)
        // Schema:
        // [ HM Undiagnosed, HF Undiagnosed, BM Undiagnosed, BF Undiagnosed, WM Undiagnosed, WF Undiagnosed,
        //   HM LTFU,        HF LTFU,        BM LTFU,        BF LTFU,        WM LTFU,        WF LTFU ]

        db_YearlyCounts_2017 = {
            4, 1,   3, 4,   4, 1,
            7, 6,   23, 26,  8, 6
        };

        db_YearlyCounts_2018 = {
            4, 2,   4, 2,   7, 1,
            11, 4,  10, 8,  12, 5
        };

        db_YearlyCounts_2019 = {
            5, 2,   6, 4,   7, 2,
            22, 6,  42, 36, 25, 7
        };

        db_YearlyCounts_2020 = {
            5, 2,   6, 2,   5, 3,
            21, 12, 35, 31, 27, 14
        };

        db_YearlyCounts_2021 = {
            12, 1,  8, 8,   13, 1,
            36, 7,  42, 52, 41, 9
        };

        db_YearlyCounts_2022 = {
            14, 4,  15, 10, 18, 4,
            27, 11, 52, 31, 34, 14
        };

        db_YearlyCounts_2023 = {
            6, 3,   10, 6,  6, 2,
            35, 10, 49, 40, 40, 11
        };

        db_YearlyCounts_2024 = {
            12, 4,  11, 10, 11, 3,
            62, 16, 76, 64, 76, 21
        };

        // Projected years 2025-2035.
        //
        // Source data gives three counts per race x gender per year: Known Positives,
        // New Positives, and Unknown Previous Result Positives. They map onto this
        // schema as:
        //     Undiagnosed = New Positives + Unknown Previous Result Positives
        //     LTFU        = Known Positives
        // A positive with no documented prior result is a new identification, not a
        // re-engagement. This matches the observed 2024 undiagnosed shares.
        //
        // Source values are at real-world scale; the tables above are at simulation
        // scale (2.5x smaller, see focusDataScaleFactor below), so every value here is
        // the source figure divided by 2.5 and rounded. That keeps the 2024/2025
        // boundary continuous.
        //
        // Race "Other" and "Other Gender" are present in the source data but have no
        // slots in this 12-group schema and are excluded (see issue #93). Targets run
        // roughly 3-4% below the source totals as a result.

        db_YearlyCounts_2025 = {
            13, 2,  13, 9,  16, 2,
            74, 20, 84, 72, 90, 23
        };

        db_YearlyCounts_2026 = {
            14, 2,  14, 10, 17, 2,
            81, 22, 86, 74, 98, 24
        };

        db_YearlyCounts_2027 = {
            16, 3,  15, 10, 18, 2,
            88, 23, 89, 76, 107, 26
        };

        db_YearlyCounts_2028 = {
            17, 3,  16, 10, 20, 2,
            94, 24, 92, 77, 115, 27
        };

        db_YearlyCounts_2029 = {
            18, 3,  18, 11, 21, 2,
            101, 26, 95, 79, 123, 28
        };

        db_YearlyCounts_2030 = {
            20, 3,  19, 12, 23, 3,
            108, 27, 98, 80, 131, 30
        };

        db_YearlyCounts_2031 = {
            21, 3,  20, 12, 24, 3,
            114, 28, 100, 82, 139, 31
        };

        db_YearlyCounts_2032 = {
            22, 3,  21, 12, 26, 3,
            121, 30, 104, 84, 147, 33
        };

        db_YearlyCounts_2033 = {
            23, 3,  22, 13, 27, 3,
            127, 31, 106, 86, 155, 34
        };

        db_YearlyCounts_2034 = {
            24, 3,  23, 14, 28, 3,
            134, 33, 109, 88, 163, 36
        };

        db_YearlyCounts_2035 = {
            25, 3,  24, 14, 30, 3,
            140, 34, 112, 89, 172, 37
        };


        // All probabilities are the same at this point but in reality they can be different
        db_YearlyProbs_2017  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};
        db_YearlyProbs_2018  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};
        db_YearlyProbs_2019  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};
        db_YearlyProbs_2020  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};
        db_YearlyProbs_2021  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};
        db_YearlyProbs_2022  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};
        db_YearlyProbs_2023  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};
        db_YearlyProbs_2024  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};
        db_YearlyProbs_2025  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};
        db_YearlyProbs_2026  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};
        db_YearlyProbs_2027  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};
        db_YearlyProbs_2028  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};
        db_YearlyProbs_2029  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};
        db_YearlyProbs_2030  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};
        db_YearlyProbs_2031  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};
        db_YearlyProbs_2032  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};
        db_YearlyProbs_2033  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};
        db_YearlyProbs_2034  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};
        db_YearlyProbs_2035  = {0.8, 0.8, 0.8, 0.8, 0.8, 0.8, 0.69, 0.69, 0.69, 0.69, 0.69, 0.69};

}

Simulation::~Simulation() = default;

void Simulation::SetFixedSeed(int seed) {
    /* Seed is Minnesota Twins retired numbers... yes, I am a dork (Erin, not Thomas!) */
    seed = seed == 0 ? 36291434 : seed;

    if (seed < 0) {
        CepacUtil::setRandomSeedType(true);
        rng_seed_ = (uint32_t) time(0);
    } else {
        CepacUtil::setRandomSeedType(false);
        rng_seed_ = seed;
    }

    parameters_.randomNums.reset(rng_seed_);
}

void Simulation::FirstStep() {
    run_time_predictor_.SetTotalMonths(duration_);

    /* No longer creating a CEPAC trace file, but we still need to change over to the results folder before creating any other output files */
    Utility::changeDirectoryToResults();

    /* output seed used for this run */
    if (parameters_.trace_files[EventParams::TraceFile::Type::Events].enabled) {
        parameters_.trace_files[EventParams::TraceFile::Type::Events] << "Seed = "
                                                                      << parameters_.randomNums.getSeed()
                                                                      << std::endl;
        parameters_.trace_files[EventParams::TraceFile::Type::Events] << Constants::Tab << Constants::Tab
                                                                      << Constants::Tab << Constants::Tab <<
                                                                      Constants::Tab << Constants::Tab
                                                                      << Constants::Tab << Constants::Tab
                                                                      << Constants::Tab
                                                                      << "Sexually Active Population"
                                                                      << Constants::Tab << Constants::Tab
                                                                      << Constants::Tab << Constants::Tab
                                                                      << Constants::Tab << Constants::Tab <<
                                                                      Constants::Tab << Constants::Tab
                                                                      << Constants::Tab << Constants::Tab
                                                                      << Constants::Tab << Constants::Tab <<
                                                                      Constants::Tab << Constants::Tab
                                                                      << Constants::Tab << Constants::Tab
                                                                      << Constants::Tab << Constants::Tab <<
                                                                      "Non Sexually Active Population" << std::endl;
    }

    if (parameters_.trace_files[EventParams::TraceFile::Type::ShiftedOutcomes].enabled) {
        population_.populationStatistics.enableShiftedOutcomes(parameters_.monthOf1990);
    }



    UpdateInterventions(population_.GetDeadPeopleThisMonth());

    /* initialize/reset monthly stats */
    population_.ResetMonthlyStats();

    /* initialize incident infections by age */
    population_.InitIncidentInfectionsByAge();

    if (population_.popWideParams.GetSeedDelay() == Time::Zero) {
        population_.ApplyIncidentPrevalence(parameters_);
    }

    /* print out prevalent infection stats & headers for rest of infection stats */
    population_.CalcPrevalentPopulation(Time::Zero);

    for (auto entity : population_.Find([](Entity *) { return true; })) {
        population_.GetPopulationStatistics().recordEntity(time_, entity);
    }

    if (parameters_.trace_files[EventParams::TraceFile::Type::Infection].enabled) {
        population_.populationStatistics.infectionsTracker.printInfections(parameters_, time_,
                                                                           parameters_.trace_files[EventParams::TraceFile::Type::Infection].file,
                                                                           &population_);
    }

    if (parameters_.trace_files[EventParams::TraceFile::Type::Partnership].enabled) {
        population_.PrintPartnerships(parameters_, time_,
                                      parameters_.trace_files[EventParams::TraceFile::Type::Partnership].file);
    }

    if (parameters_.trace_files[EventParams::TraceFile::Type::Clinical].enabled) {
        population_.PrintClinical(parameters_, time_,
                                  parameters_.trace_files[EventParams::TraceFile::Type::Clinical].file);
    }

    if (parameters_.trace_files[EventParams::TraceFile::Type::Population].enabled) {
        population_.PrintPopulation(parameters_, time_,
                                    parameters_.trace_files[EventParams::TraceFile::Type::Population].file);
    }

    if (parameters_.trace_files[EventParams::TraceFile::Type::ArtRollout].enabled) {
        population_.PrintARTRolloutOutcomes(parameters_,
                                            parameters_.trace_files[EventParams::TraceFile::Type::ArtRollout].file);
    }

    if (parameters_.trace_files[EventParams::TraceFile::Type::PrepOutcomes].enabled) {
        population_.PrintPrepOutcomes(parameters_,
                                      parameters_.trace_files[EventParams::TraceFile::Type::PrepOutcomes].file);
    }

    if (parameters_.trace_files[EventParams::TraceFile::Type::CostEffectiveness].enabled) {
        population_.populationStatistics.costsTracker.PrintCosts(parameters_.currTime,
                                                                 parameters_.trace_files[EventParams::TraceFile::Type::CostEffectiveness].file);
    }
}

void Simulation::Step() {
    time_++;

    for (auto &intervention : interventions_) {
        if (intervention.IsActive(time_)) {
            if (intervention.AffectsSimulation()) {
                intervention.Apply(time_, *this);
            }

            if (intervention.AffectsPopulation()) {
                intervention.Apply(time_, population_);
            }

            if (intervention.AffectsIndividual()) {
                population_.entities->forEach([&](Entity *p) { intervention.Apply(time_, p); });
            }
        }
    }

    auto new_end = std::remove_if(interventions_.begin(), interventions_.end(),
                                  [=](const Intervention &i) { return i.IsCompleted(time_); });
    interventions_.erase(new_end, interventions_.end());


    if (parameters_.useRollout) {
        population_.ApplyRolloutContext(parameters_, time_);
    }

    SimulateMonth();

    /* print out new infection stats */
    population_.CalcPrevalentPopulation(time_);

    population_.entities->forEach([&](Entity *e) {
        population_.GetPopulationStatistics().recordEntity(time_, e);
    });

    if (parameters_.trace_files[EventParams::TraceFile::Type::Infection].enabled) {
        population_.populationStatistics.infectionsTracker.printInfections(parameters_, time_,
                                                                           parameters_.trace_files[EventParams::TraceFile::Type::Infection].file,
                                                                           &population_);
    }

    if (parameters_.trace_files[EventParams::TraceFile::Type::Population].enabled) {
        population_.PrintPopulation(parameters_, time_,
                                    parameters_.trace_files[EventParams::TraceFile::Type::Population].file);
    }

    if (parameters_.trace_files[EventParams::TraceFile::Type::Partnership].enabled) {
        population_.PrintPartnerships(parameters_, time_,
                                      parameters_.trace_files[EventParams::TraceFile::Type::Partnership].file);
    }

    if (parameters_.trace_files[EventParams::TraceFile::Type::Clinical].enabled) {
        population_.PrintClinical(parameters_, time_,
                                  parameters_.trace_files[EventParams::TraceFile::Type::Clinical].file);
    }

    /* For now, this must come after infectionsTracker.printInfections as it is what calculate prevalence */
    if (parameters_.trace_files[EventParams::TraceFile::Type::ShiftedOutcomes].enabled) {
        population_.RecordShiftedOutcomes(parameters_,
                                          parameters_.trace_files[EventParams::TraceFile::Type::ShiftedOutcomes].file);
    }

    if (parameters_.trace_files[EventParams::TraceFile::Type::ArtRollout].enabled) {
        population_.PrintARTRolloutOutcomes(parameters_,
                                            parameters_.trace_files[EventParams::TraceFile::Type::ArtRollout].file);
    }

    if (parameters_.trace_files[EventParams::TraceFile::Type::PrepOutcomes].enabled) {
        population_.PrintPrepOutcomes(parameters_,
                                      parameters_.trace_files[EventParams::TraceFile::Type::PrepOutcomes].file);
    }

    if (parameters_.trace_files[EventParams::TraceFile::Type::CostEffectiveness].enabled) {
        population_.populationStatistics.costsTracker.PrintCosts(parameters_.currTime,
                                                                 parameters_.trace_files[EventParams::TraceFile::Type::CostEffectiveness].file);
    }

    if (parameters_.trace_files[EventParams::TraceFile::Type::PartnerNetwork].enabled) {
        if (std::find(parameters_.partnerNetworkRecordTimes.begin(), parameters_.partnerNetworkRecordTimes.end(),
                      time_.in_months()) != parameters_.partnerNetworkRecordTimes.end()) {

            /* save the partnership network data in the user specified month */
            population_.WritePartnershipNetwork(parameters_);

            population_.PrintPartnershipTracking(parameters_.trace_files
                                                 [EventParams::TraceFile::Type::PartnerAcquisition].file, time_);
        }
    }

    if (parameters_.calibrationInputs.useCalibration) {
        if (parameters_.calibrationInputs.monthOfCalibration == time_) {
            passedCalibration_ = population_.PassesPartnershipCalibration(parameters_);
            string passedCalibrationString = (passedCalibration_ ? "true" : "false");
            std::cerr << "PARTNERSHIP CALIBRATION PASSED: " << passedCalibrationString << endl;

            /* If this run doesn't pass the partnership calibration stop the run and */
            /*  discard specified trace files */
            if (!passedCalibration_) {
                return;
            }
        }

        std::map<Time, std::pair<double, double>>::iterator incidenceRange;
        incidenceRange = parameters_.calibrationInputs.yearlyIncidenceRanges.find(time_);
        if (incidenceRange != parameters_.calibrationInputs.yearlyIncidenceRanges.end()) {
            double incidence = population_.populationStatistics.infectionsTracker.getPopAnnualIncidence();

            std::pair<double, double> range = incidenceRange->second;
            if (incidence < range.first || incidence > range.second) {
                std::cerr << "INCIDENCE CALIBRATION FAILED at time: "
                          << time_.in_months() << " " << incidence << endl;
                passedCalibration_ = false;
                return;
            } else {
                std::cerr << "INCIDENCE CALIBRATION PASSED at time: "
                          << time_.in_months() << " " << incidence << endl;
            }
        }
    }

    population_.ResetMonthlyStats();

    if (time_ > Time(0, 5)) {
        run_time_predictor_.Update(std::make_pair((int) time_.in_months(), timer_.GetTime() - start_time_));
        int seconds_remaining = (int) run_time_predictor_.GetEstimatedTimeRemaining();
        int hours_remaining = seconds_remaining / 3600;
        seconds_remaining -= hours_remaining * 3600;
        int minutes_remaining = seconds_remaining / 60;
        seconds_remaining -= minutes_remaining * 60;
        auto message = run_time_predictor_.MakeProgressBar(40) + " " +
                       std::to_string(time_.in_months()) + " " + std::to_string(hours_remaining) + ":" +
                       std::to_string(minutes_remaining) + ":" + std::to_string(seconds_remaining);
        std::cout << message << std::endl;
    } else if (time_ > Time(0, 1)) {
        run_time_predictor_.Update(std::make_pair(time_.in_months(), timer_.GetTime() - start_time_));
    }

    start_time_ = timer_.GetTime();

    prevalence_ = population_.GetPopulationStatistics().infectionsTracker.getSAPrev(population_);
    incidence_ = population_.GetPopulationStatistics().infectionsTracker.getCurrTimeStepIncidentInfsTotal() /
                 ((double) (population_.GetSize()) - population_.GetNASize());
}

void Simulation::LastStep() {

    /* print survival statistics */
    if (parameters_.trace_files[EventParams::TraceFile::Type::Survival].enabled) {
        population_.populationStatistics.printSurvivalStats(
                parameters_.trace_files[EventParams::TraceFile::Type::Survival].file);
    }

    /* Run every infected person left through CEPAC until they die */
    if (passedCalibration_) {
        population_.UpdateFinalPhysicalState(parameters_);
    }

    if (parameters_.trace_files[EventParams::TraceFile::Type::Infection].enabled) {
        population_.populationStatistics.printLMStats(
                parameters_.trace_files[EventParams::TraceFile::Type::Infection].file);
    }

    /* finalize and print CEPAC output, but only if at least one patient went through CEPAC */
    try {
        if (parameters_.cepacRunStats->getPopulationSummary()->numCohorts > 0) {
            parameters_.cepacRunStats->finalizeStats();
            parameters_.cepacRunStats->writeStatsFile();
            parameters_.cepacCostStats->finalizeStats();
            parameters_.cepacCostStats->writeStatsFile();
        }
    }
    catch (std::string errorString) {
        std::cout << errorString;
//        parameters_.displayOut(errorString);
    }

    /* if failed partnership calibration toss unneeded files */
    if (!passedCalibration_) {
        for (auto &trace_file : parameters_.trace_files) {
            if (trace_file.second.toss) {
                trace_file.second.file.close();
                std::string fileName = parameters_.simName;
                fileName.append("-" + trace_file.second.extension);
                remove(fileName.c_str());
            }
        }
    }

    outputs_.intervention_outcomes.Write(parameters_.simName + "-InterventionOutcomes.xls");

    std::fstream summary_stream(parameters_.simName + "-IndividualSummaries.json", std::ios::out);
    population_.SaveIndividualSummaries(summary_stream);
}

/** Sets the Non aids death from a cepac simcontext */
void Simulation::SetNonAidsDeathFromCepac(SimContext &cepacSimContext, std::vector<double> &male,
                                          std::vector<double> &female) {
    male.clear();
    female.clear();

    for (int i = 0; i <= SimContext::AGE_YRS; i++) {
        male.push_back(cepacSimContext.getNatHistInputs()->monthlyNonAIDSDeathProb[SimContext::GENDER_MALE][i]);
        female.push_back(cepacSimContext.getNatHistInputs()->monthlyNonAIDSDeathProb[SimContext::GENDER_FEMALE][i]);
    }
}

void Simulation::UpdateInterventions(const std::unordered_set<Entity *> &dead_people) {
    if (groups_.empty()) return;

    for (auto &group : groups_) {
        group.Update(population_, time_, parameters_.randomNums, dead_people);
    }

    outputs_.intervention_outcomes.Update(parameters_);
}

void Simulation::RegisterTargetGroup(const TargetGroup &group) {
    groups_.push_back(group);

    if (groups_.size() == 1) {
        outputs_.intervention_outcomes.RegisterGroupContainer(groups_);
    }
}

/** This function executes one timestep of the simulation
The ordering of events within this function determines the ordering of events in each timestep */
std::size_t Simulation::SimulateMonth() {
    parameters_.currTime = time_;

    /* change non AIDS death if it is time to switch cepac files */
    if (parameters_.itIsTimeToSwitchSimContext() && !parameters_.useRollout) {
        int simIndex = 0;

        for (std::size_t i = 0; i < parameters_.cepacSimContexts.size(); i++) {
            if (parameters_.currTime > parameters_.timesToSwitchSimContext[i]) {
                simIndex = static_cast<int>(i);
            }
        }

        SetNonAidsDeathFromCepac(*parameters_.cepacSimContexts[simIndex],
                                 Entity::probDeathNatCauses[(std::size_t) DemographicProfile::Gender::Male],
                                 Entity::probDeathNatCauses[(std::size_t) DemographicProfile::Gender::Female]);
    }

    bool recordLE = false;
    bool recordPartAcq = false;
    bool firstMonthToRecord = false;
    bool lastMonthToRecord = false;

    if (population_.populationStatistics.isTimeToRecordLE(time_)) {
        recordLE = true;
    }

    if (population_.populationStatistics.isTimeToRecordPartAcq(time_)) {
        recordPartAcq = true;
    }

    if (population_.populationStatistics.isFirstMonthToRecordLE(time_)) {
        firstMonthToRecord = true;
    }

    if (population_.populationStatistics.isTimeToPrintLE(time_)) {
        lastMonthToRecord = true;
    }

    UpdateInterventions(population_.GetDeadPeopleThisMonth());

//    if (parameters_.useRollout) {
//        population_.ApplyARTRollout(parameters_);
//    }


    // ====================================================================
    // ===== FOCUS selection: runs BEFORE UpdatePhysicalState so that
    //       newly detected/linked patients go through simulateMonth()
    //       normally and get ART through the standard treatment path =====
    // ====================================================================

    // Get the current month index (0-11)
    // (time_ is 1-based, so month 1 -> index 0, month 12 -> index 11)
    int currentMonthIndex = (time_.in_months() - 1) % 12;

    // if year 2007 is 600 then year 2017 is 600 + (10*12) = 720, 2018 = 732, 2019 = 744, 2020 = 756, 2021 = 768,
    // 2022 = 780, 2023 = 792, 2024 = 804, 2025 = 816, 2026 = 828, 2027 = 840, 2028 = 852, 2029 = 864,
    // 2030 = 876, 2031 = 888, 2032 = 900, 2033 = 912, 2034 = 924, 2035 = 936 (ends at 948)
    int current_month = time_.in_months();

    // FOCUS analysis only runs if enabled via --focus on command line
    if (parameters_.focusEnabled && current_month >= 720 && current_month < 948) {
        int current_year = 2007 + (current_month - 600) / 12;
        int month_in_year = (current_month - 600) % 12 + 1;

        // Detect new year - reset counters and set new targets
        if (current_year != currentFocusYear) {
            currentFocusYear = current_year;

            // Reset yearly screened counts
            std::fill(yearlyScreenedCount.begin(), yearlyScreenedCount.end(), 0);

            // Set yearly targets based on current year (scaled)
            const std::vector<int>* yearData = nullptr;
            if (current_month >= 720 && current_month < 732) {
                yearData = &db_YearlyCounts_2017;
            } else if (current_month >= 732 && current_month < 744) {
                yearData = &db_YearlyCounts_2018;
            } else if (current_month >= 744 && current_month < 756) {
                yearData = &db_YearlyCounts_2019;
            } else if (current_month >= 756 && current_month < 768) {
                yearData = &db_YearlyCounts_2020;
            } else if (current_month >= 768 && current_month < 780) {
                yearData = &db_YearlyCounts_2021;
            } else if (current_month >= 780 && current_month < 792) {
                yearData = &db_YearlyCounts_2022;
            } else if (current_month >= 792 && current_month < 804) {
                yearData = &db_YearlyCounts_2023;
            } else if (current_month >= 804 && current_month < 816) {
                yearData = &db_YearlyCounts_2024;
            } else if (current_month >= 816 && current_month < 828) {
                yearData = &db_YearlyCounts_2025;
            } else if (current_month >= 828 && current_month < 840) {
                yearData = &db_YearlyCounts_2026;
            } else if (current_month >= 840 && current_month < 852) {
                yearData = &db_YearlyCounts_2027;
            } else if (current_month >= 852 && current_month < 864) {
                yearData = &db_YearlyCounts_2028;
            } else if (current_month >= 864 && current_month < 876) {
                yearData = &db_YearlyCounts_2029;
            } else if (current_month >= 876 && current_month < 888) {
                yearData = &db_YearlyCounts_2030;
            } else if (current_month >= 888 && current_month < 900) {
                yearData = &db_YearlyCounts_2031;
            } else if (current_month >= 900 && current_month < 912) {
                yearData = &db_YearlyCounts_2032;
            } else if (current_month >= 912 && current_month < 924) {
                yearData = &db_YearlyCounts_2033;
            } else if (current_month >= 924 && current_month < 936) {
                yearData = &db_YearlyCounts_2034;
            } else if (current_month >= 936 && current_month < 948) {
                yearData = &db_YearlyCounts_2035;
            }

            if (yearData) {
                for (int i = 0; i < 12; ++i) {
                    yearlyScreeningTarget[i] = static_cast<int>(
                        std::round((*yearData)[i] * focusDataScaleFactor)
                    );
                }
            }

            cout << "[FOCUS] ==== NEW FOCUS YEAR " << current_year << " ====" << endl;
            cout << "[FOCUS] Population Scale Factor: " << focusDataScaleFactor
                 << " (sim is " << (1.0/focusDataScaleFactor) << "x smaller than real life)" << endl;
            cout << "[FOCUS] Yearly Targets Set (scaled): ";
            int totalTarget = 0;
            for (int i = 0; i < 12; ++i) {
                totalTarget += yearlyScreeningTarget[i];
            }
            cout << totalTarget << " individuals across 12 groups" << endl;
        }

        // Calculate how many more need to be screened this month to reach yearly goal
        int months_remaining = 13 - month_in_year;  // Including current month

        if (current_month >= 720 && current_month < 732) { // Year 2017
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                // Distribute remaining evenly over remaining months (front-loaded for now)
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2017[i];  // One-time probability at selection
            }

        } else if (current_month >= 732 && current_month < 744) { // Year 2018
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2018[i];  // One-time probability at selection
            }

        } else if (current_month >= 744 && current_month < 756) { // Year 2019
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2019[i];  // One-time probability at selection
            }

        } else if (current_month >= 756 && current_month < 768) { // Year 2020
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                // Distribute remaining evenly over remaining months (front-loaded for now)
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2020[i];  // One-time probability at selection
            }

        } else if (current_month >= 768 && current_month < 780) { // Year 2021
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2021[i];  // One-time probability at selection
            }

        } else if (current_month >= 780 && current_month < 792) { // Year 2022
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2022[i];  // One-time probability at selection
            }

        } else if (current_month >= 792 && current_month < 804) { // Year 2023
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2023[i];  // One-time probability at selection
            }

        } else if (current_month >= 804 && current_month < 816) { // Year 2024
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2024[i];  // One-time probability at selection
            }

        } else if (current_month >= 816 && current_month < 828) { // Year 2025
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2025[i];  // One-time probability at selection
            }

        } else if (current_month >= 828 && current_month < 840) { // Year 2026
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2026[i];  // One-time probability at selection
            }

        } else if (current_month >= 840 && current_month < 852) { // Year 2027
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2027[i];  // One-time probability at selection
            }

        } else if (current_month >= 852 && current_month < 864) { // Year 2028
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2028[i];  // One-time probability at selection
            }

        } else if (current_month >= 864 && current_month < 876) { // Year 2029
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2029[i];  // One-time probability at selection
            }

        } else if (current_month >= 876 && current_month < 888) { // Year 2030
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2030[i];  // One-time probability at selection
            }

        } else if (current_month >= 888 && current_month < 900) { // Year 2031
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2031[i];  // One-time probability at selection
            }

        } else if (current_month >= 900 && current_month < 912) { // Year 2032
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2032[i];  // One-time probability at selection
            }

        } else if (current_month >= 912 && current_month < 924) { // Year 2033
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2033[i];  // One-time probability at selection
            }

        } else if (current_month >= 924 && current_month < 936) { // Year 2034
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2034[i];  // One-time probability at selection
            }

        } else if (current_month >= 936 && current_month < 948) { // Year 2035
            for (int i = 0; i < 12; ++i) {
                int remaining = std::max(0, yearlyScreeningTarget[i] - yearlyScreenedCount[i]);
                monthlyScreeningNumbers[i] = (months_remaining > 0) ?
                    static_cast<int>(std::ceil(static_cast<double>(remaining) / months_remaining)) : 0;
                monthlyFocusProbabilities[i] = db_YearlyProbs_2035[i];  // One-time probability at selection
            }
        }

        cout << "[FOCUS] Month " << month_in_year << "/" << current_year
             << " (sim month " << current_month << ")" << endl;
        cout << "[FOCUS] Starting selection process..." << endl;

        // Run FOCUS selection and get actual screened counts
        std::vector<int> actualScreened = population_.UpdateForFOCUSAnalysis(
            parameters_, monthlyScreeningNumbers, monthlyFocusProbabilities);

        // Update yearly tracking counters
        for (int i = 0; i < 12; ++i) {
            yearlyScreenedCount[i] += actualScreened[i];
        }

        // Print year-to-date progress
        int ytdTotal = 0, targetTotal = 0;
        for (int i = 0; i < 12; ++i) {
            ytdTotal += yearlyScreenedCount[i];
            targetTotal += yearlyScreeningTarget[i];
        }
        cout << "[FOCUS] Year-to-Date Progress: " << ytdTotal << "/" << targetTotal
             << " screened (" << (targetTotal > 0 ? 100.0 * ytdTotal / targetTotal : 0)
             << "% of yearly target)" << endl;
        cout << "[FOCUS] Selection process completed." << endl << endl;
    }

    population_.UpdatePhysicalState(parameters_, recordLE, firstMonthToRecord);

    population_.Births(parameters_);

    if (lastMonthToRecord) {
        population_.UpdateAgeBucketsLE();

        if (parameters_.trace_files[EventParams::TraceFile::Type::LifeExpectancy].enabled) {
            population_.populationStatistics.printLEStats(
                    parameters_.trace_files[EventParams::TraceFile::Type::LifeExpectancy].file, time_);
        }

        delete population_.populationStatistics.selectedLEStats;
        population_.populationStatistics.selectedLEStats = nullptr;
    }

    /* steadyCouple, flings, and dissolveSexualPartnerships */
//    population_.ResetPartnershipTracking();
    population_.UpdatePartnerships(parameters_);

    if (recordPartAcq) {
        population_.populationStatistics.selectedPartAcqStats = new PopulationStatisticsOld::SinglePartAcqStats();
        population_.RecordPartAcqFreq();

//        if (parameters_.trace_files[EventParams::TraceFile::Type::PartnerAcquisition].enabled) {
//            population_.populationStatistics.printPartAcqStats(
//            parameters_.trace_files[EventParams::TraceFile::Type::PartnerAcquisition].file, time_);
//        }

        delete population_.populationStatistics.selectedPartAcqStats;
        population_.populationStatistics.selectedPartAcqStats = nullptr;
    }

    /* apply incident prevalence */
    Time seedDelay = population_.popWideParams.GetSeedDelay();
    if (seedDelay != Time::Zero && seedDelay.in_months() == time_.in_months()) {
        population_.ApplyIncidentPrevalence(parameters_);
    }
    
    /* Will confirm that population_.currSize is correct and update size of age ranges */
    return population_.UpdateSize();
}

RunStats &Simulation::GetCEPACRunStats() const {
    return *parameters_.cepacRunStats;
}

CostStats &Simulation::GetCEPACCostStats() {
    return *parameters_.cepacCostStats;
}

PopulationStatisticsOld &Simulation::GetPopulationStatistics() {
    return population_.populationStatistics;
}

EventParams &Simulation::GetEventParams() {
    return parameters_;
}

void Simulation::Initialize(SimulationParameters &parameters) {
    name_ = parameters.GetName();
    duration_ = TimeSpan(0, parameters.GetDuration());
    SetFixedSeed(parameters.GetFixedSeed());
    parameters_.simName = name_;
    parameters_.monthOf1990 = Time::from_months(parameters.GetMonthOf1990());
    parameters_.calibrationInputs = parameters.GetCalibrationParameters();
    parameters_.delayPrevalence = Time::from_months(parameters.GetInitialInfectionDelay());
    auto cepac_params = parameters.GetCepacParameters();
    for (const auto &prop : cepac_params.target_yearly_rollout_proportions) {
        parameters_.targetYearlyRolloutProportions[prop.first] = prop.second;
    }
    parameters_.concurrencyDef = parameters.GetConcurrencyDefinition();
    parameters_.useRollout = (cepac_params.file_type == CepacParameters::FileType::Art);
    parameters_.enableDynamicTreatmentScaling = cepac_params.dynamic_feedback_enabled;
    parameters_.dynamicFeedbackPeriod = cepac_params.dynamic_feedback_period;
    parameters_.rolloutEligibility = cepac_params.eligibility_criteria;
    parameters_.rolloutProportionDenominator = cepac_params.rollout_proportion_denominator;

    auto load_context = [](const std::string &file_name) {

        /* Set the CEPAC simContext from the specified CEPAC .in file */
        auto context = new SimContext(file_name.substr(0, file_name.find(CepacUtil::FILE_EXTENSION_FOR_INPUT)));

        /* Don't trace any CEPAC patients -- the output doesn't make any sense and it just gets overly large for no reason */
        /* TODO: The reason is because the CEPAC Patient number doesn't get updated until the patient dies: this should be changed! */
        context->numPatientsToTrace = 0;

        //Read in the inputs
        try {
            context->readInputs();
        }
        catch (std::string errorString) {
            throw std::runtime_error("error loading rollout file, " + file_name + ": " + errorString);
        }

        return context;
    };

    if (parameters_.useRollout) {
        parameters_.untreatedContext = load_context(cepac_params.default_cepac_file.filename);

        for (auto &cepac_file : cepac_params.cepac_files) {
            if (cepac_file.time == Time::Zero && cepac_file.target_population == 0)
                continue; // skip untreated context
            auto context = load_context(cepac_file.filename);
            auto rollout_context = new RolloutContext(cepac_file.time, context,
                                                      cepac_file.target_population);
            parameters_.timesToSwitchSimContext.push_back(cepac_file.time);
            parameters_.rolloutSimContexts.push_back(rollout_context);
        }

        /** Sorting the rollout contexts with respect to their timeToApply **/
//        sort(parameters_.rolloutSimContexts.begin(), parameters_.rolloutSimContexts.end(),
//             [] (RolloutContext * a, RolloutContext * b) -> bool
//        {
//            return a->timeToApply.in_months() > b->timeToApply.in_months();
//        });

        parameters_.cepacTracer = new Tracer(name_, parameters_.untreatedContext, 1);
        parameters_.cepacRunStats = new RunStats(name_, parameters_.untreatedContext);
        parameters_.cepacCostStats = new CostStats(name_, parameters_.untreatedContext);
    } else {
        parameters_.cepacSimContexts.push_back(load_context(cepac_params.default_cepac_file.filename));

        for (auto &cepac_file : cepac_params.cepac_files) {
            auto context = load_context(cepac_file.filename);
            parameters_.timesToSwitchSimContext.push_back(cepac_file.time);
            parameters_.cepacSimContexts.push_back(context);
        }

        parameters_.cepacTracer = new Tracer(name_, parameters_.cepacSimContexts[0], 1);
        parameters_.cepacRunStats = new RunStats(name_, parameters_.cepacSimContexts[0]);
        parameters_.cepacCostStats = new CostStats(name_, parameters_.cepacSimContexts[0]);
    }

    auto tracing_parameters = parameters.GetTracingParameters();
    parameters_.tracePrevalentCases = tracing_parameters.trace_prevalent_cases;
    parameters_.numToTrace = tracing_parameters.num_to_trace;
    parameters_.numNewbornsTraced = 0;
    parameters_.numNewbornsToTrace = tracing_parameters.num_newborns_to_trace;
    parameters_.monthTraceNewborns = tracing_parameters.month_trace_newborns;
    parameters_.partnerNetworkRecordTimes = tracing_parameters.partner_network_record_times;

    Utility::changeDirectoryToResults();

    for (const auto& trace_file : tracing_parameters.files) {
        auto string_to_type = [](const std::string &type_string) {
            if (type_string == "artRollout") return EventParams::TraceFile::Type::ArtRollout;
            if (type_string == "calibrationStatistics") return EventParams::TraceFile::Type::CalibrationStatistics;
            if (type_string == "clinical") return EventParams::TraceFile::Type::Clinical;
            if (type_string == "costEffectiveness") return EventParams::TraceFile::Type::CostEffectiveness;
            if (type_string == "events") return EventParams::TraceFile::Type::Events;
            if (type_string == "infection") return EventParams::TraceFile::Type::Infection;
            if (type_string == "lifeExpectancy") return EventParams::TraceFile::Type::LifeExpectancy;
            if (type_string == "partnerAcquisition") return EventParams::TraceFile::Type::PartnerAcquisition;
            if (type_string == "partnerNetwork") return EventParams::TraceFile::Type::PartnerNetwork;
            if (type_string == "partnership") return EventParams::TraceFile::Type::Partnership;
            if (type_string == "population") return EventParams::TraceFile::Type::Population;
            if (type_string == "prepOutcomes") return EventParams::TraceFile::Type::PrepOutcomes;
            if (type_string == "shiftedOutcomes") return EventParams::TraceFile::Type::ShiftedOutcomes;
            if (type_string == "singlePerson") return EventParams::TraceFile::Type::SinglePerson;
            if (type_string == "survival") return EventParams::TraceFile::Type::Survival;
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

    for (const auto& intervention : parameters.GetPopulationInterventions()) {
        RegisterPopulationIntervention(intervention);
    }

    for (const auto& group : parameters.GetTargetGroups()) {
        RegisterTargetGroup(group.second);
    }

    population_.Initialize(parameters.GetPopulationParameters());
}

Outputs Simulation::Run() {
    batch_status_.set_state(name_, SimState::running);
    batch_status_.set_process_id(name_, Utility::get_current_process_id());

    if (time_ == Time::Zero) {
        FirstStep();
    }

    auto end_time = Time::Zero + duration_;
    while (time_ < end_time && passedCalibration_) {
        Step();
        auto percent = static_cast<int>(100.0 * time_.in_months() / duration_.in_months());
        batch_status_.set_percent_complete(name_, percent);
    }

    LastStep();
    batch_status_.set_state(name_, SimState::completed);

    return outputs_;
}

}// namespace transm
