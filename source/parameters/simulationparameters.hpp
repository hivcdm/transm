#pragma once

#include <pugixml.hpp>
#include <rana/rana.hpp>

#include "concurrencydefinition.hpp"
#include "populationparameters.hpp"
#include "core/intervention.hpp"
#include "core/targetgroup.hpp"
#include "entities/demographicprofile.hpp"

namespace transm {

class SimulationParameters
{
public:
    using EntityDistributions = std::vector<DemographicProfile::DoublePair>;
    using TransmissionCoefficients = std::array<double, (std::size_t)Entity::HVLStrata::Last>;
    using TransmissionCoefficientsMap = std::unordered_map<TransmissionType, TransmissionCoefficients>;
    using InterventionsContainer = std::vector<Intervention>;
    using ConcurrencyDefinition = std::array<ConcurrencyDef, Constants::NumberConcurrencyDefs>;

    struct TracingParameters
    {
        using FilesContainer = std::unordered_map<std::string, TraceFileParameters>;
        FilesContainer files;
        bool trace_prevalent_cases = false;
        int num_to_trace = 0;
        int num_newborns_to_trace = 0;
        Time month_trace_newborns;
        double life_expectancy_ci = 0;
        std::vector<int> life_expectancy_record_times;
        std::vector<int> partner_acquisition_record_times;
        std::vector<int> partner_network_record_times;
    };

    SimulationParameters() : rng_(nullptr) { }
    virtual ~SimulationParameters() { }

    void SetRandomNumberGenerator(RandomNumberGenerator &rng) { rng_ = &rng; }
    RandomNumberGenerator &GetRandomNumberGenerator() const { assert(rng_ != nullptr); return *rng_; }

    virtual std::string GetName() const = 0;
    virtual Version GetVersion() const = 0;
    virtual std::uint32_t GetFixedSeed() const = 0;
    virtual int GetDuration() const = 0;
    virtual int GetMonthOf1990() const = 0;
    virtual int GetInitialInfectionDelay() const = 0;
    virtual ConcurrencyDefinition GetConcurrencyDefinition() const = 0;
    virtual TracingParameters GetTracingParameters() const = 0;
    virtual CalibrationInputs GetCalibrationParameters() const = 0;
    virtual InterventionParameters GetInterventionParameters() const = 0;
    virtual PopulationParameters GetPopulationParameters() const = 0;
    virtual std::unordered_map<std::string, TargetGroup> GetTargetGroups() const = 0;
    virtual TransmissionCoefficientsMap GetTransmissionCoefficients() const = 0;
    virtual Female::SubPopParams GetFemaleSubPopParams() const = 0;
    virtual Male::SubPopParams GetMaleSubPopParams() const = 0;
    virtual InterventionsContainer GetPopulationInterventions() const = 0;

private:
    RandomNumberGenerator *rng_;
};

class SimulationParametersFactory;

class SimulationParametersXml : public SimulationParameters
{
public:
    SimulationParametersXml(const path &filename);
    virtual ~SimulationParametersXml();

    /*virtual*/ std::string GetName() const { return name_; }
    /*virtual*/ Version GetVersion() const;
    /*virtual*/ std::uint32_t GetFixedSeed() const;
    /*virtual*/ int GetDuration() const;
    /*virtual*/ int GetMonthOf1990() const;
    /*virtual*/ int GetInitialInfectionDelay() const;
    /*virtual*/ ConcurrencyDefinition GetConcurrencyDefinition() const;
    /*virtual*/ TracingParameters GetTracingParameters() const;
    /*virtual*/ CalibrationInputs GetCalibrationParameters() const;
    /*virtual*/ InterventionParameters GetInterventionParameters() const;
    /*virtual*/ PopulationParameters GetPopulationParameters() const;
    /*virtual*/ std::unordered_map<std::string, TargetGroup> GetTargetGroups() const;
    /*virtual*/ TransmissionCoefficientsMap GetTransmissionCoefficients() const;
    /*virtual*/ Female::SubPopParams GetFemaleSubPopParams() const;
    /*virtual*/ Male::SubPopParams GetMaleSubPopParams() const;
    /*virtual*/ InterventionsContainer GetPopulationInterventions() const;

private:
    template<typename T>
    static T from_string(const std::string &value_string);

    template<typename T>
    static T Text(const pugi::xml_node &node)
    {
        return from_string<T>(node.text().as_string());
    }

    template<typename T>
    static T Attr(const pugi::xml_node &node, const std::string &name)
    {
        return from_string<T>(node.attribute(name.c_str()).as_string());
    }

    NormalDist GetNormalDist(const pugi::xml_node node) const;
    LogNormalDist GetLogNormalDist(const pugi::xml_node node) const;
    BetaDist GetBetaDist(const pugi::xml_node node) const;
    ShiftedLogNormalDist GetShiftedLogNormalDist(const pugi::xml_node node) const;

    void SetChanceCondomUseCallback(pugi::xml_node &node, Intervention &intervention, bool individual) const;
    void SetProportionCircumcisedCallback(pugi::xml_node &node, Intervention &intervention) const;
    void SetCircumciseCallback(pugi::xml_node &node, Intervention &intervention, bool individual) const;

    // template function for returning different values when calculating transform
    // values in interventions (transform meaning increase or decrease)
    template<typename V>
    V TransformInterventionValue(V target, V curr, Time time, TimeSpan duration, Time current_time) const;

    EntityDistributions GetEntityDistributions(pugi::xml_node node) const;
	inline void NormalizeEntityDistributions(PopulationParameters &parameters) const;

    InterventionsContainer GetInterventions(pugi::xml_node interventions_node, bool individual) const;

    SexualBehavior GetSexualBehavior(const std::string &entity_type, SexualPartnership::Type type) const;

    Intervention GetIntervention(pugi::xml_node &node, bool individual) const;

    RolloutEligibility GetRolloutEligibility() const;
    RolloutDenominator GetRolloutDenominator() const;

    pugi::xml_document document_;
    std::string name_;
};

class SimulationParametersJson : public SimulationParameters
{
public:
    SimulationParametersJson(const std::string &filename);
    virtual ~SimulationParametersJson();

private:
    rana::value root_;
};

} // namespace transm
