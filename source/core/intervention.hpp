#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "utility/time.hpp"

namespace transm {

class Simulation;
class Population;
class Entity;

class Intervention
{
public:
    Intervention(Time time, TimeSpan duration = TimeSpan(0, -1));

    void SetSimulationCallback(std::function<void(Time, Simulation &)> callback) { simulation_intervention_ = callback; }
    bool AffectsSimulation() const { return (bool)simulation_intervention_; }
    void Apply(Time current_time, Simulation &simulation);

    void SetPopulationCallback(std::function<void(Time, Population &)> callback) { population_intervention_ = callback; }
    bool AffectsPopulation() const { return (bool)population_intervention_; }
    void Apply(Time current_time,Population &population);

    void SetPopulationIndividualCallback(std::function<void(Time, Population &, Entity *)> callback) { population_individual_intervention_ = callback; }
    bool AffectsPopulationIndividual() const { return (bool)population_individual_intervention_; }
    void Apply(Time current_time, Population &population, Entity *person);

    void SetIndividualCallback(std::function<void(Time, Entity *)> callback) { individual_intervention_ = callback; }
    bool AffectsIndividual() const { return (bool)individual_intervention_; }
    void Apply(Time current_time, Entity *person);

    bool IsActive(Time current_time) const;
    bool IsFirstMonth(Time current_time) const;
    bool IsCompleted(Time current_time) const;

private:
    Time time_;
    TimeSpan duration_;
    std::function<void(Time, Simulation &)> simulation_intervention_;
    std::function<void(Time, Population &)> population_intervention_;
    std::function<void(Time, Population &, Entity *)> population_individual_intervention_;
    std::function<void(Time, Entity *)> individual_intervention_;
};

} // namespace transm
