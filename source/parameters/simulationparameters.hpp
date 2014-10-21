#pragma once

#include <pugixml.hpp>
#include <rana/rana.hpp>

#include "intervention.hpp"
#include "populationparameters.hpp"
#include "targetgroup.hpp"

namespace transm {

class SimulationParameters
{
public:
    using EntityDistributions = std::unordered_map<std::string, double>;
    using TransmissionCoefficients = std::array<double, Entity::ENDHVLStrata>;
    using TransmissionCoefficientsMap = std::unordered_map<TransmissionType, TransmissionCoefficients>;
    using InterventionsContainer = std::vector<Intervention>;

    SimulationParameters();
    virtual ~SimulationParameters();

    virtual Version GetVersion() = 0;
    virtual PopulationParameters GetPopulationParameters() = 0;
    virtual std::unordered_map<std::string, TargetGroup> GetTargetGroups() = 0;
    virtual EventParams::RolloutEligibility GetRolloutEligibility() = 0;
    virtual TransmissionCoefficientsMap GetTransmissionCoefficients() = 0;
    virtual Female::SubPopParams GetFemaleSubPopParams() = 0;
    virtual Male::SubPopParams GetMaleSubPopParams() = 0;
    virtual Msm::SubPopParams GetMsmSubPopParams() = 0;
    virtual Msmw::SubPopParams GetMsmwSubPopParams() = 0;
    virtual InterventionsContainer GetPopulationInterventions() = 0;
};

class SimulationParametersFactory;

class SimulationParametersXml : public SimulationParameters
{
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
};

class SimulationParametersJson : public SimulationParameters
{
protected:
    friend class SimulationParametersFactory;
    SimulationParametersJson(const std::string &filename);

private:
    rana::value root_;
};

class SimulationParametersFactory
{
public:
    static std::unique_ptr<SimulationParameters> LoadParameters(const std::string &filename);
};

} // namespace transm