#pragma once

#include <functional>
#include <string>
#include <unordered_map>

namespace transm {

class Simulation;
class Population;
class Entity;

class Intervention
{
public:
    Intervention(int time, int duration = -1);

    void SetSimulationCallback(std::function<void(Simulation &)> callback) { simulation_intervention_ = callback; }
    bool AffectsSimulation() const { return (bool)simulation_intervention_; }
    void Apply(Simulation &simulation);

    void SetPopulationCallback(std::function<void(Population &)> callback) { population_intervention_ = callback; }
    bool AffectsPopulation() const { return (bool)population_intervention_; }
    void Apply(Population &population);

    void SetPopulationIndividualCallback(std::function<void(Population &, Entity *)> callback) { population_individual_intervention_ = callback; }
    bool AffectsPopulationIndividual() const { return (bool)population_individual_intervention_; }
    void Apply(Population &population, Entity *person);

    void SetIndividualCallback(std::function<void(Entity *)> callback) { individual_intervention_ = callback; }
    bool AffectsIndividual() const { return (bool)individual_intervention_; }
    void Apply(Entity *person);

    bool IsActive(int current_time) const;
    bool IsFirstMonth(int current_time) const;
    bool IsCompleted(int current_time) const;

private:
    int time_;
    int duration_;
    std::function<void(Simulation &)> simulation_intervention_;
    std::function<void(Population &)> population_intervention_;
    std::function<void(Population &, Entity *)> population_individual_intervention_;
    std::function<void(Entity *)> individual_intervention_;
};

} // namespace transm
