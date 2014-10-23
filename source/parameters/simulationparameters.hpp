#pragma once

#include <pugixml.hpp>
#include <rana/rana.hpp>

#include "concurrencydefinition.hpp"
#include "populationparameters.hpp"
#include "core/intervention.hpp"
#include "core/targetgroup.hpp"

namespace transm {

class SimulationParameters
{
public:
    using EntityDistributions = std::unordered_map<std::string, double>;
    using TransmissionCoefficients = std::array<double, Entity::ENDHVLStrata>;
    using TransmissionCoefficientsMap = std::unordered_map<TransmissionType, TransmissionCoefficients>;
    using InterventionsContainer = std::vector<Intervention>;
    using ConcurrencyDefinition = std::array<ConcurrencyDef, Constants::NUMBER_CONCURRENCY_DEFS>;
    using TracingParameters = std::unordered_map < std::string, TraceFileParameters > ;

    SimulationParameters();
    virtual ~SimulationParameters();

    virtual std::string GetName() const = 0;
    virtual Version GetVersion() const = 0;
    virtual std::uint32_t GetFixedSeed() const = 0;
    virtual int GetDuration() const = 0;
    virtual DebugLevel GetDebugLevel() const = 0;
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
    virtual Msm::SubPopParams GetMsmSubPopParams() const = 0;
    virtual Msmw::SubPopParams GetMsmwSubPopParams() const = 0;
    virtual InterventionsContainer GetPopulationInterventions() const = 0;
};

class SimulationParametersFactory;

class SimulationParametersXml : public SimulationParameters
{
public:
    SimulationParametersXml(const std::string &filename);
    virtual ~SimulationParametersXml();

    /*virtual*/ std::string GetName() const = 0;
    /*virtual*/ Version GetVersion() const = 0;
    /*virtual*/ PopulationParameters GetPopulationParameters() const = 0;
    /*virtual*/ std::unordered_map<std::string, TargetGroup> GetTargetGroups() const = 0;
    /*virtual*/ TransmissionCoefficientsMap GetTransmissionCoefficients() const = 0;
    /*virtual*/ Female::SubPopParams GetFemaleSubPopParams() const = 0;
    /*virtual*/ Male::SubPopParams GetMaleSubPopParams() const = 0;
    /*virtual*/ Msm::SubPopParams GetMsmSubPopParams() const = 0;
    /*virtual*/ Msmw::SubPopParams GetMsmwSubPopParams() const = 0;
    /*virtual*/ InterventionsContainer GetPopulationInterventions() const = 0;

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

    NormalDist GetNormalDist(const pugi::xml_node node);
    LogNormalDist GetLogNormalDist(const pugi::xml_node node);
    BetaDist GetBetaDist(const pugi::xml_node node);
    ShiftedLogNormalDist GetShiftedLogNormalDist(const pugi::xml_node node);

    EntityDistributions ReadEntityDistributions(pugi::xml_node node);

    InterventionsContainer ParseInterventions(pugi::xml_node interventions_node, bool individual);

    SexualBehavior ReadSexualBehavior(const std::string &entity_type, SexualPartnership::Type type);

    Intervention ReadIntervention(pugi::xml_node &node, bool individual);

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