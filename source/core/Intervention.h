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
    Intervention(const std::string &parameter, const std::string &value, const std::unordered_map<std::string, std::string> &parameters);

    void Apply(Simulation &s);
    void Apply(Population &p);
    void Apply(Person *p);

    enum class TargetType
    {
        Simulation,
        Population,
        Individual
    };

    TargetType GetType() const { return type_; }
    int GetTime() const { return time_; }

private:
    TargetType type_;
    int time_;
    std::function<void(Simulation &)> simulation_intervention;
    std::function<void(Population &)> population_intervention;
    std::function<void(Person *)> individual_intervention;
};
