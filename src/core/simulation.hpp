#ifndef SIMULTAION_HPP
#define SIMULTAION_HPP


#include <unordered_set>
#include <pugixml.hpp>

#include "include.h"

#include "batchstatus.hpp"
#include "intervention.hpp"
#include "population.hpp"
#include "targetgroup.hpp"
#include "parameters/eventparams.hpp"
#include "parameters/simulationparameters.hpp"
#include "statistics/outputs.hpp"
#include "statistics/populationstatistics.hpp"
#include "utility/highresolutiontimer.hpp"
#include "utility/nullable.hpp"
#include "utility/runtimepredictor.hpp"

namespace transm {

class Simulation {

public:
    explicit Simulation(BatchStatus &batch_status);

    void Initialize(SimulationParameters &parameters);

    Outputs Run();

    /* returns eventParams.cepacRunStats for adding to the general popstats */
    RunStats &GetCEPACRunStats() const;

    /* returns eventParams.cepacCostStats for adding to the general popstats */
    CostStats &GetCEPACCostStats();

    /* returns population->popStats information for creating popStats-like file for transmission output */
    PopulationStatisticsOld &GetPopulationStatistics();

    EventParams &GetEventParams();

    /** Getters */
    /*@{*/
    double GetPrevalence() const { return prevalence_; }

    double GetIncidence() const { return incidence_; }

    Time GetTime() { return time_; }

    Population &GetPopulation() { return population_; }

    const Population &GetPopulation() const { return population_; }
    /*@}*/

    void RegisterTargetGroup(const TargetGroup &group);

    void SetDuration(TimeSpan duration) { duration_ = duration; }

    void SetFixedSeed(int seed);

    void SetName(const std::string &name) {
        name_ = name;
        parameters_.simName = name;
        batch_status_.set_state(name_, SimState::queued);
    }

    void SetLifeExpectancyConfidenceInterval(double ci) { population_.populationStatistics.setMedianLECI(ci); }

    void AddLifeExpectancyRecordTime(Time time) { population_.populationStatistics.addLifeExpectancyRecordTime(time); }

    void AddPartnerAcquisitionRecordTime(Time time) {
        population_.populationStatistics.addPartnerAcquisitionRecordTime(time);
    }

    void RegisterPopulationIntervention(const Intervention &intervention);


    /* Default deconstructor */
    ~Simulation();

private:
    friend class SimulationBuilderXml;

    friend class Intervention;

    friend class SimulationBuilderXml;

    friend class Intervention;

    void FirstStep();

    void LastStep();

    void Step();

    /** Sets the Non aids death from a cepac simcontext */
    void SetNonAidsDeathFromCepac(SimContext &context, std::vector<double> &male, std::vector<double> &female);

    /** perform one timestep of simulation */
    std::size_t SimulateMonth();

    void UpdateInterventions(const std::unordered_set<Entity *> &dead_people);


private:

    struct TreatmentFile {
        std::string file_name;
        int file_number;
        Time time;
        int target_population;
    };

    std::string name_;

    /** current time in the simulation */
    Time time_;

    /** number of months to run this file in a sequence*/
    TimeSpan duration_;

    /** housekeeping parameters that are universal to each event in the simulation */
    EventParams parameters_;

    /** current population */
    Population population_;

    bool passedCalibration_;

    bool hasPassedFirstMonthCalibPrev_;

    Time monthOfFirstMonthCalibPrev_;

    uint32_t rng_seed_;

    double incidence_;

    double prevalence_;

    HighResolutionTimer timer_;

    Outputs outputs_;

    std::vector<TargetGroup> groups_;

    std::vector<Intervention> interventions_;

    std::vector<std::pair<int, double>> population_size_;

    RunTimePredictor run_time_predictor_;

    double start_time_;

    BatchStatus &batch_status_;


    /** FOCUS module Stuff */
    // --- Yearly Data (loaded from "database") ---
    std::vector<int> yearlyScreeningNumbers;
    std::vector<double> yearlyFocusProbabilities;

    // --- Monthly Data (calculated from yearly) ---
    std::vector<int> monthlyScreeningNumbers;

    // Stores 12 calculated monthly probabilities (one for each group)
    std::vector<double> monthlyFocusProbabilities;

    // Scaling factor for FOCUS data (1.0 = use as-is, <1.0 = scale down for smaller simulations)
    // Set based on your simulation population size relative to real-world population
    double focusDataScaleFactor;

    std::vector<int> db_YearlyCounts_2020;
    std::vector<double> db_YearlyProbs_2020;

    std::vector<int> db_YearlyCounts_2021;
    std::vector<double> db_YearlyProbs_2021;

    std::vector<int> db_YearlyCounts_2022;
    std::vector<double> db_YearlyProbs_2022;

    std::vector<int> db_YearlyCounts_2023;
    std::vector<double> db_YearlyProbs_2023;

    std::vector<int> db_YearlyCounts_2024;
    std::vector<double> db_YearlyProbs_2024;

};

} // namespace transm

#endif /* SIMULATION_HPP */