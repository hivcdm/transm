#ifndef SIMULATIONPARAMETERS_HPP
#define SIMULATIONPARAMETERS_HPP

#include <pugixml.hpp>
#include <rana/rana.hpp>

#include "parameterdefinitions.hpp"
#include "populationparameters.hpp"
#include "core/intervention.hpp"
#include "core/targetgroup.hpp"
#include "entities/entitytypes.hpp"
#include "entities/demographicprofile.hpp"

namespace transm {

class SimulationParameters
{
public:
    using EntityDistributions = std::vector<DemographicProfile::DoublePair>;
    using TransmissionCoefficients = std::array<double, (std::size_t)HVLStrata::Last>;
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
        __unused double life_expectancy_ci = 0;
        std::vector<int> life_expectancy_record_times;
        std::vector<int> partner_acquisition_record_times;
        std::vector<int> partner_network_record_times;
    };

    SimulationParameters() : rng_(nullptr) { }
    virtual ~SimulationParameters() = default;

    void SetRandomNumberGenerator(RandomNumberGenerator &rng) { rng_ = &rng; }
    RandomNumberGenerator &GetRandomNumberGenerator() const { assert(rng_ != nullptr); return *rng_; }

    virtual std::string GetName() const = 0;

    __unused virtual Version GetVersion() const = 0;
    virtual std::uint32_t GetFixedSeed() const = 0;
    virtual int GetDuration() const = 0;
    virtual int GetMonthOf1990() const = 0;
    virtual int GetInitialInfectionDelay() const = 0;
    virtual ConcurrencyDefinition GetConcurrencyDefinition() const = 0;
    virtual TracingParameters GetTracingParameters() const = 0;
    virtual CalibrationInputs GetCalibrationParameters() const = 0;
    virtual PopulationParameters GetPopulationParameters() const = 0;
    virtual std::unordered_map<std::string, TargetGroup> GetTargetGroups() const = 0;
    virtual TransmissionCoefficientsMap GetTransmissionCoefficients() const = 0;
    virtual Female::SubPopParams GetFemaleSubPopParams() const = 0;
    virtual Male::SubPopParams GetMaleSubPopParams() const = 0;
    virtual InterventionsContainer GetPopulationInterventions() const = 0;
    virtual CepacParameters GetCepacParameters() const = 0;
    virtual PrepParameters GetPrepParameters() const = 0;

private:
    RandomNumberGenerator *rng_;
};

class __unused SimulationParametersFactory;

class SimulationParametersXml : public SimulationParameters
{
public:
    explicit SimulationParametersXml(const path &filename);
    ~SimulationParametersXml() override;

    /*virtual*/ std::string GetName() const override { return name_; }
    /*virtual*/ Version GetVersion() const override;
    /*virtual*/ std::uint32_t GetFixedSeed() const override;
    /*virtual*/ int GetDuration() const override;
    /*virtual*/ int GetMonthOf1990() const override;
    /*virtual*/ int GetInitialInfectionDelay() const override;
    /*virtual*/ ConcurrencyDefinition GetConcurrencyDefinition() const override;
    /*virtual*/ TracingParameters GetTracingParameters() const override;
    /*virtual*/ CalibrationInputs GetCalibrationParameters() const override;
    /*virtual*/ PopulationParameters GetPopulationParameters() const override;
    /*virtual*/ std::unordered_map<std::string, TargetGroup> GetTargetGroups() const override;
    /*virtual*/ TransmissionCoefficientsMap GetTransmissionCoefficients() const override;
    /*virtual*/ Female::SubPopParams GetFemaleSubPopParams() const override;
    /*virtual*/ Male::SubPopParams GetMaleSubPopParams() const override;
    /*virtual*/ InterventionsContainer GetPopulationInterventions() const override;
    /*virtual*/ CepacParameters GetCepacParameters() const override;
    /*virtual*/ PrepParameters GetPrepParameters() const override;

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

    NormalDist GetNormalDist(pugi::xml_node node) const;
    LogNormalDist GetLogNormalDist(pugi::xml_node node) const;
    BetaDist GetBetaDist(pugi::xml_node node) const;
    ShiftedLogNormalDist GetShiftedLogNormalDist(pugi::xml_node node) const;

    void SetChanceCondomUseCallback(pugi::xml_node &node, Intervention &intervention, bool individual) const;
    static void SetProportionCircumcisedCallback(pugi::xml_node &node, Intervention &intervention) ;
    static void SetCircumciseCallback(pugi::xml_node &node, Intervention &intervention, bool individual) ;

    // template function for returning different values when calculating transform
    // values in interventions (transform meaning increase or decrease)
    static double TransformInterventionValue(double target, double curr, Time time, TimeSpan duration, Time current_time);
    static NormalDist TransformInterventionValue(NormalDist target, NormalDist curr, Time time, TimeSpan duration, Time current_time);


    EntityDistributions GetEntityDistributions(pugi::xml_node node) const;
	inline void NormalizeEntityDistributions(PopulationParameters &parameters) const;

    InterventionsContainer GetInterventions(pugi::xml_node interventions_node, bool individual) const;

    Nullable<TargetGroup::PopulationTarget> ParseGroupEligibility(pugi::xml_node criteria_node) const;


    SexualBehavior GetSexualBehavior(const std::string &entity_type, SexualPartnership::Type type) const;

    Intervention GetIntervention(pugi::xml_node &node, bool individual) const;

    RolloutEligibility GetRolloutEligibility() const;
    RolloutDenominator GetRolloutDenominator() const;

    pugi::xml_document document_;
    std::string name_;

    static string SexPartnerType_to_String(SexualPartnership::Type type);
};

// TODO: json file format is not implemented !
class SimulationParametersJson : public SimulationParameters
{
public:
    explicit SimulationParametersJson(const std::string &filename);
    ~SimulationParametersJson() override;

private:
    rana::value root_;
};

} // namespace transm

#endif /* SIMULATIONPARAMETERS_HPP */