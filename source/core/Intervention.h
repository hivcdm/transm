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
    Intervention(const std::string &parameter, const std::string &value, const std::unordered_map<std::string, std::string> &parameters, bool individual);

    bool AffectsSimulation() const { return (bool)simulation_intervention_; }
    void Apply(Simulation &simulation);

    bool AffectsPopulation() const { return (bool)population_intervention_; }
    void Apply(Population &population);

    bool AffectsPopulationIndividual() const { return (bool)population_individual_intervention_; }
    void Apply(Population &population, Person *person);

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
