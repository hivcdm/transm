#pragma once

#include <functional>
#include <string>
#include <unordered_map>

class Simulation;
class Population;
class Person;

class Intervention
{
public:
    Intervention(int time, int duration);

    int GetTime() { return time_; }
    int GetDuration() { return duration_; }

    void SetSimulationCallback(std::function<void(Simulation &)> callback) { simulation_intervention_ = callback; }
    bool AffectsSimulation() const { return (bool)simulation_intervention_; }
    void Apply(Simulation &simulation);

    void SetPopulationCallback(std::function<void(Population &)> callback) { population_intervention_ = callback; }
    bool AffectsPopulation() const { return (bool)population_intervention_; }
    void Apply(Population &population);

    void SetPopulationIndividualCallback(std::function<void(Population &, Person *)> callback) { population_individual_intervention_ = callback; }
    bool AffectsPopulationIndividual() const { return (bool)population_individual_intervention_; }
    void Apply(Population &population, Person *person);

    void SetIndividualCallback(std::function<void(Person *)> callback) { individual_intervention_ = callback; }
    bool AffectsIndividual() const { return (bool)individual_intervention_; }
    void Apply(Person *person);

    bool IsActive(int current_time) const;
    bool IsFirstMonth(int current_time) const;
    bool IsCompleted(int current_time) const;

private:
    int time_;
    int duration_;
    std::function<void(Simulation &)> simulation_intervention_;
    std::function<void(Population &)> population_intervention_;
    std::function<void(Population &, Person *)> population_individual_intervention_;
    std::function<void(Person *)> individual_intervention_;
};
